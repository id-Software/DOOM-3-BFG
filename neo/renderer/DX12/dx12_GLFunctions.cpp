#pragma hdrstop

#include "../../idlib/precompiled.h"

#include "../../sys/win32/win_local.h"
#include "../../sys/win32/rc/doom_resource.h"
#include "../tr_local.h"

extern Win32Vars_t	win32;

/*
========================
GetDeviceName
========================
*/
static idStr GetDeviceName(const int deviceNum) {
	DISPLAY_DEVICE	device = {};
	device.cb = sizeof(device);
	if (!EnumDisplayDevices(
		0,			// lpDevice
		deviceNum,
		&device,
		0 /* dwFlags */)) {
		return false;
	}

	// get the monitor for this display
	if (!(device.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)) {
		return false;
	}

	return idStr(device.DeviceName);
}

/*
========================
GetDisplayCoordinates
========================
*/
static bool GetDisplayCoordinates(const int deviceNum, int& x, int& y, int& width, int& height, int& displayHz) {
	idStr deviceName = GetDeviceName(deviceNum);
	if (deviceName.Length() == 0) {
		return false;
	}

	DISPLAY_DEVICE	device = {};
	device.cb = sizeof(device);
	if (!EnumDisplayDevices(
		0,			// lpDevice
		deviceNum,
		&device,
		0 /* dwFlags */)) {
		return false;
	}

	DISPLAY_DEVICE	monitor;
	monitor.cb = sizeof(monitor);
	if (!EnumDisplayDevices(
		deviceName.c_str(),
		0,
		&monitor,
		0 /* dwFlags */)) {
		return false;
	}

	DEVMODE	devmode;
	devmode.dmSize = sizeof(devmode);
	if (!EnumDisplaySettings(deviceName.c_str(), ENUM_CURRENT_SETTINGS, &devmode)) {
		return false;
	}

	common->Printf("display device: %i\n", deviceNum);
	common->Printf("  DeviceName  : %s\n", device.DeviceName);
	common->Printf("  DeviceString: %s\n", device.DeviceString);
	common->Printf("  StateFlags  : 0x%x\n", device.StateFlags);
	common->Printf("  DeviceID    : %s\n", device.DeviceID);
	common->Printf("  DeviceKey   : %s\n", device.DeviceKey);
	common->Printf("      DeviceName  : %s\n", monitor.DeviceName);
	common->Printf("      DeviceString: %s\n", monitor.DeviceString);
	common->Printf("      StateFlags  : 0x%x\n", monitor.StateFlags);
	common->Printf("      DeviceID    : %s\n", monitor.DeviceID);
	common->Printf("      DeviceKey   : %s\n", monitor.DeviceKey);
	common->Printf("          dmPosition.x      : %i\n", devmode.dmPosition.x);
	common->Printf("          dmPosition.y      : %i\n", devmode.dmPosition.y);
	common->Printf("          dmBitsPerPel      : %i\n", devmode.dmBitsPerPel);
	common->Printf("          dmPelsWidth       : %i\n", devmode.dmPelsWidth);
	common->Printf("          dmPelsHeight      : %i\n", devmode.dmPelsHeight);
	common->Printf("          dmDisplayFlags    : 0x%x\n", devmode.dmDisplayFlags);
	common->Printf("          dmDisplayFrequency: %i\n", devmode.dmDisplayFrequency);

	x = devmode.dmPosition.x;
	y = devmode.dmPosition.y;
	width = devmode.dmPelsWidth;
	height = devmode.dmPelsHeight;
	displayHz = devmode.dmDisplayFrequency;

	return true;
}

/*
====================
GLW_GetWindowDimensions
====================
*/
static bool GLW_GetWindowDimensions(const glimpParms_t parms, int& x, int& y, int& w, int& h) {
	//
	// compute width and height
	//
	if (parms.fullScreen != 0) {
		if (parms.fullScreen == -1) {
			// borderless window at specific location, as for spanning
			// multiple monitor outputs
			x = parms.x;
			y = parms.y;
			w = parms.width;
			h = parms.height;
		}
		else {
			// get the current monitor position and size on the desktop, assuming
			// any required ChangeDisplaySettings has already been done
			int displayHz = 0;
			if (!GetDisplayCoordinates(parms.fullScreen - 1, x, y, w, h, displayHz)) {
				return false;
			}
		}
	}
	else {
		RECT	r;

		// adjust width and height for window border
		r.bottom = parms.height;
		r.left = 0;
		r.top = 0;
		r.right = parms.width;

		AdjustWindowRect(&r, WINDOW_STYLE | WS_SYSMENU, FALSE);

		w = r.right - r.left;
		h = r.bottom - r.top;

		x = parms.x;
		y = parms.y;
	}

	return true;
}

/*
=======================
GLW_CreateWindow
Responsible for creating the Win32 window.
If fullscreen, it won't have a border
=======================
*/
static bool GLW_CreateWindow(glimpParms_t parms) {
	int				x, y, w, h;
	if (!GLW_GetWindowDimensions(parms, x, y, w, h)) {
		return false;
	}

	int				stylebits;
	int				exstyle;
	if (parms.fullScreen != 0) {
		exstyle = WS_EX_TOPMOST;
		stylebits = WS_POPUP | WS_VISIBLE | WS_SYSMENU;
	}
	else {
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
		NULL);

	if (!win32.hWnd) {
		common->Printf("^3GLW_CreateWindow() - Couldn't create window^0\n");
		return false;
	}

	::SetTimer(win32.hWnd, 0, 100, NULL);

	ShowWindow(win32.hWnd, SW_SHOW);
	UpdateWindow(win32.hWnd);
	common->Printf("...created window @ %d,%d (%dx%d)\n", x, y, w, h);

	// makeCurrent NULL frees the DC, so get another
	win32.hDC = GetDC(win32.hWnd);
	if (!win32.hDC) {
		common->Printf("^3GLW_CreateWindow() - GetDC()failed^0\n");
		return false;
	}

	// Check to see if we can get a stereo pixel format, even if we aren't going to use it,
	// so the menu option can be 
	glConfig.stereoPixelFormatAvailable = false;

	/*if (!GLW_InitDriver(parms)) {
		ShowWindow(win32.hWnd, SW_HIDE);
		DestroyWindow(win32.hWnd);
		win32.hWnd = NULL;
		return false;
	}*/

	SetForegroundWindow(win32.hWnd);
	SetFocus(win32.hWnd);

	glConfig.isFullscreen = parms.fullScreen;

	return true;
}

