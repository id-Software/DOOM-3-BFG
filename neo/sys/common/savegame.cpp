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
#include "../sys_session_local.h"
#include "../sys_savegame.h"

idCVar savegame_winInduceDelay( "savegame_winInduceDelay", "0", CVAR_INTEGER, "on windows, this is a delay induced before any file operation occurs" );
extern idCVar fs_savepath;
extern idCVar saveGame_checksum;
extern idCVar savegame_error;

#define SAVEGAME_SENTINAL				0x12358932

// RB begin
#ifndef _WIN32 // DG: unify win32 and posix savegames
	#define ERROR_SUCCESS	0
#endif
// RB end
/*
========================
void Sys_ExecuteSavegameCommandAsync
========================
*/
void Sys_ExecuteSavegameCommandAsyncImpl( idSaveLoadParms* savegameParms )
{
	assert( savegameParms != NULL );

	session->GetSaveGameManager().GetSaveGameThread().data.saveLoadParms = savegameParms;

	if( session->GetSaveGameManager().GetSaveGameThread().GetThreadHandle() == 0 )
	{
		session->GetSaveGameManager().GetSaveGameThread().StartWorkerThread( "Savegame", CORE_ANY );
	}

	session->GetSaveGameManager().GetSaveGameThread().SignalWork();
}

/*
========================
idLocalUser * GetLocalUserFromUserId
========================
*/
idLocalUserWin* GetLocalUserFromSaveParms( const saveGameThreadArgs_t& data )
{
	if( ( data.saveLoadParms != NULL ) && ( data.saveLoadParms->inputDeviceId >= 0 ) )
	{
		idLocalUser* user = session->GetSignInManager().GetLocalUserByInputDevice( data.saveLoadParms->inputDeviceId );
		if( user != NULL )
		{
			idLocalUserWin* userWin = static_cast< idLocalUserWin* >( user );
			if( userWin != NULL && data.saveLoadParms->userId == idStr::Hash( userWin->GetGamerTag() ) )
			{
				return userWin;
			}
		}
	}

	return NULL;
}

