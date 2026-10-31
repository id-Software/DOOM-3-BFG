/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_gamepad.c - SDL3 gamepads in place of idJoystickWin32
   (neo/sys/win32/win_input.cpp: PollInputEvents, PostInputEvent,
   PushButton, SetRumble; button order from its joyRemap table). Cvars
   in_useJoystick and in_joystickRumble come from
   neo/framework/UsercmdGen.cpp.

   Differences from the original:
   - The XInput sampling thread and the current/previous state diff are
     replaced by SDL gamepad events, delivered on the main thread through
     the sysevents gamepad hook. Every event is buffered as it arrives, so a
     press and release between two polls is not lost (the original printed
     "Dropped button press"). Pending axis events coalesce to the latest value.
   - Pads get device slots 0..3 in connection order (XInput user index).
   - Buttons an XInput pad lacks map to the next free K_JOY numbers: guide
     JOY11 (as in RBDOOM-3-BFG), misc JOY12, paddles JOY13..JOY16.
   - With in_useJoystick 0 pad input is ignored entirely; when it goes to 0,
     or a pad is removed, held buttons and deflected axes are released so
     nothing sticks.
   - SDL rumble needs a duration: each set lasts RGD3_GAMEPAD_RUMBLE_MS, so
     a pad stops by itself when nobody refreshes it. */

#define RGD3_GAMEPAD_MAX_EVENTS 256
#define RGD3_GAMEPAD_KEY_RANGE 16384 /* PostInputEvent's default range */
#define RGD3_GAMEPAD_NUM_KEYS (RGD3_K_JOY_DPAD_RIGHT - RGD3_K_JOY1 + 1)
#define RGD3_GAMEPAD_RUMBLE_MS 1000

static Rgd3Cvar cv_in_useJoystick = RGD3_CVAR("in_useJoystick", "0", RGD3_CVAR_ARCHIVE | RGD3_CVAR_BOOL, "enables/disables the gamepad for PC use");
static Rgd3Cvar cv_in_joystickRumble = RGD3_CVAR("in_joystickRumble", "1", RGD3_CVAR_SYSTEM | RGD3_CVAR_ARCHIVE | RGD3_CVAR_BOOL, "enable joystick rumble");

typedef struct Rgd3GamepadEvent
{
	int event;
	int value;
} Rgd3GamepadEvent;

typedef struct Rgd3GamepadDevice
{
	SDL_Gamepad* pad;
	SDL_JoystickID id;
	int rumble_warned;
	int rumbling;
	u8 button_states[RGD3_GAMEPAD_NUM_KEYS]; /* PushButton: last SE_KEY state per K_JOY key */
	int joy_axis[RGD3_MAX_JOYSTICK_AXIS]; /* last SE_JOYSTICK value per axis */
	int last_value[RGD3_MAX_JOY_EVENT]; /* last value posted per J_* event */
	Rgd3GamepadEvent events[RGD3_GAMEPAD_MAX_EVENTS]; /* not yet polled */
	int num_events;
} Rgd3GamepadDevice;

typedef struct Rgd3GamepadState
{
	Rgd3GamepadDevice devices[RGD3_MAX_JOYSTICKS];
	Rgd3GamepadEvent polled[RGD3_GAMEPAD_MAX_EVENTS];
	int num_polled;
	int was_enabled;
} Rgd3GamepadState;

static Rgd3GamepadState rgd3_gamepad;

void rgd3_gamepad_register(void)
{
	static Rgd3Cvar* const cvars[] = { &cv_in_useJoystick, &cv_in_joystickRumble };
	rgd3_cvar_register_table(cvars, (int)RG_ARRAY_COUNT(cvars));
}

static Rgd3GamepadDevice* rgd3_gamepad_device(int device)
{
	if (device < 0 || device >= RGD3_MAX_JOYSTICKS)
		return NULL;
	return &rgd3_gamepad.devices[device];
}

static int rgd3_gamepad_find(SDL_JoystickID id)
{
	int i;
	for (i = 0; i < RGD3_MAX_JOYSTICKS; i++)
	{
		if (rgd3_gamepad.devices[i].pad && rgd3_gamepad.devices[i].id == id)
			return i;
	}
	return -1;
}

