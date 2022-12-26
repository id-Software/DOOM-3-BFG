/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2020 Stephen Pridham (Mikkelsen tangent space support)

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

#include "RenderCommon.h"

#include "libs/mikktspace/mikktspace.h"

/*
==============================================================================

TRIANGLE MESH PROCESSING

The functions in this file have no vertex / index count limits.

Truly identical vertexes that match in position, normal, and texcoord can
be merged away.

Vertexes that match in position and texcoord, but have distinct normals will
remain distinct for all purposes.  This is usually a poor choice for models,
as adding a bevel face will not add any more vertexes, and will tend to
look better.

Match in position and normal, but differ in texcoords are referenced together
for calculating tangent vectors for bump mapping.
Artists should take care to have identical texels in all maps (bump/diffuse/specular)
in this case

Vertexes that only match in position are merged for shadow edge finding.

Degenerate triangles.

Overlapped triangles, even if normals or texcoords differ, must be removed.
for the silhoette based stencil shadow algorithm to function properly.
Is this true???
Is the overlapped triangle problem just an example of the trippled edge problem?

Interpenetrating triangles are not currently clipped to surfaces.
Do they effect the shadows?

if vertexes are intended to deform apart, make sure that no vertexes
are on top of each other in the base frame, or the sil edges may be
calculated incorrectly.

We might be able to identify this from topology.

Dangling edges are acceptable, but three way edges are not.

Are any combinations of two way edges unacceptable, like one facing
the backside of the other?

Topology is determined by a collection of triangle indexes.

The edge list can be built up from this, and stays valid even under
deformations.

Somewhat non-intuitively, concave edges cannot be optimized away, or the
stencil shadow algorithm miscounts.

Face normals are needed for generating shadow volumes and for calculating
the silhouette, but they will change with any deformation.

Vertex normals and vertex tangents will change with each deformation,
but they may be able to be transformed instead of recalculated.

bounding volume, both box and sphere will change with deformation.

silhouette indexes
shade indexes
texture indexes

  shade indexes will only be > silhouette indexes if there is facet shading present

	lookups from texture to sil and texture to shade?

The normal and tangent vector smoothing is simple averaging, no attempt is
made to better handle the cases where the distribution around the shared vertex
is highly uneven.

  we may get degenerate triangles even with the uniquing and removal
  if the vertexes have different texcoords.

==============================================================================
*/

// this shouldn't change anything, but previously renderbumped models seem to need it
#define USE_INVA

// instead of using the texture T vector, cross the normal and S vector for an orthogonal axis
#define DERIVE_UNSMOOTHED_BITANGENT

// SP Begin

// Mikktspace is a standard that should be used for new assets. If you'd like to use the original
// method of calculating tangent spaces for the original game's normal maps, disable mikktspace before
// loading in the model.
// see http://www.mikktspace.com/
//idCVar r_useMikktspace( "r_useMikktspace", "1", CVAR_RENDERER | CVAR_BOOL, "Use the mikktspace standard to derive tangents" );

static void* mkAlloc( int bytes );
static void mkFree( void* mem );
static int mkGetNumFaces( const SMikkTSpaceContext* pContext );
static int mkGetNumVerticesOfFace( const SMikkTSpaceContext* pContext, const int iFace );
static void mkGetPosition( const SMikkTSpaceContext* pContext, float fvPosOut[], const int iFace, const int iVert );
static void mkGetNormal( const SMikkTSpaceContext* pContext, float fvNormOut[], const int iFace, const int iVert );
static void mkGetTexCoord( const SMikkTSpaceContext* pContext, float fvTexcOut[], const int iFace, const int iVert );
static void mkSetTSpaceBasic( const SMikkTSpaceContext* pContext, const float fvTangent[], const float fSign, const int iFace, const int iVert );

// Helper class for loading in the interface functions for mikktspace.
class idMikkTSpaceInterface
{
public:
	idMikkTSpaceInterface();

	SMikkTSpaceInterface mkInterface;
};

static idMikkTSpaceInterface mikkTSpaceInterface;

static void SetUpMikkTSpaceContext( SMikkTSpaceContext* context );

// SP end

/*
=================
R_TriSurfMemory

For memory profiling
=================
*/
int R_TriSurfMemory( const srfTriangles_t* tri )
{
	int total = 0;

	if( tri == NULL )
	{
		return total;
	}

	if( tri->preLightShadowVertexes != NULL )
	{
		total += tri->numVerts * 2 * sizeof( tri->preLightShadowVertexes[0] );
	}
	if( tri->staticShadowVertexes != NULL )
	{
		total += tri->numVerts * 2 * sizeof( tri->staticShadowVertexes[0] );
	}
	if( tri->verts != NULL )
	{
		if( tri->ambientSurface == NULL || tri->verts != tri->ambientSurface->verts )
		{
			total += tri->numVerts * sizeof( tri->verts[0] );
		}
	}
	if( tri->indexes != NULL )
	{
		if( tri->ambientSurface == NULL || tri->indexes != tri->ambientSurface->indexes )
		{
			total += tri->numIndexes * sizeof( tri->indexes[0] );
		}
	}
	if( tri->silIndexes != NULL )
	{
		total += tri->numIndexes * sizeof( tri->silIndexes[0] );
	}
	if( tri->silEdges != NULL )
	{
		total += tri->numSilEdges * sizeof( tri->silEdges[0] );
	}
	if( tri->dominantTris != NULL )
	{
		total += tri->numVerts * sizeof( tri->dominantTris[0] );
	}
	if( tri->mirroredVerts != NULL )
	{
		total += tri->numMirroredVerts * sizeof( tri->mirroredVerts[0] );
	}
	if( tri->dupVerts != NULL )
	{
		total += tri->numDupVerts * sizeof( tri->dupVerts[0] );
	}

	total += sizeof( *tri );

	return total;
}

/*
==============
R_FreeStaticTriSurfVertexCaches
==============
*/
void R_FreeStaticTriSurfVertexCaches( srfTriangles_t* tri )
{
	// we don't support reclaiming static geometry memory
	// without a level change
	tri->ambientCache = 0;
	tri->indexCache = 0;
	tri->shadowCache = 0;
}

/*
==============
R_FreeStaticTriSurf
==============
*/
void R_FreeStaticTriSurf( srfTriangles_t* tri )
{
	if( !tri )
	{
		return;
	}

	R_FreeStaticTriSurfVertexCaches( tri );

	if( !tri->referencedVerts )
	{
		if( tri->verts != NULL )
		{
			// R_CreateLightTris points tri->verts at the verts of the ambient surface
			if( tri->ambientSurface == NULL || tri->verts != tri->ambientSurface->verts )
			{
				Mem_Free( tri->verts );
			}
		}
	}

	if( !tri->referencedIndexes )
	{
		if( tri->indexes != NULL )
		{
			// if a surface is completely inside a light volume R_CreateLightTris points tri->indexes at the indexes of the ambient surface
			if( tri->ambientSurface == NULL || tri->indexes != tri->ambientSurface->indexes )
			{
				Mem_Free( tri->indexes );
			}
		}
		if( tri->silIndexes != NULL )
		{
			Mem_Free( tri->silIndexes );
		}
		if( tri->silEdges != NULL )
		{
			Mem_Free( tri->silEdges );
		}
		if( tri->dominantTris != NULL )
		{
			Mem_Free( tri->dominantTris );
		}
		if( tri->mirroredVerts != NULL )
		{
			Mem_Free( tri->mirroredVerts );
		}
		if( tri->dupVerts != NULL )
		{
			Mem_Free( tri->dupVerts );
		}
	}

	if( tri->preLightShadowVertexes != NULL )
	{
		Mem_Free( tri->preLightShadowVertexes );
	}
	if( tri->staticShadowVertexes != NULL )
	{
		Mem_Free( tri->staticShadowVertexes );
	}

	// clear the tri out so we don't retain stale data
	memset( tri, 0, sizeof( srfTriangles_t ) );

	Mem_Free( tri );
}

/*
==============
R_FreeStaticTriSurfVerts
==============
*/
void R_FreeStaticTriSurfVerts( srfTriangles_t* tri )
{
	// we don't support reclaiming static geometry memory
	// without a level change
	tri->ambientCache = 0;

	if( tri->verts != NULL )
	{
		// R_CreateLightTris points tri->verts at the verts of the ambient surface
		if( tri->ambientSurface == NULL || tri->verts != tri->ambientSurface->verts )
		{
			Mem_Free( tri->verts );
		}
	}
}

/*
==============
R_AllocStaticTriSurf
==============
*/
srfTriangles_t* R_AllocStaticTriSurf()
{
	srfTriangles_t* tris = ( srfTriangles_t* )Mem_ClearedAlloc( sizeof( srfTriangles_t ), TAG_SRFTRIS );
	return tris;
}

/*
=================
R_CopyStaticTriSurf

This only duplicates the indexes and verts, not any of the derived data.
=================
*/
srfTriangles_t* R_CopyStaticTriSurf( const srfTriangles_t* tri )
{
	srfTriangles_t*	newTri;

	newTri = R_AllocStaticTriSurf();
	R_AllocStaticTriSurfVerts( newTri, tri->numVerts );
	R_AllocStaticTriSurfIndexes( newTri, tri->numIndexes );
	newTri->numVerts = tri->numVerts;
	newTri->numIndexes = tri->numIndexes;
	memcpy( newTri->verts, tri->verts, tri->numVerts * sizeof( newTri->verts[0] ) );
	memcpy( newTri->indexes, tri->indexes, tri->numIndexes * sizeof( newTri->indexes[0] ) );

	return newTri;
}

/*
=================
R_AllocStaticTriSurfVerts
=================
*/
void R_AllocStaticTriSurfVerts( srfTriangles_t* tri, int numVerts )
{
	assert( tri->verts == NULL );
	tri->verts = ( idDrawVert* )Mem_Alloc16( numVerts * sizeof( idDrawVert ), TAG_TRI_VERTS );
}

