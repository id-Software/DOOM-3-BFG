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

#include "sys_lobby_backend.h"

#define INVALID_LOBBY_USER_NAME " " // Used to be "INVALID" but Sony might not like that.

class idSessionCallbacks;
class idDebugGraph;
/*
========================
idLobby
========================
*/
class idLobby : public idLobbyBase
{
public:
	idLobby();

	enum lobbyType_t
	{
		TYPE_PARTY				= 0,
		TYPE_GAME				= 1,
		TYPE_GAME_STATE			= 2,
		TYPE_INVALID			= 0xff
	};

	enum lobbyState_t
	{
		STATE_IDLE,
		STATE_CREATE_LOBBY_BACKEND,
		STATE_SEARCHING,
		STATE_OBTAINING_ADDRESS,
		STATE_CONNECT_HELLO_WAIT,
		STATE_FINALIZE_CONNECT,
		STATE_FAILED,
		NUM_STATES
	};

	enum failedReason_t
	{
		FAILED_UNKNOWN,
		FAILED_CONNECT_FAILED,
		FAILED_MIGRATION_CONNECT_FAILED,
	};

	void								Initialize( lobbyType_t sessionType_, class idSessionCallbacks* callbacks );
	void								StartHosting( const idMatchParameters& parms );
	void								StartFinding( const idMatchParameters& parms_ );
	void								Pump();
	void								ProcessSnapAckQueue();
	void								Shutdown( bool retainMigrationInfo = false, bool skipGoodbye = false );						// Goto idle state
	void								HandlePacket( lobbyAddress_t& remoteAddress, idBitMsg fragMsg, idPacketProcessor::sessionId_t sessionID );
	lobbyState_t						GetState()
	{
		return state;
	}
	virtual bool						HasActivePeers() const;
	virtual bool						IsLobbyFull() const
	{
		return NumFreeSlots() == 0;
	}
	int									NumFreeSlots() const;

public:

	enum reliablePlayerToPlayer_t
	{
		//RELIABLE_PLAYER_TO_PLAYER_VOICE_EVENT,
		RELIABLE_PLAYER_TO_PLAYER_GAME_DATA,
		// Game messages would be reserved here in the same way that RELIABLE_GAME_DATA is.
		// I'm worried about using up the 0xff values we have for reliable type, so I'm not
		// going to reserve anything here just yet.
		NUM_RELIABLE_PLAYER_TO_PLAYER,
	};

	enum reliableType_t
	{
		RELIABLE_HELLO,							// host to peer : connection established
		RELIABLE_USER_CONNECTED,				// host to peer : a new session user connected
		RELIABLE_USER_DISCONNECTED,				// host to peer : a session user disconnected
		RELIABLE_START_LOADING,					// host to peer : peer should begin loading the map
		RELIABLE_LOADING_DONE,					// peer to host : finished loading map
		RELIABLE_IN_GAME,						// peer to host : first full snap received, in game now
		RELIABLE_SNAPSHOT_ACK,					// peer to host : got a snapshot
		RELIABLE_RESOURCE_ACK,					// peer to host : got some new resources
		RELIABLE_CONNECT_AND_MOVE_TO_LOBBY,		// host to peer : connect to this server
		RELIABLE_PARTY_CONNECT_OK,				// host to peer
		RELIABLE_PARTY_LEAVE_GAME_LOBBY,		// host to peer : leave game lobby
		RELIABLE_MATCH_PARMS,					// host to peer : update in match parms
		RELIABLE_UPDATE_MATCH_PARMS,			// peer to host : peer updating match parms

		// User join in progress msg's (join in progress for the party/game lobby, not inside a match)
		RELIABLE_USER_CONNECT_REQUEST,			// peer to host: local user wants to join session in progress
		RELIABLE_USER_CONNECT_DENIED,			// host to peer: user join session in progress denied (not enough slots)

		// User leave in progress msg's (leave in progress for the party/game lobby, not inside a match)
		RELIABLE_USER_DISCONNECT_REQUEST,		// peer to host: request host to remove user from session

		RELIABLE_KICK_PLAYER,					// host to peer : kick a player

		RELIABLE_MATCHFINISHED,					// host to peer - Match is in post looking at score board
		RELIABLE_ENDMATCH,						// host to peer - End match, and go to game lobby
		RELIABLE_ENDMATCH_PREMATURE,			// host to peer - End match prematurely, and go to game lobby (onl possible in unrated/custom games)

		RELIABLE_SESSION_USER_MODIFIED,			// peer to host : user changed something (emblem, name, etc)
		RELIABLE_UPDATE_SESSION_USER,			// host to peers : inform all peers of the change

		RELIABLE_HEADSET_STATE,					// * to * : headset state change for user
		RELIABLE_VOICE_STATE,					// * to * : voice state changed for user pair (mute, unmute, etc)
		RELIABLE_PING,							// * to * : send host->peer, then reflected
		RELIABLE_PING_VALUES,					// host to peers : ping data from lobbyUser_t for everyone

		RELIABLE_BANDWIDTH_VALUES,				// peer to host: data back about bandwidth test

		RELIABLE_ARBITRATE,						// host to peer : start arbitration
		RELIABLE_ARBITRATE_OK,					// peer to host : ack arbitration request

		RELIABLE_POST_STATS,					// host to peer : here, write these stats now (hacky)

		RELIABLE_MIGRATION_GAME_DATA,			// host to peers: game data to use incase of a migration

		RELIABLE_START_MATCH_GAME_LOBBY_HOST,	// game lobby host to game state lobby host: start the match, since all players are in

