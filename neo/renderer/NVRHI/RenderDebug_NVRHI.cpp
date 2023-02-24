/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2014-2022 Robert Beckebans
Copyright (C) 2016-2017 Dustin Land
Copyright (C) 2022 Stephen Pridham

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
#include "../ImmediateMode.h"

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
#if 0
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
#endif
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

	GL_SelectTexture( 0 );
	globalImages->whiteImage->Bind();

	RB_SetVertexColorParms( SVC_MODULATE );

	renderProgManager.BindShader_TextureVertexColor();
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
							   0.35f, colorYellow, tr.primaryView->renderView.viewaxis );
	tr.primaryWorld->DrawText( surfMatName, surfPoint,
							   0.35f, colorCyan, tr.primaryView->renderView.viewaxis );
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
			GL_State( GLS_DEPTHFUNC_ALWAYS | GLS_POLYMODE_LINE | GLS_DEPTHMASK );
			//GL_State( GLS_DEPTHFUNC_ALWAYS | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHMASK );

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
	GL_State( GLS_POLYMODE_LINE | GLS_DEPTHFUNC_ALWAYS );

	idRenderWorldLocal& world = *viewDef->renderWorld;

	fhImmediateMode im( tr.backend.GL_GetCommandList() );

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

			im.Begin( GFX_LINES );
			int j = 0;
			for( ; j < w->GetNumPoints(); j++ )
			{
				// draw a triangle for each line
				if( j >= 1 )
				{
					im.Vertex3fv( ( *w )[ j - 1 ].ToFloatPtr() );
					im.Vertex3fv( ( *w )[ j ].ToFloatPtr() );
					im.Vertex3fv( ( *w )[ j ].ToFloatPtr() );
				}
			}

			im.Vertex3fv( ( *w )[ 0 ].ToFloatPtr() );
			im.Vertex3fv( ( *w )[ 0 ].ToFloatPtr() );
			im.Vertex3fv( ( *w )[ j - 1 ].ToFloatPtr() );

			im.End();
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
	renderProgManager.CommitUniforms( tr.backend.GL_GetCurrentState() );

	fhImmediateMode im( tr.backend.GL_GetCommandList() );

	int i, j, len, num, index, charIndex, line;
	float textLen = 1.0f, spacing = 1.0f;
	idVec3 org, p1, p2;

	if( text && *text )
	{
		im.Begin( GFX_LINES );
		//im.Color3fv( color.ToFloatPtr() );

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

				im.Vertex3fv( p1.ToFloatPtr() );
				im.Vertex3fv( p2.ToFloatPtr() );
				im.Vertex3fv( p2.ToFloatPtr() ); // RB: just build a triangle of this line
			}
			org -= viewAxis[1] * ( spacing * scale );
		}

		im.End();
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
	debugText_t*	text;

	if( !rb_numDebugText )
	{
		return;
	}

	// all lines are expressed in world coordinates
	DBG_SimpleWorldSetup();

	/*
	int width = r_debugLineWidth.GetInteger();
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
	*/

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

	//glLineWidth( 1 );
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
	debugLine_t*	line;

	if( !rb_numDebugLines )
	{
		return;
	}

	// all lines are expressed in world coordinates
	DBG_SimpleWorldSetup();

	renderProgManager.BindShader_VertexColor();
	renderProgManager.CommitUniforms( glStateBits );

	/*
	int width = r_debugLineWidth.GetInteger();
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
	*/

	if( !r_debugLineDepthTest.GetBool() )
	{
		GL_State( GLS_POLYMODE_LINE | GLS_DEPTHFUNC_ALWAYS );
	}
	else
	{
		GL_State( GLS_POLYMODE_LINE );
	}

	fhImmediateMode im( tr.backend.GL_GetCommandList() );

	im.Begin( GFX_LINES );
	line = rb_debugLines;
	for( i = 0; i < rb_numDebugLines; i++, line++ )
	{
		if( !line->depthTest )
		{
			im.Color3fv( line->rgb.ToFloatPtr() );

			im.Vertex3fv( line->start.ToFloatPtr() );
			im.Vertex3fv( line->end.ToFloatPtr() );
			im.Vertex3fv( line->end.ToFloatPtr() );
		}
	}
	im.End();

	if( !r_debugLineDepthTest.GetBool() )
	{
		GL_State( GLS_POLYMODE_LINE );
	}

	im.Begin( GFX_LINES );
	line = rb_debugLines;
	for( i = 0; i < rb_numDebugLines; i++, line++ )
	{
		if( line->depthTest )
		{
			im.Color4fv( line->rgb.ToFloatPtr() );

			im.Vertex3fv( line->start.ToFloatPtr() );
			im.Vertex3fv( line->end.ToFloatPtr() );

			// RB: could use nvrhi::PrimitiveType::LineList
			// but we rather to keep the number of pipelines low so just make a triangle of this
			im.Vertex3fv( line->end.ToFloatPtr() );

		}
	}

	im.End();

	//glLineWidth( 1 );
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
	idImage* image   = NULL;
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

		// SRS - Don't need calibrated time for testing cinematics, so just call ImageForTime( ) with current system time
		// This simplification allows cinematic test playback to work over both 2D and 3D background scenes
		cin = tr.testVideo->ImageForTime( Sys_Milliseconds() /*viewDef->renderView.time[1] - tr.testVideoStartTime*/, commandList );
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
	scale[5] = h; // scale
	scale[10] = 1.0f;
	scale[12] = halfScreenWidth - ( halfScreenWidth * w ); 			// translate to center x
	scale[13] = halfScreenHeight / 2 - ( halfScreenHeight * h );	// translate to center y of console dropdown
	scale[14] = -0.5f;												// translate to center z
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
