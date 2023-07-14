/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2014 Carl Kenner
Copyright (C) 2016-2017 Dustin Land
Copyright (C) 2013-2023 Robert Beckebans
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

#include "precompiled.h"
#pragma hdrstop

#include "framework/Common_local.h"
#include "RenderCommon.h"
#include "Framebuffer.h"

#include "imgui/ImGui_Hooks.h"

#include "RenderPass.h"
#include <sys/DeviceManager.h>
#include <nvrhi/utils.h>

idCVar r_useNewSsaoPass( "r_useNewSSAOPass", "1", CVAR_RENDERER | CVAR_BOOL, "use the new SSAO pass from Donut" );
extern DeviceManager* deviceManager;

idCVar r_drawEyeColor( "r_drawEyeColor", "0", CVAR_RENDERER | CVAR_BOOL, "Draw a colored box, red = left eye, blue = right eye, grey = non-stereo" );
idCVar r_motionBlur( "r_motionBlur", "0", CVAR_RENDERER | CVAR_INTEGER | CVAR_ARCHIVE, "1 - 5, log2 of the number of motion blur samples" );
idCVar r_forceZPassStencilShadows( "r_forceZPassStencilShadows", "0", CVAR_RENDERER | CVAR_BOOL, "force Z-pass rendering for performance testing" );
idCVar r_useStencilShadowPreload( "r_useStencilShadowPreload", "0", CVAR_RENDERER | CVAR_BOOL, "use stencil shadow preload algorithm instead of Z-fail" );
idCVar r_skipShaderPasses( "r_skipShaderPasses", "0", CVAR_RENDERER | CVAR_BOOL, "" );
idCVar r_skipInteractionFastPath( "r_skipInteractionFastPath", "1", CVAR_RENDERER | CVAR_BOOL, "" );
idCVar r_useLightStencilSelect( "r_useLightStencilSelect", "0", CVAR_RENDERER | CVAR_BOOL, "use stencil select pass" );

extern idCVar stereoRender_swapEyes;

// SRS - flag indicating whether we are drawing a 3d view vs. a 2d-only view (e.g. menu or pda)
bool drawView3D;

/*
================
SetVertexParm
================
*/
static ID_INLINE void SetVertexParm( renderParm_t rp, const float* value )
{
	renderProgManager.SetUniformValue( rp, value );
}

/*
================
SetVertexParms
================
*/
static ID_INLINE void SetVertexParms( renderParm_t rp, const float* value, int num )
{
	for( int i = 0; i < num; i++ )
	{
		renderProgManager.SetUniformValue( ( renderParm_t )( rp + i ), value + ( i * 4 ) );
	}
}

/*
================
SetFragmentParm
================
*/
static ID_INLINE void SetFragmentParm( renderParm_t rp, const float* value )
{
	renderProgManager.SetUniformValue( rp, value );
}

/*
================
RB_SetMVP
================
*/
void RB_SetMVP( const idRenderMatrix& mvp )
{
	SetVertexParms( RENDERPARM_MVPMATRIX_X, mvp[0], 4 );
}

/*
================
RB_SetMVPWithStereoOffset
================
*/
static void RB_SetMVPWithStereoOffset( const idRenderMatrix& mvp, const float stereoOffset )
{
	idRenderMatrix offset = mvp;
	offset[0][3] += stereoOffset;

	SetVertexParms( RENDERPARM_MVPMATRIX_X, offset[0], 4 );
}

static const float zero[4] = { 0, 0, 0, 0 };
static const float one[4] = { 1, 1, 1, 1 };
static const float negOne[4] = { -1, -1, -1, -1 };

/*
================
RB_SetVertexColorParms
================
*/
void RB_SetVertexColorParms( stageVertexColor_t svc )
{
	switch( svc )
	{
		case SVC_IGNORE:
			SetVertexParm( RENDERPARM_VERTEXCOLOR_MODULATE, zero );
			SetVertexParm( RENDERPARM_VERTEXCOLOR_ADD, one );
			break;
		case SVC_MODULATE:
			SetVertexParm( RENDERPARM_VERTEXCOLOR_MODULATE, one );
			SetVertexParm( RENDERPARM_VERTEXCOLOR_ADD, zero );
			break;
		case SVC_INVERSE_MODULATE:
			SetVertexParm( RENDERPARM_VERTEXCOLOR_MODULATE, negOne );
			SetVertexParm( RENDERPARM_VERTEXCOLOR_ADD, one );
			break;
	}
}



/*
======================
RB_GetShaderTextureMatrix
======================
*/
void RB_GetShaderTextureMatrix( const float* shaderRegisters, const textureStage_t* texture, float matrix[16] )
{
	matrix[0 * 4 + 0] = shaderRegisters[ texture->matrix[0][0] ];
	matrix[1 * 4 + 0] = shaderRegisters[ texture->matrix[0][1] ];
	matrix[2 * 4 + 0] = 0.0f;
	matrix[3 * 4 + 0] = shaderRegisters[ texture->matrix[0][2] ];

	matrix[0 * 4 + 1] = shaderRegisters[ texture->matrix[1][0] ];
	matrix[1 * 4 + 1] = shaderRegisters[ texture->matrix[1][1] ];
	matrix[2 * 4 + 1] = 0.0f;
	matrix[3 * 4 + 1] = shaderRegisters[ texture->matrix[1][2] ];

	// we attempt to keep scrolls from generating incredibly large texture values, but
	// center rotations and center scales can still generate offsets that need to be > 1
	if( matrix[3 * 4 + 0] < -40.0f || matrix[12] > 40.0f )
	{
		matrix[3 * 4 + 0] -= ( int )matrix[3 * 4 + 0];
	}
	if( matrix[13] < -40.0f || matrix[13] > 40.0f )
	{
		matrix[13] -= ( int )matrix[13];
	}

	matrix[0 * 4 + 2] = 0.0f;
	matrix[1 * 4 + 2] = 0.0f;
	matrix[2 * 4 + 2] = 1.0f;
	matrix[3 * 4 + 2] = 0.0f;

	matrix[0 * 4 + 3] = 0.0f;
	matrix[1 * 4 + 3] = 0.0f;
	matrix[2 * 4 + 3] = 0.0f;
	matrix[3 * 4 + 3] = 1.0f;
}

/*
======================
RB_LoadShaderTextureMatrix
======================
*/
void RB_LoadShaderTextureMatrix( const float* shaderRegisters, const textureStage_t* texture )
{
	float texS[4] = { 1.0f, 0.0f, 0.0f, 0.0f };
	float texT[4] = { 0.0f, 1.0f, 0.0f, 0.0f };

	if( texture->hasMatrix )
	{
		float matrix[16];
		RB_GetShaderTextureMatrix( shaderRegisters, texture, matrix );
		texS[0] = matrix[0 * 4 + 0];
		texS[1] = matrix[1 * 4 + 0];
		texS[2] = matrix[2 * 4 + 0];
		texS[3] = matrix[3 * 4 + 0];

		texT[0] = matrix[0 * 4 + 1];
		texT[1] = matrix[1 * 4 + 1];
		texT[2] = matrix[2 * 4 + 1];
		texT[3] = matrix[3 * 4 + 1];

		//RENDERLOG_PRINTF( "Setting Texture Matrix\n" );
		//renderLog.Indent();
		//RENDERLOG_PRINTF( "Texture Matrix S : %4.3f, %4.3f, %4.3f, %4.3f\n", texS[0], texS[1], texS[2], texS[3] );
		//RENDERLOG_PRINTF( "Texture Matrix T : %4.3f, %4.3f, %4.3f, %4.3f\n", texT[0], texT[1], texT[2], texT[3] );
		//renderLog.Outdent();
	}

	SetVertexParm( RENDERPARM_TEXTUREMATRIX_S, texS );
	SetVertexParm( RENDERPARM_TEXTUREMATRIX_T, texT );
}

/*
=====================
RB_BakeTextureMatrixIntoTexgen
=====================
*/
void RB_BakeTextureMatrixIntoTexgen( idPlane lightProject[3], const float* textureMatrix )
{
	float genMatrix[16];
	float final[16];

	genMatrix[0 * 4 + 0] = lightProject[0][0];
	genMatrix[1 * 4 + 0] = lightProject[0][1];
	genMatrix[2 * 4 + 0] = lightProject[0][2];
	genMatrix[3 * 4 + 0] = lightProject[0][3];

	genMatrix[0 * 4 + 1] = lightProject[1][0];
	genMatrix[1 * 4 + 1] = lightProject[1][1];
	genMatrix[2 * 4 + 1] = lightProject[1][2];
	genMatrix[3 * 4 + 1] = lightProject[1][3];

	genMatrix[0 * 4 + 2] = 0.0f;
	genMatrix[1 * 4 + 2] = 0.0f;
	genMatrix[2 * 4 + 2] = 0.0f;
	genMatrix[3 * 4 + 2] = 0.0f;

	genMatrix[0 * 4 + 3] = lightProject[2][0];
	genMatrix[1 * 4 + 3] = lightProject[2][1];
	genMatrix[2 * 4 + 3] = lightProject[2][2];
	genMatrix[3 * 4 + 3] = lightProject[2][3];

	R_MatrixMultiply( genMatrix, textureMatrix, final );

	lightProject[0][0] = final[0 * 4 + 0];
	lightProject[0][1] = final[1 * 4 + 0];
	lightProject[0][2] = final[2 * 4 + 0];
	lightProject[0][3] = final[3 * 4 + 0];

	lightProject[1][0] = final[0 * 4 + 1];
	lightProject[1][1] = final[1 * 4 + 1];
	lightProject[1][2] = final[2 * 4 + 1];
	lightProject[1][3] = final[3 * 4 + 1];
}

/*
======================
idRenderBackend::BindVariableStageImage

Handles generating a cinematic frame if needed
======================
*/
void idRenderBackend::BindVariableStageImage( const textureStage_t* texture, const float* shaderRegisters, nvrhi::ICommandList* commandList )
{
	if( texture->cinematic )
	{
		cinData_t cin;

		if( r_skipDynamicTextures.GetBool() )
		{
			globalImages->defaultImage->Bind();
			return;
		}

		// offset time by shaderParm[7] (FIXME: make the time offset a parameter of the shader?)
		// We make no attempt to optimize for multiple identical cinematics being in view, or
		// for cinematics going at a lower framerate than the renderer.
		cin = texture->cinematic->ImageForTime( viewDef->renderView.time[0] + idMath::Ftoi( 1000.0f * viewDef->renderView.shaderParms[11] ), commandList );
		if( cin.imageY != NULL )
		{
			GL_SelectTexture( 0 );
			cin.imageY->Bind();

			GL_SelectTexture( 1 );
			cin.imageCr->Bind();

			GL_SelectTexture( 2 );
			cin.imageCb->Bind();

			// DG: imageY is only used for bink videos (with libbinkdec), so the bink shader must be used
			if( viewDef->is2Dgui )
			{
				renderProgManager.BindShader_BinkGUI();
			}
			else
			{
				renderProgManager.BindShader_Bink();
			}
		}
		else if( cin.image != NULL )
		{
			// Carl: A single RGB image works better with the FFMPEG BINK codec.
			GL_SelectTexture( 0 );
			cin.image->Bind();

			// SRS - Reenable shaders so ffmpeg and RoQ decoder cinematics are rendered with correct colour
			if( viewDef->is2Dgui )
			{
				renderProgManager.BindShader_TextureVertexColor_sRGB();
			}
			else
			{
				renderProgManager.BindShader_TextureVertexColor();
			}
		}
		else
		{
			globalImages->blackImage->Bind();

			// because the shaders may have already been set - we need to make sure we are not using a bink shader which would
			// display incorrectly.  We may want to get rid of RB_BindVariableStageImage and inline the code so that the
			// SWF GUI case is handled better, too
			renderProgManager.BindShader_TextureVertexColor();
		}
	}
	else
	{
		if( texture->image != NULL )
		{
			texture->image->Bind();
		}
	}
}

