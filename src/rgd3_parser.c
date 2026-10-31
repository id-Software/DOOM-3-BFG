/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_parser.c - the C/C++ compatible pre-compiler.
   Port of neo/idlib/Parser.cpp (and the define/indent structures of
   neo/idlib/Parser.h): the same define hash (PC_NameHash, 2048 buckets),
   define expansion with '#' stringizing and '##' merging, builtin defines,
   the include stack, conditional compilation, the #if/$evalint expression
   evaluator (operator priorities, '?:', integer-only operators) and the
   whole ReadToken family, with identical error and warning texts.

   Deliberate deviations, all on paths where the original leaks, crashes or
   corrupts memory:
	- every heap token the parser owns is on a per-parser list, so
	  rgd3_parser_free_source() reclaims chains a dropped error left in
	  flight; error returns free their temporary chains;
	- DefineFromString starts with an empty define table, so CopyFirstDefine
	  returns the new define and not whichever global define hashes lowest;
	- a recursively included script is freed instead of leaked;
	- GetStringFromMarker copies the text instead of writing a '\0' into
	  the (const, possibly read-only) script buffer;
	- a recursive-define token is not re-expanded inside macro arguments
	  (the original loops forever there);
	- NULL guards where the original dereferences NULL ("defined" as the
	  last #if token, missing operands, AddDefine before a load, an empty
	  include path, INT_MIN / -1). */

#define RGD3_MAX_DEFINEPARMS 128
#define RGD3_DEFINEHASHSIZE 2048
#define RGD3_TOKEN_FL_RECURSIVE_DEFINE 1
#define RGD3_EVAL_MAX_VALUES 64
#define RGD3_EVAL_MAX_OPERATORS 64

/* A heap token owned by a parser. The token comes first so the chain
   pointers (Rgd3Token*) cast back to the node. */
typedef struct Rgd3ParserToken
{
	Rgd3Token token;
	struct Rgd3ParserToken* live_prev;
	struct Rgd3ParserToken* live_next;
} Rgd3ParserToken;

static Rgd3Define* rgd3_parser_global_defines;

/* Scratch parsers for DefineFromString and GetStringFromMarker. Static so
   a drop half-way leaves nothing unreachable: the next use and
   rgd3_parser_shutdown() free whatever is left. Main thread only. */
static Rgd3Parser rgd3_parser_define_src;
static Rgd3Parser rgd3_parser_marker_src;
static char* rgd3_parser_marker_text;

static int rgd3_parser_directive_define(Rgd3Parser* p);
static int rgd3_parser_expand_define_into_source(Rgd3Parser* p, Rgd3Token* deftoken, Rgd3Define* define);

/* ---- token and lexer allocation ---- */

static Rgd3Token* rgd3_parser_new_token(Rgd3Parser* p, const Rgd3Token* src)
{
	Rgd3ParserToken* n = (Rgd3ParserToken*)rgd3_heap_alloc(sizeof(Rgd3ParserToken), RGD3_TAG_PARSER);
	rgd3_token_init(&n->token);
	if (src)
		rgd3_token_copy(&n->token, src);
	n->live_prev = NULL;
	n->live_next = p->live;
	if (p->live)
		p->live->live_prev = n;
	p->live = n;
	return &n->token;
}

static void rgd3_parser_delete_token(Rgd3Parser* p, Rgd3Token* t)
{
	Rgd3ParserToken* n = (Rgd3ParserToken*)t;
	if (n->live_prev)
		n->live_prev->live_next = n->live_next;
	else
		p->live = n->live_next;
	if (n->live_next)
		n->live_next->live_prev = n->live_prev;
	rgd3_token_free(&n->token);
	rgd3_heap_free(n);
}

static void rgd3_parser_delete_chain(Rgd3Parser* p, Rgd3Token* t)
{
	Rgd3Token* next;
	for (; t; t = next)
	{
		next = t->next;
		rgd3_parser_delete_token(p, t);
	}
}

/* define parameter and body tokens belong to their define, not a parser */
static Rgd3Token* rgd3_parser_new_define_token(const Rgd3Token* src)
{
	Rgd3Token* t = (Rgd3Token*)rgd3_heap_alloc(sizeof(Rgd3Token), RGD3_TAG_PARSER);
	rgd3_token_init(t);
	rgd3_token_copy(t, src);
	return t;
}

static void rgd3_parser_free_define_chain(Rgd3Token* t)
{
	Rgd3Token* next;
	for (; t; t = next)
	{
		next = t->next;
		rgd3_token_free(t);
		rgd3_heap_free(t);
	}
}

static Rgd3Lexer* rgd3_parser_new_lexer(void)
{
	Rgd3Lexer* lex = (Rgd3Lexer*)rgd3_heap_alloc(sizeof(Rgd3Lexer), RGD3_TAG_PARSER);
	rgd3_lexer_init(lex, 0);
	return lex;
}

static void rgd3_parser_delete_lexer(Rgd3Lexer* lex)
{
	rgd3_lexer_free(lex);
	rgd3_heap_free(lex);
}

/* ---- global defines ---- */

static Rgd3Define* rgd3_parser_define_from_string(const char* string);
static void rgd3_parser_free_define(Rgd3Define* define);

void rgd3_parser_set_base_folder(const char* path)
{
	rgd3_lexer_set_base_folder(path);
}

int rgd3_parser_add_global_define(const char* string)
{
	Rgd3Define* define = rgd3_parser_define_from_string(string);
	if (!define)
		return 0;
	define->next = rgd3_parser_global_defines;
	rgd3_parser_global_defines = define;
	return 1;
}

int rgd3_parser_remove_global_define(const char* name)
{
	Rgd3Define* d;
	Rgd3Define* prev;

	for (prev = NULL, d = rgd3_parser_global_defines; d; prev = d, d = d->next)
	{
		if (!strcmp(d->name, name))
			break;
	}
	if (d)
	{
		if (prev)
			prev->next = d->next;
		else
			rgd3_parser_global_defines = d->next;
		rgd3_parser_free_define(d);
		return 1;
	}
	return 0;
}

void rgd3_parser_remove_all_global_defines(void)
{
	Rgd3Define* define;
	for (define = rgd3_parser_global_defines; define; define = rgd3_parser_global_defines)
	{
		rgd3_parser_global_defines = rgd3_parser_global_defines->next;
		rgd3_parser_free_define(define);
	}
}

static void rgd3_parser_release_marker_scratch(void)
{
	rgd3_parser_free(&rgd3_parser_marker_src);
	if (rgd3_parser_marker_text)
	{
		rgd3_heap_free(rgd3_parser_marker_text);
		rgd3_parser_marker_text = NULL;
	}
}

void rgd3_parser_shutdown(void)
{
	rgd3_parser_remove_all_global_defines();
	rgd3_parser_free(&rgd3_parser_define_src);
	rgd3_parser_release_marker_scratch();
}

/* ---- define table ---- */

static int rgd3_parser_name_hash(const char* name)
{
	int hash, i;

	hash = 0;
	for (i = 0; name[i] != '\0'; i++)
		hash += name[i] * (119 + i); /* signed char, as on MSVC */
	hash = (hash ^ (hash >> 10) ^ (hash >> 20)) & (RGD3_DEFINEHASHSIZE - 1);
	return hash;
}

static void rgd3_parser_add_define_to_hash(Rgd3Define* define, Rgd3Define** definehash)
{
	int hash = rgd3_parser_name_hash(define->name);
	define->hashnext = definehash[hash];
	definehash[hash] = define;
}

static Rgd3Define* rgd3_parser_find_hashed_define(Rgd3Define** definehash, const char* name)
{
	Rgd3Define* d;
	if (!definehash)
		return NULL;
	for (d = definehash[rgd3_parser_name_hash(name)]; d; d = d->hashnext)
	{
		if (!strcmp(d->name, name))
			return d;
	}
	return NULL;
}

static int rgd3_parser_find_define_parm(const Rgd3Define* define, const char* name)
{
	const Rgd3Token* p;
	int i = 0;
	for (p = define->parms; p; p = p->next)
	{
		if (rgd3_token_is(p, name))
			return i;
		i++;
	}
	return -1;
}

static Rgd3Define* rgd3_parser_alloc_define(const char* name)
{
	size_t len = strlen(name);
	Rgd3Define* define = (Rgd3Define*)rgd3_heap_alloc(sizeof(Rgd3Define) + len + 1, RGD3_TAG_PARSER);
	memset(define, 0, sizeof(*define));
	define->name = (char*)(define + 1);
	memcpy(define->name, name, len + 1);
	return define;
}

static Rgd3Define* rgd3_parser_copy_define(const Rgd3Define* define)
{
	Rgd3Define* newdefine;
	Rgd3Token* token;
	Rgd3Token* newtoken;
	Rgd3Token* lasttoken;

	newdefine = rgd3_parser_alloc_define(define->name);
	newdefine->flags = define->flags;
	newdefine->builtin = define->builtin;
	newdefine->numparms = define->numparms;
	/* the copy is not linked */
	newdefine->next = NULL;
	newdefine->hashnext = NULL;
	newdefine->tokens = NULL;
	for (lasttoken = NULL, token = define->tokens; token; token = token->next)
	{
		newtoken = rgd3_parser_new_define_token(token);
		newtoken->next = NULL;
		if (lasttoken)
			lasttoken->next = newtoken;
		else
			newdefine->tokens = newtoken;
		lasttoken = newtoken;
	}
	newdefine->parms = NULL;
	for (lasttoken = NULL, token = define->parms; token; token = token->next)
	{
		newtoken = rgd3_parser_new_define_token(token);
		newtoken->next = NULL;
		if (lasttoken)
			lasttoken->next = newtoken;
		else
			newdefine->parms = newtoken;
		lasttoken = newtoken;
	}
	return newdefine;
}

static void rgd3_parser_free_define(Rgd3Define* define)
{
	rgd3_parser_free_define_chain(define->parms);
	rgd3_parser_free_define_chain(define->tokens);
	rgd3_heap_free(define);
}

static void rgd3_parser_add_global_defines_to_source(Rgd3Parser* p)
{
	Rgd3Define* define;
	for (define = rgd3_parser_global_defines; define; define = define->next)
		rgd3_parser_add_define_to_hash(rgd3_parser_copy_define(define), p->definehash);
}

/* LoadFile/LoadMemory create the table on first load; AddDefine and
   AddBuiltinDefines create it too instead of dereferencing NULL. */
static void rgd3_parser_ensure_define_hash(Rgd3Parser* p)
{
	if (p->definehash)
		return;
	p->defines = NULL;
	p->definehash = (Rgd3Define**)rgd3_heap_calloc(RGD3_DEFINEHASHSIZE * sizeof(Rgd3Define*), RGD3_TAG_PARSER);
	rgd3_parser_add_global_defines_to_source(p);
}

static Rgd3Define* rgd3_parser_copy_first_define(Rgd3Parser* p)
{
	int i;
	for (i = 0; i < RGD3_DEFINEHASHSIZE; i++)
	{
		if (p->definehash[i])
			return rgd3_parser_copy_define(p->definehash[i]);
	}
	return NULL;
}

static Rgd3Define* rgd3_parser_define_from_string(const char* string)
{
	Rgd3Parser* src = &rgd3_parser_define_src;
	Rgd3Define* def;

	rgd3_parser_free_source(src, 0); /* left over if the previous call dropped */
	rgd3_parser_init(src, 0);
	/* an empty table keeps the global defines out (see the file header) */
	src->definehash = (Rgd3Define**)rgd3_heap_calloc(RGD3_DEFINEHASHSIZE * sizeof(Rgd3Define*), RGD3_TAG_PARSER);
	if (!rgd3_parser_load_memory(src, string, (int)strlen(string), "*defineString"))
	{
		rgd3_parser_free_source(src, 0);
		return NULL;
	}
	if (!rgd3_parser_directive_define(src))
	{
		rgd3_parser_free_source(src, 0);
		return NULL;
	}
	def = rgd3_parser_copy_first_define(src);
	rgd3_parser_free_source(src, 0);
	return def;
}

/* ---- errors ---- */

void rgd3_parser_error(const Rgd3Parser* p, const char* fmt, ...)
{
	char text[RGD3_MAX_STRING_CHARS];
	va_list args;

	va_start(args, fmt);
	rg_vsnprintf(text, sizeof(text), fmt, args);
	va_end(args);
	if (p->scriptstack)
		rgd3_lexer_error(p->scriptstack, "%s", text);
}

void rgd3_parser_warning(const Rgd3Parser* p, const char* fmt, ...)
{
	char text[RGD3_MAX_STRING_CHARS];
	va_list args;

	va_start(args, fmt);
	rg_vsnprintf(text, sizeof(text), fmt, args);
	va_end(args);
	if (p->scriptstack)
		rgd3_lexer_warning(p->scriptstack, "%s", text);
}

/* ---- indents and scripts ---- */

static void rgd3_parser_push_indent(Rgd3Parser* p, int type, int skip)
{
	Rgd3Indent* indent = (Rgd3Indent*)rgd3_heap_alloc(sizeof(Rgd3Indent), RGD3_TAG_PARSER);
	indent->type = type;
	indent->script = p->scriptstack;
	indent->skip = (skip != 0);
	p->skip += indent->skip;
	indent->next = p->indentstack;
	p->indentstack = indent;
}

static void rgd3_parser_pop_indent(Rgd3Parser* p, int* type, int* skip)
{
	Rgd3Indent* indent;

	*type = 0;
	*skip = 0;
	indent = p->indentstack;
	if (!indent)
		return;
	/* must be an indent from the current script */
	if (indent->script != p->scriptstack)
		return;
	*type = indent->type;
	*skip = indent->skip;
	p->indentstack = indent->next;
	p->skip -= indent->skip;
	rgd3_heap_free(indent);
}

/* returns 0 (and does not take the script) when it is already included */
static int rgd3_parser_push_script(Rgd3Parser* p, Rgd3Lexer* script)
{
	Rgd3Lexer* s;
	for (s = p->scriptstack; s; s = s->next)
	{
		if (!rgd3_str_icmp(rgd3_lexer_file_name(s), rgd3_lexer_file_name(script)))
		{
			rgd3_parser_warning(p, "'%s' recursively included", rgd3_lexer_file_name(script));
			return 0;
		}
	}
	script->next = p->scriptstack;
	p->scriptstack = script;
	return 1;
}

/* ---- source tokens ---- */

static int rgd3_parser_read_source_token(Rgd3Parser* p, Rgd3Token* token)
{
	Rgd3Token* t;
	Rgd3Lexer* script;
	int type, skip, changed_script;

	if (!p->scriptstack)
		rgd3_error_fatal("rgd3_parser_read_source_token: not loaded");
	changed_script = 0;
	while (!p->tokens)
	{
		if (rgd3_lexer_read_token(p->scriptstack, token))
		{
			token->lines_crossed += changed_script;
			/* the marker starts at the first token read after SetMarker */
			if (!p->marker_p)
				p->marker_p = token->white_space_end_p;
			return 1;
		}
		if (rgd3_lexer_end_of_file(p->scriptstack))
		{
			/* remove all indents of the script */
			while (p->indentstack && p->indentstack->script == p->scriptstack)
			{
				rgd3_parser_warning(p, "missing #endif");
				rgd3_parser_pop_indent(p, &type, &skip);
			}
			changed_script = 1;
		}
		/* the initial script */
		if (!p->scriptstack->next)
			return 0;
		/* back to the including script */
		script = p->scriptstack;
		p->scriptstack = p->scriptstack->next;
		rgd3_parser_delete_lexer(script);
	}
	t = p->tokens;
	rgd3_token_copy(token, t);
	p->tokens = t->next;
	rgd3_parser_delete_token(p, t);
	return 1;
}

