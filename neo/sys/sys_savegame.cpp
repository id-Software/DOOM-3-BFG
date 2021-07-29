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
#include "sys_session_local.h"


idCVar saveGame_verbose( "saveGame_verbose", "0", CVAR_BOOL | CVAR_ARCHIVE, "debug spam" );
idCVar saveGame_checksum( "saveGame_checksum", "1", CVAR_BOOL, "data integrity check" );
idCVar saveGame_enable( "saveGame_enable", "1", CVAR_BOOL, "are savegames enabled" );

void Sys_ExecuteSavegameCommandAsyncImpl( idSaveLoadParms* savegameParms );


/*
========================
Sys_ExecuteSavegameCommandAsync
========================
*/
void Sys_ExecuteSavegameCommandAsync( idSaveLoadParms* savegameParms )
{
	if( savegameParms == NULL )
	{
		idLib::Error( "Programming Error with [%s]", __FUNCTION__ );
		return;
	}

	if( !saveGame_enable.GetBool() )
	{
		idLib::Warning( "Savegames are disabled (saveGame_enable = 0). Skipping physical save to media." );
		savegameParms->errorCode = SAVEGAME_E_CANCELLED;
		savegameParms->callbackSignal.Raise();
		return;
	}

	Sys_ExecuteSavegameCommandAsyncImpl( savegameParms );
}

#define ASSERT_ENUM_STRING_BITFIELD( string, index )		( 1 / (int)!( string - ( 1 << index ) ) ) ? #string : ""

const char* saveGameErrorStrings[ SAVEGAME_E_NUM ] =
{
	ASSERT_ENUM_STRING_BITFIELD( SAVEGAME_E_CANCELLED,							0 ),
	ASSERT_ENUM_STRING_BITFIELD( SAVEGAME_E_INSUFFICIENT_ROOM,					1 ),
	ASSERT_ENUM_STRING_BITFIELD( SAVEGAME_E_CORRUPTED,							2 ),
	ASSERT_ENUM_STRING_BITFIELD( SAVEGAME_E_UNABLE_TO_SELECT_STORAGE_DEVICE,	3 ),
	ASSERT_ENUM_STRING_BITFIELD( SAVEGAME_E_UNKNOWN,							4 ),
	ASSERT_ENUM_STRING_BITFIELD( SAVEGAME_E_INVALID_FILENAME,					5 ),
	ASSERT_ENUM_STRING_BITFIELD( SAVEGAME_E_STEAM_ERROR,						6 ),
	ASSERT_ENUM_STRING_BITFIELD( SAVEGAME_E_FOLDER_NOT_FOUND,					7 ),
	ASSERT_ENUM_STRING_BITFIELD( SAVEGAME_E_FILE_NOT_FOUND,						8 ),
	ASSERT_ENUM_STRING_BITFIELD( SAVEGAME_E_DLC_NOT_FOUND,						9 ),
	ASSERT_ENUM_STRING_BITFIELD( SAVEGAME_E_INVALID_USER,						10 ),
	ASSERT_ENUM_STRING_BITFIELD( SAVEGAME_E_PROFILE_TOO_BIG,					11 ),
	ASSERT_ENUM_STRING_BITFIELD( SAVEGAME_E_DISC_SWAP,							12 ),
	ASSERT_ENUM_STRING_BITFIELD( SAVEGAME_E_INCOMPATIBLE_NEWER_VERSION,			13 ),
};

CONSOLE_COMMAND( savegamePrintErrors, "Prints error code corresponding to each bit", 0 )
{
	idLib::Printf( "Bit  Description\n"
				   "---  -----------\n" );
	for( int i = 0; i < SAVEGAME_E_BITS_USED; i++ )
	{
		idLib::Printf( "%03d  %s\n", i, saveGameErrorStrings[i] );
	}
}

