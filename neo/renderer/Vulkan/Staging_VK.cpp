/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
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
#pragma hdrstop
#include "precompiled.h"

#include "../RenderCommon.h"
#include "../RenderBackend.h"
#include "Staging_VK.h"

idCVar r_vkUploadBufferSizeMB( "r_vkUploadBufferSizeMB", "64", CVAR_INTEGER | CVAR_INIT, "Size of gpu upload buffer." );
idCVar r_vkStagingMaxCommands( "r_vkStagingMaxCommands", "-1", CVAR_INTEGER | CVAR_INIT, "Maximum amount of commands staged (-1 for no limit)" );

/*
===========================================================================

idVulkanStagingManager

===========================================================================
*/

idVulkanStagingManager stagingManager;

/*
=============
idVulkanStagingManager::idVulkanStagingManager
=============
*/
idVulkanStagingManager::idVulkanStagingManager() :
	maxBufferSize( 0 ),
	currentBuffer( 0 ),
	mappedData( NULL ),
	memory( VK_NULL_HANDLE ),
	commandPool( VK_NULL_HANDLE )
{

}

/*
=============
idVulkanStagingManager::~idVulkanStagingManager
=============
*/
idVulkanStagingManager::~idVulkanStagingManager()
{

}

/*
=============
idVulkanStagingManager::Init
=============
*/
void idVulkanStagingManager::Init()
{
	maxBufferSize = ( size_t )( r_vkUploadBufferSizeMB.GetInteger() * 1024 * 1024 );

	VkBufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = maxBufferSize;
	bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

	for( int i = 0; i < NUM_FRAME_DATA; ++i )
	{
		buffers[ i ].offset = 0;

		ID_VK_CHECK( vkCreateBuffer( vkcontext.device, &bufferCreateInfo, NULL, &buffers[ i ].buffer ) );
	}

	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements( vkcontext.device, buffers[ 0 ].buffer, &memoryRequirements );

	const VkDeviceSize alignMod = memoryRequirements.size % memoryRequirements.alignment;
	const VkDeviceSize alignedSize = ( alignMod == 0 ) ? memoryRequirements.size : ( memoryRequirements.size + memoryRequirements.alignment - alignMod );

	VkMemoryAllocateInfo memoryAllocateInfo = {};
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.allocationSize = alignedSize * NUM_FRAME_DATA;
	memoryAllocateInfo.memoryTypeIndex = FindMemoryTypeIndex( memoryRequirements.memoryTypeBits, VULKAN_MEMORY_USAGE_CPU_TO_GPU );

	ID_VK_CHECK( vkAllocateMemory( vkcontext.device, &memoryAllocateInfo, NULL, &memory ) );

	for( int i = 0; i < NUM_FRAME_DATA; ++i )
	{
		ID_VK_CHECK( vkBindBufferMemory( vkcontext.device, buffers[ i ].buffer, memory, i * alignedSize ) );
	}

	ID_VK_CHECK( vkMapMemory( vkcontext.device, memory, 0, alignedSize * NUM_FRAME_DATA, 0, reinterpret_cast< void** >( &mappedData ) ) );

	VkCommandPoolCreateInfo commandPoolCreateInfo = {};
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	commandPoolCreateInfo.queueFamilyIndex = vkcontext.graphicsFamilyIdx;
	ID_VK_CHECK( vkCreateCommandPool( vkcontext.device, &commandPoolCreateInfo, NULL, &commandPool ) );

	VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.commandPool = commandPool;
	commandBufferAllocateInfo.commandBufferCount = 1;

	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

	VkCommandBufferBeginInfo commandBufferBeginInfo = {};
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	for( int i = 0; i < NUM_FRAME_DATA; ++i )
	{
		ID_VK_CHECK( vkAllocateCommandBuffers( vkcontext.device, &commandBufferAllocateInfo, &buffers[ i ].commandBuffer ) );
		ID_VK_CHECK( vkCreateFence( vkcontext.device, &fenceCreateInfo, NULL, &buffers[ i ].fence ) );
		ID_VK_CHECK( vkBeginCommandBuffer( buffers[ i ].commandBuffer, &commandBufferBeginInfo ) );

		buffers[ i ].data = ( byte* )mappedData + ( i * alignedSize );
	}
}

