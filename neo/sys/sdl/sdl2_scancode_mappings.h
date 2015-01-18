/*
 * Maps SDL2 scancodes to directinput keynums/scancodes.
 * Useful if you're porting a game that uses dinput scancodes internally
 * (for key bindings etc) or any other lib (like CEGUI) that uses them.
 *
 * (C) 2015 Daniel Gibson, do whatever you want with this.
 *
 * No warranty is offered or implied; use this code at your own risk.
 *
 * Released to the Public Domain, do whatever you want with this: use it,
 * modify it, copy it into your own code, ...
 * No need to mention me or this "license" in your code or docs, even though
 * it would be appreciated, of course.
 */


#ifndef _SDL2_SCANCODE_TO_DINPUT_MAPPINGS_H_
#define _SDL2_SCANCODE_TO_DINPUT_MAPPINGS_H_

// TODO: map the following keys, if possible:
// #define DIK_UNDERLINE       0x93    /*                     (NEC PC98) */
// #define DIK_KANJI           0x94    /* (Japanese keyboard)            */
// #define DIK_AX              0x96    /*                     (Japan AX) */
// #define DIK_UNLABELED       0x97    /*                        (J3100) */
//
// #define DIK_WAKE            0xE3    /* System Wake */
//
// (#define DIK_ABNT_C2         0x7E    /* Numpad . on Brazilian keyboard */ - system should map this to KP_COMMA or something,
//                                         according to USB doc, so probably it doesn't need mapping here)

