/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_decl_basic.c - see rgd3_decl_basic.h. Parsers lifted from
   neo/framework/DeclTable.cpp (Parse, TableLookup), DeclEntityDef.cpp
   (Parse with "inherit"), DeclSkin.cpp (Parse, SetDefaultText) and
   DeclPDA.cpp (pda/email/video/audio Parse, lang string ids); type
   keywords and allocators from DeclManager.cpp Init and Game_local.cpp. */

static Rgd3Cvar cv_g_useOldPDAStrings = RGD3_CVAR("g_useOldPDAStrings", "0", RGD3_CVAR_BOOL, "Read strings from the .pda files rather than from the .lang file");

#define RGD3_PDA_LEXER_FLAGS (LEXFL_NOSTRINGCONCAT | LEXFL_ALLOWPATHNAMES | LEXFL_ALLOWMULTICHARLITERALS | \
	LEXFL_ALLOWBACKSLASHSTRINGCONCAT | LEXFL_NOFATALERRORS)

static void rgd3_decl_basic_set_str(char** dst, const char* value)
{
	if (*dst)
		rgd3_heap_free(*dst);
	*dst = rgd3_heap_strdup(value ? value : "", RGD3_TAG_DECL);
}

static void rgd3_decl_basic_free_str(char** s)
{
	if (*s)
		rgd3_heap_free(*s);
	*s = NULL;
}

static void rgd3_decl_basic_open(Rgd3Lexer* src, const void* body, const char* text, int length, int flags)
{
	rgd3_lexer_init(src, flags);
	rgd3_lexer_load_memory(src, text, length, rgd3_decl_file_name(body), rgd3_decl_line_num(body));
	rgd3_lexer_skip_until_string(src, "{");
}

/* ---- table ---- */

static void rgd3_decl_table_free(void* body)
{
	Rgd3DeclTable* t = (Rgd3DeclTable*)body;
	t->snap = 0;
	t->clamp = 0;
	if (t->values)
		rgd3_heap_free(t->values);
	t->values = NULL;
	t->num_values = 0;
}

static void rgd3_decl_table_append(Rgd3DeclTable* t, float v)
{
	if ((t->num_values & 15) == 0)
		t->values = (float*)rgd3_heap_realloc(t->values, (size_t)(t->num_values + 16) * sizeof(float), RGD3_TAG_DECL);
	t->values[t->num_values++] = v;
}

static const char* rgd3_decl_table_default(const void* body)
{
	(void)body;
	return "{ { 0 } }";
}

static int rgd3_decl_table_parse(void* body, const char* text, int length, int allow_binary)
{
	Rgd3DeclTable* t = (Rgd3DeclTable*)body;
	Rgd3Lexer src;
	Rgd3Token token = { 0 };
	int ok = 1;
	(void)allow_binary;

	rgd3_decl_basic_open(&src, body, text, length, RGD3_DECL_LEXER_FLAGS);
	t->snap = 0;
	t->clamp = 0;
	t->num_values = 0;

	while (ok)
	{
		if (!rgd3_lexer_read_token(&src, &token))
			break;
		if (rgd3_token_is(&token, "}"))
			break;
		if (!rgd3_token_icmp(&token, "snap"))
		{
			t->snap = 1;
		}
		else if (!rgd3_token_icmp(&token, "clamp"))
		{
			t->clamp = 1;
		}
		else if (!rgd3_token_icmp(&token, "{"))
		{
			for (;;)
			{
				int error_flag;
				float v = rgd3_lexer_parse_float(&src, &error_flag);
				if (error_flag)
				{
					/* something non-numeric */
					ok = 0;
					break;
				}
				rgd3_decl_table_append(t, v);
				rgd3_lexer_read_token(&src, &token);
				if (rgd3_token_is(&token, "}"))
					break;
				if (rgd3_token_is(&token, ","))
					continue;
				rgd3_lexer_warning(&src, "expected comma or brace");
				ok = 0;
				break;
			}
		}
		else
		{
			rgd3_lexer_warning(&src, "unknown token '%s'", token.data);
			ok = 0;
		}
	}
	rgd3_token_free(&token);
	rgd3_lexer_free(&src);
	if (!ok)
	{
		rgd3_decl_make_default(body);
		return 0;
	}
	/* repeat element 0 at the end so lerping needs no wrap case */
	rgd3_decl_table_append(t, t->num_values ? t->values[0] : 0.0f);
	return 1;
}

