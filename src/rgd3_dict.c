/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_dict.c - see rgd3_dict.h. Lifted from neo/idlib/containers/StrPool.h
   (AllocString/FreeString/CopyString) and neo/idlib/Dict.cpp (Set, Copy,
   SetDefaults, Delete, MatchPrefix, Checksum, Get* parsing, file handle
   format). */

static Rgd3StrPool rgd3_dict_keys;
static Rgd3StrPool rgd3_dict_values;

/* ---- string pool ---- */

void rgd3_strpool_set_case_sensitive(Rgd3StrPool* pool, int case_sensitive)
{
	pool->case_sensitive = case_sensitive;
}

static int rgd3_strpool_cmp(const Rgd3StrPool* pool, const char* a, const char* b)
{
	return pool->case_sensitive ? strcmp(a, b) : rgd3_str_icmp(a, b);
}

const Rgd3PoolStr* rgd3_strpool_alloc(Rgd3StrPool* pool, const char* text)
{
	int i, key, len;
	Rgd3PoolStr* ps;

	key = rgd3_hashindex_key_string(&pool->hash, text, pool->case_sensitive);
	for (i = rgd3_hashindex_first(&pool->hash, key); i != -1; i = rgd3_hashindex_next(&pool->hash, i))
	{
		if (rgd3_strpool_cmp(pool, pool->strings[i]->str, text) == 0)
		{
			pool->strings[i]->num_users++;
			return pool->strings[i];
		}
	}
	len = (int)strlen(text);
	ps = (Rgd3PoolStr*)rgd3_heap_alloc(offsetof(Rgd3PoolStr, str) + (size_t)len + 1, RGD3_TAG_DICT);
	ps->pool = pool;
	ps->num_users = 1;
	ps->len = len;
	memcpy(ps->str, text, (size_t)len + 1);
	if (pool->num == pool->alloced)
	{
		pool->alloced = pool->alloced ? pool->alloced * 2 : 1024;
		pool->strings = (Rgd3PoolStr**)rgd3_heap_realloc(pool->strings, (size_t)pool->alloced * sizeof(Rgd3PoolStr*), RGD3_TAG_DICT);
	}
	pool->strings[pool->num] = ps;
	rgd3_hashindex_add(&pool->hash, key, pool->num);
	pool->num++;
	return ps;
}

/* The pool's order carries no meaning, so a freed slot is filled by the
   last string (the original shifts every index down, which is O(n)). */
void rgd3_strpool_release(Rgd3StrPool* pool, const Rgd3PoolStr* str)
{
	Rgd3PoolStr* ps = (Rgd3PoolStr*)str;
	int i, key, last;

	RG_ASSERT(ps->num_users >= 1);
	RG_ASSERT(ps->pool == pool);
	if (--ps->num_users > 0)
		return;
	key = rgd3_hashindex_key_string(&pool->hash, ps->str, pool->case_sensitive);
	for (i = rgd3_hashindex_first(&pool->hash, key); i != -1; i = rgd3_hashindex_next(&pool->hash, i))
	{
		if (pool->strings[i] == ps)
			break;
	}
	RG_ASSERT(i != -1);
	if (i == -1)
		return;
	rgd3_hashindex_remove(&pool->hash, key, i);
	last = pool->num - 1;
	if (i != last)
	{
		Rgd3PoolStr* moved = pool->strings[last];
		int moved_key = rgd3_hashindex_key_string(&pool->hash, moved->str, pool->case_sensitive);
		rgd3_hashindex_remove(&pool->hash, moved_key, last);
		pool->strings[i] = moved;
		rgd3_hashindex_add(&pool->hash, moved_key, i);
	}
	pool->num--;
	rgd3_heap_free(ps);
}

const Rgd3PoolStr* rgd3_strpool_copy(Rgd3StrPool* pool, const Rgd3PoolStr* str)
{
	RG_ASSERT(str->num_users >= 1);
	if (str->pool == pool)
	{
		((Rgd3PoolStr*)str)->num_users++;
		return str;
	}
	return rgd3_strpool_alloc(pool, str->str);
}

