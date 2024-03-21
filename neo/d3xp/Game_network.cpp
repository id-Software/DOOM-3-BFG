/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2014-2016 Robert Beckebans
Copyright (C) 2014-2016 Kot in Action Creative Artel

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

#include "precompiled.h"
#pragma hdrstop

#include "Game_local.h"
#include "../framework/Common_local.h"

static const int SNAP_GAMESTATE = 0;
static const int SNAP_SHADERPARMS = 1;
static const int SNAP_PORTALS = 2;
static const int SNAP_PLAYERSTATE = SNAP_PORTALS + 1;
static const int SNAP_PLAYERSTATE_END = SNAP_PLAYERSTATE + MAX_PLAYERS;
static const int SNAP_ENTITIES = SNAP_PLAYERSTATE_END;
static const int SNAP_ENTITIES_END = SNAP_ENTITIES + MAX_GENTITIES;
static const int SNAP_LAST_CLIENT_FRAME = SNAP_ENTITIES_END;
static const int SNAP_LAST_CLIENT_FRAME_END = SNAP_LAST_CLIENT_FRAME + MAX_PLAYERS;

/*
===============================================================================

	Client running game code:
	- entity events don't work and should not be issued
	- entities should never be spawned outside idGameLocal::ClientReadSnapshot

===============================================================================
*/

idCVar net_clientSmoothing( "net_clientSmoothing", "0.8", CVAR_GAME | CVAR_FLOAT, "smooth other clients angles and position.", 0.0f, 0.95f );
idCVar net_clientSelfSmoothing( "net_clientSelfSmoothing", "0.6", CVAR_GAME | CVAR_FLOAT, "smooth self position if network causes prediction error.", 0.0f, 0.95f );
extern idCVar net_clientMaxPrediction;

idCVar cg_predictedSpawn_debug( "cg_predictedSpawn_debug", "0", CVAR_BOOL, "Debug predictive spawning of presentables" );
idCVar g_clientFire_checkLineOfSightDebug( "g_clientFire_checkLineOfSightDebug", "0", CVAR_BOOL, "" );


/*
================
idGameLocal::InitAsyncNetwork
================
*/
void idGameLocal::InitAsyncNetwork()
{
	eventQueue.Init();
	savedEventQueue.Init();

	entityDefBits = -( idMath::BitsForInteger( declManager->GetNumDecls( DECL_ENTITYDEF ) ) + 1 );
	realClientTime = 0;
	fast.Set( 0, 0, 0 );
	slow.Set( 0, 0, 0 );
	isNewFrame = true;
	clientSmoothing = net_clientSmoothing.GetFloat();

	lastCmdRunTimeOnClient.Zero();
	lastCmdRunTimeOnServer.Zero();
	usercmdLastClientMilliseconds.Zero();
}

/*
================
idGameLocal::ShutdownAsyncNetwork
================
*/
void idGameLocal::ShutdownAsyncNetwork()
{
	eventQueue.Shutdown();
	savedEventQueue.Shutdown();
}

/*
================
idGameLocal::ServerRemapDecl
================
*/
int idGameLocal::ServerRemapDecl( int clientNum, declType_t type, int index )
{
	return index;
}

/*
================
idGameLocal::ClientRemapDecl
================
*/
int idGameLocal::ClientRemapDecl( declType_t type, int index )
{
	return index;
}

/*
================
idGameLocal::SyncPlayersWithLobbyUsers
================
*/
void idGameLocal::SyncPlayersWithLobbyUsers( bool initial )
{
	idLobbyBase& lobby = session->GetActingGameStateLobbyBase();
	if( !lobby.IsHost() )
	{
		return;
	}

	idStaticList< lobbyUserID_t, MAX_CLIENTS > newLobbyUsers;

	// First, loop over lobby users, and see if we find a lobby user that we haven't registered
	for( int i = 0; i < lobby.GetNumLobbyUsers(); i++ )
	{
		lobbyUserID_t lobbyUserID1 = lobby.GetLobbyUserIdByOrdinal( i );

		if( !lobbyUserID1.IsValid() )
		{
			continue;
		}

		if( !initial && !lobby.IsLobbyUserLoaded( lobbyUserID1 ) )
		{
			continue;
		}

		// Now, see if we find this lobby user in our list
		bool found = false;

		for( int j = 0; j < MAX_PLAYERS; j++ )
		{
			idPlayer* player = static_cast<idPlayer*>( entities[ j ] );
			if( player == NULL )
			{
				continue;
			}

			lobbyUserID_t lobbyUserID2 = lobbyUserIDs[j];

			if( lobbyUserID1 == lobbyUserID2 )
			{
				found = true;
				break;
			}
		}

		if( !found )
		{
			// If we didn't find it, we need to create a player and assign it to this new lobby user
			newLobbyUsers.Append( lobbyUserID1 );
		}
	}

	// Validate connected players
	for( int i = 0; i < MAX_PLAYERS; i++ )
	{
		idPlayer* player = static_cast<idPlayer*>( entities[ i ] );
		if( player == NULL )
		{
			continue;
		}

		lobbyUserID_t lobbyUserID = lobbyUserIDs[i];

		if( !lobby.IsLobbyUserValid( lobbyUserID ) )
		{
			delete entities[ i ];
			mpGame.DisconnectClient( i );
			lobbyUserIDs[i] = lobbyUserID_t();
			continue;
		}

		lobby.EnableSnapshotsForLobbyUser( lobbyUserID );
	}

	while( newLobbyUsers.Num() > 0 )
	{
		// Find a free player data slot to use for this new player
		int freePlayerDataIndex = -1;

		for( int i = 0; i < MAX_PLAYERS; ++i )
		{
			idPlayer* player = static_cast<idPlayer*>( entities[ i ] );
			if( player == NULL )
			{
				freePlayerDataIndex = i;
				break;
			}
		}
		if( freePlayerDataIndex == -1 )
		{
			break;			// No player data slots (this shouldn't happen)
		}
		lobbyUserID_t lobbyUserID = newLobbyUsers[0];
		newLobbyUsers.RemoveIndex( 0 );

		mpGame.ServerClientConnect( freePlayerDataIndex );
		Printf( "client %d connected.\n", freePlayerDataIndex );

		lobbyUserIDs[ freePlayerDataIndex ] = lobbyUserID;

		// Clear this player's old usercmds.
		common->ResetPlayerInput( freePlayerDataIndex );

		common->UpdateLevelLoadPacifier();


		// spawn the player
		SpawnPlayer( freePlayerDataIndex );

		common->UpdateLevelLoadPacifier();

		ServerWriteInitialReliableMessages( freePlayerDataIndex, lobbyUserID );
	}
}

