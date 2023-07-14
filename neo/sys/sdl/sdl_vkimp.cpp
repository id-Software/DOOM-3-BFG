/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.
Copyright (C) 2012 dhewg (dhewm3)
Copyright (C) 2012-2014 Robert Beckebans
Copyright (C) 2013 Daniel Gibson
Copyright (C) 2023 Stephen Saunders

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

#include <sys/DeviceManager.h>
extern DeviceManager* deviceManager;

idCVar in_nograb( "in_nograb", "0", CVAR_SYSTEM | CVAR_NOCHEAT, "prevents input grabbing" );

static bool grabbed = false;
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

// SRS - Helper method for creating SDL Vulkan surface within DeviceManager_VK() when NVRHI enabled
vk::Result DeviceManager::CreateSDLWindowSurface( vk::Instance instance, vk::SurfaceKHR* surface )
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
	if( parms.fullScreen == -1 )
	{
		flags |= SDL_WINDOW_BORDERLESS;
	}

	window = SDL_CreateWindow( GAME_NAME,
							   parms.x,
							   parms.y,
							   parms.width, parms.height, flags );

	if( !window )
	{
		common->Warning( "Error while creating SDL Vulkan window: %s", SDL_GetError() );
		return false;
	}

	// RB
	m_DeviceParams.backBufferWidth = parms.width;
	m_DeviceParams.backBufferHeight = parms.height;
	m_DeviceParams.backBufferSampleCount = parms.multiSamples;
	m_DeviceParams.vsyncEnabled = m_RequestedVSync;

	if( !CreateDeviceAndSwapChain() )
	{
		return false;
	}

	return true;
}

void DeviceManager::UpdateWindowSize( const glimpParms_t& parms )
{
	m_windowVisible = true;

	if( int( m_DeviceParams.backBufferWidth ) != parms.width ||
			int( m_DeviceParams.backBufferHeight ) != parms.height ||
#if ID_MSAA
			int( m_DeviceParams.backBufferSampleCount ) != parms.multiSamples ||
#endif
			( m_DeviceParams.vsyncEnabled != m_RequestedVSync && GetGraphicsAPI() == nvrhi::GraphicsAPI::VULKAN ) )
	{
		// window is not minimized, and the size has changed

		BackBufferResizing();

		m_DeviceParams.backBufferWidth = parms.width;
		m_DeviceParams.backBufferHeight = parms.height;
		m_DeviceParams.backBufferSampleCount = parms.multiSamples;
		m_DeviceParams.vsyncEnabled = m_RequestedVSync;

		ResizeSwapChain();

		// SRS - Get actual swapchain dimensions to set new render size
		deviceManager->GetWindowDimensions( glConfig.nativeScreenWidth, glConfig.nativeScreenHeight );

		BackBufferResized();
	}
	else
	{
		m_DeviceParams.vsyncEnabled = m_RequestedVSync;
	}
}

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
#if defined(__APPLE__) && SDL_VERSION_ATLEAST(2, 0, 2)
		// SRS - Disable macOS Spaces for multi-monitor desktop in borderless fullscreen mode
		SDL_SetHint( SDL_HINT_VIDEO_MAC_FULLSCREEN_SPACES, "0" );
#endif
		if( SDL_Init( SDL_INIT_VIDEO ) )
		{
			common->Error( "Error while initializing SDL: %s", SDL_GetError() );
		}
	}
}

/*
===================
 Helper functions for VKimp_Init() and VKimp_SetScreenParms()
===================
*/

// SRS - Function to get display index for both fullscreen and windowed modes
static int GetDisplayIndex( glimpParms_t parms )
{
	int displayIdx = -1;

	if( parms.fullScreen > 0 )
	{
		if( parms.fullScreen <= SDL_GetNumVideoDisplays() )
		{
			displayIdx = parms.fullScreen - 1; // first display for SDL is 0, in parms it's 1
		}
	}
	else // 0, -1, -2 == windowed and borderless modes
	{
		// SRS - Find display containing the center of the bordered or borderless window, return -1 if not found
		int windowPosX = parms.x + parms.width / 2;
		int windowPosY = parms.y + parms.height / 2;

		for( int i = 0; i < SDL_GetNumVideoDisplays(); i++ )
		{
			SDL_Rect rect;
			SDL_GetDisplayBounds( i, &rect );
			if( ( windowPosX >= rect.x && windowPosX < ( rect.x + rect.w ) && windowPosY >= rect.y && windowPosY < ( rect.y + rect.h ) ) ||
					( parms.x == SDL_WINDOWPOS_CENTERED_DISPLAY( i ) && parms.y == SDL_WINDOWPOS_CENTERED_DISPLAY( i ) ) )
			{
				displayIdx = i;
				break;
			}
		}
	}

	return displayIdx;
}

