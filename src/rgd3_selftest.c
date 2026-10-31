/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_selftest.c - `rgd3_dev.exe --self-test`: unit checks for the
   foundation modules plus data checks against the shipped BFG files
   (container sizes, decl counts, a known entityDef, a lang string, the
   mars_city1 map and preload manifest). Returns the number of failures;
   the process exit code is 0 only when everything passed. */

static int rgd3_selftest_checks;
static int rgd3_selftest_failures;

#define RGD3_CHECK(cond) rgd3_selftest_check((cond) != 0, #cond, __FILE__, __LINE__)

static void rgd3_selftest_check(int ok, const char* what, const char* file, int line)
{
	rgd3_selftest_checks++;
	if (!ok)
	{
		rgd3_selftest_failures++;
		rgd3_printf("^1FAILED^7 %s (%s:%d)\n", what, rgd3_str_file_name(file), line);
	}
}

static void rgd3_selftest_section(const char* name, int checks_before, int failures_before)
{
	rgd3_printf("%-10s %4d checks, %d failures\n", name, rgd3_selftest_checks - checks_before, rgd3_selftest_failures - failures_before);
}

/* ---- foundation ---- */

static void rgd3_selftest_str(void)
{
	char buf[64];
	Rgd3StrBuf sb = { 0 };
	float f3[3] = { 1.0f, 0.0f, 0.5f };

	RGD3_CHECK(rgd3_str_icmp("Hello", "hELLO") == 0);
	RGD3_CHECK(rgd3_str_icmp("a_b", "aa") < 0); /* idStr lowercases: '_' < 'a' */
	RGD3_CHECK(rgd3_str_filter("fs_*", "fs_basepath", 0));
	RGD3_CHECK(!rgd3_str_filter("fs_*", "r_mode", 0));
	RGD3_CHECK(rgd3_str_filter("*.cfg", "default.cfg", 0));
	rgd3_str_float_to_string(1.5f, buf, sizeof(buf));
	RGD3_CHECK(strcmp(buf, "1.5") == 0);
	RGD3_CHECK(strcmp(rgd3_str_float_array_to_string(f3, 3, 2), "1 0 0.5") == 0);
	RGD3_CHECK(rgd3_str_id_hash("a") == 'a' * 119);
	RGD3_CHECK(rgd3_str_id_ihash("AB") == rgd3_str_id_hash("ab"));
	RGD3_CHECK(rgd3_crc32_block("123456789", 9) == 0xCBF43926u);

	rgd3_strbuf_set(&sb, "  x y  ");
	rgd3_strbuf_strip(&sb, ' ');
	RGD3_CHECK(strcmp(rgd3_strbuf_cstr(&sb), "x y") == 0);
	rgd3_strbuf_printf(&sb, "%d", 42);
	RGD3_CHECK(strcmp(rgd3_strbuf_cstr(&sb), "x y42") == 0 && sb.len == 5);
	rgd3_strbuf_free(&sb);
	RGD3_CHECK(strcmp(rgd3_strbuf_cstr(&sb), "") == 0);
}

static void rgd3_selftest_cmdargs(void)
{
	Rgd3CmdArgs args;
	char match[256];

	rgd3_cmdargs_tokenize(&args, "bind \"F12\" \"echo hi\" -5 1.5 // comment", 0);
	RGD3_CHECK(rgd3_cmdargs_argc(&args) == 5);
	RGD3_CHECK(strcmp(rgd3_cmdargs_argv(&args, 2), "echo hi") == 0);
	RGD3_CHECK(strcmp(rgd3_cmdargs_argv(&args, 3), "-5") == 0);
	RGD3_CHECK(strcmp(rgd3_cmdargs_argv(&args, 4), "1.5") == 0);
	rgd3_cmdargs_tokenize(&args, "set fs_game \"\"; echo $fs_debug", 0);
	RGD3_CHECK(rgd3_cmdargs_argc(&args) == 6 && strcmp(rgd3_cmdargs_argv(&args, 5), "0") == 0);
	rgd3_cmdargs_tokenize(&args, "listCvars fs_*", 0);
	RGD3_CHECK(rgd3_cmdargs_argc(&args) == 3);
	RGD3_CHECK(strcmp(rgd3_cmdargs_match(&args, 1, match, sizeof(match)), "fs_*") == 0);
	rgd3_cmdargs_tokenize(&args, "map game/mars_city1 192.168.0.1:27666", 0);
	RGD3_CHECK(strcmp(rgd3_cmdargs_argv(&args, 1), "game/mars_city1") == 0);
	RGD3_CHECK(strcmp(rgd3_cmdargs_argv(&args, 2), "192.168.0.1:27666") == 0);
}

