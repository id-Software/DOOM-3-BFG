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

#ifndef __GAME_AF_H__
#define __GAME_AF_H__


/*
===============================================================================

  Articulated figure controller.

===============================================================================
*/

typedef struct jointConversion_s
{
	int						bodyId;				// id of the body
	jointHandle_t			jointHandle;		// handle of joint this body modifies
	AFJointModType_t		jointMod;			// modify joint axis, origin or both
	idVec3					jointBodyOrigin;	// origin of body relative to joint
	idMat3					jointBodyAxis;		// axis of body relative to joint
} jointConversion_t;

typedef struct afTouch_s
{
	idEntity* 				touchedEnt;
	idClipModel* 			touchedClipModel;
	idAFBody* 				touchedByBody;
} afTouch_t;

class idAF
{
public:
	idAF();
	~idAF();

	void					Save( idSaveGame* savefile ) const;
	void					Restore( idRestoreGame* savefile );

	void					SetAnimator( idAnimator* a )
	{
		animator = a;
	}
	bool					Load( idEntity* ent, const char* fileName );
	bool					IsLoaded() const
	{
		return isLoaded && self != NULL;
	}
	const char* 			GetName() const
	{
		return name.c_str();
	}
	void					SetupPose( idEntity* ent, int time );
	void					ChangePose( idEntity* ent, int time );
	int						EntitiesTouchingAF( afTouch_t touchList[ MAX_GENTITIES ] ) const;
	void					Start();
	void					StartFromCurrentPose( int inheritVelocityTime );
	void					Stop();
	void					Rest();
	bool					IsActive() const
	{
		return isActive;
	}
	void					SetConstraintPosition( const char* name, const idVec3& pos );

	idPhysics_AF* 			GetPhysics()
	{
		return &physicsObj;
	}
	const idPhysics_AF* 	GetPhysics() const
	{
		return &physicsObj;
	}
	idBounds				GetBounds() const;
	bool					UpdateAnimation();

	void					GetPhysicsToVisualTransform( idVec3& origin, idMat3& axis ) const;
	void					GetImpactInfo( idEntity* ent, int id, const idVec3& point, impactInfo_t* info );
	void					ApplyImpulse( idEntity* ent, int id, const idVec3& point, const idVec3& impulse );
	void					AddForce( idEntity* ent, int id, const idVec3& point, const idVec3& force );
	int						BodyForClipModelId( int id ) const;

	void					SaveState( idDict& args ) const;
	void					LoadState( const idDict& args );

	void					AddBindConstraints();
	void					RemoveBindConstraints();

protected:
	idStr					name;				// name of the loaded .af file
	idPhysics_AF			physicsObj;			// articulated figure physics
	idEntity* 				self;				// entity using the animated model
	idAnimator* 			animator;			// animator on entity
	int						modifiedAnim;		// anim to modify
	idVec3					baseOrigin;			// offset of base body relative to skeletal model origin
	idMat3					baseAxis;			// axis of base body relative to skeletal model origin
	idList<jointConversion_t, TAG_AF>	jointMods;			// list with transforms from skeletal model joints to articulated figure bodies
	idList<int, TAG_AF>		jointBody;			// table to find the nearest articulated figure body for a joint of the skeletal model
	int						poseTime;			// last time the articulated figure was transformed to reflect the current animation pose
	int						restStartTime;		// time the articulated figure came to rest
	bool					isLoaded;			// true when the articulated figure is properly loaded
	bool					isActive;			// true if the articulated figure physics is active
	bool					hasBindConstraints;	// true if the bind constraints have been added

protected:
	void					SetBase( idAFBody* body, const idJointMat* joints );
	void					AddBody( idAFBody* body, const idJointMat* joints, const char* jointName, const AFJointModType_t mod );

	bool					LoadBody( const idDeclAF_Body* fb, const idJointMat* joints );
	bool					LoadConstraint( const idDeclAF_Constraint* fc );

	bool					TestSolid() const;
};

#endif /* !__GAME_AF_H__ */