/*
================
idGameLocal::ServerSendNetworkSyncCvars
================
*/
void idGameLocal::ServerSendNetworkSyncCvars()
{
	if( ( cvarSystem->GetModifiedFlags() & CVAR_NETWORKSYNC ) == 0 )
	{
		return;
	}
	cvarSystem->ClearModifiedFlags( CVAR_NETWORKSYNC );

	idBitMsg	outMsg;
	byte		msgBuf[MAX_GAME_MESSAGE_SIZE];

	idLobbyBase& lobby = session->GetActingGameStateLobbyBase();

	outMsg.InitWrite( msgBuf, sizeof( msgBuf ) );
	outMsg.BeginWriting();
	idDict syncedCvars;
	cvarSystem->MoveCVarsToDict( CVAR_NETWORKSYNC, syncedCvars, true );
	outMsg.WriteDeltaDict( syncedCvars, NULL );
	lobby.SendReliable( GAME_RELIABLE_MESSAGE_SYNCEDCVARS, outMsg, false );

	idLib::Printf( "Sending networkSync cvars:\n" );
	syncedCvars.Print();
}

/*
================
idGameLocal::ServerWriteInitialReliableMessages

  Send reliable messages to initialize the client game up to a certain initial state.
================
*/
void idGameLocal::ServerWriteInitialReliableMessages( int clientNum, lobbyUserID_t lobbyUserID )
{
	if( clientNum == GetLocalClientNum() )
	{
		// We don't need to send messages to ourself
		return;
	}

	idBitMsg	outMsg;
	byte		msgBuf[MAX_GAME_MESSAGE_SIZE];

	idLobbyBase& lobby = session->GetActingGameStateLobbyBase();

	outMsg.InitWrite( msgBuf, sizeof( msgBuf ) );
	outMsg.BeginWriting();
	idDict syncedCvars;
	cvarSystem->MoveCVarsToDict( CVAR_NETWORKSYNC, syncedCvars, true );
	outMsg.WriteDeltaDict( syncedCvars, NULL );
	lobby.SendReliableToLobbyUser( lobbyUserID, GAME_RELIABLE_MESSAGE_SYNCEDCVARS, outMsg );

	idLib::Printf( "Sending initial networkSync cvars:\n" );
	syncedCvars.Print();

	// send all saved events
	for( entityNetEvent_t* event = savedEventQueue.Start(); event; event = event->next )
	{
		outMsg.InitWrite( msgBuf, sizeof( msgBuf ) );
		outMsg.BeginWriting();
		outMsg.WriteBits( event->spawnId, 32 );
		outMsg.WriteByte( event->event );
		outMsg.WriteLong( event->time );
		outMsg.WriteBits( event->paramsSize, idMath::BitsForInteger( MAX_EVENT_PARAM_SIZE ) );
		if( event->paramsSize )
		{
			outMsg.WriteData( event->paramsBuf, event->paramsSize );
		}
		lobby.SendReliableToLobbyUser( lobbyUserID, GAME_RELIABLE_MESSAGE_EVENT, outMsg );
	}

	mpGame.ServerWriteInitialReliableMessages( clientNum, lobbyUserID );
}

/*
================
idGameLocal::SaveEntityNetworkEvent
================
*/
void idGameLocal::SaveEntityNetworkEvent( const idEntity* ent, int eventId, const idBitMsg* msg )
{
	entityNetEvent_t* event = savedEventQueue.Alloc();
	event->spawnId = GetSpawnId( ent );
	event->event = eventId;
	event->time = time;
	if( msg )
	{
		event->paramsSize = msg->GetSize();
		memcpy( event->paramsBuf, msg->GetReadData(), msg->GetSize() );
	}
	else
	{
		event->paramsSize = 0;
	}

	savedEventQueue.Enqueue( event, idEventQueue::OUTOFORDER_IGNORE );
}

