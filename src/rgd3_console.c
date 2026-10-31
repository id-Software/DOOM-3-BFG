/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_console.c - see rgd3_console.h. The stdin thread replaces the Win32
   dedicated console window (neo/sys/win32/win_syscon.cpp). */

typedef struct Rgd3ConsoleState
{
	char lines[RGD3_CONSOLE_LINES][RGD3_CONSOLE_LINE_CHARS];
	int current;
	int column;
	int total;
	int stdin_started;
} Rgd3ConsoleState;

static Rgd3ConsoleState rgd3_console;

void rgd3_console_init(void)
{
	memset(&rgd3_console, 0, sizeof(rgd3_console));
}

void rgd3_console_shutdown(void)
{
}

/* Called with the log lock held. Splits text into lines and keeps colour
   escapes for the future in-window renderer. */
void rgd3_console_print(const char* text)
{
	for (; *text; ++text)
	{
		char* line = rgd3_console.lines[rgd3_console.current];
		if (*text == '\n' || rgd3_console.column >= RGD3_CONSOLE_LINE_CHARS - 1)
		{
			line[rgd3_console.column] = 0;
			rgd3_console.current = (rgd3_console.current + 1) % RGD3_CONSOLE_LINES;
			rgd3_console.column = 0;
			rgd3_console.lines[rgd3_console.current][0] = 0;
			++rgd3_console.total;
			if (*text == '\n')
				continue;
			line = rgd3_console.lines[rgd3_console.current];
		}
		line[rgd3_console.column++] = *text;
		line[rgd3_console.column] = 0;
	}
}

int rgd3_console_num_lines(void)
{
	return rgd3_console.total < RGD3_CONSOLE_LINES ? rgd3_console.total : RGD3_CONSOLE_LINES;
}

const char* rgd3_console_line(int index_from_newest)
{
	int idx = rgd3_console.current - 1 - index_from_newest;
	while (idx < 0)
		idx += RGD3_CONSOLE_LINES;
	return rgd3_console.lines[idx % RGD3_CONSOLE_LINES];
}

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
