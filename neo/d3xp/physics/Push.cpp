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

#include "../Game_local.h"


/*
============
idPush::InitSavingPushedEntityPositions
============
*/
void idPush::InitSavingPushedEntityPositions()
{
	numPushed = 0;
}

/*
============
idPush::SaveEntityPosition
============
*/
void idPush::SaveEntityPosition( idEntity* ent )
{
	int i;
	
	// if already saved the physics state for this entity
	for( i = 0; i < numPushed; i++ )
	{
		if( pushed[i].ent == ent )
		{
			return;
		}
	}
	
	// don't overflow
	if( numPushed >= MAX_GENTITIES )
	{
		gameLocal.Error( "more than MAX_GENTITIES pushed entities" );
		return;
	}
	
	pushed[numPushed].ent = ent;
	
	// if the entity is an actor
	if( ent->IsType( idActor::Type ) )
	{
		// save the delta view angles
		pushed[numPushed].deltaViewAngles = static_cast<idActor*>( ent )->GetDeltaViewAngles();
	}
	
	// save the physics state
	ent->GetPhysics()->SaveState();
	
	numPushed++;
}

/*
============
idPush::RestorePushedEntityPositions
============
*/
void idPush::RestorePushedEntityPositions()
{
	int i;
	
	for( i = 0; i < numPushed; i++ )
	{
	
		// if the entity is an actor
		if( pushed[i].ent->IsType( idActor::Type ) )
		{
			// set back the delta view angles
			static_cast<idActor*>( pushed[i].ent )->SetDeltaViewAngles( pushed[i].deltaViewAngles );
		}
		
		// restore the physics state
		pushed[i].ent->GetPhysics()->RestoreState();
	}
}

/*
============
idPush::RotateEntityToAxial
============
*/
bool idPush::RotateEntityToAxial( idEntity* ent, idVec3 rotationPoint )
{
	int i;
	trace_t trace;
	idRotation rotation;
	idMat3 axis;
	idPhysics* physics;
	
	physics = ent->GetPhysics();
	axis = physics->GetAxis();
	if( !axis.IsRotated() )
	{
		return true;
	}
	// try to rotate the bbox back to axial with at most four rotations
	for( i = 0; i < 4; i++ )
	{
		axis = physics->GetAxis();
		rotation = axis.ToRotation();
		rotation.Scale( -1 );
		rotation.SetOrigin( rotationPoint );
		// tiny float numbers in the clip axis, this can get the entity stuck
		if( rotation.GetAngle() == 0.0f )
		{
			physics->SetAxis( mat3_identity );
			return true;
		}
		//
		ent->GetPhysics()->ClipRotation( trace, rotation, NULL );
		// if the full rotation is possible
		if( trace.fraction >= 1.0f )
		{
			// set bbox in final axial position
			physics->SetOrigin( trace.endpos );
			physics->SetAxis( mat3_identity );
			return true;
		}
		// if partial rotation was possible
		else if( trace.fraction > 0.0f )
		{
			// partial rotation
			physics->SetOrigin( trace.endpos );
			physics->SetAxis( trace.endAxis );
		}
		// next rotate around collision point
		rotationPoint = trace.c.point;
	}
	return false;
}

#ifdef NEW_PUSH

/*
============
idPush::CanPushEntity
============
*/
bool idPush::CanPushEntity( idEntity* ent, idEntity* pusher, idEntity* initialPusher, const int flags )
{

	// if the physics object is not pushable
	if( !ent->GetPhysics()->IsPushable() )
	{
		return false;
	}
	
	// if the entity doesn't clip with this pusher
	if( !( ent->GetPhysics()->GetClipMask() & pusher->GetPhysics()->GetContents() ) )
	{
		return false;
	}
	
	// don't push players in noclip mode
	if( ent->client && ent->client->noclip )
	{
		return false;
	}
	
	// if we should only push idMoveable entities
	if( ( flags & PUSHFL_ONLYMOVEABLE ) && !ent->IsType( idMoveable::Type ) )
	{
		return false;
	}
	
	// if we shouldn't push entities the original pusher rests upon
	if( flags & PUSHFL_NOGROUNDENTITIES )
	{
		if( initialPusher->GetPhysics()->IsGroundEntity( ent->entityNumber ) )
		{
			return false;
		}
	}
	
	return true;
}

/*
============
idPush::AddEntityToPushedGroup
============
*/
void idPush::AddEntityToPushedGroup( idEntity* ent, float fraction, bool groundContact )
{
	int i, j;
	
	for( i = 0; i < pushedGroupSize; i++ )
	{
		if( ent == pushedGroup[i].ent )
		{
			if( fraction > pushedGroup[i].fraction )
			{
				pushedGroup[i].fraction = fraction;
				pushedGroup[i].groundContact &= groundContact;
				pushedGroup[i].test = true;
			}
			return;
		}
		else if( fraction > pushedGroup[i].fraction )
		{
			for( j = pushedGroupSize; j > i; j-- )
			{
				pushedGroup[j] = pushedGroup[j - 1];
			}
			break;
		}
	}
	
	// put the entity in the group
	pushedGroupSize++;
	pushedGroup[i].ent = ent;
	pushedGroup[i].fraction = fraction;
	pushedGroup[i].groundContact = groundContact;
	pushedGroup[i].test = true;
	
	// remove any further occurances of the same entity in the group
	for( i++; i < pushedGroupSize; i++ )
	{
		if( ent == pushedGroup[i].ent )
		{
			for( j = i + 1; j < pushedGroupSize; j++ )
			{
				pushedGroup[j - 1] = pushedGroup[j];
			}
			pushedGroupSize--;
			break;
		}
	}
}

/*
============
idPush::IsFullyPushed
============
*/
bool idPush::IsFullyPushed( idEntity* ent )
{
	int i;
	
	for( i = 0; i < pushedGroupSize; i++ )
	{
		if( pushedGroup[i].fraction < 1.0f )
		{
			return false;
		}
		if( ent == pushedGroup[i].ent )
		{
			return true;
		}
	}
	return false;
}

