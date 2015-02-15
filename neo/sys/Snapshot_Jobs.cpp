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
#pragma hdrstop
#include "precompiled.h"

#include "Snapshot_Jobs.h"

uint32 SnapObjChecksum( const uint8* data, int length )
{
	// RB: 64 bit fixes, changed long to int
	extern unsigned int CRC32_BlockChecksum( const void* data, int length );
	// RB end
	
	return CRC32_BlockChecksum( data, length );
}

/*
========================
ObjectsSame
========================
*/
ID_INLINE bool ObjectsSame( objJobState_t& newState, objJobState_t& oldState )
{
	assert( newState.valid && oldState.valid );
	assert( newState.objectNum == oldState.objectNum );
	
	if( newState.size != oldState.size )
	{
		//assert( newState.data != oldState.data) );
		return false;		// Can't match if sizes different
	}
	
	/*
	if ( newState.data == oldState.data ) {
		return true;		// Definite match
	}
	*/
	
	if( memcmp( newState.data, oldState.data, newState.size ) == 0 )
	{
		return true;		// Byte match, same
	}
	
	return false;			// Not the same
}

/*
========================
SnapshotObjectJob
This job processes objects by delta comparing them, and then zrle encoding them to the dest stream
The dest stream is then eventually read by the lzw job, and then lzw compressed into the final delta packet
ready to be sent to peers.
========================
*/
void SnapshotObjectJob( objParms_t* parms )
{
	int				visIndex	= parms->visIndex;
	objJobState_t& 	newState	= parms->newState;
	objJobState_t& 	oldState	= parms->oldState;
	objHeader_t* 	header		= parms->destHeader;
	uint8* 			dataStart	= parms->dest;
	
	assert( newState.valid || oldState.valid );
	
	// Setup header
	header->flags	= 0;
	header->size	= newState.valid ? newState.size : 0;
	header->csize	= 0;
	header->objID	= -1;			// Default to ack
	header->data	= dataStart;
	
	assert( header->size <= MAX_UNSIGNED_TYPE( objectSize_t ) );
	
	// Setup checksum and tag
#ifdef SNAPSHOT_CHECKSUMS
	header->checksum = 0;
#endif
	
	idZeroRunLengthCompressor rleCompressor;
	
	bool visChange		= false; // visibility changes will be signified with a 0xffff state size
	bool visSendState	= false; // the state is sent when an entity is no longer stale
	
	// Compute visibility changes
	// (we need to do this before writing out object id, because we may not need to write out the id if we early out)
	// (when we don't write out the id, we assume this is an "ack" when we deserialize the objects)
	if( newState.valid && oldState.valid )
	{
		// Check visibility
		assert( newState.objectNum == oldState.objectNum );
		
		if( visIndex > 0 )
		{
			bool oldVisible = ( oldState.visMask & ( 1 << visIndex ) ) != 0;
			bool newVisible = ( newState.visMask & ( 1 << visIndex ) ) != 0;
			
			// Force visible if we need to either create or destroy this object
			newVisible |= ( newState.size == 0 ) != ( oldState.size == 0 );
			
			if( !oldVisible && !newVisible )
			{
				// object is stale and ack'ed for this client, write nothing (see 'same object' below)
				header->flags |= OBJ_SAME;
				return;
			}
			else if( oldVisible && !newVisible )
			{
				//SNAP_VERBOSE_PRINT( "object %d to client %d goes stale\n", newState->objectNum, visIndex );
				visChange = true;
				visSendState = false;
			}
			else if( !oldVisible && newVisible )
			{
				//SNAP_VERBOSE_PRINT( "object %d to client %d no longer stale\n", newState->objectNum, visIndex );
				visChange = true;
				visSendState = true;
			}
		}
		
		// Same object, write a delta (never early out during vis changes)
		if( !visChange && ObjectsSame( newState, oldState ) )
		{
			// same state, write nothing
			header->flags |= OBJ_SAME;
			return;
		}
	}
	
	// Get the id of the object we are writing out
	int32 objectNum = ( newState.valid ) ? newState.objectNum : oldState.objectNum;
	
	// Write out object id
	header->objID = objectNum;
	
	if( !newState.valid )
	{
		// Deleted, write 0 size
		assert( oldState.valid );
		header->flags |= OBJ_DELETED;
	}
	else if( !oldState.valid )
	{
		// New object, write out full state
		assert( newState.valid );
		// delta against an empty snap
		rleCompressor.Start( dataStart, NULL, OBJ_DEST_SIZE_ALIGN16( newState.size ) );
		rleCompressor.WriteBytes( newState.data, newState.size );
		header->csize = rleCompressor.End();
		header->flags |= OBJ_NEW;
		if( header->csize == -1 )
		{
			// Not enough space, don't compress, have lzw job do zrle compression instead
			memcpy( dataStart, newState.data, newState.size );
		}
	}
	else
	{
		// Compare to same obj id in different snapshot
		assert( newState.objectNum == oldState.objectNum );
		
		header->flags |= OBJ_DIFFERENT;
		
		if( visChange )
		{
			header->flags |= visSendState ? OBJ_VIS_NOT_STALE : OBJ_VIS_STALE;
		}
		
		if( !visChange || visSendState )
		{
			int compareSize = Min( newState.size, oldState.size );
			rleCompressor.Start( dataStart, NULL, OBJ_DEST_SIZE_ALIGN16( newState.size ) );
			for( int b = 0; b < compareSize; b++ )
			{
				byte delta = newState.data[b] - oldState.data[b];
				rleCompressor.WriteByte( ( 0xFF + 1 + delta ) & 0xFF );
			}
			// Get leftover
			int leftOver = newState.size - compareSize;
			
			if( leftOver > 0 )
			{
				rleCompressor.WriteBytes( newState.data + compareSize, leftOver );
			}
			
			header->csize = rleCompressor.End();
			
			if( header->csize == -1 )
			{
				// Not enough space, don't compress, have lzw job do zrle compression instead
				for( int b = 0; b < compareSize; b++ )
				{
					*dataStart++ = ( ( 0xFF + 1 + ( newState.data[b] - oldState.data[b] ) ) & 0xFF );
				}
				// Get leftover
				int leftOver = newState.size - compareSize;
				
				if( leftOver > 0 )
				{
					memcpy( dataStart, newState.data + compareSize, leftOver );
				}
			}
		}
	}
	
	assert( header->csize <= OBJ_DEST_SIZE_ALIGN16( header->size ) );
	
#ifdef SNAPSHOT_CHECKSUMS
	if( newState.valid )
	{
		assert( newState.size );
		header->checksum = SnapObjChecksum( newState.data, newState.size );
	}
#endif
}

