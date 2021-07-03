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

idCVar net_migration_debug( "net_migration_debug", "0", CVAR_BOOL, "debug" );
idCVar net_migration_disable( "net_migration_disable", "0", CVAR_BOOL, "debug" );
idCVar net_migration_forcePeerAsHost( "net_migration_forcePeerAsHost", "-1", CVAR_INTEGER, "When set to >-1, it forces that peer number to be the new host during migration" );


/*
========================
idLobby::IsBetterHost
========================
*/
bool idLobby::IsBetterHost( int ping1, lobbyUserID_t userId1, int ping2, lobbyUserID_t userId2 )
{
	if( lobbyType == TYPE_PARTY )
	{
		return userId1 < userId2;			// Only use user id for party, since ping doesn't matter
	}

	if( ping1 < ping2 )
	{
		// Better ping wins
		return true;
	}
	else if( ping1 == ping2 && userId1 < userId2 )
	{
		// User id is tie breaker
		return true;
	}

	return false;
}

/*
========================
idLobby::FindMigrationInviteIndex
========================
*/
int idLobby::FindMigrationInviteIndex( lobbyAddress_t& address )
{
	if( migrationInfo.state == MIGRATE_NONE )
	{
		return -1;
	}

	for( int i = 0; i < migrationInfo.invites.Num(); i++ )
	{
		if( migrationInfo.invites[i].address.Compare( address, true ) )
		{
			return i;
		}
	}

	return -1;
}

/*
========================
idLobby::UpdateHostMigration
========================
*/
void idLobby::UpdateHostMigration()
{

	int time = Sys_Milliseconds();

	// If we are picking a new host, then update that
	if( migrationInfo.state == MIGRATE_PICKING_HOST )
	{
		const int MIGRATION_PICKING_HOST_TIMEOUT_IN_SECONDS = 20;		// FIXME: set back to 5 // Give other hosts 5 seconds

		if( time - migrationInfo.migrationStartTime > session->GetTitleStorageInt( "MIGRATION_PICKING_HOST_TIMEOUT_IN_SECONDS", MIGRATION_PICKING_HOST_TIMEOUT_IN_SECONDS ) * 1000 )
		{
			// Just become the host if we haven't heard from a host in awhile
			BecomeHost();
		}
		else
		{
			return;
		}
	}

	// See if we are a new migrated host that needs to invite the original members back
	if( migrationInfo.state != MIGRATE_BECOMING_HOST )
	{
		return;
	}

	if( lobbyBackend == NULL || lobbyBackend->GetState() != idLobbyBackend::STATE_READY )
	{
		return;
	}

	if( state != STATE_IDLE )
	{
		return;
	}

	if( !IsHost() )
	{
		return;
	}

	const int MIGRATION_TIMEOUT_IN_SECONDS		= 30; // FIXME: setting to 30 for dev purposes. 10 seems more reasonable. Need to make unloading game / loading lobby async
	const int MIGRATION_INVITE_TIME_IN_SECONDS	= 2;

	if( migrationInfo.invites.Num() == 0 || time - migrationInfo.migrationStartTime > session->GetTitleStorageInt( "MIGRATION_TIMEOUT_IN_SECONDS", MIGRATION_TIMEOUT_IN_SECONDS ) * 1000 )
	{
		// Either everyone acked, or we timed out, just keep who we have, and stop sending invites
		EndMigration();
		return;
	}

	// Send invites to anyone who hasn't responded
	for( int i = 0; i < migrationInfo.invites.Num(); i++ )
	{
		if( time - migrationInfo.invites[i].lastInviteTime < session->GetTitleStorageInt( "MIGRATION_INVITE_TIME_IN_SECONDS", MIGRATION_INVITE_TIME_IN_SECONDS ) * 1000 )
		{
			continue;		// Not enough time passed
		}

		// Mark the time
		migrationInfo.invites[i].lastInviteTime = time;

		byte buffer[ idPacketProcessor::MAX_PACKET_SIZE - 2 ];
		idBitMsg outmsg( buffer, sizeof( buffer ) );

		// Have lobbyBackend fill out msg with connection info
		lobbyConnectInfo_t connectInfo = lobbyBackend->GetConnectInfo();
		connectInfo.WriteToMsg( outmsg );

		// Let them know whether or not this was from in game
		outmsg.WriteBool( migrationInfo.persistUntilGameEndsData.wasMigratedGame );

		NET_VERBOSE_PRINT( "NET: Sending migration invite to %s\n", migrationInfo.invites[i].address.ToString() );

		// Send the migration invite
		SendConnectionLess( migrationInfo.invites[i].address, OOB_MIGRATE_INVITE, outmsg.GetReadData(), outmsg.GetSize() );
	}
}

