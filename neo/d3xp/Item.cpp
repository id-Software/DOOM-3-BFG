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

#include "precompiled.h"
#pragma hdrstop

#include "Game_local.h"


/*
===============================================================================

  idItem

===============================================================================
*/

const idEventDef EV_DropToFloor( "<dropToFloor>" );
const idEventDef EV_RespawnItem( "respawn" );
const idEventDef EV_RespawnFx( "<respawnFx>" );
const idEventDef EV_GetPlayerPos( "<getplayerpos>" );
const idEventDef EV_HideObjective( "<hideobjective>", "e" );

CLASS_DECLARATION( idEntity, idItem )
EVENT( EV_DropToFloor,		idItem::Event_DropToFloor )
EVENT( EV_Touch,			idItem::Event_Touch )
EVENT( EV_Activate,			idItem::Event_Trigger )
EVENT( EV_RespawnItem,		idItem::Event_Respawn )
EVENT( EV_RespawnFx,		idItem::Event_RespawnFx )
END_CLASS


/*
================
idItem::idItem
================
*/
idItem::idItem() :
	clientPredictPickupMilliseconds( 0 )
{
	spin = false;
	inView = false;
	inViewTime = 0;
	lastCycle = 0;
	lastRenderViewTime = -1;
	itemShellHandle = -1;
	shellMaterial = NULL;
	orgOrigin.Zero();
	canPickUp = true;
	fl.networkSync = true;
}

/*
================
idItem::~idItem
================
*/
idItem::~idItem()
{
	// remove the highlight shell
	if( itemShellHandle != -1 )
	{
		gameRenderWorld->FreeEntityDef( itemShellHandle );
	}
}

/*
================
idItem::Save
================
*/
void idItem::Save( idSaveGame* savefile ) const
{

	savefile->WriteVec3( orgOrigin );
	savefile->WriteBool( spin );
	savefile->WriteBool( pulse );
	savefile->WriteBool( canPickUp );

	savefile->WriteMaterial( shellMaterial );

	savefile->WriteBool( inView );
	savefile->WriteInt( inViewTime );
	savefile->WriteInt( lastCycle );
	savefile->WriteInt( lastRenderViewTime );
}

/*
================
idItem::Restore
================
*/
void idItem::Restore( idRestoreGame* savefile )
{

	savefile->ReadVec3( orgOrigin );
	savefile->ReadBool( spin );
	savefile->ReadBool( pulse );
	savefile->ReadBool( canPickUp );

	savefile->ReadMaterial( shellMaterial );

	savefile->ReadBool( inView );
	savefile->ReadInt( inViewTime );
	savefile->ReadInt( lastCycle );
	savefile->ReadInt( lastRenderViewTime );

	itemShellHandle = -1;
}

/*
================
idItem::UpdateRenderEntity
================
*/
bool idItem::UpdateRenderEntity( renderEntity_s* renderEntity, const renderView_t* renderView ) const
{

	if( lastRenderViewTime == renderView->time[timeGroup] )
	{
		return false;
	}

	lastRenderViewTime = renderView->time[timeGroup];

	// check for glow highlighting if near the center of the view
	idVec3 dir = renderEntity->origin - renderView->vieworg;
	dir.Normalize();
	float d = dir * renderView->viewaxis[0];

	// two second pulse cycle
	float cycle = ( renderView->time[timeGroup] - inViewTime ) / 2000.0f;

	if( d > 0.94f )
	{
		if( !inView )
		{
			inView = true;
			if( cycle > lastCycle )
			{
				// restart at the beginning
				inViewTime = renderView->time[timeGroup];
				cycle = 0.0f;
			}
		}
	}
	else
	{
		if( inView )
		{
			inView = false;
			lastCycle = ceil( cycle );
		}
	}

	// fade down after the last pulse finishes
	if( !inView && cycle > lastCycle )
	{
		renderEntity->shaderParms[4] = 0.0f;
	}
	else
	{
		// pulse up in 1/4 second
		cycle -= ( int )cycle;
		if( cycle < 0.1f )
		{
			renderEntity->shaderParms[4] = cycle * 10.0f;
		}
		else if( cycle < 0.2f )
		{
			renderEntity->shaderParms[4] = 1.0f;
		}
		else if( cycle < 0.3f )
		{
			renderEntity->shaderParms[4] = 1.0f - ( cycle - 0.2f ) * 10.0f;
		}
		else
		{
			// stay off between pulses
			renderEntity->shaderParms[4] = 0.0f;
		}
	}

	// update every single time this is in view
	return true;
}

/*
================
idItem::ModelCallback
================
*/
bool idItem::ModelCallback( renderEntity_t* renderEntity, const renderView_t* renderView )
{
	const idItem* ent;

	// this may be triggered by a model trace or other non-view related source
	if( !renderView )
	{
		return false;
	}

	ent = static_cast<idItem*>( gameLocal.entities[ renderEntity->entityNum ] );
	if( ent == NULL )
	{
		gameLocal.Error( "idItem::ModelCallback: callback with NULL game entity" );
		return false;
	}

	return ent->UpdateRenderEntity( renderEntity, renderView );
}

/*
================
idItem::Think
================
*/
void idItem::Think()
{
	if( thinkFlags & TH_THINK )
	{
		if( spin )
		{
			idAngles	ang;
			idVec3		org;

			ang.pitch = ang.roll = 0.0f;
			ang.yaw = ( gameLocal.time & 4095 ) * 360.0f / -4096.0f;
			SetAngles( ang );

			float scale = 0.005f + entityNumber * 0.00001f;

			org = orgOrigin;
			org.z += 4.0f + cos( ( gameLocal.time + 2000 ) * scale ) * 4.0f;
			SetOrigin( org );
		}
	}

	Present();
}

/*
================
idItem::Present
================
*/
void idItem::Present()
{
	idEntity::Present();

	if( !fl.hidden && pulse )
	{
		// also add a highlight shell model
		renderEntity_t	shell;

		shell = renderEntity;

		// we will mess with shader parms when the item is in view
		// to give the "item pulse" effect
		shell.callback = idItem::ModelCallback;
		shell.entityNum = entityNumber;
		shell.customShader = shellMaterial;
		if( itemShellHandle == -1 )
		{
			itemShellHandle = gameRenderWorld->AddEntityDef( &shell );
		}
		else
		{
			gameRenderWorld->UpdateEntityDef( itemShellHandle, &shell );
		}

	}
}

/*
================
idItem::Spawn
================
*/
void idItem::Spawn()
{
	idStr		giveTo;
	idEntity* 	ent;
	float		tsize;

	if( spawnArgs.GetBool( "dropToFloor" ) )
	{
		PostEventMS( &EV_DropToFloor, 0 );
	}

	if( spawnArgs.GetFloat( "triggersize", "0", tsize ) )
	{
		GetPhysics()->GetClipModel()->LoadModel( idTraceModel( idBounds( vec3_origin ).Expand( tsize ) ) );
		GetPhysics()->GetClipModel()->Link( gameLocal.clip );
	}

	if( spawnArgs.GetBool( "start_off" ) )
	{
		GetPhysics()->SetContents( 0 );
		Hide();
	}
	else
	{
		GetPhysics()->SetContents( CONTENTS_TRIGGER );
	}

	giveTo = spawnArgs.GetString( "owner" );
	if( giveTo.Length() )
	{
		ent = gameLocal.FindEntity( giveTo );
		if( !ent )
		{
			gameLocal.Error( "Item couldn't find owner '%s'", giveTo.c_str() );
		}

		// RB: 64 bit fixes, changed NULL to 0
		PostEventMS( &EV_Touch, 0, ent, 0 );
		// RB end
	}

	// idItemTeam does not rotate and bob
	if( spawnArgs.GetBool( "spin" ) || ( common->IsMultiplayer() && !this->IsType( idItemTeam::Type ) ) )
	{
		spin = true;
		BecomeActive( TH_THINK );
	}

	//pulse = !spawnArgs.GetBool( "nopulse" );
	//temp hack for tim
	pulse = false;
	orgOrigin = GetPhysics()->GetOrigin();

	canPickUp = !( spawnArgs.GetBool( "triggerFirst" ) || spawnArgs.GetBool( "no_touch" ) );

	inViewTime = -1000;
	lastCycle = -1;
	itemShellHandle = -1;
	shellMaterial = declManager->FindMaterial( "itemHighlightShell" );
}

