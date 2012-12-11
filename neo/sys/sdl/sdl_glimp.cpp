/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.
Copyright (C) 2012 dhewg (dhewm3)
Copyright (C) 2012 Robert Beckebans

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
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include <SDL.h>
#include <SDL_syswm.h>

#include "../../idlib/precompiled.h"

#include "renderer/tr_local.h"
#include "sdl_local.h"

idCVar in_nograb( "in_nograb", "0", CVAR_SYSTEM | CVAR_NOCHEAT, "prevents input grabbing" );
idCVar r_waylandcompat( "r_waylandcompat", "0", CVAR_SYSTEM | CVAR_NOCHEAT | CVAR_ARCHIVE, "wayland compatible framebuffer" );

static bool grabbed = false;

#if SDL_VERSION_ATLEAST(2, 0, 0)
static SDL_Window* window = NULL;
static SDL_GLContext context = NULL;
#else
static SDL_Surface* window = NULL;
#define SDL_WINDOW_OPENGL SDL_OPENGL
#define SDL_WINDOW_FULLSCREEN SDL_FULLSCREEN
#endif

bool QGL_Init( const char* dllname );
void QGL_Shutdown();

/*
===================
GLimp_Init
===================
*/
bool GLimp_Init( glimpParms_t parms )
{
	common->Printf( "Initializing OpenGL subsystem\n" );
	
	if( !SDL_WasInit( SDL_INIT_VIDEO ) )
	{
		if( SDL_Init( SDL_INIT_VIDEO ) )
			common->Error( "Error while initializing SDL: %s", SDL_GetError() );
	}
	
	Uint32 flags = SDL_WINDOW_OPENGL;
	
	if( parms.fullScreen )
		flags |= SDL_WINDOW_FULLSCREEN;
		
	int colorbits = 24;
	int depthbits = 24;
	int stencilbits = 8;
	
	for( int i = 0; i < 16; i++ )
	{
		// 0 - default
		// 1 - minus colorbits
		// 2 - minus depthbits
		// 3 - minus stencil
		if( ( i % 4 ) == 0 && i )
		{
			// one pass, reduce
			switch( i / 4 )
			{
				case 2 :
					if( colorbits == 24 )
						colorbits = 16;
					break;
				case 1 :
					if( depthbits == 24 )
						depthbits = 16;
					else if( depthbits == 16 )
						depthbits = 8;
				case 3 :
					if( stencilbits == 24 )
						stencilbits = 16;
					else if( stencilbits == 16 )
						stencilbits = 8;
			}
		}
		
		int tcolorbits = colorbits;
		int tdepthbits = depthbits;
		int tstencilbits = stencilbits;
		
		if( ( i % 4 ) == 3 )
		{
			// reduce colorbits
			if( tcolorbits == 24 )
				tcolorbits = 16;
		}
		
		if( ( i % 4 ) == 2 )
		{
			// reduce depthbits
			if( tdepthbits == 24 )
				tdepthbits = 16;
			else if( tdepthbits == 16 )
				tdepthbits = 8;
		}
		
		if( ( i % 4 ) == 1 )
		{
			// reduce stencilbits
			if( tstencilbits == 24 )
				tstencilbits = 16;
			else if( tstencilbits == 16 )
				tstencilbits = 8;
			else
				tstencilbits = 0;
		}
		
		int channelcolorbits = 4;
		if( tcolorbits == 24 )
			channelcolorbits = 8;
			
#if 1
		SDL_GL_SetAttribute( SDL_GL_RED_SIZE, channelcolorbits );
		SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, channelcolorbits );
		SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, channelcolorbits );
		SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
		SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, tdepthbits );
		SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, tstencilbits );
		
		if( r_waylandcompat.GetBool() )
			SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, 0 );
		else
			SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, channelcolorbits );
			
		SDL_GL_SetAttribute( SDL_GL_STEREO, parms.stereo ? 1 : 0 );
		
		SDL_GL_SetAttribute( SDL_GL_MULTISAMPLEBUFFERS, parms.multiSamples ? 1 : 0 );
		SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES, parms.multiSamples );
