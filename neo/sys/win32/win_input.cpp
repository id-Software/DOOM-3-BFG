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

#pragma hdrstop
#include "precompiled.h"
#include "../sys_session_local.h"

#include "win_local.h"

#define DINPUT_BUFFERSIZE           256

/*
============================================================

DIRECT INPUT KEYBOARD CONTROL

============================================================
*/

bool IN_StartupKeyboard()
{
	HRESULT hr;
	bool    bExclusive;
	bool    bForeground;
	bool    bImmediate;
	bool    bDisableWindowsKey;
	DWORD   dwCoopFlags;
	
	if( !win32.g_pdi )
	{
		common->Printf( "keyboard: DirectInput has not been started\n" );
		return false;
	}
	
	if( win32.g_pKeyboard )
	{
		win32.g_pKeyboard->Release();
		win32.g_pKeyboard = NULL;
	}
	
	// Detrimine where the buffer would like to be allocated
	bExclusive         = false;
	bForeground        = true;
	bImmediate         = false;
	bDisableWindowsKey = true;
	
	if( bExclusive )
		dwCoopFlags = DISCL_EXCLUSIVE;
	else
		dwCoopFlags = DISCL_NONEXCLUSIVE;
		
	if( bForeground )
		dwCoopFlags |= DISCL_FOREGROUND;
	else
		dwCoopFlags |= DISCL_BACKGROUND;
		
	// Disabling the windows key is only allowed only if we are in foreground nonexclusive
	if( bDisableWindowsKey && !bExclusive && bForeground )
		dwCoopFlags |= DISCL_NOWINKEY;
		
	// Obtain an interface to the system keyboard device.
	if( FAILED( hr = win32.g_pdi->CreateDevice( GUID_SysKeyboard, &win32.g_pKeyboard, NULL ) ) )
	{
		common->Printf( "keyboard: couldn't find a keyboard device\n" );
		return false;
	}
	
	// Set the data format to "keyboard format" - a predefined data format
	//
	// A data format specifies which controls on a device we
	// are interested in, and how they should be reported.
	//
	// This tells DirectInput that we will be passing an array
	// of 256 bytes to IDirectInputDevice::GetDeviceState.
	if( FAILED( hr = win32.g_pKeyboard->SetDataFormat( &c_dfDIKeyboard ) ) )
		return false;
		
	// Set the cooperativity level to let DirectInput know how
	// this device should interact with the system and with other
	// DirectInput applications.
	hr = win32.g_pKeyboard->SetCooperativeLevel( win32.hWnd, dwCoopFlags );
	if( hr == DIERR_UNSUPPORTED && !bForeground && bExclusive )
	{
		common->Printf( "keyboard: SetCooperativeLevel() returned DIERR_UNSUPPORTED.\nFor security reasons, background exclusive keyboard access is not allowed.\n" );
		return false;
	}
	
	if( FAILED( hr ) )
	{
		return false;
	}
	
	if( !bImmediate )
	{
		// IMPORTANT STEP TO USE BUFFERED DEVICE DATA!
		//
		// DirectInput uses unbuffered I/O (buffer size = 0) by default.
		// If you want to read buffered data, you need to set a nonzero
		// buffer size.
		//
		// Set the buffer size to DINPUT_BUFFERSIZE (defined above) elements.
		//
		// The buffer size is a DWORD property associated with the device.
		DIPROPDWORD dipdw;
		
		dipdw.diph.dwSize       = sizeof( DIPROPDWORD );
		dipdw.diph.dwHeaderSize = sizeof( DIPROPHEADER );
		dipdw.diph.dwObj        = 0;
		dipdw.diph.dwHow        = DIPH_DEVICE;
		dipdw.dwData            = DINPUT_BUFFERSIZE; // Arbitary buffer size
		
		if( FAILED( hr = win32.g_pKeyboard->SetProperty( DIPROP_BUFFERSIZE, &dipdw.diph ) ) )
			return false;
	}
	
	// Acquire the newly created device
	win32.g_pKeyboard->Acquire();
	
	common->Printf( "keyboard: DirectInput initialized.\n" );
	return true;
}

