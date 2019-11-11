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
#include "precompiled.h"
#include "sys_lobby_backend.h"
#include "sys_lobby_backend_direct.h"

extern idCVar net_port;
extern idCVar net_ip;

extern idLobbyToSessionCB* lobbyToSessionCB;

/*
========================
idLobbyBackendDirect::idLobbyBackendWin
========================
*/
idLobbyBackendDirect::idLobbyBackendDirect()
{
	state = STATE_INVALID;
}

/*
========================
idLobbyBackendDirect::StartHosting
========================
*/
void idLobbyBackendDirect::StartHosting( const idMatchParameters& p, float skillLevel, lobbyBackendType_t type )
{
	NET_VERBOSE_PRINT( "idLobbyBackendDirect::StartHosting\n" );

	isLocal = MatchTypeIsLocal( p.matchFlags );
	isHost	= true;

	state	= STATE_READY;
	isLocal = true;
}

/*
========================
idLobbyBackendDirect::StartFinding
========================
*/
void idLobbyBackendDirect::StartFinding( const idMatchParameters& p, int numPartyUsers, float skillLevel )
{
	isLocal = MatchTypeIsLocal( p.matchFlags );
	isHost	= false;

	if( lobbyToSessionCB->CanJoinLocalHost() )
	{
		state = STATE_READY;
	}
	else
	{
		state = STATE_FAILED;
	}
}

/*
========================
idLobbyBackendDirect::GetSearchResults
========================
*/
void idLobbyBackendDirect::GetSearchResults( idList< lobbyConnectInfo_t >& searchResults )
{
	lobbyConnectInfo_t fakeResult;
	searchResults.Clear();
	searchResults.Append( fakeResult );
}

/*
========================
idLobbyBackendDirect::JoinFromConnectInfo
========================
*/
void idLobbyBackendDirect::JoinFromConnectInfo( const lobbyConnectInfo_t& connectInfo )
{
	if( lobbyToSessionCB->CanJoinLocalHost() )
	{
		// TODO: "CanJoinLocalHost" == *must* join LocalHost ?!
		Sys_StringToNetAdr( "localhost", &address, true );
		address.port = net_port.GetInteger();
		NET_VERBOSE_PRINT( "NET: idLobbyBackendDirect::JoinFromConnectInfo(): canJoinLocalHost\n" );
	}
	else
	{
		address = connectInfo.netAddr;
		NET_VERBOSE_PRINT( "NET: idLobbyBackendDirect::JoinFromConnectInfo(): %s\n", Sys_NetAdrToString( address ) );
	}

	state		= STATE_READY;
	isLocal		= false;
	isHost		= false;
}

/*
========================
idLobbyBackendDirect::Shutdown
========================
*/
void idLobbyBackendDirect::Shutdown()
{
	state = STATE_SHUTDOWN;
}

/*
========================
idLobbyBackendDirect::BecomeHost
========================
*/
void idLobbyBackendDirect::BecomeHost( int numInvites )
{
}

/*
========================
idLobbyBackendDirect::FinishBecomeHost
========================
*/
void idLobbyBackendDirect::FinishBecomeHost()
{
	isHost = true;
}

/*
========================
idLobbyBackendDirect::GetOwnerAddress
========================
*/
void idLobbyBackendDirect::GetOwnerAddress( lobbyAddress_t& outAddr )
{
	outAddr.netAddr = address;
	state			= STATE_READY;
}

/*
========================
idLobbyBackendDirect::SetIsJoinable
========================
*/
void idLobbyBackendDirect::SetIsJoinable( bool joinable )
{
}

/*
========================
idLobbyBackendDirect::GetConnectInfo
========================
*/
lobbyConnectInfo_t idLobbyBackendDirect::GetConnectInfo()
{
	lobbyConnectInfo_t connectInfo;

	// If we aren't the host, this lobby should have been joined through JoinFromConnectInfo
	if( IsHost() )
	{
		// If we are the host, give them our ip address
		// DG: always using the first IP doesn't work, because on linux that's 127.0.0.1
		// and even if not, this causes trouble with NAT.
		// So either use net_ip or, if it's not set ("localhost"), use 0.0.0.0 which is
		// a special case the client will treat as "just use the IP I used for the lobby"
		// (which is the right behavior for the Direct backend, I guess).
		// the client special case is in idLobby::HandleReliableMsg
		const char* ip = net_ip.GetString();
		if( ip == NULL || idStr::Length( ip ) == 0 || idStr::Icmp( ip, "localhost" ) == 0 )
		{
			ip = "0.0.0.0";
		}
		// DG end
		Sys_StringToNetAdr( ip, &address, false );
		address.port = net_port.GetInteger();
	}

	connectInfo.netAddr = address;

	return connectInfo;
}

/*
========================
idLobbyBackendDirect::IsOwnerOfConnectInfo
========================
*/
bool idLobbyBackendDirect::IsOwnerOfConnectInfo( const lobbyConnectInfo_t& connectInfo ) const
{
	return Sys_CompareNetAdrBase( address, connectInfo.netAddr );
}

/*
========================
idLobbyBackendDirect::Pump
========================
*/
void idLobbyBackendDirect::Pump()
{
}

/*
========================
idLobbyBackendDirect::UpdateMatchParms
========================
*/
void idLobbyBackendDirect::UpdateMatchParms( const idMatchParameters& p )
{
}

/*
========================
idLobbyBackendDirect::UpdateLobbySkill
========================
*/
void idLobbyBackendDirect::UpdateLobbySkill( float lobbySkill )
{
}

/*
========================
idLobbyBackendDirect::SetInGame
========================
*/
void idLobbyBackendDirect::SetInGame( bool value )
{
}

/*
========================
idLobbyBackendDirect::RegisterUser
========================
*/
void idLobbyBackendDirect::RegisterUser( lobbyUser_t* user, bool isLocal )
{
}

/*
========================
idLobbyBackendDirect::UnregisterUser
========================
*/
void idLobbyBackendDirect::UnregisterUser( lobbyUser_t* user, bool isLocal )
{
}
