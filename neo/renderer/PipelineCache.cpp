#include <precompiled.h>
#pragma hdrstrop

#include "renderer/RenderCommon.h"

#include "PipelineCache.h"

void GetRenderState( uint64 stateBits, PipelineKey key, nvrhi::RenderState& renderState );

PipelineCache::PipelineCache( )
{
}

void PipelineCache::Init( nvrhi::DeviceHandle deviceHandle )
{
	device = deviceHandle;
}

void PipelineCache::Clear( )
{
	pipelines.Clear( );
	pipelineHash.Clear( );
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
	programInfo_t progInfo = renderProgManager.GetProgramInfo( key.program );
	pipelineDesc.setVertexShader( progInfo.vs ).setFragmentShader( progInfo.ps );
	pipelineDesc.inputLayout = progInfo.inputLayout;
	pipelineDesc.bindingLayouts = { progInfo.bindingLayout };
	pipelineDesc.primType = nvrhi::PrimitiveType::TriangleList;

	// Set up default state.
	pipelineDesc.renderState.rasterState.enableScissor( );
	pipelineDesc.renderState.depthStencilState.enableDepthTest( ).enableDepthWrite( );
	pipelineDesc.renderState.blendState.targets[0].enableBlend( );
	//pipelineDesc.renderState.rasterState.enableDepthClip( );
	pipelineDesc.renderState.rasterState.depthBias = 0;
	pipelineDesc.renderState.rasterState.slopeScaledDepthBias = 0;

	// Specialize the state with the state key.
	GetRenderState( key.state, key, pipelineDesc.renderState );

	auto pipeline = device->createGraphicsPipeline( pipelineDesc, key.framebuffer->GetApiObject( ) );

	pipelineHash.Add( h, pipelines.Append( { key, pipeline } ) );

	return pipeline;
}


void GetRenderState( uint64 stateBits, PipelineKey key, nvrhi::RenderState& renderState )
{
	uint64 diff = stateBits ^ GLS_DEFAULT;

	if( diff == 0 )
	{
		return;
	}

	auto& currentBlendState = renderState.blendState;
	auto& currentDepthStencilState = renderState.depthStencilState;
	auto& currentRasterState = renderState.rasterState;

	//
	// culling
	//
	if( diff & ( GLS_CULL_BITS ) )//| GLS_MIRROR_VIEW ) )
	{
		switch( stateBits & GLS_CULL_BITS )
		{
			case GLS_CULL_TWOSIDED:
				currentRasterState.setCullNone( );
				break;

			case GLS_CULL_BACKSIDED:
				if( key.mirrored )
				{
					stateBits |= GLS_MIRROR_VIEW;
					currentRasterState.setCullFront( );
				}
				else
				{
					currentRasterState.setCullBack( );
				}
				break;

			case GLS_CULL_FRONTSIDED:
			default:
				if( key.mirrored )
				{
					stateBits |= GLS_MIRROR_VIEW;
					currentRasterState.setCullBack( );
				}
				else
				{
					currentRasterState.setCullFront( );
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
			renderTarget.disableBlend( );
		}
		else
		{
			currentBlendState.setAlphaToCoverageEnable( true );
			renderTarget.enableBlend( );
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
			currentDepthStencilState.disableDepthWrite( );
			if( ( stateBits & GLS_DEPTHFUNC_BITS ) == GLS_DEPTHFUNC_ALWAYS )
			{
				currentDepthStencilState.disableDepthTest( );
			}
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

		renderTarget.enableBlend( );
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
			currentRasterState.setFillMode( nvrhi::RasterFillMode::Wireframe );
			//currentRasterState.setCullNone( );
		}
		else
		{
			//currentRasterState.setCullNone( );
			currentRasterState.setFillMode( nvrhi::RasterFillMode::Solid );
		}
	}

	//
	// polygon offset
	//
	if( diff & GLS_POLYGON_OFFSET )
	{
		if( stateBits & GLS_POLYGON_OFFSET )
		{
			currentRasterState.depthBias = key.depthBias;
			currentRasterState.slopeScaledDepthBias = key.slopeBias;
			currentRasterState.enableQuadFill();
		}
		else
		{
			//currentRasterState.disableQuadFill();
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
			currentDepthStencilState.enableStencil( );
			//currentDepthStencilState.enableDepthWrite( );
		}
		else
		{
			currentDepthStencilState.disableStencil( );
			//currentDepthStencilState.disableDepthWrite( );
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
}