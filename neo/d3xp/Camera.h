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

#ifndef __GAME_CAMERA_H__
#define __GAME_CAMERA_H__


/*
===============================================================================

Camera providing an alternative view of the level.

===============================================================================
*/

class idCamera : public idEntity
{
public:
	ABSTRACT_PROTOTYPE( idCamera );

	void					Spawn();
	virtual void			GetViewParms( renderView_t* view ) = 0;
	virtual renderView_t* 	GetRenderView();
	virtual void			Stop() {} ;
};

/*
===============================================================================

idCameraView

===============================================================================
*/

class idCameraView : public idCamera
{
public:
	CLASS_PROTOTYPE( idCameraView );
	idCameraView();

	// save games
	void					Save( idSaveGame* savefile ) const;				// archives object for save game file
	void					Restore( idRestoreGame* savefile );				// unarchives object from save game file

	void					Spawn();
	virtual void			GetViewParms( renderView_t* view );
	virtual void			Stop();

protected:
	void					Event_Activate( idEntity* activator );
	void					Event_SetAttachments();
	void					SetAttachment( idEntity** e, const char* p );
	float					fov;
	idEntity*				attachedTo;
	idEntity*				attachedView;
};



/*
===============================================================================

A camera which follows a path defined by an animation.

===============================================================================
*/

typedef struct
{
	idCQuat				q;
	idVec3				t;
	float				fov;
} cameraFrame_t;

class idCameraAnim : public idCamera
{
public:
	CLASS_PROTOTYPE( idCameraAnim );

	idCameraAnim();
	~idCameraAnim();

	// save games
	void					Save( idSaveGame* savefile ) const;				// archives object for save game file
	void					Restore( idRestoreGame* savefile );				// unarchives object from save game file

	void					Spawn();
	virtual void			GetViewParms( renderView_t* view );

private:
	int						threadNum;
	idVec3					offset;
	int						frameRate;
	int						starttime;
	int						cycle;
	idList<int>				cameraCuts;
	idList<cameraFrame_t>	camera;
	idEntityPtr<idEntity>	activator;

	void					Start();
	void					Stop();
	void					Think();

	void					LoadAnim();
	void					Event_Start();
	void					Event_Stop();
	void					Event_SetCallback();
	void					Event_Activate( idEntity* activator );

	void					gltfLoadAnim( idStr gltfFileName, idStr animName );
	void					WriteBinaryCamAnim( idFile* file, ID_TIME_T* _timeStamp = NULL );
	bool					LoadBinaryCamAnim( idFile* file, const ID_TIME_T sourceTimeStamp );
};

#endif /* !__GAME_CAMERA_H__ */