/*
================
idRenderBackend::PrepareStageTexturing
================
*/
void idRenderBackend::PrepareStageTexturing( const shaderStage_t* pStage,  const drawSurf_t* surf )
{
	float useTexGenParm[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	// set the texture matrix if needed
	RB_LoadShaderTextureMatrix( surf->shaderRegisters, &pStage->texture );

	// texgens
	if( pStage->texture.texgen == TG_REFLECT_CUBE )
	{

		// see if there is also a bump map specified
		const shaderStage_t* bumpStage = surf->material->GetBumpStage();
		if( bumpStage != NULL )
		{
			// per-pixel reflection mapping with bump mapping
			GL_SelectTexture( 1 );
			bumpStage->texture.image->Bind();

			GL_SelectTexture( 0 );

			if( surf->jointCache )
			{
				renderProgManager.BindShader_BumpyEnvironmentSkinned();
			}
			else
			{
				renderProgManager.BindShader_BumpyEnvironment();
			}
		}
		else
		{
			if( surf->jointCache )
			{
				renderProgManager.BindShader_EnvironmentSkinned();
			}
			else
			{
				renderProgManager.BindShader_Environment();
			}
		}

	}
	else if( pStage->texture.texgen == TG_SKYBOX_CUBE )
	{
		renderProgManager.BindShader_SkyBox();
	}
	else if( pStage->texture.texgen == TG_WOBBLESKY_CUBE )
	{

		const int* parms = surf->material->GetTexGenRegisters();

		float wobbleDegrees = surf->shaderRegisters[ parms[0] ] * ( idMath::PI / 180.0f );
		float wobbleSpeed = surf->shaderRegisters[ parms[1] ] * ( 2.0f * idMath::PI / 60.0f );
		float rotateSpeed = surf->shaderRegisters[ parms[2] ] * ( 2.0f * idMath::PI / 60.0f );

		idVec3 axis[3];
		{
			// very ad-hoc "wobble" transform
			float s, c;
			idMath::SinCos( wobbleSpeed * viewDef->renderView.time[0] * 0.001f, s, c );

			float ws, wc;
			idMath::SinCos( wobbleDegrees, ws, wc );

			axis[2][0] = ws * c;
			axis[2][1] = ws * s;
			axis[2][2] = wc;

			axis[1][0] = -s * s * ws;
			axis[1][2] = -s * ws * ws;
			axis[1][1] = idMath::Sqrt( idMath::Fabs( 1.0f - ( axis[1][0] * axis[1][0] + axis[1][2] * axis[1][2] ) ) );

			// make the second vector exactly perpendicular to the first
			axis[1] -= ( axis[2] * axis[1] ) * axis[2];
			axis[1].Normalize();

			// construct the third with a cross
			axis[0].Cross( axis[1], axis[2] );
		}

		// add the rotate
		float rs, rc;
		idMath::SinCos( rotateSpeed * viewDef->renderView.time[0] * 0.001f, rs, rc );

		float transform[12];
		transform[0 * 4 + 0] = axis[0][0] * rc + axis[1][0] * rs;
		transform[0 * 4 + 1] = axis[0][1] * rc + axis[1][1] * rs;
		transform[0 * 4 + 2] = axis[0][2] * rc + axis[1][2] * rs;
		transform[0 * 4 + 3] = 0.0f;

		transform[1 * 4 + 0] = axis[1][0] * rc - axis[0][0] * rs;
		transform[1 * 4 + 1] = axis[1][1] * rc - axis[0][1] * rs;
		transform[1 * 4 + 2] = axis[1][2] * rc - axis[0][2] * rs;
		transform[1 * 4 + 3] = 0.0f;

		transform[2 * 4 + 0] = axis[2][0];
		transform[2 * 4 + 1] = axis[2][1];
		transform[2 * 4 + 2] = axis[2][2];
		transform[2 * 4 + 3] = 0.0f;

		SetVertexParms( RENDERPARM_WOBBLESKY_X, transform, 3 );
		renderProgManager.BindShader_WobbleSky();

	}
	else if( ( pStage->texture.texgen == TG_SCREEN ) || ( pStage->texture.texgen == TG_SCREEN2 ) )
	{

		useTexGenParm[0] = 1.0f;
		useTexGenParm[1] = 1.0f;
		useTexGenParm[2] = 1.0f;
		useTexGenParm[3] = 1.0f;

		float mat[16];
		R_MatrixMultiply( surf->space->modelViewMatrix, viewDef->projectionMatrix, mat );

		//RENDERLOG_PRINTF( "TexGen : %s\n", ( pStage->texture.texgen == TG_SCREEN ) ? "TG_SCREEN" : "TG_SCREEN2" );
		//renderLog.Indent();

		float plane[4];
		plane[0] = mat[0 * 4 + 0];
		plane[1] = mat[1 * 4 + 0];
		plane[2] = mat[2 * 4 + 0];
		plane[3] = mat[3 * 4 + 0];
		SetVertexParm( RENDERPARM_TEXGEN_0_S, plane );
		//RENDERLOG_PRINTF( "TEXGEN_S = %4.3f, %4.3f, %4.3f, %4.3f\n",  plane[0], plane[1], plane[2], plane[3] );

		plane[0] = mat[0 * 4 + 1];
		plane[1] = mat[1 * 4 + 1];
		plane[2] = mat[2 * 4 + 1];
		plane[3] = mat[3 * 4 + 1];
		SetVertexParm( RENDERPARM_TEXGEN_0_T, plane );
		//RENDERLOG_PRINTF( "TEXGEN_T = %4.3f, %4.3f, %4.3f, %4.3f\n",  plane[0], plane[1], plane[2], plane[3] );

		plane[0] = mat[0 * 4 + 3];
		plane[1] = mat[1 * 4 + 3];
		plane[2] = mat[2 * 4 + 3];
		plane[3] = mat[3 * 4 + 3];
		SetVertexParm( RENDERPARM_TEXGEN_0_Q, plane );
		//RENDERLOG_PRINTF( "TEXGEN_Q = %4.3f, %4.3f, %4.3f, %4.3f\n",  plane[0], plane[1], plane[2], plane[3] );

		//renderLog.Outdent();

	}
	else if( pStage->texture.texgen == TG_DIFFUSE_CUBE )
	{
		// As far as I can tell, this is never used
		idLib::Warning( "Using Diffuse Cube! Please contact Brian!" );

	}
	else if( pStage->texture.texgen == TG_GLASSWARP )
	{
		// As far as I can tell, this is never used
		idLib::Warning( "Using GlassWarp! Please contact Brian!" );
	}

	SetVertexParm( RENDERPARM_TEXGEN_0_ENABLED, useTexGenParm );
}

/*
================
idRenderBackend::FinishStageTexturing
================
*/
void idRenderBackend::FinishStageTexturing( const shaderStage_t* pStage, const drawSurf_t* surf )
{
	if( pStage->texture.cinematic )
	{
		// unbind the extra bink textures
		GL_SelectTexture( 0 );
	}

	if( pStage->texture.texgen == TG_REFLECT_CUBE )
	{
		// see if there is also a bump map specified
		const shaderStage_t* bumpStage = surf->material->GetBumpStage();
		if( bumpStage != NULL )
		{
			// per-pixel reflection mapping with bump mapping
			GL_SelectTexture( 0 );
		}
		else
		{
			// per-pixel reflection mapping without bump mapping
		}
		renderProgManager.Unbind();
	}
}

// RB: moved this up because we need to call this several times for shadow mapping
void idRenderBackend::ResetViewportAndScissorToDefaultCamera( const viewDef_t* _viewDef )
{
	// set the window clipping
	GL_Viewport( _viewDef->viewport.x1,
				 _viewDef->viewport.y1,
				 _viewDef->viewport.x2 + 1 - _viewDef->viewport.x1,
				 _viewDef->viewport.y2 + 1 - _viewDef->viewport.y1 );

	// the scissor may be smaller than the viewport for subviews
	GL_Scissor( viewDef->viewport.x1 + _viewDef->scissor.x1,
				viewDef->viewport.y1 + _viewDef->scissor.y1,
				_viewDef->scissor.x2 + 1 - _viewDef->scissor.x1,
				_viewDef->scissor.y2 + 1 - _viewDef->scissor.y1 );

	currentScissor = viewDef->scissor;
}
// RB end

/*
=========================================================================================

DEPTH BUFFER RENDERING

=========================================================================================
*/

/*
==================
idRenderBackend::FillDepthBufferGeneric
==================
*/
void idRenderBackend::FillDepthBufferGeneric( const drawSurf_t* const* drawSurfs, int numDrawSurfs )
{
	for( int i = 0; i < numDrawSurfs; i++ )
	{
		const drawSurf_t* drawSurf = drawSurfs[i];
		const idMaterial* shader = drawSurf->material;

		// translucent surfaces don't put anything in the depth buffer and don't
		// test against it, which makes them fail the mirror clip plane operation
		if( shader->Coverage() == MC_TRANSLUCENT )
		{
			continue;
		}

		// get the expressions for conditionals / color / texcoords
		const float* regs = drawSurf->shaderRegisters;

		// if all stages of a material have been conditioned off, don't do anything
		int stage = 0;
		for( ; stage < shader->GetNumStages(); stage++ )
		{
			const shaderStage_t* pStage = shader->GetStage( stage );
			// check the stage enable condition
			if( regs[ pStage->conditionRegister ] != 0 )
			{
				break;
			}
		}

		if( stage == shader->GetNumStages() )
		{
			continue;
		}

		// change the matrix if needed
		if( drawSurf->space != currentSpace )
		{
			RB_SetMVP( drawSurf->space->mvp );

			currentSpace = drawSurf->space;
		}

		uint64 surfGLState = 0;

		// set polygon offset if necessary
		if( shader->TestMaterialFlag( MF_POLYGONOFFSET ) )
		{
			surfGLState |= GLS_POLYGON_OFFSET;
			GL_PolygonOffset( r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat() * shader->GetPolygonOffset() );
		}

		// subviews will just down-modulate the color buffer
		idVec4 color;
		if( shader->GetSort() == SS_SUBVIEW )
		{
			surfGLState |= GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO | GLS_DEPTHFUNC_LESS;
			color[0] = 1.0f;
			color[1] = 1.0f;
			color[2] = 1.0f;
			color[3] = 1.0f;
		}
		else
		{
			// others just draw black
			color[0] = 0.0f;
			color[1] = 0.0f;
			color[2] = 0.0f;
			color[3] = 1.0f;
		}

		renderLog.OpenBlock( shader->GetName(), colorMdGrey );

		bool drawSolid = false;
		if( shader->Coverage() == MC_OPAQUE )
		{
			drawSolid = true;
		}
		else if( shader->Coverage() == MC_PERFORATED )
		{
			// we may have multiple alpha tested stages
			// if the only alpha tested stages are condition register omitted,
			// draw a normal opaque surface
			bool didDraw = false;

			// perforated surfaces may have multiple alpha tested stages
			for( stage = 0; stage < shader->GetNumStages(); stage++ )
			{
				const shaderStage_t* pStage = shader->GetStage( stage );

				if( !pStage->hasAlphaTest )
				{
					continue;
				}

				// check the stage enable condition
				if( regs[ pStage->conditionRegister ] == 0 )
				{
					continue;
				}

				// if we at least tried to draw an alpha tested stage,
				// we won't draw the opaque surface
				didDraw = true;

				// set the alpha modulate
				color[3] = regs[ pStage->color.registers[3] ];

				// skip the entire stage if alpha would be black
				if( color[3] <= 0.0f )
				{
					continue;
				}

				uint64 stageGLState = surfGLState;

				// set privatePolygonOffset if necessary
				if( pStage->privatePolygonOffset )
				{
					GL_PolygonOffset( r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat() * pStage->privatePolygonOffset );
					stageGLState |= GLS_POLYGON_OFFSET;
				}

				GL_Color( color );

				GL_State( stageGLState );
				idVec4 alphaTestValue( regs[ pStage->alphaTestRegister ] );
				SetFragmentParm( RENDERPARM_ALPHA_TEST, alphaTestValue.ToFloatPtr() );

				if( drawSurf->jointCache )
				{
					renderProgManager.BindShader_TextureVertexColorSkinned();
				}
				else
				{
					renderProgManager.BindShader_TextureVertexColor();
				}

				RB_SetVertexColorParms( SVC_IGNORE );

				// bind the texture
				GL_SelectTexture( 0 );
				pStage->texture.image->Bind();

				// set texture matrix and texGens
				PrepareStageTexturing( pStage, drawSurf );

				// must render with less-equal for Z-Cull to work properly
				assert( ( GL_GetCurrentState() & GLS_DEPTHFUNC_BITS ) == GLS_DEPTHFUNC_LESS );

				// draw it
				DrawElementsWithCounters( drawSurf );

				// clean up
				FinishStageTexturing( pStage, drawSurf );

				// unset privatePolygonOffset if necessary
				if( pStage->privatePolygonOffset )
				{
					GL_PolygonOffset( r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat() * shader->GetPolygonOffset() );
				}
			}

			if( !didDraw )
			{
				drawSolid = true;
			}
		}

		// draw the entire surface solid
		if( drawSolid )
		{
			if( shader->GetSort() == SS_SUBVIEW )
			{
				renderProgManager.BindShader_Color();
				GL_Color( color );
				GL_State( surfGLState );
			}
			else
			{
				if( drawSurf->jointCache )
				{
					renderProgManager.BindShader_DepthSkinned();
				}
				else
				{
					renderProgManager.BindShader_Depth();
				}
				GL_State( surfGLState | GLS_ALPHAMASK );
			}

			// must render with less-equal for Z-Cull to work properly
			assert( ( GL_GetCurrentState() & GLS_DEPTHFUNC_BITS ) == GLS_DEPTHFUNC_LESS );

			// draw it
			DrawElementsWithCounters( drawSurf );
		}

		renderLog.CloseBlock();
	}

	SetFragmentParm( RENDERPARM_ALPHA_TEST, vec4_zero.ToFloatPtr() );
}

/*
=====================
idRenderBackend::FillDepthBufferFast

Optimized fast path code.

If there are subview surfaces, they must be guarded in the depth buffer to allow
the mirror / subview to show through underneath the current view rendering.

Surfaces with perforated shaders need the full shader setup done, but should be
drawn after the opaque surfaces.

The bulk of the surfaces should be simple opaque geometry that can be drawn very rapidly.

If there are no subview surfaces, we could clear to black and use fast-Z rendering
on the 360.
=====================
*/
void idRenderBackend::FillDepthBufferFast( drawSurf_t** drawSurfs, int numDrawSurfs )
{
	OPTICK_EVENT( "Render_FillDepthBufferFast" );

	nvrhi::ObjectType commandObject = nvrhi::ObjectTypes::D3D12_GraphicsCommandList;
	if( deviceManager->GetGraphicsAPI() == nvrhi::GraphicsAPI::VULKAN )
	{
		commandObject = nvrhi::ObjectTypes::VK_CommandBuffer;
	}
	OPTICK_GPU_CONTEXT( ( void* ) commandList->getNativeObject( commandObject ) );
	OPTICK_GPU_EVENT( "Render_FillDepthBufferFast" );

	if( numDrawSurfs == 0 )
	{
		return;
	}

	// if we are just doing 2D rendering, no need to fill the depth buffer
	if( viewDef->viewEntitys == NULL )
	{
		return;
	}

	renderLog.OpenMainBlock( MRB_FILL_DEPTH_BUFFER );
	renderLog.OpenBlock( "Render_FillDepthBufferFast", colorBlue );

	// force MVP change on first surface
	currentSpace = NULL;

	// draw all the subview surfaces, which will already be at the start of the sorted list,
	// with the general purpose path
	GL_State( GLS_DEFAULT );

	int	surfNum;
	for( surfNum = 0; surfNum < numDrawSurfs; surfNum++ )
	{
		if( drawSurfs[surfNum]->material->GetSort() != SS_SUBVIEW )
		{
			break;
		}

		FillDepthBufferGeneric( &drawSurfs[surfNum], 1 );
	}

	const drawSurf_t** perforatedSurfaces = ( const drawSurf_t** )_alloca( numDrawSurfs * sizeof( drawSurf_t* ) );
	int numPerforatedSurfaces = 0;

	// draw all the opaque surfaces and build up a list of perforated surfaces that
	// we will defer drawing until all opaque surfaces are done
	GL_State( GLS_DEFAULT );

	// continue checking past the subview surfaces
	for( ; surfNum < numDrawSurfs; surfNum++ )
	{
		const drawSurf_t* surf = drawSurfs[ surfNum ];
		const idMaterial* shader = surf->material;

		// translucent surfaces don't put anything in the depth buffer
		if( shader->Coverage() == MC_TRANSLUCENT )
		{
			continue;
		}
		if( shader->Coverage() == MC_PERFORATED )
		{
			// save for later drawing
			perforatedSurfaces[ numPerforatedSurfaces ] = surf;
			numPerforatedSurfaces++;
			continue;
		}

		// set polygon offset?

		// set mvp matrix
		if( surf->space != currentSpace )
		{
			RB_SetMVP( surf->space->mvp );
			currentSpace = surf->space;
		}

		renderLog.OpenBlock( shader->GetName(), colorMdGrey );

		if( surf->jointCache )
		{
			renderProgManager.BindShader_DepthSkinned();
		}
		else
		{
			renderProgManager.BindShader_Depth();
		}

		// must render with less-equal for Z-Cull to work properly
		assert( ( GL_GetCurrentState() & GLS_DEPTHFUNC_BITS ) == GLS_DEPTHFUNC_LESS );

		// draw it solid
		DrawElementsWithCounters( surf );

		renderLog.CloseBlock();
	}

	// draw all perforated surfaces with the general code path
	if( numPerforatedSurfaces > 0 )
	{
		FillDepthBufferGeneric( perforatedSurfaces, numPerforatedSurfaces );
	}

	renderLog.CloseBlock();
	renderLog.CloseMainBlock();
}

/*
=========================================================================================

GENERAL INTERACTION RENDERING

=========================================================================================
*/

const int INTERACTION_TEXUNIT_BUMP			= 0;
const int INTERACTION_TEXUNIT_SPECULARMIX	= 1;
const int INTERACTION_TEXUNIT_BASECOLOR		= 2;

const int INTERACTION_TEXUNIT_FALLOFF		= 3;	// RB: also _brdfLut
const int INTERACTION_TEXUNIT_PROJECTION	= 4;	// RB: also SSAO render target
const int INTERACTION_TEXUNIT_SHADOWMAPS	= 5;
const int INTERACTION_TEXUNIT_JITTER		= 6;

const int INTERACTION_TEXUNIT_AMBIENT_CUBE1 = 7;
const int INTERACTION_TEXUNIT_SPECULAR_CUBE1 = 8;
const int INTERACTION_TEXUNIT_SPECULAR_CUBE2 = 9;
const int INTERACTION_TEXUNIT_SPECULAR_CUBE3 = 10;

/*
==================
idRenderBackend::SetupInteractionStage
==================
*/
void idRenderBackend::SetupInteractionStage( const shaderStage_t* surfaceStage, const float* surfaceRegs, const float lightColor[4],
		idVec4 matrix[2], float color[4] )
{

	if( surfaceStage->texture.hasMatrix )
	{
		matrix[0][0] = surfaceRegs[surfaceStage->texture.matrix[0][0]];
		matrix[0][1] = surfaceRegs[surfaceStage->texture.matrix[0][1]];
		matrix[0][2] = 0.0f;
		matrix[0][3] = surfaceRegs[surfaceStage->texture.matrix[0][2]];

		matrix[1][0] = surfaceRegs[surfaceStage->texture.matrix[1][0]];
		matrix[1][1] = surfaceRegs[surfaceStage->texture.matrix[1][1]];
		matrix[1][2] = 0.0f;
		matrix[1][3] = surfaceRegs[surfaceStage->texture.matrix[1][2]];

		// we attempt to keep scrolls from generating incredibly large texture values, but
		// center rotations and center scales can still generate offsets that need to be > 1
		if( matrix[0][3] < -40.0f || matrix[0][3] > 40.0f )
		{
			matrix[0][3] -= idMath::Ftoi( matrix[0][3] );
		}
		if( matrix[1][3] < -40.0f || matrix[1][3] > 40.0f )
		{
			matrix[1][3] -= idMath::Ftoi( matrix[1][3] );
		}
	}
	else
	{
		matrix[0][0] = 1.0f;
		matrix[0][1] = 0.0f;
		matrix[0][2] = 0.0f;
		matrix[0][3] = 0.0f;

		matrix[1][0] = 0.0f;
		matrix[1][1] = 1.0f;
		matrix[1][2] = 0.0f;
		matrix[1][3] = 0.0f;
	}

	if( color != NULL )
	{
		for( int i = 0; i < 4; i++ )
		{
			// clamp here, so cards with a greater range don't look different.
			// we could perform overbrighting like we do for lights, but
			// it doesn't currently look worth it.
			color[i] = idMath::ClampFloat( 0.0f, 1.0f, surfaceRegs[surfaceStage->color.registers[i]] ) * lightColor[i];
		}
	}
}

/*
=================
idRenderBackend::DrawSingleInteraction
=================
*/
void idRenderBackend::DrawSingleInteraction( drawInteraction_t* din, bool useFastPath, bool useIBL, bool setInteractionShader )
{
	if( !useFastPath )
	{
		if( din->bumpImage == NULL )
		{
			// stage wasn't actually an interaction
			return;
		}

		if( r_skipDiffuse.GetInteger() == 2 )
		{
			din->diffuseImage = globalImages->whiteImage;
		}
		else if( din->diffuseImage == NULL || r_skipDiffuse.GetInteger() > 0 )
		{
			din->diffuseImage = globalImages->blackDiffuseImage;
		}
		if( din->specularImage == NULL || r_skipSpecular.GetBool() || ( din->vLight && din->vLight->lightShader->IsAmbientLight() ) )
		{
			din->specularImage = globalImages->blackImage;
		}
		if( r_skipBump.GetBool() )
		{
			din->bumpImage = globalImages->flatNormalMap;
		}

		// if we wouldn't draw anything, don't call the Draw function
		const bool diffuseIsBlack = ( din->diffuseImage == globalImages->blackImage )
									|| ( ( din->diffuseColor[0] <= 0 ) && ( din->diffuseColor[1] <= 0 ) && ( din->diffuseColor[2] <= 0 ) );
		const bool specularIsBlack = ( din->specularImage == globalImages->blackImage )
									 || ( ( din->specularColor[0] <= 0 ) && ( din->specularColor[1] <= 0 ) && ( din->specularColor[2] <= 0 ) );
		if( diffuseIsBlack && specularIsBlack )
		{
			return;
		}

		// bump matrix
		SetVertexParm( RENDERPARM_BUMPMATRIX_S, din->bumpMatrix[0].ToFloatPtr() );
		SetVertexParm( RENDERPARM_BUMPMATRIX_T, din->bumpMatrix[1].ToFloatPtr() );

		// diffuse matrix
		SetVertexParm( RENDERPARM_DIFFUSEMATRIX_S, din->diffuseMatrix[0].ToFloatPtr() );
		SetVertexParm( RENDERPARM_DIFFUSEMATRIX_T, din->diffuseMatrix[1].ToFloatPtr() );

		// specular matrix
		SetVertexParm( RENDERPARM_SPECULARMATRIX_S, din->specularMatrix[0].ToFloatPtr() );
		SetVertexParm( RENDERPARM_SPECULARMATRIX_T, din->specularMatrix[1].ToFloatPtr() );

		RB_SetVertexColorParms( din->vertexColor );

		SetFragmentParm( RENDERPARM_DIFFUSEMODIFIER, din->diffuseColor.ToFloatPtr() );
		SetFragmentParm( RENDERPARM_SPECULARMODIFIER, din->specularColor.ToFloatPtr() );
	}

	const textureUsage_t specUsage = din->specularImage->GetUsage();

	// RB begin
	if( useIBL && currentSpace->useLightGrid && r_useLightGrid.GetBool() )
	{
		idVec4 probeMins, probeMaxs, probeCenter;

		probeMins[0] = viewDef->globalProbeBounds[0][0];
		probeMins[1] = viewDef->globalProbeBounds[0][1];
		probeMins[2] = viewDef->globalProbeBounds[0][2];
		probeMins[3] = viewDef->globalProbeBounds.IsCleared() ? 0.0f : 1.0f;

		probeMaxs[0] = viewDef->globalProbeBounds[1][0];
		probeMaxs[1] = viewDef->globalProbeBounds[1][1];
		probeMaxs[2] = viewDef->globalProbeBounds[1][2];
		probeMaxs[3] = 0.0f;

		idVec3 center = viewDef->globalProbeBounds.GetCenter();
		probeCenter.Set( center.x, center.y, center.z, 1.0f );

		SetVertexParm( RENDERPARM_WOBBLESKY_X, probeMins.ToFloatPtr() );
		SetVertexParm( RENDERPARM_WOBBLESKY_Y, probeMaxs.ToFloatPtr() );
		SetVertexParm( RENDERPARM_WOBBLESKY_Z, probeCenter.ToFloatPtr() );

		// use rpGlobalLightOrigin for lightGrid center
		idVec4 lightGridOrigin( currentSpace->lightGridOrigin.x, currentSpace->lightGridOrigin.y, currentSpace->lightGridOrigin.z, 1.0f );
		idVec4 lightGridSize( currentSpace->lightGridSize.x, currentSpace->lightGridSize.y, currentSpace->lightGridSize.z, 1.0f );
		idVec4 lightGridBounds( currentSpace->lightGridBounds[0], currentSpace->lightGridBounds[1], currentSpace->lightGridBounds[2], 1.0f );

		renderProgManager.SetUniformValue( RENDERPARM_GLOBALLIGHTORIGIN, lightGridOrigin.ToFloatPtr() );
		renderProgManager.SetUniformValue( RENDERPARM_JITTERTEXSCALE, lightGridSize.ToFloatPtr() );
		renderProgManager.SetUniformValue( RENDERPARM_JITTERTEXOFFSET, lightGridBounds.ToFloatPtr() );

		// individual probe sizes on the atlas image
		idVec4 probeSize;
		probeSize[0] = currentSpace->lightGridAtlasSingleProbeSize - currentSpace->lightGridAtlasBorderSize;
		probeSize[1] = currentSpace->lightGridAtlasSingleProbeSize;
		probeSize[2] = currentSpace->lightGridAtlasBorderSize;
		probeSize[3] = float( currentSpace->lightGridAtlasSingleProbeSize - currentSpace->lightGridAtlasBorderSize ) / currentSpace->lightGridAtlasSingleProbeSize;
		renderProgManager.SetUniformValue( RENDERPARM_SCREENCORRECTIONFACTOR, probeSize.ToFloatPtr() ); // rpScreenCorrectionFactor

		// specular cubemap blend weights
		renderProgManager.SetUniformValue( RENDERPARM_LOCALLIGHTORIGIN, viewDef->radianceImageBlends.ToFloatPtr() );

		if( specUsage == TD_SPECULAR_PBR_RMAO || specUsage == TD_SPECULAR_PBR_RMAOD )
		{
			// PBR path with roughness, metal and AO
			if( din->surf->jointCache )
			{
				renderProgManager.BindShader_ImageBasedLightGridSkinned_PBR();
			}
			else
			{
				renderProgManager.BindShader_ImageBasedLightGrid_PBR();
			}
		}
		else
		{
			if( din->surf->jointCache )
			{
				renderProgManager.BindShader_ImageBasedLightGridSkinned();
			}
			else
			{
				renderProgManager.BindShader_ImageBasedLightGrid();
			}
		}

		GL_SelectTexture( INTERACTION_TEXUNIT_FALLOFF );
		globalImages->brdfLutImage->Bind();

		GL_SelectTexture( INTERACTION_TEXUNIT_PROJECTION );
		if( !r_useSSAO.GetBool() || ( viewDef->renderView.rdflags & ( RDF_NOAMBIENT | RDF_IRRADIANCE ) ) )
		{
			globalImages->whiteImage->Bind();
		}
		else
		{
			globalImages->ambientOcclusionImage[0]->Bind();
		}

		GL_SelectTexture( INTERACTION_TEXUNIT_AMBIENT_CUBE1 );
		currentSpace->lightGridAtlasImage->Bind();

		idVec2i res = currentSpace->lightGridAtlasImage->GetUploadResolution();
		idVec4 textureSize( res.x, res.y, 1.0f / res.x, 1.0f / res.y );

		renderProgManager.SetUniformValue( RENDERPARM_CASCADEDISTANCES, textureSize.ToFloatPtr() );

		GL_SelectTexture( INTERACTION_TEXUNIT_SPECULAR_CUBE1 );
		viewDef->radianceImages[0]->Bind();

		GL_SelectTexture( INTERACTION_TEXUNIT_SPECULAR_CUBE2 );
		viewDef->radianceImages[1]->Bind();

		GL_SelectTexture( INTERACTION_TEXUNIT_SPECULAR_CUBE3 );
		viewDef->radianceImages[2]->Bind();
	}
	else if( useIBL )
	{
		idVec4 probeMins, probeMaxs, probeCenter;

		probeMins[0] = viewDef->globalProbeBounds[0][0];
		probeMins[1] = viewDef->globalProbeBounds[0][1];
		probeMins[2] = viewDef->globalProbeBounds[0][2];
		probeMins[3] = viewDef->globalProbeBounds.IsCleared() ? 0.0f : 1.0f;

		probeMaxs[0] = viewDef->globalProbeBounds[1][0];
		probeMaxs[1] = viewDef->globalProbeBounds[1][1];
		probeMaxs[2] = viewDef->globalProbeBounds[1][2];
		probeMaxs[3] = 0.0f;

		idVec3 center = viewDef->globalProbeBounds.GetCenter();
		probeCenter.Set( center.x, center.y, center.z, 1.0f );

		SetVertexParm( RENDERPARM_WOBBLESKY_X, probeMins.ToFloatPtr() );
		SetVertexParm( RENDERPARM_WOBBLESKY_Y, probeMaxs.ToFloatPtr() );
		SetVertexParm( RENDERPARM_WOBBLESKY_Z, probeCenter.ToFloatPtr() );

		// specular cubemap blend weights
		renderProgManager.SetUniformValue( RENDERPARM_LOCALLIGHTORIGIN, viewDef->radianceImageBlends.ToFloatPtr() );

		if( specUsage == TD_SPECULAR_PBR_RMAO || specUsage == TD_SPECULAR_PBR_RMAOD )
		{
			// PBR path with roughness, metal and AO
			if( din->surf->jointCache )
			{
				renderProgManager.BindShader_ImageBasedLightingSkinned_PBR();
			}
			else
			{
				renderProgManager.BindShader_ImageBasedLighting_PBR();
			}
		}
		else
		{
			if( din->surf->jointCache )
			{
				renderProgManager.BindShader_ImageBasedLightingSkinned();
			}
			else
			{
				renderProgManager.BindShader_ImageBasedLighting();
			}
		}

		GL_SelectTexture( INTERACTION_TEXUNIT_FALLOFF );
		globalImages->brdfLutImage->Bind();

		GL_SelectTexture( INTERACTION_TEXUNIT_PROJECTION );
		if( !r_useSSAO.GetBool() || ( viewDef->renderView.rdflags & ( RDF_NOAMBIENT | RDF_IRRADIANCE ) ) )
		{
			globalImages->whiteImage->Bind();
		}
		else
		{
			globalImages->ambientOcclusionImage[0]->Bind();
		}

		GL_SelectTexture( INTERACTION_TEXUNIT_AMBIENT_CUBE1 );
		viewDef->irradianceImage->Bind();

		GL_SelectTexture( INTERACTION_TEXUNIT_SPECULAR_CUBE1 );
		viewDef->radianceImages[0]->Bind();

		GL_SelectTexture( INTERACTION_TEXUNIT_SPECULAR_CUBE2 );
		viewDef->radianceImages[1]->Bind();

		GL_SelectTexture( INTERACTION_TEXUNIT_SPECULAR_CUBE3 );
		viewDef->radianceImages[2]->Bind();
	}
	else if( setInteractionShader )
	{
		// TODO extra paths for foliage, terrain and skin

		if( specUsage == TD_SPECULAR_PBR_RMAO || specUsage == TD_SPECULAR_PBR_RMAOD )
		{
			// PBR path with roughness, metal and AO
			// select the render prog

			if( din->vLight->lightShader->IsAmbientLight() )
			{
				if( din->surf->jointCache )
				{
					renderProgManager.BindShader_PBR_InteractionAmbientSkinned();
				}
				else
				{
					renderProgManager.BindShader_PBR_InteractionAmbient();
				}
			}
			else
			{
				if( !r_skipShadows.GetBool() && din->vLight->globalShadows && din->vLight->shadowLOD > -1 )
				{
					// RB: we have shadow mapping enabled and shadow maps so do a shadow compare

					if( r_useShadowAtlas.GetBool() )
					{
						if( din->vLight->ImageAtlasPlaced() )
						{
							if( din->vLight->parallel )
							{
								if( din->surf->jointCache )
								{
									renderProgManager.BindShader_PBR_Interaction_ShadowAtlas_Parallel_Skinned();
								}
								else
								{
									renderProgManager.BindShader_PBR_Interaction_ShadowAtlas_Parallel();
								}
							}
							else if( din->vLight->pointLight )
							{
								if( din->surf->jointCache )
								{
									renderProgManager.BindShader_PBR_Interaction_ShadowAtlas_Point_Skinned();
								}
								else
								{
									renderProgManager.BindShader_PBR_Interaction_ShadowAtlas_Point();
								}
							}
							else
							{
								if( din->surf->jointCache )
								{
									renderProgManager.BindShader_PBR_Interaction_ShadowAtlas_Spot_Skinned();
								}
								else
								{
									renderProgManager.BindShader_PBR_Interaction_ShadowAtlas_Spot();
								}
							}
						}
						else
						{
							// no shadowmap allocation possible
							if( din->surf->jointCache )
							{
								renderProgManager.BindShader_PBR_InteractionSkinned();
							}
							else
							{
								renderProgManager.BindShader_PBR_Interaction();
							}
						}
					}
					else
					{
						// regular shadow mapping

						if( din->vLight->parallel )
						{
							if( din->surf->jointCache )
							{
								renderProgManager.BindShader_PBR_Interaction_ShadowMapping_Parallel_Skinned();
							}
							else
							{
								renderProgManager.BindShader_PBR_Interaction_ShadowMapping_Parallel();
							}
						}
						else if( din->vLight->pointLight )
						{
							if( din->surf->jointCache )
							{
								renderProgManager.BindShader_PBR_Interaction_ShadowMapping_Point_Skinned();
							}
							else
							{
								renderProgManager.BindShader_PBR_Interaction_ShadowMapping_Point();
							}
						}
						else
						{
							if( din->surf->jointCache )
							{
								renderProgManager.BindShader_PBR_Interaction_ShadowMapping_Spot_Skinned();
							}
							else
							{
								renderProgManager.BindShader_PBR_Interaction_ShadowMapping_Spot();
							}
						}
					}
				}
				else
				{
					if( din->surf->jointCache )
					{
						renderProgManager.BindShader_PBR_InteractionSkinned();
					}
					else
					{
						renderProgManager.BindShader_PBR_Interaction();
					}
				}
			}
		}
		else
		{
			// only oldschool D3 gloss maps provided

			if( din->vLight->lightShader->IsAmbientLight() )
			{
				if( din->surf->jointCache )
				{
					renderProgManager.BindShader_InteractionAmbientSkinned();
				}
				else
				{
					renderProgManager.BindShader_InteractionAmbient();
				}
			}
			else
			{
				if( !r_skipShadows.GetBool() && din->vLight->globalShadows )
				{
					// RB: we have shadow mapping enabled and shadow maps so do a shadow compare

					if( r_useShadowAtlas.GetBool() )
					{
						if( din->vLight->ImageAtlasPlaced() )
						{
							if( din->vLight->parallel )
							{
								if( din->surf->jointCache )
								{
									renderProgManager.BindShader_Interaction_ShadowAtlas_Parallel_Skinned();
								}
								else
								{
									renderProgManager.BindShader_Interaction_ShadowAtlas_Parallel();
								}
							}
							else if( din->vLight->pointLight )
							{
								if( din->surf->jointCache )
								{
									renderProgManager.BindShader_Interaction_ShadowAtlas_Point_Skinned();
								}
								else
								{
									renderProgManager.BindShader_Interaction_ShadowAtlas_Point();
								}
							}
							else
							{
								if( din->surf->jointCache )
								{
									renderProgManager.BindShader_Interaction_ShadowAtlas_Spot_Skinned();
								}
								else
								{
									renderProgManager.BindShader_Interaction_ShadowAtlas_Spot();
								}
							}
						}
						else
						{
							// no shadowmap allocation possible
							if( din->surf->jointCache )
							{
								renderProgManager.BindShader_InteractionSkinned();
							}
							else
							{
								renderProgManager.BindShader_Interaction();
							}
						}
					}
					else
					{
						// regular shadow mapping

						if( din->vLight->parallel )
						{
							if( din->surf->jointCache )
							{
								renderProgManager.BindShader_Interaction_ShadowMapping_Parallel_Skinned();
							}
							else
							{
								renderProgManager.BindShader_Interaction_ShadowMapping_Parallel();
							}
						}
						else if( din->vLight->pointLight )
						{
							if( din->surf->jointCache )
							{
								renderProgManager.BindShader_Interaction_ShadowMapping_Point_Skinned();
							}
							else
							{
								renderProgManager.BindShader_Interaction_ShadowMapping_Point();
							}
						}
						else
						{
							if( din->surf->jointCache )
							{
								renderProgManager.BindShader_Interaction_ShadowMapping_Spot_Skinned();
							}
							else
							{
								renderProgManager.BindShader_Interaction_ShadowMapping_Spot();
							}
						}
					}
				}
				else
				{
					if( din->surf->jointCache )
					{
						renderProgManager.BindShader_InteractionSkinned();
					}
					else
					{
						renderProgManager.BindShader_Interaction();
					}
				}
			}
		}
	}
	// RB end

	// texture 0 will be the per-surface bump map
	GL_SelectTexture( INTERACTION_TEXUNIT_BUMP );
	din->bumpImage->Bind();

	// texture 1 is the per-surface specular map
	GL_SelectTexture( INTERACTION_TEXUNIT_SPECULARMIX );
	din->specularImage->Bind();

	// texture 2 is the per-surface diffuse map
	GL_SelectTexture( INTERACTION_TEXUNIT_BASECOLOR );
	din->diffuseImage->Bind();

	DrawElementsWithCounters( din->surf );
}

/*
=================
RB_SetupForFastPathInteractions

These are common for all fast path surfaces
=================
*/
static void RB_SetupForFastPathInteractions( const idVec4& diffuseColor, const idVec4& specularColor )
{
	const idVec4 sMatrix( 1, 0, 0, 0 );
	const idVec4 tMatrix( 0, 1, 0, 0 );

	// bump matrix
	SetVertexParm( RENDERPARM_BUMPMATRIX_S, sMatrix.ToFloatPtr() );
	SetVertexParm( RENDERPARM_BUMPMATRIX_T, tMatrix.ToFloatPtr() );

	// diffuse matrix
	SetVertexParm( RENDERPARM_DIFFUSEMATRIX_S, sMatrix.ToFloatPtr() );
	SetVertexParm( RENDERPARM_DIFFUSEMATRIX_T, tMatrix.ToFloatPtr() );

	// specular matrix
	SetVertexParm( RENDERPARM_SPECULARMATRIX_S, sMatrix.ToFloatPtr() );
	SetVertexParm( RENDERPARM_SPECULARMATRIX_T, tMatrix.ToFloatPtr() );

	RB_SetVertexColorParms( SVC_IGNORE );

	SetFragmentParm( RENDERPARM_DIFFUSEMODIFIER, diffuseColor.ToFloatPtr() );
	SetFragmentParm( RENDERPARM_SPECULARMODIFIER, specularColor.ToFloatPtr() );
}

/*
=============
idRenderBackend::RenderInteractions

With added sorting and trivial path work.
=============
*/
void idRenderBackend::RenderInteractions( const drawSurf_t* surfList, const viewLight_t* vLight, int depthFunc, bool performStencilTest, bool useLightDepthBounds )
{
	if( surfList == NULL )
	{
		return;
	}

	// change the scissor if needed, it will be constant across all the surfaces lit by the light
	if( !currentScissor.Equals( vLight->scissorRect ) && r_useScissor.GetBool() )
	{
		GL_Scissor( viewDef->viewport.x1 + vLight->scissorRect.x1,
					viewDef->viewport.y1 + vLight->scissorRect.y1,
					vLight->scissorRect.x2 + 1 - vLight->scissorRect.x1,
					vLight->scissorRect.y2 + 1 - vLight->scissorRect.y1 );

		currentScissor = vLight->scissorRect;
	}

	// perform setup here that will be constant for all interactions
	if( performStencilTest )
	{
		GL_State(
			GLS_SRCBLEND_ONE |
			GLS_DSTBLEND_ONE |
			GLS_DEPTHMASK |
			depthFunc |
			GLS_STENCIL_FUNC_EQUAL |
			GLS_STENCIL_MAKE_REF( STENCIL_SHADOW_TEST_VALUE ) |
			GLS_STENCIL_MAKE_MASK( STENCIL_SHADOW_MASK_VALUE ) );

	}
	else
	{
		GL_State(
			GLS_SRCBLEND_ONE |
			GLS_DSTBLEND_ONE |
			GLS_DEPTHMASK |
			depthFunc |
			GLS_STENCIL_FUNC_ALWAYS );
	}

	// some rare lights have multiple animating stages, loop over them outside the surface list
	const idMaterial* lightShader = vLight->lightShader;
	const float* lightRegs = vLight->shaderRegisters;

	drawInteraction_t inter = {};
	inter.vLight = vLight;

	//---------------------------------
	// Split out the complex surfaces from the fast-path surfaces
	// so we can do the fast path ones all in a row.
	// The surfaces should already be sorted by space because they
	// are added single-threaded, and there is only a negligable amount
	// of benefit to trying to sort by materials.
	//---------------------------------
	static const int MAX_INTERACTIONS_PER_LIGHT = 1024;
	static const int MAX_COMPLEX_INTERACTIONS_PER_LIGHT = 256;
	idStaticList< const drawSurf_t*, MAX_INTERACTIONS_PER_LIGHT > allSurfaces;
	idStaticList< const drawSurf_t*, MAX_COMPLEX_INTERACTIONS_PER_LIGHT > complexSurfaces;
	for( const drawSurf_t* walk = surfList; walk != NULL; walk = walk->nextOnLight )
	{
		const idMaterial* surfaceShader = walk->material;
		if( surfaceShader->GetFastPathBumpImage() )
		{
			allSurfaces.Append( walk );
		}
		else
		{
			complexSurfaces.Append( walk );
		}
	}
	for( int i = 0; i < complexSurfaces.Num(); i++ )
	{
		allSurfaces.Append( complexSurfaces[i] );
	}

	bool lightDepthBoundsDisabled = false;

	// RB begin
	if( !r_skipShadows.GetBool() && vLight->shadowLOD > -1 )
	{
		const static int JITTER_SIZE = 128;

		// default high quality
		float jitterSampleScale = 1.0f;
		float shadowMapSamples = r_shadowMapSamples.GetInteger();

		float jitterTexOffset[4];
		jitterTexOffset[0] = 1.0f / globalImages->blueNoiseImage256->GetUploadWidth();
		jitterTexOffset[1] = 1.0f / globalImages->blueNoiseImage256->GetUploadWidth();

		if( r_shadowMapRandomizeJitter.GetBool() )
		{
			jitterTexOffset[2] = Sys_Milliseconds() / 1000.0f;
			jitterTexOffset[3] = tr.frameCount % 64;
		}
		else
		{
			jitterTexOffset[2] = 0.0f;
			jitterTexOffset[3] = 0.0f;
		}
		SetFragmentParm( RENDERPARM_JITTERTEXOFFSET, jitterTexOffset ); // rpJitterTexOffset

		if( vLight->parallel )
		{
			float cascadeDistances[4];
			cascadeDistances[0] = viewDef->frustumSplitDistances[0];
			cascadeDistances[1] = viewDef->frustumSplitDistances[1];
			cascadeDistances[2] = viewDef->frustumSplitDistances[2];
			cascadeDistances[3] = viewDef->frustumSplitDistances[3];
			SetFragmentParm( RENDERPARM_CASCADEDISTANCES, cascadeDistances ); // rpCascadeDistances
		}

		if( r_useShadowAtlas.GetBool() && vLight->ImageAtlasPlaced() )
		{
			// screen power of two correction factor
			float screenCorrectionParm[4];
			screenCorrectionParm[0] = 1.0f / ( JITTER_SIZE * shadowMapSamples ) ;
			screenCorrectionParm[1] = 1.0f / JITTER_SIZE;
			screenCorrectionParm[2] = 1.0f / r_shadowMapAtlasSize.GetInteger(); // atlas sample scale
			screenCorrectionParm[3] = vLight->parallel ? r_shadowMapSunDepthBiasScale.GetFloat() : r_shadowMapRegularDepthBiasScale.GetFloat();
			SetFragmentParm( RENDERPARM_SCREENCORRECTIONFACTOR, screenCorrectionParm ); // rpScreenCorrectionFactor

			float jitterTexScale[4];
			jitterTexScale[0] = r_shadowMapJitterScale.GetFloat() * jitterSampleScale;	// TODO shadow buffer size fraction shadowMapSize / maxShadowMapSize
			jitterTexScale[1] = vLight->imageSize.x / float( r_shadowMapAtlasSize.GetInteger() );
			jitterTexScale[2] = vLight->shadowFadeOut;
			jitterTexScale[3] = shadowMapSamples;
			SetFragmentParm( RENDERPARM_JITTERTEXSCALE, jitterTexScale ); // rpJitterTexScale

			// float4
			idVec4 shadowOffsets[6];

			for( int i = 0; i < 6; i++ )
			{
				shadowOffsets[ i ].x = vLight->imageAtlasOffset[ i ].x * ( 1.0f / r_shadowMapAtlasSize.GetInteger() );
				shadowOffsets[ i ].y = vLight->imageAtlasOffset[ i ].y * ( 1.0f / r_shadowMapAtlasSize.GetInteger() );
			}

			SetVertexParms( RENDERPARM_SHADOW_ATLAS_OFFSET_0, &shadowOffsets[0][0], 6 );
		}
		else
		{
			// screen power of two correction factor
			float screenCorrectionParm[4];
			screenCorrectionParm[0] = 1.0f / ( JITTER_SIZE * shadowMapSamples ) ;
			screenCorrectionParm[1] = 1.0f / JITTER_SIZE;
			screenCorrectionParm[2] = 1.0f / shadowMapResolutions[vLight->shadowLOD];
			screenCorrectionParm[3] = vLight->parallel ? r_shadowMapSunDepthBiasScale.GetFloat() : r_shadowMapRegularDepthBiasScale.GetFloat();
			SetFragmentParm( RENDERPARM_SCREENCORRECTIONFACTOR, screenCorrectionParm ); // rpScreenCorrectionFactor

			float jitterTexScale[4];
			jitterTexScale[0] = r_shadowMapJitterScale.GetFloat() * jitterSampleScale;	// TODO shadow buffer size fraction shadowMapSize / maxShadowMapSize
			jitterTexScale[1] = r_shadowMapJitterScale.GetFloat() * jitterSampleScale;
			jitterTexScale[2] = vLight->shadowFadeOut;
			jitterTexScale[3] = shadowMapSamples;
			SetFragmentParm( RENDERPARM_JITTERTEXSCALE, jitterTexScale ); // rpJitterTexScale
		}

	}
	// RB end

	const float lightScale = r_lightScale.GetFloat();

	for( int lightStageNum = 0; lightStageNum < lightShader->GetNumStages(); lightStageNum++ )
	{
		const shaderStage_t*	lightStage = lightShader->GetStage( lightStageNum );

		// ignore stages that fail the condition
		if( !lightRegs[ lightStage->conditionRegister ] )
		{
			continue;
		}

		const idVec4 lightColor(
			lightScale * lightRegs[ lightStage->color.registers[0] ],
			lightScale * lightRegs[ lightStage->color.registers[1] ],
			lightScale * lightRegs[ lightStage->color.registers[2] ],
			lightRegs[ lightStage->color.registers[3] ] );

		// apply the world-global overbright and the 2x factor for specular
		idVec4 diffuseColor = lightColor;
// jmarshall
		idVec4 specularColor = lightColor * 2.0f;

		if( vLight->lightDef->parms.noSpecular )
		{
			specularColor.Zero();
		}
// jmarshall end

		float lightTextureMatrix[16];
		if( lightStage->texture.hasMatrix )
		{
			RB_GetShaderTextureMatrix( lightRegs, &lightStage->texture, lightTextureMatrix );
		}

		// texture 1 will be the light falloff texture
		GL_SelectTexture( INTERACTION_TEXUNIT_FALLOFF );
		vLight->falloffImage->Bind();

		// texture 2 will be the light projection texture
		GL_SelectTexture( INTERACTION_TEXUNIT_PROJECTION );
		lightStage->texture.image->Bind();

		// texture 5 will be the shadow maps array
		GL_SelectTexture( INTERACTION_TEXUNIT_SHADOWMAPS );
		if( r_useShadowAtlas.GetBool() )
		{
			globalImages->shadowAtlasImage->Bind();
		}
		else
		{
			globalImages->shadowImage[vLight->shadowLOD]->Bind();
		}

		// texture 6 will be the jitter texture for soft shadowing
		GL_SelectTexture( INTERACTION_TEXUNIT_JITTER );
		globalImages->blueNoiseImage256->Bind();

		// force the light textures to not use anisotropic filtering, which is wasted on them
		// all of the texture sampler parms should be constant for all interactions, only
		// the actual texture image bindings will change

		//----------------------------------
		// For all surfaces on this light list, generate an interaction for this light stage
		//----------------------------------

		// setup renderparms assuming we will be drawing trivial surfaces first
		RB_SetupForFastPathInteractions( diffuseColor, specularColor );

		// even if the space does not change between light stages, each light stage may need a different lightTextureMatrix baked in
		currentSpace = NULL;

		for( int sortedSurfNum = 0; sortedSurfNum < allSurfaces.Num(); sortedSurfNum++ )
		{
			const drawSurf_t* const surf = allSurfaces[ sortedSurfNum ];

			const idMaterial* surfaceShader = surf->material;
			const float* surfaceRegs = surf->shaderRegisters;

			inter.surf = surf;

			// change the MVP matrix, view/light origin and light projection vectors if needed
			if( surf->space != currentSpace )
			{
				currentSpace = surf->space;

				// turn off the light depth bounds test if this model is rendered with a depth hack
				if( useLightDepthBounds )
				{
					if( !surf->space->weaponDepthHack && surf->space->modelDepthHack == 0.0f )
					{
						if( lightDepthBoundsDisabled )
						{
							GL_DepthBoundsTest( vLight->scissorRect.zmin, vLight->scissorRect.zmax );
							lightDepthBoundsDisabled = false;
						}
					}
					else
					{
						if( !lightDepthBoundsDisabled )
						{
							GL_DepthBoundsTest( 0.0f, 0.0f );
							lightDepthBoundsDisabled = true;
						}
					}
				}

				// model-view-projection
				RB_SetMVP( surf->space->mvp );

				// RB begin
				idRenderMatrix modelMatrix;
				idRenderMatrix::Transpose( *( idRenderMatrix* )surf->space->modelMatrix, modelMatrix );

				SetVertexParms( RENDERPARM_MODELMATRIX_X, modelMatrix[0], 4 );

				// for determining the shadow mapping cascades
				idRenderMatrix modelViewMatrix, tmp;
				idRenderMatrix::Transpose( *( idRenderMatrix* )surf->space->modelViewMatrix, modelViewMatrix );
				SetVertexParms( RENDERPARM_MODELVIEWMATRIX_X, modelViewMatrix[0], 4 );

				idVec4 globalLightOrigin( vLight->globalLightOrigin.x, vLight->globalLightOrigin.y, vLight->globalLightOrigin.z, 1.0f );
				SetVertexParm( RENDERPARM_GLOBALLIGHTORIGIN, globalLightOrigin.ToFloatPtr() );
				// RB end

				// tranform the light/view origin into model local space
				idVec4 localLightOrigin( 0.0f );
				idVec4 localViewOrigin( 1.0f );
				R_GlobalPointToLocal( surf->space->modelMatrix, vLight->globalLightOrigin, localLightOrigin.ToVec3() );
				R_GlobalPointToLocal( surf->space->modelMatrix, viewDef->renderView.vieworg, localViewOrigin.ToVec3() );

				// set the local light/view origin
				SetVertexParm( RENDERPARM_LOCALLIGHTORIGIN, localLightOrigin.ToFloatPtr() );
				SetVertexParm( RENDERPARM_LOCALVIEWORIGIN, localViewOrigin.ToFloatPtr() );

				// transform the light project into model local space
				idPlane lightProjection[4];
				for( int i = 0; i < 4; i++ )
				{
					R_GlobalPlaneToLocal( surf->space->modelMatrix, vLight->lightProject[i], lightProjection[i] );
				}

				// optionally multiply the local light projection by the light texture matrix
				if( lightStage->texture.hasMatrix )
				{
					RB_BakeTextureMatrixIntoTexgen( lightProjection, lightTextureMatrix );
				}

				// set the light projection
				SetVertexParm( RENDERPARM_LIGHTPROJECTION_S, lightProjection[0].ToFloatPtr() );
				SetVertexParm( RENDERPARM_LIGHTPROJECTION_T, lightProjection[1].ToFloatPtr() );
				SetVertexParm( RENDERPARM_LIGHTPROJECTION_Q, lightProjection[2].ToFloatPtr() );
				SetVertexParm( RENDERPARM_LIGHTFALLOFF_S, lightProjection[3].ToFloatPtr() );

				// RB begin
				if( !r_skipShadows.GetBool() && vLight->ImageAtlasPlaced() )
				{
					if( vLight->parallel )
					{
						for( int i = 0; i < ( r_shadowMapSplits.GetInteger() + 1 ); i++ )
						{
							idRenderMatrix modelToShadowMatrix;
							idRenderMatrix::Multiply( vLight->shadowV[i], modelMatrix, modelToShadowMatrix );

							idRenderMatrix shadowClipMVP;
							idRenderMatrix::Multiply( vLight->shadowP[i], modelToShadowMatrix, shadowClipMVP );

							idRenderMatrix shadowWindowMVP;
							idRenderMatrix::Multiply( renderMatrix_clipSpaceToWindowSpace, shadowClipMVP, shadowWindowMVP );

							SetVertexParms( ( renderParm_t )( RENDERPARM_SHADOW_MATRIX_0_X + i * 4 ), shadowWindowMVP[0], 4 );
						}
					}
					else if( vLight->pointLight )
					{
						for( int i = 0; i < 6; i++ )
						{
							idRenderMatrix modelToShadowMatrix;
							idRenderMatrix::Multiply( vLight->shadowV[i], modelMatrix, modelToShadowMatrix );

							idRenderMatrix shadowClipMVP;
							idRenderMatrix::Multiply( vLight->shadowP[i], modelToShadowMatrix, shadowClipMVP );

							idRenderMatrix shadowWindowMVP;
							idRenderMatrix::Multiply( renderMatrix_clipSpaceToWindowSpace, shadowClipMVP, shadowWindowMVP );

							SetVertexParms( ( renderParm_t )( RENDERPARM_SHADOW_MATRIX_0_X + i * 4 ), shadowWindowMVP[0], 4 );
						}
					}
					else
					{
						// spot light

						idRenderMatrix modelToShadowMatrix;
						idRenderMatrix::Multiply( vLight->shadowV[0], modelMatrix, modelToShadowMatrix );

						idRenderMatrix shadowClipMVP;
						idRenderMatrix::Multiply( vLight->shadowP[0], modelToShadowMatrix, shadowClipMVP );

						SetVertexParms( ( renderParm_t )( RENDERPARM_SHADOW_MATRIX_0_X ), shadowClipMVP[0], 4 );
					}
				}
				// RB end
			}

			// check for the fast path
			if( surfaceShader->GetFastPathBumpImage() && !r_skipInteractionFastPath.GetBool() )
			{
				renderLog.OpenBlock( surf->material->GetName(), colorMdGrey );

				inter.bumpImage = surfaceShader->GetFastPathBumpImage();
				inter.specularImage = surfaceShader->GetFastPathSpecularImage();
				inter.diffuseImage = surfaceShader->GetFastPathDiffuseImage();

				DrawSingleInteraction( &inter, true, false, true );

				renderLog.CloseBlock();
				continue;
			}

			renderLog.OpenBlock( surf->material->GetName(), colorMdGrey );

			inter.bumpImage = NULL;
			inter.specularImage = NULL;
			inter.diffuseImage = NULL;
			inter.diffuseColor[0] = inter.diffuseColor[1] = inter.diffuseColor[2] = inter.diffuseColor[3] = 0;
			inter.specularColor[0] = inter.specularColor[1] = inter.specularColor[2] = inter.specularColor[3] = 0;

			// go through the individual surface stages
			//
			// This is somewhat arcane because of the old support for video cards that had to render
			// interactions in multiple passes.
			//
			// We also have the very rare case of some materials that have conditional interactions
			// for the "hell writing" that can be shined on them.
			for( int surfaceStageNum = 0; surfaceStageNum < surfaceShader->GetNumStages(); surfaceStageNum++ )
			{
				const shaderStage_t*	surfaceStage = surfaceShader->GetStage( surfaceStageNum );

				switch( surfaceStage->lighting )
				{
					case SL_COVERAGE:
					{
						// ignore any coverage stages since they should only be used for the depth fill pass
						// for diffuse stages that use alpha test.
						break;
					}
					case SL_AMBIENT:
					{
						// ignore ambient stages while drawing interactions
						break;
					}
					case SL_BUMP:
					{
						// ignore stage that fails the condition
						if( !surfaceRegs[ surfaceStage->conditionRegister ] )
						{
							break;
						}
						// draw any previous interaction
						if( inter.bumpImage != NULL )
						{
							DrawSingleInteraction( &inter, false, false, true );
						}
						inter.bumpImage = surfaceStage->texture.image;
						inter.diffuseImage = NULL;
						inter.specularImage = NULL;
						SetupInteractionStage( surfaceStage, surfaceRegs, NULL, inter.bumpMatrix, NULL );
						break;
					}
					case SL_DIFFUSE:
					{
						// ignore stage that fails the condition
						if( !surfaceRegs[ surfaceStage->conditionRegister ] )
						{
							break;
						}
						// draw any previous interaction
						if( inter.diffuseImage != NULL )
						{
							DrawSingleInteraction( &inter, false, false, true );
						}
						inter.diffuseImage = surfaceStage->texture.image;
						inter.vertexColor = surfaceStage->vertexColor;
						SetupInteractionStage( surfaceStage, surfaceRegs, diffuseColor.ToFloatPtr(),
											   inter.diffuseMatrix, inter.diffuseColor.ToFloatPtr() );
						break;
					}
					case SL_SPECULAR:
					{
						// ignore stage that fails the condition
						if( !surfaceRegs[ surfaceStage->conditionRegister ] || vLight->lightDef->parms.noSpecular ) // SRS - From RB forums
						{
							break;
						}
						// draw any previous interaction
						if( inter.specularImage != NULL )
						{
							DrawSingleInteraction( &inter, false, false, true );
						}
						inter.specularImage = surfaceStage->texture.image;
						inter.vertexColor = surfaceStage->vertexColor;
						SetupInteractionStage( surfaceStage, surfaceRegs, specularColor.ToFloatPtr(),
											   inter.specularMatrix, inter.specularColor.ToFloatPtr() );
						break;
					}
				}
			}

			// draw the final interaction
			DrawSingleInteraction( &inter, false, false, true );

			renderLog.CloseBlock();
		}
	}

	if( useLightDepthBounds && lightDepthBoundsDisabled )
	{
		GL_DepthBoundsTest( vLight->scissorRect.zmin, vLight->scissorRect.zmax );
	}

	renderProgManager.Unbind();
}

// RB begin

/*
=========================================================================================

AMBIENT PASS RENDERING

=========================================================================================
*/

/*
==================
idRenderBackend::AmbientPass
==================
*/
void idRenderBackend::AmbientPass( const drawSurf_t* const* drawSurfs, int numDrawSurfs, bool fillGbuffer )
{
	Framebuffer* previousFramebuffer = Framebuffer::GetActiveFramebuffer();

	if( numDrawSurfs == 0 )
	{
		return;
	}

	if( !drawSurfs )
	{
		return;
	}

	// if we are just doing 2D rendering, no need to fill the depth buffer
	if( viewDef->viewEntitys == NULL )
	{
		return;
	}

	if( viewDef->renderView.rdflags & RDF_NOAMBIENT )
	{
		return;
	}

	renderLog.OpenMainBlock( fillGbuffer ? MRB_FILL_GEOMETRY_BUFFER : MRB_AMBIENT_PASS );
	renderLog.OpenBlock( fillGbuffer ? "Fill_GeometryBuffer" : "Render_AmbientPass", colorBlue );

	// make sure rpWindowCoord is set even without post processing surfaces in the view
	int x = viewDef->viewport.x1;
	int y = viewDef->viewport.y1;
	int	w = viewDef->viewport.x2 - viewDef->viewport.x1 + 1;
	int	h = viewDef->viewport.y2 - viewDef->viewport.y1 + 1;

	// window coord to 0.0 to 1.0 conversion
	float windowCoordParm[4];
	windowCoordParm[0] = 1.0f / w;
	windowCoordParm[1] = 1.0f / h;
	windowCoordParm[2] = w;
	windowCoordParm[3] = h;
	renderProgManager.SetUniformValue( RENDERPARM_WINDOWCOORD, windowCoordParm ); // rpWindowCoord

	if( fillGbuffer )
	{
		commandList->clearTextureFloat( globalImages->gbufferNormalsRoughnessImage->GetTextureHandle(), nvrhi::AllSubresources, nvrhi::Color( 0.f ) );
	}

	// RB: TODO remove this
	if( !fillGbuffer && r_useSSAO.GetBool() && r_ssaoDebug.GetBool() )
	{
		GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO | GLS_DEPTHMASK | GLS_DEPTHFUNC_ALWAYS );

		// We just want to do a quad pass - so make sure we disable any texgen and
		// set the texture matrix to the identity so we don't get anomalies from
		// any stale uniform data being present from a previous draw call
		const float texS[4] = { 1.0f, 0.0f, 0.0f, 0.0f };
		const float texT[4] = { 0.0f, 1.0f, 0.0f, 0.0f };
		renderProgManager.SetRenderParm( RENDERPARM_TEXTUREMATRIX_S, texS );
		renderProgManager.SetRenderParm( RENDERPARM_TEXTUREMATRIX_T, texT );

		// disable any texgen
		const float texGenEnabled[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		renderProgManager.SetRenderParm( RENDERPARM_TEXGEN_0_ENABLED, texGenEnabled );

		currentSpace = NULL;
		auto mvp = renderMatrix_identity;
		mvp[1][1] = -mvp[1][1]; // flip y
		RB_SetMVP( mvp );

		renderProgManager.BindShader_Texture();
		GL_Color( 1, 1, 1, 1 );

		GL_SelectTexture( 0 );
		globalImages->ambientOcclusionImage[0]->Bind();

		//commonPasses.BlitTexture( commandList, (nvrhi::IFramebuffer*)globalFramebuffers.hdrFBO->GetApiObject(), globalImages->ambientOcclusionImage[0]->GetTextureHandle().Get(), &bindingCache );

		DrawElementsWithCounters( &unitSquareSurface );

		renderProgManager.Unbind();
		GL_State( GLS_DEFAULT );

		SetFragmentParm( RENDERPARM_ALPHA_TEST, vec4_zero.ToFloatPtr() );

		renderLog.CloseBlock();
		renderLog.CloseMainBlock();
		return;
	}

	if( fillGbuffer )
	{
		globalFramebuffers.geometryBufferFBO->Bind();

		GL_Clear( true, false, false, 0, 0.0f, 0.0f, 0.0f, 1.0f, false );
	}

	// RB: not needed
	// GL_StartDepthPass( backEnd.viewDef->scissor );

	// force MVP change on first surface
	currentSpace = NULL;

	// draw all the subview surfaces, which will already be at the start of the sorted list,
	// with the general purpose path
	GL_State( GLS_DEFAULT );

#define BLEND_NORMALS 1

	// RB: even use additive blending to blend the normals
	//GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHMASK | GLS_DEPTHFUNC_EQUAL );

	GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO | GLS_DEPTHMASK | GLS_DEPTHFUNC_EQUAL );

	GL_Color( colorWhite );

	idVec4 diffuseColor;
	idVec4 specularColor;
	idVec4 ambientColor;

	if( viewDef->renderView.rdflags & RDF_IRRADIANCE )
	{
		// RB: don't let artist run into a trap when baking multibounce lightgrids

		// use default value of r_lightScale 3
		const float lightScale = 3;
		const idVec4 lightColor = colorWhite * lightScale;

		// apply the world-global overbright and the 2x factor for specular
		diffuseColor = lightColor;
		specularColor = lightColor;// * 2.0f;

		// loose 5% with every bounce like in DDGI
		const float energyConservation = 0.95f;

		//ambientColor.Set( energyConservation, energyConservation, energyConservation, 1.0f );
		float a = r_forceAmbient.GetFloat();

		ambientColor.Set( a, a, a, 1 );
	}
	else
	{
		const float lightScale = r_lightScale.GetFloat();
		const idVec4 lightColor = colorWhite * lightScale;

		// apply the world-global overbright and tune down specular a bit so we have less fresnel overglow
		diffuseColor = lightColor;
		specularColor = lightColor;// * 0.5f;

		float ambientBoost = 1.0f;
		if( !r_usePBR.GetBool() )
		{
			ambientBoost += r_useSSAO.GetBool() ? 0.2f : 0.0f;
			ambientBoost *= 1.1f;
		}

		ambientColor.x = r_forceAmbient.GetFloat() * ambientBoost;
		ambientColor.y = r_forceAmbient.GetFloat() * ambientBoost;
		ambientColor.z = r_forceAmbient.GetFloat() * ambientBoost;
		ambientColor.w = 1;
	}

	renderProgManager.SetRenderParm( RENDERPARM_AMBIENT_COLOR, ambientColor.ToFloatPtr() );

	bool useIBL = r_usePBR.GetBool() && !fillGbuffer;

	// setup renderparms assuming we will be drawing trivial surfaces first
	RB_SetupForFastPathInteractions( diffuseColor, specularColor );

	for( int i = 0; i < numDrawSurfs; i++ )
	{
		const drawSurf_t* drawSurf = drawSurfs[i];
		const idMaterial* surfaceMaterial = drawSurf->material;

		// translucent surfaces don't put anything in the depth buffer and don't
		// test against it, which makes them fail the mirror clip plane operation
		if( surfaceMaterial->Coverage() == MC_TRANSLUCENT )
		{
			continue;
		}

		// get the expressions for conditionals / color / texcoords
		const float* surfaceRegs = drawSurf->shaderRegisters;

		// if all stages of a material have been conditioned off, don't do anything
		int stage = 0;
		for( ; stage < surfaceMaterial->GetNumStages(); stage++ )
		{
			const shaderStage_t* pStage = surfaceMaterial->GetStage( stage );
			// check the stage enable condition
			if( surfaceRegs[ pStage->conditionRegister ] != 0 )
			{
				break;
			}
		}
		if( stage == surfaceMaterial->GetNumStages() )
		{
			continue;
		}

		//bool isWorldModel = ( drawSurf->space->entityDef->parms.origin == vec3_origin );

		//if( isWorldModel )
		//{
		//	renderProgManager.BindShader_VertexLighting();
		//}
		//else
		{
			if( fillGbuffer )
			{
				// TODO support PBR textures and store roughness in the alpha channel

				// fill geometry buffer with normal/roughness information
				if( drawSurf->jointCache )
				{
					renderProgManager.BindShader_SmallGeometryBufferSkinned();
				}
				else
				{
					renderProgManager.BindShader_SmallGeometryBuffer();
				}
			}
		}

		// change the matrix if needed
		if( drawSurf->space != currentSpace )
		{
			currentSpace = drawSurf->space;

			RB_SetMVP( drawSurf->space->mvp );

			// tranform the view origin into model local space
			idVec4 localViewOrigin( 1.0f );
			R_GlobalPointToLocal( drawSurf->space->modelMatrix, viewDef->renderView.vieworg, localViewOrigin.ToVec3() );
			SetVertexParm( RENDERPARM_LOCALVIEWORIGIN, localViewOrigin.ToFloatPtr() );

			// RB: if we want to store the normals in world space so we need the model -> world matrix
			idRenderMatrix modelMatrix;
			idRenderMatrix::Transpose( *( idRenderMatrix* )drawSurf->space->modelMatrix, modelMatrix );

			SetVertexParms( RENDERPARM_MODELMATRIX_X, modelMatrix[0], 4 );

			// RB: if we want to store the normals in camera space so we need the model -> camera matrix
			float modelViewMatrixTranspose[16];
			R_MatrixTranspose( drawSurf->space->modelViewMatrix, modelViewMatrixTranspose );
			SetVertexParms( RENDERPARM_MODELVIEWMATRIX_X, modelViewMatrixTranspose, 4 );
		}

		/*
		uint64 surfGLState = 0;

		// set polygon offset if necessary
		if( surfaceMaterial->TestMaterialFlag( MF_POLYGONOFFSET ) )
		{
			surfGLState |= GLS_POLYGON_OFFSET;
			GL_PolygonOffset( r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat() * surfaceMaterial->GetPolygonOffset() );
		}

		// subviews will just down-modulate the color buffer
		idVec4 color;
		if( surfaceMaterial->GetSort() == SS_SUBVIEW )
		{
			surfGLState |= GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO | GLS_DEPTHFUNC_LESS;
			color[0] = 1.0f;
			color[1] = 1.0f;
			color[2] = 1.0f;
			color[3] = 1.0f;
		}
		else
		{
			// others just draw black
		#if 0
			color[0] = 0.0f;
			color[1] = 0.0f;
			color[2] = 0.0f;
			color[3] = 1.0f;
		#else
			color = colorWhite;
		#endif
		}
		*/

		drawInteraction_t inter = {};
		inter.surf = drawSurf;

		inter.diffuseColor[0] = inter.diffuseColor[1] = inter.diffuseColor[2] = inter.diffuseColor[3] = 1;
		inter.specularColor[0] = inter.specularColor[1] = inter.specularColor[2] = inter.specularColor[3] = 0;

		// check for the fast path
		if( surfaceMaterial->GetFastPathBumpImage() && !r_skipInteractionFastPath.GetBool() )
		{
			renderLog.OpenBlock( surfaceMaterial->GetName(), colorMdGrey );

			inter.bumpImage = surfaceMaterial->GetFastPathBumpImage();
			inter.specularImage = surfaceMaterial->GetFastPathSpecularImage();
			inter.diffuseImage = surfaceMaterial->GetFastPathDiffuseImage();

			DrawSingleInteraction( &inter, true, useIBL, false );

			renderLog.CloseBlock();
			continue;
		}

		renderLog.OpenBlock( surfaceMaterial->GetName(), colorMdGrey );

		//bool drawSolid = false;

		inter.bumpImage = NULL;
		inter.specularImage = NULL;
		inter.diffuseImage = NULL;

		// we may have multiple alpha tested stages
		// if the only alpha tested stages are condition register omitted,
		// draw a normal opaque surface
		bool didDraw = false;

		// perforated surfaces may have multiple alpha tested stages
		for( stage = 0; stage < surfaceMaterial->GetNumStages(); stage++ )
		{
			const shaderStage_t* surfaceStage = surfaceMaterial->GetStage( stage );

			switch( surfaceStage->lighting )
			{
				case SL_COVERAGE:
				{
					// ignore any coverage stages since they should only be used for the depth fill pass
					// for diffuse stages that use alpha test.
					break;
				}

				case SL_AMBIENT:
				{
					// ignore ambient stages while drawing interactions
					break;
				}

				case SL_BUMP:
				{
					// ignore stage that fails the condition
					if( !surfaceRegs[ surfaceStage->conditionRegister ] )
					{
						break;
					}
					// draw any previous interaction
					if( inter.bumpImage != NULL )
					{
#if BLEND_NORMALS
						if( inter.vertexColor == SVC_IGNORE )
						{
							GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO | GLS_DEPTHMASK | GLS_DEPTHFUNC_EQUAL );
						}
						else
						{
							// RB: this is a bit hacky: use additive blending to blend the normals
							GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHMASK | GLS_DEPTHFUNC_EQUAL );
						}
#endif

						DrawSingleInteraction( &inter, false, useIBL, false );
					}
					inter.bumpImage = surfaceStage->texture.image;
					inter.diffuseImage = NULL;
					inter.specularImage = NULL;
					SetupInteractionStage( surfaceStage, surfaceRegs, NULL,
										   inter.bumpMatrix, NULL );
					break;
				}

				case SL_DIFFUSE:
				{
					// ignore stage that fails the condition
					if( !surfaceRegs[ surfaceStage->conditionRegister ] )
					{
						break;
					}

					// draw any previous interaction
					if( inter.diffuseImage != NULL )
					{
#if BLEND_NORMALS
						if( inter.vertexColor == SVC_IGNORE )
						{
							GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO | GLS_DEPTHMASK | GLS_DEPTHFUNC_EQUAL );
						}
						else
						{
							// RB: this is a bit hacky: use additive blending to blend the normals
							GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHMASK | GLS_DEPTHFUNC_EQUAL );
						}
#endif

						DrawSingleInteraction( &inter, false, useIBL, false );
					}

					inter.diffuseImage = surfaceStage->texture.image;
					inter.vertexColor = surfaceStage->vertexColor;
					SetupInteractionStage( surfaceStage, surfaceRegs, diffuseColor.ToFloatPtr(),
										   inter.diffuseMatrix, inter.diffuseColor.ToFloatPtr() );
					break;
				}

				case SL_SPECULAR:
				{
					// ignore stage that fails the condition
					if( !surfaceRegs[ surfaceStage->conditionRegister ] )
					{
						break;
					}
					// draw any previous interaction
					if( inter.specularImage != NULL )
					{
#if BLEND_NORMALS
						if( inter.vertexColor == SVC_IGNORE )
						{
							GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO | GLS_DEPTHMASK | GLS_DEPTHFUNC_EQUAL );
						}
						else
						{
							// RB: this is a bit hacky: use additive blending to blend the normals
							GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHMASK | GLS_DEPTHFUNC_EQUAL );
						}