/*
================
idGameLocal::ServerWriteSnapshot

  Write a snapshot of the current game state
================
*/
void idGameLocal::ServerWriteSnapshot( idSnapShot& ss )
{

	ss.SetTime( fast.time );

	byte buffer[ MAX_ENTITY_STATE_SIZE ];
	idBitMsg msg;

	// First write the generic game state to the snapshot
	msg.InitWrite( buffer, sizeof( buffer ) );
	mpGame.WriteToSnapshot( msg );
	ss.S_AddObject( SNAP_GAMESTATE, ~0U, msg, "Game State" );

	// Update global shader parameters
	msg.InitWrite( buffer, sizeof( buffer ) );
	for( int i = 0; i < MAX_GLOBAL_SHADER_PARMS; i++ )
	{
		msg.WriteFloat( globalShaderParms[i] );
	}
	ss.S_AddObject( SNAP_SHADERPARMS, ~0U, msg, "Shader Parms" );

	// update portals for opened doors
	msg.InitWrite( buffer, sizeof( buffer ) );
	int numPortals = gameRenderWorld->NumPortals();
	msg.WriteLong( numPortals );
	for( int i = 0; i < numPortals; i++ )
	{
		msg.WriteBits( gameRenderWorld->GetPortalState( ( qhandle_t )( i + 1 ) ) , NUM_RENDER_PORTAL_BITS );
	}
	ss.S_AddObject( SNAP_PORTALS, ~0U, msg, "Portal State" );

	idEntity* skyEnt = portalSkyEnt.GetEntity();
	pvsHandle_t	portalSkyPVS;
	portalSkyPVS.i = -1;
	if( skyEnt != NULL )
	{
		portalSkyPVS = pvs.SetupCurrentPVS( skyEnt->GetPVSAreas(), skyEnt->GetNumPVSAreas() );
	}

	// Build PVS data for each player and write their player state to the snapshot as well
	pvsHandle_t pvsHandles[ MAX_PLAYERS ];
	for( int i = 0; i < MAX_PLAYERS; i++ )
	{
		idPlayer* player = static_cast<idPlayer*>( entities[ i ] );
		if( player == NULL )
		{
			pvsHandles[i].i = -1;
			continue;
		}
		idPlayer* spectated = player;
		if( player->spectating && player->spectator != i && entities[ player->spectator ] )
		{
			spectated = static_cast< idPlayer* >( entities[ player->spectator ] );
		}

		msg.InitWrite( buffer, sizeof( buffer ) );
		spectated->WritePlayerStateToSnapshot( msg );
		ss.S_AddObject( SNAP_PLAYERSTATE + i, ~0U, msg, "Player State" );

		int sourceAreas[ idEntity::MAX_PVS_AREAS ];
		int numSourceAreas = gameRenderWorld->BoundsInAreas( spectated->GetPlayerPhysics()->GetAbsBounds(), sourceAreas, idEntity::MAX_PVS_AREAS );
		pvsHandles[i] = pvs.SetupCurrentPVS( sourceAreas, numSourceAreas, PVS_NORMAL );
		if( portalSkyPVS.i >= 0 )
		{
			pvsHandle_t	tempPVS = pvs.MergeCurrentPVS( pvsHandles[i], portalSkyPVS );
			pvs.FreeCurrentPVS( pvsHandles[i] );
			pvsHandles[i] = tempPVS;
		}

		// Write the last usercmd processed by the server so that clients know
		// when to stop predicting.
		msg.BeginWriting();
		msg.WriteLong( usercmdLastClientMilliseconds[i] );
		ss.S_AddObject( SNAP_LAST_CLIENT_FRAME + i, ~0U, msg, "Last client frame" );
	}

	if( portalSkyPVS.i >= 0 )
	{
		pvs.FreeCurrentPVS( portalSkyPVS );
	}

	// Add all entities to the snapshot
	for( idEntity* ent = spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() )
	{
		if( ent->GetSkipReplication() )
		{
			continue;
		}

		msg.InitWrite( buffer, sizeof( buffer ) );
		msg.WriteBits( spawnIds[ ent->entityNumber ], 32 - GENTITYNUM_BITS );
		msg.WriteBits( ent->GetType()->typeNum, idClass::GetTypeNumBits() );
		msg.WriteBits( ServerRemapDecl( -1, DECL_ENTITYDEF, ent->entityDefNumber ), entityDefBits );

		msg.WriteBits( ent->GetPredictedKey(), 32 );

		if( ent->fl.networkSync )
		{
			// write the class specific data to the snapshot
			ent->WriteToSnapshot( msg );
		}

		ss.S_AddObject( SNAP_ENTITIES + ent->entityNumber, ~0U, msg, ent->GetName() );
	}

	// Free PVS handles for all the players
	for( int i = 0; i < MAX_PLAYERS; i++ )
	{
		if( pvsHandles[i].i < 0 )
		{
			continue;
		}
		pvs.FreeCurrentPVS( pvsHandles[i] );
	}
}

/*
================
idGameLocal::NetworkEventWarning
================
*/
void idGameLocal::NetworkEventWarning( const entityNetEvent_t* event, const char* fmt, ... )
{
	char buf[1024];
	int length = 0;
	va_list argptr;

	int entityNum	= event->spawnId & ( ( 1 << GENTITYNUM_BITS ) - 1 );
	int id			= event->spawnId >> GENTITYNUM_BITS;

	length += idStr::snPrintf( buf + length, sizeof( buf ) - 1 - length, "event %d for entity %d %d: ", event->event, entityNum, id );
	va_start( argptr, fmt );
	length = idStr::vsnPrintf( buf + length, sizeof( buf ) - 1 - length, fmt, argptr );
	va_end( argptr );
	idStr::Append( buf, sizeof( buf ), "\n" );

	common->DWarning( buf );
}

/*
================
idGameLocal::ServerProcessEntityNetworkEventQueue
================
*/
void idGameLocal::ServerProcessEntityNetworkEventQueue()
{
	while( eventQueue.Start() )
	{
		entityNetEvent_t* event = eventQueue.Start();

		if( event->time > time )
		{
			break;
		}

		idEntityPtr< idEntity > entPtr;

		if( !entPtr.SetSpawnId( event->spawnId ) )
		{
			NetworkEventWarning( event, "Entity does not exist any longer, or has not been spawned yet." );
		}
		else
		{
			idEntity* ent = entPtr.GetEntity();
			assert( ent );

			idBitMsg eventMsg;
			eventMsg.InitRead( event->paramsBuf, sizeof( event->paramsBuf ) );
			eventMsg.SetSize( event->paramsSize );
			eventMsg.BeginReading();
			if( !ent->ServerReceiveEvent( event->event, event->time, eventMsg ) )
			{
				NetworkEventWarning( event, "unknown event" );
			}
		}

		entityNetEvent_t* freedEvent = eventQueue.Dequeue();
		verify( freedEvent == event );
		eventQueue.Free( event );
	}
}

/*
================
idGameLocal::ProcessReliableMessage
================
*/
void idGameLocal::ProcessReliableMessage( int clientNum, int type, const idBitMsg& msg )
{
	if( session->GetActingGameStateLobbyBase().IsPeer() )
	{
		ClientProcessReliableMessage( type, msg );
	}
	else
	{
		ServerProcessReliableMessage( clientNum, type, msg );
	}
}

