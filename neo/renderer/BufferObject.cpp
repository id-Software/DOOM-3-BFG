/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013 Robert Beckebans

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
#include "tr_local.h"

idCVar r_showBuffers( "r_showBuffers", "0", CVAR_INTEGER, "" );


//static const GLenum bufferUsage = GL_STATIC_DRAW;
static const GLenum bufferUsage = GL_DYNAMIC_DRAW;

// RB begin
#if defined(_WIN32)
/*
==================
IsWriteCombined
==================
*/
bool IsWriteCombined( void* base )
{
	MEMORY_BASIC_INFORMATION info;
	SIZE_T size = VirtualQueryEx( GetCurrentProcess(), base, &info, sizeof( info ) );
	if( size == 0 )
	{
		DWORD error = GetLastError();
		error = error;
		return false;
	}
	bool isWriteCombined = ( ( info.AllocationProtect & PAGE_WRITECOMBINE ) != 0 );
	return isWriteCombined;
}
#endif
// RB end


/*
================================================================================================

	Buffer Objects

================================================================================================
*/

/*
========================
UnbindBufferObjects
========================
*/
void UnbindBufferObjects()
{
	glBindBuffer( GL_ARRAY_BUFFER, 0 );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
}

#if defined(USE_INTRINSICS)

void CopyBuffer( byte* dst, const byte* src, int numBytes )
{
	assert_16_byte_aligned( dst );
	assert_16_byte_aligned( src );
	
	int i = 0;
	for( ; i + 128 <= numBytes; i += 128 )
	{
		__m128i d0 = _mm_load_si128( ( __m128i* )&src[i + 0 * 16] );
		__m128i d1 = _mm_load_si128( ( __m128i* )&src[i + 1 * 16] );
		__m128i d2 = _mm_load_si128( ( __m128i* )&src[i + 2 * 16] );
		__m128i d3 = _mm_load_si128( ( __m128i* )&src[i + 3 * 16] );
		__m128i d4 = _mm_load_si128( ( __m128i* )&src[i + 4 * 16] );
		__m128i d5 = _mm_load_si128( ( __m128i* )&src[i + 5 * 16] );
		__m128i d6 = _mm_load_si128( ( __m128i* )&src[i + 6 * 16] );
		__m128i d7 = _mm_load_si128( ( __m128i* )&src[i + 7 * 16] );
		_mm_stream_si128( ( __m128i* )&dst[i + 0 * 16], d0 );
		_mm_stream_si128( ( __m128i* )&dst[i + 1 * 16], d1 );
		_mm_stream_si128( ( __m128i* )&dst[i + 2 * 16], d2 );
		_mm_stream_si128( ( __m128i* )&dst[i + 3 * 16], d3 );
		_mm_stream_si128( ( __m128i* )&dst[i + 4 * 16], d4 );
		_mm_stream_si128( ( __m128i* )&dst[i + 5 * 16], d5 );
		_mm_stream_si128( ( __m128i* )&dst[i + 6 * 16], d6 );
		_mm_stream_si128( ( __m128i* )&dst[i + 7 * 16], d7 );
	}
	for( ; i + 16 <= numBytes; i += 16 )
	{
		__m128i d = _mm_load_si128( ( __m128i* )&src[i] );
		_mm_stream_si128( ( __m128i* )&dst[i], d );
	}
	for( ; i + 4 <= numBytes; i += 4 )
	{
		*( uint32* )&dst[i] = *( const uint32* )&src[i];
	}
	for( ; i < numBytes; i++ )
	{
		dst[i] = src[i];
	}
	_mm_sfence();
}

#else

void CopyBuffer( byte* dst, const byte* src, int numBytes )
{
	assert_16_byte_aligned( dst );
	assert_16_byte_aligned( src );
	memcpy( dst, src, numBytes );
}

#endif

/*
================================================================================================

	idVertexBuffer

================================================================================================
*/

/*
========================
idVertexBuffer::idVertexBuffer
========================
*/
idVertexBuffer::idVertexBuffer()
{
	size = 0;
	offsetInOtherBuffer = OWNS_BUFFER_FLAG;
	apiObject = NULL;
	SetUnmapped();
}

