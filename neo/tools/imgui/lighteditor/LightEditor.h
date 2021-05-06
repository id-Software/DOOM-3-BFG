/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.
Copyright (C) 2015 Daniel Gibson

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


namespace ImGuiTools
{

class LightInfo
{
public:
	bool		pointLight;
	float		fallOff;
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


#if 0 // FIXME: unused, delete?
	bool		fog;
	idVec4		fogDensity;
#endif // 0

	idVec3		lightRadius;
	bool		castShadows;
	bool		castSpecular;
	bool		hasCenter;
	bool		isParallel;
	int			lightStyle;

	LightInfo();

	void		Defaults();
	void		DefaultProjected();
	void		DefaultPoint();
	void		FromDict( const idDict* e );
	void		ToDict( idDict* e );
};

class LightEditor
{
private:
	idStr	title;
	idStr	entityName;
	idVec3	entityPos;

	LightInfo original;
	LightInfo cur; // current status of the light

	idEntity* lightEntity;

	idList<idStr> textureNames;
	int currentTextureIndex;
	idImage* currentTexture;
	const idMaterial* currentTextureMaterial;

	// RB: light style support
	idList<idStr> styleNames;
	int currentStyleIndex;

	void LoadLightStyles();
	static bool StyleItemsGetter( void* data, int idx, const char** out_text );

	void Init( const idDict* dict, idEntity* light );
	void Reset();

	void LoadLightTextures();
	static bool TextureItemsGetter( void* data, int idx, const char** out_text );
	void LoadCurrentTexture();

	void DrawWindow();

	void TempApplyChanges();
	void SaveChanges();
	void CancelChanges();

	LightEditor()
	{
		Reset();
	}

	static LightEditor TheLightEditor; // FIXME: maybe at some point we could allow more than one..

public:
	static void ReInit( const idDict* dict, idEntity* light );

	static void Draw();

	static bool showIt;
};

} //namespace ImGuiTools

#endif /* NEO_TOOLS_EDITORS_LIGHTEDITOR_H_ */
