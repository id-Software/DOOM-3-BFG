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

#include "dmap.h"

/*

  T junction fixing never creates more xyz points, but
  new vertexes will be created when different surfaces
  cause a fix

  The vertex cleaning accomplishes two goals: removing extranious low order
  bits to avoid numbers like 1.000001233, and grouping nearby vertexes
  together.  Straight truncation accomplishes the first foal, but two vertexes
  only a tiny epsilon apart could still be spread to different snap points.
  To avoid this, we allow the merge test to group points together that
  snapped to neighboring integer coordinates.

  Snaping verts can drag some triangles backwards or collapse them to points,
  which will cause them to be removed.


  When snapping to ints, a point can move a maximum of sqrt(3)/2 distance
  Two points that were an epsilon apart can then become sqrt(3) apart

  A case that causes recursive overflow with point to triangle fixing:

               A
	C            D
	           B

  Triangle ABC tests against point D and splits into triangles ADC and DBC
  Triangle DBC then tests against point A again and splits into ABC and ADB
  infinite recursive loop


  For a given source triangle
	init the no-check list to hold the three triangle hashVerts

  recursiveFixTriAgainstHash

  recursiveFixTriAgainstHashVert_r
	if hashVert is on the no-check list
		exit
	if the hashVert should split the triangle
		add to the no-check list
		recursiveFixTriAgainstHash(a)
		recursiveFixTriAgainstHash(b)

*/

#define	SNAP_FRACTIONS	32
//#define	SNAP_FRACTIONS	8
//#define	SNAP_FRACTIONS	1

#define	VERTEX_EPSILON	( 1.0 / SNAP_FRACTIONS )

#define	COLINEAR_EPSILON	( 1.8 * VERTEX_EPSILON )

#define	HASH_BINS	16

typedef struct hashVert_s
{
	struct hashVert_s*	next;
	idVec3				v;
	int					iv[3];
} hashVert_t;

static idBounds	hashBounds;
static idVec3	hashScale;
static hashVert_t*	hashVerts[HASH_BINS][HASH_BINS][HASH_BINS];
static int		numHashVerts, numTotalVerts;
static int		hashIntMins[3], hashIntScale[3];

/*
===============
GetHashVert

Also modifies the original vert to the snapped value
===============
*/
struct hashVert_s*	GetHashVert( idVec3& v )
{
	int		iv[3];
	int		block[3];
	int		i;
	hashVert_t*	hv;

	numTotalVerts++;

	// snap the vert to integral values
	for( i = 0 ; i < 3 ; i++ )
	{
		iv[i] = floor( ( v[i] + 0.5 / SNAP_FRACTIONS ) * SNAP_FRACTIONS );
		block[i] = ( iv[i] - hashIntMins[i] ) / hashIntScale[i];
		if( block[i] < 0 )
		{
			block[i] = 0;
		}
		else if( block[i] >= HASH_BINS )
		{
			block[i] = HASH_BINS - 1;
		}
	}

	// see if a vertex near enough already exists
	// this could still fail to find a near neighbor right at the hash block boundary
	for( hv = hashVerts[block[0]][block[1]][block[2]] ; hv ; hv = hv->next )
	{
		for( i = 0 ; i < 3 ; i++ )
		{
			int	d;
			d = hv->iv[i] - iv[i];
			if( d < -1 || d > 1 )
			{
				break;
			}
		}
		if( i == 3 )
		{
			v = hv->v;
			return hv;
		}
	}

	// create a new one
	hv = ( hashVert_t* )Mem_Alloc( sizeof( *hv ), TAG_TOOLS );

	hv->next = hashVerts[block[0]][block[1]][block[2]];
	hashVerts[block[0]][block[1]][block[2]] = hv;

	hv->iv[0] = iv[0];
	hv->iv[1] = iv[1];
	hv->iv[2] = iv[2];

	hv->v[0] = ( float )iv[0] / SNAP_FRACTIONS;
	hv->v[1] = ( float )iv[1] / SNAP_FRACTIONS;
	hv->v[2] = ( float )iv[2] / SNAP_FRACTIONS;

	v = hv->v;

