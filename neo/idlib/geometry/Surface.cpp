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

#pragma hdrstop
#include "precompiled.h"

/*
=================
UpdateVertexIndex
=================
*/
ID_INLINE int UpdateVertexIndex( int vertexIndexNum[2], int* vertexRemap, int* vertexCopyIndex, int vertNum )
{
	int s = INT32_SIGNBITSET( vertexRemap[vertNum] );
	vertexIndexNum[0] = vertexRemap[vertNum];
	vertexRemap[vertNum] = vertexIndexNum[s];
	vertexIndexNum[1] += s;
	vertexCopyIndex[vertexRemap[vertNum]] = vertNum;
	return vertexRemap[vertNum];
}

/*
=================
idSurface::Split
=================
*/
int idSurface::Split( const idPlane& plane, const float epsilon, idSurface** front, idSurface** back, int* frontOnPlaneEdges, int* backOnPlaneEdges ) const
{
	float* 			dists;
	float			f;
	byte* 			sides;
	int				counts[3];
	int* 			edgeSplitVertex;
	int				numEdgeSplitVertexes;
	int* 			vertexRemap[2];
	int				vertexIndexNum[2][2];
	int* 			vertexCopyIndex[2];
	int* 			indexPtr[2];
	int				indexNum[2];
	int* 			index;
	int* 			onPlaneEdges[2];
	int				numOnPlaneEdges[2];
	int				maxOnPlaneEdges;
	int				i;
	idSurface* 		surface[2];
	idDrawVert		v;

	dists = ( float* ) _alloca( verts.Num() * sizeof( float ) );
	sides = ( byte* ) _alloca( verts.Num() * sizeof( byte ) );

	counts[0] = counts[1] = counts[2] = 0;

	// determine side for each vertex
	for( i = 0; i < verts.Num(); i++ )
	{
		dists[i] = f = plane.Distance( verts[i].xyz );
		if( f > epsilon )
		{
			sides[i] = SIDE_FRONT;
		}
		else if( f < -epsilon )
		{
			sides[i] = SIDE_BACK;
		}
		else
		{
			sides[i] = SIDE_ON;
		}
		counts[sides[i]]++;
	}

	*front = *back = NULL;

	// if coplanar, put on the front side if the normals match
	if( !counts[SIDE_FRONT] && !counts[SIDE_BACK] )
	{

		f = ( verts[indexes[1]].xyz - verts[indexes[0]].xyz ).Cross( verts[indexes[0]].xyz - verts[indexes[2]].xyz ) * plane.Normal();
		if( IEEE_FLT_SIGNBITSET( f ) )
		{
			*back = new( TAG_IDLIB_SURFACE ) idSurface( *this );
			return SIDE_BACK;
		}
		else
		{
			*front = new( TAG_IDLIB_SURFACE ) idSurface( *this );
			return SIDE_FRONT;
		}
	}
	// if nothing at the front of the clipping plane
	if( !counts[SIDE_FRONT] )
	{
		*back = new( TAG_IDLIB_SURFACE ) idSurface( *this );
		return SIDE_BACK;
	}
	// if nothing at the back of the clipping plane
	if( !counts[SIDE_BACK] )
	{
		*front = new( TAG_IDLIB_SURFACE ) idSurface( *this );
		return SIDE_FRONT;
	}

	// allocate front and back surface
	*front = surface[0] = new( TAG_IDLIB_SURFACE ) idSurface();
	*back = surface[1] = new( TAG_IDLIB_SURFACE ) idSurface();

	edgeSplitVertex = ( int* ) _alloca( edges.Num() * sizeof( int ) );
	numEdgeSplitVertexes = 0;

	maxOnPlaneEdges = 4 * counts[SIDE_ON];
	counts[SIDE_FRONT] = counts[SIDE_BACK] = counts[SIDE_ON] = 0;

	// split edges
	for( i = 0; i < edges.Num(); i++ )
	{
		int v0 = edges[i].verts[0];
		int v1 = edges[i].verts[1];
		int sidesOr = ( sides[v0] | sides[v1] );

		// if both vertexes are on the same side or one is on the clipping plane
		if( !( sides[v0] ^ sides[v1] ) || ( sidesOr & SIDE_ON ) )
		{
			edgeSplitVertex[i] = -1;
			counts[sidesOr & SIDE_BACK]++;
			counts[SIDE_ON] += ( sidesOr & SIDE_ON ) >> 1;
		}
		else
		{
			f = dists[v0] / ( dists[v0] - dists[v1] );
			v.LerpAll( verts[v0], verts[v1], f );
			edgeSplitVertex[i] = numEdgeSplitVertexes++;
			surface[0]->verts.Append( v );
			surface[1]->verts.Append( v );
		}
	}

	// each edge is shared by at most two triangles, as such there can never be more indexes than twice the number of edges
	surface[0]->indexes.Resize( ( ( counts[SIDE_FRONT] + counts[SIDE_ON] ) * 2 ) + ( numEdgeSplitVertexes * 4 ) );
	surface[1]->indexes.Resize( ( ( counts[SIDE_BACK] + counts[SIDE_ON] ) * 2 ) + ( numEdgeSplitVertexes * 4 ) );

	// allocate indexes to construct the triangle indexes for the front and back surface
	vertexRemap[0] = ( int* ) _alloca( verts.Num() * sizeof( int ) );
	memset( vertexRemap[0], -1, verts.Num() * sizeof( int ) );
	vertexRemap[1] = ( int* ) _alloca( verts.Num() * sizeof( int ) );
	memset( vertexRemap[1], -1, verts.Num() * sizeof( int ) );

	vertexCopyIndex[0] = ( int* ) _alloca( ( numEdgeSplitVertexes + verts.Num() ) * sizeof( int ) );
	vertexCopyIndex[1] = ( int* ) _alloca( ( numEdgeSplitVertexes + verts.Num() ) * sizeof( int ) );

	vertexIndexNum[0][0] = vertexIndexNum[1][0] = 0;
	vertexIndexNum[0][1] = vertexIndexNum[1][1] = numEdgeSplitVertexes;

	indexPtr[0] = surface[0]->indexes.Ptr();
	indexPtr[1] = surface[1]->indexes.Ptr();
	indexNum[0] = surface[0]->indexes.Num();
	indexNum[1] = surface[1]->indexes.Num();

	maxOnPlaneEdges += 4 * numEdgeSplitVertexes;
	// allocate one more in case no triangles are actually split which may happen for a disconnected surface
	onPlaneEdges[0] = ( int* ) _alloca( ( maxOnPlaneEdges + 1 ) * sizeof( int ) );
	onPlaneEdges[1] = ( int* ) _alloca( ( maxOnPlaneEdges + 1 ) * sizeof( int ) );
	numOnPlaneEdges[0] = numOnPlaneEdges[1] = 0;

	// split surface triangles
	for( i = 0; i < edgeIndexes.Num(); i += 3 )
	{
		int e0, e1, e2, v0, v1, v2, s, n;

		e0 = abs( edgeIndexes[i + 0] );
		e1 = abs( edgeIndexes[i + 1] );
		e2 = abs( edgeIndexes[i + 2] );

		v0 = indexes[i + 0];
		v1 = indexes[i + 1];
		v2 = indexes[i + 2];

		switch( ( INT32_SIGNBITSET( edgeSplitVertex[e0] ) | ( INT32_SIGNBITSET( edgeSplitVertex[e1] ) << 1 ) | ( INT32_SIGNBITSET( edgeSplitVertex[e2] ) << 2 ) ) ^ 7 )
		{
			case 0:  	// no edges split
			{
				if( ( sides[v0] & sides[v1] & sides[v2] ) & SIDE_ON )
				{
					// coplanar
					f = ( verts[v1].xyz - verts[v0].xyz ).Cross( verts[v0].xyz - verts[v2].xyz ) * plane.Normal();
					s = IEEE_FLT_SIGNBITSET( f );
				}
				else
				{
					s = ( sides[v0] | sides[v1] | sides[v2] ) & SIDE_BACK;
				}
				n = indexNum[s];
				onPlaneEdges[s][numOnPlaneEdges[s]] = n;
				numOnPlaneEdges[s] += ( sides[v0] & sides[v1] ) >> 1;
				onPlaneEdges[s][numOnPlaneEdges[s]] = n + 1;
				numOnPlaneEdges[s] += ( sides[v1] & sides[v2] ) >> 1;
				onPlaneEdges[s][numOnPlaneEdges[s]] = n + 2;
				numOnPlaneEdges[s] += ( sides[v2] & sides[v0] ) >> 1;
				index = indexPtr[s];
				index[n++] = UpdateVertexIndex( vertexIndexNum[s], vertexRemap[s], vertexCopyIndex[s], v0 );
				index[n++] = UpdateVertexIndex( vertexIndexNum[s], vertexRemap[s], vertexCopyIndex[s], v1 );
				index[n++] = UpdateVertexIndex( vertexIndexNum[s], vertexRemap[s], vertexCopyIndex[s], v2 );
				indexNum[s] = n;
				break;
			}
			case 1:  	// first edge split
			{
				s = sides[v0] & SIDE_BACK;
				n = indexNum[s];
				onPlaneEdges[s][numOnPlaneEdges[s]++] = n;
				index = indexPtr[s];
				index[n++] = edgeSplitVertex[e0];
				index[n++] = UpdateVertexIndex( vertexIndexNum[s], vertexRemap[s], vertexCopyIndex[s], v2 );
				index[n++] = UpdateVertexIndex( vertexIndexNum[s], vertexRemap[s], vertexCopyIndex[s], v0 );
				indexNum[s] = n;
				s ^= 1;
				n = indexNum[s];
				onPlaneEdges[s][numOnPlaneEdges[s]++] = n;
				index = indexPtr[s];
				index[n++] = UpdateVertexIndex( vertexIndexNum[s], vertexRemap[s], vertexCopyIndex[s], v2 );
				index[n++] = edgeSplitVertex[e0];
				index[n++] = UpdateVertexIndex( vertexIndexNum[s], vertexRemap[s], vertexCopyIndex[s], v1 );
				indexNum[s] = n;
				break;
			}
			case 2:  	// second edge split
			{
				s = sides[v1] & SIDE_BACK;
				n = indexNum[s];
				onPlaneEdges[s][numOnPlaneEdges[s]++] = n;
				index = indexPtr[s];
				index[n++] = edgeSplitVertex[e1];
				index[n++] = UpdateVertexIndex( vertexIndexNum[s], vertexRemap[s], vertexCopyIndex[s], v0 );
				index[n++] = UpdateVertexIndex( vertexIndexNum[s], vertexRemap[s], vertexCopyIndex[s], v1 );
				indexNum[s] = n;
				s ^= 1;
				n = indexNum[s];
				onPlaneEdges[s][numOnPlaneEdges[s]++] = n;
				index = indexPtr[s];
				index[n++] = UpdateVertexIndex( vertexIndexNum[s], vertexRemap[s], vertexCopyIndex[s], v0 );
				index[n++] = edgeSplitVertex[e1];
				index[n++] = UpdateVertexIndex( vertexIndexNum[s], vertexRemap[s], vertexCopyIndex[s], v2 );
				indexNum[s] = n;
				break;
			}
			case 3:  	// first and second edge split
			{
				s = sides[v1] & SIDE_BACK;
				n = indexNum[s];
				onPlaneEdges[s][numOnPlaneEdges[s]++] = n;
				index = indexPtr[s];
				index[n++] = edgeSplitVertex[e1];
				index[n++] = edgeSplitVertex[e0];
				index[n++] = UpdateVertexIndex( vertexIndexNum[s], vertexRemap[s], vertexCopyIndex[s], v1 );
				indexNum[s] = n;
				s ^= 1;
				n = indexNum[s];
				onPlaneEdges[s][numOnPlaneEdges[s]++] = n;
				index = indexPtr[s];
				index[n++] = edgeSplitVertex[e0];
				index[n++] = edgeSplitVertex[e1];
				index[n++] = UpdateVertexIndex( vertexIndexNum[s], vertexRemap[s], vertexCopyIndex[s], v0 );
				index[n++] = edgeSplitVertex[e1];
				index[n++] = UpdateVertexIndex( vertexIndexNum[s], vertexRemap[s], vertexCopyIndex[s], v2 );
				index[n++] = UpdateVertexIndex( vertexIndexNum[s], vertexRemap[s], vertexCopyIndex[s], v0 );
				indexNum[s] = n;
				break;
			}
			case 4:  	// third edge split
			{
				s = sides[v2] & SIDE_BACK;
				n = indexNum[s];
				onPlaneEdges[s][numOnPlaneEdges[s]++] = n;
				index = indexPtr[s];
				index[n++] = edgeSplitVertex[e2];
				index[n++] = UpdateVertexIndex( vertexIndexNum[s], vertexRemap[s], vertexCopyIndex[s], v1 );
				index[n++] = UpdateVertexIndex( vertexIndexNum[s], vertexRemap[s], vertexCopyIndex[s], v2 );
				indexNum[s] = n;
				s ^= 1;
				n = indexNum[s];
				onPlaneEdges[s][numOnPlaneEdges[s]++] = n;
				index = indexPtr[s];
				index[n++] = UpdateVertexIndex( vertexIndexNum[s], vertexRemap[s], vertexCopyIndex[s], v1 );
				index[n++] = edgeSplitVertex[e2];
				index[n++] = UpdateVertexIndex( vertexIndexNum[s], vertexRemap[s], vertexCopyIndex[s], v0 );
				indexNum[s] = n;
				break;
			}
			case 5:  	// first and third edge split
			{
				s = sides[v0] & SIDE_BACK;
				n = indexNum[s];
				onPlaneEdges[s][numOnPlaneEdges[s]++] = n;
				index = indexPtr[s];
				index[n++] = edgeSplitVertex[e0];
				index[n++] = edgeSplitVertex[e2];
				index[n++] = UpdateVertexIndex( vertexIndexNum[s], vertexRemap[s], vertexCopyIndex[s], v0 );
				indexNum[s] = n;
				s ^= 1;
				n = indexNum[s];
				onPlaneEdges[s][numOnPlaneEdges[s]++] = n;
				index = indexPtr[s];
				index[n++] = edgeSplitVertex[e2];
				index[n++] = edgeSplitVertex[e0];
				index[n++] = UpdateVertexIndex( vertexIndexNum[s], vertexRemap[s], vertexCopyIndex[s], v1 );
				index[n++] = UpdateVertexIndex( vertexIndexNum[s], vertexRemap[s], vertexCopyIndex[s], v1 );
				index[n++] = UpdateVertexIndex( vertexIndexNum[s], vertexRemap[s], vertexCopyIndex[s], v2 );
				index[n++] = edgeSplitVertex[e2];
				indexNum[s] = n;
				break;
			}
			case 6:  	// second and third edge split
			{
				s = sides[v2] & SIDE_BACK;
				n = indexNum[s];
				onPlaneEdges[s][numOnPlaneEdges[s]++] = n;
				index = indexPtr[s];
				index[n++] = edgeSplitVertex[e2];
				index[n++] = edgeSplitVertex[e1];
				index[n++] = UpdateVertexIndex( vertexIndexNum[s], vertexRemap[s], vertexCopyIndex[s], v2 );
				indexNum[s] = n;
				s ^= 1;
				n = indexNum[s];
				onPlaneEdges[s][numOnPlaneEdges[s]++] = n;
				index = indexPtr[s];
				index[n++] = edgeSplitVertex[e1];
				index[n++] = edgeSplitVertex[e2];
				index[n++] = UpdateVertexIndex( vertexIndexNum[s], vertexRemap[s], vertexCopyIndex[s], v1 );
				index[n++] = UpdateVertexIndex( vertexIndexNum[s], vertexRemap[s], vertexCopyIndex[s], v0 );
				index[n++] = UpdateVertexIndex( vertexIndexNum[s], vertexRemap[s], vertexCopyIndex[s], v1 );
				index[n++] = edgeSplitVertex[e2];
				indexNum[s] = n;
				break;
			}
		}
	}

	surface[0]->indexes.SetNum( indexNum[0] );
	surface[1]->indexes.SetNum( indexNum[1] );

	// copy vertexes
	surface[0]->verts.SetNum( vertexIndexNum[0][1] );
	index = vertexCopyIndex[0];
	for( i = numEdgeSplitVertexes; i < surface[0]->verts.Num(); i++ )
	{
		surface[0]->verts[i] = verts[index[i]];
	}
	surface[1]->verts.SetNum( vertexIndexNum[1][1] );
	index = vertexCopyIndex[1];
	for( i = numEdgeSplitVertexes; i < surface[1]->verts.Num(); i++ )
	{
		surface[1]->verts[i] = verts[index[i]];
	}

	// generate edge indexes
	surface[0]->GenerateEdgeIndexes();
	surface[1]->GenerateEdgeIndexes();

	if( frontOnPlaneEdges )
	{
		memcpy( frontOnPlaneEdges, onPlaneEdges[0], numOnPlaneEdges[0] * sizeof( int ) );
		frontOnPlaneEdges[numOnPlaneEdges[0]] = -1;
	}

	if( backOnPlaneEdges )
	{
		memcpy( backOnPlaneEdges, onPlaneEdges[1], numOnPlaneEdges[1] * sizeof( int ) );
		backOnPlaneEdges[numOnPlaneEdges[1]] = -1;
	}

	return SIDE_CROSS;
}

