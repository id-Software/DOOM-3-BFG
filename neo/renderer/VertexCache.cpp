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
#include "../idlib/precompiled.h"

#include "tr_local.h"

idVertexCache vertexCache;

idCVar r_showVertexCache( "r_showVertexCache", "0", CVAR_RENDERER | CVAR_BOOL, "Print stats about the vertex cache every frame" );
idCVar r_showVertexCacheTimings( "r_showVertexCache", "0", CVAR_RENDERER | CVAR_BOOL, "Print stats about the vertex cache every frame" );


/*
==============
ClearGeoBufferSet
==============
*/
static void ClearGeoBufferSet( geoBufferSet_t &gbs ) {
	gbs.indexMemUsed.SetValue( 0 );
	gbs.vertexMemUsed.SetValue( 0 );
	gbs.jointMemUsed.SetValue( 0 );
	gbs.allocations = 0;
}

/*
==============
MapGeoBufferSet
==============
*/
static void MapGeoBufferSet( geoBufferSet_t &gbs ) {
	if ( gbs.mappedVertexBase == NULL ) {
		gbs.mappedVertexBase = (byte *)gbs.vertexBuffer.MapBuffer( BM_WRITE );
	}
	if ( gbs.mappedIndexBase == NULL ) {
		gbs.mappedIndexBase = (byte *)gbs.indexBuffer.MapBuffer( BM_WRITE );
	}
	if ( gbs.mappedJointBase == NULL && gbs.jointBuffer.GetAllocedSize() != 0 ) {
		gbs.mappedJointBase = (byte *)gbs.jointBuffer.MapBuffer( BM_WRITE );
	}
}

/*
==============
UnmapGeoBufferSet
==============
*/
static void UnmapGeoBufferSet( geoBufferSet_t &gbs ) {
	if ( gbs.mappedVertexBase != NULL ) {
		gbs.vertexBuffer.UnmapBuffer();
		gbs.mappedVertexBase = NULL;
	}
	if ( gbs.mappedIndexBase != NULL ) {
		gbs.indexBuffer.UnmapBuffer();
		gbs.mappedIndexBase = NULL;
	}
	if ( gbs.mappedJointBase != NULL ) {
		gbs.jointBuffer.UnmapBuffer();
		gbs.mappedJointBase = NULL;
	}
}

/*
==============
AllocGeoBufferSet
==============
*/
static void AllocGeoBufferSet( geoBufferSet_t &gbs, const int vertexBytes, const int indexBytes, const int jointBytes ) {
	gbs.vertexBuffer.AllocBufferObject( NULL, vertexBytes );
	gbs.indexBuffer.AllocBufferObject( NULL, indexBytes );
	if ( jointBytes != 0 ) {
		gbs.jointBuffer.AllocBufferObject( NULL, jointBytes / sizeof( idJointMat ) );
	}
	ClearGeoBufferSet( gbs );
}

/*
==============
idVertexCache::Init
==============
*/
void idVertexCache::Init( bool restart ) {
	currentFrame = 0;
	listNum = 0;

	mostUsedVertex = 0;
	mostUsedIndex = 0;
	mostUsedJoint = 0;

	for ( int i = 0; i < VERTCACHE_NUM_FRAMES; i++ ) {
		AllocGeoBufferSet( frameData[i], VERTCACHE_VERTEX_MEMORY_PER_FRAME, VERTCACHE_INDEX_MEMORY_PER_FRAME, VERTCACHE_JOINT_MEMORY_PER_FRAME );
	}
	AllocGeoBufferSet( staticData, STATIC_VERTEX_MEMORY, STATIC_INDEX_MEMORY, 0 );

	MapGeoBufferSet( frameData[listNum] );
}

/*
==============
idVertexCache::Shutdown
==============
*/
void idVertexCache::Shutdown() {
	for ( int i = 0; i < VERTCACHE_NUM_FRAMES; i++ ) {
		frameData[i].vertexBuffer.FreeBufferObject();
		frameData[i].indexBuffer.FreeBufferObject();
		frameData[i].jointBuffer.FreeBufferObject();
	}
}

/*
==============
idVertexCache::PurgeAll
==============
*/
void idVertexCache::PurgeAll() {
	Shutdown();
	Init( true );
}

