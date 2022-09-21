/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013-2022 Robert Beckebans

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

//=================================================================================


#if 0

should we try and snap values very close to 0.5, 0.25, 0.125, etc ?

	do we write out normals, or just a "smooth shade" flag ?
		resolved : normals.  otherwise adjacent facet shaded surfaces get their
		vertexes merged, and they would have to be split apart before drawing

		do we save out "wings" for shadow silhouette info ?


#endif

			static int c_totalObjVerts = 0;

#define	AREANUM_DIFFERENT	-2
/*
=============
PruneNodes_r

Any nodes that have all children with the same
area can be combined into a single leaf node

Returns the area number of all children, or
AREANUM_DIFFERENT if not the same.
=============
*/
int	PruneNodes_r( node_t* node )
{
	int		a1, a2;

	if( node->planenum == PLANENUM_LEAF )
	{
		return node->area;
	}

	a1 = PruneNodes_r( node->children[0] );
	a2 = PruneNodes_r( node->children[1] );

	if( a1 != a2 || a1 == AREANUM_DIFFERENT )
	{
		return AREANUM_DIFFERENT;
	}

	// free all the nodes below this point
	FreeTreePortals_r( node->children[0] );
	FreeTreePortals_r( node->children[1] );
	FreeTree_r( node->children[0] );
	FreeTree_r( node->children[1] );

	// change this node to a leaf
	node->planenum = PLANENUM_LEAF;
	node->area = a1;

	return a1;
}

static void WriteFloat( idFile* f, float v )
{
	if( idMath::Fabs( v - idMath::Rint( v ) ) < 0.001 )
	{
		f->WriteFloatString( "%i ", ( int )idMath::Rint( v ) );
	}
	else
	{
		f->WriteFloatString( "%f ", v );
	}
}

void Write1DMatrix( idFile* f, int x, float* m )
{
	int		i;

	f->WriteFloatString( "( " );

	for( i = 0; i < x; i++ )
	{
		WriteFloat( f, m[i] );
	}

	f->WriteFloatString( ") " );
}

static int CountUniqueShaders( optimizeGroup_t* groups )
{
	optimizeGroup_t*		a, *b;
	int					count;

	count = 0;

	for( a = groups ; a ; a = a->nextGroup )
	{
		if( !a->triList )  	// ignore groups with no tris
		{
			continue;
		}
		for( b = groups ; b != a ; b = b->nextGroup )
		{
			if( !b->triList )
			{
				continue;
			}
			if( a->material != b->material )
			{
				continue;
			}
			if( a->mergeGroup != b->mergeGroup )
			{
				continue;
			}
			break;
		}
		if( a == b )
		{
			count++;
		}
	}

	return count;
}


/*
==============
MatchVert
==============
*/
#define	XYZ_EPSILON	0.01
#define	ST_EPSILON	0.001
#define	COSINE_EPSILON	0.999

static bool MatchVert( const idDrawVert* a, const idDrawVert* b )
{
	if( idMath::Fabs( a->xyz[0] - b->xyz[0] ) > XYZ_EPSILON )
	{
		return false;
	}
	if( idMath::Fabs( a->xyz[1] - b->xyz[1] ) > XYZ_EPSILON )
	{
		return false;
	}
	if( idMath::Fabs( a->xyz[2] - b->xyz[2] ) > XYZ_EPSILON )
	{
		return false;
	}

	if( idMath::Fabs( a->GetTexCoordS() - b->GetTexCoordS() ) > ST_EPSILON )
	{
		return false;
	}
	if( idMath::Fabs( a->GetTexCoordT() - b->GetTexCoordT() ) > ST_EPSILON )
	{
		return false;
	}

	// RB begin
	// if the normal is 0 (smoothed normals), consider it a match
	if( a->GetNormal().Length() == 0 && b->GetNormal().Length() == 0 )
	{
		return true;
	}

	// otherwise do a dot-product cosine check
	if( ( a->GetNormal() * b->GetNormal() ) < COSINE_EPSILON )
	{
		return false;
	}
	// RB end

	return true;
}

