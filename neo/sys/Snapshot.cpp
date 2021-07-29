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

idCVar net_verboseSnapshot( "net_verboseSnapshot", "0", CVAR_INTEGER | CVAR_NOCHEAT, "Verbose snapshot code to help debug snapshot problems. Greater the number greater the spam" );
idCVar net_verboseSnapshotCompression( "net_verboseSnapshotCompression", "0", CVAR_INTEGER | CVAR_NOCHEAT, "Verbose snapshot code to help debug snapshot problems. Greater the number greater the spam" );
idCVar net_verboseSnapshotReport( "net_verboseSnapshotReport", "0", CVAR_INTEGER | CVAR_NOCHEAT, "Verbose snapshot code to help debug snapshot problems. Greater the number greater the spam" );

idCVar net_ssTemplateDebug( "net_ssTemplateDebug", "0", CVAR_BOOL, "Debug snapshot template states" );
idCVar net_ssTemplateDebug_len( "net_ssTemplateDebug_len", "32", CVAR_INTEGER, "Offset to start template state debugging" );
idCVar net_ssTemplateDebug_start( "net_ssTemplateDebug_start", "0", CVAR_INTEGER, "length of template state to print in debugging" );

/*
========================
InDebugRange
Helper function for net_ssTemplateDebug debugging
========================
*/
bool InDebugRange( int i )
{
	return ( i >= net_ssTemplateDebug_start.GetInteger() && i < net_ssTemplateDebug_start.GetInteger() + net_ssTemplateDebug_len.GetInteger() );
}
/*
========================
PrintAlign
Helper function for net_ssTemplateDebug debugging
========================
*/
void PrintAlign( const char* text )
{
	idLib::Printf( "%25s: 0x", text );
}

/*
========================
idSnapShot::objectState_t::Print
Helper function for net_ssTemplateDebug debugging
========================
*/
void idSnapShot::objectState_t::Print( const char* name )
{

	unsigned int start = ( unsigned int )net_ssTemplateDebug_start.GetInteger();
	unsigned int end = Min( ( unsigned int )buffer.Size(), start + net_ssTemplateDebug_len.GetInteger() );

	PrintAlign( va( "%s: [sz %d]", name, buffer.Size() ) );

	for( unsigned int i = start; i < end; i++ )
	{
		idLib::Printf( "%02X", buffer[i] );
	}
	idLib::Printf( "\n" );
}

/*
========================
idSnapShot::objectBuffer_t::Alloc
========================
*/
void idSnapShot::objectBuffer_t::Alloc( int s )
{
	//assert( mem.IsMapHeap() );
	if( !verify( s < SIZE_NOT_STALE ) )
	{
		idLib::FatalError( "s >= SIZE_NOT_STALE" );
	}
	_Release();
	data = ( byte* )Mem_Alloc( s + 1, TAG_NETWORKING );
	size = s;
	data[size] = 1;
}

/*
========================
idSnapShot::objectBuffer_t::AddRef
========================
*/
void idSnapShot::objectBuffer_t::_AddRef()
{
	if( data != NULL )
	{
		assert( size > 0 );
		assert( data[size] < 255 );
		data[size]++;
	}
}

/*
========================
idSnapShot::objectBuffer_t::Release
========================
*/
void idSnapShot::objectBuffer_t::_Release()
{
	//assert( mem.IsMapHeap() );
	if( data != NULL )
	{
		assert( size > 0 );
		if( --data[size] == 0 )
		{
			Mem_Free( data );
		}
		data = NULL;
		size = 0;
	}
}

/*
========================
idSnapShot::objectBuffer_t::operator=
========================
*/
void idSnapShot::objectBuffer_t::operator=( const idSnapShot::objectBuffer_t& other )
{
	//assert( mem.IsMapHeap() );
	if( this != &other )
	{
		_Release();
		data = other.data;
		size = other.size;
		_AddRef();
	}
}

/*
========================
idSnapShot::idSnapShot
========================
*/
idSnapShot::idSnapShot() :
	time( 0 ),
	recvTime( 0 )
{
}

/*
========================
idSnapShot::idSnapShot
========================
*/
idSnapShot::idSnapShot( const idSnapShot& other ) : time( 0 ), recvTime( 0 )
{
	*this = other;
}

/*
========================
idSnapShot::~idSnapShot
========================
*/
idSnapShot::~idSnapShot()
{
	Clear();
}

/*
========================
idSnapShot::Clear
========================
*/
void idSnapShot::Clear()
{
	time = 0;
	recvTime = 0;
	for( int i = 0; i < objectStates.Num(); i++ )
	{
		FreeObjectState( i );
	}
	objectStates.Clear();
	allocatedObjs.Shutdown();
}

/*
========================
idSnapShot::operator=
========================
*/
void idSnapShot::operator=( const idSnapShot& other )
{
	//assert( mem.IsMapHeap() );

	if( this != &other )
	{
		for( int i = other.objectStates.Num(); i < objectStates.Num(); i++ )
		{
			FreeObjectState( i );
		}
		objectStates.AssureSize( other.objectStates.Num(), NULL );
		for( int i = 0; i < objectStates.Num(); i++ )
		{
			const objectState_t& otherState = *other.objectStates[i];
			if( objectStates[i] == NULL )
			{
				objectStates[i] = allocatedObjs.Alloc();
			}
			objectState_t& state = *objectStates[i];
			state.objectNum		= otherState.objectNum;
			state.buffer		= otherState.buffer;
			state.visMask		= otherState.visMask;
			state.stale			= otherState.stale;
			state.deleted		= otherState.deleted;
			state.changedCount	= otherState.changedCount;
			state.expectedSequence = otherState.expectedSequence;
			state.createdFromTemplate = otherState.createdFromTemplate;
		}
		time = other.time;
		recvTime = other.recvTime;
	}
}

