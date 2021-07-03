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

idCVar net_snapshot_send_warntime( "net_snapshot_send_warntime", "500", CVAR_INTEGER, "Print warning messages if we take longer than this to send a client a snapshot." );

idCVar net_queueSnapAcks( "net_queueSnapAcks", "1", CVAR_BOOL, "" );

idCVar net_peer_throttle_mode( "net_peer_throttle_mode", "0", CVAR_INTEGER, "= 0 off, 1 = enable fixed, 2 = absolute, 3 = both" );

idCVar net_peer_throttle_minSnapSeq( "net_peer_throttle_minSnapSeq", "150", CVAR_INTEGER, "Minumum number of snapshot exchanges before throttling can be triggered" );

idCVar net_peer_throttle_bps_peer_threshold_pct( "net_peer_throttle_bps_peer_threshold_pct", "0.60", CVAR_FLOAT, "Min reported incoming bps % of sent from host that a peer must maintain before throttling kicks in" );
idCVar net_peer_throttle_bps_host_threshold( "net_peer_throttle_bps_host_threshold", "1024", CVAR_FLOAT, "Min outgoing bps of host for bps based throttling to be considered" );

idCVar net_peer_throttle_bps_decay( "net_peer_throttle_bps_decay", "0.25f", CVAR_FLOAT, "If peer exceeds this number of queued snap deltas, then throttle his effective snap rate" );
idCVar net_peer_throttle_bps_duration( "net_peer_throttle_bps_duration", "3000", CVAR_INTEGER, "If peer exceeds this number of queued snap deltas, then throttle his effective snap rate" );

idCVar net_peer_throttle_maxSnapRate( "net_peer_throttle_maxSnapRate", "4", CVAR_INTEGER, "Highest factor of server base snapRate that a client can be throttled" );

idCVar net_snap_bw_test_throttle_max_scale( "net_snap_bw_test_throttle_max_scale", "0.80", CVAR_FLOAT, "When clamping bandwidth to reported values, scale reported value by this" );

idCVar net_snap_redundant_resend_in_ms( "net_snap_redundant_resend_in_ms", "800", CVAR_INTEGER, "Delay between redundantly sending snaps during initial snap exchange" );
idCVar net_min_ping_in_ms( "net_min_ping_in_ms", "1500", CVAR_INTEGER, "Ping has to be higher than this before we consider throttling to recover" );
idCVar net_pingIncPercentBeforeRecover( "net_pingIncPercentBeforeRecover", "1.3", CVAR_FLOAT, "Percentage change increase of ping before we try to recover" );
idCVar net_maxFailedPingRecoveries( "net_maxFailedPingRecoveries", "10", CVAR_INTEGER, "Max failed ping recoveries before we stop trying" );
idCVar net_pingRecoveryThrottleTimeInSeconds( "net_pingRecoveryThrottleTimeInSeconds", "3", CVAR_INTEGER, "Throttle snaps for this amount of time in seconds to recover from ping spike" );

idCVar net_peer_timeout_loading( "net_peer_timeout_loading", "90000", CVAR_INTEGER, "time in MS to disconnect clients during loading - production only" );


/*
========================
idLobby::UpdateSnaps
========================
*/
void idLobby::UpdateSnaps()
{

	assert( lobbyType == GetActingGameStateLobbyType() );

	SCOPED_PROFILE_EVENT( "UpdateSnaps" );

#if 0
	uint64 startTimeMicroSec = Sys_Microseconds();
#endif

	haveSubmittedSnaps = false;

	if( !SendCompletedSnaps() )
	{
		// If we weren't able to send all the submitted snaps, we need to wait till we can.
		// We can't start new jobs until they are all sent out.
		return;
	}

	for( int p = 0; p < peers.Num(); p++ )
	{
		peer_t& peer = peers[p];

		if( !peer.IsConnected() )
		{
			continue;
		}

		if( peer.needToSubmitPendingSnap )
		{
			// Submit the snap
			if( SubmitPendingSnap( p ) )
			{
				peer.needToSubmitPendingSnap = false;	// only clear this if we actually submitted the snap
			}

		}
	}

#if 0
	uint64 endTimeMicroSec = Sys_Microseconds();

	if( endTimeMicroSec - startTimeMicroSec > 200 )  	// .2 ms
	{
		idLib::Printf( "NET: UpdateSnaps time in ms: %f\n", ( float )( endTimeMicroSec - startTimeMicroSec ) / 1000.0f );
	}
#endif
}

