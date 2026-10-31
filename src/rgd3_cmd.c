/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_cmd.c - buffer splitting (';' outside quotes, newlines), 'wait',
   exec/vstr/echo/listCmds and Tab completion (CommandCompletion,
   ArgCompletion, ArgCompletion_FolderExtension with its per-argument cache,
   ArgCompletion_DeclName, the CmdSystem.h ArgCompletion_* helpers) follow
   neo/framework/CmdSystem.cpp/.h. */

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
	/* ArgCompletion_FolderExtension cache: the "cmd arg" string it was built
	   for and the heap-allocated completion lines */
	char completion_string[RGD3_MAX_COMMAND_STRING];
	char** completion_parms;
	int num_completion_parms;
	int max_completion_parms;
} Rgd3CmdState;

static Rgd3CmdState rgd3_cmd;

void rgd3_cmd_init(void)
{
	memset(&rgd3_cmd, 0, sizeof(rgd3_cmd));
	rgd3_str_copynz(rgd3_cmd.completion_string, "*", sizeof(rgd3_cmd.completion_string));
}

static void rgd3_cmd_clear_completion_parms(void)
{
	int i;
	for (i = 0; i < rgd3_cmd.num_completion_parms; ++i)
		rgd3_heap_free(rgd3_cmd.completion_parms[i]);
	rgd3_heap_free(rgd3_cmd.completion_parms);
	rgd3_cmd.completion_parms = NULL;
	rgd3_cmd.num_completion_parms = 0;
	rgd3_cmd.max_completion_parms = 0;
}

