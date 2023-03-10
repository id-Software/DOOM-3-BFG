/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013-2023 Robert Beckebans
Copyright (C) 2022 Stephen Pridham

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
** WIN_GLIMP.C
**
** This file contains ALL Win32 specific stuff having to do with the
** OpenGL refresh.  When a port is being made the following functions
** must be implemented by the port:
**
** GLimp_SwapBuffers
** GLimp_Init
** GLimp_Shutdown
** GLimp_SetGamma
**
** Note that the GLW_xxx functions are Windows specific GL-subsystem
** related functions that are relevant ONLY to win_glimp.c
*/
#include "precompiled.h"
#pragma hdrstop

#include "win_local.h"
#include "rc/doom_resource.h"
#include "../../renderer/RenderCommon.h"

#include <sys/DeviceManager.h>
extern DeviceManager* deviceManager;



/*
========================
GLimp_GetOldGammaRamp
========================
*/
static void GLimp_SaveGamma()
{
	HDC			hDC;
	BOOL		success;

	hDC = GetDC( GetDesktopWindow() );
	success = GetDeviceGammaRamp( hDC, win32.oldHardwareGamma );
	common->DPrintf( "...getting default gamma ramp: %s\n", success ? "success" : "failed" );
	ReleaseDC( GetDesktopWindow(), hDC );
}

/*
========================
GLimp_RestoreGamma
========================
*/
static void GLimp_RestoreGamma()
{
	HDC hDC;
	BOOL success;

	// if we never read in a reasonable looking
	// table, don't write it out
	if( win32.oldHardwareGamma[0][255] == 0 )
	{
		return;
	}

	hDC = GetDC( GetDesktopWindow() );
	success = SetDeviceGammaRamp( hDC, win32.oldHardwareGamma );
	common->DPrintf( "...restoring hardware gamma: %s\n", success ? "success" : "failed" );
	ReleaseDC( GetDesktopWindow(), hDC );
}


/*
========================
GLimp_SetGamma

The renderer calls this when the user adjusts r_gamma or r_brightness
========================
*/
void GLimp_SetGamma( unsigned short red[256], unsigned short green[256], unsigned short blue[256] )
{
	unsigned short table[3][256];
	int i;

	if( !win32.hDC )
	{
		return;
	}

	for( i = 0; i < 256; i++ )
	{
		table[0][i] = red[i];
		table[1][i] = green[i];
		table[2][i] = blue[i];
	}

	if( !SetDeviceGammaRamp( win32.hDC, table ) )
	{
		common->Printf( "WARNING: SetDeviceGammaRamp failed.\n" );
	}
}

/*
====================
GLW_CreateWindowClasses
====================
*/
static void GLW_CreateWindowClasses()
{
	WNDCLASS wc;

	//
	// register the window class if necessary
	//
	if( win32.windowClassRegistered )
	{
		return;
	}

	memset( &wc, 0, sizeof( wc ) );

	wc.style         = 0;
	wc.lpfnWndProc   = ( WNDPROC ) MainWndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = win32.hInstance;
	wc.hIcon         = LoadIcon( win32.hInstance, MAKEINTRESOURCE( IDI_ICON1 ) );
	wc.hCursor       = NULL;
	wc.hbrBackground = ( struct HBRUSH__* )COLOR_GRAYTEXT;
	wc.lpszMenuName  = 0;
	wc.lpszClassName = WIN32_WINDOW_CLASS_NAME;

	if( !RegisterClass( &wc ) )
	{
		common->FatalError( "GLW_CreateWindow: could not register window class" );
	}
	common->Printf( "...registered window class\n" );

	win32.windowClassRegistered = true;
}

/*
========================
GetDisplayName
========================
*/
static const char* GetDisplayName( const int deviceNum )
{
	static DISPLAY_DEVICE	device;
	device.cb = sizeof( device );
	if( !EnumDisplayDevices(
				0,			// lpDevice
				deviceNum,
				&device,
				0 /* dwFlags */ ) )
	{
		return NULL;
	}
	return device.DeviceName;
}