/*
=============
idVulkanStagingManager::Shutdown
=============
*/
void idVulkanStagingManager::Shutdown()
{
	// SRS - use vkFreeMemory (with implicit unmap) vs. vkUnmapMemory to avoid validation layer errors on shutdown
	//vkUnmapMemory( vkcontext.device, memory );
	vkFreeMemory( vkcontext.device, memory, NULL );
	memory = VK_NULL_HANDLE;
	mappedData = NULL;

	for( int i = 0; i < NUM_FRAME_DATA; ++i )
	{
		vkDestroyFence( vkcontext.device, buffers[ i ].fence, NULL );
		vkDestroyBuffer( vkcontext.device, buffers[ i ].buffer, NULL );
		vkFreeCommandBuffers( vkcontext.device, commandPool, 1, &buffers[ i ].commandBuffer );
	}
	memset( buffers, 0, sizeof( buffers ) );

	maxBufferSize = 0;
	currentBuffer = 0;

	// SRS - destroy command pool to avoid validation layer errors on shutdown
	vkDestroyCommandPool( vkcontext.device, commandPool, NULL );
	commandPool = VK_NULL_HANDLE;
}

/*
=============
idVulkanStagingManager::Stage
=============
*/
byte* idVulkanStagingManager::Stage( const int size, const int alignment, VkCommandBuffer& commandBuffer, VkBuffer& buffer, int& bufferOffset )
{
	if( size > maxBufferSize )
	{
		idLib::FatalError( "Can't allocate %d MB in gpu transfer buffer", ( int )( size / 1024 / 1024 ) );
	}

	stagingBuffer_t* stage = &buffers[ currentBuffer ];
	const int alignMod = stage->offset % alignment;
	stage->offset = ( ( stage->offset % alignment ) == 0 ) ? stage->offset : ( stage->offset + alignment - alignMod );

	if( ( stage->offset + size ) >= ( maxBufferSize ) && !stage->submitted )
	{
		Flush();
	}

	int maxCommands = r_vkStagingMaxCommands.GetInteger();
	if( ( maxCommands > 0 ) && ( stage->stagedCommands >= maxCommands ) )
	{
		Flush();
	}

	stage = &buffers[ currentBuffer ];
	if( stage->submitted )
	{
		Wait( *stage );
	}

	commandBuffer = stage->commandBuffer;
	buffer = stage->buffer;
	bufferOffset = stage->offset;

	byte* data = stage->data + stage->offset;
	stage->offset += size;

	stage->stagedCommands++;

	return data;
}

/*
=============
idVulkanStagingManager::Flush
=============
*/
void idVulkanStagingManager::Flush()
{
	stagingBuffer_t& stage = buffers[ currentBuffer ];
	if( stage.submitted || stage.offset == 0 )
	{
		return;
	}

	VkMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_INDEX_READ_BIT;
	vkCmdPipelineBarrier(
		stage.commandBuffer,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
		0, 1, &barrier, 0, NULL, 0, NULL );

	vkEndCommandBuffer( stage.commandBuffer );

	VkMappedMemoryRange memoryRange = {};
	memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	memoryRange.memory = memory;
	memoryRange.size = VK_WHOLE_SIZE;
	vkFlushMappedMemoryRanges( vkcontext.device, 1, &memoryRange );

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &stage.commandBuffer;

	vkQueueSubmit( vkcontext.graphicsQueue, 1, &submitInfo, stage.fence );

	stage.submitted = true;
	stage.stagedCommands = 0;

	currentBuffer = ( currentBuffer + 1 ) % NUM_FRAME_DATA;
}

/*
=============
idVulkanStagingManager::Wait
=============
*/
void idVulkanStagingManager::Wait( stagingBuffer_t& stage )
{
	if( stage.submitted == false )
	{
		return;
	}

	ID_VK_CHECK( vkWaitForFences( vkcontext.device, 1, &stage.fence, VK_TRUE, UINT64_MAX ) );
	ID_VK_CHECK( vkResetFences( vkcontext.device, 1, &stage.fence ) );

	stage.stagedCommands = 0;
	stage.offset = 0;
	stage.submitted = false;

	VkCommandBufferBeginInfo commandBufferBeginInfo = {};
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	ID_VK_CHECK( vkBeginCommandBuffer( stage.commandBuffer, &commandBufferBeginInfo ) );
}