void rgd3_cmd_shutdown(void)
{
	Rgd3CmdDef* cmd = rgd3_cmd.commands;
	rgd3_cmd_clear_completion_parms();
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

/* "wait N" holds the buffer for N frames. The original decrements the
   counter on every ExecuteCommandBuffer call, which the event loop makes
   once per event, so with input arriving a wait ends early; rgd3 counts
   frames, as the command's description says. */
void rgd3_cmd_begin_frame(void)
{
	if (rgd3_cmd.wait > 0)
		--rgd3_cmd.wait;
}

void rgd3_cmd_execute_buffer(void)
{
	char line[RGD3_MAX_COMMAND_STRING];
	while (rgd3_cmd.text_length)
	{
		int i, quotes = 0;
		if (rgd3_cmd.wait)
			break; /* counted down per frame by rgd3_cmd_begin_frame */
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

/* ---------------------------------------------------------------------------
   Completion (idCmdSystemLocal::CommandCompletion / ArgCompletion /
   ArgCompletion_FolderExtension / ArgCompletion_DeclName, CmdSystem.h
   inline ArgCompletion_* helpers)
--------------------------------------------------------------------------- */

void rgd3_cmd_command_completion(Rgd3ArgCompletionCallback callback)
{
	Rgd3CmdDef* cmd;
	for (cmd = rgd3_cmd.commands; cmd; cmd = cmd->next)
		callback(cmd->name);
}

void rgd3_cmd_arg_completion(const char* cmd_string, Rgd3ArgCompletionCallback callback)
{
	Rgd3CmdArgs args;
	Rgd3CmdDef* cmd;
	rgd3_cmdargs_tokenize(&args, cmd_string, 0);
	for (cmd = rgd3_cmd.commands; cmd; cmd = cmd->next)
	{
		if (!cmd->completion)
			continue;
		if (rgd3_str_icmp(rgd3_cmdargs_argv(&args, 0), cmd->name) == 0)
		{
			cmd->completion(&args, callback);
			break;
		}
	}
}

void rgd3_cmd_arg_completion_boolean(const Rgd3CmdArgs* args, Rgd3ArgCompletionCallback callback)
{
	callback(rgd3_va("%s 0", rgd3_cmdargs_argv(args, 0)));
	callback(rgd3_va("%s 1", rgd3_cmdargs_argv(args, 0)));
}

void rgd3_cmd_arg_completion_integer(const Rgd3CmdArgs* args, Rgd3ArgCompletionCallback callback, int min, int max)
{
	int i;
	for (i = min; i <= max; i++)
		callback(rgd3_va("%s %d", rgd3_cmdargs_argv(args, 0), i));
}

void rgd3_cmd_arg_completion_string(const Rgd3CmdArgs* args, Rgd3ArgCompletionCallback callback, const char* const* strings)
{
	int i;
	for (i = 0; strings[i]; i++)
		callback(rgd3_va("%s %s", rgd3_cmdargs_argv(args, 0), strings[i]));
}

static void rgd3_cmd_add_completion_parm(const char* text)
{
	if (rgd3_cmd.num_completion_parms == rgd3_cmd.max_completion_parms)
	{
		rgd3_cmd.max_completion_parms = rgd3_cmd.max_completion_parms ? rgd3_cmd.max_completion_parms * 2 : 64;
		rgd3_cmd.completion_parms = (char**)rgd3_heap_realloc(rgd3_cmd.completion_parms,
			sizeof(char*) * (size_t)rgd3_cmd.max_completion_parms, RGD3_TAG_CMD);
	}
	rgd3_cmd.completion_parms[rgd3_cmd.num_completion_parms++] = rgd3_heap_strdup(text, RGD3_TAG_CMD);
}

/* idStr::Strip(const char*): removes the string from the front and the end */
static void rgd3_cmd_strip_both(char* name, const char* strip)
{
	size_t n = strlen(strip), len;
	if (!n)
		return;
	while (strncmp(name, strip, n) == 0)
		memmove(name, name + n, strlen(name + n) + 1);
	len = strlen(name);
	while (len >= n && strcmp(name + len - n, strip) == 0)
	{
		len -= n;
		name[len] = 0;
	}
}

static void rgd3_cmd_add_listed(const Rgd3CmdArgs* args, Rgd3FileList* names, const char* folder, int strip_folder, int is_dir)
{
	int i;
	for (i = 0; i < names->count; i++)
	{
		char name[RGD3_MAX_OSPATH];
		rgd3_str_copynz(name, names->names[i], sizeof(name));
		rgd3_cmd_strip_both(name, strip_folder ? folder : "/");
		rgd3_cmd_add_completion_parm(rgd3_va("%s %s%s", rgd3_cmdargs_argv(args, 0), name, is_dir ? "/" : ""));
	}
}

void rgd3_cmd_arg_completion_folder_extension(const Rgd3CmdArgs* args, Rgd3ArgCompletionCallback callback,
	const char* folder, int strip_folder, ...)
{
	char string[RGD3_MAX_COMMAND_STRING];
	int i;
	rg_snprintf(string, sizeof(string), "%s %s", rgd3_cmdargs_argv(args, 0), rgd3_cmdargs_argv(args, 1));
	if (rgd3_str_icmp(string, rgd3_cmd.completion_string) != 0)
	{
		char path[RGD3_MAX_OSPATH];
		const char* parm = rgd3_cmdargs_argv(args, 1);
		const char* slash = strrchr(parm, '/');
		const char* extension;
		Rgd3FileList* names;
		va_list arg_ptr;
		size_t len;

		rgd3_str_copynz(rgd3_cmd.completion_string, string, sizeof(rgd3_cmd.completion_string));
		rgd3_cmd_clear_completion_parms();

		/* ExtractFilePath keeps the trailing slash */
		path[0] = 0;
		if (slash)
		{
			size_t n = (size_t)(slash - parm) + 1;
			rgd3_str_copynz(path, parm, n + 1 < sizeof(path) ? n + 1 : sizeof(path));
		}
		if (strip_folder || !path[0])
		{
			char joined[RGD3_MAX_OSPATH];
			rg_snprintf(joined, sizeof(joined), "%s%s", folder, path);
			rgd3_str_copynz(path, joined, sizeof(path));
		}
		len = strlen(path);
		while (len && path[len - 1] == '/')
			path[--len] = 0;

		/* folders */
		names = rgd3_fs_list_files(path, "/", 1, 0);
		rgd3_cmd_add_listed(args, names, folder, strip_folder, 1);
		rgd3_fs_free_file_list(names);

		/* files */
		va_start(arg_ptr, strip_folder);
		for (extension = va_arg(arg_ptr, const char*); extension; extension = va_arg(arg_ptr, const char*))
		{
			names = rgd3_fs_list_files(path, extension, 1, 0);
			rgd3_cmd_add_listed(args, names, folder, strip_folder, 0);
			rgd3_fs_free_file_list(names);
		}
		va_end(arg_ptr);
	}
	for (i = 0; i < rgd3_cmd.num_completion_parms; i++)
		callback(rgd3_cmd.completion_parms[i]);
}

void rgd3_cmd_arg_completion_decl_name(const Rgd3CmdArgs* args, Rgd3ArgCompletionCallback callback, int decl_type)
{
	int i, num = rgd3_decl_num_decls((Rgd3DeclType)decl_type);
	for (i = 0; i < num; i++)
	{
		const void* decl = rgd3_decl_by_index((Rgd3DeclType)decl_type, i, 0);
		if (decl)
			callback(rgd3_va("%s %s", rgd3_cmdargs_argv(args, 0), rgd3_decl_name(decl)));
	}
}

void rgd3_cmd_arg_completion_file_name(const Rgd3CmdArgs* args, Rgd3ArgCompletionCallback callback)
{
	rgd3_cmd_arg_completion_folder_extension(args, callback, "/", 1, "", NULL);
}

void rgd3_cmd_arg_completion_map_name(const Rgd3CmdArgs* args, Rgd3ArgCompletionCallback callback)
{
	rgd3_cmd_arg_completion_folder_extension(args, callback, "maps/", 1, ".map", NULL);
}

void rgd3_cmd_arg_completion_model_name(const Rgd3CmdArgs* args, Rgd3ArgCompletionCallback callback)
{
	rgd3_cmd_arg_completion_folder_extension(args, callback, "models/", 0, ".lwo", ".ase", ".md5mesh", ".ma", NULL);
}

void rgd3_cmd_arg_completion_sound_name(const Rgd3CmdArgs* args, Rgd3ArgCompletionCallback callback)
{
	rgd3_cmd_arg_completion_folder_extension(args, callback, "sound/", 0, ".wav", NULL);
}

void rgd3_cmd_arg_completion_image_name(const Rgd3CmdArgs* args, Rgd3ArgCompletionCallback callback)
{
	rgd3_cmd_arg_completion_folder_extension(args, callback, "/", 0, ".tga", ".dds", ".jpg", ".pcx", NULL);
}

void rgd3_cmd_arg_completion_video_name(const Rgd3CmdArgs* args, Rgd3ArgCompletionCallback callback)
{
	rgd3_cmd_arg_completion_folder_extension(args, callback, "/", 0, ".bik", NULL);
}

void rgd3_cmd_arg_completion_config_name(const Rgd3CmdArgs* args, Rgd3ArgCompletionCallback callback)
{
	rgd3_cmd_arg_completion_folder_extension(args, callback, "/", 1, ".cfg", NULL);
}

void rgd3_cmd_arg_completion_save_game(const Rgd3CmdArgs* args, Rgd3ArgCompletionCallback callback)
{
	rgd3_cmd_arg_completion_folder_extension(args, callback, "SaveGames/", 1, ".save", NULL);
}

void rgd3_cmd_arg_completion_demo_name(const Rgd3CmdArgs* args, Rgd3ArgCompletionCallback callback)
{
	rgd3_cmd_arg_completion_folder_extension(args, callback, "demos/", 1, ".demo", NULL);
}

void rgd3_cmd_register_builtins(void)
{
	rgd3_cmd_add("listCmds", rgd3_cmd_cmd_list_all, RGD3_CMD_FL_SYSTEM, "lists commands", NULL);
	rgd3_cmd_add("listSystemCmds", rgd3_cmd_cmd_list_system, RGD3_CMD_FL_SYSTEM, "lists system commands", NULL);
	rgd3_cmd_add("listRendererCmds", rgd3_cmd_cmd_list_renderer, RGD3_CMD_FL_SYSTEM, "lists renderer commands", NULL);
	rgd3_cmd_add("listSoundCmds", rgd3_cmd_cmd_list_sound, RGD3_CMD_FL_SYSTEM, "lists sound commands", NULL);
	rgd3_cmd_add("listGameCmds", rgd3_cmd_cmd_list_game, RGD3_CMD_FL_SYSTEM, "lists game commands", NULL);
	rgd3_cmd_add("exec", rgd3_cmd_cmd_exec, RGD3_CMD_FL_SYSTEM, "executes a config file", rgd3_cmd_arg_completion_config_name);
	rgd3_cmd_add("vstr", rgd3_cmd_cmd_vstr, RGD3_CMD_FL_SYSTEM, "inserts the current value of a cvar as command text", NULL);
	rgd3_cmd_add("echo", rgd3_cmd_cmd_echo, RGD3_CMD_FL_SYSTEM, "prints text", NULL);
	rgd3_cmd_add("wait", rgd3_cmd_cmd_wait, RGD3_CMD_FL_SYSTEM, "delays remaining buffered commands one or more frames", NULL);
}
