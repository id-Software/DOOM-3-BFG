/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_app.c - startup order follows idCommonLocal::Init (Common.cpp:936):
   command line, cvar/cmd registration, startup "+set" variables, file
   system, default.cfg, the user config, autoexec.cfg, video, then the
   remaining "+" commands. M0 has no game or renderer: the frame loop pumps
   events, runs the command buffer, clears the swapchain and draws the
   diagnostics overlay. The fixed 60 Hz game tick arrives with M2. */

typedef struct Rgd3App
{
	int quit;
	int fully_initialized;
	int self_test; /* --self-test: no window, run the checks, exit */
	Rgd3CmdArgs console_lines[RGD3_MAX_CONSOLE_LINES];
	int num_console_lines;

	SDL_Window* window;
	SDL_GPUDevice* gpu;
	SDL_GPUTextureFormat swapchain_format;
	int overlay_ok;
	int mouse_grabbed;
	char shader_root[RGD3_MAX_OSPATH];

	u64 frame_number;
	u64 fps_window_start_us;
	int fps_window_frames;
	double fps;
	double frame_ms;
	u64 last_frame_us;
} Rgd3App;

static Rgd3App rgd3_app;

static Rgd3Cvar cv_com_showFPS = RGD3_CVAR("com_showFPS", "1", RGD3_CVAR_BOOL | RGD3_CVAR_SYSTEM | RGD3_CVAR_ARCHIVE | RGD3_CVAR_NOCHEAT, "show frames rendered per second");
static Rgd3Cvar cv_r_windowWidth = RGD3_CVAR("r_windowWidth", "1280", RGD3_CVAR_RENDERER | RGD3_CVAR_ARCHIVE | RGD3_CVAR_INTEGER, "non-fullscreen window width");
static Rgd3Cvar cv_r_windowHeight = RGD3_CVAR("r_windowHeight", "720", RGD3_CVAR_RENDERER | RGD3_CVAR_ARCHIVE | RGD3_CVAR_INTEGER, "non-fullscreen window height");
static Rgd3Cvar cv_r_fullscreen = RGD3_CVAR("r_fullscreen", "0", RGD3_CVAR_RENDERER | RGD3_CVAR_ARCHIVE | RGD3_CVAR_BOOL, "0 = windowed, 1 = borderless fullscreen desktop");
static Rgd3Cvar cv_r_swapInterval = RGD3_CVAR_RANGE("r_swapInterval", "1", RGD3_CVAR_RENDERER | RGD3_CVAR_ARCHIVE | RGD3_CVAR_INTEGER, "0 = tear, 1 = swap-locked", 0, 1);
static Rgd3Cvar cv_r_gpuDriver = RGD3_CVAR("r_gpuDriver", "", RGD3_CVAR_RENDERER | RGD3_CVAR_ARCHIVE | RGD3_CVAR_INIT, "SDL GPU driver: direct3d12, vulkan, or empty for automatic");
static Rgd3Cvar cv_r_gpuDebug = RGD3_CVAR("r_gpuDebug", "0", RGD3_CVAR_RENDERER | RGD3_CVAR_BOOL | RGD3_CVAR_INIT, "enable the SDL GPU debug layer");
static Rgd3Cvar cv_in_nograb = RGD3_CVAR("in_nograb", "0", RGD3_CVAR_SYSTEM | RGD3_CVAR_NOCHEAT, "prevents input grabbing");
static Rgd3Cvar cv_r_shaderRoot = RGD3_CVAR("r_shaderRoot", "", RGD3_CVAR_RENDERER | RGD3_CVAR_INIT, "directory containing Compiled/{DXIL,SPIRV,MSL}; empty = <exe>/shaders");

SDL_Window* rgd3_app_window(void)
{
	return rgd3_app.window;
}

SDL_GPUDevice* rgd3_app_gpu(void)
{
	return rgd3_app.gpu;
}

void rgd3_app_quit(void)
{
	rgd3_app.quit = 1;
}

/* the error drop hook: Common.cpp Error() calls Stop() before throwing */
void rgd3_app_stop_map(void)
{
	rgd3_common_stop(1);
}

/* Sys_GrabMouseCursor: relative mouse mode while playing (in_nograb 1
   keeps the cursor free, for debugging) */