/*
========================
GetSaveGameErrorString

This returns a comma delimited string of all the errors within the bitfield.  We don't ever expect more than one error,
the errors are bitfields so that they can be used to mask which errors we want to handle in the engine or leave up to the
game.

Example:
	SAVEGAME_E_LOAD, SAVEGAME_E_INVALID_FILENAME
========================
*/
idStr GetSaveGameErrorString( int errorMask )
{
	idStr errorString;
	bool continueProcessing = errorMask > 0;
	int localError = errorMask;

	for( int i = 0; i < SAVEGAME_E_NUM && continueProcessing; ++i )
	{
		int mask = ( 1 << i );

		if( localError & mask )
		{
			localError ^= mask;	// turn off this error so we can quickly see if we are done

			continueProcessing = localError > 0;

			errorString.Append( saveGameErrorStrings[i] );

			if( continueProcessing )
			{
				errorString.Append( ", " );
			}
		}
	}

	return errorString;
}

/*
========================
GetSaveFolder

Prefixes help the PS3 filter what to show in lists
Files using the hidden prefix are not shown in the load game screen
Directory name for savegames, slot number or user-defined name appended to it
TRC R116 - PS3 folder must start with the product code
========================
*/
const idStr& GetSaveFolder( idSaveGameManager::packageType_t type )
{
	static bool initialized = false;
	static idStrStatic<MAX_FOLDER_NAME_LENGTH>	saveFolder[idSaveGameManager::PACKAGE_NUM];

	if( !initialized )
	{
		initialized = true;

		idStr ps3Header = "";

		saveFolder[idSaveGameManager::PACKAGE_GAME].Format( "%s%s", ps3Header.c_str(), SAVEGAME_GAME_DIRECTORY_PREFIX );
		saveFolder[idSaveGameManager::PACKAGE_PROFILE].Format( "%s%s", ps3Header.c_str(), SAVEGAME_PROFILE_DIRECTORY_PREFIX );
		saveFolder[idSaveGameManager::PACKAGE_RAW].Format( "%s%s", ps3Header.c_str(), SAVEGAME_RAW_DIRECTORY_PREFIX );
	}

	return saveFolder[type];
}

/*
========================
idStr AddSaveFolderPrefix

	input	= RAGE_0
	output	= GAMES-RAGE_0
========================
*/
idStr AddSaveFolderPrefix( const char* folder, idSaveGameManager::packageType_t type )
{
	idStr dir = GetSaveFolder( type );
	dir.Append( folder );


	return dir;
}

/*
========================
RemoveSaveFolderPrefix

	input	= GAMES-RAGE_0
	output	= RAGE_0
========================
*/
idStr RemoveSaveFolderPrefix( const char* folder, idSaveGameManager::packageType_t type )
{
	idStr dir = folder;
	idStr prefix = GetSaveFolder( type );
	dir.StripLeading( prefix );
	return dir;
}

/*
========================
bool SavegameReadDetailsFromFile

returns false when catastrophic error occurs, not when damaged
========================
*/
bool SavegameReadDetailsFromFile( idFile* file, idSaveGameDetails& details )
{
	details.damaged = false;

	// Read the DETAIL file for the enumerated data
	if( !details.descriptors.ReadFromIniFile( file ) )
	{
		details.damaged = true;
	}

	bool ignoreChecksum = details.descriptors.GetBool( "ignore_checksum", false );
	if( !ignoreChecksum )
	{
		// Get the checksum from the dict
		int readChecksum = details.descriptors.GetInt( SAVEGAME_DETAIL_FIELD_CHECKSUM, 0 );

		// Calculate checksum
		details.descriptors.Delete( SAVEGAME_DETAIL_FIELD_CHECKSUM );
		int checksum = ( int )details.descriptors.Checksum();
		if( readChecksum == 0 || checksum != readChecksum )
		{
			details.damaged = true;
		}
	}

	return true;
}

/*
========================
idSaveGameDetails::idSaveGameDetails
========================
*/
idSaveGameDetails::idSaveGameDetails()
{
	Clear();
}

/*
========================
idSaveGameDetails::Clear
========================
*/
void idSaveGameDetails::Clear()
{
	descriptors.Clear();
	damaged = false;
	date = 0;
	slotName[0] = '\0';
}

/*
========================
idSaveLoadParms::idSaveLoadParms
========================
*/
idSaveLoadParms::idSaveLoadParms()
{
	// These are not done when we set defaults because SetDefaults is called internally within the execution of the processor and
	// these are set once and shouldn't be touched until the processor is re-initialized
	cancelled = false;

	Init();
}

