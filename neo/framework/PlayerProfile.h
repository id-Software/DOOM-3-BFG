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
#ifndef __PLAYERPROFILE_H__
#define __PLAYERPROFILE_H__

#define	MAX_PROFILE_SIZE			( 1024 * 1000 ) // High number for the key bindings

/*
================================================
profileStatValue_t
================================================
*/
union profileStatValue_t
{
	int		i;
	float	f;
};

/*
================================================
idPlayerProfile

The general rule for using cvars for settings is that if you want the player's profile settings to affect the startup
of the game before there is a player associated with the game, use cvars.  Example: video & volume settings.
================================================
*/
class idPlayerProfile
{
	friend class idLocalUser;
	friend class idProfileMgr;
	
public:
	// Only have room to squeeze ~450 in doom3 right now
	static const int MAX_PLAYER_PROFILE_STATS = 200;
	
	enum state_t
	{
		IDLE = 0,
		SAVING,
		LOADING,
		SAVE_REQUESTED,
		LOAD_REQUESTED,
		ERR
	};
protected:
	idPlayerProfile(); // don't instantiate. we static_cast the child all over the place
public:

	virtual			~idPlayerProfile();
	
	static idPlayerProfile* CreatePlayerProfile( int deviceIndex );
	
	void			SetDefaults();
	bool			Serialize( idSerializer& ser );
	
	const int		GetDeviceNumForProfile() const
	{
		return deviceNum;
	}
	void			SetDeviceNumForProfile( int num )
	{
		deviceNum = num;
	}
	void			SaveSettings( bool forceDirty );
	void			LoadSettings();
	state_t			GetState() const
	{
		return state;
	}
	state_t			GetRequestedState() const
	{
		return requestedState;
	}
	bool			IsDirty()
	{
		return dirty;
	}
	
	bool			GetAchievement( const int id ) const;
	void			SetAchievement( const int id );
	void			ClearAchievement( const int id );
	
	int				GetDlcReleaseVersion() const
	{
		return dlcReleaseVersion;
	}
	void			SetDlcReleaseVersion( int version )
	{
		dlcReleaseVersion = version;
	}
	
	int				GetLevel() const
	{
		return 0;
	}
	
	//------------------------
	// Config
	//------------------------
	int				GetConfig() const
	{
		return configSet;
	}
	void			SetConfig( int config, bool save );
	void			RestoreDefault();
	
	void			SetLeftyFlip( bool lf );
	bool			GetLeftyFlip() const
	{
		return leftyFlip;
	}
	
private:
	void			StatSetInt( int s, int v );
	void			StatSetFloat( int s, float v );
	int				StatGetInt( int s ) const;
	float			StatGetFloat( int s ) const;
	void			SetState( state_t value )
	{
		state = value;
	}
	void			SetRequestedState( state_t value )
	{
		requestedState = value;
	}
	void			MarkDirty( bool isDirty )
	{
		dirty = isDirty;
	}
	
	void			ExecConfig( bool save = false, bool forceDefault = false );
	
protected:
	// Do not save:
	state_t			state;
	state_t			requestedState;
	int				deviceNum;
	
	// Save:
	uint64			achievementBits;
	uint64			achievementBits2;
	int				dlcReleaseVersion;
	int				configSet;
	bool			customConfig;
	bool			leftyFlip;
	
	bool			dirty;		// dirty bit to indicate whether or not we need to save
	
	idStaticList< profileStatValue_t, MAX_PLAYER_PROFILE_STATS > stats;
};

#endif
