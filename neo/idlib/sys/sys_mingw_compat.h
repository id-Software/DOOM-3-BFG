/*
===========================================================================
MinGW Compatibility Header for DOOM 3 BFG / Dark Ages
Provides compatibility shims for MSVC-specific constructs when building
with MinGW (GCC for Windows cross-compilation).
===========================================================================
*/
#ifndef SYS_MINGW_COMPAT_H
#define SYS_MINGW_COMPAT_H

#if defined(__GNUC__) && defined(_WIN32)

// MinGW defines _WIN32 but doesn't have MSVC-specific features

// Alignment
#undef ALIGN16
#undef ALIGNTYPE16
#undef ALIGNTYPE128
#define ALIGN16( x )       x __attribute__((aligned(16)))
#define ALIGNTYPE16        __attribute__((aligned(16)))
#define ALIGNTYPE128       __attribute__((aligned(128)))

// Inlining
#undef ID_FORCE_INLINE
#undef ID_FORCE_INLINE_EXTERN
#define ID_FORCE_INLINE        __attribute__((always_inline)) inline
#define ID_FORCE_INLINE_EXTERN extern __attribute__((always_inline)) inline

// No-return
#undef NO_RETURN
#define NO_RETURN __attribute__((noreturn))

// Format string verification (no-op on MinGW)
#undef VERIFY_FORMAT_STRING
#define VERIFY_FORMAT_STRING

// Suppress MSVC #pragma warning (GCC ignores unknown pragmas with -Wno-unknown-pragmas)

// MSVC intrinsics compatibility
#ifndef _MSC_VER
#define __analysis_assume(x)
#endif

// _alloca on MinGW is alloca
#ifndef _alloca
#include <malloc.h>
#define _alloca alloca
#endif

// UINT_PTR for MinGW
#ifndef UINT_PTR
#include <stdint.h>
#define UINT_PTR uintptr_t
#endif

#endif // __GNUC__ && _WIN32

#endif // SYS_MINGW_COMPAT_H
