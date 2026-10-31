/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_log.c - behaviour from neo/framework/Common_printf.cpp: developer-only
   DPrintf/DWarning and a de-duplicated warning list printed by printWarnings. */

typedef struct Rgd3LogState
{
	SDL_Mutex* lock;
	char* warnings[RGD3_MAX_WARNING_LIST];
	int num_warnings;
	char warning_reason[128];
} Rgd3LogState;

static Rgd3LogState rgd3_log;

static Rgd3Cvar cv_developer = RGD3_CVAR("developer", "0", RGD3_CVAR_BOOL | RGD3_CVAR_SYSTEM | RGD3_CVAR_NOCHEAT, "developer mode");

void rgd3_log_init(void)
{
	if (!rgd3_log.lock)
		rgd3_log.lock = SDL_CreateMutex();
}

void rgd3_log_shutdown(void)
{
	rgd3_log_clear_warnings(NULL);
	if (rgd3_log.lock)
		SDL_DestroyMutex(rgd3_log.lock);
	rgd3_log.lock = NULL;
}

int rgd3_log_developer(void)
{
	return cv_developer.registered && cv_developer.integer_value != 0;
}

static void rgd3_log_emit(const char* text)
{
	static char plain[RGD3_MAX_PRINT_MSG];
	if (rgd3_log.lock)
		SDL_LockMutex(rgd3_log.lock);
	rgd3_console_print(text);
	rgd3_str_copynz(plain, text, sizeof(plain));
	rgd3_str_remove_colors(plain);
	fputs(plain, stdout);
	fflush(stdout);
	if (rgd3_log.lock)
		SDL_UnlockMutex(rgd3_log.lock);
}

void rgd3_vprintf(const char* fmt, va_list args)
{
	char msg[RGD3_MAX_PRINT_MSG];
	rg_vsnprintf(msg, sizeof(msg), fmt, args);
	rgd3_log_emit(msg);
}

void rgd3_printf(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	rgd3_vprintf(fmt, args);
	va_end(args);
}

void rgd3_dprintf(const char* fmt, ...)
{
	va_list args;
	if (!rgd3_log_developer())
		return;
	va_start(args, fmt);
	rgd3_vprintf(fmt, args);
	va_end(args);
}

static void rgd3_log_add_warning(const char* msg)
{
	int i;
	if (rgd3_log.num_warnings >= RGD3_MAX_WARNING_LIST)
		return;
	for (i = 0; i < rgd3_log.num_warnings; ++i)
		if (strcmp(rgd3_log.warnings[i], msg) == 0)
			return;
	rgd3_log.warnings[rgd3_log.num_warnings++] = rgd3_heap_strdup(msg, RGD3_TAG_SYSTEM);
}

static SDL_AtomicInt rgd3_log_total_warnings_count;

/* every warning printed since startup, for tests that expect none */
int rgd3_log_total_warnings(void)
{
	return SDL_GetAtomicInt(&rgd3_log_total_warnings_count);
}

static void rgd3_log_vwarning(const char* fmt, va_list args)
{
	char msg[RGD3_MAX_PRINT_MSG];
	SDL_AddAtomicInt(&rgd3_log_total_warnings_count, 1);
	rg_vsnprintf(msg, sizeof(msg), fmt, args);
	rgd3_str_strip_trailing_whitespace(msg);
	rgd3_printf("^3WARNING: %s\n", msg);
	rgd3_log_add_warning(msg);
}

void rgd3_warning(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	rgd3_log_vwarning(fmt, args);
	va_end(args);
}

void rgd3_dwarning(const char* fmt, ...)
{
	va_list args;
	if (!rgd3_log_developer())
		return;
	va_start(args, fmt);
	rgd3_log_vwarning(fmt, args);
	va_end(args);
}

void rgd3_log_print_warnings(void)
{
	int i;
	if (!rgd3_log.num_warnings)
		return;
	rgd3_printf("------------- Warnings ---------------\n");
	rgd3_printf("during %s...\n", rgd3_log.warning_reason[0] ? rgd3_log.warning_reason : "startup");
	for (i = 0; i < rgd3_log.num_warnings; ++i)
		rgd3_printf("^3WARNING: %s\n", rgd3_log.warnings[i]);
	rgd3_printf("%d warning%s\n", rgd3_log.num_warnings, rgd3_log.num_warnings == 1 ? "" : "s");
}

void rgd3_log_clear_warnings(const char* reason)
{
	int i;
	for (i = 0; i < rgd3_log.num_warnings; ++i)
		rgd3_heap_free(rgd3_log.warnings[i]);
	rgd3_log.num_warnings = 0;
	rgd3_str_copynz(rgd3_log.warning_reason, reason ? reason : "", sizeof(rgd3_log.warning_reason));
}

static void rgd3_log_cmd_print_warnings(const Rgd3CmdArgs* args)
{
	RG_UNUSED(args);
	rgd3_log_print_warnings();
}

void rgd3_log_register(void)
{
	rgd3_cvar_register(&cv_developer);
	rgd3_cmd_add("printWarnings", rgd3_log_cmd_print_warnings, RGD3_CMD_FL_SYSTEM, "prints the warning list", NULL);
}
