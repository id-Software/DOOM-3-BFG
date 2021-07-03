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

	idEntityFx

===============================================================================
*/

const idEventDef EV_Fx_KillFx( "_killfx" );
const idEventDef EV_Fx_Action( "_fxAction", "e" );		// implemented by subclasses

CLASS_DECLARATION( idEntity, idEntityFx )
EVENT( EV_Activate,	   	idEntityFx::Event_Trigger )
EVENT( EV_Fx_KillFx,	idEntityFx::Event_ClearFx )
END_CLASS


/*
================
idEntityFx::Save
================
*/
void idEntityFx::Save( idSaveGame* savefile ) const
{
	int i;

	savefile->WriteInt( started );
	savefile->WriteInt( nextTriggerTime );
	savefile->WriteFX( fxEffect );
	savefile->WriteString( systemName );

	savefile->WriteInt( actions.Num() );

	for( i = 0; i < actions.Num(); i++ )
	{

		if( actions[i].lightDefHandle >= 0 )
		{
			savefile->WriteBool( true );
			savefile->WriteRenderLight( actions[i].renderLight );
		}
		else
		{
			savefile->WriteBool( false );
		}

		if( actions[i].modelDefHandle >= 0 )
		{
			savefile->WriteBool( true );
			savefile->WriteRenderEntity( actions[i].renderEntity );
		}
		else
		{
			savefile->WriteBool( false );
		}

		savefile->WriteFloat( actions[i].delay );
		savefile->WriteInt( actions[i].start );
		savefile->WriteBool( actions[i].soundStarted );
		savefile->WriteBool( actions[i].shakeStarted );
		savefile->WriteBool( actions[i].decalDropped );
		savefile->WriteBool( actions[i].launched );
	}
}

/*
================
idEntityFx::Restore
================
*/
void idEntityFx::Restore( idRestoreGame* savefile )
{
	int i;
	int num;
	bool hasObject;

	savefile->ReadInt( started );
	savefile->ReadInt( nextTriggerTime );
	savefile->ReadFX( fxEffect );
	savefile->ReadString( systemName );

	savefile->ReadInt( num );

	actions.SetNum( num );
	for( i = 0; i < num; i++ )
	{

		savefile->ReadBool( hasObject );
		if( hasObject )
		{
			savefile->ReadRenderLight( actions[i].renderLight );
			actions[i].lightDefHandle = gameRenderWorld->AddLightDef( &actions[i].renderLight );
		}
		else
		{
			memset( &actions[i].renderLight, 0, sizeof( renderLight_t ) );
			actions[i].lightDefHandle = -1;
		}

		savefile->ReadBool( hasObject );
		if( hasObject )
		{
			savefile->ReadRenderEntity( actions[i].renderEntity );
			actions[i].modelDefHandle = gameRenderWorld->AddEntityDef( &actions[i].renderEntity );
		}
		else
		{
			memset( &actions[i].renderEntity, 0, sizeof( renderEntity_t ) );
			actions[i].modelDefHandle = -1;
		}

		savefile->ReadFloat( actions[i].delay );

		// let the FX regenerate the particleSystem
		actions[i].particleSystem = -1;

		savefile->ReadInt( actions[i].start );
		savefile->ReadBool( actions[i].soundStarted );
		savefile->ReadBool( actions[i].shakeStarted );
		savefile->ReadBool( actions[i].decalDropped );
		savefile->ReadBool( actions[i].launched );
	}
}

