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

#ifndef __STAGING_VK__
#define __STAGING_VK__

/*
===========================================================================

idVulkanStagingManager

===========================================================================
*/

struct stagingBuffer_t
{
	stagingBuffer_t() :
		submitted( false ),
		commandBuffer( VK_NULL_HANDLE ),
		buffer( VK_NULL_HANDLE ),
		fence( VK_NULL_HANDLE ),
		offset( 0 ),
		data( NULL ) {}

	bool				submitted;
	VkCommandBuffer		commandBuffer;
	VkBuffer			buffer;
	VkFence				fence;
	VkDeviceSize		offset;
	byte* 				data;
};

class idVulkanStagingManager
{
public:
	idVulkanStagingManager();
	~idVulkanStagingManager();

	void			Init();
	void			Shutdown();

	byte* 			Stage( const int size, const int alignment, VkCommandBuffer& commandBuffer, VkBuffer& buffer, int& bufferOffset );
	void			Flush();

private:
	void			Wait( stagingBuffer_t& stage );

private:
	int				maxBufferSize;
	int				currentBuffer;
	byte* 			mappedData;
	VkDeviceMemory	memory;
	VkCommandPool	commandPool;

	stagingBuffer_t buffers[ NUM_FRAME_DATA ];
};

extern idVulkanStagingManager stagingManager;

#endif