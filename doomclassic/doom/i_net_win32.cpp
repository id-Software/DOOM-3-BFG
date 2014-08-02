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
#include "globaldata.h"


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <string>

#include <errno.h>

#include "i_system.h"
#include "d_event.h"
#include "d_net.h"
#include "m_argv.h"

#include "doomstat.h"

#include "i_net.h"

#include "doomlib.h"

void	NetSend (void);
qboolean NetListen (void);

namespace {
	bool IsValidSocket( int socketDescriptor );
	int GetLastSocketError();



	/*
	========================
	Returns true if the socket is valid. I made this function to help abstract the differences
	between WinSock (used on Xbox) and BSD sockets, which the PS3 follows more closely.
	========================
	*/
	bool IsValidSocket( int socketDescriptor ) {
		return false;
	}

	/*
	========================
	Returns the last error reported by the platform's socket library.
	========================
	*/
	int GetLastSocketError() {
		return 0;
	}
}

//
// NETWORKING
//
int	DOOMPORT = 1002;	// DHM - Nerve :: On original XBox, ports 1000 - 1255 saved you a byte on every packet.  360 too?


unsigned long GetServerIP() {
	//return ::g->sendaddress[::g->doomcom.consoleplayer].sin_addr.s_addr;
	return 0;
}

void	(*netget) (void);
void	(*netsend) (void);


//
// UDPsocket
//
int UDPsocket (void)
{
	//int	s;

	// allocate a socket
	//s = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	//if ( !IsValidSocket( s ) ) {
	//	int err = GetLastSocketError();
	//	I_Error( "can't create socket, error %d", err );
	//}

	//return s;
	return 0;
}

//
// BindToLocalPort
//
void BindToLocalPort( int	s, int	port )
{

}


//
// PacketSend
//
void PacketSend (void)
{

}


//
// PacketGet
//
void PacketGet (void)
{

}

static int I_TrySetupNetwork(void)
{
	// DHM - Moved to Session
	return 1;
}

//
// I_InitNetwork
//
void I_InitNetwork (void)
{
	//qboolean		trueval = true;
	int			i;
	int			p;
	//int a = 0;
	//    struct hostent*	hostentry;	// host information entry

	memset (&::g->doomcom, 0, sizeof(::g->doomcom) );

	// set up for network
	i = M_CheckParm ("-dup");
	if (i && i< ::g->myargc-1)
	{
		::g->doomcom.ticdup = ::g->myargv[i+1][0]-'0';
		if (::g->doomcom.ticdup < 1)
			::g->doomcom.ticdup = 1;
		if (::g->doomcom.ticdup > 9)
			::g->doomcom.ticdup = 9;
	}
	else
		::g->doomcom.ticdup = 1;

	if (M_CheckParm ("-extratic"))
		::g->doomcom.extratics = 1;
	else
		::g->doomcom.extratics = 0;

	p = M_CheckParm ("-port");
	if (p && p < ::g->myargc-1)
	{
		DOOMPORT = atoi (::g->myargv[p+1]);
		I_Printf ("using alternate port %i\n",DOOMPORT);
	}

	// parse network game options,
	//  -net <::g->consoleplayer> <host> <host> ...
	i = M_CheckParm ("-net");
	if (!i || !I_TrySetupNetwork())
	{
		// single player game
		::g->netgame = false;
		::g->doomcom.id = DOOMCOM_ID;
		::g->doomcom.numplayers = ::g->doomcom.numnodes = 1;
		::g->doomcom.deathmatch = false;
		::g->doomcom.consoleplayer = 0;
		return;
	}

	netsend = PacketSend;
	netget = PacketGet;

#ifdef ID_ENABLE_DOOM_CLASSIC_NETWORKING
	::g->netgame = true;

	{
		++i; // skip the '-net'
		::g->doomcom.numnodes = 0;
		::g->doomcom.consoleplayer = atoi( ::g->myargv[i] );
		// skip the console number
		++i;
		::g->doomcom.numnodes = 0;
		for (; i < ::g->myargc; ++i)
		{
			::g->sendaddress[::g->doomcom.numnodes].sin_family = AF_INET;
			::g->sendaddress[::g->doomcom.numnodes].sin_port = htons(DOOMPORT);
			
			// Pull out the port number.
			const std::string ipAddressWithPort( ::g->myargv[i] );
			const std::size_t colonPosition = ipAddressWithPort.find_last_of(':');
			std::string ipOnly;

			if( colonPosition != std::string::npos && colonPosition + 1 < ipAddressWithPort.size() ) {
				const std::string portOnly( ipAddressWithPort.substr( colonPosition + 1 ) );

				::g->sendaddress[::g->doomcom.numnodes].sin_port = htons( atoi( portOnly.c_str() ) );

				ipOnly = ipAddressWithPort.substr( 0, colonPosition );
			} else {
				// Assume the address doesn't include a port.
				ipOnly = ipAddressWithPort;
			}

			in_addr_t ipAddress = inet_addr( ipOnly.c_str() );

			if ( ipAddress == INADDR_NONE ) {
				I_Error( "Invalid IP Address: %s\n", ipOnly.c_str() );
				session->QuitMatch();
				common->AddDialog( GDM_OPPONENT_CONNECTION_LOST, DIALOG_ACCEPT, NULL, NULL, false );
			}
			::g->sendaddress[::g->doomcom.numnodes].sin_addr.s_addr = ipAddress;
			::g->doomcom.numnodes++;
		}
		
		::g->doomcom.id = DOOMCOM_ID;
		::g->doomcom.numplayers = ::g->doomcom.numnodes;
	}

	if ( globalNetworking ) {
		// Setup sockets
		::g->insocket = UDPsocket ();
		BindToLocalPort (::g->insocket,htons(DOOMPORT));
		
		// PS3 call to enable non-blocking mode
		int nonblocking = 1; // Non-zero is nonblocking mode.
		setsockopt( ::g->insocket, SOL_SOCKET, SO_NBIO, &nonblocking, sizeof(nonblocking));

		::g->sendsocket = UDPsocket ();

		I_Printf( "[+] Setting up sockets for player %d\n", DoomLib::GetPlayer() );
	}
#endif
}

// DHM - Nerve
void I_ShutdownNetwork() {
	
}

void I_NetCmd (void)
{
	//if (::g->doomcom.command == CMD_SEND)
	//{
	//	netsend ();
	//}
	//else if (::g->doomcom.command == CMD_GET)
	//{
	//	netget ();
	//}
	//else
	//	I_Error ("Bad net cmd: %i\n",::g->doomcom.command);
}

