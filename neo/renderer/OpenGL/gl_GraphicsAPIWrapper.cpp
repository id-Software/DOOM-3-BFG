/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013-2014 Robert Beckebans

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

/*
====================
GL_SelectTexture
====================
*/
void GL_SelectTexture( int unit )
{
	if( backEnd.glState.currenttmu == unit )
	{
		return;
	}
	
	if( unit < 0 || unit >= glConfig.maxTextureImageUnits )
	{
		common->Warning( "GL_SelectTexture: unit = %i", unit );
		return;
	}
	
	RENDERLOG_PRINTF( "GL_SelectTexture( %i );\n", unit );
	
	backEnd.glState.currenttmu = unit;
}

/*
====================
GL_Cull

This handles the flipping needed when the view being
rendered is a mirored view.
====================
*/
void GL_Cull( int cullType )
{
	if( backEnd.glState.faceCulling == cullType )
	{
		return;
	}
	
	if( cullType == CT_TWO_SIDED )
	{
		glDisable( GL_CULL_FACE );
	}
	else
	{
		if( backEnd.glState.faceCulling == CT_TWO_SIDED )
		{
			glEnable( GL_CULL_FACE );
		}
		
		if( cullType == CT_BACK_SIDED )
		{
			if( backEnd.viewDef->isMirror )
			{
				glCullFace( GL_FRONT );
			}
			else
			{
				glCullFace( GL_BACK );
			}
		}
		else
		{
			if( backEnd.viewDef->isMirror )
			{
				glCullFace( GL_BACK );
			}
			else
			{
				glCullFace( GL_FRONT );
			}
		}
	}
	
	backEnd.glState.faceCulling = cullType;
}

/*
====================
GL_Scissor
====================
*/
void GL_Scissor( int x /* left*/, int y /* bottom */, int w, int h )
{
	glScissor( x, y, w, h );
}

/*
====================
GL_Viewport
====================
*/
void GL_Viewport( int x /* left */, int y /* bottom */, int w, int h )
{
	glViewport( x, y, w, h );
}

/*
====================
GL_PolygonOffset
====================
*/
void GL_PolygonOffset( float scale, float bias )
{
	backEnd.glState.polyOfsScale = scale;
	backEnd.glState.polyOfsBias = bias;
	if( backEnd.glState.glStateBits & GLS_POLYGON_OFFSET )
	{
		glPolygonOffset( scale, bias );
	}
}

/*
========================
GL_DepthBoundsTest
========================
*/
void GL_DepthBoundsTest( const float zmin, const float zmax )
{
	if( !glConfig.depthBoundsTestAvailable || zmin > zmax )
	{
		return;
	}
	
	if( zmin == 0.0f && zmax == 0.0f )
	{
		glDisable( GL_DEPTH_BOUNDS_TEST_EXT );
	}
	else
	{
		glEnable( GL_DEPTH_BOUNDS_TEST_EXT );
		glDepthBoundsEXT( zmin, zmax );
	}
}

/*
========================
GL_StartDepthPass
========================
*/
void GL_StartDepthPass( const idScreenRect& rect )
{
}

/*
========================
GL_FinishDepthPass
========================
*/
void GL_FinishDepthPass()
{
}

/*
========================
GL_GetDepthPassRect
========================
*/
void GL_GetDepthPassRect( idScreenRect& rect )
{
	rect.Clear();
}

/*
====================
GL_Color
====================
*/
/*
void GL_Color( float* color )
{
	if( color == NULL )
	{
		return;
	}
	GL_Color( color[0], color[1], color[2], color[3] );
}
*/

// RB begin
void GL_Color( const idVec3& color )
{
	GL_Color( color[0], color[1], color[2], 1.0f );
}

void GL_Color( const idVec4& color )
{
	GL_Color( color[0], color[1], color[2], color[3] );
}
// RB end

/*
====================
GL_Color
====================
*/
void GL_Color( float r, float g, float b )
{
	GL_Color( r, g, b, 1.0f );
}

