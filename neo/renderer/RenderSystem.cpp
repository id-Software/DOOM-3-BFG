/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013-2023 Robert Beckebans
Copyright (C) 2014-2016 Kot in Action Creative Artel

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

#include "RenderCommon.h"
#include "../framework/Common_local.h"
#include "../imgui/BFGimgui.h"

#include <sys/DeviceManager.h>
extern DeviceManager* deviceManager;


idRenderSystemLocal	tr;
idRenderSystem* renderSystem = &tr;

/*
=====================
R_PerformanceCounters

This prints both front and back end counters, so it should
only be called when the back end thread is idle.
=====================
*/
void idRenderSystemLocal::PrintPerformanceCounters()
{
	if( r_showPrimitives.GetInteger() != 0 )
	{
		common->Printf( "views:%i draws:%i tris:%i (shdw:%i)\n",
						pc.c_numViews,
						backend.pc.c_drawElements + backend.pc.c_shadowElements,
						( backend.pc.c_drawIndexes + backend.pc.c_shadowIndexes ) / 3,
						backend.pc.c_shadowIndexes / 3
					  );
	}

	if( r_showDynamic.GetBool() )
	{
		common->Printf( "callback:%i md5:%i dfrmVerts:%i dfrmTris:%i tangTris:%i guis:%i\n",
						pc.c_entityDefCallbacks,
						pc.c_generateMd5,
						pc.c_deformedVerts,
						pc.c_deformedIndexes / 3,
						pc.c_tangentIndexes / 3,
						pc.c_guiSurfs
					  );
	}

	if( r_showCull.GetBool() )
	{
		common->Printf( "%i box in %i box out\n",
						pc.c_box_cull_in, pc.c_box_cull_out );
	}

	if( r_showAddModel.GetBool() )
	{
		common->Printf( "callback:%i createInteractions:%i createShadowVolumes:%i\n",
						pc.c_entityDefCallbacks, pc.c_createInteractions, pc.c_createShadowVolumes );
		common->Printf( "viewEntities:%i  shadowEntities:%i  viewLights:%i\n", pc.c_visibleViewEntities,
						pc.c_shadowViewEntities, pc.c_viewLights );
	}
	if( r_showUpdates.GetBool() )
	{
		common->Printf( "entityUpdates:%i  entityRefs:%i  lightUpdates:%i  lightRefs:%i\n",
						pc.c_entityUpdates, pc.c_entityReferences,
						pc.c_lightUpdates, pc.c_lightReferences );
	}
	if( r_showMemory.GetBool() )
	{
		common->Printf( "frameData: %i (%i)\n", frameData->frameMemoryAllocated.GetValue(), frameData->highWaterAllocated );
	}

	memset( &pc, 0, sizeof( pc ) );
	memset( &backend.pc, 0, sizeof( backend.pc ) );
}

/*
====================
RenderCommandBuffers
====================
*/
void idRenderSystemLocal::RenderCommandBuffers( const emptyCommand_t* const cmdHead )
{
	// if there isn't a draw view command, do nothing to avoid swapping a bad frame
	bool	hasView = false;
	for( const emptyCommand_t* cmd = cmdHead ; cmd ; cmd = ( const emptyCommand_t* )cmd->next )
	{
		if( cmd->commandId == RC_DRAW_VIEW_3D || cmd->commandId == RC_DRAW_VIEW_GUI )
		{
			hasView = true;
			break;
		}
	}
	if( !hasView )
	{
		return;
	}

	// r_skipBackEnd allows the entire time of the back end
	// to be removed from performance measurements, although
	// nothing will be drawn to the screen.  If the prints
	// are going to a file, or r_skipBackEnd is later disabled,
	// usefull data can be received.

	// r_skipRender is usually more usefull, because it will still
	// draw 2D graphics
	if( !r_skipBackEnd.GetBool() )
	{
		backend.ExecuteBackEndCommands( cmdHead );
	}

	// pass in null for now - we may need to do some map specific hackery in the future
	resolutionScale.InitForMap( NULL );
}