void rgd3_strpool_clear(Rgd3StrPool* pool)
{
	int i;
	for (i = 0; i < pool->num; i++)
		rgd3_heap_free(pool->strings[i]);
	if (pool->strings)
		rgd3_heap_free(pool->strings);
	pool->strings = NULL;
	pool->num = 0;
	pool->alloced = 0;
	rgd3_hashindex_free(&pool->hash);
}

size_t rgd3_strpool_size(const Rgd3StrPool* pool)
{
	size_t size = sizeof(*pool) + (size_t)pool->alloced * sizeof(Rgd3PoolStr*) + rgd3_hashindex_allocated(&pool->hash);
	int i;
	for (i = 0; i < pool->num; i++)
		size += offsetof(Rgd3PoolStr, str) + (size_t)pool->strings[i]->len + 1;
	return size;
}

/* ---- dict ---- */

/* the original constructor: 128 buckets, 16 indexes, granularity 16 */
static void rgd3_dict_prepare(Rgd3Dict* d)
{
	if (!d->hash.hash_size)
	{
		rgd3_hashindex_clear_resize(&d->hash, 128, 16);
		rgd3_hashindex_set_granularity(&d->hash, 16);
	}
}

static int rgd3_dict_hash_key(const Rgd3Dict* d, const char* key)
{
	int mask = d->hash.hash_size ? d->hash.hash_mask : 127;
	return rgd3_str_id_ihash(key) & mask;
}

static int rgd3_dict_append(Rgd3Dict* d, const Rgd3PoolStr* key, const Rgd3PoolStr* value)
{
	if (d->num == d->alloced)
	{
		d->alloced += 16;
		d->args = (Rgd3KeyValue*)rgd3_heap_realloc(d->args, (size_t)d->alloced * sizeof(Rgd3KeyValue), RGD3_TAG_DICT);
	}
	d->args[d->num].key = key;
	d->args[d->num].value = value;
	return d->num++;
}

void rgd3_dict_clear(Rgd3Dict* d)
{
	int i;
	for (i = 0; i < d->num; i++)
	{
		rgd3_strpool_release(&rgd3_dict_keys, d->args[i].key);
		rgd3_strpool_release(&rgd3_dict_values, d->args[i].value);
	}
	if (d->args)
		rgd3_heap_free(d->args);
	d->args = NULL;
	d->num = 0;
	d->alloced = 0;
	rgd3_hashindex_free(&d->hash);
}

void rgd3_dict_assign(Rgd3Dict* d, const Rgd3Dict* other)
{
	int i;
	if (d == other)
		return;
	rgd3_dict_clear(d);
	if (!other->num)
	{
		d->hash.hash_size = 0;
		return;
	}
	d->args = (Rgd3KeyValue*)rgd3_heap_alloc((size_t)other->alloced * sizeof(Rgd3KeyValue), RGD3_TAG_DICT);
	d->alloced = other->alloced;
	d->num = other->num;
	for (i = 0; i < other->num; i++)
	{
		d->args[i].key = rgd3_strpool_copy(&rgd3_dict_keys, other->args[i].key);
		d->args[i].value = rgd3_strpool_copy(&rgd3_dict_values, other->args[i].value);
	}
	rgd3_hashindex_assign(&d->hash, &other->hash);
}