float rgd3_decl_table_lookup(const Rgd3DeclTable* t, float index)
{
	int i_index;
	float i_frac;
	int domain = t->num_values - 1;

	if (domain <= 1)
		return 1.0f;
	if (t->clamp)
	{
		index *= (float)(domain - 1);
		if (index >= (float)(domain - 1))
			return t->values[domain - 1];
		else if (index <= 0.0f)
			return t->values[0];
		i_index = (int)index;
		i_frac = index - (float)i_index;
	}
	else
	{
		index *= (float)domain;
		if (index < 0.0f)
			index += (float)domain * ceilf(-index / (float)domain);
		i_index = (int)floorf(index);
		i_frac = index - (float)i_index;
		i_index = i_index % domain;
	}
	if (!t->snap)
	{
		/* element 0 is duplicated at the end, so no wrap is needed */
		return t->values[i_index] * (1.0f - i_frac) + t->values[i_index + 1] * i_frac;
	}
	return t->values[i_index];
}

static size_t rgd3_decl_table_size(const void* body)
{
	return sizeof(Rgd3DeclTable) + (size_t)((const Rgd3DeclTable*)body)->num_values * sizeof(float);
}

/* ---- entityDef / mapDef ---- */

static void rgd3_decl_entity_def_free(void* body)
{
	rgd3_dict_clear(&((Rgd3DeclEntityDef*)body)->dict);
}

static const char* rgd3_decl_entity_def_default(const void* body)
{
	(void)body;
	return "{\n\t\"DEFAULTED\"\t\"1\"\n}";
}

static int rgd3_decl_entity_def_parse(void* body, const char* text, int length, int allow_binary)
{
	Rgd3DeclEntityDef* def = (Rgd3DeclEntityDef*)body;
	Rgd3Lexer src;
	Rgd3Token token = { 0 };
	Rgd3Token token2 = { 0 };
	const Rgd3DeclEntityDef* inherit[64];
	int num_inherit = 0, i, ok = 1;
	(void)allow_binary;

	rgd3_decl_basic_open(&src, body, text, length, RGD3_DECL_LEXER_FLAGS);
	for (;;)
	{
		if (!rgd3_lexer_read_token(&src, &token))
			break;
		if (!rgd3_token_icmp(&token, "}"))
			break;
		if (token.type != TT_STRING)
		{
			rgd3_lexer_warning(&src, "Expected quoted string, but found '%s'", token.data);
			ok = 0;
			break;
		}
		if (!rgd3_lexer_read_token(&src, &token2))
		{
			rgd3_lexer_warning(&src, "Unexpected end of file");
			ok = 0;
			break;
		}
		if (rgd3_dict_find_key(&def->dict, token.data))
			rgd3_lexer_warning(&src, "'%s' already defined", token.data);
		rgd3_dict_set(&def->dict, token.data, token2.data);
	}

	if (ok)
	{
		/* entityDefs always carry their own name as "classname" */
		rgd3_dict_set(&def->dict, "classname", rgd3_decl_name(body));

		/* "inherit" copies another entityDef's values that are not set here.
		   Collect the parents first: copying modifies the dict. Recursion is
		   bounded because a decl is parsed only once at a time. */
		for (;;)
		{
			const Rgd3KeyValue* kv = rgd3_dict_match_prefix(&def->dict, "inherit", NULL);
			const Rgd3DeclEntityDef* copy;
			char key[RGD3_MAX_STRING_CHARS];
			if (!kv)
				break;
			copy = (const Rgd3DeclEntityDef*)rgd3_decl_find(RGD3_DECL_ENTITYDEF, rgd3_kv_value(kv), 0);
			if (!copy)
				rgd3_lexer_warning(&src, "Unknown entityDef '%s' inherited by '%s'", rgd3_kv_value(kv), rgd3_decl_name(body));
			else if (num_inherit < (int)RG_ARRAY_COUNT(inherit))
				inherit[num_inherit++] = copy;
			rgd3_str_copynz(key, rgd3_kv_key(kv), sizeof(key));
			rgd3_dict_delete(&def->dict, key);
		}
		for (i = 0; i < num_inherit; i++)
			rgd3_dict_set_defaults(&def->dict, &inherit[i]->dict);
		rgd3_decl_cache_dict_media(&def->dict);
	}

	rgd3_token_free(&token);
	rgd3_token_free(&token2);
	rgd3_lexer_free(&src);
	if (!ok)
	{
		rgd3_decl_make_default(body);
		return 0;
	}
	return 1;
}

