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

#define SAVEGAME_PROFILE_FILENAME			"profile.bin"

idCVar profile_verbose( "profile_verbose", "0", CVAR_BOOL, "Turns on debug spam for profiles" );

/*
================================================
idProfileMgr
================================================
*/

/*
========================
idProfileMgr
========================
*/
idProfileMgr::idProfileMgr() :
	profileSaveProcessor( new( TAG_SAVEGAMES ) idSaveGameProcessorSaveProfile ),
	profileLoadProcessor( new( TAG_SAVEGAMES ) idSaveGameProcessorLoadProfile ),
	profile( NULL ),
	handle( 0 )
{
}


/*
================================================
~idProfileMgr
================================================
*/
idProfileMgr::~idProfileMgr()
{
}

/*
========================
idProfileMgr::Init
========================
*/
void idProfileMgr::Init( idLocalUser* user_ )
{
	user = user_;
	handle = 0;
}

/*
========================
idProfileMgr::Pump
========================
*/
void idProfileMgr::Pump()
{
	// profile can be NULL if we forced the user to register as in the case of map-ing into a level from the press start screen
	if( profile == NULL )
	{
		return;
	}

	// See if we are done with saving/loading the profile
	bool saving = profile->GetState() == idPlayerProfile::SAVING;
	bool loading = profile->GetState() == idPlayerProfile::LOADING;
	if( ( saving || loading ) && session->IsSaveGameCompletedFromHandle( handle ) )
	{
		profile->SetState( idPlayerProfile::IDLE );

		if( saving )
		{
			// Done saving
		}
		else if( loading )
		{
			// Done loading
			const idSaveLoadParms& parms = profileLoadProcessor->GetParms();
			if( parms.GetError() == SAVEGAME_E_FOLDER_NOT_FOUND || parms.GetError() == SAVEGAME_E_FILE_NOT_FOUND )
			{
				profile->SaveSettings( true );
			}
			else if( parms.GetError() == SAVEGAME_E_CORRUPTED )
			{
				idLib::Warning( "Profile corrupt, creating a new one..." );
				common->Dialog().AddDialog( GDM_CORRUPT_PROFILE, DIALOG_CONTINUE, NULL, NULL, false );
				profile->SetDefaults();
				profile->SaveSettings( true );
			}
			else if( parms.GetError() != SAVEGAME_E_NONE )
			{
				profile->SetState( idPlayerProfile::ERR );
			}

			session->OnLocalUserProfileLoaded( user );
		}
	}
	else if( saving || loading )
	{
		return;
	}

	// See if we need to save/load the profile
	if( profile->GetRequestedState() == idPlayerProfile::SAVE_REQUESTED && profile->IsDirty() )
	{
		profile->MarkDirty( false );
		SaveSettingsAsync();
		// Syncs the steam data
		//session->StoreStats();
		profile->SetRequestedState( idPlayerProfile::IDLE );
	}
	else if( profile->GetRequestedState() == idPlayerProfile::LOAD_REQUESTED )
	{
		LoadSettingsAsync();
		profile->SetRequestedState( idPlayerProfile::IDLE );
	}
}

/*
========================
idProfileMgr::GetProfile
========================
*/
idPlayerProfile* idProfileMgr::GetProfile()
{
	assert( user != NULL );
	if( profile == NULL )
	{
		// Lazy instantiation
		// Create a new profile
		profile = idPlayerProfile::CreatePlayerProfile( user->GetInputDevice() );
		if( profile == NULL )
		{
			return NULL;
		}
	}

	bool loading = ( profile->GetState() == idPlayerProfile::LOADING ) || ( profile->GetRequestedState() == idPlayerProfile::LOAD_REQUESTED );
	if( loading )
	{
		return NULL;
	}

	return profile;
}

/*
========================
idProfileMgr::SaveSettingsAsync
========================
*/
void idProfileMgr::SaveSettingsAsync()
{
	if( !saveGame_enable.GetBool() )
	{
		idLib::Warning( "Skipping profile save because saveGame_enable = 0" );
	}

	if( GetProfile() != NULL )
	{
		// Issue the async save...
		if( profileSaveProcessor->InitSaveProfile( profile, "" ) )
		{


			profileSaveProcessor->AddCompletedCallback( MakeCallback( this, &idProfileMgr::OnSaveSettingsCompleted, &profileSaveProcessor->GetParmsNonConst() ) );
			handle = session->GetSaveGameManager().ExecuteProcessor( profileSaveProcessor.get() );
			profile->SetState( idPlayerProfile::SAVING );

		}
	}
	else
	{
		idLib::Warning( "Not saving profile, profile is NULL." );
	}
}

