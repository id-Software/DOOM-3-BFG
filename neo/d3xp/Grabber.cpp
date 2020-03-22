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
#include "Misc.h"

#define MAX_DRAG_TRACE_DISTANCE			384.0f
#define TRACE_BOUNDS_SIZE				3.f
#define HOLD_DISTANCE					72.f
#define FIRING_DELAY					1000.0f
#define DRAG_FAIL_LEN					64.f
#define	THROW_SCALE						1000
#define MAX_PICKUP_VELOCITY				1500 * 1500
#define MAX_PICKUP_SIZE					96

/*
===============================================================================

	Allows entities to be dragged through the world with physics.

===============================================================================
*/

CLASS_DECLARATION( idEntity, idGrabber )
END_CLASS

/*
==============
idGrabber::idGrabber
==============
*/
idGrabber::idGrabber()
{
	dragEnt = NULL;
	owner = NULL;
	beam = NULL;
	beamTarget = NULL;
	oldImpulseSequence = 0;
	shakeForceFlip = false;
	holdingAF = false;
	endTime = 0;
	lastFiredTime = -FIRING_DELAY;
	dragFailTime = 0;
	startDragTime = 0;
	warpId = -1;
	dragTraceDist = MAX_DRAG_TRACE_DISTANCE;
}

/*
==============
idGrabber::~idGrabber
==============
*/
idGrabber::~idGrabber()
{
	StopDrag( true );
	if( beam )
	{
		delete beam;
	}
	if( beamTarget )
	{
		delete beamTarget;
	}
}

/*
==============
idGrabber::Save
==============
*/
void idGrabber::Save( idSaveGame* savefile ) const
{

	dragEnt.Save( savefile );
	savefile->WriteStaticObject( drag );

	savefile->WriteVec3( saveGravity );
	savefile->WriteInt( id );

	savefile->WriteVec3( localPlayerPoint );

	owner.Save( savefile );

	savefile->WriteBool( holdingAF );
	savefile->WriteBool( shakeForceFlip );

	savefile->WriteInt( endTime );
	savefile->WriteInt( lastFiredTime );
	savefile->WriteInt( dragFailTime );
	savefile->WriteInt( startDragTime );
	savefile->WriteFloat( dragTraceDist );
	savefile->WriteInt( savedContents );
	savefile->WriteInt( savedClipmask );

	savefile->WriteObject( beam );
	savefile->WriteObject( beamTarget );

	savefile->WriteInt( warpId );
}

/*
==============
idGrabber::Restore
==============
*/
void idGrabber::Restore( idRestoreGame* savefile )
{
	//Spawn the beams
	Initialize();

	dragEnt.Restore( savefile );
	savefile->ReadStaticObject( drag );

	savefile->ReadVec3( saveGravity );
	savefile->ReadInt( id );

	// Restore the drag force's physics object
	if( dragEnt.IsValid() )
	{
		drag.SetPhysics( dragEnt.GetEntity()->GetPhysics(), id, dragEnt.GetEntity()->GetPhysics()->GetOrigin() );
	}

	savefile->ReadVec3( localPlayerPoint );

	owner.Restore( savefile );

	savefile->ReadBool( holdingAF );
	savefile->ReadBool( shakeForceFlip );

	savefile->ReadInt( endTime );
	savefile->ReadInt( lastFiredTime );
	savefile->ReadInt( dragFailTime );
	savefile->ReadInt( startDragTime );
	savefile->ReadFloat( dragTraceDist );
	savefile->ReadInt( savedContents );
	savefile->ReadInt( savedClipmask );

	savefile->ReadObject( reinterpret_cast<idClass*&>( beam ) );
	savefile->ReadObject( reinterpret_cast<idClass*&>( beamTarget ) );

	savefile->ReadInt( warpId );
}