/*
================
idGameLocal::ServerProcessReliableMessage
================
*/
void idGameLocal::ServerProcessReliableMessage( int clientNum, int type, const idBitMsg& msg )
{
	if( clientNum < 0 )
	{
		return;
	}
	switch( type )
	{
		case GAME_RELIABLE_MESSAGE_CHAT:
		case GAME_RELIABLE_MESSAGE_TCHAT:
		{
			char name[128];
			char text[128];

			msg.ReadString( name, sizeof( name ) );
			msg.ReadString( text, sizeof( text ) );

			mpGame.ProcessChatMessage( clientNum, type == GAME_RELIABLE_MESSAGE_TCHAT, name, text, NULL );
			break;
		}
		case GAME_RELIABLE_MESSAGE_VCHAT:
		{
			int index = msg.ReadLong();
			bool team = msg.ReadBits( 1 ) != 0;
			mpGame.ProcessVoiceChat( clientNum, team, index );
			break;
		}
		case GAME_RELIABLE_MESSAGE_DROPWEAPON:
		{
			mpGame.DropWeapon( clientNum );
			break;
		}
		case GAME_RELIABLE_MESSAGE_EVENT:
		{
			// allocate new event
			entityNetEvent_t* event = eventQueue.Alloc();
			eventQueue.Enqueue( event, idEventQueue::OUTOFORDER_DROP );

			event->spawnId = msg.ReadBits( 32 );
			event->event = msg.ReadByte();
			event->time = msg.ReadLong();

			event->paramsSize = msg.ReadBits( idMath::BitsForInteger( MAX_EVENT_PARAM_SIZE ) );
			if( event->paramsSize )
			{
				if( event->paramsSize > MAX_EVENT_PARAM_SIZE )
				{
					NetworkEventWarning( event, "invalid param size" );
					return;
				}
				msg.ReadByteAlign();
				msg.ReadData( event->paramsBuf, event->paramsSize );
			}
			break;
		}
		case GAME_RELIABLE_MESSAGE_SPECTATE:
		{
			bool spec = msg.ReadBool();
			idPlayer* player = GetClientByNum( clientNum );
			if( serverInfo.GetBool( "si_spectators" ) )
			{
				// never let spectators go back to game while sudden death is on
				if( mpGame.GetGameState() == idMultiplayerGame::SUDDENDEATH && !spec && player->wantSpectate )
				{
					// Don't allow the change
				}
				else
				{
					if( player->wantSpectate && !spec )
					{
						player->forceRespawn = true;
					}
					player->wantSpectate = spec;
				}
			}
			else
			{
				// If the server turned off si_spectators while a player is spectating, then any spectate message forces the player out of spectate mode
				if( player->wantSpectate )
				{
					player->forceRespawn = true;
				}
				player->wantSpectate = false;
			}
			break;
		}
		case GAME_RELIABLE_MESSAGE_CLIENT_HITSCAN_HIT:
		{
			const int attackerNum = msg.ReadShort();
			const int victimNum = msg.ReadShort();
			idVec3 dir;
			msg.ReadVectorFloat( dir );
			const int damageDefIndex = msg.ReadLong();
			const float damageScale = msg.ReadFloat();
			const int location = msg.ReadLong();

			if( gameLocal.entities[victimNum] == NULL )
			{
				break;
			}

			if( gameLocal.entities[attackerNum] == NULL )
			{
				break;
			}

			idPlayer& victim = static_cast< idPlayer& >( *gameLocal.entities[victimNum] );
			idPlayer& attacker = static_cast< idPlayer& >( *gameLocal.entities[attackerNum] );

			if( victim.GetPhysics() == NULL )
			{
				break;
			}

			if( attacker.weapon.GetEntity() == NULL )
			{
				break;
			}

			if( location == INVALID_JOINT )
			{
				break;
			}

			// Line of sight check. As a basic precaution against cheating,
			// the server performs a ray intersection from the client's position
			// to the joint he hit on the target.
			idVec3 muzzleOrigin;
			idMat3 muzzleAxis;

			attacker.weapon.GetEntity()->GetProjectileLaunchOriginAndAxis( muzzleOrigin, muzzleAxis );

			idVec3 targetLocation = victim.GetRenderEntity()->origin + victim.GetRenderEntity()->joints[location].ToVec3() * victim.GetRenderEntity()->axis;

			trace_t tr;
			gameLocal.clip.Translation( tr, muzzleOrigin, targetLocation, NULL, mat3_identity, MASK_SHOT_RENDERMODEL, &attacker );

			idEntity* hitEnt = gameLocal.entities[ tr.c.entityNum ];
			if( hitEnt != &victim )
			{
				break;
			}
			const idDeclEntityDef* damageDef = static_cast<const idDeclEntityDef*>( declManager->DeclByIndex( DECL_ENTITYDEF, damageDefIndex, false ) );

			if( damageDef != NULL )
			{
				victim.Damage( NULL, gameLocal.entities[attackerNum], dir, damageDef->GetName(), damageScale, location );
			}
			break;
		}
		default:
		{
			Warning( "Unknown reliable message (%d) from client %d", type, clientNum );
			break;
		}
	}
}

