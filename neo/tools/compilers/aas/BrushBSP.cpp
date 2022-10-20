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

#include "Brush.h"
#include "BrushBSP.h"


#define BSP_GRID_SIZE					512.0f
#define SPLITTER_EPSILON				0.1f
#define VERTEX_MELT_EPSILON				0.1f
#define VERTEX_MELT_HASH_SIZE			32

#define PORTAL_PLANE_NORMAL_EPSILON		0.00001f
#define PORTAL_PLANE_DIST_EPSILON		0.01f

//#define OUPUT_BSP_STATS_PER_GRID_CELL


//===============================================================
//
//	idBrushBSPPortal
//
//===============================================================

/*
============
idBrushBSPPortal::idBrushBSPPortal
============
*/
idBrushBSPPortal::idBrushBSPPortal()
{
	planeNum = -1;
	winding = NULL;
	nodes[0] = nodes[1] = NULL;
	next[0] = next[1] = NULL;
	faceNum = 0;
	flags = 0;
}

/*
============
idBrushBSPPortal::~idBrushBSPPortal
============
*/
idBrushBSPPortal::~idBrushBSPPortal()
{
	if( winding )
	{
		delete winding;
	}
}

/*
============
idBrushBSPPortal::AddToNodes
============
*/
void idBrushBSPPortal::AddToNodes( idBrushBSPNode* front, idBrushBSPNode* back )
{
	if( nodes[0] || nodes[1] )
	{
		common->Error( "AddToNode: allready included" );
	}

	assert( front && back );

	nodes[0] = front;
	next[0] = front->portals;
	front->portals = this;

	nodes[1] = back;
	next[1] = back->portals;
	back->portals = this;
}

/*
============
idBrushBSPPortal::RemoveFromNode
============
*/
void idBrushBSPPortal::RemoveFromNode( idBrushBSPNode* l )
{
	idBrushBSPPortal** pp, *t;

	// remove reference to the current portal
	pp = &l->portals;
	while( 1 )
	{
		t = *pp;
		if( !t )
		{
			common->Error( "idBrushBSPPortal::RemoveFromNode: portal not in node" );
		}

		if( t == this )
		{
			break;
		}

		if( t->nodes[0] == l )
		{
			pp = &t->next[0];
		}
		else if( t->nodes[1] == l )
		{
			pp = &t->next[1];
		}
		else
		{
			common->Error( "idBrushBSPPortal::RemoveFromNode: portal not bounding node" );
		}
	}

	if( nodes[0] == l )
	{
		*pp = next[0];
		nodes[0] = NULL;
	}
	else if( nodes[1] == l )
	{
		*pp = next[1];
		nodes[1] = NULL;
	}
	else
	{
		common->Error( "idBrushBSPPortal::RemoveFromNode: mislinked portal" );
	}
}

/*
============
idBrushBSPPortal::Flip
============
*/
void idBrushBSPPortal::Flip()
{
	idBrushBSPNode* frontNode, *backNode;

	frontNode = nodes[0];
	backNode = nodes[1];

	if( frontNode )
	{
		RemoveFromNode( frontNode );
	}
	if( backNode )
	{
		RemoveFromNode( backNode );
	}
	AddToNodes( frontNode, backNode );

	plane = -plane;
	planeNum ^= 1;
	winding->ReverseSelf();
}

/*
============
idBrushBSPPortal::Split
============
*/
int idBrushBSPPortal::Split( const idPlane& splitPlane, idBrushBSPPortal** front, idBrushBSPPortal** back )
{
	idWinding* frontWinding, *backWinding;

	( *front ) = ( *back ) = NULL;
	winding->Split( splitPlane, 0.1f, &frontWinding, &backWinding );
	if( frontWinding )
	{
		( *front ) = new idBrushBSPPortal();
		( *front )->plane = plane;
		( *front )->planeNum = planeNum;
		( *front )->flags = flags;
		( *front )->winding = frontWinding;
	}
	if( backWinding )
	{
		( *back ) = new idBrushBSPPortal();
		( *back )->plane = plane;
		( *back )->planeNum = planeNum;
		( *back )->flags = flags;
		( *back )->winding = backWinding;
	}

	if( frontWinding && backWinding )
	{
		return PLANESIDE_CROSS;
	}
	else if( frontWinding )
	{
		return PLANESIDE_FRONT;
	}
	else
	{
		return PLANESIDE_BACK;
	}
}


//===============================================================
//
//	idBrushBSPNode
//
//===============================================================

/*
============
idBrushBSPNode::idBrushBSPNode
============
*/
idBrushBSPNode::idBrushBSPNode()
{
	brushList.Clear();
	contents = 0;
	flags = 0;
	volume = NULL;
	portals = NULL;
	children[0] = children[1] = NULL;
	areaNum = 0;
	occupied = 0;
}

/*
============
idBrushBSPNode::~idBrushBSPNode
============
*/
idBrushBSPNode::~idBrushBSPNode()
{
	idBrushBSPPortal* p;

	// delete brushes
	brushList.Free();

	// delete volume brush
	if( volume )
	{
		delete volume;
	}

	// delete portals
	for( p = portals; p; p = portals )
	{
		p->RemoveFromNode( this );
		if( !p->nodes[0] && !p->nodes[1] )
		{
			delete p;
		}
	}
}

/*
============
idBrushBSPNode::SetContentsFromBrushes
============
*/
void idBrushBSPNode::SetContentsFromBrushes()
{
	idBrush* brush;

	contents = 0;
	for( brush = brushList.Head(); brush; brush = brush->Next() )
	{
		contents |= brush->GetContents();
	}
}

/*
============
idBrushBSPNode::GetPortalBounds
============
*/
idBounds idBrushBSPNode::GetPortalBounds()
{
	int s, i;
	idBrushBSPPortal* p;
	idBounds bounds;

	bounds.Clear();
	for( p = portals; p; p = p->next[s] )
	{
		s = ( p->nodes[1] == this );

		for( i = 0; i < p->winding->GetNumPoints(); i++ )
		{
			bounds.AddPoint( ( *p->winding )[i].ToVec3() );
		}
	}
	return bounds;
}

/*
============
idBrushBSPNode::TestLeafNode
============
*/
bool idBrushBSPNode::TestLeafNode()
{
	int s, n;
	float d;
	idBrushBSPPortal* p;
	idVec3 center;
	idPlane plane;

	n = 0;
	center = vec3_origin;
	for( p = portals; p; p = p->next[s] )
	{
		s = ( p->nodes[1] == this );
		center += p->winding->GetCenter();
		n++;
	}

	center /= n;

	for( p = portals; p; p = p->next[s] )
	{
		s = ( p->nodes[1] == this );
		if( s )
		{
			plane = -p->GetPlane();
		}
		else
		{
			plane = p->GetPlane();
		}
		d = plane.Distance( center );
		if( d < 0.0f )
		{
			return false;
		}
	}
	return true;
}

