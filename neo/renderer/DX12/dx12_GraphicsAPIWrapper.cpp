#pragma hdrstop
#include "../../idlib/precompiled.h"

#include "../tr_local.h"

extern DX12Renderer* dxRenderer;

void GL_SelectTexture(int uint) {
	// TODO: Setup texture select.
}

void GL_Cull(int cullType) {
	// TODO: Flip the culling as needed.
}

void GL_Scissor(int x /* left*/, int y /* bottom */, int w, int h) {
	dxRenderer->UpdateScissorRect(x, h - y, x + w, y);
}

void GL_Viewport(int x /* left */, int y /* bottom */, int w, int h) {
	dxRenderer->UpdateViewport(x, h - y, x + w, y);
}

void GL_PolygonOffset(float scale, float bias) {
	// TODO: Polygon offset.
}

void GL_DepthBoundsTest(const float zmin, const float zmax) {
	// TODO: Set depth bounds.
}

void GL_StartDepthPass(const idScreenRect& rect) {
	// TODO
}

void GL_FinishDepthPass() {
	// TODO
}

void GL_GetDepthPassRect(idScreenRect& rect) {
	rect.Clear();
}

void GL_Color(float r, float g, float b) {
	GL_Color(r, g, b, 1.0f);
}

void GL_Color(float r, float g, float b, float a) {
	float parm[4];
	parm[0] = idMath::ClampFloat(0.0f, 1.0f, r);
	parm[1] = idMath::ClampFloat(0.0f, 1.0f, g);
	parm[2] = idMath::ClampFloat(0.0f, 1.0f, b);
	parm[3] = idMath::ClampFloat(0.0f, 1.0f, a);
	renderProgManager.SetRenderParm(RENDERPARM_COLOR, parm);
}

void GL_Clear(bool color, bool depth, bool stencil, byte stencilValue, float r, float g, float b, float a) {
	// TODO: set the renderer to run the clear commands for the different buffers.
}

void GL_SetDefaultState() {
	RENDERLOG_PRINTF("--- GL_SetDefaultState ---\n");

	// TODO: Set the default graphics state.
	RENDERLOG_PRINTF("TODO: Set the default graphics state.\n");
}

void GL_State(uint64 stateBits, bool forceGlState) {
	// TODO: Check if needed and set state.

	backEnd.glState.glStateBits = stateBits;
}

uint64 GL_GetCurrentState() {
	return backEnd.glState.glStateBits;
}

uint64 GL_GetCurrentStateMinusStencil() {
	return GL_GetCurrentState() & ~(GLS_STENCIL_OP_BITS | GLS_STENCIL_FUNC_BITS | GLS_STENCIL_FUNC_REF_BITS | GLS_STENCIL_FUNC_MASK_BITS);
}
