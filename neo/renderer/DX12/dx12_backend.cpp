#pragma hdrstop
#include "../../idlib/precompiled.h"

#include "../tr_local.h"
#include "../../framework/Common_local.h"

backEndState_t	backEnd;

idCVar r_drawEyeColor("r_drawEyeColor", "0", CVAR_RENDERER | CVAR_BOOL, "Draw a colored box, red = left eye, blue = right eye, grey = non-stereo");
idCVar r_motionBlur("r_motionBlur", "0", CVAR_RENDERER | CVAR_INTEGER | CVAR_ARCHIVE, "1 - 5, log2 of the number of motion blur samples");
idCVar r_forceZPassStencilShadows("r_forceZPassStencilShadows", "0", CVAR_RENDERER | CVAR_BOOL, "force Z-pass rendering for performance testing");
idCVar r_useStencilShadowPreload("r_useStencilShadowPreload", "1", CVAR_RENDERER | CVAR_BOOL, "use stencil shadow preload algorithm instead of Z-fail");
idCVar r_skipShaderPasses("r_skipShaderPasses", "0", CVAR_RENDERER | CVAR_BOOL, "");
idCVar r_skipInteractionFastPath("r_skipInteractionFastPath", "1", CVAR_RENDERER | CVAR_BOOL, "");
idCVar r_useLightStencilSelect("r_useLightStencilSelect", "0", CVAR_RENDERER | CVAR_BOOL, "use stencil select pass");

bool R_GetModeListForDisplay(const int displayNum, idList<vidMode_t>& modeList) {
	// TODO: Implement
	for (int i = 0; i <= displayNum; ++i) {
		vidMode_t mode;
		mode.displayHz = 60;
		mode.height = 600;
		mode.width = 800;

		modeList.Append(mode);
	}

	return true;
}

/*
=============
GL_BlockingSwapBuffers

We want to exit this with the GPU idle, right at vsync
=============
*/
const void GL_BlockingSwapBuffers() {
	RENDERLOG_PRINTF("***************** GL_BlockingSwapBuffers *****************\n\n\n");

	const int beforeFinish = Sys_Milliseconds();

	// TODO: Implement

	/*if (!glConfig.syncAvailable) {
		glFinish();
	}

	const int beforeSwap = Sys_Milliseconds();
	if (r_showSwapBuffers.GetBool() && beforeSwap - beforeFinish > 1) {
		common->Printf("%i msec to glFinish\n", beforeSwap - beforeFinish);
	}*/

	// TODO: Check if this will be an issue.
	//GLimp_SwapBuffers();

	const int beforeFence = Sys_Milliseconds();
	/*if (r_showSwapBuffers.GetBool() && beforeFence - beforeSwap > 1) {
		common->Printf("%i msec to swapBuffers\n", beforeFence - beforeSwap);
	}*/

	//if (glConfig.syncAvailable) {
	//	swapIndex ^= 1;

	//	if (qglIsSync(renderSync[swapIndex])) {
	//		qglDeleteSync(renderSync[swapIndex]);
	//	}
	//	// draw something tiny to ensure the sync is after the swap
	//	const int start = Sys_Milliseconds();
	//	qglScissor(0, 0, 1, 1);
	//	qglEnable(GL_SCISSOR_TEST);
	//	qglClear(GL_COLOR_BUFFER_BIT);
	//	renderSync[swapIndex] = qglFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	//	const int end = Sys_Milliseconds();
	//	if (r_showSwapBuffers.GetBool() && end - start > 1) {
	//		common->Printf("%i msec to start fence\n", end - start);
	//	}

	//	GLsync	syncToWaitOn;
	//	if (r_syncEveryFrame.GetBool()) {
	//		syncToWaitOn = renderSync[swapIndex];
	//	}
	//	else {
	//		syncToWaitOn = renderSync[!swapIndex];
	//	}

	//	if (qglIsSync(syncToWaitOn)) {
	//		for (GLenum r = GL_TIMEOUT_EXPIRED; r == GL_TIMEOUT_EXPIRED; ) {
	//			r = qglClientWaitSync(syncToWaitOn, GL_SYNC_FLUSH_COMMANDS_BIT, 1000 * 1000);
	//		}
	//	}
	//}

	const int afterFence = Sys_Milliseconds();
	/*if (r_showSwapBuffers.GetBool() && afterFence - beforeFence > 1) {
		common->Printf("%i msec to wait on fence\n", afterFence - beforeFence);
	}*/

	const int64 exitBlockTime = Sys_Microseconds();

	static int64 prevBlockTime;
	/*if (r_showSwapBuffers.GetBool() && prevBlockTime) {
		const int delta = (int)(exitBlockTime - prevBlockTime);
		common->Printf("blockToBlock: %i\n", delta);
	}*/
	prevBlockTime = exitBlockTime;
}

void RB_DrawElementsWithCounters(const drawSurf_t* surf) {

}