/*
========================
idSaveLoadParms::~idSaveLoadParms
========================
*/
idSaveLoadParms::~idSaveLoadParms()
{
	for( int i = 0; i < files.Num(); ++i )
	{
		if( files[i]->type & SAVEGAMEFILE_AUTO_DELETE )
		{
			delete files[i];
		}
	}
}

/*
========================
idSaveLoadParms::ResetCancelled
========================
*/
void idSaveLoadParms::ResetCancelled()
{
	cancelled = false;
}

/*
========================
idSaveLoadParms::Init

This should not touch anything statically created outside this class!
========================
*/
void idSaveLoadParms::Init()
{
	files.Clear();
	mode = SAVEGAME_MBF_NONE;
	directory = "";
	pattern = "";
	postPattern = "";
	requiredSpaceInBytes = 0;
	description.Clear();
	detailList.Clear();
	callbackSignal.Clear();
	errorCode = SAVEGAME_E_NONE;
	inputDeviceId = -1;
	skipErrorDialogMask = 0;

	// These are not done when we set defaults because SetDefaults is called internally within the execution of the processor and
	// these are set once and shouldn't be touched until the processor is re-initialized
	// cancelled = false;
}

/*
========================
idSaveLoadParms::SetDefaults
========================
*/
void idSaveLoadParms::SetDefaults( int newInputDevice )
{
	// These are pulled out so SetDefaults() isn't called during global instantiation of objects that have savegame processors
	// in them that then require a session reference.
	Init();

	// fill in the user information (inputDeviceId & userId) from the master user
	idLocalUser* user = NULL;

	if( newInputDevice != -1 )
	{
		user = session->GetSignInManager().GetLocalUserByInputDevice( newInputDevice );
	}
	else if( session != NULL )
	{
		user = session->GetSignInManager().GetMasterLocalUser();
	}

	if( user != NULL )
	{
		idLocalUserWin* userWin = static_cast< idLocalUserWin* >( user );
		userId = idStr::Hash( userWin->GetGamerTag() );
		idLib::PrintfIf( saveGame_verbose.GetBool(), "profile userId/gamertag: %s (%d)\n", userWin->GetGamerTag(), userId );
		inputDeviceId = user->GetInputDevice();
	}
}

/*
========================
idSaveLoadParms::CancelSaveGameFilePipelines
========================
*/
void idSaveLoadParms::CancelSaveGameFilePipelines()
{
	for( int i = 0; i < files.Num(); i++ )
	{
		if( ( files[i]->type & SAVEGAMEFILE_PIPELINED ) != 0 )
		{
			idFile_SaveGamePipelined* file = dynamic_cast< idFile_SaveGamePipelined* >( files[i] );
			assert( file != NULL );

			if( file->GetMode() == idFile_SaveGamePipelined::WRITE )
			{
				// Notify the save game file that all writes failed which will cause all
				// writes on the other end of the pipeline to drop on the floor.
				file->NextWriteBlock( NULL );
			}
			else if( file->GetMode() == idFile_SaveGamePipelined::READ )
			{
				// Notify end-of-file to the save game file which will cause all
				// reads on the other end of the pipeline to return zero bytes.
				file->NextReadBlock( NULL, 0 );
			}
		}
	}
}

/*
========================
idSaveLoadParms::AbortSaveGameFilePipeline
========================
*/
void idSaveLoadParms::AbortSaveGameFilePipeline()
{
	for( int i = 0; i < files.Num(); i++ )
	{
		if( ( files[i]->type & SAVEGAMEFILE_PIPELINED ) != 0 )
		{
			idFile_SaveGamePipelined* file = dynamic_cast< idFile_SaveGamePipelined* >( files[i] );
			assert( file != NULL );
			file->Abort();
		}
	}
}

/*
================================================================================================
idSaveGameProcessor
================================================================================================
*/

/*
========================
idSaveGameProcessor::idSaveGameProcessor
========================
*/
idSaveGameProcessor::idSaveGameProcessor() : init( false ), working( false )
{
}

/*
========================
idSaveGameProcessor::Init
========================
*/
bool idSaveGameProcessor::Init()
{
	if( !verify( !IsWorking() ) )
	{
		idLib::Warning( "[%s] Someone is trying to execute this processor twice, this is really bad!", this->Name() );
		return false;
	}

	parms.ResetCancelled();
	parms.SetDefaults();
	savegameLogicTestIterator = 0;
	working = false;
	init = true;
	completedCallbacks.Clear();

	return true;
}

