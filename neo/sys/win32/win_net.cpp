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

/*
================================================================================================
Contains the NetworkSystem implementation specific to Win32.
================================================================================================
*/

#include <iptypes.h>
#include <iphlpapi.h>

static WSADATA	winsockdata;
static bool	winsockInitialized = false;
static bool usingSocks = false;

//lint -e569	ioctl macros trigger this

// force these libs to be included, so users of idLib don't need to add them to every project
#pragma comment(lib, "iphlpapi.lib" )
#pragma comment(lib, "wsock32.lib" )


/*
================================================================================================

	Network CVars

================================================================================================
*/

idCVar net_socksServer( "net_socksServer", "", CVAR_ARCHIVE, "" );
idCVar net_socksPort( "net_socksPort", "1080", CVAR_ARCHIVE | CVAR_INTEGER, "" );
idCVar net_socksUsername( "net_socksUsername", "", CVAR_ARCHIVE, "" );
idCVar net_socksPassword( "net_socksPassword", "", CVAR_ARCHIVE, "" );

idCVar net_ip( "net_ip", "localhost", 0, "local IP address" );

static struct sockaddr_in	socksRelayAddr;

static SOCKET	ip_socket;
static SOCKET	socks_socket;
static char		socksBuf[4096];

typedef struct {
	unsigned long ip;
	unsigned long mask;
	char addr[16];
} net_interface;

#define 		MAX_INTERFACES	32
int				num_interfaces = 0;
net_interface	netint[MAX_INTERFACES];

/*
================================================================================================

	Free Functions

================================================================================================
*/

/*
========================
NET_ErrorString
========================
*/
char *NET_ErrorString() {
	int		code;

	code = WSAGetLastError();
	switch( code ) {
	case WSAEINTR: return "WSAEINTR";
	case WSAEBADF: return "WSAEBADF";
	case WSAEACCES: return "WSAEACCES";
	case WSAEDISCON: return "WSAEDISCON";
	case WSAEFAULT: return "WSAEFAULT";
	case WSAEINVAL: return "WSAEINVAL";
	case WSAEMFILE: return "WSAEMFILE";
	case WSAEWOULDBLOCK: return "WSAEWOULDBLOCK";
	case WSAEINPROGRESS: return "WSAEINPROGRESS";
	case WSAEALREADY: return "WSAEALREADY";
	case WSAENOTSOCK: return "WSAENOTSOCK";
	case WSAEDESTADDRREQ: return "WSAEDESTADDRREQ";
	case WSAEMSGSIZE: return "WSAEMSGSIZE";
	case WSAEPROTOTYPE: return "WSAEPROTOTYPE";
	case WSAENOPROTOOPT: return "WSAENOPROTOOPT";
	case WSAEPROTONOSUPPORT: return "WSAEPROTONOSUPPORT";
	case WSAESOCKTNOSUPPORT: return "WSAESOCKTNOSUPPORT";
	case WSAEOPNOTSUPP: return "WSAEOPNOTSUPP";
	case WSAEPFNOSUPPORT: return "WSAEPFNOSUPPORT";
	case WSAEAFNOSUPPORT: return "WSAEAFNOSUPPORT";
	case WSAEADDRINUSE: return "WSAEADDRINUSE";
	case WSAEADDRNOTAVAIL: return "WSAEADDRNOTAVAIL";
	case WSAENETDOWN: return "WSAENETDOWN";
	case WSAENETUNREACH: return "WSAENETUNREACH";
	case WSAENETRESET: return "WSAENETRESET";
	case WSAECONNABORTED: return "WSAECONNABORTED";
	case WSAECONNRESET: return "WSAECONNRESET";
	case WSAENOBUFS: return "WSAENOBUFS";
	case WSAEISCONN: return "WSAEISCONN";
	case WSAENOTCONN: return "WSAENOTCONN";
	case WSAESHUTDOWN: return "WSAESHUTDOWN";
	case WSAETOOMANYREFS: return "WSAETOOMANYREFS";
	case WSAETIMEDOUT: return "WSAETIMEDOUT";
	case WSAECONNREFUSED: return "WSAECONNREFUSED";
	case WSAELOOP: return "WSAELOOP";
	case WSAENAMETOOLONG: return "WSAENAMETOOLONG";
	case WSAEHOSTDOWN: return "WSAEHOSTDOWN";
	case WSASYSNOTREADY: return "WSASYSNOTREADY";
	case WSAVERNOTSUPPORTED: return "WSAVERNOTSUPPORTED";
	case WSANOTINITIALISED: return "WSANOTINITIALISED";
	case WSAHOST_NOT_FOUND: return "WSAHOST_NOT_FOUND";
	case WSATRY_AGAIN: return "WSATRY_AGAIN";
	case WSANO_RECOVERY: return "WSANO_RECOVERY";
	case WSANO_DATA: return "WSANO_DATA";
	default: return "NO ERROR";
	}
}

