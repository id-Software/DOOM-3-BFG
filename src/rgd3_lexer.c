/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_lexer.c - tokens and the lexicographical parser.
   Port of neo/idlib/Token.cpp and neo/idlib/Lexer.cpp: same punctuation
   table, white-space and comment rules (bytes >= 0x80 count as white space,
   as with MSVC's signed char), string concatenation, escape characters
   (decimal, not octal, after '\'), number classification and the
   ParseBracedSection family. */

/* ---- tokens ---- */

static void rgd3_token_fix(Rgd3Token* t)
{
	/* zero-initialised, or struct-copied while the text was inline */
	if (t->alloced <= RGD3_TOKEN_BASE_SIZE && t->data != t->base)
	{
		if (!t->alloced)
		{
			t->len = 0;
			t->base[0] = 0;
		}
		t->data = t->base;
		t->alloced = RGD3_TOKEN_BASE_SIZE;
	}
}

void rgd3_token_init(Rgd3Token* t)
{
	memset(t, 0, sizeof(*t));
	t->data = t->base;
	t->alloced = RGD3_TOKEN_BASE_SIZE;
}

static void rgd3_token_unwind(void* object)
{
	rgd3_token_free((Rgd3Token*)object);
}

void rgd3_token_free(Rgd3Token* t)
{
	if (t->alloced > RGD3_TOKEN_BASE_SIZE && t->data && t->data != t->base)
	{
		rgd3_heap_free(t->data);
		rgd3_error_unwind_unregister(t);
	}
	t->data = t->base;
	t->alloced = RGD3_TOKEN_BASE_SIZE;
	t->len = 0;
	t->base[0] = 0;
}

void rgd3_token_ensure(Rgd3Token* t, int size, int keep)
{
	int cap;
	rgd3_token_fix(t);
	if (size <= t->alloced)
		return;
	cap = t->alloced;
	while (cap < size)
		cap *= 2;
	if (t->data == t->base)
	{
		char* heap = (char*)rgd3_heap_alloc((size_t)cap, RGD3_TAG_LEXER);
		if (keep)
			memcpy(heap, t->base, (size_t)t->len + 1);
		else
			heap[0] = 0;
		t->data = heap;
		/* long text: free it if a drop unwinds the token's frame */
		rgd3_error_unwind_register(t, rgd3_token_unwind);
	}
	else
	{
		t->data = (char*)rgd3_heap_realloc(t->data, (size_t)cap, RGD3_TAG_LEXER);
	}
	t->alloced = cap;
}

void rgd3_token_copy(Rgd3Token* dst, const Rgd3Token* src)
{
	int len;
	if (dst == src)
		return;
	len = src->len;
	rgd3_token_ensure(dst, len + 1, 0);
	memcpy(dst->data, src->data ? src->data : "", (size_t)len);
	dst->data[len] = 0;
	dst->len = len;
	dst->type = src->type;
	dst->subtype = src->subtype;
	dst->line = src->line;
	dst->lines_crossed = src->lines_crossed;
	dst->flags = src->flags;
	dst->intvalue = src->intvalue;
	dst->floatvalue = src->floatvalue;
	dst->white_space_start_p = src->white_space_start_p;
	dst->white_space_end_p = src->white_space_end_p;
	dst->next = src->next;
}

void rgd3_token_set_n(Rgd3Token* t, const char* text, int n)
{
	rgd3_token_ensure(t, n + 1, 0);
	memcpy(t->data, text, (size_t)n);
	t->data[n] = 0;
	t->len = n;
}

void rgd3_token_set(Rgd3Token* t, const char* text)
{
	rgd3_token_set_n(t, text ? text : "", text ? (int)strlen(text) : 0);
}

void rgd3_token_append(Rgd3Token* t, const char* text)
{
	int n = (int)strlen(text);
	rgd3_token_ensure(t, t->len + n + 1, 1);
	memcpy(t->data + t->len, text, (size_t)n + 1);
	t->len += n;
}

void rgd3_token_append_char(Rgd3Token* t, char c)
{
	rgd3_token_ensure(t, t->len + 2, 1);
	t->data[t->len++] = c;
	t->data[t->len] = 0;
}

/* idToken::AppendDirty: no terminator until the reader finishes */
static void rgd3_token_append_dirty(Rgd3Token* t, char c)
{
	rgd3_token_ensure(t, t->len + 2, 1);
	t->data[t->len++] = c;
}

void rgd3_token_clear(Rgd3Token* t)
{
	rgd3_token_fix(t);
	t->len = 0;
	t->data[0] = 0;
}

const char* rgd3_token_cstr(const Rgd3Token* t)
{
	if (!t->data)
		return "";
	if (t->alloced <= RGD3_TOKEN_BASE_SIZE)
		return t->base;
	return t->data;
}

int rgd3_token_is(const Rgd3Token* t, const char* text)
{
	return strcmp(rgd3_token_cstr(t), text) == 0;
}

int rgd3_token_icmp(const Rgd3Token* t, const char* text)
{
	return rgd3_str_icmp(rgd3_token_cstr(t), text);
}

void rgd3_token_number_value(Rgd3Token* t)
{
	int i, pow, div, c;
	const char* p;
	double m;

	RG_ASSERT(t->type == TT_NUMBER);
	p = rgd3_token_cstr(t);
	t->floatvalue = 0;
	t->intvalue = 0;
	if (t->subtype & TT_FLOAT)
	{
		if (t->subtype & (TT_INFINITE | TT_INDEFINITE | TT_NAN))
		{
			union { u32 u; float f; } bits;
			if (t->subtype & TT_INFINITE)
				bits.u = 0x7f800000u; /* 1.#INF */
			else if (t->subtype & TT_INDEFINITE)
				bits.u = 0xffc00000u; /* 1.#IND */
			else
				bits.u = 0x7fc00000u; /* 1.#QNAN */
			t->floatvalue = (double)bits.f;
		}
		else
		{
			while (*p && *p != '.' && *p != 'e')
			{
				t->floatvalue = t->floatvalue * 10.0 + (double)(*p - '0');
				p++;
			}
			if (*p == '.')
			{
				p++;
				for (m = 0.1; *p && *p != 'e'; p++)
				{
					t->floatvalue = t->floatvalue + (double)(*p - '0') * m;
					m *= 0.1;
				}
			}
			if (*p == 'e')
			{
				p++;
				if (*p == '-')
				{
					div = 1;
					p++;
				}
				else if (*p == '+')
				{
					div = 0;
					p++;
				}
				else
				{
					div = 0;
				}
				for (pow = 0; *p; p++)
					pow = pow * 10 + (int)(*p - '0');
				for (m = 1.0, i = 0; i < pow; i++)
					m *= 10.0;
				if (div)
					t->floatvalue /= m;
				else
					t->floatvalue *= m;
			}
		}
		/* idMath::Ftoi takes a float and truncates */
		t->intvalue = (u32)(int)(float)t->floatvalue;
	}
	else if (t->subtype & TT_DECIMAL)
	{
		while (*p)
		{
			t->intvalue = t->intvalue * 10 + (u32)(*p - '0');
			p++;
		}
		t->floatvalue = t->intvalue;
	}
	else if (t->subtype & TT_IPADDRESS)
	{
		c = 0;
		while (*p && *p != ':')
		{
			if (*p == '.')
			{
				while (c != 3)
				{
					t->intvalue = t->intvalue * 10;
					c++;
				}
				c = 0;
			}
			else
			{
				t->intvalue = t->intvalue * 10 + (u32)(*p - '0');
				c++;
			}
			p++;
		}
		while (c != 3)
		{
			t->intvalue = t->intvalue * 10;
			c++;
		}
		t->floatvalue = t->intvalue;
	}
	else if (t->subtype & TT_OCTAL)
	{
		p += 1; /* the leading zero */
		while (*p)
		{
			t->intvalue = (t->intvalue << 3) + (u32)(*p - '0');
			p++;
		}
		t->floatvalue = t->intvalue;
	}
	else if (t->subtype & TT_HEX)
	{
		p += 2; /* 0x */
		while (*p)
		{
			t->intvalue <<= 4;
			if (*p >= 'a' && *p <= 'f')
				t->intvalue += (u32)(*p - 'a' + 10);
			else if (*p >= 'A' && *p <= 'F')
				t->intvalue += (u32)(*p - 'A' + 10);
			else
				t->intvalue += (u32)(*p - '0');
			p++;
		}
		t->floatvalue = t->intvalue;
	}
	else if (t->subtype & TT_BINARY)
	{
		p += 2; /* 0b */
		while (*p)
		{
			t->intvalue = (t->intvalue << 1) + (u32)(*p - '0');
			p++;
		}
		t->floatvalue = t->intvalue;
	}
	t->subtype |= TT_VALUESVALID;
}

double rgd3_token_double_value(Rgd3Token* t)
{
	if (t->type != TT_NUMBER)
		return 0.0;
	if (!(t->subtype & TT_VALUESVALID))
		rgd3_token_number_value(t);
	return t->floatvalue;
}

float rgd3_token_float_value(Rgd3Token* t)
{
	return (float)rgd3_token_double_value(t);
}

u32 rgd3_token_unsigned_value(Rgd3Token* t)
{
	if (t->type != TT_NUMBER)
		return 0;
	if (!(t->subtype & TT_VALUESVALID))
		rgd3_token_number_value(t);
	return t->intvalue;
}

int rgd3_token_int_value(Rgd3Token* t)
{
	return (int)rgd3_token_unsigned_value(t);
}

int rgd3_token_white_space_before(const Rgd3Token* t)
{
	return t->white_space_end_p > t->white_space_start_p;
}

void rgd3_token_clear_white_space(Rgd3Token* t)
{
	t->white_space_start_p = NULL;
	t->white_space_end_p = NULL;
	t->lines_crossed = 0;
}

/* ---- punctuation ---- */

/* longer punctuations first */
const Rgd3Punctuation rgd3_default_punctuations[] = {
	{ ">>=", P_RSHIFT_ASSIGN },
	{ "<<=", P_LSHIFT_ASSIGN },
	{ "...", P_PARMS },
	{ "##", P_PRECOMPMERGE },
	{ "&&", P_LOGIC_AND },
	{ "||", P_LOGIC_OR },
	{ ">=", P_LOGIC_GEQ },
	{ "<=", P_LOGIC_LEQ },
	{ "==", P_LOGIC_EQ },
	{ "!=", P_LOGIC_UNEQ },
	{ "*=", P_MUL_ASSIGN },
	{ "/=", P_DIV_ASSIGN },
	{ "%=", P_MOD_ASSIGN },
	{ "+=", P_ADD_ASSIGN },
	{ "-=", P_SUB_ASSIGN },
	{ "++", P_INC },
	{ "--", P_DEC },
	{ "&=", P_BIN_AND_ASSIGN },
	{ "|=", P_BIN_OR_ASSIGN },
	{ "^=", P_BIN_XOR_ASSIGN },
	{ ">>", P_RSHIFT },
	{ "<<", P_LSHIFT },
	{ "->", P_POINTERREF },
	{ "::", P_CPP1 },
	{ ".*", P_CPP2 },
	{ "*", P_MUL },
	{ "/", P_DIV },
	{ "%", P_MOD },
	{ "+", P_ADD },
	{ "-", P_SUB },
	{ "=", P_ASSIGN },
	{ "&", P_BIN_AND },
	{ "|", P_BIN_OR },
	{ "^", P_BIN_XOR },
	{ "~", P_BIN_NOT },
	{ "!", P_LOGIC_NOT },
	{ ">", P_LOGIC_GREATER },
	{ "<", P_LOGIC_LESS },
	{ ".", P_REF },
	{ ",", P_COMMA },
	{ ";", P_SEMICOLON },
	{ ":", P_COLON },
	{ "?", P_QUESTIONMARK },
	{ "(", P_PARENTHESESOPEN },
	{ ")", P_PARENTHESESCLOSE },
	{ "{", P_BRACEOPEN },
	{ "}", P_BRACECLOSE },
	{ "[", P_SQBRACKETOPEN },
	{ "]", P_SQBRACKETCLOSE },
	{ "\\", P_BACKSLASH },
	{ "#", P_PRECOMP },
	{ "$", P_DOLLAR },
	{ NULL, 0 }
};

#define RGD3_NUM_DEFAULT_PUNCTUATIONS (sizeof(rgd3_default_punctuations) / sizeof(rgd3_default_punctuations[0]))

static int rgd3_lexer_default_table[256];
static int rgd3_lexer_default_next[RGD3_NUM_DEFAULT_PUNCTUATIONS];
static int rgd3_lexer_default_setup;
static char rgd3_lexer_base_folder[256];

static void rgd3_lexer_create_punctuation_table(Rgd3Lexer* lex, const Rgd3Punctuation* punctuations)
{
	int i, n, lastp;
	const Rgd3Punctuation* p;
	const Rgd3Punctuation* newp;

	if (punctuations == rgd3_default_punctuations)
	{
		lex->punctuation_table = rgd3_lexer_default_table;
		lex->next_punctuation = rgd3_lexer_default_next;
		if (rgd3_lexer_default_setup)
			return;
		rgd3_lexer_default_setup = 1;
		i = (int)RGD3_NUM_DEFAULT_PUNCTUATIONS;
	}
	else
	{
		if (!lex->punctuation_table || lex->punctuation_table == rgd3_lexer_default_table)
			lex->punctuation_table = (int*)rgd3_heap_alloc(256 * sizeof(int), RGD3_TAG_LEXER);
		if (lex->next_punctuation && lex->next_punctuation != rgd3_lexer_default_next)
			rgd3_heap_free(lex->next_punctuation);
		for (i = 0; punctuations[i].p; i++)
		{
		}
		lex->next_punctuation = (int*)rgd3_heap_alloc((size_t)(i > 0 ? i : 1) * sizeof(int), RGD3_TAG_LEXER);
	}
	memset(lex->punctuation_table, 0xFF, 256 * sizeof(int));
	memset(lex->next_punctuation, 0xFF, (size_t)i * sizeof(int));
	for (i = 0; punctuations[i].p; i++)
	{
		newp = &punctuations[i];
		lastp = -1;
		/* keep each chain sorted longest first */
		for (n = lex->punctuation_table[(u8)newp->p[0]]; n >= 0; n = lex->next_punctuation[n])
		{
			p = &punctuations[n];
			if (strlen(p->p) < strlen(newp->p))
			{
				lex->next_punctuation[i] = n;
				if (lastp >= 0)
					lex->next_punctuation[lastp] = i;
				else
					lex->punctuation_table[(u8)newp->p[0]] = i;
				break;
			}
			lastp = n;
		}
		if (n < 0)
		{
			lex->next_punctuation[i] = -1;
			if (lastp >= 0)
				lex->next_punctuation[lastp] = i;
			else
				lex->punctuation_table[(u8)newp->p[0]] = i;
		}
	}
}

const char* rgd3_lexer_get_punctuation_from_id(const Rgd3Lexer* lex, int id)
{
	int i;
	for (i = 0; lex->punctuations[i].p; i++)
	{
		if (lex->punctuations[i].n == id)
			return lex->punctuations[i].p;
	}
	return "unkown punctuation";
}

int rgd3_lexer_get_punctuation_id(const Rgd3Lexer* lex, const char* p)
{
	int i;
	for (i = 0; lex->punctuations[i].p; i++)
	{
		if (!strcmp(lex->punctuations[i].p, p))
			return lex->punctuations[i].n;
	}
	return 0;
}

void rgd3_lexer_set_punctuations(Rgd3Lexer* lex, const Rgd3Punctuation* p)
{
	rgd3_lexer_create_punctuation_table(lex, p ? p : rgd3_default_punctuations);
	lex->punctuations = p ? p : rgd3_default_punctuations;
}

/* ---- errors ---- */

void rgd3_lexer_error(Rgd3Lexer* lex, const char* fmt, ...)
{
	char text[RGD3_MAX_STRING_CHARS];
	va_list args;

	lex->had_error = 1;
	if (lex->flags & LEXFL_NOERRORS)
		return;
	va_start(args, fmt);
	rg_vsnprintf(text, sizeof(text), fmt, args);
	va_end(args);
	if (lex->flags & LEXFL_NOFATALERRORS)
		rgd3_warning("file %s, line %d: %s", lex->filename, lex->line, text);
	else
		rgd3_error_drop("file %s, line %d: %s", lex->filename, lex->line, text);
}

void rgd3_lexer_warning(Rgd3Lexer* lex, const char* fmt, ...)
{
	char text[RGD3_MAX_STRING_CHARS];
	va_list args;

	if (lex->flags & LEXFL_NOWARNINGS)
		return;
	va_start(args, fmt);
	rg_vsnprintf(text, sizeof(text), fmt, args);
	va_end(args);
	rgd3_warning("file %s, line %d: %s", lex->filename, lex->line, text);
}

int rgd3_lexer_had_error(const Rgd3Lexer* lex)
{
	return lex->had_error;
}

/* ---- white space ---- */

/* Spaces, control characters, bytes >= 0x80 (signed char) and comments.
   Returns 0 at the end of the script. */
static int rgd3_lexer_read_white_space(Rgd3Lexer* lex)
{
	for (;;)
	{
		while (*(const signed char*)lex->script_p <= ' ')
		{
			if (!*lex->script_p)
				return 0;
			if (*lex->script_p == '\n')
				lex->line++;
			lex->script_p++;
		}
		if (*lex->script_p == '/')
		{
			if (lex->script_p[1] == '/')
			{
				lex->script_p++;
				do
				{
					lex->script_p++;
					if (!*lex->script_p)
						return 0;
				} while (*lex->script_p != '\n');
				lex->line++;
				lex->script_p++;
				if (!*lex->script_p)
					return 0;
				continue;
			}
			else if (lex->script_p[1] == '*')
			{
				lex->script_p++;
				for (;;)
				{
					lex->script_p++;
					if (!*lex->script_p)
						return 0;
					if (*lex->script_p == '\n')
					{
						lex->line++;
					}
					else if (*lex->script_p == '/')
					{
						if (lex->script_p[-1] == '*')
							break;
						if (lex->script_p[1] == '*')
							rgd3_lexer_warning(lex, "nested comment");
					}
				}
				lex->script_p++;
				if (!*lex->script_p)
					return 0;
				lex->script_p++;
				if (!*lex->script_p)
					return 0;
				continue;
			}
		}
		break;
	}
	return 1;
}

int rgd3_lexer_skip_white_space(Rgd3Lexer* lex, int current_line)
{
	for (;;)
	{
		RG_ASSERT(lex->script_p <= lex->end_p);
		if (lex->script_p == lex->end_p)
			return 0;
		while (*(const signed char*)lex->script_p <= ' ')
		{
			if (lex->script_p == lex->end_p)
				return 0;
			if (!*lex->script_p)
				return 0;
			if (*lex->script_p == '\n')
			{
				lex->line++;
				if (current_line)
				{
					lex->script_p++;
					return 1;
				}
			}
			lex->script_p++;
		}
		if (*lex->script_p == '/')
		{
			if (lex->script_p[1] == '/')
			{
				lex->script_p++;
				do
				{
					lex->script_p++;
					if (!*lex->script_p)
						return 0;
				} while (*lex->script_p != '\n');
				lex->line++;
				lex->script_p++;
				if (current_line)
					return 1;
				if (!*lex->script_p)
					return 0;
				continue;
			}
			else if (lex->script_p[1] == '*')
			{
				lex->script_p++;
				for (;;)
				{
					lex->script_p++;
					if (!*lex->script_p)
						return 0;
					if (*lex->script_p == '\n')
					{
						lex->line++;
					}
					else if (*lex->script_p == '/')
					{
						if (lex->script_p[-1] == '*')
							break;
						if (lex->script_p[1] == '*')
							rgd3_lexer_warning(lex, "nested comment");
					}
				}
				lex->script_p++;
				if (!*lex->script_p)
					return 0;
				continue;
			}
		}
		break;
	}
	return 1;
}

/* ---- token readers ---- */

static int rgd3_lexer_read_escape_character(Rgd3Lexer* lex, char* ch)
{
	int c, val;

	lex->script_p++; /* the backslash */
	switch (*lex->script_p)
	{
	case '\\': c = '\\'; break;
	case 'n': c = '\n'; break;
	case 'r': c = '\r'; break;
	case 't': c = '\t'; break;
	case 'v': c = '\v'; break;
	case 'b': c = '\b'; break;
	case 'f': c = '\f'; break;
	case 'a': c = '\a'; break;
	case '\'': c = '\''; break;
	case '\"': c = '\"'; break;
	case '\?': c = '\?'; break;
	case 'x':
	{
		lex->script_p++;
		for (val = 0;; lex->script_p++)
		{
			c = *lex->script_p;
			if (c >= '0' && c <= '9')
				c = c - '0';
			else if (c >= 'A' && c <= 'Z')
				c = c - 'A' + 10;
			else if (c >= 'a' && c <= 'z')
				c = c - 'a' + 10;
			else
				break;
			val = (val << 4) + c;
		}
		lex->script_p--;
		if (val > 0xFF)
		{
			rgd3_lexer_warning(lex, "too large value in escape character");
			val = 0xFF;
		}
		c = val;
		break;
	}
	default: /* decimal ASCII code, NOT octal */
	{
		if (*lex->script_p < '0' || *lex->script_p > '9')
			rgd3_lexer_error(lex, "unknown escape char");
		for (val = 0;; lex->script_p++)
		{
			c = *lex->script_p;
			if (c >= '0' && c <= '9')
				c = c - '0';
			else
				break;
			val = val * 10 + c;
		}
		lex->script_p--;
		if (val > 0xFF)
		{
			rgd3_lexer_warning(lex, "too large value in escape character");
			val = 0xFF;
		}
		c = val;
		break;
	}
	}
	lex->script_p++; /* the escape character or the last digit */
	*ch = (char)c;
	return 1;
}

/* Escape characters are interpreted; two strings with only white space
   between them are read as one string. */
static int rgd3_lexer_read_string(Rgd3Lexer* lex, Rgd3Token* token, int quote)
{
	int tmpline;
	const char* tmpscript_p;
	char ch;

	token->type = quote == '\"' ? TT_STRING : TT_LITERAL;
	lex->script_p++; /* leading quote */

	for (;;)
	{
		if (*lex->script_p == '\\' && !(lex->flags & LEXFL_NOSTRINGESCAPECHARS))
		{
			if (!rgd3_lexer_read_escape_character(lex, &ch))
				return 0;
			rgd3_token_append_dirty(token, ch);
		}
		else if (*lex->script_p == quote)
		{
			lex->script_p++;
			if ((lex->flags & LEXFL_NOSTRINGCONCAT) && (!(lex->flags & LEXFL_ALLOWBACKSLASHSTRINGCONCAT) || quote != '\"'))
				break;

			tmpscript_p = lex->script_p;
			tmpline = lex->line;
			if (!rgd3_lexer_read_white_space(lex))
			{
				lex->script_p = tmpscript_p;
				lex->line = tmpline;
				break;
			}

			if (lex->flags & LEXFL_NOSTRINGCONCAT)
			{
				if (*lex->script_p != '\\')
				{
					lex->script_p = tmpscript_p;
					lex->line = tmpline;
					break;
				}
				lex->script_p++;
				if (!rgd3_lexer_read_white_space(lex) || *lex->script_p != quote)
				{
					rgd3_lexer_error(lex, "expecting string after '\\' terminated line");
					return 0;
				}
			}

			if (*lex->script_p != quote)
			{
				lex->script_p = tmpscript_p;
				lex->line = tmpline;
				break;
			}
			lex->script_p++;
		}
		else
		{
			if (*lex->script_p == '\0')
			{
				rgd3_lexer_error(lex, "missing trailing quote");
				return 0;
			}
			if (*lex->script_p == '\n')
			{
				rgd3_lexer_error(lex, "newline inside string");
				return 0;
			}
			rgd3_token_append_dirty(token, *lex->script_p++);
		}
	}
	rgd3_token_ensure(token, token->len + 1, 1);
	token->data[token->len] = '\0';

	if (token->type == TT_LITERAL)
	{
		if (!(lex->flags & LEXFL_ALLOWMULTICHARLITERALS) && token->len != 1)
			rgd3_lexer_warning(lex, "literal is not one character long");
		token->subtype = token->data[0];
	}
	else
	{
		token->subtype = token->len;
	}
	return 1;
}

static int rgd3_lexer_read_name(Rgd3Lexer* lex, Rgd3Token* token)
{
	char c;

	token->type = TT_NAME;
	do
	{
		rgd3_token_append_dirty(token, *lex->script_p++);
		c = *lex->script_p;
	} while ((c >= 'a' && c <= 'z') ||
		(c >= 'A' && c <= 'Z') ||
		(c >= '0' && c <= '9') ||
		c == '_' ||
		/* when everything is a string, '-' is not a separate token */
		((lex->flags & LEXFL_ONLYSTRINGS) && c == '-') ||
		((lex->flags & LEXFL_ALLOWPATHNAMES) && (c == '/' || c == '\\' || c == ':' || c == '.')));
	rgd3_token_ensure(token, token->len + 1, 1);
	token->data[token->len] = '\0';
	token->subtype = token->len;
	return 1;
}

static int rgd3_lexer_check_string(const Rgd3Lexer* lex, const char* str)
{
	int i;
	for (i = 0; str[i]; i++)
	{
		if (lex->script_p[i] != str[i])
			return 0;
	}
	return 1;
}

static int rgd3_lexer_read_number(Rgd3Lexer* lex, Rgd3Token* token)
{
	int i, dot;
	char c, c2;

	token->type = TT_NUMBER;
	token->subtype = 0;
	token->intvalue = 0;
	token->floatvalue = 0;

	c = *lex->script_p;
	c2 = lex->script_p[1];

	if (c == '0' && c2 != '.')
	{
		if (c2 == 'x' || c2 == 'X')
		{
			rgd3_token_append_dirty(token, *lex->script_p++);
			rgd3_token_append_dirty(token, *lex->script_p++);
			c = *lex->script_p;
			while ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))
			{
				rgd3_token_append_dirty(token, c);
				c = *(++lex->script_p);
			}
			token->subtype = TT_HEX | TT_INTEGER;
		}
		else if (c2 == 'b' || c2 == 'B')
		{
			rgd3_token_append_dirty(token, *lex->script_p++);
			rgd3_token_append_dirty(token, *lex->script_p++);
			c = *lex->script_p;
			while (c == '0' || c == '1')
			{
				rgd3_token_append_dirty(token, c);
				c = *(++lex->script_p);
			}
			token->subtype = TT_BINARY | TT_INTEGER;
		}
		else
		{
			rgd3_token_append_dirty(token, *lex->script_p++);
			c = *lex->script_p;
			while (c >= '0' && c <= '7')
			{
				rgd3_token_append_dirty(token, c);
				c = *(++lex->script_p);
			}
			token->subtype = TT_OCTAL | TT_INTEGER;
		}
	}
	else
	{
		/* decimal integer, floating point number or ip address */
		dot = 0;
		for (;;)
		{
			if (c >= '0' && c <= '9')
			{
			}
			else if (c == '.')
			{
				dot++;
			}
			else
			{
				break;
			}
			rgd3_token_append_dirty(token, c);
			c = *(++lex->script_p);
		}
		if (c == 'e' && dot == 0)
			dot++; /* scientific notation without a decimal point */
		if (dot == 1)
		{
			token->subtype = TT_DECIMAL | TT_FLOAT;
			if (c == 'e')
			{
				rgd3_token_append_dirty(token, c);
				c = *(++lex->script_p);
				if (c == '-' || c == '+')
				{
					rgd3_token_append_dirty(token, c);
					c = *(++lex->script_p);
				}
				while (c >= '0' && c <= '9')
				{
					rgd3_token_append_dirty(token, c);
					c = *(++lex->script_p);
				}
			}
			else if (c == '#')
			{
				/* 1.#INF, 1.#IND, 1.#NAN, 1.#QNAN, 1.#SNAN */
				c2 = 4;
				if (rgd3_lexer_check_string(lex, "INF"))
				{
					token->subtype |= TT_INFINITE;
				}
				else if (rgd3_lexer_check_string(lex, "IND"))
				{
					token->subtype |= TT_INDEFINITE;
				}
				else if (rgd3_lexer_check_string(lex, "NAN"))
				{
					token->subtype |= TT_NAN;
				}
				else if (rgd3_lexer_check_string(lex, "QNAN"))
				{
					token->subtype |= TT_NAN;
					c2++;
				}
				else if (rgd3_lexer_check_string(lex, "SNAN"))
				{
					token->subtype |= TT_NAN;
					c2++;
				}
				for (i = 0; i < c2; i++)
				{
					rgd3_token_append_dirty(token, c);
					c = *(++lex->script_p);
				}
				while (c >= '0' && c <= '9')
				{
					rgd3_token_append_dirty(token, c);
					c = *(++lex->script_p);
				}
				if (!(lex->flags & LEXFL_ALLOWFLOATEXCEPTIONS))
				{
					rgd3_token_append_dirty(token, 0);
					rgd3_lexer_error(lex, "parsed %s", token->data);
				}
			}
		}
		else if (dot > 1)
		{
			if (!(lex->flags & LEXFL_ALLOWIPADDRESSES))
			{
				rgd3_lexer_error(lex, "more than one dot in number");
				return 0;
			}
			if (dot != 3)
			{
				rgd3_lexer_error(lex, "ip address should have three dots");
				return 0;
			}
			token->subtype = TT_IPADDRESS;
		}
		else
		{
			token->subtype = TT_DECIMAL | TT_INTEGER;
		}
	}

	if (token->subtype & TT_FLOAT)
	{
		if (c > ' ')
		{
			if (c == 'f' || c == 'F')
			{
				token->subtype |= TT_SINGLE_PRECISION;
				lex->script_p++;
			}
			else if (c == 'l' || c == 'L')
			{
				token->subtype |= TT_EXTENDED_PRECISION;
				lex->script_p++;
			}
			else
			{
				token->subtype |= TT_DOUBLE_PRECISION;
			}
		}
		else
		{
			token->subtype |= TT_DOUBLE_PRECISION;
		}
	}
	else if (token->subtype & TT_INTEGER)
	{
		if (c > ' ')
		{
			for (i = 0; i < 2; i++)
			{
				if (c == 'l' || c == 'L')
					token->subtype |= TT_LONG;
				else if (c == 'u' || c == 'U')
					token->subtype |= TT_UNSIGNED;
				else
					break;
				c = *(++lex->script_p);
			}
		}
	}
	else if (token->subtype & TT_IPADDRESS)
	{
		if (c == ':')
		{
			rgd3_token_append_dirty(token, c);
			c = *(++lex->script_p);
			while (c >= '0' && c <= '9')
			{
				rgd3_token_append_dirty(token, c);
				c = *(++lex->script_p);
			}
			token->subtype |= TT_IPPORT;
		}
	}
	rgd3_token_ensure(token, token->len + 1, 1);
	token->data[token->len] = '\0';
	return 1;
}

