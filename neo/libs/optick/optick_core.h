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

#include <mutex>
#include <thread>

#include "optick_common.h"

#include "optick_memory.h"
#include "optick_message.h"
#include "optick_serialization.h"

#include "optick_gpu.h"

#include <atomic>

// We expect to have 1k unique strings going through Optick at once
// The chances to hit a collision are 1 in 10 trillion (odds of a meteor landing on your house)
// We should be quite safe here :)
// https://preshing.com/20110504/hash-collision-probabilities/
// Feel free to add a seed and wait for another strike if armageddon starts
namespace Optick
{
	struct StringHash
	{
		uint64 hash;

		StringHash(size_t h) : hash(h) {}
		StringHash(const char* str) : hash(CalcHash(str)) {}

		bool operator==(const StringHash& other) const { return hash == other.hash; }
		bool operator<(const StringHash& other) const { return hash < other.hash; }

		static uint64 CalcHash(const char* str);
	};
}

// Overriding default hash function to return hash value directly
namespace std
{
	template<>
	struct hash<Optick::StringHash>
	{
		size_t operator()(const Optick::StringHash& x) const
		{
			return (size_t)x.hash;
		}
	};
}

namespace Optick
{
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct Trace;
struct SymbolEngine;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct ScopeHeader
{
	EventTime event;
	uint32 boardNumber;
	int32 threadNumber;
	int32 fiberNumber;
	FrameType::Type type;