/*
==============
idGrabber::Initialize
==============
*/
void idGrabber::Initialize()
{
	if( !common->IsMultiplayer() )
	{
		idDict args;

		if( !beamTarget )
		{
			args.SetVector( "origin", vec3_origin );
			args.SetBool( "start_off", true );
			beamTarget = ( idBeam* )gameLocal.SpawnEntityType( idBeam::Type, &args );
		}

		if( !beam )
		{
			args.Clear();
			args.Set( "target", beamTarget->name.c_str() );
			args.SetVector( "origin", vec3_origin );
			args.SetBool( "start_off", true );
			args.Set( "width", "6" );
			args.Set( "skin", "textures/smf/flareSizeable" );
			args.Set( "_color", "0.0235 0.843 0.969 0.2" );
			beam = ( idBeam* )gameLocal.SpawnEntityType( idBeam::Type, &args );
			beam->SetShaderParm( 6, 1.0f );
		}

		endTime = 0;
		dragTraceDist = MAX_DRAG_TRACE_DISTANCE;
	}
	else
	{
		beam = NULL;
		beamTarget = NULL;
		endTime = 0;
		dragTraceDist = MAX_DRAG_TRACE_DISTANCE;
	};
}

/*
==============
idGrabber::SetDragDistance
==============
*/
void idGrabber::SetDragDistance( float dist )
{
	dragTraceDist = dist;
}

/*
==============
idGrabber::StartDrag
==============
*/
void idGrabber::StartDrag( idEntity* grabEnt, int id )
{
	int clipModelId = id;
	idPlayer* thePlayer = owner.GetEntity();

	holdingAF = false;
	dragFailTime = gameLocal.slow.time;
	startDragTime = gameLocal.slow.time;

	oldImpulseSequence = thePlayer->usercmd.impulseSequence;

	// set grabbed state for networking
	grabEnt->SetGrabbedState( true );

	// This is the new object to drag around
	dragEnt = grabEnt;

	// Show the beams!
	UpdateBeams();
	if( beam )
	{
		beam->Show();
	}
	if( beamTarget )
	{
		beamTarget->Show();
	}

	// Move the object to the fast group (helltime)
	grabEnt->timeGroup = TIME_GROUP2;

	// Handle specific class types
	if( grabEnt->IsType( idProjectile::Type ) )
	{
		idProjectile* p = ( idProjectile* )grabEnt;

		p->CatchProjectile( thePlayer, "_catch" );

		// Make the projectile non-solid to other projectiles/enemies (special hack for helltime hunter)
		if( !idStr::Cmp( grabEnt->GetEntityDefName(), "projectile_helltime_killer" ) )
		{
			savedContents = CONTENTS_PROJECTILE;
			savedClipmask = MASK_SHOT_RENDERMODEL | CONTENTS_PROJECTILE;
		}
		else
		{
			savedContents = grabEnt->GetPhysics()->GetContents();
			savedClipmask = grabEnt->GetPhysics()->GetClipMask();
		}
		grabEnt->GetPhysics()->SetContents( 0 );
		grabEnt->GetPhysics()->SetClipMask( CONTENTS_SOLID | CONTENTS_BODY );

	}
	else if( grabEnt->IsType( idExplodingBarrel::Type ) )
	{
		idExplodingBarrel* ebarrel = static_cast<idExplodingBarrel*>( grabEnt );

		ebarrel->StartBurning();

	}
	else if( grabEnt->IsType( idAFEntity_Gibbable::Type ) )
	{
		holdingAF = true;
		clipModelId = 0;

		if( grabbableAI( grabEnt->spawnArgs.GetString( "classname" ) ) )
		{
			idAI* aiEnt = static_cast<idAI*>( grabEnt );

			aiEnt->StartRagdoll();
		}
	}
	else if( grabEnt->IsType( idMoveableItem::Type ) )
	{
		// RB: 64 bit fixes, changed NULL to 0
		grabEnt->PostEventMS( &EV_Touch, 250, thePlayer, 0 );
		// RB end
	}

	// Get the current physics object to manipulate
	idPhysics* phys = grabEnt->GetPhysics();

	// Turn off gravity on object
	saveGravity = phys->GetGravity();
	phys->SetGravity( vec3_origin );

	// hold it directly in front of player
	localPlayerPoint = ( thePlayer->firstPersonViewAxis[0] * HOLD_DISTANCE ) * thePlayer->firstPersonViewAxis.Transpose();

	// Set the ending time for the hold
	endTime = gameLocal.time + g_grabberHoldSeconds.GetFloat() * 1000;

	// Start up the Force_Drag to bring it in
	drag.Init( g_grabberDamping.GetFloat() );
	drag.SetPhysics( phys, clipModelId, thePlayer->firstPersonViewOrigin + localPlayerPoint * thePlayer->firstPersonViewAxis );

	// start the screen warp
	warpId = thePlayer->playerView.AddWarp( phys->GetOrigin(), SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, 160, 2000 );
}

