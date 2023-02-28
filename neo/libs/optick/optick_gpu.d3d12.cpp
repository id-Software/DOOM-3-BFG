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
#if OPTICK_ENABLE_GPU_D3D12

#include "optick_common.h"
#include "optick_memory.h"
#include "optick_core.h"
#include "optick_gpu.h"

#include <atomic>
#include <thread>

#include <d3d12.h>
#include <dxgi.h>
#include <dxgi1_4.h>


#define OPTICK_CHECK(args) do { HRESULT __hr = args; (void)__hr; OPTICK_ASSERT(__hr == S_OK, "Failed check"); } while(false);

namespace Optick
{
	class GPUProfilerD3D12 : public GPUProfiler
	{
		struct Frame
		{
			ID3D12CommandAllocator* commandAllocator;
			ID3D12GraphicsCommandList* commandList;

			Frame() : commandAllocator(nullptr), commandList(nullptr)
			{
				Reset();
			}

			void Reset()
			{
			}

			void Shutdown();

			~Frame()
			{
				Shutdown();
			}
		};

		struct NodePayload
		{
			ID3D12CommandQueue* commandQueue;
			ID3D12QueryHeap* queryHeap;
			ID3D12Fence* syncFence;
			array<Frame, NUM_FRAMES_DELAY> frames;

			NodePayload() : commandQueue(nullptr), queryHeap(nullptr), syncFence(nullptr) {}
			~NodePayload();
		};
		vector<NodePayload*> nodePayloads;

		ID3D12Resource* queryBuffer;
		ID3D12Device* device;

		// VSync Stats
		DXGI_FRAME_STATISTICS prevFrameStatistics;

		//void UpdateRange(uint32_t start, uint32_t finish)
		void InitNodeInternal(const char* nodeName, uint32_t nodeIndex, ID3D12CommandQueue* pCmdQueue);

		void ResolveTimestamps(uint32_t startIndex, uint32_t count);

		void WaitForFrame(uint64_t frameNumber);

	public:
		GPUProfilerD3D12();
		~GPUProfilerD3D12();

		void InitDevice(ID3D12Device* pDevice, ID3D12CommandQueue** pCommandQueues, uint32_t numCommandQueues);

		void QueryTimestamp(ID3D12GraphicsCommandList* context, int64_t* outCpuTimestamp);

		void Flip(IDXGISwapChain* swapChain);


		// Interface implementation
		ClockSynchronization GetClockSynchronization(uint32_t nodeIndex) override;

		void QueryTimestamp(void* context, int64_t* outCpuTimestamp) override
		{
			QueryTimestamp((ID3D12GraphicsCommandList*)context, outCpuTimestamp);
		}

		void Flip(void* swapChain) override
		{
			Flip(static_cast<IDXGISwapChain*>(swapChain));
		}
	};

	template <class T> void SafeRelease(T **ppT)
	{
		if (*ppT)
		{
			(*ppT)->Release();
			*ppT = NULL;
		}
	}

	void InitGpuD3D12(ID3D12Device* device, ID3D12CommandQueue** cmdQueues, uint32_t numQueues)
	{
		GPUProfilerD3D12* gpuProfiler = Memory::New<GPUProfilerD3D12>();
		gpuProfiler->InitDevice(device, cmdQueues, numQueues);
		Core::Get().InitGPUProfiler(gpuProfiler);
	}

	GPUProfilerD3D12::GPUProfilerD3D12() :  queryBuffer(nullptr), device(nullptr)
	{
		prevFrameStatistics = { 0 };
	}

	GPUProfilerD3D12::~GPUProfilerD3D12()
	{
		for (NodePayload* payload : nodePayloads)
			Memory::Delete(payload);
		nodePayloads.clear();

		for (Node* node : nodes)
			Memory::Delete(node);
		nodes.clear();

		SafeRelease(&queryBuffer);
	}

	void GPUProfilerD3D12::InitDevice(ID3D12Device* pDevice, ID3D12CommandQueue** pCommandQueues, uint32_t numCommandQueues)
	{
		device = pDevice;

		uint32_t nodeCount = numCommandQueues; // device->GetNodeCount();

		nodes.resize(nodeCount);
		nodePayloads.resize(nodeCount);

		D3D12_HEAP_PROPERTIES heapDesc;
		heapDesc.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapDesc.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapDesc.CreationNodeMask = 0;
		heapDesc.VisibleNodeMask = (1u << nodeCount) - 1u;
		heapDesc.Type = D3D12_HEAP_TYPE_READBACK;

		D3D12_RESOURCE_DESC resourceDesc;
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resourceDesc.Alignment = 0;
		resourceDesc.Width = MAX_QUERIES_COUNT * sizeof(int64_t);
		resourceDesc.Height = 1;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.MipLevels = 1;
		resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.SampleDesc.Quality = 0;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		OPTICK_CHECK(device->CreateCommittedResource(
			&heapDesc,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&queryBuffer)));

