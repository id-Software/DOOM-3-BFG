/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_cvar.c - value validation, dynamic cvars and the set/seta/toggle/
   reset/listCvars commands follow neo/framework/CVarSystem.cpp. */

#define RGD3_CVAR_HASH_SIZE 1024

typedef struct Rgd3CvarState
{
	Rgd3Cvar* list;
	Rgd3Cvar* hash[RGD3_CVAR_HASH_SIZE];
	int modified_flags;
	int init_allowed;
	int count;
} Rgd3CvarState;

static Rgd3CvarState rgd3_cvars;

void rgd3_cvar_init(void)
{
	memset(&rgd3_cvars, 0, sizeof(rgd3_cvars));
	rgd3_cvars.init_allowed = 1;
}

void rgd3_cvar_set_init_allowed(int allowed)
{
	rgd3_cvars.init_allowed = allowed;
}

Rgd3Cvar* rgd3_cvar_find(const char* name)
{
	Rgd3Cvar* cv;
	for (cv = rgd3_cvars.hash[rgd3_str_ihash(name) & (RGD3_CVAR_HASH_SIZE - 1)]; cv; cv = cv->next)
		if (rgd3_str_icmp(cv->name, name) == 0)
			return cv;
	return NULL;
}

/* The hash chains reuse 'next'; a separate sorted array is built for listing. */
static void rgd3_cvar_link(Rgd3Cvar* cv)
{
	u32 slot = rgd3_str_ihash(cv->name) & (RGD3_CVAR_HASH_SIZE - 1);
	cv->next = rgd3_cvars.hash[slot];
	rgd3_cvars.hash[slot] = cv;
	++rgd3_cvars.count;
}

static void rgd3_cvar_unlink(Rgd3Cvar* cv)
{
	u32 slot = rgd3_str_ihash(cv->name) & (RGD3_CVAR_HASH_SIZE - 1);
	Rgd3Cvar** link = &rgd3_cvars.hash[slot];
	while (*link && *link != cv)
		link = &(*link)->next;
	if (*link)
	{
		*link = cv->next;
		--rgd3_cvars.count;
	}
}

static void rgd3_cvar_store(Rgd3Cvar* cv, const char* text)
{
	char* old = cv->value;
	cv->value = rgd3_heap_strdup(text, RGD3_TAG_CVAR);
	rgd3_heap_free(old);
}

/* idInternalCVar::UpdateValue: normalise the string for typed cvars. */
static void rgd3_cvar_update_value(Rgd3Cvar* cv)
{
	char buf[64];
	int clamped = 0;
	if (cv->flags & RGD3_CVAR_BOOL)
	{
		cv->integer_value = atoi(cv->value) != 0;
		cv->float_value = (float)cv->integer_value;
		if (strcmp(cv->value, cv->integer_value ? "1" : "0") != 0)
			rgd3_cvar_store(cv, cv->integer_value ? "1" : "0");
	}
	else if (cv->flags & RGD3_CVAR_INTEGER)
	{
		cv->integer_value = atoi(cv->value);
		if (cv->value_min < cv->value_max)
		{
			if (cv->integer_value < (int)cv->value_min)
			{
				cv->integer_value = (int)cv->value_min;
				clamped = 1;
			}
			else if (cv->integer_value > (int)cv->value_max)
			{
				cv->integer_value = (int)cv->value_max;
				clamped = 1;
			}
		}
		if (clamped || !rgd3_str_is_numeric(cv->value) || strchr(cv->value, '.'))
		{
			rg_snprintf(buf, sizeof(buf), "%d", cv->integer_value);
			rgd3_cvar_store(cv, buf);
		}
		cv->float_value = (float)cv->integer_value;
	}
	else if (cv->flags & RGD3_CVAR_FLOAT)
	{
		cv->float_value = (float)atof(cv->value);
		if (cv->value_min < cv->value_max)
		{
			if (cv->float_value < cv->value_min)
			{
				cv->float_value = cv->value_min;
				clamped = 1;
			}
			else if (cv->float_value > cv->value_max)
			{
				cv->float_value = cv->value_max;
				clamped = 1;
			}
		}
		if (clamped || !rgd3_str_is_numeric(cv->value))
		{
			rgd3_str_float_to_string(cv->float_value, buf, sizeof(buf));
			rgd3_cvar_store(cv, buf);
		}
		cv->integer_value = (int)cv->float_value;
	}
	else if (cv->value_strings && cv->value_strings[0])
	{
		int i;
		cv->integer_value = 0;
		for (i = 0; cv->value_strings[i]; ++i)
		{
			if (rgd3_str_icmp(cv->value, cv->value_strings[i]) == 0)
			{
				cv->integer_value = i;
				break;
			}
		}
		rgd3_cvar_store(cv, cv->value_strings[cv->integer_value]);
		cv->float_value = (float)cv->integer_value;
	}
	else
	{
		cv->integer_value = atoi(cv->value);
		cv->float_value = (float)atof(cv->value);
	}
}

