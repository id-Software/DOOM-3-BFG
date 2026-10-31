/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_gamepad.h - gamepads on SDL3 in place of the XInput joystick code
   (idJoystickWin32, neo/sys/win32/win_input.cpp; joystick enums from
   neo/sys/sys_public.h). Up to RGD3_MAX_JOYSTICKS pads get fixed device
   slots in connection order. Each pad turns its SDL events into the
   original J_* events, buffered per device for the usercmd generator
   (Sys_PollJoystickInputEvents), and into RGD3_SE_KEY (K_JOY*) and
   RGD3_SE_JOYSTICK events for the event loop (PostInputEvent). */
#ifndef RGD3_GAMEPAD_H
#define RGD3_GAMEPAD_H
#ifndef RGD3_ALL_C
#error "rgd3 headers are included through src/all.c only"
#endif

#define RGD3_MAX_JOYSTICKS 4 /* MAX_JOYSTICKS / MAX_INPUT_DEVICES */

/* joystickAxis_t */
typedef enum Rgd3JoystickAxis
{
	RGD3_AXIS_LEFT_X,
	RGD3_AXIS_LEFT_Y,
	RGD3_AXIS_RIGHT_X,
	RGD3_AXIS_RIGHT_Y,
	RGD3_AXIS_LEFT_TRIG,
	RGD3_AXIS_RIGHT_TRIG,
	RGD3_MAX_JOYSTICK_AXIS
} Rgd3JoystickAxis;

/* sys_jEvents */
typedef enum Rgd3JoyEvent
{
	RGD3_J_ACTION1,
	RGD3_J_ACTION2,
	RGD3_J_ACTION3,
	RGD3_J_ACTION4,
	RGD3_J_ACTION5,
	RGD3_J_ACTION6,
	RGD3_J_ACTION7,
	RGD3_J_ACTION8,
	RGD3_J_ACTION9,
	RGD3_J_ACTION10,
	RGD3_J_ACTION11,
	RGD3_J_ACTION12,
	RGD3_J_ACTION13,
	RGD3_J_ACTION14,
	RGD3_J_ACTION15,
	RGD3_J_ACTION16,
	RGD3_J_ACTION32 = RGD3_J_ACTION1 + 31,
	RGD3_J_ACTION_MAX = RGD3_J_ACTION32,

	RGD3_J_AXIS_MIN,
	RGD3_J_AXIS_LEFT_X = RGD3_J_AXIS_MIN + RGD3_AXIS_LEFT_X,
	RGD3_J_AXIS_LEFT_Y = RGD3_J_AXIS_MIN + RGD3_AXIS_LEFT_Y,
	RGD3_J_AXIS_RIGHT_X = RGD3_J_AXIS_MIN + RGD3_AXIS_RIGHT_X,
	RGD3_J_AXIS_RIGHT_Y = RGD3_J_AXIS_MIN + RGD3_AXIS_RIGHT_Y,
	RGD3_J_AXIS_LEFT_TRIG = RGD3_J_AXIS_MIN + RGD3_AXIS_LEFT_TRIG,
	RGD3_J_AXIS_RIGHT_TRIG = RGD3_J_AXIS_MIN + RGD3_AXIS_RIGHT_TRIG,
	RGD3_J_AXIS_MAX = RGD3_J_AXIS_MIN + RGD3_MAX_JOYSTICK_AXIS - 1,

	RGD3_J_DPAD_UP,
	RGD3_J_DPAD_DOWN,
	RGD3_J_DPAD_LEFT,
	RGD3_J_DPAD_RIGHT,

	RGD3_MAX_JOY_EVENT
} Rgd3JoyEvent;

void rgd3_gamepad_register(void);
void rgd3_gamepad_init(void);
void rgd3_gamepad_shutdown(void);

/* in_useJoystick */
int rgd3_gamepad_enabled(void);
/* lowest connected device slot, -1 when no pad is connected (stands in for
   the sign-in manager's GetMasterInputDevice) */
int rgd3_gamepad_master_device(void);
int rgd3_gamepad_is_connected(int device);

/* Sys_PollJoystickInputEvents / Sys_ReturnJoystickInputEvent /
   Sys_EndJoystickInputEvents: poll moves the device's buffered events into
   the returned list. Axis values are -32767..32767 (triggers 0..32767),
   buttons 1/0. */
int rgd3_gamepad_poll_input_events(int device);
int rgd3_gamepad_return_input_event(int n, int* action, int* value);
void rgd3_gamepad_end_input_events(void);

/* focus loss drops buffered input; resync posts what is held right now */
void rgd3_gamepad_focus_lost(void);
void rgd3_gamepad_resync(int device);
void rgd3_gamepad_resync_all(void);

/* Sys_SetRumble: motor speeds 0..65535 */
void rgd3_gamepad_set_rumble(int device, int low, int high);
/* the rumble part of idCommonLocal::ProcessGameReturn: rumble the master
   device if in_useJoystick and in_joystickRumble allow it and the shell is
   not up, otherwise stop every pad */
void rgd3_gamepad_frame_rumble(int master_device, int low, int high, int shell_active);

#endif
