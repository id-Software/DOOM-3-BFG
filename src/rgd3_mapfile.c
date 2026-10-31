/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_mapfile.c - see rgd3_mapfile.h. Lifted from neo/idlib/MapFile.cpp:
   idMapPatch::Parse, idMapBrush::Parse/ParseQ3, idMapEntity::Parse,
   idMapFile::Parse (Version line, worldspawn removeEntities /
   overrideMaterial / forceEntityNames / moveFuncGroups), the geometry CRC
   (FloatCRC/StringCRC) and ComputeAxisBase/GetTextureVectors. */

#define RGD3_MAP_LEXER_FLAGS (LEXFL_NOSTRINGCONCAT | LEXFL_NOSTRINGESCAPECHARS | LEXFL_ALLOWPATHNAMES)

static u32 rgd3_map_float_crc(float f)
{
	union { float f; u32 u; } bits;
	bits.f = f;
	return bits.u;
}

static u32 rgd3_map_string_crc(const char* str)
{
	u32 i, crc = 0;
	for (i = 0; str[i]; i++)
		crc ^= (u32)((int)(signed char)str[i] << (i & 3));
	return crc;
}

/* rotation (0, RotY, RotZ) maps X to the normal */
static void rgd3_map_compute_axis_base(Rgd3Vec3 normal, Rgd3Vec3* tex_s, Rgd3Vec3* tex_t)
{
	float rot_y, rot_z;
	Rgd3Vec3 n;
	n.x = fabsf(normal.x) < 1e-6f ? 0.0f : normal.x;
	n.y = fabsf(normal.y) < 1e-6f ? 0.0f : normal.y;
	n.z = fabsf(normal.z) < 1e-6f ? 0.0f : normal.z;
	rot_y = (float)-atan2(n.z, sqrtf(n.y * n.y + n.x * n.x));
	rot_z = (float)atan2(n.y, n.x);
	tex_s->x = (float)-sin(rot_z);
	tex_s->y = (float)cos(rot_z);
	tex_s->z = 0.0f;
	tex_t->x = (float)(-sin(rot_y) * cos(rot_z));
	tex_t->y = (float)(-sin(rot_y) * sin(rot_z));
	tex_t->z = (float)-cos(rot_y);
}

void rgd3_map_brush_side_texture_vectors(const Rgd3MapBrushSide* side, Rgd3Vec4 v[2])
{
	Rgd3Vec3 tex_x, tex_y;
	int i;
	rgd3_map_compute_axis_base(rgd3_plane_normal(side->plane), &tex_x, &tex_y);
	for (i = 0; i < 2; i++)
	{
		v[i].x = tex_x.x * side->tex_mat[i].x + tex_y.x * side->tex_mat[i].y;
		v[i].y = tex_x.y * side->tex_mat[i].x + tex_y.y * side->tex_mat[i].y;
		v[i].z = tex_x.z * side->tex_mat[i].x + tex_y.z * side->tex_mat[i].y;
		v[i].w = side->tex_mat[i].z + (side->origin.x * v[i].x + side->origin.y * v[i].y + side->origin.z * v[i].z);
	}
}

/* ---- primitives ---- */

static char* rgd3_map_material(const char* token, float version)
{
	char buf[RGD3_MAX_STRING_CHARS];
	/* the old format had an implicit 'textures/' */
	if (version < 2.0f)
	{
		rg_snprintf(buf, sizeof(buf), "textures/%s", token);
		return rgd3_heap_strdup(buf, RGD3_TAG_GAME);
	}
	return rgd3_heap_strdup(token, RGD3_TAG_GAME);
}

static void rgd3_map_primitive_free(Rgd3MapPrimitive* prim)
{
	int i;
	if (!prim)
		return;
	rgd3_dict_clear(&prim->epairs);
	if (prim->type == RGD3_MAP_PRIM_BRUSH)
	{
		Rgd3MapBrush* brush = (Rgd3MapBrush*)prim;
		for (i = 0; i < brush->num_sides; i++)
			rgd3_heap_free(brush->sides[i].material);
		if (brush->sides)
			rgd3_heap_free(brush->sides);
	}
	else if (prim->type == RGD3_MAP_PRIM_PATCH)
	{
		Rgd3MapPatch* patch = (Rgd3MapPatch*)prim;
		if (patch->material)
			rgd3_heap_free(patch->material);
		if (patch->verts)
			rgd3_heap_free(patch->verts);
	}
	rgd3_heap_free(prim);
}

