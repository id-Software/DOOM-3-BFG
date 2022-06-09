/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013-2021 Robert Beckebans

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

  After parsing, there will be a list of entities that each has
  a list of primitives.

  Primitives are either brushes, triangle soups, or model references.

  Curves are tesselated to triangle soups at load time, but model
  references are
  Brushes will have

	brushes, each of which has a side definition.

*/

//
// private declarations
//

#define MAX_BUILD_SIDES		300

static	int		entityPrimitive;		// to track editor brush numbers
static	int		c_numMapPatches;
static	int		c_areaportals;

static	uEntity_t*	uEntity;

// brushes are parsed into a temporary array of sides,
// which will have duplicates removed before the final brush is allocated
static	uBrush_t*	buildBrush;


#define	NORMAL_EPSILON			0.00001f
#define	DIST_EPSILON			0.01f


/*
===========
FindFloatPlane
===========
*/
int FindFloatPlane( const idPlane& plane, bool* fixedDegeneracies )
{
	idPlane p = plane;
	bool fixed = p.FixDegeneracies( DIST_EPSILON );
	if( fixed && fixedDegeneracies )
	{
		*fixedDegeneracies = true;
	}
	return dmapGlobals.mapPlanes.FindPlane( p, NORMAL_EPSILON, DIST_EPSILON );
}

/*
===========
SetBrushContents

The contents on all sides of a brush should be the same
Sets contentsShader, contents, opaque
===========
*/
static void SetBrushContents( uBrush_t* b )
{
	int			contents, c2;
	side_t*		s;
	int			i;
	bool	mixed;

	s = &b->sides[0];
	contents = s->material->GetContentFlags();

	b->contentShader = s->material;
	mixed = false;

	// a brush is only opaque if all sides are opaque
	b->opaque = true;

	for( i = 1 ; i < b->numsides ; i++, s++ )
	{
		s = &b->sides[i];

		if( !s->material )
		{
			continue;
		}

		c2 = s->material->GetContentFlags();
		if( c2 != contents )
		{
			mixed = true;
			contents |= c2;
		}

		if( s->material->Coverage() != MC_OPAQUE )
		{
			b->opaque = false;
		}
	}

	if( contents & CONTENTS_AREAPORTAL )
	{
		c_areaportals++;
	}

	b->contents = contents;
}


//============================================================================

/*
===============
FreeBuildBrush
===============
*/
static void FreeBuildBrush()
{
	int		i;

	for( i = 0 ; i < buildBrush->numsides ; i++ )
	{
		if( buildBrush->sides[i].winding )
		{
			delete buildBrush->sides[i].winding;
		}
	}
	buildBrush->numsides = 0;
}

/*
===============
FinishBrush

Produces a final brush based on the buildBrush->sides array
and links it to the current entity
===============
*/
static uBrush_t* FinishBrush()
{
	uBrush_t*	b;
	primitive_t*	prim;

	// create windings for sides and bounds for brush
	if( !CreateBrushWindings( buildBrush ) )
	{
		// don't keep this brush
		FreeBuildBrush();
		return NULL;
	}

	if( buildBrush->contents & CONTENTS_AREAPORTAL )
	{
		if( dmapGlobals.num_entities != 1 )
		{
			common->Printf( "Entity %i, Brush %i: areaportals only allowed in world\n"
							,  dmapGlobals.num_entities - 1, entityPrimitive );
			FreeBuildBrush();
			return NULL;
		}
	}

	// keep it
	b = CopyBrush( buildBrush );

	FreeBuildBrush();

	b->entitynum = dmapGlobals.num_entities - 1;
	b->brushnum = entityPrimitive;

	b->original = b;

	prim = ( primitive_t* )Mem_Alloc( sizeof( *prim ), TAG_TOOLS );
	memset( prim, 0, sizeof( *prim ) );
	prim->next = uEntity->primitives;
	uEntity->primitives = prim;

	prim->brush = b;

	return b;
}