static void rgd3_decl_entity_def_print(const void* body)
{
	rgd3_dict_print(&((const Rgd3DeclEntityDef*)body)->dict);
}

static size_t rgd3_decl_entity_def_size(const void* body)
{
	const Rgd3DeclEntityDef* def = (const Rgd3DeclEntityDef*)body;
	return sizeof(*def) + (size_t)def->dict.alloced * sizeof(Rgd3KeyValue);
}

const Rgd3DeclEntityDef* rgd3_decl_find_entity_def(const char* name, int make_default)
{
	return (const Rgd3DeclEntityDef*)rgd3_decl_find(RGD3_DECL_ENTITYDEF, name, make_default);
}

/* ---- skin ---- */

static void rgd3_decl_skin_free(void* body)
{
	Rgd3DeclSkin* skin = (Rgd3DeclSkin*)body;
	int i;
	if (skin->mappings)
		rgd3_heap_free(skin->mappings);
	skin->mappings = NULL;
	skin->num_mappings = 0;
	for (i = 0; i < skin->num_associated_models; i++)
		rgd3_heap_free(skin->associated_models[i]);
	if (skin->associated_models)
		rgd3_heap_free(skin->associated_models);
	skin->associated_models = NULL;
	skin->num_associated_models = 0;
}

static const char* rgd3_decl_skin_default(const void* body)
{
	(void)body;
	return "{\n\t\"*\"\t\"_default\"\n}";
}

/* a material with the skin's name implies a skin mapping everything to it */
static int rgd3_decl_skin_set_default_text(void* body)
{
	char generated[2048];
	if (!rgd3_decl_find(RGD3_DECL_MATERIAL, rgd3_decl_name(body), 0))
		return 0;
	rg_snprintf(generated, sizeof(generated), "skin %s // IMPLICITLY GENERATED\n{\n_default %s\n}\n",
		rgd3_decl_name(body), rgd3_decl_name(body));
	rgd3_decl_set_text(body, generated);
	return 1;
}

static int rgd3_decl_skin_parse(void* body, const char* text, int length, int allow_binary)
{
	Rgd3DeclSkin* skin = (Rgd3DeclSkin*)body;
	Rgd3Lexer src;
	Rgd3Token token = { 0 };
	Rgd3Token token2 = { 0 };
	int ok = 1;
	(void)allow_binary;

	rgd3_decl_basic_open(&src, body, text, length, RGD3_DECL_LEXER_FLAGS);
	for (;;)
	{
		if (!rgd3_lexer_read_token(&src, &token))
			break;
		if (!rgd3_token_icmp(&token, "}"))
			break;
		if (!rgd3_lexer_read_token(&src, &token2))
		{
			rgd3_lexer_warning(&src, "Unexpected end of file");
			ok = 0;
			break;
		}
		if (!rgd3_token_icmp(&token, "model"))
		{
			skin->associated_models = (char**)rgd3_heap_realloc(skin->associated_models,
				(size_t)(skin->num_associated_models + 1) * sizeof(char*), RGD3_TAG_DECL);
			skin->associated_models[skin->num_associated_models++] = rgd3_heap_strdup(token2.data, RGD3_TAG_DECL);
			continue;
		}
		skin->mappings = (Rgd3SkinMapping*)rgd3_heap_realloc(skin->mappings,
			(size_t)(skin->num_mappings + 1) * sizeof(Rgd3SkinMapping), RGD3_TAG_DECL);
		/* "*" is the wildcard */
		skin->mappings[skin->num_mappings].from = !rgd3_token_icmp(&token, "*") ? NULL : rgd3_decl_find(RGD3_DECL_MATERIAL, token.data, 1);
		skin->mappings[skin->num_mappings].to = rgd3_decl_find(RGD3_DECL_MATERIAL, token2.data, 1);
		skin->num_mappings++;
	}
	rgd3_token_free(&token);
	rgd3_token_free(&token2);
	rgd3_lexer_free(&src);
	if (!ok)
		rgd3_decl_make_default(body);
	return 0; /* the original always returns false */
}

/* ---- email / video / audio / pda ---- */

