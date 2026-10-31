/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_manifest.c - see rgd3_manifest.h (neo/framework/File_Manifest.cpp
   LoadManifest / LoadManifestFromFile / FindFile, preloadEntry_s::Read). */

static char* rgd3_manifest_read_string(Rgd3BinReader* r)
{
	Rgd3StrBuf s = { 0 };
	char* out;
	rgd3_bin_string(r, &s);
	out = rgd3_heap_strdup(rgd3_strbuf_cstr(&s), RGD3_TAG_RESOURCE);
	rgd3_strbuf_free(&s);
	return out;
}

int rgd3_preload_manifest_load(Rgd3PreloadManifest* m, const char* file_name)
{
	Rgd3BinReader r;
	void* data;
	int length, i, count;

	memset(m, 0, sizeof(*m));
	length = rgd3_fs_read_file(file_name, &data, NULL);
	if (length < 0)
		return 0;
	rgd3_bin_reader_init(&r, data, (size_t)length);
	count = rgd3_bin_i32_be(&r);
	m->filename = rgd3_manifest_read_string(&r);
	/* each entry is at least 20 bytes; anything else is a corrupt file */
	if (r.error || count < 0 || (size_t)count > rgd3_bin_remaining(&r) / 20)
	{
		rgd3_warning("%s: bad preload manifest entry count %d", file_name, count);
		rgd3_fs_free_file(data);
		rgd3_preload_manifest_free(m);
		return 0;
	}
	m->entries = (Rgd3PreloadEntry*)rgd3_heap_calloc((size_t)(count ? count : 1) * sizeof(Rgd3PreloadEntry), RGD3_TAG_RESOURCE);
	for (i = 0; i < count && !r.error; i++)
	{
		Rgd3PreloadEntry* e = &m->entries[i];
		e->res_type = rgd3_bin_i32_be(&r);
		e->resource_name = rgd3_manifest_read_string(&r);
		e->filter = rgd3_bin_i32_be(&r);
		e->repeat = rgd3_bin_i32_be(&r);
		e->usage = rgd3_bin_i32_be(&r);
		e->cube_map = rgd3_bin_i32_be(&r);
		m->num_entries++;
	}
	rgd3_fs_free_file(data);
	if (r.error)
	{
		rgd3_warning("%s: truncated preload manifest (%d of %d entries)", file_name, m->num_entries, count);
		rgd3_preload_manifest_free(m);
		return 0;
	}
	return 1;
}

void rgd3_preload_manifest_free(Rgd3PreloadManifest* m)
{
	int i;
	for (i = 0; i < m->num_entries; i++)
		rgd3_heap_free(m->entries[i].resource_name);
	if (m->entries)
		rgd3_heap_free(m->entries);
	if (m->filename)
		rgd3_heap_free(m->filename);
	memset(m, 0, sizeof(*m));
}

const char* rgd3_preload_type_name(int res_type)
{
	static const char* names[] = { "image", "model", "sample", "anim", "collision", "particle" };
	return (res_type >= 0 && res_type < (int)RG_ARRAY_COUNT(names)) ? names[res_type] : "unknown";
}

int rgd3_file_manifest_load(Rgd3FileManifest* m, const char* file_name)
{
	Rgd3BinReader r;
	void* data;
	int length, i, count;

	memset(m, 0, sizeof(*m));
	length = rgd3_fs_read_file(file_name, &data, NULL);
	if (length < 0)
		return 0;
	m->filename = rgd3_heap_strdup(file_name, RGD3_TAG_RESOURCE);
	rgd3_bin_reader_init(&r, data, (size_t)length);
	count = rgd3_bin_i32_be(&r);
	/* each string is at least its 4-byte length */
	if (r.error || count < 0 || (size_t)count > rgd3_bin_remaining(&r) / 4)
	{
		rgd3_warning("%s: bad file manifest entry count %d", file_name, count);
		rgd3_fs_free_file(data);
		rgd3_file_manifest_free(m);
		return 0;
	}
	m->table = (char**)rgd3_heap_calloc((size_t)(count ? count : 1) * sizeof(char*), RGD3_TAG_RESOURCE);
	rgd3_hashindex_set_granularity(&m->hash, 4096);
	for (i = 0; i < count && !r.error; i++)
	{
		m->table[i] = rgd3_manifest_read_string(&r);
		rgd3_hashindex_add(&m->hash, rgd3_hashindex_key_string(&m->hash, m->table[i], 0), i);
		m->num++;
	}
	rgd3_fs_free_file(data);
	if (r.error)
	{
		rgd3_warning("%s: truncated file manifest", file_name);
		rgd3_file_manifest_free(m);
		return 0;
	}
	return 1;
}