/*
========================
idLobby::BuildMigrationInviteList
========================
*/
void idLobby::BuildMigrationInviteList( bool inviteOldHost )
{
	migrationInfo.invites.Clear();

	// Build a list of addresses we will send invites to (gather all unique remote addresses from the session user list)
	for( int i = 0; i < GetNumLobbyUsers(); i++ )
	{
		lobbyUser_t* user = GetLobbyUser( i );

		if( !verify( user != NULL ) )
		{
			continue;
		}

		if( user->IsDisconnected() )
		{
			continue;
		}

		if( IsSessionUserIndexLocal( i ) )
		{
			migrationInfo.ourPingMs = user->pingMs;
			migrationInfo.ourUserId = user->lobbyUserID;
			migrationInfo.persistUntilGameEndsData.ourGameData = user->migrationGameData;
			NET_VERBOSE_PRINT( "^2NET: Migration game data for local user is index %d \n", user->migrationGameData );

			continue;		// Only interested in remote users
		}

		if( !inviteOldHost && user->peerIndex == -1 )
		{
			continue;		// Don't invite old host if told not to do so
		}

		if( FindMigrationInviteIndex( user->address ) == -1 )
		{
			migrationInvite_t invite;
			invite.address			= user->address;
			invite.pingMs			= user->pingMs;
			invite.userId			= user->lobbyUserID;
			invite.migrationGameData = user->migrationGameData;
			invite.lastInviteTime	= 0;

			NET_VERBOSE_PRINT( "^2NET: Migration game data for user %s is index %d \n", user->gamertag, user->migrationGameData );

			migrationInfo.invites.Append( invite );
		}
	}
}

/*
========================
idLobby::PickNewHost
========================
*/
void idLobby::PickNewHost( bool forceMe, bool inviteOldHost )
{
	if( IsHost() )
	{
		idLib::Printf( "PickNewHost: Already host of session %s\n", GetLobbyName() );
		return;
	}

	sessionCB->PrePickNewHost( *this, forceMe, inviteOldHost );
}