static void rgd3_selftest_hashindex(void)
{
	Rgd3HashIndex h = { 0 };
	int i, found;

	RGD3_CHECK(rgd3_hashindex_first(&h, 5) == -1);
	for (i = 0; i < 3000; i++)
		rgd3_hashindex_add(&h, rgd3_hashindex_key_int(&h, i * 7), i);
	found = 0;
	for (i = rgd3_hashindex_first(&h, rgd3_hashindex_key_int(&h, 700)); i != -1; i = rgd3_hashindex_next(&h, i))
		found += i == 100;
	RGD3_CHECK(found == 1);
	rgd3_hashindex_remove(&h, rgd3_hashindex_key_int(&h, 700), 100);
	found = 0;
	for (i = rgd3_hashindex_first(&h, rgd3_hashindex_key_int(&h, 700)); i != -1; i = rgd3_hashindex_next(&h, i))
		found += i == 100;
	RGD3_CHECK(found == 0);
	RGD3_CHECK(rgd3_hashindex_spread(&h) > 50);
	rgd3_hashindex_free(&h);
}

static void rgd3_selftest_dict(void)
{
	Rgd3Dict a = { 0 }, b = { 0 };
	const Rgd3KeyValue* kv;
	Rgd3Vec3 v;
	Rgd3Mat3 m;
	int n, checksum;

	rgd3_dict_set(&a, "Origin", "1 2 3");
	rgd3_dict_set(&a, "name", "player1");
	rgd3_dict_set(&a, "def_drops1", "ammo_shells");
	rgd3_dict_set(&a, "def_drops2", "ammo_clip");
	rgd3_dict_set(&a, "health", "100");
	RGD3_CHECK(rgd3_dict_num(&a) == 5);
	RGD3_CHECK(strcmp(rgd3_dict_get_string(&a, "ORIGIN", ""), "1 2 3") == 0);
	RGD3_CHECK(rgd3_dict_get_int(&a, "health", 0) == 100);
	RGD3_CHECK(rgd3_dict_get_float_s(&a, "missing", "0.25") == 0.25f);
	RGD3_CHECK(rgd3_dict_get_bool(&a, "missing", 1) == 1);
	RGD3_CHECK(rgd3_dict_get_vector(&a, "origin", NULL, &v) && v.x == 1.0f && v.y == 2.0f && v.z == 3.0f);
	RGD3_CHECK(!rgd3_dict_get_matrix(&a, "rotation", NULL, &m) && m.mat[1].y == 1.0f && m.mat[0].y == 0.0f);

	kv = rgd3_dict_match_prefix(&a, "def_drops", NULL);
	RGD3_CHECK(kv && strcmp(rgd3_kv_value(kv), "ammo_shells") == 0);
	kv = rgd3_dict_match_prefix(&a, "def_drops", kv);
	RGD3_CHECK(kv && strcmp(rgd3_kv_value(kv), "ammo_clip") == 0);
	RGD3_CHECK(rgd3_dict_match_prefix(&a, "def_drops", kv) == NULL);

	/* delete keeps the order of the rest */
	rgd3_dict_delete(&a, "NAME");
	RGD3_CHECK(rgd3_dict_num(&a) == 4 && !rgd3_dict_find_key(&a, "name"));
	RGD3_CHECK(strcmp(rgd3_kv_key(rgd3_dict_get_kv(&a, 1)), "def_drops1") == 0);
	RGD3_CHECK(rgd3_dict_find_key_index(&a, "health") == 3);

	checksum = rgd3_dict_checksum(&a);
	rgd3_dict_assign(&b, &a);
	RGD3_CHECK(rgd3_dict_checksum(&b) == checksum);
	rgd3_dict_set(&b, "health", "50");
	RGD3_CHECK(rgd3_dict_checksum(&b) != checksum);
	RGD3_CHECK(rgd3_dict_get_int(&a, "health", 0) == 100);

	/* set_defaults only adds missing keys */
	rgd3_dict_clear(&b);
	rgd3_dict_set(&b, "health", "50");
	rgd3_dict_set_defaults(&b, &a);
	RGD3_CHECK(rgd3_dict_get_int(&b, "health", 0) == 50 && rgd3_dict_num(&b) == 4);

	/* copy overwrites */
	rgd3_dict_copy(&b, &a);
	RGD3_CHECK(rgd3_dict_get_int(&b, "health", 0) == 100);

	n = rgd3_dict_num(&a);
	rgd3_dict_transfer(&b, &a);
	RGD3_CHECK(rgd3_dict_num(&a) == 0 && rgd3_dict_num(&b) == n);
	rgd3_dict_clear(&a);
	rgd3_dict_clear(&b);

	/* many keys force the index to grow */
	for (n = 0; n < 500; n++)
		rgd3_dict_set_int(&a, rgd3_va("key%d", n), n);
	RGD3_CHECK(rgd3_dict_get_int(&a, "KEY377", -1) == 377);
	rgd3_dict_clear(&a);
}

