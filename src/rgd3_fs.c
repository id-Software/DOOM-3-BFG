/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_fs.c - see rgd3_fs.h. Base path discovery replaces Sys_DefaultBasePath:
   +set fs_basepath, then RGD3_BASEPATH, cwd, exe dir, then the Steam install. */

typedef struct Rgd3ResourceEntry
{
	const char* name;
	u32 offset;
	u32 length;
} Rgd3ResourceEntry;

typedef struct Rgd3ResourceContainer
{
	char path[RGD3_MAX_OSPATH];
	SDL_IOStream* io;
	SDL_Mutex* lock;
	u8* memory;
	size_t memory_size;
	Rgd3ResourceEntry* entries;
	u32 count;
	char* names;
	u32* hash;
	u32 hash_mask;
} Rgd3ResourceContainer;

typedef struct Rgd3SearchPath
{
	char path[RGD3_MAX_OSPATH];
	char gamedir[64];
} Rgd3SearchPath;

typedef enum Rgd3FileKind
{
	RGD3_FILE_OS,
	RGD3_FILE_MEMORY,
	RGD3_FILE_CONTAINER
} Rgd3FileKind;

struct Rgd3File
{
	Rgd3FileKind kind;
	SDL_IOStream* io;
	const u8* memory;
	Rgd3ResourceContainer* container;
	u64 base;
	i64 length;
	i64 pos;
	int writable;
	char name[RGD3_MAX_OSPATH];
};

typedef struct Rgd3FsState
{
	int initialized;
	Rgd3SearchPath paths[RGD3_MAX_SEARCH_PATHS];
	int num_paths;
	Rgd3ResourceContainer* containers[RGD3_MAX_CONTAINERS];
	int num_containers;
} Rgd3FsState;

static Rgd3FsState rgd3_fs;

static Rgd3Cvar cv_fs_basepath = RGD3_CVAR("fs_basepath", "", RGD3_CVAR_SYSTEM | RGD3_CVAR_INIT, "directory that contains base/ (the DOOM 3 BFG Edition install)");
static Rgd3Cvar cv_fs_savepath = RGD3_CVAR("fs_savepath", "", RGD3_CVAR_SYSTEM | RGD3_CVAR_INIT, "writable directory for config, saves and screenshots");
static Rgd3Cvar cv_fs_game = RGD3_CVAR("fs_game", "", RGD3_CVAR_SYSTEM | RGD3_CVAR_INIT | RGD3_CVAR_SERVERINFO, "mod path");
static Rgd3Cvar cv_fs_debug = RGD3_CVAR_RANGE("fs_debug", "0", RGD3_CVAR_SYSTEM | RGD3_CVAR_INTEGER, "", 0, 2);
static Rgd3Cvar cv_fs_resourceLoadPriority = RGD3_CVAR("fs_resourceLoadPriority", "1", RGD3_CVAR_SYSTEM, "if 1, open requests are honored from resource files first; if 0, the resource files are checked after normal search paths");

static u32 rgd3_fs_be32(const u8* p)
{
	return ((u32)p[0] << 24) | ((u32)p[1] << 16) | ((u32)p[2] << 8) | (u32)p[3];
}

static u32 rgd3_fs_le32(const u8* p)
{
	return (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24);
}

/* Lowercase, '/' separators, no leading "./" or slashes. */
static void rgd3_fs_normalize(const char* in, char* out, size_t size)
{
	while (in[0] == '.' && (in[1] == '/' || in[1] == '\\'))
		in += 2;
	while (*in == '/' || *in == '\\')
		++in;
	rgd3_str_copynz(out, in, size);
	rgd3_str_backslashes_to_slashes(out);
	rgd3_str_to_lower(out);
}

void rgd3_fs_build_os_path(const char* base, const char* gamedir, const char* relative, char* out, size_t size)
{
	if (gamedir && gamedir[0])
		rg_snprintf(out, size, "%s/%s/%s", base, gamedir, relative);
	else
		rg_snprintf(out, size, "%s/%s", base, relative);
	rgd3_str_backslashes_to_slashes(out);
}

const char* rgd3_fs_basepath(void)
{
	return cv_fs_basepath.value ? cv_fs_basepath.value : "";
}

const char* rgd3_fs_savepath(void)
{
	return cv_fs_savepath.value ? cv_fs_savepath.value : "";
}

/* ---------------------------------------------------------------------------
   Resource containers
--------------------------------------------------------------------------- */

static void rgd3_fs_container_free(Rgd3ResourceContainer* c)
{
	if (!c)
		return;
	if (c->io)
		SDL_CloseIO(c->io);
	if (c->lock)
		SDL_DestroyMutex(c->lock);
	if (c->memory)
		SDL_free(c->memory);
	rgd3_heap_free(c->entries);
	rgd3_heap_free(c->names);
	rgd3_heap_free(c->hash);
	rgd3_heap_free(c);
}

static int rgd3_fs_container_read_at(Rgd3ResourceContainer* c, u64 offset, void* dst, size_t size)
{
	size_t got;
	if (c->memory)
	{
		if (offset + size > c->memory_size)
			return 0;
		memcpy(dst, c->memory + offset, size);
		return 1;
	}
	SDL_LockMutex(c->lock);
	got = 0;
	if (SDL_SeekIO(c->io, (Sint64)offset, SDL_IO_SEEK_SET) >= 0)
		got = SDL_ReadIO(c->io, dst, size);
	SDL_UnlockMutex(c->lock);
	return got == size;
}

