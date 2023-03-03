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

#include "GBufferFillPass.h"

#if 0

void GBufferFillPass::Init( nvrhi::DeviceHandle deviceHandle )
{
}

#if 0
void GBufferFillPass::RenderView( nvrhi::ICommandList* commandList, const drawSurf_t* const* drawSurfs, int numDrawSurfs, bool fillGbuffer )
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

#if defined( USE_VULKAN )
	if( fillGbuffer )
	{
		return;
	}
#endif


	/*
	if( !fillGbuffer )
	{
		// clear gbuffer
		GL_Clear( true, false, false, 0, 0.0f, 0.0f, 0.0f, 1.0f, false );
	}
	*/

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
		RB_SetMVP( renderMatrix_identity );

		renderProgManager.BindShader_Texture();
		GL_Color( idVec4( 1 ) );

		GL_SelectTexture( 0 );
		globalImages->ambientOcclusionImage[0]->Bind();

		DrawElementsWithCounters( &tr.backend.unitSquareSurface );

		renderProgManager.Unbind();
		GL_State( GLS_DEFAULT );

		renderProgManager.SetRenderParm( RENDERPARM_ALPHA_TEST, vec4_zero.ToFloatPtr() );

		return;
	}

	renderLog.OpenMainBlock( fillGbuffer ? MRB_FILL_GEOMETRY_BUFFER : MRB_AMBIENT_PASS );
	renderLog.OpenBlock( fillGbuffer ? "Fill_GeometryBuffer" : "Render_AmbientPass", colorBlue );

	if( fillGbuffer )
	{
		globalFramebuffers.geometryBufferFBO->Bind();

		GL_Clear( true, false, false, 0, 0.0f, 0.0f, 0.0f, 1.0f, false );
	}

	commandList->setEnableAutomaticBarriers( false );
	commandList->setResourceStatesForFramebuffer( currentFrameBuffer->GetApiObject() );
	commandList->commitBarriers();

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
			if( surfaceRegs[pStage->conditionRegister] != 0 )
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
			else
			{

				// TODO support PBR textures

				// draw Quake 4 style ambient
				if( drawSurf->jointCache )
				{
					renderProgManager.BindShader_AmbientLightingSkinned();
				}
				else
				{
					renderProgManager.BindShader_AmbientLighting();
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
					if( !surfaceRegs[surfaceStage->conditionRegister] )
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
					if( !surfaceRegs[surfaceStage->conditionRegister] )
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
				case SL_RMAO:
				{
					// ignore stage that fails the condition
					if( !surfaceRegs[surfaceStage->conditionRegister] )
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

nvrhi::GraphicsPipelineHandle GBufferFillPass::CreateGraphicsPipeline( nvrhi::IFramebuffer* framebuffer )
{
	return nvrhi::GraphicsPipelineHandle();
}

void GBufferFillPass::DrawElementsWithCounters( const drawSurf_t* surf )
{
	// Get vertex buffer
	const vertCacheHandle_t vbHandle = surf->ambientCache;
	idVertexBuffer* vertexBuffer;
	if( vertexCache.CacheIsStatic( vbHandle ) )
	{
		vertexBuffer = &vertexCache.staticData.vertexBuffer;
	}
	else
	{
		const uint64 frameNum = ( int )( vbHandle >> VERTCACHE_FRAME_SHIFT ) & VERTCACHE_FRAME_MASK;
		if( frameNum != ( ( vertexCache.currentFrame - 1 ) & VERTCACHE_FRAME_MASK ) )
		{
			idLib::Warning( "RB_DrawElementsWithCounters, vertexBuffer == NULL" );
			return;
		}
		vertexBuffer = &vertexCache.frameData[vertexCache.drawListNum].vertexBuffer;
	}
	const uint vertOffset = ( uint )( vbHandle >> VERTCACHE_OFFSET_SHIFT ) & VERTCACHE_OFFSET_MASK;
	auto currentVertexBuffer = vertexBuffer->GetAPIObject();

	// Get index buffer
	const vertCacheHandle_t ibHandle = surf->indexCache;
	idIndexBuffer* indexBuffer;
	if( vertexCache.CacheIsStatic( ibHandle ) )
	{
		indexBuffer = &vertexCache.staticData.indexBuffer;
	}
	else
	{
		const uint64 frameNum = ( int )( ibHandle >> VERTCACHE_FRAME_SHIFT ) & VERTCACHE_FRAME_MASK;
		if( frameNum != ( ( vertexCache.currentFrame - 1 ) & VERTCACHE_FRAME_MASK ) )
		{
			idLib::Warning( "RB_DrawElementsWithCounters, indexBuffer == NULL" );
			return;
		}
		indexBuffer = &vertexCache.frameData[vertexCache.drawListNum].indexBuffer;
	}
	const uint indexOffset = ( uint )( ibHandle >> VERTCACHE_OFFSET_SHIFT ) & VERTCACHE_OFFSET_MASK;

	auto currentIndexBuffer = ( nvrhi::IBuffer* )indexBuffer->GetAPIObject();

	if( currentIndexBuffer != ( nvrhi::IBuffer* )indexBuffer->GetAPIObject() || !r_useStateCaching.GetBool() )
	{
		currentIndexBuffer = indexBuffer->GetAPIObject();
	}

	if( !pipeline )
	{
		nvrhi::GraphicsPipelineDesc psoDesc;
		psoDesc.VS = vertexShader;
		psoDesc.PS = pixelShader;
		psoDesc.inputLayout = inputLayout;
		psoDesc.bindingLayouts = { currentBindingLayout };
		psoDesc.primType = nvrhi::PrimitiveType::TriangleList;
		currentRenderState.rasterState.enableScissor();
		psoDesc.setRenderState( currentRenderState );

		pipeline = device->createGraphicsPipeline( psoDesc, currentFramebuffer->GetApiObject() );
	}

	nvrhi::BindingSetDesc bindingSetDesc;

	if( renderProgManager.BindingLayoutType() == BINDING_LAYOUT_DEFAULT )
	{
		bindingSetDesc
		.addItem( nvrhi::BindingSetItem::ConstantBuffer( 0, renderProgManager.ConstantBuffer() ) )
		.addItem( nvrhi::BindingSetItem::Texture_SRV( 0, ( nvrhi::ITexture* )GetImageAt( 0 )->GetTextureID() ) )
		.addItem( nvrhi::BindingSetItem::Sampler( 0, ( nvrhi::ISampler* )GetImageAt( 0 )->GetSampler() ) );
	}
	else if( renderProgManager.BindingLayoutType() == BINDING_LAYOUT_GBUFFER )
	{
		bindingSetDesc
		.addItem( nvrhi::BindingSetItem::ConstantBuffer( 0, renderProgManager.ConstantBuffer() ) );
	}
	else if( renderProgManager.BindingLayoutType() == BINDING_LAYOUT_LIGHTGRID )
	{
		bindingSetDesc
		.addItem( nvrhi::BindingSetItem::ConstantBuffer( 0, renderProgManager.ConstantBuffer() ) )
		.addItem( nvrhi::BindingSetItem::Texture_SRV( 0, ( nvrhi::ITexture* )GetImageAt( 0 )->GetTextureID() ) )
		.addItem( nvrhi::BindingSetItem::Texture_SRV( 1, ( nvrhi::ITexture* )GetImageAt( 1 )->GetTextureID() ) )
		.addItem( nvrhi::BindingSetItem::Texture_SRV( 2, ( nvrhi::ITexture* )GetImageAt( 2 )->GetTextureID() ) )
		.addItem( nvrhi::BindingSetItem::Texture_SRV( 3, ( nvrhi::ITexture* )GetImageAt( 3 )->GetTextureID() ) )
		.addItem( nvrhi::BindingSetItem::Texture_SRV( 4, ( nvrhi::ITexture* )GetImageAt( 4 )->GetTextureID() ) )
		.addItem( nvrhi::BindingSetItem::Texture_SRV( 7, ( nvrhi::ITexture* )GetImageAt( 7 )->GetTextureID() ) )
		.addItem( nvrhi::BindingSetItem::Texture_SRV( 8, ( nvrhi::ITexture* )GetImageAt( 8 )->GetTextureID() ) )
		.addItem( nvrhi::BindingSetItem::Texture_SRV( 9, ( nvrhi::ITexture* )GetImageAt( 9 )->GetTextureID() ) )
		.addItem( nvrhi::BindingSetItem::Texture_SRV( 10, ( nvrhi::ITexture* )GetImageAt( 10 )->GetTextureID() ) )
		.addItem( nvrhi::BindingSetItem::Sampler( 0, ( nvrhi::ISampler* )GetImageAt( 0 )->GetSampler() ) )
		.addItem( nvrhi::BindingSetItem::Sampler( 1, ( nvrhi::ISampler* )GetImageAt( 1 )->GetSampler() ) )
		.addItem( nvrhi::BindingSetItem::Sampler( 2, ( nvrhi::ISampler* )GetImageAt( 2 )->GetSampler() ) )
		.addItem( nvrhi::BindingSetItem::Sampler( 3, ( nvrhi::ISampler* )GetImageAt( 3 )->GetSampler() ) )
		.addItem( nvrhi::BindingSetItem::Sampler( 4, ( nvrhi::ISampler* )GetImageAt( 4 )->GetSampler() ) )
		.addItem( nvrhi::BindingSetItem::Sampler( 7, ( nvrhi::ISampler* )GetImageAt( 7 )->GetSampler() ) )
		.addItem( nvrhi::BindingSetItem::Sampler( 8, ( nvrhi::ISampler* )GetImageAt( 8 )->GetSampler() ) )
		.addItem( nvrhi::BindingSetItem::Sampler( 9, ( nvrhi::ISampler* )GetImageAt( 9 )->GetSampler() ) )
		.addItem( nvrhi::BindingSetItem::Sampler( 10, ( nvrhi::ISampler* )GetImageAt( 10 )->GetSampler() ) );
	}

	currentBindingSet = bindingCache.GetOrCreateBindingSet( bindingSetDesc, currentBindingLayout );

	renderProgManager.CommitConstantBuffer( commandList );

	nvrhi::GraphicsState state;
	state.bindings = { currentBindingSet };
	state.indexBuffer = { currentIndexBuffer, nvrhi::Format::R16_UINT, indexOffset };
	state.vertexBuffers = { { currentVertexBuffer, 0, vertOffset } };
	state.pipeline = currentPipeline;
	state.framebuffer = currentFrameBuffer->GetApiObject();

	// TODO(Stephen): use currentViewport instead.
	nvrhi::Viewport viewport;
	viewport.minX = currentViewport.x1;
	viewport.minY = currentViewport.y1;
	viewport.maxX = currentViewport.x2;
	viewport.maxY = currentViewport.y2;
	viewport.minZ = currentViewport.zmin;
	viewport.maxZ = currentViewport.zmax;
	state.viewport.addViewportAndScissorRect( viewport );
	state.viewport.addScissorRect( nvrhi::Rect( currentScissor.x1, currentScissor.y1, currentScissor.x2, currentScissor.y2 ) );

	commandList->setGraphicsState( state );

	nvrhi::DrawArguments args;
	args.vertexCount = surf->numIndexes;
	commandList->drawIndexed( args );

	// RB: added stats
	pc.c_drawElements++;
	pc.c_drawIndexes += surf->numIndexes;
}

#endif

void GBufferFillPass::SetupView( nvrhi::ICommandList* commandList, viewDef_t* viewDef )
{
}

bool GBufferFillPass::SetupMaterial( const idMaterial* material, nvrhi::RasterCullMode cullMode, nvrhi::GraphicsState& state )
{
	return false;
}

void GBufferFillPass::SetupInputBuffers( const drawSurf_t* drawSurf, nvrhi::GraphicsState& state )
{
}

void GBufferFillPass::SetPushConstants( nvrhi::ICommandList* commandList, nvrhi::GraphicsState& state, nvrhi::DrawArguments& args )
{
}

#endif