void rgd3_dict_copy(Rgd3Dict* d, const Rgd3Dict* other)
{
	int i, n;
	int* found = NULL;

	if (d == other)
		return;
	rgd3_dict_prepare(d);
	n = other->num;
	if (d->num && n)
	{
		found = (int*)rgd3_heap_alloc((size_t)n * sizeof(int), RGD3_TAG_TEMP);
		for (i = 0; i < n; i++)
			found[i] = rgd3_dict_find_key_index(d, rgd3_kv_key(&other->args[i]));
	}
	for (i = 0; i < n; i++)
	{
		if (found && found[i] != -1)
		{
			/* set the new value first, then free the old one, to allow self copies */
			const Rgd3PoolStr* old_value = d->args[found[i]].value;
			d->args[found[i]].value = rgd3_strpool_copy(&rgd3_dict_values, other->args[i].value);
			rgd3_strpool_release(&rgd3_dict_values, old_value);
		}
		else
		{
			const Rgd3PoolStr* key = rgd3_strpool_copy(&rgd3_dict_keys, other->args[i].key);
			const Rgd3PoolStr* value = rgd3_strpool_copy(&rgd3_dict_values, other->args[i].value);
			rgd3_hashindex_add(&d->hash, rgd3_dict_hash_key(d, key->str), rgd3_dict_append(d, key, value));
		}
	}
	if (found)
		rgd3_heap_free(found);
}

void rgd3_dict_transfer(Rgd3Dict* d, Rgd3Dict* other)
{
	if (d == other)
		return;
	rgd3_dict_clear(d);
	*d = *other;
	memset(other, 0, sizeof(*other));
}

void rgd3_dict_set_defaults(Rgd3Dict* d, const Rgd3Dict* defaults)
{
	int i;
	rgd3_dict_prepare(d);
	for (i = 0; i < defaults->num; i++)
	{
		const Rgd3KeyValue* def = &defaults->args[i];
		if (!rgd3_dict_find_key(d, rgd3_kv_key(def)))
		{
			const Rgd3PoolStr* key = rgd3_strpool_copy(&rgd3_dict_keys, def->key);
			const Rgd3PoolStr* value = rgd3_strpool_copy(&rgd3_dict_values, def->value);
			rgd3_hashindex_add(&d->hash, rgd3_dict_hash_key(d, key->str), rgd3_dict_append(d, key, value));
		}
	}
}

void rgd3_dict_print(const Rgd3Dict* d)
{
	int i;
	for (i = 0; i < d->num; i++)
		rgd3_printf("%s = %s\n", rgd3_kv_key(&d->args[i]), rgd3_kv_value(&d->args[i]));
}

static int rgd3_dict_sort_kv(const void* a, const void* b)
{
	return rgd3_str_icmp(rgd3_kv_key((const Rgd3KeyValue*)a), rgd3_kv_key((const Rgd3KeyValue*)b));
}

/* CRC32 over key then value of the pairs sorted by key */
int rgd3_dict_checksum(const Rgd3Dict* d)
{
	Rgd3KeyValue* sorted;
	u32 crc;
	int i;

	rgd3_crc32_init(&crc);
	if (d->num)
	{
		sorted = (Rgd3KeyValue*)rgd3_heap_alloc((size_t)d->num * sizeof(Rgd3KeyValue), RGD3_TAG_TEMP);
		memcpy(sorted, d->args, (size_t)d->num * sizeof(Rgd3KeyValue));
		qsort(sorted, (size_t)d->num, sizeof(Rgd3KeyValue), rgd3_dict_sort_kv);
		for (i = 0; i < d->num; i++)
		{
			rgd3_crc32_update(&crc, sorted[i].key->str, sorted[i].key->len);
			rgd3_crc32_update(&crc, sorted[i].value->str, sorted[i].value->len);
		}
		rgd3_heap_free(sorted);
	}
	rgd3_crc32_finish(&crc);
	return (int)crc;
}

void rgd3_dict_set(Rgd3Dict* d, const char* key, const char* value)
{
	int i;
	if (key == NULL || key[0] == '\0')
		return;
	if (!value)
		value = "";
	rgd3_dict_prepare(d);
	i = rgd3_dict_find_key_index(d, key);
	if (i != -1)
	{
		/* set the new value first, then free the old one, to allow self copies */
		const Rgd3PoolStr* old_value = d->args[i].value;
		d->args[i].value = rgd3_strpool_alloc(&rgd3_dict_values, value);
		rgd3_strpool_release(&rgd3_dict_values, old_value);
	}
	else
	{
		const Rgd3PoolStr* k = rgd3_strpool_alloc(&rgd3_dict_keys, key);
		const Rgd3PoolStr* v = rgd3_strpool_alloc(&rgd3_dict_values, value);
		rgd3_hashindex_add(&d->hash, rgd3_dict_hash_key(d, k->str), rgd3_dict_append(d, k, v));
	}
}

