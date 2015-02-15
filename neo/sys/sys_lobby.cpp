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
#include "sys_lobby.h"

extern idCVar net_connectTimeoutInSeconds;
extern idCVar net_headlessServer;

idCVar net_checkVersion( "net_checkVersion", "0", CVAR_INTEGER, "Check for matching version when clients connect. 0: normal rules, 1: force check, otherwise no check (pass always)" );
idCVar net_peerTimeoutInSeconds( "net_peerTimeoutInSeconds", "30", CVAR_INTEGER, "If the host hasn't received a response from a peer in this amount of time (in seconds), the peer will be disconnected." );
idCVar net_peerTimeoutInSeconds_Lobby( "net_peerTimeoutInSeconds_Lobby", "20", CVAR_INTEGER, "If the host hasn't received a response from a peer in this amount of time (in seconds), the peer will be disconnected." );

// NOTE - The snapshot exchange does the bandwidth challenge
idCVar net_bw_challenge_enable( "net_bw_challenge_enable", "0", CVAR_BOOL, "Enable pre game bandwidth challenge for throttling snap rate" );

idCVar net_bw_test_interval( "net_bw_test_interval", "33", CVAR_INTEGER, "MS - how often to send packets in bandwidth test" );
idCVar net_bw_test_numPackets( "net_bw_test_numPackets", "30", CVAR_INTEGER, "Number of bandwidth challenge packets to send" );
idCVar net_bw_test_packetSizeBytes( "net_bw_test_packetSizeBytes", "1024", CVAR_INTEGER, "Size of each packet to send out" );
idCVar net_bw_test_timeout( "net_bw_test_timeout", "500", CVAR_INTEGER, "MS after receiving a bw test packet that client will time out" );
idCVar net_bw_test_host_timeout( "net_bw_test_host_timeout", "3000", CVAR_INTEGER, "How long host will wait in MS to hear bw results from peers" );

idCVar net_bw_test_throttle_rate_pct( "net_bw_test_throttle_rate_pct", "0.80", CVAR_FLOAT, "Min rate % a peer must match in bandwidth challenge before being throttled. 1.0=perfect, 0.0=received nothing" );
idCVar net_bw_test_throttle_byte_pct( "net_bw_test_throttle_byte_pct", "0.80", CVAR_FLOAT, "Min byte % a peer must match in bandwidth challenge before being throttled. 1.0=perfect (received everything) 0.0=Received nothing" );
idCVar net_bw_test_throttle_seq_pct( "net_bw_test_throttle_seq_pct", "0.80", CVAR_FLOAT, "Min sequence % a peer must match in bandwidth test before being throttled. 1.0=perfect. This score will be more adversely affected by packet loss than byte %" );

idCVar net_ignoreConnects( "net_ignoreConnects", "0", CVAR_INTEGER, "Test as if no one can connect to me. 0 = off, 1 = ignore with no reply, 2 = send goodbye" );

idCVar net_skipGoodbye( "net_skipGoodbye", "0", CVAR_BOOL, "" );

// RB: 64 bit fixes, changed long to int
extern unsigned int NetGetVersionChecksum();
// RB end

/*
========================
idLobby::idLobby
========================
*/
idLobby::idLobby()
{
	lobbyType				= TYPE_INVALID;
	sessionCB				= NULL;
	
	localReadSS				= NULL;
	objMemory				= NULL;
	haveSubmittedSnaps		= false;
	
	state					= STATE_IDLE;
	failedReason			= FAILED_UNKNOWN;
	
	host					= -1;
	peerIndexOnHost			= -1;
	isHost					= false;
	needToDisplayMigrateMsg	= false;
	migrateMsgFlags			= 0;
	
	partyToken				= 0;		// will be initialized later
	loaded					= false;
	respondToArbitrate		= false;
	waitForPartyOk			= false;
	startLoadingFromHost	= false;
	
	nextSendPingValuesTime	= 0;
	lastPingValuesRecvTime	= 0;
	
	nextSendMigrationGameTime = 0;
	nextSendMigrationGamePeer = 0;
	
	bandwidthChallengeStartTime = 0;
	bandwidthChallengeEndTime	= 0;
	bandwidthChallengeFinished	= false;
	bandwidthChallengeNumGoodSeq = 0;
	
	lastSnapBspHistoryUpdateSequence = -1;
	
	assert( userList.Max() == freeUsers.Max() );
	assert( userList.Max() == userPool.Max() );
	
	userPool.SetNum( userPool.Max() );
	
	assert( freeUsers.Num() == 0 );
	assert( freeUsers.Num() == 0 );
	
	// Initialize free user list
	for( int i = 0; i < userPool.Num(); i++ )
	{
		freeUsers.Append( &userPool[i] );
	}
	
	showHostLeftTheSession	= false;
	connectIsFromInvite		= false;
}

/*
========================
idLobby::Initialize
========================
*/
void idLobby::Initialize( lobbyType_t sessionType_, idSessionCallbacks* callbacks )
{
	assert( callbacks != NULL );
	
	lobbyType = sessionType_;
	sessionCB	= callbacks;
	
	if( lobbyType == GetActingGameStateLobbyType() )
	{
		// only needed in multiplayer mode
		objMemory		= ( uint8* )Mem_Alloc( SNAP_OBJ_JOB_MEMORY, TAG_NETWORKING );
		lzwData			= ( lzwCompressionData_t* )Mem_Alloc( sizeof( lzwCompressionData_t ), TAG_NETWORKING );
	}
}

//===============================================================================
//	** BEGIN PUBLIC INTERFACE ***
//===============================================================================

/*
========================
idLobby::StartHosting
========================
*/
void idLobby::StartHosting( const idMatchParameters& parms_ )
{
	parms = parms_;
	
	// Allow common to modify the parms
	common->OnStartHosting( parms );
	
	Shutdown();		// Make sure we're in a shutdown state before proceeding
	
	assert( GetNumLobbyUsers() == 0 );
	assert( lobbyBackend == NULL );
	
	// Get the skill level of all the players that will eventually go into the lobby
	StartCreating();
}

/*
========================
idLobby::StartFinding
========================
*/
void idLobby::StartFinding( const idMatchParameters& parms_ )
{
	parms = parms_;
	
	Shutdown();		// Make sure we're in a shutdown state before proceeding
	
	assert( GetNumLobbyUsers() == 0 );
	assert( lobbyBackend == NULL );
	
	// Clear search results
	searchResults.Clear();
	
	lobbyBackend = sessionCB->FindLobbyBackend( parms, sessionCB->GetPartyLobby().GetNumLobbyUsers(), sessionCB->GetPartyLobby().GetAverageSessionLevel(), idLobbyBackend::TYPE_GAME );
	
	SetState( STATE_SEARCHING );
}

/*
========================
idLobby::Pump
========================
*/
void idLobby::Pump()
{

	// Check the heartbeat of all our peers, make sure we shouldn't disconnect from peers that haven't sent a heartbeat in awhile
	CheckHeartBeats();
	
	UpdateHostMigration();
	
	UpdateLocalSessionUsers();
	
	switch( state )
	{
		case STATE_IDLE:
			State_Idle();
			break;
		case STATE_CREATE_LOBBY_BACKEND:
			State_Create_Lobby_Backend();
			break;
		case STATE_SEARCHING:
			State_Searching();
			break;
		case STATE_OBTAINING_ADDRESS:
			State_Obtaining_Address();
			break;
		case STATE_CONNECT_HELLO_WAIT:
			State_Connect_Hello_Wait();
			break;
		case STATE_FINALIZE_CONNECT:
			State_Finalize_Connect();
			break;
		case STATE_FAILED:
			break;
		default:
			idLib::Error( "idLobby::Pump:  Unknown state." );
	}
}

/*
========================
idLobby::ProcessSnapAckQueue
========================
*/
void idLobby::ProcessSnapAckQueue()
{
	SCOPED_PROFILE_EVENT( "ProcessSnapAckQueue" );
	
	const int SNAP_ACKS_TO_PROCESS_PER_FRAME = 1;
	
	int numProcessed = 0;
	
	while( snapDeltaAckQueue.Num() > 0 && numProcessed < SNAP_ACKS_TO_PROCESS_PER_FRAME )
	{
		if( ApplySnapshotDeltaInternal( snapDeltaAckQueue[0].p, snapDeltaAckQueue[0].snapshotNumber ) )
		{
			numProcessed++;
		}
		snapDeltaAckQueue.RemoveIndex( 0 );
	}
}

/*
========================
idLobby::Shutdown
========================
*/
void idLobby::Shutdown( bool retainMigrationInfo, bool skipGoodbye )
{

	// Cancel host migration if we were in the process of it and this is the session type that was migrating
	if( !retainMigrationInfo && migrationInfo.state != MIGRATE_NONE )
	{
		idLib::Printf( "Cancelling host migration on %s.\n", GetLobbyName() );
		EndMigration();
	}
	
	failedReason = FAILED_UNKNOWN;
	
	if( lobbyBackend == NULL )
	{
		NET_VERBOSE_PRINT( "NET: ShutdownLobby (already shutdown) (%s)\n", GetLobbyName() );
		
		// If we don't have this lobbyBackend type, we better be properly shutdown for this lobby
		assert( GetNumLobbyUsers() == 0 );
		assert( host == -1 );
		assert( peerIndexOnHost == -1 );
		assert( !isHost );
		assert( lobbyType != GetActingGameStateLobbyType() || !loaded );
		assert( lobbyType != GetActingGameStateLobbyType() || !respondToArbitrate );
		assert( snapDeltaAckQueue.Num() == 0 );
		
		// Make sure we don't have old peers connected to this lobby
		for( int p = 0; p < peers.Num(); p++ )
		{
			assert( peers[p].GetConnectionState() == CONNECTION_FREE );
		}
		
		state = STATE_IDLE;
		
		return;
	}
	
	NET_VERBOSE_PRINT( "NET: ShutdownLobby (%s)\n", GetLobbyName() );
	
	for( int p = 0; p < peers.Num(); p++ )
	{
		if( peers[p].GetConnectionState() != CONNECTION_FREE )
		{
			SetPeerConnectionState( p, CONNECTION_FREE, skipGoodbye );		// This will send goodbye's
		}
	}
	
	// Remove any users that weren't handled in ResetPeers
	// (this will happen as a client, because we won't get the reliable msg from the server since we are severing the connection)
	for( int i = 0; i < GetNumLobbyUsers(); i++ )
	{
		lobbyUser_t* user = GetLobbyUser( i );
		UnregisterUser( user );
	}
	
	FreeAllUsers();
	
	host					= -1;
	peerIndexOnHost			= -1;
	isHost					= false;
	needToDisplayMigrateMsg	= false;
	migrationDlg			= GDM_INVALID;
	
	partyToken				= 0;		// Reset our party token so we recompute
	loaded					= false;
	respondToArbitrate		= false;
	waitForPartyOk			= false;
	startLoadingFromHost	= false;
	
	snapDeltaAckQueue.Clear();
	
	// Shutdown the lobbyBackend
	if( !retainMigrationInfo )
	{
		sessionCB->DestroyLobbyBackend( lobbyBackend );
		lobbyBackend = NULL;
	}
	
	state = STATE_IDLE;
}

/*
========================
idLobby::HandlePacket
========================
*/
// TODO: remoteAddress const?
void idLobby::HandlePacket( lobbyAddress_t& remoteAddress, idBitMsg fragMsg, idPacketProcessor::sessionId_t sessionID )
{
	SCOPED_PROFILE_EVENT( "HandlePacket" );
	
	// msg will hold a fully constructed msg using the packet processor
	byte msgBuffer[ idPacketProcessor::MAX_MSG_SIZE ];
	
	idBitMsg msg;
	msg.InitWrite( msgBuffer, sizeof( msgBuffer ) );
	
	int peerNum		= FindPeer( remoteAddress, sessionID );
	int type		= idPacketProcessor::RETURN_TYPE_NONE;
	int	userData	= 0;
	
	if( peerNum >= 0 )
	{
		if( !peers[peerNum].IsActive() )
		{
			idLib::Printf( "NET: Received in-band packet from peer %s with no active connection.\n", remoteAddress.ToString() );
			return;
		}
		type = peers[ peerNum ].packetProc->ProcessIncoming( Sys_Milliseconds(), peers[peerNum].sessionID, fragMsg, msg, userData, peerNum );
	}
	else
	{
		if( !idPacketProcessor::ProcessConnectionlessIncoming( fragMsg, msg, userData ) )
		{
			idLib::Printf( "ProcessConnectionlessIncoming FAILED from %s.\n", remoteAddress.ToString() );
			// Not a valid connectionless packet
			return;
		}
		
		// Valid connectionless packets are always RETURN_TYPE_OOB
		type = idPacketProcessor::RETURN_TYPE_OOB;
		
		// Find the peer this connectionless msg should go to
		peerNum = FindPeer( remoteAddress, sessionID, true );
	}
	
	if( type == idPacketProcessor::RETURN_TYPE_NONE )
	{
		// This packet is not necessarily invalid, it could be a start or middle of a fragmented packet that's not fully constructed.
		return;
	}
	
	if( peerNum >= 0 )
	{
		// Update their heart beat (only if we've received a valid packet (we've checked type == idPacketProcessor::RETURN_TYPE_NONE))
		peers[peerNum].lastHeartBeat = Sys_Milliseconds();
	}
	
	// Handle server query requests.  We do this before the STATE_IDLE check.  This is so we respond.
	// We may want to change this to just ignore the request if we are idle, and change the timeout time
	// on the requesters part to just timeout faster.
	if( type == idPacketProcessor::RETURN_TYPE_OOB )
	{
		if( userData == OOB_MATCH_QUERY || userData == OOB_SYSTEMLINK_QUERY )
		{
			sessionCB->HandleServerQueryRequest( remoteAddress, msg, userData );
			return;
		}
		if( userData == OOB_MATCH_QUERY_ACK )
		{
			sessionCB->HandleServerQueryAck( remoteAddress, msg );
			return;
		}
	}
	
	if( type == idPacketProcessor::RETURN_TYPE_OOB )
	{
		if( userData == OOB_VOICE_AUDIO )
		{
			sessionCB->HandleOobVoiceAudio( remoteAddress, msg );
		}
		else if( userData == OOB_HELLO )
		{
			// Handle new peer connect request
			peerNum = HandleInitialPeerConnection( msg, remoteAddress, peerNum );
			return;
		}
		else if( userData == OOB_MIGRATE_INVITE )
		{
			NET_VERBOSE_PRINT( "NET: Migration invite for session %s from %s (state = %s)\n", GetLobbyName(), remoteAddress.ToString(), session->GetStateString() );
			
			// Get connection info
			lobbyConnectInfo_t connectInfo;
			connectInfo.ReadFromMsg( msg );
			
			if( lobbyBackend != NULL && lobbyBackend->GetState() != idLobbyBackend::STATE_FAILED && lobbyBackend->IsOwnerOfConnectInfo( connectInfo ) )  		// Ignore duplicate invites
			{
				idLib::Printf( "NET: Already migrated to %s.\n", remoteAddress.ToString() );
				return;
			}
			
			if( migrationInfo.state == MIGRATE_NONE )
			{
				if( IsPeer() && host >= 0 && host < peers.Num() && Sys_Milliseconds() - peers[host].lastHeartBeat > 8 * 1000 )
				{
					// Force migration early if we get an invite, and it has been some time since we've heard from the host
					PickNewHost();
				}
				else
				{
					idLib::Printf( "NET: Ignoring migration invite because we are not migrating %s\n", remoteAddress.ToString() );
					SendGoodbye( remoteAddress );	// So they can remove us from their invite list
					return;
				}
			}
			
			if( !sessionCB->PreMigrateInvite( *this ) )
			{
				NET_VERBOSE_PRINT( "NET: sessionCB->PreMigrateInvite( *this ) failed from %s\n", remoteAddress.ToString() );
				return;
			}
			
			// If we are also becoming a new host, see who wins
			if( migrationInfo.state == MIGRATE_BECOMING_HOST )
			{
				int inviteIndex = FindMigrationInviteIndex( remoteAddress );
				
				if( inviteIndex != -1 )
				{
					// We found them in our list, check to make sure our ping is better
					int				ping1	= migrationInfo.ourPingMs;
					lobbyUserID_t	userId1 = migrationInfo.ourUserId;
					int				ping2	= migrationInfo.invites[inviteIndex].pingMs;
					lobbyUserID_t	userId2 = migrationInfo.invites[inviteIndex].userId;
					
					if( IsBetterHost( ping1, userId1, ping2, userId2 ) )
					{
						idLib::Printf( "NET: Ignoring migration invite from %s, since our ping is better (%i / %i).\n", remoteAddress.ToString(), ping1, ping2 );
						return;
					}
				}
			}
			
			bool fromGame = msg.ReadBool();
			
			// Kill the current lobbyBackend
			Shutdown();
			
			// Connect to the lobby
			ConnectTo( connectInfo, true );		// Pass in true for the invite flag, so we can connect to invite only lobby if we need to
			
			if( verify( sessionCB != NULL ) )
			{
				if( sessionCB->BecomingPeer( *this ) )
				{
					migrationInfo.persistUntilGameEndsData.wasMigratedJoin = true;
					migrationInfo.persistUntilGameEndsData.wasMigratedGame = fromGame;
				}
			}
			
		}
		else if( userData == OOB_GOODBYE || userData == OOB_GOODBYE_W_PARTY || userData == OOB_GOODBYE_FULL )
		{
			HandleGoodbyeFromPeer( peerNum, remoteAddress, userData );
			return;
		}
		else if( userData == OOB_RESOURCE_LIST )
		{
		
			if( !verify( lobbyType == GetActingGameStateLobbyType() ) )
			{
				return;
			}
			
			if( peerNum != host )
			{
				NET_VERBOSE_PRINT( "NET: Resource list from non-host %i, %s\n", peerNum, remoteAddress.ToString() );
				return;
			}
			
			if( peerNum >= 0 && !peers[peerNum].IsConnected() )
			{
				NET_VERBOSE_PRINT( "NET: Resource list from host with no game connection: %i, %s\n", peerNum, remoteAddress.ToString() );
				return;
			}
		}
		else if( userData == OOB_BANDWIDTH_TEST )
		{
			int seqNum = msg.ReadLong();
			// TODO: We should read the random data and verify the MD5 checksum
			
			int time = Sys_Milliseconds();
			bool inOrder = ( seqNum == 0 || peers[peerNum].bandwidthSequenceNum + 1 == seqNum );
			int timeSinceLast = 0;
			
			if( bandwidthChallengeStartTime <= 0 )
			{
				// Reset the test
				NET_VERBOSE_PRINT( "\nNET: Starting bandwidth test @ %d\n", time );
				bandwidthChallengeStartTime = time;
				peers[peerNum].bandwidthSequenceNum = 0;
				peers[peerNum].bandwidthTestBytes = peers[peerNum].packetProc->GetIncomingBytes();
			}
			else
			{
				timeSinceLast = time - ( bandwidthChallengeEndTime - session->GetTitleStorageInt( "net_bw_test_timeout", net_bw_test_timeout.GetInteger() ) );
			}
			
			if( inOrder )
			{
				bandwidthChallengeNumGoodSeq++;
			}
			
			bandwidthChallengeEndTime = time + session->GetTitleStorageInt( "net_bw_test_timeout", net_bw_test_timeout.GetInteger() );
			NET_VERBOSE_PRINT( "  NET: %sRecevied OOB bandwidth test %d delta time: %d incoming rate: %.2f  incoming rate 2: %d\n", inOrder ? "^2" : "^1", seqNum, timeSinceLast, peers[peerNum].packetProc->GetIncomingRateBytes(), peers[peerNum].packetProc->GetIncomingRate2() );
			peers[peerNum].bandwidthSequenceNum = seqNum;
			
		}
		else
		{
			NET_VERBOSE_PRINT( "NET: Unknown oob packet %d from %s (%d)\n", userData, remoteAddress.ToString(), peerNum );
		}
	}
	else if( type == idPacketProcessor::RETURN_TYPE_INBAND )
	{
		// Process in-band message
		if( peerNum < 0 )
		{
			idLib::Printf( "NET: In-band message from unknown peer: %s\n", remoteAddress.ToString() );
			return;
		}
		
		if( !verify( peers[ peerNum ].address.Compare( remoteAddress ) ) )
		{
			idLib::Printf( "NET: Peer with wrong address: %i, %s\n", peerNum, remoteAddress.ToString() );
			return;
		}
		
		// Handle reliable
		int numReliable = peers[ peerNum ].packetProc->GetNumReliables();
		for( int r = 0; r < numReliable; r++ )
		{
			// Just in case one of the reliable msg's cause this peer to disconnect
			// (this can happen when our party/game host is the same, he quits the game lobby, and sends a reliable msg for us to leave the game)
			peerNum	= FindPeer( remoteAddress, sessionID );
			
			if( peerNum == -1 )
			{
				idLib::Printf( "NET: Dropped peer while processing reliable msg's: %i, %s\n", peerNum, remoteAddress.ToString() );
				break;
			}
			
			const byte* reliableData = peers[ peerNum ].packetProc->GetReliable( r );
			int reliableSize = peers[ peerNum ].packetProc->GetReliableSize( r );
			idBitMsg reliableMsg( reliableData, reliableSize );
			reliableMsg.SetSize( reliableSize );
			
			HandleReliableMsg( peerNum, reliableMsg, &remoteAddress );
		}
		
		if( peerNum == -1 || !peers[ peerNum ].IsConnected() )
		{
			// If the peer still has no connection after HandleReliableMsg, then something is wrong.
			// (We could have been in CONNECTION_CONNECTING state for this session type, but the first message
			// we should receive from the server is the ack, otherwise, something went wrong somewhere)
			idLib::Printf( "NET: In-band message from host with no active connection: %i, %s\n", peerNum, remoteAddress.ToString() );
			return;
		}
		
		// Handle unreliable part (if any)
		if( msg.GetRemainingData() > 0 && loaded )
		{
			if( !verify( lobbyType == GetActingGameStateLobbyType() ) )
			{
				idLib::Printf( "NET: Snapshot msg for non game session lobby %s\n", remoteAddress.ToString() );
				return;
			}
			
			if( peerNum == host )
			{
				idSnapShot	localSnap;
				int			sequence = -1;
				int			baseseq = -1;
				bool		fullSnap = false;
				localReadSS = &localSnap;
				
				// If we are the peer, we assume we only receive snapshot data on the in-band channel
				const byte* deltaData = msg.GetReadData() + msg.GetReadCount();
				int deltaLength = msg.GetRemainingData();
				
				if( peers[ peerNum ].snapProc->ReceiveSnapshotDelta( deltaData, deltaLength, 0, sequence, baseseq, localSnap, fullSnap ) )
				{
				
					NET_VERBOSESNAPSHOT_PRINT_LEVEL( 2, va( "NET: Got %s snapshot %d delta'd against %d. SS Time: %d\n", ( fullSnap ? "partial" : "full" ), sequence, baseseq, localSnap.GetTime() ) );
					
					if( sessionCB->GetState() != idSession::INGAME && sequence != -1 )
					{
						int seq = peers[ peerNum ].snapProc->GetLastAppendedSequence();
						
						// When we aren't in the game, we need to send this as reliable msg's, since usercmds won't be taking care of it for us
						byte ackbuffer[32];
						idBitMsg ackmsg( ackbuffer, sizeof( ackbuffer ) );
						ackmsg.WriteLong( seq );
						
						// Add incoming BPS for QoS
						float incomingBPS = peers[ peerNum ].receivedBps;
						if( peers[ peerNum ].receivedBpsIndex != seq )
						{
							incomingBPS = idMath::ClampFloat( 0.0f, static_cast<float>( idLobby::BANDWIDTH_REPORTING_MAX ), peers[host].packetProc->GetIncomingRateBytes() );
							peers[ peerNum ].receivedBpsIndex = seq;
							peers[ peerNum ].receivedBps = incomingBPS;
						}
						
						ackmsg.WriteQuantizedUFloat< idLobby::BANDWIDTH_REPORTING_MAX, idLobby::BANDWIDTH_REPORTING_BITS >( incomingBPS );
						QueueReliableMessage( host, RELIABLE_SNAPSHOT_ACK, ackbuffer, sizeof( ackbuffer ) );
					}
				}
				
				if( fullSnap )
				{
					sessionCB->ReceivedFullSnap();
					common->NetReceiveSnapshot( localSnap );
				}
				
				localReadSS = NULL;
				
			}
			else
			{
				// If we are the host, we assume we only receive usercmds on the inband channel
				
				int snapNum = 0;
				uint16 receivedBps_quantized = 0;
				
				byte usercmdBuffer[idPacketProcessor::MAX_FINAL_PACKET_SIZE];
				
				lzwCompressionData_t lzwData;
				idLZWCompressor lzwCompressor( &lzwData );
				lzwCompressor.Start( const_cast<byte*>( msg.GetReadData() ) + msg.GetReadCount(), msg.GetRemainingData() );
				lzwCompressor.ReadAgnostic( snapNum );
				lzwCompressor.ReadAgnostic( receivedBps_quantized );
				int usercmdSize = lzwCompressor.Read( usercmdBuffer, sizeof( usercmdBuffer ), true );
				lzwCompressor.End();
				
				float receivedBps = ( receivedBps_quantized / ( float )( BIT( idLobby::BANDWIDTH_REPORTING_BITS ) - 1 ) ) * ( float )idLobby::BANDWIDTH_REPORTING_MAX;
				if( peers[ peerNum ].receivedBpsIndex != snapNum )
				{
					peers[ peerNum ].receivedBps = receivedBps;
					peers[ peerNum ].receivedBpsIndex = snapNum;
				}
				
				if( snapNum < 50 )
				{
					NET_VERBOSE_PRINT( "NET: peer %d ack'd snapNum %d\n", peerNum, snapNum );
				}
				ApplySnapshotDelta( peerNum, snapNum );
				
				idBitMsg usercmdMsg( ( const byte* )usercmdBuffer, usercmdSize );
				common->NetReceiveUsercmds( peerNum, usercmdMsg );
			}
		}
	}
}