/*
====================
ShareMapTriVerts

Converts independent triangles to shared vertex triangles
====================
*/
srfTriangles_t*	ShareMapTriVerts( const mapTri_t* tris )
{
	const mapTri_t*	step;
	int			count;
	int			i, j;
	int			numVerts;
	int			numIndexes;
	srfTriangles_t*	uTri;

	// unique the vertexes
	count = CountTriList( tris );

	uTri = R_AllocStaticTriSurf();
	R_AllocStaticTriSurfVerts( uTri, count * 3 );
	R_AllocStaticTriSurfIndexes( uTri, count * 3 );

	numVerts = 0;
	numIndexes = 0;

	for( step = tris ; step ; step = step->next )
	{
		for( i = 0 ; i < 3 ; i++ )
		{
			const idDrawVert*	dv;

			dv = &step->v[i];

			// search for a match
			for( j = 0 ; j < numVerts ; j++ )
			{
				if( MatchVert( &uTri->verts[j], dv ) )
				{
					break;
				}
			}

			if( j == numVerts )
			{
				numVerts++;
				uTri->verts[j].xyz = dv->xyz;
				//uTri->verts[j].SetNormal( dv->normal[0], dv->normal[1], dv->normal[2] );
				uTri->verts[j].SetNormal( dv->GetNormal() );
				uTri->verts[j].SetTexCoordS( dv->GetTexCoordS() );
				uTri->verts[j].SetTexCoordT( dv->GetTexCoordT() );
			}

			uTri->indexes[numIndexes++] = j;
		}
	}

	uTri->numVerts = numVerts;
	uTri->numIndexes = numIndexes;

	return uTri;
}

/*
==================
CleanupUTriangles
==================
*/
static void CleanupUTriangles( srfTriangles_t* tri )
{
	// perform cleanup operations

	R_RangeCheckIndexes( tri );
	R_CreateSilIndexes( tri );
//	R_RemoveDuplicatedTriangles( tri );	// this may remove valid overlapped transparent triangles
	R_RemoveDegenerateTriangles( tri );
//	R_RemoveUnusedVerts( tri );

	R_FreeStaticTriSurfSilIndexes( tri );
}

/*
====================
WriteUTriangles

Writes text verts and indexes to procfile
====================
*/
static void WriteUTriangles( idFile* procFile, const srfTriangles_t* uTris, const idVec3& offsetOrigin = vec3_origin )
{
	int			col;
	int			i;

	// emit this chain
	procFile->WriteFloatString( "/* numVerts = */ %i /* numIndexes = */ %i\n",
								uTris->numVerts, uTris->numIndexes );

	// verts
	col = 0;
	for( i = 0 ; i < uTris->numVerts ; i++ )
	{
		float	vec[8];
		const idDrawVert* dv;

		dv = &uTris->verts[i];

		vec[0] = dv->xyz[0] - offsetOrigin.x;
		vec[1] = dv->xyz[1] - offsetOrigin.y;
		vec[2] = dv->xyz[2] - offsetOrigin.z;

		idVec2 st = dv->GetTexCoord();
		vec[3] = st.x;
		vec[4] = st.y;

		idVec3 normal = dv->GetNormal();
		vec[5] = normal.x;
		vec[6] = normal.y;
		vec[7] = normal.z;

		Write1DMatrix( procFile, 8, vec );

		if( ++col == 3 )
		{
			col = 0;
			procFile->WriteFloatString( "\n" );
		}
	}
	if( col != 0 )
	{
		procFile->WriteFloatString( "\n" );
	}

	// indexes
	col = 0;
	for( i = 0 ; i < uTris->numIndexes ; i++ )
	{
		procFile->WriteFloatString( "%i ", uTris->indexes[i] );

		if( ++col == 18 )
		{
			col = 0;
			procFile->WriteFloatString( "\n" );
		}
	}
	if( col != 0 )
	{
		procFile->WriteFloatString( "\n" );
	}
}

