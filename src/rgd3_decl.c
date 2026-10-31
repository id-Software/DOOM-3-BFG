/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_decl.c - see rgd3_decl.h. Lifted from neo/framework/DeclManager.cpp:
   idDeclFile::LoadAndParse (decl scanning, redefinition warnings, reload
   defaulting), FindTypeWithoutParsing/FindType/DeclByIndex,
   MakeNameCanonical, ParseLocal/MakeDefault/Purge, ListType/PrintType,
   ListDecls_f/ReloadDecls_f/TouchDecl_f, and the folder registration order
   of Init/Init2 and idGameLocal::Init (neo/d3xp/Game_local.cpp). */

static Rgd3Cvar cv_decl_show = RGD3_CVAR_RANGE("decl_show", "0", RGD3_CVAR_SYSTEM | RGD3_CVAR_INTEGER, "set to 1 to print parses, 2 to also print references", 0, 2);

typedef struct Rgd3DeclFolder
{
	char* folder;
	char* extension;
	Rgd3DeclType default_type;
} Rgd3DeclFolder;

typedef struct Rgd3DeclList
{
	Rgd3Decl** items;
	int num;
	int alloced;
} Rgd3DeclList;

static struct
{
	const Rgd3DeclTypeDesc* types[RGD3_DECL_MAX_TYPES];
	int num_types; /* highest registered type + 1 */
	Rgd3DeclFolder* folders;
	int num_folders;
	Rgd3DeclFile** files;
	int num_files;
	Rgd3HashIndex hash_tables[RGD3_DECL_MAX_TYPES];
	Rgd3DeclList lists[RGD3_DECL_MAX_TYPES];
	Rgd3DeclFile implicit_decls; /* decls created because no text existed */
	int indent;
	int inside_level_load;
	int initialized;
	Rgd3DeclDictHook cache_dict_media;
} rgd3_decl;

static void rgd3_decl_parse_local(Rgd3Decl* decl);

/* ---- helpers ---- */

static const Rgd3DeclTypeDesc* rgd3_decl_desc(Rgd3DeclType type)
{
	int t = (int)type;
	if (t < 0 || t >= rgd3_decl.num_types || !rgd3_decl.types[t])
		rgd3_error_fatal("rgd3_decl: bad type: %i", t);
	return rgd3_decl.types[t];
}

/* idDeclManagerLocal::MakeNameCanonical: lowercase, '\' to '/', the last
   extension removed */
static void rgd3_decl_make_name_canonical(const char* name, char* result, int max_length)
{
	int i, last_dot = -1;
	for (i = 0; i < max_length - 1 && name[i] != '\0'; i++)
	{
		char c = name[i];
		if (c == '\\')
		{
			result[i] = '/';
		}
		else if (c == '.')
		{
			last_dot = i;
			result[i] = c;
		}
		else
		{
			result[i] = (c >= 'A' && c <= 'Z') ? (char)(c + ('a' - 'A')) : c;
		}
	}
	if (last_dot != -1)
		result[last_dot] = '\0';
	else
		result[i] = '\0';
}

static void rgd3_decl_list_append(Rgd3DeclList* list, Rgd3Decl* decl)
{
	if (list->num == list->alloced)
	{
		list->alloced = list->alloced ? list->alloced * 2 : 256;
		list->items = (Rgd3Decl**)rgd3_heap_realloc(list->items, (size_t)list->alloced * sizeof(Rgd3Decl*), RGD3_TAG_DECL);
	}
	list->items[list->num++] = decl;
}

static void rgd3_decl_allocate_self(Rgd3Decl* decl)
{
	if (decl->self == NULL)
	{
		const Rgd3DeclTypeDesc* desc = rgd3_decl_desc(decl->type);
		Rgd3DeclBody* body = (Rgd3DeclBody*)rgd3_heap_calloc(desc->body_size, RGD3_TAG_DECL);
		body->base = decl;
		decl->self = body;
	}
}

static void rgd3_decl_set_text_local(Rgd3Decl* decl, const char* text, int length)
{
	if (decl->text_source)
		rgd3_heap_free(decl->text_source);
	decl->checksum = rgd3_crc32_block(text, length);
	decl->text_source = (char*)rgd3_heap_alloc((size_t)length + 1, RGD3_TAG_DECLTEXT);
	memcpy(decl->text_source, text, (size_t)length);
	decl->text_source[length] = '\0';
	decl->text_length = length;
}

static const char* rgd3_decl_default_definition(Rgd3Decl* decl)
{
	const Rgd3DeclTypeDesc* desc = rgd3_decl_desc(decl->type);
	return desc->default_definition ? desc->default_definition(decl->self) : "{ }";
}

/* idDeclLocal::Parse: skip to the braces and over them */
static int rgd3_decl_default_parse(void* body, const char* text, int length, int allow_binary)
{
	Rgd3Lexer src;
	(void)allow_binary;
	rgd3_lexer_init(&src, RGD3_DECL_LEXER_FLAGS);
	rgd3_lexer_load_memory(&src, text, length, rgd3_decl_file_name(body), rgd3_decl_line_num(body));
	rgd3_lexer_skip_until_string(&src, "{");
	rgd3_lexer_skip_braced_section(&src, 0);
	rgd3_lexer_free(&src);
	return 1;
}

static int rgd3_decl_call_parse(Rgd3Decl* decl, const char* text, int length, int allow_binary)
{
	const Rgd3DeclTypeDesc* desc = rgd3_decl_desc(decl->type);
	if (desc->parse)
		return desc->parse(decl->self, text, length, allow_binary);
	return rgd3_decl_default_parse(decl->self, text, length, allow_binary);
}

static void rgd3_decl_call_free_data(Rgd3Decl* decl)
{
	const Rgd3DeclTypeDesc* desc = rgd3_decl_desc(decl->type);
	if (desc->free_data)
		desc->free_data(decl->self);
}

