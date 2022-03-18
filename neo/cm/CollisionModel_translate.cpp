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

Collision detection for translational motion

===============================================================================
*/

/*
================
idCollisionModelManagerLocal::TranslateEdgeThroughEdge

  calculates fraction of the translation completed at which the edges collide
================
*/
ID_INLINE int idCollisionModelManagerLocal::TranslateEdgeThroughEdge( idVec3& cross, idPluecker& l1, idPluecker& l2, float* fraction )
{

	float d, t;

	/*

	a = start of line
	b = end of line
	dir = movement direction
	l1 = pluecker coordinate for line
	l2 = pluecker coordinate for edge we might collide with
	a+dir = start of line after movement
	b+dir = end of line after movement
	t = scale factor
	solve pluecker inner product for t of line (a+t*dir : b+t*dir) and line l2

	v[0] = (a[0]+t*dir[0]) * (b[1]+t*dir[1]) - (b[0]+t*dir[0]) * (a[1]+t*dir[1]);
	v[1] = (a[0]+t*dir[0]) * (b[2]+t*dir[2]) - (b[0]+t*dir[0]) * (a[2]+t*dir[2]);
	v[2] = (a[0]+t*dir[0]) - (b[0]+t*dir[0]);
	v[3] = (a[1]+t*dir[1]) * (b[2]+t*dir[2]) - (b[1]+t*dir[1]) * (a[2]+t*dir[2]);
	v[4] = (a[2]+t*dir[2]) - (b[2]+t*dir[2]);
	v[5] = (b[1]+t*dir[1]) - (a[1]+t*dir[1]);

	l2[0] * v[4] + l2[1] * v[5] + l2[2] * v[3] + l2[4] * v[0] + l2[5] * v[1] + l2[3] * v[2] = 0;

	solve t

	v[0] = (a[0]+t*dir[0]) * (b[1]+t*dir[1]) - (b[0]+t*dir[0]) * (a[1]+t*dir[1]);
	v[0] = (a[0]*b[1]) + a[0]*t*dir[1] + b[1]*t*dir[0] + (t*t*dir[0]*dir[1]) -
			((b[0]*a[1]) + b[0]*t*dir[1] + a[1]*t*dir[0] + (t*t*dir[0]*dir[1]));
	v[0] = a[0]*b[1] + a[0]*t*dir[1] + b[1]*t*dir[0] - b[0]*a[1] - b[0]*t*dir[1] - a[1]*t*dir[0];

	v[1] = (a[0]+t*dir[0]) * (b[2]+t*dir[2]) - (b[0]+t*dir[0]) * (a[2]+t*dir[2]);
	v[1] = (a[0]*b[2]) + a[0]*t*dir[2] + b[2]*t*dir[0] + (t*t*dir[0]*dir[2]) -
			((b[0]*a[2]) + b[0]*t*dir[2] + a[2]*t*dir[0] + (t*t*dir[0]*dir[2]));
	v[1] = a[0]*b[2] + a[0]*t*dir[2] + b[2]*t*dir[0] - b[0]*a[2] - b[0]*t*dir[2] - a[2]*t*dir[0];

	v[2] = (a[0]+t*dir[0]) - (b[0]+t*dir[0]);
	v[2] = a[0] - b[0];

	v[3] = (a[1]+t*dir[1]) * (b[2]+t*dir[2]) - (b[1]+t*dir[1]) * (a[2]+t*dir[2]);
	v[3] = (a[1]*b[2]) + a[1]*t*dir[2] + b[2]*t*dir[1] + (t*t*dir[1]*dir[2]) -
			((b[1]*a[2]) + b[1]*t*dir[2] + a[2]*t*dir[1] + (t*t*dir[1]*dir[2]));
	v[3] = a[1]*b[2] + a[1]*t*dir[2] + b[2]*t*dir[1] - b[1]*a[2] - b[1]*t*dir[2] - a[2]*t*dir[1];

	v[4] = (a[2]+t*dir[2]) - (b[2]+t*dir[2]);
	v[4] = a[2] - b[2];

	v[5] = (b[1]+t*dir[1]) - (a[1]+t*dir[1]);
	v[5] = b[1] - a[1];


	v[0] = a[0]*b[1] + a[0]*t*dir[1] + b[1]*t*dir[0] - b[0]*a[1] - b[0]*t*dir[1] - a[1]*t*dir[0];
	v[1] = a[0]*b[2] + a[0]*t*dir[2] + b[2]*t*dir[0] - b[0]*a[2] - b[0]*t*dir[2] - a[2]*t*dir[0];
	v[2] = a[0] - b[0];
	v[3] = a[1]*b[2] + a[1]*t*dir[2] + b[2]*t*dir[1] - b[1]*a[2] - b[1]*t*dir[2] - a[2]*t*dir[1];
	v[4] = a[2] - b[2];
	v[5] = b[1] - a[1];

	v[0] = (a[0]*dir[1] + b[1]*dir[0] - b[0]*dir[1] - a[1]*dir[0]) * t + a[0]*b[1] - b[0]*a[1];
	v[1] = (a[0]*dir[2] + b[2]*dir[0] - b[0]*dir[2] - a[2]*dir[0]) * t + a[0]*b[2] - b[0]*a[2];
	v[2] = a[0] - b[0];
	v[3] = (a[1]*dir[2] + b[2]*dir[1] - b[1]*dir[2] - a[2]*dir[1]) * t + a[1]*b[2] - b[1]*a[2];
	v[4] = a[2] - b[2];
	v[5] = b[1] - a[1];

	l2[4] * (a[0]*dir[1] + b[1]*dir[0] - b[0]*dir[1] - a[1]*dir[0]) * t + l2[4] * (a[0]*b[1] - b[0]*a[1])
		+ l2[5] * (a[0]*dir[2] + b[2]*dir[0] - b[0]*dir[2] - a[2]*dir[0]) * t + l2[5] * (a[0]*b[2] - b[0]*a[2])
		+ l2[3] * (a[0] - b[0])
		+ l2[2] * (a[1]*dir[2] + b[2]*dir[1] - b[1]*dir[2] - a[2]*dir[1]) * t + l2[2] * (a[1]*b[2] - b[1]*a[2])
		+ l2[0] * (a[2] - b[2])
		+ l2[1] * (b[1] - a[1]) = 0

	t = (- l2[4] * (a[0]*b[1] - b[0]*a[1]) -
			l2[5] * (a[0]*b[2] - b[0]*a[2]) -
			l2[3] * (a[0] - b[0]) -
			l2[2] * (a[1]*b[2] - b[1]*a[2]) -
			l2[0] * (a[2] - b[2]) -
			l2[1] * (b[1] - a[1])) /
				(l2[4] * (a[0]*dir[1] + b[1]*dir[0] - b[0]*dir[1] - a[1]*dir[0]) +
				l2[5] * (a[0]*dir[2] + b[2]*dir[0] - b[0]*dir[2] - a[2]*dir[0]) +
				l2[2] * (a[1]*dir[2] + b[2]*dir[1] - b[1]*dir[2] - a[2]*dir[1]));

	d = l2[4] * (a[0]*dir[1] + b[1]*dir[0] - b[0]*dir[1] - a[1]*dir[0]) +
		l2[5] * (a[0]*dir[2] + b[2]*dir[0] - b[0]*dir[2] - a[2]*dir[0]) +
		l2[2] * (a[1]*dir[2] + b[2]*dir[1] - b[1]*dir[2] - a[2]*dir[1]);

	t = - ( l2[4] * (a[0]*b[1] - b[0]*a[1]) +
			l2[5] * (a[0]*b[2] - b[0]*a[2]) +
			l2[3] * (a[0] - b[0]) +
			l2[2] * (a[1]*b[2] - b[1]*a[2]) +
			l2[0] * (a[2] - b[2]) +
			l2[1] * (b[1] - a[1]));
	t /= d;

	MrE pats Pluecker on the head.. good monkey

	edgeDir = a - b;
	d = l2[4] * (edgeDir[0]*dir[1] - edgeDir[1]*dir[0]) +
		l2[5] * (edgeDir[0]*dir[2] - edgeDir[2]*dir[0]) +
		l2[2] * (edgeDir[1]*dir[2] - edgeDir[2]*dir[1]);
	*/

	d = l2[4] * cross[0] + l2[5] * cross[1] + l2[2] * cross[2];

	if( d == 0.0f )
	{
		*fraction = 1.0f;
		// no collision ever
		return false;
	}

	t = -l1.PermutedInnerProduct( l2 );
	// if the lines cross each other to begin with
	if( fabs( t ) < idMath::FLT_SMALLEST_NON_DENORMAL )
	{
		*fraction = 0.0f;
		return true;
	}
	// fraction of movement at the time the lines cross each other
	*fraction = t / d;
	return true;
}

