/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <errno.h>
#include <sys/select.h>
#include <net/if.h>
#if MACOS_X
#include <ifaddrs.h>
#endif

#include "../../idlib/precompiled.h"

idPort clientPort, serverPort;

idCVar net_ip( "net_ip", "localhost", CVAR_SYSTEM, "local IP address" );
idCVar net_port( "net_port", "", CVAR_SYSTEM | CVAR_INTEGER, "local IP port number" );

typedef struct
{
	// RB: 64 bit fixes, changed long to int
	unsigned int ip;
	unsigned int mask;
	// RB end
} net_interface;

#define 		MAX_INTERFACES	32
int				num_interfaces = 0;
net_interface	netint[MAX_INTERFACES];

/*
=============
NetadrToSockadr
=============
*/
static void NetadrToSockadr( const netadr_t* a, struct sockaddr_in* s )
{
	memset( s, 0, sizeof( *s ) );
	
	if( a->type == NA_BROADCAST )
	{
		s->sin_family = AF_INET;
		
		s->sin_port = htons( ( short )a->port );
		*( int* ) &s->sin_addr = -1;
	}
	else if( a->type == NA_IP || a->type == NA_LOOPBACK )
	{
		s->sin_family = AF_INET;
		
		*( int* ) &s->sin_addr = *( int* ) &a->ip;
		s->sin_port = htons( ( short )a->port );
	}
}

/*
=============
SockadrToNetadr
=============
*/
static void SockadrToNetadr( struct sockaddr_in* s, netadr_t* a )
{
	unsigned int ip = *( int* )&s->sin_addr;
	*( int* )&a->ip = ip;
	a->port = ntohs( s->sin_port );
	// we store in network order, that loopback test is host order..
	ip = ntohl( ip );
	if( ip == INADDR_LOOPBACK )
	{
		a->type = NA_LOOPBACK;
	}
	else
	{
		a->type = NA_IP;
	}
}

/*
=============
ExtractPort
=============
*/
static bool ExtractPort( const char* src, char* buf, int bufsize, int* port )
{
	char* p;
	strncpy( buf, src, bufsize );
	p = buf;
	p += Min( bufsize - 1, ( int )strlen( src ) );
	*p = '\0';
	p = strchr( buf, ':' );
	if( !p )
	{
		return false;
	}
	*p = '\0';
	*port = strtol( p + 1, NULL, 10 );
	if( ( *port == 0 && errno == EINVAL ) ||
			// RB: 64 bit fixes, changed LONG_ to INT_
			( ( *port == INT_MIN || *port == INT_MAX ) && errno == ERANGE ) )
	{
		// RB end
		return false;
	}
	return true;
}

/*
=============
StringToSockaddr
=============
*/
static bool StringToSockaddr( const char* s, struct sockaddr_in* sadr, bool doDNSResolve )
{
	struct hostent* h;
	char buf[256];
	int port;
	
	memset( sadr, 0, sizeof( *sadr ) );
	sadr->sin_family = AF_INET;
	
	sadr->sin_port = 0;
	
	if( s[0] >= '0' && s[0] <= '9' )
	{
		if( !inet_aton( s, &sadr->sin_addr ) )
		{
			// check for port
			if( !ExtractPort( s, buf, sizeof( buf ), &port ) )
			{
				return false;
			}
			if( !inet_aton( buf, &sadr->sin_addr ) )
			{
				return false;
			}
			sadr->sin_port = htons( port );
		}
	}
	else if( doDNSResolve )
	{
		// try to remove the port first, otherwise the DNS gets confused into multiple timeouts
		// failed or not failed, buf is expected to contain the appropriate host to resolve
		if( ExtractPort( s, buf, sizeof( buf ), &port ) )
		{
			sadr->sin_port = htons( port );
		}
		if( !( h = gethostbyname( buf ) ) )
		{
			return false;
		}
		*( int* ) &sadr->sin_addr =
			*( int* ) h->h_addr_list[0];
	}
	
	return true;
}