/*
========================
idSaveGameThread::SaveGame
========================
*/
int idSaveGameThread::Save()
{
	idLocalUserWin* user = GetLocalUserFromSaveParms( data );
	if( user == NULL )
	{
		data.saveLoadParms->errorCode = SAVEGAME_E_INVALID_USER;
		return -1;
	}

	idSaveLoadParms* callback = data.saveLoadParms;
	idStr saveFolder = "savegame";

	saveFolder.AppendPath( callback->directory );

	// Check for the required storage space.
	int64 requiredSizeBytes = 0;
	{
		for( int i = 0; i < callback->files.Num(); i++ )
		{
			idFile_SaveGame* 	file = callback->files[i];
			requiredSizeBytes += ( file->Length() + sizeof( unsigned int ) ); // uint for checksum
			if( file->type == SAVEGAMEFILE_PIPELINED )
			{
				requiredSizeBytes += MIN_SAVEGAME_SIZE_BYTES;
			}
		}
	}

	int ret = ERROR_SUCCESS;

	// Check size of previous files if needed
	// ALL THE FILES RIGHT NOW----  could use pattern later...
	idStrList filesToDelete;
	if( ( callback->mode & SAVEGAME_MBF_DELETE_FILES ) && !callback->cancelled )
	{
		if( fileSystem->IsFolder( saveFolder.c_str(), "fs_savePath" ) == FOLDER_YES )
		{
			idFileList* files = fileSystem->ListFilesTree( saveFolder.c_str(), "*.*" );
			for( int i = 0; i < files->GetNumFiles(); i++ )
			{
				requiredSizeBytes -= fileSystem->GetFileLength( files->GetFile( i ) );
				filesToDelete.Append( files->GetFile( i ) );
			}
			fileSystem->FreeFileList( files );
		}
	}

	// RB: disabled savegame and profile storage checks, because it fails sometimes without any clear reason
	/*
	// Inform user about size required if necessary
	if( requiredSizeBytes > 0 && !callback->cancelled )
	{
		user->StorageSizeAvailable( requiredSizeBytes, callback->requiredSpaceInBytes );
		if( callback->requiredSpaceInBytes > 0 )
		{
			// check to make sure savepath actually exists before erroring
			idStr directory = fs_savepath.GetString();
			directory += "\\";	// so it doesn't think the last part is a file and ignores in the directory creation
			fileSystem->CreateOSPath( directory );  // we can't actually check FileExists in production builds, so just try to create it
			user->StorageSizeAvailable( requiredSizeBytes, callback->requiredSpaceInBytes );

			if( callback->requiredSpaceInBytes > 0 )
			{
				callback->errorCode = SAVEGAME_E_INSUFFICIENT_ROOM;
				// safe to return, haven't written any files yet
				return -1;
			}
		}
	}
	*/
	// RB end

	// Delete all previous files if needed
	// ALL THE FILES RIGHT NOW----  could use pattern later...
	for( int i = 0; i < filesToDelete.Num() && !callback->cancelled; i++ )
	{
		fileSystem->RemoveFile( filesToDelete[i].c_str() );
	}

	// Save the raw files.
	for( int i = 0; i < callback->files.Num() && ret == ERROR_SUCCESS && !callback->cancelled; i++ )
	{
		idFile_SaveGame* file = callback->files[i];

		idStr fileName = saveFolder;
		fileName.AppendPath( file->GetName() );
		idStr tempFileName = va( "%s.temp", fileName.c_str() );

		idFile* outputFile = fileSystem->OpenFileWrite( tempFileName, "fs_savePath" );
		if( outputFile == NULL )
		{
#ifdef _WIN32 // DG: unify windows and posix savegames => replace GetLastError with strerror(errno)
			idLib::Warning( "[%s]: Couldn't open file for writing, %s. Error = %08x", __FUNCTION__, tempFileName.c_str(), GetLastError() );
#else
			idLib::Warning( "[%s]: Couldn't open file for writing, %s. Error = %s", __FUNCTION__, tempFileName.c_str(), strerror( errno ) );
#endif // DG end
			file->error = true;
			callback->errorCode = SAVEGAME_E_UNKNOWN;
			ret = -1;
			continue;
		}

		if( ( file->type & SAVEGAMEFILE_PIPELINED ) != 0 )
		{

			idFile_SaveGamePipelined* inputFile = dynamic_cast< idFile_SaveGamePipelined* >( file );
			assert( inputFile != NULL );

			blockForIO_t block;
			while( inputFile->NextWriteBlock( & block ) )
			{
				if( ( size_t )outputFile->Write( block.data, block.bytes ) != block.bytes )
				{
#ifdef _WIN32 // DG: unify windows and posix savegames => replace GetLastError with strerror(errno)
					idLib::Warning( "[%s]: Write failed. Error = %08x", __FUNCTION__, GetLastError() );
#else
					idLib::Warning( "[%s]: Write failed. Error = %s", __FUNCTION__, strerror( errno ) );
#endif // DG end
					file->error = true;
					callback->errorCode = SAVEGAME_E_INSUFFICIENT_ROOM;
					ret = -1;
					break;
				}
			}

		}
		else
		{

			if( ( file->type & SAVEGAMEFILE_BINARY ) || ( file->type & SAVEGAMEFILE_COMPRESSED ) )
			{
				if( saveGame_checksum.GetBool() )
				{
					unsigned int checksum = MD5_BlockChecksum( file->GetDataPtr(), file->Length() );
					size_t size = outputFile->WriteBig( checksum );
					if( size != sizeof( checksum ) )
					{
#ifdef _WIN32 // DG: unify windows and posix savegames => replace GetLastError with strerror(errno)
						idLib::Warning( "[%s]: Write failed. Error = %08x", __FUNCTION__, GetLastError() );
#else
						idLib::Warning( "[%s]: Write failed. Error = %s", __FUNCTION__, strerror( errno ) );
#endif // DG end
						file->error = true;
						callback->errorCode = SAVEGAME_E_INSUFFICIENT_ROOM;
						ret = -1;
					}
				}
			}

			size_t size = outputFile->Write( file->GetDataPtr(), file->Length() );
			if( size != ( size_t )file->Length() )
			{
#ifdef _WIN32 // DG: unify windows and posix savegames => replace GetLastError with strerror(errno)
				idLib::Warning( "[%s]: Write failed. Error = %08x", __FUNCTION__, GetLastError() );
#else
				idLib::Warning( "[%s]: Write failed. Error = %s", __FUNCTION__, strerror( errno ) );
#endif // DG end
				file->error = true;
				callback->errorCode = SAVEGAME_E_INSUFFICIENT_ROOM;
				ret = -1;
			}
			else
			{
				idLib::PrintfIf( saveGame_verbose.GetBool(), "Saved %s (%s)\n", fileName.c_str(), outputFile->GetFullPath() );
			}
		}

		delete outputFile;

		if( ret == ERROR_SUCCESS )
		{
			// Remove the old file
			if( !fileSystem->RenameFile( tempFileName, fileName, "fs_savePath" ) )
			{
				idLib::Warning( "Could not start to rename temporary file %s to %s.", tempFileName.c_str(), fileName.c_str() );
			}
		}
		else
		{
			fileSystem->RemoveFile( tempFileName );
			idLib::Warning( "Invalid write to temporary file %s.", tempFileName.c_str() );
		}
	}

	if( data.saveLoadParms->cancelled )
	{
		data.saveLoadParms->errorCode = SAVEGAME_E_CANCELLED;
	}

	// Removed because it seemed a bit drastic
#if 0
	// If there is an error, delete the partially saved folder
	if( callback->errorCode != SAVEGAME_E_NONE )
	{
		if( fileSystem->IsFolder( saveFolder, "fs_savePath" ) == FOLDER_YES )
		{
			idFileList* files = fileSystem->ListFilesTree( saveFolder, "/|*" );
			for( int i = 0; i < files->GetNumFiles(); i++ )
			{
				fileSystem->RemoveFile( files->GetFile( i ) );
			}
			fileSystem->FreeFileList( files );
			fileSystem->RemoveDir( saveFolder );
		}
	}
#endif

	return ret;
}