static Rgd3MapPatch* rgd3_map_patch_parse(Rgd3MapFile* map, Rgd3Lexer* src, Rgd3Vec3 origin, int patch_def3, float version)
{
	Rgd3Token token = { 0 };
	Rgd3Token value = { 0 };
	float info[7];
	Rgd3MapPatch* patch;
	int i, j;

	if (!rgd3_lexer_expect_token_string(src, "{"))
		return NULL;
	/* the material */
	if (!rgd3_lexer_read_token(src, &token))
	{
		rgd3_lexer_error(src, "idMapPatch::Parse: unexpected EOF");
		return NULL;
	}
	if (!rgd3_lexer_parse_1d_matrix(src, patch_def3 ? 7 : 5, info))
	{
		rgd3_lexer_error(src, patch_def3 ? "idMapPatch::Parse: unable to Parse patchDef3 info" : "idMapPatch::Parse: unable to parse patchDef2 info");
		return NULL;
	}

	patch = (Rgd3MapPatch*)rgd3_heap_calloc(sizeof(Rgd3MapPatch), RGD3_TAG_GAME);
	patch->prim.type = RGD3_MAP_PRIM_PATCH;
	map->pending_prim = &patch->prim;
	patch->max_width = (int)info[0];
	patch->max_height = (int)info[1];
	patch->width = (int)info[0];
	patch->height = (int)info[1];
	patch->material = rgd3_map_material(token.data, version);
	if (patch_def3)
	{
		patch->horz_subdivisions = (int)info[2];
		patch->vert_subdivisions = (int)info[3];
		patch->explicitly_subdivided = 1;
	}
	if (patch->width < 0 || patch->height < 0)
	{
		rgd3_lexer_error(src, "idMapPatch::Parse: bad size");
		return NULL;
	}
	patch->verts = (Rgd3DrawVert*)rgd3_heap_calloc((size_t)(patch->max_width * patch->max_height + 1) * sizeof(Rgd3DrawVert), RGD3_TAG_GAME);
	for (i = 0; i < patch->max_width * patch->max_height; i++)
		rgd3_drawvert_clear(&patch->verts[i]);

	/* "these were written out in the wrong order, IMHO" */
	if (!rgd3_lexer_expect_token_string(src, "("))
	{
		rgd3_lexer_error(src, "idMapPatch::Parse: bad patch vertex data");
		return NULL;
	}
	for (j = 0; j < patch->width; j++)
	{
		if (!rgd3_lexer_expect_token_string(src, "("))
		{
			rgd3_lexer_error(src, "idMapPatch::Parse: bad vertex row data");
			return NULL;
		}
		for (i = 0; i < patch->height; i++)
		{
			float v[5];
			Rgd3DrawVert* vert;
			if (!rgd3_lexer_parse_1d_matrix(src, 5, v))
			{
				rgd3_lexer_error(src, "idMapPatch::Parse: bad vertex column data");
				return NULL;
			}
			vert = &patch->verts[i * patch->width + j];
			vert->xyz.x = v[0] - origin.x;
			vert->xyz.y = v[1] - origin.y;
			vert->xyz.z = v[2] - origin.z;
			rgd3_drawvert_set_texcoord(vert, v[3], v[4]);
		}
		if (!rgd3_lexer_expect_token_string(src, ")"))
		{
			rgd3_lexer_error(src, "idMapPatch::Parse: unable to parse patch control points");
			return NULL;
		}
	}
	if (!rgd3_lexer_expect_token_string(src, ")"))
	{
		rgd3_lexer_error(src, "idMapPatch::Parse: unable to parse patch control points, no closure");
		return NULL;
	}

	/* any key/value pairs */
	while (rgd3_lexer_read_token(src, &token))
	{
		if (rgd3_token_is(&token, "}"))
		{
			rgd3_lexer_expect_token_string(src, "}");
			break;
		}
		if (token.type == TT_STRING)
		{
			rgd3_lexer_expect_token_type(src, TT_STRING, 0, &value);
			rgd3_dict_set(&patch->prim.epairs, token.data, value.data);
		}
	}
	rgd3_token_free(&token);
	rgd3_token_free(&value);
	return patch;
}

