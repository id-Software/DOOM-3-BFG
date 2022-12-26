/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013-2014 Robert Beckebans
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

#if defined( USE_NVRHI )
	#include <sys/DeviceManager.h>
	extern DeviceManager* deviceManager;
#endif



#if defined(_WIN32)
	idCVar r_useOpenGL45( "r_useOpenGL45", "1", CVAR_INTEGER, "0 = OpenGL 4.0, 1 = OpenGL 4.5 compatibility profile, 2 = OpenGL 4.5 core profile", 0, 2 );
#else
	idCVar r_useOpenGL45( "r_useOpenGL45", "2", CVAR_INTEGER, "0 = OpenGL 4.0, 1 = OpenGL 4.5 compatibility profile, 2 = OpenGL 4.5 core profile", 0, 2 );
#endif



#if !defined(USE_VULKAN) && !defined(USE_NVRHI)
/*
========================
GLimp_TestSwapBuffers
========================
*/
void GLimp_TestSwapBuffers( const idCmdArgs& args )
{
	idLib::Printf( "GLimp_TimeSwapBuffers\n" );
	static const int MAX_FRAMES = 5;
	uint64	timestamps[MAX_FRAMES];
	glDisable( GL_SCISSOR_TEST );

	int frameMilliseconds = 16;
	for( int swapInterval = 2 ; swapInterval >= -1 ; swapInterval-- )
	{
		wglSwapIntervalEXT( swapInterval );
		for( int i = 0 ; i < MAX_FRAMES ; i++ )
		{
			if( swapInterval == -1 )
			{
				Sys_Sleep( frameMilliseconds );
			}
			if( i & 1 )
			{
				glClearColor( 0, 1, 0, 1 );
			}
			else
			{
				glClearColor( 1, 0, 0, 1 );
			}
			glClear( GL_COLOR_BUFFER_BIT );
			SwapBuffers( win32.hDC );
			glFinish();
			timestamps[i] = Sys_Microseconds();
		}

		idLib::Printf( "\nswapinterval %i\n", swapInterval );
		for( int i = 1 ; i < MAX_FRAMES ; i++ )
		{
			idLib::Printf( "%i microseconds\n", ( int )( timestamps[i] - timestamps[i - 1] ) );
		}
	}
}
#endif

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
=============================================================================

WglExtension Grabbing

This is gross -- creating a window just to get a context to get the wgl extensions

=============================================================================
*/

/*
====================
FakeWndProc

Only used to get wglExtensions
====================
*/
#if !defined(USE_VULKAN) && !defined(USE_NVRHI)
LONG WINAPI FakeWndProc(
	HWND    hWnd,
	UINT    uMsg,
	WPARAM  wParam,
	LPARAM  lParam )
{

	if( uMsg == WM_DESTROY )
	{
		PostQuitMessage( 0 );
	}

	if( uMsg != WM_CREATE )
	{
		return DefWindowProc( hWnd, uMsg, wParam, lParam );
	}

	const static PIXELFORMATDESCRIPTOR pfd =
	{
		sizeof( PIXELFORMATDESCRIPTOR ),
		1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA,
		24,
		0, 0, 0, 0, 0, 0,
		8, 0,
		0, 0, 0, 0,
		24, 8,
		0,
		PFD_MAIN_PLANE,
		0,
		0,
		0,
		0,
	};
	int		pixelFormat;
	HDC hDC;
	HGLRC hGLRC;

	hDC = GetDC( hWnd );

	// Set up OpenGL
	pixelFormat = ChoosePixelFormat( hDC, &pfd );
	SetPixelFormat( hDC, pixelFormat, &pfd );
	hGLRC = wglCreateContext( hDC );
	wglMakeCurrent( hDC, hGLRC );

	// free things
	wglMakeCurrent( NULL, NULL );
	wglDeleteContext( hGLRC );
	ReleaseDC( hWnd, hDC );

	return DefWindowProc( hWnd, uMsg, wParam, lParam );
}