/*
========================
idLobby::HasActivePeers
========================
*/
bool idLobby::HasActivePeers() const
{
	for( int p = 0; p < peers.Num(); p++ )
	{
		if( peers[p].GetConnectionState() != CONNECTION_FREE )
		{
			return true;
		}
	}
	
	return false;
}

/*
========================
idLobby::NumFreeSlots
========================
*/
int idLobby::NumFreeSlots() const
{
	if( parms.matchFlags & MATCH_JOIN_IN_PROGRESS )
	{
		return parms.numSlots - GetNumConnectedUsers();
	}
	else
	{
		return parms.numSlots - GetNumLobbyUsers();
	}
}

//===============================================================================
//	** END PUBLIC INTERFACE ***
//===============================================================================

//===============================================================================
//	** BEGIN STATE CODE ***
//===============================================================================

const char* idLobby::stateToString[ NUM_STATES ] =
{
	ASSERT_ENUM_STRING( STATE_IDLE, 0 ),
	ASSERT_ENUM_STRING( STATE_CREATE_LOBBY_BACKEND, 1 ),
	ASSERT_ENUM_STRING( STATE_SEARCHING, 2 ),
	ASSERT_ENUM_STRING( STATE_OBTAINING_ADDRESS, 3 ),
	ASSERT_ENUM_STRING( STATE_CONNECT_HELLO_WAIT, 4 ),
	ASSERT_ENUM_STRING( STATE_FINALIZE_CONNECT, 5 ),
	ASSERT_ENUM_STRING( STATE_FAILED, 6 ),
};

/*
========================
idLobby::State_Idle
========================
*/
void idLobby::State_Idle()
{
	// If lobbyBackend is in a failed state, shutdown, go to a failed state ourself, and return
	if( lobbyBackend != NULL && lobbyBackend->GetState() == idLobbyBackend::STATE_FAILED )
	{
		HandleConnectionAttemptFailed();
		common->Dialog().ClearDialog( GDM_MIGRATING );
		common->Dialog().ClearDialog( GDM_MIGRATING_WAITING );
		common->Dialog().ClearDialog( GDM_MIGRATING_RELAUNCHING );
		return;
	}
	
	if( migrationInfo.persistUntilGameEndsData.hasGameData && sessionCB->GetState() <= idSession::IDLE )
	{
		// This can happen with 'leaveGame' or 'disconnect' since those paths don't go through endMatch
		// This seems like an ok catch all place but there may be a better way to handle this
		ResetAllMigrationState();
		common->Dialog().ClearDialog( GDM_MIGRATING );
		common->Dialog().ClearDialog( GDM_MIGRATING_WAITING );
		common->Dialog().ClearDialog( GDM_MIGRATING_RELAUNCHING );
	}
}

/*
========================
idLobby::State_Create_Lobby_Backend
========================
*/
void idLobby::State_Create_Lobby_Backend()
{
	if( !verify( lobbyBackend != NULL ) )
	{
		SetState( STATE_FAILED );
		return;
	}
	
	assert( lobbyBackend != NULL );
	
	if( migrationInfo.state == MIGRATE_BECOMING_HOST )
	{
		const int DETECT_SERVICE_DISCONNECT_TIMEOUT_IN_SECONDS = session->GetTitleStorageInt( "DETECT_SERVICE_DISCONNECT_TIMEOUT_IN_SECONDS", 30 );
		
		// If we are taking too long, cancel the connection
		if( DETECT_SERVICE_DISCONNECT_TIMEOUT_IN_SECONDS > 0 )
		{
			if( Sys_Milliseconds() - migrationInfo.migrationStartTime > 1000 * DETECT_SERVICE_DISCONNECT_TIMEOUT_IN_SECONDS )
			{
				SetState( STATE_FAILED );
				return;
			}
		}
	}
	
	if( lobbyBackend->GetState() == idLobbyBackend::STATE_CREATING )
	{
		return;		// Busy but valid
	}
	
	if( lobbyBackend->GetState() != idLobbyBackend::STATE_READY )
	{
		SetState( STATE_FAILED );
		return;
	}
	
	// Success
	InitStateLobbyHost();
	
	// Set state to idle to signify to session we are done creating
	SetState( STATE_IDLE );
}

/*
========================
idLobby::State_Searching
========================
*/
void idLobby::State_Searching()
{
	if( !verify( lobbyBackend != NULL ) )
	{
		SetState( STATE_FAILED );
		return;
	}
	
	if( lobbyBackend->GetState() == idLobbyBackend::STATE_SEARCHING )
	{
		return;		// Busy but valid
	}
	
	if( lobbyBackend->GetState() != idLobbyBackend::STATE_READY )
	{
		SetState( STATE_FAILED );		// Any other lobbyBackend state is invalid
		return;
	}
	
	// Done searching, get results from lobbyBackend
	lobbyBackend->GetSearchResults( searchResults );
	
	if( searchResults.Num() == 0 )
	{
		// If we didn't get any results, set state to failed
		SetState( STATE_FAILED );
		return;
	}
	
	extern idCVar net_maxSearchResultsToTry;
	const int maxSearchResultsToTry = session->GetTitleStorageInt( "net_maxSearchResultsToTry", net_maxSearchResultsToTry.GetInteger() );
	
	if( searchResults.Num() > maxSearchResultsToTry )
	{
		searchResults.SetNum( maxSearchResultsToTry );
	}
	
	// Set state to idle to signify we are done searching
	SetState( STATE_IDLE );
}

/*
========================
idLobby::State_Obtaining_Address
========================
*/
void idLobby::State_Obtaining_Address()
{
	if( lobbyBackend->GetState() == idLobbyBackend::STATE_OBTAINING_ADDRESS )
	{
		return;		// Valid but not ready
	}
	
	if( lobbyBackend->GetState() != idLobbyBackend::STATE_READY )
	{
		// There was an error, signify to caller
		failedReason = migrationInfo.persistUntilGameEndsData.wasMigratedJoin ? FAILED_MIGRATION_CONNECT_FAILED : FAILED_CONNECT_FAILED;
		NET_VERBOSE_PRINT( "idLobby::State_Obtaining_Address: the lobby backend failed." );
		SetState( STATE_FAILED );
		return;
	}
	
	//
	//	We have the address of the lobbyBackend, we can now send a hello packet
	//
	
	// This will be the host for this lobby type
	host = AddPeer( hostAddress, GenerateSessionID() );
	
	// Record start time of connection attempt to the host
	helloStartTime		= Sys_Milliseconds();
	lastConnectRequest	= helloStartTime;
	connectionAttempts	= 0;
	
	// Change state to connecting
	SetState( STATE_CONNECT_HELLO_WAIT );
	
	// Send first connect attempt now (we'll send more periodically if we fail to receive an ack)
	// (we do this after changing state, since the function expects we're in the right state)
	SendConnectionRequest();
}

/*
========================
idLobby::State_Finalize_Connect
========================
*/
void idLobby::State_Finalize_Connect()
{
	if( lobbyBackend->GetState() == idLobbyBackend::STATE_CREATING )
	{
		// Valid but busy
		return;
	}
	
	if( lobbyBackend->GetState() != idLobbyBackend::STATE_READY )
	{
		// Any other state not valid, failed
		SetState( STATE_FAILED );
		return;
	}
	
	// Success
	SetState( STATE_IDLE );
	
	// Tell session mgr if this was a migration
	if( migrationInfo.persistUntilGameEndsData.wasMigratedJoin )
	{
		sessionCB->BecamePeer( *this );
	}
}

/*
========================
idLobby::State_Connect_Hello_Wait
========================
*/
void idLobby::State_Connect_Hello_Wait()
{
	if( lobbyBackend->GetState() != idLobbyBackend::STATE_READY )
	{
		// If the lobbyBackend is in an error state, shut everything down
		NET_VERBOSE_PRINT( "NET: Lobby is no longer ready while waiting for lobbyType %s hello.\n", GetLobbyName() );
		HandleConnectionAttemptFailed();
		return;
	}
	
	int time = Sys_Milliseconds();
	
	const int timeoutMs = session->GetTitleStorageInt( "net_connectTimeoutInSeconds", net_connectTimeoutInSeconds.GetInteger() ) * 1000;
	
	if( timeoutMs != 0 && time - helloStartTime > timeoutMs )
	{
		NET_VERBOSE_PRINT( "NET: Timeout waiting for lobbyType %s for party hello.\n", GetLobbyName() );
		HandleConnectionAttemptFailed();
		return;
	}
	
	if( connectionAttempts < MAX_CONNECT_ATTEMPTS )
	{
		assert( connectionAttempts >= 1 );		// Should have at least the initial connection attempt
		
		// See if we need to send another hello request
		// (keep getting more frequent to increase chance due to possible packet loss, but clamp to MIN_CONNECT_FREQUENCY seconds)
		// TODO: We could eventually make timing out a function of actual number of attempts rather than just plain time.
		int resendTime = Max( MIN_CONNECT_FREQUENCY_IN_SECONDS, CONNECT_REQUEST_FREQUENCY_IN_SECONDS / connectionAttempts ) * 1000;
		
		if( time - lastConnectRequest > resendTime )
		{
			SendConnectionRequest();
			lastConnectRequest = time;
		}
	}
}

/*
========================
idLobby::SetState
========================
*/
void idLobby::SetState( lobbyState_t newState )
{
	assert( newState < NUM_STATES );
	assert( state < NUM_STATES );
	
	verify_array_size( stateToString, NUM_STATES );
	
	if( state == newState )
	{
		NET_VERBOSE_PRINT( "NET: idLobby::SetState: State SAME %s for session %s\n", stateToString[ newState ], GetLobbyName() );
		return;
	}
	
	// Set the current state
	NET_VERBOSE_PRINT( "NET: idLobby::SetState: State changing from %s to %s for session %s\n", stateToString[ state ], stateToString[ newState ], GetLobbyName() );
	
	state = newState;
}

//===============================================================================
//	** END STATE CODE ***
//===============================================================================

/*
========================
idLobby::StartCreating
========================
*/
void idLobby::StartCreating()
{
	assert( lobbyBackend == NULL );
	assert( state == STATE_IDLE );
	
	float skillLevel = GetAverageLocalUserLevel( true );
	
	lobbyBackend = sessionCB->CreateLobbyBackend( parms, skillLevel, ( idLobbyBackend::lobbyBackendType_t )lobbyType );
	
	SetState( STATE_CREATE_LOBBY_BACKEND );
}

/*
========================
idLobby::FindPeer
========================
*/
int idLobby::FindPeer( const lobbyAddress_t& remoteAddress, idPacketProcessor::sessionId_t sessionID, bool ignoreSessionID )
{

	bool connectionless = ( sessionID == idPacketProcessor::SESSION_ID_CONNECTIONLESS_PARTY ||
							sessionID == idPacketProcessor::SESSION_ID_CONNECTIONLESS_GAME ||
							sessionID == idPacketProcessor::SESSION_ID_CONNECTIONLESS_GAME_STATE );
							
	if( connectionless && !ignoreSessionID )
	{
		return -1;		// This was meant to be connectionless. FindPeer is meant for connected (or connecting) peers
	}
	
	for( int p = 0; p < peers.Num(); p++ )
	{
		if( peers[p].GetConnectionState() == CONNECTION_FREE )
		{
			continue;
		}
		
		if( peers[p].address.Compare( remoteAddress ) )
		{
			if( connectionless && ignoreSessionID )
			{
				return p;
			}
			
			// Using a rolling check, so that we account for possible packet loss, and out of order issues
			if( IsPeer() )
			{
				idPacketProcessor::sessionId_t searchStart = peers[p].sessionID;
				
				// Since we only roll the code between matches, we should only need to look ahead a couple increments.
				// Worse case, if the stars line up, the client doesn't see the new sessionId, and times out, and gets booted.
				// This should be impossible though, since the timings won't be possible considering how long it takes to end the match,
				// and restart, and then restart again.
				int numTries = 2;
				
				while( numTries-- > 0 && searchStart != sessionID )
				{
					searchStart = IncrementSessionID( searchStart );
					if( searchStart == sessionID )
					{
						idLib::Printf( "NET: Rolling session ID check found new ID: %i\n", searchStart );
						if( peers[p].packetProc != NULL )
						{
							peers[p].packetProc->VerifyEmptyReliableQueue( RELIABLE_GAME_DATA, RELIABLE_DUMMY_MSG );
						}
						peers[p].sessionID = searchStart;
						break;
					}
				}
			}
			
			if( peers[p].sessionID != sessionID )
			{
				continue;
			}
			return p;
		}
	}
	return -1;
}

/*
========================
idLobby::FindAnyPeer
Find a peer when we don't know the session id, and we don't care since it's a connectionless msg
========================
*/
int idLobby::FindAnyPeer( const lobbyAddress_t& remoteAddress ) const
{

	for( int p = 0; p < peers.Num(); p++ )
	{
		if( peers[p].GetConnectionState() == CONNECTION_FREE )
		{
			continue;
		}
		
		if( peers[p].address.Compare( remoteAddress ) )
		{
			return p;
		}
	}
	return -1;
}

/*
========================
idLobby::FindFreePeer
========================
*/
int idLobby::FindFreePeer() const
{

	// Return the first non active peer
	for( int p = 0; p < peers.Num(); p++ )
	{
		if( !peers[p].IsActive() )
		{
			return p;
		}
	}
	return -1;
}

/*
========================
idLobby::AddPeer
========================
*/
int idLobby::AddPeer( const lobbyAddress_t& remoteAddress, idPacketProcessor::sessionId_t sessionID )
{
	// First, make sure we don't already have this peer
	int p = FindPeer( remoteAddress, sessionID );
	assert( p == -1 );		// When using session ID's, we SHOULDN'T find this remoteAddress/sessionID combo
	
	if( p == -1 )
	{
		// If we didn't find the peer, we need to add a new one
		
		p = FindFreePeer();
		
		if( p == -1 )
		{
			peer_t newPeer;
			p = peers.Append( newPeer );
		}
		
		peer_t& peer = peers[p];
		
		peer.ResetAllData();
		
		assert( peer.connectionState == CONNECTION_FREE );
		
		peer.address	= remoteAddress;
		
		peer.sessionID	= sessionID;
		
		NET_VERBOSE_PRINT( "NET: Added peer %s at index %i\n", remoteAddress.ToString(), p );
	}
	else
	{
		NET_VERBOSE_PRINT( "NET: Found peer %s at index %i\n", remoteAddress.ToString(), p );
	}
	
	SetPeerConnectionState( p, CONNECTION_CONNECTING );
	
	if( lobbyType == GetActingGameStateLobbyType() )
	{
		// Reset various flags used in game mode
		peers[p].ResetMatchData();
	}
	
	return p;
}