static Rgd3MapBrushSide* rgd3_map_brush_add_side(Rgd3MapBrush* brush)
{
	Rgd3MapBrushSide* side;
	if ((brush->num_sides & 7) == 0)
		brush->sides = (Rgd3MapBrushSide*)rgd3_heap_realloc(brush->sides, (size_t)(brush->num_sides + 8) * sizeof(Rgd3MapBrushSide), RGD3_TAG_GAME);
	side = &brush->sides[brush->num_sides++];
	memset(side, 0, sizeof(*side));
	return side;
}

/* the original builds the side list first and copies it into a brush; the
   brush is allocated up front here and freed on error */
static Rgd3MapBrush* rgd3_map_brush_parse(Rgd3MapFile* map, Rgd3Lexer* src, Rgd3Vec3 origin, int new_format, float version)
{
	Rgd3Token token = { 0 };
	Rgd3Token value = { 0 };
	Rgd3MapBrush* brush;
	Rgd3MapBrushSide* side;
	Rgd3Vec3 planepts[3];

	if (!rgd3_lexer_expect_token_string(src, "{"))
		return NULL;
	brush = (Rgd3MapBrush*)rgd3_heap_calloc(sizeof(Rgd3MapBrush), RGD3_TAG_GAME);
	brush->prim.type = RGD3_MAP_PRIM_BRUSH;
	map->pending_prim = &brush->prim;

	for (;;)
	{
		if (!rgd3_lexer_read_token(src, &token))
		{
			rgd3_lexer_error(src, "idMapBrush::Parse: unexpected EOF");
			return NULL;
		}
		if (rgd3_token_is(&token, "}"))
			break;

		/* brush epairs (editor only) come before the sides */
		for (;;)
		{
			if (rgd3_token_is(&token, "("))
				break;
			if (token.type != TT_STRING)
			{
				rgd3_lexer_error(src, "idMapBrush::Parse: unexpected %s, expected ( or epair key string", token.data);
				return NULL;
			}
			if (!rgd3_lexer_read_token_on_line(src, &value) || value.type != TT_STRING)
			{
				rgd3_lexer_error(src, "idMapBrush::Parse: expected epair value string not found");
				return NULL;
			}
			rgd3_dict_set(&brush->prim.epairs, token.data, value.data);
			if (!rgd3_lexer_read_token(src, &token))
			{
				rgd3_lexer_error(src, "idMapBrush::Parse: unexpected EOF");
				return NULL;
			}
		}
		rgd3_lexer_unread_token(src, &token);

		side = rgd3_map_brush_add_side(brush);
		if (new_format)
		{
			if (!rgd3_lexer_parse_1d_matrix(src, 4, side->plane.v))
			{
				rgd3_lexer_error(src, "idMapBrush::Parse: unable to read brush side plane definition");
				return NULL;
			}
		}
		else
		{
			/* three point plane definition */
			if (!rgd3_lexer_parse_1d_matrix(src, 3, planepts[0].v) || !rgd3_lexer_parse_1d_matrix(src, 3, planepts[1].v) ||
				!rgd3_lexer_parse_1d_matrix(src, 3, planepts[2].v))
			{
				rgd3_lexer_error(src, "idMapBrush::Parse: unable to read brush side plane definition");
				return NULL;
			}
			planepts[0] = rgd3_vec3_sub(planepts[0], origin);
			planepts[1] = rgd3_vec3_sub(planepts[1], origin);
			planepts[2] = rgd3_vec3_sub(planepts[2], origin);
			rgd3_plane_from_points(&side->plane, planepts[0], planepts[1], planepts[2], 1);
		}

		/* the texture matrix, 2D relative to the default planar axis */
		if (!rgd3_lexer_parse_2d_matrix(src, 2, 3, side->tex_mat[0].v))
		{
			rgd3_lexer_error(src, "idMapBrush::Parse: unable to read brush side texture matrix");
			return NULL;
		}
		side->origin = origin;

		if (!rgd3_lexer_read_token_on_line(src, &token))
		{
			rgd3_lexer_error(src, "idMapBrush::Parse: unable to read brush side material");
			return NULL;
		}
		side->material = rgd3_map_material(token.data, version);

		/* Q2 flags and values: read and ignored */
		if (rgd3_lexer_read_token_on_line(src, &token) && rgd3_lexer_read_token_on_line(src, &token))
			rgd3_lexer_read_token_on_line(src, &token);
	}

	if (!rgd3_lexer_expect_token_string(src, "}"))
	{
		brush = NULL;
	}
	rgd3_token_free(&token);
	rgd3_token_free(&value);
	return brush;
}