/* ---- state changes ---- */

static void rgd3_decl_make_default_local(Rgd3Decl* decl)
{
	static int recursion_level;
	const char* default_text;

	rgd3_decl_media_print("DEFAULTED\n");
	decl->state = RGD3_DS_DEFAULTED;
	rgd3_decl_allocate_self(decl);
	default_text = rgd3_decl_default_definition(decl);

	/* a parse error inside a default definition could recurse forever, but
	   default definitions may legitimately reference other defaults */
	if (++recursion_level > 100)
		rgd3_error_fatal("idDecl::MakeDefault: bad DefaultDefinition(): %s", default_text);
	rgd3_decl_call_free_data(decl);
	rgd3_decl_call_parse(decl, default_text, (int)strlen(default_text), 0);
	--recursion_level;
}

static void rgd3_decl_parse_local(Rgd3Decl* decl)
{
	const Rgd3DeclTypeDesc* desc;
	int generated_default_text = 0;
	char* text;

	rgd3_decl_allocate_self(decl);
	desc = rgd3_decl_desc(decl->type);
	rgd3_decl_call_free_data(decl);
	rgd3_decl_media_print("parsing %s %s\n", desc->type_name, decl->name);

	if (decl->text_source == NULL && desc->set_default_text)
		generated_default_text = desc->set_default_text(decl->self);

	rgd3_decl.indent++;
	if (decl->text_source == NULL)
	{
		rgd3_decl_make_default_local(decl);
		rgd3_decl.indent--;
		return;
	}
	decl->state = RGD3_DS_PARSED;

	/* parse a copy: parsing can find other decls, which may reload files */
	text = (char*)rgd3_heap_alloc((size_t)decl->text_length + 1, RGD3_TAG_TEMP);
	memcpy(text, decl->text_source, (size_t)decl->text_length + 1);
	rgd3_decl_call_parse(decl, text, decl->text_length, 1);
	rgd3_heap_free(text);

	if (generated_default_text)
	{
		rgd3_heap_free(decl->text_source);
		decl->text_source = NULL;
		decl->text_length = 0;
	}
	rgd3_decl.indent--;
}

/* default the data; the next find re-parses it */
static void rgd3_decl_purge(Rgd3Decl* decl)
{
	/* never purge things referenced outside level load (console, menus) */
	if (decl->parsed_outside_level_load)
		return;
	decl->referenced_this_level = 0;
	rgd3_decl_make_default_local(decl);
	decl->state = RGD3_DS_UNPARSED;
}

/* ---- lookup ---- */

static Rgd3Decl* rgd3_decl_find_type_without_parsing(Rgd3DeclType type, const char* name, int make_default)
{
	char canonical[RGD3_MAX_STRING_CHARS];
	int t = (int)type, i, hash;
	Rgd3Decl* decl;

	if (t < 0 || t >= rgd3_decl.num_types || !rgd3_decl.types[t] || t >= RGD3_DECL_MAX_TYPES)
	{
		rgd3_error_fatal("idDeclManager::FindTypeWithoutParsing: bad type: %i", t);
		return NULL;
	}
	rgd3_decl_make_name_canonical(name, canonical, sizeof(canonical));

	hash = rgd3_hashindex_key_string(&rgd3_decl.hash_tables[t], canonical, 0);
	for (i = rgd3_hashindex_first(&rgd3_decl.hash_tables[t], hash); i >= 0; i = rgd3_hashindex_next(&rgd3_decl.hash_tables[t], i))
	{
		if (rgd3_str_icmp(rgd3_decl.lists[t].items[i]->name, canonical) == 0)
		{
			if (cv_decl_show.integer_value > 1)
				rgd3_decl_media_print("referencing %s %s\n", rgd3_decl.types[t]->type_name, name);
			return rgd3_decl.lists[t].items[i];
		}
	}
	if (!make_default)
		return NULL;

	decl = (Rgd3Decl*)rgd3_heap_calloc(sizeof(Rgd3Decl), RGD3_TAG_DECL);
	decl->name = rgd3_heap_strdup(canonical, RGD3_TAG_DECL);
	decl->type = type;
	decl->state = RGD3_DS_UNPARSED;
	decl->source_file = &rgd3_decl.implicit_decls;
	decl->parsed_outside_level_load = (u8)!rgd3_decl.inside_level_load;
	decl->index = rgd3_decl.lists[t].num;
	rgd3_decl_list_append(&rgd3_decl.lists[t], decl);
	rgd3_hashindex_add(&rgd3_decl.hash_tables[t], hash, decl->index);
	return decl;
}

void* rgd3_decl_find(Rgd3DeclType type, const char* name, int make_default)
{
	Rgd3Decl* decl;

	if (!name || !name[0])
		name = "_emptyName";
	decl = rgd3_decl_find_type_without_parsing(type, name, make_default);
	if (!decl)
		return NULL;
	rgd3_decl_allocate_self(decl);
	if (decl->state == RGD3_DS_UNPARSED)
	{
		if (!rgd3_thread_is_main())
			rgd3_error_fatal("Attempted to load %s decl '%s' from a worker thread!", rgd3_decl_type_name(type), name);
		rgd3_decl_parse_local(decl);
	}
	decl->referenced_this_level = 1;
	decl->ever_referenced = 1;
	if (rgd3_decl.inside_level_load)
		decl->parsed_outside_level_load = 0;
	return decl->self;
}

void* rgd3_decl_find_without_parsing(Rgd3DeclType type, const char* name, int make_default)
{
	Rgd3Decl* decl = rgd3_decl_find_type_without_parsing(type, name, make_default);
	if (!decl)
		return NULL;
	rgd3_decl_allocate_self(decl);
	return decl->self;
}