/* the value of a localized field: the token itself with g_useOldPDAStrings,
   otherwise "#str_<decl>_<kind>_<field>" from the lang table */
static void rgd3_decl_pda_field(char** dst, const Rgd3Token* token, const void* body, const char* kind, const char* field)
{
	char id[RGD3_MAX_STRING_CHARS];
	if (cv_g_useOldPDAStrings.integer_value)
	{
		rgd3_decl_basic_set_str(dst, token->data);
		return;
	}
	rg_snprintf(id, sizeof(id), "#str_%s_%s_%s", rgd3_decl_name(body), kind, field);
	rgd3_decl_basic_set_str(dst, rgd3_lang_get_string(id));
}

static void rgd3_decl_email_free(void* body)
{
	Rgd3DeclEmail* e = (Rgd3DeclEmail*)body;
	rgd3_decl_basic_free_str(&e->text);
	rgd3_decl_basic_free_str(&e->subject);
	rgd3_decl_basic_free_str(&e->date);
	rgd3_decl_basic_free_str(&e->to);
	rgd3_decl_basic_free_str(&e->from);
}

static const char* rgd3_decl_email_default(const void* body)
{
	(void)body;
	return "{\n\t{\n\t\tto\t5Mail recipient\n\t\tsubject\t5Nothing\n\t\tfrom\t5No one\n\t}\n}";
}

static int rgd3_decl_email_parse(void* body, const char* text, int length, int allow_binary)
{
	Rgd3DeclEmail* e = (Rgd3DeclEmail*)body;
	Rgd3Lexer src;
	Rgd3Token token = { 0 };
	int ok = 1;
	(void)allow_binary;

	rgd3_decl_basic_open(&src, body, text, length, RGD3_PDA_LEXER_FLAGS);
	rgd3_decl_basic_set_str(&e->text, "");
	for (;;)
	{
		if (!rgd3_lexer_read_token(&src, &token))
			break;
		if (rgd3_token_is(&token, "}"))
			break;
		if (!rgd3_token_icmp(&token, "subject"))
		{
			rgd3_lexer_read_token(&src, &token);
			rgd3_decl_pda_field(&e->subject, &token, body, "email", "subject");
		}
		else if (!rgd3_token_icmp(&token, "to"))
		{
			rgd3_lexer_read_token(&src, &token);
			rgd3_decl_pda_field(&e->to, &token, body, "email", "to");
		}
		else if (!rgd3_token_icmp(&token, "from"))
		{
			rgd3_lexer_read_token(&src, &token);
			rgd3_decl_pda_field(&e->from, &token, body, "email", "from");
		}
		else if (!rgd3_token_icmp(&token, "date"))
		{
			rgd3_lexer_read_token(&src, &token);
			rgd3_decl_pda_field(&e->date, &token, body, "email", "date");
		}
		else if (!rgd3_token_icmp(&token, "text"))
		{
			Rgd3StrBuf body_text = { 0 };
			rgd3_lexer_read_token(&src, &token);
			if (!rgd3_token_is(&token, "{"))
			{
				rgd3_lexer_warning(&src, "Email decl '%s' had a parse error", rgd3_decl_name(body));
				ok = 0;
				break;
			}
			while (rgd3_lexer_read_token(&src, &token) && !rgd3_token_is(&token, "}"))
				rgd3_strbuf_append(&body_text, token.data);
			if (cv_g_useOldPDAStrings.integer_value)
				rgd3_decl_basic_set_str(&e->text, rgd3_strbuf_cstr(&body_text));
			else
				rgd3_decl_pda_field(&e->text, &token, body, "email", "text");
			rgd3_strbuf_free(&body_text);
		}
	}
	if (ok && rgd3_lexer_had_error(&src))
	{
		rgd3_lexer_warning(&src, "Email decl '%s' had a parse error", rgd3_decl_name(body));
		ok = 0;
	}
	rgd3_token_free(&token);
	rgd3_lexer_free(&src);
	return ok;
}

static void rgd3_decl_video_free(void* body)
{
	Rgd3DeclVideo* v = (Rgd3DeclVideo*)body;
	rgd3_decl_basic_free_str(&v->video_name);
	rgd3_decl_basic_free_str(&v->info);
}

static const char* rgd3_decl_video_default(const void* body)
{
	(void)body;
	return "{\n\t{\n\t\tname\t5Default Video\n\t}\n}";
}

