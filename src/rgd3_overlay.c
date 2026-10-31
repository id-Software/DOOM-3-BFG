/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_overlay.c - adapted from rg_doom/src/rgd_overlay.c: the demo font's
   bit rows are expanded into an RGBA atlas at startup, '-' and '.' glyphs are
   added, and right-aligned lines are drawn with a one-pixel shadow. */

typedef struct Rgd3Overlay
{
	RgTextFont font;
	RgTextGlyph glyphs[DEMO_GLYPH_COUNT + 2];
	RgTextGpuRenderer text;
	RgGpuUploadRing ring;
	int initialized;
} Rgd3Overlay;

static Rgd3Overlay rgd3_overlay;

void rgd3_overlay_destroy(void)
{
	if (!rgd3_overlay.initialized)
		return;
	rg_gpu_upload_ring_destroy(&rgd3_overlay.ring);
	rg_text_gpu_destroy(&rgd3_overlay.text);
	memset(&rgd3_overlay, 0, sizeof(rgd3_overlay));
}

int rgd3_overlay_create(SDL_GPUDevice* device, SDL_GPUTextureFormat target_format, const char* shader_root)
{
	static unsigned char atlas[DEMO_ATLAS_WIDTH * DEMO_ATLAS_HEIGHT * 4];
	static char metrics[sizeof(demo_font_metrics) + 80];
	static const unsigned char punctuation[2][7] = {
		{ 0, 0, 0, 31, 0, 0, 0 },	/* '-' */
		{ 0, 0, 0, 0, 0, 0, 4 }		/* '.' */
	};
	RgTextFontLoadDesc load = {0};
	RgTextGpuDesc desc = {0};
	u32 i;
	memset(&rgd3_overlay, 0, sizeof(rgd3_overlay));
	memset(atlas, 0, sizeof(atlas));
	rg_snprintf(metrics, sizeof(metrics), "%sglyph 45 57 21 5 7 0 0 6\nglyph 46 65 21 5 7 0 0 6\n", demo_font_metrics);
	load.data = metrics;
	load.data_size = strlen(metrics);
	load.glyphs = rgd3_overlay.glyphs;
	load.glyph_capacity = DEMO_GLYPH_COUNT + 2;
	if (!rg_text_font_load_rgfont(&rgd3_overlay.font, &load))
		return 0;
	for (i = 0; i < rgd3_overlay.font.glyph_count; ++i)
	{
		const RgTextGlyph* g = &rgd3_overlay.glyphs[i];
		const unsigned char* rows = (g->codepoint == 45 || g->codepoint == 46) ? punctuation[g->codepoint - 45] : demo_font_rows[g->codepoint];
		int row, col;
		for (row = 0; row < g->h; ++row)
			for (col = 0; col < g->w; ++col)
				if (rows[row] & (1u << (4 - col)))
					memset(atlas + ((g->y + row) * DEMO_ATLAS_WIDTH + g->x + col) * 4, 255, 4);
	}
	desc.device = device;
	desc.target_format = target_format;
	desc.shader_root = shader_root;
	desc.atlas_pixels_rgba8 = atlas;
	desc.atlas_width = DEMO_ATLAS_WIDTH;
	desc.atlas_height = DEMO_ATLAS_HEIGHT;
	desc.max_quads = 1024;
	desc.min_filter = SDL_GPU_FILTER_NEAREST;
	desc.mag_filter = SDL_GPU_FILTER_NEAREST;
	if (!rg_text_gpu_create(&rgd3_overlay.text, &desc))
	{
		rgd3_warning("overlay: rg_text_gpu_create failed (%s); are shaders built? run build_shaders.bat", SDL_GetError());
		return 0;
	}
	if (!rg_gpu_upload_ring_init(&rgd3_overlay.ring, device, 128u * 1024u))
	{
		rg_text_gpu_destroy(&rgd3_overlay.text);
		return 0;
	}
	rgd3_overlay.initialized = 1;
	return 1;
}

