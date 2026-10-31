/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_hooks.h - the entry points the framework calls in the game, the
   renderer and the sound system (idGame, neo/d3xp/Game.h; idRenderSystem /
   idRenderWorld, neo/renderer/RenderSystem.h, RenderWorld.h; idSoundSystem /
   idSoundWorld, neo/sound/sound.h), as plain functions.

   Until a subsystem exists its functions come from a stub file
   (src/rgd3_game_stub.c, rgd3_render_stub.c, rgd3_sound_stub.c); the real
   module replaces the stub file in all.c. Each stub records its calls with
   rgd3_hooks_trace so the self-test can check the map-change order.

   Co-op note: nothing here assumes one player. Player arguments are client
   numbers 0..RGD3_MAX_PLAYERS-1. */
#ifndef RGD3_HOOKS_H
#define RGD3_HOOKS_H
#ifndef RGD3_ALL_C
#error "rgd3 headers are included through src/all.c only"
#endif

/* RGD3_MAX_PLAYERS (8, player slots 0..7) comes from rgd3_usercmd.h */

/* sys_session.h; co-op gets its own mode in M11 */
#define RGD3_GAME_MODE_SINGLEPLAYER (-2)
#define RGD3_GAME_MAP_SINGLEPLAYER (-2)

struct Rgd3UsercmdMgr;

/* gameReturn_t (Game.h) */
typedef struct Rgd3GameReturn
{
	char session_command[RGD3_MAX_STRING_CHARS]; /* "map", "devmap", "died", "disconnect", ... */
	int sync_next_game_frame;
	int vibration_low;
	int vibration_high;
} Rgd3GameReturn;

/* ---- game (idGame) ---- */
void rgd3_game_init(void);
void rgd3_game_shutdown(void);
void rgd3_game_set_server_info(const Rgd3Dict* server_info);
const Rgd3Dict* rgd3_game_get_server_info(void);
int rgd3_game_get_server_game_time_ms(void);
const Rgd3Dict* rgd3_game_get_persistent_player_info(int client_num);
void rgd3_game_set_persistent_player_info(int client_num, const Rgd3Dict* info);
/* map_name is the full name ("maps/game/mars_city1.map"). Errors drop. */
void rgd3_game_init_from_new_map(const char* map_name, int game_mode, int random_seed);
int rgd3_game_init_from_save_game(const char* map_name, Rgd3File* save_file, Rgd3File* string_file, int save_version);
void rgd3_game_save_game(Rgd3File* save_file, Rgd3File* string_file);
void rgd3_game_get_save_game_details(Rgd3Dict* descriptors);
void rgd3_game_map_shutdown(void); /* idempotent: a drop can arrive mid-way */
void rgd3_game_preload(const Rgd3PreloadManifest* manifest);
void rgd3_game_run_frame(struct Rgd3UsercmdMgr* cmds, Rgd3GameReturn* ret);
int rgd3_game_draw(int client_num);
int rgd3_game_is_in_game(void);
int rgd3_game_get_local_client_num(void); /* presentation only, see PLAN.md Co-op */
int rgd3_game_inhibit_controls(void);
int rgd3_game_handle_player_gui_event(const Rgd3SysEvent* ev);
/* the shell (main menu / pause menu) */
void rgd3_game_shell_create_menu(int in_game);
void rgd3_game_shell_show(int show);
void rgd3_game_shell_close_pause(void);
int rgd3_game_shell_is_active(void);
int rgd3_game_shell_handle_gui_event(const Rgd3SysEvent* ev);

/* ---- renderer (idRenderSystem / idRenderWorld) ---- */
void rgd3_render_begin_level_load(void);
void rgd3_render_end_level_load(void);
void rgd3_render_preload(const Rgd3PreloadManifest* manifest, const char* map_name);
/* full map name; loads generated/<map>.bproc. Returns 0 if missing. */
int rgd3_render_world_init_from_map(const char* map_name);
void rgd3_render_world_generate_all_interactions(void);

/* ---- sound (idSoundSystem / idSoundWorld) ---- */
void rgd3_sound_begin_level_load(void);
void rgd3_sound_end_level_load(void);
void rgd3_sound_preload(const Rgd3PreloadManifest* manifest);
void rgd3_sound_set_paused(int paused); /* game world paused, menu world playing */
void rgd3_sound_stop_all(void);
void rgd3_sound_render(void);

/* ---- call tracing ---- */
void rgd3_hooks_register(void);
void rgd3_hooks_trace(const char* name);
void rgd3_hooks_trace_reset(void);
const char* rgd3_hooks_trace_get(void); /* names joined with ' ' since the last reset */

#endif