void* rgd3_decl_by_index(Rgd3DeclType type, int index, int force_parse)
{
	int t = (int)type;
	Rgd3Decl* decl;

	rgd3_decl_desc(type);
	if (index < 0 || index >= rgd3_decl.lists[t].num)
		rgd3_error_drop("idDeclManager::DeclByIndex: out of range");
	decl = rgd3_decl.lists[t].items[index];
	rgd3_decl_allocate_self(decl);
	if (force_parse && decl->state == RGD3_DS_UNPARSED)
		rgd3_decl_parse_local(decl);
	return decl->self;
}

int rgd3_decl_num_decls(Rgd3DeclType type)
{
	rgd3_decl_desc(type);
	return rgd3_decl.lists[(int)type].num;
}

void rgd3_decl_touch(const void* body)
{
	Rgd3Decl* decl = rgd3_decl_base(body);
	if (decl->state == RGD3_DS_UNPARSED)
		rgd3_decl_find(decl->type, decl->name, 1);
}

/* ---- decl files ---- */

static Rgd3DeclFile* rgd3_decl_new_file(const char* file_name, Rgd3DeclType default_type)
{
	Rgd3DeclFile* df = (Rgd3DeclFile*)rgd3_heap_calloc(sizeof(Rgd3DeclFile), RGD3_TAG_DECL);
	df->file_name = rgd3_heap_strdup(file_name, RGD3_TAG_DECL);
	df->default_type = default_type;
	if (rgd3_decl.num_files % 64 == 0)
		rgd3_decl.files = (Rgd3DeclFile**)rgd3_heap_realloc(rgd3_decl.files, (size_t)(rgd3_decl.num_files + 64) * sizeof(Rgd3DeclFile*), RGD3_TAG_DECL);
	rgd3_decl.files[rgd3_decl.num_files++] = df;
	return df;
}

/* idDeclFile::LoadAndParse: split the file into decls, used for the
   initial load and for reloads */
static u32 rgd3_decl_file_load_and_parse(Rgd3DeclFile* df)
{
	Rgd3Lexer src;
	Rgd3Token token = { 0 };
	Rgd3Decl* decl;
	char name[RGD3_MAX_STRING_CHARS];
	void* data;
	char* buffer;
	int length, i;

	rgd3_dprintf("...loading '%s'\n", df->file_name);
	length = rgd3_fs_read_file(df->file_name, &data, &df->timestamp);
	if (length == -1)
	{
		rgd3_error_fatal("couldn't load %s", df->file_name);
		return 0;
	}
	buffer = (char*)data;

	rgd3_lexer_init(&src, RGD3_DECL_LEXER_FLAGS);
	if (!rgd3_lexer_load_memory(&src, buffer, length, df->file_name, 1))
	{
		rgd3_fs_free_file(buffer);
		rgd3_lexer_free(&src);
		rgd3_error_drop("Couldn't parse %s", df->file_name);
	}

	/* mark the decls from the last load of this file */
	for (decl = df->decls; decl; decl = decl->next_in_file)
		decl->redefined_in_reload = 0;

	df->checksum = rgd3_crc32_block(buffer, length);
	df->file_size = length;

	for (;;)
	{
		Rgd3DeclType identified = RGD3_DECL_MAX_TYPES;
		int start_marker = rgd3_lexer_file_offset(&src);
		int source_line = rgd3_lexer_line_num(&src);
		int size, reparse;

		/* the decl type name */
		if (!rgd3_lexer_read_token(&src, &token))
			break;
		for (i = 0; i < rgd3_decl.num_types; i++)
		{
			if (rgd3_decl.types[i] && rgd3_token_icmp(&token, rgd3_decl.types[i]->type_name) == 0)
			{
				identified = rgd3_decl.types[i]->type;
				break;
			}
		}
		if (i >= rgd3_decl.num_types)
		{
			if (rgd3_token_icmp(&token, "{") == 0)
			{
				/* an open brace means the [type] <name> prefix was missed */
				rgd3_lexer_warning(&src, "Missing decl name");
				rgd3_lexer_skip_braced_section(&src, 0);
				continue;
			}
			if (df->default_type == RGD3_DECL_MAX_TYPES)
			{
				rgd3_lexer_warning(&src, "No type");
				continue;
			}
			rgd3_lexer_unread_token(&src, &token);
			identified = df->default_type;
		}

		/* the name */
		if (!rgd3_lexer_read_token(&src, &token))
		{
			rgd3_lexer_warning(&src, "Type without definition at end of file");
			break;
		}
		if (!rgd3_token_icmp(&token, "{"))
		{
			rgd3_lexer_warning(&src, "Missing decl name");
			rgd3_lexer_skip_braced_section(&src, 0);
			continue;
		}
		/* export decls are only used by the model exporter */
		if (identified == RGD3_DECL_MODELEXPORT)
		{
			rgd3_lexer_skip_braced_section(&src, 1);
			continue;
		}
		rgd3_str_copynz(name, token.data, sizeof(name));

		/* make sure there's a '{' */
		if (!rgd3_lexer_read_token(&src, &token))
		{
			rgd3_lexer_warning(&src, "Type without definition at end of file");
			break;
		}
		if (!rgd3_token_is(&token, "{"))
		{
			rgd3_lexer_warning(&src, "Expecting '{' but found '%s'", token.data);
			continue;
		}
		rgd3_lexer_unread_token(&src, &token);

		/* everything to the matching closing brace */
		rgd3_lexer_skip_braced_section(&src, 1);
		size = rgd3_lexer_file_offset(&src) - start_marker;

		/* look it up, possibly getting a newly created default decl */
		reparse = 0;
		decl = rgd3_decl_find_type_without_parsing(identified, name, 0);
		if (decl)
		{
			if (decl->source_file != df || decl->redefined_in_reload)
			{
				rgd3_lexer_warning(&src, "%s '%s' previously defined at %s:%i", rgd3_decl_type_name(identified),
					name, decl->source_file->file_name, decl->source_line);
				continue;
			}
			if (decl->state != RGD3_DS_UNPARSED)
				reparse = 1;
		}
		else
		{
			decl = rgd3_decl_find_type_without_parsing(identified, name, 1);
			decl->next_in_file = df->decls;
			df->decls = decl;
		}
		decl->redefined_in_reload = 1;
		rgd3_decl_set_text_local(decl, buffer + start_marker, size);
		decl->source_file = df;
		decl->source_text_offset = start_marker;
		decl->source_text_length = size;
		decl->source_line = source_line;
		decl->state = RGD3_DS_UNPARSED;

		/* if it is in use, reparse it now */
		if (reparse)
			rgd3_decl_parse_local(decl);
	}

	df->num_lines = rgd3_lexer_line_num(&src);
	rgd3_token_free(&token);
	rgd3_lexer_free(&src);
	rgd3_fs_free_file(buffer);

	/* decls that were not redefined lost their source: default them */
	for (decl = df->decls; decl; decl = decl->next_in_file)
	{
		if (!decl->redefined_in_reload)
		{
			rgd3_decl_make_default_local(decl);
			decl->source_text_offset = decl->source_file->file_size;
			decl->source_text_length = 0;
			decl->source_line = decl->source_file->num_lines;
		}
	}
	return df->checksum;
}

