/*
===========================================================================

Doom 3 BFG Edition GPL Source Code

===========================================================================
*/

#pragma hdrstop
#include "../../idlib/precompiled.h"
#include "win_local.h"

/*
==============================================================

        Clock ticks

==============================================================
*/

double Sys_GetClockTicks() {
#if defined(__GNUC__) && defined(__x86_64__)
    unsigned int lo, hi;
    __asm__ __volatile__ (
        "cpuid\n\t"
        "rdtsc\n\t"
        "mov %%eax, %0\n\t"
        "mov %%edx, %1"
        : "=r" (lo), "=r" (hi)
        :: "eax", "ebx", "ecx", "edx");
    return (double)lo + (double)0xFFFFFFFF * hi;
#else
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    return (double)li.QuadPart;
#endif
}

double Sys_ClockTicksPerSecond() {
    static double ticks = 0;
    if (!ticks) {
        LARGE_INTEGER li;
        QueryPerformanceFrequency(&li);
        ticks = (double)li.QuadPart;
    }
    return ticks;
}

int Sys_CPUCount() {
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return si.dwNumberOfProcessors;
}

/*
==============================================================

        CPU Detection

==============================================================
*/

static bool HasMMX() {
#if defined(__GNUC__) && defined(__x86_64__)
    return __builtin_cpu_supports("mmx");
#else
    return true;
#endif
}

static bool HasSSE() {
#if defined(__GNUC__) && defined(__x86_64__)
    return __builtin_cpu_supports("sse");
#else
    return true;
#endif
}

static bool HasSSE2() {
#if defined(__GNUC__) && defined(__x86_64__)
    return __builtin_cpu_supports("sse2");
#else
    return true;
#endif
}

static bool HasSSE3() {
#if defined(__GNUC__) && defined(__x86_64__)
    return __builtin_cpu_supports("sse3");
#else
    return false;
#endif
}

cpuid_t Sys_GetCPUId() {
    cpuid_t cpuid = CPUID_NONE;
    
    if (HasMMX()) cpuid = (cpuid_t)(cpuid | CPUID_MMX);
    if (HasSSE()) cpuid = (cpuid_t)(cpuid | CPUID_SSE);
    if (HasSSE2()) cpuid = (cpuid_t)(cpuid | CPUID_SSE2);
    if (HasSSE3()) cpuid = (cpuid_t)(cpuid | CPUID_SSE3);
    
    return cpuid;
}

/*
==============================================================

        FPU State

==============================================================
*/

void Sys_FPU_ClearStack() {
    // No-op stub for cross-compilation
}

void Sys_FPU_GetState(char *state) {
    memset(state, 0, 512);
}

bool Sys_FPU_StackIsEmpty() {
    return true;
}

const char * Sys_FPU_GetStateStr() {
    static char buf[256];
    memset(buf, 0, sizeof(buf));
    return buf;
}

int Sys_FPU_PrintStateFlags(char *ptr, int ctrl, int stat, int tags, int inof, int inse, int opof, int opse) {
    return 0;
}

void Sys_FPU_SetPrecision() {
    // No-op stub
}

void Sys_FPU_SetDAZ(bool enable) {
    // No-op stub for cross-compilation
    (void)enable;
}

void Sys_FPU_SetFTZ(bool enable) {
    // No-op stub for cross-compilation
    (void)enable;
}