/*
========================
GetDeviceName
========================
*/
static idStr GetDeviceName( const int deviceNum )
{
	DISPLAY_DEVICE	device = {};
	device.cb = sizeof( device );
	if( !EnumDisplayDevices(
				0,			// lpDevice
				deviceNum,
				&device,
				0 /* dwFlags */ ) )
	{
		return idStr();
	}

	// get the monitor for this display
	if( !( device.StateFlags & ( DISPLAY_DEVICE_ATTACHED_TO_DESKTOP | DISPLAY_DEVICE_PRIMARY_DEVICE ) ) )
	{
		return idStr();
	}

	return idStr( device.DeviceName );
}

/*
========================
GetDisplayCoordinates
========================
*/
static bool GetDisplayCoordinates( const int deviceNum, int& x, int& y, int& width, int& height, int& displayHz )
{
	bool verbose = false;

	idStr deviceName = GetDeviceName( deviceNum );
	if( deviceName.Length() == 0 )
	{
		return false;
	}

	DISPLAY_DEVICE	device = {};
	device.cb = sizeof( device );
	if( !EnumDisplayDevices(
				0,			// lpDevice
				deviceNum,
				&device,
				0 /* dwFlags */ ) )
	{
		return false;
	}

	DISPLAY_DEVICE	monitor;
	monitor.cb = sizeof( monitor );
	if( !EnumDisplayDevices(
				deviceName.c_str(),
				0,
				&monitor,
				0 /* dwFlags */ ) )
	{
		return false;
	}

	DEVMODE	devmode;
	devmode.dmSize = sizeof( devmode );
	if( !EnumDisplaySettings( deviceName.c_str(), ENUM_CURRENT_SETTINGS, &devmode ) )
	{
		return false;
	}

	if( verbose )
	{
		common->Printf( "display device: %i\n", deviceNum );
		common->Printf( "  DeviceName  : %s\n", device.DeviceName );
		common->Printf( "  DeviceString: %s\n", device.DeviceString );
		common->Printf( "  StateFlags  : 0x%x\n", device.StateFlags );
		common->Printf( "  DeviceID    : %s\n", device.DeviceID );
		common->Printf( "  DeviceKey   : %s\n", device.DeviceKey );
		common->Printf( "      DeviceName  : %s\n", monitor.DeviceName );
		common->Printf( "      DeviceString: %s\n", monitor.DeviceString );
		common->Printf( "      StateFlags  : 0x%x\n", monitor.StateFlags );
		common->Printf( "      DeviceID    : %s\n", monitor.DeviceID );
		common->Printf( "      DeviceKey   : %s\n", monitor.DeviceKey );
		common->Printf( "          dmPosition.x      : %i\n", devmode.dmPosition.x );
		common->Printf( "          dmPosition.y      : %i\n", devmode.dmPosition.y );
		common->Printf( "          dmBitsPerPel      : %i\n", devmode.dmBitsPerPel );
		common->Printf( "          dmPelsWidth       : %i\n", devmode.dmPelsWidth );
		common->Printf( "          dmPelsHeight      : %i\n", devmode.dmPelsHeight );
		common->Printf( "          dmDisplayFlags    : 0x%x\n", devmode.dmDisplayFlags );
		common->Printf( "          dmDisplayFrequency: %i\n", devmode.dmDisplayFrequency );
	}

	x = devmode.dmPosition.x;
	y = devmode.dmPosition.y;
	width = devmode.dmPelsWidth;
	height = devmode.dmPelsHeight;
	displayHz = devmode.dmDisplayFrequency;

	return true;
}

/*
====================
DMDFO
====================
*/
static const char* DMDFO( int dmDisplayFixedOutput )
{
	switch( dmDisplayFixedOutput )
	{
		case DMDFO_DEFAULT:
			return "DMDFO_DEFAULT";
		case DMDFO_CENTER:
			return "DMDFO_CENTER";
		case DMDFO_STRETCH:
			return "DMDFO_STRETCH";
	}
	return "UNKNOWN";
}