/*
============
R_GetCommandBuffer

Returns memory for a command buffer (stretchPicCommand_t,
drawSurfsCommand_t, etc) and links it to the end of the
current command chain.
============
*/
void* R_GetCommandBuffer( int bytes )
{
	emptyCommand_t*	cmd;

	cmd = ( emptyCommand_t* )R_FrameAlloc( bytes, FRAME_ALLOC_DRAW_COMMAND );
	cmd->next = NULL;
	frameData->cmdTail->next = &cmd->commandId;
	frameData->cmdTail = cmd;

	return ( void* )cmd;
}

/*
=================
R_ViewStatistics
=================
*/
static void R_ViewStatistics( viewDef_t* parms )
{
	// report statistics about this view
	if( !r_showSurfaces.GetBool() )
	{
		return;
	}
	common->Printf( "view:%p surfs:%i\n", parms, parms->numDrawSurfs );
}

/*
=============
R_AddDrawViewCmd

This is the main 3D rendering command.  A single scene may
have multiple views if a mirror, portal, or dynamic texture is present.
=============
*/
void	R_AddDrawViewCmd( viewDef_t* parms, bool guiOnly )
{
	drawSurfsCommand_t*	cmd;

	cmd = ( drawSurfsCommand_t* )R_GetCommandBuffer( sizeof( *cmd ) );
	cmd->commandId = ( guiOnly ) ? RC_DRAW_VIEW_GUI : RC_DRAW_VIEW_3D;

	cmd->viewDef = parms;

	tr.pc.c_numViews++;

	R_ViewStatistics( parms );
}

/*
=============
R_AddPostProcess

This issues the command to do a post process after all the views have
been rendered.
=============
*/
void	R_AddDrawPostProcess( viewDef_t* parms )
{
	postProcessCommand_t* cmd = ( postProcessCommand_t* )R_GetCommandBuffer( sizeof( *cmd ) );
	cmd->commandId = RC_POST_PROCESS;
	cmd->viewDef = parms;
}


//=================================================================================




/*
=============
idRenderSystemLocal::idRenderSystemLocal
=============
*/
idRenderSystemLocal::idRenderSystemLocal() :
	unitSquareTriangles( NULL ),
	zeroOneCubeTriangles( NULL ),
	zeroOneSphereTriangles( NULL ),
	testImageTriangles( NULL ),
	bInitialized( false ),
	omitSwapBuffers( false )
{
	Clear();
}

/*
=============
idRenderSystemLocal::~idRenderSystemLocal
=============
*/
idRenderSystemLocal::~idRenderSystemLocal()
{
}

/*
=============
idRenderSystemLocal::SetColor
=============
*/
void idRenderSystemLocal::SetColor( const idVec4& rgba )
{
	currentColorNativeBytesOrder = LittleLong( PackColor( rgba ) );
}

/*
=============
idRenderSystemLocal::GetColor
=============
*/
uint32 idRenderSystemLocal::GetColor()
{
	return LittleLong( currentColorNativeBytesOrder );
}

/*
=============
idRenderSystemLocal::SetGLState
=============
*/
void idRenderSystemLocal::SetGLState( const uint64 glState )
{
	currentGLState = glState;
}

/*
=============
idRenderSystemLocal::DrawFilled
=============
*/
void idRenderSystemLocal::DrawFilled( const idVec4& color, float x, float y, float w, float h )
{
	SetColor( color );
	DrawStretchPic( x, y, w, h, 0.0f, 0.0f, 1.0f, 1.0f, whiteMaterial );
}

/*
=============
idRenderSystemLocal::DrawStretchPic
=============
*/
void idRenderSystemLocal::DrawStretchPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, const idMaterial* material, float z )
{
	DrawStretchPic( idVec4( x, y, s1, t1 ), idVec4( x + w, y, s2, t1 ), idVec4( x + w, y + h, s2, t2 ), idVec4( x, y + h, s1, t2 ), material, z );
}