/*
========================
idSnapShot::PeekDeltaSequence
========================
*/
void idSnapShot::PeekDeltaSequence( const char* deltaMem, int deltaSize, int& sequence, int& baseSequence )
{
	lzwCompressionData_t	lzwData;
	idLZWCompressor			lzwCompressor( &lzwData );

	lzwCompressor.Start( ( uint8* )deltaMem, deltaSize );
	lzwCompressor.ReadAgnostic( sequence );
	lzwCompressor.ReadAgnostic( baseSequence );
}

/*
========================
idSnapShot::ReadDeltaForJob
========================
*/
bool idSnapShot::ReadDeltaForJob( const char* deltaMem, int deltaSize, int visIndex, idSnapShot* templateStates )
{

	bool report = net_verboseSnapshotReport.GetBool();
	net_verboseSnapshotReport.SetBool( false );

	lzwCompressionData_t		lzwData;
	idZeroRunLengthCompressor	rleCompressor;
	idLZWCompressor				lzwCompressor( &lzwData );
	int bytesRead = 0; // how many uncompressed bytes we read in. Used to figure out compression ratio

	lzwCompressor.Start( ( uint8* )deltaMem, deltaSize );

	// Skip past sequence and baseSequence
	int sequence		= 0;
	int baseSequence	= 0;

	lzwCompressor.ReadAgnostic( sequence );
	lzwCompressor.ReadAgnostic( baseSequence );
	lzwCompressor.ReadAgnostic( time );
	bytesRead += sizeof( int ) * 3;

	int objectNum = 0;
	uint16 delta = 0;


	while( lzwCompressor.ReadAgnostic( delta, true ) == sizeof( delta ) )
	{
		bytesRead += sizeof( delta );

		objectNum += delta;
		if( objectNum >= 0xFFFF )
		{
			// full delta
			if( net_verboseSnapshotCompression.GetBool() )
			{
				float compRatio = static_cast<float>( deltaSize ) / static_cast<float>( bytesRead );
				idLib::Printf( "Snapshot (%d/%d). ReadSize: %d DeltaSize: %d Ratio: %.3f\n", sequence, baseSequence, bytesRead, deltaSize, compRatio );
			}
			return true;
		}

		objectState_t& state = FindOrCreateObjectByID( objectNum );

		objectSize_t newsize = 0;
		lzwCompressor.ReadAgnostic( newsize );
		bytesRead += sizeof( newsize );

		if( newsize == SIZE_STALE )
		{
			NET_VERBOSESNAPSHOT_PRINT( "read delta: object %d goes stale\n", objectNum );
			// sanity
			bool oldVisible = ( state.visMask & ( 1 << visIndex ) ) != 0;
			if( !oldVisible )
			{
				NET_VERBOSESNAPSHOT_PRINT( "ERROR: unexpected already stale\n" );
			}
			state.visMask &= ~( 1 << visIndex );
			state.stale = true;
			// We need to make sure we haven't freed stale objects.
			assert( state.buffer.Size() > 0 );
			// no more data
			continue;
		}
		else if( newsize == SIZE_NOT_STALE )
		{
			NET_VERBOSESNAPSHOT_PRINT( "read delta: object %d no longer stale\n", objectNum );
			// sanity
			bool oldVisible = ( state.visMask & ( 1 << visIndex ) ) != 0;
			if( oldVisible )
			{
				NET_VERBOSESNAPSHOT_PRINT( "ERROR: unexpected not stale\n" );
			}
			state.visMask |= ( 1 << visIndex );
			state.stale = false;
			// the latest state is packed in, get the new size and continue reading the new state
			lzwCompressor.ReadAgnostic( newsize );
			bytesRead += sizeof( newsize );
		}

		objectState_t* 	objTemplateState = templateStates->FindObjectByID( objectNum );

		if( newsize == 0 )
		{
			// object deleted: reset state now so next one to use it doesn't have old data
			state.deleted = false;
			state.stale = false;
			state.changedCount = 0;
			state.expectedSequence = 0;
			state.visMask = 0;
			state.buffer._Release();
			state.createdFromTemplate = false;

			if( objTemplateState != NULL && objTemplateState->buffer.Size() && objTemplateState->expectedSequence < baseSequence )
			{
				idLib::PrintfIf( net_ssTemplateDebug.GetBool(), "Clearing old template state[%d] [%d<%d]\n", objectNum, objTemplateState->expectedSequence, baseSequence );
				objTemplateState->deleted = false;
				objTemplateState->stale = false;
				objTemplateState->changedCount = 0;
				objTemplateState->expectedSequence = 0;
				objTemplateState->visMask = 0;
				objTemplateState->buffer._Release();
			}

		}
		else
		{

			// new state?
			bool debug = false;
			if( state.buffer.Size() == 0 )
			{
				state.createdFromTemplate = true;
				// Brand new state
				if( objTemplateState != NULL && objTemplateState->buffer.Size() > 0 && sequence >= objTemplateState->expectedSequence )
				{
					idLib::PrintfIf( net_ssTemplateDebug.GetBool(), "\nAdding basestate for new object %d (for SS %d/%d. obj base created in ss %d) deltaSize: %d\n", objectNum, sequence, baseSequence, objTemplateState->expectedSequence, deltaSize );
					state.buffer = objTemplateState->buffer;

					if( net_ssTemplateDebug.GetBool() )
					{
						state.Print( "SPAWN STATE" );
						debug = true;
						PrintAlign( "DELTA STATE" );
					}
				}
				else if( net_ssTemplateDebug.GetBool() )
				{
					idLib::Printf( "\nNew snapobject[%d] in snapshot %d/%d but no basestate found locally so creating new\n", objectNum, sequence, baseSequence );
				}
			}
			else
			{
				state.createdFromTemplate = false;
			}

			// the buffer shrank or stayed the same
			objectBuffer_t newbuffer( newsize );
			rleCompressor.Start( NULL, &lzwCompressor, newsize );
			objectSize_t compareSize = Min( state.buffer.Size(), newsize );
			for( objectSize_t i = 0; i < compareSize; i++ )
			{
				byte b = rleCompressor.ReadByte();
				newbuffer[i] = state.buffer[i] + b;

				if( debug && InDebugRange( i ) )
				{
					idLib::Printf( "%02X", b );
				}
			}
			// Catch leftover
			if( newsize > compareSize )
			{
				rleCompressor.ReadBytes( newbuffer.Ptr() + compareSize, newsize - compareSize );

				if( debug )
				{
					for( objectSize_t i = compareSize; i < newsize; i++ )
					{
						if( InDebugRange( i ) )
						{
							idLib::Printf( "%02X", newbuffer[i] );
						}
					}
				}

			}
			state.buffer = newbuffer;
			state.changedCount = sequence;
			bytesRead += sizeof( byte ) * newsize;
			if( debug )
			{
				idLib::Printf( "\n" );
				state.Print( "NEW STATE" );
			}

			if( report )
			{
				idLib::Printf( "    Obj %d Compressed: Size %d \n", objectNum, rleCompressor.CompressedSize() );
			}
		}
#ifdef SNAPSHOT_CHECKSUMS
		extern uint32 SnapObjChecksum( const uint8 * data, int length );
		if( state.buffer.Size() > 0 )
		{
			uint32 checksum = 0;
			lzwCompressor.ReadAgnostic( checksum );
			bytesRead += sizeof( checksum );
			if( !verify( checksum == SnapObjChecksum( state.buffer.Ptr(), state.buffer.Size() ) ) )
			{
				idLib::Error( " Invalid snapshot checksum" );
			}
		}
#endif
	}
	// partial delta
	return false;
}