/*
==================
GLW_GetWGLExtensionsWithFakeWindow
==================
*/
// RB: replaced WGL with GLEW WGL
void GLW_CheckWGLExtensions( HDC hDC )
{
	GLenum glewResult = glewInit();
	if( GLEW_OK != glewResult )
	{
		// glewInit failed, something is seriously wrong
		common->Printf( "^GLW_CheckWGLExtensions() - GLEW could not load OpenGL subsystem: %s", glewGetErrorString( glewResult ) );
	}
	else
	{
		common->Printf( "Using GLEW %s\n", glewGetString( GLEW_VERSION ) );
	}

	if( WGLEW_ARB_extensions_string )
	{
		glConfig.wgl_extensions_string = ( const char* ) wglGetExtensionsStringARB( hDC );
	}
	else
	{
		glConfig.wgl_extensions_string = "";
	}

	// WGL_EXT_swap_control
	//wglSwapIntervalEXT = ( PFNWGLSWAPINTERVALEXTPROC ) GLimp_ExtensionPointer( "wglSwapIntervalEXT" );
	r_swapInterval.SetModified();	// force a set next frame

	// WGL_EXT_swap_control_tear
	glConfig.swapControlTearAvailable = WGLEW_EXT_swap_control_tear != 0;

	// FIXME
	// WGLEW_ARB_pixel_format
}
#endif

/*
==================
GLW_GetWGLExtensionsWithFakeWindow
==================
*/
#if !defined(USE_VULKAN) && !defined(USE_NVRHI)
static void GLW_GetWGLExtensionsWithFakeWindow()
{
	HWND	hWnd;
	MSG		msg;

	// Create a window for the sole purpose of getting
	// a valid context to get the wglextensions
	hWnd = CreateWindow( WIN32_FAKE_WINDOW_CLASS_NAME, GAME_NAME,
						 WS_OVERLAPPEDWINDOW,
						 40, 40,
						 640,
						 480,
						 NULL, NULL, win32.hInstance, NULL );
	if( !hWnd )
	{
		common->FatalError( "GLW_GetWGLExtensionsWithFakeWindow: Couldn't create fake window" );
	}

	HDC hDC = GetDC( hWnd );
	HGLRC gRC = wglCreateContext( hDC );
	wglMakeCurrent( hDC, gRC );
	GLW_CheckWGLExtensions( hDC );
	wglDeleteContext( gRC );
	ReleaseDC( hWnd, hDC );

	DestroyWindow( hWnd );
	while( GetMessage( &msg, NULL, 0, 0 ) )
	{
		TranslateMessage( &msg );
		DispatchMessage( &msg );
	}
}
#endif

/*
====================
GLW_WM_CREATE
====================
*/
void GLW_WM_CREATE( HWND hWnd )
{
}

/*
========================
CreateOpenGLContextOnDC
========================
*/
#if !defined(USE_VULKAN) && !defined(USE_NVRHI)
static HGLRC CreateOpenGLContextOnDC( const HDC hdc, const bool debugContext )
{
	int useCoreProfile = r_useOpenGL45.GetInteger();
	HGLRC m_hrc = NULL;

	// RB: for GLintercept 1.2.0 or otherwise we can't diff the framebuffers using the XML log
	if( !WGLEW_ARB_create_context || useCoreProfile == 0 )
	{
		return wglCreateContext( hdc );
	}
	// RB end

	for( int i = 0; i < 2; i++ )
	{
		const int glMajorVersion = ( useCoreProfile != 0 ) ? 4 : 4;
		const int glMinorVersion = ( useCoreProfile != 0 ) ? 5 : 3;
		const int glDebugFlag = debugContext ? WGL_CONTEXT_DEBUG_BIT_ARB : 0;
		const int glProfileMask = ( useCoreProfile != 0 ) ? WGL_CONTEXT_PROFILE_MASK_ARB : 0;
		const int glProfile = ( useCoreProfile == 1 ) ? WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB : ( ( useCoreProfile == 2 ) ? WGL_CONTEXT_CORE_PROFILE_BIT_ARB : 0 );
		const int attribs[] =
		{
			WGL_CONTEXT_MAJOR_VERSION_ARB,	glMajorVersion,
			WGL_CONTEXT_MINOR_VERSION_ARB,	glMinorVersion,
			WGL_CONTEXT_FLAGS_ARB,			glDebugFlag,
			glProfileMask,					glProfile,
			0
		};

		m_hrc = wglCreateContextAttribsARB( hdc, 0, attribs );
		if( m_hrc != NULL )
		{
			idLib::Printf( "created OpenGL %d.%d context\n", glMajorVersion, glMinorVersion );

			if( useCoreProfile == 2 )
			{
				glConfig.driverType = GLDRV_OPENGL32_CORE_PROFILE;
			}
			else if( useCoreProfile == 1 )
			{
				glConfig.driverType = GLDRV_OPENGL32_COMPATIBILITY_PROFILE;
			}
			else
			{
				glConfig.driverType = GLDRV_OPENGL3X;
			}

			break;
		}

		idLib::Printf( "failed to create OpenGL %d.%d context\n", glMajorVersion, glMinorVersion );
		useCoreProfile = 0;	// fall back to OpenGL 4.3
	}

	if( m_hrc == NULL )
	{
		int	err = GetLastError();
		switch( err )
		{
			case ERROR_INVALID_VERSION_ARB:
				idLib::Printf( "ERROR_INVALID_VERSION_ARB\n" );
				break;
			case ERROR_INVALID_PROFILE_ARB:
				idLib::Printf( "ERROR_INVALID_PROFILE_ARB\n" );
				break;
			default:
				idLib::Printf( "unknown error: 0x%x\n", err );
				break;
		}
	}

	return m_hrc;
}

