/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_savegame.c - see rgd3_savegame.h. Taken from the original:
	- neo/sys/sys_savegame.h/.cpp: saveGameError_t bits and their names
	  (GetSaveGameErrorString, savegamePrintErrors), saveGameType_t bits,
	  SAVEGAME_DETAIL_FIELD_* keys, MAX_FILES_WITHIN_SAVEGAME,
	  MAX_FOLDER_NAME_LENGTH, SavegameReadDetailsFromFile (checksum check,
	  ignore_checksum), the saveGame_verbose/_checksum/_enable cvars;
	- neo/sys/sys_session_savegames.cpp: game.details written as INI next to
	  the files (SaveGameSync), details checked on load
	  (LoadGameCheckDescriptionFile: missing -> FILE_NOT_FOUND, damaged ->
	  CORRUPTED, newer saveVersion -> INCOMPATIBLE_NEWER_VERSION),
	  savegame_error, testSavegameEnumerate / testSavegameDelete /
	  savegameSetErrorBit and the OutputDetailList format;
	- neo/sys/win32/win_savegame.cpp: write each file to a temporary name and
	  rename it, missing non-optional files load as CORRUPTED, write errors
	  as INSUFFICIENT_ROOM, open errors as UNKNOWN, enumeration by the
	  game.details files with the date from its modify time, delete removes
	  the folder's files then the folder, savegame_winInduceDelay;
	- neo/framework/Common_load.cpp: ScrubSaveGameFileName, language and
	  checksum added to the details at save time.
   New for rgd3: the container format (the original pipelined raw deflate
   with MD5 block checksums and no header is replaced), one SDL writer
   thread with newest-wins replacement instead of the processor queue,
   synchronous load/enumerate/delete on the main thread. */

static Rgd3Cvar cv_saveGame_verbose = RGD3_CVAR("saveGame_verbose", "0", RGD3_CVAR_BOOL | RGD3_CVAR_ARCHIVE, "debug spam");
static Rgd3Cvar cv_saveGame_checksum = RGD3_CVAR("saveGame_checksum", "1", RGD3_CVAR_BOOL, "data integrity check");
static Rgd3Cvar cv_saveGame_enable = RGD3_CVAR("saveGame_enable", "1", RGD3_CVAR_BOOL, "are savegames enabled");
static Rgd3Cvar cv_savegame_error = RGD3_CVAR("savegame_error", "0", RGD3_CVAR_INTEGER, "Combination of bits that will simulate and error, see 'savegamePrintErrors'.  0 = no error");
static Rgd3Cvar cv_savegame_winInduceDelay = RGD3_CVAR("savegame_winInduceDelay", "0", RGD3_CVAR_INTEGER, "on windows, this is a delay induced before any file operation occurs");

static const char* const rgd3_savegame_error_names[RGD3_SAVEGAME_E_BITS_USED] = {
	"SAVEGAME_E_CANCELLED",
	"SAVEGAME_E_INSUFFICIENT_ROOM",
	"SAVEGAME_E_CORRUPTED",
	"SAVEGAME_E_UNABLE_TO_SELECT_STORAGE_DEVICE",
	"SAVEGAME_E_UNKNOWN",
	"SAVEGAME_E_INVALID_FILENAME",
	"SAVEGAME_E_STEAM_ERROR",
	"SAVEGAME_E_FOLDER_NOT_FOUND",
	"SAVEGAME_E_FILE_NOT_FOUND",
	"SAVEGAME_E_DLC_NOT_FOUND",
	"SAVEGAME_E_INVALID_USER",
	"SAVEGAME_E_PROFILE_TOO_BIG",
	"SAVEGAME_E_DISC_SWAP",
	"SAVEGAME_E_INCOMPATIBLE_NEWER_VERSION"
};

#define RGD3_SAVEGAME_BUILD_STRING RGD3_ENGINE_NAME " " RGD3_ENGINE_VERSION " " RGD3_BUILD_MODE
#define RGD3_SAVEGAME_MAX_PAYLOAD ((u64)1 << 30)        /* sanity bound on a decoded file */
#define RGD3_SAVEGAME_MAX_BLOCK_SIZE (16u * 1024u * 1024u)
#define RGD3_SAVEGAME_ZLIB_QUALITY 6                   /* stb hash chain length */
#define RGD3_SAVEGAME_NUM_RESULTS 64

typedef struct Rgd3SaveJobFile
{
	char name[RGD3_SAVEGAME_MAX_FILE_NAME];
	int type;
	u8* data; /* RGD3_TAG_SAVEGAMES copy, freed by the writer once written */
	size_t size;
} Rgd3SaveJobFile;

typedef struct Rgd3SaveJob
{
	struct Rgd3SaveJob* next;
	Rgd3SaveHandle handle;
	char slot[RGD3_SAVEGAME_MAX_FOLDER_NAME];
	char os_dir[RGD3_MAX_OSPATH];
	int num_files;
	Rgd3SaveJobFile files[RGD3_SAVEGAME_MAX_FILES + 1]; /* + game.details */
	int delay_ms;        /* savegame_winInduceDelay at queue time */
	int simulated_error; /* savegame_error at queue time */
	int error;           /* result */
	char message[RGD3_MAX_OSPATH + 128]; /* the writer's description of a failure */
} Rgd3SaveJob;

typedef struct Rgd3SaveResult
{
	Rgd3SaveHandle handle;
	int error;
} Rgd3SaveResult;

typedef struct Rgd3SaveGameState
{
	int initialized;
	SDL_Mutex* lock;
	SDL_Condition* work; /* a job was queued, or quit */
	SDL_Condition* idle; /* a job finished */
	SDL_Thread* thread;
	int quit;
	Rgd3SaveJob* queue_head;
	Rgd3SaveJob* queue_tail;
	Rgd3SaveJob* in_flight;
	Rgd3SaveJob* done_head;
	Rgd3SaveJob* done_tail;
	Rgd3SaveHandle next_handle;
	Rgd3SaveResult results[RGD3_SAVEGAME_NUM_RESULTS];
	int next_result;
	Rgd3SaveCompletedFn callback;
	void* callback_user;
} Rgd3SaveGameState;

static Rgd3SaveGameState rgd3_savegame;
static int rgd3_savegame_quiet; /* self-test: expected failures are not printed */

/* ---------------------------------------------------------------------------
   Names and paths
--------------------------------------------------------------------------- */

const char* rgd3_savegame_error_string(int error_mask, char* buf, size_t size)
{
	int i;
	if (!size)
		return buf;
	buf[0] = 0;
	for (i = 0; i < RGD3_SAVEGAME_E_BITS_USED; i++)
	{
		if (error_mask & (1 << i))
		{
			if (buf[0])
				rgd3_str_append(buf, size, ", ");
			rgd3_str_append(buf, size, rgd3_savegame_error_names[i]);
		}
	}
	return buf;
}

void rgd3_savegame_scrub_name(const char* in, char* out, size_t size)
{
	char name[RGD3_MAX_STRING_CHARS];
	char* dot;
	size_t o = 0;
	int i;
	if (!size)
		return;
	rgd3_str_copynz(name, in ? in : "", sizeof(name));
	rgd3_str_remove_colors(name);
	dot = strrchr(name, '.');
	if (dot)
		*dot = 0;
	for (i = 0; name[i] && o + 1 < size; i++)
	{
		unsigned char c = (unsigned char)name[i];
		if (strchr("',.~!@#$%^&*()[]{}<>\\|/=?+;:-'\"", c) || c >= 128 || c == ' ')
			out[o++] = '_'; /* random junk, high ascii, spaces */
		else
			out[o++] = (char)c;
	}
	out[o] = 0;
}

/* slot and file names: non-empty, shorter than max, no path or reserved
   characters, no leading '.', no trailing '.' or ' ' */
static int rgd3_savegame_valid_name(const char* name, size_t max)
{
	size_t len;
	const char* p;
	if (!name || !name[0])
		return 0;
	len = strlen(name);
	if (len >= max || name[0] == '.' || name[len - 1] == '.' || name[len - 1] == ' ')
		return 0;
	for (p = name; *p; p++)
	{
		unsigned char c = (unsigned char)*p;
		if (c < 32 || strchr("/\\:*?\"<>|", c))
			return 0;
	}
	return 1;
}

static int rgd3_savegame_saves_dir(char* out, size_t size)
{
	return rgd3_fs_write_os_path(RGD3_SAVEGAME_DIR, out, size);
}

static int rgd3_savegame_slot_dir(const char* slot, char* out, size_t size)
{
	char relative[RGD3_MAX_OSPATH];
	rg_snprintf(relative, sizeof(relative), RGD3_SAVEGAME_DIR "/%s", slot);
	return rgd3_fs_write_os_path(relative, out, size);
}

static void rgd3_savegame_report(const char* what, const char* slot, int error, const char* detail)
{
	char names[512];
	if (rgd3_savegame_quiet)
		return;
	rgd3_savegame_error_string(error, names, sizeof(names));
	rgd3_warning("%s '%s' failed: 0x%08X %s%s%s", what, slot ? slot : "", (unsigned)error, names,
		detail && detail[0] ? " - " : "", detail ? detail : "");
}

/* ---------------------------------------------------------------------------
   Details
--------------------------------------------------------------------------- */

void rgd3_savegame_details_clear(Rgd3SaveGameDetails* details)
{
	rgd3_dict_clear(&details->descriptors);
	details->damaged = 0;
	details->date = 0;
	details->slot_name[0] = 0;
}

void rgd3_savegame_details_copy(Rgd3SaveGameDetails* dst, const Rgd3SaveGameDetails* src)
{
	if (dst == src)
		return;
	rgd3_dict_assign(&dst->descriptors, &src->descriptors);
	dst->damaged = src->damaged;
	dst->date = src->date;
	rgd3_str_copynz(dst->slot_name, src->slot_name, sizeof(dst->slot_name));
}

void rgd3_savegame_details_read(Rgd3SaveGameDetails* details, Rgd3File* file)
{
	details->damaged = 0;
	if (!rgd3_dict_read_ini(&details->descriptors, file))
		details->damaged = 1;
	if (!rgd3_dict_get_bool(&details->descriptors, "ignore_checksum", 0))
	{
		int read_checksum = rgd3_dict_get_int(&details->descriptors, RGD3_SAVEGAME_DETAIL_FIELD_CHECKSUM, 0);
		int checksum;
		rgd3_dict_delete(&details->descriptors, RGD3_SAVEGAME_DETAIL_FIELD_CHECKSUM);
		checksum = rgd3_dict_checksum(&details->descriptors);
		if (read_checksum == 0 || checksum != read_checksum)
			details->damaged = 1;
	}
}

const char* rgd3_savegame_details_map_name(const Rgd3SaveGameDetails* details)
{
	return rgd3_dict_get_string(&details->descriptors, RGD3_SAVEGAME_DETAIL_FIELD_MAP, "");
}

const char* rgd3_savegame_details_language(const Rgd3SaveGameDetails* details)
{
	return rgd3_dict_get_string(&details->descriptors, RGD3_SAVEGAME_DETAIL_FIELD_LANGUAGE, "");
}

int rgd3_savegame_details_playtime(const Rgd3SaveGameDetails* details)
{
	return rgd3_dict_get_int(&details->descriptors, RGD3_SAVEGAME_DETAIL_FIELD_PLAYTIME, 0);
}

int rgd3_savegame_details_save_version(const Rgd3SaveGameDetails* details)
{
	return rgd3_dict_get_int(&details->descriptors, RGD3_SAVEGAME_DETAIL_FIELD_SAVE_VERSION, 0);
}

static i64 rgd3_savegame_path_date(const char* os_path)
{
	SDL_PathInfo info;
	if (!SDL_GetPathInfo(os_path, &info))
		return 0;
	return (i64)(info.modify_time / SDL_NS_PER_SECOND);
}

/* ---------------------------------------------------------------------------
   Container
--------------------------------------------------------------------------- */

typedef struct Rgd3SaveBuf
{
	u8* data;
	size_t size;
	size_t capacity;
} Rgd3SaveBuf;

