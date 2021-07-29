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

idCVar net_optimalSnapDeltaSize( "net_optimalSnapDeltaSize", "1000", CVAR_INTEGER, "Optimal size of snapshot delta msgs." );
idCVar net_debugBaseStates( "net_debugBaseStates", "0", CVAR_BOOL, "Log out base state information" );
idCVar net_skipClientDeltaAppend( "net_skipClientDeltaAppend", "0", CVAR_BOOL, "Simulate delta receive buffer overflowing" );

/*
========================
idSnapshotProcessor::idSnapshotProcessor
========================
*/
idSnapshotProcessor::idSnapshotProcessor()
{

	//assert( mem.IsGlobalHeap() );

	jobMemory = ( jobMemory_t* )Mem_Alloc( sizeof( jobMemory_t ) , TAG_NETWORKING );

	assert_16_byte_aligned( jobMemory );
	assert_16_byte_aligned( jobMemory->objParms.Ptr() );
	assert_16_byte_aligned( jobMemory->headers.Ptr() );
	assert_16_byte_aligned( jobMemory->lzwParms.Ptr() );

	Reset( true );
}

/*
========================
idSnapshotProcessor::idSnapshotProcessor
========================
*/
idSnapshotProcessor::~idSnapshotProcessor()
{
	//mem.PushHeap();
	Mem_Free( jobMemory );
	//mem.PopHeap();
}

/*
========================
idSnapshotProcessor::Reset
========================
*/
void idSnapshotProcessor::Reset( bool cstor )
{
	hasPendingSnap	= false;
	snapSequence	= INITIAL_SNAP_SEQUENCE;
	baseSequence	= -1;
	lastFullSnapBaseSequence = -1;

	if( !cstor && net_debugBaseStates.GetBool() )
	{
		idLib::Printf( "NET: Reset snapshot base" );
	}

	baseState.Clear();
	submittedState.Clear();
	pendingSnap.Clear();
	deltas.Clear();

	partialBaseSequence = -1;

	memset( &jobMemory->lzwInOutData, 0, sizeof( jobMemory->lzwInOutData ) );
}

/*
========================
idSnapshotProcessor::TrySetPendingSnapshot
========================
*/
bool idSnapshotProcessor::TrySetPendingSnapshot( idSnapShot& ss )
{
	// Don't advance to the next snap until the last one was fully sent
	if( hasPendingSnap )
	{
		return false;
	}
	pendingSnap = ss;
	hasPendingSnap = true;
	return true;
}

/*
========================
idSnapshotProcessor::PeekDeltaSequence
========================
*/
void idSnapshotProcessor::PeekDeltaSequence( const char* deltaMem, int deltaSize, int& deltaSequence, int& deltaBaseSequence )
{
	idSnapShot::PeekDeltaSequence( deltaMem, deltaSize, deltaSequence, deltaBaseSequence );
}

/*
========================
idSnapshotProcessor::ApplyDeltaToSnapshot
========================
*/
bool idSnapshotProcessor::ApplyDeltaToSnapshot( idSnapShot& snap, const char* deltaMem, int deltaSize, int visIndex )
{
	return snap.ReadDeltaForJob( deltaMem, deltaSize, visIndex, &templateStates );
}

#ifdef STRESS_LZW_MEM
	// When this defined, we'll stress the lzw compressor with the smallest possible buffer, and detect when we need to grow it to make
	// sure we are gacefully detecting the situation.
	static int g_maxlwMem = 100;
#endif