/*
====================
GLW_ChoosePixelFormat

Returns -1 on failure, or a pixel format
====================
*/
static int GLW_ChoosePixelFormat( const HDC hdc, const int multisamples, const bool stereo3D )
{
	FLOAT	fAttributes[] = { 0, 0 };
	int		iAttributes[] =
	{
		WGL_SAMPLE_BUFFERS_ARB, ( ( multisamples > 1 ) ? 1 : 0 ),
		WGL_SAMPLES_ARB, multisamples,
		WGL_DOUBLE_BUFFER_ARB, TRUE,
		WGL_STENCIL_BITS_ARB, 8,
		WGL_DEPTH_BITS_ARB, 24,
		WGL_RED_BITS_ARB, 8,
		WGL_BLUE_BITS_ARB, 8,
		WGL_GREEN_BITS_ARB, 8,
		WGL_ALPHA_BITS_ARB, 8,
		WGL_STEREO_ARB, ( stereo3D ? TRUE : FALSE ),
		0, 0
	};

	int	pixelFormat;
	UINT numFormats;
	if( !wglChoosePixelFormatARB( hdc, iAttributes, fAttributes, 1, &pixelFormat, &numFormats ) )
	{
		return -1;
	}
	return pixelFormat;
}

/*
====================
GLW_InitDriver

Set the pixelformat for the window before it is
shown, and create the rendering context
====================
*/
static bool GLW_InitDriver( glimpParms_t parms )
{
	PIXELFORMATDESCRIPTOR src =
	{
		sizeof( PIXELFORMATDESCRIPTOR ),	// size of this pfd
		1,								// version number
		PFD_DRAW_TO_WINDOW |			// support window
		PFD_SUPPORT_OPENGL |			// support OpenGL
		PFD_DOUBLEBUFFER,				// double buffered
		PFD_TYPE_RGBA,					// RGBA type
		32,								// 32-bit color depth
		0, 0, 0, 0, 0, 0,				// color bits ignored
		8,								// 8 bit destination alpha
		0,								// shift bit ignored
		0,								// no accumulation buffer
		0, 0, 0, 0, 					// accum bits ignored
		24,								// 24-bit z-buffer
		8,								// 8-bit stencil buffer
		0,								// no auxiliary buffer
		PFD_MAIN_PLANE,					// main layer
		0,								// reserved
		0, 0, 0							// layer masks ignored
	};

	common->Printf( "Initializing OpenGL driver\n" );

	//
	// get a DC for our window if we don't already have one allocated
	//
	if( win32.hDC == NULL )
	{
		common->Printf( "...getting DC: " );

		if( ( win32.hDC = GetDC( win32.hWnd ) ) == NULL )
		{
			common->Printf( "^3failed^0\n" );
			return false;
		}
		common->Printf( "succeeded\n" );
	}

	// the multisample path uses the wgl
	if( WGLEW_ARB_pixel_format )
	{
		win32.pixelformat = GLW_ChoosePixelFormat( win32.hDC, parms.multiSamples, parms.stereo );
	}
	else
	{
		// this is the "classic" choose pixel format path
		common->Printf( "Using classic ChoosePixelFormat\n" );

		// eventually we may need to have more fallbacks, but for
		// now, ask for everything
		if( parms.stereo )
		{
			common->Printf( "...attempting to use stereo\n" );
			src.dwFlags |= PFD_STEREO;
		}

		//
		// choose, set, and describe our desired pixel format.  If we're
		// using a minidriver then we need to bypass the GDI functions,
		// otherwise use the GDI functions.
		//
		if( ( win32.pixelformat = ChoosePixelFormat( win32.hDC, &src ) ) == 0 )
		{
			common->Printf( "...^3GLW_ChoosePFD failed^0\n" );
			return false;
		}
		common->Printf( "...PIXELFORMAT %d selected\n", win32.pixelformat );
	}

	// get the full info
	DescribePixelFormat( win32.hDC, win32.pixelformat, sizeof( win32.pfd ), &win32.pfd );
	glConfig.colorBits = win32.pfd.cColorBits;
	glConfig.depthBits = win32.pfd.cDepthBits;
	glConfig.stencilBits = win32.pfd.cStencilBits;

	// XP seems to set this incorrectly
	if( !glConfig.stencilBits )
	{
		glConfig.stencilBits = 8;
	}

	// the same SetPixelFormat is used either way
	if( SetPixelFormat( win32.hDC, win32.pixelformat, &win32.pfd ) == FALSE )
	{
		common->Printf( "...^3SetPixelFormat failed^0\n", win32.hDC );
		return false;
	}

	//
	// startup the OpenGL subsystem by creating a context and making it current
	//
	common->Printf( "...creating GL context: " );
	win32.hGLRC = CreateOpenGLContextOnDC( win32.hDC, r_debugContext.GetBool() );
	if( win32.hGLRC == 0 )
	{
		common->Printf( "^3failed^0\n" );
		return false;
	}
	common->Printf( "succeeded\n" );

	common->Printf( "...making context current: " );
	if( !wglMakeCurrent( win32.hDC, win32.hGLRC ) )
	{
		wglDeleteContext( win32.hGLRC );
		win32.hGLRC = NULL;
		common->Printf( "^3failed^0\n" );
		return false;
	}
	common->Printf( "succeeded\n" );

	return true;
}
#endif

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

