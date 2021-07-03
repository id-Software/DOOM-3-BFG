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

#define VERTEX_HASH_BOXSIZE				(1<<6)	// must be power of 2
#define VERTEX_HASH_SIZE				(VERTEX_HASH_BOXSIZE*VERTEX_HASH_BOXSIZE)
#define EDGE_HASH_SIZE					(1<<14)

#define INTEGRAL_EPSILON				0.01f
#define VERTEX_EPSILON					0.1f

#define AAS_PLANE_NORMAL_EPSILON		0.00001f
#define AAS_PLANE_DIST_EPSILON			0.01f


idHashIndex* aas_vertexHash;
idHashIndex* aas_edgeHash;
idBounds aas_vertexBounds;
int aas_vertexShift;

/*
================
idAASBuild::SetupHash
================
*/
void idAASBuild::SetupHash()
{
	aas_vertexHash = new idHashIndex( VERTEX_HASH_SIZE, 1024 );
	aas_edgeHash = new idHashIndex( EDGE_HASH_SIZE, 1024 );
}

/*
================
idAASBuild::ShutdownHash
================
*/
void idAASBuild::ShutdownHash()
{
	delete aas_vertexHash;
	delete aas_edgeHash;
}

/*
================
idAASBuild::ClearHash
================
*/
void idAASBuild::ClearHash( const idBounds& bounds )
{
	int i;
	float f, max;

	aas_vertexHash->Clear();
	aas_edgeHash->Clear();
	aas_vertexBounds = bounds;

	max = bounds[1].x - bounds[0].x;
	f = bounds[1].y - bounds[0].y;
	if( f > max )
	{
		max = f;
	}
	aas_vertexShift = ( float ) max / VERTEX_HASH_BOXSIZE;
	for( i = 0; ( 1 << i ) < aas_vertexShift; i++ )
	{
	}
	if( i == 0 )
	{
		aas_vertexShift = 1;
	}
	else
	{
		aas_vertexShift = i;
	}
}

/*
================
idAASBuild::HashVec
================
*/
ID_INLINE int idAASBuild::HashVec( const idVec3& vec )
{
	int x, y;

	x = ( ( ( int )( vec[0] - aas_vertexBounds[0].x + 0.5 ) ) + 2 ) >> 2;
	y = ( ( ( int )( vec[1] - aas_vertexBounds[0].y + 0.5 ) ) + 2 ) >> 2;
	return ( x + y * VERTEX_HASH_BOXSIZE ) & ( VERTEX_HASH_SIZE - 1 );
}

/*
================
idAASBuild::GetVertex
================
*/
bool idAASBuild::GetVertex( const idVec3& v, int* vertexNum )
{
	int i, hashKey, vn;
	aasVertex_t vert, *p;

	for( i = 0; i < 3; i++ )
	{
		if( idMath::Fabs( v[i] - idMath::Rint( v[i] ) ) < INTEGRAL_EPSILON )
		{
			vert[i] = idMath::Rint( v[i] );
		}
		else
		{
			vert[i] = v[i];
		}
	}

	hashKey = idAASBuild::HashVec( vert );

	for( vn = aas_vertexHash->First( hashKey ); vn >= 0; vn = aas_vertexHash->Next( vn ) )
	{
		p = &file->vertices[vn];
		// first compare z-axis because hash is based on x-y plane
		if( idMath::Fabs( vert.z - p->z ) < VERTEX_EPSILON &&
				idMath::Fabs( vert.x - p->x ) < VERTEX_EPSILON &&
				idMath::Fabs( vert.y - p->y ) < VERTEX_EPSILON )
		{
			*vertexNum = vn;
			return true;
		}
	}

	*vertexNum = file->vertices.Num();
	aas_vertexHash->Add( hashKey, file->vertices.Num() );
	file->vertices.Append( vert );

	return false;
}

/*
================
idAASBuild::GetEdge
================
*/
bool idAASBuild::GetEdge( const idVec3& v1, const idVec3& v2, int* edgeNum, int v1num )
{
	int v2num, hashKey, e;
	int* vertexNum;
	aasEdge_t edge;
	bool found;

	if( v1num != -1 )
	{
		found = true;
	}
	else
	{
		found = GetVertex( v1, &v1num );
	}
	found &= GetVertex( v2, &v2num );
	// if both vertexes are the same or snapped onto each other
	if( v1num == v2num )
	{
		*edgeNum = 0;
		return true;
	}
	hashKey = aas_edgeHash->GenerateKey( v1num, v2num );
	// if both vertexes where already stored
	if( found )
	{
		for( e = aas_edgeHash->First( hashKey ); e >= 0; e = aas_edgeHash->Next( e ) )
		{

			vertexNum = file->edges[e].vertexNum;
			if( vertexNum[0] == v2num )
			{
				if( vertexNum[1] == v1num )
				{
					// negative for a reversed edge
					*edgeNum = -e;
					break;
				}
			}
			else if( vertexNum[0] == v1num )
			{
				if( vertexNum[1] == v2num )
				{
					*edgeNum = e;
					break;
				}
			}
		}
		// if edge found in hash
		if( e >= 0 )
		{
			return true;
		}
	}

	*edgeNum = file->edges.Num();
	aas_edgeHash->Add( hashKey, file->edges.Num() );

	edge.vertexNum[0] = v1num;
	edge.vertexNum[1] = v2num;

	file->edges.Append( edge );

	return false;
}

