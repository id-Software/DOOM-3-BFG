/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2022 Robert Beckebans

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
/*
game_worldspawn.cpp

Worldspawn class.  Each map has one worldspawn which handles global spawnargs.

*/

#include "precompiled.h"
#pragma hdrstop

#include "Game_local.h"


const idEventDef EV_PlayBackgroundMusic( "<playBackgroundMusic>", NULL );

/*
================
idWorldspawn

Every map should have exactly one worldspawn.
================
*/
CLASS_DECLARATION( idEntity, idWorldspawn )
EVENT( EV_Remove,				idWorldspawn::Event_Remove )
EVENT( EV_SafeRemove,			idWorldspawn::Event_Remove )
EVENT( EV_PlayBackgroundMusic,	idWorldspawn::Event_PlayBackgroundMusic )
END_CLASS

/*
================
idWorldspawn::Spawn
================
*/
void idWorldspawn::Spawn()
{
	idStr				scriptname;
	idThread*			thread;
	const function_t*	func;
	const idKeyValue*	kv;

	assert( gameLocal.world == NULL );
	gameLocal.world = this;

	g_gravity.SetFloat( spawnArgs.GetFloat( "gravity", va( "%f", DEFAULT_GRAVITY ) ) );

	// RB: start some background music Quake style
	SetMusicTrack();

	// disable stamina on hell levels
	if( spawnArgs.GetBool( "no_stamina" ) )
	{
		pm_stamina.SetFloat( 0.0f );
	}

	// load script
	scriptname = gameLocal.GetMapName();
	scriptname.SetFileExtension( ".script" );
	if( fileSystem->ReadFile( scriptname, NULL, NULL ) > 0 )
	{
		gameLocal.program.CompileFile( scriptname );

		// call the main function by default
		func = gameLocal.program.FindFunction( "main" );
		if( func != NULL )
		{
			thread = new idThread( func );
			thread->DelayedStart( 0 );
		}
	}

	// call any functions specified in worldspawn
	kv = spawnArgs.MatchPrefix( "call" );
	while( kv != NULL )
	{
		func = gameLocal.program.FindFunction( kv->GetValue() );
		if( func == NULL )
		{
			gameLocal.Error( "Function '%s' not found in script for '%s' key on worldspawn", kv->GetValue().c_str(), kv->GetKey().c_str() );
		}

		thread = new idThread( func );
		thread->DelayedStart( 0 );
		kv = spawnArgs.MatchPrefix( "call", kv );
	}
}

/*
=================
idWorldspawn::Save
=================
*/
void idWorldspawn::Save( idSaveGame* savefile )
{
}

/*
=================
idWorldspawn::Restore
=================
*/
void idWorldspawn::Restore( idRestoreGame* savefile )
{
	assert( gameLocal.world == this );

	g_gravity.SetFloat( spawnArgs.GetFloat( "gravity", va( "%f", DEFAULT_GRAVITY ) ) );

	// RB: start some background music Quake style
	SetMusicTrack();

	// disable stamina on hell levels
	if( spawnArgs.GetBool( "no_stamina" ) )
	{
		pm_stamina.SetFloat( 0.0f );
	}
}

/*
================
idWorldspawn::~idWorldspawn
================
*/
idWorldspawn::~idWorldspawn()
{
	if( gameLocal.world == this )
	{
		gameLocal.world = NULL;
	}
}

/*
================
idWorldspawn::Event_Remove
================
*/
void idWorldspawn::Event_Remove()
{
	gameLocal.Error( "Tried to remove world" );
}

