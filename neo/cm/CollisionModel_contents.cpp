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

Contents test

===============================================================================
*/

/*
================
idCollisionModelManagerLocal::TestTrmVertsInBrush

  returns true if any of the trm vertices is inside the brush
================
*/
bool idCollisionModelManagerLocal::TestTrmVertsInBrush( cm_traceWork_t* tw, cm_brush_t* b )
{
	int i, j, numVerts, bestPlane;
	float d, bestd;
	idVec3* p;

	if( b->checkcount == idCollisionModelManagerLocal::checkCount )
	{
		return false;
	}
	b->checkcount = idCollisionModelManagerLocal::checkCount;

	if( !( b->contents & tw->contents ) )
	{
		return false;
	}

	// if the brush bounds don't intersect the trace bounds
	if( !b->bounds.IntersectsBounds( tw->bounds ) )
	{
		return false;
	}

	if( tw->pointTrace )
	{
		numVerts = 1;
	}
	else
	{
		numVerts = tw->numVerts;
	}

	for( j = 0; j < numVerts; j++ )
	{
		p = &tw->vertices[j].p;

		// see if the point is inside the brush
		bestPlane = 0;
		bestd = -idMath::INFINITUM;
		for( i = 0; i < b->numPlanes; i++ )
		{
			d = b->planes[i].Distance( *p );
			if( d >= 0.0f )
			{
				break;
			}
			if( d > bestd )
			{
				bestd = d;
				bestPlane = i;
			}
		}
		if( i >= b->numPlanes )
		{
			tw->trace.fraction = 0.0f;
			tw->trace.c.type = CONTACT_TRMVERTEX;
			tw->trace.c.normal = b->planes[bestPlane].Normal();
			tw->trace.c.dist = b->planes[bestPlane].Dist();
			tw->trace.c.contents = b->contents;
			tw->trace.c.material = b->material;
			tw->trace.c.point = *p;
			tw->trace.c.modelFeature = 0;
			tw->trace.c.trmFeature = j;
			return true;
		}
	}
	return false;
}

/*
================
CM_SetTrmEdgeSidedness
================
*/
#define CM_SetTrmEdgeSidedness( edge, bpl, epl, bitNum ) {						\
	const int mask = 1 << bitNum;												\
	if ( ( edge->sideSet & mask ) == 0 ) {										\
		const float fl = (bpl).PermutedInnerProduct( epl );						\
		edge->side = ( edge->side & ~mask ) | ( ( fl < 0.0f ) ? mask : 0 );		\
		edge->sideSet |= mask;													\
	}																			\
}

/*
================
CM_SetTrmPolygonSidedness
================
*/
#define CM_SetTrmPolygonSidedness( v, plane, bitNum ) {						\
	const int mask = 1 << bitNum;											\
	if ( ( (v)->sideSet & mask ) == 0 ) {									\
		const float fl = plane.Distance( (v)->p );							\
		(v)->side = ( (v)->side & ~mask ) | ( ( fl < 0.0f ) ? mask : 0 );		\
		(v)->sideSet |= mask;												\
	}																		\
}