static void rgd3_decl_file_reload(Rgd3DeclFile* df, int force)
{
	if (!force && df->timestamp != 0)
	{
		Rgd3TimeStamp ts;
		rgd3_fs_read_file(df->file_name, NULL, &ts);
		if (ts == df->timestamp)
			return;
	}
	rgd3_decl_file_load_and_parse(df);
}

/* ---- manager ---- */

void rgd3_decl_register_type(const Rgd3DeclTypeDesc* desc)
{
	int t = (int)desc->type;
	RG_ASSERT(t >= 0 && t < RGD3_DECL_MAX_TYPES);
	RG_ASSERT(desc->body_size >= sizeof(Rgd3DeclBody));
	if (t < rgd3_decl.num_types && rgd3_decl.types[t])
	{
		rgd3_warning("idDeclManager::RegisterDeclType: type '%s' already exists", desc->type_name);
		return;
	}
	rgd3_decl.types[t] = desc;
	if (t + 1 > rgd3_decl.num_types)
		rgd3_decl.num_types = t + 1;
}

void rgd3_decl_register_folder(const char* folder, const char* extension, Rgd3DeclType default_type)
{
	Rgd3DeclFolder* df_folder = NULL;
	Rgd3FileList* list;
	int i, j;

	for (i = 0; i < rgd3_decl.num_folders; i++)
	{
		if (!rgd3_str_icmp(rgd3_decl.folders[i].folder, folder) && !rgd3_str_icmp(rgd3_decl.folders[i].extension, extension))
		{
			df_folder = &rgd3_decl.folders[i];
			break;
		}
	}
	if (!df_folder)
	{
		rgd3_decl.folders = (Rgd3DeclFolder*)rgd3_heap_realloc(rgd3_decl.folders, (size_t)(rgd3_decl.num_folders + 1) * sizeof(Rgd3DeclFolder), RGD3_TAG_DECL);
		df_folder = &rgd3_decl.folders[rgd3_decl.num_folders++];
		df_folder->folder = rgd3_heap_strdup(folder, RGD3_TAG_DECL);
		df_folder->extension = rgd3_heap_strdup(extension, RGD3_TAG_DECL);
		df_folder->default_type = default_type;
	}

	list = rgd3_fs_list_files(df_folder->folder, df_folder->extension, 0, 0);
	for (i = 0; list && i < list->count; i++)
	{
		char file_name[RGD3_MAX_OSPATH];
		Rgd3DeclFile* df = NULL;
		rg_snprintf(file_name, sizeof(file_name), "%s/%s", df_folder->folder, list->names[i]);
		for (j = 0; j < rgd3_decl.num_files; j++)
		{
			if (!rgd3_str_icmp(file_name, rgd3_decl.files[j]->file_name))
			{
				df = rgd3_decl.files[j];
				break;
			}
		}
		if (!df)
			df = rgd3_decl_new_file(file_name, default_type);
		rgd3_decl_file_load_and_parse(df);
	}
	if (list)
		rgd3_fs_free_file_list(list);
}

/* CRC32 over (index, checksum) pairs of every explicit decl, except the
   localized PDA types (the original uses MD5) */
u32 rgd3_decl_checksum(void)
{
	u32 crc;
	int i, j, total = 0;

	rgd3_crc32_init(&crc);
	for (i = 0; i < RGD3_DECL_MAX_TYPES; i++)
	{
		if (i == RGD3_DECL_PDA || i == RGD3_DECL_VIDEO || i == RGD3_DECL_AUDIO || i == RGD3_DECL_EMAIL)
			continue;
		for (j = 0; j < rgd3_decl.lists[i].num; j++)
		{
			const Rgd3Decl* decl = rgd3_decl.lists[i].items[j];
			u8 pair[8];
			if (decl->source_file == &rgd3_decl.implicit_decls)
				continue;
			pair[0] = (u8)(total >> 24);
			pair[1] = (u8)(total >> 16);
			pair[2] = (u8)(total >> 8);
			pair[3] = (u8)total;
			pair[4] = (u8)(decl->checksum >> 24);
			pair[5] = (u8)(decl->checksum >> 16);
			pair[6] = (u8)(decl->checksum >> 8);
			pair[7] = (u8)decl->checksum;
			rgd3_crc32_update(&crc, pair, 8);
			total++;
		}
	}
	rgd3_crc32_finish(&crc);
	return crc;
}

