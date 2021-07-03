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
#ifndef __SYS_ACHIEVEMENTS_H__
#define __SYS_ACHIEVEMENTS_H__

class idLocalUser;

// data structure for online achievement entry descriptions
// this is used for testing purposes to make sure that the consoles
// achievement settings match the game's decls
struct achievementDescription_t
{
	void	Clear()
	{
		name[0] = '\0';
		description[0] = '\0';
		hidden = false;
	};
	char	name[500];
	char	description[1000];
	bool	hidden;
};

/*
================================================
idAchievementSystem
================================================
*/
class idAchievementSystem
{
public:
	static const int MAX_ACHIEVEMENTS = 128;		// This matches the max number of achievements bits in the profile

	virtual			~idAchievementSystem() {}

	// PC and PS3 initialize for the system, not for a particular controller
	virtual void	Init() {}

	// PS3 has to wait to install the .TRP file until *after* we determine there's enough space, for consistent user messaging
	virtual void	Start() {}

	// Do any necessary cleanup
	virtual void	Shutdown() {}

	// Is the achievement system ready for requests
	virtual bool	IsInitialized()
	{
		return false;
	}

	// Add a local user to the system
	virtual void	RegisterLocalUser( idLocalUser* user ) {}

	// This is only necessary on the 360 right now, we need this because the 360 maintains a buffer of pending actions
	// per user.  If a user is removed from the system, we need to inform the system so it can cancel it's in flight actions
	// and allow the buffers to be reused
	virtual void	RemoveLocalUser( idLocalUser* user ) {}

	// Unlocks the achievement, all platforms silently fail if the achievement has already been unlocked
	virtual void	AchievementUnlock( idLocalUser* user, const int achievementID ) = 0;

	// Puts the achievement back to its original state, platform implementation may not allow this
	virtual void	AchievementLock( idLocalUser* user, const int achievementID ) {}

	// Puts alls achievements back to their original state, platform implementation may not allow this
	virtual void	AchievementLockAll( idLocalUser* user, const int maxId ) {}

	// Should be done every frame
	virtual void	Pump() = 0;

	// Cancels all in-flight achievements for all users if NULL, resets the system so a Init() must be re-issued
	virtual void	Reset( idLocalUser* user = NULL ) {}

	// Cancels all in-flight achievements, not very useful on PC
	virtual void	Cancel( idLocalUser* user ) {}

	// Retrieves textual information about a given achievement
	// returns false if there was an error
	virtual bool	GetAchievementDescription( idLocalUser* user, const int id, achievementDescription_t& data ) const
	{
		return false;
	}

	// How much storage is required
	// returns false if there was an error
	virtual bool	GetRequiredStorage( uint64& requiredSizeTrophiesBytes )
	{
		requiredSizeTrophiesBytes = 0;
		return true;
	}

	// Retrieves state about of all achievements cached locally (may not be online yet)
	// returns false if there was an error
	virtual bool	GetAchievementState( idLocalUser* user, idArray< bool, idAchievementSystem::MAX_ACHIEVEMENTS >& achievements ) const
	{
		return false;
	}

	// Sets state of all the achievements within list (for debug purposes only)
	// returns false if there was an error
	virtual bool	SetAchievementState( idLocalUser* user, idArray< bool, idAchievementSystem::MAX_ACHIEVEMENTS >& achievements )
	{
		return false;
	}

	// You want to get the server's cached achievement status into the user because the profile may not have been
	// saved with the achievement bits after an achievement was granted.
	void			SyncAchievementBits( idLocalUser* user );

protected:
	// Retrieves the index from the local user list
	int				GetLocalUserIndex( idLocalUser* user ) const
	{
		return users.FindIndex( user );
	}

	idStaticList< idLocalUser*, MAX_LOCAL_PLAYERS > users;
};

#endif // __SYS_ACHIEVEMENTS_H__
