/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3.h - every rgd3 module header in dependency order. Only src/all.c
   includes this; module headers never include rg_* or SDL headers. */
#ifndef RGD3_H
#define RGD3_H
#ifndef RGD3_ALL_C
#error "rgd3 headers are included through src/all.c only"
#endif

#include "rgd3_defs.h"
#include "rgd3_mem.h"
#include "rgd3_str.h"
#include "rgd3_math.h"
#include "rgd3_time.h"
#include "rgd3_log.h"
#include "rgd3_error.h"
#include "rgd3_thread.h"
#include "rgd3_platform.h"
#include "rgd3_cvar.h"
#include "rgd3_cmdargs.h"
#include "rgd3_cmd.h"
#include "rgd3_fs.h"
#include "rgd3_lexer.h"
#include "rgd3_parser.h"
#include "rgd3_hashindex.h"
#include "rgd3_dict.h"
#include "rgd3_lang.h"
#include "rgd3_decl.h"
#include "rgd3_decl_basic.h"
#include "rgd3_bin.h"
#include "rgd3_manifest.h"
#include "rgd3_mapfile.h"
#include "rgd3_keys.h"
#include "rgd3_sysevents.h"
#include "rgd3_console.h"
#include "rgd3_overlay.h"
#include "rgd3_app.h"

void rgd3_registry_register_all(void);
int rgd3_selftest_run(void); /* rgd3_selftest.c: returns the failure count */

#endif