static void rgd3_savebuf_put(Rgd3SaveBuf* b, const void* src, size_t n)
{
	if (b->size + n > b->capacity)
	{
		size_t cap = b->capacity ? b->capacity : 4096;
		while (cap < b->size + n)
			cap *= 2;
		b->data = (u8*)rgd3_heap_realloc(b->data, cap, RGD3_TAG_SAVEGAMES);
		b->capacity = cap;
	}
	memcpy(b->data + b->size, src, n);
	b->size += n;
}

static void rgd3_savebuf_u32(Rgd3SaveBuf* b, u32 v)
{
	u8 x[4];
	x[0] = (u8)v;
	x[1] = (u8)(v >> 8);
	x[2] = (u8)(v >> 16);
	x[3] = (u8)(v >> 24);
	rgd3_savebuf_put(b, x, 4);
}

/* Any thread: uses only the tagged heap, stb's deflate (malloc) and CRC-32
   (its table is built in rgd3_savegame_init before the writer starts). */
u8* rgd3_savegame_encode(const void* data, size_t size, size_t* out_size)
{
	Rgd3SaveBuf b = { 0 };
	const u8* src = (const u8*)data;
	const char* build = RGD3_SAVEGAME_BUILD_STRING;
	u32 build_len = (u32)strlen(build);
	u64 count = ((u64)size + RGD3_SAVEGAME_BLOCK_SIZE - 1) / RGD3_SAVEGAME_BLOCK_SIZE;
	size_t offset = 0;

	*out_size = 0;
	if ((u64)size > RGD3_SAVEGAME_MAX_PAYLOAD)
		return NULL;
	if (build_len > 255)
		build_len = 255;
	b.capacity = 64 + build_len + size + size / 8 + (size_t)count * 12;
	b.data = (u8*)rgd3_heap_alloc(b.capacity, RGD3_TAG_SAVEGAMES);

	rgd3_savebuf_put(&b, RGD3_SAVEGAME_MAGIC, 8);
	rgd3_savebuf_u32(&b, RGD3_SAVEGAME_FORMAT_VERSION);
	rgd3_savebuf_u32(&b, build_len);
	rgd3_savebuf_put(&b, build, build_len);
	rgd3_savebuf_u32(&b, (u32)((u64)size & 0xFFFFFFFFu));
	rgd3_savebuf_u32(&b, (u32)((u64)size >> 32));
	rgd3_savebuf_u32(&b, RGD3_SAVEGAME_BLOCK_SIZE);
	rgd3_savebuf_u32(&b, (u32)count);

	while (offset < size)
	{
		size_t n = size - offset;
		int clen = 0;
		unsigned char* comp;
		if (n > RGD3_SAVEGAME_BLOCK_SIZE)
			n = RGD3_SAVEGAME_BLOCK_SIZE;
		comp = stbi_zlib_compress((unsigned char*)(src + offset), (int)n, &clen, RGD3_SAVEGAME_ZLIB_QUALITY);
		if (!comp || clen <= 0)
		{
			if (comp)
				STBIW_FREE(comp);
			rgd3_heap_free(b.data);
			return NULL;
		}
		rgd3_savebuf_u32(&b, (u32)clen);
		rgd3_savebuf_u32(&b, (u32)n);
		rgd3_savebuf_u32(&b, rgd3_crc32_block(src + offset, (int)n));
		rgd3_savebuf_put(&b, comp, (size_t)clen);
		STBIW_FREE(comp);
		offset += n;
	}
	*out_size = b.size;
	return b.data;
}

int rgd3_savegame_decode(const u8* data, size_t size, Rgd3File* out, int verify_crc)
{
	Rgd3BinReader r;
	const u8* magic;
	u32 version, block_size, count, i;
	i32 build_len;
	u64 total, remaining;
	u8* block;
	int error = 0;

	if (!data || !out)
		return RGD3_SAVEGAME_E_CORRUPTED;
	rgd3_bin_reader_init(&r, data, size);
	magic = rgd3_bin_read_bytes(&r, 8);
	if (!magic || memcmp(magic, RGD3_SAVEGAME_MAGIC, 8) != 0)
		return RGD3_SAVEGAME_E_CORRUPTED;
	version = rgd3_bin_u32_le(&r);
	if (r.error || version == 0)
		return RGD3_SAVEGAME_E_CORRUPTED;
	if (version > RGD3_SAVEGAME_FORMAT_VERSION)
		return RGD3_SAVEGAME_E_INCOMPATIBLE_NEWER_VERSION;
	build_len = rgd3_bin_i32_le(&r);
	if (build_len < 0 || build_len > 255)
		return RGD3_SAVEGAME_E_CORRUPTED;
	rgd3_bin_skip(&r, (size_t)build_len);
	total = rgd3_bin_u32_le(&r);
	total |= (u64)rgd3_bin_u32_le(&r) << 32;
	block_size = rgd3_bin_u32_le(&r);
	count = rgd3_bin_u32_le(&r);
	if (r.error || total > RGD3_SAVEGAME_MAX_PAYLOAD || block_size == 0 || block_size > RGD3_SAVEGAME_MAX_BLOCK_SIZE)
		return RGD3_SAVEGAME_E_CORRUPTED;
	if ((u64)count != (total + block_size - 1) / block_size)
		return RGD3_SAVEGAME_E_CORRUPTED;
	/* every block needs at least its 12-byte header and one byte */
	if ((u64)count * 13u > (u64)rgd3_bin_remaining(&r))
		return RGD3_SAVEGAME_E_CORRUPTED;
	/* deflate cannot expand beyond about 1032:1, so a damaged size field
	   never makes the reservation below larger than that */
	if (total > (u64)rgd3_bin_remaining(&r) * 1032u + 1024u)
		return RGD3_SAVEGAME_E_CORRUPTED;
	if (!count)
		return rgd3_bin_remaining(&r) ? RGD3_SAVEGAME_E_CORRUPTED : 0;

	rgd3_fs_memory_reserve(out, (size_t)(rgd3_fs_tell(out) + (i64)total + 1));
	block = (u8*)rgd3_heap_alloc(block_size, RGD3_TAG_SAVEGAMES);
	remaining = total;
	for (i = 0; i < count && !error; i++)
	{
		u32 clen = rgd3_bin_u32_le(&r);
		u32 ulen = rgd3_bin_u32_le(&r);
		u32 crc = rgd3_bin_u32_le(&r);
		u64 expected = remaining < block_size ? remaining : block_size;
		const u8* src;
		size_t got;
		if (r.error || ulen != expected || clen == 0 || clen > rgd3_bin_remaining(&r))
		{
			error = RGD3_SAVEGAME_E_CORRUPTED;
			break;
		}
		src = rgd3_bin_read_bytes(&r, clen);
		got = tinfl_decompress_mem_to_mem(block, ulen, src, clen, TINFL_FLAG_PARSE_ZLIB_HEADER);
		if (got == TINFL_DECOMPRESS_MEM_TO_MEM_FAILED || got != ulen)
		{
			error = RGD3_SAVEGAME_E_CORRUPTED;
			break;
		}
		if (verify_crc && rgd3_crc32_block(block, (int)ulen) != crc)
		{
			error = RGD3_SAVEGAME_E_CORRUPTED;
			break;
		}
		if (rgd3_fs_write(out, block, ulen) != ulen)
		{
			error = RGD3_SAVEGAME_E_UNKNOWN;
			break;
		}
		remaining -= ulen;
	}
	if (!error && (remaining || rgd3_bin_remaining(&r)))
		error = RGD3_SAVEGAME_E_CORRUPTED;
	rgd3_heap_free(block);
	return error;
}

/* ---------------------------------------------------------------------------
   Writer thread: plain SDL IO on paths computed by the main thread; no
   cvars, decls, commands, rgd3_fs calls or printing.
--------------------------------------------------------------------------- */

static void rgd3_savegame_job_free_data(Rgd3SaveJob* job)
{
	int i;
	for (i = 0; i < job->num_files; i++)
	{
		rgd3_heap_free(job->files[i].data);
		job->files[i].data = NULL;
		job->files[i].size = 0;
	}
}

static void rgd3_savegame_job_free(Rgd3SaveJob* job)
{
	if (!job)
		return;
	rgd3_savegame_job_free_data(job);
	rgd3_heap_free(job);
}

static int rgd3_savegame_write_os_file(const char* path, const void* data, size_t size, char* message, size_t message_size)
{
	SDL_IOStream* io = SDL_IOFromFile(path, "wb");
	size_t written = 0;
	int closed;
	if (!io)
	{
		SDL_snprintf(message, message_size, "couldn't open %s for writing: %s", path, SDL_GetError());
		return RGD3_SAVEGAME_E_UNKNOWN;
	}
	if (size)
		written = SDL_WriteIO(io, data, size);
	closed = SDL_CloseIO(io);
	if (written != size || !closed)
	{
		SDL_snprintf(message, message_size, "write to %s failed: %s", path, SDL_GetError());
		return RGD3_SAVEGAME_E_INSUFFICIENT_ROOM;
	}
	return 0;
}

static void rgd3_savegame_run_job(Rgd3SaveJob* job)
{
	char path[RGD3_MAX_OSPATH];
	char tmp[RGD3_MAX_OSPATH];
	int i, attempted = 0;

	if (job->delay_ms > 0)
		SDL_Delay((Uint32)job->delay_ms);
	if (!SDL_CreateDirectory(job->os_dir))
	{
		SDL_snprintf(job->message, sizeof(job->message), "couldn't create %s: %s", job->os_dir, SDL_GetError());
		job->error |= RGD3_SAVEGAME_E_UNKNOWN;
	}

	/* everything to <name>.tmp first */
	for (i = 0; i < job->num_files && !job->error; i++)
	{
		Rgd3SaveJobFile* f = &job->files[i];
		const void* data = f->data;
		size_t size = f->size;
		u8* encoded = NULL;
		if (!(f->type & RGD3_SAVEGAMEFILE_TEXT))
		{
			encoded = rgd3_savegame_encode(f->data, f->size, &size);
			if (!encoded)
			{
				SDL_snprintf(job->message, sizeof(job->message), "couldn't compress %s", f->name);
				job->error |= RGD3_SAVEGAME_E_UNKNOWN;
				break;
			}
			data = encoded;
		}
		SDL_snprintf(tmp, sizeof(tmp), "%s/%s.tmp", job->os_dir, f->name);
		attempted = i + 1;
		job->error |= rgd3_savegame_write_os_file(tmp, data, size, job->message, sizeof(job->message));
		rgd3_heap_free(encoded);
		rgd3_heap_free(f->data);
		f->data = NULL;
	}
	rgd3_savegame_job_free_data(job);

	if (job->error)
	{
		for (i = 0; i < attempted; i++)
		{
			SDL_snprintf(tmp, sizeof(tmp), "%s/%s.tmp", job->os_dir, job->files[i].name);
			SDL_RemovePath(tmp);
		}
		return;
	}

	/* Then replace the old files, all or nothing: a slot must never mix files
	   from two saves (each file's CRC would still pass). Old files move to
	   <name>.bak, the new ones move in, and any failure puts the old slot
	   back exactly. game.details is last in the list. */
	{
		char bak[RGD3_MAX_OSPATH];
		int had_old[RGD3_SAVEGAME_MAX_FILES + 1] = { 0 };
		int backed_up = 0, moved_in = 0;

		for (i = 0; i < job->num_files && !job->error; i++)
		{
			SDL_snprintf(path, sizeof(path), "%s/%s", job->os_dir, job->files[i].name);
			SDL_snprintf(bak, sizeof(bak), "%s/%s.bak", job->os_dir, job->files[i].name);
			SDL_RemovePath(bak); /* left over from a crash */
			if (SDL_GetPathInfo(path, NULL))
			{
				if (!SDL_RenamePath(path, bak))
				{
					SDL_snprintf(job->message, sizeof(job->message), "couldn't move %s aside: %s", path, SDL_GetError());
					job->error |= RGD3_SAVEGAME_E_UNKNOWN;
					break;
				}
				had_old[i] = 1;
			}
			backed_up = i + 1;
		}
		for (i = 0; i < job->num_files && !job->error; i++)
		{
			SDL_snprintf(tmp, sizeof(tmp), "%s/%s.tmp", job->os_dir, job->files[i].name);
			SDL_snprintf(path, sizeof(path), "%s/%s", job->os_dir, job->files[i].name);
			if (!SDL_RenamePath(tmp, path))
			{
				SDL_snprintf(job->message, sizeof(job->message), "couldn't rename %s: %s", tmp, SDL_GetError());
				job->error |= RGD3_SAVEGAME_E_UNKNOWN;
				break;
			}
			moved_in = i + 1;
		}
		for (i = 0; i < job->num_files; i++)
		{
			SDL_snprintf(tmp, sizeof(tmp), "%s/%s.tmp", job->os_dir, job->files[i].name);
			SDL_snprintf(path, sizeof(path), "%s/%s", job->os_dir, job->files[i].name);
			SDL_snprintf(bak, sizeof(bak), "%s/%s.bak", job->os_dir, job->files[i].name);
			if (job->error)
			{
				/* roll back: new files out, old files back in */
				if (i < moved_in)
					SDL_RemovePath(path);
				if (i < backed_up && had_old[i])
					SDL_RenamePath(bak, path);
				SDL_RemovePath(tmp);
			}
			else if (had_old[i])
			{
				SDL_RemovePath(bak);
			}
		}
	}
}