/*
========================
idLobby::SendCompletedSnaps
This function will send send off any previously submitted pending snaps if they are ready
========================
*/
bool idLobby::SendCompletedSnaps()
{
	assert( lobbyType == GetActingGameStateLobbyType() );

	bool sentAllSubmitted = true;

	for( int p = 0; p < peers.Num(); p++ )
	{
		peer_t& peer = peers[p];

		if( !peer.IsConnected() )
		{
			continue;
		}

		if( peer.snapProc->PendingSnapReadyToSend() )
		{
			// Check to see if there are any snaps that were submitted that need to be sent out
			SendCompletedPendingSnap( p );
		}
		else if( IsHost() )
		{
			NET_VERBOSESNAPSHOT_PRINT_LEVEL( 7, va( "  ^8Peer %d pendingSnap not ready to send\n", p ) );
		}

		if( !peer.IsConnected() )    // peer may have been dropped in "SendCompletedPendingSnap". ugh.
		{
			continue;
		}

		if( peer.snapProc->PendingSnapReadyToSend() )
		{
			// If we still have a submitted snap, we know we're not done
			sentAllSubmitted = false;
			if( IsHost() )
			{
				NET_VERBOSESNAPSHOT_PRINT_LEVEL( 2, va( "  ^2Peer %d did not send all submitted snapshots.\n", p ) );
			}
		}
	}

	return sentAllSubmitted;
}

/*
========================
idLobby::SendResources
========================
*/
bool idLobby::SendResources( int p )
{
	assert( lobbyType == GetActingGameStateLobbyType() );

	return false;
}

/*
========================
idLobby::SubmitPendingSnap
========================
*/
bool idLobby::SubmitPendingSnap( int p )
{

	assert( lobbyType == GetActingGameStateLobbyType() );

	peer_t& peer = peers[p];

	if( !peer.IsConnected() )
	{
		return false;
	}

	// If the peer doesn't have the latest resource list, send it to him before sending any new snapshots
	if( SendResources( p ) )
	{
		return false;
	}

	if( !peer.loaded )
	{
		return false;
	}

	if( !peer.snapProc->HasPendingSnap() )
	{
		return false;
	}

	int time = Sys_Milliseconds();

	int timeFromLastSub = time - peer.lastSnapJobTime;

	int forceResendTime = session->GetTitleStorageInt( "net_snap_redundant_resend_in_ms", net_snap_redundant_resend_in_ms.GetInteger() );

	if( timeFromLastSub < forceResendTime && peer.snapProc->IsBusyConfirmingPartialSnap() )
	{
		return false;
	}

	peer.lastSnapJobTime = time;
	assert( !peer.snapProc->PendingSnapReadyToSend() );

	// Submit snapshot delta to jobs
	peer.snapProc->SubmitPendingSnap( p + 1, objMemory, SNAP_OBJ_JOB_MEMORY, lzwData );

	NET_VERBOSESNAPSHOT_PRINT_LEVEL( 2, va( "  Submitted snapshot to jobList for peer %d. Since last jobsub: %d\n", p, timeFromLastSub ) );

	return true;
}

