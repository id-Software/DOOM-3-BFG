/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2014-2021 Robert Beckebans
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

#include "precompiled.h"
#pragma hdrstop

#include "../RenderCommon.h"
#include "../simplex.h"	// line font definition

idCVar r_showCenterOfProjection( "r_showCenterOfProjection", "0", CVAR_RENDERER | CVAR_BOOL, "Draw a cross to show the center of projection" );
idCVar r_showLines( "r_showLines", "0", CVAR_RENDERER | CVAR_INTEGER, "1 = draw alternate horizontal lines, 2 = draw alternate vertical lines" );



debugLine_t		rb_debugLines[ MAX_DEBUG_LINES ];
int				rb_numDebugLines = 0;
int				rb_debugLineTime = 0;

debugText_t		rb_debugText[ MAX_DEBUG_TEXT ];
int				rb_numDebugText = 0;
int				rb_debugTextTime = 0;

debugPolygon_t	rb_debugPolygons[ MAX_DEBUG_POLYGONS ];
int				rb_numDebugPolygons = 0;
int				rb_debugPolygonTime = 0;

static void RB_DrawText( const char* text, const idVec3& origin, float scale, const idVec4& color, const idMat3& viewAxis, const int align );

void RB_SetMVP( const idRenderMatrix& mvp );

/*
================
RB_DrawBounds
================
*/
void RB_DrawBounds( const idBounds& bounds )
{
	if( bounds.IsCleared() )
	{
		return;
	}
	glBegin( GL_LINE_LOOP );
	glVertex3f( bounds[0][0], bounds[0][1], bounds[0][2] );
	glVertex3f( bounds[0][0], bounds[1][1], bounds[0][2] );
	glVertex3f( bounds[1][0], bounds[1][1], bounds[0][2] );
	glVertex3f( bounds[1][0], bounds[0][1], bounds[0][2] );
	glEnd();
	glBegin( GL_LINE_LOOP );
	glVertex3f( bounds[0][0], bounds[0][1], bounds[1][2] );
	glVertex3f( bounds[0][0], bounds[1][1], bounds[1][2] );
	glVertex3f( bounds[1][0], bounds[1][1], bounds[1][2] );
	glVertex3f( bounds[1][0], bounds[0][1], bounds[1][2] );
	glEnd();

	glBegin( GL_LINES );
	glVertex3f( bounds[0][0], bounds[0][1], bounds[0][2] );
	glVertex3f( bounds[0][0], bounds[0][1], bounds[1][2] );

	glVertex3f( bounds[0][0], bounds[1][1], bounds[0][2] );
	glVertex3f( bounds[0][0], bounds[1][1], bounds[1][2] );

	glVertex3f( bounds[1][0], bounds[0][1], bounds[0][2] );
	glVertex3f( bounds[1][0], bounds[0][1], bounds[1][2] );

	glVertex3f( bounds[1][0], bounds[1][1], bounds[0][2] );
	glVertex3f( bounds[1][0], bounds[1][1], bounds[1][2] );
	glEnd();
}


/*
================
idRenderBackend::DBG_SimpleSurfaceSetup
================
*/
void idRenderBackend::DBG_SimpleSurfaceSetup( const drawSurf_t* drawSurf )
{
	// change the matrix if needed
	if( drawSurf->space != currentSpace )
	{
		// RB begin
		RB_SetMVP( drawSurf->space->mvp );
		//qglLoadMatrixf( drawSurf->space->modelViewMatrix );
		// RB end
		currentSpace = drawSurf->space;
	}

	// change the scissor if needed
	if( !currentScissor.Equals( drawSurf->scissorRect ) && r_useScissor.GetBool() )
	{
		GL_Scissor( viewDef->viewport.x1 + drawSurf->scissorRect.x1,
					viewDef->viewport.y1 + drawSurf->scissorRect.y1,
					drawSurf->scissorRect.x2 + 1 - drawSurf->scissorRect.x1,
					drawSurf->scissorRect.y2 + 1 - drawSurf->scissorRect.y1 );

		currentScissor = drawSurf->scissorRect;
	}
}

/*
================
idRenderBackend::DBG_SimpleWorldSetup
================
*/
void idRenderBackend::DBG_SimpleWorldSetup()
{
	currentSpace = &viewDef->worldSpace;

	// RB begin
	RB_SetMVP( viewDef->worldSpace.mvp );
	// RB end

	GL_Scissor( viewDef->viewport.x1 + viewDef->scissor.x1,
				viewDef->viewport.y1 + viewDef->scissor.y1,
				viewDef->scissor.x2 + 1 - viewDef->scissor.x1,
				viewDef->scissor.y2 + 1 - viewDef->scissor.y1 );

	currentScissor = viewDef->scissor;
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
	glPushMatrix();
	glPushAttrib( GL_ALL_ATTRIB_BITS );
	glLoadIdentity();
	glDisable( GL_TEXTURE_2D );
	glDisable( GL_DEPTH_TEST );
	glDisable( GL_CULL_FACE );
	glDisable( GL_SCISSOR_TEST );
	glBegin( GL_POLYGON );
	glVertex3f( -20, -20, -10 );
	glVertex3f( 20, -20, -10 );
	glVertex3f( 20, 20, -10 );
	glVertex3f( -20, 20, -10 );
	glEnd();
	glPopAttrib();
	glPopMatrix();
}

/*
====================
idRenderBackend::DBG_ShowDestinationAlpha
====================
*/
void idRenderBackend::DBG_ShowDestinationAlpha()
{
	GL_State( GLS_SRCBLEND_DST_ALPHA | GLS_DSTBLEND_ZERO | GLS_DEPTHMASK | GLS_DEPTHFUNC_ALWAYS );
	GL_Color( 1, 1, 1 );

	DBG_PolygonClear();
}

/*
===================
idRenderBackend::DBG_ScanStencilBuffer

Debugging tool to see what values are in the stencil buffer
===================
*/
void idRenderBackend::DBG_ScanStencilBuffer()
{
	int		counts[256];
	int		i;
	byte*	stencilReadback;

	memset( counts, 0, sizeof( counts ) );

	stencilReadback = ( byte* )R_StaticAlloc( renderSystem->GetWidth() * renderSystem->GetHeight(), TAG_RENDER_TOOLS );
	glReadPixels( 0, 0, renderSystem->GetWidth(), renderSystem->GetHeight(), GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, stencilReadback );

	for( i = 0; i < renderSystem->GetWidth() * renderSystem->GetHeight(); i++ )
	{
		counts[ stencilReadback[i] ]++;
	}

	R_StaticFree( stencilReadback );

	// print some stats (not supposed to do from back end in SMP...)
	common->Printf( "stencil values:\n" );
	for( i = 0; i < 255; i++ )
	{
		if( counts[i] )
		{
			common->Printf( "%i: %i\n", i, counts[i] );
		}
	}
}


/*
===================
idRenderBackend::DBG_CountStencilBuffer

Print an overdraw count based on stencil index values
===================
*/
void idRenderBackend::DBG_CountStencilBuffer()
{
	int		count;
	int		i;
	byte*	stencilReadback;


	stencilReadback = ( byte* )R_StaticAlloc( renderSystem->GetWidth() * renderSystem->GetHeight(), TAG_RENDER_TOOLS );
	glReadPixels( 0, 0, renderSystem->GetWidth(), renderSystem->GetHeight(), GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, stencilReadback );

	count = 0;
	for( i = 0; i < renderSystem->GetWidth() * renderSystem->GetHeight(); i++ )
	{
		count += stencilReadback[i];
	}

	R_StaticFree( stencilReadback );

	// print some stats (not supposed to do from back end in SMP...)
	common->Printf( "overdraw: %5.1f\n", ( float )count / ( renderSystem->GetWidth() * renderSystem->GetHeight() ) );
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
	int		i;
	static idVec3	colors[8] =
	{
		idVec3( 0, 0, 0 ),
		idVec3( 1, 0, 0 ),
		idVec3( 0, 1, 0 ),
		idVec3( 0, 0, 1 ),
		idVec3( 0, 1, 1 ),
		idVec3( 1, 0, 1 ),
		idVec3( 1, 1, 0 ),
		idVec3( 1, 1, 1 ),
	};

	// clear color buffer to white (>6 passes)
	GL_Clear( true, false, false, 0, 1.0f, 1.0f, 1.0f, 1.0f );

	// now draw color for each stencil value
	glStencilOp( GL_KEEP, GL_KEEP, GL_KEEP );
	for( i = 0; i < 6; i++ )
	{
		GL_Color( colors[i] );
		renderProgManager.BindShader_Color();
		glStencilFunc( GL_EQUAL, i, 255 );

		DBG_PolygonClear();
	}

	glStencilFunc( GL_ALWAYS, 0, 255 );
}

/*
==================
idRenderBackend::DBG_ShowOverdraw
==================
*/
void idRenderBackend::DBG_ShowOverdraw()
{
	const idMaterial* 	material;
	int					i;
	drawSurf_t** 		drawSurfs;
	const drawSurf_t* 	surf;
	int					numDrawSurfs;
	viewLight_t* 		vLight;

	if( r_showOverDraw.GetInteger() == 0 )
	{
		return;
	}

	material = declManager->FindMaterial( "textures/common/overdrawtest", false );
	if( material == NULL )
	{
		return;
	}

	drawSurfs = viewDef->drawSurfs;
	numDrawSurfs = viewDef->numDrawSurfs;

	int interactions = 0;
	for( vLight = viewDef->viewLights; vLight; vLight = vLight->next )
	{
		for( surf = vLight->localInteractions; surf; surf = surf->nextOnLight )
		{
			interactions++;
		}
		for( surf = vLight->globalInteractions; surf; surf = surf->nextOnLight )
		{
			interactions++;
		}
	}

	// FIXME: can't frame alloc from the renderer back-end
	drawSurf_t** newDrawSurfs = ( drawSurf_t** )R_FrameAlloc( numDrawSurfs + interactions * sizeof( newDrawSurfs[0] ), FRAME_ALLOC_DRAW_SURFACE_POINTER );

	for( i = 0; i < numDrawSurfs; i++ )
	{
		surf = drawSurfs[i];
		if( surf->material )
		{
			const_cast<drawSurf_t*>( surf )->material = material;
		}
		newDrawSurfs[i] = const_cast<drawSurf_t*>( surf );
	}

	for( vLight = viewDef->viewLights; vLight; vLight = vLight->next )
	{
		for( surf = vLight->localInteractions; surf; surf = surf->nextOnLight )
		{
			const_cast<drawSurf_t*>( surf )->material = material;
			newDrawSurfs[i++] = const_cast<drawSurf_t*>( surf );
		}
		for( surf = vLight->globalInteractions; surf; surf = surf->nextOnLight )
		{
			const_cast<drawSurf_t*>( surf )->material = material;
			newDrawSurfs[i++] = const_cast<drawSurf_t*>( surf );
		}
		vLight->localInteractions = NULL;
		vLight->globalInteractions = NULL;
	}

	switch( r_showOverDraw.GetInteger() )
	{
		case 1: // geometry overdraw
			const_cast<viewDef_t*>( viewDef )->drawSurfs = newDrawSurfs;
			const_cast<viewDef_t*>( viewDef )->numDrawSurfs = numDrawSurfs;
			break;
		case 2: // light interaction overdraw
			const_cast<viewDef_t*>( viewDef )->drawSurfs = &newDrawSurfs[numDrawSurfs];
			const_cast<viewDef_t*>( viewDef )->numDrawSurfs = interactions;
			break;
		case 3: // geometry + light interaction overdraw
			const_cast<viewDef_t*>( viewDef )->drawSurfs = newDrawSurfs;
			const_cast<viewDef_t*>( viewDef )->numDrawSurfs += interactions;
			break;
	}
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
	byte*	colorReadback;
	int		i, j, c;

	if( !r_showIntensity.GetBool() )
	{
		return;
	}

	colorReadback = ( byte* )R_StaticAlloc( renderSystem->GetWidth() * renderSystem->GetHeight() * 4, TAG_RENDER_TOOLS );
	glReadPixels( 0, 0, renderSystem->GetWidth(), renderSystem->GetHeight(), GL_RGBA, GL_UNSIGNED_BYTE, colorReadback );

	c = renderSystem->GetWidth() * renderSystem->GetHeight() * 4;
	for( i = 0; i < c; i += 4 )
	{
		j = colorReadback[i];
		if( colorReadback[i + 1] > j )
		{
			j = colorReadback[i + 1];
		}
		if( colorReadback[i + 2] > j )
		{
			j = colorReadback[i + 2];
		}
		if( j < 128 )
		{
			colorReadback[i + 0] = 2 * ( 128 - j );
			colorReadback[i + 1] = 2 * j;
			colorReadback[i + 2] = 0;
		}
		else
		{
			colorReadback[i + 0] = 0;
			colorReadback[i + 1] = 2 * ( 255 - j );
			colorReadback[i + 2] = 2 * ( j - 128 );
		}
	}

	// draw it back to the screen
	glLoadIdentity();
	glMatrixMode( GL_PROJECTION );
	GL_State( GLS_DEPTHFUNC_ALWAYS );
	glPushMatrix();
	glLoadIdentity();
	glOrtho( 0, 1, 0, 1, -1, 1 );
	glRasterPos2f( 0, 0 );
	glPopMatrix();
	GL_Color( 1, 1, 1 );
	glMatrixMode( GL_MODELVIEW );

	glDrawPixels( renderSystem->GetWidth(), renderSystem->GetHeight(), GL_RGBA , GL_UNSIGNED_BYTE, colorReadback );

	R_StaticFree( colorReadback );
}