/*
================
idCollisionModelManagerLocal::TestTrmInPolygon

  returns true if the trm intersects the polygon
================
*/
bool idCollisionModelManagerLocal::TestTrmInPolygon( cm_traceWork_t* tw, cm_polygon_t* p )
{
	int i, j, k, edgeNum, flip, trmEdgeNum, bitNum, bestPlane;
	int sides[MAX_TRACEMODEL_VERTS];
	float d, bestd;
	cm_trmEdge_t* trmEdge;
	cm_edge_t* edge;
	cm_vertex_t* v, *v1, *v2;

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

	// if the polygon bounds don't intersect the trace bounds
	if( !p->bounds.IntersectsBounds( tw->bounds ) )
	{
		return false;
	}

	// bounds should cross polygon plane
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

	// if the trace model is convex
	if( tw->isConvex )
	{
		// test if any polygon vertices are inside the trm
		for( i = 0; i < p->numEdges; i++ )
		{
			edgeNum = p->edges[i];
			edge = tw->model->edges + abs( edgeNum );
			// if this edge is already tested
			if( edge->checkcount == idCollisionModelManagerLocal::checkCount )
			{
				continue;
			}

			for( j = 0; j < 2; j++ )
			{
				v = &tw->model->vertices[edge->vertexNum[j]];
				// if this vertex is already tested
				if( v->checkcount == idCollisionModelManagerLocal::checkCount )
				{
					continue;
				}

				bestPlane = 0;
				bestd = -idMath::INFINITUM;
				for( k = 0; k < tw->numPolys; k++ )
				{
					d = tw->polys[k].plane.Distance( v->p );
					if( d >= 0.0f )
					{
						break;
					}
					if( d > bestd )
					{
						bestd = d;
						bestPlane = k;
					}
				}
				if( k >= tw->numPolys )
				{
					tw->trace.fraction = 0.0f;
					tw->trace.c.type = CONTACT_MODELVERTEX;
					tw->trace.c.normal = -tw->polys[bestPlane].plane.Normal();
					tw->trace.c.dist = -tw->polys[bestPlane].plane.Dist();
					tw->trace.c.contents = p->contents;
					tw->trace.c.material = p->material;
					tw->trace.c.point = v->p;
					tw->trace.c.modelFeature = edge->vertexNum[j];
					tw->trace.c.trmFeature = 0;
					return true;
				}
			}
		}
	}

	for( i = 0; i < p->numEdges; i++ )
	{
		edgeNum = p->edges[i];
		edge = tw->model->edges + abs( edgeNum );
		// reset sidedness cache if this is the first time we encounter this edge
		if( edge->checkcount != idCollisionModelManagerLocal::checkCount )
		{
			edge->sideSet = 0;
		}
		// pluecker coordinate for edge
		tw->polygonEdgePlueckerCache[i].FromLine( tw->model->vertices[edge->vertexNum[0]].p,
				tw->model->vertices[edge->vertexNum[1]].p );
		v = &tw->model->vertices[edge->vertexNum[INT32_SIGNBITSET( edgeNum )]];
		// reset sidedness cache if this is the first time we encounter this vertex
		if( v->checkcount != idCollisionModelManagerLocal::checkCount )
		{
			v->sideSet = 0;
		}
		v->checkcount = idCollisionModelManagerLocal::checkCount;
	}

	// get side of polygon for each trm vertex
	for( i = 0; i < tw->numVerts; i++ )
	{
		d = p->plane.Distance( tw->vertices[i].p );
		sides[i] = d < 0.0f ? -1 : 1;
	}

	// test if any trm edges go through the polygon
	for( i = 1; i <= tw->numEdges; i++ )
	{
		// if the trm edge does not cross the polygon plane
		if( sides[tw->edges[i].vertexNum[0]] == sides[tw->edges[i].vertexNum[1]] )
		{
			continue;
		}
		// check from which side to which side the trm edge goes
		flip = INT32_SIGNBITSET( sides[tw->edges[i].vertexNum[0]] );
		// test if trm edge goes through the polygon between the polygon edges
		for( j = 0; j < p->numEdges; j++ )
		{
			edgeNum = p->edges[j];
			edge = tw->model->edges + abs( edgeNum );
#if 1
			CM_SetTrmEdgeSidedness( edge, tw->edges[i].pl, tw->polygonEdgePlueckerCache[j], i );
			if( INT32_SIGNBITSET( edgeNum ) ^ ( ( edge->side >> i ) & 1 ) ^ flip )
			{
				break;
			}
#else
			d = tw->edges[i].pl.PermutedInnerProduct( tw->polygonEdgePlueckerCache[j] );
			if( flip )
			{
				d = -d;
			}
			if( edgeNum > 0 )
			{
				if( d <= 0.0f )
				{
					break;
				}
			}
			else
			{
				if( d >= 0.0f )
				{
					break;
				}
			}
#endif
		}
		if( j >= p->numEdges )
		{
			tw->trace.fraction = 0.0f;
			tw->trace.c.type = CONTACT_EDGE;
			tw->trace.c.normal = p->plane.Normal();
			tw->trace.c.dist = p->plane.Dist();
			tw->trace.c.contents = p->contents;
			tw->trace.c.material = p->material;
			tw->trace.c.point = tw->vertices[tw->edges[i].vertexNum[ !flip ]].p;
			tw->trace.c.modelFeature = *reinterpret_cast<int*>( &p );
			tw->trace.c.trmFeature = i;
			return true;
		}
	}

	// test if any polygon edges go through the trm polygons
	for( i = 0; i < p->numEdges; i++ )
	{
		edgeNum = p->edges[i];
		edge = tw->model->edges + abs( edgeNum );
		if( edge->checkcount == idCollisionModelManagerLocal::checkCount )
		{
			continue;
		}
		edge->checkcount = idCollisionModelManagerLocal::checkCount;

		for( j = 0; j < tw->numPolys; j++ )
		{
#if 1
			v1 = tw->model->vertices + edge->vertexNum[0];
			CM_SetTrmPolygonSidedness( v1, tw->polys[j].plane, j );
			v2 = tw->model->vertices + edge->vertexNum[1];
			CM_SetTrmPolygonSidedness( v2, tw->polys[j].plane, j );
			// if the polygon edge does not cross the trm polygon plane
			if( !( ( ( v1->side ^ v2->side ) >> j ) & 1 ) )
			{
				continue;
			}
			flip = ( v1->side >> j ) & 1;
#else
			float d1, d2;

			v1 = tw->model->vertices + edge->vertexNum[0];
			d1 = tw->polys[j].plane.Distance( v1->p );
			v2 = tw->model->vertices + edge->vertexNum[1];
			d2 = tw->polys[j].plane.Distance( v2->p );
			// if the polygon edge does not cross the trm polygon plane
			if( ( d1 >= 0.0f && d2 >= 0.0f ) || ( d1 <= 0.0f && d2 <= 0.0f ) )
			{
				continue;
			}
			flip = false;
			if( d1 < 0.0f )
			{
				flip = true;
			}
#endif
			// test if polygon edge goes through the trm polygon between the trm polygon edges
			for( k = 0; k < tw->polys[j].numEdges; k++ )
			{
				trmEdgeNum = tw->polys[j].edges[k];
				trmEdge = tw->edges + abs( trmEdgeNum );
#if 1
				bitNum = abs( trmEdgeNum );
				CM_SetTrmEdgeSidedness( edge, trmEdge->pl, tw->polygonEdgePlueckerCache[i], bitNum );
				if( INT32_SIGNBITSET( trmEdgeNum ) ^ ( ( edge->side >> bitNum ) & 1 ) ^ flip )
				{
					break;
				}
#else
				d = trmEdge->pl.PermutedInnerProduct( tw->polygonEdgePlueckerCache[i] );
				if( flip )
				{
					d = -d;
				}
				if( trmEdgeNum > 0 )
				{
					if( d <= 0.0f )
					{
						break;
					}
				}
				else
				{
					if( d >= 0.0f )
					{
						break;
					}
				}
#endif
			}
			if( k >= tw->polys[j].numEdges )
			{
				tw->trace.fraction = 0.0f;
				tw->trace.c.type = CONTACT_EDGE;
				tw->trace.c.normal = -tw->polys[j].plane.Normal();
				tw->trace.c.dist = -tw->polys[j].plane.Dist();
				tw->trace.c.contents = p->contents;
				tw->trace.c.material = p->material;
				tw->trace.c.point = tw->model->vertices[edge->vertexNum[ !flip ]].p;
				tw->trace.c.modelFeature = edgeNum;
				tw->trace.c.trmFeature = j;
				return true;
			}
		}
	}
	return false;
}

