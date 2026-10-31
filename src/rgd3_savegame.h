/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_savegame.h - save game storage: slot folders, the compressed block
   container, the game.details descriptor file and an asynchronous writer.
   It replaces idSaveGameManager / idSaveGameThread / the session save
   processors (neo/sys/sys_savegame.h/.cpp, neo/sys/sys_session_savegames.cpp,
   neo/sys/win32/win_savegame.cpp) and idFile_SaveGamePipelined
   (neo/framework/File_SaveGame.cpp). The error bits, file type bits and detail field names keep the
   original values and names behind an RGD3_ prefix, the cvar and command
   names are the original ones; the on-disk format
   is rgd3's own (original saves are not loadable, PLAN.md).

   Layout: <fs_savepath>/<fs_game or base>/saves/<slot>/
	gamedata.save, gamedata.strings   binary files in the container below
	game.details                      INI text (rgd3_dict_write_ini) with
	                                  the descriptors plus language,
	                                  saveVersion and checksum

   Container (all integers little-endian, idFile encodings):
	char[8]  "RGD3SAVE"
	u32      format version (RGD3_SAVEGAME_FORMAT_VERSION)
	string   engine build: i32 length + bytes, at most 255
	u64      uncompressed size
	u32      block size (262144)
	u32      block count = ceil(size / block size)
	blocks   u32 compressed size, u32 uncompressed size, u32 CRC-32 of the
	         uncompressed bytes, then a zlib stream (deflate + adler32)
	end of file directly after the last block
   Anything inconsistent (magic, sizes, counts, inflate or adler failure,
   CRC mismatch, short or trailing bytes) loads as RGD3_SAVEGAME_E_CORRUPTED; a
   newer format version as RGD3_SAVEGAME_E_INCOMPATIBLE_NEWER_VERSION.

   Threads: everything here is main-thread API. rgd3_savegame_save copies
   the data and queues it for the writer thread, which only compresses and
   writes files through plain SDL IO into paths computed on the main thread
   (each file to <name>.tmp, then all renamed, game.details last). A save
   still queued for the same slot is replaced by a newer one (it completes
   as RGD3_SAVEGAME_E_CANCELLED). Load, enumerate and delete are synchronous and
   first wait for queued writes, as the original's serial processor queue
   does. rgd3_savegame_pump (once per frame) completes finished saves:
   errors are printed and the completion callback runs on the main thread.
   Nothing here drops. */
#ifndef RGD3_SAVEGAME_H
#define RGD3_SAVEGAME_H
#ifndef RGD3_ALL_C
#error "rgd3 headers are included through src/all.c only"
#endif

#define RGD3_SAVEGAME_MAX_FILES 10        /* MAX_FILES_WITHIN_SAVEGAME */
#define RGD3_SAVEGAME_MAX_FOLDER_NAME 64  /* MAX_FOLDER_NAME_LENGTH */
#define RGD3_SAVEGAME_MAX_FILE_NAME 64
#define RGD3_SAVEGAME_DIR "saves"
#define RGD3_SAVEGAME_DETAILS_FILENAME "game.details"
#define RGD3_SAVEGAME_GAMEDATA_FILENAME "gamedata.save"
#define RGD3_SAVEGAME_STRINGS_FILENAME "gamedata.strings"
#define RGD3_SAVEGAME_AUTOSAVE_FOLDER "AUTOSAVE"

#define RGD3_SAVEGAME_MAGIC "RGD3SAVE"
#define RGD3_SAVEGAME_FORMAT_VERSION 1
#define RGD3_SAVEGAME_BLOCK_SIZE (256 * 1024)
/* saveVersion written into game.details when the game does not set one;
   a save with a larger saveVersion is refused (BUILD_NUMBER's role) */
#define RGD3_SAVEGAME_VERSION 1

