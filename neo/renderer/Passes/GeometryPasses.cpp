/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
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
#include <precompiled.h>
#pragma hdrstop

#include "renderer/RenderCommon.h"

#include "GeometryPasses.h"

#include "nvrhi/utils.h"

#if 0
static ID_INLINE void SetVertexParm( renderParm_t rp, const float* value )
{
	renderProgManager.SetUniformValue( rp, value );
}

static ID_INLINE void SetVertexParms( renderParm_t rp, const float* value, int num )
{
	for( int i = 0; i < num; i++ )
	{
		renderProgManager.SetUniformValue( ( renderParm_t )( rp + i ), value + ( i * 4 ) );
	}
}

static ID_INLINE void SetFragmentParm( renderParm_t rp, const float* value )
{
	renderProgManager.SetUniformValue( rp, value );
}

static void RB_GetShaderTextureMatrix( const float* shaderRegisters, const textureStage_t* texture, float matrix[16] )
{
	matrix[0 * 4 + 0] = shaderRegisters[texture->matrix[0][0]];
	matrix[1 * 4 + 0] = shaderRegisters[texture->matrix[0][1]];
	matrix[2 * 4 + 0] = 0.0f;
	matrix[3 * 4 + 0] = shaderRegisters[texture->matrix[0][2]];

	matrix[0 * 4 + 1] = shaderRegisters[texture->matrix[1][0]];
	matrix[1 * 4 + 1] = shaderRegisters[texture->matrix[1][1]];
	matrix[2 * 4 + 1] = 0.0f;
	matrix[3 * 4 + 1] = shaderRegisters[texture->matrix[1][2]];

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

static void RB_LoadShaderTextureMatrix( const float* shaderRegisters, const textureStage_t* texture )
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
	}

	SetVertexParm( RENDERPARM_TEXTUREMATRIX_S, texS );
	SetVertexParm( RENDERPARM_TEXTUREMATRIX_T, texT );
}

void IGeometryPass::PrepareStageTexturing( const shaderStage_t* pStage, const drawSurf_t* surf )
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

			RENDERLOG_PRINTF( "TexGen: TG_REFLECT_CUBE: Bumpy Environment\n" );
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
			RENDERLOG_PRINTF( "TexGen: TG_REFLECT_CUBE: Environment\n" );
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

		float wobbleDegrees = surf->shaderRegisters[parms[0]] * ( idMath::PI / 180.0f );
		float wobbleSpeed = surf->shaderRegisters[parms[1]] * ( 2.0f * idMath::PI / 60.0f );
		float rotateSpeed = surf->shaderRegisters[parms[2]] * ( 2.0f * idMath::PI / 60.0f );

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

		RENDERLOG_PRINTF( "TexGen : %s\n", ( pStage->texture.texgen == TG_SCREEN ) ? "TG_SCREEN" : "TG_SCREEN2" );
		renderLog.Indent();

		float plane[4];
		plane[0] = mat[0 * 4 + 0];
		plane[1] = mat[1 * 4 + 0];
		plane[2] = mat[2 * 4 + 0];
		plane[3] = mat[3 * 4 + 0];
		SetVertexParm( RENDERPARM_TEXGEN_0_S, plane );
		RENDERLOG_PRINTF( "TEXGEN_S = %4.3f, %4.3f, %4.3f, %4.3f\n", plane[0], plane[1], plane[2], plane[3] );

		plane[0] = mat[0 * 4 + 1];
		plane[1] = mat[1 * 4 + 1];
		plane[2] = mat[2 * 4 + 1];
		plane[3] = mat[3 * 4 + 1];
		SetVertexParm( RENDERPARM_TEXGEN_0_T, plane );
		RENDERLOG_PRINTF( "TEXGEN_T = %4.3f, %4.3f, %4.3f, %4.3f\n", plane[0], plane[1], plane[2], plane[3] );

		plane[0] = mat[0 * 4 + 3];
		plane[1] = mat[1 * 4 + 3];
		plane[2] = mat[2 * 4 + 3];
		plane[3] = mat[3 * 4 + 3];
		SetVertexParm( RENDERPARM_TEXGEN_0_Q, plane );
		RENDERLOG_PRINTF( "TEXGEN_Q = %4.3f, %4.3f, %4.3f, %4.3f\n", plane[0], plane[1], plane[2], plane[3] );

		renderLog.Outdent();

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

