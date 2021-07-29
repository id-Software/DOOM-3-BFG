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

CLASS_DECLARATION( idForce, idForce_Spring )
END_CLASS

/*
================
idForce_Spring::idForce_Spring
================
*/
idForce_Spring::idForce_Spring()
{
	Kstretch		= 100.0f;
	Kcompress		= 100.0f;
	damping			= 0.0f;
	restLength		= 0.0f;
	physics1		= NULL;
	id1				= 0;
	p1				= vec3_zero;
	physics2		= NULL;
	id2				= 0;
	p2				= vec3_zero;
}

/*
================
idForce_Spring::~idForce_Spring
================
*/
idForce_Spring::~idForce_Spring()
{
}

/*
================
idForce_Spring::InitSpring
================
*/
void idForce_Spring::InitSpring( float Kstretch, float Kcompress, float damping, float restLength )
{
	this->Kstretch = Kstretch;
	this->Kcompress = Kcompress;
	this->damping = damping;
	this->restLength = restLength;
}

/*
================
idForce_Spring::SetPosition
================
*/
void idForce_Spring::SetPosition( idPhysics* physics1, int id1, const idVec3& p1, idPhysics* physics2, int id2, const idVec3& p2 )
{
	this->physics1 = physics1;
	this->id1 = id1;
	this->p1 = p1;
	this->physics2 = physics2;
	this->id2 = id2;
	this->p2 = p2;
}

/*
================
idForce_Spring::Evaluate
================
*/
void idForce_Spring::Evaluate( int time )
{
	float length;
	idMat3 axis;
	idVec3 pos1, pos2, velocity1, velocity2, force, dampingForce;
	impactInfo_t info;

	pos1 = p1;
	pos2 = p2;
	velocity1 = velocity2 = vec3_origin;

	if( physics1 )
	{
		axis = physics1->GetAxis( id1 );
		pos1 = physics1->GetOrigin( id1 );
		pos1 += p1 * axis;
		if( damping > 0.0f )
		{
			physics1->GetImpactInfo( id1, pos1, &info );
			velocity1 = info.velocity;
		}
	}

	if( physics2 )
	{
		axis = physics2->GetAxis( id2 );
		pos2 = physics2->GetOrigin( id2 );
		pos2 += p2 * axis;
		if( damping > 0.0f )
		{
			physics2->GetImpactInfo( id2, pos2, &info );
			velocity2 = info.velocity;
		}
	}

	force = pos2 - pos1;
	dampingForce = ( damping * ( ( ( velocity2 - velocity1 ) * force ) / ( force * force ) ) ) * force;
	length = force.Normalize();

	// if the spring is stretched
	if( length > restLength )
	{
		if( Kstretch > 0.0f )
		{
			force = ( Square( length - restLength ) * Kstretch ) * force - dampingForce;
			if( physics1 )
			{
				physics1->AddForce( id1, pos1, force );
			}
			if( physics2 )
			{
				physics2->AddForce( id2, pos2, -force );
			}
		}
	}
	else
	{
		if( Kcompress > 0.0f )
		{
			force = ( Square( length - restLength ) * Kcompress ) * force - dampingForce;
			if( physics1 )
			{
				physics1->AddForce( id1, pos1, -force );
			}
			if( physics2 )
			{
				physics2->AddForce( id2, pos2, force );
			}
		}
	}
}

/*
================
idForce_Spring::RemovePhysics
================
*/
void idForce_Spring::RemovePhysics( const idPhysics* phys )
{
	if( physics1 == phys )
	{
		physics1 = NULL;
	}
	if( physics2 == phys )
	{
		physics2 = NULL;
	}
}