/*
========================
idLobby::PickNewHostInternal
========================
*/
void idLobby::PickNewHostInternal( bool forceMe, bool inviteOldHost )
{

	if( migrationInfo.state == MIGRATE_PICKING_HOST )
	{
		return;		// Already picking new host
	}

	idLib::Printf( "PickNewHost: Started picking new host %s.\n", GetLobbyName() );

	if( IsHost() )
	{
		idLib::Printf( "PickNewHost: Already host of session %s\n", GetLobbyName() );
		return;
	}

	// Find the user with the lowest ping
	int bestUserIndex			= -1;
	int bestPingMs				= 0;
	lobbyUserID_t bestUserId;

	for( int i = 0; i < GetNumLobbyUsers(); i++ )
	{
		lobbyUser_t* user = GetLobbyUser( i );

		if( !verify( user != NULL ) )
		{
			continue;
		}

		if( user->IsDisconnected() )
		{
			continue;
		}

		if( user->peerIndex == -1 )
		{
			continue;		// Don't try and pick old host
		}

		if( bestUserIndex == -1 || IsBetterHost( user->pingMs, user->lobbyUserID, bestPingMs, bestUserId ) )
		{
			bestUserIndex	= i;
			bestPingMs		= user->pingMs;
			bestUserId		= user->lobbyUserID;
		}

		if( user->peerIndex == net_migration_forcePeerAsHost.GetInteger() )
		{
			bestUserIndex	= i;
			bestPingMs		= user->pingMs;
			bestUserId		= user->lobbyUserID;
			break;
		}
	}

	// Remember when we first started picking a new host
	migrationInfo.state						= MIGRATE_PICKING_HOST;
	migrationInfo.migrationStartTime		= Sys_Milliseconds();

	migrationInfo.persistUntilGameEndsData.wasMigratedGame = sessionCB->GetState() == idSession::INGAME;

	if( bestUserIndex == -1 )  	// This can happen if we call PickNewHost on an lobby that was Shutdown
	{
		NET_VERBOSE_PRINT( "MIGRATION: PickNewHost was called on an lobby that was Shutdown\n" );
		BecomeHost();
		return;
	}

	NET_VERBOSE_PRINT( "MIGRATION: Chose user index %d (%s) for new host\n", bestUserIndex, GetLobbyUser( bestUserIndex )->gamertag );

	bool bestWasLocal = IsSessionUserIndexLocal( bestUserIndex );		// Check before shutting down the lobby
	migrateMsgFlags = parms.matchFlags;						// Save off match parms

	// Build invite list
	BuildMigrationInviteList( inviteOldHost );

	// If the best user is on this machine, then we become the host now, otherwise, wait for a new host to contact us
	if( forceMe || bestWasLocal )
	{
		BecomeHost();
	}
}

/*
========================
idLobby::BecomeHost
========================
*/
void idLobby::BecomeHost()
{

	if( !verify( migrationInfo.state == MIGRATE_PICKING_HOST ) )
	{
		idLib::Printf( "BecomeHost: Must be called from PickNewHost.\n" );
		EndMigration();
		return;
	}

	if( IsHost() )
	{
		idLib::Printf( "BecomeHost: Already host of session.\n" );
		EndMigration();
		return;
	}

	if( !sessionCB->BecomingHost( *this ) )
	{
		EndMigration();
		return;
	}

	idLib::Printf( "BecomeHost: Sending %i invites on %s.\n", migrationInfo.invites.Num(), GetLobbyName() );

	migrationInfo.state					= MIGRATE_BECOMING_HOST;
	migrationInfo.migrationStartTime	= Sys_Milliseconds();

	if( lobbyBackend == NULL )
	{
		// If we don't have a lobbyBackend, then just create one
		Shutdown();
		StartCreating();
		return;
	}

	// Shutdown the current lobby, but keep the lobbyBackend (we'll migrate it)
	Shutdown( true );

	// Migrate the lobbyBackend to host
	lobbyBackend->BecomeHost( migrationInfo.invites.Num() );

	// Wait for it to complete
	SetState( STATE_CREATE_LOBBY_BACKEND );
}

/*
========================
idLobby::EndMigration
This gets called when we are done migrating, and invites will no longer be sent out.
========================
*/
void idLobby::EndMigration()
{
	if( migrationInfo.state == MIGRATE_NONE )
	{
		idLib::Printf( "idSessionLocal::EndMigration: Not migrating.\n" );
		return;
	}

	sessionCB->MigrationEnded( *this );

	if( lobbyBackend != NULL )
	{
		lobbyBackend->FinishBecomeHost();
	}

	migrationInfo.state = MIGRATE_NONE;
	migrationInfo.invites.Clear();
}

/*
========================
idLobby::ResetAllMigrationState
This will reset all state related to host migration. Should be called
at match end so our next game is not treated as a migrated game
========================
*/
void idLobby::ResetAllMigrationState()
{
	migrationInfo.state = MIGRATE_NONE;
	migrationInfo.invites.Clear();
	migrationInfo.persistUntilGameEndsData.Clear();

	migrateMsgFlags		= 0;

	common->Dialog().ClearDialog( GDM_MIGRATING );
	common->Dialog().ClearDialog( GDM_MIGRATING_WAITING );
	common->Dialog().ClearDialog( GDM_MIGRATING_RELAUNCHING );
}

