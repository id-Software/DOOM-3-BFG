/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_usercmd.c - the usercmd generator and the per-player command ring
   (idUsercmdGenLocal and idUserCmdMgr, neo/framework/UsercmdGen.cpp/.h;
   GetUsercmdAction from neo/framework/KeyInput.cpp). Cvar names, defaults
   and flags, the userCmdStrings table, buttonState_t toggles, MouseMove's
   8-sample smoothing, AdjustAngles, KeyMove, CmdButtons, CircleToSquare,
   HandleJoystickAxis, JoystickMove, JoystickMove2 and JoypadFunction follow
   the original.

   Differences from the original:
   - Keyboard and mouse buttons arrive as RGD3_SE_KEY events that the event
     loop forwards to rgd3_usercmd_key_event; they are queued and replayed in
     arrival order inside the build, where the original polled its own
     DirectInput keyboard and mouse buffers (Keyboard(), Mouse()). Mouse
     motion comes from rgd3_sysevents_poll_mouse.
   - in_alwaysRun and in_toggleRun only applied when common->IsMultiplayer(),
     which here means deathmatch rules; see competitive_rules below.
   - Game callbacks for aim assist are function pointers set by the game.
   - KeyMove adds the key movement to the joystick movement with a clamp; the
     original added two clamped chars into a char, which wraps (full stick
     plus key gave -2).
   - When the joystick is not polled (no device, in_useJoystick 0) the
     generator releases its K_JOY keys and centres its axes, so a pad that
     goes away mid-press does not keep moving the player.
   - rgd3_usercmd_mgr_get before any write returns the zeroed slot 0
     instead of reading index -1.
   - usercmd_t::Serialize and ByteSwap wait for the bit message / network
     code (M11); DrawJoypadTexture had no callers and is not ported. */

#define RGD3_KEY_MOVESPEED 127
#define RGD3_USERCMD_MAX_KEY_EVENTS 256

static Rgd3Cvar cv_joy_mergedThreshold = RGD3_CVAR("joy_mergedThreshold", "1", RGD3_CVAR_BOOL | RGD3_CVAR_ARCHIVE, "If the thresholds aren't merged, you drift more off center");
static Rgd3Cvar cv_joy_newCode = RGD3_CVAR("joy_newCode", "1", RGD3_CVAR_BOOL | RGD3_CVAR_ARCHIVE, "Use the new codepath");
static Rgd3Cvar cv_joy_triggerThreshold = RGD3_CVAR("joy_triggerThreshold", "0.05", RGD3_CVAR_FLOAT | RGD3_CVAR_ARCHIVE, "how far the joystick triggers have to be pressed before they register as down");
static Rgd3Cvar cv_joy_deadZone = RGD3_CVAR("joy_deadZone", "0.2", RGD3_CVAR_FLOAT | RGD3_CVAR_ARCHIVE, "specifies how large the dead-zone is on the joystick");
static Rgd3Cvar cv_joy_range = RGD3_CVAR("joy_range", "1.0", RGD3_CVAR_FLOAT | RGD3_CVAR_ARCHIVE, "allow full range to be mapped to a smaller offset");
static Rgd3Cvar cv_joy_gammaLook = RGD3_CVAR("joy_gammaLook", "1", RGD3_CVAR_INTEGER | RGD3_CVAR_ARCHIVE, "use a log curve instead of a power curve for movement");
static Rgd3Cvar cv_joy_powerScale = RGD3_CVAR("joy_powerScale", "2", RGD3_CVAR_FLOAT | RGD3_CVAR_ARCHIVE, "Raise joystick values to this power");
static Rgd3Cvar cv_joy_pitchSpeed = RGD3_CVAR_RANGE("joy_pitchSpeed", "100", RGD3_CVAR_ARCHIVE | RGD3_CVAR_FLOAT, "pitch speed when pressing up or down on the joystick", 60, 600);
static Rgd3Cvar cv_joy_yawSpeed = RGD3_CVAR_RANGE("joy_yawSpeed", "240", RGD3_CVAR_ARCHIVE | RGD3_CVAR_FLOAT, "pitch speed when pressing left or right on the joystick", 60, 600);

/* these were a bad idea! */
static Rgd3Cvar cv_joy_dampenLook = RGD3_CVAR("joy_dampenLook", "1", RGD3_CVAR_BOOL | RGD3_CVAR_ARCHIVE, "Do not allow full acceleration on look");
static Rgd3Cvar cv_joy_deltaPerMSLook = RGD3_CVAR("joy_deltaPerMSLook", "0.003", RGD3_CVAR_FLOAT | RGD3_CVAR_ARCHIVE, "Max amount to be added on look per MS");

static Rgd3Cvar cv_in_mouseSpeed = RGD3_CVAR_RANGE("in_mouseSpeed", "1", RGD3_CVAR_ARCHIVE | RGD3_CVAR_FLOAT, "speed at which the mouse moves", 0.25f, 4.0f);
static Rgd3Cvar cv_in_alwaysRun = RGD3_CVAR("in_alwaysRun", "1", RGD3_CVAR_SYSTEM | RGD3_CVAR_ARCHIVE | RGD3_CVAR_BOOL, "always run (reverse _speed button) - only in MP");
static Rgd3Cvar cv_in_invertLook = RGD3_CVAR("in_invertLook", "0", RGD3_CVAR_ARCHIVE | RGD3_CVAR_BOOL, "inverts the look controls so the forward looks up (flight controls) - the proper way to play games!");
static Rgd3Cvar cv_in_mouseInvertLook = RGD3_CVAR("in_mouseInvertLook", "0", RGD3_CVAR_ARCHIVE | RGD3_CVAR_BOOL, "inverts the look controls so the forward looks up (flight controls) - the proper way to play games!");

static Rgd3Cvar cv_in_yawSpeed = RGD3_CVAR("in_yawspeed", "140", RGD3_CVAR_SYSTEM | RGD3_CVAR_ARCHIVE | RGD3_CVAR_FLOAT, "yaw change speed when holding down _left or _right button");
static Rgd3Cvar cv_in_pitchSpeed = RGD3_CVAR("in_pitchspeed", "140", RGD3_CVAR_SYSTEM | RGD3_CVAR_ARCHIVE | RGD3_CVAR_FLOAT, "pitch change speed when holding down look _lookUp or _lookDown button");
static Rgd3Cvar cv_in_angleSpeedKey = RGD3_CVAR("in_anglespeedkey", "1.5", RGD3_CVAR_SYSTEM | RGD3_CVAR_ARCHIVE | RGD3_CVAR_FLOAT, "angle change scale when holding down _speed button");
static Rgd3Cvar cv_in_toggleRun = RGD3_CVAR("in_toggleRun", "0", RGD3_CVAR_SYSTEM | RGD3_CVAR_ARCHIVE | RGD3_CVAR_BOOL, "pressing _speed button toggles run on/off - only in MP");
static Rgd3Cvar cv_in_toggleCrouch = RGD3_CVAR("in_toggleCrouch", "0", RGD3_CVAR_SYSTEM | RGD3_CVAR_ARCHIVE | RGD3_CVAR_BOOL, "pressing _movedown button toggles player crouching/standing");
static Rgd3Cvar cv_in_toggleZoom = RGD3_CVAR("in_toggleZoom", "0", RGD3_CVAR_SYSTEM | RGD3_CVAR_ARCHIVE | RGD3_CVAR_BOOL, "pressing _zoom button toggles zoom on/off");
static Rgd3Cvar cv_sensitivity = RGD3_CVAR("sensitivity", "5", RGD3_CVAR_SYSTEM | RGD3_CVAR_ARCHIVE | RGD3_CVAR_FLOAT, "mouse view sensitivity");
static Rgd3Cvar cv_m_pitch = RGD3_CVAR("m_pitch", "0.022", RGD3_CVAR_SYSTEM | RGD3_CVAR_ARCHIVE | RGD3_CVAR_FLOAT, "mouse pitch scale");
static Rgd3Cvar cv_m_yaw = RGD3_CVAR("m_yaw", "0.022", RGD3_CVAR_SYSTEM | RGD3_CVAR_ARCHIVE | RGD3_CVAR_FLOAT, "mouse yaw scale");
static Rgd3Cvar cv_m_smooth = RGD3_CVAR_RANGE("m_smooth", "1", RGD3_CVAR_SYSTEM | RGD3_CVAR_ARCHIVE | RGD3_CVAR_INTEGER, "number of samples blended for mouse viewing", 1, 8);
static Rgd3Cvar cv_m_showMouseRate = RGD3_CVAR("m_showMouseRate", "0", RGD3_CVAR_SYSTEM | RGD3_CVAR_BOOL, "shows mouse movement");

typedef struct Rgd3UserCmdString
{
	const char* string;
	int button;
} Rgd3UserCmdString;

/* userCmdStrings */
static const Rgd3UserCmdString rgd3_user_cmd_strings[] = {
	{ "_moveUp", RGD3_UB_MOVEUP },
	{ "_moveDown", RGD3_UB_MOVEDOWN },
	{ "_left", RGD3_UB_LOOKLEFT },
	{ "_right", RGD3_UB_LOOKRIGHT },
	{ "_forward", RGD3_UB_MOVEFORWARD },
	{ "_back", RGD3_UB_MOVEBACK },
	{ "_lookUp", RGD3_UB_LOOKUP },
	{ "_lookDown", RGD3_UB_LOOKDOWN },
	{ "_moveLeft", RGD3_UB_MOVELEFT },
	{ "_moveRight", RGD3_UB_MOVERIGHT },

	{ "_attack", RGD3_UB_ATTACK },
	{ "_speed", RGD3_UB_SPEED },
	{ "_zoom", RGD3_UB_ZOOM },
	{ "_showScores", RGD3_UB_SHOWSCORES },
	{ "_use", RGD3_UB_USE },

	{ "_impulse0", RGD3_UB_IMPULSE0 + 0 },
	{ "_impulse1", RGD3_UB_IMPULSE0 + 1 },
	{ "_impulse2", RGD3_UB_IMPULSE0 + 2 },
	{ "_impulse3", RGD3_UB_IMPULSE0 + 3 },
	{ "_impulse4", RGD3_UB_IMPULSE0 + 4 },
	{ "_impulse5", RGD3_UB_IMPULSE0 + 5 },
	{ "_impulse6", RGD3_UB_IMPULSE0 + 6 },
	{ "_impulse7", RGD3_UB_IMPULSE0 + 7 },
	{ "_impulse8", RGD3_UB_IMPULSE0 + 8 },
	{ "_impulse9", RGD3_UB_IMPULSE0 + 9 },
	{ "_impulse10", RGD3_UB_IMPULSE0 + 10 },
	{ "_impulse11", RGD3_UB_IMPULSE0 + 11 },
	{ "_impulse12", RGD3_UB_IMPULSE0 + 12 },
	{ "_impulse13", RGD3_UB_IMPULSE0 + 13 },
	{ "_impulse14", RGD3_UB_IMPULSE0 + 14 },
	{ "_impulse15", RGD3_UB_IMPULSE0 + 15 },
	{ "_impulse16", RGD3_UB_IMPULSE0 + 16 },
	{ "_impulse17", RGD3_UB_IMPULSE0 + 17 },
	{ "_impulse18", RGD3_UB_IMPULSE0 + 18 },
	{ "_impulse19", RGD3_UB_IMPULSE0 + 19 },
	{ "_impulse20", RGD3_UB_IMPULSE0 + 20 },
	{ "_impulse21", RGD3_UB_IMPULSE0 + 21 },
	{ "_impulse22", RGD3_UB_IMPULSE0 + 22 },
	{ "_impulse23", RGD3_UB_IMPULSE0 + 23 },
	{ "_impulse24", RGD3_UB_IMPULSE0 + 24 },
	{ "_impulse25", RGD3_UB_IMPULSE0 + 25 },
	{ "_impulse26", RGD3_UB_IMPULSE0 + 26 },
	{ "_impulse27", RGD3_UB_IMPULSE0 + 27 },
	{ "_impulse28", RGD3_UB_IMPULSE0 + 28 },
	{ "_impulse29", RGD3_UB_IMPULSE0 + 29 },
	{ "_impulse30", RGD3_UB_IMPULSE0 + 30 },
	{ "_impulse31", RGD3_UB_IMPULSE0 + 31 },

	{ NULL, RGD3_UB_NONE }
};

