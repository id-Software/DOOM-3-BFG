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
#include "../framework/Common_local.h"
#include "sys_session_local.h"

// The more the idLobby class needs to call back into this class, the more likely we're doing something wrong, and there is a better way.

/*
========================
idSessionLocalCallbacks::BecomingHost
This is called when
========================
*/
bool idSessionLocalCallbacks::BecomingHost( idLobby& lobby )
{
	if( lobby.lobbyType == idLobby::TYPE_GAME )
	{
		if( sessionLocal->GetActivePlatformLobby() != &lobby )
		{
			idLib::Printf( "BecomingHost: Must be past the party lobby to become host of a game lobby.\n" );
			return false;
		}
		if( sessionLocal->localState == idSessionLocal::STATE_INGAME || sessionLocal->localState == idSessionLocal::STATE_LOADING )
		{
			// If we are in a game, go back to the lobby before becoming the new host of a game lobby
			sessionLocal->SetState( idSessionLocal::STATE_GAME_LOBBY_PEER );

			// session mgr housekeeping that would usually be done through the standard EndMatch path
			sessionLocal->EndMatchForMigration();
		}
	}

	return true;
}

/*
========================
idSessionLocalCallbacks::BecameHost
========================
*/
void idSessionLocalCallbacks::BecameHost( idLobby& lobby )
{
	// If we were in the lobby when we switched to host, then set the right state
	if( lobby.lobbyType == idLobby::TYPE_PARTY && sessionLocal->localState == idSessionLocal::STATE_PARTY_LOBBY_PEER )
	{
		sessionLocal->SetState( idSessionLocal::STATE_PARTY_LOBBY_HOST );
	}
	else if( lobby.lobbyType == idLobby::TYPE_GAME && sessionLocal->localState == idSessionLocal::STATE_GAME_LOBBY_PEER )
	{
		sessionLocal->SetState( idSessionLocal::STATE_GAME_LOBBY_HOST );
	}
}

/*
========================
idSessionLocalCallbacks::BecomingPeer
========================
*/
bool idSessionLocalCallbacks::BecomingPeer( idLobby& lobby )
{
	if( lobby.lobbyType == idLobby::TYPE_GAME )
	{
		if( sessionLocal->localState == idSessionLocal::STATE_INGAME || sessionLocal->localState == idSessionLocal::STATE_LOADING )
		{
			// Go to the party lobby while we try to connect to the new host
			// This isn't totally necessary but we want to end the current game now and go to some screen.
			// When the connection goes through or fails will send the session mgr to the appropriate state (game lobby or main menu)

			// What happens if we got the game migration before the party migration?
			sessionLocal->SetState( sessionLocal->GetPartyLobby().IsHost() ? idSessionLocal::STATE_PARTY_LOBBY_HOST : idSessionLocal::STATE_PARTY_LOBBY_PEER );

			// session mgr housekeeping that would usually be done through the standard EndMatch path
			sessionLocal->EndMatchForMigration();

			return true; // return true tells the session that we want him to tell us when the connects/fails
		}
	}
	return false;
}

/*
========================
idSessionLocalCallbacks::BecamePeer
========================
*/
void idSessionLocalCallbacks::BecamePeer( idLobby& lobby )
{
	if( lobby.lobbyType == idLobby::TYPE_GAME )
	{
		sessionLocal->SetState( idSessionLocal::STATE_GAME_LOBBY_PEER );
	}
}

/*
========================
idSessionLocalCallbacks::FailedGameMigration
========================
*/
void idSessionLocalCallbacks::FailedGameMigration( idLobby& lobby )
{
	// We failed to complete a game migration this could happen for a couple reasons:
	// -The network invites failed / failed to join migrated session
	// -There was nobody to invite
	lobby.ResetAllMigrationState();
	if( lobby.lobbyType == idLobby::TYPE_GAME )    // this check is a  redundant since we should only get this CB from the game session
	{

		sessionLocal->SetState( idSessionLocal::STATE_GAME_LOBBY_HOST );

		// Make sure the sessions are joinable again
		sessionLocal->EndSessions();
	}
}

