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


struct OBJFace
{
	OBJFace()
	{
		material = nullptr;
	}

	const idMaterial*			material;
	idList<idDrawVert>			verts;
	idList<triIndex_t>			indexes;
};

struct OBJGroup
{
	idStr						name;
	//idList<OBJObject>			objects;
	idList<OBJFace>				faces;
};

int PortalVisibleSides( uPortal_t* p )
{
	int		fcon, bcon;

	if( !p->onnode )
	{
		return 0;    // outside
	}

	fcon = p->nodes[0]->opaque;
	bcon = p->nodes[1]->opaque;

	// same contents never create a face
	if( fcon == bcon )
	{
		return 0;
	}

	if( !fcon )
	{
		return 1;
	}
	if( !bcon )
	{
		return 2;
	}

	return 0;
}

void OutputWinding( idWinding* w, OBJGroup& group )
{
	static	int	level = 128;
	float		light;
	int			i;

	level += 28;
	light = ( level & 255 ) / 255.0;

	OBJFace& face = group.faces.Alloc();

	//for( i = 0; i < w->GetNumPoints(); i++ )

	for( i = w->GetNumPoints() - 1; i >= 0; i-- )
	{
		idDrawVert& dv = face.verts.Alloc();

		dv.xyz.x = ( *w )[i][0];
		dv.xyz.y = ( *w )[i][1];
		dv.xyz.z = ( *w )[i][2];

		dv.SetColor( level & 255 );

		//dv.SetNormal( w->GetPlane() )
	}
}

/*
void OutputWinding( const idFixedWinding& w, OBJGroup& group )
{
	static	int	level = 128;
	float		light;
	int			i;

	level += 28;
	light = ( level & 255 ) / 255.0;

	OBJFace& face = group.faces.Alloc();

	//for( i = 0; i < w->GetNumPoints(); i++ )

	for( i = w.GetNumPoints() - 1; i >= 0; i-- )
	{
		idDrawVert& dv = face.verts.Alloc();

		dv.xyz.x = ( w )[i][0];
		dv.xyz.y = ( w )[i][1];
		dv.xyz.z = ( w )[i][2];

		dv.SetColor( level & 255 );

		//dv.SetNormal( w->GetPlane() )
	}
}
*/

/*
=============
OutputPortal
=============
*/
void OutputPortal( uPortal_t* p, OBJGroup& group )
{
	idWinding*	w;
	int		sides;

	sides = PortalVisibleSides( p );
	if( !sides )
	{
		return;
	}

	//c_glfaces++;

	w = p->winding;

	if( sides == 2 )  		// back side
	{
		w = w->Reverse();
	}

	OutputWinding( w, group );

	if( sides == 2 )
	{
		delete w;
	}
}



void CollectPortals_r( node_t* node, OBJGroup& group )
{
	uPortal_t*	p, *nextp;

	if( node->planenum != PLANENUM_LEAF )
	{
		CollectPortals_r( node->children[0], group );
		CollectPortals_r( node->children[1], group );
		return;
	}

	// write all the portals
	for( p = node->portals; p; p = nextp )
	{
		if( p->nodes[0] == node )
		{
			OutputPortal( p, group );
			nextp = p->next[0];
		}
		else
		{
			nextp = p->next[1];
		}
	}
}


void OutputQuad( const idVec3 verts[4], OBJGroup* group, bool reverse )
{
	OBJFace& face = group->faces.Alloc();


	idDrawVert& dv0 = face.verts.Alloc();
	idDrawVert& dv1 = face.verts.Alloc();
	idDrawVert& dv2 = face.verts.Alloc();
	idDrawVert& dv3 = face.verts.Alloc();

	if( reverse )
	{
		dv0.xyz = verts[3];
		dv1.xyz = verts[2];
		dv2.xyz = verts[1];
		dv3.xyz = verts[0];
	}
	else
	{
		dv0.xyz = verts[0];
		dv1.xyz = verts[1];
		dv2.xyz = verts[2];
		dv3.xyz = verts[3];
	}
}

