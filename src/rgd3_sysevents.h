/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_sysevents.h - system events (sysEvent_t, neo/sys/sys_public.h) built
   from SDL3 events, and the event loop (idEventLoop, neo/framework/EventLoop.cpp).
   The queue is thread-safe so the stdin console thread can push lines. */
#ifndef RGD3_SYSEVENTS_H
#define RGD3_SYSEVENTS_H
#ifndef RGD3_ALL_C
#error "rgd3 headers are included through src/all.c only"
#endif

typedef enum Rgd3SysEventType
{
	RGD3_SE_NONE,
	RGD3_SE_KEY,			/* value: key number, value2: down flag */
	RGD3_SE_CHAR,			/* value: unicode codepoint */
	RGD3_SE_MOUSE,			/* value, value2: relative motion */
	RGD3_SE_MOUSE_ABSOLUTE,	/* value, value2: window position */
	RGD3_SE_MOUSE_LEAVE,
	RGD3_SE_JOYSTICK,		/* value: axis, value2: -32767..32767 */
	RGD3_SE_CONSOLE			/* ptr: heap copy of a typed command line */
} Rgd3SysEventType;

typedef struct Rgd3SysEvent
{
	Rgd3SysEventType type;
	int value;
	int value2;
	int ptr_length;
	void* ptr;
	int input_device;
} Rgd3SysEvent;

typedef void (*Rgd3ResizeHook)(int pixel_width, int pixel_height);

void rgd3_sysevents_init(void);
void rgd3_sysevents_shutdown(void);
void rgd3_sysevents_set_resize_hook(Rgd3ResizeHook hook);
void rgd3_sysevents_generate(void);
void rgd3_sysevents_push(const Rgd3SysEvent* ev);
int rgd3_sysevents_get(Rgd3SysEvent* out);
void rgd3_sysevents_free(Rgd3SysEvent* ev);
void rgd3_sysevents_clear(void);
int rgd3_sysevents_quit_requested(void);
void rgd3_sysevents_run_loop(int execute_commands);
int rgd3_key_from_scancode(SDL_Scancode sc);

#endif
