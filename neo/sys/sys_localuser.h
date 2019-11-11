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
#ifndef __SYS_LOCALUSER_H__
#define __SYS_LOCALUSER_H__

#include "sys_profile.h"

struct achievementDescription_t;
class idPlayerProfile;
class idProfileMgr;

enum onlineCaps_t
{
	CAP_IS_ONLINE			= BIT( 0 ),
	CAP_BLOCKED_PERMISSION	= BIT( 1 ),
	CAP_CAN_PLAY_ONLINE		= BIT( 2 ),
};

class idSerializer;

/*
================================================
localUserHandle_t
================================================
*/
struct localUserHandle_t
{
public:
	typedef uint32 userHandleType_t;

	localUserHandle_t() : handle( 0 ) {}

	explicit localUserHandle_t( userHandleType_t handle_ ) : handle( handle_ ) {}

	bool operator == ( const localUserHandle_t& other ) const
	{
		return handle == other.handle;
	}

	bool operator < ( const localUserHandle_t& other ) const
	{
		return handle < other.handle;
	}

	bool IsValid() const
	{
		return handle > 0;
	}

	void WriteToMsg( idBitMsg& msg )
	{
		msg.WriteLong( handle );
	}

	void ReadFromMsg( const idBitMsg& msg )
	{
		handle = msg.ReadLong();
	}

	void Serialize( idSerializer& ser );

private:
	userHandleType_t	handle;
};

/*
================================================
idLocalUser
An idLocalUser is a user holding a controller.
It represents someone controlling the menu or game.
They may not necessarily be in a game (which would be a session user of TYPE_GAME).
A controller user references an input device (which is a gamepad, keyboard, etc).
================================================
*/
class idLocalUser
{
public:
	idLocalUser();
	virtual						~idLocalUser() {}

	void				Pump();
	virtual void				PumpPlatform() = 0;

	virtual bool				IsPersistent() const
	{
		return IsProfileReady();    // True if this user is a persistent user, and can save stats, etc (signed in)
	}
	virtual bool				IsProfileReady() const = 0;							// True if IsPersistent is true AND profile is signed into LIVE service
	virtual bool				IsOnline() const = 0;								// True if this user has online capabilities
	virtual uint32				GetOnlineCaps() const = 0;							// Returns combination of onlineCaps_t flags
	virtual bool				HasOwnerChanged() const
	{
		return false;    // Whether or not the original persistent owner has changed since it was first registered
	}
	virtual int					GetInputDevice() const = 0;							// Input device of controller
	virtual const char* 		GetGamerTag() const = 0;							// Gamertag of user
	virtual bool				IsInParty() const = 0;								// True if the user is in a party (do we support this on pc and ps3? )
	virtual int					GetPartyCount() const = 0;							// Gets the amount of users in the party

	// Storage related
	virtual bool				IsStorageDeviceAvailable() const;					// Only false if the player has chosen to play without a storage device, only possible on 360, if available, everything needs to check for available space
	virtual void				ResetStorageDevice();

	// RB: disabled savegame and profile storage checks, because it fails sometimes without any clear reason
	//virtual bool				StorageSizeAvailable( uint64 minSizeInBytes, int64& neededBytes );
	// RB end

	// These set stats within the profile as a enum/value pair
	virtual void				SetStatInt( int stat, int value );
	virtual void				SetStatFloat( int stat, float value );
	virtual int					GetStatInt( int stat );
	virtual float				GetStatFloat( int stat );

	virtual idPlayerProfile* 	GetProfile()
	{
		return GetProfileMgr().GetProfile();
	}
	const idPlayerProfile* 		GetProfile() const
	{
		return const_cast< idLocalUser* >( this )->GetProfile();
	}

	idProfileMgr& 				GetProfileMgr()
	{
		return profileMgr;
	}

	// Helper state to determine if the user is joining a party lobby or not
	void						SetJoiningLobby( int lobbyType, bool value )
	{
		joiningLobby[lobbyType] = value;
	}
	bool						IsJoiningLobby( int lobbyType ) const
	{
		return joiningLobby[lobbyType];
	}

	bool						CanPlayOnline() const
	{
		return ( GetOnlineCaps() & CAP_CAN_PLAY_ONLINE ) > 0;
	}

	localUserHandle_t			GetLocalUserHandle() const
	{
		return localUserHandle;
	}
	void						SetLocalUserHandle( localUserHandle_t newHandle )
	{
		localUserHandle = newHandle;
	}

	// Creates a new profile if one not already there
	void						LoadProfileSettings();
	void						SaveProfileSettings();

	// Will attempt to sync the achievement bits between the server and the localUser when the achievement system is ready
	void						RequestSyncAchievements()
	{
		syncAchievementsRequested = true;
	}

private:
	bool						joiningLobby[2];
	localUserHandle_t			localUserHandle;
	idProfileMgr				profileMgr;

	bool						syncAchievementsRequested;
};

#endif // __SYS_LOCALUSER_H__
