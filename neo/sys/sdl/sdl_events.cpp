/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.
Copyright (C) 2012 dhewg (dhewm3)
Copyright (C) 2012 Robert Beckebans
Copyright (C) 2013 Daniel Gibson

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.	If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "../../idlib/precompiled.h"

#include <SDL.h>

#include "renderer/tr_local.h"
#include "sdl_local.h"
#include "../posix/posix_public.h"

#if !SDL_VERSION_ATLEAST(2, 0, 0)
#define SDL_Keycode SDLKey
#define SDLK_APPLICATION SDLK_COMPOSE
#define SDLK_SCROLLLOCK SDLK_SCROLLOCK
#define SDLK_LGUI SDLK_LSUPER
#define SDLK_RGUI SDLK_RSUPER
#define SDLK_KP_0 SDLK_KP0
#define SDLK_KP_1 SDLK_KP1
#define SDLK_KP_2 SDLK_KP2
#define SDLK_KP_3 SDLK_KP3
#define SDLK_KP_4 SDLK_KP4
#define SDLK_KP_5 SDLK_KP5
#define SDLK_KP_6 SDLK_KP6
#define SDLK_KP_7 SDLK_KP7
#define SDLK_KP_8 SDLK_KP8
#define SDLK_KP_9 SDLK_KP9
#define SDLK_NUMLOCKCLEAR SDLK_NUMLOCK
#define SDLK_PRINTSCREEN SDLK_PRINT
// DG: SDL1 doesn't seem to have defines for scancodes.. add the (only) one we need
#define SDL_SCANCODE_GRAVE 49 // in SDL2 this is 53.. but according to two different systems and keyboards this works for SDL1
// DG end
#endif

// DG: those are needed for moving/resizing windows
extern idCVar r_windowX;
extern idCVar r_windowY;
extern idCVar r_windowWidth;
extern idCVar r_windowHeight;
// DG end

const char* kbdNames[] =
{
	"english", "french", "german", "italian", "spanish", "turkish", "norwegian", NULL
};

idCVar in_kbd( "in_kbd", "english", CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_NOCHEAT, "keyboard layout", kbdNames, idCmdSystem::ArgCompletion_String<kbdNames> );

struct kbd_poll_t
{
	int key;
	bool state;
	
	kbd_poll_t()
	{
	}
	
	kbd_poll_t( int k, bool s )
	{
		key = k;
		state = s;
	}
};

struct mouse_poll_t
{
	int action;
	int value;
	
	mouse_poll_t()
	{
	}
	
	mouse_poll_t( int a, int v )
	{
		action = a;
		value = v;
	}
};

static idList<kbd_poll_t> kbd_polls;
static idList<mouse_poll_t> mouse_polls;