/* buttonState_t */
typedef struct Rgd3ButtonState
{
	int on;
	int held;
} Rgd3ButtonState;

typedef struct Rgd3UsercmdKeyEvent
{
	int key;
	int down;
} Rgd3UsercmdKeyEvent;

/* transferFunction_t */
typedef enum Rgd3TransferFunction
{
	RGD3_FUNC_LINEAR,
	RGD3_FUNC_LOGARITHMIC,
	RGD3_FUNC_EXPONENTIAL
} Rgd3TransferFunction;

typedef struct Rgd3UsercmdGen
{
	float viewangles[3];
	int impulse_sequence;
	int impulse;

	Rgd3ButtonState toggled_crouch;
	Rgd3ButtonState toggled_run;
	Rgd3ButtonState toggled_zoom;

	int button_state[RGD3_UB_MAX_BUTTONS];
	u8 key_state[RGD3_K_LAST_KEY];

	int inhibit_commands; /* nonzero when in console or menu locally */
	int initialized;

	Rgd3Usercmd cmd; /* the current cmd being built */

	int continuous_mouse_x, continuous_mouse_y; /* for gui event generation, never zeroed */
	int mouse_button; /* for gui event generation */
	int mouse_down;

	int mouse_dx, mouse_dy; /* added to by mouse events */
	float joystick_axis[RGD3_MAX_JOYSTICK_AXIS]; /* set by joystick events */

	int poll_time;
	int last_poll_time;
	float last_look_value_pitch;
	float last_look_value_yaw;

	int mouse_history[8][2]; /* MouseMove's function statics */
	int mouse_history_counter;

	/* keyboard and mouse button events since the last build */
	Rgd3UsercmdKeyEvent key_events[RGD3_USERCMD_MAX_KEY_EVENTS];
	int num_key_events;
	int last_device; /* active pad of the previous build + 1; 0 = none */

	/* COOP: in_alwaysRun and in_toggleRun took effect only under
	   common->IsMultiplayer(). That is a game-rules question
	   (rgd3_game_rules()), not a net-role one: the original MP was
	   deathmatch, which is stripped. Campaign and co-op keep the single-player
	   behaviour (run while _speed is held), so this stays 0; the self-test
	   sets it to exercise the toggle. */
	int competitive_rules;

	Rgd3AimAssistAnglesFn aim_assist_angles;
	Rgd3AimAssistSensitivityFn aim_assist_sensitivity;
} Rgd3UsercmdGen;

static Rgd3UsercmdGen rgd3_ucg;

/* ===================================================================== */
/* usercmd_t                                                              */
/* ===================================================================== */

void rgd3_usercmd_zero(Rgd3Usercmd* cmd)
{
	memset(cmd, 0, sizeof(*cmd));
}

int rgd3_usercmd_equal(const Rgd3Usercmd* a, const Rgd3Usercmd* b)
{
	return a->buttons == b->buttons && a->forwardmove == b->forwardmove && a->rightmove == b->rightmove &&
		   a->angles[0] == b->angles[0] && a->angles[1] == b->angles[1] && a->angles[2] == b->angles[2] &&
		   a->impulse == b->impulse && a->impulse_sequence == b->impulse_sequence && a->mx == b->mx &&
		   a->my == b->my && a->fire_count == b->fire_count;
}

/* ===================================================================== */
/* buttonState_t                                                          */
/* ===================================================================== */

static void rgd3_button_state_clear(Rgd3ButtonState* b)
{
	b->held = 0;
	b->on = 0;
}

static void rgd3_button_state_set_key_state(Rgd3ButtonState* b, int keystate, int toggle)
{
	if (!toggle)
	{
		b->held = 0;
		b->on = keystate;
	}
	else if (!keystate)
	{
		b->held = 0;
	}
	else if (!b->held)
	{
		b->held = 1;
		b->on ^= 1;
	}
}

/* ===================================================================== */
/* generator                                                              */
/* ===================================================================== */

void rgd3_usercmd_register(void)
{
	static Rgd3Cvar* const cvars[] = {
		&cv_joy_mergedThreshold, &cv_joy_newCode, &cv_joy_triggerThreshold, &cv_joy_deadZone, &cv_joy_range,
		&cv_joy_gammaLook, &cv_joy_powerScale, &cv_joy_pitchSpeed, &cv_joy_yawSpeed, &cv_joy_dampenLook,
		&cv_joy_deltaPerMSLook, &cv_in_mouseSpeed, &cv_in_alwaysRun, &cv_in_invertLook, &cv_in_mouseInvertLook,
		&cv_in_yawSpeed, &cv_in_pitchSpeed, &cv_in_angleSpeedKey, &cv_in_toggleRun, &cv_in_toggleCrouch,
		&cv_in_toggleZoom, &cv_sensitivity, &cv_m_pitch, &cv_m_yaw, &cv_m_smooth, &cv_m_showMouseRate
	};
	rgd3_cvar_register_table(cvars, (int)RG_ARRAY_COUNT(cvars));
	rgd3_gamepad_register();
}

void rgd3_usercmd_inhibit(int reason, int inhibit)
{
	if (reason < 0 || reason > 31)
		return;
	if (inhibit)
		rgd3_ucg.inhibit_commands |= 1 << reason;
	else
		rgd3_ucg.inhibit_commands &= ~(1 << reason);
}

int rgd3_usercmd_button_state(int button)
{
	if (button < 0 || button >= RGD3_UB_MAX_BUTTONS)
		return -1;
	return rgd3_ucg.button_state[button] > 0 ? 1 : 0;
}

int rgd3_usercmd_key_state(int keynum)
{
	if (keynum < 0 || keynum >= RGD3_K_LAST_KEY)
		return -1;
	return rgd3_ucg.key_state[keynum] ? 1 : 0;
}

static int rgd3_usercmd_inhibited(void)
{
	return rgd3_ucg.inhibit_commands != 0;
}

/* the run state from _speed (toggled or held), plus in_alwaysRun under
   competitive rules */
static int rgd3_usercmd_running(void)
{
	return rgd3_ucg.toggled_run.on || (cv_in_alwaysRun.integer_value && rgd3_ucg.competitive_rules);
}

/* AdjustAngles: moves the local angle positions */
static void rgd3_usercmd_adjust_angles(void)
{
	Rgd3UsercmdGen* g = &rgd3_ucg;
	float speed = RGD3_MS2SEC(16);

	if (rgd3_usercmd_running())
		speed *= cv_in_angleSpeedKey.float_value;

	g->viewangles[RGD3_YAW] -= speed * cv_in_yawSpeed.float_value * rgd3_usercmd_button_state(RGD3_UB_LOOKRIGHT);
	g->viewangles[RGD3_YAW] += speed * cv_in_yawSpeed.float_value * rgd3_usercmd_button_state(RGD3_UB_LOOKLEFT);

	g->viewangles[RGD3_PITCH] -= speed * cv_in_pitchSpeed.float_value * rgd3_usercmd_button_state(RGD3_UB_LOOKUP);
	g->viewangles[RGD3_PITCH] += speed * cv_in_pitchSpeed.float_value * rgd3_usercmd_button_state(RGD3_UB_LOOKDOWN);
}

/* KeyMove: sets the usercmd based on key states */
static void rgd3_usercmd_key_move(void)
{
	Rgd3UsercmdGen* g = &rgd3_ucg;
	int forward = 0;
	int side = 0;

	side += RGD3_KEY_MOVESPEED * rgd3_usercmd_button_state(RGD3_UB_MOVERIGHT);
	side -= RGD3_KEY_MOVESPEED * rgd3_usercmd_button_state(RGD3_UB_MOVELEFT);

	forward += RGD3_KEY_MOVESPEED * rgd3_usercmd_button_state(RGD3_UB_MOVEFORWARD);
	forward -= RGD3_KEY_MOVESPEED * rgd3_usercmd_button_state(RGD3_UB_MOVEBACK);

	g->cmd.forwardmove = rgd3_math_clamp_char(g->cmd.forwardmove + rgd3_math_clamp_char(forward));
	g->cmd.rightmove = rgd3_math_clamp_char(g->cmd.rightmove + rgd3_math_clamp_char(side));
}

static void rgd3_usercmd_mouse_move(void)
{
	Rgd3UsercmdGen* g = &rgd3_ucg;
	float mx, my;
	int smooth, i;

	g->mouse_history[g->mouse_history_counter & 7][0] = g->mouse_dx;
	g->mouse_history[g->mouse_history_counter & 7][1] = g->mouse_dy;

	/* allow mouse movement to be smoothed together */
	smooth = cv_m_smooth.integer_value;
	if (smooth < 1)
		smooth = 1;
	if (smooth > 8)
		smooth = 8;
	mx = 0;
	my = 0;
	for (i = 0; i < smooth; i++)
	{
		mx += (float)g->mouse_history[(g->mouse_history_counter - i + 8) & 7][0];
		my += (float)g->mouse_history[(g->mouse_history_counter - i + 8) & 7][1];
	}
	mx /= (float)smooth;
	my /= (float)smooth;

	g->mouse_history_counter++;

	if (rgd3_math_fabs(mx) > 1000 || rgd3_math_fabs(my) > 1000)
	{
		rgd3_dprintf("rgd3_usercmd_mouse_move: Ignoring ridiculous mouse delta.\n");
		mx = my = 0;
	}

	mx *= cv_sensitivity.float_value;
	my *= cv_sensitivity.float_value;

	if (cv_m_showMouseRate.integer_value)
		rgd3_printf("[%3i %3i  = %5.1f %5.1f] ", g->mouse_dx, g->mouse_dy, mx, my);

	g->mouse_dx = 0;
	g->mouse_dy = 0;

	g->viewangles[RGD3_YAW] -= cv_m_yaw.float_value * mx * cv_in_mouseSpeed.float_value;
	g->viewangles[RGD3_PITCH] += cv_m_pitch.float_value * cv_in_mouseSpeed.float_value * (cv_in_mouseInvertLook.integer_value ? -my : my);
}

