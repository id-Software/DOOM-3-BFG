/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.

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

#include "precompiled.h"
#pragma hdrstop

//
// This file implements the low-level keyboard hook that traps the task keys.
//
#include "win_local.h"

#define DLLEXPORT __declspec(dllexport)

// Magic registry key/value for "Remove Task Manager" policy.
LPCTSTR KEY_DisableTaskMgr = "Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System";
LPCTSTR VAL_DisableTaskMgr = "DisableTaskMgr";

// The section is SHARED among all instances of this DLL.
// A low-level keyboard hook is always a system-wide hook.
#pragma data_seg (".mydata")
HHOOK g_hHookKbdLL = NULL;	// hook handle
BOOL  g_bBeep = FALSE;		// beep on illegal key
#pragma data_seg ()
#pragma comment(linker, "/SECTION:.mydata,RWS") // tell linker: make it shared

/*
================
MyTaskKeyHookLL

  Low-level keyboard hook:
  Trap task-switching keys by returning without passing along.
================
*/
LRESULT CALLBACK MyTaskKeyHookLL( int nCode, WPARAM wp, LPARAM lp )
{
	KBDLLHOOKSTRUCT* pkh = ( KBDLLHOOKSTRUCT* ) lp;

	if( nCode == HC_ACTION )
	{
		BOOL bCtrlKeyDown = GetAsyncKeyState( VK_CONTROL ) >> ( ( sizeof( SHORT ) * 8 ) - 1 );

		if(	( pkh->vkCode == VK_ESCAPE && bCtrlKeyDown )				// Ctrl+Esc
				|| ( pkh->vkCode == VK_TAB && pkh->flags & LLKHF_ALTDOWN )		// Alt+TAB
				|| ( pkh->vkCode == VK_ESCAPE && pkh->flags & LLKHF_ALTDOWN )	// Alt+Esc
				|| ( pkh->vkCode == VK_LWIN || pkh->vkCode == VK_RWIN )		// Start Menu
		  )
		{

			if( g_bBeep && ( wp == WM_SYSKEYDOWN || wp == WM_KEYDOWN ) )
			{
				MessageBeep( 0 ); // beep on downstroke if requested
			}
			return 1; // return without processing the key strokes
		}
	}
	return CallNextHookEx( g_hHookKbdLL, nCode, wp, lp );
}

/*
================
AreTaskKeysDisabled

  Are task keys disabled--ie, is hook installed?
  Note: This assumes there's no other hook that does the same thing!
================
*/
BOOL AreTaskKeysDisabled()
{
	return g_hHookKbdLL != NULL;
}

/*
================
IsTaskMgrDisabled
================
*/
BOOL IsTaskMgrDisabled()
{
	HKEY hk;

	if( RegOpenKey( HKEY_CURRENT_USER, KEY_DisableTaskMgr, &hk ) != ERROR_SUCCESS )
	{
		return FALSE; // no key ==> not disabled
	}

	DWORD val = 0;
	DWORD len = 4;
	return RegQueryValueEx( hk, VAL_DisableTaskMgr, NULL, NULL, ( BYTE* )&val, &len ) == ERROR_SUCCESS && val == 1;
}

/*
================
DisableTaskKeys
================
*/
void DisableTaskKeys( BOOL bDisable, BOOL bBeep, BOOL bTaskMgr )
{

	// task keys (Ctrl+Esc, Alt-Tab, etc.)
	if( bDisable )
	{
		if( !g_hHookKbdLL )
		{
			g_hHookKbdLL = SetWindowsHookEx( WH_KEYBOARD_LL, MyTaskKeyHookLL, win32.hInstance, 0 );
		}
	}
	else if( g_hHookKbdLL != NULL )
	{
		UnhookWindowsHookEx( g_hHookKbdLL );
		g_hHookKbdLL = NULL;
	}
	g_bBeep = bBeep;

	// task manager (Ctrl+Alt+Del)
	if( bTaskMgr )
	{
		HKEY hk;
		if( RegOpenKey( HKEY_CURRENT_USER, KEY_DisableTaskMgr, &hk ) != ERROR_SUCCESS )
		{
			RegCreateKey( HKEY_CURRENT_USER, KEY_DisableTaskMgr, &hk );
		}
		if( bDisable )
		{
			// disable TM: set policy = 1
			DWORD val = 1;
			RegSetValueEx( hk, VAL_DisableTaskMgr, NULL, REG_DWORD, ( BYTE* )&val, sizeof( val ) );
		}
		else
		{
			// enable TM: remove policy
			RegDeleteValue( hk, VAL_DisableTaskMgr );
		}
	}
}
