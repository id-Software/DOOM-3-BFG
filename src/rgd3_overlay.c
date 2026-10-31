/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_overlay.c - adapted from rg_doom/src/rgd_overlay.c: the demo font's
   bit rows are expanded into an RGBA atlas at startup and right-aligned
   lines are drawn with a one-pixel shadow. rgd3 adds the glyphs the demo
   font lacks (lower case and punctuation, drawn for this file), a solid
   atlas cell for filled rectangles, and a per-frame batch API used by the
   console. */

#define RGD3_OVERLAY_FIRST_CHAR 32
#define RGD3_OVERLAY_LAST_CHAR 126
#define RGD3_OVERLAY_NUM_CHARS (RGD3_OVERLAY_LAST_CHAR - RGD3_OVERLAY_FIRST_CHAR + 1)
#define RGD3_OVERLAY_ATLAS_W 128
#define RGD3_OVERLAY_ATLAS_H 64
#define RGD3_OVERLAY_CELL_W 8
#define RGD3_OVERLAY_CELL_H 10
#define RGD3_OVERLAY_MAX_QUADS 8192
#define RGD3_OVERLAY_QUAD_BATCH 256

/* Rows use bits 4..0 left to right, like demo_font_rows. Only codepoints the
   demo font does not have are listed. Glyphs with descenders (g j p q y , ;)
   use rows 7 and 8, below the 7-row cap height. */
#define RGD3_OVERLAY_MAX_ROWS 9
static const unsigned char rgd3_overlay_extra_rows[RGD3_OVERLAY_LAST_CHAR + 1][RGD3_OVERLAY_MAX_ROWS] = {
	['!'] = { 4, 4, 4, 4, 4, 0, 4 },
	['"'] = { 10, 10, 10, 0, 0, 0, 0 },
	['#'] = { 10, 10, 31, 10, 31, 10, 10 },
	['$'] = { 4, 15, 20, 14, 5, 30, 4 },
	['%'] = { 24, 25, 2, 4, 8, 19, 3 },
	['&'] = { 12, 18, 20, 8, 21, 18, 13 },
	['\''] = { 4, 4, 8, 0, 0, 0, 0 },
	['('] = { 2, 4, 8, 8, 8, 4, 2 },
	[')'] = { 8, 4, 2, 2, 2, 4, 8 },
	['*'] = { 0, 4, 21, 14, 21, 4, 0 },
	['+'] = { 0, 4, 4, 31, 4, 4, 0 },
	[','] = { 0, 0, 0, 0, 0, 12, 12, 4, 8 },
	['-'] = { 0, 0, 0, 31, 0, 0, 0 },
	['.'] = { 0, 0, 0, 0, 0, 12, 12 },
	['/'] = { 0, 1, 2, 4, 8, 16, 0 },
	[';'] = { 0, 0, 12, 12, 0, 12, 12, 4, 8 },
	['<'] = { 2, 4, 8, 16, 8, 4, 2 },
	['='] = { 0, 0, 31, 0, 31, 0, 0 },
	['>'] = { 8, 4, 2, 1, 2, 4, 8 },
	['@'] = { 14, 17, 1, 13, 21, 21, 14 },
	['['] = { 14, 8, 8, 8, 8, 8, 14 },
	['\\'] = { 0, 16, 8, 4, 2, 1, 0 },
	[']'] = { 14, 2, 2, 2, 2, 2, 14 },
	['^'] = { 4, 10, 17, 0, 0, 0, 0 },
	['_'] = { 0, 0, 0, 0, 0, 0, 31 },
	['`'] = { 8, 4, 2, 0, 0, 0, 0 },
	['a'] = { 0, 0, 14, 1, 15, 17, 15 },
	['b'] = { 16, 16, 22, 25, 17, 17, 30 },
	['c'] = { 0, 0, 14, 16, 16, 17, 14 },
	['d'] = { 1, 1, 13, 19, 17, 17, 15 },
	['e'] = { 0, 0, 14, 17, 31, 16, 14 },
	['f'] = { 6, 9, 8, 28, 8, 8, 8 },
	['g'] = { 0, 0, 15, 17, 17, 17, 15, 1, 14 },
	['h'] = { 16, 16, 22, 25, 17, 17, 17 },
	['i'] = { 4, 0, 12, 4, 4, 4, 14 },
	['j'] = { 2, 0, 6, 2, 2, 2, 2, 18, 12 },
	['k'] = { 16, 16, 18, 20, 24, 20, 18 },
	['l'] = { 12, 4, 4, 4, 4, 4, 14 },
	['m'] = { 0, 0, 26, 21, 21, 17, 17 },
	['n'] = { 0, 0, 22, 25, 17, 17, 17 },
	['o'] = { 0, 0, 14, 17, 17, 17, 14 },
	['p'] = { 0, 0, 30, 17, 17, 17, 30, 16, 16 },
	['q'] = { 0, 0, 15, 17, 17, 17, 15, 1, 1 },
	['r'] = { 0, 0, 22, 25, 16, 16, 16 },
	['s'] = { 0, 0, 14, 16, 14, 1, 30 },
	['t'] = { 8, 8, 28, 8, 8, 9, 6 },
	['u'] = { 0, 0, 17, 17, 17, 19, 13 },
	['v'] = { 0, 0, 17, 17, 17, 10, 4 },
	['w'] = { 0, 0, 17, 17, 21, 21, 10 },
	['x'] = { 0, 0, 17, 10, 4, 10, 17 },
	['y'] = { 0, 0, 17, 17, 17, 17, 15, 1, 14 },
	['z'] = { 0, 0, 31, 2, 4, 8, 31 },
	['{'] = { 2, 4, 4, 8, 4, 4, 2 },
	['|'] = { 4, 4, 4, 4, 4, 4, 4 },
	['}'] = { 8, 4, 4, 2, 4, 4, 8 },
	['~'] = { 0, 0, 8, 21, 2, 0, 0 },
};