#if !defined(USE_VULKAN) && !defined(USE_NVRHI)
	// now register the fake window class that is only used
	// to get wgl extensions
	wc.style         = 0;
	wc.lpfnWndProc   = ( WNDPROC ) FakeWndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = win32.hInstance;
	wc.hIcon         = LoadIcon( win32.hInstance, MAKEINTRESOURCE( IDI_ICON1 ) );
	wc.hCursor       = LoadCursor( NULL, IDC_ARROW );
	wc.hbrBackground = ( struct HBRUSH__* )COLOR_GRAYTEXT;
	wc.lpszMenuName  = 0;
	wc.lpszClassName = WIN32_FAKE_WINDOW_CLASS_NAME;

	if( !RegisterClass( &wc ) )
	{
		common->FatalError( "GLW_CreateWindow: could not register window class" );
	}
	common->Printf( "...registered fake window class\n" );
#endif

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
	if( !( device.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP ) )
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
		if( !( device.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP ) )
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
			if( ( devmode.dmDisplayFrequency != 60 ) && ( devmode.dmDisplayFrequency != 120 ) )
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
	deviceParms.backBufferWidth = parms.width;
	deviceParms.backBufferHeight = parms.height;
	deviceParms.backBufferSampleCount = parms.multiSamples;
	deviceParms.vsyncEnabled = requestedVSync;

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

#if !defined(USE_VULKAN) && !defined(USE_NVRHI)
	// Check to see if we can get a stereo pixel format, even if we aren't going to use it,
	// so the menu option can be
	if( GLW_ChoosePixelFormat( win32.hDC, parms.multiSamples, true ) != -1 )
	{
		glConfig.stereoPixelFormatAvailable = true;
	}
	else
	{
		glConfig.stereoPixelFormatAvailable = false;
	}

	if( !GLW_InitDriver( parms ) )
	{
		ShowWindow( win32.hWnd, SW_HIDE );
		DestroyWindow( win32.hWnd );
		win32.hWnd = NULL;
		return false;
	}