#endif
		
#if SDL_VERSION_ATLEAST(2, 0, 0)
		window = SDL_CreateWindow( GAME_NAME,
								   SDL_WINDOWPOS_UNDEFINED,
								   SDL_WINDOWPOS_UNDEFINED,
								   parms.width, parms.height, flags );
		context = SDL_GL_CreateContext( window );
		
		if( !window )
		{
			common->DPrintf( "Couldn't set GL mode %d/%d/%d: %s",
							 channelcolorbits, tdepthbits, tstencilbits, SDL_GetError() );
			continue;
		}
		
		if( SDL_GL_SetSwapInterval( r_swapInterval.GetInteger() ) < 0 )
			common->Warning( "SDL_GL_SWAP_CONTROL not supported" );
			
		// RB begin
		SDL_GetWindowSize( window, &glConfig.nativeScreenWidth, &glConfig.nativeScreenHeight );
		// RB end
		
		glConfig.isFullscreen = ( SDL_GetWindowFlags( window ) & SDL_WINDOW_FULLSCREEN ) == SDL_WINDOW_FULLSCREEN;
#else
		SDL_WM_SetCaption( GAME_NAME, GAME_NAME );
		
		if( SDL_GL_SetAttribute( SDL_GL_SWAP_CONTROL, r_swapInterval.GetInteger() ) < 0 )
			common->Warning( "SDL_GL_SWAP_CONTROL not supported" );
		
		window = SDL_SetVideoMode( parms.width, parms.height, colorbits, flags );
		if( !window )
		{
			common->DPrintf( "Couldn't set GL mode %d/%d/%d: %s",
							 channelcolorbits, tdepthbits, tstencilbits, SDL_GetError() );
			continue;
		}
		
		glConfig.nativeScreenWidth = window->w;
		glConfig.nativeScreenHeight = window->h;
		
		glConfig.isFullscreen = ( window->flags & SDL_FULLSCREEN ) == SDL_FULLSCREEN;
#endif
		
		common->Printf( "Using %d color bits, %d depth, %d stencil display\n",
						channelcolorbits, tdepthbits, tstencilbits );
						
		glConfig.colorBits = tcolorbits;
		glConfig.depthBits = tdepthbits;
		glConfig.stencilBits = tstencilbits;
		
		glConfig.displayFrequency = 0;
		
		// RB begin
		glConfig.isStereoPixelFormat = parms.stereo;
		glConfig.multisamples = parms.multiSamples;
		// RB end
		
		break;
	}
	
	if( !window )
	{
		common->Warning( "No usable GL mode found: %s", SDL_GetError() );
		return false;
	}
	
	QGL_Init( "nodriverlib" );
	
	return true;
}

/*
===================
GLimp_SetScreenParms
===================
*/
bool GLimp_SetScreenParms( glimpParms_t parms )
{
	common->DPrintf( "TODO: GLimp_ActivateContext\n" );
	return true;
}

/*
===================
GLimp_Shutdown
===================
*/
void GLimp_Shutdown()
{
	common->Printf( "Shutting down OpenGL subsystem\n" );
	
#if SDL_VERSION_ATLEAST(2, 0, 0)
	if( context )
	{
		SDL_GL_DeleteContext( context );
		context = NULL;
	}
	
	if( window )
	{
		SDL_DestroyWindow( window );
		window = NULL;
	}
#endif
	
	QGL_Shutdown();
}

/*
===================
GLimp_SwapBuffers
===================
*/
void GLimp_SwapBuffers()
{
#if SDL_VERSION_ATLEAST(2, 0, 0)
	SDL_GL_SwapWindow( window );
#else
	SDL_GL_SwapBuffers();
#endif
}

