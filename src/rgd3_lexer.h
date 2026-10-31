/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_lexer.h - tokens and the lexicographical parser (idToken / idLexer,
   neo/idlib/Token.h/.cpp, neo/idlib/Lexer.h/.cpp).

   The flag, token type, number subtype and punctuation id values are the
   original ones: the script compiler, GUI parser and decl parsers compare
   against them.

   Tokens own their text. Short text lives in the token itself; longer text
   moves to the heap (RGD3_TAG_LEXER). Rules:
	- zero-initialise (`Rgd3Token t = {0};`) or call rgd3_token_init();
	- copy with rgd3_token_copy(), never with struct assignment;
	- call rgd3_token_free() when done (a no-op unless the text was long).
   A lexer is zero-initialised and set up with rgd3_lexer_init(); release it
   with rgd3_lexer_free(). Errors without LEXFL_NOERRORS/NOFATALERRORS drop
   to the innermost error guard, as the original's exception did. */
#ifndef RGD3_LEXER_H
#define RGD3_LEXER_H
#ifndef RGD3_ALL_C
#error "rgd3 headers are included through src/all.c only"
#endif

/* token types */
#define TT_STRING 1
#define TT_LITERAL 2
#define TT_NUMBER 3
#define TT_NAME 4
#define TT_PUNCTUATION 5

/* number sub types */
#define TT_INTEGER 0x00001
#define TT_DECIMAL 0x00002
#define TT_HEX 0x00004
#define TT_OCTAL 0x00008
#define TT_BINARY 0x00010
#define TT_LONG 0x00020
#define TT_UNSIGNED 0x00040
#define TT_FLOAT 0x00080
#define TT_SINGLE_PRECISION 0x00100
#define TT_DOUBLE_PRECISION 0x00200
#define TT_EXTENDED_PRECISION 0x00400
#define TT_INFINITE 0x00800
#define TT_INDEFINITE 0x01000
#define TT_NAN 0x02000
#define TT_IPADDRESS 0x04000
#define TT_IPPORT 0x08000
#define TT_VALUESVALID 0x10000

/* lexer flags */
enum
{
	LEXFL_NOERRORS = 1 << 0,
	LEXFL_NOWARNINGS = 1 << 1,
	LEXFL_NOFATALERRORS = 1 << 2,
	LEXFL_NOSTRINGCONCAT = 1 << 3,
	LEXFL_NOSTRINGESCAPECHARS = 1 << 4,
	LEXFL_NODOLLARPRECOMPILE = 1 << 5,
	LEXFL_NOBASEINCLUDES = 1 << 6,
	LEXFL_ALLOWPATHNAMES = 1 << 7,
	LEXFL_ALLOWNUMBERNAMES = 1 << 8,
	LEXFL_ALLOWIPADDRESSES = 1 << 9,
	LEXFL_ALLOWFLOATEXCEPTIONS = 1 << 10,
	LEXFL_ALLOWMULTICHARLITERALS = 1 << 11,
	LEXFL_ALLOWBACKSLASHSTRINGCONCAT = 1 << 12,
	LEXFL_ONLYSTRINGS = 1 << 13
};

/* punctuation ids */
#define P_RSHIFT_ASSIGN 1
#define P_LSHIFT_ASSIGN 2
#define P_PARMS 3
#define P_PRECOMPMERGE 4
#define P_LOGIC_AND 5
#define P_LOGIC_OR 6
#define P_LOGIC_GEQ 7
#define P_LOGIC_LEQ 8
#define P_LOGIC_EQ 9
#define P_LOGIC_UNEQ 10
#define P_MUL_ASSIGN 11
#define P_DIV_ASSIGN 12
#define P_MOD_ASSIGN 13
#define P_ADD_ASSIGN 14
#define P_SUB_ASSIGN 15
#define P_INC 16
#define P_DEC 17
#define P_BIN_AND_ASSIGN 18
#define P_BIN_OR_ASSIGN 19
#define P_BIN_XOR_ASSIGN 20
#define P_RSHIFT 21
#define P_LSHIFT 22
#define P_POINTERREF 23
#define P_CPP1 24
#define P_CPP2 25
#define P_MUL 26
#define P_DIV 27
#define P_MOD 28
#define P_ADD 29
#define P_SUB 30
#define P_ASSIGN 31
#define P_BIN_AND 32
#define P_BIN_OR 33
#define P_BIN_XOR 34
#define P_BIN_NOT 35
#define P_LOGIC_NOT 36
#define P_LOGIC_GREATER 37
#define P_LOGIC_LESS 38
#define P_REF 39
#define P_COMMA 40
#define P_SEMICOLON 41
#define P_COLON 42
#define P_QUESTIONMARK 43
#define P_PARENTHESESOPEN 44
#define P_PARENTHESESCLOSE 45
#define P_BRACEOPEN 46
#define P_BRACECLOSE 47
#define P_SQBRACKETOPEN 48
#define P_SQBRACKETCLOSE 49
#define P_BACKSLASH 50
#define P_PRECOMP 51
#define P_DOLLAR 52