void rgd3_dict_set_float(Rgd3Dict* d, const char* key, float value)
{
	char buf[64];
	rg_snprintf(buf, sizeof(buf), "%f", value);
	rgd3_dict_set(d, key, buf);
}

void rgd3_dict_set_int(Rgd3Dict* d, const char* key, int value)
{
	char buf[32];
	rg_snprintf(buf, sizeof(buf), "%i", value);
	rgd3_dict_set(d, key, buf);
}

void rgd3_dict_set_bool(Rgd3Dict* d, const char* key, int value)
{
	rgd3_dict_set(d, key, value ? "1" : "0");
}

void rgd3_dict_set_floats(Rgd3Dict* d, const char* key, const float* values, int count)
{
	rgd3_dict_set(d, key, rgd3_str_float_array_to_string(values, count, 2));
}

const Rgd3KeyValue* rgd3_dict_find_key(const Rgd3Dict* d, const char* key)
{
	int i, hash;
	if (key == NULL || key[0] == '\0')
	{
		rgd3_dwarning("idDict::FindKey: empty key");
		return NULL;
	}
	hash = rgd3_dict_hash_key(d, key);
	for (i = rgd3_hashindex_first(&d->hash, hash); i != -1; i = rgd3_hashindex_next(&d->hash, i))
	{
		if (rgd3_str_icmp(rgd3_kv_key(&d->args[i]), key) == 0)
			return &d->args[i];
	}
	return NULL;
}

/* returns 0 (not -1) for an empty key, as the original does */
int rgd3_dict_find_key_index(const Rgd3Dict* d, const char* key)
{
	int i, hash;
	if (key == NULL || key[0] == '\0')
	{
		rgd3_dwarning("idDict::FindKeyIndex: empty key");
		return 0;
	}
	hash = rgd3_dict_hash_key(d, key);
	for (i = rgd3_hashindex_first(&d->hash, hash); i != -1; i = rgd3_hashindex_next(&d->hash, i))
	{
		if (rgd3_str_icmp(rgd3_kv_key(&d->args[i]), key) == 0)
			return i;
	}
	return -1;
}

const char* rgd3_dict_get_string(const Rgd3Dict* d, const char* key, const char* def)
{
	const Rgd3KeyValue* kv = rgd3_dict_find_key(d, key);
	return kv ? rgd3_kv_value(kv) : def;
}

int rgd3_dict_get_string_out(const Rgd3Dict* d, const char* key, const char* def, const char** out)
{
	const Rgd3KeyValue* kv = rgd3_dict_find_key(d, key);
	*out = kv ? rgd3_kv_value(kv) : def;
	return kv != NULL;
}

float rgd3_dict_get_float(const Rgd3Dict* d, const char* key, float def)
{
	const Rgd3KeyValue* kv = rgd3_dict_find_key(d, key);
	return kv ? (float)atof(rgd3_kv_value(kv)) : def;
}

float rgd3_dict_get_float_s(const Rgd3Dict* d, const char* key, const char* def)
{
	return (float)atof(rgd3_dict_get_string(d, key, def));
}

int rgd3_dict_get_float_out(const Rgd3Dict* d, const char* key, const char* def, float* out)
{
	const char* s;
	int found = rgd3_dict_get_string_out(d, key, def, &s);
	*out = (float)atof(s);
	return found;
}

int rgd3_dict_get_int(const Rgd3Dict* d, const char* key, int def)
{
	const Rgd3KeyValue* kv = rgd3_dict_find_key(d, key);
	return kv ? atoi(rgd3_kv_value(kv)) : def;
}

