/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_session.h - the match state machine (idSession, neo/sys/sys_session.h;
   the local paths of sys_session_local.cpp). The original walks
   IDLE -> PARTY_LOBBY -> GAME_LOBBY -> LOADING -> INGAME through lobbies;
   rgd3 has no lobbies, so starting a match goes straight to LOADING, and
   the frame loop (rgd3_common_frame) loads the map when it sees LOADING.
   Co-op (M11) adds host/client roles here. */
#ifndef RGD3_SESSION_H
#define RGD3_SESSION_H
#ifndef RGD3_ALL_C
#error "rgd3 headers are included through src/all.c only"
#endif

typedef enum Rgd3SessionState
{
	RGD3_SESSION_IDLE,
	RGD3_SESSION_LOADING,
	RGD3_SESSION_INGAME
} Rgd3SessionState;

/* idMatchParameters */
typedef struct Rgd3MatchParms
{
	char map_name[RGD3_MAX_OSPATH]; /* "game/mars_city1", no extension */
	int game_mode;
	int game_map;
	int num_slots;
	Rgd3Dict server_info;
} Rgd3MatchParms;

void rgd3_session_register(void);
void rgd3_session_shutdown(void);
Rgd3SessionState rgd3_session_state(void);
const char* rgd3_session_state_name(Rgd3SessionState state);
const Rgd3MatchParms* rgd3_session_match_parms(void);
int rgd3_session_loading_id(void);
void rgd3_session_start_match(const Rgd3MatchParms* parms); /* copies parms; -> LOADING */
void rgd3_session_loading_finished(void);                  /* LOADING -> INGAME */
void rgd3_session_quit_match_to_title(void);               /* -> IDLE */
void rgd3_session_pump(void);
int rgd3_session_is_multiplayer(void);

#endif
