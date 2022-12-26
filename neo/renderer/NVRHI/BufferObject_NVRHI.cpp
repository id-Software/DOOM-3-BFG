/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013 Robert Beckebans
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
#include "precompiled.h"
#pragma hdrstop
#include "../RenderCommon.h"

#include "sys/DeviceManager.h"

#include <stddef.h>

extern idCVar r_showBuffers;

extern DeviceManager* deviceManager;

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
	//glBindBuffer( GL_ARRAY_BUFFER, 0 );
	//glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
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
	bufferHandle.Reset();
	SetUnmapped();
}

/*
========================
idVertexBuffer::AllocBufferObject
========================
*/
bool idVertexBuffer::AllocBufferObject( const void* data, int allocSize, bufferUsageType_t _usage, nvrhi::ICommandList* commandList )
{
	assert( !bufferHandle );
	assert_16_byte_aligned( data );

	if( allocSize <= 0 )
	{
		idLib::Error( "idVertexBuffer::AllocBufferObject: allocSize = %i", allocSize );
	}

	size = allocSize;
	usage = _usage;

	bool allocationFailed = false;

	int numBytes = GetAllocedSize();

	nvrhi::BufferDesc vertexBufferDesc;
	vertexBufferDesc.byteSize = numBytes;
	vertexBufferDesc.isVertexBuffer = true;

	if( usage == BU_DYNAMIC )
	{
		vertexBufferDesc.initialState = nvrhi::ResourceStates::CopyDest;
		vertexBufferDesc.cpuAccess = nvrhi::CpuAccessMode::Write;
		vertexBufferDesc.debugName = "Mapped idDrawVert vertex buffer";
	}
	else
	{
		vertexBufferDesc.initialState = nvrhi::ResourceStates::CopyDest;
		vertexBufferDesc.keepInitialState = true;
		vertexBufferDesc.debugName = "Static idDrawVert vertex buffer";
	}

	bufferHandle = deviceManager->GetDevice()->createBuffer( vertexBufferDesc );

	if( r_showBuffers.GetBool() )
	{
		idLib::Printf( "vertex buffer alloc %p, api %p (%i bytes)\n", this, bufferHandle.Get(), GetSize() );
	}

	// copy the data
	if( data )
	{
		Update( data, allocSize, 0, true, commandList );
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

	if( !bufferHandle )
	{
		return;
	}

	if( r_showBuffers.GetBool() )
	{
		idLib::Printf( "vertex buffer free %p, api %p (%i bytes)\n", this, bufferHandle.Get(), GetSize() );
	}

	bufferHandle.Reset();

	ClearWithoutFreeing();
}

/*
========================
idVertexBuffer::Update
========================
*/
void idVertexBuffer::Update( const void* data, int updateSize, int offset, bool initialUpdate, nvrhi::ICommandList* commandList ) const
{
	assert( bufferHandle );
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
		if( initialUpdate )
		{
			commandList->beginTrackingBufferState( bufferHandle, nvrhi::ResourceStates::CopyDest );
			commandList->writeBuffer( bufferHandle, data, numBytes, GetOffset() + offset );
			commandList->setPermanentBufferState( bufferHandle, nvrhi::ResourceStates::VertexBuffer );
		}
		else
		{
			commandList->writeBuffer( bufferHandle, data, numBytes, GetOffset() + offset );
		}
	}
}

