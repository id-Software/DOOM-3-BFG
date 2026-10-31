/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_cvar.h - console variables (idCVar / idCVarSystem,
   neo/framework/CVarSystem.h). Flag values match cvarFlags_t.

   Static registration replaces the original static constructors: a module
   declares its cvars with RGD3_CVAR(...) and registers them from its
   rgd3_<module>_register() function, which rgd3_registry.c calls. */
#ifndef RGD3_CVAR_H
#define RGD3_CVAR_H
#ifndef RGD3_ALL_C
#error "rgd3 headers are included through src/all.c only"
#endif

enum
{
	RGD3_CVAR_ALL = -1,
	RGD3_CVAR_BOOL = 1 << 0,
	RGD3_CVAR_INTEGER = 1 << 1,
	RGD3_CVAR_FLOAT = 1 << 2,
	RGD3_CVAR_SYSTEM = 1 << 3,
	RGD3_CVAR_RENDERER = 1 << 4,
	RGD3_CVAR_SOUND = 1 << 5,
	RGD3_CVAR_GUI = 1 << 6,
	RGD3_CVAR_GAME = 1 << 7,
	RGD3_CVAR_TOOL = 1 << 8,
	RGD3_CVAR_SERVERINFO = 1 << 10,
	RGD3_CVAR_NETWORKSYNC = 1 << 11,
	RGD3_CVAR_STATIC = 1 << 12,
	RGD3_CVAR_CHEAT = 1 << 13,
	RGD3_CVAR_NOCHEAT = 1 << 14,
	RGD3_CVAR_INIT = 1 << 15,
	RGD3_CVAR_ROM = 1 << 16,
	RGD3_CVAR_ARCHIVE = 1 << 17,
	RGD3_CVAR_MODIFIED = 1 << 18
};

struct Rgd3CmdArgs;
typedef void (*Rgd3ArgCompletionCallback)(const char* text);
typedef void (*Rgd3ArgCompletion)(const struct Rgd3CmdArgs* args, Rgd3ArgCompletionCallback callback);

typedef struct Rgd3Cvar
{
	/* declaration */
	const char* name;
	const char* default_value;
	int flags;
	const char* description;
	float value_min;
	float value_max;
	const char** value_strings;
	Rgd3ArgCompletion completion;
	/* runtime */
	char* value;
	char* reset_value;
	int integer_value;
	float float_value;
	int registered;
	int modified;
	int dynamic;
	struct Rgd3Cvar* next;
} Rgd3Cvar;

#define RGD3_CVAR(name, value, flags, desc) { name, value, (flags) | RGD3_CVAR_STATIC, desc, 1.0f, -1.0f, NULL, NULL }
#define RGD3_CVAR_RANGE(name, value, flags, desc, lo, hi) { name, value, (flags) | RGD3_CVAR_STATIC, desc, (float)(lo), (float)(hi), NULL, NULL }
#define RGD3_CVAR_STRINGS(name, value, flags, desc, strings) { name, value, (flags) | RGD3_CVAR_STATIC, desc, 1.0f, -1.0f, strings, NULL }

void rgd3_cvar_init(void);
void rgd3_cvar_shutdown(void);
void rgd3_cvar_register_commands(void);
void rgd3_cvar_register(Rgd3Cvar* cvar);
void rgd3_cvar_register_table(Rgd3Cvar* const* table, int count);
void rgd3_cvar_set_init_allowed(int allowed);

Rgd3Cvar* rgd3_cvar_find(const char* name);
void rgd3_cvar_set(Rgd3Cvar* cvar, const char* value, int force);
void rgd3_cvar_set_string(const char* name, const char* value, int flags);
void rgd3_cvar_set_int(const char* name, int value, int flags);
void rgd3_cvar_set_float(const char* name, float value, int flags);
void rgd3_cvar_set_bool(const char* name, int value, int flags);
const char* rgd3_cvar_get_string(const char* name);
int rgd3_cvar_get_int(const char* name);
float rgd3_cvar_get_float(const char* name);
int rgd3_cvar_get_bool(const char* name);
int rgd3_cvar_is_modified(Rgd3Cvar* cvar);
void rgd3_cvar_clear_modified(Rgd3Cvar* cvar);

int rgd3_cvar_command(const struct Rgd3CmdArgs* args);
int rgd3_cvar_modified_flags(void);
void rgd3_cvar_set_modified_flags(int flags);
void rgd3_cvar_clear_modified_flags(int flags);
struct Rgd3File;
void rgd3_cvar_write_flagged(int flags, const char* set_cmd, struct Rgd3File* file);

#endif
