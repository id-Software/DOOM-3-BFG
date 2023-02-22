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

#if defined( USE_AMD_ALLOCATOR )
#include "vk_mem_alloc.h"

extern VmaAllocator m_VmaAllocator;

idCVar r_vmaAllocateBufferMemory( "r_vmaAllocateBufferMemory", "1", CVAR_BOOL | CVAR_INIT, "Use VMA allocator for buffer memory." );

/*
========================
pickBufferUsage - copied from nvrhi vulkan-buffer.cpp
========================
* Copyright (c) 2014-2021, NVIDIA CORPORATION. All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
* DEALINGS IN THE SOFTWARE.
*/
vk::BufferUsageFlags pickBufferUsage( const nvrhi::BufferDesc& desc )
{
	vk::BufferUsageFlags usageFlags = vk::BufferUsageFlagBits::eTransferSrc |
									  vk::BufferUsageFlagBits::eTransferDst;

	if( desc.isVertexBuffer )
	{
		usageFlags |= vk::BufferUsageFlagBits::eVertexBuffer;
	}

	if( desc.isIndexBuffer )
	{
		usageFlags |= vk::BufferUsageFlagBits::eIndexBuffer;
	}

	if( desc.isDrawIndirectArgs )
	{
		usageFlags |= vk::BufferUsageFlagBits::eIndirectBuffer;
	}

	if( desc.isConstantBuffer )
	{
		usageFlags |= vk::BufferUsageFlagBits::eUniformBuffer;
	}

	if( desc.structStride != 0 || desc.canHaveUAVs || desc.canHaveRawViews )
	{
		usageFlags |= vk::BufferUsageFlagBits::eStorageBuffer;
	}

	if( desc.canHaveTypedViews )
	{
		usageFlags |= vk::BufferUsageFlagBits::eUniformTexelBuffer;
	}

	if( desc.canHaveTypedViews && desc.canHaveUAVs )
	{
		usageFlags |= vk::BufferUsageFlagBits::eStorageTexelBuffer;
	}

	if( desc.isAccelStructBuildInput )
	{
		usageFlags |= vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR;
	}

	if( desc.isAccelStructStorage )
	{
		usageFlags |= vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR;
	}

	if( deviceManager->IsVulkanDeviceExtensionEnabled( VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME ) )
	{
		usageFlags |= vk::BufferUsageFlagBits::eShaderDeviceAddress;
	}

	return usageFlags;
}
#endif

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
	//SRS - Generic initialization handled by idBufferObject base class
	SetUnmapped();
	SetDebugName( "Vertex Buffer" );
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

#if defined( USE_AMD_ALLOCATOR )
	if( m_VmaAllocator && r_vmaAllocateBufferMemory.GetBool() )
	{
		VkBufferCreateInfo bufferCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		bufferCreateInfo.size = numBytes;
		bufferCreateInfo.usage = static_cast< VkBufferUsageFlags >( pickBufferUsage( vertexBufferDesc ) );
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocCreateInfo = {};
		if( usage == BU_DYNAMIC )
		{
			allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
			allocCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
		}
		else
		{
			allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
		}

		VkResult result = vmaCreateBuffer( m_VmaAllocator, &bufferCreateInfo, &allocCreateInfo, &vkBuffer, &allocation, &allocationInfo );
		assert( result == VK_SUCCESS );

		bufferHandle = deviceManager->GetDevice()->createHandleForNativeBuffer( nvrhi::ObjectTypes::VK_Buffer, vkBuffer, vertexBufferDesc );
	}
	else