/*
============
idBrushBSPNode::Split
============
*/
bool idBrushBSPNode::Split( const idPlane& splitPlane, int splitPlaneNum )
{
	int s, i;
	idWinding* mid;
	idBrushBSPPortal* p, *midPortal, *newPortals[2];
	idBrushBSPNode* newNodes[2];

	mid = new idWinding( splitPlane.Normal(), splitPlane.Dist() );

	for( p = portals; p && mid; p = p->next[s] )
	{
		s = ( p->nodes[1] == this );
		if( s )
		{
			mid = mid->Clip( -p->plane, 0.1f, false );
		}
		else
		{
			mid = mid->Clip( p->plane, 0.1f, false );
		}
	}

	if( !mid )
	{
		return false;
	}

	// allocate two new nodes
	for( i = 0; i < 2; i++ )
	{
		newNodes[i] = new idBrushBSPNode();
		newNodes[i]->flags = flags;
		newNodes[i]->contents = contents;
		newNodes[i]->parent = this;
	}

	// split all portals of the node
	for( p = portals; p; p = portals )
	{
		s = ( p->nodes[1] == this );
		p->Split( splitPlane, &newPortals[0], &newPortals[1] );
		for( i = 0; i < 2; i++ )
		{
			if( newPortals[i] )
			{
				if( s )
				{
					newPortals[i]->AddToNodes( p->nodes[0], newNodes[i] );
				}
				else
				{
					newPortals[i]->AddToNodes( newNodes[i], p->nodes[1] );
				}
			}
		}
		p->RemoveFromNode( p->nodes[0] );
		p->RemoveFromNode( p->nodes[1] );
		delete p;
	}

	// add seperating portal
	midPortal = new idBrushBSPPortal();
	midPortal->plane = splitPlane;
	midPortal->planeNum = splitPlaneNum;
	midPortal->winding = mid;
	midPortal->AddToNodes( newNodes[0], newNodes[1] );

	// set new child nodes
	children[0] = newNodes[0];
	children[1] = newNodes[1];
	plane = splitPlane;

	return true;
}

/*
============
idBrushBSPNode::PlaneSide
============
*/
int idBrushBSPNode::PlaneSide( const idPlane& plane, float epsilon ) const
{
	int s, side;
	idBrushBSPPortal* p;
	bool front, back;

	front = back = false;
	for( p = portals; p; p = p->next[s] )
	{
		s = ( p->nodes[1] == this );

		side = p->winding->PlaneSide( plane, epsilon );
		if( side == SIDE_CROSS || side == SIDE_ON )
		{
			return side;
		}
		if( side == SIDE_FRONT )
		{
			if( back )
			{
				return SIDE_CROSS;
			}
			front = true;
		}
		if( side == SIDE_BACK )
		{
			if( front )
			{
				return SIDE_CROSS;
			}
			back = true;
		}
	}

	if( front )
	{
		return SIDE_FRONT;
	}
	return SIDE_BACK;
}

/*
============
idBrushBSPNode::RemoveFlagFlood
============
*/
void idBrushBSPNode::RemoveFlagFlood( int flag )
{
	int s;
	idBrushBSPPortal* p;

	RemoveFlag( flag );

	for( p = GetPortals(); p; p = p->Next( s ) )
	{
		s = ( p->GetNode( 1 ) == this );

		if( !( p->GetNode( !s )->GetFlags() & flag ) )
		{
			continue;
		}

		p->GetNode( !s )->RemoveFlagFlood( flag );
	}
}

/*
============
idBrushBSPNode::RemoveFlagRecurse
============
*/
void idBrushBSPNode::RemoveFlagRecurse( int flag )
{
	RemoveFlag( flag );
	if( children[0] )
	{
		children[0]->RemoveFlagRecurse( flag );
	}
	if( children[1] )
	{
		children[1]->RemoveFlagRecurse( flag );
	}
}

/*
============
idBrushBSPNode::RemoveFlagRecurseFlood
============
*/
void idBrushBSPNode::RemoveFlagRecurseFlood( int flag )
{
	RemoveFlag( flag );
	if( !children[0] && !children[1] )
	{
		RemoveFlagFlood( flag );
	}
	else
	{
		if( children[0] )
		{
			children[0]->RemoveFlagRecurseFlood( flag );
		}
		if( children[1] )
		{
			children[1]->RemoveFlagRecurseFlood( flag );
		}
	}
}


//===============================================================
//
//	idBrushBSP
//
//===============================================================

/*
============
idBrushBSP::idBrushBSP
============
*/
idBrushBSP::idBrushBSP()
{
	root = outside = NULL;
	numSplits = numPrunedSplits = 0;
	brushMapContents = 0;
	brushMap = NULL;
}

/*
============
idBrushBSP::~idBrushBSP
============
*/
idBrushBSP::~idBrushBSP()
{

	RemoveMultipleLeafNodeReferences_r( root );
	Free_r( root );

	if( outside )
	{
		delete outside;
	}
}

/*
============
idBrushBSP::RemoveMultipleLeafNodeReferences_r
============
*/
void idBrushBSP::RemoveMultipleLeafNodeReferences_r( idBrushBSPNode* node )
{
	if( !node )
	{
		return;
	}

	if( node->children[0] )
	{
		if( node->children[0]->parent != node )
		{
			node->children[0] = NULL;
		}
		else
		{
			RemoveMultipleLeafNodeReferences_r( node->children[0] );
		}
	}
	if( node->children[1] )
	{
		if( node->children[1]->parent != node )
		{
			node->children[1] = NULL;
		}
		else
		{
			RemoveMultipleLeafNodeReferences_r( node->children[1] );
		}
	}
}

/*
============
idBrushBSP::Free_r
============
*/
void idBrushBSP::Free_r( idBrushBSPNode* node )
{
	if( !node )
	{
		return;
	}

	Free_r( node->children[0] );
	Free_r( node->children[1] );

	delete node;
}

/*
============
idBrushBSP::IsValidSplitter
============
*/
ID_INLINE bool idBrushBSP::IsValidSplitter( const idBrushSide* side )
{
	return !( side->GetFlags() & ( SFL_SPLIT | SFL_USED_SPLITTER ) );
}

/*
============
idBrushBSP::BrushSplitterStats
============
*/
typedef struct splitterStats_s
{
	int numFront;			// number of brushes at the front of the splitter
	int numBack;			// number of brushes at the back of the splitter
	int numSplits;			// number of brush sides split by the splitter
	int numFacing;			// number of brushes facing this splitter
	int epsilonBrushes;		// number of tiny brushes this splitter would create
} splitterStats_t;