/*
========================
idLobby::DisconnectPeerFromSession
========================
*/
void idLobby::DisconnectPeerFromSession( int p )
{
	if( !verify( IsHost() ) )
	{
		return;
	}
	
	peer_t& peer = peers[p];
	
	if( peer.GetConnectionState() != CONNECTION_FREE )
	{
		SetPeerConnectionState( p, CONNECTION_FREE );
	}
}

/*
========================
idLobby::DisconnectAllPeers
========================
*/
void idLobby::DisconnectAllPeers()
{
	for( int p = 0; p < peers.Num(); p++ )
	{
		DisconnectPeerFromSession( p );
	}
}

/*
========================
idLobby::SendGoodbye
========================
*/
void idLobby::SendGoodbye( const lobbyAddress_t& remoteAddress, bool wasFull )
{

	if( net_skipGoodbye.GetBool() )
	{
		return;
	}
	
	NET_VERBOSE_PRINT( "NET: Sending goodbye to %s for %s (wasFull = %i)\n", remoteAddress.ToString(), GetLobbyName(), wasFull );
	
	static const int NUM_REDUNDANT_GOODBYES = 10;
	
	int msgType = OOB_GOODBYE;
	
	if( wasFull )
	{
		msgType = OOB_GOODBYE_FULL;
	}
	else if( lobbyType == TYPE_GAME && ( sessionCB->GetSessionOptions() & idSession::OPTION_LEAVE_WITH_PARTY ) && !( parms.matchFlags & MATCH_PARTY_INVITE_PLACEHOLDER ) )
	{
		msgType = OOB_GOODBYE_W_PARTY;
	}
	
	for( int i = 0; i < NUM_REDUNDANT_GOODBYES; i++ )
	{
		SendConnectionLess( remoteAddress, msgType );
	}
}

/*
========================
idLobby::SetPeerConnectionState
========================
*/
void idLobby::SetPeerConnectionState( int p, connectionState_t newState, bool skipGoodbye )
{

	if( !verify( p >= 0 && p < peers.Num() ) )
	{
		idLib::Printf( "NET: SetPeerConnectionState invalid peer index %i\n", p );
		return;
	}
	
	peer_t& peer = peers[p];
	
	const lobbyType_t actingGameStateLobbyType = GetActingGameStateLobbyType();
	
	if( peer.GetConnectionState() == newState )
	{
		idLib::Printf( "NET: SetPeerConnectionState: Peer already in state %i\n", newState );
		assert( 0 );	// This case means something is most likely bad, and it's the programmers fault
		assert( ( peer.packetProc != NULL ) == peer.IsActive() );
		assert( ( ( peer.snapProc != NULL ) == peer.IsActive() ) == ( actingGameStateLobbyType == lobbyType ) );
		return;
	}
	
	if( newState == CONNECTION_CONNECTING )
	{
		//mem.PushHeap();
		
		// We better be coming from a free connection state if we are trying to connect
		assert( peer.GetConnectionState() == CONNECTION_FREE );
		
		assert( peer.packetProc == NULL );
		peer.packetProc = new( TAG_NETWORKING )idPacketProcessor();
		
		if( lobbyType == actingGameStateLobbyType )
		{
			assert( peer.snapProc == NULL );
			peer.snapProc = new( TAG_NETWORKING )idSnapshotProcessor();
		}
		
		//mem.PopHeap();
	}
	else if( newState == CONNECTION_ESTABLISHED )
	{
		// If we are marking this peer as connected for the first time, make sure this peer was actually trying to connect.
		assert( peer.GetConnectionState() == CONNECTION_CONNECTING );
	}
	else if( newState == CONNECTION_FREE )
	{
		// If we are freeing this connection and we had an established connection before, make sure to send a goodbye
		if( peer.GetConnectionState() == CONNECTION_ESTABLISHED && !skipGoodbye )
		{
			idLib::Printf( "SetPeerConnectionState: Sending goodbye to peer %s from session %s\n", peer.address.ToString(), GetLobbyName() );
			SendGoodbye( peer.address );
		}
	}
	
	peer.connectionState = newState;
	
	if( !peer.IsActive() )
	{
		if( peer.packetProc != NULL )
		{
			delete peer.packetProc;
			peer.packetProc = NULL;
		}
		
		if( peer.snapProc != NULL )
		{
			assert( lobbyType == actingGameStateLobbyType );
			delete peer.snapProc;
			peer.snapProc = NULL;
		}
	}
	
	// Do this in case we disconnected the peer
	if( IsHost() )
	{
		RemoveUsersWithDisconnectedPeers();
	}
}

/*
========================
idLobby::QueueReliableMessage
========================
*/
void idLobby::QueueReliableMessage( int p, byte type, const byte* data, int dataLen )
{
	if( !verify( p >= 0 && p < peers.Num() ) )
	{
		return;
	}
	
	peer_t& peer = peers[p];
	
	if( !peer.IsConnected() )
	{
		// Don't send to this peer if we don't have an established connection of this session type
		NET_VERBOSE_PRINT( "NET: Not sending reliable type %i to peer %i because connectionState is %i\n", type, p, peer.GetConnectionState() );
		return;
	}
	
	if( peer.packetProc->NumQueuedReliables() > 2 )
	{
		idLib::PrintfIf( false, "NET: peer.packetProc->NumQueuedReliables() > 2: %i (%i / %s)\n", peer.packetProc->NumQueuedReliables(), p, peer.address.ToString() );
	}
	
	if( !peer.packetProc->QueueReliableMessage( type, data, dataLen ) )
	{
		// For now, when this happens, disconnect from all session types
		NET_VERBOSE_PRINT( "NET: Dropping peer because we overflowed his reliable message queue\n" );
		if( IsHost() )
		{
			// Disconnect peer from this session type
			DisconnectPeerFromSession( p );
		}
		else
		{
			Shutdown();		// Shutdown session if we can't queue the reliable
		}
	}
}

/*
========================
idLobby::GetNumConnectedPeers
========================
*/
int idLobby::GetNumConnectedPeers() const
{
	int numConnected = 0;
	for( int i = 0; i < peers.Num(); i++ )
	{
		if( peers[i].IsConnected() )
		{
			numConnected++;
		}
	}
	
	return numConnected;
}

/*
========================
idLobby::GetNumConnectedPeersInGame
========================
*/
int idLobby::GetNumConnectedPeersInGame() const
{
	int numActive = 0;
	for( int i = 0; i < peers.Num(); i++ )
	{
		if( peers[i].IsConnected() && peers[i].inGame )
		{
			numActive++;
		}
	}
	
	return numActive;
}


/*
========================
idLobby::SendMatchParmsToPeers
========================
*/
void idLobby::SendMatchParmsToPeers()
{
	if( !IsHost() )
	{
		return;
	}
	
	if( GetNumConnectedPeers() == 0 )
	{
		return;
	}
	
	byte buffer[ idPacketProcessor::MAX_PACKET_SIZE ];
	idBitMsg msg( buffer, sizeof( buffer ) );
	parms.Write( msg );
	
	for( int p = 0; p < peers.Num(); p++ )
	{
		if( !peers[p].IsConnected() )
		{
			continue;
		}
		QueueReliableMessage( p, RELIABLE_MATCH_PARMS, msg.GetReadData(), msg.GetSize() );
	}
}

/*
========================
STATIC idLobby::IsReliablePlayerToPlayerType
========================
*/
bool idLobby::IsReliablePlayerToPlayerType( byte type )
{
	return ( type >= RELIABLE_PLAYER_TO_PLAYER_BEGIN ) && ( type < RELIABLE_PLAYER_TO_PLAYER_END );
}

/*
========================
idLobby::HandleReliablePlayerToPlayerMsg
========================
*/
void idLobby::HandleReliablePlayerToPlayerMsg( int peerNum, idBitMsg& msg, int type )
{
	reliablePlayerToPlayerHeader_t info;
	int c, b;
	msg.SaveReadState( c, b ); // in case we need to forward or fail
	
	if( !info.Read( this, msg ) )
	{
		idLib::Warning( "NET: Ignoring invalid reliable player to player message" );
		msg.RestoreReadState( c, b );
		return;
	}
	
	const bool isForLocalPlayer = IsSessionUserIndexLocal( info.toSessionUserIndex );
	
	if( isForLocalPlayer )
	{
		HandleReliablePlayerToPlayerMsg( info, msg, type );
	}
	else if( IsHost() )
	{
		const int targetPeer = PeerIndexForSessionUserIndex( info.toSessionUserIndex );
		msg.RestoreReadState( c, b );
		// forward the rest of the data
		const byte* data = msg.GetReadData() + msg.GetReadCount();
		int dataLen = msg.GetSize() - msg.GetReadCount();
		
		QueueReliableMessage( targetPeer, type, data, dataLen );
	}
	else
	{
		idLib::Warning( "NET: Can't forward reliable message for remote player: I'm not the host" );
	}
}

/*
========================
idLobby::HandleReliablePlayerToPlayerMsg
========================
*/
void idLobby::HandleReliablePlayerToPlayerMsg( const reliablePlayerToPlayerHeader_t& info, idBitMsg& msg, int reliableType )
{
#if 0
	// Remember that the reliablePlayerToPlayerHeader_t was already removed from the msg
	reliablePlayerToPlayer_t type = ( reliablePlayerToPlayer_t )( reliableType - RELIABLE_PLAYER_TO_PLAYER_BEGIN );
	
	switch( type )
	{
		case RELIABLE_PLAYER_TO_PLAYER_VOICE_EVENT:
		{
			sessionCB->HandleReliableVoiceEvent( *this, info.fromSessionUserIndex, info.toSessionUserIndex, msg );
			break;
		}
		
		default:
		{
			idLib::Warning( "NET: Ignored unknown player to player reliable type %i", ( int ) type );
		}
	};
#endif
}

/*
========================
idLobby::SendConnectionLess
========================
*/
void idLobby::SendConnectionLess( const lobbyAddress_t& remoteAddress, byte type, const byte* data, int dataLen )
{
	idBitMsg msg( data, dataLen );
	msg.SetSize( dataLen );
	
	byte buffer[ idPacketProcessor::MAX_OOB_MSG_SIZE ];
	idBitMsg processedMsg( buffer, sizeof( buffer ) );
	
	// Process the send
	idPacketProcessor::ProcessConnectionlessOutgoing( msg, processedMsg, lobbyType, type );
	
	const bool useDirectPort = ( lobbyType == TYPE_GAME_STATE );
	
	// Send it
	sessionCB->SendRawPacket( remoteAddress, processedMsg.GetReadData(), processedMsg.GetSize(), useDirectPort );
}

/*
========================
idLobby::SendConnectionRequest
========================
*/
void idLobby::SendConnectionRequest()
{
	// Some sanity checking
	assert( state == STATE_CONNECT_HELLO_WAIT );
	assert( peers[host].GetConnectionState() == CONNECTION_CONNECTING );
	assert( GetNumLobbyUsers() == 0 );
	
	// Buffer to hold connect msg
	byte buffer[ idPacketProcessor::MAX_PACKET_SIZE - 2 ];
	idBitMsg msg( buffer, sizeof( buffer ) );
	
	// Add the current version info to the handshake
	const unsigned int localChecksum = NetGetVersionChecksum(); // DG: use int instead of long for 64bit compatibility
	
	NET_VERBOSE_PRINT( "NET: version = %u\n", localChecksum );
	
	msg.WriteLong( localChecksum );
	msg.WriteUShort( peers[host].sessionID );
	msg.WriteBool( connectIsFromInvite );
	
	// We use InitSessionUsersFromLocalUsers here to copy the current local users over to session users simply to have a list
	// to send on the initial connection attempt.  We immediately clear our session user list once sent.
	InitSessionUsersFromLocalUsers( true );
	
	if( GetNumLobbyUsers() > 0 )
	{
		// Fill up the msg with the users on this machine
		msg.WriteByte( GetNumLobbyUsers() );
		
		for( int u = 0; u < GetNumLobbyUsers(); u++ )
		{
			GetLobbyUser( u )->WriteToMsg( msg );
		}
	}
	else
	{
		FreeAllUsers();
		SetState( STATE_FAILED );
		
		return;
	}
	
	// We just used these users to fill up the msg above, we will get the real list from the server if we connect.
	FreeAllUsers();
	
	NET_VERBOSE_PRINT( "NET: Sending hello to: %s (lobbyType: %s, session ID %i, attempt: %i)\n", hostAddress.ToString(), GetLobbyName(), peers[host].sessionID, connectionAttempts );
	
	SendConnectionLess( hostAddress, OOB_HELLO, msg.GetReadData(), msg.GetSize() );
	
	connectionAttempts++;
}

/*
========================
idLobby::ConnectTo

Fires off a request to get the address of a lobbyBackend owner, and then attempts to connect (eventually handled in HandleObtainingLobbyOwnerAddress)
========================
*/
void idLobby::ConnectTo( const lobbyConnectInfo_t& connectInfo, bool fromInvite )
{
	NET_VERBOSE_PRINT( "NET: idSessionLocal::ConnectTo: fromInvite = %i\n", fromInvite );
	
	// Make sure current session is shutdown
	Shutdown();
	
	connectIsFromInvite = fromInvite;
	
	lobbyBackend = sessionCB->JoinFromConnectInfo( connectInfo, ( idLobbyBackend::lobbyBackendType_t )lobbyType );
	
	// First, we need the address of the lobbyBackend owner
	lobbyBackend->GetOwnerAddress( hostAddress );
	
	SetState( STATE_OBTAINING_ADDRESS );
	
}

/*
========================
idLobby::HandleGoodbyeFromPeer
========================
*/
void idLobby::HandleGoodbyeFromPeer( int peerNum, lobbyAddress_t& remoteAddress, int msgType )
{
	if( migrationInfo.state != MIGRATE_NONE )
	{
		// If this peer is on our invite list, remove them
		for( int i = 0; i < migrationInfo.invites.Num(); i++ )
		{
			if( migrationInfo.invites[i].address.Compare( remoteAddress, true ) )
			{
				migrationInfo.invites.RemoveIndex( i );
				break;
			}
		}
	}
	
	if( peerNum < 0 )
	{
		NET_VERBOSE_PRINT( "NET: Goodbye from unknown peer %s on session %s\n", remoteAddress.ToString(), GetLobbyName() );
		return;
	}
	
	if( peers[peerNum].GetConnectionState() == CONNECTION_FREE )
	{
		NET_VERBOSE_PRINT( "NET: Goodbye from peer %s on session %s that is not connected\n", remoteAddress.ToString(), GetLobbyName() );
		return;
	}
	
	if( IsHost() )
	{
		// Goodbye from peer, remove him
		NET_VERBOSE_PRINT( "NET: Goodbye from peer %s, on session %s\n", remoteAddress.ToString(), GetLobbyName() );
		DisconnectPeerFromSession( peerNum );
	}
	else
	{
		// Let session handler take care of this
		NET_VERBOSE_PRINT( "NET: Goodbye from host %s, on session %s\n", remoteAddress.ToString(), GetLobbyName() );
		sessionCB->GoodbyeFromHost( *this, peerNum, remoteAddress, msgType );
	}
}

/*
========================
idLobby::HandleGoodbyeFromPeer
========================
*/
void idLobby::HandleConnectionAttemptFailed()
{
	Shutdown();
	failedReason = migrationInfo.persistUntilGameEndsData.wasMigratedJoin ? FAILED_MIGRATION_CONNECT_FAILED : FAILED_CONNECT_FAILED;
	SetState( STATE_FAILED );
	
	if( migrationInfo.persistUntilGameEndsData.wasMigratedJoin )
	{
		sessionCB->FailedGameMigration( *this );
	}
	
	ResetAllMigrationState();
	
	needToDisplayMigrateMsg = false;
	migrateMsgFlags			= 0;
}

/*
========================
idLobby::ConnectToNextSearchResult
========================
*/
bool idLobby::ConnectToNextSearchResult()
{
	if( lobbyType != TYPE_GAME )
	{
		return false;		// Only game sessions use matchmaking searches
	}
	
	// End current session lobby (this WON'T free search results)
	Shutdown();
	
	if( searchResults.Num() == 0 )
	{
		return false;		// No more search results to connect to, give up
	}
	
	// Get next search result
	lobbyConnectInfo_t connectInfo = searchResults[0];
	
	// Remove this search result
	searchResults.RemoveIndex( 0 );
	
	// If we are connecting to a game lobby, tell our party to connect to this lobby as well
	if( lobbyType == TYPE_GAME && sessionCB->GetPartyLobby().IsLobbyActive() )
	{
		sessionCB->GetPartyLobby().SendMembersToLobby( lobbyType, connectInfo, true );
	}
	
	// Attempt to connect the lobby
	ConnectTo( connectInfo, true );		// Pass in true for invite, since searches are for matchmaking, and we should always be able to connect to those types of matches
	
	// Clear the "Lobby was Full" dialog in case it's up, since we are going to try to connect to a different lobby now
	common->Dialog().ClearDialog( GDM_LOBBY_FULL );
	
	return true;	// Notify caller we are attempting to connect
}

/*
========================
idLobby::CheckVersion
========================
*/
bool idLobby::CheckVersion( idBitMsg& msg, lobbyAddress_t peerAddress )
{
	const unsigned int remoteChecksum = msg.ReadLong(); // DG: use int instead of long for 64bit compatibility
	
	if( net_checkVersion.GetInteger() == 1 )
	{
		const unsigned int localChecksum = NetGetVersionChecksum(); // DG: use int instead of long for 64bit compatibility
		
		NET_VERBOSE_PRINT( "NET: Comparing handshake version - localChecksum = %u, remoteChecksum = %u\n", localChecksum, remoteChecksum );
		return ( remoteChecksum == localChecksum );
	}
	return true;
}

/*
========================
idLobby::VerifyNumConnectingUsers
Make sure number of users connecting is valid, and make sure we have enough room
========================
*/
bool idLobby::VerifyNumConnectingUsers( idBitMsg& msg )
{
	int c, b;
	msg.SaveReadState( c, b );
	const int numUsers = msg.ReadByte();
	msg.RestoreReadState( c, b );
	
	const int numFreeSlots = NumFreeSlots();
	
	NET_VERBOSE_PRINT( "NET: VerifyNumConnectingUsers %i users, %i free slots for %s\n", numUsers, numFreeSlots, GetLobbyName() );
	
	if( numUsers <= 0 || numUsers > MAX_PLAYERS - 1 )
	{
		NET_VERBOSE_PRINT( "NET: Invalid numUsers %i\n", numUsers );
		return false;
	}
	else if( numUsers > numFreeSlots )
	{
		NET_VERBOSE_PRINT( "NET: %i slots requested, but only %i are available\n", numUsers, numFreeSlots );
		return false;
	}
	else if( lobbyType == TYPE_PARTY && sessionCB->GetState() >= idSession::GAME_LOBBY && sessionCB->GetGameLobby().IsLobbyActive() && !IsMigrating() )
	{
		const int numFreeGameSlots = sessionCB->GetGameLobby().NumFreeSlots();
		
		if( numUsers > numFreeGameSlots )
		{
			NET_VERBOSE_PRINT( "NET: %i slots requested, but only %i are available on the active game session\n", numUsers, numFreeGameSlots );
			return false;
		}
	}
	
	return true;
}

/*
========================
idLobby::VerifyLobbyUserIDs
========================
*/
bool idLobby::VerifyLobbyUserIDs( idBitMsg& msg )
{
	int c, b;
	msg.SaveReadState( c, b );
	const int numUsers = msg.ReadByte();
	
	// Add the new users to our own list
	for( int u = 0; u < numUsers; u++ )
	{
		lobbyUser_t newUser;
		
		// Read in the new user
		newUser.ReadFromMsg( msg );
		
		if( GetLobbyUserIndexByID( newUser.lobbyUserID, true ) != -1 )
		{
			msg.RestoreReadState( c, b );
			return false;
		}
	}
	
	msg.RestoreReadState( c, b );
	
	return true;
}

