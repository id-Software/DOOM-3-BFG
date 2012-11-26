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

#include <errno.h>

#include "i_system.h"
#include "d_event.h"
#include "d_net.h"
#include "m_argv.h"

#include "doomstat.h"

#include "i_net.h"

#include "doomlib.h"
#include "../Main/Main.h"

void	NetSend (void);
qboolean NetListen (void);

//
// NETWORKING
//
int	DOOMPORT = 1002;	// DHM - Nerve :: On original XBox, ports 1000 - 1255 saved you a byte on every packet.  360 too?



unsigned long GetServerIP() { return ::g->sendaddress[::g->doomcom.consoleplayer].sin_addr.s_addr; }

void	(*netget) (void);
void	(*netsend) (void);


//
// UDPsocket
//
int UDPsocket (void)
{
	int	s;

	// allocate a socket
	s = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (s == INVALID_SOCKET) {
		int err = WSAGetLastError ();
		I_Error ("can't create socket: %s",strerror(errno));
	}

	return s;
}

//
// BindToLocalPort
//
void BindToLocalPort( int	s, int	port )
{
	int			v;
	struct sockaddr_in	address;

	memset (&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = port;

	v = bind (s, (sockaddr*)&address, sizeof(address));
	//if (v == -1)
		//I_Error ("BindToPort: bind: %s", strerror(errno));
}


//
// PacketSend
//
void PacketSend (void)
{
	int		c;
	doomdata_t	sw;

	// byte swap
	sw.checksum = htonl(::g->netbuffer->checksum);
	sw.sourceDest = DoomLib::BuildSourceDest(::g->doomcom.remotenode);
	sw.player = ::g->netbuffer->player;
	sw.retransmitfrom = ::g->netbuffer->retransmitfrom;
	sw.starttic = ::g->netbuffer->starttic;
	sw.numtics = ::g->netbuffer->numtics;
	for (c=0 ; c< ::g->netbuffer->numtics ; c++)
	{
		sw.cmds[c].forwardmove = ::g->netbuffer->cmds[c].forwardmove;
		sw.cmds[c].sidemove = ::g->netbuffer->cmds[c].sidemove;
		sw.cmds[c].angleturn = htons(::g->netbuffer->cmds[c].angleturn);
		//sw.cmds[c].consistancy = htons(::g->netbuffer->cmds[c].consistancy);
		sw.cmds[c].buttons = ::g->netbuffer->cmds[c].buttons;
	}

	// Send Socket
	{
		//DWORD num_sent;
		WSABUF buffer;

		buffer.buf = (char*)&sw;
		buffer.len = ::g->doomcom.datalength;

		//if ( globalNetworking ) {
		//	c = WSASendTo(::g->sendsocket, &buffer, 1, &num_sent, 0, (sockaddr*)&::g->sendaddress[::g->doomcom.remotenode],
		//					sizeof(::g->sendaddress[::g->doomcom.remotenode]), 0, 0);
		//} else {
			c = DoomLib::Send( buffer.buf, buffer.len, (sockaddr_in*)&::g->sendaddress[::g->doomcom.remotenode], ::g->doomcom.remotenode ); 
		//}
	}
}


//
// PacketGet
//
void PacketGet (void)
{
	int			i;
	int			c;
	struct sockaddr_in	fromaddress;
	int			fromlen;
	doomdata_t		sw;
	WSABUF buffer;
	DWORD num_recieved, flags = 0;

	// Try and read a socket
	buffer.buf = (char*)&sw;
	buffer.len = sizeof(sw);
	fromlen = sizeof(fromaddress);

	//if ( globalNetworking ) {
	//	c = WSARecvFrom(::g->insocket, &buffer, 1, &num_recieved, &flags, (struct sockaddr*)&fromaddress, &fromlen, 0, 0);
	//} else {
		c = DoomLib::Recv( (char*)&sw, &num_recieved );
	//}
	if (c == SOCKET_ERROR )
	{
		/*if ( globalNetworking ) {
			int err = WSAGetLastError();
			if (err != WSAEWOULDBLOCK)
				I_Error ("GetPacket: %s",strerror(errno));
		}*/

		::g->doomcom.remotenode = -1;		// no packet
		return;
	}

	

	// find remote node number
	/*for (i=0 ; i<::g->doomcom.numnodes ; i++)
		if ( fromaddress.sin_addr.s_addr == ::g->sendaddress[i].sin_addr.s_addr )
			break;

	if (i == ::g->doomcom.numnodes)
	{
		// packet is not from one of the ::g->players (new game broadcast)
		::g->doomcom.remotenode = -1;		// no packet
		return;
	}*/

	//if ( ::g->consoleplayer == 1 ) {
		//int x = 0;
	//}

	int source;
	int dest;
	DoomLib::GetSourceDest( sw.sourceDest, &source, &dest );

	i = source;

	//if ( ::g->consoleplayer == 1 ) {
		//if ( i == 2 ) {
			//int suck = 0;
		//}
	//}

	::g->doomcom.remotenode = i;			// good packet from a game player
	::g->doomcom.datalength = (short)num_recieved;

	// byte swap
	::g->netbuffer->checksum = ntohl(sw.checksum);
	::g->netbuffer->player = sw.player;
	::g->netbuffer->retransmitfrom = sw.retransmitfrom;
	::g->netbuffer->starttic = sw.starttic;
	::g->netbuffer->numtics = sw.numtics;

	for (c=0 ; c< ::g->netbuffer->numtics ; c++)
	{
		::g->netbuffer->cmds[c].forwardmove = sw.cmds[c].forwardmove;
		::g->netbuffer->cmds[c].sidemove = sw.cmds[c].sidemove;
		::g->netbuffer->cmds[c].angleturn = ntohs(sw.cmds[c].angleturn);
		//::g->netbuffer->cmds[c].consistancy = ntohs(sw.cmds[c].consistancy);
		::g->netbuffer->cmds[c].buttons = sw.cmds[c].buttons;
	}
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
	qboolean		trueval = true;
	int			i;
	int			p;
	int a = 0;
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
	if (p && p<::g->myargc-1)
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
			::g->sendaddress[::g->doomcom.numnodes].sin_addr.s_addr = inet_addr (::g->myargv[i]);//39, 17
			::g->doomcom.numnodes++;
		}
		
		::g->doomcom.id = DOOMCOM_ID;
		::g->doomcom.numplayers = ::g->doomcom.numnodes;
	}

	if ( globalNetworking ) {
		// Setup sockets
		::g->insocket = UDPsocket ();
		BindToLocalPort (::g->insocket,htons(DOOMPORT));
		ioctlsocket (::g->insocket, FIONBIO, (u_long*)&trueval);

		::g->sendsocket = UDPsocket ();

		I_Printf( "[+] Setting up sockets for player %d\n", DoomLib::GetPlayer() );
	}
}

// DHM - Nerve
void I_ShutdownNetwork( void ) {
	if ( globalNetworking ) {

		int curPlayer = DoomLib::GetPlayer();

		for (int player = 0; player < App->Game->Interface.GetNumPlayers(); ++player)
		{
			DoomLib::SetPlayer( player );

			if ( ::g->insocket != INVALID_SOCKET ) {
				I_Printf( "[-] Shut down insocket for player %d\n", DoomLib::GetPlayer() );
				shutdown( ::g->insocket, SD_BOTH );
				closesocket( ::g->insocket );
			}
			if ( ::g->sendsocket != INVALID_SOCKET ) {
				I_Printf( "[-] Shut down sendsocket for player %d\n", DoomLib::GetPlayer() );
				shutdown( ::g->sendsocket, SD_BOTH );
				closesocket( ::g->sendsocket );
			}
		}

		DoomLib::SetPlayer(curPlayer);

		globalNetworking = false;
	}
}

void I_NetCmd (void)
{
	if (::g->doomcom.command == CMD_SEND)
	{
		netsend ();
	}
	else if (::g->doomcom.command == CMD_GET)
	{
		netget ();
	}
	else
		I_Error ("Bad net cmd: %i\n",::g->doomcom.command); 
}