/*
================
idCollisionModelManagerLocal::PointNode
================
*/
cm_node_t* idCollisionModelManagerLocal::PointNode( const idVec3& p, cm_model_t* model )
{
	cm_node_t* node;

	node = model->node;
	while( node->planeType != -1 )
	{
		if( p[node->planeType] > node->planeDist )
		{
			node = node->children[0];
		}
		else
		{
			node = node->children[1];
		}

		assert( node != NULL );
	}
	return node;
}

/*
================
idCollisionModelManagerLocal::PointContents
================
*/
int idCollisionModelManagerLocal::PointContents( const idVec3 p, cmHandle_t model )
{
	int i;
	float d;
	cm_node_t* node;
	cm_brushRef_t* bref;
	cm_brush_t* b;
	idPlane* plane;

	node = idCollisionModelManagerLocal::PointNode( p, idCollisionModelManagerLocal::models[model] );
	for( bref = node->brushes; bref; bref = bref->next )
	{
		b = bref->b;
		// test if the point is within the brush bounds
		for( i = 0; i < 3; i++ )
		{
			if( p[i] < b->bounds[0][i] )
			{
				break;
			}
			if( p[i] > b->bounds[1][i] )
			{
				break;
			}
		}
		if( i < 3 )
		{
			continue;
		}
		// test if the point is inside the brush
		plane = b->planes;
		for( i = 0; i < b->numPlanes; i++, plane++ )
		{
			d = plane->Distance( p );
			if( d >= 0.0f )
			{
				break;
			}
		}
		if( i >= b->numPlanes )
		{
			return b->contents;
		}
	}
	return 0;
}

/*
==================
idCollisionModelManagerLocal::TransformedPointContents
==================
*/
int	idCollisionModelManagerLocal::TransformedPointContents( const idVec3& p, cmHandle_t model, const idVec3& origin, const idMat3& modelAxis )
{
	idVec3 p_l;

	// subtract origin offset
	p_l = p - origin;
	if( modelAxis.IsRotated() )
	{
		p_l *= modelAxis;
	}
	return idCollisionModelManagerLocal::PointContents( p_l, model );
}