#define RGD3_TOKEN_BASE_SIZE 240

/* string sub type is the length of the string, literal sub type is the
   character, punctuation sub type is the punctuation id, name sub type is
   the length of the name */
typedef struct Rgd3Token
{
	char* data; /* NUL-terminated; points at base or a heap block */
	int len;
	int alloced;
	int type;
	int subtype;
	int line;
	int lines_crossed; /* lines crossed in white space before the token */
	int flags;         /* used by the parser for recursive defines */
	u32 intvalue;      /* `unsigned long` on the original Win32 build */
	double floatvalue;
	const char* white_space_start_p;
	const char* white_space_end_p;
	struct Rgd3Token* next; /* parser chains */
	char base[RGD3_TOKEN_BASE_SIZE];
} Rgd3Token;

void rgd3_token_init(Rgd3Token* t);
void rgd3_token_free(Rgd3Token* t);
void rgd3_token_ensure(Rgd3Token* t, int size, int keep);
void rgd3_token_copy(Rgd3Token* dst, const Rgd3Token* src);
void rgd3_token_set(Rgd3Token* t, const char* text);
void rgd3_token_set_n(Rgd3Token* t, const char* text, int n);
void rgd3_token_append(Rgd3Token* t, const char* text);
void rgd3_token_append_char(Rgd3Token* t, char c);
void rgd3_token_clear(Rgd3Token* t);
const char* rgd3_token_cstr(const Rgd3Token* t);
int rgd3_token_is(const Rgd3Token* t, const char* text);  /* case-sensitive, idStr operator== */
int rgd3_token_icmp(const Rgd3Token* t, const char* text); /* idStr::Icmp */
void rgd3_token_number_value(Rgd3Token* t);
double rgd3_token_double_value(Rgd3Token* t);
float rgd3_token_float_value(Rgd3Token* t);
u32 rgd3_token_unsigned_value(Rgd3Token* t);
int rgd3_token_int_value(Rgd3Token* t);
int rgd3_token_white_space_before(const Rgd3Token* t);
void rgd3_token_clear_white_space(Rgd3Token* t);

typedef struct Rgd3Punctuation
{
	const char* p; /* punctuation character(s) */
	int n;         /* punctuation id */
} Rgd3Punctuation;

typedef struct Rgd3Lexer
{
	int loaded;
	char filename[RGD3_MAX_OSPATH];
	int allocated;
	const char* buffer;
	const char* script_p;
	const char* end_p;
	const char* last_script_p;
	const char* white_space_start_p;
	const char* white_space_end_p;
	Rgd3TimeStamp file_time;
	int length;
	int line;
	int last_line;
	int token_available;
	int flags;
	const Rgd3Punctuation* punctuations;
	int* punctuation_table;
	int* next_punctuation;
	Rgd3Token token; /* unread token */
	struct Rgd3Lexer* next;
	int had_error;
	int initialized;
} Rgd3Lexer;

extern const Rgd3Punctuation rgd3_default_punctuations[];

void rgd3_lexer_init(Rgd3Lexer* lex, int flags);
void rgd3_lexer_free(Rgd3Lexer* lex);
int rgd3_lexer_load_file(Rgd3Lexer* lex, const char* filename, int os_path);
int rgd3_lexer_load_memory(Rgd3Lexer* lex, const char* ptr, int length, const char* name, int start_line);
void rgd3_lexer_free_source(Rgd3Lexer* lex);
int rgd3_lexer_is_loaded(const Rgd3Lexer* lex);