static void rgd3_fs_container_hash_insert(Rgd3ResourceContainer* c, u32 index)
{
	u32 slot = rgd3_str_ihash(c->entries[index].name) & c->hash_mask;
	for (;;)
	{
		u32 cur = c->hash[slot];
		if (!cur)
		{
			c->hash[slot] = index + 1;
			return;
		}
		/* Duplicate names: the later entry wins, as idHashIndex::Add puts it first. */
		if (strcmp(c->entries[cur - 1].name, c->entries[index].name) == 0)
		{
			c->hash[slot] = index + 1;
			return;
		}
		slot = (slot + 1) & c->hash_mask;
	}
}

static const Rgd3ResourceEntry* rgd3_fs_container_find(const Rgd3ResourceContainer* c, const char* normalized)
{
	u32 slot = rgd3_str_ihash(normalized) & c->hash_mask;
	for (;;)
	{
		u32 cur = c->hash[slot];
		if (!cur)
			return NULL;
		if (strcmp(c->entries[cur - 1].name, normalized) == 0)
			return &c->entries[cur - 1];
		slot = (slot + 1) & c->hash_mask;
	}
}

int rgd3_fs_mount_container(const char* os_path, int in_memory)
{
	Rgd3ResourceContainer* c;
	u8 header[12];
	u8* table = NULL;
	u32 table_offset, table_length, count, i, cap;
	size_t pos, names_size;
	char* name_cursor;

	if (rgd3_fs.num_containers >= RGD3_MAX_CONTAINERS)
	{
		rgd3_warning("too many resource containers, skipping %s", os_path);
		return -1;
	}
	c = (Rgd3ResourceContainer*)rgd3_heap_calloc(sizeof(*c), RGD3_TAG_RESOURCE);
	rgd3_str_copynz(c->path, os_path, sizeof(c->path));
	if (in_memory)
	{
		c->memory = (u8*)SDL_LoadFile(os_path, &c->memory_size);
		if (!c->memory)
		{
			rgd3_warning("Unable to open resource file %s", os_path);
			rgd3_fs_container_free(c);
			return -1;
		}
	}
	else
	{
		c->io = SDL_IOFromFile(os_path, "rb");
		if (!c->io)
		{
			rgd3_warning("Unable to open resource file %s", os_path);
			rgd3_fs_container_free(c);
			return -1;
		}
		c->lock = SDL_CreateMutex();
	}

	if (!rgd3_fs_container_read_at(c, 0, header, sizeof(header)) || rgd3_fs_be32(header) != RGD3_RESOURCE_MAGIC)
	{
		rgd3_warning("%s is not a resource file", os_path);
		rgd3_fs_container_free(c);
		return -1;
	}
	table_offset = rgd3_fs_be32(header + 4);
	table_length = rgd3_fs_be32(header + 8);
	if (table_length < 4)
	{
		rgd3_warning("%s has an empty resource table", os_path);
		rgd3_fs_container_free(c);
		return -1;
	}
	table = (u8*)rgd3_heap_alloc(table_length, RGD3_TAG_TEMP);
	if (!rgd3_fs_container_read_at(c, table_offset, table, table_length))
	{
		rgd3_warning("%s: could not read the resource table", os_path);
		rgd3_heap_free(table);
		rgd3_fs_container_free(c);
		return -1;
	}

	count = rgd3_fs_be32(table);
	/* Every entry carries at least 12 bytes beyond its name. */
	if ((u64)count * 12u > (u64)table_length - 4u)
	{
		rgd3_warning("%s: corrupt resource table (%u entries)", os_path, count);
		rgd3_heap_free(table);
		rgd3_fs_container_free(c);
		return -1;
	}
	names_size = table_length;
	c->names = (char*)rgd3_heap_alloc(names_size + count, RGD3_TAG_RESOURCE);
	c->entries = (Rgd3ResourceEntry*)rgd3_heap_calloc(sizeof(Rgd3ResourceEntry) * (count ? count : 1), RGD3_TAG_RESOURCE);
	name_cursor = c->names;
	pos = 4;
	for (i = 0; i < count; ++i)
	{
		u32 len;
		if (pos + 4 > table_length)
			break;
		len = rgd3_fs_le32(table + pos);
		pos += 4;
		if (pos + len + 8 > table_length)
			break;
		memcpy(name_cursor, table + pos, len);
		name_cursor[len] = 0;
		rgd3_str_backslashes_to_slashes(name_cursor);
		rgd3_str_to_lower(name_cursor);
		c->entries[i].name = name_cursor;
		name_cursor += len + 1;
		pos += len;
		c->entries[i].offset = rgd3_fs_be32(table + pos);
		c->entries[i].length = rgd3_fs_be32(table + pos + 4);
		pos += 8;
	}
	rgd3_heap_free(table);
	if (i != count)
	{
		rgd3_warning("%s: truncated resource table (%u of %u entries)", os_path, i, count);
		count = i;
	}
	c->count = count;

	cap = 16;
	while (cap < count * 2)
		cap <<= 1;
	c->hash = (u32*)rgd3_heap_calloc(sizeof(u32) * cap, RGD3_TAG_RESOURCE);
	c->hash_mask = cap - 1;
	for (i = 0; i < count; ++i)
		if (c->entries[i].name[0])
			rgd3_fs_container_hash_insert(c, i);

	rgd3_fs.containers[rgd3_fs.num_containers++] = c;
	rgd3_printf("Loaded resource file %s (%u entries)\n", rgd3_str_file_name(os_path), count);
	return rgd3_fs.num_containers - 1;
}

