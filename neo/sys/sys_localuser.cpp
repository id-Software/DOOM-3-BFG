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

extern idCVar fs_savepath;

/*
========================
idLocalUser::idLocalUser
========================
*/
idLocalUser::idLocalUser()
{
	memset( joiningLobby, 0, sizeof( joiningLobby ) );
	profileMgr.Init( this );
	syncAchievementsRequested = 0;
}

void idLocalUser::Pump()
{
	// Pump the profile
	GetProfileMgr().Pump();

	if( GetProfileMgr().GetProfile() != NULL && GetProfileMgr().GetProfile()->GetState() == idPlayerProfile::IDLE )
	{
		// Pump achievements
		if( syncAchievementsRequested )
		{
			if( session->GetAchievementSystem().IsInitialized() )
			{
				session->GetAchievementSystem().SyncAchievementBits( this );
				syncAchievementsRequested = false;
			}
		}
		session->GetAchievementSystem().Pump();
	}

	// Extra platform pump if necessary
	PumpPlatform();
}

/*
========================
idLocalUser::IsStorageDeviceAvailable
========================
*/
bool idLocalUser::IsStorageDeviceAvailable() const
{
	return saveGame_enable.GetBool();
}

/*
========================
idLocalUser::ResetStorageDevice
========================
*/
void idLocalUser::ResetStorageDevice()
{
}

/*
========================
idLocalUser::StorageSizeAvailable
========================
*/
// RB: disabled savegame and profile storage checks, because it fails sometimes without any clear reason
/*
bool idLocalUser::StorageSizeAvailable( uint64 minSizeInBytes, int64& neededBytes )
{
	int64 size = Sys_GetDriveFreeSpaceInBytes( fs_savepath.GetString() );

	neededBytes = minSizeInBytes - size;
	if( neededBytes < 0 )
	{
		neededBytes = 0;
	}

	return neededBytes == 0;
}
*/
// RB end

/*
========================
idLocalUser::SetStatInt
========================
*/
void idLocalUser::SetStatInt( int s, int v )
{
	idPlayerProfile* profile = GetProfile();
	if( profile != NULL )
	{
		return profile->StatSetInt( s, v );
	}
}

/*
========================
idLocalUser::SetStatFloat
========================
*/
void idLocalUser::SetStatFloat( int s, float v )
{
	idPlayerProfile* profile = GetProfile();
	if( profile != NULL )
	{
		return profile->StatSetFloat( s, v );
	}
}

/*
========================
idLocalUser::GetStatInt
========================
*/
int	idLocalUser::GetStatInt( int s )
{
	const idPlayerProfile* profile = GetProfile();

	if( profile != NULL && s >= 0 )
	{
		return profile->StatGetInt( s );
	}

	return 0;
}

/*
========================
idLocalUser::GetStatFloat
========================
*/
float idLocalUser::GetStatFloat( int s )
{
	const idPlayerProfile* profile = GetProfile();

	if( profile != NULL )
	{
		return profile->StatGetFloat( s );
	}

	return 0.0f;
}

/*
========================
idLocalUser::LoadProfileSettings
========================
*/
void idLocalUser::LoadProfileSettings()
{
	idPlayerProfile* profile = GetProfileMgr().GetProfile();

	// Lazy instantiation
	if( profile == NULL )
	{
		// Create a new profile
		profile = idPlayerProfile::CreatePlayerProfile( GetInputDevice() );
	}

	if( profile != NULL )
	{
		profile->LoadSettings();
	}

	return;
}

/*
========================
idLocalUser::SaveProfileSettings
========================
*/
void idLocalUser::SaveProfileSettings()
{
	idPlayerProfile* profile = GetProfileMgr().GetProfile();
	if( profile != NULL )
	{
		profile->SaveSettings( true );
	}

	return;
}

/*
========================
localUserHandle_t::Serialize
========================
*/
void localUserHandle_t::Serialize( idSerializer& ser )
{
	ser.Serialize( handle );
}