/*
================
CM_AddContact
================
*/
ID_INLINE void CM_AddContact( cm_traceWork_t* tw )
{

	if( tw->numContacts >= tw->maxContacts )
	{
		return;
	}
	// copy contact information from trace_t
	tw->contacts[tw->numContacts] = tw->trace.c;
	tw->numContacts++;
	// set fraction back to 1 to find all other contacts
	tw->trace.fraction = 1.0f;
}

/*
================
CM_SetVertexSidedness

  stores for the given model vertex at which side of one of the trm edges it passes
================
*/
ID_INLINE void CM_SetVertexSidedness( cm_vertex_t* v, const idPluecker& vpl, const idPluecker& epl, const int bitNum )
{
	const int mask = 1 << bitNum;
	if( ( v->sideSet & mask ) == 0 )
	{
		const float fl = vpl.PermutedInnerProduct( epl );
		v->side = ( v->side & ~mask ) | ( ( fl < 0.0f ) ? mask : 0 );
		v->sideSet |= mask;
	}
}

/*
================
CM_SetEdgeSidedness

  stores for the given model edge at which side one of the trm vertices
================
*/
ID_INLINE void CM_SetEdgeSidedness( cm_edge_t* edge, const idPluecker& vpl, const idPluecker& epl, const int bitNum )
{
	const int mask = 1 << bitNum;
	if( ( edge->sideSet & mask ) == 0 )
	{
		const float fl = vpl.PermutedInnerProduct( epl );
		edge->side = ( edge->side & ~mask ) | ( ( fl < 0.0f ) ? mask : 0 );
		edge->sideSet |= mask;
	}
}

