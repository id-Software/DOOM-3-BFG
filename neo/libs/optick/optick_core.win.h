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
#if defined(_MSC_VER)

#include "optick.config.h"

#if USE_OPTICK

#include "optick_core.platform.h"

namespace Optick
{
	const char* Platform::GetName()
	{
	#if OPTICK_PC
		return "Windows";
	#else
		return "XBox";
	#endif
	}

	ThreadID Platform::GetThreadID()
	{
		return GetCurrentThreadId();
	}

	ProcessID Platform::GetProcessID()
	{
		return GetCurrentProcessId();
	}

	int64 Platform::GetFrequency()
	{
		LARGE_INTEGER frequency;
		QueryPerformanceFrequency(&frequency);
		return frequency.QuadPart;
	}

	int64 Platform::GetTime()
	{
		LARGE_INTEGER largeInteger;
		QueryPerformanceCounter(&largeInteger);
		return largeInteger.QuadPart;
	}
}

#if OPTICK_ENABLE_TRACING
#include <psapi.h>
#include "optick_core.h"

/*
Event Tracing Functions - API
https://msdn.microsoft.com/en-us/library/windows/desktop/aa363795(v=vs.85).aspx
*/

#define DECLARE_ETW (!OPTICK_PC)

#if DECLARE_ETW
// Copied from Windows SDK
#ifndef WMIAPI
#ifndef MIDL_PASS
#ifdef _WMI_SOURCE_
#define WMIAPI __stdcall
#else
#define WMIAPI DECLSPEC_IMPORT __stdcall
#endif // _WMI_SOURCE
#endif // MIDL_PASS
#endif // WMIAPI
#define INITGUID
#include <guiddef.h>
#if defined(_NTDDK_) || defined(_NTIFS_) || defined(_WMIKM_)
#define _EVNTRACE_KERNEL_MODE
#endif
#if !defined(_EVNTRACE_KERNEL_MODE)
#include <wmistr.h>
#endif

#if _MSC_VER <= 1600
#define EVENT_DESCRIPTOR_DEF
#define EVENT_HEADER_DEF
#define EVENT_HEADER_EXTENDED_DATA_ITEM_DEF
#define EVENT_RECORD_DEF
#endif

#ifndef _TRACEHANDLE_DEFINED
#define _TRACEHANDLE_DEFINED
typedef ULONG64 TRACEHANDLE, *PTRACEHANDLE;
#endif

//
// EventTraceGuid is used to identify a event tracing session
//
DEFINE_GUID( /* 68fdd900-4a3e-11d1-84f4-0000f80464e3 */
	EventTraceGuid,
	0x68fdd900,
	0x4a3e,
	0x11d1,
	0x84, 0xf4, 0x00, 0x00, 0xf8, 0x04, 0x64, 0xe3
);

//
// SystemTraceControlGuid. Used to specify event tracing for kernel
//
DEFINE_GUID( /* 9e814aad-3204-11d2-9a82-006008a86939 */
	SystemTraceControlGuid,
	0x9e814aad,
	0x3204,
	0x11d2,
	0x9a, 0x82, 0x00, 0x60, 0x08, 0xa8, 0x69, 0x39
);

//
// EventTraceConfigGuid. Used to report system configuration records
//
DEFINE_GUID( /* 01853a65-418f-4f36-aefc-dc0f1d2fd235 */
	EventTraceConfigGuid,
	0x01853a65,
	0x418f,
	0x4f36,
	0xae, 0xfc, 0xdc, 0x0f, 0x1d, 0x2f, 0xd2, 0x35
);

//
// DefaultTraceSecurityGuid. Specifies the default event tracing security
//
DEFINE_GUID( /* 0811c1af-7a07-4a06-82ed-869455cdf713 */
	DefaultTraceSecurityGuid,
	0x0811c1af,
	0x7a07,
	0x4a06,
	0x82, 0xed, 0x86, 0x94, 0x55, 0xcd, 0xf7, 0x13
);


///////////////////////////////////////////////////////////////////////////////
#define PROCESS_TRACE_MODE_REAL_TIME                0x00000100
#define PROCESS_TRACE_MODE_RAW_TIMESTAMP            0x00001000
#define PROCESS_TRACE_MODE_EVENT_RECORD             0x10000000
///////////////////////////////////////////////////////////////////////////////
#define EVENT_HEADER_FLAG_EXTENDED_INFO				0x0001
#define EVENT_HEADER_FLAG_PRIVATE_SESSION			0x0002
#define EVENT_HEADER_FLAG_STRING_ONLY				0x0004
#define EVENT_HEADER_FLAG_TRACE_MESSAGE				0x0008
#define EVENT_HEADER_FLAG_NO_CPUTIME				0x0010
#define EVENT_HEADER_FLAG_32_BIT_HEADER				0x0020
#define EVENT_HEADER_FLAG_64_BIT_HEADER				0x0040
#define EVENT_HEADER_FLAG_CLASSIC_HEADER			0x0100
#define EVENT_HEADER_FLAG_PROCESSOR_INDEX			0x0200
///////////////////////////////////////////////////////////////////////////////
#define KERNEL_LOGGER_NAMEW							L"NT Kernel Logger"
///////////////////////////////////////////////////////////////////////////////
#define EVENT_TRACE_REAL_TIME_MODE          0x00000100  // Real time mode on
///////////////////////////////////////////////////////////////////////////////
#define EVENT_TRACE_CONTROL_STOP            1
///////////////////////////////////////////////////////////////////////////////

//
// Enable flags for Kernel Events
//
#define EVENT_TRACE_FLAG_PROCESS            0x00000001  // process start & end
#define EVENT_TRACE_FLAG_THREAD             0x00000002  // thread start & end
#define EVENT_TRACE_FLAG_IMAGE_LOAD         0x00000004  // image load

#define EVENT_TRACE_FLAG_DISK_IO            0x00000100  // physical disk IO
#define EVENT_TRACE_FLAG_DISK_FILE_IO       0x00000200  // requires disk IO

#define EVENT_TRACE_FLAG_MEMORY_PAGE_FAULTS 0x00001000  // all page faults
#define EVENT_TRACE_FLAG_MEMORY_HARD_FAULTS 0x00002000  // hard faults only

#define EVENT_TRACE_FLAG_NETWORK_TCPIP      0x00010000  // tcpip send & receive

#define EVENT_TRACE_FLAG_REGISTRY           0x00020000  // registry calls
#define EVENT_TRACE_FLAG_DBGPRINT           0x00040000  // DbgPrint(ex) Calls

//
// Enable flags for Kernel Events on Vista and above
//
#define EVENT_TRACE_FLAG_PROCESS_COUNTERS   0x00000008  // process perf counters
#define EVENT_TRACE_FLAG_CSWITCH            0x00000010  // context switches
#define EVENT_TRACE_FLAG_DPC                0x00000020  // deffered procedure calls
#define EVENT_TRACE_FLAG_INTERRUPT          0x00000040  // interrupts
#define EVENT_TRACE_FLAG_SYSTEMCALL         0x00000080  // system calls

#define EVENT_TRACE_FLAG_DISK_IO_INIT       0x00000400  // physical disk IO initiation
#define EVENT_TRACE_FLAG_ALPC               0x00100000  // ALPC traces
#define EVENT_TRACE_FLAG_SPLIT_IO           0x00200000  // split io traces (VolumeManager)

#define EVENT_TRACE_FLAG_DRIVER             0x00800000  // driver delays
#define EVENT_TRACE_FLAG_PROFILE            0x01000000  // sample based profiling
#define EVENT_TRACE_FLAG_FILE_IO            0x02000000  // file IO
#define EVENT_TRACE_FLAG_FILE_IO_INIT       0x04000000  // file IO initiation

#define EVENT_TRACE_FLAG_PMC_PROFILE		0x80000000	// sample based profiling (PMC) - NOT CONFIRMED!