void OutputNode( const node_t* node, idList<OBJGroup>& groups )
{
	const idBounds& bounds = node->bounds;

	if( bounds.IsCleared() )
	{
		return;
	}

	OBJGroup* group = NULL;
	bool reverse = false;

	if( node->planenum == PLANENUM_LEAF )
	{
		if( node->occupied )
		{
			group = &groups.Alloc();
			group->name.Format( "area%i_leaf_occupied.%i", node->area, node->nodeNumber );
		}
		else if( node->opaque )
		{
			group = &groups.Alloc();

			if( node->area != -1 )
			{
				group->name.Format( "area%i_leaf_opaque.%i", node->area, node->nodeNumber );
			}
			else
			{
				group->name.Format( "void_leaf_opaque.%i", node->nodeNumber );
			}

			reverse = true;
		}
		else
		{
			group = &groups.Alloc();

			if( node->area != -1 )
			{
				group->name.Format( "area%i_leaf.%i", node->area, node->nodeNumber );
			}
			else
			{
				group->name.Format( "void_leaf.%i", node->nodeNumber );
			}
		}
	}
	else
	{
		//group = &groups.Alloc();
		//group->name.Format( "node.%i", node->nodeNumber ) ;
	}

	if( group )
	{
		idVec3 verts[4];

		verts[0].Set( bounds[0][0], bounds[0][1], bounds[0][2] );
		verts[1].Set( bounds[0][0], bounds[1][1], bounds[0][2] );
		verts[2].Set( bounds[0][0], bounds[1][1], bounds[1][2] );
		verts[3].Set( bounds[0][0], bounds[0][1], bounds[1][2] );
		OutputQuad( verts, group, reverse );

		verts[0].Set( bounds[1][0], bounds[0][1], bounds[1][2] );
		verts[1].Set( bounds[1][0], bounds[1][1], bounds[1][2] );
		verts[2].Set( bounds[1][0], bounds[1][1], bounds[0][2] );
		verts[3].Set( bounds[1][0], bounds[0][1], bounds[0][2] );
		OutputQuad( verts, group, reverse );

		verts[0].Set( bounds[0][0], bounds[0][1], bounds[1][2] );
		verts[1].Set( bounds[0][0], bounds[1][1], bounds[1][2] );
		verts[2].Set( bounds[1][0], bounds[1][1], bounds[1][2] );
		verts[3].Set( bounds[1][0], bounds[0][1], bounds[1][2] );
		OutputQuad( verts, group, reverse );

		verts[0].Set( bounds[1][0], bounds[0][1], bounds[0][2] );
		verts[1].Set( bounds[1][0], bounds[1][1], bounds[0][2] );
		verts[2].Set( bounds[0][0], bounds[1][1], bounds[0][2] );
		verts[3].Set( bounds[0][0], bounds[0][1], bounds[0][2] );
		OutputQuad( verts, group, reverse );

		verts[0].Set( bounds[0][0], bounds[0][1], bounds[0][2] );
		verts[1].Set( bounds[0][0], bounds[0][1], bounds[1][2] );
		verts[2].Set( bounds[1][0], bounds[0][1], bounds[1][2] );
		verts[3].Set( bounds[1][0], bounds[0][1], bounds[0][2] );
		OutputQuad( verts, group, reverse );

		verts[0].Set( bounds[1][0], bounds[1][1], bounds[0][2] );
		verts[1].Set( bounds[1][0], bounds[1][1], bounds[1][2] );
		verts[2].Set( bounds[0][0], bounds[1][1], bounds[1][2] );
		verts[3].Set( bounds[0][0], bounds[1][1], bounds[0][2] );
		OutputQuad( verts, group, reverse );
	}
}

void OutputSplitPlane( const node_t* node, idList<OBJGroup>& groups )
{
	const idBounds& bounds = node->bounds;

	if( bounds.IsCleared() )
	{
		return;
	}

	OBJGroup* group = NULL;
	bool reverse = false;

	if( node->planenum != PLANENUM_LEAF )
	{
		idPlane		plane;

		idFixedWinding w;

		w.BaseForPlane( dmapGlobals.mapPlanes[node->planenum] );

		group = &groups.Alloc();
		group->name.Format( "splitplane.%i", node->nodeNumber ) ;

		// cut down to AABB size
		for( int i = 0 ; i < 3 ; i++ )
		{
			plane[0] = plane[1] = plane[2] = 0;
			plane[i] = 1;
			plane[3] = -bounds[1][i];

			w.ClipInPlace( -plane );

			plane[i] = -1;
			plane[3] = bounds[0][i];

			w.ClipInPlace( -plane );
		}

		OutputWinding( &w, *group );
	}
}

void OutputAreaPortalTriangles( const node_t* node, idList<OBJGroup>& groups )
{
	const idBounds& bounds = node->bounds;

	if( bounds.IsCleared() )
	{
		return;
	}

	if( node->planenum == PLANENUM_LEAF && node->areaPortalTris )
	{
		OBJGroup& group = groups.Alloc();
		group.name.Format( "areaPortalTris.%i", node->nodeNumber ) ;

		for( mapTri_t* tri = node->areaPortalTris; tri; tri = tri->next )
		{
			OBJFace& face = group.faces.Alloc();

			for( int i = 0; i < 3; i++ )
			{
				idDrawVert& dv = face.verts.Alloc();

				dv = tri->v[i];
			}
		}
	}
}

void CollectNodes_r( node_t* node, idList<OBJGroup>& groups )
{
	OutputNode( node, groups );
	OutputSplitPlane( node, groups );
	OutputAreaPortalTriangles( node, groups );

	if( node->planenum != PLANENUM_LEAF )
	{
		CollectNodes_r( node->children[0], groups );
		CollectNodes_r( node->children[1], groups );
		return;
	}
}


int NumberNodes_r( node_t* node, int nextNode, int& nextLeaf )
{
	if( node->planenum == PLANENUM_LEAF )
	{
		node->nodeNumber = nextLeaf++;
		return nextNode;
	}

	node->nodeNumber = nextNode;
	nextNode++;
	nextNode = NumberNodes_r( node->children[0], nextNode, nextLeaf );
	nextNode = NumberNodes_r( node->children[1], nextNode, nextLeaf );

	return nextNode;
}