/*
========================
idLobby::SendCompletedPendingSnap
========================
*/
void idLobby::SendCompletedPendingSnap( int p )
{

	assert( lobbyType == GetActingGameStateLobbyType() );

	int time = Sys_Milliseconds();

	peer_t& peer = peers[p];

	if( !peer.IsConnected() )
	{
		return;
	}

	if( peer.snapProc == NULL || !peer.snapProc->PendingSnapReadyToSend() )
	{
		return;
	}

	// If we have a pending snap ready to send, we better have a pending snap
	assert( peer.snapProc->HasPendingSnap() );

	// Get the snap data blob now, even if we don't send it.
	// This is somewhat wasteful, but we have to do this to keep the snap job pipe ready to keep doing work
	// If we don't do this, this peer will cause other peers to be starved of snapshots, when they may very well be ready to send a snap
	byte buffer[ MAX_SNAP_SIZE ];
	int maxLength = sizeof( buffer ) - peer.packetProc->GetReliableDataSize() - 128;

	int size = peer.snapProc->GetPendingSnapDelta( buffer, maxLength );

	if( !CanSendMoreData( p ) )
	{
		return;
	}

	// Can't send anymore snapshots until all fragments are sent
	if( peer.packetProc->HasMoreFragments() )
	{
		return;
	}

	// If the peer doesn't have the latest resource list, send it to him before sending any new snapshots
	if( SendResources( p ) )
	{
		return;
	}

	int timeFromJobSub = time - peer.lastSnapJobTime;
	int timeFromLastSend = time - peer.lastSnapTime;

	if( timeFromLastSend > 0 )
	{
		peer.snapHz = 1000.0f / ( float )timeFromLastSend;
	}
	else
	{
		peer.snapHz = 0.0f;
	}

	if( net_snapshot_send_warntime.GetInteger() > 0 && peer.lastSnapTime != 0 && net_snapshot_send_warntime.GetInteger() < timeFromLastSend )
	{
		idLib::Printf( "NET: Took %d ms to send peer %d snapshot\n", timeFromLastSend, p );
	}

	if( peer.throttleSnapsForXSeconds != 0 )
	{
		if( time < peer.throttleSnapsForXSeconds )
		{
			return;
		}

		// If we were trying to recover ping, see if we succeeded
		if( peer.recoverPing != 0 )
		{
			if( peer.lastPingRtt >= peer.recoverPing )
			{
				peer.failedPingRecoveries++;
			}
			else
			{
				const int peer_throttle_minSnapSeq = session->GetTitleStorageInt( "net_peer_throttle_minSnapSeq", net_peer_throttle_minSnapSeq.GetInteger() );
				if( peer.snapProc->GetFullSnapBaseSequence() > idSnapshotProcessor::INITIAL_SNAP_SEQUENCE + peer_throttle_minSnapSeq )
				{
					// If throttling recovered the ping
					int maxRate = common->GetSnapRate() * session->GetTitleStorageInt( "net_peer_throttle_maxSnapRate", net_peer_throttle_maxSnapRate.GetInteger() );
					peer.throttledSnapRate = idMath::ClampInt( common->GetSnapRate(), maxRate, peer.throttledSnapRate + common->GetSnapRate() );
				}
			}
		}

		peer.throttleSnapsForXSeconds = 0;
	}

	peer.lastSnapTime = time;

	if( size != 0 )
	{
		if( size > 0 )
		{
			NET_VERBOSESNAPSHOT_PRINT_LEVEL( 3, va( "NET: (peer %d) Sending snapshot %d delta'd against %d. Since JobSub: %d Since LastSend: %d. Size: %d\n", p, peer.snapProc->GetSnapSequence(), peer.snapProc->GetBaseSequence(), timeFromJobSub, timeFromLastSend, size ) );
			ProcessOutgoingMsg( p, buffer, size, false, 0 );
		}
		else if( size < 0 )  	// Size < 0 indicates the delta buffer filled up
		{
			// There used to be code here that would disconnect peers if they were in game and filled up the buffer
			// This was causing issues in the playtests we were running (Doom 4 MP) and after some conversation
			// determined that it was not needed since a timeout mechanism has been added since
			ProcessOutgoingMsg( p, buffer, -size, false, 0 );
			if( peer.snapProc != NULL )
			{
				NET_VERBOSESNAPSHOT_PRINT( "NET: (peerNum: %d - name: %s) Resending last snapshot delta %d because his delta list filled up. Since JobSub: %d Since LastSend: %d Delta Size: %d\n", p, GetPeerName( p ), peer.snapProc->GetSnapSequence(), timeFromJobSub, timeFromLastSend, size );
			}
		}
	}

	// We calculate what our outgoing rate was for each sequence, so we can have a relative comparison
	// for when the client reports what his downstream was in the same timeframe
	if( IsHost() && peer.snapProc != NULL && peer.snapProc->GetSnapSequence() > 0 )
	{
		//NET_VERBOSE_PRINT("^8  %i Rate: %.2f   SnapSeq: %d GetBaseSequence: %d\n", lastAppendedSequence, peer.packetProc->GetOutgoingRateBytes(), peer.snapProc->GetSnapSequence(), peer.snapProc->GetBaseSequence() );
		peer.sentBpsHistory[ peer.snapProc->GetSnapSequence() % MAX_BPS_HISTORY ] = peer.packetProc->GetOutgoingRateBytes();
	}
}

