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

#include "precompiled.h"
#pragma hdrstop

#include "RenderCommon.h"

// if we hit this many planes, we will just stop cropping the
// view down, which is still correct, just conservative
const int MAX_PORTAL_PLANES	= 20;

struct portalStack_t
{
	const portal_t* 		p;
	const portalStack_t* 	next;
	// positive side is outside the visible frustum
	int						numPortalPlanes;
	idPlane					portalPlanes[MAX_PORTAL_PLANES + 1];
	idScreenRect			rect;
};

/*
=======================================================================

Create viewLights and viewEntitys for the lights and entities that are
visible in the portal areas that can be seen from the current viewpoint.

=======================================================================
*/

/*
=============
R_SetLightDefViewLight

If the lightDef is not already on the viewLight list, create
a viewLight and add it to the list with an empty scissor rect.
=============
*/
viewLight_t* R_SetLightDefViewLight( idRenderLightLocal* light )
{
	if( light->viewCount == tr.viewCount )
	{
		// already set up for this frame
		return light->viewLight;
	}
	light->viewCount = tr.viewCount;

	// add to the view light chain
	viewLight_t* vLight = ( viewLight_t* )R_ClearedFrameAlloc( sizeof( *vLight ), FRAME_ALLOC_VIEW_LIGHT );
	vLight->lightDef = light;

	// the scissorRect will be expanded as the light bounds is accepted into visible portal chains
	// and the scissor will be reduced in R_AddSingleLight based on the screen space projection
	vLight->scissorRect.Clear();

	// link the view light
	vLight->next = tr.viewDef->viewLights;
	tr.viewDef->viewLights = vLight;

	light->viewLight = vLight;

	return vLight;
}

/*
=============
R_SetEntityDefViewEntity

If the entityDef is not already on the viewEntity list, create
a viewEntity and add it to the list with an empty scissor rect.
=============
*/
viewEntity_t* R_SetEntityDefViewEntity( idRenderEntityLocal* def )
{
	if( def->viewCount == tr.viewCount )
	{
		// already set up for this frame
		return def->viewEntity;
	}
	def->viewCount = tr.viewCount;

	viewEntity_t* vModel = ( viewEntity_t* )R_ClearedFrameAlloc( sizeof( *vModel ), FRAME_ALLOC_VIEW_ENTITY );
	vModel->entityDef = def;

	// the scissorRect will be expanded as the model bounds is accepted into visible portal chains
	// It will remain clear if the model is only needed for shadows.
	vModel->scissorRect.Clear();

	vModel->next = tr.viewDef->viewEntitys;
	tr.viewDef->viewEntitys = vModel;

	def->viewEntity = vModel;

	return vModel;
}

/*
================
CullEntityByPortals

Return true if the entity reference bounds do not intersect the current portal chain.
================
*/
bool idRenderWorldLocal::CullEntityByPortals( const idRenderEntityLocal* entity, const portalStack_t* ps )
{
	if( r_useEntityPortalCulling.GetInteger() == 1 )
	{

		ALIGNTYPE16 frustumCorners_t corners;
		idRenderMatrix::GetFrustumCorners( corners, entity->inverseBaseModelProject, bounds_unitCube );
		for( int i = 0; i < ps->numPortalPlanes; i++ )
		{
			if( idRenderMatrix::CullFrustumCornersToPlane( corners, ps->portalPlanes[i] ) == FRUSTUM_CULL_FRONT )
			{
				return true;
			}
		}

	}
	else if( r_useEntityPortalCulling.GetInteger() >= 2 )
	{

		idRenderMatrix baseModelProject;
		idRenderMatrix::Inverse( entity->inverseBaseModelProject, baseModelProject );

		idPlane frustumPlanes[6];
		idRenderMatrix::GetFrustumPlanes( frustumPlanes, baseModelProject, false, true );

		// exact clip of light faces against all planes
		for( int i = 0; i < 6; i++ )
		{
			// the entity frustum planes face inward, so the planes that have the
			// view origin on the positive side will be the "back" faces of the entity,
			// which must have some fragment inside the portal stack planes to be visible
			if( frustumPlanes[i].Distance( tr.viewDef->renderView.vieworg ) <= 0.0f )
			{
				continue;
			}

			// calculate a winding for this frustum side
			idFixedWinding w;
			w.BaseForPlane( frustumPlanes[i] );
			for( int j = 0; j < 6; j++ )
			{
				if( j == i )
				{
					continue;
				}
				if( !w.ClipInPlace( frustumPlanes[j], ON_EPSILON ) )
				{
					break;
				}
			}
			if( w.GetNumPoints() <= 2 )
			{
				continue;
			}

			assert( ps->numPortalPlanes <= MAX_PORTAL_PLANES );
			assert( w.GetNumPoints() + ps->numPortalPlanes < MAX_POINTS_ON_WINDING );

			// now clip the winding against each of the portalStack planes
			// skip the last plane which is the last portal itself
			for( int j = 0; j < ps->numPortalPlanes - 1; j++ )
			{
				if( !w.ClipInPlace( -ps->portalPlanes[j], ON_EPSILON ) )
				{
					break;
				}
			}

			if( w.GetNumPoints() > 2 )
			{
				// part of the winding is visible through the portalStack,
				// so the entity is not culled
				return false;
			}
		}

		// nothing was visible
		return true;

	}

	return false;
}

