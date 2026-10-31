/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_cmdargs.h - command line tokens (idCmdArgs, neo/idlib/CmdArgs.cpp).
   Tokenized with rgd3_lexer using the original flags (no errors, no string
   concatenation or escapes, path names, IP addresses, optional
   whitespace-only strings), '-' joined to a following number, and $cvar
   expansion. */
#ifndef RGD3_CMDARGS_H
#define RGD3_CMDARGS_H
#ifndef RGD3_ALL_C
#error "rgd3 headers are included through src/all.c only"
#endif

typedef struct Rgd3CmdArgs
{
	int argc;
	char* argv[RGD3_MAX_COMMAND_ARGS];
	char tokenized[RGD3_MAX_COMMAND_STRING];
} Rgd3CmdArgs;

void rgd3_cmdargs_clear(Rgd3CmdArgs* args);
void rgd3_cmdargs_tokenize(Rgd3CmdArgs* args, const char* text, int keep_as_strings);
void rgd3_cmdargs_append(Rgd3CmdArgs* args, const char* text);
int rgd3_cmdargs_argc(const Rgd3CmdArgs* args);
const char* rgd3_cmdargs_argv(const Rgd3CmdArgs* args, int index);
const char* rgd3_cmdargs_args(const Rgd3CmdArgs* args, int start, int end, int escape);
const char* rgd3_cmdargs_match(const Rgd3CmdArgs* args, int start, char* out, size_t size);

#endif
