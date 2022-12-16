/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.
Copyright (C) 2012 dhewg (dhewm3)
Copyright (C) 2012-2014 Robert Beckebans
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
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "../../idlib/precompiled.h"

// DG: SDL.h somehow needs the following functions, so #undef those silly
//     "don't use" #defines from Str.h
#undef strncmp
#undef strcasecmp
#undef vsnprintf
// DG end

#include <SDL.h>
#include <SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <vector>

#include "renderer/RenderCommon.h"
#include "sdl_local.h"

#if defined( USE_NVRHI )
	#include <sys/DeviceManager.h>
	extern DeviceManager* deviceManager;
#endif

idCVar in_nograb( "in_nograb", "0", CVAR_SYSTEM | CVAR_NOCHEAT, "prevents input grabbing" );

// RB: FIXME this shit. We need the OpenGL alpha channel for advanced rendering effects
idCVar r_waylandcompat( "r_waylandcompat", "0", CVAR_SYSTEM | CVAR_NOCHEAT | CVAR_ARCHIVE, "wayland compatible framebuffer" );

// RB: only relevant if using SDL 2.0
#if defined(__APPLE__)
	// only core profile is supported on OS X
	idCVar r_useOpenGL32( "r_useOpenGL32", "2", CVAR_INTEGER, "0 = OpenGL 3.x, 1 = OpenGL 3.2 compatibility profile, 2 = OpenGL 3.2 core profile", 0, 2 );
#elif defined(__linux__)
	// Linux open source drivers suck
	idCVar r_useOpenGL32( "r_useOpenGL32", "0", CVAR_INTEGER, "0 = OpenGL 3.x, 1 = OpenGL 3.2 compatibility profile, 2 = OpenGL 3.2 core profile", 0, 2 );
#else
	idCVar r_useOpenGL32( "r_useOpenGL32", "1", CVAR_INTEGER, "0 = OpenGL 3.x, 1 = OpenGL 3.2 compatibility profile, 2 = OpenGL 3.2 core profile", 0, 2 );
#endif
// RB end

static bool grabbed = false;

//vulkanContext_t vkcontext; // Eric: I added this to pass SDL_Window* window to the SDL_Vulkan_* methods that are used else were.

static SDL_Window* window = nullptr;

// Eric: Integrate this into RBDoom3BFG's source code ecosystem.
// Helper function for using SDL2 and Vulkan on Linux.
std::vector<const char*> get_required_extensions()
{
	uint32_t                 sdlCount = 0;
	std::vector<const char*> sdlInstanceExtensions = {};

	SDL_Vulkan_GetInstanceExtensions( window, &sdlCount, nullptr );
	sdlInstanceExtensions.resize( sdlCount );
	SDL_Vulkan_GetInstanceExtensions( window, &sdlCount, sdlInstanceExtensions.data() );

	return sdlInstanceExtensions;
}

#if defined( USE_NVRHI )
// SRS - Helper function for creating SDL Vulkan surface within DeviceManager_VK() when NVRHI enabled
vk::Result CreateSDLWindowSurface( vk::Instance instance, vk::SurfaceKHR* surface )
{
	if( !SDL_Vulkan_CreateSurface( window, ( VkInstance )instance, ( VkSurfaceKHR* )surface ) )
	{
		common->Warning( "Error while creating SDL Vulkan surface: %s", SDL_GetError() );
		return vk::Result::eErrorSurfaceLostKHR;
	}

	return vk::Result::eSuccess;
}

bool DeviceManager::CreateWindowDeviceAndSwapChain( const glimpParms_t& parms, const char* windowTitle )
{
	Uint32 flags = SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE;
	if( parms.fullScreen )
	{
		flags |= SDL_WINDOW_FULLSCREEN;
	}

	window = SDL_CreateWindow( GAME_NAME,
							   parms.x,
							   parms.y,
							   parms.width, parms.height, flags );

	if( !window )
	{
		common->Printf( "^3SDL_CreateWindow() - Couldn't create window^0\n" );
		return false;
	}

	// RB
	deviceParms.backBufferWidth = parms.width;
	deviceParms.backBufferHeight = parms.height;
	deviceParms.backBufferSampleCount = parms.multiSamples;
	deviceParms.vsyncEnabled = requestedVSync;

	if( !CreateDeviceAndSwapChain() )
	{
		return false;
	}

	return true;
}