#endif

						DrawSingleInteraction( &inter, false, useIBL, false );
					}
					inter.specularImage = surfaceStage->texture.image;
					inter.vertexColor = surfaceStage->vertexColor;
					SetupInteractionStage( surfaceStage, surfaceRegs, specularColor.ToFloatPtr(),
										   inter.specularMatrix, inter.specularColor.ToFloatPtr() );
					break;
				}
			}
		}

		// draw the final interaction
#if BLEND_NORMALS
		if( inter.vertexColor == SVC_IGNORE )
		{
			GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO | GLS_DEPTHMASK | GLS_DEPTHFUNC_EQUAL );
		}
		else
		{
			// RB: this is a bit hacky: use additive blending to blend the normals
			GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHMASK | GLS_DEPTHFUNC_EQUAL );
		}
#endif

		DrawSingleInteraction( &inter, false, useIBL, false );

		renderLog.CloseBlock();
	}

	// disable blending
	GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO | GLS_DEPTHMASK | GLS_DEPTHFUNC_EQUAL );
	SetFragmentParm( RENDERPARM_ALPHA_TEST, vec4_zero.ToFloatPtr() );

	GL_SelectTexture( 0 );

	if( fillGbuffer )
	{
		// go back to main render target
		if( previousFramebuffer != NULL )
		{
			previousFramebuffer->Bind();
		}
		else
		{
			Framebuffer::Unbind();
		}
	}

	renderProgManager.Unbind();

	renderLog.CloseBlock();
	renderLog.CloseMainBlock();
}

