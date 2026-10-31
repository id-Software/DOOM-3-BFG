/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_console.c - the developer console, structure-faithful to
   neo/framework/Console.cpp (idConsoleLocal): the 0x30000-cell text buffer
   of (colour << 8 | char) cells wrapped at print time, Linefeed / Print /
   PageUp / PageDown / Top / Bottom, four notify times, con_speed /
   con_notifyTime / con_noPrint, ProcessEvent (console key, chars, key downs)
   and KeyDownEvent (F-key bindings, Ctrl-L, Enter, Tab, history, scrolling),
   DrawSolidConsole / DrawNotify / DrawInput, clear and conDump.
   neo/framework/ConsoleHistory.cpp gives the history (64 lines, up/down
   points, repeat suppression, consoleHistory.txt). Colours are
   g_color_table from neo/idlib/Str.cpp. The stdin thread replaces the
   Win32 dedicated console window (neo/sys/win32/win_syscon.cpp).

   Deviations, each small:
   - Layout is in overlay units (5x7 glyphs in 6x10 cells, about 640x360
     for 16:9) instead of the 640x480 virtual screen with 8x16 SMALLCHARs;
     the line width is a fixed RGD3_CON_LINE_WIDTH (100) instead of 70.
   - Print's word-wrap test compares the previous cell's character (the
     original compared the whole cell, colour bits included, so it only
     wrapped at the start of a line); DrawSolidConsole skips the empty
     current line through the column of the current line (the original
     tested a local loop variable that is never 0 there).
   - SDL delivers no control characters as text and no key repeats, so
     Backspace and Ctrl+V/C/H go to the edit field's CharEvent codes, and
     Scroll() repeats held PgUp/PgDn/Backspace/Delete/Left/Right.
   - `toggleconsole` (Quake lineage, not in BFG) toggles like the console
     key so stdin-driven runs can open it; com_allowConsole defaults to 1
     (the original's non-retail value) and is registered here until a
     Common module owns it.
   - Not ported: overlay text (PrintOverlay), debug graphs, DrawFPS /
     DrawMemoryUsage (the app draws its own diagnostics), the
     UpdateScreen call after Enter, Sys_GrabMouseCursor on close. */

#define RGD3_CON_FIRSTREPEAT 200
#define RGD3_CON_REPEAT 50
#define RGD3_CON_SAFE_LEFT 14.0f /* LOCALSAFE_LEFT in overlay units */
#define RGD3_CON_CW ((float)RGD3_OVERLAY_CHAR_WIDTH)
#define RGD3_CON_CH ((float)RGD3_OVERLAY_CHAR_HEIGHT)

/* idStr::ColorForIndex (g_color_table, neo/idlib/Str.cpp) */
static const float rgd3_console_color_table[16][4] = {
	{ 0.0f, 0.0f, 0.0f, 1.0f },
	{ 1.0f, 0.0f, 0.0f, 1.0f }, /* ^1 red */
	{ 0.0f, 1.0f, 0.0f, 1.0f }, /* ^2 green */
	{ 1.0f, 1.0f, 0.0f, 1.0f }, /* ^3 yellow */
	{ 0.0f, 0.0f, 1.0f, 1.0f }, /* ^4 blue */
	{ 0.0f, 1.0f, 1.0f, 1.0f }, /* ^5 cyan */
	{ 1.0f, 0.5f, 0.0f, 1.0f }, /* ^6 orange */
	{ 1.0f, 1.0f, 1.0f, 1.0f }, /* ^7 white */
	{ 0.5f, 0.5f, 0.5f, 1.0f }, /* ^8 gray */
	{ 0.0f, 0.0f, 0.0f, 1.0f }, /* ^9 black */
	{ 0.0f, 0.0f, 0.0f, 1.0f },
	{ 0.0f, 0.0f, 0.0f, 1.0f },
	{ 0.0f, 0.0f, 0.0f, 1.0f },
	{ 0.0f, 0.0f, 0.0f, 1.0f },
	{ 0.0f, 0.0f, 0.0f, 1.0f },
	{ 0.0f, 0.0f, 0.0f, 1.0f },
};

#define RGD3_COLOR_INDEX(c) ((c) & 15) /* idStr::ColorIndex */
#define RGD3_C_COLOR_DEFAULT '0'
#define RGD3_C_COLOR_CYAN '5'
#define RGD3_C_COLOR_WHITE '7'

typedef struct Rgd3ConsoleHistory
{
	int up_point;		/* command to be retrieved with the next up arrow */
	int down_point;		/* command to be retrieved with the next down arrow */
	int return_line;	/* last returned by retrieve */
	int num_history;
	char lines[RGD3_CON_COMMAND_HISTORY][RGD3_MAX_EDIT_LINE];
	int write_file;		/* set by rgd3_console_load_history */
} Rgd3ConsoleHistory;

typedef struct Rgd3ConsoleState
{
	SDL_Mutex* lock;	/* text, current, x, display, times */
	int key_catching;
	int text_input_on;

	u16 text[RGD3_CON_TEXTSIZE];
	int line_width;
	int total_lines;
	int current;		/* line where the next message is printed */
	int x;				/* offset in the current line for the next print */
	int display;		/* bottom of the console displays this line */
	int times[RGD3_CON_NUM_TIMES]; /* when each notify line was printed */

	int repeat_key;		/* Scroll(): held key being repeated */
	int repeat_time;

	float display_frac;	/* approaches final_frac at con_speed */
	float final_frac;	/* 0.0 to 1.0 of the screen */
	int frac_time;
	float vislines;
	u32 frame_number;	/* cursor blink, as idLib::frameNumber */

	Rgd3EditField field;
	Rgd3ConsoleHistory history;
	int stdin_started;
} Rgd3ConsoleState;

static Rgd3ConsoleState rgd3_console;

static Rgd3Cvar cv_con_speed = RGD3_CVAR("con_speed", "3", RGD3_CVAR_SYSTEM, "speed at which the console moves up and down");
static Rgd3Cvar cv_con_notifyTime = RGD3_CVAR("con_notifyTime", "3", RGD3_CVAR_SYSTEM, "time messages are displayed onscreen when console is pulled up");
#if defined(RGD3_DEBUG_BUILD)
static Rgd3Cvar cv_con_noPrint = RGD3_CVAR("con_noPrint", "0", RGD3_CVAR_BOOL | RGD3_CVAR_SYSTEM | RGD3_CVAR_NOCHEAT, "print on the console but not onscreen when console is pulled up");
#else
static Rgd3Cvar cv_con_noPrint = RGD3_CVAR("con_noPrint", "1", RGD3_CVAR_BOOL | RGD3_CVAR_SYSTEM | RGD3_CVAR_NOCHEAT, "print on the console but not onscreen when console is pulled up");
#endif
static Rgd3Cvar cv_com_allowConsole = RGD3_CVAR("com_allowConsole", "1", RGD3_CVAR_BOOL | RGD3_CVAR_SYSTEM | RGD3_CVAR_INIT, "allow toggling console with the tilde key");

static void rgd3_console_lock(void)
{
	if (rgd3_console.lock)
		SDL_LockMutex(rgd3_console.lock);
}

static void rgd3_console_unlock(void)
{
	if (rgd3_console.lock)
		SDL_UnlockMutex(rgd3_console.lock);
}

static const float* rgd3_console_color(int index)
{
	return rgd3_console_color_table[index & 15];
}

static int rgd3_console_ctrl_down(void)
{
	return rgd3_keys_is_down(RGD3_K_LCTRL) || rgd3_keys_is_down(RGD3_K_RCTRL);
}

/* ---------------------------------------------------------------------------
   Text buffer
--------------------------------------------------------------------------- */

static u16 rgd3_console_blank(void)
{
	return (u16)((RGD3_COLOR_INDEX(RGD3_C_COLOR_CYAN) << 8) | ' ');
}

static void rgd3_console_bottom(void)
{
	rgd3_console.display = rgd3_console.current;
}

static void rgd3_console_clear_locked(void)
{
	int i;
	u16 blank = rgd3_console_blank();
	for (i = 0; i < RGD3_CON_TEXTSIZE; i++)
		rgd3_console.text[i] = blank;
	rgd3_console_bottom();
}

void rgd3_console_clear(void)
{
	rgd3_console_lock();
	rgd3_console_clear_locked();
	rgd3_console_unlock();
}

void rgd3_console_init(void)
{
	memset(&rgd3_console, 0, sizeof(rgd3_console));
	rgd3_console.lock = SDL_CreateMutex();
	rgd3_console.line_width = RGD3_CON_LINE_WIDTH;
	rgd3_console.total_lines = RGD3_CON_TEXTSIZE / rgd3_console.line_width;
	rgd3_console.repeat_key = RGD3_K_NONE;
	rgd3_console_clear_locked();
	rgd3_editfield_clear(&rgd3_console.field);
	rgd3_editfield_set_width_in_chars(&rgd3_console.field, rgd3_console.line_width);
}

static void rgd3_console_set_key_catching(int on)
{
	SDL_Window* window = rgd3_app_window();
	rgd3_console.key_catching = on;
	/* SDL sends text events only while text input is on */
	if (window && on && !rgd3_console.text_input_on)
		rgd3_console.text_input_on = SDL_StartTextInput(window) ? 1 : 0;
	else if (window && !on && rgd3_console.text_input_on)
	{
		SDL_StopTextInput(window);
		rgd3_console.text_input_on = 0;
	}
}

void rgd3_console_shutdown(void)
{
	/* idempotent; prints after this still land in the buffer */
	rgd3_console_set_key_catching(0);
	rgd3_console.text_input_on = 0;
	rgd3_console.history.write_file = 0;
}

/* Linefeed: called with the lock held */
static void rgd3_console_linefeed(void)
{
	int i;
	/* mark time for the transparent overlay */
	if (rgd3_console.current >= 0)
		rgd3_console.times[rgd3_console.current % RGD3_CON_NUM_TIMES] = rgd3_time_ms();
	rgd3_console.x = 0;
	if (rgd3_console.display == rgd3_console.current)
		rgd3_console.display++;
	rgd3_console.current++;
	for (i = 0; i < rgd3_console.line_width; i++)
	{
		int offset = (int)(((unsigned int)rgd3_console.current % (unsigned int)rgd3_console.total_lines) * (unsigned int)rgd3_console.line_width) + i;
		rgd3_console.text[offset] = rgd3_console_blank();
	}
}

/* Print: cursor positioning, colour escapes, word wrap. Any thread. */
void rgd3_console_print(const char* txt)
{
	int y, c, l, color;
	const int lw = rgd3_console.line_width;
	if (rgd3_console.total_lines == 0)
		return; /* not yet initialized */
	rgd3_console_lock();
	color = RGD3_COLOR_INDEX(RGD3_C_COLOR_CYAN);
	while ((c = *(const unsigned char*)txt) != 0)
	{
		if (rgd3_str_is_color(txt))
		{
			if (txt[1] == RGD3_C_COLOR_DEFAULT)
				color = RGD3_COLOR_INDEX(RGD3_C_COLOR_CYAN);
			else
				color = RGD3_COLOR_INDEX(txt[1]);
			txt += 2;
			continue;
		}

		y = rgd3_console.current % rgd3_console.total_lines;

		/* about to print a new word: wrap to a new line if it does not fit */
		if (c > ' ' && (rgd3_console.x == 0 || (rgd3_console.text[y * lw + rgd3_console.x - 1] & 0xff) <= ' '))
		{
			for (l = 0; l < lw; l++)
				if ((unsigned char)txt[l] <= ' ')
					break;
			if (l != lw && rgd3_console.x + l >= lw)
			{
				rgd3_console_linefeed();
				y = rgd3_console.current % rgd3_console.total_lines;
			}
		}

		txt++;

		switch (c)
		{
			case '\n':
				rgd3_console_linefeed();
				break;
			case '\t':
				do
				{
					rgd3_console.text[y * lw + rgd3_console.x] = (u16)((color << 8) | ' ');
					rgd3_console.x++;
					if (rgd3_console.x >= lw)
					{
						rgd3_console_linefeed();
						y = rgd3_console.current % rgd3_console.total_lines;
						rgd3_console.x = 0;
					}
				} while (rgd3_console.x & 3);
				break;
			case '\r':
				rgd3_console.x = 0;
				break;
			default: /* display the character and advance */
				rgd3_console.text[y * lw + rgd3_console.x] = (u16)((color << 8) | c);
				rgd3_console.x++;
				if (rgd3_console.x >= lw)
				{
					rgd3_console_linefeed();
					rgd3_console.x = 0;
				}
				break;
		}
	}
	/* mark time for the transparent overlay */
	if (rgd3_console.current >= 0)
		rgd3_console.times[rgd3_console.current % RGD3_CON_NUM_TIMES] = rgd3_time_ms();
	rgd3_console_unlock();
}

static void rgd3_console_page_up(void)
{
	rgd3_console_lock();
	rgd3_console.display -= 2;
	if (rgd3_console.current - rgd3_console.display >= rgd3_console.total_lines)
		rgd3_console.display = rgd3_console.current - rgd3_console.total_lines + 1;
	rgd3_console_unlock();
}

static void rgd3_console_page_down(void)
{
	rgd3_console_lock();
	rgd3_console.display += 2;
	if (rgd3_console.display > rgd3_console.current)
		rgd3_console.display = rgd3_console.current;
	rgd3_console_unlock();
}

static void rgd3_console_top(void)
{
	rgd3_console_lock();
	rgd3_console.display = 0;
	rgd3_console_unlock();
}

static void rgd3_console_to_bottom(void)
{
	rgd3_console_lock();
	rgd3_console_bottom();
	rgd3_console_unlock();
}

void rgd3_console_clear_notify_lines(void)
{
	int i;
	rgd3_console_lock();
	for (i = 0; i < RGD3_CON_NUM_TIMES; i++)
		rgd3_console.times[i] = 0;
	rgd3_console_unlock();
}

static void rgd3_console_set_display_fraction(float frac)
{
	rgd3_console.final_frac = frac;
	rgd3_console.frac_time = rgd3_time_ms();
}

void rgd3_console_close(void)
{
	rgd3_console_set_key_catching(0);
	rgd3_console_set_display_fraction(0.0f);
	rgd3_console.display_frac = 0.0f; /* don't scroll to that point, go immediately */
	rgd3_console_clear_notify_lines();
}

int rgd3_console_active(void)
{
	return rgd3_console.key_catching;
}

/* Saves the console text, without colours, to a file under the save path. */
static void rgd3_console_dump(const char* file_name)
{
	Rgd3File* f;
	char* out;
	size_t used = 0;
	int l, x, i;
	const int lw = rgd3_console.line_width;
	f = rgd3_fs_open_write(file_name);
	if (!f)
	{
		rgd3_warning("couldn't open %s", file_name);
		return;
	}
	out = (char*)rgd3_heap_alloc((size_t)rgd3_console.total_lines * (size_t)(lw + 2) + 1, RGD3_TAG_CONSOLE);
	rgd3_console_lock();
	/* skip empty lines */
	l = rgd3_console.current - rgd3_console.total_lines + 1;
	if (l < 0)
		l = 0;
	for (; l <= rgd3_console.current; l++)
	{
		const u16* line = rgd3_console.text + (l % rgd3_console.total_lines) * lw;
		for (x = 0; x < lw; x++)
			if ((line[x] & 0xff) > ' ')
				break;
		if (x != lw)
			break;
	}
	/* write the remaining lines */
	for (; l <= rgd3_console.current; l++)
	{
		const u16* line = rgd3_console.text + (l % rgd3_console.total_lines) * lw;
		int last = -1;
		for (i = 0; i < lw; i++)
			if ((line[i] & 0xff) > ' ')
				last = i;
		for (i = 0; i <= last; i++)
			out[used++] = (char)(line[i] & 0xff);
		out[used++] = '\r';
		out[used++] = '\n';
	}
	rgd3_console_unlock();
	rgd3_fs_write(f, out, used);
	rgd3_fs_close(f);
	rgd3_heap_free(out);
}

/* ---------------------------------------------------------------------------
   History (idConsoleHistory)
--------------------------------------------------------------------------- */

static void rgd3_console_history_write(void)
{
	Rgd3ConsoleHistory* h = &rgd3_console.history;
	Rgd3File* f = rgd3_fs_open_write(RGD3_CON_HISTORY_FILE);
	int i;
	if (!f)
		return;
	for (i = h->num_history - RGD3_CON_COMMAND_HISTORY; i < h->num_history; i++)
	{
		if (i < 0)
			continue;
		rgd3_fs_printf(f, "%s\n", h->lines[i & (RGD3_CON_COMMAND_HISTORY - 1)]);
	}
	rgd3_fs_close(f);
}

/* Empty lines never change the history; repeating the last command does
   not add it again. */
static void rgd3_console_history_add(const char* line, int write_file)
{
	Rgd3ConsoleHistory* h = &rgd3_console.history;
	const char* s;
	if (!line)
		return;
	for (s = line; *s; s++)
		if (*s > ' ')
			break;
	if (!*s)
		return;

	if (rgd3_str_icmp(h->lines[(h->num_history - 1) & (RGD3_CON_COMMAND_HISTORY - 1)], line) == 0)
	{
		/* retrieved from the history: up arrow gets the same command again;
		   typed again: leave the up/down points alone */
		if (rgd3_str_icmp(h->lines[h->return_line & (RGD3_CON_COMMAND_HISTORY - 1)], line) == 0)
			h->up_point = h->return_line;
		return;
	}
	/* older lines retrieved with the up arrow: reset the points to the end */
	h->up_point = h->num_history;
	h->return_line = h->num_history;
	h->down_point = h->num_history + 1;
	rgd3_str_copynz(h->lines[h->num_history & (RGD3_CON_COMMAND_HISTORY - 1)], line, RGD3_MAX_EDIT_LINE);
	h->num_history++;

	if (write_file && h->write_file)
		rgd3_console_history_write();
}

/* Returns "" when there is nothing more in that direction. */
static const char* rgd3_console_history_retrieve(int backward)
{
	Rgd3ConsoleHistory* h = &rgd3_console.history;
	if (h->num_history == 0)
		return "";
	if (backward)
	{
		if (h->up_point < h->num_history - RGD3_CON_COMMAND_HISTORY || h->up_point < 0)
			return "";
		h->return_line = h->up_point;
		h->down_point = h->up_point + 1;
		h->up_point--;
	}
	else
	{
		if (h->down_point >= h->num_history)
			return "";
		h->return_line = h->down_point;
		h->up_point = h->down_point - 1;
		h->down_point++;
	}
	return h->lines[h->return_line & (RGD3_CON_COMMAND_HISTORY - 1)];
}

static void rgd3_console_history_clear(void)
{
	Rgd3ConsoleHistory* h = &rgd3_console.history;
	h->up_point = 0;
	h->down_point = 0;
	h->return_line = 0;
	h->num_history = 0;
}

void rgd3_console_load_history(void)
{
	void* buffer = NULL;
	int len = rgd3_fs_read_file(RGD3_CON_HISTORY_FILE, &buffer, NULL);
	if (len > 0 && buffer)
	{
		const char* p = (const char*)buffer;
		const char* end = p + len;
		while (p < end)
		{
			char line[RGD3_MAX_EDIT_LINE];
			const char* eol = p;
			size_t n;
			while (eol < end && *eol != '\n')
				eol++;
			n = (size_t)(eol - p);
			if (n >= sizeof(line))
				n = sizeof(line) - 1;
			memcpy(line, p, n);
			line[n] = 0;
			rgd3_str_strip_trailing_whitespace(line);
			rgd3_console_history_add(line, 0);
			p = eol < end ? eol + 1 : end;
		}
	}
	if (buffer)
		rgd3_fs_free_file(buffer);
	rgd3_console.history.write_file = 1;
}

/* ---------------------------------------------------------------------------
   Input
--------------------------------------------------------------------------- */

static int rgd3_console_is_repeat_key(int key)
{
	return key == RGD3_K_PGUP || key == RGD3_K_PGDN || key == RGD3_K_BACKSPACE || key == RGD3_K_DEL ||
		key == RGD3_K_LEFTARROW || key == RGD3_K_RIGHTARROW;
}

/* Handles history and scrollback; the rest goes to the edit field. */
static void rgd3_console_key_down_event(int key)
{
	/* F-key bindings still run (the original's K_F1..K_F12 range test also
	   caught the keypad and lock keys numbered between F10 and F11) */
	if ((key >= RGD3_K_F1 && key <= RGD3_K_F10) || key == RGD3_K_F11 || key == RGD3_K_F12)
	{
		rgd3_keys_exec_key_binding(key);
		return;
	}

	/* ctrl-L clears the screen */
	if (key == RGD3_K_L && rgd3_console_ctrl_down())
	{
		rgd3_console_clear();
		return;
	}

	/* enter finishes the line */
	if (key == RGD3_K_ENTER || key == RGD3_K_KP_ENTER)
	{
		const char* line = rgd3_editfield_buffer(&rgd3_console.field);
		rgd3_printf("]%s\n", line);
		rgd3_cmd_buffer_text(RGD3_CMD_EXEC_APPEND, line);
		rgd3_cmd_buffer_text(RGD3_CMD_EXEC_APPEND, "\n");
		if (line[0] != '\n' && line[0] != '\0')
			rgd3_console_history_add(line, 1);
		rgd3_editfield_clear(&rgd3_console.field);
		rgd3_editfield_set_width_in_chars(&rgd3_console.field, rgd3_console.line_width);
		return;
	}

	/* command completion */
	if (key == RGD3_K_TAB)
	{
		rgd3_editfield_auto_complete(&rgd3_console.field);
		return;
	}

	/* command history (ctrl-p ctrl-n for unix style) */
	if (key == RGD3_K_UPARROW || (key == RGD3_K_P && rgd3_console_ctrl_down()))
	{
		const char* hist = rgd3_console_history_retrieve(1);
		if (hist[0])
			rgd3_editfield_set_buffer(&rgd3_console.field, hist);
		return;
	}
	if (key == RGD3_K_DOWNARROW || (key == RGD3_K_N && rgd3_console_ctrl_down()))
	{
		const char* hist = rgd3_console_history_retrieve(0);
		if (hist[0])
			rgd3_editfield_set_buffer(&rgd3_console.field, hist);
		return;
	}

	/* console scrolling */
	if (key == RGD3_K_PGUP || key == RGD3_K_MWHEELUP)
	{
		rgd3_console_page_up();
		return;
	}
	if (key == RGD3_K_PGDN || key == RGD3_K_MWHEELDOWN)
	{
		rgd3_console_page_down();
		return;
	}

	/* ctrl-home = top of console, ctrl-end = bottom */
	if (key == RGD3_K_HOME && rgd3_console_ctrl_down())
	{
		rgd3_console_top();
		return;
	}
	if (key == RGD3_K_END && rgd3_console_ctrl_down())
	{
		rgd3_console_to_bottom();
		return;
	}

	/* the normal edit line; SDL text input carries no control characters,
	   so the CharEvent control codes come from the keys here */
	rgd3_editfield_key_down_event(&rgd3_console.field, key);
	if (key == RGD3_K_BACKSPACE)
		rgd3_editfield_char_event(&rgd3_console.field, 'h' - 'a' + 1);
	else if (rgd3_console_ctrl_down() && (key == RGD3_K_V || key == RGD3_K_C || key == RGD3_K_H))
		rgd3_editfield_char_event(&rgd3_console.field, key == RGD3_K_V ? 'v' - 'a' + 1 : key == RGD3_K_C ? 'c' - 'a' + 1 : 'h' - 'a' + 1);
}

/* Scroll(): repeats held keys, since the event loop drops key repeats */
static void rgd3_console_scroll(void)
{
	int now = rgd3_time_ms();
	if (rgd3_console.repeat_key == RGD3_K_NONE)
		return;
	if (!rgd3_console.key_catching || !rgd3_keys_is_down(rgd3_console.repeat_key))
	{
		rgd3_console.repeat_key = RGD3_K_NONE;
		return;
	}
	if (now - rgd3_console.repeat_time < 0)
		return;
	rgd3_console.repeat_time = now + RGD3_CON_REPEAT;
	rgd3_console_key_down_event(rgd3_console.repeat_key);
}

int rgd3_console_process_event(const Rgd3SysEvent* ev, int force_accept)
{
	const int console_key = ev->type == RGD3_SE_KEY && ev->value == RGD3_K_GRAVE && cv_com_allowConsole.integer_value;

	/* the console key is always caught */
	if (!force_accept && console_key)
	{
		/* ignore up events */
		if (ev->value2 == 0)
			return 1;
		rgd3_editfield_clear_auto_complete(&rgd3_console.field);
		/* a down event toggles the destination lines */
		if (rgd3_console.key_catching)
		{
			rgd3_console_close();
		}
		else
		{
			rgd3_editfield_clear(&rgd3_console.field);
			rgd3_console_set_key_catching(1);
			/* shift opens it less far */
			if (rgd3_keys_is_down(RGD3_K_LSHIFT) || rgd3_keys_is_down(RGD3_K_RSHIFT))
				rgd3_console_set_display_fraction(0.2f);
			else
				rgd3_console_set_display_fraction(0.5f);
		}
		return 1;
	}

	/* not key catching: every other event goes on */
	if (!force_accept && !rgd3_console.key_catching)
		return 0;

	if (ev->type == RGD3_SE_CHAR)
	{
		/* never send the console key as a character */
		if (ev->value != '`' && ev->value != '~')
			rgd3_editfield_char_event(&rgd3_console.field, ev->value);
		return 1;
	}

	if (ev->type == RGD3_SE_KEY)
	{
		/* ignore up events */
		if (ev->value2 == 0)
			return 1;
		if (rgd3_console_is_repeat_key(ev->value))
		{
			rgd3_console.repeat_key = ev->value;
			rgd3_console.repeat_time = rgd3_time_ms() + RGD3_CON_FIRSTREPEAT;
		}
		else
		{
			rgd3_console.repeat_key = RGD3_K_NONE;
		}
		rgd3_console_key_down_event(ev->value);
		return 1;
	}

	/* mouse, joystick and console lines are not the console's */
	return 0;
}

/* ---------------------------------------------------------------------------
   Drawing
--------------------------------------------------------------------------- */

/* UpdateDisplayFraction: scrolls towards final_frac at con_speed */
static void rgd3_console_update_display_fraction(void)
{
	int now = rgd3_time_ms();
	float speed = cv_con_speed.float_value;
	if (speed <= 0.1f)
	{
		rgd3_console.frac_time = now;
		rgd3_console.display_frac = rgd3_console.final_frac;
		return;
	}
	if (rgd3_console.final_frac < rgd3_console.display_frac)
	{
		rgd3_console.display_frac -= speed * (float)(now - rgd3_console.frac_time) * 0.001f;
		if (rgd3_console.final_frac > rgd3_console.display_frac)
			rgd3_console.display_frac = rgd3_console.final_frac;
		rgd3_console.frac_time = now;
	}
	else if (rgd3_console.final_frac > rgd3_console.display_frac)
	{
		rgd3_console.display_frac += speed * (float)(now - rgd3_console.frac_time) * 0.001f;
		if (rgd3_console.final_frac < rgd3_console.display_frac)
			rgd3_console.display_frac = rgd3_console.final_frac;
		rgd3_console.frac_time = now;
	}
}

/* one stored line, skipping spaces, changing colour per cell */
static void rgd3_console_draw_line(const u16* text_p, float y, int max_cols)
{
	int x;
	for (x = 0; x < rgd3_console.line_width && x < max_cols; x++)
	{
		int c = text_p[x] & 0xff;
		if (c == ' ')
			continue;
		rgd3_overlay_char(RGD3_CON_SAFE_LEFT + (float)(x + 1) * RGD3_CON_CW, y, c, rgd3_console_color(RGD3_COLOR_INDEX(text_p[x] >> 8)));
	}
}

static int rgd3_console_visible_columns(float vw)
{
	int cols = (int)((vw - RGD3_CON_SAFE_LEFT) / RGD3_CON_CW) - 1;
	return cols < 0 ? 0 : cols;
}

/* the last few lines, drawn over the top of the screen while closed */
static void rgd3_console_draw_notify(float vw)
{
	int i, now = rgd3_time_ms();
	float v = 0.0f;
	if (cv_con_noPrint.integer_value)
		return;
	for (i = rgd3_console.current - RGD3_CON_NUM_TIMES + 1; i <= rgd3_console.current; i++)
	{
		int t;
		if (i < 0)
			continue;
		t = rgd3_console.times[i % RGD3_CON_NUM_TIMES];
		if (t == 0)
			continue;
		if ((float)(now - t) > cv_con_notifyTime.float_value * 1000.0f)
			continue;
		rgd3_console_draw_line(rgd3_console.text + (i % rgd3_console.total_lines) * rgd3_console.line_width, v,
			rgd3_console_visible_columns(vw));
		v += RGD3_CON_CH;
	}
}

/* the edit line after a ] prompt */
static void rgd3_console_draw_input(float vw)
{
	static const float highlight[4] = { 0.8f, 0.2f, 0.2f, 0.45f };
	char str[RGD3_MAX_EDIT_LINE];
	const float* color = rgd3_console_color(RGD3_COLOR_INDEX(RGD3_C_COLOR_WHITE));
	Rgd3EditField* f = &rgd3_console.field;
	float y = rgd3_console.vislines - RGD3_CON_CH * 2.0f;
	float x0 = RGD3_CON_SAFE_LEFT + 2.0f * RGD3_CON_CW;
	int ac_len = rgd3_editfield_auto_complete_length(f);
	int cursor_col, col, i, fit;

	/* the field scrolls inside the columns that fit on screen, so the
	   cursor stays visible in narrow windows */
	fit = (int)((vw - x0 - RGD3_CON_CW) / RGD3_CON_CW);
	if (fit < 1)
		fit = 1;
	if (fit > rgd3_console.line_width)
		fit = rgd3_console.line_width;
	if (fit != f->width_in_chars)
		rgd3_editfield_set_width_in_chars(f, fit);

	if (ac_len)
	{
		int rest = (int)strlen(rgd3_editfield_buffer(f)) - ac_len;
		if (rest > 0)
			rgd3_overlay_fill(x0 + (float)ac_len * RGD3_CON_CW, y - 1.0f, (float)rest * RGD3_CON_CW, RGD3_CON_CH - 1.0f, highlight);
	}

	rgd3_overlay_char(RGD3_CON_SAFE_LEFT + RGD3_CON_CW, y, ']', rgd3_console_color(RGD3_COLOR_INDEX(RGD3_C_COLOR_CYAN)));

	/* DrawSmallStringExt(..., colorWhite, false): colour escapes apply */
	cursor_col = rgd3_editfield_visible(f, str, (int)sizeof(str));
	for (i = 0, col = 0; str[i]; i++)
	{
		if (rgd3_str_is_color(str + i))
		{
			color = rgd3_console_color(RGD3_COLOR_INDEX(str[i + 1]));
			i++;
			continue;
		}
		rgd3_overlay_char(x0 + (float)col * RGD3_CON_CW, y, (unsigned char)str[i], color);
		col++;
	}

	/* the blinking cursor: an underline inserting, a block overstriking */
	if (cursor_col < 0 || ((rgd3_console.frame_number >> 4) & 1))
		return;
	if (rgd3_editfield_overstrike())
	{
		static const float block[4] = { 1.0f, 1.0f, 1.0f, 0.5f };
		rgd3_overlay_fill(x0 + (float)cursor_col * RGD3_CON_CW, y, RGD3_CON_CW - 1.0f, (float)RGD3_OVERLAY_GLYPH_HEIGHT, block);
	}
	else
	{
		rgd3_overlay_fill(x0 + (float)cursor_col * RGD3_CON_CW, y + (float)RGD3_OVERLAY_GLYPH_HEIGHT, RGD3_CON_CW - 1.0f, 1.0f,
			rgd3_console_color(RGD3_COLOR_INDEX(RGD3_C_COLOR_WHITE)));
	}
}

/* the console with its translucent background */
static void rgd3_console_draw_solid(float frac, float vw, float vh)
{
	static const float background[4] = { 0.0f, 0.0f, 0.0f, 0.75f };
	const float* cyan = rgd3_console_color(RGD3_COLOR_INDEX(RGD3_C_COLOR_CYAN));
	const char* version = RGD3_ENGINE_NAME " " RGD3_ENGINE_VERSION;
	float lines = floorf(vh * frac);
	float y;
	int rows, row, i, x, len, cols;

	if (lines <= 0.0f)
		return;
	if (lines > vh)
		lines = vh;

	/* the background, and a cyan line under it */
	y = frac * vh - 2.0f;
	if (y < 1.0f)
		y = 0.0f;
	else
		rgd3_overlay_fill(0.0f, 0.0f, vw, y, background);
	rgd3_overlay_fill(0.0f, y, vw, 2.0f, cyan);

	/* the version string */
	len = (int)strlen(version);
	for (x = 0; x < len; x++)
		rgd3_overlay_char(vw - RGD3_CON_SAFE_LEFT - (float)(len - x) * RGD3_CON_CW, lines - (RGD3_CON_CH + RGD3_CON_CH / 4.0f),
			(unsigned char)version[x], cyan);

	/* the text, from the bottom up */
	rgd3_console.vislines = lines;
	rows = (int)((lines - RGD3_CON_CH) / RGD3_CON_CH);
	y = lines - RGD3_CON_CH * 3.0f;
	cols = rgd3_console_visible_columns(vw);

	rgd3_console_lock();
	if (rgd3_console.display != rgd3_console.current)
	{
		/* arrows show the buffer is scrolled back */
		for (x = 0; x < rgd3_console.line_width && x < cols; x += 4)
			rgd3_overlay_char(RGD3_CON_SAFE_LEFT + (float)(x + 1) * RGD3_CON_CW, y, '^', cyan);
		y -= RGD3_CON_CH;
		rows--;
	}
	row = rgd3_console.display;
	if (rgd3_console.x == 0 && rgd3_console.display == rgd3_console.current)
		row--; /* the current line is still empty */
	for (i = 0; i < rows && y + RGD3_CON_CH > 0.0f; i++, y -= RGD3_CON_CH, row--)
	{
		if (row < 0)
			break;
		if (rgd3_console.current - row >= rgd3_console.total_lines)
			continue; /* past the scrollback wrap point */
		rgd3_console_draw_line(rgd3_console.text + (row % rgd3_console.total_lines) * rgd3_console.line_width, y, cols);
	}
	rgd3_console_unlock();

	/* the input prompt, the text and the cursor */
	rgd3_console_draw_input(vw);
}

void rgd3_console_draw(int force_full_screen)
{
	float vw = 0.0f, vh = 0.0f;
	if (force_full_screen)
	{
		/* forced full screen (disconnect): closed when we go back, but
		   catching keys meanwhile */
		rgd3_console_close();
		rgd3_console_set_key_catching(1);
	}
	rgd3_console_scroll();
	rgd3_console_update_display_fraction();
	rgd3_console.frame_number++;

	rgd3_overlay_virtual_size(&vw, &vh);
	if (vw <= 0.0f || vh <= 0.0f)
		return;
	if (force_full_screen)
		rgd3_console_draw_solid(1.0f, vw, vh);
	else if (rgd3_console.display_frac > 0.0f)
		rgd3_console_draw_solid(rgd3_console.display_frac, vw, vh);
	else
	{
		rgd3_console_lock();
		rgd3_console_draw_notify(vw);
		rgd3_console_unlock();
	}
}

/* ---------------------------------------------------------------------------
   Commands
--------------------------------------------------------------------------- */

static void rgd3_console_cmd_clear(const Rgd3CmdArgs* args)
{
	RG_UNUSED(args);
	rgd3_console_clear();
}

static void rgd3_console_cmd_dump(const Rgd3CmdArgs* args)
{
	char file_name[RGD3_MAX_OSPATH];
	if (rgd3_cmdargs_argc(args) != 2)
	{
		rgd3_printf("usage: conDump <filename>\n");
		return;
	}
	rgd3_str_copynz(file_name, rgd3_cmdargs_argv(args, 1), sizeof(file_name));
	rgd3_str_default_file_extension(file_name, sizeof(file_name), ".txt");
	rgd3_printf("Dumped console text to %s.\n", file_name);
	rgd3_console_dump(file_name);
}

static void rgd3_console_cmd_toggle(const Rgd3CmdArgs* args)
{
	RG_UNUSED(args);
	if (rgd3_console.key_catching)
	{
		rgd3_console_close();
		return;
	}
	rgd3_editfield_clear(&rgd3_console.field);
	rgd3_console_set_key_catching(1);
	rgd3_console_set_display_fraction(0.5f);
}

static void rgd3_console_cmd_history(const Rgd3CmdArgs* args)
{
	Rgd3ConsoleHistory* h = &rgd3_console.history;
	int i;
	RG_UNUSED(args);
	for (i = h->num_history - RGD3_CON_COMMAND_HISTORY; i < h->num_history; i++)
	{
		if (i < 0)
			continue;
		rgd3_printf("%c%c%c%4i: %s\n", i == h->up_point ? 'U' : ' ', i == h->down_point ? 'D' : ' ',
			i == h->return_line ? 'R' : ' ', i, h->lines[i & (RGD3_CON_COMMAND_HISTORY - 1)]);
	}
}

static void rgd3_console_cmd_clear_history(const Rgd3CmdArgs* args)
{
	RG_UNUSED(args);
	rgd3_console_history_clear();
}

void rgd3_console_register(void)
{
	static Rgd3Cvar* const cvars[] = { &cv_con_speed, &cv_con_notifyTime, &cv_con_noPrint, &cv_com_allowConsole };
	rgd3_cvar_register_table(cvars, (int)RG_ARRAY_COUNT(cvars));
	rgd3_cmd_add("clear", rgd3_console_cmd_clear, RGD3_CMD_FL_SYSTEM, "clears the console", NULL);
	rgd3_cmd_add("conDump", rgd3_console_cmd_dump, RGD3_CMD_FL_SYSTEM, "dumps the console text to a file", NULL);
	rgd3_cmd_add("toggleconsole", rgd3_console_cmd_toggle, RGD3_CMD_FL_SYSTEM, "opens or closes the console", NULL);
	rgd3_cmd_add("history", rgd3_console_cmd_history, RGD3_CMD_FL_SYSTEM, "Displays the console command history", NULL);
	rgd3_cmd_add("clearHistory", rgd3_console_cmd_clear_history, RGD3_CMD_FL_SYSTEM, "Clears the console history", NULL);
}

/* ---------------------------------------------------------------------------
   stdin
--------------------------------------------------------------------------- */

static int SDLCALL rgd3_console_stdin_thread(void* user)
{
	char line[RGD3_MAX_COMMAND_STRING];
	RG_UNUSED(user);
	while (fgets(line, sizeof(line), stdin))
	{
		Rgd3SysEvent ev;
		const char* text = line;
		/* Piped input from PowerShell starts with a UTF-8 byte order mark. */
		if ((unsigned char)text[0] == 0xEF && (unsigned char)text[1] == 0xBB && (unsigned char)text[2] == 0xBF)
			text += 3;
		rgd3_str_strip_trailing_whitespace(line);
		if (!text[0])
			continue;
		memset(&ev, 0, sizeof(ev));
		ev.type = RGD3_SE_CONSOLE;
		ev.ptr = rgd3_heap_strdup(text, RGD3_TAG_EVENTS);
		ev.ptr_length = (int)strlen(text) + 1;
		rgd3_sysevents_push(&ev);
	}
	return 0;
}

void rgd3_console_start_stdin(void)
{
	if (rgd3_console.stdin_started)
		return;
	rgd3_console.stdin_started = rgd3_thread_start_detached("rgd3 stdin console", rgd3_console_stdin_thread, NULL);
}

/* ---------------------------------------------------------------------------
   Self-test (--self-test): no window, events fed straight in
--------------------------------------------------------------------------- */

static int rgd3_console_test_checks;
static int rgd3_console_test_failures;

#define RGD3_CON_CHECK(cond) rgd3_console_test_check((cond) != 0, #cond, __LINE__)

static void rgd3_console_test_check(int ok, const char* what, int line)
{
	rgd3_console_test_checks++;
	if (!ok)
	{
		rgd3_console_test_failures++;
		rgd3_printf("^1FAILED^7 console: %s (rgd3_console.c:%d)\n", what, line);
	}
}

/* as idEventLoop::ProcessEvent: key state first, then the console */
static int rgd3_console_test_key(int key, int down)
{
	Rgd3SysEvent ev = { 0 };
	ev.type = RGD3_SE_KEY;
	ev.value = key;
	ev.value2 = down;
	rgd3_keys_preliminary_key_event(key, down);
	return rgd3_console_process_event(&ev, 0);
}

static void rgd3_console_test_press(int key)
{
	rgd3_console_test_key(key, 1);
	rgd3_console_test_key(key, 0);
}

static void rgd3_console_test_ctrl(int key)
{
	rgd3_keys_preliminary_key_event(RGD3_K_LCTRL, 1);
	rgd3_console_test_press(key);
	rgd3_keys_preliminary_key_event(RGD3_K_LCTRL, 0);
}

static void rgd3_console_test_type(const char* text)
{
	for (; *text; ++text)
	{
		Rgd3SysEvent ev = { 0 };
		ev.type = RGD3_SE_CHAR;
		ev.value = (unsigned char)*text;
		rgd3_console_process_event(&ev, 0);
	}
}

static const char* rgd3_console_test_buffer(void)
{
	return rgd3_editfield_buffer(&rgd3_console.field);
}

static void rgd3_console_test_completion(const Rgd3CmdArgs* args, Rgd3ArgCompletionCallback callback)
{
	static const char* const words[] = { "alpha", "beta", "alphabet", NULL };
	rgd3_cmd_arg_completion_string(args, callback, words);
}

static void rgd3_console_test_noop(const Rgd3CmdArgs* args)
{
	RG_UNUSED(args);
}

static int rgd3_console_test_notify_lines(void)
{
	int i, n = 0, now = rgd3_time_ms();
	for (i = rgd3_console.current - RGD3_CON_NUM_TIMES + 1; i <= rgd3_console.current; i++)
	{
		int t;
		if (i < 0)
			continue;
		t = rgd3_console.times[i % RGD3_CON_NUM_TIMES];
		if (t != 0 && (float)(now - t) <= cv_con_notifyTime.float_value * 1000.0f)
			n++;
	}
	return n;
}

int rgd3_console_self_test(void)
{
	Rgd3EditField f;
	int saved_write = rgd3_console.history.write_file;
	int saved_current, saved_display, saved_overstrike = rgd3_editfield_overstrike();
	rgd3_console_test_checks = 0;
	rgd3_console_test_failures = 0;
	rgd3_console.history.write_file = 0; /* never touch the real history file */

	/* edit field on its own */
	memset(&f, 0, sizeof(f));
	rgd3_editfield_clear(&f);
	rgd3_editfield_set_width_in_chars(&f, 20);
	rgd3_editfield_char_event(&f, 'a');
	rgd3_editfield_char_event(&f, 'b');
	rgd3_editfield_char_event(&f, 'c');
	RGD3_CON_CHECK(strcmp(rgd3_editfield_buffer(&f), "abc") == 0 && rgd3_editfield_cursor(&f) == 3);
	rgd3_editfield_key_down_event(&f, RGD3_K_LEFTARROW);
	rgd3_editfield_char_event(&f, 'X');
	RGD3_CON_CHECK(strcmp(rgd3_editfield_buffer(&f), "abXc") == 0 && rgd3_editfield_cursor(&f) == 3);
	rgd3_editfield_key_down_event(&f, RGD3_K_HOME);
	RGD3_CON_CHECK(rgd3_editfield_cursor(&f) == 0);
	rgd3_editfield_key_down_event(&f, RGD3_K_DEL);
	RGD3_CON_CHECK(strcmp(rgd3_editfield_buffer(&f), "bXc") == 0);
	rgd3_editfield_key_down_event(&f, RGD3_K_END);
	RGD3_CON_CHECK(rgd3_editfield_cursor(&f) == 3);
	rgd3_editfield_char_event(&f, 'h' - 'a' + 1); /* ctrl-h backspace */
	RGD3_CON_CHECK(strcmp(rgd3_editfield_buffer(&f), "bX") == 0 && rgd3_editfield_cursor(&f) == 2);
	rgd3_editfield_char_event(&f, 'a' - 'a' + 1); /* ctrl-a home */
	RGD3_CON_CHECK(rgd3_editfield_cursor(&f) == 0);
	rgd3_editfield_key_down_event(&f, RGD3_K_INS); /* overstrike */
	rgd3_editfield_char_event(&f, 'Z');
	RGD3_CON_CHECK(strcmp(rgd3_editfield_buffer(&f), "ZX") == 0 && rgd3_editfield_cursor(&f) == 1);
	rgd3_editfield_key_down_event(&f, RGD3_K_INS);
	RGD3_CON_CHECK(!rgd3_editfield_overstrike());
	rgd3_editfield_char_event(&f, 'c' - 'a' + 1); /* ctrl-c clears */
	RGD3_CON_CHECK(rgd3_editfield_buffer(&f)[0] == 0 && rgd3_editfield_cursor(&f) == 0);
	rgd3_editfield_set_buffer(&f, "one two  three");
	rgd3_keys_preliminary_key_event(RGD3_K_LCTRL, 1);
	rgd3_editfield_key_down_event(&f, RGD3_K_LEFTARROW);
	RGD3_CON_CHECK(rgd3_editfield_cursor(&f) == 9);
	rgd3_editfield_key_down_event(&f, RGD3_K_LEFTARROW);
	RGD3_CON_CHECK(rgd3_editfield_cursor(&f) == 4);
	rgd3_editfield_key_down_event(&f, RGD3_K_RIGHTARROW);
	RGD3_CON_CHECK(rgd3_editfield_cursor(&f) == 9);
	rgd3_editfield_key_down_event(&f, RGD3_K_A); /* ctrl-a */
	RGD3_CON_CHECK(rgd3_editfield_cursor(&f) == 0);
	rgd3_editfield_key_down_event(&f, RGD3_K_E); /* ctrl-e */
	RGD3_CON_CHECK(rgd3_editfield_cursor(&f) == 14);
	rgd3_keys_preliminary_key_event(RGD3_K_LCTRL, 0);
	{
		/* scrolling keeps the cursor visible in a 20-wide field */
		char vis[RGD3_MAX_EDIT_LINE];
		int col, i;
		rgd3_editfield_clear(&f);
		for (i = 0; i < 30; i++)
			rgd3_editfield_char_event(&f, 'a' + (i % 26));
		col = rgd3_editfield_visible(&f, vis, (int)sizeof(vis));
		RGD3_CON_CHECK(col >= 0 && col < 20 && (int)strlen(vis) <= 20 && vis[col - 1] == 'd');

		/* review: the console narrowed under a scrolled line; the cursor
		   (at the end, after 'd') stays on screen */
		rgd3_editfield_set_width_in_chars(&f, 8);
		col = rgd3_editfield_visible(&f, vis, (int)sizeof(vis));
		RGD3_CON_CHECK(col > 0 && col < 8 && (int)strlen(vis) <= 8 && vis[col - 1] == 'd');

		/* a recalled line longer than the field puts the cursor at its end
		   with scroll reset to 0 (set_buffer, as history recall does) */
		rgd3_editfield_set_buffer(&f, "echo 0123456789abcdefghijXY");
		col = rgd3_editfield_visible(&f, vis, (int)sizeof(vis));
		RGD3_CON_CHECK(col > 0 && col < 8 && vis[col - 1] == 'Y');

		/* Ctrl-E (char event) used to scroll so the cursor sat one column
		   past the field */
		rgd3_editfield_set_width_in_chars(&f, 20);
		rgd3_editfield_char_event(&f, 'a' - 'a' + 1); /* ctrl-a home */
		col = rgd3_editfield_visible(&f, vis, (int)sizeof(vis));
		RGD3_CON_CHECK(col == 0 && vis[0] == 'e');
		rgd3_editfield_char_event(&f, 'e' - 'a' + 1); /* ctrl-e end */
		col = rgd3_editfield_visible(&f, vis, (int)sizeof(vis));
		RGD3_CON_CHECK(col > 0 && col < 20 && vis[col - 1] == 'Y');

		/* moving left past the scroll point scrolls back */
		for (i = 0; i < 25; i++)
			rgd3_editfield_key_down_event(&f, RGD3_K_LEFTARROW);
		col = rgd3_editfield_visible(&f, vis, (int)sizeof(vis));
		RGD3_CON_CHECK(col == 0 && vis[0] == f.buffer[f.cursor]);
	}

	/* the console key opens and closes; the key itself never types */
	rgd3_console_close();
	RGD3_CON_CHECK(!rgd3_console_test_key(RGD3_K_A, 1));
	rgd3_console_test_key(RGD3_K_A, 0);
	RGD3_CON_CHECK(rgd3_console_test_key(RGD3_K_GRAVE, 1) && rgd3_console_active());
	rgd3_console_test_key(RGD3_K_GRAVE, 0);
	RGD3_CON_CHECK(rgd3_console.final_frac == 0.5f);
	rgd3_console_test_type("`echo hi~");
	RGD3_CON_CHECK(strcmp(rgd3_console_test_buffer(), "echo hi") == 0);
	rgd3_console_test_press(RGD3_K_BACKSPACE);
	rgd3_console_test_press(RGD3_K_LEFTARROW);
	rgd3_console_test_type("X");
	RGD3_CON_CHECK(strcmp(rgd3_console_test_buffer(), "echo Xh") == 0);
	rgd3_console_test_ctrl(RGD3_K_C);
	RGD3_CON_CHECK(rgd3_console_test_buffer()[0] == 0);

	/* history: Enter adds, repeats are ignored, up/down walk it */
	rgd3_console_history_clear();
	rgd3_console_test_type("echo first");
	rgd3_console_test_press(RGD3_K_ENTER);
	RGD3_CON_CHECK(rgd3_console_test_buffer()[0] == 0);
	rgd3_console_test_type("echo second");
	rgd3_console_test_press(RGD3_K_ENTER);
	rgd3_console_test_type("echo second");
	rgd3_console_test_press(RGD3_K_ENTER);
	rgd3_cmd_execute_buffer();
	RGD3_CON_CHECK(rgd3_console.history.num_history == 2);
	rgd3_console_test_press(RGD3_K_UPARROW);
	RGD3_CON_CHECK(strcmp(rgd3_console_test_buffer(), "echo second") == 0);
	rgd3_console_test_press(RGD3_K_UPARROW);
	RGD3_CON_CHECK(strcmp(rgd3_console_test_buffer(), "echo first") == 0);
	rgd3_console_test_press(RGD3_K_UPARROW); /* nothing older: unchanged */
	RGD3_CON_CHECK(strcmp(rgd3_console_test_buffer(), "echo first") == 0);
	rgd3_console_test_press(RGD3_K_DOWNARROW);
	RGD3_CON_CHECK(strcmp(rgd3_console_test_buffer(), "echo second") == 0);
	RGD3_CON_CHECK(rgd3_console_history_retrieve(0)[0] == 0);
	rgd3_console_history_add("echo second", 0); /* retrieved line repeated: up gets it again */
	RGD3_CON_CHECK(rgd3_console.history.num_history == 2 && rgd3_console.history.up_point == 1);
	rgd3_console_history_add("   ", 0);
	RGD3_CON_CHECK(rgd3_console.history.num_history == 2);
	rgd3_console_test_ctrl(RGD3_K_C);

	/* completion: a unique command, then its (absent) arguments */
	rgd3_console_test_type("listCv");
	rgd3_console_test_press(RGD3_K_TAB);
	RGD3_CON_CHECK(strcmp(rgd3_console_test_buffer(), "listCvars ") == 0);
	rgd3_console_test_ctrl(RGD3_K_C);
	/* a bool cvar: the name, then its values (0/1) with the cycle */
	rgd3_console_test_type("con_nop");
	rgd3_console_test_press(RGD3_K_TAB);
	RGD3_CON_CHECK(strcmp(rgd3_console_test_buffer(), "con_noPrint ") == 0);
	rgd3_console_test_press(RGD3_K_TAB);
	RGD3_CON_CHECK(strcmp(rgd3_console_test_buffer(), "con_noPrint ") == 0 && rgd3_console.field.auto_complete.match_count == 2);
	rgd3_console_test_press(RGD3_K_TAB);
	RGD3_CON_CHECK(strcmp(rgd3_console_test_buffer(), "con_noPrint 1") == 0);
	rgd3_console_test_press(RGD3_K_TAB);
	RGD3_CON_CHECK(strcmp(rgd3_console_test_buffer(), "con_noPrint 0") == 0);
	rgd3_console_test_press(RGD3_K_DEL); /* clears the completion */
	RGD3_CON_CHECK(rgd3_editfield_auto_complete_length(&rgd3_console.field) == 0);
	rgd3_console_test_ctrl(RGD3_K_C);
	/* several commands share a prefix: the common part, then cycling */
	rgd3_console_test_type("listSy");
	rgd3_console_test_press(RGD3_K_TAB);
	RGD3_CON_CHECK(strcmp(rgd3_console_test_buffer(), "listSystemCmds ") == 0);
	rgd3_console_test_ctrl(RGD3_K_C);
	rgd3_console_test_type("con_");
	rgd3_console_test_press(RGD3_K_TAB);
	RGD3_CON_CHECK(strcmp(rgd3_console_test_buffer(), "con_") == 0 && rgd3_console.field.auto_complete.match_count >= 3);
	rgd3_console_test_press(RGD3_K_TAB);
	RGD3_CON_CHECK(rgd3_str_icmpn(rgd3_console_test_buffer(), "con", 3) == 0 && strlen(rgd3_console_test_buffer()) > 4);
	rgd3_console_test_ctrl(RGD3_K_C);
	/* an argument through a registered completion callback */
	rgd3_cmd_add("rgd3ConTest", rgd3_console_test_noop, RGD3_CMD_FL_SYSTEM, "console self-test", rgd3_console_test_completion);
	rgd3_console_test_type("rgd3ConTest al");
	rgd3_console_test_press(RGD3_K_TAB);
	RGD3_CON_CHECK(strcmp(rgd3_console_test_buffer(), "rgd3ConTest alpha") == 0);
	rgd3_console_test_press(RGD3_K_TAB);
	RGD3_CON_CHECK(strcmp(rgd3_console_test_buffer(), "rgd3ConTest alphabet") == 0);
	rgd3_console_test_ctrl(RGD3_K_C);
	rgd3_console_test_type("rgd3ConTest b");
	rgd3_console_test_press(RGD3_K_TAB);
	RGD3_CON_CHECK(strcmp(rgd3_console_test_buffer(), "rgd3ConTest beta") == 0);
	rgd3_console_test_ctrl(RGD3_K_C);
	rgd3_cmd_remove("rgd3ConTest");
	/* file name arguments: exec completes config files from the data */
	rgd3_console_test_type("exec defau");
	rgd3_console_test_press(RGD3_K_TAB);
	RGD3_CON_CHECK(strcmp(rgd3_console_test_buffer(), "exec default.cfg") == 0);
	rgd3_console_test_ctrl(RGD3_K_C);

	/* printing: colours per cell, word wrap, scrollback */
	if (rgd3_console.x)
		rgd3_console_print("\n");
	rgd3_console_lock();
	saved_current = rgd3_console.current;
	rgd3_console_unlock();
	rgd3_console_print("^1red ^2green\n");
	{
		const u16* line = rgd3_console.text + (saved_current % rgd3_console.total_lines) * rgd3_console.line_width;
		RGD3_CON_CHECK((line[0] & 0xff) == 'r' && (line[0] >> 8) == 1 && (line[4] & 0xff) == 'g' && (line[4] >> 8) == 2);
		RGD3_CON_CHECK(rgd3_console.current == saved_current + 1 && rgd3_console.x == 0);
	}
	{
		char long_line[RGD3_CON_LINE_WIDTH + 64];
		int i;
		for (i = 0; i < RGD3_CON_LINE_WIDTH - 3; i++)
			long_line[i] = 'x';
		rg_snprintf(long_line + i, sizeof(long_line) - (size_t)i, " wrapped\n");
		saved_current = rgd3_console.current;
		rgd3_console_print(long_line);
		/* the second word moved to its own line */
		RGD3_CON_CHECK(rgd3_console.current == saved_current + 2);
		RGD3_CON_CHECK((rgd3_console.text[((saved_current + 1) % rgd3_console.total_lines) * rgd3_console.line_width] & 0xff) == 'w');
	}
	saved_display = rgd3_console.display;
	rgd3_console_test_press(RGD3_K_PGUP);
	RGD3_CON_CHECK(rgd3_console.display == saved_display - 2);
	rgd3_console_test_press(RGD3_K_MWHEELDOWN);
	RGD3_CON_CHECK(rgd3_console.display == saved_display);
	rgd3_console_test_press(RGD3_K_MWHEELDOWN); /* clamps at the bottom */
	RGD3_CON_CHECK(rgd3_console.display == rgd3_console.current);
	rgd3_keys_preliminary_key_event(RGD3_K_LCTRL, 1);
	rgd3_console_test_press(RGD3_K_HOME);
	RGD3_CON_CHECK(rgd3_console.display == 0);
	rgd3_console_test_press(RGD3_K_END);
	RGD3_CON_CHECK(rgd3_console.display == rgd3_console.current);
	rgd3_keys_preliminary_key_event(RGD3_K_LCTRL, 0);

	/* notify lines expire after con_notifyTime and Close clears them */
	rgd3_console_print("notify\n");
	RGD3_CON_CHECK(rgd3_console_test_notify_lines() >= 1);
	{
		int i, past = rgd3_time_ms() - (int)(cv_con_notifyTime.float_value * 1000.0f) - 1;
		for (i = 0; i < RGD3_CON_NUM_TIMES; i++)
			if (rgd3_console.times[i])
				rgd3_console.times[i] = past ? past : -1;
		RGD3_CON_CHECK(rgd3_console_test_notify_lines() == 0);
	}
	rgd3_console_print("notify again\n");
	RGD3_CON_CHECK(rgd3_console_test_notify_lines() >= 1);

	/* closed again: keys pass through, the field was cleared on open */
	RGD3_CON_CHECK(rgd3_console_test_key(RGD3_K_GRAVE, 1) && !rgd3_console_active());
	rgd3_console_test_key(RGD3_K_GRAVE, 0);
	RGD3_CON_CHECK(rgd3_console_test_notify_lines() == 0);
	RGD3_CON_CHECK(!rgd3_console_test_key(RGD3_K_ENTER, 1));
	rgd3_console_test_key(RGD3_K_ENTER, 0);
	RGD3_CON_CHECK(rgd3_console.display_frac == 0.0f);

	rgd3_console_history_clear();
	rgd3_editfield_clear(&rgd3_console.field);
	rgd3_editfield_set_overstrike(saved_overstrike);
	rgd3_keys_clear_states();
	rgd3_console.history.write_file = saved_write;
	rgd3_printf("console    %4d checks, %d failures\n", rgd3_console_test_checks, rgd3_console_test_failures);
	return rgd3_console_test_failures == 0;
}