/*
========================
FinishLZWStream
========================
*/
static void FinishLZWStream( lzwParm_t* parm, idLZWCompressor* lzwCompressor )
{
	if( lzwCompressor->IsOverflowed() )
	{
		lzwCompressor->Restore();
	}
	
	lzwDelta_t& pendingDelta = parm->ioData->lzwDeltas[parm->ioData->numlzwDeltas];
	
	if( lzwCompressor->End() == -1 )
	{
		// If we couldn't end the stream, notify the main thread
		pendingDelta.offset			= -1;
		pendingDelta.size			= -1;
		pendingDelta.snapSequence	= -1;
		
		parm->ioData->numlzwDeltas++;
		return;
	}
	
	int size = lzwCompressor->Length();
	
	pendingDelta.offset			= parm->ioData->lzwBytes;		// Remember offset into buffer
	pendingDelta.size			= size;							// Remember size
	pendingDelta.snapSequence	= parm->ioData->snapSequence;	// Remember which snap sequence this delta belongs to
	
	parm->ioData->lzwBytes += size;
	parm->ioData->numlzwDeltas++;
}

/*
========================
NewLZWStream
========================
*/
static void NewLZWStream( lzwParm_t* parm, idLZWCompressor* lzwCompressor )
{

	// Reset compressor
	int maxSize = parm->ioData->maxlzwMem - parm->ioData->lzwBytes;
	lzwCompressor->Start( &parm->ioData->lzwMem[parm->ioData->lzwBytes], maxSize );
	
	parm->ioData->lastObjId = 0;
	
	parm->ioData->snapSequence++;
	
	lzwCompressor->WriteAgnostic( parm->ioData->snapSequence );
	lzwCompressor->WriteAgnostic( parm->baseSequence );
	lzwCompressor->WriteAgnostic( parm->curTime );
}

/*
========================
ContinueLZWStream
========================
*/
static void ContinueLZWStream( lzwParm_t* parm, idLZWCompressor* lzwCompressor )
{
	// Continue compressor where we left off
	int maxSize = parm->ioData->maxlzwMem - parm->ioData->lzwBytes;
	lzwCompressor->Start( &parm->ioData->lzwMem[parm->ioData->lzwBytes], maxSize, true );
}

