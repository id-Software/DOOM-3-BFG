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

  Articulated figure controller.

===============================================================================
*/
#define ARTICULATED_FIGURE_ANIM		"af_pose"
#define POSE_BOUNDS_EXPANSION		5.0f

/*
================
idAF::idAF
================
*/
idAF::idAF()
{
	self = NULL;
	animator = NULL;
	modifiedAnim = 0;
	baseOrigin.Zero();
	baseAxis.Identity();
	poseTime = -1;
	restStartTime = -1;
	isLoaded = false;
	isActive = false;
	hasBindConstraints = false;
}

/*
================
idAF::~idAF
================
*/
idAF::~idAF()
{
}

/*
================
idAF::Save
================
*/
void idAF::Save( idSaveGame* savefile ) const
{
	savefile->WriteObject( self );
	savefile->WriteString( GetName() );
	savefile->WriteBool( hasBindConstraints );
	savefile->WriteVec3( baseOrigin );
	savefile->WriteMat3( baseAxis );
	savefile->WriteInt( poseTime );
	savefile->WriteInt( restStartTime );
	savefile->WriteBool( isLoaded );
	savefile->WriteBool( isActive );
	savefile->WriteStaticObject( physicsObj );
}

/*
================
idAF::Restore
================
*/
void idAF::Restore( idRestoreGame* savefile )
{
	savefile->ReadObject( reinterpret_cast<idClass*&>( self ) );
	savefile->ReadString( name );
	savefile->ReadBool( hasBindConstraints );
	savefile->ReadVec3( baseOrigin );
	savefile->ReadMat3( baseAxis );
	savefile->ReadInt( poseTime );
	savefile->ReadInt( restStartTime );
	savefile->ReadBool( isLoaded );
	savefile->ReadBool( isActive );

	animator = NULL;
	modifiedAnim = 0;

	if( self )
	{
		SetAnimator( self->GetAnimator() );
		Load( self, name );
		if( hasBindConstraints )
		{
			AddBindConstraints();
		}
	}

	savefile->ReadStaticObject( physicsObj );

	if( self )
	{
		if( isActive )
		{
			// clear all animations
			animator->ClearAllAnims( gameLocal.time, 0 );
			animator->ClearAllJoints();

			// switch to articulated figure physics
			self->RestorePhysics( &physicsObj );
			physicsObj.EnableClip();
		}
		UpdateAnimation();
	}
}

/*
================
idAF::UpdateAnimation
================
*/
bool idAF::UpdateAnimation()
{
	int i;
	idVec3 origin, renderOrigin, bodyOrigin;
	idMat3 axis, renderAxis, bodyAxis;
	renderEntity_t* renderEntity;

	if( !IsLoaded() )
	{
		return false;
	}

	if( !IsActive() )
	{
		return false;
	}

	renderEntity = self->GetRenderEntity();
	if( !renderEntity )
	{
		return false;
	}

	if( physicsObj.IsAtRest() )
	{
		if( restStartTime == physicsObj.GetRestStartTime() )
		{
			return false;
		}
		restStartTime = physicsObj.GetRestStartTime();
	}

	// get the render position
	origin = physicsObj.GetOrigin( 0 );
	axis = physicsObj.GetAxis( 0 );
	renderAxis = baseAxis.Transpose() * axis;
	renderOrigin = origin - baseOrigin * renderAxis;

	// create an animation frame which reflects the current pose of the articulated figure
	animator->InitAFPose();
	for( i = 0; i < jointMods.Num(); i++ )
	{
		// check for the origin joint
		if( jointMods[i].jointHandle == 0 )
		{
			continue;
		}
		bodyOrigin = physicsObj.GetOrigin( jointMods[i].bodyId );
		bodyAxis = physicsObj.GetAxis( jointMods[i].bodyId );
		axis = jointMods[i].jointBodyAxis.Transpose() * ( bodyAxis * renderAxis.Transpose() );
		origin = ( bodyOrigin - jointMods[i].jointBodyOrigin * axis - renderOrigin ) * renderAxis.Transpose();
		animator->SetAFPoseJointMod( jointMods[i].jointHandle, jointMods[i].jointMod, axis, origin );
	}
	animator->FinishAFPose( modifiedAnim, GetBounds().Expand( POSE_BOUNDS_EXPANSION ), gameLocal.time );
	animator->SetAFPoseBlendWeight( 1.0f );

	return true;
}

/*
================
idAF::GetBounds

  returns bounds for the current pose
================
*/
idBounds idAF::GetBounds() const
{
	int i;
	idAFBody* body;
	idVec3 origin, entityOrigin;
	idMat3 axis, entityAxis;
	idBounds bounds, b;

	bounds.Clear();

	// get model base transform
	origin = physicsObj.GetOrigin( 0 );
	axis = physicsObj.GetAxis( 0 );

	entityAxis = baseAxis.Transpose() * axis;
	entityOrigin = origin - baseOrigin * entityAxis;

	// get bounds relative to base
	for( i = 0; i < jointMods.Num(); i++ )
	{
		body = physicsObj.GetBody( jointMods[i].bodyId );
		origin = ( body->GetWorldOrigin() - entityOrigin ) * entityAxis.Transpose();
		axis = body->GetWorldAxis() * entityAxis.Transpose();
		b.FromTransformedBounds( body->GetClipModel()->GetBounds(), origin, axis );

		bounds += b;
	}

	return bounds;
}