static int rgd3_parser_unread_source_token(Rgd3Parser* p, const Rgd3Token* token)
{
	Rgd3Token* t = rgd3_parser_new_token(p, token);
	t->next = p->tokens;
	p->tokens = t;
	return 1;
}

/* parms[0 .. define->numparms-1] must be NULL or chains on entry; on
   failure the caller frees them */
static int rgd3_parser_read_define_parms(Rgd3Parser* p, Rgd3Define* define, Rgd3Token** parms, int maxparms)
{
	Rgd3Token token = { 0 };
	Rgd3Define* newdefine;
	Rgd3Token* t;
	Rgd3Token* last;
	int i, done, lastcomma, numparms, indent, result = 0;

	if (!rgd3_parser_read_source_token(p, &token))
	{
		rgd3_parser_error(p, "define '%s' missing parameters", define->name);
		goto out;
	}
	if (define->numparms > maxparms)
	{
		rgd3_parser_error(p, "define with more than %d parameters", maxparms);
		goto out;
	}
	for (i = 0; i < define->numparms; i++)
		parms[i] = NULL;
	if (!rgd3_token_is(&token, "("))
	{
		rgd3_parser_unread_source_token(p, &token);
		rgd3_parser_error(p, "define '%s' missing parameters", define->name);
		goto out;
	}
	for (done = 0, numparms = 0, indent = 1; !done;)
	{
		if (numparms >= maxparms)
		{
			rgd3_parser_error(p, "define '%s' with too many parameters", define->name);
			goto out;
		}
		parms[numparms] = NULL;
		lastcomma = 1;
		last = NULL;
		while (!done)
		{
			if (!rgd3_parser_read_source_token(p, &token))
			{
				rgd3_parser_error(p, "define '%s' incomplete", define->name);
				goto out;
			}
			if (rgd3_token_is(&token, ","))
			{
				if (indent <= 1)
				{
					if (lastcomma)
						rgd3_parser_warning(p, "too many comma's");
					if (numparms >= define->numparms)
						rgd3_parser_warning(p, "too many define parameters");
					lastcomma = 1;
					break;
				}
			}
			else if (rgd3_token_is(&token, "("))
			{
				indent++;
			}
			else if (rgd3_token_is(&token, ")"))
			{
				indent--;
				if (indent <= 0)
				{
					if (!parms[define->numparms - 1])
						rgd3_parser_warning(p, "too few define parameters");
					done = 1;
					break;
				}
			}
			else if (token.type == TT_NAME && !(token.flags & RGD3_TOKEN_FL_RECURSIVE_DEFINE))
			{
				newdefine = rgd3_parser_find_hashed_define(p->definehash, rgd3_token_cstr(&token));
				if (newdefine)
				{
					if (!rgd3_parser_expand_define_into_source(p, &token, newdefine))
						goto out;
					continue;
				}
			}

			lastcomma = 0;

			if (numparms < define->numparms)
			{
				t = rgd3_parser_new_token(p, &token);
				t->next = NULL;
				if (last)
					last->next = t;
				else
					parms[numparms] = t;
				last = t;
			}
		}
		numparms++;
	}
	result = 1;
out:
	rgd3_token_free(&token);
	return result;
}

static int rgd3_parser_stringize_tokens(const Rgd3Token* tokens, Rgd3Token* token)
{
	const Rgd3Token* t;

	token->type = TT_STRING;
	token->white_space_start_p = NULL;
	token->white_space_end_p = NULL;
	rgd3_token_set(token, "");
	for (t = tokens; t; t = t->next)
		rgd3_token_append(token, rgd3_token_cstr(t));
	return 1;
}

static int rgd3_parser_merge_tokens(Rgd3Token* t1, const Rgd3Token* t2)
{
	/* a name with a name or number */
	if (t1->type == TT_NAME && (t2->type == TT_NAME || (t2->type == TT_NUMBER && !(t2->subtype & TT_FLOAT))))
	{
		rgd3_token_append(t1, rgd3_token_cstr(t2));
		return 1;
	}
	/* two strings */
	if (t1->type == TT_STRING && t2->type == TT_STRING)
	{
		rgd3_token_append(t1, rgd3_token_cstr(t2));
		return 1;
	}
	/* two numbers */
	if (t1->type == TT_NUMBER && t2->type == TT_NUMBER &&
		!(t1->subtype & (TT_HEX | TT_BINARY)) && !(t2->subtype & (TT_HEX | TT_BINARY)) &&
		(!(t1->subtype & TT_FLOAT) || !(t2->subtype & TT_FLOAT)))
	{
		rgd3_token_append(t1, rgd3_token_cstr(t2));
		return 1;
	}
	return 0;
}

void rgd3_parser_add_builtin_defines(Rgd3Parser* p)
{
	static const struct
	{
		const char* string;
		int id;
	} builtin[] = {
		{ "__LINE__", BUILTIN_LINE },
		{ "__FILE__", BUILTIN_FILE },
		{ "__DATE__", BUILTIN_DATE },
		{ "__TIME__", BUILTIN_TIME },
		{ "__STDC__", BUILTIN_STDC },
		{ NULL, 0 }
	};
	Rgd3Define* define;
	int i;

	rgd3_parser_ensure_define_hash(p);
	for (i = 0; builtin[i].string; i++)
	{
		define = rgd3_parser_alloc_define(builtin[i].string);
		define->flags = DEFINE_FIXED;
		define->builtin = builtin[i].id;
		rgd3_parser_add_define_to_hash(define, p->definehash);
	}
}

/* ctime() fields: "\"Mmm dd yyyy\"" and "\"hh:mm:ss\"", quotes included,
   as the original builds them from ctime() */
static void rgd3_parser_preprocessor_date_time(char* date, size_t date_size, char* time_text, size_t time_size)
{
	static const char* const months[12] = {
		"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
	};
	SDL_Time now;
	SDL_DateTime dt;

	if (!SDL_GetCurrentTime(&now) || !SDL_TimeToDateTime(now, &dt, true) || dt.month < 1 || dt.month > 12)
	{
		rgd3_str_copynz(date, "*** BAD CURTIME ***", date_size);
		rgd3_str_copynz(time_text, "*** BAD CURTIME ***", time_size);
		return;
	}
	rg_snprintf(date, date_size, "\"%s %2d %d\"", months[dt.month - 1], dt.day, dt.year);
	rg_snprintf(time_text, time_size, "\"%02d:%02d:%02d\"", dt.hour, dt.minute, dt.second);
}

static int rgd3_parser_expand_builtin_define(Rgd3Parser* p, Rgd3Token* deftoken, Rgd3Define* define, Rgd3Token** firsttoken, Rgd3Token** lasttoken)
{
	Rgd3Token* token;
	char buf[RGD3_MAX_STRING_CHARS];
	char date[64], time_text[64];

	token = rgd3_parser_new_token(p, deftoken);
	switch (define->builtin)
	{
	case BUILTIN_LINE:
		rg_snprintf(buf, sizeof(buf), "%d", deftoken->line);
		rgd3_token_set(token, buf);
		token->intvalue = (u32)deftoken->line;
		token->floatvalue = deftoken->line;
		token->type = TT_NUMBER;
		token->subtype = TT_DECIMAL | TT_INTEGER | TT_VALUESVALID;
		break;
	case BUILTIN_FILE:
		rgd3_token_set(token, rgd3_lexer_file_name(p->scriptstack));
		token->type = TT_NAME;
		token->subtype = token->len;
		break;
	case BUILTIN_DATE:
	case BUILTIN_TIME:
		rgd3_parser_preprocessor_date_time(date, sizeof(date), time_text, sizeof(time_text));
		rgd3_token_set(token, define->builtin == BUILTIN_DATE ? date : time_text);
		token->type = TT_STRING;
		token->subtype = token->len;
		break;
	case BUILTIN_STDC:
		rgd3_parser_warning(p, "__STDC__ not supported\n");
		/* fall through */
	default:
		rgd3_parser_delete_token(p, token);
		*firsttoken = NULL;
		*lasttoken = NULL;
		return 1;
	}
	token->line = deftoken->line;
	token->lines_crossed = deftoken->lines_crossed;
	token->flags = 0;
	*firsttoken = token;
	*lasttoken = token;
	return 1;
}

static int rgd3_parser_expand_define(Rgd3Parser* p, Rgd3Token* deftoken, Rgd3Define* define, Rgd3Token** firsttoken, Rgd3Token** lasttoken)
{
	Rgd3Token* parms[RGD3_MAX_DEFINEPARMS];
	Rgd3Token token = { 0 };
	Rgd3Token* dt;
	Rgd3Token* pt;
	Rgd3Token* t;
	Rgd3Token* t1;
	Rgd3Token* t2;
	Rgd3Token* first;
	Rgd3Token* last;
	int parmnum, i, nparms, result = 0;

	if (define->builtin)
		return rgd3_parser_expand_builtin_define(p, deftoken, define, firsttoken, lasttoken);

	first = NULL;
	last = NULL;
	nparms = define->numparms < RGD3_MAX_DEFINEPARMS ? define->numparms : RGD3_MAX_DEFINEPARMS;
	for (i = 0; i < nparms; i++)
		parms[i] = NULL;
	if (define->numparms)
	{
		if (!rgd3_parser_read_define_parms(p, define, parms, RGD3_MAX_DEFINEPARMS))
			goto out;
	}
	/* the expanded token list */
	for (dt = define->tokens; dt; dt = dt->next)
	{
		parmnum = -1;
		/* a name could be a define parameter */
		if (dt->type == TT_NAME)
			parmnum = rgd3_parser_find_define_parm(define, rgd3_token_cstr(dt));
		if (parmnum >= 0)
		{
			for (pt = parms[parmnum]; pt; pt = pt->next)
			{
				t = rgd3_parser_new_token(p, pt);
				t->next = NULL;
				if (last)
					last->next = t;
				else
					first = t;
				last = t;
			}
		}
		else
		{
			if (rgd3_token_is(dt, "#"))
			{
				/* the stringizing operator must be followed by a define parameter */
				if (dt->next)
					parmnum = rgd3_parser_find_define_parm(define, rgd3_token_cstr(dt->next));
				else
					parmnum = -1;

				if (parmnum >= 0)
				{
					dt = dt->next;
					if (!rgd3_parser_stringize_tokens(parms[parmnum], &token))
					{
						rgd3_parser_error(p, "can't stringize tokens");
						goto out;
					}
					t = rgd3_parser_new_token(p, &token);
					t->line = deftoken->line;
				}
				else
				{
					rgd3_parser_warning(p, "stringizing operator without define parameter");
					continue;
				}
			}
			else
			{
				t = rgd3_parser_new_token(p, dt);
				t->line = deftoken->line;
			}
			t->next = NULL;
			/* tokens from the define list take the line of the original file,
			   not of the header the define came from */
			t->line = deftoken->line;
			if (last)
				last->next = t;
			else
				first = t;
			last = t;
		}
	}
	/* the merging operator */
	for (t = first; t;)
	{
		if (t->next)
		{
			if (rgd3_token_is(t->next, "##"))
			{
				t1 = t;
				t2 = t->next->next;
				if (t2)
				{
					if (!rgd3_parser_merge_tokens(t1, t2))
					{
						rgd3_parser_error(p, "can't merge '%s' with '%s'", rgd3_token_cstr(t1), rgd3_token_cstr(t2));
						goto out;
					}
					rgd3_parser_delete_token(p, t1->next);
					t1->next = t2->next;
					if (t2 == last)
						last = t1;
					rgd3_parser_delete_token(p, t2);
					continue;
				}
			}
		}
		t = t->next;
	}
	*firsttoken = first;
	*lasttoken = last;
	first = NULL;
	result = 1;
out:
	rgd3_parser_delete_chain(p, first);
	for (i = 0; i < nparms; i++)
		rgd3_parser_delete_chain(p, parms[i]);
	rgd3_token_free(&token);
	return result;
}

static int rgd3_parser_expand_define_into_source(Rgd3Parser* p, Rgd3Token* deftoken, Rgd3Define* define)
{
	Rgd3Token* firsttoken;
	Rgd3Token* lasttoken;

	if (!rgd3_parser_expand_define(p, deftoken, define, &firsttoken, &lasttoken))
		return 0;
	/* if the define is not empty */
	if (firsttoken && lasttoken)
	{
		firsttoken->lines_crossed += deftoken->lines_crossed;
		lasttoken->next = p->tokens;
		p->tokens = firsttoken;
	}
	return 1;
}

/* A token from the current line; continues on the next line only after a
   backslash. */
static int rgd3_parser_read_line(Rgd3Parser* p, Rgd3Token* token)
{
	int crossline = 0;
	do
	{
		if (!rgd3_parser_read_source_token(p, token))
			return 0;
		if (token->lines_crossed > crossline)
		{
			rgd3_parser_unread_source_token(p, token);
			return 0;
		}
		crossline = 1;
	} while (rgd3_token_is(token, "\\"));
	return 1;
}

/* ---- directives ---- */

/* idStr::StripFilename: cut at the last separator; no separator leaves "" */
static void rgd3_parser_strip_filename(char* path)
{
	int pos = (int)strlen(path) - 1;
	while (pos > 0 && path[pos] != '/' && path[pos] != '\\')
		pos--;
	if (pos < 0)
		pos = 0;
	path[pos] = 0;
}

