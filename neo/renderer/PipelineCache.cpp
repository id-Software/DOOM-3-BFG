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

#include "precompiled.h"
#pragma hdrstop

#include "renderer/RenderCommon.h"
#include "PipelineCache.h"



PipelineCache::PipelineCache()
{
}

void PipelineCache::Init( nvrhi::DeviceHandle deviceHandle )
{
	device = deviceHandle;
}

void PipelineCache::Shutdown()
{
	// SRS - Remove reference to nvrhi::IDevice, otherwise won't clean up properly on shutdown
	device = nullptr;
}

void PipelineCache::Clear()
{
	pipelines.Clear();
	pipelineHash.Clear();
}

nvrhi::GraphicsPipelineHandle PipelineCache::GetOrCreatePipeline( const PipelineKey& key )
{
	std::size_t h = std::hash<PipelineKey> {}( key );

	for( int i = pipelineHash.First( h ); i >= 0; i = pipelineHash.Next( i ) )
	{
		if( pipelines[i].first == key )
		{
			return pipelines[i].second;
		}
	}

	nvrhi::GraphicsPipelineDesc pipelineDesc;
	const programInfo_t progInfo = renderProgManager.GetProgramInfo( key.program );
	pipelineDesc.setVertexShader( progInfo.vs ).setFragmentShader( progInfo.ps );
	pipelineDesc.inputLayout = progInfo.inputLayout;
	for( int i = 0; i < progInfo.bindingLayouts->Num(); i++ )
	{
		pipelineDesc.bindingLayouts.push_back( ( *progInfo.bindingLayouts )[i] );
	}
	pipelineDesc.primType = nvrhi::PrimitiveType::TriangleList;

	// Set up default state.
	pipelineDesc.renderState.rasterState.enableScissor();
	pipelineDesc.renderState.depthStencilState.enableDepthTest().enableDepthWrite();
	for( auto& target : pipelineDesc.renderState.blendState.targets )
	{
		target.enableBlend();
	}

	// Specialize the state with the state key.
	GetRenderState( key.state, key, pipelineDesc.renderState );

	auto pipeline = device->createGraphicsPipeline( pipelineDesc, key.framebuffer->GetApiObject() );

	pipelineHash.Add( h, pipelines.Append( { key, pipeline } ) );

	return pipeline;
}


