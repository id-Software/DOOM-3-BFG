/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "precompiled.h"
#pragma hdrstop

#include "AASBuild_local.h"

#define LEDGE_EPSILON		0.1f

//===============================================================
//
//	idLedge
//
//===============================================================

/*
============
idLedge::idLedge
============
*/
idLedge::idLedge()
{
}

/*
============
idLedge::idLedge
============
*/
idLedge::idLedge( const idVec3& v1, const idVec3& v2, const idVec3& gravityDir, idBrushBSPNode* n )
{
	start = v1;
	end = v2;
	node = n;
	numPlanes = 4;
	planes[0].SetNormal( ( v1 - v2 ).Cross( gravityDir ) );
	planes[0].Normalize();
	planes[0].FitThroughPoint( v1 );
	planes[1].SetNormal( ( v1 - v2 ).Cross( planes[0].Normal() ) );
	planes[1].Normalize();
	planes[1].FitThroughPoint( v1 );
	planes[2].SetNormal( v1 - v2 );
	planes[2].Normalize();
	planes[2].FitThroughPoint( v1 );
	planes[3].SetNormal( v2 - v1 );
	planes[3].Normalize();
	planes[3].FitThroughPoint( v2 );
}

/*
============
idLedge::AddPoint
============
*/
void idLedge::AddPoint( const idVec3& v )
{
	if( planes[2].Distance( v ) > 0.0f )
	{
		start = v;
		planes[2].FitThroughPoint( start );
	}
	if( planes[3].Distance( v ) > 0.0f )
	{
		end = v;
		planes[3].FitThroughPoint( end );
	}
}

/*
============
idLedge::CreateBevels

  NOTE: this assumes the gravity is vertical
============
*/
void idLedge::CreateBevels( const idVec3& gravityDir )
{
	int i, j;
	idBounds bounds;
	idVec3 size, normal;

	bounds.Clear();
	bounds.AddPoint( start );
	bounds.AddPoint( end );
	size = bounds[1] - bounds[0];

	// plane through ledge
	planes[0].SetNormal( ( start - end ).Cross( gravityDir ) );
	planes[0].Normalize();
	planes[0].FitThroughPoint( start );
	// axial bevels at start and end point
	i = size[1] > size[0];
	normal = vec3_origin;
	normal[i] = 1.0f;
	j = end[i] > start[i];
	planes[1 + j].SetNormal( normal );
	planes[1 + !j].SetNormal( -normal );
	planes[1].FitThroughPoint( start );
	planes[2].FitThroughPoint( end );
	numExpandedPlanes = 3;
	// if additional bevels are required
	if( idMath::Fabs( size[!i] ) > 0.01f )
	{
		normal = vec3_origin;
		normal[!i] = 1.0f;
		j = end[!i] > start[!i];
		planes[3 + j].SetNormal( normal );
		planes[3 + !j].SetNormal( -normal );
		planes[3].FitThroughPoint( start );
		planes[4].FitThroughPoint( end );
		numExpandedPlanes = 5;
	}
	// opposite of first
	planes[numExpandedPlanes + 0] = -planes[0];
	// number of planes used for splitting
	numSplitPlanes = numExpandedPlanes + 1;
	// top plane
	planes[numSplitPlanes + 0].SetNormal( ( start - end ).Cross( planes[0].Normal() ) );
	planes[numSplitPlanes + 0].Normalize();
	planes[numSplitPlanes + 0].FitThroughPoint( start );
	// bottom plane
	planes[numSplitPlanes + 1] = -planes[numSplitPlanes + 0];
	// total number of planes
	numPlanes = numSplitPlanes + 2;
}

/*
============
idLedge::Expand
============
*/
void idLedge::Expand( const idBounds& bounds, float maxStepHeight )
{
	int i, j;
	idVec3 v;

	for( i = 0; i < numExpandedPlanes; i++ )
	{

		for( j = 0; j < 3; j++ )
		{
			if( planes[i].Normal()[j] > 0.0f )
			{
				v[j] = bounds[0][j];
			}
			else
			{
				v[j] = bounds[1][j];
			}
		}

		planes[i].SetDist( planes[i].Dist() + v * -planes[i].Normal() );
	}

	planes[numSplitPlanes + 0].SetDist( planes[numSplitPlanes + 0].Dist() + maxStepHeight );
	planes[numSplitPlanes + 1].SetDist( planes[numSplitPlanes + 1].Dist() + 1.0f );
}