int rgd3_fs_unmount_container(const char* file_name)
{
	int i;
	for (i = 0; i < rgd3_fs.num_containers; ++i)
	{
		if (rgd3_str_icmp(rgd3_str_file_name(rgd3_fs.containers[i]->path), file_name) == 0)
		{
			rgd3_fs_container_free(rgd3_fs.containers[i]);
			memmove(&rgd3_fs.containers[i], &rgd3_fs.containers[i + 1],
				sizeof(rgd3_fs.containers[0]) * (size_t)(rgd3_fs.num_containers - i - 1));
			--rgd3_fs.num_containers;
			return 1;
		}
	}
	return 0;
}

int rgd3_fs_num_containers(void)
{
	return rgd3_fs.num_containers;
}

u32 rgd3_fs_container_entry_count(int index)
{
	return (index >= 0 && index < rgd3_fs.num_containers) ? rgd3_fs.containers[index]->count : 0;
}

const char* rgd3_fs_container_path(int index)
{
	return (index >= 0 && index < rgd3_fs.num_containers) ? rgd3_fs.containers[index]->path : "";
}

static int rgd3_fs_sort_names(const void* a, const void* b)
{
	return rgd3_str_icmp(*(const char* const*)a, *(const char* const*)b);
}

/* Mounts every <dir>/*.resources, sorted by name as AddGameDirectory does.
   _ordered.resources is read into memory. */
static void rgd3_fs_mount_directory(const char* os_dir)
{
	int count = 0, i;
	char** names = SDL_GlobDirectory(os_dir, "*.resources", SDL_GLOB_CASEINSENSITIVE, &count);
	if (!names)
		return;
	qsort(names, (size_t)count, sizeof(char*), rgd3_fs_sort_names);
	for (i = 0; i < count; ++i)
	{
		char path[RGD3_MAX_OSPATH];
		if (strchr(names[i], '/') || strchr(names[i], '\\'))
			continue;
		rg_snprintf(path, sizeof(path), "%s/%s", os_dir, names[i]);
		rgd3_fs_mount_container(path, rgd3_str_icmp(names[i], "_ordered.resources") == 0);
	}
	SDL_free(names);
}

/* ---------------------------------------------------------------------------
   Files
--------------------------------------------------------------------------- */

static Rgd3File* rgd3_fs_new_file(Rgd3FileKind kind, const char* name)
{
	Rgd3File* f = (Rgd3File*)rgd3_heap_calloc(sizeof(Rgd3File), RGD3_TAG_FILE);
	f->kind = kind;
	rgd3_str_copynz(f->name, name, sizeof(f->name));
	return f;
}

static Rgd3File* rgd3_fs_open_from_containers(const char* normalized)
{
	int i;
	for (i = rgd3_fs.num_containers - 1; i >= 0; --i)
	{
		Rgd3ResourceContainer* c = rgd3_fs.containers[i];
		const Rgd3ResourceEntry* e = rgd3_fs_container_find(c, normalized);
		if (e)
		{
			Rgd3File* f = rgd3_fs_new_file(c->memory ? RGD3_FILE_MEMORY : RGD3_FILE_CONTAINER, normalized);
			f->container = c;
			f->base = e->offset;
			f->length = e->length;
			if (c->memory)
				f->memory = c->memory + e->offset;
			if (cv_fs_debug.integer_value)
				rgd3_printf("rgd3_fs_open_read: %s (found in %s)\n", normalized, rgd3_str_file_name(c->path));
			return f;
		}
	}
	return NULL;
}

static Rgd3File* rgd3_fs_open_from_paths(const char* relative)
{
	int i;
	for (i = rgd3_fs.num_paths - 1; i >= 0; --i)
	{
		char os_path[RGD3_MAX_OSPATH];
		rgd3_fs_build_os_path(rgd3_fs.paths[i].path, rgd3_fs.paths[i].gamedir, relative, os_path, sizeof(os_path));
		if (rgd3_platform_is_file(os_path))
		{
			Rgd3File* f = rgd3_fs_open_explicit_read(os_path);
			if (f)
			{
				rgd3_str_copynz(f->name, relative, sizeof(f->name));
				if (cv_fs_debug.integer_value)
					rgd3_printf("rgd3_fs_open_read: %s (found in '%s')\n", relative, os_path);
				return f;
			}
		}
	}
	return NULL;
}

Rgd3File* rgd3_fs_open_read(const char* relative)
{
	char normalized[RGD3_MAX_OSPATH];
	char cleaned[RGD3_MAX_OSPATH];
	Rgd3File* f;
	if (!relative || !relative[0])
		return NULL;
	rgd3_fs_normalize(relative, normalized, sizeof(normalized));
	rgd3_str_copynz(cleaned, relative, sizeof(cleaned));
	rgd3_str_backslashes_to_slashes(cleaned);
	if (strstr(cleaned, ".."))
	{
		rgd3_warning("rgd3_fs_open_read: refusing relative path with '..': %s", relative);
		return NULL;
	}
	if (cv_fs_resourceLoadPriority.integer_value)
	{
		f = rgd3_fs_open_from_containers(normalized);
		if (!f)
			f = rgd3_fs_open_from_paths(cleaned);
	}
	else
	{
		f = rgd3_fs_open_from_paths(cleaned);
		if (!f)
			f = rgd3_fs_open_from_containers(normalized);
	}
	if (!f && cv_fs_debug.integer_value)
		rgd3_printf("rgd3_fs_open_read: can't find %s\n", relative);
	return f;
}