//
// Enable flags for Kernel Events on Win7 and above
//
#define EVENT_TRACE_FLAG_DISPATCHER         0x00000800  // scheduler (ReadyThread)
#define EVENT_TRACE_FLAG_VIRTUAL_ALLOC      0x00004000  // VM operations

//
// Enable flags for Kernel Events on Win8 and above
//
#define EVENT_TRACE_FLAG_VAMAP              0x00008000  // map/unmap (excluding images)
#define EVENT_TRACE_FLAG_NO_SYSCONFIG       0x10000000  // Do not do sys config rundown

///////////////////////////////////////////////////////////////////////////////

#pragma warning(push)
#pragma warning (disable:4201) 

#ifndef EVENT_DESCRIPTOR_DEF
#define EVENT_DESCRIPTOR_DEF
typedef struct _EVENT_DESCRIPTOR {

	USHORT      Id;
	UCHAR       Version;
	UCHAR       Channel;
	UCHAR       Level;
	UCHAR       Opcode;
	USHORT      Task;
	ULONGLONG   Keyword;

} EVENT_DESCRIPTOR, *PEVENT_DESCRIPTOR;
typedef const EVENT_DESCRIPTOR *PCEVENT_DESCRIPTOR;
#endif
///////////////////////////////////////////////////////////////////////////////
#ifndef EVENT_HEADER_DEF
#define EVENT_HEADER_DEF
typedef struct _EVENT_HEADER {

	USHORT              Size;
	USHORT              HeaderType;
	USHORT              Flags;
	USHORT              EventProperty;
	ULONG               ThreadId;
	ULONG               ProcessId;
	LARGE_INTEGER       TimeStamp;
	GUID                ProviderId;
	EVENT_DESCRIPTOR    EventDescriptor;
	union {
		struct {
			ULONG       KernelTime;
			ULONG       UserTime;
		} DUMMYSTRUCTNAME;
		ULONG64         ProcessorTime;

	} DUMMYUNIONNAME;
	GUID                ActivityId;

} EVENT_HEADER, *PEVENT_HEADER;
#endif
///////////////////////////////////////////////////////////////////////////////
#ifndef EVENT_HEADER_EXTENDED_DATA_ITEM_DEF
#define EVENT_HEADER_EXTENDED_DATA_ITEM_DEF
typedef struct _EVENT_HEADER_EXTENDED_DATA_ITEM {

	USHORT      Reserved1;                      // Reserved for internal use
	USHORT      ExtType;                        // Extended info type 
	struct {
		USHORT  Linkage : 1;       // Indicates additional extended 
								   // data item
		USHORT  Reserved2 : 15;
	};
	USHORT      DataSize;                       // Size of extended info data
	ULONGLONG   DataPtr;                        // Pointer to extended info data

} EVENT_HEADER_EXTENDED_DATA_ITEM, *PEVENT_HEADER_EXTENDED_DATA_ITEM;
#endif
///////////////////////////////////////////////////////////////////////////////
#ifndef ETW_BUFFER_CONTEXT_DEF
#define ETW_BUFFER_CONTEXT_DEF
typedef struct _ETW_BUFFER_CONTEXT {
	union {
		struct {
			UCHAR ProcessorNumber;
			UCHAR Alignment;
		} DUMMYSTRUCTNAME;
		USHORT ProcessorIndex;
	} DUMMYUNIONNAME;
	USHORT  LoggerId;
} ETW_BUFFER_CONTEXT, *PETW_BUFFER_CONTEXT;
#endif
///////////////////////////////////////////////////////////////////////////////
#ifndef EVENT_RECORD_DEF
#define EVENT_RECORD_DEF
typedef struct _EVENT_RECORD {
	EVENT_HEADER        EventHeader;
	ETW_BUFFER_CONTEXT  BufferContext;
	USHORT              ExtendedDataCount;

	USHORT              UserDataLength;
	PEVENT_HEADER_EXTENDED_DATA_ITEM ExtendedData;
	PVOID               UserData;
	PVOID               UserContext;
} EVENT_RECORD, *PEVENT_RECORD;
#endif
///////////////////////////////////////////////////////////////////////////////
typedef struct _EVENT_TRACE_PROPERTIES {
	WNODE_HEADER Wnode;
	//
	// data provided by caller
	ULONG BufferSize;                   // buffer size for logging (kbytes)
	ULONG MinimumBuffers;               // minimum to preallocate
	ULONG MaximumBuffers;               // maximum buffers allowed
	ULONG MaximumFileSize;              // maximum logfile size (in MBytes)
	ULONG LogFileMode;                  // sequential, circular
	ULONG FlushTimer;                   // buffer flush timer, in seconds
	ULONG EnableFlags;                  // trace enable flags
	union {
		LONG  AgeLimit;                 // unused
		LONG  FlushThreshold;           // Number of buffers to fill before flushing
	} DUMMYUNIONNAME;

	// data returned to caller
	ULONG NumberOfBuffers;              // no of buffers in use
	ULONG FreeBuffers;                  // no of buffers free
	ULONG EventsLost;                   // event records lost
	ULONG BuffersWritten;               // no of buffers written to file
	ULONG LogBuffersLost;               // no of logfile write failures
	ULONG RealTimeBuffersLost;          // no of rt delivery failures
	HANDLE LoggerThreadId;              // thread id of Logger
	ULONG LogFileNameOffset;            // Offset to LogFileName
	ULONG LoggerNameOffset;             // Offset to LoggerName
} EVENT_TRACE_PROPERTIES, *PEVENT_TRACE_PROPERTIES;

typedef struct _EVENT_TRACE_HEADER {        // overlays WNODE_HEADER
	USHORT          Size;                   // Size of entire record
	union {
		USHORT      FieldTypeFlags;         // Indicates valid fields
		struct {
			UCHAR   HeaderType;             // Header type - internal use only
			UCHAR   MarkerFlags;            // Marker - internal use only
		} DUMMYSTRUCTNAME;
	} DUMMYUNIONNAME;
	union {
		ULONG       Version;
		struct {
			UCHAR   Type;                   // event type
			UCHAR   Level;                  // trace instrumentation level
			USHORT  Version;                // version of trace record
		} Class;
	} DUMMYUNIONNAME2;
	ULONG           ThreadId;               // Thread Id
	ULONG           ProcessId;              // Process Id
	LARGE_INTEGER   TimeStamp;              // time when event happens
	union {
		GUID        Guid;                   // Guid that identifies event
		ULONGLONG   GuidPtr;                // use with WNODE_FLAG_USE_GUID_PTR
	} DUMMYUNIONNAME3;
	union {
		struct {
			ULONG   KernelTime;             // Kernel Mode CPU ticks
			ULONG   UserTime;               // User mode CPU ticks
		} DUMMYSTRUCTNAME;
		ULONG64     ProcessorTime;          // Processor Clock
		struct {
			ULONG   ClientContext;          // Reserved
			ULONG   Flags;                  // Event Flags
		} DUMMYSTRUCTNAME2;
	} DUMMYUNIONNAME4;
} EVENT_TRACE_HEADER, *PEVENT_TRACE_HEADER;

typedef struct _EVENT_TRACE {
	EVENT_TRACE_HEADER      Header;             // Event trace header
	ULONG                   InstanceId;         // Instance Id of this event
	ULONG                   ParentInstanceId;   // Parent Instance Id.
	GUID                    ParentGuid;         // Parent Guid;
	PVOID                   MofData;            // Pointer to Variable Data
	ULONG                   MofLength;          // Variable Datablock Length
	union {
		ULONG               ClientContext;
		ETW_BUFFER_CONTEXT  BufferContext;
	} DUMMYUNIONNAME;
} EVENT_TRACE, *PEVENT_TRACE;