/*
================
idEntityFx::Setup
================
*/
void idEntityFx::Setup( const char* fx )
{

	if( started >= 0 )
	{
		return;					// already started
	}

	// early during MP Spawn() with no information. wait till we ReadFromSnapshot for more
	if( common->IsClient() && ( !fx || fx[0] == '\0' ) )
	{
		return;
	}

	systemName = fx;
	started = 0;

	fxEffect = static_cast<const idDeclFX*>( declManager->FindType( DECL_FX, systemName.c_str() ) );

	if( fxEffect )
	{
		idFXLocalAction localAction;

		memset( &localAction, 0, sizeof( idFXLocalAction ) );

		actions.AssureSize( fxEffect->events.Num(), localAction );

		for( int i = 0; i < fxEffect->events.Num(); i++ )
		{
			const idFXSingleAction& fxaction = fxEffect->events[i];

			idFXLocalAction& laction = actions[i];
			if( fxaction.random1 || fxaction.random2 )
			{
				laction.delay = fxaction.random1 + gameLocal.random.RandomFloat() * ( fxaction.random2 - fxaction.random1 );
			}
			else
			{
				laction.delay = fxaction.delay;
			}
			laction.start = -1;
			laction.lightDefHandle = -1;
			laction.modelDefHandle = -1;
			laction.particleSystem = -1;
			laction.shakeStarted = false;
			laction.decalDropped = false;
			laction.launched = false;
		}
	}
}

/*
================
idEntityFx::EffectName
================
*/
const char* idEntityFx::EffectName()
{
	return fxEffect ? fxEffect->GetName() : NULL;
}

/*
================
idEntityFx::Joint
================
*/
const char* idEntityFx::Joint()
{
	return fxEffect ? fxEffect->joint.c_str() : NULL;
}

/*
================
idEntityFx::CleanUp
================
*/
void idEntityFx::CleanUp()
{
	if( !fxEffect )
	{
		return;
	}
	for( int i = 0; i < fxEffect->events.Num(); i++ )
	{
		const idFXSingleAction& fxaction = fxEffect->events[i];
		idFXLocalAction& laction = actions[i];
		CleanUpSingleAction( fxaction, laction );
	}
}

/*
================
idEntityFx::CleanUpSingleAction
================
*/
void idEntityFx::CleanUpSingleAction( const idFXSingleAction& fxaction, idFXLocalAction& laction )
{
	if( laction.lightDefHandle != -1 && fxaction.sibling == -1 && fxaction.type != FX_ATTACHLIGHT )
	{
		gameRenderWorld->FreeLightDef( laction.lightDefHandle );
		laction.lightDefHandle = -1;
	}
	if( laction.modelDefHandle != -1 && fxaction.sibling == -1 && fxaction.type != FX_ATTACHENTITY )
	{
		gameRenderWorld->FreeEntityDef( laction.modelDefHandle );
		laction.modelDefHandle = -1;
	}
	laction.start = -1;
}

/*
================
idEntityFx::Start
================
*/
void idEntityFx::Start( int time )
{
	if( !fxEffect )
	{
		return;
	}
	started = time;
	for( int i = 0; i < fxEffect->events.Num(); i++ )
	{
		idFXLocalAction& laction = actions[i];
		laction.start = time;
		laction.soundStarted = false;
		laction.shakeStarted = false;
		laction.particleSystem = -1;
		laction.decalDropped = false;
		laction.launched = false;
	}
}

/*
================
idEntityFx::Stop
================
*/
void idEntityFx::Stop()
{
	CleanUp();
	started = -1;
}

/*
================
idEntityFx::Duration
================
*/
const int idEntityFx::Duration()
{
	int max = 0;

	if( !fxEffect )
	{
		return max;
	}
	for( int i = 0; i < fxEffect->events.Num(); i++ )
	{
		const idFXSingleAction& fxaction = fxEffect->events[i];
		int d = ( fxaction.delay + fxaction.duration ) * 1000.0f;
		if( d > max )
		{
			max = d;
		}
	}

	return max;
}


/*
================
idEntityFx::Done
================
*/
const bool idEntityFx::Done()
{
	if( started > 0 && gameLocal.time > started + Duration() )
	{
		return true;
	}
	return false;
}