static Rgd3MapBrush* rgd3_map_brush_parse_q3(Rgd3MapFile* map, Rgd3Lexer* src, Rgd3Vec3 origin)
{
	Rgd3Token token = { 0 };
	Rgd3MapBrush* brush = (Rgd3MapBrush*)rgd3_heap_calloc(sizeof(Rgd3MapBrush), RGD3_TAG_GAME);
	Rgd3Vec3 planepts[3];
	brush->prim.type = RGD3_MAP_PRIM_BRUSH;
	map->pending_prim = &brush->prim;

	for (;;)
	{
		Rgd3MapBrushSide* side;
		if (rgd3_lexer_check_token_string(src, "}"))
			break;
		side = rgd3_map_brush_add_side(brush);
		if (!rgd3_lexer_parse_1d_matrix(src, 3, planepts[0].v) || !rgd3_lexer_parse_1d_matrix(src, 3, planepts[1].v) ||
			!rgd3_lexer_parse_1d_matrix(src, 3, planepts[2].v))
		{
			rgd3_lexer_error(src, "idMapBrush::ParseQ3: unable to read brush side plane definition");
			return NULL;
		}
		planepts[0] = rgd3_vec3_sub(planepts[0], origin);
		planepts[1] = rgd3_vec3_sub(planepts[1], origin);
		planepts[2] = rgd3_vec3_sub(planepts[2], origin);
		rgd3_plane_from_points(&side->plane, planepts[0], planepts[1], planepts[2], 1);

		if (!rgd3_lexer_read_token_on_line(src, &token))
		{
			rgd3_lexer_error(src, "idMapBrush::ParseQ3: unable to read brush side material");
			return NULL;
		}
		side->material = rgd3_map_material(token.data, 1.0f);

		/* shift, rotate and scale are read and ignored */
		rgd3_lexer_parse_int(src);
		rgd3_lexer_parse_int(src);
		rgd3_lexer_parse_int(src);
		rgd3_lexer_parse_float(src, NULL);
		rgd3_lexer_parse_float(src, NULL);
		side->tex_mat[0] = rgd3_vec3(0.03125f, 0.0f, 0.0f);
		side->tex_mat[1] = rgd3_vec3(0.0f, 0.03125f, 0.0f);
		side->origin = origin;

		if (rgd3_lexer_read_token_on_line(src, &token) && rgd3_lexer_read_token_on_line(src, &token))
			rgd3_lexer_read_token_on_line(src, &token);
	}
	rgd3_token_free(&token);
	return brush;
}

static void rgd3_map_entity_free(Rgd3MapEntity* ent);

/* frees the entity and primitive that were being parsed */
static void rgd3_mapfile_discard_pending(Rgd3MapFile* map)
{
	if (map->pending_prim)
		rgd3_map_primitive_free(map->pending_prim);
	map->pending_prim = NULL;
	if (map->pending_entity)
		rgd3_map_entity_free(map->pending_entity);
	map->pending_entity = NULL;
}

/* ---- entities ---- */

static void rgd3_map_entity_add(Rgd3MapEntity* ent, Rgd3MapPrimitive* prim)
{
	if (ent->num_primitives == ent->alloced)
	{
		ent->alloced = ent->alloced ? ent->alloced * 2 : 16;
		ent->primitives = (Rgd3MapPrimitive**)rgd3_heap_realloc(ent->primitives, (size_t)ent->alloced * sizeof(Rgd3MapPrimitive*), RGD3_TAG_GAME);
	}
	ent->primitives[ent->num_primitives++] = prim;
}

static void rgd3_map_entity_remove_primitive_data(Rgd3MapEntity* ent)
{
	int i;
	for (i = 0; i < ent->num_primitives; i++)
		rgd3_map_primitive_free(ent->primitives[i]);
	if (ent->primitives)
		rgd3_heap_free(ent->primitives);
	ent->primitives = NULL;
	ent->num_primitives = 0;
	ent->alloced = 0;
}

static void rgd3_map_entity_free(Rgd3MapEntity* ent)
{
	rgd3_map_entity_remove_primitive_data(ent);
	rgd3_dict_clear(&ent->epairs);
	rgd3_heap_free(ent);
}