/*
========================
idSessionLocal::LoadGame
========================
*/
int idSaveGameThread::Load()
{
	idSaveLoadParms* callback = data.saveLoadParms;
	idStr saveFolder = "savegame";

	saveFolder.AppendPath( callback->directory );

	if( fileSystem->IsFolder( saveFolder, "fs_savePath" ) != FOLDER_YES )
	{
		callback->errorCode = SAVEGAME_E_FOLDER_NOT_FOUND;
		return -1;
	}

	int ret = ERROR_SUCCESS;
	for( int i = 0; i < callback->files.Num() && ret == ERROR_SUCCESS && !callback->cancelled; i++ )
	{
		idFile_SaveGame* file = callback->files[i];

		idStr filename = saveFolder;
		filename.AppendPath( file->GetName() );

		idFile* inputFile = fileSystem->OpenFileRead( filename.c_str() );
		if( inputFile == NULL )
		{
			file->error = true;
			if( !( file->type & SAVEGAMEFILE_OPTIONAL ) )
			{
				callback->errorCode = SAVEGAME_E_CORRUPTED;
				ret = -1;
			}
			continue;
		}

		if( ( file->type & SAVEGAMEFILE_PIPELINED ) != 0 )
		{

			idFile_SaveGamePipelined* outputFile = dynamic_cast< idFile_SaveGamePipelined* >( file );
			assert( outputFile != NULL );

			size_t lastReadBytes = 0;
			blockForIO_t block;
			while( outputFile->NextReadBlock( &block, lastReadBytes ) && !callback->cancelled )
			{
				lastReadBytes = inputFile->Read( block.data, block.bytes );
				if( lastReadBytes != block.bytes )
				{
					// Notify end-of-file to the save game file which will cause all reads on the
					// other end of the pipeline to return zero bytes after the pipeline is drained.
					outputFile->NextReadBlock( NULL, lastReadBytes );
					break;
				}
			}

		}
		else
		{

			size_t size = inputFile->Length();

			unsigned int originalChecksum = 0;
			if( ( file->type & SAVEGAMEFILE_BINARY ) != 0 || ( file->type & SAVEGAMEFILE_COMPRESSED ) != 0 )
			{
				if( saveGame_checksum.GetBool() )
				{
					if( size >= sizeof( originalChecksum ) )
					{
						inputFile->ReadBig( originalChecksum );
						size -= sizeof( originalChecksum );
					}
				}
			}

			file->SetLength( size );

			size_t sizeRead = inputFile->Read( ( void* )file->GetDataPtr(), size );
			if( sizeRead != size )
			{
				file->error = true;
				callback->errorCode = SAVEGAME_E_CORRUPTED;
				ret = -1;
			}

			if( ( file->type & SAVEGAMEFILE_BINARY ) != 0 || ( file->type & SAVEGAMEFILE_COMPRESSED ) != 0 )
			{
				if( saveGame_checksum.GetBool() )
				{
					unsigned int checksum = MD5_BlockChecksum( file->GetDataPtr(), file->Length() );
					if( checksum != originalChecksum )
					{
						file->error = true;
						callback->errorCode = SAVEGAME_E_CORRUPTED;
						ret = -1;
					}
				}
			}
		}

		delete inputFile;
	}

	if( data.saveLoadParms->cancelled )
	{
		data.saveLoadParms->errorCode = SAVEGAME_E_CANCELLED;
	}

	return ret;
}

