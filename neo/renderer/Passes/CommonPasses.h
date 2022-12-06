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
#ifndef COMMON_PASSES_H_
#define COMMON_PASSES_H_

#include <nvrhi/nvrhi.h>
#include <memory>
#include <unordered_map>

class BindingCache;
class ShaderFactory;

constexpr uint32_t c_MaxRenderPassConstantBufferVersions = 16;

enum class BlitSampler
{
	Point,
	Linear,
	Sharpen
};

struct BlitParameters
{
	nvrhi::IFramebuffer* targetFramebuffer = nullptr;
	nvrhi::Viewport targetViewport;
	idVec4 targetBox = idVec4( 0.f, 0.f, 1.f, 1.f );

	nvrhi::ITexture* sourceTexture = nullptr;
	uint32_t sourceArraySlice = 0;
	uint32_t sourceMip = 0;
	idVec4 sourceBox = idVec4( 0.f, 0.f, 1.f, 1.f );

	BlitSampler sampler = BlitSampler::Linear;
	nvrhi::BlendState::RenderTarget blendState;
	nvrhi::Color blendConstantColor = nvrhi::Color( 0.f );
};

struct BlitConstants
{
	idVec2  sourceOrigin;
	idVec2  sourceSize;

	idVec2  targetOrigin;
	idVec2  targetSize;

	float   sharpenFactor;
};

class CommonRenderPasses
{
protected:
	nvrhi::DeviceHandle m_Device;

	struct PsoCacheKey
	{
		nvrhi::FramebufferInfoEx fbinfo;
		nvrhi::IShader* shader;
		nvrhi::BlendState::RenderTarget blendState;

		bool operator==( const PsoCacheKey& other ) const
		{
			return fbinfo == other.fbinfo && shader == other.shader && blendState == other.blendState;
		}
		bool operator!=( const PsoCacheKey& other ) const
		{
			return !( *this == other );
		}

		struct Hash
		{
			size_t operator()( const PsoCacheKey& s ) const
			{
				size_t hash = 0;
				nvrhi::hash_combine( hash, s.fbinfo );
				nvrhi::hash_combine( hash, s.shader );
				nvrhi::hash_combine( hash, s.blendState );
				return hash;
			}
		};
	};

	std::unordered_map<PsoCacheKey, nvrhi::GraphicsPipelineHandle, PsoCacheKey::Hash> m_BlitPsoCache;

public:
	nvrhi::ShaderHandle m_RectVS;
	nvrhi::ShaderHandle m_BlitPS;
	nvrhi::ShaderHandle m_BlitArrayPS;
	nvrhi::ShaderHandle m_SharpenPS;
	nvrhi::ShaderHandle m_SharpenArrayPS;

	nvrhi::TextureHandle m_BlackTexture;
	nvrhi::TextureHandle m_GrayTexture;
	nvrhi::TextureHandle m_WhiteTexture;
	nvrhi::TextureHandle m_BlackTexture2DArray;
	nvrhi::TextureHandle m_WhiteTexture2DArray;
	nvrhi::TextureHandle m_BlackCubeMapArray;

	nvrhi::SamplerHandle m_PointClampSampler;
	nvrhi::SamplerHandle m_PointWrapSampler;
	nvrhi::SamplerHandle m_LinearClampSampler;
	nvrhi::SamplerHandle m_LinearBorderSampler; // D3 zeroClamp
	nvrhi::SamplerHandle m_LinearClampCompareSampler;
	nvrhi::SamplerHandle m_LinearWrapSampler;
	nvrhi::SamplerHandle m_AnisotropicWrapSampler;
	nvrhi::SamplerHandle m_AnisotropicClampEdgeSampler;

	nvrhi::BindingLayoutHandle m_BlitBindingLayout;

	CommonRenderPasses();

	void	Init( nvrhi::IDevice* device );
	void	Shutdown();

	void	BlitTexture( nvrhi::ICommandList* commandList, const BlitParameters& params, BindingCache* bindingCache = nullptr );

	// Simplified form of BlitTexture that blits the entire source texture, mip 0 slice 0, into the entire target framebuffer using a linear sampler.
	void	BlitTexture( nvrhi::ICommandList* commandList, nvrhi::IFramebuffer* targetFramebuffer, nvrhi::ITexture* sourceTexture, BindingCache* bindingCache = nullptr );
};

#endif