/*
=============
idRenderSystemLocal::DrawStretchPic
=============
*/
static triIndex_t quadPicIndexes[6] = { 3, 0, 2, 2, 0, 1 };
void idRenderSystemLocal::DrawStretchPic( const idVec4& topLeft, const idVec4& topRight, const idVec4& bottomRight, const idVec4& bottomLeft, const idMaterial* material, float z )
{
	if( !IsInitialized() )
	{
		return;
	}
	if( material == NULL )
	{
		return;
	}

	idDrawVert* verts = guiModel->AllocTris( 4, quadPicIndexes, 6, material, currentGLState, STEREO_DEPTH_TYPE_NONE );
	if( verts == NULL )
	{
		return;
	}

	ALIGNTYPE16 idDrawVert localVerts[4];

	localVerts[0].Clear();
	localVerts[0].xyz[0] = topLeft.x;
	localVerts[0].xyz[1] = topLeft.y;
	localVerts[0].xyz[2] = z;
	localVerts[0].SetTexCoord( topLeft.z, topLeft.w );
	localVerts[0].SetNativeOrderColor( currentColorNativeBytesOrder );
	localVerts[0].ClearColor2();

	localVerts[1].Clear();
	localVerts[1].xyz[0] = topRight.x;
	localVerts[1].xyz[1] = topRight.y;
	localVerts[1].xyz[2] = z;
	localVerts[1].SetTexCoord( topRight.z, topRight.w );
	localVerts[1].SetNativeOrderColor( currentColorNativeBytesOrder );
	localVerts[1].ClearColor2();

	localVerts[2].Clear();
	localVerts[2].xyz[0] = bottomRight.x;
	localVerts[2].xyz[1] = bottomRight.y;
	localVerts[2].xyz[2] = z;
	localVerts[2].SetTexCoord( bottomRight.z, bottomRight.w );
	localVerts[2].SetNativeOrderColor( currentColorNativeBytesOrder );
	localVerts[2].ClearColor2();

	localVerts[3].Clear();
	localVerts[3].xyz[0] = bottomLeft.x;
	localVerts[3].xyz[1] = bottomLeft.y;
	localVerts[3].xyz[2] = z;
	localVerts[3].SetTexCoord( bottomLeft.z, bottomLeft.w );
	localVerts[3].SetNativeOrderColor( currentColorNativeBytesOrder );
	localVerts[3].ClearColor2();

	WriteDrawVerts16( verts, localVerts, 4 );
}

/*
=============
idRenderSystemLocal::DrawStretchTri
=============
*/
void idRenderSystemLocal::DrawStretchTri( const idVec2& p1, const idVec2& p2, const idVec2& p3, const idVec2& t1, const idVec2& t2, const idVec2& t3, const idMaterial* material )
{
	if( !IsInitialized() )
	{
		return;
	}
	if( material == NULL )
	{
		return;
	}

	triIndex_t tempIndexes[3] = { 1, 0, 2 };

	idDrawVert* verts = guiModel->AllocTris( 3, tempIndexes, 3, material, currentGLState, STEREO_DEPTH_TYPE_NONE );
	if( verts == NULL )
	{
		return;
	}

	ALIGNTYPE16 idDrawVert localVerts[3];

	localVerts[0].Clear();
	localVerts[0].xyz[0] = p1.x;
	localVerts[0].xyz[1] = p1.y;
	localVerts[0].SetTexCoord( t1 );
	localVerts[0].SetNativeOrderColor( currentColorNativeBytesOrder );
	localVerts[0].ClearColor2();

	localVerts[1].Clear();
	localVerts[1].xyz[0] = p2.x;
	localVerts[1].xyz[1] = p2.y;
	localVerts[1].SetTexCoord( t2 );
	localVerts[1].SetNativeOrderColor( currentColorNativeBytesOrder );
	localVerts[1].ClearColor2();

	localVerts[2].Clear();
	localVerts[2].xyz[0] = p3.x;
	localVerts[2].xyz[1] = p3.y;
	localVerts[2].SetTexCoord( t3 );
	localVerts[2].SetNativeOrderColor( currentColorNativeBytesOrder );
	localVerts[2].ClearColor2();

	WriteDrawVerts16( verts, localVerts, 3 );
}

