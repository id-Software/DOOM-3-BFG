/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_lang.c - see rgd3_lang.h. Lifted from neo/idlib/LangDict.cpp
   (Load: comments, quoted key, value up to the end of the line with
   unescaped quotes dropped and \n \t \" \\ escapes; FindString_r
   redirection) and neo/framework/Common.cpp (InitLanguageDict,
   FilterLangList). Keys and values are heap strings (RGD3_TAG_STRING). */

static const char* rgd3_lang_names[] = { "english", "french", "italian", "german", "spanish", "japanese", NULL };

static Rgd3Cvar cv_sys_lang = RGD3_CVAR_STRINGS("sys_lang", "english", RGD3_CVAR_SYSTEM | RGD3_CVAR_INIT, "", rgd3_lang_names);
static Rgd3Cvar cv_lang_maskLocalizedStrings = RGD3_CVAR("lang_maskLocalizedStrings", "0", RGD3_CVAR_BOOL,
	"Masks all localized strings to help debugging.  When set will replace strings with an equal length of W's and ending in an X.  Note: The masking occurs at string table load time.");

typedef struct Rgd3LangKeyValue
{
	char* key;
	char* value;
} Rgd3LangKeyValue;

static struct
{
	Rgd3LangKeyValue* kv;
	int num;
	int alloced;
	Rgd3HashIndex index;
} rgd3_lang;

void rgd3_lang_clear(void)
{
	int i;
	for (i = 0; i < rgd3_lang.num; i++)
	{
		rgd3_heap_free(rgd3_lang.kv[i].key);
		if (rgd3_lang.kv[i].value)
			rgd3_heap_free(rgd3_lang.kv[i].value);
	}
	if (rgd3_lang.kv)
		rgd3_heap_free(rgd3_lang.kv);
	rgd3_lang.kv = NULL;
	rgd3_lang.num = 0;
	rgd3_lang.alloced = 0;
	rgd3_hashindex_free(&rgd3_lang.index);
}

int rgd3_lang_find_string_index(const char* str)
{
	int i, hash;
	if (str == NULL)
		return -1;
	hash = rgd3_str_id_ihash(str);
	for (i = rgd3_hashindex_first(&rgd3_lang.index, hash); i >= 0; i = rgd3_hashindex_next(&rgd3_lang.index, i))
	{
		if (rgd3_str_icmp(str, rgd3_lang.kv[i].key) == 0)
			return i;
	}
	return -1;
}

static void rgd3_lang_add(const char* key, const char* value)
{
	int index = rgd3_lang_find_string_index(key);
	if (index >= 0)
	{
		/* SetString: a later file replaces the value */
		if (rgd3_lang.kv[index].value)
			rgd3_heap_free(rgd3_lang.kv[index].value);
		rgd3_lang.kv[index].value = rgd3_heap_strdup(value, RGD3_TAG_STRING);
		return;
	}
	if (rgd3_lang.num == rgd3_lang.alloced)
	{
		rgd3_lang.alloced = rgd3_lang.alloced ? rgd3_lang.alloced * 2 : 4096;
		rgd3_lang.kv = (Rgd3LangKeyValue*)rgd3_heap_realloc(rgd3_lang.kv, (size_t)rgd3_lang.alloced * sizeof(Rgd3LangKeyValue), RGD3_TAG_STRING);
	}
	rgd3_lang.kv[rgd3_lang.num].key = rgd3_heap_strdup(key, RGD3_TAG_STRING);
	rgd3_lang.kv[rgd3_lang.num].value = value ? rgd3_heap_strdup(value, RGD3_TAG_STRING) : NULL;
	rgd3_hashindex_add(&rgd3_lang.index, rgd3_str_id_ihash(key), rgd3_lang.num);
	rgd3_lang.num++;
}