// RB end

/*
==============================================================================================

SHADOW MAPS RENDERING

==============================================================================================
*/

/*
same as D3DXMatrixOrthoOffCenterRH

http://msdn.microsoft.com/en-us/library/bb205348(VS.85).aspx
*/
static void MatrixOrthogonalProjectionRH( float m[16], float left, float right, float bottom, float top, float zNear, float zFar )
{
	m[0] = 2 / ( right - left );
	m[4] = 0;
	m[8] = 0;
	m[12] = ( left + right ) / ( left - right );
	m[1] = 0;
	m[5] = 2 / ( top - bottom );
	m[9] = 0;
	m[13] = ( top + bottom ) / ( bottom - top );
	m[2] = 0;
	m[6] = 0;
	m[10] = 1 / ( zNear - zFar );
	m[14] = zNear / ( zNear - zFar );
	m[3] = 0;
	m[7] = 0;
	m[11] = 0;
	m[15] = 1;
}

void MatrixCrop( float m[16], const idVec3 mins, const idVec3 maxs )
{
	float			scaleX, scaleY, scaleZ;
	float			offsetX, offsetY, offsetZ;

	scaleX = 2.0f / ( maxs[0] - mins[0] );
	scaleY = 2.0f / ( maxs[1] - mins[1] );

	offsetX = -0.5f * ( maxs[0] + mins[0] ) * scaleX;
	offsetY = -0.5f * ( maxs[1] + mins[1] ) * scaleY;

	scaleZ = 1.0f / ( maxs[2] - mins[2] );
	offsetZ = -mins[2] * scaleZ;

	m[ 0] = scaleX;
	m[ 4] = 0;
	m[ 8] = 0;
	m[12] = offsetX;
	m[ 1] = 0;
	m[ 5] = scaleY;
	m[ 9] = 0;
	m[13] = offsetY;
	m[ 2] = 0;
	m[ 6] = 0;
	m[10] = scaleZ;
	m[14] = offsetZ;
	m[ 3] = 0;
	m[ 7] = 0;
	m[11] = 0;
	m[15] = 1;
}

void MatrixLookAtRH( float m[16], const idVec3& eye, const idVec3& dir, const idVec3& up )
{
	idVec3 dirN;
	idVec3 upN;
	idVec3 sideN;

	sideN = dir.Cross( up );
	sideN.Normalize();

	upN = sideN.Cross( dir );
	upN.Normalize();

	dirN = dir;
	dirN.Normalize();

	m[ 0] = sideN[0];
	m[ 4] = sideN[1];
	m[ 8] = sideN[2];
	m[12] = -( sideN * eye );
	m[ 1] = upN[0];
	m[ 5] = upN[1];
	m[ 9] = upN[2];
	m[13] = -( upN * eye );
	m[ 2] = -dirN[0];
	m[ 6] = -dirN[1];
	m[10] = -dirN[2];
	m[14] = ( dirN * eye );
	m[ 3] = 0;
	m[ 7] = 0;
	m[11] = 0;
	m[15] = 1;
}


/*
=====================
idRenderBackend::SetupShadowMapMatrices
=====================
*/
void idRenderBackend::SetupShadowMapMatrices( viewLight_t* vLight, int side, idRenderMatrix& lightProjectionRenderMatrix, idRenderMatrix& lightViewRenderMatrix )
{
	if( vLight->parallel && side >= 0 )
	{
		assert( side >= 0 && side < 6 );

		// original light direction is from surface to light origin
		idVec3 lightDir = -vLight->lightCenter;
		if( lightDir.Normalize() == 0.0f )
		{
			lightDir[2] = -1.0f;
		}

		idMat3 rotation = lightDir.ToMat3();
		//idAngles angles = lightDir.ToAngles();
		//idMat3 rotation = angles.ToMat3();

		const idVec3 viewDir = viewDef->renderView.viewaxis[0];
		const idVec3 viewPos = viewDef->renderView.vieworg;

#if 1
		idRenderMatrix::CreateViewMatrix( viewDef->renderView.vieworg, rotation, lightViewRenderMatrix );
#else
		float lightViewMatrix[16];
		MatrixLookAtRH( lightViewMatrix, viewPos, lightDir, viewDir );
		idRenderMatrix::Transpose( *( idRenderMatrix* )lightViewMatrix, lightViewRenderMatrix );
#endif

		idBounds lightBounds;
		lightBounds.Clear();

		ALIGNTYPE16 frustumCorners_t corners;
		idRenderMatrix::GetFrustumCorners( corners, vLight->inverseBaseLightProject, bounds_zeroOneCube );

		idVec4 point, transf;
		for( int j = 0; j < 8; j++ )
		{
			point[0] = corners.x[j];
			point[1] = corners.y[j];
			point[2] = corners.z[j];
			point[3] = 1;

			lightViewRenderMatrix.TransformPoint( point, transf );
			transf[0] /= transf[3];
			transf[1] /= transf[3];
			transf[2] /= transf[3];

			lightBounds.AddPoint( transf.ToVec3() );
		}

		float lightProjectionMatrix[16];
		MatrixOrthogonalProjectionRH( lightProjectionMatrix, lightBounds[0][0], lightBounds[1][0], lightBounds[0][1], lightBounds[1][1], -lightBounds[1][2], -lightBounds[0][2] );
		idRenderMatrix::Transpose( *( idRenderMatrix* )lightProjectionMatrix, lightProjectionRenderMatrix );


		// 	'frustumMVP' goes from global space -> camera local space -> camera projective space
		// invert the MVP projection so we can deform zero-to-one cubes into the frustum pyramid shape and calculate global bounds

		idRenderMatrix splitFrustumInverse;
		if( !idRenderMatrix::Inverse( viewDef->frustumMVPs[FRUSTUM_CASCADE1 + side], splitFrustumInverse ) )
		{
			idLib::Warning( "splitFrustumMVP invert failed" );
		}

		// splitFrustumCorners in global space
		ALIGNTYPE16 frustumCorners_t splitFrustumCorners;
		idRenderMatrix::GetFrustumCorners( splitFrustumCorners, splitFrustumInverse, bounds_unitCube );

#if 0
		idBounds splitFrustumBounds;
		splitFrustumBounds.Clear();
		for( int j = 0; j < 8; j++ )
		{
			point[0] = splitFrustumCorners.x[j];
			point[1] = splitFrustumCorners.y[j];
			point[2] = splitFrustumCorners.z[j];

			splitFrustumBounds.AddPoint( point.ToVec3() );
		}

		idVec3 center = splitFrustumBounds.GetCenter();
		float radius = splitFrustumBounds.GetRadius( center );

		//ALIGNTYPE16 frustumCorners_t splitFrustumCorners;
		splitFrustumBounds[0] = idVec3( -radius, -radius, -radius );
		splitFrustumBounds[1] = idVec3( radius, radius, radius );
		splitFrustumBounds.TranslateSelf( viewPos );
		idVec3 splitFrustumCorners2[8];
		splitFrustumBounds.ToPoints( splitFrustumCorners2 );

		for( int j = 0; j < 8; j++ )
		{
			splitFrustumCorners.x[j] = splitFrustumCorners2[j].x;
			splitFrustumCorners.y[j] = splitFrustumCorners2[j].y;
			splitFrustumCorners.z[j] = splitFrustumCorners2[j].z;
		}
#endif


		idRenderMatrix lightViewProjectionRenderMatrix;
		idRenderMatrix::Multiply( lightProjectionRenderMatrix, lightViewRenderMatrix, lightViewProjectionRenderMatrix );

		// find the bounding box of the current split in the light's clip space
		idBounds cropBounds;
		cropBounds.Clear();
		for( int j = 0; j < 8; j++ )
		{
			point[0] = splitFrustumCorners.x[j];
			point[1] = splitFrustumCorners.y[j];
			point[2] = splitFrustumCorners.z[j];
			point[3] = 1;

			lightViewRenderMatrix.TransformPoint( point, transf );
			transf[0] /= transf[3];
			transf[1] /= transf[3];
			transf[2] /= transf[3];

			cropBounds.AddPoint( transf.ToVec3() );
		}

		// don't let the frustum AABB be bigger than the light AABB
		if( cropBounds[0][0] < lightBounds[0][0] )
		{
			cropBounds[0][0] = lightBounds[0][0];
		}

		if( cropBounds[0][1] < lightBounds[0][1] )
		{
			cropBounds[0][1] = lightBounds[0][1];
		}

		if( cropBounds[1][0] > lightBounds[1][0] )
		{
			cropBounds[1][0] = lightBounds[1][0];
		}

		if( cropBounds[1][1] > lightBounds[1][1] )
		{
			cropBounds[1][1] = lightBounds[1][1];
		}

		cropBounds[0][2] = lightBounds[0][2];
		cropBounds[1][2] = lightBounds[1][2];

		//float cropMatrix[16];
		//MatrixCrop(cropMatrix, cropBounds[0], cropBounds[1]);

		//idRenderMatrix cropRenderMatrix;
		//idRenderMatrix::Transpose( *( idRenderMatrix* )cropMatrix, cropRenderMatrix );

		//idRenderMatrix tmp = lightProjectionRenderMatrix;
		//idRenderMatrix::Multiply( cropRenderMatrix, tmp, lightProjectionRenderMatrix );

		MatrixOrthogonalProjectionRH( lightProjectionMatrix, cropBounds[0][0], cropBounds[1][0], cropBounds[0][1], cropBounds[1][1], -cropBounds[1][2], -cropBounds[0][2] );
		idRenderMatrix::Transpose( *( idRenderMatrix* )lightProjectionMatrix, lightProjectionRenderMatrix );

		vLight->shadowV[side] = lightViewRenderMatrix;
		vLight->shadowP[side] = lightProjectionRenderMatrix;

#if defined( USE_NVRHI )
		ALIGNTYPE16 const idRenderMatrix matClipToUvzw(
			1.0f,  0.0f, 0.0f,  0.0f,
			0.0f, -1.0f, 0.0f,  0.0f,
			0.0f,  0.0f, 2.0f, -1.0f,
			0.0f,  0.0f, 0.0f,  1.0f
		);

		idRenderMatrix::Multiply( matClipToUvzw, lightProjectionRenderMatrix, vLight->shadowP[side] );
#endif
	}
	else if( vLight->pointLight && side >= 0 )
	{
		assert( side >= 0 && side < 6 );

		float	viewMatrix[16] = {0};

		idVec3	vec;
		idVec3	origin = vLight->globalLightOrigin;

		// side of a point light
		switch( side )
		{
			case 0:
				viewMatrix[0] = 1;
				viewMatrix[9] = 1;
				viewMatrix[6] = -1;
				break;
			case 1:
				viewMatrix[0] = -1;
				viewMatrix[9] = -1;
				viewMatrix[6] = -1;
				break;
			case 2:
				viewMatrix[4] = 1;
				viewMatrix[1] = -1;
				viewMatrix[10] = 1;
				break;
			case 3:
				viewMatrix[4] = -1;
				viewMatrix[1] = -1;
				viewMatrix[10] = -1;
				break;
			case 4:
				viewMatrix[8] = 1;
				viewMatrix[1] = -1;
				viewMatrix[6] = -1;
				break;
			case 5:
				viewMatrix[8] = -1;
				viewMatrix[1] = 1;
				viewMatrix[6] = -1;
				break;
		}

		viewMatrix[12] = -origin[0] * viewMatrix[0] + -origin[1] * viewMatrix[4] + -origin[2] * viewMatrix[8];
		viewMatrix[13] = -origin[0] * viewMatrix[1] + -origin[1] * viewMatrix[5] + -origin[2] * viewMatrix[9];
		viewMatrix[14] = -origin[0] * viewMatrix[2] + -origin[1] * viewMatrix[6] + -origin[2] * viewMatrix[10];

		viewMatrix[3] = 0;
		viewMatrix[7] = 0;
		viewMatrix[11] = 0;
		viewMatrix[15] = 1;

		// from world space to light origin, looking down the X axis
		float	unflippedLightViewMatrix[16];

		// from world space to OpenGL view space, looking down the negative Z axis
		float	lightViewMatrix[16];

		static float	s_flipMatrix[16] =
		{
			// convert from our coordinate system (looking down X)
			// to OpenGL's coordinate system (looking down -Z)
			0, 0, -1, 0,
			-1, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 0, 1
		};

		memcpy( unflippedLightViewMatrix, viewMatrix, sizeof( unflippedLightViewMatrix ) );
		R_MatrixMultiply( viewMatrix, s_flipMatrix, lightViewMatrix );

		idRenderMatrix::Transpose( *( idRenderMatrix* )lightViewMatrix, lightViewRenderMatrix );

		// set up 90 degree projection matrix
		const float zNear = 4;
		const float	fov = r_shadowMapFrustumFOV.GetFloat();

		float ymax = zNear * tan( fov * idMath::PI / 360.0f );
		float ymin = -ymax;

		float xmax = zNear * tan( fov * idMath::PI / 360.0f );
		float xmin = -xmax;

		const float width = xmax - xmin;
		const float height = ymax - ymin;

		// from OpenGL view space to OpenGL NDC ( -1 : 1 in XYZ )
		float lightProjectionMatrix[16];

		lightProjectionMatrix[0 * 4 + 0] = -2.0f * zNear / width;
		lightProjectionMatrix[1 * 4 + 0] = 0.0f;
		lightProjectionMatrix[2 * 4 + 0] = ( xmax + xmin ) / width;	// normally 0
		lightProjectionMatrix[3 * 4 + 0] = 0.0f;

		lightProjectionMatrix[0 * 4 + 1] = 0.0f;
		lightProjectionMatrix[1 * 4 + 1] = 2.0f * zNear / height;
		lightProjectionMatrix[2 * 4 + 1] = ( ymax + ymin ) / height;	// normally 0
		lightProjectionMatrix[3 * 4 + 1] = 0.0f;

		// this is the far-plane-at-infinity formulation, and
		// crunches the Z range slightly so w=0 vertexes do not
		// rasterize right at the wraparound point
		lightProjectionMatrix[0 * 4 + 2] = 0.0f;
		lightProjectionMatrix[1 * 4 + 2] = 0.0f;
		lightProjectionMatrix[2 * 4 + 2] = -0.999f; // adjust value to prevent imprecision issues

#if 0 //defined( USE_NVRHI )
		// the D3D clip space Z is in range [0,1] instead of [-1,1]
		lightProjectionMatrix[3 * 4 + 2] = -zNear;
#else
		lightProjectionMatrix[3 * 4 + 2] = -2.0f * zNear;
#endif

		lightProjectionMatrix[0 * 4 + 3] = 0.0f;
		lightProjectionMatrix[1 * 4 + 3] = 0.0f;
		lightProjectionMatrix[2 * 4 + 3] = -1.0f;
		lightProjectionMatrix[3 * 4 + 3] = 0.0f;

		idRenderMatrix::Transpose( *( idRenderMatrix* )lightProjectionMatrix, lightProjectionRenderMatrix );

		vLight->shadowV[side] = lightViewRenderMatrix;
		vLight->shadowP[side] = lightProjectionRenderMatrix;

#if defined( USE_NVRHI )
		ALIGNTYPE16 const idRenderMatrix matClipToUvzw(
			1.0f,  0.0f, 0.0f,  0.0f,
			0.0f, -1.0f, 0.0f,  0.0f,
			0.0f,  0.0f, 2.0f, -1.0f,
			0.0f,  0.0f, 0.0f,  1.0f
		);

		idRenderMatrix::Multiply( matClipToUvzw, lightProjectionRenderMatrix, vLight->shadowP[side] );
#endif
	}
	else
	{
		// spot light
		lightViewRenderMatrix.Identity();
		lightProjectionRenderMatrix = vLight->baseLightProject;

		vLight->shadowV[0] = lightViewRenderMatrix;
		vLight->shadowP[0] = lightProjectionRenderMatrix;

#if defined( USE_NVRHI )
		// Calculate alternate matrix that maps to [0, 1] UV space instead of [-1, 1] clip space
		ALIGNTYPE16 const idRenderMatrix matClipToUvzw(
			0.5f,  0.0f, 0.0f, 0.5f,
			0.0f, -0.5f, 0.0f, 0.5f,
			0.0f,  0.0f, 1.0f, 0.0f,
			0.0f,  0.0f, 0.0f, 1.0f
		);

		idRenderMatrix shadowToClip;
		idRenderMatrix::Multiply( renderMatrix_windowSpaceToClipSpace, lightProjectionRenderMatrix, shadowToClip );
		idRenderMatrix::Multiply( matClipToUvzw, shadowToClip, vLight->shadowP[0] );
#endif
	}
}

