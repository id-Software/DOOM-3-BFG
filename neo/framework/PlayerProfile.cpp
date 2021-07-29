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
#include "PlayerProfile.h"

// After releasing a version to the market, here are limitations for compatibility:
//	- the major version should not ever change
//	- always add new items to the bottom of the save/load routine
//	- never remove or change the size of items, just stop using them or add new items to the end
//
// The biggest reason these limitations exist is because if a newer profile is created and then loaded with a GMC
// version, we have to support it.
const int16		PROFILE_TAG					= ( 'D' << 8 ) | '3';
const int8		PROFILE_VER_MAJOR			= 10;	// If this is changed, you should reset the minor version and remove all backward compatible code
const int8		PROFILE_VER_MINOR			= 0;	// Within each major version, minor versions can be supported for backward compatibility

class idPlayerProfileLocal : public idPlayerProfile
{
};
idPlayerProfileLocal playerProfiles[MAX_INPUT_DEVICES];

/*
========================
Contains data that needs to be saved out on a per player profile basis, global for the lifetime of the player so
the data can be shared across computers.
- HUD tint colors
- key bindings
- etc...
========================
*/

/*
========================
idPlayerProfile * CreatePlayerProfile
========================
*/
idPlayerProfile* idPlayerProfile::CreatePlayerProfile( int deviceIndex )
{
	playerProfiles[deviceIndex].SetDefaults();
	playerProfiles[deviceIndex].deviceNum = deviceIndex;
	return &playerProfiles[deviceIndex];
}

/*
========================
idPlayerProfile::idPlayerProfile
========================
*/
idPlayerProfile::idPlayerProfile()
{
	SetDefaults();

	// Don't have these in SetDefaults because they're used for state management and SetDefaults is called when
	// loading the profile
	state				= IDLE;
	requestedState		= IDLE;
	deviceNum			= -1;
	dirty				= false;
}

/*
========================
idPlayerProfile::SetDefaults
========================
*/
void idPlayerProfile::SetDefaults()
{
	achievementBits = 0;
	achievementBits2	= 0;
	dlcReleaseVersion	= 0;

	stats.SetNum( MAX_PLAYER_PROFILE_STATS );
	for( int i = 0; i < MAX_PLAYER_PROFILE_STATS; ++i )
	{
		stats[i].i = 0;
	}

	leftyFlip = false;
	customConfig = false;
	configSet = 0;
}

/*
========================
idPlayerProfile::~idPlayerProfile
========================
*/
idPlayerProfile::~idPlayerProfile()
{
}

/*
========================
idPlayerProfile::Serialize
========================
*/
bool idPlayerProfile::Serialize( idSerializer& ser )
{
	// NOTE:
	// See comments at top of file on versioning rules

	// Default to current tag/version
	int32 magicNumber = 0;
	magicNumber += PROFILE_TAG << 16;
	magicNumber += PROFILE_VER_MAJOR << 8;
	magicNumber += PROFILE_VER_MINOR;

	// Serialize version
	ser.SerializePacked( magicNumber );
	int16 tag = ( magicNumber >> 16 ) & 0xffff;
	int8 majorVersion = ( magicNumber >> 8 ) & 0xff;
	int8 minorVersion = magicNumber & 0xff;
	minorVersion;

	if( tag != PROFILE_TAG )
	{
		return false;
	}

	if( majorVersion != PROFILE_VER_MAJOR )
	{
		return false;
	}

	// Archived cvars (all the menu settings for Doom3 are archived cvars)
	idDict cvarDict;
	cvarSystem->MoveCVarsToDict( CVAR_ARCHIVE, cvarDict );
	cvarDict.Serialize( ser );
	if( ser.IsReading() )
	{
		// Never sync these cvars with Steam because they require an engine or video restart
		cvarDict.Delete( "r_fullscreen" );
		cvarDict.Delete( "r_vidMode" );
		cvarDict.Delete( "r_multisamples" );
		cvarDict.Delete( "r_antiAliasing" );
		cvarDict.Delete( "com_engineHz" );
		cvarSystem->SetCVarsFromDict( cvarDict );
		common->StartupVariable( NULL );
	}

	// The dlcReleaseVersion is used to determine that new content is available
	ser.SerializePacked( dlcReleaseVersion );

	// New setting to save to make sure that we have or haven't seen this achievement before used to pass TRC R149d
	ser.Serialize( achievementBits );
	ser.Serialize( achievementBits2 );

	// Check to map sure we are on a valid map before we save, this helps prevent someone from creating a test map and
	// gaining a bunch of achievements from it
	int numStats = stats.Num();
	ser.SerializePacked( numStats );
	stats.SetNum( numStats );
	for( int i = 0; i < numStats; ++i )
	{
		ser.SerializePacked( stats[i].i );
	}

	ser.Serialize( leftyFlip );
	ser.Serialize( configSet );

	if( ser.IsReading() )
	{
		// Which binding is used on the console?
		ser.Serialize( customConfig );

		ExecConfig( false );

		if( customConfig )
		{
			for( int i = 0; i < K_LAST_KEY; ++i )
			{
				idStr bind;
				ser.SerializeString( bind );
				idKeyInput::SetBinding( i, bind.c_str() );
			}
		}
	}
	else
	{

		if( !customConfig )
		{
			ExecConfig( false );
		}

		customConfig = true;
		ser.Serialize( customConfig );

		for( int i = 0; i < K_LAST_KEY; ++i )
		{
			idStr bind = idKeyInput::GetBinding( i );
			ser.SerializeString( bind );
		}
	}

	return true;
}