/*
================
idAASBuild::GetFaceForPortal
================
*/
bool idAASBuild::GetFaceForPortal( idBrushBSPPortal* portal, int side, int* faceNum )
{
	int i, j, v1num;
	int numFaceEdges, faceEdges[MAX_POINTS_ON_WINDING];
	idWinding* w;
	aasFace_t face;

	if( portal->GetFaceNum() > 0 )
	{
		if( side )
		{
			*faceNum = -portal->GetFaceNum();
		}
		else
		{
			*faceNum = portal->GetFaceNum();
		}
		return true;
	}

	w = portal->GetWinding();
	// turn the winding into a sequence of edges
	numFaceEdges = 0;
	v1num = -1;		// first vertex unknown
	for( i = 0; i < w->GetNumPoints(); i++ )
	{

		GetEdge( ( *w )[i].ToVec3(), ( *w )[( i + 1 ) % w->GetNumPoints()].ToVec3(), &faceEdges[numFaceEdges], v1num );

		if( faceEdges[numFaceEdges] )
		{
			// last vertex of this edge is the first vertex of the next edge
			v1num = file->edges[abs( faceEdges[numFaceEdges] )].vertexNum[INT32_SIGNBITNOTSET( faceEdges[numFaceEdges] )];

			// this edge is valid so keep it
			numFaceEdges++;
		}
	}

	// should have at least 3 edges
	if( numFaceEdges < 3 )
	{
		return false;
	}

	// the polygon is invalid if some edge is found twice
	for( i = 0; i < numFaceEdges; i++ )
	{
		for( j = i + 1; j < numFaceEdges; j++ )
		{
			if( faceEdges[i] == faceEdges[j] || faceEdges[i] == -faceEdges[j] )
			{
				return false;
			}
		}
	}

	portal->SetFaceNum( file->faces.Num() );

	face.planeNum = file->planeList.FindPlane( portal->GetPlane(), AAS_PLANE_NORMAL_EPSILON, AAS_PLANE_DIST_EPSILON );
	face.flags = portal->GetFlags();
	face.areas[0] = face.areas[1] = 0;
	face.firstEdge = file->edgeIndex.Num();
	face.numEdges = numFaceEdges;
	for( i = 0; i < numFaceEdges; i++ )
	{
		file->edgeIndex.Append( faceEdges[i] );
	}
	if( side )
	{
		*faceNum = -file->faces.Num();
	}
	else
	{
		*faceNum = file->faces.Num();
	}
	file->faces.Append( face );

	return true;
}

/*
================
idAASBuild::GetAreaForLeafNode
================
*/
bool idAASBuild::GetAreaForLeafNode( idBrushBSPNode* node, int* areaNum )
{
	int s, faceNum;
	idBrushBSPPortal* p;
	aasArea_t area;

	if( node->GetAreaNum() )
	{
		*areaNum = -node->GetAreaNum();
		return true;
	}

	area.flags = node->GetFlags();
	area.cluster = area.clusterAreaNum = 0;
	area.contents = node->GetContents();
	area.firstFace = file->faceIndex.Num();
	area.numFaces = 0;
	area.reach = NULL;
	area.rev_reach = NULL;

	for( p = node->GetPortals(); p; p = p->Next( s ) )
	{
		s = ( p->GetNode( 1 ) == node );

		if( !GetFaceForPortal( p, s, &faceNum ) )
		{
			continue;
		}

		file->faceIndex.Append( faceNum );
		area.numFaces++;

		if( faceNum > 0 )
		{
			file->faces[abs( faceNum )].areas[0] = file->areas.Num();
		}
		else
		{
			file->faces[abs( faceNum )].areas[1] = file->areas.Num();
		}
	}

	if( !area.numFaces )
	{
		*areaNum = 0;
		return false;
	}

	*areaNum = -file->areas.Num();
	node->SetAreaNum( file->areas.Num() );
	file->areas.Append( area );

	DisplayRealTimeString( "\r%6d", file->areas.Num() );

	return true;
}