Rgd3File* rgd3_fs_open_explicit_read(const char* os_path)
{
	Rgd3File* f;
	SDL_IOStream* io = SDL_IOFromFile(os_path, "rb");
	if (!io)
		return NULL;
	f = rgd3_fs_new_file(RGD3_FILE_OS, os_path);
	f->io = io;
	f->length = (i64)SDL_GetIOSize(io);
	return f;
}

static void rgd3_fs_create_parent_dirs(const char* os_path)
{
	char dir[RGD3_MAX_OSPATH];
	char* slash;
	rgd3_str_copynz(dir, os_path, sizeof(dir));
	slash = strrchr(dir, '/');
	if (!slash)
		return;
	*slash = 0;
	if (!rgd3_platform_is_dir(dir))
		SDL_CreateDirectory(dir);
}

Rgd3File* rgd3_fs_open_write(const char* relative)
{
	char os_path[RGD3_MAX_OSPATH];
	SDL_IOStream* io;
	Rgd3File* f;
	if (!rgd3_fs.initialized || !relative || !relative[0] || strstr(relative, ".."))
		return NULL;
	rgd3_fs_build_os_path(rgd3_fs_savepath(), cv_fs_game.value[0] ? cv_fs_game.value : RGD3_BASE_GAMEDIR, relative, os_path, sizeof(os_path));
	rgd3_fs_create_parent_dirs(os_path);
	io = SDL_IOFromFile(os_path, "wb");
	if (!io)
	{
		rgd3_warning("couldn't open %s for writing: %s", os_path, SDL_GetError());
		return NULL;
	}
	if (cv_fs_debug.integer_value)
		rgd3_printf("rgd3_fs_open_write: %s\n", os_path);
	f = rgd3_fs_new_file(RGD3_FILE_OS, os_path);
	f->io = io;
	f->writable = 1;
	return f;
}

size_t rgd3_fs_read(Rgd3File* f, void* dst, size_t size)
{
	size_t avail, got = 0;
	if (!f || f->writable)
		return 0;
	if (f->kind == RGD3_FILE_OS)
		return SDL_ReadIO(f->io, dst, size);
	avail = f->pos < f->length ? (size_t)(f->length - f->pos) : 0;
	if (size > avail)
		size = avail;
	if (!size)
		return 0;
	if (f->kind == RGD3_FILE_MEMORY)
	{
		memcpy(dst, f->memory + f->pos, size);
		got = size;
	}
	else if (rgd3_fs_container_read_at(f->container, f->base + (u64)f->pos, dst, size))
	{
		got = size;
	}
	f->pos += (i64)got;
	return got;
}

size_t rgd3_fs_write(Rgd3File* f, const void* src, size_t size)
{
	if (!f || !f->writable)
		return 0;
	return SDL_WriteIO(f->io, src, size);
}

int rgd3_fs_printf(Rgd3File* f, const char* fmt, ...)
{
	char buf[RGD3_MAX_PRINT_MSG];
	va_list args;
	int len;
	va_start(args, fmt);
	len = rg_vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);
	if (len < 0)
		return 0;
	if (len >= (int)sizeof(buf))
		len = (int)sizeof(buf) - 1;
	return (int)rgd3_fs_write(f, buf, (size_t)len);
}

i64 rgd3_fs_length(Rgd3File* f)
{
	if (!f)
		return 0;
	if (f->kind == RGD3_FILE_OS)
		return (i64)SDL_GetIOSize(f->io);
	return f->length;
}

i64 rgd3_fs_tell(Rgd3File* f)
{
	if (!f)
		return 0;
	if (f->kind == RGD3_FILE_OS)
		return (i64)SDL_TellIO(f->io);
	return f->pos;
}

int rgd3_fs_seek(Rgd3File* f, i64 offset, Rgd3FsSeek whence)
{
	i64 target;
	if (!f)
		return 0;
	if (f->kind == RGD3_FILE_OS)
	{
		SDL_IOWhence w = whence == RGD3_FS_SEEK_SET ? SDL_IO_SEEK_SET : (whence == RGD3_FS_SEEK_CUR ? SDL_IO_SEEK_CUR : SDL_IO_SEEK_END);
		return SDL_SeekIO(f->io, offset, w) >= 0;
	}
	target = whence == RGD3_FS_SEEK_SET ? offset : (whence == RGD3_FS_SEEK_CUR ? f->pos + offset : f->length + offset);
	if (target < 0 || target > f->length)
		return 0;
	f->pos = target;
	return 1;
}

const char* rgd3_fs_file_name(Rgd3File* f)
{
	return f ? f->name : "";
}

void rgd3_fs_close(Rgd3File* f)
{
	if (!f)
		return;
	if (f->io)
		SDL_CloseIO(f->io);
	rgd3_heap_free(f);
}

