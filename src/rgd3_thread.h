/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_thread.h - thin SDL3 threading wrappers. The worker-thread and job
   system (idSysWorkerThread / idParallelJobList semantics) arrive in M9. */
#ifndef RGD3_THREAD_H
#define RGD3_THREAD_H
#ifndef RGD3_ALL_C
#error "rgd3 headers are included through src/all.c only"
#endif

typedef int (*Rgd3ThreadFn)(void* user);

void rgd3_thread_init_main(void);
int rgd3_thread_is_main(void);
int rgd3_thread_on_main_stack(const void* p); /* p is inside the main thread's stack */
int rgd3_thread_start_detached(const char* name, Rgd3ThreadFn fn, void* user);

#endif
