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

#ifndef __KEYINPUT_H__
#define __KEYINPUT_H__

struct keyBindings_t
{
	idStr keyboard;
	idStr mouse;
	idStr gamepad;
};

class idSerializer;

// Converts from a USB HID code to a K_ code
int Key_CovertHIDCode( int hid );

class idKeyInput
{
public:
	static void				Init();
	static void				Shutdown();

	static void				ArgCompletion_KeyName( const idCmdArgs& args, void( *callback )( const char* s ) );
	static void				PreliminaryKeyEvent( int keyNum, bool down );
	static bool				IsDown( int keyNum );
	static int				GetUsercmdAction( int keyNum );
	static bool				GetOverstrikeMode();
	static void				SetOverstrikeMode( bool state );
	static void				ClearStates();

	static keyNum_t			StringToKeyNum( const char* str );		// This is used by the "bind" command
	static const char* 		KeyNumToString( keyNum_t keyNum );		// This is the inverse of StringToKeyNum, used for config files
	static const char* 		LocalizedKeyName( keyNum_t keyNum );	// This returns text suitable to print on screen

	static void				SetBinding( int keyNum, const char* binding );
	static const char* 		GetBinding( int keyNum );
	static bool				UnbindBinding( const char* bind );
	static int				NumBinds( const char* binding );
	static bool				ExecKeyBinding( int keyNum );
	static const char* 		KeysFromBinding( const char* bind );
	static const char* 		BindingFromKey( const char* key );
	static bool				KeyIsBoundTo( int keyNum, const char* binding );
	static void				WriteBindings( idFile* f );
	static keyBindings_t	KeyBindingsFromBinding( const char* bind, bool firstOnly = false, bool localized = false );
};

#endif /* !__KEYINPUT_H__ */