int rgd3_fs_read_file(const char* relative, void** buffer, Rgd3TimeStamp* timestamp)
{
	Rgd3File* f;
	i64 len;
	u8* data;
	if (buffer)
		*buffer = NULL;
	if (timestamp)
		*timestamp = RGD3_FILE_NOT_FOUND_TIMESTAMP;
	f = rgd3_fs_open_read(relative);
	if (!f)
		return -1;
	len = rgd3_fs_length(f);
	if (timestamp)
	{
		SDL_PathInfo info;
		*timestamp = 0;
		if (f->kind == RGD3_FILE_OS && SDL_GetPathInfo(f->name, &info))
			*timestamp = (Rgd3TimeStamp)(info.modify_time / SDL_NS_PER_SECOND);
	}
	if (!buffer)
	{
		rgd3_fs_close(f);
		return (int)len;
	}
	data = (u8*)rgd3_heap_alloc((size_t)len + 1, RGD3_TAG_FILE);
	if (rgd3_fs_read(f, data, (size_t)len) != (size_t)len)
	{
		rgd3_warning("rgd3_fs_read_file: short read on %s", relative);
		rgd3_heap_free(data);
		rgd3_fs_close(f);
		return -1;
	}
	data[len] = 0;
	rgd3_fs_close(f);
	*buffer = data;
	return (int)len;
}

void rgd3_fs_free_file(void* buffer)
{
	rgd3_heap_free(buffer);
}

int rgd3_fs_write_file(const char* relative, const void* data, size_t size)
{
	Rgd3File* f = rgd3_fs_open_write(relative);
	size_t written;
	if (!f)
		return 0;
	written = rgd3_fs_write(f, data, size);
	rgd3_fs_close(f);
	return written == size;
}

int rgd3_fs_file_exists(const char* relative)
{
	Rgd3File* f = rgd3_fs_open_read(relative);
	if (!f)
		return 0;
	rgd3_fs_close(f);
	return 1;
}

/* ---------------------------------------------------------------------------
   Listing
--------------------------------------------------------------------------- */

typedef struct Rgd3FsListBuilder
{
	char** names;
	int count;
	int capacity;
} Rgd3FsListBuilder;

static void rgd3_fs_list_push(Rgd3FsListBuilder* b, const char* name)
{
	if (b->count == b->capacity)
	{
		b->capacity = b->capacity ? b->capacity * 2 : 64;
		b->names = (char**)rgd3_heap_realloc(b->names, sizeof(char*) * (size_t)b->capacity, RGD3_TAG_FILE);
	}
	b->names[b->count++] = rgd3_heap_strdup(name, RGD3_TAG_FILE);
}

/* Accepts rest (path below dir) under the listing rules; writes the reported
   name into out. ext "/" requests directories. */
static int rgd3_fs_list_accept(const char* rest, const char* ext, int tree, char* out, size_t size)
{
	const char* slash = strchr(rest, '/');
	if (!rest[0])
		return 0;
	if (strcmp(ext, "/") == 0)
	{
		if (!slash)
			return 0;
		rgd3_str_copynz(out, rest, size);
		if ((size_t)(slash - rest) < size)
			out[slash - rest] = 0;
		return 1;
	}
	if (slash && !tree)
		return 0;
	if (ext[0] && !rgd3_str_iends_with(rest, ext))
		return 0;
	rgd3_str_copynz(out, rest, size);
	return 1;
}

typedef struct Rgd3FsEnumContext
{
	Rgd3FsListBuilder* builder;
	const char* ext;
	const char* dir;
	char prefix[RGD3_MAX_OSPATH];
	int tree;
	int full_relative;
} Rgd3FsEnumContext;

static void rgd3_fs_list_loose(const char* os_dir, Rgd3FsEnumContext* ctx);

static SDL_EnumerationResult SDLCALL rgd3_fs_enum_callback(void* user, const char* dirname, const char* fname)
{
	Rgd3FsEnumContext* ctx = (Rgd3FsEnumContext*)user;
	char os_path[RGD3_MAX_OSPATH];
	char rest[RGD3_MAX_OSPATH];
	char name[RGD3_MAX_OSPATH];
	SDL_PathInfo info;
	rg_snprintf(os_path, sizeof(os_path), "%s%s", dirname, fname);
	rgd3_str_backslashes_to_slashes(os_path);
	if (!SDL_GetPathInfo(os_path, &info))
		return SDL_ENUM_CONTINUE;
	rg_snprintf(rest, sizeof(rest), "%s%s", ctx->prefix, fname);
	if (info.type == SDL_PATHTYPE_DIRECTORY)
	{
		if (strcmp(ctx->ext, "/") == 0)
		{
			rgd3_fs_list_push(ctx->builder, ctx->full_relative ? rgd3_va("%s/%s", ctx->dir, rest) : rest);
		}
		else if (ctx->tree)
		{
			char saved[RGD3_MAX_OSPATH];
			rgd3_str_copynz(saved, ctx->prefix, sizeof(saved));
			rg_snprintf(ctx->prefix, sizeof(ctx->prefix), "%s/", rest);
			rgd3_fs_list_loose(os_path, ctx);
			rgd3_str_copynz(ctx->prefix, saved, sizeof(ctx->prefix));
		}
		return SDL_ENUM_CONTINUE;
	}
	if (strcmp(ctx->ext, "/") != 0 && rgd3_fs_list_accept(fname, ctx->ext, 0, name, sizeof(name)))
		rgd3_fs_list_push(ctx->builder, ctx->full_relative && ctx->dir[0] ? rgd3_va("%s/%s", ctx->dir, rest) : rest);
	return SDL_ENUM_CONTINUE;
}