/*
====================
GL_Color
====================
*/
void GL_Color( float r, float g, float b, float a )
{
	float parm[4];
	parm[0] = idMath::ClampFloat( 0.0f, 1.0f, r );
	parm[1] = idMath::ClampFloat( 0.0f, 1.0f, g );
	parm[2] = idMath::ClampFloat( 0.0f, 1.0f, b );
	parm[3] = idMath::ClampFloat( 0.0f, 1.0f, a );
	renderProgManager.SetRenderParm( RENDERPARM_COLOR, parm );
}

/*
========================
GL_Clear
========================
*/
void GL_Clear( bool color, bool depth, bool stencil, byte stencilValue, float r, float g, float b, float a, bool clearHDR )
{
	int clearFlags = 0;
	if( color )
	{
		glClearColor( r, g, b, a );
		clearFlags |= GL_COLOR_BUFFER_BIT;
	}
	if( depth )
	{
		clearFlags |= GL_DEPTH_BUFFER_BIT;
	}
	if( stencil )
	{
		glClearStencil( stencilValue );
		clearFlags |= GL_STENCIL_BUFFER_BIT;
	}
	glClear( clearFlags );
	
	// RB begin
	if( r_useHDR.GetBool() && clearHDR && globalFramebuffers.hdrFBO != NULL )
	{
		bool isDefaultFramebufferActive = Framebuffer::IsDefaultFramebufferActive();
		
		globalFramebuffers.hdrFBO->Bind();
		glClear( clearFlags );
		
		if( isDefaultFramebufferActive )
		{
			Framebuffer::Unbind();
		}
	}
	// RB end
}

/*
========================
GL_SetDefaultState

This should initialize all GL state that any part of the entire program
may touch, including the editor.
========================
*/
void GL_SetDefaultState()
{
	RENDERLOG_PRINTF( "--- GL_SetDefaultState ---\n" );
	
	glClearDepth( 1.0f );
	
	// make sure our GL state vector is set correctly
	memset( &backEnd.glState, 0, sizeof( backEnd.glState ) );
	GL_State( 0, true );
	
	// RB begin
	Framebuffer::Unbind();
	// RB end
	
	// These are changed by GL_Cull
	glCullFace( GL_FRONT_AND_BACK );
	glEnable( GL_CULL_FACE );
	
	// These are changed by GL_State
	glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
	glBlendFunc( GL_ONE, GL_ZERO );
	glDepthMask( GL_TRUE );
	glDepthFunc( GL_LESS );
	glDisable( GL_STENCIL_TEST );
	glDisable( GL_POLYGON_OFFSET_FILL );
	glDisable( GL_POLYGON_OFFSET_LINE );
	glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	
	// These should never be changed
	// DG: deprecated in opengl 3.2 and not needed because we don't do fixed function pipeline
	// glShadeModel( GL_SMOOTH );
	// DG end
	glEnable( GL_DEPTH_TEST );
	glEnable( GL_BLEND );
	glEnable( GL_SCISSOR_TEST );
	glDrawBuffer( GL_BACK );
	glReadBuffer( GL_BACK );
	
	if( r_useScissor.GetBool() )
	{
		glScissor( 0, 0, renderSystem->GetWidth(), renderSystem->GetHeight() );
	}
	
	// RB: don't keep renderprogs that were enabled during level load
	renderProgManager.Unbind();
	// RB end
}

