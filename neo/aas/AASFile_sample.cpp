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

#include "precompiled.h"
#pragma hdrstop

#include "AASFile.h"
#include "AASFile_local.h"

//===============================================================
//
//	Environment Sampling
//
//===============================================================

/*
================
idAASFileLocal::EdgeCenter
================
*/
idVec3 idAASFileLocal::EdgeCenter( int edgeNum ) const
{
	const aasEdge_t* edge;
	edge = &edges[edgeNum];
	return ( vertices[edge->vertexNum[0]] + vertices[edge->vertexNum[1]] ) * 0.5f;
}

/*
================
idAASFileLocal::FaceCenter
================
*/
idVec3 idAASFileLocal::FaceCenter( int faceNum ) const
{
	int i, edgeNum;
	const aasFace_t* face;
	const aasEdge_t* edge;
	idVec3 center;

	center = vec3_origin;

	face = &faces[faceNum];
	if( face->numEdges > 0 )
	{
		for( i = 0; i < face->numEdges; i++ )
		{
			edgeNum = edgeIndex[ face->firstEdge + i ];
			edge = &edges[ abs( edgeNum ) ];
			center += vertices[ edge->vertexNum[ INT32_SIGNBITSET( edgeNum ) ] ];
		}
		center /= face->numEdges;
	}
	return center;
}

/*
================
idAASFileLocal::AreaCenter
================
*/
idVec3 idAASFileLocal::AreaCenter( int areaNum ) const
{
	int i, faceNum;
	const aasArea_t* area;
	idVec3 center;

	center = vec3_origin;

	area = &areas[areaNum];
	if( area->numFaces > 0 )
	{
		for( i = 0; i < area->numFaces; i++ )
		{
			faceNum = faceIndex[area->firstFace + i];
			center += FaceCenter( abs( faceNum ) );
		}
		center /= area->numFaces;
	}
	return center;
}

/*
============
idAASFileLocal::AreaReachableGoal
============
*/
idVec3 idAASFileLocal::AreaReachableGoal( int areaNum ) const
{
	int i, faceNum, numFaces;
	const aasArea_t* area;
	idVec3 center;
	idVec3 start, end;
	aasTrace_t trace;

	area = &areas[areaNum];

	if( !( area->flags & ( AREA_REACHABLE_WALK | AREA_REACHABLE_FLY ) ) || ( area->flags & AREA_LIQUID ) )
	{
		return AreaCenter( areaNum );
	}

	center = vec3_origin;

	numFaces = 0;
	for( i = 0; i < area->numFaces; i++ )
	{
		faceNum = faceIndex[area->firstFace + i];
		if( !( faces[abs( faceNum )].flags & FACE_FLOOR ) )
		{
			continue;
		}
		center += FaceCenter( abs( faceNum ) );
		numFaces++;
	}
	if( numFaces > 0 )
	{
		center /= numFaces;
	}
	center[2] += 1.0f;
	end = center;
	end[2] -= 1024;
	Trace( trace, center, end );

	return trace.endpos;
}

/*
================
idAASFileLocal::EdgeBounds
================
*/
idBounds idAASFileLocal::EdgeBounds( int edgeNum ) const
{
	const aasEdge_t* edge;
	idBounds bounds;

	edge = &edges[ abs( edgeNum ) ];
	bounds[0] = bounds[1] = vertices[ edge->vertexNum[0] ];
	bounds += vertices[ edge->vertexNum[1] ];
	return bounds;
}

/*
================
idAASFileLocal::FaceBounds
================
*/
idBounds idAASFileLocal::FaceBounds( int faceNum ) const
{
	int i, edgeNum;
	const aasFace_t* face;
	const aasEdge_t* edge;
	idBounds bounds;

	face = &faces[faceNum];
	bounds.Clear();

	for( i = 0; i < face->numEdges; i++ )
	{
		edgeNum = edgeIndex[ face->firstEdge + i ];
		edge = &edges[ abs( edgeNum ) ];
		bounds.AddPoint( vertices[ edge->vertexNum[ INT32_SIGNBITSET( edgeNum ) ] ] );
	}
	return bounds;
}

/*
================
idAASFileLocal::AreaBounds
================
*/
idBounds idAASFileLocal::AreaBounds( int areaNum ) const
{
	int i, faceNum;
	const aasArea_t* area;
	idBounds bounds;

	area = &areas[areaNum];
	bounds.Clear();

	for( i = 0; i < area->numFaces; i++ )
	{
		faceNum = faceIndex[area->firstFace + i];
		bounds += FaceBounds( abs( faceNum ) );
	}
	return bounds;
}