/*
================
idAF::SetupPose

  Transforms the articulated figure to match the current animation pose of the given entity.
================
*/
void idAF::SetupPose( idEntity* ent, int time )
{
	int i;
	idAFBody* body;
	idVec3 origin;
	idMat3 axis;
	idAnimator* animatorPtr;
	renderEntity_t* renderEntity;

	if( !IsLoaded() || !ent )
	{
		return;
	}

	animatorPtr = ent->GetAnimator();
	if( !animatorPtr )
	{
		return;
	}

	renderEntity = ent->GetRenderEntity();
	if( !renderEntity )
	{
		return;
	}

	// if the animation is driven by the physics
	if( self->GetPhysics() == &physicsObj )
	{
		return;
	}

	// if the pose was already updated this frame
	if( poseTime == time )
	{
		return;
	}
	poseTime = time;

	for( i = 0; i < jointMods.Num(); i++ )
	{
		body = physicsObj.GetBody( jointMods[i].bodyId );
		animatorPtr->GetJointTransform( jointMods[i].jointHandle, time, origin, axis );
		body->SetWorldOrigin( renderEntity->origin + ( origin + jointMods[i].jointBodyOrigin * axis ) * renderEntity->axis );
		body->SetWorldAxis( jointMods[i].jointBodyAxis * axis * renderEntity->axis );
	}

	if( isActive )
	{
		physicsObj.UpdateClipModels();
	}
}

/*
================
idAF::ChangePose

   Change the articulated figure to match the current animation pose of the given entity
   and set the velocity relative to the previous pose.
================
*/
void idAF::ChangePose( idEntity* ent, int time )
{
	int i;
	float invDelta;
	idAFBody* body;
	idVec3 origin, lastOrigin;
	idMat3 axis;
	idAnimator* animatorPtr;
	renderEntity_t* renderEntity;

	if( !IsLoaded() || !ent )
	{
		return;
	}

	animatorPtr = ent->GetAnimator();
	if( !animatorPtr )
	{
		return;
	}

	renderEntity = ent->GetRenderEntity();
	if( !renderEntity )
	{
		return;
	}

	// if the animation is driven by the physics
	if( self->GetPhysics() == &physicsObj )
	{
		return;
	}

	// if the pose was already updated this frame
	if( poseTime == time )
	{
		return;
	}
	invDelta = 1.0f / MS2SEC( time - poseTime );
	poseTime = time;

	for( i = 0; i < jointMods.Num(); i++ )
	{
		body = physicsObj.GetBody( jointMods[i].bodyId );
		animatorPtr->GetJointTransform( jointMods[i].jointHandle, time, origin, axis );
		lastOrigin = body->GetWorldOrigin();
		body->SetWorldOrigin( renderEntity->origin + ( origin + jointMods[i].jointBodyOrigin * axis ) * renderEntity->axis );
		body->SetWorldAxis( jointMods[i].jointBodyAxis * axis * renderEntity->axis );
		body->SetLinearVelocity( ( body->GetWorldOrigin() - lastOrigin ) * invDelta );
	}

	physicsObj.UpdateClipModels();
}

/*
================
idAF::EntitiesTouchingAF
================
*/
int idAF::EntitiesTouchingAF( afTouch_t touchList[ MAX_GENTITIES ] ) const
{
	int i, j, numClipModels;
	idAFBody* body;
	idClipModel* cm;
	idClipModel* clipModels[ MAX_GENTITIES ];
	int numTouching;

	if( !IsLoaded() )
	{
		return 0;
	}

	numTouching = 0;
	numClipModels = gameLocal.clip.ClipModelsTouchingBounds( physicsObj.GetAbsBounds(), -1, clipModels, MAX_GENTITIES );

	for( i = 0; i < jointMods.Num(); i++ )
	{
		body = physicsObj.GetBody( jointMods[i].bodyId );

		for( j = 0; j < numClipModels; j++ )
		{
			cm = clipModels[j];

			if( !cm || cm->GetEntity() == self )
			{
				continue;
			}

			if( !cm->IsTraceModel() )
			{
				continue;
			}

			if( !body->GetClipModel()->GetAbsBounds().IntersectsBounds( cm->GetAbsBounds() ) )
			{
				continue;
			}

			if( gameLocal.clip.ContentsModel( body->GetWorldOrigin(), body->GetClipModel(), body->GetWorldAxis(), -1, cm->Handle(), cm->GetOrigin(), cm->GetAxis() ) )
			{
				touchList[ numTouching ].touchedByBody = body;
				touchList[ numTouching ].touchedClipModel = cm;
				touchList[ numTouching ].touchedEnt  = cm->GetEntity();
				numTouching++;
				clipModels[j] = NULL;
			}
		}
	}

	return numTouching;
}

/*
================
idAF::BodyForClipModelId
================
*/
int idAF::BodyForClipModelId( int id ) const
{
	if( id >= 0 )
	{
		return id;
	}
	else
	{
		id = CLIPMODEL_ID_TO_JOINT_HANDLE( id );
		if( id < jointBody.Num() )
		{
			return jointBody[id];
		}
		else
		{
			return 0;
		}
	}
}