int rgd3_dict_get_int_s(const Rgd3Dict* d, const char* key, const char* def)
{
	return atoi(rgd3_dict_get_string(d, key, def));
}

int rgd3_dict_get_int_out(const Rgd3Dict* d, const char* key, const char* def, int* out)
{
	const char* s;
	int found = rgd3_dict_get_string_out(d, key, def, &s);
	*out = atoi(s);
	return found;
}

int rgd3_dict_get_bool(const Rgd3Dict* d, const char* key, int def)
{
	const Rgd3KeyValue* kv = rgd3_dict_find_key(d, key);
	return kv ? atoi(rgd3_kv_value(kv)) != 0 : def;
}

int rgd3_dict_get_bool_s(const Rgd3Dict* d, const char* key, const char* def)
{
	return atoi(rgd3_dict_get_string(d, key, def)) != 0;
}

int rgd3_dict_get_bool_out(const Rgd3Dict* d, const char* key, const char* def, int* out)
{
	const char* s;
	int found = rgd3_dict_get_string_out(d, key, def, &s);
	*out = atoi(s) != 0;
	return found;
}

int rgd3_dict_get_floats(const Rgd3Dict* d, const char* key, const char* def, float* out, int count, int identity)
{
	const char* s;
	int found, i;

	RG_ASSERT(count >= 1 && count <= 9);
	found = rgd3_dict_get_string_out(d, key, def, &s);
	for (i = 0; i < count; i++)
		out[i] = 0.0f;
	if (identity)
	{
		RG_ASSERT(count == 9);
		out[0] = out[4] = out[8] = 1.0f;
	}
	if (!s)
		return found; /* default NULL: zeros, or identity for GetMatrix */
	switch (count)
	{
	case 1: sscanf(s, "%f", &out[0]); break;
	case 2: sscanf(s, "%f %f", &out[0], &out[1]); break;
	case 3: sscanf(s, "%f %f %f", &out[0], &out[1], &out[2]); break;
	case 4: sscanf(s, "%f %f %f %f", &out[0], &out[1], &out[2], &out[3]); break;
	default:
		sscanf(s, "%f %f %f %f %f %f %f %f %f", &out[0], &out[1], &out[2], &out[3], &out[4], &out[5], &out[6], &out[7], &out[8]);
		break;
	}
	return found;
}

int rgd3_dict_num(const Rgd3Dict* d)
{
	return d->num;
}

const Rgd3KeyValue* rgd3_dict_get_kv(const Rgd3Dict* d, int index)
{
	return (index >= 0 && index < d->num) ? &d->args[index] : NULL;
}

void rgd3_dict_delete(Rgd3Dict* d, const char* key)
{
	int hash, i;
	hash = rgd3_dict_hash_key(d, key);
	for (i = rgd3_hashindex_first(&d->hash, hash); i != -1; i = rgd3_hashindex_next(&d->hash, i))
	{
		if (rgd3_str_icmp(rgd3_kv_key(&d->args[i]), key) == 0)
		{
			rgd3_strpool_release(&rgd3_dict_keys, d->args[i].key);
			rgd3_strpool_release(&rgd3_dict_values, d->args[i].value);
			memmove(&d->args[i], &d->args[i + 1], (size_t)(d->num - i - 1) * sizeof(Rgd3KeyValue));
			d->num--;
			rgd3_hashindex_remove_index(&d->hash, hash, i);
			break;
		}
	}
}

/* Next pair whose key starts with prefix (case-insensitive), after
   last_match when given. */
const Rgd3KeyValue* rgd3_dict_match_prefix(const Rgd3Dict* d, const char* prefix, const Rgd3KeyValue* last_match)
{
	int i, len, start = -1;

	RG_ASSERT(prefix);
	len = (int)strlen(prefix);
	if (last_match)
	{
		start = (int)(last_match - d->args);
		RG_ASSERT(start >= 0 && start < d->num);
		if (start < 1)
			start = 0;
	}
	for (i = start + 1; i < d->num; i++)
	{
		if (!rgd3_str_icmpn(rgd3_kv_key(&d->args[i]), prefix, (size_t)len))
			return &d->args[i];
	}
	return NULL;
}

