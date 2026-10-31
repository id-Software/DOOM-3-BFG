/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_thread.c - see rgd3_thread.h. */

static SDL_ThreadID rgd3_thread_main_id;
static const char* rgd3_thread_main_stack_lo;
static const char* rgd3_thread_main_stack_hi;

void rgd3_thread_init_main(void)
{
	rgd3_thread_main_id = SDL_GetCurrentThreadID();
#if defined(_WIN32)
	{
		ULONG_PTR lo = 0, hi = 0;
		GetCurrentThreadStackLimits(&lo, &hi);
		rgd3_thread_main_stack_lo = (const char*)lo;
		rgd3_thread_main_stack_hi = (const char*)hi;
	}
#endif
}

/* whether p lies in the main thread's stack reservation; without known
   limits (non-Windows for now) every main-thread pointer counts */
int rgd3_thread_on_main_stack(const void* p)
{
	if (!rgd3_thread_main_stack_hi)
		return 1;
	return (const char*)p >= rgd3_thread_main_stack_lo && (const char*)p < rgd3_thread_main_stack_hi;
}

int rgd3_thread_is_main(void)
{
	return SDL_GetCurrentThreadID() == rgd3_thread_main_id;
}

int rgd3_thread_start_detached(const char* name, Rgd3ThreadFn fn, void* user)
{
	SDL_Thread* thread = SDL_CreateThread(fn, name, user);
	if (!thread)
	{
		rgd3_warning("could not create thread %s: %s", name, SDL_GetError());
		return 0;
	}
	SDL_DetachThread(thread);
	return 1;
}
