/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_keys.c - key name table and bind/unbind/unbindall/listBinds.
   The names are the identifiers config files use (KeyInput.cpp keynames). */

typedef struct Rgd3KeyName
{
	int keynum;
	const char* name;
} Rgd3KeyName;

#define RGD3_KEYNAME(code) { RGD3_K_##code, #code }
#define RGD3_KEYALIAS(alias, code) { RGD3_K_##code, alias }

static const Rgd3KeyName rgd3_keynames[] = {
	RGD3_KEYNAME(ESCAPE), RGD3_KEYNAME(1), RGD3_KEYNAME(2), RGD3_KEYNAME(3), RGD3_KEYNAME(4), RGD3_KEYNAME(5),
	RGD3_KEYNAME(6), RGD3_KEYNAME(7), RGD3_KEYNAME(8), RGD3_KEYNAME(9), RGD3_KEYNAME(0),
	RGD3_KEYNAME(MINUS), RGD3_KEYNAME(EQUALS), RGD3_KEYNAME(BACKSPACE), RGD3_KEYNAME(TAB),
	RGD3_KEYNAME(Q), RGD3_KEYNAME(W), RGD3_KEYNAME(E), RGD3_KEYNAME(R), RGD3_KEYNAME(T), RGD3_KEYNAME(Y),
	RGD3_KEYNAME(U), RGD3_KEYNAME(I), RGD3_KEYNAME(O), RGD3_KEYNAME(P),
	RGD3_KEYNAME(LBRACKET), RGD3_KEYNAME(RBRACKET), RGD3_KEYNAME(ENTER), RGD3_KEYNAME(LCTRL),
	RGD3_KEYNAME(A), RGD3_KEYNAME(S), RGD3_KEYNAME(D), RGD3_KEYNAME(F), RGD3_KEYNAME(G), RGD3_KEYNAME(H),
	RGD3_KEYNAME(J), RGD3_KEYNAME(K), RGD3_KEYNAME(L),
	RGD3_KEYNAME(SEMICOLON), RGD3_KEYNAME(APOSTROPHE), RGD3_KEYNAME(GRAVE), RGD3_KEYNAME(LSHIFT), RGD3_KEYNAME(BACKSLASH),
	RGD3_KEYNAME(Z), RGD3_KEYNAME(X), RGD3_KEYNAME(C), RGD3_KEYNAME(V), RGD3_KEYNAME(B), RGD3_KEYNAME(N), RGD3_KEYNAME(M),
	RGD3_KEYNAME(COMMA), RGD3_KEYNAME(PERIOD), RGD3_KEYNAME(SLASH), RGD3_KEYNAME(RSHIFT), RGD3_KEYNAME(KP_STAR),
	RGD3_KEYNAME(LALT), RGD3_KEYNAME(SPACE), RGD3_KEYNAME(CAPSLOCK),
	RGD3_KEYNAME(F1), RGD3_KEYNAME(F2), RGD3_KEYNAME(F3), RGD3_KEYNAME(F4), RGD3_KEYNAME(F5), RGD3_KEYNAME(F6),
	RGD3_KEYNAME(F7), RGD3_KEYNAME(F8), RGD3_KEYNAME(F9), RGD3_KEYNAME(F10), RGD3_KEYNAME(NUMLOCK), RGD3_KEYNAME(SCROLL),
	RGD3_KEYNAME(KP_7), RGD3_KEYNAME(KP_8), RGD3_KEYNAME(KP_9), RGD3_KEYNAME(KP_MINUS), RGD3_KEYNAME(KP_4),
	RGD3_KEYNAME(KP_5), RGD3_KEYNAME(KP_6), RGD3_KEYNAME(KP_PLUS), RGD3_KEYNAME(KP_1), RGD3_KEYNAME(KP_2),
	RGD3_KEYNAME(KP_3), RGD3_KEYNAME(KP_0), RGD3_KEYNAME(KP_DOT),
	RGD3_KEYNAME(F11), RGD3_KEYNAME(F12), RGD3_KEYNAME(F13), RGD3_KEYNAME(F14), RGD3_KEYNAME(F15),
	RGD3_KEYNAME(KANA), RGD3_KEYNAME(CONVERT), RGD3_KEYNAME(NOCONVERT), RGD3_KEYNAME(YEN), RGD3_KEYNAME(KP_EQUALS),
	RGD3_KEYNAME(CIRCUMFLEX), RGD3_KEYNAME(AT), RGD3_KEYNAME(COLON), RGD3_KEYNAME(UNDERLINE), RGD3_KEYNAME(KANJI),
	RGD3_KEYNAME(STOP), RGD3_KEYNAME(AX), RGD3_KEYNAME(UNLABELED), RGD3_KEYNAME(KP_ENTER), RGD3_KEYNAME(RCTRL),
	RGD3_KEYNAME(KP_COMMA), RGD3_KEYNAME(KP_SLASH), RGD3_KEYNAME(PRINTSCREEN), RGD3_KEYNAME(RALT), RGD3_KEYNAME(PAUSE),
	RGD3_KEYNAME(HOME), RGD3_KEYNAME(UPARROW), RGD3_KEYNAME(PGUP), RGD3_KEYNAME(LEFTARROW), RGD3_KEYNAME(RIGHTARROW),
	RGD3_KEYNAME(END), RGD3_KEYNAME(DOWNARROW), RGD3_KEYNAME(PGDN), RGD3_KEYNAME(INS), RGD3_KEYNAME(DEL),
	RGD3_KEYNAME(LWIN), RGD3_KEYNAME(RWIN), RGD3_KEYNAME(APPS), RGD3_KEYNAME(POWER), RGD3_KEYNAME(SLEEP),
	RGD3_KEYNAME(MOUSE1), RGD3_KEYNAME(MOUSE2), RGD3_KEYNAME(MOUSE3), RGD3_KEYNAME(MOUSE4), RGD3_KEYNAME(MOUSE5),
	RGD3_KEYNAME(MOUSE6), RGD3_KEYNAME(MOUSE7), RGD3_KEYNAME(MOUSE8), RGD3_KEYNAME(MWHEELDOWN), RGD3_KEYNAME(MWHEELUP),
	RGD3_KEYNAME(JOY1), RGD3_KEYNAME(JOY2), RGD3_KEYNAME(JOY3), RGD3_KEYNAME(JOY4), RGD3_KEYNAME(JOY5),
	RGD3_KEYNAME(JOY6), RGD3_KEYNAME(JOY7), RGD3_KEYNAME(JOY8), RGD3_KEYNAME(JOY9), RGD3_KEYNAME(JOY10),
	RGD3_KEYNAME(JOY11), RGD3_KEYNAME(JOY12), RGD3_KEYNAME(JOY13), RGD3_KEYNAME(JOY14), RGD3_KEYNAME(JOY15),
	RGD3_KEYNAME(JOY16),
	RGD3_KEYNAME(JOY_DPAD_UP), RGD3_KEYNAME(JOY_DPAD_DOWN), RGD3_KEYNAME(JOY_DPAD_LEFT), RGD3_KEYNAME(JOY_DPAD_RIGHT),
	RGD3_KEYNAME(JOY_STICK1_UP), RGD3_KEYNAME(JOY_STICK1_DOWN), RGD3_KEYNAME(JOY_STICK1_LEFT), RGD3_KEYNAME(JOY_STICK1_RIGHT),
	RGD3_KEYNAME(JOY_STICK2_UP), RGD3_KEYNAME(JOY_STICK2_DOWN), RGD3_KEYNAME(JOY_STICK2_LEFT), RGD3_KEYNAME(JOY_STICK2_RIGHT),
	RGD3_KEYNAME(JOY_TRIGGER1), RGD3_KEYNAME(JOY_TRIGGER2),
	/* aliases accepted for old configs; never produced by keynum_to_string */
	RGD3_KEYALIAS("ALT", LALT), RGD3_KEYALIAS("RIGHTALT", RALT), RGD3_KEYALIAS("CTRL", LCTRL),
	RGD3_KEYALIAS("SHIFT", LSHIFT), RGD3_KEYALIAS("MENU", APPS), RGD3_KEYALIAS("COMMAND", LALT),
	RGD3_KEYALIAS("KP_HOME", KP_7), RGD3_KEYALIAS("KP_UPARROW", KP_8), RGD3_KEYALIAS("KP_PGUP", KP_9),
	RGD3_KEYALIAS("KP_LEFTARROW", KP_4), RGD3_KEYALIAS("KP_RIGHTARROW", KP_6), RGD3_KEYALIAS("KP_END", KP_1),
	RGD3_KEYALIAS("KP_DOWNARROW", KP_2), RGD3_KEYALIAS("KP_PGDN", KP_3), RGD3_KEYALIAS("KP_INS", KP_0),
	RGD3_KEYALIAS("KP_DEL", KP_DOT), RGD3_KEYALIAS("KP_NUMLOCK", NUMLOCK),
	RGD3_KEYALIAS("-", MINUS), RGD3_KEYALIAS("=", EQUALS), RGD3_KEYALIAS("[", LBRACKET), RGD3_KEYALIAS("]", RBRACKET),
	RGD3_KEYALIAS("\\", BACKSLASH), RGD3_KEYALIAS("/", SLASH), RGD3_KEYALIAS(",", COMMA), RGD3_KEYALIAS(".", PERIOD),
	{ RGD3_K_NONE, NULL }
};