/*
=============
idRenderSystemLocal::AllocTris
=============
*/
idDrawVert* idRenderSystemLocal::AllocTris( int numVerts, const triIndex_t* indexes, int numIndexes, const idMaterial* material, const stereoDepthType_t stereoType )
{
	return guiModel->AllocTris( numVerts, indexes, numIndexes, material, currentGLState, stereoType );
}

/*
=====================
idRenderSystemLocal::DrawSmallChar

small chars are drawn at native screen resolution
=====================
*/
void idRenderSystemLocal::DrawSmallChar( int x, int y, int ch )
{
	int row, col;
	float frow, fcol;
	float size;

	ch &= 255;

	if( ch == ' ' )
	{
		return;
	}

	if( y < -SMALLCHAR_HEIGHT )
	{
		return;
	}

	row = ch >> 4;
	col = ch & 15;

	frow = row * 0.0625f;
	fcol = col * 0.0625f;
	size = 0.0625f;

	DrawStretchPic( x, y, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT,
					fcol, frow,
					fcol + size, frow + size,
					charSetMaterial );
}

/*
==================
idRenderSystemLocal::DrawSmallStringExt

Draws a multi-colored string with a drop shadow, optionally forcing
to a fixed color.

Coordinates are at 640 by 480 virtual resolution
==================
*/
void idRenderSystemLocal::DrawSmallStringExt( int x, int y, const char* string, const idVec4& setColor, bool forceColor )
{
	idVec4		color;
	const unsigned char*	s;
	int			xx;

	// draw the colored text
	s = ( const unsigned char* )string;
	xx = x;
	SetColor( setColor );
	while( *s )
	{
		if( idStr::IsColor( ( const char* )s ) )
		{
			if( !forceColor )
			{
				if( *( s + 1 ) == C_COLOR_DEFAULT )
				{
					SetColor( setColor );
				}
				else
				{
					color = idStr::ColorForIndex( *( s + 1 ) );
					color[3] = setColor[3];
					SetColor( color );
				}
			}
			s += 2;
			continue;
		}
		DrawSmallChar( xx, y, *s );
		xx += SMALLCHAR_WIDTH;
		s++;
	}
	SetColor( colorWhite );
}

/*
=====================
idRenderSystemLocal::DrawBigChar
=====================
*/
void idRenderSystemLocal::DrawBigChar( int x, int y, int ch )
{
	int row, col;
	float frow, fcol;
	float size;

	ch &= 255;

	if( ch == ' ' )
	{
		return;
	}

	if( y < -BIGCHAR_HEIGHT )
	{
		return;
	}

	row = ch >> 4;
	col = ch & 15;

	frow = row * 0.0625f;
	fcol = col * 0.0625f;
	size = 0.0625f;

	DrawStretchPic( x, y, BIGCHAR_WIDTH, BIGCHAR_HEIGHT,
					fcol, frow,
					fcol + size, frow + size,
					charSetMaterial );
}

/*
==================
idRenderSystemLocal::DrawBigStringExt

Draws a multi-colored string with a drop shadow, optionally forcing
to a fixed color.

Coordinates are at 640 by 480 virtual resolution
==================
*/
void idRenderSystemLocal::DrawBigStringExt( int x, int y, const char* string, const idVec4& setColor, bool forceColor )
{
	idVec4		color;
	const char*	s;
	int			xx;

	// draw the colored text
	s = string;
	xx = x;
	SetColor( setColor );
	while( *s )
	{
		if( idStr::IsColor( s ) )
		{
			if( !forceColor )
			{
				if( *( s + 1 ) == C_COLOR_DEFAULT )
				{
					SetColor( setColor );
				}
				else
				{
					color = idStr::ColorForIndex( *( s + 1 ) );
					color[3] = setColor[3];
					SetColor( color );
				}
			}
			s += 2;
			continue;
		}
		DrawBigChar( xx, y, *s );
		xx += BIGCHAR_WIDTH;
		s++;
	}
	SetColor( colorWhite );
}

//======================================================================================

