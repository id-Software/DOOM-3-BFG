/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_cmdargs.c - see rgd3_cmdargs.h (neo/idlib/CmdArgs.cpp). */

void rgd3_cmdargs_clear(Rgd3CmdArgs* args)
{
	args->argc = 0;
}

int rgd3_cmdargs_argc(const Rgd3CmdArgs* args)
{
	return args->argc;
}

const char* rgd3_cmdargs_argv(const Rgd3CmdArgs* args, int index)
{
	return (index >= 0 && index < args->argc) ? args->argv[index] : "";
}

/* idCmdArgs::TokenizeString: the lexer with command-line flags, '-' joined
   to a following number, and "$name" replaced by the cvar's value. argv
   points into the args' own buffer. */
void rgd3_cmdargs_tokenize(Rgd3CmdArgs* args, const char* text, int keep_as_strings)
{
	Rgd3Lexer lex;
	Rgd3Token token = { 0 };
	Rgd3Token number = { 0 };
	int len, total = 0;

	args->argc = 0;
	if (!text)
		return;

	rgd3_lexer_init(&lex, LEXFL_NOERRORS | LEXFL_NOWARNINGS | LEXFL_NOSTRINGCONCAT | LEXFL_ALLOWPATHNAMES |
		LEXFL_NOSTRINGESCAPECHARS | LEXFL_ALLOWIPADDRESSES | (keep_as_strings ? LEXFL_ONLYSTRINGS : 0));
	rgd3_lexer_load_memory(&lex, text, (int)strlen(text), "idCmdSystemLocal::TokenizeString", 1);

	for (;;)
	{
		if (args->argc == RGD3_MAX_COMMAND_ARGS)
			break; /* usually something malicious */
		if (!rgd3_lexer_read_token(&lex, &token))
			break;
		if (!keep_as_strings && rgd3_token_is(&token, "-"))
		{
			if (rgd3_lexer_check_token_type(&lex, TT_NUMBER, 0, &number))
			{
				rgd3_token_set(&token, "-");
				rgd3_token_append(&token, number.data);
			}
		}
		if (rgd3_token_is(&token, "$"))
		{
			if (!rgd3_lexer_read_token(&lex, &token))
				break;
			rgd3_token_set(&token, rgd3_cvar_get_string(token.data));
		}
		len = token.len;
		if (total + len + 1 > (int)sizeof(args->tokenized))
			break; /* usually something malicious */
		args->argv[args->argc++] = args->tokenized + total;
		rgd3_str_copynz(args->tokenized + total, token.data, sizeof(args->tokenized) - (size_t)total);
		total += len + 1;
	}
	rgd3_token_free(&token);
	rgd3_token_free(&number);
	rgd3_lexer_free(&lex);
}

void rgd3_cmdargs_append(Rgd3CmdArgs* args, const char* text)
{
	char* dst;
	size_t used;
	if (args->argc >= RGD3_MAX_COMMAND_ARGS)
		return;
	if (!args->argc)
	{
		dst = args->tokenized;
	}
	else
	{
		dst = args->argv[args->argc - 1] + strlen(args->argv[args->argc - 1]) + 1;
	}
	used = (size_t)(dst - args->tokenized);
	if (used >= sizeof(args->tokenized))
		return;
	rgd3_str_copynz(dst, text, sizeof(args->tokenized) - used);
	args->argv[args->argc++] = dst;
}

/* Joins argv[start..end] with spaces; end < 0 means through the last arg.
   escape quotes each argument (idCmdArgs::Args with escapeArgs). */
const char* rgd3_cmdargs_args(const Rgd3CmdArgs* args, int start, int end, int escape)
{
	static char buf[RGD3_MAX_COMMAND_STRING];
	int i;
	buf[0] = 0;
	if (end < 0 || end >= args->argc)
		end = args->argc - 1;
	for (i = start; i <= end; ++i)
	{
		if (i > start)
			rgd3_str_append(buf, sizeof(buf), " ");
		if (escape)
			rgd3_str_append(buf, sizeof(buf), "\"");
		rgd3_str_append(buf, sizeof(buf), args->argv[i]);
		if (escape)
			rgd3_str_append(buf, sizeof(buf), "\"");
	}
	return buf;
}

/* The original list commands' filter: Args(start, -1) with the spaces
   removed, so "fs_*" (lexed as "fs_" "*") becomes one pattern again.
   Returns NULL when there are no arguments from start on. */
const char* rgd3_cmdargs_match(const Rgd3CmdArgs* args, int start, char* out, size_t size)
{
	const char* joined;
	size_t n = 0;
	if (args->argc <= start)
		return NULL;
	joined = rgd3_cmdargs_args(args, start, -1, 0);
	for (; *joined && n + 1 < size; ++joined)
	{
		if (*joined != ' ')
			out[n++] = *joined;
	}
	out[n] = 0;
	return out;
}
