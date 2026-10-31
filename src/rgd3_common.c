/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_common.c - see rgd3_common.h. Ported from neo/framework:
   common_frame.cpp (Frame: event pump, pause rule, the game tick
   accumulator with com_engineHz / timescale / com_fixedTic /
   com_deltaTimeClamp / com_noSleep / syncNextGameFrame, session pump,
   usercmd distribution, idGameThread::Run returning the previous run's
   gameReturn_t, ProcessGameReturn), Common_load.cpp (StartNewGame,
   MoveToNewMap, UnloadMap, ExecuteMapChange in its original order, SaveGame,
   LoadGame, the map/devmap/saveGame/loadGame commands), Common.cpp
   (com_engineHz numerator/denominator, Stop, LeaveGame, ProcessEvent,
   CreateMainMenu, disconnect) and Common_menu.cpp (StartMenu, MenuEvent).
   Left out: screen wipes and the loading GUI (renderer, M3+), demos, the
   Doom classic switch, lobby waits, net snapshots (co-op, M11). */


#define RGD3_GAME_NAME "rgd3" /* written at the start of every save */

static Rgd3Cvar cv_com_engineHz = RGD3_CVAR_RANGE("com_engineHz", "60", RGD3_CVAR_FLOAT | RGD3_CVAR_ARCHIVE, "Frames per second the engine runs at", 10.0f, 1024.0f);
static Rgd3Cvar cv_timescale = RGD3_CVAR_RANGE("timescale", "1", RGD3_CVAR_SYSTEM | RGD3_CVAR_FLOAT, "Number of game frames to run per render frame", 0.001f, 100.0f);
static Rgd3Cvar cv_com_fixedTic = RGD3_CVAR("com_fixedTic", "0", RGD3_CVAR_BOOL, "run a single game frame per render frame");
static Rgd3Cvar cv_com_noSleep = RGD3_CVAR("com_noSleep", "0", RGD3_CVAR_BOOL, "don't sleep if the game is running too fast");
static Rgd3Cvar cv_com_deltaTimeClamp = RGD3_CVAR("com_deltaTimeClamp", "50", RGD3_CVAR_INTEGER, "don't process more than this time in a single frame");
static Rgd3Cvar cv_com_speeds = RGD3_CVAR("com_speeds", "0", RGD3_CVAR_BOOL | RGD3_CVAR_SYSTEM | RGD3_CVAR_NOCHEAT, "show engine timings");
static Rgd3Cvar cv_com_disableAutoSaves = RGD3_CVAR("com_disableAutoSaves", "0", RGD3_CVAR_SYSTEM | RGD3_CVAR_BOOL, "");
static Rgd3Cvar cv_com_disableAllSaves = RGD3_CVAR("com_disableAllSaves", "0", RGD3_CVAR_SYSTEM | RGD3_CVAR_BOOL, "");

typedef struct Rgd3MapSpawnData
{
	Rgd3File* savegame_file; /* set while a loadGame is pending */
	Rgd3File* string_table_file;
	int savegame_version;
	Rgd3Dict persistent_player_info;
} Rgd3MapSpawnData;

static struct
{
	int initialized;
	int map_spawned;
	int inside_execute_map_change;
	int console_used;
	char current_map_name[RGD3_MAX_OSPATH];

	/* Frame() timing */
	int game_frame;
	double game_time_residual;
	int sync_next_game_frame;
	int last_frame_time;
	int have_last_frame_time;
	int paused;

	/* com_engineHz, latched at map load */
	i64 hz_numerator;
	i64 hz_denominator;
	float hz_latched;

	/* idGameThread: the return value of the previous run */
	Rgd3GameReturn game_return;

	Rgd3MapSpawnData spawn;
	int show_shell_requested;
} rgd3_common;

/* usercmds for every player, filled each frame for the tics it runs */
static Rgd3UsercmdMgr rgd3_common_user_cmd_mgr;
/* the empty commands of the initial frames after a map load */
static Rgd3UsercmdMgr rgd3_common_empty_cmd_mgr;

/* the save stream and its string table (idCommonLocal saveFile / stringsFile) */
static Rgd3File* rgd3_common_save_file;
static Rgd3File* rgd3_common_strings_file;

/* the buffers are only needed while a save is built or a load is read */
static void rgd3_common_release_save_files(void)
{
	rgd3_fs_memory_make_writable(rgd3_common_save_file);
	rgd3_fs_memory_clear(rgd3_common_save_file, 1);
	rgd3_fs_memory_make_writable(rgd3_common_strings_file);
	rgd3_fs_memory_clear(rgd3_common_strings_file, 1);
}