static void rgd3_fs_list_loose(const char* os_dir, Rgd3FsEnumContext* ctx)
{
	if (rgd3_platform_is_dir(os_dir))
		SDL_EnumerateDirectory(os_dir, rgd3_fs_enum_callback, ctx);
}

Rgd3FileList* rgd3_fs_list_files(const char* dir, const char* ext, int full_relative, int tree)
{
	Rgd3FsListBuilder b = {0};
	Rgd3FileList* list;
	char norm_dir[RGD3_MAX_OSPATH];
	size_t dir_len;
	int i, out;
	if (!ext)
		ext = "";
	rgd3_fs_normalize(dir ? dir : "", norm_dir, sizeof(norm_dir));
	rgd3_str_strip_trailing_slash(norm_dir);
	dir_len = strlen(norm_dir);

	for (i = 0; i < rgd3_fs.num_containers; ++i)
	{
		const Rgd3ResourceContainer* c = rgd3_fs.containers[i];
		u32 e;
		for (e = 0; e < c->count; ++e)
		{
			const char* name = c->entries[e].name;
			char accepted[RGD3_MAX_OSPATH];
			const char* rest;
			if (dir_len)
			{
				if (strncmp(name, norm_dir, dir_len) != 0 || name[dir_len] != '/')
					continue;
				rest = name + dir_len + 1;
			}
			else
			{
				rest = name;
			}
			if (!rgd3_fs_list_accept(rest, ext, tree, accepted, sizeof(accepted)))
				continue;
			rgd3_fs_list_push(&b, full_relative && dir_len ? rgd3_va("%s/%s", norm_dir, accepted) : accepted);
		}
	}

	for (i = 0; i < rgd3_fs.num_paths; ++i)
	{
		Rgd3FsEnumContext ctx;
		char os_dir[RGD3_MAX_OSPATH];
		memset(&ctx, 0, sizeof(ctx));
		ctx.builder = &b;
		ctx.ext = ext;
		ctx.dir = norm_dir;
		ctx.tree = tree;
		ctx.full_relative = full_relative;
		rgd3_fs_build_os_path(rgd3_fs.paths[i].path, rgd3_fs.paths[i].gamedir, norm_dir, os_dir, sizeof(os_dir));
		rgd3_str_strip_trailing_slash(os_dir);
		rgd3_fs_list_loose(os_dir, &ctx);
	}

	if (b.count)
		qsort(b.names, (size_t)b.count, sizeof(char*), rgd3_fs_sort_names);
	out = 0;
	for (i = 0; i < b.count; ++i)
	{
		if (out && rgd3_str_icmp(b.names[out - 1], b.names[i]) == 0)
		{
			rgd3_heap_free(b.names[i]);
			continue;
		}
		b.names[out++] = b.names[i];
	}
	list = (Rgd3FileList*)rgd3_heap_calloc(sizeof(Rgd3FileList), RGD3_TAG_FILE);
	list->count = out;
	list->names = b.names;
	return list;
}

void rgd3_fs_free_file_list(Rgd3FileList* list)
{
	int i;
	if (!list)
		return;
	for (i = 0; i < list->count; ++i)
		rgd3_heap_free(list->names[i]);
	rgd3_heap_free(list->names);
	rgd3_heap_free(list);
}

/* ---------------------------------------------------------------------------
   Init / shutdown
--------------------------------------------------------------------------- */

static int rgd3_fs_has_base_data(const char* root)
{
	char probe[RGD3_MAX_OSPATH];
	rg_snprintf(probe, sizeof(probe), "%s/" RGD3_BASE_GAMEDIR "/_common.resources", root);
	return rgd3_platform_is_file(probe);
}

static int rgd3_fs_find_basepath(char* out, size_t size)
{
	const char* env = SDL_getenv("RGD3_BASEPATH");
	if (cv_fs_basepath.value[0])
	{
		rgd3_str_copynz(out, cv_fs_basepath.value, size);
		rgd3_platform_normalize_path(out);
		return 1;
	}
	if (env && env[0])
	{
		rgd3_str_copynz(out, env, size);
		rgd3_platform_normalize_path(out);
		return 1;
	}
	if (rgd3_fs_has_base_data(rgd3_platform_cwd()))
	{
		rgd3_str_copynz(out, rgd3_platform_cwd(), size);
		return 1;
	}
	if (rgd3_fs_has_base_data(rgd3_platform_exe_dir()))
	{
		rgd3_str_copynz(out, rgd3_platform_exe_dir(), size);
		return 1;
	}
	if (rgd3_platform_find_steam_install(out, size))
	{
		rgd3_printf("found the Steam install of " RGD3_STEAM_GAME_DIR "\n");
		return 1;
	}
	return 0;
}