/*
====================
PrintDevMode
====================
*/
static void PrintDevMode( DEVMODE& devmode )
{
	common->Printf( "          dmPosition.x        : %i\n", devmode.dmPosition.x );
	common->Printf( "          dmPosition.y        : %i\n", devmode.dmPosition.y );
	common->Printf( "          dmBitsPerPel        : %i\n", devmode.dmBitsPerPel );
	common->Printf( "          dmPelsWidth         : %i\n", devmode.dmPelsWidth );
	common->Printf( "          dmPelsHeight        : %i\n", devmode.dmPelsHeight );
	common->Printf( "          dmDisplayFixedOutput: %s\n", DMDFO( devmode.dmDisplayFixedOutput ) );
	common->Printf( "          dmDisplayFlags      : 0x%x\n", devmode.dmDisplayFlags );
	common->Printf( "          dmDisplayFrequency  : %i\n", devmode.dmDisplayFrequency );
}

/*
====================
DumpAllDisplayDevices
====================
*/
void DumpAllDisplayDevices()
{
	common->Printf( "\n" );
	for( int deviceNum = 0 ; ; deviceNum++ )
	{
		DISPLAY_DEVICE	device = {};
		device.cb = sizeof( device );
		if( !EnumDisplayDevices(
					0,			// lpDevice
					deviceNum,
					&device,
					0 /* dwFlags */ ) )
		{
			break;
		}

		common->Printf( "display device: %i\n", deviceNum );
		common->Printf( "  DeviceName  : %s\n", device.DeviceName );
		common->Printf( "  DeviceString: %s\n", device.DeviceString );
		common->Printf( "  StateFlags  : 0x%x\n", device.StateFlags );
		common->Printf( "  DeviceID    : %s\n", device.DeviceID );
		common->Printf( "  DeviceKey   : %s\n", device.DeviceKey );

		for( int monitorNum = 0 ; ; monitorNum++ )
		{
			DISPLAY_DEVICE	monitor = {};
			monitor.cb = sizeof( monitor );
			if( !EnumDisplayDevices(
						device.DeviceName,
						monitorNum,
						&monitor,
						0 /* dwFlags */ ) )
			{
				break;
			}

			common->Printf( "      DeviceName  : %s\n", monitor.DeviceName );
			common->Printf( "      DeviceString: %s\n", monitor.DeviceString );
			common->Printf( "      StateFlags  : 0x%x\n", monitor.StateFlags );
			common->Printf( "      DeviceID    : %s\n", monitor.DeviceID );
			common->Printf( "      DeviceKey   : %s\n", monitor.DeviceKey );

			DEVMODE	currentDevmode = {};
			if( !EnumDisplaySettings( device.DeviceName, ENUM_CURRENT_SETTINGS, &currentDevmode ) )
			{
				common->Printf( "ERROR:  EnumDisplaySettings(ENUM_CURRENT_SETTINGS) failed!\n" );
			}
			common->Printf( "          -------------------\n" );
			common->Printf( "          ENUM_CURRENT_SETTINGS\n" );
			PrintDevMode( currentDevmode );

			DEVMODE	registryDevmode = {};
			if( !EnumDisplaySettings( device.DeviceName, ENUM_REGISTRY_SETTINGS, &registryDevmode ) )
			{
				common->Printf( "ERROR:  EnumDisplaySettings(ENUM_CURRENT_SETTINGS) failed!\n" );
			}
			common->Printf( "          -------------------\n" );
			common->Printf( "          ENUM_CURRENT_SETTINGS\n" );
			PrintDevMode( registryDevmode );

			for( int modeNum = 0 ; ; modeNum++ )
			{
				DEVMODE	devmode = {};

				if( !EnumDisplaySettings( device.DeviceName, modeNum, &devmode ) )
				{
					break;
				}

				if( devmode.dmBitsPerPel != 32 )
				{
					continue;
				}
				if( devmode.dmDisplayFrequency < 60 )
				{
					continue;
				}
				if( devmode.dmPelsHeight < 720 )
				{
					continue;
				}
				common->Printf( "          -------------------\n" );
				common->Printf( "          modeNum             : %i\n", modeNum );
				PrintDevMode( devmode );
			}
		}
	}
	common->Printf( "\n" );
}