/*
========================
idSnapShot::SubmitObjectJob
========================
*/

void idSnapShot::SubmitObjectJob(	const submitDeltaJobsInfo_t& 	submitDeltaJobsInfo,
									objectState_t* 					newState,
									objectState_t* 					oldState,
									objParms_t*&					baseObjParm,
									objParms_t*&					curObjParm,
									objHeader_t*&					curHeader,
									uint8*&						curObjDest,
									lzwParm_t*&					curlzwParm
								)
{
	assert( newState != NULL || oldState != NULL );
	assert_16_byte_aligned( curHeader );
	assert_16_byte_aligned( curObjDest );

	int32 dataSize = newState != NULL ? newState->buffer.Size() : 0;
	int totalSize = OBJ_DEST_SIZE_ALIGN16( dataSize );

	if( curObjParm - submitDeltaJobsInfo.objParms >= submitDeltaJobsInfo.maxObjParms )
	{
		idLib::Error( "Out of parms for snapshot jobs.\n" );
	}

	// Check to see if we are out of dest write space, and need to flush the jobs
	bool needToSubmit = ( curObjDest - submitDeltaJobsInfo.objMemory ) + totalSize >= submitDeltaJobsInfo.maxObjMemory;
	needToSubmit |= ( curHeader - submitDeltaJobsInfo.headers >= submitDeltaJobsInfo.maxHeaders );

	if( needToSubmit )
	{
		// If this obj will put us over the limit, then submit the jobs now, and start over re-using the same buffers
		SubmitLZWJob( submitDeltaJobsInfo, baseObjParm, curObjParm, curlzwParm, true );
		curHeader	= submitDeltaJobsInfo.headers;
		curObjDest	= submitDeltaJobsInfo.objMemory;
	}

	// Setup obj parms
	assert( submitDeltaJobsInfo.visIndex < 256 );
	curObjParm->visIndex	= submitDeltaJobsInfo.visIndex;
	curObjParm->destHeader	= curHeader;
	curObjParm->dest		= curObjDest;

	memset( &curObjParm->newState, 0, sizeof( curObjParm->newState ) );
	memset( &curObjParm->oldState, 0, sizeof( curObjParm->oldState ) );

	if( newState != NULL )
	{
		assert( newState->buffer.Size() <= 65535 );

		curObjParm->newState.valid		= 1;
		curObjParm->newState.data		= newState->buffer.Ptr();
		curObjParm->newState.size		= newState->buffer.Size();
		curObjParm->newState.objectNum	= newState->objectNum;
		curObjParm->newState.visMask	= newState->visMask;
	}

	if( oldState != NULL )
	{
		assert( oldState->buffer.Size() <= 65535 );

		curObjParm->oldState.valid		= 1;
		curObjParm->oldState.data		= oldState->buffer.Ptr();
		curObjParm->oldState.size		= oldState->buffer.Size();
		curObjParm->oldState.objectNum	= oldState->objectNum;
		curObjParm->oldState.visMask	= oldState->visMask;
	}

	assert_16_byte_aligned( curObjParm );
	assert_16_byte_aligned( curObjParm->newState.data );
	assert_16_byte_aligned( curObjParm->oldState.data );

	SnapshotObjectJob( curObjParm );

	// Advance past header + data
	curObjDest += totalSize;

	// Advance parm pointer
	curObjParm++;

	// Advance header pointer
	curHeader++;
}