/*
========================
idSaveGameProcessor::IsThreadFinished
========================
*/
bool idSaveGameProcessor::IsThreadFinished()
{
	return parms.callbackSignal.Wait( 0 );
}

/*
========================
idSaveGameProcessor::AddCompletedCallback
========================
*/
void idSaveGameProcessor::AddCompletedCallback( const idCallback& callback )
{
	completedCallbacks.Append( callback.Clone() );
}

/*
================================================================================================
idSaveGameManager
================================================================================================
*/

/*
========================
idSaveGameManager::idSaveGameManager
========================
*/
idSaveGameManager::idSaveGameManager() :
	processor( NULL ),
	cancel( false ),
	startTime( 0 ),
	continueProcessing( false ),
	submittedProcessorHandle( 0 ),
	executingProcessorHandle( 0 ),
	lastExecutedProcessorHandle( 0 ),
	storageAvailable( true ),
	retryFolder( NULL )
{
}

/*
========================
idSaveGameManager::~idSaveGameManager
========================
*/
idSaveGameManager::~idSaveGameManager()
{
	processor = NULL;
	enumeratedSaveGames.Clear();
}

/*
========================
idSaveGameManager::ExecuteProcessor
========================
*/
saveGameHandle_t idSaveGameManager::ExecuteProcessor( idSaveGameProcessor* processor )
{
	idLib::PrintfIf( saveGame_verbose.GetBool(), "[%s] : %s\n", __FUNCTION__, processor->Name() );

	// may not be running yet, but if we've init'd successfuly, the IsWorking() call should return true if this
	// method has been called.  You have problems when callees are asking if the processor is done working by using IsWorking()
	// the next frame after they've executed the processor.
	processor->working = true;

	if( this->processor != NULL )
	{
		if( !verify( this->processor != processor ) )
		{
			idLib::Warning( "[idSaveGameManager::ExecuteProcessor]:1 Someone is trying to execute this processor twice, this is really bad, learn patience padawan!" );
			return processor->GetHandle();
		}
		else
		{
			idSaveGameProcessor** localProcessor = processorQueue.Find( processor );
			if( !verify( localProcessor == NULL ) )
			{
				idLib::Warning( "[idSaveGameManager::ExecuteProcessor]:2 Someone is trying to execute this processor twice, this is really bad, learn patience padawan!" );
				return ( *localProcessor )->GetHandle();
			}
		}
	}

	processorQueue.Append( processor );

	// Don't allow processors to start sub-processors.
	// They need to manage their own internal state.
	assert( idLib::IsMainThread() );

	Sys_InterlockedIncrement( submittedProcessorHandle );
	processor->parms.handle = submittedProcessorHandle;

	return submittedProcessorHandle;
}

/*
========================
idSaveGameManager::ExecuteProcessorAndWait
========================
*/
saveGameHandle_t idSaveGameManager::ExecuteProcessorAndWait( idSaveGameProcessor* processor )
{
	saveGameHandle_t handle = ExecuteProcessor( processor );
	if( handle == 0 )
	{
		return 0;
	}

	while( !IsSaveGameCompletedFromHandle( handle ) )
	{
		Pump();
		Sys_Sleep( 10 );
	}

	// One more pump to get the completed callback
	//Pump();

	return handle;
}

/*
========================
idSaveGameManager::WaitForAllProcessors

Since the load & nextMap processors calls execute map change, we can't wait if they are the only ones in the queue
If there are only resettable processors in the queue or no items in the queue, don't wait.

We would need to overrideSimpleProcessorCheck if we were sure we had done something that would cause the processors
to bail out nicely.  Something like canceling a disc swap during a loading disc swap dialog...
========================
*/
void idSaveGameManager::WaitForAllProcessors( bool overrideSimpleProcessorCheck )
{
	assert( idLib::IsMainThread() );

	while( IsWorking() || ( processorQueue.Num() > 0 ) )
	{

		if( !overrideSimpleProcessorCheck )
		{
			// BEFORE WE WAIT, and potentially hang everything, make sure processors about to be executed won't sit and
			// wait for themselves to complete.
			// Since we pull off simple processors first, we can stop waiting when the processor being executed is not simple
			if( processor != NULL )
			{
				if( !processor->IsSimpleProcessor() )
				{
					break;
				}
			}
			else if( !processorQueue[0]->IsSimpleProcessor() )
			{
				break;
			}
		}

		saveThread.WaitForThread();
		Pump();
	}
}

