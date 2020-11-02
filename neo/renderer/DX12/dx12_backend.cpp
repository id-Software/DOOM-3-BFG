#pragma hdrstop
#include "../../idlib/precompiled.h"

#include "../tr_local.h"
#include "../../framework/Common_local.h"

backEndState_t	backEnd;

void RB_DrawElementsWithCounters(const drawSurf_t* surf) {

}

void RB_DrawViewInternal(const viewDef_t* viewDef, const int stereoEye) {

}

void RB_DrawView(const void* data, const int stereoEy) {
	// TODO: Draw view
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

	// TODO: switch backbuffer mode to "BACK"

	for (; cmds != NULL; cmds = (const emptyCommand_t*)cmds->next) {
		switch (cmds->commandId) {
		case RC_NOP:
			break;
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

