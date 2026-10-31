/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_cmd.c - buffer splitting (';' outside quotes, newlines), 'wait',
   exec/vstr/echo/listCmds follow neo/framework/CmdSystem.cpp. */

#define RGD3_CMD_BUFFER_SIZE 0x10000

typedef struct Rgd3CmdDef
{
	char* name;
	Rgd3CmdFn fn;
	int flags;
	char* description;
	Rgd3ArgCompletion completion;
	struct Rgd3CmdDef* next;
} Rgd3CmdDef;

typedef struct Rgd3CmdState
{
	Rgd3CmdDef* commands;
	int wait;
	int text_length;
	char text[RGD3_CMD_BUFFER_SIZE];
} Rgd3CmdState;

static Rgd3CmdState rgd3_cmd;

void rgd3_cmd_init(void)
{
	memset(&rgd3_cmd, 0, sizeof(rgd3_cmd));
}

void rgd3_cmd_shutdown(void)
{
	Rgd3CmdDef* cmd = rgd3_cmd.commands;
	while (cmd)
	{
		Rgd3CmdDef* next = cmd->next;
		rgd3_heap_free(cmd->name);
		rgd3_heap_free(cmd->description);
		rgd3_heap_free(cmd);
		cmd = next;
	}
	memset(&rgd3_cmd, 0, sizeof(rgd3_cmd));
}

static Rgd3CmdDef* rgd3_cmd_find(const char* name)
{
	Rgd3CmdDef* cmd;
	for (cmd = rgd3_cmd.commands; cmd; cmd = cmd->next)
		if (rgd3_str_icmp(cmd->name, name) == 0)
			return cmd;
	return NULL;
}

int rgd3_cmd_exists(const char* name)
{
	return rgd3_cmd_find(name) != NULL;
}

void rgd3_cmd_add(const char* name, Rgd3CmdFn fn, int flags, const char* description, Rgd3ArgCompletion completion)
{
	Rgd3CmdDef* cmd;
	if (rgd3_cvar_find(name))
	{
		rgd3_printf("rgd3_cmd_add: %s already defined as a var\n", name);
		return;
	}
	if (rgd3_cmd_find(name))
	{
		rgd3_printf("rgd3_cmd_add: %s already defined\n", name);
		return;
	}
	cmd = (Rgd3CmdDef*)rgd3_heap_calloc(sizeof(Rgd3CmdDef), RGD3_TAG_CMD);
	cmd->name = rgd3_heap_strdup(name, RGD3_TAG_CMD);
	cmd->fn = fn;
	cmd->flags = flags;
	cmd->description = rgd3_heap_strdup(description ? description : "", RGD3_TAG_CMD);
	cmd->completion = completion;
	cmd->next = rgd3_cmd.commands;
	rgd3_cmd.commands = cmd;
}

void rgd3_cmd_remove(const char* name)
{
	Rgd3CmdDef** link = &rgd3_cmd.commands;
	while (*link)
	{
		Rgd3CmdDef* cmd = *link;
		if (rgd3_str_icmp(cmd->name, name) == 0)
		{
			*link = cmd->next;
			rgd3_heap_free(cmd->name);
			rgd3_heap_free(cmd->description);
			rgd3_heap_free(cmd);
			return;
		}
		link = &cmd->next;
	}
}

static void rgd3_cmd_append_text(const char* text)
{
	int len = (int)strlen(text);
	if (rgd3_cmd.text_length + len > RGD3_CMD_BUFFER_SIZE)
	{
		rgd3_printf("rgd3_cmd_append_text: buffer overflow\n");
		return;
	}
	memcpy(rgd3_cmd.text + rgd3_cmd.text_length, text, (size_t)len);
	rgd3_cmd.text_length += len;
}

/* Inserts text plus a newline at the front of the buffer. */
static void rgd3_cmd_insert_text(const char* text)
{
	int len = (int)strlen(text) + 1;
	if (rgd3_cmd.text_length + len > RGD3_CMD_BUFFER_SIZE)
	{
		rgd3_printf("rgd3_cmd_insert_text: buffer overflow\n");
		return;
	}
	memmove(rgd3_cmd.text + len, rgd3_cmd.text, (size_t)rgd3_cmd.text_length);
	memcpy(rgd3_cmd.text, text, (size_t)len - 1);
	rgd3_cmd.text[len - 1] = '\n';
	rgd3_cmd.text_length += len;
}