/*
====================
GL_State

This routine is responsible for setting the most commonly changed state
====================
*/
void GL_State( uint64 stateBits, bool forceGlState )
{
	uint64 diff = stateBits ^ backEnd.glState.glStateBits;
	
	if( !r_useStateCaching.GetBool() || forceGlState )
	{
		// make sure everything is set all the time, so we
		// can see if our delta checking is screwing up
		diff = 0xFFFFFFFFFFFFFFFF;
	}
	else if( diff == 0 )
	{
		return;
	}
	
	//
	// check depthFunc bits
	//
	if( diff & GLS_DEPTHFUNC_BITS )
	{
		switch( stateBits & GLS_DEPTHFUNC_BITS )
		{
			case GLS_DEPTHFUNC_EQUAL:
				glDepthFunc( GL_EQUAL );
				break;
			case GLS_DEPTHFUNC_ALWAYS:
				glDepthFunc( GL_ALWAYS );
				break;
			case GLS_DEPTHFUNC_LESS:
				glDepthFunc( GL_LEQUAL );
				break;
			case GLS_DEPTHFUNC_GREATER:
				glDepthFunc( GL_GEQUAL );
				break;
		}
	}
	
	//
	// check blend bits
	//
	if( diff & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) )
	{
		GLenum srcFactor = GL_ONE;
		GLenum dstFactor = GL_ZERO;
		
		switch( stateBits & GLS_SRCBLEND_BITS )
		{
			case GLS_SRCBLEND_ZERO:
				srcFactor = GL_ZERO;
				break;
			case GLS_SRCBLEND_ONE:
				srcFactor = GL_ONE;
				break;
			case GLS_SRCBLEND_DST_COLOR:
				srcFactor = GL_DST_COLOR;
				break;
			case GLS_SRCBLEND_ONE_MINUS_DST_COLOR:
				srcFactor = GL_ONE_MINUS_DST_COLOR;
				break;
			case GLS_SRCBLEND_SRC_ALPHA:
				srcFactor = GL_SRC_ALPHA;
				break;
			case GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA:
				srcFactor = GL_ONE_MINUS_SRC_ALPHA;
				break;
			case GLS_SRCBLEND_DST_ALPHA:
				srcFactor = GL_DST_ALPHA;
				break;
			case GLS_SRCBLEND_ONE_MINUS_DST_ALPHA:
				srcFactor = GL_ONE_MINUS_DST_ALPHA;
				break;
			default:
				assert( !"GL_State: invalid src blend state bits\n" );
				break;
		}
		
		switch( stateBits & GLS_DSTBLEND_BITS )
		{
			case GLS_DSTBLEND_ZERO:
				dstFactor = GL_ZERO;
				break;
			case GLS_DSTBLEND_ONE:
				dstFactor = GL_ONE;
				break;
			case GLS_DSTBLEND_SRC_COLOR:
				dstFactor = GL_SRC_COLOR;
				break;
			case GLS_DSTBLEND_ONE_MINUS_SRC_COLOR:
				dstFactor = GL_ONE_MINUS_SRC_COLOR;
				break;
			case GLS_DSTBLEND_SRC_ALPHA:
				dstFactor = GL_SRC_ALPHA;
				break;
			case GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA:
				dstFactor = GL_ONE_MINUS_SRC_ALPHA;
				break;
			case GLS_DSTBLEND_DST_ALPHA:
				dstFactor = GL_DST_ALPHA;
				break;
			case GLS_DSTBLEND_ONE_MINUS_DST_ALPHA:
				dstFactor = GL_ONE_MINUS_DST_ALPHA;
				break;
			default:
				assert( !"GL_State: invalid dst blend state bits\n" );
				break;
		}
		
		// Only actually update GL's blend func if blending is enabled.
		if( srcFactor == GL_ONE && dstFactor == GL_ZERO )
		{
			glDisable( GL_BLEND );
		}
		else
		{
			glEnable( GL_BLEND );
			glBlendFunc( srcFactor, dstFactor );
		}
	}
	
	//
	// check depthmask
	//
	if( diff & GLS_DEPTHMASK )
	{
		if( stateBits & GLS_DEPTHMASK )
		{
			glDepthMask( GL_FALSE );
		}
		else
		{
			glDepthMask( GL_TRUE );
		}
	}
	
	//
	// check colormask
	//
	if( diff & ( GLS_REDMASK | GLS_GREENMASK | GLS_BLUEMASK | GLS_ALPHAMASK ) )
	{
		GLboolean r = ( stateBits & GLS_REDMASK ) ? GL_FALSE : GL_TRUE;
		GLboolean g = ( stateBits & GLS_GREENMASK ) ? GL_FALSE : GL_TRUE;
		GLboolean b = ( stateBits & GLS_BLUEMASK ) ? GL_FALSE : GL_TRUE;
		GLboolean a = ( stateBits & GLS_ALPHAMASK ) ? GL_FALSE : GL_TRUE;
		glColorMask( r, g, b, a );
	}
	
	//
	// fill/line mode
	//
	if( diff & GLS_POLYMODE_LINE )
	{
		if( stateBits & GLS_POLYMODE_LINE )
		{
			glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
		}
		else
		{
			glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
		}
	}
	
	//
	// polygon offset
	//
	if( diff & GLS_POLYGON_OFFSET )
	{
		if( stateBits & GLS_POLYGON_OFFSET )
		{
			glPolygonOffset( backEnd.glState.polyOfsScale, backEnd.glState.polyOfsBias );
			glEnable( GL_POLYGON_OFFSET_FILL );
			glEnable( GL_POLYGON_OFFSET_LINE );
		}
		else
		{
			glDisable( GL_POLYGON_OFFSET_FILL );
			glDisable( GL_POLYGON_OFFSET_LINE );
		}
	}
	