void DeviceManager::UpdateWindowSize( const glimpParms_t& parms )
{
	windowVisible = true;

	if( int( deviceParms.backBufferWidth ) != parms.width ||
			int( deviceParms.backBufferHeight ) != parms.height ||
#if ID_MSAA
			int( deviceParms.backBufferSampleCount ) != parms.multiSamples ||
#endif
			( deviceParms.vsyncEnabled != requestedVSync && GetGraphicsAPI() == nvrhi::GraphicsAPI::VULKAN ) )
	{
		// window is not minimized, and the size has changed

		BackBufferResizing();

		deviceParms.backBufferWidth = parms.width;
		deviceParms.backBufferHeight = parms.height;
		deviceParms.backBufferSampleCount = parms.multiSamples;
		deviceParms.vsyncEnabled = requestedVSync;

		ResizeSwapChain();
		BackBufferResized();
	}
	else
	{
		deviceParms.vsyncEnabled = requestedVSync;
	}
}
#endif

/*
===================
VKimp_PreInit

 R_GetModeListForDisplay is called before VKimp_Init(), but SDL needs SDL_Init() first.
 So do that in VKimp_PreInit()
 Calling that function more than once doesn't make a difference
===================
*/
void VKimp_PreInit() // DG: added this function for SDL compatibility
{
	if( !SDL_WasInit( SDL_INIT_VIDEO ) )
	{
		if( SDL_Init( SDL_INIT_VIDEO ) )
		{
			common->Error( "Error while initializing SDL: %s", SDL_GetError() );
		}
	}
}

// SRS - Function to get display frequency of monitor hosting the current window
static int GetDisplayFrequency( glimpParms_t parms )
{
	SDL_DisplayMode m = {0};
	if( SDL_GetWindowDisplayMode( window, &m ) < 0 )
	{
		common->Warning( "Couldn't get display refresh rate, reason: %s", SDL_GetError() );
		return parms.displayHz;
	}

	return m.refresh_rate;
}

/* Eric: Is the majority of this function not needed since switching from GL to Vulkan?
===================
VKimp_Init
===================
*/
bool VKimp_Init( glimpParms_t parms )
{

	common->Printf( "Initializing Vulkan subsystem\n" );

	VKimp_PreInit(); // DG: make sure SDL is initialized

	// DG: make window resizable
	Uint32 flags = SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE;
	// DG end

	if( parms.fullScreen )
	{
		flags |= SDL_WINDOW_FULLSCREEN;
	}

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
					{
						colorbits = 16;
					}
					break;
				case 1 :
					if( depthbits == 24 )
					{
						depthbits = 16;
					}
					else if( depthbits == 16 )
					{
						depthbits = 8;
					}
				case 3 :
					if( stencilbits == 24 )
					{
						stencilbits = 16;
					}
					else if( stencilbits == 16 )
					{
						stencilbits = 8;
					}
			}
		}

		int tcolorbits = colorbits;
		int tdepthbits = depthbits;
		int tstencilbits = stencilbits;

		if( ( i % 4 ) == 3 )
		{
			// reduce colorbits
			if( tcolorbits == 24 )
			{
				tcolorbits = 16;
			}
		}

		if( ( i % 4 ) == 2 )
		{
			// reduce depthbits
			if( tdepthbits == 24 )
			{
				tdepthbits = 16;
			}
			else if( tdepthbits == 16 )
			{
				tdepthbits = 8;
			}
		}

		if( ( i % 4 ) == 1 )
		{
			// reduce stencilbits
			if( tstencilbits == 24 )
			{
				tstencilbits = 16;
			}
			else if( tstencilbits == 16 )
			{
				tstencilbits = 8;
			}
			else
			{
				tstencilbits = 0;
			}
		}

		int channelcolorbits = 4;
		if( tcolorbits == 24 )
		{
			channelcolorbits = 8;
		}

		// DG: set display num for fullscreen
		int windowPos = SDL_WINDOWPOS_UNDEFINED;
		if( parms.fullScreen > 0 )
		{
			if( parms.fullScreen > SDL_GetNumVideoDisplays() )
			{
				common->Warning( "Couldn't set display to num %i because we only have %i displays",
								 parms.fullScreen, SDL_GetNumVideoDisplays() );
			}
			else
			{
				// -1 because SDL starts counting displays at 0, while parms.fullScreen starts at 1
				windowPos = SDL_WINDOWPOS_UNDEFINED_DISPLAY( ( parms.fullScreen - 1 ) );
			}
		}
		// TODO: if parms.fullScreen == -1 there should be a borderless window spanning multiple displays
		/*
		 * NOTE that this implicitly handles parms.fullScreen == -2 (from r_fullscreen -2) meaning
		 * "do fullscreen, but I don't care on what monitor", at least on my box it's the monitor with
		 * the mouse cursor.
		 */