/*
==========================
IN_DeactivateKeyboard
==========================
*/
void IN_DeactivateKeyboard()
{
	if( !win32.g_pKeyboard )
	{
		return;
	}
	win32.g_pKeyboard->Unacquire( );
}

/*
============================================================

DIRECT INPUT MOUSE CONTROL

============================================================
*/

/*
========================
IN_InitDirectInput
========================
*/

void IN_InitDirectInput()
{
	HRESULT		hr;
	
	common->Printf( "Initializing DirectInput...\n" );
	
	if( win32.g_pdi != NULL )
	{
		win32.g_pdi->Release();			// if the previous window was destroyed we need to do this
		win32.g_pdi = NULL;
	}
	
	// Register with the DirectInput subsystem and get a pointer
	// to a IDirectInput interface we can use.
	// Create the base DirectInput object
	if( FAILED( hr = DirectInput8Create( GetModuleHandle( NULL ), DIRECTINPUT_VERSION, IID_IDirectInput8, ( void** )&win32.g_pdi, NULL ) ) )
	{
		common->Printf( "DirectInputCreate failed\n" );
	}
}

/*
========================
IN_InitDIMouse
========================
*/
bool IN_InitDIMouse()
{
	HRESULT		hr;
	
	if( win32.g_pdi == NULL )
	{
		return false;
	}
	
	// obtain an interface to the system mouse device.
	hr = win32.g_pdi->CreateDevice( GUID_SysMouse, &win32.g_pMouse, NULL );
	
	if( FAILED( hr ) )
	{
		common->Printf( "mouse: Couldn't open DI mouse device\n" );
		return false;
	}
	
	// Set the data format to "mouse format" - a predefined data format
	//
	// A data format specifies which controls on a device we
	// are interested in, and how they should be reported.
	//
	// This tells DirectInput that we will be passing a
	// DIMOUSESTATE2 structure to IDirectInputDevice::GetDeviceState.
	if( FAILED( hr = win32.g_pMouse->SetDataFormat( &c_dfDIMouse2 ) ) )
	{
		common->Printf( "mouse: Couldn't set DI mouse format\n" );
		return false;
	}
	
	// set the cooperativity level.
	hr = win32.g_pMouse->SetCooperativeLevel( win32.hWnd, DISCL_EXCLUSIVE | DISCL_FOREGROUND );
	
	if( FAILED( hr ) )
	{
		common->Printf( "mouse: Couldn't set DI coop level\n" );
		return false;
	}
	
	
	// IMPORTANT STEP TO USE BUFFERED DEVICE DATA!
	//
	// DirectInput uses unbuffered I/O (buffer size = 0) by default.
	// If you want to read buffered data, you need to set a nonzero
	// buffer size.
	//
	// Set the buffer size to SAMPLE_BUFFER_SIZE (defined above) elements.
	//
	// The buffer size is a DWORD property associated with the device.
	DIPROPDWORD dipdw;
	dipdw.diph.dwSize       = sizeof( DIPROPDWORD );
	dipdw.diph.dwHeaderSize = sizeof( DIPROPHEADER );
	dipdw.diph.dwObj        = 0;
	dipdw.diph.dwHow        = DIPH_DEVICE;
	dipdw.dwData            = DINPUT_BUFFERSIZE; // Arbitary buffer size
	
	if( FAILED( hr = win32.g_pMouse->SetProperty( DIPROP_BUFFERSIZE, &dipdw.diph ) ) )
	{
		common->Printf( "mouse: Couldn't set DI buffersize\n" );
		return false;
	}
	
	IN_ActivateMouse();
	
	// clear any pending samples
	int	mouseEvents[MAX_MOUSE_EVENTS][2];
	Sys_PollMouseInputEvents( mouseEvents );
	
	common->Printf( "mouse: DirectInput initialized.\n" );
	return true;
}