/*
========================
idVertexBuffer::~idVertexBuffer
========================
*/
idVertexBuffer::~idVertexBuffer()
{
	FreeBufferObject();
}

/*
========================
idVertexBuffer::AllocBufferObject
========================
*/
bool idVertexBuffer::AllocBufferObject( const void* data, int allocSize )
{
	assert( apiObject == NULL );
	assert_16_byte_aligned( data );
	
	if( allocSize <= 0 )
	{
		idLib::Error( "idVertexBuffer::AllocBufferObject: allocSize = %i", allocSize );
	}
	
	size = allocSize;
	
	bool allocationFailed = false;
	
	int numBytes = GetAllocedSize();
	
	
	// clear out any previous error
	glGetError();
	
	GLuint bufferObject = 0xFFFF;
	glGenBuffers( 1, & bufferObject );
	if( bufferObject == 0xFFFF )
	{
		idLib::FatalError( "idVertexBuffer::AllocBufferObject: failed" );
	}
	glBindBuffer( GL_ARRAY_BUFFER, bufferObject );
	
	// these are rewritten every frame
	glBufferData( GL_ARRAY_BUFFER, numBytes, NULL, bufferUsage );
	apiObject = reinterpret_cast< void* >( bufferObject );
	
	GLenum err = glGetError();
	if( err == GL_OUT_OF_MEMORY )
	{
		idLib::Warning( "idVertexBuffer::AllocBufferObject: allocation failed" );
		allocationFailed = true;
	}
	
	
	if( r_showBuffers.GetBool() )
	{
		idLib::Printf( "vertex buffer alloc %p, api %p (%i bytes)\n", this, GetAPIObject(), GetSize() );
	}
	
	// copy the data
	if( data != NULL )
	{
		Update( data, allocSize );
	}
	
	return !allocationFailed;
}

/*
========================
idVertexBuffer::FreeBufferObject
========================
*/
void idVertexBuffer::FreeBufferObject()
{
	if( IsMapped() )
	{
		UnmapBuffer();
	}
	
	// if this is a sub-allocation inside a larger buffer, don't actually free anything.
	if( OwnsBuffer() == false )
	{
		ClearWithoutFreeing();
		return;
	}
	
	if( apiObject == NULL )
	{
		return;
	}
	
	if( r_showBuffers.GetBool() )
	{
		idLib::Printf( "vertex buffer free %p, api %p (%i bytes)\n", this, GetAPIObject(), GetSize() );
	}
	
	// RB: 64 bit fixes, changed GLuint to GLintptrARB
	GLintptr bufferObject = reinterpret_cast< GLintptr >( apiObject );
	glDeleteBuffers( 1, ( const unsigned int* ) & bufferObject );
	// RB end
	
	ClearWithoutFreeing();
}

/*
========================
idVertexBuffer::Reference
========================
*/
void idVertexBuffer::Reference( const idVertexBuffer& other )
{
	assert( IsMapped() == false );
	//assert( other.IsMapped() == false );	// this happens when building idTriangles while at the same time setting up idDrawVerts
	assert( other.GetAPIObject() != NULL );
	assert( other.GetSize() > 0 );
	
	FreeBufferObject();
	size = other.GetSize();						// this strips the MAPPED_FLAG
	offsetInOtherBuffer = other.GetOffset();	// this strips the OWNS_BUFFER_FLAG
	apiObject = other.apiObject;
	assert( OwnsBuffer() == false );
}

/*
========================
idVertexBuffer::Reference
========================
*/
void idVertexBuffer::Reference( const idVertexBuffer& other, int refOffset, int refSize )
{
	assert( IsMapped() == false );
	//assert( other.IsMapped() == false );	// this happens when building idTriangles while at the same time setting up idDrawVerts
	assert( other.GetAPIObject() != NULL );
	assert( refOffset >= 0 );
	assert( refSize >= 0 );
	assert( refOffset + refSize <= other.GetSize() );
	
	FreeBufferObject();
	size = refSize;
	offsetInOtherBuffer = other.GetOffset() + refOffset;
	apiObject = other.apiObject;
	assert( OwnsBuffer() == false );
}

