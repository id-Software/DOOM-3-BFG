/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_platform.h - OS specifics: directories, Steam install detection,
   message boxes. Paths use '/' separators and carry no trailing slash. */
#ifndef RGD3_PLATFORM_H
#define RGD3_PLATFORM_H
#ifndef RGD3_ALL_C
#error "rgd3 headers are included through src/all.c only"
#endif

#define RGD3_STEAM_GAME_DIR "DOOM 3 BFG Edition"

void rgd3_platform_init(void);
void rgd3_platform_shutdown(void);
const char* rgd3_platform_cwd(void);
const char* rgd3_platform_exe_dir(void);
const char* rgd3_platform_pref_path(void);
int rgd3_platform_is_file(const char* path);
int rgd3_platform_is_dir(const char* path);
int rgd3_platform_find_steam_install(char* out, size_t size);
void rgd3_platform_normalize_path(char* path);

#endif
