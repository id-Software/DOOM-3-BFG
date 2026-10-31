/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_sysevents.c - replaces the Win32 message pump and DirectInput polling
   (neo/sys/win32/win_main.cpp, win_wndproc.cpp, win_input.cpp). Gamepad
   events go to a hook owned by the usercmd module. */

#define RGD3_MAX_QUED_EVENTS 512
#define RGD3_MASK_QUED_EVENTS (RGD3_MAX_QUED_EVENTS - 1)

typedef struct Rgd3SysEventsState
{
	SDL_Mutex* lock;
	Rgd3SysEvent queue[RGD3_MAX_QUED_EVENTS];
	int head;
	int tail;
	int quit_requested;
	Rgd3ResizeHook resize_hook;
	Rgd3SdlEventHook gamepad_hook;
	/* lines from the text console wait here; one is released per frame,
	   as Sys_ConsoleInput hands over one line per Sys_GenerateEvents */
	Rgd3SysEvent console_lines[64];
	int console_head;
	int console_tail;
	int mouse_dx; /* relative motion since the last rgd3_sysevents_poll_mouse */
	int mouse_dy;
} Rgd3SysEventsState;

static Rgd3SysEventsState rgd3_sysevents;

void rgd3_sysevents_init(void)
{
	if (!rgd3_sysevents.lock)
		rgd3_sysevents.lock = SDL_CreateMutex();
}

void rgd3_sysevents_shutdown(void)
{
	rgd3_sysevents_clear();
	while (rgd3_sysevents.console_head != rgd3_sysevents.console_tail)
	{
		rgd3_heap_free(rgd3_sysevents.console_lines[rgd3_sysevents.console_tail & 63].ptr);
		++rgd3_sysevents.console_tail;
	}
	if (rgd3_sysevents.lock)
		SDL_DestroyMutex(rgd3_sysevents.lock);
	rgd3_sysevents.lock = NULL;
}

void rgd3_sysevents_set_resize_hook(Rgd3ResizeHook hook)
{
	rgd3_sysevents.resize_hook = hook;
}

int rgd3_sysevents_quit_requested(void)
{
	return rgd3_sysevents.quit_requested;
}

/* the event queue proper; caller holds the lock */
static void rgd3_sysevents_enqueue_locked(const Rgd3SysEvent* ev)
{
	if (rgd3_sysevents.head - rgd3_sysevents.tail >= RGD3_MAX_QUED_EVENTS)
	{
		/* Overflow drops the oldest event, like Sys_QueEvent. */
		Rgd3SysEvent* old = &rgd3_sysevents.queue[rgd3_sysevents.tail & RGD3_MASK_QUED_EVENTS];
		rgd3_heap_free(old->ptr);
		old->ptr = NULL;
		++rgd3_sysevents.tail;
	}
	rgd3_sysevents.queue[rgd3_sysevents.head & RGD3_MASK_QUED_EVENTS] = *ev;
	++rgd3_sysevents.head;
}

/* any thread; text-console lines wait in their own queue */
void rgd3_sysevents_push(const Rgd3SysEvent* ev)
{
	SDL_LockMutex(rgd3_sysevents.lock);
	if (ev->type == RGD3_SE_CONSOLE)
	{
		if (rgd3_sysevents.console_head - rgd3_sysevents.console_tail >= 64)
		{
			/* a full line queue drops the oldest line */
			rgd3_heap_free(rgd3_sysevents.console_lines[rgd3_sysevents.console_tail & 63].ptr);
			++rgd3_sysevents.console_tail;
		}
		rgd3_sysevents.console_lines[rgd3_sysevents.console_head & 63] = *ev;
		++rgd3_sysevents.console_head;
	}
	else
	{
		rgd3_sysevents_enqueue_locked(ev);
	}
	SDL_UnlockMutex(rgd3_sysevents.lock);
}

