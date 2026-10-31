/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_mem.c - see rgd3_mem.h. Heap blocks carry a 16-byte header so the
   per-tag counters stay exact without a separate table. */

#define RGD3_HEAP_MAGIC 0x52474433u /* "RGD3" */

typedef struct Rgd3HeapHeader
{
	u64 size;
	u32 tag;
	u32 magic;
} Rgd3HeapHeader;

typedef struct Rgd3MemState
{
	int initialized;
	RgArena permanent;
	RgArena level;
	RgArena frame[2];
	int frame_index;
	RgArena temp;
	SDL_SpinLock lock;
	i64 tag_bytes[RGD3_TAG_COUNT];
	i64 tag_count[RGD3_TAG_COUNT];
} Rgd3MemState;

static Rgd3MemState rgd3_mem;

static const char* const rgd3_mem_tag_names[RGD3_TAG_COUNT] = {
#define RGD3_MEM_TAG_NAME(name) #name,
	RGD3_MEM_TAGS(RGD3_MEM_TAG_NAME)
#undef RGD3_MEM_TAG_NAME
};

int rgd3_mem_init(size_t pool_bytes, size_t permanent_bytes)
{
	if (rgd3_mem.initialized)
		return 1;
	/* rg_malloc returns 0 on success (rg_mem.h). The pool is reserved and
	   committed lazily on Windows, so a large reservation costs nothing. */
	if (rg_malloc(pool_bytes) != 0)
		return 0;
	rgd3_mem.permanent = rg_arena_create(permanent_bytes);
	rgd3_mem.level = rg_arena_create(RGD3_LEVEL_ARENA_BYTES);
	rgd3_mem.frame[0] = rg_arena_create(RGD3_FRAME_ARENA_BYTES);
	rgd3_mem.frame[1] = rg_arena_create(RGD3_FRAME_ARENA_BYTES);
	rgd3_mem.temp = rg_arena_create(RGD3_TEMP_ARENA_BYTES);
	if (!rgd3_mem.permanent.memory || !rgd3_mem.level.memory || !rgd3_mem.frame[0].memory ||
		!rgd3_mem.frame[1].memory || !rgd3_mem.temp.memory)
		return 0;
	rgd3_mem.initialized = 1;
	return 1;
}

void rgd3_mem_shutdown(void)
{
	if (!rgd3_mem.initialized)
		return;
	rg_arena_free(&rgd3_mem.temp);
	rg_arena_free(&rgd3_mem.frame[1]);
	rg_arena_free(&rgd3_mem.frame[0]);
	rg_arena_free(&rgd3_mem.level);
	rg_arena_free(&rgd3_mem.permanent);
	rg_free();
	rgd3_mem.initialized = 0;
}

RgArena* rgd3_mem_permanent(void)
{
	return &rgd3_mem.permanent;
}

void* rgd3_perm_alloc(size_t size)
{
	void* p = rg_arena_alloc_aligned(&rgd3_mem.permanent, size, 16);
	if (!p)
		rgd3_error_fatal("permanent arena exhausted allocating %zu bytes", size);
	memset(p, 0, size);
	return p;
}

char* rgd3_perm_strdup(const char* text)
{
	size_t len = text ? strlen(text) : 0;
	char* out = (char*)rgd3_perm_alloc(len + 1);
	if (len)
		memcpy(out, text, len);
	return out;
}

/* ---- level, frame and temp arenas ---- */

void* rgd3_level_alloc(size_t size)
{
	void* p = rg_arena_alloc_aligned(&rgd3_mem.level, size ? size : 1, 16);
	if (!p)
		rgd3_error_drop("level arena exhausted allocating %zu bytes (%zu used)", size, rgd3_mem.level.used);
	memset(p, 0, size);
	return p;
}

char* rgd3_level_strdup(const char* text)
{
	size_t len = text ? strlen(text) : 0;
	char* out = (char*)rgd3_level_alloc(len + 1);
	if (len)
		memcpy(out, text, len);
	return out;
}

/* everything allocated for the level goes at once; the pages go back to
   the OS so a small map after a big one does not keep the memory */
void rgd3_mem_level_reset(void)
{
	rg_arena_decommit(&rgd3_mem.level);
}

size_t rgd3_mem_level_used(void)
{
	return rgd3_mem.level.used;
}

/* Frame memory is double buffered like the original frameData: what the
   front end builds for frame N stays valid while frame N is drawn, until
   the swap after next. */
void* rgd3_frame_alloc(size_t size)
{
	RgArena* arena = &rgd3_mem.frame[rgd3_mem.frame_index];
	void* p = rg_arena_alloc_aligned(arena, size ? size : 1, 16);
	if (!p)
		rgd3_error_drop("frame arena exhausted allocating %zu bytes (%zu used)", size, arena->used);
	return p;
}

void rgd3_mem_frame_swap(void)
{
	rgd3_mem.frame_index ^= 1;
	rg_arena_reset(&rgd3_mem.frame[rgd3_mem.frame_index]);
}

size_t rgd3_mem_frame_used(void)
{
	return rgd3_mem.frame[rgd3_mem.frame_index].used;
}

/* Scratch memory released in LIFO order (replaces _alloca16 /
   idTempArray). Reset at the start of every frame, so a drop that skips a
   release costs nothing beyond that frame. */
size_t rgd3_temp_mark(void)
{
	return rgd3_mem.temp.used;
}

void* rgd3_temp_alloc(size_t size)
{
	void* p = rg_arena_alloc_aligned(&rgd3_mem.temp, size ? size : 1, 16);
	if (!p)
		rgd3_error_drop("temp arena exhausted allocating %zu bytes (%zu used)", size, rgd3_mem.temp.used);
	return p;
}

