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
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Config
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "optick.config.h"

#if USE_OPTICK
#include <stdint.h>
#include <stddef.h>

#if defined(_MSC_VER)
#	define OPTICK_MSVC (1)
#	define OPTICK_64BIT (1)
#	if defined(_DURANGO)
#		define OPTICK_PC (0)
#	else
#		define OPTICK_PC (1)
#   endif
#elif defined(__clang__) || defined(__GNUC__)
#	define OPTICK_GCC (1)
#	if defined(__APPLE_CC__)
#		define OPTICK_OSX (1)
#		define OPTICK_64BIT (1)
#	elif defined(__linux__)
#		define OPTICK_LINUX (1)
#		define OPTICK_64BIT (1)
#	elif defined(__FreeBSD__)
#		define OPTICK_FREEBSD (1)
#		define OPTICK_64BIT (1)
#	endif
#	if defined(__aarch64__) || defined(_M_ARM64)
#		define OPTICK_ARM (1)
#		define OPTICK_64BIT (1)
#	elif defined(__arm__) || defined(_M_ARM)
#		define OPTICK_ARM (1)
#		define OPTICK_32BIT (1)
#	endif
#else
#error Compiler not supported
#endif

////////////////////////////////////////////////////////////////////////
// Target Platform
////////////////////////////////////////////////////////////////////////

#if defined(OPTICK_GCC)
#define OPTICK_FUNC __PRETTY_FUNCTION__
#elif defined(OPTICK_MSVC)
#define OPTICK_FUNC __FUNCSIG__
#else
#error Compiler not supported
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// EXPORTS 
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#if defined(OPTICK_EXPORTS) && defined(OPTICK_MSVC)
#define OPTICK_API __declspec(dllexport)
#else
#define OPTICK_API 
#endif
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define OPTICK_CONCAT_IMPL(x, y) x##y
#define OPTICK_CONCAT(x, y) OPTICK_CONCAT_IMPL(x, y)

#if defined(OPTICK_MSVC)
#define OPTICK_INLINE __forceinline
#elif defined(OPTICK_GCC)
#define OPTICK_INLINE __attribute__((always_inline)) inline
#else
#error Compiler is not supported
#endif


// Vulkan Forward Declarations
#define OPTICK_DEFINE_HANDLE(object) typedef struct object##_T *object;
OPTICK_DEFINE_HANDLE(VkDevice);
OPTICK_DEFINE_HANDLE(VkPhysicalDevice);
OPTICK_DEFINE_HANDLE(VkQueue);
OPTICK_DEFINE_HANDLE(VkCommandBuffer);
OPTICK_DEFINE_HANDLE(VkQueryPool);
OPTICK_DEFINE_HANDLE(VkCommandPool);
OPTICK_DEFINE_HANDLE(VkFence);

struct VkPhysicalDeviceProperties;
struct VkQueryPoolCreateInfo;
struct VkAllocationCallbacks;
struct VkCommandPoolCreateInfo;
struct VkCommandBufferAllocateInfo;
struct VkFenceCreateInfo;
struct VkSubmitInfo;
struct VkCommandBufferBeginInfo;

#ifndef VKAPI_PTR
#define OPTICK_VKAPI_PTR_DEFINED 1
#if defined(_WIN32)
    // On Windows, Vulkan commands use the stdcall convention
	#define VKAPI_PTR  __stdcall
#else
	#define VKAPI_PTR 
#endif
#endif

typedef void (VKAPI_PTR *PFN_vkGetPhysicalDeviceProperties_)(VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties* pProperties);
typedef int32_t (VKAPI_PTR *PFN_vkCreateQueryPool_)(VkDevice device, const VkQueryPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkQueryPool* pQueryPool);
typedef int32_t (VKAPI_PTR *PFN_vkCreateCommandPool_)(VkDevice device, const VkCommandPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkCommandPool* pCommandPool);
typedef int32_t (VKAPI_PTR *PFN_vkAllocateCommandBuffers_)(VkDevice device, const VkCommandBufferAllocateInfo* pAllocateInfo, VkCommandBuffer* pCommandBuffers);
typedef int32_t (VKAPI_PTR *PFN_vkCreateFence_)(VkDevice device, const VkFenceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkFence* pFence);
typedef void (VKAPI_PTR *PFN_vkCmdResetQueryPool_)(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount);
typedef int32_t (VKAPI_PTR *PFN_vkQueueSubmit_)(VkQueue queue, uint32_t submitCount, const VkSubmitInfo* pSubmits, VkFence fence);
typedef int32_t (VKAPI_PTR *PFN_vkWaitForFences_)(VkDevice device, uint32_t fenceCount, const VkFence* pFences, uint32_t waitAll, uint64_t timeout);
typedef int32_t (VKAPI_PTR *PFN_vkResetCommandBuffer_)(VkCommandBuffer commandBuffer, uint32_t flags);
typedef void (VKAPI_PTR *PFN_vkCmdWriteTimestamp_)(VkCommandBuffer commandBuffer, uint32_t pipelineStage, VkQueryPool queryPool, uint32_t query);
typedef int32_t (VKAPI_PTR *PFN_vkGetQueryPoolResults_)(VkDevice device, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount, size_t dataSize, void* pData, uint64_t stride, uint32_t flags);
typedef int32_t (VKAPI_PTR *PFN_vkBeginCommandBuffer_)(VkCommandBuffer commandBuffer, const VkCommandBufferBeginInfo* pBeginInfo);
typedef int32_t (VKAPI_PTR *PFN_vkEndCommandBuffer_)(VkCommandBuffer commandBuffer);
typedef int32_t (VKAPI_PTR *PFN_vkResetFences_)(VkDevice device, uint32_t fenceCount, const VkFence* pFences);
typedef void (VKAPI_PTR *PFN_vkDestroyCommandPool_)(VkDevice device, VkCommandPool commandPool, const VkAllocationCallbacks* pAllocator);
typedef void (VKAPI_PTR *PFN_vkDestroyQueryPool_)(VkDevice device, VkQueryPool queryPool, const VkAllocationCallbacks* pAllocator);
typedef void (VKAPI_PTR *PFN_vkDestroyFence_)(VkDevice device, VkFence fence, const VkAllocationCallbacks* pAllocator);
typedef void (VKAPI_PTR *PFN_vkFreeCommandBuffers_)(VkDevice device, VkCommandPool commandPool, uint32_t commandBufferCount, const VkCommandBuffer* pCommandBuffers);