// RB begin
static void WriteObjTriangles( idFile* objFile, const srfTriangles_t* uTris, const idVec3& offsetOrigin, const char* materialName )
{
	int			col;
	int			i;

	// emit this chain
	//procFile->WriteFloatString( "/* numVerts = */ %i /* numIndexes = */ %i\n",
	//							uTris->numVerts, uTris->numIndexes );

	// verts
	col = 0;
	for( i = 0 ; i < uTris->numVerts ; i++ )
	{
		float	vec[8];
		const idDrawVert* dv;

		dv = &uTris->verts[i];

		vec[0] = dv->xyz[0] - offsetOrigin.x;
		vec[1] = dv->xyz[1] - offsetOrigin.y;
		vec[2] = dv->xyz[2] - offsetOrigin.z;

		idVec2 st = dv->GetTexCoord();
		vec[3] = st.x;
		vec[4] = st.y;

		idVec3 normal = dv->GetNormal();
		vec[5] = normal.x;
		vec[6] = normal.y;
		vec[7] = normal.z;

		//Write1DMatrix( procFile, 8, vec );
		objFile->Printf( "v %1.6f %1.6f %1.6f\n", vec[0], vec[1], vec[2] );
	}

	// indexes

	// flip order for OBJ
	for( int j = 0; j < uTris->numIndexes; j += 3 )
	{
		objFile->Printf( "f %i/%i/%i %i/%i/%i %i/%i/%i\n",
						 uTris->indexes[j + 2] + 1 + c_totalObjVerts,
						 uTris->indexes[j + 2] + 1 + c_totalObjVerts,
						 uTris->indexes[j + 2] + 1 + c_totalObjVerts,

						 uTris->indexes[j + 1] + 1 + c_totalObjVerts,
						 uTris->indexes[j + 1] + 1 + c_totalObjVerts,
						 uTris->indexes[j + 1] + 1 + c_totalObjVerts,

						 uTris->indexes[j + 0] + 1 + c_totalObjVerts,
						 uTris->indexes[j + 0] + 1 + c_totalObjVerts,
						 uTris->indexes[j + 0] + 1 + c_totalObjVerts );
	}

	c_totalObjVerts += uTris->numVerts;

	objFile->Printf( "\n\n" );
}
// RB end


/*
=======================
GroupsAreSurfaceCompatible

Planes, texcoords, and groupLights can differ,
but the material and mergegroup must match
=======================
*/
static bool GroupsAreSurfaceCompatible( const optimizeGroup_t* a, const optimizeGroup_t* b )
{
	if( a->material != b->material )
	{
		return false;
	}
	if( a->mergeGroup != b->mergeGroup )
	{
		return false;
	}
	return true;
}

