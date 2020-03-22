/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.
Copyright (C) 2015 Robert Beckebans

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


#define	 TEXTURE_OFFSET_EQUAL_EPSILON	0.005
#define	 TEXTURE_VECTOR_EQUAL_EPSILON	0.001

/*
===============
AddTriListToArea

The triList is appended to the apropriate optimzeGroup_t,
creating a new one if needed.
The entire list is assumed to come from the same planar primitive
===============
*/
static void AddTriListToArea( uEntity_t* e, mapTri_t* triList, int planeNum, int areaNum, textureVectors_t* texVec )
{
	uArea_t*		area;
	optimizeGroup_t*	group;
	int			i, j;

	if( !triList )
	{
		return;
	}

	area = &e->areas[areaNum];
	for( group = area->groups ; group ; group = group->nextGroup )
	{
		if( group->material == triList->material
				&& group->planeNum == planeNum
				&& group->mergeGroup == triList->mergeGroup )
		{
			// check the texture vectors
			for( i = 0 ; i < 2 ; i++ )
			{
				for( j = 0 ; j < 3 ; j++ )
				{
					if( idMath::Fabs( texVec->v[i][j] - group->texVec.v[i][j] ) > TEXTURE_VECTOR_EQUAL_EPSILON )
					{
						break;
					}
				}
				if( j != 3 )
				{
					break;
				}
				if( idMath::Fabs( texVec->v[i][3] - group->texVec.v[i][3] ) > TEXTURE_OFFSET_EQUAL_EPSILON )
				{
					break;
				}
			}
			if( i == 2 )
			{
				break;	// exact match
			}
			else
			{
				// different texture offsets
				i = 1;	// just for debugger breakpoint
			}
		}
	}

	if( !group )
	{
		group = ( optimizeGroup_t* )Mem_Alloc( sizeof( *group ), TAG_TOOLS );
		memset( group, 0, sizeof( *group ) );
		group->planeNum = planeNum;
		group->mergeGroup = triList->mergeGroup;
		group->material = triList->material;
		group->nextGroup = area->groups;
		group->texVec = *texVec;
		area->groups = group;
	}

	group->triList = MergeTriLists( group->triList, triList );
}

/*
===================
TexVecForTri
===================
*/
static void TexVecForTri( textureVectors_t* texVec, mapTri_t* tri )
{
	float	area, inva;
	idVec3	temp;
	idVec5	d0, d1;
	idDrawVert*	a, *b, *c;

	a = &tri->v[0];
	b = &tri->v[1];
	c = &tri->v[2];

	d0[0] = b->xyz[0] - a->xyz[0];
	d0[1] = b->xyz[1] - a->xyz[1];
	d0[2] = b->xyz[2] - a->xyz[2];

	// RB begin
	const idVec2 aST = a->GetTexCoord();
	const idVec2 bST = b->GetTexCoord();
	d0[3] = bST.x - aST.x;
	d0[4] = bST.y - aST.y;

	d1[0] = c->xyz[0] - a->xyz[0];
	d1[1] = c->xyz[1] - a->xyz[1];
	d1[2] = c->xyz[2] - a->xyz[2];

	const idVec2 cST = c->GetTexCoord();
	d1[3] = cST.x - aST.x;
	d1[4] = cST.y - aST.y;

	area = d0[3] * d1[4] - d0[4] * d1[3];
	inva = 1.0 / area;

	temp[0] = ( d0[0] * d1[4] - d0[4] * d1[0] ) * inva;
	temp[1] = ( d0[1] * d1[4] - d0[4] * d1[1] ) * inva;
	temp[2] = ( d0[2] * d1[4] - d0[4] * d1[2] ) * inva;
	temp.Normalize();
	texVec->v[0].ToVec3() = temp;
	texVec->v[0][3] = tri->v[0].xyz * texVec->v[0].ToVec3() - tri->v[0].GetTexCoord().x;

	temp[0] = ( d0[3] * d1[0] - d0[0] * d1[3] ) * inva;
	temp[1] = ( d0[3] * d1[1] - d0[1] * d1[3] ) * inva;
	temp[2] = ( d0[3] * d1[2] - d0[2] * d1[3] ) * inva;
	temp.Normalize();
	texVec->v[1].ToVec3() = temp;
	texVec->v[1][3] = tri->v[0].xyz * texVec->v[0].ToVec3() - tri->v[0].GetTexCoord().y;
	// RB end
}