		RELIABLE_DUMMY_MSG,						// used as a placeholder for old removed msg's

		RELIABLE_PLAYER_TO_PLAYER_BEGIN,
		// use reliablePlayerToPlayer_t
		RELIABLE_PLAYER_TO_PLAYER_END = RELIABLE_PLAYER_TO_PLAYER_BEGIN + NUM_RELIABLE_PLAYER_TO_PLAYER,

		// * to * : misc reliable game data above this
		RELIABLE_GAME_DATA = RELIABLE_PLAYER_TO_PLAYER_END
	};

	// JGM: Reliable type in packet is a byte and there are a lot of reliable game messages.
	// Feel free to bump this up since it's arbitrary anyway, but take a look at gameReliable_t.
	// At the moment, both Doom and Rage have around 32 gameReliable_t values.
	compile_time_assert( RELIABLE_GAME_DATA < 64 );

	static const char* stateToString[ NUM_STATES ];

	// Consts

	static const int PEER_HEARTBEAT_IN_SECONDS				= 5;		// Make sure something was sent every 5 seconds, so we don't time out
	static const int CONNECT_REQUEST_FREQUENCY_IN_SECONDS	= 5;		// Frequency at which we resend a request to connect to a server (will increase in frequency over time down to MIN_CONNECT_FREQUENCY_IN_SECONDS)
	static const int MIN_CONNECT_FREQUENCY_IN_SECONDS		= 1;		// Min frequency of connection attempts
	static const int MAX_CONNECT_ATTEMPTS					= 5;
	static const int BANDWIDTH_REPORTING_MAX				= 10240;	// make bps to report receiving (clamp if higher). For quantizing
	static const int BANDWIDTH_REPORTING_BITS				= 16;		// number of bits to use for bandwidth reporting
	static const int MAX_BPS_HISTORY						= 32;		// size of outgoing bps history to maintain for each client

	static const int MAX_SNAP_SIZE				= idPacketProcessor::MAX_MSG_SIZE;
	static const int MAX_SNAPSHOT_QUEUE			= 64;

	static const int OOB_HELLO					= 0;
	static const int OOB_GOODBYE				= 1;
	static const int OOB_GOODBYE_W_PARTY		= 2;
	static const int OOB_GOODBYE_FULL			= 3;
	static const int OOB_RESOURCE_LIST			= 4;
	static const int OOB_VOICE_AUDIO			= 5;

	static const int OOB_MATCH_QUERY			= 6;
	static const int OOB_MATCH_QUERY_ACK		= 7;

	static const int OOB_SYSTEMLINK_QUERY		= 8;

	static const int OOB_MIGRATE_INVITE			= 9;

	static const int OOB_BANDWIDTH_TEST			= 10;

	enum connectionState_t
	{
		CONNECTION_FREE				= 0,		// Free peer slot
		CONNECTION_CONNECTING		= 1,		// Waiting for response from host for initial connection
		CONNECTION_ESTABLISHED		= 2,		// Connection is established and active
	};

	struct peer_t
	{
		peer_t()
		{
			loaded					= false;
			inGame					= false;
			networkChecksum			= 0;
			lastSnapTime			= 0;
			snapHz					= 0.0f;
			numResources			= 0;
			lastHeartBeat			= 0;
			connectionState			= CONNECTION_FREE;
			packetProc				= NULL;
			snapProc				= NULL;
			nextPing				= 0; // do it asap
			lastPingRtt				= 0;
			sessionID				= idPacketProcessor::SESSION_ID_INVALID;
			startResourceLoadTime	= 0;
			nextThrottleCheck		= 0;
			maxSnapQueueSize		= 0;
			throttledSnapRate		= 0;
			pauseSnapshots			= false;

			receivedBps				= -1.0f;
			maxSnapBps				= -1.0f;
			receivedThrottle		= 0;
			receivedThrottleTime	= 0;

			throttleSnapsForXSeconds = 0;
			recoverPing				= 0;
			failedPingRecoveries	= 0;
			rightBeforeSnapsPing	= 0;

			bandwidthTestLastSendTime = 0;
			bandwidthSequenceNum = 0;
			bandwidthTestBytes = 0;
			bandwidthChallengeStartSendTime = 0;
			bandwidthChallengeResults = false;
			bandwidthChallengeSendComplete = false;

			numSnapsSent			= 0;

			ResetConnectState();
		};

		void ResetConnectState()
		{
			lastResourceTime		= 0;
			lastSnapTime			= 0;
			snapHz					=
				lastProcTime			= 0;
			lastInBandProcTime		= 0;
			lastFragmentSendTime	= 0;
			needToSubmitPendingSnap	= false;
			lastSnapJobTime			= true;
			startResourceLoadTime	= 0;

			receivedBps				= -1.0;
			maxSnapBps				= -1.0f;
			receivedThrottle		= 0;
			receivedThrottleTime	= 0;
			throttleSnapsForXSeconds = 0;
			recoverPing				= 0;
			failedPingRecoveries	= 0;
			rightBeforeSnapsPing	= 0;

			bandwidthTestLastSendTime	= 0;
			bandwidthSequenceNum		= 0;
			bandwidthTestBytes			= 0;
			bandwidthChallengeStartSendTime = 0;
			bandwidthChallengeResults	= false;
			bandwidthChallengeSendComplete = false;
			memset( sentBpsHistory, 0, sizeof( sentBpsHistory ) );
			receivedBpsIndex = 0;

			debugGraphs.Clear();
		}

