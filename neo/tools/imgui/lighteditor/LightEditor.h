/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.
Copyright (C) 2015 Daniel Gibson
Copyright (C) 2016-2023 Robert Beckebans

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

// a GUI light editor, based loosely on the one from original Doom3 (neo/tools/radiant/LightDlg.*)
// LightInfo was CLightInfo, the LightEditor itself was written from scratch.

#ifndef NEO_TOOLS_EDITORS_LIGHTEDITOR_H_
#define NEO_TOOLS_EDITORS_LIGHTEDITOR_H_

#include <idlib/Dict.h>
#include "../../edit_public.h"

#include "../imgui/BFGimgui.h"
#include "../imgui/ImGuizmo.h"

namespace ImGuiTools
{

enum ELightType
{
	LIGHT_POINT,
	LIGHT_SPOT,
	LIGHT_SUN
};

class LightInfo
{
public:
	ELightType	lightType;

	idStr		strTexture;
	bool		equalRadius;
	bool		explicitStartEnd;
	idVec3		lightStart;
	idVec3		lightEnd;
	idVec3		lightUp;
	idVec3		lightRight;
	idVec3		lightTarget;
	idVec3		lightCenter;
	idVec3		color;

	bool		hasLightOrigin;
	idVec3		origin;
	idAngles	angles;			// RBDOOM specific, saved to map as "angles"
	idVec3		scale;			// not saved to .map

	idVec3		lightRadius;
	bool		castShadows;
	bool		skipSpecular;
	bool		hasCenter;
	int			lightStyle;		// RBDOOM specific, saved to map as "style"

	LightInfo();

	void		Defaults();
	void		DefaultPoint();
	void		DefaultProjected();
	void		DefaultSun();
	void		FromDict( const idDict* e );
	void		ToDict( idDict* e );
};

class LightEditor
{
private:
	bool				isShown;

	idStr				title;
	idStr				entityName;
	idVec3				entityPos;

	LightInfo			original;
	LightInfo			cur; // current status of the light
	LightInfo			curNotMoving;

	idEntity*			lightEntity;

	idList<idStr>		textureNames;
	int					currentTextureIndex;
	idImage*			currentTexture;
	const idMaterial*	currentTextureMaterial;

	// RB: light style support
	idList<idStr>		styleNames;
	int					currentStyleIndex;

	ImGuizmo::OPERATION mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
	ImGuizmo::MODE		mCurrentGizmoMode = ImGuizmo::WORLD;

	bool				useSnap = false;
	float				gridSnap[3] = { 4.0f, 4.0f, 4.0f };
	float				angleSnap = 15.0f;
	float				scaleSnap = 0.1f;
	float				bounds[6] = { -0.5f, -0.5f, -0.5f, 0.5f, 0.5f, 0.5f };
	float				boundsSnap[3] = { 0.1f, 0.1f, 0.1f };
	bool				boundSizing = false;
	bool				boundSizingSnap = false;

	bool				shortcutSaveMapEnabled;
	bool				shortcutDuplicateLightEnabled;

	void				LoadLightStyles();
	static bool			StyleItemsGetter( void* data, int idx, const char** out_text );

	void				Init( const idDict* dict, idEntity* light );
	void				Reset();

	void				LoadLightTextures();
	static bool			TextureItemsGetter( void* data, int idx, const char** out_text );
	void				LoadCurrentTexture();

	void				TempApplyChanges();
	void				SaveChanges( bool saveMap );
	void				CancelChanges();

	void				DuplicateLight();

	LightEditor()
	{
		isShown = false;

		Reset();
	}

public:

	static LightEditor&	Instance();
	static void			ReInit( const idDict* dict, idEntity* light );

	inline void			ShowIt( bool show )
	{
		isShown = show;
	}

	inline bool			IsShown() const
	{
		return isShown;
	}

	void				Draw();
};

} //namespace ImGuiTools

#endif
