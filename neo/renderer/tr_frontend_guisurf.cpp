/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2014 Robert Beckebans

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

#include "precompiled.h"
#pragma hdrstop

#include "RenderCommon.h"
#include "Model_local.h"

/*
==========================================================================================

GUI SURFACES

==========================================================================================
*/

/*
================
R_SurfaceToTextureAxis

Calculates two axis for the surface such that a point dotted against
the axis will give a 0.0 to 1.0 range in S and T when inside the gui surface
================
*/
void R_SurfaceToTextureAxis( const srfTriangles_t* tri, idVec3& origin, idVec3 axis[3] )
{
	// find the bounds of the texture
	idVec2 boundsMin( 999999.0f, 999999.0f );
	idVec2 boundsMax( -999999.0f, -999999.0f );
	for( int i = 0 ; i < tri->numVerts ; i++ )
	{
		const idVec2 uv = tri->verts[i].GetTexCoord();
		boundsMin.x = Min( uv.x, boundsMin.x );
		boundsMax.x = Max( uv.x, boundsMax.x );
		boundsMin.y = Min( uv.y, boundsMin.y );
		boundsMax.y = Max( uv.y, boundsMax.y );
	}

	// use the floor of the midpoint as the origin of the
	// surface, which will prevent a slight misalignment
	// from throwing it an entire cycle off
	const idVec2 boundsOrg( floor( ( boundsMin.x + boundsMax.x ) * 0.5f ), floor( ( boundsMin.y + boundsMax.y ) * 0.5f ) );

	// determine the world S and T vectors from the first drawSurf triangle

	// RB: added check wether GPU skinning is available at all
	const idJointMat* joints = ( tri->staticModelWithJoints != NULL && r_useGPUSkinning.GetBool() && glConfig.gpuSkinningAvailable ) ? tri->staticModelWithJoints->jointsInverted : NULL;
	// RB end

	const idVec3 aXYZ = idDrawVert::GetSkinnedDrawVertPosition( tri->verts[ tri->indexes[0] ], joints );
	const idVec3 bXYZ = idDrawVert::GetSkinnedDrawVertPosition( tri->verts[ tri->indexes[1] ], joints );
	const idVec3 cXYZ = idDrawVert::GetSkinnedDrawVertPosition( tri->verts[ tri->indexes[2] ], joints );

	const idVec2 aST = tri->verts[ tri->indexes[0] ].GetTexCoord();
	const idVec2 bST = tri->verts[ tri->indexes[1] ].GetTexCoord();
	const idVec2 cST = tri->verts[ tri->indexes[2] ].GetTexCoord();

	float d0[5];
	d0[0] = bXYZ[0] - aXYZ[0];
	d0[1] = bXYZ[1] - aXYZ[1];
	d0[2] = bXYZ[2] - aXYZ[2];
	d0[3] = bST.x - aST.x;
	d0[4] = bST.y - aST.y;

	float d1[5];
	d1[0] = cXYZ[0] - aXYZ[0];
	d1[1] = cXYZ[1] - aXYZ[1];
	d1[2] = cXYZ[2] - aXYZ[2];
	d1[3] = cST.x - aST.x;
	d1[4] = cST.y - aST.y;

	const float area = d0[3] * d1[4] - d0[4] * d1[3];
	if( area == 0.0f )
	{
		axis[0].Zero();
		axis[1].Zero();
		axis[2].Zero();
		return;	// degenerate
	}
	const float inva = 1.0f / area;

	axis[0][0] = ( d0[0] * d1[4] - d0[4] * d1[0] ) * inva;
	axis[0][1] = ( d0[1] * d1[4] - d0[4] * d1[1] ) * inva;
	axis[0][2] = ( d0[2] * d1[4] - d0[4] * d1[2] ) * inva;

	axis[1][0] = ( d0[3] * d1[0] - d0[0] * d1[3] ) * inva;
	axis[1][1] = ( d0[3] * d1[1] - d0[1] * d1[3] ) * inva;
	axis[1][2] = ( d0[3] * d1[2] - d0[2] * d1[3] ) * inva;

	idPlane plane;
	plane.FromPoints( aXYZ, bXYZ, cXYZ );
	axis[2][0] = plane[0];
	axis[2][1] = plane[1];
	axis[2][2] = plane[2];

	// take point 0 and project the vectors to the texture origin
	VectorMA( aXYZ, boundsOrg.x - aST.x, axis[0], origin );
	VectorMA( origin, boundsOrg.y - aST.y, axis[1], origin );
}