static int rgd3_parser_directive_include(Rgd3Parser* p)
{
	Rgd3Lexer* script = NULL;
	Rgd3Token token = { 0 };
	char path[RGD3_MAX_OSPATH];
	char full[RGD3_MAX_OSPATH];
	int result = 0;

	path[0] = 0;
	if (!rgd3_parser_read_source_token(p, &token))
	{
		rgd3_parser_error(p, "#include without file name");
		goto out;
	}
	if (token.lines_crossed > 0)
	{
		rgd3_parser_error(p, "#include without file name");
		goto out;
	}
	if (token.type == TT_STRING)
	{
		script = rgd3_parser_new_lexer();
		/* relative to the current file */
		rgd3_str_copynz(path, rgd3_lexer_file_name(p->scriptstack), sizeof(path));
		rgd3_parser_strip_filename(path);
		rgd3_str_append(path, sizeof(path), "/");
		rgd3_str_append(path, sizeof(path), rgd3_token_cstr(&token));
		if (!rgd3_lexer_load_file(script, path, p->os_path))
		{
			/* absolute */
			rgd3_str_copynz(path, rgd3_token_cstr(&token), sizeof(path));
			if (!rgd3_lexer_load_file(script, path, p->os_path))
			{
				/* from the include path */
				rg_snprintf(path, sizeof(path), "%s%s", p->includepath, rgd3_token_cstr(&token));
				if (!rgd3_lexer_load_file(script, path, p->os_path))
				{
					rgd3_parser_delete_lexer(script);
					script = NULL;
				}
			}
		}
	}
	else if (token.type == TT_PUNCTUATION && rgd3_token_is(&token, "<"))
	{
		rgd3_str_copynz(path, p->includepath, sizeof(path));
		while (rgd3_parser_read_source_token(p, &token))
		{
			if (token.lines_crossed > 0)
			{
				rgd3_parser_unread_source_token(p, &token);
				break;
			}
			if (token.type == TT_PUNCTUATION && rgd3_token_is(&token, ">"))
				break;
			rgd3_str_append(path, sizeof(path), rgd3_token_cstr(&token));
		}
		if (!rgd3_token_is(&token, ">"))
			rgd3_parser_warning(p, "#include missing trailing >");
		if (!path[0])
		{
			rgd3_parser_error(p, "#include without file name between < >");
			goto out;
		}
		if (p->flags & LEXFL_NOBASEINCLUDES)
		{
			result = 1;
			goto out;
		}
		script = rgd3_parser_new_lexer();
		/* path already starts with the include path; the original prefixes
		   it again */
		rg_snprintf(full, sizeof(full), "%s%s", p->includepath, path);
		if (!rgd3_lexer_load_file(script, full, p->os_path))
		{
			rgd3_parser_delete_lexer(script);
			script = NULL;
		}
	}
	else
	{
		rgd3_parser_error(p, "#include without file name");
		goto out;
	}
	if (!script)
	{
		rgd3_parser_error(p, "file '%s' not found", path);
		goto out;
	}
	rgd3_lexer_set_flags(script, p->flags);
	rgd3_lexer_set_punctuations(script, p->punctuations);
	if (!rgd3_parser_push_script(p, script))
		rgd3_parser_delete_lexer(script);
	result = 1;
out:
	rgd3_token_free(&token);
	return result;
}

static int rgd3_parser_directive_undef(Rgd3Parser* p)
{
	Rgd3Token token = { 0 };
	Rgd3Define* define;
	Rgd3Define* lastdefine;
	int hash, result = 0;

	if (!rgd3_parser_read_line(p, &token))
	{
		rgd3_parser_error(p, "undef without name");
		goto out;
	}
	if (token.type != TT_NAME)
	{
		rgd3_parser_unread_source_token(p, &token);
		rgd3_parser_error(p, "expected name but found '%s'", rgd3_token_cstr(&token));
		goto out;
	}
	hash = rgd3_parser_name_hash(rgd3_token_cstr(&token));
	for (lastdefine = NULL, define = p->definehash[hash]; define; define = define->hashnext)
	{
		if (!strcmp(define->name, rgd3_token_cstr(&token)))
		{
			if (define->flags & DEFINE_FIXED)
			{
				rgd3_parser_warning(p, "can't undef '%s'", rgd3_token_cstr(&token));
			}
			else
			{
				if (lastdefine)
					lastdefine->hashnext = define->hashnext;
				else
					p->definehash[hash] = define->hashnext;
				rgd3_parser_free_define(define);
			}
			break;
		}
		lastdefine = define;
	}
	result = 1;
out:
	rgd3_token_free(&token);
	return result;
}

static int rgd3_parser_directive_define(Rgd3Parser* p)
{
	Rgd3Token token = { 0 };
	Rgd3Token* t;
	Rgd3Token* last;
	Rgd3Define* define;
	int result = 0;

	if (!rgd3_parser_read_line(p, &token))
	{
		rgd3_parser_error(p, "#define without name");
		goto out;
	}
	if (token.type != TT_NAME)
	{
		rgd3_parser_unread_source_token(p, &token);
		rgd3_parser_error(p, "expected name after #define, found '%s'", rgd3_token_cstr(&token));
		goto out;
	}
	/* an existing define */
	define = rgd3_parser_find_hashed_define(p->definehash, rgd3_token_cstr(&token));
	if (define)
	{
		if (define->flags & DEFINE_FIXED)
		{
			rgd3_parser_error(p, "can't redefine '%s'", rgd3_token_cstr(&token));
			goto out;
		}
		rgd3_parser_warning(p, "redefinition of '%s'", rgd3_token_cstr(&token));
		/* unread the name for #undef */
		rgd3_parser_unread_source_token(p, &token);
		if (!rgd3_parser_directive_undef(p))
			goto out;
	}
	define = rgd3_parser_alloc_define(rgd3_token_cstr(&token));
	rgd3_parser_add_define_to_hash(define, p->definehash);
	/* nothing defined */
	if (!rgd3_parser_read_line(p, &token))
	{
		result = 1;
		goto out;
	}
	/* parameters: '(' right behind the name */
	if (!rgd3_token_white_space_before(&token) && rgd3_token_is(&token, "("))
	{
		last = NULL;
		if (!rgd3_parser_check_token_string(p, ")"))
		{
			for (;;)
			{
				if (!rgd3_parser_read_line(p, &token))
				{
					rgd3_parser_error(p, "expected define parameter");
					goto out;
				}
				if (token.type != TT_NAME)
				{
					rgd3_parser_error(p, "invalid define parameter");
					goto out;
				}
				if (rgd3_parser_find_define_parm(define, rgd3_token_cstr(&token)) >= 0)
				{
					rgd3_parser_error(p, "two the same define parameters");
					goto out;
				}
				t = rgd3_parser_new_define_token(&token);
				rgd3_token_clear_white_space(t);
				t->next = NULL;
				if (last)
					last->next = t;
				else
					define->parms = t;
				last = t;
				define->numparms++;
				if (!rgd3_parser_read_line(p, &token))
				{
					rgd3_parser_error(p, "define parameters not terminated");
					goto out;
				}
				if (rgd3_token_is(&token, ")"))
					break;
				if (!rgd3_token_is(&token, ","))
				{
					rgd3_parser_error(p, "define not terminated");
					goto out;
				}
			}
		}
		if (!rgd3_parser_read_line(p, &token))
		{
			result = 1;
			goto out;
		}
	}
	/* the body */
	last = NULL;
	do
	{
		t = rgd3_parser_new_define_token(&token);
		if (t->type == TT_NAME && !strcmp(rgd3_token_cstr(t), define->name))
		{
			t->flags |= RGD3_TOKEN_FL_RECURSIVE_DEFINE;
			rgd3_parser_warning(p, "recursive define (removed recursion)");
		}
		rgd3_token_clear_white_space(t);
		t->next = NULL;
		if (last)
			last->next = t;
		else
			define->tokens = t;
		last = t;
	} while (rgd3_parser_read_line(p, &token));

	if (last)
	{
		/* merge operators at the beginning or end */
		if (rgd3_token_is(define->tokens, "##") || rgd3_token_is(last, "##"))
		{
			rgd3_parser_error(p, "define with misplaced ##");
			goto out;
		}
	}
	result = 1;
out:
	rgd3_token_free(&token);
	return result;
}

int rgd3_parser_add_define(Rgd3Parser* p, const char* string)
{
	Rgd3Define* define = rgd3_parser_define_from_string(string);
	if (!define)
		return 0;
	rgd3_parser_ensure_define_hash(p);
	rgd3_parser_add_define_to_hash(define, p->definehash);
	return 1;
}

static int rgd3_parser_directive_if_def(Rgd3Parser* p, int type)
{
	Rgd3Token token = { 0 };
	Rgd3Define* d;
	int skip, result = 0;

	if (!rgd3_parser_read_line(p, &token))
	{
		rgd3_parser_error(p, "#ifdef without name");
		goto out;
	}
	if (token.type != TT_NAME)
	{
		rgd3_parser_unread_source_token(p, &token);
		rgd3_parser_error(p, "expected name after #ifdef, found '%s'", rgd3_token_cstr(&token));
		goto out;
	}
	d = rgd3_parser_find_hashed_define(p->definehash, rgd3_token_cstr(&token));
	skip = (type == INDENT_IFDEF) == (d == NULL);
	rgd3_parser_push_indent(p, type, skip);
	result = 1;
out:
	rgd3_token_free(&token);
	return result;
}

static int rgd3_parser_directive_else(Rgd3Parser* p)
{
	int type, skip;

	rgd3_parser_pop_indent(p, &type, &skip);
	if (!type)
	{
		rgd3_parser_error(p, "misplaced #else");
		return 0;
	}
	if (type == INDENT_ELSE)
	{
		rgd3_parser_error(p, "#else after #else");
		return 0;
	}
	rgd3_parser_push_indent(p, INDENT_ELSE, !skip);
	return 1;
}

static int rgd3_parser_directive_endif(Rgd3Parser* p)
{
	int type, skip;

	rgd3_parser_pop_indent(p, &type, &skip);
	if (!type)
	{
		rgd3_parser_error(p, "misplaced #endif");
		return 0;
	}
	return 1;
}

/* ---- expression evaluation ---- */

typedef struct Rgd3ParserEvalOperator
{
	int op;
	int priority;
	int parentheses;
	struct Rgd3ParserEvalOperator* prev;
	struct Rgd3ParserEvalOperator* next;
} Rgd3ParserEvalOperator;

typedef struct Rgd3ParserEvalValue
{
	i32 intvalue; /* `signed long` on the original Win32 build */
	double floatvalue;
	int parentheses;
	struct Rgd3ParserEvalValue* prev;
	struct Rgd3ParserEvalValue* next;
} Rgd3ParserEvalValue;

static int rgd3_parser_operator_priority(int op)
{
	switch (op)
	{
	case P_MUL: return 15;
	case P_DIV: return 15;
	case P_MOD: return 15;
	case P_ADD: return 14;
	case P_SUB: return 14;

	case P_LOGIC_AND: return 7;
	case P_LOGIC_OR: return 6;
	case P_LOGIC_GEQ: return 12;
	case P_LOGIC_LEQ: return 12;
	case P_LOGIC_EQ: return 11;
	case P_LOGIC_UNEQ: return 11;

	case P_LOGIC_NOT: return 16;
	case P_LOGIC_GREATER: return 12;
	case P_LOGIC_LESS: return 12;

	case P_RSHIFT: return 13;
	case P_LSHIFT: return 13;

	case P_BIN_AND: return 10;
	case P_BIN_OR: return 8;
	case P_BIN_XOR: return 9;
	case P_BIN_NOT: return 16;

	case P_COLON: return 5;
	case P_QUESTIONMARK: return 5;
	}
	return 0;
}

/* two's complement wrap-around, as the original's 32-bit long did */
static i32 rgd3_parser_eval_wrap(u32 v)
{
	return (i32)v;
}