/*
================
AdjustEntityForOrigin
================
*/
/*
static void AdjustEntityForOrigin( uEntity_t* ent )
{
	primitive_t*	prim;
	uBrush_t*	b;
	int			i;
	side_t*		s;

	for( prim = ent->primitives ; prim ; prim = prim->next )
	{
		b = prim->brush;
		if( !b )
		{
			continue;
		}
		for( i = 0; i < b->numsides; i++ )
		{
			idPlane plane;

			s = &b->sides[i];

			plane = dmapGlobals.mapPlanes[s->planenum];
			plane[3] += plane.Normal() * ent->origin;

			s->planenum = FindFloatPlane( plane );

			s->texVec.v[0][3] += DotProduct( ent->origin, s->texVec.v[0] );
			s->texVec.v[1][3] += DotProduct( ent->origin, s->texVec.v[1] );

			// remove any integral shift
			s->texVec.v[0][3] -= floor( s->texVec.v[0][3] );
			s->texVec.v[1][3] -= floor( s->texVec.v[1][3] );
		}
		CreateBrushWindings( b );
	}
}
*/

/*
=================
RemoveDuplicateBrushPlanes

Returns false if the brush has a mirrored set of planes,
meaning it encloses no volume.
Also removes planes without any normal
=================
*/
static bool RemoveDuplicateBrushPlanes( uBrush_t* b )
{
	int			i, j, k;
	side_t*		sides;

	sides = b->sides;

	for( i = 1 ; i < b->numsides ; i++ )
	{

		// check for a degenerate plane
		if( sides[i].planenum == -1 )
		{
			common->Printf( "Entity %i, Brush %i: degenerate plane\n"
							, b->entitynum, b->brushnum );
			// remove it
			for( k = i + 1 ; k < b->numsides ; k++ )
			{
				sides[k - 1] = sides[k];
			}
			b->numsides--;
			i--;
			continue;
		}

		// check for duplication and mirroring
		for( j = 0 ; j < i ; j++ )
		{
			if( sides[i].planenum == sides[j].planenum )
			{
				common->Printf( "Entity %i, Brush %i: duplicate plane\n"
								, b->entitynum, b->brushnum );
				// remove the second duplicate
				for( k = i + 1 ; k < b->numsides ; k++ )
				{
					sides[k - 1] = sides[k];
				}
				b->numsides--;
				i--;
				break;
			}

			if( sides[i].planenum == ( sides[j].planenum ^ 1 ) )
			{
				// mirror plane, brush is invalid
				common->Printf( "Entity %i, Brush %i: mirrored plane\n"
								, b->entitynum, b->brushnum );
				return false;
			}
		}
	}
	return true;
}


/*
=================
ParseBrush
=================
*/
static void ParseBrush( const idMapBrush* mapBrush, int primitiveNum )
{
	uBrush_t*	b;
	side_t*		s;
	const idMapBrushSide*	ms;
	int			i;
	bool		fixedDegeneracies = false;

	buildBrush->entitynum = dmapGlobals.num_entities - 1;
	buildBrush->brushnum = entityPrimitive;
	buildBrush->numsides = mapBrush->GetNumSides();
	for( i = 0 ; i < mapBrush->GetNumSides() ; i++ )
	{
		s = &buildBrush->sides[i];
		ms = mapBrush->GetSide( i );

		memset( s, 0, sizeof( *s ) );
		s->planenum = FindFloatPlane( ms->GetPlane(), &fixedDegeneracies );
		s->material = declManager->FindMaterial( ms->GetMaterial() );

		ms->GetTextureVectors( s->texVec.v );

		// RB: Valve 220 projection support
		s->texValve220 = ( ms->GetProjectionType() == idMapBrushSide::PROJECTION_VALVE220 );

		// RB: TODO
		s->texSize = ms->GetTextureSize();

		idImage* image = s->material->GetEditorImage();
		if( image != NULL )
		{
			s->texSize.x = image->GetUploadWidth();
			s->texSize.y = image->GetUploadHeight();
		}
		// RB end

		// remove any integral shift, which will help with grouping
		if( !s->texValve220 )
		{
			s->texVec.v[0][3] -= floor( s->texVec.v[0][3] );
			s->texVec.v[1][3] -= floor( s->texVec.v[1][3] );
		}
	}

	// if there are mirrored planes, the entire brush is invalid
	if( !RemoveDuplicateBrushPlanes( buildBrush ) )
	{
		return;
	}

	// get the content for the entire brush
	SetBrushContents( buildBrush );

	b = FinishBrush();
	if( !b )
	{
		return;
	}

	if( fixedDegeneracies && dmapGlobals.verboseentities )
	{
		common->Warning( "brush %d has degenerate plane equations", primitiveNum );
	}
}