/*
=================
R_AllocStaticTriSurfIndexes
=================
*/
void R_AllocStaticTriSurfIndexes( srfTriangles_t* tri, int numIndexes )
{
	assert( tri->indexes == NULL );
	tri->indexes = ( triIndex_t* )Mem_Alloc16( numIndexes * sizeof( triIndex_t ), TAG_TRI_INDEXES );
}

/*
=================
R_AllocStaticTriSurfSilIndexes
=================
*/
void R_AllocStaticTriSurfSilIndexes( srfTriangles_t* tri, int numIndexes )
{
	assert( tri->silIndexes == NULL );
	tri->silIndexes = ( triIndex_t* )Mem_Alloc16( numIndexes * sizeof( triIndex_t ), TAG_TRI_SIL_INDEXES );
}

/*
=================
R_AllocStaticTriSurfDominantTris
=================
*/
void R_AllocStaticTriSurfDominantTris( srfTriangles_t* tri, int numVerts )
{
	assert( tri->dominantTris == NULL );
	tri->dominantTris = ( dominantTri_t* )Mem_Alloc16( numVerts * sizeof( dominantTri_t ), TAG_TRI_DOMINANT_TRIS );
}

/*
=================
R_AllocStaticTriSurfMirroredVerts
=================
*/
void R_AllocStaticTriSurfMirroredVerts( srfTriangles_t* tri, int numMirroredVerts )
{
	assert( tri->mirroredVerts == NULL );
	tri->mirroredVerts = ( int* )Mem_Alloc16( numMirroredVerts * sizeof( *tri->mirroredVerts ), TAG_TRI_MIR_VERT );
}

/*
=================
R_AllocStaticTriSurfDupVerts
=================
*/
void R_AllocStaticTriSurfDupVerts( srfTriangles_t* tri, int numDupVerts )
{
	assert( tri->dupVerts == NULL );
	tri->dupVerts = ( int* )Mem_Alloc16( numDupVerts * 2 * sizeof( *tri->dupVerts ), TAG_TRI_DUP_VERT );
}

/*
=================
R_AllocStaticTriSurfSilEdges
=================
*/
void R_AllocStaticTriSurfSilEdges( srfTriangles_t* tri, int numSilEdges )
{
	assert( tri->silEdges == NULL );
	tri->silEdges = ( silEdge_t* )Mem_Alloc16( numSilEdges * sizeof( silEdge_t ), TAG_TRI_SIL_EDGE );
}

/*
=================
R_AllocStaticTriSurfPreLightShadowVerts
=================
*/
void R_AllocStaticTriSurfPreLightShadowVerts( srfTriangles_t* tri, int numVerts )
{
	assert( tri->preLightShadowVertexes == NULL );
	tri->preLightShadowVertexes = ( idShadowVert* )Mem_Alloc16( numVerts * sizeof( idShadowVert ), TAG_TRI_SHADOW );
}

/*
=================
R_ResizeStaticTriSurfVerts
=================
*/
void R_ResizeStaticTriSurfVerts( srfTriangles_t* tri, int numVerts )
{
	idDrawVert* newVerts = ( idDrawVert* )Mem_Alloc16( numVerts * sizeof( idDrawVert ), TAG_TRI_VERTS );
	const int copy = Min( numVerts, tri->numVerts );
	memcpy( newVerts, tri->verts, copy * sizeof( idDrawVert ) );
	Mem_Free( tri->verts );
	tri->verts = newVerts;
}

/*
=================
R_ResizeStaticTriSurfIndexes
=================
*/
void R_ResizeStaticTriSurfIndexes( srfTriangles_t* tri, int numIndexes )
{
	triIndex_t* newIndexes = ( triIndex_t* )Mem_Alloc16( numIndexes * sizeof( triIndex_t ), TAG_TRI_INDEXES );
	const int copy = std::min( numIndexes, tri->numIndexes );
	memcpy( newIndexes, tri->indexes, copy * sizeof( triIndex_t ) );
	Mem_Free( tri->indexes );
	tri->indexes = newIndexes;
}

/*
=================
R_ReferenceStaticTriSurfVerts
=================
*/
void R_ReferenceStaticTriSurfVerts( srfTriangles_t* tri, const srfTriangles_t* reference )
{
	tri->verts = reference->verts;
}

/*
=================
R_ReferenceStaticTriSurfIndexes
=================
*/
void R_ReferenceStaticTriSurfIndexes( srfTriangles_t* tri, const srfTriangles_t* reference )
{
	tri->indexes = reference->indexes;
}

/*
=================
R_FreeStaticTriSurfSilIndexes
=================
*/
void R_FreeStaticTriSurfSilIndexes( srfTriangles_t* tri )
{
	Mem_Free( tri->silIndexes );
	tri->silIndexes = NULL;
}

/*
===============
R_RangeCheckIndexes

Check for syntactically incorrect indexes, like out of range values.
Does not check for semantics, like degenerate triangles.

No vertexes is acceptable if no indexes.
No indexes is acceptable.
More vertexes than are referenced by indexes are acceptable.
===============
*/
void R_RangeCheckIndexes( const srfTriangles_t* tri )
{
	int		i;

	if( tri->numIndexes < 0 )
	{
		common->Error( "R_RangeCheckIndexes: numIndexes < 0" );
	}
	if( tri->numVerts < 0 )
	{
		common->Error( "R_RangeCheckIndexes: numVerts < 0" );
	}

	// must specify an integral number of triangles
	if( tri->numIndexes % 3 != 0 )
	{
		common->Error( "R_RangeCheckIndexes: numIndexes %% 3" );
	}

	for( i = 0; i < tri->numIndexes; i++ )
	{
		if( tri->indexes[i] >= tri->numVerts )
		{
			common->Error( "R_RangeCheckIndexes: index out of range" );
		}
	}

	// this should not be possible unless there are unused verts
	if( tri->numVerts > tri->numIndexes )
	{
		// FIXME: find the causes of these
		// common->Printf( "R_RangeCheckIndexes: tri->numVerts > tri->numIndexes\n" );
	}
}

/*
=================
R_BoundTriSurf
=================
*/
void R_BoundTriSurf( srfTriangles_t* tri )
{
	SIMDProcessor->MinMax( tri->bounds[0], tri->bounds[1], tri->verts, tri->numVerts );
}

/*
=================
R_CreateSilRemap
=================
*/
static int* R_CreateSilRemap( const srfTriangles_t* tri )
{
	int		c_removed, c_unique;
	int*		remap;
	int		i, j, hashKey;
	const idDrawVert* v1, *v2;

	remap = ( int* )R_ClearedStaticAlloc( tri->numVerts * sizeof( remap[0] ) );

	if( !r_useSilRemap.GetBool() )
	{
		for( i = 0; i < tri->numVerts; i++ )
		{
			remap[i] = i;
		}
		return remap;
	}

	idHashIndex		hash( 1024, tri->numVerts );

	c_removed = 0;
	c_unique = 0;
	for( i = 0; i < tri->numVerts; i++ )
	{
		v1 = &tri->verts[i];

		// see if there is an earlier vert that it can map to
		hashKey = hash.GenerateKey( v1->xyz );
		for( j = hash.First( hashKey ); j >= 0; j = hash.Next( j ) )
		{
			v2 = &tri->verts[j];
			if( v2->xyz[0] == v1->xyz[0]
					&& v2->xyz[1] == v1->xyz[1]
					&& v2->xyz[2] == v1->xyz[2] )
			{
				c_removed++;
				remap[i] = j;
				break;
			}
		}
		if( j < 0 )
		{
			c_unique++;
			remap[i] = i;
			hash.Add( hashKey, i );
		}
	}

	return remap;
}

/*
=================
R_CreateSilIndexes

Uniquing vertexes only on xyz before creating sil edges reduces
the edge count by about 20% on Q3 models
=================
*/
void R_CreateSilIndexes( srfTriangles_t* tri )
{
	int		i;
	int*		remap;

	if( tri->silIndexes )
	{
		Mem_Free( tri->silIndexes );
		tri->silIndexes = NULL;
	}

	remap = R_CreateSilRemap( tri );

	// remap indexes to the first one
	R_AllocStaticTriSurfSilIndexes( tri, tri->numIndexes );
	assert( tri->silIndexes != NULL );
	for( i = 0; i < tri->numIndexes; i++ )
	{
		tri->silIndexes[i] = remap[tri->indexes[i]];
	}

	R_StaticFree( remap );
}

/*
=====================
R_CreateDupVerts
=====================
*/
void R_CreateDupVerts( srfTriangles_t* tri )
{
	int i;

	idTempArray<int> remap( tri->numVerts );

	// initialize vertex remap in case there are unused verts
	for( i = 0; i < tri->numVerts; i++ )
	{
		remap[i] = i;
	}

	// set the remap based on how the silhouette indexes are remapped
	for( i = 0; i < tri->numIndexes; i++ )
	{
		remap[tri->indexes[i]] = tri->silIndexes[i];
	}

	// create duplicate vertex index based on the vertex remap
	idTempArray<int> tempDupVerts( tri->numVerts * 2 );
	tri->numDupVerts = 0;
	for( i = 0; i < tri->numVerts; i++ )
	{
		if( remap[i] != i )
		{
			tempDupVerts[tri->numDupVerts * 2 + 0] = i;
			tempDupVerts[tri->numDupVerts * 2 + 1] = remap[i];
			tri->numDupVerts++;
		}
	}

	R_AllocStaticTriSurfDupVerts( tri, tri->numDupVerts );
	memcpy( tri->dupVerts, tempDupVerts.Ptr(), tri->numDupVerts * 2 * sizeof( tri->dupVerts[0] ) );
}