void rgd3_app_grab_mouse(int grab)
{
	int want = grab && !cv_in_nograb.integer_value && rgd3_app.window != NULL;
	if (!rgd3_app.window || want == rgd3_app.mouse_grabbed)
		return;
	SDL_SetWindowRelativeMouseMode(rgd3_app.window, want ? true : false);
	rgd3_app.mouse_grabbed = want;
}

/* ---------------------------------------------------------------------------
   Command line (idCommonLocal::ParseCommandLine / StartupVariable /
   AddStartupCommands)
--------------------------------------------------------------------------- */

static void rgd3_app_parse_command_line(int argc, char** argv)
{
	int i;
	rgd3_app.num_console_lines = 0;
	for (i = 1; i < argc; ++i)
	{
		/* "--" switches are not console commands */
		if (argv[i][0] == '-' && argv[i][1] == '-')
		{
			if (!rgd3_str_icmp(argv[i], "--self-test"))
				rgd3_app.self_test = 1;
			else
				rgd3_warning("unknown switch %s", argv[i]);
			continue;
		}
		if (argv[i][0] == '+')
		{
			if (rgd3_app.num_console_lines >= RGD3_MAX_CONSOLE_LINES)
				break;
			rgd3_cmdargs_clear(&rgd3_app.console_lines[rgd3_app.num_console_lines]);
			++rgd3_app.num_console_lines;
			rgd3_cmdargs_append(&rgd3_app.console_lines[rgd3_app.num_console_lines - 1], argv[i] + 1);
		}
		else
		{
			if (!rgd3_app.num_console_lines)
			{
				rgd3_cmdargs_clear(&rgd3_app.console_lines[0]);
				rgd3_app.num_console_lines = 1;
			}
			rgd3_cmdargs_append(&rgd3_app.console_lines[rgd3_app.num_console_lines - 1], argv[i]);
		}
	}
}

/* "safe" or "cvar_restart" on the command line skips the user config. */
static int rgd3_app_safe_mode(void)
{
	int i;
	for (i = 0; i < rgd3_app.num_console_lines; ++i)
	{
		const char* cmd = rgd3_cmdargs_argv(&rgd3_app.console_lines[i], 0);
		if (rgd3_str_icmp(cmd, "safe") == 0 || rgd3_str_icmp(cmd, "cvar_restart") == 0)
		{
			rgd3_cmdargs_clear(&rgd3_app.console_lines[i]);
			return 1;
		}
	}
	return 0;
}

/* Applies "+set name value" (and seta/sets/sett/setu) lines now; match
   restricts it to one cvar name. */
static void rgd3_app_startup_variable(const char* match)
{
	int i;
	for (i = 0; i < rgd3_app.num_console_lines; ++i)
	{
		const Rgd3CmdArgs* line = &rgd3_app.console_lines[i];
		const char* cmd = rgd3_cmdargs_argv(line, 0);
		const char* name;
		if (rgd3_str_icmpn(cmd, "set", 3) != 0 || strlen(cmd) > 4 || rgd3_cmdargs_argc(line) < 3)
			continue;
		name = rgd3_cmdargs_argv(line, 1);
		if (match && rgd3_str_icmp(name, match) != 0)
			continue;
		rgd3_cvar_set_string(name, rgd3_cmdargs_args(line, 2, -1, 0), rgd3_str_icmp(cmd, "seta") == 0 ? RGD3_CVAR_ARCHIVE : 0);
	}
}

static void rgd3_app_add_startup_commands(void)
{
	int i;
	for (i = 0; i < rgd3_app.num_console_lines; ++i)
	{
		const Rgd3CmdArgs* line = &rgd3_app.console_lines[i];
		const char* cmd = rgd3_cmdargs_argv(line, 0);
		if (!rgd3_cmdargs_argc(line))
			continue;
		if (rgd3_str_icmpn(cmd, "set", 3) == 0 && strlen(cmd) <= 4)
			continue;
		rgd3_cmd_buffer_text(RGD3_CMD_EXEC_APPEND, rgd3_cmdargs_args(line, 0, -1, 1));
		rgd3_cmd_buffer_text(RGD3_CMD_EXEC_APPEND, "\n");
	}
}

/* ---------------------------------------------------------------------------
   Configuration (idCommonLocal::WriteConfiguration)
--------------------------------------------------------------------------- */