/*
============
idLedge::ChopWinding
============
*/
idWinding* idLedge::ChopWinding( const idWinding* winding ) const
{
	int i;
	idWinding* w;

	w = winding->Copy();
	for( i = 0; i < numPlanes && w; i++ )
	{
		w = w->Clip( -planes[i], ON_EPSILON, true );
	}
	return w;
}

/*
============
idLedge::PointBetweenBounds
============
*/
bool idLedge::PointBetweenBounds( const idVec3& v ) const
{
	return ( planes[2].Distance( v ) < LEDGE_EPSILON ) && ( planes[3].Distance( v ) < LEDGE_EPSILON );
}


//===============================================================
//
//	idAASBuild
//
//===============================================================

/*
============
idAASBuild::LedgeSubdivFlood_r
============
*/
void idAASBuild::LedgeSubdivFlood_r( idBrushBSPNode* node, const idLedge* ledge )
{
	int s1, i;
	idBrushBSPPortal* p1;
	idWinding* w;
	idList<idBrushBSPNode*> nodeList;

	if( node->GetFlags() & NODE_VISITED )
	{
		return;
	}

	// if this is not already a ledge area
	if( !( node->GetFlags() & AREA_LEDGE ) )
	{
		for( p1 = node->GetPortals(); p1; p1 = p1->Next( s1 ) )
		{
			s1 = ( p1->GetNode( 1 ) == node );

			if( !( p1->GetFlags() & FACE_FLOOR ) )
			{
				continue;
			}

			// split the area if some part of the floor portal is inside the expanded ledge
			w = ledge->ChopWinding( p1->GetWinding() );
			if( !w )
			{
				continue;
			}
			delete w;

			for( i = 0; i < ledge->numSplitPlanes; i++ )
			{
				if( node->PlaneSide( ledge->planes[i], 0.1f ) != SIDE_CROSS )
				{
					continue;
				}
				if( !node->Split( ledge->planes[i], -1 ) )
				{
					continue;
				}
				numLedgeSubdivisions++;
				DisplayRealTimeString( "\r%6d", numLedgeSubdivisions );
				node->GetChild( 0 )->SetFlag( NODE_VISITED );
				LedgeSubdivFlood_r( node->GetChild( 1 ), ledge );
				return;
			}

			node->SetFlag( AREA_LEDGE );
			break;
		}
	}

	node->SetFlag( NODE_VISITED );

	// get all nodes we might need to flood into
	for( p1 = node->GetPortals(); p1; p1 = p1->Next( s1 ) )
	{
		s1 = ( p1->GetNode( 1 ) == node );

		if( p1->GetNode( !s1 )->GetContents() & AREACONTENTS_SOLID )
		{
			continue;
		}

		// flood through this portal if the portal is partly inside the expanded ledge
		w = ledge->ChopWinding( p1->GetWinding() );
		if( !w )
		{
			continue;
		}
		delete w;
		// add to list, cannot flood directly cause portals might be split on the way
		nodeList.Append( p1->GetNode( !s1 ) );
	}

	// flood into other nodes
	for( i = 0; i < nodeList.Num(); i++ )
	{
		LedgeSubdivLeafNodes_r( nodeList[i], ledge );
	}
}

/*
============
idAASBuild::LedgeSubdivLeafNodes_r

  The node the ledge was originally part of might be split by other ledges.
  Here we recurse down the tree from the original node to find all the new leaf nodes the ledge might be part of.
============
*/
void idAASBuild::LedgeSubdivLeafNodes_r( idBrushBSPNode* node, const idLedge* ledge )
{
	if( !node )
	{
		return;
	}
	if( !node->GetChild( 0 ) && !node->GetChild( 1 ) )
	{
		LedgeSubdivFlood_r( node, ledge );
		return;
	}
	LedgeSubdivLeafNodes_r( node->GetChild( 0 ), ledge );
	LedgeSubdivLeafNodes_r( node->GetChild( 1 ), ledge );
}

