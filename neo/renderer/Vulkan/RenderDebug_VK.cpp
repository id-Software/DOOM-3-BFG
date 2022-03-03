/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2014-2016 Robert Beckebans
Copyright (C) 2014-2016 Kot in Action Creative Artel
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





/*
================
idRenderBackend::DBG_SimpleSurfaceSetup
================
*/
void idRenderBackend::DBG_SimpleSurfaceSetup( const drawSurf_t* drawSurf )
{

}

/*
================
idRenderBackend::DBG_SimpleWorldSetup
================
*/
void idRenderBackend::DBG_SimpleWorldSetup()
{

}

/*
=================
idRenderBackend::DBG_PolygonClear

This will cover the entire screen with normal rasterization.
Texturing is disabled, but the existing glColor, glDepthMask,
glColorMask, and the enabled state of depth buffering and
stenciling will matter.
=================
*/
void idRenderBackend::DBG_PolygonClear()
{

}

/*
====================
idRenderBackend::DBG_ShowDestinationAlpha
====================
*/
void idRenderBackend::DBG_ShowDestinationAlpha()
{

}

/*
===================
idRenderBackend::DBG_ScanStencilBuffer

Debugging tool to see what values are in the stencil buffer
===================
*/
void idRenderBackend::DBG_ScanStencilBuffer()
{

}


/*
===================
idRenderBackend::DBG_CountStencilBuffer

Print an overdraw count based on stencil index values
===================
*/
void idRenderBackend::DBG_CountStencilBuffer()
{

}

/*
===================
idRenderBackend::DBG_ColorByStencilBuffer

Sets the screen colors based on the contents of the
stencil buffer.  Stencil of 0 = black, 1 = red, 2 = green,
3 = blue, ..., 7+ = white
===================
*/
void idRenderBackend::DBG_ColorByStencilBuffer()
{

}

/*
==================
idRenderBackend::DBG_ShowOverdraw
==================
*/
void idRenderBackend::DBG_ShowOverdraw()
{

}

/*
===================
idRenderBackend::DBG_ShowIntensity

Debugging tool to see how much dynamic range a scene is using.
The greatest of the rgb values at each pixel will be used, with
the resulting color shading from red at 0 to green at 128 to blue at 255
===================
*/
void idRenderBackend::DBG_ShowIntensity()
{

}


/*
===================
idRenderBackend::DBG_ShowDepthBuffer

Draw the depth buffer as colors
===================
*/
void idRenderBackend::DBG_ShowDepthBuffer()
{

}

/*
=================
idRenderBackend::DBG_ShowLightCount

This is a debugging tool that will draw each surface with a color
based on how many lights are effecting it
=================
*/
void idRenderBackend::DBG_ShowLightCount()
{

}

/*
====================
idRenderBackend::DBG_RenderDrawSurfListWithFunction

The triangle functions can check backEnd.currentSpace != surf->space
to see if they need to perform any new matrix setup.  The modelview
matrix will already have been loaded, and backEnd.currentSpace will
be updated after the triangle function completes.
====================
*/
void idRenderBackend::DBG_RenderDrawSurfListWithFunction( drawSurf_t** drawSurfs, int numDrawSurfs )
{

}

/*
=================
idRenderBackend::DBG_ShowSilhouette

Blacks out all edges, then adds color for each edge that a shadow
plane extends from, allowing you to see doubled edges

FIXME: not thread safe!
=================
*/
void idRenderBackend::DBG_ShowSilhouette()
{

}

/*
=====================
idRenderBackend::DBG_ShowTris

Debugging tool
=====================
*/
void idRenderBackend::DBG_ShowTris( drawSurf_t** drawSurfs, int numDrawSurfs )
{


}

/*
=====================
idRenderBackend::DBG_ShowSurfaceInfo

Debugging tool
=====================
*/
void idRenderSystemLocal::OnFrame()
{

}


void idRenderBackend::DBG_ShowSurfaceInfo( drawSurf_t** drawSurfs, int numDrawSurfs )
{

}

/*
=====================
idRenderBackend::DBG_ShowViewEntitys

Debugging tool
=====================
*/
void idRenderBackend::DBG_ShowViewEntitys( viewEntity_t* vModels )
{

}

/*
=====================
idRenderBackend::DBG_ShowTexturePolarity

Shade triangle red if they have a positive texture area
green if they have a negative texture area, or blue if degenerate area
=====================
*/
void idRenderBackend::DBG_ShowTexturePolarity( drawSurf_t** drawSurfs, int numDrawSurfs )
{

}

/*
=====================
idRenderBackend::DBG_ShowUnsmoothedTangents

Shade materials that are using unsmoothed tangents
=====================
*/
void idRenderBackend::DBG_ShowUnsmoothedTangents( drawSurf_t** drawSurfs, int numDrawSurfs )
{

}

/*
=====================
RB_ShowTangentSpace

Shade a triangle by the RGB colors of its tangent space
1 = tangents[0]
2 = tangents[1]
3 = normal
=====================
*/
void idRenderBackend::DBG_ShowTangentSpace( drawSurf_t** drawSurfs, int numDrawSurfs )
{

}