/*
========================
idVertexBuffer::MapBuffer
========================
*/
void* idVertexBuffer::MapBuffer( bufferMapType_t mapType )
{
	assert( bufferHandle );
	assert( IsMapped() == false );

	nvrhi::CpuAccessMode accessMode = nvrhi::CpuAccessMode::Write;
	if( mapType == bufferMapType_t::BM_READ )
	{
		accessMode = nvrhi::CpuAccessMode::Read;
	}

	buffer = deviceManager->GetDevice()->mapBuffer( bufferHandle, accessMode, { ( uint64 )GetOffset(), ( uint64 )GetAllocedSize() } );

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
	assert( bufferHandle );
	assert( IsMapped() );

	if( deviceManager && deviceManager->GetDevice() )
	{
		deviceManager->GetDevice()->unmapBuffer( bufferHandle );
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
	bufferHandle.Reset();
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
	bufferHandle.Reset();
	SetUnmapped();
}

/*
========================
idIndexBuffer::AllocBufferObject
========================
*/
bool idIndexBuffer::AllocBufferObject( const void* data, int allocSize, bufferUsageType_t _usage, nvrhi::ICommandList* commandList )
{
	assert( !bufferHandle );
	assert_16_byte_aligned( data );

	if( allocSize <= 0 )
	{
		idLib::Error( "idIndexBuffer::AllocBufferObject: allocSize = %i", allocSize );
	}

	size = allocSize;
	usage = _usage;

	int numBytes = GetAllocedSize();

	nvrhi::BufferDesc indexBufferDesc;
	indexBufferDesc.byteSize = numBytes;
	indexBufferDesc.isIndexBuffer = true;
	indexBufferDesc.initialState = nvrhi::ResourceStates::CopyDest;
	indexBufferDesc.canHaveRawViews = true;
	indexBufferDesc.canHaveTypedViews = true;
	indexBufferDesc.format = nvrhi::Format::R16_UINT;

	if( _usage == BU_STATIC )
	{
		indexBufferDesc.keepInitialState = true;
		indexBufferDesc.debugName = "VertexCache Static Index Buffer";
	}
	else if( _usage == BU_DYNAMIC )
	{
		indexBufferDesc.cpuAccess = nvrhi::CpuAccessMode::Write;
		indexBufferDesc.debugName = "VertexCache Mapped Index Buffer";
	}

	bufferHandle  = deviceManager->GetDevice()->createBuffer( indexBufferDesc );

	if( data )
	{
		Update( data, allocSize, 0, true, commandList );
	}

	return true;
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

	if( !bufferHandle )
	{
		return;
	}

	if( r_showBuffers.GetBool() )
	{
		idLib::Printf( "index buffer free %p, api %p (%i bytes)\n", this, bufferHandle.Get(), GetSize() );
	}

	bufferHandle.Reset();

	ClearWithoutFreeing();
}

/*
========================
idIndexBuffer::Update
========================
*/
void idIndexBuffer::Update( const void* data, int updateSize, int offset, bool initialUpdate, nvrhi::ICommandList* commandList ) const
{
	assert( bufferHandle );
	assert_16_byte_aligned( data );
	assert( ( GetOffset() & 15 ) == 0 );

	if( updateSize > GetSize() )
	{
		idLib::FatalError( "idIndexBuffer::Update: size overrun, %i > %i\n", updateSize, GetSize() );
	}

	const int numBytes = ( updateSize + 15 ) & ~15;

	if( usage == BU_DYNAMIC )
	{
		CopyBuffer( ( byte* )buffer + offset, ( const byte* )data, numBytes );
	}
	else
	{
		if( initialUpdate )
		{
			commandList->beginTrackingBufferState( bufferHandle, nvrhi::ResourceStates::CopyDest );
			commandList->writeBuffer( bufferHandle, data, numBytes, GetOffset() + offset );
			commandList->setPermanentBufferState( bufferHandle, nvrhi::ResourceStates::IndexBuffer );
			commandList->commitBarriers();
		}
		else
		{
			commandList->writeBuffer( bufferHandle, data, numBytes, GetOffset() + offset );
		}
	}
}

/*
========================
idIndexBuffer::MapBuffer
========================
*/
void* idIndexBuffer::MapBuffer( bufferMapType_t mapType )
{
	assert( bufferHandle );
	assert( IsMapped() == false );

	nvrhi::CpuAccessMode accessMode = nvrhi::CpuAccessMode::Write;
	if( mapType == BM_READ )
	{
		accessMode = nvrhi::CpuAccessMode::Read;
	}

	buffer = deviceManager->GetDevice()->mapBuffer( bufferHandle, accessMode, { ( uint64 )GetOffset(), ( uint64 )GetAllocedSize() } );

	SetMapped();

	if( buffer == nullptr )
	{
		idLib::FatalError( "idVertexBuffer::MapBuffer: failed" );
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
	assert( bufferHandle );
	assert( IsMapped() );

	if( deviceManager && deviceManager->GetDevice() )
	{
		deviceManager->GetDevice()->unmapBuffer( bufferHandle );
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
	bufferHandle.Reset();
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
	bufferHandle.Reset();
	SetUnmapped();
	SetDebugName( "Uniform Buffer" );
}

/*
========================
idUniformBuffer::AllocBufferObject
========================
*/
bool idUniformBuffer::AllocBufferObject( const void* data, int allocSize, bufferUsageType_t allocatedUsage, nvrhi::ICommandList* commandList )
{
	assert( !bufferHandle );
	assert_16_byte_aligned( data );

	if( allocSize <= 0 )
	{
		idLib::Error( "idIndexBuffer::AllocBufferObject: allocSize = %i", allocSize );
	}

	size = allocSize;
	usage = allocatedUsage;

	const int numBytes = GetAllocedSize();

	// This buffer is a shader resource as opposed to a constant buffer due to
	// constant buffers not being able to be sub-ranged.
	nvrhi::BufferDesc bufferDesc;
	bufferDesc.initialState = nvrhi::ResourceStates::ConstantBuffer;
	bufferDesc.keepInitialState = true;
	bufferDesc.canHaveTypedViews = true;
	bufferDesc.canHaveRawViews = true;
	bufferDesc.byteSize = numBytes;
	bufferDesc.structStride = sizeof( idVec4 );
	bufferDesc.isConstantBuffer = true;

	if( usage == BU_DYNAMIC )
	{
		bufferDesc.debugName = debugName.c_str();
		bufferDesc.initialState = nvrhi::ResourceStates::CopyDest;
		bufferDesc.cpuAccess = nvrhi::CpuAccessMode::Write;
	}
	else
	{
		bufferDesc.debugName = debugName.c_str();
		bufferDesc.keepInitialState = true;
	}

	bufferHandle = deviceManager->GetDevice()->createBuffer( bufferDesc );

	if( !bufferHandle )
	{
		return false;
	}

	// copy the data
	if( data )
	{
		Update( data, allocSize, 0, true, commandList );
	}

	return true;
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

	if( !bufferHandle )
	{
		return;
	}

	if( r_showBuffers.GetBool() )
	{
		idLib::Printf( "index buffer free %p, api %p (%i bytes)\n", this, bufferHandle.Get(), GetSize() );
	}

	bufferHandle.Reset();

	ClearWithoutFreeing();
}

/*
========================
idUniformBuffer::Update
========================
*/
void idUniformBuffer::Update( const void* data, int updateSize, int offset, bool initialUpdate, nvrhi::ICommandList* commandList ) const
{
	assert( bufferHandle );
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
		if( initialUpdate )
		{
			commandList->beginTrackingBufferState( bufferHandle, nvrhi::ResourceStates::Common );
			commandList->writeBuffer( bufferHandle, data, numBytes, GetOffset() + offset );
			commandList->setPermanentBufferState( bufferHandle, nvrhi::ResourceStates::Common | nvrhi::ResourceStates::ShaderResource );
		}
		else
		{
			commandList->writeBuffer( bufferHandle, data, numBytes, GetOffset() + offset );
		}
	}
}

/*
========================
idUniformBuffer::MapBuffer
========================
*/
void* idUniformBuffer::MapBuffer( bufferMapType_t mapType )
{
	assert( bufferHandle );
	assert( IsMapped() == false );

	nvrhi::CpuAccessMode accessMode = nvrhi::CpuAccessMode::Write;
	if( mapType == BM_READ )
	{
		accessMode = nvrhi::CpuAccessMode::Read;
	}

	buffer = deviceManager->GetDevice()->mapBuffer( bufferHandle, accessMode, { ( uint64 )GetOffset(), ( uint64 )GetSize() } );

	SetMapped();

	if( buffer == NULL )
	{
		idLib::FatalError( "idUniformBuffer::MapBuffer: failed" );
	}

	return buffer;
}

/*
========================
idUniformBuffer::UnmapBuffer
========================
*/
void idUniformBuffer::UnmapBuffer()
{
	assert( bufferHandle );
	assert( IsMapped() );

	if( deviceManager && deviceManager->GetDevice() )
	{
		deviceManager->GetDevice()->unmapBuffer( bufferHandle );
	}

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
	bufferHandle.Reset();
}
