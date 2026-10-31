/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_manifest.h - maps/<map>.preload (idPreloadManifest) and file
   manifests (idFileManifest), neo/framework/File_Manifest.h/.cpp.

   .preload: i32 BE entry count, string (LE length + bytes) manifest name,
   then per entry: i32 BE type, string resource name, i32 BE filter,
   repeat, usage, cubeMap (image preload data, zero for other types). */
#ifndef RGD3_MANIFEST_H
#define RGD3_MANIFEST_H
#ifndef RGD3_ALL_C
#error "rgd3 headers are included through src/all.c only"
#endif

typedef enum Rgd3PreloadType
{
	RGD3_PRELOAD_IMAGE,
	RGD3_PRELOAD_MODEL,
	RGD3_PRELOAD_SAMPLE,
	RGD3_PRELOAD_ANIM,
	RGD3_PRELOAD_COLLISION,
	RGD3_PRELOAD_PARTICLE
} Rgd3PreloadType;

typedef struct Rgd3PreloadEntry
{
	int res_type;
	char* resource_name;
	int filter;
	int repeat;
	int usage;
	int cube_map;
} Rgd3PreloadEntry;

typedef struct Rgd3PreloadManifest
{
	char* filename;
	Rgd3PreloadEntry* entries;
	int num_entries;
} Rgd3PreloadManifest;

int rgd3_preload_manifest_load(Rgd3PreloadManifest* m, const char* file_name);
void rgd3_preload_manifest_free(Rgd3PreloadManifest* m);
const char* rgd3_preload_type_name(int res_type);
void rgd3_preload_manifest_name(const char* map_name, char* out, size_t size);

typedef struct Rgd3FileManifest
{
	char* filename;
	char** table;
	int num;
	Rgd3HashIndex hash;
} Rgd3FileManifest;

/* i32 BE count, then that many strings */
int rgd3_file_manifest_load(Rgd3FileManifest* m, const char* file_name);
void rgd3_file_manifest_free(Rgd3FileManifest* m);
int rgd3_file_manifest_find(const Rgd3FileManifest* m, const char* file_name);

void rgd3_manifest_register(void);

#endif