static Rgd3MapEntity* rgd3_map_entity_parse(Rgd3MapFile* map, Rgd3Lexer* src, int world_spawn, float version)
{
	Rgd3Token token = { 0 };
	Rgd3MapEntity* ent;
	Rgd3Vec3 origin = { 0 };
	int worldent = 0;
	(void)world_spawn;

	if (!rgd3_lexer_read_token(src, &token))
		return NULL;
	if (!rgd3_token_is(&token, "{"))
	{
		rgd3_lexer_error(src, "idMapEntity::Parse: { not found, found %s", token.data);
		rgd3_token_free(&token);
		return NULL;
	}
	ent = (Rgd3MapEntity*)rgd3_heap_calloc(sizeof(Rgd3MapEntity), RGD3_TAG_GAME);
	map->pending_entity = ent;
	rgd3_hashindex_clear_resize(&ent->epairs.hash, 64, 16);
	rgd3_hashindex_set_granularity(&ent->epairs.hash, 16);

	for (;;)
	{
		if (!rgd3_lexer_read_token(src, &token))
		{
			rgd3_mapfile_discard_pending(map);
			rgd3_lexer_error(src, "idMapEntity::Parse: EOF without closing brace");
			return NULL;
		}
		if (rgd3_token_is(&token, "}"))
			break;

		if (rgd3_token_is(&token, "{"))
		{
			Rgd3MapPrimitive* prim;
			/* a brush or patch */
			if (!rgd3_lexer_read_token(src, &token))
			{
				rgd3_mapfile_discard_pending(map);
				rgd3_lexer_error(src, "idMapEntity::Parse: unexpected EOF");
				return NULL;
			}
			if (worldent)
				origin = rgd3_vec3(0.0f, 0.0f, 0.0f);
			if (rgd3_str_icmpn(token.data, "brush", 5) == 0)
			{
				int new_format = !rgd3_token_icmp(&token, "brushDef2") || !rgd3_token_icmp(&token, "brushDef3");
				prim = (Rgd3MapPrimitive*)rgd3_map_brush_parse(map, src, origin, new_format, version);
			}
			else if (rgd3_str_icmpn(token.data, "patch", 5) == 0)
			{
				prim = (Rgd3MapPrimitive*)rgd3_map_patch_parse(map, src, origin, !rgd3_token_icmp(&token, "patchDef3"), version);
			}
			else
			{
				/* a Q3 or older style brush */
				rgd3_lexer_unread_token(src, &token);
				prim = (Rgd3MapPrimitive*)rgd3_map_brush_parse_q3(map, src, origin);
			}
			if (!prim)
			{
				rgd3_mapfile_discard_pending(map);
				rgd3_token_free(&token);
				return NULL;
			}
			rgd3_map_entity_add(ent, prim);
			map->pending_prim = NULL;
		}
		else
		{
			char key[RGD3_MAX_STRING_CHARS];
			char value[RGD3_MAX_STRING_CHARS];
			rgd3_str_copynz(key, token.data, sizeof(key));
			rgd3_lexer_read_token_on_line(src, &token);
			rgd3_str_copynz(value, token.data, sizeof(value));
			/* the editor sometimes adds trailing spaces */
			rgd3_str_strip_trailing_whitespace(value);
			rgd3_str_strip_trailing_whitespace(key);
			rgd3_dict_set(&ent->epairs, key, value);
			if (!rgd3_str_icmp(key, "origin"))
			{
				double v1 = 0, v2 = 0, v3 = 0;
				sscanf(value, "%lf %lf %lf", &v1, &v2, &v3);
				origin = rgd3_vec3((float)v1, (float)v2, (float)v3);
			}
			else if (!rgd3_str_icmp(key, "classname") && !rgd3_str_icmp(value, "worldspawn"))
			{
				worldent = 1;
			}
		}
	}
	map->pending_entity = NULL;
	rgd3_token_free(&token);
	return ent;
}

static u32 rgd3_map_brush_geometry_crc(const Rgd3MapBrush* brush)
{
	u32 crc = 0;
	int i, j;
	for (i = 0; i < brush->num_sides; i++)
	{
		for (j = 0; j < 4; j++)
			crc ^= rgd3_map_float_crc(brush->sides[i].plane.v[j]);
		crc ^= rgd3_map_string_crc(brush->sides[i].material);
	}
	return crc;
}

static u32 rgd3_map_patch_geometry_crc(const Rgd3MapPatch* patch)
{
	u32 crc = (u32)(patch->horz_subdivisions ^ patch->vert_subdivisions);
	int i, j;
	for (i = 0; i < patch->width; i++)
	{
		for (j = 0; j < patch->height; j++)
		{
			const Rgd3DrawVert* v = &patch->verts[j * patch->width + i];
			crc ^= rgd3_map_float_crc(v->xyz.x);
			crc ^= rgd3_map_float_crc(v->xyz.y);
			crc ^= rgd3_map_float_crc(v->xyz.z);
		}
	}
	crc ^= rgd3_map_string_crc(patch->material);
	return crc;
}

