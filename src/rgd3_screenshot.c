/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_screenshot.c - from neo/renderer/RenderSystem_init.cpp: the
   screenshot command's argument forms, R_ScreenshotFilename numbering
   (screenshots/shot%05i, first number whose file does not exist, the last
   number remembered so later shots do not rescan), console->Close() before
   the shot, and TakeScreenshot's TGA layout (18-byte header, type 2,
   24 bits, BGR rows bottom-up). The capture itself is new: the swapchain
   texture is downloaded through an SDL GPU copy pass at the end of the
   frame, and PNG (stb_image_write) is the default format.

   Not ported: tiled rendering at sizes larger than the window (the image
   is resampled instead), jittered multi-sample blends, fs_restrict. */

typedef struct Rgd3ScreenshotState
{
	int pending;
	int width;			/* requested size, 0 = window size */
	int height;
	char file_name[RGD3_MAX_OSPATH];
	int last_number;
} Rgd3ScreenshotState;

static Rgd3ScreenshotState rgd3_screenshot;

static const char* rgd3_screenshot_formats[] = { "png", "tga", NULL };
static Rgd3Cvar cv_r_screenshotFormat = RGD3_CVAR_STRINGS("r_screenshotFormat", "png", RGD3_CVAR_RENDERER | RGD3_CVAR_ARCHIVE,
	"screenshot file format: png or tga (24-bit uncompressed, as the original)", rgd3_screenshot_formats);
static Rgd3Cvar cv_r_screenshotKeepConsole = RGD3_CVAR("r_screenshotKeepConsole", "0", RGD3_CVAR_RENDERER | RGD3_CVAR_BOOL,
	"1 = screenshot leaves the console open so it is in the picture");

void rgd3_screenshot_shutdown(void)
{
	rgd3_screenshot.pending = 0;
}

int rgd3_screenshot_pending(void)
{
	return rgd3_screenshot.pending;
}

/* R_ScreenshotFilename: base + five digits + extension, the first that
   does not exist, starting after the last number used */
static void rgd3_screenshot_filename(int* last_number, const char* base, const char* ext, char* out, size_t size)
{
	(*last_number)++;
	if (*last_number > 99999)
		*last_number = 99999;
	for (; *last_number < 99999; (*last_number)++)
	{
		rg_snprintf(out, size, "%s%05d.%s", base, *last_number, ext);
		if (!rgd3_fs_file_exists(out))
			return;
	}
	rg_snprintf(out, size, "%s%05d.%s", base, *last_number, ext);
}

static void rgd3_screenshot_cmd(const Rgd3CmdArgs* args)
{
	const char* ext = cv_r_screenshotFormat.integer_value == 1 ? "tga" : "png";
	int argc = rgd3_cmdargs_argc(args);
	int width = 0, height = 0;
	switch (argc)
	{
		case 1:
			rgd3_screenshot_filename(&rgd3_screenshot.last_number, "screenshots/shot", ext, rgd3_screenshot.file_name, sizeof(rgd3_screenshot.file_name));
			break;
		case 2:
			rgd3_str_copynz(rgd3_screenshot.file_name, rgd3_cmdargs_argv(args, 1), sizeof(rgd3_screenshot.file_name));
			break;
		case 3:
		case 4: /* blends: accepted for compatibility, one sample is taken */
			width = atoi(rgd3_cmdargs_argv(args, 1));
			height = atoi(rgd3_cmdargs_argv(args, 2));
			rgd3_screenshot_filename(&rgd3_screenshot.last_number, "screenshots/shot", ext, rgd3_screenshot.file_name, sizeof(rgd3_screenshot.file_name));
			break;
		default:
			rgd3_printf("usage: screenshot\n       screenshot <filename>\n       screenshot <width> <height>\n       screenshot <width> <height> <blends>\n");
			return;
	}
	if (!rgd3_app_gpu() || !rgd3_app_window())
	{
		rgd3_printf("screenshot: no window\n");
		return;
	}
	if (width < 0 || height < 0 || width > 16384 || height > 16384)
	{
		rgd3_printf("screenshot: bad size %dx%d\n", width, height);
		return;
	}
	/* put the console away */
	if (!cv_r_screenshotKeepConsole.integer_value)
		rgd3_console_close();
	rgd3_screenshot.width = width;
	rgd3_screenshot.height = height;
	rgd3_screenshot.pending = 1;
}