/*
========================
LZWJobInternal
This job takes a stream of objects, which should already be zrle compressed, and then lzw compresses them
and builds a final delta packet ready to be sent to peers.
========================
*/
void LZWJobInternal( lzwParm_t* parm, unsigned int dmaTag )
{
	assert( parm->numObjects > 0 );
	
#ifndef ALLOW_MULTIPLE_DELTAS
	if( parm->ioData->numlzwDeltas > 0 )
	{
		// Currently, we don't use fragmented deltas.
		// We only send the first one and rely on a full snap being sent to get the whole snap across
		assert( parm->ioData->numlzwDeltas == 1 );
		assert( !parm->ioData->fullSnap );
		return;
	}
#endif
	
	assert( parm->ioData->lzwBytes < parm->ioData->maxlzwMem );
	
	dmaTag = dmaTag;
	
#ifdef __GNUC__
	// DG: remove ALIGN16 for GCC/clang, as they can't use it here and clang gets an error
	idLZWCompressor lzwCompressor( parm->ioData->lzwData );
	// DG end
#else
	ALIGN16( idLZWCompressor lzwCompressor( parm->ioData->lzwData ) );
#endif
	
	if( parm->fragmented )
	{
		// This packet was partially written out, we need to continue writing, using previous lzw dictionary values
		ContinueLZWStream( parm, &lzwCompressor );
	}
	else
	{
		// We can start a new lzw dictionary
		NewLZWStream( parm, &lzwCompressor );
	}
	
	
	int numChangedObjProcessed = 0;
	
	for( int i = 0; i < parm->numObjects; i++ )
	{
	
		// This will eventually be gracefully caught in SnapshotProcessor.cpp.
		// It's nice to know right when it happens though, so you can inspect the situation.
		assert( !lzwCompressor.IsOverflowed() || numChangedObjProcessed > 1 );
		
		// First, see if we need to finish the current lzw stream
		if( lzwCompressor.IsOverflowed() || lzwCompressor.Length() >= parm->ioData->optimalLength )
		{
			FinishLZWStream( parm, &lzwCompressor );
			// indicate how much needs to be DMA'ed back out
			parm->ioData->lzwDmaOut = parm->ioData->lzwBytes;
#ifdef ALLOW_MULTIPLE_DELTAS
			NewLZWStream( parm, &lzwCompressor );
#else
			// Currently, we don't use fragmented deltas.
			// We only send the first one and rely on a full snap being sent to get the whole snap across
			assert( !parm->ioData->fullSnap );
			assert( parm->ioData->numlzwDeltas == 1 );
			return;
#endif
		}
		
		if( numChangedObjProcessed > 0 )
		{
			// We should be at a good spot in the stream if we've written at least one obj without overflowing, so save it
			lzwCompressor.Save();
		}
		
		// Get header
		objHeader_t* header = &parm->headers[i];
		
		if( header->objID == -1 )
		{
			assert( header->flags & OBJ_SAME );
			continue;			// Don't send object (which means ack)
		}
		
		numChangedObjProcessed++;
		
		// Write obj id as delta into stream
		lzwCompressor.WriteAgnostic<uint16>( ( uint16 )( header->objID - parm->ioData->lastObjId ) );
		parm->ioData->lastObjId = ( uint16 )header->objID;
		
		// Check special stale/notstale flags
		if( header->flags & ( OBJ_VIS_STALE | OBJ_VIS_NOT_STALE ) )
		{
			// Write stale/notstale flag
			objectSize_t value = ( header->flags & OBJ_VIS_STALE ) ? SIZE_STALE : SIZE_NOT_STALE;
			lzwCompressor.WriteAgnostic<objectSize_t>( value );
		}
		
		if( header->flags & OBJ_VIS_STALE )
		{
			continue;	// Don't write out data for stale objects
		}
		
		if( header->flags & OBJ_DELETED )
		{
			// Object was deleted
			lzwCompressor.WriteAgnostic<objectSize_t>( 0 );
			continue;
		}
		
		// Write size
		lzwCompressor.WriteAgnostic<objectSize_t>( ( objectSize_t )header->size );
		
		// Get compressed data area
		uint8* compressedData = header->data;
		
		if( header->csize == -1 )
		{
			// Wasn't zrle compressed, zrle now while lzw'ing
			idZeroRunLengthCompressor rleCompressor;
			rleCompressor.Start( NULL, &lzwCompressor, 0xFFFF );
			rleCompressor.WriteBytes( compressedData, header->size );
			rleCompressor.End();
		}
		else
		{
			// Write out zero-rle compressed data
			lzwCompressor.Write( compressedData, header->csize );
		}
		
#ifdef SNAPSHOT_CHECKSUMS
		// Write checksum
		lzwCompressor.WriteAgnostic( header->checksum );
#endif
		// This will eventually be gracefully caught in SnapshotProcessor.cpp.
		// It's nice to know right when it happens though, so you can inspect the situation.
		assert( !lzwCompressor.IsOverflowed() || numChangedObjProcessed > 1 );
	}
	
	if( !parm->saveDictionary )
	{
		// Write out terminator
		uint16 objectDelta = 0xFFFF - parm->ioData->lastObjId;
		lzwCompressor.WriteAgnostic( objectDelta );
		
		// Last stream
		FinishLZWStream( parm, &lzwCompressor );
		
		// indicate how much needs to be DMA'ed back out
		parm->ioData->lzwDmaOut = parm->ioData->lzwBytes;
		
		parm->ioData->fullSnap = true;		// We sent a full snap
	}
	else
	{
		// the compressor did some work, wrote data to lzwMem, but since we didn't call FinishLZWStream to end the compression,
		// we need to figure how much needs to be DMA'ed back out
		assert( parm->ioData->lzwBytes == 0 ); // I don't think we ever hit this with lzwBytes != 0, but adding it just in case
		parm->ioData->lzwDmaOut = parm->ioData->lzwBytes + lzwCompressor.Length();
	}
	
	assert( parm->ioData->lzwBytes < parm->ioData->maxlzwMem );
}

/*
========================
LZWJob
========================
*/
void LZWJob( lzwParm_t* parm )
{
	LZWJobInternal( parm, 0 );
}