	numHashVerts++;

	return hv;
}


/*
==================
HashBlocksForTri

Returns an inclusive bounding box of hash
bins that should hold the triangle
==================
*/
static void HashBlocksForTri( const mapTri_t* tri, int blocks[2][3] )
{
	idBounds	bounds;
	int			i;

	bounds.Clear();
	bounds.AddPoint( tri->v[0].xyz );
	bounds.AddPoint( tri->v[1].xyz );
	bounds.AddPoint( tri->v[2].xyz );

	// add a 1.0 slop margin on each side
	for( i = 0 ; i < 3 ; i++ )
	{
		blocks[0][i] = ( bounds[0][i] - 1.0 - hashBounds[0][i] ) / hashScale[i];
		if( blocks[0][i] < 0 )
		{
			blocks[0][i] = 0;
		}
		else if( blocks[0][i] >= HASH_BINS )
		{
			blocks[0][i] = HASH_BINS - 1;
		}

		blocks[1][i] = ( bounds[1][i] + 1.0 - hashBounds[0][i] ) / hashScale[i];
		if( blocks[1][i] < 0 )
		{
			blocks[1][i] = 0;
		}
		else if( blocks[1][i] >= HASH_BINS )
		{
			blocks[1][i] = HASH_BINS - 1;
		}
	}
}


/*
=================
HashTriangles

Removes triangles that are degenerated or flipped backwards
=================
*/
void HashTriangles( optimizeGroup_t* groupList )
{
	mapTri_t*	a;
	int			vert;
	int			i;
	optimizeGroup_t*	group;

	// clear the hash tables
	memset( hashVerts, 0, sizeof( hashVerts ) );

	numHashVerts = 0;
	numTotalVerts = 0;

	// bound all the triangles to determine the bucket size
	hashBounds.Clear();
	for( group = groupList ; group ; group = group->nextGroup )
	{
		for( a = group->triList ; a ; a = a->next )
		{
			hashBounds.AddPoint( a->v[0].xyz );
			hashBounds.AddPoint( a->v[1].xyz );
			hashBounds.AddPoint( a->v[2].xyz );
		}
	}

	// spread the bounds so it will never have a zero size
	for( i = 0 ; i < 3 ; i++ )
	{
		hashBounds[0][i] = floor( hashBounds[0][i] - 1 );
		hashBounds[1][i] = ceil( hashBounds[1][i] + 1 );
		hashIntMins[i] = hashBounds[0][i] * SNAP_FRACTIONS;

		hashScale[i] = ( hashBounds[1][i] - hashBounds[0][i] ) / HASH_BINS;
		hashIntScale[i] = hashScale[i] * SNAP_FRACTIONS;
		if( hashIntScale[i] < 1 )
		{
			hashIntScale[i] = 1;
		}
	}

	// add all the points to the hash buckets
	for( group = groupList ; group ; group = group->nextGroup )
	{
		// don't create tjunctions against discrete surfaces (blood decals, etc)
		if( group->material != NULL && group->material->IsDiscrete() )
		{
			continue;
		}
		for( a = group->triList ; a ; a = a->next )
		{
			for( vert = 0 ; vert < 3 ; vert++ )
			{
				a->hashVert[vert] = GetHashVert( a->v[vert].xyz );
			}
		}
	}
}

/*
=================
FreeTJunctionHash

The optimizer may add some more crossing verts
after t junction processing
=================
*/
void FreeTJunctionHash()
{
	int			i, j, k;
	hashVert_t*	hv, *next;

	for( i = 0 ; i < HASH_BINS ; i++ )
	{
		for( j = 0 ; j < HASH_BINS ; j++ )
		{
			for( k = 0 ; k < HASH_BINS ; k++ )
			{
				for( hv = hashVerts[i][j][k] ; hv ; hv = next )
				{
					next = hv->next;
					Mem_Free( hv );
				}
			}
		}
	}
	memset( hashVerts, 0, sizeof( hashVerts ) );
}


