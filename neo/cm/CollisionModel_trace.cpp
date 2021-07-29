/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.

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

/*
===============================================================================

	Trace model vs. polygonal model collision detection.

===============================================================================
*/

#include "precompiled.h"
#pragma hdrstop

#include "CollisionModel_local.h"

/*
===============================================================================

Trace through the spatial subdivision

===============================================================================
*/

/*
================
idCollisionModelManagerLocal::TraceTrmThroughNode
================
*/
void idCollisionModelManagerLocal::TraceTrmThroughNode( cm_traceWork_t* tw, cm_node_t* node )
{
	cm_polygonRef_t* pref;
	cm_brushRef_t* bref;

	// position test
	if( tw->positionTest )
	{
		// if already stuck in solid
		if( tw->trace.fraction == 0.0f )
		{
			return;
		}
		// test if any of the trm vertices is inside a brush
		for( bref = node->brushes; bref; bref = bref->next )
		{
			if( idCollisionModelManagerLocal::TestTrmVertsInBrush( tw, bref->b ) )
			{
				return;
			}
		}
		// if just testing a point we're done
		if( tw->pointTrace )
		{
			return;
		}
		// test if the trm is stuck in any polygons
		for( pref = node->polygons; pref; pref = pref->next )
		{
			if( idCollisionModelManagerLocal::TestTrmInPolygon( tw, pref->p ) )
			{
				return;
			}
		}
	}
	else if( tw->rotation )
	{
		// rotate through all polygons in this leaf
		for( pref = node->polygons; pref; pref = pref->next )
		{
			if( idCollisionModelManagerLocal::RotateTrmThroughPolygon( tw, pref->p ) )
			{
				return;
			}
		}
	}
	else
	{
		// trace through all polygons in this leaf
		for( pref = node->polygons; pref; pref = pref->next )
		{
			if( idCollisionModelManagerLocal::TranslateTrmThroughPolygon( tw, pref->p ) )
			{
				return;
			}
		}
	}
}

/*
================
idCollisionModelManagerLocal::TraceThroughAxialBSPTree_r
================
*/
//#define NO_SPATIAL_SUBDIVISION

