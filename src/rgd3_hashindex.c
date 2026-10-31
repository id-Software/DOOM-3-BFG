/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_hashindex.c - see rgd3_hashindex.h (neo/idlib/containers/HashIndex.cpp). */

static void rgd3_hashindex_defaults(Rgd3HashIndex* h)
{
	if (!h->hash_size)
	{
		h->hash_size = RGD3_HASHINDEX_DEFAULT_SIZE;
		h->hash_mask = h->hash_size - 1;
		if (!h->index_size)
			h->index_size = RGD3_HASHINDEX_DEFAULT_SIZE;
	}
	if (!h->granularity)
		h->granularity = RGD3_HASHINDEX_DEFAULT_GRANULARITY;
}

void rgd3_hashindex_init(Rgd3HashIndex* h, int hash_size, int index_size)
{
	RG_ASSERT(hash_size > 0 && (hash_size & (hash_size - 1)) == 0);
	memset(h, 0, sizeof(*h));
	h->hash_size = hash_size;
	h->hash_mask = hash_size - 1;
	h->index_size = index_size;
	h->granularity = RGD3_HASHINDEX_DEFAULT_GRANULARITY;
}

static void rgd3_hashindex_allocate(Rgd3HashIndex* h, int new_hash_size, int new_index_size)
{
	int granularity = h->granularity;
	rgd3_hashindex_free(h);
	h->granularity = granularity;
	h->hash_size = new_hash_size;
	h->hash = (int*)rgd3_heap_alloc((size_t)new_hash_size * sizeof(int), RGD3_TAG_HASH);
	memset(h->hash, 0xff, (size_t)new_hash_size * sizeof(int));
	h->index_size = new_index_size;
	h->index_chain = (int*)rgd3_heap_alloc((size_t)new_index_size * sizeof(int), RGD3_TAG_HASH);
	memset(h->index_chain, 0xff, (size_t)new_index_size * sizeof(int));
	h->hash_mask = new_hash_size - 1;
}

void rgd3_hashindex_free(Rgd3HashIndex* h)
{
	if (h->hash)
	{
		rgd3_heap_free(h->hash);
		h->hash = NULL;
	}
	if (h->index_chain)
	{
		rgd3_heap_free(h->index_chain);
		h->index_chain = NULL;
	}
}

void rgd3_hashindex_clear(Rgd3HashIndex* h)
{
	/* only the buckets; the chain does not need clearing */
	if (h->hash)
		memset(h->hash, 0xff, (size_t)h->hash_size * sizeof(int));
}

void rgd3_hashindex_clear_resize(Rgd3HashIndex* h, int hash_size, int index_size)
{
	RG_ASSERT(hash_size > 0 && (hash_size & (hash_size - 1)) == 0);
	rgd3_hashindex_free(h);
	h->hash_size = hash_size;
	h->hash_mask = hash_size - 1;
	h->index_size = index_size;
}

void rgd3_hashindex_assign(Rgd3HashIndex* h, const Rgd3HashIndex* other)
{
	if (h == other)
		return;
	rgd3_hashindex_free(h);
	h->granularity = other->granularity;
	h->hash_size = other->hash_size;
	h->hash_mask = other->hash_mask;
	h->index_size = other->index_size;
	if (other->hash)
	{
		h->hash = (int*)rgd3_heap_alloc((size_t)h->hash_size * sizeof(int), RGD3_TAG_HASH);
		memcpy(h->hash, other->hash, (size_t)h->hash_size * sizeof(int));
		h->index_chain = (int*)rgd3_heap_alloc((size_t)h->index_size * sizeof(int), RGD3_TAG_HASH);
		memcpy(h->index_chain, other->index_chain, (size_t)h->index_size * sizeof(int));
	}
}

void rgd3_hashindex_set_granularity(Rgd3HashIndex* h, int granularity)
{
	RG_ASSERT(granularity > 0);
	h->granularity = granularity;
}

void rgd3_hashindex_resize_index(Rgd3HashIndex* h, int new_index_size)
{
	int mod, new_size;
	int* old_chain;

	rgd3_hashindex_defaults(h);
	if (new_index_size <= h->index_size)
		return;
	mod = new_index_size % h->granularity;
	new_size = mod ? new_index_size + h->granularity - mod : new_index_size;
	if (!h->index_chain)
	{
		h->index_size = new_size;
		return;
	}
	old_chain = h->index_chain;
	h->index_chain = (int*)rgd3_heap_alloc((size_t)new_size * sizeof(int), RGD3_TAG_HASH);
	memcpy(h->index_chain, old_chain, (size_t)h->index_size * sizeof(int));
	memset(h->index_chain + h->index_size, 0xff, (size_t)(new_size - h->index_size) * sizeof(int));
	rgd3_heap_free(old_chain);
	h->index_size = new_size;
}