/*
===================
idRenderBackend::DBG_ShowDepthBuffer

Draw the depth buffer as colors
===================
*/
void idRenderBackend::DBG_ShowDepthBuffer()
{
	void*	depthReadback;

	if( !r_showDepth.GetBool() )
	{
		return;
	}

	glPushMatrix();
	glLoadIdentity();
	glMatrixMode( GL_PROJECTION );
	glPushMatrix();
	glLoadIdentity();
	glOrtho( 0, 1, 0, 1, -1, 1 );
	glRasterPos2f( 0, 0 );
	glPopMatrix();
	glMatrixMode( GL_MODELVIEW );
	glPopMatrix();

	GL_State( GLS_DEPTHFUNC_ALWAYS );
	GL_Color( 1, 1, 1 );

	depthReadback = R_StaticAlloc( renderSystem->GetWidth() * renderSystem->GetHeight() * 4, TAG_RENDER_TOOLS );
	memset( depthReadback, 0, renderSystem->GetWidth() * renderSystem->GetHeight() * 4 );

	glReadPixels( 0, 0, renderSystem->GetWidth(), renderSystem->GetHeight(), GL_DEPTH_COMPONENT , GL_FLOAT, depthReadback );

#if 0
	for( i = 0; i < renderSystem->GetWidth() * renderSystem->GetHeight(); i++ )
	{
		( ( byte* )depthReadback )[i * 4] =
			( ( byte* )depthReadback )[i * 4 + 1] =
				( ( byte* )depthReadback )[i * 4 + 2] = 255 * ( ( float* )depthReadback )[i];
		( ( byte* )depthReadback )[i * 4 + 3] = 1;
	}
#endif

	glDrawPixels( renderSystem->GetWidth(), renderSystem->GetHeight(), GL_RGBA , GL_UNSIGNED_BYTE, depthReadback );
	R_StaticFree( depthReadback );
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
	int		i;
	const drawSurf_t*	surf;
	const viewLight_t*	vLight;

	if( !r_showLightCount.GetBool() )
	{
		return;
	}

	DBG_SimpleWorldSetup();

	GL_Clear( false, false, true, 0, 0.0f, 0.0f, 0.0f, 0.0f );

	// optionally count everything through walls
	if( r_showLightCount.GetInteger() >= 2 )
	{
		GL_State( GLS_DEPTHFUNC_EQUAL | GLS_STENCIL_OP_FAIL_KEEP | GLS_STENCIL_OP_ZFAIL_INCR | GLS_STENCIL_OP_PASS_INCR );
	}
	else
	{
		GL_State( GLS_DEPTHFUNC_EQUAL | GLS_STENCIL_OP_FAIL_KEEP | GLS_STENCIL_OP_ZFAIL_KEEP | GLS_STENCIL_OP_PASS_INCR );
	}

	globalImages->defaultImage->Bind();

	for( vLight = viewDef->viewLights; vLight; vLight = vLight->next )
	{
		for( i = 0; i < 2; i++ )
		{
			for( surf = i ? vLight->localInteractions : vLight->globalInteractions; surf; surf = ( drawSurf_t* )surf->nextOnLight )
			{
				DBG_SimpleSurfaceSetup( surf );
				DrawElementsWithCounters( surf );
			}
		}
	}

	// display the results
	DBG_ColorByStencilBuffer();

	if( r_showLightCount.GetInteger() > 2 )
	{
		DBG_CountStencilBuffer();
	}
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
	currentSpace = NULL;

	for( int i = 0 ; i < numDrawSurfs ; i++ )
	{
		const drawSurf_t* drawSurf = drawSurfs[i];
		if( drawSurf == NULL )
		{
			continue;
		}

		assert( drawSurf->space != NULL );

		// RB begin
#if 1
		if( drawSurf->space != currentSpace )
		{
			currentSpace = drawSurf->space;

			RB_SetMVP( drawSurf->space->mvp );
		}
#else

		if( drawSurf->space != NULL )  	// is it ever NULL?  Do we need to check?
		{
			// Set these values ahead of time so we don't have to reconstruct the matrices on the consoles
			if( drawSurf->space->weaponDepthHack )
			{
				RB_SetWeaponDepthHack();
			}

			if( drawSurf->space->modelDepthHack != 0.0f )
			{
				RB_SetModelDepthHack( drawSurf->space->modelDepthHack );
			}

			// change the matrix if needed
			if( drawSurf->space != backEnd.currentSpace )
			{
				RB_LoadMatrixWithBypass( drawSurf->space->modelViewMatrix );
			}

			if( drawSurf->space->weaponDepthHack )
			{
				RB_EnterWeaponDepthHack();
			}

			if( drawSurf->space->modelDepthHack != 0.0f )
			{
				RB_EnterModelDepthHack( drawSurf->space->modelDepthHack );
			}
		}
#endif

		if( drawSurf->jointCache )
		{
			renderProgManager.BindShader_ColorSkinned();
		}
		else
		{
			renderProgManager.BindShader_Color();
		}
		// RB end

		// change the scissor if needed
		if( r_useScissor.GetBool() && !currentScissor.Equals( drawSurf->scissorRect ) )
		{
			currentScissor = drawSurf->scissorRect;

			GL_Scissor( viewDef->viewport.x1 + currentScissor.x1,
						viewDef->viewport.y1 + currentScissor.y1,
						currentScissor.x2 + 1 - currentScissor.x1,
						currentScissor.y2 + 1 - currentScissor.y1 );
		}

		// render it
		DrawElementsWithCounters( drawSurf );

		// RB begin
		/*if( drawSurf->space != NULL && ( drawSurf->space->weaponDepthHack || drawSurf->space->modelDepthHack != 0.0f ) )
		{
			RB_LeaveDepthHack();
		}*/
		// RB end

		currentSpace = drawSurf->space;
	}
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
	int		i;
	const drawSurf_t*	surf;
	const viewLight_t*	vLight;

	if( !r_showSilhouette.GetBool() )
	{
		return;
	}

	// clear all triangle edges to black

	// RB
	renderProgManager.BindShader_Color();

	GL_Color( 0, 0, 0 );
	GL_State( GLS_DEPTHFUNC_ALWAYS | GLS_POLYMODE_LINE | GLS_CULL_TWOSIDED );

	DBG_RenderDrawSurfListWithFunction( viewDef->drawSurfs, viewDef->numDrawSurfs );


	// now blend in edges that cast silhouettes
	DBG_SimpleWorldSetup();

	GL_Color( 0.5, 0, 0 );
	GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );

	for( vLight = viewDef->viewLights; vLight; vLight = vLight->next )
	{
		for( i = 0; i < 2; i++ )
		{
			for( surf = i ? vLight->localShadows : vLight->globalShadows
						; surf; surf = ( drawSurf_t* )surf->nextOnLight )
			{
				DBG_SimpleSurfaceSetup( surf );

				const srfTriangles_t* tri = surf->frontEndGeo;

				idVertexBuffer vertexBuffer;
				if( !vertexCache.GetVertexBuffer( tri->shadowCache, &vertexBuffer ) )
				{
					continue;
				}

				// RB: 64 bit fixes, changed GLuint to GLintptr
				glBindBuffer( GL_ARRAY_BUFFER, ( GLintptr )vertexBuffer.GetAPIObject() );
				GLintptr vertOffset = vertexBuffer.GetOffset();
				// RB end

				glVertexPointer( 3, GL_FLOAT, sizeof( idShadowVert ), ( void* )vertOffset );
				glBegin( GL_LINES );

				for( int j = 0; j < tri->numIndexes; j += 3 )
				{
					int		i1 = tri->indexes[j + 0];
					int		i2 = tri->indexes[j + 1];
					int		i3 = tri->indexes[j + 2];

					if( ( i1 & 1 ) + ( i2 & 1 ) + ( i3 & 1 ) == 1 )
					{
						if( ( i1 & 1 ) + ( i2 & 1 ) == 0 )
						{
							glArrayElement( i1 );
							glArrayElement( i2 );
						}
						else if( ( i1 & 1 ) + ( i3 & 1 ) == 0 )
						{
							glArrayElement( i1 );
							glArrayElement( i3 );
						}
					}
				}
				glEnd();

			}
		}
	}

	GL_State( GLS_DEFAULT );
	GL_Color( 1, 1, 1 );
}