/*
=================
idSurface::ClipInPlace
=================
*/
bool idSurface::ClipInPlace( const idPlane& plane, const float epsilon, const bool keepOn )
{
	float* 			dists;
	float			f;
	byte* 			sides;
	int				counts[3];
	int				i;
	int* 			edgeSplitVertex;
	int* 			vertexRemap;
	int				vertexIndexNum[2];
	int* 			vertexCopyIndex;
	int* 			indexPtr;
	int				indexNum;
	int				numEdgeSplitVertexes;
	idDrawVert		v;
	idList<idDrawVert, TAG_IDLIB_LIST_SURFACE> newVerts;
	idList<int, TAG_IDLIB_LIST_SURFACE>		newIndexes;

	dists = ( float* ) _alloca( verts.Num() * sizeof( float ) );
	sides = ( byte* ) _alloca( verts.Num() * sizeof( byte ) );

	counts[0] = counts[1] = counts[2] = 0;

	// determine side for each vertex
	for( i = 0; i < verts.Num(); i++ )
	{
		dists[i] = f = plane.Distance( verts[i].xyz );
		if( f > epsilon )
		{
			sides[i] = SIDE_FRONT;
		}
		else if( f < -epsilon )
		{
			sides[i] = SIDE_BACK;
		}
		else
		{
			sides[i] = SIDE_ON;
		}
		counts[sides[i]]++;
	}

	// if coplanar, put on the front side if the normals match
	if( !counts[SIDE_FRONT] && !counts[SIDE_BACK] )
	{

		f = ( verts[indexes[1]].xyz - verts[indexes[0]].xyz ).Cross( verts[indexes[0]].xyz - verts[indexes[2]].xyz ) * plane.Normal();
		if( IEEE_FLT_SIGNBITSET( f ) )
		{
			Clear();
			return false;
		}
		else
		{
			return true;
		}
	}
	// if nothing at the front of the clipping plane
	if( !counts[SIDE_FRONT] )
	{
		Clear();
		return false;
	}
	// if nothing at the back of the clipping plane
	if( !counts[SIDE_BACK] )
	{
		return true;
	}

	edgeSplitVertex = ( int* ) _alloca( edges.Num() * sizeof( int ) );
	numEdgeSplitVertexes = 0;

	counts[SIDE_FRONT] = counts[SIDE_BACK] = 0;

	// split edges
	for( i = 0; i < edges.Num(); i++ )
	{
		int v0 = edges[i].verts[0];
		int v1 = edges[i].verts[1];

		// if both vertexes are on the same side or one is on the clipping plane
		if( !( sides[v0] ^ sides[v1] ) || ( ( sides[v0] | sides[v1] ) & SIDE_ON ) )
		{
			edgeSplitVertex[i] = -1;
			counts[( sides[v0] | sides[v1] ) & SIDE_BACK]++;
		}
		else
		{
			f = dists[v0] / ( dists[v0] - dists[v1] );
			v.LerpAll( verts[v0], verts[v1], f );
			edgeSplitVertex[i] = numEdgeSplitVertexes++;
			newVerts.Append( v );
		}
	}

	// each edge is shared by at most two triangles, as such there can never be
	// more indexes than twice the number of edges
	newIndexes.Resize( ( counts[SIDE_FRONT] << 1 ) + ( numEdgeSplitVertexes << 2 ) );

	// allocate indexes to construct the triangle indexes for the front and back surface
	vertexRemap = ( int* ) _alloca( verts.Num() * sizeof( int ) );
	memset( vertexRemap, -1, verts.Num() * sizeof( int ) );

	vertexCopyIndex = ( int* ) _alloca( ( numEdgeSplitVertexes + verts.Num() ) * sizeof( int ) );

	vertexIndexNum[0] = 0;
	vertexIndexNum[1] = numEdgeSplitVertexes;

	indexPtr = newIndexes.Ptr();
	indexNum = newIndexes.Num();

	// split surface triangles
	for( i = 0; i < edgeIndexes.Num(); i += 3 )
	{
		int e0, e1, e2, v0, v1, v2;

		e0 = abs( edgeIndexes[i + 0] );
		e1 = abs( edgeIndexes[i + 1] );
		e2 = abs( edgeIndexes[i + 2] );

		v0 = indexes[i + 0];
		v1 = indexes[i + 1];
		v2 = indexes[i + 2];

		switch( ( INT32_SIGNBITSET( edgeSplitVertex[e0] ) | ( INT32_SIGNBITSET( edgeSplitVertex[e1] ) << 1 ) | ( INT32_SIGNBITSET( edgeSplitVertex[e2] ) << 2 ) ) ^ 7 )
		{
			case 0:  	// no edges split
			{
				if( ( sides[v0] | sides[v1] | sides[v2] ) & SIDE_BACK )
				{
					break;
				}
				if( ( sides[v0] & sides[v1] & sides[v2] ) & SIDE_ON )
				{
					// coplanar
					if( !keepOn )
					{
						break;
					}
					f = ( verts[v1].xyz - verts[v0].xyz ).Cross( verts[v0].xyz - verts[v2].xyz ) * plane.Normal();
					if( IEEE_FLT_SIGNBITSET( f ) )
					{
						break;
					}
				}
				indexPtr[indexNum++] = UpdateVertexIndex( vertexIndexNum, vertexRemap, vertexCopyIndex, v0 );
				indexPtr[indexNum++] = UpdateVertexIndex( vertexIndexNum, vertexRemap, vertexCopyIndex, v1 );
				indexPtr[indexNum++] = UpdateVertexIndex( vertexIndexNum, vertexRemap, vertexCopyIndex, v2 );
				break;
			}
			case 1:  	// first edge split
			{
				if( !( sides[v0] & SIDE_BACK ) )
				{
					indexPtr[indexNum++] = UpdateVertexIndex( vertexIndexNum, vertexRemap, vertexCopyIndex, v0 );
					indexPtr[indexNum++] = edgeSplitVertex[e0];
					indexPtr[indexNum++] = UpdateVertexIndex( vertexIndexNum, vertexRemap, vertexCopyIndex, v2 );
				}
				else
				{
					indexPtr[indexNum++] = edgeSplitVertex[e0];
					indexPtr[indexNum++] = UpdateVertexIndex( vertexIndexNum, vertexRemap, vertexCopyIndex, v1 );
					indexPtr[indexNum++] = UpdateVertexIndex( vertexIndexNum, vertexRemap, vertexCopyIndex, v2 );
				}
				break;
			}
			case 2:  	// second edge split
			{
				if( !( sides[v1] & SIDE_BACK ) )
				{
					indexPtr[indexNum++] = UpdateVertexIndex( vertexIndexNum, vertexRemap, vertexCopyIndex, v1 );
					indexPtr[indexNum++] = edgeSplitVertex[e1];
					indexPtr[indexNum++] = UpdateVertexIndex( vertexIndexNum, vertexRemap, vertexCopyIndex, v0 );
				}
				else
				{
					indexPtr[indexNum++] = edgeSplitVertex[e1];
					indexPtr[indexNum++] = UpdateVertexIndex( vertexIndexNum, vertexRemap, vertexCopyIndex, v2 );
					indexPtr[indexNum++] = UpdateVertexIndex( vertexIndexNum, vertexRemap, vertexCopyIndex, v0 );
				}
				break;
			}
			case 3:  	// first and second edge split
			{
				if( !( sides[v1] & SIDE_BACK ) )
				{
					indexPtr[indexNum++] = UpdateVertexIndex( vertexIndexNum, vertexRemap, vertexCopyIndex, v1 );
					indexPtr[indexNum++] = edgeSplitVertex[e1];
					indexPtr[indexNum++] = edgeSplitVertex[e0];
				}
				else
				{
					indexPtr[indexNum++] = UpdateVertexIndex( vertexIndexNum, vertexRemap, vertexCopyIndex, v0 );
					indexPtr[indexNum++] = edgeSplitVertex[e0];
					indexPtr[indexNum++] = edgeSplitVertex[e1];
					indexPtr[indexNum++] = edgeSplitVertex[e1];
					indexPtr[indexNum++] = UpdateVertexIndex( vertexIndexNum, vertexRemap, vertexCopyIndex, v2 );
					indexPtr[indexNum++] = UpdateVertexIndex( vertexIndexNum, vertexRemap, vertexCopyIndex, v0 );
				}
				break;
			}
			case 4:  	// third edge split
			{
				if( !( sides[v2] & SIDE_BACK ) )
				{
					indexPtr[indexNum++] = UpdateVertexIndex( vertexIndexNum, vertexRemap, vertexCopyIndex, v2 );
					indexPtr[indexNum++] = edgeSplitVertex[e2];
					indexPtr[indexNum++] = UpdateVertexIndex( vertexIndexNum, vertexRemap, vertexCopyIndex, v1 );
				}
				else
				{
					indexPtr[indexNum++] = edgeSplitVertex[e2];
					indexPtr[indexNum++] = UpdateVertexIndex( vertexIndexNum, vertexRemap, vertexCopyIndex, v0 );
					indexPtr[indexNum++] = UpdateVertexIndex( vertexIndexNum, vertexRemap, vertexCopyIndex, v1 );
				}
				break;
			}
			case 5:  	// first and third edge split
			{
				if( !( sides[v0] & SIDE_BACK ) )
				{
					indexPtr[indexNum++] = UpdateVertexIndex( vertexIndexNum, vertexRemap, vertexCopyIndex, v0 );
					indexPtr[indexNum++] = edgeSplitVertex[e0];
					indexPtr[indexNum++] = edgeSplitVertex[e2];
				}
				else
				{
					indexPtr[indexNum++] = edgeSplitVertex[e0];
					indexPtr[indexNum++] = UpdateVertexIndex( vertexIndexNum, vertexRemap, vertexCopyIndex, v1 );
					indexPtr[indexNum++] = edgeSplitVertex[e2];
					indexPtr[indexNum++] = UpdateVertexIndex( vertexIndexNum, vertexRemap, vertexCopyIndex, v1 );
					indexPtr[indexNum++] = UpdateVertexIndex( vertexIndexNum, vertexRemap, vertexCopyIndex, v2 );
					indexPtr[indexNum++] = edgeSplitVertex[e2];
				}
				break;
			}
			case 6:  	// second and third edge split
			{
				if( !( sides[v2] & SIDE_BACK ) )
				{
					indexPtr[indexNum++] = UpdateVertexIndex( vertexIndexNum, vertexRemap, vertexCopyIndex, v2 );
					indexPtr[indexNum++] = edgeSplitVertex[e2];
					indexPtr[indexNum++] = edgeSplitVertex[e1];
				}
				else
				{
					indexPtr[indexNum++] = edgeSplitVertex[e2];
					indexPtr[indexNum++] = UpdateVertexIndex( vertexIndexNum, vertexRemap, vertexCopyIndex, v1 );
					indexPtr[indexNum++] = edgeSplitVertex[e1];
					indexPtr[indexNum++] = UpdateVertexIndex( vertexIndexNum, vertexRemap, vertexCopyIndex, v0 );
					indexPtr[indexNum++] = UpdateVertexIndex( vertexIndexNum, vertexRemap, vertexCopyIndex, v1 );
					indexPtr[indexNum++] = edgeSplitVertex[e2];
				}
				break;
			}
		}
	}

	newIndexes.SetNum( indexNum );

	// copy vertexes
	newVerts.SetNum( vertexIndexNum[1] );
	for( i = numEdgeSplitVertexes; i < newVerts.Num(); i++ )
	{
		newVerts[i] = verts[vertexCopyIndex[i]];
	}

	// copy back to this surface
	indexes = newIndexes;
	verts = newVerts;

	GenerateEdgeIndexes();

	return true;
}

