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
#ifndef	__SYS_LOBBY_BACKEND_H__
#define	__SYS_LOBBY_BACKEND_H__


extern idCVar net_verboseResource;
#define NET_VERBOSERESOURCE_PRINT if ( net_verboseResource.GetBool() ) idLib::Printf

extern idCVar net_verbose;
#define NET_VERBOSE_PRINT if ( net_verbose.GetBool() ) idLib::Printf

class lobbyAddress_t
{
public:
	lobbyAddress_t();

	void InitFromNetadr( const netadr_t& netadr );

	void InitFromIPandPort( const char* ip, int port );

	const char* ToString() const;
	bool UsingRelay() const;
	bool Compare( const lobbyAddress_t& addr, bool ignoreSessionCheck = false ) const;
	void WriteToMsg( idBitMsg& msg ) const;
	void ReadFromMsg( idBitMsg& msg );

	// IP address
	netadr_t	netAddr;
};

struct lobbyConnectInfo_t
{
public:
	void WriteToMsg( idBitMsg& msg ) const
	{
		msg.WriteNetadr( netAddr );
	}
	void ReadFromMsg( idBitMsg& msg )
	{
		msg.ReadNetadr( &netAddr );
	}
	lobbyConnectInfo_t() : netAddr() { }

	netadr_t				netAddr;
};

class idNetSessionPort
{
public:
	idNetSessionPort();

	bool InitPort( int portNumber, bool useBackend );
	bool ReadRawPacket( lobbyAddress_t& from, void* data, int& size, int maxSize );
	void SendRawPacket( const lobbyAddress_t& to, const void* data, int size );

	bool IsOpen();
	void Close();

private:
	float	forcePacketDropCurr;	// Used with net_forceDrop and net_forceDropCorrelation
	float	forcePacketDropPrev;

	idUDP	UDP;
};

struct lobbyUser_t
{
	static const int INVALID_PING = 9999;
	// gamertags can be up to 16 4-byte characters + \0
	static const int MAX_GAMERTAG	= 64 + 1;

	lobbyUser_t()
	{
		isBot				= false;
		peerIndex			= -1;
		disconnecting		= false;
		level				= 1;
		pingMs				= INVALID_PING;
		teamNumber			= 0;
		arbitrationAcked	= false;
		partyToken			= 0;

		selectedSkin		= 0;
		weaponAutoSwitch	= true;
		weaponAutoReload	= true;

		migrationGameData	= -1;
	}

	// Common variables
	bool				isBot;				// true if lobbyUser is a bot.
	int					peerIndex;			// peer number on host
	lobbyUserID_t		lobbyUserID;		// Locally generated to be unique, and internally keeps the local user handle
	char				gamertag[MAX_GAMERTAG];
	int					pingMs;				// round trip time in milliseconds

	bool				disconnecting;		// true if we've sent a msg to disconnect this user from the session
	int					level;
	int					teamNumber;
	uint32				partyToken;			// set by the server when people join as a party

	int					selectedSkin;
	bool				weaponAutoSwitch;
	bool				weaponAutoReload;

	bool				arbitrationAcked;	// if the user is verified for arbitration

	lobbyAddress_t		address;

	int					migrationGameData;	// index into the local migration gamedata array that is associated with this user. -1=no migration game data available

	// Platform variables

	bool IsDisconnected() const
	{
		return lobbyUserID.IsValid() ? false : true;
	}

	void WriteToMsg( idBitMsg& msg )
	{
		address.WriteToMsg( msg );
		lobbyUserID.WriteToMsg( msg );
		msg.WriteLong( peerIndex );
		msg.WriteShort( pingMs );
		msg.WriteLong( partyToken );
		msg.WriteString( gamertag, MAX_GAMERTAG, false );
		WriteClientMutableData( msg );
	}

	void ReadFromMsg( idBitMsg& msg )
	{
		address.ReadFromMsg( msg );
		lobbyUserID.ReadFromMsg( msg );
		peerIndex = msg.ReadLong();
		pingMs = msg.ReadShort();
		partyToken = msg.ReadLong();
		msg.ReadString( gamertag, MAX_GAMERTAG );
		ReadClientMutableData( msg );
	}

	bool UpdateClientMutableData( const idLocalUser* localUser );

	void WriteClientMutableData( idBitMsg& msg )
	{
		msg.WriteBits( selectedSkin, 4 );
		msg.WriteBits( teamNumber, 2 );		// We need two bits since we use team value of 2 for spectating
		msg.WriteBool( weaponAutoSwitch );
		msg.WriteBool( weaponAutoReload );
		release_assert( msg.GetWriteBit() == 0 );
	}

	void ReadClientMutableData( idBitMsg& msg )
	{
		selectedSkin = msg.ReadBits( 4 );
		teamNumber = msg.ReadBits( 2 );		// We need two bits since we use team value of 2 for spectating
		weaponAutoSwitch = msg.ReadBool();
		weaponAutoReload = msg.ReadBool();
	}
};

