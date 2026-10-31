/* SPDX-License-Identifier: GPL-3.0-or-later
   all.c - the single rgd3 translation unit. rg_* headers keep header-local
   state (memory pool, logger), so they are included exactly once, here.

   Order: feature macros, system and SDL headers, rg_* headers (rg_log.h has
   no include guard and must come before rg_assert.h), rgd3 headers, then
   every rgd3 .c file in dependency order. Add new .c files below. */

/* feature macros: must precede every rg header */
#ifndef SDL_MAIN_HANDLED
#define SDL_MAIN_HANDLED 1
#endif
#ifndef RG_SPRINTF_NO_ASM
#define RG_SPRINTF_NO_ASM 1
#endif
/* static inline instead of __forceinline: forced inlining across a unity
   build this size makes MSVC compile times and memory explode. */
#define RGINLINE static inline
#define RG_LOG_BUFFER_SIZE 2048
#define RGD3_ALL_C 1

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <float.h>
#include <limits.h>
#include <math.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* rg_core */
#include "rg_defs.h"
#include "rg_log.h"
#include "rg_assert.h"
#include "rg_mem.h"
#include "rg_sprintf.h"
#include "rg_bin.h"
#include "rg_math.h"
#include "rg_time.h"
#include "rg_sdl.h"
#include "rg_window.h"
#include "rg_gpu.h"
/* rg_text */
#include "rg_text.h"
#include "rg_text_gpu.h"
#include "demo_font.h"

/* rg_image third_party: PNG writing (screenshots) and zlib deflate
   (stbi_zlib_compress, saves); miniz's tinfl for inflate */
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_STATIC
#include "stb_image_write.h"
#include "miniz/miniz_tinfl.c"

/* rgd3 */
#include "rgd3.h"

/* foundation */
#include "rgd3_mem.c"
#include "rgd3_str.c"
#include "rgd3_time.c"
#include "rgd3_log.c"
#include "rgd3_math.c"
#include "rgd3_error.c"
#include "rgd3_thread.c"
#include "rgd3_jobs.c"
#include "rgd3_platform.c"
#include "rgd3_cmdargs.c"
#include "rgd3_cvar.c"
#include "rgd3_cmd.c"
#include "rgd3_fs.c"
#include "rgd3_lexer.c"
#include "rgd3_parser.c"
#include "rgd3_lexer_test.c"
#include "rgd3_hashindex.c"
#include "rgd3_dict.c"
#include "rgd3_bitmsg.c"
#include "rgd3_lang.c"
#include "rgd3_decl.c"
#include "rgd3_decl_basic.c"
#include "rgd3_bin.c"
#include "rgd3_savegame.c"
#include "rgd3_manifest.c"
#include "rgd3_mapfile.c"
#include "rgd3_keys.c"
#include "rgd3_sysevents.c"
#include "rgd3_gamepad.c"
#include "rgd3_usercmd.c"
#include "rgd3_editfield.c"
#include "rgd3_console.c"
#include "rgd3_overlay.c"
#include "rgd3_screenshot.c"

/* --- RENDERER MODULES (M3+) --- */
/* --- GAME MODULES (M4+) --- */

/* framework: session, frame loop, subsystem stubs (replaced as the real
   renderer, game and sound modules arrive) */
#include "rgd3_hooks.c"
#include "rgd3_session.c"
#include "rgd3_render_stub.c"
#include "rgd3_sound_stub.c"
#include "rgd3_game_stub.c"
#include "rgd3_common.c"

#include "rgd3_selftest.c"
#include "rgd3_registry.c"
#include "rgd3_app.c"