/*
=================
R_RenderGuiSurf

Create a texture space on the given surface and
call the GUI generator to create quads for it.
=================
*/
static void R_RenderGuiSurf( idUserInterface* gui, const drawSurf_t* drawSurf )
{
	SCOPED_PROFILE_EVENT( "R_RenderGuiSurf" );

	// for testing the performance hit
	if( r_skipGuiShaders.GetInteger() == 1 )
	{
		return;
	}

	// don't allow an infinite recursion loop
	if( tr.guiRecursionLevel == 4 )
	{
		return;
	}

	tr.pc.c_guiSurfs++;

	// create the new matrix to draw on this surface
	idVec3 origin, axis[3];
	R_SurfaceToTextureAxis( drawSurf->frontEndGeo, origin, axis );

	float guiModelMatrix[16];
	float modelMatrix[16];

	guiModelMatrix[0 * 4 + 0] = axis[0][0] * ( 1.0f / SCREEN_WIDTH );
	guiModelMatrix[1 * 4 + 0] = axis[1][0] * ( 1.0f / SCREEN_HEIGHT );
	guiModelMatrix[2 * 4 + 0] = axis[2][0];
	guiModelMatrix[3 * 4 + 0] = origin[0];

	guiModelMatrix[0 * 4 + 1] = axis[0][1] * ( 1.0f / SCREEN_WIDTH );
	guiModelMatrix[1 * 4 + 1] = axis[1][1] * ( 1.0f / SCREEN_HEIGHT );
	guiModelMatrix[2 * 4 + 1] = axis[2][1];
	guiModelMatrix[3 * 4 + 1] = origin[1];

	guiModelMatrix[0 * 4 + 2] = axis[0][2] * ( 1.0f / SCREEN_WIDTH );
	guiModelMatrix[1 * 4 + 2] = axis[1][2] * ( 1.0f / SCREEN_HEIGHT );

	guiModelMatrix[2 * 4 + 2] = axis[2][2];
	guiModelMatrix[3 * 4 + 2] = origin[2];

	guiModelMatrix[0 * 4 + 3] = 0.0f;
	guiModelMatrix[1 * 4 + 3] = 0.0f;
	guiModelMatrix[2 * 4 + 3] = 0.0f;
	guiModelMatrix[3 * 4 + 3] = 1.0f;

	R_MatrixMultiply( guiModelMatrix, drawSurf->space->modelMatrix, modelMatrix );

	tr.guiRecursionLevel++;

	// call the gui, which will call the 2D drawing functions
	tr.guiModel->Clear();
	gui->Redraw( tr.viewDef->renderView.time[0] );
	tr.guiModel->EmitToCurrentView( modelMatrix, drawSurf->space->weaponDepthHack );
	tr.guiModel->Clear();

	tr.guiRecursionLevel--;
}

/*
================
R_AddInGameGuis
================
*/
void R_AddInGameGuis( const drawSurf_t* const drawSurfs[], const int numDrawSurfs )
{
	SCOPED_PROFILE_EVENT( "R_AddInGameGuis" );

	// check for gui surfaces
	for( int i = 0; i < numDrawSurfs; i++ )
	{
		const drawSurf_t* drawSurf = drawSurfs[i];
		idUserInterface* gui = drawSurf->material->GlobalGui();

		int guiNum = drawSurf->material->GetEntityGui() - 1;
		if( guiNum >= 0 && guiNum < MAX_RENDERENTITY_GUI )
		{
			if( drawSurf->space->entityDef != NULL )
			{
				gui = drawSurf->space->entityDef->parms.gui[guiNum];
			}
		}

		if( gui == NULL )
		{
			continue;
		}

		idBounds ndcBounds;
		if( !R_PreciseCullSurface( drawSurf, ndcBounds ) )
		{
			// did we ever use this to forward an entity color to a gui that didn't set color?
			//	memcpy( tr.guiShaderParms, shaderParms, sizeof( tr.guiShaderParms ) );
			R_RenderGuiSurf( gui, drawSurf );
		}
	}
}

/*
================,
R_ReloadGuis_f

Reloads any guis that have had their file timestamps changed.
An optional "all" parameter will cause all models to reload, even
if they are not out of date.

Should we also reload the map models?
================
*/
void R_ReloadGuis_f( const idCmdArgs& args )
{
	bool all;

	if( !idStr::Icmp( args.Argv( 1 ), "all" ) )
	{
		all = true;
		common->Printf( "Reloading all gui files...\n" );
	}
	else
	{
		all = false;
		common->Printf( "Checking for changed gui files...\n" );
	}

	uiManager->Reload( all );
}

/*
================,
R_ListGuis_f
================
*/
void R_ListGuis_f( const idCmdArgs& args )
{
	uiManager->ListGuis();
}
