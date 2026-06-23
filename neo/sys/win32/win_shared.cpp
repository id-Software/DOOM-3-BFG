/*
===========================================================================

Doom 3 BFG Edition GPL Source Code

===========================================================================
*/

#pragma hdrstop
#include "../../idlib/precompiled.h"
#include "win_local.h"

#include <lmerr.h>
#include <lmcons.h>
#include <errno.h>
#include <fcntl.h>

// Stub implementations for cross-compilation

address_t GetCallerAddr( long _ebp ) {
    (void)_ebp;
    return 0;
}

void Sys_GetCallStack( address_t *callStack, const int callStackSize ) {
    for (int i = 0; i < callStackSize; i++) {
        callStack[i] = 0;
    }
}

const char *Sys_GetCallStackStr( const address_t *callStack, const int callStackSize ) {
    static char string[4096];
    string[0] = '\0';
    (void)callStack;
    (void)callStackSize;
    return string;
}

const char *Sys_GetCallStackStr( long *callStack, const int callStackSize ) {
    static char string[4096];
    string[0] = '\0';
    (void)callStack;
    (void)callStackSize;
    return string;
}

address_t GetFuncAddr( address_t ptr ) {
    (void)ptr;
    return 0;
}

void Sys_LaunchWebsite( const char *url ) {
    (void)url;
}

bool Sys_GetDDCpuClock( uint64 &clockSpeed ) {
    clockSpeed = 0;
    return false;
}
