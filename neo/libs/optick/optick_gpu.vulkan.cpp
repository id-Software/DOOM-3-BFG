// The MIT License(MIT)
//
// Copyright(c) 2019 Vadim Slyusarev
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "optick.config.h"

#if USE_OPTICK
#if OPTICK_ENABLE_GPU_VULKAN
#include <vulkan/vulkan.h>

#include "optick_core.h"
#include "optick_gpu.h"

#define OPTICK_VK_CHECK(args) do { VkResult __hr = args; OPTICK_ASSERT(__hr == VK_SUCCESS, "Failed check"); (void)__hr; } while(false);

namespace Optick
{
	class GPUProfilerVulkan : public GPUProfiler
	{
	private:
		VulkanFunctions vulkanFunctions = {};

	protected:
		struct Frame
		{
			VkCommandBuffer commandBuffer;
			VkFence fence;
			Frame() : commandBuffer(VK_NULL_HANDLE), fence(VK_NULL_HANDLE) {}
		};

		struct NodePayload
		{
			VulkanFunctions*	vulkanFunctions;
			VkDevice			device;
			VkPhysicalDevice	physicalDevice;
			VkQueue				queue;
			VkQueryPool			queryPool;
			VkCommandPool		commandPool;
			VkEvent				event;

			array<Frame, NUM_FRAMES_DELAY> frames;

			NodePayload() : vulkanFunctions(), device(VK_NULL_HANDLE), physicalDevice(VK_NULL_HANDLE), queue(VK_NULL_HANDLE), queryPool(VK_NULL_HANDLE), commandPool(VK_NULL_HANDLE), event(VK_NULL_HANDLE) {}
			~NodePayload();
		};
		vector<NodePayload*> nodePayloads;

		// VSync / Present Stats
		uint64_t prevPresentTime;
		uint32_t prevPresentID;

	public:
		GPUProfilerVulkan();
		~GPUProfilerVulkan();

		void InitDevice(VkDevice* devices, VkPhysicalDevice* physicalDevices, VkQueue* cmdQueues, uint32_t* cmdQueuesFamily, uint32_t nodeCount, const VulkanFunctions* functions);
		void QueryTimestamp(VkCommandBuffer commandBuffer, int64_t* outCpuTimestamp);
		void Flip(VkSwapchainKHR swapChain);


		// Interface implementation
		ClockSynchronization GetClockSynchronization(uint32_t nodeIndex) override;

		void QueryTimestamp(void* context, int64_t* outCpuTimestamp) override
		{
			QueryTimestamp((VkCommandBuffer)context, outCpuTimestamp);
		}

		void ResolveTimestamps(uint32_t nodeIndex, uint32_t startIndex, uint32_t count) override;

		void WaitForFrame(uint32_t nodeIndex, uint64_t frameNumber) override;

		void Flip(void* swapChain, uint32_t frameID) override
		{
			Flip(static_cast<VkSwapchainKHR>(swapChain));
		}
	};

	void InitGpuVulkan(VkDevice* vkDevices, VkPhysicalDevice* vkPhysicalDevices, VkQueue* vkQueues, uint32_t* cmdQueuesFamily, uint32_t numQueues, const VulkanFunctions* functions)
	{
		GPUProfilerVulkan* gpuProfiler = Memory::New<GPUProfilerVulkan>();
		gpuProfiler->InitDevice(vkDevices, vkPhysicalDevices, vkQueues, cmdQueuesFamily, numQueues, functions);
		Core::Get().InitGPUProfiler(gpuProfiler);
	}

	GPUProfilerVulkan::GPUProfilerVulkan()
	{
		prevPresentTime = 0;
		prevPresentID = 0;
	}