/* idJoystickWin32::PushButton: only state changes become SE_KEY events */
static void rgd3_gamepad_push_button(int device, int key, int value)
{
	Rgd3GamepadDevice* d = &rgd3_gamepad.devices[device];
	int index = key - RGD3_K_JOY1;
	if (index < 0 || index >= RGD3_GAMEPAD_NUM_KEYS)
		return;
	if (d->button_states[index] != (u8)value)
	{
		d->button_states[index] = (u8)value;
		rgd3_sysevents_queue_event(RGD3_SE_KEY, key, value, device);
	}
}

static void rgd3_gamepad_buffer_event(Rgd3GamepadDevice* d, int event, int value)
{
	int i;
	if (event >= RGD3_J_AXIS_MIN && event <= RGD3_J_AXIS_MAX)
	{
		/* the generator only keeps the latest value of an axis */
		for (i = 0; i < d->num_events; i++)
		{
			if (d->events[i].event == event)
			{
				d->events[i].value = value;
				return;
			}
		}
	}
	if (d->num_events == RGD3_GAMEPAD_MAX_EVENTS)
	{
		memmove(&d->events[0], &d->events[1], sizeof(d->events[0]) * (RGD3_GAMEPAD_MAX_EVENTS - 1));
		d->num_events--;
	}
	d->events[d->num_events].event = event;
	d->events[d->num_events].value = value;
	d->num_events++;
}

/* idJoystickWin32::PostInputEvent */
static void rgd3_gamepad_post_input_event(int device, int event, int value)
{
	const int range = RGD3_GAMEPAD_KEY_RANGE;
	Rgd3GamepadDevice* d = &rgd3_gamepad.devices[device];

	/* these events are used for GUI button presses */
	if (event >= RGD3_J_ACTION1 && event <= RGD3_J_ACTION_MAX)
	{
		rgd3_gamepad_push_button(device, RGD3_K_JOY1 + (event - RGD3_J_ACTION1), value != 0);
	}
	else if (event == RGD3_J_AXIS_LEFT_X)
	{
		rgd3_gamepad_push_button(device, RGD3_K_JOY_STICK1_LEFT, value < -range);
		rgd3_gamepad_push_button(device, RGD3_K_JOY_STICK1_RIGHT, value > range);
	}
	else if (event == RGD3_J_AXIS_LEFT_Y)
	{
		rgd3_gamepad_push_button(device, RGD3_K_JOY_STICK1_UP, value < -range);
		rgd3_gamepad_push_button(device, RGD3_K_JOY_STICK1_DOWN, value > range);
	}
	else if (event == RGD3_J_AXIS_RIGHT_X)
	{
		rgd3_gamepad_push_button(device, RGD3_K_JOY_STICK2_LEFT, value < -range);
		rgd3_gamepad_push_button(device, RGD3_K_JOY_STICK2_RIGHT, value > range);
	}
	else if (event == RGD3_J_AXIS_RIGHT_Y)
	{
		rgd3_gamepad_push_button(device, RGD3_K_JOY_STICK2_UP, value < -range);
		rgd3_gamepad_push_button(device, RGD3_K_JOY_STICK2_DOWN, value > range);
	}
	else if (event >= RGD3_J_DPAD_UP && event <= RGD3_J_DPAD_RIGHT)
	{
		rgd3_gamepad_push_button(device, RGD3_K_JOY_DPAD_UP + (event - RGD3_J_DPAD_UP), value != 0);
	}
	else if (event == RGD3_J_AXIS_LEFT_TRIG)
	{
		rgd3_gamepad_push_button(device, RGD3_K_JOY_TRIGGER1, value > range);
	}
	else if (event == RGD3_J_AXIS_RIGHT_TRIG)
	{
		rgd3_gamepad_push_button(device, RGD3_K_JOY_TRIGGER2, value > range);
	}
	if (event >= RGD3_J_AXIS_MIN && event <= RGD3_J_AXIS_MAX)
	{
		int axis = event - RGD3_J_AXIS_MIN;
		int percent = (value * 16) / range;
		if (d->joy_axis[axis] != percent)
		{
			d->joy_axis[axis] = percent;
			rgd3_sysevents_queue_event(RGD3_SE_JOYSTICK, axis, percent, device);
		}
	}

	/* these events are used for actual game input */
	if (event >= 0 && event < RGD3_MAX_JOY_EVENT)
		d->last_value[event] = value;
	rgd3_gamepad_buffer_event(d, event, value);
}