	ScopeHeader();
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OutputDataStream& operator << ( OutputDataStream& stream, const ScopeHeader& ob);
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct ScopeData
{
	ScopeHeader header;
	vector<EventData> categories;
	vector<EventData> events;

	ScopeData()
	{
		ResetHeader();
	}

	void AddEvent(const EventData& data)
	{
		events.push_back(data);
		if (data.description->color != Color::Null)
		{
			categories.push_back(data);
		}
	}

	void InitRootEvent(const EventData& data)
	{
		header.event.start = std::min(data.start, header.event.start);
		header.event.finish = std::max(data.finish, header.event.finish);
		AddEvent(data);

		header.type = FrameType::NONE;
		for (int i = 0; i < FrameType::COUNT; ++i)
			if (GetFrameDescription((FrameType::Type)i) == data.description)
				header.type = (FrameType::Type)i;
	}

	void ResetHeader();
	void Send();
	void Clear();
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#if defined(OPTICK_MSVC)
#pragma warning( push )
#pragma warning( disable : 4996 )
#endif //OPTICK_MSVC
template<int N>
struct OptickString
{
	char data[N];
	OptickString() {}
	OptickString<N>& operator=(const char* text) { strncpy(data, text ? text : "null", N - 1); data[N - 1] = 0; return *this; }
	OptickString(const char* text) { *this = text; }
	OptickString(const char* text, uint16_t length) { uint16_t maxLength = std::min((uint16_t)(N - 1), length); strncpy(data, text ? text : "null", maxLength); data[maxLength] = 0; }
};
#if defined(OPTICK_MSVC)
#pragma warning( pop )
#endif
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct Point
{
	float x, y, z;
	Point() {}
	Point(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
	Point(float pos[3]) : x(pos[0]), y(pos[1]), z(pos[2]) {}
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<int N>
OutputDataStream& operator<<(OutputDataStream &stream, const OptickString<N>& ob)
{
	size_t length = strnlen(ob.data, N);
	stream << (uint32)length;
	return stream.Write(ob.data, length);
}
OutputDataStream& operator<<(OutputDataStream& stream, const Point& ob);
OutputDataStream& operator<<(OutputDataStream& stream, const ScopeData& ob);
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef MemoryPool<EventData, 1024> EventBuffer;
typedef MemoryPool<const EventData*, 32> CategoryBuffer;
typedef MemoryPool<SyncData, 1024> SynchronizationBuffer;
typedef MemoryPool<FiberSyncData, 1024> FiberSyncBuffer;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef OptickString<32> ShortString;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef TagData<float> TagFloat;
typedef TagData<int32> TagS32;
typedef TagData<uint32> TagU32;
typedef TagData<uint64> TagU64;
typedef TagData<Point> TagPoint;
typedef TagData<ShortString> TagString;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef MemoryPool<TagFloat, 1024> TagFloatBuffer;
typedef MemoryPool<TagS32, 1024> TagS32Buffer;
typedef MemoryPool<TagU32, 1024> TagU32Buffer;
typedef MemoryPool<TagU64, 1024> TagU64Buffer;
typedef MemoryPool<TagPoint, 64> TagPointBuffer;
typedef MemoryPool<TagString, 1024> TagStringBuffer;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Base64
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
string base64_decode(string const& encoded_string);
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Board
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef MemoryPool<EventDescription, 4096> EventDescriptionList;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class EventDescriptionBoard
{
	// List of stored Event Descriptions
	EventDescriptionList boardDescriptions;

	// Shared Descriptions
	typedef unordered_map<StringHash, EventDescription*> DescriptionMap;
	DescriptionMap sharedDescriptions;
	MemoryBuffer<64 * 1024> sharedNames;
	std::mutex sharedLock;

	const char* CacheString(const char* text);
public:
	EventDescription* CreateDescription(const char* name, const char* file = nullptr, uint32_t line = 0, uint32_t color = Color::Null, uint32_t filter = 0, uint8_t flags = 0);
	EventDescription* CreateSharedDescription(const char* name, const char* file = nullptr, uint32_t line = 0, uint32_t color = Color::Null, uint32_t filter = 0);


	static EventDescriptionBoard& Get();

	const EventDescriptionList& GetEvents() const;

	void Shutdown();

	friend OutputDataStream& operator << (OutputDataStream& stream, const EventDescriptionBoard& ob);
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct EventStorage
{
	Mode::Type currentMode;
	EventBuffer eventBuffer;
	FiberSyncBuffer fiberSyncBuffer;

	TagFloatBuffer tagFloatBuffer;
	TagS32Buffer tagS32Buffer;
	TagU32Buffer tagU32Buffer;
	TagU64Buffer tagU64Buffer;
	TagPointBuffer tagPointBuffer;
	TagStringBuffer tagStringBuffer;

	struct GPUStorage
	{
		static const int MAX_GPU_NODES = 2;
		array<array<EventBuffer, GPU_QUEUE_COUNT>, MAX_GPU_NODES> gpuBuffer;
		GPUContext context;

		void Clear(bool preserveMemory);
		
		EventData* Start(const EventDescription& desc);
		void Stop(EventData& data);
	};
	GPUStorage gpuStorage;

	uint32					    pushPopEventStackIndex;
	array<EventData*, 32>		pushPopEventStack;

	bool isFiberStorage;

	EventStorage();

	OPTICK_INLINE EventData& NextEvent() 
	{
		return eventBuffer.Add(); 
	}

	// Free all temporary memory
	void Clear(bool preserveContent)
	{
		currentMode = Mode::OFF;
		eventBuffer.Clear(preserveContent);
		fiberSyncBuffer.Clear(preserveContent);
		gpuStorage.Clear(preserveContent);
		ClearTags(preserveContent);

		while (pushPopEventStackIndex)
		{
			if (--pushPopEventStackIndex < pushPopEventStack.size())
				pushPopEventStack[pushPopEventStackIndex] = nullptr;
		}
	}

	void ClearTags(bool preserveContent)
	{
		tagFloatBuffer.Clear(preserveContent);
		tagS32Buffer.Clear(preserveContent);
		tagU32Buffer.Clear(preserveContent);
		tagU64Buffer.Clear(preserveContent);
		tagPointBuffer.Clear(preserveContent);
		tagStringBuffer.Clear(preserveContent);
	}

	void Reset()
	{
		Clear(true);
	}
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct ProcessDescription
{
	string name;
	ProcessID processID;
	uint64 uniqueKey;
	ProcessDescription(const char* processName, ProcessID pid, uint64 key);
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct ThreadDescription
{
	string name;
	ThreadID threadID;
	ProcessID processID;
	int32 maxDepth;
	int32 priority;
	uint32 mask;

	bool operator==(const ThreadDescription& other) const { return name == other.name && threadID == other.threadID && processID == other.processID; }
	ThreadDescription(const char* threadName, ThreadID tid, ProcessID pid, int32 maxDepth = 1, int32 priority = 0, uint32 mask = 0);
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct FiberDescription
{
	uint64 id; 

	FiberDescription(uint64 _id)
		: id(_id)
	{}
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct ThreadEntry
{
	ThreadDescription description;
	EventStorage storage;
	EventStorage** threadTLS;

	bool isAlive;

	ThreadEntry(const ThreadDescription& desc, EventStorage** tls) : description(desc), threadTLS(tls), isAlive(true) {}
	void Activate(Mode::Type mode);
	void Sort();
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct FiberEntry
{
	FiberDescription description;
	EventStorage storage;

	FiberEntry(const FiberDescription& desc) : description(desc) {}
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef vector<ThreadEntry*> ThreadList;
typedef vector<FiberEntry*> FiberList;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct SysCallData : EventData
{
	uint64 id;
	uint64 threadID;
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OutputDataStream &operator << (OutputDataStream &stream, const SysCallData &ob);
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SysCallCollector
{
	typedef MemoryPool<SysCallData, 1024 * 32> SysCallPool;
public:
	SysCallPool syscallPool;

	SysCallData& Add();
	void Clear();

	bool Serialize(OutputDataStream& stream);
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct CallstackDesc
{
	uint64 threadID;
	uint64 timestamp;
	uint64* callstack;
	uint8 count;
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CallstackCollector
{
	// Packed callstack list: {ThreadID, Timestamp, Count, Callstack[Count]}
	typedef MemoryPool<uint64, 1024 * 32> CallstacksPool;
	CallstacksPool callstacksPool;
public:
	void Add(const CallstackDesc& desc);
	void Clear();

	bool SerializeModules(OutputDataStream& stream);
	bool SerializeSymbols(OutputDataStream& stream);
	bool SerializeCallstacks(OutputDataStream& stream);

	bool IsEmpty() const;
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct SwitchContextDesc
{
	int64_t timestamp;
	uint64 oldThreadId;
	uint64 newThreadId;
	uint8 cpuId;
	uint8 reason;
};
//////////////////////////////////////////////////////////////////////////
OutputDataStream &operator << (OutputDataStream &stream, const SwitchContextDesc &ob);
//////////////////////////////////////////////////////////////////////////
class SwitchContextCollector
{
	typedef MemoryPool<SwitchContextDesc, 1024 * 32> SwitchContextPool;
	SwitchContextPool switchContextPool;
public:
	void Add(const SwitchContextDesc& desc);
	void Clear();
	bool Serialize(OutputDataStream& stream);
};
//////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct CaptureStatus
{
	enum Type
	{
		OK = 0,
		ERR_TRACER_ALREADY_EXISTS = 1,
		ERR_TRACER_ACCESS_DENIED = 2,
		ERR_TRACER_FAILED = 3,
        ERR_TRACER_INVALID_PASSWORD = 4,
		ERR_TRACER_NOT_IMPLEMENTED = 5,
    };
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct FrameData : public EventData 
{
	uint64_t threadID;
	FrameData() : threadID(INVALID_THREAD_ID) {}
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef MemoryPool<FrameData, 128> FrameBuffer;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct FrameStorage
{
	const EventDescription* m_Description;
	FrameBuffer m_Frames;
	std::atomic<uint32_t> m_FrameNumber;

	void Clear(bool preserveMemory = true)
	{
		m_Frames.Clear(preserveMemory);
	}

	FrameStorage() : m_Description(nullptr) {}
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Core
{
	std::recursive_mutex coreLock;
    std::recursive_mutex threadsLock;

	ThreadList threads;
	FiberList fibers;

	int64 progressReportedLastTimestampMS;

	array<FrameStorage, FrameType::COUNT> frames;
	uint32 boardNumber;

	CallstackCollector callstackCollector;
	SwitchContextCollector switchContextCollector;

	vector<std::pair<string, string>> summary;

	struct Attachment
	{
		string name;
		vector<uint8_t> data;
		File::Type type;
		Attachment(File::Type t, const char* n) : name(n), type(t) {}
	};
	list<Attachment> attachments;

	StateCallback stateCallback;

	vector<ProcessDescription> processDescs;
	vector<ThreadDescription> threadDescs;

	State::Type currentState;
	State::Type pendingState;

	CaptureSettings settings;

	uint32 forcedMainThreadIndex;

	void UpdateEvents();
	bool UpdateState();

	uint32_t BeginUpdateFrame(FrameType::Type frame, int64_t timestamp, uint64_t threadID);
	uint32_t EndUpdateFrame(FrameType::Type frame, int64_t timestamp, uint64_t threadID);

	Core();
	~Core();

	void DumpCapturingProgress();
	void SendHandshakeResponse(CaptureStatus::Type status);


	void DumpEvents(EventStorage& entry, const EventTime& timeSlice, ScopeData& scope);
	void DumpTags(EventStorage& entry, ScopeData& scope);
	void DumpThread(ThreadEntry& entry, const EventTime& timeSlice, ScopeData& scope);
	void DumpFiber(FiberEntry& entry, const EventTime& timeSlice, ScopeData& scope);

	void CleanupThreadsAndFibers();

	void DumpBoard(uint32 mode, EventTime timeSlice);

	void GenerateCommonSummary();
public:
	void Activate(Mode::Type mode);
	volatile Mode::Type currentMode;
	volatile Mode::Type previousMode;

	// Active Frame (is used as buffer)
	static OPTICK_THREAD_LOCAL EventStorage* storage;

	// Resolves symbols
	SymbolEngine* symbolEngine;

	// Controls GPU activity
	// Graphics graphics;

	// System scheduler trace
	Trace* tracer;

	// SysCall Collector
	SysCallCollector syscallCollector;

	// GPU Profiler
	GPUProfiler* gpuProfiler;

	// Returns thread collection
	const vector<ThreadEntry*>& GetThreads() const;

	// Request to start a new capture
	void StartCapture();

	// Request to stop an active capture
	void StopCapture();

	// Request to stop an active capture
	void CancelCapture();

	// Requests to dump current capture
	void DumpCapture();

	// Report switch context event
	bool ReportSwitchContext(const SwitchContextDesc& desc);

	// Report switch context event
	bool ReportStackWalk(const CallstackDesc& desc);

	// Serialize and send current profiling progress
	void DumpProgress(const char* message = "");
	void DumpProgressFormatted(const char* format, ...);

	// Too much time from last report
	bool IsTimeToReportProgress() const;

	// Serialize and send frames
	void DumpFrames(uint32 mode = Mode::DEFAULT);

	// Serialize and send frames
	void DumpSummary();

	// Registers thread and create EventStorage
	ThreadEntry* RegisterThread(const ThreadDescription& description, EventStorage** slot);

	// UnRegisters thread
	bool UnRegisterThread(ThreadID threadId, bool keepAlive = false);

	// Check is registered thread
	bool IsRegistredThread(ThreadID id);

	// Registers finer and create EventStorage
	bool RegisterFiber(const FiberDescription& description, EventStorage** slot);

	// Registers ProcessDescription
	bool RegisterProcessDescription(const ProcessDescription& description);

	// Registers ThreaDescription (used for threads from other processes)
	bool RegisterThreadDescription(const ThreadDescription& description);

	// Sets state change callback
	bool SetStateChangedCallback(StateCallback cb);

	// Attaches a key-value pair to the next capture
	bool AttachSummary(const char* key, const char* value);

	// Attaches a screenshot to the current capture
	bool AttachFile(File::Type type, const char* name, const uint8_t* data, uint32_t size);
	bool AttachFile(File::Type type, const char* name, std::istream& stream);
	bool AttachFile(File::Type type, const char* name, const char* path);
	bool AttachFile(File::Type type, const char* name, const wchar_t* path);

	// Initalizes GPU profiler
	void InitGPUProfiler(GPUProfiler* profiler);

	// Initializes root password for the device
	bool SetSettings(const CaptureSettings& settings);

	// Current Frame Number (since the game started)
	uint32_t GetCurrentFrame(FrameType::Type frameType) const { return frames[frameType].m_FrameNumber; }

	// Returns Frame Description
	const EventDescription* GetFrameDescription(FrameType::Type frame) const;

	// Main Update Function
	void Update();

	// Full Destruction
	void Shutdown();

	// Frame Flip functions
	static uint32_t BeginFrame(FrameType::Type frame, int64_t timestamp, uint64_t threadID) { return Get().BeginUpdateFrame(frame, timestamp, threadID); }
	static uint32_t EndFrame(FrameType::Type frame, int64_t timestamp, uint64_t threadID) { return Get().EndUpdateFrame(frame, timestamp, threadID); }

	// Initialize Main ThreadID
	void SetMainThreadID(uint64_t threadID);

	// NOT Thread Safe singleton (performance)
	static Core& Get();
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}

#endif //USE_OPTICK