/*
================
ParseSurface
================
*/
static void ParseSurface( const idMapPatch* patch, const idSurface* surface, const idMaterial* material )
{
	int				i;
	mapTri_t*		tri;
	primitive_t*		prim;

	prim = ( primitive_t* )Mem_Alloc( sizeof( *prim ), TAG_TOOLS );
	memset( prim, 0, sizeof( *prim ) );
	prim->next = uEntity->primitives;
	uEntity->primitives = prim;

	for( i = 0; i < surface->GetNumIndexes(); i += 3 )
	{
		tri = AllocTri();

		tri->v[0] = ( *surface )[surface->GetIndexes()[i + 1]];
		tri->v[1] = ( *surface )[surface->GetIndexes()[i + 2]];
		tri->v[2] = ( *surface )[surface->GetIndexes()[i + 0]];

		tri->material = material;
		tri->next = prim->tris;
		prim->tris = tri;
	}

	// set merge groups if needed, to prevent multiple sides from being
	// merged into a single surface in the case of gui shaders, mirrors, and autosprites
	if( material->IsDiscrete() )
	{
		for( tri = prim->tris ; tri ; tri = tri->next )
		{
			tri->mergeGroup = ( void* )patch;
		}
	}
}

/*
================
ParsePatch
================
*/
static void ParsePatch( const idMapPatch* patch, int primitiveNum )
{
	const idMaterial* mat;

	if( dmapGlobals.noCurves )
	{
		return;
	}

	c_numMapPatches++;

	mat = declManager->FindMaterial( patch->GetMaterial() );

	idSurface_Patch* cp = new idSurface_Patch( *patch );

	if( patch->GetExplicitlySubdivided() )
	{
		cp->SubdivideExplicit( patch->GetHorzSubdivisions(), patch->GetVertSubdivisions(), true );
	}
	else
	{
		cp->Subdivide( DEFAULT_CURVE_MAX_ERROR, DEFAULT_CURVE_MAX_ERROR, DEFAULT_CURVE_MAX_LENGTH, true );
	}

	ParseSurface( patch, cp, mat );

	delete cp;
}


