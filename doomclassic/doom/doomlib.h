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

#ifndef _DOOM_LIB_H
#define _DOOM_LIB_H


#include "doomtype.h"
#include "doomdef.h"
#include "doominterface.h"

#include "idlib/containers/Array.h"

class idSysMutex;
class idUserCmdMgr;

#define IN_NUM_DIGITAL_BUTTONS 8
#define IN_NUM_ANALOG_BUTTONS 8
// Cutoff where the analog buttons are considered to be "pressed"
// This should be smarter.
#define IN_ANALOG_BUTTON_THRESHOLD 64

extern idCVar s_volume_sound;
extern idCVar s_volume_midi;
extern idCVar m_show_messages;
extern idCVar m_inDemoMode;
struct rumble_t
{
	int				feedback;	// SMF			// XINPUT_FEEDBACK feedback;
	int				endTime;

	// The following values are needed, becuase a rumble
	// can fail, if it hasn't processed the previous one yet,
	// so, it must be stored
	bool			waiting;
	int				left;
	int				right;
	
};

enum gameSKU_t {
	GAME_SKU_DCC = 0,				// Doom Classic Complete
	GAME_SKU_DOOM1_BFG,				// Doom 1 Ran from BFG
	GAME_SKU_DOOM2_BFG,				// Doom 2 Ran from BFG

};

/*
================================================================================================
 ExpansionData
================================================================================================
*/
struct ExpansionData {

	enum { IWAD = 0, PWAD = 1 }	type;
	GameMode_t					gameMode;
	GameMission_t				pack_type;
	const char *				expansionName;
	const char *				iWadFilename;
	const char *				pWadFilename;
	const char *				saveImageFile;
	const char **				mapNames;


};



namespace DoomLib
{
	//typedef int ( *RecvFunc)( char* buff, DWORD *numRecv );
	//typedef int ( *SendFunc)( const char* buff, DWORD size, sockaddr_in *target, int toNode );
	//typedef int ( *SendRemoteFunc)();

	void InitGlobals( void *ptr = NULL );
	void InitGame( int argc, char ** argv );
	void InitControlRemap();
	keyNum_t RemapControl( keyNum_t key );
	bool Poll();
	bool Tic( idUserCmdMgr * userCmdMgr );
	void Wipe();
	void Frame( int realoffset = 0, int buffer = 0 );
	void Draw();
	void Shutdown();

	//void SetNetworking( RecvFunc rf, SendFunc sf, SendRemoteFunc sendRemote );
	
	void SetPlayer( int id );
	int GetPlayer();

	byte BuildSourceDest( int toNode );
	void GetSourceDest( byte sourceDest, int* source, int* dest );

	int RemoteNodeToPlayerIndex( int node );
	int PlayerIndexToRemoteNode( int index );

	void PollNetwork();
	void SendNetwork();

	void *GetGlobalData( int player );

	void RunSound();

	//extern RecvFunc Recv;
	//extern SendFunc Send;
	//extern SendRemoteFunc SendRemote;

	extern void* 	(*Z_Malloc)( int size, int tag, void* user );
	extern void 	(*Z_FreeTag)(int lowtag );

	extern DoomInterface		Interface;
	extern int					idealExpansion;
	extern int					expansionSelected;
	extern bool					expansionDirty;

	extern bool					skipToLoad;
	extern char					loadGamePath[MAX_PATH];

	extern bool					skipToNew;
	extern int					chosenSkill;
	extern int					chosenEpisode;

	extern idMatchParameters	matchParms;

	const ExpansionData *		GetCurrentExpansion();
	void						SetCurrentExpansion( int expansion );

	void						SetIdealExpansion( int expansion );

	void						SetCurrentMapName( idStr name );
	const idStr &				GetCurrentMapName();
	void						SetCurrentDifficulty( idStr name );
	const idStr &				GetCurrentDifficulty();

	void						ActivateGame();
	void						ExitGame();
	void						ShowXToContinue( bool activate );
	gameSKU_t					GetGameSKU();
	void						HandleEndMatch();
	
};


#endif
