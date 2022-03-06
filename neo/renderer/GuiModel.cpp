/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013-2020 Robert Beckebans

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
#include "libs/imgui/imgui.h"

const float idGuiModel::STEREO_DEPTH_NEAR = 0.0f;
const float idGuiModel::STEREO_DEPTH_MID  = 0.5f;
const float idGuiModel::STEREO_DEPTH_FAR  = 1.0f;

/*
================
idGuiModel::idGuiModel
================
*/
idGuiModel::idGuiModel()
{
	// identity color for drawsurf register evaluation
	for( int i = 0; i < MAX_ENTITY_SHADER_PARMS; i++ )
	{
		shaderParms[i] = 1.0f;
	}
}

/*
================
idGuiModel::Clear

Begins collecting draw commands into surfaces
================
*/
void idGuiModel::Clear()
{
	surfaces.SetNum( 0 );
	AdvanceSurf();
}

/*
================
idGuiModel::WriteToDemo
================
*/
void idGuiModel::WriteToDemo( idDemoFile* demo )
{
}

/*
================
idGuiModel::ReadFromDemo
================
*/
void idGuiModel::ReadFromDemo( idDemoFile* demo )
{
}

/*
================
idGuiModel::BeginFrame
================
*/
void idGuiModel::BeginFrame()
{
	vertexBlock = vertexCache.AllocVertex( NULL, MAX_VERTS );
	indexBlock = vertexCache.AllocIndex( NULL, MAX_INDEXES );
	vertexPointer = ( idDrawVert* )vertexCache.MappedVertexBuffer( vertexBlock );
	indexPointer = ( triIndex_t* )vertexCache.MappedIndexBuffer( indexBlock );
	numVerts = 0;
	numIndexes = 0;
	Clear();
}

idCVar	stereoRender_defaultGuiDepth( "stereoRender_defaultGuiDepth", "0", CVAR_RENDERER, "Fraction of separation when not specified" );
/*
================
EmitSurfaces

For full screen GUIs, we can add in per-surface stereoscopic depth effects
================
*/
void idGuiModel::EmitSurfaces( float modelMatrix[16], float modelViewMatrix[16],
							   bool depthHack, bool allowFullScreenStereoDepth, bool linkAsEntity )
{

	viewEntity_t* guiSpace = ( viewEntity_t* )R_ClearedFrameAlloc( sizeof( *guiSpace ), FRAME_ALLOC_VIEW_ENTITY );
	memcpy( guiSpace->modelMatrix, modelMatrix, sizeof( guiSpace->modelMatrix ) );
	memcpy( guiSpace->modelViewMatrix, modelViewMatrix, sizeof( guiSpace->modelViewMatrix ) );
	guiSpace->weaponDepthHack = depthHack;
	guiSpace->isGuiSurface = true;

	// If this is an in-game gui, we need to be able to find the matrix again for head mounted
	// display bypass matrix fixup.
	if( linkAsEntity )
	{
		guiSpace->next = tr.viewDef->viewEntitys;
		tr.viewDef->viewEntitys = guiSpace;
	}

	//---------------------------
	// make a tech5 renderMatrix
	//---------------------------
	idRenderMatrix viewMat;
	idRenderMatrix::Transpose( *( idRenderMatrix* )modelViewMatrix, viewMat );
	idRenderMatrix::Multiply( tr.viewDef->projectionRenderMatrix, viewMat, guiSpace->mvp );
	if( depthHack )
	{
		idRenderMatrix::ApplyDepthHack( guiSpace->mvp );
	}

	// to allow 3D-TV effects in the menu system, we define surface flags to set
	// depth fractions between 0=screen and 1=infinity, which directly modulate the
	// screenSeparation parameter for an X offset.
	// The value is stored in the drawSurf sort value, which adjusts the matrix in the
	// backend.
	float defaultStereoDepth = stereoRender_defaultGuiDepth.GetFloat();	// default to at-screen

	// add the surfaces to this view
	for( int i = 0; i < surfaces.Num(); i++ )
	{
		const guiModelSurface_t& guiSurf = surfaces[i];
		if( guiSurf.numIndexes == 0 )
		{
			continue;
		}

		const idMaterial* shader = guiSurf.material;
		drawSurf_t* drawSurf = ( drawSurf_t* )R_FrameAlloc( sizeof( *drawSurf ), FRAME_ALLOC_DRAW_SURFACE );

		drawSurf->numIndexes = guiSurf.numIndexes;
		drawSurf->ambientCache = vertexBlock;
		// build a vertCacheHandle_t that points inside the allocated block
		drawSurf->indexCache = indexBlock + ( ( int64 )( guiSurf.firstIndex * sizeof( triIndex_t ) ) << VERTCACHE_OFFSET_SHIFT );
		drawSurf->shadowCache = 0;
		drawSurf->jointCache = 0;
		drawSurf->frontEndGeo = NULL;
		drawSurf->space = guiSpace;
		drawSurf->material = shader;
		drawSurf->extraGLState = guiSurf.glState;
		drawSurf->scissorRect = tr.viewDef->scissor;
		drawSurf->sort = shader->GetSort();
		drawSurf->renderZFail = 0;
		// process the shader expressions for conditionals / color / texcoords
		const float*	constRegs = shader->ConstantRegisters();
		if( constRegs )
		{
			// shader only uses constant values
			drawSurf->shaderRegisters = constRegs;
		}
		else
		{
			float* regs = ( float* )R_FrameAlloc( shader->GetNumRegisters() * sizeof( float ), FRAME_ALLOC_SHADER_REGISTER );
			drawSurf->shaderRegisters = regs;
			shader->EvaluateRegisters( regs, shaderParms, tr.viewDef->renderView.shaderParms, tr.viewDef->renderView.time[1] * 0.001f, NULL );
		}

		R_LinkDrawSurfToView( drawSurf, tr.viewDef );
		if( allowFullScreenStereoDepth )
		{
			// override sort with the stereoDepth
			//drawSurf->sort = stereoDepth;

			switch( guiSurf.stereoType )
			{
				case STEREO_DEPTH_TYPE_NEAR:
					drawSurf->sort = STEREO_DEPTH_NEAR;
					break;
				case STEREO_DEPTH_TYPE_MID:
					drawSurf->sort = STEREO_DEPTH_MID;
					break;
				case STEREO_DEPTH_TYPE_FAR:
					drawSurf->sort = STEREO_DEPTH_FAR;
					break;
				case STEREO_DEPTH_TYPE_NONE:
				default:
					drawSurf->sort = defaultStereoDepth;
					break;
			}
		}
	}
}

