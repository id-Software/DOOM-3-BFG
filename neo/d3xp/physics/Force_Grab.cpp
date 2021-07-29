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

CLASS_DECLARATION( idForce, idForce_Grab )
END_CLASS


/*
================
idForce_Grab::Save
================
*/
void idForce_Grab::Save( idSaveGame* savefile ) const
{

	savefile->WriteFloat( damping );
	savefile->WriteVec3( goalPosition );
	savefile->WriteFloat( distanceToGoal );
	savefile->WriteInt( id );
}

/*
================
idForce_Grab::Restore
================
*/
void idForce_Grab::Restore( idRestoreGame* savefile )
{

	//Note: Owner needs to call set physics
	savefile->ReadFloat( damping );
	savefile->ReadVec3( goalPosition );
	savefile->ReadFloat( distanceToGoal );
	savefile->ReadInt( id );
}

/*
================
idForce_Grab::idForce_Grab
================
*/
idForce_Grab::idForce_Grab()
{
	damping			= 0.5f;
	physics			= NULL;
	id				= 0;
}

/*
================
idForce_Grab::~idForce_Grab
================
*/
idForce_Grab::~idForce_Grab()
{
}

/*
================
idForce_Grab::Init
================
*/
void idForce_Grab::Init( float damping )
{
	if( damping >= 0.0f && damping < 1.0f )
	{
		this->damping = damping;
	}
}

/*
================
idForce_Grab::SetPhysics
================
*/
void idForce_Grab::SetPhysics( idPhysics* phys, int id, const idVec3& goal )
{
	this->physics = phys;
	this->id = id;
	this->goalPosition = goal;
}

/*
================
idForce_Grab::SetGoalPosition
================
*/
void idForce_Grab::SetGoalPosition( const idVec3& goal )
{
	this->goalPosition = goal;
}

/*
=================
idForce_Grab::GetDistanceToGoal
=================
*/
float idForce_Grab::GetDistanceToGoal()
{
	return distanceToGoal;
}

/*
================
idForce_Grab::Evaluate
================
*/
void idForce_Grab::Evaluate( int time )
{
	if( !physics )
	{
		return;
	}
	idVec3			forceDir, v, objectCenter;
	float			forceAmt;
	float			mass = physics->GetMass( id );

	objectCenter = physics->GetAbsBounds( id ).GetCenter();

	if( g_grabberRandomMotion.GetBool() && !common->IsMultiplayer() )
	{
		// Jitter the objectCenter around so it doesn't remain stationary
		float SinOffset = idMath::Sin( ( float )( gameLocal.time ) / 66.f );
		float randScale1 = gameLocal.random.RandomFloat();
		float randScale2 = gameLocal.random.CRandomFloat();
		objectCenter.x += ( SinOffset * 3.5f * randScale1 ) + ( randScale2 * 1.2f );
		objectCenter.y += ( SinOffset * -3.5f * randScale1 ) + ( randScale2 * 1.4f );
		objectCenter.z += ( SinOffset * 2.4f * randScale1 ) + ( randScale2 * 1.6f );
	}

	forceDir = goalPosition - objectCenter;
	distanceToGoal = forceDir.Normalize();

	float temp = distanceToGoal;
	if( temp > 12.f && temp < 32.f )
	{
		temp = 32.f;
	}
	forceAmt = ( 1000.f * mass ) + ( 500.f * temp * mass );

	if( forceAmt / mass > 120000.f )
	{
		forceAmt = 120000.f * mass;
	}
	physics->AddForce( id, objectCenter, forceDir * forceAmt );

	if( distanceToGoal < 196.f )
	{
		v = physics->GetLinearVelocity( id );
		physics->SetLinearVelocity( v * damping, id );
	}
	if( distanceToGoal < 16.f )
	{
		v = physics->GetAngularVelocity( id );
		if( v.LengthSqr() > Square( 8 ) )
		{
			physics->SetAngularVelocity( v * 0.99999f, id );
		}
	}
}

/*
================
idForce_Grab::RemovePhysics
================
*/
void idForce_Grab::RemovePhysics( const idPhysics* phys )
{
	if( physics == phys )
	{
		physics = NULL;
	}
}

