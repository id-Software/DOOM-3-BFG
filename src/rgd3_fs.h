/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_fs.h - virtual filesystem (idFileSystem, neo/framework/FileSystem.cpp)
   over loose directories and BFG .resources containers
   (neo/framework/File_Resource.cpp).

   Container layout (verified against the Steam data):
	header  u32 BE magic 0xD000000D, u32 BE table offset, u32 BE table length
	table   u32 BE entry count, then per entry:
	        u32 LE name length, name bytes (no NUL), u32 BE offset, u32 BE length
	data    uncompressed, starting at offset 12
   Names are lowercased with '/' separators at mount. Later containers win.

   Search order: with fs_resourceLoadPriority 1 (default) containers are
   searched before loose directories. Search paths are basepath then savepath
   (then fs_game for each); later paths win. All writes go to savepath. */
#ifndef RGD3_FS_H
#define RGD3_FS_H
#ifndef RGD3_ALL_C
#error "rgd3 headers are included through src/all.c only"
#endif

#define RGD3_RESOURCE_MAGIC 0xD000000Du
#define RGD3_MAX_CONTAINERS 64
#define RGD3_MAX_SEARCH_PATHS 8

typedef struct Rgd3File Rgd3File;

typedef struct Rgd3FileList
{
	int count;
	char** names;
} Rgd3FileList;

typedef enum Rgd3FsSeek
{
	RGD3_FS_SEEK_SET,
	RGD3_FS_SEEK_CUR,
	RGD3_FS_SEEK_END
} Rgd3FsSeek;

void rgd3_fs_register(void);
int rgd3_fs_init(void);
void rgd3_fs_shutdown(void);
const char* rgd3_fs_basepath(void);
const char* rgd3_fs_savepath(void);
void rgd3_fs_build_os_path(const char* base, const char* gamedir, const char* relative, char* out, size_t size);

int rgd3_fs_read_file(const char* relative, void** buffer, Rgd3TimeStamp* timestamp);
void rgd3_fs_free_file(void* buffer);
int rgd3_fs_write_file(const char* relative, const void* data, size_t size);
int rgd3_fs_file_exists(const char* relative);

Rgd3File* rgd3_fs_open_read(const char* relative);
Rgd3File* rgd3_fs_open_write(const char* relative);
Rgd3File* rgd3_fs_open_explicit_read(const char* os_path);
size_t rgd3_fs_read(Rgd3File* file, void* dst, size_t size);
size_t rgd3_fs_write(Rgd3File* file, const void* src, size_t size);
int rgd3_fs_printf(Rgd3File* file, const char* fmt, ...);
i64 rgd3_fs_length(Rgd3File* file);
i64 rgd3_fs_tell(Rgd3File* file);
int rgd3_fs_seek(Rgd3File* file, i64 offset, Rgd3FsSeek whence);
const char* rgd3_fs_file_name(Rgd3File* file);
void rgd3_fs_close(Rgd3File* file);

Rgd3FileList* rgd3_fs_list_files(const char* dir, const char* ext, int full_relative, int tree);
void rgd3_fs_free_file_list(Rgd3FileList* list);

int rgd3_fs_mount_container(const char* os_path, int in_memory); /* index, or -1 on failure */
int rgd3_fs_unmount_container(const char* file_name);
int rgd3_fs_num_containers(void);
u32 rgd3_fs_container_entry_count(int index);
const char* rgd3_fs_container_path(int index);
int rgd3_fs_load_map_resources(const char* map_name);
void rgd3_fs_unload_map_resources(void);

#endif