typedef struct Rgd3Overlay
{
	RgTextFont font;
	RgTextGlyph glyphs[RGD3_OVERLAY_NUM_CHARS];
	RgTextGpuRenderer text;
	RgGpuUploadRing ring;
	int initialized;
	/* current batch */
	int batch_open;
	u32 width;
	u32 height;
	float scale;
	RgTextQuad quads[RGD3_OVERLAY_QUAD_BATCH];
	int num_quads;
} Rgd3Overlay;

static Rgd3Overlay rgd3_overlay;

static int rgd3_overlay_demo_has(int c)
{
	return c == ' ' || (c >= '0' && c <= ':') || c == '?' || (c >= 'A' && c <= 'Z');
}

static int rgd3_overlay_has_descender(int c)
{
	return c == 'g' || c == 'j' || c == 'p' || c == 'q' || c == 'y' || c == ',' || c == ';';
}

static const unsigned char* rgd3_overlay_rows(int c)
{
	return rgd3_overlay_demo_has(c) ? demo_font_rows[c] : rgd3_overlay_extra_rows[c];
}

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
	static unsigned char atlas[RGD3_OVERLAY_ATLAS_W * RGD3_OVERLAY_ATLAS_H * 4];
	static char metrics[RGD3_OVERLAY_NUM_CHARS * 40 + 128];
	RgTextFontLoadDesc load = {0};
	RgTextGpuDesc desc = {0};
	size_t len;
	int c, row, col;
	memset(&rgd3_overlay, 0, sizeof(rgd3_overlay));
	memset(atlas, 0, sizeof(atlas));

	/* one 8x10 cell per printable character, the glyph at (1,1) in its cell;
	   the cell after '~' is solid white for filled rectangles */
	len = (size_t)rg_snprintf(metrics, sizeof(metrics), "rgfont 1\natlas %d %d\nline_height %d\nascent 7\ndescent 0\nfallback 63\n",
		RGD3_OVERLAY_ATLAS_W, RGD3_OVERLAY_ATLAS_H, RGD3_OVERLAY_CHAR_HEIGHT);
	for (c = RGD3_OVERLAY_FIRST_CHAR; c <= RGD3_OVERLAY_LAST_CHAR; ++c)
	{
		int cell = c - RGD3_OVERLAY_FIRST_CHAR;
		int gx = (cell % 16) * RGD3_OVERLAY_CELL_W + 1;
		int gy = (cell / 16) * RGD3_OVERLAY_CELL_H + 1;
		const unsigned char* rows = rgd3_overlay_rows(c);
		int h = rgd3_overlay_has_descender(c) ? RGD3_OVERLAY_MAX_ROWS : 7;
		len += (size_t)rg_snprintf(metrics + len, sizeof(metrics) - len, "glyph %d %d %d %d %d 0 0 %d\n", c, gx, gy,
			c == ' ' ? 0 : 5, c == ' ' ? 0 : h, RGD3_OVERLAY_CHAR_WIDTH);
		for (row = 0; row < h; ++row)
			for (col = 0; col < 5; ++col)
				if (rows[row] & (1u << (4 - col)))
					memset(atlas + ((gy + row) * RGD3_OVERLAY_ATLAS_W + gx + col) * 4, 255, 4);
	}
	for (row = 0; row < RGD3_OVERLAY_CELL_H; ++row)
		memset(atlas + ((5 * RGD3_OVERLAY_CELL_H + row) * RGD3_OVERLAY_ATLAS_W + 15 * RGD3_OVERLAY_CELL_W) * 4, 255,
			RGD3_OVERLAY_CELL_W * 4);

	load.data = metrics;
	load.data_size = len;
	load.glyphs = rgd3_overlay.glyphs;
	load.glyph_capacity = RGD3_OVERLAY_NUM_CHARS;
	if (!rg_text_font_load_rgfont(&rgd3_overlay.font, &load))
		return 0;
	desc.device = device;
	desc.target_format = target_format;
	desc.shader_root = shader_root;
	desc.atlas_pixels_rgba8 = atlas;
	desc.atlas_width = RGD3_OVERLAY_ATLAS_W;
	desc.atlas_height = RGD3_OVERLAY_ATLAS_H;
	desc.max_quads = RGD3_OVERLAY_MAX_QUADS;
	desc.min_filter = SDL_GPU_FILTER_NEAREST;
	desc.mag_filter = SDL_GPU_FILTER_NEAREST;
	if (!rg_text_gpu_create(&rgd3_overlay.text, &desc))
	{
		rgd3_warning("overlay: rg_text_gpu_create failed (%s); are shaders built? run build_shaders.bat", SDL_GetError());
		return 0;
	}
	if (!rg_gpu_upload_ring_init(&rgd3_overlay.ring, device, 2u * 1024u * 1024u))
	{
		rg_text_gpu_destroy(&rgd3_overlay.text);
		return 0;
	}
	rgd3_overlay.initialized = 1;
	return 1;
}

