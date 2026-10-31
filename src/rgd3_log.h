/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_log.h - the engine print path (Printf / DPrintf / Warning).
   Text goes to stdout with colour escapes stripped and to the console ring
   with escapes kept. rg_log (RG_INFO etc.) stays the rg libraries' channel. */
#ifndef RGD3_LOG_H
#define RGD3_LOG_H
#ifndef RGD3_ALL_C
#error "rgd3 headers are included through src/all.c only"
#endif

#define RGD3_MAX_PRINT_MSG 16384
#define RGD3_MAX_WARNING_LIST 256

void rgd3_log_init(void);
void rgd3_log_shutdown(void);
void rgd3_log_register(void);

void rgd3_printf(const char* fmt, ...);
void rgd3_vprintf(const char* fmt, va_list args);
void rgd3_dprintf(const char* fmt, ...);
void rgd3_warning(const char* fmt, ...);
void rgd3_dwarning(const char* fmt, ...);
int rgd3_log_developer(void);
int rgd3_log_total_warnings(void);

void rgd3_log_print_warnings(void);
void rgd3_log_clear_warnings(const char* reason);

#endif