/* CircleToSquare */
static void rgd3_usercmd_circle_to_square(float* axis_x, float* axis_y)
{
	int flip_x = 0, flip_y = 0, swap = 0;
	float len, axis_y_us, correction_ratio;

	/* bring everything in the first quadrant */
	if (*axis_x < 0.0f)
	{
		flip_x = 1;
		*axis_x *= -1.0f;
	}
	if (*axis_y < 0.0f)
	{
		flip_y = 1;
		*axis_y *= -1.0f;
	}

	/* swap the two axes so we project against the vertical line X = 1 */
	if (*axis_y > *axis_x)
	{
		float tmp = *axis_x;
		*axis_x = *axis_y;
		*axis_y = tmp;
		swap = 1;
	}

	if (*axis_x < 0.001f)
	{
		/* on one of the axes where no correction is needed (the original
		   returns here with the axes still swapped and flipped; they are
		   both near zero) */
		return;
	}

	/* length (max 1.0f at the unit circle) */
	len = rgd3_math_sqrt(*axis_x * *axis_x + *axis_y * *axis_y);
	if (len > 1.0f)
		len = 1.0f;
	/* thales */
	axis_y_us = *axis_y / *axis_x;

	/* use a power curve to shift the correction to happen closer to the unit circle */
	correction_ratio = len * len;
	*axis_x += correction_ratio * (len - *axis_x);
	*axis_y += correction_ratio * (axis_y_us - *axis_y);

	/* go back through the symmetries */
	if (swap)
	{
		float tmp = *axis_x;
		*axis_x = *axis_y;
		*axis_y = tmp;
	}
	if (flip_x)
		*axis_x *= -1.0f;
	if (flip_y)
		*axis_y *= -1.0f;
}

static float rgd3_usercmd_aim_assist_sensitivity(void)
{
	return rgd3_ucg.aim_assist_sensitivity ? rgd3_ucg.aim_assist_sensitivity() : 1.0f;
}

static void rgd3_usercmd_key(int keynum, int down);

/* HandleJoystickAxis */
static void rgd3_usercmd_handle_joystick_axis(int keynum, float unclamped_value, float threshold, int positive)
{
	Rgd3UsercmdGen* g = &rgd3_ucg;
	float value = 0.0f, look_value, dt;
	int pressed = 0, action;

	if (unclamped_value > 0.0f && !positive)
		return;
	if (unclamped_value < 0.0f && positive)
		return;
	if (unclamped_value > threshold)
	{
		value = rgd3_math_fabs((unclamped_value - threshold) / (1.0f - threshold));
		pressed = 1;
	}
	else if (unclamped_value < -threshold)
	{
		value = rgd3_math_fabs((unclamped_value + threshold) / (1.0f - threshold));
		pressed = 1;
	}

	action = rgd3_usercmd_get_key_action(keynum);
	if (action >= RGD3_UB_ATTACK)
	{
		rgd3_usercmd_key(keynum, pressed);
		return;
	}
	if (!pressed)
		return;

	if (cv_joy_gammaLook.integer_value)
		look_value = rgd3_math_pow(1.04712854805f, value * 100.0f) * 0.01f;
	else
		look_value = rgd3_math_pow(value, cv_joy_powerScale.float_value);

	look_value *= rgd3_usercmd_aim_assist_sensitivity();

	dt = RGD3_MS2SEC((float)(g->poll_time - g->last_poll_time));
	switch (action)
	{
		case RGD3_UB_MOVEFORWARD:
		{
			float move = (float)g->cmd.forwardmove + (RGD3_KEY_MOVESPEED * value);
			g->cmd.forwardmove = rgd3_math_clamp_char(rgd3_math_ftoi(move));
			break;
		}
		case RGD3_UB_MOVEBACK:
		{
			float move = (float)g->cmd.forwardmove - (RGD3_KEY_MOVESPEED * value);
			g->cmd.forwardmove = rgd3_math_clamp_char(rgd3_math_ftoi(move));
			break;
		}
		case RGD3_UB_MOVELEFT:
		{
			float move = (float)g->cmd.rightmove - (RGD3_KEY_MOVESPEED * value);
			g->cmd.rightmove = rgd3_math_clamp_char(rgd3_math_ftoi(move));
			break;
		}
		case RGD3_UB_MOVERIGHT:
		{
			float move = (float)g->cmd.rightmove + (RGD3_KEY_MOVESPEED * value);
			g->cmd.rightmove = rgd3_math_clamp_char(rgd3_math_ftoi(move));
			break;
		}
		case RGD3_UB_LOOKUP:
		case RGD3_UB_LOOKDOWN:
		{
			float invert_pitch = cv_in_invertLook.integer_value ? -1.0f : 1.0f;
			if (cv_joy_dampenLook.integer_value)
			{
				float limit = (g->poll_time - g->last_poll_time) * cv_joy_deltaPerMSLook.float_value + g->last_look_value_pitch;
				look_value = look_value < limit ? look_value : limit;
				g->last_look_value_pitch = look_value;
			}
			if (action == RGD3_UB_LOOKUP)
				g->viewangles[RGD3_PITCH] -= dt * look_value * cv_joy_pitchSpeed.float_value * invert_pitch;
			else
				g->viewangles[RGD3_PITCH] += dt * look_value * cv_joy_pitchSpeed.float_value * invert_pitch;
			break;
		}
		case RGD3_UB_LOOKLEFT:
		case RGD3_UB_LOOKRIGHT:
		{
			if (cv_joy_dampenLook.integer_value)
			{
				float limit = (g->poll_time - g->last_poll_time) * cv_joy_deltaPerMSLook.float_value + g->last_look_value_yaw;
				look_value = look_value < limit ? look_value : limit;
				g->last_look_value_yaw = look_value;
			}
			if (action == RGD3_UB_LOOKLEFT)
				g->viewangles[RGD3_YAW] += dt * look_value * cv_joy_yawSpeed.float_value;
			else
				g->viewangles[RGD3_YAW] -= dt * look_value * cv_joy_yawSpeed.float_value;
			break;
		}
		default:
			break;
	}
}

/* JoystickMove: the old code path (joy_newCode 0), sticks as keys */
static void rgd3_usercmd_joystick_move(void)
{
	Rgd3UsercmdGen* g = &rgd3_ucg;
	float threshold = cv_joy_deadZone.float_value;
	float trigger_threshold = cv_joy_triggerThreshold.float_value;
	float axis_y = g->joystick_axis[RGD3_AXIS_LEFT_Y];
	float axis_x = g->joystick_axis[RGD3_AXIS_LEFT_X];

	rgd3_usercmd_circle_to_square(&axis_x, &axis_y);

	rgd3_usercmd_handle_joystick_axis(RGD3_K_JOY_STICK1_UP, axis_y, threshold, 0);
	rgd3_usercmd_handle_joystick_axis(RGD3_K_JOY_STICK1_DOWN, axis_y, threshold, 1);
	rgd3_usercmd_handle_joystick_axis(RGD3_K_JOY_STICK1_LEFT, axis_x, threshold, 0);
	rgd3_usercmd_handle_joystick_axis(RGD3_K_JOY_STICK1_RIGHT, axis_x, threshold, 1);

	axis_y = g->joystick_axis[RGD3_AXIS_RIGHT_Y];
	axis_x = g->joystick_axis[RGD3_AXIS_RIGHT_X];
	rgd3_usercmd_circle_to_square(&axis_x, &axis_y);

	rgd3_usercmd_handle_joystick_axis(RGD3_K_JOY_STICK2_UP, axis_y, threshold, 0);
	rgd3_usercmd_handle_joystick_axis(RGD3_K_JOY_STICK2_DOWN, axis_y, threshold, 1);
	rgd3_usercmd_handle_joystick_axis(RGD3_K_JOY_STICK2_LEFT, axis_x, threshold, 0);
	rgd3_usercmd_handle_joystick_axis(RGD3_K_JOY_STICK2_RIGHT, axis_x, threshold, 1);

	rgd3_usercmd_handle_joystick_axis(RGD3_K_JOY_TRIGGER1, g->joystick_axis[RGD3_AXIS_LEFT_TRIG], trigger_threshold, 1);
	rgd3_usercmd_handle_joystick_axis(RGD3_K_JOY_TRIGGER2, g->joystick_axis[RGD3_AXIS_RIGHT_TRIG], trigger_threshold, 1);
}

/* JoypadFunction: dead zone, range rescale and acceleration of one stick */
static Rgd3Vec2 rgd3_usercmd_joypad_function(Rgd3Vec2 raw, float aim_assist_scale, float threshold, float range,
	int shape, int merged_threshold)
{
	Rgd3Vec2 threshed, rescaled;
	float range_scale, rescaled_len, accelerated, aim_assisted, clamped;

	if (range <= threshold)
		return rgd3_vec2(0.0f, 0.0f);

	if (!merged_threshold)
	{
		/* if the thresholding is performed independently, you can more easily
		   move or look in a pure axial direction without drifting */
		int i;
		for (i = 0; i < 2; i++)
		{
			float v = raw.v[i];
			float t;
			if (v > 0.0f)
				t = (v - threshold) > 0.0f ? (v - threshold) : 0.0f;
			else
				t = (v + threshold) < 0.0f ? (v + threshold) : 0.0f;
			threshed.v[i] = t;
		}
	}
	else
	{
		/* thresholding together is the most predictable in free-form
		   movement, but you tend to slide off axis based on which side your
		   thumb is on the pad */
		float raw_length = rgd3_vec2_length(raw);
		float after_threshold = (raw_length - threshold) > 0.0f ? (raw_length - threshold) : 0.0f;
		Rgd3Vec2 raw_dir = raw;
		rgd3_vec2_normalize(&raw_dir);
		threshed.x = raw_dir.x * after_threshold;
		threshed.y = raw_dir.y * after_threshold;
	}

	/* threshold and range reduce the range of raw values, but we scale them
	   back up to the full 0.0 - 1.0 range */
	range_scale = 1.0f / (range - threshold);
	rescaled.x = threshed.x * range_scale;
	rescaled.y = threshed.y * range_scale;

	rescaled_len = rgd3_vec2_length(rescaled);

	/* if inside the deadband area, return a solid 0,0 */
	if (rescaled_len <= 0.0f)
		return rgd3_vec2(0.0f, 0.0f);

	rgd3_vec2_normalize(&rescaled);

	/* apply the acceleration */
	if (shape == RGD3_FUNC_EXPONENTIAL)
		accelerated = rgd3_math_pow(1.04712854805f, rescaled_len * 100.0f) * 0.01f;
	else if (shape == RGD3_FUNC_LOGARITHMIC)
		accelerated = rgd3_math_pow(rescaled_len, 2.0f);
	else /* RGD3_FUNC_LINEAR */
		accelerated = rescaled_len;

	/* optionally slow down for aim-assist */
	aim_assisted = accelerated * aim_assist_scale;

	clamped = (aim_assisted > 1.0f) ? 1.0f : aim_assisted;

	rescaled.x *= clamped;
	rescaled.y *= clamped;
	return rescaled;
}