/*
========================
idSnapShot::SubmitLZWJob
Take the current list of delta'd + zlre processed objects, and write them to the lzw stream.
========================
*/
void idSnapShot::SubmitLZWJob(
	const submitDeltaJobsInfo_t& 	writeDeltaInfo,
	objParms_t*&					baseObjParm,		// Pointer to the first obj parm for the current stream
	objParms_t*&					curObjParm,			// Current obj parm
	lzwParm_t*&					curlzwParm,			// Current delta parm
	bool							saveDictionary
)
{
	int numObjects = curObjParm - baseObjParm;

	if( numObjects == 0 )
	{
		return;		// Nothing to do
	}

	if( curlzwParm - writeDeltaInfo.lzwParms >= writeDeltaInfo.maxDeltaParms )
	{
		idLib::Error( "SubmitLZWJob: Not enough lzwParams.\n" );
		return;		// Can't do anymore
	}

	curlzwParm->numObjects		= numObjects;
	curlzwParm->headers			= writeDeltaInfo.headers;		// We always start grabbing from the beggining of the memory (it's reused, with fences to protect memory sharing)
	curlzwParm->curTime			= this->GetTime();
	curlzwParm->baseTime		= writeDeltaInfo.oldSnap->GetTime();
	curlzwParm->baseSequence	= writeDeltaInfo.baseSequence;
	curlzwParm->fragmented		= ( curlzwParm != writeDeltaInfo.lzwParms );
	curlzwParm->saveDictionary	= saveDictionary;

	curlzwParm->ioData			= writeDeltaInfo.lzwInOutData;

	LZWJob( curlzwParm );

	curlzwParm++;

	// Set base so it now points to where the parms start for the new stream
	baseObjParm = curObjParm;
}

/*
========================
idSnapShot::GetTemplateState
Helper function for getting template objectState.
newState parameter is optional and is just used for debugging/printf comparison of the template and actual state
========================
*/
idSnapShot::objectState_t* idSnapShot::GetTemplateState( int objNum, idSnapShot* templateStates, idSnapShot::objectState_t* newState /*=NULL*/ )
{
	objectState_t* oldState = NULL;
	int spawnedStateIndex = templateStates->FindObjectIndexByID( objNum );
	if( spawnedStateIndex >= 0 )
	{
		oldState = templateStates->objectStates[ spawnedStateIndex ];

		if( net_ssTemplateDebug.GetBool() )
		{
			idLib::Printf( "\nGetTemplateState[%d]\n", objNum );
			oldState->Print( "SPAWN STATE" );
			if( newState != NULL )
			{
				newState->Print( "CUR STATE" );
			}
		}
	}
	return oldState;
}

/*
========================
idSnapShot::SubmitWriteDeltaToJobs
========================
*/
void idSnapShot::SubmitWriteDeltaToJobs( const submitDeltaJobsInfo_t& submitDeltaJobInfo )
{
	objParms_t* 	curObjParms		= submitDeltaJobInfo.objParms;
	objParms_t* 	baseObjParms	= submitDeltaJobInfo.objParms;
	lzwParm_t* 		curlzwParms		= submitDeltaJobInfo.lzwParms;
	objHeader_t* 	curHeader		= submitDeltaJobInfo.headers;
	uint8* 			curObjMemory	= submitDeltaJobInfo.objMemory;

	submitDeltaJobInfo.lzwInOutData->numlzwDeltas	= 0;
	submitDeltaJobInfo.lzwInOutData->lzwBytes		= 0;
	submitDeltaJobInfo.lzwInOutData->fullSnap		= false;

	int j = 0;

	int numOldStates = submitDeltaJobInfo.oldSnap->objectStates.Num();

	for( int i = 0; i < objectStates.Num(); i++ )
	{
		objectState_t& newState = *objectStates[i];
		if( !verify( newState.buffer.Size() > 0 ) )
		{
			// you CANNOT have a valid ss obj state w/ size = 0: this will be interpreted as a delete in ::ReadDelta and this will completely throw
			// off delta compression, eventually resulting in a checksum error that is a pain to track down.
			idLib::Warning( "Snap obj [%d] state.size <= 0... skipping ", newState.objectNum );
			continue;
		}

		if( j >= numOldStates )
		{
			// We no longer have old objects to compare to.
			// All objects are new from this point on.

			objectState_t* oldState = GetTemplateState( newState.objectNum, submitDeltaJobInfo.templateStates, &newState );
			SubmitObjectJob( submitDeltaJobInfo, &newState, oldState, baseObjParms, curObjParms, curHeader, curObjMemory, curlzwParms );
			continue;
		}

		// write any deleted entities up to this one
		for( ; j < numOldStates && newState.objectNum > submitDeltaJobInfo.oldSnap->objectStates[j]->objectNum; j++ )
		{
			objectState_t& oldState = *submitDeltaJobInfo.oldSnap->objectStates[j];

			if( ( oldState.stale && !oldState.deleted ) || oldState.buffer.Size() <= 0 )
			{
				continue;		// Don't delete objects that are stale and not marked as deleted
			}

			SubmitObjectJob( submitDeltaJobInfo, NULL, &oldState, baseObjParms, curObjParms, curHeader, curObjMemory, curlzwParms );
		}

		if( j >= numOldStates )
		{
			continue;	// Went past end of old list deleting objects
		}

		// Beyond this point, we may have old state to compare against
		objectState_t& submittedOldState = *submitDeltaJobInfo.oldSnap->objectStates[j];
		objectState_t* oldState = &submittedOldState;

		if( newState.objectNum == oldState->objectNum )
		{
			if( oldState->buffer.Size() == 0 )
			{
				// New state (even though snapObj existed, its size was zero)
				oldState = GetTemplateState( newState.objectNum, submitDeltaJobInfo.templateStates, &newState );
			}

			SubmitObjectJob( submitDeltaJobInfo, &newState, oldState, baseObjParms, curObjParms, curHeader, curObjMemory, curlzwParms );
			j++;
		}
		else
		{
			// Different object, this one is new,
			// Spawned
			oldState = GetTemplateState( newState.objectNum, submitDeltaJobInfo.templateStates, &newState );
			SubmitObjectJob( submitDeltaJobInfo, &newState, oldState, baseObjParms, curObjParms, curHeader, curObjMemory, curlzwParms );
		}
	}
	// Finally, remove any entities at the end
	for( ; j < submitDeltaJobInfo.oldSnap->objectStates.Num(); j++ )
	{
		objectState_t& oldState = *submitDeltaJobInfo.oldSnap->objectStates[j];

		if( ( oldState.stale && !oldState.deleted ) || oldState.buffer.Size() <= 0 )
		{
			continue;		// Don't delete objects that are stale and not marked as deleted
		}

		SubmitObjectJob( submitDeltaJobInfo, NULL, &oldState, baseObjParms, curObjParms, curHeader, curObjMemory, curlzwParms );
	}

	// Submit any objects that are left over (will be all if they all fit up to this point)
	SubmitLZWJob( submitDeltaJobInfo, baseObjParms, curObjParms, curlzwParms, false );
}