	void GPUProfilerVulkan::InitDevice(VkDevice* devices, VkPhysicalDevice* physicalDevices, VkQueue* cmdQueues, uint32_t* cmdQueuesFamily, uint32_t nodeCount, const VulkanFunctions* functions)
	{
		if (functions != nullptr)
		{
			vulkanFunctions = *functions;
		}
		else 
		{
			vulkanFunctions = {
				vkGetPhysicalDeviceProperties,
				(PFN_vkCreateQueryPool_)vkCreateQueryPool,
				(PFN_vkCreateCommandPool_)vkCreateCommandPool,
				(PFN_vkCreateEvent_)vkCreateEvent,
				(PFN_vkAllocateCommandBuffers_)vkAllocateCommandBuffers,
				(PFN_vkCreateFence_)vkCreateFence,
				vkCmdResetQueryPool,
				vkResetQueryPool,
				(PFN_vkCmdWaitEvents_)vkCmdWaitEvents,
				(PFN_vkResetEvent_)vkResetEvent,
				(PFN_vkSetEvent_)vkSetEvent,
				(PFN_vkQueueSubmit_)vkQueueSubmit,
				(PFN_vkWaitForFences_)vkWaitForFences,
				(PFN_vkResetCommandBuffer_)vkResetCommandBuffer,
				(PFN_vkCmdWriteTimestamp_)vkCmdWriteTimestamp,
				(PFN_vkGetQueryPoolResults_)vkGetQueryPoolResults,
				(PFN_vkBeginCommandBuffer_)vkBeginCommandBuffer,
				(PFN_vkEndCommandBuffer_)vkEndCommandBuffer,
				(PFN_vkResetFences_)vkResetFences,
				vkDestroyCommandPool,
				vkDestroyQueryPool,
				vkDestroyEvent,
				vkDestroyFence,
				vkFreeCommandBuffers,
				nullptr, // dynamically define vkGetPastPresentationTimingGOOGLE if VK_GOOGLE_display_timing extension available
			};
		}

		VkQueryPoolCreateInfo queryPoolCreateInfo;
		queryPoolCreateInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
		queryPoolCreateInfo.pNext = 0;
		queryPoolCreateInfo.flags = 0;
		queryPoolCreateInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
		queryPoolCreateInfo.queryCount = MAX_QUERIES_COUNT + 1;

		VkCommandPoolCreateInfo commandPoolCreateInfo;
		commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		commandPoolCreateInfo.pNext = 0;
		commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		VkEventCreateInfo eventCreateInfo;
		eventCreateInfo.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
		eventCreateInfo.pNext = 0;
		eventCreateInfo.flags = 0;

		nodes.resize(nodeCount);
		nodePayloads.resize(nodeCount);

		VkResult r;
		for (uint32_t i = 0; i < nodeCount; ++i)
		{
			VkPhysicalDeviceProperties properties = { 0 };
			(*vulkanFunctions.vkGetPhysicalDeviceProperties)(physicalDevices[i], &properties);
			GPUProfiler::InitNode(properties.deviceName, i);

			NodePayload* nodePayload = Memory::New<NodePayload>();
			nodePayloads[i] = nodePayload;
			nodePayload->vulkanFunctions = &vulkanFunctions;
			nodePayload->vulkanFunctions->vkGetPastPresentationTimingGOOGLE = (PFN_vkGetPastPresentationTimingGOOGLE_)vkGetDeviceProcAddr(devices[i], "vkGetPastPresentationTimingGOOGLE");
			nodePayload->device = devices[i];
			nodePayload->physicalDevice = physicalDevices[i];
			nodePayload->queue = cmdQueues[i];
			
			r = (VkResult)(*vulkanFunctions.vkCreateQueryPool)(devices[i], &queryPoolCreateInfo, 0, &nodePayload->queryPool);
			OPTICK_ASSERT(r == VK_SUCCESS, "Failed");
			(void)r;

			commandPoolCreateInfo.queueFamilyIndex = cmdQueuesFamily[i];
			r = (VkResult)(*vulkanFunctions.vkCreateCommandPool)(nodePayload->device, &commandPoolCreateInfo, 0, &nodePayload->commandPool);
			OPTICK_ASSERT(r == VK_SUCCESS, "Failed");
			(void)r;

			r = (VkResult)(*vulkanFunctions.vkCreateEvent)(nodePayload->device, &eventCreateInfo, 0, &nodePayload->event);
			OPTICK_ASSERT(r == VK_SUCCESS, "Failed");
			(void)r;

			for (uint32_t j = 0; j < nodePayload->frames.size(); ++j)
			{
				Frame& frame = nodePayload->frames[j];

				VkCommandBufferAllocateInfo allocInfo;
				allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
				allocInfo.pNext = 0;
				allocInfo.commandBufferCount = 1;
				allocInfo.commandPool = nodePayload->commandPool;
				allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
				r = (VkResult)(*vulkanFunctions.vkAllocateCommandBuffers)(nodePayload->device, &allocInfo, &frame.commandBuffer);
				OPTICK_ASSERT(r == VK_SUCCESS, "Failed");
				(void)r;

				VkFenceCreateInfo fenceCreateInfo;
				fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
				fenceCreateInfo.pNext = 0;
				fenceCreateInfo.flags = j == 0 ? 0 : VK_FENCE_CREATE_SIGNALED_BIT;
				r = (VkResult)(*vulkanFunctions.vkCreateFence)(nodePayload->device, &fenceCreateInfo, 0, &frame.fence);
				OPTICK_ASSERT(r == VK_SUCCESS, "Failed");
				(void)r;
				if (j == 0)
				{
					VkCommandBufferBeginInfo commandBufferBeginInfo;
					commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
					commandBufferBeginInfo.pNext = 0;
					commandBufferBeginInfo.pInheritanceInfo = 0;
					commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
					(*vulkanFunctions.vkBeginCommandBuffer)(frame.commandBuffer, &commandBufferBeginInfo);
					(*vulkanFunctions.vkCmdResetQueryPool)(frame.commandBuffer, nodePayload->queryPool, 0, MAX_QUERIES_COUNT);
					(*vulkanFunctions.vkEndCommandBuffer)(frame.commandBuffer);

					VkSubmitInfo submitInfo = {};
					submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
					submitInfo.pNext = nullptr;
					submitInfo.waitSemaphoreCount = 0;
					submitInfo.pWaitSemaphores = nullptr;
					submitInfo.commandBufferCount = 1;
					submitInfo.pCommandBuffers = &frame.commandBuffer;
					submitInfo.signalSemaphoreCount = 0;
					submitInfo.pSignalSemaphores = nullptr;
					(*vulkanFunctions.vkQueueSubmit)(nodePayload->queue, 1, &submitInfo, frame.fence);
					(*vulkanFunctions.vkWaitForFences)(nodePayload->device, 1, &frame.fence, 1, (uint64_t)-1);
					(*vulkanFunctions.vkResetCommandBuffer)(frame.commandBuffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
				}
			}
		}
	}

	void GPUProfilerVulkan::QueryTimestamp(VkCommandBuffer commandBuffer, int64_t* outCpuTimestamp)
	{
		if (currentState == STATE_RUNNING)
		{
			uint32_t index = nodes[currentNode]->QueryTimestamp(outCpuTimestamp);
			(*vulkanFunctions.vkCmdWriteTimestamp)(commandBuffer, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, nodePayloads[currentNode]->queryPool, index);
		}
	}

	void GPUProfilerVulkan::ResolveTimestamps(uint32_t nodeIndex, uint32_t startIndex, uint32_t count)
	{
		if (count)
		{
			Node* node = nodes[nodeIndex];

			NodePayload* payload = nodePayloads[nodeIndex];

			OPTICK_VK_CHECK((VkResult)(*vulkanFunctions.vkGetQueryPoolResults)(payload->device, payload->queryPool, startIndex, count, 8 * (size_t)count, &nodes[nodeIndex]->queryGpuTimestamps[startIndex], 8, VK_QUERY_RESULT_64_BIT));
			(*vulkanFunctions.vkResetQueryPool)(payload->device, payload->queryPool, startIndex, count);

			// Convert GPU timestamps => CPU Timestamps
			for (uint32_t index = startIndex; index < startIndex + count; ++index)
				*node->queryCpuTimestamps[index] = node->clock.GetCPUTimestamp(node->queryGpuTimestamps[index]);
		}
	}

	void GPUProfilerVulkan::WaitForFrame(uint32_t nodeIndex, uint64_t frameNumberToWait)
	{
		OPTICK_EVENT();

		int r = VK_SUCCESS;
		do
		{
			NodePayload& payload = *nodePayloads[nodeIndex];
			r = (*vulkanFunctions.vkWaitForFences)(nodePayloads[nodeIndex]->device, 1, &payload.frames[frameNumberToWait % payload.frames.size()].fence, 1, 1000 * 30);
		} while (r != VK_SUCCESS);
	}

	void GPUProfilerVulkan::Flip(VkSwapchainKHR swapChain)
	{
		OPTICK_CATEGORY("GPUProfilerVulkan::Flip", Category::Debug);

		std::lock_guard<std::recursive_mutex> lock(updateLock);

		if (currentState == STATE_STARTING)
			currentState = STATE_RUNNING;

		if (currentState == STATE_RUNNING)
		{
			Node& node = *nodes[currentNode];
			NodePayload& payload = *nodePayloads[currentNode];

			uint32_t currentFrameIndex = frameNumber % NUM_FRAMES_DELAY;
			uint32_t nextFrameIndex = (frameNumber + 1) % NUM_FRAMES_DELAY;

			QueryFrame& currentFrame = node.queryGpuframes[currentFrameIndex];
			QueryFrame& nextFrame = node.queryGpuframes[nextFrameIndex];

			VkCommandBuffer commandBuffer = payload.frames[currentFrameIndex].commandBuffer;
			VkFence fence = payload.frames[currentFrameIndex].fence;
			VkDevice device = payload.device;
			VkQueue queue = payload.queue;

			(*vulkanFunctions.vkWaitForFences)(device, 1, &fence, 1, (uint64_t)-1);
			(*vulkanFunctions.vkResetFences)(device, 1, &fence);

			VkCommandBufferBeginInfo commandBufferBeginInfo;
			commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			commandBufferBeginInfo.pNext = 0;
			commandBufferBeginInfo.pInheritanceInfo = 0;
			commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
			OPTICK_VK_CHECK((VkResult)(*vulkanFunctions.vkBeginCommandBuffer)(commandBuffer, &commandBufferBeginInfo));

			if (EventData* frameEvent = currentFrame.frameEvent)
				QueryTimestamp(commandBuffer, &frameEvent->finish);

			// Generate GPU Frame event for the next frame
			EventData& event = AddFrameEvent();
			QueryTimestamp(commandBuffer, &event.start);
			QueryTimestamp(commandBuffer, &AddFrameTag().timestamp);
			nextFrame.frameEvent = &event;

			OPTICK_VK_CHECK((VkResult)(*vulkanFunctions.vkEndCommandBuffer)(commandBuffer));
			VkSubmitInfo submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.pNext = nullptr;
			submitInfo.waitSemaphoreCount = 0;
			submitInfo.pWaitSemaphores = nullptr;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &commandBuffer;
			submitInfo.signalSemaphoreCount = 0;
			submitInfo.pSignalSemaphores = nullptr;
			OPTICK_VK_CHECK((VkResult)(*vulkanFunctions.vkQueueSubmit)(queue, 1, &submitInfo, fence));

			uint32_t queryBegin = currentFrame.queryIndexStart;
			uint32_t queryEnd = node.queryIndex;

			if (queryBegin != (uint32_t)-1)
			{
				OPTICK_ASSERT(queryEnd - queryBegin <= MAX_QUERIES_COUNT, "Too many queries in one frame? Increase GPUProfiler::MAX_QUERIES_COUNT to fix the problem!");
				currentFrame.queryIndexCount = queryEnd - queryBegin;
			}
			else
			{
				currentFrame.queryIndexStart = 0;
				currentFrame.queryIndexCount = queryEnd;
				prevPresentTime = 0;
				prevPresentID = 0;
			}

			// Preparing Next Frame
			// Try resolve timestamps for the current frame
			if (nextFrame.queryIndexStart != (uint32_t)-1)
			{
				uint32_t startIndex = nextFrame.queryIndexStart % MAX_QUERIES_COUNT;
				uint32_t finishIndex = (startIndex + nextFrame.queryIndexCount) % MAX_QUERIES_COUNT;

				if (startIndex < finishIndex)
				{
					ResolveTimestamps(currentNode, startIndex, finishIndex - startIndex);
				}
				else if (startIndex > finishIndex)
				{
					ResolveTimestamps(currentNode, startIndex, MAX_QUERIES_COUNT - startIndex);
					ResolveTimestamps(currentNode, 0, finishIndex);
				}

				// SRS - Add Vulkan presentation / vsync timing if VK_GOOGLE_display_timing extension available
				if (vulkanFunctions.vkGetPastPresentationTimingGOOGLE)
				{
					uint32_t queryPresentTimingCount = 0;
					(*vulkanFunctions.vkGetPastPresentationTimingGOOGLE)(device, swapChain, &queryPresentTimingCount, nullptr);
					if (queryPresentTimingCount > 0)
					{
						// Query Presentation Timing / VSync
						vector<VkPastPresentationTimingGOOGLE> queryPresentTimings;
						queryPresentTimings.resize(queryPresentTimingCount);
						(*vulkanFunctions.vkGetPastPresentationTimingGOOGLE)(device, swapChain, &queryPresentTimingCount, &queryPresentTimings[0]);
						for (uint32_t presentIndex = 0; presentIndex < queryPresentTimingCount; presentIndex++)
						{
							// Process Presentation Timing / VSync if swap image was actually presented (i.e. not dropped)
							VkPastPresentationTimingGOOGLE presentTiming = queryPresentTimings[presentIndex];
							if (presentTiming.actualPresentTime > prevPresentTime)
							{
								EventData& data = AddVSyncEvent("Present");
								data.start = prevPresentTime;
								data.finish = presentTiming.actualPresentTime;

								TagData<uint32>& tag = AddVSyncTag();
								tag.timestamp = prevPresentTime;
								tag.data = prevPresentID;

								prevPresentTime = presentTiming.actualPresentTime;
								prevPresentID = presentTiming.presentID;
							}
						}
					}
				}
			}

			nextFrame.queryIndexStart = queryEnd;
			nextFrame.queryIndexCount = 0;
		}

		++frameNumber;
	}

	GPUProfiler::ClockSynchronization GPUProfilerVulkan::GetClockSynchronization(uint32_t nodeIndex)
	{
		GPUProfiler::ClockSynchronization clock;

		NodePayload& node = *nodePayloads[nodeIndex];
		Frame& currentFrame = node.frames[frameNumber % NUM_FRAMES_DELAY];
		
		VkCommandBufferBeginInfo commandBufferBeginInfo;
		commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		commandBufferBeginInfo.pNext = 0;
		commandBufferBeginInfo.pInheritanceInfo = 0;
		commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		VkCommandBuffer CB = currentFrame.commandBuffer;
		VkDevice Device = node.device;
		VkFence Fence = currentFrame.fence;

		(*vulkanFunctions.vkWaitForFences)(Device, 1, &Fence, 1, (uint64_t)-1);
		(*vulkanFunctions.vkResetFences)(Device, 1, &Fence);
		(*vulkanFunctions.vkResetEvent)(Device, nodePayloads[nodeIndex]->event);
		(*vulkanFunctions.vkResetCommandBuffer)(CB, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
		(*vulkanFunctions.vkBeginCommandBuffer)(CB, &commandBufferBeginInfo);
		(*vulkanFunctions.vkCmdResetQueryPool)(CB, nodePayloads[nodeIndex]->queryPool, 0, 1);
		(*vulkanFunctions.vkCmdWaitEvents)(CB, 1, &nodePayloads[nodeIndex]->event, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_HOST_BIT | VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, nullptr, 0, nullptr, 0, nullptr);
		(*vulkanFunctions.vkCmdWriteTimestamp)(CB, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, nodePayloads[nodeIndex]->queryPool, 0);
		(*vulkanFunctions.vkEndCommandBuffer)(CB);

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pNext = nullptr;
		submitInfo.waitSemaphoreCount = 0;
		submitInfo.pWaitSemaphores = nullptr;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &CB;
		submitInfo.signalSemaphoreCount = 0;
		submitInfo.pSignalSemaphores = nullptr;
		(*vulkanFunctions.vkQueueSubmit)(nodePayloads[nodeIndex]->queue, 1, &submitInfo, Fence);

		// SRS - Improve GPU to CPU clock offset calibration by using Vulkan events
		// thanks to cdwfs for concept at https://gist.github.com/cdwfs/4222ca09cb259f8dd50f7f2cf7d09179
		std::this_thread::sleep_for(std::chrono::seconds(1));
		(*vulkanFunctions.vkSetEvent)(Device, nodePayloads[nodeIndex]->event);
		clock.timestampCPU = GetHighPrecisionTime();
		(*vulkanFunctions.vkWaitForFences)(Device, 1, &Fence, 1, (uint64_t)-1);
		(*vulkanFunctions.vkResetFences)(Device, 1, &Fence);
		clock.timestampGPU = 0;
		(*vulkanFunctions.vkGetQueryPoolResults)(Device, nodePayloads[nodeIndex]->queryPool, 0, 1, 8, &clock.timestampGPU, 8, VK_QUERY_RESULT_64_BIT);

		// SRS - Improve GPU to CPU clock frequency scaling by using floating point doubles
		clock.frequencyCPU = GetHighPrecisionFrequency();
		VkPhysicalDeviceProperties Properties;
		(*vulkanFunctions.vkGetPhysicalDeviceProperties)(nodePayloads[nodeIndex]->physicalDevice, &Properties);
		clock.frequencyGPU = (int64_t)(1000000000.0 / (double)Properties.limits.timestampPeriod);

		// SRS - Reset entire query pool to clear clock sync query + any leftover queries from previous run
		(*vulkanFunctions.vkBeginCommandBuffer)(CB, &commandBufferBeginInfo);
		(*vulkanFunctions.vkCmdResetQueryPool)(CB, nodePayloads[nodeIndex]->queryPool, 0, MAX_QUERIES_COUNT);
		(*vulkanFunctions.vkEndCommandBuffer)(CB);
		(*vulkanFunctions.vkQueueSubmit)(nodePayloads[nodeIndex]->queue, 1, &submitInfo, Fence);

		return clock;
	}

	GPUProfilerVulkan::NodePayload::~NodePayload()
	{
		(*vulkanFunctions->vkDestroyEvent)(device, event, nullptr);
		(*vulkanFunctions->vkDestroyCommandPool)(device, commandPool, nullptr);
		(*vulkanFunctions->vkDestroyQueryPool)(device, queryPool, nullptr);
	}

	GPUProfilerVulkan::~GPUProfilerVulkan()
	{
		for (NodePayload* payload : nodePayloads)
		{
			for (Frame& frame : payload->frames)
			{
				(*vulkanFunctions.vkDestroyFence)(payload->device, frame.fence, nullptr);
				(*vulkanFunctions.vkFreeCommandBuffers)(payload->device, payload->commandPool, 1, &frame.commandBuffer);
			}

			Memory::Delete(payload);
		}

		nodePayloads.clear();
	}
}
#else
#include "optick_common.h"
namespace Optick
{
	void InitGpuVulkan(VkDevice* /*vkDevices*/, VkPhysicalDevice* /*vkPhysicalDevices*/, VkQueue* /*vkQueues*/, uint32_t* /*cmdQueuesFamily*/, uint32_t /*numQueues*/, const VulkanFunctions* /*functions*/)
	{
		OPTICK_FAILED("OPTICK_ENABLE_GPU_VULKAN is disabled! Can't initialize GPU Profiler!");
	}
}
#endif //OPTICK_ENABLE_GPU_D3D12
#endif //USE_OPTICK