#if defined( USE_NVRHI )
		glimpParms_t createParms = parms;
		createParms.x = createParms.y = windowPos;

		if( !deviceManager->CreateWindowDeviceAndSwapChain( createParms, GAME_NAME ) )
#else
		window = SDL_CreateWindow( GAME_NAME,
								   windowPos,
								   windowPos,
								   parms.width, parms.height, flags );

		vkcontext.sdlWindow = window;

		if( !window )
#endif
		{
			common->DPrintf( "Couldn't set Vulkan mode %d/%d/%d: %s",
							 channelcolorbits, tdepthbits, tstencilbits, SDL_GetError() );
			continue;
		}

		// SRS - Make sure display is set to requested refresh rate from the start
		if( parms.displayHz > 0 && parms.displayHz != GetDisplayFrequency( parms ) )
		{
			SDL_DisplayMode m = {0};
			SDL_GetWindowDisplayMode( window, &m );

			m.refresh_rate = parms.displayHz;
			if( SDL_SetWindowDisplayMode( window, &m ) < 0 )
			{
				common->Warning( "Couldn't set display refresh rate to %i Hz", parms.displayHz );
			}
		}

		// RB begin
		SDL_GetWindowSize( window, &glConfig.nativeScreenWidth, &glConfig.nativeScreenHeight );
		// RB end

		glConfig.isFullscreen = ( SDL_GetWindowFlags( window ) & SDL_WINDOW_FULLSCREEN ) == SDL_WINDOW_FULLSCREEN;

#if !defined( USE_NVRHI )
		common->Printf( "Using %d color bits, %d depth, %d stencil display\n",
						channelcolorbits, tdepthbits, tstencilbits );

		glConfig.colorBits = tcolorbits;
		glConfig.depthBits = tdepthbits;
		glConfig.stencilBits = tstencilbits;
#endif

		// RB begin
		glConfig.displayFrequency = GetDisplayFrequency( parms );
		glConfig.isStereoPixelFormat = parms.stereo;
		glConfig.multisamples = parms.multiSamples;

		glConfig.pixelAspect = 1.0f;	// FIXME: some monitor modes may be distorted
		// should side-by-side stereo modes be consider aspect 0.5?

		// RB end

		break;
	}

	if( !window )
	{
		common->Printf( "No usable VK mode found: %s", SDL_GetError() );
		return false;
	}

#if defined(__APPLE__) && SDL_VERSION_ATLEAST(2, 0, 2)
	// SRS - On OSX enable SDL2 relative mouse mode warping to capture mouse properly if outside of window
	SDL_SetHintWithPriority( SDL_HINT_MOUSE_RELATIVE_MODE_WARP, "1", SDL_HINT_OVERRIDE );
#endif

	// DG: disable cursor, we have two cursors in menu (because mouse isn't grabbed in menu)
	SDL_ShowCursor( SDL_DISABLE );
	// DG end

	return true;
}

/*
===================
 Helper functions for VKimp_SetScreenParms()
===================
*/
static int ScreenParmsHandleDisplayIndex( glimpParms_t parms )
{
	int displayIdx;
	if( parms.fullScreen > 0 )
	{
		displayIdx = parms.fullScreen - 1; // first display for SDL is 0, in parms it's 1
	}
	else // -2 == use current display
	{
		displayIdx = SDL_GetWindowDisplayIndex( window );
		if( displayIdx < 0 ) // for some reason the display for the window couldn't be detected
		{
			displayIdx = 0;
		}
	}

	if( parms.fullScreen > SDL_GetNumVideoDisplays() )
	{
		common->Warning( "Can't set fullscreen mode to display number %i, because SDL2 only knows about %i displays!",
						 parms.fullScreen, SDL_GetNumVideoDisplays() );
		return -1;
	}

	if( parms.fullScreen != glConfig.isFullscreen )
	{
		// we have to switch to another display
		if( glConfig.isFullscreen )
		{
			// if we're already in fullscreen mode but want to switch to another monitor
			// we have to go to windowed mode first to move the window.. SDL-oddity.
			SDL_SetWindowFullscreen( window, SDL_FALSE );
		}
		// select display ; SDL_WINDOWPOS_UNDEFINED_DISPLAY() doesn't work.
		int x = SDL_WINDOWPOS_CENTERED_DISPLAY( displayIdx );
		// move window to the center of selected display
		SDL_SetWindowPosition( window, x, x );
	}
	return displayIdx;
}