const char* rgd3_dict_random_prefix(const Rgd3Dict* d, const char* prefix, Rgd3Random* random)
{
	enum { MAX_RANDOM_KEYS = 2048 };
	const char* list[MAX_RANDOM_KEYS];
	const Rgd3KeyValue* kv;
	int count = 0;

	list[0] = "";
	for (kv = rgd3_dict_match_prefix(d, prefix, NULL); kv && count < MAX_RANDOM_KEYS; kv = rgd3_dict_match_prefix(d, prefix, kv))
		list[count++] = rgd3_kv_value(kv);
	return list[rgd3_random_int_max(random, count)];
}

int rgd3_dict_parse(Rgd3Dict* d, Rgd3Parser* parser)
{
	Rgd3Token token = { 0 };
	Rgd3Token token2 = { 0 };
	int errors = 0; /* duplicate keys: keep parsing, report failure */
	int syntax = 0; /* malformed: stop */

	/* With fatal parser flags the errors below drop, as in the original.
	   With non-fatal flags the original would spin on the stale token at end
	   of file; here a syntax error stops the parse and returns 0. */
	if (!rgd3_parser_expect_token_string(parser, "{") || !rgd3_parser_read_token(parser, &token))
	{
		rgd3_parser_error(parser, "Unexpected end of file");
		syntax = 1;
	}
	while (!syntax && (token.type != TT_PUNCTUATION || !rgd3_token_is(&token, "}")))
	{
		if (token.type != TT_STRING)
		{
			rgd3_parser_error(parser, "Expected quoted string, but found '%s'", rgd3_token_cstr(&token));
			syntax = 1;
			break;
		}
		if (!rgd3_parser_read_token(parser, &token2))
		{
			rgd3_parser_error(parser, "Unexpected end of file");
			syntax = 1;
			break;
		}
		if (rgd3_dict_find_key(d, rgd3_token_cstr(&token)))
		{
			rgd3_parser_warning(parser, "'%s' already defined", rgd3_token_cstr(&token));
			errors = 1;
		}
		rgd3_dict_set(d, rgd3_token_cstr(&token), rgd3_token_cstr(&token2));
		if (!rgd3_parser_read_token(parser, &token))
		{
			rgd3_parser_error(parser, "Unexpected end of file");
			syntax = 1;
		}
	}
	rgd3_token_free(&token);
	rgd3_token_free(&token2);
	return !errors && !syntax;
}

/* idDict::WriteToFileHandle: LE count, then NUL-terminated key, value */
void rgd3_dict_write_to_file(const Rgd3Dict* d, Rgd3File* f)
{
	u8 c[4];
	int i, j;
	c[0] = (u8)(d->num & 0xff);
	c[1] = (u8)((d->num >> 8) & 0xff);
	c[2] = (u8)((d->num >> 16) & 0xff);
	c[3] = (u8)((d->num >> 24) & 0xff);
	rgd3_fs_write(f, c, 4);
	for (i = 0; i < d->num; i++)
	{
		const Rgd3PoolStr* s[2] = { d->args[i].key, d->args[i].value };
		for (j = 0; j < 2; j++)
		{
			if (s[j]->len >= RGD3_MAX_STRING_CHARS - 1)
				rgd3_error_drop("idDict::WriteToFileHandle: bad string");
			rgd3_fs_write(f, s[j]->str, (size_t)s[j]->len + 1);
		}
	}
}

static void rgd3_dict_read_string(Rgd3File* f, char* str)
{
	int len;
	for (len = 0; len < RGD3_MAX_STRING_CHARS; len++)
	{
		if (rgd3_fs_read(f, &str[len], 1) != 1)
			str[len] = 0;
		if (str[len] == 0)
			break;
	}
	if (len == RGD3_MAX_STRING_CHARS)
		rgd3_error_drop("idDict::ReadFromFileHandle: bad string");
}