static void rgd3_common_save_files(Rgd3SaveFile files[2])
{
	files[0].name = RGD3_SAVEGAME_STRINGS_FILENAME;
	files[0].file = rgd3_common_strings_file;
	files[0].type = RGD3_SAVEGAMEFILE_BINARY | RGD3_SAVEGAMEFILE_COMPRESSED;
	files[0].error = 0;
	files[1].name = RGD3_SAVEGAME_GAMEDATA_FILENAME;
	files[1].file = rgd3_common_save_file;
	files[1].type = RGD3_SAVEGAMEFILE_BINARY | RGD3_SAVEGAMEFILE_COMPRESSED;
	files[1].error = 0;
}

/* ---- tick <-> msec (Common.h) ---- */

int rgd3_common_frame_to_msec(i64 frame)
{
	return (int)((frame * rgd3_common.hz_numerator) / rgd3_common.hz_denominator);
}

int rgd3_common_msec_to_frame_floor(int msec)
{
	return (int)((((i64)msec * rgd3_common.hz_denominator) + (rgd3_common.hz_denominator - 1)) / rgd3_common.hz_numerator);
}

int rgd3_common_msec_to_frame_ceil(int msec)
{
	return (int)((((i64)msec * rgd3_common.hz_denominator) + (rgd3_common.hz_numerator - 1)) / rgd3_common.hz_numerator);
}

static void rgd3_common_latch_engine_hz(void)
{
	/* "allow com_engineHz to be changed between map loads" */
	rgd3_common.hz_numerator = 100LL * 1000LL;
	rgd3_common.hz_denominator = (i64)(100.0f * cv_com_engineHz.float_value);
	rgd3_common.hz_latched = cv_com_engineHz.float_value;
}

/* ---- queries ---- */

int rgd3_common_map_spawned(void)
{
	return rgd3_common.map_spawned;
}

int rgd3_common_paused(void)
{
	return rgd3_common.paused;
}

const char* rgd3_common_current_map_name(void)
{
	return rgd3_common.current_map_name;
}

i64 rgd3_common_game_frame(void)
{
	return rgd3_common.game_frame;
}

/* ---- menus (Common_menu.cpp) ---- */

static int rgd3_common_menu_event(const Rgd3SysEvent* ev)
{
	if (rgd3_game_shell_is_active())
		return rgd3_game_shell_handle_gui_event(ev);
	return rgd3_game_handle_player_gui_event(ev);
}

static void rgd3_common_start_menu(void)
{
	if (rgd3_game_shell_is_active())
		return;
	rgd3_game_shell_show(1);
	rgd3_console_close();
}

/* the main menu is created inside an empty level load so its assets are
   purged when a real map loads */
static void rgd3_common_create_main_menu(void)
{
	rgd3_decl_begin_level_load();
	rgd3_render_begin_level_load();
	rgd3_sound_begin_level_load();
	rgd3_game_shell_create_menu(0);
	rgd3_game_shell_show(1);
	rgd3_render_end_level_load();
	rgd3_sound_end_level_load();
	rgd3_decl_end_level_load();
}

/* ---- map lifecycle ---- */

/* cleanup between maps or when a game is exited; leaves map_spawned 0 */
static void rgd3_common_unload_map(void)
{
	rgd3_game_map_shutdown();
	rgd3_common.map_spawned = 0;
}

/* Common.cpp Stop: idempotent, it is also the error drop hook */
void rgd3_common_stop(int reset_session)
{
	rgd3_common_unload_map();
	rgd3_sound_stop_all();
	rgd3_common.inside_execute_map_change = 0;
	rgd3_game_shell_show(0); /* ExitMenu */
	if (reset_session)
		rgd3_session_quit_match_to_title();
	/* rgd3: also release the map's container and level memory now (the
	   original keeps them until the next level load), so leaving a map or
	   dropping out of one returns memory use to the main-menu baseline */
	rgd3_fs_unload_map_resources();
	rgd3_mem_level_reset();
	/* a drop in the middle of a load leaves no half-loaded save behind */
	if (rgd3_common.spawn.savegame_file)
		rgd3_common_release_save_files();
	rgd3_common.spawn.savegame_file = NULL;
	rgd3_common.spawn.string_table_file = NULL;
}

/* the session left the game (disconnect): back to the main menu */
static void rgd3_common_leave_game(void)
{
	rgd3_common_stop(0);
	rgd3_common_create_main_menu();
	rgd3_common_start_menu();
}

static void rgd3_common_clean_map_name(const char* map_name, char* out, size_t size)
{
	char* dot;
	rgd3_str_copynz(out, map_name, size);
	rgd3_str_backslashes_to_slashes(out);
	dot = strrchr(out, '.');
	if (dot && !strchr(dot, '/'))
		*dot = 0;
}

