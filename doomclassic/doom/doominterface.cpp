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

#include <stdio.h>

#include "globaldata.h"
#include "doominterface.h"
#include "Main.h"
#include "m_menu.h"
#include "g_game.h"

extern void I_SetTime( int );

bool waitingForWipe;

static const int dargc = 7;
static const char* dargv[4][7] =
{
	{ "doomlauncher", "-net", "0", "127.0.0.1", "127.0.0.1", "127.0.0.1", "127.0.0.1" },
	{ "doomlauncher", "-net", "1", "127.0.0.1", "127.0.0.1", "127.0.0.1", "127.0.0.1" },
	{ "doomlauncher", "-net", "2", "127.0.0.1", "127.0.0.1", "127.0.0.1", "127.0.0.1" },
	{ "doomlauncher", "-net", "3", "127.0.0.1", "127.0.0.1", "127.0.0.1", "127.0.0.1" },
};

static int				mpArgc[4];
static char				mpArgV[4][10][32];
static char*			mpArgVPtr[4][10];

static bool drawFullScreen = false;

DoomInterface::DoomInterface() {
	numplayers = 0;
	bFinished[0] = bFinished[1] = bFinished[2] = bFinished[3] = false;
	lastTicRun = 0;
}

DoomInterface::~DoomInterface() {
}


void DoomInterface::Startup( int playerscount, bool multiplayer )
{
	int i;
	int localdargc = 1; // for the commandline

	numplayers			= playerscount;
	globalNetworking	= multiplayer;
	lastTicRun			= 0;

	if (DoomLib::Z_Malloc == NULL) {
		DoomLib::Z_Malloc = Z_Malloc;
	}

	// Splitscreen
	if ( !multiplayer && playerscount > 1 ) {
		localdargc += 2; // for the '-net' and the console number
		localdargc += playerscount;
	}

	if ( multiplayer ) {
		// Force online games to 1 local player for now.
		// TODO: We should support local splitscreen and online.
		numplayers = 1;
	}

	// Start up DooM Classic
	for ( i = 0; i < numplayers; ++i)
	{
		DoomLib::SetPlayer(i);

		bFinished[i] = false;
		DoomLib::InitGlobals( NULL );

		if ( globalNetworking ) {
			printf( "Starting mulitplayer game, argv = " );
			for ( int j = 0; j < mpArgc[0]; ++j ) {
				printf( " %s", mpArgVPtr[0][j] );
			}
			printf( "\n" );
			DoomLib::InitGame(mpArgc[i], mpArgVPtr[i] );
		} else {
			DoomLib::InitGame(localdargc, (char **)dargv[i] );
		}

		if( DoomLib::skipToLoad ) {
			G_LoadGame( DoomLib::loadGamePath );
			 DoomLib::skipToLoad = false;
			 ::g->menuactive = 0;
		}

		if( DoomLib::skipToNew ) {
			static int startLevel = 1;
			G_DeferedInitNew((skill_t)DoomLib::chosenSkill,DoomLib::chosenEpisode+1, startLevel);
			DoomLib::skipToNew = false;
			::g->menuactive = 0;
		}

		DoomLib::SetPlayer(-1);
	}
}

bool DoomInterface::Frame( int iTime, idUserCmdMgr * userCmdMgr )
{
	int i;
	bool bAllFinished = true;

	if ( !globalNetworking || ( lastTicRun < iTime ) ) {

		drawFullScreen = false;

		DoomLib::SetPlayer( 0 );
		DoomLib::PollNetwork();

		for (i = 0; i < numplayers; ++i)
		{
			DoomLib::SetPlayer( i );

			I_SetTime( iTime );

			if (bFinished[i] == false) {
				bAllFinished = false;
				bFinished[i] = DoomLib::Poll();
			} else {

				if (::g->wipedone) {
					if ( !waitingForWipe ) {
						const bool didRunTic = DoomLib::Tic( userCmdMgr );
						if ( didRunTic == false ) {
							//printf( "Skipping tic and yielding because not enough time has passed.\n" );
						
							// Give lower priority threads a chance to run.
							Sys_Yield();
						}
					}
					DoomLib::Frame();
				}
				if (::g->wipe) {
					DoomLib::Wipe();
					// Draw the menus over the wipe.
					M_Drawer();
				}

				if( ::g->gamestate != GS_LEVEL && GetNumPlayers() > 2 ) {
					drawFullScreen = true;
				}
			}

			DoomLib::SetPlayer(-1);
		}

		DoomLib::SetPlayer( 0 );
		DoomLib::SendNetwork();
		DoomLib::RunSound();
		DoomLib::SetPlayer( -1 );

		lastTicRun = iTime;
	} else {
		printf( "Skipping this frame becase it's not time to run a tic yet.\n" );
	}

	return bAllFinished;
}