/*
==========================
IN_ActivateMouse
==========================
*/
void IN_ActivateMouse()
{
	int i;
	HRESULT hr;
	
	if( !win32.in_mouse.GetBool() || win32.mouseGrabbed || !win32.g_pMouse )
	{
		return;
	}
	
	win32.mouseGrabbed = true;
	for( i = 0; i < 10; i++ )
	{
		if( ::ShowCursor( false ) < 0 )
		{
			break;
		}
	}
	
	// we may fail to reacquire if the window has been recreated
	hr = win32.g_pMouse->Acquire();
	if( FAILED( hr ) )
	{
		return;
	}
	
	// set the cooperativity level.
	hr = win32.g_pMouse->SetCooperativeLevel( win32.hWnd, DISCL_EXCLUSIVE | DISCL_FOREGROUND );
}

/*
==========================
IN_DeactivateMouse
==========================
*/
void IN_DeactivateMouse()
{
	int i;
	
	if( !win32.g_pMouse || !win32.mouseGrabbed )
	{
		return;
	}
	
	win32.g_pMouse->Unacquire();
	
	for( i = 0; i < 10; i++ )
	{
		if( ::ShowCursor( true ) >= 0 )
		{
			break;
		}
	}
	win32.mouseGrabbed = false;
}

/*
==========================
IN_DeactivateMouseIfWindowed
==========================
*/
void IN_DeactivateMouseIfWindowed()
{
	if( !win32.cdsFullscreen )
	{
		IN_DeactivateMouse();
	}
}

/*
============================================================

  MOUSE CONTROL

============================================================
*/


/*
===========
Sys_ShutdownInput
===========
*/
void Sys_ShutdownInput()
{
	IN_DeactivateMouse();
	IN_DeactivateKeyboard();
	if( win32.g_pKeyboard )
	{
		win32.g_pKeyboard->Release();
		win32.g_pKeyboard = NULL;
	}
	
	if( win32.g_pMouse )
	{
		win32.g_pMouse->Release();
		win32.g_pMouse = NULL;
	}
	
	if( win32.g_pdi )
	{
		win32.g_pdi->Release();
		win32.g_pdi = NULL;
	}
}

/*
===========
Sys_InitInput
===========
*/
void Sys_InitInput()
{
	common->Printf( "\n------- Input Initialization -------\n" );
	IN_InitDirectInput();
	if( win32.in_mouse.GetBool() )
	{
		IN_InitDIMouse();
		// don't grab the mouse on initialization
		Sys_GrabMouseCursor( false );
	}
	else
	{
		common->Printf( "Mouse control not active.\n" );
	}
	IN_StartupKeyboard();
	
	common->Printf( "------------------------------------\n" );
	win32.in_mouse.ClearModified();
}

/*
==================
IN_Frame

Called every frame, even if not generating commands
==================
*/
void IN_Frame()
{
	bool	shouldGrab = true;
	
	if( !win32.in_mouse.GetBool() )
	{
		shouldGrab = false;
	}
	// if fullscreen, we always want the mouse
	if( !win32.cdsFullscreen )
	{
		if( win32.mouseReleased )
		{
			shouldGrab = false;
		}
		if( win32.movingWindow )
		{
			shouldGrab = false;
		}
		if( !win32.activeApp )
		{
			shouldGrab = false;
		}
	}
	
	if( shouldGrab != win32.mouseGrabbed )
	{
		if( usercmdGen != NULL )
		{
			usercmdGen->Clear();
		}
		
		if( win32.mouseGrabbed )
		{
			IN_DeactivateMouse();
		}
		else
		{
			IN_ActivateMouse();
			
#if 0	// if we can't reacquire, try reinitializing
			if( !IN_InitDIMouse() )
			{
				win32.in_mouse.SetBool( false );
				return;
			}
#endif
		}
	}
}


void	Sys_GrabMouseCursor( bool grabIt )
{
	win32.mouseReleased = !grabIt;
	if( !grabIt )
	{
		// release it right now
		IN_Frame();
	}
}

//=====================================================================================

static DIDEVICEOBJECTDATA polled_didod[ DINPUT_BUFFERSIZE ];  // Receives buffered data

static int diFetch;
static byte toggleFetch[2][ 256 ];


