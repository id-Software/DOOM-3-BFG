/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.
Copyright (C) 2022 Stephen Pridham

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#pragma once

#include "../util/Imgui_IdWidgets.h"

namespace ImGuiTools
{

class AfBodyEditor
{
public:
	AfBodyEditor( idDeclAF* newDecl, idDeclAF_Body* newBody );
	~AfBodyEditor();

	bool Do();

private:

	void InitJointLists();

	bool Position();
	bool PositionProperty();
	bool PitchYawRoll();
	bool CollisionModel();
	bool CollisionModelSize();
	bool InputAfVector( const char* label, idAFVector* vec );

	idDeclAF*			decl;
	idDeclAF_Body*		body;
	int					positionType;
	int					modifyJointType;
	int					comboJoint1;
	int					comboJoint2;
	int					originBoneCenterJoint1;
	int					originBoneCenterJoint2;
	int					originJoint;
	MultiSelectWidget	contentWidget;

	idStr				modifiedJoint;
	idStr				renameBody;
	idStrList			joints;
};

}