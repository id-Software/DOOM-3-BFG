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

#include "../RenderCommon.h"



/*
====================
GL_Cull

This handles the flipping needed when the view being
rendered is a mirored view.
====================
*/
void GL_Cull( int cullType )
{
	if( tr.backend.faceCulling == cullType )
	{
		return;
	}
	
	if( cullType == CT_TWO_SIDED )
	{
		glDisable( GL_CULL_FACE );
	}
	else
	{
		if( tr.backend.faceCulling == CT_TWO_SIDED )
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
	
	tr.backend.faceCulling = cullType;
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
	tr.backend.polyOfsScale = scale;
	tr.backend.polyOfsBias = bias;
	if( tr.backend.glStateBits & GLS_POLYGON_OFFSET )
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
=================
GL_GetCurrentState
=================
*/
uint64 GL_GetCurrentState()
{
	return tr.backend.glStateBits;
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