/*
========================
idSnapshotProcessor::SubmitPendingSnap
========================
*/
void idSnapshotProcessor::SubmitPendingSnap( int visIndex, uint8* objMemory, int objMemorySize, lzwCompressionData_t* lzwData )
{

	assert_16_byte_aligned( objMemory );
	assert_16_byte_aligned( lzwData );

	assert( hasPendingSnap );
	assert( jobMemory->lzwInOutData.numlzwDeltas == 0 );

	assert( net_optimalSnapDeltaSize.GetInteger() < jobMemory_t::MAX_LZW_MEM - 128 );		// Leave padding

	jobMemory->lzwInOutData.lzwDeltas		= jobMemory->lzwDeltas.Ptr();
	jobMemory->lzwInOutData.maxlzwDeltas	= jobMemory->lzwDeltas.Num();
	jobMemory->lzwInOutData.lzwMem			= jobMemory->lzwMem.Ptr();

#ifdef STRESS_LZW_MEM
	jobMemory->lzwInOutData.maxlzwMem		= g_maxlwMem;
#else
	jobMemory->lzwInOutData.maxlzwMem		= jobMemory_t::MAX_LZW_MEM;
#endif

	jobMemory->lzwInOutData.lzwDmaOut		= jobMemory_t::MAX_LZW_MEM;
	jobMemory->lzwInOutData.numlzwDeltas	= 0;
	jobMemory->lzwInOutData.lzwBytes		= 0;
	jobMemory->lzwInOutData.optimalLength	= net_optimalSnapDeltaSize.GetInteger();
	jobMemory->lzwInOutData.snapSequence	= snapSequence;
	jobMemory->lzwInOutData.lastObjId		= 0;
	jobMemory->lzwInOutData.lzwData			= lzwData;

	idSnapShot::submitDeltaJobsInfo_t submitInfo;

	submitInfo.objParms			= jobMemory->objParms.Ptr();
	submitInfo.maxObjParms		= jobMemory->objParms.Num();
	submitInfo.headers			= jobMemory->headers.Ptr();
	submitInfo.maxHeaders		= jobMemory->headers.Num();
	submitInfo.objMemory		= objMemory;
	submitInfo.maxObjMemory		= objMemorySize;
	submitInfo.lzwParms			= jobMemory->lzwParms.Ptr();
	submitInfo.maxDeltaParms	= jobMemory->lzwParms.Num();


	// Use a copy of base state to avoid race conditions.
	// The main thread could change it behind the jobs backs.
	submittedState				= baseState;
	submittedTemplateStates		= templateStates;

	submitInfo.templateStates	= &submittedTemplateStates;

	submitInfo.oldSnap			= &submittedState;
	submitInfo.visIndex			= visIndex;
	submitInfo.baseSequence		= baseSequence;

	submitInfo.lzwInOutData		= &jobMemory->lzwInOutData;

	pendingSnap.SubmitWriteDeltaToJobs( submitInfo );
}

/*
========================
idSnapshotProcessor::GetPendingSnapDelta
========================
*/
int idSnapshotProcessor::GetPendingSnapDelta( byte* outBuffer, int maxLength )
{

	assert( PendingSnapReadyToSend() );

	if( !verify( jobMemory->lzwInOutData.numlzwDeltas == 1 ) )
	{
		jobMemory->lzwInOutData.numlzwDeltas = 0;
		return 0;  // No more deltas left to send
	}

	assert( hasPendingSnap );

	jobMemory->lzwInOutData.numlzwDeltas = 0;

	int size = jobMemory->lzwDeltas[0].size;

	if( !verify( size != -1 ) )
	{
#ifdef STRESS_LZW_MEM
		if( g_maxlwMem < MAX_LZW_MEM )
		{
			g_maxlwMem += 50;
			g_maxlwMem = Min( g_maxlwMem, MAX_LZW_MEM );
			return 0;
		}
#endif

		// This can happen if there wasn't enough maxlzwMem to process one full obj in a single delta
		idLib::Error( "GetPendingSnapDelta: Delta failed." );
	}

	uint8* deltaData = &jobMemory->lzwMem[jobMemory->lzwDeltas[0].offset];

	int deltaSequence		= 0;
	int deltaBaseSequence	= 0;
	PeekDeltaSequence( ( const char* )deltaData, size, deltaSequence, deltaBaseSequence );
	// sanity check: does the compressed data we are about to send have the sequence number we expect
	assert( deltaSequence == jobMemory->lzwDeltas[0].snapSequence );

	if( !verify( size <= maxLength ) )
	{
		idLib::Error( "GetPendingSnapDelta: Size overflow." );
	}

	// Copy to out buffer
	memcpy( outBuffer, deltaData, size );

	// Set the sequence to what this delta actually belongs to
	assert( jobMemory->lzwDeltas[0].snapSequence == snapSequence + 1 );
	snapSequence = jobMemory->lzwDeltas[0].snapSequence;

	//idLib::Printf( "deltas Num: %i, Size: %i\n", deltas.Num(), deltas.GetDataLength() );

	// Copy to delta buffer
	// NOTE - We don't need to save this delta off if peer has already ack'd this basestate.
	// This can happen due to the fact that we defer the processing of snap deltas on jobs.
	// When we start processing a delta, we use the currently ack'd basestate.  If while we were processing
	// the delta, the client acks a new basestate, we can get into this situation.  In this case, we simply don't
	// store the delta, since it will just take up space, and just get removed anyways during ApplySnapshotDelta.
	//	 (and cause lots of spam when it sees the delta's basestate doesn't match the current ack'd one)
	if( deltaBaseSequence >= baseSequence )
	{
		if( !deltas.Append( snapSequence, deltaData, size ) )
		{
			int resendLength = deltas.ItemLength( deltas.Num() - 1 );

			if( !verify( resendLength <= maxLength ) )
			{
				idLib::Error( "GetPendingSnapDelta: Size overflow for resend." );
			}

			memcpy( outBuffer, deltas.ItemData( deltas.Num() - 1 ), resendLength );
			size = -resendLength;
		}
	}

	if( jobMemory->lzwInOutData.fullSnap )
	{
		// We sent the full snap, we can stop sending this pending snap now...
		NET_VERBOSESNAPSHOT_PRINT_LEVEL( 5, va( "  wrote enough deltas to a full snapshot\n" ) ); // FIXME: peer number?

		hasPendingSnap = false;
		partialBaseSequence = -1;

	}
	else
	{
		partialBaseSequence = deltaBaseSequence;
	}

	return size;
}