// RB: moved out of R_GetModeListForDisplay
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
// RB end

/*
====================
R_GetModeListForDisplay
====================
*/
bool R_GetModeListForDisplay( const int requestedDisplayNum, idList<vidMode_t>& modeList )
{
	modeList.Clear();

	bool	verbose = false;

	for( int displayNum = requestedDisplayNum; ; displayNum++ )
	{
		DISPLAY_DEVICE	device;
		device.cb = sizeof( device );
		if( !EnumDisplayDevices(
					0,			// lpDevice
					displayNum,
					&device,
					0 /* dwFlags */ ) )
		{
			return false;
		}

		// get the monitor for this display
		if( !( device.StateFlags & ( DISPLAY_DEVICE_ATTACHED_TO_DESKTOP | DISPLAY_DEVICE_PRIMARY_DEVICE ) ) )
		{
			continue;
		}

		DISPLAY_DEVICE	monitor;
		monitor.cb = sizeof( monitor );
		if( !EnumDisplayDevices(
					device.DeviceName,
					0,
					&monitor,
					0 /* dwFlags */ ) )
		{
			continue;
		}

		DEVMODE	devmode;
		devmode.dmSize = sizeof( devmode );

		if( verbose )
		{
			common->Printf( "display device: %i\n", displayNum );
			common->Printf( "  DeviceName  : %s\n", device.DeviceName );
			common->Printf( "  DeviceString: %s\n", device.DeviceString );
			common->Printf( "  StateFlags  : 0x%x\n", device.StateFlags );
			common->Printf( "  DeviceID    : %s\n", device.DeviceID );
			common->Printf( "  DeviceKey   : %s\n", device.DeviceKey );
			common->Printf( "      DeviceName  : %s\n", monitor.DeviceName );
			common->Printf( "      DeviceString: %s\n", monitor.DeviceString );
			common->Printf( "      StateFlags  : 0x%x\n", monitor.StateFlags );
			common->Printf( "      DeviceID    : %s\n", monitor.DeviceID );
			common->Printf( "      DeviceKey   : %s\n", monitor.DeviceKey );
		}

		for( int modeNum = 0 ; ; modeNum++ )
		{
			if( !EnumDisplaySettings( device.DeviceName, modeNum, &devmode ) )
			{
				break;
			}

			if( devmode.dmBitsPerPel != 32 )
			{
				continue;
			}
			if( ( devmode.dmDisplayFrequency != 60 ) &&
					( devmode.dmDisplayFrequency != 120 ) &&
					( devmode.dmDisplayFrequency != 144 ) &&
					( devmode.dmDisplayFrequency != 165 ) &&
					( devmode.dmDisplayFrequency != 240 ) )
			{
				continue;
			}
			if( devmode.dmPelsHeight < 720 )
			{
				continue;
			}
			if( verbose )
			{
				common->Printf( "          -------------------\n" );
				common->Printf( "          modeNum             : %i\n", modeNum );
				common->Printf( "          dmPosition.x        : %i\n", devmode.dmPosition.x );
				common->Printf( "          dmPosition.y        : %i\n", devmode.dmPosition.y );
				common->Printf( "          dmBitsPerPel        : %i\n", devmode.dmBitsPerPel );
				common->Printf( "          dmPelsWidth         : %i\n", devmode.dmPelsWidth );
				common->Printf( "          dmPelsHeight        : %i\n", devmode.dmPelsHeight );
				common->Printf( "          dmDisplayFixedOutput: %s\n", DMDFO( devmode.dmDisplayFixedOutput ) );
				common->Printf( "          dmDisplayFlags      : 0x%x\n", devmode.dmDisplayFlags );
				common->Printf( "          dmDisplayFrequency  : %i\n", devmode.dmDisplayFrequency );
			}
			vidMode_t mode;
			mode.width = devmode.dmPelsWidth;
			mode.height = devmode.dmPelsHeight;
			mode.displayHz = devmode.dmDisplayFrequency;
			modeList.AddUnique( mode );
		}

		if( modeList.Num() > 0 )
		{
			// sort with lowest resolution first
			modeList.SortWithTemplate( idSort_VidMode() );

			return true;
		}
	}
	// Never gets here
}