/*
========================
idSnapShot::ReadDelta
========================
*/
bool idSnapShot::ReadDelta( idFile* file, int visIndex )
{

	file->ReadBig( time );

	int objectNum = 0;
	uint16 delta = 0;
	while( file->ReadBig( delta ) == sizeof( delta ) )
	{
		objectNum += delta;
		if( objectNum >= 0xFFFF )
		{
			// full delta
			return true;
		}
		objectState_t& state = FindOrCreateObjectByID( objectNum );
		objectSize_t newsize = 0;
		file->ReadBig( newsize );

		if( newsize == SIZE_STALE )
		{
			NET_VERBOSESNAPSHOT_PRINT( "read delta: object %d goes stale\n", objectNum );
			// sanity
			bool oldVisible = ( state.visMask & ( 1 << visIndex ) ) != 0;
			if( !oldVisible )
			{
				NET_VERBOSESNAPSHOT_PRINT( "ERROR: unexpected already stale\n" );
			}
			state.visMask &= ~( 1 << visIndex );
			state.stale = true;
			// We need to make sure we haven't freed stale objects.
			assert( state.buffer.Size() > 0 );
			// no more data
			continue;
		}
		else if( newsize == SIZE_NOT_STALE )
		{
			NET_VERBOSESNAPSHOT_PRINT( "read delta: object %d no longer stale\n", objectNum );
			// sanity
			bool oldVisible = ( state.visMask & ( 1 << visIndex ) ) != 0;
			if( oldVisible )
			{
				NET_VERBOSESNAPSHOT_PRINT( "ERROR: unexpected not stale\n" );
			}
			state.visMask |= ( 1 << visIndex );
			state.stale = false;
			// the latest state is packed in, get the new size and continue reading the new state
			file->ReadBig( newsize );
		}

		if( newsize == 0 )
		{
			// object deleted
			state.buffer._Release();
		}
		else
		{
			objectBuffer_t newbuffer( newsize );
			objectSize_t compareSize = Min( newsize, state.buffer.Size() );

			for( objectSize_t i = 0; i < compareSize; i++ )
			{
				uint8 delta = 0;
				file->ReadBig<byte>( delta );
				newbuffer[i] = state.buffer[i] + delta;
			}

			if( newsize > compareSize )
			{
				file->Read( newbuffer.Ptr() + compareSize, newsize - compareSize );
			}

			state.buffer = newbuffer;
			state.changedCount++;
		}

#ifdef SNAPSHOT_CHECKSUMS
		if( state.buffer.Size() > 0 )
		{
			unsigned int checksum = 0;
			file->ReadBig( checksum );
			assert( checksum == MD5_BlockChecksum( state.buffer.Ptr(), state.buffer.Size() ) );
		}
#endif
	}

	// partial delta
	return false;
}