/*
================
idItem::GetAttributes
================
*/
void idItem::GetAttributes( idDict& attributes ) const
{
	int					i;
	const idKeyValue*	arg;

	for( i = 0; i < spawnArgs.GetNumKeyVals(); i++ )
	{
		arg = spawnArgs.GetKeyVal( i );
		if( arg->GetKey().Left( 4 ) == "inv_" )
		{
			attributes.Set( arg->GetKey().Right( arg->GetKey().Length() - 4 ), arg->GetValue() );
		}
	}
}

/*
================
idItem::GiveToPlayer
================
*/
bool idItem::GiveToPlayer( idPlayer* player, unsigned int giveFlags )
{
	if( player == NULL )
	{
		return false;
	}

	if( spawnArgs.GetBool( "inv_carry" ) )
	{
		return player->GiveInventoryItem( &spawnArgs, giveFlags );
	}

	return player->GiveItem( this, giveFlags );
}

/*
================
idItem::Pickup
================
*/
bool idItem::Pickup( idPlayer* player )
{

	const bool didGiveSucceed = GiveToPlayer( player, ITEM_GIVE_FEEDBACK );
	if( !didGiveSucceed )
	{
		return false;
	}

	// Store the time so clients know when to stop predicting and let snapshots overwrite.
	if( player->IsLocallyControlled() )
	{
		clientPredictPickupMilliseconds = gameLocal.time;
	}
	else
	{
		clientPredictPickupMilliseconds = 0;
	}

	// play pickup sound
	StartSound( "snd_acquire", SND_CHANNEL_ITEM, 0, false, NULL );

	// clear our contents so the object isn't picked up twice
	GetPhysics()->SetContents( 0 );

	// hide the model
	Hide();

	// remove the highlight shell
	if( itemShellHandle != -1 )
	{
		gameRenderWorld->FreeEntityDef( itemShellHandle );
		itemShellHandle = -1;
	}

	// Clients need to bail out after some feedback, but
	// before actually changing any values. The values
	// will be updated in the next snapshot.
	if( common->IsClient() )
	{
		return didGiveSucceed;
	}

	if( !GiveToPlayer( player, ITEM_GIVE_UPDATE_STATE ) )
	{
		return false;
	}

	// trigger our targets
	ActivateTargets( player );

	float respawn = spawnArgs.GetFloat( "respawn" );
	bool dropped = spawnArgs.GetBool( "dropped" );
	bool no_respawn = spawnArgs.GetBool( "no_respawn" );

	if( common->IsMultiplayer() && respawn == 0.0f )
	{
		respawn = 20.0f;
	}

	if( respawn && !dropped && !no_respawn )
	{
		const char* sfx = spawnArgs.GetString( "fxRespawn" );
		if( sfx != NULL && *sfx != '\0' )
		{
			PostEventSec( &EV_RespawnFx, respawn - 0.5f );
		}
		PostEventSec( &EV_RespawnItem, respawn );
	}
	else if( !spawnArgs.GetBool( "inv_objective" ) && !no_respawn )
	{
		// give some time for the pickup sound to play
		// FIXME: Play on the owner
		if( !spawnArgs.GetBool( "inv_carry" ) )
		{
			PostEventMS( &EV_Remove, 5000 );
		}
	}

	BecomeInactive( TH_THINK );
	return true;
}

/*
================
idItem::ClientThink
================
*/
void idItem::ClientThink( const int curTime, const float fraction, const bool predict )
{

	// only think forward because the state is not synced through snapshots
	if( !gameLocal.isNewFrame )
	{
		return;
	}
	Think();
}


/*
================
idItem::ClientPredictionThink
================
*/
void idItem::ClientPredictionThink()
{
	// only think forward because the state is not synced through snapshots
	if( !gameLocal.isNewFrame )
	{
		return;
	}
	Think();
}

/*
================
idItem::WriteFromSnapshot
================
*/
void idItem::WriteToSnapshot( idBitMsg& msg ) const
{
	msg.WriteBits( IsHidden(), 1 );
}

/*
================
idItem::ReadFromSnapshot
================
*/
void idItem::ReadFromSnapshot( const idBitMsg& msg )
{
	if( msg.ReadBits( 1 ) )
	{
		Hide();
	}
	else if( clientPredictPickupMilliseconds != 0 )
	{
		// Fix mispredictions
		if( gameLocal.GetLastClientUsercmdMilliseconds( gameLocal.GetLocalClientNum() ) >= clientPredictPickupMilliseconds )
		{
			if( GetPhysics()->GetContents() == 0 )
			{
				GetPhysics()->SetContents( CONTENTS_TRIGGER );
			}
			Show();
		}
	}
}

/*
================
idItem::ClientReceiveEvent
================
*/
bool idItem::ClientReceiveEvent( int event, int time, const idBitMsg& msg )
{

	switch( event )
	{
		case EVENT_RESPAWN:
		{
			Event_Respawn();
			return true;
		}
		case EVENT_RESPAWNFX:
		{
			Event_RespawnFx();
			return true;
		}
		default:
		{
			return idEntity::ClientReceiveEvent( event, time, msg );
		}
	}
}

/*
================
idItem::Event_DropToFloor
================
*/
void idItem::Event_DropToFloor()
{
	trace_t trace;

	// don't drop the floor if bound to another entity
	if( GetBindMaster() != NULL && GetBindMaster() != this )
	{
		return;
	}

	gameLocal.clip.TraceBounds( trace, renderEntity.origin, renderEntity.origin - idVec3( 0, 0, 64 ), renderEntity.bounds, MASK_SOLID | CONTENTS_CORPSE, this );
	SetOrigin( trace.endpos );
}

/*
================
idItem::Event_Touch
================
*/
void idItem::Event_Touch( idEntity* other, trace_t* trace )
{
	if( !other->IsType( idPlayer::Type ) )
	{
		return;
	}

	if( !canPickUp )
	{
		return;
	}

	Pickup( static_cast<idPlayer*>( other ) );
}

/*
================
idItem::Event_Trigger
================
*/
void idItem::Event_Trigger( idEntity* activator )
{

	if( !canPickUp && spawnArgs.GetBool( "triggerFirst" ) )
	{
		canPickUp = true;
		return;
	}

	if( activator && activator->IsType( idPlayer::Type ) )
	{
		Pickup( static_cast<idPlayer*>( activator ) );
	}
}

/*
================
idItem::Event_Respawn
================
*/
void idItem::Event_Respawn()
{
	if( common->IsServer() )
	{
		ServerSendEvent( EVENT_RESPAWN, NULL, false );
	}
	BecomeActive( TH_THINK );
	Show();
	inViewTime = -1000;
	lastCycle = -1;
	GetPhysics()->SetContents( CONTENTS_TRIGGER );
	SetOrigin( orgOrigin );
	StartSound( "snd_respawn", SND_CHANNEL_ITEM, 0, false, NULL );
	CancelEvents( &EV_RespawnItem ); // don't double respawn
}

/*
================
idItem::Event_RespawnFx
================
*/
void idItem::Event_RespawnFx()
{
	if( common->IsServer() )
	{
		ServerSendEvent( EVENT_RESPAWNFX, NULL, false );
	}
	const char* sfx = spawnArgs.GetString( "fxRespawn" );
	if( sfx != NULL && *sfx != '\0' )
	{
		idEntityFx::StartFx( sfx, NULL, NULL, this, true );
	}
}