/*
========================
Net_NetadrToSockadr
========================
*/
void Net_NetadrToSockadr( const netadr_t *a, sockaddr_in *s ) {
	memset( s, 0, sizeof(*s) );

	if ( a->type == NA_BROADCAST ) {
		s->sin_family = AF_INET;
		s->sin_addr.s_addr = INADDR_BROADCAST;
	} else if ( a->type == NA_IP || a->type == NA_LOOPBACK ) {
		s->sin_family = AF_INET;
		s->sin_addr.s_addr = *(int *)a->ip;
	}

	s->sin_port = htons( (short)a->port );
}

/*
========================
Net_SockadrToNetadr
========================
*/
void Net_SockadrToNetadr( sockaddr_in *s, netadr_t *a ) {
	unsigned int ip;
	if ( s->sin_family == AF_INET ) {
		ip = s->sin_addr.s_addr;
		*(unsigned int *)a->ip = ip;
		a->port = htons( s->sin_port );
		// we store in network order, that loopback test is host order..
		ip = ntohl( ip );
		if ( ip == INADDR_LOOPBACK ) {
			a->type = NA_LOOPBACK;
		} else {
			a->type = NA_IP;
		}
	}
}

/*
========================
Net_ExtractPort
========================
*/
static bool Net_ExtractPort( const char *src, char *buf, int bufsize, int *port ) {
	char *p;
	strncpy( buf, src, bufsize );
	p = buf; p += Min( bufsize - 1, idStr::Length( src ) ); *p = '\0';
	p = strchr( buf, ':' );
	if ( !p ) {
		return false;
	}
	*p = '\0';
	*port = strtol( p+1, NULL, 10 );
	if ( errno == ERANGE ) {
		return false;
	}
	return true;
}

/*
========================
Net_StringToSockaddr
========================
*/
static bool Net_StringToSockaddr( const char *s, sockaddr_in *sadr, bool doDNSResolve ) {
	struct hostent	*h;
	char buf[256];
	int port;
	
	memset( sadr, 0, sizeof( *sadr ) );

	sadr->sin_family = AF_INET;
	sadr->sin_port = 0;

	if( s[0] >= '0' && s[0] <= '9' ) {
		unsigned long ret = inet_addr(s);
		if ( ret != INADDR_NONE ) {
			*(int *)&sadr->sin_addr = ret;
		} else {
			// check for port
			if ( !Net_ExtractPort( s, buf, sizeof( buf ), &port ) ) {
				return false;
			}
			ret = inet_addr( buf );
			if ( ret == INADDR_NONE ) {
				return false;
			}
			*(int *)&sadr->sin_addr = ret;
			sadr->sin_port = htons( port );
		}
	} else if ( doDNSResolve ) {
		// try to remove the port first, otherwise the DNS gets confused into multiple timeouts
		// failed or not failed, buf is expected to contain the appropriate host to resolve
		if ( Net_ExtractPort( s, buf, sizeof( buf ), &port ) ) {
			sadr->sin_port = htons( port );			
		}
		h = gethostbyname( buf );
		if ( h == 0 ) {
			return false;
		}
		*(int *)&sadr->sin_addr = *(int *)h->h_addr_list[0];
	}
	
	return true;
}