void rgd3_app_write_configuration(int force)
{
	Rgd3File* f;
	if (!rgd3_app.fully_initialized)
		return;
	if (!force && !(rgd3_cvar_modified_flags() & RGD3_CVAR_ARCHIVE) && !rgd3_keys_bindings_modified())
		return;
	rgd3_cvar_clear_modified_flags(RGD3_CVAR_ARCHIVE);
	rgd3_keys_clear_bindings_modified();
	f = rgd3_fs_open_write(RGD3_CONFIG_FILE);
	if (!f)
		return;
	rgd3_fs_printf(f, "// generated by " RGD3_ENGINE_NAME ", do not modify\n");
	rgd3_keys_write_bindings(f);
	rgd3_cvar_write_flagged(RGD3_CVAR_ARCHIVE, "seta", f);
	rgd3_fs_close(f);
}

static void rgd3_app_cmd_quit(const Rgd3CmdArgs* args)
{
	RG_UNUSED(args);
	rgd3_app_quit();
}

static void rgd3_app_cmd_write_config(const Rgd3CmdArgs* args)
{
	RG_UNUSED(args);
	rgd3_app_write_configuration(1);
	rgd3_printf("Writing %s.\n", RGD3_CONFIG_FILE);
}

static void rgd3_app_cmd_version(const Rgd3CmdArgs* args)
{
	RG_UNUSED(args);
	rgd3_printf(RGD3_ENGINE_NAME " " RGD3_ENGINE_VERSION " (" RGD3_BUILD_MODE ") built " __DATE__ " " __TIME__ "\n");
}

/* ---------------------------------------------------------------------------
   Video
--------------------------------------------------------------------------- */

static void rgd3_app_apply_present_mode(void)
{
	SDL_GPUPresentMode mode = SDL_GPU_PRESENTMODE_VSYNC;
	if (!cv_r_swapInterval.integer_value)
	{
		if (SDL_WindowSupportsGPUPresentMode(rgd3_app.gpu, rgd3_app.window, SDL_GPU_PRESENTMODE_IMMEDIATE))
			mode = SDL_GPU_PRESENTMODE_IMMEDIATE;
		else if (SDL_WindowSupportsGPUPresentMode(rgd3_app.gpu, rgd3_app.window, SDL_GPU_PRESENTMODE_MAILBOX))
			mode = SDL_GPU_PRESENTMODE_MAILBOX;
	}
	if (!SDL_SetGPUSwapchainParameters(rgd3_app.gpu, rgd3_app.window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, mode))
		rgd3_warning("SDL_SetGPUSwapchainParameters failed: %s", SDL_GetError());
}

static void rgd3_app_init_video(void)
{
	RgWindowDesc wd = {0};
	RgGpuDeviceDesc gd = {0};
	const char* driver;
	rgd3_printf("------- Initializing video -------\n");
	wd.title = "rgd3 - DOOM 3 BFG";
	wd.width = cv_r_windowWidth.integer_value > 0 ? cv_r_windowWidth.integer_value : 1280;
	wd.height = cv_r_windowHeight.integer_value > 0 ? cv_r_windowHeight.integer_value : 720;
	wd.flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
	wd.min_width = 320;
	wd.min_height = 240;
	rgd3_app.window = rg_window_create(&wd);
	if (!rgd3_app.window)
		rgd3_error_fatal("could not create the window: %s", SDL_GetError());

	gd.name = cv_r_gpuDriver.value[0] ? cv_r_gpuDriver.value : NULL;
	gd.enable_debug = cv_r_gpuDebug.integer_value;
	rgd3_app.gpu = rg_gpu_device_create(&gd);
	if (!rgd3_app.gpu)
		rgd3_error_fatal("could not create an SDL GPU device%s%s: %s", gd.name ? " for driver " : "", gd.name ? gd.name : "", SDL_GetError());
	if (!rg_gpu_claim_window(rgd3_app.gpu, rgd3_app.window))
		rgd3_error_fatal("could not claim the window for the GPU device: %s", SDL_GetError());
	driver = SDL_GetGPUDeviceDriver(rgd3_app.gpu);
	rgd3_printf("GPU driver: %s\n", driver ? driver : "?");
	rgd3_app_apply_present_mode();
	if (cv_r_fullscreen.integer_value)
		rg_window_set_fullscreen(rgd3_app.window, 1);
	rgd3_app.swapchain_format = rg_gpu_swapchain_format(rgd3_app.gpu, rgd3_app.window);

	if (cv_r_shaderRoot.value[0])
		rgd3_str_copynz(rgd3_app.shader_root, cv_r_shaderRoot.value, sizeof(rgd3_app.shader_root));
	else
		rg_snprintf(rgd3_app.shader_root, sizeof(rgd3_app.shader_root), "%s/shaders", rgd3_platform_exe_dir());
	rgd3_app.overlay_ok = rgd3_overlay_create(rgd3_app.gpu, rgd3_app.swapchain_format, rgd3_app.shader_root);
	rgd3_printf("----------------------------------\n");
}