/*
=====================
idRenderBackend::ShadowMapPassPerforated
=====================
*/
void idRenderBackend::ShadowMapPassPerforated( const drawSurf_t** drawSurfs, int numDrawSurfs, viewLight_t* vLight, int side, const idRenderMatrix& lightProjectionRenderMatrix, const idRenderMatrix& lightViewRenderMatrix )
{
	if( r_skipShadows.GetBool() )
	{
		return;
	}

	if( drawSurfs == NULL || numDrawSurfs <= 0 )
	{
		return;
	}

	if( viewDef->renderView.rdflags & RDF_NOSHADOWS )
	{
		return;
	}

	renderLog.OpenBlock( "Render_ShadowMapsPerforated", colorBrown );

	uint64 glState = 0;

	// the actual stencil func will be set in the draw code, but we need to make sure it isn't
	// disabled here, and that the value will get reset for the interactions without looking
	// like a no-change-required
	GL_State( glState | GLS_POLYGON_OFFSET );

	switch( r_shadowMapOccluderFacing.GetInteger() )
	{
		case 0:
			GL_State( ( glStateBits & ~( GLS_CULL_MASK ) ) | GLS_CULL_FRONTSIDED );
			GL_PolygonOffset( r_shadowMapPolygonFactor.GetFloat(), r_shadowMapPolygonOffset.GetFloat() );
			break;

		case 1:
			GL_State( ( glStateBits & ~( GLS_CULL_MASK ) ) | GLS_CULL_BACKSIDED );
			GL_PolygonOffset( -r_shadowMapPolygonFactor.GetFloat(), -r_shadowMapPolygonOffset.GetFloat() );
			break;

		default:
			GL_State( ( glStateBits & ~( GLS_CULL_MASK ) ) | GLS_CULL_TWOSIDED );
			GL_PolygonOffset( r_shadowMapPolygonFactor.GetFloat(), r_shadowMapPolygonOffset.GetFloat() );
			break;
	}

	// process the chain of shadows with the current rendering state
	currentSpace = NULL;

	for( int surfNum = 0; surfNum < numDrawSurfs; surfNum++ )
	{
		const drawSurf_t* drawSurf = drawSurfs[ surfNum ];

		if( drawSurf->space != currentSpace )
		{
			// model -> world
			idRenderMatrix modelRenderMatrix;
			idRenderMatrix::Transpose( *( idRenderMatrix* )drawSurf->space->modelMatrix, modelRenderMatrix );

			// world -> light = light camera view of model in Doom
			idRenderMatrix modelToLightRenderMatrix;
			idRenderMatrix::Multiply( lightViewRenderMatrix, modelRenderMatrix, modelToLightRenderMatrix );

			idRenderMatrix clipMVP;
			idRenderMatrix::Multiply( lightProjectionRenderMatrix, modelToLightRenderMatrix, clipMVP );

			if( vLight->parallel )
			{
				// cascaded sun light shadowmap
				idRenderMatrix MVP;
				idRenderMatrix::Multiply( renderMatrix_clipSpaceToWindowSpace, clipMVP, MVP );

				RB_SetMVP( clipMVP );
			}
			else if( side < 0 )
			{
				// spot light
				idRenderMatrix MVP;
				idRenderMatrix::Multiply( renderMatrix_windowSpaceToClipSpace, clipMVP, MVP );

				RB_SetMVP( MVP );
			}
			else
			{
				// point light
				RB_SetMVP( clipMVP );
			}

			// set the local light position to allow the vertex program to project the shadow volume end cap to infinity
			/*
			idVec4 localLight( 0.0f );
			R_GlobalPointToLocal( drawSurf->space->modelMatrix, vLight->globalLightOrigin, localLight.ToVec3() );
			SetVertexParm( RENDERPARM_LOCALLIGHTORIGIN, localLight.ToFloatPtr() );
			*/

			currentSpace = drawSurf->space;
		}

		bool didDraw = false;

		const idMaterial* shader = drawSurf->material;

		// get the expressions for conditionals / color / texcoords
		const float* regs = drawSurf->shaderRegisters;
		idVec4 color( 0, 0, 0, 1 );

		uint64 surfGLState = 0;

		// set polygon offset if necessary
		if( shader && shader->TestMaterialFlag( MF_POLYGONOFFSET ) )
		{
			surfGLState |= GLS_POLYGON_OFFSET;
			GL_PolygonOffset( r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat() * shader->GetPolygonOffset() );
		}

		if( shader && shader->Coverage() == MC_PERFORATED )
		{
			// perforated surfaces may have multiple alpha tested stages
			for( int stage = 0; stage < shader->GetNumStages(); stage++ )
			{
				const shaderStage_t* pStage = shader->GetStage( stage );

				if( !pStage->hasAlphaTest )
				{
					continue;
				}

				// check the stage enable condition
				if( regs[ pStage->conditionRegister ] == 0 )
				{
					continue;
				}

				// if we at least tried to draw an alpha tested stage,
				// we won't draw the opaque surface
				didDraw = true;

				// set the alpha modulate
				color[3] = regs[ pStage->color.registers[3] ];

				// skip the entire stage if alpha would be black
				if( color[3] <= 0.0f )
				{
					continue;
				}

				uint64 stageGLState = surfGLState;

				// set privatePolygonOffset if necessary
				if( pStage->privatePolygonOffset )
				{
					GL_PolygonOffset( r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat() * pStage->privatePolygonOffset );
					stageGLState |= GLS_POLYGON_OFFSET;
				}

				GL_Color( color );

				GL_State( stageGLState );
				idVec4 alphaTestValue( regs[ pStage->alphaTestRegister ] );
				SetFragmentParm( RENDERPARM_ALPHA_TEST, alphaTestValue.ToFloatPtr() );

				if( drawSurf->jointCache )
				{
					renderProgManager.BindShader_TextureVertexColorSkinned();
				}
				else
				{
					renderProgManager.BindShader_TextureVertexColor();
				}

				RB_SetVertexColorParms( SVC_IGNORE );

				// bind the texture
				GL_SelectTexture( 0 );
				pStage->texture.image->Bind();

				// set texture matrix and texGens
				PrepareStageTexturing( pStage, drawSurf );

				// must render with less-equal for Z-Cull to work properly
				assert( ( GL_GetCurrentState() & GLS_DEPTHFUNC_BITS ) == GLS_DEPTHFUNC_LESS );

				// draw it
				DrawElementsWithCounters( drawSurf, true );

				// clean up
				FinishStageTexturing( pStage, drawSurf );

				// unset privatePolygonOffset if necessary
				if( pStage->privatePolygonOffset )
				{
					GL_PolygonOffset( r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat() * shader->GetPolygonOffset() );
				}
			}
		}

		if( !didDraw )
		{
			if( drawSurf->jointCache )
			{
				renderProgManager.BindShader_DepthSkinned();
			}
			else
			{
				renderProgManager.BindShader_Depth();
			}

			DrawElementsWithCounters( drawSurf, true );
		}
	}

	renderLog.CloseBlock();
}

/*
=====================
idRenderBackend::ShadowMapPassFast
=====================
*/
void idRenderBackend::ShadowMapPassFast( const drawSurf_t* drawSurfs, viewLight_t* vLight, int side, bool atlas )
{
	if( r_skipShadows.GetBool() )
	{
		return;
	}

	if( drawSurfs == NULL )
	{
		return;
	}

	if( viewDef->renderView.rdflags & RDF_NOSHADOWS )
	{
		return;
	}

	renderLog.OpenBlock( "Render_ShadowMaps", colorBrown );

	renderProgManager.BindShader_Depth();

	GL_SelectTexture( 0 );
	globalImages->blackImage->Bind();

	uint64 glState = 0;

	// the actual stencil func will be set in the draw code, but we need to make sure it isn't
	// disabled here, and that the value will get reset for the interactions without looking
	// like a no-change-required
	GL_State( glState | GLS_POLYGON_OFFSET );

	switch( r_shadowMapOccluderFacing.GetInteger() )
	{
		case 0:
			GL_State( ( glStateBits & ~( GLS_CULL_MASK ) ) | GLS_CULL_FRONTSIDED );
			GL_PolygonOffset( r_shadowMapPolygonFactor.GetFloat(), r_shadowMapPolygonOffset.GetFloat() );
			break;

		case 1:
			GL_State( ( glStateBits & ~( GLS_CULL_MASK ) ) | GLS_CULL_BACKSIDED );
			GL_PolygonOffset( -r_shadowMapPolygonFactor.GetFloat(), -r_shadowMapPolygonOffset.GetFloat() );
			break;

		default:
			GL_State( ( glStateBits & ~( GLS_CULL_MASK ) ) | GLS_CULL_TWOSIDED );
			GL_PolygonOffset( r_shadowMapPolygonFactor.GetFloat(), r_shadowMapPolygonOffset.GetFloat() );
			break;
	}

	idRenderMatrix lightProjectionRenderMatrix;
	idRenderMatrix lightViewRenderMatrix;
	SetupShadowMapMatrices( vLight, side, lightProjectionRenderMatrix, lightViewRenderMatrix );

	int slice = Max( 0, side );

	if( atlas )
	{
		globalFramebuffers.shadowAtlasFBO->Bind();

		GL_ViewportAndScissor( vLight->imageAtlasOffset[slice].x, vLight->imageAtlasOffset[slice].y, vLight->imageSize.x, vLight->imageSize.y );
	}
	else
	{
		globalFramebuffers.shadowFBO[vLight->shadowLOD][slice]->Bind();

		GL_ViewportAndScissor( 0, 0, shadowMapResolutions[vLight->shadowLOD], shadowMapResolutions[vLight->shadowLOD] );

		const nvrhi::FramebufferAttachment& att = currentFrameBuffer->GetApiObject()->getDesc().depthAttachment;
		if( att.texture )
		{
			commandList->clearDepthStencilTexture( att.texture, nvrhi::TextureSubresourceSet().setArraySlices( slice, 1 ), true, 1.f, false, 0x80 );
		}
	}

	pc.c_shadowViews++;

	// process the chain of shadows with the current rendering state
	currentSpace = NULL;

	int numDrawSurfs = 0;
	for( const drawSurf_t* drawSurf = drawSurfs; drawSurf != NULL; drawSurf = drawSurf->nextOnLight )
	{
		numDrawSurfs++;
	}

	const drawSurf_t** perforatedSurfaces = ( const drawSurf_t** )_alloca( numDrawSurfs * sizeof( drawSurf_t* ) );
	int numPerforatedSurfaces = 0;

	for( const drawSurf_t* drawSurf = drawSurfs; drawSurf != NULL; drawSurf = drawSurf->nextOnLight )
	{

		if( drawSurf->numIndexes == 0 )
		{
			continue;	// a job may have created an empty shadow geometry
		}

		const idMaterial* shader = drawSurf->material;

		if( shader == NULL )
		{
			continue;
		}

		// translucent surfaces don't put anything in the depth buffer
		if( shader->Coverage() == MC_TRANSLUCENT )
		{
			continue;
		}

		if( shader->Coverage() == MC_PERFORATED )
		{
			// save for later drawing
			perforatedSurfaces[ numPerforatedSurfaces ] = drawSurf;
			numPerforatedSurfaces++;
			continue;
		}

		if( drawSurf->space != currentSpace )
		{
			// model -> world
			idRenderMatrix modelRenderMatrix;
			idRenderMatrix::Transpose( *( idRenderMatrix* )drawSurf->space->modelMatrix, modelRenderMatrix );

			// world -> light = light camera view of model in Doom
			idRenderMatrix modelToLightRenderMatrix;
			idRenderMatrix::Multiply( lightViewRenderMatrix, modelRenderMatrix, modelToLightRenderMatrix );

			idRenderMatrix clipMVP;
			idRenderMatrix::Multiply( lightProjectionRenderMatrix, modelToLightRenderMatrix, clipMVP );

			if( vLight->parallel )
			{
				// cascaded sun light shadowmap
				idRenderMatrix MVP;
				idRenderMatrix::Multiply( renderMatrix_clipSpaceToWindowSpace, clipMVP, MVP );

				RB_SetMVP( clipMVP );
			}
			else if( side < 0 )
			{
				// spot light
				idRenderMatrix MVP;
				idRenderMatrix::Multiply( renderMatrix_windowSpaceToClipSpace, clipMVP, MVP );

				RB_SetMVP( MVP );
			}
			else
			{
				// point light
				RB_SetMVP( clipMVP );
			}

			currentSpace = drawSurf->space;
		}

		renderLog.OpenBlock( shader->GetName(), colorMdGrey );

		if( drawSurf->jointCache )
		{
			renderProgManager.BindShader_DepthSkinned();
		}
		else
		{
			renderProgManager.BindShader_Depth();
		}

		// must render with less-equal for Z-Cull to work properly
		assert( ( GL_GetCurrentState() & GLS_DEPTHFUNC_BITS ) == GLS_DEPTHFUNC_LESS );

		// draw it solid
		DrawElementsWithCounters( drawSurf , true );

		renderLog.CloseBlock();
	}

	// draw all perforated surfaces with the general code path
	if( numPerforatedSurfaces > 0 )
	{
		ShadowMapPassPerforated( perforatedSurfaces, numPerforatedSurfaces, vLight, side, lightProjectionRenderMatrix, lightViewRenderMatrix );
	}

	renderLog.CloseBlock();
}


class idSortrects : public idSort_Quick< int, idSortrects >
{
public:
	int SizeMetric( idVec2i v ) const
	{
		// skinny rects will sort earlier than square ones, because
		// they are more likely to grow the entire region
		return v.x * v.x + v.y * v.y;
	}
	int Compare( const int& a, const int& b ) const
	{
		return SizeMetric( ( *inputSizes )[b] ) - SizeMetric( ( *inputSizes )[a] );
	}
	const idList<idVec2i>* inputSizes;
};

void idRenderBackend::ShadowAtlasPass( const viewDef_t* _viewDef )
{
	if( r_skipShadows.GetBool() || !r_useShadowAtlas.GetBool() || viewDef->viewLights == NULL )
	{
		return;
	}

	OPTICK_EVENT( "Render_ShadowAtlas" );

	nvrhi::ObjectType commandObject = nvrhi::ObjectTypes::D3D12_GraphicsCommandList;
	if( deviceManager->GetGraphicsAPI() == nvrhi::GraphicsAPI::VULKAN )
	{
		commandObject = nvrhi::ObjectTypes::VK_CommandBuffer;
	}
	OPTICK_GPU_CONTEXT( ( void* ) commandList->getNativeObject( commandObject ) );
	OPTICK_GPU_EVENT( "Render_ShadowAtlas" );

	renderLog.OpenMainBlock( MRB_SHADOW_ATLAS_PASS );
	renderLog.OpenBlock( "Render_ShadowAtlas", colorYellow );

	GL_SelectTexture( 0 );

	Framebuffer* previousFramebuffer = Framebuffer::GetActiveFramebuffer();

	globalFramebuffers.shadowAtlasFBO->Bind();

	GL_ViewportAndScissor( 0, 0, r_shadowMapAtlasSize.GetInteger(), r_shadowMapAtlasSize.GetInteger() );

	/*
	const nvrhi::FramebufferAttachment& attColor = currentFrameBuffer->GetApiObject()->getDesc().colorAttachments[0];
	if( attColor.texture )
	{
		commandList->clearTextureFloat( attColor.texture, nvrhi::AllSubresources, nvrhi::Color( 0.0f ) );
	}
	*/

	const nvrhi::FramebufferAttachment& attDepth = currentFrameBuffer->GetApiObject()->getDesc().depthAttachment;
	if( attDepth.texture )
	{
		commandList->clearDepthStencilTexture( attDepth.texture, nvrhi::AllSubresources, true, 1.0f, false, 0x80 );
	}

	//
	// sort lights into atlas
	//

	int				shadowIndex = 0;
	idList<idVec2i>	inputSizes;
	//idStrList		inputNames;

	for( const viewLight_t* vLight = viewDef->viewLights; vLight != NULL; vLight = vLight->next )
	{
		if( vLight->lightShader->IsFogLight() )
		{
			continue;
		}

		if( vLight->lightShader->IsBlendLight() )
		{
			continue;
		}

		if( vLight->localInteractions == NULL && vLight->globalInteractions == NULL && vLight->translucentInteractions == NULL )
		{
			continue;
		}

		if( vLight->shadowLOD == -1 )
		{
			// light doesn't cast shadows
			continue;
		}

		int	side, sideStop;

		if( vLight->parallel )
		{
			side = 0;
			sideStop = r_shadowMapSplits.GetInteger() + 1;
		}
		else if( vLight->pointLight )
		{
			if( r_shadowMapSingleSide.GetInteger() != -1 )
			{
				side = r_shadowMapSingleSide.GetInteger();
				sideStop = side + 1;
			}
			else
			{
				side = 0;
				sideStop = 6;
			}
		}
		else
		{
			side = -1;
			sideStop = 0;
		}

		const idMaterial* lightShader = vLight->lightShader;

		for( ; side < sideStop ; side++ )
		{
			idVec2i size( shadowMapResolutions[ vLight->shadowLOD ], shadowMapResolutions[ vLight->shadowLOD ] );
			inputSizes.Append( size );

			//if( size.x >= 1024 )
			//{
			//	inputNames.Append( lightShader->GetName() );
			//}

			shadowIndex++;
		}
	}

	idList<idVec2i>	outputPositions;
	idList<int>		outputSizes;
	//idVec2i	totalSize;


	// RB: we don't use RectAllocatorQuadTree here because we don't want to rebuild the quad tree every frame

	outputPositions.SetNum( inputSizes.Num() );
	outputSizes.SetNum( inputSizes.Num() );

	idList<int> sizeRemap;
	sizeRemap.SetNum( inputSizes.Num() );
	for( int i = 0; i < inputSizes.Num(); i++ )
	{
		sizeRemap[i] = i;
	}

	// Sort the rects from largest to smallest (it makes allocating them in the image better)
	idSortrects sortrectsBySize;
	sortrectsBySize.inputSizes = &inputSizes;
	sizeRemap.SortWithTemplate( sortrectsBySize );

	tileMap.Clear();

	for( int i = 0; i < inputSizes.Num(); i++ )
	{
		shadowIndex = sizeRemap[i];
		//shadowIndex = i;

		idVec2i	size = inputSizes[ shadowIndex ];

		int area = Max( size.x, size.y );
		//int area = 1024;

		Tile tile;
		bool result = tileMap.GetTile( area, tile );

		if( !result )
		{
			outputPositions[ shadowIndex ].Set( -1, -1 );
			outputSizes[ shadowIndex ] = area;
		}
		else
		{
			int imageSize = tile.size * r_shadowMapAtlasSize.GetInteger();
			outputSizes[ shadowIndex ] = imageSize;

			// convert from [-1..-1] -> [0..1] and flip y
			idVec2 uvPos;
			uvPos.x = tile.position.x * 0.5f + 0.5f;
			uvPos.y = tile.position.y * 0.5f + 0.5f;

			idVec2i iPos;
			iPos.x = uvPos.x * r_shadowMapAtlasSize.GetInteger();
			iPos.y = uvPos.y * r_shadowMapAtlasSize.GetInteger();

			// RB: this is really odd but necessary
			iPos.x -= imageSize * 0.5f;
			iPos.y -= imageSize * 0.5f;

			outputPositions[ shadowIndex ].x = iPos.x;
			outputPositions[ shadowIndex ].y = iPos.y;

			pc.c_shadowAtlasUsage += ( imageSize * imageSize );
		}
	}

	//
	// for each light, perform shadowing to a big atlas Framebuffer
	//
	shadowIndex = 0;
	int failedNum = 0;

	for( viewLight_t* vLight = viewDef->viewLights; vLight != NULL; vLight = vLight->next )
	{
		if( vLight->lightShader->IsFogLight() )
		{
			continue;
		}

		if( vLight->lightShader->IsBlendLight() )
		{
			continue;
		}

		if( vLight->localInteractions == NULL && vLight->globalInteractions == NULL && vLight->translucentInteractions == NULL )
		{
			continue;
		}

		if( vLight->shadowLOD == -1 )
		{
			// light doesn't cast shadows
			vLight->imageSize.x = vLight->imageSize.y = -1;
			continue;
		}

		const idMaterial* lightShader = vLight->lightShader;
		renderLog.OpenBlock( lightShader->GetName(), colorMdGrey );

		int	side, sideStop;

		if( vLight->parallel )
		{
			side = 0;
			sideStop = r_shadowMapSplits.GetInteger() + 1;
		}
		else if( vLight->pointLight )
		{
			if( r_shadowMapSingleSide.GetInteger() != -1 )
			{
				side = r_shadowMapSingleSide.GetInteger();
				sideStop = side + 1;
			}
			else
			{
				side = 0;
				sideStop = 6;
			}
		}
		else
		{
			side = -1;
			sideStop = 0;
		}

		//vLight->imageSize.x = shadowMapResolutions[ vLight->shadowLOD ];
		//vLight->imageSize.y = shadowMapResolutions[ vLight->shadowLOD ];

		vLight->imageSize.x = outputSizes[ shadowIndex ];
		vLight->imageSize.y = outputSizes[ shadowIndex ];

		bool imageFitsIntoAtlas = true;

		for( ; side < sideStop ; side++ )
		{
			int slice = Max( 0, side );

			vLight->imageAtlasOffset[ slice ].x = outputPositions[ shadowIndex ].x;
			vLight->imageAtlasOffset[ slice ].y = outputPositions[ shadowIndex ].y;

			shadowIndex++;

			if( vLight->imageAtlasOffset[ slice ].x == -1 || vLight->imageAtlasOffset[ slice ].y == -1 )
			{
				// didn't fit into atlas anymore
				imageFitsIntoAtlas = false;
				failedNum++;
				continue;
			}

			ShadowMapPassFast( vLight->globalShadows, vLight, side, true );
		}

		if( !imageFitsIntoAtlas )
		{
			vLight->imageSize.x = -1;
		}

		renderLog.CloseBlock();
	}

	//idLib::Printf( "failed tiling for %i shadow maps\n", failedNum );

	// go back to main render target
	if( previousFramebuffer != NULL )
	{
		previousFramebuffer->Bind();
	}
	else
	{
		Framebuffer::Unbind();
	}
	renderProgManager.Unbind();

	GL_State( GLS_DEFAULT );

	SetFragmentParm( RENDERPARM_ALPHA_TEST, vec4_zero.ToFloatPtr() );

	// go back from light view to default camera view
	ResetViewportAndScissorToDefaultCamera( _viewDef );

	// unbind texture units
	GL_SelectTexture( 0 );

	renderLog.CloseBlock();
	renderLog.CloseMainBlock();
}

/*
==============================================================================================

DRAW INTERACTIONS

==============================================================================================
*/
/*
==================
idRenderBackend::DrawInteractions
==================
*/
void idRenderBackend::DrawInteractions( const viewDef_t* _viewDef )
{
	if( r_skipInteractions.GetBool() || viewDef->viewLights == NULL )
	{
		return;
	}

	OPTICK_EVENT( "Render_Interactions" );

	nvrhi::ObjectType commandObject = nvrhi::ObjectTypes::D3D12_GraphicsCommandList;
	if( deviceManager->GetGraphicsAPI() == nvrhi::GraphicsAPI::VULKAN )
	{
		commandObject = nvrhi::ObjectTypes::VK_CommandBuffer;
	}
	OPTICK_GPU_CONTEXT( ( void* ) commandList->getNativeObject( commandObject ) );
	OPTICK_GPU_EVENT( "Render_Interactions" );

	renderLog.OpenMainBlock( MRB_DRAW_INTERACTIONS );
	renderLog.OpenBlock( "Render_Interactions", colorYellow );

	GL_SelectTexture( 0 );

	const bool useLightDepthBounds = false; //r_useLightDepthBounds.GetBool() && !r_useShadowMapping.GetBool();

	Framebuffer* previousFramebuffer = Framebuffer::GetActiveFramebuffer();

	//
	// for each light, perform shadowing and adding
	//
	for( const viewLight_t* vLight = viewDef->viewLights; vLight != NULL; vLight = vLight->next )
	{
		// do fogging later
		if( vLight->lightShader->IsFogLight() )
		{
			continue;
		}
		if( vLight->lightShader->IsBlendLight() )
		{
			continue;
		}

		if( vLight->localInteractions == NULL && vLight->globalInteractions == NULL && vLight->translucentInteractions == NULL )
		{
			continue;
		}

		const idMaterial* lightShader = vLight->lightShader;
		renderLog.OpenBlock( lightShader->GetName(), colorMdGrey );

		// set the depth bounds for the whole light
		if( useLightDepthBounds )
		{
			GL_DepthBoundsTest( vLight->scissorRect.zmin, vLight->scissorRect.zmax );
		}

		// RB: render interactions with shadow mapping
		{
			if( !r_useShadowAtlas.GetBool() && vLight->shadowLOD > -1 )
			{
				int	side, sideStop;

				if( vLight->parallel )
				{
					side = 0;
					sideStop = r_shadowMapSplits.GetInteger() + 1;
				}
				else if( vLight->pointLight )
				{
					if( r_shadowMapSingleSide.GetInteger() != -1 )
					{
						side = r_shadowMapSingleSide.GetInteger();
						sideStop = side + 1;
					}
					else
					{
						side = 0;
						sideStop = 6;
					}
				}
				else
				{
					side = -1;
					sideStop = 0;
				}

				for( ; side < sideStop ; side++ )
				{
					// vLight is const but we make an exception here to store the shadow matrices per vLight
					// OPTIMIZE: these calculations could be moved to the renderer frontend into the multithreaded job
					ShadowMapPassFast( vLight->globalShadows, ( viewLight_t* ) vLight, side, false );
				}

				// go back to main render target
				if( previousFramebuffer != NULL )
				{
					previousFramebuffer->Bind();
				}
				else
				{
					Framebuffer::Unbind();
				}
				renderProgManager.Unbind();
			}

			GL_State( GLS_DEFAULT );

			SetFragmentParm( RENDERPARM_ALPHA_TEST, vec4_zero.ToFloatPtr() );

			// go back from light view to default camera view
			ResetViewportAndScissorToDefaultCamera( _viewDef );

			if( vLight->localInteractions != NULL )
			{
				renderLog.OpenBlock( "Local Light Interactions", colorPurple );
				RenderInteractions( vLight->localInteractions, vLight, GLS_DEPTHFUNC_EQUAL, false, useLightDepthBounds );
				renderLog.CloseBlock();
			}

			if( vLight->globalInteractions != NULL )
			{
				renderLog.OpenBlock( "Global Light Interactions", colorPurple );
				RenderInteractions( vLight->globalInteractions, vLight, GLS_DEPTHFUNC_EQUAL, false, useLightDepthBounds );
				renderLog.CloseBlock();
			}
		}
		// RB end

		if( vLight->translucentInteractions != NULL && !r_skipTranslucent.GetBool() )
		{
			renderLog.OpenBlock( "Translucent Interactions", colorCyan );

			// Disable the depth bounds test because translucent surfaces don't work with
			// the depth bounds tests since they did not write depth during the depth pass.
			if( useLightDepthBounds )
			{
				GL_DepthBoundsTest( 0.0f, 0.0f );
			}

			// The depth buffer wasn't filled in for translucent surfaces, so they
			// can never be constrained to perforated surfaces with the depthfunc equal.

			// Translucent surfaces do not receive shadows. This is a case where a
			// shadow buffer solution would work but stencil shadows do not because
			// stencil shadows only affect surfaces that contribute to the view depth
			// buffer and translucent surfaces do not contribute to the view depth buffer.

			RenderInteractions( vLight->translucentInteractions, vLight, GLS_DEPTHFUNC_LESS, false, false );

			renderLog.CloseBlock();
		}

		renderLog.CloseBlock();
	}

	// disable stencil shadow test
	GL_State( GLS_DEFAULT );

	// unbind texture units
	GL_SelectTexture( 0 );

	// reset depth bounds
	if( useLightDepthBounds )
	{
		GL_DepthBoundsTest( 0.0f, 0.0f );
	}

	renderLog.CloseBlock();
	renderLog.CloseMainBlock();
}

