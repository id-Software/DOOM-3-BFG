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

#ifndef __PHYSICS_PLAYER_H__
#define __PHYSICS_PLAYER_H__

/*
===================================================================================

	Player physics

	Simulates the motion of a player through the environment. Input from the
	player is used to allow a certain degree of control over the motion.

===================================================================================
*/

// movementType
typedef enum
{
	PM_NORMAL,				// normal physics
	PM_DEAD,				// no acceleration or turning, but free falling
	PM_SPECTATOR,			// flying without gravity but with collision detection
	PM_FREEZE,				// stuck in place without control
	PM_NOCLIP				// flying without collision detection nor gravity
} pmtype_t;

typedef enum
{
	WATERLEVEL_NONE,
	WATERLEVEL_FEET,
	WATERLEVEL_WAIST,
	WATERLEVEL_HEAD
} waterLevel_t;

#define	MAXTOUCH					32

typedef struct playerPState_s
{
	idVec3					origin;
	idVec3					velocity;
	idVec3					localOrigin;
	idVec3					pushVelocity;
	float					stepUp;
	int						movementType;
	int						movementFlags;
	int						movementTime;

	playerPState_s() :
		origin( vec3_zero ),
		velocity( vec3_zero ),
		localOrigin( vec3_zero ),
		pushVelocity( vec3_zero ),
		stepUp( 0.0f ),
		movementType( 0 ),
		movementFlags( 0 ),
		movementTime( 0 )
	{
	}
} playerPState_t;

class idPhysics_Player : public idPhysics_Actor
{

public:
	CLASS_PROTOTYPE( idPhysics_Player );

	idPhysics_Player();

	void					Save( idSaveGame* savefile ) const;
	void					Restore( idRestoreGame* savefile );

	// initialisation
	void					SetSpeed( const float newWalkSpeed, const float newCrouchSpeed );
	void					SetMaxStepHeight( const float newMaxStepHeight );
	float					GetMaxStepHeight() const;
	void					SetMaxJumpHeight( const float newMaxJumpHeight );
	void					SetMovementType( const pmtype_t type );
	void					SetPlayerInput( const usercmd_t& cmd, const idVec3& forwardVector );
	void					SetKnockBack( const int knockBackTime );
	void					SetDebugLevel( bool set );
	// feed back from last physics frame
	waterLevel_t			GetWaterLevel() const;
	int						GetWaterType() const;
	bool					HasJumped() const;
	bool					HasSteppedUp() const;
	float					GetStepUp() const;
	bool					IsCrouching() const;
	bool					OnLadder() const;
	const idVec3& 			PlayerGetOrigin() const;	// != GetOrigin

public:	// common physics interface
	bool					Evaluate( int timeStepMSec, int endTimeMSec );
	bool					Interpolate( const float fraction );
	void					UpdateTime( int endTimeMSec );
	int						GetTime() const;

	void					GetImpactInfo( const int id, const idVec3& point, impactInfo_t* info ) const;
	void					ApplyImpulse( const int id, const idVec3& point, const idVec3& impulse );
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

	bool					ClientPusherLocked( bool& justBecameUnlocked );
	void					SetPushed( int deltaTime );
	void					SetPushedWithAbnormalVelocityHack( int deltaTime );
	const idVec3& 			GetPushedLinearVelocity( const int id = 0 ) const;
	void					ClearPushedVelocity();

	void					SetMaster( idEntity* master, const bool orientated = true );

	void					WriteToSnapshot( idBitMsg& msg ) const;
	void					ReadFromSnapshot( const idBitMsg& msg );

	void					SnapToNextState()
	{
		current = next;
		previous = current;
	}

private:
	// player physics state
	playerPState_t			current;
	playerPState_t			saved;

	// physics state for client interpolation
	playerPState_t			previous;
	playerPState_t			next;

	// properties
	float					walkSpeed;
	float					crouchSpeed;
	float					maxStepHeight;
	float					maxJumpHeight;
	int						debugLevel;				// if set, diagnostic output will be printed

	// player input
	usercmd_t				command;
	idVec3					commandForward;		// can't use cmd.angles cause of the delta_angles and head tracking

	// run-time variables
	int						framemsec;
	float					frametime;
	float					playerSpeed;
	idVec3					viewForward;
	idVec3					viewRight;

	// walk movement
	bool					walking;
	bool					groundPlane;
	trace_t					groundTrace;
	const idMaterial* 		groundMaterial;

	// ladder movement
	bool					ladder;
	idVec3					ladderNormal;

	// results of last evaluate
	waterLevel_t			waterLevel;
	int						waterType;

	bool					clientPusherLocked = false;			// SRS - initialize to unlocked at start

private:
	float					CmdScale( const usercmd_t& cmd ) const;
	void					Accelerate( const idVec3& wishdir, const float wishspeed, const float accel );
	bool					SlideMove( bool gravity, bool stepUp, bool stepDown, bool push );
	void					Friction();
	void					WaterJumpMove();
	void					WaterMove();
	void					FlyMove();
	void					AirMove();
	void					WalkMove();
	void					DeadMove();
	void					NoclipMove();
	void					SpectatorMove();
	void					LadderMove();
	void					CorrectAllSolid( trace_t& trace, int contents );
	void					CheckGround();
	void					CheckDuck();
	void					CheckLadder();
	bool					CheckJump();
	bool					CheckWaterJump();
	void					SetWaterLevel();
	void					DropTimers();
	void					MovePlayer( int msec );
};

#endif /* !__PHYSICS_PLAYER_H__ */