/*
=====================
idRenderBackend::DBG_ShowVertexColor

Draw each triangle with the solid vertex colors
=====================
*/
void idRenderBackend::DBG_ShowVertexColor( drawSurf_t** drawSurfs, int numDrawSurfs )
{

}

/*
=====================
idRenderBackend::DBG_ShowNormals

Debugging tool
=====================
*/
void idRenderBackend::DBG_ShowNormals( drawSurf_t** drawSurfs, int numDrawSurfs )
{

}

/*
=====================
idRenderBackend::DBG_ShowTextureVectors

Draw texture vectors in the center of each triangle
=====================
*/
void idRenderBackend::DBG_ShowTextureVectors( drawSurf_t** drawSurfs, int numDrawSurfs )
{

}

/*
=====================
idRenderBackend::DBG_ShowDominantTris

Draw lines from each vertex to the dominant triangle center
=====================
*/
void idRenderBackend::DBG_ShowDominantTris( drawSurf_t** drawSurfs, int numDrawSurfs )
{

}

/*
=====================
idRenderBackend::DBG_ShowEdges

Debugging tool
=====================
*/
void idRenderBackend::DBG_ShowEdges( drawSurf_t** drawSurfs, int numDrawSurfs )
{

}

/*
==============
RB_ShowLights

Visualize all light volumes used in the current scene
r_showLights 1	: just print volumes numbers, highlighting ones covering the view
r_showLights 2	: also draw planes of each volume
r_showLights 3	: also draw edges of each volume
==============
*/
void idRenderBackend::DBG_ShowLights()
{

}

// RB begin
void idRenderBackend::DBG_ShowShadowMapLODs()
{

}
// RB end

/*
=====================
idRenderBackend::DBG_ShowPortals

Debugging tool, won't work correctly with SMP or when mirrors are present
=====================
*/
void idRenderBackend::DBG_ShowPortals()
{

}

/*
================
idRenderBackend::DBG_ClearDebugText
================
*/
void RB_ClearDebugText( int time )
{

}

/*
================
RB_AddDebugText
================
*/
void RB_AddDebugText( const char* text, const idVec3& origin, float scale, const idVec4& color, const idMat3& viewAxis, const int align, const int lifetime, const bool depthTest )
{

}

/*
================
RB_DrawTextLength

  returns the length of the given text
================
*/
float RB_DrawTextLength( const char* text, float scale, int len )
{
	return 0;
}

/*
================
RB_DrawText

  oriented on the viewaxis
  align can be 0-left, 1-center (default), 2-right
================
*/
void RB_DrawText( const char* text, const idVec3& origin, float scale, const idVec4& color, const idMat3& viewAxis, const int align )
{

}

/*
================
idRenderBackend::DBG_ShowDebugText
================
*/
void idRenderBackend::DBG_ShowDebugText()
{

}

/*
================
RB_ClearDebugLines
================
*/
void RB_ClearDebugLines( int time )
{

}

/*
================
RB_AddDebugLine
================
*/
void RB_AddDebugLine( const idVec4& color, const idVec3& start, const idVec3& end, const int lifeTime, const bool depthTest )
{

}

/*
================
idRenderBackend::DBG_ShowDebugLines
================
*/
void idRenderBackend::DBG_ShowDebugLines()
{

}

/*
================
RB_ClearDebugPolygons
================
*/
void RB_ClearDebugPolygons( int time )
{

}

/*
================
RB_AddDebugPolygon
================
*/
void RB_AddDebugPolygon( const idVec4& color, const idWinding& winding, const int lifeTime, const bool depthTest )
{

}

/*
================
idRenderBackend::DBG_ShowDebugPolygons
================
*/
void idRenderBackend::DBG_ShowDebugPolygons()
{

}

/*
================
idRenderBackend::DBG_ShowCenterOfProjection
================
*/
void idRenderBackend::DBG_ShowCenterOfProjection()
{

}

/*
================
idRenderBackend::DBG_ShowLines

Draw exact pixel lines to check pixel center sampling
================
*/
void idRenderBackend::DBG_ShowLines()
{

}


/*
================
idRenderBackend::DBG_TestGamma
================
*/
void idRenderBackend::DBG_TestGamma()
{

}


/*
==================
idRenderBackend::DBG_TestGammaBias
==================
*/
void idRenderBackend::DBG_TestGammaBias()
{

}