/*
========================
idVertexBuffer::Update
========================
*/
void idVertexBuffer::Update( const void* data, int updateSize ) const
{
	assert( apiObject != NULL );
	assert( IsMapped() == false );
	assert_16_byte_aligned( data );
	assert( ( GetOffset() & 15 ) == 0 );
	
	if( updateSize > size )
	{
		idLib::FatalError( "idVertexBuffer::Update: size overrun, %i > %i\n", updateSize, GetSize() );
	}
	
	int numBytes = ( updateSize + 15 ) & ~15;
	
	// RB: 64 bit fixes, changed GLuint to GLintptrARB
	GLintptr bufferObject = reinterpret_cast< GLintptr >( apiObject );
	// RB end
	
	glBindBuffer( GL_ARRAY_BUFFER, bufferObject );
	glBufferSubData( GL_ARRAY_BUFFER, GetOffset(), ( GLsizeiptr )numBytes, data );
	/*
		void * buffer = MapBuffer( BM_WRITE );
		CopyBuffer( (byte *)buffer + GetOffset(), (byte *)data, numBytes );
		UnmapBuffer();
	*/
}

/*
========================
idVertexBuffer::MapBuffer
========================
*/
void* idVertexBuffer::MapBuffer( bufferMapType_t mapType ) const
{
	assert( apiObject != NULL );
	assert( IsMapped() == false );
	
	void* buffer = NULL;
	
	// RB: 64 bit fixes, changed GLuint to GLintptrARB
	GLintptr bufferObject = reinterpret_cast< GLintptr >( apiObject );
	// RB end
	
	glBindBuffer( GL_ARRAY_BUFFER, bufferObject );
	
	if( mapType == BM_READ )
	{
#if 0 //defined(USE_GLES2)
		buffer = glMapBufferOES( GL_ARRAY_BUFFER, GL_READ_ONLY );
#else
		buffer = glMapBufferRange( GL_ARRAY_BUFFER, 0, GetAllocedSize(), GL_MAP_READ_BIT | GL_MAP_UNSYNCHRONIZED_BIT );
#endif
		if( buffer != NULL )
		{
			buffer = ( byte* )buffer + GetOffset();
		}
	}
	else if( mapType == BM_WRITE )
	{
#if 0 //defined(USE_GLES2)
		buffer = glMapBuffer( GL_ARRAY_BUFFER, GL_WRITE_ONLY );
#else
		// RB: removed GL_MAP_INVALIDATE_RANGE_BIT as it breaks with an optimization in the Nvidia WHQL drivers >= 344.11
		buffer = glMapBufferRange( GL_ARRAY_BUFFER, 0, GetAllocedSize(), GL_MAP_WRITE_BIT /*| GL_MAP_INVALIDATE_RANGE_BIT*/ | GL_MAP_UNSYNCHRONIZED_BIT );
#endif
		if( buffer != NULL )
		{
			buffer = ( byte* )buffer + GetOffset();
		}
		// assert( IsWriteCombined( buffer ) ); // commented out because it spams the console
	}
	else
	{
		assert( false );
	}
	
	SetMapped();
	
	if( buffer == NULL )
	{
		idLib::FatalError( "idVertexBuffer::MapBuffer: failed" );
	}
	return buffer;
}

/*
========================
idVertexBuffer::UnmapBuffer
========================
*/
void idVertexBuffer::UnmapBuffer() const
{
	assert( apiObject != NULL );
	assert( IsMapped() );
	
	// RB: 64 bit fixes, changed GLuint to GLintptrARB
	GLintptr bufferObject = reinterpret_cast< GLintptr >( apiObject );
	// RB end
	
	glBindBuffer( GL_ARRAY_BUFFER, bufferObject );
	if( !glUnmapBuffer( GL_ARRAY_BUFFER ) )
	{
		idLib::Printf( "idVertexBuffer::UnmapBuffer failed\n" );
	}
	
	SetUnmapped();
}

/*
========================
idVertexBuffer::ClearWithoutFreeing
========================
*/
void idVertexBuffer::ClearWithoutFreeing()
{
	size = 0;
	offsetInOtherBuffer = OWNS_BUFFER_FLAG;
	apiObject = NULL;
}