/* Index of the first alias row: keynum_to_string stops here. */
#define RGD3_KEYNAME_ALIAS_START "ALT"

typedef struct Rgd3Key
{
	int down;
	int repeats;
	char* binding;
	int usercmd_action;
} Rgd3Key;

typedef struct Rgd3KeysState
{
	Rgd3Key keys[RGD3_K_LAST_KEY];
	int bindings_modified;
} Rgd3KeysState;

static Rgd3KeysState rgd3_keys;

void rgd3_keys_init(void)
{
	memset(&rgd3_keys, 0, sizeof(rgd3_keys));
}

void rgd3_keys_shutdown(void)
{
	int i;
	for (i = 0; i < RGD3_K_LAST_KEY; ++i)
		rgd3_heap_free(rgd3_keys.keys[i].binding);
	memset(&rgd3_keys, 0, sizeof(rgd3_keys));
}

int rgd3_keys_string_to_keynum(const char* text)
{
	const Rgd3KeyName* kn;
	if (!text || !text[0])
		return RGD3_K_NONE;
	/* 0xnn hex form for keys without names */
	if (text[0] == '0' && (text[1] == 'x' || text[1] == 'X') && text[2] && text[3] && !text[4])
	{
		int value = (int)strtol(text + 2, NULL, 16);
		return (value > 0 && value < RGD3_K_LAST_KEY) ? value : RGD3_K_NONE;
	}
	for (kn = rgd3_keynames; kn->name; ++kn)
		if (rgd3_str_icmp(text, kn->name) == 0)
			return kn->keynum;
	return RGD3_K_NONE;
}