static void rgd3_selftest_dict_parse(void)
{
	static const char text[] = "#define HEALTH 100\n{ \"name\" \"imp\" \"health\" \"HEALTH\" \"name\" \"twice\" }";
	Rgd3Parser parser;
	Rgd3Dict d = { 0 };
	int ok;

	/* without NOSTRINGCONCAT adjacent strings would merge into one */
	rgd3_parser_init(&parser, LEXFL_NOSTRINGCONCAT | LEXFL_NOWARNINGS | LEXFL_NOFATALERRORS);
	rgd3_parser_load_memory(&parser, text, (int)strlen(text), "dict parse test");
	ok = rgd3_dict_parse(&d, &parser);
	RGD3_CHECK(!ok); /* "name" is repeated */
	RGD3_CHECK(rgd3_dict_num(&d) == 2);
	RGD3_CHECK(strcmp(rgd3_dict_get_string(&d, "name", ""), "twice") == 0);
	RGD3_CHECK(strcmp(rgd3_dict_get_string(&d, "health", ""), "HEALTH") == 0); /* strings are not expanded */
	rgd3_dict_clear(&d);
	rgd3_parser_free(&parser);
}

/* ---- drops: nothing may leak when an error unwinds through a parse ---- */

typedef struct Rgd3SelftestTags
{
	i64 count[RGD3_TAG_COUNT];
	int unwind;
} Rgd3SelftestTags;

static void rgd3_selftest_tags(Rgd3SelftestTags* t)
{
	int i;
	for (i = 0; i < RGD3_TAG_COUNT; i++)
		t->count[i] = rgd3_mem_tag_count((Rgd3MemTag)i);
	t->unwind = rgd3_error_unwind_count();
}

static int rgd3_selftest_tags_equal(const Rgd3SelftestTags* a, const Rgd3SelftestTags* b)
{
	int i, same = a->unwind == b->unwind;
	for (i = 0; i < RGD3_TAG_COUNT; i++)
	{
		if (a->count[i] != b->count[i])
		{
			rgd3_printf("  tag %s: %lld blocks before, %lld after\n", rgd3_mem_tag_name((Rgd3MemTag)i),
				(long long)a->count[i], (long long)b->count[i]);
			same = 0;
		}
	}
	if (a->unwind != b->unwind)
		rgd3_printf("  unwind registry: %d before, %d after\n", a->unwind, b->unwind);
	return same;
}

static char rgd3_selftest_long_name[301];