/*
===============================================================================

  idItemPowerup

===============================================================================
*/

/*
===============
idItemPowerup
===============
*/

CLASS_DECLARATION( idItem, idItemPowerup )
END_CLASS

/*
================
idItemPowerup::idItemPowerup
================
*/
idItemPowerup::idItemPowerup()
{
	time = 0;
	type = 0;
}

/*
================
idItemPowerup::Save
================
*/
void idItemPowerup::Save( idSaveGame* savefile ) const
{
	savefile->WriteInt( time );
	savefile->WriteInt( type );
}

/*
================
idItemPowerup::Restore
================
*/
void idItemPowerup::Restore( idRestoreGame* savefile )
{
	savefile->ReadInt( time );
	savefile->ReadInt( type );
}

/*
================
idItemPowerup::Spawn
================
*/
void idItemPowerup::Spawn()
{
	time = spawnArgs.GetInt( "time", "30" );
	type = spawnArgs.GetInt( "type", "0" );
}

/*
================
idItemPowerup::GiveToPlayer
================
*/
bool idItemPowerup::GiveToPlayer( idPlayer* player, unsigned int giveFlags )
{
	if( player->spectating )
	{
		return false;
	}
	player->GivePowerUp( type, time * 1000, giveFlags );
	return true;
}

/*
===============================================================================

  idItemTeam

  Used for flags in Capture the Flag

===============================================================================
*/

// temporarely removed these events

const idEventDef EV_FlagReturn( "flagreturn", "e" );
const idEventDef EV_TakeFlag( "takeflag", "e" );
const idEventDef EV_DropFlag( "dropflag", "d" );
const idEventDef EV_FlagCapture( "flagcapture" );

CLASS_DECLARATION( idItem, idItemTeam )
EVENT( EV_FlagReturn,  idItemTeam::Event_FlagReturn )
EVENT( EV_TakeFlag,    idItemTeam::Event_TakeFlag )
EVENT( EV_DropFlag,    idItemTeam::Event_DropFlag )
EVENT( EV_FlagCapture, idItemTeam::Event_FlagCapture )
END_CLASS

/*
===============
idItemTeam::idItemTeam
===============
*/
idItemTeam::idItemTeam()
{
	team		   = -1;
	carried		   = false;
	dropped		   = false;
	lastDrop	   = 0;

	itemGlowHandle = -1;

	skinDefault	= NULL;
	skinCarried	= NULL;

	scriptTaken		= NULL;
	scriptDropped	= NULL;
	scriptReturned	= NULL;
	scriptCaptured	= NULL;

	lastNuggetDrop	= 0;
	nuggetName		= 0;
}

/*
===============
idItemTeam::~idItemTeam
===============
*/
idItemTeam::~idItemTeam()
{
	FreeLightDef();
}
/*
===============
idItemTeam::Spawn
===============
*/
void idItemTeam::Spawn()
{
	team					= spawnArgs.GetInt( "team" );
	returnOrigin			= GetPhysics()->GetOrigin() + idVec3( 0, 0, 20 );
	returnAxis				= GetPhysics()->GetAxis();

	BecomeActive( TH_THINK );

	const char* skinName;
	skinName = spawnArgs.GetString( "skin", "" );
	if( skinName[0] )
	{
		skinDefault = declManager->FindSkin( skinName );
	}

	skinName = spawnArgs.GetString( "skin_carried", "" );
	if( skinName[0] )
	{
		skinCarried = declManager->FindSkin( skinName );
	}

	nuggetName = spawnArgs.GetString( "nugget_name", "" );
	if( !nuggetName[0] )
	{
		nuggetName = NULL;
	}

	scriptTaken		= LoadScript( "script_taken" );
	scriptDropped	= LoadScript( "script_dropped" );
	scriptReturned	= LoadScript( "script_returned" );
	scriptCaptured	= LoadScript( "script_captured" );

	/* Spawn attached dlight */
	/*
	idDict args;
	idVec3 lightOffset( 0.0f, 20.0f, 0.0f );

	// Set up the flag's dynamic light
	memset( &itemGlow, 0, sizeof( itemGlow ) );
	itemGlow.axis = mat3_identity;
	itemGlow.lightRadius.x = 128.0f;
	itemGlow.lightRadius.y = itemGlow.lightRadius.z = itemGlow.lightRadius.x;
	itemGlow.noShadows  = true;
	itemGlow.pointLight = true;
	itemGlow.shaderParms[ SHADERPARM_RED ] = 0.0f;
	itemGlow.shaderParms[ SHADERPARM_GREEN ] = 0.0f;
	itemGlow.shaderParms[ SHADERPARM_BLUE ] = 0.0f;
	itemGlow.shaderParms[ SHADERPARM_ALPHA ] = 0.0f;

	// Select a shader based on the team
	if ( team == 0 )
		itemGlow.shader = declManager->FindMaterial( "lights/redflag" );
	else
		itemGlow.shader = declManager->FindMaterial( "lights/blueflag" );
	*/

	idMoveableItem::Spawn();

	physicsObj.SetContents( 0 );
	physicsObj.SetClipMask( MASK_SOLID | CONTENTS_MOVEABLECLIP );
	physicsObj.SetGravity( idVec3( 0, 0, spawnArgs.GetInt( "gravity", "-30" ) ) );
}


/*
===============
idItemTeam::LoadScript
===============
*/
function_t* idItemTeam::LoadScript( const char* script )
{
	function_t* function = NULL;
	idStr funcname = spawnArgs.GetString( script, "" );
	if( funcname.Length() )
	{
		function = gameLocal.program.FindFunction( funcname );
		if( function == NULL )
		{
#ifdef _DEBUG
			gameLocal.Warning( "idItemTeam '%s' at (%s) calls unknown function '%s'", name.c_str(), GetPhysics()->GetOrigin().ToString( 0 ), funcname.c_str() );
#endif
		}
	}
	return function;
}


/*
===============
idItemTeam::Think
===============
*/
void idItemTeam::Think()
{
	idMoveableItem::Think();

	TouchTriggers();

	// TODO : only update on updatevisuals
	/*idVec3 offset( 0.0f, 0.0f, 20.0f );
	itemGlow.origin = GetPhysics()->GetOrigin() + offset;
	if ( itemGlowHandle == -1 ) {
		itemGlowHandle = gameRenderWorld->AddLightDef( &itemGlow );
	} else {
		gameRenderWorld->UpdateLightDef( itemGlowHandle, &itemGlow );
	}*/

#if 1
	// should only the server do this?
	if( common->IsServer() && nuggetName && carried && ( !lastNuggetDrop || ( gameLocal.time - lastNuggetDrop ) >  spawnArgs.GetInt( "nugget_frequency" ) ) )
	{

		SpawnNugget( GetPhysics()->GetOrigin() );
		lastNuggetDrop = gameLocal.time;
	}
#endif

	// return dropped flag after si_flagDropTimeLimit seconds
	if( dropped && !carried && lastDrop != 0 && ( gameLocal.time - lastDrop ) > ( si_flagDropTimeLimit.GetInteger() * 1000 ) )
	{

		Return();	// return flag after 30 seconds on ground
		return;
	}
}

/*
===============
idItemTeam::Pickup
===============
*/
bool idItemTeam::Pickup( idPlayer* player )
{
	if( !gameLocal.mpGame.IsGametypeFlagBased() )  /* CTF */
	{
		return false;
	}

	if( gameLocal.mpGame.GetGameState() == idMultiplayerGame::WARMUP ||
			gameLocal.mpGame.GetGameState() == idMultiplayerGame::COUNTDOWN )
	{
		return false;
	}

	// wait 2 seconds after drop before beeing picked up again
	if( lastDrop != 0 && ( gameLocal.time - lastDrop ) < spawnArgs.GetInt( "pickupDelay", "500" ) )
	{
		return false;
	}

	if( carried == false && player->team != this->team )
	{

		PostEventMS( &EV_TakeFlag, 0, player );

		return true;
	}
	else if( carried == false && dropped == true && player->team == this->team )
	{

		gameLocal.mpGame.PlayerScoreCTF( player->entityNumber, 5 );

		// return flag
		PostEventMS( &EV_FlagReturn, 0, player );

		return false;
	}

	return false;
}

