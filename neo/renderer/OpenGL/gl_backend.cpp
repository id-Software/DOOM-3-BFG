/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.

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

#pragma hdrstop
#include "precompiled.h"

#include "../tr_local.h"
#include "../../framework/Common_local.h"

idCVar r_drawFlickerBox( "r_drawFlickerBox", "0", CVAR_RENDERER | CVAR_BOOL, "visual test for dropping frames" );
idCVar stereoRender_warp( "stereoRender_warp", "0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_BOOL, "use the optical warping renderprog instead of stereoDeGhost" );
idCVar stereoRender_warpStrength( "stereoRender_warpStrength", "1.45", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_FLOAT, "amount of pre-distortion" );

idCVar stereoRender_warpCenterX( "stereoRender_warpCenterX", "0.5", CVAR_RENDERER | CVAR_FLOAT | CVAR_ARCHIVE, "center for left eye, right eye will be 1.0 - this" );
idCVar stereoRender_warpCenterY( "stereoRender_warpCenterY", "0.5", CVAR_RENDERER | CVAR_FLOAT | CVAR_ARCHIVE, "center for both eyes" );
idCVar stereoRender_warpParmZ( "stereoRender_warpParmZ", "0", CVAR_RENDERER | CVAR_FLOAT | CVAR_ARCHIVE, "development parm" );
idCVar stereoRender_warpParmW( "stereoRender_warpParmW", "0", CVAR_RENDERER | CVAR_FLOAT | CVAR_ARCHIVE, "development parm" );
idCVar stereoRender_warpTargetFraction( "stereoRender_warpTargetFraction", "1.0", CVAR_RENDERER | CVAR_FLOAT | CVAR_ARCHIVE, "fraction of half-width the through-lens view covers" );

idCVar r_showSwapBuffers( "r_showSwapBuffers", "0", CVAR_BOOL, "Show timings from GL_BlockingSwapBuffers" );
idCVar r_syncEveryFrame( "r_syncEveryFrame", "1", CVAR_BOOL, "Don't let the GPU buffer execution past swapbuffers" );

static int		swapIndex;		// 0 or 1 into renderSync
static GLsync	renderSync[2];

void GLimp_SwapBuffers();
void RB_SetMVP( const idRenderMatrix& mvp );

/*
============================================================================

RENDER BACK END THREAD FUNCTIONS

============================================================================
*/

/*
=============
RB_DrawFlickerBox
=============
*/
static void RB_DrawFlickerBox()
{
	if( !r_drawFlickerBox.GetBool() )
	{
		return;
	}
	if( tr.frameCount & 1 )
	{
		glClearColor( 1, 0, 0, 1 );
	}
	else
	{
		glClearColor( 0, 1, 0, 1 );
	}
	glScissor( 0, 0, 256, 256 );
	glClear( GL_COLOR_BUFFER_BIT );
}

/*
=============
RB_SetBuffer
=============
*/
static void	RB_SetBuffer( const void* data )
{
	// see which draw buffer we want to render the frame to
	
	const setBufferCommand_t* cmd = ( const setBufferCommand_t* )data;
	
	RENDERLOG_PRINTF( "---------- RB_SetBuffer ---------- to buffer # %d\n", cmd->buffer );
	
	GL_Scissor( 0, 0, tr.GetWidth(), tr.GetHeight() );
	
	// clear screen for debugging
	// automatically enable this with several other debug tools
	// that might leave unrendered portions of the screen
	if( r_clear.GetFloat() || idStr::Length( r_clear.GetString() ) != 1 || r_singleArea.GetBool() || r_showOverDraw.GetBool() )
	{
		float c[3];
		if( sscanf( r_clear.GetString(), "%f %f %f", &c[0], &c[1], &c[2] ) == 3 )
		{
			GL_Clear( true, false, false, 0, c[0], c[1], c[2], 1.0f );
		}
		else if( r_clear.GetInteger() == 2 )
		{
			GL_Clear( true, false, false, 0, 0.0f, 0.0f,  0.0f, 1.0f );
		}
		else if( r_showOverDraw.GetBool() )
		{
			GL_Clear( true, false, false, 0, 1.0f, 1.0f, 1.0f, 1.0f );
		}
		else
		{
			GL_Clear( true, false, false, 0, 0.4f, 0.0f, 0.25f, 1.0f );
		}
	}
}

/*
=============
GL_BlockingSwapBuffers

We want to exit this with the GPU idle, right at vsync
=============
*/
const void GL_BlockingSwapBuffers()
{
	RENDERLOG_PRINTF( "***************** GL_BlockingSwapBuffers *****************\n\n\n" );
	
	const int beforeFinish = Sys_Milliseconds();
	
	if( !glConfig.syncAvailable )
	{
		glFinish();
	}
	
	const int beforeSwap = Sys_Milliseconds();
	if( r_showSwapBuffers.GetBool() && beforeSwap - beforeFinish > 1 )
	{
		common->Printf( "%i msec to glFinish\n", beforeSwap - beforeFinish );
	}
	
	GLimp_SwapBuffers();
	
	const int beforeFence = Sys_Milliseconds();
	if( r_showSwapBuffers.GetBool() && beforeFence - beforeSwap > 1 )
	{
		common->Printf( "%i msec to swapBuffers\n", beforeFence - beforeSwap );
	}
	
	if( glConfig.syncAvailable )
	{
		swapIndex ^= 1;
		
		if( glIsSync( renderSync[swapIndex] ) )
		{
			glDeleteSync( renderSync[swapIndex] );
		}
		// draw something tiny to ensure the sync is after the swap
		const int start = Sys_Milliseconds();
		glScissor( 0, 0, 1, 1 );
		glEnable( GL_SCISSOR_TEST );
		glClear( GL_COLOR_BUFFER_BIT );
		renderSync[swapIndex] = glFenceSync( GL_SYNC_GPU_COMMANDS_COMPLETE, 0 );
		const int end = Sys_Milliseconds();
		if( r_showSwapBuffers.GetBool() && end - start > 1 )
		{
			common->Printf( "%i msec to start fence\n", end - start );
		}
		
		GLsync	syncToWaitOn;
		if( r_syncEveryFrame.GetBool() )
		{
			syncToWaitOn = renderSync[swapIndex];
		}
		else
		{
			syncToWaitOn = renderSync[!swapIndex];
		}
		
		if( glIsSync( syncToWaitOn ) )
		{
			for( GLenum r = GL_TIMEOUT_EXPIRED; r == GL_TIMEOUT_EXPIRED; )
			{
				r = glClientWaitSync( syncToWaitOn, GL_SYNC_FLUSH_COMMANDS_BIT, 1000 * 1000 );
			}
		}
	}
	
	const int afterFence = Sys_Milliseconds();
	if( r_showSwapBuffers.GetBool() && afterFence - beforeFence > 1 )
	{
		common->Printf( "%i msec to wait on fence\n", afterFence - beforeFence );
	}
	
	const int64 exitBlockTime = Sys_Microseconds();
	
	static int64 prevBlockTime;
	if( r_showSwapBuffers.GetBool() && prevBlockTime )
	{
		const int delta = ( int )( exitBlockTime - prevBlockTime );
		common->Printf( "blockToBlock: %i\n", delta );
	}
	prevBlockTime = exitBlockTime;
}

/*
====================
R_MakeStereoRenderImage
====================
*/
static void R_MakeStereoRenderImage( idImage* image )
{
	idImageOpts	opts;
	opts.width = renderSystem->GetWidth();
	opts.height = renderSystem->GetHeight();
	opts.numLevels = 1;
	opts.format = FMT_RGBA8;
	image->AllocImage( opts, TF_LINEAR, TR_CLAMP );
}

/*
====================
RB_StereoRenderExecuteBackEndCommands

Renders the draw list twice, with slight modifications for left eye / right eye
====================
*/
void RB_StereoRenderExecuteBackEndCommands( const emptyCommand_t* const allCmds )
{
	uint64 backEndStartTime = Sys_Microseconds();
	
	// If we are in a monoscopic context, this draws to the only buffer, and is
	// the same as GL_BACK.  In a quad-buffer stereo context, this is necessary
	// to prevent GL from forcing the rendering to go to both BACK_LEFT and
	// BACK_RIGHT at a performance penalty.
	// To allow stereo deghost processing, the views have to be copied to separate
	// textures anyway, so there isn't any benefit to rendering to BACK_RIGHT for
	// that eye.
	glDrawBuffer( GL_BACK_LEFT );
	
	// create the stereoRenderImage if we haven't already
	static idImage* stereoRenderImages[2];
	for( int i = 0; i < 2; i++ )
	{
		if( stereoRenderImages[i] == NULL )
		{
			stereoRenderImages[i] = globalImages->ImageFromFunction( va( "_stereoRender%i", i ), R_MakeStereoRenderImage );
		}
		
		// resize the stereo render image if the main window has changed size
		if( stereoRenderImages[i]->GetUploadWidth() != renderSystem->GetWidth() ||
				stereoRenderImages[i]->GetUploadHeight() != renderSystem->GetHeight() )
		{
			stereoRenderImages[i]->Resize( renderSystem->GetWidth(), renderSystem->GetHeight() );
		}
	}
	
	// In stereoRender mode, the front end has generated two RC_DRAW_VIEW commands
	// with slightly different origins for each eye.
	
	// TODO: only do the copy after the final view has been rendered, not mirror subviews?
	
	// Render the 3D draw views from the screen origin so all the screen relative
	// texture mapping works properly, then copy the portion we are going to use
	// off to a texture.
	bool foundEye[2] = { false, false };
	
	for( int stereoEye = 1; stereoEye >= -1; stereoEye -= 2 )
	{
		// set up the target texture we will draw to
		const int targetEye = ( stereoEye == 1 ) ? 1 : 0;
		
		// Set the back end into a known default state to fix any stale render state issues
		GL_SetDefaultState();
		renderProgManager.Unbind();
		renderProgManager.ZeroUniforms();
		
		for( const emptyCommand_t* cmds = allCmds; cmds != NULL; cmds = ( const emptyCommand_t* )cmds->next )
		{
			switch( cmds->commandId )
			{
				case RC_NOP:
					break;
				case RC_DRAW_VIEW_GUI:
				case RC_DRAW_VIEW_3D:
				{
					const drawSurfsCommand_t* const dsc = ( const drawSurfsCommand_t* )cmds;
					const viewDef_t&			eyeViewDef = *dsc->viewDef;
					
					if( eyeViewDef.renderView.viewEyeBuffer && eyeViewDef.renderView.viewEyeBuffer != stereoEye )
					{
						// this is the render view for the other eye
						continue;
					}
					
					foundEye[ targetEye ] = true;
					RB_DrawView( dsc, stereoEye );
					if( cmds->commandId == RC_DRAW_VIEW_GUI )
					{
					}
				}
				break;
				case RC_SET_BUFFER:
					RB_SetBuffer( cmds );
					break;
				case RC_COPY_RENDER:
					RB_CopyRender( cmds );
					break;
				case RC_POST_PROCESS:
				{
					postProcessCommand_t* cmd = ( postProcessCommand_t* )cmds;
					if( cmd->viewDef->renderView.viewEyeBuffer != stereoEye )
					{
						break;
					}
					RB_PostProcess( cmds );
				}
				break;
				default:
					common->Error( "RB_ExecuteBackEndCommands: bad commandId" );
					break;
			}
		}
		
		// copy to the target
		stereoRenderImages[ targetEye ]->CopyFramebuffer( 0, 0, renderSystem->GetWidth(), renderSystem->GetHeight() );
	}
	
	// perform the final compositing / warping / deghosting to the actual framebuffer(s)
	assert( foundEye[0] && foundEye[1] );
	
	GL_SetDefaultState();
	
	RB_SetMVP( renderMatrix_identity );
	
	// If we are in quad-buffer pixel format but testing another 3D mode,
	// make sure we draw to both eyes.  This is likely to be sub-optimal
	// performance on most cards and drivers, but it is better than getting
	// a confusing, half-ghosted view.
	if( renderSystem->GetStereo3DMode() != STEREO3D_QUAD_BUFFER )
	{
		glDrawBuffer( GL_BACK );
	}
	
	GL_State( GLS_DEPTHFUNC_ALWAYS );
	GL_Cull( CT_TWO_SIDED );
	
	// We just want to do a quad pass - so make sure we disable any texgen and
	// set the texture matrix to the identity so we don't get anomalies from
	// any stale uniform data being present from a previous draw call
	const float texS[4] = { 1.0f, 0.0f, 0.0f, 0.0f };
	const float texT[4] = { 0.0f, 1.0f, 0.0f, 0.0f };
	renderProgManager.SetRenderParm( RENDERPARM_TEXTUREMATRIX_S, texS );
	renderProgManager.SetRenderParm( RENDERPARM_TEXTUREMATRIX_T, texT );
	
	// disable any texgen
	const float texGenEnabled[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	renderProgManager.SetRenderParm( RENDERPARM_TEXGEN_0_ENABLED, texGenEnabled );
	
	renderProgManager.BindShader_Texture();
	GL_Color( 1, 1, 1, 1 );
	
	switch( renderSystem->GetStereo3DMode() )
	{
		case STEREO3D_QUAD_BUFFER:
			glDrawBuffer( GL_BACK_RIGHT );
			GL_SelectTexture( 0 );
			stereoRenderImages[1]->Bind();
			GL_SelectTexture( 1 );
			stereoRenderImages[0]->Bind();
			RB_DrawElementsWithCounters( &backEnd.unitSquareSurface );
			
			glDrawBuffer( GL_BACK_LEFT );
			GL_SelectTexture( 1 );
			stereoRenderImages[1]->Bind();
			GL_SelectTexture( 0 );
			stereoRenderImages[0]->Bind();
			RB_DrawElementsWithCounters( &backEnd.unitSquareSurface );
			
			break;
		case STEREO3D_HDMI_720:
			// HDMI 720P 3D
			GL_SelectTexture( 0 );
			stereoRenderImages[1]->Bind();
			GL_SelectTexture( 1 );
			stereoRenderImages[0]->Bind();
			GL_ViewportAndScissor( 0, 0, 1280, 720 );
			RB_DrawElementsWithCounters( &backEnd.unitSquareSurface );
			
			GL_SelectTexture( 0 );
			stereoRenderImages[0]->Bind();
			GL_SelectTexture( 1 );
			stereoRenderImages[1]->Bind();
			GL_ViewportAndScissor( 0, 750, 1280, 720 );
			RB_DrawElementsWithCounters( &backEnd.unitSquareSurface );
			
			// force the HDMI 720P 3D guard band to a constant color
			glScissor( 0, 720, 1280, 30 );
			glClear( GL_COLOR_BUFFER_BIT );
			break;
		default:
		case STEREO3D_SIDE_BY_SIDE:
			if( stereoRender_warp.GetBool() )
			{
				// this is the Rift warp
				// renderSystem->GetWidth() / GetHeight() have returned equal values (640 for initial Rift)
				// and we are going to warp them onto a symetric square region of each half of the screen
				
				renderProgManager.BindShader_StereoWarp();
				
				// clear the entire screen to black
				// we could be smart and only clear the areas we aren't going to draw on, but
				// clears are fast...
				glScissor( 0, 0, glConfig.nativeScreenWidth, glConfig.nativeScreenHeight );
				glClearColor( 0, 0, 0, 0 );
				glClear( GL_COLOR_BUFFER_BIT );
				
				// the size of the box that will get the warped pixels
				// With the 7" displays, this will be less than half the screen width
				const int pixelDimensions = ( glConfig.nativeScreenWidth >> 1 ) * stereoRender_warpTargetFraction.GetFloat();
				
				// Always scissor to the half-screen boundary, but the viewports
				// might cross that boundary if the lenses can be adjusted closer
				// together.
				glViewport( ( glConfig.nativeScreenWidth >> 1 ) - pixelDimensions,
							( glConfig.nativeScreenHeight >> 1 ) - ( pixelDimensions >> 1 ),
							pixelDimensions, pixelDimensions );
				glScissor( 0, 0, glConfig.nativeScreenWidth >> 1, glConfig.nativeScreenHeight );
				
				idVec4	color( stereoRender_warpCenterX.GetFloat(), stereoRender_warpCenterY.GetFloat(), stereoRender_warpParmZ.GetFloat(), stereoRender_warpParmW.GetFloat() );
				// don't use GL_Color(), because we don't want to clamp
				renderProgManager.SetRenderParm( RENDERPARM_COLOR, color.ToFloatPtr() );
				
				GL_SelectTexture( 0 );
				stereoRenderImages[0]->Bind();
				glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER );
				glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER );
				RB_DrawElementsWithCounters( &backEnd.unitSquareSurface );
				
				idVec4	color2( stereoRender_warpCenterX.GetFloat(), stereoRender_warpCenterY.GetFloat(), stereoRender_warpParmZ.GetFloat(), stereoRender_warpParmW.GetFloat() );
				// don't use GL_Color(), because we don't want to clamp
				renderProgManager.SetRenderParm( RENDERPARM_COLOR, color2.ToFloatPtr() );
				
				glViewport( ( glConfig.nativeScreenWidth >> 1 ),
							( glConfig.nativeScreenHeight >> 1 ) - ( pixelDimensions >> 1 ),
							pixelDimensions, pixelDimensions );
				glScissor( glConfig.nativeScreenWidth >> 1, 0, glConfig.nativeScreenWidth >> 1, glConfig.nativeScreenHeight );
				
				GL_SelectTexture( 0 );
				stereoRenderImages[1]->Bind();
				glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER );
				glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER );
				RB_DrawElementsWithCounters( &backEnd.unitSquareSurface );
				break;
			}
		// a non-warped side-by-side-uncompressed (dual input cable) is rendered
		// just like STEREO3D_SIDE_BY_SIDE_COMPRESSED, so fall through.
		case STEREO3D_SIDE_BY_SIDE_COMPRESSED:
			GL_SelectTexture( 0 );
			stereoRenderImages[0]->Bind();
			GL_SelectTexture( 1 );
			stereoRenderImages[1]->Bind();
			GL_ViewportAndScissor( 0, 0, renderSystem->GetWidth(), renderSystem->GetHeight() );
			RB_DrawElementsWithCounters( &backEnd.unitSquareSurface );
			
			GL_SelectTexture( 0 );
			stereoRenderImages[1]->Bind();
			GL_SelectTexture( 1 );
			stereoRenderImages[0]->Bind();
			GL_ViewportAndScissor( renderSystem->GetWidth(), 0, renderSystem->GetWidth(), renderSystem->GetHeight() );
			RB_DrawElementsWithCounters( &backEnd.unitSquareSurface );
			break;
			
		case STEREO3D_TOP_AND_BOTTOM_COMPRESSED:
			GL_SelectTexture( 1 );
			stereoRenderImages[0]->Bind();
			GL_SelectTexture( 0 );
			stereoRenderImages[1]->Bind();
			GL_ViewportAndScissor( 0, 0, renderSystem->GetWidth(), renderSystem->GetHeight() );
			RB_DrawElementsWithCounters( &backEnd.unitSquareSurface );
			
			GL_SelectTexture( 1 );
			stereoRenderImages[1]->Bind();
			GL_SelectTexture( 0 );
			stereoRenderImages[0]->Bind();
			GL_ViewportAndScissor( 0, renderSystem->GetHeight(), renderSystem->GetWidth(), renderSystem->GetHeight() );
			RB_DrawElementsWithCounters( &backEnd.unitSquareSurface );
			break;
			
		case STEREO3D_INTERLACED:
			// every other scanline
			GL_SelectTexture( 0 );
			stereoRenderImages[0]->Bind();
			glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
			glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
			
			GL_SelectTexture( 1 );
			stereoRenderImages[1]->Bind();
			glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
			glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
			
			GL_ViewportAndScissor( 0, 0, renderSystem->GetWidth(), renderSystem->GetHeight() * 2 );
			renderProgManager.BindShader_StereoInterlace();
			RB_DrawElementsWithCounters( &backEnd.unitSquareSurface );
			
			GL_SelectTexture( 0 );
			glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
			glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
			
			GL_SelectTexture( 1 );
			glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
			glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
			
			break;
	}
	
	// debug tool
	RB_DrawFlickerBox();
	
	// make sure the drawing is actually started
	glFlush();
	
	// we may choose to sync to the swapbuffers before the next frame
	
	// stop rendering on this thread
	uint64 backEndFinishTime = Sys_Microseconds();
	backEnd.pc.totalMicroSec = backEndFinishTime - backEndStartTime;
}