// RB begin
void idWorldspawn::SetMusicTrack()
{
	idStr music = spawnArgs.GetString( "music", "" );
	if( music != "" )
	{
		musicTrack = music;

		// play it after a few seconds
		PostEventSec( &EV_PlayBackgroundMusic, 3 );
	}
	else
	{
		// scan for music/track*.ogg files the user installed
		idFileList*	soundTracks;
		soundTracks =  fileSystem->ListFilesTree( "music", ".ogg", true );

		if( soundTracks->GetList().Num() )
		{
			idStr mapnameShort = gameLocal.GetMapName();

			// make sure that every map has a unique soundtrack
			idStrList mapList;
			mapList.AddUnique( "game/mars_city1" );
			mapList.AddUnique( "game/mc_underground" );
			mapList.AddUnique( "game/mars_city2" );
			mapList.AddUnique( "game/admin" );
			mapList.AddUnique( "game/alphalabs1" );
			mapList.AddUnique( "game/alphalabs2" );
			mapList.AddUnique( "game/alphalabs3" );
			mapList.AddUnique( "game/alphalabs4" );
			mapList.AddUnique( "game/enpro" );
			mapList.AddUnique( "game/commoutside" );
			mapList.AddUnique( "game/comm1" );
			mapList.AddUnique( "game/recycling1" );
			mapList.AddUnique( "game/recycling2" );
			mapList.AddUnique( "game/monorail" );
			mapList.AddUnique( "game/delta1" );
			mapList.AddUnique( "game/delta2a" );
			mapList.AddUnique( "game/delta2b" );
			mapList.AddUnique( "game/delta3" );
			mapList.AddUnique( "game/delta4" );
			mapList.AddUnique( "game/hell1" );
			mapList.AddUnique( "game/delta5" );
			mapList.AddUnique( "game/cpu" );
			mapList.AddUnique( "game/cpuboss" );
			mapList.AddUnique( "game/site3" );
			mapList.AddUnique( "game/caverns1" );
			mapList.AddUnique( "game/caverns2" );
			mapList.AddUnique( "game/hellhole" );
			//mapList.AddUnique(NULL, "-DOOM 3 Expansion-" ) );
			mapList.AddUnique( "game/erebus1" );
			mapList.AddUnique( "game/erebus2" );
			mapList.AddUnique( "game/erebus3" );
			mapList.AddUnique( "game/erebus4" );
			mapList.AddUnique( "game/erebus5" );
			mapList.AddUnique( "game/erebus6" );
			mapList.AddUnique( "game/phobos1" );
			mapList.AddUnique( "game/phobos2" );
			mapList.AddUnique( "game/phobos3" );
			mapList.AddUnique( "game/phobos4" );
			mapList.AddUnique( "game/deltax" );
			mapList.AddUnique( "game/hell" );
			//mapList.AddUnique(NULL, "-Lost Missions-" ) );
			mapList.AddUnique( "game/le_enpro1" );
			mapList.AddUnique( "game/le_enpro2" );
			mapList.AddUnique( "game/le_underground" );
			mapList.AddUnique( "game/le_underground2" );
			mapList.AddUnique( "game/le_exis1" );
			mapList.AddUnique( "game/le_exis2" );
			mapList.AddUnique( "game/le_hell" );
			mapList.AddUnique( "game/le_hell_post" );

			int mapIndex = -1;
			for( int i = 0; i < mapList.Num(); i++ )
			{
				const char* mapStr = mapList[ i ].c_str();
				if( mapnameShort.Find( mapStr ) != -1 )
				{
					mapIndex = i;
					break;
				}
			}

			if( mapIndex == -1 )
			{
				// unknown map
				mapIndex = idStr::Hash( gameLocal.GetMapName() );
			}

			mapIndex %= soundTracks->GetList().Num();

			// skip it for mars_city1
			if( mapnameShort.Find( "mars_city1" ) == -1 )
			{
				musicTrack = soundTracks->GetList()[ mapIndex ];

				const idSoundShader* soundShader = declManager->FindSound( musicTrack );
				if( soundShader->GetState() == DS_DEFAULTED )
				{
					// this is bad, we have no sound shader found that enables the loop
					// this will only play the music until it ends
					musicTrack = soundTracks->GetList()[ mapIndex ];
				}

				// play it after a few seconds
				PostEventSec( &EV_PlayBackgroundMusic, 3 );
			}
		}

		fileSystem->FreeFileList( soundTracks );
	}
	// RB end
}

void idWorldspawn::Event_PlayBackgroundMusic()
{
	if( !musicTrack.IsEmpty() )
	{
		common->Printf( "Playing custom music sound track: %s\n", musicTrack.c_str() );
		gameSoundWorld->PlayShaderDirectly( musicTrack, SND_CHANNEL_MUSIC );
	}
}
// RB end