/*
========================
idLobby::CheckPeerThrottle
========================
*/
void idLobby::CheckPeerThrottle( int p )
{
	assert( lobbyType == GetActingGameStateLobbyType() );

	if( !verify( p >= 0 && p < peers.Num() ) )
	{
		return;
	}

	peer_t& peer = peers[p];

	if( !peer.IsConnected() )
	{
		return;
	}

	if( !IsHost() )
	{
		return;
	}

	if( session->GetTitleStorageInt( "net_peer_throttle_mode", net_peer_throttle_mode.GetInteger() ) == 0 )
	{
		return;
	}

	if( peer.receivedBps < 0.0f )
	{
		return;
	}

	int time = Sys_Milliseconds();

	if( !AllPeersHaveBaseState() )
	{
		return;
	}

	if( verify( peer.snapProc != NULL ) )
	{
		const int peer_throttle_minSnapSeq = session->GetTitleStorageInt( "net_peer_throttle_minSnapSeq", net_peer_throttle_minSnapSeq.GetInteger() );
		if( peer.snapProc->GetFullSnapBaseSequence() <= idSnapshotProcessor::INITIAL_SNAP_SEQUENCE + peer_throttle_minSnapSeq )
		{
			return;
		}
	}

	// This is bps throttling which compares the sent bytes per second to the reported received bps
	float peer_throttle_bps_host_threshold = session->GetTitleStorageFloat( "net_peer_throttle_bps_host_threshold", net_peer_throttle_bps_host_threshold.GetFloat() );

	if( peer_throttle_bps_host_threshold > 0.0f )
	{
		int deltaT = idMath::ClampInt( 0, 100, time - peer.receivedThrottleTime );
		if( deltaT > 0 && peer.receivedThrottleTime > 0 && peer.receivedBpsIndex > 0 )
		{

			bool throttled = false;
			float sentBps = peer.sentBpsHistory[ peer.receivedBpsIndex % MAX_BPS_HISTORY ];

			// Min outgoing rate from server (don't throttle if we are sending < 1k)
			if( sentBps > peer_throttle_bps_host_threshold )
			{
				float pct = peer.receivedBps / idMath::ClampFloat( 0.01f, static_cast<float>( BANDWIDTH_REPORTING_MAX ), sentBps ); // note the receivedBps is implicitly clamped on client end to 10k/sec

				/*
				static int lastSeq = 0;
				if ( peer.receivedBpsIndex != lastSeq ) {
					NET_VERBOSE_PRINT( "%ssentBpsHistory[%d] = %.2f   received: %.2f PCT: %.2f \n", ( pct > 1.0f ? "^1" : "" ), peer.receivedBpsIndex, sentBps, peer.receivedBps, pct );
				}
				lastSeq = peer.receivedBpsIndex;
				*/

				// Increase throttle time if peer is < % of what we are sending him
				if( pct < session->GetTitleStorageFloat( "net_peer_throttle_bps_peer_threshold_pct", net_peer_throttle_bps_peer_threshold_pct.GetFloat() ) )
				{
					peer.receivedThrottle += ( float )deltaT;
					throttled = true;
					NET_VERBOSE_PRINT( "NET: throttled... %.2f ....pct %.2f  receivedBps %.2f outgoingBps %.2f, peer %i, seq %i\n", peer.receivedThrottle, pct, peer.receivedBps, sentBps, p, peer.snapProc->GetFullSnapBaseSequence() );
				}
			}

			if( !throttled )
			{
				float decayRate = session->GetTitleStorageFloat( "net_peer_throttle_bps_decay", net_peer_throttle_bps_decay.GetFloat() );

				peer.receivedThrottle = Max<float>( 0.0f, peer.receivedThrottle - ( ( ( float )deltaT ) * decayRate ) );
				//NET_VERBOSE_PRINT("NET: !throttled... %.2f ....receivedBps %.2f outgoingBps %.2f\n", peer.receivedThrottle, peer.receivedBps, sentBps );
			}

			float duration = session->GetTitleStorageFloat( "net_peer_throttle_bps_duration", net_peer_throttle_bps_duration.GetFloat() );

			if( peer.receivedThrottle > duration )
			{
				peer.maxSnapBps = peer.receivedBps * session->GetTitleStorageFloat( "net_snap_bw_test_throttle_max_scale", net_snap_bw_test_throttle_max_scale.GetFloat() );

				int maxRate = common->GetSnapRate() * session->GetTitleStorageInt( "net_peer_throttle_maxSnapRate", net_peer_throttle_maxSnapRate.GetInteger() );

				if( peer.throttledSnapRate == 0 )
				{
					peer.throttledSnapRate = common->GetSnapRate() * 2;
				}
				else if( peer.throttledSnapRate < maxRate )
				{
					peer.throttledSnapRate = idMath::ClampInt( common->GetSnapRate(), maxRate, peer.throttledSnapRate + common->GetSnapRate() );
				}

				peer.receivedThrottle = 0.0f;	// Start over, so we don't immediately throttle again
			}
		}
		peer.receivedThrottleTime = time;
	}
}

