/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company. 

This file is part of the Doom 3 BFG Edition GPL Source Code ("Doom 3 BFG Edition Source Code").  

Doom 3 BFG Edition Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 BFG Edition Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 BFG Edition Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 BFG Edition Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 BFG Edition Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/


#ifndef _MAIN_H_
#define _MAIN_H_

#include "idlib/precompiled.h"

#include "../doom/doomlib.h"
#include "../doom/doominterface.h"
#include "../doom/globaldata.h"


// DHM - Nerve :: Enable demo recording for game clips
#define _DEMO_RECORDING

#ifdef _DEBUG
	#define safeOutputDebug(x) printf( "%s", x );
#else
	#define safeOutputDebug(x)
#endif

struct SplitscreenData {
	int		PLAYERCOUNT;
	int		globalSkill;
	int		globalEpisode;
	int		globalLevel;
	int		globalTimeLimit;
	int		globalFragLimit;
};

void			DL_InitNetworking( DoomInterface *pdi );

extern int		PLAYERCOUNT;
extern bool		globalNetworking;
extern bool		debugOutput;
extern bool		globalLicenseFullGame;
extern int		globalRichPresenceState;  // values from spa.h X_CONTEXT_PRESENCE
extern int		globalNeedUpsell;
// PS3
//extern HXUISTRINGTABLE globalStrings;     // gStrings for short
extern bool		globalPauseTime;


enum MenuStates{
	MENU_NONE,
	MENU_XBOX_SYSTEM,
	MENU_PAUSE,
	MENU_UPSELL,
	MENU_UPSELL_INVITE,
	MENU_ENDLEVEL_UPSELL,
	MENU_ERROR_MESSAGE,
	MENU_ERROR_MESSAGE_FATAL,
	MENU_END_LEVEL,
	MENU_END_EPISODE,
	MENU_END_CAST,
	MENU_END_LEVEL_COOP,
	MENU_END_LEVEL_DM,
	MENU_END_GAME_LOBBY,
	MENU_END_GAME_LOBBY_PLAYER,
	MENU_LOBBY,
	MENU_LOBBY_PLAYER,
	MENU_INVITE,
	MENU_COUNT
};

typedef struct {
	int maxPing;
	
	const wchar_t *	image;
} PingImage_t;

extern PingImage_t pingsImages[];


#endif