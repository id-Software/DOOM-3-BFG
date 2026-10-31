/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_defs.h - constants shared by every rgd3 module. */
#ifndef RGD3_DEFS_H
#define RGD3_DEFS_H
#ifndef RGD3_ALL_C
#error "rgd3 headers are included through src/all.c only"
#endif

#define RGD3_ENGINE_NAME "rgd3"
#define RGD3_ENGINE_VERSION "0.0.2-m1"
#ifndef RGD3_BUILD_MODE
#define RGD3_BUILD_MODE "unknown"
#endif

#define RGD3_MAX_STRING_CHARS 1024
#define RGD3_MAX_OSPATH 512
#define RGD3_MAX_COMMAND_ARGS 64
#define RGD3_MAX_COMMAND_STRING (2 * RGD3_MAX_STRING_CHARS)
#define RGD3_MAX_CONSOLE_LINES 32
#define RGD3_BIT(n) (1u << (n))

#define RGD3_BASE_GAMEDIR "base"
#define RGD3_CONFIG_FILE "rgd3.cfg"
#define RGD3_PREF_ORG "superwendel"
#define RGD3_PREF_APP "rgd3"

typedef i64 Rgd3TimeStamp;
#define RGD3_FILE_NOT_FOUND_TIMESTAMP ((Rgd3TimeStamp)-1)

#if defined(_MSC_VER)
#define RGD3_NORETURN __declspec(noreturn)
#else
#define RGD3_NORETURN __attribute__((noreturn))
#endif

#endif