int rgd3_sysevents_get(Rgd3SysEvent* out)
{
	int got = 0;
	SDL_LockMutex(rgd3_sysevents.lock);
	if (rgd3_sysevents.head > rgd3_sysevents.tail)
	{
		*out = rgd3_sysevents.queue[rgd3_sysevents.tail & RGD3_MASK_QUED_EVENTS];
		++rgd3_sysevents.tail;
		got = 1;
	}
	SDL_UnlockMutex(rgd3_sysevents.lock);
	return got;
}

void rgd3_sysevents_free(Rgd3SysEvent* ev)
{
	rgd3_heap_free(ev->ptr);
	ev->ptr = NULL;
}

/* Focus loss: key, mouse and pad events still queued are stale (releases
   never arrive), so drop them and the accumulated mouse motion before
   clearing the pad, key and usercmd states. */
static void rgd3_sysevents_focus_lost(void)
{
	int read, write;
	SDL_LockMutex(rgd3_sysevents.lock);
	rgd3_sysevents.mouse_dx = 0;
	rgd3_sysevents.mouse_dy = 0;
	write = rgd3_sysevents.tail;
	for (read = rgd3_sysevents.tail; read < rgd3_sysevents.head; read++)
	{
		Rgd3SysEvent ev = rgd3_sysevents.queue[read & RGD3_MASK_QUED_EVENTS];
		if (ev.type == RGD3_SE_KEY || ev.type == RGD3_SE_MOUSE || ev.type == RGD3_SE_JOYSTICK)
			continue;
		rgd3_sysevents.queue[write & RGD3_MASK_QUED_EVENTS] = ev;
		write++;
	}
	rgd3_sysevents.head = write;
	SDL_UnlockMutex(rgd3_sysevents.lock);
	rgd3_gamepad_focus_lost();
	rgd3_keys_clear_states();
	rgd3_usercmd_clear(); /* idKeyInput::ClearStates clears the generator too */
}

/* moves the oldest waiting console line into the event queue */
static void rgd3_sysevents_release_console_line(void)
{
	SDL_LockMutex(rgd3_sysevents.lock);
	if (rgd3_sysevents.console_head != rgd3_sysevents.console_tail)
	{
		rgd3_sysevents_enqueue_locked(&rgd3_sysevents.console_lines[rgd3_sysevents.console_tail & 63]);
		++rgd3_sysevents.console_tail;
	}
	SDL_UnlockMutex(rgd3_sysevents.lock);
}

/* clears pending events; waiting console lines stay (Sys_ClearEvents
   does not touch the text console either) */
void rgd3_sysevents_clear(void)
{
	Rgd3SysEvent ev;
	if (!rgd3_sysevents.lock)
		return;
	while (rgd3_sysevents_get(&ev))
		rgd3_sysevents_free(&ev);
}

static void rgd3_sysevents_queue(Rgd3SysEventType type, int value, int value2)
{
	Rgd3SysEvent ev;
	memset(&ev, 0, sizeof(ev));
	ev.type = type;
	ev.value = value;
	ev.value2 = value2;
	rgd3_sysevents_push(&ev);
}