/* JoystickMove2: the default path, both sticks as analog vectors */
static void rgd3_usercmd_joystick_move2(void)
{
	Rgd3UsercmdGen* g = &rgd3_ucg;
	const int invert_look = cv_in_invertLook.integer_value;
	const float threshold = cv_joy_deadZone.float_value;
	const float range = cv_joy_range.float_value;
	const int shape = cv_joy_gammaLook.integer_value;
	const int merged_threshold = cv_joy_mergedThreshold.integer_value;
	const float pitch_speed = cv_joy_pitchSpeed.float_value;
	const float yaw_speed = cv_joy_yawSpeed.float_value;
	const float aim_assist = rgd3_usercmd_aim_assist_sensitivity();
	const float trigger_threshold = cv_joy_triggerThreshold.float_value;
	const float dt = RGD3_MS2SEC((float)(g->poll_time - g->last_poll_time));
	Rgd3Vec2 left_raw = rgd3_vec2(g->joystick_axis[RGD3_AXIS_LEFT_X], g->joystick_axis[RGD3_AXIS_LEFT_Y]);
	Rgd3Vec2 right_raw = rgd3_vec2(g->joystick_axis[RGD3_AXIS_RIGHT_X], g->joystick_axis[RGD3_AXIS_RIGHT_Y]);
	Rgd3Vec2 left_mapped, right_mapped;

	/* optional stick swap */
	if (rgd3_usercmd_get_key_action(RGD3_K_JOY_STICK1_LEFT) == RGD3_UB_LOOKLEFT)
	{
		Rgd3Vec2 temp = left_raw;
		left_raw = right_raw;
		right_raw = temp;
	}

	/* optional invert look by inverting the right Y axis */
	if (invert_look)
		right_raw.y = -right_raw.y;

	left_mapped = rgd3_usercmd_joypad_function(left_raw, 1.0f, threshold, range, shape, merged_threshold);
	right_mapped = rgd3_usercmd_joypad_function(right_raw, aim_assist, threshold, range, shape, merged_threshold);

	/* because the player physics' CmdScale scales movement values down so
	   that 1,1 = sqrt(2), sqrt(2), we need to expand our circular values out
	   to a square */
	rgd3_usercmd_circle_to_square(&left_mapped.x, &left_mapped.y);

	/* add on top of mouse / keyboard move values */
	g->cmd.forwardmove = rgd3_math_clamp_char(rgd3_math_ftoi(g->cmd.forwardmove + RGD3_KEY_MOVESPEED * -left_mapped.y));
	g->cmd.rightmove = rgd3_math_clamp_char(rgd3_math_ftoi(g->cmd.rightmove + RGD3_KEY_MOVESPEED * left_mapped.x));

	g->viewangles[RGD3_PITCH] += dt * right_mapped.y * pitch_speed;
	g->viewangles[RGD3_YAW] += dt * -right_mapped.x * yaw_speed;

	rgd3_usercmd_handle_joystick_axis(RGD3_K_JOY_TRIGGER1, g->joystick_axis[RGD3_AXIS_LEFT_TRIG], trigger_threshold, 1);
	rgd3_usercmd_handle_joystick_axis(RGD3_K_JOY_TRIGGER2, g->joystick_axis[RGD3_AXIS_RIGHT_TRIG], trigger_threshold, 1);
}

/* CmdButtons */
static void rgd3_usercmd_cmd_buttons(void)
{
	Rgd3UsercmdGen* g = &rgd3_ucg;
	g->cmd.buttons = 0;

	/* check the attack button */
	if (rgd3_usercmd_button_state(RGD3_UB_ATTACK))
		g->cmd.buttons |= RGD3_BUTTON_ATTACK;

	/* check the use button */
	if (rgd3_usercmd_button_state(RGD3_UB_USE))
		g->cmd.buttons |= RGD3_BUTTON_USE;

	/* check the run button */
	if (rgd3_usercmd_running())
		g->cmd.buttons |= RGD3_BUTTON_RUN;

	/* check the zoom button */
	if (g->toggled_zoom.on)
		g->cmd.buttons |= RGD3_BUTTON_ZOOM;

	if (rgd3_usercmd_button_state(RGD3_UB_MOVEUP))
		g->cmd.buttons |= RGD3_BUTTON_JUMP;
	if (g->toggled_crouch.on)
		g->cmd.buttons |= RGD3_BUTTON_CROUCH;
}

/* InitCurrent: inits the current command for this frame */
static void rgd3_usercmd_init_current(void)
{
	Rgd3UsercmdGen* g = &rgd3_ucg;
	memset(&g->cmd, 0, sizeof(g->cmd));
	g->cmd.impulse_sequence = (u8)g->impulse_sequence;
	g->cmd.impulse = (u8)g->impulse;
	if (cv_in_alwaysRun.integer_value && g->competitive_rules)
		g->cmd.buttons |= RGD3_BUTTON_RUN;
}

/* AimAssist: callback to the game to update the aim assist */
static void rgd3_usercmd_aim_assist(void)
{
	Rgd3UsercmdGen* g = &rgd3_ucg;
	float angles[3] = { 0.0f, 0.0f, 0.0f };
	if (g->aim_assist_angles)
		g->aim_assist_angles(angles);
	g->viewangles[RGD3_YAW] += angles[RGD3_YAW];
	g->viewangles[RGD3_PITCH] += angles[RGD3_PITCH];
	g->viewangles[RGD3_ROLL] += angles[RGD3_ROLL];
}

/* MakeCurrent: creates the current command for this frame */
static void rgd3_usercmd_make_current(void)
{
	Rgd3UsercmdGen* g = &rgd3_ucg;
	float old_angles[3];
	int i;

	memcpy(old_angles, g->viewangles, sizeof(old_angles));

	if (!rgd3_usercmd_inhibited())
	{
		/* update toggled key states */
		rgd3_button_state_set_key_state(&g->toggled_crouch, rgd3_usercmd_button_state(RGD3_UB_MOVEDOWN), cv_in_toggleCrouch.integer_value);
		rgd3_button_state_set_key_state(&g->toggled_run, rgd3_usercmd_button_state(RGD3_UB_SPEED), cv_in_toggleRun.integer_value && g->competitive_rules);
		rgd3_button_state_set_key_state(&g->toggled_zoom, rgd3_usercmd_button_state(RGD3_UB_ZOOM), cv_in_toggleZoom.integer_value);

		/* get basic movement from mouse */
		rgd3_usercmd_mouse_move();

		/* get basic movement from joystick and set key bits;
		   must be done before CmdButtons! */
		if (cv_joy_newCode.integer_value)
			rgd3_usercmd_joystick_move2();
		else
			rgd3_usercmd_joystick_move();

		/* keyboard angle adjustment */
		rgd3_usercmd_adjust_angles();

		/* set button bits */
		rgd3_usercmd_cmd_buttons();

		/* get basic movement from keyboard */
		rgd3_usercmd_key_move();

		/* aim assist */
		rgd3_usercmd_aim_assist();

		/* check to make sure the angles haven't wrapped */
		if (g->viewangles[RGD3_PITCH] - old_angles[RGD3_PITCH] > 90)
			g->viewangles[RGD3_PITCH] = old_angles[RGD3_PITCH] + 90;
		else if (old_angles[RGD3_PITCH] - g->viewangles[RGD3_PITCH] > 90)
			g->viewangles[RGD3_PITCH] = old_angles[RGD3_PITCH] - 90;
	}
	else
	{
		g->mouse_dx = 0;
		g->mouse_dy = 0;
	}

	for (i = 0; i < 3; i++)
		g->cmd.angles[i] = (i16)RGD3_ANGLE2SHORT(g->viewangles[i]);

	g->cmd.mx = (i16)g->continuous_mouse_x;
	g->cmd.my = (i16)g->continuous_mouse_y;

	g->impulse_sequence = g->cmd.impulse_sequence;
	g->impulse = g->cmd.impulse;
}

int rgd3_usercmd_command_string_data(const char* cmd_string)
{
	const Rgd3UserCmdString* ucs;
	if (!cmd_string || cmd_string[0] != '_')
		return RGD3_UB_NONE;
	for (ucs = rgd3_user_cmd_strings; ucs->string; ucs++)
	{
		if (rgd3_str_icmp(cmd_string, ucs->string) == 0)
			return ucs->button;
	}
	return RGD3_UB_NONE;
}

int rgd3_usercmd_get_key_action(int keynum)
{
	return rgd3_usercmd_command_string_data(rgd3_keys_get_binding(keynum));
}

void rgd3_usercmd_init(void)
{
	rgd3_ucg.initialized = 1;
	rgd3_gamepad_init();
}

void rgd3_usercmd_clear_angles(void)
{
	rgd3_ucg.viewangles[0] = 0.0f;
	rgd3_ucg.viewangles[1] = 0.0f;
	rgd3_ucg.viewangles[2] = 0.0f;
}

/* Clear: clears all key states (not the toggles or the angles) */
void rgd3_usercmd_clear(void)
{
	Rgd3UsercmdGen* g = &rgd3_ucg;
	memset(g->button_state, 0, sizeof(g->button_state));
	memset(g->key_state, 0, sizeof(g->key_state));
	memset(g->joystick_axis, 0, sizeof(g->joystick_axis));

	g->inhibit_commands = 0;

	g->mouse_dx = g->mouse_dy = 0;
	g->mouse_button = 0;
	g->mouse_down = 0;

	/* rgd3: key events waiting for the next build are dropped too, or a
	   press queued before the clear would hold its action down again */
	g->num_key_events = 0;
}

void rgd3_usercmd_init_for_new_map(void)
{
	Rgd3UsercmdGen* g = &rgd3_ucg;
	g->impulse_sequence = 0;
	g->impulse = 0;

	rgd3_button_state_clear(&g->toggled_crouch);
	rgd3_button_state_clear(&g->toggled_run);
	rgd3_button_state_clear(&g->toggled_zoom);
	g->toggled_run.on = 0;

	rgd3_usercmd_clear();
	rgd3_usercmd_clear_angles();
}

void rgd3_usercmd_shutdown(void)
{
	rgd3_gamepad_shutdown();
	rgd3_ucg.num_key_events = 0;
	rgd3_ucg.initialized = 0;
}

void rgd3_usercmd_set_aim_assist(Rgd3AimAssistAnglesFn angles_fn, Rgd3AimAssistSensitivityFn sensitivity_fn)
{
	rgd3_ucg.aim_assist_angles = angles_fn;
	rgd3_ucg.aim_assist_sensitivity = sensitivity_fn;
}

/* Key: handles mouse/keyboard/joystick button actions */
static void rgd3_usercmd_key(int keynum, int down)
{
	Rgd3UsercmdGen* g = &rgd3_ucg;
	int action;

	if (keynum <= RGD3_K_NONE || keynum >= RGD3_K_LAST_KEY)
		return;
	down = down != 0;

	/* sanity check, sometimes we get double message :( */
	if (g->key_state[keynum] == (u8)down)
		return;
	g->key_state[keynum] = (u8)down;

	action = rgd3_usercmd_get_key_action(keynum);

	if (down)
	{
		g->button_state[action]++;
		if (!rgd3_usercmd_inhibited())
		{
			if (action >= RGD3_UB_IMPULSE0 && action <= RGD3_UB_IMPULSE31)
			{
				g->cmd.impulse = (u8)(action - RGD3_UB_IMPULSE0);
				g->cmd.impulse_sequence++;
			}
		}
	}
	else
	{
		g->button_state[action]--;
		/* we might have one held down across an app active transition */
		if (g->button_state[action] < 0)
			g->button_state[action] = 0;
	}
}

static int rgd3_usercmd_is_joy_key(int keynum)
{
	return keynum >= RGD3_K_JOY1 && keynum <= RGD3_K_JOY_DPAD_RIGHT;
}

void rgd3_usercmd_key_event(int keynum, int down)
{
	Rgd3UsercmdGen* g = &rgd3_ucg;
	if (keynum <= RGD3_K_NONE || keynum >= RGD3_K_LAST_KEY || rgd3_usercmd_is_joy_key(keynum))
		return;
	if (g->num_key_events == RGD3_USERCMD_MAX_KEY_EVENTS)
	{
		/* a full buffer drops the oldest event, like a DirectInput overflow */
		memmove(&g->key_events[0], &g->key_events[1], sizeof(g->key_events[0]) * (RGD3_USERCMD_MAX_KEY_EVENTS - 1));
		g->num_key_events--;
	}
	g->key_events[g->num_key_events].key = keynum;
	g->key_events[g->num_key_events].down = down != 0;
	g->num_key_events++;
}

/* Mouse() and Keyboard(): motion since the last build, then the queued
   button and key events in arrival order */