/*
===============
idItemTeam::ClientReceiveEvent
===============
*/
bool idItemTeam::ClientReceiveEvent( int event, int time, const idBitMsg& msg )
{
	gameLocal.DPrintf( "ClientRecieveEvent: %i\n", event );

	switch( event )
	{
		case EVENT_TAKEFLAG:
		{
			idPlayer* player = static_cast<idPlayer*>( gameLocal.entities[ msg.ReadBits( GENTITYNUM_BITS ) ] );
			if( player == NULL )
			{
				gameLocal.Warning( "NULL player takes flag?\n" );
				return false;
			}

			Event_TakeFlag( player );
		}
		return true;

		case EVENT_DROPFLAG :
		{
			bool death = bool( msg.ReadBits( 1 ) == 1 );
			Event_DropFlag( death );
		}
		return true;

		case EVENT_FLAGRETURN :
		{
			Hide();

			FreeModelDef();
			FreeLightDef();

			Event_FlagReturn();
		}
		return true;

		case EVENT_FLAGCAPTURE :
		{
			Hide();

			FreeModelDef();
			FreeLightDef();

			Event_FlagCapture();
		}
		return true;
	};

	return false;
}

/*
================
idItemTeam::Drop
================
*/
void idItemTeam::Drop( bool death )
{
//	PostEventMS( &EV_DropFlag, 0, int(death == true) );
// had to remove the delayed drop because of drop flag on disconnect
	Event_DropFlag( death );
}

/*
================
idItemTeam::Return
================
*/
void idItemTeam::Return( idPlayer* player )
{
	if( team != 0 && team != 1 )
	{
		return;
	}

//	PostEventMS( &EV_FlagReturn, 0 );
	Event_FlagReturn();
}

/*
================
idItemTeam::Capture
================
*/
void idItemTeam::Capture()
{
	if( team != 0 && team != 1 )
	{
		return;
	}

	PostEventMS( &EV_FlagCapture, 0 );
}

/*
================
idItemTeam::PrivateReturn
================
*/
void idItemTeam::PrivateReturn()
{
	Unbind();

	if( common->IsServer() && carried && !dropped )
	{
		int playerIdx = gameLocal.mpGame.GetFlagCarrier( 1 - team );
		if( playerIdx != -1 )
		{
			idPlayer* player = static_cast<idPlayer*>( gameLocal.entities[ playerIdx ] );
			player->carryingFlag = false;
		}
		else
		{
			gameLocal.Warning( "BUG: carried flag has no carrier before return" );
		}
	}

	dropped = false;
	carried = false;

	SetOrigin( returnOrigin );
	SetAxis( returnAxis );

	trigger->Link( gameLocal.clip, this, 0, GetPhysics()->GetOrigin(), mat3_identity );

	SetSkin( skinDefault );

	// Turn off the light
	/*itemGlow.shaderParms[ SHADERPARM_RED ] = 0.0f;
	itemGlow.shaderParms[ SHADERPARM_GREEN ] = 0.0f;
	itemGlow.shaderParms[ SHADERPARM_BLUE ] = 0.0f;
	itemGlow.shaderParms[ SHADERPARM_ALPHA ] = 0.0f;

	if ( itemGlowHandle != -1 )
		gameRenderWorld->UpdateLightDef( itemGlowHandle, &itemGlow );*/

	GetPhysics()->SetLinearVelocity( idVec3( 0, 0, 0 ) );
	GetPhysics()->SetAngularVelocity( idVec3( 0, 0, 0 ) );
}

/*
================
idItemTeam::Event_TakeFlag
================
*/
void idItemTeam::Event_TakeFlag( idPlayer* player )
{
	gameLocal.DPrintf( "Event_TakeFlag()!\n" );

	assert( player != NULL );

	if( player->carryingFlag )
	{
		// Don't do anything if the player is already carrying the flag.
		// Prevents duplicate messages.
		return;
	}

	if( common->IsServer() )
	{
		idBitMsg msg;
		byte msgBuf[MAX_EVENT_PARAM_SIZE];
		// Send the event
		msg.InitWrite( msgBuf, sizeof( msgBuf ) );
		msg.BeginWriting();
		msg.WriteBits( player->entityNumber, GENTITYNUM_BITS );
		ServerSendEvent( EVENT_TAKEFLAG, &msg, false );

		gameLocal.mpGame.PlayTeamSound( player->team, SND_FLAG_TAKEN_THEIRS );
		gameLocal.mpGame.PlayTeamSound( team, SND_FLAG_TAKEN_YOURS );

		gameLocal.mpGame.PrintMessageEvent( idMultiplayerGame::MSG_FLAGTAKEN, team, player->entityNumber );

		// dont drop a nugget RIGHT away
		lastNuggetDrop = gameLocal.time - gameLocal.random.RandomInt( 1000 );

	}

	BindToJoint( player, g_flagAttachJoint.GetString(), true );
	idVec3 origin( g_flagAttachOffsetX.GetFloat(), g_flagAttachOffsetY.GetFloat(), g_flagAttachOffsetZ.GetFloat() );
	idAngles angle( g_flagAttachAngleX.GetFloat(), g_flagAttachAngleY.GetFloat(), g_flagAttachAngleZ.GetFloat() );
	SetAngles( angle );
	SetOrigin( origin );

	// Turn the light on
	/*itemGlow.shaderParms[ SHADERPARM_RED ] = 1.0f;
	itemGlow.shaderParms[ SHADERPARM_GREEN ] = 1.0f;
	itemGlow.shaderParms[ SHADERPARM_BLUE ] = 1.0f;
	itemGlow.shaderParms[ SHADERPARM_ALPHA ] = 1.0f;

	if ( itemGlowHandle != -1 )
		gameRenderWorld->UpdateLightDef( itemGlowHandle, &itemGlow );*/

	if( scriptTaken )
	{
		idThread* thread = new idThread();
		thread->CallFunction( scriptTaken, false );
		thread->DelayedStart( 0 );
	}

	dropped = false;
	carried = true;
	player->carryingFlag = true;

	SetSkin( skinCarried );

	UpdateVisuals();
	UpdateGuis();

	if( common->IsServer() )
	{
		if( team == 0 )
		{
			gameLocal.mpGame.player_red_flag = player->entityNumber;
		}
		else
		{
			gameLocal.mpGame.player_blue_flag = player->entityNumber;
		}
	}
}