/*
==================
idCollisionModelManagerLocal::ContentsTrm
==================
*/
int idCollisionModelManagerLocal::ContentsTrm( trace_t* results, const idVec3& start,
		const idTraceModel* trm, const idMat3& trmAxis, int contentMask,
		cmHandle_t model, const idVec3& modelOrigin, const idMat3& modelAxis )
{
	int i;
	bool model_rotated, trm_rotated;
	idMat3 invModelAxis, tmpAxis;
	idVec3 dir;
	ALIGN16( cm_traceWork_t tw );

	// fast point case
	if( !trm || ( trm->bounds[1][0] - trm->bounds[0][0] <= 0.0f &&
				  trm->bounds[1][1] - trm->bounds[0][1] <= 0.0f &&
				  trm->bounds[1][2] - trm->bounds[0][2] <= 0.0f ) )
	{

		results->c.contents = idCollisionModelManagerLocal::TransformedPointContents( start, model, modelOrigin, modelAxis );
		results->fraction = ( results->c.contents == 0 );
		results->endpos = start;
		results->endAxis = trmAxis;

		return results->c.contents;
	}

	idCollisionModelManagerLocal::checkCount++;

	tw.trace.fraction = 1.0f;
	tw.trace.c.contents = 0;
	tw.trace.c.type = CONTACT_NONE;
	tw.contents = contentMask;
	tw.isConvex = true;
	tw.rotation = false;
	tw.positionTest = true;
	tw.pointTrace = false;
	tw.quickExit = false;
	tw.numContacts = 0;
	tw.model = idCollisionModelManagerLocal::models[model];
	tw.start = start - modelOrigin;
	tw.end = tw.start;

	model_rotated = modelAxis.IsRotated();
	if( model_rotated )
	{
		invModelAxis = modelAxis.Transpose();
	}

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
	}


	// setup trm vertices
	tw.size.Clear();
	for( i = 0; i < tw.numVerts; i++ )
	{
		// get axial trm size after rotations
		tw.size.AddPoint( tw.vertices[i].p - tw.start );
	}

	// setup trm edges
	for( i = 1; i <= tw.numEdges; i++ )
	{
		// edge start, end and pluecker coordinate
		tw.edges[i].start = tw.vertices[tw.edges[i].vertexNum[0]].p;
		tw.edges[i].end = tw.vertices[tw.edges[i].vertexNum[1]].p;
		tw.edges[i].pl.FromLine( tw.edges[i].start, tw.edges[i].end );
	}

	// setup trm polygons
	if( trm_rotated & model_rotated )
	{
		tmpAxis = trmAxis * invModelAxis;
		for( i = 0; i < tw.numPolys; i++ )
		{
			tw.polys[i].plane *= tmpAxis;
		}
	}
	else if( trm_rotated )
	{
		for( i = 0; i < tw.numPolys; i++ )
		{
			tw.polys[i].plane *= trmAxis;
		}
	}
	else if( model_rotated )
	{
		for( i = 0; i < tw.numPolys; i++ )
		{
			tw.polys[i].plane *= invModelAxis;
		}
	}
	for( i = 0; i < tw.numPolys; i++ )
	{
		tw.polys[i].plane.FitThroughPoint( tw.edges[abs( tw.polys[i].edges[0] )].start );
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

	// trace through the model
	idCollisionModelManagerLocal::TraceThroughModel( &tw );

	*results = tw.trace;
	results->fraction = ( results->c.contents == 0 );
	results->endpos = start;
	results->endAxis = trmAxis;

	return results->c.contents;
}

/*
==================
idCollisionModelManagerLocal::Contents
==================
*/
int idCollisionModelManagerLocal::Contents( const idVec3& start,
		const idTraceModel* trm, const idMat3& trmAxis, int contentMask,
		cmHandle_t model, const idVec3& modelOrigin, const idMat3& modelAxis )
{
	trace_t results;

	if( model < 0 || model > idCollisionModelManagerLocal::maxModels || model > MAX_SUBMODELS )
	{
		common->Printf( "idCollisionModelManagerLocal::Contents: invalid model handle\n" );
		return 0;
	}
	if( !idCollisionModelManagerLocal::models || !idCollisionModelManagerLocal::models[model] )
	{
		common->Printf( "idCollisionModelManagerLocal::Contents: invalid model\n" );
		return 0;
	}

	return ContentsTrm( &results, start, trm, trmAxis, contentMask, model, modelOrigin, modelAxis );
}