void rgd3_dict_read_from_file(Rgd3Dict* d, Rgd3File* f)
{
	char key[RGD3_MAX_STRING_CHARS];
	char value[RGD3_MAX_STRING_CHARS];
	u8 c[4] = { 0 };
	int i, count;

	rgd3_dict_clear(d);
	rgd3_fs_read(f, c, 4);
	count = (int)((u32)c[0] | ((u32)c[1] << 8) | ((u32)c[2] << 16) | ((u32)c[3] << 24));
	for (i = 0; i < count; i++)
	{
		rgd3_dict_read_string(f, key);
		rgd3_dict_read_string(f, value);
		rgd3_dict_set(d, key, value);
	}
}

/* ---- module ---- */

void rgd3_dict_init(void)
{
	rgd3_strpool_set_case_sensitive(&rgd3_dict_keys, 0);
	rgd3_strpool_set_case_sensitive(&rgd3_dict_values, 1);
}

void rgd3_dict_shutdown(void)
{
	rgd3_strpool_clear(&rgd3_dict_keys);
	rgd3_strpool_clear(&rgd3_dict_values);
}

static void rgd3_dict_cmd_show_memory(const Rgd3CmdArgs* args)
{
	(void)args;
	rgd3_printf("%5d KB in %d keys\n", (int)(rgd3_strpool_size(&rgd3_dict_keys) >> 10), rgd3_dict_keys.num);
	rgd3_printf("%5d KB in %d values\n", (int)(rgd3_strpool_size(&rgd3_dict_values) >> 10), rgd3_dict_values.num);
}

static int rgd3_dict_sort_pool_str(const void* a, const void* b)
{
	return strcmp((*(const Rgd3PoolStr* const*)a)->str, (*(const Rgd3PoolStr* const*)b)->str);
}

/* the original prints "Not implemented"; these list the pools sorted */
static void rgd3_dict_list_pool(const Rgd3StrPool* pool, const char* what)
{
	Rgd3PoolStr** sorted;
	int i;
	if (!pool->num)
	{
		rgd3_printf("%5d %s\n", 0, what);
		return;
	}
	sorted = (Rgd3PoolStr**)rgd3_heap_alloc((size_t)pool->num * sizeof(Rgd3PoolStr*), RGD3_TAG_TEMP);
	memcpy(sorted, pool->strings, (size_t)pool->num * sizeof(Rgd3PoolStr*));
	qsort(sorted, (size_t)pool->num, sizeof(Rgd3PoolStr*), rgd3_dict_sort_pool_str);
	for (i = 0; i < pool->num; i++)
		rgd3_printf("%s\n", sorted[i]->str);
	rgd3_printf("%5d %s\n", pool->num, what);
	rgd3_heap_free(sorted);
}

static void rgd3_dict_cmd_list_keys(const Rgd3CmdArgs* args)
{
	(void)args;
	rgd3_dict_list_pool(&rgd3_dict_keys, "keys");
}

static void rgd3_dict_cmd_list_values(const Rgd3CmdArgs* args)
{
	(void)args;
	rgd3_dict_list_pool(&rgd3_dict_values, "values");
}

void rgd3_dict_register(void)
{
	rgd3_cmd_add("showDictMemory", rgd3_dict_cmd_show_memory, RGD3_CMD_FL_SYSTEM, "shows memory used by dictionaries", NULL);
	rgd3_cmd_add("listDictKeys", rgd3_dict_cmd_list_keys, RGD3_CMD_FL_SYSTEM, "lists all keys used by dictionaries", NULL);
	rgd3_cmd_add("listDictValues", rgd3_dict_cmd_list_values, RGD3_CMD_FL_SYSTEM, "lists all values used by dictionaries", NULL);
}
