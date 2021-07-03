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
#ifndef __SYS_PROFILE_H__
#define __SYS_PROFILE_H__

#include "sys_savegame.h"
#include "sys_session_savegames.h"


class idSaveGameProcessorSaveProfile;
class idSaveGameProcessorLoadProfile;
class idLocalUser;
class idPlayerProfile;

/*
================================================
idProfileMgr
================================================
*/
class idProfileMgr
{
public:
	idProfileMgr();
	~idProfileMgr();

	// Not copyable because we use unique_ptrs.
	idProfileMgr& operator=( const idProfileMgr& ) = delete;

	// Called the first time it's asked to load
	void				Init( idLocalUser* user );

	void 				Pump();
	idPlayerProfile* 	GetProfile();

private:
	void				LoadSettingsAsync();
	void				SaveSettingsAsync();

	void				OnLoadSettingsCompleted( idSaveLoadParms* parms );
	void				OnSaveSettingsCompleted( idSaveLoadParms* parms );

private:
	std::unique_ptr< idSaveGameProcessorSaveProfile >	profileSaveProcessor;
	std::unique_ptr< idSaveGameProcessorLoadProfile >	profileLoadProcessor;

	idLocalUser* 						user;					// reference passed in
	idPlayerProfile* 					profile;
	saveGameHandle_t					handle;
};

/*
================================================
idSaveGameProcessorSaveProfile
================================================
*/
class idSaveGameProcessorSaveProfile : public idSaveGameProcessorSaveFiles
{
public:
	DEFINE_CLASS( idSaveGameProcessorSaveProfile );

	idSaveGameProcessorSaveProfile();

	bool			InitSaveProfile( idPlayerProfile* profile, const char* folder );
	virtual bool	Process();

private:
	idFile_SaveGame* 	profileFile;
	idPlayerProfile* 	profile;

};

/*
================================================
idSaveGameProcessorLoadProfile
================================================
*/
class idSaveGameProcessorLoadProfile : public idSaveGameProcessorLoadFiles
{
public:
	DEFINE_CLASS( idSaveGameProcessorLoadProfile );

	idSaveGameProcessorLoadProfile();
	~idSaveGameProcessorLoadProfile();

	bool			InitLoadProfile( idPlayerProfile* profile, const char* folder );
	virtual bool	Process();


private:
	idFile_SaveGame* 	profileFile;
	idPlayerProfile* 	profile;

};

// Synchronous check, just checks if a profile exists within the savegame location
bool Sys_SaveGameProfileCheck();

#endif