void I_ShutdownNetwork();

void DoomInterface::Shutdown() {
	int i;

	for ( i=0; i < numplayers; i++ ) {
		DoomLib::SetPlayer( i );
		D_QuitNetGame();
	}

	// Shutdown local network state
	I_ShutdownNetwork();

	for ( i=0; i < numplayers; i++ ) {
		DoomLib::SetPlayer( i );
		DoomLib::Shutdown();
	}

	DoomLib::SetPlayer( -1 );
	numplayers = 0;
	lastTicRun = 0;
}

qboolean G_CheckDemoStatus();

void DoomInterface::QuitCurrentGame() {
	for ( int i = 0; i < numplayers; i++ ) {
		DoomLib::SetPlayer( i );

		if(::g->netgame) {
			// Shut down networking
			D_QuitNetGame();
		}

		G_CheckDemoStatus();

		globalPauseTime = false;
		::g->menuactive = false;
		::g->usergame = false;
		::g->netgame = false;

		lastTicRun = 0;

		//if ( !gameLocal->IsSplitscreen() ) {
			// Start background demos
			D_StartTitle();
		//}
	}

	// Shutdown local network state
	I_ShutdownNetwork();
}

void DoomInterface::EndDMGame() {

	for ( int i = 0; i < numplayers; i++ ) {
		DoomLib::SetPlayer( i );

		if(::g->netgame) {
			D_QuitNetGame();
		}

		G_CheckDemoStatus();

		globalPauseTime = false;
		::g->menuactive = false;
		::g->usergame = false;
		::g->netgame = false;

		lastTicRun = 0;

		D_StartTitle();
	}
}

//static 
int DoomInterface::CurrentPlayer() {
	return DoomLib::GetPlayer();
}

int DoomInterface::GetNumPlayers() const {
	return numplayers;
}

#ifdef ID_ENABLE_DOOM_CLASSIC_NETWORKING
void DoomInterface::SetNetworking( DoomLib::RecvFunc recv, DoomLib::SendFunc send, DoomLib::SendRemoteFunc sendRemote ) {
	DoomLib::SetNetworking( recv, send, sendRemote );
}
#endif

void DoomInterface::SetMultiplayerPlayers(int localPlayerIndex, int playerCount, int localPlayer, std::vector<std::string> playerAddresses) {
	
	for(int i = 0; i < 10; i++) {
		mpArgVPtr[localPlayerIndex][i] = mpArgV[localPlayerIndex][i];
	}
	
	mpArgc[localPlayerIndex] = playerCount+5;

	strcpy(mpArgV[localPlayerIndex][0], "doomlauncher");
	strcpy(mpArgV[localPlayerIndex][1], "-dup");
	strcpy(mpArgV[localPlayerIndex][2], "1");
	strcpy(mpArgV[localPlayerIndex][3], "-net");
	
	sprintf(mpArgV[localPlayerIndex][4], "%d", localPlayer);
	strcpy(mpArgV[localPlayerIndex][5], playerAddresses[localPlayer].c_str());

	int currentArg = 6;
	for(int i = 0; i < playerCount; i++) {
		if(i != localPlayer) {
			strcpy(mpArgV[localPlayerIndex][currentArg], playerAddresses[i].c_str());
			currentArg++;
		}
	}
}