void OutputAreaPortals( idList<OBJGroup>& groups )
{
	int			i;
	interAreaPortal_t*	iap;
	idWinding*			w;

	for( i = 0; i < interAreaPortals.Num(); i++ )
	{
		iap = &interAreaPortals[i];

		if( iap->side )
		{
			w = iap->side->winding;
		}
		else
		{
			w = & iap->w;
		}

		OBJGroup& group = groups.Alloc();
		group.name.Format( "interAreaPortal.%i", i );
		OutputWinding( w, group );
	}
}

/*
=============
WriteGLView
=============
*/
void WriteGLView( tree_t* tree, const char* source, int entityNum, bool force )
{
	//c_glfaces = 0;
	//common->Printf( "Writing %s\n", source );

	if( entityNum != 0 && !force )
	{
		return;
	}

	idStrStatic< MAX_OSPATH > path;
	path.Format( "%s_BSP_%s_%i.obj", dmapGlobals.mapFileBase, source, entityNum );
	idFileLocal objFile( fileSystem->OpenFileWrite( path, "fs_basepath" ) );

	//path.SetFileExtension( ".mtl" );
	//idFileLocal mtlFile( fileSystem->OpenFileWrite( path, "fs_basepath" ) );

	idList<OBJGroup> groups;

	OBJGroup& portals = groups.Alloc();
	portals.name = "portals";

	CollectPortals_r( tree->headnode, portals );

	common->Printf( "%5i c_glfaces\n", portals.faces.Num() );

	int numLeafs = 0;
	int numNodes = NumberNodes_r( tree->headnode, 0, numLeafs );

	CollectNodes_r( tree->headnode, groups );

	if( entityNum == 0 )
	{
		OutputAreaPortals( groups );
	}

	int numVerts = 0;

	for( int g = 0; g < groups.Num(); g++ )
	{
		const OBJGroup& group = groups[g];

		objFile->Printf( "g %s\n", group.name.c_str() );

		for( int i = 0; i < group.faces.Num(); i++ )
		{
			const OBJFace& face = group.faces[i];

			for( int j = 0; j < face.verts.Num(); j++ )
			{
				const idVec3& v = face.verts[j].xyz;

				objFile->Printf( "v %1.6f %1.6f %1.6f\n", v.x, v.y, v.z );
			}

			/*
			for( int j = 0; j < face.verts.Num(); j++ )
			{
				const idVec2& vST = face.verts[j].GetTexCoord();

				objFile->Printf( "vt %1.6f %1.6f\n", vST.x, vST.y );
			}

			for( int j = 0; j < face.verts.Num(); j++ )
			{
				const idVec3& n = face.verts[j].GetNormal();

				objFile->Printf( "vn %1.6f %1.6f %1.6f\n", n.x, n.y, n.z );
			}

			objFile->Printf( "usemtl %s\n", face.material->GetName() );
			*/

			//for( int j = 0; j < face.indexes.Num(); j += 3 )

			objFile->Printf( "f " );
			for( int j = 0; j < face.verts.Num(); j++ )
			{
				objFile->Printf( "%i// ", numVerts + 1 + j );
			}

			numVerts += face.verts.Num();

			objFile->Printf( "\n\n" );
		}
	}
}


void WriteGLView( bspface_t* list, const char* source )
{
	if( dmapGlobals.entityNum != 0 )
	{
		return;
	}

	idStrStatic< MAX_OSPATH > path;
	path.Format( "%s_BSP_%s_%i.obj", dmapGlobals.mapFileBase, source, dmapGlobals.entityNum );
	idFileLocal objFile( fileSystem->OpenFileWrite( path, "fs_basepath" ) );

	//path.SetFileExtension( ".mtl" );
	//idFileLocal mtlFile( fileSystem->OpenFileWrite( path, "fs_basepath" ) );

	idList<OBJFace> faces;

	for( bspface_t*	face = list ; face ; face = face->next )
	{
		OBJFace& objFace = faces.Alloc();

		for( int i = 0; i < face->w->GetNumPoints(); i++ )
			//for( int i = face->w->GetNumPoints() - 1; i >= 0; i-- )
		{
			idDrawVert& dv = objFace.verts.Alloc();

			dv.xyz.x = ( *face->w )[i][0];
			dv.xyz.y = ( *face->w )[i][1];
			dv.xyz.z = ( *face->w )[i][2];

			//dv.SetColor( level & 255 );
		}
	}

	//common->Printf( "%5i c_glfaces\n", faces.Num() );

	int numVerts = 0;

	for( int i = 0; i < faces.Num(); i++ )
	{
		OBJFace& face = faces[i];

		for( int j = 0; j < face.verts.Num(); j++ )
		{
			const idVec3& v = face.verts[j].xyz;

			objFile->Printf( "v %1.6f %1.6f %1.6f\n", v.x, v.y, v.z );
		}

		objFile->Printf( "f " );
		for( int j = 0; j < face.verts.Num(); j++ )
		{
			objFile->Printf( "%i// ", numVerts + 1 + j );
		}

		numVerts += face.verts.Num();

		objFile->Printf( "\n\n" );
	}
}