/*
====================
GLW_CreateWindowClasses
====================
*/
static void GLW_CreateWindowClasses() {
	WNDCLASS wc;

	//
	// register the window class if necessary
	//
	if (win32.windowClassRegistered) {
		return;
	}

	memset(&wc, 0, sizeof(wc));

	wc.style = 0;
	wc.lpfnWndProc = (WNDPROC)MainWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = win32.hInstance;
	wc.hIcon = LoadIcon(win32.hInstance, MAKEINTRESOURCE(IDI_ICON1));
	wc.hCursor = NULL;
	wc.hbrBackground = (struct HBRUSH__*)COLOR_GRAYTEXT;
	wc.lpszMenuName = 0;
	wc.lpszClassName = WIN32_WINDOW_CLASS_NAME;

	if (!RegisterClass(&wc)) {
		common->FatalError("GLW_CreateWindow: could not register window class");
	}
	common->Printf("...registered window class\n");

	win32.windowClassRegistered = true;
}

bool GLimp_Init(glimpParms_t parms) {
	const char* driverName;
	HDC		hDC;

	common->Printf("Initializing DirectX12 subsystem with multisamples:%i stereo:%i fullscreen:%i\n",
		parms.multiSamples, parms.stereo, parms.fullScreen);

	// check our desktop attributes
	hDC = GetDC(GetDesktopWindow());
	win32.desktopBitsPixel = GetDeviceCaps(hDC, BITSPIXEL);
	win32.desktopWidth = GetDeviceCaps(hDC, HORZRES);
	win32.desktopHeight = GetDeviceCaps(hDC, VERTRES);
	ReleaseDC(GetDesktopWindow(), hDC);

	// we can't run in a window unless it is 32 bpp
	if (win32.desktopBitsPixel < 32 && parms.fullScreen <= 0) {
		common->Printf("^3Windowed mode requires 32 bit desktop depth^0\n");
		return false;
	}

	// create our window classes if we haven't already
	GLW_CreateWindowClasses();

	// TODO: Implement
	// Optionally ChangeDisplaySettings to get a different fullscreen resolution.
	/*if (!GLW_ChangeDislaySettingsIfNeeded(parms)) {
		GLimp_Shutdown();
		return false;
	}*/

	// try to create a window with the correct pixel format
	// and init the renderer context
	if (!GLW_CreateWindow(parms)) {
		GLimp_Shutdown();
		return false;
	}

	glConfig.isFullscreen = parms.fullScreen;
	glConfig.isStereoPixelFormat = parms.stereo;
	glConfig.nativeScreenWidth = parms.width;
	glConfig.nativeScreenHeight = parms.height;
	glConfig.multisamples = parms.multiSamples;

	glConfig.pixelAspect = 1.0f;	// FIXME: some monitor modes may be distorted
									// should side-by-side stereo modes be consider aspect 0.5?

	// TODO: Get device name.
	// get the screen size, which may not be reliable...
	// If we use the windowDC, I get my 30" monitor, even though the window is
	// on a 27" monitor, so get a dedicated DC for the full screen device name.
	const idStr deviceName = GetDeviceName(Max(0, parms.fullScreen - 1));

	HDC deviceDC = CreateDC(deviceName.c_str(), deviceName.c_str(), NULL, NULL);
	const int mmWide = GetDeviceCaps(win32.hDC, HORZSIZE);
	DeleteDC(deviceDC);

	if (mmWide == 0) {
		glConfig.physicalScreenWidthInCentimeters = 100.0f;
	}
	else {
		glConfig.physicalScreenWidthInCentimeters = 0.1f * mmWide;
	}

	return true;
}

/*
===================
GLimp_Shutdown
This routine does all OS specific shutdown procedures for the OpenGL
subsystem.
===================
*/
void GLimp_Shutdown() {
	const char* success[] = { "failed", "success" };
	int retVal;

	common->Printf("Shutting down DirectX subsystem\n");

	// release DC
	if (win32.hDC) {
		retVal = ReleaseDC(win32.hWnd, win32.hDC) != 0;
		common->Printf("...releasing DC: %s\n", success[retVal]);
		win32.hDC = NULL;
	}

	// destroy window
	if (win32.hWnd) {
		common->Printf("...destroying window\n");
		ShowWindow(win32.hWnd, SW_HIDE);
		DestroyWindow(win32.hWnd);
		win32.hWnd = NULL;
	}

	// reset display settings
	if (win32.cdsFullscreen) {
		common->Printf("...resetting display\n");
		ChangeDisplaySettings(0, 0);
		win32.cdsFullscreen = 0;
	}

	// close the thread so the handle doesn't dangle
	if (win32.renderThreadHandle) {
		common->Printf("...closing smp thread\n");
		CloseHandle(win32.renderThreadHandle);
		win32.renderThreadHandle = NULL;
	}

	dxRenderer.OnDestroy();
}