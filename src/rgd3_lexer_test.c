/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_lexer_test.c - self test for rgd3_lexer.c. The expectations follow
   neo/idlib/Lexer.cpp and neo/idlib/Token.cpp, quirks included: decimal
   (not octal) escapes, bytes >= 0x80 as white space, number suffixes left
   out of the token text, "1.#INF" reading as a plain float because
   CheckString looks at the '#' itself, and ReadWhiteSpace eating the
   character right after a block comment. */

typedef struct Rgd3LexerTest
{
	int checks;
	int failures;
} Rgd3LexerTest;

static void rgd3_lexer_test_check(Rgd3LexerTest* t, int ok, const char* what)
{
	t->checks++;
	if (!ok)
	{
		t->failures++;
		rgd3_printf("lexer test FAILED: %s\n", what);
	}
}

static int rgd3_lexer_test_type_char(int type)
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

/* The whole source must produce exactly the expected tokens. expected is
   NULL-terminated; each entry is a type character (s string, l literal,
   n number, w name, p punctuation, * any) and the text. */
static void rgd3_lexer_test_stream(Rgd3LexerTest* t, const char* what, const char* src, int flags, const char* const* expected)
{
	Rgd3Lexer lex;
	Rgd3Token tok = { 0 };
	int i = 0, ok = 1;

	rgd3_lexer_init(&lex, flags);
	rgd3_lexer_load_memory(&lex, src, (int)strlen(src), what, 1);
	while (rgd3_lexer_read_token(&lex, &tok))
	{
		if (!expected[i])
		{
			rgd3_printf("  %s: unexpected trailing token '%s'\n", what, rgd3_token_cstr(&tok));
			ok = 0;
			break;
		}
		if ((expected[i][0] != '*' && expected[i][0] != rgd3_lexer_test_type_char(tok.type)) || !rgd3_token_is(&tok, expected[i] + 1))
		{
			rgd3_printf("  %s: token %d is %c'%s', expected '%s'\n", what, i, rgd3_lexer_test_type_char(tok.type), rgd3_token_cstr(&tok), expected[i]);
			ok = 0;
			break;
		}
		i++;
	}
	if (ok && expected[i])
	{
		rgd3_printf("  %s: ended before token %d '%s'\n", what, i, expected[i]);
		ok = 0;
	}
	rgd3_lexer_test_check(t, ok, what);
	rgd3_token_free(&tok);
	rgd3_lexer_free(&lex);
}

/* loads src into lex; free with rgd3_lexer_free */
static void rgd3_lexer_test_load(Rgd3Lexer* lex, const char* src, int flags, const char* name)
{
	rgd3_lexer_init(lex, flags);
	rgd3_lexer_load_memory(lex, src, (int)strlen(src), name, 1);
}