/*
===============
R_DefineEdge
===============
*/
static int c_duplicatedEdges, c_tripledEdges;
static const int MAX_SIL_EDGES			= 0x7ffff;

static void R_DefineEdge( const int v1, const int v2, const int planeNum, const int numPlanes,
						  idList<silEdge_t>& silEdges, idHashIndex&	 silEdgeHash )
{
	int		i, hashKey;

	// check for degenerate edge
	if( v1 == v2 )
	{
		return;
	}
	hashKey = silEdgeHash.GenerateKey( v1, v2 );
	// search for a matching other side
	for( i = silEdgeHash.First( hashKey ); i >= 0 && i < MAX_SIL_EDGES; i = silEdgeHash.Next( i ) )
	{
		if( silEdges[i].v1 == v1 && silEdges[i].v2 == v2 )
		{
			c_duplicatedEdges++;
			// allow it to still create a new edge
			continue;
		}
		if( silEdges[i].v2 == v1 && silEdges[i].v1 == v2 )
		{
			if( silEdges[i].p2 != numPlanes )
			{
				c_tripledEdges++;
				// allow it to still create a new edge
				continue;
			}
			// this is a matching back side
			silEdges[i].p2 = planeNum;
			return;
		}

	}

	// define the new edge
	silEdgeHash.Add( hashKey, silEdges.Num() );

	silEdge_t silEdge;

	silEdge.p1 = planeNum;
	silEdge.p2 = numPlanes;
	silEdge.v1 = v1;
	silEdge.v2 = v2;

	silEdges.Append( silEdge );
}

/*
=================
SilEdgeSort
=================
*/
static int SilEdgeSort( const void* a, const void* b )
{
	if( ( ( silEdge_t* )a )->p1 < ( ( silEdge_t* )b )->p1 )
	{
		return -1;
	}
	if( ( ( silEdge_t* )a )->p1 > ( ( silEdge_t* )b )->p1 )
	{
		return 1;
	}
	if( ( ( silEdge_t* )a )->p2 < ( ( silEdge_t* )b )->p2 )
	{
		return -1;
	}
	if( ( ( silEdge_t* )a )->p2 > ( ( silEdge_t* )b )->p2 )
	{
		return 1;
	}
	return 0;
}

/*
=================
R_IdentifySilEdges

If the surface will not deform, coplanar edges (polygon interiors)
can never create silhouette plains, and can be omited
=================
*/
int	c_coplanarSilEdges;
int	c_totalSilEdges;

void R_IdentifySilEdges( srfTriangles_t* tri, bool omitCoplanarEdges )
{
	int		i;
	int		shared, single;

	omitCoplanarEdges = false;	// optimization doesn't work for some reason

	static const int SILEDGE_HASH_SIZE		= 1024;

	const int numTris = tri->numIndexes / 3;

	idList<silEdge_t>	silEdges( MAX_SIL_EDGES );
	idHashIndex	silEdgeHash( SILEDGE_HASH_SIZE, MAX_SIL_EDGES );
	int			numPlanes = numTris;


	silEdgeHash.Clear();

	c_duplicatedEdges = 0;
	c_tripledEdges = 0;

	for( i = 0; i < numTris; i++ )
	{
		int		i1, i2, i3;

		i1 = tri->silIndexes[ i * 3 + 0 ];
		i2 = tri->silIndexes[ i * 3 + 1 ];
		i3 = tri->silIndexes[ i * 3 + 2 ];

		// create the edges
		R_DefineEdge( i1, i2, i, numPlanes, silEdges, silEdgeHash );
		R_DefineEdge( i2, i3, i, numPlanes, silEdges, silEdgeHash );
		R_DefineEdge( i3, i1, i, numPlanes, silEdges, silEdgeHash );
	}

	if( c_duplicatedEdges || c_tripledEdges )
	{
		common->DWarning( "%i duplicated edge directions, %i tripled edges", c_duplicatedEdges, c_tripledEdges );
	}

	// if we know that the vertexes aren't going
	// to deform, we can remove interior triangulation edges
	// on otherwise planar polygons.
	// I earlier believed that I could also remove concave
	// edges, because they are never silhouettes in the conventional sense,
	// but they are still needed to balance out all the true sil edges
	// for the shadow algorithm to function
	int		c_coplanarCulled;

	c_coplanarCulled = 0;
	if( omitCoplanarEdges )
	{
		for( i = 0; i < silEdges.Num(); i++ )
		{
			int			i1, i2, i3;
			idPlane		plane;
			int			base;
			int			j;
			float		d;

			if( silEdges[i].p2 == numPlanes )  	// the fake dangling edge
			{
				continue;
			}

			base = silEdges[i].p1 * 3;
			i1 = tri->silIndexes[ base + 0 ];
			i2 = tri->silIndexes[ base + 1 ];
			i3 = tri->silIndexes[ base + 2 ];

			plane.FromPoints( tri->verts[i1].xyz, tri->verts[i2].xyz, tri->verts[i3].xyz );

			// check to see if points of second triangle are not coplanar
			base = silEdges[i].p2 * 3;
			for( j = 0; j < 3; j++ )
			{
				i1 = tri->silIndexes[ base + j ];
				d = plane.Distance( tri->verts[i1].xyz );
				if( d != 0 )  		// even a small epsilon causes problems
				{
					break;
				}
			}

			if( j == 3 )
			{
				// we can cull this sil edge
				memmove( &silEdges[i], &silEdges[i + 1], ( silEdges.Num() - i - 1 ) * sizeof( silEdges[i] ) );
				c_coplanarCulled++;
				silEdges.SetNum( silEdges.Num() - 1 );
				i--;
			}
		}
		if( c_coplanarCulled )
		{
			c_coplanarSilEdges += c_coplanarCulled;
//			common->Printf( "%i of %i sil edges coplanar culled\n", c_coplanarCulled,
//				c_coplanarCulled + numSilEdges );
		}
	}
	c_totalSilEdges += silEdges.Num();

	// sort the sil edges based on plane number
	qsort( silEdges.Ptr(), silEdges.Num(), sizeof( silEdges[0] ), SilEdgeSort );

	// count up the distribution.
	// a perfectly built model should only have shared
	// edges, but most models will have some interpenetration
	// and dangling edges
	shared = 0;
	single = 0;
	for( i = 0; i < silEdges.Num(); i++ )
	{
		if( silEdges[i].p2 == numPlanes )
		{
			single++;
		}
		else
		{
			shared++;
		}
	}

	if( !single )
	{
		tri->perfectHull = true;
	}
	else
	{
		tri->perfectHull = false;
	}

	tri->numSilEdges = silEdges.Num();
	R_AllocStaticTriSurfSilEdges( tri, silEdges.Num() );
	memcpy( tri->silEdges, silEdges.Ptr(), silEdges.Num() * sizeof( tri->silEdges[0] ) );
}

/*
===============
R_FaceNegativePolarity

Returns true if the texture polarity of the face is negative, false if it is positive or zero
===============
*/
static bool R_FaceNegativePolarity( const srfTriangles_t* tri, int firstIndex )
{
	const idDrawVert* a = tri->verts + tri->indexes[firstIndex + 0];
	const idDrawVert* b = tri->verts + tri->indexes[firstIndex + 1];
	const idDrawVert* c = tri->verts + tri->indexes[firstIndex + 2];

	const idVec2 aST = a->GetTexCoord();
	const idVec2 bST = b->GetTexCoord();
	const idVec2 cST = c->GetTexCoord();

	float d0[5];
	d0[3] = bST[0] - aST[0];
	d0[4] = bST[1] - aST[1];

	float d1[5];
	d1[3] = cST[0] - aST[0];
	d1[4] = cST[1] - aST[1];

	const float area = d0[3] * d1[4] - d0[4] * d1[3];
	if( area >= 0 )
	{
		return false;
	}
	return true;
}

/*
===================
R_DuplicateMirroredVertexes

Modifies the surface to bust apart any verts that are shared by both positive and
negative texture polarities, so tangent space smoothing at the vertex doesn't
degenerate.

This will create some identical vertexes (which will eventually get different tangent
vectors), so never optimize the resulting mesh, or it will get the mirrored edges back.

Reallocates tri->verts and changes tri->indexes in place
Silindexes are unchanged by this.

sets mirroredVerts and mirroredVerts[]
===================
*/
struct tangentVert_t
{
	bool	polarityUsed[2];
	int		negativeRemap;
};

static void	R_DuplicateMirroredVertexes( srfTriangles_t* tri )
{
	tangentVert_t*	vert;
	int				i, j;
	int				totalVerts;
	int				numMirror;

	idTempArray<tangentVert_t> tverts( tri->numVerts );
	tverts.Zero();

	// determine texture polarity of each surface

	// mark each vert with the polarities it uses
	for( i = 0; i < tri->numIndexes; i += 3 )
	{
		int	polarity = R_FaceNegativePolarity( tri, i );
		for( j = 0; j < 3; j++ )
		{
			tverts[tri->indexes[i + j]].polarityUsed[ polarity ] = true;
		}
	}

	// now create new vertex indices as needed
	totalVerts = tri->numVerts;
	for( i = 0; i < tri->numVerts; i++ )
	{
		vert = &tverts[i];
		if( vert->polarityUsed[0] && vert->polarityUsed[1] )
		{
			vert->negativeRemap = totalVerts;
			totalVerts++;
		}
	}

	tri->numMirroredVerts = totalVerts - tri->numVerts;

	if( tri->numMirroredVerts == 0 )
	{
		tri->mirroredVerts = NULL;
		return;
	}

	// now create the new list
	R_AllocStaticTriSurfMirroredVerts( tri, tri->numMirroredVerts );
	R_ResizeStaticTriSurfVerts( tri, totalVerts );

	// create the duplicates
	numMirror = 0;
	for( i = 0; i < tri->numVerts; i++ )
	{
		j = tverts[i].negativeRemap;
		if( j )
		{
			tri->verts[j] = tri->verts[i];
			tri->mirroredVerts[numMirror] = i;
			numMirror++;
		}
	}
	tri->numVerts = totalVerts;

	// change the indexes
	for( i = 0; i < tri->numIndexes; i++ )
	{
		if( tverts[tri->indexes[i]].negativeRemap && R_FaceNegativePolarity( tri, 3 * ( i / 3 ) ) )
		{
			tri->indexes[i] = tverts[tri->indexes[i]].negativeRemap;
		}
	}
}

