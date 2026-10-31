/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_game_stub.c - the game entry points until the game exists (G0 in
   M4). It does just enough real work to exercise the framework: loads and
   counts the map's entities, keeps a game clock from the frame count,
   keeps server and persistent player info, writes and verifies a small save
   payload, and has a minimal shell (paused / not paused). Replaced in
   all.c by the real game. */

#define RGD3_GAME_STUB_SAVE_MAGIC 0x52474453u /* "RGDS" */

static struct
{
	Rgd3Dict server_info;
	Rgd3Dict persistent[RGD3_MAX_PLAYERS];
	Rgd3MapFile map;
	char map_name[RGD3_MAX_OSPATH];
	int in_game;
	int game_frame;
	int game_time_ms;
	int shell_active;
	int initialized;
	Rgd3Usercmd last_cmd[RGD3_MAX_PLAYERS];
} rgd3_game_stub;

/* ---- little-endian helpers matching idFile::WriteInt / WriteString ---- */

static void rgd3_game_stub_write_int(Rgd3File* f, int value)
{
	u8 b[4];
	rg_bin_store_u32_le(b, (u32)value);
	rgd3_fs_write(f, b, 4);
}

static int rgd3_game_stub_read_int(Rgd3File* f, int* value)
{
	u8 b[4];
	if (rgd3_fs_read(f, b, 4) != 4)
		return 0;
	*value = (int)rg_bin_load_u32_le(b);
	return 1;
}

static void rgd3_game_stub_write_string(Rgd3File* f, const char* s)
{
	int len = (int)strlen(s);
	rgd3_game_stub_write_int(f, len);
	rgd3_fs_write(f, s, (size_t)len);
}

static int rgd3_game_stub_read_string(Rgd3File* f, char* out, int size)
{
	int len;
	if (!rgd3_game_stub_read_int(f, &len) || len < 0 || len >= size)
		return 0;
	if (rgd3_fs_read(f, out, (size_t)len) != (size_t)len)
		return 0;
	out[len] = 0;
	return 1;
}

/* ---- lifetime ---- */

void rgd3_game_init(void)
{
	int i;
	rgd3_hooks_trace("game_init");
	for (i = 0; i < RGD3_MAX_PLAYERS; i++)
		rgd3_dict_set_int(&rgd3_game_stub.persistent[i], "health", 100);
	rgd3_game_stub.initialized = 1;
}

void rgd3_game_map_shutdown(void)
{
	rgd3_hooks_trace("game_map_shutdown");
	rgd3_mapfile_free(&rgd3_game_stub.map);
	rgd3_game_stub.in_game = 0;
	rgd3_game_stub.shell_active = 0;
	rgd3_game_stub.map_name[0] = 0;
}

void rgd3_game_shutdown(void)
{
	int i;
	if (!rgd3_game_stub.initialized)
		return;
	rgd3_hooks_trace("game_shutdown");
	rgd3_game_map_shutdown();
	rgd3_dict_clear(&rgd3_game_stub.server_info);
	for (i = 0; i < RGD3_MAX_PLAYERS; i++)
		rgd3_dict_clear(&rgd3_game_stub.persistent[i]);
	rgd3_game_stub.initialized = 0;
}

void rgd3_game_set_server_info(const Rgd3Dict* server_info)
{
	rgd3_hooks_trace("game_set_server_info");
	rgd3_dict_assign(&rgd3_game_stub.server_info, server_info);
}

const Rgd3Dict* rgd3_game_get_server_info(void)
{
	return &rgd3_game_stub.server_info;
}

int rgd3_game_get_server_game_time_ms(void)
{
	return rgd3_game_stub.game_time_ms;
}

const Rgd3Dict* rgd3_game_get_persistent_player_info(int client_num)
{
	RG_ASSERT(client_num >= 0 && client_num < RGD3_MAX_PLAYERS);
	return &rgd3_game_stub.persistent[client_num];
}

void rgd3_game_set_persistent_player_info(int client_num, const Rgd3Dict* info)
{
	rgd3_hooks_trace("game_set_persistent_player_info");
	RG_ASSERT(client_num >= 0 && client_num < RGD3_MAX_PLAYERS);
	if (rgd3_dict_num(info))
		rgd3_dict_assign(&rgd3_game_stub.persistent[client_num], info);
}

/* loads the .map the way idGameLocal::InitFromNewMap starts: a missing or
   broken map file drops */
static void rgd3_game_stub_load_map(const char* map_name)
{
	rgd3_mapfile_free(&rgd3_game_stub.map);
	if (!rgd3_mapfile_parse(&rgd3_game_stub.map, map_name, 1, 0))
		rgd3_error_drop("Couldn't load %s", map_name);
	rgd3_str_copynz(rgd3_game_stub.map_name, map_name, sizeof(rgd3_game_stub.map_name));
	rgd3_game_stub.game_frame = 0;
	rgd3_game_stub.game_time_ms = 0;
	rgd3_game_stub.shell_active = 0;
	rgd3_game_stub.in_game = 1;
	rgd3_printf("game stub: %s has %d entities\n", map_name, rgd3_game_stub.map.num_entities);
}

void rgd3_game_init_from_new_map(const char* map_name, int game_mode, int random_seed)
{
	(void)game_mode;
	(void)random_seed;
	rgd3_hooks_trace("game_init_from_new_map");
	rgd3_game_stub_load_map(map_name);
}