// RB begin
static int SDL_KeyToDoom3Key( SDL_Keycode key, bool& isChar )
{
	isChar = false;
	
	if( key >= SDLK_SPACE && key < SDLK_DELETE )
	{
		isChar = true;
		//return key;// & 0xff;
	}
	
	switch( key )
	{
		case SDLK_ESCAPE:
			return K_ESCAPE;
			
		case SDLK_SPACE:
			return K_SPACE;
			
			//case SDLK_EXCLAIM:
			/*
			SDLK_QUOTEDBL:
			SDLK_HASH:
			SDLK_DOLLAR:
			SDLK_AMPERSAND:
			SDLK_QUOTE		= 39,
			SDLK_LEFTPAREN		= 40,
			SDLK_RIGHTPAREN		= 41,
			SDLK_ASTERISK		= 42,
			SDLK_PLUS		= 43,
			SDLK_COMMA		= 44,
			SDLK_MINUS		= 45,
			SDLK_PERIOD		= 46,
			SDLK_SLASH		= 47,
			*/
		case SDLK_0:
			return K_0;
			
		case SDLK_1:
			return K_1;
			
		case SDLK_2:
			return K_2;
			
		case SDLK_3:
			return K_3;
			
		case SDLK_4:
			return K_4;
			
		case SDLK_5:
			return K_5;
			
		case SDLK_6:
			return K_6;
			
		case SDLK_7:
			return K_7;
			
		case SDLK_8:
			return K_8;
			
		case SDLK_9:
			return K_9;
			
			// DG: add some missing keys..
		case SDLK_UNDERSCORE:
			return K_UNDERLINE;
			
		case SDLK_MINUS:
			return K_MINUS;
			
		case SDLK_COMMA:
			return K_COMMA;
			
		case SDLK_COLON:
			return K_COLON;
			
		case SDLK_SEMICOLON:
			return K_SEMICOLON;
			
		case SDLK_PERIOD:
			return K_PERIOD;
			
		case SDLK_AT:
			return K_AT;
			
		case SDLK_EQUALS:
			return K_EQUALS;
			// DG end
			
			/*
			SDLK_COLON		= 58,
			SDLK_SEMICOLON		= 59,
			SDLK_LESS		= 60,
			SDLK_EQUALS		= 61,
			SDLK_GREATER		= 62,
			SDLK_QUESTION		= 63,
			SDLK_AT			= 64,
			*/
			/*
			   Skip uppercase letters
			 */
			/*
			SDLK_LEFTBRACKET	= 91,
			SDLK_BACKSLASH		= 92,
			SDLK_RIGHTBRACKET	= 93,
			SDLK_CARET		= 94,
			SDLK_UNDERSCORE		= 95,
			SDLK_BACKQUOTE		= 96,
			*/
			
		case SDLK_a:
			return K_A;
			
		case SDLK_b:
			return K_B;
			
		case SDLK_c:
			return K_C;
			
		case SDLK_d:
			return K_D;
			
		case SDLK_e:
			return K_E;
			
		case SDLK_f:
			return K_F;
			
		case SDLK_g:
			return K_G;
			
		case SDLK_h:
			return K_H;
			
		case SDLK_i:
			return K_I;
			
		case SDLK_j:
			return K_J;
			
		case SDLK_k:
			return K_K;
			
		case SDLK_l:
			return K_L;
			
		case SDLK_m:
			return K_M;
			
		case SDLK_n:
			return K_N;
			
		case SDLK_o:
			return K_O;
			
		case SDLK_p:
			return K_P;
			
		case SDLK_q:
			return K_Q;
			
		case SDLK_r:
			return K_R;
			
		case SDLK_s:
			return K_S;
			
		case SDLK_t:
			return K_T;
			
		case SDLK_u:
			return K_U;
			
		case SDLK_v:
			return K_V;
			
		case SDLK_w:
			return K_W;
			
		case SDLK_x:
			return K_X;
			
		case SDLK_y:
			return K_Y;
			
		case SDLK_z:
			return K_Z;
			
		case SDLK_RETURN:
			return K_ENTER;
			
		case SDLK_BACKSPACE:
			return K_BACKSPACE;
			
		case SDLK_PAUSE:
			return K_PAUSE;
			
			// DG: add tab key support
		case SDLK_TAB:
			return K_TAB;
			// DG end
			
			//case SDLK_APPLICATION:
			//	return K_COMMAND;
		case SDLK_CAPSLOCK:
			return K_CAPSLOCK;
			
		case SDLK_SCROLLLOCK:
			return K_SCROLL;
			
		case SDLK_POWER:
			return K_POWER;
			
		case SDLK_UP:
			return K_UPARROW;
			
		case SDLK_DOWN:
			return K_DOWNARROW;
			
		case SDLK_LEFT:
			return K_LEFTARROW;
			
		case SDLK_RIGHT:
			return K_RIGHTARROW;
			
		case SDLK_LGUI:
			return K_LWIN;
			
		case SDLK_RGUI:
			return K_RWIN;
			//case SDLK_MENU:
			//	return K_MENU;
			
		case SDLK_LALT:
			return K_LALT;
			
		case SDLK_RALT:
			return K_RALT;
			
		case SDLK_RCTRL:
			return K_RCTRL;
			
		case SDLK_LCTRL:
			return K_LCTRL;
			
		case SDLK_RSHIFT:
			return K_RSHIFT;
			
		case SDLK_LSHIFT:
			return K_LSHIFT;
			
		case SDLK_INSERT:
			return K_INS;
			
		case SDLK_DELETE:
			return K_DEL;
			
		case SDLK_PAGEDOWN:
			return K_PGDN;
			
		case SDLK_PAGEUP:
			return K_PGUP;
			
		case SDLK_HOME:
			return K_HOME;
			
		case SDLK_END:
			return K_END;
			
		case SDLK_F1:
			return K_F1;
			
		case SDLK_F2:
			return K_F2;
			
		case SDLK_F3:
			return K_F3;
			
		case SDLK_F4:
			return K_F4;
			
		case SDLK_F5:
			return K_F5;
			
		case SDLK_F6:
			return K_F6;
			
		case SDLK_F7:
			return K_F7;
			
		case SDLK_F8:
			return K_F8;
			
		case SDLK_F9:
			return K_F9;
			
		case SDLK_F10:
			return K_F10;
			
		case SDLK_F11:
			return K_F11;
			
		case SDLK_F12:
			return K_F12;
			// K_INVERTED_EXCLAMATION;
			
		case SDLK_F13:
			return K_F13;
			
		case SDLK_F14:
			return K_F14;
			
		case SDLK_F15:
			return K_F15;
			
		case SDLK_KP_7:
			return K_KP_7;
			
		case SDLK_KP_8:
			return K_KP_8;
			
		case SDLK_KP_9:
			return K_KP_9;
			
		case SDLK_KP_4:
			return K_KP_4;
			
		case SDLK_KP_5:
			return K_KP_5;
			
		case SDLK_KP_6:
			return K_KP_6;
			
		case SDLK_KP_1:
			return K_KP_1;
			
		case SDLK_KP_2:
			return K_KP_2;
			
		case SDLK_KP_3:
			return K_KP_3;
			
		case SDLK_KP_ENTER:
			return K_KP_ENTER;
			
		case SDLK_KP_0:
			return K_KP_0;
			
		case SDLK_KP_PERIOD:
			return K_KP_DOT;
			
		case SDLK_KP_DIVIDE:
			return K_KP_SLASH;
			// K_SUPERSCRIPT_TWO;
			
		case SDLK_KP_MINUS:
			return K_KP_MINUS;
			// K_ACUTE_ACCENT;
			
		case SDLK_KP_PLUS:
			return K_KP_PLUS;
			
		case SDLK_NUMLOCKCLEAR:
			return K_NUMLOCK;
			
		case SDLK_KP_MULTIPLY:
			return K_KP_STAR;
			
		case SDLK_KP_EQUALS:
			return K_KP_EQUALS;
			
			// K_MASCULINE_ORDINATOR;
			// K_GRAVE_A;
			// K_AUX1;
			// K_CEDILLA_C;
			// K_GRAVE_E;
			// K_AUX2;
			// K_AUX3;
			// K_AUX4;
			// K_GRAVE_I;
			// K_AUX5;
			// K_AUX6;
			// K_AUX7;
			// K_AUX8;
			// K_TILDE_N;
			// K_GRAVE_O;
			// K_AUX9;
			// K_AUX10;
			// K_AUX11;
			// K_AUX12;
			// K_AUX13;
			// K_AUX14;
			// K_GRAVE_U;
			// K_AUX15;
			// K_AUX16;
			
		case SDLK_PRINTSCREEN:
			return K_PRINTSCREEN;
			
		case SDLK_MODE:
			return K_RALT;
	}
	
	return 0;
}
// RB end