/* release every held button and centre every axis */
static void rgd3_gamepad_release_all(int device)
{
	Rgd3GamepadDevice* d = &rgd3_gamepad.devices[device];
	int e;
	for (e = 0; e < RGD3_MAX_JOY_EVENT; e++)
	{
		if (d->last_value[e] != 0)
			rgd3_gamepad_post_input_event(device, e, 0);
	}
}

/* in_useJoystick going to 0 releases what the pads were holding */
int rgd3_gamepad_enabled(void)
{
	int enabled = cv_in_useJoystick.integer_value != 0;
	if (rgd3_gamepad.was_enabled && !enabled)
	{
		int i;
		for (i = 0; i < RGD3_MAX_JOYSTICKS; i++)
		{
			if (rgd3_gamepad.devices[i].pad)
				rgd3_gamepad_release_all(i);
		}
	}
	rgd3_gamepad.was_enabled = enabled;
	return enabled;
}

static void rgd3_gamepad_open(SDL_JoystickID id)
{
	Rgd3GamepadDevice* d;
	SDL_Gamepad* pad;
	int slot;

	if (rgd3_gamepad_find(id) >= 0)
		return;
	for (slot = 0; slot < RGD3_MAX_JOYSTICKS; slot++)
	{
		if (!rgd3_gamepad.devices[slot].pad)
			break;
	}
	if (slot == RGD3_MAX_JOYSTICKS)
	{
		rgd3_printf("gamepad ignored: %d are already open\n", RGD3_MAX_JOYSTICKS);
		return;
	}
	pad = SDL_OpenGamepad(id);
	if (!pad)
	{
		rgd3_warning("SDL_OpenGamepad failed: %s", SDL_GetError());
		return;
	}
	d = &rgd3_gamepad.devices[slot];
	memset(d, 0, sizeof(*d));
	d->pad = pad;
	d->id = id;
	rgd3_printf("gamepad %d: %s\n", slot, SDL_GetGamepadName(pad) ? SDL_GetGamepadName(pad) : "unnamed");
}

static void rgd3_gamepad_close(int slot)
{
	Rgd3GamepadDevice* d = &rgd3_gamepad.devices[slot];
	if (!d->pad)
		return;
	rgd3_gamepad_release_all(slot);
	SDL_RumbleGamepad(d->pad, 0, 0, 0);
	SDL_CloseGamepad(d->pad);
	d->pad = NULL;
	d->id = 0;
	rgd3_printf("gamepad %d removed\n", slot);
}

static int rgd3_gamepad_button_event(int button)
{
	switch (button)
	{
		case SDL_GAMEPAD_BUTTON_SOUTH: return RGD3_J_ACTION1; /* A */
		case SDL_GAMEPAD_BUTTON_EAST: return RGD3_J_ACTION2; /* B */
		case SDL_GAMEPAD_BUTTON_WEST: return RGD3_J_ACTION3; /* X */
		case SDL_GAMEPAD_BUTTON_NORTH: return RGD3_J_ACTION4; /* Y */
		case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER: return RGD3_J_ACTION5;
		case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER: return RGD3_J_ACTION6;
		case SDL_GAMEPAD_BUTTON_LEFT_STICK: return RGD3_J_ACTION7;
		case SDL_GAMEPAD_BUTTON_RIGHT_STICK: return RGD3_J_ACTION8;
		case SDL_GAMEPAD_BUTTON_START: return RGD3_J_ACTION9;
		case SDL_GAMEPAD_BUTTON_BACK: return RGD3_J_ACTION10;
		case SDL_GAMEPAD_BUTTON_GUIDE: return RGD3_J_ACTION11;
		case SDL_GAMEPAD_BUTTON_MISC1: return RGD3_J_ACTION12;
		case SDL_GAMEPAD_BUTTON_RIGHT_PADDLE1: return RGD3_J_ACTION13;
		case SDL_GAMEPAD_BUTTON_LEFT_PADDLE1: return RGD3_J_ACTION14;
		case SDL_GAMEPAD_BUTTON_RIGHT_PADDLE2: return RGD3_J_ACTION15;
		case SDL_GAMEPAD_BUTTON_LEFT_PADDLE2: return RGD3_J_ACTION16;
		case SDL_GAMEPAD_BUTTON_DPAD_UP: return RGD3_J_DPAD_UP;
		case SDL_GAMEPAD_BUTTON_DPAD_DOWN: return RGD3_J_DPAD_DOWN;
		case SDL_GAMEPAD_BUTTON_DPAD_LEFT: return RGD3_J_DPAD_LEFT;
		case SDL_GAMEPAD_BUTTON_DPAD_RIGHT: return RGD3_J_DPAD_RIGHT;
		default: return -1;
	}
}