static void rgd3_app_shutdown_video(void)
{
	if (rgd3_app.gpu)
		rg_gpu_wait_idle(rgd3_app.gpu);
	rgd3_overlay_destroy();
	if (rgd3_app.gpu && rgd3_app.window)
		SDL_ReleaseWindowFromGPUDevice(rgd3_app.gpu, rgd3_app.window);
	if (rgd3_app.gpu)
		rg_gpu_device_destroy(rgd3_app.gpu);
	if (rgd3_app.window)
		rg_window_destroy(rgd3_app.window);
	rgd3_app.gpu = NULL;
	rgd3_app.window = NULL;
}

static void rgd3_app_check_video_cvars(void)
{
	if (rgd3_cvar_is_modified(&cv_r_fullscreen))
	{
		rgd3_cvar_clear_modified(&cv_r_fullscreen);
		rg_window_set_fullscreen(rgd3_app.window, cv_r_fullscreen.integer_value);
	}
	if (rgd3_cvar_is_modified(&cv_r_swapInterval))
	{
		rgd3_cvar_clear_modified(&cv_r_swapInterval);
		rgd3_app_apply_present_mode();
	}
}

static void rgd3_app_update_fps(void)
{
	u64 now = rgd3_time_us();
	if (rgd3_app.last_frame_us)
		rgd3_app.frame_ms = (double)(now - rgd3_app.last_frame_us) / 1000.0;
	rgd3_app.last_frame_us = now;
	if (!rgd3_app.fps_window_start_us)
		rgd3_app.fps_window_start_us = now;
	++rgd3_app.fps_window_frames;
	if (now - rgd3_app.fps_window_start_us >= 500000)
	{
		rgd3_app.fps = (double)rgd3_app.fps_window_frames * 1000000.0 / (double)(now - rgd3_app.fps_window_start_us);
		rgd3_app.fps_window_frames = 0;
		rgd3_app.fps_window_start_us = now;
	}
}

static void rgd3_app_render_frame(void)
{
	SDL_GPUCommandBuffer* cmd;
	SDL_GPUTexture* swapchain = NULL;
	u32 width = 0, height = 0;
	cmd = SDL_AcquireGPUCommandBuffer(rgd3_app.gpu);
	if (!cmd)
		return;
	if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmd, rgd3_app.window, &swapchain, &width, &height))
	{
		SDL_CancelGPUCommandBuffer(cmd);
		return;
	}
	if (swapchain)
	{
		SDL_GPUColorTargetInfo color = {0};
		SDL_GPURenderPass* pass;
		color.texture = swapchain;
		color.load_op = SDL_GPU_LOADOP_CLEAR;
		color.store_op = SDL_GPU_STOREOP_STORE;
		color.clear_color = (SDL_FColor){ 0.03f, 0.03f, 0.04f, 1.0f };
		pass = SDL_BeginGPURenderPass(cmd, &color, 1, NULL);
		if (pass)
			SDL_EndGPURenderPass(pass);
		/* one overlay batch per frame: the console, then the diagnostics */
		if (rgd3_app.overlay_ok && rgd3_overlay_begin(width, height))
		{
			rgd3_console_draw(0);
			if (cv_com_showFPS.integer_value)
			{
				char fps[48], frame[48], mode[64], game[RGD3_MAX_OSPATH + 64];
				const char* lines[4];
				const char* driver = SDL_GetGPUDeviceDriver(rgd3_app.gpu);
				rg_snprintf(fps, sizeof(fps), "FPS %.1f", rgd3_app.fps);
				rg_snprintf(frame, sizeof(frame), "FRAME %.2f MS", rgd3_app.frame_ms);
				rg_snprintf(mode, sizeof(mode), "%s %uX%u %s", driver ? driver : "", width, height, cv_r_swapInterval.integer_value ? "VSYNC" : "NOVSYNC");
				/* until the renderer draws the world: what the game loop is doing */
				if (rgd3_common_map_spawned())
					rg_snprintf(game, sizeof(game), "%s %s TIME %.2f", rgd3_common_paused() ? "PAUSED" : "IN GAME",
						rgd3_common_current_map_name(), rgd3_game_get_server_game_time_ms() / 1000.0);
				else
					rg_snprintf(game, sizeof(game), "%s - NO MAP", rgd3_session_state_name(rgd3_session_state()));
				lines[0] = fps;
				lines[1] = frame;
				lines[2] = mode;
				lines[3] = game;
				rgd3_overlay_right_lines(lines, 4);
			}
			rgd3_overlay_end(cmd, swapchain);
		}
	}
	/* submits, capturing the finished frame first if a screenshot is pending */
	rgd3_screenshot_submit(rgd3_app.gpu, cmd, swapchain, width, height, rgd3_app.swapchain_format);
}