/*
=============
Sys_StringToAdr
=============
*/
bool Sys_StringToNetAdr( const char* s, netadr_t* a, bool doDNSResolve )
{
	struct sockaddr_in sadr;
	
	if( !StringToSockaddr( s, &sadr, doDNSResolve ) )
	{
		return false;
	}
	
	SockadrToNetadr( &sadr, a );
	return true;
}

/*
=============
Sys_NetAdrToString
=============
*/
const char* Sys_NetAdrToString( const netadr_t a )
{
	static char s[64];
	
	if( a.type == NA_LOOPBACK )
	{
		if( a.port )
		{
			idStr::snPrintf( s, sizeof( s ), "localhost:%i", a.port );
		}
		else
		{
			idStr::snPrintf( s, sizeof( s ), "localhost" );
		}
	}
	else if( a.type == NA_IP )
	{
		idStr::snPrintf( s, sizeof( s ), "%i.%i.%i.%i:%i",
						 a.ip[0], a.ip[1], a.ip[2], a.ip[3], a.port );
	}
	return s;
}

/*
==================
Sys_IsLANAddress
==================
*/
bool Sys_IsLANAddress( const netadr_t adr )
{
	int i;
	// RB: 64 bit fixes, changed long to int
	unsigned int* p_ip;
	unsigned int ip;
	// RB end
	
#if ID_NOLANADDRESS
	common->Printf( "Sys_IsLANAddress: ID_NOLANADDRESS\n" );
	return false;
#endif
	
	if( adr.type == NA_LOOPBACK )
	{
		return true;
	}
	
	if( adr.type != NA_IP )
	{
		return false;
	}
	
	if( !num_interfaces )
	{
		return false;	// well, if there's no networking, there are no LAN addresses, right
	}
	
	for( i = 0; i < num_interfaces; i++ )
	{
		// RB: 64 bit fixes, changed long to int
		p_ip = ( unsigned int* )&adr.ip[0];
		// RB end
		ip = ntohl( *p_ip );
		if( ( netint[i].ip & netint[i].mask ) == ( ip & netint[i].mask ) )
		{
			return true;
		}
	}
	
	return false;
}

/*
===================
Sys_CompareNetAdrBase

Compares without the port
===================
*/
bool Sys_CompareNetAdrBase( const netadr_t a, const netadr_t b )
{
	if( a.type != b.type )
	{
		return false;
	}
	
	if( a.type == NA_LOOPBACK )
	{
		return true;
	}
	
	if( a.type == NA_IP )
	{
		if( a.ip[0] == b.ip[0] && a.ip[1] == b.ip[1] && a.ip[2] == b.ip[2] && a.ip[3] == b.ip[3] )
		{
			return true;
		}
		return false;
	}
	
	common->Printf( "Sys_CompareNetAdrBase: bad address type\n" );
	return false;
}