/*
================
idCollisionModelManagerLocal::TranslateTrmEdgeThroughPolygon
================
*/
void idCollisionModelManagerLocal::TranslateTrmEdgeThroughPolygon( cm_traceWork_t* tw, cm_polygon_t* poly, cm_trmEdge_t* trmEdge )
{
	int i, edgeNum;
	float f1, f2, dist, d1, d2;
	idVec3 start, end, normal;
	cm_edge_t* edge;
	cm_vertex_t* v1, *v2;
	idPluecker* pl, epsPl;

	// check edges for a collision
	for( i = 0; i < poly->numEdges; i++ )
	{
		edgeNum = poly->edges[i];
		edge = tw->model->edges + abs( edgeNum );
		// if this edge is already checked
		if( edge->checkcount == idCollisionModelManagerLocal::checkCount )
		{
			continue;
		}
		// can never collide with internal edges
		if( edge->internal )
		{
			continue;
		}
		pl = &tw->polygonEdgePlueckerCache[i];
		// get the sides at which the trm edge vertices pass the polygon edge
		CM_SetEdgeSidedness( edge, *pl, tw->vertices[trmEdge->vertexNum[0]].pl, trmEdge->vertexNum[0] );
		CM_SetEdgeSidedness( edge, *pl, tw->vertices[trmEdge->vertexNum[1]].pl, trmEdge->vertexNum[1] );
		// if the trm edge start and end vertex do not pass the polygon edge at different sides
		if( !( ( ( edge->side >> trmEdge->vertexNum[0] ) ^ ( edge->side >> trmEdge->vertexNum[1] ) ) & 1 ) )
		{
			continue;
		}
		// get the sides at which the polygon edge vertices pass the trm edge
		v1 = tw->model->vertices + edge->vertexNum[INT32_SIGNBITSET( edgeNum )];
		CM_SetVertexSidedness( v1, tw->polygonVertexPlueckerCache[i], trmEdge->pl, trmEdge->bitNum );
		v2 = tw->model->vertices + edge->vertexNum[INT32_SIGNBITNOTSET( edgeNum )];
		CM_SetVertexSidedness( v2, tw->polygonVertexPlueckerCache[i + 1], trmEdge->pl, trmEdge->bitNum );
		// if the polygon edge start and end vertex do not pass the trm edge at different sides
		if( !( ( v1->side ^ v2->side ) & ( 1 << trmEdge->bitNum ) ) )
		{
			continue;
		}
		// if there is no possible collision between the trm edge and the polygon edge
		if( !idCollisionModelManagerLocal::TranslateEdgeThroughEdge( trmEdge->cross, trmEdge->pl, *pl, &f1 ) )
		{
			continue;
		}
		// if moving away from edge
		if( f1 < 0.0f )
		{
			continue;
		}

		// pluecker coordinate for epsilon expanded edge
		epsPl.FromLine( tw->model->vertices[edge->vertexNum[0]].p + edge->normal * CM_CLIP_EPSILON,
						tw->model->vertices[edge->vertexNum[1]].p + edge->normal * CM_CLIP_EPSILON );
		// calculate collision fraction with epsilon expanded edge
		if( !idCollisionModelManagerLocal::TranslateEdgeThroughEdge( trmEdge->cross, trmEdge->pl, epsPl, &f2 ) )
		{
			continue;
		}
		// if no collision with epsilon edge or moving away from edge
		if( f2 > 1.0f || f1 < f2 )
		{
			continue;
		}

		if( f2 < 0.0f )
		{
			f2 = 0.0f;
		}

		if( f2 < tw->trace.fraction )
		{
			tw->trace.fraction = f2;
			// create plane with normal vector orthogonal to both the polygon edge and the trm edge
			start = tw->model->vertices[edge->vertexNum[0]].p;
			end = tw->model->vertices[edge->vertexNum[1]].p;
			tw->trace.c.normal = ( end - start ).Cross( trmEdge->end - trmEdge->start );
			// FIXME: do this normalize when we know the first collision
			tw->trace.c.normal.Normalize();
			tw->trace.c.dist = tw->trace.c.normal * start;
			// make sure the collision plane faces the trace model
			if( tw->trace.c.normal * trmEdge->start - tw->trace.c.dist < 0.0f )
			{
				tw->trace.c.normal = -tw->trace.c.normal;
				tw->trace.c.dist = -tw->trace.c.dist;
			}
			tw->trace.c.contents = poly->contents;
			tw->trace.c.material = poly->material;
			tw->trace.c.type = CONTACT_EDGE;
			tw->trace.c.modelFeature = edgeNum;
			tw->trace.c.trmFeature = trmEdge - tw->edges;
			// calculate collision point
			normal[0] = trmEdge->cross[2];
			normal[1] = -trmEdge->cross[1];
			normal[2] = trmEdge->cross[0];
			dist = normal * trmEdge->start;
			d1 = normal * start - dist;
			d2 = normal * end - dist;
			f1 = d1 / ( d1 - d2 );
			//assert( f1 >= 0.0f && f1 <= 1.0f );
			tw->trace.c.point = start + f1 * ( end - start );
			// if retrieving contacts
			if( tw->getContacts )
			{
				CM_AddContact( tw );
			}
		}
	}
}

/*
================
CM_TranslationPlaneFraction
================
*/
float CM_TranslationPlaneFraction( const idPlane& plane, const idVec3& start, const idVec3& end )
{
	const float d2 = plane.Distance( end );
	// if the end point is closer to the plane than an epsilon we still take it for a collision
	if( d2 >= CM_CLIP_EPSILON )
	{
		return 1.0f;
	}
	const float d1 = plane.Distance( start );

	// if completely behind the polygon
	if( d1 <= 0.0f )
	{
		return 1.0f;
	}
	// leaves polygon
	if( d1 - d2 < idMath::FLT_SMALLEST_NON_DENORMAL )
	{
		return 1.0f;
	}
	return ( d1 - CM_CLIP_EPSILON ) / ( d1 - d2 );
}

