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

#define POLYTOPE_VERTEX_EPSILON		0.1f

/*
====================
idSurface_Polytope::FromPlanes
====================
*/
void idSurface_Polytope::FromPlanes( const idPlane* planes, const int numPlanes )
{
	int i, j, k, *windingVerts;
	idFixedWinding w;
	idDrawVert newVert;

	windingVerts = ( int* ) _alloca( MAX_POINTS_ON_WINDING * sizeof( int ) );
	memset( &newVert, 0, sizeof( newVert ) );

	for( i = 0; i < numPlanes; i++ )
	{

		w.BaseForPlane( planes[i] );

		for( j = 0; j < numPlanes; j++ )
		{
			if( j == i )
			{
				continue;
			}
			if( !w.ClipInPlace( -planes[j], ON_EPSILON, true ) )
			{
				break;
			}
		}
		if( !w.GetNumPoints() )
		{
			continue;
		}

		for( j = 0; j < w.GetNumPoints(); j++ )
		{
			for( k = 0; k < verts.Num(); k++ )
			{
				if( verts[k].xyz.Compare( w[j].ToVec3(), POLYTOPE_VERTEX_EPSILON ) )
				{
					break;
				}
			}
			if( k >= verts.Num() )
			{
				newVert.xyz = w[j].ToVec3();
				k = verts.Append( newVert );
			}
			windingVerts[j] = k;
		}

		for( j = 2; j < w.GetNumPoints(); j++ )
		{
			indexes.Append( windingVerts[0] );
			indexes.Append( windingVerts[j - 1] );
			indexes.Append( windingVerts[j] );
		}
	}

	GenerateEdgeIndexes();
}

/*
====================
idSurface_Polytope::SetupTetrahedron
====================
*/
void idSurface_Polytope::SetupTetrahedron( const idBounds& bounds )
{
	idVec3 center, scale;
	float c1, c2, c3;

	c1 = 0.4714045207f;
	c2 = 0.8164965809f;
	c3 = -0.3333333333f;

	center = bounds.GetCenter();
	scale = bounds[1] - center;

	verts.SetNum( 4 );
	verts[0].xyz = center + idVec3( 0.0f, 0.0f, scale.z );
	verts[1].xyz = center + idVec3( 2.0f * c1 * scale.x, 0.0f, c3 * scale.z );
	verts[2].xyz = center + idVec3( -c1 * scale.x, c2 * scale.y, c3 * scale.z );
	verts[3].xyz = center + idVec3( -c1 * scale.x, -c2 * scale.y, c3 * scale.z );

	indexes.SetNum( 4 * 3 );
	indexes[0 * 3 + 0] = 0;
	indexes[0 * 3 + 1] = 1;
	indexes[0 * 3 + 2] = 2;
	indexes[1 * 3 + 0] = 0;
	indexes[1 * 3 + 1] = 2;
	indexes[1 * 3 + 2] = 3;
	indexes[2 * 3 + 0] = 0;
	indexes[2 * 3 + 1] = 3;
	indexes[2 * 3 + 2] = 1;
	indexes[3 * 3 + 0] = 1;
	indexes[3 * 3 + 1] = 3;
	indexes[3 * 3 + 2] = 2;

	GenerateEdgeIndexes();
}

