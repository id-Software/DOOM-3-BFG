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

#ifndef RENDERER_PASSES_GEOMETRYPASSES_H_
#define RENDERER_PASSES_GEOMETRYPASSES_H_

constexpr std::size_t MAX_IMAGE_PARMS = 16;

#if 0

class IGeometryPass
{
public:
	virtual         ~IGeometryPass() = default;

	virtual void    SetupView( nvrhi::ICommandList* commandList, viewDef_t* viewDef ) = 0;
	virtual bool    SetupMaterial( const idMaterial* material, nvrhi::RasterCullMode cullMode, nvrhi::GraphicsState& state ) = 0;
	virtual void    SetupInputBuffers( const drawSurf_t* drawSurf, nvrhi::GraphicsState& state ) = 0;
	virtual void    SetPushConstants( nvrhi::ICommandList* commandList, nvrhi::GraphicsState& state, nvrhi::DrawArguments& args ) = 0;

protected:

	void PrepareStageTexturing( const shaderStage_t* stage, const drawSurf_t* surf );
	void FinishStageTexturing( const shaderStage_t* stage, const drawSurf_t* surf );

protected:

	int										currentImageParm = 0;
	idArray< idImage*, MAX_IMAGE_PARMS >	imageParms;
	uint64                                  glStateBits;
	nvrhi::GraphicsPipelineDesc             pipelineDesc;
	nvrhi::GraphicsPipelineHandle			pipeline;
	BindingCache				        	bindingCache;
	Framebuffer*                            previousFramebuffer;
	Framebuffer*                            currentFramebuffer;
	Framebuffer*                            lastFramebuffer;
	const viewDef_t*                        viewDef;
	const viewEntity_t*                     currentSpace;
	idScreenRect					        currentViewport;
	idScreenRect					        currentScissor;
	idVec4                                  clearColor;
	float                                   depthClearValue;
	byte                                    stencilClearValue;

	// Updates state to bits in stateBits. Only updates the different bits.
	bool GL_State( uint64 stateBits, bool forceGlState = false );
	void GL_SelectTexture( int textureNum );
	void GL_BindTexture( idImage* img );
	void GL_BindFramebuffer( Framebuffer* framebuffer );
	void GL_BindGraphicsShader( int shader );
	void GL_DepthBoundsTest( const float zmin, const float zmax );
	void GL_PolygonOffset( float scale, float bias );
	void GL_Viewport( int x, int y, int w, int h );
	void GL_Scissor( int x, int y, int w, int h );
	void GL_Color( const idVec4 color );
	void GL_ClearColor( const idVec4 color );
	void GL_ClearDepthStencilValue( float depthValue, byte stencilValue = 0xF );
	void GL_ClearColor( nvrhi::ICommandList* commandList, int attachmentIndex = 0 );
	void GL_ClearDepthStencil( nvrhi::ICommandList* commandList );

	ID_INLINE uint64 GL_GetCurrentState() const
	{
		return glStateBits;
	}

	ID_INLINE void	GL_ViewportAndScissor( int x, int y, int w, int h )
	{
		GL_Viewport( x, y, w, h );
		GL_Scissor( x, y, w, h );
	}
};
#endif

#endif