/*
================
idCollisionModelManagerLocal::TranslateTrmVertexThroughPolygon
================
*/
void idCollisionModelManagerLocal::TranslateTrmVertexThroughPolygon( cm_traceWork_t* tw, cm_polygon_t* poly, cm_trmVertex_t* v, int bitNum )
{
	int i, edgeNum;
	float f;
	cm_edge_t* edge;

	f = CM_TranslationPlaneFraction( poly->plane, v->p, v->endp );
	if( f < tw->trace.fraction )
	{

		for( i = 0; i < poly->numEdges; i++ )
		{
			edgeNum = poly->edges[i];
			edge = tw->model->edges + abs( edgeNum );
			CM_SetEdgeSidedness( edge, tw->polygonEdgePlueckerCache[i], v->pl, bitNum );
			if( INT32_SIGNBITSET( edgeNum ) ^ ( ( edge->side >> bitNum ) & 1 ) )
			{
				return;
			}
		}
		if( f < 0.0f )
		{
			f = 0.0f;
		}
		tw->trace.fraction = f;
		// collision plane is the polygon plane
		tw->trace.c.normal = poly->plane.Normal();
		tw->trace.c.dist = poly->plane.Dist();
		tw->trace.c.contents = poly->contents;
		tw->trace.c.material = poly->material;
		tw->trace.c.type = CONTACT_TRMVERTEX;
		tw->trace.c.modelFeature = *reinterpret_cast<int*>( &poly );
		tw->trace.c.trmFeature = v - tw->vertices;
		tw->trace.c.point = v->p + tw->trace.fraction * ( v->endp - v->p );
		// if retrieving contacts
		if( tw->getContacts )
		{
			CM_AddContact( tw );
			// no need to store the trm vertex more than once as a contact
			v->used = false;
		}
	}
}

/*
================
idCollisionModelManagerLocal::TranslatePointThroughPolygon
================
*/
void idCollisionModelManagerLocal::TranslatePointThroughPolygon( cm_traceWork_t* tw, cm_polygon_t* poly, cm_trmVertex_t* v )
{
	int i, edgeNum;
	float f;
	cm_edge_t* edge;
	idPluecker pl;

	f = CM_TranslationPlaneFraction( poly->plane, v->p, v->endp );
	if( f < tw->trace.fraction )
	{

		for( i = 0; i < poly->numEdges; i++ )
		{
			edgeNum = poly->edges[i];
			edge = tw->model->edges + abs( edgeNum );
			// if we didn't yet calculate the sidedness for this edge
			if( edge->checkcount != idCollisionModelManagerLocal::checkCount )
			{
				float fl;
				edge->checkcount = idCollisionModelManagerLocal::checkCount;
				pl.FromLine( tw->model->vertices[edge->vertexNum[0]].p, tw->model->vertices[edge->vertexNum[1]].p );
				fl = v->pl.PermutedInnerProduct( pl );
				edge->side = ( fl < 0.0f );
			}
			// if the point passes the edge at the wrong side
			//if ( (edgeNum > 0) == edge->side ) {
			if( INT32_SIGNBITSET( edgeNum ) ^ edge->side )
			{
				return;
			}
		}
		if( f < 0.0f )
		{
			f = 0.0f;
		}
		tw->trace.fraction = f;
		// collision plane is the polygon plane
		tw->trace.c.normal = poly->plane.Normal();
		tw->trace.c.dist = poly->plane.Dist();
		tw->trace.c.contents = poly->contents;
		tw->trace.c.material = poly->material;
		tw->trace.c.type = CONTACT_TRMVERTEX;
		tw->trace.c.modelFeature = *reinterpret_cast<int*>( &poly );
		tw->trace.c.trmFeature = v - tw->vertices;
		tw->trace.c.point = v->p + tw->trace.fraction * ( v->endp - v->p );
		// if retrieving contacts
		if( tw->getContacts )
		{
			CM_AddContact( tw );
			// no need to store the trm vertex more than once as a contact
			v->used = false;
		}
	}
}

/*
================
idCollisionModelManagerLocal::TranslateVertexThroughTrmPolygon
================
*/
void idCollisionModelManagerLocal::TranslateVertexThroughTrmPolygon( cm_traceWork_t* tw, cm_trmPolygon_t* trmpoly, cm_polygon_t* poly, cm_vertex_t* v, idVec3& endp, idPluecker& pl )
{
	int i, edgeNum;
	float f;
	cm_trmEdge_t* edge;

	f = CM_TranslationPlaneFraction( trmpoly->plane, v->p, endp );
	if( f < tw->trace.fraction )
	{

		for( i = 0; i < trmpoly->numEdges; i++ )
		{
			edgeNum = trmpoly->edges[i];
			edge = tw->edges + abs( edgeNum );

			CM_SetVertexSidedness( v, pl, edge->pl, edge->bitNum );
			if( INT32_SIGNBITSET( edgeNum ) ^ ( ( v->side >> edge->bitNum ) & 1 ) )
			{
				return;
			}
		}
		if( f < 0.0f )
		{
			f = 0.0f;
		}
		tw->trace.fraction = f;
		// collision plane is the inverse trm polygon plane
		tw->trace.c.normal = -trmpoly->plane.Normal();
		tw->trace.c.dist = -trmpoly->plane.Dist();
		tw->trace.c.contents = poly->contents;
		tw->trace.c.material = poly->material;
		tw->trace.c.type = CONTACT_MODELVERTEX;
		tw->trace.c.modelFeature = v - tw->model->vertices;
		tw->trace.c.trmFeature = trmpoly - tw->polys;
		tw->trace.c.point = v->p + tw->trace.fraction * ( endp - v->p );
		// if retrieving contacts
		if( tw->getContacts )
		{
			CM_AddContact( tw );
		}
	}
}