void rgd3_common_start_new_game(const char* map_name, int devmap, int game_mode)
{
	Rgd3MatchParms parms = { 0 };

	if (!map_name || !map_name[0])
	{
		rgd3_printf("usage: map <name>, e.g. map game/mars_city1\n");
		return;
	}
	rgd3_common_clean_map_name(map_name, parms.map_name, sizeof(parms.map_name));
	parms.num_slots = 1;
	parms.game_mode = game_mode;
	parms.game_map = RGD3_GAME_MAP_SINGLEPLAYER;
	/* MoveCVarsToDict(CVAR_SERVERINFO): no serverinfo cvars exist before the
	   game (si_*), so only devmap goes in for now */
	if (devmap)
		rgd3_dict_set(&parms.server_info, "devmap", "1");
	rgd3_session_quit_match_to_title();
	rgd3_cvar_set_bool("developer", devmap, 0);
	rgd3_session_start_match(&parms);
	rgd3_dict_clear(&parms.server_info);
}

/* single player transition from one map to another */
void rgd3_common_move_to_new_map(const char* map_name, int devmap)
{
	Rgd3MatchParms parms = { 0 };

	rgd3_common_clean_map_name(map_name, parms.map_name, sizeof(parms.map_name));
	parms.num_slots = 1;
	parms.game_mode = RGD3_GAME_MODE_SINGLEPLAYER;
	parms.game_map = RGD3_GAME_MAP_SINGLEPLAYER;
	if (devmap)
	{
		rgd3_dict_set(&parms.server_info, "devmap", "1");
		rgd3_dict_clear(&rgd3_common.spawn.persistent_player_info);
	}
	else
	{
		rgd3_dict_assign(&rgd3_common.spawn.persistent_player_info, rgd3_game_get_persistent_player_info(0));
	}
	rgd3_session_quit_match_to_title();
	rgd3_session_start_match(&parms);
	rgd3_dict_clear(&parms.server_info);
}

/* runs `count` game frames with empty usercmds for every player */
static void rgd3_common_run_empty_frames(int count)
{
	Rgd3GameReturn ret;
	int i;
	for (i = 0; i < count; i++)
	{
		Rgd3Usercmd empty;
		int player;
		rgd3_usercmd_zero(&empty);
		for (player = 0; player < RGD3_MAX_PLAYERS; player++)
			rgd3_usercmd_mgr_put(&rgd3_common_empty_cmd_mgr, player, &empty);
		rgd3_game_run_frame(&rgd3_common_empty_cmd_mgr, &ret);
	}
}

/* ExecuteMapChange: loads the map of the session's match parameters, in
   the original order. Leaves map_spawned 1. */