/*
============
R_DeriveMikktspaceTangents

Derives the tangent space for the given triangles using the Mikktspace standard.
Normals must be calculated beforehand.
============
*/
static bool R_DeriveMikktspaceTangents( srfTriangles_t* tri )
{
	SMikkTSpaceContext context;
	SetUpMikkTSpaceContext( &context );
	context.m_pUserData = tri;

	return ( genTangSpaceDefault( &context ) != 0 );
}

/*
============
R_DeriveNormalsAndTangents

Derives the normal and orthogonal tangent vectors for the triangle vertices.
For each vertex the normal and tangent vectors are derived from all triangles
using the vertex which results in smooth tangents across the mesh.
============
*/
void R_DeriveNormalsAndTangents( srfTriangles_t* tri )
{
	idTempArray< idVec3 > vertexNormals( tri->numVerts );
	idTempArray< idVec3 > vertexTangents( tri->numVerts );
	idTempArray< idVec3 > vertexBitangents( tri->numVerts );

	vertexNormals.Zero();
	vertexTangents.Zero();
	vertexBitangents.Zero();

	for( int i = 0; i < tri->numIndexes; i += 3 )
	{
		const int v0 = tri->indexes[i + 0];
		const int v1 = tri->indexes[i + 1];
		const int v2 = tri->indexes[i + 2];

		const idDrawVert* a = tri->verts + v0;
		const idDrawVert* b = tri->verts + v1;
		const idDrawVert* c = tri->verts + v2;

		const idVec2 aST = a->GetTexCoord();
		const idVec2 bST = b->GetTexCoord();
		const idVec2 cST = c->GetTexCoord();

		float d0[5];
		d0[0] = b->xyz[0] - a->xyz[0];
		d0[1] = b->xyz[1] - a->xyz[1];
		d0[2] = b->xyz[2] - a->xyz[2];
		d0[3] = bST[0] - aST[0];
		d0[4] = bST[1] - aST[1];

		float d1[5];
		d1[0] = c->xyz[0] - a->xyz[0];
		d1[1] = c->xyz[1] - a->xyz[1];
		d1[2] = c->xyz[2] - a->xyz[2];
		d1[3] = cST[0] - aST[0];
		d1[4] = cST[1] - aST[1];

		idVec3 normal;
		normal[0] = d1[1] * d0[2] - d1[2] * d0[1];
		normal[1] = d1[2] * d0[0] - d1[0] * d0[2];
		normal[2] = d1[0] * d0[1] - d1[1] * d0[0];

		const float f0 = idMath::InvSqrt( normal.x * normal.x + normal.y * normal.y + normal.z * normal.z );

		normal.x *= f0;
		normal.y *= f0;
		normal.z *= f0;

		// area sign bit
		const float area = d0[3] * d1[4] - d0[4] * d1[3];
		unsigned int signBit = ( *( unsigned int* )&area ) & ( 1 << 31 );

		idVec3 tangent;
		tangent[0] = d0[0] * d1[4] - d0[4] * d1[0];
		tangent[1] = d0[1] * d1[4] - d0[4] * d1[1];
		tangent[2] = d0[2] * d1[4] - d0[4] * d1[2];

		const float f1 = idMath::InvSqrt( tangent.x * tangent.x + tangent.y * tangent.y + tangent.z * tangent.z );
		*( unsigned int* )&f1 ^= signBit;

		tangent.x *= f1;
		tangent.y *= f1;
		tangent.z *= f1;

		idVec3 bitangent;
		bitangent[0] = d0[3] * d1[0] - d0[0] * d1[3];
		bitangent[1] = d0[3] * d1[1] - d0[1] * d1[3];
		bitangent[2] = d0[3] * d1[2] - d0[2] * d1[3];

		const float f2 = idMath::InvSqrt( bitangent.x * bitangent.x + bitangent.y * bitangent.y + bitangent.z * bitangent.z );
		*( unsigned int* )&f2 ^= signBit;

		bitangent.x *= f2;
		bitangent.y *= f2;
		bitangent.z *= f2;

		vertexNormals[v0] += normal;
		vertexTangents[v0] += tangent;
		vertexBitangents[v0] += bitangent;

		vertexNormals[v1] += normal;
		vertexTangents[v1] += tangent;
		vertexBitangents[v1] += bitangent;

		vertexNormals[v2] += normal;
		vertexTangents[v2] += tangent;
		vertexBitangents[v2] += bitangent;
	}

	// add the normal of a duplicated vertex to the normal of the first vertex with the same XYZ
	for( int i = 0; i < tri->numDupVerts; i++ )
	{
		vertexNormals[tri->dupVerts[i * 2 + 0]] += vertexNormals[tri->dupVerts[i * 2 + 1]];
	}

	// copy vertex normals to duplicated vertices
	for( int i = 0; i < tri->numDupVerts; i++ )
	{
		vertexNormals[tri->dupVerts[i * 2 + 1]] = vertexNormals[tri->dupVerts[i * 2 + 0]];
	}

	// Project the summed vectors onto the normal plane and normalize.
	// The tangent vectors will not necessarily be orthogonal to each
	// other, but they will be orthogonal to the surface normal.
	for( int i = 0; i < tri->numVerts; i++ )
	{
		const float normalScale = idMath::InvSqrt( vertexNormals[i].x * vertexNormals[i].x + vertexNormals[i].y * vertexNormals[i].y + vertexNormals[i].z * vertexNormals[i].z );
		vertexNormals[i].x *= normalScale;
		vertexNormals[i].y *= normalScale;
		vertexNormals[i].z *= normalScale;

		vertexTangents[i] -= ( vertexTangents[i] * vertexNormals[i] ) * vertexNormals[i];
		vertexBitangents[i] -= ( vertexBitangents[i] * vertexNormals[i] ) * vertexNormals[i];

		const float tangentScale = idMath::InvSqrt( vertexTangents[i].x * vertexTangents[i].x + vertexTangents[i].y * vertexTangents[i].y + vertexTangents[i].z * vertexTangents[i].z );
		vertexTangents[i].x *= tangentScale;
		vertexTangents[i].y *= tangentScale;
		vertexTangents[i].z *= tangentScale;

		const float bitangentScale = idMath::InvSqrt( vertexBitangents[i].x * vertexBitangents[i].x + vertexBitangents[i].y * vertexBitangents[i].y + vertexBitangents[i].z * vertexBitangents[i].z );
		vertexBitangents[i].x *= bitangentScale;
		vertexBitangents[i].y *= bitangentScale;
		vertexBitangents[i].z *= bitangentScale;
	}

	// compress the normals and tangents
	for( int i = 0; i < tri->numVerts; i++ )
	{
		tri->verts[i].SetNormal( vertexNormals[i] );
		tri->verts[i].SetTangent( vertexTangents[i] );
		tri->verts[i].SetBiTangent( vertexBitangents[i] );
	}
}

/*
============
R_DeriveUnsmoothedNormalsAndTangents
============
*/
void R_DeriveUnsmoothedNormalsAndTangents( srfTriangles_t* tri )
{
	for( int i = 0; i < tri->numVerts; i++ )
	{
		float d0, d1, d2, d3, d4;
		float d5, d6, d7, d8, d9;
		float s0, s1, s2;
		float n0, n1, n2;
		float t0, t1, t2;
		float t3, t4, t5;

		const dominantTri_t& dt = tri->dominantTris[i];

		idDrawVert* a = tri->verts + i;
		idDrawVert* b = tri->verts + dt.v2;
		idDrawVert* c = tri->verts + dt.v3;

		const idVec2 aST = a->GetTexCoord();
		const idVec2 bST = b->GetTexCoord();
		const idVec2 cST = c->GetTexCoord();

		d0 = b->xyz[0] - a->xyz[0];
		d1 = b->xyz[1] - a->xyz[1];
		d2 = b->xyz[2] - a->xyz[2];
		d3 = bST[0] - aST[0];
		d4 = bST[1] - aST[1];

		d5 = c->xyz[0] - a->xyz[0];
		d6 = c->xyz[1] - a->xyz[1];
		d7 = c->xyz[2] - a->xyz[2];
		d8 = cST[0] - aST[0];
		d9 = cST[1] - aST[1];

		s0 = dt.normalizationScale[0];
		s1 = dt.normalizationScale[1];
		s2 = dt.normalizationScale[2];

		n0 = s2 * ( d6 * d2 - d7 * d1 );
		n1 = s2 * ( d7 * d0 - d5 * d2 );
		n2 = s2 * ( d5 * d1 - d6 * d0 );

		t0 = s0 * ( d0 * d9 - d4 * d5 );
		t1 = s0 * ( d1 * d9 - d4 * d6 );
		t2 = s0 * ( d2 * d9 - d4 * d7 );

#ifndef DERIVE_UNSMOOTHED_BITANGENT
		t3 = s1 * ( d3 * d5 - d0 * d8 );
		t4 = s1 * ( d3 * d6 - d1 * d8 );
		t5 = s1 * ( d3 * d7 - d2 * d8 );
#else
		t3 = s1 * ( n2 * t1 - n1 * t2 );
		t4 = s1 * ( n0 * t2 - n2 * t0 );
		t5 = s1 * ( n1 * t0 - n0 * t1 );
#endif

		a->SetNormal( n0, n1, n2 );
		a->SetTangent( t0, t1, t2 );
		a->SetBiTangent( t3, t4, t5 );
	}
}