/*
================
idCollisionModelManagerLocal::TranslateTrmThroughPolygon

  returns true if the polygon blocks the complete translation
================
*/
bool idCollisionModelManagerLocal::TranslateTrmThroughPolygon( cm_traceWork_t* tw, cm_polygon_t* p )
{
	int i, j, k, edgeNum;
	float fraction, d;
	idVec3 endp;
	idPluecker* pl;
	cm_trmVertex_t* bv;
	cm_trmEdge_t* be;
	cm_trmPolygon_t* bp;
	cm_vertex_t* v;
	cm_edge_t* e;

	// if already checked this polygon
	if( p->checkcount == idCollisionModelManagerLocal::checkCount )
	{
		return false;
	}
	p->checkcount = idCollisionModelManagerLocal::checkCount;

	// if this polygon does not have the right contents behind it
	if( !( p->contents & tw->contents ) )
	{
		return false;
	}

	// if the the trace bounds do not intersect the polygon bounds
	if( !tw->bounds.IntersectsBounds( p->bounds ) )
	{
		return false;
	}

	// only collide with the polygon if approaching at the front
	if( ( p->plane.Normal() * tw->dir ) > 0.0f )
	{
		return false;
	}

	// if the polygon is too far from the first heart plane
	d = p->bounds.PlaneDistance( tw->heartPlane1 );
	if( idMath::Fabs( d ) > tw->maxDistFromHeartPlane1 )
	{
		return false;
	}

	// if the polygon is too far from the second heart plane
	d = p->bounds.PlaneDistance( tw->heartPlane2 );
	if( idMath::Fabs( d ) > tw->maxDistFromHeartPlane2 )
	{
		return false;
	}
	fraction = tw->trace.fraction;

	// fast point trace
	if( tw->pointTrace )
	{
		idCollisionModelManagerLocal::TranslatePointThroughPolygon( tw, p, &tw->vertices[0] );
	}
	else
	{

		// trace bounds should cross polygon plane
		switch( tw->bounds.PlaneSide( p->plane ) )
		{
			case PLANESIDE_CROSS:
				break;
			case PLANESIDE_FRONT:
				if( tw->model->isConvex )
				{
					tw->quickExit = true;
					return true;
				}
			default:
				return false;
		}

		// calculate pluecker coordinates for the polygon edges and polygon vertices
		for( i = 0; i < p->numEdges; i++ )
		{
			edgeNum = p->edges[i];
			e = tw->model->edges + abs( edgeNum );
			// reset sidedness cache if this is the first time we encounter this edge during this trace
			if( e->checkcount != idCollisionModelManagerLocal::checkCount )
			{
				e->sideSet = 0;
			}
			// pluecker coordinate for edge
			tw->polygonEdgePlueckerCache[i].FromLine( tw->model->vertices[e->vertexNum[0]].p,
					tw->model->vertices[e->vertexNum[1]].p );

			v = &tw->model->vertices[e->vertexNum[INT32_SIGNBITSET( edgeNum )]];
			// reset sidedness cache if this is the first time we encounter this vertex during this trace
			if( v->checkcount != idCollisionModelManagerLocal::checkCount )
			{
				v->sideSet = 0;
			}
			// pluecker coordinate for vertex movement vector
			tw->polygonVertexPlueckerCache[i].FromRay( v->p, -tw->dir );
		}
		// copy first to last so we can easily cycle through for the edges
		tw->polygonVertexPlueckerCache[p->numEdges] = tw->polygonVertexPlueckerCache[0];

		// trace trm vertices through polygon
		for( i = 0; i < tw->numVerts; i++ )
		{
			bv = tw->vertices + i;
			if( bv->used )
			{
				idCollisionModelManagerLocal::TranslateTrmVertexThroughPolygon( tw, p, bv, i );
			}
		}

		// trace trm edges through polygon
		for( i = 1; i <= tw->numEdges; i++ )
		{
			be = tw->edges + i;
			if( be->used )
			{
				idCollisionModelManagerLocal::TranslateTrmEdgeThroughPolygon( tw, p, be );
			}
		}

		// trace all polygon vertices through the trm
		for( i = 0; i < p->numEdges; i++ )
		{
			edgeNum = p->edges[i];
			e = tw->model->edges + abs( edgeNum );

			if( e->checkcount == idCollisionModelManagerLocal::checkCount )
			{
				continue;
			}
			// set edge check count
			e->checkcount = idCollisionModelManagerLocal::checkCount;
			// can never collide with internal edges
			if( e->internal )
			{
				continue;
			}
			// got to check both vertices because we skip internal edges
			for( k = 0; k < 2; k++ )
			{

				v = tw->model->vertices + e->vertexNum[k ^ INT32_SIGNBITSET( edgeNum )];
				// if this vertex is already checked
				if( v->checkcount == idCollisionModelManagerLocal::checkCount )
				{
					continue;
				}
				// set vertex check count
				v->checkcount = idCollisionModelManagerLocal::checkCount;

				// if the vertex is outside the trace bounds
				if( !tw->bounds.ContainsPoint( v->p ) )
				{
					continue;
				}

				// vertex end point after movement
				endp = v->p - tw->dir;
				// pluecker coordinate for vertex movement vector
				pl = &tw->polygonVertexPlueckerCache[i + k];

				for( j = 0; j < tw->numPolys; j++ )
				{
					bp = tw->polys + j;
					if( bp->used )
					{
						idCollisionModelManagerLocal::TranslateVertexThroughTrmPolygon( tw, bp, p, v, endp, *pl );
					}
				}
			}
		}
	}

	// if there was a collision with this polygon and we are not retrieving contacts
	if( tw->trace.fraction < fraction && !tw->getContacts )
	{
		fraction = tw->trace.fraction;
		endp = tw->start + fraction * tw->dir;
		// decrease bounds
		for( i = 0; i < 3; i++ )
		{
			if( tw->start[i] < endp[i] )
			{
				tw->bounds[0][i] = tw->start[i] + tw->size[0][i] - CM_BOX_EPSILON;
				tw->bounds[1][i] = endp[i] + tw->size[1][i] + CM_BOX_EPSILON;
			}
			else
			{
				tw->bounds[0][i] = endp[i] + tw->size[0][i] - CM_BOX_EPSILON;
				tw->bounds[1][i] = tw->start[i] + tw->size[1][i] + CM_BOX_EPSILON;
			}
		}
	}

	return ( tw->trace.fraction == 0.0f );
}