int idBrushBSP::BrushSplitterStats( const idBrush* brush, int planeNum, const idPlaneSet& planeList, bool* testedPlanes, struct splitterStats_s& stats )
{
	int i, j, num, s, lastNumSplits;
	const idPlane* plane;
	const idWinding* w;
	float d, d_front, d_back, brush_front, brush_back;

	plane = &planeList[planeNum];

	// get the plane side for the brush bounds
	s = brush->GetBounds().PlaneSide( *plane, SPLITTER_EPSILON );
	if( s == PLANESIDE_FRONT )
	{
		stats.numFront++;
		return BRUSH_PLANESIDE_FRONT;
	}
	if( s == PLANESIDE_BACK )
	{
		stats.numBack++;
		return BRUSH_PLANESIDE_BACK;
	}

	// if the brush actually uses the planenum, we can tell the side for sure
	for( i = 0; i < brush->GetNumSides(); i++ )
	{
		num = brush->GetSide( i )->GetPlaneNum();

		if( !( ( num ^ planeNum ) >> 1 ) )
		{
			if( num == planeNum )
			{
				stats.numBack++;
				stats.numFacing++;
				return ( BRUSH_PLANESIDE_BACK | BRUSH_PLANESIDE_FACING );
			}
			if( num == ( planeNum ^ 1 ) )
			{
				stats.numFront++;
				stats.numFacing++;
				return ( BRUSH_PLANESIDE_FRONT | BRUSH_PLANESIDE_FACING );
			}
		}
	}

	lastNumSplits = stats.numSplits;
	brush_front = brush_back = 0.0f;
	for( i = 0; i < brush->GetNumSides(); i++ )
	{

		if( !IsValidSplitter( brush->GetSide( i ) ) )
		{
			continue;
		}

		j = brush->GetSide( i )->GetPlaneNum();
		if( testedPlanes[j] || testedPlanes[j ^ 1] )
		{
			continue;
		}

		w = brush->GetSide( i )->GetWinding();
		if( !w )
		{
			continue;
		}
		d_front = d_back = 0.0f;
		for( j = 0; j < w->GetNumPoints(); j++ )
		{
			d = plane->Distance( ( *w )[j].ToVec3() );
			if( d > d_front )
			{
				d_front = d;
			}
			else if( d < d_back )
			{
				d_back = d;
			}
		}
		if( d_front > SPLITTER_EPSILON && d_back < -SPLITTER_EPSILON )
		{
			stats.numSplits++;
		}
		if( d_front > brush_front )
		{
			brush_front = d_front;
		}
		else if( d_back < brush_back )
		{
			brush_back = d_back;
		}
	}

	// if brush sides are split and the brush only pokes one unit through the plane
	if( stats.numSplits > lastNumSplits && ( brush_front < 1.0f || brush_back > -1.0f ) )
	{
		stats.epsilonBrushes++;
	}

	return BRUSH_PLANESIDE_BOTH;
}

/*
============
idBrushBSP::FindSplitter
============
*/
int idBrushBSP::FindSplitter( idBrushBSPNode* node, const idPlaneSet& planeList, bool* testedPlanes, struct splitterStats_s& bestStats )
{
	int i, planeNum, bestSplitter, value, bestValue, f, numBrushSides;
	idBrush* brush, *b;
	splitterStats_t stats;

	memset( testedPlanes, 0, planeList.Num() * sizeof( bool ) );

	bestSplitter = -1;
	bestValue = -99999999;
	for( brush = node->brushList.Head(); brush; brush = brush->Next() )
	{

		if( brush->GetFlags() & BFL_NO_VALID_SPLITTERS )
		{
			continue;
		}

		for( i = 0; i < brush->GetNumSides(); i++ )
		{

			if( !IsValidSplitter( brush->GetSide( i ) ) )
			{
				continue;
			}

			planeNum = brush->GetSide( i )->GetPlaneNum();

			if( testedPlanes[planeNum] || testedPlanes[planeNum ^ 1] )
			{
				continue;
			}

			testedPlanes[planeNum] = testedPlanes[planeNum ^ 1] = true;

			if( node->volume->Split( planeList[planeNum], planeNum, NULL, NULL ) != PLANESIDE_CROSS )
			{
				continue;
			}

			memset( &stats, 0, sizeof( stats ) );

			f = 15 + 5 * ( brush->GetSide( i )->GetPlane().Type() < PLANETYPE_TRUEAXIAL );
			numBrushSides = node->brushList.NumSides();

			for( b = node->brushList.Head(); b; b = b->Next() )
			{

				// if the brush has no valid splitters left
				if( b->GetFlags() & BFL_NO_VALID_SPLITTERS )
				{
					b->SetPlaneSide( BRUSH_PLANESIDE_BOTH );
				}
				else
				{
					b->SetPlaneSide( BrushSplitterStats( b, planeNum, planeList, testedPlanes, stats ) );
				}

				numBrushSides -= b->GetNumSides();
				// best value we can get using this plane as a splitter
				value = f * ( stats.numFacing + numBrushSides ) - 10 * stats.numSplits - stats.epsilonBrushes * 1000;
				// if the best value for this plane can't get any better than the best value we have
				if( value < bestValue )
				{
					break;
				}
			}

			if( b )
			{
				continue;
			}

			value = f * stats.numFacing - 10 * stats.numSplits - abs( stats.numFront - stats.numBack ) - stats.epsilonBrushes * 1000;

			if( value > bestValue )
			{
				bestValue = value;
				bestSplitter = planeNum;
				bestStats = stats;

				for( b = node->brushList.Head(); b; b = b->Next() )
				{
					b->SavePlaneSide();
				}
			}
		}
	}

	return bestSplitter;
}

/*
============
idBrushBSP::SetSplitterUsed
============
*/
void idBrushBSP::SetSplitterUsed( idBrushBSPNode* node, int planeNum )
{
	int i, numValidBrushSplitters;
	idBrush* brush;

	for( brush = node->brushList.Head(); brush; brush = brush->Next() )
	{
		if( !( brush->GetSavedPlaneSide() & BRUSH_PLANESIDE_FACING ) )
		{
			continue;
		}
		numValidBrushSplitters = 0;
		for( i = 0; i < brush->GetNumSides(); i++ )
		{

			if( !( ( brush->GetSide( i )->GetPlaneNum() ^ planeNum ) >> 1 ) )
			{
				brush->GetSide( i )->SetFlag( SFL_USED_SPLITTER );
			}
			else if( IsValidSplitter( brush->GetSide( i ) ) )
			{
				numValidBrushSplitters++;
			}
		}
		if( numValidBrushSplitters == 0 )
		{
			brush->SetFlag( BFL_NO_VALID_SPLITTERS );
		}
	}
}

/*
============
idBrushBSP::BuildBrushBSP_r
============
*/
idBrushBSPNode* idBrushBSP::BuildBrushBSP_r( idBrushBSPNode* node, const idPlaneSet& planeList, bool* testedPlanes, int skipContents )
{
	int planeNum;
	splitterStats_t bestStats;

	planeNum = FindSplitter( node, planeList, testedPlanes, bestStats );

	// if no split plane found this is a leaf node
	if( planeNum == -1 )
	{

		node->SetContentsFromBrushes();

		if( brushMap && ( node->contents & brushMapContents ) )
		{
			brushMap->WriteBrush( node->volume );
		}

		// free node memory
		node->brushList.Free();
		delete node->volume;
		node->volume = NULL;

		node->children[0] = node->children[1] = NULL;
		return node;
	}

	numSplits++;
	numGridCellSplits++;

	// mark all brush sides on the split plane as used
	SetSplitterUsed( node, planeNum );

	// set node split plane
	node->plane = planeList[planeNum];

	// allocate children
	node->children[0] = new idBrushBSPNode();
	node->children[1] = new idBrushBSPNode();

	// split node volume and brush list for children
	node->volume->Split( node->plane, -1, &node->children[0]->volume, &node->children[1]->volume );
	node->brushList.Split( node->plane, -1, node->children[0]->brushList, node->children[1]->brushList, true );
	node->children[0]->parent = node->children[1]->parent = node;

	// free node memory
	node->brushList.Free();
	delete node->volume;
	node->volume = NULL;

	// process children
	node->children[0] = BuildBrushBSP_r( node->children[0], planeList, testedPlanes, skipContents );
	node->children[1] = BuildBrushBSP_r( node->children[1], planeList, testedPlanes, skipContents );

	// if both children contain the skip contents
	if( node->children[0]->contents & node->children[1]->contents & skipContents )
	{
		node->contents = node->children[0]->contents | node->children[1]->contents;
		delete node->children[0];
		delete node->children[1];
		node->children[0] = node->children[1] = NULL;
		numSplits--;
		numGridCellSplits--;
	}

	return node;
}