/* assumes index is not in the hash yet */
void rgd3_hashindex_add(Rgd3HashIndex* h, int key, int index)
{
	int k;
	RG_ASSERT(index >= 0);
	rgd3_hashindex_defaults(h);
	if (!h->hash)
		rgd3_hashindex_allocate(h, h->hash_size, index >= h->index_size ? index + 1 : h->index_size);
	else if (index >= h->index_size)
		rgd3_hashindex_resize_index(h, index + 1);
	k = key & h->hash_mask;
	h->index_chain[index] = h->hash[k];
	h->hash[k] = index;
}

void rgd3_hashindex_remove(Rgd3HashIndex* h, int key, int index)
{
	int k, i;
	if (!h->hash)
		return;
	k = key & h->hash_mask;
	if (h->hash[k] == index)
	{
		h->hash[k] = h->index_chain[index];
	}
	else
	{
		for (i = h->hash[k]; i != -1; i = h->index_chain[i])
		{
			if (h->index_chain[i] == index)
			{
				h->index_chain[i] = h->index_chain[index];
				break;
			}
		}
	}
	h->index_chain[index] = -1;
}

/* insert an entry, shifting every index >= index up by one */
void rgd3_hashindex_insert_index(Rgd3HashIndex* h, int key, int index)
{
	int i, max;
	if (h->hash)
	{
		max = index;
		for (i = 0; i < h->hash_size; i++)
		{
			if (h->hash[i] >= index)
			{
				h->hash[i]++;
				if (h->hash[i] > max)
					max = h->hash[i];
			}
		}
		for (i = 0; i < h->index_size; i++)
		{
			if (h->index_chain[i] >= index)
			{
				h->index_chain[i]++;
				if (h->index_chain[i] > max)
					max = h->index_chain[i];
			}
		}
		if (max >= h->index_size)
			rgd3_hashindex_resize_index(h, max + 1);
		for (i = max; i > index; i--)
			h->index_chain[i] = h->index_chain[i - 1];
		h->index_chain[index] = -1;
	}
	rgd3_hashindex_add(h, key, index);
}

/* remove an entry, shifting every index > index down by one */
void rgd3_hashindex_remove_index(Rgd3HashIndex* h, int key, int index)
{
	int i, max;
	rgd3_hashindex_remove(h, key, index);
	if (h->hash)
	{
		max = index;
		for (i = 0; i < h->hash_size; i++)
		{
			if (h->hash[i] >= index)
			{
				if (h->hash[i] > max)
					max = h->hash[i];
				h->hash[i]--;
			}
		}
		for (i = 0; i < h->index_size; i++)
		{
			if (h->index_chain[i] >= index)
			{
				if (h->index_chain[i] > max)
					max = h->index_chain[i];
				h->index_chain[i]--;
			}
		}
		for (i = index; i < max; i++)
			h->index_chain[i] = h->index_chain[i + 1];
		h->index_chain[max] = -1;
	}
}

/* 0..100: how evenly the entries spread over the buckets */
int rgd3_hashindex_spread(const Rgd3HashIndex* h)
{
	int i, index, total = 0, average, error = 0, e;
	int* counts;

	if (!h->hash)
		return 100;
	counts = (int*)rgd3_heap_alloc((size_t)h->hash_size * sizeof(int), RGD3_TAG_TEMP);
	for (i = 0; i < h->hash_size; i++)
	{
		counts[i] = 0;
		for (index = h->hash[i]; index >= 0; index = h->index_chain[index])
			counts[i]++;
		total += counts[i];
	}
	if (total <= 1)
	{
		rgd3_heap_free(counts);
		return 100;
	}
	average = total / h->hash_size;
	for (i = 0; i < h->hash_size; i++)
	{
		e = abs(counts[i] - average);
		if (e > 1)
			error += e - 1;
	}
	rgd3_heap_free(counts);
	return 100 - (error * 100 / total);
}

size_t rgd3_hashindex_allocated(const Rgd3HashIndex* h)
{
	return h->hash ? (size_t)(h->hash_size + h->index_size) * sizeof(int) : 0;
}
