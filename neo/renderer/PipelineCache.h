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
#ifndef RENDERER_PIPELINECACHE_H_
#define RENDERER_PIPELINECACHE_H_

struct PipelineKey
{
	uint64 state;
	int program;
	int depthBias;
	float slopeBias;

	Framebuffer* framebuffer;
};

inline bool operator==( const PipelineKey& lhs, const PipelineKey& rhs )
{
	return lhs.state == rhs.state &&
		   lhs.program == rhs.program &&
		   lhs.framebuffer == rhs.framebuffer &&
		   lhs.depthBias == rhs.depthBias &&
		   lhs.slopeBias == rhs.slopeBias;
}

template<>
struct std::hash<PipelineKey>
{
	std::size_t operator()( const PipelineKey& key ) const noexcept
	{
		std::size_t h = 0;
		nvrhi::hash_combine( h, key.state );
		nvrhi::hash_combine( h, key.program );
		nvrhi::hash_combine( h, key.framebuffer );
		nvrhi::hash_combine( h, key.depthBias );
		nvrhi::hash_combine( h, key.slopeBias );
		return h;
	}
};

class PipelineCache
{
public:

	PipelineCache();

	void Init( nvrhi::DeviceHandle deviceHandle );
	void Shutdown();

	void Clear();

	nvrhi::GraphicsPipelineHandle GetOrCreatePipeline( const PipelineKey& key );

private:

	void GetRenderState( uint64 stateBits, PipelineKey key, nvrhi::RenderState& renderState );
	nvrhi::DepthStencilState::StencilOpDesc GetStencilOpState( uint64 stateBits );

	nvrhi::DeviceHandle												device;
	idHashIndex														pipelineHash;
	idList<std::pair<PipelineKey, nvrhi::GraphicsPipelineHandle>>	pipelines;
};


#endif