/*
==================
FixTriangleAgainstHashVert

Returns a list of two new mapTri if the hashVert is
on an edge of the given mapTri, otherwise returns NULL.
==================
*/
static mapTri_t* FixTriangleAgainstHashVert( const mapTri_t* a, const hashVert_t* hv )
{
	int			i;
	const idDrawVert*	v1, *v2, *v3;
	idDrawVert	split;
	idVec3		dir;
	float		len;
	float		frac;
	mapTri_t*	new1, *new2;
	idVec3		temp;
	float		d, off;
	const idVec3* v;
	idPlane		plane1, plane2;

	v = &hv->v;

	// if the triangle already has this hashVert as a vert,
	// it can't be split by it
	if( a->hashVert[0] == hv || a->hashVert[1] == hv || a->hashVert[2] == hv )
	{
		return NULL;
	}

	// we probably should find the edge that the vertex is closest to.
	// it is possible to be < 1 unit away from multiple
	// edges, but we only want to split by one of them
	for( i = 0 ; i < 3 ; i++ )
	{
		v1 = &a->v[i];
		v2 = &a->v[( i + 1 ) % 3];
		v3 = &a->v[( i + 2 ) % 3];
		dir = v2->xyz - v1->xyz;

		len = dir.Normalize();

		// if it is close to one of the edge vertexes, skip it
		temp = *v - v1->xyz;
		d = temp * dir;
		if( d <= 0 || d >= len )
		{
			continue;
		}

		// make sure it is on the line
		VectorMA( v1->xyz, d, dir, temp );
		temp = *v - temp;
		off = temp.Length();
		if( off <= -COLINEAR_EPSILON || off >= COLINEAR_EPSILON )
		{
			continue;
		}

		// take the x/y/z from the splitter,
		// but interpolate everything else from the original tri
		split.xyz = *v;
		frac = d / len;

		// RB begin
		const idVec2 v1ST = v1->GetTexCoord();
		const idVec2 v2ST = v2->GetTexCoord();

		split.SetTexCoord(	v1ST.x + frac * ( v2ST.x - v1ST.x ),
							v1ST.y + frac * ( v2ST.y - v1ST.y ) );

		idVec3 splitNormal;
		idVec3 v1Normal = v1->GetNormal();
		idVec3 v2Normal = v2->GetNormal();

		splitNormal[0] = v1Normal[0] + frac * ( v2Normal[0] - v1Normal[0] );
		splitNormal[1] = v1Normal[1] + frac * ( v2Normal[1] - v1Normal[1] );
		splitNormal[2] = v1Normal[2] + frac * ( v2Normal[2] - v1Normal[2] );
		splitNormal.Normalize();

		split.SetNormal( splitNormal );
		// RB end

		// split the tri
		new1 = CopyMapTri( a );
		new1->v[( i + 1 ) % 3] = split;
		new1->hashVert[( i + 1 ) % 3] = hv;
		new1->next = NULL;

		new2 = CopyMapTri( a );
		new2->v[i] = split;
		new2->hashVert[i] = hv;
		new2->next = new1;

		plane1.FromPoints( new1->hashVert[0]->v, new1->hashVert[1]->v, new1->hashVert[2]->v );
		plane2.FromPoints( new2->hashVert[0]->v, new2->hashVert[1]->v, new2->hashVert[2]->v );

		d = plane1.Normal() * plane2.Normal();

		// if the two split triangle's normals don't face the same way,
		// it should not be split
		if( d <= 0 )
		{
			FreeTriList( new2 );
			continue;
		}

		return new2;
	}


	return NULL;
}