/*
============
idBrushBSP::ProcessGridCell
============
*/
idBrushBSPNode* idBrushBSP::ProcessGridCell( idBrushBSPNode* node, int skipContents )
{
	idPlaneSet planeList;
	bool* testedPlanes;

#ifdef OUPUT_BSP_STATS_PER_GRID_CELL
	common->Printf( "[Grid Cell %d]\n", ++numGridCells );
	common->Printf( "%6d brushes\n", node->brushList.Num() );
#endif

	numGridCellSplits = 0;

	// chop away all brush overlap
	node->brushList.Chop( BrushChopAllowed );

	// merge brushes if possible
	//node->brushList.Merge( BrushMergeAllowed );

	// create a list with planes for this grid cell
	node->brushList.CreatePlaneList( planeList );

#ifdef OUPUT_BSP_STATS_PER_GRID_CELL
	common->Printf( "[Grid Cell BSP]\n" );
#endif

	testedPlanes = new bool[planeList.Num()];

	BuildBrushBSP_r( node, planeList, testedPlanes, skipContents );

	delete testedPlanes;

#ifdef OUPUT_BSP_STATS_PER_GRID_CELL
	common->Printf( "\r%6d splits\n", numGridCellSplits );
#endif

	return node;
}

/*
============
idBrushBSP::BuildGrid_r
============
*/
void idBrushBSP::BuildGrid_r( idList<idBrushBSPNode*>& gridCells, idBrushBSPNode* node )
{
	int axis;
	float dist;
	idBounds bounds;
	idVec3 normal, halfSize;

	if( !node->brushList.Num() )
	{
		delete node->volume;
		node->volume = NULL;
		node->children[0] = node->children[1] = NULL;
		return;
	}

	bounds = node->volume->GetBounds();
	halfSize = ( bounds[1] - bounds[0] ) * 0.5f;
	for( axis = 0; axis < 3; axis++ )
	{
		if( halfSize[axis] > BSP_GRID_SIZE )
		{
			dist = BSP_GRID_SIZE * ( floor( ( bounds[0][axis] + halfSize[axis] ) / BSP_GRID_SIZE ) + 1 );
		}
		else
		{
			dist = BSP_GRID_SIZE * ( floor( bounds[0][axis] / BSP_GRID_SIZE ) + 1 );
		}
		if( dist > bounds[0][axis] + 1.0f && dist < bounds[1][axis] - 1.0f )
		{
			break;
		}
	}
	if( axis >= 3 )
	{
		gridCells.Append( node );
		return;
	}

	numSplits++;

	normal = vec3_origin;
	normal[axis] = 1.0f;
	node->plane.SetNormal( normal );
	node->plane.SetDist( ( int ) dist );

	// allocate children
	node->children[0] = new idBrushBSPNode();
	node->children[1] = new idBrushBSPNode();

	// split volume and brush list for children
	node->volume->Split( node->plane, -1, &node->children[0]->volume, &node->children[1]->volume );
	node->brushList.Split( node->plane, -1, node->children[0]->brushList, node->children[1]->brushList );
	node->children[0]->brushList.SetFlagOnFacingBrushSides( node->plane, SFL_USED_SPLITTER );
	node->children[1]->brushList.SetFlagOnFacingBrushSides( node->plane, SFL_USED_SPLITTER );
	node->children[0]->parent = node->children[1]->parent = node;

	// free node memory
	node->brushList.Free();
	delete node->volume;
	node->volume = NULL;

	// process children
	BuildGrid_r( gridCells, node->children[0] );
	BuildGrid_r( gridCells, node->children[1] );
}

/*
============
idBrushBSP::Build
============
*/
void idBrushBSP::Build( idBrushList brushList, int skipContents,
						bool ( *ChopAllowed )( idBrush* b1, idBrush* b2 ),
						bool ( *MergeAllowed )( idBrush* b1, idBrush* b2 ) )
{

	int i;
	idList<idBrushBSPNode*> gridCells;

	common->Printf( "[Brush BSP]\n" );
	common->Printf( "%6d brushes\n", brushList.Num() );

	BrushChopAllowed = ChopAllowed;
	BrushMergeAllowed = MergeAllowed;

	numGridCells = 0;
	treeBounds = brushList.GetBounds();
	root = new idBrushBSPNode();
	root->brushList = brushList;
	root->volume = new idBrush();
	root->volume->FromBounds( treeBounds );
	root->parent = NULL;

	BuildGrid_r( gridCells, root );

	common->Printf( "\r%6d grid cells\n", gridCells.Num() );

#ifdef OUPUT_BSP_STATS_PER_GRID_CELL
	for( i = 0; i < gridCells.Num(); i++ )
	{
		ProcessGridCell( gridCells[i], skipContents );
	}
#else
	common->Printf( "\r%6d %%", 0 );
	for( i = 0; i < gridCells.Num(); i++ )
	{
		DisplayRealTimeString( "\r%6d", i * 100 / gridCells.Num() );
		ProcessGridCell( gridCells[i], skipContents );
	}
	common->Printf( "\r%6d %%\n", 100 );
#endif

	common->Printf( "\r%6d splits\n", numSplits );

	if( brushMap )
	{
		delete brushMap;
	}
}

/*
============
idBrushBSP::WriteBrushMap
============
*/
void idBrushBSP::WriteBrushMap( const idStr& fileName, const idStr& ext, int contents )
{
	brushMap = new idBrushMap( fileName, ext );
	brushMapContents = contents;
}

/*
============
idBrushBSP::PruneTree_r
============
*/
void idBrushBSP::PruneTree_r( idBrushBSPNode* node, int contents )
{
	int i, s;
	idBrushBSPNode* nodes[2];
	idBrushBSPPortal* p, *nextp;

	if( !node->children[0] || !node->children[1] )
	{
		return;
	}

	PruneTree_r( node->children[0], contents );
	PruneTree_r( node->children[1], contents );

	if( ( node->children[0]->contents & node->children[1]->contents & contents ) )
	{

		node->contents = node->children[0]->contents | node->children[1]->contents;
		// move all child portals to parent
		for( i = 0; i < 2; i++ )
		{
			for( p = node->children[i]->portals; p; p = nextp )
			{
				s = ( p->nodes[1] == node->children[i] );
				nextp = p->next[s];
				nodes[s] = node;
				nodes[!s] = p->nodes[!s];
				p->RemoveFromNode( p->nodes[0] );
				p->RemoveFromNode( p->nodes[1] );
				if( nodes[!s] == node->children[!i] )
				{
					delete p;	// portal seperates both children
				}
				else
				{
					p->AddToNodes( nodes[0], nodes[1] );
				}
			}
		}

		delete node->children[0];
		delete node->children[1];
		node->children[0] = NULL;
		node->children[1] = NULL;

		numPrunedSplits++;
	}
}

/*
============
idBrushBSP::PruneTree
============
*/
void idBrushBSP::PruneTree( int contents )
{
	numPrunedSplits = 0;
	common->Printf( "[Prune BSP]\n" );
	PruneTree_r( root, contents );
	common->Printf( "%6d splits pruned\n", numPrunedSplits );
}

/*
============
idBrushBSP::BaseWindingForNode
============
*/
#define	BASE_WINDING_EPSILON		0.001f