/*
====================
NET_InitNetworking
====================
*/
void Sys_InitNetworking()
{
	// haven't been able to clearly pinpoint which standards or RFCs define SIOCGIFCONF, SIOCGIFADDR, SIOCGIFNETMASK ioctls
	// it seems fairly widespread, in Linux kernel ioctl, and in BSD .. so let's assume it's always available on our targets
	
#if MACOS_X
	unsigned int ip, mask;
	struct ifaddrs* ifap, *ifp;
	
	num_interfaces = 0;
	
	if( getifaddrs( &ifap ) < 0 )
	{
		common->FatalError( "InitNetworking: SIOCGIFCONF error - %s\n", strerror( errno ) );
		return;
	}
	
	for( ifp = ifap; ifp; ifp = ifp->ifa_next )
	{
		if( ifp->ifa_addr->sa_family != AF_INET )
			continue;
			
		if( !( ifp->ifa_flags & IFF_UP ) )
			continue;
			
		if( !ifp->ifa_addr )
			continue;
			
		if( !ifp->ifa_netmask )
			continue;
			
		// RB: 64 bit fixes, changed long to int
		ip = ntohl( *( unsigned int* )&ifp->ifa_addr->sa_data[2] );
		mask = ntohl( *( unsigned int* )&ifp->ifa_netmask->sa_data[2] );
		// RB end
		
		if( ip == INADDR_LOOPBACK )
		{
			common->Printf( "loopback\n" );
		}
		else
		{
			common->Printf( "IP: %d.%d.%d.%d\n",
							( unsigned char )ifp->ifa_addr->sa_data[2],
							( unsigned char )ifp->ifa_addr->sa_data[3],
							( unsigned char )ifp->ifa_addr->sa_data[4],
							( unsigned char )ifp->ifa_addr->sa_data[5] );
			common->Printf( "NetMask: %d.%d.%d.%d\n",
							( unsigned char )ifp->ifa_netmask->sa_data[2],
							( unsigned char )ifp->ifa_netmask->sa_data[3],
							( unsigned char )ifp->ifa_netmask->sa_data[4],
							( unsigned char )ifp->ifa_netmask->sa_data[5] );
		}
		netint[ num_interfaces ].ip = ip;
		netint[ num_interfaces ].mask = mask;
		num_interfaces++;
	}
#else
	int		s;
	char	buf[ MAX_INTERFACES * sizeof( ifreq ) ];
	ifconf	ifc;
	ifreq*	ifr;
	int		ifindex;
	unsigned int ip, mask;
	
	num_interfaces = 0;
	
	s = socket( AF_INET, SOCK_DGRAM, 0 );
	ifc.ifc_len = MAX_INTERFACES * sizeof( ifreq );
	ifc.ifc_buf = buf;
	if( ioctl( s, SIOCGIFCONF, &ifc ) < 0 )
	{
		common->FatalError( "InitNetworking: SIOCGIFCONF error - %s\n", strerror( errno ) );
		return;
	}
	ifindex = 0;
	while( ifindex < ifc.ifc_len )
	{
		common->Printf( "found interface %s - ", ifc.ifc_buf + ifindex );
		// find the type - ignore interfaces for which we can find we can't get IP and mask ( not configured )
		ifr = ( ifreq* )( ifc.ifc_buf + ifindex );
		if( ioctl( s, SIOCGIFADDR, ifr ) < 0 )
		{
			common->Printf( "SIOCGIFADDR failed: %s\n", strerror( errno ) );
		}
		else
		{
			if( ifr->ifr_addr.sa_family != AF_INET )
			{
				common->Printf( "not AF_INET\n" );
			}
			else
			{
				// RB: 64 bit fixes, changed long to int
				ip = ntohl( *( unsigned int* )&ifr->ifr_addr.sa_data[2] );
				// RB end
				if( ip == INADDR_LOOPBACK )
				{
					common->Printf( "loopback\n" );
				}
				else
				{
					common->Printf( "%d.%d.%d.%d",
									( unsigned char )ifr->ifr_addr.sa_data[2],
									( unsigned char )ifr->ifr_addr.sa_data[3],
									( unsigned char )ifr->ifr_addr.sa_data[4],
									( unsigned char )ifr->ifr_addr.sa_data[5] );
				}
				if( ioctl( s, SIOCGIFNETMASK, ifr ) < 0 )
				{
					common->Printf( " SIOCGIFNETMASK failed: %s\n", strerror( errno ) );
				}
				else
				{
					// RB: 64 bit fixes, changed long to int
					mask = ntohl( *( unsigned int* )&ifr->ifr_addr.sa_data[2] );
					// RB end
					if( ip != INADDR_LOOPBACK )
					{
						common->Printf( "/%d.%d.%d.%d\n",
										( unsigned char )ifr->ifr_addr.sa_data[2],
										( unsigned char )ifr->ifr_addr.sa_data[3],
										( unsigned char )ifr->ifr_addr.sa_data[4],
										( unsigned char )ifr->ifr_addr.sa_data[5] );
					}
					netint[ num_interfaces ].ip = ip;
					netint[ num_interfaces ].mask = mask;
					num_interfaces++;
				}
			}
		}
		ifindex += sizeof( ifreq );
	}
#endif
}

