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

#include "framework/Common_local.h"
#include "renderer/RenderCommon.h"
#include "renderer/Framebuffer.h"

#include "FowardShadingPass.h"

#if 0

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

void ForwardShadingPass::Init( nvrhi::DeviceHandle deviceHandle )
{
	device = deviceHandle;

	auto texturedBindingLayoutDesc = nvrhi::BindingLayoutDesc()
									 .setVisibility( nvrhi::ShaderType::All )
									 .addItem( nvrhi::BindingLayoutItem::VolatileConstantBuffer( 0 ) )
									 .addItem( nvrhi::BindingLayoutItem::Texture_SRV( 0 ) )
									 .addItem( nvrhi::BindingLayoutItem::Sampler( 0 ) );

	auto geometrySkinnedBindingLayoutDesc = nvrhi::BindingLayoutDesc()
											.setVisibility( nvrhi::ShaderType::All )
											.addItem( nvrhi::BindingLayoutItem::VolatileConstantBuffer( 0 ) )
											.addItem( nvrhi::BindingLayoutItem::VolatileConstantBuffer( 1 ) );

	auto geometryBindingLayoutDesc = nvrhi::BindingLayoutDesc()
									 .setVisibility( nvrhi::ShaderType::All )
									 .addItem( nvrhi::BindingLayoutItem::VolatileConstantBuffer( 0 ) );

	texturedBindingLayout = device->createBindingLayout( texturedBindingLayoutDesc );
	geometryBindingLayout = device->createBindingLayout( geometryBindingLayoutDesc );
	pipelineDesc.bindingLayouts = { geometryBindingLayout };

	geometryBindingSetDesc = nvrhi::BindingSetDesc()
							 .addItem( nvrhi::BindingSetItem::ConstantBuffer( 0, renderProgManager.ConstantBuffer() ) );

	samplerCache.Init( deviceHandle.Get() );

	pipeline = nullptr;
}

void ForwardShadingPass::DrawInteractions( nvrhi::ICommandList* commandList, const viewDef_t* viewDef )
{
	if( r_skipInteractions.GetBool() || viewDef->viewLights == NULL )
	{
		return;
	}

	commandList->beginMarker( "DrawInteractions" );

	GL_SelectTexture( 0 );

	const bool useLightDepthBounds = r_useLightDepthBounds.GetBool() && !r_useShadowMapping.GetBool();

	Framebuffer* previousFramebuffer = currentFramebuffer;

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
		commandList->beginMarker( lightShader->GetName() );

		// set the depth bounds for the whole light
		if( useLightDepthBounds )
		{
			GL_DepthBoundsTest( vLight->scissorRect.zmin, vLight->scissorRect.zmax );
		}

		// RB: shadow mapping
		if( r_useShadowMapping.GetBool() )
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

			for( ; side < sideStop; side++ )
			{
				ShadowMapPass( commandList, vLight->globalShadows, vLight, side );
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

			GL_State( GLS_DEFAULT, false );
		}
		// RB end

		commandList->endMarker();
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

	commandList->endMarker();
}

