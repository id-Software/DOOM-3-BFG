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
/*

  SecurityCamera.cpp

  Security camera that triggers targets when player is in view

*/

#include "precompiled.h"
#pragma hdrstop

#include "Game_local.h"


/***********************************************************************

  idSecurityCamera

***********************************************************************/

const idEventDef EV_SecurityCam_ReverseSweep( "<reverseSweep>" );
const idEventDef EV_SecurityCam_ContinueSweep( "<continueSweep>" );
const idEventDef EV_SecurityCam_Pause( "<pause>" );
const idEventDef EV_SecurityCam_Alert( "<alert>" );
const idEventDef EV_SecurityCam_AddLight( "<addLight>" );

CLASS_DECLARATION( idEntity, idSecurityCamera )
EVENT( EV_SecurityCam_ReverseSweep,		idSecurityCamera::Event_ReverseSweep )
EVENT( EV_SecurityCam_ContinueSweep,	idSecurityCamera::Event_ContinueSweep )
EVENT( EV_SecurityCam_Pause,			idSecurityCamera::Event_Pause )
EVENT( EV_SecurityCam_Alert,			idSecurityCamera::Event_Alert )
EVENT( EV_SecurityCam_AddLight,			idSecurityCamera::Event_AddLight )
END_CLASS

/*
================
idSecurityCamera::Save
================
*/
void idSecurityCamera::Save( idSaveGame* savefile ) const
{
	savefile->WriteFloat( angle );
	savefile->WriteFloat( sweepAngle );
	savefile->WriteInt( modelAxis );
	savefile->WriteBool( flipAxis );
	savefile->WriteFloat( scanDist );
	savefile->WriteFloat( scanFov );
	
	savefile->WriteFloat( sweepStart );
	savefile->WriteFloat( sweepEnd );
	savefile->WriteBool( negativeSweep );
	savefile->WriteBool( sweeping );
	savefile->WriteInt( alertMode );
	savefile->WriteFloat( stopSweeping );
	savefile->WriteFloat( scanFovCos );
	
	savefile->WriteVec3( viewOffset );
	
	savefile->WriteInt( pvsArea );
	savefile->WriteStaticObject( physicsObj );
	savefile->WriteTraceModel( trm );
}

/*
================
idSecurityCamera::Restore
================
*/
void idSecurityCamera::Restore( idRestoreGame* savefile )
{
	savefile->ReadFloat( angle );
	savefile->ReadFloat( sweepAngle );
	savefile->ReadInt( modelAxis );
	savefile->ReadBool( flipAxis );
	savefile->ReadFloat( scanDist );
	savefile->ReadFloat( scanFov );
	
	savefile->ReadFloat( sweepStart );
	savefile->ReadFloat( sweepEnd );
	savefile->ReadBool( negativeSweep );
	savefile->ReadBool( sweeping );
	savefile->ReadInt( alertMode );
	savefile->ReadFloat( stopSweeping );
	savefile->ReadFloat( scanFovCos );
	
	savefile->ReadVec3( viewOffset );
	
	savefile->ReadInt( pvsArea );
	savefile->ReadStaticObject( physicsObj );
	savefile->ReadTraceModel( trm );
}

/*
================
idSecurityCamera::Spawn
================
*/
void idSecurityCamera::Spawn()
{
	idStr	str;
	
	sweepAngle	= spawnArgs.GetFloat( "sweepAngle", "90" );
	health		= spawnArgs.GetInt( "health", "100" );
	scanFov		= spawnArgs.GetFloat( "scanFov", "90" );
	scanDist	= spawnArgs.GetFloat( "scanDist", "200" );
	flipAxis	= spawnArgs.GetBool( "flipAxis" );
	
	modelAxis	= spawnArgs.GetInt( "modelAxis" );
	if( modelAxis < 0 || modelAxis > 2 )
	{
		modelAxis = 0;
	}
	
	spawnArgs.GetVector( "viewOffset", "0 0 0", viewOffset );
	
	if( spawnArgs.GetBool( "spotLight" ) )
	{
		PostEventMS( &EV_SecurityCam_AddLight, 0 );
	}
	
	negativeSweep = ( sweepAngle < 0 ) ? true : false;
	sweepAngle = idMath::Fabs( sweepAngle );
	
	scanFovCos = cos( scanFov * idMath::PI / 360.0f );
	
	angle = GetPhysics()->GetAxis().ToAngles().yaw;
	StartSweep();
	SetAlertMode( SCANNING );
	BecomeActive( TH_THINK );
	
	if( health )
	{
		fl.takedamage = true;
	}
	
	pvsArea = gameLocal.pvs.GetPVSArea( GetPhysics()->GetOrigin() );
	// if no target specified use ourself
	str = spawnArgs.GetString( "cameraTarget" );
	if( str.Length() == 0 )
	{
		spawnArgs.Set( "cameraTarget", spawnArgs.GetString( "name" ) );
	}
	
	// check if a clip model is set
	spawnArgs.GetString( "clipmodel", "", str );
	if( !str[0] )
	{
		str = spawnArgs.GetString( "model" );		// use the visual model
	}
	
	if( !collisionModelManager->TrmFromModel( str, trm ) )
	{
		gameLocal.Error( "idSecurityCamera '%s': cannot load collision model %s", name.c_str(), str.c_str() );
		return;
	}
	
	GetPhysics()->SetContents( CONTENTS_SOLID );
	GetPhysics()->SetClipMask( MASK_SOLID | CONTENTS_BODY | CONTENTS_CORPSE | CONTENTS_MOVEABLECLIP );
	// setup the physics
	UpdateChangeableSpawnArgs( NULL );
}

