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
#include "PlayerProfile.h"
#include "PS3_Includes.h"
#include "PSN/PS3_Session.h"

const int32		FRAMEWORK_PROFILE_VER		= 1;


// Store master volume settings in archived cvars, becausue we want them to apply
// even if a user isn't signed in.
// The range is from 0 to 15, which matches the setting in vanilla DOOM.
idCVar s_volume_sound( "s_volume_sound", "8", CVAR_ARCHIVE | CVAR_INTEGER, "sound volume", 0, 15 );
idCVar s_volume_midi( "s_volume_midi", "8", CVAR_ARCHIVE | CVAR_INTEGER, "music volume", 0, 15 );



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
	profileSaveProcessor( new (TAG_SAVEGAMES) idSaveGameProcessorSaveProfile ),
	profileLoadProcessor( new (TAG_SAVEGAMES) idSaveGameProcessorLoadProfile ),
	profile( NULL ),
	handle( 0 ) {
}


/*
================================================
~idProfileMgr
================================================
*/
idProfileMgr::~idProfileMgr() {
	delete profileSaveProcessor;
	profileSaveProcessor = NULL;

	delete profileLoadProcessor;
	profileLoadProcessor = NULL;
}

/*
========================
idProfileMgr::Init
========================
*/
void idProfileMgr::Init( idPlayerProfile * profile_ ) {
	profile = profile_;
	handle = 0;
}

/*
========================
idProfileMgr::Pump
========================
*/
void idProfileMgr::Pump() {
	// profile can be NULL if we forced the user to register as in the case of map-ing into a level from the press start screen
	if ( profile == NULL ) {
		return;
	}

	// See if we are done with saving/loading the profile
	bool saving = profile->GetState() == idPlayerProfile::SAVING;
	bool loading = profile->GetState() == idPlayerProfile::LOADING;
	if ( ( saving || loading ) && psn_session->GetSaveGameManager()->IsSaveGameCompletedFromHandle( handle ) ) {
		profile->SetState( idPlayerProfile::IDLE );

		if ( saving ) {
			// Done saving
		} else if ( loading ) {
			// Done loading
			const idSaveLoadParms & parms = profileLoadProcessor->GetParms();
			if ( parms.GetError() == SAVEGAME_E_FOLDER_NOT_FOUND || parms.GetError() == SAVEGAME_E_FILE_NOT_FOUND ) {
				profile->SaveSettings();
			} else if ( parms.GetError() != SAVEGAME_E_NONE ) {
				profile->SetState( idPlayerProfile::ERR );
			}
		}
	}

	// See if we need to save/load the profile
	if ( profile->GetRequestedState() == idPlayerProfile::SAVE_REQUESTED ) {
		SaveSettings();
		profile->SetRequestedState( idPlayerProfile::IDLE );
	} else if ( profile->GetRequestedState() == idPlayerProfile::LOAD_REQUESTED ) {
		LoadSettings();
		profile->SetRequestedState( idPlayerProfile::IDLE );
	}
}

/*
========================
idProfileMgr::GetProfile
========================
*/
idPlayerProfile * idProfileMgr::GetProfile() { 
	if ( profile == NULL ) {
		return NULL;
	}

	bool loading = ( profile->GetState() == idPlayerProfile::LOADING ) || ( profile->GetRequestedState() == idPlayerProfile::LOAD_REQUESTED );
	if ( loading ) {
		return NULL;
	}

	return profile;
}

/*
========================
idProfileMgr::SaveSettings
========================
*/
void idProfileMgr::SaveSettings() {
	if ( profile != NULL && saveGame_enable.GetBool() ) {
		// Issue the async save...
		if ( profileSaveProcessor->InitSaveProfile( profile, "" ) ) {
			handle = psn_session->GetSaveGameManager()->ExecuteProcessor( profileSaveProcessor );
			profile->SetState( idPlayerProfile::SAVING );
		}
	} else {
		// If not able to save the profile, just change the state and leave
		if ( profile == NULL ) {
			idLib::Warning( "Not saving profile, profile is NULL." );
		}
		if ( !saveGame_enable.GetBool() ) {
			idLib::Warning( "Skipping profile save because saveGame_enable = 0" );
		}
	}
}

