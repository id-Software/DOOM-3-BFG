/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 2014-2022 Robert Beckebans
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

#include "precompiled.h"
#pragma hdrstop

#include "../RenderCommon.h"
#include "../Framebuffer.h"

#include "sys/DeviceManager.h"

extern DeviceManager* deviceManager;

static void R_ListFramebuffers_f( const idCmdArgs& args )
{
	// TODO
}

Framebuffer::Framebuffer( const char* name, int w, int h )
	: fboName( name )
	, frameBuffer( 0 )
	, colorFormat( 0 )
	, depthBuffer( 0 )
	, depthFormat( 0 )
	, stencilFormat( 0 )
	, stencilBuffer( 0 )
	, width( w )
	, height( h )
	, msaaSamples( false )
{
	framebuffers.Append( this );
}

Framebuffer::Framebuffer( const char* name, const nvrhi::FramebufferDesc& desc )
	: fboName( name )
	, frameBuffer( 0 )
	, colorFormat( 0 )
	, depthBuffer( 0 )
	, depthFormat( 0 )
	, stencilFormat( 0 )
	, stencilBuffer( 0 )
	, msaaSamples( false )
{
	framebuffers.Append( this );
	apiObject = deviceManager->GetDevice()->createFramebuffer( desc );
	width = apiObject->getFramebufferInfo().width;
	height = apiObject->getFramebufferInfo().height;
}

Framebuffer::~Framebuffer()
{
	apiObject.Reset();
}

void Framebuffer::Init()
{
	cmdSystem->AddCommand( "listFramebuffers", R_ListFramebuffers_f, CMD_FL_RENDERER, "lists framebuffers" );

	// HDR
	ResizeFramebuffers();
}

void Framebuffer::CheckFramebuffers()
{
	//int screenWidth = renderSystem->GetWidth();
	//int screenHeight = renderSystem->GetHeight();
}

void Framebuffer::Shutdown()
{
	framebuffers.DeleteContents( true );
}

void Framebuffer::ResizeFramebuffers( bool reloadImages )
{
	tr.backend.ClearCaches();

	// RB: FIXME I think allocating new Framebuffers lead to a memory leak
	framebuffers.DeleteContents( true );

	if( reloadImages )
	{
		ReloadImages();
	}

	uint32_t backBufferCount = deviceManager->GetBackBufferCount();
	globalFramebuffers.swapFramebuffers.Resize( backBufferCount );
	globalFramebuffers.swapFramebuffers.SetNum( backBufferCount );

	for( uint32_t index = 0; index < backBufferCount; index++ )
	{
		globalFramebuffers.swapFramebuffers[index] = new Framebuffer(
			va( "_swapChain%d", index ),
			nvrhi::FramebufferDesc()
			.addColorAttachment( deviceManager->GetBackBuffer( index ) ) );
	}

	for( int arr = 0; arr < 6; arr++ )
	{
		for( int mip = 0; mip < MAX_SHADOWMAP_RESOLUTIONS; mip++ )
		{
			globalFramebuffers.shadowFBO[mip][arr] = new Framebuffer( va( "_shadowMap%i_%i", mip, arr ),
					nvrhi::FramebufferDesc().setDepthAttachment(
						nvrhi::FramebufferAttachment()
						.setTexture( globalImages->shadowImage[mip]->GetTextureHandle().Get() )
						.setArraySlice( arr ) ) );
		}
	}

	globalFramebuffers.shadowAtlasFBO = new Framebuffer( "_shadowAtlas",
			nvrhi::FramebufferDesc()
			.setDepthAttachment( globalImages->shadowAtlasImage->texture ) );

	globalFramebuffers.ldrFBO = new Framebuffer( "_ldr",
			nvrhi::FramebufferDesc()
			.addColorAttachment( globalImages->ldrImage->texture )
			.setDepthAttachment( globalImages->currentDepthImage->texture ) );

	globalFramebuffers.hdrFBO = new Framebuffer( "_hdr",
			nvrhi::FramebufferDesc()
			.addColorAttachment( globalImages->currentRenderHDRImage->texture )
			.setDepthAttachment( globalImages->currentDepthImage->texture ) );

	globalFramebuffers.postProcFBO = new Framebuffer( "_postProc",
			nvrhi::FramebufferDesc()
			.addColorAttachment( globalImages->currentRenderImage->texture ) );

	globalFramebuffers.taaMotionVectorsFBO = new Framebuffer( "_taaMotionVectors",
			nvrhi::FramebufferDesc()
			.addColorAttachment( globalImages->taaMotionVectorsImage->texture ) );

	globalFramebuffers.taaResolvedFBO = new Framebuffer( "_taaResolved",
			nvrhi::FramebufferDesc()
			.addColorAttachment( globalImages->taaResolvedImage->texture ) );

	globalFramebuffers.envprobeFBO = new Framebuffer( "_envprobeRender",
			nvrhi::FramebufferDesc()
			.addColorAttachment( globalImages->envprobeHDRImage->texture )
			.setDepthAttachment( globalImages->envprobeDepthImage->texture ) );

	globalFramebuffers.hdr64FBO = new Framebuffer( "_hdr64",
			nvrhi::FramebufferDesc()
			.addColorAttachment( globalImages->currentRenderHDRImage64->texture ) );

	for( int i = 0; i < MAX_SSAO_BUFFERS; i++ )
	{
		globalFramebuffers.ambientOcclusionFBO[i] = new Framebuffer( va( "_aoRender%i", i ),
				nvrhi::FramebufferDesc()
				.addColorAttachment( globalImages->ambientOcclusionImage[i]->texture ) );
	}

	// HIERARCHICAL Z BUFFER
	for( int i = 0; i < MAX_HIERARCHICAL_ZBUFFERS; i++ )
	{
		globalFramebuffers.csDepthFBO[i] = new Framebuffer( va( "_csz%d", i ),
				nvrhi::FramebufferDesc().addColorAttachment(
					nvrhi::FramebufferAttachment()
					.setTexture( globalImages->hierarchicalZbufferImage->texture )
					.setMipLevel( i ) ) );
	}

	globalFramebuffers.geometryBufferFBO = new Framebuffer( "_gbuffer",
			nvrhi::FramebufferDesc()
			.addColorAttachment( globalImages->gbufferNormalsRoughnessImage->texture )
			.setDepthAttachment( globalImages->currentDepthImage->texture ) );

	globalFramebuffers.smaaEdgesFBO = new Framebuffer( "_smaaEdges",
			nvrhi::FramebufferDesc()
			.addColorAttachment( globalImages->smaaEdgesImage->texture ) );

	globalFramebuffers.smaaBlendFBO = new Framebuffer( "_smaaBlend",
			nvrhi::FramebufferDesc()
			.addColorAttachment( globalImages->smaaBlendImage->texture ) );

	for( int i = 0; i < MAX_BLOOM_BUFFERS; i++ )
	{
		globalFramebuffers.bloomRenderFBO[i] = new Framebuffer( va( "_bloomRender%i", i ),
				nvrhi::FramebufferDesc()
				.addColorAttachment( globalImages->bloomRenderImage[i]->texture ) );
	}

	globalFramebuffers.guiRenderTargetFBO = new Framebuffer( "_guiEdit",
			nvrhi::FramebufferDesc()
			.addColorAttachment( globalImages->guiEdit->texture )
			.setDepthAttachment( globalImages->guiEditDepthStencilImage->texture ) );

	Framebuffer::Unbind();
}