static int rgd3_parser_evaluate_tokens(Rgd3Parser* p, Rgd3Token* tokens, i32* intvalue, double* floatvalue, int integer)
{
	Rgd3ParserEvalOperator operator_heap[RGD3_EVAL_MAX_OPERATORS];
	Rgd3ParserEvalValue value_heap[RGD3_EVAL_MAX_VALUES];
	Rgd3ParserEvalOperator* o;
	Rgd3ParserEvalOperator* firstoperator;
	Rgd3ParserEvalOperator* lastoperator;
	Rgd3ParserEvalValue* v;
	Rgd3ParserEvalValue* firstvalue;
	Rgd3ParserEvalValue* lastvalue;
	Rgd3ParserEvalValue* v1;
	Rgd3ParserEvalValue* v2;
	Rgd3Token* t;
	int brace = 0;
	int parentheses = 0;
	int error = 0;
	int lastwasvalue = 0;
	int negativevalue = 0;
	i32 questmarkintvalue = 0;
	double questmarkfloatvalue = 0;
	int gotquestmarkvalue = 0;
	int numoperators = 0;
	int numvalues = 0;

	firstoperator = lastoperator = NULL;
	firstvalue = lastvalue = NULL;
	if (intvalue)
		*intvalue = 0;
	if (floatvalue)
		*floatvalue = 0;
	for (t = tokens; t; t = t->next)
	{
		switch (t->type)
		{
		case TT_NAME:
		{
			if (lastwasvalue || negativevalue)
			{
				rgd3_parser_error(p, "syntax error in #if/#elif");
				error = 1;
				break;
			}
			if (!rgd3_token_is(t, "defined"))
			{
				rgd3_parser_error(p, "undefined name '%s' in #if/#elif", rgd3_token_cstr(t));
				error = 1;
				break;
			}
			t = t->next;
			if (t && rgd3_token_is(t, "("))
			{
				brace = 1;
				t = t->next;
			}
			if (!t || t->type != TT_NAME)
			{
				rgd3_parser_error(p, "defined() without name in #if/#elif");
				error = 1;
				break;
			}
			if (numvalues >= RGD3_EVAL_MAX_VALUES)
			{
				rgd3_parser_error(p, "out of value space\n");
				error = 1;
				break;
			}
			v = &value_heap[numvalues++];
			if (rgd3_parser_find_hashed_define(p->definehash, rgd3_token_cstr(t)))
			{
				v->intvalue = 1;
				v->floatvalue = 1;
			}
			else
			{
				v->intvalue = 0;
				v->floatvalue = 0;
			}
			v->parentheses = parentheses;
			v->next = NULL;
			v->prev = lastvalue;
			if (lastvalue)
				lastvalue->next = v;
			else
				firstvalue = v;
			lastvalue = v;
			if (brace)
			{
				t = t->next;
				if (!t || !rgd3_token_is(t, ")"))
				{
					rgd3_parser_error(p, "defined missing ) in #if/#elif");
					error = 1;
					break;
				}
			}
			brace = 0;
			/* defined() creates a value */
			lastwasvalue = 1;
			break;
		}
		case TT_NUMBER:
		{
			if (lastwasvalue)
			{
				rgd3_parser_error(p, "syntax error in #if/#elif");
				error = 1;
				break;
			}
			if (numvalues >= RGD3_EVAL_MAX_VALUES)
			{
				rgd3_parser_error(p, "out of value space\n");
				error = 1;
				break;
			}
			v = &value_heap[numvalues++];
			if (negativevalue)
			{
				v->intvalue = rgd3_parser_eval_wrap(0u - (u32)rgd3_token_int_value(t));
				v->floatvalue = -rgd3_token_float_value(t);
			}
			else
			{
				v->intvalue = rgd3_token_int_value(t);
				v->floatvalue = rgd3_token_float_value(t);
			}
			v->parentheses = parentheses;
			v->next = NULL;
			v->prev = lastvalue;
			if (lastvalue)
				lastvalue->next = v;
			else
				firstvalue = v;
			lastvalue = v;
			lastwasvalue = 1;
			negativevalue = 0;
			break;
		}
		case TT_PUNCTUATION:
		{
			if (negativevalue)
			{
				rgd3_parser_error(p, "misplaced minus sign in #if/#elif");
				error = 1;
				break;
			}
			if (t->subtype == P_PARENTHESESOPEN)
			{
				parentheses++;
				break;
			}
			else if (t->subtype == P_PARENTHESESCLOSE)
			{
				parentheses--;
				if (parentheses < 0)
				{
					rgd3_parser_error(p, "too many ) in #if/#elsif");
					error = 1;
				}
				break;
			}
			/* invalid operators on floating point values */
			if (!integer)
			{
				if (t->subtype == P_BIN_NOT || t->subtype == P_MOD ||
					t->subtype == P_RSHIFT || t->subtype == P_LSHIFT ||
					t->subtype == P_BIN_AND || t->subtype == P_BIN_OR ||
					t->subtype == P_BIN_XOR)
				{
					rgd3_parser_error(p, "illigal operator '%s' on floating point operands\n", rgd3_token_cstr(t));
					error = 1;
					break;
				}
			}
			switch (t->subtype)
			{
			case P_LOGIC_NOT:
			case P_BIN_NOT:
				if (lastwasvalue)
				{
					rgd3_parser_error(p, "! or ~ after value in #if/#elif");
					error = 1;
				}
				break;
			case P_INC:
			case P_DEC:
				/* reported, but not an error: the operator is still added */
				rgd3_parser_error(p, "++ or -- used in #if/#elif");
				break;
			case P_SUB:
				if (!lastwasvalue)
				{
					negativevalue = 1;
					break;
				}
				/* fall through */
			case P_MUL:
			case P_DIV:
			case P_MOD:
			case P_ADD:

			case P_LOGIC_AND:
			case P_LOGIC_OR:
			case P_LOGIC_GEQ:
			case P_LOGIC_LEQ:
			case P_LOGIC_EQ:
			case P_LOGIC_UNEQ:

			case P_LOGIC_GREATER:
			case P_LOGIC_LESS:

			case P_RSHIFT:
			case P_LSHIFT:

			case P_BIN_AND:
			case P_BIN_OR:
			case P_BIN_XOR:

			case P_COLON:
			case P_QUESTIONMARK:
				if (!lastwasvalue)
				{
					rgd3_parser_error(p, "operator '%s' after operator in #if/#elif", rgd3_token_cstr(t));
					error = 1;
				}
				break;
			default:
				rgd3_parser_error(p, "invalid operator '%s' in #if/#elif", rgd3_token_cstr(t));
				error = 1;
				break;
			}
			if (!error && !negativevalue)
			{
				if (numoperators >= RGD3_EVAL_MAX_OPERATORS)
				{
					rgd3_parser_error(p, "out of operator space\n");
					error = 1;
					break;
				}
				o = &operator_heap[numoperators++];
				o->op = t->subtype;
				o->priority = rgd3_parser_operator_priority(t->subtype);
				o->parentheses = parentheses;
				o->next = NULL;
				o->prev = lastoperator;
				if (lastoperator)
					lastoperator->next = o;
				else
					firstoperator = o;
				lastoperator = o;
				lastwasvalue = 0;
			}
			break;
		}
		default:
		{
			rgd3_parser_error(p, "unknown '%s' in #if/#elif", rgd3_token_cstr(t));
			error = 1;
			break;
		}
		}
		if (error)
			break;
	}
	if (!error)
	{
		if (!lastwasvalue)
		{
			rgd3_parser_error(p, "trailing operator in #if/#elif");
			error = 1;
		}
		else if (parentheses)
		{
			rgd3_parser_error(p, "too many ( in #if/#elif");
			error = 1;
		}
	}

	gotquestmarkvalue = 0;
	questmarkintvalue = 0;
	questmarkfloatvalue = 0;
	/* while there are operators */
	while (!error && firstoperator)
	{
		v = firstvalue;
		for (o = firstoperator; o->next; o = o->next)
		{
			/* the current operator is nested deeper in parentheses than the next */
			if (o->parentheses > o->next->parentheses)
				break;
			/* equally deep: the current one goes first at equal or higher priority */
			if (o->parentheses == o->next->parentheses)
			{
				if (o->priority >= o->next->priority)
					break;
			}
			/* operators with arity 2 consume a value */
			if (o->op != P_LOGIC_NOT && o->op != P_BIN_NOT)
				v = v->next;
			if (!v)
			{
				rgd3_parser_error(p, "mising values in #if/#elif");
				error = 1;
				break;
			}
		}
		if (error)
			break;
		v1 = v;
		v2 = v->next;
		if (!v2 && o->op != P_LOGIC_NOT && o->op != P_BIN_NOT)
		{
			/* the original dereferences NULL here */
			rgd3_parser_error(p, "mising values in #if/#elif");
			error = 1;
			break;
		}
		switch (o->op)
		{
		case P_LOGIC_NOT:
			v1->intvalue = !v1->intvalue;
			v1->floatvalue = !v1->floatvalue;
			break;
		case P_BIN_NOT:
			v1->intvalue = ~v1->intvalue;
			break;
		case P_MUL:
			v1->intvalue = rgd3_parser_eval_wrap((u32)v1->intvalue * (u32)v2->intvalue);
			v1->floatvalue *= v2->floatvalue;
			break;
		case P_DIV:
			if (!v2->intvalue || !v2->floatvalue)
			{
				rgd3_parser_error(p, "divide by zero in #if/#elif\n");
				error = 1;
				break;
			}
			if (v2->intvalue == -1)
				v1->intvalue = rgd3_parser_eval_wrap(0u - (u32)v1->intvalue);
			else
				v1->intvalue /= v2->intvalue;
			v1->floatvalue /= v2->floatvalue;
			break;
		case P_MOD:
			if (!v2->intvalue)
			{
				rgd3_parser_error(p, "divide by zero in #if/#elif\n");
				error = 1;
				break;
			}
			if (v2->intvalue == -1)
				v1->intvalue = 0;
			else
				v1->intvalue %= v2->intvalue;
			break;
		case P_ADD:
			v1->intvalue = rgd3_parser_eval_wrap((u32)v1->intvalue + (u32)v2->intvalue);
			v1->floatvalue += v2->floatvalue;
			break;
		case P_SUB:
			v1->intvalue = rgd3_parser_eval_wrap((u32)v1->intvalue - (u32)v2->intvalue);
			v1->floatvalue -= v2->floatvalue;
			break;
		case P_LOGIC_AND:
			v1->intvalue = v1->intvalue && v2->intvalue;
			v1->floatvalue = v1->floatvalue && v2->floatvalue;
			break;
		case P_LOGIC_OR:
			v1->intvalue = v1->intvalue || v2->intvalue;
			v1->floatvalue = v1->floatvalue || v2->floatvalue;
			break;
		case P_LOGIC_GEQ:
			v1->intvalue = v1->intvalue >= v2->intvalue;
			v1->floatvalue = v1->floatvalue >= v2->floatvalue;
			break;
		case P_LOGIC_LEQ:
			v1->intvalue = v1->intvalue <= v2->intvalue;
			v1->floatvalue = v1->floatvalue <= v2->floatvalue;
			break;
		case P_LOGIC_EQ:
			v1->intvalue = v1->intvalue == v2->intvalue;
			v1->floatvalue = v1->floatvalue == v2->floatvalue;
			break;
		case P_LOGIC_UNEQ:
			v1->intvalue = v1->intvalue != v2->intvalue;
			v1->floatvalue = v1->floatvalue != v2->floatvalue;
			break;
		case P_LOGIC_GREATER:
			v1->intvalue = v1->intvalue > v2->intvalue;
			v1->floatvalue = v1->floatvalue > v2->floatvalue;
			break;
		case P_LOGIC_LESS:
			v1->intvalue = v1->intvalue < v2->intvalue;
			v1->floatvalue = v1->floatvalue < v2->floatvalue;
			break;
		case P_RSHIFT:
			/* x86 masks the shift count */
			v1->intvalue >>= (v2->intvalue & 31);
			break;
		case P_LSHIFT:
			v1->intvalue = rgd3_parser_eval_wrap((u32)v1->intvalue << (v2->intvalue & 31));
			break;
		case P_BIN_AND:
			v1->intvalue &= v2->intvalue;
			break;
		case P_BIN_OR:
			v1->intvalue |= v2->intvalue;
			break;
		case P_BIN_XOR:
			v1->intvalue ^= v2->intvalue;
			break;
		case P_COLON:
			if (!gotquestmarkvalue)
			{
				rgd3_parser_error(p, ": without ? in #if/#elif");
				error = 1;
				break;
			}
			if (integer)
			{
				if (!questmarkintvalue)
					v1->intvalue = v2->intvalue;
			}
			else
			{
				if (!questmarkfloatvalue)
					v1->floatvalue = v2->floatvalue;
			}
			gotquestmarkvalue = 0;
			break;
		case P_QUESTIONMARK:
			if (gotquestmarkvalue)
			{
				rgd3_parser_error(p, "? after ? in #if/#elif");
				error = 1;
				break;
			}
			questmarkintvalue = v1->intvalue;
			questmarkfloatvalue = v1->floatvalue;
			gotquestmarkvalue = 1;
			break;
		}
		if (error)
			break;
		/* operators with arity 2 remove a value: the second one, or the
		   condition for '?' */
		if (o->op != P_LOGIC_NOT && o->op != P_BIN_NOT)
		{
			if (o->op != P_QUESTIONMARK)
				v = v->next;
			if (v->prev)
				v->prev->next = v->next;
			else
				firstvalue = v->next;
			if (v->next)
				v->next->prev = v->prev;
			else
				lastvalue = v->prev;
		}
		/* remove the operator */
		if (o->prev)
			o->prev->next = o->next;
		else
			firstoperator = o->next;
		if (o->next)
			o->next->prev = o->prev;
		else
			lastoperator = o->prev;
	}
	if (firstvalue)
	{
		if (intvalue)
			*intvalue = firstvalue->intvalue;
		if (floatvalue)
			*floatvalue = firstvalue->floatvalue;
	}
	if (!error)
		return 1;
	if (intvalue)
		*intvalue = 0;
	if (floatvalue)
		*floatvalue = 0;
	return 0;
}

static void rgd3_parser_append_copy(Rgd3Parser* p, const Rgd3Token* token, Rgd3Token** first, Rgd3Token** last)
{
	Rgd3Token* t = rgd3_parser_new_token(p, token);
	t->next = NULL;
	if (*last)
		(*last)->next = t;
	else
		*first = t;
	*last = t;
}

static int rgd3_parser_evaluate(Rgd3Parser* p, i32* intvalue, double* floatvalue, int integer)
{
	Rgd3Token token = { 0 };
	Rgd3Token* firsttoken = NULL;
	Rgd3Token* lasttoken = NULL;
	Rgd3Define* define;
	int defined = 0, result = 0;

	if (intvalue)
		*intvalue = 0;
	if (floatvalue)
		*floatvalue = 0;
	if (!rgd3_parser_read_line(p, &token))
	{
		rgd3_parser_error(p, "no value after #if/#elif");
		goto out;
	}
	do
	{
		if (token.type == TT_NAME)
		{
			if (defined)
			{
				defined = 0;
				rgd3_parser_append_copy(p, &token, &firsttoken, &lasttoken);
			}
			else if (rgd3_token_is(&token, "defined"))
			{
				defined = 1;
				rgd3_parser_append_copy(p, &token, &firsttoken, &lasttoken);
			}
			else
			{
				/* then it must be a define */
				define = rgd3_parser_find_hashed_define(p->definehash, rgd3_token_cstr(&token));
				if (!define)
				{
					rgd3_parser_error(p, "can't Evaluate '%s', not defined", rgd3_token_cstr(&token));
					goto out;
				}
				if (!rgd3_parser_expand_define_into_source(p, &token, define))
					goto out;
			}
		}
		else if (token.type == TT_NUMBER || token.type == TT_PUNCTUATION)
		{
			rgd3_parser_append_copy(p, &token, &firsttoken, &lasttoken);
		}
		else
		{
			rgd3_parser_error(p, "can't Evaluate '%s'", rgd3_token_cstr(&token));
			goto out;
		}
	} while (rgd3_parser_read_line(p, &token));

	if (!rgd3_parser_evaluate_tokens(p, firsttoken, intvalue, floatvalue, integer))
		goto out;
	result = 1;
out:
	rgd3_parser_delete_chain(p, firsttoken);
	rgd3_token_free(&token);
	return result;
}

static int rgd3_parser_dollar_evaluate(Rgd3Parser* p, i32* intvalue, double* floatvalue, int integer)
{
	Rgd3Token token = { 0 };
	Rgd3Token* firsttoken = NULL;
	Rgd3Token* lasttoken = NULL;
	Rgd3Define* define;
	int indent, defined = 0, result = 0;

	if (intvalue)
		*intvalue = 0;
	if (floatvalue)
		*floatvalue = 0;
	/* the leading '(' is not checked */
	if (!rgd3_parser_read_source_token(p, &token))
	{
		rgd3_parser_error(p, "no leading ( after $evalint/$evalfloat");
		goto out;
	}
	if (!rgd3_parser_read_source_token(p, &token))
	{
		rgd3_parser_error(p, "nothing to Evaluate");
		goto out;
	}
	indent = 1;
	do
	{
		if (token.type == TT_NAME)
		{
			if (defined)
			{
				defined = 0;
				rgd3_parser_append_copy(p, &token, &firsttoken, &lasttoken);
			}
			else if (rgd3_token_is(&token, "defined"))
			{
				defined = 1;
				rgd3_parser_append_copy(p, &token, &firsttoken, &lasttoken);
			}
			else
			{
				define = rgd3_parser_find_hashed_define(p->definehash, rgd3_token_cstr(&token));
				if (!define)
				{
					rgd3_parser_warning(p, "can't Evaluate '%s', not defined", rgd3_token_cstr(&token));
					goto out;
				}
				if (!rgd3_parser_expand_define_into_source(p, &token, define))
					goto out;
			}
		}
		else if (token.type == TT_NUMBER || token.type == TT_PUNCTUATION)
		{
			if (rgd3_token_cstr(&token)[0] == '(')
				indent++;
			else if (rgd3_token_cstr(&token)[0] == ')')
				indent--;
			if (indent <= 0)
				break;
			rgd3_parser_append_copy(p, &token, &firsttoken, &lasttoken);
		}
		else
		{
			rgd3_parser_error(p, "can't Evaluate '%s'", rgd3_token_cstr(&token));
			goto out;
		}
	} while (rgd3_parser_read_source_token(p, &token));

	if (!rgd3_parser_evaluate_tokens(p, firsttoken, intvalue, floatvalue, integer))
		goto out;
	result = 1;
out:
	rgd3_parser_delete_chain(p, firsttoken);
	rgd3_token_free(&token);
	return result;
}

static int rgd3_parser_directive_elif(Rgd3Parser* p)
{
	i32 value;
	int type, skip;

	rgd3_parser_pop_indent(p, &type, &skip);
	if (!type || type == INDENT_ELSE)
	{
		rgd3_parser_error(p, "misplaced #elif");
		return 0;
	}
	if (!rgd3_parser_evaluate(p, &value, NULL, 1))
		return 0;
	skip = (value == 0);
	rgd3_parser_push_indent(p, INDENT_ELIF, skip);
	return 1;
}