#endif
	{
		bufferHandle = deviceManager->GetDevice()->createBuffer( vertexBufferDesc );
	}

	if( !bufferHandle )
	{
		return false;
	}

	if( r_showBuffers.GetBool() )
	{
		idLib::Printf( "vertex buffer alloc %p, api %p (%i bytes)\n", this, bufferHandle.Get(), GetSize() );
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

#if defined( USE_AMD_ALLOCATOR )
	if( m_VmaAllocator && r_vmaAllocateBufferMemory.GetBool() )
	{
		vmaDestroyBuffer( m_VmaAllocator, vkBuffer, allocation );
	}
#endif

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
		assert( IsMapped() );

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
	assert( usage == BU_DYNAMIC );
	assert( IsMapped() == false );

#if defined( USE_AMD_ALLOCATOR )
	if( m_VmaAllocator && r_vmaAllocateBufferMemory.GetBool() )
	{
		buffer = ( byte* )allocationInfo.pMappedData + GetOffset();
	}
	else
#endif
	{
		nvrhi::CpuAccessMode accessMode = nvrhi::CpuAccessMode::Write;
		if( mapType == bufferMapType_t::BM_READ )
		{
			accessMode = nvrhi::CpuAccessMode::Read;
		}

		buffer = deviceManager->GetDevice()->mapBuffer( bufferHandle, accessMode, { ( uint64 )GetOffset(), ( uint64 )GetAllocedSize() } );
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
	assert( bufferHandle );
	assert( usage == BU_DYNAMIC );
	assert( IsMapped() );

#if defined( USE_AMD_ALLOCATOR )
	if( !m_VmaAllocator || !r_vmaAllocateBufferMemory.GetBool() )
#endif
	{
		if( deviceManager && deviceManager->GetDevice() )
		{
			deviceManager->GetDevice()->unmapBuffer( bufferHandle );
		}
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

#if defined( USE_AMD_ALLOCATOR )
	allocation = NULL;
	allocationInfo = {};
#endif
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
	//SRS - Generic initialization handled by idBufferObject base class
	SetUnmapped();
	SetDebugName( "Index Buffer" );
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

#if defined( USE_AMD_ALLOCATOR )
	if( m_VmaAllocator && r_vmaAllocateBufferMemory.GetBool() )
	{
		VkBufferCreateInfo bufferCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		bufferCreateInfo.size = numBytes;
		bufferCreateInfo.usage = static_cast< VkBufferUsageFlags >( pickBufferUsage( indexBufferDesc ) );
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocCreateInfo = {};
		if( usage == BU_DYNAMIC )
		{
			allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
			allocCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
		}
		else
		{
			allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
		}

		VkResult result = vmaCreateBuffer( m_VmaAllocator, &bufferCreateInfo, &allocCreateInfo, &vkBuffer, &allocation, &allocationInfo );
		assert( result == VK_SUCCESS );

		bufferHandle = deviceManager->GetDevice()->createHandleForNativeBuffer( nvrhi::ObjectTypes::VK_Buffer, vkBuffer, indexBufferDesc );
	}
	else
#endif
	{
		bufferHandle  = deviceManager->GetDevice()->createBuffer( indexBufferDesc );
	}

	if( !bufferHandle )
	{
		return false;
	}

	if( r_showBuffers.GetBool() )
	{
		idLib::Printf( "index buffer alloc %p, api %p (%i bytes)\n", this, bufferHandle.Get(), GetSize() );
	}

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

#if defined( USE_AMD_ALLOCATOR )
	if( m_VmaAllocator && r_vmaAllocateBufferMemory.GetBool() )
	{
		vmaDestroyBuffer( m_VmaAllocator, vkBuffer, allocation );
	}
#endif

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
		assert( IsMapped() );

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
	assert( usage == BU_DYNAMIC );
	assert( IsMapped() == false );

#if defined( USE_AMD_ALLOCATOR )
	if( m_VmaAllocator && r_vmaAllocateBufferMemory.GetBool() )
	{
		buffer = ( byte* )allocationInfo.pMappedData + GetOffset();
	}
	else
#endif
	{
		nvrhi::CpuAccessMode accessMode = nvrhi::CpuAccessMode::Write;
		if( mapType == bufferMapType_t::BM_READ )
		{
			accessMode = nvrhi::CpuAccessMode::Read;
		}

		buffer = deviceManager->GetDevice()->mapBuffer( bufferHandle, accessMode, { ( uint64 )GetOffset(), ( uint64 )GetAllocedSize() } );
	}

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
	assert( usage == BU_DYNAMIC );
	assert( IsMapped() );

#if defined( USE_AMD_ALLOCATOR )
	if( !m_VmaAllocator || !r_vmaAllocateBufferMemory.GetBool() )
#endif
	{
		if( deviceManager && deviceManager->GetDevice() )
		{
			deviceManager->GetDevice()->unmapBuffer( bufferHandle );
		}
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

#if defined( USE_AMD_ALLOCATOR )
	allocation = NULL;
	allocationInfo = {};
#endif
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
	//SRS - Generic initialization handled by idBufferObject base class
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
		idLib::Error( "idUniformBuffer::AllocBufferObject: allocSize = %i", allocSize );
	}

	size = allocSize;
	usage = allocatedUsage;

	const int numBytes = GetAllocedSize();

	// This buffer is a shader resource as opposed to a constant buffer due to
	// constant buffers not being able to be sub-ranged.
	nvrhi::BufferDesc bufferDesc;
	//bufferDesc.initialState = nvrhi::ResourceStates::ConstantBuffer;		// SRS - shouldn't this be initialized to CopyDest?
	bufferDesc.initialState = nvrhi::ResourceStates::CopyDest;
	//bufferDesc.keepInitialState = true;									// SRS - shouldn't this be set for BU_STATIC only?
	bufferDesc.canHaveTypedViews = true;
	bufferDesc.canHaveRawViews = true;
	bufferDesc.byteSize = numBytes;
	bufferDesc.structStride = sizeof( idVec4 );
	bufferDesc.isConstantBuffer = true;

	if( usage == BU_DYNAMIC )
	{
		bufferDesc.debugName = "VertexCache Mapped Joint Buffer";
		bufferDesc.cpuAccess = nvrhi::CpuAccessMode::Write;
	}
	else
	{
		bufferDesc.debugName = "Static Uniform Buffer";
		bufferDesc.keepInitialState = true;
	}

#if defined( USE_AMD_ALLOCATOR )
	if( m_VmaAllocator && r_vmaAllocateBufferMemory.GetBool() )
	{
		VkBufferCreateInfo bufferCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		bufferCreateInfo.size = numBytes;
		bufferCreateInfo.usage = static_cast< VkBufferUsageFlags >( pickBufferUsage( bufferDesc ) );
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocCreateInfo = {};
		if( usage == BU_DYNAMIC )
		{
			allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
			allocCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
		}
		else
		{
			allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
		}

		VkResult result = vmaCreateBuffer( m_VmaAllocator, &bufferCreateInfo, &allocCreateInfo, &vkBuffer, &allocation, &allocationInfo );
		assert( result == VK_SUCCESS );

		bufferHandle = deviceManager->GetDevice()->createHandleForNativeBuffer( nvrhi::ObjectTypes::VK_Buffer, vkBuffer, bufferDesc );
	}
	else
#endif
	{
		bufferHandle  = deviceManager->GetDevice()->createBuffer( bufferDesc );
	}

	if( !bufferHandle )
	{
		return false;
	}

	if( r_showBuffers.GetBool() )
	{
		idLib::Printf( "uniform buffer alloc %p, api %p (%i bytes)\n", this, bufferHandle.Get(), GetSize() );
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
		idLib::Printf( "uniform buffer free %p, api %p (%i bytes)\n", this, bufferHandle.Get(), GetSize() );
	}

	bufferHandle.Reset();

#if defined( USE_AMD_ALLOCATOR )
	if( m_VmaAllocator && r_vmaAllocateBufferMemory.GetBool() )
	{
		vmaDestroyBuffer( m_VmaAllocator, vkBuffer, allocation );
	}
#endif

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
		idLib::FatalError( "idUniformBuffer::Update: size overrun, %i > %i\n", updateSize, GetSize() );
	}

	int numBytes = ( updateSize + 15 ) & ~15;

	if( usage == BU_DYNAMIC )
	{
		assert( IsMapped() );

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
	assert( usage == BU_DYNAMIC );
	assert( IsMapped() == false );

#if defined( USE_AMD_ALLOCATOR )
	if( m_VmaAllocator && r_vmaAllocateBufferMemory.GetBool() )
	{
		buffer = ( byte* )allocationInfo.pMappedData + GetOffset();
	}
	else
#endif
	{
		nvrhi::CpuAccessMode accessMode = nvrhi::CpuAccessMode::Write;
		if( mapType == bufferMapType_t::BM_READ )
		{
			accessMode = nvrhi::CpuAccessMode::Read;
		}

		buffer = deviceManager->GetDevice()->mapBuffer( bufferHandle, accessMode, { ( uint64 )GetOffset(), ( uint64 )GetAllocedSize() } );
	}

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
	assert( usage == BU_DYNAMIC );
	assert( IsMapped() );

#if defined( USE_AMD_ALLOCATOR )
	if( !m_VmaAllocator || !r_vmaAllocateBufferMemory.GetBool() )
#endif
	{
		if( deviceManager && deviceManager->GetDevice() )
		{
			deviceManager->GetDevice()->unmapBuffer( bufferHandle );
		}
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

#if defined( USE_AMD_ALLOCATOR )
	allocation = NULL;
	allocationInfo = {};
#endif
}
