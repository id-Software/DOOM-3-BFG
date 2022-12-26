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
#ifndef RENDERER_PASSES_SSAOPASS_H_
#define RENDERER_PASSES_SSAOPASS_H_

struct SsaoParameters
{
	float amount = 2.f;
	float backgroundViewDepth = 100.f;
	float radiusWorld = 0.5f;
	float surfaceBias = 0.1f;
	float powerExponent = 2.f;
	bool enableBlur = true;
	float blurSharpness = 16.f;
};

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
		const SsaoParameters& params,
		viewDef_t* viewDef,
		int bindingSetIndex = 0 );
};

#endif