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

/*
============
idAASBuild::AllGapsLeadToOtherNode
============
*/
bool idAASBuild::AllGapsLeadToOtherNode( idBrushBSPNode* nodeWithGaps, idBrushBSPNode* otherNode )
{
	int s;
	idBrushBSPPortal* p;

	for( p = nodeWithGaps->GetPortals(); p; p = p->Next( s ) )
	{
		s = ( p->GetNode( 1 ) == nodeWithGaps );

		if( !PortalIsGap( p, s ) )
		{
			continue;
		}

		if( p->GetNode( !s ) != otherNode )
		{
			return false;
		}
	}
	return true;
}

/*
============
idAASBuild::MergeWithAdjacentLeafNodes
============
*/
bool idAASBuild::MergeWithAdjacentLeafNodes( idBrushBSP& bsp, idBrushBSPNode* node )
{
	int s, numMerges = 0, otherNodeFlags;
	idBrushBSPPortal* p;

	do
	{
		for( p = node->GetPortals(); p; p = p->Next( s ) )
		{
			s = ( p->GetNode( 1 ) == node );

			// both leaf nodes must have the same contents
			if( node->GetContents() != p->GetNode( !s )->GetContents() )
			{
				continue;
			}

			// cannot merge leaf nodes if one is near a ledge and the other is not
			if( ( node->GetFlags() & AREA_LEDGE ) != ( p->GetNode( !s )->GetFlags() & AREA_LEDGE ) )
			{
				continue;
			}

			// cannot merge leaf nodes if one has a floor portal and the other a gap portal
			if( node->GetFlags() & AREA_FLOOR )
			{
				if( p->GetNode( !s )->GetFlags() & AREA_GAP )
				{
					if( !AllGapsLeadToOtherNode( p->GetNode( !s ), node ) )
					{
						continue;
					}
				}
			}
			else if( node->GetFlags() & AREA_GAP )
			{
				if( p->GetNode( !s )->GetFlags() & AREA_FLOOR )
				{
					if( !AllGapsLeadToOtherNode( node, p->GetNode( !s ) ) )
					{
						continue;
					}
				}
			}

			otherNodeFlags = p->GetNode( !s )->GetFlags();

			// try to merge the leaf nodes
			if( bsp.TryMergeLeafNodes( p, s ) )
			{
				node->SetFlag( otherNodeFlags );
				if( node->GetFlags() & AREA_FLOOR )
				{
					node->RemoveFlag( AREA_GAP );
				}
				numMerges++;
				DisplayRealTimeString( "\r%6d", ++numMergedLeafNodes );
				break;
			}
		}
	}
	while( p );

	if( numMerges )
	{
		return true;
	}
	return false;
}

/*
============
idAASBuild::MergeLeafNodes_r
============
*/
void idAASBuild::MergeLeafNodes_r( idBrushBSP& bsp, idBrushBSPNode* node )
{

	if( !node )
	{
		return;
	}

	if( node->GetContents() & AREACONTENTS_SOLID )
	{
		return;
	}

	if( node->GetFlags() & NODE_DONE )
	{
		return;
	}

	if( !node->GetChild( 0 ) && !node->GetChild( 1 ) )
	{
		MergeWithAdjacentLeafNodes( bsp, node );
		node->SetFlag( NODE_DONE );
		return;
	}

	MergeLeafNodes_r( bsp, node->GetChild( 0 ) );
	MergeLeafNodes_r( bsp, node->GetChild( 1 ) );

	return;
}

/*
============
idAASBuild::MergeLeafNodes
============
*/
void idAASBuild::MergeLeafNodes( idBrushBSP& bsp )
{
	numMergedLeafNodes = 0;

	common->Printf( "[Merge Leaf Nodes]\n" );

	MergeLeafNodes_r( bsp, bsp.GetRootNode() );
	bsp.GetRootNode()->RemoveFlagRecurse( NODE_DONE );
	bsp.PruneMergedTree_r( bsp.GetRootNode() );

	common->Printf( "\r%6d leaf nodes merged\n", numMergedLeafNodes );
}
