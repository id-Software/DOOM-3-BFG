/*
* Copyright (c) 2014-2021, NVIDIA CORPORATION. All rights reserved.
* Copyright (C) 2022 Robert Beckebans (id Tech 4x integration)
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

#include "precompiled.h"
#pragma hdrstop

#include "TemporalAntiAliasingPass.h"
#include "CommonPasses.h"

#include "renderer/RenderCommon.h"

#include <nvrhi/utils.h>

#include <assert.h>
#include <random>

TemporalAntiAliasingPass::TemporalAntiAliasingPass()
	: m_CommonPasses( NULL )
	, m_FrameIndex( 0 )
	, m_StencilMask( 0 )//params.motionVectorStencilMask )
	, m_R2Jitter( 0.0f, 0.0f )
{
}

void TemporalAntiAliasingPass::Init(
	nvrhi::IDevice* device,
	CommonRenderPasses* _commonPasses,
	const viewDef_t* viewDef,
	const CreateParameters& params )
{
	m_CommonPasses = _commonPasses;

	const nvrhi::TextureDesc& unresolvedColorDesc = params.unresolvedColor->getDesc();
	const nvrhi::TextureDesc& resolvedColorDesc = params.resolvedColor->getDesc();
	const nvrhi::TextureDesc& feedback1Desc = params.feedback1->getDesc();
	const nvrhi::TextureDesc& feedback2Desc = params.feedback2->getDesc();

	assert( feedback1Desc.width == feedback2Desc.width );
	assert( feedback1Desc.height == feedback2Desc.height );
	assert( feedback1Desc.format == feedback2Desc.format );
	assert( feedback1Desc.isUAV );
	assert( feedback2Desc.isUAV );
	assert( resolvedColorDesc.isUAV );

	bool useStencil = false;
	nvrhi::Format stencilFormat = nvrhi::Format::UNKNOWN;
	if( params.motionVectorStencilMask )
	{
		useStencil = true;

		nvrhi::Format depthFormat = params.sourceDepth->getDesc().format;

		if( depthFormat == nvrhi::Format::D24S8 )
		{
			stencilFormat = nvrhi::Format::X24G8_UINT;
		}
		else if( depthFormat == nvrhi::Format::D32S8 )
		{
			stencilFormat = nvrhi::Format::X32G8_UINT;
		}
		else
		{
			common->Error( "the format of sourceDepth texture doesn't have a stencil plane" );
		}
	}

	//std::vector<ShaderMacro> MotionVectorMacros;
	//MotionVectorMacros.push_back( ShaderMacro( "USE_STENCIL", useStencil ? "1" : "0" ) );
	//m_MotionVectorPS = shaderFactory->CreateShader( "donut/passes/motion_vectors_ps.hlsl", "main", &MotionVectorMacros, nvrhi::ShaderType::Pixel );

	auto taaMotionVectorsShaderInfo = renderProgManager.GetProgramInfo( BUILTIN_TAA_MOTION_VECTORS );
	m_MotionVectorPS = taaMotionVectorsShaderInfo.ps;

	//std::vector<ShaderMacro> ResolveMacros;
	//ResolveMacros.push_back( ShaderMacro( "SAMPLE_COUNT", std::to_string( unresolvedColorDesc.sampleCount ) ) );
	//ResolveMacros.push_back( ShaderMacro( "USE_CATMULL_ROM_FILTER", params.useCatmullRomFilter ? "1" : "0" ) );
	//m_TemporalAntiAliasingCS = shaderFactory->CreateShader( "donut/passes/taa_cs.hlsl", "main", &ResolveMacros, nvrhi::ShaderType::Compute );

	switch( r_antiAliasing.GetInteger() )
	{
#if ID_MSAA
		case ANTI_ALIASING_MSAA_2X:
		{
			auto taaResolveShaderInfo = renderProgManager.GetProgramInfo( BUILTIN_TAA_RESOLVE_MSAA_2X );
			m_TemporalAntiAliasingCS = taaResolveShaderInfo.cs;
			break;
		}

		case ANTI_ALIASING_MSAA_4X:
		{
			auto taaResolveShaderInfo = renderProgManager.GetProgramInfo( BUILTIN_TAA_RESOLVE_MSAA_4X );
			m_TemporalAntiAliasingCS = taaResolveShaderInfo.cs;
			break;
		}
#endif

		default:
		{
			auto taaResolveShaderInfo = renderProgManager.GetProgramInfo( BUILTIN_TAA_RESOLVE );
			m_TemporalAntiAliasingCS = taaResolveShaderInfo.cs;
			break;
		}
	}

	nvrhi::SamplerDesc samplerDesc;
	samplerDesc.addressU = samplerDesc.addressV = samplerDesc.addressW = nvrhi::SamplerAddressMode::Border;
	samplerDesc.borderColor = nvrhi::Color( 0.0f );
	m_BilinearSampler = device->createSampler( samplerDesc );

	nvrhi::BufferDesc constantBufferDesc;
	constantBufferDesc.byteSize = sizeof( TemporalAntiAliasingConstants );
	constantBufferDesc.debugName = "TemporalAntiAliasingConstants";
	constantBufferDesc.isConstantBuffer = true;
	constantBufferDesc.isVolatile = true;
	constantBufferDesc.maxVersions = params.numConstantBufferVersions;
	m_TemporalAntiAliasingCB = device->createBuffer( constantBufferDesc );

	if( params.sourceDepth )
	{
		nvrhi::BindingLayoutDesc layoutDesc;
		layoutDesc.visibility = nvrhi::ShaderType::Pixel;
		layoutDesc.bindings =
		{
			nvrhi::BindingLayoutItem::VolatileConstantBuffer( 0 ),
			nvrhi::BindingLayoutItem::Texture_SRV( 0 )
		};

		if( useStencil )
		{
			layoutDesc.bindings.push_back( nvrhi::BindingLayoutItem::Texture_SRV( 1 ) );
		}

		m_MotionVectorsBindingLayout = device->createBindingLayout( layoutDesc );

		nvrhi::BindingSetDesc bindingSetDesc;
		bindingSetDesc.bindings =
		{
			nvrhi::BindingSetItem::ConstantBuffer( 0, m_TemporalAntiAliasingCB ),
			nvrhi::BindingSetItem::Texture_SRV( 0, params.sourceDepth ),
		};
		if( useStencil )
		{
			bindingSetDesc.bindings.push_back( nvrhi::BindingSetItem::Texture_SRV( 1, params.sourceDepth, stencilFormat ) );
		}
		m_MotionVectorsBindingSet = device->createBindingSet( bindingSetDesc, m_MotionVectorsBindingLayout );
	}

	{
		nvrhi::BindingSetDesc bindingSetDesc;
		bindingSetDesc.bindings =
		{
			nvrhi::BindingSetItem::ConstantBuffer( 0, m_TemporalAntiAliasingCB ),
			nvrhi::BindingSetItem::Sampler( 0, m_BilinearSampler ),
			nvrhi::BindingSetItem::Texture_SRV( 0, params.unresolvedColor ),
			nvrhi::BindingSetItem::Texture_SRV( 1, params.motionVectors ),
			nvrhi::BindingSetItem::Texture_SRV( 2, params.feedback1 ),
			nvrhi::BindingSetItem::Texture_UAV( 0, params.resolvedColor ),
			nvrhi::BindingSetItem::Texture_UAV( 1, params.feedback2 )
		};

		nvrhi::utils::CreateBindingSetAndLayout( device, nvrhi::ShaderType::Compute, 0, bindingSetDesc, m_ResolveBindingLayout, m_ResolveBindingSet );

		// Swap resolvedColor and resolvedColorPrevious (t2 and u0)
		bindingSetDesc.bindings[4].resourceHandle = params.feedback2;
		bindingSetDesc.bindings[6].resourceHandle = params.feedback1;
		m_ResolveBindingSetPrevious = device->createBindingSet( bindingSetDesc, m_ResolveBindingLayout );

		nvrhi::ComputePipelineDesc pipelineDesc;
		pipelineDesc.CS = m_TemporalAntiAliasingCS;
		pipelineDesc.bindingLayouts = { m_ResolveBindingLayout };

		m_ResolvePso = device->createComputePipeline( pipelineDesc );
	}
}

void TemporalAntiAliasingPass::TemporalResolve(
	nvrhi::ICommandList* commandList,
	const TemporalAntiAliasingParameters& params,
	bool feedbackIsValid,
	const viewDef_t* viewDef )
{
	nvrhi::Viewport viewportInput{ ( float )viewDef->viewport.x1,
								   ( float )viewDef->viewport.x2,
								   ( float )viewDef->viewport.y1,
								   ( float )viewDef->viewport.y2,
								   viewDef->viewport.zmin,
								   viewDef->viewport.zmax };

	const nvrhi::Viewport viewportOutput = viewportInput;

	TemporalAntiAliasingConstants taaConstants = {};
	const float marginSize = 1.f;
	taaConstants.inputViewOrigin = idVec2( viewportInput.minX, viewportInput.minY );
	// RB: TODO figure out why 1 pixel is missing and the old code for resolving _currentImage adds 1 pixel to each side
	taaConstants.inputViewSize = idVec2( viewportInput.width() + 1, viewportInput.height() + 1 );
	taaConstants.outputViewOrigin = idVec2( viewportOutput.minX, viewportOutput.minY );
	taaConstants.outputViewSize = idVec2( viewportOutput.width() + 1, viewportOutput.height() + 1 );
	taaConstants.inputPixelOffset = GetCurrentPixelOffset();
	taaConstants.outputTextureSizeInv = idVec2( 1.0f, 1.0f ) / idVec2( float( renderSystem->GetWidth() ), float( renderSystem->GetHeight() ) );
	taaConstants.inputOverOutputViewSize = taaConstants.inputViewSize / taaConstants.outputViewSize;
	taaConstants.outputOverInputViewSize = taaConstants.outputViewSize / taaConstants.inputViewSize;
	taaConstants.clampingFactor = params.enableHistoryClamping ? params.clampingFactor : -1.f;
	taaConstants.newFrameWeight = feedbackIsValid ? params.newFrameWeight : 1.f;
	taaConstants.pqC = idMath::ClampFloat( 1e-4f, 1e8f, params.maxRadiance );
	taaConstants.invPqC = 1.f / taaConstants.pqC;
	commandList->writeBuffer( m_TemporalAntiAliasingCB, &taaConstants, sizeof( taaConstants ) );

	idVec2i viewportSize = idVec2i( taaConstants.outputViewSize.x, taaConstants.outputViewSize.y );
	idVec2i gridSize = ( viewportSize + 15 ) / 16;

	nvrhi::ComputeState state;
	state.pipeline = m_ResolvePso;
	state.bindings = { m_ResolveBindingSet };
	commandList->setComputeState( state );

	commandList->dispatch( gridSize.x, gridSize.y, 1 );
}

void TemporalAntiAliasingPass::AdvanceFrame()
{
	m_FrameIndex++;

	std::swap( m_ResolveBindingSet, m_ResolveBindingSetPrevious );

	if( TemporalAntiAliasingJitter( r_taaJitter.GetInteger() ) == TemporalAntiAliasingJitter::R2 )
	{
		// Advance R2 jitter sequence
		// http://extremelearning.com.au/unreasonable-effectiveness-of-quasirandom-sequences/

		static const float g = 1.32471795724474602596f;
		static const float a1 = 1.0f / g;
		static const float a2 = 1.0f / ( g * g );
		m_R2Jitter[0] = fmodf( m_R2Jitter[0] + a1, 1.0f );
		m_R2Jitter[1] = fmodf( m_R2Jitter[1] + a2, 1.0f );
	}
}

static float VanDerCorput( size_t base, size_t index )
{
	float ret = 0.0f;
	float denominator = float( base );
	while( index > 0 )
	{
		size_t multiplier = index % base;
		ret += float( multiplier ) / denominator;
		index = index / base;
		denominator *= base;
	}
	return ret;
}

idVec2 TemporalAntiAliasingPass::GetCurrentPixelOffset()
{
	switch( r_taaJitter.GetInteger() )
	{
		default:
		case( int )TemporalAntiAliasingJitter::MSAA:
		{
			const idVec2 offsets[] =
			{
				idVec2( 0.0625f, -0.1875f ), idVec2( -0.0625f, 0.1875f ), idVec2( 0.3125f, 0.0625f ), idVec2( -0.1875f, -0.3125f ),
				idVec2( -0.3125f, 0.3125f ), idVec2( -0.4375f, 0.0625f ), idVec2( 0.1875f, 0.4375f ), idVec2( 0.4375f, -0.4375f )
			};

			return offsets[m_FrameIndex % 8];
		}
		case( int )TemporalAntiAliasingJitter::Halton:
		{
			uint32_t index = ( m_FrameIndex % 16 ) + 1;
			return idVec2{ VanDerCorput( 2, index ), VanDerCorput( 3, index ) } - idVec2( 0.5f, 0.5f );
		}
		case( int )TemporalAntiAliasingJitter::R2:
		{
			return m_R2Jitter - idVec2( 0.5f, 0.5f );
		}
		case( int )TemporalAntiAliasingJitter::WhiteNoise:
		{
			std::mt19937 rng( m_FrameIndex );
			std::uniform_real_distribution<float> dist( -0.5f, 0.5f );
			return idVec2{ dist( rng ), dist( rng ) };
		}
		case( int )TemporalAntiAliasingJitter::None:
		{
			return idVec2( 0, 0 );
		}
	}
}