/*
================
idGameLocal::ClientReadSnapshot
================
*/
void idGameLocal::ClientReadSnapshot( const idSnapShot& ss )
{
	if( GetLocalClientNum() < 0 )
	{
		return;
	}

	// if prediction is off, enable local client smoothing
	//localPlayer->SetSelfSmooth( dupeUsercmds > 2 );

	// clear any debug lines from a previous frame
	gameRenderWorld->DebugClearLines( time );

	// clear any debug polygons from a previous frame
	gameRenderWorld->DebugClearPolygons( time );

	SelectTimeGroup( false );

	// so that StartSound/StopSound doesn't risk skipping
	isNewFrame = true;

	// clear the snapshot entity list
	snapshotEntities.Clear();

	// read all entities from the snapshot
	for( int o = 0; o < ss.NumObjects(); o++ )
	{
		idBitMsg msg;
		int snapObjectNum = ss.GetObjectMsgByIndex( o, msg );
		if( snapObjectNum < 0 )
		{
			assert( false );
			continue;
		}
		if( snapObjectNum == SNAP_GAMESTATE )
		{
			mpGame.ReadFromSnapshot( msg );
			continue;
		}
		if( snapObjectNum == SNAP_SHADERPARMS )
		{
			for( int i = 0; i < MAX_GLOBAL_SHADER_PARMS; i++ )
			{
				globalShaderParms[i] = msg.ReadFloat();
			}
			continue;
		}
		if( snapObjectNum == SNAP_PORTALS )
		{
			// update portals for opened doors
			int numPortals = msg.ReadLong();
			assert( numPortals == gameRenderWorld->NumPortals() );
			for( int i = 0; i < numPortals; i++ )
			{
				gameRenderWorld->SetPortalState( ( qhandle_t )( i + 1 ), msg.ReadBits( NUM_RENDER_PORTAL_BITS ) );
			}
			continue;
		}
		if( snapObjectNum >= SNAP_PLAYERSTATE && snapObjectNum < SNAP_PLAYERSTATE_END )
		{
			int playerNumber = snapObjectNum - SNAP_PLAYERSTATE;
			idPlayer* otherPlayer = static_cast< idPlayer* >( entities[ playerNumber ] );

			// Don't process Player Snapshots that are disconnected.
			const int lobbyIndex = session->GetActingGameStateLobbyBase().GetLobbyUserIndexFromLobbyUserID( lobbyUserIDs[ playerNumber ] );
			if( lobbyIndex < 0 || session->GetActingGameStateLobbyBase().IsLobbyUserConnected( lobbyIndex ) == false )
			{
				continue;
			}

			if( otherPlayer != NULL )
			{
				otherPlayer->ReadPlayerStateFromSnapshot( msg );
				if( otherPlayer != entities[ GetLocalClientNum() ] )    // This happens when we spectate another player
				{
					idWeapon* weap = otherPlayer->weapon.GetEntity();
					if( weap && ( weap->GetRenderEntity()->bounds[0] == weap->GetRenderEntity()->bounds[1] ) )
					{
						// update the weapon's viewmodel bounds so that the model doesn't flicker in the spectator's view
						weap->GetAnimator()->GetBounds( gameLocal.time, weap->GetRenderEntity()->bounds );
						weap->UpdateVisuals();
					}
				}
			}
			continue;
		}
		if( snapObjectNum >= SNAP_LAST_CLIENT_FRAME && snapObjectNum < SNAP_LAST_CLIENT_FRAME_END )
		{
			int playerNumber = snapObjectNum - SNAP_LAST_CLIENT_FRAME;

			// Don't process Player Snapshots that are disconnected.
			const int lobbyIndex = session->GetActingGameStateLobbyBase().GetLobbyUserIndexFromLobbyUserID( lobbyUserIDs[ playerNumber ] );
			if( lobbyIndex < 0 || session->GetActingGameStateLobbyBase().IsLobbyUserConnected( lobbyIndex ) == false )
			{
				continue;
			}

			usercmdLastClientMilliseconds[playerNumber] = msg.ReadLong();
			continue;
		}
		if( snapObjectNum < SNAP_ENTITIES || snapObjectNum >= SNAP_ENTITIES_END )
		{
			continue;
		}

		int entityNumber = snapObjectNum - SNAP_ENTITIES;

		if( msg.GetSize() == 0 )
		{
			delete entities[entityNumber];
			continue;
		}

		bool debug = false;

		int spawnId = msg.ReadBits( 32 - GENTITYNUM_BITS );
		int typeNum = msg.ReadBits( idClass::GetTypeNumBits() );
		int entityDefNumber = ClientRemapDecl( DECL_ENTITYDEF, msg.ReadBits( entityDefBits ) );
		const int predictedKey = msg.ReadBits( 32 );

		idTypeInfo* typeInfo = idClass::GetType( typeNum );
		if( !typeInfo )
		{
			idLib::Error( "Unknown type number %d for entity %d with class number %d", typeNum, entityNumber, entityDefNumber );
		}

		// If there is no entity on this client, but the server's entity matches a predictionKey, move the client's
		// predicted entity to the normal, replicated area in the entity list.
		if( entities[entityNumber] == NULL )
		{
			if( predictedKey != idEntity::INVALID_PREDICTION_KEY )
			{
				idLib::PrintfIf( debug, "Looking for predicted key %d.\n", predictedKey );
				idEntity* predictedEntity = FindPredictedEntity( predictedKey, typeInfo );

				if( predictedEntity != NULL )
				{
					// This presentable better be in the proper place in the list or bad things will happen if we move this presentable around
					assert( predictedEntity->GetEntityNumber() >= ENTITYNUM_FIRST_NON_REPLICATED );
					continue;
#if 0
					idProjectile* predictedProjectile = idProjectile::CastTo( predictedEntity );
					if( predictedProjectile != NULL )
					{
						for( int i = 0; i < MAX_PLAYERS; i++ )
						{
							if( entities[i] == NULL )
							{
								continue;
							}
							idPlayer* player = idPlayer::CastTo( entities[i] );
							if( player != NULL )
							{
								if( player->GetUniqueProjectile() == predictedProjectile )
								{
									// Set new spawn id
									player->TrackUniqueProjectile( predictedProjectile );
								}
							}
						}
					}

					idLib::PrintfIf( debug, "Found predicted EntNum old:%i new:%i spawnID:%i\n", predictedEntity->GetEntityNumber(), entityNumber, spawnId >> GENTITYNUM_BITS );

					// move the entity
					RemoveEntityFromHash( predictedEntity->name.c_str(), predictedEntity );
					UnregisterEntity( predictedEntity );
					assert( entities[predictedEntity->GetEntityNumber()] == NULL );
					predictedEntity->spawnArgs.SetInt( "spawn_entnum", entityNumber );
					RegisterEntity( predictedEntity, spawnId, predictedEntity->spawnArgs );
					predictedEntity->SetName( "" );

					// now mark us as no longer predicted
					predictedEntity->BecomeReplicated();
#endif
				}
				//TODO make this work with non-client preditced entities
				/* else {
					idLib::Warning( "Could not find predicted entity - key: %d. EntityIndex: %d", predictedKey, entityNum );
				} */
			}
		}

		idEntity* ent = entities[entityNumber];

		// if there is no entity or an entity of the wrong type
		if( !ent || ent->GetType()->typeNum != typeNum || ent->entityDefNumber != entityDefNumber || spawnId != spawnIds[ entityNumber ] )
		{
			delete ent;

			spawnCount = spawnId;

			if( entityNumber < MAX_CLIENTS )
			{
				commonLocal.GetUCmdMgr().ResetPlayer( entityNumber );
				SpawnPlayer( entityNumber );
				ent = entities[ entityNumber ];
				ent->FreeModelDef();
			}
			else
			{
				idDict args;
				args.SetInt( "spawn_entnum", entityNumber );
				args.Set( "name", va( "entity%d", entityNumber ) );

				if( entityDefNumber >= 0 )
				{
					if( entityDefNumber >= declManager->GetNumDecls( DECL_ENTITYDEF ) )
					{
						Error( "server has %d entityDefs instead of %d", entityDefNumber, declManager->GetNumDecls( DECL_ENTITYDEF ) );
					}
					const char* classname = declManager->DeclByIndex( DECL_ENTITYDEF, entityDefNumber, false )->GetName();
					args.Set( "classname", classname );
					if( !SpawnEntityDef( args, &ent ) || !entities[entityNumber] || entities[entityNumber]->GetType()->typeNum != typeNum )
					{
						Error( "Failed to spawn entity with classname '%s' of type '%s'", classname, typeInfo->classname );
					}
				}
				else
				{
					ent = SpawnEntityType( *typeInfo, &args, true );
					if( !entities[entityNumber] || entities[entityNumber]->GetType()->typeNum != typeNum )
					{
						Error( "Failed to spawn entity of type '%s'", typeInfo->classname );
					}
				}
				if( ent != NULL )
				{
					// Fixme: for now, force all think flags on. We'll need to figure out how we want dormancy to work on clients
					// (but for now since clientThink is so light weight, this is ok)
					ent->BecomeActive( TH_ANIMATE );
					ent->BecomeActive( TH_THINK );
					ent->BecomeActive( TH_PHYSICS );
				}
				if( entityNumber < MAX_CLIENTS && entityNumber >= numClients )
				{
					numClients = entityNumber + 1;
				}
			}
		}

		if( ss.ObjectIsStaleByIndex( o ) )
		{
			if( ent->entityNumber >= MAX_CLIENTS && ent->entityNumber < mapSpawnCount && !ent->spawnArgs.GetBool( "net_dynamic", "0" ) ) //_D3XP
			{
				// server says it's not in PVS
				// if that happens on map entities, most likely something is wrong
				// I can see that moving pieces along several PVS could be a legit situation though
				// this is a band aid, which means something is not done right elsewhere
				common->DWarning( "map entity 0x%x (%s) is stale", ent->entityNumber, ent->name.c_str() );
			}
			else
			{
				ent->snapshotStale = true;

				ent->FreeModelDef();
				// possible fix for left over lights on CTF flag
				ent->FreeLightDef();
				ent->UpdateVisuals();
				ent->GetPhysics()->UnlinkClip();
			}
		}
		else
		{
			// add the entity to the snapshot list
			ent->snapshotNode.AddToEnd( snapshotEntities );
			int snapshotChanged = ss.ObjectChangedCountByIndex( o );
			msg.SetHasChanged( ent->snapshotChanged != snapshotChanged );
			ent->snapshotChanged = snapshotChanged;

			ent->FlagNewSnapshot();

			// read the class specific data from the snapshot; SRS - only if network-synced
			if( msg.GetRemainingReadBits() > 0 && ent->fl.networkSync )
			{
				ent->ReadFromSnapshot_Ex( msg );
				ent->snapshotBits = msg.GetSize();
			}

			// Set after ReadFromSnapshot so we can detect coming unstale
			ent->snapshotStale = false;
		}
	}

	// process entity events
	ClientProcessEntityNetworkEventQueue();
}