/*
================
idEntityFx::ApplyFade
================
*/
void idEntityFx::ApplyFade( const idFXSingleAction& fxaction, idFXLocalAction& laction, const int time, const int actualStart )
{
	if( fxaction.fadeInTime || fxaction.fadeOutTime )
	{
		float fadePct = ( float )( time - actualStart ) / ( 1000.0f * ( ( fxaction.fadeInTime != 0 ) ? fxaction.fadeInTime : fxaction.fadeOutTime ) );
		if( fadePct > 1.0 )
		{
			fadePct = 1.0;
		}
		if( laction.modelDefHandle != -1 )
		{
			laction.renderEntity.shaderParms[SHADERPARM_RED] = ( fxaction.fadeInTime ) ? fadePct : 1.0f - fadePct;
			laction.renderEntity.shaderParms[SHADERPARM_GREEN] = ( fxaction.fadeInTime ) ? fadePct : 1.0f - fadePct;
			laction.renderEntity.shaderParms[SHADERPARM_BLUE] = ( fxaction.fadeInTime ) ? fadePct : 1.0f - fadePct;

			gameRenderWorld->UpdateEntityDef( laction.modelDefHandle, &laction.renderEntity );
		}
		if( laction.lightDefHandle != -1 )
		{
			laction.renderLight.shaderParms[SHADERPARM_RED] = fxaction.lightColor.x * ( ( fxaction.fadeInTime ) ? fadePct : 1.0f - fadePct );
			laction.renderLight.shaderParms[SHADERPARM_GREEN] = fxaction.lightColor.y * ( ( fxaction.fadeInTime ) ? fadePct : 1.0f - fadePct );
			laction.renderLight.shaderParms[SHADERPARM_BLUE] = fxaction.lightColor.z * ( ( fxaction.fadeInTime ) ? fadePct : 1.0f - fadePct );

			gameRenderWorld->UpdateLightDef( laction.lightDefHandle, &laction.renderLight );
		}
	}
}

