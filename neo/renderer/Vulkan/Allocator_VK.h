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

#ifndef __HEAP_VK_H__
#define __HEAP_VK_H__

enum vulkanMemoryUsage_t
{
	VULKAN_MEMORY_USAGE_UNKNOWN,
	VULKAN_MEMORY_USAGE_GPU_ONLY,
	VULKAN_MEMORY_USAGE_CPU_ONLY,
	VULKAN_MEMORY_USAGE_CPU_TO_GPU,
	VULKAN_MEMORY_USAGE_GPU_TO_CPU,
	VULKAN_MEMORY_USAGES,
};

enum vulkanAllocationType_t
{
	VULKAN_ALLOCATION_TYPE_FREE,
	VULKAN_ALLOCATION_TYPE_BUFFER,
	VULKAN_ALLOCATION_TYPE_IMAGE,
	VULKAN_ALLOCATION_TYPE_IMAGE_LINEAR,
	VULKAN_ALLOCATION_TYPE_IMAGE_OPTIMAL,
	VULKAN_ALLOCATION_TYPES,
};

uint32 FindMemoryTypeIndex( const uint32 memoryTypeBits, const vulkanMemoryUsage_t usage );

class idVulkanBlock;
struct vulkanAllocation_t
{
	vulkanAllocation_t() :
		block( NULL ),
		id( 0 ),
		deviceMemory( VK_NULL_HANDLE ),
		offset( 0 ),
		size( 0 ),
		data( NULL )
	{
	}

	idVulkanBlock* block;
	uint32			id;
	VkDeviceMemory	deviceMemory;
	VkDeviceSize	offset;
	VkDeviceSize	size;
	byte* 			data;
};

/*
================================================================================================

idVulkanBlock

================================================================================================
*/

class idVulkanBlock
{
	friend class idVulkanAllocator;
public:
	idVulkanBlock( const uint32 memoryTypeIndex, const VkDeviceSize size, vulkanMemoryUsage_t usage );
	~idVulkanBlock();

	bool				Init();
	void				Shutdown();

	bool				IsHostVisible() const
	{
		return usage != VULKAN_MEMORY_USAGE_GPU_ONLY;
	}

	bool				Allocate(
		const uint32 size,
		const uint32 align,
		const VkDeviceSize granularity,
		const vulkanAllocationType_t allocType,
		vulkanAllocation_t& allocation );
	void				Free( vulkanAllocation_t& allocation );

	void				Print();

private:
	struct chunk_t
	{
		uint32					id;
		VkDeviceSize			size;
		VkDeviceSize			offset;
		chunk_t* 				prev;
		chunk_t* 				next;
		vulkanAllocationType_t	type;
	};
	chunk_t* 			head;

	uint32				nextBlockId;
	uint32				memoryTypeIndex;
	vulkanMemoryUsage_t	usage;
	VkDeviceMemory		deviceMemory;
	VkDeviceSize		size;
	VkDeviceSize		allocated;
	byte* 				data;
};

typedef idArray< idList< idVulkanBlock* >, VK_MAX_MEMORY_TYPES > idVulkanBlocks;

/*
================================================================================================

idVulkanAllocator

================================================================================================
*/

class idVulkanAllocator
{
public:
	idVulkanAllocator();

	void					Init();
	void					Shutdown();

	vulkanAllocation_t			Allocate(
		const uint32 size,
		const uint32 align,
		const uint32 memoryTypeBits,
		const vulkanMemoryUsage_t usage,
		const vulkanAllocationType_t allocType );
	void					Free( const vulkanAllocation_t allocation );
	void					EmptyGarbage();

	void					Print();

private:
	int							garbageIndex;

	int							deviceLocalMemoryBytes;
	int							hostVisibleMemoryBytes;
	VkDeviceSize				bufferImageGranularity;

	idVulkanBlocks				blocks;
	idList<vulkanAllocation_t>	garbage[ NUM_FRAME_DATA ];
};

#if defined( USE_AMD_ALLOCATOR )
	extern VmaAllocator vmaAllocator;
#else
	extern idVulkanAllocator vulkanAllocator;
#endif

#endif