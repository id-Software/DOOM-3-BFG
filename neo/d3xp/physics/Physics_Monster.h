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

#ifndef __PHYSICS_MONSTER_H__
#define __PHYSICS_MONSTER_H__

/*
===================================================================================

	Monster physics

	Simulates the motion of a monster through the environment. The monster motion
	is typically driven by animations.

===================================================================================
*/

typedef enum
{
	MM_OK,
	MM_SLIDING,
	MM_BLOCKED,
	MM_STEPPED,
	MM_FALLING
} monsterMoveResult_t;

typedef struct monsterPState_s
{
	int						atRest;
	bool					onGround;
	idVec3					origin;
	idVec3					velocity;
	idVec3					localOrigin;
	idVec3					pushVelocity;
} monsterPState_t;

class idPhysics_Monster : public idPhysics_Actor
{

public:
	CLASS_PROTOTYPE( idPhysics_Monster );

	idPhysics_Monster();

	void					Save( idSaveGame* savefile ) const;
	void					Restore( idRestoreGame* savefile );

	// maximum step up the monster can take, default 18 units
	void					SetMaxStepHeight( const float newMaxStepHeight );
	float					GetMaxStepHeight() const;
	// minimum cosine of floor angle to be able to stand on the floor
	void					SetMinFloorCosine( const float newMinFloorCosine );
	// set delta for next move
	void					SetDelta( const idVec3& d );
	// returns true if monster is standing on the ground
	bool					OnGround() const;
	// returns the movement result
	monsterMoveResult_t		GetMoveResult() const;
	// overrides any velocity for pure delta movement
	void					ForceDeltaMove( bool force );
	// whether velocity should be affected by gravity
	void					UseFlyMove( bool force );
	// don't use delta movement
	void					UseVelocityMove( bool force );
	// get entity blocking the move
	idEntity* 				GetSlideMoveEntity() const;
	// enable/disable activation by impact
	void					EnableImpact();
	void					DisableImpact();

public:	// common physics interface
	bool					Evaluate( int timeStepMSec, int endTimeMSec );
	void					UpdateTime( int endTimeMSec );
	int						GetTime() const;

	void					GetImpactInfo( const int id, const idVec3& point, impactInfo_t* info ) const;
	void					ApplyImpulse( const int id, const idVec3& point, const idVec3& impulse );
	void					Activate();
	void					PutToRest();
	bool					IsAtRest() const;
	int						GetRestStartTime() const;

	void					SaveState();
	void					RestoreState();

	void					SetOrigin( const idVec3& newOrigin, int id = -1 );
	void					SetAxis( const idMat3& newAxis, int id = -1 );

	void					Translate( const idVec3& translation, int id = -1 );
	void					Rotate( const idRotation& rotation, int id = -1 );

	void					SetLinearVelocity( const idVec3& newLinearVelocity, int id = 0 );

	const idVec3& 			GetLinearVelocity( int id = 0 ) const;

	void					SetPushed( int deltaTime );
	const idVec3& 			GetPushedLinearVelocity( const int id = 0 ) const;

	void					SetMaster( idEntity* master, const bool orientated = true );

	void					WriteToSnapshot( idBitMsg& msg ) const;
	void					ReadFromSnapshot( const idBitMsg& msg );

private:
	// monster physics state
	monsterPState_t			current;
	monsterPState_t			saved;

	// client interpolation state
	monsterPState_t			previous;
	monsterPState_t			next;

	// properties
	float					maxStepHeight;		// maximum step height
	float					minFloorCosine;		// minimum cosine of floor angle
	idVec3					delta;				// delta for next move

	bool					forceDeltaMove;
	bool					fly;
	bool					useVelocityMove;
	bool					noImpact;			// if true do not activate when another object collides

	// results of last evaluate
	monsterMoveResult_t		moveResult;
	idEntity* 				blockingEntity;

private:
	void					CheckGround( monsterPState_t& state );
	monsterMoveResult_t		SlideMove( idVec3& start, idVec3& velocity, const idVec3& delta );
	monsterMoveResult_t		StepMove( idVec3& start, idVec3& velocity, const idVec3& delta );
	void					Rest();
};

#endif /* !__PHYSICS_MONSTER_H__ */