const char* rgd3_keys_keynum_to_string(int keynum)
{
	static char hex[8];
	const Rgd3KeyName* kn;
	for (kn = rgd3_keynames; kn->name; ++kn)
	{
		if (strcmp(kn->name, RGD3_KEYNAME_ALIAS_START) == 0)
			break;
		if (kn->keynum == keynum)
			return kn->name;
	}
	rg_snprintf(hex, sizeof(hex), "0x%02x", keynum);
	return hex;
}

/* Bindings beginning with '_' are usercmd buttons/impulses; the usercmd
   generator (M2) polls them instead of running them as commands. */
void rgd3_keys_set_binding(int keynum, const char* binding)
{
	Rgd3Key* key;
	if (keynum <= RGD3_K_NONE || keynum >= RGD3_K_LAST_KEY)
		return;
	/* clear the usercmd button states so a key held while its binding
	   changes doesn't leave the old action down forever */
	rgd3_usercmd_clear();
	key = &rgd3_keys.keys[keynum];
	rgd3_heap_free(key->binding);
	key->binding = (binding && binding[0]) ? rgd3_heap_strdup(binding, RGD3_TAG_KEYS) : NULL;
	key->usercmd_action = key->binding && key->binding[0] == '_';
	rgd3_keys.bindings_modified = 1;
}

const char* rgd3_keys_get_binding(int keynum)
{
	if (keynum <= RGD3_K_NONE || keynum >= RGD3_K_LAST_KEY)
		return "";
	return rgd3_keys.keys[keynum].binding ? rgd3_keys.keys[keynum].binding : "";
}

int rgd3_keys_is_down(int keynum)
{
	return keynum > RGD3_K_NONE && keynum < RGD3_K_LAST_KEY && rgd3_keys.keys[keynum].down;
}

void rgd3_keys_preliminary_key_event(int keynum, int down)
{
	Rgd3Key* key;
	if (keynum <= RGD3_K_NONE || keynum >= RGD3_K_LAST_KEY)
		return;
	key = &rgd3_keys.keys[keynum];
	key->down = down;
	key->repeats = down ? key->repeats + 1 : 0;
}