/*
========================
idLobby::GetMigrationGameData
This will setup the passed in idBitMsg to either read or write from the global migration game data buffer
========================
*/
bool idLobby::GetMigrationGameData( idBitMsg& msg, bool reading )
{
	if( reading )
	{
		if( !IsMigratedStatsGame() || !migrationInfo.persistUntilGameEndsData.wasMigratedHost )
		{
			// This was not a migrated session, we have no migration data
			return false;
		}
		msg.InitRead( migrationInfo.persistUntilGameEndsData.gameData, sizeof( migrationInfo.persistUntilGameEndsData.gameData ) );
	}
	else
	{
		migrationInfo.persistUntilGameEndsData.hasGameData = true;
		memset( migrationInfo.persistUntilGameEndsData.gameData, 0, sizeof( migrationInfo.persistUntilGameEndsData.gameData ) );
		msg.InitWrite( migrationInfo.persistUntilGameEndsData.gameData, sizeof( migrationInfo.persistUntilGameEndsData.gameData ) );
	}

	return true;
}

/*
========================
idLobby::GetMigrationGameDataUser
This will setup the passed in idBitMsg to either read or write from the user's migration game data buffer
========================
*/
bool idLobby::GetMigrationGameDataUser( lobbyUserID_t lobbyUserID, idBitMsg& msg, bool reading )
{
	const int userNum = GetLobbyUserIndexByID( lobbyUserID );

	if( !verify( userNum >= 0 && userNum < MAX_PLAYERS ) )
	{
		return false;
	}

	lobbyUser_t* u = GetLobbyUser( userNum );
	if( u != NULL )
	{
		if( reading )
		{

			if( !IsMigratedStatsGame() || !migrationInfo.persistUntilGameEndsData.wasMigratedHost )
			{
				// This was not a migrated session, we have no migration data
				return false;
			}

			if( u->migrationGameData >= 0 && u->migrationGameData < MAX_PLAYERS )
			{
				msg.InitRead( migrationInfo.persistUntilGameEndsData.gameDataUser[ u->migrationGameData ], sizeof( migrationInfo.persistUntilGameEndsData.gameDataUser[ 0 ] ) );
			}
			else
			{
				// We don't have migration data for this user
				idLib::Warning( "No migration data for user %d in a migrated game (%d)", userNum, u->migrationGameData );
				return false;
			}
		}
		else
		{
			// Writing
			migrationInfo.persistUntilGameEndsData.hasGameData = true;
			u->migrationGameData = userNum;
			memset( migrationInfo.persistUntilGameEndsData.gameDataUser[ userNum ], 0, sizeof( migrationInfo.persistUntilGameEndsData.gameDataUser[0] ) );
			msg.InitWrite( migrationInfo.persistUntilGameEndsData.gameDataUser[ userNum ], sizeof( migrationInfo.persistUntilGameEndsData.gameDataUser[0] ) );

		}
		return true;
	}
	return false;
}