/*
====================
idRenderSystemLocal::SwapCommandBuffers

Performs final closeout of any gui models being defined.

Waits for the previous GPU rendering to complete and vsync.

Returns the head of the linked command list that was just closed off.

Returns timing information from the previous frame.

After this is called, new command buffers can be built up in parallel
with the rendering of the closed off command buffers by RenderCommandBuffers()
====================
*/
const emptyCommand_t* idRenderSystemLocal::SwapCommandBuffers(
	uint64* frontEndMicroSec,
	uint64* backEndMicroSec,
	uint64* shadowMicroSec,
	uint64* gpuMicroSec,
	backEndCounters_t* bc,
	performanceCounters_t* pc
)
{
	SwapCommandBuffers_FinishRendering( frontEndMicroSec, backEndMicroSec, shadowMicroSec, gpuMicroSec, bc, pc );

	return SwapCommandBuffers_FinishCommandBuffers();
}

/*
=====================
idRenderSystemLocal::SwapCommandBuffers_FinishRendering
=====================
*/
#define	FPS_FRAMES	6
void idRenderSystemLocal::SwapCommandBuffers_FinishRendering(
	uint64* frontEndMicroSec,
	uint64* backEndMicroSec,
	uint64* shadowMicroSec,
	uint64* gpuMicroSec,
	backEndCounters_t* bc,
	performanceCounters_t* pc
)
{
	SCOPED_PROFILE_EVENT( "SwapCommandBuffers_FinishRendering" );

	if( gpuMicroSec != NULL )
	{
		*gpuMicroSec = 0;		// until shown otherwise
	}

	if( !IsInitialized() )
	{
		return;
	}

	//GL_CheckErrors();

	// After coming back from an autoswap, we won't have anything to render
	//if( frameData && frameData->cmdHead->next != NULL )

	// keep capturing envprobes completely in the background
	// and only update the screen when we update the progress bar in the console
	if( !omitSwapBuffers )
	{

#if !IMGUI_BFGUI
		ImGuiHook::Render();
#endif

		// wait for our fence to hit, which means the swap has actually happened
		// We must do this before clearing any resources the GPU may be using
		backend.GL_BlockingSwapBuffers();
	}

	if( gpuMicroSec != NULL )
	{
		*gpuMicroSec = backend.pc.gpuMicroSec;
	}

	//------------------------------

	// save out timing information
	if( frontEndMicroSec != NULL )
	{
		*frontEndMicroSec = this->pc.frontEndMicroSec;
	}

	if( backEndMicroSec != NULL )
	{
		*backEndMicroSec = backend.pc.cpuTotalMicroSec;
	}

	if( shadowMicroSec != NULL )
	{
		*shadowMicroSec = backend.pc.cpuShadowMicroSec;
	}

	// RB: TODO clean up the above and just pass entire backend and performance stats before they get cleared
	if( bc != NULL )
	{
		*bc = backend.pc;
	}

	if( pc != NULL )
	{
		*pc = this->pc;
	}

	// print any other statistics and clear all of them
	PrintPerformanceCounters();

	// check for dynamic changes that require some initialization
	backend.CheckCVars();

	// RB: resize HDR buffers
	Framebuffer::CheckFramebuffers();

	// check for errors
	GL_CheckErrors();
}