int rgd3_keys_exec_key_binding(int keynum)
{
	const Rgd3Key* key;
	if (keynum <= RGD3_K_NONE || keynum >= RGD3_K_LAST_KEY)
		return 0;
	key = &rgd3_keys.keys[keynum];
	if (key->usercmd_action || !key->binding)
		return 0;
	rgd3_cmd_buffer_text(RGD3_CMD_EXEC_APPEND, key->binding);
	rgd3_cmd_buffer_text(RGD3_CMD_EXEC_APPEND, "\n");
	return 1;
}

void rgd3_keys_clear_states(void)
{
	int i;
	for (i = 0; i < RGD3_K_LAST_KEY; ++i)
	{
		rgd3_keys.keys[i].down = 0;
		rgd3_keys.keys[i].repeats = 0;
	}
}

int rgd3_keys_bindings_modified(void)
{
	return rgd3_keys.bindings_modified;
}

void rgd3_keys_clear_bindings_modified(void)
{
	rgd3_keys.bindings_modified = 0;
}

void rgd3_keys_write_bindings(Rgd3File* file)
{
	int i;
	rgd3_fs_printf(file, "unbindall\n");
	for (i = 0; i < RGD3_K_LAST_KEY; ++i)
	{
		const char* binding = rgd3_keys.keys[i].binding;
		if (!binding || !binding[0])
			continue;
		rgd3_fs_printf(file, "bind \"%s\" \"%s\"\n", rgd3_keys_keynum_to_string(i), binding);
	}
}

static void rgd3_keys_cmd_bind(const Rgd3CmdArgs* args)
{
	int argc = rgd3_cmdargs_argc(args);
	int keynum;
	if (argc < 2)
	{
		rgd3_printf("bind <key> [command] : attach a command to a key\n");
		return;
	}
	keynum = rgd3_keys_string_to_keynum(rgd3_cmdargs_argv(args, 1));
	if (keynum == RGD3_K_NONE)
	{
		rgd3_printf("\"%s\" isn't a valid key\n", rgd3_cmdargs_argv(args, 1));
		return;
	}
	if (argc == 2)
	{
		const char* binding = rgd3_keys_get_binding(keynum);
		if (binding[0])
			rgd3_printf("\"%s\" = \"%s\"\n", rgd3_cmdargs_argv(args, 1), binding);
		else
			rgd3_printf("\"%s\" is not bound\n", rgd3_cmdargs_argv(args, 1));
		return;
	}
	rgd3_keys_set_binding(keynum, rgd3_cmdargs_args(args, 2, -1, 0));
}

static void rgd3_keys_cmd_unbind(const Rgd3CmdArgs* args)
{
	int keynum;
	if (rgd3_cmdargs_argc(args) != 2)
	{
		rgd3_printf("unbind <key> : remove commands from a key\n");
		return;
	}
	keynum = rgd3_keys_string_to_keynum(rgd3_cmdargs_argv(args, 1));
	if (keynum == RGD3_K_NONE)
	{
		rgd3_printf("\"%s\" isn't a valid key\n", rgd3_cmdargs_argv(args, 1));
		return;
	}
	rgd3_keys_set_binding(keynum, "");
}

static void rgd3_keys_cmd_unbindall(const Rgd3CmdArgs* args)
{
	int i;
	RG_UNUSED(args);
	for (i = 0; i < RGD3_K_LAST_KEY; ++i)
		rgd3_keys_set_binding(i, "");
}

static void rgd3_keys_cmd_list(const Rgd3CmdArgs* args)
{
	int i, count = 0;
	RG_UNUSED(args);
	for (i = 0; i < RGD3_K_LAST_KEY; ++i)
	{
		if (rgd3_keys.keys[i].binding && rgd3_keys.keys[i].binding[0])
		{
			rgd3_printf("%-16s \"%s\"\n", rgd3_keys_keynum_to_string(i), rgd3_keys.keys[i].binding);
			++count;
		}
	}
	rgd3_printf("%d bindings\n", count);
}

void rgd3_keys_register(void)
{
	rgd3_cmd_add("bind", rgd3_keys_cmd_bind, RGD3_CMD_FL_SYSTEM, "binds a command to a key", NULL);
	rgd3_cmd_add("unbind", rgd3_keys_cmd_unbind, RGD3_CMD_FL_SYSTEM, "unbinds any command from a key", NULL);
	rgd3_cmd_add("unbindall", rgd3_keys_cmd_unbindall, RGD3_CMD_FL_SYSTEM, "unbinds any commands from all keys", NULL);
	rgd3_cmd_add("listBinds", rgd3_keys_cmd_list, RGD3_CMD_FL_SYSTEM, "lists key bindings", NULL);
}