/*
================
idAF::GetPhysicsToVisualTransform
================
*/
void idAF::GetPhysicsToVisualTransform( idVec3& origin, idMat3& axis ) const
{
	origin = - baseOrigin;
	axis = baseAxis.Transpose();
}

/*
================
idAF::GetImpactInfo
================
*/
void idAF::GetImpactInfo( idEntity* ent, int id, const idVec3& point, impactInfo_t* info )
{
	SetupPose( self, gameLocal.time );
	physicsObj.GetImpactInfo( BodyForClipModelId( id ), point, info );
}

/*
================
idAF::ApplyImpulse
================
*/
void idAF::ApplyImpulse( idEntity* ent, int id, const idVec3& point, const idVec3& impulse )
{
	SetupPose( self, gameLocal.time );
	physicsObj.ApplyImpulse( BodyForClipModelId( id ), point, impulse );
}

/*
================
idAF::AddForce
================
*/
void idAF::AddForce( idEntity* ent, int id, const idVec3& point, const idVec3& force )
{
	SetupPose( self, gameLocal.time );
	physicsObj.AddForce( BodyForClipModelId( id ), point, force );
}

/*
================
idAF::AddBody

  Adds a body.
================
*/
void idAF::AddBody( idAFBody* body, const idJointMat* joints, const char* jointName, const AFJointModType_t mod )
{
	int index;
	jointHandle_t handle;
	idVec3 origin;
	idMat3 axis;

	handle = animator->GetJointHandle( jointName );
	if( handle == INVALID_JOINT )
	{
		gameLocal.Error( "idAF for entity '%s' at (%s) modifies unknown joint '%s'", self->name.c_str(), self->GetPhysics()->GetOrigin().ToString( 0 ), jointName );
	}

	assert( handle < animator->NumJoints() );
	origin = joints[ handle ].ToVec3();
	axis = joints[ handle ].ToMat3();

	index = jointMods.Num();
	jointMods.SetNum( index + 1 );
	jointMods[index].bodyId = physicsObj.GetBodyId( body );
	jointMods[index].jointHandle = handle;
	jointMods[index].jointMod = mod;
	jointMods[index].jointBodyOrigin = ( body->GetWorldOrigin() - origin ) * axis.Transpose();
	jointMods[index].jointBodyAxis = body->GetWorldAxis() * axis.Transpose();
}

/*
================
idAF::SetBase

  Sets the base body.
================
*/
void idAF::SetBase( idAFBody* body, const idJointMat* joints )
{
	physicsObj.ForceBodyId( body, 0 );
	baseOrigin = body->GetWorldOrigin();
	baseAxis = body->GetWorldAxis();
	AddBody( body, joints, animator->GetJointName( animator->GetFirstChild( "origin" ) ), AF_JOINTMOD_AXIS );
}