/*
=====================
R_CreateVertexNormals

Averages together the contributions of all faces that are
used by a vertex, creating drawVert->normal
=====================
*/
void R_CreateVertexNormals( srfTriangles_t* tri )
{
	if( tri->silIndexes == NULL )
	{
		R_CreateSilIndexes( tri );
	}

	idTempArray< idVec3 > vertexNormals( tri->numVerts );
	vertexNormals.Zero();

	assert( tri->silIndexes != NULL );
	for( int i = 0; i < tri->numIndexes; i += 3 )
	{
		const int i0 = tri->silIndexes[i + 0];
		const int i1 = tri->silIndexes[i + 1];
		const int i2 = tri->silIndexes[i + 2];

		const idDrawVert& v0 = tri->verts[i0];
		const idDrawVert& v1 = tri->verts[i1];
		const idDrawVert& v2 = tri->verts[i2];

		const idPlane plane( v0.xyz, v1.xyz, v2.xyz );

		vertexNormals[i0] += plane.Normal();
		vertexNormals[i1] += plane.Normal();
		vertexNormals[i2] += plane.Normal();
	}

	// replicate from silIndexes to all indexes
	for( int i = 0; i < tri->numIndexes; i++ )
	{
		vertexNormals[tri->indexes[i]] = vertexNormals[tri->silIndexes[i]];
	}

	// normalize
	for( int i = 0; i < tri->numVerts; i++ )
	{
		vertexNormals[i].Normalize();
	}

	// compress the normals
	for( int i = 0; i < tri->numVerts; i++ )
	{
		tri->verts[i].SetNormal( vertexNormals[i] );
	}
}

/*
=================
R_DeriveTangentsWithoutNormals

Build texture space tangents for bump mapping
If a surface is deformed, this must be recalculated

This assumes that any mirrored vertexes have already been duplicated, so
any shared vertexes will have the tangent spaces smoothed across.

Texture wrapping slightly complicates this, but as long as the normals
are shared, and the tangent vectors are projected onto the normals, the
separate vertexes should wind up with identical tangent spaces.

mirroring a normalmap WILL cause a slightly visible seam unless the normals
are completely flat around the edge's full bilerp support.

Vertexes which are smooth shaded must have their tangent vectors
in the same plane, which will allow a seamless
rendering as long as the normal map is even on both sides of the
seam.

A smooth shaded surface may have multiple tangent vectors at a vertex
due to texture seams or mirroring, but it should only have a single
normal vector.

Each triangle has a pair of tangent vectors in it's plane

Should we consider having vertexes point at shared tangent spaces
to save space or speed transforms?

this version only handles bilateral symetry
=================
*/
void R_DeriveTangentsWithoutNormals( srfTriangles_t* tri, bool useMikktspace )
{
	// SP begin
	if( useMikktspace )
	{
		if( !R_DeriveMikktspaceTangents( tri ) )
		{
			idLib::Warning( "Mikkelsen tangent space calculation failed" );
		}
		else
		{
			tri->tangentsCalculated = true;
			return;
		}
	}
	// SP End

	idTempArray< idVec3 > triangleTangents( tri->numIndexes / 3 );
	idTempArray< idVec3 > triangleBitangents( tri->numIndexes / 3 );

	//
	// calculate tangent vectors for each face in isolation
	//
	int c_positive = 0;
	int c_negative = 0;
	int c_textureDegenerateFaces = 0;
	for( int i = 0; i < tri->numIndexes; i += 3 )
	{
		idVec3	temp;

		idDrawVert* a = tri->verts + tri->indexes[i + 0];
		idDrawVert* b = tri->verts + tri->indexes[i + 1];
		idDrawVert* c = tri->verts + tri->indexes[i + 2];

		const idVec2 aST = a->GetTexCoord();
		const idVec2 bST = b->GetTexCoord();
		const idVec2 cST = c->GetTexCoord();

		float d0[5];
		d0[0] = b->xyz[0] - a->xyz[0];
		d0[1] = b->xyz[1] - a->xyz[1];
		d0[2] = b->xyz[2] - a->xyz[2];
		d0[3] = bST[0] - aST[0];
		d0[4] = bST[1] - aST[1];

		float d1[5];
		d1[0] = c->xyz[0] - a->xyz[0];
		d1[1] = c->xyz[1] - a->xyz[1];
		d1[2] = c->xyz[2] - a->xyz[2];
		d1[3] = cST[0] - aST[0];
		d1[4] = cST[1] - aST[1];

		const float area = d0[3] * d1[4] - d0[4] * d1[3];
		if( fabs( area ) < 1e-20f )
		{
			triangleTangents[i / 3].Zero();
			triangleBitangents[i / 3].Zero();
			c_textureDegenerateFaces++;
			continue;
		}
		if( area > 0.0f )
		{
			c_positive++;
		}
		else
		{
			c_negative++;
		}

#ifdef USE_INVA
		float inva = ( area < 0.0f ) ? -1.0f : 1.0f;		// was = 1.0f / area;

		temp[0] = ( d0[0] * d1[4] - d0[4] * d1[0] ) * inva;
		temp[1] = ( d0[1] * d1[4] - d0[4] * d1[1] ) * inva;
		temp[2] = ( d0[2] * d1[4] - d0[4] * d1[2] ) * inva;
		temp.Normalize();
		triangleTangents[i / 3] = temp;

		temp[0] = ( d0[3] * d1[0] - d0[0] * d1[3] ) * inva;
		temp[1] = ( d0[3] * d1[1] - d0[1] * d1[3] ) * inva;
		temp[2] = ( d0[3] * d1[2] - d0[2] * d1[3] ) * inva;
		temp.Normalize();
		triangleBitangents[i / 3] = temp;
#else
		temp[0] = ( d0[0] * d1[4] - d0[4] * d1[0] );
		temp[1] = ( d0[1] * d1[4] - d0[4] * d1[1] );
		temp[2] = ( d0[2] * d1[4] - d0[4] * d1[2] );
		temp.Normalize();
		triangleTangents[i / 3] = temp;

		temp[0] = ( d0[3] * d1[0] - d0[0] * d1[3] );
		temp[1] = ( d0[3] * d1[1] - d0[1] * d1[3] );
		temp[2] = ( d0[3] * d1[2] - d0[2] * d1[3] );
		temp.Normalize();
		triangleBitangents[i / 3] = temp;
#endif
	}

	idTempArray< idVec3 > vertexTangents( tri->numVerts );
	idTempArray< idVec3 > vertexBitangents( tri->numVerts );

	// clear the tangents
	for( int i = 0; i < tri->numVerts; ++i )
	{
		vertexTangents[i].Zero();
		vertexBitangents[i].Zero();
	}

	// sum up the neighbors
	for( int i = 0; i < tri->numIndexes; i += 3 )
	{
		// for each vertex on this face
		for( int j = 0; j < 3; j++ )
		{
			vertexTangents[tri->indexes[i + j]] += triangleTangents[i / 3];
			vertexBitangents[tri->indexes[i + j]] += triangleBitangents[i / 3];
		}
	}

	// Project the summed vectors onto the normal plane and normalize.
	// The tangent vectors will not necessarily be orthogonal to each
	// other, but they will be orthogonal to the surface normal.
	for( int i = 0; i < tri->numVerts; i++ )
	{
		idVec3 normal = tri->verts[i].GetNormal();
		normal.Normalize();

		vertexTangents[i] -= ( vertexTangents[i] * normal ) * normal;
		vertexTangents[i].Normalize();

		vertexBitangents[i] -= ( vertexBitangents[i] * normal ) * normal;
		vertexBitangents[i].Normalize();
	}

	for( int i = 0; i < tri->numVerts; i++ )
	{
		tri->verts[i].SetTangent( vertexTangents[i] );
		tri->verts[i].SetBiTangent( vertexBitangents[i] );
	}

	tri->tangentsCalculated = true;
}

/*
===================
R_BuildDominantTris

Find the largest triangle that uses each vertex
===================
*/
typedef struct
{
	int		vertexNum;
	int		faceNum;
} indexSort_t;

static int IndexSort( const void* a, const void* b )
{
	if( ( ( indexSort_t* )a )->vertexNum < ( ( indexSort_t* )b )->vertexNum )
	{
		return -1;
	}
	if( ( ( indexSort_t* )a )->vertexNum > ( ( indexSort_t* )b )->vertexNum )
	{
		return 1;
	}
	return 0;
}

