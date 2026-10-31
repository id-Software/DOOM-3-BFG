/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_screenshot.h - the `screenshot` command (R_ScreenShot_f,
   R_ScreenshotFilename, idRenderSystemLocal::TakeScreenshot in
   neo/renderer/RenderSystem_init.cpp).

	screenshot                       screenshots/shot<NNNNN>.<fmt>, first free number
	screenshot <filename>            that file
	screenshot <width> <height>      resampled to that size
	screenshot <width> <height> <n>  as above; blends are accepted and ignored

   Until the scene renderer exists the shot is the window's swapchain image
   of the next frame, read back after everything is drawn. Files go under
   the save path (<savepath>/base/...). r_screenshotFormat picks png
   (default) or tga (the original's 24-bit uncompressed bottom-up layout).
   Like the original the command puts the console away first;
   r_screenshotKeepConsole 1 keeps it in the picture. */
#ifndef RGD3_SCREENSHOT_H
#define RGD3_SCREENSHOT_H
#ifndef RGD3_ALL_C
#error "rgd3 headers are included through src/all.c only"
#endif

void rgd3_screenshot_register(void);
void rgd3_screenshot_shutdown(void);	/* drops a pending request */
int rgd3_screenshot_pending(void);
int rgd3_screenshot_self_test(void);

/* Submits the frame's command buffer in place of SDL_SubmitGPUCommandBuffer.
   With a shot pending and a swapchain texture, it records a download of the
   texture after everything drawn so far, submits with a fence, waits, and
   writes the file; otherwise it only submits. */
void rgd3_screenshot_submit(SDL_GPUDevice* device, SDL_GPUCommandBuffer* command, SDL_GPUTexture* swapchain,
	u32 width, u32 height, SDL_GPUTextureFormat format);

#endif