/*
========================
NET_IPSocket
========================
*/
int NET_IPSocket( const char *net_interface, int port, netadr_t *bound_to ) {
	SOCKET				newsocket;
	sockaddr_in			address;
	unsigned long		_true = 1;
	int					i = 1;
	int					err;

	if ( port != PORT_ANY ) {
		if( net_interface ) {
			idLib::Printf( "Opening IP socket: %s:%i\n", net_interface, port );
		} else {
			idLib::Printf( "Opening IP socket: localhost:%i\n", port );
		}
	}

	if( ( newsocket = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP ) ) == INVALID_SOCKET ) {
		err = WSAGetLastError();
		if( err != WSAEAFNOSUPPORT ) {
			idLib::Printf( "WARNING: UDP_OpenSocket: socket: %s\n", NET_ErrorString() );
		}
		return 0;
	}

	// make it non-blocking
	if( ioctlsocket( newsocket, FIONBIO, &_true ) == SOCKET_ERROR ) {
		idLib::Printf( "WARNING: UDP_OpenSocket: ioctl FIONBIO: %s\n", NET_ErrorString() );
		closesocket( newsocket );
		return 0;
	}

	// make it broadcast capable
	if( setsockopt( newsocket, SOL_SOCKET, SO_BROADCAST, (char *)&i, sizeof(i) ) == SOCKET_ERROR ) {
		idLib::Printf( "WARNING: UDP_OpenSocket: setsockopt SO_BROADCAST: %s\n", NET_ErrorString() );
		closesocket( newsocket );
		return 0;
	}

	if( !net_interface || !net_interface[0] || !idStr::Icmp( net_interface, "localhost" ) ) {
		address.sin_addr.s_addr = INADDR_ANY;
	}
	else {
		Net_StringToSockaddr( net_interface, &address, true );
	}

	if( port == PORT_ANY ) {
		address.sin_port = 0;
	}
	else {
		address.sin_port = htons( (short)port );
	}

	address.sin_family = AF_INET;

	if( bind( newsocket, (const sockaddr *)&address, sizeof(address) ) == SOCKET_ERROR ) {
		idLib::Printf( "WARNING: UDP_OpenSocket: bind: %s\n", NET_ErrorString() );
		closesocket( newsocket );
		return 0;
	}

	// if the port was PORT_ANY, we need to query again to know the real port we got bound to
	// ( this used to be in idUDP::InitForPort )
	if ( bound_to ) {
		int len = sizeof( address );
		getsockname( newsocket, (sockaddr *)&address, &len );
		Net_SockadrToNetadr( &address, bound_to );
	}

	return newsocket;
}