idWinding* idBrushBSP::BaseWindingForNode( idBrushBSPNode* node )
{
	idWinding* w;
	idBrushBSPNode* n;

	w = new idWinding( node->plane.Normal(), node->plane.Dist() );

	// clip by all the parents
	for( n = node->parent; n && w; n = n->parent )
	{

		if( n->children[0] == node )
		{
			// take front
			w = w->Clip( n->plane, BASE_WINDING_EPSILON );
		}
		else
		{
			// take back
			w = w->Clip( -n->plane, BASE_WINDING_EPSILON );
		}
		node = n;
	}

	return w;
}

/*
============
idBrushBSP::MakeNodePortal

  create the new portal by taking the full plane winding for the cutting
  plane and clipping it by all of parents of this node
============
*/
void idBrushBSP::MakeNodePortal( idBrushBSPNode* node )
{
	idBrushBSPPortal* newPortal, *p;
	idWinding* w;
	int side = 0;

	w = BaseWindingForNode( node );

	// clip the portal by all the other portals in the node
	for( p = node->portals; p && w; p = p->next[side] )
	{
		if( p->nodes[0] == node )
		{
			side = 0;
			w = w->Clip( p->plane, 0.1f );
		}
		else if( p->nodes[1] == node )
		{
			side = 1;
			w = w->Clip( -p->plane, 0.1f );
		}
		else
		{
			common->Error( "MakeNodePortal: mislinked portal" );
		}
	}

	if( !w )
	{
		return;
	}

	if( w->IsTiny() )
	{
		delete w;
		return;
	}

	newPortal = new idBrushBSPPortal();
	newPortal->plane = node->plane;
	newPortal->winding = w;
	newPortal->AddToNodes( node->children[0], node->children[1] );
}

/*
============
idBrushBSP::SplitNodePortals

  Move or split the portals that bound the node so that the node's children have portals instead of node.
============
*/
#define	SPLIT_WINDING_EPSILON		0.001f

void idBrushBSP::SplitNodePortals( idBrushBSPNode* node )
{
	int side = 0;
	idBrushBSPPortal* p, *nextPortal, *newPortal;
	idBrushBSPNode* f, *b, *otherNode;
	idPlane* plane;
	idWinding* frontWinding, *backWinding;

	plane = &node->plane;
	f = node->children[0];
	b = node->children[1];

	for( p = node->portals; p; p = nextPortal )
	{
		if( p->nodes[0] == node )
		{
			side = 0;
		}
		else if( p->nodes[1] == node )
		{
			side = 1;
		}
		else
		{
			common->Error( "idBrushBSP::SplitNodePortals: mislinked portal" );
		}
		nextPortal = p->next[side];

		otherNode = p->nodes[!side];
		p->RemoveFromNode( p->nodes[0] );
		p->RemoveFromNode( p->nodes[1] );

		// cut the portal into two portals, one on each side of the cut plane
		p->winding->Split( *plane, SPLIT_WINDING_EPSILON, &frontWinding, &backWinding );

		if( frontWinding && frontWinding->IsTiny() )
		{
			delete frontWinding;
			frontWinding = NULL;
			//tinyportals++;
		}

		if( backWinding && backWinding->IsTiny() )
		{
			delete backWinding;
			backWinding = NULL;
			//tinyportals++;
		}

		if( !frontWinding && !backWinding )
		{
			// tiny windings on both sides
			continue;
		}

		if( !frontWinding )
		{
			delete backWinding;
			if( side == 0 )
			{
				p->AddToNodes( b, otherNode );
			}
			else
			{
				p->AddToNodes( otherNode, b );
			}
			continue;
		}
		if( !backWinding )
		{
			delete frontWinding;
			if( side == 0 )
			{
				p->AddToNodes( f, otherNode );
			}
			else
			{
				p->AddToNodes( otherNode, f );
			}
			continue;
		}

		// the winding is split
		newPortal = new idBrushBSPPortal();
		*newPortal = *p;
		newPortal->winding = backWinding;
		delete p->winding;
		p->winding = frontWinding;

		if( side == 0 )
		{
			p->AddToNodes( f, otherNode );
			newPortal->AddToNodes( b, otherNode );
		}
		else
		{
			p->AddToNodes( otherNode, f );
			newPortal->AddToNodes( otherNode, b );
		}
	}

	node->portals = NULL;
}

/*
============
idBrushBSP::MakeTreePortals_r
============
*/
void idBrushBSP::MakeTreePortals_r( idBrushBSPNode* node )
{
	int i;
	idBounds bounds;

	numPortals++;
	DisplayRealTimeString( "\r%6d", numPortals );

	bounds = node->GetPortalBounds();

	if( bounds[0][0] >= bounds[1][0] )
	{
		//common->Warning( "node without volume" );
	}

	for( i = 0; i < 3; i++ )
	{
		if( bounds[0][i] < MIN_WORLD_COORD || bounds[1][i] > MAX_WORLD_COORD )
		{
			common->Warning( "node with unbounded volume" );
			break;
		}
	}

	if( !node->children[0] || !node->children[1] )
	{
		return;
	}

	MakeNodePortal( node );
	SplitNodePortals( node );

	MakeTreePortals_r( node->children[0] );
	MakeTreePortals_r( node->children[1] );
}

/*
============
idBrushBSP::MakeOutsidePortals
============
*/
void idBrushBSP::MakeOutsidePortals()
{
	int i, j, n;
	idBounds bounds;
	idBrushBSPPortal* p, *portals[6];
	idVec3 normal;
	idPlane planes[6];

	// pad with some space so there will never be null volume leaves
	bounds = treeBounds.Expand( 32 );

	for( i = 0; i < 3; i++ )
	{
		if( bounds[0][i] > bounds[1][i] )
		{
			common->Error( "empty BSP tree" );
		}
	}

	outside = new idBrushBSPNode();
	outside->parent = NULL;
	outside->children[0] = outside->children[1] = NULL;
	outside->brushList.Clear();
	outside->portals = NULL;
	outside->contents = 0;

	for( i = 0; i < 3; i++ )
	{
		for( j = 0; j < 2; j++ )
		{

			p = new idBrushBSPPortal();
			normal = vec3_origin;
			normal[i] = j ? -1 : 1;
			p->plane.SetNormal( normal );
			p->plane.SetDist( j ? -bounds[j][i] : bounds[j][i] );
			p->winding = new idWinding( p->plane.Normal(), p->plane.Dist() );
			p->AddToNodes( root, outside );

			n = j * 3 + i;
			portals[n] = p;
		}
	}

	// clip the base windings with all the other planes
	for( i = 0; i < 6; i++ )
	{
		for( j = 0; j < 6; j++ )
		{
			if( j == i )
			{
				continue;
			}
			portals[i]->winding = portals[i]->winding->Clip( portals[j]->plane, ON_EPSILON );
		}
	}
}

/*
============
idBrushBSP::Portalize
============
*/
void idBrushBSP::Portalize()
{
	common->Printf( "[Portalize BSP]\n" );
	common->Printf( "%6d nodes\n", ( numSplits - numPrunedSplits ) * 2 + 1 );
	numPortals = 0;
	MakeOutsidePortals();
	MakeTreePortals_r( root );
	common->Printf( "\r%6d nodes portalized\n", numPortals );
}

