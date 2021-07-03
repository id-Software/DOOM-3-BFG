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

#ifndef __GAME_IK_H__
#define __GAME_IK_H__

/*
===============================================================================

  IK base class with a simple fast two bone solver.

===============================================================================
*/

#define IK_ANIM				"ik_pose"

class idIK
{
public:
	idIK();
	virtual					~idIK();

	void					Save( idSaveGame* savefile ) const;
	void					Restore( idRestoreGame* savefile );

	bool					IsInitialized() const;

	virtual bool			Init( idEntity* self, const char* anim, const idVec3& modelOffset );
	virtual void			Evaluate();
	virtual void			ClearJointMods();

	bool					SolveTwoBones( const idVec3& startPos, const idVec3& endPos, const idVec3& dir, float len0, float len1, idVec3& jointPos );
	float					GetBoneAxis( const idVec3& startPos, const idVec3& endPos, const idVec3& dir, idMat3& axis );

protected:
	bool					initialized;
	bool					ik_activate;
	idEntity* 				self;				// entity using the animated model
	idAnimator* 			animator;			// animator on entity
	int						modifiedAnim;		// animation modified by the IK
	idVec3					modelOffset;
};


/*
===============================================================================

  IK controller for a walking character with an arbitrary number of legs.

===============================================================================
*/

class idIK_Walk : public idIK
{
public:

	idIK_Walk();
	virtual					~idIK_Walk();

	void					Save( idSaveGame* savefile ) const;
	void					Restore( idRestoreGame* savefile );

	virtual bool			Init( idEntity* self, const char* anim, const idVec3& modelOffset );
	virtual void			Evaluate();
	virtual void			ClearJointMods();

	void					EnableAll();
	void					DisableAll();
	void					EnableLeg( int num );
	void					DisableLeg( int num );

private:
	static const int		MAX_LEGS		= 8;

	idClipModel* 			footModel;

	int						numLegs;
	int						enabledLegs;
	jointHandle_t			footJoints[MAX_LEGS];
	jointHandle_t			ankleJoints[MAX_LEGS];
	jointHandle_t			kneeJoints[MAX_LEGS];
	jointHandle_t			hipJoints[MAX_LEGS];
	jointHandle_t			dirJoints[MAX_LEGS];
	jointHandle_t			waistJoint;

	idVec3					hipForward[MAX_LEGS];
	idVec3					kneeForward[MAX_LEGS];

	float					upperLegLength[MAX_LEGS];
	float					lowerLegLength[MAX_LEGS];

	idMat3					upperLegToHipJoint[MAX_LEGS];
	idMat3					lowerLegToKneeJoint[MAX_LEGS];

	float					smoothing;
	float					waistSmoothing;
	float					footShift;
	float					waistShift;
	float					minWaistFloorDist;
	float					minWaistAnkleDist;
	float					footUpTrace;
	float					footDownTrace;
	bool					tiltWaist;
	bool					usePivot;

	// state
	int						pivotFoot;
	float					pivotYaw;
	idVec3					pivotPos;
	bool					oldHeightsValid;
	float					oldWaistHeight;
	float					oldAnkleHeights[MAX_LEGS];
	idVec3					waistOffset;
};


/*
===============================================================================

  IK controller for reaching a position with an arm or leg.

===============================================================================
*/

class idIK_Reach : public idIK
{
public:

	idIK_Reach();
	virtual					~idIK_Reach();

	void					Save( idSaveGame* savefile ) const;
	void					Restore( idRestoreGame* savefile );

	virtual bool			Init( idEntity* self, const char* anim, const idVec3& modelOffset );
	virtual void			Evaluate();
	virtual void			ClearJointMods();

private:

	static const int		MAX_ARMS	= 2;

	int						numArms;
	int						enabledArms;
	jointHandle_t			handJoints[MAX_ARMS];
	jointHandle_t			elbowJoints[MAX_ARMS];
	jointHandle_t			shoulderJoints[MAX_ARMS];
	jointHandle_t			dirJoints[MAX_ARMS];

	idVec3					shoulderForward[MAX_ARMS];
	idVec3					elbowForward[MAX_ARMS];

	float					upperArmLength[MAX_ARMS];
	float					lowerArmLength[MAX_ARMS];

	idMat3					upperArmToShoulderJoint[MAX_ARMS];
	idMat3					lowerArmToElbowJoint[MAX_ARMS];
};

#endif /* !__GAME_IK_H__ */