/*
==============
idGrabber::StopDrag
==============
*/
void idGrabber::StopDrag( bool dropOnly )
{
	idPlayer* thePlayer = owner.GetEntity();

	if( beam )
	{
		beam->Hide();
	}
	if( beamTarget )
	{
		beamTarget->Hide();
	}

	if( dragEnt.IsValid() )
	{
		idEntity* ent = dragEnt.GetEntity();

		// set grabbed state for networking
		ent->SetGrabbedState( false );

		// If a cinematic has started, allow dropped object to think in cinematics
		if( gameLocal.inCinematic )
		{
			ent->cinematic = true;
		}

		// Restore Gravity
		ent->GetPhysics()->SetGravity( saveGravity );

		// Move the object back to the slow group (helltime)
		ent->timeGroup = TIME_GROUP1;

		if( holdingAF )
		{
			idAFEntity_Gibbable* af = static_cast<idAFEntity_Gibbable*>( ent );
			idPhysics_AF*	af_Phys = static_cast<idPhysics_AF*>( af->GetPhysics() );

			if( grabbableAI( ent->spawnArgs.GetString( "classname" ) ) )
			{
				idAI* aiEnt = static_cast<idAI*>( ent );

				aiEnt->Damage( thePlayer, thePlayer, vec3_origin, "damage_suicide", 1.0f, INVALID_JOINT );
			}

			af->SetThrown( !dropOnly );

			// Reset timers so that it isn't forcibly put to rest in mid-air
			af_Phys->PutToRest();
			af_Phys->Activate();

			af_Phys->SetTimeScaleRamp( MS2SEC( gameLocal.slow.time ) - 1.5f, MS2SEC( gameLocal.slow.time ) + 1.0f );
		}

		// If the object isn't near its goal, just drop it in place.
		if( !ent->IsType( idProjectile::Type ) && ( dropOnly || drag.GetDistanceToGoal() > DRAG_FAIL_LEN ) )
		{
			ent->GetPhysics()->SetLinearVelocity( vec3_origin );
			thePlayer->StartSoundShader( declManager->FindSound( "grabber_maindrop" ), SND_CHANNEL_WEAPON, 0, false, NULL );

			if( ent->IsType( idExplodingBarrel::Type ) )
			{
				idExplodingBarrel* ebarrel = static_cast<idExplodingBarrel*>( ent );

				ebarrel->SetStability( true );
				ebarrel->StopBurning();
			}
		}
		else
		{
			// Shoot the object forward
			ent->ApplyImpulse( thePlayer, 0, ent->GetPhysics()->GetOrigin(), thePlayer->firstPersonViewAxis[0] * THROW_SCALE * ent->GetPhysics()->GetMass() );
			thePlayer->StartSoundShader( declManager->FindSound( "grabber_release" ), SND_CHANNEL_WEAPON, 0, false, NULL );

			// Orient projectiles away from the player
			if( ent->IsType( idProjectile::Type ) )
			{
				idPlayer* player = owner.GetEntity();
				idAngles ang = player->firstPersonViewAxis[0].ToAngles();

				ang.pitch += 90.f;
				ent->GetPhysics()->SetAxis( ang.ToMat3() );
				ent->GetPhysics()->SetAngularVelocity( vec3_origin );

				// Restore projectile contents
				ent->GetPhysics()->SetContents( savedContents );
				ent->GetPhysics()->SetClipMask( savedClipmask );

				idProjectile* projectile = static_cast< idProjectile* >( ent );
				if( projectile != NULL )
				{
					projectile->SetLaunchedFromGrabber( true );
				}

			}
			else if( ent->IsType( idMoveable::Type ) )
			{
				// Turn on damage for this object
				idMoveable* obj = static_cast<idMoveable*>( ent );
				obj->EnableDamage( true, 2.5f );
				obj->SetAttacker( thePlayer );

				if( ent->IsType( idExplodingBarrel::Type ) )
				{
					idExplodingBarrel* ebarrel = static_cast<idExplodingBarrel*>( ent );
					ebarrel->SetStability( false );
				}

			}
			else if( ent->IsType( idMoveableItem::Type ) )
			{
				ent->GetPhysics()->SetClipMask( MASK_MONSTERSOLID );
			}
		}

		// Remove the Force_Drag's control of the entity
		drag.RemovePhysics( ent->GetPhysics() );
	}

	if( warpId != -1 )
	{
		thePlayer->playerView.FreeWarp( warpId );
		warpId = -1;
	}

	lastFiredTime = gameLocal.time;
	dragEnt = NULL;
	endTime = 0;
}

