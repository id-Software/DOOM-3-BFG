/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013 Robert Beckebans
Copyright (C) 2016-2017 Dustin Land

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
#include "../RenderCommon.h"

extern idCVar r_showBuffers;


//static const GLenum bufferUsage = GL_STATIC_DRAW;
static const GLenum bufferUsage = GL_DYNAMIC_DRAW;




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
	apiObject = 0xFFFF;
	SetUnmapped();
}

/*
========================
idVertexBuffer::AllocBufferObject
========================
*/
bool idVertexBuffer::AllocBufferObject( const void* data, int allocSize, bufferUsageType_t _usage )
{
	assert( apiObject == 0xFFFF );
	assert_16_byte_aligned( data );

	if( allocSize <= 0 )
	{
		idLib::Error( "idVertexBuffer::AllocBufferObject: allocSize = %i", allocSize );
	}

	size = allocSize;
	usage = _usage;

	bool allocationFailed = false;

	int numBytes = GetAllocedSize();

	// clear out any previous error
	GL_CheckErrors();

	glGenBuffers( 1, ( GLuint* ) &apiObject );
	if( apiObject == 0xFFFF )
	{
		idLib::FatalError( "idVertexBuffer::AllocBufferObject: failed" );
	}
	glBindBuffer( GL_ARRAY_BUFFER, apiObject );

	// these are rewritten every frame
	glBufferDataARB( GL_ARRAY_BUFFER, numBytes, NULL, bufferUsage );

	GLenum err = glGetError();
	if( err == GL_OUT_OF_MEMORY )
	{
		idLib::Warning( "idVertexBuffer::AllocBufferObject: allocation failed" );
		allocationFailed = true;
	}

	if( r_showBuffers.GetBool() )
	{
		idLib::Printf( "vertex buffer alloc %p, api %p (%i bytes)\n", this, ( GLuint* )&apiObject, GetSize() );
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

	if( apiObject == 0xFFFF )
	{
		return;
	}

	if( r_showBuffers.GetBool() )
	{
		idLib::Printf( "vertex buffer free %p, api %p (%i bytes)\n", this, ( GLuint* )&apiObject, GetSize() );
	}

	glDeleteBuffers( 1, ( GLuint* )&apiObject );

	ClearWithoutFreeing();
}

/*
========================
idVertexBuffer::Update
========================
*/
void idVertexBuffer::Update( const void* data, int updateSize, int offset ) const
{
	assert( apiObject != 0xFFFF );
	assert_16_byte_aligned( data );
	assert( ( GetOffset() & 15 ) == 0 );

	if( updateSize > GetSize() )
	{
		idLib::FatalError( "idVertexBuffer::Update: size overrun, %i > %i\n", updateSize, GetSize() );
	}

	int numBytes = ( updateSize + 15 ) & ~15;

	if( usage == BU_DYNAMIC )
	{
		CopyBuffer( ( byte* )buffer + offset, ( const byte* )data, numBytes );
	}
	else
	{
		glBindBuffer( GL_ARRAY_BUFFER, apiObject );
		glBufferSubData( GL_ARRAY_BUFFER, GetOffset() + offset, ( GLsizeiptrARB )numBytes, data );
	}
}

/*
========================
idVertexBuffer::MapBuffer
========================
*/
void* idVertexBuffer::MapBuffer( bufferMapType_t mapType )
{
	assert( apiObject != 0xFFFF );
	assert( IsMapped() == false );

	buffer = NULL;

	glBindBuffer( GL_ARRAY_BUFFER, apiObject );
	if( mapType == BM_READ )
	{
		buffer = glMapBufferRange( GL_ARRAY_BUFFER_ARB, 0, GetAllocedSize(), GL_MAP_READ_BIT | GL_MAP_UNSYNCHRONIZED_BIT );
		if( buffer != NULL )
		{
			buffer = ( byte* )buffer + GetOffset();
		}
	}
	else if( mapType == BM_WRITE )
	{
		// RB: removed GL_MAP_INVALIDATE_RANGE_BIT as it breaks with an optimization in the Nvidia WHQL drivers >= 344.11
		buffer = glMapBufferRange( GL_ARRAY_BUFFER, 0, GetAllocedSize(), GL_MAP_WRITE_BIT /*| GL_MAP_INVALIDATE_RANGE_BIT*/ | GL_MAP_UNSYNCHRONIZED_BIT );
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
void idVertexBuffer::UnmapBuffer()
{
	assert( apiObject != 0xFFFF );
	assert( IsMapped() );

	glBindBuffer( GL_ARRAY_BUFFER, apiObject );
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
	apiObject = 0xFFFF;
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
	apiObject = 0xFFFF;
	SetUnmapped();
}

/*
========================
idIndexBuffer::AllocBufferObject
========================
*/
bool idIndexBuffer::AllocBufferObject( const void* data, int allocSize, bufferUsageType_t _usage )
{
	assert( apiObject == 0xFFFF );
	assert_16_byte_aligned( data );

	if( allocSize <= 0 )
	{
		idLib::Error( "idIndexBuffer::AllocBufferObject: allocSize = %i", allocSize );
	}

	size = allocSize;
	usage = _usage;

	bool allocationFailed = false;

	int numBytes = GetAllocedSize();


	// clear out any previous error
	GL_CheckErrors();

	glGenBuffersARB( 1, ( GLuint* )&apiObject );
	if( apiObject == 0xFFFF )
	{
		GLenum error = glGetError();
		idLib::FatalError( "idIndexBuffer::AllocBufferObject: failed - GL_Error %d", error );
	}
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, apiObject );

	// these are rewritten every frame
	glBufferData( GL_ELEMENT_ARRAY_BUFFER, numBytes, NULL, bufferUsage );

	GLenum err = glGetError();
	if( err == GL_OUT_OF_MEMORY )
	{
		idLib::Warning( "idIndexBuffer:AllocBufferObject: allocation failed" );
		allocationFailed = true;
	}

	if( r_showBuffers.GetBool() )
	{
		idLib::Printf( "index buffer alloc %p, api %p (%i bytes)\n", this, ( GLuint* )&apiObject, GetSize() );
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

	if( apiObject == 0xFFFF )
	{
		return;
	}

	if( r_showBuffers.GetBool() )
	{
		idLib::Printf( "index buffer free %p, api %p (%i bytes)\n", this, ( GLuint* )&apiObject, GetSize() );
	}

	glDeleteBuffers( 1, ( GLuint* )&apiObject );

	ClearWithoutFreeing();
}

/*
========================
idIndexBuffer::Update
========================
*/
void idIndexBuffer::Update( const void* data, int updateSize, int offset ) const
{
	assert( apiObject != 0xFFFF );
	assert_16_byte_aligned( data );
	assert( ( GetOffset() & 15 ) == 0 );

	if( updateSize > GetSize() )
	{
		idLib::FatalError( "idIndexBuffer::Update: size overrun, %i > %i\n", updateSize, GetSize() );
	}

	int numBytes = ( updateSize + 15 ) & ~15;

	if( usage == BU_DYNAMIC )
	{
		CopyBuffer( ( byte* )buffer + offset, ( const byte* )data, numBytes );
	}
	else
	{
		glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, apiObject );
		glBufferSubData( GL_ELEMENT_ARRAY_BUFFER, GetOffset() + offset, ( GLsizeiptrARB )numBytes, data );
	}
}

/*
========================
idIndexBuffer::MapBuffer
========================
*/
void* idIndexBuffer::MapBuffer( bufferMapType_t mapType )
{
	assert( apiObject != 0xFFFF );
	assert( IsMapped() == false );

	buffer = NULL;

	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, apiObject );
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
void idIndexBuffer::UnmapBuffer()
{
	assert( apiObject != 0xFFFF );
	assert( IsMapped() );

	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, apiObject );
	if( !glUnmapBuffer( GL_ELEMENT_ARRAY_BUFFER ) )
	{
		idLib::Printf( "idIndexBuffer::UnmapBuffer failed\n" );
	}

	buffer = NULL;

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
	apiObject = 0xFFFF;
}

/*
================================================================================================

idUniformBuffer

================================================================================================
*/

/*
========================
idUniformBuffer::idUniformBuffer
========================
*/
idUniformBuffer::idUniformBuffer()
{
	size = 0;
	offsetInOtherBuffer = OWNS_BUFFER_FLAG;
	apiObject = 0xFFFF;
	SetUnmapped();
}

/*
========================
idUniformBuffer::AllocBufferObject
========================
*/
bool idUniformBuffer::AllocBufferObject( const void* data, int allocSize, bufferUsageType_t _usage )
{
	assert( apiObject == 0xFFFF );
	assert_16_byte_aligned( data );

	if( allocSize <= 0 )
	{
		idLib::Error( "idUniformBuffer::AllocBufferObject: allocSize = %i", allocSize );
	}

	size = allocSize;
	usage = _usage;

	bool allocationFailed = false;

	const int numBytes = GetAllocedSize();

	glGenBuffers( 1, ( GLuint* )&apiObject );
	glBindBuffer( GL_UNIFORM_BUFFER, apiObject );
	glBufferData( GL_UNIFORM_BUFFER, numBytes, NULL, GL_STREAM_DRAW_ARB );
	glBindBuffer( GL_UNIFORM_BUFFER, 0 );

	if( r_showBuffers.GetBool() )
	{
		idLib::Printf( "joint buffer alloc %p, api %p (%i joints)\n", this, ( GLuint* )&apiObject, GetSize() );
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
idUniformBuffer::FreeBufferObject
========================
*/
void idUniformBuffer::FreeBufferObject()
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

	if( apiObject == 0xFFFF )
	{
		return;
	}

	if( r_showBuffers.GetBool() )
	{
		idLib::Printf( "joint buffer free %p, api %p (%i size)\n", this, ( GLuint* )&apiObject, GetSize() );
	}

	glBindBuffer( GL_UNIFORM_BUFFER, 0 );
	glDeleteBuffers( 1, ( GLuint* )&apiObject );

	ClearWithoutFreeing();
}

/*
========================
idUniformBuffer::Update
========================
*/
void idUniformBuffer::Update( const void* data, int updateSize, int offset ) const
{
	assert( apiObject != 0xFFFF );
	assert_16_byte_aligned( data );
	assert( ( GetOffset() & 15 ) == 0 );

	if( updateSize > GetSize() )
	{
		idLib::FatalError( "idUniformBuffer::Update: size overrun, %i > %i\n", updateSize, GetSize() );
	}

	const int numBytes = ( updateSize + 15 ) & ~15;

	if( usage == BU_DYNAMIC )
	{
		CopyBuffer( ( byte* )buffer + offset, ( const byte* )data, numBytes );
	}
	else
	{
		glBindBuffer( GL_ARRAY_BUFFER, apiObject );
		glBufferSubData( GL_ARRAY_BUFFER, GetOffset() + offset, ( GLsizeiptr )numBytes, data );
	}
}

/*
========================
idUniformBuffer::MapBuffer
========================
*/
void* idUniformBuffer::MapBuffer( bufferMapType_t mapType )
{
	assert( IsMapped() == false );
	assert( mapType == BM_WRITE );
	assert( apiObject != 0xFFFF );

	int numBytes = GetAllocedSize();

	buffer = NULL;

	glBindBuffer( GL_UNIFORM_BUFFER, apiObject );
	numBytes = numBytes;
	assert( GetOffset() == 0 );

	// RB: removed GL_MAP_INVALIDATE_RANGE_BIT as it breaks with an optimization in the Nvidia WHQL drivers >= 344.11
	buffer = glMapBufferRange( GL_UNIFORM_BUFFER, 0, GetAllocedSize(), GL_MAP_WRITE_BIT /*| GL_MAP_INVALIDATE_RANGE_BIT*/ | GL_MAP_UNSYNCHRONIZED_BIT );
	if( buffer != NULL )
	{
		buffer = ( byte* )buffer + GetOffset();
	}

	SetMapped();

	if( buffer == NULL )
	{
		idLib::FatalError( "idUniformBuffer::MapBuffer: failed" );
	}
	return ( float* ) buffer;
}

/*
========================
idUniformBuffer::UnmapBuffer
========================
*/
void idUniformBuffer::UnmapBuffer()
{
	assert( apiObject != 0xFFFF );
	assert( IsMapped() );

	glBindBuffer( GL_UNIFORM_BUFFER, apiObject );
	if( !glUnmapBuffer( GL_UNIFORM_BUFFER ) )
	{
		idLib::Printf( "idUniformBuffer::UnmapBuffer failed\n" );
	}

	buffer = NULL;

	SetUnmapped();
}

/*
========================
idUniformBuffer::ClearWithoutFreeing
========================
*/
void idUniformBuffer::ClearWithoutFreeing()
{
	size = 0;
	offsetInOtherBuffer = OWNS_BUFFER_FLAG;
	apiObject = 0xFFFF;
}