/*
===================
AddAreaViewEntities

Any models that are visible through the current portalStack will have their scissor rect updated.
===================
*/
void idRenderWorldLocal::AddAreaViewEntities( int areaNum, const portalStack_t* ps )
{
	portalArea_t* area = &portalAreas[ areaNum ];

	for( areaReference_t* ref = area->entityRefs.areaNext; ref != &area->entityRefs; ref = ref->areaNext )
	{
		idRenderEntityLocal*	 entity = ref->entity;

		// debug tool to allow viewing of only one entity at a time
		if( r_singleEntity.GetInteger() >= 0 && r_singleEntity.GetInteger() != entity->index )
		{
			continue;
		}

		// remove decals that are completely faded away
		R_FreeEntityDefFadedDecals( entity, tr.viewDef->renderView.time[0] );

		// check for completely suppressing the model
		if( !r_skipSuppress.GetBool() )
		{
			if( entity->parms.suppressSurfaceInViewID
					&& entity->parms.suppressSurfaceInViewID == tr.viewDef->renderView.viewID )
			{
				continue;
			}
			if( entity->parms.allowSurfaceInViewID
					&& entity->parms.allowSurfaceInViewID != tr.viewDef->renderView.viewID )
			{
				continue;
			}
		}

		// cull reference bounds
		if( CullEntityByPortals( entity, ps ) )
		{
			// we are culled out through this portal chain, but it might
			// still be visible through others
			continue;
		}

		viewEntity_t* vEnt = R_SetEntityDefViewEntity( entity );

		// possibly expand the scissor rect
		vEnt->scissorRect.Union( ps->rect );
	}
}

/*
================
CullLightByPortals

Return true if the light frustum does not intersect the current portal chain.
================
*/
bool idRenderWorldLocal::CullLightByPortals( const idRenderLightLocal* light, const portalStack_t* ps )
{
	if( r_useLightPortalCulling.GetInteger() == 1 )
	{

		ALIGNTYPE16 frustumCorners_t corners;
		idRenderMatrix::GetFrustumCorners( corners, light->inverseBaseLightProject, bounds_zeroOneCube );
		for( int i = 0; i < ps->numPortalPlanes; i++ )
		{
			if( idRenderMatrix::CullFrustumCornersToPlane( corners, ps->portalPlanes[i] ) == FRUSTUM_CULL_FRONT )
			{
				return true;
			}
		}

	}
	else if( r_useLightPortalCulling.GetInteger() >= 2 )
	{

		idPlane frustumPlanes[6];
		idRenderMatrix::GetFrustumPlanes( frustumPlanes, light->baseLightProject, true, true );

		// exact clip of light faces against all planes
		for( int i = 0; i < 6; i++ )
		{
			// the light frustum planes face inward, so the planes that have the
			// view origin on the positive side will be the "back" faces of the light,
			// which must have some fragment inside the the portal stack planes to be visible
			if( frustumPlanes[i].Distance( tr.viewDef->renderView.vieworg ) <= 0.0f )
			{
				continue;
			}

			// calculate a winding for this frustum side
			idFixedWinding w;
			w.BaseForPlane( frustumPlanes[i] );
			for( int j = 0; j < 6; j++ )
			{
				if( j == i )
				{
					continue;
				}
				if( !w.ClipInPlace( frustumPlanes[j], ON_EPSILON ) )
				{
					break;
				}
			}
			if( w.GetNumPoints() <= 2 )
			{
				continue;
			}

			assert( ps->numPortalPlanes <= MAX_PORTAL_PLANES );
			assert( w.GetNumPoints() + ps->numPortalPlanes < MAX_POINTS_ON_WINDING );

			// now clip the winding against each of the portalStack planes
			// skip the last plane which is the last portal itself
			for( int j = 0; j < ps->numPortalPlanes - 1; j++ )
			{
				if( !w.ClipInPlace( -ps->portalPlanes[j], ON_EPSILON ) )
				{
					break;
				}
			}

			if( w.GetNumPoints() > 2 )
			{
				// part of the winding is visible through the portalStack,
				// so the light is not culled
				return false;
			}
		}

		// nothing was visible
		return true;
	}

	return false;
}

