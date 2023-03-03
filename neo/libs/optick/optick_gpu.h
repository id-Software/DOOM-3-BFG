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

#pragma once
#include "optick.config.h"

#if USE_OPTICK

#include <atomic>
#include <mutex>

#include "optick_common.h"
#include "optick_memory.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
namespace Optick
{
	const char* GetGPUQueueName(GPUQueueType queue);
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class GPUProfiler
	{
	public:
		static const int MAX_FRAME_EVENTS = 1024;
		static const int NUM_FRAMES_DELAY = 4;
		static const int MAX_QUERIES_COUNT = (2 * MAX_FRAME_EVENTS) * NUM_FRAMES_DELAY;
	protected:

		enum State
		{
			STATE_OFF,
			STATE_STARTING,
			STATE_RUNNING,
			STATE_FINISHING,
		};

		struct ClockSynchronization
		{
			int64_t frequencyCPU;
			int64_t frequencyGPU;
			int64_t timestampCPU;
			int64_t timestampGPU;

			int64_t GetCPUTimestamp(int64_t gpuTimestamp)
			{
				return timestampCPU + (gpuTimestamp - timestampGPU) * frequencyCPU / frequencyGPU;
			}

			ClockSynchronization() : frequencyCPU(0), frequencyGPU(0), timestampCPU(0), timestampGPU(0) {}
		};

		struct QueryFrame
		{
			EventData* frameEvent;
			uint32_t queryIndexStart;
			uint32_t queryIndexCount;

			QueryFrame()
			{
				Reset();
			}

			void Reset()
			{
				frameEvent = nullptr;
				queryIndexStart = (uint32_t)-1;
				queryIndexCount = 0;
			}
		};

		struct Node
		{
			array<QueryFrame, NUM_FRAMES_DELAY> queryGpuframes;
			array<int64_t, MAX_QUERIES_COUNT> queryGpuTimestamps;
			array<int64_t*, MAX_QUERIES_COUNT> queryCpuTimestamps;
			std::atomic<uint32_t> queryIndex;

			ClockSynchronization clock;

			array<EventStorage*, GPU_QUEUE_COUNT> gpuEventStorage;

			uint32_t QueryTimestamp(int64_t* outCpuTimestamp)
			{
				uint32_t index = queryIndex.fetch_add(1) % MAX_QUERIES_COUNT;
				queryCpuTimestamps[index] = outCpuTimestamp;
				return index;
			}

			string name;

			void Reset();

			Node() : queryIndex(0) { gpuEventStorage.fill(nullptr); }
		};

		std::recursive_mutex updateLock;
		volatile State currentState;

		vector<Node*> nodes;
		uint32_t currentNode;

		uint32_t frameNumber;

		void Reset();

		EventData& AddFrameEvent();
		EventData& AddVSyncEvent();
		TagData<uint32>& AddFrameTag();

	public:
		GPUProfiler();

		// Init
		virtual void InitNode(const char* nodeName, uint32_t nodeIndex);

		// Capture Controls 
		virtual void Start(uint32 mode);
		virtual void Stop(uint32 mode);
		virtual void Dump(uint32 mode);

		virtual string GetName() const;

		// Interface to implement
		virtual ClockSynchronization GetClockSynchronization(uint32_t nodeIndex) = 0;
		virtual void QueryTimestamp(void* context, int64_t* cpuTimestampOut) = 0;
		virtual void Flip(void* swapChain) = 0;

		virtual ~GPUProfiler();
	};
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}

#endif //USE_OPTICK