static void PushConsoleEvent( const char* s )
{
	char* b;
	size_t len;
	
	len = strlen( s ) + 1;
	b = ( char* )Mem_Alloc( len, TAG_EVENTS );
	strcpy( b, s );
	
	SDL_Event event;
	
	event.type = SDL_USEREVENT;
	event.user.code = SE_CONSOLE;
	event.user.data1 = ( void* )len;
	event.user.data2 = b;
	
	SDL_PushEvent( &event );
}

/*
=================
Sys_InitInput
=================
*/
void Sys_InitInput()
{
	kbd_polls.SetGranularity( 64 );
	mouse_polls.SetGranularity( 64 );
	
#if !SDL_VERSION_ATLEAST(2, 0, 0)
	SDL_EnableUNICODE( 1 );
	SDL_EnableKeyRepeat( SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL );
#endif
	
	in_kbd.SetModified();
}

/*
=================
Sys_ShutdownInput
=================
*/
void Sys_ShutdownInput()
{
	kbd_polls.Clear();
	mouse_polls.Clear();
}

/*
===========
Sys_InitScanTable
===========
*/
// Windows has its own version due to the tools
#ifndef _WIN32
void Sys_InitScanTable()
{
}
#endif

/*
===============
Sys_GetConsoleKey
===============
*/
unsigned char Sys_GetConsoleKey( bool shifted )
{
	static unsigned char keys[2] = { '`', '~' };
	
	if( in_kbd.IsModified() )
	{
		idStr lang = in_kbd.GetString();
		
		if( lang.Length() )
		{
			if( !lang.Icmp( "french" ) )
			{
				keys[0] = '<';
				keys[1] = '>';
			}
			else if( !lang.Icmp( "german" ) )
			{
				keys[0] = '^';
				keys[1] = 176; // °
			}
			else if( !lang.Icmp( "italian" ) )
			{
				keys[0] = '\\';
				keys[1] = '|';
			}
			else if( !lang.Icmp( "spanish" ) )
			{
				keys[0] = 186; // º
				keys[1] = 170; // ª
			}
			else if( !lang.Icmp( "turkish" ) )
			{
				keys[0] = '"';
				keys[1] = 233; // é
			}
			else if( !lang.Icmp( "norwegian" ) )
			{
				keys[0] = 124; // |
				keys[1] = 167; // §
			}
		}
		
		in_kbd.ClearModified();
	}
	
	return shifted ? keys[1] : keys[0];
}