static void rgd3_savegame_push_done(Rgd3SaveJob* job)
{
	/* lock held */
	job->next = NULL;
	if (rgd3_savegame.done_tail)
		rgd3_savegame.done_tail->next = job;
	else
		rgd3_savegame.done_head = job;
	rgd3_savegame.done_tail = job;
}

static int SDLCALL rgd3_savegame_thread(void* user)
{
	RG_UNUSED(user);
	SDL_LockMutex(rgd3_savegame.lock);
	for (;;)
	{
		Rgd3SaveJob* job;
		while (!rgd3_savegame.quit && !rgd3_savegame.queue_head)
			SDL_WaitCondition(rgd3_savegame.work, rgd3_savegame.lock);
		job = rgd3_savegame.queue_head;
		if (!job)
			break; /* quit with an empty queue */
		rgd3_savegame.queue_head = job->next;
		if (!rgd3_savegame.queue_head)
			rgd3_savegame.queue_tail = NULL;
		rgd3_savegame.in_flight = job;
		SDL_UnlockMutex(rgd3_savegame.lock);

		rgd3_savegame_run_job(job);

		SDL_LockMutex(rgd3_savegame.lock);
		rgd3_savegame.in_flight = NULL;
		rgd3_savegame_push_done(job);
		SDL_BroadcastCondition(rgd3_savegame.idle);
	}
	SDL_UnlockMutex(rgd3_savegame.lock);
	return 0;
}

/* lock held: queued (not yet writing) jobs for the slot complete as cancelled */
static void rgd3_savegame_cancel_queued(const char* slot)
{
	Rgd3SaveJob* prev = NULL;
	Rgd3SaveJob* job = rgd3_savegame.queue_head;
	while (job)
	{
		Rgd3SaveJob* next = job->next;
		if (rgd3_str_icmp(job->slot, slot) == 0)
		{
			if (prev)
				prev->next = next;
			else
				rgd3_savegame.queue_head = next;
			if (rgd3_savegame.queue_tail == job)
				rgd3_savegame.queue_tail = prev;
			rgd3_savegame_job_free_data(job);
			job->error = RGD3_SAVEGAME_E_CANCELLED;
			rgd3_savegame_push_done(job);
		}
		else
		{
			prev = job;
		}
		job = next;
	}
}

/* ---------------------------------------------------------------------------
   Manager (main thread)
--------------------------------------------------------------------------- */

int rgd3_savegame_init(void)
{
	u32 crc;
	if (rgd3_savegame.initialized)
		return 1;
	memset(&rgd3_savegame, 0, sizeof(rgd3_savegame));
	rgd3_crc32_init(&crc); /* builds the CRC table before another thread can race on it */
	rgd3_savegame.lock = SDL_CreateMutex();
	rgd3_savegame.work = SDL_CreateCondition();
	rgd3_savegame.idle = SDL_CreateCondition();
	if (!rgd3_savegame.lock || !rgd3_savegame.work || !rgd3_savegame.idle)
	{
		rgd3_warning("savegame: couldn't create the writer's mutex: %s", SDL_GetError());
		if (rgd3_savegame.idle)
			SDL_DestroyCondition(rgd3_savegame.idle);
		if (rgd3_savegame.work)
			SDL_DestroyCondition(rgd3_savegame.work);
		if (rgd3_savegame.lock)
			SDL_DestroyMutex(rgd3_savegame.lock);
		memset(&rgd3_savegame, 0, sizeof(rgd3_savegame));
		return 0;
	}
	rgd3_savegame.thread = SDL_CreateThread(rgd3_savegame_thread, "Savegame", NULL);
	if (!rgd3_savegame.thread)
		rgd3_warning("savegame: couldn't start the writer thread (%s); saves will be written synchronously", SDL_GetError());
	rgd3_savegame.initialized = 1;
	return 1;
}

void rgd3_savegame_shutdown(void)
{
	if (!rgd3_savegame.initialized)
		return;
	rgd3_savegame_wait();
	SDL_LockMutex(rgd3_savegame.lock);
	rgd3_savegame.quit = 1;
	SDL_SignalCondition(rgd3_savegame.work);
	SDL_UnlockMutex(rgd3_savegame.lock);
	if (rgd3_savegame.thread)
		SDL_WaitThread(rgd3_savegame.thread, NULL);
	rgd3_savegame.thread = NULL;
	rgd3_savegame.callback = NULL;
	rgd3_savegame_pump();
	SDL_DestroyCondition(rgd3_savegame.idle);
	SDL_DestroyCondition(rgd3_savegame.work);
	SDL_DestroyMutex(rgd3_savegame.lock);
	memset(&rgd3_savegame, 0, sizeof(rgd3_savegame));
}

void rgd3_savegame_set_completed_callback(Rgd3SaveCompletedFn fn, void* user)
{
	rgd3_savegame.callback = fn;
	rgd3_savegame.callback_user = user;
}

/* One finished job at a time, freed before its report and callback, so a
   drop out of the callback leaves nothing behind. */
void rgd3_savegame_pump(void)
{
	if (!rgd3_savegame.initialized)
		return;
	for (;;)
	{
		Rgd3SaveJob* job;
		Rgd3SaveHandle handle;
		char slot[RGD3_SAVEGAME_MAX_FOLDER_NAME];
		char message[sizeof(((Rgd3SaveJob*)0)->message)];
		int error;
		Rgd3SaveResult* result;

		SDL_LockMutex(rgd3_savegame.lock);
		job = rgd3_savegame.done_head;
		if (job)
		{
			rgd3_savegame.done_head = job->next;
			if (!rgd3_savegame.done_head)
				rgd3_savegame.done_tail = NULL;
		}
		SDL_UnlockMutex(rgd3_savegame.lock);
		if (!job)
			break;

		handle = job->handle;
		error = job->error;
		if (job->simulated_error)
			error = job->simulated_error; /* savegame_error overrides the result */
		rgd3_str_copynz(slot, job->slot, sizeof(slot));
		rgd3_str_copynz(message, job->message, sizeof(message));
		rgd3_savegame_job_free(job);

		result = &rgd3_savegame.results[rgd3_savegame.next_result];
		rgd3_savegame.next_result = (rgd3_savegame.next_result + 1) % RGD3_SAVEGAME_NUM_RESULTS;
		result->handle = handle;
		result->error = error;

		if (error & ~RGD3_SAVEGAME_E_CANCELLED)
			rgd3_savegame_report("saving", slot, error, message);
		else if (error)
			rgd3_dprintf("save to '%s' cancelled (replaced by a newer save or deleted)\n", slot);
		else if (cv_saveGame_verbose.integer_value)
			rgd3_printf("saved '%s'\n", slot);
		if (rgd3_savegame.callback)
			rgd3_savegame.callback(handle, slot, error, rgd3_savegame.callback_user);
	}
}

int rgd3_savegame_is_busy(void)
{
	int busy;
	if (!rgd3_savegame.initialized)
		return 0;
	SDL_LockMutex(rgd3_savegame.lock);
	busy = rgd3_savegame.queue_head || rgd3_savegame.in_flight || rgd3_savegame.done_head;
	SDL_UnlockMutex(rgd3_savegame.lock);
	return busy;
}

void rgd3_savegame_wait(void)
{
	if (!rgd3_savegame.initialized)
		return;
	SDL_LockMutex(rgd3_savegame.lock);
	while (rgd3_savegame.thread && (rgd3_savegame.queue_head || rgd3_savegame.in_flight))
		SDL_WaitCondition(rgd3_savegame.idle, rgd3_savegame.lock);
	SDL_UnlockMutex(rgd3_savegame.lock);
	rgd3_savegame_pump();
}

static int rgd3_savegame_job_listed(const Rgd3SaveJob* job, Rgd3SaveHandle handle)
{
	for (; job; job = job->next)
		if (job->handle == handle)
			return 1;
	return 0;
}

int rgd3_savegame_handle_state(Rgd3SaveHandle handle, int* error)
{
	int i, pending;
	if (error)
		*error = 0;
	if (!rgd3_savegame.initialized || handle <= 0)
		return RGD3_SAVE_STATE_UNKNOWN;
	SDL_LockMutex(rgd3_savegame.lock);
	pending = rgd3_savegame_job_listed(rgd3_savegame.queue_head, handle) ||
		(rgd3_savegame.in_flight && rgd3_savegame.in_flight->handle == handle) ||
		rgd3_savegame_job_listed(rgd3_savegame.done_head, handle);
	SDL_UnlockMutex(rgd3_savegame.lock);
	if (pending)
		return RGD3_SAVE_STATE_PENDING;
	for (i = 0; i < RGD3_SAVEGAME_NUM_RESULTS; i++)
	{
		if (rgd3_savegame.results[i].handle == handle)
		{
			if (error)
				*error = rgd3_savegame.results[i].error;
			return RGD3_SAVE_STATE_DONE;
		}
	}
	return RGD3_SAVE_STATE_UNKNOWN;
}

int rgd3_savegame_is_completed(Rgd3SaveHandle handle)
{
	return handle == 0 || rgd3_savegame_handle_state(handle, NULL) != RGD3_SAVE_STATE_PENDING;
}

/* The details file: descriptors + language + saveVersion (if absent) +
   checksum over the rest, as Common_load.cpp SaveGame. */
static int rgd3_savegame_details_text(const Rgd3SaveGameDetails* details, Rgd3SaveJobFile* out)
{
	Rgd3Dict d = { 0 };
	Rgd3File* f;
	if (details)
		rgd3_dict_assign(&d, &details->descriptors);
	rgd3_dict_set(&d, RGD3_SAVEGAME_DETAIL_FIELD_LANGUAGE, rgd3_cvar_get_string("sys_lang"));
	if (!rgd3_dict_find_key(&d, RGD3_SAVEGAME_DETAIL_FIELD_SAVE_VERSION))
		rgd3_dict_set_int(&d, RGD3_SAVEGAME_DETAIL_FIELD_SAVE_VERSION, RGD3_SAVEGAME_VERSION);
	rgd3_dict_delete(&d, RGD3_SAVEGAME_DETAIL_FIELD_CHECKSUM);
	rgd3_dict_set_int(&d, RGD3_SAVEGAME_DETAIL_FIELD_CHECKSUM, rgd3_dict_checksum(&d));
	f = rgd3_fs_open_memory_write(RGD3_SAVEGAME_DETAILS_FILENAME);
	rgd3_dict_write_ini(&d, f);
	rgd3_dict_clear(&d);
	rgd3_str_copynz(out->name, RGD3_SAVEGAME_DETAILS_FILENAME, sizeof(out->name));
	out->type = RGD3_SAVEGAMEFILE_TEXT;
	out->size = (size_t)rgd3_fs_length(f);
	out->data = (u8*)rgd3_heap_alloc(out->size ? out->size : 1, RGD3_TAG_SAVEGAMES);
	if (out->size)
		memcpy(out->data, rgd3_fs_memory_data(f), out->size);
	rgd3_fs_close(f);
	return 1;
}

