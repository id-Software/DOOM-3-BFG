/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_keys.h - key numbers and bindings (idKeyInput,
   neo/framework/KeyInput.cpp). Key numbers keep the original keyNum_t values
   (DirectInput scan codes) so bindings in the shipped default.cfg resolve. */
#ifndef RGD3_KEYS_H
#define RGD3_KEYS_H
#ifndef RGD3_ALL_C
#error "rgd3 headers are included through src/all.c only"
#endif

typedef enum Rgd3KeyNum
{
	RGD3_K_NONE,
	RGD3_K_ESCAPE, RGD3_K_1, RGD3_K_2, RGD3_K_3, RGD3_K_4, RGD3_K_5, RGD3_K_6, RGD3_K_7, RGD3_K_8, RGD3_K_9, RGD3_K_0,
	RGD3_K_MINUS, RGD3_K_EQUALS, RGD3_K_BACKSPACE, RGD3_K_TAB,
	RGD3_K_Q, RGD3_K_W, RGD3_K_E, RGD3_K_R, RGD3_K_T, RGD3_K_Y, RGD3_K_U, RGD3_K_I, RGD3_K_O, RGD3_K_P,
	RGD3_K_LBRACKET, RGD3_K_RBRACKET, RGD3_K_ENTER, RGD3_K_LCTRL,
	RGD3_K_A, RGD3_K_S, RGD3_K_D, RGD3_K_F, RGD3_K_G, RGD3_K_H, RGD3_K_J, RGD3_K_K, RGD3_K_L,
	RGD3_K_SEMICOLON, RGD3_K_APOSTROPHE, RGD3_K_GRAVE, RGD3_K_LSHIFT, RGD3_K_BACKSLASH,
	RGD3_K_Z, RGD3_K_X, RGD3_K_C, RGD3_K_V, RGD3_K_B, RGD3_K_N, RGD3_K_M,
	RGD3_K_COMMA, RGD3_K_PERIOD, RGD3_K_SLASH, RGD3_K_RSHIFT, RGD3_K_KP_STAR, RGD3_K_LALT, RGD3_K_SPACE, RGD3_K_CAPSLOCK,
	RGD3_K_F1, RGD3_K_F2, RGD3_K_F3, RGD3_K_F4, RGD3_K_F5, RGD3_K_F6, RGD3_K_F7, RGD3_K_F8, RGD3_K_F9, RGD3_K_F10,
	RGD3_K_NUMLOCK, RGD3_K_SCROLL,
	RGD3_K_KP_7, RGD3_K_KP_8, RGD3_K_KP_9, RGD3_K_KP_MINUS, RGD3_K_KP_4, RGD3_K_KP_5, RGD3_K_KP_6, RGD3_K_KP_PLUS,
	RGD3_K_KP_1, RGD3_K_KP_2, RGD3_K_KP_3, RGD3_K_KP_0, RGD3_K_KP_DOT,
	RGD3_K_F11 = 0x57,
	RGD3_K_F12 = 0x58,
	RGD3_K_F13 = 0x64,
	RGD3_K_F14 = 0x65,
	RGD3_K_F15 = 0x66,
	RGD3_K_KANA = 0x70,
	RGD3_K_CONVERT = 0x79,
	RGD3_K_NOCONVERT = 0x7B,
	RGD3_K_YEN = 0x7D,
	RGD3_K_KP_EQUALS = 0x8D,
	RGD3_K_CIRCUMFLEX = 0x90,
	RGD3_K_AT = 0x91,
	RGD3_K_COLON = 0x92,
	RGD3_K_UNDERLINE = 0x93,
	RGD3_K_KANJI = 0x94,
	RGD3_K_STOP = 0x95,
	RGD3_K_AX = 0x96,
	RGD3_K_UNLABELED = 0x97,
	RGD3_K_KP_ENTER = 0x9C,
	RGD3_K_RCTRL = 0x9D,
	RGD3_K_KP_COMMA = 0xB3,
	RGD3_K_KP_SLASH = 0xB5,
	RGD3_K_PRINTSCREEN = 0xB7,
	RGD3_K_RALT = 0xB8,
	RGD3_K_PAUSE = 0xC5,
	RGD3_K_HOME = 0xC7,
	RGD3_K_UPARROW = 0xC8,
	RGD3_K_PGUP = 0xC9,
	RGD3_K_LEFTARROW = 0xCB,
	RGD3_K_RIGHTARROW = 0xCD,
	RGD3_K_END = 0xCF,
	RGD3_K_DOWNARROW = 0xD0,
	RGD3_K_PGDN = 0xD1,
	RGD3_K_INS = 0xD2,
	RGD3_K_DEL = 0xD3,
	RGD3_K_LWIN = 0xDB,
	RGD3_K_RWIN = 0xDC,
	RGD3_K_APPS = 0xDD,
	RGD3_K_POWER = 0xDE,
	RGD3_K_SLEEP = 0xDF,

	RGD3_K_JOY1 = 256,
	RGD3_K_JOY2, RGD3_K_JOY3, RGD3_K_JOY4, RGD3_K_JOY5, RGD3_K_JOY6, RGD3_K_JOY7, RGD3_K_JOY8,
	RGD3_K_JOY9, RGD3_K_JOY10, RGD3_K_JOY11, RGD3_K_JOY12, RGD3_K_JOY13, RGD3_K_JOY14, RGD3_K_JOY15, RGD3_K_JOY16,
	RGD3_K_JOY_STICK1_UP, RGD3_K_JOY_STICK1_DOWN, RGD3_K_JOY_STICK1_LEFT, RGD3_K_JOY_STICK1_RIGHT,
	RGD3_K_JOY_STICK2_UP, RGD3_K_JOY_STICK2_DOWN, RGD3_K_JOY_STICK2_LEFT, RGD3_K_JOY_STICK2_RIGHT,
	RGD3_K_JOY_TRIGGER1, RGD3_K_JOY_TRIGGER2,
	RGD3_K_JOY_DPAD_UP, RGD3_K_JOY_DPAD_DOWN, RGD3_K_JOY_DPAD_LEFT, RGD3_K_JOY_DPAD_RIGHT,

	RGD3_K_MOUSE1, RGD3_K_MOUSE2, RGD3_K_MOUSE3, RGD3_K_MOUSE4, RGD3_K_MOUSE5, RGD3_K_MOUSE6, RGD3_K_MOUSE7, RGD3_K_MOUSE8,
	RGD3_K_MWHEELDOWN, RGD3_K_MWHEELUP,

	RGD3_K_LAST_KEY
} Rgd3KeyNum;

void rgd3_keys_init(void);
void rgd3_keys_shutdown(void);
void rgd3_keys_register(void);

int rgd3_keys_string_to_keynum(const char* text);
const char* rgd3_keys_keynum_to_string(int keynum);
void rgd3_keys_set_binding(int keynum, const char* binding);
const char* rgd3_keys_get_binding(int keynum);
int rgd3_keys_is_down(int keynum);
void rgd3_keys_preliminary_key_event(int keynum, int down);
int rgd3_keys_exec_key_binding(int keynum);
void rgd3_keys_clear_states(void);
int rgd3_keys_bindings_modified(void);
void rgd3_keys_clear_bindings_modified(void);
void rgd3_keys_write_bindings(Rgd3File* file);

#endif