static void rgd3_common_execute_map_change(void)
{
	const Rgd3MatchParms* parms;
	Rgd3PreloadManifest manifest;
	char full_map_name[RGD3_MAX_OSPATH];
	char manifest_name[RGD3_MAX_OSPATH];
	int cached_loading_id, start, sm, ms;

	if (rgd3_session_state() != RGD3_SESSION_LOADING)
	{
		rgd3_warning("Session state is not LOADING in ExecuteMapChange");
		return;
	}
	/* a new load request from inside this one changes the id */
	cached_loading_id = rgd3_session_loading_id();
	parms = rgd3_session_match_parms();
	if (parms->num_slots <= 0)
	{
		rgd3_warning("numSlots <= 0 in ExecuteMapChange");
		return;
	}
	rgd3_common.inside_execute_map_change = 1;

	rgd3_printf("--------- Execute Map Change ---------\n");
	rgd3_printf("Map: %s\n", parms->map_name);
	start = rgd3_time_ms();

	/* close the console, pause the game sounds */
	rgd3_console_close();
	rgd3_sound_set_paused(1);
	rgd3_sound_render();

	rgd3_common_clean_map_name(parms->map_name, rgd3_common.current_map_name, sizeof(rgd3_common.current_map_name));
	rg_snprintf(full_map_name, sizeof(full_map_name), "maps/%s.map", rgd3_common.current_map_name);

	/* fileSystem->BeginLevelLoad: the map's own .resources container */
	if (!rgd3_fs_load_map_resources(rgd3_common.current_map_name))
		rgd3_dprintf("no resource container for %s\n", rgd3_common.current_map_name);

	/* shut down the existing game if it is running */
	sm = rgd3_time_ms();
	rgd3_common_unload_map();
	ms = rgd3_time_ms() - sm;
	rgd3_printf("%6d msec to unload map\n", ms);

	/* free media from the previous level, note what this one needs */
	sm = rgd3_time_ms();
	rgd3_render_begin_level_load();
	rgd3_sound_begin_level_load();
	rgd3_decl_begin_level_load();
	/* everything from the last level is gone now: drop its arena */
	rgd3_mem_level_reset();
	ms = rgd3_time_ms() - sm;
	rgd3_printf("%6d msec to free assets\n", ms);

	/* preload manifest: "game/x" -> "maps/x.preload" */
	rgd3_preload_manifest_name(rgd3_common.current_map_name, manifest_name, sizeof(manifest_name));
	if (rgd3_preload_manifest_load(&manifest, manifest_name))
	{
		rgd3_render_preload(&manifest, rgd3_common.current_map_name);
		rgd3_sound_preload(&manifest);
		rgd3_game_preload(&manifest);
		rgd3_preload_manifest_free(&manifest);
	}

	rgd3_common_latch_engine_hz();

	/* note any warnings printed during the load */
	rgd3_log_clear_warnings(rgd3_common.current_map_name);

	/* release the mouse before this potentially long operation */
	rgd3_app_grab_mouse(0);

	/* let the renderer load the geometry */
	if (!rgd3_render_world_init_from_map(full_map_name))
		rgd3_error_drop("couldn't load %s", full_map_name);

	/* the angles rolled over between levels for the old synchronous net code */
	rgd3_usercmd_init_for_new_map();

	/* load and spawn all other entities, from a save possibly */
	if (rgd3_common.spawn.savegame_file)
	{
		if (!rgd3_game_init_from_save_game(full_map_name, rgd3_common.spawn.savegame_file, rgd3_common.spawn.string_table_file,
				rgd3_common.spawn.savegame_version))
		{
			/* a failed load ends the session: back to the main menu */
			rgd3_session_quit_match_to_title();
		}
	}
	else
	{
		if (!rgd3_session_is_multiplayer())
			rgd3_game_set_persistent_player_info(0, &rgd3_common.spawn.persistent_player_info);
		rgd3_game_set_server_info(&parms->server_info);
		rgd3_game_init_from_new_map(full_map_name, parms->game_mode, rgd3_time_ms());
	}

	rgd3_game_shell_create_menu(1);

	/* if the session is not loading any more, something went wrong */
	if (rgd3_session_state() == RGD3_SESSION_LOADING && rgd3_session_loading_id() == cached_loading_id)
		rgd3_session_loading_finished();

	/* run a frame to catch resources referenced by events posted in spawn */
	if (!rgd3_common.spawn.savegame_file)
		rgd3_common_run_empty_frames(1);

	rgd3_render_end_level_load();
	rgd3_sound_end_level_load();
	rgd3_decl_end_level_load();

	if (!rgd3_common.spawn.savegame_file && !rgd3_session_is_multiplayer())
	{
		/* single player: let ragdolls settle, then autosave so the map can
		   always be continued */
		rgd3_printf("----- Running initial game frames -----\n");
		rgd3_common_run_empty_frames(100);
		rgd3_printf("----- Saving Game -----\n");
		rgd3_common_save_game("autosave");
	}

	rgd3_printf("----- Generating Interactions -----\n");
	rgd3_render_world_generate_all_interactions();

	rgd3_log_print_warnings();
	rgd3_session_pump();
	rgd3_usercmd_clear();
	rgd3_console_clear_notify_lines();

	rgd3_common.inside_execute_map_change = 0;
	rgd3_common.map_spawned = 1;
	rgd3_sysevents_clear();

	rgd3_printf("%6d msec to load %s\n", rgd3_time_ms() - start, rgd3_common.current_map_name);
	rgd3_sound_render();
}

/* ---- saves ---- */