/* bilinear resample of tightly packed RGB rows */
static void rgd3_screenshot_resample(const u8* src, int sw, int sh, u8* dst, int dw, int dh)
{
	int x, y, c;
	for (y = 0; y < dh; y++)
	{
		float fy = ((float)y + 0.5f) * (float)sh / (float)dh - 0.5f;
		int y0 = (int)floorf(fy);
		float ty = fy - (float)y0;
		int y1 = y0 + 1;
		y0 = y0 < 0 ? 0 : (y0 >= sh ? sh - 1 : y0);
		y1 = y1 < 0 ? 0 : (y1 >= sh ? sh - 1 : y1);
		for (x = 0; x < dw; x++)
		{
			float fx = ((float)x + 0.5f) * (float)sw / (float)dw - 0.5f;
			int x0 = (int)floorf(fx);
			float tx = fx - (float)x0;
			int x1 = x0 + 1;
			x0 = x0 < 0 ? 0 : (x0 >= sw ? sw - 1 : x0);
			x1 = x1 < 0 ? 0 : (x1 >= sw ? sw - 1 : x1);
			for (c = 0; c < 3; c++)
			{
				float a = (float)src[(y0 * sw + x0) * 3 + c] * (1.0f - tx) + (float)src[(y0 * sw + x1) * 3 + c] * tx;
				float b = (float)src[(y1 * sw + x0) * 3 + c] * (1.0f - tx) + (float)src[(y1 * sw + x1) * 3 + c] * tx;
				dst[(y * dw + x) * 3 + c] = (u8)(a * (1.0f - ty) + b * ty + 0.5f);
			}
		}
	}
}

typedef struct Rgd3ScreenshotPngOut
{
	Rgd3File* file;
	int failed; /* a short write anywhere fails the whole file */
} Rgd3ScreenshotPngOut;

static void rgd3_screenshot_png_write(void* context, void* data, int size)
{
	Rgd3ScreenshotPngOut* out = (Rgd3ScreenshotPngOut*)context;
	if (out->failed)
		return;
	if (rgd3_fs_write(out->file, data, (size_t)size) != (size_t)size)
		out->failed = 1;
}

/* encodes into an open file; 0 if any write came up short */
static int rgd3_screenshot_encode(Rgd3File* f, int tga, const u8* rgb, int w, int h)
{
	int ok;
	if (tga)
	{
		/* TakeScreenshot's header and bottom-up BGR rows */
		u8 header[18];
		u8* row = (u8*)rgd3_heap_alloc((size_t)w * 3, RGD3_TAG_TEMP);
		int x, y;
		memset(header, 0, sizeof(header));
		header[2] = 2; /* uncompressed type */
		header[12] = (u8)(w & 255);
		header[13] = (u8)(w >> 8);
		header[14] = (u8)(h & 255);
		header[15] = (u8)(h >> 8);
		header[16] = 24; /* pixel size */
		ok = rgd3_fs_write(f, header, sizeof(header)) == sizeof(header);
		for (y = h - 1; y >= 0 && ok; y--)
		{
			const u8* s = rgb + (size_t)y * (size_t)w * 3;
			for (x = 0; x < w; x++)
			{
				row[x * 3 + 0] = s[x * 3 + 2];
				row[x * 3 + 1] = s[x * 3 + 1];
				row[x * 3 + 2] = s[x * 3 + 0];
			}
			ok = rgd3_fs_write(f, row, (size_t)w * 3) == (size_t)w * 3;
		}
		rgd3_heap_free(row);
	}
	else
	{
		Rgd3ScreenshotPngOut out;
		out.file = f;
		out.failed = 0;
		/* the callback cannot report errors, so stb returns success
		   even when writes fail; the failed flag carries them */
		ok = stbi_write_png_to_func(rgd3_screenshot_png_write, &out, w, h, 3, rgb, w * 3) != 0 && !out.failed;
	}
	return ok;
}