/*
========================
idSnapshotProcessor::IsBusyConfirmingPartialSnap
========================
*/
bool idSnapshotProcessor::IsBusyConfirmingPartialSnap()
{
	if( partialBaseSequence != -1 && baseSequence <= partialBaseSequence )
	{
		return true;
	}

	return false;
}

/*
========================
idSnapshotProcessor::ReceiveSnapshotDelta
NOTE: we use ReadDeltaForJob twice, once to build the same base as the server (based on server acks, down ApplySnapshotDelta), and another time to apply the snapshot we just received
could we avoid the double apply by keeping outSnap cached in memory and avoid rebuilding it from a delta when the next one comes around?
========================
*/
bool idSnapshotProcessor::ReceiveSnapshotDelta( const byte* deltaData, int deltaLength, int visIndex, int& outSeq, int& outBaseSeq, idSnapShot& outSnap, bool& fullSnap )
{

	fullSnap = false;

	int deltaSequence		= 0;
	int deltaBaseSequence	= 0;

	// Get the sequence of this delta, and the base sequence it is delta'd from
	PeekDeltaSequence( ( const char* )deltaData, deltaLength, deltaSequence, deltaBaseSequence );

	//idLib::Printf("Incoming snapshot: %i, %i\n", deltaSequence, deltaBaseSequence );

	if( deltaSequence <= snapSequence )
	{
		NET_VERBOSESNAPSHOT_PRINT( "Rejecting old delta: %d (snapSequence: %d \n", deltaSequence, snapSequence );
		return false;		// Completely reject older out of order deltas
	}

	// Bring the base state up to date with the basestate this delta was compared to
	ApplySnapshotDelta( visIndex, deltaBaseSequence );

	// Once we get here, our base state should be caught up to that of the server
	assert( baseSequence == deltaBaseSequence );

	// Save the new delta
	if( net_skipClientDeltaAppend.GetBool() || !deltas.Append( deltaSequence, deltaData, deltaLength ) )
	{
		// This can happen if the delta queues get desync'd between the server and client.
		// With recent fixes, this should be extremely rare, or impossible.
		// Just in case this happens, we can recover by assuming we didn't even receive this delta.
		idLib::Printf( "NET: ReceiveSnapshotDelta: No room to append delta %d/%d \n", deltaSequence, deltaBaseSequence );
		return false;
	}

	// Update our snapshot sequence number to the newer one we just got (now that it's safe)
	snapSequence = deltaSequence;

	if( deltas.Num() > 10 )
	{
		NET_VERBOSESNAPSHOT_PRINT( "NET: ReceiveSnapshotDelta: deltas.Num() > 10: %d\n   ", deltas.Num() );
		for( int i = 0; i < deltas.Num(); i++ )
		{
			NET_VERBOSESNAPSHOT_PRINT( "%d ", deltas.ItemSequence( i ) );
		}
		NET_VERBOSESNAPSHOT_PRINT( "\n" );
	}


	if( baseSequence != deltaBaseSequence )
	{
		// NOTE - With recent fixes, this should no longer be possible unless the delta is trashed
		// We should probably disconnect from the server when this happens now.
		static bool failed = false;
		if( !failed )
		{
			idLib::Printf( "NET: incorrect base state? not sure how this can happen... baseSequence: %d  deltaBaseSequence: %d \n", baseSequence, deltaBaseSequence );
		}
		failed = true;
		return false;
	}

	// Copy out the current deltas sequence values to caller
	outSeq		= deltaSequence;
	outBaseSeq	= deltaBaseSequence;

	if( baseSequence < 50 && net_debugBaseStates.GetBool() )
	{
		idLib::Printf( "NET: Proper basestate...  baseSequence: %d  deltaBaseSequence: %d \n", baseSequence, deltaBaseSequence );
	}

	// Make a copy of the basestate the server used to create this delta, and then apply and return it
	outSnap = baseState;

	fullSnap = ApplyDeltaToSnapshot( outSnap, ( const char* )deltaData, deltaLength, visIndex );

	// We received a new delta
	return true;
}

