/*
* Copyright (c) 2014-2021, NVIDIA CORPORATION. All rights reserved.
* Copyright (C) 2022 Stephen Pridham (id Tech 4x integration)
* Copyright (C) 2023 Stephen Saunders (id Tech 4x integration)
* Copyright (C) 2023 Robert Beckebans (id Tech 4x integration)
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
* DEALINGS IN THE SOFTWARE.
*/
#include <precompiled.h>
#pragma hdrstop

#include "renderer/RenderCommon.h"

#include "SsaoPass.h"


static idCVar r_ssaoBackgroundViewDepth( "r_ssaoBackgroundViewDepth", "100", CVAR_RENDERER | CVAR_FLOAT, "specified in meters" );
static idCVar r_ssaoRadiusWorld( "r_ssaoRadiusWorld", "1.0", CVAR_RENDERER | CVAR_FLOAT, "specified in meters" );
static idCVar r_ssaoSurfaceBias( "r_ssaoSurfaceBias", "0.05", CVAR_RENDERER | CVAR_FLOAT, "specified in meters" );
static idCVar r_ssaoPowerExponent( "r_ssaoPowerExponent", "2", CVAR_RENDERER | CVAR_FLOAT, "" );
static idCVar r_ssaoBlurSharpness( "r_ssaoBlurSharpness", "16", CVAR_RENDERER | CVAR_FLOAT, "" );
static idCVar r_ssaoAmount( "r_ssaoAmount", "4", CVAR_RENDERER | CVAR_FLOAT, "" );

struct SsaoConstants
{
	idVec2		viewportOrigin;
	idVec2		viewportSize;
	idVec2		pixelOffset;
	idVec2		unused;

	idRenderMatrix matClipToView;
	idRenderMatrix matWorldToView; // unused
	idRenderMatrix matViewToWorld; // unused

	idVec2      clipToView;
	idVec2      invQuantizedGbufferSize;

	idVec2i     quantizedViewportOrigin;
	float       amount;
	float       invBackgroundViewDepth;
	float       radiusWorld;
	float       surfaceBias;

	float       radiusToScreen;
	float       powerExponent;
};