static int rgd3_lexer_read_punctuation(Rgd3Lexer* lex, Rgd3Token* token)
{
	int l, n;
	const char* p;
	const Rgd3Punctuation* punc;

	for (n = lex->punctuation_table[(u8)*lex->script_p]; n >= 0; n = lex->next_punctuation[n])
	{
		punc = &lex->punctuations[n];
		p = punc->p;
		for (l = 0; p[l] && lex->script_p[l]; l++)
		{
			if (lex->script_p[l] != p[l])
				break;
		}
		if (!p[l])
		{
			rgd3_token_set_n(token, p, l);
			lex->script_p += l;
			token->type = TT_PUNCTUATION;
			token->subtype = punc->n;
			return 1;
		}
	}
	return 0;
}

int rgd3_lexer_read_token(Rgd3Lexer* lex, Rgd3Token* token)
{
	int c;

	if (!lex->loaded)
	{
		rgd3_error_drop("rgd3_lexer_read_token: no file loaded");
		return 0;
	}
	if (lex->script_p == NULL)
		return 0;

	/* a token from rgd3_lexer_unread_token */
	if (lex->token_available)
	{
		lex->token_available = 0;
		rgd3_token_copy(token, &lex->token);
		return 1;
	}
	lex->last_script_p = lex->script_p;
	lex->last_line = lex->line;
	rgd3_token_clear(token);
	lex->white_space_start_p = lex->script_p;
	token->white_space_start_p = lex->script_p;
	if (!rgd3_lexer_read_white_space(lex))
		return 0;
	lex->white_space_end_p = lex->script_p;
	token->white_space_end_p = lex->script_p;
	token->line = lex->line;
	token->lines_crossed = lex->line - lex->last_line;
	token->flags = 0;

	c = *lex->script_p;

	if (lex->flags & LEXFL_ONLYSTRINGS)
	{
		/* everything is a white-space delimited string */
		if (c == '\"' || c == '\'')
		{
			if (!rgd3_lexer_read_string(lex, token, c))
				return 0;
		}
		else if (!rgd3_lexer_read_name(lex, token))
		{
			return 0;
		}
	}
	else if ((c >= '0' && c <= '9') || (c == '.' && lex->script_p[1] >= '0' && lex->script_p[1] <= '9'))
	{
		if (!rgd3_lexer_read_number(lex, token))
			return 0;
		if (lex->flags & LEXFL_ALLOWNUMBERNAMES)
		{
			c = *lex->script_p;
			if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_')
			{
				if (!rgd3_lexer_read_name(lex, token))
					return 0;
			}
		}
	}
	else if (c == '\"' || c == '\'')
	{
		if (!rgd3_lexer_read_string(lex, token, c))
			return 0;
	}
	else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_')
	{
		if (!rgd3_lexer_read_name(lex, token))
			return 0;
	}
	else if ((lex->flags & LEXFL_ALLOWPATHNAMES) && (c == '/' || c == '\\' || c == '.'))
	{
		/* names may start with a slash when path names are allowed */
		if (!rgd3_lexer_read_name(lex, token))
			return 0;
	}
	else if (!rgd3_lexer_read_punctuation(lex, token))
	{
		rgd3_lexer_error(lex, "unknown punctuation %c", c);
		return 0;
	}
	return 1;
}