/*
================
idAF::LoadBody
================
*/
bool idAF::LoadBody( const idDeclAF_Body* fb, const idJointMat* joints )
{
	int id, i;
	float length, mass;
	idTraceModel trm;
	idClipModel* clip;
	idAFBody* body;
	idMat3 axis, inertiaTensor;
	idVec3 centerOfMass, origin;
	idBounds bounds;
	idList<jointHandle_t, TAG_AF> jointList;

	origin = fb->origin.ToVec3();
	axis = fb->angles.ToMat3();
	bounds[0] = fb->v1.ToVec3();
	bounds[1] = fb->v2.ToVec3();

	switch( fb->modelType )
	{
		case TRM_BOX:
		{
			trm.SetupBox( bounds );
			break;
		}
		case TRM_OCTAHEDRON:
		{
			trm.SetupOctahedron( bounds );
			break;
		}
		case TRM_DODECAHEDRON:
		{
			trm.SetupDodecahedron( bounds );
			break;
		}
		case TRM_CYLINDER:
		{
			trm.SetupCylinder( bounds, fb->numSides );
			break;
		}
		case TRM_CONE:
		{
			// place the apex at the origin
			bounds[0].z -= bounds[1].z;
			bounds[1].z = 0.0f;
			trm.SetupCone( bounds, fb->numSides );
			break;
		}
		case TRM_BONE:
		{
			// direction of bone
			axis[2] = fb->v2.ToVec3() - fb->v1.ToVec3();
			length = axis[2].Normalize();
			// axis of bone trace model
			axis[2].NormalVectors( axis[0], axis[1] );
			axis[1] = -axis[1];
			// create bone trace model
			trm.SetupBone( length, fb->width );
			break;
		}
		default:
			assert( 0 );
			break;
	}
	trm.GetMassProperties( 1.0f, mass, centerOfMass, inertiaTensor );
	trm.Translate( -centerOfMass );
	origin += centerOfMass * axis;

	body = physicsObj.GetBody( fb->name );
	if( body )
	{
		clip = body->GetClipModel();
		if( !clip->IsEqual( trm ) )
		{
			clip = new( TAG_PHYSICS_CLIP_AF ) idClipModel( trm );
			clip->SetContents( fb->contents );
			clip->Link( gameLocal.clip, self, 0, origin, axis );
			body->SetClipModel( clip );
		}
		clip->SetContents( fb->contents );
		body->SetDensity( fb->density, fb->inertiaScale );
		body->SetWorldOrigin( origin );
		body->SetWorldAxis( axis );
		id = physicsObj.GetBodyId( body );
	}
	else
	{
		clip = new( TAG_PHYSICS_CLIP_AF ) idClipModel( trm );
		clip->SetContents( fb->contents );
		clip->Link( gameLocal.clip, self, 0, origin, axis );
		body = new( TAG_PHYSICS_AF ) idAFBody( fb->name, clip, fb->density );
		if( fb->inertiaScale != mat3_identity )
		{
			body->SetDensity( fb->density, fb->inertiaScale );
		}
		id = physicsObj.AddBody( body );
	}
	if( fb->linearFriction != -1.0f )
	{
		body->SetFriction( fb->linearFriction, fb->angularFriction, fb->contactFriction );
	}
	body->SetClipMask( fb->clipMask );
	body->SetSelfCollision( fb->selfCollision );

	if( fb->jointName == "origin" )
	{
		SetBase( body, joints );
	}
	else
	{
		AFJointModType_t mod;
		if( fb->jointMod == DECLAF_JOINTMOD_AXIS )
		{
			mod = AF_JOINTMOD_AXIS;
		}
		else if( fb->jointMod == DECLAF_JOINTMOD_ORIGIN )
		{
			mod = AF_JOINTMOD_ORIGIN;
		}
		else if( fb->jointMod == DECLAF_JOINTMOD_BOTH )
		{
			mod = AF_JOINTMOD_BOTH;
		}
		else
		{
			mod = AF_JOINTMOD_AXIS;
		}
		AddBody( body, joints, fb->jointName, mod );
	}

	if( fb->frictionDirection.ToVec3() != vec3_origin )
	{
		body->SetFrictionDirection( fb->frictionDirection.ToVec3() );
	}
	if( fb->contactMotorDirection.ToVec3() != vec3_origin )
	{
		body->SetContactMotorDirection( fb->contactMotorDirection.ToVec3() );
	}

	// update table to find the nearest articulated figure body for a joint of the skeletal model
	animator->GetJointList( fb->containedJoints, jointList );
	for( i = 0; i < jointList.Num(); i++ )
	{
		if( jointBody[ jointList[ i ] ] != -1 )
		{
			gameLocal.Warning( "%s: joint '%s' is already contained by body '%s'",
							   name.c_str(), animator->GetJointName( ( jointHandle_t )jointList[i] ),
							   physicsObj.GetBody( jointBody[ jointList[ i ] ] )->GetName().c_str() );
		}
		jointBody[ jointList[ i ] ] = id;
	}

	return true;
}