int rgd3_lexer_read_token(Rgd3Lexer* lex, Rgd3Token* token);
int rgd3_lexer_expect_token_string(Rgd3Lexer* lex, const char* string);
int rgd3_lexer_expect_token_type(Rgd3Lexer* lex, int type, int subtype, Rgd3Token* token);
int rgd3_lexer_expect_any_token(Rgd3Lexer* lex, Rgd3Token* token);
int rgd3_lexer_check_token_string(Rgd3Lexer* lex, const char* string);
int rgd3_lexer_check_token_type(Rgd3Lexer* lex, int type, int subtype, Rgd3Token* token);
int rgd3_lexer_peek_token_string(Rgd3Lexer* lex, const char* string);
int rgd3_lexer_peek_token_type(Rgd3Lexer* lex, int type, int subtype, Rgd3Token* token);
int rgd3_lexer_skip_until_string(Rgd3Lexer* lex, const char* string);
int rgd3_lexer_skip_rest_of_line(Rgd3Lexer* lex);
int rgd3_lexer_skip_braced_section(Rgd3Lexer* lex, int parse_first_brace);
int rgd3_lexer_skip_white_space(Rgd3Lexer* lex, int current_line);
void rgd3_lexer_unread_token(Rgd3Lexer* lex, const Rgd3Token* token);
int rgd3_lexer_read_token_on_line(Rgd3Lexer* lex, Rgd3Token* token);
const char* rgd3_lexer_read_rest_of_line(Rgd3Lexer* lex, Rgd3StrBuf* out);

int rgd3_lexer_parse_int(Rgd3Lexer* lex);
int rgd3_lexer_parse_bool(Rgd3Lexer* lex);
float rgd3_lexer_parse_float(Rgd3Lexer* lex, int* error_flag);
int rgd3_lexer_parse_1d_matrix(Rgd3Lexer* lex, int x, float* m);
int rgd3_lexer_parse_2d_matrix(Rgd3Lexer* lex, int y, int x, float* m);
int rgd3_lexer_parse_3d_matrix(Rgd3Lexer* lex, int z, int y, int x, float* m);
const char* rgd3_lexer_parse_braced_section(Rgd3Lexer* lex, Rgd3StrBuf* out);
const char* rgd3_lexer_parse_braced_section_exact(Rgd3Lexer* lex, Rgd3StrBuf* out, int tabs);
const char* rgd3_lexer_parse_rest_of_line(Rgd3Lexer* lex, Rgd3StrBuf* out);
const char* rgd3_lexer_parse_complete_line(Rgd3Lexer* lex, Rgd3StrBuf* out);

int rgd3_lexer_get_last_white_space(const Rgd3Lexer* lex, Rgd3StrBuf* out);
int rgd3_lexer_get_last_white_space_start(const Rgd3Lexer* lex);
int rgd3_lexer_get_last_white_space_end(const Rgd3Lexer* lex);
void rgd3_lexer_set_punctuations(Rgd3Lexer* lex, const Rgd3Punctuation* p);
const char* rgd3_lexer_get_punctuation_from_id(const Rgd3Lexer* lex, int id);
int rgd3_lexer_get_punctuation_id(const Rgd3Lexer* lex, const char* p);
void rgd3_lexer_set_flags(Rgd3Lexer* lex, int flags);
int rgd3_lexer_get_flags(const Rgd3Lexer* lex);
void rgd3_lexer_reset(Rgd3Lexer* lex);
int rgd3_lexer_end_of_file(const Rgd3Lexer* lex);
const char* rgd3_lexer_file_name(const Rgd3Lexer* lex);
int rgd3_lexer_file_offset(const Rgd3Lexer* lex);
Rgd3TimeStamp rgd3_lexer_file_time(const Rgd3Lexer* lex);
int rgd3_lexer_line_num(const Rgd3Lexer* lex);
void rgd3_lexer_error(Rgd3Lexer* lex, const char* fmt, ...);
void rgd3_lexer_warning(Rgd3Lexer* lex, const char* fmt, ...);
int rgd3_lexer_had_error(const Rgd3Lexer* lex);
void rgd3_lexer_set_base_folder(const char* path);

#endif