static int rgd3_parser_directive_if(Rgd3Parser* p)
{
	i32 value;
	int skip;

	if (!rgd3_parser_evaluate(p, &value, NULL, 1))
		return 0;
	skip = (value == 0);
	rgd3_parser_push_indent(p, INDENT_IF, skip);
	return 1;
}

static int rgd3_parser_directive_line(Rgd3Parser* p)
{
	Rgd3Token token = { 0 };

	rgd3_parser_error(p, "#line directive not supported");
	while (rgd3_parser_read_line(p, &token))
	{
	}
	rgd3_token_free(&token);
	return 1;
}

static int rgd3_parser_directive_error(Rgd3Parser* p)
{
	Rgd3Token token = { 0 };
	int result = 0;

	if (!rgd3_parser_read_line(p, &token) || token.type != TT_STRING)
	{
		rgd3_parser_error(p, "#error without string");
		goto out;
	}
	rgd3_parser_error(p, "#error: %s", rgd3_token_cstr(&token));
	result = 1;
out:
	rgd3_token_free(&token);
	return result;
}

static int rgd3_parser_directive_warning(Rgd3Parser* p)
{
	Rgd3Token token = { 0 };
	int result = 0;

	if (!rgd3_parser_read_line(p, &token) || token.type != TT_STRING)
	{
		rgd3_parser_warning(p, "#warning without string");
		goto out;
	}
	rgd3_parser_warning(p, "#warning: %s", rgd3_token_cstr(&token));
	result = 1;
out:
	rgd3_token_free(&token);
	return result;
}

static int rgd3_parser_directive_pragma(Rgd3Parser* p)
{
	Rgd3Token token = { 0 };

	rgd3_parser_warning(p, "#pragma directive not supported");
	while (rgd3_parser_read_line(p, &token))
	{
	}
	rgd3_token_free(&token);
	return 1;
}

static void rgd3_parser_unread_sign_token(Rgd3Parser* p)
{
	Rgd3Token token = { 0 };

	token.line = rgd3_lexer_line_num(p->scriptstack);
	token.white_space_start_p = NULL;
	token.white_space_end_p = NULL;
	token.lines_crossed = 0;
	token.flags = 0;
	rgd3_token_set(&token, "-");
	token.type = TT_PUNCTUATION;
	token.subtype = P_SUB;
	rgd3_parser_unread_source_token(p, &token);
	rgd3_token_free(&token);
}

/* pushes |value| (and a '-' in front when negative) back into the source */
static void rgd3_parser_unread_value(Rgd3Parser* p, const char* text, int subtype, int set_values, u32 intvalue, double floatvalue, int negative)
{
	Rgd3Token token = { 0 };

	token.line = rgd3_lexer_line_num(p->scriptstack);
	token.white_space_start_p = NULL;
	token.white_space_end_p = NULL;
	token.lines_crossed = 0;
	token.flags = 0;
	rgd3_token_set(&token, text);
	token.type = TT_NUMBER;
	token.subtype = subtype;
	if (set_values)
	{
		token.intvalue = intvalue;
		token.floatvalue = floatvalue;
	}
	rgd3_parser_unread_source_token(p, &token);
	if (negative)
		rgd3_parser_unread_sign_token(p);
	rgd3_token_free(&token);
}

static u32 rgd3_parser_abs(i32 value)
{
	/* abs(INT_MIN) stays INT_MIN, as with the CRT */
	return value < 0 ? 0u - (u32)value : (u32)value;
}

static int rgd3_parser_directive_eval(Rgd3Parser* p)
{
	i32 value;
	char buf[128];

	if (!rgd3_parser_evaluate(p, &value, NULL, 1))
		return 0;
	rg_snprintf(buf, sizeof(buf), "%d", (int)rgd3_parser_abs(value));
	rgd3_parser_unread_value(p, buf, TT_INTEGER | TT_LONG | TT_DECIMAL, 0, 0, 0, value < 0);
	return 1;
}

static int rgd3_parser_directive_evalfloat(Rgd3Parser* p)
{
	double value;
	char buf[128];

	if (!rgd3_parser_evaluate(p, NULL, &value, 0))
		return 0;
	rg_snprintf(buf, sizeof(buf), "%1.2f", fabs(value));
	rgd3_parser_unread_value(p, buf, TT_FLOAT | TT_LONG | TT_DECIMAL, 0, 0, 0, value < 0);
	return 1;
}

static int rgd3_parser_read_directive(Rgd3Parser* p)
{
	Rgd3Token token = { 0 };
	int result = 0;

	/* the directive name */
	if (!rgd3_parser_read_source_token(p, &token))
	{
		rgd3_parser_error(p, "found '#' without name");
		goto out;
	}
	/* must be on the same line */
	if (token.lines_crossed > 0)
	{
		rgd3_parser_unread_source_token(p, &token);
		rgd3_parser_error(p, "found '#' at end of line");
		goto out;
	}
	if (token.type == TT_NAME)
	{
		if (rgd3_token_is(&token, "if"))
		{
			result = rgd3_parser_directive_if(p);
			goto out;
		}
		else if (rgd3_token_is(&token, "ifdef"))
		{
			result = rgd3_parser_directive_if_def(p, INDENT_IFDEF);
			goto out;
		}
		else if (rgd3_token_is(&token, "ifndef"))
		{
			result = rgd3_parser_directive_if_def(p, INDENT_IFNDEF);
			goto out;
		}
		else if (rgd3_token_is(&token, "elif"))
		{
			result = rgd3_parser_directive_elif(p);
			goto out;
		}
		else if (rgd3_token_is(&token, "else"))
		{
			result = rgd3_parser_directive_else(p);
			goto out;
		}
		else if (rgd3_token_is(&token, "endif"))
		{
			result = rgd3_parser_directive_endif(p);
			goto out;
		}
		else if (p->skip > 0)
		{
			/* skip the rest of the line */
			while (rgd3_parser_read_line(p, &token))
			{
			}
			result = 1;
			goto out;
		}
		else
		{
			if (rgd3_token_is(&token, "include"))
			{
				result = rgd3_parser_directive_include(p);
				goto out;
			}
			else if (rgd3_token_is(&token, "define"))
			{
				result = rgd3_parser_directive_define(p);
				goto out;
			}
			else if (rgd3_token_is(&token, "undef"))
			{
				result = rgd3_parser_directive_undef(p);
				goto out;
			}
			else if (rgd3_token_is(&token, "line"))
			{
				result = rgd3_parser_directive_line(p);
				goto out;
			}
			else if (rgd3_token_is(&token, "error"))
			{
				result = rgd3_parser_directive_error(p);
				goto out;
			}
			else if (rgd3_token_is(&token, "warning"))
			{
				result = rgd3_parser_directive_warning(p);
				goto out;
			}
			else if (rgd3_token_is(&token, "pragma"))
			{
				result = rgd3_parser_directive_pragma(p);
				goto out;
			}
			else if (rgd3_token_is(&token, "eval"))
			{
				result = rgd3_parser_directive_eval(p);
				goto out;
			}
			else if (rgd3_token_is(&token, "evalfloat"))
			{
				result = rgd3_parser_directive_evalfloat(p);
				goto out;
			}
		}
	}
	rgd3_parser_error(p, "unknown precompiler directive '%s'", rgd3_token_cstr(&token));
out:
	rgd3_token_free(&token);
	return result;
}

static int rgd3_parser_dollar_directive_evalint(Rgd3Parser* p)
{
	i32 value;
	u32 mag;
	char buf[128];

	if (!rgd3_parser_dollar_evaluate(p, &value, NULL, 1))
		return 0;
	mag = rgd3_parser_abs(value);
	rg_snprintf(buf, sizeof(buf), "%d", (int)mag);
	rgd3_parser_unread_value(p, buf, TT_INTEGER | TT_LONG | TT_DECIMAL | TT_VALUESVALID, 1, mag, (double)(int)mag, value < 0);
	return 1;
}

static int rgd3_parser_dollar_directive_evalfloat(Rgd3Parser* p)
{
	double value;
	char buf[128];

	if (!rgd3_parser_dollar_evaluate(p, NULL, &value, 0))
		return 0;
	rg_snprintf(buf, sizeof(buf), "%1.2f", fabs(value));
	rgd3_parser_unread_value(p, buf, TT_FLOAT | TT_LONG | TT_DECIMAL | TT_VALUESVALID, 1, (u32)fabs(value), fabs(value), value < 0);
	return 1;
}

static int rgd3_parser_read_dollar_directive(Rgd3Parser* p)
{
	Rgd3Token token = { 0 };
	int result = 0;

	/* the directive name */
	if (!rgd3_parser_read_source_token(p, &token))
	{
		rgd3_parser_error(p, "found '$' without name");
		goto out;
	}
	/* must be on the same line */
	if (token.lines_crossed > 0)
	{
		rgd3_parser_unread_source_token(p, &token);
		rgd3_parser_error(p, "found '$' at end of line");
		goto out;
	}
	if (token.type == TT_NAME)
	{
		if (rgd3_token_is(&token, "evalint"))
		{
			result = rgd3_parser_dollar_directive_evalint(p);
			goto out;
		}
		else if (rgd3_token_is(&token, "evalfloat"))
		{
			result = rgd3_parser_dollar_directive_evalfloat(p);
			goto out;
		}
	}
	rgd3_parser_unread_source_token(p, &token);
out:
	rgd3_token_free(&token);
	return result;
}

/* ---- public read API ---- */

int rgd3_parser_read_token(Rgd3Parser* p, Rgd3Token* token)
{
	Rgd3Define* define;
	const char* text;

	for (;;)
	{
		if (!rgd3_parser_read_source_token(p, token))
			return 0;
		text = rgd3_token_cstr(token);
		/* precompiler directives */
		if (token->type == TT_PUNCTUATION && text[0] == '#' && text[1] == '\0')
		{
			if (!rgd3_parser_read_directive(p))
				return 0;
			continue;
		}
		/* skipping because of conditional compilation */
		if (p->skip)
			continue;
		/* concatenate consecutive strings, still resolving defines */
		if (token->type == TT_STRING && !(rgd3_lexer_get_flags(p->scriptstack) & LEXFL_NOSTRINGCONCAT))
		{
			Rgd3Token newtoken = { 0 };
			if (rgd3_parser_read_token(p, &newtoken))
			{
				if (newtoken.type == TT_STRING)
					rgd3_token_append(token, rgd3_token_cstr(&newtoken));
				else
					rgd3_parser_unread_source_token(p, &newtoken);
			}
			rgd3_token_free(&newtoken);
			text = rgd3_token_cstr(token);
		}
		if (!(rgd3_lexer_get_flags(p->scriptstack) & LEXFL_NODOLLARPRECOMPILE))
		{
			/* $evalint / $evalfloat */
			if (token->type == TT_PUNCTUATION && text[0] == '$' && text[1] == '\0')
			{
				if (rgd3_parser_read_dollar_directive(p))
					continue;
			}
		}
		if (token->type == TT_NAME && !(token->flags & RGD3_TOKEN_FL_RECURSIVE_DEFINE))
		{
			define = rgd3_parser_find_hashed_define(p->definehash, rgd3_token_cstr(token));
			if (define)
			{
				if (!rgd3_parser_expand_define_into_source(p, token, define))
					return 0;
				continue;
			}
		}
		return 1;
	}
}

int rgd3_parser_expect_token_string(Rgd3Parser* p, const char* string)
{
	Rgd3Token token = { 0 };
	int result = 0;

	if (!rgd3_parser_read_token(p, &token))
		rgd3_parser_error(p, "couldn't find expected '%s'", string);
	else if (!rgd3_token_is(&token, string))
		rgd3_parser_error(p, "expected '%s' but found '%s'", string, rgd3_token_cstr(&token));
	else
		result = 1;
	rgd3_token_free(&token);
	return result;
}

int rgd3_parser_expect_token_type(Rgd3Parser* p, int type, int subtype, Rgd3Token* token)
{
	char str[128];

	if (!rgd3_parser_read_token(p, token))
	{
		rgd3_parser_error(p, "couldn't read expected token");
		return 0;
	}
	if (token->type != type)
	{
		const char* name;
		switch (type)
		{
		case TT_STRING: name = "string"; break;
		case TT_LITERAL: name = "literal"; break;
		case TT_NUMBER: name = "number"; break;
		case TT_NAME: name = "name"; break;
		case TT_PUNCTUATION: name = "punctuation"; break;
		default: name = "unknown type"; break;
		}
		rgd3_parser_error(p, "expected a %s but found '%s'", name, rgd3_token_cstr(token));
		return 0;
	}
	if (token->type == TT_NUMBER)
	{
		if ((token->subtype & subtype) != subtype)
		{
			size_t len;
			str[0] = 0;
			if (subtype & TT_DECIMAL)
				rgd3_str_copynz(str, "decimal ", sizeof(str));
			if (subtype & TT_HEX)
				rgd3_str_copynz(str, "hex ", sizeof(str));
			if (subtype & TT_OCTAL)
				rgd3_str_copynz(str, "octal ", sizeof(str));
			if (subtype & TT_BINARY)
				rgd3_str_copynz(str, "binary ", sizeof(str));
			if (subtype & TT_UNSIGNED)
				rgd3_str_append(str, sizeof(str), "unsigned ");
			if (subtype & TT_LONG)
				rgd3_str_append(str, sizeof(str), "long ");
			if (subtype & TT_FLOAT)
				rgd3_str_append(str, sizeof(str), "float ");
			if (subtype & TT_INTEGER)
				rgd3_str_append(str, sizeof(str), "integer ");
			len = strlen(str);
			while (len > 0 && str[len - 1] == ' ')
				str[--len] = 0;
			rgd3_parser_error(p, "expected %s but found '%s'", str, rgd3_token_cstr(token));
			return 0;
		}
	}
	else if (token->type == TT_PUNCTUATION)
	{
		if (subtype < 0)
		{
			rgd3_parser_error(p, "BUG: wrong punctuation subtype");
			return 0;
		}
		if (token->subtype != subtype)
		{
			rgd3_parser_error(p, "expected '%s' but found '%s'", rgd3_lexer_get_punctuation_from_id(p->scriptstack, subtype), rgd3_token_cstr(token));
			return 0;
		}
	}
	return 1;
}

int rgd3_parser_expect_any_token(Rgd3Parser* p, Rgd3Token* token)
{
	if (!rgd3_parser_read_token(p, token))
	{
		rgd3_parser_error(p, "couldn't read expected token");
		return 0;
	}
	return 1;
}

int rgd3_parser_check_token_string(Rgd3Parser* p, const char* string)
{
	Rgd3Token tok = { 0 };
	int found = 0;

	if (rgd3_parser_read_token(p, &tok))
	{
		if (rgd3_token_is(&tok, string))
			found = 1;
		else
			rgd3_parser_unread_source_token(p, &tok);
	}
	rgd3_token_free(&tok);
	return found;
}

int rgd3_parser_check_token_type(Rgd3Parser* p, int type, int subtype, Rgd3Token* token)
{
	Rgd3Token tok = { 0 };
	int found = 0;

	if (rgd3_parser_read_token(p, &tok))
	{
		if (tok.type == type && (tok.subtype & subtype) == subtype)
		{
			rgd3_token_copy(token, &tok);
			found = 1;
		}
		else
		{
			rgd3_parser_unread_source_token(p, &tok);
		}
	}
	rgd3_token_free(&tok);
	return found;
}