static int rgd3_decl_video_parse(void* body, const char* text, int length, int allow_binary)
{
	Rgd3DeclVideo* v = (Rgd3DeclVideo*)body;
	Rgd3Lexer src;
	Rgd3Token token = { 0 };
	int ok = 1;
	(void)allow_binary;

	rgd3_decl_basic_open(&src, body, text, length, RGD3_PDA_LEXER_FLAGS);
	for (;;)
	{
		if (!rgd3_lexer_read_token(&src, &token))
			break;
		if (rgd3_token_is(&token, "}"))
			break;
		if (!rgd3_token_icmp(&token, "name"))
		{
			rgd3_lexer_read_token(&src, &token);
			rgd3_decl_pda_field(&v->video_name, &token, body, "video", "name");
		}
		else if (!rgd3_token_icmp(&token, "preview"))
		{
			rgd3_lexer_read_token(&src, &token);
			v->preview = rgd3_decl_find(RGD3_DECL_MATERIAL, token.data, 1);
		}
		else if (!rgd3_token_icmp(&token, "video"))
		{
			rgd3_lexer_read_token(&src, &token);
			v->video = rgd3_decl_find(RGD3_DECL_MATERIAL, token.data, 1);
		}
		else if (!rgd3_token_icmp(&token, "info"))
		{
			rgd3_lexer_read_token(&src, &token);
			rgd3_decl_pda_field(&v->info, &token, body, "video", "info");
		}
		else if (!rgd3_token_icmp(&token, "audio"))
		{
			rgd3_lexer_read_token(&src, &token);
			v->audio = rgd3_decl_find(RGD3_DECL_SOUND, token.data, 1);
		}
	}
	if (rgd3_lexer_had_error(&src))
	{
		rgd3_lexer_warning(&src, "Video decl '%s' had a parse error", rgd3_decl_name(body));
		ok = 0;
	}
	rgd3_token_free(&token);
	rgd3_lexer_free(&src);
	return ok;
}

static void rgd3_decl_audio_free(void* body)
{
	Rgd3DeclAudio* a = (Rgd3DeclAudio*)body;
	rgd3_decl_basic_free_str(&a->audio_name);
	rgd3_decl_basic_free_str(&a->info);
}

static const char* rgd3_decl_audio_default(const void* body)
{
	(void)body;
	return "{\n\t{\n\t\tname\t5Default Audio\n\t}\n}";
}

static int rgd3_decl_audio_parse(void* body, const char* text, int length, int allow_binary)
{
	Rgd3DeclAudio* a = (Rgd3DeclAudio*)body;
	Rgd3Lexer src;
	Rgd3Token token = { 0 };
	int ok = 1;
	(void)allow_binary;

	rgd3_decl_basic_open(&src, body, text, length, RGD3_PDA_LEXER_FLAGS);
	for (;;)
	{
		if (!rgd3_lexer_read_token(&src, &token))
			break;
		if (rgd3_token_is(&token, "}"))
			break;
		if (!rgd3_token_icmp(&token, "name"))
		{
			rgd3_lexer_read_token(&src, &token);
			rgd3_decl_pda_field(&a->audio_name, &token, body, "audio", "name");
		}
		else if (!rgd3_token_icmp(&token, "audio"))
		{
			rgd3_lexer_read_token(&src, &token);
			a->audio = rgd3_decl_find(RGD3_DECL_SOUND, token.data, 1);
		}
		else if (!rgd3_token_icmp(&token, "info"))
		{
			rgd3_lexer_read_token(&src, &token);
			rgd3_decl_pda_field(&a->info, &token, body, "audio", "info");
		}
	}
	if (rgd3_lexer_had_error(&src))
	{
		rgd3_lexer_warning(&src, "Audio decl '%s' had a parse error", rgd3_decl_name(body));
		ok = 0;
	}
	rgd3_token_free(&token);
	rgd3_lexer_free(&src);
	return ok;
}