static int rgd3_screenshot_write(const char* file_name, const u8* rgb, int w, int h)
{
	const char* ext = rgd3_str_file_extension(file_name);
	int tga = rgd3_str_icmp(ext, ".tga") == 0;
	int ok;
	Rgd3File* f;
	/* a named file without .png/.tga gets the cvar's format */
	if (!tga && rgd3_str_icmp(ext, ".png") != 0)
		tga = cv_r_screenshotFormat.integer_value == 1;
	f = rgd3_fs_open_write(file_name);
	if (!f)
		return 0;
	ok = rgd3_screenshot_encode(f, tga, rgb, w, h);
	rgd3_fs_close(f);
	return ok;
}

/* encoding into a memory file: PNG and TGA headers are right, and a file
   that rejects writes fails both formats */
int rgd3_screenshot_self_test(void)
{
	u8 rgb[4 * 3 * 3];
	static const u8 png_sig[8] = { 0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n' };
	u8 head[18];
	int ok = 1, i;
	Rgd3File* f;
	for (i = 0; i < (int)sizeof(rgb); i++)
		rgb[i] = (u8)(i * 7);

	f = rgd3_fs_open_memory_write("shot.png");
	ok &= rgd3_screenshot_encode(f, 0, rgb, 4, 3);
	rgd3_fs_memory_make_read_only(f);
	ok &= rgd3_fs_read(f, head, 8) == 8 && memcmp(head, png_sig, 8) == 0;
	/* now read-only: every write fails */
	ok &= !rgd3_screenshot_encode(f, 0, rgb, 4, 3);
	ok &= !rgd3_screenshot_encode(f, 1, rgb, 4, 3);
	rgd3_fs_close(f);

	f = rgd3_fs_open_memory_write("shot.tga");
	ok &= rgd3_screenshot_encode(f, 1, rgb, 4, 3);
	ok &= rgd3_fs_length(f) == 18 + (i64)sizeof(rgb);
	rgd3_fs_memory_make_read_only(f);
	ok &= rgd3_fs_read(f, head, 18) == 18 && head[2] == 2 && head[12] == 4 && head[14] == 3 && head[16] == 24;
	/* the first TGA row is the bottom one, as BGR */
	ok &= rgd3_fs_read(f, head, 3) == 3 && head[0] == rgb[2 * 12 + 2] && head[2] == rgb[2 * 12 + 0];
	rgd3_fs_close(f);

	if (!ok)
		rgd3_printf("screenshot: self-test FAILED\n");
	return ok;
}

/* swapchain texels to tightly packed RGB; 0 for formats it does not know */
static int rgd3_screenshot_convert(const u8* src, u32 pitch, u32 w, u32 h, SDL_GPUTextureFormat format, u8* rgb)
{
	u32 x, y;
	int bgra = format == SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM || format == SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM_SRGB;
	int rgba = format == SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM || format == SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM_SRGB;
	int rgb10 = format == SDL_GPU_TEXTUREFORMAT_R10G10B10A2_UNORM;
	if (!bgra && !rgba && !rgb10)
		return 0;
	for (y = 0; y < h; y++)
	{
		const u8* s = src + (size_t)y * pitch;
		u8* d = rgb + (size_t)y * w * 3;
		for (x = 0; x < w; x++, s += 4, d += 3)
		{
			if (bgra)
			{
				d[0] = s[2];
				d[1] = s[1];
				d[2] = s[0];
			}
			else if (rgba)
			{
				d[0] = s[0];
				d[1] = s[1];
				d[2] = s[2];
			}
			else
			{
				u32 v = (u32)s[0] | ((u32)s[1] << 8) | ((u32)s[2] << 16) | ((u32)s[3] << 24);
				d[0] = (u8)(((v >> 0) & 1023) >> 2);
				d[1] = (u8)(((v >> 10) & 1023) >> 2);
				d[2] = (u8)(((v >> 20) & 1023) >> 2);
			}
		}
	}
	return 1;
}

void rgd3_screenshot_submit(SDL_GPUDevice* device, SDL_GPUCommandBuffer* command, SDL_GPUTexture* swapchain,
	u32 width, u32 height, SDL_GPUTextureFormat format)
{
	SDL_GPUTransferBufferCreateInfo info = {0};
	SDL_GPUTextureRegion source = {0};
	SDL_GPUTextureTransferInfo destination = {0};
	SDL_GPUTransferBuffer* transfer;
	SDL_GPUCopyPass* copy;
	SDL_GPUFence* fence;
	const u8* pixels;
	u8* rgb;
	u32 pitch = (width * 4u + 255u) & ~255u;
	int ok = 0;

	if (!rgd3_screenshot.pending || !device || !swapchain || !width || !height)
	{
		SDL_SubmitGPUCommandBuffer(command);
		return;
	}
	rgd3_screenshot.pending = 0;

	info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD;
	info.size = pitch * height;
	transfer = SDL_CreateGPUTransferBuffer(device, &info);
	if (!transfer)
	{
		rgd3_warning("screenshot: SDL_CreateGPUTransferBuffer failed: %s", SDL_GetError());
		SDL_SubmitGPUCommandBuffer(command);
		return;
	}
	copy = SDL_BeginGPUCopyPass(command);
	if (copy)
	{
		source.texture = swapchain;
		source.w = width;
		source.h = height;
		source.d = 1;
		destination.transfer_buffer = transfer;
		destination.pixels_per_row = pitch / 4u;
		destination.rows_per_layer = height;
		SDL_DownloadFromGPUTexture(copy, &source, &destination);
		SDL_EndGPUCopyPass(copy);
	}
	fence = SDL_SubmitGPUCommandBufferAndAcquireFence(command);
	if (!copy || !fence || !SDL_WaitForGPUFences(device, true, &fence, 1))
	{
		rgd3_warning("screenshot: GPU read-back failed: %s", SDL_GetError());
		if (fence)
			SDL_ReleaseGPUFence(device, fence);
		SDL_ReleaseGPUTransferBuffer(device, transfer);
		return;
	}
	SDL_ReleaseGPUFence(device, fence);

	pixels = (const u8*)SDL_MapGPUTransferBuffer(device, transfer, false);
	rgb = (u8*)rgd3_heap_alloc((size_t)width * height * 3, RGD3_TAG_TEMP);
	if (pixels && rgd3_screenshot_convert(pixels, pitch, width, height, format, rgb))
	{
		int w = rgd3_screenshot.width > 0 ? rgd3_screenshot.width : (int)width;
		int h = rgd3_screenshot.height > 0 ? rgd3_screenshot.height : (int)height;
		SDL_UnmapGPUTransferBuffer(device, transfer);
		pixels = NULL;
		if (w != (int)width || h != (int)height)
		{
			u8* scaled = (u8*)rgd3_heap_alloc((size_t)w * (size_t)h * 3, RGD3_TAG_TEMP);
			rgd3_screenshot_resample(rgb, (int)width, (int)height, scaled, w, h);
			ok = rgd3_screenshot_write(rgd3_screenshot.file_name, scaled, w, h);
			rgd3_heap_free(scaled);
		}
		else
		{
			ok = rgd3_screenshot_write(rgd3_screenshot.file_name, rgb, w, h);
		}
	}
	else if (pixels)
	{
		rgd3_warning("screenshot: swapchain format %d is not supported", (int)format);
	}
	if (pixels)
		SDL_UnmapGPUTransferBuffer(device, transfer);
	rgd3_heap_free(rgb);
	SDL_ReleaseGPUTransferBuffer(device, transfer);
	if (ok)
		rgd3_printf("Wrote %s\n", rgd3_screenshot.file_name);
	else
		rgd3_printf("screenshot: could not write %s\n", rgd3_screenshot.file_name);
}

void rgd3_screenshot_register(void)
{
	static Rgd3Cvar* const cvars[] = { &cv_r_screenshotFormat, &cv_r_screenshotKeepConsole };
	rgd3_cvar_register_table(cvars, (int)RG_ARRAY_COUNT(cvars));
	rgd3_cmd_add("screenshot", rgd3_screenshot_cmd, RGD3_CMD_FL_RENDERER, "takes a screenshot", NULL);
}