int rgd3_parser_peek_token_string(Rgd3Parser* p, const char* string)
{
	Rgd3Token tok = { 0 };
	int found = 0;

	if (rgd3_parser_read_token(p, &tok))
	{
		rgd3_parser_unread_source_token(p, &tok);
		found = rgd3_token_is(&tok, string);
	}
	rgd3_token_free(&tok);
	return found;
}

int rgd3_parser_peek_token_type(Rgd3Parser* p, int type, int subtype, Rgd3Token* token)
{
	Rgd3Token tok = { 0 };
	int found = 0;

	if (rgd3_parser_read_token(p, &tok))
	{
		rgd3_parser_unread_source_token(p, &tok);
		if (tok.type == type && (tok.subtype & subtype) == subtype)
		{
			rgd3_token_copy(token, &tok);
			found = 1;
		}
	}
	rgd3_token_free(&tok);
	return found;
}

int rgd3_parser_skip_until_string(Rgd3Parser* p, const char* string)
{
	Rgd3Token token = { 0 };
	int found = 0;

	while (rgd3_parser_read_token(p, &token))
	{
		if (rgd3_token_is(&token, string))
		{
			found = 1;
			break;
		}
	}
	rgd3_token_free(&token);
	return found;
}

int rgd3_parser_skip_rest_of_line(Rgd3Parser* p)
{
	Rgd3Token token = { 0 };
	int found = 0;

	while (rgd3_parser_read_token(p, &token))
	{
		if (token.lines_crossed)
		{
			rgd3_parser_unread_source_token(p, &token);
			found = 1;
			break;
		}
	}
	rgd3_token_free(&token);
	return found;
}

/* Skips until the matching close brace; inner braces are counted. */
int rgd3_parser_skip_braced_section(Rgd3Parser* p, int parse_first_brace)
{
	Rgd3Token token = { 0 };
	int depth = parse_first_brace ? 0 : 1;
	int result = 1;

	do
	{
		if (!rgd3_parser_read_token(p, &token))
		{
			result = 0;
			break;
		}
		if (token.type == TT_PUNCTUATION)
		{
			if (rgd3_token_is(&token, "{"))
				depth++;
			else if (rgd3_token_is(&token, "}"))
				depth--;
		}
	} while (depth);
	rgd3_token_free(&token);
	return result;
}

/* Exact characters, straight from the current script (no precompilation,
   as in the original). */
const char* rgd3_parser_parse_braced_section_exact(Rgd3Parser* p, Rgd3StrBuf* out, int tabs)
{
	return rgd3_lexer_parse_braced_section_exact(p->scriptstack, out, tabs);
}

/* Re-emits tokens up to the matching outro, separated by spaces, strings
   and literals re-quoted, "\r\n" per crossed line and, with tabs >= 0,
   re-indented. */
const char* rgd3_parser_parse_braced_section(Rgd3Parser* p, Rgd3StrBuf* out, int tabs, int parse_first_brace, char intro, char outro)
{
	Rgd3Token token = { 0 };
	int i, depth, do_tabs;
	const char* text;
	char temp[2];

	temp[0] = intro;
	temp[1] = 0;
	rgd3_strbuf_clear(out);
	if (parse_first_brace)
	{
		if (!rgd3_parser_expect_token_string(p, temp))
			return rgd3_strbuf_cstr(out);
		rgd3_strbuf_set(out, temp);
	}
	depth = 1;
	do_tabs = tabs >= 0;
	do
	{
		if (!rgd3_parser_read_token(p, &token))
		{
			rgd3_parser_error(p, "missing closing brace");
			break;
		}
		text = rgd3_token_cstr(&token);
		/* the token is on a new line */
		for (i = 0; i < token.lines_crossed; i++)
			rgd3_strbuf_append(out, "\r\n");
		if (do_tabs && token.lines_crossed)
		{
			i = tabs;
			if (text[0] == outro && i > 0)
				i--;
			while (i-- > 0)
				rgd3_strbuf_appendc(out, '\t');
		}
		if (token.type == TT_STRING)
		{
			rgd3_strbuf_appendc(out, '\"');
			rgd3_strbuf_append(out, text);
			rgd3_strbuf_appendc(out, '\"');
		}
		else if (token.type == TT_LITERAL)
		{
			rgd3_strbuf_appendc(out, '\'');
			rgd3_strbuf_append(out, text);
			rgd3_strbuf_appendc(out, '\'');
		}
		else
		{
			if (text[0] == intro)
			{
				depth++;
				if (do_tabs)
					tabs++;
			}
			else if (text[0] == outro)
			{
				depth--;
				if (do_tabs)
					tabs--;
			}
			rgd3_strbuf_append(out, text);
		}
		rgd3_strbuf_appendc(out, ' ');
	} while (depth);
	rgd3_token_free(&token);
	return rgd3_strbuf_cstr(out);
}

const char* rgd3_parser_parse_rest_of_line(Rgd3Parser* p, Rgd3StrBuf* out)
{
	Rgd3Token token = { 0 };

	rgd3_strbuf_clear(out);
	while (rgd3_parser_read_token(p, &token))
	{
		if (token.lines_crossed)
		{
			rgd3_parser_unread_source_token(p, &token);
			break;
		}
		if (out->len)
			rgd3_strbuf_appendc(out, ' ');
		rgd3_strbuf_append(out, rgd3_token_cstr(&token));
	}
	rgd3_token_free(&token);
	return rgd3_strbuf_cstr(out);
}

void rgd3_parser_unread_token(Rgd3Parser* p, const Rgd3Token* token)
{
	rgd3_parser_unread_source_token(p, token);
}

int rgd3_parser_read_token_on_line(Rgd3Parser* p, Rgd3Token* token)
{
	Rgd3Token tok = { 0 };
	int found = 0;

	if (rgd3_parser_read_token(p, &tok))
	{
		/* no lines crossed before the token */
		if (!tok.lines_crossed)
		{
			rgd3_token_copy(token, &tok);
			found = 1;
		}
		else
		{
			rgd3_parser_unread_source_token(p, &tok);
		}
	}
	rgd3_token_free(&tok);
	return found;
}

int rgd3_parser_parse_int(Rgd3Parser* p)
{
	Rgd3Token token = { 0 };
	int value;

	if (!rgd3_parser_read_token(p, &token))
	{
		rgd3_parser_error(p, "couldn't read expected integer");
		rgd3_token_free(&token);
		return 0;
	}
	if (token.type == TT_PUNCTUATION && rgd3_token_is(&token, "-"))
	{
		rgd3_parser_expect_token_type(p, TT_NUMBER, TT_INTEGER, &token);
		value = -rgd3_token_int_value(&token);
		rgd3_token_free(&token);
		return value;
	}
	else if (token.type != TT_NUMBER || token.subtype == TT_FLOAT)
	{
		rgd3_parser_error(p, "expected integer value, found '%s'", rgd3_token_cstr(&token));
	}
	value = rgd3_token_int_value(&token);
	rgd3_token_free(&token);
	return value;
}

int rgd3_parser_parse_bool(Rgd3Parser* p)
{
	Rgd3Token token = { 0 };
	int value;

	if (!rgd3_parser_expect_token_type(p, TT_NUMBER, 0, &token))
	{
		rgd3_parser_error(p, "couldn't read expected boolean");
		rgd3_token_free(&token);
		return 0;
	}
	value = rgd3_token_int_value(&token) != 0;
	rgd3_token_free(&token);
	return value;
}

float rgd3_parser_parse_float(Rgd3Parser* p)
{
	Rgd3Token token = { 0 };
	float value;

	if (!rgd3_parser_read_token(p, &token))
	{
		rgd3_parser_error(p, "couldn't read expected floating point number");
		rgd3_token_free(&token);
		return 0.0f;
	}
	if (token.type == TT_PUNCTUATION && rgd3_token_is(&token, "-"))
	{
		rgd3_parser_expect_token_type(p, TT_NUMBER, 0, &token);
		value = -rgd3_token_float_value(&token);
		rgd3_token_free(&token);
		return value;
	}
	else if (token.type != TT_NUMBER)
	{
		rgd3_parser_error(p, "expected float value, found '%s'", rgd3_token_cstr(&token));
	}
	value = rgd3_token_float_value(&token);
	rgd3_token_free(&token);
	return value;
}

int rgd3_parser_parse_1d_matrix(Rgd3Parser* p, int x, float* m)
{
	int i;
	if (!rgd3_parser_expect_token_string(p, "("))
		return 0;
	for (i = 0; i < x; i++)
		m[i] = rgd3_parser_parse_float(p);
	if (!rgd3_parser_expect_token_string(p, ")"))
		return 0;
	return 1;
}

int rgd3_parser_parse_2d_matrix(Rgd3Parser* p, int y, int x, float* m)
{
	int i;
	if (!rgd3_parser_expect_token_string(p, "("))
		return 0;
	for (i = 0; i < y; i++)
	{
		if (!rgd3_parser_parse_1d_matrix(p, x, m + i * x))
			return 0;
	}
	if (!rgd3_parser_expect_token_string(p, ")"))
		return 0;
	return 1;
}

int rgd3_parser_parse_3d_matrix(Rgd3Parser* p, int z, int y, int x, float* m)
{
	int i;
	if (!rgd3_parser_expect_token_string(p, "("))
		return 0;
	for (i = 0; i < z; i++)
	{
		if (!rgd3_parser_parse_2d_matrix(p, y, x, m + i * x * y))
			return 0;
	}
	if (!rgd3_parser_expect_token_string(p, ")"))
		return 0;
	return 1;
}

int rgd3_parser_get_last_white_space(const Rgd3Parser* p, Rgd3StrBuf* out)
{
	if (p->scriptstack)
		rgd3_lexer_get_last_white_space(p->scriptstack, out);
	else
		rgd3_strbuf_clear(out);
	return out->len;
}

void rgd3_parser_set_marker(Rgd3Parser* p)
{
	p->marker_p = NULL;
}

/* The source text from the marker (the first token read after SetMarker)
   up to the current position. With clean, the text is re-parsed and the
   token texts are appended to out without separators; otherwise out is
   replaced by the raw text. */
void rgd3_parser_get_string_from_marker(Rgd3Parser* p, Rgd3StrBuf* out, int clean)
{
	const Rgd3Lexer* script = p->scriptstack;
	const char* start;
	const char* end;
	size_t len;
	char* text;

	if (!script)
		return;
	if (!p->marker_p)
		p->marker_p = script->buffer;
	start = p->marker_p;
	end = p->tokens ? p->tokens->white_space_start_p : script->script_p;
	if (end && end >= start && start >= script->buffer && end <= script->end_p)
		len = (size_t)(end - start);
	else
		len = strlen(start); /* the end is not in the marker's buffer */

	rgd3_parser_release_marker_scratch();
	text = (char*)rgd3_heap_alloc(len + 1, RGD3_TAG_PARSER);
	memcpy(text, start, len);
	text[len] = 0;
	rgd3_parser_marker_text = text;

	if (clean)
	{
		Rgd3Parser* temp = &rgd3_parser_marker_src;
		Rgd3Token token = { 0 };
		rgd3_parser_init(temp, p->flags);
		rgd3_parser_load_memory(temp, text, (int)len, "temp");
		while (rgd3_parser_read_token(temp, &token))
			rgd3_strbuf_append(out, rgd3_token_cstr(&token));
		rgd3_token_free(&token);
	}
	else
	{
		rgd3_strbuf_set(out, text);
	}
	rgd3_parser_release_marker_scratch();
}

void rgd3_parser_set_include_path(Rgd3Parser* p, const char* path)
{
	size_t len;

	rgd3_str_copynz(p->includepath, path, sizeof(p->includepath));
	len = strlen(p->includepath);
	/* trailing separator; '/' is the rgd3 file system separator */
	if (len > 0 && p->includepath[len - 1] != '\\' && p->includepath[len - 1] != '/')
		rgd3_str_append(p->includepath, sizeof(p->includepath), "/");
}

void rgd3_parser_set_punctuations(Rgd3Parser* p, const Rgd3Punctuation* punctuations)
{
	p->punctuations = punctuations;
}

const char* rgd3_parser_get_punctuation_from_id(const Rgd3Parser* p, int id)
{
	const Rgd3Punctuation* punctuations = p->punctuations ? p->punctuations : rgd3_default_punctuations;
	int i;

	for (i = 0; punctuations[i].p; i++)
	{
		if (punctuations[i].n == id)
			return punctuations[i].p;
	}
	return "unkown punctuation";
}

int rgd3_parser_get_punctuation_id(const Rgd3Parser* p, const char* punctuation)
{
	const Rgd3Punctuation* punctuations = p->punctuations ? p->punctuations : rgd3_default_punctuations;
	int i;

	for (i = 0; punctuations[i].p; i++)
	{
		if (!strcmp(punctuations[i].p, punctuation))
			return punctuations[i].n;
	}
	return 0;
}

void rgd3_parser_set_flags(Rgd3Parser* p, int flags)
{
	Rgd3Lexer* s;

	p->flags = flags;
	for (s = p->scriptstack; s; s = s->next)
		rgd3_lexer_set_flags(s, flags);
}

int rgd3_parser_get_flags(const Rgd3Parser* p)
{
	return p->flags;
}

const char* rgd3_parser_get_file_name(const Rgd3Parser* p)
{
	return p->scriptstack ? rgd3_lexer_file_name(p->scriptstack) : "";
}

int rgd3_parser_get_file_offset(const Rgd3Parser* p)
{
	return p->scriptstack ? rgd3_lexer_file_offset(p->scriptstack) : 0;
}

Rgd3TimeStamp rgd3_parser_get_file_time(const Rgd3Parser* p)
{
	return p->scriptstack ? rgd3_lexer_file_time(p->scriptstack) : 0;
}

int rgd3_parser_get_line_num(const Rgd3Parser* p)
{
	return p->scriptstack ? rgd3_lexer_line_num(p->scriptstack) : 0;
}

int rgd3_parser_end_of_file(const Rgd3Parser* p)
{
	return p->scriptstack ? rgd3_lexer_end_of_file(p->scriptstack) : 1;
}

/* ---- lifetime ---- */

/* a parser on the stack is freed if a drop unwinds its frame (rgd3_error.h) */
static void rgd3_parser_unwind(void* object)
{
	rgd3_parser_free((Rgd3Parser*)object);
}

void rgd3_parser_init(Rgd3Parser* p, int flags)
{
	memset(p, 0, sizeof(*p));
	p->flags = flags;
	rgd3_error_unwind_register(p, rgd3_parser_unwind);
}

static void rgd3_parser_start_source(Rgd3Parser* p, Rgd3Lexer* script, const char* name)
{
	rgd3_lexer_set_flags(script, p->flags);
	rgd3_lexer_set_punctuations(script, p->punctuations);
	script->next = NULL;
	rgd3_str_copynz(p->filename, name ? name : "", sizeof(p->filename));
	rgd3_error_unwind_register(p, rgd3_parser_unwind); /* zero-initialised parsers */
	p->scriptstack = script;
	p->tokens = NULL;
	p->indentstack = NULL;
	p->skip = 0;
	p->loaded = 1;
	rgd3_parser_ensure_define_hash(p);
}