int rgd3_common_save_game(const char* save_name)
{
	if (cv_com_disableAllSaves.integer_value || (cv_com_disableAutoSaves.integer_value && !rgd3_str_icmp(save_name, "autosave")))
		return 0;
	/* only a running game, or the autosave at the end of a map load (the
	   original gets this from GetPersistentPlayerInfo having no player and
	   so no health; rgd3 checks it explicitly so a stale player dict can
	   never overwrite a slot with an empty save) */
	if (!rgd3_game_is_in_game() || (!rgd3_common.map_spawned && !rgd3_common.inside_execute_map_change))
	{
		rgd3_printf("No game to save.\n");
		return 0;
	}
	if (rgd3_session_is_multiplayer())
	{
		rgd3_printf("Can't save during net play.\n");
		return 0;
	}
	if (rgd3_common.spawn.savegame_file)
		return 0;
	if (rgd3_dict_get_int(rgd3_game_get_persistent_player_info(0), "health", 0) <= 0)
	{
		rgd3_printf("You must be alive to save the game\n");
		return 0;
	}
	{
		Rgd3SaveGameDetails details = { 0 };
		Rgd3SaveFile files[2];
		Rgd3SaveHandle handle;
		char slot[RGD3_SAVEGAME_MAX_FOLDER_NAME];

		rgd3_sound_set_paused(1);
		rgd3_sound_render();

		/* writable, from the start */
		rgd3_fs_memory_make_writable(rgd3_common_save_file);
		rgd3_fs_memory_clear(rgd3_common_save_file, 0);
		rgd3_fs_memory_make_writable(rgd3_common_strings_file);
		rgd3_fs_memory_clear(rgd3_common_strings_file, 0);

		/* header: game name, map, console used, server info */
		rgd3_fs_write_string(rgd3_common_save_file, RGD3_GAME_NAME);
		rgd3_fs_write_string(rgd3_common_save_file, rgd3_common.current_map_name);
		rgd3_fs_write_bool(rgd3_common_save_file, rgd3_common.console_used);
		rgd3_dict_write_to_file(rgd3_game_get_server_info(), rgd3_common_save_file);

		/* the game's own state */
		rgd3_game_save_game(rgd3_common_save_file, rgd3_common_strings_file);

		/* language, version and checksum are added by the save module */
		rgd3_game_get_save_game_details(&details.descriptors);
		rgd3_savegame_scrub_name(save_name, slot, sizeof(slot));
		rgd3_common_save_files(files);
		handle = rgd3_savegame_save(slot, files, 2, &details);
		rgd3_savegame_details_clear(&details);
		rgd3_common_release_save_files(); /* the save module has its own copy */
		if (!handle)
			return 0;

		/* don't let the game catch up on the time the save took */
		rgd3_common.sync_next_game_frame = 1;
		return 1;
	}
}

int rgd3_common_load_game(const char* save_name)
{
	if (rgd3_session_is_multiplayer())
	{
		rgd3_printf("Can't load during net play.\n");
		return 0;
	}
	if (rgd3_common.spawn.savegame_file)
		return 0;
	{
		Rgd3SaveGameDetails details = { 0 };
		Rgd3SaveFile files[2];
		Rgd3MatchParms parms = { 0 };
		Rgd3StrBuf game_name = { 0 };
		Rgd3StrBuf map_name = { 0 };
		char slot[RGD3_SAVEGAME_MAX_FOLDER_NAME];
		char errors[256];
		int err, ok = 0;

		rgd3_savegame_scrub_name(save_name, slot, sizeof(slot));
		rgd3_common_save_files(files);
		err = rgd3_savegame_load_sync(slot, files, 2, &details);
		if (err)
		{
			if (err & RGD3_SAVEGAME_E_FILE_NOT_FOUND)
				rgd3_printf("Could not find save '%s'\n", save_name);
			else
				rgd3_warning("loading '%s' failed: %s", save_name, rgd3_savegame_error_string(err, errors, sizeof(errors)));
			rgd3_savegame_details_clear(&details);
			rgd3_common_release_save_files();
			return 0;
		}
		/* saves hold localized text: the same language only */
		if (rgd3_str_icmp(rgd3_savegame_details_language(&details), rgd3_cvar_get_string("sys_lang")))
		{
			rgd3_printf("'%s' was saved in %s, sys_lang is %s\n", save_name, rgd3_savegame_details_language(&details),
				rgd3_cvar_get_string("sys_lang"));
			rgd3_savegame_details_clear(&details);
			rgd3_common_release_save_files();
			return 0;
		}

		/* OnLoadFilesCompleted: check the header, then start the match */
		rgd3_fs_read_string(rgd3_common_save_file, &game_name);
		rgd3_fs_read_string(rgd3_common_save_file, &map_name);
		if (strcmp(rgd3_strbuf_cstr(&game_name), RGD3_GAME_NAME) || !map_name.len ||
			rgd3_savegame_details_save_version(&details) > RGD3_SAVEGAME_VERSION)
		{
			rgd3_warning("Attempted to load an invalid savegame");
			rgd3_common_release_save_files();
		}
		else
		{
			rgd3_dprintf("loading savegame\n");
			rgd3_fs_read_bool(rgd3_common_save_file, &rgd3_common.console_used);
			rgd3_str_copynz(parms.map_name, rgd3_strbuf_cstr(&map_name), sizeof(parms.map_name));
			parms.num_slots = 1;
			parms.game_mode = RGD3_GAME_MODE_SINGLEPLAYER;
			parms.game_map = RGD3_GAME_MAP_SINGLEPLAYER;
			rgd3_dict_read_from_file(&parms.server_info, rgd3_common_save_file);

			/* the next frame loads the map from the save */
			rgd3_common.spawn.savegame_file = rgd3_common_save_file;
			rgd3_common.spawn.string_table_file = rgd3_common_strings_file;
			rgd3_common.spawn.savegame_version = rgd3_savegame_details_save_version(&details);
			rgd3_session_quit_match_to_title();
			rgd3_session_start_match(&parms);
			ok = 1;
		}
		rgd3_dict_clear(&parms.server_info);
		rgd3_strbuf_free(&game_name);
		rgd3_strbuf_free(&map_name);
		rgd3_savegame_details_clear(&details);
		return ok;
	}
}