int rgd3_lexer_expect_token_string(Rgd3Lexer* lex, const char* string)
{
	Rgd3Token token = { 0 };
	int ok = 0;

	if (!rgd3_lexer_read_token(lex, &token))
		rgd3_lexer_error(lex, "couldn't find expected '%s'", string);
	else if (!rgd3_token_is(&token, string))
		rgd3_lexer_error(lex, "expected '%s' but found '%s'", string, rgd3_token_cstr(&token));
	else
		ok = 1;
	rgd3_token_free(&token);
	return ok;
}

int rgd3_lexer_expect_token_type(Rgd3Lexer* lex, int type, int subtype, Rgd3Token* token)
{
	char str[128];

	if (!rgd3_lexer_read_token(lex, token))
	{
		rgd3_lexer_error(lex, "couldn't read expected token");
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
		rgd3_lexer_error(lex, "expected a %s but found '%s'", name, rgd3_token_cstr(token));
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
			rgd3_lexer_error(lex, "expected %s but found '%s'", str, rgd3_token_cstr(token));
			return 0;
		}
	}
	else if (token->type == TT_PUNCTUATION)
	{
		if (subtype < 0)
		{
			rgd3_lexer_error(lex, "BUG: wrong punctuation subtype");
			return 0;
		}
		if (token->subtype != subtype)
		{
			rgd3_lexer_error(lex, "expected '%s' but found '%s'", rgd3_lexer_get_punctuation_from_id(lex, subtype), rgd3_token_cstr(token));
			return 0;
		}
	}
	return 1;
}