/*
============
idAASBuild::LedgeSubdiv
============
*/
void idAASBuild::LedgeSubdiv( idBrushBSPNode* root )
{
	int i, j;
	idBrush* brush;
	idList<idBrushSide*> sideList;

	// create ledge bevels and expand ledges
	for( i = 0; i < ledgeList.Num(); i++ )
	{

		ledgeList[i].CreateBevels( aasSettings->gravityDir );
		ledgeList[i].Expand( aasSettings->boundingBoxes[0], aasSettings->maxStepHeight );

		// if we should write out a ledge map
		if( ledgeMap )
		{
			sideList.SetNum( 0 );
			for( j = 0; j < ledgeList[i].numPlanes; j++ )
			{
				sideList.Append( new idBrushSide( ledgeList[i].planes[j], -1 ) );
			}

			brush = new idBrush();
			brush->FromSides( sideList );

			ledgeMap->WriteBrush( brush );

			delete brush;
		}

		// flood tree from the ledge node and subdivide areas with the ledge
		LedgeSubdivLeafNodes_r( ledgeList[i].node, &ledgeList[i] );

		// remove the node visited flags
		ledgeList[i].node->RemoveFlagRecurseFlood( NODE_VISITED );
	}
}

/*
============
idAASBuild::IsLedgeSide_r
============
*/
bool idAASBuild::IsLedgeSide_r( idBrushBSPNode* node, idFixedWinding* w, const idPlane& plane, const idVec3& normal, const idVec3& origin, const float radius )
{
	int res, i;
	idFixedWinding back;
	float dist;

	if( !node )
	{
		return false;
	}

	while( node->GetChild( 0 ) && node->GetChild( 1 ) )
	{
		dist = node->GetPlane().Distance( origin );
		if( dist > radius )
		{
			res = SIDE_FRONT;
		}
		else if( dist < -radius )
		{
			res = SIDE_BACK;
		}
		else
		{
			res = w->Split( &back, node->GetPlane(), LEDGE_EPSILON );
		}
		if( res == SIDE_FRONT )
		{
			node = node->GetChild( 0 );
		}
		else if( res == SIDE_BACK )
		{
			node = node->GetChild( 1 );
		}
		else if( res == SIDE_ON )
		{
			// continue with the side the winding faces
			if( node->GetPlane().Normal() * normal > 0.0f )
			{
				node = node->GetChild( 0 );
			}
			else
			{
				node = node->GetChild( 1 );
			}
		}
		else
		{
			if( IsLedgeSide_r( node->GetChild( 1 ), &back, plane, normal, origin, radius ) )
			{
				return true;
			}
			node = node->GetChild( 0 );
		}
	}

	if( node->GetContents() & AREACONTENTS_SOLID )
	{
		return false;
	}

	for( i = 0; i < w->GetNumPoints(); i++ )
	{
		if( plane.Distance( ( *w )[i].ToVec3() ) > 0.0f )
		{
			return true;
		}
	}

	return false;
}

/*
============
idAASBuild::AddLedge
============
*/
void idAASBuild::AddLedge( const idVec3& v1, const idVec3& v2, idBrushBSPNode* node )
{
	int i, j, merged;

	// first try to merge the ledge with existing ledges
	merged = -1;
	for( i = 0; i < ledgeList.Num(); i++ )
	{

		for( j = 0; j < 2; j++ )
		{
			if( idMath::Fabs( ledgeList[i].planes[j].Distance( v1 ) ) > LEDGE_EPSILON )
			{
				break;
			}
			if( idMath::Fabs( ledgeList[i].planes[j].Distance( v2 ) ) > LEDGE_EPSILON )
			{
				break;
			}
		}
		if( j < 2 )
		{
			continue;
		}

		if( !ledgeList[i].PointBetweenBounds( v1 ) &&
				!ledgeList[i].PointBetweenBounds( v2 ) )
		{
			continue;
		}

		if( merged == -1 )
		{
			ledgeList[i].AddPoint( v1 );
			ledgeList[i].AddPoint( v2 );
			merged = i;
		}
		else
		{
			ledgeList[merged].AddPoint( ledgeList[i].start );
			ledgeList[merged].AddPoint( ledgeList[i].end );
			ledgeList.RemoveIndex( i );
			break;
		}
	}

	// if the ledge could not be merged
	if( merged == -1 )
	{
		ledgeList.Append( idLedge( v1, v2, aasSettings->gravityDir, node ) );
	}
}