#if OPTICK_VKAPI_PTR_DEFINED
#undef VKAPI_PTR
#endif

// D3D12 Forward Declarations
struct ID3D12CommandList;
struct ID3D12Device;
struct ID3D12CommandQueue;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace Optick
{
	struct OPTICK_API VulkanFunctions
	{
		PFN_vkGetPhysicalDeviceProperties_ vkGetPhysicalDeviceProperties;
		PFN_vkCreateQueryPool_ vkCreateQueryPool;
		PFN_vkCreateCommandPool_ vkCreateCommandPool;
		PFN_vkAllocateCommandBuffers_ vkAllocateCommandBuffers;
		PFN_vkCreateFence_ vkCreateFence;
		PFN_vkCmdResetQueryPool_ vkCmdResetQueryPool;
		PFN_vkQueueSubmit_ vkQueueSubmit;
		PFN_vkWaitForFences_ vkWaitForFences;
		PFN_vkResetCommandBuffer_ vkResetCommandBuffer;
		PFN_vkCmdWriteTimestamp_ vkCmdWriteTimestamp;
		PFN_vkGetQueryPoolResults_ vkGetQueryPoolResults;
		PFN_vkBeginCommandBuffer_ vkBeginCommandBuffer;
		PFN_vkEndCommandBuffer_ vkEndCommandBuffer;
		PFN_vkResetFences_ vkResetFences;
		PFN_vkDestroyCommandPool_ vkDestroyCommandPool;
		PFN_vkDestroyQueryPool_ vkDestroyQueryPool;
		PFN_vkDestroyFence_ vkDestroyFence;
		PFN_vkFreeCommandBuffers_ vkFreeCommandBuffers;
	};

	// Source: http://msdn.microsoft.com/en-us/library/system.windows.media.colors(v=vs.110).aspx
	// Image:  http://i.msdn.microsoft.com/dynimg/IC24340.png
	struct Color
	{
		enum
		{
			Null = 0x00000000,
			AliceBlue = 0xFFF0F8FF,
			AntiqueWhite = 0xFFFAEBD7,
			Aqua = 0xFF00FFFF,
			Aquamarine = 0xFF7FFFD4,
			Azure = 0xFFF0FFFF,
			Beige = 0xFFF5F5DC,
			Bisque = 0xFFFFE4C4,
			Black = 0xFF000000,
			BlanchedAlmond = 0xFFFFEBCD,
			Blue = 0xFF0000FF,
			BlueViolet = 0xFF8A2BE2,
			Brown = 0xFFA52A2A,
			BurlyWood = 0xFFDEB887,
			CadetBlue = 0xFF5F9EA0,
			Chartreuse = 0xFF7FFF00,
			Chocolate = 0xFFD2691E,
			Coral = 0xFFFF7F50,
			CornflowerBlue = 0xFF6495ED,
			Cornsilk = 0xFFFFF8DC,
			Crimson = 0xFFDC143C,
			Cyan = 0xFF00FFFF,
			DarkBlue = 0xFF00008B,
			DarkCyan = 0xFF008B8B,
			DarkGoldenRod = 0xFFB8860B,
			DarkGray = 0xFFA9A9A9,
			DarkGreen = 0xFF006400,
			DarkKhaki = 0xFFBDB76B,
			DarkMagenta = 0xFF8B008B,
			DarkOliveGreen = 0xFF556B2F,
			DarkOrange = 0xFFFF8C00,
			DarkOrchid = 0xFF9932CC,
			DarkRed = 0xFF8B0000,
			DarkSalmon = 0xFFE9967A,
			DarkSeaGreen = 0xFF8FBC8F,
			DarkSlateBlue = 0xFF483D8B,
			DarkSlateGray = 0xFF2F4F4F,
			DarkTurquoise = 0xFF00CED1,
			DarkViolet = 0xFF9400D3,
			DeepPink = 0xFFFF1493,
			DeepSkyBlue = 0xFF00BFFF,
			DimGray = 0xFF696969,
			DodgerBlue = 0xFF1E90FF,
			FireBrick = 0xFFB22222,
			FloralWhite = 0xFFFFFAF0,
			ForestGreen = 0xFF228B22,
			Fuchsia = 0xFFFF00FF,
			Gainsboro = 0xFFDCDCDC,
			GhostWhite = 0xFFF8F8FF,
			Gold = 0xFFFFD700,
			GoldenRod = 0xFFDAA520,
			Gray = 0xFF808080,
			Green = 0xFF008000,
			GreenYellow = 0xFFADFF2F,
			HoneyDew = 0xFFF0FFF0,
			HotPink = 0xFFFF69B4,
			IndianRed = 0xFFCD5C5C,
			Indigo = 0xFF4B0082,
			Ivory = 0xFFFFFFF0,
			Khaki = 0xFFF0E68C,
			Lavender = 0xFFE6E6FA,
			LavenderBlush = 0xFFFFF0F5,
			LawnGreen = 0xFF7CFC00,
			LemonChiffon = 0xFFFFFACD,
			LightBlue = 0xFFADD8E6,
			LightCoral = 0xFFF08080,
			LightCyan = 0xFFE0FFFF,
			LightGoldenRodYellow = 0xFFFAFAD2,
			LightGray = 0xFFD3D3D3,
			LightGreen = 0xFF90EE90,
			LightPink = 0xFFFFB6C1,
			LightSalmon = 0xFFFFA07A,
			LightSeaGreen = 0xFF20B2AA,
			LightSkyBlue = 0xFF87CEFA,
			LightSlateGray = 0xFF778899,
			LightSteelBlue = 0xFFB0C4DE,
			LightYellow = 0xFFFFFFE0,
			Lime = 0xFF00FF00,
			LimeGreen = 0xFF32CD32,
			Linen = 0xFFFAF0E6,
			Magenta = 0xFFFF00FF,
			Maroon = 0xFF800000,
			MediumAquaMarine = 0xFF66CDAA,
			MediumBlue = 0xFF0000CD,
			MediumOrchid = 0xFFBA55D3,
			MediumPurple = 0xFF9370DB,
			MediumSeaGreen = 0xFF3CB371,
			MediumSlateBlue = 0xFF7B68EE,
			MediumSpringGreen = 0xFF00FA9A,
			MediumTurquoise = 0xFF48D1CC,
			MediumVioletRed = 0xFFC71585,
			MidnightBlue = 0xFF191970,
			MintCream = 0xFFF5FFFA,
			MistyRose = 0xFFFFE4E1,
			Moccasin = 0xFFFFE4B5,
			NavajoWhite = 0xFFFFDEAD,
			Navy = 0xFF000080,
			OldLace = 0xFFFDF5E6,
			Olive = 0xFF808000,
			OliveDrab = 0xFF6B8E23,
			Orange = 0xFFFFA500,
			OrangeRed = 0xFFFF4500,
			Orchid = 0xFFDA70D6,
			PaleGoldenRod = 0xFFEEE8AA,
			PaleGreen = 0xFF98FB98,
			PaleTurquoise = 0xFFAFEEEE,
			PaleVioletRed = 0xFFDB7093,
			PapayaWhip = 0xFFFFEFD5,
			PeachPuff = 0xFFFFDAB9,
			Peru = 0xFFCD853F,
			Pink = 0xFFFFC0CB,
			Plum = 0xFFDDA0DD,
			PowderBlue = 0xFFB0E0E6,
			Purple = 0xFF800080,
			Red = 0xFFFF0000,
			RosyBrown = 0xFFBC8F8F,
			RoyalBlue = 0xFF4169E1,
			SaddleBrown = 0xFF8B4513,
			Salmon = 0xFFFA8072,
			SandyBrown = 0xFFF4A460,
			SeaGreen = 0xFF2E8B57,
			SeaShell = 0xFFFFF5EE,
			Sienna = 0xFFA0522D,
			Silver = 0xFFC0C0C0,
			SkyBlue = 0xFF87CEEB,
			SlateBlue = 0xFF6A5ACD,
			SlateGray = 0xFF708090,
			Snow = 0xFFFFFAFA,
			SpringGreen = 0xFF00FF7F,
			SteelBlue = 0xFF4682B4,
			Tan = 0xFFD2B48C,
			Teal = 0xFF008080,
			Thistle = 0xFFD8BFD8,
			Tomato = 0xFFFF6347,
			Turquoise = 0xFF40E0D0,
			Violet = 0xFFEE82EE,
			Wheat = 0xFFF5DEB3,
			White = 0xFFFFFFFF,
			WhiteSmoke = 0xFFF5F5F5,
			Yellow = 0xFFFFFF00,
			YellowGreen = 0xFF9ACD32,
		};
	};