int rgd3_lexer_expect_any_token(Rgd3Lexer* lex, Rgd3Token* token)
{
	if (!rgd3_lexer_read_token(lex, token))
	{
		rgd3_lexer_error(lex, "couldn't read expected token");
		return 0;
	}
	return 1;
}

int rgd3_lexer_check_token_string(Rgd3Lexer* lex, const char* string)
{
	Rgd3Token tok = { 0 };
	int found = 0;

	if (rgd3_lexer_read_token(lex, &tok))
	{
		if (rgd3_token_is(&tok, string))
		{
			found = 1;
		}
		else
		{
			lex->script_p = lex->last_script_p;
			lex->line = lex->last_line;
		}
	}
	rgd3_token_free(&tok);
	return found;
}

int rgd3_lexer_check_token_type(Rgd3Lexer* lex, int type, int subtype, Rgd3Token* token)
{
	Rgd3Token tok = { 0 };
	int found = 0;

	if (rgd3_lexer_read_token(lex, &tok))
	{
		if (tok.type == type && (tok.subtype & subtype) == subtype)
		{
			rgd3_token_copy(token, &tok);
			found = 1;
		}
		else
		{
			lex->script_p = lex->last_script_p;
			lex->line = lex->last_line;
		}
	}
	rgd3_token_free(&tok);
	return found;
}