/*
========================
idSnapshotProcessor::ApplySnapshotDelta
Apply a snapshot delta to our current basestate, and make that the new base.
We can remove all deltas that refer to the basetate we just removed.
========================
*/
bool idSnapshotProcessor::ApplySnapshotDelta( int visIndex, int snapshotNumber )
{

	NET_VERBOSESNAPSHOT_PRINT_LEVEL( 6, va( "idSnapshotProcessor::ApplySnapshotDelta snapshotNumber: %d\n", snapshotNumber ) );

	// Sanity check deltas
	SanityCheckDeltas();

	// dump any deltas older than the acknoweledged snapshot, which should only happen if there is packet loss
	deltas.RemoveOlderThan( snapshotNumber );

	if( deltas.Num() == 0 || deltas.ItemSequence( 0 ) != snapshotNumber )
	{
		// this means the snapshot was either already acknowledged or came out of order
		// On the server, this can happen because the client is continuously/redundantly sending acks
		// Once the server has ack'd a certain base sequence, it will need to ignore all the redundant ones.
		// On the client, this will only happen due to out of order, or dropped packets.

		if( !common->IsServer() )
		{
			// these should be printed every time on the clients
			// printing on server is not useful / results in tons of spam
			if( deltas.Num() == 0 )
			{
				NET_VERBOSESNAPSHOT_PRINT( "NET: Got snapshot but ignored... deltas.Num(): %d snapshotNumber: %d \n", deltas.Num(), snapshotNumber );
			}
			else
			{
				NET_VERBOSESNAPSHOT_PRINT( "NET: Got snapshot but ignored... deltas.ItemSequence( 0 ): %d != snapshotNumber: %d \n   ", deltas.ItemSequence( 0 ), snapshotNumber );

				for( int i = 0; i < deltas.Num(); i++ )
				{
					NET_VERBOSESNAPSHOT_PRINT( "%d ", deltas.ItemSequence( i ) );
				}
				NET_VERBOSESNAPSHOT_PRINT( "\n" );

			}
		}
		return false;
	}

	int deltaSequence		= 0;
	int deltaBaseSequence	= 0;

	PeekDeltaSequence( ( const char* )deltas.ItemData( 0 ), deltas.ItemLength( 0 ), deltaSequence, deltaBaseSequence );

	assert( deltaSequence == snapshotNumber );		// Make sure compressed sequence number matches that in data queue
	assert( baseSequence == deltaBaseSequence );	// If this delta isn't based off of our currently ack'd basestate, something is trashed...
	assert( deltaSequence > baseSequence );

	if( baseSequence != deltaBaseSequence )
	{
		// NOTE - This should no longer happen with recent fixes.
		// We should probably disconnect from the server if this happens. (packets are trashed most likely)
		NET_VERBOSESNAPSHOT_PRINT( "NET: Got snapshot %d but baseSequence does not match. baseSequence: %d deltaBaseSequence: %d. \n", snapshotNumber, baseSequence, deltaBaseSequence );
		return false;
	}

	// Apply this delta to our base state
	if( ApplyDeltaToSnapshot( baseState, ( const char* )deltas.ItemData( 0 ), deltas.ItemLength( 0 ), visIndex ) )
	{
		lastFullSnapBaseSequence = deltaSequence;
	}

	baseSequence = deltaSequence;		// This is now our new base sequence

	// Remove deltas that we no longer need
	RemoveDeltasForOldBaseSequence();

	// Sanity check deltas
	SanityCheckDeltas();

	return true;
}