		void ResetAllData()
		{
			ResetConnectState();
			ResetMatchData();
		}

		void ResetMatchData()
		{
			loaded					= false;
			networkChecksum			= 0;
			inGame					= false;
			numResources			= 0;
			needToSubmitPendingSnap	= false;
			throttledSnapRate		= 0;
			maxSnapQueueSize		= 0;
			receivedBpsIndex		= -1;
			numSnapsSent			= 0;
			pauseSnapshots			= false;

			// Reset the snapshot processor
			if( snapProc != NULL )
			{
				snapProc->Reset( false );
			}
		}

		void Print()
		{
			idLib::Printf( "   lastResourceTime: %d\n", lastResourceTime );
			idLib::Printf( "   lastSnapTime: %d\n", lastSnapTime );
			idLib::Printf( "   lastProcTime: %d\n", lastProcTime );
			idLib::Printf( "   lastInBandProcTime: %d\n", lastInBandProcTime );
			idLib::Printf( "   lastFragmentSendTime: %d\n", lastFragmentSendTime );
			idLib::Printf( "   needToSubmitPendingSnap: %d\n", needToSubmitPendingSnap );
			idLib::Printf( "   lastSnapJobTime: %d\n", lastSnapJobTime );
		}

		bool IsActive() const
		{
			return connectionState != CONNECTION_FREE;
		}
		bool IsConnected() const
		{
			return connectionState == CONNECTION_ESTABLISHED;
		}

		connectionState_t	GetConnectionState() const;

		connectionState_t	connectionState;
		bool				loaded;						// true if this peer has finished loading the map
		bool				inGame;						// true if this peer received the first snapshot, and is in-game
		int					lastSnapTime;				// Last time a snapshot was sent on the network to this peer
		float				snapHz;
		int					lastProcTime;				// Used to determine when a packet was processed for sending to this peer
		int					lastInBandProcTime;			// Last time a in-band packet was processed for sending
		int					lastFragmentSendTime;		// Last time a fragment was sent out (fragments are processed msg's, waiting to be fully sent)
		// DG: use int instead of long for 64bit compatibility
		unsigned int		networkChecksum;			// Checksum used to determine if a peer loaded the network resources the EXACT same as the server did
		// DG end
		int					pauseSnapshots;

		lobbyAddress_t	address;

		int					numResources;				// number of network resources we know the peer has

		idPacketProcessor* 		packetProc;				// Processes packets for this peer
		idSnapshotProcessor* 	snapProc;				// Processes snapshots for this peer
		idStaticList< idDebugGraph*, 4 >	debugGraphs; //

		int					lastResourceTime;			// Used to throttle the sending of resources

		int					lastHeartBeat;
		int					nextPing;					// next Sys_Milliseconds when I'll send this peer a RELIABLE_PING
		int					lastPingRtt;
		bool				needToSubmitPendingSnap;
		int					lastSnapJobTime;			// Last time a snapshot was sent to the joblist for this peer


		int					startResourceLoadTime;		// Used to determine how long a peer has been loading resources

		int					maxSnapQueueSize;			// how big has the snap queue gotten?
		int					throttledSnapRate;			// effective snap rate for this peer
		int					nextThrottleCheck;

		int					numSnapsSent;

		float				sentBpsHistory[ MAX_BPS_HISTORY ];
		int					receivedBpsIndex;

		float				receivedBps;				// peer's reported bps (they tell us their effective downstream)
		float				maxSnapBps;
		float				receivedThrottle;			// amount of accumlated time this client has been lagging behind
		int					receivedThrottleTime;		// last time we did received based throttle calculations

		int					throttleSnapsForXSeconds;
		int					recoverPing;
		int					failedPingRecoveries;
		int					rightBeforeSnapsPing;

		int					bandwidthChallengeStartSendTime;	// time we sent first packet of bw challenge to this peer
		int					bandwidthTestLastSendTime;			// last time in MS we sent them a bw challenge packet
		int					bandwidthTestBytes;					// used to measure number of bytes we sent them
		int					bandwidthSequenceNum;				// number of challenge sequences we sent them
		bool				bandwidthChallengeResults;			// we got results back
		bool				bandwidthChallengeSendComplete;		// we finished sending everything


		idPacketProcessor::sessionId_t sessionID;
	};

	const char* 						GetLobbyName()
	{
		switch( lobbyType )
		{
			case TYPE_PARTY:
				return "TYPE_PARTY";
			case TYPE_GAME:
				return "TYPE_GAME";
			case TYPE_GAME_STATE:
				return "TYPE_GAME_STATE";
		}

		return "LOBBY_INVALID";
	}

	virtual lobbyUserID_t				AllocLobbyUserSlotForBot( const char* botName );							// find a open user slot for the bot, and return the userID.
	virtual void						RemoveBotFromLobbyUserList( lobbyUserID_t lobbyUserID );					// release the session user slot, so that it can be claimed by a player, etc.
	virtual bool						GetLobbyUserIsBot( lobbyUserID_t lobbyUserID ) const;						// check to see if the lobby user is a bot or not

	virtual int							GetNumLobbyUsers() const
	{
		return userList.Num();
	}
	virtual int							GetNumActiveLobbyUsers() const;
	virtual bool						AllPeersInGame() const;
	lobbyUser_t* 						GetLobbyUser( int index )
	{
		return ( index >= 0 && index < GetNumLobbyUsers() ) ? userList[index] : NULL;
	}
	const lobbyUser_t* 					GetLobbyUser( int index ) const
	{
		return ( index >= 0 && index < GetNumLobbyUsers() ) ? userList[index] : NULL;
	}

