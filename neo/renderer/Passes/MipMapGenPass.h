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
#ifndef RENDERER_PASSES_MIPMAPGENPASS_H_
#define RENDERER_PASSES_MIPMAPGENPASS_H_

class CommonRenderPasses;

class MipMapGenPass
{

public:

	enum Mode : uint8_t
	{
		MODE_COLOR = 0,  // bilinear reduction of RGB channels
		MODE_MIN = 1,    // min() reduction of R channel
		MODE_MAX = 2,    // max() reduction of R channel
		MODE_MINMAX = 3, // min() and max() reductions of R channel into RG channels
	};

	// note : 'texture' must have been allocated with some mip levels
	MipMapGenPass(
		nvrhi::IDevice* device,
		nvrhi::TextureHandle texture,
		Mode mode = Mode::MODE_MAX );

	// Dispatches reduction kernel : reads LOD 0 and populates
	// LOD 1 and up
	void Dispatch( nvrhi::ICommandList* commandList, int maxLOD = -1 );

	// debug : blits mip-map levels in spiral pattern to 'target'
	// (assumes 'target' texture resolution is high enough...)
	void Display(
		CommonRenderPasses& commonPasses,
		nvrhi::ICommandList* commandList,
		nvrhi::IFramebuffer* target );

private:

	nvrhi::DeviceHandle m_Device;
	nvrhi::ShaderHandle m_Shader;
	nvrhi::TextureHandle m_Texture;
	nvrhi::BufferHandle m_ConstantBuffer;
	nvrhi::BindingLayoutHandle m_BindingLayout;
	idList<nvrhi::BindingSetHandle> m_BindingSets;
	nvrhi::ComputePipelineHandle m_Pso;

	// Set of unique dummy textures - see details in class implementation
	struct NullTextures;
	std::shared_ptr<NullTextures> m_NullTextures;

	BindingCache m_BindingCache;
};

#endif