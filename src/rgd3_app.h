/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_app.h - entry point, startup order and the frame loop
   (idCommonLocal::Init / Frame, neo/framework/Common.cpp, common_frame.cpp). */
#ifndef RGD3_APP_H
#define RGD3_APP_H
#ifndef RGD3_ALL_C
#error "rgd3 headers are included through src/all.c only"
#endif

void rgd3_app_register(void);
int rgd3_app_run(int argc, char** argv);
void rgd3_app_quit(void);
void rgd3_app_process_event(const Rgd3SysEvent* ev);
void rgd3_app_write_configuration(int force);
void rgd3_app_stop_map(void);
SDL_Window* rgd3_app_window(void);
SDL_GPUDevice* rgd3_app_gpu(void);

#endif