void R_BuildDominantTris( srfTriangles_t* tri )
{
	int i, j;
	dominantTri_t* dt;
	const int numIndexes = tri->numIndexes;
	indexSort_t* ind = ( indexSort_t* )R_StaticAlloc( numIndexes * sizeof( indexSort_t ) );
	if( ind == NULL )
	{
		idLib::Error( "Couldn't allocate index sort array" );
		return;
	}

	for( i = 0; i < tri->numIndexes; i++ )
	{
		ind[i].vertexNum = tri->indexes[i];
		ind[i].faceNum = i / 3;
	}
	qsort( ind, tri->numIndexes, sizeof( *ind ), IndexSort );

	R_AllocStaticTriSurfDominantTris( tri, tri->numVerts );
	dt = tri->dominantTris;
	memset( dt, 0, tri->numVerts * sizeof( dt[0] ) );

	for( i = 0; i < numIndexes; i += j )
	{
		float	maxArea = 0;
#pragma warning( disable: 6385 ) // This is simply to get pass a false defect for /analyze -- if you can figure out a better way, please let Shawn know...
		int		vertNum = ind[i].vertexNum;
#pragma warning( default: 6385 )
		for( j = 0; i + j < tri->numIndexes && ind[i + j].vertexNum == vertNum; j++ )
		{
			float		d0[5], d1[5];
			idDrawVert*	a, *b, *c;
			idVec3		normal, tangent, bitangent;

			int	i1 = tri->indexes[ind[i + j].faceNum * 3 + 0];
			int	i2 = tri->indexes[ind[i + j].faceNum * 3 + 1];
			int	i3 = tri->indexes[ind[i + j].faceNum * 3 + 2];

			a = tri->verts + i1;
			b = tri->verts + i2;
			c = tri->verts + i3;

			const idVec2 aST = a->GetTexCoord();
			const idVec2 bST = b->GetTexCoord();
			const idVec2 cST = c->GetTexCoord();

			d0[0] = b->xyz[0] - a->xyz[0];
			d0[1] = b->xyz[1] - a->xyz[1];
			d0[2] = b->xyz[2] - a->xyz[2];
			d0[3] = bST[0] - aST[0];
			d0[4] = bST[1] - aST[1];

			d1[0] = c->xyz[0] - a->xyz[0];
			d1[1] = c->xyz[1] - a->xyz[1];
			d1[2] = c->xyz[2] - a->xyz[2];
			d1[3] = cST[0] - aST[0];
			d1[4] = cST[1] - aST[1];

			normal[0] = ( d1[1] * d0[2] - d1[2] * d0[1] );
			normal[1] = ( d1[2] * d0[0] - d1[0] * d0[2] );
			normal[2] = ( d1[0] * d0[1] - d1[1] * d0[0] );

			float area = normal.Length();

			// if this is smaller than what we already have, skip it
			if( area < maxArea )
			{
				continue;
			}
			maxArea = area;

			if( i1 == vertNum )
			{
				dt[vertNum].v2 = i2;
				dt[vertNum].v3 = i3;
			}
			else if( i2 == vertNum )
			{
				dt[vertNum].v2 = i3;
				dt[vertNum].v3 = i1;
			}
			else
			{
				dt[vertNum].v2 = i1;
				dt[vertNum].v3 = i2;
			}

			float	len = area;
			if( len < 0.001f )
			{
				len = 0.001f;
			}
			dt[vertNum].normalizationScale[2] = 1.0f / len;		// normal

			// texture area
			area = d0[3] * d1[4] - d0[4] * d1[3];

			tangent[0] = ( d0[0] * d1[4] - d0[4] * d1[0] );
			tangent[1] = ( d0[1] * d1[4] - d0[4] * d1[1] );
			tangent[2] = ( d0[2] * d1[4] - d0[4] * d1[2] );
			len = tangent.Length();
			if( len < 0.001f )
			{
				len = 0.001f;
			}
			dt[vertNum].normalizationScale[0] = ( area > 0 ? 1 : -1 ) / len;	// tangents[0]

			bitangent[0] = ( d0[3] * d1[0] - d0[0] * d1[3] );
			bitangent[1] = ( d0[3] * d1[1] - d0[1] * d1[3] );
			bitangent[2] = ( d0[3] * d1[2] - d0[2] * d1[3] );
			len = bitangent.Length();
			if( len < 0.001f )
			{
				len = 0.001f;
			}
#ifdef DERIVE_UNSMOOTHED_BITANGENT
			dt[vertNum].normalizationScale[1] = ( area > 0 ? 1 : -1 );
#else
			dt[vertNum].normalizationScale[1] = ( area > 0 ? 1 : -1 ) / len;	// tangents[1]
#endif
		}
	}

	R_StaticFree( ind );
}

/*
==================
R_DeriveTangents

This is called once for static surfaces, and every frame for deforming surfaces

Builds tangents, normals, and face planes
==================
*/
void R_DeriveTangents( srfTriangles_t* tri )
{
	if( tri->tangentsCalculated )
	{
		return;
	}

	tr.pc.c_tangentIndexes += tri->numIndexes;

	if( tri->dominantTris != NULL )
	{
		R_DeriveUnsmoothedNormalsAndTangents( tri );
	}
	else
	{
		R_DeriveNormalsAndTangents( tri );
	}
	tri->tangentsCalculated = true;
}

/*
=================
R_RemoveDuplicatedTriangles

silIndexes must have already been calculated

silIndexes are used instead of indexes, because duplicated
triangles could have different texture coordinates.
=================
*/
void R_RemoveDuplicatedTriangles( srfTriangles_t* tri )
{
	int		c_removed;
	int		i, j, r;
	int		a, b, c;

	c_removed = 0;

	// check for completely duplicated triangles
	// any rotation of the triangle is still the same, but a mirroring
	// is considered different
	for( i = 0; i < tri->numIndexes; i += 3 )
	{
		for( r = 0; r < 3; r++ )
		{
			a = tri->silIndexes[i + r];
			b = tri->silIndexes[i + ( r + 1 ) % 3];
			c = tri->silIndexes[i + ( r + 2 ) % 3];
			for( j = i + 3; j < tri->numIndexes; j += 3 )
			{
				if( tri->silIndexes[j] == a && tri->silIndexes[j + 1] == b && tri->silIndexes[j + 2] == c )
				{
					c_removed++;
					memmove( tri->indexes + j, tri->indexes + j + 3, ( tri->numIndexes - j - 3 ) * sizeof( tri->indexes[0] ) );
					memmove( tri->silIndexes + j, tri->silIndexes + j + 3, ( tri->numIndexes - j - 3 ) * sizeof( tri->silIndexes[0] ) );
					tri->numIndexes -= 3;
					j -= 3;
				}
			}
		}
	}

	if( c_removed )
	{
		common->Printf( "removed %i duplicated triangles\n", c_removed );
	}
}

/*
=================
R_RemoveDegenerateTriangles

silIndexes must have already been calculated
=================
*/
void R_RemoveDegenerateTriangles( srfTriangles_t* tri )
{
	int		c_removed;
	int		i;
	int		a, b, c;

	assert( tri->silIndexes != NULL );

	// check for completely degenerate triangles
	c_removed = 0;
	for( i = 0; i < tri->numIndexes; i += 3 )
	{
		a = tri->silIndexes[i];
		b = tri->silIndexes[i + 1];
		c = tri->silIndexes[i + 2];
		if( a == b || a == c || b == c )
		{
			c_removed++;
			memmove( tri->indexes + i, tri->indexes + i + 3, ( tri->numIndexes - i - 3 ) * sizeof( tri->indexes[0] ) );
			memmove( tri->silIndexes + i, tri->silIndexes + i + 3, ( tri->numIndexes - i - 3 ) * sizeof( tri->silIndexes[0] ) );
			tri->numIndexes -= 3;
			i -= 3;
		}
	}

	// this doesn't free the memory used by the unused verts

	if( c_removed )
	{
		common->Printf( "removed %i degenerate triangles\n", c_removed );
	}
}

/*
=================
R_TestDegenerateTextureSpace
=================
*/
void R_TestDegenerateTextureSpace( srfTriangles_t* tri )
{
	int		c_degenerate;
	int		i;

	// check for triangles with a degenerate texture space
	c_degenerate = 0;
	for( i = 0; i < tri->numIndexes; i += 3 )
	{
		const idDrawVert& a = tri->verts[tri->indexes[i + 0]];
		const idDrawVert& b = tri->verts[tri->indexes[i + 1]];
		const idDrawVert& c = tri->verts[tri->indexes[i + 2]];

		// RB: compare texcoords instead of pointers
		if( a.GetTexCoord() == b.GetTexCoord() || b.GetTexCoord() == c.GetTexCoord() || c.GetTexCoord() == a.GetTexCoord() )
		{
			c_degenerate++;
		}
		// RB end
	}

	if( c_degenerate )
	{
//		common->Printf( "%d triangles with a degenerate texture space\n", c_degenerate );
	}
}

/*
=================
R_RemoveUnusedVerts
=================
*/
void R_RemoveUnusedVerts( srfTriangles_t* tri )
{
	int		i;
	int*		mark;
	int		index;
	int		used;

	mark = ( int* )R_ClearedStaticAlloc( tri->numVerts * sizeof( *mark ) );

	for( i = 0; i < tri->numIndexes; i++ )
	{
		index = tri->indexes[i];
		if( index < 0 || index >= tri->numVerts )
		{
			common->Error( "R_RemoveUnusedVerts: bad index" );
		}
		mark[ index ] = 1;

		if( tri->silIndexes )
		{
			index = tri->silIndexes[i];
			if( index < 0 || index >= tri->numVerts )
			{
				common->Error( "R_RemoveUnusedVerts: bad index" );
			}
			mark[ index ] = 1;
		}
	}

	used = 0;
	for( i = 0; i < tri->numVerts; i++ )
	{
		if( !mark[i] )
		{
			continue;
		}
		mark[i] = used + 1;
		used++;
	}

	if( used != tri->numVerts )
	{
		for( i = 0; i < tri->numIndexes; i++ )
		{
			tri->indexes[i] = mark[ tri->indexes[i] ] - 1;
			if( tri->silIndexes )
			{
				tri->silIndexes[i] = mark[ tri->silIndexes[i] ] - 1;
			}
		}
		tri->numVerts = used;

		for( i = 0; i < tri->numVerts; i++ )
		{
			index = mark[ i ];
			if( !index )
			{
				continue;
			}
			tri->verts[ index - 1 ] = tri->verts[i];
		}

		// this doesn't realloc the arrays to save the memory used by the unused verts
	}

	R_StaticFree( mark );
}