/*
============
idPush::ClipTranslationAgainstPusher
============
*/
bool idPush::ClipTranslationAgainstPusher( trace_t& results, idEntity* ent, idEntity* pusher, const idVec3& translation )
{
	int i, n;
	trace_t t;
	
	results.fraction = 1.0f;
	
	n = pusher->GetPhysics()->GetNumClipModels();
	for( i = 0; i < n; i++ )
	{
		ent->GetPhysics()->ClipTranslation( t, translation, pusher->GetPhysics()->GetClipModel( i ) );
		if( t.fraction < results.fraction )
		{
			results = t;
		}
	}
	return ( results.fraction < 1.0f );
}

/*
============
idPush::GetPushableEntitiesForTranslation
============
*/
int idPush::GetPushableEntitiesForTranslation( idEntity* pusher, idEntity* initialPusher, const int flags,
		const idVec3& translation, idEntity* entityList[], int maxEntities )
{
	int i, n, l;
	idBounds bounds, pushBounds;
	idPhysics* physics;
	
	// get bounds for the whole movement
	physics = pusher->GetPhysics();
	bounds = physics->GetBounds();
	pushBounds.FromBoundsTranslation( bounds, physics->GetOrigin(), physics->GetAxis(), translation );
	pushBounds.ExpandSelf( 2.0f );
	
	// get all entities within the push bounds
	n = gameLocal.clip.EntitiesTouchingBounds( pushBounds, -1, entityList, MAX_GENTITIES );
	
	for( l = i = 0; i < n; i++ )
	{
		if( entityList[i] == pusher || entityList[i] == initialPusher )
		{
			continue;
		}
		if( CanPushEntity( entityList[i], pusher, initialPusher, flags ) )
		{
			entityList[l++] = entityList[i];
		}
	}
	
	return l;
}

/*
============
idPush::ClipTranslationalPush

  Try to push other entities by translating the given entity.
============
*/
float idPush::ClipTranslationalPush( trace_t& results, idEntity* pusher, const int flags,
									 const idVec3& newOrigin, const idVec3& translation )
{
	int i, j, numListedEntities;
	idEntity* curPusher, *ent, *entityList[ MAX_GENTITIES ];
	float fraction;
	bool groundContact, blocked = false;
	float totalMass;
	trace_t trace;
	idVec3 realTranslation, partialTranslation;
	
	totalMass = 0.0f;
	
	results.fraction = 1.0f;
	results.endpos = newOrigin;
	results.endAxis = pusher->GetPhysics()->GetAxis();
	memset( results.c, 0, sizeof( results.c ) );
	
	if( translation == vec3_origin )
	{
		return totalMass;
	}
	
	// clip against all non-pushable physics objects
	if( flags & PUSHFL_CLIP )
	{
	
		numListedEntities = GetPushableEntitiesForTranslation( pusher, pusher, flags, translation, entityList, MAX_GENTITIES );
		// disable pushable entities for collision detection
		for( i = 0; i < numListedEntities; i++ )
		{
			entityList[i]->GetPhysics()->DisableClip();
		}
		// clip translation
		pusher->GetPhysics()->ClipTranslation( results, translation, NULL );
		// enable pushable entities
		for( i = 0; i < numListedEntities; i++ )
		{
			entityList[i]->GetPhysics()->EnableClip();
		}
		if( results.fraction == 0.0f )
		{
			return totalMass;
		}
		realTranslation = results.fraction * translation;
	}
	else
	{
		realTranslation = translation;
	}
	
	// put the pusher in the group of pushed physics objects
	pushedGroup[0].ent = pusher;
	pushedGroup[0].fraction = 1.0f;
	pushedGroup[0].groundContact = true;
	pushedGroup[0].test = true;
	pushedGroupSize = 1;
	
	// get all physics objects that need to be pushed
	for( i = 0; i < pushedGroupSize; )
	{
		if( !pushedGroup[i].test )
		{
			i++;
			continue;
		}
		pushedGroup[i].test = false;
		curPusher = pushedGroup[i].ent;
		fraction = pushedGroup[i].fraction;
		groundContact = pushedGroup[i].groundContact;
		i = 0;
		
		numListedEntities = GetPushableEntitiesForTranslation( curPusher, pusher, flags, realTranslation, entityList, MAX_GENTITIES );
		
		for( j = 0; j < numListedEntities; j++ )
		{
			ent = entityList[ j ];
			
			if( IsFullyPushed( ent ) )
			{
				continue;
			}
			
			if( !CanPushEntity( ent, curPusher, pusher, flags ) )
			{
				continue;
			}
			
			if( ent->GetPhysics()->IsGroundEntity( curPusher->entityNumber ) )
			{
				AddEntityToPushedGroup( ent, 1.0f * fraction, false );
			}
			else if( ClipTranslationAgainstPusher( trace, ent, curPusher, -fraction * realTranslation ) )
			{
				AddEntityToPushedGroup( ent, ( 1.0f - trace.fraction ) * fraction, groundContact );
			}
		}
	}
	
	// save physics states and disable physics objects for collision detection
	for( i = 0; i < pushedGroupSize; i++ )
	{
		SaveEntityPosition( pushedGroup[i].ent );
		pushedGroup[i].ent->GetPhysics()->DisableClip();
	}
	
	// clip all pushed physics objects
	for( i = 1; i < pushedGroupSize; i++ )
	{
		partialTranslation = realTranslation * pushedGroup[i].fraction;
		
		pushedGroup[i].ent->GetPhysics()->ClipTranslation( trace, partialTranslation, NULL );
		
		if( trace.fraction < 1.0f )
		{
			blocked = true;
			break;
		}
	}
	
	// enable all physics objects for collision detection
	for( i = 1; i < pushedGroupSize; i++ )
	{
		pushedGroup[i].ent->GetPhysics()->EnableClip();
	}
	
	// push all or nothing
	if( blocked )
	{
		if( flags & PUSHFL_CLIP )
		{
			pusher->GetPhysics()->ClipTranslation( results, realTranslation, NULL );
		}
		else
		{
			results.fraction = 0.0f;
			results.endpos = pusher->GetPhysics()->GetOrigin();
			results.endAxis = pusher->GetPhysics()->GetAxis();
		}
	}
	else
	{
		// translate all pushed physics objects
		for( i = 1; i < pushedGroupSize; i++ )
		{
			partialTranslation = realTranslation * pushedGroup[i].fraction;
			pushedGroup[i].ent->GetPhysics()->Translate( partialTranslation );
			totalMass += pushedGroup[i].ent->GetPhysics()->GetMass();
		}
		// translate the clip models of the pusher
		for( i = 0; i < pusher->GetPhysics()->GetNumClipModels(); i++ )
		{
			pusher->GetPhysics()->GetClipModel( i )->Translate( results.fraction * realTranslation );
			pusher->GetPhysics()->GetClipModel( i )->Link( gameLocal.clip );
		}
	}
	
	return totalMass;
}