/*
====================
IPSocket
====================
*/
static int IPSocket( const char* net_interface, int port, netadr_t* bound_to = NULL )
{
	int newsocket;
	struct sockaddr_in address;
	int i = 1;
	
	if( net_interface )
	{
		common->Printf( "Opening IP socket: %s:%i\n", net_interface, port );
	}
	else
	{
		common->Printf( "Opening IP socket: localhost:%i\n", port );
	}
	
	if( ( newsocket = socket( PF_INET, SOCK_DGRAM, IPPROTO_UDP ) ) == -1 )
	{
		common->Printf( "ERROR: IPSocket: socket: %s", strerror( errno ) );
		return 0;
	}
	// make it non-blocking
	int on = 1;
	if( ioctl( newsocket, FIONBIO, &on ) == -1 )
	{
		common->Printf( "ERROR: IPSocket: ioctl FIONBIO:%s\n",
						strerror( errno ) );
		return 0;
	}
	// make it broadcast capable
	if( setsockopt( newsocket, SOL_SOCKET, SO_BROADCAST, ( char* ) &i, sizeof( i ) ) == -1 )
	{
		common->Printf( "ERROR: IPSocket: setsockopt SO_BROADCAST:%s\n", strerror( errno ) );
		return 0;
	}
	
	if( !net_interface || !net_interface[ 0 ]
			|| !idStr::Icmp( net_interface, "localhost" ) )
	{
		address.sin_addr.s_addr = INADDR_ANY;
	}
	else
	{
		StringToSockaddr( net_interface, &address, true );
	}
	
	if( port == PORT_ANY )
	{
		address.sin_port = 0;
	}
	else
	{
		address.sin_port = htons( ( short ) port );
	}
	
	address.sin_family = AF_INET;
	
	if( bind( newsocket, ( const struct sockaddr* )&address, sizeof( address ) ) == -1 )
	{
		common->Printf( "ERROR: IPSocket: bind: %s\n", strerror( errno ) );
		close( newsocket );
		return 0;
	}
	
	if( bound_to )
	{
		unsigned int len = sizeof( address );
		if( ( unsigned int )( getsockname( newsocket, ( struct sockaddr* )&address, ( socklen_t* )&len ) ) == -1 )
		{
			common->Printf( "ERROR: IPSocket: getsockname: %s\n", strerror( errno ) );
			close( newsocket );
			return 0;
		}
		SockadrToNetadr( &address, bound_to );
	}
	
	return newsocket;
}

/*
==================
idPort::idPort
==================
*/
idPort::idPort()
{
	netSocket = 0;
	memset( &bound_to, 0, sizeof( bound_to ) );
}

/*
==================
idPort::~idPort
==================
*/
idPort::~idPort()
{
	Close();
}

/*
==================
idPort::Close
==================
*/
void idPort::Close()
{
	if( netSocket )
	{
		close( netSocket );
		netSocket = 0;
		memset( &bound_to, 0, sizeof( bound_to ) );
	}
}

/*
==================
idPort::GetPacket
==================
*/
bool idPort::GetPacket( netadr_t& net_from, void* data, int& size, int maxSize )
{
	int ret;
	struct sockaddr_in from;
	int fromlen;
	
	if( !netSocket )
	{
		return false;
	}
	
	fromlen = sizeof( from );
	ret = recvfrom( netSocket, data, maxSize, 0, ( struct sockaddr* ) &from, ( socklen_t* ) &fromlen );
	
	if( ret == -1 )
	{
		if( errno == EWOULDBLOCK || errno == ECONNREFUSED )
		{
			// those commonly happen, don't verbose
			return false;
		}
		common->DPrintf( "idPort::GetPacket recvfrom(): %s\n", strerror( errno ) );
		return false;
	}
	
	assert( ret < maxSize );
	
	SockadrToNetadr( &from, &net_from );
	size = ret;
	return true;
}