static int rgd3_savegame_check_files(const char* what, const char* slot, const Rgd3SaveFile* files, int num_files, int need_file)
{
	int i, j;
	if (!rgd3_savegame_valid_name(slot, RGD3_SAVEGAME_MAX_FOLDER_NAME))
	{
		rgd3_savegame_report(what, slot, RGD3_SAVEGAME_E_INVALID_FILENAME, "bad slot name");
		return 0;
	}
	if (num_files < 0 || num_files > RGD3_SAVEGAME_MAX_FILES || (num_files && !files))
	{
		rgd3_savegame_report(what, slot, RGD3_SAVEGAME_E_UNKNOWN, "bad file count");
		return 0;
	}
	for (i = 0; i < num_files; i++)
	{
		const char* name = files[i].name;
		/* .tmp and .bak are the writer's staging names */
		if (!rgd3_savegame_valid_name(name, RGD3_SAVEGAME_MAX_FILE_NAME) || rgd3_str_icmp(name, RGD3_SAVEGAME_DETAILS_FILENAME) == 0 ||
			rgd3_str_iends_with(name, ".tmp") || rgd3_str_iends_with(name, ".bak"))
		{
			rgd3_savegame_report(what, slot, RGD3_SAVEGAME_E_INVALID_FILENAME, name ? name : "(null)");
			return 0;
		}
		for (j = 0; j < i; j++)
		{
			if (rgd3_str_icmp(name, files[j].name) == 0)
			{
				rgd3_savegame_report(what, slot, RGD3_SAVEGAME_E_INVALID_FILENAME, "duplicate file name");
				return 0;
			}
		}
		if (need_file && !rgd3_fs_is_memory_file(files[i].file))
		{
			rgd3_savegame_report(what, slot, RGD3_SAVEGAME_E_UNKNOWN, "not a memory file");
			return 0;
		}
		if (!need_file && files[i].file && !rgd3_fs_is_memory_file(files[i].file))
		{
			rgd3_savegame_report(what, slot, RGD3_SAVEGAME_E_UNKNOWN, "not a memory file");
			return 0;
		}
	}
	return 1;
}

Rgd3SaveHandle rgd3_savegame_save(const char* slot, const Rgd3SaveFile* files, int num_files, const Rgd3SaveGameDetails* details)
{
	Rgd3SaveJob* job;
	int i;
	if (!rgd3_savegame.initialized)
	{
		rgd3_warning("rgd3_savegame_save: the save system is not running");
		return 0;
	}
	if (!cv_saveGame_enable.integer_value)
	{
		rgd3_warning("Savegames are disabled (saveGame_enable = 0). Skipping physical save to media.");
		return 0;
	}
	if (!rgd3_savegame_check_files("saving", slot, files, num_files, 1))
		return 0;
	if (num_files == 0)
	{
		rgd3_warning("No files to save.");
		return 0;
	}

	job = (Rgd3SaveJob*)rgd3_heap_calloc(sizeof(Rgd3SaveJob), RGD3_TAG_SAVEGAMES);
	rgd3_str_copynz(job->slot, slot, sizeof(job->slot));
	if (!rgd3_savegame_slot_dir(slot, job->os_dir, sizeof(job->os_dir)))
	{
		rgd3_heap_free(job);
		rgd3_savegame_report("saving", slot, RGD3_SAVEGAME_E_UNKNOWN, "no save path");
		return 0;
	}
	for (i = 0; i < num_files; i++)
	{
		Rgd3SaveJobFile* f = &job->files[i];
		const u8* data = rgd3_fs_memory_data(files[i].file);
		rgd3_str_copynz(f->name, files[i].name, sizeof(f->name));
		f->type = files[i].type;
		f->size = (size_t)rgd3_fs_length(files[i].file);
		f->data = (u8*)rgd3_heap_alloc(f->size ? f->size : 1, RGD3_TAG_SAVEGAMES);
		if (f->size)
			memcpy(f->data, data, f->size);
	}
	rgd3_savegame_details_text(details, &job->files[num_files]);
	job->num_files = num_files + 1;
	job->delay_ms = cv_savegame_winInduceDelay.integer_value;
	job->simulated_error = cv_savegame_error.integer_value;

	SDL_LockMutex(rgd3_savegame.lock);
	job->handle = ++rgd3_savegame.next_handle;
	rgd3_savegame_cancel_queued(slot); /* newest wins */
	if (rgd3_savegame.thread)
	{
		job->next = NULL;
		if (rgd3_savegame.queue_tail)
			rgd3_savegame.queue_tail->next = job;
		else
			rgd3_savegame.queue_head = job;
		rgd3_savegame.queue_tail = job;
		SDL_SignalCondition(rgd3_savegame.work);
		SDL_UnlockMutex(rgd3_savegame.lock);
	}
	else
	{
		SDL_UnlockMutex(rgd3_savegame.lock);
		rgd3_savegame_run_job(job);
		SDL_LockMutex(rgd3_savegame.lock);
		rgd3_savegame_push_done(job);
		SDL_UnlockMutex(rgd3_savegame.lock);
	}
	if (cv_saveGame_verbose.integer_value)
		rgd3_printf("savegame: queued '%s' (%d files, handle %d)\n", slot, num_files + 1, job->handle);
	return job->handle;
}

int rgd3_savegame_load_sync(const char* slot, Rgd3SaveFile* files, int num_files, Rgd3SaveGameDetails* details)
{
	char dir[RGD3_MAX_OSPATH];
	char path[RGD3_MAX_OSPATH];
	Rgd3SaveGameDetails local = { 0 };
	Rgd3SaveGameDetails* d = details ? details : &local;
	int error = 0, i;
	void* buffer;
	size_t size;

	if (details)
		rgd3_savegame_details_clear(details);
	if (!rgd3_savegame.initialized)
		return RGD3_SAVEGAME_E_UNKNOWN;
	if (!cv_saveGame_enable.integer_value)
	{
		rgd3_warning("Savegames are disabled (saveGame_enable = 0). Skipping physical load from media.");
		return RGD3_SAVEGAME_E_CANCELLED;
	}
	if (!rgd3_savegame_check_files("loading", slot, files, num_files, 0))
		return RGD3_SAVEGAME_E_INVALID_FILENAME;
	rgd3_savegame_wait(); /* writes still in flight come first */
	if (!rgd3_savegame_slot_dir(slot, dir, sizeof(dir)) || !rgd3_platform_is_dir(dir))
		return RGD3_SAVEGAME_E_FOLDER_NOT_FOUND;

	for (i = 0; i < num_files; i++)
	{
		Rgd3SaveFile* sf = &files[i];
		sf->error = 0;
		if (!sf->file)
			sf->file = rgd3_fs_open_memory_write(sf->name);
		rgd3_fs_memory_make_writable(sf->file);
		rgd3_fs_memory_clear(sf->file, 0);
		rg_snprintf(path, sizeof(path), "%s/%s", dir, sf->name);
		buffer = SDL_LoadFile(path, &size);
		if (!buffer)
		{
			sf->error = 1;
			if (!(sf->type & RGD3_SAVEGAMEFILE_OPTIONAL))
				error |= RGD3_SAVEGAME_E_CORRUPTED;
		}
		else
		{
			if (sf->type & RGD3_SAVEGAMEFILE_TEXT)
			{
				if (size && rgd3_fs_write(sf->file, buffer, size) != size)
					sf->error = RGD3_SAVEGAME_E_UNKNOWN;
			}
			else
			{
				sf->error = rgd3_savegame_decode((const u8*)buffer, size, sf->file, cv_saveGame_checksum.integer_value);
			}
			error |= sf->error;
			sf->error = sf->error != 0;
			SDL_free(buffer);
		}
		rgd3_fs_memory_make_read_only(sf->file);
	}

	/* LoadGameCheckDescriptionFile */
	rg_snprintf(path, sizeof(path), "%s/" RGD3_SAVEGAME_DETAILS_FILENAME, dir);
	buffer = SDL_LoadFile(path, &size);
	if (!buffer)
	{
		error |= RGD3_SAVEGAME_E_FILE_NOT_FOUND;
	}
	else
	{
		Rgd3File* f = rgd3_fs_open_memory_read(RGD3_SAVEGAME_DETAILS_FILENAME, buffer, size);
		rgd3_savegame_details_read(d, f);
		rgd3_fs_close(f);
		SDL_free(buffer);
		d->date = rgd3_savegame_path_date(path);
		rgd3_str_copynz(d->slot_name, slot, sizeof(d->slot_name));
		if (d->damaged)
			error |= RGD3_SAVEGAME_E_CORRUPTED;
		else if (rgd3_savegame_details_save_version(d) > RGD3_SAVEGAME_VERSION)
			error |= RGD3_SAVEGAME_E_INCOMPATIBLE_NEWER_VERSION;
	}
	if (cv_savegame_error.integer_value)
		error = cv_savegame_error.integer_value;
	if (error)
		rgd3_savegame_report("loading", slot, error, NULL);
	rgd3_savegame_details_clear(&local);
	return error;
}

typedef struct Rgd3SaveDirList
{
	char** names;
	int count;
	int capacity;
	const char* dir;
	int dirs; /* 1: collect directories, 0: everything */
} Rgd3SaveDirList;

static SDL_EnumerationResult SDLCALL rgd3_savegame_enum_cb(void* user, const char* dirname, const char* fname)
{
	Rgd3SaveDirList* list = (Rgd3SaveDirList*)user;
	char path[RGD3_MAX_OSPATH];
	RG_UNUSED(dirname);
	rg_snprintf(path, sizeof(path), "%s/%s", list->dir, fname);
	if (list->dirs && !rgd3_platform_is_dir(path))
		return SDL_ENUM_CONTINUE;
	if (list->count == list->capacity)
	{
		list->capacity = list->capacity ? list->capacity * 2 : 16;
		list->names = (char**)rgd3_heap_realloc(list->names, (size_t)list->capacity * sizeof(char*), RGD3_TAG_SAVEGAMES);
	}
	list->names[list->count++] = rgd3_heap_strdup(fname, RGD3_TAG_SAVEGAMES);
	return SDL_ENUM_CONTINUE;
}

static void rgd3_savegame_dir_list(Rgd3SaveDirList* list, const char* dir, int dirs)
{
	memset(list, 0, sizeof(*list));
	list->dir = dir;
	list->dirs = dirs;
	SDL_EnumerateDirectory(dir, rgd3_savegame_enum_cb, list);
}

static void rgd3_savegame_dir_list_free(Rgd3SaveDirList* list)
{
	int i;
	for (i = 0; i < list->count; i++)
		rgd3_heap_free(list->names[i]);
	rgd3_heap_free(list->names);
	memset(list, 0, sizeof(*list));
}

/* newer (larger date) towards the start, then by name */
static int rgd3_savegame_sort_details(const void* a, const void* b)
{
	const Rgd3SaveGameDetails* x = (const Rgd3SaveGameDetails*)a;
	const Rgd3SaveGameDetails* y = (const Rgd3SaveGameDetails*)b;
	if (x->date != y->date)
		return x->date > y->date ? -1 : 1;
	return rgd3_str_icmp(x->slot_name, y->slot_name);
}

int rgd3_savegame_enumerate(Rgd3SaveGameList* list)
{
	char saves[RGD3_MAX_OSPATH];
	char path[RGD3_MAX_OSPATH];
	Rgd3SaveDirList dirs;
	int i;

	list->items = NULL;
	list->count = 0;
	if (!rgd3_savegame.initialized)
		return RGD3_SAVEGAME_E_UNKNOWN;
	if (!cv_saveGame_enable.integer_value)
		return RGD3_SAVEGAME_E_CANCELLED;
	rgd3_savegame_wait();
	if (!rgd3_savegame_saves_dir(saves, sizeof(saves)) || !rgd3_platform_is_dir(saves))
		return RGD3_SAVEGAME_E_FOLDER_NOT_FOUND;
	rgd3_savegame_dir_list(&dirs, saves, 1);
	if (dirs.count)
		list->items = (Rgd3SaveGameDetails*)rgd3_heap_calloc((size_t)dirs.count * sizeof(Rgd3SaveGameDetails), RGD3_TAG_SAVEGAMES);
	for (i = 0; i < dirs.count; i++)
	{
		Rgd3SaveGameDetails* d;
		void* buffer;
		size_t size;
		rg_snprintf(path, sizeof(path), "%s/%s/" RGD3_SAVEGAME_DETAILS_FILENAME, saves, dirs.names[i]);
		if (!rgd3_platform_is_file(path))
			continue;
		d = &list->items[list->count++];
		rgd3_str_copynz(d->slot_name, dirs.names[i], sizeof(d->slot_name));
		buffer = SDL_LoadFile(path, &size);
		if (!buffer)
		{
			d->damaged = 1;
		}
		else
		{
			Rgd3File* f = rgd3_fs_open_memory_read(RGD3_SAVEGAME_DETAILS_FILENAME, buffer, size);
			rgd3_savegame_details_read(d, f);
			rgd3_fs_close(f);
			SDL_free(buffer);
		}
		d->date = rgd3_savegame_path_date(path);
	}
	rgd3_savegame_dir_list_free(&dirs);
	if (list->count > 1)
		qsort(list->items, (size_t)list->count, sizeof(Rgd3SaveGameDetails), rgd3_savegame_sort_details);
	if (!list->count)
	{
		rgd3_heap_free(list->items);
		list->items = NULL;
	}
	return 0;
}