void rgd3_file_manifest_free(Rgd3FileManifest* m)
{
	int i;
	for (i = 0; i < m->num; i++)
		rgd3_heap_free(m->table[i]);
	if (m->table)
		rgd3_heap_free(m->table);
	if (m->filename)
		rgd3_heap_free(m->filename);
	rgd3_hashindex_free(&m->hash);
	memset(m, 0, sizeof(*m));
}

int rgd3_file_manifest_find(const Rgd3FileManifest* m, const char* file_name)
{
	int key = rgd3_hashindex_key_string(&m->hash, file_name, 0);
	int i;
	for (i = rgd3_hashindex_first(&m->hash, key); i != -1; i = rgd3_hashindex_next(&m->hash, i))
	{
		if (!rgd3_str_icmp(m->table[i], file_name))
			return i;
	}
	return -1;
}

/* Common_load.cpp: "game/mars_city1" -> "maps/mars_city1.preload" */
void rgd3_preload_manifest_name(const char* map_name, char* out, size_t size)
{
	const char* base = map_name;
	if (rgd3_str_icmpn(base, "game/", 5) == 0)
		rg_snprintf(out, size, "maps/%s.preload", base + 5);
	else if (rgd3_str_icmpn(base, "maps/", 5) == 0)
		rg_snprintf(out, size, "%s.preload", base);
	else
		rg_snprintf(out, size, "maps/%s.preload", base);
}

/* printPreload <map> [type]: dumps maps/<map>.preload (rgd3 addition) */
static void rgd3_manifest_cmd_print_preload(const Rgd3CmdArgs* args)
{
	Rgd3PreloadManifest m;
	char path[RGD3_MAX_OSPATH];
	const char* map = rgd3_cmdargs_argv(args, 1);
	const char* only = rgd3_cmdargs_argc(args) > 2 ? rgd3_cmdargs_argv(args, 2) : NULL;
	int counts[7] = { 0 };
	int i;

	if (rgd3_cmdargs_argc(args) < 2)
	{
		rgd3_printf("usage: printPreload <map> [image|model|sample|anim|collision|particle]\n");
		return;
	}
	rgd3_preload_manifest_name(map, path, sizeof(path));
	if (!rgd3_preload_manifest_load(&m, path))
	{
		rgd3_printf("couldn't load %s\n", path);
		return;
	}
	rgd3_printf("dump for manifest %s (%s)\n", path, m.filename);
	for (i = 0; i < m.num_entries; i++)
	{
		const Rgd3PreloadEntry* e = &m.entries[i];
		counts[(e->res_type >= 0 && e->res_type < 6) ? e->res_type : 6]++;
		if (only && rgd3_str_icmp(only, rgd3_preload_type_name(e->res_type)))
			continue;
		if (only)
			rgd3_printf("%-9s %s\n", rgd3_preload_type_name(e->res_type), e->resource_name);
	}
	rgd3_printf("%d entries: %d image, %d model, %d sample, %d anim, %d collision, %d particle, %d unknown\n",
		m.num_entries, counts[0], counts[1], counts[2], counts[3], counts[4], counts[5], counts[6]);
	rgd3_preload_manifest_free(&m);
}

void rgd3_manifest_register(void)
{
	rgd3_cmd_add("printPreload", rgd3_manifest_cmd_print_preload, RGD3_CMD_FL_SYSTEM, "dumps a map's .preload manifest", NULL);
}