/*
================
idSecurityCamera::Event_AddLight
================
*/
void idSecurityCamera::Event_AddLight()
{
	idDict	args;
	idVec3	right, up, target, temp;
	idVec3	dir;
	float	radius;
	idVec3	lightOffset;
	idLight*	spotLight;
	
	dir = GetAxis();
	dir.NormalVectors( right, up );
	target = GetPhysics()->GetOrigin() + dir * scanDist;
	
	radius = tan( scanFov * idMath::PI / 360.0f );
	up = dir + up * radius;
	up.Normalize();
	up = GetPhysics()->GetOrigin() + up * scanDist;
	up -= target;
	
	right = dir + right * radius;
	right.Normalize();
	right = GetPhysics()->GetOrigin() + right * scanDist;
	right -= target;
	
	spawnArgs.GetVector( "lightOffset", "0 0 0", lightOffset );
	
	args.Set( "origin", ( GetPhysics()->GetOrigin() + lightOffset ).ToString() );
	args.Set( "light_target", target.ToString() );
	args.Set( "light_right", right.ToString() );
	args.Set( "light_up", up.ToString() );
	args.SetFloat( "angle", GetPhysics()->GetAxis()[0].ToYaw() );
	
	spotLight = static_cast<idLight*>( gameLocal.SpawnEntityType( idLight::Type, &args ) );
	spotLight->Bind( this, true );
	spotLight->UpdateVisuals();
}

/*
================
idSecurityCamera::DrawFov
================
*/
void idSecurityCamera::DrawFov()
{
	int i;
	float radius, a, s, c, halfRadius;
	idVec3 right, up;
	idVec4 color( 1, 0, 0, 1 ), color2( 0, 0, 1, 1 );
	idVec3 lastPoint, point, lastHalfPoint, halfPoint, center;
	
	idVec3 dir = GetAxis();
	dir.NormalVectors( right, up );
	
	radius = tan( scanFov * idMath::PI / 360.0f );
	halfRadius = radius * 0.5f;
	lastPoint = dir + up * radius;
	lastPoint.Normalize();
	lastPoint = GetPhysics()->GetOrigin() + lastPoint * scanDist;
	lastHalfPoint = dir + up * halfRadius;
	lastHalfPoint.Normalize();
	lastHalfPoint = GetPhysics()->GetOrigin() + lastHalfPoint * scanDist;
	center = GetPhysics()->GetOrigin() + dir * scanDist;
	for( i = 1; i < 12; i++ )
	{
		a = idMath::TWO_PI * i / 12.0f;
		idMath::SinCos( a, s, c );
		point = dir + right * s * radius + up * c * radius;
		point.Normalize();
		point = GetPhysics()->GetOrigin() + point * scanDist;
		gameRenderWorld->DebugLine( color, lastPoint, point );
		gameRenderWorld->DebugLine( color, GetPhysics()->GetOrigin(), point );
		lastPoint = point;
		
		halfPoint = dir + right * s * halfRadius + up * c * halfRadius;
		halfPoint.Normalize();
		halfPoint = GetPhysics()->GetOrigin() + halfPoint * scanDist;
		gameRenderWorld->DebugLine( color2, point, halfPoint );
		gameRenderWorld->DebugLine( color2, lastHalfPoint, halfPoint );
		lastHalfPoint = halfPoint;
		
		gameRenderWorld->DebugLine( color2, halfPoint, center );
	}
}

/*
================
idSecurityCamera::GetRenderView
================
*/
renderView_t* idSecurityCamera::GetRenderView()
{
	renderView_t* rv = idEntity::GetRenderView();
	rv->fov_x = scanFov;
	rv->fov_y = scanFov;
	rv->viewaxis = GetAxis().ToAngles().ToMat3();
	rv->vieworg = GetPhysics()->GetOrigin() + viewOffset;
	return rv;
}