/*
==============
idVertexCache::FreeStaticData

call on loading a new map
==============
*/
void idVertexCache::FreeStaticData() {
	ClearGeoBufferSet( staticData );
	mostUsedVertex = 0;
	mostUsedIndex = 0;
	mostUsedJoint = 0;
}

/*
==============
idVertexCache::ActuallyAlloc
==============
*/
vertCacheHandle_t idVertexCache::ActuallyAlloc( geoBufferSet_t & vcs, const void * data, int bytes, cacheType_t type ) {
	if ( bytes == 0 ) {
		return (vertCacheHandle_t)0;
	}

	assert( ( ((UINT_PTR)(data)) & 15 ) == 0 );
	assert( ( bytes & 15 ) == 0 );

	// thread safe interlocked adds
	byte ** base = NULL;
	int	endPos = 0;
	if ( type == CACHE_INDEX ) {
		base = &vcs.mappedIndexBase;
		endPos = vcs.indexMemUsed.Add( bytes );
		if ( endPos > vcs.indexBuffer.GetAllocedSize() ) {
			idLib::Error( "Out of index cache" );
		}
	} else if ( type == CACHE_VERTEX ) {
		base = &vcs.mappedVertexBase;
		endPos = vcs.vertexMemUsed.Add( bytes );
		if ( endPos > vcs.vertexBuffer.GetAllocedSize() ) {
			idLib::Error( "Out of vertex cache" );
		}
	} else if ( type == CACHE_JOINT ) {
		base = &vcs.mappedJointBase;
		endPos = vcs.jointMemUsed.Add( bytes );
		if ( endPos > vcs.jointBuffer.GetAllocedSize() ) {
			idLib::Error( "Out of joint buffer cache" );
		}
	} else {
		assert( false );
	}

	vcs.allocations++;

	int offset = endPos - bytes;

	// Actually perform the data transfer
	if ( data != NULL ) {
		MapGeoBufferSet( vcs );
		CopyBuffer( *base + offset, (const byte *)data, bytes );
	}

	vertCacheHandle_t handle =	( (uint64)(currentFrame & VERTCACHE_FRAME_MASK ) << VERTCACHE_FRAME_SHIFT ) |
								( (uint64)(offset & VERTCACHE_OFFSET_MASK ) << VERTCACHE_OFFSET_SHIFT ) |
								( (uint64)(bytes & VERTCACHE_SIZE_MASK ) << VERTCACHE_SIZE_SHIFT );
	if ( &vcs == &staticData ) {
		handle |= VERTCACHE_STATIC;
	}
	return handle;
}

/*
==============
idVertexCache::GetVertexBuffer
==============
*/
bool idVertexCache::GetVertexBuffer( vertCacheHandle_t handle, idVertexBuffer * vb ) {
	const int isStatic = handle & VERTCACHE_STATIC;
	const uint64 size = (int)( handle >> VERTCACHE_SIZE_SHIFT ) & VERTCACHE_SIZE_MASK;
	const uint64 offset = (int)( handle >> VERTCACHE_OFFSET_SHIFT ) & VERTCACHE_OFFSET_MASK;
	const uint64 frameNum = (int)( handle >> VERTCACHE_FRAME_SHIFT ) & VERTCACHE_FRAME_MASK;
	if ( isStatic ) {
		vb->Reference( staticData.vertexBuffer, offset, size );
		return true;
	}
	if ( frameNum != ( ( currentFrame - 1 ) & VERTCACHE_FRAME_MASK ) ) {
		return false;
	}
	vb->Reference( frameData[drawListNum].vertexBuffer, offset, size );
	return true;
}

/*
==============
idVertexCache::GetIndexBuffer
==============
*/
bool idVertexCache::GetIndexBuffer( vertCacheHandle_t handle, idIndexBuffer * ib ) {
	const int isStatic = handle & VERTCACHE_STATIC;
	const uint64 size = (int)( handle >> VERTCACHE_SIZE_SHIFT ) & VERTCACHE_SIZE_MASK;
	const uint64 offset = (int)( handle >> VERTCACHE_OFFSET_SHIFT ) & VERTCACHE_OFFSET_MASK;
	const uint64 frameNum = (int)( handle >> VERTCACHE_FRAME_SHIFT ) & VERTCACHE_FRAME_MASK;
	if ( isStatic ) {
		ib->Reference( staticData.indexBuffer, offset, size );
		return true;
	}
	if ( frameNum != ( ( currentFrame - 1 ) & VERTCACHE_FRAME_MASK ) ) {
		return false;
	}
	ib->Reference( frameData[drawListNum].indexBuffer, offset, size );
	return true;
}