/*
================
idGameLocal::ClientProcessEntityNetworkEventQueue
================
*/
void idGameLocal::ClientProcessEntityNetworkEventQueue()
{
	while( eventQueue.Start() )
	{
		entityNetEvent_t* event = eventQueue.Start();

		// only process forward, in order
		if( event->time >  this->serverTime )
		{
			break;
		}

		idEntityPtr< idEntity > entPtr;

		if( !entPtr.SetSpawnId( event->spawnId ) )
		{
			if( !gameLocal.entities[ event->spawnId & ( ( 1 << GENTITYNUM_BITS ) - 1 ) ] )
			{
				// if new entity exists in this position, silently ignore
				NetworkEventWarning( event, "Entity does not exist any longer, or has not been spawned yet." );
			}
		}
		else
		{
			idEntity* ent = entPtr.GetEntity();
			assert( ent );

			idBitMsg eventMsg;
			eventMsg.InitRead( event->paramsBuf, sizeof( event->paramsBuf ) );
			eventMsg.SetSize( event->paramsSize );
			eventMsg.BeginReading();
			if( !ent->ClientReceiveEvent( event->event, event->time, eventMsg ) )
			{
				NetworkEventWarning( event, "unknown event" );
			}
		}

		verify( eventQueue.Dequeue() == event );
		eventQueue.Free( event );
	}
}

