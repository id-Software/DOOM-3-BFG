/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_mem.h - memory lifetimes: the rg_core pool, permanent / level / frame /
   temp arenas and a tagged heap for anything that is freed or replaced. Tags follow the
   original sys_alloc_tags.h grouping. */
#ifndef RGD3_MEM_H
#define RGD3_MEM_H
#ifndef RGD3_ALL_C
#error "rgd3 headers are included through src/all.c only"
#endif

#define RGD3_MEM_TAGS(X) \
	X(TEMP) X(FILE) X(RESOURCE) X(DECL) X(DECLTEXT) X(CVAR) X(CMD) X(STRING) \
	X(LEXER) X(PARSER) X(HASH) X(DICT) X(EVENTS) X(KEYS) X(CONSOLE) X(SAVEGAMES) X(SYSTEM) \
	X(IMAGE) X(MODEL) X(RENDER) X(GAME) X(ENTITY) X(SCRIPT) X(AI) X(PHYSICS) \
	X(COLLISION) X(AAS) X(AUDIO) X(SWF) X(FONT)

typedef enum Rgd3MemTag
{
#define RGD3_MEM_TAG_ENUM(name) RGD3_TAG_##name,
	RGD3_MEM_TAGS(RGD3_MEM_TAG_ENUM)
#undef RGD3_MEM_TAG_ENUM
	RGD3_TAG_COUNT
} Rgd3MemTag;

int rgd3_mem_init(size_t pool_bytes, size_t permanent_bytes);
void rgd3_mem_shutdown(void);
void rgd3_mem_register(void);

RgArena* rgd3_mem_permanent(void);
void* rgd3_perm_alloc(size_t size);
char* rgd3_perm_strdup(const char* text);

/* Arenas (bump allocated from the pool, never freed one by one):
   level: data that lives until the next map change (zeroed);
   frame: double buffered, valid until the swap after next;
   temp:  LIFO scratch with mark/release, reset every frame.
   Exhaustion drops. */
#define RGD3_LEVEL_ARENA_BYTES MB(768)
#define RGD3_FRAME_ARENA_BYTES MB(64)
#define RGD3_TEMP_ARENA_BYTES MB(64)
void* rgd3_level_alloc(size_t size);
char* rgd3_level_strdup(const char* text);
void rgd3_mem_level_reset(void);
size_t rgd3_mem_level_used(void);
void* rgd3_frame_alloc(size_t size);
void rgd3_mem_frame_swap(void);
size_t rgd3_mem_frame_used(void);
size_t rgd3_temp_mark(void);
void* rgd3_temp_alloc(size_t size);
void rgd3_temp_release(size_t mark);
void rgd3_mem_temp_reset(void);

void* rgd3_heap_alloc(size_t size, Rgd3MemTag tag);
void* rgd3_heap_calloc(size_t size, Rgd3MemTag tag);
void* rgd3_heap_realloc(void* ptr, size_t size, Rgd3MemTag tag);
void rgd3_heap_free(void* ptr);
char* rgd3_heap_strdup(const char* text, Rgd3MemTag tag);

i64 rgd3_mem_tag_bytes(Rgd3MemTag tag);
i64 rgd3_mem_tag_count(Rgd3MemTag tag);
const char* rgd3_mem_tag_name(Rgd3MemTag tag);
void rgd3_mem_print_tags(void);

#endif