/*
========================
idLobby::ApplySnapshotDelta
========================
*/
void idLobby::ApplySnapshotDelta( int p, int snapshotNumber )
{
	assert( lobbyType == GetActingGameStateLobbyType() );

	if( !verify( p >= 0 && p < peers.Num() ) )
	{
		return;
	}
	peer_t& peer = peers[p];
	if( !peer.IsConnected() )
	{
		return;
	}

	if( net_queueSnapAcks.GetBool() && AllPeersHaveBaseState() )
	{
		// If we've reached our queue limit, force the oldest one out now
		if( snapDeltaAckQueue.Num() == snapDeltaAckQueue.Max() )
		{
			ApplySnapshotDeltaInternal( snapDeltaAckQueue[0].p, snapDeltaAckQueue[0].snapshotNumber );
			snapDeltaAckQueue.RemoveIndex( 0 );
		}

		// Queue up acks, so we can spread them out over frames to lighten the load when they all come in at once
		snapDeltaAck_t snapDeltaAck;

		snapDeltaAck.p				= p;
		snapDeltaAck.snapshotNumber = snapshotNumber;

		snapDeltaAckQueue.Append( snapDeltaAck );
	}
	else
	{
		ApplySnapshotDeltaInternal( p, snapshotNumber );
	}
}

/*
========================
idLobby::ApplySnapshotDeltaInternal
========================
*/
bool idLobby::ApplySnapshotDeltaInternal( int p, int snapshotNumber )
{
	assert( lobbyType == GetActingGameStateLobbyType() );

	if( !verify( p >= 0 && p < peers.Num() ) )
	{
		return false;
	}

	peer_t& peer = peers[p];

	if( !peer.IsConnected() )
	{
		return false;
	}

	// on the server, player = peer number + 1, this only works as long as we don't support clients joining and leaving during game
	// on the client, always 0
	bool result = peer.snapProc->ApplySnapshotDelta( IsHost() ? p + 1 : 0, snapshotNumber );

	if( result && IsHost() && peer.snapProc->HasPendingSnap() )
	{
		// Send more of the pending snap if we have one for this peer.
		// The reason we can do this, is because we know more about this peers base state now.
		// And since we maxed out the optimal snap delta size, we'll now be able
		// to send more data, since we assume we'll get better and better delta compression as
		// our version of this peers base state approaches parity with the peers actual state.

		// We don't send immediately, since we have to coordinate sending snaps for all peers in same place considering jobs.
		peer.needToSubmitPendingSnap = true;
		NET_VERBOSESNAPSHOT_PRINT( "NET: Sent more unsent snapshot data to peer %d for snapshot %d\n", p, snapshotNumber );
	}

	return result;
}