#if 1
// I tried doing the full-state get to address a keyboard problem on one system,
// but it didn't make any difference

/*
====================
Sys_PollKeyboardInputEvents
====================
*/
int Sys_PollKeyboardInputEvents()
{
	DWORD              dwElements;
	HRESULT            hr;
	
	if( win32.g_pKeyboard == NULL )
	{
		return 0;
	}
	
	dwElements = DINPUT_BUFFERSIZE;
	hr = win32.g_pKeyboard->GetDeviceData( sizeof( DIDEVICEOBJECTDATA ),
										   polled_didod, &dwElements, 0 );
	if( hr != DI_OK )
	{
		// We got an error or we got DI_BUFFEROVERFLOW.
		//
		// Either way, it means that continuous contact with the
		// device has been lost, either due to an external
		// interruption, or because the buffer overflowed
		// and some events were lost.
		hr = win32.g_pKeyboard->Acquire();
		
		
		
		// nuke the garbage
		if( !FAILED( hr ) )
		{
			//Bug 951: The following command really clears the garbage input.
			//The original will still process keys in the buffer and was causing
			//some problems.
			win32.g_pKeyboard->GetDeviceData( sizeof( DIDEVICEOBJECTDATA ), NULL, &dwElements, 0 );
			dwElements = 0;
		}
		// hr may be DIERR_OTHERAPPHASPRIO or other errors.  This
		// may occur when the app is minimized or in the process of
		// switching, so just try again later
	}
	
	if( FAILED( hr ) )
	{
		return 0;
	}
	
	return dwElements;
}

#else

/*
====================
Sys_PollKeyboardInputEvents

Fake events by getting the entire device state
and checking transitions
====================
*/
int Sys_PollKeyboardInputEvents()
{
	HRESULT            hr;
	
	if( win32.g_pKeyboard == NULL )
	{
		return 0;
	}
	
	hr = win32.g_pKeyboard->GetDeviceState( sizeof( toggleFetch[ diFetch ] ), toggleFetch[ diFetch ] );
	if( hr != DI_OK )
	{
		// We got an error or we got DI_BUFFEROVERFLOW.
		//
		// Either way, it means that continuous contact with the
		// device has been lost, either due to an external
		// interruption, or because the buffer overflowed
		// and some events were lost.
		hr = win32.g_pKeyboard->Acquire();
		
		// nuke the garbage
		if( !FAILED( hr ) )
		{
			hr = win32.g_pKeyboard->GetDeviceState( sizeof( toggleFetch[ diFetch ] ), toggleFetch[ diFetch ] );
		}
		// hr may be DIERR_OTHERAPPHASPRIO or other errors.  This
		// may occur when the app is minimized or in the process of
		// switching, so just try again later
	}
	
	if( FAILED( hr ) )
	{
		return 0;
	}
	
	// build faked events
	int		numChanges = 0;
	
	for( int i = 0 ; i < 256 ; i++ )
	{
		if( toggleFetch[0][i] != toggleFetch[1][i] )
		{
			polled_didod[ numChanges ].dwOfs = i;
			polled_didod[ numChanges ].dwData = toggleFetch[ diFetch ][i] ? 0x80 : 0;
			numChanges++;
		}
	}
	
	diFetch ^= 1;
	
	return numChanges;
}

#endif

/*
====================
Sys_PollKeyboardInputEvents
====================
*/
int Sys_ReturnKeyboardInputEvent( const int n, int& ch, bool& state )
{
	ch = polled_didod[ n ].dwOfs;
	state = ( polled_didod[ n ].dwData & 0x80 ) == 0x80;
	if( ch == K_PRINTSCREEN || ch == K_LCTRL || ch == K_LALT || ch == K_RCTRL || ch == K_RALT )
	{
		// for windows, add a keydown event for print screen here, since
		// windows doesn't send keydown events to the WndProc for this key.
		// ctrl and alt are handled here to get around windows sending ctrl and
		// alt messages when the right-alt is pressed on non-US 102 keyboards.
		Sys_QueEvent( SE_KEY, ch, state, 0, NULL, 0 );
	}
	return ch;
}