/*
========================
NET_OpenSocks
========================
*/
void NET_OpenSocks( int port ) {
	sockaddr_in			address;
	struct hostent		*h;
	int					len;
	bool				rfc1929;
	unsigned char		buf[64];

	usingSocks = false;

	idLib::Printf( "Opening connection to SOCKS server.\n" );

	if ( ( socks_socket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP ) ) == INVALID_SOCKET ) {
		idLib::Printf( "WARNING: NET_OpenSocks: socket: %s\n", NET_ErrorString() );
		return;
	}

	h = gethostbyname( net_socksServer.GetString() );
	if ( h == NULL ) {
		idLib::Printf( "WARNING: NET_OpenSocks: gethostbyname: %s\n", NET_ErrorString() );
		return;
	}
	if ( h->h_addrtype != AF_INET ) {
		idLib::Printf( "WARNING: NET_OpenSocks: gethostbyname: address type was not AF_INET\n" );
		return;
	}
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = *(int *)h->h_addr_list[0];
	address.sin_port = htons( (short)net_socksPort.GetInteger() );

	if ( connect( socks_socket, (sockaddr *)&address, sizeof( address ) ) == SOCKET_ERROR ) {
		idLib::Printf( "NET_OpenSocks: connect: %s\n", NET_ErrorString() );
		return;
	}

	// send socks authentication handshake
	if ( *net_socksUsername.GetString() || *net_socksPassword.GetString() ) {
		rfc1929 = true;
	}
	else {
		rfc1929 = false;
	}

	buf[0] = 5;		// SOCKS version
	// method count
	if ( rfc1929 ) {
		buf[1] = 2;
		len = 4;
	}
	else {
		buf[1] = 1;
		len = 3;
	}
	buf[2] = 0;		// method #1 - method id #00: no authentication
	if ( rfc1929 ) {
		buf[2] = 2;		// method #2 - method id #02: username/password
	}
	if ( send( socks_socket, (const char *)buf, len, 0 ) == SOCKET_ERROR ) {
		idLib::Printf( "NET_OpenSocks: send: %s\n", NET_ErrorString() );
		return;
	}

	// get the response
	len = recv( socks_socket, (char *)buf, 64, 0 );
	if ( len == SOCKET_ERROR ) {
		idLib::Printf( "NET_OpenSocks: recv: %s\n", NET_ErrorString() );
		return;
	}
	if ( len != 2 || buf[0] != 5 ) {
		idLib::Printf( "NET_OpenSocks: bad response\n" );
		return;
	}
	switch( buf[1] ) {
	case 0:	// no authentication
		break;
	case 2: // username/password authentication
		break;
	default:
		idLib::Printf( "NET_OpenSocks: request denied\n" );
		return;
	}

	// do username/password authentication if needed
	if ( buf[1] == 2 ) {
		int		ulen;
		int		plen;

		// build the request
		ulen = idStr::Length( net_socksUsername.GetString() );
		plen = idStr::Length( net_socksPassword.GetString() );

		buf[0] = 1;		// username/password authentication version
		buf[1] = ulen;
		if ( ulen ) {
			memcpy( &buf[2], net_socksUsername.GetString(), ulen );
		}
		buf[2 + ulen] = plen;
		if ( plen ) {
			memcpy( &buf[3 + ulen], net_socksPassword.GetString(), plen );
		}

		// send it
		if ( send( socks_socket, (const char *)buf, 3 + ulen + plen, 0 ) == SOCKET_ERROR ) {
			idLib::Printf( "NET_OpenSocks: send: %s\n", NET_ErrorString() );
			return;
		}

		// get the response
		len = recv( socks_socket, (char *)buf, 64, 0 );
		if ( len == SOCKET_ERROR ) {
			idLib::Printf( "NET_OpenSocks: recv: %s\n", NET_ErrorString() );
			return;
		}
		if ( len != 2 || buf[0] != 1 ) {
			idLib::Printf( "NET_OpenSocks: bad response\n" );
			return;
		}
		if ( buf[1] != 0 ) {
			idLib::Printf( "NET_OpenSocks: authentication failed\n" );
			return;
		}
	}

	// send the UDP associate request
	buf[0] = 5;		// SOCKS version
	buf[1] = 3;		// command: UDP associate
	buf[2] = 0;		// reserved
	buf[3] = 1;		// address type: IPV4
	*(int *)&buf[4] = INADDR_ANY;
	*(short *)&buf[8] = htons( (short)port );		// port
	if ( send( socks_socket, (const char *)buf, 10, 0 ) == SOCKET_ERROR ) {
		idLib::Printf( "NET_OpenSocks: send: %s\n", NET_ErrorString() );
		return;
	}

	// get the response
	len = recv( socks_socket, (char *)buf, 64, 0 );
	if( len == SOCKET_ERROR ) {
		idLib::Printf( "NET_OpenSocks: recv: %s\n", NET_ErrorString() );
		return;
	}
	if( len < 2 || buf[0] != 5 ) {
		idLib::Printf( "NET_OpenSocks: bad response\n" );
		return;
	}
	// check completion code
	if( buf[1] != 0 ) {
		idLib::Printf( "NET_OpenSocks: request denied: %i\n", buf[1] );
		return;
	}
	if( buf[3] != 1 ) {
		idLib::Printf( "NET_OpenSocks: relay address is not IPV4: %i\n", buf[3] );
		return;
	}
	socksRelayAddr.sin_family = AF_INET;
	socksRelayAddr.sin_addr.s_addr = *(int *)&buf[4];
	socksRelayAddr.sin_port = *(short *)&buf[8];
	memset( socksRelayAddr.sin_zero, 0, sizeof( socksRelayAddr.sin_zero ) );

	usingSocks = true;
}

/*
========================
Net_WaitForData
========================
*/
bool Net_WaitForData( int netSocket, int timeout ) {
	int					ret;
	fd_set				set;
	struct timeval		tv;

	if ( !netSocket ) {
		return false;
	}

	if ( timeout < 0 ) {
		return true;
	}

	FD_ZERO( &set );
	FD_SET( static_cast<unsigned int>( netSocket ), &set );

	tv.tv_sec = 0;
	tv.tv_usec = timeout * 1000;

	ret = select( netSocket + 1, &set, NULL, NULL, &tv );

	if ( ret == -1 ) {
		idLib::Printf( "Net_WaitForData select(): %s\n", strerror( errno ) );
		return false;
	}

	// timeout with no data
	if ( ret == 0 ) {
		return false;
	}

	return true;
}