/* sysevents hook: SDL gamepad events on the main thread */
static void rgd3_gamepad_hook(const SDL_Event* e)
{
	int enabled = rgd3_gamepad_enabled();
	int slot;

	switch (e->type)
	{
		case SDL_EVENT_GAMEPAD_ADDED:
			rgd3_gamepad_open(e->gdevice.which);
			break;
		case SDL_EVENT_GAMEPAD_REMOVED:
			slot = rgd3_gamepad_find(e->gdevice.which);
			if (slot >= 0)
				rgd3_gamepad_close(slot);
			break;
		case SDL_EVENT_GAMEPAD_AXIS_MOTION:
		{
			int value = e->gaxis.value;
			slot = rgd3_gamepad_find(e->gaxis.which);
			if (!enabled || slot < 0 || e->gaxis.axis >= RGD3_MAX_JOYSTICK_AXIS)
				break;
			/* SDL axes come in joystickAxis_t order; stick Y is positive
			   down, which is what the original made of XInput's -sThumbLY */
			if (value < -32767)
				value = -32767;
			rgd3_gamepad_post_input_event(slot, RGD3_J_AXIS_MIN + e->gaxis.axis, value);
			break;
		}
		case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
		case SDL_EVENT_GAMEPAD_BUTTON_UP:
		{
			int event = rgd3_gamepad_button_event(e->gbutton.button);
			slot = rgd3_gamepad_find(e->gbutton.which);
			if (!enabled || slot < 0 || event < 0)
				break;
			rgd3_gamepad_post_input_event(slot, event, e->gbutton.down ? 1 : 0);
			break;
		}
		default:
			break;
	}
}

void rgd3_gamepad_init(void)
{
	SDL_JoystickID* ids;
	int count = 0, i;

	rgd3_gamepad.was_enabled = cv_in_useJoystick.integer_value != 0;
	rgd3_sysevents_set_gamepad_hook(rgd3_gamepad_hook);
	if (!SDL_WasInit(SDL_INIT_GAMEPAD))
		return;
	/* pads connected before init (the ADDED events for them may already
	   have gone by) */
	ids = SDL_GetGamepads(&count);
	for (i = 0; ids && i < count; i++)
		rgd3_gamepad_open(ids[i]);
	SDL_free(ids);
}

void rgd3_gamepad_shutdown(void)
{
	int i;
	rgd3_sysevents_set_gamepad_hook(NULL);
	for (i = 0; i < RGD3_MAX_JOYSTICKS; i++)
	{
		Rgd3GamepadDevice* d = &rgd3_gamepad.devices[i];
		if (d->pad)
		{
			SDL_RumbleGamepad(d->pad, 0, 0, 0);
			SDL_CloseGamepad(d->pad);
		}
	}
	memset(&rgd3_gamepad, 0, sizeof(rgd3_gamepad));
}

int rgd3_gamepad_is_connected(int device)
{
	Rgd3GamepadDevice* d = rgd3_gamepad_device(device);
	return d && d->pad != NULL;
}

int rgd3_gamepad_master_device(void)
{
	int i;
	for (i = 0; i < RGD3_MAX_JOYSTICKS; i++)
	{
		if (rgd3_gamepad.devices[i].pad)
			return i;
	}
	return -1;
}

/* Focus loss: buffered events and remembered states are dropped without
   posting releases (the key and usercmd states are cleared by the caller).
   SDL sends no pad events while the window is in the background, so
   rgd3_gamepad_resync picks up whatever is held when focus returns. */
