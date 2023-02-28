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

#include "optick.h"

#include <cstdio>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#if defined(OPTICK_MSVC)

#ifdef OPTICK_UE4
#include "Core/Public/Windows/AllowWindowsPlatformTypes.h"
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#ifdef OPTICK_UE4
#include "Core/Public/Windows/HideWindowsPlatformTypes.h"
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#endif

namespace Optick
{
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Types
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef signed char int8;
typedef unsigned char uint8;
typedef unsigned char byte;
typedef short int16;
typedef unsigned short uint16;
typedef int int32;
typedef unsigned int uint32;
#if defined(OPTICK_MSVC)
typedef __int64 int64;
typedef unsigned __int64 uint64;
#elif defined(OPTICK_GCC)
typedef int64_t int64;
typedef uint64_t uint64;
#else
#error Compiler is not supported
#endif
static_assert(sizeof(int8) == 1, "Invalid type size, int8");
static_assert(sizeof(uint8) == 1, "Invalid type size, uint8");
static_assert(sizeof(byte) == 1, "Invalid type size, byte");
static_assert(sizeof(int16) == 2, "Invalid type size, int16");
static_assert(sizeof(uint16) == 2, "Invalid type size, uint16");
static_assert(sizeof(int32) == 4, "Invalid type size, int32");
static_assert(sizeof(uint32) == 4, "Invalid type size, uint32");
static_assert(sizeof(int64) == 8, "Invalid type size, int64");
static_assert(sizeof(uint64) == 8, "Invalid type size, uint64");
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef uint64 ThreadID;
static const ThreadID INVALID_THREAD_ID = (ThreadID)-1;
typedef uint32 ProcessID;
static const ProcessID INVALID_PROCESS_ID = (ProcessID)-1;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Memory
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#if defined(OPTICK_MSVC)
#define OPTICK_ALIGN(N) __declspec( align( N ) )
#elif defined(OPTICK_GCC)
#define OPTICK_ALIGN(N) __attribute__((aligned(N)))
#else
#error Can not define OPTICK_ALIGN. Unknown platform.
#endif
#define OPTICK_ARRAY_SIZE(ARR) (sizeof(ARR)/sizeof((ARR)[0]))
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#if defined(OPTICK_MSVC)
#define OPTICK_NOINLINE __declspec(noinline)
#elif defined(OPTICK_GCC)
#define OPTICK_NOINLINE __attribute__((__noinline__))
#else
#error Compiler is not supported
#endif
////////////////////////////////////////////////////////////////////////
// OPTICK_THREAD_LOCAL
////////////////////////////////////////////////////////////////////////
#if defined(OPTICK_MSVC)
#define OPTICK_THREAD_LOCAL __declspec(thread)
#elif defined(OPTICK_GCC)
#define OPTICK_THREAD_LOCAL __thread
#else
#error Can not define OPTICK_THREAD_LOCAL. Unknown platform.
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Asserts
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#if defined(OPTICK_MSVC)
#define OPTICK_DEBUG_BREAK __debugbreak()
#elif defined(OPTICK_GCC)
#define OPTICK_DEBUG_BREAK __builtin_trap()
#else
	#error Can not define OPTICK_DEBUG_BREAK. Unknown platform.
#endif
#define OPTICK_UNUSED(x) (void)(x)
#ifdef _DEBUG
	#define OPTICK_ASSERT(arg, description) if (!(arg)) { OPTICK_DEBUG_BREAK; }
	#define OPTICK_FAILED(description) { OPTICK_DEBUG_BREAK; }
#else
	#define OPTICK_ASSERT(arg, description)
	#define OPTICK_FAILED(description)
#endif
#define OPTICK_VERIFY(arg, description, operation) if (!(arg)) { OPTICK_DEBUG_BREAK; operation; }
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Safe functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#if defined(OPTICK_LINUX) || defined(OPTICK_OSX)
template<size_t sizeOfBuffer>
inline int sprintf_s(char(&buffer)[sizeOfBuffer], const char* format, ...)
{
	va_list ap;
	va_start(ap, format);
	int result = vsnprintf(buffer, sizeOfBuffer, format, ap);
	va_end(ap);
	return result;
}
#endif

#if defined(OPTICK_GCC)
#include <string.h>
template<size_t sizeOfBuffer>
inline int wcstombs_s(char(&buffer)[sizeOfBuffer], const wchar_t* src, size_t maxCount)
{
	return wcstombs(buffer, src, maxCount);
}
#endif

#if defined(OPTICK_MSVC)
template<size_t sizeOfBuffer>
inline int wcstombs_s(char(&buffer)[sizeOfBuffer], const wchar_t* src, size_t maxCount)
{
	size_t converted = 0;
	return ::wcstombs_s(&converted, buffer, src, maxCount);
}
#endif
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#endif //USE_OPTICK