/*
====================
WriteOutputSurfaces
====================
*/
static void WriteOutputSurfaces( int entityNum, int areaNum, idFile* procFile, idFile* objFile )
{
	mapTri_t*	ambient, *copy;
	int			surfaceNum;
	int			numSurfaces;
	idMapEntity*	entity;
	uArea_t*		area;
	optimizeGroup_t*	group, *groupStep;
	int			i; // , j;
//	int			col;
	srfTriangles_t*	uTri;
//	mapTri_t	*tri;
	typedef struct interactionTris_s
	{
		struct interactionTris_s*	next;
		mapTri_t*	triList;
		mapLight_t*	light;
	} interactionTris_t;

	interactionTris_t*	interactions, *checkInter; //, *nextInter;


	area = &dmapGlobals.uEntities[entityNum].areas[areaNum];
	entity = dmapGlobals.uEntities[entityNum].mapEntity;

	numSurfaces = CountUniqueShaders( area->groups );


	if( entityNum == 0 )
	{
		procFile->WriteFloatString( "model { /* name = */ \"_area%i\" /* numSurfaces = */ %i\n\n",
									areaNum, numSurfaces );

		if( objFile )
		{
			objFile->Printf( "g _area%i\n", areaNum );
		}
	}
	else
	{
		const char* name;

		entity->epairs.GetString( "name", "", &name );
		if( !name[0] )
		{
			fileSystem->CloseFile( procFile );
			common->Error( "Entity %i has surfaces, but no name key", entityNum );
		}

		procFile->WriteFloatString( "model { /* name = */ \"%s\" /* numSurfaces = */ %i\n\n",
									name, numSurfaces );

		if( objFile )
		{
			objFile->Printf( "g model_%s\n", name );
		}
	}

	surfaceNum = 0;
	for( group = area->groups ; group ; group = group->nextGroup )
	{
		if( group->surfaceEmited )
		{
			continue;
		}

		// combine all groups compatible with this one
		// usually several optimizeGroup_t can be combined into a single
		// surface, even though they couldn't be merged together to save
		// vertexes because they had different planes, texture coordinates, or lights.
		// Different mergeGroups will stay in separate surfaces.
		ambient = NULL;

		// each light that illuminates any of the groups in the surface will
		// get its own list of indexes out of the original surface
		interactions = NULL;

		for( groupStep = group ; groupStep ; groupStep = groupStep->nextGroup )
		{
			if( groupStep->surfaceEmited )
			{
				continue;
			}
			if( !GroupsAreSurfaceCompatible( group, groupStep ) )
			{
				continue;
			}

			// copy it out to the ambient list
			copy = CopyTriList( groupStep->triList );
			ambient = MergeTriLists( ambient, copy );
			groupStep->surfaceEmited = true;

			// duplicate it into an interaction for each groupLight
			for( i = 0 ; i < groupStep->numGroupLights ; i++ )
			{
				for( checkInter = interactions ; checkInter ; checkInter = checkInter->next )
				{
					if( checkInter->light == groupStep->groupLights[i] )
					{
						break;
					}
				}
				if( !checkInter )
				{
					// create a new interaction
					checkInter = ( interactionTris_t* )Mem_ClearedAlloc( sizeof( *checkInter ), TAG_TOOLS );
					checkInter->light = groupStep->groupLights[i];
					checkInter->next = interactions;
					interactions = checkInter;
				}
				copy = CopyTriList( groupStep->triList );
				checkInter->triList = MergeTriLists( checkInter->triList, copy );
			}
		}

		if( !ambient )
		{
			continue;
		}

		if( surfaceNum >= numSurfaces )
		{
			fileSystem->CloseFile( procFile );
			common->Error( "WriteOutputSurfaces: surfaceNum >= numSurfaces" );
		}

		procFile->WriteFloatString( "/* surface %i */ { ", surfaceNum );
		surfaceNum++;
		procFile->WriteFloatString( "\"%s\" ", ambient->material->GetName() );

		uTri = ShareMapTriVerts( ambient );
		idStrStatic<256> matName( ambient->material->GetName() );
		FreeTriList( ambient );

		CleanupUTriangles( uTri );
		WriteUTriangles( procFile, uTri, entity->originOffset );

		// RB
		if( objFile )
		{
			WriteObjTriangles( objFile, uTri, entity->originOffset, matName.c_str() );
		}



		R_FreeStaticTriSurf( uTri );

		procFile->WriteFloatString( "}\n\n" );
	}

	procFile->WriteFloatString( "}\n\n" );
}

/*
===============
WriteNode_r

===============
*/
static void WriteNode_r( node_t* node, idFile* procFile )
{
	int		child[2];
	int		i;
	idPlane*	plane;

	if( node->planenum == PLANENUM_LEAF )
	{
		// we shouldn't get here unless the entire world
		// was a single leaf
		procFile->WriteFloatString( "/* node 0 */ ( 0 0 0 0 ) -1 -1\n" );
		return;
	}

	for( i = 0 ; i < 2 ; i++ )
	{
		if( node->children[i]->planenum == PLANENUM_LEAF )
		{
			child[i] = -1 - node->children[i]->area;
		}
		else
		{
			child[i] = node->children[i]->nodeNumber;
		}
	}

	plane = &dmapGlobals.mapPlanes[node->planenum];

	procFile->WriteFloatString( "/* node %i */ ", node->nodeNumber );
	Write1DMatrix( procFile, 4, plane->ToFloatPtr() );
	procFile->WriteFloatString( "%i %i\n", child[0], child[1] );

	if( child[0] > 0 )
	{
		WriteNode_r( node->children[0], procFile );
	}
	if( child[1] > 0 )
	{
		WriteNode_r( node->children[1], procFile );
	}
}