	struct Filter
	{
		enum Type : uint32_t
		{
			None,
			
			// CPU
			AI,
			Animation, 
			Audio,
			Debug,
			Camera,
			Cloth,
			GameLogic,
			Input,
			Navigation,
			Network,
			Physics,
			Rendering,
			Scene,
			Script,
			Streaming,
			UI,
			VFX,
			Visibility,
			Wait,

			// IO
			IO,

			// GPU
			GPU_Cloth,
			GPU_Lighting,
			GPU_PostFX,
			GPU_Reflections,
			GPU_Scene,
			GPU_Shadows,
			GPU_UI,
			GPU_VFX,
			GPU_Water,

		};
	};

	#define OPTICK_MAKE_CATEGORY(filter, color) ((Optick::Category::Type)(((uint64_t)(1ull) << (filter + 32)) | (uint64_t)color))

	struct Category
	{
		enum Type : uint64_t
		{
			// CPU
			None			= OPTICK_MAKE_CATEGORY(Filter::None, Color::Null),
			AI				= OPTICK_MAKE_CATEGORY(Filter::AI, Color::Purple),
			Animation		= OPTICK_MAKE_CATEGORY(Filter::Animation, Color::LightSkyBlue),
			Audio			= OPTICK_MAKE_CATEGORY(Filter::Audio, Color::HotPink),
			Debug			= OPTICK_MAKE_CATEGORY(Filter::Debug, Color::Black),
			Camera			= OPTICK_MAKE_CATEGORY(Filter::Camera, Color::Black),
			Cloth			= OPTICK_MAKE_CATEGORY(Filter::Cloth, Color::DarkGreen),
			GameLogic		= OPTICK_MAKE_CATEGORY(Filter::GameLogic, Color::RoyalBlue),
			Input			= OPTICK_MAKE_CATEGORY(Filter::Input, Color::Ivory),
			Navigation		= OPTICK_MAKE_CATEGORY(Filter::Navigation, Color::Magenta),
			Network			= OPTICK_MAKE_CATEGORY(Filter::Network, Color::Olive),
			Physics			= OPTICK_MAKE_CATEGORY(Filter::Physics, Color::LawnGreen),
			Rendering		= OPTICK_MAKE_CATEGORY(Filter::Rendering, Color::BurlyWood),
			Scene			= OPTICK_MAKE_CATEGORY(Filter::Scene, Color::RoyalBlue),
			Script			= OPTICK_MAKE_CATEGORY(Filter::Script, Color::Plum),
			Streaming		= OPTICK_MAKE_CATEGORY(Filter::Streaming, Color::Gold),
			UI				= OPTICK_MAKE_CATEGORY(Filter::UI, Color::PaleTurquoise),
			VFX				= OPTICK_MAKE_CATEGORY(Filter::VFX, Color::SaddleBrown),
			Visibility		= OPTICK_MAKE_CATEGORY(Filter::Visibility, Color::Snow),
			Wait			= OPTICK_MAKE_CATEGORY(Filter::Wait, Color::Tomato),
			WaitEmpty		= OPTICK_MAKE_CATEGORY(Filter::Wait, Color::White),
			// IO
			IO				= OPTICK_MAKE_CATEGORY(Filter::IO, Color::Khaki),
			// GPU
			GPU_Cloth		= OPTICK_MAKE_CATEGORY(Filter::GPU_Cloth, Color::DarkGreen),
			GPU_Lighting	= OPTICK_MAKE_CATEGORY(Filter::GPU_Lighting, Color::Khaki),
			GPU_PostFX		= OPTICK_MAKE_CATEGORY(Filter::GPU_PostFX, Color::Maroon),
			GPU_Reflections = OPTICK_MAKE_CATEGORY(Filter::GPU_Reflections, Color::CadetBlue),
			GPU_Scene		= OPTICK_MAKE_CATEGORY(Filter::GPU_Scene, Color::RoyalBlue),
			GPU_Shadows		= OPTICK_MAKE_CATEGORY(Filter::GPU_Shadows, Color::LightSlateGray),
			GPU_UI			= OPTICK_MAKE_CATEGORY(Filter::GPU_UI, Color::PaleTurquoise),
			GPU_VFX			= OPTICK_MAKE_CATEGORY(Filter::GPU_VFX, Color::SaddleBrown),
			GPU_Water		= OPTICK_MAKE_CATEGORY(Filter::GPU_Water, Color::SteelBlue),
		};

