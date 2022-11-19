#include "precompiled.h"
#pragma hdrstop

#include "renderer/RenderCommon.h"

#include "TonemapPass.h"

TonemapPass::TonemapPass()
	: isLoaded( false )
	, colorLut( nullptr )
	, colorLutSize( 0 )
	, commonPasses( nullptr )
{
}

void TonemapPass::Init( nvrhi::DeviceHandle _device, CommonRenderPasses* _commonPasses, const CreateParameters& _params, nvrhi::IFramebuffer* _sampleFramebuffer )
{
	assert( _params.histogramBins <= 256 );

	device = _device;
	commonPasses = _commonPasses;

	auto histogramShaderInfo = ( _params.isTextureArray ) ? renderProgManager.GetProgramInfo( BUILTIN_HISTOGRAM_TEX_ARRAY_CS ) : renderProgManager.GetProgramInfo( BUILTIN_HISTOGRAM_CS );
	auto exposureShaderInfo = renderProgManager.GetProgramInfo( BUILTIN_EXPOSURE_CS );
	auto tonemapShaderInfo = renderProgManager.GetProgramInfo( BUILTIN_TONEMAPPING );

	histogramShader = histogramShaderInfo.cs;
	exposureShader = exposureShaderInfo.cs;
	tonemapShader = tonemapShaderInfo.ps;

	renderBindingLayoutHandle = ( *tonemapShaderInfo.bindingLayouts )[0];
	histogramBindingLayoutHandle = ( *histogramShaderInfo.bindingLayouts )[0];

	nvrhi::BufferDesc constantBufferDesc;
	constantBufferDesc.byteSize = sizeof( ToneMappingConstants );
	constantBufferDesc.debugName = "ToneMappingConstants";
	constantBufferDesc.isConstantBuffer = true;
	constantBufferDesc.isVolatile = true;
	constantBufferDesc.maxVersions = _params.numConstantBufferVersions;
	toneMappingCb = device->createBuffer( constantBufferDesc );

	nvrhi::BufferDesc storageBufferDesc;
	storageBufferDesc.byteSize = sizeof( uint ) * _params.histogramBins;
	storageBufferDesc.format = nvrhi::Format::R32_UINT;
	storageBufferDesc.canHaveUAVs = true;
	storageBufferDesc.debugName = "HistogramBuffer";
	storageBufferDesc.initialState = nvrhi::ResourceStates::UnorderedAccess;
	storageBufferDesc.keepInitialState = true;
	storageBufferDesc.canHaveTypedViews = true;
	histogramBuffer = device->createBuffer( storageBufferDesc );

	if( _params.exposureBufferOverride )
	{
		exposureBuffer = _params.exposureBufferOverride;
	}
	else
	{
		storageBufferDesc.byteSize = sizeof( uint );
		storageBufferDesc.format = nvrhi::Format::R32_UINT;
		storageBufferDesc.debugName = "ExposureBuffer";
		exposureBuffer = device->createBuffer( storageBufferDesc );
	}

	colorLut = globalImages->blackImage;

	if( _params.colorLUT )
	{
		int w = _params.colorLUT->GetOpts().width;
		int h = _params.colorLUT->GetOpts().height;

		colorLutSize = h;

		if( w != h * h )
		{
			common->Error( "Color LUT texture size must be: width = (n*n), height = (n)" );
			colorLutSize = 0.f;
		}
		else
		{
			colorLut = _params.colorLUT;
		}
	}

	{
		nvrhi::ComputePipelineDesc computePipelineDesc;
		computePipelineDesc.CS = histogramShader;
		computePipelineDesc.bindingLayouts = { histogramBindingLayoutHandle };
		histogramPipeline = device->createComputePipeline( computePipelineDesc );
	}

	{
		nvrhi::BindingSetDesc bindingSetDesc;
		bindingSetDesc.bindings =
		{
			nvrhi::BindingSetItem::ConstantBuffer( 0, toneMappingCb ),
			nvrhi::BindingSetItem::TypedBuffer_SRV( 0, histogramBuffer ),
			nvrhi::BindingSetItem::TypedBuffer_UAV( 0, exposureBuffer )
		};
		exposureBindingSet = device->createBindingSet( bindingSetDesc, ( *exposureShaderInfo.bindingLayouts )[0] );

		nvrhi::ComputePipelineDesc computePipelineDesc;
		computePipelineDesc.CS = exposureShader;
		computePipelineDesc.bindingLayouts = { ( *exposureShaderInfo.bindingLayouts )[0] };
		exposurePipeline = device->createComputePipeline( computePipelineDesc );
	}

	{
		nvrhi::GraphicsPipelineDesc pipelineDesc;
		pipelineDesc.primType = nvrhi::PrimitiveType::TriangleStrip;
		pipelineDesc.VS = tonemapShaderInfo.vs;
		pipelineDesc.PS = tonemapShaderInfo.ps;
		pipelineDesc.bindingLayouts = { renderBindingLayoutHandle };

		pipelineDesc.renderState.rasterState.setCullNone();
		pipelineDesc.renderState.depthStencilState.depthTestEnable = false;
		pipelineDesc.renderState.depthStencilState.stencilEnable = false;

		renderPipeline = device->createGraphicsPipeline( pipelineDesc, _sampleFramebuffer );
	}

	isLoaded = true;
}

