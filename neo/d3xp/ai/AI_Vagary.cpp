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
/***********************************************************************

game/ai/AI_Vagary.cpp

Vagary specific AI code

***********************************************************************/

#include "precompiled.h"
#pragma hdrstop


#include "../Game_local.h"

class idAI_Vagary : public idAI
{
public:
	CLASS_PROTOTYPE( idAI_Vagary );

private:
	void	Event_ChooseObjectToThrow( const idVec3& mins, const idVec3& maxs, float speed, float minDist, float offset );
	void	Event_ThrowObjectAtEnemy( idEntity* ent, float speed );
};

const idEventDef AI_Vagary_ChooseObjectToThrow( "vagary_ChooseObjectToThrow", "vvfff", 'e' );
const idEventDef AI_Vagary_ThrowObjectAtEnemy( "vagary_ThrowObjectAtEnemy", "ef" );

CLASS_DECLARATION( idAI, idAI_Vagary )
EVENT( AI_Vagary_ChooseObjectToThrow,	idAI_Vagary::Event_ChooseObjectToThrow )
EVENT( AI_Vagary_ThrowObjectAtEnemy,	idAI_Vagary::Event_ThrowObjectAtEnemy )
END_CLASS

/*
================
idAI_Vagary::Event_ChooseObjectToThrow
================
*/
void idAI_Vagary::Event_ChooseObjectToThrow( const idVec3& mins, const idVec3& maxs, float speed, float minDist, float offset )
{
	idEntity* 	ent;
	idEntity* 	entityList[ MAX_GENTITIES ];
	int			numListedEntities;
	int			i, index;
	float		dist;
	idVec3		vel;
	idVec3		offsetVec( 0, 0, offset );
	idEntity*	enemyEnt = enemy.GetEntity();

	if( !enemyEnt )
	{
		idThread::ReturnEntity( NULL );
		return;
	}

	idVec3 enemyEyePos = lastVisibleEnemyPos + lastVisibleEnemyEyeOffset;
	const idBounds& myBounds = physicsObj.GetAbsBounds();
	idBounds checkBounds( mins, maxs );
	checkBounds.TranslateSelf( physicsObj.GetOrigin() );
	numListedEntities = gameLocal.clip.EntitiesTouchingBounds( checkBounds, -1, entityList, MAX_GENTITIES );

	index = gameLocal.random.RandomInt( numListedEntities );
	for( i = 0; i < numListedEntities; i++, index++ )
	{
		if( index >= numListedEntities )
		{
			index = 0;
		}
		ent = entityList[ index ];
		if( !ent->IsType( idMoveable::Type ) )
		{
			continue;
		}

		if( ent->fl.hidden )
		{
			// don't throw hidden objects
			continue;
		}

		idPhysics* entPhys = ent->GetPhysics();
		const idVec3& entOrg = entPhys->GetOrigin();
		dist = ( entOrg - enemyEyePos ).LengthFast();
		if( dist < minDist )
		{
			continue;
		}

		idBounds expandedBounds = myBounds.Expand( entPhys->GetBounds().GetRadius() );
		if( expandedBounds.LineIntersection( entOrg, enemyEyePos ) )
		{
			// ignore objects that are behind us
			continue;
		}

		if( PredictTrajectory( entPhys->GetOrigin() + offsetVec, enemyEyePos, speed, entPhys->GetGravity(),
							   entPhys->GetClipModel(), entPhys->GetClipMask(), MAX_WORLD_SIZE, NULL, enemyEnt, ai_debugTrajectory.GetBool() ? 4000 : 0, vel ) )
		{
			idThread::ReturnEntity( ent );
			return;
		}
	}

	idThread::ReturnEntity( NULL );
}

/*
================
idAI_Vagary::Event_ThrowObjectAtEnemy
================
*/
void idAI_Vagary::Event_ThrowObjectAtEnemy( idEntity* ent, float speed )
{
	idVec3		vel;
	idEntity*	enemyEnt;
	idPhysics*	entPhys;

	entPhys	= ent->GetPhysics();
	enemyEnt = enemy.GetEntity();
	if( !enemyEnt )
	{
		vel = ( viewAxis[ 0 ] * physicsObj.GetGravityAxis() ) * speed;
	}
	else
	{
		PredictTrajectory( entPhys->GetOrigin(), lastVisibleEnemyPos + lastVisibleEnemyEyeOffset, speed, entPhys->GetGravity(),
						   entPhys->GetClipModel(), entPhys->GetClipMask(), MAX_WORLD_SIZE, NULL, enemyEnt, ai_debugTrajectory.GetBool() ? 4000 : 0, vel );
		vel *= speed;
	}

	entPhys->SetLinearVelocity( vel );

	if( ent->IsType( idMoveable::Type ) )
	{
		idMoveable* ment = static_cast<idMoveable*>( ent );
		ment->EnableDamage( true, 2.5f );
	}
}