/*
===================
AddAreaViewLights

This is the only point where lights get added to the viewLights list.
Any lights that are visible through the current portalStack will have their scissor rect updated.
===================
*/
void idRenderWorldLocal::AddAreaViewLights( int areaNum, const portalStack_t* ps )
{
	portalArea_t* area = &portalAreas[ areaNum ];

	for( areaReference_t* lref = area->lightRefs.areaNext; lref != &area->lightRefs; lref = lref->areaNext )
	{
		idRenderLightLocal* light = lref->light;

		// debug tool to allow viewing of only one light at a time
		// RB: use this elsewhere in the backend debug drawing code
#if 0
		if( r_singleLight.GetInteger() >= 0 && r_singleLight.GetInteger() != light->index )
		{
			continue;
		}
#endif

		// check for being closed off behind a door
		// a light that doesn't cast shadows will still light even if it is behind a door
		if( r_useLightAreaCulling.GetBool() && !light->LightCastsShadows()
				&& light->areaNum != -1 && !tr.viewDef->connectedAreas[ light->areaNum ] )
		{
			continue;
		}

		// cull frustum
		if( CullLightByPortals( light, ps ) )
		{
			// we are culled out through this portal chain, but it might
			// still be visible through others
			continue;
		}

		viewLight_t* vLight = R_SetLightDefViewLight( light );

		// expand the scissor rect
		vLight->scissorRect.Union( ps->rect );
	}
}

/*
===================
AddAreaToView

This may be entered multiple times with different planes
if more than one portal sees into the area
===================
*/
void idRenderWorldLocal::AddAreaToView( int areaNum, const portalStack_t* ps )
{
	// mark the viewCount, so r_showPortals can display the considered portals
	portalAreas[ areaNum ].viewCount = tr.viewCount;

	// add the models and lights, using more precise culling to the planes
	AddAreaViewEntities( areaNum, ps );
	AddAreaViewLights( areaNum, ps );
	AddAreaViewEnvprobes( areaNum, ps ); // RB
}

/*
===================
idRenderWorldLocal::ScreenRectForWinding
===================
*/
idScreenRect idRenderWorldLocal::ScreenRectFromWinding( const idWinding* w, const viewEntity_t* space )
{
	const float viewWidth = ( float ) tr.viewDef->viewport.x2 - ( float ) tr.viewDef->viewport.x1;
	const float viewHeight = ( float ) tr.viewDef->viewport.y2 - ( float ) tr.viewDef->viewport.y1;

	idScreenRect r;
	r.Clear();
	for( int i = 0; i < w->GetNumPoints(); i++ )
	{
		idVec3 v;
		idVec3 ndc;
		R_LocalPointToGlobal( space->modelMatrix, ( *w )[i].ToVec3(), v );
		R_GlobalToNormalizedDeviceCoordinates( v, ndc );

		float windowX = ( ndc[0] * 0.5f + 0.5f ) * viewWidth;
		float windowY = ( ndc[1] * 0.5f + 0.5f ) * viewHeight;

		r.AddPoint( windowX, windowY );
	}

	r.Expand();

	return r;
}