/*
================
idCollisionModelManagerLocal::SetupTrm
================
*/
void idCollisionModelManagerLocal::SetupTrm( cm_traceWork_t* tw, const idTraceModel* trm )
{
	int i, j;

	// vertices
	tw->numVerts = trm->numVerts;
	for( i = 0; i < trm->numVerts; i++ )
	{
		tw->vertices[i].p = trm->verts[i];
		tw->vertices[i].used = false;
	}
	// edges
	tw->numEdges = trm->numEdges;
	for( i = 1; i <= trm->numEdges; i++ )
	{
		tw->edges[i].vertexNum[0] = trm->edges[i].v[0];
		tw->edges[i].vertexNum[1] = trm->edges[i].v[1];
		tw->edges[i].used = false;
	}
	// polygons
	tw->numPolys = trm->numPolys;
	for( i = 0; i < trm->numPolys; i++ )
	{
		tw->polys[i].numEdges = trm->polys[i].numEdges;
		for( j = 0; j < trm->polys[i].numEdges; j++ )
		{
			tw->polys[i].edges[j] = trm->polys[i].edges[j];
		}
		tw->polys[i].plane.SetNormal( trm->polys[i].normal );
		tw->polys[i].used = false;
	}
	// is the trace model convex or not
	tw->isConvex = trm->isConvex;
}

/*
================
idCollisionModelManagerLocal::SetupTranslationHeartPlanes
================
*/
void idCollisionModelManagerLocal::SetupTranslationHeartPlanes( cm_traceWork_t* tw )
{
	idVec3 dir, normal1, normal2;

	// calculate trace heart planes
	dir = tw->dir;
	dir.Normalize();
	dir.NormalVectors( normal1, normal2 );
	tw->heartPlane1.SetNormal( normal1 );
	tw->heartPlane1.FitThroughPoint( tw->start );
	tw->heartPlane2.SetNormal( normal2 );
	tw->heartPlane2.FitThroughPoint( tw->start );
}

/*
================
idCollisionModelManagerLocal::Translation
================
*/
#ifdef _DEBUG
	static int entered = 0;
#endif