typedef struct _TRACE_LOGFILE_HEADER {
	ULONG           BufferSize;         // Logger buffer size in Kbytes
	union {
		ULONG       Version;            // Logger version
		struct {
			UCHAR   MajorVersion;
			UCHAR   MinorVersion;
			UCHAR   SubVersion;
			UCHAR   SubMinorVersion;
		} VersionDetail;
	} DUMMYUNIONNAME;
	ULONG           ProviderVersion;    // defaults to NT version
	ULONG           NumberOfProcessors; // Number of Processors
	LARGE_INTEGER   EndTime;            // Time when logger stops
	ULONG           TimerResolution;    // assumes timer is constant!!!
	ULONG           MaximumFileSize;    // Maximum in Mbytes
	ULONG           LogFileMode;        // specify logfile mode
	ULONG           BuffersWritten;     // used to file start of Circular File
	union {
		GUID LogInstanceGuid;           // For RealTime Buffer Delivery
		struct {
			ULONG   StartBuffers;       // Count of buffers written at start.
			ULONG   PointerSize;        // Size of pointer type in bits
			ULONG   EventsLost;         // Events losts during log session
			ULONG   CpuSpeedInMHz;      // Cpu Speed in MHz
		} DUMMYSTRUCTNAME;
	} DUMMYUNIONNAME2;
#if defined(_WMIKM_)
	PWCHAR          LoggerName;
	PWCHAR          LogFileName;
	RTL_TIME_ZONE_INFORMATION TimeZone;
#else
	LPWSTR          LoggerName;
	LPWSTR          LogFileName;
	TIME_ZONE_INFORMATION TimeZone;
#endif
	LARGE_INTEGER   BootTime;
	LARGE_INTEGER   PerfFreq;           // Reserved
	LARGE_INTEGER   StartTime;          // Reserved
	ULONG           ReservedFlags;      // ClockType
	ULONG           BuffersLost;
} TRACE_LOGFILE_HEADER, *PTRACE_LOGFILE_HEADER;

typedef enum _TRACE_QUERY_INFO_CLASS {
	TraceGuidQueryList,
	TraceGuidQueryInfo,
	TraceGuidQueryProcess,
	TraceStackTracingInfo,   // Win7
	TraceSystemTraceEnableFlagsInfo,
	TraceSampledProfileIntervalInfo,
	TraceProfileSourceConfigInfo,
	TraceProfileSourceListInfo,
	TracePmcEventListInfo,
	TracePmcCounterListInfo,
	MaxTraceSetInfoClass
} TRACE_QUERY_INFO_CLASS, TRACE_INFO_CLASS;

typedef struct _CLASSIC_EVENT_ID {
	GUID  EventGuid;
	UCHAR Type;
	UCHAR Reserved[7];
} CLASSIC_EVENT_ID, *PCLASSIC_EVENT_ID;

typedef struct _TRACE_PROFILE_INTERVAL {
	ULONG Source;
	ULONG Interval;
} TRACE_PROFILE_INTERVAL, *PTRACE_PROFILE_INTERVAL;

typedef struct _EVENT_TRACE_LOGFILEW
EVENT_TRACE_LOGFILEW, *PEVENT_TRACE_LOGFILEW;

typedef ULONG(WINAPI * PEVENT_TRACE_BUFFER_CALLBACKW)
(PEVENT_TRACE_LOGFILEW Logfile);

typedef VOID(WINAPI *PEVENT_CALLBACK)(PEVENT_TRACE pEvent);

typedef struct _EVENT_RECORD
EVENT_RECORD, *PEVENT_RECORD;

typedef VOID(WINAPI *PEVENT_RECORD_CALLBACK) (PEVENT_RECORD EventRecord);

struct _EVENT_TRACE_LOGFILEW {
	LPWSTR                  LogFileName;      // Logfile Name
	LPWSTR                  LoggerName;       // LoggerName
	LONGLONG                CurrentTime;      // timestamp of last event
	ULONG                   BuffersRead;      // buffers read to date
	union {
		// Mode of the logfile
		ULONG               LogFileMode;
		// Processing flags used on Vista and above
		ULONG               ProcessTraceMode;
	} DUMMYUNIONNAME;
	EVENT_TRACE             CurrentEvent;     // Current Event from this stream.
	TRACE_LOGFILE_HEADER    LogfileHeader;    // logfile header structure
	PEVENT_TRACE_BUFFER_CALLBACKW             // callback before each buffer
		BufferCallback;   // is read
						  //
						  // following variables are filled for BufferCallback.
						  //
	ULONG                   BufferSize;
	ULONG                   Filled;
	ULONG                   EventsLost;
	//
	// following needs to be propaged to each buffer
	//
	union {
		// Callback with EVENT_TRACE
		PEVENT_CALLBACK         EventCallback;
		// Callback with EVENT_RECORD on Vista and above
		PEVENT_RECORD_CALLBACK  EventRecordCallback;
	} DUMMYUNIONNAME2;

	ULONG                   IsKernelTrace;    // TRUE for kernel logfile

	PVOID                   Context;          // reserved for internal use
};

#pragma warning(pop)

#define PEVENT_TRACE_BUFFER_CALLBACK    PEVENT_TRACE_BUFFER_CALLBACKW
#define EVENT_TRACE_LOGFILE             EVENT_TRACE_LOGFILEW
#define PEVENT_TRACE_LOGFILE            PEVENT_TRACE_LOGFILEW
#define KERNEL_LOGGER_NAME              KERNEL_LOGGER_NAMEW
#define GLOBAL_LOGGER_NAME              GLOBAL_LOGGER_NAMEW
#define EVENT_LOGGER_NAME               EVENT_LOGGER_NAMEW

EXTERN_C
ULONG
WMIAPI
ProcessTrace(
	_In_reads_(HandleCount) PTRACEHANDLE HandleArray,
	_In_ ULONG HandleCount,
	_In_opt_ LPFILETIME StartTime,
	_In_opt_ LPFILETIME EndTime
);

EXTERN_C
ULONG
WMIAPI
StartTraceW(
	_Out_ PTRACEHANDLE TraceHandle,
	_In_ LPCWSTR InstanceName,
	_Inout_ PEVENT_TRACE_PROPERTIES Properties
);

EXTERN_C
ULONG
WMIAPI
ControlTraceW(
	_In_ TRACEHANDLE TraceHandle,
	_In_opt_ LPCWSTR InstanceName,
	_Inout_ PEVENT_TRACE_PROPERTIES Properties,
	_In_ ULONG ControlCode
);

EXTERN_C
TRACEHANDLE
WMIAPI
OpenTraceW(
	_Inout_ PEVENT_TRACE_LOGFILEW Logfile
);

EXTERN_C
ULONG
WMIAPI
CloseTrace(
	_In_ TRACEHANDLE TraceHandle
);

EXTERN_C
ULONG
WMIAPI
TraceSetInformation(
	_In_ TRACEHANDLE SessionHandle,
	_In_ TRACE_INFO_CLASS InformationClass,
	_In_reads_bytes_(InformationLength) PVOID TraceInformation,
	_In_ ULONG InformationLength
);

EXTERN_C
ULONG
WMIAPI
TraceQueryInformation(
	_In_ TRACEHANDLE SessionHandle,
	_In_ TRACE_INFO_CLASS InformationClass,
	_Out_writes_bytes_(InformationLength) PVOID TraceInformation,
	_In_ ULONG InformationLength,
	_Out_opt_ PULONG ReturnLength
);

//////////////////////////////////////////////////////////////////////////
#define RegisterTraceGuids      RegisterTraceGuidsW
#define StartTrace              StartTraceW
#define ControlTrace            ControlTraceW
#define StopTrace               StopTraceW
#define QueryTrace              QueryTraceW
#define UpdateTrace             UpdateTraceW
#define FlushTrace              FlushTraceW
#define QueryAllTraces          QueryAllTracesW
#define OpenTrace               OpenTraceW
//////////////////////////////////////////////////////////////////////////
#else
#define INITGUID  // Causes definition of SystemTraceControlGuid in evntrace.h.
#include <wmistr.h>
#include <evntrace.h>
#include <strsafe.h>
#include <evntcons.h>
#endif //DECLARE_ETW