/*
===================
idRenderWorldLocal::PortalIsFoggedOut
===================
*/
bool idRenderWorldLocal::PortalIsFoggedOut( const portal_t* p )
{
	idRenderLightLocal* ldef = p->doublePortal->fogLight;
	if( ldef == NULL )
	{
		return false;
	}

	// find the current density of the fog
	const idMaterial* lightShader = ldef->lightShader;
	const int size = lightShader->GetNumRegisters() * sizeof( float );
	float* regs = ( float* )_alloca( size );

	lightShader->EvaluateRegisters( regs, ldef->parms.shaderParms,
									tr.viewDef->renderView.shaderParms, tr.viewDef->renderView.time[0] * 0.001f, ldef->parms.referenceSound );

	const shaderStage_t*	stage = lightShader->GetStage( 0 );

	const float alpha = regs[ stage->color.registers[3] ];

	// if they left the default value on, set a fog distance of 500
	float a;
	if( alpha <= 1.0f )
	{
		a = -0.5f / DEFAULT_FOG_DISTANCE;
	}
	else
	{
		// otherwise, distance = alpha color
		a = -0.5f / alpha;
	}

	idPlane forward;
	forward[0] = a * tr.viewDef->worldSpace.modelViewMatrix[0 * 4 + 2];
	forward[1] = a * tr.viewDef->worldSpace.modelViewMatrix[1 * 4 + 2];
	forward[2] = a * tr.viewDef->worldSpace.modelViewMatrix[2 * 4 + 2];
	forward[3] = a * tr.viewDef->worldSpace.modelViewMatrix[3 * 4 + 2];

	const idWinding*	 w = p->w;
	for( int i = 0; i < w->GetNumPoints(); i++ )
	{
		const float d = forward.Distance( ( *w )[i].ToVec3() );
		if( d < 0.5f )
		{
			return false;		// a point not clipped off
		}
	}

	return true;
}

/*
===================
idRenderWorldLocal::FloodViewThroughArea_r
===================
*/
void idRenderWorldLocal::FloodViewThroughArea_r( const idVec3& origin, int areaNum, const portalStack_t* ps )
{
	portalArea_t* area = &portalAreas[ areaNum ];

	// cull models and lights to the current collection of planes
	AddAreaToView( areaNum, ps );

	if( areaScreenRect[areaNum].IsEmpty() )
	{
		areaScreenRect[areaNum] = ps->rect;
	}
	else
	{
		areaScreenRect[areaNum].Union( ps->rect );
	}

	// go through all the portals
	for( const portal_t* p = area->portals; p != NULL; p = p->next )
	{
		// an enclosing door may have sealed the portal off
		if( p->doublePortal->blockingBits & PS_BLOCK_VIEW )
		{
			continue;
		}

		// make sure this portal is facing away from the view
		const float d = p->plane.Distance( origin );
		if( d < -0.1f )
		{
			continue;
		}

		// make sure the portal isn't in our stack trace,
		// which would cause an infinite loop
		const portalStack_t* check = ps;
		for( ; check != NULL; check = check->next )
		{
			if( check->p == p )
			{
				break;		// don't recursively enter a stack
			}
		}
		if( check )
		{
			continue;	// already in stack
		}

		// if we are very close to the portal surface, don't bother clipping
		// it, which tends to give epsilon problems that make the area vanish
		if( d < 1.0f )
		{

			// go through this portal
			portalStack_t newStack;
			newStack = *ps;
			newStack.p = p;
			newStack.next = ps;
			FloodViewThroughArea_r( origin, p->intoArea, &newStack );
			continue;
		}

		// clip the portal winding to all of the planes
		idFixedWinding w;		// we won't overflow because MAX_PORTAL_PLANES = 20
		w = *p->w;
		for( int j = 0; j < ps->numPortalPlanes; j++ )
		{
			if( !w.ClipInPlace( -ps->portalPlanes[j], 0 ) )
			{
				break;
			}
		}
		if( !w.GetNumPoints() )
		{
			continue;	// portal not visible
		}

		// see if it is fogged out
		if( PortalIsFoggedOut( p ) )
		{
			continue;
		}

		// go through this portal
		portalStack_t newStack;
		newStack.p = p;
		newStack.next = ps;

		// find the screen pixel bounding box of the remaining portal
		// so we can scissor things outside it
		newStack.rect = ScreenRectFromWinding( &w, &tr.identitySpace );

		// slop might have spread it a pixel outside, so trim it back
		newStack.rect.Intersect( ps->rect );

		// generate a set of clipping planes that will further restrict
		// the visible view beyond just the scissor rect

		int addPlanes = w.GetNumPoints();
		if( addPlanes > MAX_PORTAL_PLANES )
		{
			addPlanes = MAX_PORTAL_PLANES;
		}

		newStack.numPortalPlanes = 0;
		for( int i = 0; i < addPlanes; i++ )
		{
			int j = i + 1;
			if( j == w.GetNumPoints() )
			{
				j = 0;
			}

			const idVec3& v1 = origin - w[i].ToVec3();
			const idVec3& v2 = origin - w[j].ToVec3();

			newStack.portalPlanes[newStack.numPortalPlanes].Normal().Cross( v2, v1 );

			// if it is degenerate, skip the plane
			if( newStack.portalPlanes[newStack.numPortalPlanes].Normalize() < 0.01f )
			{
				continue;
			}
			newStack.portalPlanes[newStack.numPortalPlanes].FitThroughPoint( origin );

			newStack.numPortalPlanes++;
		}

		// the last stack plane is the portal plane
		newStack.portalPlanes[newStack.numPortalPlanes] = p->plane;
		newStack.numPortalPlanes++;

		FloodViewThroughArea_r( origin, p->intoArea, &newStack );
	}
}