	virtual bool						IsLobbyUserConnected( int index ) const
	{
		return !IsLobbyUserDisconnected( index );
	}

	virtual int							PeerIndexFromLobbyUser( lobbyUserID_t lobbyUserID ) const;

	virtual int							GetPeerTimeSinceLastPacket( int peerIndex ) const;
	virtual int							PeerIndexForHost() const
	{
		return host;
	}

	virtual int							PeerIndexOnHost() const
	{
		return peerIndexOnHost;    // Returns -1 if we are the host
	}

	virtual const idMatchParameters& 	GetMatchParms() const
	{
		return parms;
	}

	lobbyType_t							GetActingGameStateLobbyType() const;

	// If IsHost is true, we are a host accepting connections from peers
	bool	IsHost() const
	{
		return isHost;
	}
	// If IsPeer is true, we are a peer, with an active connection to a host
	bool	IsPeer() const
	{
		if( host == -1 )
		{
			return false;		// Can't possibly be a peer if we haven't setup a host
		}
		assert( !IsHost() );
		return peers[host].IsConnected();
	}
	bool	IsConnectingPeer() const
	{
		if( host == -1 )
		{
			return false;		// Can't possibly be a peer if we haven't setup a host
		}
		assert( !IsHost() );
		return peers[host].connectionState == CONNECTION_CONNECTING;
	}

	// IsRunningAsHostOrPeer means we are either an active host, and can accept connections from peers, or we are a peer with an active connection to a host
	bool	IsRunningAsHostOrPeer() const
	{
		return IsHost() || IsPeer();
	}
	bool	IsLobbyActive() const
	{
		return IsRunningAsHostOrPeer();
	}


	struct reliablePlayerToPlayerHeader_t
	{
		int fromSessionUserIndex;
		int toSessionUserIndex;

		reliablePlayerToPlayerHeader_t();

		// Both read and write return false if the data is invalid.
		// The state of the msg and object are undefined if false is returned.
		// The network packets contain userIds, and Read/Write will translate from userId to a
		//	sessionUserIndex.  The sessionUserIndex should be the same on all peers, but the
		//	userId has to be used in case the target player quits while the message is on the
		//	wire from the originating peer to the server.
		bool Read( idLobby* lobby, idBitMsg& msg );
		bool Write( idLobby* lobby, idBitMsg& msg );
	};

	int		GetTotalOutgoingRate(); // returns total instant outgoing bandwidth in B/s

//private:
public:		// Turning this on for now, for the sake of getting this up and running to see where things are
	// State functions
	void								State_Idle();
	void								State_Create_Lobby_Backend();
	void								State_Searching();
	void								State_Obtaining_Address();
	void								State_Finalize_Connect();
	void								State_Connect_Hello_Wait();

	void								SetState( lobbyState_t newState );

	void								StartCreating();

	int									FindPeer( const lobbyAddress_t& remoteAddress, idPacketProcessor::sessionId_t sessionID, bool ignoreSessionID = false );
	int									FindAnyPeer( const lobbyAddress_t& remoteAddress ) const;
	int									FindFreePeer() const;
	int									AddPeer( const lobbyAddress_t& remoteAddress, idPacketProcessor::sessionId_t sessionID );
	void								DisconnectPeerFromSession( int p );
	void								SetPeerConnectionState( int p, connectionState_t newState, bool skipGoodbye = false );
	void								DisconnectAllPeers();

	virtual void						SendReliable( int type, idBitMsg& msg, bool callReceiveReliable = true, peerMask_t sessionUserMask = MAX_UNSIGNED_TYPE( peerMask_t ) );
	virtual void						SendReliableToLobbyUser( lobbyUserID_t lobbyUserID, int type, idBitMsg& msg );
	virtual void						SendReliableToHost( int type, idBitMsg& msg );
	void								SendGoodbye( const lobbyAddress_t& remoteAddress, bool wasFull = false );
	void								QueueReliableMessage( int peerNum, byte type )
	{
		QueueReliableMessage( peerNum, type, NULL, 0 );
	}
	void								QueueReliableMessage( int p, byte type, const byte* data, int dataLen );
	virtual int							GetNumConnectedPeers() const;
	virtual int							GetNumConnectedPeersInGame() const;
	void								SendMatchParmsToPeers();

	static bool							IsReliablePlayerToPlayerType( byte type );
	void								HandleReliablePlayerToPlayerMsg( int peerNum, idBitMsg& msg, int type );
	void								HandleReliablePlayerToPlayerMsg( const reliablePlayerToPlayerHeader_t& info, idBitMsg& msg, int reliableType );

	void								SendConnectionLess( const lobbyAddress_t& remoteAddress, byte type )
	{
		SendConnectionLess( remoteAddress, type, NULL, 0 );
	}
	void								SendConnectionLess( const lobbyAddress_t& remoteAddress, byte type, const byte* data, int dataLen );
	void								SendConnectionRequest();
	void								ConnectTo( const lobbyConnectInfo_t& connectInfo, bool fromInvite );
	void								HandleGoodbyeFromPeer( int peerNum, lobbyAddress_t& remoteAddress, int msgType );
	void								HandleConnectionAttemptFailed();
	bool								ConnectToNextSearchResult();
	bool								CheckVersion( idBitMsg& msg, lobbyAddress_t peerAddress );
	bool								VerifyNumConnectingUsers( idBitMsg& msg );
	bool								VerifyLobbyUserIDs( idBitMsg& msg );
	int									HandleInitialPeerConnection( idBitMsg& msg, const lobbyAddress_t& peerAddress, int peerNum );
	void								InitStateLobbyHost();