void rgd3_gamepad_focus_lost(void)
{
	int i;
	for (i = 0; i < RGD3_MAX_JOYSTICKS; i++)
	{
		Rgd3GamepadDevice* d = &rgd3_gamepad.devices[i];
		d->num_events = 0;
		memset(d->button_states, 0, sizeof(d->button_states));
		memset(d->joy_axis, 0, sizeof(d->joy_axis));
		memset(d->last_value, 0, sizeof(d->last_value));
	}
	rgd3_gamepad.num_polled = 0;
}

/* Posts the pad's current physical state for every control that is held
   now or was held before: input is edge-based, so after input is cleared
   (focus regained, the active pad changed) controls still held would
   otherwise stay unseen until they move. */
void rgd3_gamepad_resync(int device)
{
	Rgd3GamepadDevice* d = rgd3_gamepad_device(device);
	int b, a;
	if (!d || !d->pad || !rgd3_gamepad_enabled())
		return;
	for (b = 0; b < SDL_GAMEPAD_BUTTON_COUNT; b++)
	{
		int event = rgd3_gamepad_button_event(b);
		int value;
		if (event < 0)
			continue;
		value = SDL_GetGamepadButton(d->pad, (SDL_GamepadButton)b) ? 1 : 0;
		if (value || d->last_value[event])
			rgd3_gamepad_post_input_event(device, event, value);
	}
	for (a = 0; a < SDL_GAMEPAD_AXIS_COUNT && a < RGD3_MAX_JOYSTICK_AXIS; a++)
	{
		int event = RGD3_J_AXIS_MIN + a;
		int value = SDL_GetGamepadAxis(d->pad, (SDL_GamepadAxis)a);
		if (value < -32767)
			value = -32767;
		if (value || d->last_value[event])
			rgd3_gamepad_post_input_event(device, event, value);
	}
}

void rgd3_gamepad_resync_all(void)
{
	int i;
	for (i = 0; i < RGD3_MAX_JOYSTICKS; i++)
		rgd3_gamepad_resync(i);
}

/* Sys_PollJoystickInputEvents */
int rgd3_gamepad_poll_input_events(int device)
{
	Rgd3GamepadDevice* d = rgd3_gamepad_device(device);
	rgd3_gamepad.num_polled = 0;
	if (!d)
		return 0;
	rgd3_gamepad_enabled();
	memcpy(rgd3_gamepad.polled, d->events, sizeof(d->events[0]) * (size_t)d->num_events);
	rgd3_gamepad.num_polled = d->num_events;
	d->num_events = 0;
	return rgd3_gamepad.num_polled;
}

/* Sys_ReturnJoystickInputEvent */
int rgd3_gamepad_return_input_event(int n, int* action, int* value)
{
	if (n < 0 || n >= rgd3_gamepad.num_polled)
		return 0;
	*action = rgd3_gamepad.polled[n].event;
	*value = rgd3_gamepad.polled[n].value;
	return 1;
}

/* Sys_EndJoystickInputEvents */
void rgd3_gamepad_end_input_events(void)
{
	rgd3_gamepad.num_polled = 0;
}

/* Sys_SetRumble */
void rgd3_gamepad_set_rumble(int device, int low, int high)
{
	Rgd3GamepadDevice* d = rgd3_gamepad_device(device);
	if (!d || !d->pad)
		return;
	low = rgd3_math_clamp_int(0, 65535, low);
	high = rgd3_math_clamp_int(0, 65535, high);
	/* stopped pads stay stopped without a call per frame */
	if (!low && !high && !d->rumbling)
		return;
	d->rumbling = low || high;
	if (!SDL_RumbleGamepad(d->pad, (Uint16)low, (Uint16)high, RGD3_GAMEPAD_RUMBLE_MS) && !d->rumble_warned)
	{
		/* pads without motors fail every call: warn once */
		d->rumble_warned = 1;
		rgd3_warning("gamepad %d rumble: %s", device, SDL_GetError());
	}
}

void rgd3_gamepad_frame_rumble(int master_device, int low, int high, int shell_active)
{
	int i;
	if (rgd3_gamepad_enabled() && cv_in_joystickRumble.integer_value && !shell_active && master_device >= 0)
	{
		/* only set the rumble on the active controller */
		rgd3_gamepad_set_rumble(master_device, low, high);
		return;
	}
	for (i = 0; i < RGD3_MAX_JOYSTICKS; i++)
		rgd3_gamepad_set_rumble(i, 0, 0);
}