		static uint32_t GetMask(Type t) { return (uint32_t)(t >> 32); }
		static uint32_t GetColor(Type t) { return (uint32_t)(t); }
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


namespace Optick
{
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct Mode
{
	enum Type
	{
		// OFF
		OFF = 0x0,
		// Collect Categories (top-level events)
		INSTRUMENTATION_CATEGORIES = (1 << 0),
		// Collect Events
		INSTRUMENTATION_EVENTS = (1 << 1),
		// Collect Events + Categories
		INSTRUMENTATION = (INSTRUMENTATION_CATEGORIES | INSTRUMENTATION_EVENTS),
		// Legacy (keep for compatibility reasons)
		SAMPLING = (1 << 2),
		// Collect Data Tags
		TAGS = (1 << 3),
		// Enable Autosampling Events (automatic callstacks)
		AUTOSAMPLING = (1 << 4),
		// Enable Switch-Contexts Events
		SWITCH_CONTEXT = (1 << 5),
		// Collect I/O Events
		IO = (1 << 6),
		// Collect GPU Events
		GPU = (1 << 7),
		END_SCREENSHOT = (1 << 8),
		RESERVED_0 = (1 << 9),
		RESERVED_1 = (1 << 10),
		// Collect HW Events
		HW_COUNTERS = (1 << 11),
		// Collect Events in Live mode
		LIVE = (1 << 12),
		RESERVED_2 = (1 << 13),
		RESERVED_3 = (1 << 14),
		RESERVED_4 = (1 << 15),
		// Collect System Calls
		SYS_CALLS = (1 << 16),
		// Collect Events from Other Processes
		OTHER_PROCESSES = (1 << 17),
		// Automation
		NOGUI = (1 << 18),

		TRACER = AUTOSAMPLING | SWITCH_CONTEXT | SYS_CALLS,
		DEFAULT = INSTRUMENTATION | TAGS | AUTOSAMPLING | SWITCH_CONTEXT | IO | GPU | SYS_CALLS | OTHER_PROCESSES,
	};
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct FrameType
{
	enum Type
	{
		CPU,
		GPU,
		Render,
		COUNT,

		NONE = -1,
	};
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OPTICK_API int64_t GetHighPrecisionTime();
OPTICK_API int64_t GetHighPrecisionFrequency();
OPTICK_API void Update();
OPTICK_API uint32_t BeginFrame(FrameType::Type type = FrameType::CPU, int64_t timestamp = -1, uint64_t threadID = (uint64_t)-1);
OPTICK_API uint32_t EndFrame(FrameType::Type type = FrameType::CPU, int64_t timestamp = -1, uint64_t threadID = (uint64_t)-1);
OPTICK_API bool IsActive(Mode::Type mode = Mode::INSTRUMENTATION_EVENTS);
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct EventStorage;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OPTICK_API bool RegisterFiber(uint64_t fiberId, EventStorage** slot);
OPTICK_API bool RegisterThread(const char* name);
OPTICK_API bool RegisterThread(const wchar_t* name);
OPTICK_API bool UnRegisterThread(bool keepAlive);
OPTICK_API EventStorage** GetEventStorageSlotForCurrentThread();
OPTICK_API bool IsFiberStorage(EventStorage* fiberStorage);
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct ThreadMask
{
	enum Type
	{
		None	= 0,
		Main	= 1 << 0,
		GPU		= 1 << 1,
		IO		= 1 << 2,
		Idle	= 1 << 3,
		Render  = 1 << 4,
	};
};

OPTICK_API EventStorage* RegisterStorage(const char* name, uint64_t threadID = uint64_t(-1), ThreadMask::Type type = ThreadMask::None);
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct State
{
	enum Type
	{
		// Starting a new capture
		START_CAPTURE,

		// Stopping current capture
		STOP_CAPTURE,

		// Dumping capture to the GUI
		// Useful for attaching summary and screenshot to the capture
		DUMP_CAPTURE,

		// Cancel current capture
		CANCEL_CAPTURE,
	};
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sets a state change callback
typedef bool (*StateCallback)(State::Type state);
OPTICK_API bool SetStateChangedCallback(StateCallback cb);

// Attaches a key-value pair to the capture's summary
// Example: AttachSummary("Version", "v12.0.1");
//			AttachSummary("Platform", "Windows");
//			AttachSummary("Config", "Release_x64");
//			AttachSummary("Settings", "Ultra");
//			AttachSummary("Map", "Atlantida");
//			AttachSummary("Position", "123.0,120.0,41.1");
//			AttachSummary("CPU", "Intel(R) Xeon(R) CPU E5410@2.33GHz");
//			AttachSummary("GPU", "NVIDIA GeForce GTX 980 Ti");
OPTICK_API bool AttachSummary(const char* key, const char* value);

struct File
{
	enum Type
	{
		// Supported formats: PNG, JPEG, BMP, TIFF
		OPTICK_IMAGE,
		
		// Text file
		OPTICK_TEXT,

		// Any other type
		OPTICK_OTHER,
	};
};
// Attaches a file to the current capture
OPTICK_API bool AttachFile(File::Type type, const char* name, const uint8_t* data, uint32_t size);
OPTICK_API bool AttachFile(File::Type type, const char* name, const char* path);
OPTICK_API bool AttachFile(File::Type type, const char* name, const wchar_t* path);
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct EventDescription;
struct Frame;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct EventTime
{
	static const int64_t INVALID_TIMESTAMP = (int64_t)-1;

	int64_t start;
	int64_t finish;

	OPTICK_INLINE void Start() { start  = Optick::GetHighPrecisionTime(); }
	OPTICK_INLINE void Stop() 	{ finish = Optick::GetHighPrecisionTime(); }
	OPTICK_INLINE bool IsValid() const { return start < finish && start != INVALID_TIMESTAMP && finish != INVALID_TIMESTAMP;  }
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct EventData : public EventTime
{
	const EventDescription* description;

	bool operator<(const EventData& other) const
	{
		if (start != other.start)
			return start < other.start;

		// Reversed order for finish intervals (parent first)
		return  finish > other.finish;
	}
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct OPTICK_API SyncData : public EventTime
{
	uint64_t newThreadId;
	uint64_t oldThreadId;
	uint8_t core;
	int8_t reason;
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct OPTICK_API FiberSyncData : public EventTime
{
	uint64_t threadId;

	static void AttachToThread(EventStorage* storage, uint64_t threadId);
	static void DetachFromThread(EventStorage* storage);
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<class T>
struct TagData
{
	const EventDescription* description;
	int64_t timestamp;
	T data;
	TagData() {}
	TagData(const EventDescription& desc, T d) : description(&desc), timestamp(Optick::GetHighPrecisionTime()), data(d) {}
	TagData(const EventDescription& desc, T d, int64_t t) : description(&desc), timestamp(t), data(d) {}
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct OPTICK_API EventDescription
{
	enum Flags : uint8_t
	{
		IS_CUSTOM_NAME = 1 << 0,
		COPY_NAME_STRING = 1 << 1,
		COPY_FILENAME_STRING = 1 << 2,
	};

	const char* name;
	const char* file;
	uint32_t line;
	uint32_t index;
	uint32_t color;
	uint32_t filter;
	uint8_t flags;

	static EventDescription* Create(const char* eventName, const char* fileName, const unsigned long fileLine, const unsigned long eventColor = Color::Null, const unsigned long filter = 0, const uint8_t eventFlags = 0);
	static EventDescription* CreateShared(const char* eventName, const char* fileName = nullptr, const unsigned long fileLine = 0, const unsigned long eventColor = Color::Null, const unsigned long filter = 0);

	EventDescription();
private:
	friend class EventDescriptionBoard;
	EventDescription& operator=(const EventDescription&);
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct OPTICK_API Event
{
	EventData* data;

	static EventData* Start(const EventDescription& description);
	static void Stop(EventData& data);

	static void Push(const char* name);
	static void Push(const EventDescription& description);
	static void Pop();

	static void Add(EventStorage* storage, const EventDescription* description, int64_t timestampStart, int64_t timestampFinish);
	static void Push(EventStorage* storage, const EventDescription* description, int64_t timestampStart);
	static void Pop(EventStorage* storage, int64_t timestampStart);


	Event(const EventDescription& description)
	{
		data = Start(description);
	}

	~Event()
	{
		if (data)
			Stop(*data);
	}
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OPTICK_INLINE Optick::EventDescription* CreateDescription(const char* functionName, const char* fileName, int fileLine, const char* eventName = nullptr, const ::Optick::Category::Type category = ::Optick::Category::None, uint8_t flags = 0)
{
	if (eventName != nullptr)
		flags |= ::Optick::EventDescription::IS_CUSTOM_NAME;

	return ::Optick::EventDescription::Create(eventName != nullptr ? eventName : functionName, fileName, (unsigned long)fileLine, ::Optick::Category::GetColor(category), ::Optick::Category::GetMask(category), flags);
}
OPTICK_INLINE Optick::EventDescription* CreateDescription(const char* functionName, const char* fileName, int fileLine, const ::Optick::Category::Type category)
{
	return ::Optick::EventDescription::Create(functionName, fileName, (unsigned long)fileLine, ::Optick::Category::GetColor(category), ::Optick::Category::GetMask(category));
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct OPTICK_API GPUEvent
{
	EventData* data;

	static EventData* Start(const EventDescription& description);
	static void Stop(EventData& data);

	GPUEvent(const EventDescription& description)
	{
		data = Start(description);
	}

	~GPUEvent()
	{
		if (data)
			Stop(*data);
	}
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct OPTICK_API Tag
{
	static void Attach(const EventDescription& description, float val);
	static void Attach(const EventDescription& description, int32_t val);
	static void Attach(const EventDescription& description, uint32_t val);
	static void Attach(const EventDescription& description, uint64_t val);
	static void Attach(const EventDescription& description, float val[3]);
	static void Attach(const EventDescription& description, const char* val);
	static void Attach(const EventDescription& description, const char* val, uint16_t length);

	// Derived
	static void Attach(const EventDescription& description, float x, float y, float z)
	{
		float p[3] = { x, y, z }; Attach(description, p);
	}

};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct ThreadScope
{
    ThreadScope(const char* name)
	{
		RegisterThread(name);
	}

	ThreadScope(const wchar_t* name)
	{
		RegisterThread(name);
	}

	~ThreadScope()
	{
		UnRegisterThread(false);
	}
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
enum OPTICK_API GPUQueueType
{
	GPU_QUEUE_GRAPHICS,
	GPU_QUEUE_COMPUTE,
	GPU_QUEUE_TRANSFER,
	GPU_QUEUE_VSYNC,

	GPU_QUEUE_COUNT,
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct OPTICK_API GPUContext
{
	void* cmdBuffer;
	GPUQueueType queue;
	int node;
	GPUContext(void* c = nullptr, GPUQueueType q = GPU_QUEUE_GRAPHICS, int n = 0) : cmdBuffer(c), queue(q), node(n) {}
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OPTICK_API void InitGpuD3D12(ID3D12Device* device, ID3D12CommandQueue** cmdQueues, uint32_t numQueues);
OPTICK_API void InitGpuVulkan(VkDevice* vkDevices, VkPhysicalDevice* vkPhysicalDevices, VkQueue* vkQueues, uint32_t* cmdQueuesFamily, uint32_t numQueues, const VulkanFunctions* functions);
OPTICK_API void GpuFlip(void* swapChain);
OPTICK_API GPUContext SetGpuContext(GPUContext context);
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct OPTICK_API GPUContextScope
{
	GPUContext prevContext;

	GPUContextScope(ID3D12CommandList* cmdList, GPUQueueType queue = GPU_QUEUE_GRAPHICS, int node = 0)
	{
		prevContext = SetGpuContext(GPUContext(cmdList, queue, node));
	}

	GPUContextScope(VkCommandBuffer cmdBuffer, GPUQueueType queue = GPU_QUEUE_GRAPHICS, int node = 0)
	{
		prevContext = SetGpuContext(GPUContext(cmdBuffer, queue, node));
	}

	~GPUContextScope()
	{
		SetGpuContext(prevContext);
	}
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OPTICK_API const EventDescription* GetFrameDescription(FrameType::Type frame = FrameType::CPU);
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef void* (*AllocateFn)(size_t);
typedef void  (*DeallocateFn)(void*);
typedef void  (*InitThreadCb)(void);
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OPTICK_API void SetAllocator(AllocateFn allocateFn, DeallocateFn deallocateFn, InitThreadCb initThreadCb);
OPTICK_API void Shutdown();
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef void(*CaptureSaveChunkCb)(const char*,size_t);
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OPTICK_API bool StartCapture(Mode::Type mode = Mode::DEFAULT, int samplingFrequency = 1000, bool force = true);
OPTICK_API bool StopCapture(bool force = true);
OPTICK_API bool SaveCapture(CaptureSaveChunkCb dataCb, bool force = true);
OPTICK_API bool SaveCapture(const char* path, bool force = true);
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct OptickApp
{
	const char* m_Name;
	OptickApp(const char* name) : m_Name(name) { StartCapture(); }
	~OptickApp() { StopCapture(); SaveCapture(m_Name); }
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}

#define OPTICK_UNUSED(x) (void)(x)
// Workaround for gcc compiler
#define OPTICK_VA_ARGS(...) , ##__VA_ARGS__

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Scoped profiling event which automatically grabs current function name.
// Use this macro 95% of the time.
// Example A:
//		void Function()
//		{
//			OPTICK_EVENT();
//			... code ...
//		}
//		or
//		void Function()
//		{
//			OPTICK_EVENT("CustomFunctionName");
//			... code ...
//		}
// Notes:
//		Optick captures full name of the function including name space and arguments.
//		Full name is usually shortened in the Optick GUI in order to highlight the most important bits.
#define OPTICK_EVENT(...)	 static ::Optick::EventDescription* OPTICK_CONCAT(autogen_description_, __LINE__) = nullptr; \
							 if (OPTICK_CONCAT(autogen_description_, __LINE__) == nullptr) OPTICK_CONCAT(autogen_description_, __LINE__) = ::Optick::CreateDescription(OPTICK_FUNC, __FILE__, __LINE__, ##__VA_ARGS__); \
							 ::Optick::Event OPTICK_CONCAT(autogen_event_, __LINE__)( *(OPTICK_CONCAT(autogen_description_, __LINE__)) ); 

// Backward compatibility with previous versions of Optick
//#if !defined(PROFILE)
//#define PROFILE OPTICK_EVENT()
//#endif

// Scoped profiling macro with predefined color.
// Use this macro for high-level function calls (e.g. AI, Physics, Audio, Render etc.).
// Example:
//		void UpdateAI()
//		{
//			OPTICK_CATEGORY("UpdateAI", Optick::Category::AI);
//			... code ...
//		}
//	
//		Macro could automatically capture current function name:
//		void UpdateAI()
//		{
//			OPTICK_CATEGORY(OPTICK_FUNC, Optick::Category::AI);
//			... code ...
//		}
#define OPTICK_CATEGORY(NAME, CATEGORY)	OPTICK_EVENT(NAME, CATEGORY)

// Profiling event for Main Loop update.
// You need to call this function in the beginning of the each new frame.
// Example:
//		while (true)
//		{
//			OPTICK_FRAME("MainThread");
//			... code ...
//		}
#define OPTICK_FRAME(FRAME_NAME, ...)	static ::Optick::ThreadScope mainThreadScope(FRAME_NAME);		\
										OPTICK_UNUSED(mainThreadScope);									\
										::Optick::EndFrame(__VA_ARGS__);								\
										::Optick::Update();												\
										uint32_t frameNumber = ::Optick::BeginFrame(__VA_ARGS__);		\
										::Optick::Event OPTICK_CONCAT(autogen_event_, __LINE__)(*::Optick::GetFrameDescription(__VA_ARGS__)); \
										OPTICK_TAG("Frame", frameNumber);

#define OPTICK_UPDATE()						::Optick::Update();
#define OPTICK_FRAME_FLIP(...)				::Optick::EndFrame(__VA_ARGS__); ::Optick::BeginFrame(__VA_ARGS__);

// Scoped event for categorized frame types.
// Example:
//	void UpdateFrame()
//	{
//		// Flip "Main/Update" frame
//		OPTICK_FRAME_EVENT(Optick::FrameType::CPU);
//
//		// Root category event 
//		OPTICK_CATEGORY("UpdateFrame", Optick::Category::GameLogic);
//
//		...
//  }
//
#define OPTICK_FRAME_EVENT(FRAME_TYPE, ...)		::Optick::EndFrame(FRAME_TYPE);									\
												switch (FRAME_TYPE) {											\
													case Optick::FrameType::CPU:								\
														::Optick::Update();										\
														break;													\
													default:													\
														break;													\
												}																\
												::Optick::BeginFrame(FRAME_TYPE);								\
												::Optick::Event OPTICK_CONCAT(autogen_event_, __LINE__)(*::Optick::GetFrameDescription(FRAME_TYPE));


// Thread registration macro.
// Example:
//		void WorkerThread(...)
//		{
//			OPTICK_THREAD("Worker");
//			while (isRunning)
//			{
//				...
//			}
//		}
#define OPTICK_THREAD(THREAD_NAME) ::Optick::ThreadScope brofilerThreadScope(THREAD_NAME);	\
									 OPTICK_UNUSED(brofilerThreadScope);					\


// Thread registration macros.
// Useful for integration with custom job-managers.
#define OPTICK_START_THREAD(THREAD_NAME) ::Optick::RegisterThread(THREAD_NAME);
#define OPTICK_STOP_THREAD() ::Optick::UnRegisterThread(false);

// Attaches a custom data-tag.
// Supported types: int32, uint32, uint64, vec3, string (cut to 32 characters)
// Example:
//		OPTICK_TAG("PlayerName", name[index]);
//		OPTICK_TAG("Health", 100);
//		OPTICK_TAG("Score", 0x80000000u);
//		OPTICK_TAG("Height(cm)", 176.3f);
//		OPTICK_TAG("Address", (uint64)*this);
//		OPTICK_TAG("Position", 123.0f, 456.0f, 789.0f);
#define OPTICK_TAG(NAME, ...)		static ::Optick::EventDescription* OPTICK_CONCAT(autogen_tag_, __LINE__) = nullptr; \
									if (OPTICK_CONCAT(autogen_tag_, __LINE__) == nullptr) OPTICK_CONCAT(autogen_tag_, __LINE__) = ::Optick::EventDescription::Create( NAME, __FILE__, __LINE__ ); \
									::Optick::Tag::Attach(*OPTICK_CONCAT(autogen_tag_, __LINE__), __VA_ARGS__); \

// Scoped macro with DYNAMIC name.
// Optick holds a copy of the provided name.
// Each scope does a search in hashmap for the name.
// Please use variations with STATIC names where it's possible.
// Use this macro for quick prototyping or intergratoin with other profiling systems (e.g. UE4)
// Example:
//		const char* name = ... ;
//		OPTICK_EVENT_DYNAMIC(name);
#define OPTICK_EVENT_DYNAMIC(NAME)	OPTICK_CUSTOM_EVENT(::Optick::EventDescription::CreateShared(NAME, __FILE__, __LINE__));
// Push\Pop profiling macro with DYNAMIC name.
#define OPTICK_PUSH_DYNAMIC(NAME)		::Optick::Event::Push(NAME);		

// Push\Pop profiling macro with STATIC name.
// Please avoid using Push\Pop approach in favor for scoped macros.
// For backward compatibility with some engines.
// Example:
//		OPTICK_PUSH("ScopeName");
//		...
//		OPTICK_POP();
#define OPTICK_PUSH(NAME)				static ::Optick::EventDescription* OPTICK_CONCAT(autogen_description_, __LINE__) = nullptr; \
										if (OPTICK_CONCAT(autogen_description_, __LINE__) == nullptr) OPTICK_CONCAT(autogen_description_, __LINE__) = ::Optick::EventDescription::Create( NAME, __FILE__, __LINE__ ); \
										::Optick::Event::Push(*OPTICK_CONCAT(autogen_description_, __LINE__));		
#define OPTICK_POP()					::Optick::Event::Pop();


// Scoped macro with predefined Optick::EventDescription.
// Use these events instead of DYNAMIC macros to minimize overhead.
// Common use-case: integrating Optick with internal script languages (e.g. Lua, Actionscript(Scaleform), etc.).
// Example:
//		Generating EventDescription once during initialization:
//		Optick::EventDescription* description = Optick::EventDescription::CreateShared("FunctionName");
//
//		Then we could just use a pointer to cached description later for profiling:
//		OPTICK_CUSTOM_EVENT(description);
#define OPTICK_CUSTOM_EVENT(DESCRIPTION) 							::Optick::Event						  OPTICK_CONCAT(autogen_event_, __LINE__)( *DESCRIPTION ); \

// Registration of a custom EventStorage (e.g. GPU, IO, etc.)
// Use it to present any extra information on the timeline.
// Example:
//		Optick::EventStorage* IOStorage = Optick::RegisterStorage("I/O");
// Notes:
//		Registration of a new storage is thread-safe.
#define OPTICK_STORAGE_REGISTER(STORAGE_NAME)														::Optick::RegisterStorage(STORAGE_NAME);

// Adding events to the custom storage.
// Helps to integrate Optick into already existing profiling systems (e.g. GPU Profiler, I/O profiler, etc.).
// Example:
//			//Registering a storage - should be done once during initialization
//			static Optick::EventStorage* IOStorage = Optick::RegisterStorage("I/O");
//
//			int64_t cpuTimestampStart = Optick::GetHighPrecisionTime();
//			...
//			int64_t cpuTimestampFinish = Optick::GetHighPrecisionTime();
//
//			//Creating a shared event-description
//			static Optick::EventDescription* IORead = Optick::EventDescription::CreateShared("IO Read");
// 
//			OPTICK_STORAGE_EVENT(IOStorage, IORead, cpuTimestampStart, cpuTimestampFinish);
// Notes:
//		It's not thread-safe to add events to the same storage from multiple threads.
//		Please guarantee thread-safety on the higher level if access from multiple threads to the same storage is required.
#define OPTICK_STORAGE_EVENT(STORAGE, DESCRIPTION, CPU_TIMESTAMP_START, CPU_TIMESTAMP_FINISH)	if (::Optick::IsActive()) { ::Optick::Event::Add(STORAGE, DESCRIPTION, CPU_TIMESTAMP_START, CPU_TIMESTAMP_FINISH); }
#define OPTICK_STORAGE_PUSH(STORAGE, DESCRIPTION, CPU_TIMESTAMP_START)							if (::Optick::IsActive()) { ::Optick::Event::Push(STORAGE, DESCRIPTION, CPU_TIMESTAMP_START); }
#define OPTICK_STORAGE_POP(STORAGE, CPU_TIMESTAMP_FINISH)										if (::Optick::IsActive()) { ::Optick::Event::Pop(STORAGE, CPU_TIMESTAMP_FINISH); }


// Registers state change callback
// If callback returns false - the call is repeated the next frame
#define OPTICK_SET_STATE_CHANGED_CALLBACK(CALLBACK)			::Optick::SetStateChangedCallback(CALLBACK);


// Registers custom memory allocator within Optick core
// Example:
//		OPTICK_SET_MEMORY_ALLOCATOR([](size_t size) -> void* { return operator new(size); }, [](void* p) { operator delete(p); }, nullptr);
// Params:
//		INIT_THREAD_CALLBACK - callback for internal Optick threads (useful if you need to setup some TLS variables related to the memory allocator for your thread)
// Notes:
//		Should be called before the first call to OPTICK_FRAME
//		Allocation and deallocation functions should be thread-safe - Optick doesn't do any synchronization for these calls
#define OPTICK_SET_MEMORY_ALLOCATOR(ALLOCATE_FUNCTION, DEALLOCATE_FUNCTION, INIT_THREAD_CALLBACK)				::Optick::SetAllocator(ALLOCATE_FUNCTION, DEALLOCATE_FUNCTION, INIT_THREAD_CALLBACK);

// Shutdown
// Clears all the internal buffers allocated by Optick
// Notes:
//		You shouldn't call any Optick functions after shutting down the system (it might lead to the undefined behaviour)
#define OPTICK_SHUTDOWN()																						::Optick::Shutdown();

// GPU events
#define OPTICK_GPU_INIT_D3D12(DEVICE, CMD_QUEUES, NUM_CMD_QUEUS)													::Optick::InitGpuD3D12(DEVICE, CMD_QUEUES, NUM_CMD_QUEUS);
#define OPTICK_GPU_INIT_VULKAN(DEVICES, PHYSICAL_DEVICES, CMD_QUEUES, CMD_QUEUES_FAMILY, NUM_CMD_QUEUS, FUNCTIONS)	::Optick::InitGpuVulkan(DEVICES, PHYSICAL_DEVICES, CMD_QUEUES, CMD_QUEUES_FAMILY, NUM_CMD_QUEUS, FUNCTIONS);

// Setup GPU context:
// Params:
//		(CommandBuffer\CommandList, [Optional] Optick::GPUQueue queue, [Optional] int NodeIndex)
// Examples:
//		OPTICK_GPU_CONTEXT(cmdBuffer); - all OPTICK_GPU_EVENT will use the same command buffer within the scope
//		OPTICK_GPU_CONTEXT(cmdBuffer, Optick::GPU_QUEUE_COMPUTE); - all events will use the same command buffer and queue for the scope 
//		OPTICK_GPU_CONTEXT(cmdBuffer, Optick::GPU_QUEUE_COMPUTE, gpuIndex); - all events will use the same command buffer and queue for the scope 
#define OPTICK_GPU_CONTEXT(...)	 ::Optick::GPUContextScope OPTICK_CONCAT(gpu_autogen_context_, __LINE__)(__VA_ARGS__); \
									 (void)OPTICK_CONCAT(gpu_autogen_context_, __LINE__);

#define OPTICK_GPU_EVENT(NAME)	 OPTICK_EVENT(NAME); \
									 static ::Optick::EventDescription* OPTICK_CONCAT(gpu_autogen_description_, __LINE__) = nullptr; \
									 if (OPTICK_CONCAT(gpu_autogen_description_, __LINE__) == nullptr) OPTICK_CONCAT(gpu_autogen_description_, __LINE__) = ::Optick::EventDescription::Create( NAME, __FILE__, __LINE__ ); \
									 ::Optick::GPUEvent OPTICK_CONCAT(gpu_autogen_event_, __LINE__)( *(OPTICK_CONCAT(gpu_autogen_description_, __LINE__)) ); \

#define OPTICK_GPU_FLIP(SWAP_CHAIN)		::Optick::GpuFlip(SWAP_CHAIN);

/////////////////////////////////////////////////////////////////////////////////
// [Automation][Startup]
/////////////////////////////////////////////////////////////////////////////////

// Starts a new capture
// Params:
//		[Optional] Mode::Type mode /*= Mode::DEFAULT*/
//		[Optional] int samplingFrequency /*= 1000*/
#define OPTICK_START_CAPTURE(...)				::Optick::StartCapture(__VA_ARGS__);

// Stops a new capture (Keeps data intact in the local buffers)
#define OPTICK_STOP_CAPTURE(...)				::Optick::StopCapture(__VA_ARGS__);

// Saves capture
// Params:
//		const char* FilePath - path to the capture
// or
//		CaptureSaveChunkCb dataCb - callback for saving chunks of data
// Example:
//		OPTICK_SAVE_CAPTURE("ConsoleApp.opt");
#define OPTICK_SAVE_CAPTURE(...)				::Optick::SaveCapture(__VA_ARGS__);

// Generate a capture for the whole scope
// Params:
//		NAME - name of the application
// Examples:
//		int main() {
//			OPTICK_APP("MyGame"); //Optick will automatically save a capture in the working directory with the name "MyGame(2019-09-08.14-30-19).opt"	
//			...
//		}
#define OPTICK_APP(NAME)			OPTICK_THREAD(NAME); \
									::Optick::OptickApp _optickApp(NAME); \
									OPTICK_UNUSED(_optickApp);


#else
#define OPTICK_EVENT(...)
#define OPTICK_CATEGORY(NAME, CATEGORY)
#define OPTICK_FRAME(NAME)
#define OPTICK_THREAD(THREAD_NAME)
#define OPTICK_START_THREAD(THREAD_NAME)
#define OPTICK_STOP_THREAD()
#define OPTICK_TAG(NAME, DATA)
#define OPTICK_EVENT_DYNAMIC(NAME)	
#define OPTICK_PUSH_DYNAMIC(NAME)		
#define OPTICK_PUSH(NAME)				
#define OPTICK_POP()		
#define OPTICK_CUSTOM_EVENT(DESCRIPTION)
#define OPTICK_STORAGE_REGISTER(STORAGE_NAME)
#define OPTICK_STORAGE_EVENT(STORAGE, DESCRIPTION, CPU_TIMESTAMP_START, CPU_TIMESTAMP_FINISH)
#define OPTICK_STORAGE_PUSH(STORAGE, DESCRIPTION, CPU_TIMESTAMP_START)
#define OPTICK_STORAGE_POP(STORAGE, CPU_TIMESTAMP_FINISH)				
#define OPTICK_SET_STATE_CHANGED_CALLBACK(CALLBACK)
#define OPTICK_SET_MEMORY_ALLOCATOR(ALLOCATE_FUNCTION, DEALLOCATE_FUNCTION)	
#define OPTICK_SHUTDOWN()
#define OPTICK_GPU_INIT_D3D12(DEVICE, CMD_QUEUES, NUM_CMD_QUEUS)
#define OPTICK_GPU_INIT_VULKAN(DEVICES, PHYSICAL_DEVICES, CMD_QUEUES, CMD_QUEUES_FAMILY, NUM_CMD_QUEUS, FUNCTIONS)
#define OPTICK_GPU_CONTEXT(...)
#define OPTICK_GPU_EVENT(NAME)
#define OPTICK_GPU_FLIP(SWAP_CHAIN)
#define OPTICK_UPDATE()
#define OPTICK_FRAME_FLIP(...)
#define OPTICK_FRAME_EVENT(FRAME_TYPE, ...)
#define OPTICK_START_CAPTURE(...)
#define OPTICK_STOP_CAPTURE()
#define OPTICK_SAVE_CAPTURE(...)
#define OPTICK_APP(NAME)
#endif