int rgd3_game_init_from_save_game(const char* map_name, Rgd3File* save_file, Rgd3File* string_file, int save_version)
{
	int magic = 0, frame = 0, time_ms = 0, entities = 0;
	char saved_map[RGD3_MAX_OSPATH];
	(void)string_file;
	(void)save_version;

	rgd3_hooks_trace("game_init_from_save_game");
	rgd3_game_stub_load_map(map_name);
	if (!rgd3_game_stub_read_int(save_file, &magic) || (u32)magic != RGD3_GAME_STUB_SAVE_MAGIC ||
		!rgd3_game_stub_read_string(save_file, saved_map, sizeof(saved_map)) || !rgd3_game_stub_read_int(save_file, &frame) ||
		!rgd3_game_stub_read_int(save_file, &time_ms) || !rgd3_game_stub_read_int(save_file, &entities))
	{
		rgd3_warning("game stub: bad save data");
		return 0;
	}
	if (rgd3_str_icmp(saved_map, map_name) || entities != rgd3_game_stub.map.num_entities)
	{
		rgd3_warning("game stub: save is for %s with %d entities, loaded %s with %d", saved_map, entities, map_name,
			rgd3_game_stub.map.num_entities);
		return 0;
	}
	rgd3_dict_read_from_file(&rgd3_game_stub.persistent[0], save_file);
	rgd3_game_stub.game_frame = frame;
	rgd3_game_stub.game_time_ms = time_ms;
	return 1;
}

void rgd3_game_save_game(Rgd3File* save_file, Rgd3File* string_file)
{
	(void)string_file;
	rgd3_hooks_trace("game_save_game");
	rgd3_game_stub_write_int(save_file, (int)RGD3_GAME_STUB_SAVE_MAGIC);
	rgd3_game_stub_write_string(save_file, rgd3_game_stub.map_name);
	rgd3_game_stub_write_int(save_file, rgd3_game_stub.game_frame);
	rgd3_game_stub_write_int(save_file, rgd3_game_stub.game_time_ms);
	rgd3_game_stub_write_int(save_file, rgd3_game_stub.map.num_entities);
	rgd3_dict_write_to_file(&rgd3_game_stub.persistent[0], save_file);
}

/* the original's SAVEGAME_DETAIL_FIELD_* keys (sys_savegame.h) */
void rgd3_game_get_save_game_details(Rgd3Dict* descriptors)
{
	char short_name[RGD3_MAX_OSPATH];
	char* dot;
	const char* name = rgd3_game_stub.map_name;
	if (!rgd3_str_icmpn(name, "maps/", 5))
		name += 5;
	rgd3_str_copynz(short_name, name, sizeof(short_name));
	dot = strrchr(short_name, '.');
	if (dot)
		*dot = 0;
	rgd3_dict_clear(descriptors);
	rgd3_dict_set_int(descriptors, "expansion", 0);
	rgd3_dict_set(descriptors, "mapName", short_name);
	rgd3_dict_set(descriptors, "mapLocation", rgd3_lang_get_string("#str_02911"));
	rgd3_dict_set_int(descriptors, "difficulty", 1);
	rgd3_dict_set_int(descriptors, "playTime", rgd3_game_stub.game_time_ms / 1000);
}

void rgd3_game_preload(const Rgd3PreloadManifest* manifest)
{
	(void)manifest;
	rgd3_hooks_trace("game_preload");
}

void rgd3_game_run_frame(struct Rgd3UsercmdMgr* cmds, Rgd3GameReturn* ret)
{
	int player;
	memset(ret, 0, sizeof(*ret));
	/* every tic consumes one queued usercmd per player, as the game does */
	for (player = 0; player < RGD3_MAX_PLAYERS; player++)
	{
		if (rgd3_usercmd_mgr_has(cmds, player, 0))
			rgd3_game_stub.last_cmd[player] = *rgd3_usercmd_mgr_get(cmds, player);
	}
	if (!rgd3_game_stub.in_game)
		return;
	rgd3_game_stub.game_frame++;
	rgd3_game_stub.game_time_ms = rgd3_common_frame_to_msec(rgd3_game_stub.game_frame);
}

int rgd3_game_draw(int client_num)
{
	(void)client_num;
	return rgd3_game_stub.in_game;
}

int rgd3_game_is_in_game(void)
{
	return rgd3_game_stub.in_game;
}

int rgd3_game_get_local_client_num(void)
{
	return 0;
}

int rgd3_game_inhibit_controls(void)
{
	return 0;
}

int rgd3_game_handle_player_gui_event(const Rgd3SysEvent* ev)
{
	(void)ev;
	return 0;
}

/* ---- shell ---- */

void rgd3_game_shell_create_menu(int in_game)
{
	(void)in_game;
	rgd3_hooks_trace("game_shell_create_menu");
}

void rgd3_game_shell_show(int show)
{
	rgd3_game_stub.shell_active = show;
}

void rgd3_game_shell_close_pause(void)
{
	rgd3_game_stub.shell_active = 0;
}

int rgd3_game_shell_is_active(void)
{
	return rgd3_game_stub.shell_active;
}

/* no menu yet: the stub shell eats key presses and Escape closes it */
int rgd3_game_shell_handle_gui_event(const Rgd3SysEvent* ev)
{
	if (!rgd3_game_stub.shell_active)
		return 0;
	if (ev->type == RGD3_SE_KEY && ev->value2 && ev->value == RGD3_K_ESCAPE && rgd3_game_stub.in_game)
		rgd3_game_stub.shell_active = 0;
	return ev->type == RGD3_SE_KEY || ev->type == RGD3_SE_CHAR;
}