/*
=====================
idRenderBackend::DBG_ShowTris

Debugging tool
=====================
*/
void idRenderBackend::DBG_ShowTris( drawSurf_t** drawSurfs, int numDrawSurfs )
{

	modelTrace_t mt;
	idVec3 end;

	if( r_showTris.GetInteger() == 0 )
	{
		return;
	}

	idVec4 color( 1, 1, 1, 1 );

	GL_PolygonOffset( -1.0f, -2.0f );

	switch( r_showTris.GetInteger() )
	{
		case 1: // only draw visible ones
			GL_State( GLS_DEPTHMASK | GLS_ALPHAMASK | GLS_POLYMODE_LINE | GLS_POLYGON_OFFSET );
			break;
		case 2:	// draw all front facing
		case 3: // draw all
			GL_State( GLS_DEPTHMASK | GLS_ALPHAMASK | GLS_POLYMODE_LINE | GLS_POLYGON_OFFSET | GLS_DEPTHFUNC_ALWAYS );
			break;
		case 4: // only draw visible ones with blended lines
			GL_State( GLS_DEPTHMASK | GLS_ALPHAMASK | GLS_POLYMODE_LINE | GLS_POLYGON_OFFSET | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
			color[3] = 0.4f;
			break;
	}

	if( r_showTris.GetInteger() == 3 )
	{
		GL_State( ( glStateBits & ~( GLS_CULL_MASK ) ) | GLS_CULL_TWOSIDED );       // SRS - Added parens to silence build warnings
	}

	GL_Color( color );

	DBG_RenderDrawSurfListWithFunction( drawSurfs, numDrawSurfs );

	if( r_showTris.GetInteger() == 3 )
	{
		GL_State( ( glStateBits & ~( GLS_CULL_MASK ) ) | GLS_CULL_FRONTSIDED );     // SRS - Added parens to silence build warnings
	}
}

/*
=====================
idRenderBackend::DBG_ShowSurfaceInfo

Debugging tool
=====================
*/


static idStr surfModelName, surfMatName;
static idVec3 surfPoint;
static bool surfTraced = false;


void idRenderSystemLocal::OnFrame()
{
	// Do tracing at a safe time to avoid threading issues.
	modelTrace_t mt;
	idVec3 start, end;

	surfTraced = false;

	if( !r_showSurfaceInfo.GetBool() )
	{
		return;
	}

	if( tr.primaryView == NULL )
	{
		return;
	}

	// start far enough away that we don't hit the player model
	start = tr.primaryView->renderView.vieworg + tr.primaryView->renderView.viewaxis[0] * 32;
	end = start + tr.primaryView->renderView.viewaxis[0] * 1000.0f;
	if( !tr.primaryWorld->Trace( mt, start, end, 0.0f, false ) )
	{
		return;
	}

	surfPoint = mt.point;
	surfModelName = mt.entity->hModel->Name();
	surfMatName = mt.material->GetName();
	surfTraced = true;
}


void idRenderBackend::DBG_ShowSurfaceInfo( drawSurf_t** drawSurfs, int numDrawSurfs )
{
	if( !r_showSurfaceInfo.GetBool() || !surfTraced )
	{
		return;
	}

	// globalImages->BindNull();
	// qglDisable( GL_TEXTURE_2D );

	DBG_SimpleWorldSetup();

	// foresthale 2014-05-02: don't use a shader for tools
	//renderProgManager.BindShader_TextureVertexColor();
	GL_SelectTexture( 0 );
	globalImages->whiteImage->Bind();

	RB_SetVertexColorParms( SVC_MODULATE );
	// foresthale 2014-05-02: don't use a shader for tools
	//renderProgManager.CommitUniforms();

	GL_Color( 1, 1, 1 );

	static float scale = -1;
	static float bias = -2;

	GL_PolygonOffset( scale, bias );
	GL_State( GLS_DEPTHFUNC_ALWAYS | GLS_POLYMODE_LINE | GLS_POLYGON_OFFSET );

	// idVec3	trans[3];
	// float	matrix[16];

	// transform the object verts into global space
	// R_AxisToModelMatrix( mt.entity->axis, mt.entity->origin, matrix );

	tr.primaryWorld->DrawText( surfModelName, surfPoint + tr.primaryView->renderView.viewaxis[2] * 12,
							   0.35f, colorRed, tr.primaryView->renderView.viewaxis );
	tr.primaryWorld->DrawText( surfMatName, surfPoint,
							   0.35f, colorBlue, tr.primaryView->renderView.viewaxis );
}

/*
=====================
idRenderBackend::DBG_ShowViewEntitys

Debugging tool
=====================
*/
void idRenderBackend::DBG_ShowViewEntitys( viewEntity_t* vModels )
{
	if( !r_showViewEntitys.GetBool() )
	{
		return;
	}

	if( r_showViewEntitys.GetInteger() >= 2 )
	{
		common->Printf( "view entities: " );
		for( const viewEntity_t* vModel = vModels; vModel; vModel = vModel->next )
		{
			if( vModel->entityDef->IsDirectlyVisible() )
			{
				common->Printf( "<%i> ", vModel->entityDef->index );
			}
			else
			{
				common->Printf( "%i ", vModel->entityDef->index );
			}
		}
		common->Printf( "\n" );
	}

	renderProgManager.BindShader_Color();

	GL_Color( 1, 1, 1 );
	GL_State( GLS_DEPTHFUNC_ALWAYS | GLS_POLYMODE_LINE | GLS_CULL_TWOSIDED );

	for( const viewEntity_t* vModel = vModels; vModel; vModel = vModel->next )
	{
		idBounds	b;

		//glLoadMatrixf( vModel->modelViewMatrix );

		const idRenderEntityLocal* edef = vModel->entityDef;
		if( !edef )
		{
			continue;
		}

		// draw the model bounds in white if directly visible,
		// or, blue if it is only-for-sahdow
		idVec4	color;
		if( edef->IsDirectlyVisible() )
		{
			color.Set( 1, 1, 1, 1 );
		}
		else
		{
			color.Set( 0, 0, 1, 1 );
		}
		GL_Color( color[0], color[1], color[2] );
		RB_DrawBounds( edef->localReferenceBounds );

		// transform the upper bounds corner into global space
		if( r_showViewEntitys.GetInteger() >= 2 )
		{
			idVec3 corner;
			R_LocalPointToGlobal( vModel->modelMatrix, edef->localReferenceBounds[1], corner );

			tr.primaryWorld->DrawText(
				va( "%i:%s", edef->index, edef->parms.hModel->Name() ),
				corner,
				0.25f, color,
				tr.primaryView->renderView.viewaxis );
		}

		// draw the actual bounds in yellow if different
		if( r_showViewEntitys.GetInteger() >= 3 )
		{
			GL_Color( 1, 1, 0 );
			// FIXME: cannot instantiate a dynamic model from the renderer back-end
			idRenderModel* model = R_EntityDefDynamicModel( vModel->entityDef );
			if( !model )
			{
				continue;	// particles won't instantiate without a current view
			}
			b = model->Bounds( &vModel->entityDef->parms );
			if( b != vModel->entityDef->localReferenceBounds )
			{
				RB_DrawBounds( b );
			}
		}
	}
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
	int		i, j;
	drawSurf_t*	drawSurf;
	const srfTriangles_t*	tri;

	if( !r_showTexturePolarity.GetBool() )
	{
		return;
	}

	GL_State( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );

	GL_Color( 1, 1, 1 );

	for( i = 0; i < numDrawSurfs; i++ )
	{
		drawSurf = drawSurfs[i];
		tri = drawSurf->frontEndGeo;
		if( tri == NULL || tri->verts == NULL )
		{
			continue;
		}

		DBG_SimpleSurfaceSetup( drawSurf );

		glBegin( GL_TRIANGLES );
		for( j = 0; j < tri->numIndexes; j += 3 )
		{
			idDrawVert*	a, *b, *c;
			float		d0[5], d1[5];
			float		area;

			a = tri->verts + tri->indexes[j];
			b = tri->verts + tri->indexes[j + 1];
			c = tri->verts + tri->indexes[j + 2];

			const idVec2 aST = a->GetTexCoord();
			const idVec2 bST = b->GetTexCoord();
			const idVec2 cST = c->GetTexCoord();

			d0[3] = bST[0] - aST[0];
			d0[4] = bST[1] - aST[1];

			d1[3] = cST[0] - aST[0];
			d1[4] = cST[1] - aST[1];

			area = d0[3] * d1[4] - d0[4] * d1[3];

			if( idMath::Fabs( area ) < 0.0001 )
			{
				GL_Color( 0, 0, 1, 0.5 );
			}
			else  if( area < 0 )
			{
				GL_Color( 1, 0, 0, 0.5 );
			}
			else
			{
				GL_Color( 0, 1, 0, 0.5 );
			}
			glVertex3fv( a->xyz.ToFloatPtr() );
			glVertex3fv( b->xyz.ToFloatPtr() );
			glVertex3fv( c->xyz.ToFloatPtr() );
		}
		glEnd();
	}

	GL_State( GLS_DEFAULT );
}

/*
=====================
idRenderBackend::DBG_ShowUnsmoothedTangents

Shade materials that are using unsmoothed tangents
=====================
*/
void idRenderBackend::DBG_ShowUnsmoothedTangents( drawSurf_t** drawSurfs, int numDrawSurfs )
{
	int		i, j;
	drawSurf_t*	drawSurf;
	const srfTriangles_t*	tri;

	if( !r_showUnsmoothedTangents.GetBool() )
	{
		return;
	}

	GL_State( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );

	GL_Color( 0, 1, 0, 0.5 );

	for( i = 0; i < numDrawSurfs; i++ )
	{
		drawSurf = drawSurfs[i];

		if( !drawSurf->material->UseUnsmoothedTangents() )
		{
			continue;
		}

		DBG_SimpleSurfaceSetup( drawSurf );

		tri = drawSurf->frontEndGeo;
		if( tri == NULL || tri->verts == NULL )
		{
			continue;
		}

		glBegin( GL_TRIANGLES );
		for( j = 0; j < tri->numIndexes; j += 3 )
		{
			idDrawVert*	a, *b, *c;

			a = tri->verts + tri->indexes[j];
			b = tri->verts + tri->indexes[j + 1];
			c = tri->verts + tri->indexes[j + 2];

			glVertex3fv( a->xyz.ToFloatPtr() );
			glVertex3fv( b->xyz.ToFloatPtr() );
			glVertex3fv( c->xyz.ToFloatPtr() );
		}
		glEnd();
	}

	GL_State( GLS_DEFAULT );
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
	int		i, j;
	drawSurf_t*	drawSurf;
	const srfTriangles_t*	tri;

	if( !r_showTangentSpace.GetInteger() )
	{
		return;
	}

	GL_State( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );

	for( i = 0; i < numDrawSurfs; i++ )
	{
		drawSurf = drawSurfs[i];

		DBG_SimpleSurfaceSetup( drawSurf );

		tri = drawSurf->frontEndGeo;
		if( tri == NULL || tri->verts == NULL )
		{
			continue;
		}

		glBegin( GL_TRIANGLES );
		for( j = 0; j < tri->numIndexes; j++ )
		{
			const idDrawVert* v;

			v = &tri->verts[tri->indexes[j]];

			if( r_showTangentSpace.GetInteger() == 1 )
			{
				const idVec3 vertexTangent = v->GetTangent();
				GL_Color( 0.5 + 0.5 * vertexTangent[0],  0.5 + 0.5 * vertexTangent[1],
						  0.5 + 0.5 * vertexTangent[2], 0.5 );
			}
			else if( r_showTangentSpace.GetInteger() == 2 )
			{
				const idVec3 vertexBiTangent = v->GetBiTangent();
				GL_Color( 0.5 + 0.5 * vertexBiTangent[0],  0.5 + 0.5 * vertexBiTangent[1],
						  0.5 + 0.5 * vertexBiTangent[2], 0.5 );
			}
			else
			{
				const idVec3 vertexNormal = v->GetNormal();
				GL_Color( 0.5 + 0.5 * vertexNormal[0],  0.5 + 0.5 * vertexNormal[1],
						  0.5 + 0.5 * vertexNormal[2], 0.5 );
			}
			glVertex3fv( v->xyz.ToFloatPtr() );
		}
		glEnd();
	}

	GL_State( GLS_DEFAULT );
}

/*
=====================
idRenderBackend::DBG_ShowVertexColor

Draw each triangle with the solid vertex colors
=====================
*/
void idRenderBackend::DBG_ShowVertexColor( drawSurf_t** drawSurfs, int numDrawSurfs )
{
	int		i, j;
	drawSurf_t*	drawSurf;
	const srfTriangles_t*	tri;

	if( !r_showVertexColor.GetBool() )
	{
		return;
	}

	// RB begin
	renderProgManager.BindShader_VertexColor();

	GL_State( GLS_DEPTHFUNC_LESS );

	for( i = 0; i < numDrawSurfs; i++ )
	{
		drawSurf = drawSurfs[i];

		DBG_SimpleSurfaceSetup( drawSurf );

		tri = drawSurf->frontEndGeo;
		if( tri == NULL || tri->verts == NULL )
		{
			continue;
		}

		renderProgManager.CommitUniforms( glStateBits );

		glBegin( GL_TRIANGLES );
		for( j = 0; j < tri->numIndexes; j++ )
		{
			const idDrawVert* v;

			v = &tri->verts[tri->indexes[j]];
			glColor4ubv( v->color );
			glVertex3fv( v->xyz.ToFloatPtr() );
		}
		glEnd();
	}

	// RB end

	GL_State( GLS_DEFAULT );
}

/*
=====================
idRenderBackend::DBG_ShowNormals

Debugging tool
=====================
*/
void idRenderBackend::DBG_ShowNormals( drawSurf_t** drawSurfs, int numDrawSurfs )
{
	int			i, j;
	drawSurf_t*	drawSurf;
	idVec3		end;
	const srfTriangles_t*	tri;
	float		size;
	bool		showNumbers;
	idVec3		pos;

	if( r_showNormals.GetFloat() == 0.0f )
	{
		return;
	}

	if( !r_debugLineDepthTest.GetBool() )
	{
		GL_State( GLS_POLYMODE_LINE | GLS_DEPTHFUNC_ALWAYS );
	}
	else
	{
		GL_State( GLS_POLYMODE_LINE );
	}

	size = r_showNormals.GetFloat();
	if( size < 0.0f )
	{
		size = -size;
		showNumbers = true;
	}
	else
	{
		showNumbers = false;
	}

	for( i = 0; i < numDrawSurfs; i++ )
	{
		drawSurf = drawSurfs[i];

		DBG_SimpleSurfaceSetup( drawSurf );

		tri = drawSurf->frontEndGeo;
		if( tri == NULL || tri->verts == NULL )
		{
			continue;
		}

		// RB begin
		renderProgManager.BindShader_VertexColor();

		glBegin( GL_LINES );
		for( j = 0; j < tri->numVerts; j++ )
		{
			const idVec3 normal = tri->verts[j].GetNormal();
			const idVec3 tangent = tri->verts[j].GetTangent();
			const idVec3 bitangent = tri->verts[j].GetBiTangent();

			glColor3f( 0, 0, 1 );
			glVertex3fv( tri->verts[j].xyz.ToFloatPtr() );
			VectorMA( tri->verts[j].xyz, size, normal, end );
			glVertex3fv( end.ToFloatPtr() );

			glColor3f( 1, 0, 0 );
			glVertex3fv( tri->verts[j].xyz.ToFloatPtr() );
			VectorMA( tri->verts[j].xyz, size, tangent, end );
			glVertex3fv( end.ToFloatPtr() );

			glColor3f( 0, 1, 0 );
			glVertex3fv( tri->verts[j].xyz.ToFloatPtr() );
			VectorMA( tri->verts[j].xyz, size, bitangent, end );
			glVertex3fv( end.ToFloatPtr() );
		}
		glEnd();

		// RB end
	}

	if( showNumbers )
	{
		DBG_SimpleWorldSetup();

		for( i = 0; i < numDrawSurfs; i++ )
		{
			drawSurf = drawSurfs[i];
			tri = drawSurf->frontEndGeo;
			if( tri == NULL || tri->verts == NULL )
			{
				continue;
			}

			for( j = 0; j < tri->numVerts; j++ )
			{
				const idVec3 normal = tri->verts[j].GetNormal();
				const idVec3 tangent = tri->verts[j].GetTangent();
				R_LocalPointToGlobal( drawSurf->space->modelMatrix, tri->verts[j].xyz + tangent + normal * 0.2f, pos );
				RB_DrawText( va( "%d", j ), pos, 0.01f, colorWhite, viewDef->renderView.viewaxis, 1 );
			}

			for( j = 0; j < tri->numIndexes; j += 3 )
			{
				const idVec3 normal = tri->verts[ tri->indexes[ j + 0 ] ].GetNormal();
				R_LocalPointToGlobal( drawSurf->space->modelMatrix, ( tri->verts[ tri->indexes[ j + 0 ] ].xyz + tri->verts[ tri->indexes[ j + 1 ] ].xyz + tri->verts[ tri->indexes[ j + 2 ] ].xyz ) * ( 1.0f / 3.0f ) + normal * 0.2f, pos );
				RB_DrawText( va( "%d", j / 3 ), pos, 0.01f, colorCyan, viewDef->renderView.viewaxis, 1 );
			}
		}
	}
}

/*
=====================
idRenderBackend::DBG_ShowTextureVectors

Draw texture vectors in the center of each triangle
=====================
*/
void idRenderBackend::DBG_ShowTextureVectors( drawSurf_t** drawSurfs, int numDrawSurfs )
{
	if( r_showTextureVectors.GetFloat() == 0.0f )
	{
		return;
	}

	GL_State( GLS_DEPTHFUNC_LESS );

	for( int i = 0; i < numDrawSurfs; i++ )
	{
		drawSurf_t* drawSurf = drawSurfs[i];

		const srfTriangles_t* tri = drawSurf->frontEndGeo;

		if( tri == NULL || tri->verts == NULL )
		{
			continue;
		}

		DBG_SimpleSurfaceSetup( drawSurf );

		// draw non-shared edges in yellow
		glBegin( GL_LINES );

		for( int j = 0; j < tri->numIndexes; j += 3 )
		{
			float d0[5], d1[5];
			idVec3 temp;
			idVec3 tangents[2];

			const idDrawVert* a = &tri->verts[tri->indexes[j + 0]];
			const idDrawVert* b = &tri->verts[tri->indexes[j + 1]];
			const idDrawVert* c = &tri->verts[tri->indexes[j + 2]];

			const idPlane plane( a->xyz, b->xyz, c->xyz );

			// make the midpoint slightly above the triangle
			const idVec3 mid = ( a->xyz + b->xyz + c->xyz ) * ( 1.0f / 3.0f ) + 0.1f * plane.Normal();

			// calculate the texture vectors
			const idVec2 aST = a->GetTexCoord();
			const idVec2 bST = b->GetTexCoord();
			const idVec2 cST = c->GetTexCoord();

			d0[0] = b->xyz[0] - a->xyz[0];
			d0[1] = b->xyz[1] - a->xyz[1];
			d0[2] = b->xyz[2] - a->xyz[2];
			d0[3] = bST[0] - aST[0];
			d0[4] = bST[1] - aST[1];

			d1[0] = c->xyz[0] - a->xyz[0];
			d1[1] = c->xyz[1] - a->xyz[1];
			d1[2] = c->xyz[2] - a->xyz[2];
			d1[3] = cST[0] - aST[0];
			d1[4] = cST[1] - aST[1];

			const float area = d0[3] * d1[4] - d0[4] * d1[3];
			if( area == 0 )
			{
				continue;
			}
			const float inva = 1.0f / area;

			temp[0] = ( d0[0] * d1[4] - d0[4] * d1[0] ) * inva;
			temp[1] = ( d0[1] * d1[4] - d0[4] * d1[1] ) * inva;
			temp[2] = ( d0[2] * d1[4] - d0[4] * d1[2] ) * inva;
			temp.Normalize();
			tangents[0] = temp;

			temp[0] = ( d0[3] * d1[0] - d0[0] * d1[3] ) * inva;
			temp[1] = ( d0[3] * d1[1] - d0[1] * d1[3] ) * inva;
			temp[2] = ( d0[3] * d1[2] - d0[2] * d1[3] ) * inva;
			temp.Normalize();
			tangents[1] = temp;

			// draw the tangents
			tangents[0] = mid + tangents[0] * r_showTextureVectors.GetFloat();
			tangents[1] = mid + tangents[1] * r_showTextureVectors.GetFloat();

			GL_Color( 1, 0, 0 );
			glVertex3fv( mid.ToFloatPtr() );
			glVertex3fv( tangents[0].ToFloatPtr() );

			GL_Color( 0, 1, 0 );
			glVertex3fv( mid.ToFloatPtr() );
			glVertex3fv( tangents[1].ToFloatPtr() );
		}

		glEnd();
	}
}

/*
=====================
idRenderBackend::DBG_ShowDominantTris

Draw lines from each vertex to the dominant triangle center
=====================
*/
void idRenderBackend::DBG_ShowDominantTris( drawSurf_t** drawSurfs, int numDrawSurfs )
{
	int			i, j;
	drawSurf_t*	drawSurf;
	const srfTriangles_t*	tri;

	if( !r_showDominantTri.GetBool() )
	{
		return;
	}

	GL_State( GLS_DEPTHFUNC_LESS );

	GL_PolygonOffset( -1, -2 );
	glEnable( GL_POLYGON_OFFSET_LINE );

	for( i = 0; i < numDrawSurfs; i++ )
	{
		drawSurf = drawSurfs[i];

		tri = drawSurf->frontEndGeo;

		if( tri == NULL || tri->verts == NULL )
		{
			continue;
		}
		if( !tri->dominantTris )
		{
			continue;
		}

		DBG_SimpleSurfaceSetup( drawSurf );

		GL_Color( 1, 1, 0 );
		glBegin( GL_LINES );

		for( j = 0; j < tri->numVerts; j++ )
		{
			const idDrawVert* a, *b, *c;
			idVec3		mid;

			// find the midpoint of the dominant tri

			a = &tri->verts[j];
			b = &tri->verts[tri->dominantTris[j].v2];
			c = &tri->verts[tri->dominantTris[j].v3];

			mid = ( a->xyz + b->xyz + c->xyz ) * ( 1.0f / 3.0f );

			glVertex3fv( mid.ToFloatPtr() );
			glVertex3fv( a->xyz.ToFloatPtr() );
		}

		glEnd();
	}
	glDisable( GL_POLYGON_OFFSET_LINE );
}

/*
=====================
idRenderBackend::DBG_ShowEdges

Debugging tool
=====================
*/
void idRenderBackend::DBG_ShowEdges( drawSurf_t** drawSurfs, int numDrawSurfs )
{
	int			i, j, k, m, n, o;
	drawSurf_t*	drawSurf;
	const srfTriangles_t*	tri;
	const silEdge_t*			edge;
	int			danglePlane;

	if( !r_showEdges.GetBool() )
	{
		return;
	}

	GL_State( GLS_DEPTHFUNC_ALWAYS );

	for( i = 0; i < numDrawSurfs; i++ )
	{
		drawSurf = drawSurfs[i];

		tri = drawSurf->frontEndGeo;

		idDrawVert* ac = ( idDrawVert* )tri->verts;
		if( !ac )
		{
			continue;
		}

		DBG_SimpleSurfaceSetup( drawSurf );

		// draw non-shared edges in yellow
		GL_Color( 1, 1, 0 );
		glBegin( GL_LINES );

		for( j = 0; j < tri->numIndexes; j += 3 )
		{
			for( k = 0; k < 3; k++ )
			{
				int		l, i1, i2;
				l = ( k == 2 ) ? 0 : k + 1;
				i1 = tri->indexes[j + k];
				i2 = tri->indexes[j + l];

				// if these are used backwards, the edge is shared
				for( m = 0; m < tri->numIndexes; m += 3 )
				{
					for( n = 0; n < 3; n++ )
					{
						o = ( n == 2 ) ? 0 : n + 1;
						if( tri->indexes[m + n] == i2 && tri->indexes[m + o] == i1 )
						{
							break;
						}
					}
					if( n != 3 )
					{
						break;
					}
				}

				// if we didn't find a backwards listing, draw it in yellow
				if( m == tri->numIndexes )
				{
					glVertex3fv( ac[ i1 ].xyz.ToFloatPtr() );
					glVertex3fv( ac[ i2 ].xyz.ToFloatPtr() );
				}

			}
		}

		glEnd();

		// draw dangling sil edges in red
		if( !tri->silEdges )
		{
			continue;
		}

		// the plane number after all real planes
		// is the dangling edge
		danglePlane = tri->numIndexes / 3;

		GL_Color( 1, 0, 0 );

		glBegin( GL_LINES );
		for( j = 0; j < tri->numSilEdges; j++ )
		{
			edge = tri->silEdges + j;

			if( edge->p1 != danglePlane && edge->p2 != danglePlane )
			{
				continue;
			}

			glVertex3fv( ac[ edge->v1 ].xyz.ToFloatPtr() );
			glVertex3fv( ac[ edge->v2 ].xyz.ToFloatPtr() );
		}
		glEnd();
	}
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
	if( !r_showLights.GetInteger() )
	{
		return;
	}

	GL_State( GLS_DEFAULT | GLS_CULL_TWOSIDED );

	renderProgManager.BindShader_Color();

	common->Printf( "volumes: " );	// FIXME: not in back end!

	int count = 0;
	for( viewLight_t* vLight = viewDef->viewLights; vLight != NULL; vLight = vLight->next )
	{
		count++;

		// depth buffered planes
		if( r_showLights.GetInteger() >= 2 )
		{
			GL_State( GLS_DEPTHFUNC_ALWAYS | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHMASK );

			// RB: show different light types
			if( vLight->parallel )
			{
				GL_Color( 1.0f, 0.0f, 0.0f, 0.25f );
			}
			else if( vLight->pointLight )
			{
				GL_Color( 0.0f, 0.0f, 1.0f, 0.25f );
			}
			else
			{
				GL_Color( 0.0f, 1.0f, 0.0f, 0.25f );
			}
			// RB end

			idRenderMatrix invProjectMVPMatrix;
			idRenderMatrix::Multiply( viewDef->worldSpace.mvp, vLight->inverseBaseLightProject, invProjectMVPMatrix );
			RB_SetMVP( invProjectMVPMatrix );
			DrawElementsWithCounters( &zeroOneCubeSurface );
		}

		// non-hidden lines
		if( r_showLights.GetInteger() >= 3 )
		{
			GL_State( GLS_DEPTHFUNC_ALWAYS | GLS_POLYMODE_LINE | GLS_DEPTHMASK );
			GL_Color( 1.0f, 1.0f, 1.0f );
			idRenderMatrix invProjectMVPMatrix;
			idRenderMatrix::Multiply( viewDef->worldSpace.mvp, vLight->inverseBaseLightProject, invProjectMVPMatrix );
			RB_SetMVP( invProjectMVPMatrix );
			DrawElementsWithCounters( &zeroOneCubeSurface );
		}

		common->Printf( "%i ", vLight->lightDef->index );
	}

	common->Printf( " = %i total\n", count );
}

// RB begin
/*
==============
RB_ShowViewEnvprobes

Visualize all environment probes used in the current scene
==============
*/
class idSort_DebugCompareViewEnvprobe : public idSort_Quick< RenderEnvprobeLocal*, idSort_DebugCompareViewEnvprobe >
{
	idVec3	viewOrigin;

public:
	idSort_DebugCompareViewEnvprobe( const idVec3& origin )
	{
		viewOrigin = origin;
	}

	int Compare( RenderEnvprobeLocal* const& a, RenderEnvprobeLocal* const& b ) const
	{
		float adist = ( viewOrigin - a->parms.origin ).LengthSqr();
		float bdist = ( viewOrigin - b->parms.origin ).LengthSqr();

		if( adist < bdist )
		{
			return -1;
		}

		if( adist > bdist )
		{
			return 1;
		}

		return 0;
	}
};

void idRenderBackend::DBG_ShowViewEnvprobes()
{
	if( !r_showViewEnvprobes.GetInteger() )
	{
		return;
	}

	GL_State( GLS_DEFAULT | GLS_CULL_TWOSIDED );

	int count = 0;
	for( viewEnvprobe_t* vProbe = viewDef->viewEnvprobes; vProbe != NULL; vProbe = vProbe->next )
	{
		count++;

		renderProgManager.BindShader_DebugOctahedron();

		GL_State( GLS_DEPTHFUNC_ALWAYS | GLS_DEPTHMASK );
		GL_Color( 1.0f, 1.0f, 1.0f );

		idMat3 axis;
		axis.Identity();

		float modelMatrix[16];
		R_AxisToModelMatrix( axis, vProbe->globalOrigin, modelMatrix );

		idRenderMatrix modelRenderMatrix;
		idRenderMatrix::CreateFromOriginAxis( vProbe->globalOrigin, axis, modelRenderMatrix );

		// calculate the matrix that transforms the unit cube to exactly cover the model in world space
		const float size = 16.0f;
		idBounds debugBounds( idVec3( -size ), idVec3( size ) );

		idRenderMatrix inverseBaseModelProject;
		idRenderMatrix::OffsetScaleForBounds( modelRenderMatrix, debugBounds, inverseBaseModelProject );

		idRenderMatrix invProjectMVPMatrix;
		idRenderMatrix::Multiply( viewDef->worldSpace.mvp, inverseBaseModelProject, invProjectMVPMatrix );
		RB_SetMVP( invProjectMVPMatrix );

		idVec4 localViewOrigin( 1.0f );
		idVec4 globalViewOrigin;
		globalViewOrigin.x = viewDef->renderView.vieworg.x;
		globalViewOrigin.y = viewDef->renderView.vieworg.y;
		globalViewOrigin.z = viewDef->renderView.vieworg.z;
		globalViewOrigin.w = 1.0f;

		//inverseBaseModelProject.TransformPoint( globalViewOrigin, localViewOrigin );
		R_GlobalPointToLocal( modelMatrix, viewDef->renderView.vieworg, localViewOrigin.ToVec3() );

		renderProgManager.SetUniformValue( RENDERPARM_LOCALVIEWORIGIN, localViewOrigin.ToFloatPtr() ); // rpLocalViewOrigin

		idVec4 textureSize;

		GL_SelectTexture( 0 );
		if( r_showViewEnvprobes.GetInteger() >= 2 )
		{
			vProbe->irradianceImage->Bind();

			idVec2i res = vProbe->irradianceImage->GetUploadResolution();
			textureSize.Set( res.x, res.y, 1.0f / res.x, 1.0f / res.y );
		}
		else
		{
			vProbe->radianceImage->Bind();

			idVec2i res = vProbe->radianceImage->GetUploadResolution();
			textureSize.Set( res.x, res.y, 1.0f / res.x, 1.0f / res.y );
		}

		renderProgManager.SetUniformValue( RENDERPARM_CASCADEDISTANCES, textureSize.ToFloatPtr() );

		DrawElementsWithCounters( &zeroOneSphereSurface );

		// non-hidden lines
#if 0
		if( r_showViewEnvprobes.GetInteger() >= 3 )
		{
			renderProgManager.BindShader_Color();

			GL_State( GLS_DEPTHFUNC_ALWAYS | GLS_POLYMODE_LINE | GLS_DEPTHMASK );
			GL_Color( 1.0f, 1.0f, 1.0f );

			idRenderMatrix invProjectMVPMatrix;
			idRenderMatrix::Multiply( viewDef->worldSpace.mvp, vProbe->inverseBaseProbeProject, invProjectMVPMatrix );
			RB_SetMVP( invProjectMVPMatrix );
			DrawElementsWithCounters( &zeroOneCubeSurface );
		}
#endif
	}

	if( tr.primaryWorld && r_showViewEnvprobes.GetInteger() == 3 )
	{
		/*
		idList<viewEnvprobe_t*, TAG_RENDER_ENVPROBE> viewEnvprobes;
		for( viewEnvprobe_t* vProbe = viewDef->viewEnvprobes; vProbe != NULL; vProbe = vProbe->next )
		{
			viewEnvprobes.AddUnique( vProbe );
		}
		*/

		idList<RenderEnvprobeLocal*, TAG_RENDER_ENVPROBE> viewEnvprobes;
		for( int i = 0; i < tr.primaryWorld->envprobeDefs.Num(); i++ )
		{
			RenderEnvprobeLocal* vProbe = tr.primaryWorld->envprobeDefs[i];
			if( vProbe )
			{
				// check for being closed off behind a door
				if( r_useLightAreaCulling.GetBool()
						&& vProbe->areaNum != -1 && !viewDef->connectedAreas[ vProbe->areaNum ] )
				{
					continue;
				}

				viewEnvprobes.AddUnique( vProbe );
			}
		}

		if( viewEnvprobes.Num() == 0 )
		{
			return;
		}

		idVec3 testOrigin = viewDef->renderView.vieworg;
		//testOrigin += viewDef->renderView.viewaxis[0] * 150.0f;
		//testOrigin -= viewDef->renderView.viewaxis[2] * 16.0f;

		// sort by distance
		viewEnvprobes.SortWithTemplate( idSort_DebugCompareViewEnvprobe( testOrigin ) );

		// draw 3 nearest probes
		renderProgManager.BindShader_Color();

		const int numColors = 3;
		static idVec4 colors[numColors] = { colorRed, colorGreen, colorBlue };

		// form a triangle of the 3 closest probes
		idVec3 verts[3];
		for( int i = 0; i < 3; i++ )
		{
			verts[i] = viewEnvprobes[0]->parms.origin;
		}

		for( int i = 0; i < viewEnvprobes.Num() && i < 3; i++ )
		{
			RenderEnvprobeLocal* vProbe = viewEnvprobes[i];

			verts[i] = vProbe->parms.origin;
		}

		idVec3 closest = R_ClosestPointPointTriangle( testOrigin, verts[0], verts[1], verts[2] );
		idVec3 barycentricWeights;

		// find the barycentric coordinates
		float denom = idWinding::TriangleArea( verts[0], verts[1], verts[2] );
		if( denom == 0 )
		{
			// all points at same location
			barycentricWeights.Set( 1, 0, 0 );
		}
		else
		{
			float	a, b, c;

			a = idWinding::TriangleArea( closest, verts[1], verts[2] ) / denom;
			b = idWinding::TriangleArea( closest, verts[2], verts[0] ) / denom;
			c = idWinding::TriangleArea( closest, verts[0], verts[1] ) / denom;

			barycentricWeights.Set( a, b, c );
		}

		idLib::Printf( "bary weights %.2f %.2f %.2f\n", barycentricWeights.x, barycentricWeights.y, barycentricWeights.z );

		idMat3 axis;
		axis.Identity();

		for( int i = 0; i < viewEnvprobes.Num() && i < 3; i++ )
		{
			RenderEnvprobeLocal* vProbe = viewEnvprobes[i];

			verts[i] = vProbe->parms.origin;

			//GL_Color( colors[i] );

			idVec4 color = Lerp( colorBlack, colors[i], barycentricWeights[i] );
			GL_Color( color );

			idRenderMatrix modelRenderMatrix;
			idRenderMatrix::CreateFromOriginAxis( vProbe->parms.origin, axis, modelRenderMatrix );

			// calculate the matrix that transforms the unit cube to exactly cover the model in world space
			const float size = 16.0f;
			idBounds debugBounds( idVec3( -size ), idVec3( size ) );

			idRenderMatrix inverseBaseModelProject;
			idRenderMatrix::OffsetScaleForBounds( modelRenderMatrix, debugBounds, inverseBaseModelProject );

			idRenderMatrix invProjectMVPMatrix;
			idRenderMatrix::Multiply( viewDef->worldSpace.mvp, inverseBaseModelProject, invProjectMVPMatrix );
			RB_SetMVP( invProjectMVPMatrix );

			DrawElementsWithCounters( &zeroOneSphereSurface );
		}

		// draw closest hit
		{
			GL_Color( colorYellow );

			idRenderMatrix modelRenderMatrix;
			idRenderMatrix::CreateFromOriginAxis( closest, axis, modelRenderMatrix );

			// calculate the matrix that transforms the unit cube to exactly cover the model in world space
			const float size = 4.0f;
			idBounds debugBounds( idVec3( -size ), idVec3( size ) );

			idRenderMatrix inverseBaseModelProject;
			idRenderMatrix::OffsetScaleForBounds( modelRenderMatrix, debugBounds, inverseBaseModelProject );

			idRenderMatrix invProjectMVPMatrix;
			idRenderMatrix::Multiply( viewDef->worldSpace.mvp, inverseBaseModelProject, invProjectMVPMatrix );
			RB_SetMVP( invProjectMVPMatrix );

			DrawElementsWithCounters( &zeroOneSphereSurface );
		}
	}
}

void idRenderBackend::DBG_ShowLightGrid()
{
	if( r_showLightGrid.GetInteger() <= 0 || !tr.primaryWorld )
	{
		return;
	}

	// all volumes are expressed in world coordinates

	GL_State( GLS_DEPTHFUNC_ALWAYS | GLS_DEPTHMASK );
	GL_Color( 1.0f, 1.0f, 1.0f );

	idMat3 axis;
	axis.Identity();

	// only show current area
	int cameraArea = tr.primaryWorld->PointInArea( viewDef->renderView.vieworg );
	if( cameraArea == -1 && r_showLightGrid.GetInteger() < 3 )
	{
		return;
	}

	const int numColors = 7;
	static idVec4 colors[numColors] = { colorBrown, colorBlue, colorCyan, colorGreen, colorYellow, colorRed, colorWhite };

	for( int a = 0; a < tr.primaryWorld->NumAreas(); a++ )
	{
		if( r_showLightGrid.GetInteger() < 3 && ( cameraArea != a ) )
		{
			continue;
		}

		portalArea_t* area = &tr.primaryWorld->portalAreas[a];

		// use rpGlobalLightOrigin for lightGrid center
		idVec4 lightGridOrigin( area->lightGrid.lightGridOrigin.x, area->lightGrid.lightGridOrigin.y, area->lightGrid.lightGridOrigin.z, 1.0f );
		idVec4 lightGridSize( area->lightGrid.lightGridSize.x, area->lightGrid.lightGridSize.y, area->lightGrid.lightGridSize.z, 1.0f );
		idVec4 lightGridBounds( area->lightGrid.lightGridBounds[0], area->lightGrid.lightGridBounds[1], area->lightGrid.lightGridBounds[2], 1.0f );

		renderProgManager.SetUniformValue( RENDERPARM_GLOBALLIGHTORIGIN, lightGridOrigin.ToFloatPtr() );
		renderProgManager.SetUniformValue( RENDERPARM_JITTERTEXSCALE, lightGridSize.ToFloatPtr() );
		renderProgManager.SetUniformValue( RENDERPARM_JITTERTEXOFFSET, lightGridBounds.ToFloatPtr() );

		// individual probe sizes on the atlas image
		idVec4 probeSize;
		probeSize[0] = area->lightGrid.imageSingleProbeSize - area->lightGrid.imageBorderSize;
		probeSize[1] = area->lightGrid.imageSingleProbeSize;
		probeSize[2] = area->lightGrid.imageBorderSize;
		probeSize[3] = float( area->lightGrid.imageSingleProbeSize - area->lightGrid.imageBorderSize ) / area->lightGrid.imageSingleProbeSize;
		renderProgManager.SetUniformValue( RENDERPARM_SCREENCORRECTIONFACTOR, probeSize.ToFloatPtr() ); // rpScreenCorrectionFactor

		for( int i = 0; i < area->lightGrid.lightGridPoints.Num(); i++ )
		{
			lightGridPoint_t* gridPoint = &area->lightGrid.lightGridPoints[i];
			if( !gridPoint->valid && r_showLightGrid.GetInteger() < 3 )
			{
				continue;
			}

			idVec3 distanceToCam = gridPoint->origin - viewDef->renderView.vieworg;
			if( distanceToCam.LengthSqr() > ( 1024 * 1024 ) && r_showLightGrid.GetInteger() < 3 )
			{
				continue;
			}

#if 0
			if( i > 53 )
			{
				break;
			}
#endif

			// move center into the cube so we can void using negative results with GetBaseGridCoord
			idVec3 gridPointOrigin = gridPoint->origin + idVec3( 4, 4, 4 );

			idVec4 localViewOrigin( 1.0f );
			idVec4 globalViewOrigin;
			globalViewOrigin.x = viewDef->renderView.vieworg.x;
			globalViewOrigin.y = viewDef->renderView.vieworg.y;
			globalViewOrigin.z = viewDef->renderView.vieworg.z;
			globalViewOrigin.w = 1.0f;

			float modelMatrix[16];
			R_AxisToModelMatrix( axis, gridPointOrigin, modelMatrix );

			R_GlobalPointToLocal( modelMatrix, viewDef->renderView.vieworg, localViewOrigin.ToVec3() );

			renderProgManager.SetUniformValue( RENDERPARM_LOCALVIEWORIGIN, localViewOrigin.ToFloatPtr() ); // rpLocalViewOrigin

			// RB: if we want to get the normals in world space so we need the model -> world matrix
			idRenderMatrix modelMatrix2;
			idRenderMatrix::Transpose( *( idRenderMatrix* )modelMatrix, modelMatrix2 );

			renderProgManager.SetUniformValue( RENDERPARM_MODELMATRIX_X, &modelMatrix2[0][0] );
			renderProgManager.SetUniformValue( RENDERPARM_MODELMATRIX_Y, &modelMatrix2[1][0] );
			renderProgManager.SetUniformValue( RENDERPARM_MODELMATRIX_Z, &modelMatrix2[2][0] );
			renderProgManager.SetUniformValue( RENDERPARM_MODELMATRIX_W, &modelMatrix2[3][0] );


#if 0
			renderProgManager.BindShader_Color();

			int gridCoord[3];
			area->lightGrid.GetBaseGridCoord( gridPoint->origin, gridCoord );

			idVec3 color = area->lightGrid.GetGridCoordDebugColor( gridCoord );
			//idVec3 color = area->lightGrid.GetProbeIndexDebugColor( i );
			//idVec4 color = colors[ i % numColors ];
			GL_Color( color );
#else

			if( r_showLightGrid.GetInteger() == 4 || !area->lightGrid.GetIrradianceImage() )
			{
				renderProgManager.BindShader_Color();

				idVec4 color;
				if( !gridPoint->valid )
				{
					color = colorPurple;
				}
				else
				{
					color = colors[ a % numColors ];
				}

				GL_Color( color );
			}
			else
			{
				renderProgManager.BindShader_DebugLightGrid();

				GL_SelectTexture( 0 );
				area->lightGrid.GetIrradianceImage()->Bind();

				idVec2i res = area->lightGrid.GetIrradianceImage()->GetUploadResolution();
				idVec4 textureSize( res.x, res.y, 1.0f / res.x, 1.0f / res.y );

				renderProgManager.SetUniformValue( RENDERPARM_CASCADEDISTANCES, textureSize.ToFloatPtr() );
			}
#endif

			idRenderMatrix modelRenderMatrix;
			idRenderMatrix::CreateFromOriginAxis( gridPoint->origin, axis, modelRenderMatrix );

			// calculate the matrix that transforms the unit cube to exactly cover the model in world space
			const float size = 3.0f;
			idBounds debugBounds( idVec3( -size ), idVec3( size ) );

			idRenderMatrix inverseBaseModelProject;
			idRenderMatrix::OffsetScaleForBounds( modelRenderMatrix, debugBounds, inverseBaseModelProject );

			idRenderMatrix invProjectMVPMatrix;
			idRenderMatrix::Multiply( viewDef->worldSpace.mvp, inverseBaseModelProject, invProjectMVPMatrix );
			RB_SetMVP( invProjectMVPMatrix );

			DrawElementsWithCounters( &zeroOneSphereSurface );
			//DrawElementsWithCounters( &zeroOneCubeSurface );
		}
	}

	if( r_showLightGrid.GetInteger() == 2 )
	{
		// show 8 nearest grid points around the camera and illustrate how the trilerping works

		idVec3          lightOrigin;
		int             pos[3];
		int				gridPointIndex;
		int				gridPointIndex2;
		lightGridPoint_t*  gridPoint;
		lightGridPoint_t*  gridPoint2;
		float           frac[3];
		int             gridStep[3];
		float           totalFactor;

		portalArea_t* area = &tr.primaryWorld->portalAreas[cameraArea];

		renderProgManager.BindShader_Color();

		lightOrigin = viewDef->renderView.vieworg;
		lightOrigin += viewDef->renderView.viewaxis[0] * 150.0f;
		lightOrigin -= viewDef->renderView.viewaxis[2] * 16.0f;

		// draw sample origin we want to test the grid with
		{
			GL_Color( colorYellow );

			idRenderMatrix modelRenderMatrix;
			idRenderMatrix::CreateFromOriginAxis( lightOrigin, axis, modelRenderMatrix );

			// calculate the matrix that transforms the unit cube to exactly cover the model in world space
			const float size = 2.0f;
			idBounds debugBounds( idVec3( -size ), idVec3( size ) );

			idRenderMatrix inverseBaseModelProject;
			idRenderMatrix::OffsetScaleForBounds( modelRenderMatrix, debugBounds, inverseBaseModelProject );

			idRenderMatrix invProjectMVPMatrix;
			idRenderMatrix::Multiply( viewDef->worldSpace.mvp, inverseBaseModelProject, invProjectMVPMatrix );
			RB_SetMVP( invProjectMVPMatrix );

			DrawElementsWithCounters( &zeroOneSphereSurface );
		}

		// find base grid point
		lightOrigin -= area->lightGrid.lightGridOrigin;
		for( int i = 0; i < 3; i++ )
		{
			float           v;

			v = lightOrigin[i] * ( 1.0f / area->lightGrid.lightGridSize[i] );
			pos[i] = floor( v );
			frac[i] = v - pos[i];

			if( pos[i] < 0 )
			{
				pos[i] = 0;
			}
			else if( pos[i] >= area->lightGrid.lightGridBounds[i] - 1 )
			{
				pos[i] = area->lightGrid.lightGridBounds[i] - 1;
			}
		}

		// trilerp the light value
		gridStep[0] = 1;
		gridStep[1] = area->lightGrid.lightGridBounds[0];
		gridStep[2] = area->lightGrid.lightGridBounds[0] * area->lightGrid.lightGridBounds[1];

		gridPointIndex = pos[0] * gridStep[0] + pos[1] * gridStep[1] + pos[2] * gridStep[2];
		gridPoint = &area->lightGrid.lightGridPoints[ gridPointIndex ];

		totalFactor = 0;
		idVec3 cornerOffsets[8];

		for( int i = 0; i < 8; i++ )
		{
			float  factor = 1.0;

			gridPoint2 = gridPoint;
			gridPointIndex2 = gridPointIndex;

			for( int j = 0; j < 3; j++ )
			{
				cornerOffsets[i][j] = i & ( 1 << j );

				if( cornerOffsets[i][j] > 0.0f )
				{
					factor *= frac[j];

#if 1
					gridPointIndex2 += gridStep[j];
					if( gridPointIndex2 < 0 || gridPointIndex2 >= area->lightGrid.lightGridPoints.Num() )
					{
						// ignore values outside lightgrid
						continue;
					}

					gridPoint2 = &area->lightGrid.lightGridPoints[ gridPointIndex2 ];
#else
					if( pos[j] + 1 > area->lightGrid.lightGridBounds[j] - 1 )
					{
						// ignore values outside lightgrid
						break;
					}

					gridPoint2 += gridStep[j];
#endif
				}
				else
				{
					factor *= ( 1.0f - frac[j] );
				}
			}

			if( !gridPoint2->valid )
			{
				// ignore samples in walls
				continue;
			}

			totalFactor += factor;

			idVec4 color = Lerp( colorBlack, colorGreen, factor );
			GL_Color( color );

			idRenderMatrix modelRenderMatrix;
			idRenderMatrix::CreateFromOriginAxis( gridPoint2->origin, axis, modelRenderMatrix );

			// calculate the matrix that transforms the unit cube to exactly cover the model in world space
			const float size = 4.0f;
			idBounds debugBounds( idVec3( -size ), idVec3( size ) );

			idRenderMatrix inverseBaseModelProject;
			idRenderMatrix::OffsetScaleForBounds( modelRenderMatrix, debugBounds, inverseBaseModelProject );

			idRenderMatrix invProjectMVPMatrix;
			idRenderMatrix::Multiply( viewDef->worldSpace.mvp, inverseBaseModelProject, invProjectMVPMatrix );
			RB_SetMVP( invProjectMVPMatrix );

			DrawElementsWithCounters( &zeroOneSphereSurface );
		}

		// draw main grid point where camera position snapped to
		GL_Color( colorRed );

		idRenderMatrix modelRenderMatrix;
		idRenderMatrix::CreateFromOriginAxis( gridPoint->origin, axis, modelRenderMatrix );

		// calculate the matrix that transforms the unit cube to exactly cover the model in world space
		const float size = 5.0f;
		idBounds debugBounds( idVec3( -size ), idVec3( size ) );

		idRenderMatrix inverseBaseModelProject;
		idRenderMatrix::OffsetScaleForBounds( modelRenderMatrix, debugBounds, inverseBaseModelProject );

		idRenderMatrix invProjectMVPMatrix;
		idRenderMatrix::Multiply( viewDef->worldSpace.mvp, inverseBaseModelProject, invProjectMVPMatrix );
		RB_SetMVP( invProjectMVPMatrix );

		DrawElementsWithCounters( &zeroOneSphereSurface );
	}
}

void idRenderBackend::DBG_ShowShadowMapLODs()
{
	if( !r_showShadowMapLODs.GetInteger() )
	{
		return;
	}

	GL_State( GLS_DEFAULT | GLS_CULL_TWOSIDED );

	renderProgManager.BindShader_Color();

	common->Printf( "volumes: " );	// FIXME: not in back end!

	int count = 0;
	for( viewLight_t* vLight = viewDef->viewLights; vLight != NULL; vLight = vLight->next )
	{
		if( !vLight->lightDef->LightCastsShadows() )
		{
			continue;
		}

		count++;

		// depth buffered planes
		if( r_showShadowMapLODs.GetInteger() >= 1 )
		{
			GL_State( GLS_DEPTHFUNC_ALWAYS | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHMASK );

			idVec4 c;
			if( vLight->shadowLOD == 0 )
			{
				c = colorRed;
			}
			else if( vLight->shadowLOD == 1 )
			{
				c = colorGreen;
			}
			else if( vLight->shadowLOD == 2 )
			{
				c = colorBlue;
			}
			else if( vLight->shadowLOD == 3 )
			{
				c = colorYellow;
			}
			else if( vLight->shadowLOD == 4 )
			{
				c = colorMagenta;
			}
			else if( vLight->shadowLOD == 5 )
			{
				c = colorCyan;
			}
			else
			{
				c = colorMdGrey;
			}

			c[3] = 0.25f;
			GL_Color( c );

			idRenderMatrix invProjectMVPMatrix;
			idRenderMatrix::Multiply( viewDef->worldSpace.mvp, vLight->inverseBaseLightProject, invProjectMVPMatrix );
			RB_SetMVP( invProjectMVPMatrix );
			DrawElementsWithCounters( &zeroOneCubeSurface );
		}

		// non-hidden lines
		if( r_showShadowMapLODs.GetInteger() >= 2 )
		{
			GL_State( GLS_DEPTHFUNC_ALWAYS | GLS_POLYMODE_LINE | GLS_DEPTHMASK );
			GL_Color( 1.0f, 1.0f, 1.0f );
			idRenderMatrix invProjectMVPMatrix;
			idRenderMatrix::Multiply( viewDef->worldSpace.mvp, vLight->inverseBaseLightProject, invProjectMVPMatrix );
			RB_SetMVP( invProjectMVPMatrix );
			DrawElementsWithCounters( &zeroOneCubeSurface );
		}

		common->Printf( "%i ", vLight->lightDef->index );
	}

	common->Printf( " = %i total\n", count );
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
	if( !r_showPortals.GetBool() )
	{
		return;
	}

	// all portals are expressed in world coordinates
	DBG_SimpleWorldSetup();

	renderProgManager.BindShader_Color();
	GL_State( GLS_DEPTHFUNC_ALWAYS );

	idRenderWorldLocal& world = *viewDef->renderWorld;

	// flood out through portals, setting area viewCount
	for( int i = 0; i < world.numPortalAreas; i++ )
	{
		portalArea_t* area = &world.portalAreas[i];

		if( area->viewCount != tr.viewCount )
		{
			continue;
		}

		for( portal_t* p = area->portals; p; p = p->next )
		{
			idWinding* w = p->w;
			if( !w )
			{
				continue;
			}

			if( world.portalAreas[ p->intoArea ].viewCount != tr.viewCount )
			{
				// red = can't see
				GL_Color( 1, 0, 0 );
			}
			else
			{
				// green = see through
				GL_Color( 0, 1, 0 );
			}

			// RB begin
			renderProgManager.CommitUniforms( glStateBits );
			// RB end

			glBegin( GL_LINE_LOOP );
			for( int j = 0; j < w->GetNumPoints(); j++ )
			{
				glVertex3fv( ( *w )[j].ToFloatPtr() );
			}
			glEnd();
		}
	}
}

/*
================
idRenderBackend::DBG_ClearDebugText
================
*/
void RB_ClearDebugText( int time )
{
	int			i;
	int			num;
	debugText_t*	text;

	rb_debugTextTime = time;

	if( !time )
	{
		// free up our strings
		text = rb_debugText;
		for( i = 0; i < MAX_DEBUG_TEXT; i++, text++ )
		{
			text->text.Clear();
		}
		rb_numDebugText = 0;
		return;
	}

	// copy any text that still needs to be drawn
	num	= 0;
	text = rb_debugText;
	for( i = 0; i < rb_numDebugText; i++, text++ )
	{
		if( text->lifeTime > time )
		{
			if( num != i )
			{
				rb_debugText[ num ] = *text;
			}
			num++;
		}
	}
	rb_numDebugText = num;
}

/*
================
RB_AddDebugText
================
*/
void RB_AddDebugText( const char* text, const idVec3& origin, float scale, const idVec4& color, const idMat3& viewAxis, const int align, const int lifetime, const bool depthTest )
{
	debugText_t* debugText;

	if( rb_numDebugText < MAX_DEBUG_TEXT )
	{
		debugText = &rb_debugText[ rb_numDebugText++ ];
		debugText->text			= text;
		debugText->origin		= origin;
		debugText->scale		= scale;
		debugText->color		= color;
		debugText->viewAxis		= viewAxis;
		debugText->align		= align;
		debugText->lifeTime		= rb_debugTextTime + lifetime;
		debugText->depthTest	= depthTest;
	}
}

/*
================
RB_DrawTextLength

  returns the length of the given text
================
*/
float RB_DrawTextLength( const char* text, float scale, int len )
{
	int i, num, index, charIndex;
	float spacing, textLen = 0.0f;

	if( text && *text )
	{
		if( !len )
		{
			len = strlen( text );
		}
		for( i = 0; i < len; i++ )
		{
			charIndex = text[i] - 32;
			if( charIndex < 0 || charIndex > NUM_SIMPLEX_CHARS )
			{
				continue;
			}
			num = simplex[charIndex][0] * 2;
			spacing = simplex[charIndex][1];
			index = 2;

			while( index - 2 < num )
			{
				if( simplex[charIndex][index] < 0 )
				{
					index++;
					continue;
				}
				index += 2;
				if( simplex[charIndex][index] < 0 )
				{
					index++;
					continue;
				}
			}
			textLen += spacing * scale;
		}
	}
	return textLen;
}

/*
================
RB_DrawText

  oriented on the viewaxis
  align can be 0-left, 1-center (default), 2-right
================
*/
static void RB_DrawText( const char* text, const idVec3& origin, float scale, const idVec4& color, const idMat3& viewAxis, const int align )
{
	renderProgManager.BindShader_Color();

	// RB begin
	//GL_Color( color[0], color[1], color[2], 1 /*color[3]*/ );
	renderProgManager.CommitUniforms( tr.backend.GL_GetCurrentState() );
	// RB end

	int i, j, len, num, index, charIndex, line;
	float textLen = 1.0f, spacing = 1.0f;
	idVec3 org, p1, p2;

	if( text && *text )
	{
		glBegin( GL_LINES );

		if( text[0] == '\n' )
		{
			line = 1;
		}
		else
		{
			line = 0;
		}

		len = strlen( text );
		for( i = 0; i < len; i++ )
		{

			if( i == 0 || text[i] == '\n' )
			{
				org = origin - viewAxis[2] * ( line * 36.0f * scale );
				if( align != 0 )
				{
					for( j = 1; i + j <= len; j++ )
					{
						if( i + j == len || text[i + j] == '\n' )
						{
							textLen = RB_DrawTextLength( text + i, scale, j );
							break;
						}
					}
					if( align == 2 )
					{
						// right
						org += viewAxis[1] * textLen;
					}
					else
					{
						// center
						org += viewAxis[1] * ( textLen * 0.5f );
					}
				}
				line++;
			}

			charIndex = text[i] - 32;
			if( charIndex < 0 || charIndex > NUM_SIMPLEX_CHARS )
			{
				continue;
			}
			num = simplex[charIndex][0] * 2;
			spacing = simplex[charIndex][1];
			index = 2;

			while( index - 2 < num )
			{
				if( simplex[charIndex][index] < 0 )
				{
					index++;
					continue;
				}
				p1 = org + scale * simplex[charIndex][index] * -viewAxis[1] + scale * simplex[charIndex][index + 1] * viewAxis[2];
				index += 2;
				if( simplex[charIndex][index] < 0 )
				{
					index++;
					continue;
				}
				p2 = org + scale * simplex[charIndex][index] * -viewAxis[1] + scale * simplex[charIndex][index + 1] * viewAxis[2];

				glVertex3fv( p1.ToFloatPtr() );
				glVertex3fv( p2.ToFloatPtr() );
			}
			org -= viewAxis[1] * ( spacing * scale );
		}

		glEnd();
	}
}

/*
================
idRenderBackend::DBG_ShowDebugText
================
*/
void idRenderBackend::DBG_ShowDebugText()
{
	int			i;
	int			width;
	debugText_t*	text;

	if( !rb_numDebugText )
	{
		return;
	}

	// all lines are expressed in world coordinates
	DBG_SimpleWorldSetup();

	width = r_debugLineWidth.GetInteger();
	if( width < 1 )
	{
		width = 1;
	}
	else if( width > 10 )
	{
		width = 10;
	}

	// draw lines
	glLineWidth( width );


	if( !r_debugLineDepthTest.GetBool() )
	{
		GL_State( GLS_POLYMODE_LINE | GLS_DEPTHFUNC_ALWAYS );
	}
	else
	{
		GL_State( GLS_POLYMODE_LINE );
	}

	text = rb_debugText;
	for( i = 0; i < rb_numDebugText; i++, text++ )
	{
		if( !text->depthTest )
		{
			GL_Color( text->color.ToVec3() );
			RB_DrawText( text->text, text->origin, text->scale, text->color, text->viewAxis, text->align );
		}
	}

	if( !r_debugLineDepthTest.GetBool() )
	{
		GL_State( GLS_POLYMODE_LINE );
	}

	text = rb_debugText;
	for( i = 0; i < rb_numDebugText; i++, text++ )
	{
		if( text->depthTest )
		{
			GL_Color( text->color.ToVec3() );
			RB_DrawText( text->text, text->origin, text->scale, text->color, text->viewAxis, text->align );
		}
	}

	glLineWidth( 1 );
}

/*
================
RB_ClearDebugLines
================
*/
void RB_ClearDebugLines( int time )
{
	int			i;
	int			num;
	debugLine_t*	line;

	rb_debugLineTime = time;

	if( !time )
	{
		rb_numDebugLines = 0;
		return;
	}

	// copy any lines that still need to be drawn
	num	= 0;
	line = rb_debugLines;
	for( i = 0; i < rb_numDebugLines; i++, line++ )
	{
		if( line->lifeTime > time )
		{
			if( num != i )
			{
				rb_debugLines[ num ] = *line;
			}
			num++;
		}
	}
	rb_numDebugLines = num;
}

/*
================
RB_AddDebugLine
================
*/
void RB_AddDebugLine( const idVec4& color, const idVec3& start, const idVec3& end, const int lifeTime, const bool depthTest )
{
	debugLine_t* line;

	if( rb_numDebugLines < MAX_DEBUG_LINES )
	{
		line = &rb_debugLines[ rb_numDebugLines++ ];
		line->rgb		= color;
		line->start		= start;
		line->end		= end;
		line->depthTest = depthTest;
		line->lifeTime	= rb_debugLineTime + lifeTime;
	}
}

/*
================
idRenderBackend::DBG_ShowDebugLines
================
*/
void idRenderBackend::DBG_ShowDebugLines()
{
	int			i;
	int			width;
	debugLine_t*	line;

	if( !rb_numDebugLines )
	{
		return;
	}

	// all lines are expressed in world coordinates
	DBG_SimpleWorldSetup();

	// RB begin
	renderProgManager.BindShader_VertexColor();
	renderProgManager.CommitUniforms( glStateBits );
	// RB end

	width = r_debugLineWidth.GetInteger();
	if( width < 1 )
	{
		width = 1;
	}
	else if( width > 10 )
	{
		width = 10;
	}

	// draw lines
	glLineWidth( width );

	if( !r_debugLineDepthTest.GetBool() )
	{
		GL_State( GLS_POLYMODE_LINE | GLS_DEPTHFUNC_ALWAYS );
	}
	else
	{
		GL_State( GLS_POLYMODE_LINE );
	}

	glBegin( GL_LINES );

	line = rb_debugLines;
	for( i = 0; i < rb_numDebugLines; i++, line++ )
	{
		if( !line->depthTest )
		{
			glColor3fv( line->rgb.ToFloatPtr() );
			glVertex3fv( line->start.ToFloatPtr() );
			glVertex3fv( line->end.ToFloatPtr() );
		}
	}
	glEnd();

	if( !r_debugLineDepthTest.GetBool() )
	{
		GL_State( GLS_POLYMODE_LINE );
	}

	glBegin( GL_LINES );

	line = rb_debugLines;
	for( i = 0; i < rb_numDebugLines; i++, line++ )
	{
		if( line->depthTest )
		{
			glColor4fv( line->rgb.ToFloatPtr() );
			glVertex3fv( line->start.ToFloatPtr() );
			glVertex3fv( line->end.ToFloatPtr() );
		}
	}

	glEnd();

	glLineWidth( 1 );
	GL_State( GLS_DEFAULT );
}

/*
================
RB_ClearDebugPolygons
================
*/
void RB_ClearDebugPolygons( int time )
{
	int				i;
	int				num;
	debugPolygon_t*	poly;

	rb_debugPolygonTime = time;

	if( !time )
	{
		rb_numDebugPolygons = 0;
		return;
	}

	// copy any polygons that still need to be drawn
	num	= 0;

	poly = rb_debugPolygons;
	for( i = 0; i < rb_numDebugPolygons; i++, poly++ )
	{
		if( poly->lifeTime > time )
		{
			if( num != i )
			{
				rb_debugPolygons[ num ] = *poly;
			}
			num++;
		}
	}
	rb_numDebugPolygons = num;
}

/*
================
RB_AddDebugPolygon
================
*/
void RB_AddDebugPolygon( const idVec4& color, const idWinding& winding, const int lifeTime, const bool depthTest )
{
	debugPolygon_t* poly;

	if( rb_numDebugPolygons < MAX_DEBUG_POLYGONS )
	{
		poly = &rb_debugPolygons[ rb_numDebugPolygons++ ];
		poly->rgb		= color;
		poly->winding	= winding;
		poly->depthTest = depthTest;
		poly->lifeTime	= rb_debugPolygonTime + lifeTime;
	}
}

/*
================
idRenderBackend::DBG_ShowDebugPolygons
================
*/
void idRenderBackend::DBG_ShowDebugPolygons()
{
	int				i, j;
	debugPolygon_t*	poly;

	if( !rb_numDebugPolygons )
	{
		return;
	}

	// all lines are expressed in world coordinates
	DBG_SimpleWorldSetup();

	// RB begin
	renderProgManager.BindShader_VertexColor();
	renderProgManager.CommitUniforms( glStateBits );
	// RB end

	if( r_debugPolygonFilled.GetBool() )
	{
		GL_State( GLS_POLYGON_OFFSET | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHMASK );
		GL_PolygonOffset( -1, -2 );
	}
	else
	{
		GL_State( GLS_POLYGON_OFFSET | GLS_POLYMODE_LINE );
		GL_PolygonOffset( -1, -2 );
	}

	poly = rb_debugPolygons;
	for( i = 0; i < rb_numDebugPolygons; i++, poly++ )
	{
//		if ( !poly->depthTest ) {

		glColor4fv( poly->rgb.ToFloatPtr() );

		glBegin( GL_POLYGON );

		for( j = 0; j < poly->winding.GetNumPoints(); j++ )
		{
			glVertex3fv( poly->winding[j].ToFloatPtr() );
		}

		glEnd();
//		}
	}

	GL_State( GLS_DEFAULT );

	if( r_debugPolygonFilled.GetBool() )
	{
		glDisable( GL_POLYGON_OFFSET_FILL );
	}
	else
	{
		glDisable( GL_POLYGON_OFFSET_LINE );
	}

	GL_State( GLS_DEFAULT );
}

/*
================
idRenderBackend::DBG_ShowCenterOfProjection
================
*/
void idRenderBackend::DBG_ShowCenterOfProjection()
{
	if( !r_showCenterOfProjection.GetBool() )
	{
		return;
	}

	const int w = viewDef->scissor.GetWidth();
	const int h = viewDef->scissor.GetHeight();
	glClearColor( 1, 0, 0, 1 );
	for( float f = 0.0f ; f <= 1.0f ; f += 0.125f )
	{
		glScissor( w * f - 1 , 0, 3, h );
		glClear( GL_COLOR_BUFFER_BIT );
		glScissor( 0, h * f - 1 , w, 3 );
		glClear( GL_COLOR_BUFFER_BIT );
	}
	glClearColor( 0, 1, 0, 1 );
	float f = 0.5f;
	glScissor( w * f - 1 , 0, 3, h );
	glClear( GL_COLOR_BUFFER_BIT );
	glScissor( 0, h * f - 1 , w, 3 );
	glClear( GL_COLOR_BUFFER_BIT );

	glScissor( 0, 0, w, h );
}

/*
================
idRenderBackend::DBG_ShowLines

Draw exact pixel lines to check pixel center sampling
================
*/
void idRenderBackend::DBG_ShowLines()
{
	if( !r_showLines.GetBool() )
	{
		return;
	}

	glEnable( GL_SCISSOR_TEST );
	if( viewDef->renderView.viewEyeBuffer == 0 )
	{
		glClearColor( 1, 0, 0, 1 );
	}
	else if( viewDef->renderView.viewEyeBuffer == 1 )
	{
		glClearColor( 0, 1, 0, 1 );
	}
	else
	{
		glClearColor( 0, 0, 1, 1 );
	}

	const int start = ( r_showLines.GetInteger() > 2 );	// 1,3 = horizontal, 2,4 = vertical
	if( r_showLines.GetInteger() == 1 || r_showLines.GetInteger() == 3 )
	{
		for( int i = start ; i < tr.GetHeight() ; i += 2 )
		{
			glScissor( 0, i, tr.GetWidth(), 1 );
			glClear( GL_COLOR_BUFFER_BIT );
		}
	}
	else
	{
		for( int i = start ; i < tr.GetWidth() ; i += 2 )
		{
			glScissor( i, 0, 1, tr.GetHeight() );
			glClear( GL_COLOR_BUFFER_BIT );
		}
	}
}


/*
================
idRenderBackend::DBG_TestGamma
================
*/
#define	G_WIDTH		512
#define	G_HEIGHT	512
#define	BAR_HEIGHT	64

void idRenderBackend::DBG_TestGamma()
{
	byte	image[G_HEIGHT][G_WIDTH][4];
	int		i, j;
	int		c, comp;
	int		v, dither;
	int		mask, y;

	if( r_testGamma.GetInteger() <= 0 )
	{
		return;
	}

	v = r_testGamma.GetInteger();
	if( v <= 1 || v >= 196 )
	{
		v = 128;
	}

	memset( image, 0, sizeof( image ) );

	for( mask = 0; mask < 8; mask++ )
	{
		y = mask * BAR_HEIGHT;
		for( c = 0; c < 4; c++ )
		{
			v = c * 64 + 32;
			// solid color
			for( i = 0; i < BAR_HEIGHT / 2; i++ )
			{
				for( j = 0; j < G_WIDTH / 4; j++ )
				{
					for( comp = 0; comp < 3; comp++ )
					{
						if( mask & ( 1 << comp ) )
						{
							image[y + i][c * G_WIDTH / 4 + j][comp] = v;
						}
					}
				}
				// dithered color
				for( j = 0; j < G_WIDTH / 4; j++ )
				{
					if( ( i ^ j ) & 1 )
					{
						dither = c * 64;
					}
					else
					{
						dither = c * 64 + 63;
					}
					for( comp = 0; comp < 3; comp++ )
					{
						if( mask & ( 1 << comp ) )
						{
							image[y + BAR_HEIGHT / 2 + i][c * G_WIDTH / 4 + j][comp] = dither;
						}
					}
				}
			}
		}
	}

	// draw geometrically increasing steps in the bottom row
	y = 0 * BAR_HEIGHT;
	float	scale = 1;
	for( c = 0; c < 4; c++ )
	{
		v = ( int )( 64 * scale );
		if( v < 0 )
		{
			v = 0;
		}
		else if( v > 255 )
		{
			v = 255;
		}
		scale = scale * 1.5;
		for( i = 0; i < BAR_HEIGHT; i++ )
		{
			for( j = 0; j < G_WIDTH / 4; j++ )
			{
				image[y + i][c * G_WIDTH / 4 + j][0] = v;
				image[y + i][c * G_WIDTH / 4 + j][1] = v;
				image[y + i][c * G_WIDTH / 4 + j][2] = v;
			}
		}
	}

	glLoadIdentity();

	glMatrixMode( GL_PROJECTION );
	GL_State( GLS_DEPTHFUNC_ALWAYS );
	GL_Color( 1, 1, 1 );
	glPushMatrix();
	glLoadIdentity();
	glDisable( GL_TEXTURE_2D );
	glOrtho( 0, 1, 0, 1, -1, 1 );
	glRasterPos2f( 0.01f, 0.01f );
	glDrawPixels( G_WIDTH, G_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, image );
	glPopMatrix();
	glEnable( GL_TEXTURE_2D );
	glMatrixMode( GL_MODELVIEW );
}


/*
==================
idRenderBackend::DBG_TestGammaBias
==================
*/
void idRenderBackend::DBG_TestGammaBias()
{
	byte	image[G_HEIGHT][G_WIDTH][4];

	if( r_testGammaBias.GetInteger() <= 0 )
	{
		return;
	}

	int y = 0;
	for( int bias = -40; bias < 40; bias += 10, y += BAR_HEIGHT )
	{
		float	scale = 1;
		for( int c = 0; c < 4; c++ )
		{
			int v = ( int )( 64 * scale + bias );
			scale = scale * 1.5;
			if( v < 0 )
			{
				v = 0;
			}
			else if( v > 255 )
			{
				v = 255;
			}
			for( int i = 0; i < BAR_HEIGHT; i++ )
			{
				for( int j = 0; j < G_WIDTH / 4; j++ )
				{
					image[y + i][c * G_WIDTH / 4 + j][0] = v;
					image[y + i][c * G_WIDTH / 4 + j][1] = v;
					image[y + i][c * G_WIDTH / 4 + j][2] = v;
				}
			}
		}
	}

	glLoadIdentity();
	glMatrixMode( GL_PROJECTION );
	GL_State( GLS_DEPTHFUNC_ALWAYS );
	GL_Color( 1, 1, 1 );
	glPushMatrix();
	glLoadIdentity();
	glDisable( GL_TEXTURE_2D );
	glOrtho( 0, 1, 0, 1, -1, 1 );
	glRasterPos2f( 0.01f, 0.01f );
	glDrawPixels( G_WIDTH, G_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, image );
	glPopMatrix();
	glEnable( GL_TEXTURE_2D );
	glMatrixMode( GL_MODELVIEW );
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
	GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO | GLS_DEPTHMASK | GLS_DEPTHFUNC_ALWAYS | GLS_CULL_TWOSIDED );

	// Set Parms
	float texS[4] = { 1.0f, 0.0f, 0.0f, 0.0f };
	float texT[4] = { 0.0f, 1.0f, 0.0f, 0.0f };
	renderProgManager.SetRenderParm( RENDERPARM_TEXTUREMATRIX_S, texS );
	renderProgManager.SetRenderParm( RENDERPARM_TEXTUREMATRIX_T, texT );

	float texGenEnabled[4] = { 0, 0, 0, 0 };
	renderProgManager.SetRenderParm( RENDERPARM_TEXGEN_0_ENABLED, texGenEnabled );

#if 1
	// not really necessary but just for clarity
	const float screenWidth = 1.0f;
	const float screenHeight = 1.0f;
	const float halfScreenWidth = screenWidth * 0.5f;
	const float halfScreenHeight = screenHeight * 0.5f;

	float scale[16] = { 0 };
	scale[0] = w; // scale
	scale[5] = h; // scale			(SRS - changed h from -ve to +ve so video plays right side up)
	scale[12] = halfScreenWidth - ( halfScreenWidth * w ); // translate
	scale[13] = halfScreenHeight - ( halfScreenHeight * h ) - h; // translate (SRS - moved up by h)
	scale[10] = 1.0f;
	scale[15] = 1.0f;

	float ortho[16] = { 0 };
	ortho[0] = 2.0f / screenWidth;
	ortho[5] = -2.0f / screenHeight;
	ortho[10] = -2.0f;
	ortho[12] = -1.0f;
	ortho[13] = 1.0f;
	ortho[14] = -1.0f;
	ortho[15] = 1.0f;

	float finalOrtho[16];
	R_MatrixMultiply( scale, ortho, finalOrtho );

	float projMatrixTranspose[16];
	R_MatrixTranspose( finalOrtho, projMatrixTranspose );
	renderProgManager.SetRenderParms( RENDERPARM_MVPMATRIX_X, projMatrixTranspose, 4 );
#else
	// draw texture over entire screen
	RB_SetMVP( renderMatrix_identity );
#endif

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
		// SRS - Use Bink shader without sRGB to linear conversion, otherwise cinematic colours may be wrong
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
	idImage*	image = NULL;
	int		max;
	float	w, h;

	if( !r_showShadowMaps.GetBool() )
	{
		return;
	}

	image = globalImages->shadowImage[0];
	if( !image )
	{
		return;
	}

	// Set State
	GL_State( GLS_DEPTHFUNC_ALWAYS | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO );

	// Set Parms
	float texS[4] = { 1.0f, 0.0f, 0.0f, 0.0f };
	float texT[4] = { 0.0f, 1.0f, 0.0f, 0.0f };
	renderProgManager.SetRenderParm( RENDERPARM_TEXTUREMATRIX_S, texS );
	renderProgManager.SetRenderParm( RENDERPARM_TEXTUREMATRIX_T, texT );

	float texGenEnabled[4] = { 0, 0, 0, 0 };
	renderProgManager.SetRenderParm( RENDERPARM_TEXGEN_0_ENABLED, texGenEnabled );

	for( int i = 0; i < ( r_shadowMapSplits.GetInteger() + 1 ); i++ )
	{
		max = image->GetUploadWidth() > image->GetUploadHeight() ? image->GetUploadWidth() : image->GetUploadHeight();

		w = 0.25 * image->GetUploadWidth() / max;
		h = 0.25 * image->GetUploadHeight() / max;

		w *= ( float )renderSystem->GetHeight() / renderSystem->GetWidth();

		// not really necessary but just for clarity
		const float screenWidth = 1.0f;
		const float screenHeight = 1.0f;
		const float halfScreenWidth = screenWidth * 0.5f;
		const float halfScreenHeight = screenHeight * 0.5f;

		float scale[16] = { 0 };
		scale[0] = w; // scale
		scale[5] = h; // scale
		scale[12] = ( halfScreenWidth * w * 2.1f * i ); // translate
		scale[13] = halfScreenHeight + ( halfScreenHeight * h ); // translate
		scale[10] = 1.0f;
		scale[15] = 1.0f;

		float ortho[16] = { 0 };
		ortho[0] = 2.0f / screenWidth;
		ortho[5] = -2.0f / screenHeight;
		ortho[10] = -2.0f;
		ortho[12] = -1.0f;
		ortho[13] = 1.0f;
		ortho[14] = -1.0f;
		ortho[15] = 1.0f;

		float finalOrtho[16];
		R_MatrixMultiply( scale, ortho, finalOrtho );

		float projMatrixTranspose[16];
		R_MatrixTranspose( finalOrtho, projMatrixTranspose );
		renderProgManager.SetRenderParms( RENDERPARM_MVPMATRIX_X, projMatrixTranspose, 4 );

		float screenCorrectionParm[4];
		screenCorrectionParm[0] = i;
		screenCorrectionParm[1] = 0.0f;
		screenCorrectionParm[2] = 0.0f;
		screenCorrectionParm[3] = 1.0f;
		renderProgManager.SetRenderParm( RENDERPARM_SCREENCORRECTIONFACTOR, screenCorrectionParm ); // rpScreenCorrectionFactor

		//	glMatrixMode( GL_PROJECTION );
		//	glLoadMatrixf( finalOrtho );
		//	glMatrixMode( GL_MODELVIEW );
		//	glLoadIdentity();

		// Set Color
		GL_Color( 1, 1, 1, 1 );

		GL_SelectTexture( 0 );
		image->Bind();
		glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_MODE, GL_NONE );


		renderProgManager.BindShader_DebugShadowMap();

		DrawElementsWithCounters( &testImageSurface );
	}

	glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE );
}
// RB end