/*
================
idAASBuild::StoreTree_r
================
*/
int idAASBuild::StoreTree_r( idBrushBSPNode* node )
{
	int areaNum, nodeNum, child0, child1;
	aasNode_t aasNode;

	if( !node )
	{
		return 0;
	}

	if( node->GetContents() & AREACONTENTS_SOLID )
	{
		return 0;
	}

	if( !node->GetChild( 0 ) && !node->GetChild( 1 ) )
	{
		if( GetAreaForLeafNode( node, &areaNum ) )
		{
			return areaNum;
		}
		return 0;
	}

	aasNode.planeNum = file->planeList.FindPlane( node->GetPlane(), AAS_PLANE_NORMAL_EPSILON, AAS_PLANE_DIST_EPSILON );
	aasNode.children[0] = aasNode.children[1] = 0;
	nodeNum = file->nodes.Num();
	file->nodes.Append( aasNode );

	// !@#$%^ cause of some bug we cannot set the children directly with the StoreTree_r return value
	child0 = StoreTree_r( node->GetChild( 0 ) );
	file->nodes[nodeNum].children[0] = child0;
	child1 = StoreTree_r( node->GetChild( 1 ) );
	file->nodes[nodeNum].children[1] = child1;

	if( !child0 && !child1 )
	{
		file->nodes.SetNum( file->nodes.Num() - 1 );
		return 0;
	}

	return nodeNum;
}

/*
================
idAASBuild::GetSizeEstimate_r
================
*/
typedef struct sizeEstimate_s
{
	int			numEdgeIndexes;
	int			numFaceIndexes;
	int			numAreas;
	int			numNodes;
} sizeEstimate_t;

void idAASBuild::GetSizeEstimate_r( idBrushBSPNode* parent, idBrushBSPNode* node, struct sizeEstimate_s& size )
{
	idBrushBSPPortal* p;
	int s;

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
		// multiple branches of the bsp tree might point to the same leaf node
		if( node->GetParent() == parent )
		{
			size.numAreas++;
			for( p = node->GetPortals(); p; p = p->Next( s ) )
			{
				s = ( p->GetNode( 1 ) == node );
				size.numFaceIndexes++;
				size.numEdgeIndexes += p->GetWinding()->GetNumPoints();
			}
		}
	}
	else
	{
		size.numNodes++;
	}

	GetSizeEstimate_r( node, node->GetChild( 0 ), size );
	GetSizeEstimate_r( node, node->GetChild( 1 ), size );
}

/*
================
idAASBuild::SetSizeEstimate
================
*/
void idAASBuild::SetSizeEstimate( const idBrushBSP& bsp, idAASFileLocal* file )
{
	sizeEstimate_t size;

	size.numEdgeIndexes = 1;
	size.numFaceIndexes = 1;
	size.numAreas = 1;
	size.numNodes = 1;

	GetSizeEstimate_r( NULL, bsp.GetRootNode(), size );

	file->planeList.Resize( size.numNodes / 2, 1024 );
	file->vertices.Resize( size.numEdgeIndexes / 3, 1024 );
	file->edges.Resize( size.numEdgeIndexes / 2, 1024 );
	file->edgeIndex.Resize( size.numEdgeIndexes, 4096 );
	file->faces.Resize( size.numFaceIndexes, 1024 );
	file->faceIndex.Resize( size.numFaceIndexes, 4096 );
	file->areas.Resize( size.numAreas, 1024 );
	file->nodes.Resize( size.numNodes, 1024 );
}

/*
================
idAASBuild::StoreFile
================
*/
bool idAASBuild::StoreFile( const idBrushBSP& bsp )
{
	aasEdge_t edge;
	aasFace_t face;
	aasArea_t area;
	aasNode_t node;

	common->Printf( "[Store AAS]\n" );

	SetupHash();
	ClearHash( bsp.GetTreeBounds() );

	file = new idAASFileLocal();

	file->Clear();

	SetSizeEstimate( bsp, file );

	// the first edge is a dummy
	memset( &edge, 0, sizeof( edge ) );
	file->edges.Append( edge );

	// the first face is a dummy
	memset( &face, 0, sizeof( face ) );
	file->faces.Append( face );

	// the first area is a dummy
	memset( &area, 0, sizeof( area ) );
	file->areas.Append( area );

	// the first node is a dummy
	memset( &node, 0, sizeof( node ) );
	file->nodes.Append( node );

	// store the tree
	StoreTree_r( bsp.GetRootNode() );

	// calculate area bounds and a reachable point in the area
	file->FinishAreas();

	ShutdownHash();

	common->Printf( "\r%6d areas\n", file->areas.Num() );

	return true;
}
