/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_error.c - drop/fatal policy from Common_printf.cpp: a drop stops the
   current map and returns to the innermost guard; three drops within 100 ms
   escalate to fatal; a drop without a guard is fatal. */

typedef struct Rgd3ErrorState
{
	Rgd3ErrorGuard* top;
	Rgd3ErrorHook drop_hook;
	Rgd3ErrorHook fatal_hook;
	int last_drop_time;
	int drop_count;
	int in_fatal;
	char message[RGD3_MAX_PRINT_MSG];
} Rgd3ErrorState;

static Rgd3ErrorState rgd3_error;

void rgd3_error_set_drop_hook(Rgd3ErrorHook hook)
{
	rgd3_error.drop_hook = hook;
}

void rgd3_error_set_fatal_hook(Rgd3ErrorHook hook)
{
	rgd3_error.fatal_hook = hook;
}

/* ---- unwind registry ---- */

typedef struct Rgd3UnwindEntry
{
	void* object;
	Rgd3UnwindFn fn;
	u64 seq; /* registration order; a guard frees everything newer than itself */
} Rgd3UnwindEntry;

/* plain malloc: the registry must not show up in the heap tag counters */
static Rgd3UnwindEntry* rgd3_unwind_entries;
static int rgd3_unwind_num;
static int rgd3_unwind_cap;
static u64 rgd3_unwind_seq;

/* only objects on the main thread's stack: heap objects belong to their
   owners, and no other thread ever drops */
void rgd3_error_unwind_register(void* object, Rgd3UnwindFn fn)
{
	int i;
	if (!object || !rgd3_thread_is_main() || !rgd3_thread_on_main_stack(object))
		return;
	for (i = rgd3_unwind_num - 1; i >= 0; i--)
	{
		if (rgd3_unwind_entries[i].object == object)
		{
			/* already registered: keep its original order */
			rgd3_unwind_entries[i].fn = fn;
			return;
		}
	}
	if (rgd3_unwind_num == rgd3_unwind_cap)
	{
		int cap = rgd3_unwind_cap ? rgd3_unwind_cap * 2 : 64;
		Rgd3UnwindEntry* grown = (Rgd3UnwindEntry*)realloc(rgd3_unwind_entries, (size_t)cap * sizeof(Rgd3UnwindEntry));
		if (!grown)
			return;
		rgd3_unwind_entries = grown;
		rgd3_unwind_cap = cap;
	}
	rgd3_unwind_entries[rgd3_unwind_num].object = object;
	rgd3_unwind_entries[rgd3_unwind_num].fn = fn;
	rgd3_unwind_entries[rgd3_unwind_num].seq = ++rgd3_unwind_seq;
	rgd3_unwind_num++;
}

void rgd3_error_unwind_unregister(void* object)
{
	int i;
	if (!rgd3_thread_is_main())
		return; /* workers never register, and must not touch the array */
	for (i = rgd3_unwind_num - 1; i >= 0; i--)
	{
		if (rgd3_unwind_entries[i].object == object)
		{
			rgd3_unwind_entries[i] = rgd3_unwind_entries[--rgd3_unwind_num];
			return;
		}
	}
}

int rgd3_error_unwind_count(void)
{
	return rgd3_unwind_num;
}

/* frees every object registered after `seq`; a free function may register
   or unregister other objects, so rescan after each call */
static void rgd3_error_unwind(u64 seq)
{
	for (;;)
	{
		int i;
		Rgd3UnwindEntry e;
		for (i = rgd3_unwind_num - 1; i >= 0; i--)
		{
			if (rgd3_unwind_entries[i].seq > seq)
				break;
		}
		if (i < 0)
			return;
		e = rgd3_unwind_entries[i];
		rgd3_unwind_entries[i] = rgd3_unwind_entries[--rgd3_unwind_num];
		e.fn(e.object);
	}
}

/* for tests that drop on purpose: forget recent drops so the "three in
   100 ms is fatal" rule does not trigger */
void rgd3_error_reset_drop_count(void)
{
	rgd3_error.drop_count = 0;
	rgd3_error.last_drop_time = -1000;
}

void rgd3_error_shutdown(void)
{
	free(rgd3_unwind_entries);
	rgd3_unwind_entries = NULL;
	rgd3_unwind_num = 0;
	rgd3_unwind_cap = 0;
}

void rgd3_error_push(Rgd3ErrorGuard* guard)
{
	guard->unwind_seq = rgd3_unwind_seq;
	guard->prev = rgd3_error.top;
	rgd3_error.top = guard;
}

void rgd3_error_pop(Rgd3ErrorGuard* guard)
{
	if (rgd3_error.top == guard)
		rgd3_error.top = guard->prev;
}

const char* rgd3_error_message(void)
{
	return rgd3_error.message;
}

RGD3_NORETURN void rgd3_error_fatal(const char* fmt, ...)
{
	va_list args;
	char msg[RGD3_MAX_PRINT_MSG];
	va_start(args, fmt);
	rg_vsnprintf(msg, sizeof(msg), fmt, args);
	va_end(args);
	if (rgd3_error.in_fatal)
	{
		fprintf(stderr, "recursive fatal error: %s\n", msg);
		exit(1);
	}
	rgd3_error.in_fatal = 1;
	rgd3_str_copynz(rgd3_error.message, msg, sizeof(rgd3_error.message));
	rgd3_printf("^1FATAL ERROR: %s\n", msg);
	if (rgd3_error.fatal_hook)
		rgd3_error.fatal_hook();
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, RGD3_ENGINE_NAME " fatal error", msg, NULL);
	exit(1);
}

RGD3_NORETURN void rgd3_error_drop(const char* fmt, ...)
{
	va_list args;
	Rgd3ErrorGuard* guard;
	int now;
	va_start(args, fmt);
	rg_vsnprintf(rgd3_error.message, sizeof(rgd3_error.message), fmt, args);
	va_end(args);

	now = rgd3_time_ms();
	if (now - rgd3_error.last_drop_time < 100)
	{
		if (++rgd3_error.drop_count > 2)
			rgd3_error_fatal("recursive error after: %s", rgd3_error.message);
	}
	else
	{
		rgd3_error.drop_count = 0;
	}
	rgd3_error.last_drop_time = now;

	guard = rgd3_error.top;
	if (!guard)
		rgd3_error_fatal("%s", rgd3_error.message);
	rgd3_printf("^1ERROR: %s\n", rgd3_error.message);
	if (rgd3_error.drop_hook)
		rgd3_error.drop_hook();
	rgd3_error.top = guard->prev;
	/* free the stack objects registered since the guard was pushed */
	rgd3_error_unwind(guard->unwind_seq);
	longjmp(guard->jump, 1);
}

static void rgd3_error_cmd_error(const Rgd3CmdArgs* args)
{
	if (!rgd3_log_developer())
	{
		rgd3_printf("error may only be used in developer mode\n");
		return;
	}
	if (rgd3_cmdargs_argc(args) > 1)
		rgd3_error_drop("%s", rgd3_cmdargs_args(args, 1, -1, 0));
	rgd3_error_drop("testing error");
}

void rgd3_error_register(void)
{
	rgd3_cmd_add("error", rgd3_error_cmd_error, RGD3_CMD_FL_SYSTEM, "causes an error (developer mode)", NULL);
}