/*
=====================
idRenderSystemLocal::SwapCommandBuffers_FinishCommandBuffers
=====================
*/
const emptyCommand_t* idRenderSystemLocal::SwapCommandBuffers_FinishCommandBuffers()
{
	OPTICK_EVENT( "SwapCommandBuffers_FinishCommandBuffers" );

	if( !IsInitialized() )
	{
		return NULL;
	}

	// RB: general GUI system path to treat ImGui surfaces in the renderer frontend like SWF
	// this calls io.RenderDrawListsFn
#if IMGUI_BFGUI
	ImGuiHook::Render();
#endif

	// close any gui drawing
	guiModel->EmitFullScreen();
	guiModel->Clear();

	// unmap the buffer objects so they can be used by the GPU
	vertexCache.BeginBackEnd();

	// save off this command buffer
	const emptyCommand_t* commandBufferHead = frameData->cmdHead;

	// copy the code-used drawsurfs that were
	// allocated at the start of the buffer memory to the backEnd referenced locations
	backend.unitSquareSurface = tr.unitSquareSurface_;
	backend.zeroOneCubeSurface = tr.zeroOneCubeSurface_;
	backend.zeroOneSphereSurface = tr.zeroOneSphereSurface_;
	backend.testImageSurface = tr.testImageSurface_;

	// use the other buffers next frame, because another CPU
	// may still be rendering into the current buffers
	R_ToggleSmpFrame();

	// possibly change the stereo3D mode
	// PC
	UpdateStereo3DMode();

	if( !commandList )
	{
		commandList = deviceManager->GetDevice()->createCommandList();
	}

	// prepare the new command buffer
	guiModel->BeginFrame();

	//------------------------------
	// Make sure that geometry used by code is present in the buffer cache.
	// These use frame buffer cache (not static) because they may be used during
	// map loads.
	//
	// It is important to do this first, so if the buffers overflow during
	// scene generation, the basic surfaces needed for drawing the buffers will
	// always be present.
	//------------------------------
	R_InitDrawSurfFromTri( tr.unitSquareSurface_, *tr.unitSquareTriangles, commandList );
	R_InitDrawSurfFromTri( tr.zeroOneCubeSurface_, *tr.zeroOneCubeTriangles, commandList );
	R_InitDrawSurfFromTri( tr.zeroOneSphereSurface_, *tr.zeroOneSphereTriangles, commandList );
	R_InitDrawSurfFromTri( tr.testImageSurface_, *tr.testImageTriangles, commandList );

	// Reset render crop to be the full screen
	renderCrops[0].x1 = 0;
	renderCrops[0].y1 = 0;
	renderCrops[0].x2 = GetWidth() - 1;
	renderCrops[0].y2 = GetHeight() - 1;
	currentRenderCrop = 0;

	// this is the ONLY place this is modified
	frameCount++;

	// just in case we did a common->Error while this
	// was set
	guiRecursionLevel = 0;

	// the first rendering will be used for commands like
	// screenshot, rather than a possible subsequent remote
	// or mirror render
//	primaryWorld = NULL;

	// set the time for shader effects in 2D rendering
	frameShaderTime = Sys_Milliseconds() * 0.001;

	// RB: TODO RC_SET_BUFFER is not handled in OpenGL
	setBufferCommand_t* cmd2 = ( setBufferCommand_t* )R_GetCommandBuffer( sizeof( *cmd2 ) );
	cmd2->commandId = RC_SET_BUFFER;
	cmd2->buffer = 0;

	// the old command buffer can now be rendered, while the new one can
	// be built in parallel
	return commandBufferHead;
}

/*
=====================
idRenderSystemLocal::WriteDemoPics
=====================
*/
void idRenderSystemLocal::WriteDemoPics()
{
	common->WriteDemo()->WriteInt( DS_RENDER );
	common->WriteDemo()->WriteInt( DC_GUI_MODEL );
}

/*
=====================
idRenderSystemLocal::WriteEndFrame
=====================
*/
void idRenderSystemLocal::WriteEndFrame()
{
	common->WriteDemo()->WriteInt( DS_RENDER );
	common->WriteDemo()->WriteInt( DC_END_FRAME );
}

/*
=====================
idRenderSystemLocal::DrawDemoPics
=====================
*/
void idRenderSystemLocal::DrawDemoPics()
{
}

/*
=====================
idRenderSystemLocal::GetCroppedViewport

Returns the current cropped pixel coordinates
=====================
*/
void idRenderSystemLocal::GetCroppedViewport( idScreenRect* viewport )
{
	*viewport = renderCrops[currentRenderCrop];
}