// maps SDL2 scancodes to directinput keynums/scancodes - dinput_key = scanCodeToKeyNum[(int)your_sdl2_scancode];
static int scanCodeToKeyNum[SDL_NUM_SCANCODES] =
{
	0x0, 	//	SDL_SCANCODE_UNKNOWN = 0, => 0 should also work for dinput codes as "not assigned/unknown"
	0x0, 	//	// 1 (unused)
	0x0, 	//	// 2 (unused)
	0x0, 	//	// 3 (unused)
	0x1E,	//	SDL_SCANCODE_A = 4, - DIK_A
	0x30,	//	SDL_SCANCODE_B = 5, - DIK_B
	0x2E,	//	SDL_SCANCODE_C = 6, - DIK_C
	0x20,	//	SDL_SCANCODE_D = 7, - DIK_D
	0x12,	//	SDL_SCANCODE_E = 8, - DIK_E
	0x21,	//	SDL_SCANCODE_F = 9, - DIK_F
	0x22,	//	SDL_SCANCODE_G = 10, - DIK_G
	0x23,	//	SDL_SCANCODE_H = 11, - DIK_H
	0x17,	//	SDL_SCANCODE_I = 12, - DIK_I
	0x24,	//	SDL_SCANCODE_J = 13, - DIK_J
	0x25,	//	SDL_SCANCODE_K = 14, - DIK_K
	0x26,	//	SDL_SCANCODE_L = 15, - DIK_L
	0x32,	//	SDL_SCANCODE_M = 16, - DIK_M
	0x31,	//	SDL_SCANCODE_N = 17, - DIK_N
	0x18,	//	SDL_SCANCODE_O = 18, - DIK_O
	0x19,	//	SDL_SCANCODE_P = 19, - DIK_P
	0x10,	//	SDL_SCANCODE_Q = 20, - DIK_Q
	0x13,	//	SDL_SCANCODE_R = 21, - DIK_R
	0x1F,	//	SDL_SCANCODE_S = 22, - DIK_S
	0x14,	//	SDL_SCANCODE_T = 23, - DIK_T
	0x16,	//	SDL_SCANCODE_U = 24, - DIK_U
	0x2F,	//	SDL_SCANCODE_V = 25, - DIK_V
	0x11,	//	SDL_SCANCODE_W = 26, - DIK_W
	0x2D,	//	SDL_SCANCODE_X = 27, - DIK_X
	0x15,	//	SDL_SCANCODE_Y = 28, - DIK_Y
	0x2C,	//	SDL_SCANCODE_Z = 29, - DIK_Z
	
	0x02,	//	SDL_SCANCODE_1 = 30, - DIK_1
	0x03,	//	SDL_SCANCODE_2 = 31, - DIK_2
	0x04,	//	SDL_SCANCODE_3 = 32, - DIK_3
	0x05,	//	SDL_SCANCODE_4 = 33, - DIK_4
	0x06,	//	SDL_SCANCODE_5 = 34, - DIK_5
	0x07,	//	SDL_SCANCODE_6 = 35, - DIK_6
	0x08,	//	SDL_SCANCODE_7 = 36, - DIK_7
	0x09,	//	SDL_SCANCODE_8 = 37, - DIK_8
	0x0A,	//	SDL_SCANCODE_9 = 38, - DIK_9
	0x0B,	//	SDL_SCANCODE_0 = 39, - DIK_0
	
	0x1C,	//	SDL_SCANCODE_RETURN = 40, - DIK_RETURN
	0x01,	//	SDL_SCANCODE_ESCAPE = 41, - DIK_ESCAPE
	0x0E,	//	SDL_SCANCODE_BACKSPACE = 42, - DIK_BACK
	0x0F,	//	SDL_SCANCODE_TAB = 43, - DIK_TAB
	0x39,	//	SDL_SCANCODE_SPACE = 44, - DIK_SPACE
	
	0x0C,	//	SDL_SCANCODE_MINUS = 45, - DIK_MINUS
	0x0D,	//	SDL_SCANCODE_EQUALS = 46, - DIK_EQUALS
	0x1A,	//	SDL_SCANCODE_LEFTBRACKET = 47, - DIK_LBRACKET
	0x1B,	//	SDL_SCANCODE_RIGHTBRACKET = 48, - DIK_RBRACKET
	0x2B,	//	SDL_SCANCODE_BACKSLASH = 49, // next to enter, US: [\|] DE: [#'] UK: [#~] - DIK_BACKSLASH
	0x2B,	//	SDL_SCANCODE_NONUSHASH = 50, // same key as before actually on some layouts, systems should map this to SDL_SCANCODE_BACKSLASH - DIK_BACKSLASH
	0x27,	//	SDL_SCANCODE_SEMICOLON = 51, - DIK_SEMICOLON
	0x28,	//	SDL_SCANCODE_APOSTROPHE = 52, - DIK_APOSTROPHE
	0x29,	//	SDL_SCANCODE_GRAVE = 53, // "quake/doom3 console key" - DIK_GRAVE
	0x33,	//	SDL_SCANCODE_COMMA = 54, - DIK_COMMA
	0x34,	//	SDL_SCANCODE_PERIOD = 55, - DIK_PERIOD
	0x35,	//	SDL_SCANCODE_SLASH = 56, - DIK_SLASH
	
	0x3A,	//	SDL_SCANCODE_CAPSLOCK = 57, - DIK_CAPITAL
	
	0x3B,	//	SDL_SCANCODE_F1 = 58,  - DIK_F1
	0x3C,	//	SDL_SCANCODE_F2 = 59,  - DIK_F2
	0x3D,	//	SDL_SCANCODE_F3 = 60,  - DIK_F3
	0x3E,	//	SDL_SCANCODE_F4 = 61,  - DIK_F4
	0x3F,	//	SDL_SCANCODE_F5 = 62,  - DIK_F5
	0x40,	//	SDL_SCANCODE_F6 = 63,  - DIK_F6
	0x41,	//	SDL_SCANCODE_F7 = 64,  - DIK_F7
	0x42,	//	SDL_SCANCODE_F8 = 65,  - DIK_F8
	0x43,	//	SDL_SCANCODE_F9 = 66,  - DIK_F9
	0x44,	//	SDL_SCANCODE_F10 = 67, - DIK_F10
	0x57,	//	SDL_SCANCODE_F11 = 68, - DIK_F11
	0x58,	//	SDL_SCANCODE_F12 = 69, - DIK_F12
	
	0xB7,	//	SDL_SCANCODE_PRINTSCREEN = 70, // - DIK_SYSRQ; SDL_SCANCODE_SYSREQ also maps to this!
	
	0x46,	//	SDL_SCANCODE_SCROLLLOCK = 71, - DIK_SCROLL
	0xC5,	//	SDL_SCANCODE_PAUSE = 72, - DIK_PAUSE
	0xD2,	//	SDL_SCANCODE_INSERT = 73, // insert on PC, help on some Mac keyboards (but does send code 73, not 117) - DIK_INSERT
	0xC7,	//	SDL_SCANCODE_HOME = 74, - DIK_HOME
	0xC9,	//	SDL_SCANCODE_PAGEUP = 75, - DIK_PRIOR
	0xD3,	//	SDL_SCANCODE_DELETE = 76, - DIK_DELETE
	0xCF,	//	SDL_SCANCODE_END = 77, - DIK_END
	0xD1,	//	SDL_SCANCODE_PAGEDOWN = 78, - DIK_NEXT
	0xCD,	//	SDL_SCANCODE_RIGHT = 79, - DIK_RIGHT
	0xCB,	//	SDL_SCANCODE_LEFT = 80, - DIK_LEFT
	0xD0,	//	SDL_SCANCODE_DOWN = 81, - DIK_DOWN
	0xC8,	//	SDL_SCANCODE_UP = 82, - DIK_UP
	
	0x45,	//	SDL_SCANCODE_NUMLOCKCLEAR = 83, // num lock on PC, clear on Mac keyboards - DIK_NUMLOCK
	
	0xB5,	//	SDL_SCANCODE_KP_DIVIDE = 84, - DIK_DIVIDE
	0x37,	//	SDL_SCANCODE_KP_MULTIPLY = 85, - DIK_MULTIPLY
	0x4A,	//	SDL_SCANCODE_KP_MINUS = 86, - DIK_SUBTRACT
	0x4E,	//	SDL_SCANCODE_KP_PLUS = 87, - DIK_ADD
	0x9C,	//	SDL_SCANCODE_KP_ENTER = 88, - DIK_NUMPADENTER
	0x4F,	//	SDL_SCANCODE_KP_1 = 89, - DIK_NUMPAD1
	0x50,	//	SDL_SCANCODE_KP_2 = 90, - DIK_NUMPAD2
	0x51,	//	SDL_SCANCODE_KP_3 = 91, - DIK_NUMPAD3
	0x4B,	//	SDL_SCANCODE_KP_4 = 92, - DIK_NUMPAD4
	0x4C,	//	SDL_SCANCODE_KP_5 = 93, - DIK_NUMPAD5
	0x4D,	//	SDL_SCANCODE_KP_6 = 94, - DIK_NUMPAD6
	0x47,	//	SDL_SCANCODE_KP_7 = 95, - DIK_NUMPAD7
	0x48,	//	SDL_SCANCODE_KP_8 = 96, - DIK_NUMPAD8
	0x49,	//	SDL_SCANCODE_KP_9 = 97, - DIK_NUMPAD9
	0x52,	//	SDL_SCANCODE_KP_0 = 98, - DIK_NUMPAD0
	0x53,	//	SDL_SCANCODE_KP_PERIOD = 99, - DIK_DECIMAL
	
	0x56,	//	SDL_SCANCODE_NONUSBACKSLASH = 100, // [<>|] on german keyboard, next to left shift - DIK_OEM_102
	0xDD,	//	SDL_SCANCODE_APPLICATION = 101, // windows contextual menu, compose - DIK_APPS
	0xDE,	//	SDL_SCANCODE_POWER = 102, // should be a status flag, but some mac keyboards have a power key - DIK_POWER
	
	0x8D,	//	SDL_SCANCODE_KP_EQUALS = 103, - DIK_NUMPADEQUALS
	0x64,	//	SDL_SCANCODE_F13 = 104, - DIK_F13
	0x65,	//	SDL_SCANCODE_F14 = 105, - DIK_F14
	0x66,	//	SDL_SCANCODE_F15 = 106, - DIK_F15
	0x67,	//	SDL_SCANCODE_F16 = 107, // TODO: F16 and up don't have DIK_ constants! is this right?
	0x68,	//	SDL_SCANCODE_F17 = 108, //        (at least 0x67-0x6F have no DIK_constants at all)
	0x69,	//	SDL_SCANCODE_F18 = 109,
	0x6A,	//	SDL_SCANCODE_F19 = 110,
	0x6B,	//	SDL_SCANCODE_F20 = 111,
	0x6C,	//	SDL_SCANCODE_F21 = 112,
	0x6D,	//	SDL_SCANCODE_F22 = 113,
	0x6E,	//	SDL_SCANCODE_F23 = 114,
	0x6F,	//	SDL_SCANCODE_F24 = 115,
	
	0x0,	//	SDL_SCANCODE_EXECUTE = 116,
	0x0,	//	SDL_SCANCODE_HELP = 117,
	0x0,	//	SDL_SCANCODE_MENU = 118,
	0x0,	//	SDL_SCANCODE_SELECT = 119,
	
	0x95,	//	SDL_SCANCODE_STOP = 120, - DIK_STOP
	
	0x0,	//	SDL_SCANCODE_AGAIN = 121,  // redo
	0x0,	//	SDL_SCANCODE_UNDO = 122,
	0x0,	//	SDL_SCANCODE_CUT = 123,
	0x0,	//	SDL_SCANCODE_COPY = 124,
	0x0,	//	SDL_SCANCODE_PASTE = 125,
	0x0,	//	SDL_SCANCODE_FIND = 126,
	0x0,	//	SDL_SCANCODE_MUTE = 127,
	
	0xB0,	//	SDL_SCANCODE_VOLUMEUP = 128, - DIK_VOLUMEUP
	0xAE,	//	SDL_SCANCODE_VOLUMEDOWN = 129, - DIK_VOLUMEDOWN
	
	//	/* not sure whether there's a reason to enable these */
	0x0,	//	/*     SDL_SCANCODE_LOCKINGCAPSLOCK = 130,  */
	0x0,	//	/*     SDL_SCANCODE_LOCKINGNUMLOCK = 131, */
	0x0,	//	/*     SDL_SCANCODE_LOCKINGSCROLLLOCK = 132, */
	
	0xB3,	//	SDL_SCANCODE_KP_COMMA = 133, - DIK_NUMPADCOMMA
	
	0x0,	//	SDL_SCANCODE_KP_EQUALSAS400 = 134,
	
	0x73,	//	SDL_SCANCODE_INTERNATIONAL1 = 135, // this is really brazilian / and ? - DIK_ABNT_C1
	0x0,	//	SDL_SCANCODE_INTERNATIONAL2 = 136, // TODO: Hut1_12v2.pdf page 60, footnote 16
	0x7D,	//	SDL_SCANCODE_INTERNATIONAL3 = 137, - DIK_YEN
	0x79,	//	SDL_SCANCODE_INTERNATIONAL4 = 138, // Japan: XFER/"convert kana -> kanji", right of space - DIK_CONVERT
	0x7B,	//	SDL_SCANCODE_INTERNATIONAL5 = 139, // Japan: NFER/"don't convert kana -> kanji", left of space - DIK_NOCONVERT
	0x0,	//	SDL_SCANCODE_INTERNATIONAL6 = 140, // TODO: Hut1_12v2.pdf page 60, footnote 20
	0x0,	//	SDL_SCANCODE_INTERNATIONAL7 = 141, // Toggle Double-Byte/Single-Byte mode.
	0x0,	//	SDL_SCANCODE_INTERNATIONAL8 = 142, // Undefined, available for other Front End Language Processors
	0x0,	//	SDL_SCANCODE_INTERNATIONAL9 = 143, // Undefined, available for other Front End Language Processors
	0x0,	//	SDL_SCANCODE_LANG1 = 144, // Hangul/English toggle (Korea)
	0x0,	//	SDL_SCANCODE_LANG2 = 145, // Hanja conversion (Korea)
	0x70,	//	SDL_SCANCODE_LANG3 = 146, // Katakana (Japan) - DIK_KANA
	0x0,	//	SDL_SCANCODE_LANG4 = 147, // Hiragana (Japan)
	0x0,	//	SDL_SCANCODE_LANG5 = 148, // Zenkaku/Hankaku (Japan)
	0x0,	//	SDL_SCANCODE_LANG6 = 149, // reserved
	0x0,	//	SDL_SCANCODE_LANG7 = 150, // reserved
	0x0,	//	SDL_SCANCODE_LANG8 = 151, // reserved
	0x0,	//	SDL_SCANCODE_LANG9 = 152, // reserved
	
	0x0,	//	SDL_SCANCODE_ALTERASE = 153, // Erase-Eaze
	
	0xB7,	//	SDL_SCANCODE_SYSREQ = 154, - DIK_SYSRQ; SDL_SCANCODE_PRINTSCREEN also maps to this!
	
	0x0,	//	SDL_SCANCODE_CANCEL = 155,
	0x0,	//	SDL_SCANCODE_CLEAR = 156,
	0x0,	//	SDL_SCANCODE_PRIOR = 157,
	0x0,	//	SDL_SCANCODE_RETURN2 = 158,
	0x0,	//	SDL_SCANCODE_SEPARATOR = 159,
	0x0,	//	SDL_SCANCODE_OUT = 160,
	0x0,	//	SDL_SCANCODE_OPER = 161,
	0x0,	//	SDL_SCANCODE_CLEARAGAIN = 162,
	0x0,	//	SDL_SCANCODE_CRSEL = 163,
	0x0,	//	SDL_SCANCODE_EXSEL = 164,
	
	0x0,	// 165 (unused)
	0x0,	// 166 (unused)
	0x0,	// 167 (unused)
	0x0,	// 168 (unused)
	0x0,	// 169 (unused)
	0x0,	// 170 (unused)
	0x0,	// 171 (unused)
	0x0,	// 172 (unused)
	0x0,	// 173 (unused)
	0x0,	// 174 (unused)
	0x0,	// 175 (unused)
	
	0x0,	//	SDL_SCANCODE_KP_00 = 176,
	0x0,	//	SDL_SCANCODE_KP_000 = 177,
	0x0,	//	SDL_SCANCODE_THOUSANDSSEPARATOR = 178,
	0x0,	//	SDL_SCANCODE_DECIMALSEPARATOR = 179,
	0x0,	//	SDL_SCANCODE_CURRENCYUNIT = 180,
	0x0,	//	SDL_SCANCODE_CURRENCYSUBUNIT = 181,
	0x0,	//	SDL_SCANCODE_KP_LEFTPAREN = 182,
	0x0,	//	SDL_SCANCODE_KP_RIGHTPAREN = 183,
	0x0,	//	SDL_SCANCODE_KP_LEFTBRACE = 184,
	0x0,	//	SDL_SCANCODE_KP_RIGHTBRACE = 185,
	0x0,	//	SDL_SCANCODE_KP_TAB = 186,
	0x0,	//	SDL_SCANCODE_KP_BACKSPACE = 187,
	0x0,	//	SDL_SCANCODE_KP_A = 188,
	0x0,	//	SDL_SCANCODE_KP_B = 189,
	0x0,	//	SDL_SCANCODE_KP_C = 190,
	0x0,	//	SDL_SCANCODE_KP_D = 191,
	0x0,	//	SDL_SCANCODE_KP_E = 192,
	0x0,	//	SDL_SCANCODE_KP_F = 193,
	0x0,	//	SDL_SCANCODE_KP_XOR = 194,
	0x0,	//	SDL_SCANCODE_KP_POWER = 195,
	0x0,	//	SDL_SCANCODE_KP_PERCENT = 196,
	0x0,	//	SDL_SCANCODE_KP_LESS = 197,
	0x0,	//	SDL_SCANCODE_KP_GREATER = 198,
	0x0,	//	SDL_SCANCODE_KP_AMPERSAND = 199,
	0x0,	//	SDL_SCANCODE_KP_DBLAMPERSAND = 200,
	0x0,	//	SDL_SCANCODE_KP_VERTICALBAR = 201,
	0x0,	//	SDL_SCANCODE_KP_DBLVERTICALBAR = 202,
	
	0x92,	//	SDL_SCANCODE_KP_COLON = 203, - DIK_COLON
	
	0x0,	//	SDL_SCANCODE_KP_HASH = 204,
	0x0,	//	SDL_SCANCODE_KP_SPACE = 205,
	
	0x91,	//	SDL_SCANCODE_KP_AT = 206, - DIK_AT
	
	0x0,	//	SDL_SCANCODE_KP_EXCLAM = 207,
	0x0,	//	SDL_SCANCODE_KP_MEMSTORE = 208,
	0x0,	//	SDL_SCANCODE_KP_MEMRECALL = 209,
	0x0,	//	SDL_SCANCODE_KP_MEMCLEAR = 210,
	0x0,	//	SDL_SCANCODE_KP_MEMADD = 211,
	0x0,	//	SDL_SCANCODE_KP_MEMSUBTRACT = 212,
	0x0,	//	SDL_SCANCODE_KP_MEMMULTIPLY = 213,
	0x0,	//	SDL_SCANCODE_KP_MEMDIVIDE = 214,
	0x0,	//	SDL_SCANCODE_KP_PLUSMINUS = 215,
	0x0,	//	SDL_SCANCODE_KP_CLEAR = 216,
	0x0,	//	SDL_SCANCODE_KP_CLEARENTRY = 217,
	0x0,	//	SDL_SCANCODE_KP_BINARY = 218,
	0x0,	//	SDL_SCANCODE_KP_OCTAL = 219,
	0x0,	//	SDL_SCANCODE_KP_DECIMAL = 220,
	0x0,	//	SDL_SCANCODE_KP_HEXADECIMAL = 221,
	
	0x0,	// 222 (unused)
	0x0,	// 223 (unused)
	
	0x1D,	//	SDL_SCANCODE_LCTRL = 224, - DIK_LCONTROL
	0x2A,	//	SDL_SCANCODE_LSHIFT = 225, - DIK_LSHIFT
	0x38,	//	SDL_SCANCODE_LALT = 226, // left Alt, option - DIK_LMENU
	0xDB,	//	SDL_SCANCODE_LGUI = 227, // left windows, command (apple), meta - DIK_LWIN
	0x9D,	//	SDL_SCANCODE_RCTRL = 228, - DIK_RCONTROL
	0x36,	//	SDL_SCANCODE_RSHIFT = 229, - DIK_RSHIFT
	0xB8,	//	SDL_SCANCODE_RALT = 230, // right Alt/AltGr, option - DIK_RMENU, also used for SDL_SCANCODE_MODE!
	0xDC,	//	SDL_SCANCODE_RGUI = 231, // left windows, command (apple), meta - DIK_RWIN
	
	// 232 - 256 unused
	0, 0, 0, 0, 0, 0, 0, 0, 0,    // 232 - 240 unused
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 241-250 unused
	0, 0, 0, 0, 0, 0,             // 251-256 unused
	
	0xB8,	//	SDL_SCANCODE_MODE = 257,  // this seems to be the AltGr Key? - DIK_RMENU (right alt)
	
	// These values are mapped from usage page 0x0C (USB consumer page).
	0x99,	//	SDL_SCANCODE_AUDIONEXT = 258, - DIK_NEXTTRACK
	0x90,	//	SDL_SCANCODE_AUDIOPREV = 259, - DIK_PREVTRACK, which is DIK_CIRCUMFLEX on japanese keyboards
	0xA4,	//	SDL_SCANCODE_AUDIOSTOP = 260, - DIK_MEDIASTOP
	0xA2,	//	SDL_SCANCODE_AUDIOPLAY = 261, - DIK_PLAYPAUSE
	0xA0,	//	SDL_SCANCODE_AUDIOMUTE = 262, - DIK_MUTE
	0xED,	//	SDL_SCANCODE_MEDIASELECT = 263, - DIK_MEDIASELECT
	
	0x0,	//	SDL_SCANCODE_WWW = 264,
	
	0xEC,	//	SDL_SCANCODE_MAIL = 265, - DIK_MAIL
	0xA1,	//	SDL_SCANCODE_CALCULATOR = 266, - DIK_CALCULATOR
	0xEB,	//	SDL_SCANCODE_COMPUTER = 267, - DIK_MYCOMPUTER
	0xE5,	//	SDL_SCANCODE_AC_SEARCH = 268, - DIK_WEBSEARCH
	0xB2,	//	SDL_SCANCODE_AC_HOME = 269, - DIK_WEBHOME
	0xEA,	//	SDL_SCANCODE_AC_BACK = 270, - DIK_WEBBACK
	0xE9,	//	SDL_SCANCODE_AC_FORWARD = 271, - DIK_WEBFORWARD
	0xE8,	//	SDL_SCANCODE_AC_STOP = 272, - DIK_WEBSTOP
	0xE7,	//	SDL_SCANCODE_AC_REFRESH = 273, - DIK_WEBREFRESH
	0xE6,	//	SDL_SCANCODE_AC_BOOKMARKS = 274, - DIK_WEBFAVORITES
	
	// These are values that Christian Walther added (for mac keyboard?).
	0x0,	//	SDL_SCANCODE_BRIGHTNESSDOWN = 275,
	0x0,	//	SDL_SCANCODE_BRIGHTNESSUP = 276,
	0x0,	//	SDL_SCANCODE_DISPLAYSWITCH = 277, // display mirroring/dual display switch, video mode switch
	0x0,	//	SDL_SCANCODE_KBDILLUMTOGGLE = 278,
	0x0,	//	SDL_SCANCODE_KBDILLUMDOWN = 279,
	0x0,	//	SDL_SCANCODE_KBDILLUMUP = 280,
	0x0,	//	SDL_SCANCODE_EJECT = 281,
	
	0xDF,	//	SDL_SCANCODE_SLEEP = 282, - DIK_SLEEP
	
	0x0,	//	SDL_SCANCODE_APP1 = 283,
	0x0,	//	SDL_SCANCODE_APP2 = 284,
	// end of Walther-keys
	
	// the rest up to 511 are currently not named in SDL
	
	0, 0, 0, 0, 0, 0,             // 285-290 unused
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 291-300 unused
	
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 301-320 unused
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 321-340 unused
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 341-360 unused
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 361-380 unused
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 381-400 unused
	
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 401-420 unused
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 421-440 unused
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 441-460 unused
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 461-480 unused
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 481-500 unused
	
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 // 501-511 unused
};


#endif /* _SDL2_SCANCODE_TO_DINPUT_MAPPINGS_H_ */