namespace Optick
{
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const int MAX_CPU_CORES = 256;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct ETWRuntime
{
	array<ThreadID, MAX_CPU_CORES> activeCores;
	vector<std::pair<uint8_t, SysCallData*>> activeSyscalls;
	unordered_set<uint64> activeThreadsIDs;
	ProcessID currentProcessId;

	ETWRuntime()
	{
		Reset();
	}

	void Reset()
	{
		currentProcessId = INVALID_PROCESS_ID;
		activeCores.fill(INVALID_THREAD_ID);
		activeSyscalls.resize(0);
		activeThreadsIDs.clear();
	}
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ETW : public Trace
{
	static const int ETW_BUFFER_SIZE = 1024 << 10; // 1Mb
	static const int ETW_BUFFER_COUNT = 32;
	static const int ETW_MAXIMUM_SESSION_NAME = 1024;

	EVENT_TRACE_PROPERTIES *traceProperties;
	EVENT_TRACE_LOGFILE logFile;
	TRACEHANDLE traceSessionHandle;
	TRACEHANDLE openedHandle;

	HANDLE processThreadHandle;
	DWORD currentProcessId;

	bool isActive;

	static DWORD WINAPI RunProcessTraceThreadFunction(LPVOID parameter);
	static void AdjustPrivileges();

	unordered_map<uint64_t, const EventDescription*> syscallDescriptions;
public:

	ETWRuntime runtime;

	ETW();
	~ETW();

	virtual CaptureStatus::Type Start(Mode::Type mode, int frequency, const ThreadList& threads) override;
	virtual bool Stop() override;

	DWORD GetProcessID() const { return currentProcessId; }
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct CSwitch
{
	// New thread ID after the switch.
	uint32 NewThreadId;

	// Previous thread ID.
	uint32 OldThreadId;

	// Thread priority of the new thread.
	int8  NewThreadPriority;

	// Thread priority of the previous thread.
	int8  OldThreadPriority;

	//The index of the C-state that was last used by the processor. A value of 0 represents the lightest idle state with higher values representing deeper C-states.
	uint8  PreviousCState;

	// Not used.
	int8  SpareByte;

	// Wait reason for the previous thread. The following are the possible values:
	//       0	Executive
	//       1	FreePage
	//       2	PageIn
	//       3	PoolAllocation
	//       4	DelayExecution
	//       5	Suspended
	//       6	UserRequest
	//       7	WrExecutive
	//       8	WrFreePage
	//       9	WrPageIn
	//       10	WrPoolAllocation
	//       11	WrDelayExecution
	//       12	WrSuspended
	//       13	WrUserRequest
	//       14	WrEventPair
	//       15	WrQueue
	//       16	WrLpcReceive
	//       17	WrLpcReply
	//       18	WrVirtualMemory
	//       19	WrPageOut
	//       20	WrRendezvous
	//       21	WrKeyedEvent
	//       22	WrTerminated
	//       23	WrProcessInSwap
	//       24	WrCpuRateControl
	//       25	WrCalloutStack
	//       26	WrKernel
	//       27	WrResource
	//       28	WrPushLock
	//       29	WrMutex
	//       30	WrQuantumEnd
	//       31	WrDispatchInt
	//       32	WrPreempted
	//       33	WrYieldExecution
	//       34	WrFastMutex
	//       35	WrGuardedMutex
	//       36	WrRundown
	//       37	MaximumWaitReason
	int8  OldThreadWaitReason;

	// Wait mode for the previous thread. The following are the possible values:
	//     0 KernelMode
	//     1 UserMode
	int8  OldThreadWaitMode;

	// State of the previous thread. The following are the possible state values:
	//     0 Initialized
	//     1 Ready
	//     2 Running
	//     3 Standby
	//     4 Terminated
	//     5 Waiting
	//     6 Transition
	//     7 DeferredReady (added for Windows Server 2003)
	int8  OldThreadState;

	// Ideal wait time of the previous thread.
	int8  OldThreadWaitIdealProcessor;

	// Wait time for the new thread.
	uint32 NewThreadWaitTime;

	// Reserved.
	uint32 Reserved;

	static const byte OPCODE = 36;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct StackWalk_Event
{
	// Original event time stamp from the event header
	uint64 EventTimeStamp;

	// The process identifier of the original event
	uint32 StackProcess;

	// The thread identifier of the original event
	uint32 StackThread;

	// Callstack head
	uint64 Stack0;

	static const byte OPCODE = 32;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct Thread_TypeGroup1
{
	// Process identifier of the thread involved in the event.
	uint32 ProcessId;
	// Thread identifier of the thread involved in the event.
	uint32 TThreadId;
	// Base address of the thread's stack.
	uint64 StackBase;
	// Limit of the thread's stack.
	uint64 StackLimit;
	// Base address of the thread's user-mode stack.
	uint64 UserStackBase;
	// Limit of the thread's user-mode stack.
	uint64 UserStackLimit;
	// The set of processors on which the thread is allowed to run.
	uint32 Affinity;
	// Starting address of the function to be executed by this thread.
	uint64 Win32StartAddr;
	// Thread environment block base address.
	uint64 TebBase;
	// Identifies the service if the thread is owned by a service; otherwise, zero.
	uint32 SubProcessTag;
	// The scheduler priority of the thread
	uint8  BasePriority;
	// A memory page priority hint for memory pages accessed by the thread.
	uint8  PagePriority;
	// An IO priority hint for scheduling IOs generated by the thread.
	uint8  IoPriority;
	// Not used.
	uint8  ThreadFlags;

	enum Opcode : uint8
	{
		Start = 1,
		End = 2,
		DCStart = 3,
		DCEnd = 4,
	};
};

size_t GetSIDSize(uint8* ptr)
{
	size_t result = 0;

	int sid = *((int*)ptr);

	if (sid != 0)
	{
		size_t tokenSize = 16;
		ptr += tokenSize;
		result += tokenSize;
		result += 8 + (4 * ((SID*)ptr)->SubAuthorityCount);
	}
	else
	{
		result = 4;
	}

	return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// https://github.com/Microsoft/perfview/blob/688a8564062d51321bbab53cd71d9e174a77d2ce/src/TraceEvent/TraceEvent.cs
struct Process_TypeGroup1
{
	// The address of the process object in the kernel.
	uint64 UniqueProcessKey;
	// Global process identifier that you can use to identify a process. 
	uint32 ProcessId;
	// Unique identifier of the process that creates this process. 
	uint32 ParentId;
	// Unique identifier that an operating system generates when it creates a new session.
	uint32 SessionId;
	// Exit status of the stopped process.
	int32 ExitStatus;
	// The physical address of the page table of the process.
	uint64 DirectoryTableBase;
	// (?) uint8 Flags;
	// object UserSID;
	// string ImageFileName;
	// wstring CommandLine;

	static size_t GetSIDOffset(PEVENT_RECORD pEvent)
	{
		if (pEvent->EventHeader.EventDescriptor.Version >= 4)
			return 36;

		if (pEvent->EventHeader.EventDescriptor.Version == 3)
			return 32;

		return 24;
	}

	const char* GetProcessName(PEVENT_RECORD pEvent) const
	{
		OPTICK_ASSERT((pEvent->EventHeader.Flags & EVENT_HEADER_FLAG_64_BIT_HEADER) != 0, "32-bit is not supported! Disable OPTICK_ENABLE_TRACING on 32-bit platform if needed!");
		size_t sidOffset = GetSIDOffset(pEvent);
		size_t sidSize = GetSIDSize((uint8*)this + sidOffset);
		return (char*)this + sidOffset + sidSize;
	}

	enum Opcode
	{
		Start = 1,
		End = 2,
		DCStart = 3,
		DCEnd = 4,
		Defunct = 39,
	};
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct SampledProfile
{
	uint32 InstructionPointer;
	uint32 ThreadId;
	uint32 Count;

	static const byte OPCODE = 46;
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct SysCallEnter
{
	uintptr_t SysCallAddress;

	static const byte OPCODE = 51;
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct SysCallExit
{
	uint32 SysCallNtStatus;

	static const byte OPCODE = 52;
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
constexpr uint32 GuidHash(uint32 u0, uint32 u1, uint32 u2, uint32 u3)
{
	return u0 ^ u1 ^ u2 ^ u3;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint32 GuidHash(GUID guid)
{
	return GuidHash(guid.Data1, (guid.Data3 << 16) | guid.Data2, ((uint32*)guid.Data4)[0], ((uint32*)guid.Data4)[1]);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define ETW_GUID(NAME, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) DEFINE_GUID(NAME, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8); \
																  const uint32 NAME##Hash = GuidHash((uint32)l, (uint32)((w2 << 16) | w1), (uint32)((b4 << 24) | (b3 << 16) | (b2 << 8) | b1), (uint32)((b8 << 24) | (b7 << 16) | (b6 << 8) | b5));

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ce1dbfb4-137e-4da6-87b0-3f59aa102cbc 
ETW_GUID(SampledProfileGuid, 0xce1dbfb4, 0x137e, 0x4da6, 0x87, 0xb0, 0x3f, 0x59, 0xaa, 0x10, 0x2c, 0xbc);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 3d6fa8d1-fe05-11d0-9dda-00c04fd7ba7c
// https://docs.microsoft.com/en-us/windows/desktop/etw/thread
ETW_GUID(ThreadGuid, 0x3d6fa8d1, 0xfe05, 0x11d0, 0x9d, 0xda, 0x00, 0xc0, 0x4f, 0xd7, 0xba, 0x7c);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 3d6fa8d0-fe05-11d0-9dda-00c04fd7ba7c
// https://docs.microsoft.com/en-us/windows/desktop/etw/process
ETW_GUID(ProcessGuid, 0x3d6fa8d0, 0xfe05, 0x11d0, 0x9d, 0xda, 0x00, 0xc0, 0x4f, 0xd7, 0xba, 0x7c);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// def2fe46-7bd6-4b80-bd94-f57fe20d0ce3
// https://docs.microsoft.com/en-us/windows/win32/etw/stackwalk
ETW_GUID(StackWalkGuid, 0xdef2fe46, 0x7bd6, 0x4b80, 0xbd, 0x94, 0xf5, 0x7f, 0xe2, 0x0d, 0x0c, 0xe3);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// https://docs.microsoft.com/en-us/windows/win32/etw/perfinfo
// ce1dbfb4-137e-4da6-87b0-3f59aa102cbc
ETW_GUID(PerfInfoGuid, 0xce1dbfb4, 0x137e, 0x4da6, 0x87, 0xb0, 0x3f, 0x59, 0xaa, 0x10, 0x2c, 0xbc);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ETW* g_ETW;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void OnThreadEvent(PEVENT_RECORD eventRecord)
{
	ETWRuntime& runtime = g_ETW->runtime;

	switch (eventRecord->EventHeader.EventDescriptor.Opcode)
	{
	case CSwitch::OPCODE:
		if (sizeof(CSwitch) == eventRecord->UserDataLength)
		{
			CSwitch* pSwitchEvent = (CSwitch*)eventRecord->UserData;

			SwitchContextDesc desc;
			desc.reason = pSwitchEvent->OldThreadWaitReason;
			desc.cpuId = eventRecord->BufferContext.ProcessorNumber;
			desc.oldThreadId = (uint64)pSwitchEvent->OldThreadId;
			desc.newThreadId = (uint64)pSwitchEvent->NewThreadId;
			desc.timestamp = eventRecord->EventHeader.TimeStamp.QuadPart;
			Core::Get().ReportSwitchContext(desc);


			// Assign ThreadID to the cores
			if (runtime.activeThreadsIDs.find(desc.newThreadId) != runtime.activeThreadsIDs.end())
			{
				runtime.activeCores[desc.cpuId] = desc.newThreadId;
			}
			else if (runtime.activeThreadsIDs.find(desc.oldThreadId) != runtime.activeThreadsIDs.end())
			{
				runtime.activeCores[desc.cpuId] = INVALID_THREAD_ID;
			}
		}
		break;

	case Thread_TypeGroup1::Start:
	case Thread_TypeGroup1::DCStart:
		if (eventRecord->UserDataLength >= sizeof(Thread_TypeGroup1))
		{
			const Thread_TypeGroup1* pThreadEvent = (const Thread_TypeGroup1*)eventRecord->UserData;
			Core::Get().RegisterThreadDescription(ThreadDescription("", pThreadEvent->TThreadId, pThreadEvent->ProcessId, 1, pThreadEvent->BasePriority));

			if (pThreadEvent->ProcessId == runtime.currentProcessId)
				runtime.activeThreadsIDs.insert(pThreadEvent->TThreadId);
		}
		break;

	default:
		break;
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void OnProcessEvent(PEVENT_RECORD eventRecord)
{
	switch (eventRecord->EventHeader.EventDescriptor.Opcode)
	{
	case Process_TypeGroup1::Start:
	case Process_TypeGroup1::DCStart:
		if (eventRecord->UserDataLength >= sizeof(Process_TypeGroup1))
		{
			const Process_TypeGroup1* pProcessEvent = (const Process_TypeGroup1*)eventRecord->UserData;
			Core::Get().RegisterProcessDescription(ProcessDescription(pProcessEvent->GetProcessName(eventRecord), pProcessEvent->ProcessId, pProcessEvent->UniqueProcessKey));
		}
		break;

	default:
		break;
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void OnStackWalkEvent(PEVENT_RECORD eventRecord)
{
	switch (eventRecord->EventHeader.EventDescriptor.Opcode)
	{
	case StackWalk_Event::OPCODE:
		if (eventRecord->UserData && eventRecord->UserDataLength >= sizeof(StackWalk_Event))
		{
			//TODO: Support x86 windows kernels
			const size_t osKernelPtrSize = sizeof(uint64);

			StackWalk_Event* pStackWalkEvent = (StackWalk_Event*)eventRecord->UserData;
			uint32 count = 1 + (eventRecord->UserDataLength - sizeof(StackWalk_Event)) / osKernelPtrSize;

			if (count && pStackWalkEvent->StackThread != 0)
			{
				if (pStackWalkEvent->StackProcess == g_ETW->GetProcessID())
				{
					CallstackDesc desc;
					desc.threadID = pStackWalkEvent->StackThread;
					desc.timestamp = pStackWalkEvent->EventTimeStamp;

					static_assert(osKernelPtrSize == sizeof(uint64), "Incompatible types!");
					desc.callstack = &pStackWalkEvent->Stack0;

					desc.count = (uint8)count;
					Core::Get().ReportStackWalk(desc);
				}
			}
		}
		break;
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void OnPerfInfoEvent(PEVENT_RECORD eventRecord)
{
	ETWRuntime& runtime = g_ETW->runtime;

	switch (eventRecord->EventHeader.EventDescriptor.Opcode)
	{
	case SysCallEnter::OPCODE:
		if (eventRecord->UserDataLength >= sizeof(SysCallEnter))
		{
			uint8_t cpuId = eventRecord->BufferContext.ProcessorNumber;
			uint64_t threadId = runtime.activeCores[cpuId];

			if (threadId != INVALID_THREAD_ID)
			{
				SysCallEnter* pEventEnter = (SysCallEnter*)eventRecord->UserData;

				SysCallData& sysCall = Core::Get().syscallCollector.Add();
				sysCall.start = eventRecord->EventHeader.TimeStamp.QuadPart;
				sysCall.finish = EventTime::INVALID_TIMESTAMP;
				sysCall.threadID = threadId;
				sysCall.id = pEventEnter->SysCallAddress;
				sysCall.description = nullptr;

				runtime.activeSyscalls.push_back(std::make_pair(cpuId, &sysCall));
			}
		}
		break;

	case SysCallExit::OPCODE:
		if (eventRecord->UserDataLength >= sizeof(SysCallExit))
		{
			uint8_t cpuId = eventRecord->BufferContext.ProcessorNumber;
			if (runtime.activeCores[cpuId] != INVALID_THREAD_ID)
			{
				for (int i = (int)runtime.activeSyscalls.size() - 1; i >= 0; --i)
				{
					if (runtime.activeSyscalls[i].first == cpuId)
					{
						runtime.activeSyscalls[i].second->finish = eventRecord->EventHeader.TimeStamp.QuadPart;
						runtime.activeSyscalls.erase(runtime.activeSyscalls.begin() + i);
						break;
					}
				}
			}
		}
		break;

	default:
		break;
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WINAPI OnRecordEvent(PEVENT_RECORD eventRecord)
{
	const uint32 eventHash = GuidHash(eventRecord->EventHeader.ProviderId);

	switch (eventHash)
	{
	case ThreadGuidHash:
		OnThreadEvent(eventRecord);
		break;

	case ProcessGuidHash:
		OnProcessEvent(eventRecord);
		break;

	case StackWalkGuidHash:
		OnStackWalkEvent(eventRecord);
		break;

	case PerfInfoGuidHash:
		OnPerfInfoEvent(eventRecord);
		break;

	default:
		break;
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static ULONG WINAPI OnBufferRecord(_In_ PEVENT_TRACE_LOGFILE Buffer)
{
	OPTICK_UNUSED(Buffer);
	return true;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const TRACEHANDLE INVALID_TRACEHANDLE = (TRACEHANDLE)-1;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DWORD WINAPI ETW::RunProcessTraceThreadFunction(LPVOID parameter)
{
	Memory::InitThread();
	Core::Get().RegisterThreadDescription(ThreadDescription("[Optick] ETW", GetCurrentThreadId(), GetCurrentProcessId()));
	ETW* etw = (ETW*)parameter;
	ULONG status = ProcessTrace(&etw->openedHandle, 1, 0, 0);
	OPTICK_UNUSED(status);
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ETW::AdjustPrivileges()
{
#if OPTICK_PC
	HANDLE token = 0;
	if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token))
	{
		TOKEN_PRIVILEGES tokenPrivileges;
		memset(&tokenPrivileges, 0, sizeof(tokenPrivileges));
		tokenPrivileges.PrivilegeCount = 1;
		tokenPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
		LookupPrivilegeValue(NULL, SE_SYSTEM_PROFILE_NAME, &tokenPrivileges.Privileges[0].Luid);

		AdjustTokenPrivileges(token, FALSE, &tokenPrivileges, 0, (PTOKEN_PRIVILEGES)NULL, 0);
		CloseHandle(token);
	}
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ETW::ETW()
	: traceProperties(nullptr)
	, traceSessionHandle(INVALID_TRACEHANDLE)
	, openedHandle(INVALID_TRACEHANDLE)
	, processThreadHandle(INVALID_HANDLE_VALUE)
	, currentProcessId((DWORD)-1)
	, isActive(false)
{
	currentProcessId = GetCurrentProcessId();

	OPTICK_ASSERT(g_ETW == nullptr, "Can't create more than one ETW session");
	g_ETW = this;
}

CaptureStatus::Type ETW::Start(Mode::Type mode, int frequency, const ThreadList& threads)
{
	if (!isActive)
	{
		AdjustPrivileges();

		runtime.Reset();

		for (auto it = threads.begin(); it != threads.end(); ++it)
		{
			ThreadEntry* entry = *it;
			if (entry->isAlive)
			{
				runtime.activeThreadsIDs.insert(entry->description.threadID);
			}
		}

				
		ULONG bufferSize = sizeof(EVENT_TRACE_PROPERTIES) + (ETW_MAXIMUM_SESSION_NAME + MAX_PATH) * sizeof(WCHAR);
		if (traceProperties == nullptr)
			traceProperties = (EVENT_TRACE_PROPERTIES*)Memory::Alloc(bufferSize);
		ZeroMemory(traceProperties, bufferSize);
		traceProperties->Wnode.BufferSize = bufferSize;
		traceProperties->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);
		traceProperties->EnableFlags = 0;

#if OPTICK_PC
		traceProperties->BufferSize = ETW_BUFFER_SIZE;
		traceProperties->MinimumBuffers = ETW_BUFFER_COUNT;
#endif

		if (mode & Mode::SWITCH_CONTEXT)
		{
			traceProperties->EnableFlags |= EVENT_TRACE_FLAG_CSWITCH;
		}

		if (mode & Mode::AUTOSAMPLING)
		{
			traceProperties->EnableFlags |= EVENT_TRACE_FLAG_PROFILE;
		}

		if (mode & Mode::SYS_CALLS)
		{
			traceProperties->EnableFlags |= EVENT_TRACE_FLAG_SYSTEMCALL;
		}

		if (mode & Mode::OTHER_PROCESSES)
		{
			traceProperties->EnableFlags |= EVENT_TRACE_FLAG_PROCESS;
			traceProperties->EnableFlags |= EVENT_TRACE_FLAG_THREAD;
		}

		traceProperties->LogFileMode = EVENT_TRACE_REAL_TIME_MODE;
		traceProperties->Wnode.Flags = WNODE_FLAG_TRACED_GUID;
		//
		// https://msdn.microsoft.com/en-us/library/windows/desktop/aa364160(v=vs.85).aspx
		// Clock resolution = QPC
		traceProperties->Wnode.ClientContext = 1;
		traceProperties->Wnode.Guid = SystemTraceControlGuid;

		// ERROR_BAD_LENGTH(24): The Wnode.BufferSize member of Properties specifies an incorrect size. Properties does not have sufficient space allocated to hold a copy of SessionName.
		// ERROR_ALREADY_EXISTS(183): A session with the same name or GUID is already running.
		// ERROR_ACCESS_DENIED(5): Only users with administrative privileges, users in the Performance Log Users group, and services running as LocalSystem, LocalService, NetworkService can control event tracing sessions.
		// ERROR_INVALID_PARAMETER(87)
		// ERROR_BAD_PATHNAME(161)
		// ERROR_DISK_FULL(112)
		// ERROR_NO_SUCH_PRIVILEGE(1313)
		int retryCount = 4;
		ULONG status = CaptureStatus::OK;

		while (--retryCount >= 0)
		{
			status = StartTrace(&traceSessionHandle, KERNEL_LOGGER_NAME, traceProperties);

			switch (status)
			{
			case ERROR_NO_SUCH_PRIVILEGE:
				AdjustPrivileges();
				break;

			case ERROR_ALREADY_EXISTS:
				ControlTrace(0, KERNEL_LOGGER_NAME, traceProperties, EVENT_TRACE_CONTROL_STOP);
				break;

			case ERROR_ACCESS_DENIED:
				return CaptureStatus::ERR_TRACER_ACCESS_DENIED;

			case ERROR_SUCCESS:
				retryCount = 0;
				break;

			default:
				return CaptureStatus::ERR_TRACER_FAILED;
			}
		}

		if (status != ERROR_SUCCESS)
		{
			return CaptureStatus::ERR_TRACER_FAILED;
		}

		CLASSIC_EVENT_ID callstackSamples[4];
		int callstackCountSamplesCount = 0;

		if (mode & Mode::AUTOSAMPLING)
		{
			callstackSamples[callstackCountSamplesCount].EventGuid = SampledProfileGuid;
			callstackSamples[callstackCountSamplesCount].Type = SampledProfile::OPCODE;
			++callstackCountSamplesCount;
		}

		if (mode & Mode::SYS_CALLS)
		{
			callstackSamples[callstackCountSamplesCount].EventGuid = SampledProfileGuid;
			callstackSamples[callstackCountSamplesCount].Type = SysCallEnter::OPCODE;
			++callstackCountSamplesCount;
		}

		/*
					callstackSamples[callstackCountSamplesCount].EventGuid = CSwitchProfileGuid;
					callstackSamples[callstackCountSamplesCount].Type = CSwitch::OPCODE;
					++callstackCountSamplesCount;
		*/


		/*
				https://msdn.microsoft.com/en-us/library/windows/desktop/dd392328%28v=vs.85%29.aspx?f=255&MSPPError=-2147217396
				Typically, on 64-bit computers, you cannot capture the kernel stack in certain contexts when page faults are not allowed. To enable walking the kernel stack on x64, set
				the DisablePagingExecutive Memory Management registry value to 1. The DisablePagingExecutive registry value is located under the following registry key:
				HKEY_LOCAL_MACHINE\System\CurrentControlSet\Control\Session Manager\Memory Management
		*/
		if (callstackCountSamplesCount > 0)
		{
			status = TraceSetInformation(traceSessionHandle, TraceStackTracingInfo, &callstackSamples[0], sizeof(CLASSIC_EVENT_ID) * callstackCountSamplesCount);
			if (status != ERROR_SUCCESS)
			{
				OPTICK_FAILED("TraceSetInformation - failed");
				return CaptureStatus::ERR_TRACER_FAILED;
			}
		}

		if (mode & Mode::AUTOSAMPLING)
		{
			TRACE_PROFILE_INTERVAL itnerval = { 0 };
			memset(&itnerval, 0, sizeof(TRACE_PROFILE_INTERVAL));
			int step = 10000 * 1000 / frequency; // 1ms = 10000 steps
			itnerval.Interval = step; // std::max(1221, std::min(step, 10000));
			// The SessionHandle is irrelevant for this information class and must be zero, else the function returns ERROR_INVALID_PARAMETER.
			status = TraceSetInformation(0, TraceSampledProfileIntervalInfo, &itnerval, sizeof(TRACE_PROFILE_INTERVAL));
			OPTICK_ASSERT(status == ERROR_SUCCESS, "TraceSetInformation - failed");
		}

		ZeroMemory(&logFile, sizeof(EVENT_TRACE_LOGFILE));
		logFile.LoggerName = const_cast<PTCHAR>(KERNEL_LOGGER_NAME);
		logFile.ProcessTraceMode = (PROCESS_TRACE_MODE_REAL_TIME | PROCESS_TRACE_MODE_EVENT_RECORD | PROCESS_TRACE_MODE_RAW_TIMESTAMP);
		logFile.EventRecordCallback = OnRecordEvent;
		logFile.BufferCallback = OnBufferRecord;
		openedHandle = OpenTrace(&logFile);
		if (openedHandle == INVALID_TRACEHANDLE)
		{
			OPTICK_FAILED("OpenTrace - failed");
			return CaptureStatus::ERR_TRACER_FAILED;
		}

		DWORD threadID;
		processThreadHandle = CreateThread(0, 0, RunProcessTraceThreadFunction, this, 0, &threadID);

		isActive = true;
	}

	return CaptureStatus::OK;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ETW::Stop()
{
	if (!isActive)
	{
		return false;
	}

	ULONG controlTraceResult = ControlTrace(openedHandle, KERNEL_LOGGER_NAME, traceProperties, EVENT_TRACE_CONTROL_STOP);

	// ERROR_CTX_CLOSE_PENDING(7007L): The call was successful. The ProcessTrace function will stop after it has processed all real-time events in its buffers (it will not receive any new events).
	// ERROR_BUSY(170L): Prior to Windows Vista, you cannot close the trace until the ProcessTrace function completes.
	// ERROR_INVALID_HANDLE(6L): One of the following is true: TraceHandle is NULL. TraceHandle is INVALID_HANDLE_VALUE.
	ULONG closeTraceStatus = CloseTrace(openedHandle);

	// Wait for ProcessThread to finish
	WaitForSingleObject(processThreadHandle, INFINITE);
	BOOL wasThreadClosed = CloseHandle(processThreadHandle);

	isActive = false;

	runtime.activeThreadsIDs.clear();

	return wasThreadClosed && (closeTraceStatus == ERROR_SUCCESS) && (controlTraceResult == ERROR_SUCCESS);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ETW::~ETW()
{
	Stop();
	Memory::Free(traceProperties);
	traceProperties = nullptr;
	g_ETW = nullptr;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Trace* Platform::CreateTrace()
{
	return Memory::New<ETW>();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Symbol Resolving
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define USE_DBG_HELP (OPTICK_PC)

#if USE_DBG_HELP
#include <DbgHelp.h>
#pragma comment( lib, "DbgHelp.Lib" )
#endif

#include "optick_serialization.h"

#if OPTICK_PC
#include <psapi.h>
#else
// Forward declare kernel functions
#pragma pack(push,8)
typedef struct _MODULEINFO {
	LPVOID lpBaseOfDll;
	DWORD SizeOfImage;
	LPVOID EntryPoint;
} MODULEINFO, *LPMODULEINFO;
#pragma pack(pop)
#ifndef EnumProcessModulesEx
#define EnumProcessModulesEx        K32EnumProcessModulesEx
EXTERN_C DWORD WINAPI K32EnumProcessModulesEx(HANDLE hProcess, HMODULE *lphModule, DWORD cb, LPDWORD lpcbNeeded, DWORD dwFilterFlag);
#endif
#ifndef GetModuleInformation
#define GetModuleInformation        K32GetModuleInformation
EXTERN_C DWORD WINAPI K32GetModuleInformation(HANDLE hProcess, HMODULE hModule, LPMODULEINFO lpmodinfo, DWORD cb);
#endif

#ifndef GetModuleFileNameExA
#define GetModuleFileNameExA        K32GetModuleFileNameExA
EXTERN_C DWORD WINAPI K32GetModuleFileNameExA(HANDLE hProcess, HMODULE hModule, LPSTR lpFilename, DWORD nSize);
#endif
#endif

namespace Optick
{
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//void ReportLastError()
//{
//	LPVOID lpMsgBuf;
//	DWORD dw = GetLastError();
//
//	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
//								NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
//								(LPTSTR)&lpMsgBuf, 0, NULL);
//
//	MessageBox(NULL, (LPCTSTR)lpMsgBuf, TEXT("Error"), MB_OK);
//	LocalFree(lpMsgBuf);
//}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef array<uintptr_t, 512> CallStackBuffer;
typedef unordered_map<uint64, Symbol> SymbolCache;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class WinSymbolEngine : public SymbolEngine
{
	HANDLE hProcess;

	bool isInitialized;

	bool needRestorePreviousSettings;
	uint32 previousOptions;
	static const size_t MAX_SEARCH_PATH_LENGTH = 2048;
	char previousSearchPath[MAX_SEARCH_PATH_LENGTH];

	SymbolCache cache;
	vector<Module> modules;

	void InitSystemModules();
	void InitApplicationModules();
public:
	WinSymbolEngine();
	~WinSymbolEngine();

	void Init();
	void Close();

	// Get Symbol from PDB file
	virtual const Symbol * GetSymbol(uint64 dwAddress) override;
	virtual const vector<Module>& GetModules() override;
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
WinSymbolEngine::WinSymbolEngine() : hProcess(GetCurrentProcess()), isInitialized(false), needRestorePreviousSettings(false), previousOptions(0)
{
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
WinSymbolEngine::~WinSymbolEngine()
{
	Close();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const Symbol* WinSymbolEngine::GetSymbol(uint64 address)
{
	if (address == 0)
		return nullptr;

	Init();

	Symbol& symbol = cache[address];

	if (symbol.address != 0)
		return &symbol;

	if (!isInitialized)
		return nullptr;

	symbol.address = address;

#if USE_DBG_HELP
	DWORD64 dwAddress = static_cast<DWORD64>(address);

	// FileName and Line
	IMAGEHLP_LINEW64 lineInfo;
	memset(&lineInfo, 0, sizeof(IMAGEHLP_LINEW64));
	lineInfo.SizeOfStruct = sizeof(lineInfo);
	DWORD dwDisp;
	if (SymGetLineFromAddrW64(hProcess, dwAddress, &dwDisp, &lineInfo))
	{
		symbol.file = lineInfo.FileName;
		symbol.line = lineInfo.LineNumber;
	}

	const size_t length = (sizeof(SYMBOL_INFOW) + MAX_SYM_NAME * sizeof(WCHAR) + sizeof(ULONG64) - 1) / sizeof(ULONG64) + 1;

	// Function Name
	ULONG64 buffer[length];
	PSYMBOL_INFOW dbgSymbol = (PSYMBOL_INFOW)buffer;
	memset(dbgSymbol, 0, sizeof(buffer));
	dbgSymbol->SizeOfStruct = sizeof(SYMBOL_INFOW);
	dbgSymbol->MaxNameLen = MAX_SYM_NAME;

	DWORD64 offset = 0;
	if (SymFromAddrW(hProcess, dwAddress, &offset, dbgSymbol))
	{
		symbol.function.resize(dbgSymbol->NameLen);
		memcpy(&symbol.function[0], &dbgSymbol->Name[0], sizeof(WCHAR) * dbgSymbol->NameLen);
	}

	symbol.offset = static_cast<uintptr_t>(offset);
#endif

	return &symbol;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const vector<Module>& WinSymbolEngine::GetModules()
{
	if (modules.empty())
	{
		InitSystemModules();
		InitApplicationModules();
	}
	return modules;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// const char* USER_SYMBOL_SEARCH_PATH = "http://msdl.microsoft.com/download/symbols";
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WinSymbolEngine::Init()
{
	if (!isInitialized)
	{
#if USE_DBG_HELP
		previousOptions = SymGetOptions();

		memset(previousSearchPath, 0, MAX_SEARCH_PATH_LENGTH);
		SymGetSearchPath(hProcess, previousSearchPath, MAX_SEARCH_PATH_LENGTH);

		SymSetOptions(SymGetOptions() | SYMOPT_LOAD_LINES | SYMOPT_DEFERRED_LOADS | SYMOPT_UNDNAME | SYMOPT_INCLUDE_32BIT_MODULES | SYMOPT_LOAD_ANYTHING);
		if (!SymInitialize(hProcess, NULL, TRUE))
		{
			needRestorePreviousSettings = true;
			SymCleanup(hProcess);

			if (SymInitialize(hProcess, NULL, TRUE))
				isInitialized = true;
		}
		else
		{
			isInitialized = true;
		}

		const vector<Module>& loadedModules = GetModules();
		for (size_t i = 0; i < loadedModules.size(); ++i)
		{
			const Module& module = loadedModules[i];
			SymLoadModule64(hProcess, NULL, module.path.c_str(), NULL, (DWORD64)module.address, (DWORD)module.size);
		}

#else
		isInitialized = true;
#endif
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef DWORD(__stdcall *pZwQuerySystemInformation)(DWORD, LPVOID, DWORD, DWORD*);
#define SystemModuleInformation 11 // SYSTEMINFOCLASS
#define MAXIMUM_FILENAME_LENGTH 256

struct SYSTEM_MODULE_INFORMATION
{
	DWORD reserved1;
	DWORD reserved2;
	PVOID mappedBase;
	PVOID imageBase;
	DWORD imageSize;
	DWORD flags;
	WORD loadOrderIndex;
	WORD initOrderIndex;
	WORD loadCount;
	WORD moduleNameOffset;
	CHAR imageName[MAXIMUM_FILENAME_LENGTH];
};

#pragma warning (push)
#pragma warning(disable : 4200)
struct MODULE_LIST
{
	DWORD dwModules;
	SYSTEM_MODULE_INFORMATION pModulesInfo[];
};
#pragma warning (pop)

void WinSymbolEngine::InitSystemModules()
{
	ULONG returnLength = 0;
	ULONG systemInformationLength = 0;
	MODULE_LIST* pModuleList = nullptr;

#pragma warning (push)
#pragma warning(disable : 4191)
	pZwQuerySystemInformation ZwQuerySystemInformation = (pZwQuerySystemInformation)GetProcAddress(GetModuleHandle(TEXT("ntdll.dll")), "ZwQuerySystemInformation");
#pragma warning (pop)

	ZwQuerySystemInformation(SystemModuleInformation, pModuleList, systemInformationLength, &returnLength);
	systemInformationLength = returnLength;
	pModuleList = (MODULE_LIST*)Memory::Alloc(systemInformationLength);
	DWORD status = ZwQuerySystemInformation(SystemModuleInformation, pModuleList, systemInformationLength, &returnLength);
	if (status == ERROR_SUCCESS)
	{
		char systemRootPath[MAXIMUM_FILENAME_LENGTH] = { 0 };
#if OPTICK_PC
		ExpandEnvironmentStringsA("%SystemRoot%", systemRootPath, MAXIMUM_FILENAME_LENGTH);
#else
		strcpy_s(systemRootPath, "C:\\Windows");
#endif

		const char* systemRootPattern = "\\SystemRoot";

		modules.reserve(modules.size() + pModuleList->dwModules);

		for (uint32_t i = 0; i < pModuleList->dwModules; ++i)
		{
			SYSTEM_MODULE_INFORMATION& module = pModuleList->pModulesInfo[i];

			char path[MAXIMUM_FILENAME_LENGTH] = { 0 };

			if (strstr(module.imageName, systemRootPattern) == module.imageName)
			{
				strcpy_s(path, systemRootPath);
				strcat_s(path, module.imageName + strlen(systemRootPattern));
			}
			else
			{
				strcpy_s(path, module.imageName);
			}

			modules.push_back(Module(path, (void*)module.imageBase, module.imageSize));
		}
	}
	else
	{
		OPTICK_FAILED("Can't query System Module Information!");
	}

	if (pModuleList)
	{
		Memory::Free(pModuleList);
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WinSymbolEngine::InitApplicationModules()
{
	HANDLE processHandle = GetCurrentProcess();
	HMODULE hModules[256];
	DWORD modulesSize = 0;
	EnumProcessModulesEx(processHandle, hModules, sizeof(hModules), &modulesSize, 0);

	int moduleCount = modulesSize / sizeof(HMODULE);

	modules.reserve(modules.size() + moduleCount);

	for (int i = 0; i < moduleCount; ++i)
	{
		MODULEINFO info = { 0 };
		if (GetModuleInformation(processHandle, hModules[i], &info, sizeof(MODULEINFO)))
		{
			char name[MAX_PATH] = "UnknownModule";
			GetModuleFileNameExA(processHandle, hModules[i], name, MAX_PATH);

			modules.push_back(Module(name, info.lpBaseOfDll, info.SizeOfImage));
		}
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WinSymbolEngine::Close()
{
	if (isInitialized)
	{
#if USE_DBG_HELP
		SymCleanup(hProcess);
		if (needRestorePreviousSettings)
		{
			HANDLE currentProcess = GetCurrentProcess();

			SymSetOptions(previousOptions);
			SymSetSearchPath(currentProcess, previousSearchPath);
			SymInitialize(currentProcess, NULL, TRUE);

			needRestorePreviousSettings = false;
		}
#endif
		modules.clear();
		isInitialized = false;
	}
}
//////////////////////////////////////////////////////////////////////////
SymbolEngine* Platform::CreateSymbolEngine()
{
	return Memory::New<WinSymbolEngine>();
}
//////////////////////////////////////////////////////////////////////////
}
#endif //OPTICK_ENABLE_TRACING
#endif //USE_OPTICK
#endif //_MSC_VER