/*
========================
idProfileMgr::LoadSettingsAsync
========================
*/
void idProfileMgr::LoadSettingsAsync()
{
	if( profile != NULL && saveGame_enable.GetBool() )
	{
		if( profileLoadProcessor->InitLoadProfile( profile, "" ) )
		{
			// Skip the not found error because this might be the first time to play the game!
			profileLoadProcessor->SetSkipSystemErrorDialogMask( SAVEGAME_E_FOLDER_NOT_FOUND | SAVEGAME_E_FILE_NOT_FOUND );

			profileLoadProcessor->AddCompletedCallback( MakeCallback( this, &idProfileMgr::OnLoadSettingsCompleted, &profileLoadProcessor->GetParmsNonConst() ) );
			handle = session->GetSaveGameManager().ExecuteProcessor( profileLoadProcessor.get() );
			profile->SetState( idPlayerProfile::LOADING );


		}
	}
	else
	{
		// If not able to save the profile, just change the state and leave
		if( profile == NULL )
		{
			idLib::Warning( "Not loading profile, profile is NULL." );
		}
		if( !saveGame_enable.GetBool() )
		{
			idLib::Warning( "Skipping profile load because saveGame_enable = 0" );
		}
	}
}

/*
========================
idProfileMgr::OnLoadSettingsCompleted
========================
*/
void idProfileMgr::OnLoadSettingsCompleted( idSaveLoadParms* parms )
{





	// Don't process if error already detected
	if( parms->errorCode != SAVEGAME_E_NONE )
	{
		return;
	}

	// Serialize the loaded profile
	idFile_SaveGame** profileFileContainer = FindFromGenericPtr( parms->files, SAVEGAME_PROFILE_FILENAME );
	idFile_SaveGame* profileFile = profileFileContainer == NULL ? NULL : *profileFileContainer;

	bool foundProfile = profileFile != NULL && profileFile->Length() > 0;

	if( foundProfile )
	{
		idTempArray< byte > buffer( MAX_PROFILE_SIZE );

		// Serialize settings from this buffer
		profileFile->MakeReadOnly();
		unsigned int originalChecksum;
		profileFile->ReadBig( originalChecksum );

		int dataLength = profileFile->Length() - ( int )sizeof( originalChecksum );
		profileFile->ReadBigArray( buffer.Ptr(), dataLength );

		// Validate the checksum before we let the game serialize the settings
		unsigned int checksum = MD5_BlockChecksum( buffer.Ptr(), dataLength );
		if( originalChecksum != checksum )
		{
			idLib::Warning( "Checksum: 0x%08x, originalChecksum: 0x%08x, size = %d", checksum, originalChecksum, dataLength );
			parms->errorCode = SAVEGAME_E_CORRUPTED;
		}
		else
		{
			idBitMsg msg;
			msg.InitRead( buffer.Ptr(), ( int )buffer.Size() );
			idSerializer ser( msg, false );
			if( !profile->Serialize( ser ) )
			{
				parms->errorCode = SAVEGAME_E_CORRUPTED;
			}
		}

	}
	else
	{
		parms->errorCode = SAVEGAME_E_FILE_NOT_FOUND;
	}
}

/*
========================
idProfileMgr::OnSaveSettingsCompleted
========================
*/
void idProfileMgr::OnSaveSettingsCompleted( idSaveLoadParms* parms )
{
	common->Dialog().ShowSaveIndicator( false );

	if( parms->GetError() != SAVEGAME_E_NONE )
	{
		common->Dialog().AddDialog( GDM_PROFILE_SAVE_ERROR, DIALOG_CONTINUE, NULL, NULL, false );
	}
	if( game )
	{
		game->Shell_UpdateSavedGames();
	}
}

/*
================================================
idSaveGameProcessorSaveProfile
================================================
*/

/*
========================
idSaveGameProcessorSaveProfile::idSaveGameProcessorSaveProfile
========================
*/
idSaveGameProcessorSaveProfile::idSaveGameProcessorSaveProfile()
{
	profileFile = NULL;
	profile = NULL;

}