static void rgd3_lexer_test_strings(Rgd3LexerTest* t)
{
	static const char* const concat[] = { "sabcdef", "wx", NULL };
	static const char* const no_concat[] = { "sabc", "sdef", NULL };
	static const char* const backslash_concat[] = { "sabcd", "sef", NULL };
	static const char* const paths[] = { "wtextures/base_wall/a.tga", "w/abs/path", "w..\\up", "wc:/dir", NULL };
	static const char* const no_paths[] = { "wtextures", "p/", "wbase_wall", NULL };
	static const char* const only_strings[] = { "wa-b", "wc_d", "w12", "sq r", "w{", NULL };
	Rgd3Lexer lex;
	Rgd3Token tok = { 0 };

	rgd3_lexer_test_stream(t, "string concatenation", "\"abc\"  \n \"def\" x", 0, concat);
	rgd3_lexer_test_stream(t, "LEXFL_NOSTRINGCONCAT", "\"abc\" \"def\"", LEXFL_NOSTRINGCONCAT, no_concat);
	rgd3_lexer_test_stream(t, "LEXFL_ALLOWBACKSLASHSTRINGCONCAT", "\"ab\" \\\n \"cd\" \"ef\"", LEXFL_NOSTRINGCONCAT | LEXFL_ALLOWBACKSLASHSTRINGCONCAT, backslash_concat);

	/* \" \n \65 (decimal) \x41 */
	rgd3_lexer_test_load(&lex, "\"a\\\"b\\n\\65\\x41\" 'c' \"t\\\\\"", 0, "escapes");
	rgd3_lexer_test_check(t, rgd3_lexer_read_token(&lex, &tok) && tok.type == TT_STRING && !strcmp(rgd3_token_cstr(&tok), "a\"b\nAA") && tok.subtype == 6, "string escapes");
	rgd3_lexer_test_check(t, rgd3_lexer_read_token(&lex, &tok) && tok.type == TT_LITERAL && tok.subtype == 'c' && rgd3_token_is(&tok, "c"), "literal");
	rgd3_lexer_test_check(t, rgd3_lexer_read_token(&lex, &tok) && rgd3_token_is(&tok, "t\\"), "escaped backslash");
	rgd3_lexer_free(&lex);

	rgd3_lexer_test_load(&lex, "\"a\\nb\"", LEXFL_NOSTRINGESCAPECHARS, "no escapes");
	rgd3_lexer_test_check(t, rgd3_lexer_read_token(&lex, &tok) && rgd3_token_is(&tok, "a\\nb"), "LEXFL_NOSTRINGESCAPECHARS");
	rgd3_lexer_free(&lex);

	/* a newline inside a string is an error */
	rgd3_lexer_test_load(&lex, "\"abc\ndef\"", LEXFL_NOERRORS, "newline in string");
	rgd3_lexer_test_check(t, !rgd3_lexer_read_token(&lex, &tok) && rgd3_lexer_had_error(&lex), "newline inside string");
	rgd3_lexer_free(&lex);

	/* long strings move to the heap */
	{
		char src[600];
		int i;
		src[0] = '\"';
		for (i = 1; i < 501; i++)
			src[i] = (char)('a' + i % 26);
		src[501] = '\"';
		src[502] = 0;
		rgd3_lexer_test_load(&lex, src, 0, "long string");
		rgd3_lexer_test_check(t, rgd3_lexer_read_token(&lex, &tok) && tok.len == 500 && rgd3_token_cstr(&tok)[499] == src[500], "long string");
		rgd3_lexer_free(&lex);
	}

	rgd3_lexer_test_stream(t, "LEXFL_ALLOWPATHNAMES", "textures/base_wall/a.tga /abs/path ..\\up c:/dir", LEXFL_ALLOWPATHNAMES, paths);
	rgd3_lexer_test_stream(t, "path names off", "textures/base_wall", 0, no_paths);
	rgd3_lexer_test_stream(t, "LEXFL_ONLYSTRINGS", "a-b c_d 12 \"q r\" {", LEXFL_ONLYSTRINGS, only_strings);
	rgd3_token_free(&tok);
}