/*
============
idPush::ClipRotationAgainstPusher
============
*/
bool idPush::ClipRotationAgainstPusher( trace_t& results, idEntity* ent, idEntity* pusher, const idRotation& rotation )
{
	int i, n;
	trace_t t;
	
	results.fraction = 1.0f;
	
	n = pusher->GetPhysics()->GetNumClipModels();
	for( i = 0; i < n; i++ )
	{
		ent->GetPhysics()->ClipRotation( t, rotation, pusher->GetPhysics()->GetClipModel( i ) );
		if( t.fraction < results.fraction )
		{
			results = t;
		}
	}
	return ( results.fraction < 1.0f );
}

/*
============
idPush::GetPushableEntitiesForRotation
============
*/
int idPush::GetPushableEntitiesForRotation( idEntity* pusher, idEntity* initialPusher, const int flags,
		const idRotation& rotation, idEntity* entityList[], int maxEntities )
{
	int i, n, l;
	idBounds bounds, pushBounds;
	idPhysics* physics;
	
	// get bounds for the whole movement
	physics = pusher->GetPhysics();
	bounds = physics->GetBounds();
	pushBounds.FromBoundsRotation( bounds, physics->GetOrigin(), physics->GetAxis(), rotation );
	pushBounds.ExpandSelf( 2.0f );
	
	// get all entities within the push bounds
	n = gameLocal.clip.EntitiesTouchingBounds( pushBounds, -1, entityList, MAX_GENTITIES );
	
	for( l = i = 0; i < n; i++ )
	{
		if( entityList[i] == pusher || entityList[i] == initialPusher )
		{
			continue;
		}
		if( CanPushEntity( entityList[i], pusher, initialPusher, flags ) )
		{
			entityList[l++] = entityList[i];
		}
	}
	
	return l;
}