/*
========================
idSaveGameThread::Delete

This deletes a complete savegame directory
========================
*/
int idSaveGameThread::Delete()
{
	idSaveLoadParms* callback = data.saveLoadParms;
	idStr saveFolder = "savegame";

	saveFolder.AppendPath( callback->directory );

	int ret = ERROR_SUCCESS;
	if( fileSystem->IsFolder( saveFolder, "fs_savePath" ) == FOLDER_YES )
	{
		idFileList* files = fileSystem->ListFilesTree( saveFolder, "/|*" );
		for( int i = 0; i < files->GetNumFiles() && !callback->cancelled; i++ )
		{
			fileSystem->RemoveFile( files->GetFile( i ) );
		}
		fileSystem->FreeFileList( files );

		fileSystem->RemoveDir( saveFolder );
	}
	else
	{
		callback->errorCode = SAVEGAME_E_FOLDER_NOT_FOUND;
		ret = -1;
	}

	if( data.saveLoadParms->cancelled )
	{
		data.saveLoadParms->errorCode = SAVEGAME_E_CANCELLED;
	}

	return ret;
}

/*
========================
idSaveGameThread::Enumerate
========================
*/
int idSaveGameThread::Enumerate()
{
	idSaveLoadParms* callback = data.saveLoadParms;
	idStr saveFolder = "savegame";

	callback->detailList.Clear();

	int ret = ERROR_SUCCESS;
	if( fileSystem->IsFolder( saveFolder, "fs_savePath" ) == FOLDER_YES )
	{
		idFileList* files = fileSystem->ListFilesTree( saveFolder, SAVEGAME_DETAILS_FILENAME );
		const idStrList& fileList = files->GetList();

		for( int i = 0; i < fileList.Num() && !callback->cancelled; i++ )
		{
			idSaveGameDetails* details = callback->detailList.Alloc();
			// We have more folders on disk than we have room in our save detail list, stop trying to read them in and continue with what we have
			if( details == NULL )
			{
				break;
			}
			idStr directory = fileList[i];

			idFile* file = fileSystem->OpenFileRead( directory.c_str() );

			if( file != NULL )
			{
				// Read the DETAIL file for the enumerated data
				if( callback->mode & SAVEGAME_MBF_READ_DETAILS )
				{
					if( !SavegameReadDetailsFromFile( file, *details ) )
					{
						details->damaged = true;
						ret = -1;
					}
				}
#ifdef _WIN32 // DG: unification of win32 and posix savagame code
				// Use the date from the directory
				WIN32_FILE_ATTRIBUTE_DATA attrData;
				BOOL attrRet = GetFileAttributesEx( file->GetFullPath(), GetFileExInfoStandard, &attrData );
				delete file;
				if( attrRet == TRUE )
				{
					FILETIME		lastWriteTime = attrData.ftLastWriteTime;
					const ULONGLONG second = 10000000L; // One second = 10,000,000 * 100 nsec
					SYSTEMTIME		base_st = { 1970, 1, 0, 1, 0, 0, 0, 0 };
					ULARGE_INTEGER	itime;
					FILETIME		base_ft;
					BOOL			success = SystemTimeToFileTime( &base_st, &base_ft );

					itime.QuadPart = ( ( ULARGE_INTEGER* )&lastWriteTime )->QuadPart;
					if( success )
					{
						itime.QuadPart -= ( ( ULARGE_INTEGER* )&base_ft )->QuadPart;
					}
					else
					{
						// Hard coded number of 100-nanosecond units from 1/1/1601 to 1/1/1970
						itime.QuadPart -= 116444736000000000LL;
					}
					itime.QuadPart /= second;
					details->date = itime.QuadPart;
				}
#else
				// DG: just use the idFile object's timestamp - the windows code gets file attributes and
				//  other complicated stuff like that.. I'm wonderin what that was good for.. this seems to work.
				details->date = file->Timestamp();
#endif // DG end
			}
			else
			{
				details->damaged = true;
			}

			// populate the game details struct
			directory = directory.StripFilename();
			details->slotName = directory.c_str() + saveFolder.Length() + 1; // Strip off the prefix too
// JDC: I hit this all the time			assert( fileSystem->IsFolder( directory.c_str(), "fs_savePath" ) == FOLDER_YES );
		}
		fileSystem->FreeFileList( files );
	}
	else
	{
		callback->errorCode = SAVEGAME_E_FOLDER_NOT_FOUND;
		ret = -3;
	}

	if( data.saveLoadParms->cancelled )
	{
		data.saveLoadParms->errorCode = SAVEGAME_E_CANCELLED;
	}

	return ret;
}