/*
====================
GLW_GetWindowDimensions
====================
*/
static bool GLW_GetWindowDimensions( const glimpParms_t parms, int& x, int& y, int& w, int& h )
{
	//
	// compute width and height
	//
	if( parms.fullScreen != 0 )
	{
		if( parms.fullScreen == -1 )
		{
			// borderless window at specific location, as for spanning
			// multiple monitor outputs
			x = parms.x;
			y = parms.y;
			w = parms.width;
			h = parms.height;
		}
		else
		{
			// get the current monitor position and size on the desktop, assuming
			// any required ChangeDisplaySettings has already been done
			int displayHz = 0;
			if( !GetDisplayCoordinates( parms.fullScreen - 1, x, y, w, h, displayHz ) )
			{
				return false;
			}
		}
	}
	else
	{
		RECT	r;

		// adjust width and height for window border
		r.bottom = parms.height;
		r.left = 0;
		r.top = 0;
		r.right = parms.width;

		AdjustWindowRect( &r, WINDOW_STYLE | WS_SYSMENU, FALSE );

		w = r.right - r.left;
		h = r.bottom - r.top;

		x = parms.x;
		y = parms.y;
	}

	return true;
}

static bool GetCenteredWindowDimensions( int& x, int& y, int& w, int& h )
{
	// get position and size of default display for windowed mode (parms.fullScreen = 0)
	int displayX, displayY, displayW, displayH, displayHz = 0;
	if( !GetDisplayCoordinates( 0, displayX, displayY, displayW, displayH, displayHz ) )
	{
		return false;
	}

	// find the centered position not exceeding display bounds
	const int centreX = displayX + displayW / 2;
	const int centreY = displayY + displayH / 2;
	const int left = centreX - w / 2;
	const int right = left + w;
	const int top = centreY - h / 2;
	const int bottom = top + h;
	x = Max( left, displayX );
	y = Max( top, displayY );
	w = Min( right - left, displayW );
	h = Min( bottom - top, displayH );

	return true;
}

bool DeviceManager::CreateWindowDeviceAndSwapChain( const glimpParms_t& parms, const char* windowTitle )
{
	int x, y, w, h;
	if( !GLW_GetWindowDimensions( parms, x, y, w, h ) )
	{
		return false;
	}

	// SRS - if in windowed mode, start with centered windowed on default display, afterwards use r_windowX / r_windowY
	if( parms.fullScreen == 0 )
	{
		if( !GetCenteredWindowDimensions( x, y, w, h ) )
		{
			return false;
		}
	}

	int	stylebits;
	int	exstyle;

	if( parms.fullScreen != 0 )
	{
		exstyle = WS_EX_TOPMOST;
		stylebits = WS_POPUP | WS_VISIBLE | WS_SYSMENU;
	}
	else
	{
		exstyle = 0;
		stylebits = WINDOW_STYLE | WS_SYSMENU;
	}

	win32.hWnd = CreateWindowEx(
					 exstyle,
					 WIN32_WINDOW_CLASS_NAME,
					 GAME_NAME,
					 stylebits,
					 x, y, w, h,
					 NULL,
					 NULL,
					 win32.hInstance,
					 NULL );

	windowInstance = win32.hInstance;
	windowHandle = win32.hWnd;

	if( !win32.hWnd )
	{
		common->Printf( "^3GLW_CreateWindow() - Couldn't create window^0\n" );
		return false;
	}

	::SetTimer( win32.hWnd, 0, 100, NULL );

	ShowWindow( win32.hWnd, SW_SHOW );
	UpdateWindow( win32.hWnd );
	common->Printf( "...created window @ %d,%d (%dx%d)\n", x, y, w, h );

	// makeCurrent NULL frees the DC, so get another
	win32.hDC = GetDC( win32.hWnd );
	if( !win32.hDC )
	{
		common->Printf( "^3GLW_CreateWindow() - GetDC()failed^0\n" );
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

	SetForegroundWindow( win32.hWnd );
	SetFocus( win32.hWnd );

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
		BackBufferResized();
	}
	else
	{
		m_DeviceParams.vsyncEnabled = m_RequestedVSync;
	}
}