/*
========================
idSessionLocalCallbacks::MigrationEnded
========================
*/
void idSessionLocalCallbacks::MigrationEnded( idLobby& lobby )
{
	if( lobby.migrationInfo.persistUntilGameEndsData.wasMigratedGame )
	{
#if 1
		if( lobby.lobbyType == idLobby::TYPE_GAME || ( lobby.lobbyType == idLobby::TYPE_PARTY && session->GetState() <= idSession::PARTY_LOBBY ) )
		{
			common->Dialog().ClearDialog( GDM_MIGRATING );
			common->Dialog().ClearDialog( GDM_MIGRATING_WAITING );
			common->Dialog().ClearDialog( GDM_MIGRATING_RELAUNCHING );

			if( lobby.GetNumLobbyUsers() <= 1 )
			{
				if( MatchTypeHasStats( lobby.parms.matchFlags ) )
				{
					common->Dialog().AddDialog( GDM_MIGRATING_FAILED_DISBANDED_STATS, DIALOG_ACCEPT, NULL, NULL, false, "", 0, true );		// Game has disbanded
				}
				else
				{
					common->Dialog().AddDialog( GDM_MIGRATING_FAILED_DISBANDED, DIALOG_ACCEPT, NULL, NULL, false, "", 0, true );		// Game has disbanded
				}
			}
			else
			{
				//common->Dialog().AddDialog( GDM_MIGRATING_FAILED_CONNECTION, DIALOG_ACCEPT, NULL, NULL, false, "", 0, true );		// Lost connection to game
				if( lobby.lobbyType == idLobby::TYPE_GAME && MatchTypeHasStats( lobby.parms.matchFlags ) )
				{
					// This means we came from a public match, so tell them they didn't lose stats
					common->Dialog().AddDialog( GDM_HOST_CONNECTION_LOST_STATS, DIALOG_ACCEPT, NULL, NULL, false, "", 0, true );		// The connection to the host has been lost. This game will not count towards your ranking.
				}
				else
				{
					// This means we came from a private match, just say host quit
					common->Dialog().AddDialog( GDM_HOST_CONNECTION_LOST, DIALOG_ACCEPT, NULL, NULL, false, "", 0, true );			// The connection to the host has been lost.
				}
			}

			lobby.ResetAllMigrationState();

			// Make sure the sessions are joinable again
			sessionLocal->EndSessions();
		}
#else
		// If we get here, we migrated from a game
		if( lobby.GetNumLobbyUsers() <= 1 && lobby.lobbyType == idLobby::TYPE_GAME )
		{
			if( !MatchTypeIsJoinInProgress( lobby.parms.matchFlags ) )
			{
				// Handles 'soft' failed game migration where we migrated from a game and are now alone
				gameDialogMessages_t errorDlg = GDM_INVALID;
				lobby.migrationInfo.persistUntilGameEndsData.hasGameData = false; // never restart the game if we are by ourselves
				if( lobby.migrationInfo.invites.Num() > 0 )
				{
					// outstanding invites: migration failed
					errorDlg = ( MatchTypeHasStats( lobby.migrateMsgFlags ) && ( sessionLocal->GetFlushedStats() == false ) ) ? GDM_MIGRATING_FAILED_CONNECTION_STATS : GDM_MIGRATING_FAILED_CONNECTION;
				}
				else
				{
					// there was no one to invite
					errorDlg = ( MatchTypeHasStats( lobby.migrateMsgFlags ) && ( sessionLocal->GetFlushedStats() == false ) ) ? GDM_MIGRATING_FAILED_DISBANDED_STATS : GDM_MIGRATING_FAILED_DISBANDED;
				}
				if( errorDlg != GDM_INVALID )
				{
					common->Dialog().AddDialog( errorDlg, DIALOG_ACCEPT, NULL, NULL, false );
				}
				common->Dialog().ClearDialog( GDM_MIGRATING );
				common->Dialog().ClearDialog( GDM_MIGRATING_WAITING );
				common->Dialog().ClearDialog( GDM_MIGRATING_RELAUNCHING );

				FailedGameMigration( lobby );
			}
		}
		else if( lobby.lobbyType == idLobby::TYPE_PARTY )
		{
			if( session->GetState() <= idSession::PARTY_LOBBY )
			{
				// We got dropped the party lobby, let them know what happened
				common->Dialog().ClearDialog( GDM_MIGRATING );
				common->Dialog().ClearDialog( GDM_MIGRATING_WAITING );
				common->Dialog().ClearDialog( GDM_MIGRATING_RELAUNCHING );

				if( lobby.GetNumLobbyUsers() <= 1 )
				{
					common->Dialog().AddDialog( GDM_MIGRATING_FAILED_DISBANDED, DIALOG_ACCEPT, NULL, NULL, false, "", 0, true );		// Game has disbanded
				}
				else
				{
					//common->Dialog().AddDialog( GDM_MIGRATING_FAILED_CONNECTION, DIALOG_ACCEPT, NULL, NULL, false, "", 0, true );	// Lost connection to game
					common->Dialog().AddDialog( GDM_HOST_CONNECTION_LOST_STATS, DIALOG_ACCEPT, NULL, NULL, false, "", 0, true );
				}

				lobby.ResetAllMigrationState();

				// Make sure the sessions are joinable again
				sessionLocal->EndSessions();
			}
		}
#endif
	}
	else if( lobby.GetNumLobbyUsers() <= 1 && session->GetState() == idSession::PARTY_LOBBY )
	{
		// If they didn't come from a game, and are by themselves, just show the lobby disband msg
		common->Dialog().AddDialog( GDM_LOBBY_DISBANDED, DIALOG_ACCEPT, NULL, NULL, false, "", 0, true );				// The lobby you were previously in has disbanded

		// Make sure the sessions are joinable again
		sessionLocal->EndSessions();
	}
}

