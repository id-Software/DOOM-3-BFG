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

#ifndef	__AIMASSIST_H__
#define	__AIMASSIST_H__

/*
================================================================================================
Contains the AimAssist declaration.
================================================================================================
*/

class idEntity;
class idPlayer;

/*
================================================
idAimAssist modifies the angle of Weapon firing to help the Player
hit a Target.
================================================
*/
class idAimAssist
{
public:

	idAimAssist() : player( NULL ), angleCorrection( ang_zero ), frictionScalar( 1.0f ), lastTargetPos( vec3_zero ) {}

	void		Init( idPlayer* player );
	void		Update();
	void		GetAngleCorrection( idAngles& correction ) const
	{
		correction = angleCorrection;
	}
	float		GetFrictionScalar() const
	{
		return frictionScalar;
	}

	idEntity* 	GetLastTarget()
	{
		return targetEntity;
	}
	idEntity* 	FindAimAssistTarget( idVec3& targetPos );

private:
	void		UpdateNewAimAssist();
	float		ComputeEntityAimAssistScore( const idVec3& targetPos, const idVec3& cameraPos, const idMat3& cameraAxis );
	bool		ComputeTargetPos( idEntity* pTarget, idVec3& primaryTargetPos, idVec3& secondaryTargetPos );
	float		ComputeFrictionRadius( float distanceToTarget );
	void		UpdateAdhesion( idEntity* pTarget, const idVec3& targetPos );
	void		UpdateFriction( idEntity* pTarget, const idVec3& targetPos );

	idPlayer* 				player;						// player associated with this object
	idAngles				angleCorrection;			// the angle delta to apply for aim assistance
	float					frictionScalar;				// friction scalar
	idEntityPtr<idEntity>	targetEntity;				// the last target we had (updated every frame)
	idVec3					lastTargetPos;				// the last target position ( updated every frame );
};

#endif // !__AIMASSIST_H__