/*
================================================================================================

	idIndexBuffer

================================================================================================
*/

/*
========================
idIndexBuffer::idIndexBuffer
========================
*/
idIndexBuffer::idIndexBuffer()
{
	size = 0;
	offsetInOtherBuffer = OWNS_BUFFER_FLAG;
	apiObject = NULL;
	SetUnmapped();
}

/*
========================
idIndexBuffer::~idIndexBuffer
========================
*/
idIndexBuffer::~idIndexBuffer()
{
	FreeBufferObject();
}

/*
========================
idIndexBuffer::AllocBufferObject
========================
*/
bool idIndexBuffer::AllocBufferObject( const void* data, int allocSize )
{
	assert( apiObject == NULL );
	assert_16_byte_aligned( data );
	
	if( allocSize <= 0 )
	{
		idLib::Error( "idIndexBuffer::AllocBufferObject: allocSize = %i", allocSize );
	}
	
	size = allocSize;
	
	bool allocationFailed = false;
	
	int numBytes = GetAllocedSize();
	
	
	// clear out any previous error
	glGetError();
	
	GLuint bufferObject = 0xFFFF;
	glGenBuffers( 1, & bufferObject );
	if( bufferObject == 0xFFFF )
	{
		GLenum error = glGetError();
		idLib::FatalError( "idIndexBuffer::AllocBufferObject: failed - GL_Error %d", error );
	}
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, bufferObject );
	
	// these are rewritten every frame
	glBufferData( GL_ELEMENT_ARRAY_BUFFER, numBytes, NULL, bufferUsage );
	apiObject = reinterpret_cast< void* >( bufferObject );
	
	GLenum err = glGetError();
	if( err == GL_OUT_OF_MEMORY )
	{
		idLib::Warning( "idIndexBuffer:AllocBufferObject: allocation failed" );
		allocationFailed = true;
	}
	
	
	if( r_showBuffers.GetBool() )
	{
		idLib::Printf( "index buffer alloc %p, api %p (%i bytes)\n", this, GetAPIObject(), GetSize() );
	}
	
	// copy the data
	if( data != NULL )
	{
		Update( data, allocSize );
	}
	
	return !allocationFailed;
}

/*
========================
idIndexBuffer::FreeBufferObject
========================
*/
void idIndexBuffer::FreeBufferObject()
{
	if( IsMapped() )
	{
		UnmapBuffer();
	}
	
	// if this is a sub-allocation inside a larger buffer, don't actually free anything.
	if( OwnsBuffer() == false )
	{
		ClearWithoutFreeing();
		return;
	}
	
	if( apiObject == NULL )
	{
		return;
	}
	
	if( r_showBuffers.GetBool() )
	{
		idLib::Printf( "index buffer free %p, api %p (%i bytes)\n", this, GetAPIObject(), GetSize() );
	}
	
	// RB: 64 bit fixes, changed GLuint to GLintptrARB
	GLintptr bufferObject = reinterpret_cast< GLintptr >( apiObject );
	glDeleteBuffers( 1, ( const unsigned int* )& bufferObject );
	// RB end
	
	ClearWithoutFreeing();
}

/*
========================
idIndexBuffer::Reference
========================
*/
void idIndexBuffer::Reference( const idIndexBuffer& other )
{
	assert( IsMapped() == false );
	//assert( other.IsMapped() == false );	// this happens when building idTriangles while at the same time setting up triIndex_t
	assert( other.GetAPIObject() != NULL );
	assert( other.GetSize() > 0 );
	
	FreeBufferObject();
	size = other.GetSize();						// this strips the MAPPED_FLAG
	offsetInOtherBuffer = other.GetOffset();	// this strips the OWNS_BUFFER_FLAG
	apiObject = other.apiObject;
	assert( OwnsBuffer() == false );
}