void rgd3_cmd_execute_tokenized(const Rgd3CmdArgs* args)
{
	Rgd3CmdDef* cmd;
	if (!rgd3_cmdargs_argc(args))
		return;
	cmd = rgd3_cmd_find(rgd3_cmdargs_argv(args, 0));
	if (cmd)
	{
		if (cmd->fn)
			cmd->fn(args);
		return;
	}
	if (rgd3_cvar_command(args))
		return;
	rgd3_printf("Unknown command '%s'\n", rgd3_cmdargs_argv(args, 0));
}

void rgd3_cmd_execute_string(const char* text)
{
	Rgd3CmdArgs args;
	rgd3_cmdargs_tokenize(&args, text, 0);
	rgd3_cmd_execute_tokenized(&args);
}

void rgd3_cmd_buffer_text(Rgd3CmdExec exec, const char* text)
{
	switch (exec)
	{
		case RGD3_CMD_EXEC_NOW:
			rgd3_cmd_execute_string(text);
			break;
		case RGD3_CMD_EXEC_INSERT:
			rgd3_cmd_insert_text(text);
			break;
		case RGD3_CMD_EXEC_APPEND:
			rgd3_cmd_append_text(text);
			break;
	}
}

void rgd3_cmd_execute_buffer(void)
{
	char line[RGD3_MAX_COMMAND_STRING];
	while (rgd3_cmd.text_length)
	{
		int i, quotes = 0;
		if (rgd3_cmd.wait)
		{
			--rgd3_cmd.wait;
			break;
		}
		for (i = 0; i < rgd3_cmd.text_length; ++i)
		{
			char c = rgd3_cmd.text[i];
			if (c == '"')
				++quotes;
			if (!(quotes & 1) && c == ';')
				break;
			if (c == '\n' || c == '\r')
				break;
		}
		{
			int copy = i < (int)sizeof(line) - 1 ? i : (int)sizeof(line) - 1;
			memcpy(line, rgd3_cmd.text, (size_t)copy);
			line[copy] = 0;
		}
		/* Remove the line before running it: the command may insert text. */
		if (i == rgd3_cmd.text_length)
		{
			rgd3_cmd.text_length = 0;
		}
		else
		{
			++i;
			rgd3_cmd.text_length -= i;
			memmove(rgd3_cmd.text, rgd3_cmd.text + i, (size_t)rgd3_cmd.text_length);
		}
		rgd3_cmd_execute_string(line);
	}
}

void rgd3_cmd_exec_file(const char* name, int quiet)
{
	char path[RGD3_MAX_OSPATH];
	void* buffer = NULL;
	int len;
	rgd3_str_copynz(path, name, sizeof(path));
	rgd3_str_default_file_extension(path, sizeof(path), ".cfg");
	len = rgd3_fs_read_file(path, &buffer, NULL);
	if (len < 0 || !buffer)
	{
		if (!quiet)
			rgd3_printf("couldn't exec %s\n", path);
		return;
	}
	rgd3_printf("execing %s\n", path);
	rgd3_cmd_insert_text((const char*)buffer);
	rgd3_fs_free_file(buffer);
}

static void rgd3_cmd_cmd_exec(const Rgd3CmdArgs* args)
{
	if (rgd3_cmdargs_argc(args) != 2)
	{
		rgd3_printf("exec <filename> : execute a script file\n");
		return;
	}
	rgd3_cmd_exec_file(rgd3_cmdargs_argv(args, 1), 0);
}

static void rgd3_cmd_cmd_vstr(const Rgd3CmdArgs* args)
{
	if (rgd3_cmdargs_argc(args) != 2)
	{
		rgd3_printf("vstr <variablename> : execute a variable command\n");
		return;
	}
	rgd3_cmd_insert_text(rgd3_cvar_get_string(rgd3_cmdargs_argv(args, 1)));
}

static void rgd3_cmd_cmd_echo(const Rgd3CmdArgs* args)
{
	rgd3_printf("%s\n", rgd3_cmdargs_args(args, 1, -1, 0));
}