/* ---- events (Common.cpp ProcessEvent) ---- */

int rgd3_common_process_event(const Rgd3SysEvent* ev)
{
	/* escape anywhere in a game brings up the menu */
	if (rgd3_game_is_in_game())
	{
		if (ev->type == RGD3_SE_KEY && ev->value2 == 1 && (ev->value == RGD3_K_ESCAPE || ev->value == RGD3_K_JOY9))
		{
			if (!rgd3_game_shell_is_active())
			{
				if (rgd3_common_menu_event(ev))
					return 1;
				rgd3_console_close();
				rgd3_common_start_menu();
				return 1;
			}
			rgd3_console_close();
			if (rgd3_common_menu_event(ev))
				return 1;
			rgd3_game_shell_close_pause();
		}
	}

	/* the pull-down console takes it if it wants */
	if (rgd3_console_process_event(ev, 0))
		return 1;

	if (rgd3_common_menu_event(ev))
		return 1;

	/* outside a game the console gets everything */
	if (!rgd3_common.map_spawned)
	{
		rgd3_console_process_event(ev, 1);
		return 1;
	}

	/* in game, key downs run their bindings */
	if (ev->type == RGD3_SE_KEY && ev->value2 == 1)
	{
		rgd3_keys_exec_key_binding(ev->value);
		return 1;
	}
	return 0;
}

/* ---- frame (common_frame.cpp) ---- */

static void rgd3_common_process_game_return(const Rgd3GameReturn* ret)
{
	Rgd3CmdArgs args;

	/* rumble on the active pad only, never while a menu is up */
	rgd3_gamepad_frame_rumble(rgd3_gamepad_master_device(), ret->vibration_low, ret->vibration_high, rgd3_game_shell_is_active());
	rgd3_common.sync_next_game_frame = ret->sync_next_game_frame;
	if (!ret->session_command[0])
		return;
	rgd3_cmdargs_tokenize(&args, ret->session_command, 0);
	if (!rgd3_str_icmp(rgd3_cmdargs_argv(&args, 0), "map"))
		rgd3_common_move_to_new_map(rgd3_cmdargs_argv(&args, 1), 0);
	else if (!rgd3_str_icmp(rgd3_cmdargs_argv(&args, 0), "devmap"))
		rgd3_common_move_to_new_map(rgd3_cmdargs_argv(&args, 1), 1);
	else if (!rgd3_str_icmp(rgd3_cmdargs_argv(&args, 0), "died"))
	{
		if (!rgd3_session_is_multiplayer())
			rgd3_game_shell_show(1);
	}
	else if (!rgd3_str_icmp(rgd3_cmdargs_argv(&args, 0), "disconnect"))
		rgd3_cmd_buffer_text(RGD3_CMD_EXEC_INSERT, "stoprecording ; disconnect");
}

/* How many game ticks to run this frame. Sleeps until at least one is
   due unless paused, com_fixedTic, a forced sync or com_noSleep. */
static int rgd3_common_num_game_frames(int pause_game)
{
	int num_game_frames = 0;

	for (;;)
	{
		const int this_frame_time = rgd3_time_ms();
		int delta, clamped;

		if (!rgd3_common.have_last_frame_time)
		{
			rgd3_common.last_frame_time = this_frame_time;
			rgd3_common.have_last_frame_time = 1;
		}
		delta = this_frame_time - rgd3_common.last_frame_time;
		rgd3_common.last_frame_time = this_frame_time;

		/* after a long gap or at very low frame rates, limit the frames run */
		clamped = delta < cv_com_deltaTimeClamp.integer_value ? delta : cv_com_deltaTimeClamp.integer_value;
		rgd3_common.game_time_residual += clamped * cv_timescale.float_value;

		if (pause_game)
		{
			rgd3_common.game_frame++;
			rgd3_common.game_time_residual = 0;
			break;
		}
		/* debug: force game tics */
		if (cv_com_fixedTic.integer_value > 0)
		{
			num_game_frames = cv_com_fixedTic.integer_value;
			rgd3_common.game_frame += num_game_frames;
			rgd3_common.game_time_residual = 0;
			break;
		}
		if (rgd3_common.sync_next_game_frame)
		{
			rgd3_common.sync_next_game_frame = 0;
			rgd3_common.game_frame++;
			num_game_frames++;
			rgd3_common.game_time_residual = 0;
			break;
		}
		for (;;)
		{
			const int frame_delay = rgd3_common_frame_to_msec(rgd3_common.game_frame + 1) - rgd3_common_frame_to_msec(rgd3_common.game_frame);
			if (rgd3_common.game_time_residual < frame_delay)
				break;
			rgd3_common.game_time_residual -= frame_delay;
			rgd3_common.game_frame++;
			num_game_frames++;
		}
		if (num_game_frames > 0)
			break;
		/* with vsync, always run a frame and never sleep */
		if (cv_com_noSleep.integer_value)
		{
			num_game_frames = 1;
			rgd3_common.game_frame += num_game_frames;
			rgd3_common.game_time_residual = 0;
			break;
		}
		/* not enough time has passed: sleep a bit and look again */
		rgd3_time_sleep_ms(0);
	}
	return num_game_frames;
}