u32 rgd3_map_entity_geometry_crc(const Rgd3MapEntity* ent)
{
	u32 crc = 0;
	int i;
	for (i = 0; i < ent->num_primitives; i++)
	{
		if (ent->primitives[i]->type == RGD3_MAP_PRIM_BRUSH)
			crc ^= rgd3_map_brush_geometry_crc((const Rgd3MapBrush*)ent->primitives[i]);
		else if (ent->primitives[i]->type == RGD3_MAP_PRIM_PATCH)
			crc ^= rgd3_map_patch_geometry_crc((const Rgd3MapPatch*)ent->primitives[i]);
	}
	return crc;
}

/* ---- map file ---- */

static void rgd3_mapfile_add_entity(Rgd3MapFile* map, Rgd3MapEntity* ent)
{
	if (map->num_entities == map->alloced)
	{
		map->alloced = map->alloced ? map->alloced * 2 : 1024;
		map->entities = (Rgd3MapEntity**)rgd3_heap_realloc(map->entities, (size_t)map->alloced * sizeof(Rgd3MapEntity*), RGD3_TAG_GAME);
	}
	map->entities[map->num_entities++] = ent;
}

void rgd3_mapfile_free(Rgd3MapFile* map)
{
	int i;
	rgd3_mapfile_discard_pending(map);
	rgd3_error_unwind_unregister(map);
	for (i = 0; i < map->num_entities; i++)
		rgd3_map_entity_free(map->entities[i]);
	if (map->entities)
		rgd3_heap_free(map->entities);
	map->entities = NULL;
	map->num_entities = 0;
	map->alloced = 0;
	map->has_primitive_data = 0;
}

static void rgd3_mapfile_unwind(void* object)
{
	rgd3_mapfile_free((Rgd3MapFile*)object);
}