int NumberNodes_r( node_t* node, int nextNumber )
{
	if( node->planenum == PLANENUM_LEAF )
	{
		return nextNumber;
	}
	node->nodeNumber = nextNumber;
	nextNumber++;
	nextNumber = NumberNodes_r( node->children[0], nextNumber );
	nextNumber = NumberNodes_r( node->children[1], nextNumber );

	return nextNumber;
}

// RB begin
// https://stackoverflow.com/questions/801740/c-how-to-draw-a-binary-tree-to-the-console
//
static int WriteASCIIArtNode_r( node_t* node, bool is_left, int offset, int depth, char s[20][2048], idFile* procFile )
{
	char b[20];
	int width = 5;

	if( node->planenum == PLANENUM_LEAF )
	{
		int val = -1 - node->area;

		if( val == 0 )
		{
			// is in solid / touches the outside void
			idStr::snPrintf( b, 20, "(666)", val );
		}
		else
		{
			// leaf is area
			idStr::snPrintf( b, 20, "(A%02d)", val );
		}
	}
	else
	{
		int val = node->nodeNumber;
		idStr::snPrintf( b, 20, "(%03d)", val );
	}

	int left = 0;
	int right = 0;

	if( node->planenum != PLANENUM_LEAF )
	{
		/*
		int		child[2];

		for( int i = 0 ; i < 2 ; i++ )
		{
			if( node->children[i]->planenum == PLANENUM_LEAF )
			{
				child[i] = -1 - node->children[i]->area;
			}
			else
			{
				child[i] = node->children[i]->nodeNumber;
			}
		}
		*/

		if( depth < 19 )
		{
			//if( child[0] > 0 )
			{
				left = WriteASCIIArtNode_r( node->children[0], true, offset, depth + 1, s, procFile );
			}

			//if( child[1] > 0 )
			{
				right = WriteASCIIArtNode_r( node->children[1], false, offset + left + width, depth + 1, s, procFile );
			}
		}
	}

	for( int i = 0; i < width; i++ )
	{
		s[depth][offset + left + i] = b[i];
	}

	if( depth && is_left )
	{
		for( int i = 0; i < width + right; i++ )
		{
			s[depth - 1][offset + left + width / 2 + i] = '-';
		}

		s[depth - 1][offset + left + width / 2] = '.';
	}
	else if( depth && !is_left )
	{
		for( int i = 0; i < left + width; i++ )
		{
			s[depth - 1][offset - width / 2 + i] = '-';
		}

		s[depth - 1][offset + left + width / 2] = '.';
	}

	return left + width + right;
}

static void WriteVisualBSPTree( node_t* node, idFile* procFile )
{
	// TODO calculuate depth instead of assuming 20

	int s_len = 20;
	char s[20][2048];

	// output
	procFile->WriteFloatString( "/* BSP tree visualization:\n\n" );

	for( int i = 0; i < 20; i++ )
	{
		idStr::snPrintf( s[i], 2048, "%640s", " " );
	}

	WriteASCIIArtNode_r( node, 0, 0, 0, s, procFile );

	for( int i = 0; i < 20; i++ )
	{
		procFile->WriteFloatString( "%s\n", s[i] );
	}

	procFile->WriteFloatString( "*/\n\n" );
}
// RB end

/*
====================
WriteOutputNodes
====================
*/
static void WriteOutputNodes( node_t* node, idFile* procFile )
{
	int		numNodes;

	// prune unneeded nodes and count
	PruneNodes_r( node );
	numNodes = NumberNodes_r( node, 0 );

	// output
	procFile->WriteFloatString( "nodes { /* numNodes = */ %i\n\n", numNodes );
	procFile->WriteFloatString( "/* node format is: ( planeVector ) positiveChild negativeChild */\n" );
	procFile->WriteFloatString( "/* a child number of 0 is an opaque, solid area */\n" );
	procFile->WriteFloatString( "/* negative child numbers are areas: (-1-child) */\n" );

	// RB: draw an extra ASCII BSP tree visualization for YouTube tutorial
	if( dmapGlobals.glview )
	{
		WriteVisualBSPTree( node, procFile );
	}
	// RB end

	WriteNode_r( node, procFile );

	procFile->WriteFloatString( "}\n\n" );
}