/*
================
idAF::LoadConstraint
================
*/
bool idAF::LoadConstraint( const idDeclAF_Constraint* fc )
{
	idAFBody* body1, *body2;
	idAngles angles;
	idMat3 axis;

	body1 = physicsObj.GetBody( fc->body1 );
	body2 = physicsObj.GetBody( fc->body2 );

	switch( fc->type )
	{
		case DECLAF_CONSTRAINT_FIXED:
		{
			idAFConstraint_Fixed* c;
			c = static_cast<idAFConstraint_Fixed*>( physicsObj.GetConstraint( fc->name ) );
			if( c )
			{
				c->SetBody1( body1 );
				c->SetBody2( body2 );
			}
			else
			{
				c = new( TAG_PHYSICS_AF ) idAFConstraint_Fixed( fc->name, body1, body2 );
				physicsObj.AddConstraint( c );
			}
			break;
		}
		case DECLAF_CONSTRAINT_BALLANDSOCKETJOINT:
		{
			idAFConstraint_BallAndSocketJoint* c;
			c = static_cast<idAFConstraint_BallAndSocketJoint*>( physicsObj.GetConstraint( fc->name ) );
			if( c )
			{
				c->SetBody1( body1 );
				c->SetBody2( body2 );
			}
			else
			{
				c = new( TAG_PHYSICS_AF ) idAFConstraint_BallAndSocketJoint( fc->name, body1, body2 );
				physicsObj.AddConstraint( c );
			}
			c->SetAnchor( fc->anchor.ToVec3() );
			c->SetFriction( fc->friction );
			switch( fc->limit )
			{
				case idDeclAF_Constraint::LIMIT_CONE:
				{
					c->SetConeLimit( fc->limitAxis.ToVec3(), fc->limitAngles[0], fc->shaft[0].ToVec3() );
					break;
				}
				case idDeclAF_Constraint::LIMIT_PYRAMID:
				{
					angles = fc->limitAxis.ToVec3().ToAngles();
					angles.roll = fc->limitAngles[2];
					axis = angles.ToMat3();
					c->SetPyramidLimit( axis[0], axis[1], fc->limitAngles[0], fc->limitAngles[1], fc->shaft[0].ToVec3() );
					break;
				}
				default:
				{
					c->SetNoLimit();
					break;
				}
			}
			break;
		}
		case DECLAF_CONSTRAINT_UNIVERSALJOINT:
		{
			idAFConstraint_UniversalJoint* c;
			c = static_cast<idAFConstraint_UniversalJoint*>( physicsObj.GetConstraint( fc->name ) );
			if( c )
			{
				c->SetBody1( body1 );
				c->SetBody2( body2 );
			}
			else
			{
				c = new( TAG_PHYSICS_AF ) idAFConstraint_UniversalJoint( fc->name, body1, body2 );
				physicsObj.AddConstraint( c );
			}
			c->SetAnchor( fc->anchor.ToVec3() );
			c->SetShafts( fc->shaft[0].ToVec3(), fc->shaft[1].ToVec3() );
			c->SetFriction( fc->friction );
			switch( fc->limit )
			{
				case idDeclAF_Constraint::LIMIT_CONE:
				{
					c->SetConeLimit( fc->limitAxis.ToVec3(), fc->limitAngles[0] );
					break;
				}
				case idDeclAF_Constraint::LIMIT_PYRAMID:
				{
					angles = fc->limitAxis.ToVec3().ToAngles();
					angles.roll = fc->limitAngles[2];
					axis = angles.ToMat3();
					c->SetPyramidLimit( axis[0], axis[1], fc->limitAngles[0], fc->limitAngles[1] );
					break;
				}
				default:
				{
					c->SetNoLimit();
					break;
				}
			}
			break;
		}
		case DECLAF_CONSTRAINT_HINGE:
		{
			idAFConstraint_Hinge* c;
			c = static_cast<idAFConstraint_Hinge*>( physicsObj.GetConstraint( fc->name ) );
			if( c )
			{
				c->SetBody1( body1 );
				c->SetBody2( body2 );
			}
			else
			{
				c = new( TAG_PHYSICS_AF ) idAFConstraint_Hinge( fc->name, body1, body2 );
				physicsObj.AddConstraint( c );
			}
			c->SetAnchor( fc->anchor.ToVec3() );
			c->SetAxis( fc->axis.ToVec3() );
			c->SetFriction( fc->friction );
			switch( fc->limit )
			{
				case idDeclAF_Constraint::LIMIT_CONE:
				{
					idVec3 left, up, axis, shaft;
					fc->axis.ToVec3().OrthogonalBasis( left, up );
					axis = left * idRotation( vec3_origin, fc->axis.ToVec3(), fc->limitAngles[0] );
					shaft = left * idRotation( vec3_origin, fc->axis.ToVec3(), fc->limitAngles[2] );
					c->SetLimit( axis, fc->limitAngles[1], shaft );
					break;
				}
				default:
				{
					c->SetNoLimit();
					break;
				}
			}
			break;
		}
		case DECLAF_CONSTRAINT_SLIDER:
		{
			idAFConstraint_Slider* c;
			c = static_cast<idAFConstraint_Slider*>( physicsObj.GetConstraint( fc->name ) );
			if( c )
			{
				c->SetBody1( body1 );
				c->SetBody2( body2 );
			}
			else
			{
				c = new( TAG_PHYSICS_AF ) idAFConstraint_Slider( fc->name, body1, body2 );
				physicsObj.AddConstraint( c );
			}
			c->SetAxis( fc->axis.ToVec3() );
			break;
		}
		case DECLAF_CONSTRAINT_SPRING:
		{
			idAFConstraint_Spring* c;
			c = static_cast<idAFConstraint_Spring*>( physicsObj.GetConstraint( fc->name ) );
			if( c )
			{
				c->SetBody1( body1 );
				c->SetBody2( body2 );
			}
			else
			{
				c = new( TAG_PHYSICS_AF ) idAFConstraint_Spring( fc->name, body1, body2 );
				physicsObj.AddConstraint( c );
			}
			c->SetAnchor( fc->anchor.ToVec3(), fc->anchor2.ToVec3() );
			c->SetSpring( fc->stretch, fc->compress, fc->damping, fc->restLength );
			c->SetLimit( fc->minLength, fc->maxLength );
			break;
		}
	}
	return true;
}

/*
================
GetJointTransform
================
*/
static bool GetJointTransform( void* model, const idJointMat* frame, const char* jointName, idVec3& origin, idMat3& axis )
{
	jointHandle_t	joint;

	joint = reinterpret_cast<idAnimator*>( model )->GetJointHandle( jointName );
	if( ( joint >= 0 ) && ( joint < reinterpret_cast<idAnimator*>( model )->NumJoints() ) )
	{
		origin = frame[ joint ].ToVec3();
		axis = frame[ joint ].ToMat3();
		return true;
	}
	else
	{
		return false;
	}
}

