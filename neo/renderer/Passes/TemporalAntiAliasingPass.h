/*
* Copyright (c) 2014-2021, NVIDIA CORPORATION. All rights reserved.
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

#pragma once


//#include <donut/core/math/math.h>
#include <nvrhi/nvrhi.h>
#include <memory>

/*
namespace donut::engine
{
class ShaderFactory;
class ShadowMap;
class FramebufferFactory;
class ICompositeView;
}

namespace donut::render
{
*/
class CommonRenderPasses;

enum class TemporalAntiAliasingJitter
{
	None,
	MSAA,
	Halton,
	R2,
	WhiteNoise
};

struct TemporalAntiAliasingParameters
{
	float newFrameWeight = 0.1f;
	float clampingFactor = 1.0f;
	float maxRadiance = 10000.f;
	bool enableHistoryClamping = true;
};

struct TemporalAntiAliasingConstants
{
	idRenderMatrix reprojectionMatrix;

	idVec2 inputViewOrigin;
	idVec2 inputViewSize;

	idVec2 outputViewOrigin;
	idVec2 outputViewSize;

	idVec2 inputPixelOffset;
	idVec2 outputTextureSizeInv;

	idVec2 inputOverOutputViewSize;
	idVec2 outputOverInputViewSize;

	float clampingFactor;
	float newFrameWeight;
	float pqC;
	float invPqC;

	uint stencilMask;
};

class TemporalAntiAliasingPass
{
private:
	CommonRenderPasses* m_CommonPasses;

	nvrhi::ShaderHandle m_MotionVectorPS;
	nvrhi::ShaderHandle m_TemporalAntiAliasingCS;
	nvrhi::SamplerHandle m_BilinearSampler;
	nvrhi::BufferHandle m_TemporalAntiAliasingCB;

	nvrhi::BindingLayoutHandle m_MotionVectorsBindingLayout;
	nvrhi::BindingSetHandle m_MotionVectorsBindingSet;
	nvrhi::GraphicsPipelineHandle m_MotionVectorsPso;
	//std::unique_ptr<engine::FramebufferFactory> m_MotionVectorsFramebufferFactory;

	nvrhi::BindingLayoutHandle m_ResolveBindingLayout;
	nvrhi::BindingSetHandle m_ResolveBindingSet;
	nvrhi::BindingSetHandle m_ResolveBindingSetPrevious;
	nvrhi::ComputePipelineHandle m_ResolvePso;

	uint32_t m_FrameIndex;
	uint32_t m_StencilMask;

	idVec2 m_R2Jitter;

public:
	struct CreateParameters
	{
		nvrhi::ITexture* sourceDepth = nullptr;
		nvrhi::ITexture* motionVectors = nullptr;
		nvrhi::ITexture* unresolvedColor = nullptr;
		nvrhi::ITexture* resolvedColor = nullptr;
		nvrhi::ITexture* feedback1 = nullptr;
		nvrhi::ITexture* feedback2 = nullptr;
		bool useCatmullRomFilter = true;
		uint32_t motionVectorStencilMask = 0;
		uint32_t numConstantBufferVersions = 16;
	};

	TemporalAntiAliasingPass();

	void Init(
		nvrhi::IDevice* device,
		//std::shared_ptr<engine::ShaderFactory> shaderFactory,
		CommonRenderPasses* commonPasses,
		const viewDef_t* viewDef,
		const CreateParameters& params );

	void RenderMotionVectors(
		nvrhi::ICommandList* commandList,
		const viewDef_t* viewDef,
		const viewDef_t* viewDefPrevious,
		idVec3 preViewTranslationDifference = vec3_zero );

	void TemporalResolve(
		nvrhi::ICommandList* commandList,
		const TemporalAntiAliasingParameters& params,
		bool feedbackIsValid,
		const viewDef_t* viewDef );

	void AdvanceFrame();
	idVec2 GetCurrentPixelOffset();
};