int rgd3_mapfile_parse(Rgd3MapFile* map, const char* filename, int ignore_region, int os_path)
{
	Rgd3Lexer src;
	Rgd3Token token = { 0 };
	char full_name[RGD3_MAX_OSPATH];
	char* dot;
	int i, j, k;

	rgd3_mapfile_free(map);
	/* a map on a stack frame that a parse error unwinds is freed */
	rgd3_error_unwind_register(map, rgd3_mapfile_unwind);
	map->version = (float)RGD3_CURRENT_MAP_VERSION;
	rgd3_str_copynz(map->name, filename, sizeof(map->name));
	dot = strrchr(map->name, '.');
	if (dot && !strchr(dot, '/'))
		*dot = 0;

	/* no string concatenation for epairs, path names for materials */
	rgd3_lexer_init(&src, RGD3_MAP_LEXER_FLAGS);
	if (!ignore_region)
	{
		/* a .reg file first */
		rg_snprintf(full_name, sizeof(full_name), "%s.reg", map->name);
		rgd3_lexer_load_file(&src, full_name, os_path);
	}
	if (!rgd3_lexer_is_loaded(&src))
	{
		rg_snprintf(full_name, sizeof(full_name), "%s.map", map->name);
		rgd3_lexer_load_file(&src, full_name, os_path);
		if (!rgd3_lexer_is_loaded(&src))
		{
			rgd3_lexer_free(&src);
			rgd3_error_unwind_unregister(map);
			return 0;
		}
	}

	map->version = (float)RGD3_OLD_MAP_VERSION;
	map->file_time = rgd3_lexer_file_time(&src);
	if (rgd3_lexer_check_token_string(&src, "Version"))
	{
		rgd3_lexer_read_token_on_line(&src, &token);
		map->version = rgd3_token_float_value(&token);
	}
	for (;;)
	{
		Rgd3MapEntity* ent = rgd3_map_entity_parse(map, &src, map->num_entities == 0, map->version);
		if (!ent)
			break;
		rgd3_mapfile_add_entity(map, ent);
	}
	rgd3_token_free(&token);
	rgd3_lexer_free(&src);

	map->geometry_crc = 0;
	for (i = 0; i < map->num_entities; i++)
		map->geometry_crc ^= rgd3_map_entity_geometry_crc(map->entities[i]);

	if (map->num_entities)
	{
		Rgd3Dict* world = &map->entities[0]->epairs;
		const Rgd3KeyValue* kv;
		const char* material;

		/* "removeEntities" "classname" in the worldspawn removes every such entity */
		kv = rgd3_dict_match_prefix(world, "removeEntities", NULL);
		while (kv)
		{
			rgd3_mapfile_remove_entities(map, rgd3_kv_value(kv));
			kv = rgd3_dict_match_prefix(world, "removeEntities", kv);
		}

		/* "overrideMaterial" resets every material */
		if (rgd3_dict_get_string_out(world, "overrideMaterial", "", &material))
		{
			for (i = 0; i < map->num_entities; i++)
			{
				Rgd3MapEntity* ent = map->entities[i];
				for (j = 0; j < ent->num_primitives; j++)
				{
					Rgd3MapPrimitive* prim = ent->primitives[j];
					if (prim->type == RGD3_MAP_PRIM_BRUSH)
					{
						Rgd3MapBrush* brush = (Rgd3MapBrush*)prim;
						for (k = 0; k < brush->num_sides; k++)
						{
							rgd3_heap_free(brush->sides[k].material);
							brush->sides[k].material = rgd3_heap_strdup(material, RGD3_TAG_GAME);
						}
					}
					else if (prim->type == RGD3_MAP_PRIM_PATCH)
					{
						Rgd3MapPatch* patch = (Rgd3MapPatch*)prim;
						rgd3_heap_free(patch->material);
						patch->material = rgd3_heap_strdup(material, RGD3_TAG_GAME);
					}
				}
			}
		}

		/* force every entity to have a name */
		if (rgd3_dict_get_bool(world, "forceEntityNames", 0))
		{
			for (i = 1; i < map->num_entities; i++)
			{
				Rgd3MapEntity* ent = map->entities[i];
				if (!rgd3_dict_find_key(&ent->epairs, "name"))
				{
					char name[RGD3_MAX_STRING_CHARS];
					rg_snprintf(name, sizeof(name), "%s%d", rgd3_dict_get_string(&ent->epairs, "classname", "forcedName"), i);
					rgd3_dict_set(&ent->epairs, "name", name);
				}
			}
		}

		/* move the primitives of func_group entities into the worldspawn */
		if (rgd3_dict_get_bool(world, "moveFuncGroups", 0))
		{
			for (i = 1; i < map->num_entities; i++)
			{
				Rgd3MapEntity* ent = map->entities[i];
				if (!rgd3_str_icmp(rgd3_dict_get_string(&ent->epairs, "classname", ""), "func_group"))
				{
					for (j = 0; j < ent->num_primitives; j++)
						rgd3_map_entity_add(map->entities[0], ent->primitives[j]);
					ent->num_primitives = 0;
				}
			}
		}
	}
	map->has_primitive_data = 1;
	rgd3_error_unwind_unregister(map);
	return 1;
}

Rgd3MapEntity* rgd3_mapfile_find_entity(Rgd3MapFile* map, const char* name)
{
	int i;
	for (i = 0; i < map->num_entities; i++)
	{
		if (!rgd3_str_icmp(rgd3_dict_get_string(&map->entities[i]->epairs, "name", ""), name))
			return map->entities[i];
	}
	return NULL;
}

void rgd3_mapfile_remove_entities(Rgd3MapFile* map, const char* classname)
{
	int i;
	for (i = 0; i < map->num_entities; i++)
	{
		if (!rgd3_str_icmp(rgd3_dict_get_string(&map->entities[i]->epairs, "classname", ""), classname))
		{
			rgd3_map_entity_free(map->entities[i]);
			memmove(&map->entities[i], &map->entities[i + 1], (size_t)(map->num_entities - i - 1) * sizeof(Rgd3MapEntity*));
			map->num_entities--;
			i--;
		}
	}
}

void rgd3_mapfile_remove_primitive_data(Rgd3MapFile* map)
{
	int i;
	for (i = 0; i < map->num_entities; i++)
		rgd3_map_entity_remove_primitive_data(map->entities[i]);
	map->has_primitive_data = 0;
}

/* ---- mapinfo ---- */

typedef struct Rgd3MapClassCount
{
	const char* classname;
	int count;
} Rgd3MapClassCount;

static int rgd3_map_sort_class_count(const void* a, const void* b)
{
	const Rgd3MapClassCount* x = (const Rgd3MapClassCount*)a;
	const Rgd3MapClassCount* y = (const Rgd3MapClassCount*)b;
	if (x->count != y->count)
		return y->count - x->count;
	return rgd3_str_icmp(x->classname, y->classname);
}