/*
========================
idSnapShot::WriteObject
========================
*/
void idSnapShot::WriteObject( idFile* file, int visIndex, objectState_t* newState, objectState_t* oldState, int& lastobjectNum )
{
	assert( newState != NULL || oldState != NULL );

	bool visChange		= false; // visibility changes will be signified with a 0xffff state size
	bool visSendState	= false; // the state is sent when an entity is no longer stale

	// Compute visibility changes
	// (we need to do this before writing out object id, because we may not need to write out the id if we early out)
	// (when we don't write out the id, we assume this is an "ack" when we deserialize the objects)
	if( newState != NULL && oldState != NULL )
	{
		// Check visibility
		assert( newState->objectNum == oldState->objectNum );

		if( visIndex > 0 )
		{
			bool oldVisible = ( oldState->visMask & ( 1 << visIndex ) ) != 0;
			bool newVisible = ( newState->visMask & ( 1 << visIndex ) ) != 0;

			// Force visible if we need to either create or destroy this object
			newVisible |= ( newState->buffer.Size() == 0 ) != ( oldState->buffer.Size() == 0 );

			if( !oldVisible && !newVisible )
			{
				// object is stale and ack'ed for this client, write nothing (see 'same object' below)
				return;
			}
			else if( oldVisible && !newVisible )
			{
				NET_VERBOSESNAPSHOT_PRINT( "object %d to client %d goes stale\n", newState->objectNum, visIndex );
				visChange = true;
				visSendState = false;
			}
			else if( !oldVisible && newVisible )
			{
				NET_VERBOSESNAPSHOT_PRINT( "object %d to client %d no longer stale\n", newState->objectNum, visIndex );
				visChange = true;
				visSendState = true;
			}
		}

		// Same object, write a delta (never early out during vis changes)
		if( !visChange && newState->buffer.Size() == oldState->buffer.Size() &&
				( ( newState->buffer.Ptr() == oldState->buffer.Ptr() ) || memcmp( newState->buffer.Ptr(), oldState->buffer.Ptr(), newState->buffer.Size() ) == 0 ) )
		{
			// same state, write nothing
			return;
		}
	}

	// Get the id of the object we are writing out
	uint16 objectNum;
	if( newState != NULL )
	{
		objectNum = newState->objectNum;
	}
	else if( oldState != NULL )
	{
		objectNum = oldState->objectNum;
	}
	else
	{
		objectNum = 0;
	}

	assert( objectNum == 0 || objectNum > lastobjectNum );

	// Write out object id (using delta)
	uint16 objectDelta = objectNum - lastobjectNum;
	file->WriteBig( objectDelta );
	lastobjectNum = objectNum;

	if( newState == NULL )
	{
		// Deleted, write 0 size
		assert( oldState != NULL );
		file->WriteBig<objectSize_t>( 0 );
	}
	else if( oldState == NULL )
	{
		// New object, write out full state
		assert( newState != NULL );
		// delta against an empty snap
		file->WriteBig( newState->buffer.Size() );
		file->Write( newState->buffer.Ptr(), newState->buffer.Size() );
	}
	else
	{
		// Compare to last object
		assert( newState != NULL && oldState != NULL );
		assert( newState->objectNum == oldState->objectNum );

		if( visChange )
		{
			// fake size indicates vis state change
			// NOTE: we may still send a real size and a state below, for 'no longer stale' transitions
			// TMP: send 0xFFFF for going stale and 0xFFFF - 1 for no longer stale
			file->WriteBig<objectSize_t>( visSendState ? SIZE_NOT_STALE : SIZE_STALE );
		}
		if( !visChange || visSendState )
		{

			objectSize_t compareSize = Min( newState->buffer.Size(), oldState->buffer.Size() );		// Get the number of bytes that overlap

			file->WriteBig( newState->buffer.Size() );										// Write new size

			// Compare bytes that overlap
			for( objectSize_t b = 0; b < compareSize; b++ )
			{
				file->WriteBig<byte>( ( 0xFF + 1 + newState->buffer[b] - oldState->buffer[b] ) & 0xFF );
			}

			// Write leftover
			if( newState->buffer.Size() > compareSize )
			{
				file->Write( newState->buffer.Ptr() + oldState->buffer.Size(), newState->buffer.Size() - compareSize );
			}
		}
	}

#ifdef SNAPSHOT_CHECKSUMS
	if( ( !visChange || visSendState ) && newState != NULL )
	{
		assert( newState->buffer.Size() > 0 );
		unsigned int checksum = MD5_BlockChecksum( newState->buffer.Ptr(), newState->buffer.Size() );
		file->WriteBig( checksum );
	}
#endif
}

/*
========================
idSnapShot::PrintReport
========================
*/
void idSnapShot::PrintReport()
{
}

/*
========================
idSnapShot::WriteDelta
========================
*/
bool idSnapShot::WriteDelta( idSnapShot& old, int visIndex, idFile* file, int maxLength, int optimalLength )
{
	file->WriteBig( time );

	int objectHeaderSize = sizeof( uint16 ) + sizeof( objectSize_t );
#ifdef SNAPSHOT_CHECKSUMS
	objectHeaderSize += sizeof( unsigned int );
#endif

	int lastobjectNum = 0;
	int j = 0;

	for( int i = 0; i < objectStates.Num(); i++ )
	{
		objectState_t& newState = *objectStates[i];

		if( optimalLength > 0 && file->Length() >= optimalLength )
		{
			return false;
		}

		if( !verify( newState.buffer.Size() < maxLength ) )
		{
			// If the new state's size is > the max packet size, we'll never be able to send it!
			idLib::Warning( "Snap obj [%d] state.size > max packet length. Skipping... ", newState.objectNum );
			continue;
		}
		else if( !verify( newState.buffer.Size() > 0 ) )
		{
			// you CANNOT have a valid ss obj state w/ size = 0: this will be interpreted as a delete in ::ReadDelta and this will completely throw
			// off delta compression, eventually resulting in a checksum error that is a pain to track down.
			idLib::Warning( "Snap obj [%d] state.size <= 0... skipping ", newState.objectNum );
			continue;
		}

		if( file->Length() + objectHeaderSize + newState.buffer.Size() >= maxLength )
		{
			return false;
		}

		if( j >= old.objectStates.Num() )
		{
			// delta against an empty snap
			WriteObject( file, visIndex, &newState, NULL, lastobjectNum );
			continue;
		}

		// write any deleted entities up to this one
		for( ; newState.objectNum > old.objectStates[j]->objectNum; j++ )
		{
			if( file->Length() + objectHeaderSize >= maxLength )
			{
				return false;
			}
			objectState_t& oldState = *old.objectStates[j];
			WriteObject( file, visIndex, NULL, &oldState, lastobjectNum );
		}

		// Beyond this point, we have old state to compare against
		objectState_t& oldState = *old.objectStates[j];

		if( newState.objectNum == oldState.objectNum )
		{
			// FIXME: We don't need to early out if WriteObject determines that we won't send the object due to being stale
			if( file->Length() + objectHeaderSize + newState.buffer.Size() >= maxLength )
			{
				return false;
			}
			WriteObject( file, visIndex, &newState, &oldState, lastobjectNum );
			j++;
		}
		else
		{
			if( file->Length() + objectHeaderSize + newState.buffer.Size() >= maxLength )
			{
				return false;
			}

			// Different object, this one is new, write the full state
			WriteObject( file, visIndex, &newState, NULL, lastobjectNum );
		}
	}
	// Finally, remove any entities at the end
	for( ; j < old.objectStates.Num(); j++ )
	{
		if( file->Length() + objectHeaderSize >= maxLength )
		{
			return false;
		}

		if( optimalLength > 0 && file->Length() >= optimalLength )
		{
			return false;
		}

		objectState_t& oldState = *old.objectStates[j];
		WriteObject( file, visIndex, NULL, &oldState, lastobjectNum );
	}
	if( file->Length() + 2 >= maxLength )
	{
		return false;
	}
	uint16 objectDelta = 0xFFFF - lastobjectNum;
	file->WriteBig( objectDelta );

	return true;
}

