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

#include "optick_core.h"

#if USE_OPTICK

#include "optick.h"
#include "optick_server.h"

#include <algorithm>
#include <fstream>
#include <iomanip>

//////////////////////////////////////////////////////////////////////////
// Start of the Platform-specific stuff
//////////////////////////////////////////////////////////////////////////
#if defined(OPTICK_MSVC)
#include "optick_core.win.h"
#elif defined(OPTICK_LINUX)
#include "optick_core.linux.h"
#elif defined(OPTICK_OSX)
#include "optick_core.macos.h"
#elif defined(OPTICK_PS4)
#include "optick_core.ps4.h"
#elif defined(OPTICK_FREEBSD)
#include "optick_core.freebsd.h"
#endif
//////////////////////////////////////////////////////////////////////////
// End of the Platform-specific stuff
//////////////////////////////////////////////////////////////////////////

extern "C" Optick::EventData* NextEvent()
{
	if (Optick::EventStorage* storage = Optick::Core::storage)
	{
		return &storage->NextEvent();
	}

	return nullptr;
}

namespace Optick
{
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void* (*Memory::allocate)(size_t) = [](size_t size)->void* { return operator new(size); };
void (*Memory::deallocate)(void* p) = [](void* p) { operator delete(p); };
void (*Memory::initThread)(void) = nullptr;
#if defined(OPTICK_32BIT)
	std::atomic<uint32_t> Memory::memAllocated;
#else
	std::atomic<uint64_t> Memory::memAllocated;
#endif
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint64_t MurmurHash64A(const void * key, int len, uint64_t seed)
{
	const uint64_t m = 0xc6a4a7935bd1e995;
	const int r = 47;

	uint64_t h = seed ^ (len * m);

	const uint64_t * data = (const uint64_t *)key;
	const uint64_t * end = data + (len / 8);

	while (data != end)
	{
		uint64_t k = *data++;

		k *= m;
		k ^= k >> r;
		k *= m;

		h ^= k;
		h *= m;
	}

	const unsigned char * data2 = (const unsigned char*)data;

	switch (len & 7)
	{
	case 7: h ^= uint64_t(data2[6]) << 48; // fallthrough
	case 6: h ^= uint64_t(data2[5]) << 40; // fallthrough
	case 5: h ^= uint64_t(data2[4]) << 32; // fallthrough
	case 4: h ^= uint64_t(data2[3]) << 24; // fallthrough
	case 3: h ^= uint64_t(data2[2]) << 16; // fallthrough
	case 2: h ^= uint64_t(data2[1]) << 8;  // fallthrough
	case 1: h ^= uint64_t(data2[0]);       // fallthrough
		h *= m;
	};

	h ^= h >> r;
	h *= m;
	h ^= h >> r;

	return h;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint64_t StringHash::CalcHash(const char* str)
{
	return MurmurHash64A(str, (int)strlen(str), 0);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Base 64
// https://renenyffenegger.ch/notes/development/Base64/Encoding-and-decoding-base-64-with-cpp
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static inline bool is_base64(unsigned char c) {
	return (isalnum(c) || (c == '+') || (c == '/'));
}
string base64_decode(string const& encoded_string) {
	static string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	int in_len = (int)encoded_string.size();
	int i = 0;
	int j = 0;
	int in_ = 0;
	unsigned char char_array_4[4], char_array_3[3];
	string ret;

	while (in_len-- && (encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
		char_array_4[i++] = encoded_string[in_]; in_++;
		if (i == 4) {
			for (i = 0; i < 4; i++)
				char_array_4[i] = (unsigned char)base64_chars.find(char_array_4[i]);

			char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
			char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
			char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

			for (i = 0; (i < 3); i++)
				ret += char_array_3[i];
			i = 0;
		}
	}

	if (i) {
		for (j = i; j < 4; j++)
			char_array_4[j] = 0;

		for (j = 0; j < 4; j++)
			char_array_4[j] = (unsigned char)base64_chars.find(char_array_4[j]);

		char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
		char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
		char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

		for (j = 0; (j < i - 1); j++) ret += char_array_3[j];
	}

	return ret;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Get current time in milliseconds
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int64 GetTimeMilliSeconds()
{
	return Platform::GetTime() * 1000 / Platform::GetFrequency();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int64 TicksToMs(int64 ticks)
{
	return ticks * 1000 / Platform::GetFrequency();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int64 TicksToUs(int64 ticks)
{
	return ticks * 1000000 / Platform::GetFrequency();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<class T>
OutputDataStream& operator<<(OutputDataStream& stream, const TagData<T>& ob)
{
	return stream << ob.timestamp << ob.description->index << ob.data;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OutputDataStream& operator<<(OutputDataStream& os, const Symbol * const symbol)
{
	OPTICK_VERIFY(symbol, "Can't serialize NULL symbol!", return os);
	return os << symbol->address << symbol->function << symbol->file << symbol->line;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OutputDataStream& operator<<(OutputDataStream& os, const Module& module)
{
	return os << module.path << (uint64)module.address << (uint64)module.size;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// VS TODO: Replace with random access iterator for MemoryPool
template<class T, uint32 SIZE>
void SortMemoryPool(MemoryPool<T, SIZE>& memoryPool)
{
	size_t count = memoryPool.Size();
	if (count == 0)
		return;

	vector<T> memoryArray;
	memoryArray.resize(count);
	memoryPool.ToArray(&memoryArray[0]);

	std::sort(memoryArray.begin(), memoryArray.end());

	memoryPool.Clear(true);

	for (const T& item : memoryArray)
		memoryPool.Add(item);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
EventDescription* EventDescription::Create(const char* eventName, const char* fileName, const unsigned long fileLine, const unsigned long eventColor /*= Color::Null*/, const unsigned long filter /*= 0*/, const uint8_t eventFlags /*= 0*/)
{
	return EventDescriptionBoard::Get().CreateDescription(eventName, fileName, fileLine, eventColor, filter, eventFlags);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
EventDescription* EventDescription::CreateShared(const char* eventName, const char* fileName, const unsigned long fileLine, const unsigned long eventColor /*= Color::Null*/, const unsigned long filter /*= 0*/)
{
	return EventDescriptionBoard::Get().CreateSharedDescription(eventName, fileName, fileLine, eventColor, filter);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
EventDescription::EventDescription() : name(""), file(""), line(0), index((uint32_t)-1), color(0), filter(0), flags(0)
{
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
EventDescription& EventDescription::operator=(const EventDescription&)
{
	OPTICK_FAILED("It is pointless to copy EventDescription. Please, check you logic!"); return *this;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
EventData* Event::Start(const EventDescription& description)
{
	EventData* result = nullptr;

	if (EventStorage* storage = Core::storage)
	{
		result = &storage->NextEvent();
		result->description = &description;
		result->Start();
	}
	return result;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Event::Stop(EventData& data)
{
	if (Core::storage != nullptr)
	{
		data.Stop();
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void OPTICK_INLINE PushEvent(EventStorage* pStorage, const EventDescription* description, int64_t timestampStart)
{
	if (EventStorage* storage = pStorage)
	{
		if (storage->pushPopEventStackIndex++ < storage->pushPopEventStack.size())
		{
			EventData& result = storage->NextEvent();
			result.description = description;
			result.start = timestampStart;
			result.finish = EventTime::INVALID_TIMESTAMP;
			storage->pushPopEventStack[storage->pushPopEventStackIndex - 1] = &result;
		}
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void OPTICK_INLINE PopEvent(EventStorage* pStorage, int64_t timestampFinish)
{
	if (EventStorage* storage = pStorage)
		if (storage->pushPopEventStackIndex > 0)
			if (--(storage->pushPopEventStackIndex) < storage->pushPopEventStack.size())
				storage->pushPopEventStack[storage->pushPopEventStackIndex]->finish = timestampFinish;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Event::Push(const char* name)
{
	if (EventStorage* storage = Core::storage)
	{
		EventDescription* desc = EventDescription::CreateShared(name);
		PushEvent(storage, desc, GetHighPrecisionTime());
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Event::Push(const EventDescription& description)
{
	PushEvent(Core::storage, &description, GetHighPrecisionTime());
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Event::Pop()
{
	PopEvent(Core::storage, GetHighPrecisionTime());
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Event::Add(EventStorage* storage, const EventDescription* description, int64_t timestampStart, int64_t timestampFinish)
{
	EventData& data = storage->eventBuffer.Add();
	data.description = description;
	data.start = timestampStart;
	data.finish = timestampFinish;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Event::Push(EventStorage* storage, const EventDescription* description, int64_t timestampStart)
{
	PushEvent(storage, description, timestampStart);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Event::Pop(EventStorage* storage, int64_t timestampFinish)
{
	PopEvent(storage, timestampFinish);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
EventData* GPUEvent::Start(const EventDescription& description)
{
	EventData* result = nullptr;

	if (EventStorage* storage = Core::storage)
		result = storage->gpuStorage.Start(description);

	return result;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void GPUEvent::Stop(EventData& data)
{
	if (EventStorage* storage = Core::storage)
		storage->gpuStorage.Stop(data);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FiberSyncData::AttachToThread(EventStorage* storage, uint64_t threadId)
{
	if (storage)
	{
		FiberSyncData& data = storage->fiberSyncBuffer.Add();
		data.Start();
		data.finish = EventTime::INVALID_TIMESTAMP;
		data.threadId = threadId;
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FiberSyncData::DetachFromThread(EventStorage* storage)
{
	if (storage)
	{
		if (FiberSyncData* syncData = storage->fiberSyncBuffer.Back())
		{
			syncData->Stop();
		}
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Tag::Attach(const EventDescription& description, float val)
{
	if (EventStorage* storage = Core::storage)
		if (storage->currentMode & Mode::TAGS)
			storage->tagFloatBuffer.Add(TagFloat(description, val));
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Tag::Attach(const EventDescription& description, int32_t val)
{
	if (EventStorage* storage = Core::storage)
		if (storage->currentMode & Mode::TAGS)
			storage->tagS32Buffer.Add(TagS32(description, val));
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Tag::Attach(const EventDescription& description, uint32_t val)
{
	if (EventStorage* storage = Core::storage)
		if (storage->currentMode & Mode::TAGS)
			storage->tagU32Buffer.Add(TagU32(description, val));
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Tag::Attach(const EventDescription& description, uint64_t val)
{
	if (EventStorage* storage = Core::storage)
		if (storage->currentMode & Mode::TAGS)
			storage->tagU64Buffer.Add(TagU64(description, val));
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Tag::Attach(const EventDescription& description, float val[3])
{
	if (EventStorage* storage = Core::storage)
		if (storage->currentMode & Mode::TAGS)
			storage->tagPointBuffer.Add(TagPoint(description, val));
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Tag::Attach(const EventDescription& description, const char* val)
{
	if (EventStorage* storage = Core::storage)
		if (storage->currentMode & Mode::TAGS)
			storage->tagStringBuffer.Add(TagString(description, val));
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Tag::Attach(const EventDescription& description, const char* val, uint16_t length)
{
	if (EventStorage * storage = Core::storage)
		if (storage->currentMode & Mode::TAGS)
			storage->tagStringBuffer.Add(TagString(description, val, length));
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OutputDataStream & operator<<(OutputDataStream &stream, const EventDescription &ob)
{
	return stream << ob.name << ob.file << ob.line << ob.filter << ob.color << (float)0.0f << ob.flags;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OutputDataStream& operator<<(OutputDataStream& stream, const EventTime& ob)
{
	return stream << ob.start << ob.finish;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OutputDataStream& operator<<(OutputDataStream& stream, const EventData& ob)
{
	return stream << (EventTime)(ob) << (ob.description ? ob.description->index : (uint32)-1);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OutputDataStream& operator<<(OutputDataStream& stream, const SyncData& ob)
{
	return stream << (EventTime)(ob) << ob.core << ob.reason << ob.newThreadId;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OutputDataStream& operator<<(OutputDataStream& stream, const FiberSyncData& ob)
{
	return stream << (EventTime)(ob) << ob.threadId;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OutputDataStream& operator<<(OutputDataStream& stream, const FrameData& ob)
{
	return stream << (EventData)(ob) << ob.threadID;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static std::mutex& GetBoardLock()
{
	// Initialize as static local variable to prevent problems with static initialization order
	static std::mutex lock;
	return lock;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
EventDescriptionBoard& EventDescriptionBoard::Get()
{
	static EventDescriptionBoard instance;
	return instance;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const EventDescriptionList& EventDescriptionBoard::GetEvents() const
{
	return boardDescriptions;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void EventDescriptionBoard::Shutdown()
{
	boardDescriptions.Clear(false);
	sharedNames.Clear(false);
	sharedDescriptions.clear();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
EventDescription* EventDescriptionBoard::CreateDescription(const char* name, const char* file /*= nullptr*/, uint32_t line /*= 0*/, uint32_t color /*= Color::Null*/, uint32_t filter /*= 0*/, uint8_t flags /*= 0*/)
{
	std::lock_guard<std::mutex> lock(GetBoardLock());

	size_t index = boardDescriptions.Size();

	EventDescription& desc = boardDescriptions.Add();
	desc.index = (uint32)index;
	desc.name = (flags & EventDescription::COPY_NAME_STRING) != 0 ? CacheString(name) : name;
	desc.file = (flags & EventDescription::COPY_FILENAME_STRING) != 0 ? CacheString(file) : file;
	desc.line = line;
	desc.color = color;
	desc.filter = filter;
	desc.flags = flags;

	return &desc;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
EventDescription* EventDescriptionBoard::CreateSharedDescription(const char* name, const char* file /*= nullptr*/, uint32_t line /*= 0*/, uint32_t color /*= Color::Null*/, uint32_t filter /*= 0*/)
{
	StringHash nameHash(name);

	std::lock_guard<std::mutex> lock(sharedLock);

	std::pair<DescriptionMap::iterator, bool> cached = sharedDescriptions.insert({ nameHash, nullptr });

	if (cached.second)
	{
		const char* nameCopy = CacheString(name);
		cached.first->second = CreateDescription(nameCopy, file, line, color, filter);
	}

	return cached.first->second;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char* EventDescriptionBoard::CacheString(const char* name)
{
	return sharedNames.Add(name, strlen(name) + 1, false);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OutputDataStream& operator << (OutputDataStream& stream, const EventDescriptionBoard& ob)
{
	std::lock_guard<std::mutex> lock(GetBoardLock());
	stream << ob.GetEvents();
	return stream;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ProcessDescription::ProcessDescription(const char* processName, ProcessID pid, uint64 key) : name(processName), processID(pid), uniqueKey(key)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ThreadDescription::ThreadDescription(const char* threadName, ThreadID tid, ProcessID pid, int32 _maxDepth /*= 1*/, int32 _priority /*= 0*/, uint32 _mask /*= 0*/)
	: name(threadName), threadID(tid), processID(pid), maxDepth(_maxDepth), priority(_priority), mask(_mask)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int64_t GetHighPrecisionTime()
{
	return Platform::GetTime();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int64_t GetHighPrecisionFrequency()
{
	return Platform::GetFrequency();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OutputDataStream & operator<<(OutputDataStream &stream, const SysCallData &ob)
{
	return stream << (const EventData&)ob << ob.threadID << ob.id;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SysCallData& SysCallCollector::Add()
{
	return syscallPool.Add();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SysCallCollector::Clear()
{
	syscallPool.Clear(false);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool SysCallCollector::Serialize(OutputDataStream& stream)
{
	stream << syscallPool;

	if (!syscallPool.IsEmpty())
	{
		syscallPool.Clear(false);
		return true;
	}

	return false;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CallstackCollector::Add(const CallstackDesc& desc)
{
	if (uint64* storage = callstacksPool.TryAdd(desc.count + 3))
	{
		storage[0] = desc.threadID;
		storage[1] = desc.timestamp;
		storage[2] = desc.count;

		for (uint64 i = 0; i < desc.count; ++i)
		{
			storage[3 + i] = desc.callstack[desc.count - i - 1];
		}
	}
	else
	{
		uint64& item0 = callstacksPool.Add();
		uint64& item1 = callstacksPool.Add();
		uint64& item2 = callstacksPool.Add();

		item0 = desc.threadID;
		item1 = desc.timestamp;
		item2 = desc.count;

		for (uint64 i = 0; i < desc.count; ++i)
		{
			callstacksPool.Add() = desc.callstack[desc.count - i - 1];
		}
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CallstackCollector::Clear()
{
	callstacksPool.Clear(false);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CallstackCollector::SerializeModules(OutputDataStream& stream)
{
	if (SymbolEngine* symEngine = Core::Get().symbolEngine)
	{
		stream << symEngine->GetModules();
		return true;
	}
	else
	{
		stream << (int)0;
	}
	return false;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CallstackCollector::SerializeSymbols(OutputDataStream& stream)
{
	typedef unordered_set<uint64> SymbolSet;
	SymbolSet symbolSet;

	Core::Get().DumpProgress("Collecting Callstacks...");

	for (CallstacksPool::const_iterator it = callstacksPool.begin(); it != callstacksPool.end();)
	{
		CallstacksPool::const_iterator startIt = it;
		OPTICK_UNUSED(startIt);

		uint64 threadID = *it;
		OPTICK_UNUSED(threadID);
		++it; //Skip ThreadID
		uint64 timestamp = *it;
		OPTICK_UNUSED
		(timestamp);
		++it; //Skip Timestamp
		uint64 count = *it;
		count = (count & 0xFF);
		++it; //Skip Count

		bool isBadAddrFound = false;

		for (uint64 i = 0; i < count; ++i)
		{
			uint64 address = *it;
			++it;

			if (address == 0)
			{
				isBadAddrFound = true;
			}

			if (!isBadAddrFound)
			{
				symbolSet.insert(address);
			}
		}
	}

	SymbolEngine* symEngine = Core::Get().symbolEngine;

	vector<const Symbol*> symbols;
	symbols.reserve(symbolSet.size());

	Core::Get().DumpProgress("Resolving addresses ... ");

	if (symEngine)
	{
		int total = (int)symbolSet.size();
		const int progressBatchSize = 100;
		for (auto it = symbolSet.begin(); it != symbolSet.end(); ++it)
		{
			uint64 address = *it;
			if (const Symbol* symbol = symEngine->GetSymbol(address))
			{
				symbols.push_back(symbol);

				if ((symbols.size() % progressBatchSize == 0) && Core::Get().IsTimeToReportProgress())
				{
					Core::Get().DumpProgressFormatted("Resolving addresses %d / %d", (int)symbols.size(), total);
				}
			}
		}
	}

	stream << symbols;
	return true;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CallstackCollector::SerializeCallstacks(OutputDataStream& stream)
{
	stream << callstacksPool;

	if (!callstacksPool.IsEmpty())
	{
		callstacksPool.Clear(false);
		return true;
	}

	return false;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CallstackCollector::IsEmpty() const
{
	return callstacksPool.IsEmpty();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OutputDataStream & operator<<(OutputDataStream &stream, const SwitchContextDesc &ob)
{
	return stream << ob.timestamp << ob.oldThreadId << ob.newThreadId << ob.cpuId << ob.reason;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SwitchContextCollector::Add(const SwitchContextDesc& desc)
{
	switchContextPool.Add() = desc;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SwitchContextCollector::Clear()
{
	switchContextPool.Clear(false);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool SwitchContextCollector::Serialize(OutputDataStream& stream)
{
	stream << switchContextPool;

	if (!switchContextPool.IsEmpty())
	{
		switchContextPool.Clear(false);
		return true;
	}

	return false;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#if defined(OPTICK_MSVC)
#include <intrin.h>
#define CPUID(INFO, ID) __cpuid(INFO, ID)
#elif (defined(__ANDROID__) || defined(OPTICK_ARM))
// Nothing
#elif defined(OPTICK_GCC)
#include <cpuid.h>
#define CPUID(INFO, ID) __cpuid(ID, INFO[0], INFO[1], INFO[2], INFO[3])
#else
#error Platform is not supported!
#endif
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
string GetCPUName()
{
#if defined(__ANDROID__)
	FILE * fp = popen("cat /proc/cpuinfo | grep -m1 'model name'","r");
    char res[128] = {0};
    fread(res, 1, sizeof(res)-1, fp);
    fclose(fp);
    char* name = strstr(res, ":");
    if (name && strlen(name) > 2)
    {
    	string s = name + 2;
    	s.erase(std::remove(s.begin(), s.end(), '\n'), s.end());
    	return s;
    }
	return "Undefined CPU";
#elif defined(OPTICK_ARM)
	#if defined(OPTICK_ARM32)
		return "ARM 32-bit";
	#else
		return "ARM 64-bit";
	#endif
#else
	int cpuInfo[4] = { -1 };
	char cpuBrandString[0x40] = { 0 };
	CPUID(cpuInfo, 0x80000000);
	unsigned nExIds = cpuInfo[0];
	for (unsigned i = 0x80000000; i <= nExIds; ++i)
	{
		CPUID(cpuInfo, i);
		if (i == 0x80000002)
			memcpy(cpuBrandString, cpuInfo, sizeof(cpuInfo));
		else if (i == 0x80000003)
			memcpy(cpuBrandString + 16, cpuInfo, sizeof(cpuInfo));
		else if (i == 0x80000004)
			memcpy(cpuBrandString + 32, cpuInfo, sizeof(cpuInfo));
	}
	return string(cpuBrandString);
#endif
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Core& Core::Get()
{
	static Core instance;
	return instance;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Core::StartCapture()
{
	pendingState = State::START_CAPTURE;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Core::StopCapture()
{
	pendingState = State::STOP_CAPTURE;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Core::CancelCapture()
{
	pendingState = State::CANCEL_CAPTURE;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Core::DumpCapture()
{
	pendingState = State::DUMP_CAPTURE;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Core::DumpProgress(const char* message)
{
	progressReportedLastTimestampMS = GetTimeMilliSeconds();

	OutputDataStream stream;
	stream << message;

	Server::Get().Send(DataResponse::ReportProgress, stream);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#if defined(OPTICK_MSVC)
#pragma warning( push )
#pragma warning( disable : 4996)
#endif
void Core::DumpProgressFormatted(const char* format, ...)
{
	va_list arglist;
	char buffer[256] = { 0 };
	va_start(arglist, format);
#ifdef OPTICK_MSVC
	vsprintf_s(buffer, format, arglist);
#else
	vsprintf(buffer, format, arglist);
#endif
	va_end(arglist);
	DumpProgress(buffer);
}
#if defined(OPTICK_MSVC)
#pragma warning( pop )
#endif
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool IsFrameDescription(const EventDescription* desc)
{
	for (int i = 0; i < FrameType::COUNT; ++i)
		if (GetFrameDescription((FrameType::Type)i) == desc)
			return true;

	return false;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool IsSleepDescription(const EventDescription* desc)
{
	return desc->color == Color::White;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool IsSleepOnlyScope(const ScopeData& scope)
{
	//if (!scope.categories.empty())
	//	return false;

	const vector<EventData>& events = scope.events;
	for (auto it = events.begin(); it != events.end(); ++it)
	{
		const EventData& data = *it;

		if (!IsSleepDescription(data.description))
		{
			return false;
		}
	}

	return true;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Core::DumpEvents(EventStorage& entry, const EventTime& timeSlice, ScopeData& scope)
{
	if (!entry.eventBuffer.IsEmpty())
	{
		const EventData* rootEvent = nullptr;
		const int64 batchLimitMs = 3;

		entry.eventBuffer.ForEach([&](const EventData& data)
		{
			if (data.finish >= data.start && data.start >= timeSlice.start && timeSlice.finish >= data.finish)
			{
				if (!rootEvent)
				{
					rootEvent = &data;
					scope.InitRootEvent(*rootEvent);
				} 
				else if (rootEvent->finish < data.finish)
				{
					// Batching together small buckets
					// Flushing if we hit the following conditions:
					// * Frame Description - don't batch frames together
					// * SleepOnly scope - we ignore them
					// * Sleep Event - flush the previous batch
					if (IsFrameDescription(rootEvent->description) || TicksToMs(scope.header.event.finish - scope.header.event.start) > batchLimitMs || IsSleepDescription(data.description) || IsSleepOnlyScope(scope))
						scope.Send();

					rootEvent = &data;
					scope.InitRootEvent(*rootEvent);
				}
				else
				{
					scope.AddEvent(data);
				}
			}
		});

		scope.Send();

		entry.eventBuffer.Clear(false);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Core::DumpTags(EventStorage& entry, ScopeData& scope)
{
	if (!entry.tagFloatBuffer.IsEmpty() ||
		!entry.tagS32Buffer.IsEmpty() ||
		!entry.tagU32Buffer.IsEmpty() ||
		!entry.tagU64Buffer.IsEmpty() ||
		!entry.tagPointBuffer.IsEmpty() ||
		!entry.tagStringBuffer.IsEmpty())
	{
		OutputDataStream tagStream;
		tagStream << scope.header.boardNumber << scope.header.threadNumber;
		tagStream  
			<< (uint32)0
			<< entry.tagFloatBuffer
			<< entry.tagU32Buffer
			<< entry.tagS32Buffer
			<< entry.tagU64Buffer
			<< entry.tagPointBuffer
			<< (uint32)0
			<< (uint32)0
			<< entry.tagStringBuffer;
		Server::Get().Send(DataResponse::TagsPack, tagStream);

		entry.ClearTags(false);
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Core::DumpThread(ThreadEntry& entry, const EventTime& timeSlice, ScopeData& scope)
{
	// We need to sort events for all the custom thread storages
	if (entry.description.threadID == INVALID_THREAD_ID)
		entry.Sort();

	// Events
	DumpProgressFormatted("Serializing %s", entry.description.name.c_str());
	DumpEvents(entry.storage, timeSlice, scope);
	DumpTags(entry.storage, scope);
	OPTICK_ASSERT(entry.storage.fiberSyncBuffer.IsEmpty(), "Fiber switch events in native threads?");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Core::DumpFiber(FiberEntry& entry, const EventTime& timeSlice, ScopeData& scope)
{
	// Events
	DumpEvents(entry.storage, timeSlice, scope);

	if (!entry.storage.fiberSyncBuffer.IsEmpty())
	{
		OutputDataStream fiberSynchronizationStream;
		fiberSynchronizationStream << scope.header.boardNumber;
		fiberSynchronizationStream << scope.header.fiberNumber;
		fiberSynchronizationStream << entry.storage.fiberSyncBuffer;
		Server::Get().Send(DataResponse::FiberSynchronizationData, fiberSynchronizationStream);
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
EventTime CalculateRange(const ThreadEntry& entry, const EventDescription* rootDescription)
{
	EventTime timeSlice = { INT64_MAX, INT64_MIN };
	entry.storage.eventBuffer.ForEach([&](const EventData& data)
	{
		if (data.description == rootDescription)
		{
			timeSlice.start = std::min(timeSlice.start, data.start);
			timeSlice.finish = std::max(timeSlice.finish, data.finish);
		}
	});
	return timeSlice;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
EventTime CalculateRange(FrameStorage& frameStorage)
{
	EventTime timeSlice = { INT64_MAX, INT64_MIN };
	frameStorage.m_Frames.ForEach([&](const FrameData& data)
	{
		timeSlice.start = std::min(timeSlice.start, data.start);
		timeSlice.finish = std::max(timeSlice.finish, data.finish);
	});
	return timeSlice;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Core::DumpFrames(uint32 mode)
{
    std::lock_guard<std::recursive_mutex> lock(threadsLock);
    
	if (frames.empty() || threads.empty())
		return;

	++boardNumber;

	Server::Get().SendStart();

	DumpProgress("Generating summary...");

	GenerateCommonSummary();
	DumpSummary();

	DumpProgress("Collecting Frame Events...");

	std::array<EventTime, FrameType::COUNT> timeSlice;
	for (int i = 0; i < FrameType::COUNT; ++i)
	{
		timeSlice[i] = CalculateRange(frames[i]);
	} 

	DumpBoard(mode, timeSlice[FrameType::CPU]);

	{
		DumpProgress("Serializing Frames");
		OutputDataStream framesStream;
		framesStream << boardNumber;
		framesStream << (uint32)frames.size();
		for (size_t i = 0; i < frames.size(); ++i)
			framesStream << frames[i].m_Frames;
		Server::Get().Send(DataResponse::FramesPack, framesStream);
	}

	ScopeData threadScope;
	threadScope.header.boardNumber = boardNumber;
	threadScope.header.fiberNumber = -1;

	if (gpuProfiler)
		gpuProfiler->Dump(mode);

	for (size_t i = 0; i < threads.size(); ++i)
	{
		threadScope.header.threadNumber = (uint32)i;

		ThreadEntry* entry = threads[i];

		EventTime range = timeSlice[FrameType::CPU];

		if ((entry->description.mask & ThreadMask::GPU) != 0 && timeSlice[FrameType::GPU].IsValid())
			range = timeSlice[FrameType::GPU];

		DumpThread(*entry, range, threadScope);
	}

	ScopeData fiberScope;
	fiberScope.header.boardNumber = (uint32)boardNumber;
	fiberScope.header.threadNumber = -1;
	for (size_t i = 0; i < fibers.size(); ++i)
	{
		fiberScope.header.fiberNumber = (uint32)i;
		DumpFiber(*fibers[i], timeSlice[FrameType::CPU], fiberScope);
	}

	for (int i = 0; i < FrameType::COUNT; ++i)
		frames[i].Clear(false);

	CleanupThreadsAndFibers();

	{
		DumpProgress("Serializing SwitchContexts");
		OutputDataStream switchContextsStream;
		switchContextsStream << boardNumber;
		switchContextCollector.Serialize(switchContextsStream);
		Server::Get().Send(DataResponse::SynchronizationData, switchContextsStream);
	}

	{
		DumpProgress("Serializing SysCalls");
		OutputDataStream callstacksStream;
		callstacksStream << boardNumber;
		syscallCollector.Serialize(callstacksStream);
		Server::Get().Send(DataResponse::SyscallPack, callstacksStream);
	}

	if (!callstackCollector.IsEmpty())
	{
		OutputDataStream symbolsStream;
		symbolsStream << boardNumber;
		DumpProgress("Serializing Modules");
		callstackCollector.SerializeModules(symbolsStream);
		callstackCollector.SerializeSymbols(symbolsStream);
		Server::Get().Send(DataResponse::CallstackDescriptionBoard, symbolsStream);

		// We can free some memory now to unlock space for callstack serialization
		DumpProgress("Deallocating memory for SymbolEngine");
		Memory::Delete(symbolEngine);
		symbolEngine = nullptr;

		DumpProgress("Serializing callstacks");
		OutputDataStream callstacksStream;
		callstacksStream << boardNumber;
		callstackCollector.SerializeCallstacks(callstacksStream);
		Server::Get().Send(DataResponse::CallstackPack, callstacksStream);
	}

	forcedMainThreadIndex = (uint32)-1;

	Server::Get().SendFinish();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Core::DumpSummary()
{
	OutputDataStream stream;

	// Board Number
	stream << boardNumber;

	// Frames
	double frequency = (double)Platform::GetFrequency();
	stream << (uint32_t)frames[FrameType::CPU].m_Frames.Size();
	for (const EventTime& frame : frames[FrameType::CPU].m_Frames)
	{
		double frameTimeMs = 1000.0 * (frame.finish - frame.start) / frequency;
		stream << (float)frameTimeMs;
	}

	// Summary
	stream << (uint32_t)summary.size();
	for (size_t i = 0; i < summary.size(); ++i)
		stream << summary[i].first << summary[i].second;
	summary.clear();

	// Attachments
	stream << (uint32_t)attachments.size();
	for (const Attachment& att : attachments)
		stream << (uint32_t)att.type << att.name << att.data;
	attachments.clear();

	// Send
	Server::Get().Send(DataResponse::SummaryPack, stream);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Core::CleanupThreadsAndFibers()
{
	std::lock_guard<std::recursive_mutex> lock(threadsLock);

	for (ThreadList::iterator it = threads.begin(); it != threads.end();)
	{
		if (!(*it)->isAlive)
		{
			Memory::Delete(*it);
			it = threads.erase(it);
		}
		else
		{
			++it;
		}
	}
}

void Core::DumpBoard(uint32 mode, EventTime timeSlice)
{
	OutputDataStream boardStream;

	boardStream << boardNumber;
	boardStream << Platform::GetFrequency();
	boardStream << (uint64)0; // Origin
	boardStream << (uint32)0; // Precision
	boardStream << timeSlice;
	boardStream << threads;
	boardStream << fibers;
	boardStream << forcedMainThreadIndex;
	boardStream << EventDescriptionBoard::Get();
	boardStream << (uint32)0; // Tags
	boardStream << (uint32)0; // Run
	boardStream << (uint32)0; // Filters
	boardStream << (uint32)0; // ThreadDescs
	boardStream << mode; // Mode
	boardStream << processDescs;
	boardStream << threadDescs;
	boardStream << (uint32)Platform::GetProcessID();
	boardStream << (uint32)std::thread::hardware_concurrency();
	Server::Get().Send(DataResponse::FrameDescriptionBoard, boardStream);

	// Cleanup
	processDescs.clear();
	threadDescs.clear();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Core::GenerateCommonSummary()
{
	AttachSummary("Platform", Platform::GetName());
	AttachSummary("CPU", GetCPUName().c_str());
	if (gpuProfiler)
		AttachSummary("GPU", gpuProfiler->GetName().c_str());
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Core::Core()
	: progressReportedLastTimestampMS(0)
	, boardNumber(0)
	, stateCallback(nullptr)
	, currentState(State::DUMP_CAPTURE)
	, pendingState(State::DUMP_CAPTURE)
	, forcedMainThreadIndex((uint32)-1)
	, currentMode(Mode::OFF)
	, previousMode(Mode::OFF)
	, symbolEngine(nullptr)
	, tracer(nullptr)
	, gpuProfiler(nullptr)
{
	frames[FrameType::CPU].m_Description = EventDescription::Create("CPU Frame", __FILE__, __LINE__);
	frames[FrameType::GPU].m_Description = EventDescription::Create("GPU Frame", __FILE__, __LINE__);
	frames[FrameType::Render].m_Description = EventDescription::Create("Render Frame", __FILE__, __LINE__);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool Core::UpdateState()
{
	if (currentState != pendingState)
	{
		State::Type nextState = pendingState;
		if (pendingState == State::DUMP_CAPTURE && currentState == State::START_CAPTURE)
			nextState = State::STOP_CAPTURE;

		if ((stateCallback != nullptr) && !stateCallback(nextState))
			return false;

		switch (nextState)
		{
		case State::START_CAPTURE:
			Activate((Mode::Type)settings.mode);
			break;

		case State::STOP_CAPTURE:
		case State::CANCEL_CAPTURE:
			Activate(Mode::OFF);
			break;

		case State::DUMP_CAPTURE:
			DumpFrames(previousMode);
			break;
		}
		currentState = nextState;
		return true;
	}
	return false;
}


void Core::Update()
{
	std::lock_guard<std::recursive_mutex> lock(coreLock);

	if (currentMode != Mode::OFF)
	{
		FrameBuffer frameBuffer = frames[FrameType::CPU].m_Frames;

		if (frameBuffer.Size() > 0)
		{
			if (settings.frameLimit > 0 && frameBuffer.Size() >= settings.frameLimit)
				DumpCapture();

			if (settings.timeLimitUs > 0)
			{
				if (TicksToUs(frameBuffer.Back()->finish - frameBuffer.Front()->start) >= settings.timeLimitUs)
					DumpCapture();
			}

			if (settings.spikeLimitUs > 0)
			{
				if (TicksToUs(frameBuffer.Back()->finish - frameBuffer.Front()->start) >= settings.spikeLimitUs)
					DumpCapture();
			}
		}

		if (IsTimeToReportProgress())
			DumpCapturingProgress();
	}

	UpdateEvents();

	while (UpdateState()) {}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t Core::BeginUpdateFrame(FrameType::Type frameType, int64_t timestamp, uint64_t threadID)
{
	std::lock_guard<std::recursive_mutex> lock(coreLock);

	if (currentMode != Mode::OFF)
	{
		FrameData& data = frames[frameType].m_Frames.Add();
		data.description = frames[frameType].m_Description;
		data.start = timestamp;
		data.finish = timestamp;
		data.threadID = threadID;
	}

	return ++frames[frameType].m_FrameNumber;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t Core::EndUpdateFrame(FrameType::Type frameType, int64_t timestamp, uint64_t /*threadID*/)
{
	std::lock_guard<std::recursive_mutex> lock(coreLock);

	if (currentMode != Mode::OFF)
	{
		if (FrameData* lastFrame = frames[frameType].m_Frames.Back())
		{
			lastFrame->finish = timestamp;
		}
	}

	return frames[frameType].m_FrameNumber;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Core::UpdateEvents()
{
	Server::Get().Update();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool Core::ReportSwitchContext(const SwitchContextDesc& desc)
{
	switchContextCollector.Add(desc);
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool Core::ReportStackWalk(const CallstackDesc& desc)
{
	callstackCollector.Add(desc);
	return true;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Core::Activate(Mode::Type mode)
{
	if (mode != currentMode)
	{
		previousMode = currentMode;
		currentMode = mode;

        {
            std::lock_guard<std::recursive_mutex> lock(threadsLock);
            for(auto it = threads.begin(); it != threads.end(); ++it)
            {
                ThreadEntry* entry = *it;
                entry->Activate(mode);
            }
        }


		if (mode != Mode::OFF)
		{
			CaptureStatus::Type status = CaptureStatus::ERR_TRACER_NOT_IMPLEMENTED;

#if OPTICK_ENABLE_TRACING
			if (mode & Mode::TRACER)
			{
				if (tracer == nullptr)
					tracer = Platform::CreateTrace();

				if (tracer)
				{
					tracer->SetPassword(settings.password.c_str());

					std::lock_guard<std::recursive_mutex> lock(threadsLock);

					status = tracer->Start(mode, settings.samplingFrequency, threads);
					
					// Let's retry with more narrow setup
					if (status != CaptureStatus::OK && (mode & Mode::AUTOSAMPLING))
						status = tracer->Start((Mode::Type)(mode & ~Mode::AUTOSAMPLING), settings.samplingFrequency, threads);
				}
			}

			if (mode & Mode::AUTOSAMPLING)
				if (symbolEngine == nullptr)
					symbolEngine = Platform::CreateSymbolEngine();
#endif

			if (gpuProfiler && (mode & Mode::GPU))
				gpuProfiler->Start(mode);

			SendHandshakeResponse(status);
		}
		else
		{
			if (tracer)
			{
				tracer->Stop();
				Memory::Delete(tracer);
				tracer = nullptr;
			}
				

			if (gpuProfiler)
				gpuProfiler->Stop(previousMode);
		}
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Core::DumpCapturingProgress()
{
	stringstream stream;

	if (currentMode != Mode::OFF)
	{
		size_t memUsedKb = Memory::GetAllocatedSize() >> 10;
		float memUsedMb = memUsedKb / 1024.0f;

		stream << "Capturing Frame " << (uint32)frames[FrameType::CPU].m_Frames.Size() << "..." << std::endl << "Memory Used: " << std::fixed << std::setprecision(3) << memUsedMb << " Mb";
	}

	DumpProgress(stream.str().c_str());
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool Core::IsTimeToReportProgress() const
{
	return GetTimeMilliSeconds() > progressReportedLastTimestampMS + 200;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Core::SendHandshakeResponse(CaptureStatus::Type status)
{
	OutputDataStream stream;
	stream << (uint32)status;
	stream << Platform::GetName();
	stream << Server::Get().GetHostName();
	Server::Get().Send(DataResponse::Handshake, stream);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool Core::IsRegistredThread(ThreadID id)
{
	std::lock_guard<std::recursive_mutex> lock(threadsLock);

	for (ThreadList::iterator it = threads.begin(); it != threads.end(); ++it)
	{
		ThreadEntry* entry = *it;
		if (entry->description.threadID == id)
		{
			return true;
		}
	}
	return false;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ThreadEntry* Core::RegisterThread(const ThreadDescription& description, EventStorage** slot)
{
	std::lock_guard<std::recursive_mutex> lock(threadsLock);

	ThreadEntry* entry = nullptr;

	auto it = std::find_if(threads.begin(), threads.end(), [&description](const ThreadEntry* entry) { return entry->description == description; });
	if (it == threads.end())
	{
		entry = Memory::New<ThreadEntry>(description, slot);
		threads.push_back(entry);
	}
	else
	{
		entry = *it;
	}

	if ((currentMode != Mode::OFF) && slot != nullptr)
		*slot = &entry->storage;

	return entry;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool Core::UnRegisterThread(ThreadID threadID, bool keepAlive)
{
	std::lock_guard<std::recursive_mutex> lock(threadsLock);

	for (ThreadList::iterator it = threads.begin(); it != threads.end(); ++it)
	{
		ThreadEntry* entry = *it;
		if (entry->description.threadID == threadID && entry->isAlive)
		{
			if ((currentMode == Mode::OFF) && !keepAlive)
			{
				Memory::Delete(entry);
				threads.erase(it);
				return true;
			}
			else
			{
				entry->isAlive = false;
				return true;
			}
		}
	}

	return false;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool Core::RegisterFiber(const FiberDescription& description, EventStorage** slot)
{
	std::lock_guard<std::recursive_mutex> lock(coreLock);
	FiberEntry* entry = Memory::New<FiberEntry>(description);
	fibers.push_back(entry);
	entry->storage.isFiberStorage = true;
	*slot = &entry->storage;
	return true;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool Core::RegisterProcessDescription(const ProcessDescription& description)
{
	processDescs.push_back(description);
	return false;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool Core::RegisterThreadDescription(const ThreadDescription& description)
{
	threadDescs.push_back(description);
	return false;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool Core::SetStateChangedCallback(StateCallback cb)
{
	stateCallback = cb;
	return stateCallback != nullptr;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool Core::AttachSummary(const char* key, const char* value)
{
	summary.push_back(make_pair(string(key), string(value)));
	return true;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool Core::AttachFile(File::Type type, const char* name, const uint8_t* data, uint32_t size)
{
	if (size > 0)
	{
		attachments.push_back(Attachment(type, name));
		Attachment& attachment = attachments.back();
		attachment.data.resize(size);
		memcpy(&attachment.data[0], data, size);
		return true;
	}
	return false;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool Core::AttachFile(File::Type type, const char* name, std::istream& stream)
{
	std::streampos beg = stream.tellg();
	stream.seekg(0, std::ios::end);
	std::streampos end = stream.tellg();
	stream.seekg(beg, std::ios::beg);

	size_t size =(size_t)(end - beg);
	void* buffer = Memory::Alloc(size);

	stream.read((char*)buffer, size);
	bool result = AttachFile(type, name, (uint8*)buffer, (uint32_t)size);

	Memory::Free(buffer);
	return result;

}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool Core::AttachFile(File::Type type, const char* name, const char* path)
{
    std::ifstream stream(path, std::ios::binary);
	return AttachFile(type, name, stream);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool Core::AttachFile(File::Type type, const char* name, const wchar_t* path)
{
#if defined(OPTICK_MSVC)
	std::ifstream stream(path, std::ios::binary);
	return AttachFile(type, name, stream);
#else
	char p[256] = { 0 };
	wcstombs(p, path, sizeof(p));
    std::ifstream stream(p, std::ios::binary);
	return AttachFile(type, name, stream);
#endif
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Core::InitGPUProfiler(GPUProfiler* profiler)
{
	OPTICK_ASSERT(gpuProfiler == nullptr, "Can't reinitialize GPU profiler! Not supported yet!");
	gpuProfiler = profiler;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool Core::SetSettings(const CaptureSettings& captureSettings)
{
	settings = captureSettings;

	//if (tracer)
	//{
	//	string decoded = base64_decode(encodedPassword);
	//	tracer->SetPassword(decoded.c_str());
	//	return true;
	//}
	return false;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Core::SetMainThreadID(uint64_t threadID)
{
	std::lock_guard<std::recursive_mutex> lock(threadsLock);

	if (threadID == INVALID_THREAD_ID)
	{
		forcedMainThreadIndex = (uint32)-1;
	}
	else
	{
		for (size_t i = 0; i < threads.size(); ++i)
		{
			ThreadEntry* entry = threads[i];
			if (entry->description.threadID == threadID)
			{
				forcedMainThreadIndex = (uint32)i;
			}
		}
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const EventDescription* Core::GetFrameDescription(FrameType::Type frame) const
{
	return frames[frame].m_Description;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Core::Shutdown()
{
	std::lock_guard<std::recursive_mutex> lock(threadsLock);

	Memory::Delete<GPUProfiler>(gpuProfiler);
	gpuProfiler = nullptr;

	for (ThreadList::iterator it = threads.begin(); it != threads.end(); ++it)
	{
		Memory::Delete(*it);
	}
	threads.clear();

	for (FiberList::iterator it = fibers.begin(); it != fibers.end(); ++it)
	{
		Memory::Delete(*it);
	}
	fibers.clear();

	Memory::Delete(symbolEngine);
	symbolEngine = nullptr;

	EventDescriptionBoard::Get().Shutdown();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Core::~Core()
{
	Shutdown();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const vector<ThreadEntry*>& Core::GetThreads() const
{
	return threads;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OPTICK_THREAD_LOCAL EventStorage* Core::storage = nullptr;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ScopeHeader::ScopeHeader() : boardNumber(0), threadNumber(0), fiberNumber(0), type(FrameType::NONE)
{
	event.start = EventTime::INVALID_TIMESTAMP;
	event.finish = EventTime::INVALID_TIMESTAMP;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OutputDataStream& operator<<(OutputDataStream& stream, const ScopeHeader& header)
{
	return stream << header.boardNumber << header.threadNumber << header.fiberNumber << header.event << header.type;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OutputDataStream& operator<<(OutputDataStream& stream, const ScopeData& ob)
{
	return stream << ob.header << ob.categories << ob.events;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OutputDataStream& operator<<(OutputDataStream& stream, const ThreadDescription& description)
{
	return stream << description.threadID << description.processID << description.name << description.maxDepth << description.priority << description.mask;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OutputDataStream& operator<<(OutputDataStream& stream, const ThreadEntry* entry)
{
	return stream << entry->description;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OutputDataStream& operator<<(OutputDataStream& stream, const FiberDescription& description)
{
	return stream << description.id;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OutputDataStream& operator<<(OutputDataStream& stream, const FiberEntry* entry)
{
	return stream << entry->description;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OutputDataStream& operator<<(OutputDataStream& stream, const ProcessDescription& description)
{
	return stream << description.processID << description.name << description.uniqueKey;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OPTICK_API bool SetStateChangedCallback(StateCallback cb)
{
	return Core::Get().SetStateChangedCallback(cb);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OPTICK_API bool AttachSummary(const char* key, const char* value)
{
	return Core::Get().AttachSummary(key, value);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OPTICK_API bool AttachFile(File::Type type, const char* name, const uint8_t* data, uint32_t size)
{
	return Core::Get().AttachFile(type, name, data, size);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OPTICK_API bool AttachFile(File::Type type, const char* name, const char* path)
{
	return Core::Get().AttachFile(type, name, path);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OPTICK_API bool AttachFile(File::Type type, const char* name, const wchar_t* path)
{
	return Core::Get().AttachFile(type, name, path);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OutputDataStream& operator<<(OutputDataStream& stream, const Point& ob)
{
	return stream << ob.x << ob.y << ob.z;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OPTICK_API void Update()
{
	return Core::Get().Update();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OPTICK_API uint32_t BeginFrame(Optick::FrameType::Type frameType, int64_t timestamp, uint64_t threadID)
{
	return Core::BeginFrame(frameType, timestamp != EventTime::INVALID_TIMESTAMP ? timestamp : Optick::GetHighPrecisionTime(), threadID != INVALID_THREAD_ID ? threadID : Platform::GetThreadID());
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OPTICK_API uint32_t EndFrame(Optick::FrameType::Type frameType, int64_t timestamp, uint64_t threadID)
{
	return Core::EndFrame(frameType, timestamp != EventTime::INVALID_TIMESTAMP ? timestamp : Optick::GetHighPrecisionTime(), threadID != INVALID_THREAD_ID ? threadID : Platform::GetThreadID());
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OPTICK_API bool IsActive(Mode::Type mode /*= Mode::INSTRUMENTATION_EVENTS*/)
{
	return (Core::Get().currentMode & mode) != 0;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OPTICK_API EventStorage** GetEventStorageSlotForCurrentThread()
{
	return &Core::Get().storage;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OPTICK_API bool IsFiberStorage(EventStorage* fiberStorage)
{
	return fiberStorage->isFiberStorage;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OPTICK_API bool RegisterThread(const char* name)
{
	return Core::Get().RegisterThread(ThreadDescription(name, Platform::GetThreadID(), Platform::GetProcessID()), &Core::storage) != nullptr;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OPTICK_API bool RegisterThread(const wchar_t* name)
{
	const int THREAD_NAME_LENGTH = 128;
	char mbName[THREAD_NAME_LENGTH];
	wcstombs_s(mbName, name, THREAD_NAME_LENGTH);

	return Core::Get().RegisterThread(ThreadDescription(mbName, Platform::GetThreadID(), Platform::GetProcessID()), &Core::storage) != nullptr;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OPTICK_API bool UnRegisterThread(bool keepAlive)
{
	return Core::Get().UnRegisterThread(Platform::GetThreadID(), keepAlive);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OPTICK_API bool RegisterFiber(uint64 fiberId, EventStorage** slot)
{
	return Core::Get().RegisterFiber(FiberDescription(fiberId), slot);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OPTICK_API EventStorage* RegisterStorage(const char* name, uint64_t threadID, ThreadMask::Type type)
{
	ThreadEntry* entry = Core::Get().RegisterThread(ThreadDescription(name, threadID, Platform::GetProcessID(), 1, 0, type), nullptr);
	return entry ? &entry->storage : nullptr;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OPTICK_API void GpuFlip(void* swapChain)
{
	if (GPUProfiler* gpuProfiler = Core::Get().gpuProfiler)
		gpuProfiler->Flip(swapChain);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OPTICK_API GPUContext SetGpuContext(GPUContext context)
{
	if (EventStorage* storage = Core::storage)
	{
		GPUContext prevContext = storage->gpuStorage.context;
		storage->gpuStorage.context = context;
		return prevContext;
	}
	return GPUContext();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OPTICK_API const EventDescription* GetFrameDescription(FrameType::Type frame)
{
	return Core::Get().GetFrameDescription(frame);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OPTICK_API void SetAllocator(AllocateFn allocateFn, DeallocateFn deallocateFn, InitThreadCb initThreadCb)
{
	Memory::SetAllocator(allocateFn, deallocateFn, initThreadCb);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OPTICK_API bool StartCapture(Mode::Type mode /*= Mode::DEFAULT*/, int samplingFrequency /*= 1000*/, bool force /*= true*/)
{
	if (IsActive())
		return false;

	CaptureSettings settings;
	settings.mode = mode | Mode::NOGUI;
	settings.samplingFrequency = samplingFrequency;

	Core& core = Core::Get();
	core.SetSettings(settings);

	if (!core.IsRegistredThread(Platform::GetThreadID()))
		RegisterThread("MainThread");

	core.StartCapture();

	if (force)
	{
		core.Update();
		core.SetMainThreadID(Platform::GetThreadID());
		core.BeginFrame(FrameType::CPU, GetHighPrecisionTime(), Platform::GetThreadID());
	}
	
	return true;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OPTICK_API bool StopCapture(bool force /*= true*/)
{
	if (!IsActive())
		return false;

	Core& core = Core::Get();
	core.StopCapture();

	if (force)
	{
		core.EndFrame(FrameType::CPU, GetHighPrecisionTime(), Platform::GetThreadID());
		core.Update();
	}

	return true;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct SaveHelper
{
	static void Init(const char* path)
	{
		GetOutputFile().open(path, std::ios::out | std::ios::binary);
	}

	static void Write(const char* data, size_t size)
	{
		if (data)
			GetOutputFile().write(data, size);
		else
			GetOutputFile().close();
	}

	static fstream& GetOutputFile()
	{
		static fstream file;
		return file;
	}
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool EndsWith(const char* str, const char* substr)
{
	size_t strLength = strlen(str);
	size_t substrLength = strlen(substr);
	return strLength >= substrLength && strcmp(substr, &str[strLength - substrLength]) == 0;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OPTICK_API bool SaveCapture(const char* path, bool force /*= true*/)
{
	char filePath[512] = { 0 };
#if defined(OPTICK_MSVC)
	strcpy_s(filePath, 512, path);
#else
	strcpy(filePath, path);
#endif
	
	if (path == nullptr || !EndsWith(path, ".opt"))
	{
		time_t now = time(0);
		struct tm tstruct;
#if defined(OPTICK_MSVC)
		localtime_s(&tstruct, &now);
#else
		localtime_r(&now, &tstruct);
#endif
		char timeStr[80] = { 0 };
		strftime(timeStr, sizeof(timeStr), "(%Y-%m-%d.%H-%M-%S).opt", &tstruct);
#if defined(OPTICK_MSVC)
		strcat_s(filePath, 512, timeStr);
#else
		strcat(filePath, timeStr);
#endif
	}

	SaveHelper::Init(filePath);
	return SaveCapture(SaveHelper::Write, force);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OPTICK_API bool SaveCapture(CaptureSaveChunkCb dataCb /*= nullptr*/, bool force /*= true*/)
{
	Server::Get().SetSaveCallback(dataCb);

	Core& core = Core::Get();
	core.DumpCapture();
	if (force)
		core.Update();

	return true;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OPTICK_API void Shutdown()
{
	Core::Get().Shutdown();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
EventStorage::EventStorage(): currentMode(Mode::OFF), pushPopEventStackIndex(0), isFiberStorage(false)
{
	 
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ThreadEntry::Activate(Mode::Type mode)
{
	if (!isAlive)
		return;

	if (mode != Mode::OFF)
		storage.Clear(true);

	if (threadTLS != nullptr)
	{
		storage.currentMode = mode;
		*threadTLS = mode != Mode::OFF ? &storage : nullptr;
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ThreadEntry::Sort()
{
	SortMemoryPool(storage.eventBuffer);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ScopeData::Send()
{
	if (!events.empty() || !categories.empty())
	{
		if (!IsSleepOnlyScope(*this))
		{
			OutputDataStream frameStream;
			frameStream << *this;
			Server::Get().Send(DataResponse::EventFrame, frameStream);
		}
	}

	Clear();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ScopeData::ResetHeader()
{
	header.event.start = INT64_MAX;
	header.event.finish = INT64_MIN;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ScopeData::Clear()
{
	ResetHeader();
	events.clear();
	categories.clear();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void EventStorage::GPUStorage::Clear(bool preserveMemory)
{
	for (size_t i = 0; i < gpuBuffer.size(); ++i)
		for (int j = 0; j < GPU_QUEUE_COUNT; ++j)
			gpuBuffer[i][j].Clear(preserveMemory);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
EventData* EventStorage::GPUStorage::Start(const EventDescription &desc)
{
	if (GPUProfiler* gpuProfiler = Core::Get().gpuProfiler)
	{
		EventData& result = gpuBuffer[context.node][context.queue].Add();
		result.description = &desc;
		result.start = EventTime::INVALID_TIMESTAMP;
		result.finish = EventTime::INVALID_TIMESTAMP;
		gpuProfiler->QueryTimestamp(context.cmdBuffer, &result.start);
		return &result;
	}
	return nullptr;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void EventStorage::GPUStorage::Stop(EventData& data)
{
	if (GPUProfiler* gpuProfiler = Core::Get().gpuProfiler)
	{
		gpuProfiler->QueryTimestamp(context.cmdBuffer, &data.finish);
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}

#endif //USE_OPTICK