/*
=======================
idRenderWorldLocal::FlowViewThroughPortals

Finds viewLights and viewEntities by flowing from an origin through the visible
portals that the origin point can see into. The planes array defines a volume with
the planes pointing outside the volume. Zero planes assumes an unbounded volume.
=======================
*/
void idRenderWorldLocal::FlowViewThroughPortals( const idVec3& origin, int numPlanes, const idPlane* planes )
{
	portalStack_t ps;
	ps.next = NULL;
	ps.p = NULL;

	assert( numPlanes <= MAX_PORTAL_PLANES );
	for( int i = 0; i < numPlanes; i++ )
	{
		ps.portalPlanes[i] = planes[i];
	}

	ps.numPortalPlanes = numPlanes;
	ps.rect = tr.viewDef->scissor;

	// if outside the world, mark everything
	if( tr.viewDef->areaNum < 0 )
	{
		for( int i = 0; i < numPortalAreas; i++ )
		{
			areaScreenRect[i] = tr.viewDef->scissor;
			AddAreaToView( i, &ps );
		}
	}
	else
	{
		// flood out through portals, setting area viewCount
		FloodViewThroughArea_r( origin, tr.viewDef->areaNum, &ps );
	}
}

/*
===================
idRenderWorldLocal::BuildConnectedAreas_r
===================
*/
void idRenderWorldLocal::BuildConnectedAreas_r( int areaNum )
{
	if( tr.viewDef->connectedAreas[areaNum] )
	{
		return;
	}

	tr.viewDef->connectedAreas[areaNum] = true;

	// flood through all non-blocked portals
	portalArea_t* area = &portalAreas[ areaNum ];
	for( const portal_t* portal = area->portals; portal; portal = portal->next )
	{
		if( ( portal->doublePortal->blockingBits & PS_BLOCK_VIEW ) == 0 )
		{
			BuildConnectedAreas_r( portal->intoArea );
		}
	}
}