	void								SendMembersToLobby( lobbyType_t destLobbyType, const lobbyConnectInfo_t& connectInfo, bool waitForOtherMembers );
	void								SendMembersToLobby( idLobby& destLobby, bool waitForOtherMembers );
	void								SendPeerMembersToLobby( int peerIndex, lobbyType_t destLobbyType, const lobbyConnectInfo_t& connectInfo, bool waitForOtherMembers );
	void								SendPeerMembersToLobby( int peerIndex, lobbyType_t destLobbyType, bool waitForOtherMembers );
	void								NotifyPartyOfLeavingGameLobby();
	uint32								GetPartyTokenAsHost();

	virtual void						DrawDebugNetworkHUD() const;
	virtual void						DrawDebugNetworkHUD2() const;
	virtual void						DrawDebugNetworkHUD_ServerSnapshotMetrics( bool draw );

	void								CheckHeartBeats();
	bool								IsLosingConnectionToHost() const;
	bool								IsMigratedStatsGame() const;
	bool								ShouldRelaunchMigrationGame() const;
	bool								ShouldShowMigratingDialog() const;
	bool								IsMigrating() const;

	// Pings
	struct pktPing_t
	{
		int timestamp;
	};

	void								PingPeers();
	void								SendPingValues();
	void								PumpPings();
	void								HandleReliablePing( int p, idBitMsg& msg );
	void								HandlePingReply( int p, const pktPing_t& ping );
	void								HandlePingValues( idBitMsg& msg );
	void								HandleBandwidhTestValue( int p, idBitMsg& msg );
	void								HandleMigrationGameData( idBitMsg& msg );
	void								HandleHeadsetStateChange( int fromPeer, idBitMsg& msg );

	bool								SendAnotherFragment( int p );
	bool								CanSendMoreData( int p );
	void								ProcessOutgoingMsg( int p, const void* data, int size, bool isOOB, int userData );
	void								ResendReliables( int p );
	void								PumpPackets();

	void								UpdateMatchParms( const idMatchParameters& p );

	// SessionID helpers
	idPacketProcessor::sessionId_t		EncodeSessionID( uint32 key ) const;
	void								DecodeSessionID( idPacketProcessor::sessionId_t sessionID, uint32& key ) const;
	idPacketProcessor::sessionId_t		GenerateSessionID() const;
	bool								SessionIDCanBeUsedForInBand( idPacketProcessor::sessionId_t sessionID ) const;
	idPacketProcessor::sessionId_t		IncrementSessionID( idPacketProcessor::sessionId_t sessionID ) const;

	void								HandleHelloAck( int p, idBitMsg& msg );

	virtual const char* 				GetLobbyUserName( lobbyUserID_t lobbyUserID ) const;
	virtual bool						GetLobbyUserWeaponAutoReload( lobbyUserID_t lobbyUserID ) const;
	virtual bool						GetLobbyUserWeaponAutoSwitch( lobbyUserID_t lobbyUserID ) const;
	virtual int							GetLobbyUserSkinIndex( lobbyUserID_t lobbyUserID ) const;
	virtual int							GetLobbyUserLevel( lobbyUserID_t lobbyUserID ) const;
	virtual int							GetLobbyUserQoS( lobbyUserID_t lobbyUserID ) const;
	virtual int							GetLobbyUserTeam( lobbyUserID_t lobbyUserID ) const;
	virtual bool						SetLobbyUserTeam( lobbyUserID_t lobbyUserID, int teamNumber );
	virtual int							GetLobbyUserPartyToken( lobbyUserID_t lobbyUserID ) const;
	virtual idPlayerProfile* 			GetProfileFromLobbyUser( lobbyUserID_t lobbyUserID );
	virtual idLocalUser* 				GetLocalUserFromLobbyUser( lobbyUserID_t lobbyUserID );
	virtual int							GetNumLobbyUsersOnTeam( int teamNumber ) const;

	const char* 						GetPeerName( int peerNum ) const;
	virtual const char* 				GetHostUserName() const;

	void								HandleReliableMsg( int p, idBitMsg& msg, const lobbyAddress_t* remoteAddress = NULL );

	// Bandwidth / Qos / Throttling
	void								BeginBandwidthTest();
	bool								BandwidthTestStarted();

	void								ServerUpdateBandwidthTest();
	void								ClientUpdateBandwidthTest();

	void								ThrottlePeerSnapRate( int peerNum );

	//
	// sys_session_instance_users.cpp
	//

	lobbyUser_t* 						AllocUser( const lobbyUser_t& defaults );
	void								FreeUser( lobbyUser_t* user );
	bool								VerifyUser( const lobbyUser_t* lobbyUser ) const;
	void								FreeAllUsers();
	void								RegisterUser( lobbyUser_t* lobbyUser );
	void								UnregisterUser( lobbyUser_t* lobbyUser );

	bool								IsSessionUserLocal( const lobbyUser_t* lobbyUser ) const;
	bool								IsSessionUserIndexLocal( int i ) const;
	int									GetLobbyUserIndexByID( lobbyUserID_t lobbyUserId, bool ignoreLobbyType = false ) const;
	lobbyUser_t*							GetLobbyUserByID( lobbyUserID_t lobbyUserId, bool ignoreLobbyType = false );