static void rgd3_usercmd_mouse_and_keyboard(void)
{
	Rgd3UsercmdGen* g = &rgd3_ucg;
	int dx = 0, dy = 0, i;

	rgd3_sysevents_poll_mouse(&dx, &dy);
	g->mouse_dx += dx;
	g->continuous_mouse_x += dx;
	g->mouse_dy += dy;
	g->continuous_mouse_y += dy;

	for (i = 0; i < g->num_key_events; i++)
	{
		int key = g->key_events[i].key;
		int down = g->key_events[i].down;
		if (key >= RGD3_K_MOUSE1 && key <= RGD3_K_MOUSE8)
		{
			g->mouse_button = key;
			g->mouse_down = down;
		}
		else if (key == RGD3_K_MWHEELDOWN || key == RGD3_K_MWHEELUP)
		{
			g->mouse_button = key;
			g->mouse_down = 1;
		}
		rgd3_usercmd_key(key, down);
	}
	g->num_key_events = 0;
}

/* Joystick() */
static void rgd3_usercmd_joystick(int device_num)
{
	Rgd3UsercmdGen* g = &rgd3_ucg;
	int num_events = rgd3_gamepad_poll_input_events(device_num);
	int i;

	for (i = 0; i < num_events; i++)
	{
		int action, value;
		if (!rgd3_gamepad_return_input_event(i, &action, &value))
			continue;
		if (action >= RGD3_J_ACTION1 && action <= RGD3_J_ACTION_MAX)
		{
			int joy_button = RGD3_K_JOY1 + (action - RGD3_J_ACTION1);
			if (joy_button <= RGD3_K_JOY16)
				rgd3_usercmd_key(joy_button, value != 0);
		}
		else if (action >= RGD3_J_AXIS_MIN && action <= RGD3_J_AXIS_MAX)
		{
			g->joystick_axis[action - RGD3_J_AXIS_MIN] = (float)value / 32767.0f;
		}
		else if (action >= RGD3_J_DPAD_UP && action <= RGD3_J_DPAD_RIGHT)
		{
			rgd3_usercmd_key(RGD3_K_JOY_DPAD_UP + (action - RGD3_J_DPAD_UP), value != 0);
		}
	}

	rgd3_gamepad_end_input_events();
}

/* no joystick this frame: let go of what it was holding */
static void rgd3_usercmd_release_joystick(void)
{
	Rgd3UsercmdGen* g = &rgd3_ucg;
	int k;
	for (k = RGD3_K_JOY1; k <= RGD3_K_JOY_DPAD_RIGHT; k++)
	{
		if (g->key_state[k])
			rgd3_usercmd_key(k, 0);
	}
	memset(g->joystick_axis, 0, sizeof(g->joystick_axis));
}

/* BuildCurrentUsercmd with an explicit clock (the self-test drives it) */
static void rgd3_usercmd_build_at(int device_num, int now_ms)
{
	Rgd3UsercmdGen* g = &rgd3_ucg;
	int use_joystick = rgd3_gamepad_enabled() && device_num >= 0;
	int i;

	g->poll_time = now_ms;
	if (g->poll_time - g->last_poll_time > 100)
		g->last_poll_time = g->poll_time - 100;

	/* initialize current usercmd */
	rgd3_usercmd_init_current();

	/* process the system mouse and keyboard events */
	rgd3_usercmd_mouse_and_keyboard();

	/* rgd3: a different (or no) active pad: its held buttons and stick
	   values belong to the old device, whose releases may never arrive
	   (unplugged) */
	if ((use_joystick ? device_num + 1 : 0) != g->last_device)
	{
		rgd3_usercmd_release_joystick();
		g->last_device = use_joystick ? device_num + 1 : 0;
		/* and controls already held on the new pad count from now on */
		if (use_joystick)
			rgd3_gamepad_resync(device_num);
	}

	/* process the system joystick events */
	if (use_joystick)
		rgd3_usercmd_joystick(device_num);
	else
		rgd3_usercmd_release_joystick();

	/* pads that are not the active one are drained so their events don't
	   pile up (the original polled every device when there was no master) */
	for (i = 0; i < RGD3_MAX_JOYSTICKS; i++)
	{
		if (use_joystick && i == device_num)
			continue;
		rgd3_gamepad_poll_input_events(i);
		rgd3_gamepad_end_input_events();
	}

	/* create the usercmd */
	rgd3_usercmd_make_current();

	g->last_poll_time = g->poll_time;
}

void rgd3_usercmd_build_current(int device_num)
{
	rgd3_usercmd_build_at(device_num, rgd3_time_ms());
}

Rgd3Usercmd rgd3_usercmd_get_current(void)
{
	return rgd3_ucg.cmd;
}

void rgd3_usercmd_mouse_state(int* x, int* y, int* button, int* down)
{
	*x = rgd3_ucg.continuous_mouse_x;
	*y = rgd3_ucg.continuous_mouse_y;
	*button = rgd3_ucg.mouse_button;
	*down = rgd3_ucg.mouse_down;
}

/* ===================================================================== */
/* idUserCmdMgr                                                           */
/* ===================================================================== */

static int rgd3_usercmd_mgr_valid(int player)
{
	RG_ASSERT(player >= 0 && player < RGD3_MAX_PLAYERS);
	return player >= 0 && player < RGD3_MAX_PLAYERS;
}

/* ring slot of a frame number; frame -1 (nothing written yet) reads slot 0 */
static int rgd3_usercmd_mgr_slot(int frame)
{
	return frame < 0 ? 0 : frame % RGD3_USERCMD_BUFFER_SIZE;
}

void rgd3_usercmd_mgr_clear(Rgd3UsercmdMgr* mgr)
{
	int i;
	memset(mgr->cmd_buffer, 0, sizeof(mgr->cmd_buffer));
	for (i = 0; i < RGD3_MAX_PLAYERS; i++)
	{
		mgr->write_frame[i] = 0;
		mgr->read_frame[i] = -1;
	}
}

void rgd3_usercmd_mgr_reset_player(Rgd3UsercmdMgr* mgr, int player)
{
	int i;
	if (!rgd3_usercmd_mgr_valid(player))
		return;
	for (i = 0; i < RGD3_USERCMD_BUFFER_SIZE; i++)
		memset(&mgr->cmd_buffer[i][player], 0, sizeof(Rgd3Usercmd));
	mgr->write_frame[player] = 0;
	mgr->read_frame[player] = -1;
}

void rgd3_usercmd_mgr_put(Rgd3UsercmdMgr* mgr, int player, const Rgd3Usercmd* cmd)
{
	if (!rgd3_usercmd_mgr_valid(player))
		return;
	mgr->cmd_buffer[rgd3_usercmd_mgr_slot(mgr->write_frame[player])][player] = *cmd;
	if (mgr->write_frame[player] - mgr->read_frame[player] + 1 > RGD3_USERCMD_BUFFER_SIZE)
	{
		/* set to middle of buffer as a temp fix until we can catch the
		   client up correctly */
		mgr->read_frame[player] = mgr->write_frame[player] - RGD3_USERCMD_BUFFER_SIZE / 2;
		rgd3_printf("PutUserCmdForPlayer: buffer overflow.\n");
	}
	mgr->write_frame[player]++;
}

int rgd3_usercmd_mgr_has(const Rgd3UsercmdMgr* mgr, int player, int buffer)
{
	if (!rgd3_usercmd_mgr_valid(player))
		return 0;
	/* the last frame we read from (+ buffer) is before the last frame we
	   wrote to (write_frame is where we write to next) */
	return mgr->read_frame[player] + buffer < mgr->write_frame[player] - 1;
}

int rgd3_usercmd_mgr_num_unread(const Rgd3UsercmdMgr* mgr, int player)
{
	if (!rgd3_usercmd_mgr_valid(player))
		return 0;
	return (mgr->write_frame[player] - 1) - mgr->read_frame[player];
}

const Rgd3Usercmd* rgd3_usercmd_mgr_newest(const Rgd3UsercmdMgr* mgr, int player)
{
	int index;
	if (!rgd3_usercmd_mgr_valid(player))
		player = 0;
	index = mgr->write_frame[player] - 1 > 0 ? mgr->write_frame[player] - 1 : 0;
	return &mgr->cmd_buffer[rgd3_usercmd_mgr_slot(index)][player];
}

int rgd3_usercmd_mgr_has_for_client_time_buffer(const Rgd3UsercmdMgr* mgr, int player, int millisecond_buffer)
{
	const Rgd3Usercmd* first_cmd;
	const Rgd3Usercmd* last_cmd;

	/* at least one command in addition to enough commands to cover the buffer */
	if (millisecond_buffer == 0)
		return rgd3_usercmd_mgr_has(mgr, player, 0);
	if (rgd3_usercmd_mgr_num_unread(mgr, player) < 2)
		return 0;

	first_cmd = &mgr->cmd_buffer[rgd3_usercmd_mgr_slot(mgr->read_frame[player] + 1)][player];
	last_cmd = rgd3_usercmd_mgr_newest(mgr, player);
	return last_cmd->client_game_milliseconds - first_cmd->client_game_milliseconds > millisecond_buffer;
}

Rgd3Usercmd* rgd3_usercmd_mgr_get_writable(Rgd3UsercmdMgr* mgr, int player)
{
	if (!rgd3_usercmd_mgr_valid(player))
		player = 0;
	/* we want to never skip over a cmd (unless make_read_ptr_current is
	   called) and never get ahead of the write frame: increment before
	   reading, so the same input is not read twice */
	if (mgr->read_frame[player] < mgr->write_frame[player] - 1)
		mgr->read_frame[player]++;
	return &mgr->cmd_buffer[rgd3_usercmd_mgr_slot(mgr->read_frame[player])][player];
}

const Rgd3Usercmd* rgd3_usercmd_mgr_get(Rgd3UsercmdMgr* mgr, int player)
{
	return rgd3_usercmd_mgr_get_writable(mgr, player);
}

int rgd3_usercmd_mgr_next_client_time(const Rgd3UsercmdMgr* mgr, int player)
{
	if (!rgd3_usercmd_mgr_has(mgr, player, 0))
		return 0;
	return mgr->cmd_buffer[rgd3_usercmd_mgr_slot(mgr->read_frame[player] + 1)][player].client_game_milliseconds;
}

void rgd3_usercmd_mgr_make_read_ptr_current(Rgd3UsercmdMgr* mgr, int player)
{
	/* -1 points at the last written cmd: a read before the next write gets
	   the last write, a write before the next read gets the new write */
	if (!rgd3_usercmd_mgr_valid(player))
		return;
	mgr->read_frame[player] = mgr->write_frame[player] - 1;
}

void rgd3_usercmd_mgr_skip_buffered(Rgd3UsercmdMgr* mgr, int player)
{
	/* has() stays true iff there was at least one fresh cmd; never rolls
	   the read frame back */
	int skip_to;
	if (!rgd3_usercmd_mgr_valid(player))
		return;
	skip_to = mgr->write_frame[player] - 2;
	if (mgr->read_frame[player] < skip_to)
		mgr->read_frame[player] = skip_to;
}

int rgd3_usercmd_mgr_get_player_cmds(Rgd3UsercmdMgr* mgr, int player, Rgd3Usercmd** buffer, int buffer_size)
{
	int count, start, num_cmds, i;
	if (!rgd3_usercmd_mgr_valid(player) || buffer_size <= 0)
		return 0;
	count = buffer_size < RGD3_USERCMD_BUFFER_SIZE ? buffer_size : RGD3_USERCMD_BUFFER_SIZE;
	start = mgr->write_frame[player] - count > 0 ? mgr->write_frame[player] - count : 0;
	num_cmds = mgr->write_frame[player] - start;
	for (i = 0; i < num_cmds; i++)
		buffer[i] = &mgr->cmd_buffer[rgd3_usercmd_mgr_slot(start + i)][player];
	return num_cmds;
}