/*
=================
TriListForSide
=================
*/
//#define	SNAP_FLOAT_TO_INT	8
#define	SNAP_FLOAT_TO_INT	256
#define	SNAP_INT_TO_FLOAT	(1.0/SNAP_FLOAT_TO_INT)

mapTri_t* TriListForSide( const side_t* s, const idWinding* w )
{
	int				i, j;
	idDrawVert*		dv;
	mapTri_t*		tri, *triList;
	const idVec3*		vec;
	const idMaterial*	si;

	si = s->material;

	// skip any generated faces
	if( !si )
	{
		return NULL;
	}

	// don't create faces for non-visible sides
	if( !si->SurfaceCastsShadow() && !si->IsDrawn() )
	{
		return NULL;
	}

	if( 1 )
	{
		// triangle fan using only the outer verts
		// this gives the minimum triangle count,
		// but may have some very distended triangles
		triList = NULL;
		for( i = 2 ; i < w->GetNumPoints() ; i++ )
		{
			tri = AllocTri();
			tri->material = si;
			tri->next = triList;
			triList = tri;

			for( j = 0 ; j < 3 ; j++ )
			{
				if( j == 0 )
				{
					vec = &( ( *w )[0] ).ToVec3();
				}
				else if( j == 1 )
				{
					vec = &( ( *w )[i - 1] ).ToVec3();
				}
				else
				{
					vec = &( ( *w )[i] ).ToVec3();
				}

				dv = tri->v + j;
#if 0
				// round the xyz to a given precision
				for( k = 0 ; k < 3 ; k++ )
				{
					dv->xyz[k] = SNAP_INT_TO_FLOAT * floor( vec[k] * SNAP_FLOAT_TO_INT + 0.5 );
				}
#else
				dv->xyz = *vec;
#endif

				// calculate texture s/t from brush primitive texture matrix
				idVec2 st;
				st.x = ( dv->xyz * s->texVec.v[0].ToVec3() ) + s->texVec.v[0][3];
				st.y = ( dv->xyz * s->texVec.v[1].ToVec3() ) + s->texVec.v[1][3];

				dv->SetTexCoord( st );

				// copy normal
				dv->SetNormal( dmapGlobals.mapPlanes[s->planenum].Normal() );
				if( dv->GetNormal().Length() < 0.9 || dv->GetNormal().Length() > 1.1 )
				{
					common->Error( "Bad normal in TriListForSide" );
				}
			}
		}
	}
	else
	{
		// triangle fan from central point, more verts and tris, but less distended
		// I use this when debugging some tjunction problems
		triList = NULL;
		for( i = 0 ; i < w->GetNumPoints() ; i++ )
		{
			idVec3	midPoint;

			tri = AllocTri();
			tri->material = si;
			tri->next = triList;
			triList = tri;

			for( j = 0 ; j < 3 ; j++ )
			{
				if( j == 0 )
				{
					vec = &midPoint;
					midPoint = w->GetCenter();
				}
				else if( j == 1 )
				{
					vec = &( ( *w )[i] ).ToVec3();
				}
				else
				{
					vec = &( ( *w )[( i + 1 ) % w->GetNumPoints()] ).ToVec3();
				}

				dv = tri->v + j;
				dv->xyz = *vec;

				idVec2 st;
				st.x = ( dv->xyz * s->texVec.v[0].ToVec3() ) + s->texVec.v[0][3];
				st.y = ( dv->xyz * s->texVec.v[1].ToVec3() ) + s->texVec.v[1][3];
				dv->SetTexCoord( st );

				// copy normal
				dv->SetNormal( dmapGlobals.mapPlanes[s->planenum].Normal() );
				if( dv->GetNormal().Length() < 0.9f || dv->GetNormal().Length() > 1.1f )
				{
					common->Error( "Bad normal in TriListForSide" );
				}
			}
		}
	}

	// set merge groups if needed, to prevent multiple sides from being
	// merged into a single surface in the case of gui shaders, mirrors, and autosprites
	if( s->material->IsDiscrete() )
	{
		for( tri = triList ; tri ; tri = tri->next )
		{
			tri->mergeGroup = ( void* )s;
		}
	}

	return triList;
}