/*
========================
idSaveGameManager::CancelAllProcessors
========================
*/
void idSaveGameManager::CancelAllProcessors( const bool forceCancelInFlightProcessor )
{
	assert( idLib::IsMainThread() );

	cancel = true;

	if( forceCancelInFlightProcessor )
	{
		if( processor != NULL )
		{
			processor->GetSignal().Raise();
		}
	}

	Pump();	// must be called from the main thread
	Clear();
	cancel = false;
}

/*
========================
idSaveGameManager::CancelToTerminate
========================
*/
void idSaveGameManager::CancelToTerminate()
{
	if( processor != NULL )
	{
		processor->parms.cancelled = true;
		processor->GetSignal().Raise();
		saveThread.WaitForThread();
	}
}

/*
========================
idSaveGameManager::DeviceSelectorWaitingOnSaveRetry
========================
*/
bool idSaveGameManager::DeviceSelectorWaitingOnSaveRetry()
{

	if( retryFolder == NULL )
	{
		return false;
	}

	return ( idStr::Icmp( retryFolder, "GAME-autosave" ) == 0 );
}

/*
========================
idSaveGameManager::Set360RetrySaveAfterDeviceSelected
========================
*/
void idSaveGameManager::Set360RetrySaveAfterDeviceSelected( const char* folder, const int64 bytes )
{
	retryFolder = folder;
	retryBytes = bytes;
}

/*
========================
idSaveGameManager::ClearRetryInfo
========================
*/
void idSaveGameManager::ClearRetryInfo()
{
	retryFolder = NULL;
	retryBytes = 0;
}

/*
========================
idSaveGameManager::RetrySave
========================
*/
void idSaveGameManager::RetrySave()
{
	if( DeviceSelectorWaitingOnSaveRetry() && !common->Dialog().HasDialogMsg( GDM_WARNING_FOR_NEW_DEVICE_ABOUT_TO_LOSE_PROGRESS, NULL ) )
	{
		cmdSystem->AppendCommandText( "savegame autosave\n" );
	}
}

/*
========================
idSaveGameManager::ShowRetySaveDialog
========================
*/
void idSaveGameManager::ShowRetySaveDialog()
{
	ShowRetySaveDialog( retryFolder, retryBytes );
}

/*
========================
idSaveGameManager::ShowRetySaveDialog
========================
*/
void idSaveGameManager::ShowRetySaveDialog( const char* folder, const int64 bytes )
{

	idStaticList< idSWFScriptFunction*, 4 > callbacks;
	idStaticList< idStrId, 4 > optionText;

	class idSWFScriptFunction_Continue : public idSWFScriptFunction_RefCounted
	{
	public:
		idSWFScriptVar Call( idSWFScriptObject* thisObject, const idSWFParmList& parms )
		{
			common->Dialog().ClearDialog( GDM_INSUFFICENT_STORAGE_SPACE );
			session->GetSaveGameManager().ClearRetryInfo();
			return idSWFScriptVar();
		}
	};

	callbacks.Append( new( TAG_SWF ) idSWFScriptFunction_Continue() );
	optionText.Append( idStrId( "#str_dlg_continue_without_saving" ) );



	// build custom space required string
	// #str_dlg_space_required ~= "There is insufficient storage available.  Please free %s and try again."
	idStr format = idStrId( "#str_dlg_space_required" ).GetLocalizedString();
	idStr size;
	if( bytes > ( 1024 * 1024 ) )
	{
		const float roundUp = ( ( 1024.0f * 1024.0f / 10.0f ) - 1.0f );
		size = va( "%.1f MB", ( roundUp + ( float ) bytes ) / ( 1024.0f * 1024.0f ) );
	}
	else
	{
		const float roundUp = 1024.0f - 1.0f;
		size = va( "%.0f KB", ( roundUp + ( float ) bytes ) / 1024.0f );
	}
	idStr msg = va( format.c_str(), size.c_str() );

	common->Dialog().AddDynamicDialog( GDM_INSUFFICENT_STORAGE_SPACE, callbacks, optionText, true, msg, true );
}