/*
============
idPush::ClipRotationalPush

  Try to push other entities by rotating the given entity.
============
*/
float idPush::ClipRotationalPush( trace_t& results, idEntity* pusher, const int flags,
								  const idMat3& newAxis, const idRotation& rotation )
{
	int i, j, numListedEntities;
	idEntity* curPusher, *ent, *entityList[ MAX_GENTITIES ];
	float fraction;
	bool groundContact, blocked = false;
	float totalMass;
	trace_t trace;
	idRotation realRotation, partialRotation;
	idMat3 oldAxis;
	
	totalMass = 0.0f;
	
	results.fraction = 1.0f;
	results.endpos = pusher->GetPhysics()->GetOrigin();
	results.endAxis = newAxis;
	memset( results.c, 0, sizeof( results.c ) );
	
	if( !rotation.GetAngle() )
	{
		return totalMass;
	}
	
	// clip against all non-pushable physics objects
	if( flags & PUSHFL_CLIP )
	{
	
		numListedEntities = GetPushableEntitiesForRotation( pusher, pusher, flags, rotation, entityList, MAX_GENTITIES );
		// disable pushable entities for collision detection
		for( i = 0; i < numListedEntities; i++ )
		{
			entityList[i]->GetPhysics()->DisableClip();
		}
		// clip rotation
		pusher->GetPhysics()->ClipRotation( results, rotation, NULL );
		// enable pushable entities
		for( i = 0; i < numListedEntities; i++ )
		{
			entityList[i]->GetPhysics()->EnableClip();
		}
		if( results.fraction == 0.0f )
		{
			return totalMass;
		}
		realRotation = results.fraction * rotation;
	}
	else
	{
		realRotation = rotation;
	}
	
	// put the pusher in the group of pushed physics objects
	pushedGroup[0].ent = pusher;
	pushedGroup[0].fraction = 1.0f;
	pushedGroup[0].groundContact = true;
	pushedGroup[0].test = true;
	pushedGroupSize = 1;
	
	// get all physics objects that need to be pushed
	for( i = 0; i < pushedGroupSize; )
	{
		if( !pushedGroup[i].test )
		{
			i++;
			continue;
		}
		pushedGroup[i].test = false;
		curPusher = pushedGroup[i].ent;
		fraction = pushedGroup[i].fraction;
		groundContact = pushedGroup[i].groundContact;
		i = 0;
		
		numListedEntities = GetPushableEntitiesForRotation( curPusher, pusher, flags, realRotation, entityList, MAX_GENTITIES );
		
		for( j = 0; j < numListedEntities; j++ )
		{
			ent = entityList[ j ];
			
			if( IsFullyPushed( ent ) )
			{
				continue;
			}
			
			if( ent->GetPhysics()->IsGroundEntity( curPusher->entityNumber ) )
			{
				AddEntityToPushedGroup( ent, 1.0f * fraction, false );
			}
			else if( ClipRotationAgainstPusher( trace, ent, curPusher, -fraction * realRotation ) )
			{
				AddEntityToPushedGroup( ent, ( 1.0f - trace.fraction ) * fraction, groundContact );
			}
		}
	}
	
	// save physics states and disable physics objects for collision detection
	for( i = 1; i < pushedGroupSize; i++ )
	{
		SaveEntityPosition( pushedGroup[i].ent );
		pushedGroup[i].ent->GetPhysics()->DisableClip();
	}
	
	// clip all pushed physics objects
	for( i = 1; i < pushedGroupSize; i++ )
	{
		partialRotation = realRotation * pushedGroup[i].fraction;
		
		pushedGroup[i].ent->GetPhysics()->ClipRotation( trace, partialRotation, NULL );
		
		if( trace.fraction < 1.0f )
		{
			blocked = true;
			break;
		}
	}
	
	// enable all physics objects for collision detection
	for( i = 1; i < pushedGroupSize; i++ )
	{
		pushedGroup[i].ent->GetPhysics()->EnableClip();
	}
	
	// push all or nothing
	if( blocked )
	{
		if( flags & PUSHFL_CLIP )
		{
			pusher->GetPhysics()->ClipRotation( results, realRotation, NULL );
		}
		else
		{
			results.fraction = 0.0f;
			results.endpos = pusher->GetPhysics()->GetOrigin();
			results.endAxis = pusher->GetPhysics()->GetAxis();
		}
	}
	else
	{
		// rotate all pushed physics objects
		for( i = 1; i < pushedGroupSize; i++ )
		{
			partialRotation = realRotation * pushedGroup[i].fraction;
			pushedGroup[i].ent->GetPhysics()->Rotate( partialRotation );
			totalMass += pushedGroup[i].ent->GetPhysics()->GetMass();
		}
		// rotate the clip models of the pusher
		for( i = 0; i < pusher->GetPhysics()->GetNumClipModels(); i++ )
		{
			pusher->GetPhysics()->GetClipModel( i )->Rotate( realRotation );
			pusher->GetPhysics()->GetClipModel( i )->Link( gameLocal.clip );
			pusher->GetPhysics()->GetClipModel( i )->Enable();
		}
		// rotate any actors back to axial
		for( i = 1; i < pushedGroupSize; i++ )
		{
			// if the entity is using actor physics
			if( pushedGroup[i].ent->GetPhysics()->IsType( idPhysics_Actor::Type ) )
			{
			
				// rotate the collision model back to axial
				if( !RotateEntityToAxial( pushedGroup[i].ent, pushedGroup[i].ent->GetPhysics()->GetOrigin() ) )
				{
					// don't allow rotation if the bbox is no longer axial
					results.fraction = 0.0f;
					results.endpos = pusher->GetPhysics()->GetOrigin();
					results.endAxis = pusher->GetPhysics()->GetAxis();
				}
			}
		}
	}
	
	return totalMass;
}

#else /* !NEW_PUSH */

enum
{
	PUSH_NO,			// not pushed
	PUSH_OK,			// pushed ok
	PUSH_BLOCKED		// blocked
};

/*
============
idPush::ClipEntityRotation
============
*/
void idPush::ClipEntityRotation( trace_t& trace, const idEntity* ent, const idClipModel* clipModel, idClipModel* skip, const idRotation& rotation )
{

	if( skip )
	{
		skip->Disable();
	}
	
	ent->GetPhysics()->ClipRotation( trace, rotation, clipModel );
	
	if( skip )
	{
		skip->Enable();
	}
}

/*
============
idPush::ClipEntityTranslation
============
*/
void idPush::ClipEntityTranslation( trace_t& trace, const idEntity* ent, const idClipModel* clipModel, idClipModel* skip, const idVec3& translation )
{

	if( skip )
	{
		skip->Disable();
	}
	
	ent->GetPhysics()->ClipTranslation( trace, translation, clipModel );
	
	if( skip )
	{
		skip->Enable();
	}
}

/*
============
idPush::TryRotatePushEntity
============
*/
#ifdef _DEBUG
//	#define ROTATIONAL_PUSH_DEBUG
#endif