/*
==============
idGrabber::Update
==============
*/
int idGrabber::Update( idPlayer* player, bool hide )
{
	trace_t trace;
	idEntity* newEnt;

	// pause before allowing refire
	if( lastFiredTime + FIRING_DELAY > gameLocal.time )
	{
		return 3;
	}

	// Dead players release the trigger
	if( hide || player->health <= 0 )
	{
		StopDrag( true );
		if( hide )
		{
			lastFiredTime = gameLocal.time - FIRING_DELAY + 250;
		}
		return 3;
	}

	// Check if object being held has been removed (dead demon, projectile, etc.)
	if( endTime > gameLocal.time )
	{
		bool abort = !dragEnt.IsValid();

		if( !abort && dragEnt.GetEntity()->IsType( idProjectile::Type ) )
		{
			idProjectile* proj = ( idProjectile* )dragEnt.GetEntity();

			if( proj->GetProjectileState() >= 3 )
			{
				abort = true;
			}
		}
		if( !abort && dragEnt.GetEntity() && dragEnt.GetEntity()->IsHidden() )
		{
			abort = true;
		}
		// Not in multiplayer :: Pressing "reload" lets you carefully drop an item
		if( !common->IsMultiplayer() && !abort && ( player->usercmd.impulseSequence != oldImpulseSequence ) && ( player->usercmd.impulse == IMPULSE_13 ) )
		{
			abort = true;
		}

		if( abort )
		{
			StopDrag( true );
			return 3;
		}
	}

	owner = player;

	// if no entity selected for dragging
	if( !dragEnt.GetEntity() )
	{
		idBounds bounds;
		idVec3 end = player->firstPersonViewOrigin + player->firstPersonViewAxis[0] * dragTraceDist;

		bounds.Zero();
		bounds.ExpandSelf( TRACE_BOUNDS_SIZE );

		gameLocal.clip.TraceBounds( trace, player->firstPersonViewOrigin, end, bounds, MASK_SHOT_RENDERMODEL | CONTENTS_PROJECTILE | CONTENTS_MOVEABLECLIP, player );
		// If the trace hit something
		if( trace.fraction < 1.0f )
		{
			newEnt = gameLocal.entities[ trace.c.entityNum ];

			// if entity is already being grabbed then bypass
			if( common->IsMultiplayer() && newEnt && newEnt->IsGrabbed() )
			{
				return 0;
			}

			// Check if this is a valid entity to hold
			if( newEnt && ( newEnt->IsType( idMoveable::Type ) ||
							newEnt->IsType( idMoveableItem::Type ) ||
							newEnt->IsType( idProjectile::Type ) ||
							newEnt->IsType( idAFEntity_Gibbable::Type )
						  ) &&
					newEnt->noGrab == false &&
					newEnt->GetPhysics()->GetBounds().GetRadius() < MAX_PICKUP_SIZE &&
					newEnt->GetPhysics()->GetLinearVelocity().LengthSqr() < MAX_PICKUP_VELOCITY )
			{

				bool validAF = true;

				if( newEnt->IsType( idAFEntity_Gibbable::Type ) )
				{
					idAFEntity_Gibbable* afEnt = static_cast<idAFEntity_Gibbable*>( newEnt );

					if( grabbableAI( newEnt->spawnArgs.GetString( "classname" ) ) )
					{
						// Make sure it's also active
						if( !afEnt->IsActive() )
						{
							validAF = false;
						}
					}
					else if( !afEnt->IsActiveAF() )
					{
						validAF = false;
					}
				}

				if( validAF && player->usercmd.buttons & BUTTON_ATTACK )
				{
					// Grab this entity and start dragging it around
					StartDrag( newEnt, trace.c.id );
				}
				else if( validAF )
				{
					// A holdable object is ready to be grabbed
					return 1;
				}
			}
		}
	}

	// check backwards server time in multiplayer
	bool allow = true;

	if( common->IsMultiplayer() )
	{

		// if we've marched backwards
		if( gameLocal.slow.time < startDragTime )
		{
			allow = false;
		}
	}


	// if there is an entity selected for dragging
	if( dragEnt.GetEntity() && allow )
	{
		idPhysics* entPhys = dragEnt.GetEntity()->GetPhysics();
		idVec3 goalPos;

		// If the player lets go of attack, or time is up
		if( !( player->usercmd.buttons & BUTTON_ATTACK ) )
		{
			StopDrag( false );
			return 3;
		}
		if( gameLocal.time > endTime )
		{
			StopDrag( true );
			return 3;
		}

		// Check if the player is standing on the object
		if( !holdingAF )
		{
			idBounds	playerBounds;
			idBounds	objectBounds = entPhys->GetAbsBounds();
			idVec3		newPoint = player->GetPhysics()->GetOrigin();

			// create a bounds at the players feet
			playerBounds.Clear();
			playerBounds.AddPoint( newPoint );
			newPoint.z -= 1.f;
			playerBounds.AddPoint( newPoint );
			playerBounds.ExpandSelf( 8.f );

			// If it intersects the object bounds, then drop it
			if( playerBounds.IntersectsBounds( objectBounds ) )
			{
				StopDrag( true );
				return 3;
			}
		}

		// Shake the object at the end of the hold
		if( g_grabberEnableShake.GetBool() && !common->IsMultiplayer() )
		{
			ApplyShake();
		}

		// Set and evaluate drag force
		goalPos = player->firstPersonViewOrigin + localPlayerPoint * player->firstPersonViewAxis;

		drag.SetGoalPosition( goalPos );
		drag.Evaluate( gameLocal.time );

		// If an object is flying too fast toward the player, stop it hard
		if( g_grabberHardStop.GetBool() )
		{
			idPlane theWall;
			idVec3 toPlayerVelocity, objectCenter;
			float toPlayerSpeed;

			toPlayerVelocity = -player->firstPersonViewAxis[0];
			toPlayerSpeed = entPhys->GetLinearVelocity() * toPlayerVelocity;

			if( toPlayerSpeed > 64.f )
			{
				objectCenter = entPhys->GetAbsBounds().GetCenter();

				theWall.SetNormal( player->firstPersonViewAxis[0] );
				theWall.FitThroughPoint( goalPos );

				if( theWall.Side( objectCenter, 0.1f ) == PLANESIDE_BACK )
				{
					int i, num;

					num = entPhys->GetNumClipModels();
					for( i = 0; i < num; i++ )
					{
						entPhys->SetLinearVelocity( vec3_origin, i );
					}
				}
			}

			// Make sure the object isn't spinning too fast
			const float MAX_ROTATION_SPEED = 12.f;

			idVec3	angVel = entPhys->GetAngularVelocity();
			float	rotationSpeed = angVel.LengthFast();

			if( rotationSpeed > MAX_ROTATION_SPEED )
			{
				angVel.NormalizeFast();
				angVel *= MAX_ROTATION_SPEED;
				entPhys->SetAngularVelocity( angVel );
			}
		}

		// Orient projectiles away from the player
		if( dragEnt.GetEntity()->IsType( idProjectile::Type ) )
		{
			idAngles ang = player->firstPersonViewAxis[0].ToAngles();
			ang.pitch += 90.f;
			entPhys->SetAxis( ang.ToMat3() );
		}

		// Some kind of effect from gun to object?
		UpdateBeams();

		// If the object is stuck away from its intended position for more than 500ms, let it go.
		if( drag.GetDistanceToGoal() > DRAG_FAIL_LEN )
		{
			if( dragFailTime < ( gameLocal.slow.time - 500 ) )
			{
				StopDrag( true );
				return 3;
			}
		}
		else
		{
			dragFailTime = gameLocal.slow.time;
		}

		// Currently holding an object
		return 2;
	}

	// Not holding, nothing to hold
	return 0;
}