static bool SetScreenParmsFullscreen( glimpParms_t parms )
{
	SDL_DisplayMode m = {0};
	int displayIdx = ScreenParmsHandleDisplayIndex( parms );
	if( displayIdx < 0 )
	{
		return false;
	}

	// get current mode of display the window should be full-screened on
	SDL_GetCurrentDisplayMode( displayIdx, &m );

	// change settings in that display mode according to parms
	// FIXME: check if refreshrate, width and height are supported?
	m.refresh_rate = parms.displayHz;
	m.w = parms.width;
	m.h = parms.height;

	// set that displaymode
	if( SDL_SetWindowDisplayMode( window, &m ) < 0 )
	{
		common->Warning( "Couldn't set window mode for fullscreen, reason: %s", SDL_GetError() );
		return false;
	}

	// if we're currently not in fullscreen mode, we need to switch to fullscreen
	if( !( SDL_GetWindowFlags( window ) & SDL_WINDOW_FULLSCREEN ) )
	{
		if( SDL_SetWindowFullscreen( window, SDL_TRUE ) < 0 )
		{
			common->Warning( "Couldn't switch to fullscreen mode, reason: %s!", SDL_GetError() );
			return false;
		}
	}
	return true;
}

static bool SetScreenParmsWindowed( glimpParms_t parms )
{
	// SRS - handle differences in WM behaviour: for macOS set position first, for linux set it last
#if defined(__APPLE__)
	SDL_SetWindowPosition( window, parms.x, parms.y );
#endif

	// if we're currently in fullscreen mode, we need to disable that
	if( SDL_GetWindowFlags( window ) & SDL_WINDOW_FULLSCREEN )
	{
		if( SDL_SetWindowFullscreen( window, SDL_FALSE ) < 0 )
		{
			common->Warning( "Couldn't switch to windowed mode, reason: %s!", SDL_GetError() );
			return false;
		}
	}

	SDL_SetWindowSize( window, parms.width, parms.height );

	// SRS - this logic prevents window position drift on linux when coming in and out of fullscreen
#if !defined(__APPLE__)
	SDL_bool borderState = SDL_GetWindowFlags( window ) & SDL_WINDOW_BORDERLESS ? SDL_FALSE : SDL_TRUE;
	SDL_SetWindowBordered( window, SDL_FALSE );
	SDL_SetWindowPosition( window, parms.x, parms.y );
	SDL_SetWindowBordered( window, borderState );
#endif

	return true;
}


/*
===================
VKimp_SetScreenParms
===================
*/
bool VKimp_SetScreenParms( glimpParms_t parms )
{

	if( parms.fullScreen > 0 || parms.fullScreen == -2 )
	{
		if( !SetScreenParmsFullscreen( parms ) )
		{
			return false;
		}
	}
	else if( parms.fullScreen == 0 ) // windowed mode
	{
		if( !SetScreenParmsWindowed( parms ) )
		{
			return false;
		}
	}
	else
	{
		common->Warning( "VKimp_SetScreenParms: fullScreen -1 (borderless window for multiple displays) currently unsupported!" );
		return false;
	}

	glConfig.isFullscreen = parms.fullScreen;
	glConfig.isStereoPixelFormat = parms.stereo;
	glConfig.nativeScreenWidth = parms.width;
	glConfig.nativeScreenHeight = parms.height;
	glConfig.displayFrequency = GetDisplayFrequency( parms );
	glConfig.multisamples = parms.multiSamples;

	return true;
}

#if defined( USE_NVRHI )
void DeviceManager::Shutdown()
{
	DestroyDeviceAndSwapChain();
}
#endif

/*
===================
VKimp_Shutdown
===================
*/
void VKimp_Shutdown()
{
	common->Printf( "Shutting down Vulkan subsystem\n" );

#if defined( USE_NVRHI )
	if( deviceManager )
	{
		deviceManager->Shutdown();
	}
#endif

	if( window )
	{
		SDL_DestroyWindow( window );
		window = nullptr;
	}

}

/* Eric: Is this needed/used for Vulkan?
=================
VKimp_SetGamma
=================
*/
void VKimp_SetGamma( unsigned short red[256], unsigned short green[256], unsigned short blue[256] )
{
#ifndef USE_VULKAN
	if( !window )
	{
		common->Warning( "VKimp_SetGamma called without window" );
		return;
	}

#if SDL_VERSION_ATLEAST(2, 0, 0)
	if( SDL_SetWindowGammaRamp( window, red, green, blue ) )
#else
	if( SDL_SetGammaRamp( red, green, blue ) )
#endif
		common->Warning( "Couldn't set gamma ramp: %s", SDL_GetError() );
#endif
}

