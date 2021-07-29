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

#include "../Game_local.h"

CLASS_DECLARATION( idPhysics, idPhysics_Base )
END_CLASS

/*
================
idPhysics_Base::idPhysics_Base
================
*/
idPhysics_Base::idPhysics_Base()
{
	self = NULL;
	clipMask = 0;
	SetGravity( gameLocal.GetGravity() );
	ClearContacts();
}

/*
================
idPhysics_Base::~idPhysics_Base
================
*/
idPhysics_Base::~idPhysics_Base()
{
	if( self && self->GetPhysics() == this )
	{
		self->SetPhysics( NULL );
	}
	idForce::DeletePhysics( this );
	ClearContacts();
}

/*
================
idPhysics_Base::Save
================
*/
void idPhysics_Base::Save( idSaveGame* savefile ) const
{
	int i;

	savefile->WriteObject( self );
	savefile->WriteInt( clipMask );
	savefile->WriteVec3( gravityVector );
	savefile->WriteVec3( gravityNormal );

	savefile->WriteInt( contacts.Num() );
	for( i = 0; i < contacts.Num(); i++ )
	{
		savefile->WriteContactInfo( contacts[i] );
	}

	savefile->WriteInt( contactEntities.Num() );
	for( i = 0; i < contactEntities.Num(); i++ )
	{
		contactEntities[i].Save( savefile );
	}
}

/*
================
idPhysics_Base::Restore
================
*/
void idPhysics_Base::Restore( idRestoreGame* savefile )
{
	int i, num;

	savefile->ReadObject( reinterpret_cast<idClass*&>( self ) );
	savefile->ReadInt( clipMask );
	savefile->ReadVec3( gravityVector );
	savefile->ReadVec3( gravityNormal );

	savefile->ReadInt( num );
	contacts.SetNum( num );
	for( i = 0; i < contacts.Num(); i++ )
	{
		savefile->ReadContactInfo( contacts[i] );
	}

	savefile->ReadInt( num );
	contactEntities.SetNum( num );
	for( i = 0; i < contactEntities.Num(); i++ )
	{
		contactEntities[i].Restore( savefile );
	}
}

/*
================
idPhysics_Base::SetSelf
================
*/
void idPhysics_Base::SetSelf( idEntity* e )
{
	assert( e );
	self = e;
}

/*
================
idPhysics_Base::SetClipModel
================
*/
void idPhysics_Base::SetClipModel( idClipModel* model, float density, int id, bool freeOld )
{
}

/*
================
idPhysics_Base::GetClipModel
================
*/
idClipModel* idPhysics_Base::GetClipModel( int id ) const
{
	return NULL;
}

/*
================
idPhysics_Base::GetNumClipModels
================
*/
int idPhysics_Base::GetNumClipModels() const
{
	return 0;
}

/*
================
idPhysics_Base::SetMass
================
*/
void idPhysics_Base::SetMass( float mass, int id )
{
}

/*
================
idPhysics_Base::GetMass
================
*/
float idPhysics_Base::GetMass( int id ) const
{
	return 0.0f;
}

/*
================
idPhysics_Base::SetContents
================
*/
void idPhysics_Base::SetContents( int contents, int id )
{
}

/*
================
idPhysics_Base::SetClipMask
================
*/
int idPhysics_Base::GetContents( int id ) const
{
	return 0;
}

/*
================
idPhysics_Base::SetClipMask
================
*/
void idPhysics_Base::SetClipMask( int mask, int id )
{
	clipMask = mask;
}

/*
================
idPhysics_Base::GetClipMask
================
*/
int idPhysics_Base::GetClipMask( int id ) const
{
	return clipMask;
}

/*
================
idPhysics_Base::GetBounds
================
*/
const idBounds& idPhysics_Base::GetBounds( int id ) const
{
	return bounds_zero;
}

/*
================
idPhysics_Base::GetAbsBounds
================
*/
const idBounds& idPhysics_Base::GetAbsBounds( int id ) const
{
	return bounds_zero;
}

/*
================
idPhysics_Base::Evaluate
================
*/
bool idPhysics_Base::Evaluate( int timeStepMSec, int endTimeMSec )
{
	return false;
}

/*
================
idPhysics_Base::Interpolate
================
*/
bool idPhysics_Base::Interpolate( const float fraction )
{
	return false;
}

/*
================
idPhysics_Base::ResetInterpolationState
================
*/
void idPhysics_Base::ResetInterpolationState( const idVec3& origin, const idMat3& axis )
{

}

