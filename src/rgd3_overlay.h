/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_overlay.h - 2D text and filled rectangles drawn with rg_text on the
   swapchain until the real 2D renderer exists (M3). The font is the 5x7
   demo font from rg_text/examples/assets (MIT), as rg_doom uses, extended
   to all printable ASCII.

   Coordinates are virtual pixels: one unit is one font pixel, and the
   virtual screen is the window size divided by an integer scale
   (floor(min(w / 640, h / 360)), 1..4), so a 16:9 window is about 640x360
   units and 106 character cells wide. A frame is one batch:

	if (rgd3_overlay_begin(width, height))
	{
		rgd3_overlay_fill / _char / _string / _right_lines ...
		rgd3_overlay_end(command_buffer, swapchain_texture);
	}

   rgd3_overlay_draw() is the M0 one-call form (begin, right-aligned
   lines, end). Every call is a no-op without a GPU device. */
#ifndef RGD3_OVERLAY_H
#define RGD3_OVERLAY_H
#ifndef RGD3_ALL_C
#error "rgd3 headers are included through src/all.c only"
#endif

#define RGD3_OVERLAY_MAX_LINES 8
#define RGD3_OVERLAY_CHAR_WIDTH 6	/* cell advance in virtual pixels */
#define RGD3_OVERLAY_CHAR_HEIGHT 10	/* line height in virtual pixels */
#define RGD3_OVERLAY_GLYPH_HEIGHT 7	/* drawn rows of a glyph */

int rgd3_overlay_create(SDL_GPUDevice* device, SDL_GPUTextureFormat target_format, const char* shader_root);
void rgd3_overlay_destroy(void);
int rgd3_overlay_draw(SDL_GPUCommandBuffer* command, SDL_GPUTexture* target, u32 width, u32 height,
	const char* const* lines, int num_lines);

int rgd3_overlay_begin(u32 width, u32 height);
void rgd3_overlay_virtual_size(float* width, float* height);
void rgd3_overlay_fill(float x, float y, float w, float h, const float* rgba);
void rgd3_overlay_char(float x, float y, int c, const float* rgba);
float rgd3_overlay_string(float x, float y, const char* text, const float* rgba, int shadow);
void rgd3_overlay_right_lines(const char* const* lines, int num_lines);
int rgd3_overlay_end(SDL_GPUCommandBuffer* command, SDL_GPUTexture* target);

#endif