	// Helper function to create a lobby user from a local user
	lobbyUser_t							CreateLobbyUserFromLocalUser( const idLocalUser* localUser );

	// This function is designed to initialize the session users of type lobbyType (TYPE_GAME or TYPE_PARTY)
	// to the current list of local users that are being tracked by the sign-in manager
	void								InitSessionUsersFromLocalUsers( bool onlineMatch );

	// Convert an local userhandle to a session user (-1 if there is no session user with this handle)
	int									GetLobbyUserIndexByLocalUserHandle( const localUserHandle_t localUserHandle ) const;

	// This takes a session user, and converts to a controller user
	idLocalUser* 						GetLocalUserFromLobbyUserIndex( int lobbyUserIndex );

	// Takes a controller user, and converts to a session user (will return NULL if there is no session user for this controller user)
	lobbyUser_t* 						GetSessionUserFromLocalUser( const idLocalUser* controller );

	void								RemoveUsersWithDisconnectedPeers();
	void								RemoveSessionUsersByIDList( idList< lobbyUserID_t >& usersToRemoveByID );
	void								SendNewUsersToPeers( int skipPeer, int userStart, int numUsers );
	void								SendPeersMicStatusToNewUsers( int peerNumber );
	void								AddUsersFromMsg( idBitMsg& msg, int fromPeer );
	void								UpdateSessionUserOnPeers( idBitMsg& msg );
	void								HandleUpdateSessionUser( idBitMsg& msg );
	void								CreateUserUpdateMessage( int userIndex, idBitMsg& msg );
	void								UpdateLocalSessionUsers();
	int									PeerIndexForSessionUserIndex( int sessionUserIndex ) const;
	void								HandleUserConnectFailure( int p, idBitMsg& inMsg, int reliableType );
	void								ProcessUserDisconnectMsg( idBitMsg& msg );
	void								CompactDisconnectedUsers();

	// Sends a request to the host to join a local user to a session
	void								RequestLocalUserJoin( idLocalUser* localUser );
	// Sends a request to the host to remove a session user from the session
	void								RequestSessionUserDisconnect( int sessionUserIndex );

	// This function sycs the session users with the current list of of local users on the signin manager.
	// It will remove the session users that are either no longer on the signin manager, or it
	// will remove them if they are no longer allowed to be in the session.
	// If it finds a local users that are not in a particular session, it will add that user if allowed.
	void								SyncLobbyUsersWithLocalUsers( bool allowJoin, bool onlineMatch );

	bool								ValidateConnectedUser( const lobbyUser_t* user ) const;
	virtual bool						IsLobbyUserDisconnected( int userIndex ) const;
	virtual bool						IsLobbyUserValid( lobbyUserID_t lobbyUserID ) const;
	virtual bool						IsLobbyUserLoaded( lobbyUserID_t lobbyUserID ) const;
	virtual bool						LobbyUserHasFirstFullSnap( lobbyUserID_t lobbyUserID ) const;
	virtual lobbyUserID_t				GetLobbyUserIdByOrdinal( int userIndex ) const;
	virtual	int							GetLobbyUserIndexFromLobbyUserID( lobbyUserID_t lobbyUserID ) const;
	virtual void						EnableSnapshotsForLobbyUser( lobbyUserID_t lobbyUserID );
	virtual bool						IsPeerDisconnected( int peerIndex ) const
	{
		return !peers[peerIndex].IsConnected();
	}

	float								GetAverageSessionLevel();
	float								GetAverageLocalUserLevel( bool onlineOnly );

	void								QueueReliablePlayerToPlayerMessage( int fromSessionUserIndex, int toSessionUserIndex, reliablePlayerToPlayer_t type, const byte* data, int dataLen );
	virtual void						KickLobbyUser( lobbyUserID_t lobbyUserID );

	int									GetNumConnectedUsers() const;

	//
	// sys_session_instance_migrate.cpp
	//

	bool								IsBetterHost( int ping1, lobbyUserID_t userId1, int ping2, lobbyUserID_t userId2 );
	int									FindMigrationInviteIndex( lobbyAddress_t& address );
	void								UpdateHostMigration();
	void								BuildMigrationInviteList( bool inviteOldHost );
	void								PickNewHost( bool forceMe = false, bool inviteOldHost = false );
	void								PickNewHostInternal( bool forceMe, bool inviteOldHost );
	void								BecomeHost();
	void								EndMigration();
	void								ResetAllMigrationState();

	void								SendMigrationGameData();

	bool								GetMigrationGameData( idBitMsg& msg, bool reading );
	bool								GetMigrationGameDataUser( lobbyUserID_t lobbyUserID, idBitMsg& msg, bool reading );

	//
	// Snapshots
	// sys_session_instance_snapshot.cpp
	//
	void								UpdateSnaps();
	bool								SendCompletedSnaps();
	bool								SendResources( int p );
	bool								SubmitPendingSnap( int p );
	void								SendCompletedPendingSnap( int p );
	void								CheckPeerThrottle( int p );
	void								ApplySnapshotDelta( int p, int snapshotNumber );
	bool								ApplySnapshotDeltaInternal( int p, int snapshotNumber );
	void								SendSnapshotToPeer( idSnapShot& ss, int p );
	bool								AllPeersHaveBaseState();
	void								ThrottleSnapsForXSeconds( int p, int seconds, bool recoverPing );
	bool								FirstSnapHasBeenSent( int p );
	virtual bool						EnsureAllPeersHaveBaseState();
	virtual bool						AllPeersHaveStaleSnapObj( int objId );
	virtual bool						AllPeersHaveExpectedSnapObj( int objId );
	virtual void						MarkSnapObjDeleted( int objId );
	virtual void						RefreshSnapObj( int objId );
	void								ResetBandwidthStats();
	void								DetectSaturation( int p );
	virtual void						AddSnapObjTemplate( int objID, idBitMsg& msg );