void rgd3_savegame_list_free(Rgd3SaveGameList* list)
{
	int i;
	for (i = 0; i < list->count; i++)
		rgd3_savegame_details_clear(&list->items[i]);
	rgd3_heap_free(list->items);
	list->items = NULL;
	list->count = 0;
}

int rgd3_savegame_delete(const char* slot)
{
	char dir[RGD3_MAX_OSPATH];
	char path[RGD3_MAX_OSPATH];
	Rgd3SaveDirList files;
	int i, error = 0;

	if (!rgd3_savegame.initialized)
		return RGD3_SAVEGAME_E_UNKNOWN;
	if (!cv_saveGame_enable.integer_value)
		return RGD3_SAVEGAME_E_CANCELLED;
	if (!rgd3_savegame_valid_name(slot, RGD3_SAVEGAME_MAX_FOLDER_NAME))
		return RGD3_SAVEGAME_E_INVALID_FILENAME;
	SDL_LockMutex(rgd3_savegame.lock);
	rgd3_savegame_cancel_queued(slot);
	SDL_UnlockMutex(rgd3_savegame.lock);
	rgd3_savegame_wait();
	if (!rgd3_savegame_slot_dir(slot, dir, sizeof(dir)) || !rgd3_platform_is_dir(dir))
		return RGD3_SAVEGAME_E_FOLDER_NOT_FOUND;
	rgd3_savegame_dir_list(&files, dir, 0);
	for (i = 0; i < files.count; i++)
	{
		rg_snprintf(path, sizeof(path), "%s/%s", dir, files.names[i]);
		if (!SDL_RemovePath(path))
			error = RGD3_SAVEGAME_E_UNKNOWN;
	}
	rgd3_savegame_dir_list_free(&files);
	if (!SDL_RemovePath(dir))
		error = RGD3_SAVEGAME_E_UNKNOWN;
	return error;
}

/* ---------------------------------------------------------------------------
   Commands
--------------------------------------------------------------------------- */

/* Sys_TimeStampToStr, English form: "MM/DD/YYYY hh:mm am" */
static const char* rgd3_savegame_time_string(i64 date, char* out, size_t size)
{
	SDL_DateTime dt;
	int hour;
	if (!SDL_TimeToDateTime((SDL_Time)date * SDL_NS_PER_SECOND, &dt, true))
		return "??/??/???? ??:??";
	hour = dt.hour > 12 ? dt.hour - 12 : (dt.hour == 0 ? 12 : dt.hour);
	rg_snprintf(out, size, "%02d/%02d/%d %02d:%02d %s", dt.month, dt.day, dt.year, hour, dt.minute, dt.hour >= 12 ? "pm" : "am");
	return out;
}

/* OutputDetailList */
static void rgd3_savegame_output_detail_list(const Rgd3SaveGameList* list)
{
	int i;
	char date[64];
	for (i = 0; i < list->count; i++)
	{
		const Rgd3SaveGameDetails* d = &list->items[i];
		rgd3_printf("^3\t%s - %s\n^7\t\tMap: %s\n\t\tTime: %s\n", d->slot_name, d->damaged ? "^1CORRUPT" : "^2OK",
			d->damaged ? "?" : rgd3_savegame_details_map_name(d), rgd3_savegame_time_string(d->date, date, sizeof(date)));
	}
}

static void rgd3_savegame_cmd_enumerate(const Rgd3CmdArgs* args)
{
	Rgd3SaveGameList list;
	int error;
	RG_UNUSED(args);
	error = rgd3_savegame_enumerate(&list);
	if (error && error != RGD3_SAVEGAME_E_FOLDER_NOT_FOUND)
	{
		rgd3_printf("Error enumerating.\n");
		return;
	}
	rgd3_printf("Savegames found: %d\n\n", list.count);
	rgd3_savegame_output_detail_list(&list);
	rgd3_savegame_list_free(&list);
}

static void rgd3_savegame_cmd_delete(const Rgd3CmdArgs* args)
{
	char names[512];
	int error;
	if (rgd3_cmdargs_argc(args) != 2)
	{
		rgd3_printf("Usage: testSavegameDelete <slot>\n");
		return;
	}
	error = rgd3_savegame_delete(rgd3_cmdargs_argv(args, 1));
	rgd3_printf("Completed process.\n");
	rgd3_printf("Error = 0x%08X, %s\n", (unsigned)error, rgd3_savegame_error_string(error, names, sizeof(names)));
}

static void rgd3_savegame_cmd_print_errors(const Rgd3CmdArgs* args)
{
	int i;
	RG_UNUSED(args);
	rgd3_printf("Bit  Description\n---  -----------\n");
	for (i = 0; i < RGD3_SAVEGAME_E_BITS_USED; i++)
		rgd3_printf("%03d  %s\n", i, rgd3_savegame_error_names[i]);
}

static void rgd3_savegame_cmd_set_error_bit(const Rgd3CmdArgs* args)
{
	char value[32];
	int bit = atoi(rgd3_cmdargs_argv(args, 1));
	if (bit < 0 || bit >= RGD3_SAVEGAME_E_BITS_USED)
	{
		rgd3_printf("usage: savegameSetErrorBit <0..%d>\n", RGD3_SAVEGAME_E_BITS_USED - 1);
		return;
	}
	rg_snprintf(value, sizeof(value), "%d", cv_savegame_error.integer_value | (1 << bit));
	rgd3_cvar_set(&cv_savegame_error, value, 0);
}

void rgd3_savegame_register(void)
{
	static Rgd3Cvar* const cvars[] = {
		&cv_saveGame_verbose, &cv_saveGame_checksum, &cv_saveGame_enable, &cv_savegame_error, &cv_savegame_winInduceDelay
	};
	rgd3_cvar_register_table(cvars, (int)RG_ARRAY_COUNT(cvars));
	rgd3_cmd_add("testSavegameEnumerate", rgd3_savegame_cmd_enumerate, RGD3_CMD_FL_SYSTEM, "enumerates the savegames available", NULL);
	rgd3_cmd_add("testSavegameDelete", rgd3_savegame_cmd_delete, RGD3_CMD_FL_SYSTEM, "deletes a savegames without confirmation", NULL);
	rgd3_cmd_add("savegamePrintErrors", rgd3_savegame_cmd_print_errors, RGD3_CMD_FL_SYSTEM, "Prints error code corresponding to each bit", NULL);
	rgd3_cmd_add("savegameSetErrorBit", rgd3_savegame_cmd_set_error_bit, RGD3_CMD_FL_SYSTEM, "Allows you to set savegame_error by bit instead of integer value", NULL);
}

/* ---------------------------------------------------------------------------
   Self-test
--------------------------------------------------------------------------- */

#define RGD3_SAVEGAME_TEST_SLOT "rgd3_selftest"
#define RGD3_SAVEGAME_TEST_SLOT_B "rgd3_selftest_b"

static int rgd3_savegame_test_checks;
static int rgd3_savegame_test_failures;

#define RGD3_SAVE_CHECK(cond) rgd3_savegame_test_check((cond) != 0, #cond, __LINE__)

static void rgd3_savegame_test_check(int ok, const char* what, int line)
{
	rgd3_savegame_test_checks++;
	if (!ok)
	{
		rgd3_savegame_test_failures++;
		rgd3_printf("^1FAILED^7 %s (rgd3_savegame.c:%d)\n", what, line);
	}
}

static const Rgd3MemTag rgd3_savegame_test_tags[] = {
	RGD3_TAG_TEMP, RGD3_TAG_FILE, RGD3_TAG_STRING, RGD3_TAG_LEXER, RGD3_TAG_DICT, RGD3_TAG_SAVEGAMES
};

static void rgd3_savegame_test_snapshot(i64* counts)
{
	int i;
	for (i = 0; i < (int)RG_ARRAY_COUNT(rgd3_savegame_test_tags); i++)
		counts[i] = rgd3_mem_tag_count(rgd3_savegame_test_tags[i]);
}

static int rgd3_savegame_test_same(const i64* a, const i64* b)
{
	int i, same = 1;
	for (i = 0; i < (int)RG_ARRAY_COUNT(rgd3_savegame_test_tags); i++)
	{
		if (a[i] != b[i])
		{
			rgd3_printf("savegame test: tag %s %lld -> %lld blocks\n", rgd3_mem_tag_name(rgd3_savegame_test_tags[i]), (long long)a[i], (long long)b[i]);
			same = 0;
		}
	}
	return same;
}

/* deterministic payload: noise and text runs, so blocks compress unevenly */
static u8* rgd3_savegame_test_payload(size_t size, u32 seed)
{
	u8* p = (u8*)rgd3_heap_alloc(size, RGD3_TAG_SAVEGAMES);
	static const char text[] = "monster_demon_imp health 130 origin \"1024 -512 64\" ";
	size_t i;
	u32 x = seed * 2654435761u + 1u;
	for (i = 0; i < size; i++)
	{
		if ((i / 4096) & 1)
		{
			p[i] = (u8)text[i % (sizeof(text) - 1)];
		}
		else
		{
			x = x * 1664525u + 1013904223u;
			p[i] = (u8)(x >> 24);
		}
	}
	return p;
}