static void rgd3_fs_add_search_path(const char* path, const char* gamedir)
{
	Rgd3SearchPath* sp;
	int i;
	for (i = 0; i < rgd3_fs.num_paths; ++i)
		if (rgd3_str_icmp(rgd3_fs.paths[i].path, path) == 0 && rgd3_str_icmp(rgd3_fs.paths[i].gamedir, gamedir) == 0)
			return;
	if (rgd3_fs.num_paths >= RGD3_MAX_SEARCH_PATHS)
		return;
	sp = &rgd3_fs.paths[rgd3_fs.num_paths++];
	rgd3_str_copynz(sp->path, path, sizeof(sp->path));
	rgd3_str_copynz(sp->gamedir, gamedir, sizeof(sp->gamedir));
}

static void rgd3_fs_add_game_directory(const char* gamedir)
{
	char dir[RGD3_MAX_OSPATH];
	int first = rgd3_fs.num_paths;
	int i;
	rgd3_fs_add_search_path(rgd3_fs_basepath(), gamedir);
	rgd3_fs_add_search_path(rgd3_fs_savepath(), gamedir);
	for (i = first; i < rgd3_fs.num_paths; ++i)
	{
		rgd3_fs_build_os_path(rgd3_fs.paths[i].path, rgd3_fs.paths[i].gamedir, "", dir, sizeof(dir));
		rgd3_str_strip_trailing_slash(dir);
		rgd3_fs_mount_directory(dir);
	}
}

int rgd3_fs_init(void)
{
	char base[RGD3_MAX_OSPATH];
	if (rgd3_fs.initialized)
		return 1;
	if (!rgd3_fs_find_basepath(base, sizeof(base)) || !rgd3_fs_has_base_data(base))
	{
		rgd3_error_fatal("Couldn't find the DOOM 3 BFG Edition data (base/_common.resources).\n"
			"Run with +set fs_basepath \"<install dir>\" or set RGD3_BASEPATH.");
	}
	rgd3_cvar_set(&cv_fs_basepath, base, 1);
	if (!cv_fs_savepath.value[0])
		rgd3_cvar_set(&cv_fs_savepath, rgd3_platform_pref_path(), 1);

	rgd3_printf("------ Initializing File System ------\n");
	rgd3_fs_add_game_directory(RGD3_BASE_GAMEDIR);
	if (cv_fs_game.value[0] && rgd3_str_icmp(cv_fs_game.value, RGD3_BASE_GAMEDIR) != 0)
		rgd3_fs_add_game_directory(cv_fs_game.value);
	rgd3_fs.initialized = 1;
	{
		int i;
		rgd3_printf("Current search path:\n");
		for (i = rgd3_fs.num_paths - 1; i >= 0; --i)
			rgd3_printf("  %s/%s\n", rgd3_fs.paths[i].path, rgd3_fs.paths[i].gamedir);
	}
	rgd3_printf("file system initialized.\n");
	rgd3_printf("--------------------------------------\n");
	return 1;
}

void rgd3_fs_shutdown(void)
{
	int i;
	for (i = 0; i < rgd3_fs.num_containers; ++i)
		rgd3_fs_container_free(rgd3_fs.containers[i]);
	memset(&rgd3_fs, 0, sizeof(rgd3_fs));
}

/* ---------------------------------------------------------------------------
   Commands
--------------------------------------------------------------------------- */

static void rgd3_fs_cmd_dir_common(const Rgd3CmdArgs* args, int tree)
{
	const char* dir;
	const char* ext = "";
	Rgd3FileList* list;
	int i;
	int argc = rgd3_cmdargs_argc(args);
	if (argc < 2 || argc > 3)
	{
		rgd3_printf("usage: %s <directory> [extension]\n", rgd3_cmdargs_argv(args, 0));
		return;
	}
	dir = rgd3_cmdargs_argv(args, 1);
	if (argc == 3)
		ext = rgd3_cmdargs_argv(args, 2);
	rgd3_printf("Listing of %s/*%s%s\n", dir, ext, tree ? " /s" : "");
	rgd3_printf("---------------\n");
	list = rgd3_fs_list_files(dir, ext, 1, tree);
	for (i = 0; i < list->count; ++i)
		rgd3_printf("%s\n", list->names[i]);
	rgd3_printf("%d files\n", list->count);
	rgd3_fs_free_file_list(list);
}

static void rgd3_fs_cmd_dir(const Rgd3CmdArgs* args)
{
	rgd3_fs_cmd_dir_common(args, 0);
}

static void rgd3_fs_cmd_dirtree(const Rgd3CmdArgs* args)
{
	rgd3_fs_cmd_dir_common(args, 1);
}

static void rgd3_fs_cmd_path(const Rgd3CmdArgs* args)
{
	int i;
	RG_UNUSED(args);
	rgd3_printf("Current search path:\n");
	for (i = rgd3_fs.num_containers - 1; i >= 0; --i)
		rgd3_printf("  %s (%u files)\n", rgd3_fs.containers[i]->path, rgd3_fs.containers[i]->count);
	for (i = rgd3_fs.num_paths - 1; i >= 0; --i)
		rgd3_printf("  %s/%s\n", rgd3_fs.paths[i].path, rgd3_fs.paths[i].gamedir);
}