static void rgd3_lexer_test_numbers(Rgd3LexerTest* t)
{
	static const char* const number_names[] = { "w3dtext", "n42", NULL };
	Rgd3Lexer lex;
	Rgd3Token tok = { 0 };

	rgd3_lexer_test_load(&lex, "0x1F 017 0b101 3.25 1.5e3 2e2 .5 10ul 7.0f 0 2.5e-1", 0, "numbers");
	rgd3_lexer_test_check(t, rgd3_lexer_read_token(&lex, &tok) && tok.type == TT_NUMBER && tok.subtype == (TT_HEX | TT_INTEGER) && rgd3_token_int_value(&tok) == 31, "hex");
	rgd3_lexer_test_check(t, rgd3_lexer_read_token(&lex, &tok) && tok.subtype == (TT_OCTAL | TT_INTEGER) && rgd3_token_int_value(&tok) == 15, "octal");
	rgd3_lexer_test_check(t, rgd3_lexer_read_token(&lex, &tok) && tok.subtype == (TT_BINARY | TT_INTEGER) && rgd3_token_int_value(&tok) == 5, "binary");
	rgd3_lexer_test_check(t, rgd3_lexer_read_token(&lex, &tok) && tok.subtype == (TT_DECIMAL | TT_FLOAT | TT_DOUBLE_PRECISION) && rgd3_token_float_value(&tok) == 3.25f && rgd3_token_int_value(&tok) == 3, "float");
	rgd3_lexer_test_check(t, rgd3_lexer_read_token(&lex, &tok) && rgd3_token_is(&tok, "1.5e3") && rgd3_token_float_value(&tok) == 1500.0f, "exponent");
	rgd3_lexer_test_check(t, rgd3_lexer_read_token(&lex, &tok) && (tok.subtype & TT_FLOAT) && rgd3_token_float_value(&tok) == 200.0f, "exponent without a dot");
	rgd3_lexer_test_check(t, rgd3_lexer_read_token(&lex, &tok) && rgd3_token_is(&tok, ".5") && rgd3_token_float_value(&tok) == 0.5f, "leading dot");
	rgd3_lexer_test_check(t, rgd3_lexer_read_token(&lex, &tok) && rgd3_token_is(&tok, "10") && tok.subtype == (TT_DECIMAL | TT_INTEGER | TT_LONG | TT_UNSIGNED), "integer suffixes");
	rgd3_lexer_test_check(t, rgd3_lexer_read_token(&lex, &tok) && rgd3_token_is(&tok, "7.0") && (tok.subtype & TT_SINGLE_PRECISION), "float suffix");
	rgd3_lexer_test_check(t, rgd3_lexer_read_token(&lex, &tok) && tok.subtype == (TT_OCTAL | TT_INTEGER) && rgd3_token_int_value(&tok) == 0, "zero");
	rgd3_lexer_test_check(t, rgd3_lexer_read_token(&lex, &tok) && rgd3_token_float_value(&tok) == 0.25f, "negative exponent");
	rgd3_lexer_free(&lex);

	/* 1.#INF: TT_INFINITE is never set (CheckString starts at the '#') */
	rgd3_lexer_test_load(&lex, "1.#INF 2", LEXFL_ALLOWFLOATEXCEPTIONS, "float exception");
	rgd3_lexer_test_check(t, rgd3_lexer_read_token(&lex, &tok) && rgd3_token_is(&tok, "1.#INF") && (tok.subtype & TT_FLOAT) && !(tok.subtype & TT_INFINITE) && !rgd3_lexer_had_error(&lex), "1.#INF with LEXFL_ALLOWFLOATEXCEPTIONS");
	rgd3_lexer_test_check(t, rgd3_lexer_read_token(&lex, &tok) && rgd3_token_is(&tok, "2"), "after 1.#INF");
	rgd3_lexer_free(&lex);

	rgd3_lexer_test_load(&lex, "1.#INF", LEXFL_NOERRORS, "float exception error");
	rgd3_lexer_test_check(t, rgd3_lexer_read_token(&lex, &tok) && rgd3_lexer_had_error(&lex), "1.#INF without LEXFL_ALLOWFLOATEXCEPTIONS");
	rgd3_lexer_free(&lex);

	rgd3_lexer_test_load(&lex, "192.168.0.1:28000 10.0.0.2", LEXFL_ALLOWIPADDRESSES, "ip");
	rgd3_lexer_test_check(t, rgd3_lexer_read_token(&lex, &tok) && rgd3_token_is(&tok, "192.168.0.1:28000") && tok.subtype == (TT_IPADDRESS | TT_IPPORT), "ip address with port");
	rgd3_lexer_test_check(t, rgd3_lexer_read_token(&lex, &tok) && rgd3_token_is(&tok, "10.0.0.2") && tok.subtype == TT_IPADDRESS, "ip address");
	rgd3_lexer_free(&lex);

	rgd3_lexer_test_load(&lex, "1.2.3", LEXFL_NOERRORS, "two dots");
	rgd3_lexer_test_check(t, !rgd3_lexer_read_token(&lex, &tok) && rgd3_lexer_had_error(&lex), "more than one dot");
	rgd3_lexer_free(&lex);

	rgd3_lexer_test_stream(t, "LEXFL_ALLOWNUMBERNAMES", "3dtext 42", LEXFL_ALLOWNUMBERNAMES, number_names);

	/* ParseInt / ParseFloat / ParseBool */
	rgd3_lexer_test_load(&lex, "-5 12 -2.5 3 1 0 x", LEXFL_NOWARNINGS, "parse numbers");
	rgd3_lexer_test_check(t, rgd3_lexer_parse_int(&lex) == -5, "ParseInt -5");
	rgd3_lexer_test_check(t, rgd3_lexer_parse_int(&lex) == 12, "ParseInt");
	rgd3_lexer_test_check(t, rgd3_lexer_parse_float(&lex, NULL) == -2.5f, "ParseFloat -2.5");
	rgd3_lexer_test_check(t, rgd3_lexer_parse_float(&lex, NULL) == 3.0f, "ParseFloat integer");
	rgd3_lexer_test_check(t, rgd3_lexer_parse_bool(&lex) == 1 && rgd3_lexer_parse_bool(&lex) == 0, "ParseBool");
	{
		int error_flag = 0;
		float f = rgd3_lexer_parse_float(&lex, &error_flag);
		rgd3_lexer_test_check(t, f == 0.0f && error_flag == 1, "ParseFloat error flag");
	}
	rgd3_lexer_free(&lex);

	{
		float m[2][3];
		rgd3_lexer_test_load(&lex, "( ( 1 2 3 ) ( -4 5.5 6 ) )", 0, "matrix");
		rgd3_lexer_test_check(t, rgd3_lexer_parse_2d_matrix(&lex, 2, 3, &m[0][0]) && m[0][2] == 3.0f && m[1][0] == -4.0f && m[1][1] == 5.5f, "Parse2DMatrix");
		rgd3_lexer_free(&lex);
	}
	rgd3_token_free(&tok);
}