/*
==============
idVertexCache::GetJointBuffer
==============
*/
bool idVertexCache::GetJointBuffer( vertCacheHandle_t handle, idJointBuffer * jb ) {
	const int isStatic = handle & VERTCACHE_STATIC;
	const uint64 numBytes = (int)( handle >> VERTCACHE_SIZE_SHIFT ) & VERTCACHE_SIZE_MASK;
	const uint64 jointOffset = (int)( handle >> VERTCACHE_OFFSET_SHIFT ) & VERTCACHE_OFFSET_MASK;
	const uint64 frameNum = (int)( handle >> VERTCACHE_FRAME_SHIFT ) & VERTCACHE_FRAME_MASK;
	const uint64 numJoints = numBytes / sizeof( idJointMat );
	if ( isStatic ) {
		jb->Reference( staticData.jointBuffer, jointOffset, numJoints );
		return true;
	}
	if ( frameNum != ( ( currentFrame - 1 ) & VERTCACHE_FRAME_MASK ) ) {
		return false;
	}
	jb->Reference( frameData[drawListNum].jointBuffer, jointOffset, numJoints );
	return true;
}

/*
==============
idVertexCache::BeginBackEnd
==============
*/
void idVertexCache::BeginBackEnd() {
	mostUsedVertex = Max( mostUsedVertex, frameData[listNum].vertexMemUsed.GetValue() );
	mostUsedIndex = Max( mostUsedIndex, frameData[listNum].indexMemUsed.GetValue() );
	mostUsedJoint = Max( mostUsedJoint, frameData[listNum].jointMemUsed.GetValue() );

	if ( r_showVertexCache.GetBool() ) {
		idLib::Printf( "%08d: %d allocations, %dkB vertex, %dkB index, %kB joint : %dkB vertex, %dkB index, %kB joint\n", 
			currentFrame, frameData[listNum].allocations,
			frameData[listNum].vertexMemUsed.GetValue() / 1024,
			frameData[listNum].indexMemUsed.GetValue() / 1024,
			frameData[listNum].jointMemUsed.GetValue() / 1024,
			mostUsedVertex / 1024,
			mostUsedIndex / 1024,
			mostUsedJoint / 1024 );
	}

	// unmap the current frame so the GPU can read it
	const int startUnmap = Sys_Milliseconds();
	UnmapGeoBufferSet( frameData[listNum] );
	UnmapGeoBufferSet( staticData );
	const int endUnmap = Sys_Milliseconds();
	if ( endUnmap - startUnmap > 1 ) {
		idLib::PrintfIf( r_showVertexCacheTimings.GetBool(), "idVertexCache::unmap took %i msec\n", endUnmap - startUnmap );
	}
	drawListNum = listNum;

	// prepare the next frame for writing to by the CPU
	currentFrame++;

	listNum = currentFrame % VERTCACHE_NUM_FRAMES;
	const int startMap = Sys_Milliseconds();
	MapGeoBufferSet( frameData[listNum] );
	const int endMap = Sys_Milliseconds();
	if ( endMap - startMap > 1 ) {
		idLib::PrintfIf( r_showVertexCacheTimings.GetBool(), "idVertexCache::map took %i msec\n", endMap - startMap );
	}

	ClearGeoBufferSet( frameData[listNum] );


#if 0
	const int startBind = Sys_Milliseconds();
	qglBindBufferARB( GL_ARRAY_BUFFER_ARB, (GLuint)frameData[drawListNum].vertexBuffer.GetAPIObject() );
	qglBindBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, (GLuint)frameData[drawListNum].indexBuffer.GetAPIObject() );
	const int endBind = Sys_Milliseconds();
	if ( endBind - startBind > 1 ) {
		idLib::Printf( "idVertexCache::bind took %i msec\n", endBind - startBind );
	}
#endif

}