static void rgd3_decl_pda_free(void* body)
{
	Rgd3DeclPDA* p = (Rgd3DeclPDA*)body;
	if (p->videos)
		rgd3_heap_free((void*)p->videos);
	if (p->audios)
		rgd3_heap_free((void*)p->audios);
	if (p->emails)
		rgd3_heap_free((void*)p->emails);
	p->videos = NULL;
	p->audios = NULL;
	p->emails = NULL;
	p->num_videos = p->num_audios = p->num_emails = 0;
	p->original_emails = 0;
	p->original_videos = 0;
	rgd3_decl_basic_free_str(&p->pda_name);
	rgd3_decl_basic_free_str(&p->full_name);
	rgd3_decl_basic_free_str(&p->icon);
	rgd3_decl_basic_free_str(&p->id);
	rgd3_decl_basic_free_str(&p->post);
	rgd3_decl_basic_free_str(&p->title);
	rgd3_decl_basic_free_str(&p->security);
}

static const char* rgd3_decl_pda_default(const void* body)
{
	(void)body;
	return "{\n\tname  \"default pda\"\n}";
}

static void rgd3_decl_pda_append(const void*** list, int* num, const void* item)
{
	*list = (const void**)rgd3_heap_realloc((void*)*list, (size_t)(*num + 1) * sizeof(void*), RGD3_TAG_DECL);
	(*list)[(*num)++] = item;
}

static int rgd3_decl_pda_parse(void* body, const char* text, int length, int allow_binary)
{
	Rgd3DeclPDA* p = (Rgd3DeclPDA*)body;
	Rgd3Lexer src;
	Rgd3Token token = { 0 };
	int ok = 1;
	(void)allow_binary;

	rgd3_decl_basic_open(&src, body, text, length, RGD3_DECL_LEXER_FLAGS);
	for (;;)
	{
		if (!rgd3_lexer_read_token(&src, &token))
			break;
		if (rgd3_token_is(&token, "}"))
			break;
		if (!rgd3_token_icmp(&token, "name"))
		{
			rgd3_lexer_read_token(&src, &token);
			rgd3_decl_pda_field(&p->pda_name, &token, body, "pda", "name");
		}
		else if (!rgd3_token_icmp(&token, "fullname"))
		{
			rgd3_lexer_read_token(&src, &token);
			rgd3_decl_pda_field(&p->full_name, &token, body, "pda", "fullname");
		}
		else if (!rgd3_token_icmp(&token, "icon"))
		{
			rgd3_lexer_read_token(&src, &token);
			rgd3_decl_basic_set_str(&p->icon, token.data);
		}
		else if (!rgd3_token_icmp(&token, "id"))
		{
			rgd3_lexer_read_token(&src, &token);
			rgd3_decl_pda_field(&p->id, &token, body, "pda", "id");
		}
		else if (!rgd3_token_icmp(&token, "post"))
		{
			rgd3_lexer_read_token(&src, &token);
			rgd3_decl_pda_field(&p->post, &token, body, "pda", "post");
		}
		else if (!rgd3_token_icmp(&token, "title"))
		{
			rgd3_lexer_read_token(&src, &token);
			rgd3_decl_pda_field(&p->title, &token, body, "pda", "title");
		}
		else if (!rgd3_token_icmp(&token, "security"))
		{
			rgd3_lexer_read_token(&src, &token);
			rgd3_decl_pda_field(&p->security, &token, body, "pda", "security");
		}
		else if (!rgd3_token_icmp(&token, "pda_email"))
		{
			rgd3_lexer_read_token(&src, &token);
			rgd3_decl_pda_append((const void***)&p->emails, &p->num_emails, rgd3_decl_find(RGD3_DECL_EMAIL, token.data, 1));
		}
		else if (!rgd3_token_icmp(&token, "pda_audio"))
		{
			rgd3_lexer_read_token(&src, &token);
			rgd3_decl_pda_append((const void***)&p->audios, &p->num_audios, rgd3_decl_find(RGD3_DECL_AUDIO, token.data, 1));
		}
		else if (!rgd3_token_icmp(&token, "pda_video"))
		{
			rgd3_lexer_read_token(&src, &token);
			rgd3_decl_pda_append((const void***)&p->videos, &p->num_videos, rgd3_decl_find(RGD3_DECL_VIDEO, token.data, 1));
		}
	}
	if (rgd3_lexer_had_error(&src))
	{
		rgd3_lexer_warning(&src, "PDA decl '%s' had a parse error", rgd3_decl_name(body));
		ok = 0;
	}
	else
	{
		p->original_videos = p->num_videos;
		p->original_emails = p->num_emails;
	}
	rgd3_token_free(&token);
	rgd3_lexer_free(&src);
	return ok;
}

/* ---- registration ---- */