/*
====================
idSurface_Polytope::SetupHexahedron
====================
*/
void idSurface_Polytope::SetupHexahedron( const idBounds& bounds )
{
	idVec3 center, scale;

	center = bounds.GetCenter();
	scale = bounds[1] - center;

	verts.SetNum( 8 );
	verts[0].xyz = center + idVec3( -scale.x, -scale.y, -scale.z );
	verts[1].xyz = center + idVec3( scale.x, -scale.y, -scale.z );
	verts[2].xyz = center + idVec3( scale.x,  scale.y, -scale.z );
	verts[3].xyz = center + idVec3( -scale.x,  scale.y, -scale.z );
	verts[4].xyz = center + idVec3( -scale.x, -scale.y,  scale.z );
	verts[5].xyz = center + idVec3( scale.x, -scale.y,  scale.z );
	verts[6].xyz = center + idVec3( scale.x,  scale.y,  scale.z );
	verts[7].xyz = center + idVec3( -scale.x,  scale.y,  scale.z );

	indexes.SetNum( 12 * 3 );
	indexes[ 0 * 3 + 0] = 0;
	indexes[ 0 * 3 + 1] = 3;
	indexes[ 0 * 3 + 2] = 2;
	indexes[ 1 * 3 + 0] = 0;
	indexes[ 1 * 3 + 1] = 2;
	indexes[ 1 * 3 + 2] = 1;
	indexes[ 2 * 3 + 0] = 0;
	indexes[ 2 * 3 + 1] = 1;
	indexes[ 2 * 3 + 2] = 5;
	indexes[ 3 * 3 + 0] = 0;
	indexes[ 3 * 3 + 1] = 5;
	indexes[ 3 * 3 + 2] = 4;
	indexes[ 4 * 3 + 0] = 0;
	indexes[ 4 * 3 + 1] = 4;
	indexes[ 4 * 3 + 2] = 7;
	indexes[ 5 * 3 + 0] = 0;
	indexes[ 5 * 3 + 1] = 7;
	indexes[ 5 * 3 + 2] = 3;
	indexes[ 6 * 3 + 0] = 6;
	indexes[ 6 * 3 + 1] = 5;
	indexes[ 6 * 3 + 2] = 1;
	indexes[ 7 * 3 + 0] = 6;
	indexes[ 7 * 3 + 1] = 1;
	indexes[ 7 * 3 + 2] = 2;
	indexes[ 8 * 3 + 0] = 6;
	indexes[ 8 * 3 + 1] = 2;
	indexes[ 8 * 3 + 2] = 3;
	indexes[ 9 * 3 + 0] = 6;
	indexes[ 9 * 3 + 1] = 3;
	indexes[ 9 * 3 + 2] = 7;
	indexes[10 * 3 + 0] = 6;
	indexes[10 * 3 + 1] = 7;
	indexes[10 * 3 + 2] = 4;
	indexes[11 * 3 + 0] = 6;
	indexes[11 * 3 + 1] = 4;
	indexes[11 * 3 + 2] = 5;

	GenerateEdgeIndexes();
}

/*
====================
idSurface_Polytope::SetupOctahedron
====================
*/
void idSurface_Polytope::SetupOctahedron( const idBounds& bounds )
{
	idVec3 center, scale;

	center = bounds.GetCenter();
	scale = bounds[1] - center;

	verts.SetNum( 6 );
	verts[0].xyz = center + idVec3( scale.x, 0.0f, 0.0f );
	verts[1].xyz = center + idVec3( -scale.x, 0.0f, 0.0f );
	verts[2].xyz = center + idVec3( 0.0f,  scale.y, 0.0f );
	verts[3].xyz = center + idVec3( 0.0f, -scale.y, 0.0f );
	verts[4].xyz = center + idVec3( 0.0f, 0.0f,  scale.z );
	verts[5].xyz = center + idVec3( 0.0f, 0.0f, -scale.z );

	indexes.SetNum( 8 * 3 );
	indexes[0 * 3 + 0] = 4;
	indexes[0 * 3 + 1] = 0;
	indexes[0 * 3 + 2] = 2;
	indexes[1 * 3 + 0] = 4;
	indexes[1 * 3 + 1] = 2;
	indexes[1 * 3 + 2] = 1;
	indexes[2 * 3 + 0] = 4;
	indexes[2 * 3 + 1] = 1;
	indexes[2 * 3 + 2] = 3;
	indexes[3 * 3 + 0] = 4;
	indexes[3 * 3 + 1] = 3;
	indexes[3 * 3 + 2] = 0;
	indexes[4 * 3 + 0] = 5;
	indexes[4 * 3 + 1] = 2;
	indexes[4 * 3 + 2] = 0;
	indexes[5 * 3 + 0] = 5;
	indexes[5 * 3 + 1] = 1;
	indexes[5 * 3 + 2] = 2;
	indexes[6 * 3 + 0] = 5;
	indexes[6 * 3 + 1] = 3;
	indexes[6 * 3 + 2] = 1;
	indexes[7 * 3 + 0] = 5;
	indexes[7 * 3 + 1] = 0;
	indexes[7 * 3 + 2] = 3;

	GenerateEdgeIndexes();
}

