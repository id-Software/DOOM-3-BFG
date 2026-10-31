/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_render_stub.c - renderer entry points until the renderer exists
   (M3/M4). Replaced in all.c by the real modules. */

void rgd3_render_begin_level_load(void)
{
	rgd3_hooks_trace("render_begin_level_load");
}

void rgd3_render_end_level_load(void)
{
	rgd3_hooks_trace("render_end_level_load");
}

void rgd3_render_preload(const Rgd3PreloadManifest* manifest, const char* map_name)
{
	(void)manifest;
	(void)map_name;
	rgd3_hooks_trace("render_preload");
}

/* idRenderWorldLocal::InitFromMap reads generated/<map>.bproc; the stub
   only checks that it is there */
int rgd3_render_world_init_from_map(const char* map_name)
{
	char base[RGD3_MAX_OSPATH];
	char proc[RGD3_MAX_OSPATH];
	char* dot;

	rgd3_hooks_trace("render_world_init_from_map");
	rgd3_str_copynz(base, map_name, sizeof(base));
	dot = strrchr(base, '.');
	if (dot && !strchr(dot, '/'))
		*dot = 0;
	rg_snprintf(proc, sizeof(proc), "generated/%s.bproc", base);
	if (!rgd3_fs_file_exists(proc))
	{
		rgd3_warning("rgd3_render_world_init_from_map: %s not found", proc);
		return 0;
	}
	return 1;
}

void rgd3_render_world_generate_all_interactions(void)
{
	rgd3_hooks_trace("render_generate_all_interactions");
}