// SRS - Function to get display frequency of monitor corresponding to the window position
static int GetDisplayFrequency( glimpParms_t parms )
{
	int displayIndex = GetDisplayIndex( parms );
	if( displayIndex < 0 )
	{
		// SRS - window is out of bounds for desktop, fall back to primary display
		displayIndex = 0;
	}

	SDL_DisplayMode m = {0};
	if( SDL_GetCurrentDisplayMode( displayIndex, &m ) )
	{
		common->Warning( "Couldn't get display refresh rate, reason: %s", SDL_GetError() );
		return parms.displayHz;
	}

	return m.refresh_rate;
}

/*
===================
VKimp_Init
===================
*/
bool VKimp_Init( glimpParms_t parms )
{

	common->Printf( "Initializing Vulkan subsystem\n" );

	// DG: make sure SDL is initialized
	VKimp_PreInit();

	// SRS - create window in the specified desktop position unless overridden by modes below
	glimpParms_t createParms = parms;
	if( parms.fullScreen == 0 )
	{
		// SRS - startup with window in centered position, use r_windowX and r_windowY after
		createParms.x = createParms.y = SDL_WINDOWPOS_CENTERED;
	}
	else if( parms.fullScreen > 0 )
	{
		if( parms.fullScreen > SDL_GetNumVideoDisplays() )
		{
			common->Warning( "Couldn't set display to r_fullscreen = %i because we only have %i display(s)",
							 parms.fullScreen, SDL_GetNumVideoDisplays() );
			return false;
		}
		else
		{
			// -1 because SDL starts counting displays at 0, while parms.fullScreen starts at 1
			createParms.x = createParms.y = SDL_WINDOWPOS_CENTERED_DISPLAY( ( parms.fullScreen - 1 ) );
		}
	}
	else if( GetDisplayIndex( parms ) < 0 ) // verify window position for -1 and -2 borderless modes
	{
		// SRS - window is out of bounds for desktop, startup on primary display instead
		createParms.x = createParms.y = SDL_WINDOWPOS_CENTERED;
		common->Warning( "Window position out of bounds, falling back to primary display" );
	}

	if( !deviceManager->CreateWindowDeviceAndSwapChain( createParms, GAME_NAME ) )
	{
		common->Warning( "Couldn't initialize Vulkan subsystem for r_fullscreen = %i", parms.fullScreen );
		return false;
	}

	if( parms.fullScreen > 0 )
	{
		// SRS - Make sure display is set to the requested refresh rate
		if( parms.displayHz > 0 && parms.displayHz != GetDisplayFrequency( parms ) )
		{
			SDL_DisplayMode m = {0};
			SDL_GetWindowDisplayMode( window, &m );

			m.refresh_rate = parms.displayHz;
			if( SDL_SetWindowDisplayMode( window, &m ) < 0 )
			{
				common->Warning( "Couldn't set display refresh rate to %i Hz, reason: %s", parms.displayHz, SDL_GetError() );
			}
		}

		// SRS - Switch into fullscreen mode after window creation to avoid SDL platform differences
		if( SDL_SetWindowFullscreen( window, SDL_WINDOW_FULLSCREEN ) < 0 )
		{
			common->Warning( "Couldn't switch to fullscreen mode, reason: %s", SDL_GetError() );
		}
	}
	else if( parms.fullScreen == -2 )
	{
		// SRS - Switch into borderless fullscreen mode after window creation
		if( SDL_SetWindowFullscreen( window, SDL_WINDOW_FULLSCREEN_DESKTOP ) < 0 )
		{
			common->Warning( "Couldn't switch to borderless fullscreen mode, reason: %s", SDL_GetError() );
		}
	}
	else if( parms.fullScreen == -1 )
	{
		// SRS - Make sure custom borderless window is in position after window creation
		SDL_SetWindowPosition( window, createParms.x, createParms.y );
	}

	if( parms.fullScreen )
	{
		// SRS - Get window's client area dimensions to set initial render size for fullscreen modes
		SDL_GetWindowSize( window, &glConfig.nativeScreenWidth, &glConfig.nativeScreenHeight );
	}
	else
	{
		// SRS - Get actual swapchain dimensions to set initial render size for windowed mode
		deviceManager->GetWindowDimensions( glConfig.nativeScreenWidth, glConfig.nativeScreenHeight );
	}

	// SRS - Detect and save actual fullscreen state supporting all modes (-2, -1, 0, 1, ...)
	glConfig.isFullscreen = ( SDL_GetWindowFlags( window ) & SDL_WINDOW_FULLSCREEN ) || ( parms.fullScreen == -1 ) ? parms.fullScreen : 0;

	// RB begin
	glConfig.displayFrequency = GetDisplayFrequency( parms );
	glConfig.isStereoPixelFormat = parms.stereo;
	glConfig.multisamples = parms.multiSamples;

	glConfig.pixelAspect = 1.0f;	// FIXME: some monitor modes may be distorted
	// should side-by-side stereo modes be consider aspect 0.5?
	// RB end

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
	// SRS - For reliable operation on all SDL2 platforms, disable fullscreen before monitor or mode switching
	if( SDL_GetWindowFlags( window ) & SDL_WINDOW_FULLSCREEN )
	{
		// if we're already in fullscreen mode but want to switch to another monitor
		// we have to go to windowed mode first to move the window.. SDL-oddity.
		SDL_SetWindowFullscreen( window, SDL_FALSE );
	}

	// SRS - For reliable operation on all SDL2 platforms, restore window before monitor or mode switching
	if( SDL_GetWindowFlags( window ) & SDL_WINDOW_MAXIMIZED )
	{
		// if window is maximized but want to switch to another monitor
		// we have to restore first to move the window.. SDL-oddity.
		SDL_RestoreWindow( window );
	}

	int displayIdx = GetDisplayIndex( parms );

	if( parms.fullScreen > 0 )
	{
		if( displayIdx < 0 )
		{
			common->Warning( "Couldn't set display to r_fullscreen = %i because we only have %i display(s)",
							 parms.fullScreen, SDL_GetNumVideoDisplays() );
			return displayIdx;
		}

		if( parms.fullScreen != glConfig.isFullscreen )
		{
			// select display ; SDL_WINDOWPOS_UNDEFINED_DISPLAY() doesn't work.
			int windowPos = SDL_WINDOWPOS_CENTERED_DISPLAY( displayIdx );
			// move window to the center of selected display
			SDL_SetWindowPosition( window, windowPos, windowPos );
		}
	}
	else // -2 == use current display for borderless fullscreen
	{
		// SRS - verify window position
		int windowPosX = parms.x, windowPosY = parms.y;
		if( displayIdx < 0 )
		{
			// SRS - window is out of bounds for desktop, reposition onto primary display
			displayIdx = 0;
			windowPosX = windowPosY = SDL_WINDOWPOS_CENTERED;
			common->Warning( "Window position out of bounds, falling back to primary display" );
		}

		// move window to the specified desktop position
		SDL_SetWindowPosition( window, windowPosX, windowPosY );
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

	// change displaymode settings according to parms
	// FIXME: check if refreshrate, width and height are supported?
	if( parms.fullScreen > 0 )
	{
		SDL_DisplayMode m = {0};
		SDL_GetWindowDisplayMode( window, &m );

		m.w = parms.width;
		m.h = parms.height;
		m.refresh_rate = parms.displayHz;

		// set the window displaymode
		if( SDL_SetWindowDisplayMode( window, &m ) < 0 )
		{
			common->Warning( "Couldn't set window mode for fullscreen, reason: %s", SDL_GetError() );
			return false;
		}

		// SRS - Move to fullscreen mode
		if( SDL_SetWindowFullscreen( window, SDL_WINDOW_FULLSCREEN ) < 0 )
		{
			common->Warning( "Couldn't switch to fullscreen mode, reason: %s", SDL_GetError() );
			return false;
		}
	}
	else // -2 == use current display for borderless fullscreen
	{
		// SRS - Move to borderless fullscreen mode
		if( SDL_SetWindowFullscreen( window, SDL_WINDOW_FULLSCREEN_DESKTOP ) < 0 )
		{
			common->Warning( "Couldn't switch to borderless fullscreen mode, reason: %s", SDL_GetError() );
			return false;
		}
	}
	return true;
}

static bool SetScreenParmsWindowed( glimpParms_t parms )
{
	// SRS - verify window position for 0 and -1 windowed modes
	int windowPosX = parms.x, windowPosY = parms.y;
	if( GetDisplayIndex( parms ) < 0 )
	{
		// SRS - window is out of bounds for desktop, reposition onto primary display
		windowPosX = windowPosY = SDL_WINDOWPOS_CENTERED;
		common->Warning( "Window position out of bounds, falling back to primary display" );
	}

	// SRS - handle differences in WM behaviour: for macOS set position first, for linux set it last
#if defined(__APPLE__)
	SDL_SetWindowPosition( window, windowPosX, windowPosY );
#endif

	// if we're currently in fullscreen mode, we need to disable that
	if( SDL_GetWindowFlags( window ) & SDL_WINDOW_FULLSCREEN )
	{
		if( SDL_SetWindowFullscreen( window, SDL_FALSE ) < 0 )
		{
			common->Warning( "Couldn't switch to windowed mode, reason: %s", SDL_GetError() );
			return false;
		}
	}

	// if window is maximized, restore it to normal before setting size
	if( SDL_GetWindowFlags( window ) & SDL_WINDOW_MAXIMIZED )
	{
		SDL_RestoreWindow( window );
	}

	// set window to bordered or borderless based on parms
	SDL_SetWindowBordered( window, parms.fullScreen == 0 ? SDL_TRUE : SDL_FALSE );

	SDL_SetWindowSize( window, parms.width, parms.height );

#if !defined(__APPLE__)
	SDL_SetWindowPosition( window, windowPosX, windowPosY );
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
	else // windowed modes 0 and -1
	{
		if( !SetScreenParmsWindowed( parms ) )
		{
			return false;
		}
	}

	glConfig.isFullscreen = parms.fullScreen;
	glConfig.isStereoPixelFormat = parms.stereo;

	// SRS - Get window's client area dimensions to set new render size
	SDL_GetWindowSize( window, &glConfig.nativeScreenWidth, &glConfig.nativeScreenHeight );

	glConfig.displayFrequency = GetDisplayFrequency( parms );
	glConfig.multisamples = parms.multiSamples;

	return true;
}

void DeviceManager::Shutdown()
{
	DestroyDeviceAndSwapChain();
}

/*
===================
VKimp_Shutdown
===================
*/
void VKimp_Shutdown()
{
	common->Printf( "Shutting down Vulkan subsystem\n" );

	if( deviceManager )
	{
		deviceManager->Shutdown();
	}

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
}

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
	modeList.AddUnique( vidMode_t( 2560, 1440, 60 ) );
	modeList.AddUnique( vidMode_t( 2560, 1600, 60 ) );
	modeList.AddUnique( vidMode_t( 3840, 2160, 60 ) );

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

	bool	verbose = false;

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
				common->Warning( "Can't get video mode no %i, because of %s", i, SDL_GetError() );
				continue;
			}

			if( SDL_BITSPERPIXEL( m.format ) != 32 && SDL_BITSPERPIXEL( m.format ) != 24 )
			{
				continue;
			}
			if( ( m.refresh_rate != 60 ) && ( m.refresh_rate != 120 ) &&
					( m.refresh_rate != 144 ) && ( m.refresh_rate != 165 ) && ( m.refresh_rate != 240 ) )
			{
				continue;
			}
			if( m.h < 720 )
			{
				continue;
			}
			if( verbose )
			{
				common->Printf( "          -------------------\n" );
				common->Printf( "          modeNum             : %i\n", i );
				common->Printf( "          dmBitsPerPel        : %i\n", SDL_BITSPERPIXEL( m.format ) );
				common->Printf( "          dmPelsWidth         : %i\n", m.w );
				common->Printf( "          dmPelsHeight        : %i\n", m.h );
				common->Printf( "          dmDisplayFrequency  : %i\n", m.refresh_rate );
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
			common->Warning( "Reason was: %s", SDL_GetError() );
		}
		FillStaticVidModes( modeList );
	}

	return true;
	// DG end
}