/*
=============
idSurface::IsConnected
=============
*/
bool idSurface::IsConnected() const
{
	int i, j, numIslands, numTris;
	int queueStart, queueEnd;
	int* queue, *islandNum;
	int curTri, nextTri, edgeNum;
	const int* index;

	numIslands = 0;
	numTris = indexes.Num() / 3;
	islandNum = ( int* ) _alloca16( numTris * sizeof( int ) );
	memset( islandNum, -1, numTris * sizeof( int ) );
	queue = ( int* ) _alloca16( numTris * sizeof( int ) );

	for( i = 0; i < numTris; i++ )
	{

		if( islandNum[i] != -1 )
		{
			continue;
		}

		queueStart = 0;
		queueEnd = 1;
		queue[0] = i;
		islandNum[i] = numIslands;

		for( curTri = queue[queueStart]; queueStart < queueEnd; curTri = queue[++queueStart] )
		{

			index = &edgeIndexes[curTri * 3];

			for( j = 0; j < 3; j++ )
			{

				edgeNum = index[j];
				nextTri = edges[abs( edgeNum )].tris[INT32_SIGNBITNOTSET( edgeNum )];

				if( nextTri == -1 )
				{
					continue;
				}

				nextTri /= 3;

				if( islandNum[nextTri] != -1 )
				{
					continue;
				}

				queue[queueEnd++] = nextTri;
				islandNum[nextTri] = numIslands;
			}
		}
		numIslands++;
	}

	return ( numIslands == 1 );
}