/*
===============
Sys_MapCharForKey
===============
*/
unsigned char Sys_MapCharForKey( int key )
{
	return key & 0xff;
}

/*
===============
Sys_GrabMouseCursor
===============
*/
void Sys_GrabMouseCursor( bool grabIt )
{
	int flags;
	
	if( grabIt )
	{
		// DG: disabling the cursor is now done once in GLimp_Init() because it should always be disabled
		flags = GRAB_ENABLE | GRAB_SETSTATE;
		// DG end
	}
	else
	{
		flags = GRAB_SETSTATE;
	}
	
	GLimp_GrabInput( flags );
}

/*
================
Sys_GetEvent
================
*/
sysEvent_t Sys_GetEvent()
{
	SDL_Event ev;
	sysEvent_t res = { };
	int key;
	
	static const sysEvent_t res_none = { SE_NONE, 0, 0, 0, NULL };
	
#if SDL_VERSION_ATLEAST(2, 0, 0)
	static char* s = NULL;
	static size_t s_pos = 0;
	
	if( s )
	{
		res.evType = SE_CHAR;
		res.evValue = s[s_pos];
		
		s_pos++;
		if( !s[s_pos] )
		{
			free( s );
			s = NULL;
			s_pos = 0;
		}
		
		return res;
	}
	
	// DG: fake a "mousewheel not pressed anymore" event for SDL2
	// so scrolling in menus stops after one step
	static int mwheelRel = 0;
	if( mwheelRel )
	{
		res.evType = SE_KEY;
		res.evValue = mwheelRel;
		res.evValue2 = 0; // "not pressed anymore"
		mwheelRel = 0;
		return res;
	}
	// DG end
#endif
	
	static byte c = 0;
	
	if( c )
	{
		res.evType = SE_CHAR;
		res.evValue = c;
		
		c = 0;
		
		return res;
	}
	
	if( SDL_PollEvent( &ev ) )
	{
		switch( ev.type )
		{
#if SDL_VERSION_ATLEAST(2, 0, 0)
			case SDL_WINDOWEVENT:
				switch( ev.window.event )
				{
					case SDL_WINDOWEVENT_FOCUS_GAINED:
					{
						// unset modifier, in case alt-tab was used to leave window and ALT is still set
						// as that can cause fullscreen-toggling when pressing enter...
						SDL_Keymod currentmod = SDL_GetModState();
						int newmod = KMOD_NONE;
						if( currentmod & KMOD_CAPS ) // preserve capslock
							newmod |= KMOD_CAPS;
							
						SDL_SetModState( ( SDL_Keymod )newmod );
						
						// DG: un-pause the game when focus is gained, that also re-grabs the input
						//     disabling the cursor is now done once in GLimp_Init() because it should always be disabled
						cvarSystem->SetCVarBool( "com_pause", false );
						// DG end
						break;
					}
					
					case SDL_WINDOWEVENT_FOCUS_LOST:
						// DG: pause the game when focus is lost, that also un-grabs the input
						cvarSystem->SetCVarBool( "com_pause", true );
						// DG end
						break;
						
						// DG: handle resizing and moving of window
					case SDL_WINDOWEVENT_RESIZED:
					{
						int w = ev.window.data1;
						int h = ev.window.data2;
						r_windowWidth.SetInteger( w );
						r_windowHeight.SetInteger( h );
						
						glConfig.nativeScreenWidth = w;
						glConfig.nativeScreenHeight = h;
						break;
					}
					
					case SDL_WINDOWEVENT_MOVED:
					{
						int x = ev.window.data1;
						int y = ev.window.data2;
						r_windowX.SetInteger( x );
						r_windowY.SetInteger( y );
						break;
					}
					// DG end
				}
				
				return res_none;
#else
			case SDL_ACTIVEEVENT:
			{
				// DG: (un-)pause the game when focus is gained, that also (un-)grabs the input
				bool pause = true;
			
				if( ev.active.gain )
				{
			
					pause = false;
			
					// unset modifier, in case alt-tab was used to leave window and ALT is still set
					// as that can cause fullscreen-toggling when pressing enter...
					SDLMod currentmod = SDL_GetModState();
					int newmod = KMOD_NONE;
					if( currentmod & KMOD_CAPS ) // preserve capslock
						newmod |= KMOD_CAPS;
			
					SDL_SetModState( ( SDLMod )newmod );
				}
			
				cvarSystem->SetCVarBool( "com_pause", pause );
			}
			
			return res_none;
			
			case SDL_VIDEOEXPOSE:
				return res_none;
				
				// DG: handle resizing and moving of window
			case SDL_VIDEORESIZE:
			{
				int w = ev.resize.w;
				int h = ev.resize.h;
				r_windowWidth.SetInteger( w );
				r_windowHeight.SetInteger( h );
			
				glConfig.nativeScreenWidth = w;
				glConfig.nativeScreenHeight = h;
				// for some reason this needs a vid_restart in SDL1 but not SDL2 so GLimp_SetScreenParms() is called
				PushConsoleEvent( "vid_restart" );
				return res_none;
			}
			// DG end
#endif
			
			case SDL_KEYDOWN:
				if( ev.key.keysym.sym == SDLK_RETURN && ( ev.key.keysym.mod & KMOD_ALT ) > 0 )
				{
					// DG: go to fullscreen on current display, instead of always first display
					int fullscreen = 0;
					if( ! renderSystem->IsFullScreen() )
					{
						// this will be handled as "fullscreen on current window"
						// r_fullscreen 1 means "fullscreen on first window" in d3 bfg
						fullscreen = -2;
					}
					cvarSystem->SetCVarInteger( "r_fullscreen", fullscreen );
					// DG end
					PushConsoleEvent( "vid_restart" );
					return res_none;
				}
				
				// DG: ctrl-g to un-grab mouse - yeah, left ctrl shoots, then just use right ctrl :)
				if( ev.key.keysym.sym == SDLK_g && ( ev.key.keysym.mod & KMOD_CTRL ) > 0 )
				{
					bool grab = cvarSystem->GetCVarBool( "in_nograb" );
					grab = !grab;
					cvarSystem->SetCVarBool( "in_nograb", grab );
					return res_none;
				}
				// DG end
				
				// fall through
			case SDL_KEYUP:
			{
				bool isChar;
				
				// DG: special case for SDL_SCANCODE_GRAVE - the console key under Esc
				if( ev.key.keysym.scancode == SDL_SCANCODE_GRAVE )
				{
					key = K_GRAVE;
					c = K_BACKSPACE; // bad hack to get empty console inputline..
				} // DG end, the original code is in the else case
				else
				{
					key = SDL_KeyToDoom3Key( ev.key.keysym.sym, isChar );
					
					if( key == 0 )
					{
						unsigned char c;
						
						// check if its an unmapped console key
						if( ev.key.keysym.unicode == ( c = Sys_GetConsoleKey( false ) ) )
						{
							key = c;
						}
						else if( ev.key.keysym.unicode == ( c = Sys_GetConsoleKey( true ) ) )
						{
							key = c;
						}
						else
						{
							if( ev.type == SDL_KEYDOWN )
								common->Warning( "unmapped SDL key %d (0x%x) scancode %d", ev.key.keysym.sym, ev.key.keysym.unicode, ev.key.keysym.scancode );
							return res_none;
						}
					}
				}
				
				res.evType = SE_KEY;
				res.evValue = key;
				res.evValue2 = ev.key.state == SDL_PRESSED ? 1 : 0;
				
				kbd_polls.Append( kbd_poll_t( key, ev.key.state == SDL_PRESSED ) );
				
				if( key == K_BACKSPACE && ev.key.state == SDL_PRESSED )
					c = key;
#if ! SDL_VERSION_ATLEAST(2, 0, 0)
				if( ev.key.state == SDL_PRESSED && isChar && ( ev.key.keysym.unicode & 0xff00 ) == 0 )
				{
					c = ev.key.keysym.unicode & 0xff;
				}
#endif
				
				return res;
			}
			
#if SDL_VERSION_ATLEAST(2, 0, 0)
			case SDL_TEXTINPUT:
				if( ev.text.text && *ev.text.text )
				{
					if( !ev.text.text[1] )
						c = *ev.text.text;
					else
						s = strdup( ev.text.text );
				}
				
				return res_none;
#endif
				
			case SDL_MOUSEMOTION:
				// DG: return event with absolute mouse-coordinates when in menu
				// to fix cursor problems in windowed mode
				if( game && game->Shell_IsActive() )
				{
					res.evType = SE_MOUSE_ABSOLUTE;
					res.evValue = ev.motion.x;
					res.evValue2 = ev.motion.y;
				}
				else     // this is the old, default behavior
				{
					res.evType = SE_MOUSE;
					res.evValue = ev.motion.xrel;
					res.evValue2 = ev.motion.yrel;
				}
				// DG end
				
				mouse_polls.Append( mouse_poll_t( M_DELTAX, ev.motion.xrel ) );
				mouse_polls.Append( mouse_poll_t( M_DELTAY, ev.motion.yrel ) );
				
				return res;
				
#if SDL_VERSION_ATLEAST(2, 0, 0)
			case SDL_MOUSEWHEEL:
				res.evType = SE_KEY;
				
				if( ev.wheel.y > 0 )
				{
					res.evValue = K_MWHEELUP;
					mouse_polls.Append( mouse_poll_t( M_DELTAZ, 1 ) );
				}
				else
				{
					res.evValue = K_MWHEELDOWN;
					mouse_polls.Append( mouse_poll_t( M_DELTAZ, -1 ) );
				}
				
				// DG: remember mousewheel direction to issue a "not pressed anymore" event
				mwheelRel = res.evValue;
				// DG end
				
				res.evValue2 = 1;
				
				return res;
#endif
				
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
				res.evType = SE_KEY;
				
				switch( ev.button.button )
				{
					case SDL_BUTTON_LEFT:
						res.evValue = K_MOUSE1;
						mouse_polls.Append( mouse_poll_t( M_ACTION1, ev.button.state == SDL_PRESSED ? 1 : 0 ) );
						break;
					case SDL_BUTTON_MIDDLE:
						res.evValue = K_MOUSE3;
						mouse_polls.Append( mouse_poll_t( M_ACTION3, ev.button.state == SDL_PRESSED ? 1 : 0 ) );
						break;
					case SDL_BUTTON_RIGHT:
						res.evValue = K_MOUSE2;
						mouse_polls.Append( mouse_poll_t( M_ACTION2, ev.button.state == SDL_PRESSED ? 1 : 0 ) );
						break;
						
#if !SDL_VERSION_ATLEAST(2, 0, 0)
					case SDL_BUTTON_WHEELUP:
						res.evValue = K_MWHEELUP;
						if( ev.button.state == SDL_PRESSED )
							mouse_polls.Append( mouse_poll_t( M_DELTAZ, 1 ) );
						break;
					case SDL_BUTTON_WHEELDOWN:
						res.evValue = K_MWHEELDOWN;
						if( ev.button.state == SDL_PRESSED )
							mouse_polls.Append( mouse_poll_t( M_DELTAZ, -1 ) );
						break;
#endif
				}
				
				res.evValue2 = ev.button.state == SDL_PRESSED ? 1 : 0;
				
				return res;
				
			case SDL_QUIT:
				PushConsoleEvent( "quit" );
				return res_none;
				
			case SDL_USEREVENT:
				switch( ev.user.code )
				{
					case SE_CONSOLE:
						res.evType = SE_CONSOLE;
						res.evPtrLength = ( intptr_t )ev.user.data1;
						res.evPtr = ev.user.data2;
						return res;
					default:
						common->Warning( "unknown user event %u", ev.user.code );
						return res_none;
				}
			default:
				common->Warning( "unknown event %u", ev.type );
				return res_none;
		}
	}
	
	return res_none;
}