/*
================
idPhysics_Base::UpdateTime
================
*/
void idPhysics_Base::UpdateTime( int endTimeMSec )
{
}

/*
================
idPhysics_Base::GetTime
================
*/
int idPhysics_Base::GetTime() const
{
	return 0;
}

/*
================
idPhysics_Base::GetImpactInfo
================
*/
void idPhysics_Base::GetImpactInfo( const int id, const idVec3& point, impactInfo_t* info ) const
{
	memset( info, 0, sizeof( *info ) );
}

/*
================
idPhysics_Base::ApplyImpulse
================
*/
void idPhysics_Base::ApplyImpulse( const int id, const idVec3& point, const idVec3& impulse )
{
}

/*
================
idPhysics_Base::AddForce
================
*/
void idPhysics_Base::AddForce( const int id, const idVec3& point, const idVec3& force )
{
}

/*
================
idPhysics_Base::Activate
================
*/
void idPhysics_Base::Activate()
{
}

/*
================
idPhysics_Base::PutToRest
================
*/
void idPhysics_Base::PutToRest()
{
}

/*
================
idPhysics_Base::IsAtRest
================
*/
bool idPhysics_Base::IsAtRest() const
{
	return true;
}

/*
================
idPhysics_Base::GetRestStartTime
================
*/
int idPhysics_Base::GetRestStartTime() const
{
	return 0;
}

/*
================
idPhysics_Base::IsPushable
================
*/
bool idPhysics_Base::IsPushable() const
{
	return true;
}

/*
================
idPhysics_Base::SaveState
================
*/
void idPhysics_Base::SaveState()
{
}

/*
================
idPhysics_Base::RestoreState
================
*/
void idPhysics_Base::RestoreState()
{
}

/*
================
idPhysics_Base::SetOrigin
================
*/
void idPhysics_Base::SetOrigin( const idVec3& newOrigin, int id )
{
}

/*
================
idPhysics_Base::SetAxis
================
*/
void idPhysics_Base::SetAxis( const idMat3& newAxis, int id )
{
}

/*
================
idPhysics_Base::Translate
================
*/
void idPhysics_Base::Translate( const idVec3& translation, int id )
{
}

/*
================
idPhysics_Base::Rotate
================
*/
void idPhysics_Base::Rotate( const idRotation& rotation, int id )
{
}

/*
================
idPhysics_Base::GetOrigin
================
*/
const idVec3& idPhysics_Base::GetOrigin( int id ) const
{
	return vec3_origin;
}

/*
================
idPhysics_Base::GetAxis
================
*/
const idMat3& idPhysics_Base::GetAxis( int id ) const
{
	return mat3_identity;
}

/*
================
idPhysics_Base::SetLinearVelocity
================
*/
void idPhysics_Base::SetLinearVelocity( const idVec3& newLinearVelocity, int id )
{
}

/*
================
idPhysics_Base::SetAngularVelocity
================
*/
void idPhysics_Base::SetAngularVelocity( const idVec3& newAngularVelocity, int id )
{
}

/*
================
idPhysics_Base::GetLinearVelocity
================
*/
const idVec3& idPhysics_Base::GetLinearVelocity( int id ) const
{
	return vec3_origin;
}

/*
================
idPhysics_Base::GetAngularVelocity
================
*/
const idVec3& idPhysics_Base::GetAngularVelocity( int id ) const
{
	return vec3_origin;
}

/*
================
idPhysics_Base::SetGravity
================
*/
void idPhysics_Base::SetGravity( const idVec3& newGravity )
{
	gravityVector = newGravity;
	gravityNormal = newGravity;
	gravityNormal.Normalize();
}

/*
================
idPhysics_Base::GetGravity
================
*/
const idVec3& idPhysics_Base::GetGravity() const
{
	return gravityVector;
}

/*
================
idPhysics_Base::GetGravityNormal
================
*/
const idVec3& idPhysics_Base::GetGravityNormal() const
{
	return gravityNormal;
}

/*
================
idPhysics_Base::ClipTranslation
================
*/
void idPhysics_Base::ClipTranslation( trace_t& results, const idVec3& translation, const idClipModel* model ) const
{
	memset( &results, 0, sizeof( trace_t ) );
}

/*
================
idPhysics_Base::ClipRotation
================
*/
void idPhysics_Base::ClipRotation( trace_t& results, const idRotation& rotation, const idClipModel* model ) const
{
	memset( &results, 0, sizeof( trace_t ) );
}