/*
========================
idPlayerProfile::StatSetInt
========================
*/
void idPlayerProfile::StatSetInt( int s, int v )
{
	stats[s].i = v;
	MarkDirty( true );
}

/*
========================
idPlayerProfile::StatSetFloat
========================
*/
void idPlayerProfile::StatSetFloat( int s, float v )
{
	stats[s].f = v;
	MarkDirty( true );
}

/*
========================
idPlayerProfile::StatGetInt
========================
*/
int	idPlayerProfile::StatGetInt( int s ) const
{
	return stats[s].i;
}

/*
========================
idPlayerProfile::StatGetFloat
========================
*/
float idPlayerProfile::StatGetFloat( int s ) const
{
	return stats[s].f;
}

/*
========================
idPlayerProfile::SaveSettings
========================
*/
void idPlayerProfile::SaveSettings( bool forceDirty )
{
	if( state != SAVING )
	{
		if( forceDirty )
		{
			MarkDirty( true );
		}
		if( GetRequestedState() == IDLE && IsDirty() )
		{
			SetRequestedState( SAVE_REQUESTED );
		}
	}
}

/*
========================
idPlayerProfile::SaveSettings
========================
*/
void idPlayerProfile::LoadSettings()
{
	if( state != LOADING )
	{
		if( verify( GetRequestedState() == IDLE ) )
		{
			SetRequestedState( LOAD_REQUESTED );
		}
	}
}

/*
========================
idPlayerProfile::SetAchievement
========================
*/
void idPlayerProfile::SetAchievement( const int id )
{
	if( id >= idAchievementSystem::MAX_ACHIEVEMENTS )
	{
		assert( false );		// FIXME: add another set of achievement bit flags
		return;
	}

	uint64 mask = 0;
	if( id < 64 )
	{
		mask = achievementBits;
		achievementBits |= ( int64 )1 << id;
		mask = ~mask & achievementBits;
	}
	else
	{
		mask = achievementBits2;
		achievementBits2 |= ( int64 )1 << ( id - 64 );
		mask = ~mask & achievementBits2;
	}

	// Mark the profile dirty if achievement bits changed
	if( mask != 0 )
	{
		MarkDirty( true );
	}
}

/*
========================
idPlayerProfile::ClearAchievement
========================
*/
void idPlayerProfile::ClearAchievement( const int id )
{
	if( id >= idAchievementSystem::MAX_ACHIEVEMENTS )
	{
		assert( false );		// FIXME: add another set of achievement bit flags
		return;
	}

	if( id < 64 )
	{
		achievementBits &= ~( ( int64 )1 << id );
	}
	else
	{
		achievementBits2 &= ~( ( int64 )1 << ( id - 64 ) );
	}

	MarkDirty( true );
}

/*
========================
idPlayerProfile::GetAchievement
========================
*/
bool idPlayerProfile::GetAchievement( const int id ) const
{
	if( id >= idAchievementSystem::MAX_ACHIEVEMENTS )
	{
		assert( false );		// FIXME: add another set of achievement bit flags
		return false;
	}

	if( id < 64 )
	{
		return ( achievementBits & ( int64 )1 << id ) != 0;
	}
	else
	{
		return ( achievementBits2 & ( int64 )1 << ( id - 64 ) ) != 0;
	}
}

/*
========================
idPlayerProfile::SetConfig
========================
*/
void idPlayerProfile::SetConfig( int config, bool save )
{
	configSet = config;
	ExecConfig( save );
}

/*
========================
idPlayerProfile::SetConfig
========================
*/
void idPlayerProfile::RestoreDefault()
{
	ExecConfig( true, true );
}

/*
========================
idPlayerProfile::SetLeftyFlip
========================
*/
void idPlayerProfile::SetLeftyFlip( bool lf )
{
	leftyFlip = lf;
	ExecConfig( true );
}

/*
========================
idPlayerProfile::ExecConfig
========================
*/
void idPlayerProfile::ExecConfig( bool save, bool forceDefault )
{

	int flags = 0;
	if( !save )
	{
		flags = cvarSystem->GetModifiedFlags();
	}

	if( !customConfig || forceDefault )
	{
		cmdSystem->AppendCommandText( "exec default.cfg\n" );
		cmdSystem->AppendCommandText( "exec joy_360_0.cfg\n" );
	}

	if( leftyFlip )
	{
		cmdSystem->AppendCommandText( "exec joy_lefty.cfg\n" );
		cmdSystem->AppendCommandText( "exec joy_360_0.cfg\n" );
	}
	else
	{
		cmdSystem->AppendCommandText( "exec joy_righty.cfg\n" );
		cmdSystem->AppendCommandText( "exec joy_360_0.cfg\n" );
	}

	cmdSystem->ExecuteCommandBuffer();

	if( !save )
	{
		cvarSystem->ClearModifiedFlags( CVAR_ARCHIVE );
		cvarSystem->SetModifiedFlags( flags );
	}
}

CONSOLE_COMMAND( setProfileDefaults, "sets profile settings to default and saves", 0 )
{
	if( session->GetSignInManager().GetMasterLocalUser() == NULL )
	{
		return;
	}
	idPlayerProfile* profile = session->GetSignInManager().GetMasterLocalUser()->GetProfile();
	if( verify( profile != NULL ) )
	{
		profile->SetDefaults();
		profile->SaveSettings( true );
	}
}
