/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_session.c - see rgd3_session.h. State names and transitions from
   neo/sys/sys_session.h (sessionState_t) and sys_session_local.cpp
   (StartMatch, LoadingFinished, QuitMatchToTitle). */

static struct
{
	Rgd3SessionState state;
	Rgd3MatchParms parms;
	int loading_id;
} rgd3_session;

static Rgd3Cvar cv_net_verbose = RGD3_CVAR("net_verbose", "0", RGD3_CVAR_BOOL | RGD3_CVAR_SYSTEM, "Print a bunch of message about the network session");

static void rgd3_session_set_state(Rgd3SessionState state)
{
	if (cv_net_verbose.integer_value && state != rgd3_session.state)
		rgd3_printf("session: %s -> %s\n", rgd3_session_state_name(rgd3_session.state), rgd3_session_state_name(state));
	rgd3_session.state = state;
}

const char* rgd3_session_state_name(Rgd3SessionState state)
{
	switch (state)
	{
	case RGD3_SESSION_IDLE: return "IDLE";
	case RGD3_SESSION_LOADING: return "LOADING";
	case RGD3_SESSION_INGAME: return "INGAME";
	}
	return "?";
}

Rgd3SessionState rgd3_session_state(void)
{
	return rgd3_session.state;
}

const Rgd3MatchParms* rgd3_session_match_parms(void)
{
	return &rgd3_session.parms;
}

int rgd3_session_loading_id(void)
{
	return rgd3_session.loading_id;
}

void rgd3_session_start_match(const Rgd3MatchParms* parms)
{
	if (parms != &rgd3_session.parms)
	{
		rgd3_str_copynz(rgd3_session.parms.map_name, parms->map_name, sizeof(rgd3_session.parms.map_name));
		rgd3_session.parms.game_mode = parms->game_mode;
		rgd3_session.parms.game_map = parms->game_map;
		rgd3_session.parms.num_slots = parms->num_slots;
		rgd3_dict_assign(&rgd3_session.parms.server_info, &parms->server_info);
	}
	rgd3_session.loading_id++;
	rgd3_session_set_state(RGD3_SESSION_LOADING);
}

void rgd3_session_loading_finished(void)
{
	if (rgd3_session.state == RGD3_SESSION_LOADING)
		rgd3_session_set_state(RGD3_SESSION_INGAME);
}

void rgd3_session_quit_match_to_title(void)
{
	rgd3_session_set_state(RGD3_SESSION_IDLE);
}

void rgd3_session_pump(void)
{
	/* nothing to pump without lobbies; co-op sends and receives here */
}

int rgd3_session_is_multiplayer(void)
{
	return 0;
}

void rgd3_session_shutdown(void)
{
	rgd3_dict_clear(&rgd3_session.parms.server_info);
	rgd3_session.state = RGD3_SESSION_IDLE;
}

void rgd3_session_register(void)
{
	rgd3_cvar_register(&cv_net_verbose);
}