/* ---------------------------------------------------------------------------
   Batch
--------------------------------------------------------------------------- */

static void rgd3_overlay_flush_quads(void)
{
	if (rgd3_overlay.num_quads)
		rg_text_gpu_queue_quads(&rgd3_overlay.text, rgd3_overlay.quads, (size_t)rgd3_overlay.num_quads);
	rgd3_overlay.num_quads = 0;
}

static RgTextQuad* rgd3_overlay_alloc_quad(void)
{
	if (rgd3_overlay.num_quads == RGD3_OVERLAY_QUAD_BATCH)
		rgd3_overlay_flush_quads();
	return &rgd3_overlay.quads[rgd3_overlay.num_quads++];
}

int rgd3_overlay_begin(u32 width, u32 height)
{
	float scale;
	if (!rgd3_overlay.initialized || !width || !height)
		return 0;
	scale = floorf(fminf((float)width / 640.0f, (float)height / 360.0f));
	rgd3_overlay.scale = fmaxf(1.0f, fminf(scale, 4.0f));
	rgd3_overlay.width = width;
	rgd3_overlay.height = height;
	rgd3_overlay.num_quads = 0;
	rgd3_overlay.batch_open = 1;
	rg_text_gpu_begin(&rgd3_overlay.text);
	return 1;
}

void rgd3_overlay_virtual_size(float* width, float* height)
{
	float s = rgd3_overlay.scale > 0.0f ? rgd3_overlay.scale : 1.0f;
	*width = (float)rgd3_overlay.width / s;
	*height = (float)rgd3_overlay.height / s;
}

void rgd3_overlay_fill(float x, float y, float w, float h, const float* rgba)
{
	RgTextQuad* q;
	/* centre of the solid cell, so nearest sampling never leaves it */
	const float u = (15.0f * RGD3_OVERLAY_CELL_W + 4.0f) / (float)RGD3_OVERLAY_ATLAS_W;
	const float v = (5.0f * RGD3_OVERLAY_CELL_H + 5.0f) / (float)RGD3_OVERLAY_ATLAS_H;
	if (!rgd3_overlay.batch_open || w <= 0.0f || h <= 0.0f)
		return;
	q = rgd3_overlay_alloc_quad();
	q->x0 = x;
	q->y0 = y;
	q->x1 = x + w;
	q->y1 = y + h;
	q->u0 = q->u1 = u;
	q->v0 = q->v1 = v;
	q->color = (RgTextColor){ rgba[0], rgba[1], rgba[2], rgba[3] };
}