void idCollisionModelManagerLocal::Translation( trace_t* results, const idVec3& start, const idVec3& end,
		const idTraceModel* trm, const idMat3& trmAxis, int contentMask,
		cmHandle_t model, const idVec3& modelOrigin, const idMat3& modelAxis )
{

	int i, j;
	float dist;
	bool model_rotated, trm_rotated;
	idVec3 dir1, dir2, dir;
	idMat3 invModelAxis, tmpAxis;
	cm_trmPolygon_t* poly;
	cm_trmEdge_t* edge;
	cm_trmVertex_t* vert;
	ALIGN16( static cm_traceWork_t tw );

	assert( ( ( byte* )&start ) < ( ( byte* )results ) || ( ( byte* )&start ) >= ( ( ( byte* )results ) + sizeof( trace_t ) ) );
	assert( ( ( byte* )&end ) < ( ( byte* )results ) || ( ( byte* )&end ) >= ( ( ( byte* )results ) + sizeof( trace_t ) ) );
	assert( ( ( byte* )&trmAxis ) < ( ( byte* )results ) || ( ( byte* )&trmAxis ) >= ( ( ( byte* )results ) + sizeof( trace_t ) ) );

	memset( results, 0, sizeof( *results ) );

	if( model < 0 || model > MAX_SUBMODELS || model > idCollisionModelManagerLocal::maxModels )
	{
		common->Printf( "idCollisionModelManagerLocal::Translation: invalid model handle\n" );
		return;
	}
	if( !idCollisionModelManagerLocal::models[model] )
	{
		common->Printf( "idCollisionModelManagerLocal::Translation: invalid model\n" );
		return;
	}

	// if case special position test
	if( start[0] == end[0] && start[1] == end[1] && start[2] == end[2] )
	{
		idCollisionModelManagerLocal::ContentsTrm( results, start, trm, trmAxis, contentMask, model, modelOrigin, modelAxis );
		return;
	}

#ifdef _DEBUG
	bool startsolid = false;
	// test whether or not stuck to begin with
	if( cm_debugCollision.GetBool() )
	{
		if( !entered && !idCollisionModelManagerLocal::getContacts )
		{
			entered = 1;
			// if already messed up to begin with
			if( idCollisionModelManagerLocal::Contents( start, trm, trmAxis, -1, model, modelOrigin, modelAxis ) & contentMask )
			{
				startsolid = true;
			}
			entered = 0;
		}
	}
#endif

	idCollisionModelManagerLocal::checkCount++;

	tw.trace.fraction = 1.0f;
	tw.trace.c.contents = 0;
	tw.trace.c.type = CONTACT_NONE;
	tw.trace.c.material = NULL;
	tw.trace.c.id = 0;
	tw.contents = contentMask;
	tw.isConvex = true;
	tw.rotation = false;
	tw.positionTest = false;
	tw.quickExit = false;
	tw.getContacts = idCollisionModelManagerLocal::getContacts;
	tw.contacts = idCollisionModelManagerLocal::contacts;
	tw.maxContacts = idCollisionModelManagerLocal::maxContacts;
	tw.numContacts = 0;
	tw.model = idCollisionModelManagerLocal::models[model];
	tw.start = start - modelOrigin;
	tw.end = end - modelOrigin;
	tw.dir = end - start;

	model_rotated = modelAxis.IsRotated();
	if( model_rotated )
	{
		invModelAxis = modelAxis.Transpose();
	}

	// if optimized point trace
	if( !trm || ( trm->bounds[1][0] - trm->bounds[0][0] <= 0.0f &&
				  trm->bounds[1][1] - trm->bounds[0][1] <= 0.0f &&
				  trm->bounds[1][2] - trm->bounds[0][2] <= 0.0f ) )
	{

		if( model_rotated )
		{
			// rotate trace instead of model
			tw.start *= invModelAxis;
			tw.end *= invModelAxis;
			tw.dir *= invModelAxis;
		}

		// trace bounds
		for( i = 0; i < 3; i++ )
		{
			if( tw.start[i] < tw.end[i] )
			{
				tw.bounds[0][i] = tw.start[i] - CM_BOX_EPSILON;
				tw.bounds[1][i] = tw.end[i] + CM_BOX_EPSILON;
			}
			else
			{
				tw.bounds[0][i] = tw.end[i] - CM_BOX_EPSILON;
				tw.bounds[1][i] = tw.start[i] + CM_BOX_EPSILON;
			}
		}
		tw.extents[0] = tw.extents[1] = tw.extents[2] = CM_BOX_EPSILON;
		tw.size.Zero();

		// setup trace heart planes
		idCollisionModelManagerLocal::SetupTranslationHeartPlanes( &tw );
		tw.maxDistFromHeartPlane1 = CM_BOX_EPSILON;
		tw.maxDistFromHeartPlane2 = CM_BOX_EPSILON;
		// collision with single point
		tw.numVerts = 1;
		tw.vertices[0].p = tw.start;
		tw.vertices[0].endp = tw.vertices[0].p + tw.dir;
		tw.vertices[0].pl.FromRay( tw.vertices[0].p, tw.dir );
		tw.numEdges = tw.numPolys = 0;
		tw.pointTrace = true;
		// trace through the model
		idCollisionModelManagerLocal::TraceThroughModel( &tw );
		// store results
		*results = tw.trace;
		results->endpos = start + results->fraction * ( end - start );
		results->endAxis = mat3_identity;

		if( results->fraction < 1.0f )
		{
			// rotate trace plane normal if there was a collision with a rotated model
			if( model_rotated )
			{
				results->c.normal *= modelAxis;
				results->c.point *= modelAxis;
			}
			results->c.point += modelOrigin;
			results->c.dist += modelOrigin * results->c.normal;
		}
		idCollisionModelManagerLocal::numContacts = tw.numContacts;
		return;
	}

	// the trace fraction is too inaccurate to describe translations over huge distances
	if( tw.dir.LengthSqr() > Square( CM_MAX_TRACE_DIST ) )
	{
		results->fraction = 0.0f;
		results->endpos = start;
		results->endAxis = trmAxis;
		results->c.normal = vec3_origin;
		results->c.material = NULL;
		results->c.point = start;
		if( common->RW() )
		{
			common->RW()->DebugArrow( colorRed, start, end, 1 );
		}
		common->Printf( "idCollisionModelManagerLocal::Translation: huge translation\n" );
		return;
	}

	tw.pointTrace = false;
	tw.size.Clear();

	// setup trm structure
	idCollisionModelManagerLocal::SetupTrm( &tw, trm );

	trm_rotated = trmAxis.IsRotated();

	// calculate vertex positions
	if( trm_rotated )
	{
		for( i = 0; i < tw.numVerts; i++ )
		{
			// rotate trm around the start position
			tw.vertices[i].p *= trmAxis;
		}
	}
	for( i = 0; i < tw.numVerts; i++ )
	{
		// set trm at start position
		tw.vertices[i].p += tw.start;
	}
	if( model_rotated )
	{
		for( i = 0; i < tw.numVerts; i++ )
		{
			// rotate trm around model instead of rotating the model
			tw.vertices[i].p *= invModelAxis;
		}
	}

	// add offset to start point
	if( trm_rotated )
	{
		dir = trm->offset * trmAxis;
		tw.start += dir;
		tw.end += dir;
	}
	else
	{
		tw.start += trm->offset;
		tw.end += trm->offset;
	}
	if( model_rotated )
	{
		// rotate trace instead of model
		tw.start *= invModelAxis;
		tw.end *= invModelAxis;
		tw.dir *= invModelAxis;
	}

	// rotate trm polygon planes
	if( trm_rotated & model_rotated )
	{
		tmpAxis = trmAxis * invModelAxis;
		for( poly = tw.polys, i = 0; i < tw.numPolys; i++, poly++ )
		{
			poly->plane *= tmpAxis;
		}
	}
	else if( trm_rotated )
	{
		for( poly = tw.polys, i = 0; i < tw.numPolys; i++, poly++ )
		{
			poly->plane *= trmAxis;
		}
	}
	else if( model_rotated )
	{
		for( poly = tw.polys, i = 0; i < tw.numPolys; i++, poly++ )
		{
			poly->plane *= invModelAxis;
		}
	}

	// setup trm polygons
	for( poly = tw.polys, i = 0; i < tw.numPolys; i++, poly++ )
	{
		// if the trm poly plane is facing in the movement direction
		dist = poly->plane.Normal() * tw.dir;
		if( dist > 0.0f || ( !trm->isConvex && dist == 0.0f ) )
		{
			// this trm poly and it's edges and vertices need to be used for collision
			poly->used = true;
			for( j = 0; j < poly->numEdges; j++ )
			{
				edge = &tw.edges[abs( poly->edges[j] )];
				edge->used = true;
				tw.vertices[edge->vertexNum[0]].used = true;
				tw.vertices[edge->vertexNum[1]].used = true;
			}
		}
	}

	// setup trm vertices
	for( vert = tw.vertices, i = 0; i < tw.numVerts; i++, vert++ )
	{
		if( !vert->used )
		{
			continue;
		}
		// get axial trm size after rotations
		tw.size.AddPoint( vert->p - tw.start );
		// calculate the end position of each vertex for a full trace
		vert->endp = vert->p + tw.dir;
		// pluecker coordinate for vertex movement line
		vert->pl.FromRay( vert->p, tw.dir );
	}

	// setup trm edges
	for( edge = tw.edges + 1, i = 1; i <= tw.numEdges; i++, edge++ )
	{
		if( !edge->used )
		{
			continue;
		}
		// edge start, end and pluecker coordinate
		edge->start = tw.vertices[edge->vertexNum[0]].p;
		edge->end = tw.vertices[edge->vertexNum[1]].p;
		edge->pl.FromLine( edge->start, edge->end );
		// calculate normal of plane through movement plane created by the edge
		dir = edge->start - edge->end;
		edge->cross[0] = dir[0] * tw.dir[1] - dir[1] * tw.dir[0];
		edge->cross[1] = dir[0] * tw.dir[2] - dir[2] * tw.dir[0];
		edge->cross[2] = dir[1] * tw.dir[2] - dir[2] * tw.dir[1];
		// bit for vertex sidedness bit cache
		edge->bitNum = i;
	}

	// set trm plane distances
	for( poly = tw.polys, i = 0; i < tw.numPolys; i++, poly++ )
	{
		if( poly->used )
		{
			poly->plane.FitThroughPoint( tw.edges[abs( poly->edges[0] )].start );
		}
	}

	// bounds for full trace, a little bit larger for epsilons
	for( i = 0; i < 3; i++ )
	{
		if( tw.start[i] < tw.end[i] )
		{
			tw.bounds[0][i] = tw.start[i] + tw.size[0][i] - CM_BOX_EPSILON;
			tw.bounds[1][i] = tw.end[i] + tw.size[1][i] + CM_BOX_EPSILON;
		}
		else
		{
			tw.bounds[0][i] = tw.end[i] + tw.size[0][i] - CM_BOX_EPSILON;
			tw.bounds[1][i] = tw.start[i] + tw.size[1][i] + CM_BOX_EPSILON;
		}
		if( idMath::Fabs( tw.size[0][i] ) > idMath::Fabs( tw.size[1][i] ) )
		{
			tw.extents[i] = idMath::Fabs( tw.size[0][i] ) + CM_BOX_EPSILON;
		}
		else
		{
			tw.extents[i] = idMath::Fabs( tw.size[1][i] ) + CM_BOX_EPSILON;
		}
	}

	// setup trace heart planes
	idCollisionModelManagerLocal::SetupTranslationHeartPlanes( &tw );
	tw.maxDistFromHeartPlane1 = 0;
	tw.maxDistFromHeartPlane2 = 0;
	// calculate maximum trm vertex distance from both heart planes
	for( vert = tw.vertices, i = 0; i < tw.numVerts; i++, vert++ )
	{
		if( !vert->used )
		{
			continue;
		}
		dist = idMath::Fabs( tw.heartPlane1.Distance( vert->p ) );
		if( dist > tw.maxDistFromHeartPlane1 )
		{
			tw.maxDistFromHeartPlane1 = dist;
		}
		dist = idMath::Fabs( tw.heartPlane2.Distance( vert->p ) );
		if( dist > tw.maxDistFromHeartPlane2 )
		{
			tw.maxDistFromHeartPlane2 = dist;
		}
	}
	// for epsilons
	tw.maxDistFromHeartPlane1 += CM_BOX_EPSILON;
	tw.maxDistFromHeartPlane2 += CM_BOX_EPSILON;

	// trace through the model
	idCollisionModelManagerLocal::TraceThroughModel( &tw );

	// if we're getting contacts
	if( tw.getContacts )
	{
		// move all contacts to world space
		if( model_rotated )
		{
			for( i = 0; i < tw.numContacts; i++ )
			{
				tw.contacts[i].normal *= modelAxis;
				tw.contacts[i].point *= modelAxis;
			}
		}
		if( modelOrigin != vec3_origin )
		{
			for( i = 0; i < tw.numContacts; i++ )
			{
				tw.contacts[i].point += modelOrigin;
				tw.contacts[i].dist += modelOrigin * tw.contacts[i].normal;
			}
		}
		idCollisionModelManagerLocal::numContacts = tw.numContacts;
	}
	else
	{
		// store results
		*results = tw.trace;
		results->endpos = start + results->fraction * ( end - start );
		results->endAxis = trmAxis;

		if( results->fraction < 1.0f )
		{
			// if the fraction is tiny the actual movement could end up zero
			if( results->fraction > 0.0f && results->endpos.Compare( start ) )
			{
				results->fraction = 0.0f;
			}
			// rotate trace plane normal if there was a collision with a rotated model
			if( model_rotated )
			{
				results->c.normal *= modelAxis;
				results->c.point *= modelAxis;
			}
			results->c.point += modelOrigin;
			results->c.dist += modelOrigin * results->c.normal;
		}
	}

#ifdef _DEBUG
	// test for missed collisions
	if( cm_debugCollision.GetBool() )
	{
		if( !entered && !idCollisionModelManagerLocal::getContacts )
		{
			entered = 1;
			// if the trm is stuck in the model
			if( idCollisionModelManagerLocal::Contents( results->endpos, trm, trmAxis, -1, model, modelOrigin, modelAxis ) & contentMask )
			{
				trace_t tr;

				// test where the trm is stuck in the model
				idCollisionModelManagerLocal::Contents( results->endpos, trm, trmAxis, -1, model, modelOrigin, modelAxis );
				// re-run collision detection to find out where it failed
				idCollisionModelManagerLocal::Translation( &tr, start, end, trm, trmAxis, contentMask, model, modelOrigin, modelAxis );
			}
			entered = 0;
		}
	}
#endif
}
