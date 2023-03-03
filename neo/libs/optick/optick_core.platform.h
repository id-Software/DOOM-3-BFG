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

#include "optick_common.h"
#include "optick_memory.h"

//////////////////////////////////////////////////////////////////////////
// Platform-specific stuff
//////////////////////////////////////////////////////////////////////////
namespace Optick
{
	struct Trace;
	struct Module;
	struct Symbol;
	struct SymbolEngine;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Platform API
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct Platform
	{
		// Platform Name
		static OPTICK_INLINE const char* GetName();
		// Thread ID (system thread id)
		static OPTICK_INLINE ThreadID GetThreadID();
		// Process ID
		static OPTICK_INLINE ProcessID GetProcessID();
		// CPU Frequency
		static OPTICK_INLINE int64 GetFrequency();
		// CPU Time (Ticks)
		static OPTICK_INLINE int64 GetTime();
		// System Tracer
		static OPTICK_INLINE Trace* CreateTrace();
		// Symbol Resolver
		static OPTICK_INLINE SymbolEngine* CreateSymbolEngine();
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Tracing API
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct Trace
	{
		virtual void SetPassword(const char* /*pwd*/) {};
		virtual CaptureStatus::Type Start(Mode::Type mode, int frequency, const ThreadList& threads) = 0;
		virtual bool Stop() = 0;
		virtual ~Trace() {};
	};


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Symbol API
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct Module
	{
		string path;
		void* address;
		size_t size;
		Module(const char* p, void* a, size_t s) : path(p), address(a), size(s) {}
	};
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct Symbol
	{
		uint64 address;
		uint64 offset;
		wstring file;
		wstring function;
		uint32 line;
		Symbol()
			: address(0)
			, offset(0)
			, line(0)
		{}
	};
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct SymbolEngine
	{
		// Get list of loaded modules
		virtual const vector<Module>& GetModules() = 0;

		// Get Symbol from address
		virtual const Symbol* GetSymbol(uint64 dwAddress) = 0;

		virtual ~SymbolEngine() {};
	};
}
//////////////////////////////////////////////////////////////////////////

#endif //USE_OPTICK