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

#ifndef __GAME_EDIT_H__
#define __GAME_EDIT_H__


/*
===============================================================================

	Ingame cursor.

===============================================================================
*/

class idCursor3D : public idEntity
{
public:
	CLASS_PROTOTYPE( idCursor3D );

	idCursor3D();
	~idCursor3D();

	void					Spawn();
	void					Present();
	void					Think();

	idForce_Drag			drag;
	idVec3					draggedPosition;
};


/*
===============================================================================

	Allows entities to be dragged through the world with physics.

===============================================================================
*/

class idDragEntity
{
public:
	idDragEntity();
	~idDragEntity();

	void					Clear();
	void					Update( idPlayer* player );
	void					SetSelected( idEntity* ent );
	idEntity* 				GetSelected() const
	{
		return selected.GetEntity();
	}
	void					DeleteSelected();
	void					BindSelected();
	void					UnbindSelected();

private:
	idEntityPtr<idEntity>	dragEnt;			// entity being dragged
	jointHandle_t			joint;				// joint being dragged
	int						id;					// id of body being dragged
	idVec3					localEntityPoint;	// dragged point in entity space
	idVec3					localPlayerPoint;	// dragged point in player space
	idStr					bodyName;			// name of the body being dragged
	idCursor3D* 			cursor;				// cursor entity
	idEntityPtr<idEntity>	selected;			// last dragged entity

	void					StopDrag();
};


/*
===============================================================================

	Handles ingame entity editing.

===============================================================================
*/
typedef struct selectedTypeInfo_s
{
	idTypeInfo* typeInfo;
	idStr		textKey;
} selectedTypeInfo_t;

class idEditEntities
{
public:
	idEditEntities();
	bool					SelectEntity( const idVec3& origin, const idVec3& dir, const idEntity* skip );
	void					AddSelectedEntity( idEntity* ent );
	void					RemoveSelectedEntity( idEntity* ent );
	void					ClearSelectedEntities();
	void					DisplayEntities();
	bool					EntityIsSelectable( idEntity* ent, idVec4* color = NULL, idStr* text = NULL );
private:
	int						nextSelectTime;
	idList<selectedTypeInfo_t> selectableEntityClasses;
	idList<idEntity*>		selectedEntities;
};

#endif /* !__GAME_EDIT_H__ */