/*
========================
idSaveGameManager::CancelWithHandle
========================
*/
void idSaveGameManager::CancelWithHandle( const saveGameHandle_t& handle )
{
	if( handle == 0 || IsSaveGameCompletedFromHandle( handle ) )
	{
		return;
	}

	// check processor in flight first
	if( processor != NULL )
	{
		if( processor->GetHandle() == handle )
		{
			processor->Cancel();
			return;
		}
	}

	// remove from queue
	for( int i = 0; i < processorQueue.Num(); ++i )
	{
		if( processorQueue[i]->GetHandle() == handle )
		{
			processorQueue[i]->Cancel();
			return;
		}
	}
}

/*
========================
idSaveGameManager::StartNextProcessor

Get the next not-reset-capable processor.  If there aren't any left, just get what's next.
========================
*/
void idSaveGameManager::StartNextProcessor()
{
	if( cancel )
	{
		return;
	}

	idSaveGameProcessor* nextProcessor = NULL;
	int index = 0;

	// pick off the first simple processor
	for( int i = 0; i < processorQueue.Num(); ++i )
	{
		if( processorQueue[i]->IsSimpleProcessor() )
		{
			index = i;
			break;
		}
	}

	if( processorQueue.Num() > 0 )
	{
		nextProcessor = processorQueue[index];


		Sys_InterlockedIncrement( executingProcessorHandle );

		processorQueue.RemoveIndex( index );
		processor = nextProcessor;
		processor->parms.callbackSignal.Raise();	// signal that the thread is ready for work
		startTime = Sys_Milliseconds();
	}
}

/*
========================
idSaveGameManager::FinishProcessor
========================
*/
void idSaveGameManager::FinishProcessor( idSaveGameProcessor* localProcessor )
{

	assert( localProcessor != NULL );
	idLib::PrintfIf( saveGame_verbose.GetBool(), "[%s] : %s, %d ms\n", __FUNCTION__, localProcessor->Name(), Sys_Milliseconds() - startTime );

	// This will delete from the files set for auto-deletion
	// Don't remove files not set for auto-deletion, they may be used outside of the savegame manager by game-side callbacks for example
	for( int i = ( localProcessor->parms.files.Num() - 1 ); i >= 0; --i )
	{
		if( localProcessor->parms.files[i]->type & SAVEGAMEFILE_AUTO_DELETE )
		{
			delete localProcessor->parms.files[i];
			localProcessor->parms.files.RemoveIndexFast( i );
		}
	}

	localProcessor->init = false;
	localProcessor = NULL;
}

/*
========================
idSaveGameManager::Clear
========================
*/
void idSaveGameManager::Clear()
{
	processorQueue.Clear();
}

/*
========================
idSaveGameManager::IsWorking
========================
*/
bool idSaveGameManager::IsWorking() const
{
	return processor != NULL;
}

