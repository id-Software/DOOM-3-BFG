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

#include "precompiled.h"
#pragma hdrstop

#include "Common_local.h"

/*
==============
idCommonLocal::InitializeMPMapsModes
==============
*/
void idCommonLocal::InitializeMPMapsModes()
{

	const char** gameModes = NULL;
	const char** gameModesDisplay = NULL;
	int numModes = game->GetMPGameModes( &gameModes, &gameModesDisplay );
	mpGameModes.SetNum( numModes );
	for( int i = 0; i < numModes; i++ )
	{
		mpGameModes[i] = gameModes[i];
	}
	mpDisplayGameModes.SetNum( numModes );
	for( int i = 0; i < numModes; i++ )
	{
		mpDisplayGameModes[i] = gameModesDisplay[i];
	}
	int numMaps = declManager->GetNumDecls( DECL_MAPDEF );
	mpGameMaps.Clear();
	for( int i = 0; i < numMaps; i++ )
	{
		const idDeclEntityDef* mapDef = static_cast<const idDeclEntityDef*>( declManager->DeclByIndex( DECL_MAPDEF, i ) );
		uint32 supportedModes = 0;
		for( int j = 0; j < numModes; j++ )
		{
			if( mapDef->dict.GetBool( gameModes[j], false ) )
			{
				supportedModes |= BIT( j );
			}
		}
		if( supportedModes != 0 )
		{
			mpMap_t& mpMap = mpGameMaps.Alloc();
			mpMap.mapFile = mapDef->GetName();
			mpMap.mapName = mapDef->dict.GetString( "name", mpMap.mapFile );
			mpMap.supportedModes = supportedModes;
		}
	}
}

/*
==============
idCommonLocal::OnStartHosting
==============
*/
void idCommonLocal::OnStartHosting( idMatchParameters& parms )
{
	if( ( parms.matchFlags & MATCH_REQUIRE_PARTY_LOBBY ) == 0 )
	{
		return; // This is the party lobby or a SP match
	}
	
	// If we were searching for a random match but didn't find one, we'll need to select parameters now
	if( parms.gameMap < 0 )
	{
		if( parms.gameMode < 0 )
		{
			// Random mode means any map will do
			parms.gameMap = idLib::frameNumber % mpGameMaps.Num();
		}
		else
		{
			// Select a map which supports the chosen mode
			idList<int> supportedMaps;
			uint32 supportedMode = BIT( parms.gameMode );
			for( int i = 0; i < mpGameMaps.Num(); i++ )
			{
				if( mpGameMaps[i].supportedModes & supportedMode )
				{
					supportedMaps.Append( i );
				}
			}
			if( supportedMaps.Num() == 0 )
			{
				// We don't have any maps which support the chosen mode...
				parms.gameMap = idLib::frameNumber % mpGameMaps.Num();
				parms.gameMode = -1;
			}
			else
			{
				parms.gameMap = supportedMaps[ idLib::frameNumber % supportedMaps.Num() ];
			}
		}
	}
	if( parms.gameMode < 0 )
	{
		uint32 supportedModes = mpGameMaps[parms.gameMap].supportedModes;
		int8 supportedModeList[32] = {};
		int numSupportedModes = 0;
		for( int i = 0; i < 32; i++ )
		{
			if( supportedModes & BIT( i ) )
			{
				supportedModeList[numSupportedModes] = i;
				numSupportedModes++;
			}
		}
		parms.gameMode = supportedModeList[( idLib::frameNumber / mpGameMaps.Num() ) % numSupportedModes ];
	}
	parms.mapName = mpGameMaps[parms.gameMap].mapFile;
	parms.numSlots = session->GetTitleStorageInt( "MAX_PLAYERS_ALLOWED", 4 );
}

/*
==============
idCommonLocal::StartMainMenu
==============
*/
void idCommonLocal::StartMenu( bool playIntro )
{
	if( game && game->Shell_IsActive() )
	{
		return;
	}
	
	if( readDemo )
	{
		// if we're playing a demo, esc kills it
		UnloadMap();
	}
	
	if( game )
	{
		game->Shell_Show( true );
		game->Shell_SyncWithSession();
	}
	
	console->Close();
	
}

/*
===============
idCommonLocal::ExitMenu
===============
*/
void idCommonLocal::ExitMenu()
{
	if( game )
	{
		game->Shell_Show( false );
	}
}

/*
==============
idCommonLocal::MenuEvent

Executes any commands returned by the gui
==============
*/
bool idCommonLocal::MenuEvent( const sysEvent_t* event )
{

	if( session->GetSignInManager().ProcessInputEvent( event ) )
	{
		return true;
	}
	
	if( game && game->Shell_IsActive() )
	{
		return game->Shell_HandleGuiEvent( event );
	}
	
	if( game )
	{
		return game->HandlePlayerGuiEvent( event );
	}
	
	return false;
}

/*
=================
idCommonLocal::GuiFrameEvents
=================
*/
void idCommonLocal::GuiFrameEvents()
{
	if( game )
	{
		game->Shell_SyncWithSession();
	}
}