/*
========================
idSnapShot::AddObject
========================
*/
idSnapShot::objectState_t* idSnapShot::S_AddObject( int objectNum, uint32 visMask, const char* data, int _size, const char* tag )
{
	objectSize_t size = _size;
	objectState_t& state = FindOrCreateObjectByID( objectNum );
	state.visMask = visMask;
	if( state.buffer.Size() == size && state.buffer.NumRefs() == 1 )
	{
		// re-use the same buffer
		memcpy( state.buffer.Ptr(), data, size );
	}
	else
	{
		objectBuffer_t buffer( size );
		memcpy( buffer.Ptr(), data, size );
		state.buffer = buffer;
	}
	return &state;
}

/*
========================
idSnapShot::CopyObject
========================
*/

bool idSnapShot::CopyObject( const idSnapShot& oldss, int objectNum, bool forceStale )
{

	int oldIndex = oldss.FindObjectIndexByID( objectNum );
	if( oldIndex == -1 )
	{
		return false;
	}

	const objectState_t& oldState = *oldss.objectStates[oldIndex];
	objectState_t& newState = FindOrCreateObjectByID( objectNum );

	newState.buffer			= oldState.buffer;
	newState.visMask		= oldState.visMask;
	newState.stale			= oldState.stale;
	newState.deleted		= oldState.deleted;
	newState.changedCount	= oldState.changedCount;
	newState.expectedSequence = oldState.expectedSequence;
	newState.createdFromTemplate = oldState.createdFromTemplate;

	if( forceStale )
	{
		newState.visMask = 0;
	}

	return true;
}

/*
========================
idSnapShot::CompareObject
start, end, and oldStart can optionally be passed in to compare subsections of the object
default parameters will compare entire object
========================
*/
int idSnapShot::CompareObject( const idSnapShot* oldss, int objectNum, int start, int end, int oldStart )
{
	if( oldss == NULL )
	{
		return 0;
	}

	assert( FindObjectIndexByID( objectNum ) >= 0 );

	objectState_t& newState = FindOrCreateObjectByID( objectNum );

	int oldIndex = oldss->FindObjectIndexByID( objectNum );
	if( oldIndex == -1 )
	{
		return ( end == 0 ? newState.buffer.Size() : end - start );	// Didn't exist in the old state, so we take the hit on the entire size
	}

	objectState_t& oldState = const_cast< objectState_t& >( *oldss->objectStates[oldIndex] );

	int bytes = 0;

	int oldOffset = oldStart - start;
	int commonSize = ( newState.buffer.Size() <= oldState.buffer.Size() - oldOffset ) ? newState.buffer.Size() : oldState.buffer.Size() - oldOffset;
	if( end == 0 )
	{
		// default 0 means compare the whole thing
		end = commonSize;

		// Get leftover (if any)
		bytes = ( newState.buffer.Size() > oldState.buffer.Size() ) ? ( newState.buffer.Size() - oldState.buffer.Size() ) : 0;
	}
	else
	{

		// else only compare up to end or the max buffer and dont include leftover
		end = Min( commonSize, end );
	}

	for( int b = start; b < end; b++ )
	{
		if( verify( b >= 0 && b < ( int )newState.buffer.Size() && b + oldOffset >= 0 && b + oldOffset < ( int )oldState.buffer.Size() ) )
		{
			bytes += ( newState.buffer[b] != oldState.buffer[b + oldOffset] ) ? 1 : 0;
		}
	}

	return bytes;
}

/*
========================
idSnapShot::GetObjectMsgByIndex
========================
*/
int idSnapShot::GetObjectMsgByIndex( int i, idBitMsg& msg, bool ignoreIfStale ) const
{
	if( i < 0 || i >= objectStates.Num() )
	{
		return -1;
	}
	objectState_t& state = *objectStates[i];
	if( state.stale && ignoreIfStale )
	{
		return -1;
	}
	msg.InitRead( state.buffer.Ptr(), state.buffer.Size() );
	return state.objectNum;
}

/*
========================
idSnapShot::ObjectIsStaleByIndex
========================
*/
bool idSnapShot::ObjectIsStaleByIndex( int i ) const
{
	if( i < 0 || i >= objectStates.Num() )
	{
		return false;
	}
	return objectStates[i]->stale;
}

/*
========================
idSnapShot::ObjectChangedCountByIndex
========================
*/
int idSnapShot::ObjectChangedCountByIndex( int i ) const
{
	if( i < 0 || i >= objectStates.Num() )
	{
		return false;
	}
	return objectStates[i]->changedCount;
}

/*
========================
idSnapShot::FindObjectIndexByID
========================
*/
int idSnapShot::FindObjectIndexByID( int objectNum ) const
{
	int i = BinarySearch( objectNum );
	if( i >= 0 && i < objectStates.Num() && objectStates[i]->objectNum == objectNum )
	{
		return i;
	}
	return -1;
}

/*
========================
idSnapShot::BinarySearch
========================
*/
int idSnapShot::BinarySearch( int objectNum ) const
{
	int lo = 0;
	int hi = objectStates.Num();
	while( hi != lo )
	{
		int mid = ( hi + lo ) >> 1;

		if( objectStates[mid]->objectNum == objectNum )
		{
			return mid;		// Early out if we can
		}

		if( objectStates[mid]->objectNum < objectNum )
		{
			lo = mid + 1;
		}
		else
		{
			hi = mid;
		}
	}
	return hi;
}

