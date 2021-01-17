#pragma hdrstop
#include "../../idlib/precompiled.h"

#include "../tr_local.h"

extern DX12Renderer dxRenderer;

void GL_SelectTexture(int uint) {
	dxRenderer.SetActiveTextureRegister(uint);
}

void GL_Cull(int cullType) {
	// Set the state. This will be hashed into a program key and used to load the program state.
	backEnd.glState.faceCulling = cullType;
}

void GL_Scissor(int x, int y, int w, int h) {
	dxRenderer.UpdateScissorRect(x, y, w, h);
}

void GL_Viewport(int x, int y, int w, int h) {
	dxRenderer.UpdateViewport(x, y, w, h);
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

void GL_Color(float* color) {
	if (color == NULL) {
		return;
	}
	GL_Color(color[0], color[1], color[2], color[3]);
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
	float colorRGBA[4] = { r, g, b, a };
	dxRenderer.Clear(color, depth, stencil, stencilValue, colorRGBA);
}

void GL_SetDefaultState() {
	RENDERLOG_PRINTF("--- GL_SetDefaultState ---\n");

	// make sure our GL state vector is set correctly
	memset(&backEnd.glState, 0, sizeof(backEnd.glState));
	GL_State(0, true);

	// These are changed by GL_Cull
	GL_Cull(CT_TWO_SIDED);

	if (r_useScissor.GetBool()) {
		GL_Scissor(0, 0, renderSystem->GetWidth(), renderSystem->GetHeight());
	}
}

void GL_State(uint64 stateBits, bool forceGlState) {
	// TODO: Finish Implementing
	uint64 diff = stateBits ^ backEnd.glState.glStateBits;

	if (!r_useStateCaching.GetBool() || forceGlState) {
		// make sure everything is set all the time, so we
		// can see if our delta checking is screwing up
		diff = 0xFFFFFFFFFFFFFFFF;
	}
	else if (diff == 0) {
		return;
	}

	//
	// check depthFunc bits
	//
	/*if (diff & GLS_DEPTHFUNC_BITS) {
		switch (stateBits & GLS_DEPTHFUNC_BITS) {
		case GLS_DEPTHFUNC_EQUAL:	qglDepthFunc(GL_EQUAL); break;
		case GLS_DEPTHFUNC_ALWAYS:	qglDepthFunc(GL_ALWAYS); break;
		case GLS_DEPTHFUNC_LESS:	qglDepthFunc(GL_LEQUAL); break;
		case GLS_DEPTHFUNC_GREATER:	qglDepthFunc(GL_GEQUAL); break;
		}
	}*/

	//
	// check blend bits
	//
	/*if (diff & (GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS)) {
		D3D12_BLEND srcFactor = D3D12_BLEND_ONE;
		D3D12_BLEND dstFactor = D3D12_BLEND_ZERO;

		switch (stateBits & GLS_SRCBLEND_BITS) {
		case GLS_SRCBLEND_ZERO:					srcFactor = D3D12_BLEND_ZERO; break;
		case GLS_SRCBLEND_ONE:					srcFactor = D3D12_BLEND_ONE; break;
		case GLS_SRCBLEND_DST_COLOR:			srcFactor = D3D12_BLEND_DEST_COLOR; break;
		case GLS_SRCBLEND_ONE_MINUS_DST_COLOR:	srcFactor = D3D12_BLEND_INV_DEST_COLOR; break;
		case GLS_SRCBLEND_SRC_ALPHA:			srcFactor = D3D12_BLEND_SRC_ALPHA; break;
		case GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA:	srcFactor = D3D12_BLEND_INV_SRC_ALPHA; break;
		case GLS_SRCBLEND_DST_ALPHA:			srcFactor = D3D12_BLEND_DEST_ALPHA; break;
		case GLS_SRCBLEND_ONE_MINUS_DST_ALPHA:	srcFactor = D3D12_BLEND_INV_DEST_ALPHA; break;
		default:
			assert(!"GL_State: invalid src blend state bits\n");
			break;
		}

		switch (stateBits & GLS_DSTBLEND_BITS) {
		case GLS_DSTBLEND_ZERO:					dstFactor = D3D12_BLEND_ZERO; break;
		case GLS_DSTBLEND_ONE:					dstFactor = D3D12_BLEND_ONE; break;
		case GLS_DSTBLEND_SRC_COLOR:			dstFactor = D3D12_BLEND_SRC_COLOR; break;
		case GLS_DSTBLEND_ONE_MINUS_SRC_COLOR:	dstFactor = D3D12_BLEND_INV_SRC_COLOR; break;
		case GLS_DSTBLEND_SRC_ALPHA:			dstFactor = D3D12_BLEND_SRC_ALPHA; break;
		case GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA:	dstFactor = D3D12_BLEND_INV_SRC_ALPHA; break;
		case GLS_DSTBLEND_DST_ALPHA:			dstFactor = D3D12_BLEND_DEST_ALPHA; break;
		case GLS_DSTBLEND_ONE_MINUS_DST_ALPHA:  dstFactor = D3D12_BLEND_INV_DEST_ALPHA; break;
		default:
			assert(!"GL_State: invalid dst blend state bits\n");
			break;
		}

		// Only actually update GL's blend func if blending is enabled.
		if (srcFactor == D3D12_BLEND_ONE && dstFactor == D3D12_BLEND_ZERO) {
			//qglDisable(GL_BLEND);
		}
		else {
			//qglEnable(GL_BLEND);
			//qglBlendFunc(srcFactor, dstFactor);
		}
	}*/

	//
	// check depthmask
	//
	/*if (diff & GLS_DEPTHMASK) {
		if (stateBits & GLS_DEPTHMASK) {
			qglDepthMask(GL_FALSE);
		}
		else {
			qglDepthMask(GL_TRUE);
		}
	}*/

	//
	// check colormask
	//
	/*if (diff & (GLS_REDMASK | GLS_GREENMASK | GLS_BLUEMASK | GLS_ALPHAMASK)) {
		GLboolean r = (stateBits & GLS_REDMASK) ? GL_FALSE : GL_TRUE;
		GLboolean g = (stateBits & GLS_GREENMASK) ? GL_FALSE : GL_TRUE;
		GLboolean b = (stateBits & GLS_BLUEMASK) ? GL_FALSE : GL_TRUE;
		GLboolean a = (stateBits & GLS_ALPHAMASK) ? GL_FALSE : GL_TRUE;
		qglColorMask(r, g, b, a);
	}*/

	//
	// fill/line mode
	//
	/*if (diff & GLS_POLYMODE_LINE) {
		if (stateBits & GLS_POLYMODE_LINE) {
			qglPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
		else {
			qglPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}
	}*/

	//
	// polygon offset
	//
	/*if (diff & GLS_POLYGON_OFFSET) {
		if (stateBits & GLS_POLYGON_OFFSET) {
			qglPolygonOffset(backEnd.glState.polyOfsScale, backEnd.glState.polyOfsBias);
			qglEnable(GL_POLYGON_OFFSET_FILL);
			qglEnable(GL_POLYGON_OFFSET_LINE);
		}
		else {
			qglDisable(GL_POLYGON_OFFSET_FILL);
			qglDisable(GL_POLYGON_OFFSET_LINE);
		}
	}*/

#if !defined( USE_CORE_PROFILE )
	//
	// alpha test
	//
	/*if (diff & (GLS_ALPHATEST_FUNC_BITS | GLS_ALPHATEST_FUNC_REF_BITS)) {
		if ((stateBits & GLS_ALPHATEST_FUNC_BITS) != 0) {
			qglEnable(GL_ALPHA_TEST);

			GLenum func = GL_ALWAYS;
			switch (stateBits & GLS_ALPHATEST_FUNC_BITS) {
			case GLS_ALPHATEST_FUNC_LESS:		func = GL_LESS; break;
			case GLS_ALPHATEST_FUNC_EQUAL:		func = GL_EQUAL; break;
			case GLS_ALPHATEST_FUNC_GREATER:	func = GL_GEQUAL; break;
			default: assert(false);
			}
			GLclampf ref = ((stateBits & GLS_ALPHATEST_FUNC_REF_BITS) >> GLS_ALPHATEST_FUNC_REF_SHIFT) / (float)0xFF;
			qglAlphaFunc(func, ref);
		}
		else {
			qglDisable(GL_ALPHA_TEST);
		}
	}*/
#endif

	//
	// stencil
	//
	if (diff & (GLS_STENCIL_FUNC_BITS | GLS_STENCIL_OP_BITS)) {
		/*if ((stateBits & (GLS_STENCIL_FUNC_BITS | GLS_STENCIL_OP_BITS)) != 0) {
			qglEnable(GL_STENCIL_TEST);
		}
		else {
			qglDisable(GL_STENCIL_TEST);
		}*/
	}
	if (diff & (GLS_STENCIL_FUNC_BITS | GLS_STENCIL_FUNC_REF_BITS | GLS_STENCIL_FUNC_MASK_BITS)) {
		/*GLuint ref = GLuint((stateBits & GLS_STENCIL_FUNC_REF_BITS) >> GLS_STENCIL_FUNC_REF_SHIFT);
		GLuint mask = GLuint((stateBits & GLS_STENCIL_FUNC_MASK_BITS) >> GLS_STENCIL_FUNC_MASK_SHIFT);
		GLenum func = 0;

		switch (stateBits & GLS_STENCIL_FUNC_BITS) {
		case GLS_STENCIL_FUNC_NEVER:		func = GL_NEVER; break;
		case GLS_STENCIL_FUNC_LESS:			func = GL_LESS; break;
		case GLS_STENCIL_FUNC_EQUAL:		func = GL_EQUAL; break;
		case GLS_STENCIL_FUNC_LEQUAL:		func = GL_LEQUAL; break;
		case GLS_STENCIL_FUNC_GREATER:		func = GL_GREATER; break;
		case GLS_STENCIL_FUNC_NOTEQUAL:		func = GL_NOTEQUAL; break;
		case GLS_STENCIL_FUNC_GEQUAL:		func = GL_GEQUAL; break;
		case GLS_STENCIL_FUNC_ALWAYS:		func = GL_ALWAYS; break;
		}
		qglStencilFunc(func, ref, mask);*/
	}
	if (diff & (GLS_STENCIL_OP_FAIL_BITS | GLS_STENCIL_OP_ZFAIL_BITS | GLS_STENCIL_OP_PASS_BITS)) {
		/*GLenum sFail = 0;
		GLenum zFail = 0;
		GLenum pass = 0;

		switch (stateBits & GLS_STENCIL_OP_FAIL_BITS) {
		case GLS_STENCIL_OP_FAIL_KEEP:		sFail = GL_KEEP; break;
		case GLS_STENCIL_OP_FAIL_ZERO:		sFail = GL_ZERO; break;
		case GLS_STENCIL_OP_FAIL_REPLACE:	sFail = GL_REPLACE; break;
		case GLS_STENCIL_OP_FAIL_INCR:		sFail = GL_INCR; break;
		case GLS_STENCIL_OP_FAIL_DECR:		sFail = GL_DECR; break;
		case GLS_STENCIL_OP_FAIL_INVERT:	sFail = GL_INVERT; break;
		case GLS_STENCIL_OP_FAIL_INCR_WRAP: sFail = GL_INCR_WRAP; break;
		case GLS_STENCIL_OP_FAIL_DECR_WRAP: sFail = GL_DECR_WRAP; break;
		}
		switch (stateBits & GLS_STENCIL_OP_ZFAIL_BITS) {
		case GLS_STENCIL_OP_ZFAIL_KEEP:		zFail = GL_KEEP; break;
		case GLS_STENCIL_OP_ZFAIL_ZERO:		zFail = GL_ZERO; break;
		case GLS_STENCIL_OP_ZFAIL_REPLACE:	zFail = GL_REPLACE; break;
		case GLS_STENCIL_OP_ZFAIL_INCR:		zFail = GL_INCR; break;
		case GLS_STENCIL_OP_ZFAIL_DECR:		zFail = GL_DECR; break;
		case GLS_STENCIL_OP_ZFAIL_INVERT:	zFail = GL_INVERT; break;
		case GLS_STENCIL_OP_ZFAIL_INCR_WRAP:zFail = GL_INCR_WRAP; break;
		case GLS_STENCIL_OP_ZFAIL_DECR_WRAP:zFail = GL_DECR_WRAP; break;
		}
		switch (stateBits & GLS_STENCIL_OP_PASS_BITS) {
		case GLS_STENCIL_OP_PASS_KEEP:		pass = GL_KEEP; break;
		case GLS_STENCIL_OP_PASS_ZERO:		pass = GL_ZERO; break;
		case GLS_STENCIL_OP_PASS_REPLACE:	pass = GL_REPLACE; break;
		case GLS_STENCIL_OP_PASS_INCR:		pass = GL_INCR; break;
		case GLS_STENCIL_OP_PASS_DECR:		pass = GL_DECR; break;
		case GLS_STENCIL_OP_PASS_INVERT:	pass = GL_INVERT; break;
		case GLS_STENCIL_OP_PASS_INCR_WRAP:	pass = GL_INCR_WRAP; break;
		case GLS_STENCIL_OP_PASS_DECR_WRAP:	pass = GL_DECR_WRAP; break;
		}
		qglStencilOp(sFail, zFail, pass);*/
	}

	backEnd.glState.glStateBits = stateBits;
}

uint64 GL_GetCurrentState() {
	return backEnd.glState.glStateBits;
}

uint64 GL_GetCurrentStateMinusStencil() {
	return GL_GetCurrentState() & ~(GLS_STENCIL_OP_BITS | GLS_STENCIL_FUNC_BITS | GLS_STENCIL_FUNC_REF_BITS | GLS_STENCIL_FUNC_MASK_BITS);
}