/*
==================
FixTriangleAgainstHash

Potentially splits a triangle into a list of triangles based on tjunctions
==================
*/
static mapTri_t*	FixTriangleAgainstHash( const mapTri_t* tri )
{
	mapTri_t*		fixed;
	mapTri_t*		a;
	mapTri_t*		test, *next;
	int				blocks[2][3];
	int				i, j, k;
	hashVert_t*		hv;

	// if this triangle is degenerate after point snapping,
	// do nothing (this shouldn't happen, because they should
	// be removed as they are hashed)
	if( tri->hashVert[0] == tri->hashVert[1]
			|| tri->hashVert[0] == tri->hashVert[2]
			|| tri->hashVert[1] == tri->hashVert[2] )
	{
		return NULL;
	}

	fixed = CopyMapTri( tri );
	fixed->next = NULL;

	HashBlocksForTri( tri, blocks );
	for( i = blocks[0][0] ; i <= blocks[1][0] ; i++ )
	{
		for( j = blocks[0][1] ; j <= blocks[1][1] ; j++ )
		{
			for( k = blocks[0][2] ; k <= blocks[1][2] ; k++ )
			{
				for( hv = hashVerts[i][j][k] ; hv ; hv = hv->next )
				{
					// fix all triangles in the list against this point
					test = fixed;
					fixed = NULL;
					for( ; test ; test = next )
					{
						next = test->next;
						a = FixTriangleAgainstHashVert( test, hv );
						if( a )
						{
							// cut into two triangles
							a->next->next = fixed;
							fixed = a;
							FreeTri( test );
						}
						else
						{
							test->next = fixed;
							fixed = test;
						}
					}
				}
			}
		}
	}

	return fixed;
}


/*
==================
CountGroupListTris
==================
*/
int CountGroupListTris( const optimizeGroup_t* groupList )
{
	int		c;

	c = 0;
	for( ; groupList ; groupList = groupList->nextGroup )
	{
		c += CountTriList( groupList->triList );
	}

	return c;
}

/*
==================
FixAreaGroupsTjunctions
==================
*/
void	FixAreaGroupsTjunctions( optimizeGroup_t* groupList )
{
	const mapTri_t*	tri;
	mapTri_t*		newList;
	mapTri_t*		fixed;
	int				startCount, endCount;
	optimizeGroup_t*	group;

	if( dmapGlobals.noTJunc )
	{
		return;
	}

	if( !groupList )
	{
		return;
	}

	startCount = CountGroupListTris( groupList );

	if( dmapGlobals.verbose )
	{
		common->Printf( "----- FixAreaGroupsTjunctions -----\n" );
		common->Printf( "%6i triangles in\n", startCount );
	}

	HashTriangles( groupList );

	for( group = groupList ; group ; group = group->nextGroup )
	{
		// don't touch discrete surfaces
		if( group->material != NULL && group->material->IsDiscrete() )
		{
			continue;
		}

		newList = NULL;
		for( tri = group->triList ; tri ; tri = tri->next )
		{
			fixed = FixTriangleAgainstHash( tri );
			newList = MergeTriLists( newList, fixed );
		}
		FreeTriList( group->triList );
		group->triList = newList;
	}

	endCount = CountGroupListTris( groupList );
	if( dmapGlobals.verbose )
	{
		common->Printf( "%6i triangles out\n", endCount );
	}
}


/*
==================
FixEntityTjunctions
==================
*/
void	FixEntityTjunctions( uEntity_t* e )
{
	int		i;

	for( i = 0 ; i < e->numAreas ; i++ )
	{
		FixAreaGroupsTjunctions( e->areas[i].groups );
		FreeTJunctionHash();
	}
}