/*
================
idGameLocal::ClientProcessReliableMessage
================
*/
void idGameLocal::ClientProcessReliableMessage( int type, const idBitMsg& msg )
{
	switch( type )
	{
		case GAME_RELIABLE_MESSAGE_SYNCEDCVARS:
		{
			idDict syncedCvars;
			msg.ReadDeltaDict( syncedCvars, NULL );

			idLib::Printf( "Got networkSync cvars:\n" );
			syncedCvars.Print();

			cvarSystem->ResetFlaggedVariables( CVAR_NETWORKSYNC );
			cvarSystem->SetCVarsFromDict( syncedCvars );
			break;
		}
		case GAME_RELIABLE_MESSAGE_CHAT:
		case GAME_RELIABLE_MESSAGE_TCHAT:   // (client should never get a TCHAT though)
		{
			char name[128];
			char text[128];
			msg.ReadString( name, sizeof( name ) );
			msg.ReadString( text, sizeof( text ) );
			mpGame.AddChatLine( "%s^0: %s\n", name, text );
			break;
		}
		case GAME_RELIABLE_MESSAGE_SOUND_EVENT:
		{
			snd_evt_t snd_evt = ( snd_evt_t )msg.ReadByte();
			mpGame.PlayGlobalSound( -1, snd_evt );
			break;
		}
		case GAME_RELIABLE_MESSAGE_SOUND_INDEX:
		{
			int index = gameLocal.ClientRemapDecl( DECL_SOUND, msg.ReadLong() );
			if( index >= 0 && index < declManager->GetNumDecls( DECL_SOUND ) )
			{
				const idSoundShader* shader = declManager->SoundByIndex( index );
				mpGame.PlayGlobalSound( -1, SND_COUNT, shader->GetName() );
			}
			break;
		}
		case GAME_RELIABLE_MESSAGE_DB:
		{
			idMultiplayerGame::msg_evt_t msg_evt = ( idMultiplayerGame::msg_evt_t )msg.ReadByte();
			int parm1, parm2;
			parm1 = msg.ReadByte();
			parm2 = msg.ReadByte();
			mpGame.PrintMessageEvent( msg_evt, parm1, parm2 );
			break;
		}
		case GAME_RELIABLE_MESSAGE_EVENT:
		{
			// allocate new event
			entityNetEvent_t* event = eventQueue.Alloc();
			eventQueue.Enqueue( event, idEventQueue::OUTOFORDER_IGNORE );

			event->spawnId = msg.ReadBits( 32 );
			event->event = msg.ReadByte();
			event->time = msg.ReadLong();

			event->paramsSize = msg.ReadBits( idMath::BitsForInteger( MAX_EVENT_PARAM_SIZE ) );
			if( event->paramsSize )
			{
				if( event->paramsSize > MAX_EVENT_PARAM_SIZE )
				{
					NetworkEventWarning( event, "invalid param size" );
					return;
				}
				msg.ReadByteAlign();
				msg.ReadData( event->paramsBuf, event->paramsSize );
			}
			break;
		}
		case GAME_RELIABLE_MESSAGE_RESTART:
		{
			MapRestart();
			break;
		}
		case GAME_RELIABLE_MESSAGE_TOURNEYLINE:
		{
			int line = msg.ReadByte();
			idPlayer* p = static_cast< idPlayer* >( entities[ GetLocalClientNum() ] );
			if( !p )
			{
				break;
			}
			p->tourneyLine = line;
			break;
		}
		case GAME_RELIABLE_MESSAGE_STARTSTATE:
		{
			mpGame.ClientReadStartState( msg );
			break;
		}
		case GAME_RELIABLE_MESSAGE_WARMUPTIME:
		{
			mpGame.ClientReadWarmupTime( msg );
			break;
		}
		case GAME_RELIABLE_MESSAGE_LOBBY_COUNTDOWN:
		{
			int timeRemaining = msg.ReadLong();
			Shell_UpdateClientCountdown( timeRemaining );
			break;
		}
		case GAME_RELIABLE_MESSAGE_RESPAWN_AVAILABLE:
		{
			idPlayer* p = static_cast< idPlayer* >( entities[ GetLocalClientNum() ] );
			if( p )
			{
				p->ShowRespawnHudMessage();
			}
			break;
		}
		case GAME_RELIABLE_MESSAGE_MATCH_STARTED_TIME:
		{
			mpGame.ClientReadMatchStartedTime( msg );
			break;
		}
		case GAME_RELIABLE_MESSAGE_ACHIEVEMENT_UNLOCK:
		{
			mpGame.ClientReadAchievementUnlock( msg );
			break;
		}
		default:
		{
			Error( "Unknown reliable message (%d) from host", type );
			break;
		}
	}
}

/*
================
idGameLocal::ClientRunFrame
================
*/
void idGameLocal::ClientRunFrame( idUserCmdMgr& cmdMgr, bool lastPredictFrame, gameReturn_t& ret )
{
	idEntity* ent;

	// update the game time
	previousTime = FRAME_TO_MSEC( framenum );
	framenum++;
	time = FRAME_TO_MSEC( framenum );

	idPlayer* player = static_cast<idPlayer*>( entities[GetLocalClientNum()] );
	if( !player )
	{

		// service any pending events
		idEvent::ServiceEvents();

		return;
	}

	// check for local client lag
	idLobbyBase& lobby = session->GetActingGameStateLobbyBase();
	if( lobby.GetPeerTimeSinceLastPacket( lobby.PeerIndexForHost() ) >= net_clientMaxPrediction.GetInteger() )
	{
		player->isLagged = true;
	}
	else
	{
		player->isLagged = false;
	}

	// update the real client time and the new frame flag
	if( time > realClientTime )
	{
		realClientTime = time;
		isNewFrame = true;
	}
	else
	{
		isNewFrame = false;
	}

	slow.Set( time, previousTime, realClientTime );
	fast.Set( time, previousTime, realClientTime );

	// run prediction on all active entities
	for( ent = activeEntities.Next(); ent != NULL; ent = ent->activeNode.Next() )
	{
		ent->thinkFlags |= TH_PHYSICS;

		if( ent->entityNumber != GetLocalClientNum() )
		{
			ent->ClientThink( netInterpolationInfo.serverGameMs, netInterpolationInfo.pct, true );
		}
		else
		{
			RunAllUserCmdsForPlayer( cmdMgr, ent->entityNumber );
		}
	}

	// service any pending events
	idEvent::ServiceEvents();

	// show any debug info for this frame
	if( isNewFrame )
	{
		RunDebugInfo();
		D_DrawDebugLines();
	}

	BuildReturnValue( ret );
}

/*
===============
idGameLocal::Tokenize
===============
*/
void idGameLocal::Tokenize( idStrList& out, const char* in )
{
	char buf[ MAX_STRING_CHARS ];
	char* token, *next;

	idStr::Copynz( buf, in, MAX_STRING_CHARS );
	token = buf;
	next = strchr( token, ';' );
	while( token )
	{
		if( next )
		{
			*next = '\0';
		}
		idStr::ToLower( token );
		out.Append( token );
		if( next )
		{
			token = next + 1;
			next = strchr( token, ';' );
		}
		else
		{
			token = NULL;
		}
	}
}