void IGeometryPass::FinishStageTexturing( const shaderStage_t* stage, const drawSurf_t* surf )
{
	if( stage->texture.cinematic )
	{
		// unbind the extra bink textures
		GL_SelectTexture( 0 );
	}

	if( stage->texture.texgen == TG_REFLECT_CUBE )
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

bool IGeometryPass::GL_State( uint64 stateBits, bool forceGlState )
{
	uint64 diff = stateBits ^ glStateBits;

	if( !r_useStateCaching.GetBool() || forceGlState )
	{
		// make sure everything is set all the time, so we
		// can see if our delta checking is screwing up
		diff = 0xFFFFFFFFFFFFFFFF;
	}
	else if( diff == 0 )
	{
		return false;
	}

	// Reset pipeline
	auto& currentBlendState = pipelineDesc.renderState.blendState;
	auto& currentDepthStencilState = pipelineDesc.renderState.depthStencilState;
	auto& currentRasterState = pipelineDesc.renderState.rasterState;

	//
	// culling
	//
	if( diff & ( GLS_CULL_BITS ) )//| GLS_MIRROR_VIEW ) )
	{
		switch( stateBits & GLS_CULL_BITS )
		{
			case GLS_CULL_TWOSIDED:
				currentRasterState.setCullNone();
				break;

			case GLS_CULL_BACKSIDED:
				if( viewDef != NULL && viewDef->isMirror )
				{
					stateBits |= GLS_MIRROR_VIEW;
					currentRasterState.setCullFront();
				}
				else
				{
					currentRasterState.setCullBack();
				}
				break;

			case GLS_CULL_FRONTSIDED:
			default:
				if( viewDef != NULL && viewDef->isMirror )
				{
					stateBits |= GLS_MIRROR_VIEW;
					currentRasterState.setCullBack();
				}
				else
				{
					currentRasterState.setCullFront();
				}
				break;
		}
	}

	//
	// check depthFunc bits
	//
	if( diff & GLS_DEPTHFUNC_BITS )
	{
		switch( stateBits & GLS_DEPTHFUNC_BITS )
		{
			case GLS_DEPTHFUNC_EQUAL:
				currentDepthStencilState.depthFunc = nvrhi::ComparisonFunc::Equal;
				break;
			case GLS_DEPTHFUNC_ALWAYS:
				currentDepthStencilState.depthFunc = nvrhi::ComparisonFunc::Always;
				break;
			case GLS_DEPTHFUNC_LESS:
				currentDepthStencilState.depthFunc = nvrhi::ComparisonFunc::Less;
				break;
			case GLS_DEPTHFUNC_GREATER:
				currentDepthStencilState.depthFunc = nvrhi::ComparisonFunc::Greater;
				break;
		}
	}

	nvrhi::BlendState::RenderTarget renderTarget;

	//
	// check blend bits
	//
	if( diff & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) )
	{
		nvrhi::BlendFactor srcFactor = nvrhi::BlendFactor::One;
		nvrhi::BlendFactor dstFactor = nvrhi::BlendFactor::One;

		switch( stateBits & GLS_SRCBLEND_BITS )
		{
			case GLS_SRCBLEND_ZERO:
				srcFactor = nvrhi::BlendFactor::Zero;
				break;
			case GLS_SRCBLEND_ONE:
				srcFactor = nvrhi::BlendFactor::One;
				break;
			case GLS_SRCBLEND_DST_COLOR:
				srcFactor = nvrhi::BlendFactor::DstColor;
				break;
			case GLS_SRCBLEND_ONE_MINUS_DST_COLOR:
				srcFactor = nvrhi::BlendFactor::OneMinusDstColor;
				break;
			case GLS_SRCBLEND_SRC_ALPHA:
				srcFactor = nvrhi::BlendFactor::SrcAlpha;
				break;
			case GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA:
				srcFactor = nvrhi::BlendFactor::OneMinusSrcAlpha;
				break;
			case GLS_SRCBLEND_DST_ALPHA:
				srcFactor = nvrhi::BlendFactor::DstAlpha;
				break;
			case GLS_SRCBLEND_ONE_MINUS_DST_ALPHA:
				srcFactor = nvrhi::BlendFactor::OneMinusDstAlpha;
				break;
			default:
				assert( !"GL_State: invalid src blend state bits\n" );
				break;
		}

		switch( stateBits & GLS_DSTBLEND_BITS )
		{
			case GLS_DSTBLEND_ZERO:
				dstFactor = nvrhi::BlendFactor::Zero;
				break;
			case GLS_DSTBLEND_ONE:
				dstFactor = nvrhi::BlendFactor::One;
				break;
			case GLS_DSTBLEND_SRC_COLOR:
				dstFactor = nvrhi::BlendFactor::SrcColor;
				break;
			case GLS_DSTBLEND_ONE_MINUS_SRC_COLOR:
				dstFactor = nvrhi::BlendFactor::OneMinusSrcColor;
				break;
			case GLS_DSTBLEND_SRC_ALPHA:
				dstFactor = nvrhi::BlendFactor::SrcAlpha;
				break;
			case GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA:
				dstFactor = nvrhi::BlendFactor::OneMinusSrcAlpha;
				break;
			case GLS_DSTBLEND_DST_ALPHA:
				dstFactor = nvrhi::BlendFactor::DstAlpha;
				break;
			case GLS_DSTBLEND_ONE_MINUS_DST_ALPHA:
				dstFactor = nvrhi::BlendFactor::OneMinusDstAlpha;
				break;
			default:
				assert( !"GL_State: invalid dst blend state bits\n" );
				break;
		}

		// Only actually update GL's blend func if blending is enabled.
		if( srcFactor == nvrhi::BlendFactor::One && dstFactor == nvrhi::BlendFactor::Zero )
		{
			renderTarget.disableBlend();
		}
		else
		{
			currentBlendState.setAlphaToCoverageEnable( true );
			nvrhi::BlendState::RenderTarget renderTarget;
			renderTarget.enableBlend();
			renderTarget.setSrcBlend( srcFactor );
			renderTarget.setDestBlend( dstFactor );
		}
	}

	//
	// check depthmask
	//
	if( diff & GLS_DEPTHMASK )
	{
		if( stateBits & GLS_DEPTHMASK )
		{
			currentDepthStencilState.disableDepthWrite();
			currentDepthStencilState.disableDepthTest();
		}
		else
		{
			currentDepthStencilState.enableDepthWrite();
			currentDepthStencilState.enableDepthTest();
		}
	}

	//
	// check colormask
	//
	if( diff & ( GLS_REDMASK | GLS_GREENMASK | GLS_BLUEMASK | GLS_ALPHAMASK ) )
	{
		nvrhi::ColorMask mask{ nvrhi::ColorMask::All };

		if( stateBits & GLS_REDMASK )
		{
			mask = mask & ~nvrhi::ColorMask::Red;
		}
		if( stateBits & GLS_GREENMASK )
		{
			mask = mask & ~nvrhi::ColorMask::Green;
		}
		if( stateBits & GLS_BLUEMASK )
		{
			mask = mask & ~nvrhi::ColorMask::Blue;
		}
		if( stateBits & GLS_ALPHAMASK )
		{
			mask = mask & ~nvrhi::ColorMask::Alpha;
		}

		renderTarget.setColorWriteMask( mask );
	}

	currentBlendState.setRenderTarget( 0, renderTarget );

	//
	// fill/line mode
	//
	if( diff & GLS_POLYMODE_LINE )
	{
		if( stateBits & GLS_POLYMODE_LINE )
		{
			currentRasterState.setFillMode( nvrhi::RasterFillMode::Line );
			currentRasterState.setCullNone();
		}
		else
		{
			currentRasterState.setCullNone();
			currentRasterState.setFillMode( nvrhi::RasterFillMode::Fill );
		}
	}

	//
	// polygon offset
	//
	if( diff & GLS_POLYGON_OFFSET )
	{
		if( stateBits & GLS_POLYGON_OFFSET )
		{
			currentRasterState.enableQuadFill();
		}
		else
		{
			currentRasterState.disableQuadFill();
		}
	}

	nvrhi::DepthStencilState::StencilOpDesc stencilOp;

	//
	// stencil
	//
	if( diff & ( GLS_STENCIL_FUNC_BITS | GLS_STENCIL_OP_BITS ) )
	{
		if( ( stateBits & ( GLS_STENCIL_FUNC_BITS | GLS_STENCIL_OP_BITS ) ) != 0 )
		{
			currentDepthStencilState.enableStencil();
			currentDepthStencilState.enableDepthWrite();
		}
		else
		{
			currentDepthStencilState.disableStencil();
			//currentDepthStencilState.disableDepthWrite();
		}
	}
	if( diff & ( GLS_STENCIL_FUNC_BITS | GLS_STENCIL_FUNC_REF_BITS | GLS_STENCIL_FUNC_MASK_BITS ) )
	{
		GLuint ref = GLuint( ( stateBits & GLS_STENCIL_FUNC_REF_BITS ) >> GLS_STENCIL_FUNC_REF_SHIFT );
		GLuint mask = GLuint( ( stateBits & GLS_STENCIL_FUNC_MASK_BITS ) >> GLS_STENCIL_FUNC_MASK_SHIFT );
		GLenum func = 0;

		currentDepthStencilState.setStencilRefValue( ( stateBits & GLS_STENCIL_FUNC_REF_BITS ) >> GLS_STENCIL_FUNC_REF_SHIFT );
		currentDepthStencilState.setStencilReadMask( ( stateBits & GLS_STENCIL_FUNC_MASK_BITS ) >> GLS_STENCIL_FUNC_MASK_SHIFT );
		currentDepthStencilState.setStencilWriteMask( ( stateBits & GLS_STENCIL_FUNC_MASK_BITS ) >> GLS_STENCIL_FUNC_MASK_SHIFT );

		switch( stateBits & GLS_STENCIL_FUNC_BITS )
		{
			case GLS_STENCIL_FUNC_NEVER:
				stencilOp.setStencilFunc( nvrhi::ComparisonFunc::Never );
				break;
			case GLS_STENCIL_FUNC_LESS:
				stencilOp.setStencilFunc( nvrhi::ComparisonFunc::Less );
				break;
			case GLS_STENCIL_FUNC_EQUAL:
				stencilOp.setStencilFunc( nvrhi::ComparisonFunc::Equal );
				break;
			case GLS_STENCIL_FUNC_LEQUAL:
				stencilOp.setStencilFunc( nvrhi::ComparisonFunc::LessOrEqual );
				break;
			case GLS_STENCIL_FUNC_GREATER:
				stencilOp.setStencilFunc( nvrhi::ComparisonFunc::Greater );
				break;
			case GLS_STENCIL_FUNC_NOTEQUAL:
				stencilOp.setStencilFunc( nvrhi::ComparisonFunc::NotEqual );
				break;
			case GLS_STENCIL_FUNC_GEQUAL:
				stencilOp.setStencilFunc( nvrhi::ComparisonFunc::GreaterOrEqual );
				break;
			case GLS_STENCIL_FUNC_ALWAYS:
				stencilOp.setStencilFunc( nvrhi::ComparisonFunc::Always );
				break;
		}
	}
	if( diff & ( GLS_STENCIL_OP_FAIL_BITS | GLS_STENCIL_OP_ZFAIL_BITS | GLS_STENCIL_OP_PASS_BITS ) )
	{
		GLenum sFail = 0;
		GLenum zFail = 0;
		GLenum pass = 0;

		switch( stateBits & GLS_STENCIL_OP_FAIL_BITS )
		{
			case GLS_STENCIL_OP_FAIL_KEEP:
				stencilOp.setFailOp( nvrhi::StencilOp::Keep );
				break;
			case GLS_STENCIL_OP_FAIL_ZERO:
				stencilOp.setFailOp( nvrhi::StencilOp::Zero );
				break;
			case GLS_STENCIL_OP_FAIL_REPLACE:
				stencilOp.setFailOp( nvrhi::StencilOp::Replace );
				break;
			case GLS_STENCIL_OP_FAIL_INCR:
				stencilOp.setFailOp( nvrhi::StencilOp::IncrementAndClamp );
				break;
			case GLS_STENCIL_OP_FAIL_DECR:
				stencilOp.setFailOp( nvrhi::StencilOp::DecrementAndClamp );
				break;
			case GLS_STENCIL_OP_FAIL_INVERT:
				stencilOp.setFailOp( nvrhi::StencilOp::Invert );
				break;
			case GLS_STENCIL_OP_FAIL_INCR_WRAP:
				stencilOp.setFailOp( nvrhi::StencilOp::IncrementAndWrap );
				break;
			case GLS_STENCIL_OP_FAIL_DECR_WRAP:
				stencilOp.setFailOp( nvrhi::StencilOp::DecrementAndWrap );
				break;
		}
		switch( stateBits & GLS_STENCIL_OP_ZFAIL_BITS )
		{
			case GLS_STENCIL_OP_ZFAIL_KEEP:
				stencilOp.setDepthFailOp( nvrhi::StencilOp::Keep );
				break;
			case GLS_STENCIL_OP_ZFAIL_ZERO:
				stencilOp.setDepthFailOp( nvrhi::StencilOp::Zero );
				break;
			case GLS_STENCIL_OP_ZFAIL_REPLACE:
				stencilOp.setDepthFailOp( nvrhi::StencilOp::Replace );
				break;
			case GLS_STENCIL_OP_ZFAIL_INCR:
				stencilOp.setDepthFailOp( nvrhi::StencilOp::IncrementAndClamp );
				break;
			case GLS_STENCIL_OP_ZFAIL_DECR:
				stencilOp.setDepthFailOp( nvrhi::StencilOp::DecrementAndClamp );
				break;
			case GLS_STENCIL_OP_ZFAIL_INVERT:
				stencilOp.setDepthFailOp( nvrhi::StencilOp::Invert );
				break;
			case GLS_STENCIL_OP_ZFAIL_INCR_WRAP:
				stencilOp.setDepthFailOp( nvrhi::StencilOp::IncrementAndWrap );
				break;
			case GLS_STENCIL_OP_ZFAIL_DECR_WRAP:
				stencilOp.setDepthFailOp( nvrhi::StencilOp::DecrementAndWrap );
				break;
		}
		switch( stateBits & GLS_STENCIL_OP_PASS_BITS )
		{
			case GLS_STENCIL_OP_PASS_KEEP:
				stencilOp.setPassOp( nvrhi::StencilOp::Keep );
				break;
			case GLS_STENCIL_OP_PASS_ZERO:
				stencilOp.setPassOp( nvrhi::StencilOp::Zero );
				break;
			case GLS_STENCIL_OP_PASS_REPLACE:
				stencilOp.setPassOp( nvrhi::StencilOp::Replace );
				break;
			case GLS_STENCIL_OP_PASS_INCR:
				stencilOp.setPassOp( nvrhi::StencilOp::IncrementAndClamp );
				break;
			case GLS_STENCIL_OP_PASS_DECR:
				stencilOp.setPassOp( nvrhi::StencilOp::DecrementAndClamp );
				break;
			case GLS_STENCIL_OP_PASS_INVERT:
				stencilOp.setPassOp( nvrhi::StencilOp::Invert );
				break;
			case GLS_STENCIL_OP_PASS_INCR_WRAP:
				stencilOp.setPassOp( nvrhi::StencilOp::IncrementAndWrap );
				break;
			case GLS_STENCIL_OP_PASS_DECR_WRAP:
				stencilOp.setPassOp( nvrhi::StencilOp::DecrementAndWrap );
				break;
		}
	}

	currentDepthStencilState.setFrontFaceStencil( stencilOp );

	glStateBits = stateBits;
	pipeline = nullptr;
}

