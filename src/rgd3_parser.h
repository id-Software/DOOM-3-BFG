/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_parser.h - the C/C++ compatible pre-compiler (idParser,
   neo/idlib/Parser.h/.cpp) on top of rgd3_lexer.

   An Rgd3Parser is zero-initialised or set up with rgd3_parser_init(), loaded
   with rgd3_parser_load_file()/load_memory() and released with
   rgd3_parser_free() (the destructor: rgd3_parser_free_source(p, 0)).
   rgd3_parser_free_source() is idempotent and also reclaims every token a
   dropped error left behind, so error recovery only has to call it.

   Errors go through the current script's lexer and its flags, as in the
   original: without LEXFL_NOERRORS/NOFATALERRORS they drop
   (rgd3_error_drop); with no script loaded they are silently ignored.

   Global defines (rgd3_parser_add_global_define) are copied into every
   source loaded afterwards; rgd3_parser_shutdown() frees them. */
#ifndef RGD3_PARSER_H
#define RGD3_PARSER_H
#ifndef RGD3_ALL_C
#error "rgd3 headers are included through src/all.c only"
#endif

#define DEFINE_FIXED 0x0001

#define BUILTIN_LINE 1
#define BUILTIN_FILE 2
#define BUILTIN_DATE 3
#define BUILTIN_TIME 4
#define BUILTIN_STDC 5

#define INDENT_IF 0x0001
#define INDENT_ELSE 0x0002
#define INDENT_ELIF 0x0004
#define INDENT_IFDEF 0x0008
#define INDENT_IFNDEF 0x0010

/* macro definition; name is stored right behind the struct */
typedef struct Rgd3Define
{
	char* name;
	int flags;    /* DEFINE_FIXED */
	int builtin;  /* > 0 for __LINE__ etc. */
	int numparms;
	Rgd3Token* parms;  /* parameter names */
	Rgd3Token* tokens; /* body (may contain parameter names) */
	struct Rgd3Define* next;     /* global define list */
	struct Rgd3Define* hashnext; /* hash chain */
} Rgd3Define;

/* conditional compilation stack entry: #if, #else, #elif, #ifdef, #ifndef */
typedef struct Rgd3Indent
{
	int type;
	int skip;
	Rgd3Lexer* script; /* the script the indent was opened in */
	struct Rgd3Indent* next;
} Rgd3Indent;

struct Rgd3ParserToken;

typedef struct Rgd3Parser
{
	int loaded;
	char filename[RGD3_MAX_OSPATH];
	char includepath[RGD3_MAX_OSPATH];
	int os_path;
	const Rgd3Punctuation* punctuations; /* NULL: the lexer defaults */
	int flags;                           /* lexer flags for every script */
	Rgd3Lexer* scriptstack;              /* include stack, top first */
	Rgd3Token* tokens;                   /* tokens to read first */
	Rgd3Define* defines;
	Rgd3Define** definehash;
	Rgd3Indent* indentstack;
	int skip; /* > 0 while skipping conditional code */
	const char* marker_p;
	struct Rgd3ParserToken* live; /* every heap token the parser owns */
} Rgd3Parser;

void rgd3_parser_init(Rgd3Parser* p, int flags);
int rgd3_parser_load_file(Rgd3Parser* p, const char* filename, int os_path);
/* ptr[length] must be '\0'; the memory is not copied */
int rgd3_parser_load_memory(Rgd3Parser* p, const char* ptr, int length, const char* name);
void rgd3_parser_free_source(Rgd3Parser* p, int keep_defines);
void rgd3_parser_free(Rgd3Parser* p);
int rgd3_parser_is_loaded(const Rgd3Parser* p);