/*
================
idAF::Load
================
*/
bool idAF::Load( idEntity* ent, const char* fileName )
{
	int i, j;
	const idDeclAF* file;
	const idDeclModelDef* modelDef;
	idRenderModel* model;
	int numJoints;
	idJointMat* joints;

	assert( ent );

	self = ent;
	physicsObj.SetSelf( self );

	if( animator == NULL )
	{
		gameLocal.Warning( "Couldn't load af '%s' for entity '%s' at (%s): NULL animator\n", name.c_str(), ent->name.c_str(), ent->GetPhysics()->GetOrigin().ToString( 0 ) );
		return false;
	}

	name = fileName;
	name.StripFileExtension();

	file = static_cast<const idDeclAF*>( declManager->FindType( DECL_AF, name ) );
	if( !file )
	{
		gameLocal.Warning( "Couldn't load af '%s' for entity '%s' at (%s)\n", name.c_str(), ent->name.c_str(), ent->GetPhysics()->GetOrigin().ToString( 0 ) );
		return false;
	}

	if( file->bodies.Num() == 0 || file->bodies[0]->jointName != "origin" )
	{
		gameLocal.Warning( "idAF::Load: articulated figure '%s' for entity '%s' at (%s) has no body which modifies the origin joint.",
						   name.c_str(), ent->name.c_str(), ent->GetPhysics()->GetOrigin().ToString( 0 ) );
		return false;
	}

	modelDef = animator->ModelDef();
	if( modelDef == NULL || modelDef->GetState() == DS_DEFAULTED )
	{
		gameLocal.Warning( "idAF::Load: articulated figure '%s' for entity '%s' at (%s) has no or defaulted modelDef '%s'",
						   name.c_str(), ent->name.c_str(), ent->GetPhysics()->GetOrigin().ToString( 0 ), modelDef ? modelDef->GetName() : "" );
		return false;
	}

	model = animator->ModelHandle();
	if( model == NULL || model->IsDefaultModel() )
	{
		gameLocal.Warning( "idAF::Load: articulated figure '%s' for entity '%s' at (%s) has no or defaulted model '%s'",
						   name.c_str(), ent->name.c_str(), ent->GetPhysics()->GetOrigin().ToString( 0 ), model ? model->Name() : "" );
		return false;
	}

	// get the modified animation
	modifiedAnim = animator->GetAnim( ARTICULATED_FIGURE_ANIM );
	if( !modifiedAnim )
	{
		gameLocal.Warning( "idAF::Load: articulated figure '%s' for entity '%s' at (%s) has no modified animation '%s'",
						   name.c_str(), ent->name.c_str(), ent->GetPhysics()->GetOrigin().ToString( 0 ), ARTICULATED_FIGURE_ANIM );
		return false;
	}

	// create the animation frame used to setup the articulated figure
	numJoints = animator->NumJoints();
	joints = ( idJointMat* )_alloca16( numJoints * sizeof( joints[0] ) );
	gameEdit->ANIM_CreateAnimFrame( model, animator->GetAnim( modifiedAnim )->MD5Anim( 0 ), numJoints, joints, 1, animator->ModelDef()->GetVisualOffset(), animator->RemoveOrigin() );

	// set all vector positions from model joints
	file->Finish( GetJointTransform, joints, animator );

	// initialize articulated figure physics
	physicsObj.SetGravity( gameLocal.GetGravity() );
	physicsObj.SetClipMask( file->clipMask );
	physicsObj.SetDefaultFriction( file->defaultLinearFriction, file->defaultAngularFriction, file->defaultContactFriction );
	physicsObj.SetSuspendSpeed( file->suspendVelocity, file->suspendAcceleration );
	physicsObj.SetSuspendTolerance( file->noMoveTime, file->noMoveTranslation, file->noMoveRotation );
	physicsObj.SetSuspendTime( file->minMoveTime, file->maxMoveTime );
	physicsObj.SetSelfCollision( file->selfCollision );

	// clear the list with transforms from joints to bodies
	jointMods.SetNum( 0 );

	// clear the joint to body conversion list
	jointBody.AssureSize( animator->NumJoints() );
	for( i = 0; i < jointBody.Num(); i++ )
	{
		jointBody[i] = -1;
	}

	// delete any bodies in the physicsObj that are no longer in the idDeclAF
	for( i = 0; i < physicsObj.GetNumBodies(); i++ )
	{
		idAFBody* body = physicsObj.GetBody( i );
		for( j = 0; j < file->bodies.Num(); j++ )
		{
			if( file->bodies[j]->name.Icmp( body->GetName() ) == 0 )
			{
				break;
			}
		}
		if( j >= file->bodies.Num() )
		{
			physicsObj.DeleteBody( i );
			i--;
		}
	}

	// delete any constraints in the physicsObj that are no longer in the idDeclAF
	for( i = 0; i < physicsObj.GetNumConstraints(); i++ )
	{
		idAFConstraint* constraint = physicsObj.GetConstraint( i );
		for( j = 0; j < file->constraints.Num(); j++ )
		{
			// idADConstraint enum is a superset of declADConstraint, so the cast is valid
			if( file->constraints[j]->name.Icmp( constraint->GetName() ) == 0 &&
					( constraintType_t )( file->constraints[j]->type ) == constraint->GetType() )
			{
				break;
			}
		}
		if( j >= file->constraints.Num() )
		{
			physicsObj.DeleteConstraint( i );
			i--;
		}
	}

	// load bodies from the file
	for( i = 0; i < file->bodies.Num(); i++ )
	{
		LoadBody( file->bodies[i], joints );
	}

	// load constraints from the file
	for( i = 0; i < file->constraints.Num(); i++ )
	{
		LoadConstraint( file->constraints[i] );
	}

	physicsObj.UpdateClipModels();

	// check if each joint is contained by a body
	for( i = 0; i < animator->NumJoints(); i++ )
	{
		if( jointBody[i] == -1 )
		{
			gameLocal.Warning( "idAF::Load: articulated figure '%s' for entity '%s' at (%s) joint '%s' is not contained by a body",
							   name.c_str(), self->name.c_str(), self->GetPhysics()->GetOrigin().ToString( 0 ), animator->GetJointName( ( jointHandle_t )i ) );
		}
	}

	physicsObj.SetMass( file->totalMass );
	physicsObj.SetChanged();

	// disable the articulated figure for collision detection until activated
	physicsObj.DisableClip();

	isLoaded = true;

	return true;
}