/*
========================
idLobby::SendSnapshotToPeer
========================
*/
idCVar net_forceDropSnap( "net_forceDropSnap", "0", CVAR_BOOL, "wait on snaps" );
void idLobby::SendSnapshotToPeer( idSnapShot& ss, int p )
{
	assert( lobbyType == GetActingGameStateLobbyType() );

	peer_t& peer = peers[p];

	if( net_forceDropSnap.GetBool() )
	{
		net_forceDropSnap.SetBool( false );
		return;
	}

	if( peer.pauseSnapshots )
	{
		return;
	}

	int time = Sys_Milliseconds();

	const int throttleMode = session->GetTitleStorageInt( "net_peer_throttle_mode", net_peer_throttle_mode.GetInteger() );

	// Real peer throttling based on performance
	// -We throttle before sending to jobs rather than before sending

	if( ( throttleMode == 1 || throttleMode == 3 ) && peer.throttledSnapRate > 0 )
	{
		if( time - peer.lastSnapJobTime < peer.throttledSnapRate / 1000 )    // fixme /1000
		{
			// This peer is throttled, skip his snap shot
			NET_VERBOSESNAPSHOT_PRINT_LEVEL( 2, va( "NET: Throttling peer %d.Skipping snapshot. Time elapsed: %d peer snap rate: %d\n", p, ( time - peer.lastSnapJobTime ), peer.throttledSnapRate ) );
			return;
		}
	}

	if( throttleMode != 0 )
	{
		DetectSaturation( p );
	}

	if( peer.maxSnapBps >= 0.0f && ( throttleMode == 2 || throttleMode == 3 ) )
	{
		if( peer.packetProc->GetOutgoingRateBytes() > peer.maxSnapBps )
		{
			return;
		}
	}

	// TrySetPendingSnapshot will try to set the new pending snap.
	// TrySetPendingSnapshot won't do anything until the last snap set was fully sent out.

	if( peer.snapProc->TrySetPendingSnapshot( ss ) )
	{
		NET_VERBOSESNAPSHOT_PRINT_LEVEL( 2, va( "  ^8Set next pending snapshot peer %d\n", 0 ) );

		peer.numSnapsSent++;

		idSnapShot* baseState = peers[p].snapProc->GetBaseState();
		if( verify( baseState != NULL ) )
		{
			baseState->UpdateExpectedSeq( peers[p].snapProc->GetSnapSequence() );
		}

	}
	else
	{
		NET_VERBOSESNAPSHOT_PRINT_LEVEL( 2, va( "  ^2FAILED Set next pending snapshot peer %d\n", 0 ) );
	}

	// We send out the pending snap, which could be the most recent, or an old one that hasn't fully been sent
	// We don't send immediately, since we have to coordinate sending snaps for all peers in same place considering jobs.
	peer.needToSubmitPendingSnap = true;
}

/*
========================
idLobby::AllPeersHaveBaseState
========================
*/
bool idLobby::AllPeersHaveBaseState()
{
	assert( lobbyType == GetActingGameStateLobbyType() );

	for( int i = 0; i < peers.Num(); ++i )
	{

		if( !peers[i].IsConnected() )
		{
			continue;
		}

		if( peers[i].snapProc->GetFullSnapBaseSequence() < idSnapshotProcessor::INITIAL_SNAP_SEQUENCE )
		{
			return false;		// If a client hasn't ack'd his first full snap, then we are still sending base state to someone
		}
	}

	return true;
}