int rgd3_parser_load_file(Rgd3Parser* p, const char* filename, int os_path)
{
	Rgd3Lexer* script;

	if (p->loaded)
		rgd3_error_fatal("rgd3_parser_load_file: another source already loaded");
	script = rgd3_parser_new_lexer();
	if (!rgd3_lexer_load_file(script, filename, os_path))
	{
		rgd3_parser_delete_lexer(script);
		return 0;
	}
	p->os_path = os_path ? 1 : 0;
	rgd3_parser_start_source(p, script, filename);
	return 1;
}

int rgd3_parser_load_memory(Rgd3Parser* p, const char* ptr, int length, const char* name)
{
	Rgd3Lexer* script;

	if (p->loaded)
		rgd3_error_fatal("rgd3_parser_load_memory: another source already loaded");
	script = rgd3_parser_new_lexer();
	if (!rgd3_lexer_load_memory(script, ptr, length, name, 1))
	{
		rgd3_parser_delete_lexer(script);
		return 0;
	}
	rgd3_parser_start_source(p, script, name);
	return 1;
}

void rgd3_parser_free_source(Rgd3Parser* p, int keep_defines)
{
	Rgd3Lexer* script;
	Rgd3Indent* indent;
	Rgd3Define* define;
	int i;

	while (p->scriptstack)
	{
		script = p->scriptstack;
		p->scriptstack = script->next;
		rgd3_parser_delete_lexer(script);
	}
	/* the read-first tokens and anything a dropped error left in flight */
	while (p->live)
		rgd3_parser_delete_token(p, &p->live->token);
	p->tokens = NULL;
	while (p->indentstack)
	{
		indent = p->indentstack;
		p->indentstack = indent->next;
		rgd3_heap_free(indent);
	}
	p->skip = 0;
	if (!keep_defines && p->definehash)
	{
		for (i = 0; i < RGD3_DEFINEHASHSIZE; i++)
		{
			while (p->definehash[i])
			{
				define = p->definehash[i];
				p->definehash[i] = define->hashnext;
				rgd3_parser_free_define(define);
			}
		}
		p->defines = NULL;
		rgd3_heap_free(p->definehash);
		p->definehash = NULL;
	}
	p->loaded = 0;
}

void rgd3_parser_free(Rgd3Parser* p)
{
	rgd3_parser_free_source(p, 0);
	rgd3_error_unwind_unregister(p);
}

int rgd3_parser_is_loaded(const Rgd3Parser* p)
{
	return p->loaded;
}

/* ---- self test ---- */

typedef struct Rgd3ParserTest
{
	int checks;
	int failures;
} Rgd3ParserTest;

static void rgd3_parser_test_check(Rgd3ParserTest* t, int ok, const char* what)
{
	t->checks++;
	if (!ok)
	{
		t->failures++;
		rgd3_printf("parser test FAILED: %s\n", what);
	}
}

static int rgd3_parser_test_type_char(int type)
{
	switch (type)
	{
	case TT_STRING: return 's';
	case TT_LITERAL: return 'l';
	case TT_NUMBER: return 'n';
	case TT_NAME: return 'w';
	case TT_PUNCTUATION: return 'p';
	}
	return '?';
}

/* expected: NULL-terminated; each entry is a type character (s string,
   l literal, n number, w name, p punctuation, * any) and the text */
static int rgd3_parser_test_tokens(Rgd3ParserTest* t, Rgd3Parser* p, const char* what, const char* const* expected)
{
	Rgd3Token tok = { 0 };
	int i = 0, ok = 1;

	while (expected[i])
	{
		if (!rgd3_parser_read_token(p, &tok))
		{
			rgd3_printf("  %s: ended before token %d '%s'\n", what, i, expected[i]);
			ok = 0;
			break;
		}
		if ((expected[i][0] != '*' && expected[i][0] != rgd3_parser_test_type_char(tok.type)) || !rgd3_token_is(&tok, expected[i] + 1))
		{
			rgd3_printf("  %s: token %d is %c'%s', expected '%s'\n", what, i, rgd3_parser_test_type_char(tok.type), rgd3_token_cstr(&tok), expected[i]);
			ok = 0;
			break;
		}
		i++;
	}
	rgd3_token_free(&tok);
	rgd3_parser_test_check(t, ok, what);
	return ok;
}

/* the whole source must produce exactly the expected tokens */
static void rgd3_parser_test_stream(Rgd3ParserTest* t, const char* what, const char* src, int flags, const char* const* expected)
{
	Rgd3Parser p;
	Rgd3Token tok = { 0 };

	rgd3_parser_init(&p, flags);
	rgd3_parser_test_check(t, rgd3_parser_load_memory(&p, src, (int)strlen(src), what), what);
	if (rgd3_parser_test_tokens(t, &p, what, expected))
	{
		int extra = rgd3_parser_read_token(&p, &tok);
		if (extra)
			rgd3_printf("  %s: unexpected trailing token '%s'\n", what, rgd3_token_cstr(&tok));
		rgd3_parser_test_check(t, !extra, what);
	}
	rgd3_token_free(&tok);
	rgd3_parser_free(&p);
}

static void rgd3_parser_test_macros(Rgd3ParserTest* t)
{
	static const char* const objects[] = {
		"wx", "p=", "n1", "p;", "wy", "p=", "p(", "p(", "n1", "p)", "p+", "p(", "n2", "p*", "n3", "p)", "p)", "p;", "wz", NULL
	};
	static const char* const merging[] = {
		"wfoobar", "wname12", "shelloworld", "p;", "sabcd", "p;", "n1234", "sabcd", "p;", "s<p>", NULL
	};
	static const char* const redefines[] = {
		"wR", "p+", "n1", "wU", "n2", "n9", "p(", "p)", NULL
	};
	Rgd3Parser p;
	Rgd3Token tok = { 0 };
	const char* src;

	rgd3_parser_test_stream(t, "object-like and function-like macros",
		"#define ONE 1\n"
		"#define ADD(a, b) ((a) + (b))\n"
		"#define EMPTY\n"
		"x = ONE; y = ADD(ONE, 2 * 3); EMPTY z\n",
		0, objects);

	rgd3_parser_test_stream(t, "## merging and # stringizing",
		"#define CAT(a, b) a ## b\n"
		"#define STR(x) #x\n"
		"#define TAG(x) \"<\" #x \">\"\n"
		"CAT(foo, bar) CAT(name, 12) STR(hello world) ;\n"
		"STR(ab) \"cd\" ; CAT(12, 34) CAT(\"ab\", \"cd\") ; TAG(p)\n",
		0, merging);

	/* recursive define, #undef, redefinition; an empty parameter list makes
	   an object-like define and leaves the "()" in the source */
	rgd3_parser_test_stream(t, "recursion, #undef and redefinition",
		"#define R R + 1\n"
		"R\n"
		"#define U 1\n"
		"#undef U\n"
		"U\n"
		"#define V 1\n"
		"#define V 2\n"
		"V\n"
		"#define F() 9\n"
		"F()\n",
		LEXFL_NOWARNINGS, redefines);

	/* a merged number gets its value computed from the merged text */
	src = "#define CAT(a, b) a##b\nCAT(12, 34)\n";
	rgd3_parser_init(&p, 0);
	rgd3_parser_load_memory(&p, src, (int)strlen(src), "merged number");
	rgd3_parser_test_check(t, rgd3_parser_read_token(&p, &tok) && tok.type == TT_NUMBER && rgd3_token_int_value(&tok) == 1234, "merged number value");
	rgd3_parser_free(&p);

	/* builtins */
	/* builtins; __DATE__ and __TIME__ keep the quotes inside the string */
	src = "a\n__LINE__ __FILE__ __STDC__ __DATE__ ;\n__TIME__\n#undef __LINE__\n__LINE__";
	rgd3_parser_init(&p, LEXFL_NOWARNINGS);
	rgd3_parser_load_memory(&p, src, (int)strlen(src), "builtins.src");
	rgd3_parser_add_builtin_defines(&p);
	rgd3_parser_read_token(&p, &tok);
	rgd3_parser_test_check(t, rgd3_parser_read_token(&p, &tok) && tok.type == TT_NUMBER && rgd3_token_is(&tok, "2") && rgd3_token_int_value(&tok) == 2, "__LINE__");
	rgd3_parser_test_check(t, rgd3_parser_read_token(&p, &tok) && tok.type == TT_NAME && rgd3_token_is(&tok, "builtins.src"), "__FILE__");
	/* __STDC__ expands to nothing */
	rgd3_parser_test_check(t, rgd3_parser_read_token(&p, &tok) && tok.type == TT_STRING && tok.len == 13 && rgd3_token_cstr(&tok)[0] == '\"', "__DATE__");
	rgd3_parser_read_token(&p, &tok);
	rgd3_parser_test_check(t, rgd3_parser_read_token(&p, &tok) && tok.type == TT_STRING && tok.len == 10 && rgd3_token_cstr(&tok)[3] == ':', "__TIME__");
	rgd3_parser_test_check(t, rgd3_parser_read_token(&p, &tok) && rgd3_token_is(&tok, "5"), "#undef of a fixed define");
	rgd3_parser_free(&p);
	rgd3_token_free(&tok);
}

static void rgd3_parser_test_conditionals(Rgd3ParserTest* t)
{
	static const char* const expected[] = { "wa", "wd", "wf", "wg", "wh", "wj", NULL };
	static const char* const evals[] = {
		"wx", "p=", "n14", "p;", "wy", "p=", "p-", "n4", "p;", "wz", "p=", "n4.50", "p;",
		"ww", "p=", "n12", "p;", "p-", "n2.50", "wv", "p$", "wother", NULL
	};
	Rgd3Parser p;
	Rgd3Token tok = { 0 };
	const char* src;
	i32 ival;

	rgd3_parser_test_stream(t, "nested #if / #ifdef / #elif / #else",
		"#define LEVEL 3\n"
		"#define ONE 1\n"
		"#if LEVEL > 2 && defined(ONE)\n"
		"  #if (LEVEL * 2 + 1) == 7 || 0\n"
		"    a\n"
		"  #else\n"
		"    b\n"
		"  #endif\n"
		"  #ifdef NOPE\n"
		"    c\n"
		"  #elif LEVEL % 2 == 1\n"
		"    d\n"
		"  #endif\n"
		"#else\n"
		"  e\n"
		"#endif\n"
		"#ifndef NOPE\n"
		"  f\n"
		"#endif\n"
		"#if !defined NOPE && (1 << 4) == 16 && (0x10 | 1) == 17 && ~0 == -1\n"
		"  g\n"
		"#endif\n"
		"#if 10 / 3 == 3 && -5 + 2 == -3 && (1 ? 2 : 3) == 2 && (0 ? 2 : 3) == 3\n"
		"  h\n"
		"#endif\n"
		"#if 0\n"
		"  #error \"skipped\"\n"
		"  i\n"
		"#endif\n"
		"#if 7 >= 7 && 2 <= 1 || 3 != 4 && (5 >> 1) == 2 && (6 ^ 3) == 5 && 8 - 3 * 2 == 2\n"
		"  j\n"
		"#endif\n",
		0, expected);

	rgd3_parser_test_stream(t, "$evalint / $evalfloat / #eval / #evalfloat",
		"x = $evalint(2 * (3 + 4));\n"
		"y = $evalint(1 - 5);\n"
		"z = $evalfloat(1.5 * 3);\n"
		"w =\n"
		"#eval 3 * 4\n"
		";\n"
		"#evalfloat 1 - 3.5\n"
		"v $other\n",
		0, evals);

	/* NODOLLARPRECOMPILE leaves '$' alone */
	src = "$evalint(1)";
	rgd3_parser_init(&p, LEXFL_NODOLLARPRECOMPILE);
	rgd3_parser_load_memory(&p, src, (int)strlen(src), "nodollar");
	rgd3_parser_test_check(t, rgd3_parser_read_token(&p, &tok) && rgd3_token_is(&tok, "$"), "LEXFL_NODOLLARPRECOMPILE");
	rgd3_parser_free(&p);

	/* $evalint values are valid without reparsing */
	src = "$evalint(6 * 7)";
	rgd3_parser_init(&p, 0);
	rgd3_parser_load_memory(&p, src, (int)strlen(src), "evalint value");
	rgd3_parser_test_check(t, rgd3_parser_read_token(&p, &tok) && (tok.subtype & TT_VALUESVALID) && tok.intvalue == 42 && tok.floatvalue == 42.0, "$evalint value");
	rgd3_parser_free(&p);

	/* integer-only operators are rejected for floats; errors only flag */
	src = "1 % 2";
	rgd3_parser_init(&p, LEXFL_NOERRORS);
	rgd3_parser_load_memory(&p, src, (int)strlen(src), "float operator");
	rgd3_parser_test_check(t, !rgd3_parser_evaluate(&p, NULL, NULL, 0) && rgd3_lexer_had_error(p.scriptstack), "float evaluation rejects %");
	rgd3_parser_free(&p);

	src = "(1 + 2) * 3 == 9 ? 5 : 6";
	rgd3_parser_init(&p, 0);
	rgd3_parser_load_memory(&p, src, (int)strlen(src), "evaluate");
	ival = -1;
	rgd3_parser_test_check(t, rgd3_parser_evaluate(&p, &ival, NULL, 1) && ival == 5, "evaluate ?:");
	rgd3_parser_free(&p);

	rgd3_token_free(&tok);
}

static void rgd3_parser_test_globals(Rgd3ParserTest* t)
{
	static const char* const with_globals[] = { "n42", "p(", "p(", "n3", "p)", "p*", "n2", "p)", NULL };
	static const char* const without[] = { "wGLOBAL_VAL", NULL };
	Rgd3Parser p;
	Rgd3Token tok = { 0 };
	const char* src;

	rgd3_parser_test_check(t, rgd3_parser_add_global_define("GLOBAL_VAL 42"), "AddGlobalDefine");
	rgd3_parser_test_check(t, rgd3_parser_add_global_define("GFUNC(x) ((x) * 2)"), "AddGlobalDefine with parameters");
	rgd3_parser_test_stream(t, "global defines", "GLOBAL_VAL GFUNC(3)", 0, with_globals);

	/* AddDefine must add the new define even with global defines around */
	src = "LOCAL GLOBAL_VAL";
	rgd3_parser_init(&p, 0);
	rgd3_parser_load_memory(&p, src, (int)strlen(src), "adddefine");
	rgd3_parser_test_check(t, rgd3_parser_add_define(&p, "LOCAL 7"), "AddDefine");
	rgd3_parser_test_check(t, rgd3_parser_read_token(&p, &tok) && rgd3_token_is(&tok, "7"), "AddDefine expansion");
	rgd3_parser_test_check(t, rgd3_parser_read_token(&p, &tok) && rgd3_token_is(&tok, "42"), "global define next to AddDefine");
	rgd3_parser_free(&p);

	rgd3_parser_test_check(t, rgd3_parser_remove_global_define("GLOBAL_VAL"), "RemoveGlobalDefine");
	rgd3_parser_test_check(t, !rgd3_parser_remove_global_define("GLOBAL_VAL"), "RemoveGlobalDefine twice");
	rgd3_parser_test_stream(t, "removed global define", "GLOBAL_VAL", 0, without);
	rgd3_parser_remove_all_global_defines();
	rgd3_parser_test_check(t, rgd3_parser_global_defines == NULL, "RemoveAllGlobalDefines");
	rgd3_token_free(&tok);
}

