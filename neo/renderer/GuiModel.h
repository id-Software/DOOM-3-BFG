/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013-2020 Robert Beckebans
Copyright (C) 2022 Stephen Pridham

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

#include "ScreenRect.h"

struct guiModelSurface_t
{
	const idMaterial* 	material;
	uint64				glState;
	int					firstIndex;
	int					numIndexes;
	stereoDepthType_t	stereoType;
	idScreenRect		clipRect;
};

class idRenderMatrix;
class Framebuffer;

namespace ImGui
{
struct ImDrawData;
}

struct ImDrawData;

class idGuiModel
{
public:
	idGuiModel();

	void		Clear();

	void		WriteToDemo( idDemoFile* demo );
	void		ReadFromDemo( idDemoFile* demo );

	// allocates memory for verts and indexes in frame-temporary buffer memory
	void		BeginFrame();

	void		EmitToCurrentView( float modelMatrix[16], bool depthHack );
	void		EmitFullScreen( Framebuffer* renderTarget = nullptr );
	void		EmitSurfaces( float modelMatrix[16], float modelViewMatrix[16], bool depthHack, bool allowFullScreenStereoDepth, bool linkAsEntity );

	// RB
	void		EmitImGui( ImDrawData* drawData );

	// the returned pointer will be in write-combined memory, so only make contiguous
	// 32 bit writes and never read from it.
	idDrawVert* AllocTris( int numVerts, const triIndex_t* indexes, int numIndexes, const idMaterial* material,
						   const uint64 glState, const stereoDepthType_t stereoType );
	idDrawVert* AllocTris( int numVerts, const triIndex_t* indexes, int numIndexes, const idMaterial* material,
						   const uint64 glState, const stereoDepthType_t stereoType, const idScreenRect& clipRect );

	//---------------------------
private:
	void		AdvanceSurf();

	guiModelSurface_t* 			surf;

	float						shaderParms[ MAX_ENTITY_SHADER_PARMS ];

	static const float STEREO_DEPTH_NEAR;
	static const float STEREO_DEPTH_MID;
	static const float STEREO_DEPTH_FAR;

	// if we exceed these limits we stop rendering GUI surfaces
	static const int MAX_INDEXES = ( 20000 * 6 );
	static const int MAX_VERTS	 = ( 20000 * 4 );

	vertCacheHandle_t			vertexBlock;
	vertCacheHandle_t			indexBlock;
	idDrawVert* 				vertexPointer;
	triIndex_t* 				indexPointer;

	int							numVerts;
	int							numIndexes;

	idList<guiModelSurface_t, TAG_MODEL>	surfaces;
};