/*
=======================
GLW_CreateWindow

Responsible for creating the Win32 window.
If fullscreen, it won't have a border
=======================
*/
static bool GLW_CreateWindow( glimpParms_t parms )
{
	int				x, y, w, h;
	if( !GLW_GetWindowDimensions( parms, x, y, w, h ) )
	{
		return false;
	}

	int				stylebits;
	int				exstyle;
	if( parms.fullScreen != 0 )
	{
		exstyle = WS_EX_TOPMOST;
		stylebits = WS_POPUP | WS_VISIBLE | WS_SYSMENU;
	}
	else
	{
		exstyle = 0;
		stylebits = WINDOW_STYLE | WS_SYSMENU;
	}

	win32.hWnd = CreateWindowEx(
					 exstyle,
					 WIN32_WINDOW_CLASS_NAME,
					 GAME_NAME,
					 stylebits,
					 x, y, w, h,
					 NULL,
					 NULL,
					 win32.hInstance,
					 NULL );

	if( !win32.hWnd )
	{
		common->Printf( "^3GLW_CreateWindow() - Couldn't create window^0\n" );
		return false;
	}

	::SetTimer( win32.hWnd, 0, 100, NULL );

	ShowWindow( win32.hWnd, SW_SHOW );
	UpdateWindow( win32.hWnd );
	common->Printf( "...created window @ %d,%d (%dx%d)\n", x, y, w, h );

	// makeCurrent NULL frees the DC, so get another
	win32.hDC = GetDC( win32.hWnd );
	if( !win32.hDC )
	{
		common->Printf( "^3GLW_CreateWindow() - GetDC()failed^0\n" );
		return false;
	}

	// TODO
	glConfig.stereoPixelFormatAvailable = false;

	SetForegroundWindow( win32.hWnd );
	SetFocus( win32.hWnd );

	glConfig.isFullscreen = parms.fullScreen;

	return true;
}

/*
===================
PrintCDSError
===================
*/
static void PrintCDSError( int value )
{
	switch( value )
	{
		case DISP_CHANGE_RESTART:
			common->Printf( "restart required\n" );
			break;
		case DISP_CHANGE_BADPARAM:
			common->Printf( "bad param\n" );
			break;
		case DISP_CHANGE_BADFLAGS:
			common->Printf( "bad flags\n" );
			break;
		case DISP_CHANGE_FAILED:
			common->Printf( "DISP_CHANGE_FAILED\n" );
			break;
		case DISP_CHANGE_BADMODE:
			common->Printf( "bad mode\n" );
			break;
		case DISP_CHANGE_NOTUPDATED:
			common->Printf( "not updated\n" );
			break;
		default:
			common->Printf( "unknown error %d\n", value );
			break;
	}
}