void Sys_EndKeyboardInputEvents()
{
}

//=====================================================================================


int Sys_PollMouseInputEvents( int mouseEvents[MAX_MOUSE_EVENTS][2] )
{
	DWORD				dwElements;
	HRESULT				hr;
	
	if( !win32.g_pMouse || !win32.mouseGrabbed )
	{
		return 0;
	}
	
	dwElements = DINPUT_BUFFERSIZE;
	hr = win32.g_pMouse->GetDeviceData( sizeof( DIDEVICEOBJECTDATA ), polled_didod, &dwElements, 0 );
	
	if( hr != DI_OK )
	{
		hr = win32.g_pMouse->Acquire();
		// clear the garbage
		if( !FAILED( hr ) )
		{
			win32.g_pMouse->GetDeviceData( sizeof( DIDEVICEOBJECTDATA ), polled_didod, &dwElements, 0 );
		}
	}
	
	if( FAILED( hr ) )
	{
		return 0;
	}
	
	if( dwElements > MAX_MOUSE_EVENTS )
	{
		dwElements = MAX_MOUSE_EVENTS;
	}
	
	for( DWORD i = 0; i < dwElements; i++ )
	{
		mouseEvents[i][0] = M_INVALID;
		mouseEvents[i][1] = 0;
		
		if( polled_didod[i].dwOfs >= DIMOFS_BUTTON0 && polled_didod[i].dwOfs <= DIMOFS_BUTTON7 )
		{
			const int mouseButton = ( polled_didod[i].dwOfs - DIMOFS_BUTTON0 );
			const bool mouseDown = ( polled_didod[i].dwData & 0x80 ) == 0x80;
			mouseEvents[i][0] = M_ACTION1 + mouseButton;
			mouseEvents[i][1] = mouseDown;
			Sys_QueEvent( SE_KEY, K_MOUSE1 + mouseButton, mouseDown, 0, NULL, 0 );
		}
		else
		{
			// RB: replaced switch enum for MinGW
			int diaction = polled_didod[i].dwOfs;
			
			if( diaction == DIMOFS_X )
			{
				mouseEvents[i][0] = M_DELTAX;
				mouseEvents[i][1] = polled_didod[i].dwData;
				Sys_QueEvent( SE_MOUSE, polled_didod[i].dwData, 0, 0, NULL, 0 );
			}
			else if( diaction == DIMOFS_Y )
			{
				mouseEvents[i][0] = M_DELTAY;
				mouseEvents[i][1] = polled_didod[i].dwData;
				Sys_QueEvent( SE_MOUSE, 0, polled_didod[i].dwData, 0, NULL, 0 );
			}
			else if( diaction == DIMOFS_Z )
			{
				mouseEvents[i][0] = M_DELTAZ;
				mouseEvents[i][1] = ( int )polled_didod[i].dwData / WHEEL_DELTA;
				{
					const int value = ( int )polled_didod[i].dwData / WHEEL_DELTA;
					const int key = value < 0 ? K_MWHEELDOWN : K_MWHEELUP;
					const int iterations = abs( value );
					for( int i = 0; i < iterations; i++ )
					{
						Sys_QueEvent( SE_KEY, key, true, 0, NULL, 0 );
						Sys_QueEvent( SE_KEY, key, false, 0, NULL, 0 );
					}
				}
			}
			// RB end
		}
	}
	
	return dwElements;
}

//=====================================================================================
//	Joystick Input Handling
//=====================================================================================

void Sys_SetRumble( int device, int low, int hi )
{
	return win32.g_Joystick.SetRumble( device, low, hi );
}

int Sys_PollJoystickInputEvents( int deviceNum )
{
	return win32.g_Joystick.PollInputEvents( deviceNum );
}


int Sys_ReturnJoystickInputEvent( const int n, int& action, int& value )
{
	return win32.g_Joystick.ReturnInputEvent( n, action, value );
}


void Sys_EndJoystickInputEvents()
{
}