/*
========================
idGameLocal::FindPredictedEntity
========================
*/
idEntity*   idGameLocal::FindPredictedEntity( uint32 predictedKey, idTypeInfo* type )
{
	for( idEntity* predictedEntity = activeEntities.Next(); predictedEntity != NULL; predictedEntity = predictedEntity->activeNode.Next() )
	{
		if( !verify( predictedEntity != NULL ) )
		{
			continue;
		}
		if( !predictedEntity->IsReplicated() && predictedEntity->GetPredictedKey() == predictedKey )
		{
			if( predictedEntity->GetType() != type )
			{
				idLib::Warning( "Mismatched presentable type. Predicted: %s Actual: %s", predictedEntity->GetType()->classname, type->classname );
			}
			return predictedEntity;
		}
	}
	return NULL;
}

/*
========================
idGameLocal::GeneratePredictionKey
========================
*/
uint32  idGameLocal::GeneratePredictionKey( idWeapon* weapon, idPlayer* playerAttacker, int overrideKey )
{
	if( overrideKey != -1 )
	{
		uint32 predictedKey = overrideKey;
		int peerIndex		= -1;

		if( common->IsServer() )
		{
			peerIndex = session->GetActingGameStateLobbyBase().PeerIndexFromLobbyUser( lobbyUserIDs[ playerAttacker->entityNumber ] );
		}
		else
		{
			peerIndex = session->GetActingGameStateLobbyBase().PeerIndexOnHost();
		}

		predictedKey |= ( peerIndex << 28 );

		return predictedKey;
	}

	uint32 predictedKey = idEntity::INVALID_PREDICTION_KEY;
	int peerIndex		= -1;

	// Get key - fireCount or throwCount
	//if ( weapon != NULL ) {
	if( common->IsClient() )
	{
		predictedKey = playerAttacker->GetClientFireCount();
	}
	else
	{
		predictedKey = playerAttacker->usercmd.fireCount;
	}
	//} else {
	//	predictedKey = ( playerAttacker->GetThrowCount() );
	//}

	// Get peer index
	if( common->IsServer() )
	{
		peerIndex = session->GetActingGameStateLobbyBase().PeerIndexFromLobbyUser( lobbyUserIDs[ playerAttacker->entityNumber ] );
	}
	else
	{
		peerIndex = session->GetActingGameStateLobbyBase().PeerIndexOnHost();
	}

	if( cg_predictedSpawn_debug.GetBool() )
	{
		idLib::Printf( "GeneratePredictionKey. predictedKey: %d peedIndex: %d\n", predictedKey, peerIndex );
	}

	predictedKey |= ( peerIndex << 28 );
	return predictedKey;
}

/*
===============
idEventQueue::Alloc
===============
*/
entityNetEvent_t* idEventQueue::Alloc()
{
	entityNetEvent_t* event = eventAllocator.Alloc();
	event->prev = NULL;
	event->next = NULL;
	return event;
}

/*
===============
idEventQueue::Free
===============
*/
void idEventQueue::Free( entityNetEvent_t* event )
{
	// should only be called on an unlinked event!
	assert( !event->next && !event->prev );
	eventAllocator.Free( event );
}

/*
===============
idEventQueue::Shutdown
===============
*/
void idEventQueue::Shutdown()
{
	eventAllocator.Shutdown();
	this->Init();
}

/*
===============
idEventQueue::Init
===============
*/
void idEventQueue::Init()
{
	start = NULL;
	end = NULL;
}

/*
===============
idEventQueue::Dequeue
===============
*/
entityNetEvent_t* idEventQueue::Dequeue()
{
	entityNetEvent_t* event = start;
	if( !event )
	{
		return NULL;
	}

	start = start->next;

	if( !start )
	{
		end = NULL;
	}
	else
	{
		start->prev = NULL;
	}

	event->next = NULL;
	event->prev = NULL;

	return event;
}

/*
===============
idEventQueue::RemoveLast
===============
*/
entityNetEvent_t* idEventQueue::RemoveLast()
{
	entityNetEvent_t* event = end;
	if( !event )
	{
		return NULL;
	}

	end = event->prev;

	if( !end )
	{
		start = NULL;
	}
	else
	{
		end->next = NULL;
	}

	event->next = NULL;
	event->prev = NULL;

	return event;
}

/*
===============
idEventQueue::Enqueue
===============
*/
void idEventQueue::Enqueue( entityNetEvent_t* event, outOfOrderBehaviour_t behaviour )
{
	if( behaviour == OUTOFORDER_DROP )
	{
		// go backwards through the queue and determine if there are
		// any out-of-order events
		while( end && end->time > event->time )
		{
			entityNetEvent_t* outOfOrder = RemoveLast();
			common->DPrintf( "WARNING: new event with id %d ( time %d ) caused removal of event with id %d ( time %d ), game time = %d.\n", event->event, event->time, outOfOrder->event, outOfOrder->time, gameLocal.time );
			Free( outOfOrder );
		}
	}
	else if( behaviour == OUTOFORDER_SORT && end )
	{
		// NOT TESTED -- sorting out of order packets hasn't been
		//				 tested yet... wasn't strictly necessary for
		//				 the patch fix.
		entityNetEvent_t* cur = end;
		// iterate until we find a time < the new event's
		while( cur && cur->time > event->time )
		{
			cur = cur->prev;
		}
		if( !cur )
		{
			// add to start
			event->next = start;
			event->prev = NULL;
			start = event;
		}
		else
		{
			// insert
			event->prev = cur;
			event->next = cur->next;
			cur->next = event;
		}
		return;
	}

	// add the new event
	event->next = NULL;
	event->prev = NULL;

	if( end )
	{
		end->next = event;
		event->prev = end;
	}
	else
	{
		start = event;
	}
	end = event;
}