// RB begin
static int ParsePolygonMesh( const MapPolygonMesh* mesh, int primitiveNum, int numPolygons )
{
	primitive_t* prim = ( primitive_t* )Mem_Alloc( sizeof( *prim ), TAG_TOOLS );
	memset( prim, 0, sizeof( *prim ) );
	prim->next = uEntity->primitives;
	uEntity->primitives = prim;

	const idList<idDrawVert>& verts = mesh->GetDrawVerts();

	for( int i = 0; i < mesh->GetNumPolygons(); i++ )
	{
		const MapPolygon& poly = mesh->GetFace( i );

		const idMaterial* mat = declManager->FindMaterial( poly.GetMaterial() );

		const idList<int>& indexes = poly.GetIndexes();

		//idList<int> unique;
		//for( int j = 0; j < indexes.Num(); j++ )
		//{
		//	unique.AddUnique( indexes[j] );
		//}

		// FIXME: avoid triangulization and use polygons

		// TODO use WindingToTriList instead ?

		if( indexes.Num() == 3 )
		{
			// RB: glTF2 workflow insists to use triangles instead of n-gons or quads
			mapTri_t* tri = AllocTri();

			tri->v[0] = verts[ indexes[ 2 ] ];
			tri->v[1] = verts[ indexes[ 1 ] ];
			tri->v[2] = verts[ indexes[ 0 ] ];

#if 0
			idLib::Printf( "indices: ( %i %i %i )\n", indexes[ 0 ], indexes[ 1 ], indexes[ 2 ] );

			idLib::Printf( "verts: ( %i %i %i ) ( %i %i %i ) ( %i %i %i )\n",
						   int( tri->v[0].xyz.x ), int( tri->v[0].xyz.y ), int( tri->v[0].xyz.z ),
						   int( tri->v[1].xyz.x ), int( tri->v[1].xyz.y ), int( tri->v[1].xyz.z ),
						   int( tri->v[2].xyz.x ), int( tri->v[2].xyz.y ), int( tri->v[2].xyz.z ) );
#endif

			idPlane plane;
			plane.FromPoints( tri->v[0].xyz, tri->v[1].xyz, tri->v[2].xyz );

			bool fixedDegeneracies = false;
			tri->planeNum = FindFloatPlane( plane, &fixedDegeneracies );

			tri->polygonId = numPolygons + i;

			tri->material = mat;
			tri->next = prim->bsptris;
			prim->bsptris = tri;

			tri->originalMapMesh = mesh;

			// set merge groups if needed, to prevent multiple sides from being
			// merged into a single surface in the case of gui shaders, mirrors, and autosprites
			if( mat->IsDiscrete() )
			{
				for( tri = prim->bsptris ; tri ; tri = tri->next )
				{
					tri->mergeGroup = ( void* )mesh;
				}
			}
		}
		else
		{
			for( int j = 1; j < indexes.Num() - 1; j++ )
			{
				mapTri_t* tri = AllocTri();

				tri->v[0] = verts[ indexes[ j + 1] ];
				tri->v[1] = verts[ indexes[ j + 0] ];
				tri->v[2] = verts[ indexes[ 0 ] ];

				idPlane plane;
				plane.FromPoints( tri->v[0].xyz, tri->v[1].xyz, tri->v[2].xyz );

				bool fixedDegeneracies = false;
				tri->planeNum = FindFloatPlane( plane, &fixedDegeneracies );

				tri->polygonId = numPolygons + i;

				tri->material = mat;
				tri->next = prim->bsptris;
				prim->bsptris = tri;

				tri->originalMapMesh = mesh;

				// set merge groups if needed, to prevent multiple sides from being
				// merged into a single surface in the case of gui shaders, mirrors, and autosprites
				if( mat->IsDiscrete() )
				{
					for( tri = prim->bsptris ; tri ; tri = tri->next )
					{
						tri->mergeGroup = ( void* )mesh;
					}
				}
			}
		}
	}

	return mesh->GetNumPolygons();
}
// RB end