//=================================================================================

/*
====================
ClipSideByTree_r

Adds non-opaque leaf fragments to the convex hull
====================
*/
static void ClipSideByTree_r( idWinding* w, side_t* side, node_t* node )
{
	idWinding*		front, *back;

	if( !w )
	{
		return;
	}

	if( node->planenum != PLANENUM_LEAF )
	{
		if( side->planenum == node->planenum )
		{
			ClipSideByTree_r( w, side, node->children[0] );
			return;
		}
		if( side->planenum == ( node->planenum ^ 1 ) )
		{
			ClipSideByTree_r( w, side, node->children[1] );
			return;
		}

		w->Split( dmapGlobals.mapPlanes[ node->planenum ], ON_EPSILON, &front, &back );
		delete w;

		ClipSideByTree_r( front, side, node->children[0] );
		ClipSideByTree_r( back, side, node->children[1] );

		return;
	}

	// if opaque leaf, don't add
	if( !node->opaque )
	{
		if( !side->visibleHull )
		{
			side->visibleHull = w->Copy();
		}
		else
		{
			side->visibleHull->AddToConvexHull( w, dmapGlobals.mapPlanes[ side->planenum ].Normal() );
		}
	}

	delete w;
	return;
}


/*
=====================
ClipSidesByTree

Creates side->visibleHull for all visible sides

The visible hull for a side will consist of the convex hull of
all points in non-opaque clusters, which allows overlaps
to be trimmed off automatically.
=====================
*/
void ClipSidesByTree( uEntity_t* e )
{
	uBrush_t*		b;
	int				i;
	idWinding*		w;
	side_t*			side;
	primitive_t*		prim;

	common->Printf( "----- ClipSidesByTree -----\n" );

	for( prim = e->primitives ; prim ; prim = prim->next )
	{
		b = prim->brush;
		if( !b )
		{
			// FIXME: other primitives!
			continue;
		}
		for( i = 0 ; i < b->numsides ; i++ )
		{
			side = &b->sides[i];
			if( !side->winding )
			{
				continue;
			}
			w = side->winding->Copy();
			side->visibleHull = NULL;
			ClipSideByTree_r( w, side, e->tree->headnode );
			// for debugging, we can choose to use the entire original side
			// but we skip this if the side was completely clipped away
			if( side->visibleHull && dmapGlobals.noClipSides )
			{
				delete side->visibleHull;
				side->visibleHull = side->winding->Copy();
			}
		}
	}
}



//=================================================================================

/*
====================
ClipTriIntoTree_r

This is used for adding curve triangles
The winding will be freed before it returns
====================
*/
void ClipTriIntoTree_r( idWinding* w, mapTri_t* originalTri, uEntity_t* e, node_t* node )
{
	idWinding*		front, *back;

	if( !w )
	{
		return;
	}

	if( node->planenum != PLANENUM_LEAF )
	{
		//common->Printf( "ClipTriIntoTree_r: splitting triangle with splitplane %i\n", node->nodeNumber );

		w->Split( dmapGlobals.mapPlanes[ node->planenum ], ON_EPSILON, &front, &back );
		delete w;

		ClipTriIntoTree_r( front, originalTri, e, node->children[0] );
		ClipTriIntoTree_r( back, originalTri, e, node->children[1] );

		return;
	}

	//common->Printf( "ClipTriIntoTree_r: leaf area = %i, opaque = %i, occupied = %i\n", node->area, node->occupied );

	// if opaque leaf, don't add
	if( !node->opaque && node->area >= 0 )
	{
		mapTri_t*	list;
		int			planeNum;
		idPlane		plane;
		textureVectors_t	texVec;

		list = WindingToTriList( w, originalTri );

		PlaneForTri( originalTri, plane );
		planeNum = FindFloatPlane( plane );

		TexVecForTri( &texVec, originalTri );

		AddTriListToArea( e, list, planeNum, node->area, &texVec );
	}

	delete w;
	return;
}





//=============================================================