/*
==================
idPort::GetPacketBlocking
==================
*/
bool idPort::GetPacketBlocking( netadr_t& net_from, void* data, int& size, int maxSize, int timeout )
{
	fd_set				set;
	struct timeval		tv;
	int					ret;
	
	if( !netSocket )
	{
		return false;
	}
	
	if( timeout < 0 )
	{
		return GetPacket( net_from, data, size, maxSize );
	}
	
	FD_ZERO( &set );
	FD_SET( netSocket, &set );
	
	tv.tv_sec = timeout / 1000;
	tv.tv_usec = ( timeout % 1000 ) * 1000;
	ret = select( netSocket + 1, &set, NULL, NULL, &tv );
	if( ret == -1 )
	{
		if( errno == EINTR )
		{
			common->DPrintf( "idPort::GetPacketBlocking: select EINTR\n" );
			return false;
		}
		else
		{
			common->Error( "idPort::GetPacketBlocking: select failed: %s\n", strerror( errno ) );
		}
	}
	
	if( ret == 0 )
	{
		// timed out
		return false;
	}
	struct sockaddr_in from;
	int fromlen;
	fromlen = sizeof( from );
	ret = recvfrom( netSocket, data, maxSize, 0, ( struct sockaddr* )&from, ( socklen_t* )&fromlen );
	if( ret == -1 )
	{
		// there should be no blocking errors once select declares things are good
		common->DPrintf( "idPort::GetPacketBlocking: %s\n", strerror( errno ) );
		return false;
	}
	assert( ret < maxSize );
	SockadrToNetadr( &from, &net_from );
	size = ret;
	return true;
}

/*
==================
idPort::SendPacket
==================
*/
void idPort::SendPacket( const netadr_t to, const void* data, int size )
{
	int ret;
	struct sockaddr_in addr;
	
	if( to.type == NA_BAD )
	{
		common->Warning( "idPort::SendPacket: bad address type NA_BAD - ignored" );
		return;
	}
	
	if( !netSocket )
	{
		return;
	}
	
	NetadrToSockadr( &to, &addr );
	
	ret = sendto( netSocket, data, size, 0, ( struct sockaddr* ) &addr, sizeof( addr ) );
	if( ret == -1 )
	{
		common->Printf( "idPort::SendPacket ERROR: to %s: %s\n", Sys_NetAdrToString( to ), strerror( errno ) );
	}
}

/*
==================
idPort::InitForPort
==================
*/
bool idPort::InitForPort( int portNumber )
{
	netSocket = IPSocket( net_ip.GetString(), portNumber, &bound_to );
	if( netSocket <= 0 )
	{
		netSocket = 0;
		memset( &bound_to, 0, sizeof( bound_to ) );
		return false;
	}
	return true;
}

//=============================================================================

/*
==================
idTCP::idTCP
==================
*/
idTCP::idTCP()
{
	fd = 0;
	memset( &address, 0, sizeof( address ) );
}

/*
==================
idTCP::~idTCP
==================
*/
idTCP::~idTCP()
{
	Close();
}

