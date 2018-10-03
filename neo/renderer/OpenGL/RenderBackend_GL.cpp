/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013-2015 Robert Beckebans
Copyright (C) 2016-2017 Dustin Land

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

#include "../RenderCommon.h"
#include "../RenderBackend.h"
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

glContext_t glcontext;

/*
==================
GL_CheckErrors
==================
*/
// RB: added filename, line parms
bool GL_CheckErrors_( const char* filename, int line )
{
	int		err;
	char	s[64];
	int		i;
	
	if( r_ignoreGLErrors.GetBool() )
	{
		return false;
	}
	
	// check for up to 10 errors pending
	bool error = false;
	for( i = 0 ; i < 10 ; i++ )
	{
		err = glGetError();
		if( err == GL_NO_ERROR )
		{
			break;
		}
		
		error = true;
		switch( err )
		{
			case GL_INVALID_ENUM:
				strcpy( s, "GL_INVALID_ENUM" );
				break;
			case GL_INVALID_VALUE:
				strcpy( s, "GL_INVALID_VALUE" );
				break;
			case GL_INVALID_OPERATION:
				strcpy( s, "GL_INVALID_OPERATION" );
				break;
#if !defined(USE_GLES2) && !defined(USE_GLES3)
			case GL_STACK_OVERFLOW:
				strcpy( s, "GL_STACK_OVERFLOW" );
				break;
			case GL_STACK_UNDERFLOW:
				strcpy( s, "GL_STACK_UNDERFLOW" );
				break;
#endif
			case GL_OUT_OF_MEMORY:
				strcpy( s, "GL_OUT_OF_MEMORY" );
				break;
			default:
				idStr::snPrintf( s, sizeof( s ), "%i", err );
				break;
		}
		
		common->Printf( "caught OpenGL error: %s in file %s line %i\n", s, filename, line );
	}
	
	return error;
}
// RB end