/*
====================
CheckWindingInAreas_r

Returns the area number that the winding is in, or
-2 if it crosses multiple areas.

====================
*/
static int CheckWindingInAreas_r( const idWinding* w, node_t* node )
{
	idWinding*		front, *back;

	if( !w )
	{
		return -1;
	}

	if( node->planenum != PLANENUM_LEAF )
	{
		int		a1, a2;
#if 0
		if( side->planenum == node->planenum )
		{
			return CheckWindingInAreas_r( w, node->children[0] );
		}
		if( side->planenum == ( node->planenum ^ 1 ) )
		{
			return CheckWindingInAreas_r( w, node->children[1] );
		}
#endif
		w->Split( dmapGlobals.mapPlanes[ node->planenum ], ON_EPSILON, &front, &back );

		a1 = CheckWindingInAreas_r( front, node->children[0] );
		delete front;
		a2 = CheckWindingInAreas_r( back, node->children[1] );
		delete back;

		if( a1 == -2 || a2 == -2 )
		{
			return -2;	// different
		}
		if( a1 == -1 )
		{
			return a2;	// one solid
		}
		if( a2 == -1 )
		{
			return a1;	// one solid
		}

		if( a1 != a2 )
		{
			return -2;	// cross areas
		}
		return a1;
	}

	return node->area;
}



/*
====================
PutWindingIntoAreas_r

Clips a winding down into the bsp tree, then converts
the fragments to triangles and adds them to the area lists
====================
*/
static void PutWindingIntoAreas_r( uEntity_t* e, const idWinding* w, side_t* side, node_t* node )
{
	idWinding*		front, *back;
	int				area;

	if( !w )
	{
		return;
	}

	if( node->planenum != PLANENUM_LEAF )
	{
		if( side->planenum == node->planenum )
		{
			PutWindingIntoAreas_r( e, w, side, node->children[0] );
			return;
		}
		if( side->planenum == ( node->planenum ^ 1 ) )
		{
			PutWindingIntoAreas_r( e, w, side, node->children[1] );
			return;
		}

		// see if we need to split it
		// adding the "noFragment" flag to big surfaces like sky boxes
		// will avoid potentially dicing them up into tons of triangles
		// that take forever to optimize back together
		if( !dmapGlobals.fullCarve || side->material->NoFragment() )
		{
			area = CheckWindingInAreas_r( w, node );
			if( area >= 0 )
			{
				mapTri_t*	tri;

				// put in single area
				tri = TriListForSide( side, w );
				AddTriListToArea( e, tri, side->planenum, area, &side->texVec );
				return;
			}
		}

		w->Split( dmapGlobals.mapPlanes[ node->planenum ], ON_EPSILON, &front, &back );

		PutWindingIntoAreas_r( e, front, side, node->children[0] );
		if( front )
		{
			delete front;
		}

		PutWindingIntoAreas_r( e, back, side, node->children[1] );
		if( back )
		{
			delete back;
		}

		return;
	}

	//common->Printf( "PutWindingIntoAreas_r: leaf area = %i, opaque = %i\n", node->area, (int) node->opaque );

	// if opaque leaf, don't add
	if( node->area >= 0 && !node->opaque )
	{
		mapTri_t*	tri;

		tri = TriListForSide( side, w );
		AddTriListToArea( e, tri, side->planenum, node->area, &side->texVec );
	}
}

/*
==================
AddMapTriToAreas

Used for curves and inlined models
==================
*/
void AddMapTriToAreas( mapTri_t* tri, uEntity_t* e )
{
	int				area;
	idWinding*		w;

	// skip degenerate triangles from pinched curves
	if( MapTriArea( tri ) <= 0 )
	{
		return;
	}

	if( dmapGlobals.fullCarve )
	{
		// always fragment into areas
		w = WindingForTri( tri );
		ClipTriIntoTree_r( w, tri, e, e->tree->headnode );
		return;
	}

	w = WindingForTri( tri );
	area = CheckWindingInAreas_r( w, e->tree->headnode );
	delete w;
	if( area == -1 )
	{
		return;
	}
	if( area >= 0 )
	{
		mapTri_t*	newTri;
		idPlane		plane;
		int			planeNum;
		textureVectors_t	texVec;

		// put in single area
		newTri = CopyMapTri( tri );
		newTri->next = NULL;

		PlaneForTri( tri, plane );
		planeNum = FindFloatPlane( plane );

		TexVecForTri( &texVec, newTri );

		AddTriListToArea( e, newTri, planeNum, area, &texVec );
	}
	else
	{
		// fragment into areas
		w = WindingForTri( tri );
		ClipTriIntoTree_r( w, tri, e, e->tree->headnode );
	}
}