/*
===================
GLW_ChangeDislaySettingsIfNeeded

Optionally ChangeDisplaySettings to get a different fullscreen resolution.
Default uses the full desktop resolution.
===================
*/
static bool GLW_ChangeDislaySettingsIfNeeded( glimpParms_t parms )
{
	// If we had previously changed the display settings on a different monitor,
	// go back to standard.
	if( win32.cdsFullscreen != 0 && win32.cdsFullscreen != parms.fullScreen )
	{
		win32.cdsFullscreen = 0;
		ChangeDisplaySettings( 0, 0 );
		Sys_Sleep( 1000 ); // Give the driver some time to think about this change
	}

	// 0 is dragable mode on desktop, -1 is borderless window on desktop
	if( parms.fullScreen <= 0 )
	{
		return true;
	}

	// if we are already in the right resolution, don't do a ChangeDisplaySettings
	int x, y, width, height, displayHz;

	if( !GetDisplayCoordinates( parms.fullScreen - 1, x, y, width, height, displayHz ) )
	{
		return false;
	}
	if( width == parms.width && height == parms.height && ( displayHz == parms.displayHz || parms.displayHz == 0 ) )
	{
		return true;
	}

	DEVMODE dm = {};

	dm.dmSize = sizeof( dm );

	dm.dmPelsWidth  = parms.width;
	dm.dmPelsHeight = parms.height;
	dm.dmBitsPerPel = 32;
	dm.dmFields     = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL;
	if( parms.displayHz != 0 )
	{
		dm.dmDisplayFrequency = parms.displayHz;
		dm.dmFields |= DM_DISPLAYFREQUENCY;
	}

	common->Printf( "...calling CDS: " );

	const char* const deviceName = GetDisplayName( parms.fullScreen - 1 );

	int		cdsRet;
	if( ( cdsRet = ChangeDisplaySettingsEx(
					   deviceName,
					   &dm,
					   NULL,
					   CDS_FULLSCREEN,
					   NULL ) ) == DISP_CHANGE_SUCCESSFUL )
	{
		common->Printf( "ok\n" );
		win32.cdsFullscreen = parms.fullScreen;
		return true;
	}

	common->Printf( "^3failed^0, " );
	PrintCDSError( cdsRet );
	return false;
}

void DeviceManager::SetWindowTitle( const char* title )
{
	SetWindowTextA( ( HWND )windowHandle, title );
}

void GLimp_PreInit()
{
	// DG: not needed on this platform, so just do nothing
}

// SRS - Function to get display frequency of monitor hosting the current window
static int GetDisplayFrequency( glimpParms_t parms )
{
	HMONITOR hMonitor = MonitorFromWindow( win32.hWnd, MONITOR_DEFAULTTOPRIMARY );

	MONITORINFOEX minfo;
	minfo.cbSize = sizeof( minfo );
	if( !GetMonitorInfo( hMonitor, &minfo ) )
	{
		return parms.displayHz;
	}

	DEVMODE devmode;
	devmode.dmSize = sizeof( devmode );
	if( !EnumDisplaySettings( minfo.szDevice, ENUM_CURRENT_SETTINGS, &devmode ) )
	{
		return parms.displayHz;
	}

	return devmode.dmDisplayFrequency;
}

/*
===================
GLimp_Init

This is the platform specific OpenGL initialization function.  It
is responsible for loading OpenGL, initializing it,
creating a window of the appropriate size, doing
fullscreen manipulations, etc.  Its overall responsibility is
to make sure that a functional OpenGL subsystem is operating
when it returns to the ref.

If there is any failure, the renderer will revert back to safe
parameters and try again.
===================
*/
bool GLimp_Init( glimpParms_t parms )
{
	HDC		hDC;

	// check our desktop attributes
	hDC = GetDC( GetDesktopWindow() );
	win32.desktopBitsPixel = GetDeviceCaps( hDC, BITSPIXEL );
	win32.desktopWidth = GetDeviceCaps( hDC, HORZRES );
	win32.desktopHeight = GetDeviceCaps( hDC, VERTRES );
	ReleaseDC( GetDesktopWindow(), hDC );

	// we can't run in a window unless it is 32 bpp
	if( win32.desktopBitsPixel < 32 && parms.fullScreen <= 0 )
	{
		common->Printf( "^3Windowed mode requires 32 bit desktop depth^0\n" );
		return false;
	}

	// save the hardware gamma so it can be
	// restored on exit
	GLimp_SaveGamma();

	// create our window classes if we haven't already
	GLW_CreateWindowClasses();

	// Optionally ChangeDisplaySettings to get a different fullscreen resolution.
	if( !GLW_ChangeDislaySettingsIfNeeded( parms ) )
	{
		GLimp_Shutdown();
		return false;
	}

	// try to create a window with the correct pixel format
	// and init the renderer context
	if( !deviceManager->CreateWindowDeviceAndSwapChain( parms, GAME_NAME ) )
	{
		//deviceManager->Shutdown();
		GLimp_Shutdown();
		return false;
	}

	glConfig.isFullscreen = parms.fullScreen;
	glConfig.isStereoPixelFormat = parms.stereo;
	glConfig.nativeScreenWidth = parms.width;
	glConfig.nativeScreenHeight = parms.height;
	glConfig.displayFrequency = GetDisplayFrequency( parms );
	glConfig.multisamples = parms.multiSamples;

	glConfig.pixelAspect = 1.0f;	// FIXME: some monitor modes may be distorted
	// should side-by-side stereo modes be consider aspect 0.5?

	// get the screen size, which may not be reliable...
	// If we use the windowDC, I get my 30" monitor, even though the window is
	// on a 27" monitor, so get a dedicated DC for the full screen device name.
	const idStr deviceName = GetDeviceName( Max( 0, parms.fullScreen - 1 ) );

	HDC deviceDC = CreateDC( deviceName.c_str(), deviceName.c_str(), NULL, NULL );
	const int mmWide = GetDeviceCaps( win32.hDC, HORZSIZE );
	DeleteDC( deviceDC );

	if( mmWide == 0 )
	{
		glConfig.physicalScreenWidthInCentimeters = 100.0f;
	}
	else
	{
		glConfig.physicalScreenWidthInCentimeters = 0.1f * mmWide;
	}

	return true;
}