/*
=============================================================================================

NON-INTERACTION SHADER PASSES

=============================================================================================
*/

/*
=====================
idRenderBackend::DrawShaderPasses

Draw non-light dependent passes

If we are rendering Guis, the drawSurf_t::sort value is a depth offset that can
be multiplied by guiEye for polarity and screenSeparation for scale.
=====================
*/
int idRenderBackend::DrawShaderPasses( const drawSurf_t* const* const drawSurfs, const int numDrawSurfs,
									   const float guiStereoScreenOffset, const int stereoEye )
{
	// only obey skipAmbient if we are rendering a view
	if( viewDef->viewEntitys && r_skipAmbient.GetBool() )
	{
		return numDrawSurfs;
	}

	OPTICK_EVENT( "Render_GenericShaderPasses" );

	renderLog.OpenBlock( "Render_GenericShaderPasses", colorBlue );
	if( viewDef->targetRender )
	{
		viewDef->targetRender->Bind();
	}

	GL_SelectTexture( 0 );

	currentSpace = ( const viewEntity_t* )1;	// using NULL makes /analyze think surf->space needs to be checked...
	float currentGuiStereoOffset = 0.0f;

	int i = 0;
	for( ; i < numDrawSurfs; i++ )
	{
		const drawSurf_t* surf = drawSurfs[i];
		const idMaterial* shader = surf->material;

		if( !shader->HasAmbient() )
		{
			continue;
		}

		if( shader->IsPortalSky() )
		{
			continue;
		}

		// some deforms may disable themselves by setting numIndexes = 0
		if( surf->numIndexes == 0 )
		{
			continue;
		}

		if( shader->SuppressInSubview() )
		{
			continue;
		}

		if( viewDef->isXraySubview && surf->space->entityDef )
		{
			if( surf->space->entityDef->parms.xrayIndex != 2 )
			{
				continue;
			}
		}

		// we need to draw the post process shaders after we have drawn the fog lights
		if( shader->GetSort() >= SS_POST_PROCESS && !currentRenderCopied )
		{
			break;
		}

		// if we are rendering a 3D view and the surface's eye index doesn't match
		// the current view's eye index then we skip the surface
		// if the stereoEye value of a surface is 0 then we need to draw it for both eyes.
		const int shaderStereoEye = shader->GetStereoEye();
		const bool isEyeValid = stereoRender_swapEyes.GetBool() ? ( shaderStereoEye == stereoEye ) : ( shaderStereoEye != stereoEye );
		if( ( stereoEye != 0 ) && ( shaderStereoEye != 0 ) && ( isEyeValid ) )
		{
			continue;
		}

		renderLog.OpenBlock( shader->GetName(), colorMdGrey );

		// determine the stereoDepth offset
		// guiStereoScreenOffset will always be zero for 3D views, so the !=
		// check will never force an update due to the current sort value.
		const float thisGuiStereoOffset = guiStereoScreenOffset * surf->sort;

		// change the matrix and other space related vars if needed
		if( surf->space != currentSpace || thisGuiStereoOffset != currentGuiStereoOffset )
		{
			currentSpace = surf->space;
			currentGuiStereoOffset = thisGuiStereoOffset;

			const viewEntity_t* space = currentSpace;

			if( guiStereoScreenOffset != 0.0f )
			{
				RB_SetMVPWithStereoOffset( space->mvp, currentGuiStereoOffset );
			}
			else
			{
				RB_SetMVP( space->mvp );
			}

			// set eye position in local space
			idVec4 localViewOrigin( 1.0f );
			R_GlobalPointToLocal( space->modelMatrix, viewDef->renderView.vieworg, localViewOrigin.ToVec3() );
			SetVertexParm( RENDERPARM_LOCALVIEWORIGIN, localViewOrigin.ToFloatPtr() );

			// set model Matrix
			float modelMatrixTranspose[16];
			R_MatrixTranspose( space->modelMatrix, modelMatrixTranspose );
			SetVertexParms( RENDERPARM_MODELMATRIX_X, modelMatrixTranspose, 4 );

			// Set ModelView Matrix
			float modelViewMatrixTranspose[16];
			R_MatrixTranspose( space->modelViewMatrix, modelViewMatrixTranspose );
			SetVertexParms( RENDERPARM_MODELVIEWMATRIX_X, modelViewMatrixTranspose, 4 );
		}

		// change the scissor if needed
		if( !currentScissor.Equals( surf->scissorRect ) && r_useScissor.GetBool() )
		{
			GL_Scissor( viewDef->viewport.x1 + surf->scissorRect.x1,
						viewDef->viewport.y1 + surf->scissorRect.y1,
						surf->scissorRect.x2 + 1 - surf->scissorRect.x1,
						surf->scissorRect.y2 + 1 - surf->scissorRect.y1 );

			currentScissor = surf->scissorRect;
		}

		// get the expressions for conditionals / color / texcoords
		const float*	regs = surf->shaderRegisters;

		// set face culling appropriately
		uint64 cullMode;
		if( surf->space->isGuiSurface )
		{
			cullMode = GLS_CULL_TWOSIDED;
		}
		else
		{
			switch( shader->GetCullType() )
			{
				case CT_TWO_SIDED:
					cullMode = GLS_CULL_TWOSIDED;
					break;

				case CT_BACK_SIDED:
					cullMode = GLS_CULL_BACKSIDED;
					break;

				case CT_FRONT_SIDED:
				default:
					cullMode = GLS_CULL_FRONTSIDED;
					break;
			}
		}

		uint64 surfGLState = surf->extraGLState | cullMode;

		// set polygon offset if necessary

		if( shader->TestMaterialFlag( MF_POLYGONOFFSET ) )
		{
			GL_PolygonOffset( r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat() * shader->GetPolygonOffset() );
			surfGLState |= GLS_POLYGON_OFFSET;
		}

		for( int stage = 0; stage < shader->GetNumStages(); stage++ )
		{
			const shaderStage_t* pStage = shader->GetStage( stage );

			// check the enable condition
			if( regs[ pStage->conditionRegister ] == 0 )
			{
				continue;
			}

			// skip the stages involved in lighting
			if( pStage->lighting != SL_AMBIENT )
			{
				continue;
			}

			uint64 stageGLState = surfGLState;
			if( ( surfGLState & GLS_OVERRIDE ) == 0 )
			{
				stageGLState |= pStage->drawStateBits;
			}

			// skip if the stage is ( GL_ZERO, GL_ONE ), which is used for some alpha masks
			if( ( stageGLState & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) ) == ( GLS_SRCBLEND_ZERO | GLS_DSTBLEND_ONE ) )
			{
				continue;
			}

			if( pStage->stencilStage != nullptr )
			{
				switch( pStage->stencilStage->comp )
				{
					case STENCIL_COMP_ALWAYS:
						stageGLState |= GLS_STENCIL_FUNC_ALWAYS;
						break;
					case STENCIL_COMP_EQUAL:
						stageGLState |= GLS_STENCIL_FUNC_EQUAL;
						break;
				};

				switch( pStage->stencilStage->pass )
				{
					case STENCIL_OP_REPLACE:
						stageGLState |= GLS_STENCIL_OP_PASS_REPLACE;
						break;
					case STENCIL_OP_KEEP:
						stageGLState |= GLS_STENCIL_OP_PASS_KEEP;
						break;
					case STENCIL_OP_DECRWRAP:
						stageGLState |= GLS_STENCIL_OP_PASS_DECR_WRAP;
						break;
				};

				switch( pStage->stencilStage->zFail )
				{
					case STENCIL_OP_DECRWRAP:
						stageGLState |= GLS_STENCIL_OP_ZFAIL_DECR_WRAP;
						break;
					case STENCIL_OP_KEEP:
						stageGLState |= GLS_STENCIL_OP_ZFAIL_KEEP;
						break;
				};

				switch( pStage->stencilStage->fail )
				{
					case STENCIL_OP_KEEP:
						stageGLState |= GLS_STENCIL_OP_FAIL_KEEP;
						break;
				};

				stageGLState |= GLS_STENCIL_MAKE_REF( pStage->stencilStage->ref )
								| GLS_STENCIL_MAKE_MASK( pStage->stencilStage->writeMask );
			}

			// see if we are a new-style stage
			newShaderStage_t* newStage = pStage->newStage;

			if( newStage != NULL )
			{
				//--------------------------
				//
				// new style stages
				//
				//--------------------------
				if( r_skipNewAmbient.GetBool() )
				{
					continue;
				}
				renderLog.OpenBlock( "Custom Renderproc Shader Stage", colorRed );

				GL_State( stageGLState );

				renderProgManager.BindProgram( newStage->glslProgram );

				for( int j = 0; j < newStage->numVertexParms; j++ )
				{
					float parm[4];
					parm[0] = regs[ newStage->vertexParms[j][0] ];
					parm[1] = regs[ newStage->vertexParms[j][1] ];
					parm[2] = regs[ newStage->vertexParms[j][2] ];
					parm[3] = regs[ newStage->vertexParms[j][3] ];
					SetVertexParm( ( renderParm_t )( RENDERPARM_USER0 + j ), parm );
				}

				// set rpEnableSkinning if the shader has optional support for skinning
				if( surf->jointCache && renderProgManager.ShaderHasOptionalSkinning() )
				{
					const idVec4 skinningParm( 1.0f );
					SetVertexParm( RENDERPARM_ENABLE_SKINNING, skinningParm.ToFloatPtr() );
				}

				// bind texture units
				for( int j = 0; j < newStage->numFragmentProgramImages; j++ )
				{
					idImage* image = newStage->fragmentProgramImages[j];
					if( image != NULL )
					{
						GL_SelectTexture( j );
						image->Bind();
					}
				}

				// draw it
				DrawElementsWithCounters( surf );

				// clear rpEnableSkinning if it was set
				if( surf->jointCache && renderProgManager.ShaderHasOptionalSkinning() )
				{
					const idVec4 skinningParm( 0.0f );
					SetVertexParm( RENDERPARM_ENABLE_SKINNING, skinningParm.ToFloatPtr() );
				}

				GL_SelectTexture( 0 );
				renderProgManager.Unbind();

				renderLog.CloseBlock();
				continue;
			}

			//--------------------------
			//
			// old style stages
			//
			//--------------------------

			// set the color
			idVec4 color;
			color[0] = regs[ pStage->color.registers[0] ];
			color[1] = regs[ pStage->color.registers[1] ];
			color[2] = regs[ pStage->color.registers[2] ];
			color[3] = regs[ pStage->color.registers[3] ];

			// skip the entire stage if an add would be black
			if( ( stageGLState & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) ) == ( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE )
					&& color[0] <= 0 && color[1] <= 0 && color[2] <= 0 )
			{
				continue;
			}

			// skip the entire stage if a blend would be completely transparent
			if( ( stageGLState & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) ) == ( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA )
					&& color[3] <= 0 )
			{
				continue;
			}

			stageVertexColor_t svc = pStage->vertexColor;

			renderLog.OpenBlock( "Standard Shader Stage", colorGreen );
			GL_Color( color );

			if( surf->space->isGuiSurface )
			{
				// Force gui surfaces to always be SVC_MODULATE
				svc = SVC_MODULATE;

				// use special shaders for bink cinematics
				if( pStage->texture.cinematic )
				{
					if( ( stageGLState & GLS_OVERRIDE ) != 0 || viewDef->targetRender )
					{
						// This is a hack... Only SWF Guis set GLS_OVERRIDE
						// Old style guis do not, and we don't want them to use the new GUI renederProg
						renderProgManager.BindShader_TextureVertexColor_sRGB();
					}
					else
					{
						renderProgManager.BindShader_TextureVertexColor();
					}
				}
				else
				{
					if( ( stageGLState & GLS_OVERRIDE ) != 0 )
					{
						// This is a hack... Only SWF Guis set GLS_OVERRIDE
						// Old style guis do not, and we don't want them to use the new GUI renderProg
						renderProgManager.BindShader_GUI();
					}
					else
					{
						if( surf->jointCache )
						{
							renderProgManager.BindShader_TextureVertexColorSkinned();
						}
						else
						{
							// For now, don't render to linear unless it's being directly rendererd to either the backbuffer or an offline framebuffer
							if( viewDef->is2Dgui || viewDef->targetRender )
							{
								// RB: 2D fullscreen drawing like warp or damage blend effects
								renderProgManager.BindShader_TextureVertexColor_sRGB();
							}
							else
							{
								renderProgManager.BindShader_TextureVertexColor();
							}
						}
					}
				}
			}
			else if( ( pStage->texture.texgen == TG_SCREEN ) || ( pStage->texture.texgen == TG_SCREEN2 ) )
			{
				renderProgManager.BindShader_TextureTexGenVertexColor();
			}
			else if( pStage->texture.cinematic )
			{
				renderProgManager.BindShader_Bink();
			}
			else
			{
				if( surf->jointCache )
				{
					renderProgManager.BindShader_TextureVertexColorSkinned();
				}
				else
				{
					renderProgManager.BindShader_TextureVertexColor();
				}
			}

			RB_SetVertexColorParms( svc );

			// bind the texture
			BindVariableStageImage( &pStage->texture, regs, commandList );

			// set privatePolygonOffset if necessary
			if( pStage->privatePolygonOffset )
			{
				stageGLState |= GLS_POLYGON_OFFSET;
			}

			// set the state
			GL_State( stageGLState );

			PrepareStageTexturing( pStage, surf );

			// draw it
			DrawElementsWithCounters( surf );

			FinishStageTexturing( pStage, surf );

			// unset privatePolygonOffset if necessary
			if( pStage->privatePolygonOffset )
			{
				GL_PolygonOffset( r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat() * shader->GetPolygonOffset() );
			}

			renderLog.CloseBlock();
		}

		renderLog.CloseBlock();
	}

	GL_Color( 1.0f, 1.0f, 1.0f );

	// disable stencil shadow test
	GL_State( GLS_DEFAULT );

	GL_SelectTexture( 0 );

	renderLog.CloseBlock();
	return i;
}

/*
=============================================================================================

BLEND LIGHT PROJECTION

=============================================================================================
*/

/*
=====================
idRenderBackend::T_BlendLight
=====================
*/
void idRenderBackend::T_BlendLight( const drawSurf_t* drawSurfs, const viewLight_t* vLight )
{
	currentSpace = NULL;

	for( const drawSurf_t* drawSurf = drawSurfs; drawSurf != NULL; drawSurf = drawSurf->nextOnLight )
	{
		if( drawSurf->scissorRect.IsEmpty() )
		{
			continue;	// !@# FIXME: find out why this is sometimes being hit!
			// temporarily jump over the scissor and draw so the gl error callback doesn't get hit
		}

		if( !currentScissor.Equals( drawSurf->scissorRect ) && r_useScissor.GetBool() )
		{
			// change the scissor
			GL_Scissor( viewDef->viewport.x1 + drawSurf->scissorRect.x1,
						viewDef->viewport.y1 + drawSurf->scissorRect.y1,
						drawSurf->scissorRect.x2 + 1 - drawSurf->scissorRect.x1,
						drawSurf->scissorRect.y2 + 1 - drawSurf->scissorRect.y1 );

			currentScissor = drawSurf->scissorRect;
		}

		if( drawSurf->space != currentSpace )
		{
			// change the matrix
			RB_SetMVP( drawSurf->space->mvp );

			// change the light projection matrix
			idPlane	lightProjectInCurrentSpace[4];
			for( int i = 0; i < 4; i++ )
			{
				R_GlobalPlaneToLocal( drawSurf->space->modelMatrix, vLight->lightProject[i], lightProjectInCurrentSpace[i] );
			}

			SetVertexParm( RENDERPARM_TEXGEN_0_S, lightProjectInCurrentSpace[0].ToFloatPtr() );
			SetVertexParm( RENDERPARM_TEXGEN_0_T, lightProjectInCurrentSpace[1].ToFloatPtr() );
			SetVertexParm( RENDERPARM_TEXGEN_0_Q, lightProjectInCurrentSpace[2].ToFloatPtr() );
			SetVertexParm( RENDERPARM_TEXGEN_1_S, lightProjectInCurrentSpace[3].ToFloatPtr() );	// falloff

			currentSpace = drawSurf->space;
		}

		DrawElementsWithCounters( drawSurf );
	}
}

/*
=====================
idRenderBackend::BlendLight

Dual texture together the falloff and projection texture with a blend
mode to the framebuffer, instead of interacting with the surface texture
=====================
*/
void idRenderBackend::BlendLight( const drawSurf_t* drawSurfs, const drawSurf_t* drawSurfs2, const viewLight_t* vLight )
{
	if( drawSurfs == NULL )
	{
		return;
	}
	if( r_skipBlendLights.GetBool() )
	{
		return;
	}
	renderLog.OpenBlock( vLight->lightShader->GetName(), colorPink );

	const idMaterial* lightShader = vLight->lightShader;
	const float*	 regs = vLight->shaderRegisters;

	// texture 1 will get the falloff texture
	GL_SelectTexture( 1 );
	vLight->falloffImage->Bind();

	// texture 0 will get the projected texture
	GL_SelectTexture( 0 );

	renderProgManager.BindShader_BlendLight();

	for( int i = 0; i < lightShader->GetNumStages(); i++ )
	{
		const shaderStage_t*	stage = lightShader->GetStage( i );

		if( !regs[ stage->conditionRegister ] )
		{
			continue;
		}

		GL_State( GLS_DEPTHMASK | stage->drawStateBits | GLS_DEPTHFUNC_EQUAL );

		GL_SelectTexture( 0 );
		stage->texture.image->Bind();

		if( stage->texture.hasMatrix )
		{
			RB_LoadShaderTextureMatrix( regs, &stage->texture );
		}

		// get the modulate values from the light, including alpha, unlike normal lights
		idVec4 lightColor;
		lightColor[0] = regs[ stage->color.registers[0] ];
		lightColor[1] = regs[ stage->color.registers[1] ];
		lightColor[2] = regs[ stage->color.registers[2] ];
		lightColor[3] = regs[ stage->color.registers[3] ];
		GL_Color( lightColor );

		T_BlendLight( drawSurfs, vLight );
		T_BlendLight( drawSurfs2, vLight );
	}

	GL_SelectTexture( 0 );

	renderProgManager.Unbind();
	renderLog.CloseBlock();
}

/*
=========================================================================================================

FOG LIGHTS

=========================================================================================================
*/

/*
=====================
idRenderBackend::T_BasicFog
=====================
*/
void idRenderBackend::T_BasicFog( const drawSurf_t* drawSurfs, const idPlane fogPlanes[4], const idRenderMatrix* inverseBaseLightProject )
{
	currentSpace = NULL;

	for( const drawSurf_t* drawSurf = drawSurfs; drawSurf != NULL; drawSurf = drawSurf->nextOnLight )
	{
		if( drawSurf->scissorRect.IsEmpty() )
		{
			continue;	// !@# FIXME: find out why this is sometimes being hit!
			// temporarily jump over the scissor and draw so the gl error callback doesn't get hit
		}

		if( !currentScissor.Equals( drawSurf->scissorRect ) && r_useScissor.GetBool() )
		{
			// change the scissor
			GL_Scissor( viewDef->viewport.x1 + drawSurf->scissorRect.x1,
						viewDef->viewport.y1 + drawSurf->scissorRect.y1,
						drawSurf->scissorRect.x2 + 1 - drawSurf->scissorRect.x1,
						drawSurf->scissorRect.y2 + 1 - drawSurf->scissorRect.y1 );

			currentScissor = drawSurf->scissorRect;
		}

		if( drawSurf->space != currentSpace )
		{
			idPlane localFogPlanes[4];
			if( inverseBaseLightProject == NULL )
			{
				RB_SetMVP( drawSurf->space->mvp );
				for( int i = 0; i < 4; i++ )
				{
					R_GlobalPlaneToLocal( drawSurf->space->modelMatrix, fogPlanes[i], localFogPlanes[i] );
				}
			}
			else
			{
				idRenderMatrix invProjectMVPMatrix;
				idRenderMatrix::Multiply( viewDef->worldSpace.mvp, *inverseBaseLightProject, invProjectMVPMatrix );
				RB_SetMVP( invProjectMVPMatrix );
				for( int i = 0; i < 4; i++ )
				{
					inverseBaseLightProject->InverseTransformPlane( fogPlanes[i], localFogPlanes[i], false );
				}
			}

			SetVertexParm( RENDERPARM_TEXGEN_0_S, localFogPlanes[0].ToFloatPtr() );
			SetVertexParm( RENDERPARM_TEXGEN_0_T, localFogPlanes[1].ToFloatPtr() );
			SetVertexParm( RENDERPARM_TEXGEN_1_T, localFogPlanes[2].ToFloatPtr() );
			SetVertexParm( RENDERPARM_TEXGEN_1_S, localFogPlanes[3].ToFloatPtr() );

			currentSpace = ( inverseBaseLightProject == NULL ) ? drawSurf->space : NULL;
		}

		if( drawSurf->jointCache )
		{
			renderProgManager.BindShader_FogSkinned();
		}
		else
		{
			renderProgManager.BindShader_Fog();
		}

		DrawElementsWithCounters( drawSurf );
	}
}

/*
==================
idRenderBackend::FogPass
==================
*/
void idRenderBackend::FogPass( const drawSurf_t* drawSurfs,  const drawSurf_t* drawSurfs2, const viewLight_t* vLight )
{
	renderLog.OpenBlock( vLight->lightShader->GetName(), colorCyan );

	// find the current color and density of the fog
	const idMaterial* lightShader = vLight->lightShader;
	const float* regs = vLight->shaderRegisters;
	// assume fog shaders have only a single stage
	const shaderStage_t* stage = lightShader->GetStage( 0 );

	idVec4 lightColor;
	lightColor[0] = regs[ stage->color.registers[0] ];
	lightColor[1] = regs[ stage->color.registers[1] ];
	lightColor[2] = regs[ stage->color.registers[2] ];
	lightColor[3] = regs[ stage->color.registers[3] ];

	GL_Color( lightColor );

	// calculate the falloff planes
	float a;

	// if they left the default value on, set a fog distance of 500
	if( lightColor[3] <= 1.0f )
	{
		a = -0.5f / DEFAULT_FOG_DISTANCE;
	}
	else
	{
		// otherwise, distance = alpha color
		a = -0.5f / lightColor[3];
	}

	// texture 0 is the falloff image
	GL_SelectTexture( 0 );
	globalImages->fogImage->Bind();

	// texture 1 is the entering plane fade correction
	GL_SelectTexture( 1 );
	globalImages->fogEnterImage->Bind();

	// S is based on the view origin
	const float s = vLight->fogPlane.Distance( viewDef->renderView.vieworg );

	const float FOG_SCALE = 0.001f;

	idPlane fogPlanes[4];

	// S-0
	fogPlanes[0][0] = a * viewDef->worldSpace.modelViewMatrix[0 * 4 + 2];
	fogPlanes[0][1] = a * viewDef->worldSpace.modelViewMatrix[1 * 4 + 2];
	fogPlanes[0][2] = a * viewDef->worldSpace.modelViewMatrix[2 * 4 + 2];
	fogPlanes[0][3] = a * viewDef->worldSpace.modelViewMatrix[3 * 4 + 2] + 0.5f;

	// T-0
	fogPlanes[1][0] = 0.0f;//a * backEnd.viewDef->worldSpace.modelViewMatrix[0*4+0];
	fogPlanes[1][1] = 0.0f;//a * backEnd.viewDef->worldSpace.modelViewMatrix[1*4+0];
	fogPlanes[1][2] = 0.0f;//a * backEnd.viewDef->worldSpace.modelViewMatrix[2*4+0];
	fogPlanes[1][3] = 0.5f;//a * backEnd.viewDef->worldSpace.modelViewMatrix[3*4+0] + 0.5f;

	// T-1 will get a texgen for the fade plane, which is always the "top" plane on unrotated lights
	fogPlanes[2][0] = FOG_SCALE * vLight->fogPlane[0];
	fogPlanes[2][1] = FOG_SCALE * vLight->fogPlane[1];
	fogPlanes[2][2] = FOG_SCALE * vLight->fogPlane[2];
	fogPlanes[2][3] = FOG_SCALE * vLight->fogPlane[3] + FOG_ENTER;

	// S-1
	fogPlanes[3][0] = 0.0f;
	fogPlanes[3][1] = 0.0f;
	fogPlanes[3][2] = 0.0f;
	fogPlanes[3][3] = FOG_SCALE * s + FOG_ENTER;

	// draw it
	GL_State( GLS_DEPTHMASK | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHFUNC_EQUAL );
	T_BasicFog( drawSurfs, fogPlanes, NULL );
	T_BasicFog( drawSurfs2, fogPlanes, NULL );

	// the light frustum bounding planes aren't in the depth buffer, so use depthfunc_less instead
	// of depthfunc_equal
	GL_State( GLS_DEPTHMASK | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHFUNC_LESS | GLS_CULL_BACKSIDED );

	zeroOneCubeSurface.space = &viewDef->worldSpace;
	zeroOneCubeSurface.scissorRect = viewDef->scissor;
	T_BasicFog( &zeroOneCubeSurface, fogPlanes, &vLight->inverseBaseLightProject );

	GL_State( ( glStateBits & ~( GLS_CULL_MASK ) ) | GLS_CULL_FRONTSIDED );

	GL_SelectTexture( 0 );

	renderProgManager.Unbind();

	renderLog.CloseBlock();
}

/*
==================
idRenderBackend::FogAllLights
==================
*/
void idRenderBackend::FogAllLights()
{
	if( r_skipFogLights.GetBool() || r_showOverDraw.GetInteger() != 0
			|| viewDef->isXraySubview /* don't fog in xray mode*/ )
	{
		return;
	}

	//OPTICK_EVENT( "Render_FogAllLights" );

	renderLog.OpenMainBlock( MRB_FOG_ALL_LIGHTS );
	renderLog.OpenBlock( "Render_FogAllLights", colorBlue );

	// force fog plane to recalculate
	currentSpace = NULL;

	for( viewLight_t* vLight = viewDef->viewLights; vLight != NULL; vLight = vLight->next )
	{
		if( vLight->lightShader->IsFogLight() )
		{
			FogPass( vLight->globalInteractions, vLight->localInteractions, vLight );
		}
		else if( vLight->lightShader->IsBlendLight() )
		{
			BlendLight( vLight->globalInteractions, vLight->localInteractions, vLight );
		}
	}

	renderLog.CloseBlock();
	renderLog.CloseMainBlock();
}