/*
=============
idRenderBackend::DrawElementsWithCounters
=============
*/
void idRenderBackend::DrawElementsWithCounters( const drawSurf_t* surf )
{
	// get vertex buffer
	const vertCacheHandle_t vbHandle = surf->ambientCache;
	idVertexBuffer* vertexBuffer;
	if( vertexCache.CacheIsStatic( vbHandle ) )
	{
		vertexBuffer = &vertexCache.staticData.vertexBuffer;
	}
	else
	{
		const uint64 frameNum = ( int )( vbHandle >> VERTCACHE_FRAME_SHIFT ) & VERTCACHE_FRAME_MASK;
		if( frameNum != ( ( vertexCache.currentFrame - 1 ) & VERTCACHE_FRAME_MASK ) )
		{
			idLib::Warning( "RB_DrawElementsWithCounters, vertexBuffer == NULL" );
			return;
		}
		vertexBuffer = &vertexCache.frameData[vertexCache.drawListNum].vertexBuffer;
	}
	const int vertOffset = ( int )( vbHandle >> VERTCACHE_OFFSET_SHIFT ) & VERTCACHE_OFFSET_MASK;
	
	// get index buffer
	const vertCacheHandle_t ibHandle = surf->indexCache;
	idIndexBuffer* indexBuffer;
	if( vertexCache.CacheIsStatic( ibHandle ) )
	{
		indexBuffer = &vertexCache.staticData.indexBuffer;
	}
	else
	{
		const uint64 frameNum = ( int )( ibHandle >> VERTCACHE_FRAME_SHIFT ) & VERTCACHE_FRAME_MASK;
		if( frameNum != ( ( vertexCache.currentFrame - 1 ) & VERTCACHE_FRAME_MASK ) )
		{
			idLib::Warning( "RB_DrawElementsWithCounters, indexBuffer == NULL" );
			return;
		}
		indexBuffer = &vertexCache.frameData[vertexCache.drawListNum].indexBuffer;
	}
	// RB: 64 bit fixes, changed int to GLintptr
	const GLintptr indexOffset = ( GLintptr )( ibHandle >> VERTCACHE_OFFSET_SHIFT ) & VERTCACHE_OFFSET_MASK;
	// RB end
	
	RENDERLOG_PRINTF( "Binding Buffers: %p:%i %p:%i\n", vertexBuffer, vertOffset, indexBuffer, indexOffset );
	
	if( surf->jointCache )
	{
		// DG: this happens all the time in the erebus1 map with blendlight.vfp,
		// so don't call assert (through verify) here until it's fixed (if fixable)
		// else the game crashes on linux when using debug builds
		
		// FIXME: fix this properly if possible?
		// RB: yes but it would require an additional blend light skinned shader
		//if( !verify( renderProgManager.ShaderUsesJoints() ) )
		if( !renderProgManager.ShaderUsesJoints() )
			// DG end
		{
			return;
		}
	}
	else
	{
		if( !verify( !renderProgManager.ShaderUsesJoints() || renderProgManager.ShaderHasOptionalSkinning() ) )
		{
			return;
		}
	}
	
	
	if( surf->jointCache )
	{
		idUniformBuffer jointBuffer;
		if( !vertexCache.GetJointBuffer( surf->jointCache, &jointBuffer ) )
		{
			idLib::Warning( "RB_DrawElementsWithCounters, jointBuffer == NULL" );
			return;
		}
		assert( ( jointBuffer.GetOffset() & ( glConfig.uniformBufferOffsetAlignment - 1 ) ) == 0 );
		
		// RB: 64 bit fixes, changed GLuint to GLintptr
		const GLintptr ubo = jointBuffer.GetAPIObject();
		// RB end
		
		glBindBufferRange( GL_UNIFORM_BUFFER, 0, ubo, jointBuffer.GetOffset(), jointBuffer.GetSize() );
	}
	
	renderProgManager.CommitUniforms();
	
	// RB: 64 bit fixes, changed GLuint to GLintptr
	if( currentIndexBuffer != ( GLintptr )indexBuffer->GetAPIObject() || !r_useStateCaching.GetBool() )
	{
		glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ( GLintptr )indexBuffer->GetAPIObject() );
		currentIndexBuffer = ( GLintptr )indexBuffer->GetAPIObject();
	}
	
	if( ( vertexLayout != LAYOUT_DRAW_VERT ) || ( currentVertexBuffer != ( GLintptr )vertexBuffer->GetAPIObject() ) || !r_useStateCaching.GetBool() )
	{
		glBindBuffer( GL_ARRAY_BUFFER, ( GLintptr )vertexBuffer->GetAPIObject() );
		currentVertexBuffer = ( GLintptr )vertexBuffer->GetAPIObject();
		
		glEnableVertexAttribArray( PC_ATTRIB_INDEX_VERTEX );
		glEnableVertexAttribArray( PC_ATTRIB_INDEX_NORMAL );
		glEnableVertexAttribArray( PC_ATTRIB_INDEX_COLOR );
		glEnableVertexAttribArray( PC_ATTRIB_INDEX_COLOR2 );
		glEnableVertexAttribArray( PC_ATTRIB_INDEX_ST );
		glEnableVertexAttribArray( PC_ATTRIB_INDEX_TANGENT );
		
#if defined(USE_GLES2) || defined(USE_GLES3)
		glVertexAttribPointer( PC_ATTRIB_INDEX_VERTEX, 3, GL_FLOAT, GL_FALSE, sizeof( idDrawVert ), ( void* )( vertOffset + DRAWVERT_XYZ_OFFSET ) );
		glVertexAttribPointer( PC_ATTRIB_INDEX_NORMAL, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( idDrawVert ), ( void* )( vertOffset + DRAWVERT_NORMAL_OFFSET ) );
		glVertexAttribPointer( PC_ATTRIB_INDEX_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( idDrawVert ), ( void* )( vertOffset + DRAWVERT_COLOR_OFFSET ) );
		glVertexAttribPointer( PC_ATTRIB_INDEX_COLOR2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( idDrawVert ), ( void* )( vertOffset + DRAWVERT_COLOR2_OFFSET ) );
#if defined(USE_ANGLE)
		glVertexAttribPointer( PC_ATTRIB_INDEX_ST, 2, GL_HALF_FLOAT_OES, GL_TRUE, sizeof( idDrawVert ), ( void* )( vertOffset + DRAWVERT_ST_OFFSET ) );
#else
		glVertexAttribPointer( PC_ATTRIB_INDEX_ST, 2, GL_HALF_FLOAT, GL_TRUE, sizeof( idDrawVert ), ( void* )( vertOffset + DRAWVERT_ST_OFFSET ) );
#endif
		glVertexAttribPointer( PC_ATTRIB_INDEX_TANGENT, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( idDrawVert ), ( void* )( vertOffset + DRAWVERT_TANGENT_OFFSET ) );
		
#else
		glVertexAttribPointer( PC_ATTRIB_INDEX_VERTEX, 3, GL_FLOAT, GL_FALSE, sizeof( idDrawVert ), ( void* )( DRAWVERT_XYZ_OFFSET ) );
		glVertexAttribPointer( PC_ATTRIB_INDEX_NORMAL, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( idDrawVert ), ( void* )( DRAWVERT_NORMAL_OFFSET ) );
		glVertexAttribPointer( PC_ATTRIB_INDEX_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( idDrawVert ), ( void* )( DRAWVERT_COLOR_OFFSET ) );
		glVertexAttribPointer( PC_ATTRIB_INDEX_COLOR2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( idDrawVert ), ( void* )( DRAWVERT_COLOR2_OFFSET ) );
		glVertexAttribPointer( PC_ATTRIB_INDEX_ST, 2, GL_HALF_FLOAT, GL_TRUE, sizeof( idDrawVert ), ( void* )( DRAWVERT_ST_OFFSET ) );
		glVertexAttribPointer( PC_ATTRIB_INDEX_TANGENT, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( idDrawVert ), ( void* )( DRAWVERT_TANGENT_OFFSET ) );
#endif // #if defined(USE_GLES2) || defined(USE_GLES3)
		
		vertexLayout = LAYOUT_DRAW_VERT;
	}
	// RB end
	
