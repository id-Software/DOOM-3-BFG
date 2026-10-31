/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_cmd.h - console commands and the command text buffer
   (idCmdSystem, neo/framework/CmdSystem.h). Flag values match cmdFlags_t. */
#ifndef RGD3_CMD_H
#define RGD3_CMD_H
#ifndef RGD3_ALL_C
#error "rgd3 headers are included through src/all.c only"
#endif

enum
{
	RGD3_CMD_FL_ALL = -1,
	RGD3_CMD_FL_CHEAT = 1 << 0,
	RGD3_CMD_FL_SYSTEM = 1 << 1,
	RGD3_CMD_FL_RENDERER = 1 << 2,
	RGD3_CMD_FL_SOUND = 1 << 3,
	RGD3_CMD_FL_GAME = 1 << 4,
	RGD3_CMD_FL_TOOL = 1 << 5
};

typedef enum Rgd3CmdExec
{
	RGD3_CMD_EXEC_NOW,
	RGD3_CMD_EXEC_INSERT,
	RGD3_CMD_EXEC_APPEND
} Rgd3CmdExec;

typedef void (*Rgd3CmdFn)(const Rgd3CmdArgs* args);

void rgd3_cmd_init(void);
void rgd3_cmd_shutdown(void);
void rgd3_cmd_register_builtins(void);
void rgd3_cmd_add(const char* name, Rgd3CmdFn fn, int flags, const char* description, Rgd3ArgCompletion completion);
void rgd3_cmd_remove(const char* name);
int rgd3_cmd_exists(const char* name);

void rgd3_cmd_buffer_text(Rgd3CmdExec exec, const char* text);
void rgd3_cmd_begin_frame(void); /* once per frame: counts "wait" down */
void rgd3_cmd_execute_buffer(void);
void rgd3_cmd_execute_tokenized(const Rgd3CmdArgs* args);
void rgd3_cmd_execute_string(const char* text);
void rgd3_cmd_exec_file(const char* name, int quiet);

/* Tab completion (idCmdSystem::CommandCompletion / ArgCompletion): the
   callback receives every command name, or every full "cmd arg" line the
   command's completion function offers for cmd_string. */
void rgd3_cmd_command_completion(Rgd3ArgCompletionCallback callback);
void rgd3_cmd_arg_completion(const char* cmd_string, Rgd3ArgCompletionCallback callback);

/* Stock argument completions (idCmdSystem::ArgCompletion_*). The ones with
   extra parameters (integer range, string list, folder/extension, decl
   type) take the place of the original templates: wrap them in a small
   function with the Rgd3ArgCompletion signature. Folder listings are cached
   per "cmd arg" string until the argument changes. */
void rgd3_cmd_arg_completion_boolean(const Rgd3CmdArgs* args, Rgd3ArgCompletionCallback callback);
void rgd3_cmd_arg_completion_integer(const Rgd3CmdArgs* args, Rgd3ArgCompletionCallback callback, int min, int max);
void rgd3_cmd_arg_completion_string(const Rgd3CmdArgs* args, Rgd3ArgCompletionCallback callback, const char* const* strings);
void rgd3_cmd_arg_completion_folder_extension(const Rgd3CmdArgs* args, Rgd3ArgCompletionCallback callback,
	const char* folder, int strip_folder, ...); /* extensions, NULL-terminated */
void rgd3_cmd_arg_completion_decl_name(const Rgd3CmdArgs* args, Rgd3ArgCompletionCallback callback, int decl_type);
void rgd3_cmd_arg_completion_file_name(const Rgd3CmdArgs* args, Rgd3ArgCompletionCallback callback);
void rgd3_cmd_arg_completion_map_name(const Rgd3CmdArgs* args, Rgd3ArgCompletionCallback callback);
void rgd3_cmd_arg_completion_model_name(const Rgd3CmdArgs* args, Rgd3ArgCompletionCallback callback);
void rgd3_cmd_arg_completion_sound_name(const Rgd3CmdArgs* args, Rgd3ArgCompletionCallback callback);
void rgd3_cmd_arg_completion_image_name(const Rgd3CmdArgs* args, Rgd3ArgCompletionCallback callback);
void rgd3_cmd_arg_completion_video_name(const Rgd3CmdArgs* args, Rgd3ArgCompletionCallback callback);
void rgd3_cmd_arg_completion_config_name(const Rgd3CmdArgs* args, Rgd3ArgCompletionCallback callback);
void rgd3_cmd_arg_completion_save_game(const Rgd3CmdArgs* args, Rgd3ArgCompletionCallback callback);
void rgd3_cmd_arg_completion_demo_name(const Rgd3CmdArgs* args, Rgd3ArgCompletionCallback callback);

#endif