/*
================
idItemTeam::Event_DropFlag
================
*/
void idItemTeam::Event_DropFlag( bool death )
{
	gameLocal.DPrintf( "Event_DropFlag()!\n" );

	if( common->IsServer() )
	{
		idBitMsg msg;
		byte msgBuf[MAX_EVENT_PARAM_SIZE];
		// Send the event
		msg.InitWrite( msgBuf, sizeof( msgBuf ) );
		msg.BeginWriting();
		msg.WriteBits( death, 1 );
		ServerSendEvent( EVENT_DROPFLAG, &msg, false );

		if( gameLocal.mpGame.IsFlagMsgOn() )
		{
			gameLocal.mpGame.PlayTeamSound( 1 - team,	SND_FLAG_DROPPED_THEIRS );
			gameLocal.mpGame.PlayTeamSound( team,	SND_FLAG_DROPPED_YOURS );

			gameLocal.mpGame.PrintMessageEvent( idMultiplayerGame::MSG_FLAGDROP, team );
		}
	}

	lastDrop = gameLocal.time;

	BecomeActive( TH_THINK );
	Show();

	if( death )
	{
		GetPhysics()->SetLinearVelocity( idVec3( 0, 0, 0 ) );
	}
	else
	{
		GetPhysics()->SetLinearVelocity( idVec3( 0, 0, 20 ) );
	}

	GetPhysics()->SetAngularVelocity( idVec3( 0, 0, 0 ) );

//	GetPhysics()->SetLinearVelocity( ( GetPhysics()->GetLinearVelocity() * GetBindMaster()->GetPhysics()->GetAxis() ) + GetBindMaster()->GetPhysics()->GetLinearVelocity() );

	if( GetBindMaster() )
	{
		const idBounds bounds = GetPhysics()->GetBounds();
		idVec3 origin = GetBindMaster()->GetPhysics()->GetOrigin() + idVec3( 0, 0, ( bounds[1].z - bounds[0].z ) * 0.6f );

		Unbind();

		SetOrigin( origin );
	}

	idAngles angle = GetPhysics()->GetAxis().ToAngles();
	angle.roll	= 0;
	angle.pitch = 0;
	SetAxis( angle.ToMat3() );

	dropped = true;
	carried = false;

	if( scriptDropped )
	{
		idThread* thread = new idThread();
		thread->CallFunction( scriptDropped, false );
		thread->DelayedStart( 0 );
	}

	SetSkin( skinDefault );
	UpdateVisuals();
	UpdateGuis();


	if( common->IsServer() )
	{
		if( team == 0 )
		{
			gameLocal.mpGame.player_red_flag = -1;
		}
		else
		{
			gameLocal.mpGame.player_blue_flag = -1;
		}

	}
}

/*
================
idItemTeam::Event_FlagReturn
================
*/
void idItemTeam::Event_FlagReturn( idPlayer* player )
{
	gameLocal.DPrintf( "Event_FlagReturn()!\n" );

	if( common->IsServer() )
	{
		ServerSendEvent( EVENT_FLAGRETURN, NULL, false );

		if( gameLocal.mpGame.IsFlagMsgOn() )
		{
			gameLocal.mpGame.PlayTeamSound( 1 - team,	SND_FLAG_RETURN );
			gameLocal.mpGame.PlayTeamSound( team,	SND_FLAG_RETURN );

			int entitynum = 255;
			if( player )
			{
				entitynum = player->entityNumber;
			}

			gameLocal.mpGame.PrintMessageEvent( idMultiplayerGame::MSG_FLAGRETURN, team, entitynum );
		}
	}

	BecomeActive( TH_THINK );
	Show();

	PrivateReturn();

	if( scriptReturned )
	{
		idThread* thread = new idThread();
		thread->CallFunction( scriptReturned, false );
		thread->DelayedStart( 0 );
	}

	UpdateVisuals();
	UpdateGuis();
//	Present();

	if( common->IsServer() )
	{
		if( team == 0 )
		{
			gameLocal.mpGame.player_red_flag = -1;
		}
		else
		{
			gameLocal.mpGame.player_blue_flag = -1;
		}
	}
}

/*
================
idItemTeam::Event_FlagCapture
================
*/
void idItemTeam::Event_FlagCapture()
{
	gameLocal.DPrintf( "Event_FlagCapture()!\n" );

	if( common->IsServer() )
	{
		int playerIdx = gameLocal.mpGame.GetFlagCarrier( 1 - team );
		if( playerIdx != -1 )
		{
			ServerSendEvent( EVENT_FLAGCAPTURE, NULL, false );

			gameLocal.mpGame.PlayTeamSound( 1 - team,	SND_FLAG_CAPTURED_THEIRS );
			gameLocal.mpGame.PlayTeamSound( team,	SND_FLAG_CAPTURED_YOURS );

			gameLocal.mpGame.TeamScoreCTF( 1 - team, 1 );

			gameLocal.mpGame.PlayerScoreCTF( playerIdx, 10 );

			gameLocal.mpGame.PrintMessageEvent( idMultiplayerGame::MSG_FLAGCAPTURE, team, playerIdx );
		}
		else
		{
			playerIdx = 255;
		}
	}

	BecomeActive( TH_THINK );
	Show();

	PrivateReturn();

	if( scriptCaptured )
	{
		idThread* thread = new idThread();
		thread->CallFunction( scriptCaptured, false );
		thread->DelayedStart( 0 );
	}

	UpdateVisuals();
	UpdateGuis();


	if( common->IsServer() )
	{
		if( team == 0 )
		{
			gameLocal.mpGame.player_red_flag = -1;
		}
		else
		{
			gameLocal.mpGame.player_blue_flag = -1;
		}
	}

}

/*
================
idItemTeam::FreeLightDef
================
*/
void idItemTeam::FreeLightDef()
{
	if( itemGlowHandle != -1 )
	{
		gameRenderWorld->FreeLightDef( itemGlowHandle );
		itemGlowHandle = -1;
	}
}

/*
================
idItemTeam::SpawnNugget
================
*/
void idItemTeam::SpawnNugget( idVec3 pos )
{

	idAngles angle( gameLocal.random.RandomInt( spawnArgs.GetInt( "nugget_pitch", "30" ) ),	gameLocal.random.RandomInt( spawnArgs.GetInt( "nugget_yaw", "360" ) ),	0 );
	float velocity = float( gameLocal.random.RandomInt( 40 ) + 15 );

	velocity *= spawnArgs.GetFloat( "nugget_velocity", "1" );

	idEntity* ent = idMoveableItem::DropItem( nuggetName, pos, GetPhysics()->GetAxis(), angle.ToMat3() * idVec3( velocity, velocity, velocity ), 0, spawnArgs.GetInt( "nugget_removedelay" ) );
	idPhysics_RigidBody* physics = static_cast<idPhysics_RigidBody*>( ent->GetPhysics() );

	if( physics != NULL && physics->IsType( idPhysics_RigidBody::Type ) )
	{
		physics->DisableImpact();
	}
}



/*
================
idItemTeam::Event_FlagCapture
================
*/
void idItemTeam::WriteToSnapshot( idBitMsg& msg ) const
{
	msg.WriteBits( carried, 1 );
	msg.WriteBits( dropped, 1 );

	WriteBindToSnapshot( msg );

	idMoveableItem::WriteToSnapshot( msg );
}


/*
================
idItemTeam::ReadFromSnapshot
================
*/
void idItemTeam::ReadFromSnapshot( const idBitMsg& msg )
{
	carried = msg.ReadBits( 1 ) == 1;
	dropped = msg.ReadBits( 1 ) == 1;

	ReadBindFromSnapshot( msg );

	if( msg.HasChanged() )
	{
		UpdateGuis();

		if( carried == true )
		{
			SetSkin( skinCarried );
		}
		else
		{
			SetSkin( skinDefault );
		}
	}

	idMoveableItem::ReadFromSnapshot( msg );
}

/*
================
idItemTeam::UpdateGuis

Update all client's huds wrt the flag status.
================
*/
void idItemTeam::UpdateGuis()
{
	idPlayer* player;

	for( int i = 0; i < gameLocal.numClients; i++ )
	{
		player = static_cast<idPlayer*>( gameLocal.entities[ i ] );

		if( player && player->hud )
		{

			player->hud->SetFlagState( 0, gameLocal.mpGame.GetFlagStatus( 0 ) );
			player->hud->SetFlagState( 1, gameLocal.mpGame.GetFlagStatus( 1 ) );

			player->hud->SetTeamScore( 0, gameLocal.mpGame.GetFlagPoints( 0 ) );
			player->hud->SetTeamScore( 1, gameLocal.mpGame.GetFlagPoints( 1 ) );
		}
	}
}

/*
================
idItemTeam::Present
================
*/
void idItemTeam::Present()
{
	// hide the flag for localplayer if in first person
	if( carried && GetBindMaster() )
	{
		idPlayer* player = static_cast<idPlayer*>( GetBindMaster() );
		if( player == gameLocal.GetLocalPlayer() && !pm_thirdPerson.GetBool() )
		{
			FreeModelDef();
			BecomeActive( TH_UPDATEVISUALS );
			return;
		}
	}

	idEntity::Present();
}