/*
====================
WriteOutputPortals
====================
*/
static void WriteOutputPortals( uEntity_t* e, idFile* procFile )
{
	int			i, j;
	interAreaPortal_t*	iap;
	idWinding*			w;

	procFile->WriteFloatString( "interAreaPortals { /* numAreas = */ %i /* numIAP = */ %i\n\n",
								e->numAreas, interAreaPortals.Num() );
	procFile->WriteFloatString( "/* interAreaPortal format is: numPoints positiveSideArea negativeSideArea ( point) ... */\n" );
	for( i = 0 ; i < interAreaPortals.Num() ; i++ )
	{
		iap = &interAreaPortals[i];

		// RB: support new area portals
		if( iap->side )
		{
			w = iap->side->winding;
		}
		else
		{
			w = & iap->w;
		}
		// RB end

		procFile->WriteFloatString( "/* iap %i */ %i %i %i ", i, w->GetNumPoints(), iap->area0, iap->area1 );
		for( j = 0 ; j < w->GetNumPoints() ; j++ )
		{
			Write1DMatrix( procFile, 3, ( *w )[j].ToFloatPtr() );
		}
		procFile->WriteFloatString( "\n" );
	}

	procFile->WriteFloatString( "}\n\n" );
}


/*
====================
WriteOutputEntity
====================
*/
static void WriteOutputEntity( int entityNum, idFile* procFile, idFile* objFile )
{
	int		i;
	uEntity_t* e;

	e = &dmapGlobals.uEntities[entityNum];

	if( entityNum != 0 )
	{
		// entities may have enclosed, empty areas that we don't need to write out
		if( e->numAreas > 1 )
		{
			e->numAreas = 1;
		}
	}

	for( i = 0 ; i < e->numAreas ; i++ )
	{
		WriteOutputSurfaces( entityNum, i, procFile, objFile );
	}

	// we will completely skip the portals and nodes if it is a single area
	if( entityNum == 0 && e->numAreas > 1 )
	{
		// output the area portals
		WriteOutputPortals( e, procFile );

		// output the nodes
		WriteOutputNodes( e->tree->headnode, procFile );
	}
}


/*
====================
WriteOutputFile
====================
*/
void WriteOutputFile()
{
	int				i;
	uEntity_t*		entity;

	// write the file
	common->Printf( "----- WriteOutputFile -----\n" );

	idStrStatic< MAX_OSPATH > qpath;
	qpath.Format( "%s." PROC_FILE_EXT, dmapGlobals.mapFileBase );
	common->Printf( "writing %s\n", qpath.c_str() );
	idFileLocal procFile = fileSystem->OpenFileWrite( qpath, "fs_basepath" );
	if( !procFile )
	{
		common->Error( "Error opening %s", qpath.c_str() );
	}

	// RB: write area models as OBJ file
	idFile* objFile = NULL;
	if( dmapGlobals.glview )
	{
		idStrStatic< MAX_OSPATH > qpath;
		qpath.Format( "%s.obj", dmapGlobals.mapFileBase );
		objFile = fileSystem->OpenFileWrite( qpath, "fs_basepath" );
		if( !objFile )
		{
			common->Error( "Error opening %s", qpath.c_str() );
		}

		c_totalObjVerts = 0;
	}

	procFile->WriteFloatString( "%s\n\n", PROC_FILE_ID );

	// write the entity models and information, writing entities first
	for( i = dmapGlobals.num_entities - 1 ; i >= 0 ; i-- )
	{
		entity = &dmapGlobals.uEntities[i];

		if( !entity->primitives )
		{
			continue;
		}

		WriteOutputEntity( i, procFile, objFile );
	}

	if( objFile )
	{
		delete objFile;
	}
}