static void rgd3_parser_test_read_api(Rgd3ParserTest* t)
{
	Rgd3Parser p;
	Rgd3Token tok = { 0 };
	Rgd3StrBuf out = { 0 };
	float m[3] = { 0, 0, 0 };
	const char* src;

	src = "alpha beta\ngamma { x { y } } delta\nrest of line\n( 1 -2 3.5 )\nlast";
	rgd3_parser_init(&p, 0);
	rgd3_parser_load_memory(&p, src, (int)strlen(src), "read api");
	rgd3_parser_test_check(t, rgd3_parser_peek_token_string(&p, "alpha"), "PeekTokenString");
	rgd3_parser_test_check(t, !rgd3_parser_check_token_string(&p, "beta"), "CheckTokenString mismatch");
	rgd3_parser_test_check(t, rgd3_parser_check_token_string(&p, "alpha"), "CheckTokenString");
	rgd3_parser_test_check(t, rgd3_parser_read_token_on_line(&p, &tok) && rgd3_token_is(&tok, "beta"), "ReadTokenOnLine");
	rgd3_parser_test_check(t, !rgd3_parser_read_token_on_line(&p, &tok), "ReadTokenOnLine at end of line");
	rgd3_parser_test_check(t, rgd3_parser_check_token_type(&p, TT_NAME, 0, &tok) && rgd3_token_is(&tok, "gamma") && tok.line == 2, "CheckTokenType");
	rgd3_parser_test_check(t, rgd3_parser_skip_braced_section(&p, 1), "SkipBracedSection");
	rgd3_parser_test_check(t, !strcmp(rgd3_parser_parse_rest_of_line(&p, &out), "delta"), "ParseRestOfLine");
	rgd3_parser_test_check(t, rgd3_parser_read_token(&p, &tok) && rgd3_token_is(&tok, "rest") && tok.lines_crossed == 1, "lines crossed");
	rgd3_parser_unread_token(&p, &tok);
	rgd3_parser_test_check(t, rgd3_parser_read_token(&p, &tok) && rgd3_token_is(&tok, "rest"), "UnreadToken");
	rgd3_parser_test_check(t, rgd3_parser_skip_rest_of_line(&p), "SkipRestOfLine");
	rgd3_parser_test_check(t, rgd3_parser_parse_1d_matrix(&p, 3, m) && m[0] == 1.0f && m[1] == -2.0f && m[2] == 3.5f, "Parse1DMatrix");
	rgd3_parser_test_check(t, rgd3_parser_peek_token_type(&p, TT_NAME, 0, &tok) && rgd3_token_is(&tok, "last"), "PeekTokenType");
	rgd3_parser_test_check(t, rgd3_parser_skip_until_string(&p, "last"), "SkipUntilString");
	rgd3_parser_test_check(t, !rgd3_parser_read_token(&p, &tok) && rgd3_parser_end_of_file(&p), "end of file");
	rgd3_parser_free(&p);

	/* ParseBracedSection, single line and re-indented */
	src = "{ a \"str\" 'c' { b } } tail";
	rgd3_parser_init(&p, 0);
	rgd3_parser_load_memory(&p, src, (int)strlen(src), "braced");
	rgd3_parser_test_check(t, !strcmp(rgd3_parser_parse_braced_section(&p, &out, -1, 1, '{', '}'), "{a \"str\" 'c' { b } } "), "ParseBracedSection");
	rgd3_parser_test_check(t, rgd3_parser_read_token(&p, &tok) && rgd3_token_is(&tok, "tail"), "after ParseBracedSection");
	rgd3_parser_free(&p);

	src = "{\na\n{\nb\n}\n}";
	rgd3_parser_init(&p, 0);
	rgd3_parser_load_memory(&p, src, (int)strlen(src), "braced tabs");
	rgd3_parser_test_check(t, !strcmp(rgd3_parser_parse_braced_section(&p, &out, 0, 1, '{', '}'), "{\r\na \r\n{ \r\n\tb \r\n} \r\n} "), "ParseBracedSection with tabs");
	rgd3_parser_free(&p);

	/* numbers through the parser */
	src = "0x1F 017 0b101 3.5e2 -7 1.5f 10u -2.25 1 0x10";
	rgd3_parser_init(&p, 0);
	rgd3_parser_load_memory(&p, src, (int)strlen(src), "numbers");
	rgd3_parser_test_check(t, rgd3_parser_parse_int(&p) == 31, "ParseInt hex");
	rgd3_parser_test_check(t, rgd3_parser_parse_int(&p) == 15, "ParseInt octal");
	rgd3_parser_test_check(t, rgd3_parser_parse_int(&p) == 5, "ParseInt binary");
	rgd3_parser_test_check(t, rgd3_parser_parse_float(&p) == 350.0f, "ParseFloat exponent");
	rgd3_parser_test_check(t, rgd3_parser_parse_int(&p) == -7, "ParseInt negative");
	rgd3_parser_test_check(t, rgd3_parser_parse_float(&p) == 1.5f, "ParseFloat suffix");
	rgd3_parser_test_check(t, rgd3_parser_parse_int(&p) == 10, "ParseInt unsigned suffix");
	rgd3_parser_test_check(t, rgd3_parser_parse_float(&p) == -2.25f, "ParseFloat negative");
	rgd3_parser_test_check(t, rgd3_parser_parse_bool(&p) == 1, "ParseBool");
	rgd3_parser_test_check(t, rgd3_parser_expect_token_type(&p, TT_NUMBER, TT_HEX | TT_INTEGER, &tok) && rgd3_token_int_value(&tok) == 16, "ExpectTokenType hex");
	rgd3_parser_free(&p);

	/* marker */
	src = "first second  third";
	rgd3_parser_init(&p, 0);
	rgd3_parser_load_memory(&p, src, (int)strlen(src), "marker");
	rgd3_parser_read_token(&p, &tok);
	rgd3_parser_set_marker(&p);
	rgd3_parser_read_token(&p, &tok);
	rgd3_parser_get_string_from_marker(&p, &out, 0);
	rgd3_parser_test_check(t, !strcmp(rgd3_strbuf_cstr(&out), "second"), "GetStringFromMarker");
	rgd3_parser_read_token(&p, &tok);
	rgd3_strbuf_clear(&out);
	rgd3_parser_get_string_from_marker(&p, &out, 1);
	rgd3_parser_test_check(t, !strcmp(rgd3_strbuf_cstr(&out), "secondthird"), "GetStringFromMarker clean");
	rgd3_parser_free(&p);

	/* punctuation lookups */
	rgd3_parser_init(&p, 0);
	rgd3_parser_test_check(t, !strcmp(rgd3_parser_get_punctuation_from_id(&p, P_RSHIFT_ASSIGN), ">>=") && rgd3_parser_get_punctuation_id(&p, "...") == P_PARMS, "punctuation lookup");
	rgd3_parser_free(&p);

	rgd3_strbuf_free(&out);
	rgd3_token_free(&tok);
}

static void rgd3_parser_test_errors(Rgd3ParserTest* t)
{
	static const char* const after_base_include[] = { "wnext", NULL };
	Rgd3Parser p;
	Rgd3Token tok = { 0 };
	const char* src;

	/* ExpectTokenType mismatches only flag the error with LEXFL_NOERRORS */
	src = "abc 12 ; 1.5 x";
	rgd3_parser_init(&p, LEXFL_NOERRORS);
	rgd3_parser_load_memory(&p, src, (int)strlen(src), "expect errors");
	rgd3_parser_test_check(t, !rgd3_parser_expect_token_type(&p, TT_NUMBER, TT_INTEGER, &tok) && rgd3_token_is(&tok, "abc"), "ExpectTokenType wrong type");
	rgd3_parser_test_check(t, rgd3_lexer_had_error(p.scriptstack), "ExpectTokenType flags the error");
	rgd3_parser_test_check(t, !rgd3_parser_expect_token_type(&p, TT_NUMBER, TT_FLOAT, &tok), "ExpectTokenType wrong subtype");
	rgd3_parser_test_check(t, !rgd3_parser_expect_token_type(&p, TT_PUNCTUATION, P_COMMA, &tok), "ExpectTokenType wrong punctuation");
	rgd3_parser_test_check(t, rgd3_parser_expect_token_type(&p, TT_NUMBER, TT_FLOAT, &tok), "ExpectTokenType float");
	rgd3_parser_test_check(t, !rgd3_parser_expect_token_string(&p, "y"), "ExpectTokenString mismatch");
	rgd3_parser_test_check(t, !rgd3_parser_expect_any_token(&p, &tok), "ExpectAnyToken at end");
	rgd3_parser_free(&p);

	/* #include <...> with LEXFL_NOBASEINCLUDES is skipped */
	rgd3_parser_test_stream(t, "#include <> with LEXFL_NOBASEINCLUDES", "#include <foo/bar.h>\nnext", LEXFL_NOBASEINCLUDES, after_base_include);

	/* a missing include fails the read, then parsing goes on */
	src = "#include \"rgd3_no_such_file.h\"\nafter";
	rgd3_parser_init(&p, LEXFL_NOERRORS);
	rgd3_parser_load_memory(&p, src, (int)strlen(src), "missing include");
	rgd3_parser_test_check(t, !rgd3_parser_read_token(&p, &tok) && rgd3_lexer_had_error(p.scriptstack), "missing #include");
	rgd3_parser_test_check(t, rgd3_parser_read_token(&p, &tok) && rgd3_token_is(&tok, "after"), "after a missing #include");
	rgd3_parser_free(&p);

	/* #else after #else (which also drops the indent), misplaced #endif,
	   unknown directive */
	src = "#if 1\n#else\n#else\n#endif\n#bogus\nok";
	rgd3_parser_init(&p, LEXFL_NOERRORS);
	rgd3_parser_load_memory(&p, src, (int)strlen(src), "directive errors");
	rgd3_parser_test_check(t, !rgd3_parser_read_token(&p, &tok), "#else after #else");
	rgd3_parser_test_check(t, !rgd3_parser_read_token(&p, &tok), "misplaced #endif");
	rgd3_parser_test_check(t, !rgd3_parser_read_token(&p, &tok), "unknown directive");
	rgd3_parser_test_check(t, rgd3_parser_read_token(&p, &tok) && rgd3_token_is(&tok, "ok"), "after directive errors");
	rgd3_parser_free(&p);

	/* "#if" with an undefined name fails the read; no tokens leak */
	src = "#if 1 + nope\n#endif\nfine";
	rgd3_parser_init(&p, LEXFL_NOERRORS);
	rgd3_parser_load_memory(&p, src, (int)strlen(src), "undefined in #if");
	rgd3_parser_test_check(t, !rgd3_parser_read_token(&p, &tok), "#if with an undefined name");
	rgd3_parser_free(&p);

	rgd3_token_free(&tok);
}

/* A real drop in the middle of #if evaluation, with a token chain in
   flight: FreeSource must reclaim it. */
static void rgd3_parser_test_drop(Rgd3ParserTest* t)
{
	static Rgd3Parser p;
	static Rgd3Token tok;
	Rgd3ErrorGuard guard;
	volatile int dropped = 0;
	i64 before = rgd3_mem_tag_count(RGD3_TAG_PARSER);
	const char* src = "#define TWO 2\n#if 1 + TWO * undefined_name\n#endif\n";

	rgd3_token_init(&tok);
	rgd3_parser_init(&p, 0);
	rgd3_parser_load_memory(&p, src, (int)strlen(src), "drop");
	rgd3_printf("parser test: the next ERROR is expected\n");
	rgd3_error_push(&guard);
	if (setjmp(guard.jump) == 0)
	{
		rgd3_parser_read_token(&p, &tok);
		rgd3_error_pop(&guard);
	}
	else
	{
		dropped = 1;
	}
	rgd3_parser_test_check(t, dropped, "Error drops without NOERRORS");
	rgd3_parser_test_check(t, p.live != NULL, "tokens in flight at the drop");
	rgd3_parser_free(&p);
	rgd3_parser_free(&p); /* idempotent */
	rgd3_token_free(&tok);
	rgd3_parser_test_check(t, rgd3_mem_tag_count(RGD3_TAG_PARSER) == before, "FreeSource after a drop frees everything");
}

/* #include through the file system: only when it is up and writable */
static void rgd3_parser_test_include(Rgd3ParserTest* t)
{
	static const char* const expected[] = { "winside", "n5", "wafter", "n5", NULL };
	static const char inc[] = "#define INC_VAL 5\ninside INC_VAL\n";
	const char* name = "rgd3_selftest_parser.inc";
	char os_path[RGD3_MAX_OSPATH];
	Rgd3File* f;

	f = rgd3_fs_open_write(name);
	if (!f)
	{
		rgd3_printf("parser test: file system not available, #include \"file\" skipped\n");
		return;
	}
	rgd3_str_copynz(os_path, rgd3_fs_file_name(f), sizeof(os_path));
	rgd3_fs_write(f, inc, sizeof(inc) - 1);
	rgd3_fs_close(f);
	rgd3_parser_test_stream(t, "#include \"file\"", "#include \"rgd3_selftest_parser.inc\"\nafter INC_VAL", 0, expected);
	SDL_RemovePath(os_path);
}

int rgd3_parser_self_test(void)
{
	Rgd3ParserTest t = { 0 };
	i64 parser_count = rgd3_mem_tag_count(RGD3_TAG_PARSER);
	i64 lexer_count = rgd3_mem_tag_count(RGD3_TAG_LEXER);
	i64 string_count = rgd3_mem_tag_count(RGD3_TAG_STRING);

	rgd3_parser_test_macros(&t);
	rgd3_parser_test_conditionals(&t);
	rgd3_parser_test_globals(&t);
	rgd3_parser_test_read_api(&t);
	rgd3_parser_test_errors(&t);
	rgd3_parser_test_include(&t);
	rgd3_parser_test_drop(&t);
	rgd3_parser_test_check(&t, rgd3_mem_tag_count(RGD3_TAG_PARSER) == parser_count, "no RGD3_TAG_PARSER leaks");
	rgd3_parser_test_check(&t, rgd3_mem_tag_count(RGD3_TAG_LEXER) == lexer_count, "no RGD3_TAG_LEXER leaks");
	rgd3_parser_test_check(&t, rgd3_mem_tag_count(RGD3_TAG_STRING) == string_count, "no RGD3_TAG_STRING leaks");
	rgd3_printf("parser: %d checks, %d failures\n", t.checks, t.failures);
	return t.failures == 0;
}