/* ===================================================================== */
/* self-test                                                              */
/* ===================================================================== */

typedef struct Rgd3UsercmdTest
{
	int checks;
	int failures;
} Rgd3UsercmdTest;

#define RGD3_UCT_CHECK(t, cond) rgd3_usercmd_test_check((t), (cond) != 0, #cond, __LINE__)

static void rgd3_usercmd_test_check(Rgd3UsercmdTest* t, int ok, const char* what, int line)
{
	t->checks++;
	if (!ok)
	{
		t->failures++;
		rgd3_printf("^1usercmd FAILED^7 %s (rgd3_usercmd.c:%d)\n", what, line);
	}
}

static Rgd3Usercmd rgd3_usercmd_test_cmd(int ms)
{
	Rgd3Usercmd cmd;
	rgd3_usercmd_zero(&cmd);
	cmd.client_game_milliseconds = ms;
	cmd.forwardmove = (i8)(ms & 63);
	return cmd;
}

static void rgd3_usercmd_test_mgr(Rgd3UsercmdTest* t)
{
	static const int wrap_players[] = { 1, 3, 6 };
	Rgd3UsercmdMgr* mgr = (Rgd3UsercmdMgr*)rgd3_heap_calloc(sizeof(Rgd3UsercmdMgr), RGD3_TAG_TEMP);
	Rgd3Usercmd* recent[4];
	Rgd3Usercmd cmd;
	int i, p, ok;

	RGD3_UCT_CHECK(t, sizeof(Rgd3Usercmd) == 44);
	rgd3_usercmd_mgr_clear(mgr);
	for (p = 0; p < RGD3_MAX_PLAYERS; p++)
		RGD3_UCT_CHECK(t, !rgd3_usercmd_mgr_has(mgr, p, 0) && rgd3_usercmd_mgr_num_unread(mgr, p) == 0);
	/* a read before any write gets a zeroed command */
	RGD3_UCT_CHECK(t, rgd3_usercmd_mgr_get(mgr, 6)->client_game_milliseconds == 0);

	/* three commands for player 2, one for player 5 */
	for (i = 1; i <= 3; i++)
	{
		cmd = rgd3_usercmd_test_cmd(i);
		rgd3_usercmd_mgr_put(mgr, 2, &cmd);
	}
	cmd = rgd3_usercmd_test_cmd(100);
	rgd3_usercmd_mgr_put(mgr, 5, &cmd);
	RGD3_UCT_CHECK(t, rgd3_usercmd_mgr_num_unread(mgr, 2) == 3 && rgd3_usercmd_mgr_num_unread(mgr, 5) == 1);
	RGD3_UCT_CHECK(t, rgd3_usercmd_mgr_has(mgr, 2, 0) && rgd3_usercmd_mgr_has(mgr, 2, 2) && !rgd3_usercmd_mgr_has(mgr, 2, 3));
	RGD3_UCT_CHECK(t, rgd3_usercmd_mgr_next_client_time(mgr, 2) == 1);
	RGD3_UCT_CHECK(t, rgd3_usercmd_mgr_has_for_client_time_buffer(mgr, 2, 1));
	RGD3_UCT_CHECK(t, !rgd3_usercmd_mgr_has_for_client_time_buffer(mgr, 2, 2));
	RGD3_UCT_CHECK(t, rgd3_usercmd_mgr_get(mgr, 2)->client_game_milliseconds == 1);
	RGD3_UCT_CHECK(t, rgd3_usercmd_mgr_get(mgr, 2)->client_game_milliseconds == 2);
	RGD3_UCT_CHECK(t, rgd3_usercmd_mgr_get(mgr, 2)->client_game_milliseconds == 3);
	RGD3_UCT_CHECK(t, !rgd3_usercmd_mgr_has(mgr, 2, 0));
	/* reading past the last write repeats the last command */
	RGD3_UCT_CHECK(t, rgd3_usercmd_mgr_get(mgr, 2)->client_game_milliseconds == 3);
	RGD3_UCT_CHECK(t, rgd3_usercmd_mgr_newest(mgr, 5)->client_game_milliseconds == 100 && rgd3_usercmd_mgr_has(mgr, 5, 0));
	RGD3_UCT_CHECK(t, !rgd3_usercmd_mgr_has(mgr, 0, 0) && !rgd3_usercmd_mgr_has(mgr, 7, 0));

	/* wrap-around: 300 commands each for players 1, 3 and 6, read as written */
	ok = 1;
	for (i = 0; i < 300; i++)
	{
		for (p = 0; p < (int)RG_ARRAY_COUNT(wrap_players); p++)
		{
			cmd = rgd3_usercmd_test_cmd(i * 10 + wrap_players[p]);
			rgd3_usercmd_mgr_put(mgr, wrap_players[p], &cmd);
		}
		for (p = 0; p < (int)RG_ARRAY_COUNT(wrap_players); p++)
		{
			ok &= rgd3_usercmd_mgr_get(mgr, wrap_players[p])->client_game_milliseconds == i * 10 + wrap_players[p];
			ok &= !rgd3_usercmd_mgr_has(mgr, wrap_players[p], 0);
		}
	}
	RGD3_UCT_CHECK(t, ok);
	RGD3_UCT_CHECK(t, rgd3_usercmd_mgr_get_player_cmds(mgr, 3, recent, 4) == 4);
	RGD3_UCT_CHECK(t, recent[0]->client_game_milliseconds == 2963 && recent[3]->client_game_milliseconds == 2993);
	RGD3_UCT_CHECK(t, rgd3_usercmd_mgr_newest(mgr, 5)->client_game_milliseconds == 100);

	/* make_read_ptr_current skips everything waiting; the next write is read */
	for (i = 0; i < 5; i++)
	{
		cmd = rgd3_usercmd_test_cmd(5000 + i);
		rgd3_usercmd_mgr_put(mgr, 1, &cmd);
	}
	rgd3_usercmd_mgr_make_read_ptr_current(mgr, 1);
	RGD3_UCT_CHECK(t, !rgd3_usercmd_mgr_has(mgr, 1, 0));
	RGD3_UCT_CHECK(t, rgd3_usercmd_mgr_get(mgr, 1)->client_game_milliseconds == 5004);
	cmd = rgd3_usercmd_test_cmd(6000);
	rgd3_usercmd_mgr_put(mgr, 1, &cmd);
	RGD3_UCT_CHECK(t, rgd3_usercmd_mgr_get(mgr, 1)->client_game_milliseconds == 6000);

	/* skip_buffered keeps the newest unread */
	for (i = 0; i < 3; i++)
	{
		cmd = rgd3_usercmd_test_cmd(7000 + i);
		rgd3_usercmd_mgr_put(mgr, 6, &cmd);
	}
	rgd3_usercmd_mgr_skip_buffered(mgr, 6);
	RGD3_UCT_CHECK(t, rgd3_usercmd_mgr_num_unread(mgr, 6) == 1);
	RGD3_UCT_CHECK(t, rgd3_usercmd_mgr_get(mgr, 6)->client_game_milliseconds == 7002);
	rgd3_usercmd_mgr_skip_buffered(mgr, 6);
	RGD3_UCT_CHECK(t, rgd3_usercmd_mgr_num_unread(mgr, 6) == 0);

	/* overflow: the 128th unread command moves the read frame to the middle
	   (frame 63), the 129th is one more */
	rgd3_printf("usercmd: the next PutUserCmdForPlayer overflow line is expected\n");
	for (i = 0; i < RGD3_USERCMD_BUFFER_SIZE + 1; i++)
	{
		cmd = rgd3_usercmd_test_cmd(i + 1);
		rgd3_usercmd_mgr_put(mgr, 4, &cmd);
	}
	RGD3_UCT_CHECK(t, rgd3_usercmd_mgr_num_unread(mgr, 4) == RGD3_USERCMD_BUFFER_SIZE / 2 + 1);
	RGD3_UCT_CHECK(t, rgd3_usercmd_mgr_get(mgr, 4)->client_game_milliseconds == RGD3_USERCMD_BUFFER_SIZE / 2 + 1);
	RGD3_UCT_CHECK(t, rgd3_usercmd_mgr_newest(mgr, 4)->client_game_milliseconds == RGD3_USERCMD_BUFFER_SIZE + 1);

	rgd3_usercmd_mgr_reset_player(mgr, 4);
	RGD3_UCT_CHECK(t, !rgd3_usercmd_mgr_has(mgr, 4, 0) && rgd3_usercmd_mgr_newest(mgr, 4)->client_game_milliseconds == 0);
	RGD3_UCT_CHECK(t, rgd3_usercmd_mgr_newest(mgr, 6)->client_game_milliseconds == 7002);

	rgd3_heap_free(mgr);
}

/* the pair of calls the event loop makes for an RGD3_SE_KEY */
static void rgd3_usercmd_test_key(int keynum, int down)
{
	rgd3_keys_preliminary_key_event(keynum, down);
	rgd3_usercmd_key_event(keynum, down);
}

static int rgd3_usercmd_test_time;

static Rgd3Usercmd rgd3_usercmd_test_build(void)
{
	rgd3_usercmd_test_time += 16;
	rgd3_usercmd_build_at(-1, rgd3_usercmd_test_time);
	return rgd3_usercmd_get_current();
}

static int rgd3_usercmd_test_near(float a, float b)
{
	return fabsf(a - b) < 1e-4f;
}

static float rgd3_usercmd_test_aim_sensitivity(void)
{
	return 1.0f;
}