/*
========================
idIndexBuffer::Reference
========================
*/
void idIndexBuffer::Reference( const idIndexBuffer& other, int refOffset, int refSize )
{
	assert( IsMapped() == false );
	//assert( other.IsMapped() == false );	// this happens when building idTriangles while at the same time setting up triIndex_t
	assert( other.GetAPIObject() != NULL );
	assert( refOffset >= 0 );
	assert( refSize >= 0 );
	assert( refOffset + refSize <= other.GetSize() );
	
	FreeBufferObject();
	size = refSize;
	offsetInOtherBuffer = other.GetOffset() + refOffset;
	apiObject = other.apiObject;
	assert( OwnsBuffer() == false );
}

/*
========================
idIndexBuffer::Update
========================
*/
void idIndexBuffer::Update( const void* data, int updateSize ) const
{

	assert( apiObject != NULL );
	assert( IsMapped() == false );
	assert_16_byte_aligned( data );
	assert( ( GetOffset() & 15 ) == 0 );
	
	if( updateSize > size )
	{
		idLib::FatalError( "idIndexBuffer::Update: size overrun, %i > %i\n", updateSize, GetSize() );
	}
	
	int numBytes = ( updateSize + 15 ) & ~15;
	
	// RB: 64 bit fixes, changed GLuint to GLintptrARB
	GLintptr bufferObject = reinterpret_cast< GLintptr >( apiObject );
	// RB end
	
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, bufferObject );
	glBufferSubData( GL_ELEMENT_ARRAY_BUFFER, GetOffset(), ( GLsizeiptr )numBytes, data );
	/*
		void * buffer = MapBuffer( BM_WRITE );
		CopyBuffer( (byte *)buffer + GetOffset(), (byte *)data, numBytes );
		UnmapBuffer();
	*/
}

/*
========================
idIndexBuffer::MapBuffer
========================
*/
void* idIndexBuffer::MapBuffer( bufferMapType_t mapType ) const
{

	assert( apiObject != NULL );
	assert( IsMapped() == false );
	
	void* buffer = NULL;
	
	// RB: 64 bit fixes, changed GLuint to GLintptrARB
	GLintptr bufferObject = reinterpret_cast< GLintptr >( apiObject );
	// RB end
	
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, bufferObject );
	
	if( mapType == BM_READ )
	{
		//buffer = glMapBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, GL_READ_ONLY_ARB );
		buffer = glMapBufferRange( GL_ELEMENT_ARRAY_BUFFER, 0, GetAllocedSize(), GL_MAP_READ_BIT | GL_MAP_UNSYNCHRONIZED_BIT );
		if( buffer != NULL )
		{
			buffer = ( byte* )buffer + GetOffset();
		}
	}
	else if( mapType == BM_WRITE )
	{
		//buffer = glMapBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, GL_WRITE_ONLY_ARB );
		
		// RB: removed GL_MAP_INVALIDATE_RANGE_BIT as it breaks with an optimization in the Nvidia WHQL drivers >= 344.11
		buffer = glMapBufferRange( GL_ELEMENT_ARRAY_BUFFER, 0, GetAllocedSize(), GL_MAP_WRITE_BIT /*| GL_MAP_INVALIDATE_RANGE_BIT*/ | GL_MAP_UNSYNCHRONIZED_BIT );
		if( buffer != NULL )
		{
			buffer = ( byte* )buffer + GetOffset();
		}
		// assert( IsWriteCombined( buffer ) ); // commented out because it spams the console
	}
	else
	{
		assert( false );
	}
	
	SetMapped();
	
	if( buffer == NULL )
	{
		idLib::FatalError( "idIndexBuffer::MapBuffer: failed" );
	}
	return buffer;
}

/*
========================
idIndexBuffer::UnmapBuffer
========================
*/
void idIndexBuffer::UnmapBuffer() const
{
	assert( apiObject != NULL );
	assert( IsMapped() );
	
	// RB: 64 bit fixes, changed GLuint to GLintptrARB
	GLintptr bufferObject = reinterpret_cast< GLintptr >( apiObject );
	// RB end
	
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, bufferObject );
	if( !glUnmapBuffer( GL_ELEMENT_ARRAY_BUFFER ) )
	{
		idLib::Printf( "idIndexBuffer::UnmapBuffer failed\n" );
	}
	
	SetUnmapped();
}

