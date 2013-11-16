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
#include <stdio.h>
#include <string.h>

#include <stdarg.h>

#include "doomdef.h"
#include "m_misc.h"
#include "i_video.h"
#include "i_sound.h"

#include "d_net.h"
#include "g_game.h"

#ifdef __GNUG__
#pragma implementation "i_system.h"
#endif
#include "i_system.h"

#include "Main.h"

#if  1



ticcmd_t*	I_BaseTiccmd(void)
{
    return &::g->emptycmd;
}


int  I_GetHeapSize (void)
{
    return ::g->mb_used*1024*1024;
}


//
// I_GetTime
// returns time in 1/70th second tics
//
int  I_GetTime (void)
{
	return ::g->current_time;
}

void I_SetTime( int time_in )
{
	::g->current_time = time_in;
}



//
// I_Init
//
void I_Init (void)
{
    I_InitSound();
    //  I_InitGraphics();
}

//
// I_Quit
//
void I_Quit (void)
{
    D_QuitNetGame ();
    I_ShutdownSound();
    I_ShutdownMusic();
    M_SaveDefaults ();
    I_ShutdownGraphics();
//    exit(0);

// Exceptions disabled by default on PS3
//	throw;
}

void I_WaitVBL(int count)
{
	// PS3 fixme
	//Sleep(0);
}

void I_BeginRead(void)
{
}

void I_EndRead(void)
{
}

//
// I_Error
//
extern bool debugOutput;
void I_Printf(const char* msg, ...)
{
	char pmsg[1024];
    va_list	argptr;

    // Message first.
	if( debugOutput ) {
		va_start (argptr,msg);
		vsprintf (pmsg, msg, argptr);

		safeOutputDebug(pmsg);

		va_end (argptr);
	}
}


void I_PrintfE(const char* msg, ...)
{
	char pmsg[1024];
    va_list	argptr;

    // Message first.
	if( debugOutput ) {
		va_start (argptr,msg);
		vsprintf (pmsg, msg, argptr);

		safeOutputDebug("ERROR: ");
		safeOutputDebug(pmsg);

	    va_end (argptr);
	}
}

void I_Error(const char *error, ...)
{
	const int ERROR_MSG_SIZE = 1024;
	char error_msg[ERROR_MSG_SIZE];
    va_list	argptr;

    // Message first.
	if( debugOutput ) {
		va_start (argptr,error);
		idStr::vsnPrintf (error_msg, ERROR_MSG_SIZE, error, argptr);

		safeOutputDebug("Error: ");
		safeOutputDebug(error_msg);
		safeOutputDebug("\n");

		va_end (argptr);
	}

	// CRASH DUMP - enable this to get extra info on error from crash dumps
	//*(int*)0x0 = 21;
	DoomLib::Interface.QuitCurrentGame();
	idLib::Printf( "DOOM Classic error: %s", error_msg );
	common->SwitchToGame( DOOM3_BFG );
}

#endif

