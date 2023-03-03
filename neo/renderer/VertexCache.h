/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2016-2017 Dustin Land
Copyright (C) 2020 Robert Beckebans
Copyright (C) 2022 Stephen Pridham

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
#ifndef __VERTEXCACHE_H__
#define __VERTEXCACHE_H__

#if 0

	// RB: increased some static memory limits for custom modder content

	const int VERTCACHE_INDEX_MEMORY_PER_FRAME = 31 * 1024 * 1024;
	const int VERTCACHE_VERTEX_MEMORY_PER_FRAME = 31 * 1024 * 1024;
	const int VERTCACHE_JOINT_MEMORY_PER_FRAME = 256 * 1024;

	// there are a lot more static indexes than vertexes, because interactions are just new
	// index lists that reference existing vertexes
	const int STATIC_INDEX_MEMORY = 4 * 31 * 1024 * 1024;
	const int STATIC_VERTEX_MEMORY = 4 * 31 * 1024 * 1024;	// make sure it fits in VERTCACHE_OFFSET_MASK!

	// vertCacheHandle_t packs size, offset, and frame number into 64 bits
	typedef uint64 vertCacheHandle_t;
	const int VERTCACHE_STATIC = 1;						// in the static set, not the per-frame set

	const int VERTCACHE_SIZE_SHIFT = 1;
	const int VERTCACHE_SIZE_MASK = 0x7fffff;			// 23 bits = 8 megs

	const int VERTCACHE_OFFSET_SHIFT = 24;
	const int VERTCACHE_OFFSET_MASK = 0x1ffffff << 2;	// 27 bits = 128 megs

	const int VERTCACHE_FRAME_SHIFT = 51;
	const int VERTCACHE_FRAME_MASK = 0x1fff;			// 13 bits = 8191 frames to wrap around


#else

	// RB: original values which are still good for low spec hardware and performance

	const int VERTCACHE_INDEX_MEMORY_PER_FRAME = 31 * 1024 * 1024;
	const int VERTCACHE_VERTEX_MEMORY_PER_FRAME = 31 * 1024 * 1024;
	const int VERTCACHE_JOINT_MEMORY_PER_FRAME = 256 * 1024;

	// there are a lot more static indexes than vertexes, because interactions are just new
	// index lists that reference existing vertexes
	const int STATIC_INDEX_MEMORY = 31 * 1024 * 1024;
	const int STATIC_VERTEX_MEMORY = 31 * 1024 * 1024;	// make sure it fits in VERTCACHE_OFFSET_MASK!

	// vertCacheHandle_t packs size, offset, and frame number into 64 bits
	typedef uint64 vertCacheHandle_t;
	const int VERTCACHE_STATIC = 1;					// in the static set, not the per-frame set
	const int VERTCACHE_SIZE_SHIFT = 1;
	const int VERTCACHE_SIZE_MASK = 0x7fffff;		// 8 megs
	const int VERTCACHE_OFFSET_SHIFT = 24;
	const int VERTCACHE_OFFSET_MASK = 0x1ffffff;	// 32 megs
	const int VERTCACHE_FRAME_SHIFT = 49;
	const int VERTCACHE_FRAME_MASK = 0x7fff;		// 15 bits = 32k frames to wrap around

#endif

const int VERTEX_CACHE_ALIGN		= 32;
const int INDEX_CACHE_ALIGN			= 16;
const int JOINT_CACHE_ALIGN			= 16;

enum cacheType_t
{
	CACHE_VERTEX,
	CACHE_INDEX,
	CACHE_JOINT
};

struct geoBufferSet_t
{
	idIndexBuffer			indexBuffer;
	idVertexBuffer			vertexBuffer;
	idUniformBuffer			jointBuffer;
	byte* 					mappedVertexBase;
	byte* 					mappedIndexBase;
	byte* 					mappedJointBase;
	idSysInterlockedInteger	indexMemUsed;
	idSysInterlockedInteger	vertexMemUsed;
	idSysInterlockedInteger	jointMemUsed;
	int						allocations;	// number of index and vertex allocations combined
};

class idVertexCache
{
public:
	void			Init( int uniformBufferOffsetAlignment, nvrhi::ICommandList* commandList );
	void			Shutdown();
	void			PurgeAll( nvrhi::ICommandList* commandList );

	// call on loading a new map
	void			FreeStaticData();

	// this data is only valid for one frame of rendering
	vertCacheHandle_t	AllocVertex( const void* data, int num, size_t size = sizeof( idDrawVert ), nvrhi::ICommandList* commandList = nullptr );
	vertCacheHandle_t	AllocIndex( const void* data, int num, size_t size = sizeof( triIndex_t ), nvrhi::ICommandList* commandList = nullptr );
	vertCacheHandle_t	AllocJoint( const void* data, int num, size_t size = sizeof( idJointMat ), nvrhi::ICommandList* commandList = nullptr );

	// this data is valid until the next map load
	vertCacheHandle_t	AllocStaticVertex( const void* data, int bytes, nvrhi::ICommandList* commandList );
	vertCacheHandle_t	AllocStaticIndex( const void* data, int bytes, nvrhi::ICommandList* commandList );

	byte* 			MappedVertexBuffer( vertCacheHandle_t handle );
	byte* 			MappedIndexBuffer( vertCacheHandle_t handle );

	// Returns false if it's been purged
	// This can only be called by the front end, the back end should only be looking at
	// vertCacheHandle_t that are already validated.
	bool			CacheIsCurrent( const vertCacheHandle_t handle );
	static bool		CacheIsStatic( const vertCacheHandle_t handle )
	{
		return ( handle & VERTCACHE_STATIC ) != 0;
	}

	// vb/ib is a temporary reference -- don't store it
	bool			GetVertexBuffer( vertCacheHandle_t handle, idVertexBuffer* vb );
	bool			GetIndexBuffer( vertCacheHandle_t handle, idIndexBuffer* ib );
	bool			GetJointBuffer( vertCacheHandle_t handle, idUniformBuffer* jb );

	void			BeginBackEnd();

public:
	int				currentFrame;	// for determining the active buffers
	int				listNum;		// currentFrame % NUM_FRAME_DATA
	int				drawListNum;	// (currentFrame-1) % NUM_FRAME_DATA

	geoBufferSet_t	staticData;
	geoBufferSet_t	frameData[ NUM_FRAME_DATA ];

	int				uniformBufferOffsetAlignment;

	// High water marks for the per-frame buffers
	int				mostUsedVertex;
	int				mostUsedIndex;
	int				mostUsedJoint;

	// Try to make room for <bytes> bytes
	vertCacheHandle_t	ActuallyAlloc( geoBufferSet_t& vcs, const void* data, int bytes, cacheType_t type, nvrhi::ICommandList* commandList );
};

// platform specific code to memcpy into vertex buffers efficiently
// 16 byte alignment is guaranteed
void CopyBuffer( byte* dst, const byte* src, int numBytes );

extern	idVertexCache	vertexCache;

#endif // __VERTEXCACHE_H__