/*
=============
LeakFile

Finds the shortest possible chain of portals that
leads from the outside leaf to a specific occupied leaf.
=============
*/
void idBrushBSP::LeakFile( const idStr& fileName )
{
	int count, next, s;
	idVec3 mid;
	idFile* lineFile;
	idBrushBSPNode* node, *nextNode = NULL;
	idBrushBSPPortal* p, *nextPortal = NULL;
	idStr qpath, name;

	if( !outside->occupied )
	{
		return;
	}

	qpath = fileName;
	qpath.SetFileExtension( "lin" );

	common->Printf( "writing %s...\n", qpath.c_str() );

	lineFile = fileSystem->OpenFileWrite( qpath, "fs_devpath" );
	if( !lineFile )
	{
		common->Error( "Couldn't open %s\n", qpath.c_str() );
		return;
	}

	count = 0;
	node = outside;
	while( node->occupied > 1 )
	{

		// find the best portal exit
		next = node->occupied;
		for( p = node->portals; p; p = p->next[!s] )
		{
			s = ( p->nodes[0] == node );
			if( p->nodes[s]->occupied && p->nodes[s]->occupied < next )
			{
				nextPortal = p;
				nextNode = p->nodes[s];
				next = nextNode->occupied;
			}
		}
		node = nextNode;
		mid = nextPortal->winding->GetCenter();
		lineFile->Printf( "%f %f %f\n", mid[0], mid[1], mid[2] );
		count++;
	}

	// add the origin of the entity from which the leak was found
	lineFile->Printf( "%f %f %f\n", leakOrigin[0], leakOrigin[1], leakOrigin[2] );

	fileSystem->CloseFile( lineFile );
}

/*
============
idBrushBSP::FloodThroughPortals_r
============
*/
void idBrushBSP::FloodThroughPortals_r( idBrushBSPNode* node, int contents, int depth )
{
	idBrushBSPPortal* p;
	int s;

	if( node->occupied )
	{
		common->Error( "FloodThroughPortals_r: node already occupied\n" );
	}
	if( !node )
	{
		common->Error( "FloodThroughPortals_r: NULL node\n" );
	}

	node->occupied = depth;

	for( p = node->portals; p; p = p->next[s] )
	{
		s = ( p->nodes[1] == node );

		// if the node at the other side of the portal is removed
		if( !p->nodes[!s] )
		{
			continue;
		}

		// if the node at the other side of the portal is occupied already
		if( p->nodes[!s]->occupied )
		{
			continue;
		}

		// can't flood through the portal if it has the seperating contents at the other side
		if( p->nodes[!s]->contents & contents )
		{
			continue;
		}

		// flood recursively through the current portal
		FloodThroughPortals_r( p->nodes[!s], contents, depth + 1 );
	}
}

/*
============
idBrushBSP::FloodFromOrigin
============
*/
bool idBrushBSP::FloodFromOrigin( const idVec3& origin, int contents )
{
	idBrushBSPNode* node;

	//find the leaf to start in
	node = root;
	while( node->children[0] && node->children[1] )
	{

		if( node->plane.Side( origin ) == PLANESIDE_BACK )
		{
			node = node->children[1];
		}
		else
		{
			node = node->children[0];
		}
	}

	if( !node )
	{
		return false;
	}

	// if inside the inside/outside seperating contents
	if( node->contents & contents )
	{
		return false;
	}

	// if the node is already occupied
	if( node->occupied )
	{
		return false;
	}

	FloodThroughPortals_r( node, contents, 1 );

	return true;
}

/*
============
idBrushBSP::FloodFromEntities

  Marks all nodes that can be reached by entites.
============
*/
bool idBrushBSP::FloodFromEntities( const idMapFile* mapFile, int contents, const idStrList& classNames )
{
	int i, j;
	bool inside;
	idVec3 origin;
	idMapEntity* mapEnt;
	idStr classname;

	inside = false;
	outside->occupied = 0;

	// skip the first entity which is assumed to be the worldspawn
	for( i = 1; i < mapFile->GetNumEntities(); i++ )
	{

		mapEnt = mapFile->GetEntity( i );

		if( !mapEnt->epairs.GetVector( "origin", "", origin ) )
		{
			continue;
		}

		if( !mapEnt->epairs.GetString( "classname", "", classname ) )
		{
			continue;
		}

		for( j = 0; j < classNames.Num(); j++ )
		{
			if( classname.Icmp( classNames[j] ) == 0 )
			{
				break;
			}
		}

		if( j >= classNames.Num() )
		{
			continue;
		}

		origin[2] += 1;

		// nudge around a little
		if( FloodFromOrigin( origin, contents ) )
		{
			inside = true;
		}

		if( outside->occupied )
		{
			leakOrigin = origin;
			break;
		}
	}

	if( !inside )
	{
		common->Warning( "no entities inside" );
	}
	else if( outside->occupied )
	{
		common->Warning( "reached outside from entity %d (%s)", i, classname.c_str() );
	}

	return ( inside && !outside->occupied );
}

/*
============
idBrushBSP::RemoveOutside_r
============
*/
void idBrushBSP::RemoveOutside_r( idBrushBSPNode* node, int contents )
{

	if( !node )
	{
		return;
	}

	if( node->children[0] || node->children[1] )
	{
		RemoveOutside_r( node->children[0], contents );
		RemoveOutside_r( node->children[1], contents );
		return;
	}

	if( !node->occupied )
	{
		if( !( node->contents & contents ) )
		{
			outsideLeafNodes++;
			node->contents |= contents;
		}
		else
		{
			solidLeafNodes++;
		}
	}
	else
	{
		insideLeafNodes++;
	}
}

/*
============
idBrushBSP::RemoveOutside
============
*/
bool idBrushBSP::RemoveOutside( const idMapFile* mapFile, int contents, const idStrList& classNames )
{
	common->Printf( "[Remove Outside]\n" );

	solidLeafNodes = outsideLeafNodes = insideLeafNodes = 0;

	if( !FloodFromEntities( mapFile, contents, classNames ) )
	{
		return false;
	}

	RemoveOutside_r( root, contents );

	common->Printf( "%6d solid leaf nodes\n", solidLeafNodes );
	common->Printf( "%6d outside leaf nodes\n", outsideLeafNodes );
	common->Printf( "%6d inside leaf nodes\n", insideLeafNodes );

	//PruneTree( contents );

	return true;
}

/*
============
idBrushBSP::SetPortalPlanes_r
============
*/
void idBrushBSP::SetPortalPlanes_r( idBrushBSPNode* node, idPlaneSet& planeList )
{
	int s;
	idBrushBSPPortal* p;

	if( !node )
	{
		return;
	}

	for( p = node->portals; p; p = p->next[s] )
	{
		s = ( p->nodes[1] == node );
		if( p->planeNum == -1 )
		{
			p->planeNum = planeList.FindPlane( p->plane, PORTAL_PLANE_NORMAL_EPSILON, PORTAL_PLANE_DIST_EPSILON );
		}
	}
	SetPortalPlanes_r( node->children[0], planeList );
	SetPortalPlanes_r( node->children[1], planeList );
}

/*
============
idBrushBSP::SetPortalPlanes

  give all portals a plane number
============
*/
void idBrushBSP::SetPortalPlanes()
{
	SetPortalPlanes_r( root, portalPlanes );
}