/*
=================
R_MergeSurfaceList

Only deals with vertexes and indexes, not silhouettes, planes, etc.
Does NOT perform a cleanup triangles, so there may be duplicated verts in the result.
=================
*/
srfTriangles_t* R_MergeSurfaceList( const srfTriangles_t** surfaces, int numSurfaces )
{
	srfTriangles_t*	newTri;
	const srfTriangles_t*	tri;
	int				i, j;
	int				totalVerts;
	int				totalIndexes;

	totalVerts = 0;
	totalIndexes = 0;
	for( i = 0; i < numSurfaces; i++ )
	{
		totalVerts += surfaces[i]->numVerts;
		totalIndexes += surfaces[i]->numIndexes;
	}

	newTri = R_AllocStaticTriSurf();
	newTri->numVerts = totalVerts;
	newTri->numIndexes = totalIndexes;
	R_AllocStaticTriSurfVerts( newTri, newTri->numVerts );
	R_AllocStaticTriSurfIndexes( newTri, newTri->numIndexes );

	totalVerts = 0;
	totalIndexes = 0;
	for( i = 0; i < numSurfaces; i++ )
	{
		tri = surfaces[i];
		memcpy( newTri->verts + totalVerts, tri->verts, tri->numVerts * sizeof( *tri->verts ) );
		for( j = 0; j < tri->numIndexes; j++ )
		{
			newTri->indexes[ totalIndexes + j ] = totalVerts + tri->indexes[j];
		}
		totalVerts += tri->numVerts;
		totalIndexes += tri->numIndexes;
	}

	return newTri;
}

/*
=================
R_MergeTriangles

Only deals with vertexes and indexes, not silhouettes, planes, etc.
Does NOT perform a cleanup triangles, so there may be duplicated verts in the result.
=================
*/
srfTriangles_t* R_MergeTriangles( const srfTriangles_t* tri1, const srfTriangles_t* tri2 )
{
	const srfTriangles_t*	tris[2];

	tris[0] = tri1;
	tris[1] = tri2;

	return R_MergeSurfaceList( tris, 2 );
}

/*
=================
R_ReverseTriangles

Lit two sided surfaces need to have the triangles actually duplicated,
they can't just turn on two sided lighting, because the normal and tangents
are wrong on the other sides.

This should be called before R_CleanupTriangles
=================
*/
void R_ReverseTriangles( srfTriangles_t* tri )
{
	int			i;

	// flip the normal on each vertex
	// If the surface is going to have generated normals, this won't matter,
	// but if it has explicit normals, this will keep it on the correct side
	for( i = 0; i < tri->numVerts; i++ )
	{
		tri->verts[i].SetNormal( vec3_origin - tri->verts[i].GetNormal() );
	}

	// flip the index order to make them back sided
	for( i = 0; i < tri->numIndexes; i += 3 )
	{
		triIndex_t	temp;

		temp = tri->indexes[ i + 0 ];
		tri->indexes[ i + 0 ] = tri->indexes[ i + 1 ];
		tri->indexes[ i + 1 ] = temp;
	}
}

/*
=================
R_CleanupTriangles

FIXME: allow createFlat and createSmooth normals, as well as explicit
=================
*/
void R_CleanupTriangles( srfTriangles_t* tri, bool createNormals, bool identifySilEdges, bool useUnsmoothedTangents, bool useMikktspace )
{
	R_RangeCheckIndexes( tri );

	R_CreateSilIndexes( tri );

//	R_RemoveDuplicatedTriangles( tri );	// this may remove valid overlapped transparent triangles

	R_RemoveDegenerateTriangles( tri );

	R_TestDegenerateTextureSpace( tri );

//	R_RemoveUnusedVerts( tri );

	if( identifySilEdges )
	{
		R_IdentifySilEdges( tri, true );	// assume it is non-deformable, and omit coplanar edges
	}

	// bust vertexes that share a mirrored edge into separate vertexes
	R_DuplicateMirroredVertexes( tri );

	R_CreateDupVerts( tri );

	R_BoundTriSurf( tri );

	if( useUnsmoothedTangents )
	{
		R_BuildDominantTris( tri );
		R_DeriveTangents( tri );
	}
	else if( !createNormals )
	{
		R_DeriveTangentsWithoutNormals( tri, useMikktspace );
	}
	else
	{
		R_DeriveTangents( tri );
	}
}

/*
===================================================================================

DEFORMED SURFACES

===================================================================================
*/

/*
===================
R_BuildDeformInfo
===================
*/
deformInfo_t* R_BuildDeformInfo( int numVerts, const idDrawVert* verts, int numIndexes, const int* indexes,
								 bool useUnsmoothedTangents )
{
	srfTriangles_t	tri;
	memset( &tri, 0, sizeof( srfTriangles_t ) );

	tri.numVerts = numVerts;
	R_AllocStaticTriSurfVerts( &tri, tri.numVerts );
	SIMDProcessor->Memcpy( tri.verts, verts, tri.numVerts * sizeof( tri.verts[0] ) );

	tri.numIndexes = numIndexes;
	R_AllocStaticTriSurfIndexes( &tri, tri.numIndexes );

	// don't memcpy, so we can change the index type from int to short without changing the interface
	for( int i = 0; i < tri.numIndexes; i++ )
	{
		tri.indexes[i] = indexes[i];
	}

	R_RangeCheckIndexes( &tri );
	R_CreateSilIndexes( &tri );
	R_IdentifySilEdges( &tri, false );			// we cannot remove coplanar edges, because they can deform to silhouettes
	R_DuplicateMirroredVertexes( &tri );		// split mirror points into multiple points
	R_CreateDupVerts( &tri );
	if( useUnsmoothedTangents )
	{
		R_BuildDominantTris( &tri );
	}
	R_DeriveTangents( &tri );

	deformInfo_t* deform = ( deformInfo_t* )R_ClearedStaticAlloc( sizeof( *deform ) );

	deform->numSourceVerts = numVerts;
	deform->numOutputVerts = tri.numVerts;
	deform->verts = tri.verts;

	deform->numIndexes = numIndexes;
	deform->indexes = tri.indexes;

	deform->silIndexes = tri.silIndexes;

	deform->numSilEdges = tri.numSilEdges;
	deform->silEdges = tri.silEdges;

	deform->numMirroredVerts = tri.numMirroredVerts;
	deform->mirroredVerts = tri.mirroredVerts;

	deform->numDupVerts = tri.numDupVerts;
	deform->dupVerts = tri.dupVerts;

	if( tri.dominantTris != NULL )
	{
		Mem_Free( tri.dominantTris );
		tri.dominantTris = NULL;
	}

	//R_CreateDeformStaticVertices( deform, commandList );

	return deform;
}

/*
==============================
R_CreateDeformStaticVertices
==============================
Uploads static vertices to the vertex cache.
*/
void R_CreateDeformStaticVertices( deformInfo_t* deform, nvrhi::ICommandList* commandList )
{
	idShadowVertSkinned* shadowVerts = ( idShadowVertSkinned* )Mem_Alloc16( ALIGN( deform->numOutputVerts * 2 * sizeof( idShadowVertSkinned ), 16 ), TAG_MODEL );
	idShadowVertSkinned::CreateShadowCache( shadowVerts, deform->verts, deform->numOutputVerts );

	deform->staticAmbientCache = vertexCache.AllocStaticVertex( deform->verts, ALIGN( deform->numOutputVerts * sizeof( idDrawVert ), VERTEX_CACHE_ALIGN ), commandList );
	deform->staticIndexCache = vertexCache.AllocStaticIndex( deform->indexes, ALIGN( deform->numIndexes * sizeof( triIndex_t ), INDEX_CACHE_ALIGN ), commandList );
	deform->staticShadowCache = vertexCache.AllocStaticVertex( shadowVerts, ALIGN( deform->numOutputVerts * 2 * sizeof( idShadowVertSkinned ), VERTEX_CACHE_ALIGN ), commandList );

	Mem_Free( shadowVerts );
}

/*
===================
R_FreeDeformInfo
===================
*/
void R_FreeDeformInfo( deformInfo_t* deformInfo )
{
	if( deformInfo->verts != NULL )
	{
		Mem_Free( deformInfo->verts );
	}
	if( deformInfo->indexes != NULL )
	{
		Mem_Free( deformInfo->indexes );
	}
	if( deformInfo->silIndexes != NULL )
	{
		Mem_Free( deformInfo->silIndexes );
	}
	if( deformInfo->silEdges != NULL )
	{
		Mem_Free( deformInfo->silEdges );
	}
	if( deformInfo->mirroredVerts != NULL )
	{
		Mem_Free( deformInfo->mirroredVerts );
	}
	if( deformInfo->dupVerts != NULL )
	{
		Mem_Free( deformInfo->dupVerts );
	}
	R_StaticFree( deformInfo );
}

/*
===================
R_DeformInfoMemoryUsed
===================
*/
int R_DeformInfoMemoryUsed( deformInfo_t* deformInfo )
{
	int total = 0;

	if( deformInfo->verts != NULL )
	{
		total += deformInfo->numOutputVerts * sizeof( deformInfo->verts[0] );
	}
	if( deformInfo->indexes != NULL )
	{
		total += deformInfo->numIndexes * sizeof( deformInfo->indexes[0] );
	}
	if( deformInfo->mirroredVerts != NULL )
	{
		total += deformInfo->numMirroredVerts * sizeof( deformInfo->mirroredVerts[0] );
	}
	if( deformInfo->dupVerts != NULL )
	{
		total += deformInfo->numDupVerts * sizeof( deformInfo->dupVerts[0] );
	}
	if( deformInfo->silIndexes != NULL )
	{
		total += deformInfo->numIndexes * sizeof( deformInfo->silIndexes[0] );
	}
	if( deformInfo->silEdges != NULL )
	{
		total += deformInfo->numSilEdges * sizeof( deformInfo->silEdges[0] );
	}

	total += sizeof( *deformInfo );
	return total;
}

/*
===================================================================================

VERTEX / INDEX CACHING

===================================================================================
*/