/*
========================
idRenderSystemLocal::PerformResolutionScaling

The 3D rendering size can be smaller than the full window resolution to reduce
fill rate requirements while still allowing the GUIs to be full resolution.
In split screen mode the rendering size is also smaller.
========================
*/
void idRenderSystemLocal::PerformResolutionScaling( int& newWidth, int& newHeight )
{
	float xScale = 1.0f;
	float yScale = 1.0f;
	resolutionScale.GetCurrentResolutionScale( xScale, yScale );

	newWidth = idMath::Ftoi( GetWidth() * xScale );
	newHeight = idMath::Ftoi( GetHeight() * yScale );
}

/*
================
idRenderSystemLocal::CropRenderSize
================
*/
void idRenderSystemLocal::CropRenderSize( int width, int height )
{
	if( !IsInitialized() )
	{
		return;
	}

	// close any gui drawing before changing the size
	guiModel->EmitFullScreen();
	guiModel->Clear();


	if( width < 1 || height < 1 )
	{
		common->Error( "CropRenderSize: bad sizes" );
	}

	if( common->WriteDemo() )
	{
		common->WriteDemo()->WriteInt( DS_RENDER );
		common->WriteDemo()->WriteInt( DC_CROP_RENDER );
		common->WriteDemo()->WriteInt( width );
		common->WriteDemo()->WriteInt( height );

		if( r_showDemo.GetBool() )
		{
			common->Printf( "write DC_CROP_RENDER\n" );
		}
	}

	idScreenRect& previous = renderCrops[currentRenderCrop];

	currentRenderCrop++;

	idScreenRect& current = renderCrops[currentRenderCrop];

	current.x1 = previous.x1;
	current.x2 = previous.x1 + width - 1;
	current.y1 = previous.y2 - height + 1;
	current.y2 = previous.y2;
}

/*
================
idRenderSystemLocal::CropRenderSize
================
*/
void idRenderSystemLocal::CropRenderSize( int x, int y, int width, int height, bool topLeftAncor )
{
	if( !IsInitialized() )
	{
		return;
	}

	// close any gui drawing before changing the size
	guiModel->EmitFullScreen();
	guiModel->Clear();


	if( width < 1 || height < 1 )
	{
		common->Error( "CropRenderSize: bad sizes" );
	}

	if( common->WriteDemo() )
	{
		common->WriteDemo()->WriteInt( DS_RENDER );
		common->WriteDemo()->WriteInt( DC_CROP_RENDER );
		common->WriteDemo()->WriteInt( width );
		common->WriteDemo()->WriteInt( height );

		if( r_showDemo.GetBool() )
		{
			common->Printf( "write DC_CROP_RENDER\n" );
		}
	}

	idScreenRect& previous = renderCrops[currentRenderCrop];

	currentRenderCrop++;

	idScreenRect& current = renderCrops[currentRenderCrop];

	if( topLeftAncor )
	{
		current.x1 = x;
		current.x2 = width - 1;
		current.y1 = y;
		current.y2 = height - 1;
	}
	else
	{
		current.x1 = x;
		current.x2 = previous.x1 + width - 1;
		current.y1 = y;
		current.y2 = previous.y2;
	}
}

/*
================
idRenderSystemLocal::UnCrop
================
*/
void idRenderSystemLocal::UnCrop()
{
	if( !IsInitialized() )
	{
		return;
	}

	if( currentRenderCrop < 1 )
	{
		common->Error( "idRenderSystemLocal::UnCrop: currentRenderCrop < 1" );
	}

	// close any gui drawing
	guiModel->EmitFullScreen();
	guiModel->Clear();

	currentRenderCrop--;

	if( common->WriteDemo() )
	{
		common->WriteDemo()->WriteInt( DS_RENDER );
		common->WriteDemo()->WriteInt( DC_UNCROP_RENDER );

		if( r_showDemo.GetBool() )
		{
			common->Printf( "write DC_UNCROP\n" );
		}
	}
}