int rgd3_lang_load(const u8* buffer, int length, const char* name)
{
	Rgd3StrBuf key = { 0 };
	Rgd3StrBuf value = { 0 };
	int utf8, line = 0, num_strings = 0, i = 0;

	if (buffer == NULL || length <= 0)
		return 0;

	rgd3_printf("Reading %s", name);
	/* the retail check: a BOM means UTF-8 */
	utf8 = length > 3 && buffer[0] == 0xEF && buffer[1] == 0xBB && buffer[2] == 0xBF;
	rgd3_printf(utf8 ? " as UTF-8\n" : " as ASCII\n");

	while (i < length)
	{
		u32 c = buffer[i++];
		if (c == '/')
		{
			/* comment to the end of the line */
			while (i < length)
			{
				c = buffer[i++];
				if (c == '\n')
				{
					line++;
					break;
				}
			}
		}
		else if (c == '}')
		{
			break;
		}
		else if (c == '\n')
		{
			line++;
		}
		else if (c == '\"')
		{
			int key_start = i, key_end = -1, val_start = -1, val_end = -1;
			while (i < length)
			{
				c = buffer[i++];
				if (c == '\"')
				{
					key_end = i - 1;
					break;
				}
			}
			if (key_end < key_start)
				rgd3_error_fatal("%s File ended while reading key at line %d", name, line);
			rgd3_strbuf_clear(&key);
			rgd3_strbuf_appendn(&key, (const char*)buffer + key_start, key_end - key_start);

			while (i < length)
			{
				c = buffer[i++];
				if (c == '\"')
				{
					val_start = i;
					break;
				}
			}
			if (val_start < 0)
				rgd3_error_fatal("%s File ended while reading value at line %d", name, line);

			/* everything to the end of the line; unescaped quotes are dropped,
			   the last one marks the end of the value */
			rgd3_strbuf_clear(&value);
			while (i < length)
			{
				c = buffer[i++];
				if (!utf8 && c >= 0x80)
					rgd3_error_fatal("Language file %s is supposed to be plain ASCII, but has byte values > 127!", name);
				if (c == '\"')
				{
					val_end = i - 1;
					continue;
				}
				if (c == '\n')
				{
					line++;
					break;
				}
				if (c == '\r')
					continue;
				if (c == '\\')
				{
					c = i < length ? buffer[i++] : 0;
					if (c == 'n')
						c = '\n';
					else if (c == 't')
						c = '\t';
					else if (c == '\"')
						c = '\"';
					else if (c == '\\')
						c = '\\';
					else
						rgd3_warning("Unknown escape sequence %x at line %d", c, line);
				}
				/* UTF-8 bytes pass through unchanged (decode + re-encode in the original) */
				rgd3_strbuf_appendc(&value, (char)c);
			}
			if (val_end < val_start)
				rgd3_error_fatal("%s File ended while reading value at line %d", name, line);
			if (cv_lang_maskLocalizedStrings.integer_value && value.len > 0 && !strstr(rgd3_strbuf_cstr(&key), "#font_"))
			{
				int n;
				for (n = 0; n < value.len - 1; n++)
					value.data[n] = 'W';
				value.data[value.len - 1] = 'X';
			}
			rgd3_lang_add(rgd3_strbuf_cstr(&key), rgd3_strbuf_cstr(&value));
			num_strings++;
		}
	}
	rgd3_printf("%i strings read\n", num_strings);
	rgd3_strbuf_free(&key);
	rgd3_strbuf_free(&value);
	return 1;
}

int rgd3_lang_is_string_id(const char* str)
{
	return rgd3_str_icmpn(str, RGD3_LANG_KEY_PREFIX, sizeof(RGD3_LANG_KEY_PREFIX) - 1) == 0;
}