/*
====================
EmitToCurrentView
====================
*/
void idGuiModel::EmitToCurrentView( float modelMatrix[16], bool depthHack )
{
	float	modelViewMatrix[16];

	R_MatrixMultiply( modelMatrix, tr.viewDef->worldSpace.modelViewMatrix, modelViewMatrix );

	EmitSurfaces( modelMatrix, modelViewMatrix, depthHack, false /* stereoDepthSort */, true /* link as entity */ );
}

// DG: move function declaration here (=> out of EmitFullScreen() method) because it confused clang
// (and possibly other compilers that just didn't complain and silently made it a float variable
// initialized to something, probably 0.0f)
float GetScreenSeparationForGuis();
// DG end

/*
================
idGuiModel::EmitFullScreen

Creates a view that covers the screen and emit the surfaces
================
*/
void idGuiModel::EmitFullScreen()
{
	if( surfaces[0].numIndexes == 0 )
	{
		return;
	}

	SCOPED_PROFILE_EVENT( "Gui::EmitFullScreen" );

	viewDef_t* viewDef = ( viewDef_t* )R_ClearedFrameAlloc( sizeof( *viewDef ), FRAME_ALLOC_VIEW_DEF );
	viewDef->is2Dgui = true;
	tr.GetCroppedViewport( &viewDef->viewport );

	bool stereoEnabled = ( renderSystem->GetStereo3DMode() != STEREO3D_OFF );
	if( stereoEnabled )
	{
		const float screenSeparation = GetScreenSeparationForGuis();

		// this will be negated on the alternate eyes, both rendered each frame
		viewDef->renderView.stereoScreenSeparation = screenSeparation;

		extern idCVar stereoRender_swapEyes;
		viewDef->renderView.viewEyeBuffer = 0;	// render to both buffers
		if( stereoRender_swapEyes.GetBool() )
		{
			viewDef->renderView.stereoScreenSeparation = -screenSeparation;
		}
	}

	viewDef->scissor.x1 = 0;
	viewDef->scissor.y1 = 0;
	viewDef->scissor.x2 = viewDef->viewport.x2 - viewDef->viewport.x1;
	viewDef->scissor.y2 = viewDef->viewport.y2 - viewDef->viewport.y1;

	// RB: IMPORTANT - the projectionMatrix has a few changes to make it work with Vulkan
	viewDef->projectionMatrix[0 * 4 + 0] = 2.0f / renderSystem->GetVirtualWidth();
	viewDef->projectionMatrix[0 * 4 + 1] = 0.0f;
	viewDef->projectionMatrix[0 * 4 + 2] = 0.0f;
	viewDef->projectionMatrix[0 * 4 + 3] = 0.0f;

	viewDef->projectionMatrix[1 * 4 + 0] = 0.0f;
#if defined(USE_VULKAN)
	viewDef->projectionMatrix[1 * 4 + 1] = 2.0f / renderSystem->GetVirtualHeight();
#else
	viewDef->projectionMatrix[1 * 4 + 1] = -2.0f / renderSystem->GetVirtualHeight();
#endif
	viewDef->projectionMatrix[1 * 4 + 2] = 0.0f;
	viewDef->projectionMatrix[1 * 4 + 3] = 0.0f;

	viewDef->projectionMatrix[2 * 4 + 0] = 0.0f;
	viewDef->projectionMatrix[2 * 4 + 1] = 0.0f;
	viewDef->projectionMatrix[2 * 4 + 2] = -1.0f;
	viewDef->projectionMatrix[2 * 4 + 3] = 0.0f;

	viewDef->projectionMatrix[3 * 4 + 0] = -1.0f; // RB: was -2.0f
#if defined(USE_VULKAN)
	viewDef->projectionMatrix[3 * 4 + 1] = -1.0f;
#else
	viewDef->projectionMatrix[3 * 4 + 1] = 1.0f;
#endif
	viewDef->projectionMatrix[3 * 4 + 2] = 0.0f; // RB: was 1.0f
	viewDef->projectionMatrix[3 * 4 + 3] = 1.0f;

	// make a tech5 renderMatrix for faster culling
	idRenderMatrix::Transpose( *( idRenderMatrix* )viewDef->projectionMatrix, viewDef->projectionRenderMatrix );

	viewDef->worldSpace.modelMatrix[0 * 4 + 0] = 1.0f;
	viewDef->worldSpace.modelMatrix[1 * 4 + 1] = 1.0f;
	viewDef->worldSpace.modelMatrix[2 * 4 + 2] = 1.0f;
	viewDef->worldSpace.modelMatrix[3 * 4 + 3] = 1.0f;

	viewDef->worldSpace.modelViewMatrix[0 * 4 + 0] = 1.0f;
	viewDef->worldSpace.modelViewMatrix[1 * 4 + 1] = 1.0f;
	viewDef->worldSpace.modelViewMatrix[2 * 4 + 2] = 1.0f;
	viewDef->worldSpace.modelViewMatrix[3 * 4 + 3] = 1.0f;

	viewDef->maxDrawSurfs = surfaces.Num();
	viewDef->drawSurfs = ( drawSurf_t** )R_FrameAlloc( viewDef->maxDrawSurfs * sizeof( viewDef->drawSurfs[0] ), FRAME_ALLOC_DRAW_SURFACE_POINTER );
	viewDef->numDrawSurfs = 0;

#if 1
	// RB: give renderView the current time to calculate 2D shader effects
	int shaderTime = tr.frameShaderTime * 1000; //Sys_Milliseconds();
	viewDef->renderView.time[0] = shaderTime;
	viewDef->renderView.time[1] = shaderTime;
	// RB end
#endif

	viewDef_t* oldViewDef = tr.viewDef;
	tr.viewDef = viewDef;

	EmitSurfaces( viewDef->worldSpace.modelMatrix, viewDef->worldSpace.modelViewMatrix,
				  false /* depthHack */ , stereoEnabled /* stereoDepthSort */, false /* link as entity */ );

	tr.viewDef = oldViewDef;

	// add the command to draw this view
	R_AddDrawViewCmd( viewDef, true );
}