	static const int MAX_PEERS = MAX_PLAYERS;

	//------------------------
	// Pings
	//------------------------
	struct pktPingValues_t
	{
		idArray<short, MAX_PEERS> pings;
	};

	static const int PING_INTERVAL_MS = 3000;

	int									lastPingValuesRecvTime; // so clients can display something when server stops pinging
	int									nextSendPingValuesTime; // the next time to send RELIABLE_PING_VALUES

	static const int MIGRATION_GAME_DATA_INTERVAL_MS = 1000;
	int									nextSendMigrationGameTime;	// when to send next migration game data
	int									nextSendMigrationGamePeer;	// who to send next migration game data to

	lobbyType_t							lobbyType;
	lobbyState_t						state;						// State of this lobby
	failedReason_t						failedReason;

	int									host;						// which peer is the host of this type of session (-1 if we are the host)
	int									peerIndexOnHost;			// -1 if we are the host
	lobbyAddress_t						hostAddress;				// address of the host for this type of session
	bool								isHost;						// true if we are the host
	idLobbyBackend* 					lobbyBackend;

	int									helloStartTime;				// Used to determine when the first hello was sent
	int									lastConnectRequest;			// Used to determine when the last hello was sent
	int									connectionAttempts;			// Number of connection attempts


	bool								needToDisplayMigrateMsg;	// If true, we migrated as host, so we need to display the msg as soon as the lobby is active
	gameDialogMessages_t				migrationDlg;				// current migration dialog we should be showing

	uint8								migrateMsgFlags;			// cached match flags from the old game we migrated from, so we know what type of msg to display

	bool								joiningMigratedGame;		// we are joining a migrated game and need to tell the session mgr if we succeed or fail

	// ------------------------
	//	Bandwidth challenge
	// ------------------------
	int									bandwidthChallengeEndTime;		// When the challenge will end/timeout
	int									bandwidthChallengeStartTime;	// time in MS the challenge started
	bool								bandwidthChallengeFinished;		// (HOST) test is finished and we received results back from all peers (or timed out)
	int									bandwidthChallengeNumGoodSeq;	// (PEER) num of good, in order packets we recevieved

	int									lastSnapBspHistoryUpdateSequence;

	void								SaveDisconnectedUser( const lobbyUser_t& user );	// This is needed to get the a user's gamertag after disconnection.

	idSessionCallbacks* 				sessionCB;

	enum migrationState_t
	{
		MIGRATE_NONE,
		MIGRATE_PICKING_HOST,
		MIGRATE_BECOMING_HOST,
	};

	struct migrationInvite_t
	{
		migrationInvite_t()
		{
			lastInviteTime	= -1;
			pingMs = 0;
			migrationGameData = -1;
		}

		lobbyAddress_t		address;
		int					pingMs;
		lobbyUserID_t		userId;
		int					lastInviteTime;
		int					migrationGameData;
	};

	struct migrationInfo_t
	{
		migrationInfo_t()
		{
			state		= MIGRATE_NONE;
			ourPingMs	= 0;
			ourUserId	= lobbyUserID_t();
		}

		migrationState_t				state;
		idStaticList< migrationInvite_t, MAX_PEERS > invites;
		int								migrationStartTime;
		int								ourPingMs;
		lobbyUserID_t					ourUserId;

		struct persistUntilGameEnds_t
		{
			persistUntilGameEnds_t()
			{
				Clear();
			}
			void Clear()
			{
				wasMigratedHost = false;
				wasMigratedJoin = false;
				wasMigratedGame = false;
				ourGameData = -1;
				hasGameData = false;
				hasRelaunchedMigratedGame = false;

				memset( gameData, 0, sizeof( gameData ) );
				memset( gameDataUser, 0, sizeof( gameDataUser ) );
			}

			int								ourGameData;
			bool							wasMigratedHost;		// we are hosting a migrated session
			bool							wasMigratedJoin;		// we joined a migrated session
			bool							wasMigratedGame;		// If true, we migrated from a game
			bool							hasRelaunchedMigratedGame;

			// A generic blob of data that the gamechallenge (or anything else) can read and write to for host migration
			static const int MIGRATION_GAME_DATA_SIZE = 32;
			byte gameData[ MIGRATION_GAME_DATA_SIZE ];

			static const int MIGRATION_GAME_DATA_USER_SIZE = 64;
			byte gameDataUser[ MAX_PLAYERS ][ MIGRATION_GAME_DATA_USER_SIZE ];

			bool hasGameData;
		} persistUntilGameEndsData;
	};

	struct disconnectedUser_t
	{
		lobbyUserID_t		lobbyUserID;		// Locally generated to be unique, and internally keeps the local user handle
		char				gamertag[lobbyUser_t::MAX_GAMERTAG];
	};

	migrationInfo_t						migrationInfo;

	bool								showHostLeftTheSession;
	bool								connectIsFromInvite;

	idList< lobbyConnectInfo_t >		searchResults;

	typedef idStaticList< lobbyUser_t*, MAX_PLAYERS >		idLobbyUserList;
	typedef idStaticList< lobbyUser_t, MAX_PLAYERS >		idLobbyUserPool;

