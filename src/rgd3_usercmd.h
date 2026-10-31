/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_usercmd.h - player input sampled into user commands (usercmd_t,
   idUsercmdGen, idUserCmdMgr: neo/framework/UsercmdGen.h/.cpp).

   The generator is per machine: it samples this machine's keyboard, mouse
   and one gamepad. The manager holds a ring of commands per player slot;
   game code reads commands for an explicit player and never assumes a
   local one. */
#ifndef RGD3_USERCMD_H
#define RGD3_USERCMD_H
#ifndef RGD3_ALL_C
#error "rgd3 headers are included through src/all.c only"
#endif

#define RGD3_MAX_PLAYERS 8 /* MAX_PLAYERS: entity numbers 0..7 are player slots */

/* usercmd_t->buttons bits */
#define RGD3_BUTTON_ATTACK RGD3_BIT(0)
#define RGD3_BUTTON_RUN RGD3_BIT(1)
#define RGD3_BUTTON_ZOOM RGD3_BIT(2)
#define RGD3_BUTTON_SCORES RGD3_BIT(3)
#define RGD3_BUTTON_USE RGD3_BIT(4)
#define RGD3_BUTTON_JUMP RGD3_BIT(5)
#define RGD3_BUTTON_CROUCH RGD3_BIT(6)
#define RGD3_BUTTON_CHATTING RGD3_BIT(7)

/* usercmd_t->impulse commands */
#define RGD3_IMPULSE_0 0 /* weap 0 */
#define RGD3_IMPULSE_1 1 /* weap 1 */
#define RGD3_IMPULSE_2 2 /* weap 2 */
#define RGD3_IMPULSE_3 3 /* weap 3 */
#define RGD3_IMPULSE_4 4 /* weap 4 */
#define RGD3_IMPULSE_5 5 /* weap 5 */
#define RGD3_IMPULSE_6 6 /* weap 6 */
#define RGD3_IMPULSE_7 7 /* weap 7 */
#define RGD3_IMPULSE_8 8 /* weap 8 */
#define RGD3_IMPULSE_9 9 /* weap 9 */
#define RGD3_IMPULSE_10 10 /* weap 10 */
#define RGD3_IMPULSE_11 11 /* weap 11 */
#define RGD3_IMPULSE_12 12 /* weap 12 */
#define RGD3_IMPULSE_13 13 /* weap reload */
#define RGD3_IMPULSE_14 14 /* weap next */
#define RGD3_IMPULSE_15 15 /* weap prev */
#define RGD3_IMPULSE_16 16 /* toggle flashlight on/off */
#define RGD3_IMPULSE_18 18 /* center view */
#define RGD3_IMPULSE_19 19 /* show PDA/SCORES */
#define RGD3_IMPULSE_22 22 /* spectate */
#define RGD3_IMPULSE_25 25 /* Envirosuit light */
#define RGD3_IMPULSE_27 27 /* Chainsaw */
#define RGD3_IMPULSE_28 28 /* quick 0 */
#define RGD3_IMPULSE_29 29 /* quick 1 */
#define RGD3_IMPULSE_30 30 /* quick 2 */
#define RGD3_IMPULSE_31 31 /* quick 3 */

/* usercmd_t: same field order and types as the original (44 bytes) */
typedef struct Rgd3Usercmd
{
	/* synchronized */
	i16 angles[3]; /* view angles (ANGLE2SHORT) */
	i8 forwardmove; /* forward/backward movement */
	i8 rightmove; /* left/right movement */
	u8 buttons; /* RGD3_BUTTON_* */
	int client_game_milliseconds; /* time this usercmd was sent from the client */
	int server_game_milliseconds; /* interpolated server time this was applied on */
	u16 fire_count; /* number of times we've fired */

	/* not synchronized */
	u8 impulse; /* impulse command */
	u8 impulse_sequence; /* incremented every time there's a new impulse */

	i16 mx; /* mouse delta x (continuous position, for GUIs) */
	i16 my; /* mouse delta y */

	/* clients are authoritative on their positions */
	Rgd3Vec3 pos;
	float speed_squared;
} Rgd3Usercmd;

/* inhibit_t */
typedef enum Rgd3InhibitReason
{
	RGD3_INHIBIT_SESSION = 0,
	RGD3_INHIBIT_ASYNC
} Rgd3InhibitReason;

/* usercmdButton_t */
typedef enum Rgd3UsercmdButton
{
	RGD3_UB_NONE,

	RGD3_UB_MOVEUP,
	RGD3_UB_MOVEDOWN,
	RGD3_UB_LOOKLEFT,
	RGD3_UB_LOOKRIGHT,
	RGD3_UB_MOVEFORWARD,
	RGD3_UB_MOVEBACK,
	RGD3_UB_LOOKUP,
	RGD3_UB_LOOKDOWN,
	RGD3_UB_MOVELEFT,
	RGD3_UB_MOVERIGHT,

	RGD3_UB_ATTACK,
	RGD3_UB_SPEED,
	RGD3_UB_ZOOM,
	RGD3_UB_SHOWSCORES,
	RGD3_UB_USE,

	RGD3_UB_IMPULSE0,
	RGD3_UB_IMPULSE31 = RGD3_UB_IMPULSE0 + 31,

	RGD3_UB_MAX_BUTTONS
} Rgd3UsercmdButton;