/*
================
idSecurityCamera::CanSeePlayer
================
*/
bool idSecurityCamera::CanSeePlayer()
{
	int i;
	float dist;
	idPlayer* ent;
	trace_t tr;
	idVec3 dir;
	pvsHandle_t handle;
	
	handle = gameLocal.pvs.SetupCurrentPVS( pvsArea );
	
	for( i = 0; i < gameLocal.numClients; i++ )
	{
		ent = static_cast<idPlayer*>( gameLocal.entities[ i ] );
		
		if( !ent || ( ent->fl.notarget ) )
		{
			continue;
		}
		
		// if there is no way we can see this player
		if( !gameLocal.pvs.InCurrentPVS( handle, ent->GetPVSAreas(), ent->GetNumPVSAreas() ) )
		{
			continue;
		}
		
		dir = ent->GetPhysics()->GetOrigin() - GetPhysics()->GetOrigin();
		dist = dir.Normalize();
		
		if( dist > scanDist )
		{
			continue;
		}
		
		if( dir * GetAxis() < scanFovCos )
		{
			continue;
		}
		
		idVec3 eye;
		
		eye = ent->EyeOffset();
		
		gameLocal.clip.TracePoint( tr, GetPhysics()->GetOrigin(), ent->GetPhysics()->GetOrigin() + eye, MASK_OPAQUE, this );
		if( tr.fraction == 1.0 || ( gameLocal.GetTraceEntity( tr ) == ent ) )
		{
			gameLocal.pvs.FreeCurrentPVS( handle );
			return true;
		}
	}
	
	gameLocal.pvs.FreeCurrentPVS( handle );
	
	return false;
}

/*
================
idSecurityCamera::SetAlertMode
================
*/
void idSecurityCamera::SetAlertMode( int alert )
{
	if( alert >= SCANNING && alert <= ACTIVATED )
	{
		alertMode = alert;
	}
	renderEntity.shaderParms[ SHADERPARM_MODE ] = alertMode;
	UpdateVisuals();
}

/*
================
idSecurityCamera::Think
================
*/
void idSecurityCamera::Think()
{
	float pct;
	float travel;
	
	if( thinkFlags & TH_THINK )
	{
		if( g_showEntityInfo.GetBool() )
		{
			DrawFov();
		}
		
		if( health <= 0 )
		{
			BecomeInactive( TH_THINK );
			return;
		}
	}
	
	// run physics
	RunPhysics();
	
	if( thinkFlags & TH_THINK )
	{
		if( CanSeePlayer() )
		{
			if( alertMode == SCANNING )
			{
				float	sightTime;
				
				SetAlertMode( ALERT );
				stopSweeping = gameLocal.time;
				if( sweeping )
				{
					CancelEvents( &EV_SecurityCam_Pause );
				}
				else
				{
					CancelEvents( &EV_SecurityCam_ReverseSweep );
				}
				sweeping = false;
				StopSound( SND_CHANNEL_ANY, false );
				StartSound( "snd_sight", SND_CHANNEL_BODY, 0, false, NULL );
				
				sightTime = spawnArgs.GetFloat( "sightTime", "5" );
				PostEventSec( &EV_SecurityCam_Alert, sightTime );
			}
		}
		else
		{
			if( alertMode == ALERT )
			{
				float	sightResume;
				
				SetAlertMode( LOSINGINTEREST );
				CancelEvents( &EV_SecurityCam_Alert );
				
				sightResume = spawnArgs.GetFloat( "sightResume", "1.5" );
				PostEventSec( &EV_SecurityCam_ContinueSweep, sightResume );
			}
			
			if( sweeping )
			{
				idAngles a = GetPhysics()->GetAxis().ToAngles();
				
				pct = ( gameLocal.time - sweepStart ) / ( sweepEnd - sweepStart );
				travel = pct * sweepAngle;
				if( negativeSweep )
				{
					a.yaw = angle + travel;
				}
				else
				{
					a.yaw = angle - travel;
				}
				
				SetAngles( a );
			}
		}
	}
	Present();
}

/*
================
idSecurityCamera::GetAxis
================
*/
const idVec3 idSecurityCamera::GetAxis() const
{
	return ( flipAxis ) ? -GetPhysics()->GetAxis()[modelAxis] : GetPhysics()->GetAxis()[modelAxis];
};

/*
================
idSecurityCamera::SweepSpeed
================
*/
float idSecurityCamera::SweepSpeed() const
{
	return spawnArgs.GetFloat( "sweepSpeed", "5" );
}

/*
================
idSecurityCamera::StartSweep
================
*/
void idSecurityCamera::StartSweep()
{
	int speed;
	
	sweeping = true;
	sweepStart = gameLocal.time;
	speed = SEC2MS( SweepSpeed() );
	sweepEnd = sweepStart + speed;
	PostEventMS( &EV_SecurityCam_Pause, speed );
	StartSound( "snd_moving", SND_CHANNEL_BODY, 0, false, NULL );
}

