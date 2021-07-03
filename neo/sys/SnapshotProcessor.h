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
#ifndef __SNAP_PROCESSOR_H__
#define __SNAP_PROCESSOR_H__

/*
================================================
idSnapshotProcessor
================================================
*/
class idSnapshotProcessor
{
public:
	static const int INITIAL_SNAP_SEQUENCE = 42;

	idSnapshotProcessor();
	~idSnapshotProcessor();

	void Reset( bool cstor = false );

	// TrySetPendingSnapshot Sets the currently pending snap.
	// No new snaps will be sent until this snap has been fully sent.
	// Returns true of the newly supplied snapshot was accepted (there were no pending snaps)
	bool TrySetPendingSnapshot( idSnapShot& ss );
	// Peek into delta to get deltaSequence, and deltaBaseSequence
	void PeekDeltaSequence( const char* deltaMem, int deltaSize, int& deltaSequence, int& deltaBaseSequence );
	// Apply a delta to the supplied snapshot
	bool ApplyDeltaToSnapshot( idSnapShot& snap, const char* deltaMem, int deltaSize, int visIndex );
	// Attempts to write the currently pending snap to the supplied buffer, which can then be sent as an unreliable msg.
	// SubmitPendingSnap will submit the pending snap to a job, so that it can be retrieved later for sending.
	void SubmitPendingSnap( int visIndex, uint8* objMemory, int objMemorySize, lzwCompressionData_t* lzwData );
	// GetPendingSnapDelta
	int GetPendingSnapDelta( byte* outBuffer, int maxLength );
	// If PendingSnapReadyToSend is true, then GetPendingSnapDelta will return something to send
	bool PendingSnapReadyToSend() const
	{
		return jobMemory->lzwInOutData.numlzwDeltas > 0;
	}
	// When you call WritePendingSnapshot, and then send the resulting buffer as a unreliable msg, you will eventually
	// receive this on the client.  Call this function to receive and apply it to the base state, and possibly return a fully received snap
	// to then apply to the client game state
	bool ReceiveSnapshotDelta( const byte* deltaData, int deltaLength, int visIndex, int& outSeq, int& outBaseSeq, idSnapShot& outSnap, bool& fullSnap );
	// Function to apply a received (or ack'd) delta to the base state
	bool ApplySnapshotDelta( int visIndex, int snapshotNumber );
	// Remove deltas for basestate we no longer have.
	// We know we can remove them, because we will never be able to apply them, since
	// the basestate needed to generate a full snap from these deltas is gone.
	void RemoveDeltasForOldBaseSequence();
	// Make sure delta sequence and basesequence values are valid, and in order, etc
	void SanityCheckDeltas();
	// HasPendingSnap will return true if there is more of the last TrySetPendingSnapshot to be sent
	bool HasPendingSnap() const
	{
		return hasPendingSnap;
	}

	idSnapShot* GetBaseState()
	{
		return &baseState;
	}
	idSnapShot* GetPendingSnap()
	{
		return &pendingSnap;
	}

	int GetSnapSequence()
	{
		return snapSequence;
	}
	int GetBaseSequence()
	{
		return baseSequence;
	}
	int GetFullSnapBaseSequence()
	{
		return lastFullSnapBaseSequence;
	}

	// This is used to ack the latest delta we have.  If we have no deltas, we sent -1 to make sure
	// Server knows we don't want to ack, since we are as up to date as we can be
	int GetLastAppendedSequence()
	{
		return deltas.Num() == 0 ? -1 : deltas.ItemSequence( deltas.Num() - 1 );
	}

	int	GetSnapQueueSize()
	{
		return deltas.Num();
	}

	bool IsBusyConfirmingPartialSnap();

	void AddSnapObjTemplate( int objID, idBitMsg& msg );

	static const int MAX_SNAPSHOT_QUEUE		= 64;

private:

	// Internal commands to set up, and flush the compressors
	static const int MAX_SNAP_SIZE			= idPacketProcessor::MAX_MSG_SIZE;
	static const int MAX_SNAPSHOT_QUEUE_MEM	= 64 * 1024;	// 64k

	// sequence number of the last snapshot we sent/received
	// on the server, the sequencing is different for each network peer (net_verboseSnapshot 1)
	// on the jobbed snapshot compression path, the sequence is incremented in NewLZWStream and pulled into this in idSnapshotProcessor::GetPendingSnapDelta
	int				snapSequence;
	int				baseSequence;
	int				lastFullSnapBaseSequence;		// Latest base sequence number that is a full snap

	idSnapShot		baseState;			// known snapshot base on the client
	idDataQueue< MAX_SNAPSHOT_QUEUE, MAX_SNAPSHOT_QUEUE_MEM >	deltas;		// list of unacknowledged snapshot deltas

	idSnapShot		pendingSnap;		// Current snap waiting to be fully sent
	bool			hasPendingSnap;		// true if pendingSnap is still waiting to be sent

	struct jobMemory_t
	{
		static const int MAX_LZW_DELTAS		= 1;			// FIXME: cleanup the old multiple delta support completely

		// @TODO this is a hack fix to allow online to load into coop (where there are lots of entities).
		// The real solution should be coming soon.
		// Doom MP: we encountered the same problem, going from 1024 to 4096 as well until a better solution is in place
		// (initial, useless, exchange of func_statics is killing us)
		static const int MAX_OBJ_PARMS		= 4096;

		static const int MAX_LZW_PARMS		= 32;
		static const int MAX_OBJ_HEADERS	= 256;
		static const int MAX_LZW_MEM		= 1024 * 8;		// 8k in the byte * lzwMem buffers, must be <= PS3_DMA_MAX

		// Parm memory to jobs
		idArray<objParms_t, MAX_OBJ_PARMS>		objParms;
		idArray<objHeader_t, MAX_OBJ_HEADERS>	headers;
		idArray<lzwParm_t, MAX_LZW_PARMS>		lzwParms;

		// Output memory from jobs
		idArray<lzwDelta_t, MAX_LZW_DELTAS>	lzwDeltas;			// Info about each pending delta output from jobs
		idArray<byte, MAX_LZW_MEM>		lzwMem;				// Memory for output from lzw jobs

		lzwInOutData_t	lzwInOutData;						// In/Out data used so lzw data can persist across lzw jobs
	};

	jobMemory_t* 	jobMemory;

	idSnapShot		submittedState;

	idSnapShot		templateStates;			// holds default snapshot states for some newly spawned object
	idSnapShot		submittedTemplateStates;

	int				partialBaseSequence;
};

#endif /* !__SNAP_PROCESSOR_H__ */