/*
===================
VKimp_ExtensionPointer
===================
*/
/*
GLExtension_t VKimp_ExtensionPointer(const char *name) {
	assert(SDL_WasInit(SDL_INIT_VIDEO));

	return (GLExtension_t)SDL_GL_GetProcAddress(name);
}
*/

void VKimp_GrabInput( int flags )
{
	bool grab = flags & GRAB_ENABLE;

	if( grab && ( flags & GRAB_REENABLE ) )
	{
		grab = false;
	}

	if( flags & GRAB_SETSTATE )
	{
		grabbed = grab;
	}

	if( in_nograb.GetBool() )
	{
		grab = false;
	}

	if( !window )
	{
		common->Warning( "VKimp_GrabInput called without window" );
		return;
	}

	// DG: disabling the cursor is now done once in VKimp_Init() because it should always be disabled

	// DG: check for GRAB_ENABLE instead of GRAB_HIDECURSOR because we always wanna hide it
	SDL_SetRelativeMouseMode( flags & GRAB_ENABLE ? SDL_TRUE : SDL_FALSE );
	SDL_SetWindowGrab( window, grab ? SDL_TRUE : SDL_FALSE );

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

// RB: resolutions supported by XreaL
static void FillStaticVidModes( idList<vidMode_t>& modeList )
{
	modeList.AddUnique( vidMode_t( 320,   240, 60 ) );
	modeList.AddUnique( vidMode_t( 400,   300, 60 ) );
	modeList.AddUnique( vidMode_t( 512,   384, 60 ) );
	modeList.AddUnique( vidMode_t( 640,   480, 60 ) );
	modeList.AddUnique( vidMode_t( 800,   600, 60 ) );
	modeList.AddUnique( vidMode_t( 960,   720, 60 ) );
	modeList.AddUnique( vidMode_t( 1024,  768, 60 ) );
	modeList.AddUnique( vidMode_t( 1152,  864, 60 ) );
	modeList.AddUnique( vidMode_t( 1280,  720, 60 ) );
	modeList.AddUnique( vidMode_t( 1280,  768, 60 ) );
	modeList.AddUnique( vidMode_t( 1280,  800, 60 ) );
	modeList.AddUnique( vidMode_t( 1280, 1024, 60 ) );
	modeList.AddUnique( vidMode_t( 1360,  768, 60 ) );
	modeList.AddUnique( vidMode_t( 1440,  900, 60 ) );
	modeList.AddUnique( vidMode_t( 1680, 1050, 60 ) );
	modeList.AddUnique( vidMode_t( 1600, 1200, 60 ) );
	modeList.AddUnique( vidMode_t( 1920, 1080, 60 ) );
	modeList.AddUnique( vidMode_t( 1920, 1200, 60 ) );
	modeList.AddUnique( vidMode_t( 2048, 1536, 60 ) );
	modeList.AddUnique( vidMode_t( 2560, 1600, 60 ) );

	modeList.SortWithTemplate( idSort_VidMode() );
}

/*
====================
R_GetModeListForDisplay
====================
*/
bool R_GetModeListForDisplay( const int requestedDisplayNum, idList<vidMode_t>& modeList )
{
	assert( requestedDisplayNum >= 0 );

	modeList.Clear();

	// DG: SDL2 implementation
	if( requestedDisplayNum >= SDL_GetNumVideoDisplays() )
	{
		// requested invalid displaynum
		return false;
	}

	int numModes = SDL_GetNumDisplayModes( requestedDisplayNum );
	if( numModes > 0 )
	{
		for( int i = 0; i < numModes; i++ )
		{
			SDL_DisplayMode m;
			int ret = SDL_GetDisplayMode( requestedDisplayNum, i, &m );
			if( ret != 0 )
			{
				common->Warning( "Can't get video mode no %i, because of %s\n", i, SDL_GetError() );
				continue;
			}

			vidMode_t mode;
			mode.width = m.w;
			mode.height = m.h;
			mode.displayHz = m.refresh_rate ? m.refresh_rate : 60; // default to 60 if unknown (0)
			modeList.AddUnique( mode );
		}

		if( modeList.Num() < 1 )
		{
			common->Warning( "Couldn't get a single video mode for display %i, using default ones..!\n", requestedDisplayNum );
			FillStaticVidModes( modeList );
		}

		// sort with lowest resolution first
		modeList.SortWithTemplate( idSort_VidMode() );
	}
	else
	{
		common->Warning( "Can't get Video Info, using default modes...\n" );
		if( numModes < 0 )
		{
			common->Warning( "Reason was: %s\n", SDL_GetError() );
		}
		FillStaticVidModes( modeList );
	}

	return true;
	// DG end
}
