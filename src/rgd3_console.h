/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_console.h - developer console. M0: a line ring that keeps everything
   printed, plus a stdin reader thread that turns typed lines into console
   events. The in-window console (rg_text, Tab completion, history) is M2. */
#ifndef RGD3_CONSOLE_H
#define RGD3_CONSOLE_H
#ifndef RGD3_ALL_C
#error "rgd3 headers are included through src/all.c only"
#endif

#define RGD3_CONSOLE_LINES 1024
#define RGD3_CONSOLE_LINE_CHARS 256

void rgd3_console_init(void);
void rgd3_console_shutdown(void);
void rgd3_console_start_stdin(void);
void rgd3_console_print(const char* text);
int rgd3_console_num_lines(void);
const char* rgd3_console_line(int index_from_newest);

#endif