SsaoPass::SsaoPass(
	nvrhi::IDevice* device,
	const CreateParameters& params,
	CommonRenderPasses* commonPasses )
	: commonRenderPasses( commonPasses )
	, m_Device( device )
{
	nvrhi::BufferDesc constantBufferDesc;
	constantBufferDesc.byteSize = sizeof( SsaoConstants );
	constantBufferDesc.debugName = "SsaoConstants";
	constantBufferDesc.isConstantBuffer = true;
	constantBufferDesc.isVolatile = true;
	constantBufferDesc.maxVersions = c_MaxRenderPassConstantBufferVersions;
	m_ConstantBuffer = device->createBuffer( constantBufferDesc );

	nvrhi::TextureDesc DeinterleavedTextureDesc;
	DeinterleavedTextureDesc.width = ( params.dimensions.x + 3 ) / 4;
	DeinterleavedTextureDesc.height = ( params.dimensions.y + 3 ) / 4;
	DeinterleavedTextureDesc.arraySize = 16;
	DeinterleavedTextureDesc.dimension = nvrhi::TextureDimension::Texture2DArray;
	DeinterleavedTextureDesc.isUAV = true;
	DeinterleavedTextureDesc.initialState = nvrhi::ResourceStates::ShaderResource;
	DeinterleavedTextureDesc.keepInitialState = true;
	DeinterleavedTextureDesc.debugName = "SSAO/DeinterleavedDepth";
	DeinterleavedTextureDesc.format = nvrhi::Format::R32_FLOAT;
	m_DeinterleavedDepth = device->createTexture( DeinterleavedTextureDesc );

	m_QuantizedGbufferTextureSize = idVec2( float( DeinterleavedTextureDesc.width ), float( DeinterleavedTextureDesc.height ) ) * 4.f;

	DeinterleavedTextureDesc.debugName = "SSAO/DeinterleavedOcclusion";
	DeinterleavedTextureDesc.format = params.directionalOcclusion ? nvrhi::Format::RGBA16_FLOAT : nvrhi::Format::R8_UNORM;
	m_DeinterleavedOcclusion = device->createTexture( DeinterleavedTextureDesc );

	{
		idList<shaderMacro_t> macros = { { "LINEAR_DEPTH", params.inputLinearDepth ? "1" : "0" } };
		int shaderIdx = renderProgManager.FindShader( "builtin/SSAO/ssao_deinterleave", SHADER_STAGE_COMPUTE, "", macros, true, LAYOUT_DRAW_VERT );
		m_Deinterleave.Shader = renderProgManager.GetShader( shaderIdx );

		nvrhi::BindingLayoutDesc DeinterleaveBindings;
		DeinterleaveBindings.visibility = nvrhi::ShaderType::Compute;
		DeinterleaveBindings.bindings =
		{
			nvrhi::BindingLayoutItem::VolatileConstantBuffer( 0 ),
			nvrhi::BindingLayoutItem::VolatileConstantBuffer( 1 ),
			nvrhi::BindingLayoutItem::Texture_SRV( 0 ),
			nvrhi::BindingLayoutItem::Texture_UAV( 0 ),
		};
		m_Deinterleave.BindingLayout = m_Device->createBindingLayout( DeinterleaveBindings );

		nvrhi::ComputePipelineDesc DeinterleavePipelineDesc;
		DeinterleavePipelineDesc.bindingLayouts = { m_Deinterleave.BindingLayout };
		DeinterleavePipelineDesc.CS = m_Deinterleave.Shader;
		m_Deinterleave.Pipeline = device->createComputePipeline( DeinterleavePipelineDesc );

		m_Deinterleave.BindingSets.resize( params.numBindingSets );
	}

	{
		idList<shaderMacro_t> macros =
		{
			{ "OCT_ENCODED_NORMALS", params.octEncodedNormals ? "1" : "0" },
			{ "DIRECTIONAL_OCCLUSION", params.directionalOcclusion ? "1" : "0" }
		};
		int shaderIdx = renderProgManager.FindShader( "builtin/SSAO/ssao_compute", SHADER_STAGE_COMPUTE, "", macros, true, LAYOUT_DRAW_VERT );
		m_Compute.Shader = renderProgManager.GetShader( shaderIdx );

		nvrhi::BindingLayoutDesc ComputeBindings;
		ComputeBindings.visibility = nvrhi::ShaderType::Compute;
		ComputeBindings.bindings =
		{
			nvrhi::BindingLayoutItem::VolatileConstantBuffer( 0 ),
			nvrhi::BindingLayoutItem::VolatileConstantBuffer( 1 ),
			nvrhi::BindingLayoutItem::Texture_SRV( 0 ),
			nvrhi::BindingLayoutItem::Texture_SRV( 1 ),
			nvrhi::BindingLayoutItem::Texture_UAV( 0 ),
		};
		m_Compute.BindingLayout = m_Device->createBindingLayout( ComputeBindings );

		nvrhi::ComputePipelineDesc ComputePipeline;
		ComputePipeline.bindingLayouts = { m_Compute.BindingLayout };
		ComputePipeline.CS = m_Compute.Shader;
		m_Compute.Pipeline = device->createComputePipeline( ComputePipeline );

		m_Compute.BindingSets.resize( params.numBindingSets );
	}

	{
		idList<shaderMacro_t> macros =
		{
			{ "DIRECTIONAL_OCCLUSION", params.directionalOcclusion ? "1" : "0" }
		};
		int shaderIdx = renderProgManager.FindShader( "builtin/SSAO/ssao_blur", SHADER_STAGE_COMPUTE, "", macros, true, LAYOUT_DRAW_VERT );
		m_Blur.Shader = renderProgManager.GetShader( shaderIdx );

		nvrhi::BindingLayoutDesc BlurBindings;
		BlurBindings.visibility = nvrhi::ShaderType::Compute;
		BlurBindings.bindings =
		{
			nvrhi::BindingLayoutItem::VolatileConstantBuffer( 0 ),
			nvrhi::BindingLayoutItem::VolatileConstantBuffer( 1 ),
			nvrhi::BindingLayoutItem::Texture_SRV( 0 ),
			nvrhi::BindingLayoutItem::Texture_SRV( 1 ),
			nvrhi::BindingLayoutItem::Texture_UAV( 0 ),
			nvrhi::BindingLayoutItem::Sampler( 0 ),
		};
		m_Blur.BindingLayout = m_Device->createBindingLayout( BlurBindings );

		nvrhi::ComputePipelineDesc BlurPipeline;
		BlurPipeline.bindingLayouts = { m_Blur.BindingLayout };
		BlurPipeline.CS = m_Blur.Shader;
		m_Blur.Pipeline = device->createComputePipeline( BlurPipeline );

		m_Blur.BindingSets.resize( params.numBindingSets );
	}
}