/*
================
idEntityFx::Run
================
*/
void idEntityFx::Run( int time )
{
	int ieff, j;
	idEntity* ent = NULL;
	const idDict* projectileDef = NULL;
	idProjectile* projectile = NULL;

	if( !fxEffect )
	{
		return;
	}

	for( ieff = 0; ieff < fxEffect->events.Num(); ieff++ )
	{
		const idFXSingleAction& fxaction = fxEffect->events[ieff];
		idFXLocalAction& laction = actions[ieff];

		//
		// if we're currently done with this one
		//
		if( laction.start == -1 )
		{
			continue;
		}

		//
		// see if it's delayed
		//
		if( laction.delay )
		{
			if( laction.start + ( time - laction.start ) < laction.start + ( laction.delay * 1000 ) )
			{
				continue;
			}
		}

		//
		// each event can have it's own delay and restart
		//
		int actualStart = laction.delay ? laction.start + ( int )( laction.delay * 1000 ) : laction.start;
		float pct = ( float )( time - actualStart ) / ( 1000 * fxaction.duration );
		if( pct >= 1.0f )
		{
			laction.start = -1;
			float totalDelay = 0.0f;
			if( fxaction.restart )
			{
				if( fxaction.random1 || fxaction.random2 )
				{
					totalDelay = fxaction.random1 + gameLocal.random.RandomFloat() * ( fxaction.random2 - fxaction.random1 );
				}
				else
				{
					totalDelay = fxaction.delay;
				}
				laction.delay = totalDelay;
				laction.start = time;
			}
			continue;
		}

		if( fxaction.fire.Length() )
		{
			for( j = 0; j < fxEffect->events.Num(); j++ )
			{
				if( fxEffect->events[j].name.Icmp( fxaction.fire ) == 0 )
				{
					actions[j].delay = 0;
				}
			}
		}

		idFXLocalAction* useAction;
		if( fxaction.sibling == -1 )
		{
			useAction = &laction;
		}
		else
		{
			useAction = &actions[fxaction.sibling];
		}
		assert( useAction );

		switch( fxaction.type )
		{
			case FX_ATTACHLIGHT:
			case FX_LIGHT:
			{
				if( useAction->lightDefHandle == -1 )
				{
					if( fxaction.type == FX_LIGHT )
					{
						memset( &useAction->renderLight, 0, sizeof( renderLight_t ) );
						useAction->renderLight.origin = GetPhysics()->GetOrigin() + fxaction.offset;
						useAction->renderLight.axis = GetPhysics()->GetAxis();
						useAction->renderLight.lightRadius[0] = fxaction.lightRadius;
						useAction->renderLight.lightRadius[1] = fxaction.lightRadius;
						useAction->renderLight.lightRadius[2] = fxaction.lightRadius;
						useAction->renderLight.shader = declManager->FindMaterial( fxaction.data, false );
						useAction->renderLight.shaderParms[ SHADERPARM_RED ]	= fxaction.lightColor.x;
						useAction->renderLight.shaderParms[ SHADERPARM_GREEN ]	= fxaction.lightColor.y;
						useAction->renderLight.shaderParms[ SHADERPARM_BLUE ]	= fxaction.lightColor.z;
						useAction->renderLight.shaderParms[ SHADERPARM_TIMESCALE ]	= 1.0f;
						useAction->renderLight.shaderParms[ SHADERPARM_TIMEOFFSET ] = -MS2SEC( time );
						useAction->renderLight.referenceSound = refSound.referenceSound;
						useAction->renderLight.pointLight = true;
						if( fxaction.noshadows )
						{
							useAction->renderLight.noShadows = true;
						}
						useAction->lightDefHandle = gameRenderWorld->AddLightDef( &useAction->renderLight );
					}
					if( fxaction.noshadows )
					{
						for( j = 0; j < fxEffect->events.Num(); j++ )
						{
							idFXLocalAction& laction2 = actions[j];
							if( laction2.modelDefHandle != -1 )
							{
								laction2.renderEntity.noShadow = true;
							}
						}
					}
				}
				ApplyFade( fxaction, *useAction, time, actualStart );
				break;
			}
			case FX_SOUND:
			{
				if( !useAction->soundStarted )
				{
					useAction->soundStarted = true;
					const idSoundShader* shader = declManager->FindSound( fxaction.data );
					StartSoundShader( shader, SND_CHANNEL_ANY, 0, false, NULL );
					for( j = 0; j < fxEffect->events.Num(); j++ )
					{
						idFXLocalAction& laction2 = actions[j];
						if( laction2.lightDefHandle != -1 )
						{
							laction2.renderLight.referenceSound = refSound.referenceSound;
							gameRenderWorld->UpdateLightDef( laction2.lightDefHandle, &laction2.renderLight );
						}
					}
				}
				break;
			}
			case FX_DECAL:
			{
				if( !useAction->decalDropped )
				{
					useAction->decalDropped = true;
					gameLocal.ProjectDecal( GetPhysics()->GetOrigin(), GetPhysics()->GetGravity(), 8.0f, true, fxaction.size, fxaction.data );
				}
				break;
			}
			case FX_SHAKE:
			{
				if( !useAction->shakeStarted )
				{
					idDict args;
					args.Clear();
					args.SetFloat( "kick_time", fxaction.shakeTime );
					args.SetFloat( "kick_amplitude", fxaction.shakeAmplitude );
					for( j = 0; j < gameLocal.numClients; j++ )
					{
						idPlayer* player = gameLocal.GetClientByNum( j );
						if( player && ( player->GetPhysics()->GetOrigin() - GetPhysics()->GetOrigin() ).LengthSqr() < Square( fxaction.shakeDistance ) )
						{
							if( !common->IsMultiplayer() || !fxaction.shakeIgnoreMaster || GetBindMaster() != player )
							{
								player->playerView.DamageImpulse( fxaction.offset, &args );
							}
						}
					}
					if( fxaction.shakeImpulse != 0.0f && fxaction.shakeDistance != 0.0f )
					{
						idEntity* ignore_ent = NULL;
						if( common->IsMultiplayer() )
						{
							ignore_ent = this;
							if( fxaction.shakeIgnoreMaster )
							{
								ignore_ent = GetBindMaster();
							}
						}
						// lookup the ent we are bound to?
						gameLocal.RadiusPush( GetPhysics()->GetOrigin(), fxaction.shakeDistance, fxaction.shakeImpulse, this, ignore_ent, 1.0f, true );
					}
					useAction->shakeStarted = true;
				}
				break;
			}
			case FX_ATTACHENTITY:
			case FX_PARTICLE:
			case FX_MODEL:
			{
				if( useAction->modelDefHandle == -1 )
				{
					memset( &useAction->renderEntity, 0, sizeof( renderEntity_t ) );
					useAction->renderEntity.origin = GetPhysics()->GetOrigin() + fxaction.offset;
					useAction->renderEntity.axis = ( fxaction.explicitAxis ) ? fxaction.axis : GetPhysics()->GetAxis();
					useAction->renderEntity.hModel = renderModelManager->FindModel( fxaction.data );
					useAction->renderEntity.shaderParms[ SHADERPARM_RED ]		= 1.0f;
					useAction->renderEntity.shaderParms[ SHADERPARM_GREEN ]		= 1.0f;
					useAction->renderEntity.shaderParms[ SHADERPARM_BLUE ]		= 1.0f;
					useAction->renderEntity.shaderParms[ SHADERPARM_TIMEOFFSET ] = -MS2SEC( time );
					useAction->renderEntity.shaderParms[3] = 1.0f;
					useAction->renderEntity.shaderParms[5] = 0.0f;
					if( useAction->renderEntity.hModel )
					{
						useAction->renderEntity.bounds = useAction->renderEntity.hModel->Bounds( &useAction->renderEntity );
					}
					useAction->modelDefHandle = gameRenderWorld->AddEntityDef( &useAction->renderEntity );
				}
				else if( fxaction.trackOrigin )
				{
					useAction->renderEntity.origin = GetPhysics()->GetOrigin() + fxaction.offset;
					useAction->renderEntity.axis = fxaction.explicitAxis ? fxaction.axis : GetPhysics()->GetAxis();
					gameRenderWorld->UpdateEntityDef( useAction->modelDefHandle, &useAction->renderEntity );
				}
				ApplyFade( fxaction, *useAction, time, actualStart );
				break;
			}
			case FX_LAUNCH:
			{
				if( common->IsClient() )
				{
					// client never spawns entities outside of ClientReadSnapshot
					useAction->launched = true;
					break;
				}
				if( !useAction->launched )
				{
					useAction->launched = true;
					projectile = NULL;
					// FIXME: may need to cache this if it is slow
					projectileDef = gameLocal.FindEntityDefDict( fxaction.data, false );
					if( !projectileDef )
					{
						gameLocal.Warning( "projectile \'%s\' not found", fxaction.data.c_str() );
					}
					else
					{
						gameLocal.SpawnEntityDef( *projectileDef, &ent, false );
						if( ent && ent->IsType( idProjectile::Type ) )
						{
							projectile = ( idProjectile* )ent;
							projectile->Create( this, GetPhysics()->GetOrigin(), GetPhysics()->GetAxis()[0] );
							projectile->Launch( GetPhysics()->GetOrigin(), GetPhysics()->GetAxis()[0], vec3_origin );
						}
					}
				}
				break;
			}
			case FX_SHOCKWAVE:
			{
				if( common->IsClient() )
				{
					useAction->shakeStarted = true;
					break;
				}
				if( !useAction->shakeStarted )
				{
					idStr	shockDefName;
					useAction->shakeStarted = true;

					shockDefName = fxaction.data;
					if( !shockDefName.Length() )
					{
						shockDefName = "func_shockwave";
					}

					projectileDef = gameLocal.FindEntityDefDict( shockDefName, false );
					if( !projectileDef )
					{
						gameLocal.Warning( "shockwave \'%s\' not found", shockDefName.c_str() );
					}
					else
					{
						gameLocal.SpawnEntityDef( *projectileDef, &ent );
						ent->SetOrigin( GetPhysics()->GetOrigin() + fxaction.offset );
						ent->PostEventMS( &EV_Remove, ent->spawnArgs.GetInt( "duration" ) );
					}
				}
				break;
			}
		}
	}
}