/* ---------------------------------------------------------------------------
   Init / frame / shutdown
--------------------------------------------------------------------------- */

static void rgd3_app_init(int argc, char** argv)
{
	rgd3_thread_init_main();
	rgd3_time_init();
	if (!rgd3_mem_init(GB(2), MB(64)))
	{
		fprintf(stderr, "rgd3: could not reserve the memory pool\n");
		exit(1);
	}
	rgd3_console_init();
	rgd3_log_init();
	rgd3_sysevents_init();
	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_GAMEPAD))
		rgd3_error_fatal("SDL_Init failed: %s", SDL_GetError());
	rgd3_platform_init();

	rgd3_printf(RGD3_ENGINE_NAME " " RGD3_ENGINE_VERSION " (" RGD3_BUILD_MODE ") built " __DATE__ " " __TIME__ "\n");

	rgd3_cvar_init();
	rgd3_cmd_init();
	rgd3_keys_init();
	rgd3_usercmd_init(); /* also opens connected gamepads */
	rgd3_dict_init();
	rgd3_registry_register_all();

	rgd3_app_parse_command_line(argc, argv);
	rgd3_app_startup_variable(NULL);

	rgd3_fs_init();
	rgd3_savegame_init();

	/* default.cfg, then the user config unless "safe", then autoexec.cfg,
	   appended as Common.cpp does: each exec inserts its file's text at the
	   front of the buffer, so calling rgd3_cmd_exec_file three times in a row
	   would run them in reverse */
	rgd3_cmd_buffer_text(RGD3_CMD_EXEC_APPEND, "exec default.cfg\n");
	if (!rgd3_app_safe_mode())
		rgd3_cmd_buffer_text(RGD3_CMD_EXEC_APPEND, "exec " RGD3_CONFIG_FILE "\n");
	rgd3_cmd_buffer_text(RGD3_CMD_EXEC_APPEND, "exec autoexec.cfg\n");
	rgd3_cmd_execute_buffer();
	rgd3_app_startup_variable(NULL);

	/* decls: Init (types, materials), Init2 (skins, sound), the language
	   table, then the folders idGameLocal::Init registers */
	rgd3_decl_init();
	rgd3_decl_init2();
	rgd3_lang_init();
	rgd3_decl_init_game();

	rgd3_jobs_init();

	/* game, session, main menu */
	rgd3_common_init();

	if (rgd3_app.self_test)
		return;

	rgd3_app_init_video();
	rgd3_sysevents_set_resize_hook(NULL);

	/* The original keeps _ordered.resources only while startup decls load. */
	rgd3_fs_unmount_container("_ordered.resources");

	rgd3_console_load_history();
	rgd3_console_start_stdin();
	rgd3_app_add_startup_commands();
	rgd3_cvar_set_init_allowed(0);
	rgd3_cvar_clear_modified_flags(RGD3_CVAR_ARCHIVE);
	rgd3_keys_clear_bindings_modified();
	rgd3_app.fully_initialized = 1;
	rgd3_log_print_warnings();
	rgd3_log_clear_warnings("game frame");
	rgd3_printf("--- Common Initialization Complete --- %d ms\n", rgd3_time_ms());
}

static void rgd3_app_frame(void)
{
	/* events, game tics, map changes (common_frame.cpp Frame) */
	rgd3_common_frame();
	if (rgd3_app.quit)
		return;
	rgd3_app_check_video_cvars();
	rgd3_app_update_fps();
	rgd3_app_render_frame();
	++rgd3_app.frame_number;
}