/*
========================
idSaveGameProcessorSaveProfile::InitSaveProfile
========================
*/
bool idSaveGameProcessorSaveProfile::InitSaveProfile( idPlayerProfile* profile_, const char* folder )
{
	// Serialize the profile and pass a file to the processor
	profileFile = new( TAG_SAVEGAMES ) idFile_SaveGame( SAVEGAME_PROFILE_FILENAME, SAVEGAMEFILE_BINARY | SAVEGAMEFILE_AUTO_DELETE );
	profileFile->MakeWritable();
	profileFile->SetMaxLength( MAX_PROFILE_SIZE );

	// Create a serialization object and let the game serialize the settings into the buffer
	const int serializeSize = MAX_PROFILE_SIZE - 8;	// -8 for checksum (all platforms) and length (on 360)
	idTempArray< byte > buffer( serializeSize );
	idBitMsg msg;
	msg.InitWrite( buffer.Ptr(), serializeSize );
	idSerializer ser( msg, true );
	profile_->Serialize( ser );

	// Get and write the checksum & length first
	unsigned int checksum = MD5_BlockChecksum( msg.GetReadData(), msg.GetSize() );
	profileFile->WriteBig( checksum );

	idLib::PrintfIf( profile_verbose.GetBool(), "checksum: 0x%08x, length: %d\n", checksum, msg.GetSize() );

	// Add data to the file and prepare for save
	profileFile->Write( msg.GetReadData(), msg.GetSize() );
	profileFile->MakeReadOnly();

	saveFileEntryList_t files;
	files.Append( profileFile );

	idSaveGameDetails description;
	if( !idSaveGameProcessorSaveFiles::InitSave( folder, files, description, idSaveGameManager::PACKAGE_PROFILE ) )
	{
		return false;
	}


	profile = profile_;


	return true;
}

/*
========================
idSaveGameProcessorSaveProfile::Process
========================
*/
bool idSaveGameProcessorSaveProfile::Process()
{


	// Files already setup for save, just execute as normal files
	return idSaveGameProcessorSaveFiles::Process();

}


/*
================================================
idSaveGameProcessorLoadProfile
================================================
*/

/*
========================
idSaveGameProcessorLoadProfile::idSaveGameProcessorLoadProfile
========================
*/
idSaveGameProcessorLoadProfile::idSaveGameProcessorLoadProfile()
{
	profileFile = NULL;
	profile = NULL;

}

/*
========================
idSaveGameProcessorLoadProfile::~idSaveGameProcessorLoadProfile
========================
*/
idSaveGameProcessorLoadProfile::~idSaveGameProcessorLoadProfile()
{
}

/*
========================
idSaveGameProcessorLoadProfile::InitLoadFiles
========================
*/
bool idSaveGameProcessorLoadProfile::InitLoadProfile( idPlayerProfile* profile_, const char* folder_ )
{
	if( !idSaveGameProcessor::Init() )
	{
		return false;
	}

	parms.directory = AddSaveFolderPrefix( folder_, idSaveGameManager::PACKAGE_PROFILE );
	parms.description.slotName = folder_;
	parms.mode = SAVEGAME_MBF_LOAD;

	profileFile = new( TAG_SAVEGAMES ) idFile_SaveGame( SAVEGAME_PROFILE_FILENAME, SAVEGAMEFILE_BINARY | SAVEGAMEFILE_AUTO_DELETE );
	parms.files.Append( profileFile );

	profile = profile_;


	return true;
}

/*
========================
idSaveGameProcessorLoadProfile::Process
========================
*/
bool idSaveGameProcessorLoadProfile::Process()
{


	return idSaveGameProcessorLoadFiles::Process();

}


/*
========================
Sys_SaveGameProfileCheck
========================
*/
bool Sys_SaveGameProfileCheck()
{
	bool exists = false;
	const char* saveFolder = "savegame";

	if( fileSystem->IsFolder( saveFolder, "fs_savePath" ) == FOLDER_YES )
	{
		idFileList* files = fileSystem->ListFiles( saveFolder, SAVEGAME_PROFILE_FILENAME );
		const idStrList& fileList = files->GetList();

		for( int i = 0; i < fileList.Num(); i++ )
		{
			idStr filename = fileList[i];
			if( filename == SAVEGAME_PROFILE_FILENAME )
			{
				exists = true;
				break;
			}
		}

		fileSystem->FreeFileList( files );
	}

	return exists;
}