static const char* rgd3_lang_find_string_r(const char* str, int* depth)
{
	int index;
	const char* value;

	(*depth)++;
	if (*depth > RGD3_LANG_MAX_REDIRECTION_DEPTH)
	{
		rgd3_warning("String '%s', indirection depth > %d", str, RGD3_LANG_MAX_REDIRECTION_DEPTH);
		return NULL;
	}
	if (str == NULL || str[0] == '\0')
		return NULL;
	index = rgd3_lang_find_string_index(str);
	if (index < 0)
		return NULL;
	value = rgd3_lang.kv[index].value;
	if (value == NULL)
		return NULL;
	if (rgd3_lang_is_string_id(value))
		return rgd3_lang_find_string_r(value, depth);
	return value;
}

const char* rgd3_lang_find_string(const char* str)
{
	int depth = 0;
	return rgd3_lang_find_string_r(str, &depth);
}

const char* rgd3_lang_get_string(const char* str)
{
	const char* localized = rgd3_lang_find_string(str);
	return localized ? localized : str;
}

int rgd3_lang_num_strings(void)
{
	return rgd3_lang.num;
}

/* keep the files whose name, after "strings/", starts with the language */
static int rgd3_lang_matches(const char* path, const char* lang)
{
	const char* name = path;
	if (rgd3_str_icmpn(name, "strings/", 8) == 0)
		name += 8;
	return rgd3_str_icmpn(name, lang, strlen(lang)) == 0;
}

void rgd3_lang_init(void)
{
	Rgd3FileList* files = rgd3_fs_list_files("strings", ".lang", 1, 1);
	int i, matched = 0;

	if (files)
	{
		for (i = 0; i < files->count; i++)
			matched += rgd3_lang_matches(files->names[i], cv_sys_lang.value);
		if (!matched)
		{
			/* reset to english and try again */
			rgd3_cvar_set(&cv_sys_lang, "english", 1);
		}
	}
	rgd3_lang_clear();
	for (i = 0; files && i < files->count; i++)
	{
		void* buffer;
		int len;
		if (!rgd3_lang_matches(files->names[i], cv_sys_lang.value))
			continue;
		len = rgd3_fs_read_file(files->names[i], &buffer, NULL);
		if (len <= 0)
		{
			rgd3_warning("couldn't read the language dict file %s", files->names[i]);
			break;
		}
		rgd3_lang_load((const u8*)buffer, len, files->names[i]);
		rgd3_fs_free_file(buffer);
	}
	if (files)
		rgd3_fs_free_file_list(files);
}

void rgd3_lang_shutdown(void)
{
	rgd3_lang_clear();
}

static void rgd3_lang_cmd_reload(const Rgd3CmdArgs* args)
{
	(void)args;
	rgd3_lang_init();
}

/* lang <#str_key>: prints the localized string (rgd3 addition) */
static void rgd3_lang_cmd_lang(const Rgd3CmdArgs* args)
{
	char match[RGD3_MAX_COMMAND_STRING];
	const char* key = rgd3_cmdargs_match(args, 1, match, sizeof(match)); /* "#" "str_x" -> "#str_x" */
	const char* value;
	if (rgd3_cmdargs_argc(args) < 2)
	{
		rgd3_printf("usage: lang <#str_key>  (%d strings loaded, sys_lang %s)\n", rgd3_lang.num, cv_sys_lang.value);
		return;
	}
	value = rgd3_lang_find_string(key);
	if (value)
		rgd3_printf("%s = \"%s\"\n", key, value);
	else
		rgd3_printf("%s not found\n", key);
}

void rgd3_lang_register(void)
{
	rgd3_cvar_register(&cv_sys_lang);
	rgd3_cvar_register(&cv_lang_maskLocalizedStrings);
	rgd3_cmd_add("reloadLanguage", rgd3_lang_cmd_reload, RGD3_CMD_FL_SYSTEM, "reload language dict", NULL);
	rgd3_cmd_add("lang", rgd3_lang_cmd_lang, RGD3_CMD_FL_SYSTEM, "prints a localized string: lang <#str_key>", NULL);
}
