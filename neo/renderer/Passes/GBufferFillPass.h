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
#ifndef RENDERER_PASSES_GBUFFERFILLPASS_H_
#define RENDERER_PASSES_GBUFFERFILLPASS_H_

#include "GeometryPasses.h"

#if 0

// "Light" G-Buffer that renders the normals of the geometry
class GBufferFillPass : IGeometryPass
{
public:

	GBufferFillPass() = default;
	virtual ~GBufferFillPass() = default;

	void Init( nvrhi::DeviceHandle deviceHandle );
	void RenderView( nvrhi::ICommandList* commandList, const drawSurf_t* const* drawSurfs, int numDrawSurfs, bool fillGbuffer );

protected:

	nvrhi::DeviceHandle			device;
	nvrhi::BindingLayoutHandle	geometryBindingLayout;
	nvrhi::BindingLayoutHandle  texturedBindingLayout;
	nvrhi::BindingSetDesc		geometryBindingSetDesc;

	nvrhi::GraphicsPipelineHandle CreateGraphicsPipeline( nvrhi::IFramebuffer* framebuffer );

	void DrawElementsWithCounters( const drawSurf_t* surf );

public:

	void SetupView( nvrhi::ICommandList* commandList, viewDef_t* viewDef ) override;
	bool SetupMaterial( const idMaterial* material, nvrhi::RasterCullMode cullMode, nvrhi::GraphicsState& state ) override;
	void SetupInputBuffers( const drawSurf_t* drawSurf, nvrhi::GraphicsState& state ) override;
	void SetPushConstants( nvrhi::ICommandList* commandList, nvrhi::GraphicsState& state, nvrhi::DrawArguments& args ) override;

};

#endif

#endif