void RB_DrawViewInternal(const viewDef_t* viewDef, const int stereoEye) {
	renderLog.OpenBlock("RB_DrawViewInternal");

	//-------------------------------------------------
	// guis can wind up referencing purged images that need to be loaded.
	// this used to be in the gui emit code, but now that it can be running
	// in a separate thread, it must not try to load images, so do it here.
	//-------------------------------------------------
	drawSurf_t** drawSurfs = (drawSurf_t**)&viewDef->drawSurfs[0];
	const int numDrawSurfs = viewDef->numDrawSurfs;

	for (int i = 0; i < numDrawSurfs; i++) {
		const drawSurf_t* ds = viewDef->drawSurfs[i];
		if (ds->material != NULL) {
			const_cast<idMaterial*>(ds->material)->EnsureNotPurged();
		}
	}

	//-------------------------------------------------
	// RB_BeginDrawingView
	//
	// Any mirrored or portaled views have already been drawn, so prepare
	// to actually render the visible surfaces for this view
	//
	// clear the z buffer, set the projection matrix, etc
	//-------------------------------------------------

	// set the window clipping
	GL_Viewport(viewDef->viewport.x1,
		viewDef->viewport.y1,
		viewDef->viewport.x2 + 1 - viewDef->viewport.x1,
		viewDef->viewport.y2 + 1 - viewDef->viewport.y1);

	// the scissor may be smaller than the viewport for subviews
	GL_Scissor(backEnd.viewDef->viewport.x1 + viewDef->scissor.x1,
		backEnd.viewDef->viewport.y1 + viewDef->scissor.y1,
		viewDef->scissor.x2 + 1 - viewDef->scissor.x1,
		viewDef->scissor.y2 + 1 - viewDef->scissor.y1);
	backEnd.currentScissor = viewDef->scissor;

	backEnd.glState.faceCulling = -1;		// force face culling to set next time

	// ensures that depth writes are enabled for the depth clear
	GL_State(GLS_DEFAULT); //TODO: We need to properly implement this

	dxRenderer.BeginDraw();

	// Clear the depth buffer and clear the stencil to 128 for stencil shadows as well as gui masking
	GL_Clear(false, true, true, STENCIL_SHADOW_TEST_VALUE, 0.0f, 0.0f, 0.0f, 0.0f); //TODO: We need to properly implement this.

	// normal face culling
	GL_Cull(CT_FRONT_SIDED); //TODO: We need to implement this.

	//TODO: Implement the rest.

	dxRenderer.EndDraw();

	renderLog.CloseBlock();
}

void RB_DrawView(const void* data, const int stereoEye) {
	const drawSurfsCommand_t* cmd = (const drawSurfsCommand_t*)data;
	backEnd.viewDef = cmd->viewDef;

	backEnd.currentRenderCopied = false;

	// if there aren't any drawsurfs, do nothing
	if (!backEnd.viewDef->numDrawSurfs) {
		return;
	}

	// skip render bypasses everything that has models, assuming
	// them to be 3D views, but leaves 2D rendering visible
	if (r_skipRender.GetBool() && backEnd.viewDef->viewEntitys) {
		return;
	}

	if (r_skipRenderContext.GetBool() && backEnd.viewDef->viewEntitys) {
		// TODO: Implement skipping the render context.
	}

	backEnd.pc.c_surfaces += backEnd.viewDef->numDrawSurfs;

	//TODO: Implement
	// RB_ShowOverdraw();

	// render the scene
	RB_DrawViewInternal(cmd->viewDef, stereoEye);

	//TODO: Implemnt
	//RB_MotionBlur();

	// restore the context for 2D drawing if we were stubbing it out
	if (r_skipRenderContext.GetBool() && backEnd.viewDef->viewEntitys) {
		//GLimp_ActivateContext();
		GL_SetDefaultState();
	}
}

void RB_CopyRender(const void* data) {
	// TODO: Copy the render
}

void RB_PostProcess(const void* data) {
	// TODO: Perform post processing.
}

void RB_ExecuteBackEndCommands(const emptyCommand_t* cmds) {
	int c_draw3d = 0;
	int c_draw2d = 0;
	int c_setBuffers = 0;
	int c_copyRenders = 0;

	renderLog.StartFrame();

	if (cmds->commandId == RC_NOP && !cmds->next) {
		return;
	}

	// TODO: Check rendering in stereo mode.

	uint64 backEndStartTime = Sys_Microseconds();

	GL_SetDefaultState();

	// TODO: switch culling mode to "BACK"

	for (; cmds != NULL; cmds = (const emptyCommand_t*)cmds->next) {
		switch (cmds->commandId) {
		case RC_NOP:
			break;
		case RC_DRAW_VIEW_GUI:
		case RC_DRAW_VIEW_3D:
			RB_DrawView(cmds, 0);
			if (((const drawSurfsCommand_t*)cmds)->viewDef->viewEntitys) {
				c_draw3d++;
			}
			else {
				c_draw2d++;
			}
			break;
		case RC_SET_BUFFER:
			dxRenderer.PresentBackbuffer();
			c_setBuffers++;
			break;
		case RC_COPY_RENDER:
			RB_CopyRender(cmds);
			c_copyRenders++;
			break;
		case RC_POST_PROCESS:
			RB_PostProcess(cmds);
			break;
		default:
			common->Error("RB_ExecuteBackEndCommands: bad commandId");
			break;
		}
	}

	// TODO: reset the color mask

	// TODO: Flush the results.

	// stop rendering on this thread
	uint64 backEndFinishTime = Sys_Microseconds();
	backEnd.pc.totalMicroSec = backEndFinishTime - backEndStartTime;

	if (r_debugRenderToTexture.GetInteger() == 1) {
		common->Printf("3d: %i, 2d: %i, SetBuf: %i, CpyRenders: %i, CpyFrameBuf: %i\n", c_draw3d, c_draw2d, c_setBuffers, c_copyRenders, backEnd.pc.c_copyFrameBuffer);
		backEnd.pc.c_copyFrameBuffer = 0;
	}
	renderLog.EndFrame();
}