/*
================
idRenderBackend::DBG_TestImage

Display a single image over most of the screen
================
*/
void idRenderBackend::DBG_TestImage()
{
	idImage*	image = NULL;
	idImage* imageCr = NULL;
	idImage* imageCb = NULL;
	int		max;
	float	w, h;

	image = tr.testImage;
	if( !image )
	{
		return;
	}

	if( tr.testVideo )
	{
		cinData_t	cin;

		// SRS - Don't need calibrated time for testing cinematics, so just call ImageForTime( 0 ) for current system time
		// This simplification allows cinematic test playback to work over both 2D and 3D background scenes
		cin = tr.testVideo->ImageForTime( 0 /*viewDef->renderView.time[1] - tr.testVideoStartTime*/ );
		if( cin.imageY != NULL )
		{
			image = cin.imageY;
			imageCr = cin.imageCr;
			imageCb = cin.imageCb;
		}
		// SRS - Also handle ffmpeg and original RoQ decoders for test videos (using cin.image)
		else if( cin.image != NULL )
		{
			image = cin.image;
		}
		else
		{
			tr.testImage = NULL;
			return;
		}
		w = 0.25;
		h = 0.25;
	}
	else
	{
		max = image->GetUploadWidth() > image->GetUploadHeight() ? image->GetUploadWidth() : image->GetUploadHeight();

		w = 0.25 * image->GetUploadWidth() / max;
		h = 0.25 * image->GetUploadHeight() / max;

		w *= ( float )renderSystem->GetHeight() / renderSystem->GetWidth();
	}

	// Set State
	GL_State( GLS_DEPTHFUNC_ALWAYS | GLS_CULL_TWOSIDED | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO );

	// Set Parms
	float texS[4] = { 1.0f, 0.0f, 0.0f, 0.0f };
	float texT[4] = { 0.0f, 1.0f, 0.0f, 0.0f };
	renderProgManager.SetRenderParm( RENDERPARM_TEXTUREMATRIX_S, texS );
	renderProgManager.SetRenderParm( RENDERPARM_TEXTUREMATRIX_T, texT );

	float texGenEnabled[4] = { 0, 0, 0, 0 };
	renderProgManager.SetRenderParm( RENDERPARM_TEXGEN_0_ENABLED, texGenEnabled );

	// not really necessary but just for clarity
	const float screenWidth = 1.0f;
	const float screenHeight = 1.0f;
	const float halfScreenWidth = screenWidth * 0.5f;
	const float halfScreenHeight = screenHeight * 0.5f;

	float scale[16] = { 0 };
	scale[0] = w; // scale
	scale[5] = h; // scale
	scale[12] = halfScreenWidth - ( halfScreenWidth * w ); // translate
	scale[13] = halfScreenHeight - ( halfScreenHeight * h ) - h; // translate (SRS - moved up by h)
	scale[10] = 1.0f;
	scale[15] = 1.0f;

	// RB: orthographic projection is changed for Vulkan
	float ortho[16] = { 0 };
	ortho[0] = 2.0f / screenWidth;
	ortho[5] = 2.0f / screenHeight;
	ortho[10] = -1.0f;
	ortho[12] = -1.0f;
	ortho[13] = -1.0f;
	ortho[14] = 0.0f;
	ortho[15] = 1.0f;

	float finalOrtho[16];
	R_MatrixMultiply( scale, ortho, finalOrtho );

	float projMatrixTranspose[16];
	R_MatrixTranspose( finalOrtho, projMatrixTranspose );
	renderProgManager.SetRenderParms( RENDERPARM_MVPMATRIX_X, projMatrixTranspose, 4 );

	// Set Color
	GL_Color( 1, 1, 1, 1 );

	// Bind the Texture
	if( ( imageCr != NULL ) && ( imageCb != NULL ) )
	{
		GL_SelectTexture( 0 );
		image->Bind();

		GL_SelectTexture( 1 );
		imageCr->Bind();

		GL_SelectTexture( 2 );
		imageCb->Bind();

		// SRS - Use Bink shader with no sRGB to linear conversion, otherwise cinematic colours may be wrong
		// BindShader_BinkGUI() does not seem to work here - perhaps due to vertex shader input dependencies?
		renderProgManager.BindShader_Bink_sRGB();
	}
	else
	{
		GL_SelectTexture( 0 );
		image->Bind();

		renderProgManager.BindShader_Texture();
	}

	// Draw!
	DrawElementsWithCounters( &testImageSurface );
}

// RB begin
void idRenderBackend::DBG_ShowShadowMaps()
{

}
// RB end

/*
=================
RB_DrawExpandedTriangles
=================
*/
static void RB_DrawExpandedTriangles( const srfTriangles_t* tri, const float radius, const idVec3& vieworg )
{

}

/*
================
idRenderBackend::DBG_ShowTrace

Debug visualization

FIXME: not thread safe!
================
*/
void idRenderBackend::DBG_ShowTrace( drawSurf_t** drawSurfs, int numDrawSurfs )
{

}

/*
=================
idRenderBackend::DBG_RenderDebugTools
=================
*/
void idRenderBackend::DBG_RenderDebugTools( drawSurf_t** drawSurfs, int numDrawSurfs )
{
	// don't do much if this was a 2D rendering
	if( !viewDef->viewEntitys )
	{
		DBG_TestImage();
		DBG_ShowLines();
		return;
	}

	// TODO
}

/*
=================
RB_ShutdownDebugTools
=================
*/
void RB_ShutdownDebugTools()
{

}