/*
============
idAASBuild::FindLeafNodeLedges
============
*/
void idAASBuild::FindLeafNodeLedges( idBrushBSPNode* root, idBrushBSPNode* node )
{
	int s1, i;
	idBrushBSPPortal* p1;
	idWinding* w;
	idVec3 v1, v2, normal, origin;
	idFixedWinding winding;
	idBounds bounds;
	idPlane plane;
	float radius;

	for( p1 = node->GetPortals(); p1; p1 = p1->Next( s1 ) )
	{
		s1 = ( p1->GetNode( 1 ) == node );

		if( !( p1->GetFlags() & FACE_FLOOR ) )
		{
			continue;
		}

		if( s1 )
		{
			plane = p1->GetPlane();
			w = p1->GetWinding()->Reverse();
		}
		else
		{
			plane = -p1->GetPlane();
			w = p1->GetWinding();
		}

		for( i = 0; i < w->GetNumPoints(); i++ )
		{

			v1 = ( *w )[i].ToVec3();
			v2 = ( *w )[( i + 1 ) % w->GetNumPoints()].ToVec3();
			normal = ( v2 - v1 ).Cross( aasSettings->gravityDir );
			if( normal.Normalize() < 0.5f )
			{
				continue;
			}

			winding.Clear();
			winding += v1 + normal * LEDGE_EPSILON * 0.5f;
			winding += v2 + normal * LEDGE_EPSILON * 0.5f;
			winding += winding[1].ToVec3() + ( aasSettings->maxStepHeight + 1.0f ) * aasSettings->gravityDir;
			winding += winding[0].ToVec3() + ( aasSettings->maxStepHeight + 1.0f ) * aasSettings->gravityDir;

			winding.GetBounds( bounds );
			origin = ( bounds[1] - bounds[0] ) * 0.5f;
			radius = origin.Length() + LEDGE_EPSILON;
			origin = bounds[0] + origin;

			plane.FitThroughPoint( v1 + aasSettings->maxStepHeight * aasSettings->gravityDir );

			if( !IsLedgeSide_r( root, &winding, plane, normal, origin, radius ) )
			{
				continue;
			}

			AddLedge( v1, v2, node );
		}

		if( w != p1->GetWinding() )
		{
			delete w;
		}
	}
}

/*
============
idAASBuild::FindLedges_r
============
*/
void idAASBuild::FindLedges_r( idBrushBSPNode* root, idBrushBSPNode* node )
{
	if( !node )
	{
		return;
	}

	if( node->GetContents() & AREACONTENTS_SOLID )
	{
		return;
	}

	if( !node->GetChild( 0 ) && !node->GetChild( 1 ) )
	{
		if( node->GetFlags() & NODE_VISITED )
		{
			return;
		}
		FindLeafNodeLedges( root, node );
		node->SetFlag( NODE_VISITED );
		return;
	}

	FindLedges_r( root, node->GetChild( 0 ) );
	FindLedges_r( root, node->GetChild( 1 ) );
}

/*
============
idAASBuild::WriteLedgeMap
============
*/
void idAASBuild::WriteLedgeMap( const idStr& fileName, const idStr& ext )
{
	ledgeMap = new idBrushMap( fileName, ext );
	ledgeMap->SetTexture( "textures/base_trim/bluetex4q_ed" );
}

/*
============
idAASBuild::LedgeSubdivision

  NOTE: this assumes the bounding box is higher than the maximum step height
		only ledges with vertical sides are considered
============
*/
void idAASBuild::LedgeSubdivision( idBrushBSP& bsp )
{
	numLedgeSubdivisions = 0;
	ledgeList.Clear();

	common->Printf( "[Ledge Subdivision]\n" );

	bsp.GetRootNode()->RemoveFlagRecurse( NODE_VISITED );
	FindLedges_r( bsp.GetRootNode(), bsp.GetRootNode() );
	bsp.GetRootNode()->RemoveFlagRecurse( NODE_VISITED );

	common->Printf( "\r%6d ledges\n", ledgeList.Num() );

	LedgeSubdiv( bsp.GetRootNode() );

	common->Printf( "\r%6d subdivisions\n", numLedgeSubdivisions );
}