/*
============
idAASFileLocal::PointAreaNum
============
*/
int idAASFileLocal::PointAreaNum( const idVec3& origin ) const
{
	int nodeNum;
	const aasNode_t* node;

	nodeNum = 1;
	do
	{
		node = &nodes[nodeNum];
		if( planeList[node->planeNum].Side( origin ) == PLANESIDE_BACK )
		{
			nodeNum = node->children[1];
		}
		else
		{
			nodeNum = node->children[0];
		}
		if( nodeNum < 0 )
		{
			return -nodeNum;
		}
	}
	while( nodeNum );

	return 0;
}

/*
============
idAASFileLocal::PointReachableAreaNum
============
*/
int idAASFileLocal::PointReachableAreaNum( const idVec3& origin, const idBounds& searchBounds, const int areaFlags, const int excludeTravelFlags ) const
{
	int areaList[32], areaNum, i;
	idVec3 start, end, pointList[32];
	aasTrace_t trace;
	idBounds bounds;
	float frac;

	start = origin;

	trace.areas = areaList;
	trace.points = pointList;
	trace.maxAreas = sizeof( areaList ) / sizeof( int );
	trace.getOutOfSolid = true;

	areaNum = PointAreaNum( start );
	if( areaNum )
	{
		if( ( areas[areaNum].flags & areaFlags ) && ( ( areas[areaNum].travelFlags & excludeTravelFlags ) == 0 ) )
		{
			return areaNum;
		}
	}
	else
	{
		// trace up
		end = start;
		end[2] += 32.0f;
		Trace( trace, start, end );
		if( trace.numAreas >= 1 )
		{
			if( ( areas[0].flags & areaFlags ) && ( ( areas[0].travelFlags & excludeTravelFlags ) == 0 ) )
			{
				return areaList[0];
			}
			start = pointList[0];
			start[2] += 1.0f;
		}
	}

	// trace down
	end = start;
	end[2] -= 32.0f;
	Trace( trace, start, end );
	if( trace.lastAreaNum )
	{
		if( ( areas[trace.lastAreaNum].flags & areaFlags ) && ( ( areas[trace.lastAreaNum].travelFlags & excludeTravelFlags ) == 0 ) )
		{
			return trace.lastAreaNum;
		}
		start = trace.endpos;
	}

	// expand bounds until an area is found
	for( i = 1; i <= 12; i++ )
	{
		frac = i * ( 1.0f / 12.0f );
		bounds[0] = origin + searchBounds[0] * frac;
		bounds[1] = origin + searchBounds[1] * frac;
		areaNum = BoundsReachableAreaNum( bounds, areaFlags, excludeTravelFlags );
		if( areaNum && ( areas[areaNum].flags & areaFlags ) && ( ( areas[areaNum].travelFlags & excludeTravelFlags ) == 0 ) )
		{
			return areaNum;
		}
	}
	return 0;
}

/*
============
idAASFileLocal::BoundsReachableAreaNum_r
============
*/
int idAASFileLocal::BoundsReachableAreaNum_r( int nodeNum, const idBounds& bounds, const int areaFlags, const int excludeTravelFlags ) const
{
	int res;
	const aasNode_t* node;

	while( nodeNum )
	{
		if( nodeNum < 0 )
		{
			if( ( areas[-nodeNum].flags & areaFlags ) && ( ( areas[-nodeNum].travelFlags & excludeTravelFlags ) == 0 ) )
			{
				return -nodeNum;
			}
			return 0;
		}
		node = &nodes[nodeNum];
		res = bounds.PlaneSide( planeList[node->planeNum] );
		if( res == PLANESIDE_BACK )
		{
			nodeNum = node->children[1];
		}
		else if( res == PLANESIDE_FRONT )
		{
			nodeNum = node->children[0];
		}
		else
		{
			nodeNum = BoundsReachableAreaNum_r( node->children[1], bounds, areaFlags, excludeTravelFlags );
			if( nodeNum )
			{
				return nodeNum;
			}
			nodeNum = node->children[0];
		}
	}

	return 0;
}