/*
================
Sys_ClearEvents
================
*/
void Sys_ClearEvents()
{
	SDL_Event ev;
	
	while( SDL_PollEvent( &ev ) )
		;
		
	kbd_polls.SetNum( 0 );
	mouse_polls.SetNum( 0 );
}

/*
================
Sys_GenerateEvents
================
*/
void Sys_GenerateEvents()
{
	char* s = Posix_ConsoleInput();
	
	if( s )
		PushConsoleEvent( s );
		
	SDL_PumpEvents();
}

/*
================
Sys_PollKeyboardInputEvents
================
*/
int Sys_PollKeyboardInputEvents()
{
	return kbd_polls.Num();
}

/*
================
Sys_ReturnKeyboardInputEvent
================
*/
int Sys_ReturnKeyboardInputEvent( const int n, int& key, bool& state )
{
	if( n >= kbd_polls.Num() )
		return 0;
		
	key = kbd_polls[n].key;
	state = kbd_polls[n].state;
	return 1;
}

/*
================
Sys_EndKeyboardInputEvents
================
*/
void Sys_EndKeyboardInputEvents()
{
	kbd_polls.SetNum( 0 );
}

/*
================
Sys_PollMouseInputEvents
================
*/
int Sys_PollMouseInputEvents( int mouseEvents[MAX_MOUSE_EVENTS][2] )
{
	int numEvents = mouse_polls.Num();
	
	if( numEvents > MAX_MOUSE_EVENTS )
	{
		numEvents = MAX_MOUSE_EVENTS;
	}
	
	for( int i = 0; i < numEvents; i++ )
	{
		const mouse_poll_t& mp = mouse_polls[i];
		
		mouseEvents[i][0] = mp.action;
		mouseEvents[i][1] = mp.value;
	}
	
	mouse_polls.SetNum( 0 );
	
	return numEvents;
}


//=====================================================================================
//	Joystick Input Handling
//=====================================================================================

void Sys_SetRumble( int device, int low, int hi )
{
	// TODO;
}

int Sys_PollJoystickInputEvents( int deviceNum )
{
	// TODO;
	return 0;
}


int Sys_ReturnJoystickInputEvent( const int n, int& action, int& value )
{
	// TODO;
	return 0;
}


void Sys_EndJoystickInputEvents()
{
}