/* listResources [container substring] [name filter] */
static void rgd3_fs_cmd_list_resources(const Rgd3CmdArgs* args)
{
	const char* which = rgd3_cmdargs_argc(args) > 1 ? rgd3_cmdargs_argv(args, 1) : NULL;
	char match[RGD3_MAX_COMMAND_STRING];
	const char* filter = rgd3_cmdargs_match(args, 2, match, sizeof(match));
	int i;
	for (i = 0; i < rgd3_fs.num_containers; ++i)
	{
		const Rgd3ResourceContainer* c = rgd3_fs.containers[i];
		char lower_name[RGD3_MAX_OSPATH];
		char lower_which[RGD3_MAX_OSPATH];
		u32 e, listed = 0;
		rgd3_str_copynz(lower_name, rgd3_str_file_name(c->path), sizeof(lower_name));
		rgd3_str_to_lower(lower_name);
		if (!which)
		{
			rgd3_printf("%-40s %8u entries%s\n", rgd3_str_file_name(c->path), c->count, c->memory ? " (in memory)" : "");
			continue;
		}
		rgd3_str_copynz(lower_which, which, sizeof(lower_which));
		rgd3_str_to_lower(lower_which);
		if (!strstr(lower_name, lower_which))
			continue;
		rgd3_printf("%s:\n%10s %10s  %s\n", c->path, "offset", "length", "name");
		for (e = 0; e < c->count; ++e)
		{
			if (filter && !rgd3_str_filter(filter, c->entries[e].name, 0))
				continue;
			rgd3_printf("%10u %10u  %s\n", c->entries[e].offset, c->entries[e].length, c->entries[e].name);
			++listed;
		}
		rgd3_printf("%u entries listed (%u in container)\n", listed, c->count);
	}
}

static void rgd3_fs_cmd_extract(const Rgd3CmdArgs* args)
{
	void* data = NULL;
	int len;
	const char* name;
	const char* dest;
	if (rgd3_cmdargs_argc(args) < 2)
	{
		rgd3_printf("usage: extractResourceFile <name> [destination]\n");
		return;
	}
	name = rgd3_cmdargs_argv(args, 1);
	dest = rgd3_cmdargs_argc(args) > 2 ? rgd3_cmdargs_argv(args, 2) : rgd3_va("extracted/%s", name);
	len = rgd3_fs_read_file(name, &data, NULL);
	if (len < 0)
	{
		rgd3_printf("couldn't find %s\n", name);
		return;
	}
	if (rgd3_fs_write_file(dest, data, (size_t)len))
		rgd3_printf("wrote %d bytes to %s\n", len, dest);
	rgd3_fs_free_file(data);
}

void rgd3_fs_register(void)
{
	static Rgd3Cvar* const cvars[] = {
		&cv_fs_basepath, &cv_fs_savepath, &cv_fs_game, &cv_fs_debug, &cv_fs_resourceLoadPriority
	};
	rgd3_cvar_register_table(cvars, (int)RG_ARRAY_COUNT(cvars));
	rgd3_cmd_add("dir", rgd3_fs_cmd_dir, RGD3_CMD_FL_SYSTEM, "lists a folder", NULL);
	rgd3_cmd_add("dirtree", rgd3_fs_cmd_dirtree, RGD3_CMD_FL_SYSTEM, "lists a folder with subfolders", NULL);
	rgd3_cmd_add("path", rgd3_fs_cmd_path, RGD3_CMD_FL_SYSTEM, "lists search paths", NULL);
	rgd3_cmd_add("listResources", rgd3_fs_cmd_list_resources, RGD3_CMD_FL_SYSTEM, "lists resource containers, or the entries of one: listResources <container> [filter]", NULL);
	rgd3_cmd_add("extractResourceFile", rgd3_fs_cmd_extract, RGD3_CMD_FL_SYSTEM, "copies a file out of the resource containers into fs_savepath", NULL);
}

/* ---- per-map containers (idFileSystemLocal::BeginLevelLoad / UnloadMapResources) ---- */

static char rgd3_fs_map_container[RGD3_MAX_OSPATH];

/* Mounts maps/<map file name>.resources for "game/mars_city1" style names,
   replacing the previous map's container. Returns 0 if there is none. */
int rgd3_fs_load_map_resources(const char* map_name)
{
	char relative[RGD3_MAX_OSPATH];
	char os_path[RGD3_MAX_OSPATH];
	char base[RGD3_MAX_OSPATH];
	char* dot;

	rgd3_str_copynz(base, rgd3_str_file_name(map_name), sizeof(base));
	dot = strrchr(base, '.');
	if (dot)
		*dot = 0;
	rg_snprintf(relative, sizeof(relative), "maps/%s.resources", base);
	if (rgd3_fs_map_container[0] && !rgd3_str_icmp(rgd3_fs_map_container, rgd3_str_file_name(relative)))
		return 1; /* already mounted */
	rgd3_fs_unload_map_resources();
	rgd3_fs_build_os_path(rgd3_fs_basepath(), RGD3_BASE_GAMEDIR, relative, os_path, sizeof(os_path));
	if (!rgd3_platform_is_file(os_path))
		return 0;
	if (rgd3_fs_mount_container(os_path, 0) < 0) /* index on success, -1 on failure */
		return 0;
	rgd3_str_copynz(rgd3_fs_map_container, rgd3_str_file_name(relative), sizeof(rgd3_fs_map_container));
	return 1;
}

void rgd3_fs_unload_map_resources(void)
{
	if (rgd3_fs_map_container[0])
		rgd3_fs_unmount_container(rgd3_fs_map_container);
	rgd3_fs_map_container[0] = 0;
}