#if defined(USE_GLES3) //defined(USE_GLES2)
	glDrawElements(	GL_TRIANGLES,
					r_singleTriangle.GetBool() ? 3 : surf->numIndexes,
					GL_INDEX_TYPE,
					( triIndex_t* )indexOffset );
#else
	glDrawElementsBaseVertex( GL_TRIANGLES,
							  r_singleTriangle.GetBool() ? 3 : surf->numIndexes,
							  GL_INDEX_TYPE,
							  ( triIndex_t* )indexOffset,
							  vertOffset / sizeof( idDrawVert ) );
#endif
					
	// RB: added stats
	pc.c_drawElements++;
	pc.c_drawIndexes += surf->numIndexes;
	// RB end
}


/*
=========================================================================================================

GL COMMANDS

=========================================================================================================
*/

/*
==================
idRenderBackend::GL_StartFrame
==================
*/
void idRenderBackend::GL_StartFrame()
{

}

/*
==================
idRenderBackend::GL_EndFrame
==================
*/
void idRenderBackend::GL_EndFrame()
{
	// Fix for the steam overlay not showing up while in game without Shell/Debug/Console/Menu also rendering
	glColorMask( 1, 1, 1, 1 );
	
	glFlush();
}

/*
========================
GL_SetDefaultState

This should initialize all GL state that any part of the entire program
may touch, including the editor.
========================
*/
void idRenderBackend::GL_SetDefaultState()
{
	RENDERLOG_PRINTF( "--- GL_SetDefaultState ---\n" );
	
	glClearDepth( 1.0f );
	
	// make sure our GL state vector is set correctly
	memset( &glcontext.tmu, 0, sizeof( glcontext.tmu ) );
	currenttmu = 0;
	currentVertexBuffer = 0;
	currentIndexBuffer = 0;
	currentFramebuffer = 0;
	faceCulling = 0;
	vertexLayout = LAYOUT_UNKNOWN;
	polyOfsScale = 0.0f;
	polyOfsBias = 0.0f;
	glStateBits = 0;
	
	hdrAverageLuminance = 0;
	hdrMaxLuminance = 0;
	hdrTime = 0;
	hdrKey = 0;
	
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
idRenderBackend::GL_State

This routine is responsible for setting the most commonly changed state
====================
*/
void idRenderBackend::GL_State( uint64 stateBits, bool forceGlState )
{
	uint64 diff = stateBits ^ glStateBits;
	
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
			glPolygonOffset( polyOfsScale, polyOfsBias );
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
	
	glStateBits = stateBits;
}

/*
====================
idRenderBackend::SelectTexture
====================
*/
void idRenderBackend::GL_SelectTexture( int unit )
{
	if( currenttmu == unit )
	{
		return;
	}
	
	if( unit < 0 || unit >= glConfig.maxTextureImageUnits )
	{
		common->Warning( "GL_SelectTexture: unit = %i", unit );
		return;
	}
	
	RENDERLOG_PRINTF( "GL_SelectTexture( %i );\n", unit );
	
	currenttmu = unit;
}

/*
====================
idRenderBackend::GL_Cull

This handles the flipping needed when the view being
rendered is a mirored view.
====================
*/
void idRenderBackend::GL_Cull( cullType_t cullType )
{
	if( faceCulling == cullType )
	{
		return;
	}
	
	if( cullType == CT_TWO_SIDED )
	{
		glDisable( GL_CULL_FACE );
	}
	else
	{
		if( faceCulling == CT_TWO_SIDED )
		{
			glEnable( GL_CULL_FACE );
		}
		
		if( cullType == CT_BACK_SIDED )
		{
			if( viewDef->isMirror )
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
			if( viewDef->isMirror )
			{
				glCullFace( GL_BACK );
			}
			else
			{
				glCullFace( GL_FRONT );
			}
		}
	}
	
	faceCulling = cullType;
}

/*
====================
idRenderBackend::GL_Scissor
====================
*/
void idRenderBackend::GL_Scissor( int x /* left*/, int y /* bottom */, int w, int h )
{
	glScissor( x, y, w, h );
}

/*
====================
idRenderBackend::GL_Viewport
====================
*/
void idRenderBackend::GL_Viewport( int x /* left */, int y /* bottom */, int w, int h )
{
	glViewport( x, y, w, h );
}

/*
====================
idRenderBackend::GL_PolygonOffset
====================
*/
void idRenderBackend::GL_PolygonOffset( float scale, float bias )
{
	polyOfsScale = scale;
	polyOfsBias = bias;
	
	if( glStateBits & GLS_POLYGON_OFFSET )
	{
		glPolygonOffset( scale, bias );
	}
}

/*
========================
idRenderBackend::GL_DepthBoundsTest
========================
*/
void idRenderBackend::GL_DepthBoundsTest( const float zmin, const float zmax )
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
====================
idRenderBackend::GL_Color
====================
*/
void idRenderBackend::GL_Color( float r, float g, float b, float a )
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
idRenderBackend::GL_Clear
========================
*/
void idRenderBackend::GL_Clear( bool color, bool depth, bool stencil, byte stencilValue, float r, float g, float b, float a, bool clearHDR )
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
=================
idRenderBackend::GL_GetCurrentState
=================
*/
uint64 idRenderBackend::GL_GetCurrentState() const
{
	return tr.backend.glStateBits;
}

/*
========================
idRenderBackend::GL_GetCurrentStateMinusStencil
========================
*/
uint64 idRenderBackend::GL_GetCurrentStateMinusStencil() const
{
	return GL_GetCurrentState() & ~( GLS_STENCIL_OP_BITS | GLS_STENCIL_FUNC_BITS | GLS_STENCIL_FUNC_REF_BITS | GLS_STENCIL_FUNC_MASK_BITS );
}


/*
=============
idRenderBackend::CheckCVars

See if some cvars that we watch have changed
=============
*/
void idRenderBackend::CheckCVars()
{
	// gamma stuff
	if( r_gamma.IsModified() || r_brightness.IsModified() )
	{
		r_gamma.ClearModified();
		r_brightness.ClearModified();
		R_SetColorMappings();
	}
	
	// filtering
	if( r_maxAnisotropicFiltering.IsModified() || r_useTrilinearFiltering.IsModified() || r_lodBias.IsModified() )
	{
		idLib::Printf( "Updating texture filter parameters.\n" );
		r_maxAnisotropicFiltering.ClearModified();
		r_useTrilinearFiltering.ClearModified();
		r_lodBias.ClearModified();
		
		for( int i = 0 ; i < globalImages->images.Num() ; i++ )
		{
			if( globalImages->images[i] )
			{
				globalImages->images[i]->Bind();
				globalImages->images[i]->SetTexParameters();
			}
		}
	}
	
	extern idCVar r_useSeamlessCubeMap;
	if( r_useSeamlessCubeMap.IsModified() )
	{
		r_useSeamlessCubeMap.ClearModified();
		if( glConfig.seamlessCubeMapAvailable )
		{
			if( r_useSeamlessCubeMap.GetBool() )
			{
				glEnable( GL_TEXTURE_CUBE_MAP_SEAMLESS );
			}
			else
			{
				glDisable( GL_TEXTURE_CUBE_MAP_SEAMLESS );
			}
		}
	}
	
	extern idCVar r_useSRGB;
	if( r_useSRGB.IsModified() )
	{
		r_useSRGB.ClearModified();
		if( glConfig.sRGBFramebufferAvailable )
		{
			if( r_useSRGB.GetBool() && r_useSRGB.GetInteger() != 3 )
			{
				glEnable( GL_FRAMEBUFFER_SRGB );
			}
			else
			{
				glDisable( GL_FRAMEBUFFER_SRGB );
			}
		}
	}
	
	if( r_antiAliasing.IsModified() )
	{
		switch( r_antiAliasing.GetInteger() )
		{
			case ANTI_ALIASING_MSAA_2X:
			case ANTI_ALIASING_MSAA_4X:
			case ANTI_ALIASING_MSAA_8X:
				if( r_antiAliasing.GetInteger() > 0 )
				{
					glEnable( GL_MULTISAMPLE );
				}
				break;
				
			default:
				glDisable( GL_MULTISAMPLE );
				break;
		}
	}
	
	if( r_useHDR.IsModified() || r_useHalfLambertLighting.IsModified() )
	{
		r_useHDR.ClearModified();
		r_useHalfLambertLighting.ClearModified();
		renderProgManager.KillAllShaders();
		renderProgManager.LoadAllShaders();
	}
	
	// RB: turn off shadow mapping for OpenGL drivers that are too slow
	switch( glConfig.driverType )
	{
		case GLDRV_OPENGL_ES2:
		case GLDRV_OPENGL_ES3:
			//case GLDRV_OPENGL_MESA:
			r_useShadowMapping.SetInteger( 0 );
			break;
			
		default:
			break;
	}
	// RB end
}


/*
==============================================================================================

STENCIL SHADOW RENDERING

==============================================================================================
*/
extern idCVar r_useStencilShadowPreload;

/*
==================
idRenderBackend::DrawStencilShadowPass
==================
*/
void idRenderBackend::DrawStencilShadowPass( const drawSurf_t* drawSurf, const bool renderZPass )
{
	// get vertex buffer
	const vertCacheHandle_t vbHandle = drawSurf->shadowCache;
	idVertexBuffer* vertexBuffer;
	if( vertexCache.CacheIsStatic( vbHandle ) )
	{
		vertexBuffer = &vertexCache.staticData.vertexBuffer;
	}
	else
	{
		const uint64 frameNum = ( int )( vbHandle >> VERTCACHE_FRAME_SHIFT ) & VERTCACHE_FRAME_MASK;
		if( frameNum != ( ( vertexCache.currentFrame - 1 ) & VERTCACHE_FRAME_MASK ) )
		{
			idLib::Warning( "DrawStencilShadowPass, vertexBuffer == NULL" );
			return;
		}
		vertexBuffer = &vertexCache.frameData[vertexCache.drawListNum].vertexBuffer;
	}
	const int vertOffset = ( int )( vbHandle >> VERTCACHE_OFFSET_SHIFT ) & VERTCACHE_OFFSET_MASK;
	
	// get index buffer
	const vertCacheHandle_t ibHandle = drawSurf->indexCache;
	idIndexBuffer* indexBuffer;
	if( vertexCache.CacheIsStatic( ibHandle ) )
	{
		indexBuffer = &vertexCache.staticData.indexBuffer;
	}
	else
	{
		const uint64 frameNum = ( int )( ibHandle >> VERTCACHE_FRAME_SHIFT ) & VERTCACHE_FRAME_MASK;
		if( frameNum != ( ( vertexCache.currentFrame - 1 ) & VERTCACHE_FRAME_MASK ) )
		{
			idLib::Warning( "DrawStencilShadowPass, indexBuffer == NULL" );
			return;
		}
		indexBuffer = &vertexCache.frameData[vertexCache.drawListNum].indexBuffer;
	}
	const uint64 indexOffset = ( int )( ibHandle >> VERTCACHE_OFFSET_SHIFT ) & VERTCACHE_OFFSET_MASK;
	
	RENDERLOG_PRINTF( "Binding Buffers: %p %p\n", vertexBuffer, indexBuffer );
	
	// RB: 64 bit fixes, changed GLuint to GLintptr
	if( currentIndexBuffer != ( GLintptr )indexBuffer->GetAPIObject() || !r_useStateCaching.GetBool() )
	{
		glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ( GLintptr )indexBuffer->GetAPIObject() );
		currentIndexBuffer = ( GLintptr )indexBuffer->GetAPIObject();
	}
	
	if( drawSurf->jointCache )
	{
		assert( renderProgManager.ShaderUsesJoints() );
		
		idUniformBuffer jointBuffer;
		if( !vertexCache.GetJointBuffer( drawSurf->jointCache, &jointBuffer ) )
		{
			idLib::Warning( "DrawStencilShadowPass, jointBuffer == NULL" );
			return;
		}
		assert( ( jointBuffer.GetOffset() & ( glConfig.uniformBufferOffsetAlignment - 1 ) ) == 0 );
		
		const GLintptr ubo = jointBuffer.GetAPIObject();
		glBindBufferRange( GL_UNIFORM_BUFFER, 0, ubo, jointBuffer.GetOffset(), jointBuffer.GetSize() );
		
		if( ( vertexLayout != LAYOUT_DRAW_SHADOW_VERT_SKINNED ) || ( currentVertexBuffer != ( GLintptr )vertexBuffer->GetAPIObject() ) || !r_useStateCaching.GetBool() )
		{
			glBindBuffer( GL_ARRAY_BUFFER, ( GLintptr )vertexBuffer->GetAPIObject() );
			currentVertexBuffer = ( GLintptr )vertexBuffer->GetAPIObject();
			
			glEnableVertexAttribArray( PC_ATTRIB_INDEX_VERTEX );
			glDisableVertexAttribArray( PC_ATTRIB_INDEX_NORMAL );
			glEnableVertexAttribArray( PC_ATTRIB_INDEX_COLOR );
			glEnableVertexAttribArray( PC_ATTRIB_INDEX_COLOR2 );
			glDisableVertexAttribArray( PC_ATTRIB_INDEX_ST );
			glDisableVertexAttribArray( PC_ATTRIB_INDEX_TANGENT );
			
#if defined(USE_GLES2) || defined(USE_GLES3)
			glVertexAttribPointer( PC_ATTRIB_INDEX_VERTEX, 4, GL_FLOAT, GL_FALSE, sizeof( idShadowVertSkinned ), ( void* )( vertOffset + SHADOWVERTSKINNED_XYZW_OFFSET ) );
			glVertexAttribPointer( PC_ATTRIB_INDEX_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( idShadowVertSkinned ), ( void* )( vertOffset + SHADOWVERTSKINNED_COLOR_OFFSET ) );
			glVertexAttribPointer( PC_ATTRIB_INDEX_COLOR2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( idShadowVertSkinned ), ( void* )( vertOffset + SHADOWVERTSKINNED_COLOR2_OFFSET ) );
#else
			glVertexAttribPointer( PC_ATTRIB_INDEX_VERTEX, 4, GL_FLOAT, GL_FALSE, sizeof( idShadowVertSkinned ), ( void* )( SHADOWVERTSKINNED_XYZW_OFFSET ) );
			glVertexAttribPointer( PC_ATTRIB_INDEX_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( idShadowVertSkinned ), ( void* )( SHADOWVERTSKINNED_COLOR_OFFSET ) );
			glVertexAttribPointer( PC_ATTRIB_INDEX_COLOR2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( idShadowVertSkinned ), ( void* )( SHADOWVERTSKINNED_COLOR2_OFFSET ) );
#endif
			
			vertexLayout = LAYOUT_DRAW_SHADOW_VERT_SKINNED;
		}
		
	}
	else
	{
		if( ( vertexLayout != LAYOUT_DRAW_SHADOW_VERT ) || ( currentVertexBuffer != ( GLintptr )vertexBuffer->GetAPIObject() ) || !r_useStateCaching.GetBool() )
		{
			glBindBuffer( GL_ARRAY_BUFFER, ( GLintptr )vertexBuffer->GetAPIObject() );
			currentVertexBuffer = ( GLintptr )vertexBuffer->GetAPIObject();
			
			glEnableVertexAttribArray( PC_ATTRIB_INDEX_VERTEX );
			glDisableVertexAttribArray( PC_ATTRIB_INDEX_NORMAL );
			glDisableVertexAttribArray( PC_ATTRIB_INDEX_COLOR );
			glDisableVertexAttribArray( PC_ATTRIB_INDEX_COLOR2 );
			glDisableVertexAttribArray( PC_ATTRIB_INDEX_ST );
			glDisableVertexAttribArray( PC_ATTRIB_INDEX_TANGENT );
			
#if defined(USE_GLES2) || defined(USE_GLES3)
			glVertexAttribPointer( PC_ATTRIB_INDEX_VERTEX, 4, GL_FLOAT, GL_FALSE, sizeof( idShadowVert ), ( void* )( vertOffset + SHADOWVERT_XYZW_OFFSET ) );
#else
			glVertexAttribPointer( PC_ATTRIB_INDEX_VERTEX, 4, GL_FLOAT, GL_FALSE, sizeof( idShadowVert ), ( void* )( SHADOWVERT_XYZW_OFFSET ) );
#endif
			
			vertexLayout = LAYOUT_DRAW_SHADOW_VERT;
		}
	}
	// RB end
	
	renderProgManager.CommitUniforms();
	
	if( drawSurf->jointCache )
	{
#if defined(USE_GLES3) //defined(USE_GLES2)
		glDrawElements( GL_TRIANGLES, r_singleTriangle.GetBool() ? 3 : drawSurf->numIndexes, GL_INDEX_TYPE, ( triIndex_t* )indexOffset );
#else
		glDrawElementsBaseVertex( GL_TRIANGLES, r_singleTriangle.GetBool() ? 3 : drawSurf->numIndexes, GL_INDEX_TYPE, ( triIndex_t* )indexOffset, vertOffset / sizeof( idShadowVertSkinned ) );
#endif
	}
	else
	{
#if defined(USE_GLES3)
		glDrawElements( GL_TRIANGLES, r_singleTriangle.GetBool() ? 3 : drawSurf->numIndexes, GL_INDEX_TYPE, ( triIndex_t* )indexOffset );
#else
		glDrawElementsBaseVertex( GL_TRIANGLES, r_singleTriangle.GetBool() ? 3 : drawSurf->numIndexes, GL_INDEX_TYPE, ( triIndex_t* )indexOffset, vertOffset / sizeof( idShadowVert ) );
#endif
	}
	
	// RB: added stats
	pc.c_shadowElements++;
	pc.c_shadowIndexes += drawSurf->numIndexes;
	// RB end
	
	if( !renderZPass && r_useStencilShadowPreload.GetBool() )
	{
		// render again with Z-pass
		glStencilOpSeparate( GL_FRONT, GL_KEEP, GL_KEEP, GL_INCR );
		glStencilOpSeparate( GL_BACK, GL_KEEP, GL_KEEP, GL_DECR );
		
		if( drawSurf->jointCache )
		{
#if defined(USE_GLES3)
			glDrawElements( GL_TRIANGLES, r_singleTriangle.GetBool() ? 3 : drawSurf->numIndexes, GL_INDEX_TYPE, ( triIndex_t* )indexOffset );
#else
			glDrawElementsBaseVertex( GL_TRIANGLES, r_singleTriangle.GetBool() ? 3 : drawSurf->numIndexes, GL_INDEX_TYPE, ( triIndex_t* )indexOffset, vertOffset / sizeof( idShadowVertSkinned ) );
#endif
		}
		else
		{
#if defined(USE_GLES3)
			glDrawElements( GL_TRIANGLES, r_singleTriangle.GetBool() ? 3 : drawSurf->numIndexes, GL_INDEX_TYPE, ( triIndex_t* )indexOffset );
#else
			glDrawElementsBaseVertex( GL_TRIANGLES, r_singleTriangle.GetBool() ? 3 : drawSurf->numIndexes, GL_INDEX_TYPE, ( triIndex_t* )indexOffset, vertOffset / sizeof( idShadowVert ) );
#endif
		}
		
		// RB: added stats
		pc.c_shadowElements++;
		pc.c_shadowIndexes += drawSurf->numIndexes;
		// RB end
	}
}