/*
========================
JoystickSamplingThread
========================
*/
static int	threadTimeDeltas[256];
static int	threadPacket[256];
static int	threadCount;
void JoystickSamplingThread( void* data )
{
	static int prevTime = 0;
	static uint64 nextCheck[MAX_JOYSTICKS] = { 0 };
	const uint64 waitTime = 5000000; // poll every 5 seconds to see if a controller was connected
	while( 1 )
	{
		// hopefully we see close to 4000 usec each loop
		int	now = Sys_Microseconds();
		int	delta;
		if( prevTime == 0 )
		{
			delta = 4000;
		}
		else
		{
			delta = now - prevTime;
		}
		prevTime = now;
		threadTimeDeltas[threadCount & 255] = delta;
		threadCount++;
		
		{
			XINPUT_STATE	joyData[MAX_JOYSTICKS];
			bool			validData[MAX_JOYSTICKS];
			for( int i = 0 ; i < MAX_JOYSTICKS ; i++ )
			{
				if( now >= nextCheck[i] )
				{
					// XInputGetState might block... for a _really_ long time..
					validData[i] = XInputGetState( i, &joyData[i] ) == ERROR_SUCCESS;
					
					// allow an immediate data poll if the input device is connected else
					// wait for some time to see if another device was reconnected.
					// Checking input state infrequently for newly connected devices prevents
					// severe slowdowns on PC, especially on WinXP64.
					if( validData[i] )
					{
						nextCheck[i] = 0;
					}
					else
					{
						nextCheck[i] = now + waitTime;
					}
				}
			}
			
			// do this short amount of processing inside a critical section
			idScopedCriticalSection cs( win32.g_Joystick.mutexXis );
			
			for( int i = 0 ; i < MAX_JOYSTICKS ; i++ )
			{
				controllerState_t* cs = &win32.g_Joystick.controllers[i];
				
				if( !validData[i] )
				{
					cs->valid = false;
					continue;
				}
				cs->valid = true;
				
				XINPUT_STATE& current = joyData[i];
				
				cs->current = current;
				
				// Switch from using cs->current to current to reduce chance of Load-Hit-Store on consoles
				
				threadPacket[threadCount & 255] = current.dwPacketNumber;
#if 0
				if( xis.dwPacketNumber == oldXis[ inputDeviceNum ].dwPacketNumber )
				{
					return numEvents;
				}
#endif
				cs->buttonBits |= current.Gamepad.wButtons;
			}
		}
		
		// we want this to be processed at least 250 times a second
		WaitForSingleObject( win32.g_Joystick.timer, INFINITE );
	}
}


/*
========================
idJoystickWin32::idJoystickWin32
========================
*/
idJoystickWin32::idJoystickWin32()
{
	numEvents = 0;
	memset( &events, 0, sizeof( events ) );
	memset( &controllers, 0, sizeof( controllers ) );
	memset( buttonStates, 0, sizeof( buttonStates ) );
	memset( joyAxis, 0, sizeof( joyAxis ) );
}

/*
========================
idJoystickWin32::Init
========================
*/
bool idJoystickWin32::Init()
{
	idJoystick::Init();
	
	// setup the timer that the high frequency thread will wait on
	// to fire every 4 msec
	timer = CreateWaitableTimer( NULL, FALSE, "JoypadTimer" );
	LARGE_INTEGER dueTime;
	dueTime.QuadPart = -1;
	if( !SetWaitableTimer( timer, &dueTime, 4, NULL, NULL, FALSE ) )
	{
		idLib::FatalError( "SetWaitableTimer for joystick failed" );
	}
	
	// spawn the high frequency joystick reading thread
	Sys_CreateThread( ( xthread_t )JoystickSamplingThread, NULL, THREAD_HIGHEST, "Joystick", CORE_1A );
	
	return false;
}

