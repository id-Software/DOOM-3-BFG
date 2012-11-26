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

#include "Precompiled.h"
#include "../Main/Main.h"

#include <sys/types.h>
#include <sys/socket.h>

#include "tech5/engine/sys/sys_lobby.h"

bool useTech5Packets = true;

#if 1

struct networkitem
{
	int source;
	int size;
	char buffer[256*64];
};

std::queue< networkitem > networkstacks[4];
//sockaddr_in sockaddrs[4];

int DoomLibRecv( char* buff, DWORD *numRecv )
{
	//int player = DoomInterface::CurrentPlayer();
	int player = ::g->consoleplayer;

	if (networkstacks[player].empty())
		return -1;

	networkitem item = networkstacks[player].front();
 
	memcpy( buff, item.buffer, item.size );
	*numRecv = item.size;
	//*source = sockaddrs[item.source];

	networkstacks[player].pop();
	
	return 1;
}

void I_Printf(char *error, ...);

int DoomLibSend( const char* buff, DWORD size, sockaddr_in *target, int toNode )
{
	int i;
	
	i = DoomLib::RemoteNodeToPlayerIndex( toNode );

	//I_Printf( "DoomLibSend %d --> %d: %d\n", ::g->consoleplayer, i, size );

	networkitem item;
	item.source = DoomInterface::CurrentPlayer();
	item.size = size;
	memcpy( item.buffer, buff, size );
	networkstacks[i].push( item );

	return 1;
}


int DoomLibSendRemote()
{
	if ( gameLocal == NULL ) {
		return 0;
	}

	int curPlayer = DoomLib::GetPlayer();

	for (int player = 0; player < gameLocal->Interface.GetNumPlayers(); ++player)
	{
		DoomLib::SetPlayer( player );

		for( int i = 0; i < 4; i++ ) {
		
			//Check if it is remote
			int node = DoomLib::PlayerIndexToRemoteNode( i );
			if ( ::g->sendaddress[node].sin_addr.s_addr == ::g->sendaddress[0].sin_addr.s_addr ) {
				continue;
			}

			while(!networkstacks[i].empty()) {
				networkitem item = networkstacks[i].front();

				int		c;
				//WSABUF buffer;
				//DWORD num_sent;

				//buffer.buf = (char*)&item.buffer;
				//buffer.len = item.size;
				
				if ( useTech5Packets ) {
					idLobby & lobby = static_cast< idLobby & >( session->GetGameLobbyBase() );

					lobbyUser_t * user = lobby.GetLobbyUser( i );

					if ( user != NULL ) {
						lobby.SendConnectionLess( user->address, idLobby::OOB_GENERIC_GAME_DATA, (const byte *)(&item.buffer[0] ), item.size );
					}
				} else {
					c = sendto( ::g->sendsocket, &item.buffer, item.size, MSG_DONTWAIT, (sockaddr*)&::g->sendaddress[node], sizeof(::g->sendaddress[node]) );

					//c = WSASendTo(::g->sendsocket, &buffer, 1, &num_sent, 0, (sockaddr*)&::g->sendaddress[node], sizeof(::g->sendaddress[node]), 0, 0);
				}

				networkstacks[i].pop();
			}
		}
	}

	DoomLib::SetPlayer(curPlayer);

	return 1;
}

void DL_InitNetworking( DoomInterface *pdi )
{
	// DHM - Nerve :: Clear out any old splitscreen packets that may be lingering.
	for ( int i = 0; i<4; i++ ) {
		while ( !networkstacks[i].empty() ) {
			networkstacks[i].pop();
		}
	}

	/*sockaddrs[0].sin_addr.s_addr = inet_addr("0.0.0.1" );
	sockaddrs[1].sin_addr.s_addr = inet_addr("0.0.0.2" );
	sockaddrs[2].sin_addr.s_addr = inet_addr("0.0.0.3" );
	sockaddrs[3].sin_addr.s_addr = inet_addr("0.0.0.4" );*/
	pdi->SetNetworking( DoomLibRecv, DoomLibSend, DoomLibSendRemote );
}

#else

void DL_InitNetworking( DoomInterface *pdi ) {

}

#endif