void idRenderBackend::DrawMotionVectors()
{
	if( !viewDef->viewEntitys )
	{
		// 3D views only
		return;
	}

	if( !R_UseTemporalAA() && r_motionBlur.GetInteger() <= 0 )
	{
		return;
	}

	if( viewDef->isSubview )
	{
		return;
	}

	if( viewDef->renderView.rdflags & ( RDF_NOAMBIENT | RDF_IRRADIANCE ) )
	{
		return;
	}

	//OPTICK_EVENT( "Render_MotionVectors" );

	renderLog.OpenBlock( "Render_MotionVectors" );

	// clear the alpha buffer and draw only the hands + weapon into it so
	// we can avoid blurring them
	GL_State( GLS_COLORMASK | GLS_DEPTHMASK );
	GL_Color( 0, 0, 0, 1 );

	renderProgManager.BindShader_Color();

	currentSpace = &viewDef->worldSpace;
	RB_SetMVP( viewDef->worldSpace.mvp );

	DrawElementsWithCounters( &unitSquareSurface );

	// draw the hands + weapon with alpha 0
	GL_Color( 0, 0, 0, 0 );

	GL_SelectTexture( 0 );
	globalImages->blackImage->Bind();
	currentSpace = NULL;

	drawSurf_t** drawSurfs = ( drawSurf_t** )&viewDef->drawSurfs[0];
	for( int surfNum = 0; surfNum < viewDef->numDrawSurfs; surfNum++ )
	{
		const drawSurf_t* surf = drawSurfs[ surfNum ];

		if( !surf->space->weaponDepthHack && !surf->space->skipMotionBlur && !surf->material->HasSubview() )
		{
			// Apply motion blur to this object
			continue;
		}

		const idMaterial* shader = surf->material;
		if( shader->Coverage() == MC_TRANSLUCENT )
		{
			// muzzle flash, etc
			continue;
		}

		// set mvp matrix
		if( surf->space != currentSpace )
		{
			RB_SetMVP( surf->space->mvp );
			currentSpace = surf->space;
		}

		// this could just be a color, but we don't have a skinned color-only prog
		if( surf->jointCache )
		{
			renderProgManager.BindShader_TextureVertexColorSkinned();
		}
		else
		{
			renderProgManager.BindShader_TextureVertexColor();
		}

		// draw it solid
		DrawElementsWithCounters( surf );
	}

	globalFramebuffers.taaMotionVectorsFBO->Bind();

	commandList->clearTextureFloat( globalImages->taaMotionVectorsImage->GetTextureHandle(), nvrhi::AllSubresources, nvrhi::Color( 0.f ) );

	// in stereo rendering, each eye needs to get a separate previous frame mvp
	int mvpIndex = ( viewDef->renderView.viewEyeBuffer == 1 ) ? 1 : 0;

	// derive the matrix to go from current pixels to previous frame pixels
	bool cameraMoved = false;
	idRenderMatrix	motionMatrix;

	if( memcmp( &viewDef->worldSpace.unjitteredMVP[0][0], &prevMVP[mvpIndex][0][0], sizeof( idRenderMatrix ) ) != 0 )
	{
		idRenderMatrix	inverseMVP;
		idRenderMatrix::Inverse( viewDef->worldSpace.unjitteredMVP, inverseMVP );

		idRenderMatrix::Multiply( prevMVP[mvpIndex], inverseMVP, motionMatrix );

		cameraMoved = true;
	}

	prevMVP[mvpIndex] = viewDef->worldSpace.unjitteredMVP;

	// make sure rpWindowCoord is set even without post processing surfaces in the view
	int x = viewDef->viewport.x1;
	int y = viewDef->viewport.y1;
	int	w = viewDef->viewport.x2 - viewDef->viewport.x1 + 1;
	int	h = viewDef->viewport.y2 - viewDef->viewport.y1 + 1;

	// window coord to 0.0 to 1.0 conversion
	float windowCoordParm[4];
	windowCoordParm[0] = 1.0f / w;
	windowCoordParm[1] = 1.0f / h;
	windowCoordParm[2] = w;
	windowCoordParm[3] = h;
	SetFragmentParm( RENDERPARM_WINDOWCOORD, windowCoordParm ); // rpWindowCoord

	if( r_taaMotionVectors.GetBool() && prevViewsValid && cameraMoved )
	{
		RB_SetMVP( motionMatrix );

		GL_State( GLS_DEPTHFUNC_ALWAYS | GLS_DEPTHMASK | GLS_CULL_TWOSIDED );

		renderProgManager.BindShader_MotionVectors();

		GL_SelectTexture( 0 );
		globalImages->currentRenderHDRImage->Bind();

		GL_SelectTexture( 1 );
		globalImages->currentDepthImage->Bind();

		DrawElementsWithCounters( &unitSquareSurface );
	}

	renderLog.CloseBlock();
}

void idRenderBackend::TemporalAAPass( const viewDef_t* _viewDef )
{
	// if we are just doing 2D rendering, no need for HDR TAA
	if( viewDef->viewEntitys == NULL )
	{
		return;
	}

	if( !R_UseTemporalAA() )
	{
		return;
	}

	if( viewDef->isSubview )
	{
		return;
	}

	if( viewDef->renderView.rdflags & ( RDF_NOAMBIENT | RDF_IRRADIANCE ) )
	{
		return;
	}

	//OPTICK_EVENT( "Render_TemporalAA" );

	renderLog.OpenMainBlock( MRB_TAA );
	renderLog.OpenBlock( "Render_TemporalAA" );

	TemporalAntiAliasingParameters params =
	{
		r_taaNewFrameWeight.GetFloat(),
		r_taaClampingFactor.GetFloat(),
		r_taaMaxRadiance.GetFloat(),
		r_taaEnableHistoryClamping.GetBool()
	};
	taaPass->TemporalResolve( commandList, params, prevViewsValid, _viewDef );
	prevViewsValid = true;

	renderLog.CloseBlock();
	renderLog.CloseMainBlock();
}

idVec2 idRenderBackend::GetCurrentPixelOffset() const
{
	if( taaPass )
	{
		return taaPass->GetCurrentPixelOffset();
	}

	return idVec2( 0, 0 );
}

// RB: FIXME currently not used
void idRenderBackend::Bloom( const viewDef_t* _viewDef )
{
	if( _viewDef->is2Dgui || ( _viewDef->renderView.rdflags & RDF_IRRADIANCE ) )
	{
		return;
	}

	renderLog.OpenMainBlock( MRB_BLOOM );
	renderLog.OpenBlock( "Render_Bloom", colorBlue );

	// BRIGHTPASS
	renderLog.OpenBlock( "Brightpass" );

	//Framebuffer::Unbind();
	//globalFramebuffers.hdrQuarterFBO->Bind();

	GL_State( /*GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO |*/ GLS_DEPTHMASK | GLS_DEPTHFUNC_ALWAYS | GLS_CULL_TWOSIDED );

	int screenWidth = renderSystem->GetWidth();
	int screenHeight = renderSystem->GetHeight();

	// set the window clipping
	GL_Viewport( 0, 0, screenWidth / 4, screenHeight / 4 );
	GL_Scissor( 0, 0, screenWidth / 4, screenHeight / 4 );

	globalFramebuffers.bloomRenderFBO[ 0 ]->Bind();

	GL_SelectTexture( 0 );

	globalImages->currentRenderHDRImage->Bind();

	renderProgManager.BindShader_Brightpass();

	float screenCorrectionParm[4];
	screenCorrectionParm[0] = hdrKey;
	screenCorrectionParm[1] = hdrAverageLuminance;
	screenCorrectionParm[2] = hdrMaxLuminance;
	screenCorrectionParm[3] = 1.0f;
	SetFragmentParm( RENDERPARM_SCREENCORRECTIONFACTOR, screenCorrectionParm ); // rpScreenCorrectionFactor

	float overbright[4];
	{
		if( r_hdrAutoExposure.GetBool() )
		{
			overbright[0] = r_hdrContrastDynamicThreshold.GetFloat();
		}
		else
		{
			overbright[0] = r_hdrContrastStaticThreshold.GetFloat();
		}
		overbright[1] = r_hdrContrastOffset.GetFloat();
		overbright[2] = 0;
		overbright[3] = 0;
	}
	SetFragmentParm( RENDERPARM_OVERBRIGHT, overbright ); // rpOverbright

	// Draw
	DrawElementsWithCounters( &unitSquareSurface );

	renderLog.CloseBlock(); // Brightpass

	renderLog.OpenBlock( "Bloom Ping Pong" );

	// BLOOM PING PONG rendering
	renderProgManager.BindShader_HDRGlareChromatic();

	int j;
	for( j = 0; j < r_hdrGlarePasses.GetInteger(); j++ )
	{
		int index = ( j + 1 ) % 2;
		globalFramebuffers.bloomRenderFBO[ index ]->Bind();

		commandList->clearTextureFloat( globalImages->bloomRenderImage[index]->GetTextureHandle(), nvrhi::AllSubresources, nvrhi::Color( 0.f, 0.f, 0.f, 1.f ) );

		globalImages->bloomRenderImage[j % 2]->Bind();

		DrawElementsWithCounters( &unitSquareSurface );
	}

	// add filtered glare back to main context
	globalFramebuffers.ldrFBO->Bind();

	ResetViewportAndScissorToDefaultCamera( _viewDef );

	GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHMASK | GLS_DEPTHFUNC_ALWAYS );

	renderProgManager.BindShader_Screen();

	globalImages->bloomRenderImage[( j + 1 ) % 2]->Bind();

	DrawElementsWithCounters( &unitSquareSurface );

	renderLog.CloseBlock(); // Bloom Ping Pong

	renderProgManager.Unbind();

	GL_State( GLS_DEFAULT );

	renderLog.CloseBlock(); // Render_Bloom
	renderLog.CloseMainBlock(); // MRB_BLOOM
}


void idRenderBackend::DrawScreenSpaceAmbientOcclusion( const viewDef_t* _viewDef )
{
	if( !_viewDef->viewEntitys || _viewDef->is2Dgui )
	{
		// 3D views only
		return;
	}

	// FIXME: the hierarchical depth buffer does not work with the MSAA depth texture source
	if( !r_useSSAO.GetBool() || R_GetMSAASamples() > 1 )
	{
		return;
	}

	// skip this in subviews because it is very expensive
	if( _viewDef->isSubview )
	{
		return;
	}

	if( _viewDef->renderView.rdflags & ( RDF_NOAMBIENT | RDF_IRRADIANCE ) )
	{
		return;
	}

	//OPTICK_EVENT( "Render_SSAO" );

	renderLog.OpenMainBlock( MRB_SSAO_PASS );
	renderLog.OpenBlock( "Render_SSAO", colorBlue );

	currentSpace = &viewDef->worldSpace;
	RB_SetMVP( viewDef->worldSpace.mvp );

	Framebuffer* previousFramebuffer = Framebuffer::GetActiveFramebuffer();

	int screenWidth = renderSystem->GetWidth();
	int screenHeight = renderSystem->GetHeight();

	commandList->clearTextureFloat( globalImages->hierarchicalZbufferImage->GetTextureHandle(), nvrhi::AllSubresources, nvrhi::Color( 1.f ) );
	commandList->clearTextureFloat( globalImages->ambientOcclusionImage[0]->GetTextureHandle(), nvrhi::AllSubresources, nvrhi::Color( 1.f ) );

	// build hierarchical depth buffer
	if( r_useHierarchicalDepthBuffer.GetBool() )
	{
		renderLog.OpenBlock( "Render_HiZ" );

		//if( R_GetMSAASamples() > 1 )
		//{
		//	commandList->resolveTexture( globalImages->hierarchicalZbufferImage->GetTextureHandle(), nvrhi::AllSubresources, globalImages->currentDepthImage->GetTextureHandle(), nvrhi::AllSubresources );
		//}
		//else
		{
			commonPasses.BlitTexture(
				commandList,
				globalFramebuffers.csDepthFBO[0]->GetApiObject(),
				globalImages->currentDepthImage->GetTextureHandle(),
				&bindingCache );
		}

		hiZGenPass->Dispatch( commandList, MAX_HIERARCHICAL_ZBUFFERS );

		renderLog.CloseBlock();
	}

	if( previousFramebuffer != NULL )
	{
		previousFramebuffer->Bind();
	}
	else
	{
		Framebuffer::Unbind();
	}

	// set the window clipping
	int aoScreenWidth = globalFramebuffers.ambientOcclusionFBO[0]->GetWidth();
	int aoScreenHeight = globalFramebuffers.ambientOcclusionFBO[0]->GetHeight();

	GL_Viewport( 0, 0, aoScreenWidth, aoScreenHeight );
	GL_Scissor( 0, 0, aoScreenWidth, aoScreenHeight );

	{
		globalFramebuffers.ambientOcclusionFBO[0]->Bind();

		GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO | GLS_DEPTHMASK | GLS_DEPTHFUNC_ALWAYS | GLS_CULL_TWOSIDED );

		const nvrhi::FramebufferAttachment& att = globalFramebuffers.ambientOcclusionFBO[0]->GetApiObject()->getDesc().colorAttachments[0];

		if( att.texture )
		{
			commandList->clearTextureFloat( att.texture, att.subresources, 0 );
		}

		if( r_ssaoFiltering.GetBool() )
		{
			renderProgManager.BindShader_AmbientOcclusion();
		}
		else
		{
			renderProgManager.BindShader_AmbientOcclusionAndOutput();
		}
	}

	float screenCorrectionParm[4];
	screenCorrectionParm[0] = float( screenWidth ) / aoScreenWidth;
	screenCorrectionParm[1] = float( screenHeight ) / aoScreenHeight;
	screenCorrectionParm[2] = 0.0f;
	screenCorrectionParm[3] = 1.0f;
	SetFragmentParm( RENDERPARM_SCREENCORRECTIONFACTOR, screenCorrectionParm ); // rpScreenCorrectionFactor

	// window coord to 0.0 to 1.0 conversion
	float windowCoordParm[4];
	windowCoordParm[0] = 1.0f / aoScreenWidth;
	windowCoordParm[1] = 1.0f / aoScreenHeight;
	windowCoordParm[2] = aoScreenWidth;
	windowCoordParm[3] = aoScreenHeight;
	SetFragmentParm( RENDERPARM_WINDOWCOORD, windowCoordParm ); // rpWindowCoord

	SetVertexParms( RENDERPARM_MODELMATRIX_X, viewDef->unprojectionToCameraRenderMatrix[0], 4 );

	const float jitterSampleScale = 1.0f;

	float jitterTexScale[4];
	jitterTexScale[0] = r_shadowMapJitterScale.GetFloat() * jitterSampleScale;
	jitterTexScale[1] = r_shadowMapJitterScale.GetFloat() * jitterSampleScale;
	jitterTexScale[2] = 0.0f;
	jitterTexScale[3] = 0.0f;
	SetFragmentParm( RENDERPARM_JITTERTEXSCALE, jitterTexScale ); // rpJitterTexScale

	float jitterTexOffset[4];
	jitterTexOffset[0] = 1.0f / globalImages->blueNoiseImage256->GetUploadWidth();
	jitterTexOffset[1] = 1.0f / globalImages->blueNoiseImage256->GetUploadHeight();

	if( r_shadowMapRandomizeJitter.GetBool() )
	{
		jitterTexOffset[2] = Sys_Milliseconds() / 1000.0f;
		jitterTexOffset[3] = tr.frameCount % 64;
	}
	else
	{
		jitterTexOffset[2] = 0.0f;
		jitterTexOffset[3] = 0.0f;
	}

	SetFragmentParm( RENDERPARM_JITTERTEXOFFSET, jitterTexOffset ); // rpJitterTexOffset

	GL_SelectTexture( 0 );
	globalImages->gbufferNormalsRoughnessImage->Bind();

	GL_SelectTexture( 1 );
	if( r_useHierarchicalDepthBuffer.GetBool() )
	{
		globalImages->hierarchicalZbufferImage->Bind();
	}
	else
	{
		globalImages->currentDepthImage->Bind();
	}

	GL_SelectTexture( 2 );
	globalImages->blueNoiseImage256->Bind();

	DrawElementsWithCounters( &unitSquareSurface );

	if( r_ssaoFiltering.GetBool() )
	{
		float jitterTexScale[4];

		commandList->clearTextureFloat( globalImages->ambientOcclusionImage[1]->GetTextureHandle(), nvrhi::AllSubresources, nvrhi::Color( 1.f ) );

		// AO blur X
		globalFramebuffers.ambientOcclusionFBO[1]->Bind();

		renderProgManager.BindShader_AmbientOcclusionBlur();

		// set axis parameter
		jitterTexScale[0] = 1;
		jitterTexScale[1] = 0;
		jitterTexScale[2] = 0;
		jitterTexScale[3] = 0;
		SetFragmentParm( RENDERPARM_JITTERTEXSCALE, jitterTexScale ); // rpJitterTexScale

		GL_SelectTexture( 2 );
		globalImages->ambientOcclusionImage[0]->Bind();

		DrawElementsWithCounters( &unitSquareSurface );

		// AO blur Y
		globalFramebuffers.ambientOcclusionFBO[0]->Bind();

		renderProgManager.BindShader_AmbientOcclusionBlurAndOutput();

		// set axis parameter
		jitterTexScale[0] = 0;
		jitterTexScale[1] = 1;
		jitterTexScale[2] = 0;
		jitterTexScale[3] = 0;
		SetFragmentParm( RENDERPARM_JITTERTEXSCALE, jitterTexScale ); // rpJitterTexScale

		GL_SelectTexture( 2 );
		globalImages->ambientOcclusionImage[1]->Bind();

		DrawElementsWithCounters( &unitSquareSurface );
	}

	{
		// go back to main scene render target
		if( previousFramebuffer != NULL )
		{
			previousFramebuffer->Bind();
		}
		else
		{
			Framebuffer::Unbind();
		}
	}

	//
	// reset state
	//
	renderProgManager.Unbind();

	GL_Viewport( 0, 0, screenWidth, screenHeight );
	GL_Scissor( 0, 0, screenWidth, screenHeight );

	GL_State( GLS_DEFAULT );

	renderLog.CloseBlock();
	renderLog.CloseMainBlock();
}


/*
NVRHI SSAO using compute shaders.
*/
void idRenderBackend::DrawScreenSpaceAmbientOcclusion2( const viewDef_t* _viewDef )
{
	if( !r_useSSAO.GetBool() )
	{
		return;
	}

	if( !_viewDef->viewEntitys || _viewDef->is2Dgui )
	{
		// 3D views only
		return;
	}

	// skip this in subviews because it is very expensive
	if( _viewDef->isSubview )
	{
		return;
	}

	if( _viewDef->renderView.rdflags & ( RDF_NOAMBIENT | RDF_IRRADIANCE ) )
	{
		return;
	}

	//OPTICK_EVENT( "Render_SSAO2" );

	renderLog.OpenMainBlock( MRB_SSAO_PASS );
	renderLog.OpenBlock( "Render_SSAO2", colorBlue );

	commandList->clearTextureFloat( globalImages->ambientOcclusionImage[0]->GetTextureHandle(), nvrhi::AllSubresources, nvrhi::Color( 1.f ) );

	ssaoPass->Render( commandList, _viewDef, 0 );

	renderLog.CloseBlock();
	renderLog.CloseMainBlock();
}

/*
=========================================================================================================

BACKEND COMMANDS

=========================================================================================================
*/

/*
====================
RB_ExecuteBackEndCommands

This function will be called syncronously if running without
smp extensions, or asyncronously by another thread.
====================
*/
void idRenderBackend::ExecuteBackEndCommands( const emptyCommand_t* cmds )
{
	SCOPED_PROFILE_EVENT( "ExecuteBackEndCommands" );

	// r_debugRenderToTexture
	int c_draw3d = 0;
	int c_draw2d = 0;
	int c_setBuffers = 0;
	int c_copyRenders = 0;

	resolutionScale.SetCurrentGPUFrameTime( commonLocal.GetRendererGPUMicroseconds() );

	ResizeImages();

	if( cmds->commandId == RC_NOP && !cmds->next )
	{
		return;
	}

	if( renderSystem->GetStereo3DMode() != STEREO3D_OFF )
	{
		StereoRenderExecuteBackEndCommands( cmds );
		//renderLog.EndFrame();
		return;
	}

	GL_StartFrame();

	void* textureId = globalImages->hierarchicalZbufferImage->GetTextureID();
	globalImages->LoadDeferredImages( commandList );

	if( !ssaoPass && r_useNewSsaoPass.GetBool() )
	{
		ssaoPass = new SsaoPass(
			deviceManager->GetDevice(),
			&commonPasses, globalImages->currentDepthImage->GetTextureHandle(),
			globalImages->gbufferNormalsRoughnessImage->GetTextureHandle(),
			globalImages->ambientOcclusionImage[0]->GetTextureHandle() );
	}

	if( globalImages->hierarchicalZbufferImage->GetTextureID() != textureId || !hiZGenPass )
	{
		if( hiZGenPass )
		{
			delete hiZGenPass;
		}

		hiZGenPass = new MipMapGenPass( deviceManager->GetDevice(), globalImages->hierarchicalZbufferImage->GetTextureHandle() );
	}


	if( !toneMapPass )
	{
		TonemapPass::CreateParameters createParms;
		toneMapPass = new TonemapPass();
		toneMapPass->Init( deviceManager->GetDevice(), &commonPasses, createParms, globalFramebuffers.ldrFBO->GetApiObject() );
	}

	if( !taaPass )
	{
		TemporalAntiAliasingPass::CreateParameters taaParams;
		taaParams.sourceDepth = globalImages->currentDepthImage->GetTextureHandle();
		taaParams.motionVectors = globalImages->taaMotionVectorsImage->GetTextureHandle();
		taaParams.unresolvedColor = globalImages->currentRenderHDRImage->GetTextureHandle();
		taaParams.resolvedColor = globalImages->taaResolvedImage->GetTextureHandle();
		taaParams.feedback1 = globalImages->taaFeedback1Image->GetTextureHandle();
		taaParams.feedback2 = globalImages->taaFeedback2Image->GetTextureHandle();
		taaParams.motionVectorStencilMask = 0; //0x01;
		taaParams.useCatmullRomFilter = true;
		taaPass = new TemporalAntiAliasingPass();
		taaPass->Init( deviceManager->GetDevice(), &commonPasses, NULL, taaParams );
	}

	uint64 backEndStartTime = Sys_Microseconds();

	// needed for editor rendering
	GL_SetDefaultState();

	// SRS - Save glConfig.timerQueryAvailable state so it can be disabled for RC_DRAW_VIEW_GUI then restored after it is finished
	const bool timerQueryAvailable = glConfig.timerQueryAvailable;
	drawView3D = false;

	for( ; cmds != NULL; cmds = ( const emptyCommand_t* )cmds->next )
	{
		switch( cmds->commandId )
		{
			case RC_NOP:
				break;

			case RC_DRAW_VIEW_GUI:
				if( drawView3D )
				{
					// SRS - Capture separate timestamps for overlay GUI rendering when RC_DRAW_VIEW_3D timestamps are active
					renderLog.OpenMainBlock( MRB_DRAW_GUI );
					renderLog.OpenBlock( "Render_DrawViewGUI", colorBlue );
					// SRS - Disable detailed timestamps during overlay GUI rendering so they do not overwrite timestamps from 3D rendering
					glConfig.timerQueryAvailable = false;

					DrawView( cmds, 0 );

					// SRS - Restore timestamp capture state after overlay GUI rendering is finished
					glConfig.timerQueryAvailable = timerQueryAvailable;
					renderLog.CloseBlock();
					renderLog.CloseMainBlock( MRB_DRAW_GUI );
				}
				else
				{
					DrawView( cmds, 0 );
				}
				c_draw2d++;
				break;

			case RC_DRAW_VIEW_3D:
				drawView3D = true;
				DrawView( cmds, 0 );
				c_draw3d++;
				break;

			case RC_SET_BUFFER:
				SetBuffer( cmds );
				c_setBuffers++;
				break;

			case RC_COPY_RENDER:
				CopyRender( cmds );
				c_copyRenders++;
				break;

			case RC_POST_PROCESS:
			{
				// apply optional post processing
				PostProcess( cmds );
				break;
			}

			default:
				common->Error( "RB_ExecuteBackEndCommands: bad commandId" );
				break;
		}
	}

	DrawFlickerBox();

	GL_EndFrame();

	// stop rendering on this thread
	uint64 backEndFinishTime = Sys_Microseconds();
	pc.cpuTotalMicroSec = backEndFinishTime - backEndStartTime;

	if( r_debugRenderToTexture.GetInteger() == 1 )
	{
		common->Printf( "3d: %i, 2d: %i, SetBuf: %i, CpyRenders: %i, CpyFrameBuf: %i\n", c_draw3d, c_draw2d, c_setBuffers, c_copyRenders, pc.c_copyFrameBuffer );
		pc.c_copyFrameBuffer = 0;
	}

	//renderLog.EndFrame();
}


