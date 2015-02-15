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

#include "Precompiled.h"
#include "globaldata.h"

#include <stdlib.h>
#include <stdarg.h>
#include <sys/types.h>

#include "i_video.h"
#include "i_system.h"

#include "doomstat.h"
#include "v_video.h"
#include "m_argv.h"
#include "d_main.h"

#include "doomdef.h"

#include "sys/sys_public.h"

#define ALLOW_CHEATS	1



extern int PLAYERCOUNT;

#define NUM_BUTTONS 4

static bool Cheat_God() {
	if( PLAYERCOUNT != 1 || ::g->netgame ) {
		return false;
	}
	::g->plyr->cheats ^= CF_GODMODE;
	if (::g->plyr->cheats & CF_GODMODE)
	{
		if (::g->plyr->mo)
			::g->plyr->mo->health = 100;

		::g->plyr->health = 100;
		::g->plyr->message = STSTR_DQDON;
	}
	else 
		::g->plyr->message = STSTR_DQDOFF;
	return true;
}

#include "g_game.h"
static bool Cheat_NextLevel() {
	if( PLAYERCOUNT != 1 || ::g->netgame ) {
		return false;
	}
	G_ExitLevel();

	return true;
}

static bool Cheat_GiveAll() {
	if( PLAYERCOUNT != 1 || ::g->netgame ) {
		return false;
	}

	::g->plyr->armorpoints = 200;
	::g->plyr->armortype = 2;

	int i;
	for (i=0;i<NUMWEAPONS;i++)
		::g->plyr->weaponowned[i] = true;

	for (i=0;i<NUMAMMO;i++)
		::g->plyr->ammo[i] = ::g->plyr->maxammo[i];

	for (i=0;i<NUMCARDS;i++)
		::g->plyr->cards[i] = true;

	::g->plyr->message = STSTR_KFAADDED;
	return true;
}

static bool Cheat_GiveAmmo() {
	if( PLAYERCOUNT != 1 || ::g->netgame ) {
		return false;
	}
	::g->plyr->armorpoints = 200;
	::g->plyr->armortype = 2;

	int i;
	for (i=0;i<NUMWEAPONS;i++)
		::g->plyr->weaponowned[i] = true;

	for (i=0;i<NUMAMMO;i++)
		::g->plyr->ammo[i] = ::g->plyr->maxammo[i];

	::g->plyr->message = STSTR_KFAADDED;
	return true;
}

static bool Cheat_Choppers() {
	if( PLAYERCOUNT != 1 || ::g->netgame ) {
		return false;
	}
	::g->plyr->weaponowned[wp_chainsaw] = true;
	::g->plyr->message = "Chainsaw!";
	return true;
}

extern qboolean P_GivePower ( player_t*	player, int /*powertype_t*/	power );

static void TogglePowerUp( int i ) {
	if (!::g->plyr->powers[i])
		P_GivePower( ::g->plyr, i);
	else if (i!=pw_strength)
		::g->plyr->powers[i] = 1;
	else
		::g->plyr->powers[i] = 0;

	::g->plyr->message = STSTR_BEHOLDX;
}

static bool Cheat_GiveInvul() {
	if( PLAYERCOUNT != 1 || ::g->netgame ) {
		return false;
	}

	TogglePowerUp( 0 );
	return true;
}

static bool Cheat_GiveBerserk() {
	if( PLAYERCOUNT != 1 || ::g->netgame ) {
		return false;
	}

	TogglePowerUp( 1 );
	return true;
}

static bool Cheat_GiveBlur() {
	if( PLAYERCOUNT != 1 || ::g->netgame ) {
		return false;
	}

	TogglePowerUp( 2 );
	return true;
}

static bool Cheat_GiveRad() {
	if( PLAYERCOUNT != 1 || ::g->netgame ) {
		return false;
	}

	TogglePowerUp( 3 );
	return true;
}

static bool Cheat_GiveMap() {
	if( PLAYERCOUNT != 1 || ::g->netgame ) {
		return false;
	}

	TogglePowerUp( 4 );
	return true;
}

static bool Cheat_GiveLight() {
	if( PLAYERCOUNT != 1 || ::g->netgame ) {
		return false;
	}

	TogglePowerUp( 5 );
	return true;
}



#ifndef __PS3__

static bool			tracking		= false;
static int			currentCode[NUM_BUTTONS];
static int			currentCheatLength;

#endif

typedef bool(*cheat_command)(void);
struct cheatcode_t
{
	int			code[NUM_BUTTONS];
	cheat_command	function;
};

static cheatcode_t codes[] = {
	{ {0, 1, 1, 0}, Cheat_God }, // a b b a
	{ {0, 0, 1, 1}, Cheat_NextLevel }, // a a b b
	{ {1, 0, 1, 0}, Cheat_GiveAmmo }, // b a b a
	{ {1, 1, 0, 0}, Cheat_Choppers}, // b b a a
	{ {0, 1, 0, 1}, Cheat_GiveAll },  // a b a b
	{ {2, 3, 3, 2}, Cheat_GiveInvul }, // x y y x
	{ {2, 2, 2, 3}, Cheat_GiveBerserk }, // x x x y
	{ {2, 2, 3, 3}, Cheat_GiveBlur }, // x x y y
	{ {2, 3, 3, 3}, Cheat_GiveRad }, // x y y y
	{ {3, 2, 3, 2}, Cheat_GiveMap }, // y x y x
	{ {3, 3, 3, 2}, Cheat_GiveLight}, // y y y x
};