/*
================
idPhysics_Base::ClipContents
================
*/
int idPhysics_Base::ClipContents( const idClipModel* model ) const
{
	return 0;
}

/*
================
idPhysics_Base::DisableClip
================
*/
void idPhysics_Base::DisableClip()
{
}

/*
================
idPhysics_Base::EnableClip
================
*/
void idPhysics_Base::EnableClip()
{
}

/*
================
idPhysics_Base::UnlinkClip
================
*/
void idPhysics_Base::UnlinkClip()
{
}

/*
================
idPhysics_Base::LinkClip
================
*/
void idPhysics_Base::LinkClip()
{
}

/*
================
idPhysics_Base::EvaluateContacts
================
*/
bool idPhysics_Base::EvaluateContacts()
{
	return false;
}

/*
================
idPhysics_Base::GetNumContacts
================
*/
int idPhysics_Base::GetNumContacts() const
{
	return contacts.Num();
}

/*
================
idPhysics_Base::GetContact
================
*/
const contactInfo_t& idPhysics_Base::GetContact( int num ) const
{
	return contacts[num];
}

/*
================
idPhysics_Base::ClearContacts
================
*/
void idPhysics_Base::ClearContacts()
{
	int i;
	idEntity* ent;

	for( i = 0; i < contacts.Num(); i++ )
	{
		ent = gameLocal.entities[ contacts[i].entityNum ];
		if( ent )
		{
			ent->RemoveContactEntity( self );
		}
	}
	contacts.SetNum( 0 );
}

/*
================
idPhysics_Base::AddContactEntity
================
*/
void idPhysics_Base::AddContactEntity( idEntity* e )
{
	int i;
	idEntity* ent;
	bool found = false;

	for( i = 0; i < contactEntities.Num(); i++ )
	{
		ent = contactEntities[i].GetEntity();
		if( ent == NULL )
		{
			contactEntities.RemoveIndex( i-- );
		}
		if( ent == e )
		{
			found = true;
		}
	}
	if( !found )
	{
		contactEntities.Alloc() = e;
	}
}

/*
================
idPhysics_Base::RemoveContactEntity
================
*/
void idPhysics_Base::RemoveContactEntity( idEntity* e )
{
	int i;
	idEntity* ent;

	for( i = 0; i < contactEntities.Num(); i++ )
	{
		ent = contactEntities[i].GetEntity();
		if( !ent )
		{
			contactEntities.RemoveIndex( i-- );
			continue;
		}
		if( ent == e )
		{
			contactEntities.RemoveIndex( i-- );
			return;
		}
	}
}

/*
================
idPhysics_Base::HasGroundContacts
================
*/
bool idPhysics_Base::HasGroundContacts() const
{
	int i;

	for( i = 0; i < contacts.Num(); i++ )
	{
		if( contacts[i].normal * -gravityNormal > 0.0f )
		{
			return true;
		}
	}
	return false;
}

/*
================
idPhysics_Base::IsGroundEntity
================
*/
bool idPhysics_Base::IsGroundEntity( int entityNum ) const
{
	int i;

	for( i = 0; i < contacts.Num(); i++ )
	{
		if( contacts[i].entityNum == entityNum && ( contacts[i].normal * -gravityNormal > 0.0f ) )
		{
			return true;
		}
	}
	return false;
}

/*
================
idPhysics_Base::IsGroundClipModel
================
*/
bool idPhysics_Base::IsGroundClipModel( int entityNum, int id ) const
{
	int i;

	for( i = 0; i < contacts.Num(); i++ )
	{
		if( contacts[i].entityNum == entityNum && contacts[i].id == id && ( contacts[i].normal * -gravityNormal > 0.0f ) )
		{
			return true;
		}
	}
	return false;
}

/*
================
idPhysics_Base::SetPushed
================
*/
void idPhysics_Base::SetPushed( int deltaTime )
{
}

/*
================
idPhysics_Base::GetPushedLinearVelocity
================
*/
const idVec3& idPhysics_Base::GetPushedLinearVelocity( const int id ) const
{
	return vec3_origin;
}

/*
================
idPhysics_Base::GetPushedAngularVelocity
================
*/
const idVec3& idPhysics_Base::GetPushedAngularVelocity( const int id ) const
{
	return vec3_origin;
}

/*
================
idPhysics_Base::SetMaster
================
*/
void idPhysics_Base::SetMaster( idEntity* master, const bool orientated )
{
}