/*
========================
Net_GetUDPPacket
========================
*/
bool Net_GetUDPPacket( int netSocket, netadr_t &net_from, char *data, int &size, int maxSize ) {
	int 			ret;
	sockaddr_in		from;
	int				fromlen;
	int				err;

	if ( !netSocket ) {
		return false;
	}

	fromlen = sizeof(from);
	ret = recvfrom( netSocket, data, maxSize, 0, (sockaddr *)&from, &fromlen );
	if ( ret == SOCKET_ERROR ) {
		err = WSAGetLastError();

		if ( err == WSAEWOULDBLOCK || err == WSAECONNRESET ) {
			return false;
		}
		char	buf[1024];
		sprintf( buf, "Net_GetUDPPacket: %s\n", NET_ErrorString() );
		idLib::Printf( buf );
		return false;
	}

	if ( static_cast<unsigned int>( netSocket ) == ip_socket ) {
		memset( from.sin_zero, 0, sizeof( from.sin_zero ) );
	}

	if ( usingSocks && static_cast<unsigned int>( netSocket ) == ip_socket && memcmp( &from, &socksRelayAddr, fromlen ) == 0 ) {
		if ( ret < 10 || data[0] != 0 || data[1] != 0 || data[2] != 0 || data[3] != 1 ) {
			return false;
		}
		net_from.type = NA_IP;
		net_from.ip[0] = data[4];
		net_from.ip[1] = data[5];
		net_from.ip[2] = data[6];
		net_from.ip[3] = data[7];
		net_from.port = *(short *)&data[8];
		memmove( data, &data[10], ret - 10 );
	} else {
		Net_SockadrToNetadr( &from, &net_from );
	}

	if ( ret > maxSize ) {
		char	buf[1024];
		sprintf( buf, "Net_GetUDPPacket: oversize packet from %s\n", Sys_NetAdrToString( net_from ) );
		idLib::Printf( buf );
		return false;
	}

	size = ret;

	return true;
}

/*
========================
Net_SendUDPPacket
========================
*/
void Net_SendUDPPacket( int netSocket, int length, const void *data, const netadr_t to ) {
	int				ret;
	sockaddr_in		addr;

	if ( !netSocket ) {
		return;
	}

	Net_NetadrToSockadr( &to, &addr );

	if ( usingSocks && to.type == NA_IP ) {
		socksBuf[0] = 0;	// reserved
		socksBuf[1] = 0;
		socksBuf[2] = 0;	// fragment (not fragmented)
		socksBuf[3] = 1;	// address type: IPV4
		*(int *)&socksBuf[4] = addr.sin_addr.s_addr;
		*(short *)&socksBuf[8] = addr.sin_port;
		memcpy( &socksBuf[10], data, length );
		ret = sendto( netSocket, socksBuf, length+10, 0, (sockaddr *)&socksRelayAddr, sizeof(socksRelayAddr) );
	} else {
		ret = sendto( netSocket, (const char *)data, length, 0, (sockaddr *)&addr, sizeof(addr) );
	}
	if ( ret == SOCKET_ERROR ) {
		int err = WSAGetLastError();

		// some PPP links do not allow broadcasts and return an error
		if ( ( err == WSAEADDRNOTAVAIL ) && ( to.type == NA_BROADCAST ) ) {
			return;
		}

		// NOTE: WSAEWOULDBLOCK used to be silently ignored,
		// but that means the packet will be dropped so I don't feel it's a good thing to ignore
		idLib::Printf( "UDP sendto error - packet dropped: %s\n", NET_ErrorString() );
	}
}