void IGeometryPass::GL_SelectTexture( int imageParm )
{
	currentImageParm = imageParm;
}

void IGeometryPass::GL_BindTexture( idImage* img )
{
	imageParms[currentImageParm] = img;
}

void IGeometryPass::GL_BindFramebuffer( Framebuffer* framebuffer )
{
	if( currentFramebuffer != framebuffer )
	{
		previousFramebuffer = currentFramebuffer;
		pipeline = nullptr;
	}
	currentFramebuffer = framebuffer;
}

void IGeometryPass::GL_BindGraphicsShader( int shaderIndex )
{
	nvrhi::ShaderHandle shader = renderProgManager.GetShader( shaderIndex );
	if( shader->getDesc().shaderType == nvrhi::ShaderType::Vertex )
	{
		if( pipelineDesc.VS != shader )
		{
			pipeline = nullptr;
		}

		pipelineDesc.setVertexShader( shader );
	}

	if( shader->getDesc().shaderType == nvrhi::ShaderType::Pixel )
	{
		if( pipelineDesc.PS != shader )
		{
			pipeline = nullptr;
		}

		pipelineDesc.setPixelShader( shader );
	}
}

void IGeometryPass::GL_DepthBoundsTest( const float zmin, const float zmax )
{
}

void IGeometryPass::GL_PolygonOffset( float scale, float bias )
{
	pipelineDesc.renderState.rasterState.setSlopeScaleDepthBias( scale ).setDepthBias( bias );
	pipeline = nullptr;
}