/*
=====================
PutPrimitivesInAreas
=====================
*/
void PutPrimitivesInAreas( uEntity_t* e )
{
	uBrush_t*		b;
	int				i;
	side_t*			side;
	primitive_t*		prim;
	mapTri_t*		tri;

	common->Printf( "----- PutPrimitivesInAreas -----\n" );

	// allocate space for surface chains for each area
	e->areas = ( uArea_t* )Mem_Alloc( e->numAreas * sizeof( e->areas[0] ), TAG_TOOLS );
	memset( e->areas, 0, e->numAreas * sizeof( e->areas[0] ) );

	// for each primitive, clip it to the non-solid leafs
	// and divide it into different areas
	for( prim = e->primitives ; prim ; prim = prim->next )
	{
		b = prim->brush;

		if( !b )
		{
			// add curve triangles
			for( tri = prim->tris ; tri ; tri = tri->next )
			{
				AddMapTriToAreas( tri, e );
			}

			// RB: add new polygon mesh
			for( tri = prim->bsptris ; tri ; tri = tri->next )
			{
#if 1
				// FIXME reverse vertex order for drawing
				idDrawVert tmp = tri->v[0];
				tri->v[0] = tri->v[2];
				tri->v[2] = tmp;
#endif

				AddMapTriToAreas( tri, e );
			}
			// RB end
			continue;
		}

		// clip in brush sides
		for( i = 0 ; i < b->numsides ; i++ )
		{
			side = &b->sides[i];
			if( !side->visibleHull )
			{
				continue;
			}
			PutWindingIntoAreas_r( e, side->visibleHull, side, e->tree->headnode );
		}
	}


	// optionally inline some of the func_static models
	if( dmapGlobals.entityNum == 0 )
	{
		bool inlineAll = dmapGlobals.uEntities[0].mapEntity->epairs.GetBool( "inlineAllStatics" );

		for( int eNum = 1 ; eNum < dmapGlobals.num_entities ; eNum++ )
		{
			uEntity_t* entity = &dmapGlobals.uEntities[eNum];
			const char* className = entity->mapEntity->epairs.GetString( "classname" );
			if( idStr::Icmp( className, "func_static" ) )
			{
				continue;
			}
			if( !entity->mapEntity->epairs.GetBool( "inline" ) && !inlineAll )
			{
				continue;
			}
			const char* modelName = entity->mapEntity->epairs.GetString( "model" );
			if( !modelName )
			{
				continue;
			}
			idRenderModel*	model = renderModelManager->FindModel( modelName );

			common->Printf( "inlining %s.\n", entity->mapEntity->epairs.GetString( "name" ) );

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
				for( int j = 0 ; j < tri->numIndexes ; j += 3 )
				{
					for( int k = 0 ; k < 3 ; k++ )
					{
						idVec3 v = tri->verts[tri->indexes[j + k]].xyz;

						mapTri.v[k].xyz = v * axis + origin;

						mapTri.v[k].SetNormal( tri->verts[tri->indexes[j + k]].GetNormal() * axis );
						mapTri.v[k].SetTexCoord( tri->verts[tri->indexes[j + k]].GetTexCoord() );
					}
					AddMapTriToAreas( &mapTri, e );
				}
			}
		}
	}
}

//============================================================================


// RB begin
int FilterMeshesIntoTree_r( idWinding* w, mapTri_t* originalTri, node_t* node )
{
	idWinding*		front, *back;
	int				c;

	if( !w )
	{
		return 0;
	}

	if( node->planenum == PLANENUM_LEAF )
	{
		// add it to the leaf list
		if( originalTri->material->GetContentFlags() & CONTENTS_AREAPORTAL )
		{
			mapTri_t* list = CopyMapTri( originalTri );
			list->next = NULL;

			node->areaPortalTris = MergeTriLists( node->areaPortalTris, list );
		}

		const MapPolygonMesh* mapMesh = originalTri->originalMapMesh;

		// classify the leaf by the structural brush
		if( mapMesh->IsOpaque() )
		{
			node->opaque = true;
		}

		delete w;
		return 1;
	}

	// split it by the node plane
	w->Split( dmapGlobals.mapPlanes[ node->planenum ], ON_EPSILON, &front, &back );
	delete w;

	c = 0;
	c += FilterMeshesIntoTree_r( front, originalTri, node->children[0] );
	c += FilterMeshesIntoTree_r( back, originalTri, node->children[1] );

	return c;
}