/*
===============================================================================

  idObjective

===============================================================================
*/

CLASS_DECLARATION( idItem, idObjective )
EVENT( EV_Activate,			idObjective::Event_Trigger )
EVENT( EV_HideObjective,	idObjective::Event_HideObjective )
EVENT( EV_GetPlayerPos,		idObjective::Event_GetPlayerPos )
END_CLASS

/*
================
idObjective::idObjective
================
*/
idObjective::idObjective()
{
	playerPos.Zero();
}

/*
================
idObjective::Save
================
*/
void idObjective::Save( idSaveGame* savefile ) const
{
	savefile->WriteVec3( playerPos );
	savefile->WriteMaterial( screenshot );
}

/*
================
idObjective::Restore
================
*/
void idObjective::Restore( idRestoreGame* savefile )
{
	savefile->ReadVec3( playerPos );
	savefile->ReadMaterial( screenshot );
}

/*
================
idObjective::Spawn
================
*/
void idObjective::Spawn()
{
	Hide();
	idStr shotName;
	shotName = gameLocal.GetMapName();
	shotName.StripFileExtension();
	shotName += "/";
	shotName += spawnArgs.GetString( "screenshot" );
	shotName.SetFileExtension( ".tga" );
	screenshot = declManager->FindMaterial( shotName );
}

/*
================
idObjective::Event_Trigger
================
*/
void idObjective::Event_Trigger( idEntity* activator )
{
	idPlayer* player = gameLocal.GetLocalPlayer();
	if( player )
	{

		//Pickup( player );

		if( spawnArgs.GetString( "inv_objective", NULL ) )
		{
			if( player )
			{
				player->GiveObjective( spawnArgs.GetString( "objectivetitle" ), spawnArgs.GetString( "objectivetext" ), screenshot );

				// a tad slow but keeps from having to update all objectives in all maps with a name ptr
				for( int i = 0; i < gameLocal.num_entities; i++ )
				{
					if( gameLocal.entities[ i ] && gameLocal.entities[ i ]->IsType( idObjectiveComplete::Type ) )
					{
						if( idStr::Icmp( spawnArgs.GetString( "objectivetitle" ), gameLocal.entities[ i ]->spawnArgs.GetString( "objectivetitle" ) ) == 0 )
						{
							gameLocal.entities[ i ]->spawnArgs.SetBool( "objEnabled", true );
							break;
						}
					}
				}

				PostEventMS( &EV_GetPlayerPos, 2000 );
			}
		}
	}
}

/*
================
idObjective::Event_GetPlayerPos
================
*/
void idObjective::Event_GetPlayerPos()
{
	idPlayer* player = gameLocal.GetLocalPlayer();
	if( player )
	{
		playerPos = player->GetPhysics()->GetOrigin();
		PostEventMS( &EV_HideObjective, 100, player );
	}
}

/*
================
idObjective::Event_HideObjective
================
*/
void idObjective::Event_HideObjective( idEntity* e )
{
	idPlayer* player = gameLocal.GetLocalPlayer();
	if( player )
	{
		idVec3 v = player->GetPhysics()->GetOrigin() - playerPos;
		if( v.Length() > 64.0f )
		{
			player->HideObjective();
			PostEventMS( &EV_Remove, 0 );
		}
		else
		{
			PostEventMS( &EV_HideObjective, 100, player );
		}
	}
}

/*
===============================================================================

  idVideoCDItem

===============================================================================
*/

CLASS_DECLARATION( idItem, idVideoCDItem )
END_CLASS

/*
================
idVideoCDItem::GiveToPlayer
================
*/
bool idVideoCDItem::GiveToPlayer( idPlayer* player, unsigned int giveFlags )
{
	if( player == NULL )
	{
		return false;
	}
	const idDeclVideo* video = static_cast<const idDeclVideo* >( declManager->FindType( DECL_VIDEO, spawnArgs.GetString( "video" ), false ) );
	if( video == NULL )
	{
		return false;
	}
	if( giveFlags & ITEM_GIVE_UPDATE_STATE )
	{
		player->GiveVideo( video, spawnArgs.GetString( "inv_name" ) );
	}
	return true;
}

/*
===============================================================================

  idPDAItem

===============================================================================
*/

CLASS_DECLARATION( idItem, idPDAItem )
END_CLASS

/*
================
idPDAItem::GiveToPlayer
================
*/
bool idPDAItem::GiveToPlayer( idPlayer* player, unsigned int giveFlags )
{
	if( player == NULL )
	{
		return false;
	}
	const char* pdaName = spawnArgs.GetString( "pda_name" );
	const char* invName = spawnArgs.GetString( "inv_name" );
	const idDeclPDA* pda = NULL;
	if( pdaName != NULL && pdaName[0] != 0 )
	{
		// An empty PDA name is legitimate, it means the personal PDA
		// But if the PDA name is not empty, it should be valid
		pda = static_cast<const idDeclPDA*>( declManager->FindType( DECL_PDA, pdaName, false ) );
		if( pda == NULL )
		{
			idLib::Warning( "PDA Item '%s' references unknown PDA %s", GetName(), pdaName );
			return false;
		}
	}
	if( giveFlags & ITEM_GIVE_UPDATE_STATE )
	{
		player->GivePDA( pda, invName );
	}
	return true;
}

/*
===============================================================================

  idMoveableItem

===============================================================================
*/

CLASS_DECLARATION( idItem, idMoveableItem )
EVENT( EV_DropToFloor,	idMoveableItem::Event_DropToFloor )
EVENT( EV_Gib,			idMoveableItem::Event_Gib )
END_CLASS

/*
================
idMoveableItem::idMoveableItem
================
*/
idMoveableItem::idMoveableItem()
{
	trigger = NULL;
	smoke = NULL;
	smokeTime = 0;
	nextSoundTime = 0;
	repeatSmoke = false;
}

/*
================
idMoveableItem::~idMoveableItem
================
*/
idMoveableItem::~idMoveableItem()
{
	if( trigger )
	{
		delete trigger;
	}
}

/*
================
idMoveableItem::Save
================
*/
void idMoveableItem::Save( idSaveGame* savefile ) const
{
	savefile->WriteStaticObject( physicsObj );

	savefile->WriteClipModel( trigger );

	savefile->WriteParticle( smoke );
	savefile->WriteInt( smokeTime );
	savefile->WriteInt( nextSoundTime );
}

/*
================
idMoveableItem::Restore
================
*/
void idMoveableItem::Restore( idRestoreGame* savefile )
{
	savefile->ReadStaticObject( physicsObj );
	RestorePhysics( &physicsObj );

	savefile->ReadClipModel( trigger );

	savefile->ReadParticle( smoke );
	savefile->ReadInt( smokeTime );
	savefile->ReadInt( nextSoundTime );
}