/*
================
idAF::Start
================
*/
void idAF::Start()
{
	if( !IsLoaded() )
	{
		return;
	}
	// clear all animations
	animator->ClearAllAnims( gameLocal.time, 0 );
	animator->ClearAllJoints();
	// switch to articulated figure physics
	self->SetPhysics( &physicsObj );
	// start the articulated figure physics simulation
	physicsObj.EnableClip();
	physicsObj.Activate();
	isActive = true;
}

/*
================
idAF::TestSolid
================
*/
bool idAF::TestSolid() const
{
	int i;
	idAFBody* body;
	trace_t trace;
	idStr str;
	bool solid;

	if( !IsLoaded() )
	{
		return false;
	}

	if( !af_testSolid.GetBool() )
	{
		return false;
	}

	solid = false;

	for( i = 0; i < physicsObj.GetNumBodies(); i++ )
	{
		body = physicsObj.GetBody( i );
		if( gameLocal.clip.Translation( trace, body->GetWorldOrigin(), body->GetWorldOrigin(), body->GetClipModel(), body->GetWorldAxis(), body->GetClipMask(), self ) )
		{
			float depth = idMath::Fabs( trace.c.point * trace.c.normal - trace.c.dist );

			body->SetWorldOrigin( body->GetWorldOrigin() + trace.c.normal * ( depth + 8.0f ) );

			gameLocal.DWarning( "%s: body '%s' stuck in %d (normal = %.2f %.2f %.2f, depth = %.2f)", self->name.c_str(),
								body->GetName().c_str(), trace.c.contents, trace.c.normal.x, trace.c.normal.y, trace.c.normal.z, depth );
			solid = true;

		}
	}
	return solid;
}

/*
================
idAF::StartFromCurrentPose
================
*/
void idAF::StartFromCurrentPose( int inheritVelocityTime )
{

	if( !IsLoaded() )
	{
		return;
	}

	// if the ragdoll should inherit velocity from the animation
	if( inheritVelocityTime > 0 )
	{

		// make sure the ragdoll is at rest
		physicsObj.PutToRest();

		// set the pose for some time back
		SetupPose( self, gameLocal.time - inheritVelocityTime );

		// change the pose for the current time and set velocities
		ChangePose( self, gameLocal.time );
	}
	else
	{
		// transform the articulated figure to reflect the current animation pose
		SetupPose( self, gameLocal.time );
	}

	physicsObj.UpdateClipModels();

	TestSolid();

	Start();

	UpdateAnimation();

	// update the render entity origin and axis
	self->UpdateModel();

	// make sure the renderer gets the updated origin and axis
	self->Present();
}

/*
================
idAF::Stop
================
*/
void idAF::Stop()
{
	// disable the articulated figure for collision detection
	physicsObj.UnlinkClip();
	isActive = false;
}

/*
================
idAF::Rest
================
*/
void idAF::Rest()
{
	physicsObj.PutToRest();
}

/*
================
idAF::SetConstraintPosition

  Only moves constraints that bind the entity to another entity.
================
*/
void idAF::SetConstraintPosition( const char* name, const idVec3& pos )
{
	idAFConstraint* constraint;

	constraint = GetPhysics()->GetConstraint( name );

	if( !constraint )
	{
		gameLocal.Warning( "can't find a constraint with the name '%s'", name );
		return;
	}

	if( constraint->GetBody2() != NULL )
	{
		gameLocal.Warning( "constraint '%s' does not bind to another entity", name );
		return;
	}

	switch( constraint->GetType() )
	{
		case CONSTRAINT_BALLANDSOCKETJOINT:
		{
			idAFConstraint_BallAndSocketJoint* bs = static_cast<idAFConstraint_BallAndSocketJoint*>( constraint );
			bs->Translate( pos - bs->GetAnchor() );
			break;
		}
		case CONSTRAINT_UNIVERSALJOINT:
		{
			idAFConstraint_UniversalJoint* uj = static_cast<idAFConstraint_UniversalJoint*>( constraint );
			uj->Translate( pos - uj->GetAnchor() );
			break;
		}
		case CONSTRAINT_HINGE:
		{
			idAFConstraint_Hinge* hinge = static_cast<idAFConstraint_Hinge*>( constraint );
			hinge->Translate( pos - hinge->GetAnchor() );
			break;
		}
		default:
		{
			gameLocal.Warning( "cannot set the constraint position for '%s'", name );
			break;
		}
	}
}

/*
================
idAF::SaveState
================
*/
void idAF::SaveState( idDict& args ) const
{
	int i;
	idAFBody* body;
	idStr key, value;

	for( i = 0; i < jointMods.Num(); i++ )
	{
		body = physicsObj.GetBody( jointMods[i].bodyId );

		key = "body " + body->GetName();
		value = body->GetWorldOrigin().ToString( 8 );
		value += " ";
		value += body->GetWorldAxis().ToAngles().ToString( 8 );
		args.Set( key, value );
	}
}