void rgd3_cvar_set(Rgd3Cvar* cv, const char* value, int force)
{
	if (!cv)
		return;
	if (!value)
		value = cv->reset_value ? cv->reset_value : "";
	if (!force)
	{
		if (cv->flags & RGD3_CVAR_ROM)
		{
			rgd3_printf("%s is read only.\n", cv->name);
			return;
		}
		if ((cv->flags & RGD3_CVAR_INIT) && !rgd3_cvars.init_allowed)
		{
			rgd3_printf("%s is write protected.\n", cv->name);
			return;
		}
	}
	if (cv->value && strcmp(cv->value, value) == 0)
		return;
	rgd3_cvar_store(cv, value);
	rgd3_cvar_update_value(cv);
	cv->modified = 1;
	cv->flags |= RGD3_CVAR_MODIFIED;
	rgd3_cvars.modified_flags |= cv->flags;
}

static Rgd3Cvar* rgd3_cvar_create_dynamic(const char* name, const char* value, int flags)
{
	Rgd3Cvar* cv = (Rgd3Cvar*)rgd3_heap_calloc(sizeof(Rgd3Cvar), RGD3_TAG_CVAR);
	cv->name = rgd3_heap_strdup(name, RGD3_TAG_CVAR);
	cv->default_value = rgd3_heap_strdup(value, RGD3_TAG_CVAR);
	cv->description = "";
	cv->flags = flags & ~RGD3_CVAR_STATIC;
	cv->value_min = 1.0f;
	cv->value_max = -1.0f;
	cv->dynamic = 1;
	cv->registered = 1;
	cv->reset_value = rgd3_heap_strdup(value, RGD3_TAG_CVAR);
	rgd3_cvar_store(cv, value);
	rgd3_cvar_update_value(cv);
	rgd3_cvar_link(cv);
	return cv;
}

static void rgd3_cvar_free_dynamic(Rgd3Cvar* cv)
{
	rgd3_heap_free((void*)cv->name);
	rgd3_heap_free((void*)cv->default_value);
	rgd3_heap_free(cv->reset_value);
	rgd3_heap_free(cv->value);
	rgd3_heap_free(cv);
}

void rgd3_cvar_register(Rgd3Cvar* cv)
{
	Rgd3Cvar* existing = rgd3_cvar_find(cv->name);
	const char* start_value = cv->default_value;
	if (existing == cv)
		return;
	cv->reset_value = rgd3_heap_strdup(cv->default_value, RGD3_TAG_CVAR);
	if (existing)
	{
		/* A user-created cvar (set before registration) keeps its value. */
		if (!existing->dynamic)
		{
			rgd3_warning("cvar '%s' registered twice", cv->name);
			return;
		}
		cv->flags |= existing->flags & (RGD3_CVAR_ARCHIVE | RGD3_CVAR_MODIFIED);
		start_value = existing->value;
		rgd3_cvar_unlink(existing);
		cv->value = rgd3_heap_strdup(start_value, RGD3_TAG_CVAR);
		rgd3_cvar_free_dynamic(existing);
	}
	else
	{
		cv->value = rgd3_heap_strdup(start_value, RGD3_TAG_CVAR);
	}
	rgd3_cvar_update_value(cv);
	cv->registered = 1;
	rgd3_cvar_link(cv);
}