/*
================
idPhysics_Base::GetBlockingInfo
================
*/
const trace_t* idPhysics_Base::GetBlockingInfo() const
{
	return NULL;
}

/*
================
idPhysics_Base::GetBlockingEntity
================
*/
idEntity* idPhysics_Base::GetBlockingEntity() const
{
	return NULL;
}

/*
================
idPhysics_Base::GetLinearEndTime
================
*/
int idPhysics_Base::GetLinearEndTime() const
{
	return 0;
}

/*
================
idPhysics_Base::GetAngularEndTime
================
*/
int idPhysics_Base::GetAngularEndTime() const
{
	return 0;
}

/*
================
idPhysics_Base::AddGroundContacts
================
*/
void idPhysics_Base::AddGroundContacts( const idClipModel* clipModel )
{
	idVec6 dir;
	int index, num;

	index = contacts.Num();
	contacts.SetNum( index + 10 );

	dir.SubVec3( 0 ) = gravityNormal;
	dir.SubVec3( 1 ) = vec3_origin;
	num = gameLocal.clip.Contacts( &contacts[index], 10, clipModel->GetOrigin(),
								   dir, CONTACT_EPSILON, clipModel, clipModel->GetAxis(), clipMask, self );
	contacts.SetNum( index + num );
}

/*
================
idPhysics_Base::AddContactEntitiesForContacts
================
*/
void idPhysics_Base::AddContactEntitiesForContacts()
{
	int i;
	idEntity* ent;

	for( i = 0; i < contacts.Num(); i++ )
	{
		ent = gameLocal.entities[ contacts[i].entityNum ];
		if( ent && ent != self )
		{
			ent->AddContactEntity( self );
		}
	}
}

/*
================
idPhysics_Base::ActivateContactEntities
================
*/
void idPhysics_Base::ActivateContactEntities()
{
	int i;
	idEntity* ent;

	for( i = 0; i < contactEntities.Num(); i++ )
	{
		ent = contactEntities[i].GetEntity();
		if( ent )
		{
			ent->ActivatePhysics( self );
		}
		else
		{
			contactEntities.RemoveIndex( i-- );
		}
	}
}

/*
================
idPhysics_Base::IsOutsideWorld
================
*/
bool idPhysics_Base::IsOutsideWorld() const
{
	if( !gameLocal.clip.GetWorldBounds().Expand( 128.0f ).IntersectsBounds( GetAbsBounds() ) )
	{
		return true;
	}
	return false;
}

/*
================
idPhysics_Base::DrawVelocity
================
*/
void idPhysics_Base::DrawVelocity( int id, float linearScale, float angularScale ) const
{
	idVec3 dir, org, vec, start, end;
	idMat3 axis;
	float length, a;

	dir = GetLinearVelocity( id );
	dir *= linearScale;
	if( dir.LengthSqr() > Square( 0.1f ) )
	{
		dir = dir.Truncate( 10.0f );
		org = GetOrigin( id );
		gameRenderWorld->DebugArrow( colorRed, org, org + dir, 1 );
	}

	dir = GetAngularVelocity( id );
	length = dir.Normalize();
	length *= angularScale;
	if( length > 0.1f )
	{
		if( length < 60.0f )
		{
			length = 60.0f;
		}
		else if( length > 360.0f )
		{
			length = 360.0f;
		}
		axis = GetAxis( id );
		vec = axis[2];
		if( idMath::Fabs( dir * vec ) > 0.99f )
		{
			vec = axis[0];
		}
		vec -= vec * dir * vec;
		vec.Normalize();
		vec *= 4.0f;
		start = org + vec;
		for( a = 20.0f; a < length; a += 20.0f )
		{
			end = org + idRotation( vec3_origin, dir, -a ).ToMat3() * vec;
			gameRenderWorld->DebugLine( colorBlue, start, end, 1 );
			start = end;
		}
		end = org + idRotation( vec3_origin, dir, -length ).ToMat3() * vec;
		gameRenderWorld->DebugArrow( colorBlue, start, end, 1 );
	}
}

/*
================
idPhysics_Base::WriteToSnapshot
================
*/
void idPhysics_Base::WriteToSnapshot( idBitMsg& msg ) const
{
}

/*
================
idPhysics_Base::ReadFromSnapshot
================
*/
void idPhysics_Base::ReadFromSnapshot( const idBitMsg& msg )
{
}
