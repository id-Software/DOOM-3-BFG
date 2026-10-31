/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_common.h - the engine frame and the map lifecycle (idCommonLocal:
   neo/framework/common_frame.cpp Frame / ProcessGameReturn,
   Common_load.cpp StartNewGame / MoveToNewMap / ExecuteMapChange /
   UnloadMap / SaveGame / LoadGame, Common.cpp Stop / LeaveGame /
   ProcessEvent, Common_menu.cpp StartMenu / MenuEvent).

   Game time runs in fixed ticks of 1000 / com_engineHz ms (60 Hz), from
   the accumulator in Frame(); rendering is free-running. */
#ifndef RGD3_COMMON_H
#define RGD3_COMMON_H
#ifndef RGD3_ALL_C
#error "rgd3 headers are included through src/all.c only"
#endif

void rgd3_common_register(void);
void rgd3_common_init(void);
void rgd3_common_shutdown(void);
void rgd3_common_frame(void);
int rgd3_common_process_event(const Rgd3SysEvent* ev);
void rgd3_common_stop(int reset_session); /* idempotent; the error drop hook */

/* game tick <-> milliseconds at the latched com_engineHz (Common.h) */
int rgd3_common_frame_to_msec(i64 frame);
int rgd3_common_msec_to_frame_floor(int msec);
int rgd3_common_msec_to_frame_ceil(int msec);

void rgd3_common_start_new_game(const char* map_name, int devmap, int game_mode);
void rgd3_common_move_to_new_map(const char* map_name, int devmap);
int rgd3_common_save_game(const char* save_name);
int rgd3_common_load_game(const char* save_name);

int rgd3_common_map_spawned(void);
int rgd3_common_paused(void);
const char* rgd3_common_current_map_name(void);
i64 rgd3_common_game_frame(void);

#endif