/*
=================
idSurface::IsClosed
=================
*/
bool idSurface::IsClosed() const
{
	for( int i = 0; i < edges.Num(); i++ )
	{
		if( edges[i].tris[0] < 0 || edges[i].tris[1] < 0 )
		{
			return false;
		}
	}
	return true;
}

/*
=============
idSurface::IsPolytope
=============
*/
bool idSurface::IsPolytope( const float epsilon ) const
{
	int i, j;
	idPlane plane;

	if( !IsClosed() )
	{
		return false;
	}

	for( i = 0; i < indexes.Num(); i += 3 )
	{
		plane.FromPoints( verts[indexes[i + 0]].xyz, verts[indexes[i + 1]].xyz, verts[indexes[i + 2]].xyz );

		for( j = 0; j < verts.Num(); j++ )
		{
			if( plane.Side( verts[j].xyz, epsilon ) == SIDE_FRONT )
			{
				return false;
			}
		}
	}
	return true;
}

/*
=============
idSurface::PlaneDistance
=============
*/
float idSurface::PlaneDistance( const idPlane& plane ) const
{
	int		i;
	float	d, min, max;

	min = idMath::INFINITY;
	max = -min;
	for( i = 0; i < verts.Num(); i++ )
	{
		d = plane.Distance( verts[i].xyz );
		if( d < min )
		{
			min = d;
			if( IEEE_FLT_SIGNBITSET( min ) & IEEE_FLT_SIGNBITNOTSET( max ) )
			{
				return 0.0f;
			}
		}
		if( d > max )
		{
			max = d;
			if( IEEE_FLT_SIGNBITSET( min ) & IEEE_FLT_SIGNBITNOTSET( max ) )
			{
				return 0.0f;
			}
		}
	}
	if( IEEE_FLT_SIGNBITNOTSET( min ) )
	{
		return min;
	}
	if( IEEE_FLT_SIGNBITSET( max ) )
	{
		return max;
	}
	return 0.0f;
}