/*
==================
idRenderBackend::DrawViewInternal
==================
*/
void idRenderBackend::DrawViewInternal( const viewDef_t* _viewDef, const int stereoEye )
{
	OPTICK_EVENT( "Backend_DrawViewInternal" );
	OPTICK_TAG( "stereoEye", stereoEye );

	nvrhi::ObjectType commandObject = nvrhi::ObjectTypes::D3D12_GraphicsCommandList;
	if( deviceManager->GetGraphicsAPI() == nvrhi::GraphicsAPI::VULKAN )
	{
		commandObject = nvrhi::ObjectTypes::VK_CommandBuffer;
	}
	OPTICK_GPU_CONTEXT( ( void* ) commandList->getNativeObject( commandObject ) );
	OPTICK_GPU_EVENT( "DrawView" );

	renderLog.OpenBlock( "Render_DrawViewInternal", colorRed );

	//-------------------------------------------------
	// guis can wind up referencing purged images that need to be loaded.
	// this used to be in the gui emit code, but now that it can be running
	// in a separate thread, it must not try to load images, so do it here.
	//-------------------------------------------------
	drawSurf_t** drawSurfs = ( drawSurf_t** )&_viewDef->drawSurfs[0];
	const int numDrawSurfs = _viewDef->numDrawSurfs;

	for( int i = 0; i < numDrawSurfs; i++ )
	{
		const drawSurf_t* ds = _viewDef->drawSurfs[ i ];
		if( ds->material != NULL )
		{
			const_cast<idMaterial*>( ds->material )->EnsureNotPurged();
		}
	}

	// SP: reset the graphics state for validation layers
	currentVertexBuffer = nullptr;
	currentIndexBuffer = nullptr;
	currentJointBuffer = nullptr;
	currentVertexOffset = 0;
	currentIndexOffset = 0;
	currentJointOffset = 0;

	//-------------------------------------------------
	// RB_BeginDrawingView
	//
	// Any mirrored or portaled views have already been drawn, so prepare
	// to actually render the visible surfaces for this view
	//
	// clear the z buffer, set the projection matrix, etc
	//-------------------------------------------------
	ResetViewportAndScissorToDefaultCamera( _viewDef );

	// ensures that depth writes are enabled for the depth clear
	GL_State( GLS_DEFAULT | GLS_CULL_FRONTSIDED, true );

	bool useHDR = !_viewDef->is2Dgui;
	bool clearColor = false;

	if( _viewDef->renderView.rdflags & RDF_IRRADIANCE )
	{
		globalFramebuffers.envprobeFBO->Bind();
		clearColor = true;
	}
	else if( useHDR )
	{
		globalFramebuffers.hdrFBO->Bind();
	}
	else if( viewDef->targetRender )
	{
		viewDef->targetRender->Bind();
	}
	else
	{
		globalFramebuffers.ldrFBO->Bind();
	}

	// Clear the depth buffer and clear the stencil to 128 for stencil shadows as well as gui masking
	GL_Clear( clearColor, true, true, STENCIL_SHADOW_TEST_VALUE, 0.0f, 0.0f, 0.0f, 0.0f, false );

	// RB end

	//------------------------------------
	// sets variables that can be used by all programs
	//------------------------------------
	{
		//
		// set eye position in global space
		//
		float parm[4];
		parm[0] = viewDef->renderView.vieworg[0];
		parm[1] = viewDef->renderView.vieworg[1];
		parm[2] = viewDef->renderView.vieworg[2];
		parm[3] = 1.0f;

		SetVertexParm( RENDERPARM_GLOBALEYEPOS, parm ); // rpGlobalEyePos

		// sets overbright to make world brighter
		// This value is baked into the specularScale and diffuseScale values so
		// the interaction programs don't need to perform the extra multiply,
		// but any other renderprogs that want to obey the brightness value
		// can reference this.
		float overbright = r_lightScale.GetFloat() * 0.5f;
		parm[0] = overbright;
		parm[1] = overbright;
		parm[2] = overbright;
		parm[3] = overbright;
		SetFragmentParm( RENDERPARM_OVERBRIGHT, parm );

		// Set Projection Matrix
		float projMatrixTranspose[16];
		R_MatrixTranspose( viewDef->projectionMatrix, projMatrixTranspose );
		SetVertexParms( RENDERPARM_PROJMATRIX_X, projMatrixTranspose, 4 );
	}

	//-------------------------------------------------
	// fill the depth buffer and clear color buffer to black except on subviews
	//-------------------------------------------------
	FillDepthBufferFast( drawSurfs, numDrawSurfs );

	//-------------------------------------------------
	// FIXME, OPTIMIZE: merge this with FillDepthBufferFast like in a light prepass deferred renderer
	//
	// fill the geometric buffer with normals and roughness
	//-------------------------------------------------
	{
		OPTICK_EVENT( "Render_GeometryBuffer" );

		AmbientPass( drawSurfs, numDrawSurfs, true );
	}

	//-------------------------------------------------
	// build hierarchical depth buffer and SSAO render target
	//-------------------------------------------------
	if( r_useNewSsaoPass.GetBool() )
	{
		DrawScreenSpaceAmbientOcclusion2( _viewDef );
	}
	else
	{
		DrawScreenSpaceAmbientOcclusion( _viewDef );
	}

	//-------------------------------------------------
	// render static lighting and consider SSAO results
	//-------------------------------------------------
	{
		OPTICK_EVENT( "Render_AmbientPass" );

		AmbientPass( drawSurfs, numDrawSurfs, false );
	}

	//-------------------------------------------------
	// render all light <-> geometry interactions to a depth buffer atlas
	//-------------------------------------------------
	ShadowAtlasPass( _viewDef );

	//-------------------------------------------------
	// main light renderer
	//-------------------------------------------------
	DrawInteractions( _viewDef );

	//-------------------------------------------------
	// now draw any non-light dependent shading passes
	//-------------------------------------------------
	int processed = 0;
	if( !r_skipShaderPasses.GetBool() )
	{
		renderLog.OpenMainBlock( MRB_DRAW_SHADER_PASSES );
		float guiScreenOffset;
		if( _viewDef->viewEntitys != NULL )
		{
			// guiScreenOffset will be 0 in non-gui views
			guiScreenOffset = 0.0f;
		}
		else
		{
			guiScreenOffset = stereoEye * _viewDef->renderView.stereoScreenSeparation;
		}
		processed = DrawShaderPasses( drawSurfs, numDrawSurfs, guiScreenOffset, stereoEye );
		renderLog.CloseMainBlock();
	}

	//-------------------------------------------------
	// use direct light and emissive light contributions to add indirect screen space light
	//-------------------------------------------------
	//RB_SSGI( viewDef );

	//-------------------------------------------------
	// fog and blend lights, drawn after emissive surfaces
	// so they are properly dimmed down
	//-------------------------------------------------
	FogAllLights();

	//-------------------------------------------------
	// now draw any screen warping post-process effects using _currentRender
	//-------------------------------------------------
	if( processed < numDrawSurfs && !r_skipPostProcess.GetBool() )
	{
		int x = viewDef->viewport.x1;
		int y = viewDef->viewport.y1;
		int	w = viewDef->viewport.x2 - viewDef->viewport.x1 + 1;
		int	h = viewDef->viewport.y2 - viewDef->viewport.y1 + 1;

		GL_SelectTexture( 0 );

		// resolve the screen
		if( R_GetMSAASamples() > 1 )
		{
			renderLog.OpenBlock( "Resolve to _currentRender" );

			commandList->resolveTexture( globalImages->currentRenderImage->GetTextureHandle(), nvrhi::AllSubresources, globalImages->currentRenderHDRImage->GetTextureHandle(), nvrhi::AllSubresources );
		}
		else
		{
			renderLog.OpenBlock( "Blit to _currentRender" );

			BlitParameters blitParms;
			nvrhi::IFramebuffer* currentFB = ( nvrhi::IFramebuffer* )currentFrameBuffer->GetApiObject();
			blitParms.sourceTexture = currentFB->getDesc().colorAttachments[0].texture;
			blitParms.targetFramebuffer = globalFramebuffers.postProcFBO->GetApiObject(); // _currentRender image
			blitParms.targetViewport = nvrhi::Viewport( renderSystem->GetWidth(), renderSystem->GetHeight() );
			commonPasses.BlitTexture( commandList, blitParms, &bindingCache );
		}

		renderLog.CloseBlock();

		currentRenderCopied = true;

		// RENDERPARM_SCREENCORRECTIONFACTOR amd RENDERPARM_WINDOWCOORD overlap
		// diffuseScale and specularScale

		// screen power of two correction factor (no longer relevant now)
		float screenCorrectionParm[4];
		screenCorrectionParm[0] = 1.0f;
		screenCorrectionParm[1] = 1.0f;
		screenCorrectionParm[2] = 0.0f;
		screenCorrectionParm[3] = 1.0f;
		SetFragmentParm( RENDERPARM_SCREENCORRECTIONFACTOR, screenCorrectionParm ); // rpScreenCorrectionFactor

		// window coord to 0.0 to 1.0 conversion
		float windowCoordParm[4];
		windowCoordParm[0] = 1.0f / w;
		windowCoordParm[1] = 1.0f / h;
		windowCoordParm[2] = w;
		windowCoordParm[3] = h;
		SetFragmentParm( RENDERPARM_WINDOWCOORD, windowCoordParm ); // rpWindowCoord

		// render the remaining surfaces
		renderLog.OpenMainBlock( MRB_DRAW_SHADER_PASSES_POST );
		DrawShaderPasses( drawSurfs + processed, numDrawSurfs - processed, 0.0f /* definitely not a gui */, stereoEye );
		renderLog.CloseMainBlock();
	}

	//-------------------------------------------------
	// render debug tools
	//-------------------------------------------------
	DBG_RenderDebugTools( drawSurfs, numDrawSurfs );

	//-------------------------------------------------
	// motion vectors are useful for TAA and motion blur
	//-------------------------------------------------
	DrawMotionVectors();

	//-------------------------------------------------
	// resolve of HDR target using temporal anti aliasing before any tonemapping and post processing
	//
	// use this to eat all stochastic noise like from volumetric light sampling or SSAO
	// runs at full resolution
	//-------------------------------------------------
	TemporalAAPass( _viewDef );

	//-------------------------------------------------
	// tonemapping: convert back from HDR to LDR range
	//-------------------------------------------------

	if( useHDR && !( _viewDef->renderView.rdflags & RDF_IRRADIANCE ) && !_viewDef->targetRender )
	{
		ToneMappingParameters parms;
		if( R_UseTemporalAA() )
		{
			toneMapPass->SimpleRender( commandList, parms, viewDef, globalImages->taaResolvedImage->GetTextureHandle(), globalFramebuffers.ldrFBO->GetApiObject() );
		}
		else
		{
			if( R_GetMSAASamples() > 1 )
			{
				commandList->resolveTexture( globalImages->taaResolvedImage->GetTextureHandle(), nvrhi::AllSubresources, globalImages->currentRenderHDRImage->GetTextureHandle(), nvrhi::AllSubresources );

				toneMapPass->SimpleRender( commandList, parms, viewDef, globalImages->taaResolvedImage->GetTextureHandle(), globalFramebuffers.ldrFBO->GetApiObject() );
			}
			else
			{
				toneMapPass->SimpleRender( commandList, parms, viewDef, globalImages->currentRenderHDRImage->GetTextureHandle(), globalFramebuffers.ldrFBO->GetApiObject() );
			}
		}
	}

	//-------------------------------------------------
	// bloom post processing
	//-------------------------------------------------

	// TODO implement bloom
	//Bloom( _viewDef );

	if( _viewDef->renderView.rdflags & RDF_IRRADIANCE )
	{
		// copy LDR result to DX12 / Vulkan swapchain image

		// we haven't changed ldrImage so it's basically the previewsRenderLDR
		BlitParameters blitParms;
		blitParms.sourceTexture = ( nvrhi::ITexture* )globalImages->ldrImage->GetTextureID();
		blitParms.targetFramebuffer = deviceManager->GetCurrentFramebuffer();
		blitParms.targetViewport = nvrhi::Viewport( renderSystem->GetWidth(), renderSystem->GetHeight() );
		commonPasses.BlitTexture( commandList, blitParms, &bindingCache );

		// blit envprobe over it for quick review where we are

		blitParms.sourceTexture = ( nvrhi::ITexture* )globalImages->envprobeHDRImage->GetTextureID();
		blitParms.targetFramebuffer = deviceManager->GetCurrentFramebuffer();
		blitParms.targetViewport = nvrhi::Viewport( ENVPROBE_CAPTURE_SIZE, ENVPROBE_CAPTURE_SIZE );
		commonPasses.BlitTexture( commandList, blitParms, &bindingCache );
	}
	else
	{
		// copy LDR result to DX12 / Vulkan swapchain image

		BlitParameters blitParms;
		blitParms.sourceTexture = ( nvrhi::ITexture* )globalImages->ldrImage->GetTextureID();
		blitParms.targetFramebuffer = deviceManager->GetCurrentFramebuffer();
		blitParms.targetViewport = nvrhi::Viewport( renderSystem->GetWidth(), renderSystem->GetHeight() );
		commonPasses.BlitTexture( commandList, blitParms, &bindingCache );
	}

	renderLog.CloseBlock();
}

/*
==================
idRenderBackend::MotionBlur

Experimental feature
==================
*/
void idRenderBackend::MotionBlur()
{
	if( !viewDef->viewEntitys )
	{
		// 3D views only
		return;
	}
	if( r_motionBlur.GetInteger() <= 0 )
	{
		return;
	}
	if( viewDef->isSubview )
	{
		return;
	}

	// clear the alpha buffer and draw only the hands + weapon into it so
	// we can avoid blurring them
	GL_State( GLS_COLORMASK | GLS_DEPTHMASK );

	GL_Color( 0, 0, 0, 0 );
	GL_SelectTexture( 0 );
	globalImages->blackImage->Bind();
	currentSpace = NULL;

	drawSurf_t** drawSurfs = ( drawSurf_t** )&viewDef->drawSurfs[0];
	for( int surfNum = 0; surfNum < viewDef->numDrawSurfs; surfNum++ )
	{
		const drawSurf_t* surf = drawSurfs[ surfNum ];

		if( !surf->space->weaponDepthHack && !surf->space->skipMotionBlur && !surf->material->HasSubview() )
		{
			// Apply motion blur to this object
			continue;
		}

		const idMaterial* shader = surf->material;
		if( shader->Coverage() == MC_TRANSLUCENT )
		{
			// muzzle flash, etc
			continue;
		}

		// set mvp matrix
		if( surf->space != currentSpace )
		{
			RB_SetMVP( surf->space->mvp );
			currentSpace = surf->space;
		}

		// this could just be a color, but we don't have a skinned color-only prog
		if( surf->jointCache )
		{
			renderProgManager.BindShader_TextureVertexColorSkinned();
		}
		else
		{
			renderProgManager.BindShader_TextureVertexColor();
		}

		// draw it solid
		DrawElementsWithCounters( surf );
	}

	GL_State( GLS_DEPTHFUNC_ALWAYS );

	// copy off the color buffer and the depth buffer for the motion blur prog
	// we use the viewport dimensions for copying the buffers in case resolution scaling is enabled.
	const idScreenRect& viewport = viewDef->viewport;
	globalImages->currentRenderImage->CopyFramebuffer( viewport.x1, viewport.y1, viewport.GetWidth(), viewport.GetHeight() );

	// in stereo rendering, each eye needs to get a separate previous frame mvp
	int mvpIndex = ( viewDef->renderView.viewEyeBuffer == 1 ) ? 1 : 0;

	// derive the matrix to go from current pixels to previous frame pixels
	idRenderMatrix	inverseMVP;
	idRenderMatrix::Inverse( viewDef->worldSpace.mvp, inverseMVP );

	idRenderMatrix	motionMatrix;
	idRenderMatrix::Multiply( prevMVP[mvpIndex], inverseMVP, motionMatrix );

	prevMVP[mvpIndex] = viewDef->worldSpace.mvp;

	RB_SetMVP( motionMatrix );

	GL_State( GLS_DEPTHFUNC_ALWAYS | GLS_CULL_TWOSIDED );

	renderProgManager.BindShader_MotionBlur();

	// let the fragment program know how many samples we are going to use
	idVec4 samples( ( float )( 1 << r_motionBlur.GetInteger() ) );
	SetFragmentParm( RENDERPARM_OVERBRIGHT, samples.ToFloatPtr() );

	GL_SelectTexture( 0 );
	globalImages->currentRenderImage->Bind();

	GL_SelectTexture( 1 );
	globalImages->currentDepthImage->Bind();

	DrawElementsWithCounters( &unitSquareSurface );
}

/*
==================
idRenderBackend::DrawView

StereoEye will always be 0 in mono modes, or -1 / 1 in stereo modes.
If the view is a GUI view that is repeated for both eyes, the viewDef.stereoEye value
is 0, so the stereoEye parameter is not always the same as that.
==================
*/
void idRenderBackend::DrawView( const void* data, const int stereoEye )
{
	//OPTICK_EVENT( "Backend_DrawView" );

	const drawSurfsCommand_t* cmd = ( const drawSurfsCommand_t* )data;

	viewDef = cmd->viewDef;

	// we will need to do a new copyTexSubImage of the screen
	// when a SS_POST_PROCESS material is used
	currentRenderCopied = false;

	// if there aren't any drawsurfs, do nothing
	if( !viewDef->numDrawSurfs )
	{
		if( viewDef->renderView.rdflags & RDF_IRRADIANCE )
		{
			nvrhi::utils::ClearColorAttachment( commandList, globalFramebuffers.envprobeFBO->GetApiObject(), 0, nvrhi::Color( 0.f ) );
		}

		return;
	}

	// skip render bypasses everything that has models, assuming
	// them to be 3D views, but leaves 2D rendering visible
	if( r_skipRender.GetBool() && viewDef->viewEntitys )
	{
		return;
	}

	pc.c_surfaces += viewDef->numDrawSurfs;

	DBG_ShowOverdraw();

	// render the scene
	DrawViewInternal( cmd->viewDef, stereoEye );

	// RB: Support motion blur in the future again?
	// It is the worst thing next to depth of field
	//MotionBlur();

	// optionally draw a box colored based on the eye number
	if( r_drawEyeColor.GetBool() )
	{
		const idScreenRect& r = viewDef->viewport;
		GL_Scissor( ( r.x1 + r.x2 ) / 2, ( r.y1 + r.y2 ) / 2, 32, 32 );
		switch( stereoEye )
		{
			case -1:
				GL_Clear( true, false, false, 0, 1.0f, 0.0f, 0.0f, 1.0f );
				break;
			case 1:
				GL_Clear( true, false, false, 0, 0.0f, 1.0f, 0.0f, 1.0f );
				break;
			default:
				GL_Clear( true, false, false, 0, 0.5f, 0.5f, 0.5f, 1.0f );
				break;
		}
	}
}

/*
==================
idRenderBackend::CopyRender

Copy part of the current framebuffer to an image
==================
*/
void idRenderBackend::CopyRender( const void* data )
{
	const copyRenderCommand_t* cmd = ( const copyRenderCommand_t* )data;

	if( r_skipCopyTexture.GetBool() )
	{
		return;
	}

	renderLog.OpenBlock( "***************** RB_CopyRender *****************" );

	if( cmd->image )
	{
		cmd->image->CopyFramebuffer( cmd->x, cmd->y, cmd->imageWidth, cmd->imageHeight );
	}

	if( cmd->clearColorAfterCopy )
	{
		GL_Clear( true, false, false, STENCIL_SHADOW_TEST_VALUE, 0, 0, 0, 0 );
	}

	renderLog.CloseBlock();
}

/*
==================
idRenderBackend::PostProcess

==================
*/
extern idCVar rs_enable;
void idRenderBackend::PostProcess( const void* data )
{
	// only do the post process step if resolution scaling is enabled. Prevents the unnecessary copying of the framebuffer and
	// corresponding full screen quad pass.
	/*
	if( rs_enable.GetInteger() == 0 && !r_useFilmicPostProcessing.GetBool() && r_antiAliasing.GetInteger() == 0 )
	{
		return;
	}
	*/

	if( viewDef->renderView.rdflags & RDF_IRRADIANCE )
	{
#if defined( USE_NVRHI )
		// we haven't changed ldrImage so it's basically the previewsRenderLDR
		BlitParameters blitParms;
		blitParms.sourceTexture = ( nvrhi::ITexture* )globalImages->ldrImage->GetTextureID();
		blitParms.targetFramebuffer = deviceManager->GetCurrentFramebuffer();
		blitParms.targetViewport = nvrhi::Viewport( renderSystem->GetWidth(), renderSystem->GetHeight() );
		commonPasses.BlitTexture( commandList, blitParms, &bindingCache );

		blitParms.sourceTexture = ( nvrhi::ITexture* )globalImages->envprobeHDRImage->GetTextureID();
		blitParms.targetFramebuffer = deviceManager->GetCurrentFramebuffer();
		blitParms.targetViewport = nvrhi::Viewport( ENVPROBE_CAPTURE_SIZE, ENVPROBE_CAPTURE_SIZE );
		commonPasses.BlitTexture( commandList, blitParms, &bindingCache );
#endif
		return;
	}

	renderLog.OpenMainBlock( MRB_POSTPROCESS );
	renderLog.OpenBlock( "Render_PostProcessing", colorBlue );

	// resolve the scaled rendering to a temporary texture
	postProcessCommand_t* cmd = ( postProcessCommand_t* )data;
	const idScreenRect& viewport = cmd->viewDef->viewport;

	GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO | GLS_DEPTHMASK | GLS_DEPTHFUNC_ALWAYS |  GLS_CULL_TWOSIDED );

	int screenWidth = renderSystem->GetWidth();
	int screenHeight = renderSystem->GetHeight();

	// set the window clipping
	GL_Viewport( 0, 0, screenWidth, screenHeight );
	GL_Scissor( 0, 0, screenWidth, screenHeight );

#if 0
	// SMAA
	int aaMode = r_antiAliasing.GetInteger();
	if( aaMode == ANTI_ALIASING_SMAA_1X )
	{
		/*
		 * The shader has three passes, chained together as follows:
		 *
		 *                           |input|------------------�
		 *                              v                     |
		 *                    [ SMAA*EdgeDetection ]          |
		 *                              v                     |
		 *                          |edgesTex|                |
		 *                              v                     |
		 *              [ SMAABlendingWeightCalculation ]     |
		 *                              v                     |
		 *                          |blendTex|                |
		 *                              v                     |
		 *                [ SMAANeighborhoodBlending ] <------�
		 *                              v
		 *                           |output|
		*/

		globalImages->smaaInputImage->CopyFramebuffer( 0, 0, screenWidth, screenHeight );

		// set SMAA_RT_METRICS = rpScreenCorrectionFactor
		float screenCorrectionParm[4];
		screenCorrectionParm[0] = 1.0f / screenWidth;
		screenCorrectionParm[1] = 1.0f / screenHeight;
		screenCorrectionParm[2] = screenWidth;
		screenCorrectionParm[3] = screenHeight;
		SetFragmentParm( RENDERPARM_SCREENCORRECTIONFACTOR, screenCorrectionParm ); // rpScreenCorrectionFactor

		globalFramebuffers.smaaEdgesFBO->Bind();

		glClearColor( 0, 0, 0, 0 );
		glClear( GL_COLOR_BUFFER_BIT );

		GL_SelectTexture( 0 );
		globalImages->smaaInputImage->Bind();

		renderProgManager.BindShader_SMAA_EdgeDetection();
		DrawElementsWithCounters( &unitSquareSurface );

#if 1
		//globalImages->smaaEdgesImage->CopyFramebuffer( viewport.x1, viewport.y1, viewport.GetWidth(), viewport.GetHeight() );

		globalFramebuffers.smaaBlendFBO->Bind();
		//Framebuffer::Unbind();

		glClear( GL_COLOR_BUFFER_BIT );

		GL_SelectTexture( 0 );
		globalImages->smaaEdgesImage->Bind();

		GL_SelectTexture( 1 );
		globalImages->smaaAreaImage->Bind();

		GL_SelectTexture( 2 );
		globalImages->smaaSearchImage->Bind();

		renderProgManager.BindShader_SMAA_BlendingWeightCalculation();
		DrawElementsWithCounters( &unitSquareSurface );

		Framebuffer::Unbind();
#endif

#if 1
		//GL_SelectTexture( 0 );
		//globalImages->smaaBlendImage->CopyFramebuffer( viewport.x1, viewport.y1, viewport.GetWidth(), viewport.GetHeight() );

		GL_SelectTexture( 0 );
		globalImages->smaaInputImage->Bind();

		GL_SelectTexture( 1 );
		globalImages->smaaBlendImage->Bind();

		renderProgManager.BindShader_SMAA_NeighborhoodBlending();
		DrawElementsWithCounters( &unitSquareSurface );
#endif
	}
#endif

	if( r_useFilmicPostProcessing.GetBool() )
	{
#if defined( USE_NVRHI )
		BlitParameters blitParms;
		blitParms.sourceTexture = ( nvrhi::ITexture* )globalImages->ldrImage->GetTextureID();
		blitParms.targetFramebuffer = globalFramebuffers.smaaBlendFBO->GetApiObject();

		blitParms.targetViewport = nvrhi::Viewport( renderSystem->GetWidth(), renderSystem->GetHeight() );
		commonPasses.BlitTexture( commandList, blitParms, &bindingCache );

		GL_SelectTexture( 0 );
		globalImages->smaaBlendImage->Bind();
#else
		globalImages->currentRenderImage->CopyFramebuffer( viewport.x1, viewport.y1, viewport.GetWidth(), viewport.GetHeight() );

		GL_SelectTexture( 0 );
		globalImages->currentRenderImage->Bind();
#endif


		globalFramebuffers.ldrFBO->Bind();

		GL_SelectTexture( 1 );
		globalImages->blueNoiseImage256->Bind();

		renderProgManager.BindShader_PostProcess();

		float jitterTexOffset[4];
		jitterTexOffset[0] = 1.0f / globalImages->blueNoiseImage256->GetUploadWidth();
		jitterTexOffset[1] = 1.0f / globalImages->blueNoiseImage256->GetUploadHeight();

		if( r_shadowMapRandomizeJitter.GetBool() )
		{
			jitterTexOffset[2] = Sys_Milliseconds() / 1000.0f;
			jitterTexOffset[3] = tr.frameCount % 64;
		}
		else
		{
			jitterTexOffset[2] = 0.0f;
			jitterTexOffset[3] = 0.0f;
		}

		SetFragmentParm( RENDERPARM_JITTERTEXOFFSET, jitterTexOffset ); // rpJitterTexOffset

		// Draw
		DrawElementsWithCounters( &unitSquareSurface );
	}

	GL_SelectTexture( 0 );
	renderProgManager.Unbind();

#if defined( USE_NVRHI )
	BlitParameters blitParms;
	blitParms.sourceTexture = ( nvrhi::ITexture* )globalImages->ldrImage->GetTextureID();
	blitParms.targetFramebuffer = deviceManager->GetCurrentFramebuffer();
	blitParms.targetViewport = nvrhi::Viewport( renderSystem->GetWidth(), renderSystem->GetHeight() );
	commonPasses.BlitTexture( commandList, blitParms, &bindingCache );

	GL_SelectTexture( 0 );
	globalImages->currentRenderImage->Bind();
#else
	globalImages->currentRenderImage->CopyFramebuffer( viewport.x1, viewport.y1, viewport.GetWidth(), viewport.GetHeight() );

	GL_SelectTexture( 0 );
	globalImages->currentRenderImage->Bind();
#endif

	renderLog.CloseBlock();
	renderLog.CloseMainBlock();
}
