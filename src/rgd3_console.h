/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_console.h - the developer console (idConsoleLocal,
   neo/framework/Console.cpp; idConsoleHistory, ConsoleHistory.cpp) plus a
   stdin reader thread that turns typed lines into RGD3_SE_CONSOLE events
   (replaces the Win32 dedicated console window).

   Everything printed lands in a colour-coded scrollback (rgd3_console_print
   is called by the log path from any thread). The in-window console opens
   with the key left of 1 (K_GRAVE, com_allowConsole) or `toggleconsole`,
   sees events before key bindings (rgd3_console_process_event), and draws
   into the rg_text overlay batch until the M3 2D renderer exists. */
#ifndef RGD3_CONSOLE_H
#define RGD3_CONSOLE_H
#ifndef RGD3_ALL_C
#error "rgd3 headers are included through src/all.c only"
#endif

#define RGD3_CON_TEXTSIZE 0x30000
#define RGD3_CON_NUM_TIMES 4
#define RGD3_CON_LINE_WIDTH 100 /* the original computes 70 from its 640-wide safe area */
#define RGD3_CON_COMMAND_HISTORY 64
#define RGD3_CON_HISTORY_FILE "consoleHistory.txt"

void rgd3_console_init(void);			/* first thing at startup: prints are buffered from here */
void rgd3_console_shutdown(void);
void rgd3_console_register(void);		/* cvars and commands */
void rgd3_console_load_history(void);	/* after the file system; enables writing the history file */
void rgd3_console_start_stdin(void);
void rgd3_console_print(const char* text);

/* Returns 1 when the console consumed the event. force_accept makes it take
   every key and char event even while closed (the original does this when
   no map is spawned). */
int rgd3_console_process_event(const Rgd3SysEvent* ev, int force_accept);
int rgd3_console_active(void);
void rgd3_console_close(void);
void rgd3_console_clear_notify_lines(void);
void rgd3_console_clear(void);

/* Queues the console (or the notify lines when closed) into the overlay
   batch: call between rgd3_overlay_begin and rgd3_overlay_end. A no-op
   without a window. */
void rgd3_console_draw(int force_full_screen);

int rgd3_console_self_test(void);

#endif