static void rgd3_usercmd_test_generator(Rgd3UsercmdTest* t)
{
	Rgd3UsercmdGen* g = &rgd3_ucg;
	Rgd3Usercmd cmd, other;
	int seq;
	float yaw, pitch;

	RGD3_UCT_CHECK(t, rgd3_usercmd_command_string_data("_forward") == RGD3_UB_MOVEFORWARD);
	RGD3_UCT_CHECK(t, rgd3_usercmd_command_string_data("_LOOKUP") == RGD3_UB_LOOKUP);
	RGD3_UCT_CHECK(t, rgd3_usercmd_command_string_data("_impulse31") == RGD3_UB_IMPULSE31);
	RGD3_UCT_CHECK(t, rgd3_usercmd_command_string_data("_impulse32") == RGD3_UB_NONE);
	RGD3_UCT_CHECK(t, rgd3_usercmd_command_string_data("say _forward") == RGD3_UB_NONE);

	rgd3_keys_set_binding(RGD3_K_W, "_forward");
	rgd3_keys_set_binding(RGD3_K_S, "_back");
	rgd3_keys_set_binding(RGD3_K_A, "_moveLeft");
	rgd3_keys_set_binding(RGD3_K_D, "_moveRight");
	rgd3_keys_set_binding(RGD3_K_SPACE, "_moveUp");
	rgd3_keys_set_binding(RGD3_K_C, "_moveDown");
	rgd3_keys_set_binding(RGD3_K_MOUSE1, "_attack");
	rgd3_keys_set_binding(RGD3_K_E, "_use");
	rgd3_keys_set_binding(RGD3_K_LSHIFT, "_speed");
	rgd3_keys_set_binding(RGD3_K_Z, "_zoom");
	rgd3_keys_set_binding(RGD3_K_1, "_impulse1");
	rgd3_keys_set_binding(RGD3_K_MWHEELUP, "_impulse14");
	rgd3_keys_set_binding(RGD3_K_LEFTARROW, "_left");
	rgd3_keys_set_binding(RGD3_K_JOY_TRIGGER2, "_attack");
	rgd3_keys_set_binding(RGD3_K_JOY_STICK1_LEFT, "_moveLeft");
	RGD3_UCT_CHECK(t, rgd3_usercmd_get_key_action(RGD3_K_W) == RGD3_UB_MOVEFORWARD);

	rgd3_usercmd_init_for_new_map();
	cmd = rgd3_usercmd_test_build();
	RGD3_UCT_CHECK(t, cmd.forwardmove == 0 && cmd.rightmove == 0 && cmd.buttons == 0 && cmd.impulse_sequence == 0);
	RGD3_UCT_CHECK(t, cmd.angles[0] == 0 && cmd.angles[1] == 0 && cmd.angles[2] == 0);

	/* movement and buttons */
	rgd3_usercmd_test_key(RGD3_K_W, 1);
	rgd3_usercmd_test_key(RGD3_K_D, 1);
	rgd3_usercmd_test_key(RGD3_K_MOUSE1, 1);
	rgd3_usercmd_test_key(RGD3_K_SPACE, 1);
	rgd3_usercmd_test_key(RGD3_K_E, 1);
	cmd = rgd3_usercmd_test_build();
	RGD3_UCT_CHECK(t, cmd.forwardmove == 127 && cmd.rightmove == 127);
	RGD3_UCT_CHECK(t, cmd.buttons == (RGD3_BUTTON_ATTACK | RGD3_BUTTON_JUMP | RGD3_BUTTON_USE));
	RGD3_UCT_CHECK(t, rgd3_usercmd_button_state(RGD3_UB_ATTACK) == 1 && rgd3_usercmd_key_state(RGD3_K_W) == 1);
	RGD3_UCT_CHECK(t, rgd3_usercmd_key_state(RGD3_K_LAST_KEY) == -1 && rgd3_usercmd_button_state(-1) == -1);
	{
		int mx, my, button, down;
		rgd3_usercmd_mouse_state(&mx, &my, &button, &down);
		RGD3_UCT_CHECK(t, button == RGD3_K_MOUSE1 && down == 1);
	}
	rgd3_usercmd_test_key(RGD3_K_S, 1);
	rgd3_usercmd_test_key(RGD3_K_A, 1);
	cmd = rgd3_usercmd_test_build();
	RGD3_UCT_CHECK(t, cmd.forwardmove == 0 && cmd.rightmove == 0);
	rgd3_usercmd_test_key(RGD3_K_W, 0);
	rgd3_usercmd_test_key(RGD3_K_D, 0);
	cmd = rgd3_usercmd_test_build();
	RGD3_UCT_CHECK(t, cmd.forwardmove == -127 && cmd.rightmove == -127);
	rgd3_usercmd_test_key(RGD3_K_S, 0);
	rgd3_usercmd_test_key(RGD3_K_A, 0);
	rgd3_usercmd_test_key(RGD3_K_MOUSE1, 0);
	rgd3_usercmd_test_key(RGD3_K_SPACE, 0);
	rgd3_usercmd_test_key(RGD3_K_E, 0);
	/* a repeated release is ignored */
	rgd3_usercmd_test_key(RGD3_K_E, 0);
	cmd = rgd3_usercmd_test_build();
	RGD3_UCT_CHECK(t, cmd.forwardmove == 0 && cmd.rightmove == 0 && cmd.buttons == 0);
	RGD3_UCT_CHECK(t, rgd3_usercmd_button_state(RGD3_UB_USE) == 0);

	/* crouch held, then toggled */
	rgd3_cvar_set(&cv_in_toggleCrouch, "0", 1);
	rgd3_usercmd_test_key(RGD3_K_C, 1);
	cmd = rgd3_usercmd_test_build();
	RGD3_UCT_CHECK(t, cmd.buttons == RGD3_BUTTON_CROUCH);
	rgd3_usercmd_test_key(RGD3_K_C, 0);
	cmd = rgd3_usercmd_test_build();
	RGD3_UCT_CHECK(t, cmd.buttons == 0);
	rgd3_cvar_set(&cv_in_toggleCrouch, "1", 1);
	rgd3_usercmd_test_key(RGD3_K_C, 1);
	cmd = rgd3_usercmd_test_build();
	RGD3_UCT_CHECK(t, cmd.buttons == RGD3_BUTTON_CROUCH);
	cmd = rgd3_usercmd_test_build();
	RGD3_UCT_CHECK(t, cmd.buttons == RGD3_BUTTON_CROUCH);
	rgd3_usercmd_test_key(RGD3_K_C, 0);
	cmd = rgd3_usercmd_test_build();
	RGD3_UCT_CHECK(t, cmd.buttons == RGD3_BUTTON_CROUCH);
	/* toggles sample the held state once per frame: a tap inside one frame
	   is not seen, as in the original */
	rgd3_usercmd_test_key(RGD3_K_C, 1);
	rgd3_usercmd_test_key(RGD3_K_C, 0);
	cmd = rgd3_usercmd_test_build();
	RGD3_UCT_CHECK(t, cmd.buttons == RGD3_BUTTON_CROUCH);
	rgd3_usercmd_test_key(RGD3_K_C, 1);
	rgd3_usercmd_test_build();
	rgd3_usercmd_test_key(RGD3_K_C, 0);
	cmd = rgd3_usercmd_test_build();
	RGD3_UCT_CHECK(t, cmd.buttons == 0);

	/* zoom toggle */
	rgd3_cvar_set(&cv_in_toggleZoom, "1", 1);
	rgd3_usercmd_test_key(RGD3_K_Z, 1);
	rgd3_usercmd_test_build();
	rgd3_usercmd_test_key(RGD3_K_Z, 0);
	cmd = rgd3_usercmd_test_build();
	RGD3_UCT_CHECK(t, cmd.buttons == RGD3_BUTTON_ZOOM);
	rgd3_usercmd_test_key(RGD3_K_Z, 1);
	rgd3_usercmd_test_build();
	rgd3_usercmd_test_key(RGD3_K_Z, 0);
	cmd = rgd3_usercmd_test_build();
	RGD3_UCT_CHECK(t, cmd.buttons == 0);

	/* run: campaign rules ignore in_toggleRun and in_alwaysRun */
	rgd3_cvar_set(&cv_in_toggleRun, "1", 1);
	rgd3_cvar_set(&cv_in_alwaysRun, "1", 1);
	g->competitive_rules = 0;
	cmd = rgd3_usercmd_test_build();
	RGD3_UCT_CHECK(t, cmd.buttons == 0);
	rgd3_usercmd_test_key(RGD3_K_LSHIFT, 1);
	cmd = rgd3_usercmd_test_build();
	RGD3_UCT_CHECK(t, cmd.buttons == RGD3_BUTTON_RUN);
	rgd3_usercmd_test_key(RGD3_K_LSHIFT, 0);
	cmd = rgd3_usercmd_test_build();
	RGD3_UCT_CHECK(t, cmd.buttons == 0);
	/* toggle run under the original MP rules */
	g->competitive_rules = 1;
	rgd3_cvar_set(&cv_in_alwaysRun, "0", 1);
	rgd3_usercmd_test_key(RGD3_K_LSHIFT, 1);
	cmd = rgd3_usercmd_test_build();
	RGD3_UCT_CHECK(t, cmd.buttons == RGD3_BUTTON_RUN);
	rgd3_usercmd_test_key(RGD3_K_LSHIFT, 0);
	cmd = rgd3_usercmd_test_build();
	RGD3_UCT_CHECK(t, cmd.buttons == RGD3_BUTTON_RUN);
	rgd3_usercmd_test_key(RGD3_K_LSHIFT, 1);
	cmd = rgd3_usercmd_test_build();
	RGD3_UCT_CHECK(t, cmd.buttons == 0);
	rgd3_usercmd_test_key(RGD3_K_LSHIFT, 0);
	rgd3_cvar_set(&cv_in_alwaysRun, "1", 1);
	cmd = rgd3_usercmd_test_build();
	RGD3_UCT_CHECK(t, cmd.buttons == RGD3_BUTTON_RUN);
	g->competitive_rules = 0;
	cmd = rgd3_usercmd_test_build();
	RGD3_UCT_CHECK(t, cmd.buttons == 0);

	/* impulses: the sequence counts every press, the impulse persists */
	cmd = rgd3_usercmd_test_build();
	seq = cmd.impulse_sequence;
	rgd3_usercmd_test_key(RGD3_K_1, 1);
	cmd = rgd3_usercmd_test_build();
	RGD3_UCT_CHECK(t, cmd.impulse == 1 && cmd.impulse_sequence == (u8)(seq + 1));
	rgd3_usercmd_test_key(RGD3_K_1, 0);
	cmd = rgd3_usercmd_test_build();
	RGD3_UCT_CHECK(t, cmd.impulse == 1 && cmd.impulse_sequence == (u8)(seq + 1));
	/* a wheel click is a press and release within one frame */
	rgd3_usercmd_test_key(RGD3_K_MWHEELUP, 1);
	rgd3_usercmd_test_key(RGD3_K_MWHEELUP, 0);
	cmd = rgd3_usercmd_test_build();
	RGD3_UCT_CHECK(t, cmd.impulse == 14 && cmd.impulse_sequence == (u8)(seq + 2));
	rgd3_usercmd_test_key(RGD3_K_1, 1);
	rgd3_usercmd_test_key(RGD3_K_1, 0);
	rgd3_usercmd_test_key(RGD3_K_MWHEELUP, 1);
	rgd3_usercmd_test_key(RGD3_K_MWHEELUP, 0);
	cmd = rgd3_usercmd_test_build();
	RGD3_UCT_CHECK(t, cmd.impulse == 14 && cmd.impulse_sequence == (u8)(seq + 4));
	/* the byte wraps */
	g->impulse_sequence = 255;
	rgd3_usercmd_test_key(RGD3_K_1, 1);
	rgd3_usercmd_test_key(RGD3_K_1, 0);
	cmd = rgd3_usercmd_test_build();
	RGD3_UCT_CHECK(t, cmd.impulse == 1 && cmd.impulse_sequence == 0);

	/* mouse look: yaw -= m_yaw * dx * sensitivity, pitch += m_pitch * dy * sensitivity */
	rgd3_cvar_set(&cv_m_smooth, "1", 1);
	rgd3_cvar_set(&cv_sensitivity, "5", 1);
	rgd3_cvar_set(&cv_m_yaw, "0.022", 1);
	rgd3_cvar_set(&cv_m_pitch, "0.022", 1);
	rgd3_cvar_set(&cv_in_mouseSpeed, "1", 1);
	rgd3_cvar_set(&cv_in_mouseInvertLook, "0", 1);
	rgd3_usercmd_clear_angles();
	g->mouse_dx = 10;
	g->mouse_dy = -4;
	cmd = rgd3_usercmd_test_build();
	RGD3_UCT_CHECK(t, rgd3_usercmd_test_near(g->viewangles[RGD3_YAW], -1.1f));
	RGD3_UCT_CHECK(t, rgd3_usercmd_test_near(g->viewangles[RGD3_PITCH], -0.44f));
	RGD3_UCT_CHECK(t, cmd.angles[RGD3_YAW] == -200 && cmd.angles[RGD3_PITCH] == -80 && cmd.angles[RGD3_ROLL] == 0);
	RGD3_UCT_CHECK(t, g->mouse_dx == 0 && g->mouse_dy == 0);
	/* two-sample smoothing averages with the previous frame */
	rgd3_cvar_set(&cv_m_smooth, "2", 1);
	cmd = rgd3_usercmd_test_build();
	RGD3_UCT_CHECK(t, rgd3_usercmd_test_near(g->viewangles[RGD3_YAW], -1.65f));
	RGD3_UCT_CHECK(t, rgd3_usercmd_test_near(g->viewangles[RGD3_PITCH], -0.66f));
	rgd3_cvar_set(&cv_m_smooth, "1", 1);
	/* inverted mouse look */
	rgd3_cvar_set(&cv_in_mouseInvertLook, "1", 1);
	rgd3_usercmd_clear_angles();
	g->mouse_dy = 10;
	rgd3_usercmd_test_build();
	RGD3_UCT_CHECK(t, rgd3_usercmd_test_near(g->viewangles[RGD3_PITCH], -1.1f));
	rgd3_cvar_set(&cv_in_mouseInvertLook, "0", 1);
	/* ridiculous deltas are ignored */
	rgd3_usercmd_clear_angles();
	g->mouse_dx = 5000;
	rgd3_usercmd_test_build();
	RGD3_UCT_CHECK(t, g->viewangles[RGD3_YAW] == 0.0f);
	/* the pitch change per frame is limited to 90 degrees */
	g->mouse_dy = 999;
	rgd3_usercmd_test_build();
	RGD3_UCT_CHECK(t, g->viewangles[RGD3_PITCH] == 90.0f);
	/* _left turns at in_yawspeed for 16 ms per frame */
	rgd3_usercmd_clear_angles();
	rgd3_usercmd_test_key(RGD3_K_LEFTARROW, 1);
	rgd3_usercmd_test_build();
	RGD3_UCT_CHECK(t, rgd3_usercmd_test_near(g->viewangles[RGD3_YAW], 0.016f * cv_in_yawSpeed.float_value));
	rgd3_usercmd_test_key(RGD3_K_LEFTARROW, 0);
	rgd3_usercmd_test_build();

	/* inhibit: an empty command while the console is up */
	yaw = g->viewangles[RGD3_YAW];
	pitch = g->viewangles[RGD3_PITCH];
	cmd = rgd3_usercmd_get_current();
	seq = cmd.impulse_sequence;
	rgd3_usercmd_inhibit(RGD3_INHIBIT_SESSION, 1);
	rgd3_usercmd_inhibit(RGD3_INHIBIT_ASYNC, 1);
	rgd3_usercmd_test_key(RGD3_K_W, 1);
	rgd3_usercmd_test_key(RGD3_K_MOUSE1, 1);
	rgd3_usercmd_test_key(RGD3_K_1, 1);
	g->mouse_dx = 30;
	cmd = rgd3_usercmd_test_build();
	RGD3_UCT_CHECK(t, cmd.forwardmove == 0 && cmd.buttons == 0 && cmd.impulse_sequence == seq);
	RGD3_UCT_CHECK(t, g->viewangles[RGD3_YAW] == yaw && g->viewangles[RGD3_PITCH] == pitch && g->mouse_dx == 0);
	rgd3_usercmd_inhibit(RGD3_INHIBIT_SESSION, 0);
	cmd = rgd3_usercmd_test_build();
	RGD3_UCT_CHECK(t, cmd.forwardmove == 0 && cmd.buttons == 0);
	rgd3_usercmd_inhibit(RGD3_INHIBIT_ASYNC, 0);
	/* keys pressed while inhibited count once it lifts, impulses do not */
	cmd = rgd3_usercmd_test_build();
	RGD3_UCT_CHECK(t, cmd.forwardmove == 127 && cmd.buttons == RGD3_BUTTON_ATTACK && cmd.impulse_sequence == seq);
	/* clear drops key states and the inhibit flags */
	rgd3_usercmd_inhibit(RGD3_INHIBIT_SESSION, 1);
	rgd3_usercmd_clear();
	cmd = rgd3_usercmd_test_build();
	RGD3_UCT_CHECK(t, cmd.forwardmove == 0 && cmd.buttons == 0 && !rgd3_usercmd_inhibited());
	rgd3_usercmd_test_key(RGD3_K_W, 0);
	rgd3_usercmd_test_key(RGD3_K_MOUSE1, 0);
	rgd3_usercmd_test_key(RGD3_K_1, 0);
	cmd = rgd3_usercmd_test_build();
	RGD3_UCT_CHECK(t, cmd.forwardmove == 0 && cmd.buttons == 0 && rgd3_usercmd_button_state(RGD3_UB_MOVEFORWARD) == 0);

	/* joystick, driven below the event layer */
	rgd3_usercmd_set_aim_assist(NULL, rgd3_usercmd_test_aim_sensitivity);
	rgd3_usercmd_clear_angles();
	g->poll_time = 1050;
	g->last_poll_time = 1000;
	rgd3_usercmd_init_current();
	g->joystick_axis[RGD3_AXIS_LEFT_Y] = -1.0f; /* full forward */
	g->joystick_axis[RGD3_AXIS_RIGHT_X] = 0.6f;
	rgd3_usercmd_joystick_move2();
	RGD3_UCT_CHECK(t, g->cmd.forwardmove == 127 && g->cmd.rightmove == 0);
	/* (0.6 - 0.2) / 0.8 = 0.5, squared (joy_gammaLook 1) = 0.25: 50 ms * 0.25 * 240 */
	RGD3_UCT_CHECK(t, rgd3_usercmd_test_near(g->viewangles[RGD3_YAW], -3.0f));
	/* inside the dead zone nothing moves */
	rgd3_usercmd_clear_angles();
	rgd3_usercmd_init_current();
	g->joystick_axis[RGD3_AXIS_LEFT_Y] = -0.15f;
	g->joystick_axis[RGD3_AXIS_RIGHT_X] = 0.15f;
	rgd3_usercmd_joystick_move2();
	RGD3_UCT_CHECK(t, g->cmd.forwardmove == 0 && g->viewangles[RGD3_YAW] == 0.0f);
	/* a stick bound to _moveLeft swaps nothing; the right trigger fires */
	g->joystick_axis[RGD3_AXIS_RIGHT_TRIG] = 0.9f;
	rgd3_usercmd_joystick_move2();
	rgd3_usercmd_cmd_buttons();
	RGD3_UCT_CHECK(t, (g->cmd.buttons & RGD3_BUTTON_ATTACK) && rgd3_usercmd_key_state(RGD3_K_JOY_TRIGGER2) == 1);
	/* no pad polled: the generator lets go */
	cmd = rgd3_usercmd_test_build();
	RGD3_UCT_CHECK(t, cmd.buttons == 0 && cmd.forwardmove == 0 && rgd3_usercmd_key_state(RGD3_K_JOY_TRIGGER2) == 0);
	RGD3_UCT_CHECK(t, g->joystick_axis[RGD3_AXIS_RIGHT_TRIG] == 0.0f);
	/* CircleToSquare: a diagonal at the unit circle reaches the corner */
	{
		float x = 0.70710678f, y = -0.70710678f;
		rgd3_usercmd_circle_to_square(&x, &y);
		RGD3_UCT_CHECK(t, rgd3_usercmd_test_near(x, 1.0f) && rgd3_usercmd_test_near(y, -1.0f));
	}
	rgd3_usercmd_set_aim_assist(NULL, NULL);

	/* equality ignores pos and speed_squared */
	rgd3_usercmd_zero(&cmd);
	other = cmd;
	other.speed_squared = 3.0f;
	other.pos.x = 1.0f;
	RGD3_UCT_CHECK(t, rgd3_usercmd_equal(&cmd, &other));
	other.impulse_sequence = 1;
	RGD3_UCT_CHECK(t, !rgd3_usercmd_equal(&cmd, &other));
}