/*
================
idMoveableItem::Spawn
================
*/
void idMoveableItem::Spawn()
{
	idTraceModel trm;
	float density, friction, bouncyness, tsize;
	idStr clipModelName;
	idBounds bounds;
	SetTimeState ts( timeGroup );

	// create a trigger for item pickup
	spawnArgs.GetFloat( "triggersize", "16.0", tsize );
	trigger = new( TAG_PHYSICS_CLIP_ENTITY ) idClipModel( idTraceModel( idBounds( vec3_origin ).Expand( tsize ) ) );
	trigger->Link( gameLocal.clip, this, 0, GetPhysics()->GetOrigin(), GetPhysics()->GetAxis() );
	trigger->SetContents( CONTENTS_TRIGGER );

	// check if a clip model is set
	spawnArgs.GetString( "clipmodel", "", clipModelName );
	if( !clipModelName[0] )
	{
		clipModelName = spawnArgs.GetString( "model" );		// use the visual model
	}

	// load the trace model
	if( !collisionModelManager->TrmFromModel( clipModelName, trm ) )
	{
		gameLocal.Error( "idMoveableItem '%s': cannot load collision model %s", name.c_str(), clipModelName.c_str() );
		return;
	}

	// if the model should be shrinked
	if( spawnArgs.GetBool( "clipshrink" ) )
	{
		trm.Shrink( CM_CLIP_EPSILON );
	}

	// get rigid body properties
	spawnArgs.GetFloat( "density", "0.5", density );
	density = idMath::ClampFloat( 0.001f, 1000.0f, density );
	spawnArgs.GetFloat( "friction", "0.05", friction );
	friction = idMath::ClampFloat( 0.0f, 1.0f, friction );
	spawnArgs.GetFloat( "bouncyness", "0.6", bouncyness );
	bouncyness = idMath::ClampFloat( 0.0f, 1.0f, bouncyness );

	// setup the physics
	physicsObj.SetSelf( this );
	physicsObj.SetClipModel( new( TAG_PHYSICS_CLIP_ENTITY ) idClipModel( trm ), density );
	physicsObj.SetOrigin( GetPhysics()->GetOrigin() );
	physicsObj.SetAxis( GetPhysics()->GetAxis() );
	physicsObj.SetBouncyness( bouncyness );
	physicsObj.SetFriction( 0.6f, 0.6f, friction );
	physicsObj.SetGravity( gameLocal.GetGravity() );
	physicsObj.SetContents( CONTENTS_RENDERMODEL );
	physicsObj.SetClipMask( MASK_SOLID | CONTENTS_MOVEABLECLIP );
	SetPhysics( &physicsObj );

	if( spawnArgs.GetBool( "nodrop" ) )
	{
		physicsObj.PutToRest();
	}

	smoke = NULL;
	smokeTime = 0;
	nextSoundTime = 0;
	const char* smokeName = spawnArgs.GetString( "smoke_trail" );
	if( *smokeName != '\0' )
	{
		smoke = static_cast<const idDeclParticle*>( declManager->FindType( DECL_PARTICLE, smokeName ) );
		smokeTime = gameLocal.time;
		BecomeActive( TH_UPDATEPARTICLES );
	}

	repeatSmoke = spawnArgs.GetBool( "repeatSmoke", "0" );
}

/*
================
idItem::ClientThink
================
*/
void idMoveableItem::ClientThink( const int curTime, const float fraction, const bool predict )
{

	InterpolatePhysicsOnly( fraction );

	if( thinkFlags & TH_PHYSICS )
	{
		// update trigger position
		trigger->Link( gameLocal.clip, this, 0, GetPhysics()->GetOrigin(), mat3_identity );
	}

	Present();
}

/*
================
idMoveableItem::Think
================
*/
void idMoveableItem::Think()
{

	RunPhysics();

	if( thinkFlags & TH_PHYSICS )
	{
		// update trigger position
		trigger->Link( gameLocal.clip, this, 0, GetPhysics()->GetOrigin(), mat3_identity );
	}

	if( thinkFlags & TH_UPDATEPARTICLES )
	{
		if( !gameLocal.smokeParticles->EmitSmoke( smoke, smokeTime, gameLocal.random.CRandomFloat(), GetPhysics()->GetOrigin(), GetPhysics()->GetAxis(), timeGroup /*_D3XP*/ ) )
		{
			if( !repeatSmoke )
			{
				smokeTime = 0;
				BecomeInactive( TH_UPDATEPARTICLES );
			}
			else
			{
				smokeTime = gameLocal.time;
			}
		}
	}

	Present();
}

/*
=================
idMoveableItem::Collide
=================
*/
bool idMoveableItem::Collide( const trace_t& collision, const idVec3& velocity )
{
	float v, f;

	v = -( velocity * collision.c.normal );
	if( v > 80 && gameLocal.time > nextSoundTime )
	{
		f = v > 200 ? 1.0f : idMath::Sqrt( v - 80 ) * 0.091f;
		if( StartSound( "snd_bounce", SND_CHANNEL_ANY, 0, false, NULL ) )
		{
			// don't set the volume unless there is a bounce sound as it overrides the entire channel
			// which causes footsteps on ai's to not honor their shader parms
			SetSoundVolume( f );
		}
		nextSoundTime = gameLocal.time + 500;
	}

	return false;
}

/*
================
idMoveableItem::Pickup
================
*/
bool idMoveableItem::Pickup( idPlayer* player )
{
	bool ret = idItem::Pickup( player );
	if( ret )
	{
		trigger->SetContents( 0 );
	}
	return ret;
}

/*
================
idMoveableItem::DropItem
================
*/
idEntity* idMoveableItem::DropItem( const char* classname, const idVec3& origin, const idMat3& axis, const idVec3& velocity, int activateDelay, int removeDelay )
{
	idDict args;
	idEntity* item;

	args.Set( "classname", classname );
	args.Set( "dropped", "1" );

	// we sometimes drop idMoveables here, so set 'nodrop' to 1 so that it doesn't get put on the floor
	args.Set( "nodrop", "1" );

	if( activateDelay )
	{
		args.SetBool( "triggerFirst", true );
	}

	gameLocal.SpawnEntityDef( args, &item );
	if( item )
	{
		// set item position
		item->GetPhysics()->SetOrigin( origin );
		item->GetPhysics()->SetAxis( axis );
		item->GetPhysics()->SetLinearVelocity( velocity );
		item->UpdateVisuals();
		if( activateDelay )
		{
			item->PostEventMS( &EV_Activate, activateDelay, item );
		}
		if( !removeDelay )
		{
			removeDelay = 5 * 60 * 1000;
		}
		// always remove a dropped item after 5 minutes in case it dropped to an unreachable location
		item->PostEventMS( &EV_Remove, removeDelay );
	}
	return item;
}

/*
================
idMoveableItem::DropItems

  The entity should have the following key/value pairs set:
	"def_drop<type>Item"			"item def"
	"drop<type>ItemJoint"			"joint name"
	"drop<type>ItemRotation"		"pitch yaw roll"
	"drop<type>ItemOffset"			"x y z"
	"skin_drop<type>"				"skin name"
  To drop multiple items the following key/value pairs can be used:
	"def_drop<type>Item<X>"			"item def"
	"drop<type>Item<X>Joint"		"joint name"
	"drop<type>Item<X>Rotation"		"pitch yaw roll"
	"drop<type>Item<X>Offset"		"x y z"
  where <X> is an aribtrary string.
================
*/
void idMoveableItem::DropItems( idAnimatedEntity*  ent, const char* type, idList<idEntity*>* list )
{
	const idKeyValue* kv;
	const char* skinName, *c, *jointName;
	idStr key, key2;
	idVec3 origin;
	idMat3 axis;
	idAngles angles;
	const idDeclSkin* skin;
	jointHandle_t joint;
	idEntity* item;

	// drop all items
	kv = ent->spawnArgs.MatchPrefix( va( "def_drop%sItem", type ), NULL );
	while( kv )
	{

		c = kv->GetKey().c_str() + kv->GetKey().Length();
		if( idStr::Icmp( c - 5, "Joint" ) != 0 && idStr::Icmp( c - 8, "Rotation" ) != 0 )
		{

			key = kv->GetKey().c_str() + 4;
			key2 = key;
			key += "Joint";
			key2 += "Offset";
			jointName = ent->spawnArgs.GetString( key );
			joint = ent->GetAnimator()->GetJointHandle( jointName );
			if( !ent->GetJointWorldTransform( joint, gameLocal.time, origin, axis ) )
			{
				gameLocal.Warning( "%s refers to invalid joint '%s' on entity '%s'\n", key.c_str(), jointName, ent->name.c_str() );
				origin = ent->GetPhysics()->GetOrigin();
				axis = ent->GetPhysics()->GetAxis();
			}
			if( g_dropItemRotation.GetString()[0] )
			{
				angles.Zero();
				sscanf( g_dropItemRotation.GetString(), "%f %f %f", &angles.pitch, &angles.yaw, &angles.roll );
			}
			else
			{
				key = kv->GetKey().c_str() + 4;
				key += "Rotation";
				ent->spawnArgs.GetAngles( key, "0 0 0", angles );
			}
			axis = angles.ToMat3() * axis;

			origin += ent->spawnArgs.GetVector( key2, "0 0 0" );

			item = DropItem( kv->GetValue(), origin, axis, vec3_origin, 0, 0 );
			if( list && item )
			{
				list->Append( item );
			}
		}

		kv = ent->spawnArgs.MatchPrefix( va( "def_drop%sItem", type ), kv );
	}

	// change the skin to hide all items
	skinName = ent->spawnArgs.GetString( va( "skin_drop%s", type ) );
	if( skinName[0] )
	{
		skin = declManager->FindSkin( skinName );
		ent->SetSkin( skin );
	}
}