void idCollisionModelManagerLocal::TraceThroughAxialBSPTree_r( cm_traceWork_t* tw, cm_node_t* node, float p1f, float p2f, idVec3& p1, idVec3& p2 )
{
	float		t1, t2, offset;
	float		frac, frac2;
	float		idist;
	idVec3		mid;
	int			side;
	float		midf;

	if( !node )
	{
		return;
	}

	if( tw->quickExit )
	{
		return;		// stop immediately
	}

	if( tw->trace.fraction <= p1f )
	{
		return;		// already hit something nearer
	}

	// if we need to test this node for collisions
	if( node->polygons || ( tw->positionTest && node->brushes ) )
	{
		// trace through node with collision data
		idCollisionModelManagerLocal::TraceTrmThroughNode( tw, node );
	}
	// if already stuck in solid
	if( tw->positionTest && tw->trace.fraction == 0.0f )
	{
		return;
	}
	// if this is a leaf node
	if( node->planeType == -1 )
	{
		return;
	}
#ifdef NO_SPATIAL_SUBDIVISION
	idCollisionModelManagerLocal::TraceThroughAxialBSPTree_r( tw, node->children[0], p1f, p2f, p1, p2 );
	idCollisionModelManagerLocal::TraceThroughAxialBSPTree_r( tw, node->children[1], p1f, p2f, p1, p2 );
	return;
#endif
	// distance from plane for trace start and end
	t1 = p1[node->planeType] - node->planeDist;
	t2 = p2[node->planeType] - node->planeDist;
	// adjust the plane distance appropriately for mins/maxs
	offset = tw->extents[node->planeType];
	// see which sides we need to consider
	if( t1 >= offset && t2 >= offset )
	{
		idCollisionModelManagerLocal::TraceThroughAxialBSPTree_r( tw, node->children[0], p1f, p2f, p1, p2 );
		return;
	}

	if( t1 < -offset && t2 < -offset )
	{
		idCollisionModelManagerLocal::TraceThroughAxialBSPTree_r( tw, node->children[1], p1f, p2f, p1, p2 );
		return;
	}

	if( t1 < t2 )
	{
		idist = 1.0f / ( t1 - t2 );
		side = 1;
		frac2 = ( t1 + offset ) * idist;
		frac = ( t1 - offset ) * idist;
	}
	else if( t1 > t2 )
	{
		idist = 1.0f / ( t1 - t2 );
		side = 0;
		frac2 = ( t1 - offset ) * idist;
		frac = ( t1 + offset ) * idist;
	}
	else
	{
		side = 0;
		frac = 1.0f;
		frac2 = 0.0f;
	}

	// move up to the node
	if( frac < 0.0f )
	{
		frac = 0.0f;
	}
	else if( frac > 1.0f )
	{
		frac = 1.0f;
	}

	midf = p1f + ( p2f - p1f ) * frac;

	mid[0] = p1[0] + frac * ( p2[0] - p1[0] );
	mid[1] = p1[1] + frac * ( p2[1] - p1[1] );
	mid[2] = p1[2] + frac * ( p2[2] - p1[2] );

	idCollisionModelManagerLocal::TraceThroughAxialBSPTree_r( tw, node->children[side], p1f, midf, p1, mid );


	// go past the node
	if( frac2 < 0.0f )
	{
		frac2 = 0.0f;
	}
	else if( frac2 > 1.0f )
	{
		frac2 = 1.0f;
	}

	midf = p1f + ( p2f - p1f ) * frac2;

	mid[0] = p1[0] + frac2 * ( p2[0] - p1[0] );
	mid[1] = p1[1] + frac2 * ( p2[1] - p1[1] );
	mid[2] = p1[2] + frac2 * ( p2[2] - p1[2] );

	idCollisionModelManagerLocal::TraceThroughAxialBSPTree_r( tw, node->children[side ^ 1], midf, p2f, mid, p2 );
}

/*
================
idCollisionModelManagerLocal::TraceThroughModel
================
*/
void idCollisionModelManagerLocal::TraceThroughModel( cm_traceWork_t* tw )
{
	float d;
	int i, numSteps;
	idVec3 start, end;
	idRotation rot;

	if( !tw->rotation )
	{
		// trace through spatial subdivision and then through leafs
		idCollisionModelManagerLocal::TraceThroughAxialBSPTree_r( tw, tw->model->node, 0, 1, tw->start, tw->end );
	}
	else
	{
		// approximate the rotation with a series of straight line movements
		// total length covered along circle
		d = tw->radius * DEG2RAD( tw->angle );
		// if more than one step
		if( d > CIRCLE_APPROXIMATION_LENGTH )
		{
			// number of steps for the approximation
			numSteps = ( int )( CIRCLE_APPROXIMATION_LENGTH / d );
			// start of approximation
			start = tw->start;
			// trace circle approximation steps through the BSP tree
			for( i = 0; i < numSteps; i++ )
			{
				// calculate next point on approximated circle
				rot.Set( tw->origin, tw->axis, tw->angle * ( ( float )( i + 1 ) / numSteps ) );
				end = start * rot;
				// trace through spatial subdivision and then through leafs
				idCollisionModelManagerLocal::TraceThroughAxialBSPTree_r( tw, tw->model->node, 0, 1, start, end );
				// no need to continue if something was hit already
				if( tw->trace.fraction < 1.0f )
				{
					return;
				}
				start = end;
			}
		}
		else
		{
			start = tw->start;
		}
		// last step of the approximation
		idCollisionModelManagerLocal::TraceThroughAxialBSPTree_r( tw, tw->model->node, 0, 1, start, tw->end );
	}
}