/* common descriptors for savegame description fields (sys_savegame.h) */
#define RGD3_SAVEGAME_DETAIL_FIELD_EXPANSION "expansion"
#define RGD3_SAVEGAME_DETAIL_FIELD_MAP "mapName"
#define RGD3_SAVEGAME_DETAIL_FIELD_MAP_LOCATE "mapLocation"
#define RGD3_SAVEGAME_DETAIL_FIELD_DIFFICULTY "difficulty"
#define RGD3_SAVEGAME_DETAIL_FIELD_PLAYTIME "playTime"
#define RGD3_SAVEGAME_DETAIL_FIELD_LANGUAGE "language"
#define RGD3_SAVEGAME_DETAIL_FIELD_SAVE_VERSION "saveVersion"
#define RGD3_SAVEGAME_DETAIL_FIELD_CHECKSUM "checksum"

/* saveGameError_t */
enum
{
	RGD3_SAVEGAME_E_NONE = 0,
	RGD3_SAVEGAME_E_CANCELLED = 1 << 0,
	RGD3_SAVEGAME_E_INSUFFICIENT_ROOM = 1 << 1,
	RGD3_SAVEGAME_E_CORRUPTED = 1 << 2,
	RGD3_SAVEGAME_E_UNABLE_TO_SELECT_STORAGE_DEVICE = 1 << 3,
	RGD3_SAVEGAME_E_UNKNOWN = 1 << 4,
	RGD3_SAVEGAME_E_INVALID_FILENAME = 1 << 5,
	RGD3_SAVEGAME_E_STEAM_ERROR = 1 << 6,
	RGD3_SAVEGAME_E_FOLDER_NOT_FOUND = 1 << 7,
	RGD3_SAVEGAME_E_FILE_NOT_FOUND = 1 << 8,
	RGD3_SAVEGAME_E_DLC_NOT_FOUND = 1 << 9,
	RGD3_SAVEGAME_E_INVALID_USER = 1 << 10,
	RGD3_SAVEGAME_E_PROFILE_TOO_BIG = 1 << 11,
	RGD3_SAVEGAME_E_DISC_SWAP = 1 << 12,
	RGD3_SAVEGAME_E_INCOMPATIBLE_NEWER_VERSION = 1 << 13,
	RGD3_SAVEGAME_E_BITS_USED = 14
};

/* saveGameType_t (the bits rgd3 uses; values unchanged). TEXT files are
   stored as they are; everything else goes into the container. */
enum
{
	RGD3_SAVEGAMEFILE_NONE = 0,
	RGD3_SAVEGAMEFILE_TEXT = 1 << 0,
	RGD3_SAVEGAMEFILE_BINARY = 1 << 1,
	RGD3_SAVEGAMEFILE_COMPRESSED = 1 << 2,
	RGD3_SAVEGAMEFILE_OPTIONAL = 1 << 7 /* missing on load is not an error */
};

typedef int Rgd3SaveHandle; /* saveGameHandle_t: 0 = none / failed to start */

/* one file of a save (idFile_SaveGame) */
typedef struct Rgd3SaveFile
{
	const char* name; /* in the slot folder, e.g. RGD3_SAVEGAME_GAMEDATA_FILENAME */
	Rgd3File* file;   /* memory file: save copies its contents; load fills it
	                     (created by load when NULL, the caller closes it)
	                     and leaves it read-only at offset 0 */
	int type;         /* RGD3_SAVEGAMEFILE_* */
	int error;        /* [out] load: this file was missing or bad */
} Rgd3SaveFile;

/* idSaveGameDetails. Zero-initialise; free with rgd3_savegame_details_clear. */
typedef struct Rgd3SaveGameDetails
{
	Rgd3Dict descriptors;
	int damaged;   /* details unreadable or checksum mismatch */
	i64 date;      /* seconds since 1970 (game.details modify time) */
	char slot_name[RGD3_SAVEGAME_MAX_FOLDER_NAME];
} Rgd3SaveGameDetails;

typedef struct Rgd3SaveGameList
{
	Rgd3SaveGameDetails* items; /* newest first */
	int count;
} Rgd3SaveGameList;