#if !defined( USE_CORE_PROFILE )
	//
	// alpha test
	//
	if( diff & ( GLS_ALPHATEST_FUNC_BITS | GLS_ALPHATEST_FUNC_REF_BITS ) )
	{
		if( ( stateBits & GLS_ALPHATEST_FUNC_BITS ) != 0 )
		{
			glEnable( GL_ALPHA_TEST );
			
			GLenum func = GL_ALWAYS;
			switch( stateBits & GLS_ALPHATEST_FUNC_BITS )
			{
				case GLS_ALPHATEST_FUNC_LESS:
					func = GL_LESS;
					break;
				case GLS_ALPHATEST_FUNC_EQUAL:
					func = GL_EQUAL;
					break;
				case GLS_ALPHATEST_FUNC_GREATER:
					func = GL_GEQUAL;
					break;
				default:
					assert( false );
			}
			GLclampf ref = ( ( stateBits & GLS_ALPHATEST_FUNC_REF_BITS ) >> GLS_ALPHATEST_FUNC_REF_SHIFT ) / ( float )0xFF;
			glAlphaFunc( func, ref );
		}
		else
		{
			glDisable( GL_ALPHA_TEST );
		}
	}
#endif
	
	//
	// stencil
	//
	if( diff & ( GLS_STENCIL_FUNC_BITS | GLS_STENCIL_OP_BITS ) )
	{
		if( ( stateBits & ( GLS_STENCIL_FUNC_BITS | GLS_STENCIL_OP_BITS ) ) != 0 )
		{
			glEnable( GL_STENCIL_TEST );
		}
		else
		{
			glDisable( GL_STENCIL_TEST );
		}
	}
	if( diff & ( GLS_STENCIL_FUNC_BITS | GLS_STENCIL_FUNC_REF_BITS | GLS_STENCIL_FUNC_MASK_BITS ) )
	{
		GLuint ref = GLuint( ( stateBits & GLS_STENCIL_FUNC_REF_BITS ) >> GLS_STENCIL_FUNC_REF_SHIFT );
		GLuint mask = GLuint( ( stateBits & GLS_STENCIL_FUNC_MASK_BITS ) >> GLS_STENCIL_FUNC_MASK_SHIFT );
		GLenum func = 0;
		
		switch( stateBits & GLS_STENCIL_FUNC_BITS )
		{
			case GLS_STENCIL_FUNC_NEVER:
				func = GL_NEVER;
				break;
			case GLS_STENCIL_FUNC_LESS:
				func = GL_LESS;
				break;
			case GLS_STENCIL_FUNC_EQUAL:
				func = GL_EQUAL;
				break;
			case GLS_STENCIL_FUNC_LEQUAL:
				func = GL_LEQUAL;
				break;
			case GLS_STENCIL_FUNC_GREATER:
				func = GL_GREATER;
				break;
			case GLS_STENCIL_FUNC_NOTEQUAL:
				func = GL_NOTEQUAL;
				break;
			case GLS_STENCIL_FUNC_GEQUAL:
				func = GL_GEQUAL;
				break;
			case GLS_STENCIL_FUNC_ALWAYS:
				func = GL_ALWAYS;
				break;
		}
		glStencilFunc( func, ref, mask );
	}
	if( diff & ( GLS_STENCIL_OP_FAIL_BITS | GLS_STENCIL_OP_ZFAIL_BITS | GLS_STENCIL_OP_PASS_BITS ) )
	{
		GLenum sFail = 0;
		GLenum zFail = 0;
		GLenum pass = 0;
		
		switch( stateBits & GLS_STENCIL_OP_FAIL_BITS )
		{
			case GLS_STENCIL_OP_FAIL_KEEP:
				sFail = GL_KEEP;
				break;
			case GLS_STENCIL_OP_FAIL_ZERO:
				sFail = GL_ZERO;
				break;
			case GLS_STENCIL_OP_FAIL_REPLACE:
				sFail = GL_REPLACE;
				break;
			case GLS_STENCIL_OP_FAIL_INCR:
				sFail = GL_INCR;
				break;
			case GLS_STENCIL_OP_FAIL_DECR:
				sFail = GL_DECR;
				break;
			case GLS_STENCIL_OP_FAIL_INVERT:
				sFail = GL_INVERT;
				break;
			case GLS_STENCIL_OP_FAIL_INCR_WRAP:
				sFail = GL_INCR_WRAP;
				break;
			case GLS_STENCIL_OP_FAIL_DECR_WRAP:
				sFail = GL_DECR_WRAP;
				break;
		}
		switch( stateBits & GLS_STENCIL_OP_ZFAIL_BITS )
		{
			case GLS_STENCIL_OP_ZFAIL_KEEP:
				zFail = GL_KEEP;
				break;
			case GLS_STENCIL_OP_ZFAIL_ZERO:
				zFail = GL_ZERO;
				break;
			case GLS_STENCIL_OP_ZFAIL_REPLACE:
				zFail = GL_REPLACE;
				break;
			case GLS_STENCIL_OP_ZFAIL_INCR:
				zFail = GL_INCR;
				break;
			case GLS_STENCIL_OP_ZFAIL_DECR:
				zFail = GL_DECR;
				break;
			case GLS_STENCIL_OP_ZFAIL_INVERT:
				zFail = GL_INVERT;
				break;
			case GLS_STENCIL_OP_ZFAIL_INCR_WRAP:
				zFail = GL_INCR_WRAP;
				break;
			case GLS_STENCIL_OP_ZFAIL_DECR_WRAP:
				zFail = GL_DECR_WRAP;
				break;
		}
		switch( stateBits & GLS_STENCIL_OP_PASS_BITS )
		{
			case GLS_STENCIL_OP_PASS_KEEP:
				pass = GL_KEEP;
				break;
			case GLS_STENCIL_OP_PASS_ZERO:
				pass = GL_ZERO;
				break;
			case GLS_STENCIL_OP_PASS_REPLACE:
				pass = GL_REPLACE;
				break;
			case GLS_STENCIL_OP_PASS_INCR:
				pass = GL_INCR;
				break;
			case GLS_STENCIL_OP_PASS_DECR:
				pass = GL_DECR;
				break;
			case GLS_STENCIL_OP_PASS_INVERT:
				pass = GL_INVERT;
				break;
			case GLS_STENCIL_OP_PASS_INCR_WRAP:
				pass = GL_INCR_WRAP;
				break;
			case GLS_STENCIL_OP_PASS_DECR_WRAP:
				pass = GL_DECR_WRAP;
				break;
		}
		glStencilOp( sFail, zFail, pass );
	}
	
	backEnd.glState.glStateBits = stateBits;
}

/*
=================
GL_GetCurrentState
=================
*/
uint64 GL_GetCurrentState()
{
	return backEnd.glState.glStateBits;
}

/*
========================
GL_GetCurrentStateMinusStencil
========================
*/
uint64 GL_GetCurrentStateMinusStencil()
{
	return GL_GetCurrentState() & ~( GLS_STENCIL_OP_BITS | GLS_STENCIL_FUNC_BITS | GLS_STENCIL_FUNC_REF_BITS | GLS_STENCIL_FUNC_MASK_BITS );
}