/*
========================
idSaveGameThread::EnumerateFiles
========================
*/
int idSaveGameThread::EnumerateFiles()
{
	idSaveLoadParms* callback = data.saveLoadParms;
	idStr folder = "savegame";

	folder.AppendPath( callback->directory );

	callback->files.Clear();

	int ret = ERROR_SUCCESS;
	if( fileSystem->IsFolder( folder, "fs_savePath" ) == FOLDER_YES )
	{
		// get listing of all the files, but filter out below
		idFileList* files = fileSystem->ListFilesTree( folder, "*.*" );

		// look for the instance pattern
		for( int i = 0; i < files->GetNumFiles() && ret == 0 && !callback->cancelled; i++ )
		{
			idStr fullFilename = files->GetFile( i );
			idStr filename = fullFilename;
			filename.StripPath();

			if( filename.IcmpPrefix( callback->pattern ) != 0 )
			{
				continue;
			}
			if( !callback->postPattern.IsEmpty() && filename.Right( callback->postPattern.Length() ).IcmpPrefix( callback->postPattern ) != 0 )
			{
				continue;
			}

			// Read the DETAIL file for the enumerated data
			if( callback->mode & SAVEGAME_MBF_READ_DETAILS )
			{
				idSaveGameDetails& details = callback->description;
				idFile* uncompressed = fileSystem->OpenFileRead( fullFilename.c_str() );

				if( uncompressed == NULL )
				{
					details.damaged = true;
				}
				else
				{
					if( !SavegameReadDetailsFromFile( uncompressed, details ) )
					{
						ret = -1;
					}

					delete uncompressed;
				}

				// populate the game details struct
				details.slotName = callback->directory;
				assert( fileSystem->IsFolder( details.slotName, "fs_savePath" ) == FOLDER_YES );
			}

			idFile_SaveGame* file = new( TAG_SAVEGAMES ) idFile_SaveGame( filename, SAVEGAMEFILE_AUTO_DELETE );
			callback->files.Append( file );
		}
		fileSystem->FreeFileList( files );
	}
	else
	{
		callback->errorCode = SAVEGAME_E_FOLDER_NOT_FOUND;
		ret = -3;
	}

	if( data.saveLoadParms->cancelled )
	{
		data.saveLoadParms->errorCode = SAVEGAME_E_CANCELLED;
	}

	return ret;
}