/*
======================
idMoveableItem::WriteToSnapshot
======================
*/
void idMoveableItem::WriteToSnapshot( idBitMsg& msg ) const
{
	physicsObj.WriteToSnapshot( msg );
	msg.WriteBool( IsHidden() );
}

/*
======================
idMoveableItem::ReadFromSnapshot
======================
*/
void idMoveableItem::ReadFromSnapshot( const idBitMsg& msg )
{
	physicsObj.ReadFromSnapshot( msg );
	const bool snapshotHidden = msg.ReadBool();

	if( snapshotHidden )
	{
		Hide();
	}
	else if( GetPredictPickupMilliseconds() != 0 )
	{
		if( gameLocal.GetLastClientUsercmdMilliseconds( gameLocal.GetLocalClientNum() ) >= GetPredictPickupMilliseconds() )
		{
			if( trigger->GetContents() == 0 )
			{
				trigger->SetContents( CONTENTS_TRIGGER );
			}
			Show();
		}
	}
	if( msg.HasChanged() )
	{
		UpdateVisuals();
	}
}

/*
============
idMoveableItem::Gib
============
*/
void idMoveableItem::Gib( const idVec3& dir, const char* damageDefName )
{
	// spawn smoke puff
	const char* smokeName = spawnArgs.GetString( "smoke_gib" );
	if( *smokeName != '\0' )
	{
		const idDeclParticle* smoke = static_cast<const idDeclParticle*>( declManager->FindType( DECL_PARTICLE, smokeName ) );
		gameLocal.smokeParticles->EmitSmoke( smoke, gameLocal.time, gameLocal.random.CRandomFloat(), renderEntity.origin, renderEntity.axis, timeGroup /*_D3XP*/ );
	}
	// remove the entity
	PostEventMS( &EV_Remove, 0 );
}

/*
================
idMoveableItem::Event_DropToFloor
================
*/
void idMoveableItem::Event_DropToFloor()
{
	// the physics will drop the moveable to the floor
}

/*
============
idMoveableItem::Event_Gib
============
*/
void idMoveableItem::Event_Gib( const char* damageDefName )
{
	Gib( idVec3( 0, 0, 1 ), damageDefName );
}

/*
===============================================================================

  idMoveablePDAItem

===============================================================================
*/

CLASS_DECLARATION( idMoveableItem, idMoveablePDAItem )
END_CLASS

/*
================
idMoveablePDAItem::GiveToPlayer
================
*/
bool idMoveablePDAItem::GiveToPlayer( idPlayer* player, unsigned int giveFlags )
{
	if( player == NULL )
	{
		return false;
	}
	const char* pdaName = spawnArgs.GetString( "pda_name" );
	const char* invName = spawnArgs.GetString( "inv_name" );
	const idDeclPDA* pda = NULL;
	if( pdaName != NULL && pdaName[0] != 0 )
	{
		// An empty PDA name is legitimate, it means the personal PDA
		// But if the PDA name is not empty, it should be valid
		pda = static_cast<const idDeclPDA*>( declManager->FindType( DECL_PDA, pdaName, false ) );
		if( pda == NULL )
		{
			idLib::Warning( "PDA Item '%s' references unknown PDA %s", GetName(), pdaName );
			return false;
		}
	}
	if( giveFlags & ITEM_GIVE_UPDATE_STATE )
	{
		player->GivePDA( pda, invName );
	}
	return true;
}

/*
===============================================================================

  idItemRemover

===============================================================================
*/

CLASS_DECLARATION( idEntity, idItemRemover )
EVENT( EV_Activate,		idItemRemover::Event_Trigger )
END_CLASS

/*
================
idItemRemover::Spawn
================
*/
void idItemRemover::Spawn()
{
}

/*
================
idItemRemover::RemoveItem
================
*/
void idItemRemover::RemoveItem( idPlayer* player )
{
	const char* remove;

	remove = spawnArgs.GetString( "remove" );
	player->RemoveInventoryItem( remove );
}

/*
================
idItemRemover::Event_Trigger
================
*/
void idItemRemover::Event_Trigger( idEntity* activator )
{
	if( activator->IsType( idPlayer::Type ) )
	{
		RemoveItem( static_cast<idPlayer*>( activator ) );
	}
}

/*
===============================================================================

  idObjectiveComplete

===============================================================================
*/

CLASS_DECLARATION( idItemRemover, idObjectiveComplete )
EVENT( EV_Activate,			idObjectiveComplete::Event_Trigger )
EVENT( EV_HideObjective,	idObjectiveComplete::Event_HideObjective )
EVENT( EV_GetPlayerPos,		idObjectiveComplete::Event_GetPlayerPos )
END_CLASS

/*
================
idObjectiveComplete::idObjectiveComplete
================
*/
idObjectiveComplete::idObjectiveComplete()
{
	playerPos.Zero();
}

/*
================
idObjectiveComplete::Save
================
*/
void idObjectiveComplete::Save( idSaveGame* savefile ) const
{
	savefile->WriteVec3( playerPos );
}

/*
================
idObjectiveComplete::Restore
================
*/
void idObjectiveComplete::Restore( idRestoreGame* savefile )
{
	savefile->ReadVec3( playerPos );
}

/*
================
idObjectiveComplete::Spawn
================
*/
void idObjectiveComplete::Spawn()
{
	spawnArgs.SetBool( "objEnabled", false );
	Hide();
}

/*
================
idObjectiveComplete::Event_Trigger
================
*/
void idObjectiveComplete::Event_Trigger( idEntity* activator )
{
	if( !spawnArgs.GetBool( "objEnabled" ) )
	{
		return;
	}
	idPlayer* player = gameLocal.GetLocalPlayer();
	if( player )
	{
		RemoveItem( player );

		if( spawnArgs.GetString( "inv_objective", NULL ) )
		{
			player->CompleteObjective( spawnArgs.GetString( "objectivetitle" ) );
			PostEventMS( &EV_GetPlayerPos, 2000 );
		}
	}
}

/*
================
idObjectiveComplete::Event_GetPlayerPos
================
*/
void idObjectiveComplete::Event_GetPlayerPos()
{
	idPlayer* player = gameLocal.GetLocalPlayer();
	if( player )
	{
		playerPos = player->GetPhysics()->GetOrigin();
		PostEventMS( &EV_HideObjective, 100, player );
	}
}

/*
================
idObjectiveComplete::Event_HideObjective
================
*/
void idObjectiveComplete::Event_HideObjective( idEntity* e )
{
	idPlayer* player = gameLocal.GetLocalPlayer();
	if( player )
	{
		idVec3 v = player->GetPhysics()->GetOrigin();
		v -= playerPos;
		if( v.Length() > 64.0f )
		{
			player->HideObjective();
			PostEventMS( &EV_Remove, 0 );
		}
		else
		{
			PostEventMS( &EV_HideObjective, 100, player );
		}
	}
}