/*
================
ProcessMapEntity
================
*/
static bool	ProcessMapEntity( idMapEntity* mapEnt )
{
	idMapPrimitive*	prim;

	uEntity = &dmapGlobals.uEntities[dmapGlobals.num_entities];
	memset( uEntity, 0, sizeof( *uEntity ) );
	uEntity->mapEntity = mapEnt;
	dmapGlobals.num_entities++;

	int numPolygons = 0;

	for( entityPrimitive = 0; entityPrimitive < mapEnt->GetNumPrimitives(); entityPrimitive++ )
	{
		prim = mapEnt->GetPrimitive( entityPrimitive );

		if( prim->GetType() == idMapPrimitive::TYPE_BRUSH )
		{
			ParseBrush( static_cast<idMapBrush*>( prim ), entityPrimitive );
		}
		else if( prim->GetType() == idMapPrimitive::TYPE_PATCH )
		{
			ParsePatch( static_cast<idMapPatch*>( prim ), entityPrimitive );
		}
		// RB begin
		else if( prim->GetType() == idMapPrimitive::TYPE_MESH )
		{
			numPolygons += ParsePolygonMesh( static_cast<MapPolygonMesh*>( prim ), entityPrimitive, numPolygons );
		}
		// RB end
	}

	// never put an origin on the world, even if the editor left one there
	if( dmapGlobals.num_entities != 1 )
	{
		uEntity->mapEntity->epairs.GetVector( "origin", "", uEntity->origin );
	}

	return true;
}

//===================================================================

/*
==============
CreateMapLight
==============
*/
static void CreateMapLight( const idMapEntity* mapEnt )
{
	mapLight_t*	light;
	bool	dynamic;

	// designers can add the "noPrelight" flag to signal that
	// the lights will move around, so we don't want
	// to bother chopping up the surfaces under it or creating
	// shadow volumes
	mapEnt->epairs.GetBool( "noPrelight", "0", dynamic );
	if( dynamic )
	{
		return;
	}

	light = new mapLight_t;

	// parse parms exactly as the game do
	// use the game's epair parsing code so
	// we can use the same renderLight generation
	gameEdit->ParseSpawnArgsToRenderLight( &mapEnt->epairs, &light->def.parms );

	R_DeriveLightData( &light->def );

	// RB begin
	idRenderMatrix::GetFrustumPlanes( light->frustumPlanes, light->def.baseLightProject, true, true );

	// the DOOM 3 frustum planes point outside the frustum
	for( int i = 0; i < 6; i++ )
	{
		light->frustumPlanes[i] = -light->frustumPlanes[i];
	}
	// RB end

#if 0
	// use the renderer code to get the bounding planes for the light
	// based on all the parameters
	R_RenderLightFrustum( light->parms, light->frustum );
	light->lightShader = light->parms.shader;
#endif

	dmapGlobals.mapLights.Append( light );

}

/*
==============
CreateMapLights

==============
*/
static void CreateMapLights( const idMapFile* dmapFile )
{
	int		i;
	const idMapEntity* mapEnt;
	const char*	value;

	for( i = 0 ; i < dmapFile->GetNumEntities() ; i++ )
	{
		mapEnt = dmapFile->GetEntity( i );
		mapEnt->epairs.GetString( "classname", "", &value );
		if( !idStr::Icmp( value, "light" ) )
		{
			CreateMapLight( mapEnt );
		}

	}

}