static void rgd3_app_shutdown(void)
{
	rgd3_printf("--------- Shutting down ----------\n");
	rgd3_app_write_configuration(0);
	rgd3_app.fully_initialized = 0;
	rgd3_screenshot_shutdown();
	rgd3_console_shutdown();
	rgd3_app_shutdown_video();
	rgd3_common_shutdown();
	rgd3_savegame_shutdown();
	rgd3_jobs_shutdown();
	rgd3_decl_shutdown();
	rgd3_parser_shutdown();
	rgd3_lang_shutdown();
	rgd3_dict_shutdown();
	rgd3_fs_shutdown();
	rgd3_keys_shutdown();
	rgd3_cmd_shutdown();
	rgd3_cvar_shutdown();
	rgd3_usercmd_shutdown();
	rgd3_sysevents_shutdown();
	rgd3_log_shutdown();
	SDL_Quit();
}

static void rgd3_app_fatal_hook(void)
{
	if (rgd3_app.fully_initialized)
		rgd3_app_write_configuration(0);
}

int rgd3_app_run(int argc, char** argv)
{
	Rgd3ErrorGuard init_guard;
	rgd3_error_set_fatal_hook(rgd3_app_fatal_hook);
	rgd3_error_set_drop_hook(rgd3_app_stop_map);

	/* A drop during startup is fatal (Common.cpp catches it in Init). */
	rgd3_error_push(&init_guard);
	if (setjmp(init_guard.jump) == 0)
	{
		rgd3_app_init(argc, argv);
		rgd3_error_pop(&init_guard);
	}
	else
	{
		rgd3_error_fatal("error during initialization: %s", rgd3_error_message());
	}

	if (rgd3_app.self_test)
	{
		volatile int failures = 1;
		Rgd3ErrorGuard test_guard;
		rgd3_error_push(&test_guard);
		if (setjmp(test_guard.jump) == 0)
		{
			failures = rgd3_selftest_run();
			rgd3_error_pop(&test_guard);
		}
		else
		{
			rgd3_printf("self-test dropped: %s\n", rgd3_error_message());
		}
		rgd3_app_shutdown();
		{
			/* after shutdown every heap tag should be back to zero; a leak fails the run */
			int tag;
			for (tag = 0; tag < RGD3_TAG_COUNT; tag++)
			{
				if (rgd3_mem_tag_count((Rgd3MemTag)tag))
				{
					printf("leak: tag %s still holds %lld bytes in %lld blocks\n", rgd3_mem_tag_name((Rgd3MemTag)tag),
						(long long)rgd3_mem_tag_bytes((Rgd3MemTag)tag), (long long)rgd3_mem_tag_count((Rgd3MemTag)tag));
					failures++;
				}
			}
			if (rgd3_error_unwind_count())
			{
				printf("leak: %d objects still in the unwind registry\n", rgd3_error_unwind_count());
				failures++;
			}
			rgd3_error_shutdown();
		}
		if (failures)
			printf("self-test FAILED\n");
		return failures ? 1 : 0;
	}

	while (!rgd3_app.quit)
	{
		/* A drop during a frame returns here and the loop continues. */
		Rgd3ErrorGuard frame_guard;
		rgd3_error_push(&frame_guard);
		if (setjmp(frame_guard.jump) == 0)
			rgd3_app_frame();
		rgd3_error_pop(&frame_guard);
	}

	rgd3_app_shutdown();
	rgd3_error_shutdown();
	return 0;
}

void rgd3_app_register(void)
{
	static Rgd3Cvar* const cvars[] = {
		&cv_com_showFPS, &cv_r_windowWidth, &cv_r_windowHeight, &cv_r_fullscreen, &cv_r_swapInterval,
		&cv_r_gpuDriver, &cv_r_gpuDebug, &cv_r_shaderRoot, &cv_in_nograb
	};
	rgd3_cvar_register_table(cvars, (int)RG_ARRAY_COUNT(cvars));
	rgd3_cmd_add("quit", rgd3_app_cmd_quit, RGD3_CMD_FL_SYSTEM, "quits the game", NULL);
	rgd3_cmd_add("exit", rgd3_app_cmd_quit, RGD3_CMD_FL_SYSTEM, "exits the game", NULL);
	rgd3_cmd_add("writeConfig", rgd3_app_cmd_write_config, RGD3_CMD_FL_SYSTEM, "writes the config file", NULL);
	rgd3_cmd_add("version", rgd3_app_cmd_version, RGD3_CMD_FL_SYSTEM, "prints the engine version", NULL);
}

int main(int argc, char** argv)
{
	SDL_SetMainReady();
	return rgd3_app_run(argc, argv);
}