int rgd3_decl_num_types(void)
{
	return rgd3_decl.num_types;
}

const char* rgd3_decl_type_name(Rgd3DeclType type)
{
	int t = (int)type;
	if (t < 0 || t >= rgd3_decl.num_types || !rgd3_decl.types[t])
		rgd3_error_fatal("idDeclManager::GetDeclNameFromType: bad type: %i", t);
	return rgd3_decl.types[t]->type_name;
}

Rgd3DeclType rgd3_decl_type_from_name(const char* type_name)
{
	int i;
	for (i = 0; i < rgd3_decl.num_types; i++)
	{
		if (rgd3_decl.types[i] && !rgd3_str_icmp(rgd3_decl.types[i]->type_name, type_name))
			return rgd3_decl.types[i]->type;
	}
	return RGD3_DECL_MAX_TYPES;
}

void rgd3_decl_reload(int force)
{
	int i;
	for (i = 0; i < rgd3_decl.num_files; i++)
		rgd3_decl_file_reload(rgd3_decl.files[i], force);
}

void rgd3_decl_reload_file(const char* filename, int force)
{
	int i;
	for (i = 0; i < rgd3_decl.num_files; i++)
	{
		if (!rgd3_str_icmp(rgd3_decl.files[i]->file_name, filename))
			rgd3_decl_file_reload(rgd3_decl.files[i], force);
	}
}

void rgd3_decl_begin_level_load(void)
{
	int i, j;
	rgd3_decl.inside_level_load = 1;
	/* purge everything so the next reference reparses it */
	for (i = 0; i < RGD3_DECL_MAX_TYPES; i++)
	{
		for (j = 0; j < rgd3_decl.lists[i].num; j++)
			rgd3_decl_purge(rgd3_decl.lists[i].items[j]);
	}
}

void rgd3_decl_end_level_load(void)
{
	rgd3_decl.inside_level_load = 0;
}

void rgd3_decl_media_print(const char* fmt, ...)
{
	char buffer[1024];
	va_list args;
	int i;
	if (!cv_decl_show.integer_value)
		return;
	for (i = 0; i < rgd3_decl.indent; i++)
		rgd3_printf("    ");
	va_start(args, fmt);
	rg_vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);
	rgd3_printf("%s", buffer);
}

void rgd3_decl_set_cache_dict_media_hook(Rgd3DeclDictHook hook)
{
	rgd3_decl.cache_dict_media = hook;
}

void rgd3_decl_cache_dict_media(const Rgd3Dict* dict)
{
	if (rgd3_decl.cache_dict_media)
		rgd3_decl.cache_dict_media(dict);
}

/* ---- body accessors ---- */

const char* rgd3_decl_name(const void* body)
{
	return rgd3_decl_base(body)->name;
}

Rgd3DeclType rgd3_decl_type(const void* body)
{
	return rgd3_decl_base(body)->type;
}

Rgd3DeclState rgd3_decl_state(const void* body)
{
	return rgd3_decl_base(body)->state;
}

int rgd3_decl_is_implicit(const void* body)
{
	return rgd3_decl_base(body)->source_file == &rgd3_decl.implicit_decls;
}

int rgd3_decl_is_valid(const void* body)
{
	return rgd3_decl_base(body)->state != RGD3_DS_UNPARSED;
}

int rgd3_decl_index(const void* body)
{
	return rgd3_decl_base(body)->index;
}

int rgd3_decl_line_num(const void* body)
{
	return rgd3_decl_base(body)->source_line;
}

const char* rgd3_decl_file_name(const void* body)
{
	const Rgd3Decl* decl = rgd3_decl_base(body);
	return decl->source_file ? decl->source_file->file_name : "*invalid*";
}

const char* rgd3_decl_text(const void* body)
{
	return rgd3_decl_base(body)->text_source;
}

int rgd3_decl_text_length(const void* body)
{
	return rgd3_decl_base(body)->text_length;
}

void rgd3_decl_set_text(void* body, const char* text)
{
	rgd3_decl_set_text_local(rgd3_decl_base(body), text, (int)strlen(text));
}

void rgd3_decl_make_default(void* body)
{
	rgd3_decl_make_default_local(rgd3_decl_base(body));
}

void rgd3_decl_invalidate(void* body)
{
	rgd3_decl_base(body)->state = RGD3_DS_UNPARSED;
}

void rgd3_decl_ensure_not_purged(void* body)
{
	Rgd3Decl* decl = rgd3_decl_base(body);
	if (decl->state == RGD3_DS_UNPARSED)
		rgd3_decl_parse_local(decl);
}

int rgd3_decl_ever_referenced(const void* body)
{
	return rgd3_decl_base(body)->ever_referenced;
}

/* ---- commands ---- */

/* list<type>s [all|ever] */
static void rgd3_decl_list_type(const Rgd3CmdArgs* args, Rgd3DeclType type)
{
	const Rgd3DeclTypeDesc* desc = rgd3_decl_desc(type);
	int all = !rgd3_str_icmp(rgd3_cmdargs_argv(args, 1), "all");
	int ever = !rgd3_str_icmp(rgd3_cmdargs_argv(args, 1), "ever");
	int i, printed = 0, count = rgd3_decl.lists[(int)type].num;

	rgd3_printf("--------------------\n");
	for (i = 0; i < count; i++)
	{
		Rgd3Decl* decl = rgd3_decl.lists[(int)type].items[i];
		if (!all && decl->state == RGD3_DS_UNPARSED)
			continue;
		if (!all && !ever && !decl->referenced_this_level)
			continue;
		rgd3_printf("%s%s%4i: ", decl->referenced_this_level ? "*" : decl->ever_referenced ? "." : " ",
			decl->state == RGD3_DS_DEFAULTED ? "D" : " ", decl->index);
		printed++;
		if (decl->state == RGD3_DS_UNPARSED || !desc->list)
			rgd3_printf("%s\n", decl->name);
		else
			desc->list(decl->self);
	}
	rgd3_printf("--------------------\n");
	rgd3_printf("%i of %i %s\n", printed, count, desc->type_name);
}