/*
========================
idIndexBuffer::ClearWithoutFreeing
========================
*/
void idIndexBuffer::ClearWithoutFreeing()
{
	size = 0;
	offsetInOtherBuffer = OWNS_BUFFER_FLAG;
	apiObject = NULL;
}

/*
================================================================================================

	idJointBuffer

================================================================================================
*/

/*
========================
idJointBuffer::idJointBuffer
========================
*/
idJointBuffer::idJointBuffer()
{
	numJoints = 0;
	offsetInOtherBuffer = OWNS_BUFFER_FLAG;
	apiObject = NULL;
	SetUnmapped();
}

/*
========================
idJointBuffer::~idJointBuffer
========================
*/
idJointBuffer::~idJointBuffer()
{
	FreeBufferObject();
}

/*
========================
idJointBuffer::AllocBufferObject
========================
*/
bool idJointBuffer::AllocBufferObject( const float* joints, int numAllocJoints )
{
	assert( apiObject == NULL );
	assert_16_byte_aligned( joints );
	
	if( numAllocJoints <= 0 )
	{
		idLib::Error( "idJointBuffer::AllocBufferObject: joints = %i", numAllocJoints );
	}
	
	numJoints = numAllocJoints;
	
	bool allocationFailed = false;
	
	const int numBytes = GetAllocedSize();
	
	GLuint buffer = 0;
	glGenBuffers( 1, &buffer );
	glBindBuffer( GL_UNIFORM_BUFFER, buffer );
	glBufferData( GL_UNIFORM_BUFFER, numBytes, NULL, GL_STREAM_DRAW );
	glBindBuffer( GL_UNIFORM_BUFFER, 0 );
	apiObject = reinterpret_cast< void* >( buffer );
	
	if( r_showBuffers.GetBool() )
	{
		idLib::Printf( "joint buffer alloc %p, api %p (%i joints)\n", this, GetAPIObject(), GetNumJoints() );
	}
	
	// copy the data
	if( joints != NULL )
	{
		Update( joints, numAllocJoints );
	}
	
	return !allocationFailed;
}

/*
========================
idJointBuffer::FreeBufferObject
========================
*/
void idJointBuffer::FreeBufferObject()
{
	if( IsMapped() )
	{
		UnmapBuffer();
	}
	
	// if this is a sub-allocation inside a larger buffer, don't actually free anything.
	if( OwnsBuffer() == false )
	{
		ClearWithoutFreeing();
		return;
	}
	
	if( apiObject == NULL )
	{
		return;
	}
	
	if( r_showBuffers.GetBool() )
	{
		idLib::Printf( "joint buffer free %p, api %p (%i joints)\n", this, GetAPIObject(), GetNumJoints() );
	}
	
	// RB: 64 bit fixes, changed GLuint to GLintptrARB
	GLintptr buffer = reinterpret_cast< GLintptr >( apiObject );
	
	glBindBuffer( GL_UNIFORM_BUFFER, 0 );
	glDeleteBuffers( 1, ( const GLuint* )& buffer );
	// RB end
	
	ClearWithoutFreeing();
}

/*
========================
idJointBuffer::Reference
========================
*/
void idJointBuffer::Reference( const idJointBuffer& other )
{
	assert( IsMapped() == false );
	assert( other.IsMapped() == false );
	assert( other.GetAPIObject() != NULL );
	assert( other.GetNumJoints() > 0 );
	
	FreeBufferObject();
	numJoints = other.GetNumJoints();			// this strips the MAPPED_FLAG
	offsetInOtherBuffer = other.GetOffset();	// this strips the OWNS_BUFFER_FLAG
	apiObject = other.apiObject;
	assert( OwnsBuffer() == false );
}

/*
========================
idJointBuffer::Reference
========================
*/
void idJointBuffer::Reference( const idJointBuffer& other, int jointRefOffset, int numRefJoints )
{
	assert( IsMapped() == false );
	assert( other.IsMapped() == false );
	assert( other.GetAPIObject() != NULL );
	assert( jointRefOffset >= 0 );
	assert( numRefJoints >= 0 );
	assert( jointRefOffset + numRefJoints * sizeof( idJointMat ) <= other.GetNumJoints() * sizeof( idJointMat ) );
	assert_16_byte_aligned( numRefJoints * 3 * 4 * sizeof( float ) );
	
	FreeBufferObject();
	numJoints = numRefJoints;
	offsetInOtherBuffer = other.GetOffset() + jointRefOffset;
	apiObject = other.apiObject;
	assert( OwnsBuffer() == false );
}