/* mapinfo <map> [all|classname filter]: mounts the map's container, parses
   maps/<map>.map and summarises it (rgd3 addition) */
static void rgd3_mapfile_cmd_mapinfo(const Rgd3CmdArgs* args)
{
	Rgd3MapFile map = { 0 };
	char path[RGD3_MAX_OSPATH];
	char match[RGD3_MAX_COMMAND_STRING];
	const char* name = rgd3_cmdargs_argv(args, 1);
	const char* filter = rgd3_cmdargs_match(args, 2, match, sizeof(match));
	Rgd3MapClassCount* classes;
	int num_classes = 0, brushes = 0, patches = 0, sides = 0, unknown_classes = 0, i, j;
	int mounted;
	u64 start;

	if (rgd3_cmdargs_argc(args) < 2)
	{
		rgd3_printf("usage: mapinfo <map, e.g. game/mars_city1> [all | classname filter]\n");
		return;
	}
	mounted = rgd3_fs_load_map_resources(name);
	rgd3_printf("map container: %s\n", mounted ? "mounted" : "none");
	rg_snprintf(path, sizeof(path), "maps/%s", name);
	start = rgd3_time_us();
	if (!rgd3_mapfile_parse(&map, path, 1, 0))
	{
		rgd3_printf("couldn't load %s.map\n", path);
		rgd3_fs_unload_map_resources();
		return;
	}
	rgd3_printf("%s.map: version %g, %d entities, geometry crc 0x%08x, parsed in %.1f ms\n", map.name, map.version,
		map.num_entities, map.geometry_crc, (double)(rgd3_time_us() - start) / 1000.0);

	classes = (Rgd3MapClassCount*)rgd3_heap_calloc((size_t)(map.num_entities + 1) * sizeof(Rgd3MapClassCount), RGD3_TAG_TEMP);
	for (i = 0; i < map.num_entities; i++)
	{
		const Rgd3MapEntity* ent = map.entities[i];
		const char* classname = rgd3_dict_get_string(&ent->epairs, "classname", "");
		for (j = 0; j < ent->num_primitives; j++)
		{
			if (ent->primitives[j]->type == RGD3_MAP_PRIM_BRUSH)
			{
				brushes++;
				sides += ((const Rgd3MapBrush*)ent->primitives[j])->num_sides;
			}
			else
			{
				patches++;
			}
		}
		for (j = 0; j < num_classes; j++)
		{
			if (!rgd3_str_icmp(classes[j].classname, classname))
				break;
		}
		if (j == num_classes)
		{
			classes[num_classes].classname = classname;
			num_classes++;
		}
		classes[j].count++;
		if (filter && (!rgd3_str_icmp(filter, "all") || rgd3_str_filter(filter, classname, 0)))
		{
			Rgd3Vec3 origin;
			rgd3_dict_get_vector(&ent->epairs, "origin", NULL, &origin);
			rgd3_printf("%5d %-32s %-40s (%g %g %g)\n", i, classname, rgd3_dict_get_string(&ent->epairs, "name", ""),
				origin.x, origin.y, origin.z);
		}
	}
	rgd3_printf("%d brushes (%d sides), %d patches\n", brushes, sides, patches);
	qsort(classes, (size_t)num_classes, sizeof(Rgd3MapClassCount), rgd3_map_sort_class_count);
	for (i = 0; i < num_classes; i++)
	{
		/* spawn classes must resolve to an entityDef (or be worldspawn) */
		int known = !rgd3_str_icmp(classes[i].classname, "worldspawn") ||
			rgd3_decl_find(RGD3_DECL_ENTITYDEF, classes[i].classname, 0) != NULL;
		if (!known)
			unknown_classes++;
		if (i < 12 || !known)
			rgd3_printf("  %4d %s%s\n", classes[i].count, classes[i].classname, known ? "" : "  ^1(no entityDef)");
	}
	rgd3_printf("%d classes, %d without an entityDef\n", num_classes, unknown_classes);
	rgd3_heap_free(classes);
	rgd3_mapfile_free(&map);
	rgd3_fs_unload_map_resources();
}

void rgd3_mapfile_register(void)
{
	rgd3_cmd_add("mapinfo", rgd3_mapfile_cmd_mapinfo, RGD3_CMD_FL_SYSTEM, "parses a map and summarises its entities: mapinfo <map> [all|filter]", NULL);
}