/*
=====================
FilterMeshesIntoTree

Mark the leafs as opaque and areaportals and put mesh
fragments in each leaf so portal surfaces can be matched
to materials
=====================
*/
void FilterMeshesIntoTree( uEntity_t* e )
{
	uBrush_t*		b;
	primitive_t*	prim;
	mapTri_t*		tri;
	int				r;
	int				c_unique, c_clusters;

	common->Printf( "----- FilterMeshesIntoTree -----\n" );

	c_unique = 0;
	c_clusters = 0;
	for( prim = e->primitives ; prim ; prim = prim->next )
	{
		b = prim->brush;

		if( !b )
		{
			// add BSP triangles
			for( tri = prim->bsptris ; tri ; tri = tri->next )
			{
				idWinding* w = WindingForTri( tri );

				c_unique++;
				r = FilterMeshesIntoTree_r( w, tri, e->tree->headnode );
				c_clusters += r;
			}
			continue;
		}
	}

	common->Printf( "%5i total BSP triangles\n", c_unique );
	common->Printf( "%5i cluster references\n", c_clusters );
}
// RB end

//============================================================================

/*
=================
ClipTriByLight

Carves a triangle by the frustom planes of a light, producing
a (possibly empty) list of triangles on the inside and outside.

The original triangle is not modified.

If no clipping is required, the result will be a copy of the original.

If clipping was required, the outside fragments will be planar clips, which
will benefit from re-optimization.
=================
*/
static void ClipTriByLight( const mapLight_t* light, const mapTri_t* tri,
							mapTri_t** in, mapTri_t** out )
{
	idWinding*	inside, *oldInside;
	idWinding*	outside[6];
	bool	hasOutside;
	int			i;

	*in = NULL;
	*out = NULL;

	// clip this winding to the light
	inside = WindingForTri( tri );
	hasOutside = false;
	for( i = 0 ; i < 6 ; i++ )
	{
		oldInside = inside;
		if( oldInside )
		{
			// RB begin
			oldInside->Split( light->frustumPlanes[i], 0, &outside[i], &inside );
			delete oldInside;
			// RB end
		}
		else
		{
			outside[i] = NULL;
		}
		if( outside[i] )
		{
			hasOutside = true;
		}
	}

	if( !inside )
	{
		// the entire winding is outside this light

		// free the clipped fragments
		for( i = 0 ; i < 6 ; i++ )
		{
			if( outside[i] )
			{
				delete outside[i];
			}
		}

		*out = CopyMapTri( tri );
		( *out )->next = NULL;

		return;
	}

	if( !hasOutside )
	{
		// the entire winding is inside this light

		// free the inside copy
		delete inside;

		*in = CopyMapTri( tri );
		( *in )->next = NULL;

		return;
	}

	// the winding is split
	*in = WindingToTriList( inside, tri );
	delete inside;

	// combine all the outside fragments
	for( i = 0 ; i < 6 ; i++ )
	{
		if( outside[i] )
		{
			mapTri_t*	list;

			list = WindingToTriList( outside[i], tri );
			delete outside[i];
			*out = MergeTriLists( *out, list );
		}
	}
}

/*
=================
BoundOptimizeGroup
=================
*/
static void BoundOptimizeGroup( optimizeGroup_t* group )
{
	group->bounds.Clear();
	for( mapTri_t* tri = group->triList ; tri ; tri = tri->next )
	{
		group->bounds.AddPoint( tri->v[0].xyz );
		group->bounds.AddPoint( tri->v[1].xyz );
		group->bounds.AddPoint( tri->v[2].xyz );
	}
}