void TonemapPass::Render(
	nvrhi::ICommandList* commandList,
	const ToneMappingParameters& params,
	const viewDef_t* viewDef,
	nvrhi::ITexture* sourceTexture,
	nvrhi::FramebufferHandle _targetFb )
{
	size_t renderHash = std::hash< nvrhi::ITexture* >()( sourceTexture );
	nvrhi::BindingSetHandle renderBindingSet;
	for( int i = renderBindingHash.First( renderHash ); i != -1; i = renderBindingHash.Next( i ) )
	{
		nvrhi::BindingSetHandle bindingSet = renderBindingSets[i];
		if( sourceTexture == bindingSet->getDesc()->bindings[1].resourceHandle )
		{
			renderBindingSet = bindingSet;
			break;
		}
	}

	if( !renderBindingSet )
	{
		nvrhi::BindingSetDesc bindingSetDesc;
		bindingSetDesc.bindings =
		{
			nvrhi::BindingSetItem::ConstantBuffer( 0, toneMappingCb ),
			nvrhi::BindingSetItem::Texture_SRV( 0, sourceTexture ),
			nvrhi::BindingSetItem::TypedBuffer_SRV( 1, exposureBuffer ),
			nvrhi::BindingSetItem::Texture_SRV( 2, colorLut->GetTextureHandle() ),
			nvrhi::BindingSetItem::Sampler( 0, commonPasses->m_LinearClampSampler )
		};
		renderBindingSet = device->createBindingSet( bindingSetDesc, renderBindingLayoutHandle );
	}

	{
		nvrhi::GraphicsState state;
		state.pipeline = renderPipeline;
		state.framebuffer = _targetFb;
		state.bindings = { renderBindingSet };
		nvrhi::Viewport viewport{ ( float )viewDef->viewport.x1,
								  ( float )viewDef->viewport.x2 + 1,
								  ( float )viewDef->viewport.y1,
								  ( float )viewDef->viewport.y2 + 1,
								  viewDef->viewport.zmin,
								  viewDef->viewport.zmax };
		state.viewport.addViewportAndScissorRect( viewport );
		//state.viewport.addScissorRect( nvrhi::Rect( viewDef->scissor.x1, viewDef->scissor.y1, viewDef->scissor.x2, viewDef->scissor.y2 ) );

		bool enableColorLUT = params.enableColorLUT && colorLutSize > 0;

		ToneMappingConstants toneMappingConstants = {};
		toneMappingConstants.exposureScale = ::exp2f( r_exposure.GetFloat() );
		toneMappingConstants.whitePointInvSquared = 1.f / powf( params.whitePoint, 2.f );
		toneMappingConstants.minAdaptedLuminance = r_hdrMinLuminance.GetFloat();
		toneMappingConstants.maxAdaptedLuminance = r_hdrMaxLuminance.GetFloat();
		toneMappingConstants.sourceSlice = 0;
		toneMappingConstants.colorLUTTextureSize = enableColorLUT ? idVec2( colorLutSize * colorLutSize, colorLutSize ) : idVec2( 0.f, 0.f );
		toneMappingConstants.colorLUTTextureSizeInv = enableColorLUT ? 1.f / toneMappingConstants.colorLUTTextureSize : idVec2( 0.f, 0.f );
		commandList->writeBuffer( toneMappingCb, &toneMappingConstants, sizeof( toneMappingConstants ) );

		commandList->setGraphicsState( state );

		nvrhi::DrawArguments args;
		args.instanceCount = 1;
		args.vertexCount = 4;
		commandList->draw( args );
	}
}

void TonemapPass::SimpleRender( nvrhi::ICommandList* commandList, const ToneMappingParameters& params, const viewDef_t* view, nvrhi::ITexture* sourceTexture, nvrhi::FramebufferHandle _fbHandle )
{
	commandList->beginMarker( "ToneMapping" );
	ResetHistogram( commandList );
	AddFrameToHistogram( commandList, view, sourceTexture );
	ComputeExposure( commandList, params );
	Render( commandList, params, view, sourceTexture, _fbHandle );
	commandList->endMarker();
}

void TonemapPass::ResetExposure( nvrhi::ICommandList* commandList, float initialExposure )
{
	uint32_t uintValue = *( uint32_t* )&initialExposure;
	commandList->clearBufferUInt( exposureBuffer, uintValue );
}