/*
=============
idSurface::PlaneSide
=============
*/
int idSurface::PlaneSide( const idPlane& plane, const float epsilon ) const
{
	bool	front, back;
	int		i;
	float	d;

	front = false;
	back = false;
	for( i = 0; i < verts.Num(); i++ )
	{
		d = plane.Distance( verts[i].xyz );
		if( d < -epsilon )
		{
			if( front )
			{
				return SIDE_CROSS;
			}
			back = true;
			continue;
		}
		else if( d > epsilon )
		{
			if( back )
			{
				return SIDE_CROSS;
			}
			front = true;
			continue;
		}
	}

	if( back )
	{
		return SIDE_BACK;
	}
	if( front )
	{
		return SIDE_FRONT;
	}
	return SIDE_ON;
}

/*
=================
idSurface::LineIntersection
=================
*/
bool idSurface::LineIntersection( const idVec3& start, const idVec3& end, bool backFaceCull ) const
{
	float scale;

	RayIntersection( start, end - start, scale, false );
	return ( scale >= 0.0f && scale <= 1.0f );
}

/*
=================
idSurface::RayIntersection
=================
*/
bool idSurface::RayIntersection( const idVec3& start, const idVec3& dir, float& scale, bool backFaceCull ) const
{
	int i, i0, i1, i2, s0, s1, s2;
	float d, s;
	byte* sidedness;
	idPluecker rayPl, pl;
	idPlane plane;

	sidedness = ( byte* )_alloca( edges.Num() * sizeof( byte ) );
	scale = idMath::INFINITY;

	rayPl.FromRay( start, dir );

	// ray sidedness for edges
	for( i = 0; i < edges.Num(); i++ )
	{
		pl.FromLine( verts[ edges[i].verts[1] ].xyz, verts[ edges[i].verts[0] ].xyz );
		d = pl.PermutedInnerProduct( rayPl );
		sidedness[ i ] = IEEE_FLT_SIGNBITSET( d );
	}

	// test triangles
	for( i = 0; i < edgeIndexes.Num(); i += 3 )
	{
		i0 = edgeIndexes[i + 0];
		i1 = edgeIndexes[i + 1];
		i2 = edgeIndexes[i + 2];
		s0 = sidedness[abs( i0 )] ^ INT32_SIGNBITSET( i0 );
		s1 = sidedness[abs( i1 )] ^ INT32_SIGNBITSET( i1 );
		s2 = sidedness[abs( i2 )] ^ INT32_SIGNBITSET( i2 );

		if( s0 & s1 & s2 )
		{
			plane.FromPoints( verts[indexes[i + 0]].xyz, verts[indexes[i + 1]].xyz, verts[indexes[i + 2]].xyz );
			plane.RayIntersection( start, dir, s );
			if( idMath::Fabs( s ) < idMath::Fabs( scale ) )
			{
				scale = s;
			}
		}
		else if( !backFaceCull && !( s0 | s1 | s2 ) )
		{
			plane.FromPoints( verts[indexes[i + 0]].xyz, verts[indexes[i + 1]].xyz, verts[indexes[i + 2]].xyz );
			plane.RayIntersection( start, dir, s );
			if( idMath::Fabs( s ) < idMath::Fabs( scale ) )
			{
				scale = s;
			}
		}
	}

	if( idMath::Fabs( scale ) < idMath::INFINITY )
	{
		return true;
	}
	return false;
}