/*
========================
idSessionLocalCallbacks::GoodbyeFromHost
========================
*/
void idSessionLocalCallbacks::GoodbyeFromHost( idLobby& lobby, int peerNum, const lobbyAddress_t& remoteAddress, int msgType )
{
	sessionLocal->GoodbyeFromHost( lobby, peerNum, remoteAddress, msgType );
}

/*
========================
idSessionLocalCallbacks::AnyPeerHasAddress
========================
*/
bool idSessionLocalCallbacks::AnyPeerHasAddress( const lobbyAddress_t& remoteAddress ) const
{
	return sessionLocal->GetPartyLobby().FindAnyPeer( remoteAddress ) || sessionLocal->GetGameLobby().FindAnyPeer( remoteAddress );
}

/*
========================
idSessionLocalCallbacks::RecvLeaderboardStats
========================
*/
void idSessionLocalCallbacks::RecvLeaderboardStats( idBitMsg& msg )
{
	// Steam and PS3 just write them as they come per player, they don't need to flush
	sessionLocal->RecvLeaderboardStatsForPlayer( msg );
}

/*
========================
idSessionLocalCallbacks::ReceivedFullSnap
========================
*/
void idSessionLocalCallbacks::ReceivedFullSnap()
{
	// If we received a full snap, then we can transition into the INGAME state
	sessionLocal->numFullSnapsReceived++;

	if( sessionLocal->numFullSnapsReceived < 2 )
	{
		return;
	}

	if( sessionLocal->localState != idSessionLocal::STATE_INGAME )
	{
		sessionLocal->GetActingGameStateLobby().QueueReliableMessage( sessionLocal->GetActingGameStateLobby().host, idLobby::RELIABLE_IN_GAME );		// Let host know we are in game now
		sessionLocal->SetState( idSessionLocal::STATE_INGAME );
	}
}

/*
========================
idSessionLocalCallbacks::LeaveGameLobby
========================
*/
void idSessionLocalCallbacks::LeaveGameLobby()
{

	// Make sure we're in the game lobby
	if( session->GetState() != idSession::GAME_LOBBY )
	{
		return;
	}

	// If we're the host of the party, only we are allowed to make this call
	if( sessionLocal->GetPartyLobby().IsHost() )
	{
		return;
	}

	sessionLocal->GetGameLobby().Shutdown();
	sessionLocal->SetState( idSessionLocal::STATE_PARTY_LOBBY_PEER );
}

/*
========================
idSessionLocalCallbacks::PrePickNewHost
This is called when we have determined that we need to pick a new host.
Call PickNewHostInternal to continue on with the host picking process.
========================
*/
void idSessionLocalCallbacks::PrePickNewHost( idLobby& lobby, bool forceMe, bool inviteOldHost )
{
	sessionLocal->PrePickNewHost( lobby, forceMe, inviteOldHost );
}

/*
========================
idSessionLocalCallbacks::PreMigrateInvite
This is called just before we get invited to a migrated session
If we return false, the invite will be ignored
========================
*/
bool idSessionLocalCallbacks::PreMigrateInvite( idLobby& lobby )
{
	return sessionLocal->PreMigrateInvite( lobby );
}