static void rgd3_selftest_make_long_name(void)
{
	int i;
	for (i = 0; i < 300; i++)
		rgd3_selftest_long_name[i] = (char)('a' + i % 26);
	rgd3_selftest_long_name[300] = 0;
}

/* a stack lexer holding a long (heap) token when the lexer errors */
static void rgd3_selftest_drop_lexer(void)
{
	char text[400];
	Rgd3Lexer lex;
	Rgd3Token token = { 0 };
	rg_snprintf(text, sizeof(text), "%s @", rgd3_selftest_long_name);
	rgd3_lexer_init(&lex, 0); /* errors are fatal */
	rgd3_lexer_load_memory(&lex, text, (int)strlen(text), "drop test", 1);
	rgd3_lexer_read_token(&lex, &token); /* 300 characters: heap text */
	rgd3_lexer_unread_token(&lex, &token); /* the lexer's copy is heap too */
	rgd3_lexer_read_token(&lex, &token);
	rgd3_lexer_read_token(&lex, &token); /* '@': unknown punctuation, drops */
}

/* Codex's case: an undefined 300-character name in #if */
static void rgd3_selftest_drop_parser(void)
{
	char text[400];
	Rgd3Parser parser;
	Rgd3Token token = { 0 };
	rg_snprintf(text, sizeof(text), "#if %s\nx\n#endif\n", rgd3_selftest_long_name);
	rgd3_parser_init(&parser, 0);
	rgd3_parser_load_memory(&parser, text, (int)strlen(text), "drop test");
	rgd3_parser_read_token(&parser, &token);
}

static const char rgd3_selftest_truncated_map[] =
	"Version 2\n{\n\"classname\" \"worldspawn\"\n{\nbrushDef3\n{\n"
	"( 0 0 1 0 ) ( ( 0.0078125 0 0 ) ( 0 0.0078125 0 ) ) \"textures/common/caulk\" 0 0 0\n"
	"( 0 0 -1 0 ) ( ( 0.0078125 0 0 ";

/* the map lives in this frame, which the drop unwinds */
static void rgd3_selftest_drop_map_local(void)
{
	Rgd3MapFile map = { 0 };
	rgd3_mapfile_parse(&map, "rgd3_selftest_truncated", 1, 0);
}

/* outside the stack, like a map owned by the game: a drop leaves it to its
   owner (static, so it is also valid after longjmp) */
static Rgd3MapFile rgd3_selftest_survivor;