int idPush::TryRotatePushEntity( trace_t& results, idEntity* check, idClipModel* clipModel, const int flags,
								 const idMat3& newAxis, const idRotation& rotation )
{
	trace_t trace;
	idVec3 rotationPoint;
	idRotation newRotation;
	float checkAngle;
	idPhysics* physics;
	
	physics = check->GetPhysics();
	
#ifdef ROTATIONAL_PUSH_DEBUG
	bool startsolid = false;
	if( physics->ClipContents( clipModel ) )
	{
		startsolid = true;
	}
#endif
	
	results.fraction = 1.0f;
	results.endpos = clipModel->GetOrigin();
	results.endAxis = newAxis;
	memset( &results.c, 0, sizeof( results.c ) );
	
	// always pushed when standing on the pusher
	if( physics->IsGroundClipModel( clipModel->GetEntity()->entityNumber, clipModel->GetId() ) )
	{
		// rotate the entity colliding with all other entities except the pusher itself
		ClipEntityRotation( trace, check, NULL, clipModel, rotation );
		// if there is a collision
		if( trace.fraction < 1.0f )
		{
			// angle along which the entity is pushed
			checkAngle = rotation.GetAngle() * trace.fraction;
			// test if the entity can stay at it's partly pushed position by rotating
			// the entity in reverse only colliding with pusher
			newRotation.Set( rotation.GetOrigin(), rotation.GetVec(), -( rotation.GetAngle() - checkAngle ) );
			ClipEntityRotation( results, check, clipModel, NULL, newRotation );
			// if there is a collision
			if( results.fraction < 1.0f )
			{
			
				// FIXME: try to push the blocking entity as well or try to slide along collision plane(s)?
				
				results.c.normal = -results.c.normal;
				results.c.dist = -results.c.dist;
				
				// the entity will be crushed between the pusher and some other entity
				return PUSH_BLOCKED;
			}
		}
		else
		{
			// angle along which the entity is pushed
			checkAngle = rotation.GetAngle();
		}
		// point to rotate entity bbox around back to axial
		rotationPoint = physics->GetOrigin();
	}
	else
	{
		// rotate entity in reverse only colliding with pusher
		newRotation = rotation;
		newRotation.Scale( -1 );
		//
		ClipEntityRotation( results, check, clipModel, NULL, newRotation );
		// if no collision with the pusher then the entity is not pushed by the pusher
		if( results.fraction >= 1.0f )
		{
#ifdef ROTATIONAL_PUSH_DEBUG
			// set pusher into final position
			clipModel->Link( gameLocal.clip, clipModel->GetEntity(), clipModel->GetId(), clipModel->GetOrigin(), newAxis );
			if( physics->ClipContents( clipModel ) )
			{
				if( !startsolid )
				{
					int bah = 1;
				}
			}
#endif
			return PUSH_NO;
		}
		// get point to rotate bbox around back to axial
		rotationPoint = results.c.point;
		// angle along which the entity will be pushed
		checkAngle = rotation.GetAngle() * ( 1.0f - results.fraction );
		// rotate the entity colliding with all other entities except the pusher itself
		newRotation.Set( rotation.GetOrigin(), rotation.GetVec(), checkAngle );
		ClipEntityRotation( trace, check, NULL, clipModel, newRotation );
		// if there is a collision
		if( trace.fraction < 1.0f )
		{
		
			// FIXME: try to push the blocking entity as well or try to slide along collision plane(s)?
			
			results.c.normal = -results.c.normal;
			results.c.dist = -results.c.dist;
			
			// the entity will be crushed between the pusher and some other entity
			return PUSH_BLOCKED;
		}
	}
	
	SaveEntityPosition( check );
	
	newRotation.Set( rotation.GetOrigin(), rotation.GetVec(), checkAngle );
	// NOTE:	this code prevents msvc 6.0 & 7.0 from screwing up the above code in
	//			release builds moving less floats than it should
	static float shit = checkAngle;
	
	newRotation.RotatePoint( rotationPoint );
	
	// rotate the entity
	physics->Rotate( newRotation );
	
	// set pusher into final position
	clipModel->Link( gameLocal.clip, clipModel->GetEntity(), clipModel->GetId(), clipModel->GetOrigin(), newAxis );
	
#ifdef ROTATIONAL_PUSH_DEBUG
	if( physics->ClipContents( clipModel ) )
	{
		if( !startsolid )
		{
			int bah = 1;
		}
	}
#endif
	
	// if the entity uses actor physics
	if( physics->IsType( idPhysics_Actor::Type ) )
	{
	
		// rotate the collision model back to axial
		if( !RotateEntityToAxial( check, rotationPoint ) )
		{
			// don't allow rotation if the bbox is no longer axial
			return PUSH_BLOCKED;
		}
	}
	
#ifdef ROTATIONAL_PUSH_DEBUG
	if( physics->ClipContents( clipModel ) )
	{
		if( !startsolid )
		{
			int bah = 1;
		}
	}
#endif
	
	// if the entity is an actor using actor physics
	if( check->IsType( idActor::Type ) && physics->IsType( idPhysics_Actor::Type ) )
	{
	
		// if the entity is standing ontop of the pusher
		if( physics->IsGroundClipModel( clipModel->GetEntity()->entityNumber, clipModel->GetId() ) )
		{
			// rotate actor view
			idActor* actor = static_cast<idActor*>( check );
			idAngles delta = actor->GetDeltaViewAngles();
			delta.yaw += newRotation.ToMat3()[0].ToYaw();
			actor->SetDeltaViewAngles( delta );
		}
	}
	
	return PUSH_OK;
}

/*
============
idPush::TryTranslatePushEntity
============
*/
#ifdef _DEBUG
//	#define TRANSLATIONAL_PUSH_DEBUG
#endif