/*
========================
idSaveGameThread::DeleteFiles
========================
*/
int idSaveGameThread::DeleteFiles()
{
	idSaveLoadParms* callback = data.saveLoadParms;
	idStr folder = "savegame";

	folder.AppendPath( callback->directory );

	// delete the explicitly requested files first
	for( int j = 0; j < callback->files.Num() && !callback->cancelled; ++j )
	{
		idFile_SaveGame* file = callback->files[j];
		idStr fullpath = folder;
		fullpath.AppendPath( file->GetName() );
		fileSystem->RemoveFile( fullpath );
	}

	int ret = ERROR_SUCCESS;
	if( fileSystem->IsFolder( folder, "fs_savePath" ) == FOLDER_YES )
	{
		// get listing of all the files, but filter out below
		idFileList* files = fileSystem->ListFilesTree( folder, "*.*" );

		// look for the instance pattern
		for( int i = 0; i < files->GetNumFiles() && !callback->cancelled; i++ )
		{
			idStr filename = files->GetFile( i );
			filename.StripPath();

			// If there are post/pre patterns to match, make sure we adhere to the patterns
			if( callback->pattern.IsEmpty() || ( filename.IcmpPrefix( callback->pattern ) != 0 ) )
			{
				continue;
			}
			if( callback->postPattern.IsEmpty() || ( filename.Right( callback->postPattern.Length() ).IcmpPrefix( callback->postPattern ) != 0 ) )
			{
				continue;
			}

			fileSystem->RemoveFile( files->GetFile( i ) );
		}
		fileSystem->FreeFileList( files );
	}
	else
	{
		callback->errorCode = SAVEGAME_E_FOLDER_NOT_FOUND;
		ret = -3;
	}

	if( data.saveLoadParms->cancelled )
	{
		data.saveLoadParms->errorCode = SAVEGAME_E_CANCELLED;
	}

	return ret;
}

/*
========================
idSaveGameThread::DeleteAll

This deletes all savegame directories
========================
*/
int idSaveGameThread::DeleteAll()
{
	idSaveLoadParms* callback = data.saveLoadParms;
	idStr saveFolder = "savegame";
	int ret = ERROR_SUCCESS;

	if( fileSystem->IsFolder( saveFolder, "fs_savePath" ) == FOLDER_YES )
	{
		idFileList* files = fileSystem->ListFilesTree( saveFolder, "/|*" );
		// remove directories after files
		for( int i = 0; i < files->GetNumFiles() && !callback->cancelled; i++ )
		{
			// contained files should always be first
			if( fileSystem->IsFolder( files->GetFile( i ), "fs_savePath" ) == FOLDER_YES )
			{
				fileSystem->RemoveDir( files->GetFile( i ) );
			}
			else
			{
				fileSystem->RemoveFile( files->GetFile( i ) );
			}
		}
		fileSystem->FreeFileList( files );
	}
	else
	{
		callback->errorCode = SAVEGAME_E_FOLDER_NOT_FOUND;
		ret = -3;
	}

	if( data.saveLoadParms->cancelled )
	{
		data.saveLoadParms->errorCode = SAVEGAME_E_CANCELLED;
	}

	return ret;
}