/*
========================
idLobby::HandleMigrationGameData
========================
*/
void idLobby::HandleMigrationGameData( idBitMsg& msg )
{
	// Receives game migration data from the server. Just save off the raw data. If we ever become host we'll let the game code read
	// that chunk in (we can't do anything with it now anyways: we don't have entities or any server code to read it in to)
	migrationInfo.persistUntilGameEndsData.hasGameData = true;

	// Reset each user's migration game data. If we don't receive new data for them in this msg, we don't want to use the old data
	for( int i = 0; i < GetNumLobbyUsers(); i++ )
	{
		lobbyUser_t* u = GetLobbyUser( i );
		if( u != NULL )
		{
			u->migrationGameData = -1;
		}
	}

	msg.ReadData( migrationInfo.persistUntilGameEndsData.gameData, sizeof( migrationInfo.persistUntilGameEndsData.gameData ) );
	int numUsers = msg.ReadByte();
	int dataIndex = 0;
	for( int i = 0; i < numUsers; i++ )
	{
		lobbyUserID_t lobbyUserID;
		lobbyUserID.ReadFromMsg( msg );
		lobbyUser_t* user = GetLobbyUser( GetLobbyUserIndexByID( lobbyUserID ) );
		if( user != NULL )
		{

			NET_VERBOSE_PRINT( "NET:    Got migration data[%d] for user %s\n", dataIndex, user->gamertag );

			user->migrationGameData = dataIndex;
			msg.ReadData( migrationInfo.persistUntilGameEndsData.gameDataUser[ dataIndex ], sizeof( migrationInfo.persistUntilGameEndsData.gameDataUser[ dataIndex ] ) );
			dataIndex++;
		}
	}
}

/*
========================
idLobby::SendMigrationGameData
========================
*/
void idLobby::SendMigrationGameData()
{
	if( net_migration_disable.GetBool() )
	{
		return;
	}

	if( sessionCB->GetState() != idSession::INGAME )
	{
		return;
	}

	if( !migrationInfo.persistUntilGameEndsData.hasGameData )
	{
		// Haven't been given any migration game data yet
		return;
	}

	const int now = Sys_Milliseconds();
	if( nextSendMigrationGameTime > now )
	{
		return;
	}

	byte	packetData[ idPacketProcessor::MAX_MSG_SIZE ];
	idBitMsg msg( packetData, sizeof( packetData ) );

	// Write global data
	msg.WriteData( &migrationInfo.persistUntilGameEndsData.gameData, sizeof( migrationInfo.persistUntilGameEndsData.gameData ) );
	msg.WriteByte( GetNumLobbyUsers() );

	// Write user data
	for( int userIndex = 0; userIndex < GetNumLobbyUsers(); ++userIndex )
	{
		lobbyUser_t* u = GetLobbyUser( userIndex );
		if( u->IsDisconnected() || u->migrationGameData < 0 )
		{
			continue;
		}

		u->lobbyUserID.WriteToMsg( msg );
		msg.WriteData( migrationInfo.persistUntilGameEndsData.gameDataUser[ u->migrationGameData ], sizeof( migrationInfo.persistUntilGameEndsData.gameDataUser[ u->migrationGameData ] ) );
	}

	// Send to 1 peer
	for( int i = 0; i < peers.Num(); i++ )
	{
		int peerToSend = ( nextSendMigrationGamePeer + i ) % peers.Num();

		if( peers[ peerToSend ].IsConnected() && peers[ peerToSend ].loaded )
		{
			if( peers[ peerToSend ].packetProc->NumQueuedReliables() > idPacketProcessor::MAX_RELIABLE_QUEUE / 2 )
			{
				// This is kind of a hack for development so we don't DC clients by sending them too many reliable migration messages
				// when they aren't responding. Doesn't seem like a horrible thing to have in a shipping product but is not necessary.
				NET_VERBOSE_PRINT( "NET: Skipping reliable game migration data msg because client reliable queue is > half full\n" );

			}
			else
			{
				if( net_migration_debug.GetBool() )
				{
					idLib::Printf( "NET: Sending migration game data to peer %d. size: %d\n", peerToSend, msg.GetSize() );
				}
				QueueReliableMessage( peerToSend, RELIABLE_MIGRATION_GAME_DATA, msg.GetReadData(), msg.GetSize() );
			}
			break;
		}
	}

	// Increment next send time / next send peer
	nextSendMigrationGamePeer++;
	if( nextSendMigrationGamePeer >= peers.Num() )
	{
		nextSendMigrationGamePeer = 0;
	}

	nextSendMigrationGameTime = now + MIGRATION_GAME_DATA_INTERVAL_MS;
}