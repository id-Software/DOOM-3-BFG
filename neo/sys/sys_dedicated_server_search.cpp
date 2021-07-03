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
#include "sys_dedicated_server_search.h"

/*
========================
idDedicatedServerSearch::idDedicatedServerSearch
========================
*/
idDedicatedServerSearch::idDedicatedServerSearch() :
	callback( NULL )
{
}

/*
========================
idDedicatedServerSearch::~idDedicatedServerSearch
========================
*/
idDedicatedServerSearch::~idDedicatedServerSearch()
{
	if( callback != NULL )
	{
		delete callback;
	}
}



/*
========================
idDedicatedServerSearch::StartSearch
========================
*/
void idDedicatedServerSearch::StartSearch( const idCallback& cb )
{
	Clear();
	callback = cb.Clone();
}

/*
========================
idDedicatedServerSearch::Clear
========================
*/
void idDedicatedServerSearch::Clear()
{
	if( callback != NULL )
	{
		delete callback;
		callback = NULL;
	}
	list.Clear();
}

/*
========================
idDedicatedServerSearch::Clear
========================
*/
void idDedicatedServerSearch::HandleQueryAck( lobbyAddress_t& addr, idBitMsg& msg )
{
	bool found = false;
	// Find the server this ack belongs to
	for( int i = 0; i < list.Num(); i++ )
	{
		serverInfoDedicated_t& query = list[i];


		if( query.addr.Compare( addr ) )
		{
			// Found the server
			found = true;

			bool canJoin = msg.ReadBool();

			if( !canJoin )
			{
				// If we can't join this server, then remove it
				list.RemoveIndex( i-- );
				break;
			}

			query.serverInfo.Read( msg );
			query.connectedPlayers.Clear();
			for( int i = 0; i < query.serverInfo.numPlayers; i++ )
			{
				idStr user;
				msg.ReadString( user );
				query.connectedPlayers.Append( user );
			}
			break;
		}
	}

	if( !found )
	{
		bool canJoin = msg.ReadBool();
		if( canJoin )
		{
			serverInfoDedicated_t newServer;
			newServer.addr = addr;
			newServer.serverInfo.Read( msg );
			if( newServer.serverInfo.serverName.IsEmpty() )
			{
				newServer.serverInfo.serverName = addr.ToString();
			}
			newServer.connectedPlayers.Clear();
			for( int i = 0; i < newServer.serverInfo.numPlayers; i++ )
			{
				idStr user;
				msg.ReadString( user );
				newServer.connectedPlayers.Append( user );
			}
			list.Append( newServer );
		}
	}


	if( callback != NULL )
	{
		callback->Call();
	}
}

/*
========================
idDedicatedServerSearch::GetAddrAtIndex
========================
*/
bool idDedicatedServerSearch::GetAddrAtIndex( netadr_t& addr, int i )
{
	if( i >= 0 && i < list.Num() )
	{
		addr = list[i].addr.netAddr;
		return true;
	}
	return false;
}

/*
========================
idDedicatedServerSearch::DescribeServerAtIndex
========================
*/
const serverInfo_t* idDedicatedServerSearch::DescribeServerAtIndex( int i ) const
{
	if( i >= 0 && i < list.Num() )
	{
		return &list[i].serverInfo;
	}
	return NULL;
}

/*
========================
idDedicatedServerSearch::GetServerPlayersAtIndex
========================
*/
const idList< idStr >* idDedicatedServerSearch::GetServerPlayersAtIndex( int i ) const
{
	if( i >= 0 && i < list.Num() )
	{
		return &list[i].connectedPlayers;
	}
	return NULL;
}

/*
========================
idDedicatedServerSearch::NumServers
========================
*/
int idDedicatedServerSearch::NumServers() const
{
	return list.Num();
}