/*
================================================
idLobbyBackend
This class interfaces with the various back ends for the different platforms
================================================
*/
class idLobbyBackend
{
public:
	enum lobbyBackendState_t
	{
		STATE_INVALID			= 0,
		STATE_READY				= 1,
		STATE_CREATING			= 2,		// In the process of creating the lobby as a host
		STATE_SEARCHING			= 3,		// In the process of searching for a lobby to join
		STATE_OBTAINING_ADDRESS	= 4,		// In the process of obtaining the address of the lobby owner
		STATE_ARBITRATING		= 5,		// Arbitrating
		STATE_SHUTTING_DOWN		= 6,		// In the process of shutting down
		STATE_SHUTDOWN			= 7,		// Was a host or peer at one point, now ready to be deleted
		STATE_FAILED			= 8,		// Failure occurred
		NUM_STATES
	};

	static const char* GetStateString( lobbyBackendState_t state_ )
	{
		static const char* stateToString[NUM_STATES] =
		{
			"STATE_INVALID",
			"STATE_READY",
			"STATE_CREATING",
			"STATE_SEARCHING",
			"STATE_OBTAINING_ADDRESS",
			"STATE_ARBITRATING",
			"STATE_SHUTTING_DOWN",
			"STATE_SHUTDOWN",
			"STATE_FAILED"
		};

		return stateToString[ state_ ];
	}

	enum lobbyBackendType_t
	{
		TYPE_PARTY		= 0,
		TYPE_GAME		= 1,
		TYPE_GAME_STATE	= 2,
		TYPE_INVALID	= 0xff,
	};

	idLobbyBackend() : type( TYPE_INVALID ), isLocal( false ), isHost( false ) {}
	idLobbyBackend( lobbyBackendType_t lobbyType ) : type( lobbyType ), isLocal( false ), isHost( false ) {}

	virtual                 ~idLobbyBackend() {}                                            // SRS - Added virtual destructor

	virtual void			StartHosting( const idMatchParameters& p, float skillLevel, lobbyBackendType_t type ) = 0;
	virtual void			StartFinding( const idMatchParameters& p, int numPartyUsers, float skillLevel ) = 0;
	virtual void			JoinFromConnectInfo( const lobbyConnectInfo_t& connectInfo ) = 0;
	virtual void			GetSearchResults( idList< lobbyConnectInfo_t >& searchResults ) = 0;
	virtual lobbyConnectInfo_t GetConnectInfo()	= 0;
	virtual void			FillMsgWithPostConnectInfo( idBitMsg& msg ) = 0;				// Passed itno PostConnectFromMsg
	virtual void			PostConnectFromMsg( idBitMsg& msg ) = 0;						// Uses results from FillMsgWithPostConnectInfo
	virtual bool			IsOwnerOfConnectInfo( const lobbyConnectInfo_t& connectInfo ) const
	{
		return false;
	}
	virtual void			Shutdown() = 0;
	virtual void			GetOwnerAddress( lobbyAddress_t& outAddr ) = 0;
	virtual bool			IsHost()
	{
		return isHost;
	}
	virtual void			SetIsJoinable( bool joinable ) {}
	virtual void			Pump() = 0;
	virtual void			UpdateMatchParms( const idMatchParameters& p ) = 0;
	virtual void			UpdateLobbySkill( float lobbySkill ) = 0;
	virtual void			SetInGame( bool value ) {}

	virtual lobbyBackendState_t	GetState() = 0;
	virtual bool			IsLocal() const
	{
		return isLocal;
	}
	virtual bool			IsOnline() const
	{
		return !isLocal;
	}

	virtual bool			StartArbitration()
	{
		return false;
	}
	virtual void			Arbitrate() {}
	virtual void			VerifyArbitration() {}
	virtual bool			UserArbitrated( lobbyUser_t* user )
	{
		return false;
	}

	virtual void			RegisterUser( lobbyUser_t* user, bool isLocal ) {}
	virtual void			UnregisterUser( lobbyUser_t* user, bool isLocal ) {}

	virtual void			StartSession() {}
	virtual void			EndSession() {}
	virtual bool			IsSessionStarted()
	{
		return false;
	}
	virtual void			FlushStats() {}

	virtual void			BecomeHost( int numInvites ) {}						// Become the host of this lobby
	virtual	void			RegisterAddress( lobbyAddress_t& address ) {}	// Called after becoming a new host, to register old addresses to send invites to
	virtual void			FinishBecomeHost() {}

	void					SetLobbyType( lobbyBackendType_t lobbyType )
	{
		type = lobbyType;
	}
	lobbyBackendType_t		GetLobbyType() const
	{
		return type;
	}
	const char* 			GetLobbyTypeString() const
	{
		return ( GetLobbyType() == TYPE_PARTY ) ? "Party" : "Game";
	}

	bool					IsRanked()
	{
		return MatchTypeIsRanked( parms.matchFlags );
	}
	bool					IsPrivate()
	{
		return MatchTypeIsPrivate( parms.matchFlags );
	}

protected:
	lobbyBackendType_t		type;
	idMatchParameters		parms;
	bool					isLocal;		// True if this lobby is restricted to local play only (won't need and can't connect to online lobbies)
	bool					isHost;			// True if we created this lobby
};

class idLobbyToSessionCB
{
public:
	virtual class idLobbyBackend* 				GetLobbyBackend( idLobbyBackend::lobbyBackendType_t type ) const = 0;
	virtual bool								CanJoinLocalHost() const = 0;

	// Ugh, hate having to ifdef these, but we're doing some fairly platform specific callbacks
};

#endif	// __SYS_LOBBY_BACKEND_H__ 