/*
===================
idRenderWorldLocal::BuildConnectedAreas

This is only valid for a given view, not all views in a frame
===================
*/
void idRenderWorldLocal::BuildConnectedAreas()
{
	tr.viewDef->connectedAreas = ( bool* )R_FrameAlloc( numPortalAreas * sizeof( tr.viewDef->connectedAreas[0] ) );

	// if we are outside the world, we can see all areas
	if( tr.viewDef->areaNum == -1 )
	{
		for( int i = 0; i < numPortalAreas; i++ )
		{
			tr.viewDef->connectedAreas[i] = true;
		}
		return;
	}

	// start with none visible, and flood fill from the current area
	memset( tr.viewDef->connectedAreas, 0, numPortalAreas * sizeof( tr.viewDef->connectedAreas[0] ) );
	BuildConnectedAreas_r( tr.viewDef->areaNum );
}

/*
=============
idRenderWorldLocal::FindViewLightsAndEntites

All the modelrefs and lightrefs that are in visible areas
will have viewEntitys and viewLights created for them.

The scissorRects on the viewEntitys and viewLights may be empty if
they were considered, but not actually visible.

Entities and lights can have cached viewEntities / viewLights that
will be used if the viewCount variable matches.
=============
*/
void idRenderWorldLocal::FindViewLightsAndEntities()
{
	SCOPED_PROFILE_EVENT( "FindViewLightsAndEntities" );

	// bumping this counter invalidates cached viewLights / viewEntities,
	// when a light or entity is next considered, it will create a new
	// viewLight / viewEntity
	tr.viewCount++;

	// clear the visible lightDef and entityDef lists
	tr.viewDef->viewLights = NULL;
	tr.viewDef->viewEntitys = NULL;
	tr.viewDef->viewEnvprobes = NULL; // RB

	// all areas are initially not visible, but each portal
	// chain that leads to them will expand the visible rectangle
	for( int i = 0; i < numPortalAreas; i++ )
	{
		areaScreenRect[i].Clear();
	}

	// find the area to start the portal flooding in
	if( !r_usePortals.GetBool() )
	{
		// debug tool to force no portal culling
		tr.viewDef->areaNum = -1;
	}
	else
	{
		tr.viewDef->areaNum = PointInArea( tr.viewDef->initialViewAreaOrigin );
	}

	// determine all possible connected areas for
	// light-behind-door culling
	BuildConnectedAreas();

	// flow through all the portals and add models / lights
	if( r_singleArea.GetBool() )
	{
		// if debugging, only mark this area
		// if we are outside the world, don't draw anything
		if( tr.viewDef->areaNum >= 0 )
		{
			static int lastPrintedAreaNum;
			if( tr.viewDef->areaNum != lastPrintedAreaNum )
			{
				lastPrintedAreaNum = tr.viewDef->areaNum;
				common->Printf( "entering portal area %i\n", tr.viewDef->areaNum );
			}

			portalStack_t ps;
			for( int i = 0; i < 5; i++ )
			{
				ps.portalPlanes[i] = tr.viewDef->frustums[FRUSTUM_PRIMARY][i];
			}
			ps.numPortalPlanes = 5;
			ps.rect = tr.viewDef->scissor;

			AddAreaToView( tr.viewDef->areaNum, &ps );
		}
	}
	else
	{
		// note that the center of projection for flowing through portals may
		// be a different point than initialViewAreaOrigin for subviews that
		// may have the viewOrigin in a solid/invalid area
		FlowViewThroughPortals( tr.viewDef->renderView.vieworg, 5, tr.viewDef->frustums[FRUSTUM_PRIMARY] );
	}
}

/*
=======================================================================

Light linking into the BSP tree by flooding through portals

=======================================================================
*/

