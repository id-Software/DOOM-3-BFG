/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_mapfile.h - .map files (idMapFile, neo/idlib/MapFile.h/.cpp): entities
   with key/value pairs and their brush / patch primitives, in file order.
   Version 2 (brushDef3/patchDef3) plus the older brushDef/patchDef2 and
   Q3-style brushes. No validation is done. Parse errors drop (the
   original's lexer flags make them fatal); the partly parsed data stays in
   the Rgd3MapFile (pending_* included) for rgd3_mapfile_free, or is freed by
   the unwind registry when the map itself was on an unwound stack frame.
   Writing .map files is not ported (editor/dmap). */
#ifndef RGD3_MAPFILE_H
#define RGD3_MAPFILE_H
#ifndef RGD3_ALL_C
#error "rgd3 headers are included through src/all.c only"
#endif

#define RGD3_OLD_MAP_VERSION 1
#define RGD3_CURRENT_MAP_VERSION 2

typedef enum Rgd3MapPrimitiveType
{
	RGD3_MAP_PRIM_INVALID = -1,
	RGD3_MAP_PRIM_BRUSH,
	RGD3_MAP_PRIM_PATCH
} Rgd3MapPrimitiveType;

typedef struct Rgd3MapPrimitive
{
	Rgd3MapPrimitiveType type;
	Rgd3Dict epairs;
} Rgd3MapPrimitive;

typedef struct Rgd3MapBrushSide
{
	char* material;
	Rgd3Plane plane;
	Rgd3Vec3 tex_mat[2];
	Rgd3Vec3 origin;
} Rgd3MapBrushSide;

typedef struct Rgd3MapBrush
{
	Rgd3MapPrimitive prim; /* first */
	Rgd3MapBrushSide* sides;
	int num_sides;
} Rgd3MapBrush;

/* control points of an idSurface_Patch: verts[row * width + column] */
typedef struct Rgd3MapPatch
{
	Rgd3MapPrimitive prim; /* first */
	char* material;
	int horz_subdivisions;
	int vert_subdivisions;
	int explicitly_subdivided;
	int width;
	int height;
	int max_width;
	int max_height;
	Rgd3DrawVert* verts;
} Rgd3MapPatch;

typedef struct Rgd3MapEntity
{
	Rgd3Dict epairs;
	Rgd3MapPrimitive** primitives;
	int num_primitives;
	int alloced;
} Rgd3MapEntity;

typedef struct Rgd3MapFile
{
	float version;
	Rgd3TimeStamp file_time;
	u32 geometry_crc;
	Rgd3MapEntity** entities;
	int num_entities;
	int alloced;
	char name[RGD3_MAX_OSPATH]; /* without extension */
	int has_primitive_data;
	/* parsed but not yet attached; freed by rgd3_mapfile_free, so a drop
	   during parsing leaks nothing */
	Rgd3MapEntity* pending_entity;
	Rgd3MapPrimitive* pending_prim;
} Rgd3MapFile;

/* filename needs no extension; a .reg file is preferred unless ignore_region */
int rgd3_mapfile_parse(Rgd3MapFile* map, const char* filename, int ignore_region, int os_path);
void rgd3_mapfile_free(Rgd3MapFile* map);
Rgd3MapEntity* rgd3_mapfile_find_entity(Rgd3MapFile* map, const char* name);
void rgd3_mapfile_remove_entities(Rgd3MapFile* map, const char* classname);
void rgd3_mapfile_remove_primitive_data(Rgd3MapFile* map);
void rgd3_map_brush_side_texture_vectors(const Rgd3MapBrushSide* side, Rgd3Vec4 v[2]);
u32 rgd3_map_entity_geometry_crc(const Rgd3MapEntity* ent);

void rgd3_mapfile_register(void);

#endif