void ForwardShadingPass::ShadowMapPass( nvrhi::ICommandList* commandList, const drawSurf_t* drawSurfs, const viewLight_t* vLight, int side )
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

	renderProgManager.BindShader_Depth();

	GL_SelectTexture( 0 );

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

		shadowV[side] = lightViewRenderMatrix;
		shadowP[side] = lightProjectionRenderMatrix;
	}
	else if( vLight->pointLight && side >= 0 )
	{
		assert( side >= 0 && side < 6 );

		// FIXME OPTIMIZE no memset

		float	viewMatrix[16];

		idVec3	vec;
		idVec3	origin = vLight->globalLightOrigin;

		// side of a point light
		memset( viewMatrix, 0, sizeof( viewMatrix ) );
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

		lightProjectionMatrix[0 * 4 + 0] = 2.0f * zNear / width;
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
		lightProjectionMatrix[3 * 4 + 2] = -2.0f * zNear;

		lightProjectionMatrix[0 * 4 + 3] = 0.0f;
		lightProjectionMatrix[1 * 4 + 3] = 0.0f;
		lightProjectionMatrix[2 * 4 + 3] = -1.0f;
		lightProjectionMatrix[3 * 4 + 3] = 0.0f;

		idRenderMatrix::Transpose( *( idRenderMatrix* )lightProjectionMatrix, lightProjectionRenderMatrix );

		shadowV[side] = lightViewRenderMatrix;
		shadowP[side] = lightProjectionRenderMatrix;
	}
	else
	{
		lightViewRenderMatrix.Identity();
		lightProjectionRenderMatrix = vLight->baseLightProject;

		shadowV[0] = lightViewRenderMatrix;
		shadowP[0] = lightProjectionRenderMatrix;
	}

	GL_ViewportAndScissor( 0, 0, shadowMapResolutions[vLight->shadowLOD], shadowMapResolutions[vLight->shadowLOD] );

	//glClear( GL_DEPTH_BUFFER_BIT );

	// process the chain of shadows with the current rendering state
	currentSpace = NULL;

	nvrhi::GraphicsState graphicsState;
	graphicsState.framebuffer = currentFramebuffer->GetApiObject();
	nvrhi::Viewport viewport;
	viewport.minX = currentViewport.x1;
	viewport.minY = currentViewport.y1;
	viewport.maxX = currentViewport.x2;
	viewport.maxY = currentViewport.y2;
	viewport.minZ = currentViewport.zmin;
	viewport.maxZ = currentViewport.zmax;
	graphicsState.viewport.addViewportAndScissorRect( viewport );
	graphicsState.viewport.addScissorRect( nvrhi::Rect( currentScissor.x1, currentScissor.y1, currentScissor.x2, currentScissor.y2 ) );
	renderProgManager.BindShader_Depth();
	auto currentBindingSet = bindingCache.GetOrCreateBindingSet( geometryBindingSetDesc, renderProgManager.BindingLayout() );
	graphicsState.bindings = { currentBindingSet };

	bool stateValid = false;

	nvrhi::DrawArguments currentDraw;
	currentDraw.instanceCount = 0;

	shaderStage_t lastStage;
	bool changedStage = false;

	int lastShader = -1;

	for( const drawSurf_t* drawSurf = drawSurfs; drawSurf != NULL; drawSurf = drawSurf->nextOnLight )
	{
		stateValid = false;

		// Make sure the shadow occluder geometry is done
		if( drawSurf->shadowVolumeState != SHADOWVOLUME_DONE )
		{
			assert( drawSurf->shadowVolumeState == SHADOWVOLUME_UNFINISHED || drawSurf->shadowVolumeState == SHADOWVOLUME_DONE );

			uint64 start = Sys_Microseconds();
			while( drawSurf->shadowVolumeState == SHADOWVOLUME_UNFINISHED )
			{
				Sys_Yield();
			}
			uint64 end = Sys_Microseconds();

			// TODO(Stephen): will probably need to change this if we're going to make this multi-threaded.
			tr.backend.pc.cpuShadowMicroSec += end - start;
		}

		if( drawSurf->numIndexes == 0 )
		{
			continue;	// a job may have created an empty shadow geometry
		}

		if( drawSurf->space != currentSpace )
		{
			idRenderMatrix modelRenderMatrix;
			idRenderMatrix::Transpose( *( idRenderMatrix* )drawSurf->space->modelMatrix, modelRenderMatrix );

			idRenderMatrix modelToLightRenderMatrix;
			idRenderMatrix::Multiply( lightViewRenderMatrix, modelRenderMatrix, modelToLightRenderMatrix );

			idRenderMatrix clipMVP;
			idRenderMatrix::Multiply( lightProjectionRenderMatrix, modelToLightRenderMatrix, clipMVP );

			if( vLight->parallel )
			{
				idRenderMatrix MVP;
				idRenderMatrix::Multiply( renderMatrix_clipSpaceToWindowSpace, clipMVP, MVP );

				RB_SetMVP( clipMVP );
			}
			else if( side < 0 )
			{
				// from OpenGL view space to OpenGL NDC ( -1 : 1 in XYZ )
				idRenderMatrix MVP;
				idRenderMatrix::Multiply( renderMatrix_windowSpaceToClipSpace, clipMVP, MVP );

				RB_SetMVP( MVP );
			}
			else
			{
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
				if( regs[pStage->conditionRegister] == 0 )
				{
					continue;
				}

				// if we at least tried to draw an alpha tested stage,
				// we won't draw the opaque surface
				didDraw = true;

				// set the alpha modulate
				color[3] = regs[pStage->color.registers[3]];

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
				idVec4 alphaTestValue( regs[pStage->alphaTestRegister] );
				renderProgManager.SetRenderParm( RENDERPARM_ALPHA_TEST, alphaTestValue.ToFloatPtr() );

				if( drawSurf->jointCache )
				{
					renderProgManager.BindShader_TextureVertexColorSkinned();
				}
				else
				{
					renderProgManager.BindShader_TextureVertexColor();
				}

				RB_SetVertexColorParms( SVC_IGNORE );

				bool imageChanged = ( imageParms[0] != pStage->texture.image );

				// bind the texture
				GL_SelectTexture( 0 );
				GL_BindTexture( pStage->texture.image );

				// set texture matrix and texGens
				PrepareStageTexturing( pStage, drawSurf );

				if( imageChanged )
				{
					auto bindingSetDesc = nvrhi::BindingSetDesc()
										  .addItem( nvrhi::BindingSetItem::ConstantBuffer( 0, renderProgManager.ConstantBuffer() ) )
										  .addItem( nvrhi::BindingSetItem::Texture_SRV( 0, ( nvrhi::ITexture* )imageParms[0]->GetTextureID() ) )
										  .addItem( nvrhi::BindingSetItem::Sampler( 0, ( nvrhi::ISampler* )imageParms[0]->GetSampler( samplerCache ) ) );

					auto currentBindingSet = bindingCache.GetOrCreateBindingSet( bindingSetDesc, renderProgManager.BindingLayout() );

					graphicsState.bindings = { currentBindingSet };

					stateValid = false;
				}

				// must render with less-equal for Z-Cull to work properly
				assert( ( GL_GetCurrentState() & GLS_DEPTHFUNC_BITS ) == GLS_DEPTHFUNC_LESS );

				SetupInputBuffers( drawSurf, graphicsState );

				renderProgManager.CommitConstantBuffer( commandList );

				if( !stateValid )
				{
					commandList->setGraphicsState( graphicsState );
					stateValid = true;
				}

				if( !pipeline )
				{
					pipeline = CreateGraphicsPipeline( currentFramebuffer->GetApiObject() );
				}

				nvrhi::DrawArguments args;
				args.vertexCount = drawSurf->numIndexes;
				args.instanceCount = 1;
				commandList->drawIndexed( args );

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

			// must render with less-equal for Z-Cull to work properly
			assert( ( GL_GetCurrentState() & GLS_DEPTHFUNC_BITS ) == GLS_DEPTHFUNC_LESS );

			SetupInputBuffers( drawSurf, graphicsState );

			if( !stateValid )
			{
				commandList->setGraphicsState( graphicsState );
				stateValid = true;
			}

			if( !pipeline )
			{
				pipeline = CreateGraphicsPipeline( currentFramebuffer->GetApiObject() );
			}

			renderProgManager.CommitConstantBuffer( commandList );

			nvrhi::DrawArguments args;
			args.vertexCount = drawSurf->numIndexes;
			args.instanceCount = 1;
			commandList->drawIndexed( args );
		}
	}
}