int rgd3_key_from_scancode(SDL_Scancode sc)
{
	if (sc >= SDL_SCANCODE_A && sc <= SDL_SCANCODE_Z)
	{
		static const int letters[26] = {
			RGD3_K_A, RGD3_K_B, RGD3_K_C, RGD3_K_D, RGD3_K_E, RGD3_K_F, RGD3_K_G, RGD3_K_H, RGD3_K_I,
			RGD3_K_J, RGD3_K_K, RGD3_K_L, RGD3_K_M, RGD3_K_N, RGD3_K_O, RGD3_K_P, RGD3_K_Q, RGD3_K_R,
			RGD3_K_S, RGD3_K_T, RGD3_K_U, RGD3_K_V, RGD3_K_W, RGD3_K_X, RGD3_K_Y, RGD3_K_Z
		};
		return letters[sc - SDL_SCANCODE_A];
	}
	if (sc >= SDL_SCANCODE_1 && sc <= SDL_SCANCODE_0)
		return RGD3_K_1 + (int)(sc - SDL_SCANCODE_1);
	if (sc >= SDL_SCANCODE_F1 && sc <= SDL_SCANCODE_F10)
		return RGD3_K_F1 + (int)(sc - SDL_SCANCODE_F1);
	switch (sc)
	{
		case SDL_SCANCODE_ESCAPE: return RGD3_K_ESCAPE;
		case SDL_SCANCODE_MINUS: return RGD3_K_MINUS;
		case SDL_SCANCODE_EQUALS: return RGD3_K_EQUALS;
		case SDL_SCANCODE_BACKSPACE: return RGD3_K_BACKSPACE;
		case SDL_SCANCODE_TAB: return RGD3_K_TAB;
		case SDL_SCANCODE_LEFTBRACKET: return RGD3_K_LBRACKET;
		case SDL_SCANCODE_RIGHTBRACKET: return RGD3_K_RBRACKET;
		case SDL_SCANCODE_RETURN: return RGD3_K_ENTER;
		case SDL_SCANCODE_LCTRL: return RGD3_K_LCTRL;
		case SDL_SCANCODE_SEMICOLON: return RGD3_K_SEMICOLON;
		case SDL_SCANCODE_APOSTROPHE: return RGD3_K_APOSTROPHE;
		case SDL_SCANCODE_GRAVE: return RGD3_K_GRAVE;
		case SDL_SCANCODE_LSHIFT: return RGD3_K_LSHIFT;
		case SDL_SCANCODE_BACKSLASH: return RGD3_K_BACKSLASH;
		case SDL_SCANCODE_COMMA: return RGD3_K_COMMA;
		case SDL_SCANCODE_PERIOD: return RGD3_K_PERIOD;
		case SDL_SCANCODE_SLASH: return RGD3_K_SLASH;
		case SDL_SCANCODE_RSHIFT: return RGD3_K_RSHIFT;
		case SDL_SCANCODE_KP_MULTIPLY: return RGD3_K_KP_STAR;
		case SDL_SCANCODE_LALT: return RGD3_K_LALT;
		case SDL_SCANCODE_SPACE: return RGD3_K_SPACE;
		case SDL_SCANCODE_CAPSLOCK: return RGD3_K_CAPSLOCK;
		case SDL_SCANCODE_NUMLOCKCLEAR: return RGD3_K_NUMLOCK;
		case SDL_SCANCODE_SCROLLLOCK: return RGD3_K_SCROLL;
		case SDL_SCANCODE_KP_7: return RGD3_K_KP_7;
		case SDL_SCANCODE_KP_8: return RGD3_K_KP_8;
		case SDL_SCANCODE_KP_9: return RGD3_K_KP_9;
		case SDL_SCANCODE_KP_MINUS: return RGD3_K_KP_MINUS;
		case SDL_SCANCODE_KP_4: return RGD3_K_KP_4;
		case SDL_SCANCODE_KP_5: return RGD3_K_KP_5;
		case SDL_SCANCODE_KP_6: return RGD3_K_KP_6;
		case SDL_SCANCODE_KP_PLUS: return RGD3_K_KP_PLUS;
		case SDL_SCANCODE_KP_1: return RGD3_K_KP_1;
		case SDL_SCANCODE_KP_2: return RGD3_K_KP_2;
		case SDL_SCANCODE_KP_3: return RGD3_K_KP_3;
		case SDL_SCANCODE_KP_0: return RGD3_K_KP_0;
		case SDL_SCANCODE_KP_PERIOD: return RGD3_K_KP_DOT;
		case SDL_SCANCODE_F11: return RGD3_K_F11;
		case SDL_SCANCODE_F12: return RGD3_K_F12;
		case SDL_SCANCODE_F13: return RGD3_K_F13;
		case SDL_SCANCODE_F14: return RGD3_K_F14;
		case SDL_SCANCODE_F15: return RGD3_K_F15;
		case SDL_SCANCODE_KP_EQUALS: return RGD3_K_KP_EQUALS;
		case SDL_SCANCODE_KP_ENTER: return RGD3_K_KP_ENTER;
		case SDL_SCANCODE_RCTRL: return RGD3_K_RCTRL;
		case SDL_SCANCODE_KP_COMMA: return RGD3_K_KP_COMMA;
		case SDL_SCANCODE_KP_DIVIDE: return RGD3_K_KP_SLASH;
		case SDL_SCANCODE_PRINTSCREEN: return RGD3_K_PRINTSCREEN;
		case SDL_SCANCODE_RALT: return RGD3_K_RALT;
		case SDL_SCANCODE_PAUSE: return RGD3_K_PAUSE;
		case SDL_SCANCODE_HOME: return RGD3_K_HOME;
		case SDL_SCANCODE_UP: return RGD3_K_UPARROW;
		case SDL_SCANCODE_PAGEUP: return RGD3_K_PGUP;
		case SDL_SCANCODE_LEFT: return RGD3_K_LEFTARROW;
		case SDL_SCANCODE_RIGHT: return RGD3_K_RIGHTARROW;
		case SDL_SCANCODE_END: return RGD3_K_END;
		case SDL_SCANCODE_DOWN: return RGD3_K_DOWNARROW;
		case SDL_SCANCODE_PAGEDOWN: return RGD3_K_PGDN;
		case SDL_SCANCODE_INSERT: return RGD3_K_INS;
		case SDL_SCANCODE_DELETE: return RGD3_K_DEL;
		case SDL_SCANCODE_LGUI: return RGD3_K_LWIN;
		case SDL_SCANCODE_RGUI: return RGD3_K_RWIN;
		case SDL_SCANCODE_APPLICATION: return RGD3_K_APPS;
		case SDL_SCANCODE_POWER: return RGD3_K_POWER;
		case SDL_SCANCODE_SLEEP: return RGD3_K_SLEEP;
		default: return RGD3_K_NONE;
	}
}

