/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_decl.h - the declaration manager (idDeclManager / idDeclLocal,
   neo/framework/DeclManager.h/.cpp).

   "Small text" data (materials, sound shaders, entityDefs, skins, tables,
   fx, particles, AFs, PDAs...) is scanned from decl folders at startup and
   parsed lazily on first find. A decl is never freed before shutdown: the
   same type/name always returns the same pointer, and its index in the
   per-type list never changes. Data inside a decl may be reparsed (level
   purge, reload), so never keep pointers into it across frames.

   C layout: the manager keeps one Rgd3Decl record per decl (idDeclLocal).
   The type-specific body (idDecl subclass) is allocated on first use with
   the size from its Rgd3DeclTypeDesc, zeroed, and starts with
   `Rgd3Decl* base`. Finds return the body.

   Differences from the original: decl text is stored uncompressed (no
   Huffman), and the per-decl and global checksums are CRC32 instead of
   MD5 (only compared between rgd3 builds). Main thread only. */
#ifndef RGD3_DECL_H
#define RGD3_DECL_H
#ifndef RGD3_ALL_C
#error "rgd3 headers are included through src/all.c only"
#endif

typedef enum Rgd3DeclType
{
	RGD3_DECL_TABLE = 0,
	RGD3_DECL_MATERIAL,
	RGD3_DECL_SKIN,
	RGD3_DECL_SOUND,
	RGD3_DECL_ENTITYDEF,
	RGD3_DECL_MODELDEF,
	RGD3_DECL_FX,
	RGD3_DECL_PARTICLE,
	RGD3_DECL_AF,
	RGD3_DECL_PDA,
	RGD3_DECL_VIDEO,
	RGD3_DECL_AUDIO,
	RGD3_DECL_EMAIL,
	RGD3_DECL_MODELEXPORT,
	RGD3_DECL_MAPDEF,
	RGD3_DECL_MAX_TYPES = 32
} Rgd3DeclType;

typedef enum Rgd3DeclState
{
	RGD3_DS_UNPARSED,
	RGD3_DS_DEFAULTED, /* parse failed, or there was no source */
	RGD3_DS_PARSED
} Rgd3DeclState;

#define RGD3_DECL_LEXER_FLAGS (LEXFL_NOSTRINGCONCAT | LEXFL_NOSTRINGESCAPECHARS | LEXFL_ALLOWPATHNAMES | \
	LEXFL_ALLOWMULTICHARLITERALS | LEXFL_ALLOWBACKSLASHSTRINGCONCAT | LEXFL_NOFATALERRORS)

typedef struct Rgd3Decl Rgd3Decl;
typedef struct Rgd3DeclFile Rgd3DeclFile;

/* First member of every typed decl body. */
typedef struct Rgd3DeclBody
{
	Rgd3Decl* base;
} Rgd3DeclBody;

/* A decl type: its keyword, body size and behaviour. NULL functions use
   the idDeclLocal defaults (parse skips the braced section, default
   definition "{ }", list prints the name). parse receives the whole decl
   text ("type name { ... }"); it may call rgd3_decl_make_default() and
   return 0 on errors, and must only warn, never drop. */
typedef struct Rgd3DeclTypeDesc
{
	const char* type_name;
	Rgd3DeclType type;
	size_t body_size;
	int (*parse)(void* body, const char* text, int length, int allow_binary);
	void (*free_data)(void* body);
	const char* (*default_definition)(const void* body);
	int (*set_default_text)(void* body);
	void (*list)(const void* body);
	void (*print)(const void* body);
	size_t (*size)(const void* body);
} Rgd3DeclTypeDesc;

struct Rgd3Decl
{
	void* self; /* the typed body, NULL until first use */
	char* name;
	char* text_source;
	int text_length;
	Rgd3DeclFile* source_file;
	int source_text_offset;
	int source_text_length;
	int source_line;
	u32 checksum;
	Rgd3DeclType type;
	Rgd3DeclState state;
	int index;
	u8 parsed_outside_level_load; /* never purged */
	u8 ever_referenced;
	u8 referenced_this_level;
	u8 redefined_in_reload;
	Rgd3Decl* next_in_file;
};

struct Rgd3DeclFile
{
	char* file_name;
	Rgd3DeclType default_type;
	Rgd3TimeStamp timestamp;
	u32 checksum;
	int file_size;
	int num_lines;
	Rgd3Decl* decls;
};

void rgd3_decl_register(void);
void rgd3_decl_init(void);  /* types, materials folder */
void rgd3_decl_init2(void); /* skins, sound */
void rgd3_decl_init_game(void); /* the folders idGameLocal::Init registers */
void rgd3_decl_shutdown(void);
void rgd3_decl_reload(int force);
void rgd3_decl_begin_level_load(void);
void rgd3_decl_end_level_load(void);

void rgd3_decl_register_type(const Rgd3DeclTypeDesc* desc);
void rgd3_decl_register_folder(const char* folder, const char* extension, Rgd3DeclType default_type);
u32 rgd3_decl_checksum(void);
int rgd3_decl_num_types(void);
const char* rgd3_decl_type_name(Rgd3DeclType type);
Rgd3DeclType rgd3_decl_type_from_name(const char* type_name);

/* Find (and parse) a decl. With make_default, a missing decl is created
   as an implicit default; otherwise NULL. Empty names become "_emptyName". */
void* rgd3_decl_find(Rgd3DeclType type, const char* name, int make_default);
void* rgd3_decl_find_without_parsing(Rgd3DeclType type, const char* name, int make_default);
void* rgd3_decl_by_index(Rgd3DeclType type, int index, int force_parse);
int rgd3_decl_num_decls(Rgd3DeclType type);
void rgd3_decl_touch(const void* body);
void rgd3_decl_reload_file(const char* filename, int force);
void rgd3_decl_media_print(const char* fmt, ...);
int rgd3_decl_parse_all(const char* only, int verbose, int* out_defaulted, int* out_warnings);

/* Called by entityDef parsing (game->CacheDictionaryMedia); NULL until the
   game exists. */
typedef void (*Rgd3DeclDictHook)(const struct Rgd3Dict* dict);
void rgd3_decl_set_cache_dict_media_hook(Rgd3DeclDictHook hook);
void rgd3_decl_cache_dict_media(const struct Rgd3Dict* dict);

/* accessors on a typed body */
RGINLINE Rgd3Decl* rgd3_decl_base(const void* body)
{
	return ((const Rgd3DeclBody*)body)->base;
}

const char* rgd3_decl_name(const void* body);
Rgd3DeclType rgd3_decl_type(const void* body);
Rgd3DeclState rgd3_decl_state(const void* body);
int rgd3_decl_is_implicit(const void* body);
int rgd3_decl_is_valid(const void* body);
int rgd3_decl_index(const void* body);
int rgd3_decl_line_num(const void* body);
const char* rgd3_decl_file_name(const void* body);
const char* rgd3_decl_text(const void* body); /* NULL when there is no source */
int rgd3_decl_text_length(const void* body);
void rgd3_decl_set_text(void* body, const char* text);
void rgd3_decl_make_default(void* body);
void rgd3_decl_invalidate(void* body);
void rgd3_decl_ensure_not_purged(void* body);
int rgd3_decl_ever_referenced(const void* body);

#endif