nvrhi::GraphicsPipelineHandle ForwardShadingPass::CreateGraphicsPipeline( nvrhi::IFramebuffer* framebuffer )
{
	return device->createGraphicsPipeline( pipelineDesc, framebuffer );
}

void ForwardShadingPass::SetupView( nvrhi::ICommandList* commandList, viewDef_t* viewDef )
{
}

bool ForwardShadingPass::SetupMaterial( const idMaterial* material, nvrhi::RasterCullMode cullMode, nvrhi::GraphicsState& state )
{
	if( !pipeline )
	{
		pipeline = CreateGraphicsPipeline( state.framebuffer );
	}

	if( !pipeline )
	{
		return false;
	}

	assert( pipeline->getFramebufferInfo() == state.framebuffer->getFramebufferInfo() );

	state.pipeline = pipeline;

	return true;
}

void ForwardShadingPass::SetupInputBuffers( const drawSurf_t* surf, nvrhi::GraphicsState& state )
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

	state.indexBuffer = { indexBuffer->GetAPIObject(), nvrhi::Format::R16_UINT, indexOffset };
	state.vertexBuffers = { { vertexBuffer->GetAPIObject(), 0, vertOffset } };
}

void ForwardShadingPass::SetPushConstants( nvrhi::ICommandList* commandList, nvrhi::GraphicsState& state, nvrhi::DrawArguments& args )
{
}

#endif