/*
========================
idLobby::HandleInitialPeerConnection
Received on an initial peer connect request (OOB_HELLO)
========================
*/
int idLobby::HandleInitialPeerConnection( idBitMsg& msg, const lobbyAddress_t& peerAddress, int peerNum )
{
	if( net_ignoreConnects.GetInteger() > 0 )
	{
		if( net_ignoreConnects.GetInteger() == 2 )
		{
			SendGoodbye( peerAddress );
		}
		return -1;
	}
	
	if( !IsHost() )
	{
		NET_VERBOSE_PRINT( "NET: Got connectionless hello from peer %s (num %i) on session, and we are not a host\n", peerAddress.ToString(), peerNum );
		SendGoodbye( peerAddress );
		return -1;
	}
	
	// See if this is a peer migrating to us, if so, remove them from our invite list
	bool migrationInvite = false;
	int migrationGameData = -1;
	
	
	for( int i = migrationInfo.invites.Num() - 1; i >= 0; i-- )
	{
		if( migrationInfo.invites[i].address.Compare( peerAddress, true ) )
		{
		
			migrationGameData = migrationInfo.invites[i].migrationGameData;
			migrationInfo.invites.RemoveIndex( i );	// Remove this peer from the list, since this peer will now be connected (or rejected, either way we don't want to keep sending invites)
			migrationInvite = true;
			NET_VERBOSE_PRINT( "^2NET: Response from migration invite %s. GameData: %d\n", peerAddress.ToString(), migrationGameData );
		}
	}
	
	if( !MatchTypeIsJoinInProgress( parms.matchFlags ) && lobbyType == TYPE_GAME && migrationInfo.persistUntilGameEndsData.wasMigratedHost && IsMigratedStatsGame() && !migrationInvite )
	{
		// No matter what, don't let people join migrated game sessions that are going to continue on to the same game
		// Not on invite list in a migrated game session - bounce him
		NET_VERBOSE_PRINT( "NET: Denying game connection from %s since not on migration invite list\n", peerAddress.ToString() );
		for( int i = migrationInfo.invites.Num() - 1; i >= 0; i-- )
		{
			NET_VERBOSE_PRINT( "   Invite[%d] addr: %s\n", i, migrationInfo.invites[i].address.ToString() );
		}
		SendGoodbye( peerAddress );
		return -1;
	}
	
	
	if( MatchTypeIsJoinInProgress( parms.matchFlags ) )
	{
		// If this is for a game connection, make sure we have a game lobby
		if( ( lobbyType == TYPE_GAME || lobbyType == TYPE_GAME_STATE ) && sessionCB->GetState() < idSession::GAME_LOBBY )
		{
			NET_VERBOSE_PRINT( "NET: Denying game connection from %s because we don't have a game lobby\n", peerAddress.ToString() );
			SendGoodbye( peerAddress );
			return -1;
		}
	}
	else
	{
		// If this is for a game connection, make sure we are in the game lobby
		if( lobbyType == TYPE_GAME && sessionCB->GetState() != idSession::GAME_LOBBY )
		{
			NET_VERBOSE_PRINT( "NET: Denying game connection from %s while not in game lobby\n", peerAddress.ToString() );
			SendGoodbye( peerAddress );
			return -1;
		}
		
		// If this is for a party connection, make sure we are not in game, unless this was for host migration invite
		if( !migrationInvite && lobbyType == TYPE_PARTY && ( sessionCB->GetState() == idSession::INGAME || sessionCB->GetState() == idSession::LOADING ) )
		{
			NET_VERBOSE_PRINT( "NET: Denying party connection from %s because we were already in a game\n", peerAddress.ToString() );
			SendGoodbye( peerAddress );
			return -1;
		}
	}
	
	if( !CheckVersion( msg, peerAddress ) )
	{
		idLib::Printf( "NET: Denying user %s with wrong version number\n", peerAddress.ToString() );
		SendGoodbye( peerAddress );
		return -1;
	}
	
	idPacketProcessor::sessionId_t sessionID = msg.ReadUShort();
	
	// Check to see if this is a peer trying to connect with a different sessionID
	// If the peer got abruptly disconnected, the peer could be trying to reconnect from a non clean disconnect
	if( peerNum >= 0 )
	{
		peer_t& existingPeer = peers[peerNum];
		
		assert( existingPeer.GetConnectionState() != CONNECTION_FREE );
		
		if( existingPeer.sessionID == sessionID )
		{
			return peerNum;		// If this is the same sessionID, then assume redundant connection attempt
		}
		
		//
		// This peer must be trying to reconnect from a previous abrupt disconnect
		//
		
		NET_VERBOSE_PRINT( "NET: Reconnecting peer %s for session %s\n", peerAddress.ToString(), GetLobbyName() );
		
		// Assume a peer is trying to reconnect from a non clean disconnect
		// We want to set the connection back to FREE manually, so we don't send a goodbye
		existingPeer.connectionState = CONNECTION_FREE;
		
		if( existingPeer.packetProc != NULL )
		{
			delete existingPeer.packetProc;
			existingPeer.packetProc = NULL;
		}
		
		if( existingPeer.snapProc != NULL )
		{
			assert( lobbyType == TYPE_GAME );		// Only games sessions should be creating snap processors
			delete existingPeer.snapProc;
			existingPeer.snapProc = NULL;
		}
		
		RemoveUsersWithDisconnectedPeers();
		
		peerNum = -1;
	}
	
	// See if this was from an invite we sent out. If it wasn't, make sure we aren't invite only
	const bool fromInvite = msg.ReadBool();
	
	if( !fromInvite && MatchTypeInviteOnly( parms.matchFlags ) )
	{
		idLib::Printf( "NET: Denying user %s because they were not invited to an invite only match\n", peerAddress.ToString() );
		SendGoodbye( peerAddress );
		return -1;
	}
	
	// Make sure we have room for the users connecting
	if( !VerifyNumConnectingUsers( msg ) )
	{
		NET_VERBOSE_PRINT( "NET: Denying connection from %s in session %s due to being out of user slots\n", peerAddress.ToString(), GetLobbyName() );
		SendGoodbye( peerAddress, true );
		return -1;
	}
	
	// Make sure there are no lobby id conflicts
	if( !verify( VerifyLobbyUserIDs( msg ) ) )
	{
		NET_VERBOSE_PRINT( "NET: Denying connection from %s in session %s due to lobby id conflict\n", peerAddress.ToString(), GetLobbyName() );
		SendGoodbye( peerAddress, true );
		return -1;
	}
	
	// Calling AddPeer will set our connectionState to this peer as CONNECTION_CONNECTING (which will get set to CONNECTION_ESTABLISHED below)
	peerNum = AddPeer( peerAddress, sessionID );
	
	peer_t& newPeer = peers[peerNum];
	
	assert( newPeer.GetConnectionState() == CONNECTION_CONNECTING );
	assert( lobbyType != GetActingGameStateLobbyType() || newPeer.snapProc != NULL );
	
	// First, add users from this new peer to our user list
	// (which will then forward the list to all peers except peerNum)
	AddUsersFromMsg( msg, peerNum );
	
	// Mark the peer as connected for this session type
	SetPeerConnectionState( peerNum, CONNECTION_ESTABLISHED );
	
	// Update their heart beat to current
	newPeer.lastHeartBeat = Sys_Milliseconds();
	
	byte buffer[ idPacketProcessor::MAX_PACKET_SIZE ];
	idBitMsg outmsg( buffer, sizeof( buffer ) );
	
	// Let them know their peer index on this host
	// peerIndexOnHost (put this here so it shows up in search results when finding out where it's used/referenced)
	outmsg.WriteLong( peerNum );
	
	// If they are connecting to our party lobby, let them know the party token
	if( lobbyType == TYPE_PARTY )
	{
		outmsg.WriteLong( GetPartyTokenAsHost() );
	}
	
	if( lobbyType == TYPE_GAME || lobbyType == TYPE_GAME_STATE )
	{
		// If this is a game session, reset the loading and ingame flags
		newPeer.loaded = false;
		newPeer.inGame = false;
	}
	
	// Write out current match parms
	parms.Write( outmsg );
	
	// Send list of existing users to this new peer
	// (the users from the new peer will also be in this list, since we already called AddUsersFromMsg)
	outmsg.WriteByte( GetNumLobbyUsers() );
	
	for( int u = 0; u < GetNumLobbyUsers(); u++ )
	{
		GetLobbyUser( u )->WriteToMsg( outmsg );
	}
	
	lobbyBackend->FillMsgWithPostConnectInfo( outmsg );
	
	NET_VERBOSE_PRINT( "NET: Sending response to %s, lobbyType %s, sessionID %i\n", peerAddress.ToString(), GetLobbyName(), sessionID );
	
	QueueReliableMessage( peerNum, RELIABLE_HELLO, outmsg.GetReadData(), outmsg.GetSize() );
	
	if( MatchTypeIsJoinInProgress( parms.matchFlags ) )
	{
		// If have an active game lobby, and someone joins our party, tell them to join our game
		if( lobbyType == TYPE_PARTY && sessionCB->GetState() >= idSession::GAME_LOBBY )
		{
			SendPeerMembersToLobby( peerNum, TYPE_GAME, false );
		}
		
		// We are are ingame, then start the client loading immediately
		if( ( lobbyType == TYPE_GAME || lobbyType == TYPE_GAME_STATE ) && sessionCB->GetState() >= idSession::LOADING )
		{
			idLib::Printf( "******* JOIN IN PROGRESS ********\n" );
			if( sessionCB->GetState() == idSession::INGAME )
			{
				newPeer.pauseSnapshots = true;		// Since this player joined in progress, let game dictate when to start sending snaps
			}
			QueueReliableMessage( peerNum, idLobby::RELIABLE_START_LOADING );
		}
	}
	else
	{
		// If we are in a game lobby, and someone joins our party, tell them to join our game
		if( lobbyType == TYPE_PARTY && sessionCB->GetState() == idSession::GAME_LOBBY )
		{
			SendPeerMembersToLobby( peerNum, TYPE_GAME, false );
		}
	}
	
	// Send mic status of the current lobby to applicable peers
	SendPeersMicStatusToNewUsers( peerNum );
	
	// If we made is this far, update the users migration game data index
	for( int u = 0; u < GetNumLobbyUsers(); u++ )
	{
		if( GetLobbyUser( u )->peerIndex == peerNum )
		{
			GetLobbyUser( u )->migrationGameData = migrationGameData;
		}
	}
	
	return peerNum;
}

/*
========================
idLobby::InitStateLobbyHost
========================
*/
void idLobby::InitStateLobbyHost()
{
	assert( lobbyBackend != NULL );
	
	// We will be the host
	isHost = true;
	
	if( net_headlessServer.GetBool() )
	{
		return;		// Don't add any players to headless server
	}
	
	if( migrationInfo.state != MIGRATE_NONE )
	{
		migrationInfo.persistUntilGameEndsData.wasMigratedHost = true; // InitSessionUsersFromLocalUsers needs to know this
		migrationInfo.persistUntilGameEndsData.hasRelaunchedMigratedGame = false;
		// migrationDlg = GDM_MIGRATING_WAITING;
	}
	
	// Initialize the initial user list for this lobby
	InitSessionUsersFromLocalUsers( MatchTypeIsOnline( parms.matchFlags ) );
	
	// Set the session's hostAddress to the local players' address.
	const int myUserIndex = GetLobbyUserIndexByLocalUserHandle( sessionCB->GetSignInManager().GetMasterLocalUserHandle() );
	if( myUserIndex != -1 )
	{
		hostAddress = GetLobbyUser( myUserIndex )->address;
	}
	
	// Since we are the host, we have to register our initial session users with the lobby
	// All additional users will join through AddUsersFromMsg, and RegisterUser is handled in there from here on out.
	// Peers will add users exclusively through AddUsersFromMsg.
	for( int i = 0; i < GetNumLobbyUsers(); i++ )
	{
		lobbyUser_t* user = GetLobbyUser( i );
		RegisterUser( user );
		if( lobbyType == TYPE_PARTY )
		{
			user->partyToken = GetPartyTokenAsHost();
		}
	}
	
	// Set the lobbies skill level
	lobbyBackend->UpdateLobbySkill( GetAverageSessionLevel() );
	
	// Make sure and register all the addresses of the invites we'll send out as the new host
	if( migrationInfo.state != MIGRATE_NONE )
	{
		// Tell the session that we became the host, so the session mgr can adjust state if needed
		sessionCB->BecameHost( *this );
		
		// Register this address with this lobbyBackend
		for( int i = 0; i < migrationInfo.invites.Num(); i++ )
		{
			lobbyBackend->RegisterAddress( migrationInfo.invites[i].address );
		}
	}
}

/*
========================
idLobby::SendMembersToLobby
========================
*/
void idLobby::SendMembersToLobby( lobbyType_t destLobbyType, const lobbyConnectInfo_t& connectInfo, bool waitForOtherMembers )
{

	// It's not our job to send party members to a game if we aren't the party host
	if( !IsHost() )
	{
		return;
	}
	
	// Send the message to all connected peers
	for( int i = 0; i < peers.Num(); i++ )
	{
		if( peers[ i ].IsConnected() )
		{
			SendPeerMembersToLobby( i, destLobbyType, connectInfo, waitForOtherMembers );
		}
	}
}

/*
========================
idLobby::SendMembersToLobby
========================
*/
void idLobby::SendMembersToLobby( idLobby& destLobby, bool waitForOtherMembers )
{
	if( destLobby.lobbyBackend == NULL )
	{
		return;		// We don't have a game lobbyBackend to get an address for
	}
	
	lobbyConnectInfo_t connectInfo = destLobby.lobbyBackend->GetConnectInfo();
	
	SendMembersToLobby( destLobby.lobbyType, connectInfo, waitForOtherMembers );
}

/*
========================
idLobby::SendPeerMembersToLobby
Give the address of a game lobby to a particular peer, notifying that peer to send a hello to the same server.
========================
*/
void idLobby::SendPeerMembersToLobby( int peerIndex, lobbyType_t destLobbyType, const lobbyConnectInfo_t& connectInfo, bool waitForOtherMembers )
{
	// It's not our job to send party members to a game if we aren't the party host
	if( !IsHost() )
	{
		return;
	}
	
	assert( peerIndex >= 0 );
	assert( peerIndex < peers.Num() );
	peer_t& peer = peers[ peerIndex ];
	
	NET_VERBOSE_PRINT( "NET: Sending peer %i (%s) to game lobby\n", peerIndex, peer.address.ToString() );
	
	if( !peer.IsConnected() )
	{
		idLib::Warning( "NET: Can't send peer %i to game lobby: peer isn't in party", peerIndex );
		return;
	}
	
	byte buffer[ idPacketProcessor::MAX_PACKET_SIZE - 2 ];
	idBitMsg outmsg( buffer, sizeof( buffer ) );
	
	// Have lobby fill out msg with connection info
	connectInfo.WriteToMsg( outmsg );
	
	outmsg.WriteByte( destLobbyType );
	outmsg.WriteBool( waitForOtherMembers );
	
	QueueReliableMessage( peerIndex, RELIABLE_CONNECT_AND_MOVE_TO_LOBBY, outmsg.GetReadData(), outmsg.GetSize() );
}

/*
========================
idLobby::SendPeerMembersToLobby

Give the address of a game lobby to a particular peer, notifying that peer to send a hello to the same server.
========================
*/
void idLobby::SendPeerMembersToLobby( int peerIndex, lobbyType_t destLobbyType, bool waitForOtherMembers )
{
	idLobby* lobby = sessionCB->GetLobbyFromType( destLobbyType );
	
	if( !verify( lobby != NULL ) )
	{
		return;
	}
	
	if( !verify( lobby->lobbyBackend != NULL ) )
	{
		return;
	}
	
	lobbyConnectInfo_t connectInfo = lobby->lobbyBackend->GetConnectInfo();
	
	SendPeerMembersToLobby( peerIndex, destLobbyType, connectInfo, waitForOtherMembers );
}

/*
========================
idLobby::NotifyPartyOfLeavingGameLobby
========================
*/
void idLobby::NotifyPartyOfLeavingGameLobby()
{
	if( lobbyType != TYPE_PARTY )
	{
		return;		// We are not a party lobby
	}
	
	if( !IsHost() )
	{
		return;		// We are not the host of a party lobby, we can't do this
	}
	
	if( !( sessionCB->GetSessionOptions() & idSession::OPTION_LEAVE_WITH_PARTY ) )
	{
		return;		// Options aren't set to notify party of leaving
	}
	
	// Tell our party to leave the game they are in
	for( int i = 0; i < peers.Num(); i++ )
	{
		if( peers[ i ].IsConnected() )
		{
			QueueReliableMessage( i, RELIABLE_PARTY_LEAVE_GAME_LOBBY );
		}
	}
}

/*
========================
idLobby::GetPartyTokenAsHost
========================
*/
uint32 idLobby::GetPartyTokenAsHost()
{
	assert( lobbyType == TYPE_PARTY );
	assert( IsHost() );
	
	if( partyToken == 0 )
	{
		// I don't know if this is mathematically sound, but it seems reasonable.
		// Don't do this at app startup (i.e. in the constructor) or it will be a lot less random.
		// DG: use int instead of long for 64bit compatibility
		unsigned int seed = Sys_Milliseconds(); // time app has been running
		// DG end
		idLocalUser* masterUser = session->GetSignInManager().GetMasterLocalUser();
		if( masterUser != NULL )
		{
			seed += idStr::Hash( masterUser->GetGamerTag() );
		}
		partyToken = idRandom( seed ).RandomInt();
		idLib::Printf( "NET: PartyToken is %u (seed = %u)\n", partyToken, seed );
	}
	return partyToken;
}

/*
========================
idLobby::EncodeSessionID
========================
*/
idPacketProcessor::sessionId_t idLobby::EncodeSessionID( uint32 key ) const
{
	assert( sizeof( uint32 ) >= sizeof( idPacketProcessor::sessionId_t ) );
	const int numBits = sizeof( idPacketProcessor::sessionId_t ) * 8 - idPacketProcessor::NUM_LOBBY_TYPE_BITS;
	const uint32 mask = ( 1 << numBits ) - 1;
	idPacketProcessor::sessionId_t sessionID = ( key & mask ) << idPacketProcessor::NUM_LOBBY_TYPE_BITS;
	sessionID |= ( lobbyType + 1 );
	return sessionID;
}

/*
========================
idLobby::EncodeSessionID
========================
*/
void idLobby::DecodeSessionID( idPacketProcessor::sessionId_t sessionID, uint32& key ) const
{
	assert( sizeof( uint32 ) >= sizeof( idPacketProcessor::sessionId_t ) );
	key = sessionID >> idPacketProcessor::NUM_LOBBY_TYPE_BITS;
}

/*
========================
idLobby::GenerateSessionID
========================
*/
idPacketProcessor::sessionId_t idLobby::GenerateSessionID() const
{
	idPacketProcessor::sessionId_t sessionID = EncodeSessionID( Sys_Milliseconds() );
	
	// Make sure we can use it
	while( !SessionIDCanBeUsedForInBand( sessionID ) )
	{
		sessionID = IncrementSessionID( sessionID );
	}
	
	return sessionID;
}

/*
========================
idLobby::SessionIDCanBeUsedForInBand
========================
*/
bool idLobby::SessionIDCanBeUsedForInBand( idPacketProcessor::sessionId_t sessionID ) const
{
	if( sessionID == idPacketProcessor::SESSION_ID_INVALID )
	{
		return false;
	}
	
	if( sessionID == idPacketProcessor::SESSION_ID_CONNECTIONLESS_PARTY )
	{
		return false;
	}
	
	if( sessionID == idPacketProcessor::SESSION_ID_CONNECTIONLESS_GAME )
	{
		return false;
	}
	
	if( sessionID == idPacketProcessor::SESSION_ID_CONNECTIONLESS_GAME_STATE )
	{
		return false;
	}
	
	return true;
}

/*
========================
idLobby::IncrementSessionID
========================
*/
idPacketProcessor::sessionId_t idLobby::IncrementSessionID( idPacketProcessor::sessionId_t sessionID ) const
{
	// Increment, taking into account valid id's
	while( 1 )
	{
		uint32 key = 0;
		
		DecodeSessionID( sessionID, key );
		
		key++;
		
		sessionID = EncodeSessionID( key );
		
		if( SessionIDCanBeUsedForInBand( sessionID ) )
		{
			break;
		}
	}
	
	return sessionID;
}

