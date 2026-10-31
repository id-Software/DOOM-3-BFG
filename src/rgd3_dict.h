/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_dict.h - interned string pools and the key/value dictionary
   (idStrPool / idDict, neo/idlib/containers/StrPool.h, neo/idlib/Dict.h/.cpp).

   Keys are compared case-insensitively. Keys and values live in two global
   reference-counted pools (keys case-insensitive, values case-sensitive),
   so equal strings are stored once across every dict. Insertion order is
   kept (MatchPrefix and spawn code depend on it).

   A zero-initialised Rgd3Dict is empty and valid; rgd3_dict_clear() frees
   it. Assign with rgd3_dict_assign(), never by struct copy. Main thread
   only. */
#ifndef RGD3_DICT_H
#define RGD3_DICT_H
#ifndef RGD3_ALL_C
#error "rgd3 headers are included through src/all.c only"
#endif

typedef struct Rgd3StrPool Rgd3StrPool;

typedef struct Rgd3PoolStr
{
	Rgd3StrPool* pool;
	int num_users;
	int len;
	char str[1]; /* allocated to len + 1 */
} Rgd3PoolStr;

struct Rgd3StrPool
{
	int case_sensitive;
	Rgd3PoolStr** strings;
	int num;
	int alloced;
	Rgd3HashIndex hash;
};

void rgd3_strpool_set_case_sensitive(Rgd3StrPool* pool, int case_sensitive);
const Rgd3PoolStr* rgd3_strpool_alloc(Rgd3StrPool* pool, const char* text);
void rgd3_strpool_release(Rgd3StrPool* pool, const Rgd3PoolStr* str);
const Rgd3PoolStr* rgd3_strpool_copy(Rgd3StrPool* pool, const Rgd3PoolStr* str);
void rgd3_strpool_clear(Rgd3StrPool* pool);
size_t rgd3_strpool_size(const Rgd3StrPool* pool);

typedef struct Rgd3KeyValue
{
	const Rgd3PoolStr* key;
	const Rgd3PoolStr* value;
} Rgd3KeyValue;

RGINLINE const char* rgd3_kv_key(const Rgd3KeyValue* kv)
{
	return kv->key->str;
}

RGINLINE const char* rgd3_kv_value(const Rgd3KeyValue* kv)
{
	return kv->value->str;
}

typedef struct Rgd3Dict
{
	Rgd3KeyValue* args;
	int num;
	int alloced;
	Rgd3HashIndex hash;
} Rgd3Dict;

void rgd3_dict_init(void);
void rgd3_dict_shutdown(void);
void rgd3_dict_register(void);

void rgd3_dict_clear(Rgd3Dict* d);
void rgd3_dict_assign(Rgd3Dict* d, const Rgd3Dict* other);       /* operator= */
void rgd3_dict_copy(Rgd3Dict* d, const Rgd3Dict* other);         /* merge, other wins */
void rgd3_dict_transfer(Rgd3Dict* d, Rgd3Dict* other);           /* move, other ends empty */
void rgd3_dict_set_defaults(Rgd3Dict* d, const Rgd3Dict* defaults); /* add keys d lacks */
void rgd3_dict_print(const Rgd3Dict* d);
int rgd3_dict_checksum(const Rgd3Dict* d);

void rgd3_dict_set(Rgd3Dict* d, const char* key, const char* value);
void rgd3_dict_set_float(Rgd3Dict* d, const char* key, float value); /* "%f" */
void rgd3_dict_set_int(Rgd3Dict* d, const char* key, int value);
void rgd3_dict_set_bool(Rgd3Dict* d, const char* key, int value);
void rgd3_dict_set_floats(Rgd3Dict* d, const char* key, const float* values, int count); /* ToString(2) */

/* Getters: the plain forms take a typed default (0 / 0.0 / false / "");
   the _s forms take a default string that is parsed like a value; the
   _out forms return whether the key exists. */