int rgd3_parser_read_token(Rgd3Parser* p, Rgd3Token* token);
int rgd3_parser_expect_token_string(Rgd3Parser* p, const char* string);
int rgd3_parser_expect_token_type(Rgd3Parser* p, int type, int subtype, Rgd3Token* token);
int rgd3_parser_expect_any_token(Rgd3Parser* p, Rgd3Token* token);
int rgd3_parser_check_token_string(Rgd3Parser* p, const char* string);
int rgd3_parser_check_token_type(Rgd3Parser* p, int type, int subtype, Rgd3Token* token);
int rgd3_parser_peek_token_string(Rgd3Parser* p, const char* string);
int rgd3_parser_peek_token_type(Rgd3Parser* p, int type, int subtype, Rgd3Token* token);
int rgd3_parser_skip_until_string(Rgd3Parser* p, const char* string);
int rgd3_parser_skip_rest_of_line(Rgd3Parser* p);
int rgd3_parser_skip_braced_section(Rgd3Parser* p, int parse_first_brace);
const char* rgd3_parser_parse_braced_section(Rgd3Parser* p, Rgd3StrBuf* out, int tabs, int parse_first_brace, char intro, char outro);
const char* rgd3_parser_parse_braced_section_exact(Rgd3Parser* p, Rgd3StrBuf* out, int tabs);
const char* rgd3_parser_parse_rest_of_line(Rgd3Parser* p, Rgd3StrBuf* out);
void rgd3_parser_unread_token(Rgd3Parser* p, const Rgd3Token* token);
int rgd3_parser_read_token_on_line(Rgd3Parser* p, Rgd3Token* token);
int rgd3_parser_parse_int(Rgd3Parser* p);
int rgd3_parser_parse_bool(Rgd3Parser* p);
float rgd3_parser_parse_float(Rgd3Parser* p);
int rgd3_parser_parse_1d_matrix(Rgd3Parser* p, int x, float* m);
int rgd3_parser_parse_2d_matrix(Rgd3Parser* p, int y, int x, float* m);
int rgd3_parser_parse_3d_matrix(Rgd3Parser* p, int z, int y, int x, float* m);

int rgd3_parser_get_last_white_space(const Rgd3Parser* p, Rgd3StrBuf* out);
void rgd3_parser_set_marker(Rgd3Parser* p);
void rgd3_parser_get_string_from_marker(Rgd3Parser* p, Rgd3StrBuf* out, int clean);
int rgd3_parser_add_define(Rgd3Parser* p, const char* string);
void rgd3_parser_add_builtin_defines(Rgd3Parser* p);
void rgd3_parser_set_include_path(Rgd3Parser* p, const char* path);
void rgd3_parser_set_punctuations(Rgd3Parser* p, const Rgd3Punctuation* punctuations);
const char* rgd3_parser_get_punctuation_from_id(const Rgd3Parser* p, int id);
int rgd3_parser_get_punctuation_id(const Rgd3Parser* p, const char* punctuation);
void rgd3_parser_set_flags(Rgd3Parser* p, int flags);
int rgd3_parser_get_flags(const Rgd3Parser* p);
const char* rgd3_parser_get_file_name(const Rgd3Parser* p);
int rgd3_parser_get_file_offset(const Rgd3Parser* p);
Rgd3TimeStamp rgd3_parser_get_file_time(const Rgd3Parser* p);
int rgd3_parser_get_line_num(const Rgd3Parser* p);
void rgd3_parser_error(const Rgd3Parser* p, const char* fmt, ...);
void rgd3_parser_warning(const Rgd3Parser* p, const char* fmt, ...);
int rgd3_parser_end_of_file(const Rgd3Parser* p);

int rgd3_parser_add_global_define(const char* string);
int rgd3_parser_remove_global_define(const char* name);
void rgd3_parser_remove_all_global_defines(void);
void rgd3_parser_set_base_folder(const char* path);
/* frees the global defines and the scratch parsers; idempotent */
void rgd3_parser_shutdown(void);

int rgd3_parser_self_test(void);
int rgd3_lexer_self_test(void);

#endif