int rgd3_lexer_peek_token_string(Rgd3Lexer* lex, const char* string)
{
	Rgd3Token tok = { 0 };
	int found;

	if (!rgd3_lexer_read_token(lex, &tok))
	{
		rgd3_token_free(&tok);
		return 0;
	}
	lex->script_p = lex->last_script_p;
	lex->line = lex->last_line;
	found = rgd3_token_is(&tok, string);
	rgd3_token_free(&tok);
	return found;
}

int rgd3_lexer_peek_token_type(Rgd3Lexer* lex, int type, int subtype, Rgd3Token* token)
{
	Rgd3Token tok = { 0 };
	int found = 0;

	if (!rgd3_lexer_read_token(lex, &tok))
	{
		rgd3_token_free(&tok);
		return 0;
	}
	lex->script_p = lex->last_script_p;
	lex->line = lex->last_line;
	if (tok.type == type && (tok.subtype & subtype) == subtype)
	{
		rgd3_token_copy(token, &tok);
		found = 1;
	}
	rgd3_token_free(&tok);
	return found;
}

int rgd3_lexer_skip_until_string(Rgd3Lexer* lex, const char* string)
{
	Rgd3Token token = { 0 };
	int found = 0;

	while (rgd3_lexer_read_token(lex, &token))
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

int rgd3_lexer_skip_rest_of_line(Rgd3Lexer* lex)
{
	Rgd3Token token = { 0 };
	int found = 0;

	while (rgd3_lexer_read_token(lex, &token))
	{
		if (token.lines_crossed)
		{
			lex->script_p = lex->last_script_p;
			lex->line = lex->last_line;
			found = 1;
			break;
		}
	}
	rgd3_token_free(&token);
	return found;
}

/* Skips until the matching close brace; inner braces are counted. */
int rgd3_lexer_skip_braced_section(Rgd3Lexer* lex, int parse_first_brace)
{
	Rgd3Token token = { 0 };
	int depth = parse_first_brace ? 0 : 1;

	do
	{
		if (!rgd3_lexer_read_token(lex, &token))
		{
			rgd3_token_free(&token);
			return 0;
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
	return 1;
}

void rgd3_lexer_unread_token(Rgd3Lexer* lex, const Rgd3Token* token)
{
	if (lex->token_available)
		rgd3_error_fatal("rgd3_lexer_unread_token: unread token twice");
	rgd3_token_copy(&lex->token, token);
	lex->token_available = 1;
}

int rgd3_lexer_read_token_on_line(Rgd3Lexer* lex, Rgd3Token* token)
{
	Rgd3Token tok = { 0 };

	if (!rgd3_lexer_read_token(lex, &tok))
	{
		lex->script_p = lex->last_script_p;
		lex->line = lex->last_line;
		rgd3_token_free(&tok);
		return 0;
	}
	if (!tok.lines_crossed)
	{
		rgd3_token_copy(token, &tok);
		rgd3_token_free(&tok);
		return 1;
	}
	lex->script_p = lex->last_script_p;
	lex->line = lex->last_line;
	rgd3_token_clear(token);
	rgd3_token_free(&tok);
	return 0;
}

/* Appends the rest of the line (control characters become spaces), then
   strips leading and trailing spaces. Does not clear `out` first. */
const char* rgd3_lexer_read_rest_of_line(Rgd3Lexer* lex, Rgd3StrBuf* out)
{
	for (;;)
	{
		if (*lex->script_p == '\n')
		{
			lex->line++;
			break;
		}
		if (!*lex->script_p)
			break;
		if (*(const signed char*)lex->script_p <= ' ')
			rgd3_strbuf_appendc(out, ' ');
		else
			rgd3_strbuf_appendc(out, *lex->script_p);
		lex->script_p++;
	}
	rgd3_strbuf_strip(out, ' ');
	return rgd3_strbuf_cstr(out);
}

int rgd3_lexer_parse_int(Rgd3Lexer* lex)
{
	Rgd3Token token = { 0 };
	int value;

	if (!rgd3_lexer_read_token(lex, &token))
	{
		rgd3_lexer_error(lex, "couldn't read expected integer");
		rgd3_token_free(&token);
		return 0;
	}
	if (token.type == TT_PUNCTUATION && rgd3_token_is(&token, "-"))
	{
		rgd3_lexer_expect_token_type(lex, TT_NUMBER, TT_INTEGER, &token);
		value = -(int)rgd3_token_int_value(&token);
		rgd3_token_free(&token);
		return value;
	}
	else if (token.type != TT_NUMBER || token.subtype == TT_FLOAT)
	{
		rgd3_lexer_error(lex, "expected integer value, found '%s'", rgd3_token_cstr(&token));
	}
	value = rgd3_token_int_value(&token);
	rgd3_token_free(&token);
	return value;
}

int rgd3_lexer_parse_bool(Rgd3Lexer* lex)
{
	Rgd3Token token = { 0 };
	int value;

	if (!rgd3_lexer_expect_token_type(lex, TT_NUMBER, 0, &token))
	{
		rgd3_lexer_error(lex, "couldn't read expected boolean");
		rgd3_token_free(&token);
		return 0;
	}
	value = rgd3_token_int_value(&token) != 0;
	rgd3_token_free(&token);
	return value;
}

/* With error_flag NULL a non-number is an Error(); otherwise a Warning()
   and *error_flag = 1. */
float rgd3_lexer_parse_float(Rgd3Lexer* lex, int* error_flag)
{
	Rgd3Token token = { 0 };
	float value;

	if (error_flag)
		*error_flag = 0;

	if (!rgd3_lexer_read_token(lex, &token))
	{
		if (error_flag)
		{
			rgd3_lexer_warning(lex, "couldn't read expected floating point number");
			*error_flag = 1;
		}
		else
		{
			rgd3_lexer_error(lex, "couldn't read expected floating point number");
		}
		rgd3_token_free(&token);
		return 0;
	}
	if (token.type == TT_PUNCTUATION && rgd3_token_is(&token, "-"))
	{
		rgd3_lexer_expect_token_type(lex, TT_NUMBER, 0, &token);
		value = -rgd3_token_float_value(&token);
		rgd3_token_free(&token);
		return value;
	}
	else if (token.type != TT_NUMBER)
	{
		if (error_flag)
		{
			rgd3_lexer_warning(lex, "expected float value, found '%s'", rgd3_token_cstr(&token));
			*error_flag = 1;
		}
		else
		{
			rgd3_lexer_error(lex, "expected float value, found '%s'", rgd3_token_cstr(&token));
		}
	}
	value = rgd3_token_float_value(&token);
	rgd3_token_free(&token);
	return value;
}

int rgd3_lexer_parse_1d_matrix(Rgd3Lexer* lex, int x, float* m)
{
	int i;
	if (!rgd3_lexer_expect_token_string(lex, "("))
		return 0;
	for (i = 0; i < x; i++)
		m[i] = rgd3_lexer_parse_float(lex, NULL);
	if (!rgd3_lexer_expect_token_string(lex, ")"))
		return 0;
	return 1;
}

int rgd3_lexer_parse_2d_matrix(Rgd3Lexer* lex, int y, int x, float* m)
{
	int i;
	if (!rgd3_lexer_expect_token_string(lex, "("))
		return 0;
	for (i = 0; i < y; i++)
	{
		if (!rgd3_lexer_parse_1d_matrix(lex, x, m + i * x))
			return 0;
	}
	if (!rgd3_lexer_expect_token_string(lex, ")"))
		return 0;
	return 1;
}

int rgd3_lexer_parse_3d_matrix(Rgd3Lexer* lex, int z, int y, int x, float* m)
{
	int i;
	if (!rgd3_lexer_expect_token_string(lex, "("))
		return 0;
	for (i = 0; i < z; i++)
	{
		if (!rgd3_lexer_parse_2d_matrix(lex, y, x, m + i * x * y))
			return 0;
	}
	if (!rgd3_lexer_expect_token_string(lex, ")"))
		return 0;
	return 1;
}

/* The next token must be '{'. Copies the exact characters up to the
   matching '}'; with tabs >= 0, re-indents each line. */
const char* rgd3_lexer_parse_braced_section_exact(Rgd3Lexer* lex, Rgd3StrBuf* out, int tabs)
{
	int depth, do_tabs, skip_white;

	rgd3_strbuf_clear(out);
	if (!rgd3_lexer_expect_token_string(lex, "{"))
		return rgd3_strbuf_cstr(out);

	rgd3_strbuf_set(out, "{");
	depth = 1;
	skip_white = 0;
	do_tabs = tabs >= 0;

	while (depth && *lex->script_p)
	{
		char c = *(lex->script_p++);

		switch (c)
		{
		case '\t':
		case ' ':
			if (skip_white)
				continue;
			break;
		case '\n':
			if (do_tabs)
			{
				skip_white = 1;
				rgd3_strbuf_appendc(out, c);
				continue;
			}
			break;
		case '{':
			depth++;
			tabs++;
			break;
		case '}':
			depth--;
			tabs--;
			break;
		}

		if (skip_white)
		{
			int i = tabs;
			if (c == '{')
				i--;
			skip_white = 0;
			for (; i > 0; i--)
				rgd3_strbuf_appendc(out, '\t');
		}
		rgd3_strbuf_appendc(out, c);
	}
	return rgd3_strbuf_cstr(out);
}

/* The next token must be '{'. Re-emits tokens up to the matching '}',
   separated by spaces, strings re-quoted, "\r\n" per crossed line. */
const char* rgd3_lexer_parse_braced_section(Rgd3Lexer* lex, Rgd3StrBuf* out)
{
	Rgd3Token token = { 0 };
	int i, depth;

	rgd3_strbuf_clear(out);
	if (!rgd3_lexer_expect_token_string(lex, "{"))
		return rgd3_strbuf_cstr(out);
	rgd3_strbuf_set(out, "{");
	depth = 1;
	do
	{
		if (!rgd3_lexer_read_token(lex, &token))
		{
			rgd3_lexer_error(lex, "missing closing brace");
			break;
		}
		for (i = 0; i < token.lines_crossed; i++)
			rgd3_strbuf_append(out, "\r\n");
		if (token.type == TT_PUNCTUATION)
		{
			if (token.data[0] == '{')
				depth++;
			else if (token.data[0] == '}')
				depth--;
		}
		if (token.type == TT_STRING)
		{
			rgd3_strbuf_appendc(out, '\"');
			rgd3_strbuf_append(out, token.data);
			rgd3_strbuf_appendc(out, '\"');
		}
		else
		{
			rgd3_strbuf_append(out, token.data);
		}
		rgd3_strbuf_appendc(out, ' ');
	} while (depth);
	rgd3_token_free(&token);
	return rgd3_strbuf_cstr(out);
}

const char* rgd3_lexer_parse_rest_of_line(Rgd3Lexer* lex, Rgd3StrBuf* out)
{
	Rgd3Token token = { 0 };

	rgd3_strbuf_clear(out);
	while (rgd3_lexer_read_token(lex, &token))
	{
		if (token.lines_crossed)
		{
			lex->script_p = lex->last_script_p;
			lex->line = lex->last_line;
			break;
		}
		if (out->len)
			rgd3_strbuf_appendc(out, ' ');
		rgd3_strbuf_append(out, token.data);
	}
	rgd3_token_free(&token);
	return rgd3_strbuf_cstr(out);
}

/* Up to and including the '\n'; leading white space of the next line is
   left alone. */
const char* rgd3_lexer_parse_complete_line(Rgd3Lexer* lex, Rgd3StrBuf* out)
{
	const char* start = lex->script_p;

	for (;;)
	{
		if (*lex->script_p == 0)
			break;
		if (*lex->script_p == '\n')
		{
			lex->line++;
			lex->script_p++;
			break;
		}
		lex->script_p++;
	}
	rgd3_strbuf_clear(out);
	rgd3_strbuf_appendn(out, start, (int)(lex->script_p - start));
	return rgd3_strbuf_cstr(out);
}

int rgd3_lexer_get_last_white_space(const Rgd3Lexer* lex, Rgd3StrBuf* out)
{
	rgd3_strbuf_clear(out);
	if (lex->white_space_start_p && lex->white_space_end_p > lex->white_space_start_p)
		rgd3_strbuf_appendn(out, lex->white_space_start_p, (int)(lex->white_space_end_p - lex->white_space_start_p));
	return out->len;
}

int rgd3_lexer_get_last_white_space_start(const Rgd3Lexer* lex)
{
	return (int)(lex->white_space_start_p - lex->buffer);
}

int rgd3_lexer_get_last_white_space_end(const Rgd3Lexer* lex)
{
	return (int)(lex->white_space_end_p - lex->buffer);
}

/* ---- lifetime ---- */

static void rgd3_lexer_unwind(void* object)
{
	rgd3_lexer_free((Rgd3Lexer*)object);
}

void rgd3_lexer_init(Rgd3Lexer* lex, int flags)
{
	memset(lex, 0, sizeof(*lex));
	rgd3_token_init(&lex->token);
	lex->flags = flags;
	rgd3_lexer_set_punctuations(lex, NULL);
	lex->initialized = 1;
	rgd3_error_unwind_register(lex, rgd3_lexer_unwind);
}

void rgd3_lexer_reset(Rgd3Lexer* lex)
{
	lex->script_p = lex->buffer;
	lex->last_script_p = lex->buffer;
	lex->white_space_start_p = NULL;
	lex->white_space_end_p = NULL;
	lex->token_available = 0;
	lex->line = 1;
	lex->last_line = 1;
	rgd3_token_clear(&lex->token);
}

int rgd3_lexer_load_file(Rgd3Lexer* lex, const char* filename, int os_path)
{
	char pathname[RGD3_MAX_OSPATH];
	char* buf;
	int length;

	if (!lex->initialized)
		rgd3_lexer_init(lex, 0);
	if (lex->loaded)
	{
		rgd3_error_drop("rgd3_lexer_load_file: another script already loaded");
		return 0;
	}
	if (!os_path && rgd3_lexer_base_folder[0])
		rg_snprintf(pathname, sizeof(pathname), "%s/%s", rgd3_lexer_base_folder, filename);
	else
		rgd3_str_copynz(pathname, filename, sizeof(pathname));

	if (os_path)
	{
		Rgd3File* fp = rgd3_fs_open_explicit_read(pathname);
		if (!fp)
			return 0;
		length = (int)rgd3_fs_length(fp);
		buf = (char*)rgd3_heap_alloc((size_t)length + 1, RGD3_TAG_LEXER);
		buf[length] = 0;
		length = (int)rgd3_fs_read(fp, buf, (size_t)length);
		buf[length] = 0;
		rgd3_fs_close(fp);
		lex->file_time = 0;
	}
	else
	{
		void* data;
		length = rgd3_fs_read_file(pathname, &data, &lex->file_time);
		if (length < 0)
			return 0;
		buf = (char*)data; /* NUL-terminated by rgd3_fs_read_file */
	}
	rgd3_str_copynz(lex->filename, pathname, sizeof(lex->filename));

	lex->buffer = buf;
	lex->length = length;
	lex->script_p = lex->buffer;
	lex->last_script_p = lex->buffer;
	lex->end_p = lex->buffer + length;
	lex->token_available = 0;
	lex->line = 1;
	lex->last_line = 1;
	lex->allocated = 1;
	lex->loaded = 1;
	return 1;
}

/* ptr[length] must be '\0'. start_line lets text extracted from a file keep
   its line numbers. The memory is not copied. */
int rgd3_lexer_load_memory(Rgd3Lexer* lex, const char* ptr, int length, const char* name, int start_line)
{
	if (!lex->initialized)
		rgd3_lexer_init(lex, 0);
	if (lex->loaded)
	{
		rgd3_error_drop("rgd3_lexer_load_memory: another script already loaded");
		return 0;
	}
	rgd3_str_copynz(lex->filename, name ? name : "", sizeof(lex->filename));
	lex->buffer = ptr;
	lex->file_time = 0;
	lex->length = length;
	lex->script_p = lex->buffer;
	lex->last_script_p = lex->buffer;
	lex->end_p = lex->buffer + length;
	lex->token_available = 0;
	lex->line = start_line;
	lex->last_line = start_line;
	lex->allocated = 0;
	lex->loaded = 1;
	return 1;
}

void rgd3_lexer_free_source(Rgd3Lexer* lex)
{
	if (lex->punctuation_table && lex->punctuation_table != rgd3_lexer_default_table)
	{
		rgd3_heap_free(lex->punctuation_table);
		lex->punctuation_table = NULL;
	}
	if (lex->next_punctuation && lex->next_punctuation != rgd3_lexer_default_next)
	{
		rgd3_heap_free(lex->next_punctuation);
		lex->next_punctuation = NULL;
	}
	if (lex->allocated)
	{
		rgd3_heap_free((void*)lex->buffer);
		lex->buffer = NULL;
		lex->allocated = 0;
	}
	lex->token_available = 0;
	rgd3_token_clear(&lex->token);
	lex->loaded = 0;
}

void rgd3_lexer_free(Rgd3Lexer* lex)
{
	if (!lex->initialized)
		return;
	rgd3_lexer_free_source(lex);
	rgd3_token_free(&lex->token);
	lex->initialized = 0;
	rgd3_error_unwind_unregister(lex);
}

int rgd3_lexer_is_loaded(const Rgd3Lexer* lex)
{
	return lex->loaded;
}

void rgd3_lexer_set_flags(Rgd3Lexer* lex, int flags)
{
	lex->flags = flags;
}

int rgd3_lexer_get_flags(const Rgd3Lexer* lex)
{
	return lex->flags;
}

int rgd3_lexer_end_of_file(const Rgd3Lexer* lex)
{
	return lex->script_p >= lex->end_p;
}

const char* rgd3_lexer_file_name(const Rgd3Lexer* lex)
{
	return lex->filename;
}

int rgd3_lexer_file_offset(const Rgd3Lexer* lex)
{
	return (int)(lex->script_p - lex->buffer);
}

Rgd3TimeStamp rgd3_lexer_file_time(const Rgd3Lexer* lex)
{
	return lex->file_time;
}

int rgd3_lexer_line_num(const Rgd3Lexer* lex)
{
	return lex->line;
}

void rgd3_lexer_set_base_folder(const char* path)
{
	rgd3_str_copynz(rgd3_lexer_base_folder, path, sizeof(rgd3_lexer_base_folder));
}