/*
============
idAASFileLocal::BoundsReachableAreaNum
============
*/
int idAASFileLocal::BoundsReachableAreaNum( const idBounds& bounds, const int areaFlags, const int excludeTravelFlags ) const
{

	return BoundsReachableAreaNum_r( 1, bounds, areaFlags, excludeTravelFlags );
}

/*
============
idAASFileLocal::PushPointIntoAreaNum
============
*/
void idAASFileLocal::PushPointIntoAreaNum( int areaNum, idVec3& point ) const
{
	int i, faceNum;
	const aasArea_t* area;
	const aasFace_t* face;

	area = &areas[areaNum];

	// push the point to the right side of all area face planes
	for( i = 0; i < area->numFaces; i++ )
	{
		faceNum = faceIndex[area->firstFace + i];
		face = &faces[abs( faceNum )];

		const idPlane& plane = planeList[face->planeNum ^ INT32_SIGNBITSET( faceNum )];
		float dist = plane.Distance( point );

		// project the point onto the face plane if it is on the wrong side
		if( dist < 0.0f )
		{
			point -= dist * plane.Normal();
		}
	}
}

/*
============
idAASFileLocal::Trace
============
*/
#define TRACEPLANE_EPSILON		0.125f

typedef struct aasTraceStack_s
{
	idVec3			start;
	idVec3			end;
	int				planeNum;
	int				nodeNum;
} aasTraceStack_t;