/*
========================
Sys_InitNetworking
========================
*/
void Sys_InitNetworking() {
	int		r;

	if ( winsockInitialized ) {
		return;
	}
	r = WSAStartup( MAKEWORD( 1, 1 ), &winsockdata );
	if( r ) {
		idLib::Printf( "WARNING: Winsock initialization failed, returned %d\n", r );
		return;
	}

	winsockInitialized = true;
	idLib::Printf( "Winsock Initialized\n" );

	PIP_ADAPTER_INFO pAdapterInfo;
	PIP_ADAPTER_INFO pAdapter = NULL;
	DWORD dwRetVal = 0;
	PIP_ADDR_STRING pIPAddrString;
	ULONG ulOutBufLen;
	bool foundloopback;

	num_interfaces = 0;
	foundloopback = false;

	pAdapterInfo = (IP_ADAPTER_INFO *)malloc( sizeof( IP_ADAPTER_INFO ) );
	if( !pAdapterInfo ) {
		idLib::FatalError( "Sys_InitNetworking: Couldn't malloc( %d )", sizeof( IP_ADAPTER_INFO ) );
	}
	ulOutBufLen = sizeof( IP_ADAPTER_INFO );

	// Make an initial call to GetAdaptersInfo to get
	// the necessary size into the ulOutBufLen variable
	if( GetAdaptersInfo( pAdapterInfo, &ulOutBufLen ) == ERROR_BUFFER_OVERFLOW ) {
		free( pAdapterInfo );
		pAdapterInfo = (IP_ADAPTER_INFO *)malloc( ulOutBufLen ); 
		if( !pAdapterInfo ) {
			idLib::FatalError( "Sys_InitNetworking: Couldn't malloc( %ld )", ulOutBufLen );
		}
	}

	if( ( dwRetVal = GetAdaptersInfo( pAdapterInfo, &ulOutBufLen) ) != NO_ERROR ) {
		// happens if you have no network connection
		idLib::Printf( "Sys_InitNetworking: GetAdaptersInfo failed (%ld).\n", dwRetVal );
	} else {
		pAdapter = pAdapterInfo;
		while( pAdapter ) {
			idLib::Printf( "Found interface: %s %s - ", pAdapter->AdapterName, pAdapter->Description );
			pIPAddrString = &pAdapter->IpAddressList;
			while( pIPAddrString ) {
				unsigned long ip_a, ip_m;
				if( !idStr::Icmp( "127.0.0.1", pIPAddrString->IpAddress.String ) ) {
					foundloopback = true;
				}
				ip_a = ntohl( inet_addr( pIPAddrString->IpAddress.String ) );
				ip_m = ntohl( inet_addr( pIPAddrString->IpMask.String ) );
				//skip null netmasks
				if( !ip_m ) {
					idLib::Printf( "%s NULL netmask - skipped\n", pIPAddrString->IpAddress.String );
					pIPAddrString = pIPAddrString->Next;
					continue;
				}
				idLib::Printf( "%s/%s\n", pIPAddrString->IpAddress.String, pIPAddrString->IpMask.String );
				netint[num_interfaces].ip = ip_a;
				netint[num_interfaces].mask = ip_m;
				idStr::Copynz( netint[num_interfaces].addr, pIPAddrString->IpAddress.String, sizeof( netint[num_interfaces].addr ) );
				num_interfaces++;
				if( num_interfaces >= MAX_INTERFACES ) {
					idLib::Printf( "Sys_InitNetworking: MAX_INTERFACES(%d) hit.\n", MAX_INTERFACES );
					free( pAdapterInfo );
					return;
				}
				pIPAddrString = pIPAddrString->Next;
			}
			pAdapter = pAdapter->Next;
		}
	}
	// for some retarded reason, win32 doesn't count loopback as an adapter...
	if( !foundloopback && num_interfaces < MAX_INTERFACES ) {
		idLib::Printf( "Sys_InitNetworking: adding loopback interface\n" );
		netint[num_interfaces].ip = ntohl( inet_addr( "127.0.0.1" ) );
		netint[num_interfaces].mask = ntohl( inet_addr( "255.0.0.0" ) );
		num_interfaces++;
	}
	free( pAdapterInfo );
}

/*
========================
Sys_ShutdownNetworking
========================
*/
void Sys_ShutdownNetworking() {
	if ( !winsockInitialized ) {
		return;
	}
	WSACleanup();
	winsockInitialized = false;
}

/*
========================
Sys_StringToNetAdr
========================
*/
bool Sys_StringToNetAdr( const char *s, netadr_t *a, bool doDNSResolve ) {
	sockaddr_in sadr;
	
	if ( !Net_StringToSockaddr( s, &sadr, doDNSResolve ) ) {
		return false;
	}
	
	Net_SockadrToNetadr( &sadr, a );
	return true;
}

/*
========================
Sys_NetAdrToString
========================
*/
const char *Sys_NetAdrToString( const netadr_t a ) {
	static int index = 0;
	static char buf[ 4 ][ 64 ];	// flip/flop
	char *s;

	s = buf[index];
	index = (index + 1) & 3;

	if ( a.type == NA_LOOPBACK ) {
		if ( a.port ) {
			idStr::snPrintf( s, 64, "localhost:%i", a.port );
		} else {
			idStr::snPrintf( s, 64, "localhost" );
		}
	} else if ( a.type == NA_IP ) {
		idStr::snPrintf( s, 64, "%i.%i.%i.%i:%i", a.ip[0], a.ip[1], a.ip[2], a.ip[3], a.port );
	}
	return s;
}