/*
========================
idSaveGameManager::Pump

Important sections called out with -- EXTRA LARGE -- comments!
========================
*/
void idSaveGameManager::Pump()
{



	// After a processor is done, the next is pulled off the queue so the only way the manager isn't working is if
	// there isn't something executing or in the queue.
	if( !IsWorking() )
	{
		// Unified start to initialize system on PS3 and do appropriate checks for system combination issues
		// ------------------------------------
		// START
		// ------------------------------------
		StartNextProcessor();

		if( !IsWorking() )
		{
			return;
		}

		continueProcessing = true;
	}

	if( cancel )
	{
		processor->parms.AbortSaveGameFilePipeline();
	}

	// Quickly checks to see if the savegame thread is done, otherwise, exit and continue frame commands
	if( processor->IsThreadFinished() )
	{
		idLib::PrintfIf( saveGame_verbose.GetBool(), "%s waited on processor [%s], error = 0x%08X, %s\n", __FUNCTION__, processor->Name(), processor->GetError(), GetSaveGameErrorString( processor->GetError() ).c_str() );

		if( !cancel && continueProcessing )
		{
			// Check for available storage unit
			if( session->GetSignInManager().GetMasterLocalUser() != NULL )
			{
				if( !session->GetSignInManager().GetMasterLocalUser()->IsStorageDeviceAvailable() )
				{
					// this will not allow further processing
					processor->parms.errorCode = SAVEGAME_E_UNABLE_TO_SELECT_STORAGE_DEVICE;
				}
			}

			// Execute Process() on the processor, if there was an error in a previous Process() call, give the
			// processor the chance to validate that error and either clean itself up or convert it to another error or none.
			if( processor->GetError() == SAVEGAME_E_NONE || processor->ValidateLastError() )
			{
				idLib::PrintfIf( saveGame_verbose.GetBool(), "%s calling %s::Process(), error = 0x%08X, %s\n", __FUNCTION__, processor->Name(), processor->GetError(), GetSaveGameErrorString( processor->GetError() ).c_str() );

				// ------------------------------------
				// PROCESS
				// ------------------------------------
				continueProcessing = processor->Process();

				// If we don't return here, the completedCallback will be executed before it's done with it's async operation
				// during it's last process stage.
				return;
			}
			else
			{
				continueProcessing = false;
			}
		}

		// This section does specific post-processing for each of the save commands
		if( !continueProcessing )
		{

			// Clear out details if we detect corruption but keep directory/slot information
			for( int i = 0; i < processor->parms.detailList.Num(); ++i )
			{
				idSaveGameDetails& details = processor->parms.detailList[i];
				if( details.damaged )
				{
					details.descriptors.Clear();
				}
			}

			idLib::PrintfIf( saveGame_verbose.GetBool(), "%s calling %s::CompletedCallback()\n", __FUNCTION__, processor->Name() );
			processor->working = false;

			// This ensures that the savegame manager will believe the processor is done when there is a potentially
			// catastrophic thing that will happen within CompletedCallback which might try to sync all threads
			// The most common case of this is executing a map change (which we no longer do).
			// We flush the heap and wait for all background processes to finish.  After all this is called, we will
			// cleanup the old processor within FinishProcessor()
			idSaveGameProcessor* localProcessor = processor;
			processor = NULL;

			// ------------------------------------
			// COMPLETEDCALLBACK
			// At this point, the handle will be completed
			// ------------------------------------
			Sys_InterlockedIncrement( lastExecutedProcessorHandle );

			for( int i = 0; i < localProcessor->completedCallbacks.Num(); i++ )
			{
				localProcessor->completedCallbacks[i]->Call();
			}
			localProcessor->completedCallbacks.DeleteContents( true );

			// ------------------------------------
			// FINISHPROCESSOR
			// ------------------------------------
			FinishProcessor( localProcessor );
		}
	}
	else if( processor->ShouldTimeout() )
	{
		// Hack for the PS3 threading hang
		idLib::PrintfIf( saveGame_verbose.GetBool(), "----- PROCESSOR TIMEOUT ----- (%s)\n", processor->Name() );

		idSaveGameProcessor* tempProcessor = processor;

		CancelAllProcessors( true );

		class idSWFScriptFunction_TryAgain : public idSWFScriptFunction_RefCounted
		{
		public:
			idSWFScriptFunction_TryAgain( idSaveGameManager* manager, idSaveGameProcessor* processor )
			{
				this->manager = manager;
				this->processor = processor;
			}
			idSWFScriptVar Call( idSWFScriptObject* thisObject, const idSWFParmList& parms )
			{
				common->Dialog().ClearDialog( GDM_ERROR_SAVING_SAVEGAME );
				manager->ExecuteProcessor( processor );
				return idSWFScriptVar();
			}
		private:
			idSaveGameManager* manager;
			idSaveGameProcessor* processor;
		};

		idStaticList< idSWFScriptFunction*, 4 > callbacks;
		idStaticList< idStrId, 4 > optionText;
		callbacks.Append( new( TAG_SWF ) idSWFScriptFunction_TryAgain( this, tempProcessor ) );
		optionText.Append( idStrId( "#STR_SWF_RETRY" ) );
		common->Dialog().AddDynamicDialog( GDM_ERROR_SAVING_SAVEGAME, callbacks, optionText, true, "" );
	}
}