/*
========================
idLobby::ThrottleSnapsForXSeconds
========================
*/
void idLobby::ThrottleSnapsForXSeconds( int p, int seconds, bool recoverPing )
{
	assert( lobbyType == GetActingGameStateLobbyType() );

	if( peers[p].throttleSnapsForXSeconds != 0 )
	{
		return;		// Already throttling snaps
	}

	idLib::Printf( "Throttling peer %i for %i seconds...\n", p, seconds );

	peers[p].throttleSnapsForXSeconds	= Sys_Milliseconds() + seconds * 1000;
	peers[p].recoverPing				= recoverPing ? peers[p].lastPingRtt : 0;
}

/*
========================
idLobby::FirstSnapHasBeenSent
========================
*/
bool idLobby::FirstSnapHasBeenSent( int p )
{
	assert( lobbyType == GetActingGameStateLobbyType() );

	if( !verify( p >= 0 && p < peers.Num() ) )
	{
		return false;
	}

	peer_t& peer = peers[p];

	if( peer.numSnapsSent == 0 )
	{
		return false;
	}

	if( peer.snapProc == NULL )
	{
		return false;
	}

	idSnapShot* ss = peer.snapProc->GetPendingSnap();

	if( ss == NULL )
	{
		return false;
	}

	if( ss->NumObjects() == 0 )
	{
		return false;
	}

	return true;
}

/*
========================
idLobby::EnsureAllPeersHaveBaseState
This function ensures all peers that started the match together (they were in the lobby when it started) start together.
Join in progress peers will be handled as they join.
========================
*/
bool idLobby::EnsureAllPeersHaveBaseState()
{

	assert( lobbyType == GetActingGameStateLobbyType() );

	int time = Sys_Milliseconds();


	for( int i = 0; i < peers.Num(); ++i )
	{
		if( !peers[i].IsConnected() )
		{
			continue;
		}

		if( !FirstSnapHasBeenSent( i ) )
		{
			continue;		// Must be join in progress peer
		}

		if( peers[i].snapProc->GetFullSnapBaseSequence() < idSnapshotProcessor::INITIAL_SNAP_SEQUENCE )
		{
			if( time - peers[i].lastSnapTime > session->GetTitleStorageInt( "net_snap_redundant_resend_in_ms", net_snap_redundant_resend_in_ms.GetInteger() ) )
			{
				SendSnapshotToPeer( *peers[i].snapProc->GetPendingSnap(), i );
			}
			return false;
		}
	}

	return true;
}

/*
========================
idLobby::AllPeersHaveStaleSnapObj
========================
*/
bool idLobby::AllPeersHaveStaleSnapObj( int objId )
{
	assert( lobbyType == GetActingGameStateLobbyType() );

	for( int i = 0; i < peers.Num(); i++ )
	{
		if( !peers[i].IsConnected() )
		{
			continue;
		}

		idSnapShot* baseState = peers[i].snapProc->GetBaseState();

		idSnapShot::objectState_t* state = baseState->FindObjectByID( objId );

		if( state == NULL || !state->stale )
		{
			return false;
		}
	}
	return true;
}

/*
========================
idLobby::AllPeersHaveExpectedSnapObj
========================
*/
bool idLobby::AllPeersHaveExpectedSnapObj( int objId )
{
	assert( lobbyType == GetActingGameStateLobbyType() );

	for( int i = 0; i < peers.Num(); i++ )
	{
		if( !peers[i].IsConnected() )
		{
			continue;
		}

		idSnapShot* baseState = peers[i].snapProc->GetBaseState();
		idSnapShot::objectState_t* state = baseState->FindObjectByID( objId );
		if( state == NULL )
		{
			return false;
		}

		if( state->expectedSequence == -2 )
		{
			return false;
		}

		if( state->expectedSequence > 0 && peers[i].snapProc->GetFullSnapBaseSequence() <= state->expectedSequence )
		{
			//idLib::Printf("^3Not ready to go stale. obj %d Base: %d  expected: %d\n", objId, peers[i].snapProc->GetBaseSequence(), state->expectedSequence );
			return false;
		}
	}

	return true;
}