/* Keeps only characters the demo font has; lowercase is folded to upper. */
static void rgd3_overlay_sanitize(const char* in, char* out, size_t size)
{
	size_t n = 0;
	for (; *in && n + 1 < size; ++in)
	{
		char c = *in;
		if (c >= 'a' && c <= 'z')
			c = (char)(c - 'a' + 'A');
		if (rg_text_find_glyph(&rgd3_overlay.font, (u32)(unsigned char)c) == NULL)
			c = ' ';
		out[n++] = c;
	}
	out[n] = 0;
}

static void rgd3_overlay_line(const char* text, float y, float scale, float width, float margin)
{
	RgTextBuildDesc desc = {0};
	desc.font = &rgd3_overlay.font;
	desc.text = text;
	desc.text_size = strlen(text);
	desc.scale = scale;
	desc.align = RG_TEXT_ALIGN_RIGHT;
	desc.align_width = width - 2.0f * margin;
	desc.x = margin + scale;
	desc.y = y + scale;
	desc.color = (RgTextColor){ 0, 0, 0, 1 };
	rg_text_gpu_queue_ex(&rgd3_overlay.text, &desc);
	desc.x = margin;
	desc.y = y;
	desc.color = (RgTextColor){ 1, 1, 1, 1 };
	rg_text_gpu_queue_ex(&rgd3_overlay.text, &desc);
}

int rgd3_overlay_draw(SDL_GPUCommandBuffer* command, SDL_GPUTexture* target, u32 width, u32 height,
	const char* const* lines, int num_lines)
{
	RgTextGpuUpload upload = {0};
	RgTextGpuUniforms uniforms = {0};
	SDL_GPUColorTargetInfo color = {0};
	SDL_GPUCopyPass* copy;
	SDL_GPURenderPass* pass;
	float scale, margin;
	int i;
	if (!rgd3_overlay.initialized || !command || !target || !width || !height || num_lines <= 0)
		return 0;
	scale = floorf(fminf((float)width / 640.0f, (float)height / 360.0f));
	scale = fmaxf(1.0f, fminf(scale, 4.0f));
	margin = 6.0f * scale;
	rg_text_gpu_begin(&rgd3_overlay.text);
	for (i = 0; i < num_lines && i < RGD3_OVERLAY_MAX_LINES; ++i)
	{
		char clean[128];
		rgd3_overlay_sanitize(lines[i], clean, sizeof(clean));
		rgd3_overlay_line(clean, margin + (float)i * 10.0f * scale, scale, (float)width, margin);
	}
	rg_gpu_upload_ring_begin(&rgd3_overlay.ring, 1);
	if (!rg_text_gpu_stage_upload(&rgd3_overlay.text, &rgd3_overlay.ring, &upload))
	{
		rg_gpu_upload_ring_end(&rgd3_overlay.ring);
		return 0;
	}
	rg_gpu_upload_ring_end(&rgd3_overlay.ring);
	copy = SDL_BeginGPUCopyPass(command);
	if (!copy)
		return 0;
	rg_text_gpu_encode_upload(&rgd3_overlay.text, copy, &rgd3_overlay.ring, &upload);
	SDL_EndGPUCopyPass(copy);
	uniforms.projection[0] = 2.0f / (float)width;
	uniforms.projection[5] = -2.0f / (float)height;
	uniforms.projection[10] = 1.0f;
	uniforms.projection[15] = 1.0f;
	uniforms.projection[12] = -1.0f;
	uniforms.projection[13] = 1.0f;
	uniforms.transform[0] = 1.0f;
	uniforms.transform[5] = 1.0f;
	uniforms.transform[10] = 1.0f;
	uniforms.transform[15] = 1.0f;
	color.texture = target;
	color.load_op = SDL_GPU_LOADOP_LOAD;
	color.store_op = SDL_GPU_STOREOP_STORE;
	pass = SDL_BeginGPURenderPass(command, &color, 1, NULL);
	if (!pass)
		return 0;
	rg_text_gpu_flush(&rgd3_overlay.text, command, pass, &uniforms);
	SDL_EndGPURenderPass(pass);
	return 1;
}