int idPush::TryTranslatePushEntity( trace_t& results, idEntity* check, idClipModel* clipModel, const int flags,
									const idVec3& newOrigin, const idVec3& move )
{
	trace_t		trace;
	idVec3		checkMove;
	idVec3		oldOrigin;
	idPhysics*	physics;
	
	physics = check->GetPhysics();
	
#ifdef TRANSLATIONAL_PUSH_DEBUG
	bool startsolid = false;
	if( physics->ClipContents( clipModel ) )
	{
		startsolid = true;
	}
#endif
	
	results.fraction = 1.0f;
	results.endpos = newOrigin;
	results.endAxis = clipModel->GetAxis();
	memset( &results.c, 0, sizeof( results.c ) );
	
	// always pushed when standing on the pusher
	if( physics->IsGroundClipModel( clipModel->GetEntity()->entityNumber, clipModel->GetId() ) )
	{
		// move the entity colliding with all other entities except the pusher itself
		ClipEntityTranslation( trace, check, NULL, clipModel, move );
		// if there is a collision
		if( trace.fraction < 1.0f )
		{
			// vector along which the entity is pushed
			checkMove = move * trace.fraction;
			// test if the entity can stay at it's partly pushed position by moving the entity in reverse only colliding with pusher
			ClipEntityTranslation( results, check, clipModel, NULL, -( move - checkMove ) );
			// if there is a collision
			if( results.fraction < 1.0f )
			{
			
				// FIXME: try to push the blocking entity as well or try to slide along collision plane(s)?
				
				results.c.normal = -results.c.normal;
				results.c.dist = -results.c.dist;
				
				// the entity will be crushed between the pusher and some other entity
				return PUSH_BLOCKED;
			}
		}
		else
		{
			// vector along which the entity is pushed
			checkMove = move;
		}
	}
	else
	{
		// move entity in reverse only colliding with pusher
		ClipEntityTranslation( results, check, clipModel, NULL, -move );
		// if no collision with the pusher then the entity is not pushed by the pusher
		if( results.fraction >= 1.0f )
		{
			return PUSH_NO;
		}
		// vector along which the entity is pushed
		checkMove = move * ( 1.0f - results.fraction );
		// move the entity colliding with all other entities except the pusher itself
		ClipEntityTranslation( trace, check, NULL, clipModel, checkMove );
		// if there is a collisions
		if( trace.fraction < 1.0f )
		{
		
			results.c.normal = -results.c.normal;
			results.c.dist = -results.c.dist;
			
			// FIXME: try to push the blocking entity as well ?
			// FIXME: handle sliding along more than one collision plane ?
			// FIXME: this code has issues, player pushing box into corner in "maps/mre/aaron/test.map"
			
			/*
						oldOrigin = physics->GetOrigin();
			
						// movement still remaining
						checkMove *= (1.0f - trace.fraction);
			
						// project the movement along the collision plane
						if ( !checkMove.ProjectAlongPlane( trace.c.normal, 0.1f, 1.001f ) ) {
							return PUSH_BLOCKED;
						}
						checkMove *= 1.001f;
			
						// move entity from collision point along the collision plane
						physics->SetOrigin( trace.endpos );
						ClipEntityTranslation( trace, check, NULL, NULL, checkMove );
			
						if ( trace.fraction < 1.0f ) {
							physics->SetOrigin( oldOrigin );
							return PUSH_BLOCKED;
						}
			
						checkMove = trace.endpos - oldOrigin;
			
						// move entity in reverse only colliding with pusher
						physics->SetOrigin( trace.endpos );
						ClipEntityTranslation( trace, check, clipModel, NULL, -move );
			
						physics->SetOrigin( oldOrigin );
			*/
			if( trace.fraction < 1.0f )
			{
				return PUSH_BLOCKED;
			}
		}
	}
	
	SaveEntityPosition( check );
	
	// translate the entity
	physics->Translate( checkMove );
	
#ifdef TRANSLATIONAL_PUSH_DEBUG
	// set the pusher in the translated position
	clipModel->Link( gameLocal.clip, clipModel->GetEntity(), clipModel->GetId(), newOrigin, clipModel->GetAxis() );
	if( physics->ClipContents( clipModel ) )
	{
		if( !startsolid )
		{
			int bah = 1;
		}
	}
#endif
	
	return PUSH_OK;
}

/*
============
idPush::DiscardEntities
============
*/
int idPush::DiscardEntities( idEntity* entityList[], int numEntities, int flags, idEntity* pusher )
{
	int i, num;
	idEntity* check;
	
	// remove all entities we cannot or should not push from the list
	for( num = i = 0; i < numEntities; i++ )
	{
		check = entityList[ i ];
		
		// if the physics object is not pushable
		if( !check->GetPhysics()->IsPushable() )
		{
			continue;
		}
		
		// if the entity doesn't clip with this pusher
		if( !( check->GetPhysics()->GetClipMask() & pusher->GetPhysics()->GetContents() ) )
		{
			continue;
		}
		
		// don't push players in noclip mode
		if( check->IsType( idPlayer::Type ) && static_cast<idPlayer*>( check )->noclip )
		{
			continue;
		}
		
		// if we should only push idMoveable entities
		if( ( flags & PUSHFL_ONLYMOVEABLE ) && !check->IsType( idMoveable::Type ) )
		{
			continue;
		}
		
		// if we shouldn't push entities the clip model rests upon
		if( flags & PUSHFL_NOGROUNDENTITIES )
		{
			if( pusher->GetPhysics()->IsGroundEntity( check->entityNumber ) )
			{
				continue;
			}
		}
		
		// keep entity in list
		entityList[ num++ ] = entityList[i];
	}
	
	return num;
}