static void rgd3_savegame_test_memory_files(void)
{
	Rgd3File* f = rgd3_fs_open_memory_write("test.bin");
	Rgd3File* r;
	Rgd3StrBuf s = { 0 };
	Rgd3Vec2 v2 = { { 1.0f, -2.0f } };
	Rgd3Vec3 v3, g3;
	Rgd3Vec4 v4, g4;
	Rgd3Vec2 g2;
	Rgd3Mat3 m, gm;
	Rgd3Dict d = { 0 }, back = { 0 };
	const u8* data;
	i32 iv;
	u32 uv;
	i16 sv;
	u16 usv;
	i8 cv;
	u8 ucv;
	float fv;
	int bv;
	i64 lv;
	int i;

	v3.x = 0.5f; v3.y = 1.5f; v3.z = -3.25f;
	v4.x = 1.0f; v4.y = 2.0f; v4.z = 3.0f; v4.w = 4.0f;
	for (i = 0; i < 9; i++)
		((float*)&m)[i] = (float)i * 0.5f;

	RGD3_SAVE_CHECK(rgd3_fs_is_memory_file(f) && rgd3_fs_length(f) == 0);
	RGD3_SAVE_CHECK(rgd3_fs_write_int(f, 0x11223344) == 4);
	data = rgd3_fs_memory_data(f);
	RGD3_SAVE_CHECK(data && data[0] == 0x44 && data[1] == 0x33 && data[2] == 0x22 && data[3] == 0x11 && data[4] == 0);
	RGD3_SAVE_CHECK(rgd3_fs_write_string(f, "ab") == 2);
	data = rgd3_fs_memory_data(f);
	RGD3_SAVE_CHECK(data[4] == 2 && data[5] == 0 && data[6] == 0 && data[7] == 0 && data[8] == 'a' && data[9] == 'b');
	rgd3_fs_write_uint(f, 0xDEADBEEFu);
	rgd3_fs_write_short(f, -2);
	rgd3_fs_write_ushort(f, 65535);
	rgd3_fs_write_char(f, -128);
	rgd3_fs_write_uchar(f, 200);
	rgd3_fs_write_float(f, -1.5f);
	rgd3_fs_write_bool(f, 7);
	rgd3_fs_write_string(f, "");
	rgd3_fs_write_vec2(f, v2);
	rgd3_fs_write_vec3(f, v3);
	rgd3_fs_write_vec4(f, v4);
	rgd3_fs_write_mat3(f, &m);
	RGD3_SAVE_CHECK(rgd3_fs_write_big_int(f, 0x01020304) == 4);
	data = rgd3_fs_memory_data(f);
	RGD3_SAVE_CHECK(data[rgd3_fs_length(f) - 4] == 1 && data[rgd3_fs_length(f) - 1] == 4);
	rgd3_fs_write_big_short(f, -3);
	rgd3_fs_write_big_int64(f, -5);
	rgd3_fs_write_big_float(f, 2.5f);
	rgd3_fs_printf(f, "x%d", 42);
	rgd3_dict_set(&d, "classname", "worldspawn");
	rgd3_dict_set(&d, "message", "hello world");
	rgd3_dict_write_to_file(&d, f);
	RGD3_SAVE_CHECK(rgd3_fs_tell(f) == rgd3_fs_length(f));
	/* overwrite in place: the length stays */
	{
		i64 len = rgd3_fs_length(f);
		RGD3_SAVE_CHECK(rgd3_fs_seek(f, 0, RGD3_FS_SEEK_SET));
		rgd3_fs_write_int(f, 0x55667788);
		RGD3_SAVE_CHECK(rgd3_fs_length(f) == len && rgd3_fs_tell(f) == 4);
		RGD3_SAVE_CHECK(!rgd3_fs_seek(f, len + 1, RGD3_FS_SEEK_SET));
	}
	RGD3_SAVE_CHECK(rgd3_fs_read(f, &iv, 4) == 0); /* write mode */

	RGD3_SAVE_CHECK(rgd3_fs_memory_make_read_only(f));
	RGD3_SAVE_CHECK(rgd3_fs_write_int(f, 1) == 0); /* read mode */
	RGD3_SAVE_CHECK(rgd3_fs_read_int(f, &iv) == 4 && iv == 0x55667788);
	RGD3_SAVE_CHECK(rgd3_fs_read_string(f, &s) == 2 && strcmp(rgd3_strbuf_cstr(&s), "ab") == 0);
	RGD3_SAVE_CHECK(rgd3_fs_read_uint(f, &uv) == 4 && uv == 0xDEADBEEFu);
	RGD3_SAVE_CHECK(rgd3_fs_read_short(f, &sv) == 2 && sv == -2);
	RGD3_SAVE_CHECK(rgd3_fs_read_ushort(f, &usv) == 2 && usv == 65535);
	RGD3_SAVE_CHECK(rgd3_fs_read_char(f, &cv) == 1 && cv == -128);
	RGD3_SAVE_CHECK(rgd3_fs_read_uchar(f, &ucv) == 1 && ucv == 200);
	RGD3_SAVE_CHECK(rgd3_fs_read_float(f, &fv) == 4 && fv == -1.5f);
	RGD3_SAVE_CHECK(rgd3_fs_read_bool(f, &bv) == 1 && bv == 1);
	RGD3_SAVE_CHECK(rgd3_fs_read_string(f, &s) == 0 && s.len == 0);
	RGD3_SAVE_CHECK(rgd3_fs_read_vec2(f, &g2) == 8 && g2.x == 1.0f && g2.y == -2.0f);
	RGD3_SAVE_CHECK(rgd3_fs_read_vec3(f, &g3) == 12 && g3.x == v3.x && g3.y == v3.y && g3.z == v3.z);
	RGD3_SAVE_CHECK(rgd3_fs_read_vec4(f, &g4) == 16 && g4.w == 4.0f);
	RGD3_SAVE_CHECK(rgd3_fs_read_mat3(f, &gm) == 36 && memcmp(&gm, &m, sizeof(m)) == 0);
	RGD3_SAVE_CHECK(rgd3_fs_read_big_int(f, &iv) == 4 && iv == 0x01020304);
	RGD3_SAVE_CHECK(rgd3_fs_read_big_short(f, &sv) == 2 && sv == -3);
	RGD3_SAVE_CHECK(rgd3_fs_read_big_int64(f, &lv) == 8 && lv == -5);
	RGD3_SAVE_CHECK(rgd3_fs_read_big_float(f, &fv) == 4 && fv == 2.5f);
	{
		char x[5] = { 0 };
		RGD3_SAVE_CHECK(rgd3_fs_read(f, x, 4) == 4 && x[0] == 'x' && x[1] == '4' && x[2] == '2');
		rgd3_fs_seek(f, -1, RGD3_FS_SEEK_CUR); /* the 4th byte was the dict count */
	}
	rgd3_dict_read_from_file(&back, f);
	RGD3_SAVE_CHECK(rgd3_dict_checksum(&back) == rgd3_dict_checksum(&d));
	RGD3_SAVE_CHECK(rgd3_fs_tell(f) == rgd3_fs_length(f));
	RGD3_SAVE_CHECK(rgd3_fs_read_int(f, &iv) == 0 && iv == 0); /* past the end */

	/* a referencing reader; a bogus string length is rejected */
	{
		static const u8 bogus[8] = { 0xFF, 0xFF, 0xFF, 0x7F, 'a', 'b', 'c', 'd' };
		static const u8 negative[4] = { 0xFE, 0xFF, 0xFF, 0xFF };
		r = rgd3_fs_open_memory_read("bogus", bogus, sizeof(bogus));
		RGD3_SAVE_CHECK(rgd3_fs_read_string(r, &s) == 0 && s.len == 0);
		/* writable copies the referenced data first */
		RGD3_SAVE_CHECK(rgd3_fs_memory_make_writable(r));
		RGD3_SAVE_CHECK(rgd3_fs_memory_data(r) != bogus && rgd3_fs_length(r) == 8);
		rgd3_fs_write_uchar(r, 1);
		RGD3_SAVE_CHECK(bogus[0] == 0xFF && rgd3_fs_memory_data(r)[0] == 1);
		rgd3_fs_memory_clear(r, 0);
		RGD3_SAVE_CHECK(rgd3_fs_length(r) == 0 && rgd3_fs_tell(r) == 0);
		rgd3_fs_close(r);
		r = rgd3_fs_open_memory_read("negative", negative, sizeof(negative));
		RGD3_SAVE_CHECK(rgd3_fs_read_string(r, &s) == 0);
		rgd3_fs_close(r);
	}
	rgd3_fs_memory_clear(f, 1);
	RGD3_SAVE_CHECK(rgd3_fs_length(f) == 0 && rgd3_fs_memory_data(f) == NULL);
	rgd3_strbuf_free(&s);
	rgd3_dict_clear(&d);
	rgd3_dict_clear(&back);
	rgd3_fs_close(f);
}

static void rgd3_savegame_test_ini(void)
{
	Rgd3Dict d = { 0 }, back = { 0 }, small = { 0 };
	Rgd3File* f;
	const char* expected = "a=\"1\"\nb=\"x\"\n\n[s]\nk=\"v\"\n";
	static const char hand[] = "// written by hand\nmapName=\"game/admin\"\n[sec]\nkey=\"v w\"\n\n[sec/sub]\nx=\"-1\"\n";
	static const char broken[] = "a=\"1\"\n[open\nb=\"2\"\n";

	/* exact layout */
	rgd3_dict_set(&small, "s/k", "v");
	rgd3_dict_set(&small, "b", "x");
	rgd3_dict_set(&small, "a", "1");
	f = rgd3_fs_open_memory_write("small.ini");
	rgd3_dict_write_ini(&small, f);
	RGD3_SAVE_CHECK(rgd3_fs_length(f) == (i64)strlen(expected) && memcmp(rgd3_fs_memory_data(f), expected, strlen(expected)) == 0);
	rgd3_fs_close(f);

	/* TestDictIniFile values, plus quoting */
	rgd3_dict_set_int(&d, "section1/section3/a", -1);
	rgd3_dict_set_int(&d, "section1/section3/b", 0);
	rgd3_dict_set_int(&d, "section1/section3/c", 3);
	rgd3_dict_set_float(&d, "section2/d", 4.0f);
	rgd3_dict_set_float(&d, "section2/e", -5.0f);
	rgd3_dict_set_bool(&d, "section2/f", 1);
	rgd3_dict_set_bool(&d, "section1/g", 0);
	rgd3_dict_set(&d, "section1/h", "test1");
	rgd3_dict_set(&d, "section1/zz", "after the nested section");
	rgd3_dict_set(&d, "i", "1234");
	rgd3_dict_set_int(&d, "j", 9);
	rgd3_dict_set(&d, "quote", "say \"hi\" \\ back\\slash");
	rgd3_dict_set(&d, "ctl", "tab\there\nnew line?");
	rgd3_dict_set(&d, "punct", "a=b [c] 'd' // not a comment");
	rgd3_dict_set(&d, "empty", "");
	rgd3_dict_set(&d, "mapName", "game/mars_city1");
	f = rgd3_fs_open_memory_write("test.ini");
	rgd3_dict_write_ini(&d, f);
	rgd3_fs_memory_make_read_only(f);
	RGD3_SAVE_CHECK(rgd3_dict_read_ini(&back, f));
	RGD3_SAVE_CHECK(rgd3_dict_num(&back) == rgd3_dict_num(&d));
	RGD3_SAVE_CHECK(rgd3_dict_checksum(&back) == rgd3_dict_checksum(&d));
	RGD3_SAVE_CHECK(strcmp(rgd3_dict_get_string(&back, "quote", ""), "say \"hi\" \\ back\\slash") == 0);
	RGD3_SAVE_CHECK(strcmp(rgd3_dict_get_string(&back, "ctl", ""), "tab\there\nnew line?") == 0);
	RGD3_SAVE_CHECK(strcmp(rgd3_dict_get_string(&back, "section1/section3/a", ""), "-1") == 0);
	RGD3_SAVE_CHECK(strcmp(rgd3_dict_get_string(&back, "section1/zz", ""), "after the nested section") == 0);
	rgd3_fs_close(f);

	/* hand-written, with a comment and no blank-line rules */
	f = rgd3_fs_open_memory_read("hand.ini", hand, sizeof(hand) - 1);
	RGD3_SAVE_CHECK(rgd3_dict_read_ini(&back, f));
	RGD3_SAVE_CHECK(rgd3_dict_num(&back) == 3);
	RGD3_SAVE_CHECK(strcmp(rgd3_dict_get_string(&back, "sec/key", ""), "v w") == 0);
	RGD3_SAVE_CHECK(rgd3_dict_get_int(&back, "sec/sub/x", 0) == -1);
	rgd3_fs_close(f);

	/* a broken section header fails without dropping */
	f = rgd3_fs_open_memory_read("broken.ini", broken, sizeof(broken) - 1);
	RGD3_SAVE_CHECK(!rgd3_dict_read_ini(&back, f));
	rgd3_fs_close(f);
	f = rgd3_fs_open_memory_read("empty.ini", "", 0);
	RGD3_SAVE_CHECK(rgd3_dict_read_ini(&back, f) && rgd3_dict_num(&back) == 0);
	rgd3_fs_close(f);

	rgd3_dict_clear(&d);
	rgd3_dict_clear(&back);
	rgd3_dict_clear(&small);
}

static int rgd3_savegame_test_decode(const u8* data, size_t size, const u8* expect, size_t expect_size)
{
	Rgd3File* out = rgd3_fs_open_memory_write("decoded");
	int error = rgd3_savegame_decode(data, size, out, 1);
	if (!error && expect && (rgd3_fs_length(out) != (i64)expect_size || (expect_size && memcmp(rgd3_fs_memory_data(out), expect, expect_size) != 0)))
		error = -1;
	rgd3_fs_close(out);
	return error;
}