/*
============================================================================

RENDER BACK END THREAD FUNCTIONS

============================================================================
*/

/*
=============
idRenderBackend::DrawFlickerBox
=============
*/
void idRenderBackend::DrawFlickerBox()
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
idRenderBackend::SetBuffer
=============
*/
void idRenderBackend::SetBuffer( const void* data )
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
			GL_Clear( true, false, false, 0, c[0], c[1], c[2], 1.0f, true );
		}
		else if( r_clear.GetInteger() == 2 )
		{
			GL_Clear( true, false, false, 0, 0.0f, 0.0f,  0.0f, 1.0f, true );
		}
		else if( r_showOverDraw.GetBool() )
		{
			GL_Clear( true, false, false, 0, 1.0f, 1.0f, 1.0f, 1.0f, true );
		}
		else
		{
			GL_Clear( true, false, false, 0, 0.4f, 0.0f, 0.25f, 1.0f, true );
		}
	}
}

/*
=============
GL_BlockingSwapBuffers

We want to exit this with the GPU idle, right at vsync
=============
*/
void idRenderBackend::BlockingSwapBuffers()
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
=============
idRenderBackend::idRenderBackend
=============
*/
idRenderBackend::idRenderBackend()
{
	Init();
}

/*
=============
idRenderBackend::~idRenderBackend
=============
*/
idRenderBackend::~idRenderBackend()
{

}

