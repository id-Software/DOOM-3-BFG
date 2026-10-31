/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_hooks.c - call tracing for the subsystem entry points (rgd3_hooks.h).
   com_traceHooks 1 prints each call; the self-test reads the recorded
   order to check map changes against Common_load.cpp's sequence. */

static Rgd3Cvar cv_com_traceHooks = RGD3_CVAR("com_traceHooks", "0", RGD3_CVAR_BOOL | RGD3_CVAR_SYSTEM,
	"print every call from the framework into the game, renderer and sound system");

static char rgd3_hooks_trace_buffer[4096];
static int rgd3_hooks_trace_len;

void rgd3_hooks_trace(const char* name)
{
	size_t n = strlen(name);
	if (cv_com_traceHooks.integer_value)
		rgd3_printf("^5hook^7 %s\n", name);
	/* keep the most recent calls; a full buffer starts over */
	if (rgd3_hooks_trace_len + (int)n + 2 > (int)sizeof(rgd3_hooks_trace_buffer))
		rgd3_hooks_trace_len = 0;
	if (rgd3_hooks_trace_len)
		rgd3_hooks_trace_buffer[rgd3_hooks_trace_len++] = ' ';
	memcpy(rgd3_hooks_trace_buffer + rgd3_hooks_trace_len, name, n);
	rgd3_hooks_trace_len += (int)n;
	rgd3_hooks_trace_buffer[rgd3_hooks_trace_len] = 0;
}

void rgd3_hooks_trace_reset(void)
{
	rgd3_hooks_trace_len = 0;
	rgd3_hooks_trace_buffer[0] = 0;
}

const char* rgd3_hooks_trace_get(void)
{
	return rgd3_hooks_trace_buffer;
}

void rgd3_hooks_register(void)
{
	rgd3_cvar_register(&cv_com_traceHooks);
}