static void rgd3_lexer_test_punctuation(Rgd3LexerTest* t)
{
	static const char* const longest[] = {
		"wa", "p>>=", "wb", "p...", "wc", "p->", "wd", "p>>", "we", "p##", "wf", "p<<=", "p<", "p.", "p&&", "p&", "p!=", "p::", NULL
	};
	Rgd3Lexer lex;
	Rgd3Token tok = { 0 };

	rgd3_lexer_test_stream(t, "punctuation longest match", "a>>=b...c->d >> e ## f <<=< . && & != ::", 0, longest);

	rgd3_lexer_test_load(&lex, ">>= ...", 0, "punctuation ids");
	rgd3_lexer_test_check(t, rgd3_lexer_read_token(&lex, &tok) && tok.type == TT_PUNCTUATION && tok.subtype == P_RSHIFT_ASSIGN, "P_RSHIFT_ASSIGN");
	rgd3_lexer_test_check(t, rgd3_lexer_read_token(&lex, &tok) && tok.subtype == P_PARMS, "P_PARMS");
	rgd3_lexer_test_check(t, !strcmp(rgd3_lexer_get_punctuation_from_id(&lex, P_POINTERREF), "->") && rgd3_lexer_get_punctuation_id(&lex, "##") == P_PRECOMPMERGE, "punctuation lookup");
	rgd3_lexer_free(&lex);

	/* an unknown punctuation is an error */
	rgd3_lexer_test_load(&lex, "@", LEXFL_NOERRORS, "unknown punctuation");
	rgd3_lexer_test_check(t, !rgd3_lexer_read_token(&lex, &tok) && rgd3_lexer_had_error(&lex), "unknown punctuation");
	rgd3_lexer_free(&lex);
	rgd3_token_free(&tok);
}

static void rgd3_lexer_test_white_space(Rgd3LexerTest* t)
{
	static const char* const high_bytes[] = { "wa", "wb", "wc", "sd\xe9", NULL };
	static const char* const eaten[] = { "wa", "wc", NULL };
	Rgd3Lexer lex;
	Rgd3Token tok = { 0 };
	Rgd3StrBuf ws = { 0 };

	/* comments, lines and lines_crossed */
	rgd3_lexer_test_load(&lex, "a // comment\nb /* multi\nline */ c\n\n/* x */ d", 0, "comments");
	rgd3_lexer_test_check(t, rgd3_lexer_read_token(&lex, &tok) && rgd3_token_is(&tok, "a") && tok.line == 1 && tok.lines_crossed == 0, "token before a comment");
	rgd3_lexer_test_check(t, rgd3_lexer_read_token(&lex, &tok) && rgd3_token_is(&tok, "b") && tok.line == 2 && tok.lines_crossed == 1, "after // comment");
	rgd3_lexer_test_check(t, rgd3_lexer_read_token(&lex, &tok) && rgd3_token_is(&tok, "c") && tok.line == 3 && tok.lines_crossed == 1, "after /* */ comment");
	rgd3_lexer_test_check(t, rgd3_lexer_read_token(&lex, &tok) && rgd3_token_is(&tok, "d") && tok.line == 5 && tok.lines_crossed == 2, "lines_crossed");
	rgd3_lexer_test_check(t, rgd3_lexer_get_last_white_space(&lex, &ws) == 10 && !strcmp(rgd3_strbuf_cstr(&ws), "\n\n/* x */ "), "GetLastWhiteSpace");
	rgd3_lexer_test_check(t, !rgd3_lexer_read_token(&lex, &tok) && rgd3_lexer_end_of_file(&lex), "end of file");
	rgd3_lexer_free(&lex);

	/* ReadWhiteSpace steps one character past a closing "*" "/" */
	rgd3_lexer_test_stream(t, "character after a block comment", "a/*c*/bc", 0, eaten);

	/* bytes >= 0x80 outside strings are white space (signed char) */
	rgd3_lexer_test_stream(t, "high bytes as white space", "a\x80\xff" "b\xc3\xa9" "c \"d\xe9\"", 0, high_bytes);

	/* the start line of memory sources */
	rgd3_lexer_init(&lex, 0);
	rgd3_lexer_load_memory(&lex, "x\ny", 3, "start line", 10);
	rgd3_lexer_read_token(&lex, &tok);
	rgd3_lexer_test_check(t, rgd3_lexer_read_token(&lex, &tok) && tok.line == 11 && rgd3_lexer_line_num(&lex) == 11, "start line");
	rgd3_lexer_free(&lex);

	rgd3_strbuf_free(&ws);
	rgd3_token_free(&tok);
}