/*
============
idBrushBSP::MergeLeafNodePortals
============
*/
void idBrushBSP::MergeLeafNodePortals( idBrushBSPNode* node, int skipContents )
{
	int s1, s2;
	bool foundPortal;
	idBrushBSPPortal* p1, *p2, *nextp1, *nextp2;
	idWinding* newWinding, *reverse;

	// pass 1: merge all portals that seperate the same leaf nodes
	for( p1 = node->GetPortals(); p1; p1 = nextp1 )
	{
		s1 = ( p1->GetNode( 1 ) == node );
		nextp1 = p1->Next( s1 );

		for( p2 = nextp1; p2; p2 = nextp2 )
		{
			s2 = ( p2->GetNode( 1 ) == node );
			nextp2 = p2->Next( s2 );

			// if both portals seperate the same leaf nodes
			if( p1->nodes[!s1] == p2->nodes[!s2] )
			{

				// add the winding of p2 to the winding of p1
				p1->winding->AddToConvexHull( p2->winding, p1->plane.Normal() );

				// delete p2
				p2->RemoveFromNode( p2->nodes[0] );
				p2->RemoveFromNode( p2->nodes[1] );
				delete p2;

				numMergedPortals++;

				nextp1 = node->GetPortals();
				break;
			}
		}
	}

	// pass 2: merge all portals in the same plane if they all have the skip contents at the other side
	for( p1 = node->GetPortals(); p1; p1 = nextp1 )
	{
		s1 = ( p1->GetNode( 1 ) == node );
		nextp1 = p1->Next( s1 );

		if( !( p1->nodes[!s1]->contents & skipContents ) )
		{
			continue;
		}

		// test if all portals in this plane have the skip contents at the other side
		foundPortal = false;
		for( p2 = node->GetPortals(); p2; p2 = nextp2 )
		{
			s2 = ( p2->GetNode( 1 ) == node );
			nextp2 = p2->Next( s2 );

			if( p2 == p1 || ( p2->planeNum & ~1 ) != ( p1->planeNum & ~1 ) )
			{
				continue;
			}
			foundPortal = true;
			if( !( p2->nodes[!s2]->contents & skipContents ) )
			{
				break;
			}
		}

		// if all portals in this plane have the skip contents at the other side
		if( !p2 && foundPortal )
		{
			for( p2 = node->GetPortals(); p2; p2 = nextp2 )
			{
				s2 = ( p2->GetNode( 1 ) == node );
				nextp2 = p2->Next( s2 );

				if( p2 == p1 || ( p2->planeNum & ~1 ) != ( p1->planeNum & ~1 ) )
				{
					continue;
				}

				// add the winding of p2 to the winding of p1
				p1->winding->AddToConvexHull( p2->winding, p1->plane.Normal() );

				// delete p2
				p2->RemoveFromNode( p2->nodes[0] );
				p2->RemoveFromNode( p2->nodes[1] );
				delete p2;

				numMergedPortals++;
			}
			nextp1 = node->GetPortals();
		}
	}

	// pass 3: try to merge portals in the same plane that have the skip contents at the other side
	for( p1 = node->GetPortals(); p1; p1 = nextp1 )
	{
		s1 = ( p1->GetNode( 1 ) == node );
		nextp1 = p1->Next( s1 );

		if( !( p1->nodes[!s1]->contents & skipContents ) )
		{
			continue;
		}

		for( p2 = nextp1; p2; p2 = nextp2 )
		{
			s2 = ( p2->GetNode( 1 ) == node );
			nextp2 = p2->Next( s2 );

			if( !( p2->nodes[!s2]->contents & skipContents ) )
			{
				continue;
			}

			if( ( p2->planeNum & ~1 ) != ( p1->planeNum & ~1 ) )
			{
				continue;
			}

			// try to merge the two portal windings
			if( p2->planeNum == p1->planeNum )
			{
				newWinding = p1->winding->TryMerge( *p2->winding, p1->plane.Normal() );
			}
			else
			{
				reverse = p2->winding->Reverse();
				newWinding = p1->winding->TryMerge( *reverse, p1->plane.Normal() );
				delete reverse;
			}

			// if successfully merged
			if( newWinding )
			{

				// replace the winding of the first portal
				delete p1->winding;
				p1->winding = newWinding;

				// delete p2
				p2->RemoveFromNode( p2->nodes[0] );
				p2->RemoveFromNode( p2->nodes[1] );
				delete p2;

				numMergedPortals++;

				nextp1 = node->GetPortals();
				break;
			}
		}
	}
}

/*
============
idBrushBSP::MergePortals_r
============
*/
void idBrushBSP::MergePortals_r( idBrushBSPNode* node, int skipContents )
{

	if( !node )
	{
		return;
	}

	if( node->contents & skipContents )
	{
		return;
	}

	if( !node->children[0] && !node->children[1] )
	{
		MergeLeafNodePortals( node, skipContents );
		return;
	}

	MergePortals_r( node->children[0], skipContents );
	MergePortals_r( node->children[1], skipContents );
}

/*
============
idBrushBSP::MergePortals
============
*/
void idBrushBSP::MergePortals( int skipContents )
{
	numMergedPortals = 0;
	common->Printf( "[Merge Portals]\n" );
	SetPortalPlanes();
	MergePortals_r( root, skipContents );
	common->Printf( "%6d portals merged\n", numMergedPortals );
}

/*
============
idBrushBSP::PruneMergedTree_r
============
*/
void idBrushBSP::PruneMergedTree_r( idBrushBSPNode* node )
{
	int i;
	idBrushBSPNode* leafNode;

	if( !node )
	{
		return;
	}

	PruneMergedTree_r( node->children[0] );
	PruneMergedTree_r( node->children[1] );

	for( i = 0; i < 2; i++ )
	{
		if( node->children[i] )
		{
			leafNode = node->children[i]->children[0];
			if( leafNode && leafNode == node->children[i]->children[1] )
			{
				if( leafNode->parent == node->children[i] )
				{
					leafNode->parent = node;
				}
				delete node->children[i];
				node->children[i] = leafNode;
			}
		}
	}
}

/*
============
idBrushBSP::UpdateTreeAfterMerge_r
============
*/
void idBrushBSP::UpdateTreeAfterMerge_r( idBrushBSPNode* node, const idBounds& bounds, idBrushBSPNode* oldNode, idBrushBSPNode* newNode )
{

	if( !node )
	{
		return;
	}

	if( !node->children[0] && !node->children[1] )
	{
		return;
	}

	if( node->children[0] == oldNode )
	{
		node->children[0] = newNode;
	}
	if( node->children[1] == oldNode )
	{
		node->children[1] = newNode;
	}

	switch( bounds.PlaneSide( node->plane, 2.0f ) )
	{
		case PLANESIDE_FRONT:
			UpdateTreeAfterMerge_r( node->children[0], bounds, oldNode, newNode );
			break;
		case PLANESIDE_BACK:
			UpdateTreeAfterMerge_r( node->children[1], bounds, oldNode, newNode );
			break;
		default:
			UpdateTreeAfterMerge_r( node->children[0], bounds, oldNode, newNode );
			UpdateTreeAfterMerge_r( node->children[1], bounds, oldNode, newNode );
			break;
	}
}