/*
====================
idSurface_Polytope::SetupDodecahedron
====================
*/
void idSurface_Polytope::SetupDodecahedron( const idBounds& bounds )
{
}

/*
====================
idSurface_Polytope::SetupIcosahedron
====================
*/
void idSurface_Polytope::SetupIcosahedron( const idBounds& bounds )
{
}

/*
====================
idSurface_Polytope::SetupCylinder
====================
*/
void idSurface_Polytope::SetupCylinder( const idBounds& bounds, const int numSides )
{
}

/*
====================
idSurface_Polytope::SetupCone
====================
*/
void idSurface_Polytope::SetupCone( const idBounds& bounds, const int numSides )
{
}

/*
====================
idSurface_Polytope::SplitPolytope
====================
*/
int idSurface_Polytope::SplitPolytope( const idPlane& plane, const float epsilon, idSurface_Polytope** front, idSurface_Polytope** back ) const
{
	int side, i, j, s, v0, v1, v2, edgeNum;
	idSurface* surface[2];
	idSurface_Polytope* polytopeSurfaces[2], *surf;
	int* onPlaneEdges[2];

	onPlaneEdges[0] = ( int* ) _alloca( indexes.Num() / 3 * sizeof( int ) );
	onPlaneEdges[1] = ( int* ) _alloca( indexes.Num() / 3 * sizeof( int ) );

	side = Split( plane, epsilon, &surface[0], &surface[1], onPlaneEdges[0], onPlaneEdges[1] );

	*front = polytopeSurfaces[0] = new( TAG_IDLIB_SURFACE ) idSurface_Polytope;
	*back = polytopeSurfaces[1] = new( TAG_IDLIB_SURFACE ) idSurface_Polytope;

	for( s = 0; s < 2; s++ )
	{
		if( surface[s] )
		{
			polytopeSurfaces[s] = new idSurface_Polytope( *surface[s] );
			delete surface[s];
			surface[s] = NULL;
		}
	}

	*front = polytopeSurfaces[0];
	*back = polytopeSurfaces[1];

	if( side != SIDE_CROSS )
	{
		return side;
	}

	// add triangles to close off the front and back polytope
	for( s = 0; s < 2; s++ )
	{

		surf = polytopeSurfaces[s];

		edgeNum = surf->edgeIndexes[onPlaneEdges[s][0]];
		v0 = surf->edges[abs( edgeNum )].verts[INT32_SIGNBITSET( edgeNum )];
		v1 = surf->edges[abs( edgeNum )].verts[INT32_SIGNBITNOTSET( edgeNum )];

		for( i = 1; onPlaneEdges[s][i] >= 0; i++ )
		{
			for( j = i + 1; onPlaneEdges[s][j] >= 0; j++ )
			{
				edgeNum = surf->edgeIndexes[onPlaneEdges[s][j]];
				if( v1 == surf->edges[abs( edgeNum )].verts[INT32_SIGNBITSET( edgeNum )] )
				{
					v1 = surf->edges[abs( edgeNum )].verts[INT32_SIGNBITNOTSET( edgeNum )];
					SwapValues( onPlaneEdges[s][i], onPlaneEdges[s][j] );
					break;
				}
			}
		}

		for( i = 2; onPlaneEdges[s][i] >= 0; i++ )
		{
			edgeNum = surf->edgeIndexes[onPlaneEdges[s][i]];
			v1 = surf->edges[abs( edgeNum )].verts[INT32_SIGNBITNOTSET( edgeNum )];
			v2 = surf->edges[abs( edgeNum )].verts[INT32_SIGNBITSET( edgeNum )];
			surf->indexes.Append( v0 );
			surf->indexes.Append( v1 );
			surf->indexes.Append( v2 );
		}

		surf->GenerateEdgeIndexes();
	}

	return side;
}