void PipelineCache::GetRenderState( uint64 stateBits, PipelineKey key, nvrhi::RenderState& renderState )
{
	/*
	uint64 diff = stateBits ^ GLS_DEFAULT;

	if( diff == 0 )
	{
		return;
	}
	*/

	auto& rasterizationState = renderState.rasterState;

	//
	// culling & mirrors
	//
	//if( stateBits & ( GLS_CULL_BITS ) )//| GLS_MIRROR_VIEW ) )
	{
		switch( stateBits & GLS_CULL_BITS )
		{
			case GLS_CULL_TWOSIDED:
				rasterizationState.setCullNone();
				break;

			case GLS_CULL_BACKSIDED:
				if( stateBits & GLS_MIRROR_VIEW )
				{
					rasterizationState.setCullFront();
				}
				else
				{
					rasterizationState.setCullBack();
				}
				break;

			case GLS_CULL_FRONTSIDED:
			default:
				if( stateBits & GLS_MIRROR_VIEW )
				{
					rasterizationState.setCullBack();
				}
				else
				{
					rasterizationState.setCullFront();
				}
				break;
		}
	}

	rasterizationState.setFrontCounterClockwise( ( stateBits & GLS_CLOCKWISE ) == 0 );

	//
	// fill/line mode
	//
	//if( diff & GLS_POLYMODE_LINE )
	{
		if( stateBits & GLS_POLYMODE_LINE )
		{
			rasterizationState.setFillMode( nvrhi::RasterFillMode::Wireframe );
		}
		else
		{
			rasterizationState.setFillMode( nvrhi::RasterFillMode::Solid );
		}
	}

	//
	// polygon offset
	//
	//if( diff & GLS_POLYGON_OFFSET )
	{
		if( stateBits & GLS_POLYGON_OFFSET )
		{
			rasterizationState.depthBias = key.depthBias;
			rasterizationState.slopeScaledDepthBias = key.slopeBias;
			rasterizationState.enableQuadFill();
		}
		else
		{
			//currentRasterState.disableQuadFill();
		}
	}

	nvrhi::BlendState::RenderTarget renderTarget;

	//
	// check blend bits
	//
	//if( stateBits & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) )
	{
		nvrhi::BlendFactor srcFactor = nvrhi::BlendFactor::One;
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

		nvrhi::BlendFactor dstFactor = nvrhi::BlendFactor::Zero;
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

		// RB: blend ops are only used by the SWF render states so far
		nvrhi::BlendOp blendOp = nvrhi::BlendOp::Add;
		switch( stateBits & GLS_BLENDOP_BITS )
		{
			case GLS_BLENDOP_MIN:
				blendOp = nvrhi::BlendOp::Min;
				break;
			case GLS_BLENDOP_MAX:
				blendOp = nvrhi::BlendOp::Max;
				break;
			case GLS_BLENDOP_ADD:
				blendOp = nvrhi::BlendOp::Add;
				break;
			case GLS_BLENDOP_SUB:
				blendOp = nvrhi::BlendOp::Subrtact;
				break;
		}

		// Only actually update GL's blend func if blending is enabled.
		if( srcFactor == nvrhi::BlendFactor::One && dstFactor == nvrhi::BlendFactor::Zero )
		{
			renderTarget.disableBlend();
		}
		else
		{
			//colorBlendState.setAlphaToCoverageEnable( true );
			renderTarget.enableBlend();

			//renderTarget.setBlendOp( blendOp );
			renderTarget.setSrcBlend( srcFactor );
			renderTarget.setDestBlend( dstFactor );

			//renderTarget.setBlendOpAlpha( blendOp );
			//renderTarget.setSrcBlendAlpha( srcFactor );
			//renderTarget.setDestBlendAlpha( dstFactor );
		}
	}

	//
	// check colormask
	//
	//if( diff & ( GLS_REDMASK | GLS_GREENMASK | GLS_BLUEMASK | GLS_ALPHAMASK ) )
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

		//renderTarget.enableBlend();
		renderTarget.setColorWriteMask( mask );
	}

	renderState.blendState.setRenderTarget( 0, renderTarget );


	auto& depthStencilState = renderState.depthStencilState;

	//
	// check depthFunc bits
	//
	//if( diff & GLS_DEPTHFUNC_BITS )
	{
		switch( stateBits & GLS_DEPTHFUNC_BITS )
		{
			case GLS_DEPTHFUNC_EQUAL:
				depthStencilState.depthFunc = nvrhi::ComparisonFunc::Equal;
				break;
			case GLS_DEPTHFUNC_ALWAYS:
				depthStencilState.depthFunc = nvrhi::ComparisonFunc::Always;
				break;
			case GLS_DEPTHFUNC_LESS:
				depthStencilState.depthFunc = nvrhi::ComparisonFunc::LessOrEqual;
				break;
			case GLS_DEPTHFUNC_GREATER:
				depthStencilState.depthFunc = nvrhi::ComparisonFunc::Greater;
				break;
		}
	}

	//
	// check depthmask
	//
	//if( diff & GLS_DEPTHMASK )
	{
		if( stateBits & GLS_DEPTHMASK )
		{
			depthStencilState.disableDepthWrite();
			if( ( stateBits & GLS_DEPTHFUNC_BITS ) == GLS_DEPTHFUNC_ALWAYS )
			{
				depthStencilState.disableDepthTest();
			}
		}
	}

	//
	// stencil
	//
	//if( diff & ( GLS_STENCIL_FUNC_BITS | GLS_STENCIL_OP_BITS ) )
	{
		if( ( stateBits & ( GLS_STENCIL_FUNC_BITS | GLS_STENCIL_OP_BITS | GLS_SEPARATE_STENCIL ) ) != 0 )
		{
			depthStencilState.enableStencil();
		}
		else
		{
			depthStencilState.disableStencil();
		}
	}

	if( stateBits & ( GLS_STENCIL_FUNC_BITS | GLS_STENCIL_FUNC_REF_BITS | GLS_STENCIL_FUNC_MASK_BITS ) )
	{
		uint32 ref = uint32( ( stateBits & GLS_STENCIL_FUNC_REF_BITS ) >> GLS_STENCIL_FUNC_REF_SHIFT );
		uint32 mask = uint32( ( stateBits & GLS_STENCIL_FUNC_MASK_BITS ) >> GLS_STENCIL_FUNC_MASK_SHIFT );

		depthStencilState.setStencilRefValue( ref );
		depthStencilState.setStencilReadMask( mask );
		depthStencilState.setStencilWriteMask( 0xFF );
	}

	// Carmack's Reverse with GLS_SEPARATE_STENCIL
	if( stateBits & GLS_SEPARATE_STENCIL )
	{
		nvrhi::DepthStencilState::StencilOpDesc frontStencilOp = GetStencilOpState( stateBits & GLS_STENCIL_FRONT_OPS );
		nvrhi::DepthStencilState::StencilOpDesc backStencilOp = GetStencilOpState( ( stateBits & GLS_STENCIL_BACK_OPS ) >> 12 );

		depthStencilState.setFrontFaceStencil( frontStencilOp );
		depthStencilState.setFrontFaceStencil( backStencilOp );
	}
	else
	{
		nvrhi::DepthStencilState::StencilOpDesc stencilOp = GetStencilOpState( stateBits );

		depthStencilState.setFrontFaceStencil( stencilOp );
		depthStencilState.setBackFaceStencil( stencilOp );
	}
}

nvrhi::DepthStencilState::StencilOpDesc PipelineCache::GetStencilOpState( uint64 stateBits )
{
	nvrhi::DepthStencilState::StencilOpDesc stencilOp;

	//if( stateBits & ( GLS_STENCIL_OP_FAIL_BITS | GLS_STENCIL_OP_ZFAIL_BITS | GLS_STENCIL_OP_PASS_BITS ) )
	{
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

	return stencilOp;
}