void rgd3_overlay_char(float x, float y, int c, const float* rgba)
{
	const RgTextGlyph* g;
	RgTextQuad* q;
	if (!rgd3_overlay.batch_open || c <= ' ' || c > RGD3_OVERLAY_LAST_CHAR)
		return;
	g = rg_text_find_glyph(&rgd3_overlay.font, (u32)c);
	if (!g || !g->w)
		return;
	q = rgd3_overlay_alloc_quad();
	q->x0 = x;
	q->y0 = y;
	q->x1 = x + (float)g->w;
	q->y1 = y + (float)g->h;
	q->u0 = (float)g->x / (float)RGD3_OVERLAY_ATLAS_W;
	q->v0 = (float)g->y / (float)RGD3_OVERLAY_ATLAS_H;
	q->u1 = (float)(g->x + g->w) / (float)RGD3_OVERLAY_ATLAS_W;
	q->v1 = (float)(g->y + g->h) / (float)RGD3_OVERLAY_ATLAS_H;
	q->color = (RgTextColor){ rgba[0], rgba[1], rgba[2], rgba[3] };
}

/* Draws text without colour escapes; returns the x after the last cell. */
float rgd3_overlay_string(float x, float y, const char* text, const float* rgba, int shadow)
{
	static const float black[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	for (; *text; ++text, x += RGD3_OVERLAY_CHAR_WIDTH)
	{
		if (shadow)
			rgd3_overlay_char(x + 1.0f, y + 1.0f, (unsigned char)*text, black);
		rgd3_overlay_char(x, y, (unsigned char)*text, rgba);
	}
	return x;
}

/* The M0 diagnostics lines: right-aligned at the top right with a shadow. */
void rgd3_overlay_right_lines(const char* const* lines, int num_lines)
{
	static const float white[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	float vw, vh;
	int i;
	if (!rgd3_overlay.batch_open)
		return;
	rgd3_overlay_virtual_size(&vw, &vh);
	for (i = 0; i < num_lines && i < RGD3_OVERLAY_MAX_LINES; ++i)
	{
		float x = vw - 6.0f - (float)strlen(lines[i]) * RGD3_OVERLAY_CHAR_WIDTH;
		rgd3_overlay_string(x, 6.0f + (float)i * RGD3_OVERLAY_CHAR_HEIGHT, lines[i], white, 1);
	}
}

int rgd3_overlay_end(SDL_GPUCommandBuffer* command, SDL_GPUTexture* target)
{
	RgTextGpuUpload upload = {0};
	RgTextGpuUniforms uniforms = {0};
	SDL_GPUColorTargetInfo color = {0};
	SDL_GPUCopyPass* copy;
	SDL_GPURenderPass* pass;
	if (!rgd3_overlay.batch_open)
		return 0;
	rgd3_overlay_flush_quads();
	rgd3_overlay.batch_open = 0;
	if (!command || !target || !rgd3_overlay.text.quad_count)
		return 0;
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
	/* pixels to clip space, with the virtual-to-pixel scale in the transform */
	uniforms.projection[0] = 2.0f / (float)rgd3_overlay.width;
	uniforms.projection[5] = -2.0f / (float)rgd3_overlay.height;
	uniforms.projection[10] = 1.0f;
	uniforms.projection[15] = 1.0f;
	uniforms.projection[12] = -1.0f;
	uniforms.projection[13] = 1.0f;
	uniforms.transform[0] = rgd3_overlay.scale;
	uniforms.transform[5] = rgd3_overlay.scale;
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

int rgd3_overlay_draw(SDL_GPUCommandBuffer* command, SDL_GPUTexture* target, u32 width, u32 height,
	const char* const* lines, int num_lines)
{
	if (num_lines <= 0 || !rgd3_overlay_begin(width, height))
		return 0;
	rgd3_overlay_right_lines(lines, num_lines);
	return rgd3_overlay_end(command, target);
}