/*
====================
CarveGroupsByLight

Divide each group into an inside group and an outside group, based
on which fragments are illuminated by the light's beam tree
====================
*/
static void CarveGroupsByLight( uEntity_t* e, mapLight_t* light )
{
	int			i;
	optimizeGroup_t*	group, *newGroup, *carvedGroups, *nextGroup;
	mapTri_t*	tri, *inside, *outside;
	uArea_t*		area;

	for( i = 0 ; i < e->numAreas ; i++ )
	{
		area = &e->areas[i];
		carvedGroups = NULL;

		// we will be either freeing or reassigning the groups as we go
		for( group = area->groups ; group ; group = nextGroup )
		{
			nextGroup = group->nextGroup;

			// if the surface doesn't get lit, don't carve it up
			if( ( light->def.lightShader->IsFogLight() && !group->material->ReceivesFog() )
					|| ( !light->def.lightShader->IsFogLight() && !group->material->ReceivesLighting() )
					|| !group->bounds.IntersectsBounds( light->def.globalLightBounds ) )
			{

				group->nextGroup = carvedGroups;
				carvedGroups = group;
				continue;
			}

			if( group->numGroupLights == MAX_GROUP_LIGHTS )
			{
				common->Error( "MAX_GROUP_LIGHTS around %f %f %f",
							   group->triList->v[0].xyz[0], group->triList->v[0].xyz[1], group->triList->v[0].xyz[2] );
			}

			// if the group doesn't face the light,
			// it won't get carved at all
			if( !light->def.lightShader->LightEffectsBackSides() &&
					!group->material->ReceivesLightingOnBackSides() &&
					dmapGlobals.mapPlanes[ group->planeNum ].Distance( light->def.parms.origin ) <= 0 )
			{

				group->nextGroup = carvedGroups;
				carvedGroups = group;
				continue;
			}

			// split into lists for hit-by-light, and not-hit-by-light
			inside = NULL;
			outside = NULL;

			for( tri = group->triList ; tri ; tri = tri->next )
			{
				mapTri_t*	in, *out;

				ClipTriByLight( light, tri, &in, &out );
				inside = MergeTriLists( inside, in );
				outside = MergeTriLists( outside, out );
			}

			if( inside )
			{
				newGroup = ( optimizeGroup_t* )Mem_Alloc( sizeof( *newGroup ), TAG_TOOLS );
				*newGroup = *group;
				newGroup->groupLights[newGroup->numGroupLights] = light;
				newGroup->numGroupLights++;
				newGroup->triList = inside;
				newGroup->nextGroup = carvedGroups;
				carvedGroups = newGroup;
			}

			if( outside )
			{
				newGroup = ( optimizeGroup_t* )Mem_Alloc( sizeof( *newGroup ), TAG_TOOLS );
				*newGroup = *group;
				newGroup->triList = outside;
				newGroup->nextGroup = carvedGroups;
				carvedGroups = newGroup;
			}

			// free the original
			group->nextGroup = NULL;
			FreeOptimizeGroupList( group );
		}

		// replace this area's group list with the new one
		area->groups = carvedGroups;
	}
}

/*
=====================
Prelight

Break optimize groups up into additional groups at light boundaries, so
optimization won't cross light bounds
=====================
*/
void Prelight( uEntity_t* e )
{
	int			i;
	int			start, end;
	mapLight_t*	light;

	// don't prelight anything but the world entity
	if( dmapGlobals.entityNum != 0 )
	{
		return;
	}

	if( dmapGlobals.shadowOptLevel > 0 )
	{
		common->Printf( "----- BuildLightShadows -----\n" );
		start = Sys_Milliseconds();

		// calc bounds for all the groups to speed things up
		for( i = 0 ; i < e->numAreas ; i++ )
		{
			uArea_t* area = &e->areas[i];

			for( optimizeGroup_t* group = area->groups ; group ; group = group->nextGroup )
			{
				BoundOptimizeGroup( group );
			}
		}

		end = Sys_Milliseconds();
		common->Printf( "%5.1f seconds for BuildLightShadows\n", ( end - start ) / 1000.0 );
	}


	if( !dmapGlobals.noLightCarve )
	{
		common->Printf( "----- CarveGroupsByLight -----\n" );
		start = Sys_Milliseconds();
		// now subdivide the optimize groups into additional groups for
		// each light that illuminates them
		for( i = 0 ; i < dmapGlobals.mapLights.Num() ; i++ )
		{
			light = dmapGlobals.mapLights[i];
			CarveGroupsByLight( e, light );
		}

		end = Sys_Milliseconds();
		common->Printf( "%5.1f seconds for CarveGroupsByLight\n", ( end - start ) / 1000.0 );
	}

}