/*
===================
idRenderWorldLocal::FloodLightThroughArea_r
===================
*/
void idRenderWorldLocal::FloodLightThroughArea_r( idRenderLightLocal* light, int areaNum,
		const portalStack_t* ps )
{
	assert( ps != NULL ); // compiler warning
	portal_t*		p = NULL;
	float			d;
	portalArea_t* 	area = NULL;
	const portalStack_t*	check = NULL, *firstPortalStack = NULL;
	portalStack_t	newStack;
	int				i, j;
	idVec3			v1, v2;
	int				addPlanes;
	idFixedWinding	w;		// we won't overflow because MAX_PORTAL_PLANES = 20

	area = &portalAreas[ areaNum ];

	// add an areaRef
	AddLightRefToArea( light, area );

	// go through all the portals
	for( p = area->portals; p; p = p->next )
	{
		// make sure this portal is facing away from the view
		d = p->plane.Distance( light->globalLightOrigin );
		if( d < -0.1f )
		{
			continue;
		}

		// make sure the portal isn't in our stack trace,
		// which would cause an infinite loop
		for( check = ps; check; check = check->next )
		{
			firstPortalStack = check;
			if( check->p == p )
			{
				break;		// don't recursively enter a stack
			}
		}
		if( check )
		{
			continue;	// already in stack
		}

		// if we are very close to the portal surface, don't bother clipping
		// it, which tends to give epsilon problems that make the area vanish
		if( d < 1.0f )
		{
			// go through this portal
			newStack = *ps;
			newStack.p = p;
			newStack.next = ps;
			FloodLightThroughArea_r( light, p->intoArea, &newStack );
			continue;
		}

		// clip the portal winding to all of the planes
		w = *p->w;
		for( j = 0; j < ps->numPortalPlanes; j++ )
		{
			if( !w.ClipInPlace( -ps->portalPlanes[j], 0 ) )
			{
				break;
			}
		}
		if( !w.GetNumPoints() )
		{
			continue;	// portal not visible
		}
		// also always clip to the original light planes, because they aren't
		// necessarily extending to infinitiy like a view frustum
		for( j = 0; j < firstPortalStack->numPortalPlanes; j++ )
		{
			if( !w.ClipInPlace( -firstPortalStack->portalPlanes[j], 0 ) )
			{
				break;
			}
		}
		if( !w.GetNumPoints() )
		{
			continue;	// portal not visible
		}

		// go through this portal
		newStack.p = p;
		newStack.next = ps;

		// generate a set of clipping planes that will further restrict
		// the visible view beyond just the scissor rect

		addPlanes = w.GetNumPoints();
		if( addPlanes > MAX_PORTAL_PLANES )
		{
			addPlanes = MAX_PORTAL_PLANES;
		}

		newStack.numPortalPlanes = 0;
		for( i = 0; i < addPlanes; i++ )
		{
			j = i + 1;
			if( j == w.GetNumPoints() )
			{
				j = 0;
			}

			v1 = light->globalLightOrigin - w[i].ToVec3();
			v2 = light->globalLightOrigin - w[j].ToVec3();

			newStack.portalPlanes[newStack.numPortalPlanes].Normal().Cross( v2, v1 );

			// if it is degenerate, skip the plane
			if( newStack.portalPlanes[newStack.numPortalPlanes].Normalize() < 0.01f )
			{
				continue;
			}
			newStack.portalPlanes[newStack.numPortalPlanes].FitThroughPoint( light->globalLightOrigin );

			newStack.numPortalPlanes++;
		}

		FloodLightThroughArea_r( light, p->intoArea, &newStack );
	}
}

/*
=======================
idRenderWorldLocal::FlowLightThroughPortals

Adds an arearef in each area that the light center flows into.
This can only be used for shadow casting lights that have a generated
prelight, because shadows are cast from back side which may not be in visible areas.
=======================
*/
void idRenderWorldLocal::FlowLightThroughPortals( idRenderLightLocal* light )
{
	// if the light origin areaNum is not in a valid area,
	// the light won't have any area refs
	if( light->areaNum == -1 )
	{
		return;
	}

	idPlane frustumPlanes[6];
	idRenderMatrix::GetFrustumPlanes( frustumPlanes, light->baseLightProject, true, true );

	portalStack_t ps;
	memset( &ps, 0, sizeof( ps ) );
	ps.numPortalPlanes = 6;
	for( int i = 0; i < 6; i++ )
	{
		ps.portalPlanes[i] = -frustumPlanes[i];
	}

	FloodLightThroughArea_r( light, light->areaNum, &ps );
}

/*
=======================================================================

Portal State Management

=======================================================================
*/

/*
==============
NumPortals
==============
*/
int idRenderWorldLocal::NumPortals() const
{
	return numInterAreaPortals;
}