#define VERIFY_CONNECTED_PEER( p, sessionType_, msgType )				\
	if ( !verify( lobbyType == sessionType_ ) ) {						\
		idLib::Printf( "NET: " #msgType ", peer:%s invalid session type for " #sessionType_ " %i.\n", peer.address.ToString(), sessionType_ );	\
		return;															\
	}																	\
	if ( peers[p].GetConnectionState() != CONNECTION_ESTABLISHED ) {	\
		idLib::Printf( "NET: " #msgType ", peer:%s not connected for " #sessionType_ " %i.\n", peer.address.ToString(), sessionType_ );	\
		return;															\
	}

#define VERIFY_CONNECTING_PEER( p, sessionType_, msgType )				\
	if ( !verify( lobbyType == sessionType_ ) ) {						\
		idLib::Printf( "NET: " #msgType ", peer:%s invalid session type for " #sessionType_ " %i.\n", peer.address.ToString(), sessionType_ );	\
		return;															\
	}																	\
	if ( peers[p].GetConnectionState() != CONNECTION_CONNECTING ) {		\
		idLib::Printf( "NET: " #msgType ", peer:%s not connecting for " #sessionType_ " %i.\n", peer.address.ToString(), sessionType_ );	\
		return;															\
	}

#define VERIFY_FROM_HOST( p, sessionType_, msgType )					\
	VERIFY_CONNECTED_PEER( p, sessionType_, msgType );					\
	if ( p != host ) {													\
		idLib::Printf( "NET: "#msgType", not from "#sessionType_" host: %s\n", peer.address.ToString() );	\
		return;															\
	}																	\

#define VERIFY_FROM_CONNECTING_HOST( p, sessionType_, msgType )			\
	VERIFY_CONNECTING_PEER( p, sessionType_, msgType );					\
	if ( p != host ) {													\
		idLib::Printf( "NET: "#msgType", not from "#sessionType_" host: %s\n", peer.address.ToString() );	\
		return;															\
	}																	\

/*
========================
idLobby::HandleHelloAck
========================
*/
void idLobby::HandleHelloAck( int p, idBitMsg& msg )
{
	peer_t& peer = peers[p];
	
	if( state != STATE_CONNECT_HELLO_WAIT )
	{
		idLib::Printf( "NET: Hello ack for session type %s while not waiting for hello.\n", GetLobbyName() );
		SendGoodbye( peer.address );		// We send a customary goodbye to make sure we are not in their list anymore
		return;
	}
	if( p != host )
	{
		// This shouldn't be possible
		idLib::Printf( "NET: Hello ack for session type %s, not from correct host.\n", GetLobbyName() );
		SendGoodbye( peer.address );		// We send a customary goodbye to make sure we are not in their list anymore
		return;
	}
	
	assert( GetNumLobbyUsers() == 0 );
	
	NET_VERBOSE_PRINT( "NET: Hello ack for session type %s from %s\n", GetLobbyName(), peer.address.ToString() );
	
	// We are now connected to this session type
	SetPeerConnectionState( p, CONNECTION_ESTABLISHED );
	
	// Obtain what our peer index is on the host is
	peerIndexOnHost = msg.ReadLong();
	
	// If we connected to a party lobby, get the party token from the lobby owner
	if( lobbyType == TYPE_PARTY )
	{
		partyToken = msg.ReadLong();
	}
	
	// Read match parms
	parms.Read( msg );
	
	// Update lobbyBackend with parms
	if( lobbyBackend != NULL )
	{
		lobbyBackend->UpdateMatchParms( parms );
	}
	
	// Populate the user list with the one from the host (which will also include our local users)
	// This ensures the user lists are kept in sync
	FreeAllUsers();
	AddUsersFromMsg( msg, p );
	
	// Make sure the host has a current heartbeat
	peer.lastHeartBeat = Sys_Milliseconds();
	
	lobbyBackend->PostConnectFromMsg( msg );
	
	// Tell the lobby controller to finalize the connection
	SetState( STATE_FINALIZE_CONNECT );
	
	//
	// Success - We've received an ack from the server, letting us know we've been registered with the lobbies
	//
}

/*
========================
idLobby::GetLobbyUserName
========================
*/
const char* idLobby::GetLobbyUserName( lobbyUserID_t lobbyUserID ) const
{
	const int index = GetLobbyUserIndexByID( lobbyUserID );
	const lobbyUser_t* user = GetLobbyUser( index );
	
	if( user == NULL )
	{
		for( int i = 0; i < disconnectedUsers.Num(); i++ )
		{
			if( disconnectedUsers[i].lobbyUserID.CompareIgnoreLobbyType( lobbyUserID ) )
			{
				return disconnectedUsers[i].gamertag;
			}
		}
		return INVALID_LOBBY_USER_NAME;
	}
	
	return user->gamertag;
}

/*
========================
idLobby::GetLobbyUserSkinIndex
========================
*/
int idLobby::GetLobbyUserSkinIndex( lobbyUserID_t lobbyUserID ) const
{
	const int userIndex = GetLobbyUserIndexByID( lobbyUserID );
	const lobbyUser_t* user = GetLobbyUser( userIndex );
	return user ? user->selectedSkin : 0;
}

/*
========================
idLobby::GetLobbyUserWeaponAutoSwitch
========================
*/
bool idLobby::GetLobbyUserWeaponAutoSwitch( lobbyUserID_t lobbyUserID ) const
{
	const int userIndex = GetLobbyUserIndexByID( lobbyUserID );
	const lobbyUser_t* user = GetLobbyUser( userIndex );
	return user ? user->weaponAutoSwitch : true;
}

/*
========================
idLobby::GetLobbyUserWeaponAutoReload
========================
*/
bool idLobby::GetLobbyUserWeaponAutoReload( lobbyUserID_t lobbyUserID ) const
{
	const int userIndex = GetLobbyUserIndexByID( lobbyUserID );
	const lobbyUser_t* user = GetLobbyUser( userIndex );
	return user ? user->weaponAutoReload : true;
}

/*
========================
idLobby::GetLobbyUserLevel
========================
*/
int idLobby::GetLobbyUserLevel( lobbyUserID_t lobbyUserID ) const
{
	const int userIndex = GetLobbyUserIndexByID( lobbyUserID );
	const lobbyUser_t* user = GetLobbyUser( userIndex );
	return user ? user->level : 0;
}

/*
========================
idLobby::GetLobbyUserQoS
========================
*/
int idLobby::GetLobbyUserQoS( lobbyUserID_t lobbyUserID ) const
{
	const int userIndex = GetLobbyUserIndexByID( lobbyUserID );
	
	if( IsHost() && IsSessionUserIndexLocal( userIndex ) )
	{
		return 0;		// Local users on the host of the active session have 0 ping
	}
	
	const lobbyUser_t* user = GetLobbyUser( userIndex );
	
	if( !verify( user != NULL ) )
	{
		return 0;
	}
	
	return user->pingMs;
}

/*
========================
idLobby::GetLobbyUserTeam
========================
*/
int idLobby::GetLobbyUserTeam( lobbyUserID_t lobbyUserID ) const
{
	const int userIndex = GetLobbyUserIndexByID( lobbyUserID );
	const lobbyUser_t* user = GetLobbyUser( userIndex );
	return user ? user->teamNumber : 0;
}

/*
========================
idLobby::SetLobbyUserTeam
========================
*/
bool idLobby::SetLobbyUserTeam( lobbyUserID_t lobbyUserID, int teamNumber )
{
	const int userIndex = GetLobbyUserIndexByID( lobbyUserID );
	lobbyUser_t* user = GetLobbyUser( userIndex );
	
	if( user != NULL )
	{
		if( teamNumber != user->teamNumber )
		{
			user->teamNumber = teamNumber;
			if( IsHost() )
			{
				byte buffer[ idPacketProcessor::MAX_PACKET_SIZE - 2 ];
				idBitMsg msg( buffer, sizeof( buffer ) );
				CreateUserUpdateMessage( userIndex, msg );
				idBitMsg readMsg;
				readMsg.InitRead( buffer, msg.GetSize() );
				UpdateSessionUserOnPeers( readMsg );
			}
			return true;
		}
	}
	return false;
}

/*
========================
idLobby::GetLobbyUserPartyToken
========================
*/
int idLobby::GetLobbyUserPartyToken( lobbyUserID_t lobbyUserID ) const
{
	const int userIndex = GetLobbyUserIndexByID( lobbyUserID );
	const lobbyUser_t* user = GetLobbyUser( userIndex );
	return user ? user->partyToken : 0;
}

/*
========================
idLobby::GetProfileFromLobbyUser
========================
*/
idPlayerProfile* idLobby::GetProfileFromLobbyUser( lobbyUserID_t lobbyUserID )
{
	const int userIndex = GetLobbyUserIndexByID( lobbyUserID );
	
	idPlayerProfile* profile = NULL;
	
	idLocalUser* localUser = GetLocalUserFromLobbyUserIndex( userIndex );
	
	if( localUser != NULL )
	{
		profile = localUser->GetProfile();
	}
	
	if( profile == NULL )
	{
		// Whoops
		profile = session->GetSignInManager().GetDefaultProfile();
		//idLib::Warning( "Returning fake profile until the code is fixed to handle NULL profiles." );
	}
	
	return profile;
}

/*
========================
idLobby::GetLocalUserFromLobbyUser
========================
*/
idLocalUser* idLobby::GetLocalUserFromLobbyUser( lobbyUserID_t lobbyUserID )
{
	const int userIndex = GetLobbyUserIndexByID( lobbyUserID );
	
	return GetLocalUserFromLobbyUserIndex( userIndex );
}

/*
========================
idLobby::GetNumLobbyUsersOnTeam
========================
*/
int idLobby::GetNumLobbyUsersOnTeam( int teamNumber ) const
{
	int numTeam = 0;
	for( int i = 0; i < GetNumLobbyUsers(); ++i )
	{
		if( GetLobbyUser( i )->teamNumber == teamNumber )
		{
			++numTeam;
		}
	}
	return numTeam;
}

/*
========================
idLobby::GetPeerName
========================
*/
const char* idLobby::GetPeerName( int peerNum ) const
{

	for( int i = 0; i < GetNumLobbyUsers(); ++i )
	{
		if( !verify( GetLobbyUser( i ) != NULL ) )
		{
			continue;
		}
		
		if( GetLobbyUser( i )->peerIndex == peerNum )
		{
			return GetLobbyUserName( GetLobbyUser( i )->lobbyUserID );
		}
	}
	
	return INVALID_LOBBY_USER_NAME;
}

/*
========================
idLobby::HandleReliableMsg
========================
*/
void idLobby::HandleReliableMsg( int p, idBitMsg& msg, const lobbyAddress_t* remoteAddress /* = NULL */ )
{
	peer_t& peer = peers[p];
	
	int reliableType = msg.ReadByte();
	
	NET_VERBOSE_PRINT( " Received reliable msg: %i \n", reliableType );
	
	const lobbyType_t actingGameStateLobbyType = GetActingGameStateLobbyType();
	
	if( reliableType == RELIABLE_HELLO )
	{
		VERIFY_FROM_CONNECTING_HOST( p, lobbyType, RELIABLE_HELLO );
		// This is sent from the host acking a request to join the game lobby
		HandleHelloAck( p, msg );
		return;
	}
	else if( reliableType == RELIABLE_USER_CONNECT_REQUEST )
	{
		VERIFY_CONNECTED_PEER( p, lobbyType, RELIABLE_USER_CONNECT_REQUEST );
		
		// This message is sent from a peer requesting for a new user to join the game lobby
		// This will be sent while we are in a game lobby as a host.  otherwise, denied.
		NET_VERBOSE_PRINT( "NET: RELIABLE_USER_CONNECT_REQUEST (%s) from %s\n", GetLobbyName(), peer.address.ToString() );
		
		idSession::sessionState_t expectedState = ( lobbyType == TYPE_PARTY ) ? idSession::PARTY_LOBBY : idSession::GAME_LOBBY;
		
		if( sessionCB->GetState() == expectedState && IsHost() && NumFreeSlots() > 0 )  	// This assumes only one user in the msg
		{
			// Add user to session, which will also forward the operation to all other peers
			AddUsersFromMsg( msg, p );
		}
		else
		{
			// Let peer know user couldn't be added
			HandleUserConnectFailure( p, msg, RELIABLE_USER_CONNECT_DENIED );
		}
	}
	else if( reliableType == RELIABLE_USER_CONNECT_DENIED )
	{
		// This message is sent back from the host when a RELIABLE_PARTY_USER_CONNECT_REQUEST failed
		VERIFY_FROM_HOST( p, lobbyType, RELIABLE_PARTY_USER_CONNECT_DENIED );
		
		// Remove this user from the sign-in manager, so we don't keep trying to add them
		if( !sessionCB->GetSignInManager().RemoveLocalUserByHandle( localUserHandle_t( msg.ReadLong() ) ) )
		{
			NET_VERBOSE_PRINT( "NET: RELIABLE_PARTY_USER_CONNECT_DENIED, local user not found\n" );
			return;
		}
	}
	else if( reliableType == RELIABLE_KICK_PLAYER )
	{
		VERIFY_FROM_HOST( p, lobbyType, RELIABLE_KICK_PLAYER );
		common->Dialog().AddDialog( GDM_KICKED, DIALOG_ACCEPT, NULL, NULL, false );
		if( sessionCB->GetPartyLobby().IsHost() )
		{
			session->SetSessionOption( idSession::OPTION_LEAVE_WITH_PARTY );
		}
		session->Cancel();
	}
	else if( reliableType == RELIABLE_HEADSET_STATE )
	{
		HandleHeadsetStateChange( p, msg );
	}
	else if( reliableType == RELIABLE_USER_CONNECTED )
	{
		// This message is sent back from the host when users have connected, and we need to update our lists to reflect that
		VERIFY_FROM_HOST( p, lobbyType, RELIABLE_USER_CONNECTED );
		
		NET_VERBOSE_PRINT( "NET: RELIABLE_USER_CONNECTED (%s) from %s\n", GetLobbyName(), peer.address.ToString() );
		AddUsersFromMsg( msg, p );
	}
	else if( reliableType == RELIABLE_USER_DISCONNECTED )
	{
		// This message is sent back from the host when users have diconnected, and we need to update our lists to reflect that
		VERIFY_FROM_HOST( p, lobbyType, RELIABLE_USER_DISCONNECTED );
		
		ProcessUserDisconnectMsg( msg );
	}
	else if( reliableType == RELIABLE_MATCH_PARMS )
	{
		parms.Read( msg );
		// Update lobby with parms
		if( lobbyBackend != NULL )
		{
			lobbyBackend->UpdateMatchParms( parms );
		}
	}
	else if( reliableType == RELIABLE_START_LOADING )
	{
		// This message is sent from the host to start loading a map
		VERIFY_FROM_HOST( p, actingGameStateLobbyType, RELIABLE_START_LOADING );
		
		NET_VERBOSE_PRINT( "NET: RELIABLE_START_LOADING from %s\n", peer.address.ToString() );
		
		startLoadingFromHost = true;
	}
	else if( reliableType == RELIABLE_LOADING_DONE )
	{
		// This message is sent from the peers to state they are done loading the map
		VERIFY_CONNECTED_PEER( p, actingGameStateLobbyType, RELIABLE_LOADING_DONE );
		
		unsigned int networkChecksum = 0; // DG: use int instead of long for 64bit compatibility
		networkChecksum = msg.ReadLong();
		
		peer.networkChecksum = networkChecksum;
		peer.loaded = true;
	}
	else if( reliableType == RELIABLE_IN_GAME )
	{
		VERIFY_CONNECTED_PEER( p, actingGameStateLobbyType, RELIABLE_IN_GAME );
		
		peer.inGame = true;
	}
	else if( reliableType == RELIABLE_SNAPSHOT_ACK )
	{
		VERIFY_CONNECTED_PEER( p, actingGameStateLobbyType, RELIABLE_SNAPSHOT_ACK );
		
		// update our base state for his last received snapshot
		int snapNum = msg.ReadLong();
		float receivedBps = msg.ReadQuantizedUFloat< BANDWIDTH_REPORTING_MAX, BANDWIDTH_REPORTING_BITS >();
		
		// Update reported received bps
		if( peer.receivedBpsIndex != snapNum )
		{
			// Only do this the first time we get reported bps per snapshot. Subsequent ACKs of the same shot will usually have lower reported bps
			// due to more time elapsing but not receiving a new ss
			peer.receivedBps = receivedBps;
			peer.receivedBpsIndex = snapNum;
		}
		
		ApplySnapshotDelta( p, snapNum );
		
		//idLib::Printf( "NET: Peer %d Ack'd snapshot %d\n", p, snapNum );
		NET_VERBOSESNAPSHOT_PRINT_LEVEL( 2, va( "NET: Peer %d Ack'd snapshot %d\n", p, snapNum ) );
		
	}
	else if( reliableType == RELIABLE_RESOURCE_ACK )
	{
	}
	else if( reliableType == RELIABLE_UPDATE_MATCH_PARMS )
	{
		VERIFY_CONNECTED_PEER( p, TYPE_GAME, RELIABLE_UPDATE_MATCH_PARMS );
		int msgType = msg.ReadLong();
		sessionCB->HandlePeerMatchParamUpdate( p, msgType );
		
	}
	else if( reliableType == RELIABLE_MATCHFINISHED )
	{
		VERIFY_FROM_HOST( p, actingGameStateLobbyType, RELIABLE_MATCHFINISHED );
		
		sessionCB->ClearMigrationState();
		
	}
	else if( reliableType == RELIABLE_ENDMATCH )
	{
		VERIFY_FROM_HOST( p, actingGameStateLobbyType, RELIABLE_ENDMATCH );
		
		sessionCB->EndMatchInternal();
		
	}
	else if( reliableType == RELIABLE_ENDMATCH_PREMATURE )
	{
		VERIFY_FROM_HOST( p, actingGameStateLobbyType, RELIABLE_ENDMATCH_PREMATURE );
		
		sessionCB->EndMatchInternal( true );
		
	}
	else if( reliableType == RELIABLE_START_MATCH_GAME_LOBBY_HOST )
	{
		// This message should be from the host of the game lobby, telling us (as the host of the GameStateLobby) to start loading
		VERIFY_CONNECTED_PEER( p, TYPE_GAME_STATE, RELIABLE_START_MATCH_GAME_LOBBY_HOST );
		
		if( session->GetState() >= idSession::LOADING )
		{
			NET_VERBOSE_PRINT( "NET: RELIABLE_START_MATCH_GAME_LOBBY_HOST already loading\n" );
			return;
		}
		
		// Read match parms, and start loading
		parms.Read( msg );
		
		// Send these new match parms to currently connected peers
		SendMatchParmsToPeers();
		
		startLoadingFromHost = true;		// Hijack this flag
	}
	else if( reliableType == RELIABLE_ARBITRATE )
	{
		VERIFY_CONNECTED_PEER( p, TYPE_GAME, RELIABLE_ARBITRATE );
		// Host telling us to arbitrate
		// Set a flag to do this later, since the lobby may not be in a state where it can fulfil the request at the moment
		respondToArbitrate = true;
	}
	else if( reliableType == RELIABLE_ARBITRATE_OK )
	{
		VERIFY_CONNECTED_PEER( p, TYPE_GAME, RELIABLE_ARBITRATE_OK );
		
		NET_VERBOSE_PRINT( "NET: Got an arbitration ok from %d\n", p );
		
		everyoneArbitrated = true;
		for( int i = 0; i < GetNumLobbyUsers(); i++ )
		{
			lobbyUser_t* user = GetLobbyUser( i );
			if( !verify( user != NULL ) )
			{
				continue;
			}
			if( user->peerIndex == p )
			{
				user->arbitrationAcked = true;
			}
			else if( !user->arbitrationAcked )
			{
				everyoneArbitrated = false;
			}
		}
		
		if( everyoneArbitrated )
		{
			NET_VERBOSE_PRINT( "NET: Everyone says they registered for arbitration, verifying\n" );
			lobbyBackend->Arbitrate();
			//sessionCB->EveryoneArbitrated();
			return;
		}
	}
	else if( reliableType == RELIABLE_POST_STATS )
	{
		VERIFY_FROM_HOST( p, actingGameStateLobbyType, RELIABLE_POST_STATS );
		sessionCB->RecvLeaderboardStats( msg );
	}
	else if( reliableType == RELIABLE_SESSION_USER_MODIFIED )
	{
		VERIFY_CONNECTED_PEER( p, lobbyType, RELIABLE_SESSION_USER_MODIFIED );
		UpdateSessionUserOnPeers( msg );
		
	}
	else if( reliableType == RELIABLE_UPDATE_SESSION_USER )
	{
		VERIFY_FROM_HOST( p, lobbyType, RELIABLE_UPDATE_SESSION_USER );
		HandleUpdateSessionUser( msg );
	}
	else if( reliableType == RELIABLE_CONNECT_AND_MOVE_TO_LOBBY )
	{
		VERIFY_FROM_HOST( p, lobbyType, RELIABLE_CONNECT_AND_MOVE_TO_LOBBY );
		
		NET_VERBOSE_PRINT( "NET: RELIABLE_CONNECT_AND_MOVE_TO_LOBBY\n" );
		
		if( IsHost() )
		{
			idLib::Printf( "RELIABLE_CONNECT_AND_MOVE_TO_LOBBY: We are the host.\n" );
			return;
		}
		
		// Get connection info
		lobbyConnectInfo_t connectInfo;
		
		connectInfo.ReadFromMsg( msg );
		
		// DG: if connectInfo.ip = 0.0.0.0 just use remoteAddress
		//     i.e. the IP used to connect to the lobby
		if( remoteAddress && *( ( int* )connectInfo.netAddr.ip ) == 0 )
		{
			connectInfo.netAddr = remoteAddress->netAddr;
		}
		// DG end
		
		const lobbyType_t	destLobbyType	= ( lobbyType_t )msg.ReadByte();
		const bool			waitForMembers	= msg.ReadBool();
		
		assert( destLobbyType > lobbyType );		// Make sure this is a proper transition (i.e. TYPE_PARTY moves to TYPE_GAME, TYPE_GAME moves to TYPE_GAME_STATE)
		
		sessionCB->ConnectAndMoveToLobby( destLobbyType, connectInfo, waitForMembers );
	}
	else if( reliableType == RELIABLE_PARTY_CONNECT_OK )
	{
		VERIFY_FROM_HOST( p, TYPE_PARTY, RELIABLE_PARTY_CONNECT_OK );
		if( !sessionCB->GetGameLobby().waitForPartyOk )
		{
			idLib::Printf( "RELIABLE_PARTY_CONNECT_OK: Wasn't waiting for ok.\n" );
		}
		sessionCB->GetGameLobby().waitForPartyOk = false;
	}
	else if( reliableType == RELIABLE_PARTY_LEAVE_GAME_LOBBY )
	{
		VERIFY_FROM_HOST( p, TYPE_PARTY, RELIABLE_PARTY_LEAVE_GAME_LOBBY );
		
		NET_VERBOSE_PRINT( "NET: RELIABLE_PARTY_LEAVE_GAME_LOBBY\n" );
		
		if( sessionCB->GetState() != idSession::GAME_LOBBY )
		{
			idLib::Printf( "RELIABLE_PARTY_LEAVE_GAME_LOBBY: Not in a game lobby, ignoring.\n" );
			return;
		}
		
		if( IsHost() )
		{
			idLib::Printf( "RELIABLE_PARTY_LEAVE_GAME_LOBBY: Host of party, ignoring.\n" );
			return;
		}
		
		sessionCB->LeaveGameLobby();
	}
	else if( IsReliablePlayerToPlayerType( reliableType ) )
	{
		HandleReliablePlayerToPlayerMsg( p, msg, reliableType );
	}
	else if( reliableType == RELIABLE_PING )
	{
		HandleReliablePing( p, msg );
	}
	else if( reliableType == RELIABLE_PING_VALUES )
	{
		HandlePingValues( msg );
	}
	else if( reliableType == RELIABLE_BANDWIDTH_VALUES )
	{
		HandleBandwidhTestValue( p, msg );
	}
	else if( reliableType == RELIABLE_MIGRATION_GAME_DATA )
	{
		HandleMigrationGameData( msg );
	}
	else if( reliableType >= RELIABLE_GAME_DATA )
	{
	
		VERIFY_CONNECTED_PEER( p, lobbyType, RELIABLE_GAME_DATA );
		
		common->NetReceiveReliable( p, reliableType - RELIABLE_GAME_DATA, msg );
	}
	else if( reliableType == RELIABLE_DUMMY_MSG )
	{
		// Ignore dummy msg's
		NET_VERBOSE_PRINT( "NET: ignoring dummy msg from %s\n", peer.address.ToString() );
	}
	else
	{
		NET_VERBOSE_PRINT( "NET: Unknown reliable packet type %d from %s\n", reliableType, peer.address.ToString() );
	}
}

/*
========================
idLobby::GetTotalOutgoingRate
========================
*/
int idLobby::GetTotalOutgoingRate()
{
	int totalSendRate = 0;
	for( int p = 0; p < peers.Num(); p++ )
	{
		const peer_t& peer = peers[p];
		
		if( !peer.IsConnected() )
		{
			continue;
		}
		
		const idPacketProcessor& proc = *peer.packetProc;
		
		totalSendRate += proc.GetOutgoingRateBytes();
	}
	return totalSendRate;
}

/*
========================
idLobby::DrawDebugNetworkHUD
========================
*/
extern idCVar net_forceUpstream;
void idLobby::DrawDebugNetworkHUD() const
{
	int		totalSendRate = 0;
	int		totalRecvRate = 0;
	float	totalSentMB = 0.0f;
	float	totalRecvMB = 0.0f;
	
	const float Y_OFFSET	= 20.0f;
	const float X_OFFSET	= 20.0f;
	const float Y_SPACING	= 15.0f;
	
	float curY = Y_OFFSET;
	
	int numLines = ( net_forceUpstream.GetFloat() != 0.0f ? 6 : 5 );
	
	renderSystem->DrawFilled( idVec4( 0.0f, 0.0f, 0.0f, 0.7f ), X_OFFSET - 10.0f, curY - 10.0f, 1550, ( peers.Num() + numLines ) * Y_SPACING + 20.0f );
	
	renderSystem->DrawSmallStringExt( idMath::Ftoi( X_OFFSET ), idMath::Ftoi( curY ), "# Peer                   | Sent kB/s | Recv kB/s | Sent MB | Recv MB | Ping   | L |  %  | R.NM | R.SZ | R.AK | T", colorGreen, false );
	curY += Y_SPACING;
	
	renderSystem->DrawSmallStringExt( idMath::Ftoi( X_OFFSET ), idMath::Ftoi( curY ), "------------------------------------------------------------------------------------------------------------------------------------", colorGreen, false );
	curY += Y_SPACING;
	
	for( int p = 0; p < peers.Num(); p++ )
	{
		const peer_t& peer = peers[p];
		
		if( !peer.IsConnected() )
		{
			continue;
		}
		
		const idPacketProcessor& proc = *peer.packetProc;
		
		totalSendRate += proc.GetOutgoingRateBytes();
		totalRecvRate += proc.GetIncomingRateBytes();
		float sentKps = ( float )proc.GetOutgoingRateBytes() / 1024.0f;
		float recvKps = ( float )proc.GetIncomingRateBytes() / 1024.0f;
		float sentMB = ( float )proc.GetOutgoingBytes() / ( 1024.0f * 1024.0f );
		float recvMB = ( float )proc.GetIncomingBytes() / ( 1024.0f * 1024.0f );
		
		totalSentMB += sentMB;
		totalRecvMB += recvMB;
		
		idVec4 color = sentKps > 20.0f ? colorRed : colorGreen;
		
		int resourcePercent = 0;
		idStr name = peer.address.ToString();
		
		name += lobbyType == TYPE_PARTY ? "(P" : "(G";
		name += host == p ? ":H)" : ":C)";
		
		renderSystem->DrawSmallStringExt( X_OFFSET, curY, va( "%i %22s | %2.02f kB/s | %2.02f kB/s | %2.02f MB | %2.02f MB |%4i ms | %i | %i%% | %i | %i | %i | %2.2f / %2.2f / %i", p, name.c_str(), sentKps, recvKps, sentMB, recvMB, peer.lastPingRtt, peer.loaded, resourcePercent, peer.packetProc->NumQueuedReliables(), peer.packetProc->GetReliableDataSize(), peer.packetProc->NeedToSendReliableAck(), peer.snapHz, peer.maxSnapBps, peer.failedPingRecoveries ), color, false );
		curY += Y_SPACING;
	}
	
	renderSystem->DrawSmallStringExt( idMath::Ftoi( X_OFFSET ), idMath::Ftoi( curY ), "------------------------------------------------------------------------------------------------------------------------------------", colorGreen, false );
	curY += Y_SPACING;
	
	float totalSentKps = ( float )totalSendRate / 1024.0f;
	float totalRecvKps = ( float )totalRecvRate / 1024.0f;
	
	idVec4 color = totalSentKps > 100.0f ? colorRed : colorGreen;
	
	renderSystem->DrawSmallStringExt( X_OFFSET, curY, va( "# %20s | %2.02f KB/s | %2.02f KB/s | %2.02f MB | %2.02f MB", "", totalSentKps, totalRecvKps, totalSentMB, totalRecvMB ), color, false );
	curY += Y_SPACING;
	
	if( net_forceUpstream.GetFloat() != 0.0f )
	{
		float upstreamDropRate = session->GetUpstreamDropRate();
		float upstreamQueuedRate = session->GetUpstreamQueueRate();
		
		int queuedBytes = session->GetQueuedBytes();
		renderSystem->DrawSmallStringExt( X_OFFSET, curY, va( "Queued: %d | Dropping: %2.02f kB/s Queue: %2.02f kB/s -> Effective %2.02f kB/s", queuedBytes, upstreamDropRate / 1024.0f, upstreamQueuedRate / 1024.0f, totalSentKps - ( upstreamDropRate / 1024.0f ) + ( upstreamQueuedRate / 1024.0f ) ), color, false );
	}
}

/*
========================
idLobby::DrawDebugNetworkHUD2
========================
*/
void idLobby::DrawDebugNetworkHUD2() const
{
	int		totalSendRate = 0;
	int		totalRecvRate = 0;
	
	const float Y_OFFSET	= 20.0f;
	const float X_OFFSET	= 20.0f;
	const float Y_SPACING	= 15.0f;
	
	float	curY = Y_OFFSET;
	
	renderSystem->DrawFilled( idVec4( 0.0f, 0.0f, 0.0f, 0.7f ), X_OFFSET - 10.0f, curY - 10.0f, 550, ( peers.Num() + 4 ) * Y_SPACING + 20.0f );
	
	const char* stateName = session->GetStateString();
	
	renderSystem->DrawFilled( idVec4( 1.0f, 1.0f, 1.0f, 0.7f ), X_OFFSET - 10.0f, curY - 10.0f, 550, ( peers.Num() + 5 ) * Y_SPACING + 20.0f );
	
	renderSystem->DrawSmallStringExt( idMath::Ftoi( X_OFFSET ), idMath::Ftoi( curY ), va( "State: %s. Local time: %d", stateName, Sys_Milliseconds() ), colorGreen, false );
	curY += Y_SPACING;
	
	renderSystem->DrawSmallStringExt( idMath::Ftoi( X_OFFSET ), idMath::Ftoi( curY ), "Peer           | Sent kB/s | Recv kB/s | L | R | Resources", colorGreen, false );
	curY += Y_SPACING;
	
	renderSystem->DrawSmallStringExt( idMath::Ftoi( X_OFFSET ), idMath::Ftoi( curY ), "------------------------------------------------------------------", colorGreen, false );
	curY += Y_SPACING;
	
	for( int p = 0; p < peers.Num(); p++ )
	{
	
		if( !peers[ p ].IsConnected() )
		{
			continue;
		}
		
		idPacketProcessor& proc = *peers[ p ].packetProc;
		
		totalSendRate += proc.GetOutgoingRate2();
		totalRecvRate += proc.GetIncomingRate2();
		float sentKps = ( float )proc.GetOutgoingRate2() / 1024.0f;
		float recvKps = ( float )proc.GetIncomingRate2() / 1024.0f;
		
		// should probably complement that with a bandwidth reading
		// right now I am mostly concerned about fragmentation and the latency spikes it will cause
		idVec4 color = proc.TickFragmentAccumulator() ? colorRed : colorGreen;
		
		int rLoaded = peers[ p ].numResources;
		int rTotal = 0;
		
		// show the names of the clients connected to the server. Also make sure it looks reasonably good.
		idStr peerName;
		if( IsHost() )
		{
			peerName = GetPeerName( p );
			
			int MAX_PEERNAME_LENGTH = 10;
			int nameLength = peerName.Length();
			if( nameLength > MAX_PEERNAME_LENGTH )
			{
				peerName = peerName.Left( MAX_PEERNAME_LENGTH );
			}
			else if( nameLength < MAX_PEERNAME_LENGTH )
			{
				idStr filler;
				filler.Fill( ' ', MAX_PEERNAME_LENGTH );
				peerName += filler.Left( MAX_PEERNAME_LENGTH - nameLength );
			}
		}
		else
		{
			peerName = "Local     ";
		}
		
		renderSystem->DrawSmallStringExt( X_OFFSET, curY, va( "%i - %s | %2.02f kB/s | %2.02f kB/s | %i | %i | %d/%d", p, peerName.c_str(), sentKps, recvKps, peers[p].loaded, peers[p].address.UsingRelay(), rLoaded, rTotal ), color, false );
		curY += Y_SPACING;
	}
	
	renderSystem->DrawSmallStringExt( idMath::Ftoi( X_OFFSET ), idMath::Ftoi( curY ), "------------------------------------------------------------------", colorGreen, false );
	curY += Y_SPACING;
	
	float totalSentKps = ( float )totalSendRate / 1024.0f;
	float totalRecvKps = ( float )totalRecvRate / 1024.0f;
	
	renderSystem->DrawSmallStringExt( X_OFFSET, curY, va( "Total | %2.02f KB/s | %2.02f KB/s", totalSentKps, totalRecvKps ), colorGreen, false );
}


/*
========================
idLobby::DrawDebugNetworkHUD_ServerSnapshotMetrics
========================
*/
idCVar net_debughud3_bps_max( "net_debughud3_bps_max", "5120.0f", CVAR_FLOAT, "Highest factor of server base snapRate that a client can be throttled" );
void idLobby::DrawDebugNetworkHUD_ServerSnapshotMetrics( bool draw )
{
	const float Y_OFFSET	= 20.0f;
	const float X_OFFSET	= 20.0f;
	const float Y_SPACING	= 15.0f;
	idVec4 color = colorWhite;
	
	float	curY = Y_OFFSET;
	
	if( !draw )
	{
		for( int p = 0; p < peers.Num(); p++ )
		{
			for( int i = 0; i < peers[p].debugGraphs.Num(); i++ )
			{
				if( peers[p].debugGraphs[i] != NULL )
				{
					peers[p].debugGraphs[i]->Enable( false );
				}
				else
				{
					return;
				}
			}
		}
		return;
	}
	
	static int lastTime = 0;
	int time = Sys_Milliseconds();
	
	for( int p = 0; p < peers.Num(); p++ )
	{
	
		peer_t& peer = peers[p];
		
		if( !peer.IsConnected() )
		{
			continue;
		}
		
		idPacketProcessor* packetProc = peer.packetProc;
		idSnapshotProcessor* snapProc = peer.snapProc;
		
		if( !verify( packetProc != NULL && snapProc != NULL ) )
		{
			continue;
		}
		
		int snapSeq = snapProc->GetSnapSequence();
		int snapBase = snapProc->GetBaseSequence();
		int deltaSeq = snapSeq - snapBase;
		bool throttled = peer.throttledSnapRate > common->GetSnapRate();
		
		int numLines =  net_forceUpstream.GetBool() ? 5 : 4;
		
		const int width = renderSystem->GetWidth() / 2.0f - ( X_OFFSET * 2 );
		
		enum netDebugGraphs_t
		{
			GRAPH_SNAPSENT,
			GRAPH_OUTGOING,
			GRAPH_INCOMINGREPORTED,
			GRAPH_MAX
		};
		
		peer.debugGraphs.SetNum( GRAPH_MAX, NULL );
		for( int i = 0; i < GRAPH_MAX; i++ )
		{
			// Initialize graphs
			if( peer.debugGraphs[i] == NULL )
			{
				peer.debugGraphs[i] = console->CreateGraph( 500 );
				if( !verify( peer.debugGraphs[i] != NULL ) )
				{
					continue;
				}
				
				peer.debugGraphs[i]->SetPosition( X_OFFSET - 10.0f + width, curY - 10.0f, width , Y_SPACING * numLines );
			}
			
			peer.debugGraphs[i]->Enable( true );
		}
		
		renderSystem->DrawFilled( idVec4( 0.0f, 0.0f, 0.0f, 0.7f ), X_OFFSET - 10.0f, curY - 10.0f, width, ( Y_SPACING * numLines ) + 20.0f );
		
		renderSystem->DrawSmallStringExt( X_OFFSET, curY, va( "Peer %d - %s RTT %d %sPeerSnapRate: %d %s", p, GetPeerName( p ), peer.lastPingRtt, throttled ? "^1" : "^2", peer.throttledSnapRate / 1000, throttled ? "^1Throttled" : "" ), color, false );
		curY += Y_SPACING;
		
		renderSystem->DrawSmallStringExt( X_OFFSET, curY, va( "SnapSeq %d  BaseSeq %d  Delta %d  Queue %d", snapSeq, snapBase, deltaSeq, snapProc->GetSnapQueueSize() ), color, false );
		curY += Y_SPACING;
		
		renderSystem->DrawSmallStringExt( X_OFFSET, curY, va( "Reliables: %d / %d bytes Reliable Ack: %d", packetProc->NumQueuedReliables(), packetProc->GetReliableDataSize(), packetProc->NeedToSendReliableAck() ), color, false );
		curY += Y_SPACING;
		
		renderSystem->DrawSmallStringExt( X_OFFSET, curY, va( "Outgoing %.2f kB/s  Reported %.2f kB/s Throttle: %.2f", peer.packetProc->GetOutgoingRateBytes() / 1024.0f, peers[p].receivedBps / 1024.0f, peer.receivedThrottle ), color, false );
		curY += Y_SPACING;
		
		if( net_forceUpstream.GetFloat() != 0.0f )
		{
			float upstreamDropRate = session->GetUpstreamDropRate();
			float upstreamQueuedRate = session->GetUpstreamQueueRate();
			int queuedBytes = session->GetQueuedBytes();
			renderSystem->DrawSmallStringExt( X_OFFSET, curY, va( "Queued: %d | Dropping: %2.02f kB/s Queue: %2.02f kB/s ", queuedBytes, upstreamDropRate / 1024.0f, upstreamQueuedRate / 1024.0f ), color, false );
			
		}
		
		curY += Y_SPACING;
		
		
		
		if( peer.debugGraphs[GRAPH_SNAPSENT] != NULL )
		{
			if( peer.lastSnapTime > lastTime )
			{
				peer.debugGraphs[GRAPH_SNAPSENT]->SetValue( -1, 1.0f, colorBlue );
			}
			else
			{
				peer.debugGraphs[GRAPH_SNAPSENT]->SetValue( -1, 0.0f, colorBlue );
			}
		}
		
		if( peer.debugGraphs[GRAPH_OUTGOING] != NULL )
		{
			idVec4 bgColor( vec4_zero );
			peer.debugGraphs[GRAPH_OUTGOING]->SetBackgroundColor( bgColor );
			
			idVec4 lineColor = colorLtGrey;
			lineColor.w	 = 0.5f;
			float outgoingRate = peer.sentBpsHistory[ peer.receivedBpsIndex % MAX_BPS_HISTORY ];
			// peer.packetProc->GetOutgoingRateBytes()
			peer.debugGraphs[GRAPH_OUTGOING]->SetValue( -1, idMath::ClampFloat( 0.0f, 1.0f, outgoingRate / net_debughud3_bps_max.GetFloat() ), lineColor );
		}
		
		
		if( peer.debugGraphs[GRAPH_INCOMINGREPORTED] != NULL )
		{
			idVec4 lineColor = colorYellow;
			extern idCVar net_peer_throttle_bps_peer_threshold_pct;
			extern idCVar net_peer_throttle_bps_host_threshold;
			
			if( peer.packetProc->GetOutgoingRateBytes() > net_peer_throttle_bps_host_threshold.GetFloat() )
			{
				float pct = peer.packetProc->GetOutgoingRateBytes() > 0.0f ? peer.receivedBps / peer.packetProc->GetOutgoingRateBytes() : 0.0f;
				if( pct < net_peer_throttle_bps_peer_threshold_pct.GetFloat() )
				{
					lineColor = colorRed;
				}
				else
				{
					lineColor = colorGreen;
				}
			}
			idVec4 bgColor( vec4_zero );
			peer.debugGraphs[GRAPH_INCOMINGREPORTED]->SetBackgroundColor( bgColor );
			peer.debugGraphs[GRAPH_INCOMINGREPORTED]->SetFillMode( idDebugGraph::GRAPH_LINE );
			peer.debugGraphs[GRAPH_INCOMINGREPORTED]->SetValue( -1, idMath::ClampFloat( 0.0f, 1.0f, peer.receivedBps / net_debughud3_bps_max.GetFloat() ), lineColor );
		}
		
		
		
		// Skip down
		curY += ( Y_SPACING * 2.0f );
	}
	
	lastTime = time;
}

/*
========================
idLobby::CheckHeartBeats
========================
*/
void idLobby::CheckHeartBeats()
{
	// Disconnect peers that haven't responded within net_peerTimeoutInSeconds
	int time = Sys_Milliseconds();
	
	int timeoutInMs = session->GetTitleStorageInt( "net_peerTimeoutInSeconds", net_peerTimeoutInSeconds.GetInteger() ) * 1000;
	
	if( sessionCB->GetState() < idSession::LOADING && migrationInfo.state == MIGRATE_NONE )
	{
		// Use shorter timeout in lobby (TCR)
		timeoutInMs = session->GetTitleStorageInt( "net_peerTimeoutInSeconds_Lobby", net_peerTimeoutInSeconds_Lobby.GetInteger() ) * 1000;
	}
	
	if( timeoutInMs > 0 )
	{
		for( int p = 0; p < peers.Num(); p++ )
		{
			if( peers[p].IsConnected() )
			{
			
				bool peerTimeout = false;
				
				if( time - peers[p].lastHeartBeat > timeoutInMs )
				{
					peerTimeout = true;
				}
				
				// if reliable queue is almost full, disconnect the peer.
				// (this seems reasonable since the reliable queue is set to 64 currently. In practice we should never
				// have more than 3 or 4 queued)
				if( peers[ p ].packetProc->NumQueuedReliables() > idPacketProcessor::MAX_RELIABLE_QUEUE - 1 )
				{
					peerTimeout = true;
				}
				
				if( peerTimeout )
				{
					// Disconnect the peer from any sessions we are a host of
					if( IsHost() )
					{
						idLib::Printf( "Peer %i timed out for %s session @ %d (lastHeartBeat %d)\n", p, GetLobbyName(), time, peers[p].lastHeartBeat );
						DisconnectPeerFromSession( p );
					}
					
					// Handle peers not receiving a heartbeat from the host in awhile
					if( IsPeer() )
					{
						if( migrationInfo.state != MIGRATE_PICKING_HOST )
						{
							idLib::Printf( "Host timed out for %s session\n", GetLobbyName() );
							
							// Pick a host for this session
							PickNewHost();
						}
					}
				}
			}
		}
	}
	
	if( IsHost() && lobbyType == GetActingGameStateLobbyType() )
	{
		for( int p = 0; p < peers.Num(); p++ )
		{
			if( !peers[p].IsConnected() )
			{
				continue;
			}
			
			CheckPeerThrottle( p );
		}
	}
}

/*
========================
idLobby::CheckHeartBeats
========================
*/
bool idLobby::IsLosingConnectionToHost() const
{
	if( !verify( IsPeer() && host >= 0 && host < peers.Num() ) )
	{
		return false;
	}
	
	if( !peers[ host ].IsConnected() )
	{
		return true;
	}
	
	int time = Sys_Milliseconds();
	
	int timeoutInMs = session->GetTitleStorageInt( "net_peerTimeoutInSeconds", net_peerTimeoutInSeconds.GetInteger() ) * 1000;
	
	// return true if heartbeat > half the timeout length
	if( timeoutInMs > 0 &&  time - peers[ host ].lastHeartBeat > timeoutInMs / 2 )
	{
		return true;
	}
	
	// return true if reliable queue is more than half full
	// (this seems reasonable since the reliable queue is set to 64 currently. In practice we should never
	// have more than 3 or 4 queued)
	if( peers[ host ].packetProc->NumQueuedReliables() > idPacketProcessor::MAX_RELIABLE_QUEUE / 2 )
	{
		return true;
	}
	
	return false;
}

/*
========================
idLobby::IsMigratedStatsGame
========================
*/
bool idLobby::IsMigratedStatsGame() const
{
	if( !IsLobbyActive() )
	{
		return false;
	}
	
	if( lobbyType != TYPE_GAME )
	{
		return false;		// Only game session migrates games stats
	}
	
	if( !MatchTypeHasStats( parms.matchFlags ) )
	{
		return false;		// Only stats games migrate stats
	}
	
	if( !MatchTypeIsRanked( parms.matchFlags ) )
	{
		return false;		// Only ranked games should migrate stats into new game
	}
	
	return migrationInfo.persistUntilGameEndsData.wasMigratedGame && migrationInfo.persistUntilGameEndsData.hasGameData;
}

/*
========================
idLobby::ShouldRelaunchMigrationGame
returns true if we are hosting a migrated game and we had valid migration data
========================
*/
bool idLobby::ShouldRelaunchMigrationGame() const
{
	if( IsMigrating() )
	{
		return false;		// Don't relaunch until all clients have reconnected
	}
	
	if( !IsMigratedStatsGame() )
	{
		return false;		// If we are not migrating stats, we don't want to relaunch a new game
	}
	
	if( !migrationInfo.persistUntilGameEndsData.wasMigratedHost )
	{
		return false;		// Only relaunch if we are the host
	}
	
	if( migrationInfo.persistUntilGameEndsData.hasRelaunchedMigratedGame )
	{
		return false;		// We already relaunched this game
	}
	
	return true;
}

/*
========================
idLobby::ShouldShowMigratingDialog
========================
*/
bool idLobby::ShouldShowMigratingDialog() const
{
	if( IsMigrating() )
	{
		return true;	// If we are in the process of truly migrating, then definitely return true
	}
	
	if( sessionCB->GetState() == idSession::INGAME )
	{
		return false;
	}
	
	// We're either waiting on the server (which could be us) to relaunch, so show the dialog
	return IsMigratedStatsGame() && sessionCB->GetState() != idSession::INGAME;
}

/*
========================
idLobby::IsMigrating
========================
*/
bool idLobby::IsMigrating() const
{
	return migrationInfo.state != idLobby::MIGRATE_NONE;
}

/*
========================
idLobby::PingPeers
Host only.
========================
*/
void idLobby::PingPeers()
{
	if( !verify( IsHost() ) )
	{
		return;
	}
	
	const int now = Sys_Milliseconds();
	
	pktPing_t packet;
	memset( &packet, 0, sizeof( packet ) ); // We're gonna memset like it's 1999.
	packet.timestamp = now;
	
	byte packetCopy[ sizeof( packet ) ];
	idBitMsg msg( packetCopy, sizeof( packetCopy ) );
	msg.WriteLong( packet.timestamp );
	
	for( int i = 0; i < peers.Num(); ++i )
	{
		peer_t& peer = peers[ i ];
		if( !peer.IsConnected() )
		{
			continue;
		}
		if( peer.nextPing <= now )
		{
			peer.nextPing = now + PING_INTERVAL_MS;
			QueueReliableMessage( i, RELIABLE_PING, msg.GetReadData(), msg.GetSize() );
		}
	}
}

/*
========================
idLobby::ThrottlePeerSnapRate
========================
*/
void idLobby::ThrottlePeerSnapRate( int p )
{
	if( !verify( IsHost() ) || !verify( p >= 0 ) )
	{
		return;
	}
	
	peers[p].throttledSnapRate = common->GetSnapRate() * 2;
	idLib::Printf( "^1Throttling peer %d %s!\n", p, GetPeerName( p ) );
	idLib::Printf( "  New snaprate: %d\n", peers[p].throttledSnapRate / 1000 );
}

/*
========================
idLobby::SaturatePeers
========================
*/
void idLobby::BeginBandwidthTest()
{
	if( !verify( IsHost() ) )
	{
		idLib::Warning( "Bandwidth test should only be done on host" );
		return;
	}
	
	if( bandwidthChallengeStartTime > 0 )
	{
		idLib::Warning( "Already started bandwidth test" );
		return;
	}
	
	int time = Sys_Milliseconds();
	bandwidthChallengeStartTime = time;
	bandwidthChallengeEndTime = 0;
	bandwidthChallengeFinished = false;
	bandwidthChallengeNumGoodSeq = 0;
	
	for( int p = 0; p < peers.Num(); ++p )
	{
		if( !peers[ p ].IsConnected() )
		{
			continue;
		}
		
		if( !verify( peers[ p ].packetProc != NULL ) )
		{
			continue;
		}
		
		peers[ p ].bandwidthSequenceNum = 0;
		peers[ p ].bandwidthChallengeStartSendTime = 0;
		peers[ p ].bandwidthChallengeResults = false;
		peers[ p ].bandwidthChallengeSendComplete	 = false;
		peers[ p ].bandwidthTestBytes = peers[ p ].packetProc->GetOutgoingBytes(); // cache this off so we can see the difference when we are done
	}
}

/*
========================
idLobby::SaturatePeers
========================
*/
bool idLobby::BandwidthTestStarted()
{
	return bandwidthChallengeStartTime != 0;
}
/*
========================
idLobby::ServerUpdateBandwidthTest
========================
*/
void idLobby::ServerUpdateBandwidthTest()
{
	if( bandwidthChallengeStartTime <= 0 )
	{
		// Not doing a test
		return;
	}
	
	if( !verify( IsHost() ) )
	{
		return;
	}
	
	int time = Sys_Milliseconds();
	
	if( bandwidthChallengeFinished )
	{
		// test is over
		return;
	}
	
	idRandom random;
	random.SetSeed( time );
	
	bool sentAll = true;
	bool recAll = true;
	
	for( int i = 0; i < peers.Num(); ++i )
	{
		peer_t& peer = peers[ i ];
		if( !peer.IsConnected() )
		{
			continue;
		}
		
		if( peer.bandwidthChallengeResults )
		{
			continue;
		}
		recAll = false;
		
		if( peer.bandwidthChallengeSendComplete )
		{
			continue;
		}
		sentAll = false;
		
		if( time - peer.bandwidthTestLastSendTime < session->GetTitleStorageInt( "net_bw_test_interval", net_bw_test_interval.GetInteger() ) )
		{
			continue;
		}
		
		if( peer.packetProc->HasMoreFragments() )
		{
			continue;
		}
		
		if( peer.bandwidthChallengeStartSendTime == 0 )
		{
			peer.bandwidthChallengeStartSendTime = time;
		}
		
		peer.bandwidthTestLastSendTime = time;
		
		// Ok, send him a big packet
		byte buffer[ idPacketProcessor::MAX_OOB_MSG_SIZE ];		// <---- NOTE - When calling ProcessOutgoingMsg with true for oob, we can't go over this size
		idBitMsg msg( buffer, sizeof( buffer ) );
		
		msg.WriteLong( peer.bandwidthSequenceNum++ );
		
		unsigned int randomSize = Min( ( unsigned int )( sizeof( buffer ) - 12 ), ( unsigned int )session->GetTitleStorageInt( "net_bw_test_packetSizeBytes", net_bw_test_packetSizeBytes.GetInteger() ) );
		msg.WriteLong( randomSize );
		
		for( unsigned int j = 0; j < randomSize; j++ )
		{
			msg.WriteByte( random.RandomInt( 255 ) );
		}
		
		unsigned int checksum = MD5_BlockChecksum( &buffer[8], randomSize );
		msg.WriteLong( checksum );
		
		NET_VERBOSE_PRINT( "Net: Sending bw challenge to peer %d time %d packet size %d\n", i, time, msg.GetSize() );
		
		ProcessOutgoingMsg( i, buffer, msg.GetSize(), true, OOB_BANDWIDTH_TEST );
		
		if( session->GetTitleStorageInt( "net_bw_test_numPackets", net_bw_test_numPackets.GetInteger() ) > 0 && peer.bandwidthSequenceNum >= net_bw_test_numPackets.GetInteger() )
		{
			int sentBytes = peers[i].packetProc->GetOutgoingBytes() - peers[i].bandwidthTestBytes; // FIXME: this won't include the last sent msg
			peers[i].bandwidthTestBytes = sentBytes; // this now means total bytes sent (we don't care about starting/ending total bytes sent to peer)
			peers[i].bandwidthChallengeSendComplete = true;
			
			NET_VERBOSE_PRINT( "Sent enough packets to peer %d for bandwidth test in %dms. Total bytes: %d\n", i, time - bandwidthChallengeStartTime, sentBytes );
		}
	}
	
	if( sentAll )
	{
		if( bandwidthChallengeEndTime == 0 )
		{
			// We finished sending all our packets, set the timeout time
			bandwidthChallengeEndTime = time + session->GetTitleStorageInt( "net_bw_test_host_timeout", net_bw_test_host_timeout.GetInteger() );
			NET_VERBOSE_PRINT( "Net: finished sending BWC to peers. Waiting until %d to hear back\n", bandwidthChallengeEndTime );
		}
	}
	
	if( recAll )
	{
		bandwidthChallengeFinished = true;
		bandwidthChallengeStartTime = 0;
		
	}
	else if( bandwidthChallengeEndTime != 0 && bandwidthChallengeEndTime < time )
	{
		// Timed out waiting for someone - throttle them and move on
		NET_VERBOSE_PRINT( "^2Net: timed out waiting for bandwidth challenge results \n" );
		for( int i = 0; i < peers.Num(); i++ )
		{
			NET_VERBOSE_PRINT( "  Peer[%d] %s. SentAll: %d  RecAll: %d\n", i, GetPeerName( i ), peers[i].bandwidthChallengeSendComplete, peers[i].bandwidthChallengeResults );
			if( peers[i].bandwidthChallengeSendComplete && !peers[i].bandwidthChallengeResults )
			{
				ThrottlePeerSnapRate( i );
			}
		}
		bandwidthChallengeFinished = true;
		bandwidthChallengeStartTime = 0;
	}
}

/*
========================
idLobby::UpdateBandwidthTest
This will be called on clients to check current state of bandwidth testing
========================
*/
void idLobby::ClientUpdateBandwidthTest()
{
	if( !verify( !IsHost() ) || !verify( host >= 0 ) )
	{
		return;
	}
	
	if( !peers[host].IsConnected() )
	{
		return;
	}
	
	if( bandwidthChallengeStartTime <= 0 )
	{
		// Not doing a test
		return;
	}
	
	int time = Sys_Milliseconds();
	if( bandwidthChallengeEndTime > time )
	{
		// Test is still going on
		return;
	}
	
	// Its been long enough since we last received bw test msg. So lets send the results to the server
	byte buffer[ idPacketProcessor::MAX_MSG_SIZE ];
	idBitMsg msg( buffer, sizeof( buffer ) );
	
	// Send total time it took to receive all the msgs
	// (note, subtract net_bw_test_timeout to get 'last recevied bandwidth test packet')
	// (^^ Note if the last packet is fragmented and we never get it, this is technically wrong!)
	int totalTime = ( bandwidthChallengeEndTime - session->GetTitleStorageInt( "net_bw_test_timeout", net_bw_test_timeout.GetInteger() ) ) - bandwidthChallengeStartTime;
	msg.WriteLong( totalTime );
	
	// Send total number of complete, in order packets we got
	msg.WriteLong( bandwidthChallengeNumGoodSeq );
	
	// Send the overall average bandwidth in KBS
	// Note that sending the number of good packets is not enough. If the packets going out are fragmented, and we
	// drop fragments, the number of good sequences will be lower than the bandwidth we actually received.
	int totalIncomingBytes = peers[host].packetProc->GetIncomingBytes() - peers[host].bandwidthTestBytes;
	msg.WriteLong( totalIncomingBytes );
	
	idLib::Printf( "^3Finished Bandwidth test: \n" );
	idLib::Printf( "  Total time: %d\n", totalTime );
	idLib::Printf( "  Num good packets: %d\n", bandwidthChallengeNumGoodSeq );
	idLib::Printf( "  Total received byes: %d\n\n", totalIncomingBytes );
	
	bandwidthChallengeStartTime = 0;
	bandwidthChallengeNumGoodSeq = 0;
	
	QueueReliableMessage( host, RELIABLE_BANDWIDTH_VALUES, msg.GetReadData(), msg.GetSize() );
}

/*
========================
idLobby::HandleBandwidhTestValue
========================
*/
void idLobby::HandleBandwidhTestValue( int p, idBitMsg& msg )
{
	if( !IsHost() )
	{
		return;
	}
	
	idLib::Printf( "Received RELIABLE_BANDWIDTH_CHECK %d\n", Sys_Milliseconds() );
	
	if( bandwidthChallengeStartTime < 0 || bandwidthChallengeFinished )
	{
		idLib::Warning( "Received bandwidth test results too early from peer %d", p );
		return;
	}
	
	int totalTime = msg.ReadLong();
	int totalGoodSeq = msg.ReadLong();
	int totalReceivedBytes = msg.ReadLong();
	
	// This is the % of complete packets we received. If the packets used in the BWC are big enough to fragment, then pctPackets
	// will be lower than bytesPct (we will have received a larger PCT of overall bandwidth than PCT of full packets received).
	// Im not sure if this is a useful distinction or not, but it may be good to compare against for now.
	float pctPackets = peers[p].bandwidthSequenceNum > 0 ? ( float ) totalGoodSeq / ( float )peers[p].bandwidthSequenceNum : -1.0f;
	
	// This is the % of total bytes sent/bytes received.
	float bytesPct = peers[p].bandwidthTestBytes > 0 ? ( float ) totalReceivedBytes / ( float )peers[p].bandwidthTestBytes : -1.0f;
	
	// Calculate overall bandwidth for the test. That is, total amount received over time.
	// We may want to expand this to also factor in an average instantaneous rate.
	// For now we are mostly concerned with culling out poor performing clients
	float peerKBS = -1.0f;
	if( verify( totalTime > 0 ) )
	{
		peerKBS = ( ( float )totalReceivedBytes / 1024.0f ) / MS2SEC( totalTime );
	}
	
	int totalSendTime = peers[p].bandwidthTestLastSendTime - peers[p].bandwidthChallengeStartSendTime;
	float outgoingKBS = -1.0f;
	if( verify( totalSendTime > 0 ) )
	{
		outgoingKBS = ( ( float )peers[p].bandwidthTestBytes / 1024.0f ) / MS2SEC( totalSendTime );
	}
	
	float pctKBS = peerKBS / outgoingKBS;
	
	bool failedRate = ( pctKBS < session->GetTitleStorageFloat( "net_bw_test_throttle_rate_pct", net_bw_test_throttle_rate_pct.GetFloat() ) );
	bool failedByte = ( bytesPct < session->GetTitleStorageFloat( "net_bw_test_throttle_byte_pct", net_bw_test_throttle_byte_pct.GetFloat() ) );
	bool failedSeq	= ( pctPackets < session->GetTitleStorageFloat( "net_bw_test_throttle_seq_pct", net_bw_test_throttle_seq_pct.GetFloat() ) );
	
	idLib::Printf( "^3Finished Bandwidth test %s: \n", GetPeerName( p ) );
	idLib::Printf( "  Total time: %dms\n", totalTime );
	idLib::Printf( "  %sNum good packets: %d  (%.2f%%)\n", ( failedSeq ? "^1" : "^2" ), totalGoodSeq, pctPackets );
	idLib::Printf( "  %sTotal received bytes: %d  (%.2f%%)\n", ( failedByte ? "^1" : "^2" ), totalReceivedBytes, bytesPct );
	idLib::Printf( "  %sEffective downstream: %.2fkbs (host: %.2fkbs) -> %.2f%%\n\n", ( failedRate ? "^1" : "^2" ), peerKBS, outgoingKBS, pctKBS );
	
	// If shittConnection(totalTime, totalGoodSeq/totalSeq, totalReceivedBytes/totalSentBytes)
	//	throttle this user:
	//	peers[p].throttledSnapRate = baseSnapRate * 2
	if( failedRate || failedByte || failedSeq )
	{
		ThrottlePeerSnapRate( p );
	}
	
	
	// See if we are finished
	peers[p].bandwidthChallengeResults = true;
	bandwidthChallengeFinished = true;
	for( int i = 0; i < peers.Num(); i++ )
	{
		if( peers[i].bandwidthChallengeSendComplete && !peers[i].bandwidthChallengeResults )
		{
			bandwidthChallengeFinished = false;
		}
	}
	
	if( bandwidthChallengeFinished )
	{
		bandwidthChallengeStartTime = 0;
	}
}

/*
========================
idLobby::SendPingValues
Host only
Periodically send all peers' pings to all peers (for the UI).
========================
*/
void idLobby::SendPingValues()
{
	if( !verify( IsHost() ) )
	{
		// paranoia
		return;
	}
	
	const int now = Sys_Milliseconds();
	
	if( nextSendPingValuesTime > now )
	{
		return;
	}
	
	nextSendPingValuesTime = now + PING_INTERVAL_MS;
	
	pktPingValues_t packet;
	
	memset( &packet, 0, sizeof( packet ) );
	
	for( int i = 0; i < peers.Max(); ++i )
	{
		if( i >= peers.Num() )
		{
			packet.pings[ i ] = -1;
		}
		else if( peers[ i ].IsConnected() )
		{
			packet.pings[ i ] = peers[ i ].lastPingRtt;
		}
		else
		{
			packet.pings[ i ] = -1;
		}
	}
	
	byte packetCopy[ sizeof( packet ) ];
	idBitMsg msg( packetCopy, sizeof( packetCopy ) );
	for( int i = 0; i < peers.Max(); ++i )
	{
		msg.WriteShort( packet.pings[ i ] );
	}
	
	for( int i = 0; i < peers.Num(); i++ )
	{
		if( peers[ i ].IsConnected() )
		{
			QueueReliableMessage( i, RELIABLE_PING_VALUES, msg.GetReadData(), msg.GetSize() );
		}
	}
}

/*
========================
idLobby::PumpPings
Host: Periodically determine the round-trip time for a packet to all peers, and tell everyone
	what everyone else's ping to the host is so they can display it in the UI.
Client: Indicate to the player when the server hasn't updated the ping values in too long.
	This is usually going to preceed a connection timeout.
========================
*/
void idLobby::PumpPings()
{
	if( IsHost() )
	{
		// Calculate ping to all peers
		PingPeers();
		// Send the hosts calculated ping values to each peer to everyone has updated ping times
		SendPingValues();
		// Do bandwidth testing
		ServerUpdateBandwidthTest();
		// Send Migration Data
		SendMigrationGameData();
	}
	else if( IsPeer() )
	{
		ClientUpdateBandwidthTest();
		
		if( lastPingValuesRecvTime + PING_INTERVAL_MS + 1000 < Sys_Milliseconds() && migrationInfo.state == MIGRATE_NONE )
		{
			for( int userIndex = 0; userIndex < GetNumLobbyUsers(); ++userIndex )
			{
				lobbyUser_t* user = GetLobbyUser( userIndex );
				if( !verify( user != NULL ) )
				{
					continue;
				}
				user->pingMs = 999999;
			}
		}
	}
}

/*
========================
idLobby::HandleReliablePing
========================
*/
void idLobby::HandleReliablePing( int p, idBitMsg& msg )
{
	int c, b;
	msg.SaveReadState( c, b );
	
	pktPing_t ping;
	
	memset( &ping, 0, sizeof( ping ) );
	if( !verify( sizeof( ping ) <= msg.GetRemainingData() ) )
	{
		NET_VERBOSE_PRINT( "NET: Ignoring ping from peer %i because packet was the wrong size\n", p );
		return;
	}
	
	ping.timestamp = msg.ReadLong();
	
	if( IsHost() )
	{
		// we should probably verify here whether or not this ping was solicited or not
		HandlePingReply( p, ping );
	}
	else
	{
		// this means the server is requesting a ping, so reply
		msg.RestoreReadState( c, b );
		QueueReliableMessage( p, RELIABLE_PING, msg.GetReadData() + msg.GetReadCount(), msg.GetRemainingData() );
	}
}

/*
========================
idLobby::HandlePingReply
========================
*/
void idLobby::HandlePingReply( int p, const pktPing_t& ping )
{
	const int now = Sys_Milliseconds();
	
	const int rtt = now - ping.timestamp;
	peers[p].lastPingRtt = rtt;
	
	for( int userIndex = 0; userIndex < GetNumLobbyUsers(); ++userIndex )
	{
		lobbyUser_t* u = GetLobbyUser( userIndex );
		if( u->peerIndex == p )
		{
			u->pingMs = rtt;
		}
	}
}

/*
========================
idLobby::HandlePingValues
========================
*/
void idLobby::HandlePingValues( idBitMsg& msg )
{
	pktPingValues_t packet;
	memset( &packet, 0, sizeof( packet ) );
	for( int i = 0; i < peers.Max(); ++i )
	{
		packet.pings[ i ] = msg.ReadShort();
	}
	
	assert( IsPeer() );
	
	lastPingValuesRecvTime = Sys_Milliseconds();
	
	for( int userIndex = 0; userIndex < GetNumLobbyUsers(); ++userIndex )
	{
		lobbyUser_t* u = GetLobbyUser( userIndex );
		if( u->peerIndex != -1 && verify( u->peerIndex >= 0 && u->peerIndex < MAX_PEERS ) )
		{
			u->pingMs = packet.pings[ u->peerIndex ];
		}
		else
		{
			u->pingMs = 0;
		}
	}
	
	// Stuff our ping in the hosts slot
	if( peerIndexOnHost != -1 && verify( peerIndexOnHost >= 0 && peerIndexOnHost < MAX_PEERS ) )
	{
		peers[host].lastPingRtt = packet.pings[ peerIndexOnHost ];
	}
	else
	{
		peers[host].lastPingRtt = 0;
	}
}

/*
========================
idLobby::SendAnotherFragment
Other than connectionless sends, this should be the chokepoint for sending packets to peers.
========================
*/
bool idLobby::SendAnotherFragment( int p )
{
	peer_t& peer = peers[p];
	
	if( !peer.IsConnected() )  	// Not connected to any mode (party or game), so no need to send
	{
		return false;
	}
	
	if( !peer.packetProc->HasMoreFragments() )
	{
		return false;		// No fragments to send for this peer
	}
	
	if( !CanSendMoreData( p ) )
	{
		return false;		// We need to throttle the sends so we don't saturate the connection
	}
	
	int time = Sys_Milliseconds();
	
	if( time - peer.lastFragmentSendTime < 2 )
	{
		NET_VERBOSE_PRINT( "Too soon to send another packet. Delta: %d \n", ( time - peer.lastFragmentSendTime ) );
		return false;		// Too soon to send another fragment
	}
	
	peer.lastFragmentSendTime = time;
	
	bool sentFragment = false;
	
	while( true )
	{
		idBitMsg msg;
		// We use the final packet size here because it has been processed, and no more headers will be added
		byte buffer[ idPacketProcessor::MAX_FINAL_PACKET_SIZE ];
		msg.InitWrite( buffer, sizeof( buffer ) );
		
		if( !peers[p].packetProc->GetSendFragment( time, peers[p].sessionID, msg ) )
		{
			break;
		}
		
		const bool useDirectPort = ( lobbyType == TYPE_GAME_STATE );
		
		msg.BeginReading();
		sessionCB->SendRawPacket( peers[p].address, msg.GetReadData(), msg.GetSize(), useDirectPort );
		sentFragment = true;
		break;		// Comment this out to send all fragments in one burst
	}
	
	if( peer.packetProc->HasMoreFragments() )
	{
		NET_VERBOSE_PRINT( "More packets left after ::SendAnotherFragment\n" );
	}
	
	return sentFragment;
}

/*
========================
idLobby::CanSendMoreData
========================
*/
bool idLobby::CanSendMoreData( int p )
{
	if( !verify( p >= 0 && p < peers.Num() ) )
	{
		NET_VERBOSE_PRINT( "NET: CanSendMoreData %i NO: not a peer\n", p );
		return false;
	}
	peer_t& peer = peers[p];
	if( !peer.IsConnected() )
	{
		NET_VERBOSE_PRINT( "NET: CanSendMoreData %i NO: not connected\n", p );
		return false;
	}
	
	return peer.packetProc->CanSendMoreData();
}

/*
========================
idLobby::ProcessOutgoingMsg
========================
*/
void idLobby::ProcessOutgoingMsg( int p, const void* data, int size, bool isOOB, int userData )
{

	peer_t& peer = peers[p];
	
	if( peer.GetConnectionState() != CONNECTION_ESTABLISHED )
	{
		idLib::Printf( "peer.GetConnectionState() != CONNECTION_ESTABLISHED\n" );
		return;	// Peer not fully connected for this session type, return
	}
	
	if( peer.packetProc->HasMoreFragments() )
	{
		idLib::Error( "FATAL: Attempt to process a packet while fragments still need to be sent.\n" ); // We can't handle this case
	}
	
	int currentTime = Sys_Milliseconds();
	
	// if ( currentTime - peer.lastProcTime < 30 ) {
	//	 idLib::Printf("ProcessOutgoingMsg called within %dms %s\n", (currentTime - peer.lastProcTime), GetLobbyName() );
	// }
	
	peer.lastProcTime = currentTime;
	
	if( !isOOB )
	{
		// Keep track of the last time an in-band packet was sent
		// (used for things like knowing when reliables could have been last sent)
		peer.lastInBandProcTime = peer.lastProcTime;
	}
	
	idBitMsg msg;
	msg.InitRead( ( byte* )data, size );
	peer.packetProc->ProcessOutgoing( currentTime, msg, isOOB, userData );
}

/*
========================
idLobby::ResendReliables
========================
*/
void idLobby::ResendReliables( int p )
{

	peer_t& peer = peers[p];
	
	if( !peer.IsConnected() )
	{
		return;
	}
	
	if( peer.packetProc->HasMoreFragments() )
	{
		return;		// We can't send more data while fragments are still being sent out
	}
	
	if( !CanSendMoreData( p ) )
	{
		return;
	}
	
	int time = Sys_Milliseconds();
	
	const int DEFAULT_MIN_RESEND		= 20;		// Quicker resend while not in game to speed up resource transmission acks
	const int DEFAULT_MIN_RESEND_INGAME	= 100;
	
	int resendWait = DEFAULT_MIN_RESEND_INGAME;
	
	if( sessionCB->GetState() == idSession::INGAME )
	{
		// setup some minimum waits and account for ping
		resendWait = Max( DEFAULT_MIN_RESEND_INGAME, peer.lastPingRtt / 2 );
		if( lobbyType == TYPE_PARTY )
		{
			resendWait = Max( 500, resendWait ); // party session does not need fast frequency at all once in game
		}
	}
	else
	{
		// don't trust the ping when still loading stuff
		// need to resend fast to speed up transmission of network decls
		resendWait = DEFAULT_MIN_RESEND;
	}
	
	if( time - peer.lastInBandProcTime < resendWait )
	{
		// no need to resend reliables if they went out on an in-band packet recently
		return;
	}
	
	if( peer.packetProc->NumQueuedReliables() > 0 || peer.packetProc->NeedToSendReliableAck() )
	{
		//NET_VERBOSE_PRINT( "NET: ResendReliables %s\n", GetLobbyName() );
		ProcessOutgoingMsg( p, NULL, 0, false, 0 );		// Force an empty unreliable msg so any reliables will get processed as well
	}
}

/*
========================
idLobby::PumpPackets
========================
*/
void idLobby::PumpPackets()
{
	int newTime = Sys_Milliseconds();
	
	for( int p = 0; p < peers.Num(); p++ )
	{
		if( peers[p].IsConnected() )
		{
			peers[p].packetProc->RefreshRates( newTime );
		}
	}
	
	// Resend reliable msg's (do this before we send out the fragments)
	for( int p = 0; p < peers.Num(); p++ )
	{
		ResendReliables( p );
	}
	
	// If we haven't sent anything to our peers in a long time, make sure to send an empty packet (so our heartbeat gets updated) so we don't get disconnected
	// NOTE - We used to only send these to the host, but the host needs to also send these to clients
	for( int p = 0; p < peers.Num(); p++ )
	{
		if( !peers[p].IsConnected() || peers[p].packetProc->HasMoreFragments() )
		{
			continue;
		}
		if( newTime - peers[p].lastProcTime > 1000 * PEER_HEARTBEAT_IN_SECONDS )
		{
			//NET_VERBOSE_PRINT( "NET: ProcessOutgoing Heartbeat %s\n", GetLobbyName() );
			ProcessOutgoingMsg( p, NULL, 0, false, 0 );
		}
	}
	
	// Send any unsent fragments for each peer (do this last)
	for( int p = 0; p < peers.Num(); p++ )
	{
		SendAnotherFragment( p );
	}
}

/*
========================
idLobby::UpdateMatchParms
========================
*/
void idLobby::UpdateMatchParms( const idMatchParameters& p )
{
	if( !IsHost() )
	{
		return;
	}
	
	parms = p;
	
	// Update lobbyBackend with parms
	if( lobbyBackend != NULL )
	{
		lobbyBackend->UpdateMatchParms( parms );
	}
	
	SendMatchParmsToPeers();
}

/*
========================
idLobby::GetHostUserName
========================
*/
const char* idLobby::GetHostUserName() const
{
	if( !IsLobbyActive() )
	{
		return INVALID_LOBBY_USER_NAME;
	}
	return GetPeerName( -1 );		// This will just grab the first user with this peerIndex (which should be the host)
}

/*
========================
idLobby::SendReliable
========================
*/
void idLobby::SendReliable( int type, idBitMsg& msg, bool callReceiveReliable /*= true*/, peerMask_t sessionUserMask /*= MAX_UNSIGNED_TYPE( peerMask_t ) */ )
{
	//assert( lobbyType == GetActingGameStateLobbyType() );
	
	assert( type < 256 ); // QueueReliable only accepts a byte for message type
	
	// the queuing below sends the whole message
	// I don't know if whole message is a good thing or a bad thing, but if the passed message has been read from already, this is most likely not going to do what the caller expects
	assert( msg.GetReadCount() + msg.GetReadBit() == 0 );
	
	if( callReceiveReliable )
	{
		// NOTE: this will put the msg's read status to fully read - which is why the assert check is above
		common->NetReceiveReliable( -1, type, msg );
	}
	
	uint32 sentPeerMask = 0;
	for( int i = 0; i < GetNumLobbyUsers(); ++i )
	{
		lobbyUser_t* user = GetLobbyUser( i );
		if( user->peerIndex == -1 )
		{
			continue;
		}
		
		// We only care about sending these to peers in our party lobby
		if( user->IsDisconnected() )
		{
			continue;
		}
		
		// Don't sent to a user if they are in the exlusion session user mask
		if( sessionUserMask != 0 && ( sessionUserMask & ( BIT( i ) ) ) == 0 )
		{
			continue;
		}
		
		const int peerIndex = user->peerIndex;
		
		if( peerIndex >= peers.Num() )
		{
			continue;
		}
		
		peer_t& peer = peers[peerIndex];
		
		if( !peer.IsConnected() )
		{
			continue;
		}
		
		if( ( sentPeerMask & ( 1 << user->peerIndex ) ) == 0 )
		{
			QueueReliableMessage( user->peerIndex, idLobby::RELIABLE_GAME_DATA + type, msg.GetReadData(), msg.GetSize() );
			sentPeerMask |= 1 << user->peerIndex;
		}
	}
}

/*
========================
idLobby::SendReliableToLobbyUser
can only be used on the server. will take care of calling locally if addressed to player 0
========================
*/
void idLobby::SendReliableToLobbyUser( lobbyUserID_t lobbyUserID, int type, idBitMsg& msg )
{
	assert( lobbyType == GetActingGameStateLobbyType() );
	assert( type < 256 );			// QueueReliable only accepts a byte for message type
	assert( IsHost() );				// This function should only be called in the server atm
	const int peerIndex = PeerIndexFromLobbyUser( lobbyUserID );
	if( peerIndex >= 0 )
	{
		// will send the remainder of a message that was started reading through, but not handling a partial byte read
		assert( msg.GetReadBit() == 0 );
		QueueReliableMessage( peerIndex, idLobby::RELIABLE_GAME_DATA + type, msg.GetReadData() + msg.GetReadCount(), msg.GetRemainingData() );
	}
	else
	{
		common->NetReceiveReliable( -1, type, msg );
	}
}

/*
========================
idLobby::SendReliableToHost
will make sure to invoke locally if used on the server
========================
*/
void idLobby::SendReliableToHost( int type, idBitMsg& msg )
{
	assert( lobbyType == GetActingGameStateLobbyType() );
	
	if( IsHost() )
	{
		common->NetReceiveReliable( -1, type, msg );
	}
	else
	{
		// will send the remainder of a message that was started reading through, but not handling a partial byte read
		assert( msg.GetReadBit() == 0 );
		QueueReliableMessage( host, idLobby::RELIABLE_GAME_DATA + type, msg.GetReadData() + msg.GetReadCount(), msg.GetRemainingData() );
	}
}

/*
================================================================================================
idLobby::reliablePlayerToPlayerHeader_t
================================================================================================
*/

/*
========================
idLobby::reliablePlayerToPlayerHeader_t::reliablePlayerToPlayerHeader_t
========================
*/
idLobby::reliablePlayerToPlayerHeader_t::reliablePlayerToPlayerHeader_t() : fromSessionUserIndex( -1 ), toSessionUserIndex( -1 )
{
}

/*
========================
idSessionLocal::reliablePlayerToPlayerHeader_t::Read
========================
*/
bool idLobby::reliablePlayerToPlayerHeader_t::Read( idLobby* lobby, idBitMsg& msg )
{
	assert( lobby != NULL );
	
	lobbyUserID_t lobbyUserIDFrom;
	lobbyUserID_t lobbyUserIDTo;
	
	lobbyUserIDFrom.ReadFromMsg( msg );
	lobbyUserIDTo.ReadFromMsg( msg );
	
	fromSessionUserIndex	= lobby->GetLobbyUserIndexByID( lobbyUserIDFrom );
	toSessionUserIndex		= lobby->GetLobbyUserIndexByID( lobbyUserIDTo );
	
	if( !verify( lobby->GetLobbyUser( fromSessionUserIndex ) != NULL ) )
	{
		return false;
	}
	
	if( !verify( lobby->GetLobbyUser( toSessionUserIndex ) != NULL ) )
	{
		return false;
	}
	
	return true;
}

/*
========================
idLobby::reliablePlayerToPlayerHeader_t::Write
========================
*/
bool idLobby::reliablePlayerToPlayerHeader_t::Write( idLobby* lobby, idBitMsg& msg )
{


	if( !verify( lobby->GetLobbyUser( fromSessionUserIndex ) != NULL ) )
	{
		return false;
	}
	
	if( !verify( lobby->GetLobbyUser( toSessionUserIndex ) != NULL ) )
	{
		return false;
	}
	
	lobby->GetLobbyUser( fromSessionUserIndex )->lobbyUserID.WriteToMsg( msg );
	lobby->GetLobbyUser( toSessionUserIndex )->lobbyUserID.WriteToMsg( msg );
	
	return true;
}

/*
========================
idLobby::GetNumActiveLobbyUsers
========================
*/
int idLobby::GetNumActiveLobbyUsers() const
{
	int numActive = 0;
	for( int i = 0; i < GetNumLobbyUsers(); ++i )
	{
		if( !GetLobbyUser( i )->IsDisconnected() )
		{
			numActive++;
		}
	}
	return numActive;
}

/*
========================
idLobby::AllPeersInGame
========================
*/
bool idLobby::AllPeersInGame() const
{
	assert( lobbyType == GetActingGameStateLobbyType() );		// This function doesn't make sense on a party lobby currently
	
	for( int p = 0; p < peers.Num(); p++ )
	{
		if( peers[p].IsConnected() && !peers[p].inGame )
		{
			return false;
		}
	}
	
	return true;
}

/*
========================
idLobby::PeerIndexFromLobbyUser
========================
*/
int	idLobby::PeerIndexFromLobbyUser( lobbyUserID_t lobbyUserID ) const
{
	const int lobbyUserIndex = GetLobbyUserIndexByID( lobbyUserID );
	
	const lobbyUser_t* user = GetLobbyUser( lobbyUserIndex );
	
	if( user == NULL )
	{
		// This needs to be OK for bot support ( or else add bots at the session level )
		return -1;
	}
	
	return user->peerIndex;
}

/*
========================
idLobby::GetPeerTimeSinceLastPacket
========================
*/
int idLobby::GetPeerTimeSinceLastPacket( int peerIndex ) const
{
	if( peerIndex < 0 )
	{
		return 0;
	}
	return Sys_Milliseconds() - peers[peerIndex].lastHeartBeat;
}

/*
========================
idLobby::GetActingGameStateLobbyType
========================
*/
idLobby::lobbyType_t idLobby::GetActingGameStateLobbyType() const
{
	extern idCVar net_useGameStateLobby;
	return ( net_useGameStateLobby.GetBool() ) ? TYPE_GAME_STATE : TYPE_GAME;
}

//========================================================================================================================
//	idLobby::peer_t
//========================================================================================================================

/*
========================
idLobby::peer_t::GetConnectionState
========================
*/
idLobby::connectionState_t idLobby::peer_t::GetConnectionState() const
{
	return connectionState;
}