/*
============
idBrushBSP::TryMergeLeafNodes

  NOTE: multiple brances of the BSP tree might point to the same leaf node after merging
============
*/
bool idBrushBSP::TryMergeLeafNodes( idBrushBSPPortal* portal, int side )
{
	int i, j, k, s1, s2, s;
	idBrushBSPNode* nodes[2], *node1, *node2;
	idBrushBSPPortal* p1, *p2, *p, *nextp;
	idPlane plane;
	idWinding* w;
	idBounds bounds, b;

	nodes[0] = node1 = portal->nodes[side];
	nodes[1] = node2 = portal->nodes[!side];

	// check if the merged node would still be convex
	for( i = 0; i < 2; i++ )
	{

		j = !i;

		for( p1 = nodes[i]->portals; p1; p1 = p1->next[s1] )
		{
			s1 = ( p1->nodes[1] == nodes[i] );

			if( p1->nodes[!s1] == nodes[j] )
			{
				continue;
			}

			if( s1 )
			{
				plane = -p1->plane;
			}
			else
			{
				plane = p1->plane;
			}

			// all the non seperating portals of the other node should be at the front or on the plane
			for( p2 = nodes[j]->portals; p2; p2 = p2->next[s2] )
			{
				s2 = ( p2->nodes[1] == nodes[j] );

				if( p2->nodes[!s2] == nodes[i] )
				{
					continue;
				}

				w = p2->winding;
				for( k = 0; k < w->GetNumPoints(); k++ )
				{
					if( plane.Distance( ( *w )[k].ToVec3() ) < -0.1f )
					{
						return false;
					}
				}
			}
		}
	}

	// remove all portals that seperate the two nodes
	for( p = node1->portals; p; p = nextp )
	{
		s = ( p->nodes[1] == node1 );
		nextp = p->next[s];

		if( p->nodes[!s] == node2 )
		{
			p->RemoveFromNode( p->nodes[0] );
			p->RemoveFromNode( p->nodes[1] );
			delete p;
		}
	}

	// move all portals of node2 to node1
	for( p = node2->portals; p; p = node2->portals )
	{
		s = ( p->nodes[1] == node2 );

		nodes[s] = node1;
		nodes[!s] = p->nodes[!s];
		p->RemoveFromNode( p->nodes[0] );
		p->RemoveFromNode( p->nodes[1] );
		p->AddToNodes( nodes[0], nodes[1] );
	}

	// get bounds for the new node
	bounds.Clear();
	for( p = node1->portals; p; p = p->next[s] )
	{
		s = ( p->nodes[1] == node1 );
		p->GetWinding()->GetBounds( b );
		bounds += b;
	}

	// replace every reference to node2 by a reference to node1
	UpdateTreeAfterMerge_r( root, bounds, node2, node1 );

	if (node2->GetFlags() & NODE_DONE)
		delete node2;

	return true;
}

/*
============
idBrushBSP::MeltFloor_r

  flood through portals touching the bounds to find all vertices that might be inside the bounds
============
*/
void idBrushBSP::MeltFlood_r( idBrushBSPNode* node, int skipContents, idBounds& bounds, idVectorSet<idVec3, 3>& vertexList )
{
	int s1, i;
	idBrushBSPPortal* p1;
	idBounds b;
	const idWinding* w;

	node->SetFlag( NODE_VISITED );

	for( p1 = node->GetPortals(); p1; p1 = p1->Next( s1 ) )
	{
		s1 = ( p1->GetNode( 1 ) == node );

		if( p1->GetNode( !s1 )->GetFlags() & NODE_VISITED )
		{
			continue;
		}

		w = p1->GetWinding();

		for( i = 0; i < w->GetNumPoints(); i++ )
		{
			if( bounds.ContainsPoint( ( *w )[i].ToVec3() ) )
			{
				vertexList.FindVector( ( *w )[i].ToVec3(), VERTEX_MELT_EPSILON );
			}
		}
	}

	for( p1 = node->GetPortals(); p1; p1 = p1->Next( s1 ) )
	{
		s1 = ( p1->GetNode( 1 ) == node );

		if( p1->GetNode( !s1 )->GetFlags() & NODE_VISITED )
		{
			continue;
		}

		if( p1->GetNode( !s1 )->GetContents() & skipContents )
		{
			continue;
		}

		w = p1->GetWinding();
		w->GetBounds( b );

		if( !bounds.IntersectsBounds( b ) )
		{
			continue;
		}

		MeltFlood_r( p1->GetNode( !s1 ), skipContents, bounds, vertexList );
	}
}

/*
============
idBrushBSP::MeltLeafNodePortals
============
*/
void idBrushBSP::MeltLeafNodePortals( idBrushBSPNode* node, int skipContents, idVectorSet<idVec3, 3>& vertexList )
{
	int s1, i;
	idBrushBSPPortal* p1;
	idBounds bounds;

	if( node->GetFlags() & NODE_DONE )
	{
		return;
	}

	node->SetFlag( NODE_DONE );

	// melt things together
	for( p1 = node->GetPortals(); p1; p1 = p1->Next( s1 ) )
	{
		s1 = ( p1->GetNode( 1 ) == node );

		if( p1->GetNode( !s1 )->GetFlags() & NODE_DONE )
		{
			continue;
		}

		p1->winding->GetBounds( bounds );
		bounds.ExpandSelf( 2 * VERTEX_MELT_HASH_SIZE * VERTEX_MELT_EPSILON );
		vertexList.Init( bounds[0], bounds[1], VERTEX_MELT_HASH_SIZE, 128 );

		// get all vertices to be considered
		MeltFlood_r( node, skipContents, bounds, vertexList );
		node->RemoveFlagFlood( NODE_VISITED );

		for( i = 0; i < vertexList.Num(); i++ )
		{
			if( p1->winding->InsertPointIfOnEdge( vertexList[i], p1->plane, 0.1f ) )
			{
				numInsertedPoints++;
			}
		}
	}
	DisplayRealTimeString( "\r%6d", numInsertedPoints );
}

/*
============
idBrushBSP::MeltPortals_r
============
*/
void idBrushBSP::MeltPortals_r( idBrushBSPNode* node, int skipContents, idVectorSet<idVec3, 3>& vertexList )
{
	if( !node )
	{
		return;
	}

	if( node->contents & skipContents )
	{
		return;
	}

	if( !node->children[0] && !node->children[1] )
	{
		MeltLeafNodePortals( node, skipContents, vertexList );
		return;
	}

	MeltPortals_r( node->children[0], skipContents, vertexList );
	MeltPortals_r( node->children[1], skipContents, vertexList );
}

/*
============
idBrushBSP::RemoveLeafNodeColinearPoints
============
*/
void idBrushBSP::RemoveLeafNodeColinearPoints( idBrushBSPNode* node )
{
	int s1;
	idBrushBSPPortal* p1;

	// remove colinear points
	for( p1 = node->GetPortals(); p1; p1 = p1->Next( s1 ) )
	{
		s1 = ( p1->GetNode( 1 ) == node );
		p1->winding->RemoveColinearPoints( p1->plane.Normal(), 0.1f );
	}
}

/*
============
idBrushBSP::RemoveColinearPoints_r
============
*/
void idBrushBSP::RemoveColinearPoints_r( idBrushBSPNode* node, int skipContents )
{
	if( !node )
	{
		return;
	}

	if( node->contents & skipContents )
	{
		return;
	}

	if( !node->children[0] && !node->children[1] )
	{
		RemoveLeafNodeColinearPoints( node );
		return;
	}

	RemoveColinearPoints_r( node->children[0], skipContents );
	RemoveColinearPoints_r( node->children[1], skipContents );
}

/*
============
idBrushBSP::MeltPortals
============
*/
void idBrushBSP::MeltPortals( int skipContents )
{
	idVectorSet<idVec3, 3> vertexList;

	numInsertedPoints = 0;
	common->Printf( "[Melt Portals]\n" );
	RemoveColinearPoints_r( root, skipContents );
	MeltPortals_r( root, skipContents, vertexList );
	root->RemoveFlagRecurse( NODE_DONE );
	common->Printf( "\r%6d points inserted\n", numInsertedPoints );
}
