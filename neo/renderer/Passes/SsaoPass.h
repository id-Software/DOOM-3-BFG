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
#ifndef RENDERER_PASSES_SSAOPASS_H_
#define RENDERER_PASSES_SSAOPASS_H_

class SsaoPass
{
private:
	struct SubPass
	{
		nvrhi::ShaderHandle Shader;
		nvrhi::BindingLayoutHandle BindingLayout;
		std::vector<nvrhi::BindingSetHandle> BindingSets;
		nvrhi::ComputePipelineHandle Pipeline;
	};

	SubPass m_Deinterleave;
	SubPass m_Compute;
	SubPass m_Blur;

	nvrhi::DeviceHandle m_Device;
	nvrhi::BufferHandle m_ConstantBuffer;
	CommonRenderPasses* commonRenderPasses;

	nvrhi::TextureHandle m_DeinterleavedDepth;
	nvrhi::TextureHandle m_DeinterleavedOcclusion;
	idVec2 m_QuantizedGbufferTextureSize;

public:
	struct CreateParameters
	{
		idVec2 dimensions;
		bool inputLinearDepth = false;
		bool octEncodedNormals = false;
		bool directionalOcclusion = false;
		int numBindingSets = 1;
	};

	SsaoPass(
		nvrhi::IDevice* device,
		const CreateParameters& params,
		CommonRenderPasses* commonRenderPasses );

	SsaoPass(
		nvrhi::IDevice* device,
		CommonRenderPasses* commonPasses,
		nvrhi::ITexture* gbufferDepth,
		nvrhi::ITexture* gbufferNormals,
		nvrhi::ITexture* destinationTexture );

	void CreateBindingSet(
		nvrhi::ITexture* gbufferDepth,
		nvrhi::ITexture* gbufferNormals,
		nvrhi::ITexture* destinationTexture,
		int bindingSetIndex = 0 );

	void Render(
		nvrhi::ICommandList* commandList,
		const viewDef_t* viewDef,
		int bindingSetIndex = 0 );
};

#endif