/*
==============
FindPortal

Game code uses this to identify which portals are inside doors.
Returns 0 if no portal contacts the bounds
==============
*/
qhandle_t idRenderWorldLocal::FindPortal( const idBounds& b ) const
{
	int				i, j;
	idBounds		wb;
	doublePortal_t*	portal;
	idWinding*		w;

	for( i = 0; i < numInterAreaPortals; i++ )
	{
		portal = &doublePortals[i];
		w = portal->portals[0]->w;

		wb.Clear();
		for( j = 0; j < w->GetNumPoints(); j++ )
		{
			wb.AddPoint( ( *w )[j].ToVec3() );
		}
		if( wb.IntersectsBounds( b ) )
		{
			return i + 1;
		}
	}

	return 0;
}

/*
=============
FloodConnectedAreas
=============
*/
void idRenderWorldLocal::FloodConnectedAreas( portalArea_t* area, int portalAttributeIndex )
{
	if( area->connectedAreaNum[portalAttributeIndex] == connectedAreaNum )
	{
		return;
	}
	area->connectedAreaNum[portalAttributeIndex] = connectedAreaNum;

	for( portal_t* p = area->portals; p != NULL; p = p->next )
	{
		if( !( p->doublePortal->blockingBits & ( 1 << portalAttributeIndex ) ) )
		{
			FloodConnectedAreas( &portalAreas[p->intoArea], portalAttributeIndex );
		}
	}
}

/*
==============
AreasAreConnected
==============
*/
bool idRenderWorldLocal::AreasAreConnected( int areaNum1, int areaNum2, portalConnection_t connection ) const
{
	if( areaNum1 == -1 || areaNum2 == -1 )
	{
		return false;
	}
	if( areaNum1 > numPortalAreas || areaNum2 > numPortalAreas || areaNum1 < 0 || areaNum2 < 0 )
	{
		common->Error( "idRenderWorldLocal::AreAreasConnected: bad parms: %i, %i", areaNum1, areaNum2 );
	}

	int	attribute = 0;

	int	intConnection = ( int )connection;

	while( intConnection > 1 )
	{
		attribute++;
		intConnection >>= 1;
	}
	if( attribute >= NUM_PORTAL_ATTRIBUTES || ( 1 << attribute ) != ( int )connection )
	{
		common->Error( "idRenderWorldLocal::AreasAreConnected: bad connection number: %i\n", ( int )connection );
	}

	return portalAreas[areaNum1].connectedAreaNum[attribute] == portalAreas[areaNum2].connectedAreaNum[attribute];
}

/*
==============
SetPortalState

doors explicitly close off portals when shut
==============
*/
void idRenderWorldLocal::SetPortalState( qhandle_t portal, int blockTypes )
{
	if( portal == 0 )
	{
		return;
	}

	if( portal < 1 || portal > numInterAreaPortals )
	{
		common->Error( "SetPortalState: bad portal number %i", portal );
	}
	int	old = doublePortals[portal - 1].blockingBits;
	if( old == blockTypes )
	{
		return;
	}
	doublePortals[portal - 1].blockingBits = blockTypes;

	// leave the connectedAreaGroup the same on one side,
	// then flood fill from the other side with a new number for each changed attribute
	for( int i = 0; i < NUM_PORTAL_ATTRIBUTES; i++ )
	{
		if( ( old ^ blockTypes ) & ( 1 << i ) )
		{
			connectedAreaNum++;
			FloodConnectedAreas( &portalAreas[doublePortals[portal - 1].portals[1]->intoArea], i );
		}
	}

	if( common->WriteDemo() )
	{
		common->WriteDemo()->WriteInt( DS_RENDER );
		common->WriteDemo()->WriteInt( DC_SET_PORTAL_STATE );
		common->WriteDemo()->WriteInt( portal );
		common->WriteDemo()->WriteInt( blockTypes );
	}
}

/*
==============
GetPortalState
==============
*/
int idRenderWorldLocal::GetPortalState( qhandle_t portal )
{
	if( portal == 0 )
	{
		return 0;
	}

	if( portal < 1 || portal > numInterAreaPortals )
	{
		common->Error( "GetPortalState: bad portal number %i", portal );
	}

	return doublePortals[portal - 1].blockingBits;
}