static void rgd3_savegame_test_container(void)
{
	const size_t size = 600123;
	u8* payload = rgd3_savegame_test_payload(size, 1);
	size_t enc_size = 0, empty_size = 0;
	u8* enc = rgd3_savegame_encode(payload, size, &enc_size);
	u8* empty = rgd3_savegame_encode(payload, 0, &empty_size);
	u8* bad;
	Rgd3BinReader r;
	u32 count = 0, block_size = 0, first_clen = 0;
	size_t header_size, block1_data, block2_crc;
	int build_len;

	RGD3_SAVE_CHECK(enc != NULL && enc_size > 0 && enc_size < size);
	RGD3_SAVE_CHECK(empty != NULL);
	if (!enc || !empty)
	{
		rgd3_heap_free(enc);
		rgd3_heap_free(empty);
		rgd3_heap_free(payload);
		return;
	}
	RGD3_SAVE_CHECK(memcmp(enc, "RGD3SAVE", 8) == 0);
	rgd3_bin_reader_init(&r, enc, enc_size);
	rgd3_bin_skip(&r, 8);
	RGD3_SAVE_CHECK(rgd3_bin_u32_le(&r) == RGD3_SAVEGAME_FORMAT_VERSION);
	build_len = rgd3_bin_i32_le(&r);
	rgd3_bin_skip(&r, (size_t)build_len);
	RGD3_SAVE_CHECK(rgd3_bin_u32_le(&r) == size && rgd3_bin_u32_le(&r) == 0);
	block_size = rgd3_bin_u32_le(&r);
	count = rgd3_bin_u32_le(&r);
	RGD3_SAVE_CHECK(block_size == RGD3_SAVEGAME_BLOCK_SIZE && count == 3);
	header_size = r.pos;
	first_clen = rgd3_bin_u32_le(&r);
	RGD3_SAVE_CHECK(rgd3_bin_u32_le(&r) == RGD3_SAVEGAME_BLOCK_SIZE);
	block1_data = header_size + 12;
	block2_crc = block1_data + first_clen + 8;

	/* round trips */
	RGD3_SAVE_CHECK(rgd3_savegame_test_decode(enc, enc_size, payload, size) == 0);
	RGD3_SAVE_CHECK(rgd3_savegame_test_decode(empty, empty_size, payload, 0) == 0);

	/* corruption: one flipped byte anywhere that matters */
	bad = (u8*)rgd3_heap_alloc(enc_size + 1, RGD3_TAG_SAVEGAMES);
	{
		const size_t spots[] = { 0, 8, header_size - 9, header_size - 5, header_size, block1_data + 2, block1_data + first_clen / 2,
			block2_crc, enc_size / 2, enc_size - 1 };
		int i;
		for (i = 0; i < (int)RG_ARRAY_COUNT(spots); i++)
		{
			int error;
			memcpy(bad, enc, enc_size);
			bad[spots[i]] ^= 0x5A;
			error = rgd3_savegame_test_decode(bad, enc_size, payload, size);
			RGD3_SAVE_CHECK(error > 0);
			if (spots[i] != 8)
				RGD3_SAVE_CHECK(error == RGD3_SAVEGAME_E_CORRUPTED);
		}
		/* a newer format version */
		memcpy(bad, enc, enc_size);
		bad[8] = RGD3_SAVEGAME_FORMAT_VERSION + 1;
		RGD3_SAVE_CHECK(rgd3_savegame_test_decode(bad, enc_size, payload, size) == RGD3_SAVEGAME_E_INCOMPATIBLE_NEWER_VERSION);
	}
	/* truncated and padded */
	RGD3_SAVE_CHECK(rgd3_savegame_test_decode(enc, enc_size - 1, payload, size) == RGD3_SAVEGAME_E_CORRUPTED);
	RGD3_SAVE_CHECK(rgd3_savegame_test_decode(enc, enc_size / 2, payload, size) == RGD3_SAVEGAME_E_CORRUPTED);
	RGD3_SAVE_CHECK(rgd3_savegame_test_decode(enc, header_size, payload, size) == RGD3_SAVEGAME_E_CORRUPTED);
	RGD3_SAVE_CHECK(rgd3_savegame_test_decode(enc, 7, payload, size) == RGD3_SAVEGAME_E_CORRUPTED);
	RGD3_SAVE_CHECK(rgd3_savegame_test_decode(enc, 0, payload, size) == RGD3_SAVEGAME_E_CORRUPTED);
	memcpy(bad, enc, enc_size);
	bad[enc_size] = 0;
	RGD3_SAVE_CHECK(rgd3_savegame_test_decode(bad, enc_size + 1, payload, size) == RGD3_SAVEGAME_E_CORRUPTED);
	memcpy(bad, empty, empty_size);
	bad[empty_size] = 0;
	RGD3_SAVE_CHECK(rgd3_savegame_test_decode(bad, empty_size + 1, payload, 0) == RGD3_SAVEGAME_E_CORRUPTED);

	rgd3_heap_free(bad);
	rgd3_heap_free(empty);
	rgd3_heap_free(enc);
	rgd3_heap_free(payload);
}

static int rgd3_savegame_test_callbacks;
static int rgd3_savegame_test_cancelled;

static void rgd3_savegame_test_on_complete(Rgd3SaveHandle handle, const char* slot, int error, void* user)
{
	RG_UNUSED(handle);
	RG_UNUSED(slot);
	RG_UNUSED(user);
	rgd3_savegame_test_callbacks++;
	if (error & RGD3_SAVEGAME_E_CANCELLED)
		rgd3_savegame_test_cancelled++;
}

static int rgd3_savegame_test_finish(Rgd3SaveHandle h)
{
	u64 start = rgd3_time_us();
	while (!rgd3_savegame_is_completed(h))
	{
		rgd3_savegame_pump();
		if (rgd3_time_us() - start > 30000000u)
			return 0;
		SDL_Delay(1);
	}
	return 1;
}

/* writes the two game files plus details to a slot */
static Rgd3SaveHandle rgd3_savegame_test_save(const char* slot, const u8* data, size_t size, const char* map)
{
	Rgd3SaveFile files[2];
	Rgd3SaveGameDetails details = { 0 };
	Rgd3SaveHandle h;
	memset(files, 0, sizeof(files));
	files[0].name = RGD3_SAVEGAME_STRINGS_FILENAME;
	files[0].file = rgd3_fs_open_memory_write(files[0].name);
	files[0].type = RGD3_SAVEGAMEFILE_BINARY;
	rgd3_fs_write_string(files[0].file, "#str_02990");
	rgd3_fs_write_string(files[0].file, map);
	files[1].name = RGD3_SAVEGAME_GAMEDATA_FILENAME;
	files[1].file = rgd3_fs_open_memory_write(files[1].name);
	files[1].type = RGD3_SAVEGAMEFILE_BINARY;
	rgd3_fs_write(files[1].file, data, size);
	rgd3_dict_set(&details.descriptors, RGD3_SAVEGAME_DETAIL_FIELD_MAP, map);
	rgd3_dict_set_int(&details.descriptors, RGD3_SAVEGAME_DETAIL_FIELD_PLAYTIME, 1234);
	rgd3_dict_set_int(&details.descriptors, RGD3_SAVEGAME_DETAIL_FIELD_EXPANSION, 0);
	h = rgd3_savegame_save(slot, files, 2, &details);
	rgd3_savegame_details_clear(&details);
	rgd3_fs_close(files[0].file);
	rgd3_fs_close(files[1].file);
	return h;
}

/* loads a slot and compares gamedata.save with expect; returns error bits */
static int rgd3_savegame_test_load(const char* slot, const u8* expect, size_t size, int strings_type, Rgd3SaveGameDetails* details, int* same)
{
	Rgd3SaveFile files[2];
	int error;
	memset(files, 0, sizeof(files));
	files[0].name = RGD3_SAVEGAME_STRINGS_FILENAME;
	files[0].type = strings_type;
	files[1].name = RGD3_SAVEGAME_GAMEDATA_FILENAME;
	files[1].file = rgd3_fs_open_memory_write("reused");
	rgd3_fs_write_string(files[1].file, "stale contents");
	files[1].type = RGD3_SAVEGAMEFILE_BINARY;
	error = rgd3_savegame_load_sync(slot, files, 2, details);
	if (same)
	{
		*same = rgd3_fs_length(files[1].file) == (i64)size && memcmp(rgd3_fs_memory_data(files[1].file), expect, size) == 0;
		if (!files[0].error)
		{
			Rgd3StrBuf s = { 0 };
			rgd3_fs_read_string(files[0].file, &s);
			*same = *same && strcmp(rgd3_strbuf_cstr(&s), "#str_02990") == 0;
			rgd3_strbuf_free(&s);
		}
	}
	rgd3_fs_close(files[0].file);
	rgd3_fs_close(files[1].file);
	return error;
}

static const Rgd3SaveGameDetails* rgd3_savegame_test_find(const Rgd3SaveGameList* list, const char* slot)
{
	int i;
	for (i = 0; i < list->count; i++)
		if (!rgd3_str_icmp(list->items[i].slot_name, slot))
			return &list->items[i];
	return NULL;
}