static void rgd3_selftest_drops(void)
{
	static const char* labels[] = { "lexer (callee frame)", "parser (callee frame)", "lexer (inside the guarded block)",
		"map (callee frame)", "map (inside the guarded block)", "map (owned outside the stack)" };
	Rgd3SelftestTags before, after;
	Rgd3File* f;
	char os_path[RGD3_MAX_OSPATH] = { 0 };
	volatile int which;
	int num_tests;

	rgd3_selftest_make_long_name();
	f = rgd3_fs_open_write("rgd3_selftest_truncated.map");
	if (f)
	{
		rgd3_str_copynz(os_path, rgd3_fs_file_name(f), sizeof(os_path));
		rgd3_fs_write(f, rgd3_selftest_truncated_map, sizeof(rgd3_selftest_truncated_map) - 1);
		rgd3_fs_close(f);
	}
	num_tests = f ? 6 : 3; /* the map tests need a writable save path */
	rgd3_printf("drop tests: the next %d ERROR lines are expected\n", num_tests);
	for (which = 0; which < num_tests; which++)
	{
		Rgd3ErrorGuard guard;
		volatile int dropped = 0;
		rgd3_error_reset_drop_count();
		rgd3_selftest_tags(&before);
		rgd3_error_push(&guard);
		if (setjmp(guard.jump) == 0)
		{
			if (which == 0)
			{
				rgd3_selftest_drop_lexer();
			}
			else if (which == 1)
			{
				rgd3_selftest_drop_parser();
			}
			else if (which == 2)
			{
				/* declared in the guard's own function */
				char text[400];
				Rgd3Lexer lex;
				Rgd3Token token = { 0 };
				rg_snprintf(text, sizeof(text), "%s @", rgd3_selftest_long_name);
				rgd3_lexer_init(&lex, 0);
				rgd3_lexer_load_memory(&lex, text, (int)strlen(text), "drop test", 1);
				rgd3_lexer_read_token(&lex, &token);
				rgd3_lexer_read_token(&lex, &token); /* drops */
			}
			else if (which == 3)
			{
				rgd3_selftest_drop_map_local();
			}
			else if (which == 4)
			{
				Rgd3MapFile map = { 0 };
				rgd3_mapfile_parse(&map, "rgd3_selftest_truncated", 1, 0);
			}
			else
			{
				rgd3_mapfile_parse(&rgd3_selftest_survivor, "rgd3_selftest_truncated", 1, 0);
			}
			rgd3_error_pop(&guard);
		}
		else
		{
			dropped = 1;
		}
		/* the map owned outside the stack keeps its partial data for its owner */
		if (which == 5)
		{
			RGD3_CHECK(rgd3_selftest_survivor.num_entities == 0 && rgd3_selftest_survivor.pending_entity != NULL);
			rgd3_mapfile_free(&rgd3_selftest_survivor);
		}
		rgd3_selftest_tags(&after);
		if (!dropped)
			rgd3_printf("^1drop test %s did not drop\n", labels[which]);
		RGD3_CHECK(dropped);
		if (!rgd3_selftest_tags_equal(&before, &after))
			rgd3_printf("^1drop test %s leaked\n", labels[which]);
		RGD3_CHECK(rgd3_selftest_tags_equal(&before, &after));
	}
	rgd3_error_reset_drop_count();
	if (os_path[0])
		SDL_RemovePath(os_path);
}

static void rgd3_selftest_bin(void)
{
	Rgd3BinWriter w = { 0 };
	Rgd3BinReader r;
	Rgd3StrBuf s = { 0 };

	rgd3_bin_write_u32_be(&w, 0xD000000Du);
	rgd3_bin_write_string(&w, "maps/game/mars_city1.map");
	rgd3_bin_write_f32_be(&w, 1.5f);
	rgd3_bin_write_i64_be(&w, -2);
	RGD3_CHECK(w.size == 4 + 4 + 24 + 4 + 8);
	RGD3_CHECK(w.data[0] == 0xD0 && w.data[4] == 24 && w.data[5] == 0); /* length is little-endian */

	rgd3_bin_reader_init(&r, w.data, w.size);
	RGD3_CHECK(rgd3_bin_u32_be(&r) == 0xD000000Du);
	RGD3_CHECK(rgd3_bin_string(&r, &s) == 24 && strcmp(rgd3_strbuf_cstr(&s), "maps/game/mars_city1.map") == 0);
	RGD3_CHECK(rgd3_bin_f32_be(&r) == 1.5f);
	RGD3_CHECK(rgd3_bin_i64_be(&r) == -2);
	RGD3_CHECK(!r.error && rgd3_bin_remaining(&r) == 0);
	RGD3_CHECK(rgd3_bin_u32_be(&r) == 0 && r.error);
	rgd3_strbuf_free(&s);
	rgd3_bin_writer_free(&w);
}

/* ---- shipped data ---- */

/* Tokenizes every decl file with the decl lexer flags and CRCs the token
   types, subtypes, line numbers and text. The pinned values guard the
   lexer against regressions; they were recorded from this port, cross-
   checked by the decl and map results below rather than by the C++. */
#define RGD3_SELFTEST_CORPUS_FILES 896
#define RGD3_SELFTEST_CORPUS_TOKENS 668772u
#define RGD3_SELFTEST_CORPUS_CRC 0x39f535dfu