/*
=================
RB_DrawExpandedTriangles
=================
*/
static void RB_DrawExpandedTriangles( const srfTriangles_t* tri, const float radius, const idVec3& vieworg )
{
	int i, j, k;
	idVec3 dir[6], normal, point;

	for( i = 0; i < tri->numIndexes; i += 3 )
	{

		idVec3 p[3] = { tri->verts[ tri->indexes[ i + 0 ] ].xyz, tri->verts[ tri->indexes[ i + 1 ] ].xyz, tri->verts[ tri->indexes[ i + 2 ] ].xyz };

		dir[0] = p[0] - p[1];
		dir[1] = p[1] - p[2];
		dir[2] = p[2] - p[0];

		normal = dir[0].Cross( dir[1] );

		if( normal * p[0] < normal * vieworg )
		{
			continue;
		}

		dir[0] = normal.Cross( dir[0] );
		dir[1] = normal.Cross( dir[1] );
		dir[2] = normal.Cross( dir[2] );

		dir[0].Normalize();
		dir[1].Normalize();
		dir[2].Normalize();

		glBegin( GL_LINE_LOOP );

		for( j = 0; j < 3; j++ )
		{
			k = ( j + 1 ) % 3;

			dir[4] = ( dir[j] + dir[k] ) * 0.5f;
			dir[4].Normalize();

			dir[3] = ( dir[j] + dir[4] ) * 0.5f;
			dir[3].Normalize();

			dir[5] = ( dir[4] + dir[k] ) * 0.5f;
			dir[5].Normalize();

			point = p[k] + dir[j] * radius;
			glVertex3f( point[0], point[1], point[2] );

			point = p[k] + dir[3] * radius;
			glVertex3f( point[0], point[1], point[2] );

			point = p[k] + dir[4] * radius;
			glVertex3f( point[0], point[1], point[2] );

			point = p[k] + dir[5] * radius;
			glVertex3f( point[0], point[1], point[2] );

			point = p[k] + dir[k] * radius;
			glVertex3f( point[0], point[1], point[2] );
		}

		glEnd();
	}
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
	int						i;
	const srfTriangles_t*	tri;
	const drawSurf_t*		surf;
	idVec3					start, end;
	idVec3					localStart, localEnd;
	localTrace_t			hit;
	float					radius;

	if( r_showTrace.GetInteger() == 0 )
	{
		return;
	}

	if( r_showTrace.GetInteger() == 2 )
	{
		radius = 5.0f;
	}
	else
	{
		radius = 0.0f;
	}

	// determine the points of the trace
	start = viewDef->renderView.vieworg;
	end = start + 4000 * viewDef->renderView.viewaxis[0];

	// check and draw the surfaces
	globalImages->whiteImage->Bind();

	// find how many are ambient
	for( i = 0; i < numDrawSurfs; i++ )
	{
		surf = drawSurfs[i];
		tri = surf->frontEndGeo;

		if( tri == NULL || tri->verts == NULL )
		{
			continue;
		}

		// transform the points into local space
		R_GlobalPointToLocal( surf->space->modelMatrix, start, localStart );
		R_GlobalPointToLocal( surf->space->modelMatrix, end, localEnd );

		// check the bounding box
		if( !tri->bounds.Expand( radius ).LineIntersection( localStart, localEnd ) )
		{
			continue;
		}

		glLoadMatrixf( surf->space->modelViewMatrix );

		// highlight the surface
		GL_State( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );

		GL_Color( 1, 0, 0, 0.25 );
		DrawElementsWithCounters( surf );

		// draw the bounding box
		GL_State( GLS_DEPTHFUNC_ALWAYS );

		GL_Color( 1, 1, 1, 1 );
		RB_DrawBounds( tri->bounds );

		if( radius != 0.0f )
		{
			// draw the expanded triangles
			GL_Color( 0.5f, 0.5f, 1.0f, 1.0f );
			RB_DrawExpandedTriangles( tri, radius, localStart );
		}

		// check the exact surfaces
		hit = R_LocalTrace( localStart, localEnd, radius, tri );
		if( hit.fraction < 1.0 )
		{
			GL_Color( 1, 1, 1, 1 );
			RB_DrawBounds( idBounds( hit.point ).Expand( 1 ) );
		}
	}
}