/*
========================
idSessionLocalCallbacks::ConnectAndMoveToLobby
========================
*/
void idSessionLocalCallbacks::ConnectAndMoveToLobby( idLobby::lobbyType_t destLobbyType, const lobbyConnectInfo_t& connectInfo, bool waitForPartyOk )
{

	// See if we are already in the game lobby
	idLobby* lobby = sessionLocal->GetLobbyFromType( destLobbyType );

	if( lobby == NULL )
	{
		idLib::Printf( "RELIABLE_CONNECT_AND_MOVE_TO_LOBBY: Invalid lobby type.\n" );
		return;
	}

	if( lobby->lobbyBackend != NULL && lobby->lobbyBackend->IsOwnerOfConnectInfo( connectInfo ) )
	{
		idLib::Printf( "RELIABLE_CONNECT_AND_MOVE_TO_LOBBY: Already in lobby.\n" );
		return;
	}

	// See if we are in a game, or loading into a game.  If so, ignore invites from our party host
	if( destLobbyType == idLobby::TYPE_GAME || destLobbyType == idLobby::TYPE_GAME_STATE )
	{
		if( GetState() == idSession::INGAME || GetState() == idSession::LOADING )
		{
			idLib::Printf( "RELIABLE_CONNECT_AND_MOVE_TO_LOBBY: In a different game, ignoring.\n" );
			return;
		}
	}

	// End current game lobby
	lobby->Shutdown();

	// waitForPartyOk will be true if the party host wants us to wait for his ok to stay in the lobby
	lobby->waitForPartyOk = waitForPartyOk;

	// Connect to new game lobby
	sessionLocal->ConnectAndMoveToLobby( *lobby, connectInfo, true );		// Consider this an invite if party host told us to connect
}

/*
========================
idSessionLocalCallbacks::HandleServerQueryRequest
========================
*/
void idSessionLocalCallbacks::HandleServerQueryRequest( lobbyAddress_t& remoteAddr, idBitMsg& msg, int msgType )
{
	sessionLocal->HandleServerQueryRequest( remoteAddr, msg, msgType );
}

/*
========================
idSessionLocalCallbacks::HandleServerQueryAck
========================
*/
void idSessionLocalCallbacks::HandleServerQueryAck( lobbyAddress_t& remoteAddr, idBitMsg& msg )
{
	sessionLocal->HandleServerQueryAck( remoteAddr, msg );
}

extern idCVar net_headlessServer;

/*
========================
idSessionLocalCallbacks::HandlePeerMatchParamUpdate
========================
*/
void idSessionLocalCallbacks::HandlePeerMatchParamUpdate( int peer, int msg )
{
	if( net_headlessServer.GetBool() )
	{
		sessionLocal->storedPeer = peer;
		sessionLocal->storedMsgType = msg;
	}
}

/*
========================
idSessionLocalCallbacks::CreateLobbyBackend
========================
*/
idLobbyBackend* idSessionLocalCallbacks::CreateLobbyBackend( const idMatchParameters& p, float skillLevel, idLobbyBackend::lobbyBackendType_t lobbyType )
{
	return sessionLocal->CreateLobbyBackend( p, skillLevel, lobbyType );
}

/*
========================
idSessionLocalCallbacks::FindLobbyBackend
========================
*/
idLobbyBackend* idSessionLocalCallbacks::FindLobbyBackend( const idMatchParameters& p, int numPartyUsers, float skillLevel, idLobbyBackend::lobbyBackendType_t lobbyType )
{
	return sessionLocal->FindLobbyBackend( p, numPartyUsers, skillLevel, lobbyType );
}

/*
========================
idSessionLocalCallbacks::JoinFromConnectInfo
========================
*/
idLobbyBackend* idSessionLocalCallbacks::JoinFromConnectInfo( const lobbyConnectInfo_t& connectInfo , idLobbyBackend::lobbyBackendType_t lobbyType )
{
	return sessionLocal->JoinFromConnectInfo( connectInfo, lobbyType );
}

/*
========================
idSessionLocalCallbacks::DestroyLobbyBackend
========================
*/
void idSessionLocalCallbacks::DestroyLobbyBackend( idLobbyBackend* lobbyBackend )
{
	sessionLocal->DestroyLobbyBackend( lobbyBackend );
}