const char* rgd3_dict_get_string(const Rgd3Dict* d, const char* key, const char* def);
int rgd3_dict_get_string_out(const Rgd3Dict* d, const char* key, const char* def, const char** out);
float rgd3_dict_get_float(const Rgd3Dict* d, const char* key, float def);
float rgd3_dict_get_float_s(const Rgd3Dict* d, const char* key, const char* def);
int rgd3_dict_get_float_out(const Rgd3Dict* d, const char* key, const char* def, float* out);
int rgd3_dict_get_int(const Rgd3Dict* d, const char* key, int def);
int rgd3_dict_get_int_s(const Rgd3Dict* d, const char* key, const char* def);
int rgd3_dict_get_int_out(const Rgd3Dict* d, const char* key, const char* def, int* out);
int rgd3_dict_get_bool(const Rgd3Dict* d, const char* key, int def);
int rgd3_dict_get_bool_s(const Rgd3Dict* d, const char* key, const char* def);
int rgd3_dict_get_bool_out(const Rgd3Dict* d, const char* key, const char* def, int* out);
/* count floats parsed with sscanf("%f ..."); out is zeroed first, or set to
   identity when identity is nonzero (GetMatrix); def NULL = all zeros */
int rgd3_dict_get_floats(const Rgd3Dict* d, const char* key, const char* def, float* out, int count, int identity);

/* GetVector / GetVec2 / GetVec4 / GetAngles / GetMatrix (default NULL =
   zeros, identity for the matrix); return whether the key exists */
RGINLINE int rgd3_dict_get_vector(const Rgd3Dict* d, const char* key, const char* def, Rgd3Vec3* out)
{
	return rgd3_dict_get_floats(d, key, def, out->v, 3, 0);
}

RGINLINE int rgd3_dict_get_vec2(const Rgd3Dict* d, const char* key, const char* def, Rgd3Vec2* out)
{
	return rgd3_dict_get_floats(d, key, def, out->v, 2, 0);
}

RGINLINE int rgd3_dict_get_vec4(const Rgd3Dict* d, const char* key, const char* def, Rgd3Vec4* out)
{
	return rgd3_dict_get_floats(d, key, def, out->v, 4, 0);
}

RGINLINE int rgd3_dict_get_angles(const Rgd3Dict* d, const char* key, const char* def, Rgd3Angles* out)
{
	return rgd3_dict_get_floats(d, key, def, out->v, 3, 0);
}

RGINLINE int rgd3_dict_get_matrix(const Rgd3Dict* d, const char* key, const char* def, Rgd3Mat3* out)
{
	float m[9];
	int found = rgd3_dict_get_floats(d, key, def, m, 9, 1);
	memcpy(out, m, sizeof(m));
	return found;
}

RGINLINE void rgd3_dict_set_vector(Rgd3Dict* d, const char* key, Rgd3Vec3 v)
{
	rgd3_dict_set_floats(d, key, v.v, 3);
}

RGINLINE void rgd3_dict_set_angles(Rgd3Dict* d, const char* key, Rgd3Angles a)
{
	rgd3_dict_set_floats(d, key, a.v, 3);
}

RGINLINE void rgd3_dict_set_matrix(Rgd3Dict* d, const char* key, const Rgd3Mat3* m)
{
	rgd3_dict_set_floats(d, key, &m->mat[0].x, 9);
}

/* idDict::RandomPrefix: the value of a random key with the prefix, "" if none */
const char* rgd3_dict_random_prefix(const Rgd3Dict* d, const char* prefix, Rgd3Random* random);

int rgd3_dict_num(const Rgd3Dict* d);
const Rgd3KeyValue* rgd3_dict_get_kv(const Rgd3Dict* d, int index);
const Rgd3KeyValue* rgd3_dict_find_key(const Rgd3Dict* d, const char* key);
int rgd3_dict_find_key_index(const Rgd3Dict* d, const char* key);
void rgd3_dict_delete(Rgd3Dict* d, const char* key);
const Rgd3KeyValue* rgd3_dict_match_prefix(const Rgd3Dict* d, const char* prefix, const Rgd3KeyValue* last_match);

/* idDict::Parse: { "key" "value" ... }; returns 0 if a key was repeated */
int rgd3_dict_parse(Rgd3Dict* d, Rgd3Parser* parser);

void rgd3_dict_write_to_file(const Rgd3Dict* d, Rgd3File* f);
void rgd3_dict_read_from_file(Rgd3Dict* d, Rgd3File* f);

#endif