/*
========================
idSnapShot::FindOrCreateObjectByID
========================
*/
idSnapShot::objectState_t& idSnapShot::FindOrCreateObjectByID( int objectNum )
{
	//assert( mem.IsMapHeap() );

	int i = BinarySearch( objectNum );

	if( i >= 0 && i < objectStates.Num() && objectStates[i]->objectNum == objectNum )
	{
		return *objectStates[i];
	}

	objectState_t* newstate = allocatedObjs.Alloc();
	newstate->objectNum = objectNum;

	objectStates.Insert( newstate, i );

	return *objectStates[i];
}

/*
========================
idSnapShot::FindObjectByID
========================
*/
idSnapShot::objectState_t* idSnapShot::FindObjectByID( int objectNum ) const
{

	//assert( mem.IsMapHeap() );

	int i = BinarySearch( objectNum );

	if( i >= 0 && i < objectStates.Num() && objectStates[i]->objectNum == objectNum )
	{
		return objectStates[i];
	}

	return NULL;
}

/*
========================
idSnapShot::CleanupEmptyStates
========================
*/
void idSnapShot::CleanupEmptyStates()
{
	for( int i = objectStates.Num() - 1; i >= 0 ; i-- )
	{
		if( objectStates[i]->buffer.Size() == 0 )
		{
			FreeObjectState( i );
			objectStates.RemoveIndex( i );
		}
	}
}

/*
========================
idSnapShot::UpdateExpectedSeq
========================
*/
void idSnapShot::UpdateExpectedSeq( int newSeq )
{
	for( int i = 0; i < objectStates.Num(); i++ )
	{
		if( objectStates[i]->expectedSequence == -2 )
		{
			objectStates[i]->expectedSequence = newSeq;
		}
	}
}

/*
========================
idSnapShot::FreeObjectState
========================
*/
void idSnapShot::FreeObjectState( int index )
{
	assert( objectStates[index] != NULL );
	//assert( mem.IsMapHeap() );
	objectStates[index]->buffer._Release();
	allocatedObjs.Free( objectStates[index] );
	objectStates[index] = NULL;
}

/*
========================
idSnapShot::ApplyToExistingState
Take uncompressed state in msg and add it to existing state
========================
*/
void idSnapShot::ApplyToExistingState( int objId, idBitMsg& msg )
{
	objectState_t* 	objectState = FindObjectByID( objId );
	if( !verify( objectState != NULL ) )
	{
		return;
	}

	if( !objectState->createdFromTemplate )
	{
		// We were created this from a template, so we shouldn't be applying it again
		if( net_ssTemplateDebug.GetBool() )
		{
			idLib::Printf( "NOT ApplyToExistingState[%d] because object was created from existing base state. %d\n", objId, objectState->expectedSequence );
			objectState->Print( "SS STATE" );
		}
		return;
	}

	// Debug print the template (spawn) and delta state
	if( net_ssTemplateDebug.GetBool() )
	{
		idLib::Printf( "\nApplyToExistingState[%d]. buffer size: %d msg size: %d\n", objId, objectState->buffer.Size(), msg.GetSize() );
		objectState->Print( "DELTA STATE" );

		PrintAlign( "SPAWN STATE" );
		for( int i = 0; i < msg.GetSize(); i++ )
		{
			if( InDebugRange( i ) )
			{
				idLib::Printf( "%02X", msg.GetReadData()[i] );
			}
		}
		idLib::Printf( "\n" );
	}

	// Actually apply it
	for( objectSize_t i = 0; i < Min( objectState->buffer.Size(), msg.GetSize() ); i++ )
	{
		objectState->buffer[i] += msg.GetReadData()[i];
	}

	// Debug print the final state
	if( net_ssTemplateDebug.GetBool() )
	{
		objectState->Print( "NEW STATE" );
		idLib::Printf( "\n" );
	}
}

#if 0
CONSOLE_COMMAND( serializeQTest, "Serialization Sanity Test", 0 )
{

	byte buffer[1024];
	memset( buffer, 0, sizeof( buffer ) );

	float values[] = { 0.0001f, 0.001f, 0.01f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 0.999f,
					   1.0f, 1.01f, 1.1f, 10.0f, 10.1f, 10.101f, 100.0f, 101.0f, 101.1f, 101.101f
					 };
	int num = sizeof( values ) / sizeof( float );

	idLib::Printf( "\n^3Testing SerializeQ and SerializeUQ \n" );

	{
		idBitMsg writeBitMsg;
		writeBitMsg.InitWrite( buffer, sizeof( buffer ) );
		idSerializer writeSerializer( writeBitMsg, true );

		for( int i = 0; i < num; i++ )
		{
			writeSerializer.SerializeUQ( values[i], 255.0f, 16 );
			writeSerializer.SerializeQ( values[i], 128.0f, 16 );
		}
	}

	{
		idBitMsg readBitMsg;
		readBitMsg.InitRead( buffer, sizeof( buffer ) );
		idSerializer readSerializer( readBitMsg, false );

		for( int i = 0; i < num; i++ )
		{

			float resultUQ = -999.0f;
			float resultQ  = -999.0f;

			readSerializer.SerializeUQ( resultUQ, 255.0f, 16 );
			readSerializer.SerializeQ( resultQ, 128.0f, 16 );

			float errorUQ = idMath::Fabs( ( resultUQ - values[i] ) ) / values[i];
			float errorQ  = idMath::Fabs( ( resultQ - values[i] ) ) / values[i];

			idLib::Printf( "%s%f SerializeUQ: %f. Error: %f \n", errorUQ > 0.1f ? "^1" : "", values[i], resultUQ, errorUQ );
			idLib::Printf( "%s%f SerializeQ: %f. Error: %f \n",   errorQ > 0.1f ? "^1" : "", values[i], resultQ, errorQ );
		}
	}
}
#endif