/*
========================
idProfileMgr::LoadSettings
========================
*/
void idProfileMgr::LoadSettings() {
	if ( profile != NULL && saveGame_enable.GetBool() ) {
		if ( profileLoadProcessor->InitLoadProfile( profile, "" ) ) {
			// Skip the not found error because this might be the first time to play the game!
			profileLoadProcessor->SetSkipSystemErrorDialogMask( SAVEGAME_E_FOLDER_NOT_FOUND | SAVEGAME_E_FILE_NOT_FOUND );

			handle = psn_session->GetSaveGameManager()->ExecuteProcessor( profileLoadProcessor );
			profile->SetState( idPlayerProfile::LOADING );
		}
	} else {
		// If not able to save the profile, just change the state and leave
		if ( profile == NULL ) {
			idLib::Warning( "Not loading profile, profile is NULL." );
		}
		if ( !saveGame_enable.GetBool() ) {
			idLib::Warning( "Skipping profile load because saveGame_enable = 0" );
		}
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
idSaveGameProcessorSaveProfile::idSaveGameProcessorSaveProfile() {
	profileFile = NULL;
	profile = NULL;
}

/*
========================
idSaveGameProcessorSaveProfile::InitSaveProfile
========================
*/
bool idSaveGameProcessorSaveProfile::InitSaveProfile( idPlayerProfile * profile_, const char * folder ) {

	// Serialize the profile and pass a file to the processor
	profileFile = new (TAG_SAVEGAMES) idFile_Memory( SAVEGAME_PROFILE_FILENAME );
	profileFile->MakeWritable();
	profileFile->SetMaxLength( MAX_PROFILE_SIZE );

	idTempArray< byte > buffer( MAX_PROFILE_SIZE );
	idBitMsg msg;
	msg.InitWrite( buffer.Ptr(), MAX_PROFILE_SIZE );
	idSerializer ser( msg, true );
	profile_->SerializeSettings( ser );

	profileFile->Write( msg.GetReadData(), msg.GetSize() );
	profileFile->MakeReadOnly();

	idList< idSaveFileEntry > files;
	files.Append( idSaveFileEntry( profileFile, SAVEGAMEFILE_BINARY | SAVEGAMEFILE_AUTO_DELETE, SAVEGAME_PROFILE_FILENAME ) );

	idSaveGameDetails description;
	if ( !idSaveGameProcessor::Init() ) {
		return false;
	}

	if ( files.Num() == 0 ) {
		idLib::Warning( "No files to save." );
		return false;
	}

	// Setup save system
	parms.directory = AddSaveFolderPrefix( folder, idSaveGameManager::PACKAGE_PROFILE  );
	parms.mode = SAVEGAME_MBF_SAVE | SAVEGAME_MBF_HIDDEN;	// do NOT delete the existing files
	parms.saveFileType = SAVEFILE_TYPE_AUTO;
	for ( int i = 0; i < files.Num(); ++i ) {
		parms.files.Append( files[i] );
	}


	description.title = idLocalization::GetString( "#str_savegame_title" );
	description.subTitle = idLocalization::GetString( "#str_savegame_profile_heading" );
	description.summary = idLocalization::GetString( "#str_savegame_profile_desc" );


	// Add static image as the thumbnail
	staticScreenshotFile = new (TAG_SAVEGAMES) idFile_Memory( "image" );

	// Open up the Image file and Make it a memory file.
	void* thumbImage = NULL;
	int imagesize = fileSystem->ReadFile( "base/textures/PROFILE.PNG", &thumbImage );		// This file lives at USRData.. i think.
	staticScreenshotFile->MakeWritable();
	staticScreenshotFile->Write( thumbImage, imagesize );
	staticScreenshotFile->MakeReadOnly();

	parms.files.Append( idSaveFileEntry( staticScreenshotFile, SAVEGAMEFILE_THUMB, "image" ) );
	fileSystem->FreeFile( thumbImage );


	this->parms.description = description;
	parms.description.slotName = folder;



	// TODO:KC - what was the purpose of this?
	// JAF idKeyInput::SetUserDeviceNumForBind( profile_->GetDeviceNumForProfile() );

	profile = profile_;

	return true;
}

/*
========================
idSaveGameProcessorSaveProfile::Process
========================
*/
bool idSaveGameProcessorSaveProfile::Process() {
	// Files already setup for save, just execute as normal files

	// Platform-specific implementation
	// This will start a worker thread for async operation.
	// It will always signal when it's completed.
	Sys_ExecuteSavegameCommandAsync( &parms );

	return false;
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
idSaveGameProcessorLoadProfile::idSaveGameProcessorLoadProfile() {
	profileFile = NULL;
	profile = NULL;
}

/*
========================
idSaveGameProcessorLoadProfile::~idSaveGameProcessorLoadProfile
========================
*/
idSaveGameProcessorLoadProfile::~idSaveGameProcessorLoadProfile() {
}

/*
========================
idSaveGameProcessorLoadProfile::InitLoadFiles
========================
*/
bool idSaveGameProcessorLoadProfile::InitLoadProfile( idPlayerProfile * profile_, const char * folder_ ) {
	if ( !idSaveGameProcessor::Init() ) {
		return false;
	}

	parms.directory = AddSaveFolderPrefix( folder_, idSaveGameManager::PACKAGE_PROFILE );
	parms.description.slotName = folder_;
	parms.mode = SAVEGAME_MBF_LOAD | SAVEGAME_MBF_HIDDEN;
	parms.saveFileType = SAVEFILE_TYPE_AUTO;

	profileFile = new (TAG_SAVEGAMES) idFile_Memory( SAVEGAME_PROFILE_FILENAME );
	parms.files.Append( idSaveFileEntry( profileFile, SAVEGAMEFILE_BINARY, SAVEGAME_PROFILE_FILENAME ) );

	profile = profile_;

	return true;
}

/*
========================
idSaveGameProcessorLoadProfile::Process
========================
*/
bool idSaveGameProcessorLoadProfile::Process() {
	Sys_ExecuteSavegameCommandAsync( &parms );

	return false;
}

/*
========================
idSaveGameProcessorLoadProfile::PostProcess
========================
*/
void idSaveGameProcessorLoadProfile::PostProcess() {
	// Serialize the loaded profile
	bool foundProfile = profileFile->Length() > 0;

	if ( foundProfile ) {
		idTempArray< byte> buffer( MAX_PROFILE_SIZE );

		// Serialize settings from this buffer
		profileFile->MakeReadOnly();
		profileFile->ReadBigArray( buffer.Ptr(), profileFile->Length() );

		idBitMsg msg;
		msg.InitRead( buffer.Ptr(), (int)buffer.Size() );
		idSerializer ser( msg, false );
		profile->SerializeSettings( ser );

		// JAF idKeyInput::SetUserDeviceNumForBind( profile->GetDeviceNumForProfile() );

	} else {
		parms.errorCode = SAVEGAME_E_FILE_NOT_FOUND;
	}

	delete profileFile;
}

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
idPlayerProfile::idPlayerProfile
========================
*/
idPlayerProfile::idPlayerProfile() {
	SetDefaults();

	// Don't have these in SetDefaults because they're used for state management and SetDefaults is called when
	// loading the profile
	state				= IDLE;
	requestedState		= IDLE;
}

/*
========================
idPlayerProfile::SetDefaults
========================
*/
void idPlayerProfile::SetDefaults() {
	
	achievementBits		= 0;
	seenInstallMessage  = false;
	stats.SetNum( MAX_PLAYER_PROFILE_STATS );
	for ( int i = 0; i < MAX_PLAYER_PROFILE_STATS; ++i ) {
		stats[i].i = 0;
	}

	deviceNum			= 0;
	state				= IDLE;
	requestedState		= IDLE;
	frameScaleX			= 0.85f;
	frameScaleY			= 0.85f;
}

/*
========================
idPlayerProfile::Init
========================
*/
void idPlayerProfile::Init() {
	SetDefaults();
}

/*
========================
idPlayerProfile::~idPlayerProfile
========================
*/
idPlayerProfile::~idPlayerProfile() {
}

/*
========================
idPlayerProfile::SerializeSettings
========================
*/
bool idPlayerProfile::SerializeSettings( idSerializer & ser ) {
	int flags = cvarSystem->GetModifiedFlags();

	// Default to current tag/version
	int32	tag		= GetProfileTag();
	int32	version	= FRAMEWORK_PROFILE_VER;

	// Serialize tag/version
	ser.SerializePacked( tag );
	if ( tag != GetProfileTag() ) {
		idLib::Warning( "Profile tag did not match, profile will be re-initialized" );
		SetDefaults();
		SaveSettings();	// Flag the profile to save so we have the latest version stored

		return false;		
	}
	ser.SerializePacked( version );
	if ( version != FRAMEWORK_PROFILE_VER ) {
		// For now, don't allow profiles with invalid versions load
		// We could easily support old version by doing a few version checks below to pick and choose what we load as well.
		idLib::Warning( "Profile version did not match.  Profile will be replaced" );
		SetDefaults();
		SaveSettings();	// Flag the profile to save so we have the latest version stored

		return false;		
	}

	// Serialize audio settings
	SERIALIZE_BOOL( ser, seenInstallMessage );

	// New setting to save to make sure that we have or haven't seen this achievement before used to pass TRC R149d
	ser.Serialize( achievementBits );

	ser.Serialize( frameScaleX );
	ser.Serialize( frameScaleY );
	SERIALIZE_BOOL( ser, alwaysRun );


	// we save all the cvar-based settings in the profile even though some cvars are archived
	// so that we are consistent and don't miss any or get affected when the archive flag is changed
	SERIALIZE_CVAR_INT( ser, s_volume_sound );
	SERIALIZE_CVAR_INT( ser, s_volume_midi );
	
	// Don't trigger profile save due to modified archived cvars during profile load
	cvarSystem->ClearModifiedFlags( CVAR_ARCHIVE );	// must clear because set() is an OR operation, not assignment...
	cvarSystem->SetModifiedFlags( flags );

	return true;
}
/*
========================
idPlayerProfile::GetLevel
========================
*/
int idPlayerProfile::GetLevel() const {
	return 1;
}

/*
========================
idPlayerProfile::StatSetInt
========================
*/
void idPlayerProfile::StatSetInt( int s, int v ) {
	stats[s].i = v;
}

/*
========================
idPlayerProfile::StatSetFloat
========================
*/
void idPlayerProfile::StatSetFloat( int s, float v ) {
	stats[s].f = v;
}

/*
========================
idPlayerProfile::StatGetInt
========================
*/
int	idPlayerProfile::StatGetInt( int s ) const { 
	return stats[s].i;
}

/*
========================
idPlayerProfile::StatGetFloat
========================
*/
float idPlayerProfile::StatGetFloat( int s ) const {
	return stats[s].f;
}

/*
========================
idPlayerProfile::SaveSettings
========================
*/
void idPlayerProfile::SaveSettings() {
	if ( state != SAVING ) {
		if ( GetRequestedState() == IDLE ) {
			SetRequestedState( SAVE_REQUESTED );
		}
	}
}

/*
========================
idPlayerProfile::SaveSettings
========================
*/
void idPlayerProfile::LoadSettings() {
	if ( state != LOADING ) {
		if ( verify( GetRequestedState() == IDLE ) ) {
			SetRequestedState( LOAD_REQUESTED );
		}
	}
}

/*
========================
idPlayerProfile::SetAchievementBit
========================
*/
void idPlayerProfile::SetAchievementBit( const int id ) {
	if ( id > 63 ) {
		assert( false );		// FIXME: add another set of achievement bit flags
		return;
	}

	achievementBits |= (int64)1 << id;
}

/*
========================
idPlayerProfile::ClearAchievementBit
========================
*/
void idPlayerProfile::ClearAchievementBit( const int id ) {
	if ( id > 63 ) {
		assert( false );		// FIXME: add another set of achievement bit flags
		return;
	}

	achievementBits &= ~( (int64)1 << id );
}

/*
========================
idPlayerProfile::GetAchievementBit
========================
*/
bool idPlayerProfile::GetAchievementBit( const int id ) const {
	if ( id > 63 ) {
		assert( false );		// FIXME: add another set of achievement bit flags
		return false;
	}

	return ( achievementBits & (int64)1 << id ) != 0;
}

/*
========================
Returns the value stored in the music volume cvar.
========================
*/
int	idPlayerProfile::GetMusicVolume() const {
	return s_volume_midi.GetInteger();
}

/*
========================
Returns the value stored in the sound volume cvar.
========================
*/
int	idPlayerProfile::GetSoundVolume() const {
	return s_volume_sound.GetInteger();
}

/*
========================
Sets the music volume cvar.
========================
*/
void idPlayerProfile::SetMusicVolume( int volume ) {
	s_volume_midi.SetInteger( volume );
}

/*
========================
Sets the sound volume cvar.
========================
*/
void idPlayerProfile::SetSoundVolume( int volume ) {
	s_volume_sound.SetInteger( volume );
}