typedef struct Rgd3UsercmdTestCvar
{
	Rgd3Cvar* cvar;
	char value[64];
	int flags;
	int modified;
} Rgd3UsercmdTestCvar;

int rgd3_usercmd_self_test(void)
{
	static const int keys[] = {
		RGD3_K_W, RGD3_K_S, RGD3_K_A, RGD3_K_D, RGD3_K_SPACE, RGD3_K_C, RGD3_K_MOUSE1, RGD3_K_E, RGD3_K_LSHIFT,
		RGD3_K_Z, RGD3_K_1, RGD3_K_MWHEELUP, RGD3_K_LEFTARROW, RGD3_K_JOY_TRIGGER2, RGD3_K_JOY_STICK1_LEFT
	};
	Rgd3Cvar* const cvars[] = {
		&cv_in_toggleCrouch, &cv_in_toggleZoom, &cv_in_toggleRun, &cv_in_alwaysRun, &cv_m_smooth, &cv_sensitivity,
		&cv_m_yaw, &cv_m_pitch, &cv_in_mouseSpeed, &cv_in_mouseInvertLook, &cv_m_showMouseRate, &cv_in_yawSpeed,
		&cv_in_invertLook, &cv_joy_newCode, &cv_joy_deadZone, &cv_joy_range, &cv_joy_gammaLook,
		&cv_joy_mergedThreshold, &cv_joy_yawSpeed, &cv_joy_pitchSpeed, &cv_joy_triggerThreshold
	};
	static Rgd3UsercmdGen saved_gen;
	static char saved_bindings[RG_ARRAY_COUNT(keys)][256];
	Rgd3UsercmdTestCvar saved_cvars[RG_ARRAY_COUNT(cvars)];
	Rgd3UsercmdTest t = { 0 };
	int bindings_modified = rgd3_keys_bindings_modified();
	int cvar_modified_flags = rgd3_cvar_modified_flags();
	int i;

	/* save what the test touches */
	saved_gen = rgd3_ucg;
	for (i = 0; i < (int)RG_ARRAY_COUNT(keys); i++)
		rgd3_str_copynz(saved_bindings[i], rgd3_keys_get_binding(keys[i]), sizeof(saved_bindings[i]));
	for (i = 0; i < (int)RG_ARRAY_COUNT(cvars); i++)
	{
		saved_cvars[i].cvar = cvars[i];
		rgd3_str_copynz(saved_cvars[i].value, cvars[i]->value ? cvars[i]->value : "", sizeof(saved_cvars[i].value));
		saved_cvars[i].flags = cvars[i]->flags;
		saved_cvars[i].modified = cvars[i]->modified;
	}
	rgd3_ucg.num_key_events = 0;
	rgd3_usercmd_test_time = 100000;
	/* the expectations assume the shipped defaults */
	for (i = 0; i < (int)RG_ARRAY_COUNT(cvars); i++)
		rgd3_cvar_set(cvars[i], cvars[i]->default_value, 1);

	rgd3_usercmd_test_mgr(&t);
	rgd3_usercmd_test_generator(&t);

	/* restore: keys up, bindings, cvars, generator */
	for (i = 0; i < (int)RG_ARRAY_COUNT(keys); i++)
	{
		rgd3_keys_preliminary_key_event(keys[i], 0);
		rgd3_keys_set_binding(keys[i], saved_bindings[i]);
	}
	if (!bindings_modified)
		rgd3_keys_clear_bindings_modified();
	for (i = 0; i < (int)RG_ARRAY_COUNT(cvars); i++)
	{
		rgd3_cvar_set(saved_cvars[i].cvar, saved_cvars[i].value, 1);
		saved_cvars[i].cvar->flags = saved_cvars[i].flags;
		saved_cvars[i].cvar->modified = saved_cvars[i].modified;
	}
	rgd3_cvar_clear_modified_flags(rgd3_cvar_modified_flags() & ~cvar_modified_flags);
	rgd3_ucg = saved_gen;

	rgd3_printf("usercmd: %d checks, %d failures\n", t.checks, t.failures);
	return t.failures == 0;
}