/*
========================
idLobby::MarkSnapObjDeleted
========================
*/
void idLobby::RefreshSnapObj( int objId )
{
	assert( lobbyType == GetActingGameStateLobbyType() );

	for( int i = 0; i < peers.Num(); i++ )
	{
		if( !peers[i].IsConnected() )
		{
			continue;
		}

		idSnapShot* baseState = peers[i].snapProc->GetBaseState();
		idSnapShot::objectState_t* state = baseState->FindObjectByID( objId );
		if( state != NULL )
		{
			// Setting to -2 will defer setting the expected sequence until the current snap is ready to send
			state->expectedSequence = -2;
		}
	}
}

/*
========================
idLobby::MarkSnapObjDeleted
========================
*/
void idLobby::MarkSnapObjDeleted( int objId )
{
	assert( lobbyType == GetActingGameStateLobbyType() );

	for( int i = 0; i < peers.Num(); i++ )
	{
		if( !peers[i].IsConnected() )
		{
			continue;
		}

		idSnapShot* baseState = peers[i].snapProc->GetBaseState();

		idSnapShot::objectState_t* state = baseState->FindObjectByID( objId );

		if( state != NULL )
		{
			state->deleted = true;
		}
	}
}

/*
========================
idLobby::ResetBandwidthStats
========================
*/
void idLobby::ResetBandwidthStats()
{
	assert( lobbyType == GetActingGameStateLobbyType() );

	lastSnapBspHistoryUpdateSequence = -1;

	for( int p = 0; p < peers.Num(); p++ )
	{
		peers[p].maxSnapBps					= -1.0f;
		peers[p].throttledSnapRate			= 0;
		peers[p].rightBeforeSnapsPing		= peers[p].lastPingRtt;
		peers[p].throttleSnapsForXSeconds	= 0;
		peers[p].recoverPing				= 0;
		peers[p].failedPingRecoveries		= 0;
		peers[p].rightBeforeSnapsPing		= 0;

	}
}

/*
========================
idLobby::DetectSaturation
See if the ping shot up, which indicates a previously saturated connection
========================
*/
void idLobby::DetectSaturation( int p )
{
	assert( lobbyType == GetActingGameStateLobbyType() );

	peer_t& peer = peers[p];

	if( !peer.IsConnected() )
	{
		return;
	}

	const float pingIncPercentBeforeThottle			= session->GetTitleStorageFloat( "net_pingIncPercentBeforeRecover", net_pingIncPercentBeforeRecover.GetFloat() );
	const int	pingThreshold						= session->GetTitleStorageInt( "net_min_ping_in_ms", net_min_ping_in_ms.GetInteger() );
	const int	maxFailedPingRecoveries				= session->GetTitleStorageInt( "net_maxFailedPingRecoveries", net_maxFailedPingRecoveries.GetInteger() );
	const int	pingRecoveryThrottleTimeInSeconds	= session->GetTitleStorageInt( "net_pingRecoveryThrottleTimeInSeconds", net_pingRecoveryThrottleTimeInSeconds.GetInteger() );

	if( peer.lastPingRtt > peer.rightBeforeSnapsPing * pingIncPercentBeforeThottle && peer.lastPingRtt > pingThreshold )
	{
		if( peer.failedPingRecoveries < maxFailedPingRecoveries )
		{
			ThrottleSnapsForXSeconds( p, pingRecoveryThrottleTimeInSeconds, true );
		}
	}
}

/*
========================
idLobby::AddSnapObjTemplate
========================
*/
void idLobby::AddSnapObjTemplate( int objID, idBitMsg& msg )
{
	assert( lobbyType == GetActingGameStateLobbyType() );

	// If we are in the middle of a SS read, apply this state to what we
	// just deserialized (the obj we just deserialized is a delta from the template object we are adding right now)
	if( localReadSS != NULL )
	{
		localReadSS->ApplyToExistingState( objID, msg );
	}

	// Add the template to the snapshot proc for future snapshot processing
	for( int p = 0; p < peers.Num(); p++ )
	{
		if( !peers[p].IsConnected() || peers[p].snapProc == NULL )
		{
			continue;
		}
		peers[p].snapProc->AddSnapObjTemplate( objID, msg );
	}
}