/*
=============
idRenderBackend::Init
=============
*/
void idRenderBackend::Init()
{
	memset( glcontext.tmu, 0, sizeof( glcontext.tmu ) );
	memset( glcontext.stencilOperations, 0, sizeof( glcontext.stencilOperations ) );
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
idRenderBackend::StereoRenderExecuteBackEndCommands

Renders the draw list twice, with slight modifications for left eye / right eye
====================
*/
void idRenderBackend::StereoRenderExecuteBackEndCommands( const emptyCommand_t* const allCmds )
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
					DrawView( dsc, stereoEye );
					if( cmds->commandId == RC_DRAW_VIEW_GUI )
					{
					}
				}
				break;
				case RC_SET_BUFFER:
					SetBuffer( cmds );
					break;
				case RC_COPY_RENDER:
					CopyRender( cmds );
					break;
				case RC_POST_PROCESS:
				{
					postProcessCommand_t* cmd = ( postProcessCommand_t* )cmds;
					if( cmd->viewDef->renderView.viewEyeBuffer != stereoEye )
					{
						break;
					}
					PostProcess( cmds );
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
			DrawElementsWithCounters( &unitSquareSurface );
			
			glDrawBuffer( GL_BACK_LEFT );
			GL_SelectTexture( 1 );
			stereoRenderImages[1]->Bind();
			GL_SelectTexture( 0 );
			stereoRenderImages[0]->Bind();
			DrawElementsWithCounters( &unitSquareSurface );
			break;
			
		case STEREO3D_HDMI_720:
			// HDMI 720P 3D
			GL_SelectTexture( 0 );
			stereoRenderImages[1]->Bind();
			GL_SelectTexture( 1 );
			stereoRenderImages[0]->Bind();
			GL_ViewportAndScissor( 0, 0, 1280, 720 );
			DrawElementsWithCounters( &unitSquareSurface );
			
			GL_SelectTexture( 0 );
			stereoRenderImages[0]->Bind();
			GL_SelectTexture( 1 );
			stereoRenderImages[1]->Bind();
			GL_ViewportAndScissor( 0, 750, 1280, 720 );
			DrawElementsWithCounters( &unitSquareSurface );
			
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
				DrawElementsWithCounters( &unitSquareSurface );
				
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
				DrawElementsWithCounters( &unitSquareSurface );
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
			DrawElementsWithCounters( &unitSquareSurface );
			
			GL_SelectTexture( 0 );
			stereoRenderImages[1]->Bind();
			GL_SelectTexture( 1 );
			stereoRenderImages[0]->Bind();
			GL_ViewportAndScissor( renderSystem->GetWidth(), 0, renderSystem->GetWidth(), renderSystem->GetHeight() );
			DrawElementsWithCounters( &unitSquareSurface );
			break;
			
		case STEREO3D_TOP_AND_BOTTOM_COMPRESSED:
			GL_SelectTexture( 1 );
			stereoRenderImages[0]->Bind();
			GL_SelectTexture( 0 );
			stereoRenderImages[1]->Bind();
			GL_ViewportAndScissor( 0, 0, renderSystem->GetWidth(), renderSystem->GetHeight() );
			DrawElementsWithCounters( &unitSquareSurface );
			
			GL_SelectTexture( 1 );
			stereoRenderImages[1]->Bind();
			GL_SelectTexture( 0 );
			stereoRenderImages[0]->Bind();
			GL_ViewportAndScissor( 0, renderSystem->GetHeight(), renderSystem->GetWidth(), renderSystem->GetHeight() );
			DrawElementsWithCounters( &unitSquareSurface );
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
			DrawElementsWithCounters( &unitSquareSurface );
			
			GL_SelectTexture( 0 );
			glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
			glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
			
			GL_SelectTexture( 1 );
			glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
			glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
			
			break;
	}
	
	// debug tool
	DrawFlickerBox();
	
	// make sure the drawing is actually started
	glFlush();
	
	// we may choose to sync to the swapbuffers before the next frame
	
	// stop rendering on this thread
	uint64 backEndFinishTime = Sys_Microseconds();
	pc.totalMicroSec = backEndFinishTime - backEndStartTime;
}

