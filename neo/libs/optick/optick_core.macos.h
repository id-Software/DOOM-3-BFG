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
#if defined(__APPLE_CC__)

#include "optick.config.h"
#if USE_OPTICK

#include "optick_core.platform.h"

#include <mach/mach_time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <pthread.h>
#include <unistd.h>

namespace Optick
{
	const char* Platform::GetName()
	{
		return "MacOS";
	}

	ThreadID Platform::GetThreadID()
	{
		uint64_t tid;
		pthread_threadid_np(pthread_self(), &tid);
		return tid;
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
}

#if OPTICK_ENABLE_TRACING

#include "optick_core.h"

namespace Optick
{
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class DTrace : public Trace
{
	static const bool isSilent = true;

	std::thread processThread;
	string password;

	enum State
	{
		STATE_IDLE,
		STATE_RUNNING,
		STATE_ABORT,
	};

	volatile State state;
	volatile int64 timeout;

	struct CoreState
	{
		ProcessID pid;
		ThreadID tid;
		int prio;
		bool IsValid() const { return tid != INVALID_THREAD_ID; }
		CoreState() : pid(INVALID_PROCESS_ID), tid(INVALID_THREAD_ID), prio(0) {}
	};
	static const int MAX_CPU_CORES = 256;
	array<CoreState, MAX_CPU_CORES> cores;

	static void AsyncProcess(DTrace* trace);
	void Process();

	bool CheckRootAccess();

	enum ParseResult
	{
		PARSE_OK,
		PARSE_TIMEOUT,
		PARSE_FAILED,
	};
	ParseResult Parse(const char* line);
public:

	DTrace();

	virtual void SetPassword(const char* pwd) override { password = pwd; }
	virtual CaptureStatus::Type Start(Mode::Type mode, int frequency, const ThreadList& threads) override;
	virtual bool Stop() override;
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DTrace::DTrace() : state(STATE_IDLE), timeout(0)
{
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool DTrace::CheckRootAccess()
{
	char cmd[256] = { 0 };
	sprintf_s(cmd, "echo \'%s\' | sudo -S echo %s", password.c_str(), isSilent ? "2> /dev/null" : "");
	return system(cmd) == 0;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CaptureStatus::Type DTrace::Start(Mode::Type mode, int /*frequency*/, const ThreadList& /*threads*/)
{
	if (state == STATE_IDLE && (mode & Mode::SWITCH_CONTEXT) != 0)
	{
		if (!CheckRootAccess())
			return CaptureStatus::ERR_TRACER_INVALID_PASSWORD;

		state = STATE_RUNNING;
		timeout = INT64_MAX;
		cores.fill(CoreState());
		processThread = std::thread(AsyncProcess, this);
	}

	return CaptureStatus::OK;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool DTrace::Stop()
{
	if (state != STATE_RUNNING)
	{
		return false;
	}

	timeout = Platform::GetTime();
	processThread.join();
	state = STATE_IDLE;

	return true;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FILE* popen2(const char *program, const char *type, pid_t* outPid)
{
	FILE *iop;
	int pdes[2];
	pid_t pid;
	if ((*type != 'r' && *type != 'w') || type[1] != '\0') {
		errno = EINVAL;
		return (NULL);
	}

	if (pipe(pdes) < 0) {
		return (NULL);
	}

	switch (pid = fork()) {
	case -1:            /* Error. */
		(void)close(pdes[0]);
		(void)close(pdes[1]);
		return (NULL);
		/* NOTREACHED */
	case 0:                /* Child. */
	{
		if (*type == 'r') {
			(void)close(pdes[0]);
			if (pdes[1] != STDOUT_FILENO) {
				(void)dup2(pdes[1], STDOUT_FILENO);
				(void)close(pdes[1]);
			}
		}
		else {
			(void)close(pdes[1]);
			if (pdes[0] != STDIN_FILENO) {
				(void)dup2(pdes[0], STDIN_FILENO);
				(void)close(pdes[0]);
			}
		}
		execl("/bin/sh", "sh", "-c", program, NULL);
		perror("execl");
		exit(1);
		/* NOTREACHED */
	}
	}
	/* Parent; assume fdopen can't fail. */
	if (*type == 'r') {
		iop = fdopen(pdes[0], type);
		(void)close(pdes[1]);
	}
	else {
		iop = fdopen(pdes[1], type);
		(void)close(pdes[0]);
	}

	if (outPid)
		*outPid = pid;

	return (iop);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void DTrace::Process()
{
	const char* command = "dtrace -n fbt::thread_dispatch:return'\\''{printf(\"@%d %d %d %d\", pid, tid, curthread->sched_pri, walltimestamp)}'\\''";

	char buffer[256] = { 0 };
	sprintf_s(buffer, "echo \'%s\' | sudo -S sh -c \'%s\' %s", password.c_str(), command, isSilent ? "2> /dev/null" : "");
	pid_t pid;
	if (FILE* pipe = popen2(buffer, "r", &pid))
	{
		char* line = NULL;
		size_t len = 0;
		while (state == STATE_RUNNING && (getline(&line, &len, pipe)) != -1)
		{
			if (Parse(line) == PARSE_TIMEOUT)
				break;
		}
		fclose(pipe);

		int internal_stat;
		waitpid(pid, &internal_stat, 0);
	}
	else
	{
		OPTICK_FAILED("Failed to open communication pipe!");
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DTrace::ParseResult DTrace::Parse(const char* line)
{
	if (const char* cmd = strchr(line, '@'))
	{
		int cpu = atoi(line);

		CoreState currState;

		currState.pid = atoi(cmd + 1);
		cmd = strchr(cmd, ' ') + 1;

		currState.tid = atoi(cmd);
		cmd = strchr(cmd, ' ') + 1;

		currState.prio = atoi(cmd);
		cmd = strchr(cmd, ' ') + 1;

		int64_t timestamp = (int64_t)atoll(cmd);

		if (timestamp > timeout)
			return PARSE_TIMEOUT;

		const CoreState& prevState = cores[cpu];

		if (prevState.IsValid())
		{
			SwitchContextDesc desc;
			desc.reason = 0;
			desc.cpuId = cpu;
			desc.oldThreadId = prevState.tid;
			desc.newThreadId = currState.tid;
			desc.timestamp = timestamp;
			Core::Get().ReportSwitchContext(desc);
		}

		cores[cpu] = currState;
	}
	return PARSE_FAILED;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void DTrace::AsyncProcess(DTrace *trace) {
	trace->Process();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Trace* Platform::CreateTrace()
{
	return Memory::New<DTrace>();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SymbolEngine* Platform::CreateSymbolEngine()
{
	return nullptr;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}
#endif //OPTICK_ENABLE_TRACING
#endif //USE_OPTICK
#endif //__APPLE_CC__