/*
============
idPush::ClipTranslationalPush

  Try to push other entities by moving the given entity.
============
*/
float idPush::ClipTranslationalPush( trace_t& results, idEntity* pusher, const int flags,
									 const idVec3& newOrigin, const idVec3& translation )
{
	int			i, listedEntities, res;
	idEntity*	check, *entityList[ MAX_GENTITIES ];
	idBounds	bounds, pushBounds;
	idVec3		clipMove, clipOrigin, oldOrigin, dir, impulse;
	trace_t		pushResults;
	bool		wasEnabled;
	float		totalMass;
	idClipModel* clipModel;
	
	clipModel = pusher->GetPhysics()->GetClipModel();
	
	totalMass = 0.0f;
	
	results.fraction = 1.0f;
	results.endpos = newOrigin;
	results.endAxis = clipModel->GetAxis();
	memset( &results.c, 0, sizeof( results.c ) );
	
	if( translation == vec3_origin )
	{
		return totalMass;
	}
	
	dir = translation;
	dir.Normalize();
	dir.z += 1.0f;
	dir *= 10.0f;
	
	// get bounds for the whole movement
	bounds = clipModel->GetBounds();
	if( bounds[0].x >= bounds[1].x )
	{
		return totalMass;
	}
	pushBounds.FromBoundsTranslation( bounds, clipModel->GetOrigin(), clipModel->GetAxis(), translation );
	
	wasEnabled = clipModel->IsEnabled();
	
	// make sure we don't get the pushing clip model in the list
	clipModel->Disable();
	
	listedEntities = gameLocal.clip.EntitiesTouchingBounds( pushBounds, -1, entityList, MAX_GENTITIES );
	
	// discard entities we cannot or should not push
	listedEntities = DiscardEntities( entityList, listedEntities, flags, pusher );
	
	if( flags & PUSHFL_CLIP )
	{
	
		// can only clip movement of a trace model
		assert( clipModel->IsTraceModel() );
		
		// disable to be pushed entities for collision detection
		for( i = 0; i < listedEntities; i++ )
		{
			entityList[i]->GetPhysics()->DisableClip();
		}
		
		gameLocal.clip.Translation( results, clipModel->GetOrigin(), clipModel->GetOrigin() + translation, clipModel, clipModel->GetAxis(), pusher->GetPhysics()->GetClipMask(), NULL );
		
		// enable to be pushed entities for collision detection
		for( i = 0; i < listedEntities; i++ )
		{
			entityList[i]->GetPhysics()->EnableClip();
		}
		
		if( results.fraction == 0.0f )
		{
			if( wasEnabled )
			{
				clipModel->Enable();
			}
			return totalMass;
		}
		
		clipMove = results.endpos - clipModel->GetOrigin();
		clipOrigin = results.endpos;
		
	}
	else
	{
	
		clipMove = translation;
		clipOrigin = newOrigin;
	}
	
	// we have to enable the clip model because we use it during pushing
	clipModel->Enable();
	
	// save pusher old position
	oldOrigin = clipModel->GetOrigin();
	
	// try to push the entities
	for( i = 0; i < listedEntities; i++ )
	{
	
		check = entityList[ i ];
		
		idPhysics* physics = check->GetPhysics();
		
		// disable the entity for collision detection
		physics->DisableClip();
		
		res = TryTranslatePushEntity( pushResults, check, clipModel, flags, clipOrigin, clipMove );
		
		// enable the entity for collision detection
		physics->EnableClip();
		
		// if the entity is pushed
		if( res == PUSH_OK )
		{
			// set the pusher in the translated position
			clipModel->Link( gameLocal.clip, clipModel->GetEntity(), clipModel->GetId(), newOrigin, clipModel->GetAxis() );
			// the entity might be pushed off the ground
			physics->EvaluateContacts();
			// put pusher back in old position
			clipModel->Link( gameLocal.clip, clipModel->GetEntity(), clipModel->GetId(), oldOrigin, clipModel->GetAxis() );
			
			// wake up this object
			if( flags & PUSHFL_APPLYIMPULSE )
			{
				impulse = physics->GetMass() * dir;
			}
			else
			{
				impulse.Zero();
			}
			check->ApplyImpulse( clipModel->GetEntity(), clipModel->GetId(), clipModel->GetOrigin(), impulse );
			
			// add mass of pushed entity
			totalMass += physics->GetMass();
		}
		
		// if the entity is not blocking
		if( res != PUSH_BLOCKED )
		{
			continue;
		}
		
		// if the blocking entity is a projectile
		if( check->IsType( idProjectile::Type ) )
		{
			check->ProcessEvent( &EV_Explode );
			continue;
		}
		
		// if blocking entities should be crushed
		if( flags & PUSHFL_CRUSH )
		{
			check->Damage( clipModel->GetEntity(), clipModel->GetEntity(), vec3_origin, "damage_crush", 1.0f, CLIPMODEL_ID_TO_JOINT_HANDLE( pushResults.c.id ) );
			continue;
		}
		
		// if the entity is an active articulated figure and gibs
		if( check->IsType( idAFEntity_Base::Type ) && check->spawnArgs.GetBool( "gib" ) )
		{
			if( static_cast<idAFEntity_Base*>( check )->IsActiveAF() )
			{
				check->ProcessEvent( &EV_Gib, "damage_Gib" );
			}
		}
		
		// if the entity is a moveable item and gibs
		if( check->IsType( idMoveableItem::Type ) && check->spawnArgs.GetBool( "gib" ) )
		{
			check->ProcessEvent( &EV_Gib, "damage_Gib" );
		}
		
		// blocked
		results = pushResults;
		results.fraction = 0.0f;
		results.endAxis = clipModel->GetAxis();
		results.endpos = clipModel->GetOrigin();
		results.c.entityNum = check->entityNumber;
		results.c.id = 0;
		
		if( !wasEnabled )
		{
			clipModel->Disable();
		}
		
		return totalMass;
	}
	
	if( !wasEnabled )
	{
		clipModel->Disable();
	}
	
	return totalMass;
}