// RB begin
/*
================
idGuiModel::ImGui_RenderDrawLists
================
*/
void idGuiModel::EmitImGui( ImDrawData* drawData )
{
	// NOTE: this implementation does not support scissor clipping for the indivudal draw commands
	// but it is sufficient for things like com_showFPS

	const float sysWidth = renderSystem->GetWidth();
	const float sysHeight = renderSystem->GetHeight();

	idVec2 scaleToVirtual( ( float )renderSystem->GetVirtualWidth() / sysWidth, ( float )renderSystem->GetVirtualHeight() / sysHeight );

	for( int a = 0; a < drawData->CmdListsCount; a++ )
	{
		const ImDrawList* cmd_list = drawData->CmdLists[a];
		const ImDrawIdx* indexBufferOffset = &cmd_list->IdxBuffer.front();

		int numVerts = cmd_list->VtxBuffer.size();

		for( int b = 0; b < cmd_list->CmdBuffer.size(); b++ )
		{
			const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[b];

			int numIndexes = pcmd->ElemCount;

			// support more than just the imGui Font texture
			const idMaterial* mat = tr.imgGuiMaterial;
			if( pcmd->TextureId && ( mat != ( const idMaterial* )pcmd->TextureId ) )
			{
				mat = ( const idMaterial* )pcmd->TextureId;
			}

			idDrawVert* verts = renderSystem->AllocTris( numVerts, indexBufferOffset, numIndexes, mat, STEREO_DEPTH_TYPE_NONE );
			if( verts == NULL )
			{
				continue;
			}

			if( pcmd->UserCallback )
			{
				pcmd->UserCallback( cmd_list, pcmd );
			}
			else
			{
				for( int j = 0; j < numVerts; j++ )
				{
					const ImDrawVert* imVert = &cmd_list->VtxBuffer[j];

					ALIGNTYPE16 idDrawVert tempVert;

					//tempVert.xyz = idVec3( imVert->pos.x, imVert->pos.y, 0.0f );
					tempVert.xyz.ToVec2() = idVec2( imVert->pos.x, imVert->pos.y ).Scale( scaleToVirtual );
					tempVert.xyz.z = 0.0f;
					tempVert.SetTexCoord( imVert->uv.x, imVert->uv.y );
					tempVert.SetColor( imVert->col );

					WriteDrawVerts16( &verts[j], &tempVert, 1 );
				}
			}

			indexBufferOffset += pcmd->ElemCount;
		}
	}
}
// RB end