/*
========================
idJoystickWin32::SetRumble
========================
*/
void idJoystickWin32::SetRumble( int inputDeviceNum, int rumbleLow, int rumbleHigh )
{
	if( inputDeviceNum < 0 || inputDeviceNum >= MAX_JOYSTICKS )
	{
		return;
	}
	if( !controllers[inputDeviceNum].valid )
	{
		return;
	}
	XINPUT_VIBRATION vibration;
	vibration.wLeftMotorSpeed = idMath::ClampInt( 0, 65535, rumbleLow );
	vibration.wRightMotorSpeed = idMath::ClampInt( 0, 65535, rumbleHigh );
	DWORD err = XInputSetState( inputDeviceNum, &vibration );
	if( err != ERROR_SUCCESS )
	{
		idLib::Warning( "XInputSetState error: 0x%x", err );
	}
}

/*
========================
idJoystickWin32::PostInputEvent
========================
*/
void idJoystickWin32::PostInputEvent( int inputDeviceNum, int event, int value, int range )
{
	// These events are used for GUI button presses
	if( ( event >= J_ACTION1 ) && ( event <= J_ACTION_MAX ) )
	{
		PushButton( inputDeviceNum, K_JOY1 + ( event - J_ACTION1 ), value != 0 );
	}
	else if( event == J_AXIS_LEFT_X )
	{
		PushButton( inputDeviceNum, K_JOY_STICK1_LEFT, ( value < -range ) );
		PushButton( inputDeviceNum, K_JOY_STICK1_RIGHT, ( value > range ) );
	}
	else if( event == J_AXIS_LEFT_Y )
	{
		PushButton( inputDeviceNum, K_JOY_STICK1_UP, ( value < -range ) );
		PushButton( inputDeviceNum, K_JOY_STICK1_DOWN, ( value > range ) );
	}
	else if( event == J_AXIS_RIGHT_X )
	{
		PushButton( inputDeviceNum, K_JOY_STICK2_LEFT, ( value < -range ) );
		PushButton( inputDeviceNum, K_JOY_STICK2_RIGHT, ( value > range ) );
	}
	else if( event == J_AXIS_RIGHT_Y )
	{
		PushButton( inputDeviceNum, K_JOY_STICK2_UP, ( value < -range ) );
		PushButton( inputDeviceNum, K_JOY_STICK2_DOWN, ( value > range ) );
	}
	else if( ( event >= J_DPAD_UP ) && ( event <= J_DPAD_RIGHT ) )
	{
		PushButton( inputDeviceNum, K_JOY_DPAD_UP + ( event - J_DPAD_UP ), value != 0 );
	}
	else if( event == J_AXIS_LEFT_TRIG )
	{
		PushButton( inputDeviceNum, K_JOY_TRIGGER1, ( value > range ) );
	}
	else if( event == J_AXIS_RIGHT_TRIG )
	{
		PushButton( inputDeviceNum, K_JOY_TRIGGER2, ( value > range ) );
	}
	if( event >= J_AXIS_MIN && event <= J_AXIS_MAX )
	{
		int axis = event - J_AXIS_MIN;
		int percent = ( value * 16 ) / range;
		if( joyAxis[inputDeviceNum][axis] != percent )
		{
			joyAxis[inputDeviceNum][axis] = percent;
			Sys_QueEvent( SE_JOYSTICK, axis, percent, 0, NULL, inputDeviceNum );
		}
	}
	
	// These events are used for actual game input
	events[numEvents].event = event;
	events[numEvents].value = value;
	numEvents++;
}