		// Get Device Name
		LUID adapterLUID = pDevice->GetAdapterLuid();

		IDXGIFactory4* factory;
		OPTICK_CHECK(CreateDXGIFactory2(0, IID_PPV_ARGS(&factory)));

		IDXGIAdapter1* adapter;
		factory->EnumAdapterByLuid(adapterLUID, IID_PPV_ARGS(&adapter));
		
		DXGI_ADAPTER_DESC1 desc;
		adapter->GetDesc1(&desc);

		adapter->Release();
		factory->Release();

		char deviceName[128] = { 0 };
		wcstombs_s(deviceName, desc.Description, OPTICK_ARRAY_SIZE(deviceName) - 1);

		for (uint32_t nodeIndex = 0; nodeIndex < nodeCount; ++nodeIndex)
			InitNodeInternal(deviceName, nodeIndex, pCommandQueues[nodeIndex]);
	}

	void GPUProfilerD3D12::InitNodeInternal(const char* nodeName, uint32_t nodeIndex, ID3D12CommandQueue* pCmdQueue)
	{
		GPUProfiler::InitNode(nodeName, nodeIndex);

		NodePayload* node = Memory::New<NodePayload>();
		nodePayloads[nodeIndex] = node;
		node->commandQueue = pCmdQueue;

		D3D12_QUERY_HEAP_DESC queryHeapDesc;
		queryHeapDesc.Count = MAX_QUERIES_COUNT;
		queryHeapDesc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
		queryHeapDesc.NodeMask = 1u << nodeIndex;
		OPTICK_CHECK(device->CreateQueryHeap(&queryHeapDesc, IID_PPV_ARGS(&node->queryHeap)));

		OPTICK_CHECK(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&node->syncFence)));

		for (Frame& frame : node->frames)
		{
			OPTICK_CHECK(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&frame.commandAllocator)));
			OPTICK_CHECK(device->CreateCommandList(1u << nodeIndex, D3D12_COMMAND_LIST_TYPE_DIRECT, frame.commandAllocator, nullptr, IID_PPV_ARGS(&frame.commandList)));
			OPTICK_CHECK(frame.commandList->Close());
		}
	}

	void GPUProfilerD3D12::QueryTimestamp(ID3D12GraphicsCommandList* context, int64_t* outCpuTimestamp)
	{
		if (currentState == STATE_RUNNING)
		{
			uint32_t index = nodes[currentNode]->QueryTimestamp(outCpuTimestamp);
			context->EndQuery(nodePayloads[currentNode]->queryHeap, D3D12_QUERY_TYPE_TIMESTAMP, index);
		}
	}

	void GPUProfilerD3D12::ResolveTimestamps(uint32_t startIndex, uint32_t count)
	{
		if (count)
		{
			Node* node = nodes[currentNode];

			D3D12_RANGE range = { sizeof(uint64_t)*startIndex, sizeof(uint64_t)*(startIndex + count) };
			void* pData = nullptr;
			queryBuffer->Map(0, &range, &pData);
			memcpy(&node->queryGpuTimestamps[startIndex], (uint64_t*)pData + startIndex, sizeof(uint64_t) * count);
			queryBuffer->Unmap(0, 0);

			// Convert GPU timestamps => CPU Timestamps
			for (uint32_t index = startIndex; index < startIndex + count; ++index)
				*node->queryCpuTimestamps[index] = node->clock.GetCPUTimestamp(node->queryGpuTimestamps[index]);
		}
	}

	void GPUProfilerD3D12::WaitForFrame(uint64_t frameNumberToWait)
	{
		OPTICK_EVENT();

		NodePayload* payload = nodePayloads[currentNode];
		while (frameNumberToWait > payload->syncFence->GetCompletedValue())
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}

	void GPUProfilerD3D12::Flip(IDXGISwapChain* swapChain)
	{
		OPTICK_CATEGORY("GPUProfilerD3D12::Flip", Category::Debug);

		std::lock_guard<std::recursive_mutex> lock(updateLock);

		if (currentState == STATE_STARTING)
			currentState = STATE_RUNNING;

		if (currentState == STATE_RUNNING)
		{
			Node& node = *nodes[currentNode];
			NodePayload& payload = *nodePayloads[currentNode];

			uint32_t currentFrameIndex = frameNumber % NUM_FRAMES_DELAY;
			uint32_t nextFrameIndex = (frameNumber + 1) % NUM_FRAMES_DELAY;

			//Frame& currentFrame = frames[frameNumber % NUM_FRAMES_DELAY];
			//Frame& nextFrame = frames[(frameNumber + 1) % NUM_FRAMES_DELAY];

			QueryFrame& currentFrame = node.queryGpuframes[currentFrameIndex];
			QueryFrame& nextFrame = node.queryGpuframes[nextFrameIndex];

			ID3D12GraphicsCommandList* commandList = payload.frames[currentFrameIndex].commandList;
			ID3D12CommandAllocator* commandAllocator = payload.frames[currentFrameIndex].commandAllocator;
			commandAllocator->Reset();
			commandList->Reset(commandAllocator, nullptr);

			if (EventData* frameEvent = currentFrame.frameEvent)
				QueryTimestamp(commandList, &frameEvent->finish);

			// Generate GPU Frame event for the next frame
			EventData& event = AddFrameEvent();
			QueryTimestamp(commandList, &event.start);
			QueryTimestamp(commandList, &AddFrameTag().timestamp);
			nextFrame.frameEvent = &event;

			uint32_t queryBegin = currentFrame.queryIndexStart;
			uint32_t queryEnd = node.queryIndex;

			if (queryBegin != (uint32_t)-1)
			{
				OPTICK_ASSERT(queryEnd - queryBegin <= MAX_QUERIES_COUNT, "Too many queries in one frame? Increase GPUProfiler::MAX_QUERIES_COUNT to fix the problem!");
				currentFrame.queryIndexCount = queryEnd - queryBegin;

				uint32_t startIndex = queryBegin % MAX_QUERIES_COUNT;
				uint32_t finishIndex = queryEnd % MAX_QUERIES_COUNT;

				if (startIndex < finishIndex)
				{
					commandList->ResolveQueryData(payload.queryHeap, D3D12_QUERY_TYPE_TIMESTAMP, startIndex, queryEnd - queryBegin, queryBuffer, startIndex * sizeof(int64_t));
				}
				else
				{
					commandList->ResolveQueryData(payload.queryHeap, D3D12_QUERY_TYPE_TIMESTAMP, startIndex, MAX_QUERIES_COUNT - startIndex, queryBuffer, startIndex * sizeof(int64_t));
					commandList->ResolveQueryData(payload.queryHeap, D3D12_QUERY_TYPE_TIMESTAMP, 0, finishIndex, queryBuffer, 0);
				}
			}

			commandList->Close();

			payload.commandQueue->ExecuteCommandLists(1, (ID3D12CommandList*const*)&commandList);
			payload.commandQueue->Signal(payload.syncFence, frameNumber);

			// Preparing Next Frame
			// Try resolve timestamps for the current frame
			if (frameNumber >= NUM_FRAMES_DELAY && nextFrame.queryIndexCount)
			{
				WaitForFrame(frameNumber + 1 - NUM_FRAMES_DELAY);

				uint32_t resolveStart = nextFrame.queryIndexStart % MAX_QUERIES_COUNT;
				uint32_t resolveFinish = resolveStart + nextFrame.queryIndexCount;
				ResolveTimestamps(resolveStart, std::min<uint32_t>(resolveFinish, MAX_QUERIES_COUNT) - resolveStart);
				if (resolveFinish > MAX_QUERIES_COUNT)
					ResolveTimestamps(0, resolveFinish - MAX_QUERIES_COUNT);
			}
				
			nextFrame.queryIndexStart = queryEnd;
			nextFrame.queryIndexCount = 0;

			// Process VSync
			DXGI_FRAME_STATISTICS currentFrameStatistics = { 0 };
			HRESULT result = swapChain->GetFrameStatistics(&currentFrameStatistics);
			if ((result == S_OK) && (prevFrameStatistics.PresentCount + 1 == currentFrameStatistics.PresentCount))
			{
				EventData& data = AddVSyncEvent();
				data.start = prevFrameStatistics.SyncQPCTime.QuadPart;
				data.finish = currentFrameStatistics.SyncQPCTime.QuadPart;
			}
			prevFrameStatistics = currentFrameStatistics;
		}

		++frameNumber;
	}

	GPUProfiler::ClockSynchronization GPUProfilerD3D12::GetClockSynchronization(uint32_t nodeIndex)
	{
		ClockSynchronization clock;
		clock.frequencyCPU = GetHighPrecisionFrequency();
		nodePayloads[nodeIndex]->commandQueue->GetTimestampFrequency((uint64_t*)&clock.frequencyGPU);
		nodePayloads[nodeIndex]->commandQueue->GetClockCalibration((uint64_t*)&clock.timestampGPU, (uint64_t*)&clock.timestampCPU);
		return clock;
	}

	GPUProfilerD3D12::NodePayload::~NodePayload()
	{
		SafeRelease(&queryHeap);
		SafeRelease(&syncFence);
	}

	void GPUProfilerD3D12::Frame::Shutdown()
	{
		SafeRelease(&commandAllocator);
		SafeRelease(&commandList);
	}
}

#else
#include "optick_common.h"

namespace Optick
{
	void InitGpuD3D12(ID3D12Device* /*device*/, ID3D12CommandQueue** /*cmdQueues*/, uint32_t /*numQueues*/)
	{
		OPTICK_FAILED("OPTICK_ENABLE_GPU_D3D12 is disabled! Can't initialize GPU Profiler!");
	}
}

#endif //OPTICK_ENABLE_GPU_D3D12
#endif //USE_OPTICK