	idLobbyUserList						userList;						// list of currently connected users to this lobby
	idLobbyUserList						freeUsers;						// list of free users
	idLobbyUserPool						userPool;

	idList< disconnectedUser_t>			disconnectedUsers;				// List of users which were connected, but aren't anymore, for printing their name on the hud

	idStaticList< peer_t, MAX_PEERS >	peers;							// Unique machines connected to this lobby

	uint32								partyToken;

	idMatchParameters					parms;

	bool								loaded;							// Used for game sessions, whether this machine is loaded or not
	bool								respondToArbitrate;				// true when the host has requested us to arbitrate our session (for TYPE_GAME only)
	bool								everyoneArbitrated;
	bool								waitForPartyOk;
	bool								startLoadingFromHost;

	//------------------------
	// Snapshot jobs
	//------------------------
	static const int SNAP_OBJ_JOB_MEMORY = 1024 * 128;			// 128k of obj memory

	lzwCompressionData_t* 				lzwData;				// Shared across all snapshot jobs
	uint8* 								objMemory;				// Shared across all snapshot jobs
	bool								haveSubmittedSnaps;		// True if we previously submitted snaps to jobs
	idSnapShot* 						localReadSS;

	struct snapDeltaAck_t
	{
		int			p;
		int			snapshotNumber;
	};

	idStaticList< snapDeltaAck_t, 16 >	snapDeltaAckQueue;
};

/*
========================
idSessionCallbacks
========================
*/
class idSessionCallbacks
{
public:
	virtual                         ~idSessionCallbacks() {}                    // SRS - Added virtual destructor

	virtual idLobby& 				GetPartyLobby() = 0;
	virtual idLobby& 				GetGameLobby() = 0;
	virtual idLobby& 				GetActingGameStateLobby() = 0;
	virtual idLobby* 				GetLobbyFromType( idLobby::lobbyType_t lobbyType ) = 0;
	virtual int						GetUniquePlayerId() const = 0;
	virtual idSignInManagerBase&		GetSignInManager() = 0;
	virtual	void					SendRawPacket( const lobbyAddress_t& to, const void* data, int size, bool useDirectPort ) = 0;

	virtual bool					BecomingHost( idLobby& lobby ) = 0;			// Called when a lobby is about to become host
	virtual void					BecameHost( idLobby& lobby ) = 0;				// Called when a lobby becomes a host
	virtual bool					BecomingPeer( idLobby& lobby ) = 0;			// Called when a lobby is about to become peer
	virtual void					BecamePeer( idLobby& lobby ) = 0;				// Called when a lobby becomes a peer
	virtual void					FailedGameMigration( idLobby& lobby ) = 0;
	virtual void					MigrationEnded( idLobby& lobby ) = 0;

	virtual void					GoodbyeFromHost( idLobby& lobby, int peerNum, const lobbyAddress_t& remoteAddress, int msgType ) = 0;

	virtual	uint32					GetSessionOptions() = 0;
	virtual bool					AnyPeerHasAddress( const lobbyAddress_t& remoteAddress ) const = 0;

	virtual idSession::sessionState_t GetState() const = 0;

	virtual void					ClearMigrationState() = 0;
	// Called when the lobby receives a RELIABLE_ENDMATCH msg
	virtual void					EndMatchInternal( bool premature = false ) = 0;

	// Called when the game lobby receives leaderboard stats
	virtual void					RecvLeaderboardStats( idBitMsg& msg ) = 0;

	// Called once the lobby received its first full snap (used to advance from LOADING to INGAME state)
	virtual void					ReceivedFullSnap() = 0;

	// Called when lobby received RELIABLE_PARTY_LEAVE_GAME_LOBBY msg
	virtual void					LeaveGameLobby() = 0;

	virtual void					PrePickNewHost( idLobby& lobby, bool forceMe, bool inviteOldHost ) = 0;
	virtual bool					PreMigrateInvite( idLobby& lobby ) = 0;

	virtual void					HandleOobVoiceAudio( const lobbyAddress_t& from, const idBitMsg& msg ) = 0;

	// ConnectAndMoveToLobby is called when the lobby receives a RELIABLE_CONNECT_AND_MOVE_TO_LOBBY
	virtual void					ConnectAndMoveToLobby( idLobby::lobbyType_t destLobbyType, const lobbyConnectInfo_t& connectInfo, bool waitForPartyOk ) = 0;

	virtual class idVoiceChatMgr* 	GetVoiceChat() = 0;

	virtual void					HandleServerQueryRequest( lobbyAddress_t& remoteAddr, idBitMsg& msg, int msgType ) = 0;
	virtual void 					HandleServerQueryAck( lobbyAddress_t& remoteAddr, idBitMsg& msg ) = 0;

	virtual void					HandlePeerMatchParamUpdate( int peer, int msg ) = 0;

	virtual idLobbyBackend* 		CreateLobbyBackend( const idMatchParameters& p, float skillLevel, idLobbyBackend::lobbyBackendType_t lobbyType ) = 0;
	virtual idLobbyBackend* 		FindLobbyBackend( const idMatchParameters& p, int numPartyUsers, float skillLevel, idLobbyBackend::lobbyBackendType_t lobbyType ) = 0;
	virtual idLobbyBackend* 		JoinFromConnectInfo( const lobbyConnectInfo_t& connectInfo , idLobbyBackend::lobbyBackendType_t lobbyType ) = 0;
	virtual void					DestroyLobbyBackend( idLobbyBackend* lobbyBackend ) = 0;
};