/*
========================
Sys_IsLANAddress
========================
*/
bool Sys_IsLANAddress( const netadr_t adr ) {
	if ( adr.type == NA_LOOPBACK ) {
		return true;
	}

	if ( adr.type != NA_IP ) {
		return false;
	}

	if ( num_interfaces ) {
		int i;
		unsigned long *p_ip;
		unsigned long ip;
		p_ip = (unsigned long *)&adr.ip[0];
		ip = ntohl( *p_ip );

		for( i = 0; i < num_interfaces; i++ ) {
			if( ( netint[i].ip & netint[i].mask ) == ( ip & netint[i].mask ) ) {
				return true;
			}
		}
	}
	return false;
}

/*
========================
Sys_CompareNetAdrBase

Compares without the port.
========================
*/
bool Sys_CompareNetAdrBase( const netadr_t a, const netadr_t b ) {
	if ( a.type != b.type ) {
		return false;
	}

	if ( a.type == NA_LOOPBACK ) {
		if ( a.port == b.port ) {
			return true;
		}

		return false;
	}

	if ( a.type == NA_IP ) {
		if ( a.ip[0] == b.ip[0] && a.ip[1] == b.ip[1] && a.ip[2] == b.ip[2] && a.ip[3] == b.ip[3] ) {
			return true;
		}
		return false;
	}

	idLib::Printf( "Sys_CompareNetAdrBase: bad address type\n" );
	return false;
}

/*
========================
Sys_GetLocalIPCount
========================
*/
int	Sys_GetLocalIPCount() {
	return num_interfaces;
}

/*
========================
Sys_GetLocalIP
========================
*/
const char * Sys_GetLocalIP( int i ) {
	if ( ( i < 0 ) || ( i >= num_interfaces ) ) {
		return NULL;
	}
	return netint[i].addr;
}

/*
================================================================================================

	idUDP

================================================================================================
*/

/*
========================
idUDP::idUDP
========================
*/
idUDP::idUDP() {
	netSocket = 0;
	memset( &bound_to, 0, sizeof( bound_to ) );
	silent = false;
	packetsRead = 0;
	bytesRead = 0;
	packetsWritten = 0;
	bytesWritten = 0;
}

/*
========================
idUDP::~idUDP
========================
*/
idUDP::~idUDP() {
	Close();
}

/*
========================
idUDP::InitForPort
========================
*/
bool idUDP::InitForPort( int portNumber ) {
	netSocket = NET_IPSocket( net_ip.GetString(), portNumber, &bound_to );
	if ( netSocket <= 0 ) {
		netSocket = 0;
		memset( &bound_to, 0, sizeof( bound_to ) );
		return false;
	}

	return true;
}

/*
========================
idUDP::Close
========================
*/
void idUDP::Close() {
	if ( netSocket ) {
		closesocket( netSocket );
		netSocket = 0;
		memset( &bound_to, 0, sizeof( bound_to ) );
	}
}

/*
========================
idUDP::GetPacket
========================
*/
bool idUDP::GetPacket( netadr_t &from, void *data, int &size, int maxSize ) {
	bool ret;

	while ( 1 ) {

		ret = Net_GetUDPPacket( netSocket, from, (char *)data, size, maxSize );
		if ( !ret ) {
			break;
		}

		packetsRead++;
		bytesRead += size;

		break;
	}

	return ret;
}

/*
========================
idUDP::GetPacketBlocking
========================
*/
bool idUDP::GetPacketBlocking( netadr_t &from, void *data, int &size, int maxSize, int timeout ) {

	if ( !Net_WaitForData( netSocket, timeout ) ) {
		return false;
	}

	if ( GetPacket( from, data, size, maxSize ) ) {
		return true;
	}

	return false;
}

/*
========================
idUDP::SendPacket
========================
*/
void idUDP::SendPacket( const netadr_t to, const void *data, int size ) {
	if ( to.type == NA_BAD ) {
		idLib::Warning( "idUDP::SendPacket: bad address type NA_BAD - ignored" );
		return;
	}

	packetsWritten++;
	bytesWritten += size;

	if ( silent ) {
		return;
	}

	Net_SendUDPPacket( netSocket, size, data, to );
}