static const Rgd3DeclTypeDesc rgd3_decl_basic_types[] = {
	{ "table", RGD3_DECL_TABLE, sizeof(Rgd3DeclTable), rgd3_decl_table_parse, rgd3_decl_table_free, rgd3_decl_table_default, NULL, NULL, NULL, rgd3_decl_table_size },
	/* material: rgd3_material (M3) replaces this entry */
	{ "material", RGD3_DECL_MATERIAL, sizeof(Rgd3DeclBody), NULL, NULL, NULL, NULL, NULL, NULL, NULL },
	{ "skin", RGD3_DECL_SKIN, sizeof(Rgd3DeclSkin), rgd3_decl_skin_parse, rgd3_decl_skin_free, rgd3_decl_skin_default, rgd3_decl_skin_set_default_text, NULL, NULL, NULL },
	/* sound: rgd3_sound (M8) */
	{ "sound", RGD3_DECL_SOUND, sizeof(Rgd3DeclBody), NULL, NULL, NULL, NULL, NULL, NULL, NULL },
	{ "entityDef", RGD3_DECL_ENTITYDEF, sizeof(Rgd3DeclEntityDef), rgd3_decl_entity_def_parse, rgd3_decl_entity_def_free, rgd3_decl_entity_def_default, NULL, NULL, rgd3_decl_entity_def_print, rgd3_decl_entity_def_size },
	{ "mapDef", RGD3_DECL_MAPDEF, sizeof(Rgd3DeclEntityDef), rgd3_decl_entity_def_parse, rgd3_decl_entity_def_free, rgd3_decl_entity_def_default, NULL, NULL, rgd3_decl_entity_def_print, rgd3_decl_entity_def_size },
	/* fx, particle, articulatedFigure: game and renderer modules (M5-M7) */
	{ "fx", RGD3_DECL_FX, sizeof(Rgd3DeclBody), NULL, NULL, NULL, NULL, NULL, NULL, NULL },
	{ "particle", RGD3_DECL_PARTICLE, sizeof(Rgd3DeclBody), NULL, NULL, NULL, NULL, NULL, NULL, NULL },
	{ "articulatedFigure", RGD3_DECL_AF, sizeof(Rgd3DeclBody), NULL, NULL, NULL, NULL, NULL, NULL, NULL },
	{ "pda", RGD3_DECL_PDA, sizeof(Rgd3DeclPDA), rgd3_decl_pda_parse, rgd3_decl_pda_free, rgd3_decl_pda_default, NULL, NULL, NULL, NULL },
	{ "email", RGD3_DECL_EMAIL, sizeof(Rgd3DeclEmail), rgd3_decl_email_parse, rgd3_decl_email_free, rgd3_decl_email_default, NULL, NULL, NULL, NULL },
	{ "video", RGD3_DECL_VIDEO, sizeof(Rgd3DeclVideo), rgd3_decl_video_parse, rgd3_decl_video_free, rgd3_decl_video_default, NULL, NULL, NULL, NULL },
	{ "audio", RGD3_DECL_AUDIO, sizeof(Rgd3DeclAudio), rgd3_decl_audio_parse, rgd3_decl_audio_free, rgd3_decl_audio_default, NULL, NULL, NULL, NULL },
};

static const Rgd3DeclTypeDesc rgd3_decl_game_types[] = {
	/* model: rgd3_modeldef (M5) */
	{ "model", RGD3_DECL_MODELDEF, sizeof(Rgd3DeclBody), NULL, NULL, NULL, NULL, NULL, NULL, NULL },
	{ "export", RGD3_DECL_MODELEXPORT, sizeof(Rgd3DeclBody), NULL, NULL, NULL, NULL, NULL, NULL, NULL },
};

void rgd3_decl_register_basic_types(void)
{
	int i;
	for (i = 0; i < (int)RG_ARRAY_COUNT(rgd3_decl_basic_types); i++)
		rgd3_decl_register_type(&rgd3_decl_basic_types[i]);
}

void rgd3_decl_register_game_types(void)
{
	int i;
	for (i = 0; i < (int)RG_ARRAY_COUNT(rgd3_decl_game_types); i++)
		rgd3_decl_register_type(&rgd3_decl_game_types[i]);
}

void rgd3_decl_basic_register(void)
{
	rgd3_cvar_register(&cv_g_useOldPDAStrings);
}
