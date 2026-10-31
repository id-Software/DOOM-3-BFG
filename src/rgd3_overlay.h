/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_overlay.h - diagnostics text drawn with rg_text on the swapchain.
   Uses the 5x7 demo font from rg_text/examples/assets (MIT), as rg_doom does. */
#ifndef RGD3_OVERLAY_H
#define RGD3_OVERLAY_H
#ifndef RGD3_ALL_C
#error "rgd3 headers are included through src/all.c only"
#endif

#define RGD3_OVERLAY_MAX_LINES 8

int rgd3_overlay_create(SDL_GPUDevice* device, SDL_GPUTextureFormat target_format, const char* shader_root);
void rgd3_overlay_destroy(void);
int rgd3_overlay_draw(SDL_GPUCommandBuffer* command, SDL_GPUTexture* target, u32 width, u32 height,
	const char* const* lines, int num_lines);

#endif