static void rgd3_lexer_test_read_api(Rgd3LexerTest* t)
{
	Rgd3Lexer lex;
	Rgd3Token tok = { 0 };
	Rgd3StrBuf out = { 0 };

	/* ReadTokenOnLine */
	rgd3_lexer_test_load(&lex, "a b\nc", 0, "on line");
	rgd3_lexer_read_token(&lex, &tok);
	rgd3_lexer_test_check(t, rgd3_lexer_read_token_on_line(&lex, &tok) && rgd3_token_is(&tok, "b"), "ReadTokenOnLine");
	rgd3_lexer_test_check(t, !rgd3_lexer_read_token_on_line(&lex, &tok) && tok.len == 0, "ReadTokenOnLine at end of line");
	rgd3_lexer_test_check(t, rgd3_lexer_read_token(&lex, &tok) && rgd3_token_is(&tok, "c") && tok.line == 2, "after ReadTokenOnLine");
	rgd3_lexer_free(&lex);

	/* CheckTokenString / PeekTokenString / CheckTokenType / PeekTokenType */
	rgd3_lexer_test_load(&lex, "x y 5", 0, "check and peek");
	rgd3_lexer_test_check(t, rgd3_lexer_peek_token_string(&lex, "x"), "PeekTokenString");
	rgd3_lexer_test_check(t, !rgd3_lexer_check_token_string(&lex, "y"), "CheckTokenString mismatch");
	rgd3_lexer_test_check(t, rgd3_lexer_check_token_string(&lex, "x"), "CheckTokenString");
	rgd3_lexer_test_check(t, rgd3_lexer_peek_token_string(&lex, "y") && !rgd3_lexer_peek_token_string(&lex, "x"), "PeekTokenString after Check");
	rgd3_lexer_test_check(t, !rgd3_lexer_check_token_type(&lex, TT_NUMBER, 0, &tok), "CheckTokenType mismatch");
	rgd3_lexer_test_check(t, rgd3_lexer_read_token(&lex, &tok) && rgd3_token_is(&tok, "y"), "read after Peek");
	rgd3_lexer_test_check(t, rgd3_lexer_peek_token_type(&lex, TT_NUMBER, TT_INTEGER, &tok) && rgd3_token_is(&tok, "5"), "PeekTokenType");
	rgd3_lexer_test_check(t, rgd3_lexer_check_token_type(&lex, TT_NUMBER, TT_DECIMAL, &tok) && rgd3_token_int_value(&tok) == 5, "CheckTokenType");
	rgd3_lexer_free(&lex);

	/* UnreadToken */
	rgd3_lexer_test_load(&lex, "a b", 0, "unread");
	rgd3_lexer_read_token(&lex, &tok);
	rgd3_lexer_unread_token(&lex, &tok);
	rgd3_lexer_test_check(t, rgd3_lexer_read_token(&lex, &tok) && rgd3_token_is(&tok, "a"), "UnreadToken");
	rgd3_lexer_test_check(t, rgd3_lexer_read_token(&lex, &tok) && rgd3_token_is(&tok, "b"), "after UnreadToken");
	rgd3_lexer_free(&lex);

	/* ParseBracedSection, then the next token */
	rgd3_lexer_test_load(&lex, "{ a \"s\" { b }\n c } tail", 0, "braced");
	rgd3_lexer_test_check(t, !strcmp(rgd3_lexer_parse_braced_section(&lex, &out), "{a \"s\" { b } \r\nc } "), "ParseBracedSection");
	rgd3_lexer_test_check(t, rgd3_lexer_read_token(&lex, &tok) && rgd3_token_is(&tok, "tail"), "after ParseBracedSection");
	rgd3_lexer_free(&lex);

	rgd3_lexer_test_load(&lex, "{ a\n  { b }\n} z", 0, "braced exact");
	rgd3_lexer_test_check(t, !strcmp(rgd3_lexer_parse_braced_section_exact(&lex, &out, -1), "{ a\n  { b }\n}"), "ParseBracedSectionExact");
	rgd3_lexer_free(&lex);

	rgd3_lexer_test_load(&lex, "{ a\n  { b }\n}", 0, "braced exact tabs");
	rgd3_lexer_test_check(t, !strcmp(rgd3_lexer_parse_braced_section_exact(&lex, &out, 0), "{ a\n{ b }\n}"), "ParseBracedSectionExact with tabs");
	rgd3_lexer_free(&lex);

	/* skipping */
	rgd3_lexer_test_load(&lex, "{ a { b } } c d\ne f ; g", 0, "skip");
	rgd3_lexer_test_check(t, rgd3_lexer_skip_braced_section(&lex, 1), "SkipBracedSection");
	rgd3_lexer_test_check(t, !strcmp(rgd3_lexer_parse_rest_of_line(&lex, &out), "c d"), "ParseRestOfLine");
	rgd3_lexer_test_check(t, rgd3_lexer_skip_until_string(&lex, ";"), "SkipUntilString");
	rgd3_lexer_test_check(t, rgd3_lexer_read_token(&lex, &tok) && rgd3_token_is(&tok, "g"), "after SkipUntilString");
	rgd3_lexer_free(&lex);

	rgd3_lexer_test_load(&lex, "a b c\nd", 0, "skip line");
	rgd3_lexer_read_token(&lex, &tok);
	rgd3_lexer_test_check(t, rgd3_lexer_skip_rest_of_line(&lex) && rgd3_lexer_read_token(&lex, &tok) && rgd3_token_is(&tok, "d"), "SkipRestOfLine");
	rgd3_lexer_free(&lex);

	rgd3_lexer_test_load(&lex, "  key  value with\tspaces  \nnext", 0, "complete line");
	rgd3_lexer_read_token(&lex, &tok);
	rgd3_strbuf_clear(&out);
	rgd3_lexer_test_check(t, !strcmp(rgd3_lexer_read_rest_of_line(&lex, &out), "value with spaces"), "ReadRestOfLine");
	rgd3_lexer_free(&lex);

	/* ExpectTokenType / ExpectTokenString only flag errors with NOERRORS */
	rgd3_lexer_test_load(&lex, "abc 12 ; 1.5", LEXFL_NOERRORS, "expect");
	rgd3_lexer_test_check(t, !rgd3_lexer_expect_token_type(&lex, TT_NUMBER, 0, &tok) && rgd3_lexer_had_error(&lex), "ExpectTokenType wrong type");
	rgd3_lexer_test_check(t, !rgd3_lexer_expect_token_type(&lex, TT_NUMBER, TT_FLOAT, &tok), "ExpectTokenType wrong subtype");
	rgd3_lexer_test_check(t, !rgd3_lexer_expect_token_type(&lex, TT_PUNCTUATION, P_COMMA, &tok), "ExpectTokenType wrong punctuation");
	rgd3_lexer_test_check(t, !rgd3_lexer_expect_token_string(&lex, "2.5"), "ExpectTokenString mismatch");
	rgd3_lexer_test_check(t, !rgd3_lexer_expect_any_token(&lex, &tok), "ExpectAnyToken at end");
	rgd3_lexer_free(&lex);

	rgd3_strbuf_free(&out);
	rgd3_token_free(&tok);
}

int rgd3_lexer_self_test(void)
{
	Rgd3LexerTest t = { 0 };
	i64 lexer_count = rgd3_mem_tag_count(RGD3_TAG_LEXER);
	i64 string_count = rgd3_mem_tag_count(RGD3_TAG_STRING);

	rgd3_lexer_test_strings(&t);
	rgd3_lexer_test_numbers(&t);
	rgd3_lexer_test_punctuation(&t);
	rgd3_lexer_test_white_space(&t);
	rgd3_lexer_test_read_api(&t);
	rgd3_lexer_test_check(&t, rgd3_mem_tag_count(RGD3_TAG_LEXER) == lexer_count, "no RGD3_TAG_LEXER leaks");
	rgd3_lexer_test_check(&t, rgd3_mem_tag_count(RGD3_TAG_STRING) == string_count, "no RGD3_TAG_STRING leaks");
	rgd3_printf("lexer: %d checks, %d failures\n", t.checks, t.failures);
	return t.failures == 0;
}