enum
{
	RGD3_SAVE_STATE_UNKNOWN, /* no such handle (or older than the result history) */
	RGD3_SAVE_STATE_PENDING, /* queued, writing, or finished but not pumped yet */
	RGD3_SAVE_STATE_DONE
};

/* main thread, from rgd3_savegame_pump, once per finished save */
typedef void (*Rgd3SaveCompletedFn)(Rgd3SaveHandle handle, const char* slot, int error, void* user);

void rgd3_savegame_register(void);
int rgd3_savegame_init(void);      /* after rgd3_fs_init: starts the writer thread */
void rgd3_savegame_shutdown(void); /* waits for queued writes; idempotent */
void rgd3_savegame_pump(void);
int rgd3_savegame_is_busy(void);   /* anything queued, writing or unpumped */
void rgd3_savegame_wait(void);     /* blocks until the queue is empty, then pumps */
void rgd3_savegame_set_completed_callback(Rgd3SaveCompletedFn fn, void* user);

/* Non-blocking. Copies the files' data and the details (adding language,
   saveVersion if absent, and the checksum) and queues the write. Returns 0
   (after printing why) for a bad slot or file name, too many files, or
   saveGame_enable 0. */
Rgd3SaveHandle rgd3_savegame_save(const char* slot, const Rgd3SaveFile* files, int num_files, const Rgd3SaveGameDetails* details);
int rgd3_savegame_handle_state(Rgd3SaveHandle handle, int* error);
int rgd3_savegame_is_completed(Rgd3SaveHandle handle); /* IsSaveGameCompletedFromHandle */

/* Blocking. Reads, checks and decompresses the files into their memory
   files and reads game.details into details (may be NULL). Returns the
   RGD3_SAVEGAME_E_* bits, 0 on success. */
int rgd3_savegame_load_sync(const char* slot, Rgd3SaveFile* files, int num_files, Rgd3SaveGameDetails* details);
/* Blocking. Every slot with a game.details, newest first. Returns error
   bits (RGD3_SAVEGAME_E_FOLDER_NOT_FOUND when there is no saves folder; the list
   is then empty). Free with rgd3_savegame_list_free. */
int rgd3_savegame_enumerate(Rgd3SaveGameList* list);
void rgd3_savegame_list_free(Rgd3SaveGameList* list);
/* Blocking. Cancels queued saves to the slot, then removes its folder. */
int rgd3_savegame_delete(const char* slot);

/* idCommonLocal::ScrubSaveGameFileName: colours and extension removed,
   spaces, punctuation and bytes >= 128 become '_' */
void rgd3_savegame_scrub_name(const char* in, char* out, size_t size);
/* GetSaveGameErrorString: comma separated RGD3_SAVEGAME_E_* names */
const char* rgd3_savegame_error_string(int error_mask, char* buf, size_t size);

void rgd3_savegame_details_clear(Rgd3SaveGameDetails* details);
void rgd3_savegame_details_copy(Rgd3SaveGameDetails* dst, const Rgd3SaveGameDetails* src);
/* SavegameReadDetailsFromFile: reads the INI and sets damaged when it does
   not parse or the checksum does not match */
void rgd3_savegame_details_read(Rgd3SaveGameDetails* details, Rgd3File* file);
const char* rgd3_savegame_details_map_name(const Rgd3SaveGameDetails* details);
const char* rgd3_savegame_details_language(const Rgd3SaveGameDetails* details);
int rgd3_savegame_details_playtime(const Rgd3SaveGameDetails* details);
int rgd3_savegame_details_save_version(const Rgd3SaveGameDetails* details);

/* The container on its own (any thread). encode returns a heap block
   (RGD3_TAG_SAVEGAMES, free with rgd3_heap_free) or NULL; decode appends
   the payload to a writable memory file and returns RGD3_SAVEGAME_E_* bits. */
u8* rgd3_savegame_encode(const void* data, size_t size, size_t* out_size);
int rgd3_savegame_decode(const u8* data, size_t size, Rgd3File* out, int verify_crc);

int rgd3_savegame_self_test(void); /* 1 on pass */

#endif