void rgd3_common_frame(void)
{
	int num_game_frames, pause_game, i;
	Rgd3GameReturn latched;
	u64 frame_start = rgd3_time_us();
	u64 game_start, game_end;

	rgd3_mem_temp_reset();
	rgd3_mem_frame_swap();
	rgd3_cmd_begin_frame();

	/* pump all the events */
	rgd3_sysevents_generate();
	if (rgd3_sysevents_quit_requested())
		rgd3_app_quit();

	/* write the config if anything changed */
	rgd3_app_write_configuration(0);

	rgd3_sysevents_run_loop(1);

	if (rgd3_common.show_shell_requested)
	{
		rgd3_game_shell_show(1);
		rgd3_common.show_shell_requested = 0;
	}

	/* with the console or a menu up, the mouse is free and input is held */
	if (rgd3_console_active() || rgd3_game_inhibit_controls() || rgd3_game_shell_is_active() || !rgd3_common.map_spawned)
	{
		rgd3_app_grab_mouse(0);
		rgd3_usercmd_inhibit(RGD3_INHIBIT_SESSION, 1);
	}
	else
	{
		rgd3_app_grab_mouse(1);
		rgd3_usercmd_inhibit(RGD3_INHIBIT_SESSION, 0);
	}

	pause_game = !rgd3_common.map_spawned || (!rgd3_session_is_multiplayer() && rgd3_game_shell_is_active());
	rgd3_common.paused = pause_game;

	num_game_frames = rgd3_common_num_game_frames(pause_game);

	/* sync with the session after sleeping */
	rgd3_session_pump();
	if (rgd3_session_state() == RGD3_SESSION_LOADING)
	{
		/* the session wants a map: load it */
		rgd3_common_execute_map_change();
		if (rgd3_common.spawn.savegame_file)
			rgd3_common_release_save_files();
		rgd3_common.spawn.savegame_file = NULL;
		rgd3_common.spawn.string_table_file = NULL;
		rgd3_dict_clear(&rgd3_common.spawn.persistent_player_info);
		return;
	}
	else if (rgd3_session_state() != RGD3_SESSION_INGAME && rgd3_common.map_spawned)
	{
		/* the game is running but the session left it: disconnect */
		rgd3_common_leave_game();
		return;
	}

	/* build this frame's usercmd and queue a copy for each tic that runs
	   (ideally they'd be built at 60 Hz in another thread, says the original) */
	{
		const int client = rgd3_game_get_local_client_num();
		Rgd3Usercmd cmd;
		rgd3_usercmd_build_current(rgd3_gamepad_master_device());
		if (pause_game)
			rgd3_usercmd_clear();
		cmd = rgd3_usercmd_get_current();
		cmd.server_game_milliseconds = rgd3_game_get_server_game_time_ms();
		rgd3_usercmd_mgr_make_read_ptr_current(&rgd3_common_user_cmd_mgr, client);
		for (i = 0; i < num_game_frames; i++)
		{
			cmd.client_game_milliseconds = rgd3_common_frame_to_msec(rgd3_common.game_frame - num_game_frames + i + 1);
			rgd3_usercmd_mgr_put(&rgd3_common_user_cmd_mgr, client, &cmd);
		}
	}

	/* idGameThread::RunGameAndDraw: run the tics; hand back the previous
	   run's return value, as the threaded original does */
	latched = rgd3_common.game_return;
	game_start = rgd3_time_us();
	if (num_game_frames == 0)
		memset(&rgd3_common.game_return, 0, sizeof(rgd3_common.game_return));
	for (i = 0; i < num_game_frames; i++)
	{
		rgd3_game_run_frame(&rgd3_common_user_cmd_mgr, &rgd3_common.game_return);
		if (rgd3_common.game_return.sync_next_game_frame || rgd3_common.game_return.session_command[0])
			break;
	}
	game_end = rgd3_time_us();
	rgd3_game_draw(rgd3_game_get_local_client_num());

	/* the sound system plays the menu world while paused */
	rgd3_sound_set_paused(pause_game);
	rgd3_sound_render();

	/* map changes etc. */
	rgd3_common_process_game_return(&latched);

	if (cv_com_speeds.integer_value)
	{
		rgd3_printf("frame:%lld all:%3d gfr:%3d tics:%d\n", (long long)rgd3_common.game_frame,
			(int)((rgd3_time_us() - frame_start) / 1000), (int)((game_end - game_start) / 1000), num_game_frames);
	}
	rgd3_savegame_pump();
}