/*
=================
GLimp_SetGamma
=================
*/
void GLimp_SetGamma( unsigned short red[256], unsigned short green[256], unsigned short blue[256] )
{
	if( !window )
	{
		common->Warning( "GLimp_SetGamma called without window" );
		return;
	}
	
#if SDL_VERSION_ATLEAST(2, 0, 0)
	if( SDL_SetWindowGammaRamp( window, red, green, blue ) )
#else
	if( SDL_SetGammaRamp( red, green, blue ) )
#endif
		common->Warning( "Couldn't set gamma ramp: %s", SDL_GetError() );
}

/*
===================
GLimp_ExtensionPointer
===================
*/
GLExtension_t GLimp_ExtensionPointer( const char* name )
{
	assert( SDL_WasInit( SDL_INIT_VIDEO ) );
	
	return ( GLExtension_t )SDL_GL_GetProcAddress( name );
}

void GLimp_GrabInput( int flags )
{
	bool grab = flags & GRAB_ENABLE;
	
	if( grab && ( flags & GRAB_REENABLE ) )
		grab = false;
		
	if( flags & GRAB_SETSTATE )
		grabbed = grab;
		
	if( in_nograb.GetBool() )
		grab = false;
		
	if( !window )
	{
		common->Warning( "GLimp_GrabInput called without window" );
		return;
	}
	
#if SDL_VERSION_ATLEAST(2, 0, 0)
	SDL_ShowCursor( flags & GRAB_HIDECURSOR ? SDL_DISABLE : SDL_ENABLE );
	SDL_SetRelativeMouseMode( flags & GRAB_HIDECURSOR ? SDL_TRUE : SDL_FALSE );
	SDL_SetWindowGrab( window, grab ? SDL_TRUE : SDL_FALSE );
#else
	SDL_ShowCursor( flags & GRAB_HIDECURSOR ? SDL_DISABLE : SDL_ENABLE );
	SDL_WM_GrabInput( grab ? SDL_GRAB_ON : SDL_GRAB_OFF );
#endif
}

/*
====================
DumpAllDisplayDevices
====================
*/
void DumpAllDisplayDevices()
{
	common->DPrintf( "TODO: DumpAllDisplayDevices\n" );
}



class idSort_VidMode : public idSort_Quick< vidMode_t, idSort_VidMode >
{
public:
	int Compare( const vidMode_t& a, const vidMode_t& b ) const
	{
		int wd = a.width - b.width;
		int hd = a.height - b.height;
		int fd = a.displayHz - b.displayHz;
		return ( hd != 0 ) ? hd : ( wd != 0 ) ? wd : fd;
	}
};

/*
====================
R_GetModeListForDisplay
====================
*/
bool R_GetModeListForDisplay( const int requestedDisplayNum, idList<vidMode_t>& modeList )
{
	modeList.Clear();
	
	bool	verbose = false;
	
	const SDL_VideoInfo* videoInfo = SDL_GetVideoInfo();
	
	SDL_Rect** modes = SDL_ListModes( videoInfo->vfmt, SDL_OPENGL | SDL_FULLSCREEN );
	
	if( !modes )
	{
		common->Warning( "Can't get list of available modes\n" );
		return false;
	}
	
	if( modes == ( SDL_Rect** ) - 1 )
	{
		common->Printf( "Display supports any resolution\n" );
		return false;					// can set any resolution
	}
	
	int numModes;
	for( numModes = 0; modes[numModes]; numModes++ );
	
	if( numModes > 1 )
	{
		for( int i = 0; i < numModes; i++ )
		{
			vidMode_t mode;
			mode.width =  modes[i]->w;
			mode.height =  modes[i]->h;
			mode.displayHz = 60; // FIXME;
			modeList.AddUnique( mode );
		}
		
		// sort with lowest resolution first
		modeList.SortWithTemplate( idSort_VidMode() );
		
		return true;
	}
	
	return false;
}