void rgd3_cvar_register_table(Rgd3Cvar* const* table, int count)
{
	int i;
	for (i = 0; i < count; ++i)
		rgd3_cvar_register(table[i]);
}

void rgd3_cvar_set_string(const char* name, const char* value, int flags)
{
	Rgd3Cvar* cv = rgd3_cvar_find(name);
	if (!cv)
	{
		rgd3_cvar_create_dynamic(name, value, flags);
		rgd3_cvars.modified_flags |= flags;
		return;
	}
	cv->flags |= flags & ~(RGD3_CVAR_BOOL | RGD3_CVAR_INTEGER | RGD3_CVAR_FLOAT);
	rgd3_cvar_set(cv, value, 0);
}

void rgd3_cvar_set_int(const char* name, int value, int flags)
{
	rgd3_cvar_set_string(name, rgd3_va("%d", value), flags);
}

void rgd3_cvar_set_float(const char* name, float value, int flags)
{
	char buf[64];
	rgd3_str_float_to_string(value, buf, sizeof(buf));
	rgd3_cvar_set_string(name, buf, flags);
}

void rgd3_cvar_set_bool(const char* name, int value, int flags)
{
	rgd3_cvar_set_string(name, value ? "1" : "0", flags);
}

const char* rgd3_cvar_get_string(const char* name)
{
	Rgd3Cvar* cv = rgd3_cvar_find(name);
	return cv ? cv->value : "";
}

int rgd3_cvar_get_int(const char* name)
{
	Rgd3Cvar* cv = rgd3_cvar_find(name);
	return cv ? cv->integer_value : 0;
}

float rgd3_cvar_get_float(const char* name)
{
	Rgd3Cvar* cv = rgd3_cvar_find(name);
	return cv ? cv->float_value : 0.0f;
}

int rgd3_cvar_get_bool(const char* name)
{
	return rgd3_cvar_get_int(name) != 0;
}

int rgd3_cvar_is_modified(Rgd3Cvar* cv)
{
	return cv->modified;
}

void rgd3_cvar_clear_modified(Rgd3Cvar* cv)
{
	cv->modified = 0;
}

int rgd3_cvar_modified_flags(void)
{
	return rgd3_cvars.modified_flags;
}

void rgd3_cvar_set_modified_flags(int flags)
{
	rgd3_cvars.modified_flags |= flags;
}

void rgd3_cvar_clear_modified_flags(int flags)
{
	rgd3_cvars.modified_flags &= ~flags;
}

/* "name" prints, "name value" sets (idCVarSystemLocal::Command). */
int rgd3_cvar_command(const Rgd3CmdArgs* args)
{
	Rgd3Cvar* cv = rgd3_cvar_find(rgd3_cmdargs_argv(args, 0));
	if (!cv)
		return 0;
	if (rgd3_cmdargs_argc(args) == 1)
	{
		rgd3_printf("\"%s\" is:\"%s\"^7 default:\"%s\"\n", cv->name, cv->value, cv->reset_value ? cv->reset_value : "");
		if (cv->description && cv->description[0])
			rgd3_printf("^3%s\n", cv->description);
		return 1;
	}
	rgd3_cvar_set(cv, rgd3_cmdargs_args(args, 1, -1, 0), 0);
	return 1;
}

static int rgd3_cvar_sort(const void* a, const void* b)
{
	return rgd3_str_icmp((*(const Rgd3Cvar* const*)a)->name, (*(const Rgd3Cvar* const*)b)->name);
}

/* Returns a heap array of every cvar sorted by name; caller frees. */
static Rgd3Cvar** rgd3_cvar_sorted(int* out_count)
{
	Rgd3Cvar** list = (Rgd3Cvar**)rgd3_heap_alloc(sizeof(Rgd3Cvar*) * (size_t)(rgd3_cvars.count + 1), RGD3_TAG_TEMP);
	int i, n = 0;
	for (i = 0; i < RGD3_CVAR_HASH_SIZE; ++i)
	{
		Rgd3Cvar* cv;
		for (cv = rgd3_cvars.hash[i]; cv; cv = cv->next)
			list[n++] = cv;
	}
	qsort(list, (size_t)n, sizeof(Rgd3Cvar*), rgd3_cvar_sort);
	*out_count = n;
	return list;
}