static void rgd3_selftest_lexer_corpus(void)
{
	static const char* folders[][2] = {
		{ "materials", ".mtr" }, { "skins", ".skin" }, { "sound", ".sndshd" }, { "def", ".def" },
		{ "fx", ".fx" }, { "particles", ".prt" }, { "af", ".af" }, { "newpdas", ".pda" },
	};
	Rgd3Token token = { 0 };
	u32 crc, tokens = 0;
	int files = 0, f, i;

	rgd3_crc32_init(&crc);
	for (f = 0; f < (int)RG_ARRAY_COUNT(folders); f++)
	{
		Rgd3FileList* list = rgd3_fs_list_files(folders[f][0], folders[f][1], 0, 0);
		for (i = 0; list && i < list->count; i++)
		{
			Rgd3Lexer lex;
			char path[RGD3_MAX_OSPATH];
			rg_snprintf(path, sizeof(path), "%s/%s", folders[f][0], list->names[i]);
			rgd3_lexer_init(&lex, RGD3_DECL_LEXER_FLAGS | LEXFL_NOWARNINGS);
			if (!rgd3_lexer_load_file(&lex, path, 0))
			{
				rgd3_lexer_free(&lex);
				continue;
			}
			files++;
			while (rgd3_lexer_read_token(&lex, &token))
			{
				int head[3];
				head[0] = token.type;
				head[1] = token.subtype;
				head[2] = token.line;
				rgd3_crc32_update(&crc, head, sizeof(head));
				rgd3_crc32_update(&crc, token.data, token.len);
				tokens++;
			}
			rgd3_lexer_free(&lex);
		}
		if (list)
			rgd3_fs_free_file_list(list);
	}
	rgd3_crc32_finish(&crc);
	rgd3_token_free(&token);
	rgd3_printf("lexer corpus: %d files, %u tokens, crc 0x%08x\n", files, tokens, crc);
	RGD3_CHECK(files == RGD3_SELFTEST_CORPUS_FILES);
	RGD3_CHECK(tokens == RGD3_SELFTEST_CORPUS_TOKENS);
	RGD3_CHECK(crc == RGD3_SELFTEST_CORPUS_CRC);
}

static u32 rgd3_selftest_container_entries(const char* file_name)
{
	int i;
	for (i = 0; i < rgd3_fs_num_containers(); i++)
	{
		if (!rgd3_str_icmp(rgd3_str_file_name(rgd3_fs_container_path(i)), file_name))
			return rgd3_fs_container_entry_count(i);
	}
	return 0;
}