void TonemapPass::ResetHistogram( nvrhi::ICommandList* commandList )
{
	commandList->clearBufferUInt( histogramBuffer, 0 );
}

void TonemapPass::AddFrameToHistogram( nvrhi::ICommandList* commandList, const viewDef_t* viewDef, nvrhi::ITexture* sourceTexture )
{
	size_t renderHash = std::hash< nvrhi::ITexture* >()( sourceTexture );
	nvrhi::BindingSetHandle bindingSet;
	for( int i = histogramBindingHash.First( renderHash ); i != -1; i = histogramBindingHash.Next( i ) )
	{
		nvrhi::BindingSetHandle foundSet = histogramBindingSets[i];
		if( sourceTexture == foundSet->getDesc()->bindings[1].resourceHandle )
		{
			bindingSet = foundSet;
			break;
		}
	}

	if( !bindingSet )
	{
		nvrhi::BindingSetDesc bindingSetDesc;
		bindingSetDesc.bindings =
		{
			nvrhi::BindingSetItem::ConstantBuffer( 0, toneMappingCb ),
			nvrhi::BindingSetItem::Texture_SRV( 0, sourceTexture ),
			nvrhi::BindingSetItem::TypedBuffer_UAV( 0, histogramBuffer )
		};

		bindingSet = device->createBindingSet( bindingSetDesc, histogramBindingLayoutHandle );
	}

	nvrhi::ViewportState viewportState;
	nvrhi::Viewport viewport{ ( float )viewDef->viewport.x1,
							  ( float )viewDef->viewport.x2,
							  ( float )viewDef->viewport.y1,
							  ( float )viewDef->viewport.y2,
							  viewDef->viewport.zmin,
							  viewDef->viewport.zmax };
	viewportState.addViewportAndScissorRect( viewport );
	//viewportState.addScissorRect( nvrhi::Rect( viewDef->scissor.x1, viewDef->scissor.y1, viewDef->scissor.x2, viewDef->scissor.y2 ) );

	for( uint viewportIndex = 0; viewportIndex < viewportState.scissorRects.size(); viewportIndex++ )
	{
		ToneMappingConstants toneMappingConstants = {};
		toneMappingConstants.logLuminanceScale = 1.0f / ( r_hdrMinLuminance.GetFloat() - r_hdrMaxLuminance.GetFloat() );
		toneMappingConstants.logLuminanceBias = -r_hdrMinLuminance.GetFloat() * toneMappingConstants.logLuminanceScale;

		nvrhi::Rect& scissor = viewportState.scissorRects[viewportIndex];
		toneMappingConstants.viewOrigin = idVec2i( scissor.minX, scissor.minY );
		toneMappingConstants.viewSize = idVec2i( scissor.maxX - scissor.minX, scissor.maxY - scissor.minY );
		toneMappingConstants.sourceSlice = 0;

		commandList->writeBuffer( toneMappingCb, &toneMappingConstants, sizeof( toneMappingConstants ) );

		nvrhi::ComputeState state;
		state.pipeline = histogramPipeline;
		state.bindings = { bindingSet };
		commandList->setComputeState( state );

		idVec2i numGroups = ( toneMappingConstants.viewSize + idVec2i( 15, 15 ) ) / idVec2i( 16, 16 );
		commandList->dispatch( numGroups.x, numGroups.y, 1 );
	}
}

static const float maxLuminance = 4.0f;
static const float minLuminance = -10.0f;

void TonemapPass::ComputeExposure( nvrhi::ICommandList* commandList, const ToneMappingParameters& params )
{
	ToneMappingConstants toneMappingConstants = {};
	toneMappingConstants.logLuminanceScale = maxLuminance - minLuminance;
	toneMappingConstants.logLuminanceBias = minLuminance;
	toneMappingConstants.histogramLowPercentile = Min( 0.99f, Max( 0.f, params.histogramLowPercentile ) );
	toneMappingConstants.histogramHighPercentile = Min( 1.f, Max( toneMappingConstants.histogramLowPercentile, params.histogramHighPercentile ) );
	toneMappingConstants.eyeAdaptationSpeedUp = r_hdrAdaptionRate.GetFloat();
	toneMappingConstants.eyeAdaptationSpeedDown = r_hdrAdaptionRate.GetFloat() / 2.f;
	toneMappingConstants.minAdaptedLuminance = r_hdrMinLuminance.GetFloat();
	toneMappingConstants.maxAdaptedLuminance = r_hdrMaxLuminance.GetFloat();
	toneMappingConstants.frameTime = Sys_Milliseconds() / 1000.0f;

	commandList->writeBuffer( toneMappingCb, &toneMappingConstants, sizeof( toneMappingConstants ) );

	nvrhi::ComputeState state;
	state.pipeline = exposurePipeline;
	state.bindings = { exposureBindingSet };
	commandList->setComputeState( state );

	commandList->dispatch( 1 );
}