/*
===================
GLimp_SetScreenParms

Sets up the screen based on passed parms..
===================
*/
bool GLimp_SetScreenParms( glimpParms_t parms )
{
	// Optionally ChangeDisplaySettings to get a different fullscreen resolution.
	if( !GLW_ChangeDislaySettingsIfNeeded( parms ) )
	{
		return false;
	}

	int x, y, w, h;
	if( !GLW_GetWindowDimensions( parms, x, y, w, h ) )
	{
		return false;
	}

	int exstyle;
	int stylebits;

	if( parms.fullScreen )
	{
		exstyle = WS_EX_TOPMOST;
		stylebits = WS_POPUP | WS_VISIBLE | WS_SYSMENU;
	}
	else
	{
		exstyle = 0;
		stylebits = WINDOW_STYLE | WS_SYSMENU;
	}

	// TODO(Stephen): Update the swap chain.
	SetWindowLong( win32.hWnd, GWL_STYLE, stylebits );
	SetWindowLong( win32.hWnd, GWL_EXSTYLE, exstyle );
	SetWindowPos( win32.hWnd, parms.fullScreen ? HWND_TOPMOST : HWND_NOTOPMOST, x, y, w, h, SWP_SHOWWINDOW );

	glConfig.isFullscreen = parms.fullScreen;
	glConfig.pixelAspect = 1.0f;	// FIXME: some monitor modes may be distorted

	glConfig.isStereoPixelFormat = parms.stereo;
	glConfig.nativeScreenWidth = parms.width;
	glConfig.nativeScreenHeight = parms.height;
	glConfig.displayFrequency = GetDisplayFrequency( parms );
	glConfig.multisamples = parms.multiSamples;

	return true;
}

void DeviceManager::Shutdown()
{
	DestroyDeviceAndSwapChain();

	const char* success[] = { "failed", "success" };
	int retVal;

	// release DC
	if( win32.hDC )
	{
		retVal = ReleaseDC( win32.hWnd, win32.hDC ) != 0;
		common->Printf( "...releasing DC: %s\n", success[retVal] );
		win32.hDC = NULL;
	}

	// destroy window
	if( windowHandle )
	{
		common->Printf( "...destroying window\n" );
		ShowWindow( ( HWND )windowHandle, SW_HIDE );
		DestroyWindow( ( HWND )windowHandle );
		windowHandle = nullptr;
		win32.hWnd = NULL;
	}

	// reset display settings
	if( win32.cdsFullscreen )
	{
		common->Printf( "...resetting display\n" );
		ChangeDisplaySettings( 0, 0 );
		win32.cdsFullscreen = 0;
	}

	// restore gamma
	GLimp_RestoreGamma();
}

/*
===================
GLimp_Shutdown

This routine does all OS specific shutdown procedures for the OpenGL
subsystem.
===================
*/
void GLimp_Shutdown()
{
	if( deviceManager )
	{
		deviceManager->Shutdown();
	}
}