/* print<type> <name> */
static void rgd3_decl_print_type(const Rgd3CmdArgs* args, Rgd3DeclType type)
{
	const Rgd3DeclTypeDesc* desc = rgd3_decl_desc(type);
	Rgd3Decl* decl;

	if (rgd3_cmdargs_argc(args) < 2)
	{
		rgd3_printf("USAGE: Print<decl type> <decl name> [type specific parms]\n");
		return;
	}
	/* skip the public path so it won't parse or reference */
	decl = rgd3_decl_find_type_without_parsing(type, rgd3_cmdargs_argv(args, 1), 0);
	if (!decl)
	{
		rgd3_printf("%s '%s' not found.\n", desc->type_name, rgd3_cmdargs_argv(args, 1));
		return;
	}
	rgd3_printf("%s %s:\n", desc->type_name, decl->name);
	rgd3_printf("source: %s:%i\n", decl->source_file->file_name, decl->source_line);
	rgd3_printf("----------\n");
	if (decl->text_source)
		rgd3_printf("%s\n", decl->text_source);
	else
		rgd3_printf("NO SOURCE\n");
	rgd3_printf("----------\n");
	switch (decl->state)
	{
	case RGD3_DS_UNPARSED: rgd3_printf("Unparsed.\n"); break;
	case RGD3_DS_DEFAULTED: rgd3_printf("<DEFAULTED>\n"); break;
	case RGD3_DS_PARSED: rgd3_printf("Parsed.\n"); break;
	}
	if (decl->referenced_this_level)
		rgd3_printf("Currently referenced this level.\n");
	else if (decl->ever_referenced)
		rgd3_printf("Referenced in a previous level.\n");
	else
		rgd3_printf("Never referenced.\n");
	if (decl->self && desc->print)
		desc->print(decl->self);
}

#define RGD3_DECL_TYPE_COMMANDS(X) \
	X(Tables, TABLE, "tables", "a table") \
	X(Materials, MATERIAL, "materials", "a material") \
	X(Skins, SKIN, "skins", "a skin") \
	X(SoundShaders, SOUND, "sound shaders", "a sound shader") \
	X(EntityDefs, ENTITYDEF, "entity defs", "an entity def") \
	X(FX, FX, "FX systems", "an FX system") \
	X(Particles, PARTICLE, "particle systems", "a particle system") \
	X(AF, AF, "articulated figures", "an articulated figure") \
	X(PDAs, PDA, "PDAs", "an PDA") \
	X(Emails, EMAIL, "Emails", "an Email") \
	X(Videos, VIDEO, "Videos", "an Audio") \
	X(Audios, AUDIO, "Audios", "a Video")

#define RGD3_DECL_LIST_THUNK(cmd, type, plural, single) \
	static void rgd3_decl_cmd_list_##cmd(const Rgd3CmdArgs* args) { rgd3_decl_list_type(args, RGD3_DECL_##type); }
RGD3_DECL_TYPE_COMMANDS(RGD3_DECL_LIST_THUNK)
#undef RGD3_DECL_LIST_THUNK

/* the print commands drop the plural "s" of the list names */
static void rgd3_decl_cmd_print_Table(const Rgd3CmdArgs* a) { rgd3_decl_print_type(a, RGD3_DECL_TABLE); }
static void rgd3_decl_cmd_print_Material(const Rgd3CmdArgs* a) { rgd3_decl_print_type(a, RGD3_DECL_MATERIAL); }
static void rgd3_decl_cmd_print_Skin(const Rgd3CmdArgs* a) { rgd3_decl_print_type(a, RGD3_DECL_SKIN); }
static void rgd3_decl_cmd_print_SoundShader(const Rgd3CmdArgs* a) { rgd3_decl_print_type(a, RGD3_DECL_SOUND); }
static void rgd3_decl_cmd_print_EntityDef(const Rgd3CmdArgs* a) { rgd3_decl_print_type(a, RGD3_DECL_ENTITYDEF); }
static void rgd3_decl_cmd_print_FX(const Rgd3CmdArgs* a) { rgd3_decl_print_type(a, RGD3_DECL_FX); }
static void rgd3_decl_cmd_print_Particle(const Rgd3CmdArgs* a) { rgd3_decl_print_type(a, RGD3_DECL_PARTICLE); }
static void rgd3_decl_cmd_print_AF(const Rgd3CmdArgs* a) { rgd3_decl_print_type(a, RGD3_DECL_AF); }
static void rgd3_decl_cmd_print_PDA(const Rgd3CmdArgs* a) { rgd3_decl_print_type(a, RGD3_DECL_PDA); }
static void rgd3_decl_cmd_print_Email(const Rgd3CmdArgs* a) { rgd3_decl_print_type(a, RGD3_DECL_EMAIL); }
static void rgd3_decl_cmd_print_Video(const Rgd3CmdArgs* a) { rgd3_decl_print_type(a, RGD3_DECL_VIDEO); }
static void rgd3_decl_cmd_print_Audio(const Rgd3CmdArgs* a) { rgd3_decl_print_type(a, RGD3_DECL_AUDIO); }

