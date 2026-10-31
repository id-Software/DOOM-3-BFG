/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_platform.c - directory discovery replaces the Win32 code in
   neo/sys/win32/win_main.cpp (Sys_DefaultBasePath / Sys_DefaultSavePath).
   Writes always go to SDL_GetPrefPath; reads auto-detect the Steam install. */

typedef struct Rgd3PlatformState
{
	char cwd[RGD3_MAX_OSPATH];
	char exe_dir[RGD3_MAX_OSPATH];
	char pref_path[RGD3_MAX_OSPATH];
} Rgd3PlatformState;

static Rgd3PlatformState rgd3_platform;

void rgd3_platform_normalize_path(char* path)
{
	rgd3_str_backslashes_to_slashes(path);
	rgd3_str_strip_trailing_slash(path);
}

static void rgd3_platform_copy_sdl_path(char* dst, char* sdl_owned)
{
	rgd3_str_copynz(dst, sdl_owned ? sdl_owned : ".", RGD3_MAX_OSPATH);
	rgd3_platform_normalize_path(dst);
	if (sdl_owned)
		SDL_free(sdl_owned);
}

void rgd3_platform_init(void)
{
	const char* base = SDL_GetBasePath();
	rgd3_platform_copy_sdl_path(rgd3_platform.cwd, SDL_GetCurrentDirectory());
	rgd3_str_copynz(rgd3_platform.exe_dir, base ? base : ".", sizeof(rgd3_platform.exe_dir));
	rgd3_platform_normalize_path(rgd3_platform.exe_dir);
	rgd3_platform_copy_sdl_path(rgd3_platform.pref_path, SDL_GetPrefPath(RGD3_PREF_ORG, RGD3_PREF_APP));
}

void rgd3_platform_shutdown(void)
{
}

const char* rgd3_platform_cwd(void)
{
	return rgd3_platform.cwd;
}

const char* rgd3_platform_exe_dir(void)
{
	return rgd3_platform.exe_dir;
}

const char* rgd3_platform_pref_path(void)
{
	return rgd3_platform.pref_path;
}

int rgd3_platform_is_file(const char* path)
{
	SDL_PathInfo info;
	return SDL_GetPathInfo(path, &info) && info.type == SDL_PATHTYPE_FILE;
}

int rgd3_platform_is_dir(const char* path)
{
	SDL_PathInfo info;
	return SDL_GetPathInfo(path, &info) && info.type == SDL_PATHTYPE_DIRECTORY;
}

static int rgd3_platform_check_steam_library(const char* library, char* out, size_t size)
{
	char game[RGD3_MAX_OSPATH];
	char probe[RGD3_MAX_OSPATH];
	rg_snprintf(game, sizeof(game), "%s/steamapps/common/" RGD3_STEAM_GAME_DIR, library);
	rgd3_platform_normalize_path(game);
	rg_snprintf(probe, sizeof(probe), "%s/" RGD3_BASE_GAMEDIR "/_common.resources", game);
	if (!rgd3_platform_is_file(probe))
		return 0;
	rgd3_str_copynz(out, game, size);
	return 1;
}

#if defined(_WIN32)
static int rgd3_platform_registry_string(HKEY root, const char* key, const char* value, char* out, DWORD size)
{
	DWORD bytes = size;
	if (RegGetValueA(root, key, value, RRF_RT_REG_SZ, NULL, out, &bytes) != ERROR_SUCCESS)
		return 0;
	out[size - 1] = 0;
	return out[0] != 0;
}
#endif

/* libraryfolders.vdf lists every Steam library as "path" "<dir>" with
   backslashes escaped. Only the "path" keys matter here. */
static int rgd3_platform_scan_library_folders(const char* steam_root, char* out, size_t size)
{
	char vdf[RGD3_MAX_OSPATH];
	size_t len = 0;
	char* text;
	const char* p;
	int found = 0;
	rg_snprintf(vdf, sizeof(vdf), "%s/steamapps/libraryfolders.vdf", steam_root);
	text = (char*)SDL_LoadFile(vdf, &len);
	if (!text)
		return 0;
	p = text;
	while (!found && (p = strstr(p, "\"path\"")) != NULL)
	{
		char library[RGD3_MAX_OSPATH];
		size_t n = 0;
		p += 6;
		while (*p && *p != '"')
			++p;
		if (*p != '"')
			break;
		++p;
		while (*p && *p != '"' && n + 1 < sizeof(library))
		{
			if (p[0] == '\\' && p[1] == '\\')
				++p;
			library[n++] = *p++;
		}
		library[n] = 0;
		found = rgd3_platform_check_steam_library(library, out, size);
	}
	SDL_free(text);
	return found;
}

int rgd3_platform_find_steam_install(char* out, size_t size)
{
	char steam[RGD3_MAX_OSPATH] = {0};
#if defined(_WIN32)
	if (!rgd3_platform_registry_string(HKEY_CURRENT_USER, "Software\\Valve\\Steam", "SteamPath", steam, sizeof(steam)) &&
		!rgd3_platform_registry_string(HKEY_LOCAL_MACHINE, "SOFTWARE\\WOW6432Node\\Valve\\Steam", "InstallPath", steam, sizeof(steam)))
	{
		rgd3_str_copynz(steam, "C:/Program Files (x86)/Steam", sizeof(steam));
	}
#else
	const char* home = SDL_getenv("HOME");
	rg_snprintf(steam, sizeof(steam), "%s/.local/share/Steam", home ? home : "");
#endif
	rgd3_platform_normalize_path(steam);
	if (rgd3_platform_check_steam_library(steam, out, size))
		return 1;
	return rgd3_platform_scan_library_folders(steam, out, size);
}