/*
================
idEntityFx::idEntityFx
================
*/
idEntityFx::idEntityFx()
{
	fxEffect = NULL;
	started = -1;
	nextTriggerTime = -1;
	fl.networkSync = true;
}

/*
================
idEntityFx::~idEntityFx
================
*/
idEntityFx::~idEntityFx()
{
	CleanUp();
	fxEffect = NULL;
}

/*
================
idEntityFx::Spawn
================
*/
void idEntityFx::Spawn()
{

	if( g_skipFX.GetBool() )
	{
		return;
	}

	const char* fx;
	nextTriggerTime = 0;
	fxEffect = NULL;
	if( spawnArgs.GetString( "fx", "", &fx ) )
	{
		systemName = fx;
	}
	if( !spawnArgs.GetBool( "triggered" ) )
	{
		Setup( fx );
		if( spawnArgs.GetBool( "test" ) || spawnArgs.GetBool( "start" ) || spawnArgs.GetFloat( "restart" ) )
		{
			PostEventMS( &EV_Activate, 0, this );
		}
	}
}

/*
================
idEntityFx::Think

  Clears any visual fx started when {item,mob,player} was spawned
================
*/
void idEntityFx::Think()
{
	if( g_skipFX.GetBool() )
	{
		return;
	}

	if( thinkFlags & TH_THINK )
	{
		Run( gameLocal.time );
	}

	RunPhysics();
	Present();
}