void IGeometryPass::GL_Viewport( int x, int y, int w, int h )
{
	currentViewport.Clear();
	currentViewport.AddPoint( x, y );
	currentViewport.AddPoint( x + w, y + h );
}

void IGeometryPass::GL_Scissor( int x, int y, int w, int h )
{
	currentScissor.Clear();
	currentScissor.AddPoint( x, y );
	currentScissor.AddPoint( x + w, y + h );
}

void IGeometryPass::GL_Color( const idVec4 color )
{
	// TODO(Stephen): Hold local copy and then set in a constant buffer later.
	float parm[4];
	parm[0] = idMath::ClampFloat( 0.0f, 1.0f, color.x );
	parm[1] = idMath::ClampFloat( 0.0f, 1.0f, color.y );
	parm[2] = idMath::ClampFloat( 0.0f, 1.0f, color.z );
	parm[3] = idMath::ClampFloat( 0.0f, 1.0f, color.w );
	renderProgManager.SetRenderParm( RENDERPARM_COLOR, parm );
}

void IGeometryPass::GL_ClearColor( const idVec4 color )
{
	clearColor = color;
}

void IGeometryPass::GL_ClearDepthStencilValue( float depthValue, byte stencilValue )
{
	depthClearValue = depthValue;
	stencilClearValue = stencilValue;
}

void IGeometryPass::GL_ClearColor( nvrhi::ICommandList* commandList, int attachmentIndex )
{
	nvrhi::utils::ClearColorAttachment(
		commandList,
		currentFramebuffer->GetApiObject(),
		attachmentIndex,
		nvrhi::Color( clearColor.x, clearColor.y, clearColor.z, clearColor.w ) );
}

void IGeometryPass::GL_ClearDepthStencil( nvrhi::ICommandList* commandList )
{
	nvrhi::utils::ClearDepthStencilAttachment( commandList, currentFramebuffer->GetApiObject(), depthClearValue, stencilClearValue );
}
#endif