bool idAASFileLocal::Trace( aasTrace_t& trace, const idVec3& start, const idVec3& end ) const
{
	int side, nodeNum, tmpPlaneNum;
	double front, back, frac;
	idVec3 cur_start, cur_end, cur_mid, v1, v2;
	aasTraceStack_t tracestack[MAX_AAS_TREE_DEPTH];
	aasTraceStack_t* tstack_p;
	const aasNode_t* node;
	const idPlane* plane;

	trace.numAreas = 0;
	trace.lastAreaNum = 0;
	trace.blockingAreaNum = 0;

	tstack_p = tracestack;
	tstack_p->start = start;
	tstack_p->end = end;
	tstack_p->planeNum = 0;
	tstack_p->nodeNum = 1;		//start with the root of the tree
	tstack_p++;

	while( 1 )
	{

		tstack_p--;
		// if the trace stack is empty
		if( tstack_p < tracestack )
		{
			if( !trace.lastAreaNum )
			{
				// completely in solid
				trace.fraction = 0.0f;
				trace.endpos = start;
			}
			else
			{
				// nothing was hit
				trace.fraction = 1.0f;
				trace.endpos = end;
			}
			trace.planeNum = 0;
			return false;
		}

		// number of the current node to test the line against
		nodeNum = tstack_p->nodeNum;

		// if it is an area
		if( nodeNum < 0 )
		{
			// if can't enter the area
			if( ( areas[-nodeNum].flags & trace.flags ) || ( areas[-nodeNum].travelFlags & trace.travelFlags ) )
			{
				if( !trace.lastAreaNum )
				{
					trace.fraction = 0.0f;
					v1 = vec3_origin;
				}
				else
				{
					v1 = end - start;
					v2 = tstack_p->start - start;
					trace.fraction = v2.Length() / v1.Length();
				}
				trace.endpos = tstack_p->start;
				trace.blockingAreaNum = -nodeNum;
				trace.planeNum = tstack_p->planeNum;
				// always take the plane with normal facing towards the trace start
				plane = &planeList[trace.planeNum];
				if( v1 * plane->Normal() > 0.0f )
				{
					trace.planeNum ^= 1;
				}
				return true;
			}
			trace.lastAreaNum = -nodeNum;
			if( trace.numAreas < trace.maxAreas )
			{
				if( trace.areas )
				{
					trace.areas[trace.numAreas] = -nodeNum;
				}
				if( trace.points )
				{
					trace.points[trace.numAreas] = tstack_p->start;
				}
				trace.numAreas++;
			}
			continue;
		}

		// if it is a solid leaf
		if( !nodeNum )
		{
			if( !trace.lastAreaNum )
			{
				trace.fraction = 0.0f;
				v1 = vec3_origin;
			}
			else
			{
				v1 = end - start;
				v2 = tstack_p->start - start;
				trace.fraction = v2.Length() / v1.Length();
			}
			trace.endpos = tstack_p->start;
			trace.blockingAreaNum = 0;	// hit solid leaf
			trace.planeNum = tstack_p->planeNum;
			// always take the plane with normal facing towards the trace start
			plane = &planeList[trace.planeNum];
			if( v1 * plane->Normal() > 0.0f )
			{
				trace.planeNum ^= 1;
			}
			if( !trace.lastAreaNum && trace.getOutOfSolid )
			{
				continue;
			}
			else
			{
				return true;
			}
		}

		// the node to test against
		node = &nodes[nodeNum];
		// start point of current line to test against node
		cur_start = tstack_p->start;
		// end point of the current line to test against node
		cur_end = tstack_p->end;
		// the current node plane
		plane = &planeList[node->planeNum];

		front = plane->Distance( cur_start );
		back = plane->Distance( cur_end );

		// if the whole to be traced line is totally at the front of this node
		// only go down the tree with the front child
		if( front >= -ON_EPSILON && back >= -ON_EPSILON )
		{
			// keep the current start and end point on the stack and go down the tree with the front child
			tstack_p->nodeNum = node->children[0];
			tstack_p++;
			if( tstack_p >= &tracestack[MAX_AAS_TREE_DEPTH] )
			{
				common->Error( "idAASFileLocal::Trace: stack overflow\n" );
				return false;
			}
		}
		// if the whole to be traced line is totally at the back of this node
		// only go down the tree with the back child
		else if( front < ON_EPSILON && back < ON_EPSILON )
		{
			// keep the current start and end point on the stack and go down the tree with the back child
			tstack_p->nodeNum = node->children[1];
			tstack_p++;
			if( tstack_p >= &tracestack[MAX_AAS_TREE_DEPTH] )
			{
				common->Error( "idAASFileLocal::Trace: stack overflow\n" );
				return false;
			}
		}
		// go down the tree both at the front and back of the node
		else
		{
			tmpPlaneNum = tstack_p->planeNum;
			// calculate the hit point with the node plane
			// put the cross point TRACEPLANE_EPSILON on the near side
			if( front < 0 )
			{
				frac = ( front + TRACEPLANE_EPSILON ) / ( front - back );
			}
			else
			{
				frac = ( front - TRACEPLANE_EPSILON ) / ( front - back );
			}

			if( frac < 0 )
			{
				frac = 0.001f; //0
			}
			else if( frac > 1 )
			{
				frac = 0.999f; //1
			}

			cur_mid = cur_start + ( cur_end - cur_start ) * frac;

			// side the front part of the line is on
			side = front < 0;

			// first put the end part of the line on the stack (back side)
			tstack_p->start = cur_mid;
			tstack_p->planeNum = node->planeNum;
			tstack_p->nodeNum = node->children[!side];
			tstack_p++;
			if( tstack_p >= &tracestack[MAX_AAS_TREE_DEPTH] )
			{
				common->Error( "idAASFileLocal::Trace: stack overflow\n" );
				return false;
			}
			// now put the part near the start of the line on the stack so we will
			// continue with that part first.
			tstack_p->start = cur_start;
			tstack_p->end = cur_mid;
			tstack_p->planeNum = tmpPlaneNum;
			tstack_p->nodeNum = node->children[side];
			tstack_p++;
			if( tstack_p >= &tracestack[MAX_AAS_TREE_DEPTH] )
			{
				common->Error( "idAASFileLocal::Trace: stack overflow\n" );
				return false;
			}
		}
	}
	return false;
}

/*
============
idAASLocal::AreaContentsTravelFlags
============
*/
int idAASFileLocal::AreaContentsTravelFlags( int areaNum ) const
{
	if( areas[areaNum].contents & AREACONTENTS_WATER )
	{
		return TFL_WATER;
	}
	return TFL_AIR;
}

/*
============
idAASFileLocal::MaxTreeDepth_r
============
*/
void idAASFileLocal::MaxTreeDepth_r( int nodeNum, int& depth, int& maxDepth ) const
{
	const aasNode_t* node;

	if( nodeNum <= 0 )
	{
		return;
	}

	depth++;
	if( depth > maxDepth )
	{
		maxDepth = depth;
	}

	node = &nodes[nodeNum];
	MaxTreeDepth_r( node->children[0], depth, maxDepth );
	MaxTreeDepth_r( node->children[1], depth, maxDepth );

	depth--;
}

/*
============
idAASFileLocal::MaxTreeDepth
============
*/
int idAASFileLocal::MaxTreeDepth() const
{
	int depth, maxDepth;

	depth = maxDepth = 0;
	MaxTreeDepth_r( 1, depth, maxDepth );
	return maxDepth;
}