#endif

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

#if !defined(USE_VULKAN) && !defined(USE_NVRHI)
	cmdSystem->AddCommand( "testSwapBuffers", GLimp_TestSwapBuffers, CMD_FL_SYSTEM, "Times swapbuffer options" );

	common->Printf( "Initializing OpenGL subsystem with multisamples:%i stereo:%i fullscreen:%i\n",
					parms.multiSamples, parms.stereo, parms.fullScreen );
#endif

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

#if !defined( USE_VULKAN ) && !defined( USE_NVRHI )
	// this will load the dll and set all our gl* function pointers,
	// but doesn't create a window

	// getting the wgl extensions involves creating a fake window to get a context,
	// which is pretty disgusting, and seems to mess with the AGP VAR allocation
	GLW_GetWGLExtensionsWithFakeWindow();
#endif

	// Optionally ChangeDisplaySettings to get a different fullscreen resolution.
	if( !GLW_ChangeDislaySettingsIfNeeded( parms ) )
	{
		GLimp_Shutdown();
		return false;
	}

	// try to create a window with the correct pixel format
	// and init the renderer context
#if defined( USE_NVRHI )
	if( !deviceManager->CreateWindowDeviceAndSwapChain( parms, GAME_NAME ) )
#else
	if( !GLW_CreateWindow( parms ) )
#endif
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

	// RB: we probably have a new OpenGL 3.2 core context so reinitialize GLEW
#if !defined( USE_VULKAN ) && !defined( USE_NVRHI )
	GLenum glewResult = glewInit();
	if( GLEW_OK != glewResult )
	{
		// glewInit failed, something is seriously wrong
		common->Printf( "^3GLimp_Init() - GLEW could not load OpenGL subsystem: %s", glewGetErrorString( glewResult ) );
	}
	else
	{
		common->Printf( "Using GLEW %s\n", glewGetString( GLEW_VERSION ) );
	}
#endif

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
#if defined( USE_NVRHI )
	if( deviceManager )
	{
		deviceManager->Shutdown();
	}
#else
	const char* success[] = { "failed", "success" };
	int retVal;

#if defined(USE_VULKAN)
	// TODO
#else
	common->Printf( "Shutting down OpenGL subsystem\n" );

	// set current context to NULL
	//if( wglMakeCurrent )
	{
		retVal = wglMakeCurrent( NULL, NULL ) != 0;
		common->Printf( "...wglMakeCurrent( NULL, NULL ): %s\n", success[retVal] );
	}

	// delete HGLRC
	if( win32.hGLRC )
	{
		retVal = wglDeleteContext( win32.hGLRC ) != 0;
		common->Printf( "...deleting GL context: %s\n", success[retVal] );
		win32.hGLRC = NULL;
	}
#endif

	// release DC
	if( win32.hDC )
	{
		retVal = ReleaseDC( win32.hWnd, win32.hDC ) != 0;
		common->Printf( "...releasing DC: %s\n", success[retVal] );
		win32.hDC   = NULL;
	}

	// destroy window
	if( win32.hWnd )
	{
		common->Printf( "...destroying window\n" );
		ShowWindow( win32.hWnd, SW_HIDE );
		DestroyWindow( win32.hWnd );
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
#endif
}

/*
=====================
GLimp_SwapBuffers
=====================
*/
// RB: use GLEW for V-Sync
#if !defined(USE_VULKAN) && !defined(USE_NVRHI)
void GLimp_SwapBuffers()
{
	if( r_swapInterval.IsModified() )
	{
		r_swapInterval.ClearModified();

		int interval = 0;
		if( r_swapInterval.GetInteger() == 1 )
		{
			interval = ( glConfig.swapControlTearAvailable ) ? -1 : 1;
		}
		else if( r_swapInterval.GetInteger() == 2 )
		{
			interval = 1;
		}

		if( WGLEW_EXT_swap_control )
		{
			wglSwapIntervalEXT( interval );
		}
	}

	SwapBuffers( win32.hDC );
}
#endif