void rgd3_cvar_write_flagged(int flags, const char* set_cmd, struct Rgd3File* file)
{
	int n, i;
	Rgd3Cvar** list = rgd3_cvar_sorted(&n);
	for (i = 0; i < n; ++i)
		if (list[i]->flags & flags)
			rgd3_fs_printf(file, "%s %s \"%s\"\n", set_cmd, list[i]->name, list[i]->value);
	rgd3_heap_free(list);
}

static void rgd3_cvar_set_with_flags(const Rgd3CmdArgs* args, int flags)
{
	if (rgd3_cmdargs_argc(args) < 3)
	{
		rgd3_printf("usage: %s <variable> <value>\n", rgd3_cmdargs_argv(args, 0));
		return;
	}
	rgd3_cvar_set_string(rgd3_cmdargs_argv(args, 1), rgd3_cmdargs_args(args, 2, -1, 0), flags);
}

static void rgd3_cvar_cmd_set(const Rgd3CmdArgs* args)
{
	rgd3_cvar_set_with_flags(args, 0);
}

static void rgd3_cvar_cmd_seta(const Rgd3CmdArgs* args)
{
	rgd3_cvar_set_with_flags(args, RGD3_CVAR_ARCHIVE);
}

static void rgd3_cvar_cmd_sets(const Rgd3CmdArgs* args)
{
	rgd3_cvar_set_with_flags(args, RGD3_CVAR_SERVERINFO);
}

static void rgd3_cvar_cmd_sett(const Rgd3CmdArgs* args)
{
	rgd3_cvar_set_with_flags(args, RGD3_CVAR_TOOL);
}

static void rgd3_cvar_cmd_setu(const Rgd3CmdArgs* args)
{
	rgd3_cvar_set_with_flags(args, 0);
}

/* toggle <var> [value1 value2 ...]: cycles booleans or a value list. */
static void rgd3_cvar_cmd_toggle(const Rgd3CmdArgs* args)
{
	Rgd3Cvar* cv;
	int argc = rgd3_cmdargs_argc(args);
	int i;
	if (argc < 2)
	{
		rgd3_printf("usage: toggle <variable> [value1] [value2] [value3] ...\n");
		return;
	}
	cv = rgd3_cvar_find(rgd3_cmdargs_argv(args, 1));
	if (!cv)
	{
		rgd3_warning("Toggle_f: cvar \"%s\" not found", rgd3_cmdargs_argv(args, 1));
		return;
	}
	if (argc < 3)
	{
		rgd3_cvar_set(cv, cv->integer_value ? "0" : "1", 0);
		rgd3_printf("set %s = %s\n", cv->name, cv->value);
		return;
	}
	for (i = 2; i < argc; ++i)
		if (rgd3_str_icmp(cv->value, rgd3_cmdargs_argv(args, i)) == 0)
			break;
	i = (i + 1 < argc) ? i + 1 : 2;
	rgd3_cvar_set(cv, rgd3_cmdargs_argv(args, i), 0);
	rgd3_printf("set %s = %s\n", cv->name, cv->value);
}

static void rgd3_cvar_cmd_reset(const Rgd3CmdArgs* args)
{
	Rgd3Cvar* cv;
	if (rgd3_cmdargs_argc(args) != 2)
	{
		rgd3_printf("usage: reset <variable>\n");
		return;
	}
	cv = rgd3_cvar_find(rgd3_cmdargs_argv(args, 1));
	if (cv)
		rgd3_cvar_set(cv, cv->reset_value, 0);
}

static void rgd3_cvar_cmd_restart(const Rgd3CmdArgs* args)
{
	int n, i;
	Rgd3Cvar** list = rgd3_cvar_sorted(&n);
	RG_UNUSED(args);
	for (i = 0; i < n; ++i)
		if (!(list[i]->flags & (RGD3_CVAR_ROM | RGD3_CVAR_INIT)))
			rgd3_cvar_set(list[i], list[i]->reset_value, 1);
	rgd3_heap_free(list);
}