void Framebuffer::ReloadImages()
{
	tr.backend.commandList->open();
	globalImages->ldrImage->Reload( false, tr.backend.commandList );
	globalImages->currentRenderImage->Reload( false, tr.backend.commandList );
	globalImages->currentDepthImage->Reload( false, tr.backend.commandList );
	globalImages->currentRenderHDRImage->Reload( false, tr.backend.commandList );
	globalImages->currentRenderHDRImage64->Reload( false, tr.backend.commandList );
	for( int i = 0; i < MAX_SSAO_BUFFERS; i++ )
	{
		globalImages->ambientOcclusionImage[i]->Reload( false, tr.backend.commandList );
	}
	globalImages->hierarchicalZbufferImage->Reload( false, tr.backend.commandList );
	globalImages->gbufferNormalsRoughnessImage->Reload( false, tr.backend.commandList );
	globalImages->taaMotionVectorsImage->Reload( false, tr.backend.commandList );
	globalImages->taaResolvedImage->Reload( false, tr.backend.commandList );
	globalImages->taaFeedback1Image->Reload( false, tr.backend.commandList );
	globalImages->taaFeedback2Image->Reload( false, tr.backend.commandList );
	globalImages->smaaEdgesImage->Reload( false, tr.backend.commandList );
	globalImages->smaaBlendImage->Reload( false, tr.backend.commandList );
	globalImages->shadowAtlasImage->Reload( false, tr.backend.commandList );
	for( int i = 0; i < MAX_SHADOWMAP_RESOLUTIONS; i++ )
	{
		globalImages->shadowImage[i]->Reload( false, tr.backend.commandList );
	}
	for( int i = 0; i < MAX_BLOOM_BUFFERS; i++ )
	{
		globalImages->bloomRenderImage[i]->Reload( false, tr.backend.commandList );
	}
	globalImages->guiEdit->Reload( false, tr.backend.commandList );
	tr.backend.commandList->close();
	deviceManager->GetDevice()->executeCommandList( tr.backend.commandList );
}

void Framebuffer::Bind()
{
	if( tr.backend.currentFrameBuffer != this )
	{
		tr.backend.currentPipeline = nullptr;
	}

	tr.backend.lastFrameBuffer = tr.backend.currentFrameBuffer;
	tr.backend.currentFrameBuffer = this;
}

bool Framebuffer::IsBound()
{
	return tr.backend.currentFrameBuffer == this;
}

void Framebuffer::Unbind()
{
	globalFramebuffers.swapFramebuffers[deviceManager->GetCurrentBackBufferIndex()]->Bind();
}

bool Framebuffer::IsDefaultFramebufferActive()
{
	return tr.backend.currentFrameBuffer == globalFramebuffers.swapFramebuffers[deviceManager->GetCurrentBackBufferIndex()];
}

Framebuffer* Framebuffer::GetActiveFramebuffer()
{
	return tr.backend.currentFrameBuffer;
}

void Framebuffer::AddColorBuffer( int format, int index, int multiSamples )
{
}

void Framebuffer::AddDepthBuffer( int format, int multiSamples )
{
}

void Framebuffer::AddStencilBuffer( int format, int multiSamples )
{
}

void Framebuffer::AttachImage2D( int target, idImage* image, int index, int mipmapLod )
{
}

void Framebuffer::AttachImageDepth( int target, idImage* image )
{
}

void Framebuffer::AttachImageDepthLayer( idImage* image, int layer )
{
}

void Framebuffer::Check()
{
}

idScreenRect Framebuffer::GetViewPortInfo() const
{
	nvrhi::Viewport viewport = apiObject->getFramebufferInfo().getViewport();
	idScreenRect screenRect;
	screenRect.Clear();
	screenRect.AddPoint( viewport.minX, viewport.minY );
	screenRect.AddPoint( viewport.maxX, viewport.maxY );
	return screenRect;
}