/*
================
idRenderSystemLocal::CaptureRenderToImage
================
*/
void idRenderSystemLocal::CaptureRenderToImage( const char* imageName, bool clearColorAfterCopy )
{
	if( !IsInitialized() )
	{
		return;
	}
	guiModel->EmitFullScreen();
	guiModel->Clear();

	if( common->WriteDemo() )
	{
		common->WriteDemo()->WriteInt( DS_RENDER );
		common->WriteDemo()->WriteInt( DC_CAPTURE_RENDER );
		common->WriteDemo()->WriteHashString( imageName );

		if( r_showDemo.GetBool() )
		{
			common->Printf( "write DC_CAPTURE_RENDER: %s\n", imageName );
		}
	}
	idImage* image = globalImages->GetImage( imageName );
	if( image == NULL )
	{
		image = globalImages->AllocImage( imageName );
	}

	idScreenRect& rc = renderCrops[currentRenderCrop];

	copyRenderCommand_t* cmd = ( copyRenderCommand_t* )R_GetCommandBuffer( sizeof( *cmd ) );
	cmd->commandId = RC_COPY_RENDER;
	cmd->x = rc.x1;
	cmd->y = rc.y1;
	cmd->imageWidth = rc.GetWidth();
	cmd->imageHeight = rc.GetHeight();
	cmd->image = image;
	cmd->clearColorAfterCopy = clearColorAfterCopy;

	guiModel->Clear();
}

/*
==============
idRenderSystemLocal::CaptureRenderToFile
==============
*/
void idRenderSystemLocal::CaptureRenderToFile( const char* fileName, bool fixAlpha )
{
	if( !IsInitialized() )
	{
		return;
	}

	idScreenRect& rc = renderCrops[currentRenderCrop];

	guiModel->EmitFullScreen();
	guiModel->Clear();

	RenderCommandBuffers( frameData->cmdHead );

	// TODO implement for NVRHI

#if !defined( USE_VULKAN ) && !defined( USE_NVRHI )
	glReadBuffer( GL_BACK );

	// include extra space for OpenGL padding to word boundaries
	int	c = ( rc.GetWidth() + 3 ) * rc.GetHeight();
	byte* data = ( byte* )R_StaticAlloc( c * 3 );

	glReadPixels( rc.x1, rc.y1, rc.GetWidth(), rc.GetHeight(), GL_RGB, GL_UNSIGNED_BYTE, data );

	byte* data2 = ( byte* )R_StaticAlloc( c * 4 );

	for( int i = 0 ; i < c ; i++ )
	{
		data2[ i * 4 ] = data[ i * 3 ];
		data2[ i * 4 + 1 ] = data[ i * 3 + 1 ];
		data2[ i * 4 + 2 ] = data[ i * 3 + 2 ];
		data2[ i * 4 + 3 ] = 0xff;
	}

	R_WriteTGA( fileName, data2, rc.GetWidth(), rc.GetHeight(), true );

	R_StaticFree( data );
	R_StaticFree( data2 );
#endif
}


/*
==============
idRenderSystemLocal::AllocRenderWorld
==============
*/
idRenderWorld* idRenderSystemLocal::AllocRenderWorld()
{
	idRenderWorldLocal* rw;
	rw = new( TAG_RENDER ) idRenderWorldLocal;
	worlds.Append( rw );
	return rw;
}

/*
==============
idRenderSystemLocal::FreeRenderWorld
==============
*/
void idRenderSystemLocal::FreeRenderWorld( idRenderWorld* rw )
{
	if( primaryWorld == rw )
	{
		primaryWorld = NULL;
	}
	worlds.Remove( static_cast<idRenderWorldLocal*>( rw ) );
	delete rw;
}

/*
==============
idRenderSystemLocal::PrintMemInfo
==============
*/
void idRenderSystemLocal::PrintMemInfo( MemInfo_t* mi )
{
	// sum up image totals
	globalImages->PrintMemInfo( mi );

	// sum up model totals
	renderModelManager->PrintMemInfo( mi );

	// compute render totals

}

/*
===============
idRenderSystemLocal::UploadImage
===============
*/
bool idRenderSystemLocal::UploadImage( const char* imageName, const byte* data, int width, int height )
{
	idImage* image = globalImages->GetImage( imageName );
	if( !image )
	{
		return false;
	}

	commandList->open();

	image->UploadScratch( data, width, height, commandList );

	commandList->close();
	deviceManager->GetDevice()->executeCommandList( commandList );

	return true;
}