/* idGame::GetAimAssistAngles / GetAimAssistSensitivity, installed by the
   game once it exists; without them aim assist adds nothing and scales by 1 */
typedef void (*Rgd3AimAssistAnglesFn)(float angles[3]);
typedef float (*Rgd3AimAssistSensitivityFn)(void);

/* ---- usercmd_t ---- */

void rgd3_usercmd_zero(Rgd3Usercmd* cmd);
/* operator==: speed_squared and pos do not take part (the original compares
   speedSquared with itself) */
int rgd3_usercmd_equal(const Rgd3Usercmd* a, const Rgd3Usercmd* b);

/* ---- generator (idUsercmdGen) ---- */

void rgd3_usercmd_register(void);
void rgd3_usercmd_init(void);
void rgd3_usercmd_shutdown(void);
/* prepares for a new map: impulses, toggles, key states and angles reset */
void rgd3_usercmd_init_for_new_map(void);
/* clears all key states and the inhibit flags (not the angles) */
void rgd3_usercmd_clear(void);
void rgd3_usercmd_clear_angles(void);
/* while inhibited only an empty command is built (console, menus);
   reasons are OR'ed */
void rgd3_usercmd_inhibit(int reason, int inhibit);
/* the RGD3_UB_* a binding string maps to ("_forward"), RGD3_UB_NONE if none */
int rgd3_usercmd_command_string_data(const char* cmd_string);
/* idKeyInput::GetUsercmdAction: the RGD3_UB_* of the key's binding */
int rgd3_usercmd_get_key_action(int keynum);

/* Every RGD3_SE_KEY from the event loop must be forwarded here (keyboard and
   mouse buttons/wheel; K_JOY* keys are ignored, pads come in through
   rgd3_gamepad). Events are queued and applied in the next build, like the
   original's buffered keyboard and mouse polls. */
void rgd3_usercmd_key_event(int keynum, int down);

/* called once per frame: samples input into the current command. device_num
   is the gamepad slot (rgd3_gamepad_master_device()), -1 for none */
void rgd3_usercmd_build_current(int device_num);
Rgd3Usercmd rgd3_usercmd_get_current(void);

/* for full screen GUIs: continuous mouse position, last mouse button */
void rgd3_usercmd_mouse_state(int* x, int* y, int* button, int* down);
/* 1 if the RGD3_UB_* button is held, 0 if not, -1 if out of range */
int rgd3_usercmd_button_state(int button);
/* 1 if the generator sees the key down, 0 if not, -1 if out of range */
int rgd3_usercmd_key_state(int keynum);

void rgd3_usercmd_set_aim_assist(Rgd3AimAssistAnglesFn angles_fn, Rgd3AimAssistSensitivityFn sensitivity_fn);

int rgd3_usercmd_self_test(void);

/* ---- per-player command ring (idUserCmdMgr) ---- */

#define RGD3_USERCMD_BUFFER_SIZE 128

typedef struct Rgd3UsercmdMgr
{
	Rgd3Usercmd cmd_buffer[RGD3_USERCMD_BUFFER_SIZE][RGD3_MAX_PLAYERS];
	int write_frame[RGD3_MAX_PLAYERS]; /* where we write to next */
	int read_frame[RGD3_MAX_PLAYERS]; /* the last frame we read */
} Rgd3UsercmdMgr;

/* SetDefaults: every player empty */
void rgd3_usercmd_mgr_clear(Rgd3UsercmdMgr* mgr);
void rgd3_usercmd_mgr_reset_player(Rgd3UsercmdMgr* mgr, int player);
void rgd3_usercmd_mgr_put(Rgd3UsercmdMgr* mgr, int player, const Rgd3Usercmd* cmd);
/* a command after the last one read (+ buffer) is waiting */
int rgd3_usercmd_mgr_has(const Rgd3UsercmdMgr* mgr, int player, int buffer);
/* the unread commands cover more than millisecond_buffer of client time */
int rgd3_usercmd_mgr_has_for_client_time_buffer(const Rgd3UsercmdMgr* mgr, int player, int millisecond_buffer);
const Rgd3Usercmd* rgd3_usercmd_mgr_newest(const Rgd3UsercmdMgr* mgr, int player);
/* the next command to process: advances the read frame unless it would pass
   the last write, so a read without a new write returns the last command */
const Rgd3Usercmd* rgd3_usercmd_mgr_get(Rgd3UsercmdMgr* mgr, int player);
/* same, writable (the player injects its position) */
Rgd3Usercmd* rgd3_usercmd_mgr_get_writable(Rgd3UsercmdMgr* mgr, int player);
int rgd3_usercmd_mgr_next_client_time(const Rgd3UsercmdMgr* mgr, int player);
/* as if every waiting command was read; has() is 0 afterwards */
void rgd3_usercmd_mgr_make_read_ptr_current(Rgd3UsercmdMgr* mgr, int player);
/* like make_read_ptr_current but keeps the newest command unread */
void rgd3_usercmd_mgr_skip_buffered(Rgd3UsercmdMgr* mgr, int player);
int rgd3_usercmd_mgr_num_unread(const Rgd3UsercmdMgr* mgr, int player);
/* pointers to the newest min(buffer_size, RGD3_USERCMD_BUFFER_SIZE)
   commands, oldest first; returns the count */
int rgd3_usercmd_mgr_get_player_cmds(Rgd3UsercmdMgr* mgr, int player, Rgd3Usercmd** buffer, int buffer_size);

#endif