// Backwards compatibility constructor
SsaoPass::SsaoPass(
	nvrhi::IDevice* device,
	CommonRenderPasses* commonPasses,
	nvrhi::ITexture* gbufferDepth,
	nvrhi::ITexture* gbufferNormals,
	nvrhi::ITexture* destinationTexture )
	: SsaoPass( device, CreateParameters{ idVec2( gbufferDepth->getDesc().width, gbufferDepth->getDesc().height ), false, false, false, 1 }, commonPasses )
{
	const nvrhi::TextureDesc& depthDesc = gbufferDepth->getDesc();
	const nvrhi::TextureDesc& normalsDesc = gbufferNormals->getDesc();
	assert( depthDesc.sampleCount == normalsDesc.sampleCount );
	assert( depthDesc.sampleCount == 1 ); // more is currently unsupported
	assert( depthDesc.dimension == nvrhi::TextureDimension::Texture2D ); // arrays are currently unsupported

	CreateBindingSet( gbufferDepth, gbufferNormals, destinationTexture, 0 );
}

void SsaoPass::CreateBindingSet(
	nvrhi::ITexture* gbufferDepth,
	nvrhi::ITexture* gbufferNormals,
	nvrhi::ITexture* destinationTexture,
	int bindingSetIndex )
{
	nvrhi::BindingSetDesc DeinterleaveBindings;
	DeinterleaveBindings.bindings =
	{
		nvrhi::BindingSetItem::ConstantBuffer( 0, renderProgManager.ConstantBuffer() ),
		nvrhi::BindingSetItem::ConstantBuffer( 1, m_ConstantBuffer ),
		nvrhi::BindingSetItem::Texture_SRV( 0, gbufferDepth ),
		nvrhi::BindingSetItem::Texture_UAV( 0, m_DeinterleavedDepth )
	};
	m_Deinterleave.BindingSets[bindingSetIndex] = m_Device->createBindingSet( DeinterleaveBindings, m_Deinterleave.BindingLayout );

	nvrhi::BindingSetDesc ComputeBindings;
	ComputeBindings.bindings =
	{
		nvrhi::BindingSetItem::ConstantBuffer( 0, renderProgManager.ConstantBuffer() ),
		nvrhi::BindingSetItem::ConstantBuffer( 1, m_ConstantBuffer ),
		nvrhi::BindingSetItem::Texture_SRV( 0, m_DeinterleavedDepth ),
		nvrhi::BindingSetItem::Texture_SRV( 1, gbufferNormals ),
		nvrhi::BindingSetItem::Texture_UAV( 0, m_DeinterleavedOcclusion )
	};
	m_Compute.BindingSets[bindingSetIndex] = m_Device->createBindingSet( ComputeBindings, m_Compute.BindingLayout );

	nvrhi::BindingSetDesc BlurBindings;
	BlurBindings.bindings =
	{
		nvrhi::BindingSetItem::ConstantBuffer( 0, renderProgManager.ConstantBuffer() ),
		nvrhi::BindingSetItem::ConstantBuffer( 1, m_ConstantBuffer ),
		nvrhi::BindingSetItem::Texture_SRV( 0, m_DeinterleavedDepth ),
		nvrhi::BindingSetItem::Texture_SRV( 1, m_DeinterleavedOcclusion ),
		nvrhi::BindingSetItem::Texture_UAV( 0, destinationTexture ),
		nvrhi::BindingSetItem::Sampler( 0, commonRenderPasses->m_PointClampSampler )
	};
	m_Blur.BindingSets[bindingSetIndex] = m_Device->createBindingSet( BlurBindings, m_Blur.BindingLayout );
}