/*
=================
idRenderBackend::DBG_RenderDebugTools
=================
*/
void idRenderBackend::DBG_RenderDebugTools( drawSurf_t** drawSurfs, int numDrawSurfs )
{
	if( viewDef->renderView.rdflags & RDF_IRRADIANCE )
	{
		return;
	}

	// don't do much if this was a 2D rendering
	if( !viewDef->viewEntitys )
	{
		DBG_TestImage();
		DBG_ShowLines();
		return;
	}

	renderLog.OpenMainBlock( MRB_DRAW_DEBUG_TOOLS );
	renderLog.OpenBlock( "Render_DebugTools", colorGreen );

	GL_State( GLS_DEFAULT );

	GL_Scissor( viewDef->viewport.x1 + viewDef->scissor.x1,
				viewDef->viewport.y1 + viewDef->scissor.y1,
				viewDef->scissor.x2 + 1 - viewDef->scissor.x1,
				viewDef->scissor.y2 + 1 - viewDef->scissor.y1 );

	currentScissor = viewDef->scissor;

	DBG_ShowLightCount();
	DBG_ShowTexturePolarity( drawSurfs, numDrawSurfs );
	DBG_ShowTangentSpace( drawSurfs, numDrawSurfs );
	DBG_ShowVertexColor( drawSurfs, numDrawSurfs );
	DBG_ShowTris( drawSurfs, numDrawSurfs );
	DBG_ShowUnsmoothedTangents( drawSurfs, numDrawSurfs );
	DBG_ShowSurfaceInfo( drawSurfs, numDrawSurfs );
	DBG_ShowEdges( drawSurfs, numDrawSurfs );
	DBG_ShowNormals( drawSurfs, numDrawSurfs );
	DBG_ShowViewEntitys( viewDef->viewEntitys );
	DBG_ShowLights();
	// RB begin
	DBG_ShowLightGrid();
	DBG_ShowViewEnvprobes();
	DBG_ShowShadowMapLODs();
	DBG_ShowShadowMaps();
	// RB end

	DBG_ShowTextureVectors( drawSurfs, numDrawSurfs );
	DBG_ShowDominantTris( drawSurfs, numDrawSurfs );
	if( r_testGamma.GetInteger() > 0 )  	// test here so stack check isn't so damn slow on debug builds
	{
		DBG_TestGamma();
	}
	if( r_testGammaBias.GetInteger() > 0 )
	{
		DBG_TestGammaBias();
	}
	DBG_TestImage();
	DBG_ShowPortals();
	DBG_ShowSilhouette();
	DBG_ShowDepthBuffer();
	DBG_ShowIntensity();
	DBG_ShowCenterOfProjection();
	DBG_ShowLines();
	DBG_ShowDebugLines();
	DBG_ShowDebugText();
	DBG_ShowDebugPolygons();
	DBG_ShowTrace( drawSurfs, numDrawSurfs );

	renderLog.CloseBlock();
	renderLog.CloseMainBlock();
}

/*
=================
RB_ShutdownDebugTools
=================
*/
void RB_ShutdownDebugTools()
{
	for( int i = 0; i < MAX_DEBUG_POLYGONS; i++ )
	{
		rb_debugPolygons[i].winding.Clear();
	}
}