/*
==================
idTCP::Init
==================
*/
bool idTCP::Init( const char* host, short port )
{
	struct sockaddr_in sadr;
	if( !Sys_StringToNetAdr( host, &address, true ) )
	{
		common->Printf( "Couldn't resolve server name \"%s\"\n", host );
		return false;
	}
	address.type = NA_IP;
	if( !address.port )
	{
		address.port = port;
	}
	common->Printf( "\"%s\" resolved to %i.%i.%i.%i:%i\n", host,
					address.ip[0], address.ip[1], address.ip[2], address.ip[3],  address.port );
	NetadrToSockadr( &address, &sadr );
	
	if( fd )
	{
		common->Warning( "idTCP::Init: already initialized?\n" );
	}
	
	if( ( fd = socket( PF_INET, SOCK_STREAM, 0 ) ) == -1 )
	{
		fd = 0;
		common->Printf( "ERROR: idTCP::Init: socket: %s\n", strerror( errno ) );
		return false;
	}
	
	if( connect( fd, ( const sockaddr* )&sadr, sizeof( sadr ) ) == -1 )
	{
		common->Printf( "ERROR: idTCP::Init: connect: %s\n", strerror( errno ) );
		close( fd );
		fd = 0;
		return false;
	}
	
	int status;
	if( ( status = fcntl( fd, F_GETFL, 0 ) ) != -1 )
	{
		status |= O_NONBLOCK; /* POSIX */
		status = fcntl( fd, F_SETFL, status );
	}
	if( status == -1 )
	{
		common->Printf( "ERROR: idTCP::Init: fcntl / O_NONBLOCK: %s\n", strerror( errno ) );
		close( fd );
		fd = 0;
		return false;
	}
	
	common->DPrintf( "Opened TCP connection\n" );
	return true;
}

/*
==================
idTCP::Close
==================
*/
void idTCP::Close()
{
	if( fd )
	{
		close( fd );
	}
	fd = 0;
}

/*
==================
idTCP::Read
==================
*/
int idTCP::Read( void* data, int size )
{
	int nbytes;
	
	if( !fd )
	{
		common->Printf( "idTCP::Read: not initialized\n" );
		return -1;
	}
	
#if defined(_GNU_SOURCE)
	// handle EINTR interrupted system call with TEMP_FAILURE_RETRY -  this is probably GNU libc specific
	if( ( nbytes = TEMP_FAILURE_RETRY( read( fd, data, size ) ) ) == -1 )
	{
#else
	do
	{
		nbytes = read( fd, data, size );
	}
	while( nbytes == -1 && errno == EINTR );
	if( nbytes == -1 )
	{
#endif
		if( errno == EAGAIN )
		{
			return 0;
		}
		common->Printf( "ERROR: idTCP::Read: %s\n", strerror( errno ) );
		Close();
		return -1;
	}
	
	// a successful read of 0 bytes indicates remote has closed the connection
	if( nbytes == 0 )
	{
		common->DPrintf( "idTCP::Read: read 0 bytes - assume connection closed\n" );
		return -1;
	}
	
	return nbytes;
}

/*
==================
idTCP::Write
==================
*/

static void got_SIGPIPE( int signum )
{
	common->Printf( "idTCP: SIGPIPE\n" );
}

int	idTCP::Write( void* data, int size )
{
	int nbytes;
	
	if( !fd )
	{
		common->Printf( "idTCP::Write: not initialized\n" );
		return -1;
	}
	
	struct sigaction bak_action;
	struct sigaction action;
	
	action.sa_handler = got_SIGPIPE;
	sigemptyset( &action.sa_mask );
	action.sa_flags = 0;
	
	if( sigaction( SIGPIPE, &action, &bak_action ) != 0 )
	{
		common->Printf( "ERROR: idTCP::Write: failed to set temporary SIGPIPE handler\n" );
		Close();
		return -1;
	}
	
#if defined(_GNU_SOURCE)
	// handle EINTR interrupted system call with TEMP_FAILURE_RETRY -  this is probably GNU libc specific
	if( ( nbytes = TEMP_FAILURE_RETRY( write( fd, data, size ) ) ) == -1 )
	{
#else
	do
	{
		nbytes = write( fd, data, size );
	}
	while( nbytes == -1 && errno == EINTR );
	if( nbytes == -1 )
	{
#endif
		common->Printf( "ERROR: idTCP::Write: %s\n", strerror( errno ) );
		Close();
		return -1;
	}
	
	if( sigaction( SIGPIPE, &bak_action, NULL ) != 0 )
	{
		common->Printf( "ERROR: idTCP::Write: failed to reset SIGPIPE handler\n" );
		Close();
		return -1;
	}
	
	return nbytes;
}