/*
========================
idJointBuffer::Update
========================
*/
void idJointBuffer::Update( const float* joints, int numUpdateJoints ) const
{
	assert( apiObject != NULL );
	assert( IsMapped() == false );
	assert_16_byte_aligned( joints );
	assert( ( GetOffset() & 15 ) == 0 );
	
	if( numUpdateJoints > numJoints )
	{
		idLib::FatalError( "idJointBuffer::Update: size overrun, %i > %i\n", numUpdateJoints, numJoints );
	}
	
	const int numBytes = numUpdateJoints * 3 * 4 * sizeof( float );
	
	// RB: 64 bit fixes, changed GLuint to GLintptrARB
	glBindBuffer( GL_UNIFORM_BUFFER, reinterpret_cast< GLintptr >( apiObject ) );
	// RB end
	
	glBufferSubData( GL_UNIFORM_BUFFER, GetOffset(), ( GLsizeiptr )numBytes, joints );
}

/*
========================
idJointBuffer::MapBuffer
========================
*/
float* idJointBuffer::MapBuffer( bufferMapType_t mapType ) const
{
	assert( IsMapped() == false );
	assert( mapType == BM_WRITE );
	assert( apiObject != NULL );
	
	int numBytes = GetAllocedSize();
	
	void* buffer = NULL;
	
	// RB: 64 bit fixes, changed GLuint to GLintptrARB
	glBindBuffer( GL_UNIFORM_BUFFER, reinterpret_cast< GLintptr >( apiObject ) );
	// RB end
	
	numBytes = numBytes;
	assert( GetOffset() == 0 );
	//buffer = glMapBufferARB( GL_UNIFORM_BUFFER, GL_WRITE_ONLY_ARB );
	
	// RB: removed GL_MAP_INVALIDATE_RANGE_BIT as it breaks with an optimization in the Nvidia WHQL drivers >= 344.11
	buffer = glMapBufferRange( GL_UNIFORM_BUFFER, 0, GetAllocedSize(), GL_MAP_WRITE_BIT /*| GL_MAP_INVALIDATE_RANGE_BIT*/ | GL_MAP_UNSYNCHRONIZED_BIT );
	if( buffer != NULL )
	{
		buffer = ( byte* )buffer + GetOffset();
	}
	
	SetMapped();
	
	if( buffer == NULL )
	{
		idLib::FatalError( "idJointBuffer::MapBuffer: failed" );
	}
	return ( float* ) buffer;
}

/*
========================
idJointBuffer::UnmapBuffer
========================
*/
void idJointBuffer::UnmapBuffer() const
{
	assert( apiObject != NULL );
	assert( IsMapped() );
	
	// RB: 64 bit fixes, changed GLuint to GLintptrARB
	glBindBuffer( GL_UNIFORM_BUFFER, reinterpret_cast< GLintptr >( apiObject ) );
	// RB end
	
	if( !glUnmapBuffer( GL_UNIFORM_BUFFER ) )
	{
		idLib::Printf( "idJointBuffer::UnmapBuffer failed\n" );
	}
	
	SetUnmapped();
}

/*
========================
idJointBuffer::ClearWithoutFreeing
========================
*/
void idJointBuffer::ClearWithoutFreeing()
{
	numJoints = 0;
	offsetInOtherBuffer = OWNS_BUFFER_FLAG;
	apiObject = NULL;
}

/*
========================
idJointBuffer::Swap
========================
*/
void idJointBuffer::Swap( idJointBuffer& other )
{
	// Make sure the ownership of the buffer is not transferred to an unintended place.
	assert( other.OwnsBuffer() == OwnsBuffer() );
	
	SwapValues( other.numJoints, numJoints );
	SwapValues( other.offsetInOtherBuffer, offsetInOtherBuffer );
	SwapValues( other.apiObject, apiObject );
}