/* ---- commands ---- */

static void rgd3_common_cmd_map(const Rgd3CmdArgs* args)
{
	rgd3_common_start_new_game(rgd3_cmdargs_argv(args, 1), 0, RGD3_GAME_MODE_SINGLEPLAYER);
}

static void rgd3_common_cmd_devmap(const Rgd3CmdArgs* args)
{
	rgd3_common_start_new_game(rgd3_cmdargs_argv(args, 1), 1, RGD3_GAME_MODE_SINGLEPLAYER);
}

static void rgd3_common_cmd_disconnect(const Rgd3CmdArgs* args)
{
	(void)args;
	rgd3_session_quit_match_to_title();
}

static void rgd3_common_cmd_save_game(const Rgd3CmdArgs* args)
{
	const char* name = rgd3_cmdargs_argc(args) > 1 ? rgd3_cmdargs_argv(args, 1) : "quick";
	if (rgd3_common_save_game(name))
		rgd3_printf("Saved: %s\n", name);
}

static void rgd3_common_cmd_load_game(const Rgd3CmdArgs* args)
{
	rgd3_console_close();
	rgd3_common_load_game(rgd3_cmdargs_argc(args) > 1 ? rgd3_cmdargs_argv(args, 1) : "quick");
}

/* rgd3 addition: where the frame loop stands, for scripts and tests */
static void rgd3_common_cmd_session_info(const Rgd3CmdArgs* args)
{
	(void)args;
	rgd3_printf("session %s, map %s, spawned %d, paused %d, game frame %d, game time %d ms, engine %.0f Hz\n",
		rgd3_session_state_name(rgd3_session_state()), rgd3_common.current_map_name[0] ? rgd3_common.current_map_name : "-",
		rgd3_common.map_spawned, rgd3_common.paused, rgd3_common.game_frame, rgd3_game_get_server_game_time_ms(),
		rgd3_common.hz_latched);
}

void rgd3_common_register(void)
{
	static Rgd3Cvar* const cvars[] = {
		&cv_com_engineHz, &cv_timescale, &cv_com_fixedTic, &cv_com_noSleep, &cv_com_deltaTimeClamp, &cv_com_speeds,
		&cv_com_disableAutoSaves, &cv_com_disableAllSaves,
	};
	rgd3_cvar_register_table(cvars, (int)RG_ARRAY_COUNT(cvars));
	rgd3_cmd_add("map", rgd3_common_cmd_map, RGD3_CMD_FL_SYSTEM, "loads a map", NULL);
	rgd3_cmd_add("devmap", rgd3_common_cmd_devmap, RGD3_CMD_FL_SYSTEM, "loads a map in developer mode", NULL);
	rgd3_cmd_add("disconnect", rgd3_common_cmd_disconnect, RGD3_CMD_FL_SYSTEM, "disconnects from a game", NULL);
	rgd3_cmd_add("saveGame", rgd3_common_cmd_save_game, RGD3_CMD_FL_SYSTEM, "saves a game", NULL);
	rgd3_cmd_add("loadGame", rgd3_common_cmd_load_game, RGD3_CMD_FL_SYSTEM, "loads a game", NULL);
	rgd3_cmd_add("sessionInfo", rgd3_common_cmd_session_info, RGD3_CMD_FL_SYSTEM, "prints the session and game frame state", NULL);
}

/* ---- lifetime ---- */

void rgd3_common_init(void)
{
	rgd3_common_latch_engine_hz();
	rgd3_common_save_file = rgd3_fs_open_memory_write("gamedata.save");
	rgd3_common_strings_file = rgd3_fs_open_memory_write("gamedata.strings");
	rgd3_game_init();
	rgd3_common_create_main_menu();
	rgd3_common_start_menu();
	rgd3_common.initialized = 1;
}

void rgd3_common_shutdown(void)
{
	if (!rgd3_common.initialized)
		return;
	rgd3_common_stop(1);
	rgd3_game_shutdown();
	rgd3_session_shutdown();
	rgd3_dict_clear(&rgd3_common.spawn.persistent_player_info);
	rgd3_fs_close(rgd3_common_save_file);
	rgd3_fs_close(rgd3_common_strings_file);
	rgd3_common_save_file = NULL;
	rgd3_common_strings_file = NULL;
	rgd3_common.initialized = 0;
}