/*
=============
AdvanceSurf
=============
*/
void idGuiModel::AdvanceSurf()
{
	guiModelSurface_t	s;

	if( surfaces.Num() )
	{
		s.material = surf->material;
		s.glState = surf->glState;
	}
	else
	{
		s.material = tr.defaultMaterial;
		s.glState = 0;
	}

	// advance indexes so the pointer to each surface will be 16 byte aligned
	numIndexes = ALIGN( numIndexes, 8 );

	s.numIndexes = 0;
	s.firstIndex = numIndexes;

	surfaces.Append( s );
	surf = &surfaces[ surfaces.Num() - 1 ];
}

/*
=============
AllocTris
=============
*/
idDrawVert* idGuiModel::AllocTris( int vertCount, const triIndex_t* tempIndexes, int indexCount, const idMaterial* material, const uint64 glState, const stereoDepthType_t stereoType )
{
	if( material == NULL )
	{
		return NULL;
	}
	if( numIndexes + indexCount > MAX_INDEXES )
	{
		static int warningFrame = 0;
		if( warningFrame != tr.frameCount )
		{
			warningFrame = tr.frameCount;
			idLib::Warning( "idGuiModel::AllocTris: MAX_INDEXES exceeded" );
		}
		return NULL;
	}
	if( numVerts + vertCount > MAX_VERTS )
	{
		static int warningFrame = 0;
		if( warningFrame != tr.frameCount )
		{
			warningFrame = tr.frameCount;
			idLib::Warning( "idGuiModel::AllocTris: MAX_VERTS exceeded" );
		}
		return NULL;
	}

	// break the current surface if we are changing to a new material or we can't
	// fit the data into our allocated block
	if( material != surf->material || glState != surf->glState || stereoType != surf->stereoType )
	{
		if( surf->numIndexes )
		{
			AdvanceSurf();
		}
		surf->material = material;
		surf->glState = glState;
		surf->stereoType = stereoType;
	}

	int startVert = numVerts;
	int startIndex = numIndexes;

	numVerts += vertCount;
	numIndexes += indexCount;

	surf->numIndexes += indexCount;

	if( ( startIndex & 1 ) || ( indexCount & 1 ) )
	{
		// slow for write combined memory!
		// this should be very rare, since quads are always an even index count
		for( int i = 0; i < indexCount; i++ )
		{
			indexPointer[startIndex + i] = startVert + tempIndexes[i];
		}
	}
	else
	{
		for( int i = 0; i < indexCount; i += 2 )
		{
			WriteIndexPair( indexPointer + startIndex + i, startVert + tempIndexes[i], startVert + tempIndexes[i + 1] );
		}
	}

	return vertexPointer + startVert;
}