static void rgd3_decl_cmd_list_decls(const Rgd3CmdArgs* args)
{
	int i, j, total_decls = 0;
	size_t total_text = 0, total_structs = 0;
	(void)args;
	for (i = 0; i < rgd3_decl.num_types; i++)
	{
		size_t size = 0;
		int num;
		if (!rgd3_decl.types[i])
			continue;
		num = rgd3_decl.lists[i].num;
		total_decls += num;
		for (j = 0; j < num; j++)
		{
			const Rgd3Decl* decl = rgd3_decl.lists[i].items[j];
			size += sizeof(Rgd3Decl) + strlen(decl->name) + 1;
			if (decl->self)
				size += rgd3_decl.types[i]->size ? rgd3_decl.types[i]->size(decl->self) : rgd3_decl.types[i]->body_size;
		}
		total_structs += size;
		rgd3_printf("%4ik %4i %s\n", (int)(size >> 10), num, rgd3_decl.types[i]->type_name);
	}
	for (i = 0; i < rgd3_decl.num_files; i++)
		total_text += (size_t)rgd3_decl.files[i]->file_size;
	rgd3_printf("%i total decls is %i decl files\n", total_decls, rgd3_decl.num_files);
	rgd3_printf("%iKB in text, %iKB in structures\n", (int)(total_text >> 10), (int)(total_structs >> 10));
}

static void rgd3_decl_cmd_reload_decls(const Rgd3CmdArgs* args)
{
	int force = !rgd3_str_icmp(rgd3_cmdargs_argv(args, 1), "all");
	rgd3_printf(force ? "reloading all decl files:\n" : "reloading changed decl files:\n");
	rgd3_decl_reload(force);
}

static void rgd3_decl_cmd_touch(const Rgd3CmdArgs* args)
{
	int i;
	if (rgd3_cmdargs_argc(args) != 3)
	{
		rgd3_printf("usage: touch <type> <name>\n");
		rgd3_printf("valid types: ");
		for (i = 0; i < rgd3_decl.num_types; i++)
		{
			if (rgd3_decl.types[i])
				rgd3_printf("%s ", rgd3_decl.types[i]->type_name);
		}
		rgd3_printf("\n");
		return;
	}
	for (i = 0; i < rgd3_decl.num_types; i++)
	{
		if (rgd3_decl.types[i] && !rgd3_str_icmp(rgd3_decl.types[i]->type_name, rgd3_cmdargs_argv(args, 1)))
			break;
	}
	if (i >= rgd3_decl.num_types)
	{
		rgd3_printf("unknown decl type '%s'\n", rgd3_cmdargs_argv(args, 1));
		return;
	}
	if (!rgd3_decl_find((Rgd3DeclType)i, rgd3_cmdargs_argv(args, 2), 0))
		rgd3_printf("%s '%s' not found\n", rgd3_decl.types[i]->type_name, rgd3_cmdargs_argv(args, 2));
}

/* Parses every explicit decl (of one type when only != NULL); returns the
   number parsed. verbose prints a line per type. */
int rgd3_decl_parse_all(const char* only, int verbose, int* out_defaulted, int* out_warnings)
{
	int warnings_before = rgd3_log_total_warnings();
	int i, j, parsed = 0, defaulted = 0;

	for (i = 0; i < rgd3_decl.num_types; i++)
	{
		int type_parsed = 0, type_defaulted = 0, type_warnings = rgd3_log_total_warnings();
		if (!rgd3_decl.types[i] || (only && rgd3_str_icmp(only, rgd3_decl.types[i]->type_name)))
			continue;
		for (j = 0; j < rgd3_decl.lists[i].num; j++)
		{
			Rgd3Decl* decl = rgd3_decl.lists[i].items[j];
			if (decl->source_file == &rgd3_decl.implicit_decls)
				continue;
			rgd3_decl_find((Rgd3DeclType)i, decl->name, 0);
			type_parsed++;
			if (decl->state == RGD3_DS_DEFAULTED)
				type_defaulted++;
		}
		if (verbose && type_parsed)
			rgd3_printf("%6i %-18s %4i defaulted %4i warnings\n", type_parsed, rgd3_decl.types[i]->type_name, type_defaulted,
				rgd3_log_total_warnings() - type_warnings);
		parsed += type_parsed;
		defaulted += type_defaulted;
	}
	if (out_defaulted)
		*out_defaulted = defaulted;
	if (out_warnings)
		*out_warnings = rgd3_log_total_warnings() - warnings_before;
	return parsed;
}

/* parseAllDecls [type]: parses every explicit decl and reports how many
   warnings that produced (rgd3 addition, the M1 acceptance check) */
static void rgd3_decl_cmd_parse_all(const Rgd3CmdArgs* args)
{
	const char* only = rgd3_cmdargs_argc(args) > 1 ? rgd3_cmdargs_argv(args, 1) : NULL;
	int defaulted, warnings, parsed;
	u64 start = rgd3_time_us();
	parsed = rgd3_decl_parse_all(only, 1, &defaulted, &warnings);
	rgd3_printf("parsed %i decls in %.1f ms: %i defaulted, %i warnings\n", parsed,
		(double)(rgd3_time_us() - start) / 1000.0, defaulted, warnings);
}