/*
==================
FixGlobalTjunctions
==================
*/
void	FixGlobalTjunctions( uEntity_t* e )
{
	mapTri_t*	a;
	int			vert;
	int			i;
	optimizeGroup_t*	group;
	int			areaNum;

	common->Printf( "----- FixGlobalTjunctions -----\n" );

	// clear the hash tables
	memset( hashVerts, 0, sizeof( hashVerts ) );

	numHashVerts = 0;
	numTotalVerts = 0;

	// bound all the triangles to determine the bucket size
	hashBounds.Clear();
	for( areaNum = 0 ; areaNum < e->numAreas ; areaNum++ )
	{
		for( group = e->areas[areaNum].groups ; group ; group = group->nextGroup )
		{
			for( a = group->triList ; a ; a = a->next )
			{
				hashBounds.AddPoint( a->v[0].xyz );
				hashBounds.AddPoint( a->v[1].xyz );
				hashBounds.AddPoint( a->v[2].xyz );
			}
		}
	}

	// spread the bounds so it will never have a zero size
	for( i = 0 ; i < 3 ; i++ )
	{
		hashBounds[0][i] = floor( hashBounds[0][i] - 1 );
		hashBounds[1][i] = ceil( hashBounds[1][i] + 1 );
		hashIntMins[i] = hashBounds[0][i] * SNAP_FRACTIONS;

		hashScale[i] = ( hashBounds[1][i] - hashBounds[0][i] ) / HASH_BINS;
		hashIntScale[i] = hashScale[i] * SNAP_FRACTIONS;
		if( hashIntScale[i] < 1 )
		{
			hashIntScale[i] = 1;
		}
	}

	// add all the points to the hash buckets
	for( areaNum = 0 ; areaNum < e->numAreas ; areaNum++ )
	{
		for( group = e->areas[areaNum].groups ; group ; group = group->nextGroup )
		{
			// don't touch discrete surfaces
			if( group->material != NULL && group->material->IsDiscrete() )
			{
				continue;
			}

			for( a = group->triList ; a ; a = a->next )
			{
				for( vert = 0 ; vert < 3 ; vert++ )
				{
					a->hashVert[vert] = GetHashVert( a->v[vert].xyz );
				}
			}
		}
	}

	// add all the func_static model vertexes to the hash buckets
	// optionally inline some of the func_static models
	if( dmapGlobals.entityNum == 0 )
	{
		for( int eNum = 1 ; eNum < dmapGlobals.num_entities ; eNum++ )
		{
			uEntity_t* entity = &dmapGlobals.uEntities[eNum];
			const char* className = entity->mapEntity->epairs.GetString( "classname" );
			if( idStr::Icmp( className, "func_static" ) )
			{
				continue;
			}
			const char* modelName = entity->mapEntity->epairs.GetString( "model" );
			if( !modelName )
			{
				continue;
			}

			// RB: DAE support
			if( !strstr( modelName, ".lwo" ) && !strstr( modelName, ".ase" ) && !strstr( modelName, ".ma" ) && !strstr( modelName, ".dae" ) )
			{
				continue;
			}

			idRenderModel*	model = renderModelManager->FindModel( modelName );

//			common->Printf( "adding T junction verts for %s.\n", entity->mapEntity->epairs.GetString( "name" ) );

			idMat3	axis;
			// get the rotation matrix in either full form, or single angle form
			if( !entity->mapEntity->epairs.GetMatrix( "rotation", "1 0 0 0 1 0 0 0 1", axis ) )
			{
				float angle = entity->mapEntity->epairs.GetFloat( "angle" );
				if( angle != 0.0f )
				{
					axis = idAngles( 0.0f, angle, 0.0f ).ToMat3();
				}
				else
				{
					axis.Identity();
				}
			}

			idVec3	origin = entity->mapEntity->epairs.GetVector( "origin" );

			for( i = 0 ; i < model->NumSurfaces() ; i++ )
			{
				const modelSurface_t* surface = model->Surface( i );
				const srfTriangles_t* tri = surface->geometry;

				mapTri_t	mapTri;
				memset( &mapTri, 0, sizeof( mapTri ) );
				mapTri.material = surface->shader;
				// don't let discretes (autosprites, etc) merge together
				if( mapTri.material->IsDiscrete() )
				{
					mapTri.mergeGroup = ( void* )surface;
				}
				for( int j = 0 ; j < tri->numVerts ; j += 3 )
				{
					idVec3 v = tri->verts[j].xyz * axis + origin;
					GetHashVert( v );
				}
			}
		}
	}



	// now fix each area
	for( areaNum = 0 ; areaNum < e->numAreas ; areaNum++ )
	{
		for( group = e->areas[areaNum].groups ; group ; group = group->nextGroup )
		{
			// don't touch discrete surfaces
			if( group->material != NULL && group->material->IsDiscrete() )
			{
				continue;
			}

			mapTri_t* newList = NULL;
			for( mapTri_t* tri = group->triList ; tri ; tri = tri->next )
			{
				mapTri_t* fixed = FixTriangleAgainstHash( tri );
				newList = MergeTriLists( newList, fixed );
			}
			FreeTriList( group->triList );
			group->triList = newList;
		}
	}


	// done
	FreeTJunctionHash();
}