static void rgd3_selftest_data(void)
{
	const Rgd3DeclEntityDef* imp;
	const Rgd3DeclTable* table;
	Rgd3PreloadManifest preload;
	Rgd3MapFile map = { 0 };
	int defaulted = 0, warnings = 0, parsed, i, unknown = 0;

	/* .resources tables (verified 2026-10-29) */
	RGD3_CHECK(rgd3_selftest_container_entries("_common.resources") == 2262);
	RGD3_CHECK(rgd3_selftest_container_entries("_ordered.resources") == 1336);
	RGD3_CHECK(rgd3_selftest_container_entries("_sound_pc.resources") == 4759);
	RGD3_CHECK(rgd3_selftest_container_entries("_sound_pc_en.resources") == 840);

	/* decls: every explicit decl parses; the only defaults are three tables
	   with typos in the shipped .mtr files (lavaflicker3, fivestage, blood2),
	   plus one duplicate key warning in monster_hunter_invul.def */
	RGD3_CHECK(rgd3_decl_num_decls(RGD3_DECL_MATERIAL) >= 7672);
	RGD3_CHECK(rgd3_decl_num_decls(RGD3_DECL_ENTITYDEF) >= 1572);
	parsed = rgd3_decl_parse_all(NULL, 0, &defaulted, &warnings);
	RGD3_CHECK(parsed >= 15667);
	RGD3_CHECK(defaulted == 3);
	RGD3_CHECK(warnings == 4);

	imp = rgd3_decl_find_entity_def("monster_demon_imp", 0);
	RGD3_CHECK(imp != NULL);
	if (imp)
	{
		RGD3_CHECK(rgd3_dict_get_int(&imp->dict, "health", 0) == 130);
		RGD3_CHECK(strcmp(rgd3_dict_get_string(&imp->dict, "classname", ""), "monster_demon_imp") == 0);
		RGD3_CHECK(!rgd3_dict_find_key(&imp->dict, "inherit"));
		/* inherited from monster_default */
		RGD3_CHECK(rgd3_dict_find_key(&imp->dict, "editor_var noDamage") != NULL);
	}
	RGD3_CHECK(rgd3_decl_find_entity_def("no_such_def_xyz", 0) == NULL);

	table = (const Rgd3DeclTable*)rgd3_decl_find(RGD3_DECL_TABLE, "sintable", 0);
	RGD3_CHECK(table != NULL && table->num_values > 2);
	if (table)
		RGD3_CHECK(fabsf(rgd3_decl_table_lookup(table, 0.25f) - 1.0f) < 0.01f);

	RGD3_CHECK(rgd3_lang_num_strings() > 9000);
	RGD3_CHECK(strcmp(rgd3_lang_get_string("#str_02990"), "Sub Level 9") == 0);
	RGD3_CHECK(strcmp(rgd3_lang_get_string("#str_no_such_key"), "#str_no_such_key") == 0);

	/* mars_city1 */
	RGD3_CHECK(rgd3_preload_manifest_load(&preload, "maps/mars_city1.preload"));
	RGD3_CHECK(preload.num_entries == 4053);
	rgd3_preload_manifest_free(&preload);

	RGD3_CHECK(rgd3_fs_load_map_resources("game/mars_city1"));
	RGD3_CHECK(rgd3_mapfile_parse(&map, "maps/game/mars_city1", 1, 0));
	RGD3_CHECK(map.num_entities == 2026);
	RGD3_CHECK(map.geometry_crc == 0x8b89b51cu);
	RGD3_CHECK(map.num_entities && !rgd3_str_icmp(rgd3_dict_get_string(&map.entities[0]->epairs, "classname", ""), "worldspawn"));
	for (i = 1; i < map.num_entities; i++)
	{
		if (!rgd3_decl_find(RGD3_DECL_ENTITYDEF, rgd3_dict_get_string(&map.entities[i]->epairs, "classname", ""), 0))
			unknown++;
	}
	RGD3_CHECK(unknown == 0);
	rgd3_mapfile_free(&map);
	rgd3_fs_unload_map_resources();
}

int rgd3_selftest_run(void)
{
	int c, f;
	u64 start = rgd3_time_us();

	rgd3_printf("------- rgd3 self-test -------\n");
	c = rgd3_selftest_checks;
	f = rgd3_selftest_failures;
	rgd3_selftest_str();
	rgd3_selftest_section("str", c, f);

	c = rgd3_selftest_checks;
	f = rgd3_selftest_failures;
	rgd3_selftest_cmdargs();
	rgd3_selftest_section("cmdargs", c, f);

	c = rgd3_selftest_checks;
	f = rgd3_selftest_failures;
	rgd3_selftest_hashindex();
	rgd3_selftest_dict();
	rgd3_selftest_section("dict", c, f);

	c = rgd3_selftest_checks;
	f = rgd3_selftest_failures;
	rgd3_selftest_bin();
	rgd3_selftest_section("bin", c, f);

	/* modules with their own tests count as one check each */
	RGD3_CHECK(rgd3_math_self_test());
	RGD3_CHECK(rgd3_lexer_self_test());
	RGD3_CHECK(rgd3_parser_self_test()); /* prints one expected ERROR line */
	rgd3_selftest_dict_parse();

	c = rgd3_selftest_checks;
	f = rgd3_selftest_failures;
	rgd3_selftest_drops();
	rgd3_selftest_section("drops", c, f);

	c = rgd3_selftest_checks;
	f = rgd3_selftest_failures;
	rgd3_selftest_lexer_corpus();
	rgd3_selftest_data();
	rgd3_selftest_section("data", c, f);

	rgd3_printf("self-test: %d checks, %d failures, %.0f ms\n", rgd3_selftest_checks, rgd3_selftest_failures,
		(double)(rgd3_time_us() - start) / 1000.0);
	return rgd3_selftest_failures;
}
