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
#ifndef __SYS_SESSION_SAVEGAMES_H__
#define __SYS_SESSION_SAVEGAMES_H__

/*
================================================
idSaveGameProcessorLoadFiles
================================================
*/
class idSaveGameProcessorLoadFiles : public idSaveGameProcessor
{
public:
	DEFINE_CLASS( idSaveGameProcessorLoadFiles );

	virtual bool	InitLoadFiles( const char* folder,
								   const saveFileEntryList_t& files,
								   idSaveGameManager::packageType_t type = idSaveGameManager::PACKAGE_GAME );
	virtual bool	Process();
};

/*
================================================win_sav
idSaveGameProcessorDelete
================================================
*/
class idSaveGameProcessorDelete : public idSaveGameProcessor
{
public:
	DEFINE_CLASS( idSaveGameProcessorDelete );

	bool			InitDelete( const char* folder, idSaveGameManager::packageType_t type = idSaveGameManager::PACKAGE_GAME );
	virtual bool	Process();
};

/*
================================================
idSaveGameProcessorSaveFiles
================================================
*/
class idSaveGameProcessorSaveFiles : public idSaveGameProcessor
{
public:
	DEFINE_CLASS( idSaveGameProcessorSaveFiles );

	// Passing in idSaveGameDetails so that we have a copy on output
	bool			InitSave( const char* folder,
							  const saveFileEntryList_t& files,
							  const idSaveGameDetails& description,
							  idSaveGameManager::packageType_t type = idSaveGameManager::PACKAGE_GAME );
	virtual bool	Process();
};

/*
================================================
idSaveGameProcessorEnumerateGames
================================================
*/
class idSaveGameProcessorEnumerateGames : public idSaveGameProcessor
{
public:
	DEFINE_CLASS( idSaveGameProcessorEnumerateGames );

	virtual bool	Process();
};

#endif