static void rgd3_cvar_cmd_add(const Rgd3CmdArgs* args)
{
	Rgd3Cvar* cv;
	if (rgd3_cmdargs_argc(args) != 3)
	{
		rgd3_printf("usage: cvarAdd <variable> <value>\n");
		return;
	}
	cv = rgd3_cvar_find(rgd3_cmdargs_argv(args, 1));
	if (!cv)
		return;
	rgd3_cvar_set(cv, rgd3_va("%g", cv->float_value + (float)atof(rgd3_cmdargs_argv(args, 2))), 0);
	rgd3_printf("%s = %s\n", cv->name, cv->value);
}

/* listCvars [-help|-type|-flags] [filter], argument handling as
   idCVarSystemLocal::ListByFlags: the lexer splits "-help" into "-" "help"
   and the filter is the remaining arguments joined without spaces. */
static void rgd3_cvar_cmd_list(const Rgd3CmdArgs* args)
{
	enum { SHOW_VALUE, SHOW_DESCRIPTION, SHOW_TYPE, SHOW_FLAGS } show = SHOW_VALUE;
	char match[RGD3_MAX_COMMAND_STRING];
	const char* filter;
	int n, i, arg_num = 1, listed = 0;
	Rgd3Cvar** list;
	const char* a = rgd3_cmdargs_argv(args, 1);
	const char* b = rgd3_cmdargs_argv(args, 2);

	if (rgd3_str_icmp(a, "-") == 0 || rgd3_str_icmp(a, "/") == 0)
	{
		if (rgd3_str_icmp(b, "help") == 0 || rgd3_str_icmp(b, "?") == 0)
		{
			arg_num = 3;
			show = SHOW_DESCRIPTION;
		}
		else if (rgd3_str_icmp(b, "type") == 0 || rgd3_str_icmp(b, "range") == 0)
		{
			arg_num = 3;
			show = SHOW_TYPE;
		}
		else if (rgd3_str_icmp(b, "flags") == 0)
		{
			arg_num = 3;
			show = SHOW_FLAGS;
		}
	}
	filter = rgd3_cmdargs_match(args, arg_num, match, sizeof(match));
	if (filter && !filter[0])
		filter = NULL;

	list = rgd3_cvar_sorted(&n);
	for (i = 0; i < n; ++i)
	{
		Rgd3Cvar* cv = list[i];
		int f = cv->flags;
		if (filter && !rgd3_str_filter(filter, cv->name, 0))
			continue;
		switch (show)
		{
		case SHOW_VALUE:
			rgd3_printf("%-32s ^7\"%s\"\n", cv->name, cv->value);
			break;
		case SHOW_DESCRIPTION:
			rgd3_printf("%-32s ^7%s\n", cv->name, cv->description ? cv->description : "");
			break;
		case SHOW_TYPE:
			if (f & RGD3_CVAR_BOOL)
				rgd3_printf("%-32s ^5bool\n", cv->name);
			else if ((f & RGD3_CVAR_INTEGER) && cv->value_min < cv->value_max)
				rgd3_printf("%-32s ^2int ^7[%d, %d]\n", cv->name, (int)cv->value_min, (int)cv->value_max);
			else if (f & RGD3_CVAR_INTEGER)
				rgd3_printf("%-32s ^2int\n", cv->name);
			else if ((f & RGD3_CVAR_FLOAT) && cv->value_min < cv->value_max)
			{
				char lo[32], hi[32];
				rgd3_str_float_to_string(cv->value_min, lo, sizeof(lo));
				rgd3_str_float_to_string(cv->value_max, hi, sizeof(hi));
				rgd3_printf("%-32s ^1float ^7[%s, %s]\n", cv->name, lo, hi);
			}
			else if (f & RGD3_CVAR_FLOAT)
				rgd3_printf("%-32s ^1float\n", cv->name);
			else if (cv->value_strings)
			{
				int j;
				rgd3_printf("%-32s ^7string ^7[", cv->name);
				for (j = 0; cv->value_strings[j]; ++j)
					rgd3_printf(j ? "^7, %s" : "^7%s", cv->value_strings[j]);
				rgd3_printf("^7]\n");
			}
			else
				rgd3_printf("%-32s ^7string\n", cv->name);
			break;
		case SHOW_FLAGS:
			rgd3_printf("%-32s %s%s%s%s%s%s%s%s%s%s\n", cv->name,
				(f & RGD3_CVAR_BOOL) ? "^5B " : (f & RGD3_CVAR_INTEGER) ? "^2I " : (f & RGD3_CVAR_FLOAT) ? "^1F " : "^7S ",
				(f & RGD3_CVAR_SYSTEM) ? "^7SYS  " : (f & RGD3_CVAR_RENDERER) ? "^7RNDR " : (f & RGD3_CVAR_SOUND) ? "^7SND  " :
				(f & RGD3_CVAR_GUI) ? "^7GUI  " : (f & RGD3_CVAR_GAME) ? "^7GAME " : (f & RGD3_CVAR_TOOL) ? "^7TOOL " : "^7     ",
				(f & RGD3_CVAR_SERVERINFO) ? "SI " : "   ",
				(f & RGD3_CVAR_STATIC) ? "ST " : "   ",
				(f & RGD3_CVAR_CHEAT) ? "CH " : "   ",
				(f & RGD3_CVAR_INIT) ? "IN " : "   ",
				(f & RGD3_CVAR_ROM) ? "RO " : "   ",
				(f & RGD3_CVAR_ARCHIVE) ? "AR " : "   ",
				(f & RGD3_CVAR_MODIFIED) ? "MO " : "   ",
				"");
			break;
		}
		++listed;
	}
	rgd3_heap_free(list);
	rgd3_printf("\n%d cvars listed\n", listed);
}