/*
============
idPush::ClipRotationalPush

  Try to push other entities by moving the given entity.
============
*/
float idPush::ClipRotationalPush( trace_t& results, idEntity* pusher, const int flags,
								  const idMat3& newAxis, const idRotation& rotation )
{
	int			i, listedEntities, res;
	idEntity*	check, *entityList[ MAX_GENTITIES ];
	idBounds	bounds, pushBounds;
	idRotation	clipRotation;
	idMat3		clipAxis, oldAxis;
	trace_t		pushResults;
	bool		wasEnabled;
	float		totalMass;
	idClipModel* clipModel;
	
	clipModel = pusher->GetPhysics()->GetClipModel();
	
	totalMass = 0.0f;
	
	results.fraction = 1.0f;
	results.endpos = clipModel->GetOrigin();
	results.endAxis = newAxis;
	memset( &results.c, 0, sizeof( results.c ) );
	
	if( !rotation.GetAngle() )
	{
		return totalMass;
	}
	
	// get bounds for the whole movement
	bounds = clipModel->GetBounds();
	if( bounds[0].x >= bounds[1].x )
	{
		return totalMass;
	}
	pushBounds.FromBoundsRotation( bounds, clipModel->GetOrigin(), clipModel->GetAxis(), rotation );
	
	wasEnabled = clipModel->IsEnabled();
	
	// make sure we don't get the pushing clip model in the list
	clipModel->Disable();
	
	listedEntities = gameLocal.clip.EntitiesTouchingBounds( pushBounds, -1, entityList, MAX_GENTITIES );
	
	// discard entities we cannot or should not push
	listedEntities = DiscardEntities( entityList, listedEntities, flags, pusher );
	
	if( flags & PUSHFL_CLIP )
	{
	
		// can only clip movement of a trace model
		assert( clipModel->IsTraceModel() );
		
		// disable to be pushed entities for collision detection
		for( i = 0; i < listedEntities; i++ )
		{
			entityList[i]->GetPhysics()->DisableClip();
		}
		
		gameLocal.clip.Rotation( results, clipModel->GetOrigin(), rotation, clipModel, clipModel->GetAxis(), pusher->GetPhysics()->GetClipMask(), NULL );
		
		// enable to be pushed entities for collision detection
		for( i = 0; i < listedEntities; i++ )
		{
			entityList[i]->GetPhysics()->EnableClip();
		}
		
		if( results.fraction == 0.0f )
		{
			if( wasEnabled )
			{
				clipModel->Enable();
			}
			return totalMass;
		}
		
		clipRotation = rotation * results.fraction;
		clipAxis = results.endAxis;
	}
	else
	{
	
		clipRotation = rotation;
		clipAxis = newAxis;
	}
	
	// we have to enable the clip model because we use it during pushing
	clipModel->Enable();
	
	// save pusher old position
	oldAxis = clipModel->GetAxis();
	
	// try to push all the entities
	for( i = 0; i < listedEntities; i++ )
	{
	
		check = entityList[ i ];
		
		idPhysics* physics = check->GetPhysics();
		
		// disable the entity for collision detection
		physics->DisableClip();
		
		res = TryRotatePushEntity( pushResults, check, clipModel, flags, clipAxis, clipRotation );
		
		// enable the entity for collision detection
		physics->EnableClip();
		
		// if the entity is pushed
		if( res == PUSH_OK )
		{
			// set the pusher in the rotated position
			clipModel->Link( gameLocal.clip, clipModel->GetEntity(), clipModel->GetId(), clipModel->GetOrigin(), newAxis );
			// the entity might be pushed off the ground
			physics->EvaluateContacts();
			// put pusher back in old position
			clipModel->Link( gameLocal.clip, clipModel->GetEntity(), clipModel->GetId(), clipModel->GetOrigin(), oldAxis );
			
			// wake up this object
			check->ApplyImpulse( clipModel->GetEntity(), clipModel->GetId(), clipModel->GetOrigin(), vec3_origin );
			
			// add mass of pushed entity
			totalMass += physics->GetMass();
		}
		
		// if the entity is not blocking
		if( res != PUSH_BLOCKED )
		{
			continue;
		}
		
		// if the blocking entity is a projectile
		if( check->IsType( idProjectile::Type ) )
		{
			check->ProcessEvent( &EV_Explode );
			continue;
		}
		
		// if blocking entities should be crushed
		if( flags & PUSHFL_CRUSH )
		{
			check->Damage( clipModel->GetEntity(), clipModel->GetEntity(), vec3_origin, "damage_crush", 1.0f, CLIPMODEL_ID_TO_JOINT_HANDLE( pushResults.c.id ) );
			continue;
		}
		
		// if the entity is an active articulated figure and gibs
		if( check->IsType( idAFEntity_Base::Type ) && check->spawnArgs.GetBool( "gib" ) )
		{
			if( static_cast<idAFEntity_Base*>( check )->IsActiveAF() )
			{
				check->ProcessEvent( &EV_Gib, "damage_Gib" );
			}
		}
		
		// blocked
		results = pushResults;
		results.fraction = 0.0f;
		results.endAxis = clipModel->GetAxis();
		results.endpos = clipModel->GetOrigin();
		results.c.entityNum = check->entityNumber;
		results.c.id = 0;
		
		if( !wasEnabled )
		{
			clipModel->Disable();
		}
		
		return totalMass;
	}
	
	if( !wasEnabled )
	{
		clipModel->Disable();
	}
	
	return totalMass;
}

#endif /* !NEW_PUSH */


/*
============
idPush::ClipPush

  Try to push other entities by moving the given entity.
============
*/
float idPush::ClipPush( trace_t& results, idEntity* pusher, const int flags,
						const idVec3& oldOrigin, const idMat3& oldAxis,
						idVec3& newOrigin, idMat3& newAxis )
{
	idVec3 translation;
	idRotation rotation;
	float mass;
	
	mass = 0.0f;
	
	results.fraction = 1.0f;
	results.endpos = newOrigin;
	results.endAxis = newAxis;
	memset( &results.c, 0, sizeof( results.c ) );
	
	// translational push
	translation = newOrigin - oldOrigin;
	
	// if the pusher translates
	if( translation != vec3_origin )
	{
	
		mass += ClipTranslationalPush( results, pusher, flags, newOrigin, translation );
		if( results.fraction < 1.0f )
		{
			newOrigin = oldOrigin;
			newAxis = oldAxis;
			return mass;
		}
	}
	else
	{
		newOrigin = oldOrigin;
	}
	
	// rotational push
	rotation = ( oldAxis.Transpose() * newAxis ).ToRotation();
	rotation.SetOrigin( newOrigin );
	rotation.Normalize180();
	rotation.ReCalculateMatrix();		// recalculate the rotation matrix to avoid accumulating rounding errors
	
	// if the pusher rotates
	if( rotation.GetAngle() != 0.0f )
	{
	
		// recalculate new axis to avoid floating point rounding problems
		newAxis = oldAxis * rotation.ToMat3();
		newAxis.OrthoNormalizeSelf();
		newAxis.FixDenormals();
		newAxis.FixDegeneracies();
		
		pusher->GetPhysics()->GetClipModel()->SetPosition( newOrigin, oldAxis );
		
		mass += ClipRotationalPush( results, pusher, flags, newAxis, rotation );
		if( results.fraction < 1.0f )
		{
			newOrigin = oldOrigin;
			newAxis = oldAxis;
			return mass;
		}
	}
	else
	{
		newAxis = oldAxis;
	}
	
	return mass;
}