/*
====================
RB_ExecuteBackEndCommands

This function will be called syncronously if running without
smp extensions, or asyncronously by another thread.
====================
*/
void idRenderBackend::ExecuteBackEndCommands( const emptyCommand_t* cmds )
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
		StereoRenderExecuteBackEndCommands( cmds );
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
				DrawView( cmds, 0 );
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
				//RB_SetBuffer( cmds );
				c_setBuffers++;
				break;
			case RC_COPY_RENDER:
				CopyRender( cmds );
				c_copyRenders++;
				break;
			case RC_POST_PROCESS:
				PostProcess( cmds );
				break;
			default:
				common->Error( "RB_ExecuteBackEndCommands: bad commandId" );
				break;
		}
	}
	
	DrawFlickerBox();
	
	// Fix for the steam overlay not showing up while in game without Shell/Debug/Console/Menu also rendering
	glColorMask( 1, 1, 1, 1 );
	
	glFlush();
	
	// stop rendering on this thread
	uint64 backEndFinishTime = Sys_Microseconds();
	pc.totalMicroSec = backEndFinishTime - backEndStartTime;
	
	if( r_debugRenderToTexture.GetInteger() == 1 )
	{
		common->Printf( "3d: %i, 2d: %i, SetBuf: %i, CpyRenders: %i, CpyFrameBuf: %i\n", c_draw3d, c_draw2d, c_setBuffers, c_copyRenders, pc.c_copyFrameBuffer );
		pc.c_copyFrameBuffer = 0;
	}
	
	renderLog.EndFrame();
}