void SsaoPass::Render(
	nvrhi::ICommandList* commandList,
	const viewDef_t* viewDef,
	int bindingSetIndex )
{
	assert( m_Deinterleave.BindingSets[bindingSetIndex] );
	assert( m_Compute.BindingSets[bindingSetIndex] );
	assert( m_Blur.BindingSets[bindingSetIndex] );

	{
		// RB HACK: add one 1 extra pixel to width and height
		nvrhi::Rect viewExtent( viewDef->viewport.x1, viewDef->viewport.x2 + 1, viewDef->viewport.y1, viewDef->viewport.y2 + 1 );
		nvrhi::Rect quarterResExtent = viewExtent;
		quarterResExtent.minX /= 4;
		quarterResExtent.minY /= 4;
		quarterResExtent.maxX = ( quarterResExtent.maxX + 3 ) / 4;
		quarterResExtent.maxY = ( quarterResExtent.maxY + 3 ) / 4;

		// TODO required and remove this by fixing the shaders
		renderProgManager.BindShader_TextureVertexColor();

		renderProgManager.CommitConstantBuffer( commandList, true );

		SsaoConstants ssaoConstants = {};
		ssaoConstants.viewportOrigin = idVec2( viewDef->viewport.x1, viewDef->viewport.y1 );
		ssaoConstants.viewportSize = idVec2( viewDef->viewport.GetWidth(), viewDef->viewport.GetHeight() );
		ssaoConstants.pixelOffset = tr.backend.GetCurrentPixelOffset();

		// RB: this actually should work but it only works with the old SSAO method ...
		//ssaoConstants.matClipToView = viewDef->unprojectionToCameraRenderMatrix;

		idRenderMatrix::Inverse( *( idRenderMatrix* ) viewDef->projectionMatrix, ssaoConstants.matClipToView );

		// RB: TODO: only need for DIRECTIONAL_OCCLUSION
		// we don't store the view matrix separatly yet
		//ssaoConstants.matViewToWorld = viewDef->worldSpace;
		//idRenderMatrix::Inverse( ssaoConstants.matViewToWorld, ssaoConstants.matWorldToView );

		ssaoConstants.clipToView = idVec2(
									   viewDef->projectionMatrix[2 * 4 + 3] / viewDef->projectionMatrix[0 * 4 + 0],
									   viewDef->projectionMatrix[2 * 4 + 3] / viewDef->projectionMatrix[1 * 4 + 1] );

		ssaoConstants.invQuantizedGbufferSize = idVec2( 1.0f, 1.0f ) / m_QuantizedGbufferTextureSize;
		ssaoConstants.quantizedViewportOrigin = idVec2i( quarterResExtent.minX, quarterResExtent.minY ) * 4;
		ssaoConstants.amount = r_ssaoAmount.GetFloat();
		ssaoConstants.invBackgroundViewDepth = ( r_ssaoBackgroundViewDepth.GetFloat() > 0.f ) ? 1.f / r_ssaoBackgroundViewDepth.GetFloat() : 0.f;
		ssaoConstants.radiusWorld = r_ssaoRadiusWorld.GetFloat();
		ssaoConstants.surfaceBias = r_ssaoSurfaceBias.GetFloat();
		ssaoConstants.powerExponent = r_ssaoPowerExponent.GetFloat();
		ssaoConstants.radiusToScreen = 0.5f * viewDef->viewport.GetHeight() * abs( viewDef->projectionMatrix[1 * 4 + 1] );
		commandList->writeBuffer( m_ConstantBuffer, &ssaoConstants, sizeof( ssaoConstants ) );

		uint32_t dispatchWidth = ( quarterResExtent.width() + 7 ) / 8;
		uint32_t dispatchHeight = ( quarterResExtent.height() + 7 ) / 8;

		nvrhi::ComputeState state;
		state.pipeline = m_Deinterleave.Pipeline;
		state.bindings = { m_Deinterleave.BindingSets[bindingSetIndex] };
		commandList->setComputeState( state );
		commandList->dispatch( dispatchWidth, dispatchHeight, 1 );

		state.pipeline = m_Compute.Pipeline;
		state.bindings = { m_Compute.BindingSets[bindingSetIndex] };
		commandList->setComputeState( state );
		commandList->dispatch( dispatchWidth, dispatchHeight, 16 );

		dispatchWidth = ( viewExtent.width() + 15 ) / 16;
		dispatchHeight = ( viewExtent.height() + 15 ) / 16;

		state.pipeline = m_Blur.Pipeline;
		state.bindings = { m_Blur.BindingSets[bindingSetIndex] };
		commandList->setComputeState( state );
		commandList->dispatch( dispatchWidth, dispatchHeight, 1 );
	}
}