/*
================
idSecurityCamera::Event_ContinueSweep
================
*/
void idSecurityCamera::Event_ContinueSweep()
{
	float pct = ( stopSweeping - sweepStart ) / ( sweepEnd - sweepStart );
	float f = gameLocal.time - ( sweepEnd - sweepStart ) * pct;
	int speed;
	
	sweepStart = f;
	speed = MS2SEC( SweepSpeed() );
	sweepEnd = sweepStart + speed;
	PostEventMS( &EV_SecurityCam_Pause, speed * ( 1.0 - pct ) );
	StartSound( "snd_moving", SND_CHANNEL_BODY, 0, false, NULL );
	SetAlertMode( SCANNING );
	sweeping = true;
}

/*
================
idSecurityCamera::Event_Alert
================
*/
void idSecurityCamera::Event_Alert()
{
	float	wait;
	
	SetAlertMode( ACTIVATED );
	StopSound( SND_CHANNEL_ANY, false );
	StartSound( "snd_activate", SND_CHANNEL_BODY, 0, false, NULL );
	ActivateTargets( this );
	CancelEvents( &EV_SecurityCam_ContinueSweep );
	
	wait = spawnArgs.GetFloat( "wait", "20" );
	PostEventSec( &EV_SecurityCam_ContinueSweep, wait );
}

/*
================
idSecurityCamera::Event_ReverseSweep
================
*/
void idSecurityCamera::Event_ReverseSweep()
{
	angle = GetPhysics()->GetAxis().ToAngles().yaw;
	negativeSweep = !negativeSweep;
	StartSweep();
}

/*
================
idSecurityCamera::Event_Pause
================
*/
void idSecurityCamera::Event_Pause()
{
	float	sweepWait;
	
	sweepWait = spawnArgs.GetFloat( "sweepWait", "0.5" );
	sweeping = false;
	StopSound( SND_CHANNEL_ANY, false );
	StartSound( "snd_stop", SND_CHANNEL_BODY, 0, false, NULL );
	PostEventSec( &EV_SecurityCam_ReverseSweep, sweepWait );
}

/*
============
idSecurityCamera::Killed
============
*/
void idSecurityCamera::Killed( idEntity* inflictor, idEntity* attacker, int damage, const idVec3& dir, int location )
{
	sweeping = false;
	StopSound( SND_CHANNEL_ANY, false );
	const char* fx = spawnArgs.GetString( "fx_destroyed" );
	if( fx[0] != '\0' )
	{
		idEntityFx::StartFx( fx, NULL, NULL, this, true );
	}
	
	physicsObj.SetSelf( this );
	physicsObj.SetClipModel( new( TAG_PHYSICS_CLIP_ENTITY ) idClipModel( trm ), 0.02f );
	physicsObj.SetOrigin( GetPhysics()->GetOrigin() );
	physicsObj.SetAxis( GetPhysics()->GetAxis() );
	physicsObj.SetBouncyness( 0.2f );
	physicsObj.SetFriction( 0.6f, 0.6f, 0.2f );
	physicsObj.SetGravity( gameLocal.GetGravity() );
	physicsObj.SetContents( CONTENTS_SOLID );
	physicsObj.SetClipMask( MASK_SOLID | CONTENTS_BODY | CONTENTS_CORPSE | CONTENTS_MOVEABLECLIP );
	SetPhysics( &physicsObj );
	physicsObj.DropToFloor();
}


/*
============
idSecurityCamera::Pain
============
*/
bool idSecurityCamera::Pain( idEntity* inflictor, idEntity* attacker, int damage, const idVec3& dir, int location )
{
	const char* fx = spawnArgs.GetString( "fx_damage" );
	if( fx[0] != '\0' )
	{
		idEntityFx::StartFx( fx, NULL, NULL, this, true );
	}
	return true;
}


/*
================
idSecurityCamera::Present

Present is called to allow entities to generate refEntities, lights, etc for the renderer.
================
*/
void idSecurityCamera::Present()
{
	// don't present to the renderer if the entity hasn't changed
	if( !( thinkFlags & TH_UPDATEVISUALS ) )
	{
		return;
	}
	BecomeInactive( TH_UPDATEVISUALS );
	
	// camera target for remote render views
	if( cameraTarget )
	{
		renderEntity.remoteRenderView = cameraTarget->GetRenderView();
	}
	
	// if set to invisible, skip
	if( !renderEntity.hModel || IsHidden() )
	{
		return;
	}
	
	// add to refresh list
	if( modelDefHandle == -1 )
	{
		modelDefHandle = gameRenderWorld->AddEntityDef( &renderEntity );
	}
	else
	{
		gameRenderWorld->UpdateEntityDef( modelDefHandle, &renderEntity );
	}
}