/*
========================
idSnapshotProcessor::RemoveDeltasForOldBaseSequence
Remove deltas for basestate we no longer have. We know we can remove them, because we will never
be able to apply them, since the basestate needed to generate a full snap from these deltas is gone.

Ways we can get deltas based on basestate we no longer have:
	1. Server sends sequence 50 based on 49.  It then sends sequence 51 based on 49.
	   Client acks 50, server applies it to 49, 50 is new base state.
	   Server now has a delta sequence 51 based on 49 that it won't ever be able to apply (50 is new basestate).

This is annoying, because it makes a lot of our sanity checks incorrectly fire off for benign issues.
Here is a series of events that make the old ( baseSequence != deltaBaseSequence ) assert:

Server:
49->50, 49->51, ack 50, 50->52, ack 51 (bam), 50->53

Client
49->50, ack 50, 49->51, ack 51 (bam), 50->52, 50->53

The client above will ack 51, even though he can't even apply that delta.  To get around this, we simply don't
allow delta to exist in the list, unless their basestate is the current basestate we maintain.
This allows us to put sanity checks in place that don't fire off during benign conditions, and allow us
to truly check for trashed conditions.

========================
*/
void idSnapshotProcessor::RemoveDeltasForOldBaseSequence()
{
	// Remove any deltas that would apply to the old base we no longer maintain
	// (we will never be able to apply these, since we don't have that base anymore)
	for( int i = deltas.Num() - 1; i >= 0; i-- )
	{
		int deltaSequence		= 0;
		int deltaBaseSequence	= 0;
		baseState.PeekDeltaSequence( ( const char* )deltas.ItemData( i ), deltas.ItemLength( i ), deltaSequence, deltaBaseSequence );
		if( deltaBaseSequence < baseSequence )
		{
			// Remove this delta, and all deltas before this one
			deltas.RemoveOlderThan( deltas.ItemSequence( i ) + 1 );
			break;
		}
	}
}

/*
========================
idSnapshotProcessor::SanityCheckDeltas
Make sure delta sequence and basesequence values are valid, and in order, etc
========================
*/
void idSnapshotProcessor::SanityCheckDeltas()
{
	int deltaSequence			= 0;
	int deltaBaseSequence		= 0;
	int lastDeltaSequence		= -1;
	int lastDeltaBaseSequence	= -1;

	for( int i = 0; i < deltas.Num(); i++ )
	{
		baseState.PeekDeltaSequence( ( const char* )deltas.ItemData( i ), deltas.ItemLength( i ), deltaSequence, deltaBaseSequence );
		assert( deltaSequence == deltas.ItemSequence( i ) );	// Make sure delta stored in compressed form matches the one stored in the data queue
		assert( deltaSequence > lastDeltaSequence );			// Make sure they are in order (we reject out of order sequences in ApplysnapshotDelta)
		assert( deltaBaseSequence >= lastDeltaBaseSequence );	// Make sure they are in order (they can be the same, since base sequences don't change until they've been ack'd)
		assert( deltaBaseSequence >= baseSequence );			// We should have removed old delta's that can no longer be applied
		assert( deltaBaseSequence == baseSequence || deltaBaseSequence == lastDeltaSequence );	// Make sure we still have a base (or eventually will have) that we can apply this delta to
		lastDeltaSequence = deltaSequence;
		lastDeltaBaseSequence = deltaBaseSequence;
	}
}

/*
========================
idSnapshotProcessor::AddSnapObjTemplate
========================
*/
void idSnapshotProcessor::AddSnapObjTemplate( int objID, idBitMsg& msg )
{
	extern idCVar net_ssTemplateDebug;
	idSnapShot::objectState_t* state = templateStates.S_AddObject( objID, MAX_UNSIGNED_TYPE( uint32 ), msg );
	if( verify( state != NULL ) )
	{
		if( net_ssTemplateDebug.GetBool() )
		{
			idLib::PrintfIf( net_ssTemplateDebug.GetBool(), "InjectingSnapObjBaseState[%d] size: %d\n", objID, state->buffer.Size() );
			state->Print( "BASE STATE" );
		}
		state->expectedSequence = snapSequence;
	}
}
