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
#ifndef __SNAPSHOT_JOBS_H__
#define __SNAPSHOT_JOBS_H__

#include "LightweightCompression.h"

//#define SNAPSHOT_CHECKSUMS

typedef int32 objectSize_t;

static const objectSize_t SIZE_STALE		= MAX_TYPE( objectSize_t );				// Special size to indicate object went stale
static const objectSize_t SIZE_NOT_STALE	= MAX_TYPE( objectSize_t ) - 1;			// Special size to indicate object is no longer stale

static const int RLE_COMPRESSION_PADDING				= 16;			// Padding to accommodate possible enlargement due to zlre compression

// OBJ_DEST_SIZE_ALIGN16 returns the total space needed to store an object for reading/writing during jobs
#define OBJ_DEST_SIZE_ALIGN16( s ) ( ( ( s ) + 15 ) & ~15 )

static const uint32 OBJ_VIS_STALE		= ( 1 << 0 );			// Object went stale
static const uint32 OBJ_VIS_NOT_STALE	= ( 1 << 1 );			// Object no longer stale
static const uint32 OBJ_NEW				= ( 1 << 2 );			// New object (not in the last snap)
static const uint32 OBJ_DELETED			= ( 1 << 3 );			// Object was deleted (not going to be in the new snap)
static const uint32 OBJ_DIFFERENT		= ( 1 << 4 );			// Objects are in both snaps, but different
static const uint32 OBJ_SAME			= ( 1 << 5 );			// Objects are in both snaps, and are the same (we don't send these, which means ack)

// This struct is used to communicate data from the obj jobs to the lzw job
struct ALIGNTYPE16 objHeader_t
{
	int32	objID;					// Id of object.
	int32	size;					// Size data object holds (will be 0 if the obj is being deleted)
	int32	csize;					// Size after zrle compression
	uint32	flags;					// Flags used to communicate state from obj job to lzw delta job
	uint8* data;					// Data ptr to obj memory
#ifdef SNAPSHOT_CHECKSUMS
	uint32	checksum;				// Checksum before compression, used for sanity checking
#endif
};

struct objJobState_t
{
	uint8				valid;
	uint8* 				data;
	uint16				size;
	uint16				objectNum;
	uint32				visMask;
};

// Input to initial jobs that produce delta'd zrle compressed versions of all the snap obj's
struct ALIGNTYPE16 objParms_t
{
	// Input
	uint8				visIndex;

	objJobState_t		newState;
	objJobState_t		oldState;

	// Output
	objHeader_t*			destHeader;
	uint8* 				dest;
};

// Output from the job that takes the results of the delta'd zrle obj's.
// This struct contains the start of where the final delta packet data is within lzwMem
struct ALIGNTYPE16 lzwDelta_t
{
	int					offset;						// Offset into lzwMem
	int					size;
	int					snapSequence;
};

// Struct used to maintain state that needs to persist across lzw jobs
struct ALIGNTYPE16 lzwInOutData_t
{
	int						numlzwDeltas;			// Num pending deltas written
	bool					fullSnap;				// True if entire snap was written out in one delta
	lzwDelta_t* 			lzwDeltas;				// Info about each final delta packet written out
	int						maxlzwDeltas;			// Max lzw deltas
	uint8* 					lzwMem;					// Resulting final lzw delta packet data
	int						maxlzwMem;				// Max size in bytes that can fit in lzwMem
	int						lzwDmaOut;				// How much of lzwMem needs to be DMA'ed back out
	int						lzwBytes;				// Final delta packet bytes written
	int						optimalLength;			// Optimal length of lzw streams
	int						snapSequence;
	uint16					lastObjId;				// Last obj id written out
	lzwCompressionData_t* 	lzwData;
};

// Input to the job that takes the results of the delta'd zrle obj's, and turns them into lzw delta packets
struct ALIGNTYPE16 lzwParm_t
{
	// Input
	int						numObjects;				// Number of objects this job needs to process
	objHeader_t*				headers;				// Object headers
	int						curTime;				// Cur snap time
	int						baseTime;				// Base snap time
	int						baseSequence;
	bool					saveDictionary;
	bool					fragmented;				// This lzw stream should continue where the last one left off

	// In/Out
	lzwInOutData_t* 		ioData;					// In/Out
};

extern void SnapshotObjectJob( objParms_t* parms );
extern void LZWJob( lzwParm_t* parm );

#endif // __SNAPSHOT_JOBS_H__
