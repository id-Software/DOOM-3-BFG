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

#include "Precompiled.h"
#include "Serializer.h"
//#include "SaveGameManager.h"

#define	MAX_PROFILE_SIZE			( 1024 * 1000 ) // High number for the key bindings

#define SAVEGAME_PROFILE_FILENAME			"_PROF"


extern idCVar s_volume_sound;
extern idCVar s_volume_midi;


class idSaveGameProcessorSaveProfile;
class idSaveGameProcessorLoadProfile;
class idPlayerProfile;

/*
================================================
idProfileMgr 
================================================
*/
class idProfileMgr {
public:
	idProfileMgr();
	~idProfileMgr();

	// Called the first time it's asked to load
	void				Init( idPlayerProfile * profile );

	void 				Pump();
	idPlayerProfile *	GetProfile();

private:
	void				LoadSettings();
	void				SaveSettings();

private:
	idSaveGameProcessorSaveProfile *	profileSaveProcessor;
	idSaveGameProcessorLoadProfile *	profileLoadProcessor;
	idPlayerProfile *					profile;
	saveGameHandle_t					handle;
};

/*
================================================
idSaveGameProcessorSaveProfile
================================================
*/
class idSaveGameProcessorSaveProfile : public idSaveGameProcessor {
public:
	DEFINE_CLASS( idSaveGameProcessorSaveProfile );

	idSaveGameProcessorSaveProfile();

	bool			InitSaveProfile( idPlayerProfile * profile, const char * folder );
	virtual bool	Process();

private:
	idFile_Memory *		profileFile;
	idFile_Memory *		staticScreenshotFile;
	idPlayerProfile *	profile;
};

/*
================================================
idSaveGameProcessorLoadProfile
================================================
*/
class idSaveGameProcessorLoadProfile: public idSaveGameProcessor {
public:
	DEFINE_CLASS( idSaveGameProcessorLoadProfile );

	idSaveGameProcessorLoadProfile();
	~idSaveGameProcessorLoadProfile();

	bool			InitLoadProfile( idPlayerProfile * profile, const char * folder );
	virtual bool	Process();
	virtual void	PostProcess();

private:
	idFile_Memory *		profileFile;
	idPlayerProfile *	profile;
};

/*
================================================
profileStatValue_t 
================================================
*/
union profileStatValue_t {
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
class idPlayerProfile {
public: 
	static const int MAX_PLAYER_PROFILE_STATS = 500;

	enum state_t {
		IDLE = 0,
		SAVING,
		LOADING,
		SAVE_REQUESTED,
		LOAD_REQUESTED,
		ERR
	};

	enum displayMode_t {
		DISPLAY_INVALID = -1,
		DISPLAY_WINDOWED,
		DISPLAY_FULLSCREEN,
		MAX_DISPLAY_MODES
	};

	enum syncTypes_t {
		SYNC_INVALID = -1,
		SYNC_TEAR,
		SYNC_ON,
		SYNC_SMART,
		MAX_SYNC_COUNT,
	};

public:
					idPlayerProfile(); // don't instantiate. we static_cast the child all over the place
	virtual			~idPlayerProfile();

	//------------------------
	// each game can override but call the parent serialize first
	//------------------------
	virtual void	SetDefaults();
	virtual void	Init();
	virtual bool	SerializeSettings( idSerializer & ser );

	//------------------------
	// each game must override, not an abstract method because we have a static object as a hack... ugh.
	//------------------------
	virtual int32	GetProfileTag() { return -1; }

	int				GetDeviceNumForProfile() { return deviceNum; }
	void			SetDeviceNumForProfile( int num ) { deviceNum = num; }

	//------------------------
	void			SaveSettings();
	void			LoadSettings();

	state_t			GetState() const { return state; }
	state_t			GetRequestedState() const { return requestedState; }
	
	//------------------------
	// settings
	//------------------------
	float			GetFrameScaleX() const { return frameScaleX; }
	float			GetFrameScaleY() const { return frameScaleY; }
	void			SetFrameScaleX( float scale ) { frameScaleX = scale; }
	void			SetFrameScaleY( float scale ) { frameScaleY = scale; } 

	int				GetMusicVolume() const;
	int				GetSoundVolume() const;
	void			SetMusicVolume( int volume );
	void			SetSoundVolume( int volume );

	bool			GetAlwaysRun() const { return alwaysRun; }
	void			SetAlwaysRun( bool set ) { alwaysRun = set; }

	//------------------------
	// misc
	//------------------------
	virtual int		GetLevel() const;

	void			ClearAchievementBit( const int id );		// Should only be called by idLocalUser
	bool			GetAchievementBit( const int id ) const;
	void			SetAchievementBit( const int id );			// Should only be called by idLocalUser

	bool			GetSeenInstallMessage() const { return seenInstallMessage; }
	void			SetSeenInstallMessage( bool seen ) { seenInstallMessage = seen; }

	bool			HasSavedGame() const { return hasSavedGame; }
	void			SetHasSavedGame() { hasSavedGame = true; }

protected:
	friend class idLocalUser;
	friend class idProfileMgr;

	// used by idLocalUser and internally
	void			StatSetInt( int s, int v );
	void			StatSetFloat( int s, float v );
	int				StatGetInt( int s ) const;
	float			StatGetFloat( int s ) const;

private:
	void			SetState( state_t value ) { state = value; }
	void			SetRequestedState( state_t value ) { requestedState = value; }

protected:
	//------------------------
	// settings
	//------------------------
	bool				alwaysRun;
	int					musicVolume;
	int					soundVolume;

	//------------------------
	// video settings
	//------------------------
	float				frameScaleX;
	float				frameScaleY;

	//------------------------
	// state management
	//------------------------
	state_t				state;
	state_t				requestedState;

	//------------------------
	// stats are stored in the profile
	//------------------------
	idStaticList< profileStatValue_t, MAX_PLAYER_PROFILE_STATS > stats;
	
	//------------------------
	// misc
	//------------------------
	int					deviceNum;
	bool				seenInstallMessage;
	uint64				achievementBits;
	bool				hasSavedGame;
};

#endif
