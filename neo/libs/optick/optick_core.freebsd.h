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
#if defined(__FreeBSD__)

#include "optick.config.h"
#if USE_OPTICK

#include "optick_core.platform.h"

#include <sys/time.h>
#include <sys/types.h>
#include <pthread.h>
#include <unistd.h>

namespace Optick
{
	const char* Platform::GetName()
	{
		return "PS4";
	}

	ThreadID Platform::GetThreadID()
	{
		return (uint64_t)pthread_self();
	}

	ProcessID Platform::GetProcessID()
	{
		return (ProcessID)getpid();
	}

	int64 Platform::GetFrequency()
	{
		return 1000000000;
	}

	int64 Platform::GetTime()
	{
		struct timespec ts;
		clock_gettime(CLOCK_REALTIME, &ts);
		return ts.tv_sec * 1000000000LL + ts.tv_nsec;
	}

	Trace* Platform::CreateTrace()
	{
		return nullptr;
	}

	SymbolEngine* Platform::CreateSymbolEngine()
	{
		return nullptr;
	}
}

#endif //USE_OPTICK
#endif //__FreeBSD__