/*
========================
idSaveGameThread::Run
========================
*/
int idSaveGameThread::Run()
{
	OPTICK_THREAD( "idSaveGameThread" );

	int ret = ERROR_SUCCESS;

	try
	{
		idLocalUserWin* user = GetLocalUserFromSaveParms( data );
		if( user != NULL && !user->IsStorageDeviceAvailable() )
		{
			data.saveLoadParms->errorCode = SAVEGAME_E_UNABLE_TO_SELECT_STORAGE_DEVICE;
		}

		if( savegame_winInduceDelay.GetInteger() > 0 )
		{
			Sys_Sleep( savegame_winInduceDelay.GetInteger() );
		}

		if( data.saveLoadParms->mode & SAVEGAME_MBF_SAVE )
		{
			ret = Save();
		}
		else if( data.saveLoadParms->mode & SAVEGAME_MBF_LOAD )
		{
			ret = Load();
		}
		else if( data.saveLoadParms->mode & SAVEGAME_MBF_ENUMERATE )
		{
			ret = Enumerate();
		}
		else if( data.saveLoadParms->mode & SAVEGAME_MBF_DELETE_FOLDER )
		{
			ret = Delete();
		}
		else if( data.saveLoadParms->mode & SAVEGAME_MBF_DELETE_ALL_FOLDERS )
		{
			ret = DeleteAll();
		}
		else if( data.saveLoadParms->mode & SAVEGAME_MBF_DELETE_FILES )
		{
			ret = DeleteFiles();
		}
		else if( data.saveLoadParms->mode & SAVEGAME_MBF_ENUMERATE_FILES )
		{
			ret = EnumerateFiles();
		}

		// if something failed and no one set an error code, do it now.
		if( ret != 0 && data.saveLoadParms->errorCode == SAVEGAME_E_NONE )
		{
			data.saveLoadParms->errorCode = SAVEGAME_E_UNKNOWN;
		}
	}
	catch( ... )
	{
		// if anything horrible happens, leave it up to the savegame processors to handle in PostProcess().
		data.saveLoadParms->errorCode = SAVEGAME_E_UNKNOWN;
	}

	// Make sure to cancel any save game file pipelines.
	if( data.saveLoadParms->errorCode != SAVEGAME_E_NONE )
	{
		data.saveLoadParms->CancelSaveGameFilePipelines();
	}

	// Override error if cvar set
	if( savegame_error.GetInteger() != 0 )
	{
		data.saveLoadParms->errorCode = ( saveGameError_t )savegame_error.GetInteger();
	}

	// Tell the waiting caller that we are done
	data.saveLoadParms->callbackSignal.Raise();

	return ret;
}

/*
========================
Sys_SaveGameCheck
========================
*/
void Sys_SaveGameCheck( bool& exists, bool& autosaveExists )
{
	exists = false;
	autosaveExists = false;

	const idStr autosaveFolderStr = AddSaveFolderPrefix( SAVEGAME_AUTOSAVE_FOLDER, idSaveGameManager::PACKAGE_GAME );
	const char* autosaveFolder = autosaveFolderStr.c_str();
	const char* saveFolder = "savegame";

	if( fileSystem->IsFolder( saveFolder, "fs_savePath" ) == FOLDER_YES )
	{
		idFileList* files = fileSystem->ListFiles( saveFolder, "/" );
		const idStrList& fileList = files->GetList();

		idLib::PrintfIf( saveGame_verbose.GetBool(), "found %d savegames\n", fileList.Num() );

		for( int i = 0; i < fileList.Num(); i++ )
		{
			const char* directory = va( "%s/%s", saveFolder, fileList[i].c_str() );

			if( fileSystem->IsFolder( directory, "fs_savePath" ) == FOLDER_YES )
			{
				exists = true;

				idLib::PrintfIf( saveGame_verbose.GetBool(), "found savegame: %s\n", fileList[i].c_str() );

				if( idStr::Icmp( fileList[i].c_str(), autosaveFolder ) == 0 )
				{
					autosaveExists = true;
					break;
				}
			}
		}

		fileSystem->FreeFileList( files );
	}
}