/*
====================
RB_ExecuteBackEndCommands

This function will be called syncronously if running without
smp extensions, or asyncronously by another thread.
====================
*/
void RB_ExecuteBackEndCommands( const emptyCommand_t* cmds )
{
	// r_debugRenderToTexture
	int c_draw3d = 0;
	int c_draw2d = 0;
	int c_setBuffers = 0;
	int c_copyRenders = 0;
	
	resolutionScale.SetCurrentGPUFrameTime( commonLocal.GetRendererGPUMicroseconds() );
	
	renderLog.StartFrame();
	
	if( cmds->commandId == RC_NOP && !cmds->next )
	{
		return;
	}
	
	if( renderSystem->GetStereo3DMode() != STEREO3D_OFF )
	{
		RB_StereoRenderExecuteBackEndCommands( cmds );
		renderLog.EndFrame();
		return;
	}
	
	uint64 backEndStartTime = Sys_Microseconds();
	
	// needed for editor rendering
	GL_SetDefaultState();
	
	// If we have a stereo pixel format, this will draw to both
	// the back left and back right buffers, which will have a
	// performance penalty.
	glDrawBuffer( GL_BACK );
	
	for( ; cmds != NULL; cmds = ( const emptyCommand_t* )cmds->next )
	{
		switch( cmds->commandId )
		{
			case RC_NOP:
				break;
			case RC_DRAW_VIEW_3D:
			case RC_DRAW_VIEW_GUI:
				RB_DrawView( cmds, 0 );
				if( ( ( const drawSurfsCommand_t* )cmds )->viewDef->viewEntitys )
				{
					c_draw3d++;
				}
				else
				{
					c_draw2d++;
				}
				break;
			case RC_SET_BUFFER:
				c_setBuffers++;
				break;
			case RC_COPY_RENDER:
				RB_CopyRender( cmds );
				c_copyRenders++;
				break;
			case RC_POST_PROCESS:
				RB_PostProcess( cmds );
				break;
			default:
				common->Error( "RB_ExecuteBackEndCommands: bad commandId" );
				break;
		}
	}
	
	RB_DrawFlickerBox();
	
	// Fix for the steam overlay not showing up while in game without Shell/Debug/Console/Menu also rendering
	glColorMask( 1, 1, 1, 1 );
	
	glFlush();
	
	// stop rendering on this thread
	uint64 backEndFinishTime = Sys_Microseconds();
	backEnd.pc.totalMicroSec = backEndFinishTime - backEndStartTime;
	
	if( r_debugRenderToTexture.GetInteger() == 1 )
	{
		common->Printf( "3d: %i, 2d: %i, SetBuf: %i, CpyRenders: %i, CpyFrameBuf: %i\n", c_draw3d, c_draw2d, c_setBuffers, c_copyRenders, backEnd.pc.c_copyFrameBuffer );
		backEnd.pc.c_copyFrameBuffer = 0;
	}
	renderLog.EndFrame();
}