static void rgd3_cmd_cmd_wait(const Rgd3CmdArgs* args)
{
	rgd3_cmd.wait = rgd3_cmdargs_argc(args) == 2 ? atoi(rgd3_cmdargs_argv(args, 1)) : 1;
}

static int rgd3_cmd_sort(const void* a, const void* b)
{
	return rgd3_str_icmp((*(const Rgd3CmdDef* const*)a)->name, (*(const Rgd3CmdDef* const*)b)->name);
}

static void rgd3_cmd_list(const Rgd3CmdArgs* args, int flags)
{
	char match[RGD3_MAX_COMMAND_STRING];
	const char* filter = rgd3_cmdargs_match(args, 1, match, sizeof(match));
	Rgd3CmdDef* cmd;
	Rgd3CmdDef** list;
	int count = 0, i;
	for (cmd = rgd3_cmd.commands; cmd; cmd = cmd->next)
		++count;
	list = (Rgd3CmdDef**)rgd3_heap_alloc(sizeof(Rgd3CmdDef*) * (size_t)(count + 1), RGD3_TAG_TEMP);
	count = 0;
	for (cmd = rgd3_cmd.commands; cmd; cmd = cmd->next)
		if ((cmd->flags & flags) && (!filter || rgd3_str_filter(filter, cmd->name, 0)))
			list[count++] = cmd;
	qsort(list, (size_t)count, sizeof(Rgd3CmdDef*), rgd3_cmd_sort);
	for (i = 0; i < count; ++i)
		rgd3_printf("  %-21s %s\n", list[i]->name, list[i]->description);
	rgd3_printf("%d commands\n", count);
	rgd3_heap_free(list);
}

static void rgd3_cmd_cmd_list_all(const Rgd3CmdArgs* args)
{
	rgd3_cmd_list(args, RGD3_CMD_FL_ALL);
}

static void rgd3_cmd_cmd_list_system(const Rgd3CmdArgs* args)
{
	rgd3_cmd_list(args, RGD3_CMD_FL_SYSTEM);
}

static void rgd3_cmd_cmd_list_renderer(const Rgd3CmdArgs* args)
{
	rgd3_cmd_list(args, RGD3_CMD_FL_RENDERER);
}

static void rgd3_cmd_cmd_list_sound(const Rgd3CmdArgs* args)
{
	rgd3_cmd_list(args, RGD3_CMD_FL_SOUND);
}

static void rgd3_cmd_cmd_list_game(const Rgd3CmdArgs* args)
{
	rgd3_cmd_list(args, RGD3_CMD_FL_GAME);
}

void rgd3_cmd_register_builtins(void)
{
	rgd3_cmd_add("listCmds", rgd3_cmd_cmd_list_all, RGD3_CMD_FL_SYSTEM, "lists commands", NULL);
	rgd3_cmd_add("listSystemCmds", rgd3_cmd_cmd_list_system, RGD3_CMD_FL_SYSTEM, "lists system commands", NULL);
	rgd3_cmd_add("listRendererCmds", rgd3_cmd_cmd_list_renderer, RGD3_CMD_FL_SYSTEM, "lists renderer commands", NULL);
	rgd3_cmd_add("listSoundCmds", rgd3_cmd_cmd_list_sound, RGD3_CMD_FL_SYSTEM, "lists sound commands", NULL);
	rgd3_cmd_add("listGameCmds", rgd3_cmd_cmd_list_game, RGD3_CMD_FL_SYSTEM, "lists game commands", NULL);
	rgd3_cmd_add("exec", rgd3_cmd_cmd_exec, RGD3_CMD_FL_SYSTEM, "executes a config file", NULL);
	rgd3_cmd_add("vstr", rgd3_cmd_cmd_vstr, RGD3_CMD_FL_SYSTEM, "inserts the current value of a cvar as command text", NULL);
	rgd3_cmd_add("echo", rgd3_cmd_cmd_echo, RGD3_CMD_FL_SYSTEM, "prints text", NULL);
	rgd3_cmd_add("wait", rgd3_cmd_cmd_wait, RGD3_CMD_FL_SYSTEM, "delays remaining buffered commands one or more frames", NULL);
}
