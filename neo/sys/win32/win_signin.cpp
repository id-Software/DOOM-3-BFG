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

#pragma hdrstop
#include "../../idlib/precompiled.h"
#include "../../framework/PlayerProfile.h"
#include "../sys_session_local.h"
#include "win_signin.h"

#ifdef _DEBUG
idCVar win_userPersistent( "win_userPersistent", "1", CVAR_BOOL, "debugging cvar for profile persistence status" );
idCVar win_userOnline( "win_userOnline", "1", CVAR_BOOL, "debugging cvar for profile online status" );
idCVar win_isInParty( "win_isInParty", "0", CVAR_BOOL, "debugging cvar for platform party status" );
idCVar win_partyCount( "win_partyCount", "0", CVAR_INTEGER, "debugginc var for platform party count" );
#endif

/*
========================
idSignInManagerWin::Shutdown
========================
*/
void idSignInManagerWin::Shutdown() {
}

/*
========================
idSignInManagerWin::Pump
========================
*/
void idSignInManagerWin::Pump() {
	
	// If we have more users than we need, then set to the lower amount
	// (don't remove the master user though)
	if ( localUsers.Num() > 1 && localUsers.Num() > maxDesiredLocalUsers ) {
		localUsers.SetNum( maxDesiredLocalUsers );
	}
	
#ifndef ID_RETAIL
	// If we don't have enough, then make sure we do
	// NOTE - We always want at least one user on windows for now, 
	// and this master user will always use controller 0
	while ( localUsers.Num() < minDesiredLocalUsers ) {
		RegisterLocalUser( localUsers.Num() );
	}
#endif
	
	// See if we need to save settings on any of the profiles
	for ( int i = 0; i < localUsers.Num(); i++ ) {
		localUsers[i].Pump();
	}
}

/*
========================
idSignInManagerWin::RemoveLocalUserByIndex
========================
*/
void idSignInManagerWin::RemoveLocalUserByIndex( int index ) {
	session->OnLocalUserSignout( &localUsers[index] );
	localUsers.RemoveIndex( index );
}

/*
========================
idSignInManagerWin::RegisterLocalUser
========================
*/
void idSignInManagerWin::RegisterLocalUser( int inputDevice ) {
	if ( GetLocalUserByInputDevice( inputDevice ) != NULL ) {
		return;
	}
	
	static char machineName[128];
	DWORD len = 128;
	::GetComputerName( machineName, &len );

	const char * nameSource = machineName;

	idStr name( nameSource );
	int nameLength = name.Length();
	if ( idStr::IsValidUTF8( nameSource, nameLength ) ) {
		int nameIndex = 0;
		int numChars = 0;
		name.Empty();
		while ( nameIndex < nameLength && numChars++ < idLocalUserWin::MAX_GAMERTAG_CHARS ) {
			uint32 c = idStr::UTF8Char( nameSource, nameIndex );
			name.AppendUTF8Char( c );
		}
	}
	
	idLocalUserWin & localUser = *localUsers.Alloc();
	
	localUser.Init( inputDevice, name.c_str(), localUsers.Num() );
	localUser.SetLocalUserHandle( GetUniqueLocalUserHandle( localUser.GetGamerTag() ) );

	session->OnLocalUserSignin( &localUser );
}

/*
========================
idSignInManagerWin::CreateNewUser
========================
*/
bool idSignInManagerWin::CreateNewUser( winUserState_t & state ) {
	//idScopedGlobalHeap	everythingHereGoesInTheGlobalHeap;	// users obviously persist across maps

	RemoveAllLocalUsers();
	RegisterLocalUser( state.inputDevice );

	if ( localUsers.Num() > 0 ) {
		if ( !localUsers[0].VerifyUserState( state ) ) {
			RemoveAllLocalUsers();
		}
	}

	return true;
}

CONSOLE_COMMAND( testRemoveAllLocalUsers, "Forces removal of local users - mainly for PC testing", NULL ) {
	session->GetSignInManager().RemoveAllLocalUsers();
}