void rgd3_sysevents_set_gamepad_hook(Rgd3SdlEventHook hook)
{
	rgd3_sysevents.gamepad_hook = hook;
}

/* Sys_PollMouseInputEvents: relative motion accumulated since the last
   call (main thread). Buttons and the wheel arrive as key events. */
void rgd3_sysevents_poll_mouse(int* dx, int* dy)
{
	*dx = rgd3_sysevents.mouse_dx;
	*dy = rgd3_sysevents.mouse_dy;
	rgd3_sysevents.mouse_dx = 0;
	rgd3_sysevents.mouse_dy = 0;
}

/* queue an event from the main thread (gamepad buttons, synthetic keys) */
void rgd3_sysevents_queue_event(Rgd3SysEventType type, int value, int value2, int input_device)
{
	Rgd3SysEvent ev = { 0 };
	ev.type = type;
	ev.value = value;
	ev.value2 = value2;
	ev.input_device = input_device;
	rgd3_sysevents_push(&ev);
}

static void rgd3_sysevents_text(const char* utf8)
{
	const char* p = utf8;
	while (*p)
	{
		u32 cp = SDL_StepUTF8(&p, NULL);
		if (!cp)
			break;
		rgd3_sysevents_queue(RGD3_SE_CHAR, (int)cp, 0);
	}
}