/*
========================
idJoystickWin32::PollInputEvents
========================
*/
int idJoystickWin32::PollInputEvents( int inputDeviceNum )
{
	numEvents = 0;
	
	if( !win32.activeApp )
	{
		return numEvents;
	}
	
	assert( inputDeviceNum < 4 );
	
//	if ( inputDeviceNum > in_joystick.GetInteger() ) {
//		return numEvents;
//	}

	controllerState_t* cs = &controllers[ inputDeviceNum ];
	
	// grab the current packet under a critical section
	XINPUT_STATE xis;
	XINPUT_STATE old;
	int		orBits;
	{
		idScopedCriticalSection crit( mutexXis );
		xis = cs->current;
		old = cs->previous;
		cs->previous = xis;
		// fetch or'd button bits
		orBits = cs->buttonBits;
		cs->buttonBits = 0;
	}
#if 0
	if( XInputGetState( inputDeviceNum, &xis ) != ERROR_SUCCESS )
	{
		return numEvents;
	}
#endif
	for( int i = 0 ; i < 32 ; i++ )
	{
		int	bit = 1 << i;
		
		if( ( ( xis.Gamepad.wButtons | old.Gamepad.wButtons ) & bit ) == 0
				&& ( orBits & bit ) )
		{
			idLib::Printf( "Dropped button press on bit %i\n", i );
		}
	}
	
	if( session->IsSystemUIShowing() )
	{
		// memset xis so the current input does not get latched if the UI is showing
		memset( &xis, 0, sizeof( XINPUT_STATE ) );
	}
	
	int joyRemap[16] =
	{
		J_DPAD_UP,		J_DPAD_DOWN,	// Up, Down
		J_DPAD_LEFT,	J_DPAD_RIGHT,	// Left, Right
		J_ACTION9,		J_ACTION10,		// Start, Back
		J_ACTION7,		J_ACTION8,		// Left Stick Down, Right Stick Down
		J_ACTION5,		J_ACTION6,		// Black, White (Left Shoulder, Right Shoulder)
		0,				0,				// Unused
		J_ACTION1,		J_ACTION2,		// A, B
		J_ACTION3,		J_ACTION4,		// X, Y
	};
	
	// Check the digital buttons
	for( int i = 0; i < 16; i++ )
	{
		int mask = ( 1 << i );
		if( ( xis.Gamepad.wButtons & mask ) != ( old.Gamepad.wButtons & mask ) )
		{
			PostInputEvent( inputDeviceNum, joyRemap[i], ( xis.Gamepad.wButtons & mask ) > 0 );
		}
	}
	
	// Check the triggers
	if( xis.Gamepad.bLeftTrigger != old.Gamepad.bLeftTrigger )
	{
		PostInputEvent( inputDeviceNum, J_AXIS_LEFT_TRIG, xis.Gamepad.bLeftTrigger * 128 );
	}
	if( xis.Gamepad.bRightTrigger != old.Gamepad.bRightTrigger )
	{
		PostInputEvent( inputDeviceNum, J_AXIS_RIGHT_TRIG, xis.Gamepad.bRightTrigger * 128 );
	}
	
	if( xis.Gamepad.sThumbLX != old.Gamepad.sThumbLX )
	{
		PostInputEvent( inputDeviceNum, J_AXIS_LEFT_X, xis.Gamepad.sThumbLX );
	}
	if( xis.Gamepad.sThumbLY != old.Gamepad.sThumbLY )
	{
		PostInputEvent( inputDeviceNum, J_AXIS_LEFT_Y, -xis.Gamepad.sThumbLY );
	}
	if( xis.Gamepad.sThumbRX != old.Gamepad.sThumbRX )
	{
		PostInputEvent( inputDeviceNum, J_AXIS_RIGHT_X, xis.Gamepad.sThumbRX );
	}
	if( xis.Gamepad.sThumbRY != old.Gamepad.sThumbRY )
	{
		PostInputEvent( inputDeviceNum, J_AXIS_RIGHT_Y, -xis.Gamepad.sThumbRY );
	}
	
	return numEvents;
}


/*
========================
idJoystickWin32::ReturnInputEvent
========================
*/
int idJoystickWin32::ReturnInputEvent( const int n, int& action, int& value )
{
	if( ( n < 0 ) || ( n >= MAX_JOY_EVENT ) )
	{
		return 0;
	}
	
	action = events[ n ].event;
	value = events[ n ].value;
	
	return 1;
}

/*
========================
idJoystickWin32::PushButton
========================
*/
void idJoystickWin32::PushButton( int inputDeviceNum, int key, bool value )
{
	// So we don't keep sending the same SE_KEY message over and over again
	if( buttonStates[inputDeviceNum][key] != value )
	{
		buttonStates[inputDeviceNum][key] = value;
		Sys_QueEvent( SE_KEY, key, value, 0, NULL, inputDeviceNum );
	}
}