/*
================
idAF::LoadState
================
*/
void idAF::LoadState( const idDict& args )
{
	const idKeyValue* kv;
	idStr name;
	idAFBody* body;
	idVec3 origin;
	idAngles angles;

	kv = args.MatchPrefix( "body ", NULL );
	while( kv )
	{

		name = kv->GetKey();
		name.Strip( "body " );
		body = physicsObj.GetBody( name );
		if( body )
		{
			sscanf( kv->GetValue(), "%f %f %f %f %f %f", &origin.x, &origin.y, &origin.z, &angles.pitch, &angles.yaw, &angles.roll );
			body->SetWorldOrigin( origin );
			body->SetWorldAxis( angles.ToMat3() );
		}
		else
		{
			gameLocal.Warning( "Unknown body part %s in articulated figure %s", name.c_str(), this->name.c_str() );
		}

		kv = args.MatchPrefix( "body ", kv );
	}

	physicsObj.UpdateClipModels();
}

/*
================
idAF::AddBindConstraints
================
*/
void idAF::AddBindConstraints()
{
	const idKeyValue* kv;
	idStr name;
	idAFBody* body;
	idLexer lexer;
	idToken type, bodyName, jointName;
	idVec3 origin, renderOrigin;
	idMat3 axis, renderAxis;

	if( !IsLoaded() )
	{
		return;
	}

	const idDict& args = self->spawnArgs;

	// get the render position
	origin = physicsObj.GetOrigin( 0 );
	axis = physicsObj.GetAxis( 0 );
	renderAxis = baseAxis.Transpose() * axis;
	renderOrigin = origin - baseOrigin * renderAxis;

	// parse all the bind constraints
	for( kv = args.MatchPrefix( "bindConstraint ", NULL ); kv; kv = args.MatchPrefix( "bindConstraint ", kv ) )
	{
		name = kv->GetKey();
		name.Strip( "bindConstraint " );

		lexer.LoadMemory( kv->GetValue(), kv->GetValue().Length(), kv->GetKey() );
		lexer.ReadToken( &type );

		lexer.ReadToken( &bodyName );
		body = physicsObj.GetBody( bodyName );
		if( !body )
		{
			gameLocal.Warning( "idAF::AddBindConstraints: body '%s' not found on entity '%s'", bodyName.c_str(), self->name.c_str() );
			lexer.FreeSource();
			continue;
		}

		if( type.Icmp( "fixed" ) == 0 )
		{
			idAFConstraint_Fixed* c;

			c = new( TAG_PHYSICS_AF ) idAFConstraint_Fixed( name, body, NULL );
			physicsObj.AddConstraint( c );
		}
		else if( type.Icmp( "ballAndSocket" ) == 0 )
		{
			idAFConstraint_BallAndSocketJoint* c;

			c = new( TAG_PHYSICS_AF ) idAFConstraint_BallAndSocketJoint( name, body, NULL );
			physicsObj.AddConstraint( c );
			lexer.ReadToken( &jointName );

			jointHandle_t joint = animator->GetJointHandle( jointName );
			if( joint == INVALID_JOINT )
			{
				gameLocal.Warning( "idAF::AddBindConstraints: joint '%s' not found", jointName.c_str() );
			}

			animator->GetJointTransform( joint, gameLocal.time, origin, axis );
			c->SetAnchor( renderOrigin + origin * renderAxis );
		}
		else if( type.Icmp( "universal" ) == 0 )
		{
			idAFConstraint_UniversalJoint* c;

			c = new( TAG_PHYSICS_AF ) idAFConstraint_UniversalJoint( name, body, NULL );
			physicsObj.AddConstraint( c );
			lexer.ReadToken( &jointName );

			jointHandle_t joint = animator->GetJointHandle( jointName );
			if( joint == INVALID_JOINT )
			{
				gameLocal.Warning( "idAF::AddBindConstraints: joint '%s' not found", jointName.c_str() );
			}
			animator->GetJointTransform( joint, gameLocal.time, origin, axis );
			c->SetAnchor( renderOrigin + origin * renderAxis );
			c->SetShafts( idVec3( 0, 0, 1 ), idVec3( 0, 0, -1 ) );
		}
		else
		{
			gameLocal.Warning( "idAF::AddBindConstraints: unknown constraint type '%s' on entity '%s'", type.c_str(), self->name.c_str() );
		}

		lexer.FreeSource();
	}

	hasBindConstraints = true;
}

/*
================
idAF::RemoveBindConstraints
================
*/
void idAF::RemoveBindConstraints()
{
	const idKeyValue* kv;

	if( !IsLoaded() )
	{
		return;
	}

	const idDict& args = self->spawnArgs;
	idStr name;

	kv = args.MatchPrefix( "bindConstraint ", NULL );
	while( kv )
	{
		name = kv->GetKey();
		name.Strip( "bindConstraint " );

		if( physicsObj.GetConstraint( name ) )
		{
			physicsObj.DeleteConstraint( name );
		}

		kv = args.MatchPrefix( "bindConstraint ", kv );
	}

	hasBindConstraints = false;
}