void rgd3_decl_register(void)
{
	rgd3_cvar_register(&cv_decl_show);
	rgd3_cmd_add("listDecls", rgd3_decl_cmd_list_decls, RGD3_CMD_FL_SYSTEM, "lists all decls", NULL);
	rgd3_cmd_add("reloadDecls", rgd3_decl_cmd_reload_decls, RGD3_CMD_FL_SYSTEM, "reloads decls", NULL);
	rgd3_cmd_add("touch", rgd3_decl_cmd_touch, RGD3_CMD_FL_SYSTEM, "touches a decl", NULL);
	rgd3_cmd_add("parseAllDecls", rgd3_decl_cmd_parse_all, RGD3_CMD_FL_SYSTEM, "parses every decl and counts warnings: parseAllDecls [type]", NULL);
#define RGD3_DECL_ADD_LIST(cmd, type, plural, single) \
	rgd3_cmd_add("list" #cmd, rgd3_decl_cmd_list_##cmd, RGD3_CMD_FL_SYSTEM, "lists " plural, NULL);
	RGD3_DECL_TYPE_COMMANDS(RGD3_DECL_ADD_LIST)
#undef RGD3_DECL_ADD_LIST
	rgd3_cmd_add("printTable", rgd3_decl_cmd_print_Table, RGD3_CMD_FL_SYSTEM, "prints a table", NULL);
	rgd3_cmd_add("printMaterial", rgd3_decl_cmd_print_Material, RGD3_CMD_FL_SYSTEM, "prints a material", NULL);
	rgd3_cmd_add("printSkin", rgd3_decl_cmd_print_Skin, RGD3_CMD_FL_SYSTEM, "prints a skin", NULL);
	rgd3_cmd_add("printSoundShader", rgd3_decl_cmd_print_SoundShader, RGD3_CMD_FL_SYSTEM, "prints a sound shader", NULL);
	rgd3_cmd_add("printEntityDef", rgd3_decl_cmd_print_EntityDef, RGD3_CMD_FL_SYSTEM, "prints an entity def", NULL);
	rgd3_cmd_add("printFX", rgd3_decl_cmd_print_FX, RGD3_CMD_FL_SYSTEM, "prints an FX system", NULL);
	rgd3_cmd_add("printParticle", rgd3_decl_cmd_print_Particle, RGD3_CMD_FL_SYSTEM, "prints a particle system", NULL);
	rgd3_cmd_add("printAF", rgd3_decl_cmd_print_AF, RGD3_CMD_FL_SYSTEM, "prints an articulated figure", NULL);
	rgd3_cmd_add("printPDA", rgd3_decl_cmd_print_PDA, RGD3_CMD_FL_SYSTEM, "prints an PDA", NULL);
	rgd3_cmd_add("printEmail", rgd3_decl_cmd_print_Email, RGD3_CMD_FL_SYSTEM, "prints an Email", NULL);
	rgd3_cmd_add("printVideo", rgd3_decl_cmd_print_Video, RGD3_CMD_FL_SYSTEM, "prints an Audio", NULL);
	rgd3_cmd_add("printAudio", rgd3_decl_cmd_print_Audio, RGD3_CMD_FL_SYSTEM, "prints a Video", NULL);
}

/* ---- lifetime ---- */

void rgd3_decl_init(void)
{
	rgd3_printf("----- Initializing Decls -----\n");
	rgd3_decl.implicit_decls.file_name = "<implicit file>";
	rgd3_decl.implicit_decls.default_type = RGD3_DECL_MAX_TYPES;
	rgd3_decl_register_basic_types();
	rgd3_decl_register_folder("materials", ".mtr", RGD3_DECL_MATERIAL);
	rgd3_decl.initialized = 1;
	rgd3_printf("------------------------------\n");
}

void rgd3_decl_init2(void)
{
	rgd3_decl_register_folder("skins", ".skin", RGD3_DECL_SKIN);
	rgd3_decl_register_folder("sound", ".sndshd", RGD3_DECL_SOUND);
}

/* idGameLocal::Init's decl registration; moves to the game module in M4 */
void rgd3_decl_init_game(void)
{
	rgd3_decl_register_game_types();
	rgd3_decl_register_folder("def", ".def", RGD3_DECL_ENTITYDEF);
	rgd3_decl_register_folder("fx", ".fx", RGD3_DECL_FX);
	rgd3_decl_register_folder("particles", ".prt", RGD3_DECL_PARTICLE);
	rgd3_decl_register_folder("af", ".af", RGD3_DECL_AF);
	rgd3_decl_register_folder("newpdas", ".pda", RGD3_DECL_PDA);
}

void rgd3_decl_shutdown(void)
{
	int i, j;
	if (!rgd3_decl.initialized)
		return;
	for (i = 0; i < RGD3_DECL_MAX_TYPES; i++)
	{
		for (j = 0; j < rgd3_decl.lists[i].num; j++)
		{
			Rgd3Decl* decl = rgd3_decl.lists[i].items[j];
			if (decl->self)
			{
				rgd3_decl_call_free_data(decl);
				rgd3_heap_free(decl->self);
			}
			if (decl->text_source)
				rgd3_heap_free(decl->text_source);
			rgd3_heap_free(decl->name);
			rgd3_heap_free(decl);
		}
		if (rgd3_decl.lists[i].items)
			rgd3_heap_free(rgd3_decl.lists[i].items);
		memset(&rgd3_decl.lists[i], 0, sizeof(rgd3_decl.lists[i]));
		rgd3_hashindex_free(&rgd3_decl.hash_tables[i]);
	}
	for (i = 0; i < rgd3_decl.num_files; i++)
	{
		rgd3_heap_free(rgd3_decl.files[i]->file_name);
		rgd3_heap_free(rgd3_decl.files[i]);
	}
	if (rgd3_decl.files)
		rgd3_heap_free(rgd3_decl.files);
	for (i = 0; i < rgd3_decl.num_folders; i++)
	{
		rgd3_heap_free(rgd3_decl.folders[i].folder);
		rgd3_heap_free(rgd3_decl.folders[i].extension);
	}
	if (rgd3_decl.folders)
		rgd3_heap_free(rgd3_decl.folders);
	rgd3_decl.files = NULL;
	rgd3_decl.num_files = 0;
	rgd3_decl.folders = NULL;
	rgd3_decl.num_folders = 0;
	for (i = 0; i < RGD3_DECL_MAX_TYPES; i++)
		rgd3_decl.types[i] = NULL;
	rgd3_decl.num_types = 0;
	rgd3_decl.initialized = 0;
}