/*
================
idEntityFx::Event_ClearFx

  Clears any visual fx started when item(mob) was spawned
================
*/
void idEntityFx::Event_ClearFx()
{

	if( g_skipFX.GetBool() )
	{
		return;
	}

	Stop();
	CleanUp();
	BecomeInactive( TH_THINK );

	if( spawnArgs.GetBool( "test" ) )
	{
		PostEventMS( &EV_Activate, 0, this );
	}
	else
	{
		if( spawnArgs.GetFloat( "restart" ) || !spawnArgs.GetBool( "triggered" ) )
		{
			float rest = spawnArgs.GetFloat( "restart", "0" );
			if( rest == 0.0f )
			{
				PostEventSec( &EV_Remove, 0.1f );
			}
			else
			{
				rest *= gameLocal.random.RandomFloat();
				PostEventSec( &EV_Activate, rest, this );
			}
		}
	}
}

/*
================
idEntityFx::Event_Trigger
================
*/
void idEntityFx::Event_Trigger( idEntity* activator )
{

	if( g_skipFX.GetBool() )
	{
		return;
	}

	float		fxActionDelay;
	const char* fx;

	if( gameLocal.time < nextTriggerTime )
	{
		return;
	}

	if( spawnArgs.GetString( "fx", "", &fx ) )
	{
		Setup( fx );
		Start( gameLocal.time );
		PostEventMS( &EV_Fx_KillFx, Duration() );
		BecomeActive( TH_THINK );
	}

	fxActionDelay = spawnArgs.GetFloat( "fxActionDelay" );
	if( fxActionDelay != 0.0f )
	{
		nextTriggerTime = gameLocal.time + SEC2MS( fxActionDelay );
	}
	else
	{
		// prevent multiple triggers on same frame
		nextTriggerTime = gameLocal.time + 1;
	}
	PostEventSec( &EV_Fx_Action, fxActionDelay, activator );
}