/*
================
LoadDMapFile
================
*/
bool LoadDMapFile( const char* filename )
{
	primitive_t*	prim;
	idBounds	mapBounds;
	int			brushes, triSurfs;
	int			i;
	int			size;

	common->Printf( "--- LoadDMapFile ---\n" );
	common->Printf( "loading %s\n", filename );

	// load and parse the map file into canonical form
	dmapGlobals.dmapFile = new idMapFile();
	if( !dmapGlobals.dmapFile->Parse( filename ) )
	{
		delete dmapGlobals.dmapFile;
		dmapGlobals.dmapFile = NULL;
		common->Warning( "Couldn't load map file: '%s'", filename );
		return false;
	}

	dmapGlobals.mapPlanes.Clear();
	dmapGlobals.mapPlanes.SetGranularity( 1024 );

	// process the canonical form into utility form
	dmapGlobals.num_entities = 0;
	c_numMapPatches = 0;
	c_areaportals = 0;

	size = dmapGlobals.dmapFile->GetNumEntities() * sizeof( dmapGlobals.uEntities[0] );
	dmapGlobals.uEntities = ( uEntity_t* )Mem_Alloc( size, TAG_TOOLS );
	memset( dmapGlobals.uEntities, 0, size );

	// allocate a very large temporary brush for building
	// the brushes as they are loaded
	buildBrush = AllocBrush( MAX_BUILD_SIDES );

	for( i = 0 ; i < dmapGlobals.dmapFile->GetNumEntities() ; i++ )
	{
		ProcessMapEntity( dmapGlobals.dmapFile->GetEntity( i ) );
	}

	CreateMapLights( dmapGlobals.dmapFile );

	brushes = 0;
	triSurfs = 0;

	mapBounds.Clear();
	for( prim = dmapGlobals.uEntities[0].primitives ; prim ; prim = prim->next )
	{
		if( prim->brush )
		{
			brushes++;
			mapBounds.AddBounds( prim->brush->bounds );
		}
		else if( prim->tris )
		{
			triSurfs++;
		}
		// RB begin
		else if( prim->bsptris )
		{
			for( mapTri_t* tri = prim->bsptris ; tri ; tri = tri->next )
			{
				mapBounds.AddPoint( tri->v[0].xyz );
				mapBounds.AddPoint( tri->v[1].xyz );
				mapBounds.AddPoint( tri->v[2].xyz );
			}

			triSurfs++;
		}
		// RB end
	}

	common->Printf( "%5i total world brushes\n", brushes );
	common->Printf( "%5i total world triSurfs\n", triSurfs );
	common->Printf( "%5i patches\n", c_numMapPatches );
	common->Printf( "%5i entities\n", dmapGlobals.num_entities );
	common->Printf( "%5i planes\n", dmapGlobals.mapPlanes.Num() );
	common->Printf( "%5i areaportals\n", c_areaportals );
	common->Printf( "size: %5.0f,%5.0f,%5.0f to %5.0f,%5.0f,%5.0f\n", mapBounds[0][0], mapBounds[0][1], mapBounds[0][2],
					mapBounds[1][0], mapBounds[1][1], mapBounds[1][2] );

	return true;
}

/*
================
FreeOptimizeGroupList
================
*/
void FreeOptimizeGroupList( optimizeGroup_t* groups )
{
	optimizeGroup_t*	next;

	for( ; groups ; groups = next )
	{
		next = groups->nextGroup;
		FreeTriList( groups->triList );
		Mem_Free( groups );
	}
}

/*
================
FreeDMapFile
================
*/
void FreeDMapFile()
{
	int		i, j;

	FreeBrush( buildBrush );
	buildBrush = NULL;

	// free the entities and brushes
	for( i = 0 ; i < dmapGlobals.num_entities ; i++ )
	{
		uEntity_t*	ent;
		primitive_t*	prim, *nextPrim;

		ent = &dmapGlobals.uEntities[i];

		FreeTree( ent->tree );

		// free primitives
		for( prim = ent->primitives ; prim ; prim = nextPrim )
		{
			nextPrim = prim->next;

			if( prim->brush )
			{
				FreeBrush( prim->brush );
			}

			if( prim->tris )
			{
				FreeTriList( prim->tris );
			}

			if( prim->bsptris )
			{
				FreeTriList( prim->bsptris );
			}

			Mem_Free( prim );
		}

		// free area surfaces
		if( ent->areas )
		{
			for( j = 0 ; j < ent->numAreas ; j++ )
			{
				uArea_t*	area;

				area = &ent->areas[j];
				FreeOptimizeGroupList( area->groups );

			}
			Mem_Free( ent->areas );
		}
	}

	Mem_Free( dmapGlobals.uEntities );

	dmapGlobals.num_entities = 0;

	// free the map lights
	for( i = 0; i < dmapGlobals.mapLights.Num(); i++ )
	{
		R_FreeLightDefDerivedData( &dmapGlobals.mapLights[i]->def );
	}
	dmapGlobals.mapLights.DeleteContents( true );
}