/*
======================
idGrabber::UpdateBeams
======================
*/
void idGrabber::UpdateBeams()
{
	jointHandle_t	muzzle_joint;
	idVec3	muzzle_origin;
	idMat3	muzzle_axis;
	renderEntity_t* re;

	if( !beam )
	{
		return;
	}

	if( dragEnt.IsValid() )
	{
		idPlayer* thePlayer = owner.GetEntity();

		if( beamTarget )
		{
			beamTarget->SetOrigin( dragEnt.GetEntity()->GetPhysics()->GetAbsBounds().GetCenter() );
		}

		muzzle_joint = thePlayer->weapon.GetEntity()->GetAnimator()->GetJointHandle( "particle_upper" );
		if( muzzle_joint != INVALID_JOINT )
		{
			thePlayer->weapon.GetEntity()->GetJointWorldTransform( muzzle_joint, gameLocal.time, muzzle_origin, muzzle_axis );
		}
		else
		{
			muzzle_origin = thePlayer->GetPhysics()->GetOrigin();
		}

		beam->SetOrigin( muzzle_origin );
		re = beam->GetRenderEntity();
		re->origin = muzzle_origin;

		beam->UpdateVisuals();
		beam->Present();
	}
}

/*
==============
idGrabber::ApplyShake
==============
*/
void idGrabber::ApplyShake()
{
	float u = 1 - ( float )( endTime - gameLocal.time ) / ( g_grabberHoldSeconds.GetFloat() * 1000 );

	if( u >= 0.8f )
	{
		idVec3 point, impulse;
		float shakeForceMagnitude = 450.f;
		float mass = dragEnt.GetEntity()->GetPhysics()->GetMass();

		shakeForceFlip = !shakeForceFlip;

		// get point to rotate around
		point = dragEnt.GetEntity()->GetPhysics()->GetOrigin();
		point.y += 1;

		// Articulated figures get less violent shake
		if( holdingAF )
		{
			shakeForceMagnitude = 120.f;
		}

		// calc impulse
		if( shakeForceFlip )
		{
			impulse.Set( 0, 0, shakeForceMagnitude * u * mass );
		}
		else
		{
			impulse.Set( 0, 0, -shakeForceMagnitude * u * mass );
		}

		dragEnt.GetEntity()->ApplyImpulse( NULL, 0, point, impulse );
	}
}

/*
==============
idGrabber::grabbableAI
==============
*/
bool idGrabber::grabbableAI( const char* aiName )
{
	// skip "monster_"
	aiName += 8;

	if( !idStr::Cmpn( aiName, "flying_lostsoul", 15 ) ||
			!idStr::Cmpn( aiName, "demon_trite", 11 ) ||
			!idStr::Cmp( aiName, "flying_forgotten" ) ||
			!idStr::Cmp( aiName, "demon_cherub" ) ||
			!idStr::Cmp( aiName, "demon_tick" ) )
	{

		return true;
	}

	return false;
}