/*
================
idEntityFx::StartFx
================
*/
idEntityFx* idEntityFx::StartFx( const char* fx, const idVec3* useOrigin, const idMat3* useAxis, idEntity* ent, bool bind )
{

	if( g_skipFX.GetBool() || !fx || !*fx )
	{
		return NULL;
	}

	idDict args;
	args.SetBool( "start", true );
	args.Set( "fx", fx );
	idEntityFx* nfx = static_cast<idEntityFx*>( gameLocal.SpawnEntityType( idEntityFx::Type, &args ) );
	if( nfx->Joint() && *nfx->Joint() )
	{
		nfx->BindToJoint( ent, nfx->Joint(), true );
		nfx->SetOrigin( vec3_origin );
	}
	else
	{
		nfx->SetOrigin( ( useOrigin ) ? *useOrigin : ent->GetPhysics()->GetOrigin() );
		nfx->SetAxis( ( useAxis ) ? *useAxis : ent->GetPhysics()->GetAxis() );
	}

	if( bind )
	{
		// never bind to world spawn
		if( ent != gameLocal.world )
		{
			nfx->Bind( ent, true );
		}
	}
	nfx->Show();
	return nfx;
}

/*
=================
idEntityFx::WriteToSnapshot
=================
*/
void idEntityFx::WriteToSnapshot( idBitMsg& msg ) const
{
	GetPhysics()->WriteToSnapshot( msg );
	WriteBindToSnapshot( msg );
	msg.WriteLong( ( fxEffect != NULL ) ? gameLocal.ServerRemapDecl( -1, DECL_FX, fxEffect->Index() ) : -1 );
	msg.WriteLong( started );
}

/*
=================
idEntityFx::ReadFromSnapshot
=================
*/
void idEntityFx::ReadFromSnapshot( const idBitMsg& msg )
{
	int fx_index, start_time, max_lapse;

	GetPhysics()->ReadFromSnapshot( msg );
	ReadBindFromSnapshot( msg );
	fx_index = gameLocal.ClientRemapDecl( DECL_FX, msg.ReadLong() );
	start_time = msg.ReadLong();

	if( fx_index != -1 && start_time > 0 && !fxEffect && started < 0 )
	{
		spawnArgs.GetInt( "effect_lapse", "1000", max_lapse );
		if( gameLocal.time - start_time > max_lapse )
		{
			// too late, skip the effect completely
			started = 0;
			return;
		}
		const idDeclFX* fx = static_cast<const idDeclFX*>( declManager->DeclByIndex( DECL_FX, fx_index ) );
		if( !fx )
		{
			gameLocal.Error( "FX at index %d not found", fx_index );
		}
		fxEffect = fx;
		Setup( fx->GetName() );
		Start( start_time );
	}
}

/*
=================
idEntityFx::ClientThink
=================
*/
void idEntityFx::ClientThink( const int curTime, const float fraction, const bool predict )
{

	if( gameLocal.isNewFrame )
	{
		Run( gameLocal.serverTime );
	}

	InterpolatePhysics( fraction );
	Present();
}

/*
=================
idEntityFx::ClientPredictionThink
=================
*/
void idEntityFx::ClientPredictionThink()
{
	if( gameLocal.isNewFrame )
	{
		Run( gameLocal.time );
	}
	RunPhysics();
	Present();
}

/*
===============================================================================

  idTeleporter

===============================================================================
*/

CLASS_DECLARATION( idEntityFx, idTeleporter )
EVENT( EV_Fx_Action,	idTeleporter::Event_DoAction )
END_CLASS

/*
================
idTeleporter::Event_DoAction
================
*/
void idTeleporter::Event_DoAction( idEntity* activator )
{
	float angle;

	angle = spawnArgs.GetFloat( "angle" );
	idAngles a( 0, spawnArgs.GetFloat( "angle" ), 0 );
	activator->Teleport( GetPhysics()->GetOrigin(), a, NULL );
}