/*
===================
R_InitDrawSurfFromTri
===================
*/
void R_InitDrawSurfFromTri( drawSurf_t& ds, srfTriangles_t& tri, nvrhi::ICommandList* commandList )
{
	if( tri.numIndexes == 0 )
	{
		ds.numIndexes = 0;
		return;
	}

	// copy verts and indexes to this frame's hardware memory if they aren't already there
	//
	// deformed surfaces will not have any vertices but the ambient cache will have already
	// been created for them.
	if( ( tri.verts == NULL ) && !tri.referencedIndexes )
	{
		// pre-generated shadow models will not have any verts, just shadowVerts
		tri.ambientCache = 0;
	}
	else if( !vertexCache.CacheIsCurrent( tri.ambientCache ) )
	{
		tri.ambientCache = vertexCache.AllocVertex( tri.verts, tri.numVerts, sizeof( idDrawVert ), commandList );
	}
	if( !vertexCache.CacheIsCurrent( tri.indexCache ) )
	{
		tri.indexCache = vertexCache.AllocIndex( tri.indexes, tri.numIndexes, sizeof( triIndex_t ), commandList );
	}

	ds.numIndexes = tri.numIndexes;
	ds.ambientCache = tri.ambientCache;
	ds.indexCache = tri.indexCache;
	ds.shadowCache = tri.shadowCache;
	ds.jointCache = 0;
}

/*
===================
R_CreateStaticBuffersForTri

For static surfaces, the indexes, ambient, and shadow buffers can be pre-created at load
time, rather than being re-created each frame in the frame temporary buffers.
===================
*/
void R_CreateStaticBuffersForTri( srfTriangles_t& tri, nvrhi::ICommandList* commandList )
{
	tri.indexCache = 0;
	tri.ambientCache = 0;
	tri.shadowCache = 0;

	// index cache
	if( tri.indexes != NULL )
	{
		tri.indexCache = vertexCache.AllocStaticIndex( tri.indexes, ALIGN( tri.numIndexes * sizeof( tri.indexes[0] ), INDEX_CACHE_ALIGN ), commandList );
	}

	// vertex cache
	if( tri.verts != NULL )
	{
		tri.ambientCache = vertexCache.AllocStaticVertex( tri.verts, ALIGN( tri.numVerts * sizeof( tri.verts[0] ), VERTEX_CACHE_ALIGN ), commandList );
	}

	// shadow cache
	if( tri.preLightShadowVertexes != NULL )
	{
		// this should only be true for the _prelight<NAME> pre-calculated shadow volumes
		assert( tri.verts == NULL );	// pre-light shadow volume surfaces don't have ambient vertices
		const int shadowSize = ALIGN( tri.numVerts * 2 * sizeof( idShadowVert ), VERTEX_CACHE_ALIGN );
		tri.shadowCache = vertexCache.AllocStaticVertex( tri.preLightShadowVertexes, shadowSize, commandList );
	}
	else if( tri.verts != NULL )
	{
		// the shadowVerts for normal models include all the xyz values duplicated
		// for a W of 1 (near cap) and a W of 0 (end cap, projected to infinity)
		const int shadowSize = ALIGN( tri.numVerts * 2 * sizeof( idShadowVert ), VERTEX_CACHE_ALIGN );
		if( tri.staticShadowVertexes == NULL )
		{
			tri.staticShadowVertexes = ( idShadowVert* ) Mem_Alloc16( shadowSize, TAG_TEMP );
			idShadowVert::CreateShadowCache( tri.staticShadowVertexes, tri.verts, tri.numVerts );
		}
		tri.shadowCache = vertexCache.AllocStaticVertex( tri.staticShadowVertexes, shadowSize, commandList );

#if !defined( KEEP_INTERACTION_CPU_DATA )
		Mem_Free( tri.staticShadowVertexes );
		tri.staticShadowVertexes = NULL;
#endif
	}
}

// SP begin
static void* mkAlloc( int bytes )
{
	return R_StaticAlloc( bytes );
}

static void mkFree( void* mem )
{
	R_StaticFree( mem );
}

static int mkGetNumFaces( const SMikkTSpaceContext* pContext )
{
	srfTriangles_t* tris = reinterpret_cast<srfTriangles_t*>( pContext->m_pUserData );
	return tris->numIndexes / 3;
}

static int mkGetNumVerticesOfFace( const SMikkTSpaceContext* pContext, const int iFace )
{
	return 3;
}

static void mkGetPosition( const SMikkTSpaceContext* pContext, float fvPosOut[], const int iFace, const int iVert )
{
	srfTriangles_t* tris = reinterpret_cast<srfTriangles_t*>( pContext->m_pUserData );

	const int vertIndex = iFace * 3;
	const int index = tris->indexes[vertIndex + iVert];
	const idDrawVert& vert = tris->verts[index];

	fvPosOut[0] = vert.xyz[0];
	fvPosOut[1] = vert.xyz[1];
	fvPosOut[2] = vert.xyz[2];
}

static void mkGetNormal( const SMikkTSpaceContext* pContext, float fvNormOut[], const int iFace, const int iVert )
{
	srfTriangles_t* tris = reinterpret_cast<srfTriangles_t*>( pContext->m_pUserData );

	const int vertIndex = iFace * 3;
	const int index = tris->indexes[vertIndex + iVert];
	const idDrawVert& vert = tris->verts[index];

	const idVec3 norm = vert.GetNormal();
	fvNormOut[0] = norm.x;
	fvNormOut[1] = norm.y;
	fvNormOut[2] = norm.z;
}

static void mkGetTexCoord( const SMikkTSpaceContext* pContext, float fvTexcOut[], const int iFace, const int iVert )
{
	srfTriangles_t* tris = reinterpret_cast<srfTriangles_t*>( pContext->m_pUserData );

	const int vertIndex = iFace * 3;
	const int index = tris->indexes[vertIndex + iVert];
	const idDrawVert& vert = tris->verts[index];

	const idVec2 texCoord = vert.GetTexCoord();
	fvTexcOut[0] = texCoord.x;
	fvTexcOut[1] = texCoord.y;
}

static void mkSetTSpaceBasic( const SMikkTSpaceContext* pContext, const float fvTangent[], const float fSign, const int iFace, const int iVert )
{
	srfTriangles_t* tris = reinterpret_cast<srfTriangles_t*>( pContext->m_pUserData );

	const int vertIndex = iFace * 3;
	const int index = tris->indexes[vertIndex + iVert];

	const idVec3 tangent( fvTangent[0], fvTangent[1], fvTangent[2] );
	tris->verts[index].SetTangent( tangent );
	tris->verts[index].SetBiTangentSign( fSign );
}

idMikkTSpaceInterface::idMikkTSpaceInterface()
	: mkInterface()
{
	mkInterface.m_alloc = mkAlloc;
	mkInterface.m_free = mkFree;
	mkInterface.m_getNumFaces = mkGetNumFaces;
	mkInterface.m_getNumVerticesOfFace = mkGetNumVerticesOfFace;
	mkInterface.m_getPosition = mkGetPosition;
	mkInterface.m_getNormal = mkGetNormal;
	mkInterface.m_getTexCoord = mkGetTexCoord;
	mkInterface.m_setTSpaceBasic = mkSetTSpaceBasic;
}

static void SetUpMikkTSpaceContext( SMikkTSpaceContext* context )
{
	context->m_pInterface = &mikkTSpaceInterface.mkInterface;
}

// SP end


// RB: Determines the closest point between a point and a triangle
idVec3 R_ClosestPointPointTriangle( const idVec3& point, const idVec3& vertex1, const idVec3& vertex2, const idVec3& vertex3 )
{
	idVec3 result;

	// Source: Real-Time Collision Detection by Christer Ericson
	// Reference: Page 136

	// check if P in vertex region outside A
	idVec3 ab = vertex2 - vertex1;
	idVec3 ac = vertex3 - vertex1;
	idVec3 ap = point - vertex1;

	float d1 = ( ab * ap );
	float d2 = ( ac * ap );
	if( d1 <= 0.0f && d2 <= 0.0f )
	{
		result = vertex1; //Barycentric coordinates (1,0,0)
		return result;
	}

	// Check if P in vertex region outside B
	idVec3 bp = point - vertex2;
	float d3 = ( ab * bp );
	float d4 = ( ac * bp );
	if( d3 >= 0.0f && d4 <= d3 )
	{
		result = vertex2; // barycentric coordinates (0,1,0)
		return result;
	}

	// Check if P in edge region of AB, if so return projection of P onto AB
	float vc = d1 * d4 - d3 * d2;
	if( vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f )
	{
		float v = d1 / ( d1 - d3 );
		result = vertex1 + v * ab; //Barycentric coordinates (1-v,v,0)
		return result;
	}

	// Check if P in vertex region outside C
	idVec3 cp = point - vertex3;
	float d5 = ( ab * cp );
	float d6 = ( ac * cp );
	if( d6 >= 0.0f && d5 <= d6 )
	{
		result = vertex3; //Barycentric coordinates (0,0,1)
		return result;
	}

	// Check if P in edge region of AC, if so return projection of P onto AC
	float vb = d5 * d2 - d1 * d6;
	if( vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f )
	{
		float w = d2 / ( d2 - d6 );
		result = vertex1 + w * ac; //Barycentric coordinates (1-w,0,w)
		return result;
	}

	// Check if P in edge region of BC, if so return projection of P onto BC
	float va = d3 * d6 - d5 * d4;
	if( va <= 0.0f && ( d4 - d3 ) >= 0.0f && ( d5 - d6 ) >= 0.0f )
	{
		float w = ( d4 - d3 ) / ( ( d4 - d3 ) + ( d5 - d6 ) );
		result = vertex2 + w * ( vertex3 - vertex2 ); //Barycentric coordinates (0,1-w,w)
		return result;
	}

	// P inside face region. Compute Q through its barycentric coordinates (u,v,w)
	float denom = 1.0f / ( va + vb + vc );
	float v2 = vb * denom;
	float w2 = vc * denom;
	result = vertex1 + ab * v2 + ac * w2; //= u*vertex1 + v*vertex2 + w*vertex3, u = va * denom = 1.0f - v - w

	return result;
}