/*
* Copyright (c) 2014-2021, NVIDIA CORPORATION. All rights reserved.
* Copyright (C) 2022 Stephen Pridham (id Tech 4x integration)
* Copyright (C) 2022-2023 Robert Beckebans (id Tech 4x integration)
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

#ifndef RENDERER_PASSES_TONEMAPPASS
#define RENDERER_PASSES_TONEMAPPASS

#include "CommonPasses.h"

struct ToneMappingConstants
{
	idVec2i viewOrigin;
	idVec2i viewSize;

	float logLuminanceScale;
	float logLuminanceBias;
	float histogramLowPercentile;
	float histogramHighPercentile;

	float eyeAdaptationSpeedUp;
	float eyeAdaptationSpeedDown;
	float minAdaptedLuminance;
	float maxAdaptedLuminance;

	float frameTime;
	float exposureScale;
	float whitePointInvSquared;
	uint sourceSlice;

	idVec2 colorLUTTextureSize;
	idVec2 colorLUTTextureSizeInv;
};

struct ToneMappingParameters
{
	float histogramLowPercentile = 0.8f;
	float histogramHighPercentile = 0.95f;
	float eyeAdaptationSpeedUp = 1.f;
	float eyeAdaptationSpeedDown = 0.5f;
	float minAdaptedLuminance = 0.02f;
	float maxAdaptedLuminance = 0.5f;
	float exposureBias = -0.5f;
	float whitePoint = 3.f;
	bool enableColorLUT = true;
};

class TonemapPass
{
public:

	struct CreateParameters
	{
		bool isTextureArray = false;
		uint32_t histogramBins = 256;
		uint32_t numConstantBufferVersions = 32;
		nvrhi::IBuffer* exposureBufferOverride = nullptr;
		idImage* colorLUT = nullptr;
	};

	TonemapPass();

	void Init( nvrhi::DeviceHandle deviceHandle, CommonRenderPasses* commonPasses, const CreateParameters& params, nvrhi::IFramebuffer* _sampleFramebuffer );

	void Render( nvrhi::ICommandList* commandList, const ToneMappingParameters& params, const viewDef_t* viewDef, nvrhi::ITexture* sourceTexture, nvrhi::FramebufferHandle _targetFb );

	void SimpleRender( nvrhi::ICommandList* commandList, const ToneMappingParameters& params, const viewDef_t* viewDef, nvrhi::ITexture* sourceTexture, nvrhi::FramebufferHandle _targetFb );

	bool IsLoaded() const
	{
		return isLoaded;
	}

private:

	void ResetExposure( nvrhi::ICommandList* commandList, float initialExposure );

	void ResetHistogram( nvrhi::ICommandList* commandList );

	void AddFrameToHistogram( nvrhi::ICommandList* commandList, const viewDef_t* viewDef, nvrhi::ITexture* sourceTexture );

	void ComputeExposure( nvrhi::ICommandList* commandList, const ToneMappingParameters& params );

	bool                            isLoaded;
	idImage*                        colorLut;
	int                             colorLutSize;
	CommonRenderPasses*             commonPasses;
	nvrhi::DeviceHandle             device;
	nvrhi::ShaderHandle             histogramShader;
	nvrhi::ShaderHandle             exposureShader;
	nvrhi::ShaderHandle             tonemapShader;
	nvrhi::BufferHandle             toneMappingCb;
	nvrhi::BufferHandle             histogramBuffer;
	nvrhi::BufferHandle             exposureBuffer;
	nvrhi::BindingLayoutHandle      renderBindingLayoutHandle;
	nvrhi::BindingLayoutHandle      histogramBindingLayoutHandle;
	nvrhi::ComputePipelineHandle    histogramPipeline;
	nvrhi::ComputePipelineHandle    exposurePipeline;
	nvrhi::GraphicsPipelineHandle   renderPipeline;
	nvrhi::BindingSetHandle         exposureBindingSet;
	idList<nvrhi::BindingSetHandle> histogramBindingSets;
	idHashIndex						histogramBindingHash;
	idList<nvrhi::BindingSetHandle> renderBindingSets;
	idHashIndex						renderBindingHash;
};

#endif