static void rgd3_savegame_test_manager(void)
{
	const size_t size = 600123;
	u8* v1 = rgd3_savegame_test_payload(size, 1);
	u8* v2 = rgd3_savegame_test_payload(size, 2);
	u8* v3 = rgd3_savegame_test_payload(size + 7, 3);
	char dir[RGD3_MAX_OSPATH], path[RGD3_MAX_OSPATH];
	Rgd3SaveGameDetails details = { 0 };
	Rgd3SaveGameList list;
	const Rgd3SaveGameDetails* found;
	Rgd3SaveHandle h, ha, hb, hc;
	int error, same = 0, state;

	rgd3_savegame_delete(RGD3_SAVEGAME_TEST_SLOT);
	rgd3_savegame_delete(RGD3_SAVEGAME_TEST_SLOT_B);
	rgd3_savegame_slot_dir(RGD3_SAVEGAME_TEST_SLOT, dir, sizeof(dir));

	/* save -> pump -> load: byte-identical */
	h = rgd3_savegame_test_save(RGD3_SAVEGAME_TEST_SLOT, v1, size, "game/mars_city1");
	RGD3_SAVE_CHECK(h > 0);
	RGD3_SAVE_CHECK(rgd3_savegame_is_busy());
	RGD3_SAVE_CHECK(rgd3_savegame_test_finish(h));
	RGD3_SAVE_CHECK(rgd3_savegame_handle_state(h, &error) == RGD3_SAVE_STATE_DONE && error == 0);
	RGD3_SAVE_CHECK(!rgd3_savegame_is_busy());
	rg_snprintf(path, sizeof(path), "%s/" RGD3_SAVEGAME_GAMEDATA_FILENAME, dir);
	RGD3_SAVE_CHECK(rgd3_platform_is_file(path));
	rg_snprintf(path, sizeof(path), "%s/" RGD3_SAVEGAME_GAMEDATA_FILENAME ".tmp", dir);
	RGD3_SAVE_CHECK(!rgd3_platform_is_file(path));
	error = rgd3_savegame_test_load(RGD3_SAVEGAME_TEST_SLOT, v1, size, RGD3_SAVEGAMEFILE_BINARY, &details, &same);
	RGD3_SAVE_CHECK(error == 0 && same);
	RGD3_SAVE_CHECK(!details.damaged && !strcmp(details.slot_name, RGD3_SAVEGAME_TEST_SLOT));
	RGD3_SAVE_CHECK(!strcmp(rgd3_savegame_details_map_name(&details), "game/mars_city1"));
	RGD3_SAVE_CHECK(!strcmp(rgd3_savegame_details_language(&details), rgd3_cvar_get_string("sys_lang")));
	RGD3_SAVE_CHECK(rgd3_savegame_details_playtime(&details) == 1234);
	RGD3_SAVE_CHECK(rgd3_savegame_details_save_version(&details) == RGD3_SAVEGAME_VERSION);
	RGD3_SAVE_CHECK(details.date > 0);

	/* newest wins: B is still queued behind the delayed A when C arrives */
	rgd3_savegame_test_callbacks = 0;
	rgd3_savegame_test_cancelled = 0;
	rgd3_savegame_set_completed_callback(rgd3_savegame_test_on_complete, NULL);
	rgd3_cvar_set(&cv_savegame_winInduceDelay, "300", 0);
	ha = rgd3_savegame_test_save(RGD3_SAVEGAME_TEST_SLOT_B, v1, size, "game/admin");
	rgd3_cvar_set(&cv_savegame_winInduceDelay, "0", 0);
	hb = rgd3_savegame_test_save(RGD3_SAVEGAME_TEST_SLOT, v2, size, "game/alphalabs1");
	hc = rgd3_savegame_test_save(RGD3_SAVEGAME_TEST_SLOT, v3, size + 7, "game/alphalabs2");
	RGD3_SAVE_CHECK(ha > 0 && hb > ha && hc > hb);
	RGD3_SAVE_CHECK(rgd3_savegame_handle_state(hc, NULL) == RGD3_SAVE_STATE_PENDING);
	RGD3_SAVE_CHECK(rgd3_savegame_test_finish(ha) && rgd3_savegame_test_finish(hb) && rgd3_savegame_test_finish(hc));
	state = rgd3_savegame_handle_state(hb, &error);
	RGD3_SAVE_CHECK(state == RGD3_SAVE_STATE_DONE && error == RGD3_SAVEGAME_E_CANCELLED);
	RGD3_SAVE_CHECK(rgd3_savegame_handle_state(ha, &error) == RGD3_SAVE_STATE_DONE && error == 0);
	RGD3_SAVE_CHECK(rgd3_savegame_handle_state(hc, &error) == RGD3_SAVE_STATE_DONE && error == 0);
	RGD3_SAVE_CHECK(rgd3_savegame_test_callbacks == 3 && rgd3_savegame_test_cancelled == 1);
	rgd3_savegame_set_completed_callback(NULL, NULL);
	error = rgd3_savegame_test_load(RGD3_SAVEGAME_TEST_SLOT, v3, size + 7, RGD3_SAVEGAMEFILE_BINARY, &details, &same);
	RGD3_SAVE_CHECK(error == 0 && same && !strcmp(rgd3_savegame_details_map_name(&details), "game/alphalabs2"));
	RGD3_SAVE_CHECK(rgd3_savegame_handle_state(9999999, NULL) == RGD3_SAVE_STATE_UNKNOWN);
	RGD3_SAVE_CHECK(rgd3_savegame_is_completed(0));

	/* enumerate */
	RGD3_SAVE_CHECK(rgd3_savegame_enumerate(&list) == 0);
	found = rgd3_savegame_test_find(&list, RGD3_SAVEGAME_TEST_SLOT);
	RGD3_SAVE_CHECK(found && !found->damaged && !strcmp(rgd3_savegame_details_map_name(found), "game/alphalabs2"));
	found = rgd3_savegame_test_find(&list, RGD3_SAVEGAME_TEST_SLOT_B);
	RGD3_SAVE_CHECK(found && !found->damaged && !strcmp(rgd3_savegame_details_map_name(found), "game/admin"));
	rgd3_savegame_list_free(&list);

	/* on-disk damage: a flipped byte, then a truncated file */
	rg_snprintf(path, sizeof(path), "%s/" RGD3_SAVEGAME_GAMEDATA_FILENAME, dir);
	{
		size_t len = 0;
		u8* raw = (u8*)SDL_LoadFile(path, &len);
		RGD3_SAVE_CHECK(raw != NULL && len > 100);
		if (raw)
		{
			raw[len / 2] ^= 0x01;
			SDL_SaveFile(path, raw, len);
			error = rgd3_savegame_test_load(RGD3_SAVEGAME_TEST_SLOT, v3, size + 7, RGD3_SAVEGAMEFILE_BINARY, NULL, NULL);
			RGD3_SAVE_CHECK(error == RGD3_SAVEGAME_E_CORRUPTED);
			raw[len / 2] ^= 0x01;
			SDL_SaveFile(path, raw, len / 3);
			error = rgd3_savegame_test_load(RGD3_SAVEGAME_TEST_SLOT, v3, size + 7, RGD3_SAVEGAMEFILE_BINARY, NULL, NULL);
			RGD3_SAVE_CHECK(error == RGD3_SAVEGAME_E_CORRUPTED);
			SDL_SaveFile(path, raw, len);
			error = rgd3_savegame_test_load(RGD3_SAVEGAME_TEST_SLOT, v3, size + 7, RGD3_SAVEGAMEFILE_BINARY, NULL, &same);
			RGD3_SAVE_CHECK(error == 0 && same);
			SDL_free(raw);
		}
	}

	/* missing files: required -> corrupted, optional -> fine */
	rg_snprintf(path, sizeof(path), "%s/" RGD3_SAVEGAME_STRINGS_FILENAME, dir);
	RGD3_SAVE_CHECK(SDL_RemovePath(path));
	RGD3_SAVE_CHECK(rgd3_savegame_test_load(RGD3_SAVEGAME_TEST_SLOT, v3, size + 7, RGD3_SAVEGAMEFILE_BINARY, NULL, NULL) == RGD3_SAVEGAME_E_CORRUPTED);
	RGD3_SAVE_CHECK(rgd3_savegame_test_load(RGD3_SAVEGAME_TEST_SLOT, v3, size + 7, RGD3_SAVEGAMEFILE_BINARY | RGD3_SAVEGAMEFILE_OPTIONAL, NULL, &same) == 0 && same);

	/* edited details: checksum mismatch marks the slot damaged */
	rg_snprintf(path, sizeof(path), "%s/" RGD3_SAVEGAME_DETAILS_FILENAME, dir);
	{
		size_t len = 0;
		char* text = (char*)SDL_LoadFile(path, &len);
		char* at = text ? strstr(text, "alphalabs2") : NULL;
		RGD3_SAVE_CHECK(at != NULL);
		if (at)
		{
			at[9] = '3';
			SDL_SaveFile(path, text, len);
		}
		SDL_free(text);
	}
	RGD3_SAVE_CHECK(rgd3_savegame_enumerate(&list) == 0);
	found = rgd3_savegame_test_find(&list, RGD3_SAVEGAME_TEST_SLOT);
	RGD3_SAVE_CHECK(found && found->damaged);
	rgd3_savegame_list_free(&list);
	RGD3_SAVE_CHECK(rgd3_savegame_test_load(RGD3_SAVEGAME_TEST_SLOT, v3, size + 7, RGD3_SAVEGAMEFILE_OPTIONAL, NULL, NULL) & RGD3_SAVEGAME_E_CORRUPTED);
	RGD3_SAVE_CHECK(SDL_RemovePath(path));
	RGD3_SAVE_CHECK(rgd3_savegame_test_load(RGD3_SAVEGAME_TEST_SLOT, v3, size + 7, RGD3_SAVEGAMEFILE_OPTIONAL, NULL, NULL) == RGD3_SAVEGAME_E_FILE_NOT_FOUND);

	/* bad names never reach the disk */
	RGD3_SAVE_CHECK(rgd3_savegame_test_save("../evil", v1, 16, "x") == 0);
	RGD3_SAVE_CHECK(rgd3_savegame_test_save("a/b", v1, 16, "x") == 0);
	RGD3_SAVE_CHECK(rgd3_savegame_test_save("", v1, 16, "x") == 0);
	RGD3_SAVE_CHECK(rgd3_savegame_load_sync("..", NULL, 0, NULL) == RGD3_SAVEGAME_E_INVALID_FILENAME);
	RGD3_SAVE_CHECK(rgd3_savegame_delete("x\\y") == RGD3_SAVEGAME_E_INVALID_FILENAME);

	/* delete */
	RGD3_SAVE_CHECK(rgd3_savegame_delete(RGD3_SAVEGAME_TEST_SLOT) == 0);
	RGD3_SAVE_CHECK(rgd3_savegame_delete(RGD3_SAVEGAME_TEST_SLOT_B) == 0);
	RGD3_SAVE_CHECK(!rgd3_platform_is_dir(dir));
	RGD3_SAVE_CHECK(rgd3_savegame_delete(RGD3_SAVEGAME_TEST_SLOT) == RGD3_SAVEGAME_E_FOLDER_NOT_FOUND);
	RGD3_SAVE_CHECK(rgd3_savegame_test_load(RGD3_SAVEGAME_TEST_SLOT, v1, size, RGD3_SAVEGAMEFILE_BINARY, NULL, NULL) == RGD3_SAVEGAME_E_FOLDER_NOT_FOUND);
	error = rgd3_savegame_enumerate(&list);
	RGD3_SAVE_CHECK((error == 0 || error == RGD3_SAVEGAME_E_FOLDER_NOT_FOUND) && !rgd3_savegame_test_find(&list, RGD3_SAVEGAME_TEST_SLOT));
	rgd3_savegame_list_free(&list);

	/* a queued save cancelled by delete leaves no folder behind */
	rgd3_cvar_set(&cv_savegame_winInduceDelay, "200", 0);
	ha = rgd3_savegame_test_save(RGD3_SAVEGAME_TEST_SLOT_B, v1, 1000, "game/admin");
	rgd3_cvar_set(&cv_savegame_winInduceDelay, "0", 0);
	hb = rgd3_savegame_test_save(RGD3_SAVEGAME_TEST_SLOT, v1, 1000, "game/admin");
	RGD3_SAVE_CHECK(rgd3_savegame_delete(RGD3_SAVEGAME_TEST_SLOT) == RGD3_SAVEGAME_E_FOLDER_NOT_FOUND);
	RGD3_SAVE_CHECK(rgd3_savegame_handle_state(hb, &error) == RGD3_SAVE_STATE_DONE && error == RGD3_SAVEGAME_E_CANCELLED);
	RGD3_SAVE_CHECK(!rgd3_platform_is_dir(dir));
	RGD3_SAVE_CHECK(rgd3_savegame_test_finish(ha) && rgd3_savegame_delete(RGD3_SAVEGAME_TEST_SLOT_B) == 0);

	rgd3_savegame_details_clear(&details);
	rgd3_heap_free(v3);
	rgd3_heap_free(v2);
	rgd3_heap_free(v1);
}

static void rgd3_savegame_test_names(void)
{
	char out[64];
	char errors[256];
	rgd3_savegame_scrub_name("My Save: #1.sav", out, sizeof(out));
	RGD3_SAVE_CHECK(!strcmp(out, "My_Save___1"));
	rgd3_savegame_scrub_name("^1Red^7 Dawn", out, sizeof(out));
	RGD3_SAVE_CHECK(!strcmp(out, "Red_Dawn"));
	rgd3_savegame_scrub_name("caf\xE9", out, sizeof(out));
	RGD3_SAVE_CHECK(!strcmp(out, "caf_"));
	rgd3_savegame_scrub_name("a.b.c", out, sizeof(out));
	RGD3_SAVE_CHECK(!strcmp(out, "a_b"));
	rgd3_savegame_scrub_name("QUICKSAVE", out, 6);
	RGD3_SAVE_CHECK(!strcmp(out, "QUICK"));
	RGD3_SAVE_CHECK(rgd3_savegame_valid_name("AUTOSAVE", RGD3_SAVEGAME_MAX_FOLDER_NAME));
	RGD3_SAVE_CHECK(!rgd3_savegame_valid_name("..", RGD3_SAVEGAME_MAX_FOLDER_NAME));
	RGD3_SAVE_CHECK(!rgd3_savegame_valid_name("a:b", RGD3_SAVEGAME_MAX_FOLDER_NAME));
	RGD3_SAVE_CHECK(!strcmp(rgd3_savegame_error_string(RGD3_SAVEGAME_E_CORRUPTED | RGD3_SAVEGAME_E_FOLDER_NOT_FOUND, errors, sizeof(errors)),
		"SAVEGAME_E_CORRUPTED, SAVEGAME_E_FOLDER_NOT_FOUND"));
	RGD3_SAVE_CHECK(!strcmp(rgd3_savegame_error_string(0, errors, sizeof(errors)), ""));
}

int rgd3_savegame_self_test(void)
{
	i64 before[RG_ARRAY_COUNT(rgd3_savegame_test_tags)];
	i64 after[RG_ARRAY_COUNT(rgd3_savegame_test_tags)];
	rgd3_savegame_test_checks = 0;
	rgd3_savegame_test_failures = 0;
	rgd3_savegame_test_snapshot(before);
	rgd3_savegame_quiet = 1;
	rgd3_savegame_test_memory_files();
	rgd3_savegame_test_ini();
	rgd3_savegame_test_container();
	rgd3_savegame_test_names();
	if (rgd3_savegame.initialized)
		rgd3_savegame_test_manager();
	else
		RGD3_SAVE_CHECK(!"the save system is not initialised");
	rgd3_savegame_quiet = 0;
	rgd3_savegame_test_snapshot(after);
	RGD3_SAVE_CHECK(rgd3_savegame_test_same(before, after));
	rgd3_printf("savegame: %d checks, %d failures\n", rgd3_savegame_test_checks, rgd3_savegame_test_failures);
	return rgd3_savegame_test_failures == 0;
}