void rgd3_sysevents_generate(void)
{
	SDL_Event e;
	rgd3_sysevents_release_console_line();
	while (SDL_PollEvent(&e))
	{
		switch (e.type)
		{
			case SDL_EVENT_QUIT:
				rgd3_sysevents.quit_requested = 1;
				break;
			case SDL_EVENT_KEY_DOWN:
			case SDL_EVENT_KEY_UP:
			{
				int key = rgd3_key_from_scancode(e.key.scancode);
				if (key != RGD3_K_NONE && !e.key.repeat)
					rgd3_sysevents_queue(RGD3_SE_KEY, key, e.type == SDL_EVENT_KEY_DOWN);
				break;
			}
			case SDL_EVENT_TEXT_INPUT:
				rgd3_sysevents_text(e.text.text);
				break;
			case SDL_EVENT_MOUSE_MOTION:
				rgd3_sysevents.mouse_dx += (int)e.motion.xrel;
				rgd3_sysevents.mouse_dy += (int)e.motion.yrel;
				rgd3_sysevents_queue(RGD3_SE_MOUSE, (int)e.motion.xrel, (int)e.motion.yrel);
				rgd3_sysevents_queue(RGD3_SE_MOUSE_ABSOLUTE, (int)e.motion.x, (int)e.motion.y);
				break;
			case SDL_EVENT_MOUSE_BUTTON_DOWN:
			case SDL_EVENT_MOUSE_BUTTON_UP:
			{
				int button = e.button.button;
				/* SDL: 1 left, 2 middle, 3 right. Original: MOUSE1 left, MOUSE2 right, MOUSE3 middle. */
				int key = button == SDL_BUTTON_LEFT ? RGD3_K_MOUSE1 : button == SDL_BUTTON_RIGHT ? RGD3_K_MOUSE2 :
					button == SDL_BUTTON_MIDDLE ? RGD3_K_MOUSE3 : (button >= 4 && button <= 8) ? RGD3_K_MOUSE4 + (button - 4) : RGD3_K_NONE;
				if (key != RGD3_K_NONE)
					rgd3_sysevents_queue(RGD3_SE_KEY, key, e.type == SDL_EVENT_MOUSE_BUTTON_DOWN);
				break;
			}
			case SDL_EVENT_MOUSE_WHEEL:
			{
				int key = e.wheel.y > 0 ? RGD3_K_MWHEELUP : (e.wheel.y < 0 ? RGD3_K_MWHEELDOWN : RGD3_K_NONE);
				if (key != RGD3_K_NONE)
				{
					rgd3_sysevents_queue(RGD3_SE_KEY, key, 1);
					rgd3_sysevents_queue(RGD3_SE_KEY, key, 0);
				}
				break;
			}
			case SDL_EVENT_WINDOW_MOUSE_LEAVE:
				rgd3_sysevents_queue(RGD3_SE_MOUSE_LEAVE, 0, 0);
				break;
			case SDL_EVENT_WINDOW_FOCUS_LOST:
				rgd3_sysevents_focus_lost();
				break;
			case SDL_EVENT_WINDOW_FOCUS_GAINED:
				rgd3_gamepad_resync_all(); /* pads held through the switch */
				break;
			case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
				if (rgd3_sysevents.resize_hook)
					rgd3_sysevents.resize_hook(e.window.data1, e.window.data2);
				break;
			case SDL_EVENT_GAMEPAD_ADDED:
			case SDL_EVENT_GAMEPAD_REMOVED:
			case SDL_EVENT_GAMEPAD_REMAPPED:
			case SDL_EVENT_GAMEPAD_AXIS_MOTION:
			case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
			case SDL_EVENT_GAMEPAD_BUTTON_UP:
				if (rgd3_sysevents.gamepad_hook)
					rgd3_sysevents.gamepad_hook(&e);
				break;
			default:
				break;
		}
	}
}

/* idEventLoop::RunEventLoop / ProcessEvent: bound commands run before each
   event; keys update the key state first; lines from the text console are
   buffered; everything else goes to rgd3_common_process_event (console,
   menus, then key bindings). */
void rgd3_sysevents_run_loop(int execute_commands)
{
	Rgd3SysEvent ev;
	for (;;)
	{
		if (execute_commands)
			rgd3_cmd_execute_buffer();
		if (!rgd3_sysevents_get(&ev))
			break;
		if (ev.type == RGD3_SE_KEY)
		{
			rgd3_keys_preliminary_key_event(ev.value, ev.value2 != 0);
			/* replayed by the next usercmd build, so taps inside a frame count */
			rgd3_usercmd_key_event(ev.value, ev.value2);
		}
		if (ev.type == RGD3_SE_CONSOLE)
		{
			if (ev.ptr)
			{
				rgd3_cmd_buffer_text(RGD3_CMD_EXEC_APPEND, (const char*)ev.ptr);
				rgd3_cmd_buffer_text(RGD3_CMD_EXEC_APPEND, "\n");
			}
		}
		else
		{
			rgd3_common_process_event(&ev);
		}
		rgd3_sysevents_free(&ev);
	}
}
