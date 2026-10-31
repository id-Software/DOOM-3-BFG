/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_hashindex.h - hash table of int indexes into an external array
   (idHashIndex, neo/idlib/containers/HashIndex.h/.cpp). Memory is allocated
   on the first add (RGD3_TAG_HASH). A zero-initialised index behaves like
   the default constructor: 1024 buckets, 1024 indexes, granularity 1024.
   Keys come from rgd3_hashindex_key_*; the string keys use idStr::Hash /
   idStr::IHash so chains match the original. */
#ifndef RGD3_HASHINDEX_H
#define RGD3_HASHINDEX_H
#ifndef RGD3_ALL_C
#error "rgd3 headers are included through src/all.c only"
#endif

#define RGD3_HASHINDEX_DEFAULT_SIZE 1024
#define RGD3_HASHINDEX_DEFAULT_GRANULARITY 1024

typedef struct Rgd3HashIndex
{
	int hash_size;   /* power of two; 0 = default */
	int* hash;       /* NULL until allocated */
	int index_size;
	int* index_chain;
	int granularity; /* 0 = default */
	int hash_mask;
} Rgd3HashIndex;

void rgd3_hashindex_init(Rgd3HashIndex* h, int hash_size, int index_size);
void rgd3_hashindex_free(Rgd3HashIndex* h);
void rgd3_hashindex_clear(Rgd3HashIndex* h);                                 /* empty, keep memory */
void rgd3_hashindex_clear_resize(Rgd3HashIndex* h, int hash_size, int index_size); /* free + new sizes */
void rgd3_hashindex_assign(Rgd3HashIndex* h, const Rgd3HashIndex* other);
void rgd3_hashindex_set_granularity(Rgd3HashIndex* h, int granularity);
void rgd3_hashindex_resize_index(Rgd3HashIndex* h, int new_index_size);
void rgd3_hashindex_add(Rgd3HashIndex* h, int key, int index);
void rgd3_hashindex_remove(Rgd3HashIndex* h, int key, int index);
void rgd3_hashindex_insert_index(Rgd3HashIndex* h, int key, int index);
void rgd3_hashindex_remove_index(Rgd3HashIndex* h, int key, int index);
int rgd3_hashindex_spread(const Rgd3HashIndex* h);
size_t rgd3_hashindex_allocated(const Rgd3HashIndex* h);

RGINLINE int rgd3_hashindex_mask(const Rgd3HashIndex* h)
{
	return h->hash_size ? h->hash_mask : RGD3_HASHINDEX_DEFAULT_SIZE - 1;
}

RGINLINE int rgd3_hashindex_first(const Rgd3HashIndex* h, int key)
{
	return h->hash ? h->hash[key & h->hash_mask] : -1;
}

RGINLINE int rgd3_hashindex_next(const Rgd3HashIndex* h, int index)
{
	return h->index_chain ? h->index_chain[index] : -1;
}

RGINLINE int rgd3_hashindex_key_string(const Rgd3HashIndex* h, const char* text, int case_sensitive)
{
	return (case_sensitive ? rgd3_str_id_hash(text) : rgd3_str_id_ihash(text)) & rgd3_hashindex_mask(h);
}

RGINLINE int rgd3_hashindex_key_int2(const Rgd3HashIndex* h, int n1, int n2)
{
	return (n1 + n2) & rgd3_hashindex_mask(h);
}

RGINLINE int rgd3_hashindex_key_int(const Rgd3HashIndex* h, int n)
{
	return n & rgd3_hashindex_mask(h);
}

RGINLINE int rgd3_hashindex_key_vec3(const Rgd3HashIndex* h, const float* v)
{
	return ((int)v[0] + (int)v[1] + (int)v[2]) & rgd3_hashindex_mask(h);
}

#endif
