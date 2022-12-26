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

#include "SsaoPass.h"

struct SsaoConstants
{
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
	const SsaoParameters& params,
	viewDef_t* viewDef,
	int bindingSetIndex )
{
	assert( m_Deinterleave.BindingSets[bindingSetIndex] );
	assert( m_Compute.BindingSets[bindingSetIndex] );
	assert( m_Blur.BindingSets[bindingSetIndex] );

	commandList->beginMarker( "SSAO" );

	nvrhi::Rect viewExtent( viewDef->viewport.x1, viewDef->viewport.x2, viewDef->viewport.y1, viewDef->viewport.y2 );
	nvrhi::Rect quarterResExtent = viewExtent;
	quarterResExtent.minX /= 4;
	quarterResExtent.minY /= 4;
	quarterResExtent.maxX = ( quarterResExtent.maxX + 3 ) / 4;
	quarterResExtent.maxY = ( quarterResExtent.maxY + 3 ) / 4;

	SsaoConstants ssaoConstants = {};
	ssaoConstants.clipToView = idVec2(
								   viewDef->projectionMatrix[2 * 4 + 3] / viewDef->projectionMatrix[0 * 4 + 0],
								   viewDef->projectionMatrix[2 * 4 + 3] / viewDef->projectionMatrix[0 * 4 + 1] );
	ssaoConstants.invQuantizedGbufferSize = 1.f / m_QuantizedGbufferTextureSize;
	ssaoConstants.quantizedViewportOrigin = idVec2i( quarterResExtent.minX, quarterResExtent.minY ) * 4;
	ssaoConstants.amount = params.amount;
	ssaoConstants.invBackgroundViewDepth = ( params.backgroundViewDepth > 0.f ) ? 1.f / params.backgroundViewDepth : 0.f;
	ssaoConstants.radiusWorld = params.radiusWorld;
	ssaoConstants.surfaceBias = params.surfaceBias;
	ssaoConstants.powerExponent = params.powerExponent;
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

	commandList->endMarker();
}