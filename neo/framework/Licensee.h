/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2012 Robert Beckebans

This file is part of the Doom 3 BFG Edition GPL Source Code ("Doom 3 BFG Edition Source Code").

Doom 3 BFG Edition Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 BFG Edition Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 BFG Edition Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 BFG Edition Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 BFG Edition Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

/*
===============================================================================

	Definitions for information that is related to a licensee's game name and location.

===============================================================================
*/

#define GAME_NAME						"DOOM 3: BFG Edition"		// appears on window titles and errors

// RB: changed home folder so we don't break the savegame of the original game
#define SAVE_PATH						"\\id Software\\RBDOOM 3 BFG"

#define ENGINE_VERSION					"RBDOOM 3 BFG 1.0.3"	// printed in console
// RB end

#define	BASE_GAMEDIR					"base"

#define CONFIG_FILE						"D3BFGConfig.cfg"

// see ASYNC_PROTOCOL_VERSION
// use a different major for each game
#define ASYNC_PROTOCOL_MAJOR			1

// <= Doom v1.1: 1. no DS_VERSION token ( default )
// Doom v1.2:  2
// Doom 3 BFG: 3
#define RENDERDEMO_VERSION				3

// win32 info
#define WIN32_CONSOLE_CLASS				"D3BFG_WinConsole"
#define	WIN32_WINDOW_CLASS_NAME			"D3BFG"
#define	WIN32_FAKE_WINDOW_CLASS_NAME	"D3BFG_WGL_FAKE"

// RB begin
// Default base path (used only if none could be found)
#ifdef __APPLE__
#define DEFAULT_BASEPATH				"/Applications/RBDOOM-3-BFG.app/Contents/Resources"
#else
#define DEFAULT_BASEPATH				"/usr/share/games/doom3bfg"
#endif
// RB end