/*
=================
idSurface::GenerateEdgeIndexes

  Assumes each edge is shared by at most two triangles.
=================
*/
void idSurface::GenerateEdgeIndexes()
{
	int i, j, i0, i1, i2, s, v0, v1, edgeNum;
	int* index, *vertexEdges, *edgeChain;
	surfaceEdge_t e[3];

	vertexEdges = ( int* ) _alloca16( verts.Num() * sizeof( int ) );
	memset( vertexEdges, -1, verts.Num() * sizeof( int ) );
	edgeChain = ( int* ) _alloca16( indexes.Num() * sizeof( int ) );

	edgeIndexes.SetNum( indexes.Num() );

	edges.Clear();

	// the first edge is a dummy
	e[0].verts[0] = e[0].verts[1] = e[0].tris[0] = e[0].tris[1] = 0;
	edges.Append( e[0] );

	for( i = 0; i < indexes.Num(); i += 3 )
	{
		index = indexes.Ptr() + i;
		// vertex numbers
		i0 = index[0];
		i1 = index[1];
		i2 = index[2];
		// setup edges each with smallest vertex number first
		s = INT32_SIGNBITSET( i1 - i0 );
		e[0].verts[0] = index[s];
		e[0].verts[1] = index[s ^ 1];
		s = INT32_SIGNBITSET( i2 - i1 ) + 1;
		e[1].verts[0] = index[s];
		e[1].verts[1] = index[s ^ 3];
		s = INT32_SIGNBITSET( i2 - i0 ) << 1;
		e[2].verts[0] = index[s];
		e[2].verts[1] = index[s ^ 2];
		// get edges
		for( j = 0; j < 3; j++ )
		{
			v0 = e[j].verts[0];
			v1 = e[j].verts[1];
			for( edgeNum = vertexEdges[v0]; edgeNum >= 0; edgeNum = edgeChain[edgeNum] )
			{
				if( edges[edgeNum].verts[1] == v1 )
				{
					break;
				}
			}
			// if the edge does not yet exist
			if( edgeNum < 0 )
			{
				e[j].tris[0] = e[j].tris[1] = -1;
				edgeNum = edges.Append( e[j] );
				edgeChain[edgeNum] = vertexEdges[v0];
				vertexEdges[v0] = edgeNum;
			}
			// update edge index and edge tri references
			if( index[j] == v0 )
			{
				assert( edges[edgeNum].tris[0] == -1 ); // edge may not be shared by more than two triangles
				edges[edgeNum].tris[0] = i;
				edgeIndexes[i + j] = edgeNum;
			}
			else
			{
				assert( edges[edgeNum].tris[1] == -1 ); // edge may not be shared by more than two triangles
				edges[edgeNum].tris[1] = i;
				edgeIndexes[i + j] = -edgeNum;
			}
		}
	}
}

/*
=================
idSurface::FindEdge
=================
*/
int idSurface::FindEdge( int v1, int v2 ) const
{
	int i, firstVert, secondVert;

	if( v1 < v2 )
	{
		firstVert = v1;
		secondVert = v2;
	}
	else
	{
		firstVert = v2;
		secondVert = v1;
	}
	for( i = 1; i < edges.Num(); i++ )
	{
		if( edges[i].verts[0] == firstVert )
		{
			if( edges[i].verts[1] == secondVert )
			{
				break;
			}
		}
	}
	if( i < edges.Num() )
	{
		return v1 < v2 ? i : -i;
	}
	return 0;
}
