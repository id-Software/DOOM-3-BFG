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

#ifndef __ANIM_TESTMODEL_H__
#define __ANIM_TESTMODEL_H__

/*
==============================================================================================

	idTestModel

==============================================================================================
*/

class idTestModel : public idAnimatedEntity
{
public:
	CLASS_PROTOTYPE( idTestModel );

	idTestModel();
	~idTestModel();

	void					Save( idSaveGame* savefile );
	void					Restore( idRestoreGame* savefile );

	void					Spawn();

	virtual bool			ShouldConstructScriptObjectAtSpawn() const;

	void					NextAnim( const idCmdArgs& args );
	void					PrevAnim( const idCmdArgs& args );
	void					NextFrame( const idCmdArgs& args );
	void					PrevFrame( const idCmdArgs& args );
	void					TestAnim( const idCmdArgs& args );
	void					BlendAnim( const idCmdArgs& args );

	static void 			KeepTestModel_f( const idCmdArgs& args );
	static void 			TestModel_f( const idCmdArgs& args );
	static void				ArgCompletion_TestModel( const idCmdArgs& args, void( *callback )( const char* s ) );
	static void 			TestSkin_f( const idCmdArgs& args );
	static void 			TestShaderParm_f( const idCmdArgs& args );
	static void 			TestParticleStopTime_f( const idCmdArgs& args );
	static void 			TestAnim_f( const idCmdArgs& args );
	static void				ArgCompletion_TestAnim( const idCmdArgs& args, void( *callback )( const char* s ) );
	static void 			TestBlend_f( const idCmdArgs& args );
	static void 			TestModelNextAnim_f( const idCmdArgs& args );
	static void 			TestModelPrevAnim_f( const idCmdArgs& args );
	static void 			TestModelNextFrame_f( const idCmdArgs& args );
	static void 			TestModelPrevFrame_f( const idCmdArgs& args );

private:
	idEntityPtr<idEntity>	head;
	idAnimator*				headAnimator;
	idAnim					customAnim;
	idPhysics_Parametric	physicsObj;
	idStr					animname;
	int						anim;
	int						headAnim;
	int						mode;
	int						frame;
	int						starttime;
	int						animtime;

	idList<copyJoints_t>	copyJoints;

	virtual void			Think();

	void					Event_Footstep();
};

#endif /* !__ANIM_TESTMODEL_H__*/