void rgd3_temp_release(size_t mark)
{
	RG_ASSERT(mark <= rgd3_mem.temp.used);
	if (mark <= rgd3_mem.temp.used)
		rgd3_mem.temp.used = mark;
}

void rgd3_mem_temp_reset(void)
{
	rg_arena_reset(&rgd3_mem.temp);
}

static void rgd3_mem_account(u32 tag, i64 bytes, i64 count)
{
	SDL_LockSpinlock(&rgd3_mem.lock);
	rgd3_mem.tag_bytes[tag] += bytes;
	rgd3_mem.tag_count[tag] += count;
	SDL_UnlockSpinlock(&rgd3_mem.lock);
}

void* rgd3_heap_alloc(size_t size, Rgd3MemTag tag)
{
	Rgd3HeapHeader* header;
	if ((u32)tag >= RGD3_TAG_COUNT)
		tag = RGD3_TAG_SYSTEM;
	header = (Rgd3HeapHeader*)SDL_malloc(sizeof(Rgd3HeapHeader) + size);
	if (!header)
		rgd3_error_fatal("out of memory allocating %zu bytes (%s)", size, rgd3_mem_tag_names[tag]);
	header->size = size;
	header->tag = (u32)tag;
	header->magic = RGD3_HEAP_MAGIC;
	rgd3_mem_account((u32)tag, (i64)size, 1);
	return header + 1;
}

void* rgd3_heap_calloc(size_t size, Rgd3MemTag tag)
{
	void* p = rgd3_heap_alloc(size, tag);
	memset(p, 0, size);
	return p;
}

void* rgd3_heap_realloc(void* ptr, size_t size, Rgd3MemTag tag)
{
	Rgd3HeapHeader* header;
	Rgd3HeapHeader* grown;
	if (!ptr)
		return rgd3_heap_alloc(size, tag);
	header = (Rgd3HeapHeader*)ptr - 1;
	RG_ASSERT_MSG(header->magic == RGD3_HEAP_MAGIC, "rgd3_heap_realloc on a foreign pointer");
	grown = (Rgd3HeapHeader*)SDL_realloc(header, sizeof(Rgd3HeapHeader) + size);
	if (!grown)
		rgd3_error_fatal("out of memory reallocating %zu bytes", size);
	rgd3_mem_account(grown->tag, (i64)size - (i64)grown->size, 0);
	grown->size = size;
	return grown + 1;
}

void rgd3_heap_free(void* ptr)
{
	Rgd3HeapHeader* header;
	if (!ptr)
		return;
	header = (Rgd3HeapHeader*)ptr - 1;
	RG_ASSERT_MSG(header->magic == RGD3_HEAP_MAGIC, "rgd3_heap_free on a foreign pointer");
	header->magic = 0;
	rgd3_mem_account(header->tag, -(i64)header->size, -1);
	SDL_free(header);
}

char* rgd3_heap_strdup(const char* text, Rgd3MemTag tag)
{
	size_t len = text ? strlen(text) : 0;
	char* out = (char*)rgd3_heap_alloc(len + 1, tag);
	if (len)
		memcpy(out, text, len);
	out[len] = 0;
	return out;
}

i64 rgd3_mem_tag_bytes(Rgd3MemTag tag)
{
	i64 v;
	SDL_LockSpinlock(&rgd3_mem.lock);
	v = rgd3_mem.tag_bytes[tag];
	SDL_UnlockSpinlock(&rgd3_mem.lock);
	return v;
}

i64 rgd3_mem_tag_count(Rgd3MemTag tag)
{
	i64 v;
	SDL_LockSpinlock(&rgd3_mem.lock);
	v = rgd3_mem.tag_count[tag];
	SDL_UnlockSpinlock(&rgd3_mem.lock);
	return v;
}

const char* rgd3_mem_tag_name(Rgd3MemTag tag)
{
	return (u32)tag < RGD3_TAG_COUNT ? rgd3_mem_tag_names[tag] : "?";
}

void rgd3_mem_print_tags(void)
{
	i64 total = 0;
	int i;
	rgd3_printf("%-12s %12s %8s\n", "tag", "bytes", "blocks");
	for (i = 0; i < RGD3_TAG_COUNT; ++i)
	{
		i64 bytes = rgd3_mem_tag_bytes((Rgd3MemTag)i);
		i64 count = rgd3_mem_tag_count((Rgd3MemTag)i);
		if (!bytes && !count)
			continue;
		rgd3_printf("%-12s %12lld %8lld\n", rgd3_mem_tag_names[i], (long long)bytes, (long long)count);
		total += bytes;
	}
	rgd3_printf("heap total %lld bytes, permanent arena %zu / %zu bytes, pool %zu / %zu bytes\n",
		(long long)total, rgd3_mem.permanent.used, rgd3_mem.permanent.capacity, rg_used(), rg_total());
	rgd3_printf("level arena %zu / %zu, frame arenas %zu + %zu / %zu each, temp %zu / %zu bytes\n",
		rgd3_mem.level.used, rgd3_mem.level.capacity, rgd3_mem.frame[0].used, rgd3_mem.frame[1].used,
		rgd3_mem.frame[0].capacity, rgd3_mem.temp.used, rgd3_mem.temp.capacity);
}

static void rgd3_mem_cmd_tags(const Rgd3CmdArgs* args)
{
	RG_UNUSED(args);
	rgd3_mem_print_tags();
}

void rgd3_mem_register(void)
{
	rgd3_cmd_add("memTags", rgd3_mem_cmd_tags, RGD3_CMD_FL_SYSTEM, "prints heap usage per memory tag", NULL);
}