const static int numberOfCodes = sizeof(codes) / sizeof(codes[0]);


void BeginTrackingCheat() {
#if ALLOW_CHEATS
	tracking = true;
	currentCheatLength = 0;
	memset( currentCode, 0, sizeof( currentCode ) );
#endif
}

void EndTrackingCheat() {
#if ALLOW_CHEATS
	tracking = false;
#endif
}

extern void S_StartSound ( void*		origin, int		sfx_id );

void CheckCheat( int button ) {
#if ALLOW_CHEATS
	if( tracking && !::g->netgame ) {

		currentCode[ currentCheatLength++ ] = button;

		if( currentCheatLength == NUM_BUTTONS ) {
			for( int i = 0; i < numberOfCodes; ++i) {
				if( memcmp( &codes[i].code[0], &currentCode[0], sizeof(currentCode) ) == 0 ) {
					if(codes[i].function()) {
						S_StartSound(0, sfx_cybsit);
					}
				}
			}
			// reset the code
			memset( currentCode, 0, sizeof( currentCode ) );
			currentCheatLength = 0;
		}
	}
#endif
}



float xbox_deadzone = 0.28f;

// input event storage
//PRIVATE TO THE INPUT THREAD!



void I_InitInput(void)
{
}

void I_ShutdownInput() 
{
}


static float _joyAxisConvert(short x, float xbxScale, float dScale, float deadZone)
{
	//const float signConverted = x - 127;
	float y = x - 127;
	y		= y / xbxScale;
	return (fabs(y) < deadZone) ? 0.f : (y * dScale);
}


int I_PollMouseInputEvents( controller_t *con) 
{
	int numEvents = 0;

	return numEvents;
}

int I_ReturnMouseInputEvent( const int n, event_t* e) {
	e->type = ev_mouse;
	e->data1 = e->data2 = e->data3 = 0;

	switch(::g->mouseEvents[n].type) {
	case IETAxis:
		switch (::g->mouseEvents[n].action)
		{
		case M_DELTAX:
			e->data2 = ::g->mouseEvents[n].data;
			break;
		case M_DELTAY:
			e->data3 = ::g->mouseEvents[n].data;
			break;
		}
		return 1;

	default:
		break;
	}
	return 0;
}

int I_PollJoystickInputEvents( controller_t *con ) {
	int numEvents	= 0;

	return numEvents;
}

//
//  Translates the key currently in X_event
//
static int xlatekey(int key)
{
	int rc = KEY_F1;
	
	switch (key)
	{
	case 0:	// A
		//rc = KEY_ENTER;
		rc = ' ';
		break;
	case 3: // Y
		rc = '1';
		break;
	case 1:	// B
		if( ::g->menuactive ) {
			rc = KEY_BACKSPACE;
		}
		else {
			rc = '2';
		}
		break;
	case 2: // X
		//rc = ' ';
		rc = KEY_TAB;
		break;
	case 4:	// White
		rc = KEY_MINUS;
		break;
	case 5: // Black	
		rc = KEY_EQUALS;
		break;
	case 6: // Left triggers
		rc = KEY_RSHIFT;
		break;
	case 7: // Right
		rc = KEY_RCTRL;
		break;
	case 8:	// Up
		if( ::g->menuactive ) {
			rc = KEY_UPARROW;
		}
		else {
			//rc = KEY_ENTER;
			rc = '3';
		}
		break;
	case 9:
		if( ::g->menuactive ) {
			rc = KEY_DOWNARROW;
		}
		else {
			//rc = KEY_TAB;
			rc = '5';
		}
		break;
	case 10:
		if( ::g->menuactive ) {
			rc = KEY_UPARROW;
		}
		else {
			//rc = '1';
			rc = '6';
		}
		break;
	case 11:
		if( ::g->menuactive ) {
			rc = KEY_DOWNARROW;
		}
		else {
			//rc = '2';
			rc = '4';
		}
		break;
	case 12:	// start
		rc = KEY_ESCAPE;
		break;
	case 13:	//select
		//rc = KEY_ESCAPE;
		break;
	case 14:	// lclick
	case 15:	// rclick
		//rc = ' ';
		break;
	}
    return rc;
}

int I_ReturnJoystickInputEvent( const int n, event_t* e) {

	e->data1 = e->data2 = e->data3 = 0;

	switch(::g->joyEvents[n].type)
	{
	case IETAxis:
		e->type = ev_joystick;//ev_mouse;
		switch (::g->joyEvents[n].action)
		{
		case J_DELTAX:
/*
			if (::g->joyEvents[n].data < 0) 
				e->data2 = -1;
			else if (::g->joyEvents[n].data > 0)
				e->data2 = 1;
*/
			e->data2 = ::g->joyEvents[n].data;
			break;
		case J_DELTAY:
			e->type = ev_mouse;
			e->data3 = ::g->joyEvents[n].data;
			break;
		}
		return 1;
	case IETButtonAnalog:
	case IETButtonDigital:
		if (::g->joyEvents[n].data) 
			e->type = ev_keydown;
		else
			e->type = ev_keyup;
		e->data1 = xlatekey(::g->joyEvents[n].action);
		return 1;

	case IETNone:
		break;
	}

	return 0;
}

void I_EndJoystickInputEvents() {
	int i;
	for(i = 0; i < 18; i++)
	{
		::g->joyEvents[i].type = IETNone;
	}

}