void rgd3_cvar_register_commands(void)
{
	rgd3_cmd_add("set", rgd3_cvar_cmd_set, RGD3_CMD_FL_SYSTEM, "sets a cvar", NULL);
	rgd3_cmd_add("seta", rgd3_cvar_cmd_seta, RGD3_CMD_FL_SYSTEM, "sets a cvar and flags it as archive", NULL);
	rgd3_cmd_add("sets", rgd3_cvar_cmd_sets, RGD3_CMD_FL_SYSTEM, "sets a cvar and flags it as server info", NULL);
	rgd3_cmd_add("sett", rgd3_cvar_cmd_sett, RGD3_CMD_FL_SYSTEM, "sets a cvar and flags it as tool", NULL);
	rgd3_cmd_add("setu", rgd3_cvar_cmd_setu, RGD3_CMD_FL_SYSTEM, "sets a cvar and flags it as user info", NULL);
	rgd3_cmd_add("toggle", rgd3_cvar_cmd_toggle, RGD3_CMD_FL_SYSTEM, "toggles a cvar", NULL);
	rgd3_cmd_add("reset", rgd3_cvar_cmd_reset, RGD3_CMD_FL_SYSTEM, "resets a cvar", NULL);
	rgd3_cmd_add("listCvars", rgd3_cvar_cmd_list, RGD3_CMD_FL_SYSTEM, "lists cvars", NULL);
	rgd3_cmd_add("cvar_restart", rgd3_cvar_cmd_restart, RGD3_CMD_FL_SYSTEM, "restart the cvar system", NULL);
	rgd3_cmd_add("cvarAdd", rgd3_cvar_cmd_add, RGD3_CMD_FL_SYSTEM, "adds a value to a numeric cvar", NULL);
}

void rgd3_cvar_shutdown(void)
{
	int i;
	for (i = 0; i < RGD3_CVAR_HASH_SIZE; ++i)
	{
		Rgd3Cvar* cv = rgd3_cvars.hash[i];
		while (cv)
		{
			Rgd3Cvar* next = cv->next;
			if (cv->dynamic)
			{
				rgd3_cvar_free_dynamic(cv);
			}
			else
			{
				rgd3_heap_free(cv->value);
				rgd3_heap_free(cv->reset_value);
				cv->value = NULL;
				cv->reset_value = NULL;
				cv->registered = 0;
			}
			cv = next;
		}
	}
	memset(&rgd3_cvars, 0, sizeof(rgd3_cvars));
}
