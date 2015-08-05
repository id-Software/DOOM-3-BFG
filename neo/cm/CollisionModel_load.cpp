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

/*
===============================================================================

	Trace model vs. polygonal model collision detection.

	It is more important to minimize the number of collision polygons
	than it is to minimize the number of edges used for collision
	detection (total edges - internal edges).

	Stitching the world tends to minimize the number of edges used
	for collision detection (more internal edges). However stitching
	also results in more collision polygons which usually makes a
	stitched world slower.

	In an average map over 30% of all edges is internal.

===============================================================================
*/

#pragma hdrstop
#include "precompiled.h"


#include "CollisionModel_local.h"

#define CMODEL_BINARYFILE_EXT	"bcmodel"

idCollisionModelManagerLocal	collisionModelManagerLocal;
idCollisionModelManager* 		collisionModelManager = &collisionModelManagerLocal;

cm_windingList_t* 				cm_windingList;
cm_windingList_t* 				cm_outList;
cm_windingList_t* 				cm_tmpList;

idHashIndex* 					cm_vertexHash;
idHashIndex* 					cm_edgeHash;

idBounds						cm_modelBounds;
int								cm_vertexShift;


idCVar preLoad_Collision( "preLoad_Collision", "1", CVAR_SYSTEM | CVAR_BOOL, "preload collision beginlevelload" );

/*
===============================================================================

Proc BSP tree for data pruning

===============================================================================
*/

/*
================
idCollisionModelManagerLocal::ParseProcNodes
================
*/
void idCollisionModelManagerLocal::ParseProcNodes( idLexer* src )
{
	int i;
	
	src->ExpectTokenString( "{" );
	
	numProcNodes = src->ParseInt();
	if( numProcNodes < 0 )
	{
		src->Error( "ParseProcNodes: bad numProcNodes" );
	}
	procNodes = ( cm_procNode_t* )Mem_ClearedAlloc( numProcNodes * sizeof( cm_procNode_t ), TAG_COLLISION );
	
	for( i = 0; i < numProcNodes; i++ )
	{
		cm_procNode_t* node;
		
		node = &procNodes[i];
		
		src->Parse1DMatrix( 4, node->plane.ToFloatPtr() );
		node->children[0] = src->ParseInt();
		node->children[1] = src->ParseInt();
	}
	
	src->ExpectTokenString( "}" );
}

/*
================
idCollisionModelManagerLocal::LoadProcBSP

  FIXME: if the nodes would be at the start of the .proc file it would speed things up considerably
================
*/
void idCollisionModelManagerLocal::LoadProcBSP( const char* name )
{
	idStr filename;
	idToken token;
	idLexer* src;
	
	// load it
	filename = name;
	filename.SetFileExtension( PROC_FILE_EXT );
	src = new( TAG_COLLISION ) idLexer( filename, LEXFL_NOSTRINGCONCAT | LEXFL_NODOLLARPRECOMPILE );
	if( !src->IsLoaded() )
	{
		common->Warning( "idCollisionModelManagerLocal::LoadProcBSP: couldn't load %s", filename.c_str() );
		delete src;
		return;
	}
	
	if( !src->ReadToken( &token ) || token.Icmp( PROC_FILE_ID ) )
	{
		common->Warning( "idCollisionModelManagerLocal::LoadProcBSP: bad id '%s' instead of '%s'", token.c_str(), PROC_FILE_ID );
		delete src;
		return;
	}
	
	// parse the file
	while( 1 )
	{
		if( !src->ReadToken( &token ) )
		{
			break;
		}
		
		if( token == "model" )
		{
			src->SkipBracedSection();
			continue;
		}
		
		if( token == "shadowModel" )
		{
			src->SkipBracedSection();
			continue;
		}
		
		if( token == "interAreaPortals" )
		{
			src->SkipBracedSection();
			continue;
		}
		
		if( token == "nodes" )
		{
			ParseProcNodes( src );
			break;
		}
		
		src->Error( "idCollisionModelManagerLocal::LoadProcBSP: bad token \"%s\"", token.c_str() );
	}
	
	delete src;
}

/*
===============================================================================

Free map

===============================================================================
*/

/*
================
idCollisionModelManagerLocal::Clear
================
*/
void idCollisionModelManagerLocal::Clear()
{
	mapName.Clear();
	mapFileTime = 0;
	loaded = 0;
	checkCount = 0;
	maxModels = 0;
	numModels = 0;
	models = NULL;
	memset( trmPolygons, 0, sizeof( trmPolygons ) );
	trmBrushes[0] = NULL;
	trmMaterial = NULL;
	numProcNodes = 0;
	procNodes = NULL;
	getContacts = false;
	contacts = NULL;
	maxContacts = 0;
	numContacts = 0;
}

/*
================
idCollisionModelManagerLocal::RemovePolygonReferences_r
================
*/
void idCollisionModelManagerLocal::RemovePolygonReferences_r( cm_node_t* node, cm_polygon_t* p )
{
	cm_polygonRef_t* pref;
	
	while( node )
	{
		for( pref = node->polygons; pref; pref = pref->next )
		{
			if( pref->p == p )
			{
				pref->p = NULL;
				// cannot return here because we can have links down the tree due to polygon merging
				//return;
			}
		}
		// if leaf node
		if( node->planeType == -1 )
		{
			break;
		}
		if( p->bounds[0][node->planeType] > node->planeDist )
		{
			node = node->children[0];
		}
		else if( p->bounds[1][node->planeType] < node->planeDist )
		{
			node = node->children[1];
		}
		else
		{
			RemovePolygonReferences_r( node->children[1], p );
			node = node->children[0];
		}
	}
}

/*
================
idCollisionModelManagerLocal::RemoveBrushReferences_r
================
*/
void idCollisionModelManagerLocal::RemoveBrushReferences_r( cm_node_t* node, cm_brush_t* b )
{
	cm_brushRef_t* bref;
	
	while( node )
	{
		for( bref = node->brushes; bref; bref = bref->next )
		{
			if( bref->b == b )
			{
				bref->b = NULL;
				return;
			}
		}
		// if leaf node
		if( node->planeType == -1 )
		{
			break;
		}
		if( b->bounds[0][node->planeType] > node->planeDist )
		{
			node = node->children[0];
		}
		else if( b->bounds[1][node->planeType] < node->planeDist )
		{
			node = node->children[1];
		}
		else
		{
			RemoveBrushReferences_r( node->children[1], b );
			node = node->children[0];
		}
	}
}

/*
================
idCollisionModelManagerLocal::FreeNode
================
*/
void idCollisionModelManagerLocal::FreeNode( cm_node_t* node )
{
	// don't free the node here
	// the nodes are allocated in blocks which are freed when the model is freed
}

/*
================
idCollisionModelManagerLocal::FreePolygonReference
================
*/
void idCollisionModelManagerLocal::FreePolygonReference( cm_polygonRef_t* pref )
{
	// don't free the polygon reference here
	// the polygon references are allocated in blocks which are freed when the model is freed
}

/*
================
idCollisionModelManagerLocal::FreeBrushReference
================
*/
void idCollisionModelManagerLocal::FreeBrushReference( cm_brushRef_t* bref )
{
	// don't free the brush reference here
	// the brush references are allocated in blocks which are freed when the model is freed
}

/*
================
idCollisionModelManagerLocal::FreePolygon
================
*/
void idCollisionModelManagerLocal::FreePolygon( cm_model_t* model, cm_polygon_t* poly )
{
	model->numPolygons--;
	model->polygonMemory -= sizeof( cm_polygon_t ) + ( poly->numEdges - 1 ) * sizeof( poly->edges[0] );
	if( model->polygonBlock == NULL )
	{
		Mem_Free( poly );
	}
}

/*
================
idCollisionModelManagerLocal::FreeBrush
================
*/
void idCollisionModelManagerLocal::FreeBrush( cm_model_t* model, cm_brush_t* brush )
{
	model->numBrushes--;
	model->brushMemory -= sizeof( cm_brush_t ) + ( brush->numPlanes - 1 ) * sizeof( brush->planes[0] );
	if( model->brushBlock == NULL )
	{
		Mem_Free( brush );
	}
}

/*
================
idCollisionModelManagerLocal::FreeTree_r
================
*/
void idCollisionModelManagerLocal::FreeTree_r( cm_model_t* model, cm_node_t* headNode, cm_node_t* node )
{
	cm_polygonRef_t* pref;
	cm_polygon_t* p;
	cm_brushRef_t* bref;
	cm_brush_t* b;
	
	// free all polygons at this node
	for( pref = node->polygons; pref; pref = node->polygons )
	{
		p = pref->p;
		if( p )
		{
			// remove all other references to this polygon
			RemovePolygonReferences_r( headNode, p );
			FreePolygon( model, p );
		}
		node->polygons = pref->next;
		FreePolygonReference( pref );
	}
	// free all brushes at this node
	for( bref = node->brushes; bref; bref = node->brushes )
	{
		b = bref->b;
		if( b )
		{
			// remove all other references to this brush
			RemoveBrushReferences_r( headNode, b );
			FreeBrush( model, b );
		}
		node->brushes = bref->next;
		FreeBrushReference( bref );
	}
	// recurse down the tree
	if( node->planeType != -1 )
	{
		FreeTree_r( model, headNode, node->children[0] );
		node->children[0] = NULL;
		FreeTree_r( model, headNode, node->children[1] );
		node->children[1] = NULL;
	}
	FreeNode( node );
}

/*
================
idCollisionModelManagerLocal::FreeModel
================
*/
void idCollisionModelManagerLocal::FreeModel( cm_model_t* model )
{
	cm_polygonRefBlock_t* polygonRefBlock, *nextPolygonRefBlock;
	cm_brushRefBlock_t* brushRefBlock, *nextBrushRefBlock;
	cm_nodeBlock_t* nodeBlock, *nextNodeBlock;
	
	// free the tree structure
	if( model->node )
	{
		FreeTree_r( model, model->node, model->node );
	}
	// free blocks with polygon references
	for( polygonRefBlock = model->polygonRefBlocks; polygonRefBlock; polygonRefBlock = nextPolygonRefBlock )
	{
		nextPolygonRefBlock = polygonRefBlock->next;
		Mem_Free( polygonRefBlock );
	}
	// free blocks with brush references
	for( brushRefBlock = model->brushRefBlocks; brushRefBlock; brushRefBlock = nextBrushRefBlock )
	{
		nextBrushRefBlock = brushRefBlock->next;
		Mem_Free( brushRefBlock );
	}
	// free blocks with nodes
	for( nodeBlock = model->nodeBlocks; nodeBlock; nodeBlock = nextNodeBlock )
	{
		nextNodeBlock = nodeBlock->next;
		Mem_Free( nodeBlock );
	}
	// free block allocated polygons
	Mem_Free( model->polygonBlock );
	// free block allocated brushes
	Mem_Free( model->brushBlock );
	// free edges
	Mem_Free( model->edges );
	// free vertices
	Mem_Free( model->vertices );
	// free the model
	delete model;
}

/*
================
idCollisionModelManagerLocal::FreeMap
================
*/
void idCollisionModelManagerLocal::FreeMap()
{
	int i;
	
	if( !loaded )
	{
		Clear();
		return;
	}
	
	for( i = 0; i < maxModels; i++ )
	{
		if( !models[i] )
		{
			continue;
		}
		FreeModel( models[i] );
	}
	
	FreeTrmModelStructure();
	
	Mem_Free( models );
	
	Clear();
	
	ShutdownHash();
}

/*
================
idCollisionModelManagerLocal::FreeTrmModelStructure
================
*/
void idCollisionModelManagerLocal::FreeTrmModelStructure()
{
	int i;
	
	assert( models );
	if( !models[MAX_SUBMODELS] )
	{
		return;
	}
	
	for( i = 0; i < MAX_TRACEMODEL_POLYS; i++ )
	{
		FreePolygon( models[MAX_SUBMODELS], trmPolygons[i]->p );
	}
	FreeBrush( models[MAX_SUBMODELS], trmBrushes[0]->b );
	
	models[MAX_SUBMODELS]->node->polygons = NULL;
	models[MAX_SUBMODELS]->node->brushes = NULL;
	FreeModel( models[MAX_SUBMODELS] );
}


/*
===============================================================================

Edge normals

===============================================================================
*/

/*
================
idCollisionModelManagerLocal::CalculateEdgeNormals
================
*/
#define SHARP_EDGE_DOT	-0.7f

void idCollisionModelManagerLocal::CalculateEdgeNormals( cm_model_t* model, cm_node_t* node )
{
	cm_polygonRef_t* pref;
	cm_polygon_t* p;
	cm_edge_t* edge;
	float dot, s;
	int i, edgeNum;
	idVec3 dir;
	
	while( 1 )
	{
		for( pref = node->polygons; pref; pref = pref->next )
		{
			p = pref->p;
			// if we checked this polygon already
			if( p->checkcount == checkCount )
			{
				continue;
			}
			p->checkcount = checkCount;
			
			for( i = 0; i < p->numEdges; i++ )
			{
				edgeNum = p->edges[i];
				edge = model->edges + abs( edgeNum );
				if( edge->normal[0] == 0.0f && edge->normal[1] == 0.0f && edge->normal[2] == 0.0f )
				{
					// if the edge is only used by this polygon
					if( edge->numUsers == 1 )
					{
						dir = model->vertices[ edge->vertexNum[edgeNum < 0]].p - model->vertices[ edge->vertexNum[edgeNum > 0]].p;
						edge->normal = p->plane.Normal().Cross( dir );
						edge->normal.Normalize();
					}
					else
					{
						// the edge is used by more than one polygon
						edge->normal = p->plane.Normal();
					}
				}
				else
				{
					dot = edge->normal * p->plane.Normal();
					// if the two planes make a very sharp edge
					if( dot < SHARP_EDGE_DOT )
					{
						// max length normal pointing outside both polygons
						dir = model->vertices[ edge->vertexNum[edgeNum > 0]].p - model->vertices[ edge->vertexNum[edgeNum < 0]].p;
						edge->normal = edge->normal.Cross( dir ) + p->plane.Normal().Cross( -dir );
						edge->normal *= ( 0.5f / ( 0.5f + 0.5f * SHARP_EDGE_DOT ) ) / edge->normal.Length();
						model->numSharpEdges++;
					}
					else
					{
						s = 0.5f / ( 0.5f + 0.5f * dot );
						edge->normal = s * ( edge->normal + p->plane.Normal() );
					}
				}
			}
		}
		// if leaf node
		if( node->planeType == -1 )
		{
			break;
		}
		CalculateEdgeNormals( model, node->children[1] );
		node = node->children[0];
	}
}

/*
===============================================================================

Trace model to general collision model

===============================================================================
*/

/*
================
idCollisionModelManagerLocal::AllocModel
================
*/
cm_model_t* idCollisionModelManagerLocal::AllocModel()
{
	cm_model_t* model;
	
	model = new( TAG_COLLISION ) cm_model_t;
	model->contents = 0;
	model->isConvex = false;
	model->maxVertices = 0;
	model->numVertices = 0;
	model->vertices = NULL;
	model->maxEdges = 0;
	model->numEdges = 0;
	model->edges = NULL;
	model->node = NULL;
	model->nodeBlocks = NULL;
	model->polygonRefBlocks = NULL;
	model->brushRefBlocks = NULL;
	model->polygonBlock = NULL;
	model->brushBlock = NULL;
	model->numPolygons = model->polygonMemory =
							 model->numBrushes = model->brushMemory =
										 model->numNodes = model->numBrushRefs =
												 model->numPolygonRefs = model->numInternalEdges =
														 model->numSharpEdges = model->numRemovedPolys =
																 model->numMergedPolys = model->usedMemory = 0;
																 
	return model;
}

/*
================
idCollisionModelManagerLocal::AllocNode
================
*/
cm_node_t* idCollisionModelManagerLocal::AllocNode( cm_model_t* model, int blockSize )
{
	int i;
	cm_node_t* node;
	cm_nodeBlock_t* nodeBlock;
	
	if( !model->nodeBlocks || !model->nodeBlocks->nextNode )
	{
		nodeBlock = ( cm_nodeBlock_t* ) Mem_ClearedAlloc( sizeof( cm_nodeBlock_t ) + blockSize * sizeof( cm_node_t ), TAG_COLLISION );
		nodeBlock->nextNode = ( cm_node_t* )( ( ( byte* ) nodeBlock ) + sizeof( cm_nodeBlock_t ) );
		nodeBlock->next = model->nodeBlocks;
		model->nodeBlocks = nodeBlock;
		node = nodeBlock->nextNode;
		for( i = 0; i < blockSize - 1; i++ )
		{
			node->parent = node + 1;
			node = node->parent;
		}
		node->parent = NULL;
	}
	
	node = model->nodeBlocks->nextNode;
	model->nodeBlocks->nextNode = node->parent;
	node->parent = NULL;
	
	return node;
}

/*
================
idCollisionModelManagerLocal::AllocPolygonReference
================
*/
cm_polygonRef_t* idCollisionModelManagerLocal::AllocPolygonReference( cm_model_t* model, int blockSize )
{
	int i;
	cm_polygonRef_t* pref;
	cm_polygonRefBlock_t* prefBlock;
	
	if( !model->polygonRefBlocks || !model->polygonRefBlocks->nextRef )
	{
		prefBlock = ( cm_polygonRefBlock_t* ) Mem_ClearedAlloc( sizeof( cm_polygonRefBlock_t ) + blockSize * sizeof( cm_polygonRef_t ), TAG_COLLISION );
		prefBlock->nextRef = ( cm_polygonRef_t* )( ( ( byte* ) prefBlock ) + sizeof( cm_polygonRefBlock_t ) );
		prefBlock->next = model->polygonRefBlocks;
		model->polygonRefBlocks = prefBlock;
		pref = prefBlock->nextRef;
		for( i = 0; i < blockSize - 1; i++ )
		{
			pref->next = pref + 1;
			pref = pref->next;
		}
		pref->next = NULL;
	}
	
	pref = model->polygonRefBlocks->nextRef;
	model->polygonRefBlocks->nextRef = pref->next;
	
	return pref;
}

/*
================
idCollisionModelManagerLocal::AllocBrushReference
================
*/
cm_brushRef_t* idCollisionModelManagerLocal::AllocBrushReference( cm_model_t* model, int blockSize )
{
	int i;
	cm_brushRef_t* bref;
	cm_brushRefBlock_t* brefBlock;
	
	if( !model->brushRefBlocks || !model->brushRefBlocks->nextRef )
	{
		brefBlock = ( cm_brushRefBlock_t* ) Mem_ClearedAlloc( sizeof( cm_brushRefBlock_t ) + blockSize * sizeof( cm_brushRef_t ), TAG_COLLISION );
		brefBlock->nextRef = ( cm_brushRef_t* )( ( ( byte* ) brefBlock ) + sizeof( cm_brushRefBlock_t ) );
		brefBlock->next = model->brushRefBlocks;
		model->brushRefBlocks = brefBlock;
		bref = brefBlock->nextRef;
		for( i = 0; i < blockSize - 1; i++ )
		{
			bref->next = bref + 1;
			bref = bref->next;
		}
		bref->next = NULL;
	}
	
	bref = model->brushRefBlocks->nextRef;
	model->brushRefBlocks->nextRef = bref->next;
	
	return bref;
}

/*
================
idCollisionModelManagerLocal::AllocPolygon
================
*/
cm_polygon_t* idCollisionModelManagerLocal::AllocPolygon( cm_model_t* model, int numEdges )
{
	cm_polygon_t* poly;
	int size;
	
	size = sizeof( cm_polygon_t ) + ( numEdges - 1 ) * sizeof( poly->edges[0] );
	model->numPolygons++;
	model->polygonMemory += size;
	if( model->polygonBlock && model->polygonBlock->bytesRemaining >= size )
	{
		poly = ( cm_polygon_t* ) model->polygonBlock->next;
		model->polygonBlock->next += size;
		model->polygonBlock->bytesRemaining -= size;
	}
	else
	{
		poly = ( cm_polygon_t* ) Mem_ClearedAlloc( size, TAG_COLLISION );
	}
	return poly;
}

/*
================
idCollisionModelManagerLocal::AllocBrush
================
*/
cm_brush_t* idCollisionModelManagerLocal::AllocBrush( cm_model_t* model, int numPlanes )
{
	cm_brush_t* brush;
	int size;
	
	size = sizeof( cm_brush_t ) + ( numPlanes - 1 ) * sizeof( brush->planes[0] );
	model->numBrushes++;
	model->brushMemory += size;
	if( model->brushBlock && model->brushBlock->bytesRemaining >= size )
	{
		brush = ( cm_brush_t* ) model->brushBlock->next;
		model->brushBlock->next += size;
		model->brushBlock->bytesRemaining -= size;
	}
	else
	{
		brush = ( cm_brush_t* ) Mem_ClearedAlloc( size, TAG_COLLISION );
	}
	return brush;
}

/*
================
idCollisionModelManagerLocal::AddPolygonToNode
================
*/
void idCollisionModelManagerLocal::AddPolygonToNode( cm_model_t* model, cm_node_t* node, cm_polygon_t* p )
{
	cm_polygonRef_t* pref;
	
	pref = AllocPolygonReference( model, model->numPolygonRefs < REFERENCE_BLOCK_SIZE_SMALL ? REFERENCE_BLOCK_SIZE_SMALL : REFERENCE_BLOCK_SIZE_LARGE );
	pref->p = p;
	pref->next = node->polygons;
	node->polygons = pref;
	model->numPolygonRefs++;
}

/*
================
idCollisionModelManagerLocal::AddBrushToNode
================
*/
void idCollisionModelManagerLocal::AddBrushToNode( cm_model_t* model, cm_node_t* node, cm_brush_t* b )
{
	cm_brushRef_t* bref;
	
	bref = AllocBrushReference( model, model->numBrushRefs < REFERENCE_BLOCK_SIZE_SMALL ? REFERENCE_BLOCK_SIZE_SMALL : REFERENCE_BLOCK_SIZE_LARGE );
	bref->b = b;
	bref->next = node->brushes;
	node->brushes = bref;
	model->numBrushRefs++;
}

/*
================
idCollisionModelManagerLocal::SetupTrmModelStructure
================
*/
void idCollisionModelManagerLocal::SetupTrmModelStructure()
{
	int i;
	cm_node_t* node;
	cm_model_t* model;
	
	// setup model
	model = AllocModel();
	
	assert( models );
	models[MAX_SUBMODELS] = model;
	// create node to hold the collision data
	node = ( cm_node_t* ) AllocNode( model, 1 );
	node->planeType = -1;
	model->node = node;
	// allocate vertex and edge arrays
	model->numVertices = 0;
	model->maxVertices = MAX_TRACEMODEL_VERTS;
	model->vertices = ( cm_vertex_t* ) Mem_ClearedAlloc( model->maxVertices * sizeof( cm_vertex_t ), TAG_COLLISION );
	model->numEdges = 0;
	model->maxEdges = MAX_TRACEMODEL_EDGES + 1;
	model->edges = ( cm_edge_t* ) Mem_ClearedAlloc( model->maxEdges * sizeof( cm_edge_t ), TAG_COLLISION );
	// create a material for the trace model polygons
	trmMaterial = declManager->FindMaterial( "_tracemodel", false );
	if( !trmMaterial )
	{
		common->FatalError( "_tracemodel material not found" );
	}
	
	// allocate polygons
	for( i = 0; i < MAX_TRACEMODEL_POLYS; i++ )
	{
		trmPolygons[i] = AllocPolygonReference( model, MAX_TRACEMODEL_POLYS );
		trmPolygons[i]->p = AllocPolygon( model, MAX_TRACEMODEL_POLYEDGES );
		trmPolygons[i]->p->bounds.Clear();
		trmPolygons[i]->p->plane.Zero();
		trmPolygons[i]->p->checkcount = 0;
		trmPolygons[i]->p->contents = -1;		// all contents
		trmPolygons[i]->p->material = trmMaterial;
		trmPolygons[i]->p->numEdges = 0;
	}
	// allocate brush for position test
	trmBrushes[0] = AllocBrushReference( model, 1 );
	trmBrushes[0]->b = AllocBrush( model, MAX_TRACEMODEL_POLYS );
	trmBrushes[0]->b->primitiveNum = 0;
	trmBrushes[0]->b->bounds.Clear();
	trmBrushes[0]->b->checkcount = 0;
	trmBrushes[0]->b->contents = -1;		// all contents
	trmBrushes[ 0 ]->b->material = trmMaterial;
	trmBrushes[0]->b->numPlanes = 0;
}

/*
================
idCollisionModelManagerLocal::SetupTrmModel

Trace models (item boxes, etc) are converted to collision models on the fly, using the last model slot
as a reusable temporary buffer
================
*/
cmHandle_t idCollisionModelManagerLocal::SetupTrmModel( const idTraceModel& trm, const idMaterial* material )
{
	int i, j;
	cm_vertex_t* vertex;
	cm_edge_t* edge;
	cm_polygon_t* poly;
	cm_model_t* model;
	const traceModelVert_t* trmVert;
	const traceModelEdge_t* trmEdge;
	const traceModelPoly_t* trmPoly;
	
	assert( models );
	
	if( material == NULL )
	{
		material = trmMaterial;
	}
	
	model = models[MAX_SUBMODELS];
	model->node->brushes = NULL;
	model->node->polygons = NULL;
	// if not a valid trace model
	if( trm.type == TRM_INVALID || !trm.numPolys )
	{
		return TRACE_MODEL_HANDLE;
	}
	// vertices
	model->numVertices = trm.numVerts;
	vertex = model->vertices;
	trmVert = trm.verts;
	for( i = 0; i < trm.numVerts; i++, vertex++, trmVert++ )
	{
		vertex->p = *trmVert;
		vertex->sideSet = 0;
	}
	// edges
	model->numEdges = trm.numEdges;
	edge = model->edges + 1;
	trmEdge = trm.edges + 1;
	for( i = 0; i < trm.numEdges; i++, edge++, trmEdge++ )
	{
		edge->vertexNum[0] = trmEdge->v[0];
		edge->vertexNum[1] = trmEdge->v[1];
		edge->normal = trmEdge->normal;
		edge->internal = false;
		edge->sideSet = 0;
	}
	// polygons
	model->numPolygons = trm.numPolys;
	trmPoly = trm.polys;
	for( i = 0; i < trm.numPolys; i++, trmPoly++ )
	{
		poly = trmPolygons[i]->p;
		poly->numEdges = trmPoly->numEdges;
		for( j = 0; j < trmPoly->numEdges; j++ )
		{
			poly->edges[j] = trmPoly->edges[j];
		}
		poly->plane.SetNormal( trmPoly->normal );
		poly->plane.SetDist( trmPoly->dist );
		poly->bounds = trmPoly->bounds;
		poly->material = material;
		// link polygon at node
		trmPolygons[i]->next = model->node->polygons;
		model->node->polygons = trmPolygons[i];
	}
	// if the trace model is convex
	if( trm.isConvex )
	{
		// setup brush for position test
		trmBrushes[0]->b->numPlanes = trm.numPolys;
		for( i = 0; i < trm.numPolys; i++ )
		{
			trmBrushes[0]->b->planes[i] = trmPolygons[i]->p->plane;
		}
		trmBrushes[0]->b->bounds = trm.bounds;
		// link brush at node
		trmBrushes[0]->next = model->node->brushes;
		trmBrushes[ 0 ]->b->material = material;
		model->node->brushes = trmBrushes[0];
	}
	// model bounds
	model->bounds = trm.bounds;
	// convex
	model->isConvex = trm.isConvex;
	
	return TRACE_MODEL_HANDLE;
}

/*
===============================================================================

Optimisation, removal of polygons contained within brushes or solid

===============================================================================
*/

/*
============
idCollisionModelManagerLocal::R_ChoppedAwayByProcBSP
============
*/
int idCollisionModelManagerLocal::R_ChoppedAwayByProcBSP( int nodeNum, idFixedWinding* w, const idVec3& normal, const idVec3& origin, const float radius )
{
	int res;
	idFixedWinding back;
	cm_procNode_t* node;
	float dist;
	
	do
	{
		node = procNodes + nodeNum;
		dist = node->plane.Normal() * origin + node->plane[3];
		if( dist > radius )
		{
			res = SIDE_FRONT;
		}
		else if( dist < -radius )
		{
			res = SIDE_BACK;
		}
		else
		{
			res = w->Split( &back, node->plane, CHOP_EPSILON );
		}
		if( res == SIDE_FRONT )
		{
			nodeNum = node->children[0];
		}
		else if( res == SIDE_BACK )
		{
			nodeNum = node->children[1];
		}
		else if( res == SIDE_ON )
		{
			// continue with the side the winding faces
			if( node->plane.Normal() * normal > 0.0f )
			{
				nodeNum = node->children[0];
			}
			else
			{
				nodeNum = node->children[1];
			}
		}
		else
		{
			// if either node is not solid
			if( node->children[0] < 0 || node->children[1] < 0 )
			{
				return false;
			}
			// only recurse if the node is not solid
			if( node->children[1] > 0 )
			{
				if( !R_ChoppedAwayByProcBSP( node->children[1], &back, normal, origin, radius ) )
				{
					return false;
				}
			}
			nodeNum = node->children[0];
		}
	}
	while( nodeNum > 0 );
	if( nodeNum < 0 )
	{
		return false;
	}
	return true;
}

/*
============
idCollisionModelManagerLocal::ChoppedAwayByProcBSP
============
*/
int idCollisionModelManagerLocal::ChoppedAwayByProcBSP( const idFixedWinding& w, const idPlane& plane, int contents )
{
	idFixedWinding neww;
	idBounds bounds;
	float radius;
	idVec3 origin;
	
	// if the .proc file has no BSP tree
	if( procNodes == NULL )
	{
		return false;
	}
	// don't chop if the polygon is not solid
	if( !( contents & CONTENTS_SOLID ) )
	{
		return false;
	}
	// make a local copy of the winding
	neww = w;
	neww.GetBounds( bounds );
	origin = ( bounds[1] - bounds[0] ) * 0.5f;
	radius = origin.Length() + CHOP_EPSILON;
	origin = bounds[0] + origin;
	//
	return R_ChoppedAwayByProcBSP( 0, &neww, plane.Normal(), origin, radius );
}

/*
=============
idCollisionModelManagerLocal::ChopWindingWithBrush

  returns the least number of winding fragments outside the brush
=============
*/
void idCollisionModelManagerLocal::ChopWindingListWithBrush( cm_windingList_t* list, cm_brush_t* b )
{
	int i, k, res, startPlane, planeNum, bestNumWindings;
	idFixedWinding back, front;
	idPlane plane;
	bool chopped;
	int sidedness[MAX_POINTS_ON_WINDING];
	float dist;
	
	if( b->numPlanes > MAX_POINTS_ON_WINDING )
	{
		return;
	}
	
	// get sidedness for the list of windings
	for( i = 0; i < b->numPlanes; i++ )
	{
		plane = -b->planes[i];
		
		dist = plane.Distance( list->origin );
		if( dist > list->radius )
		{
			sidedness[i] = SIDE_FRONT;
		}
		else if( dist < -list->radius )
		{
			sidedness[i] = SIDE_BACK;
		}
		else
		{
			sidedness[i] = list->bounds.PlaneSide( plane );
			if( sidedness[i] == PLANESIDE_FRONT )
			{
				sidedness[i] = SIDE_FRONT;
			}
			else if( sidedness[i] == PLANESIDE_BACK )
			{
				sidedness[i] = SIDE_BACK;
			}
			else
			{
				sidedness[i] = SIDE_CROSS;
			}
		}
	}
	
	cm_outList->numWindings = 0;
	for( k = 0; k < list->numWindings; k++ )
	{
		//
		startPlane = 0;
		bestNumWindings = 1 + b->numPlanes;
		chopped = false;
		do
		{
			front = list->w[k];
			cm_tmpList->numWindings = 0;
			for( planeNum = startPlane, i = 0; i < b->numPlanes; i++, planeNum++ )
			{
			
				if( planeNum >= b->numPlanes )
				{
					planeNum = 0;
				}
				
				res = sidedness[planeNum];
				
				if( res == SIDE_CROSS )
				{
					plane = -b->planes[planeNum];
					res = front.Split( &back, plane, CHOP_EPSILON );
				}
				
				// NOTE:	disabling this can create gaps at places where Z-fighting occurs
				//			Z-fighting should not occur but what if there is a decal brush side
				//			with exactly the same size as another brush side ?
				// only leave windings on a brush if the winding plane and brush side plane face the same direction
				if( res == SIDE_ON && list->primitiveNum >= 0 && ( list->normal * b->planes[planeNum].Normal() ) > 0 )
				{
					// return because all windings in the list will be on this brush side plane
					return;
				}
				
				if( res == SIDE_BACK )
				{
					if( cm_outList->numWindings >= MAX_WINDING_LIST )
					{
						common->Warning( "idCollisionModelManagerLocal::ChopWindingWithBrush: primitive %d more than %d windings", list->primitiveNum, MAX_WINDING_LIST );
						return;
					}
					// winding and brush didn't intersect, store the original winding
					cm_outList->w[cm_outList->numWindings] = list->w[k];
					cm_outList->numWindings++;
					chopped = false;
					break;
				}
				
				if( res == SIDE_CROSS )
				{
					if( cm_tmpList->numWindings >= MAX_WINDING_LIST )
					{
						common->Warning( "idCollisionModelManagerLocal::ChopWindingWithBrush: primitive %d more than %d windings", list->primitiveNum, MAX_WINDING_LIST );
						return;
					}
					// store the front winding in the temporary list
					cm_tmpList->w[cm_tmpList->numWindings] = back;
					cm_tmpList->numWindings++;
					chopped = true;
				}
				
				// if already found a start plane which generates less fragments
				if( cm_tmpList->numWindings >= bestNumWindings )
				{
					break;
				}
			}
			
			// find the best start plane to get the least number of fragments outside the brush
			if( cm_tmpList->numWindings < bestNumWindings )
			{
				bestNumWindings = cm_tmpList->numWindings;
				// store windings from temporary list in the out list
				for( i = 0; i < cm_tmpList->numWindings; i++ )
				{
					if( cm_outList->numWindings + i >= MAX_WINDING_LIST )
					{
						common->Warning( "idCollisionModelManagerLocal::ChopWindingWithBrush: primitive %d more than %d windings", list->primitiveNum, MAX_WINDING_LIST );
						return;
					}
					cm_outList->w[cm_outList->numWindings + i] = cm_tmpList->w[i];
				}
				// if only one winding left then we can't do any better
				if( bestNumWindings == 1 )
				{
					break;
				}
			}
			
			// try the next start plane
			startPlane++;
			
		}
		while( chopped && startPlane < b->numPlanes );
		//
		if( chopped )
		{
			cm_outList->numWindings += bestNumWindings;
		}
	}
	for( k = 0; k < cm_outList->numWindings; k++ )
	{
		list->w[k] = cm_outList->w[k];
	}
	list->numWindings = cm_outList->numWindings;
}

/*
============
idCollisionModelManagerLocal::R_ChopWindingListWithTreeBrushes
============
*/
void idCollisionModelManagerLocal::R_ChopWindingListWithTreeBrushes( cm_windingList_t* list, cm_node_t* node )
{
	int i;
	cm_brushRef_t* bref;
	cm_brush_t* b;
	
	while( 1 )
	{
		for( bref = node->brushes; bref; bref = bref->next )
		{
			b = bref->b;
			// if we checked this brush already
			if( b->checkcount == checkCount )
			{
				continue;
			}
			b->checkcount = checkCount;
			// if the windings in the list originate from this brush
			if( b->primitiveNum == list->primitiveNum )
			{
				continue;
			}
			// if brush has a different contents
			if( b->contents != list->contents )
			{
				continue;
			}
			// brush bounds and winding list bounds should overlap
			for( i = 0; i < 3; i++ )
			{
				if( list->bounds[0][i] > b->bounds[1][i] )
				{
					break;
				}
				if( list->bounds[1][i] < b->bounds[0][i] )
				{
					break;
				}
			}
			if( i < 3 )
			{
				continue;
			}
			// chop windings in the list with brush
			ChopWindingListWithBrush( list, b );
			// if all windings are chopped away we're done
			if( !list->numWindings )
			{
				return;
			}
		}
		// if leaf node
		if( node->planeType == -1 )
		{
			break;
		}
		if( list->bounds[0][node->planeType] > node->planeDist )
		{
			node = node->children[0];
		}
		else if( list->bounds[1][node->planeType] < node->planeDist )
		{
			node = node->children[1];
		}
		else
		{
			R_ChopWindingListWithTreeBrushes( list, node->children[1] );
			if( !list->numWindings )
			{
				return;
			}
			node = node->children[0];
		}
	}
}

/*
============
idCollisionModelManagerLocal::WindingOutsideBrushes

  Returns one winding which is not fully contained in brushes.
  We always favor less polygons over a stitched world.
  If the winding is partly contained and the contained pieces can be chopped off
  without creating multiple winding fragments then the chopped winding is returned.
============
*/
idFixedWinding* idCollisionModelManagerLocal::WindingOutsideBrushes( idFixedWinding* w, const idPlane& plane, int contents, int primitiveNum, cm_node_t* headNode )
{
	int i, windingLeft;
	
	cm_windingList->bounds.Clear();
	for( i = 0; i < w->GetNumPoints(); i++ )
	{
		cm_windingList->bounds.AddPoint( ( *w )[i].ToVec3() );
	}
	
	cm_windingList->origin = ( cm_windingList->bounds[1] - cm_windingList->bounds[0] ) * 0.5;
	cm_windingList->radius = cm_windingList->origin.Length() + CHOP_EPSILON;
	cm_windingList->origin = cm_windingList->bounds[0] + cm_windingList->origin;
	cm_windingList->bounds[0] -= idVec3( CHOP_EPSILON, CHOP_EPSILON, CHOP_EPSILON );
	cm_windingList->bounds[1] += idVec3( CHOP_EPSILON, CHOP_EPSILON, CHOP_EPSILON );
	
	cm_windingList->w[0] = *w;
	cm_windingList->numWindings = 1;
	cm_windingList->normal = plane.Normal();
	cm_windingList->contents = contents;
	cm_windingList->primitiveNum = primitiveNum;
	//
	checkCount++;
	R_ChopWindingListWithTreeBrushes( cm_windingList, headNode );
	//
	if( !cm_windingList->numWindings )
	{
		return NULL;
	}
	if( cm_windingList->numWindings == 1 )
	{
		return &cm_windingList->w[0];
	}
	// if not the world model
	if( numModels != 0 )
	{
		return w;
	}
	// check if winding fragments would be chopped away by the proc BSP tree
	windingLeft = -1;
	for( i = 0; i < cm_windingList->numWindings; i++ )
	{
		if( !ChoppedAwayByProcBSP( cm_windingList->w[i], plane, contents ) )
		{
			if( windingLeft >= 0 )
			{
				return w;
			}
			windingLeft = i;
		}
	}
	if( windingLeft >= 0 )
	{
		return &cm_windingList->w[windingLeft];
	}
	return NULL;
}

/*
===============================================================================

Merging polygons

===============================================================================
*/

/*
=============
idCollisionModelManagerLocal::ReplacePolygons

  does not allow for a node to have multiple references to the same polygon
=============
*/
void idCollisionModelManagerLocal::ReplacePolygons( cm_model_t* model, cm_node_t* node, cm_polygon_t* p1, cm_polygon_t* p2, cm_polygon_t* newp )
{
	cm_polygonRef_t* pref, *lastpref, *nextpref;
	cm_polygon_t* p;
	bool linked;
	
	while( 1 )
	{
		linked = false;
		lastpref = NULL;
		for( pref = node->polygons; pref; pref = nextpref )
		{
			nextpref = pref->next;
			//
			p = pref->p;
			// if this polygon reference should change
			if( p == p1 || p == p2 )
			{
				// if the new polygon is already linked at this node
				if( linked )
				{
					if( lastpref )
					{
						lastpref->next = nextpref;
					}
					else
					{
						node->polygons = nextpref;
					}
					FreePolygonReference( pref );
					model->numPolygonRefs--;
				}
				else
				{
					pref->p = newp;
					linked = true;
					lastpref = pref;
				}
			}
			else
			{
				lastpref = pref;
			}
		}
		// if leaf node
		if( node->planeType == -1 )
		{
			break;
		}
		if( p1->bounds[0][node->planeType] > node->planeDist && p2->bounds[0][node->planeType] > node->planeDist )
		{
			node = node->children[0];
		}
		else if( p1->bounds[1][node->planeType] < node->planeDist && p2->bounds[1][node->planeType] < node->planeDist )
		{
			node = node->children[1];
		}
		else
		{
			ReplacePolygons( model, node->children[1], p1, p2, newp );
			node = node->children[0];
		}
	}
}

/*
=============
idCollisionModelManagerLocal::TryMergePolygons
=============
*/
#define	CONTINUOUS_EPSILON	0.005f
#define NORMAL_EPSILON		0.01f

cm_polygon_t* idCollisionModelManagerLocal::TryMergePolygons( cm_model_t* model, cm_polygon_t* p1, cm_polygon_t* p2 )
{
	int i, j, nexti, prevj;
	int p1BeforeShare, p1AfterShare, p2BeforeShare, p2AfterShare;
	int newEdges[CM_MAX_POLYGON_EDGES], newNumEdges;
	int edgeNum, edgeNum1, edgeNum2, newEdgeNum1, newEdgeNum2;
	cm_edge_t* edge;
	cm_polygon_t* newp;
	idVec3 delta, normal;
	float dot;
	bool keep1, keep2;
	
	if( p1->material != p2->material )
	{
		return NULL;
	}
	if( idMath::Fabs( p1->plane.Dist() - p2->plane.Dist() ) > NORMAL_EPSILON )
	{
		return NULL;
	}
	for( i = 0; i < 3; i++ )
	{
		if( idMath::Fabs( p1->plane.Normal()[i] - p2->plane.Normal()[i] ) > NORMAL_EPSILON )
		{
			return NULL;
		}
		if( p1->bounds[0][i] > p2->bounds[1][i] )
		{
			return NULL;
		}
		if( p1->bounds[1][i] < p2->bounds[0][i] )
		{
			return NULL;
		}
	}
	// this allows for merging polygons with multiple shared edges
	// polygons with multiple shared edges probably never occur tho ;)
	p1BeforeShare = p1AfterShare = p2BeforeShare = p2AfterShare = -1;
	for( i = 0; i < p1->numEdges; i++ )
	{
		nexti = ( i + 1 ) % p1->numEdges;
		for( j = 0; j < p2->numEdges; j++ )
		{
			prevj = ( j + p2->numEdges - 1 ) % p2->numEdges;
			//
			if( abs( p1->edges[i] ) != abs( p2->edges[j] ) )
			{
				// if the next edge of p1 and the previous edge of p2 are the same
				if( abs( p1->edges[nexti] ) == abs( p2->edges[prevj] ) )
				{
					// if both polygons don't use the edge in the same direction
					if( p1->edges[nexti] != p2->edges[prevj] )
					{
						p1BeforeShare = i;
						p2AfterShare = j;
					}
					break;
				}
			}
			// if both polygons don't use the edge in the same direction
			else if( p1->edges[i] != p2->edges[j] )
			{
				// if the next edge of p1 and the previous edge of p2 are not the same
				if( abs( p1->edges[nexti] ) != abs( p2->edges[prevj] ) )
				{
					p1AfterShare = nexti;
					p2BeforeShare = prevj;
					break;
				}
			}
		}
	}
	if( p1BeforeShare < 0 || p1AfterShare < 0 || p2BeforeShare < 0 || p2AfterShare < 0 )
	{
		return NULL;
	}
	
	// check if the new polygon would still be convex
	edgeNum = p1->edges[p1BeforeShare];
	edge = model->edges + abs( edgeNum );
	delta = model->vertices[edge->vertexNum[INT32_SIGNBITNOTSET( edgeNum )]].p -
			model->vertices[edge->vertexNum[INT32_SIGNBITSET( edgeNum )]].p;
	normal = p1->plane.Normal().Cross( delta );
	normal.Normalize();
	
	edgeNum = p2->edges[p2AfterShare];
	edge = model->edges + abs( edgeNum );
	delta = model->vertices[edge->vertexNum[INT32_SIGNBITNOTSET( edgeNum )]].p -
			model->vertices[edge->vertexNum[INT32_SIGNBITSET( edgeNum )]].p;
			
	dot = delta * normal;
	if( dot < -CONTINUOUS_EPSILON )
		return NULL;			// not a convex polygon
	keep1 = ( bool )( dot > CONTINUOUS_EPSILON );
	
	edgeNum = p2->edges[p2BeforeShare];
	edge = model->edges + abs( edgeNum );
	delta = model->vertices[edge->vertexNum[INT32_SIGNBITNOTSET( edgeNum )]].p -
			model->vertices[edge->vertexNum[INT32_SIGNBITSET( edgeNum )]].p;
	normal = p1->plane.Normal().Cross( delta );
	normal.Normalize();
	
	edgeNum = p1->edges[p1AfterShare];
	edge = model->edges + abs( edgeNum );
	delta = model->vertices[edge->vertexNum[INT32_SIGNBITNOTSET( edgeNum )]].p -
			model->vertices[edge->vertexNum[INT32_SIGNBITSET( edgeNum )]].p;
			
	dot = delta * normal;
	if( dot < -CONTINUOUS_EPSILON )
		return NULL;			// not a convex polygon
	keep2 = ( bool )( dot > CONTINUOUS_EPSILON );
	
	newEdgeNum1 = newEdgeNum2 = 0;
	// get new edges if we need to replace colinear ones
	if( !keep1 )
	{
		edgeNum1 = p1->edges[p1BeforeShare];
		edgeNum2 = p2->edges[p2AfterShare];
		GetEdge( model, model->vertices[model->edges[abs( edgeNum1 )].vertexNum[INT32_SIGNBITSET( edgeNum1 )]].p,
				 model->vertices[model->edges[abs( edgeNum2 )].vertexNum[INT32_SIGNBITNOTSET( edgeNum2 )]].p,
				 &newEdgeNum1, -1 );
		if( newEdgeNum1 == 0 )
		{
			keep1 = true;
		}
	}
	if( !keep2 )
	{
		edgeNum1 = p2->edges[p2BeforeShare];
		edgeNum2 = p1->edges[p1AfterShare];
		GetEdge( model, model->vertices[model->edges[abs( edgeNum1 )].vertexNum[INT32_SIGNBITSET( edgeNum1 )]].p,
				 model->vertices[model->edges[abs( edgeNum2 )].vertexNum[INT32_SIGNBITNOTSET( edgeNum2 )]].p,
				 &newEdgeNum2, -1 );
		if( newEdgeNum2 == 0 )
		{
			keep2 = true;
		}
	}
	// set edges for new polygon
	newNumEdges = 0;
	if( !keep2 )
	{
		newEdges[newNumEdges++] = newEdgeNum2;
	}
	if( p1AfterShare < p1BeforeShare )
	{
		for( i = p1AfterShare + ( !keep2 ); i <= p1BeforeShare - ( !keep1 ); i++ )
		{
			newEdges[newNumEdges++] = p1->edges[i];
		}
	}
	else
	{
		for( i = p1AfterShare + ( !keep2 ); i < p1->numEdges; i++ )
		{
			newEdges[newNumEdges++] = p1->edges[i];
		}
		for( i = 0; i <= p1BeforeShare - ( !keep1 ); i++ )
		{
			newEdges[newNumEdges++] = p1->edges[i];
		}
	}
	if( !keep1 )
	{
		newEdges[newNumEdges++] = newEdgeNum1;
	}
	if( p2AfterShare < p2BeforeShare )
	{
		for( i = p2AfterShare + ( !keep1 ); i <= p2BeforeShare - ( !keep2 ); i++ )
		{
			newEdges[newNumEdges++] = p2->edges[i];
		}
	}
	else
	{
		for( i = p2AfterShare + ( !keep1 ); i < p2->numEdges; i++ )
		{
			newEdges[newNumEdges++] = p2->edges[i];
		}
		for( i = 0; i <= p2BeforeShare - ( !keep2 ); i++ )
		{
			newEdges[newNumEdges++] = p2->edges[i];
		}
	}
	
	newp = AllocPolygon( model, newNumEdges );
	memcpy( newp, p1, sizeof( cm_polygon_t ) );
	memcpy( newp->edges, newEdges, newNumEdges * sizeof( int ) );
	newp->numEdges = newNumEdges;
	newp->checkcount = 0;
	// increase usage count for the edges of this polygon
	for( i = 0; i < newp->numEdges; i++ )
	{
		if( !keep1 && newp->edges[i] == newEdgeNum1 )
		{
			continue;
		}
		if( !keep2 && newp->edges[i] == newEdgeNum2 )
		{
			continue;
		}
		model->edges[abs( newp->edges[i] )].numUsers++;
	}
	// create new bounds from the merged polygons
	newp->bounds = p1->bounds + p2->bounds;
	
	return newp;
}

/*
=============
idCollisionModelManagerLocal::MergePolygonWithTreePolygons
=============
*/
bool idCollisionModelManagerLocal::MergePolygonWithTreePolygons( cm_model_t* model, cm_node_t* node, cm_polygon_t* polygon )
{
	int i;
	cm_polygonRef_t* pref;
	cm_polygon_t* p, *newp;
	
	while( 1 )
	{
		for( pref = node->polygons; pref; pref = pref->next )
		{
			p = pref->p;
			//
			if( p == polygon )
			{
				continue;
			}
			//
			newp = TryMergePolygons( model, polygon, p );
			// if polygons were merged
			if( newp )
			{
				model->numMergedPolys++;
				// replace links to the merged polygons with links to the new polygon
				ReplacePolygons( model, model->node, polygon, p, newp );
				// decrease usage count for edges of both merged polygons
				for( i = 0; i < polygon->numEdges; i++ )
				{
					model->edges[abs( polygon->edges[i] )].numUsers--;
				}
				for( i = 0; i < p->numEdges; i++ )
				{
					model->edges[abs( p->edges[i] )].numUsers--;
				}
				// free merged polygons
				FreePolygon( model, polygon );
				FreePolygon( model, p );
				
				return true;
			}
		}
		// if leaf node
		if( node->planeType == -1 )
		{
			break;
		}
		if( polygon->bounds[0][node->planeType] > node->planeDist )
		{
			node = node->children[0];
		}
		else if( polygon->bounds[1][node->planeType] < node->planeDist )
		{
			node = node->children[1];
		}
		else
		{
			if( MergePolygonWithTreePolygons( model, node->children[1], polygon ) )
			{
				return true;
			}
			node = node->children[0];
		}
	}
	return false;
}

/*
=============
idCollisionModelManagerLocal::MergeTreePolygons

  try to merge any two polygons with the same surface flags and the same contents
=============
*/
void idCollisionModelManagerLocal::MergeTreePolygons( cm_model_t* model, cm_node_t* node )
{
	cm_polygonRef_t* pref;
	cm_polygon_t* p;
	bool merge;
	
	while( 1 )
	{
		do
		{
			merge = false;
			for( pref = node->polygons; pref; pref = pref->next )
			{
				p = pref->p;
				// if we checked this polygon already
				if( p->checkcount == checkCount )
				{
					continue;
				}
				p->checkcount = checkCount;
				// try to merge this polygon with other polygons in the tree
				if( MergePolygonWithTreePolygons( model, model->node, p ) )
				{
					merge = true;
					break;
				}
			}
		}
		while( merge );
		// if leaf node
		if( node->planeType == -1 )
		{
			break;
		}
		MergeTreePolygons( model, node->children[1] );
		node = node->children[0];
	}
}

/*
===============================================================================

Find internal edges

===============================================================================
*/

/*

	if (two polygons have the same contents)
		if (the normals of the two polygon planes face towards each other)
			if (an edge is shared between the polygons)
				if (the edge is not shared in the same direction)
					then this is an internal edge
			else
				if (this edge is on the plane of the other polygon)
					if (this edge if fully inside the winding of the other polygon)
						then this edge is an internal edge

*/

/*
=============
idCollisionModelManagerLocal::PointInsidePolygon
=============
*/
bool idCollisionModelManagerLocal::PointInsidePolygon( cm_model_t* model, cm_polygon_t* p, idVec3& v )
{
	int i, edgeNum;
	idVec3* v1, *v2, dir1, dir2, vec;
	cm_edge_t* edge;
	
	for( i = 0; i < p->numEdges; i++ )
	{
		edgeNum = p->edges[i];
		edge = model->edges + abs( edgeNum );
		//
		v1 = &model->vertices[edge->vertexNum[INT32_SIGNBITSET( edgeNum )]].p;
		v2 = &model->vertices[edge->vertexNum[INT32_SIGNBITNOTSET( edgeNum )]].p;
		dir1 = ( *v2 ) - ( *v1 );
		vec = v - ( *v1 );
		dir2 = dir1.Cross( p->plane.Normal() );
		if( vec * dir2 > VERTEX_EPSILON )
		{
			return false;
		}
	}
	return true;
}

/*
=============
idCollisionModelManagerLocal::FindInternalEdgesOnPolygon
=============
*/
void idCollisionModelManagerLocal::FindInternalEdgesOnPolygon( cm_model_t* model, cm_polygon_t* p1, cm_polygon_t* p2 )
{
	int i, j, k, edgeNum;
	cm_edge_t* edge;
	idVec3* v1, *v2, dir1, dir2;
	float d;
	
	// bounds of polygons should overlap or touch
	for( i = 0; i < 3; i++ )
	{
		if( p1->bounds[0][i] > p2->bounds[1][i] )
		{
			return;
		}
		if( p1->bounds[1][i] < p2->bounds[0][i] )
		{
			return;
		}
	}
	//
	// FIXME: doubled geometry causes problems
	//
	for( i = 0; i < p1->numEdges; i++ )
	{
		edgeNum = p1->edges[i];
		edge = model->edges + abs( edgeNum );
		// if already an internal edge
		if( edge->internal )
		{
			continue;
		}
		//
		v1 = &model->vertices[edge->vertexNum[INT32_SIGNBITSET( edgeNum )]].p;
		v2 = &model->vertices[edge->vertexNum[INT32_SIGNBITNOTSET( edgeNum )]].p;
		// if either of the two vertices is outside the bounds of the other polygon
		for( k = 0; k < 3; k++ )
		{
			d = p2->bounds[1][k] + VERTEX_EPSILON;
			if( ( *v1 )[k] > d || ( *v2 )[k] > d )
			{
				break;
			}
			d = p2->bounds[0][k] - VERTEX_EPSILON;
			if( ( *v1 )[k] < d || ( *v2 )[k] < d )
			{
				break;
			}
		}
		if( k < 3 )
		{
			continue;
		}
		//
		k = abs( edgeNum );
		for( j = 0; j < p2->numEdges; j++ )
		{
			if( k == abs( p2->edges[j] ) )
			{
				break;
			}
		}
		// if the edge is shared between the two polygons
		if( j < p2->numEdges )
		{
			// if the edge is used by more than 2 polygons
			if( edge->numUsers > 2 )
			{
				// could still be internal but we'd have to test all polygons using the edge
				continue;
			}
			// if the edge goes in the same direction for both polygons
			if( edgeNum == p2->edges[j] )
			{
				// the polygons can lay ontop of each other or one can obscure the other
				continue;
			}
		}
		// the edge was not shared
		else
		{
			// both vertices should be on the plane of the other polygon
			d = p2->plane.Distance( *v1 );
			if( idMath::Fabs( d ) > VERTEX_EPSILON )
			{
				continue;
			}
			d = p2->plane.Distance( *v2 );
			if( idMath::Fabs( d ) > VERTEX_EPSILON )
			{
				continue;
			}
		}
		// the two polygon plane normals should face towards each other
		dir1 = ( *v2 ) - ( *v1 );
		dir2 = p1->plane.Normal().Cross( dir1 );
		if( p2->plane.Normal() * dir2 < 0 )
		{
			//continue;
			break;
		}
		// if the edge was not shared
		if( j >= p2->numEdges )
		{
			// both vertices of the edge should be inside the winding of the other polygon
			if( !PointInsidePolygon( model, p2, *v1 ) )
			{
				continue;
			}
			if( !PointInsidePolygon( model, p2, *v2 ) )
			{
				continue;
			}
		}
		// we got another internal edge
		edge->internal = true;
		model->numInternalEdges++;
	}
}

/*
=============
idCollisionModelManagerLocal::FindInternalPolygonEdges
=============
*/
void idCollisionModelManagerLocal::FindInternalPolygonEdges( cm_model_t* model, cm_node_t* node, cm_polygon_t* polygon )
{
	cm_polygonRef_t* pref;
	cm_polygon_t* p;
	
	if( polygon->material->GetCullType() == CT_TWO_SIDED || polygon->material->ShouldCreateBackSides() )
	{
		return;
	}
	
	while( 1 )
	{
		for( pref = node->polygons; pref; pref = pref->next )
		{
			p = pref->p;
			//
			// FIXME: use some sort of additional checkcount because currently
			//			polygons can be checked multiple times
			//
			// if the polygons don't have the same contents
			if( p->contents != polygon->contents )
			{
				continue;
			}
			if( p == polygon )
			{
				continue;
			}
			FindInternalEdgesOnPolygon( model, polygon, p );
		}
		// if leaf node
		if( node->planeType == -1 )
		{
			break;
		}
		if( polygon->bounds[0][node->planeType] > node->planeDist )
		{
			node = node->children[0];
		}
		else if( polygon->bounds[1][node->planeType] < node->planeDist )
		{
			node = node->children[1];
		}
		else
		{
			FindInternalPolygonEdges( model, node->children[1], polygon );
			node = node->children[0];
		}
	}
}

/*
=============
idCollisionModelManagerLocal::FindContainedEdges
=============
*/
void idCollisionModelManagerLocal::FindContainedEdges( cm_model_t* model, cm_polygon_t* p )
{
	int i, edgeNum;
	cm_edge_t* edge;
	idFixedWinding w;
	
	for( i = 0; i < p->numEdges; i++ )
	{
		edgeNum = p->edges[i];
		edge = model->edges + abs( edgeNum );
		if( edge->internal )
		{
			continue;
		}
		w.Clear();
		w += model->vertices[edge->vertexNum[INT32_SIGNBITSET( edgeNum )]].p;
		w += model->vertices[edge->vertexNum[INT32_SIGNBITNOTSET( edgeNum )]].p;
		if( ChoppedAwayByProcBSP( w, p->plane, p->contents ) )
		{
			edge->internal = true;
		}
	}
}

/*
=============
idCollisionModelManagerLocal::FindInternalEdges
=============
*/
void idCollisionModelManagerLocal::FindInternalEdges( cm_model_t* model, cm_node_t* node )
{
	cm_polygonRef_t* pref;
	cm_polygon_t* p;
	
	while( 1 )
	{
		for( pref = node->polygons; pref; pref = pref->next )
		{
			p = pref->p;
			// if we checked this polygon already
			if( p->checkcount == checkCount )
			{
				continue;
			}
			p->checkcount = checkCount;
			
			FindInternalPolygonEdges( model, model->node, p );
			
			//FindContainedEdges( model, p );
		}
		// if leaf node
		if( node->planeType == -1 )
		{
			break;
		}
		FindInternalEdges( model, node->children[1] );
		node = node->children[0];
	}
}

/*
===============================================================================

Spatial subdivision

===============================================================================
*/

/*
================
CM_FindSplitter
================
*/
static int CM_FindSplitter( const cm_node_t* node, const idBounds& bounds, int* planeType, float* planeDist )
{
	int i, j, type, axis[3], polyCount;
	float dist, t, bestt, size[3];
	cm_brushRef_t* bref;
	cm_polygonRef_t* pref;
	const cm_node_t* n;
	bool forceSplit = false;
	
	for( i = 0; i < 3; i++ )
	{
		size[i] = bounds[1][i] - bounds[0][i];
		axis[i] = i;
	}
	// sort on largest axis
	for( i = 0; i < 2; i++ )
	{
		if( size[i] < size[i + 1] )
		{
			t = size[i];
			size[i] = size[i + 1];
			size[i + 1] = t;
			j = axis[i];
			axis[i] = axis[i + 1];
			axis[i + 1] = j;
			i = -1;
		}
	}
	// if the node is too small for further splits
	if( size[0] < MIN_NODE_SIZE )
	{
		polyCount = 0;
		for( pref = node->polygons; pref; pref = pref->next )
		{
			polyCount++;
		}
		if( polyCount > MAX_NODE_POLYGONS )
		{
			forceSplit = true;
		}
	}
	// find an axial aligned splitter
	for( i = 0; i < 3; i++ )
	{
		// start with the largest axis first
		type = axis[i];
		bestt = size[i];
		// if the node is small anough in this axis direction
		if( !forceSplit && bestt < MIN_NODE_SIZE )
		{
			break;
		}
		// find an axial splitter from the brush bounding boxes
		// also try brushes from parent nodes
		for( n = node; n; n = n->parent )
		{
			for( bref = n->brushes; bref; bref = bref->next )
			{
				for( j = 0; j < 2; j++ )
				{
					dist = bref->b->bounds[j][type];
					// if the splitter is already used or outside node bounds
					if( dist >= bounds[1][type] || dist <= bounds[0][type] )
					{
						continue;
					}
					// find the most centered splitter
					t = abs( ( bounds[1][type] - dist ) - ( dist - bounds[0][type] ) );
					if( t < bestt )
					{
						bestt = t;
						*planeType = type;
						*planeDist = dist;
					}
				}
			}
		}
		// find an axial splitter from the polygon bounding boxes
		// also try brushes from parent nodes
		for( n = node; n; n = n->parent )
		{
			for( pref = n->polygons; pref; pref = pref->next )
			{
				for( j = 0; j < 2; j++ )
				{
					dist = pref->p->bounds[j][type];
					// if the splitter is already used or outside node bounds
					if( dist >= bounds[1][type] || dist <= bounds[0][type] )
					{
						continue;
					}
					// find the most centered splitter
					t = abs( ( bounds[1][type] - dist ) - ( dist - bounds[0][type] ) );
					if( t < bestt )
					{
						bestt = t;
						*planeType = type;
						*planeDist = dist;
					}
				}
			}
		}
		// if we found a splitter on the largest axis
		if( bestt < size[i] )
		{
			// if forced split due to lots of polygons
			if( forceSplit )
			{
				return true;
			}
			// don't create splitters real close to the bounds
			if( bounds[1][type] - *planeDist > ( MIN_NODE_SIZE * 0.5f ) &&
					*planeDist - bounds[0][type] > ( MIN_NODE_SIZE * 0.5f ) )
			{
				return true;
			}
		}
	}
	return false;
}

/*
================
CM_R_InsideAllChildren
================
*/
static int CM_R_InsideAllChildren( cm_node_t* node, const idBounds& bounds )
{
	assert( node != NULL );
	if( node->planeType != -1 )
	{
		if( bounds[0][node->planeType] >= node->planeDist )
		{
			return false;
		}
		if( bounds[1][node->planeType] <= node->planeDist )
		{
			return false;
		}
		if( !CM_R_InsideAllChildren( node->children[0], bounds ) )
		{
			return false;
		}
		if( !CM_R_InsideAllChildren( node->children[1], bounds ) )
		{
			return false;
		}
	}
	return true;
}

/*
================
idCollisionModelManagerLocal::R_FilterPolygonIntoTree
================
*/
void idCollisionModelManagerLocal::R_FilterPolygonIntoTree( cm_model_t* model, cm_node_t* node, cm_polygonRef_t* pref, cm_polygon_t* p )
{
	assert( node != NULL );
	while( node->planeType != -1 )
	{
		if( CM_R_InsideAllChildren( node, p->bounds ) )
		{
			break;
		}
		if( p->bounds[0][node->planeType] >= node->planeDist )
		{
			node = node->children[0];
		}
		else if( p->bounds[1][node->planeType] <= node->planeDist )
		{
			node = node->children[1];
		}
		else
		{
			R_FilterPolygonIntoTree( model, node->children[1], NULL, p );
			node = node->children[0];
		}
	}
	if( pref )
	{
		pref->next = node->polygons;
		node->polygons = pref;
	}
	else
	{
		AddPolygonToNode( model, node, p );
	}
}

/*
================
idCollisionModelManagerLocal::R_FilterBrushIntoTree
================
*/
void idCollisionModelManagerLocal::R_FilterBrushIntoTree( cm_model_t* model, cm_node_t* node, cm_brushRef_t* pref, cm_brush_t* b )
{
	assert( node != NULL );
	while( node->planeType != -1 )
	{
		if( CM_R_InsideAllChildren( node, b->bounds ) )
		{
			break;
		}
		if( b->bounds[0][node->planeType] >= node->planeDist )
		{
			node = node->children[0];
		}
		else if( b->bounds[1][node->planeType] <= node->planeDist )
		{
			node = node->children[1];
		}
		else
		{
			R_FilterBrushIntoTree( model, node->children[1], NULL, b );
			node = node->children[0];
		}
	}
	if( pref )
	{
		pref->next = node->brushes;
		node->brushes = pref;
	}
	else
	{
		AddBrushToNode( model, node, b );
	}
}

/*
================
idCollisionModelManagerLocal::R_CreateAxialBSPTree

  a brush or polygon is linked in the node closest to the root where
  the brush or polygon is inside all children
================
*/
cm_node_t* idCollisionModelManagerLocal::R_CreateAxialBSPTree( cm_model_t* model, cm_node_t* node, const idBounds& bounds )
{
	int planeType;
	float planeDist;
	cm_polygonRef_t* pref, *nextpref, *prevpref;
	cm_brushRef_t* bref, *nextbref, *prevbref;
	cm_node_t* frontNode, *backNode, *n;
	idBounds frontBounds, backBounds;
	
	if( !CM_FindSplitter( node, bounds, &planeType, &planeDist ) )
	{
		node->planeType = -1;
		return node;
	}
	// create two child nodes
	frontNode = AllocNode( model, NODE_BLOCK_SIZE_LARGE );
	memset( frontNode, 0, sizeof( cm_node_t ) );
	frontNode->parent = node;
	frontNode->planeType = -1;
	//
	backNode = AllocNode( model, NODE_BLOCK_SIZE_LARGE );
	memset( backNode, 0, sizeof( cm_node_t ) );
	backNode->parent = node;
	backNode->planeType = -1;
	//
	model->numNodes += 2;
	// set front node bounds
	frontBounds = bounds;
	frontBounds[0][planeType] = planeDist;
	// set back node bounds
	backBounds = bounds;
	backBounds[1][planeType] = planeDist;
	//
	node->planeType = planeType;
	node->planeDist = planeDist;
	node->children[0] = frontNode;
	node->children[1] = backNode;
	// filter polygons and brushes down the tree if necesary
	for( n = node; n; n = n->parent )
	{
		prevpref = NULL;
		for( pref = n->polygons; pref; pref = nextpref )
		{
			nextpref = pref->next;
			// if polygon is not inside all children
			if( !CM_R_InsideAllChildren( n, pref->p->bounds ) )
			{
				// filter polygon down the tree
				R_FilterPolygonIntoTree( model, n, pref, pref->p );
				if( prevpref )
				{
					prevpref->next = nextpref;
				}
				else
				{
					n->polygons = nextpref;
				}
			}
			else
			{
				prevpref = pref;
			}
		}
		prevbref = NULL;
		for( bref = n->brushes; bref; bref = nextbref )
		{
			nextbref = bref->next;
			// if brush is not inside all children
			if( !CM_R_InsideAllChildren( n, bref->b->bounds ) )
			{
				// filter brush down the tree
				R_FilterBrushIntoTree( model, n, bref, bref->b );
				if( prevbref )
				{
					prevbref->next = nextbref;
				}
				else
				{
					n->brushes = nextbref;
				}
			}
			else
			{
				prevbref = bref;
			}
		}
	}
	R_CreateAxialBSPTree( model, frontNode, frontBounds );
	R_CreateAxialBSPTree( model, backNode, backBounds );
	return node;
}

/*
int cm_numSavedPolygonLinks;
int cm_numSavedBrushLinks;

int CM_R_CountChildren( cm_node_t *node ) {
	if ( node->planeType == -1 ) {
		return 0;
	}
	return 2 + CM_R_CountChildren(node->children[0]) + CM_R_CountChildren(node->children[1]);
}

void CM_R_TestOptimisation( cm_node_t *node ) {
	int polyCount, brushCount, numChildren;
	cm_polygonRef_t *pref;
	cm_brushRef_t *bref;

	if ( node->planeType == -1 ) {
		return;
	}
	polyCount = 0;
	for ( pref = node->polygons; pref; pref = pref->next) {
		polyCount++;
	}
	brushCount = 0;
	for ( bref = node->brushes; bref; bref = bref->next) {
		brushCount++;
	}
	if ( polyCount || brushCount ) {
		numChildren = CM_R_CountChildren( node );
		cm_numSavedPolygonLinks += (numChildren - 1) * polyCount;
		cm_numSavedBrushLinks += (numChildren - 1) * brushCount;
	}
	CM_R_TestOptimisation( node->children[0] );
	CM_R_TestOptimisation( node->children[1] );
}
*/

/*
================
idCollisionModelManagerLocal::CreateAxialBSPTree
================
*/
cm_node_t* idCollisionModelManagerLocal::CreateAxialBSPTree( cm_model_t* model, cm_node_t* node )
{
	cm_polygonRef_t* pref;
	cm_brushRef_t* bref;
	idBounds bounds;
	
	// get head node bounds
	bounds.Clear();
	for( pref = node->polygons; pref; pref = pref->next )
	{
		bounds += pref->p->bounds;
	}
	for( bref = node->brushes; bref; bref = bref->next )
	{
		bounds += bref->b->bounds;
	}
	
	// create axial BSP tree from head node
	node = R_CreateAxialBSPTree( model, node, bounds );
	
	return node;
}

/*
===============================================================================

Raw polygon and brush data

===============================================================================
*/

/*
================
idCollisionModelManagerLocal::SetupHash
================
*/
void idCollisionModelManagerLocal::SetupHash()
{
	if( !cm_vertexHash )
	{
		cm_vertexHash = new( TAG_COLLISION ) idHashIndex( VERTEX_HASH_SIZE, 1024 );
	}
	if( !cm_edgeHash )
	{
		cm_edgeHash = new( TAG_COLLISION ) idHashIndex( EDGE_HASH_SIZE, 1024 );
	}
	// init variables used during loading and optimization
	if( !cm_windingList )
	{
		cm_windingList = new( TAG_COLLISION ) cm_windingList_t;
	}
	if( !cm_outList )
	{
		cm_outList = new( TAG_COLLISION ) cm_windingList_t;
	}
	if( !cm_tmpList )
	{
		cm_tmpList = new( TAG_COLLISION ) cm_windingList_t;
	}
}

/*
================
idCollisionModelManagerLocal::ShutdownHash
================
*/
void idCollisionModelManagerLocal::ShutdownHash()
{
	delete cm_vertexHash;
	cm_vertexHash = NULL;
	delete cm_edgeHash;
	cm_edgeHash = NULL;
	delete cm_tmpList;
	cm_tmpList = NULL;
	delete cm_outList;
	cm_outList = NULL;
	delete cm_windingList;
	cm_windingList = NULL;
}

/*
================
idCollisionModelManagerLocal::ClearHash
================
*/
void idCollisionModelManagerLocal::ClearHash( idBounds& bounds )
{
	int i;
	float f, max;
	
	cm_vertexHash->Clear();
	cm_edgeHash->Clear();
	
	cm_modelBounds = bounds;
	max = bounds[1].x - bounds[0].x;
	f = bounds[1].y - bounds[0].y;
	if( f > max )
	{
		max = f;
	}
	cm_vertexShift = ( float ) max / VERTEX_HASH_BOXSIZE;
	for( i = 0; ( 1 << i ) < cm_vertexShift; i++ )
	{
	}
	if( i == 0 )
	{
		cm_vertexShift = 1;
	}
	else
	{
		cm_vertexShift = i;
	}
}

/*
================
idCollisionModelManagerLocal::HashVec
================
*/
ID_INLINE int idCollisionModelManagerLocal::HashVec( const idVec3& vec )
{
	/*
	int x, y;
	
	x = (((int)(vec[0] - cm_modelBounds[0].x + 0.5 )) >> cm_vertexShift) & (VERTEX_HASH_BOXSIZE-1);
	y = (((int)(vec[1] - cm_modelBounds[0].y + 0.5 )) >> cm_vertexShift) & (VERTEX_HASH_BOXSIZE-1);
	
	assert (x >= 0 && x < VERTEX_HASH_BOXSIZE && y >= 0 && y < VERTEX_HASH_BOXSIZE);
	
	return y * VERTEX_HASH_BOXSIZE + x;
	*/
	int x, y, z;
	
	x = ( ( ( int )( vec[0] - cm_modelBounds[0].x + 0.5 ) ) + 2 ) >> 2;
	y = ( ( ( int )( vec[1] - cm_modelBounds[0].y + 0.5 ) ) + 2 ) >> 2;
	z = ( ( ( int )( vec[2] - cm_modelBounds[0].z + 0.5 ) ) + 2 ) >> 2;
	return ( x + y * VERTEX_HASH_BOXSIZE + z ) & ( VERTEX_HASH_SIZE - 1 );
}

/*
================
idCollisionModelManagerLocal::GetVertex
================
*/
int idCollisionModelManagerLocal::GetVertex( cm_model_t* model, const idVec3& v, int* vertexNum )
{
	int i, hashKey, vn;
	idVec3 vert, *p;
	
	for( i = 0; i < 3; i++ )
	{
		if( idMath::Fabs( v[i] - idMath::Rint( v[i] ) ) < INTEGRAL_EPSILON )
			vert[i] = idMath::Rint( v[i] );
		else
			vert[i] = v[i];
	}
	
	hashKey = HashVec( vert );
	
	for( vn = cm_vertexHash->First( hashKey ); vn >= 0; vn = cm_vertexHash->Next( vn ) )
	{
		p = &model->vertices[vn].p;
		// first compare z-axis because hash is based on x-y plane
		if( idMath::Fabs( vert[2] - ( *p )[2] ) < VERTEX_EPSILON &&
				idMath::Fabs( vert[0] - ( *p )[0] ) < VERTEX_EPSILON &&
				idMath::Fabs( vert[1] - ( *p )[1] ) < VERTEX_EPSILON )
		{
			*vertexNum = vn;
			return true;
		}
	}
	
	if( model->numVertices >= model->maxVertices )
	{
		cm_vertex_t* oldVertices;
		
		// resize vertex array
		model->maxVertices = ( float ) model->maxVertices * 1.5f + 1;
		oldVertices = model->vertices;
		model->vertices = ( cm_vertex_t* ) Mem_ClearedAlloc( model->maxVertices * sizeof( cm_vertex_t ), TAG_COLLISION );
		memcpy( model->vertices, oldVertices, model->numVertices * sizeof( cm_vertex_t ) );
		Mem_Free( oldVertices );
		
		cm_vertexHash->ResizeIndex( model->maxVertices );
	}
	model->vertices[model->numVertices].p = vert;
	model->vertices[model->numVertices].checkcount = 0;
	*vertexNum = model->numVertices;
	// add vertice to hash
	cm_vertexHash->Add( hashKey, model->numVertices );
	//
	model->numVertices++;
	return false;
}

/*
================
idCollisionModelManagerLocal::GetEdge
================
*/
int idCollisionModelManagerLocal::GetEdge( cm_model_t* model, const idVec3& v1, const idVec3& v2, int* edgeNum, int v1num )
{
	int v2num, hashKey, e;
	int found, *vertexNum;
	
	// the first edge is a dummy
	if( model->numEdges == 0 )
	{
		model->numEdges = 1;
	}
	
	if( v1num != -1 )
	{
		found = 1;
	}
	else
	{
		found = GetVertex( model, v1, &v1num );
	}
	found &= GetVertex( model, v2, &v2num );
	// if both vertices are the same or snapped onto each other
	if( v1num == v2num )
	{
		*edgeNum = 0;
		return true;
	}
	hashKey = cm_edgeHash->GenerateKey( v1num, v2num );
	// if both vertices where already stored
	if( found )
	{
		for( e = cm_edgeHash->First( hashKey ); e >= 0; e = cm_edgeHash->Next( e ) )
		{
			// NOTE: only allow at most two users that use the edge in opposite direction
			if( model->edges[e].numUsers != 1 )
			{
				continue;
			}
			
			vertexNum = model->edges[e].vertexNum;
			if( vertexNum[0] == v2num )
			{
				if( vertexNum[1] == v1num )
				{
					// negative for a reversed edge
					*edgeNum = -e;
					break;
				}
			}
			/*
			else if ( vertexNum[0] == v1num ) {
				if ( vertexNum[1] == v2num ) {
					*edgeNum = e;
					break;
				}
			}
			*/
		}
		// if edge found in hash
		if( e >= 0 )
		{
			model->edges[e].numUsers++;
			return true;
		}
	}
	if( model->numEdges >= model->maxEdges )
	{
		cm_edge_t* oldEdges;
		
		// resize edge array
		model->maxEdges = ( float ) model->maxEdges * 1.5f + 1;
		oldEdges = model->edges;
		model->edges = ( cm_edge_t* ) Mem_ClearedAlloc( model->maxEdges * sizeof( cm_edge_t ), TAG_COLLISION );
		memcpy( model->edges, oldEdges, model->numEdges * sizeof( cm_edge_t ) );
		Mem_Free( oldEdges );
		
		cm_edgeHash->ResizeIndex( model->maxEdges );
	}
	// setup edge
	model->edges[model->numEdges].vertexNum[0] = v1num;
	model->edges[model->numEdges].vertexNum[1] = v2num;
	model->edges[model->numEdges].internal = false;
	model->edges[model->numEdges].checkcount = 0;
	model->edges[model->numEdges].numUsers = 1; // used by one polygon atm
	model->edges[model->numEdges].normal.Zero();
	//
	*edgeNum = model->numEdges;
	// add edge to hash
	cm_edgeHash->Add( hashKey, model->numEdges );
	
	model->numEdges++;
	
	return false;
}

/*
================
idCollisionModelManagerLocal::CreatePolygon
================
*/
void idCollisionModelManagerLocal::CreatePolygon( cm_model_t* model, idFixedWinding* w, const idPlane& plane, const idMaterial* material, int primitiveNum )
{
	int i, j, edgeNum, v1num;
	int numPolyEdges, polyEdges[MAX_POINTS_ON_WINDING];
	idBounds bounds;
	cm_polygon_t* p;
	
	// turn the winding into a sequence of edges
	numPolyEdges = 0;
	v1num = -1;		// first vertex unknown
	for( i = 0, j = 1; i < w->GetNumPoints(); i++, j++ )
	{
		if( j >= w->GetNumPoints() )
		{
			j = 0;
		}
		GetEdge( model, ( *w )[i].ToVec3(), ( *w )[j].ToVec3(), &polyEdges[numPolyEdges], v1num );
		if( polyEdges[numPolyEdges] )
		{
			// last vertex of this edge is the first vertex of the next edge
			v1num = model->edges[ abs( polyEdges[numPolyEdges] ) ].vertexNum[ INT32_SIGNBITNOTSET( polyEdges[numPolyEdges] ) ];
			// this edge is valid so keep it
			numPolyEdges++;
		}
	}
	// should have at least 3 edges
	if( numPolyEdges < 3 )
	{
		return;
	}
	// the polygon is invalid if some edge is found twice
	for( i = 0; i < numPolyEdges; i++ )
	{
		for( j = i + 1; j < numPolyEdges; j++ )
		{
			if( abs( polyEdges[i] ) == abs( polyEdges[j] ) )
			{
				return;
			}
		}
	}
	// don't overflow max edges
	if( numPolyEdges > CM_MAX_POLYGON_EDGES )
	{
		common->Warning( "idCollisionModelManagerLocal::CreatePolygon: polygon has more than %d edges", numPolyEdges );
		numPolyEdges = CM_MAX_POLYGON_EDGES;
	}
	
	w->GetBounds( bounds );
	
	p = AllocPolygon( model, numPolyEdges );
	p->numEdges = numPolyEdges;
	p->contents = material->GetContentFlags();
	p->material = material;
	p->checkcount = 0;
	p->plane = plane;
	p->bounds = bounds;
	for( i = 0; i < numPolyEdges; i++ )
	{
		edgeNum = polyEdges[i];
		p->edges[i] = edgeNum;
	}
	R_FilterPolygonIntoTree( model, model->node, NULL, p );
}

/*
================
idCollisionModelManagerLocal::PolygonFromWinding

  NOTE: for patches primitiveNum < 0 and abs(primitiveNum) is the real number
================
*/
void idCollisionModelManagerLocal::PolygonFromWinding( cm_model_t* model, idFixedWinding* w, const idPlane& plane, const idMaterial* material, int primitiveNum )
{
	int contents;
	
	contents = material->GetContentFlags();
	
	// if this polygon is part of the world model
	if( numModels == 0 )
	{
		// if the polygon is fully chopped away by the proc bsp tree
		if( ChoppedAwayByProcBSP( *w, plane, contents ) )
		{
			model->numRemovedPolys++;
			return;
		}
	}
	
	// get one winding that is not or only partly contained in brushes
	w = WindingOutsideBrushes( w, plane, contents, primitiveNum, model->node );
	
	// if the polygon is fully contained within a brush
	if( !w )
	{
		model->numRemovedPolys++;
		return;
	}
	
	if( w->IsHuge() )
	{
		common->Warning( "idCollisionModelManagerLocal::PolygonFromWinding: model %s primitive %d is degenerate", model->name.c_str(), abs( primitiveNum ) );
		return;
	}
	
	CreatePolygon( model, w, plane, material, primitiveNum );
	
	if( material->GetCullType() == CT_TWO_SIDED || material->ShouldCreateBackSides() )
	{
		w->ReverseSelf();
		CreatePolygon( model, w, -plane, material, primitiveNum );
	}
}

/*
=================
idCollisionModelManagerLocal::CreatePatchPolygons
=================
*/
void idCollisionModelManagerLocal::CreatePatchPolygons( cm_model_t* model, idSurface_Patch& mesh, const idMaterial* material, int primitiveNum )
{
	int i, j;
	float dot;
	int v1, v2, v3, v4;
	idFixedWinding w;
	idPlane plane;
	idVec3 d1, d2;
	
	for( i = 0; i < mesh.GetWidth() - 1; i++ )
	{
		for( j = 0; j < mesh.GetHeight() - 1; j++ )
		{
		
			v1 = j * mesh.GetWidth() + i;
			v2 = v1 + 1;
			v3 = v1 + mesh.GetWidth() + 1;
			v4 = v1 + mesh.GetWidth();
			
			d1 = mesh[v2].xyz - mesh[v1].xyz;
			d2 = mesh[v3].xyz - mesh[v1].xyz;
			plane.SetNormal( d1.Cross( d2 ) );
			if( plane.Normalize() != 0.0f )
			{
				plane.FitThroughPoint( mesh[v1].xyz );
				dot = plane.Distance( mesh[v4].xyz );
				// if we can turn it into a quad
				if( idMath::Fabs( dot ) < 0.1f )
				{
					w.Clear();
					w += mesh[v1].xyz;
					w += mesh[v2].xyz;
					w += mesh[v3].xyz;
					w += mesh[v4].xyz;
					
					PolygonFromWinding( model, &w, plane, material, -primitiveNum );
					continue;
				}
				else
				{
					// create one of the triangles
					w.Clear();
					w += mesh[v1].xyz;
					w += mesh[v2].xyz;
					w += mesh[v3].xyz;
					
					PolygonFromWinding( model, &w, plane, material, -primitiveNum );
				}
			}
			// create the other triangle
			d1 = mesh[v3].xyz - mesh[v1].xyz;
			d2 = mesh[v4].xyz - mesh[v1].xyz;
			plane.SetNormal( d1.Cross( d2 ) );
			if( plane.Normalize() != 0.0f )
			{
				plane.FitThroughPoint( mesh[v1].xyz );
				
				w.Clear();
				w += mesh[v1].xyz;
				w += mesh[v3].xyz;
				w += mesh[v4].xyz;
				
				PolygonFromWinding( model, &w, plane, material, -primitiveNum );
			}
		}
	}
}

/*
=================
CM_EstimateVertsAndEdges
=================
*/
static void CM_EstimateVertsAndEdges( const idMapEntity* mapEnt, int* numVerts, int* numEdges )
{
	int j, width, height;
	
	*numVerts = *numEdges = 0;
	for( j = 0; j < mapEnt->GetNumPrimitives(); j++ )
	{
		const idMapPrimitive* mapPrim;
		mapPrim = mapEnt->GetPrimitive( j );
		if( mapPrim->GetType() == idMapPrimitive::TYPE_PATCH )
		{
			// assume maximum tesselation without adding verts
			width = static_cast<const idMapPatch*>( mapPrim )->GetWidth();
			height = static_cast<const idMapPatch*>( mapPrim )->GetHeight();
			*numVerts += width * height;
			*numEdges += ( width - 1 ) * height + width * ( height - 1 ) + ( width - 1 ) * ( height - 1 );
			continue;
		}
		if( mapPrim->GetType() == idMapPrimitive::TYPE_BRUSH )
		{
			// assume cylinder with a polygon with (numSides - 2) edges ontop and on the bottom
			*numVerts += ( static_cast<const idMapBrush*>( mapPrim )->GetNumSides() - 2 ) * 2;
			*numEdges += ( static_cast<const idMapBrush*>( mapPrim )->GetNumSides() - 2 ) * 3;
			continue;
		}
	}
}

/*
=================
idCollisionModelManagerLocal::ConverPatch
=================
*/
void idCollisionModelManagerLocal::ConvertPatch( cm_model_t* model, const idMapPatch* patch, int primitiveNum )
{
	const idMaterial* material;
	idSurface_Patch* cp;
	
	material = declManager->FindMaterial( patch->GetMaterial() );
	if( !( material->GetContentFlags() & CONTENTS_REMOVE_UTIL ) )
	{
		return;
	}
	
	// copy the patch
	cp = new( TAG_COLLISION ) idSurface_Patch( *patch );
	
	// if the patch has an explicit number of subdivisions use it to avoid cracks
	if( patch->GetExplicitlySubdivided() )
	{
		cp->SubdivideExplicit( patch->GetHorzSubdivisions(), patch->GetVertSubdivisions(), false, true );
	}
	else
	{
		cp->Subdivide( DEFAULT_CURVE_MAX_ERROR_CD, DEFAULT_CURVE_MAX_ERROR_CD, DEFAULT_CURVE_MAX_LENGTH_CD, false );
	}
	
	// create collision polygons for the patch
	CreatePatchPolygons( model, *cp, material, primitiveNum );
	
	delete cp;
}

/*
================
idCollisionModelManagerLocal::ConvertBrushSides
================
*/
void idCollisionModelManagerLocal::ConvertBrushSides( cm_model_t* model, const idMapBrush* mapBrush, int primitiveNum )
{
	int i, j;
	idMapBrushSide* mapSide;
	idFixedWinding w;
	idPlane* planes;
	const idMaterial* material;
	
	// fix degenerate planes
	planes = ( idPlane* ) _alloca16( mapBrush->GetNumSides() * sizeof( planes[0] ) );
	for( i = 0; i < mapBrush->GetNumSides(); i++ )
	{
		planes[i] = mapBrush->GetSide( i )->GetPlane();
		planes[i].FixDegeneracies( DEGENERATE_DIST_EPSILON );
	}
	
	// create a collision polygon for each brush side
	for( i = 0; i < mapBrush->GetNumSides(); i++ )
	{
		mapSide = mapBrush->GetSide( i );
		material = declManager->FindMaterial( mapSide->GetMaterial() );
		if( !( material->GetContentFlags() & CONTENTS_REMOVE_UTIL ) )
		{
			continue;
		}
		w.BaseForPlane( -planes[i] );
		for( j = 0; j < mapBrush->GetNumSides() && w.GetNumPoints(); j++ )
		{
			if( i == j )
			{
				continue;
			}
			w.ClipInPlace( -planes[j], 0 );
		}
		
		if( w.GetNumPoints() )
		{
			PolygonFromWinding( model, &w, planes[i], material, primitiveNum );
		}
	}
}

/*
================
idCollisionModelManagerLocal::ConvertBrush
================
*/
void idCollisionModelManagerLocal::ConvertBrush( cm_model_t* model, const idMapBrush* mapBrush, int primitiveNum )
{
	int i, j, contents;
	idBounds bounds;
	idMapBrushSide* mapSide;
	cm_brush_t* brush;
	idPlane* planes;
	idFixedWinding w;
	const idMaterial* material = NULL;
	
	contents = 0;
	bounds.Clear();
	
	// fix degenerate planes
	planes = ( idPlane* ) _alloca16( mapBrush->GetNumSides() * sizeof( planes[0] ) );
	for( i = 0; i < mapBrush->GetNumSides(); i++ )
	{
		planes[i] = mapBrush->GetSide( i )->GetPlane();
		planes[i].FixDegeneracies( DEGENERATE_DIST_EPSILON );
	}
	
	// we are only getting the bounds for the brush so there's no need
	// to create a winding for the last brush side
	for( i = 0; i < mapBrush->GetNumSides() - 1; i++ )
	{
		mapSide = mapBrush->GetSide( i );
		material = declManager->FindMaterial( mapSide->GetMaterial() );
		contents |= ( material->GetContentFlags() & CONTENTS_REMOVE_UTIL );
		w.BaseForPlane( -planes[i] );
		for( j = 0; j < mapBrush->GetNumSides() && w.GetNumPoints(); j++ )
		{
			if( i == j )
			{
				continue;
			}
			w.ClipInPlace( -planes[j], 0 );
		}
		
		for( j = 0; j < w.GetNumPoints(); j++ )
		{
			bounds.AddPoint( w[j].ToVec3() );
		}
	}
	if( !contents )
	{
		return;
	}
	// create brush for position test
	brush = AllocBrush( model, mapBrush->GetNumSides() );
	brush->checkcount = 0;
	brush->contents = contents;
	brush->material = material;
	brush->primitiveNum = primitiveNum;
	brush->bounds = bounds;
	brush->numPlanes = mapBrush->GetNumSides();
	for( i = 0; i < mapBrush->GetNumSides(); i++ )
	{
		brush->planes[i] = planes[i];
	}
	AddBrushToNode( model, model->node, brush );
}

/*
================
CM_CountNodeBrushes
================
*/
static int CM_CountNodeBrushes( const cm_node_t* node )
{
	int count;
	cm_brushRef_t* bref;
	
	count = 0;
	for( bref = node->brushes; bref; bref = bref->next )
	{
		count++;
	}
	return count;
}

/*
================
CM_R_GetModelBounds
================
*/
static void CM_R_GetNodeBounds( idBounds* bounds, cm_node_t* node )
{
	cm_polygonRef_t* pref;
	cm_brushRef_t* bref;
	
	while( 1 )
	{
		for( pref = node->polygons; pref; pref = pref->next )
		{
			bounds->AddPoint( pref->p->bounds[0] );
			bounds->AddPoint( pref->p->bounds[1] );
		}
		for( bref = node->brushes; bref; bref = bref->next )
		{
			bounds->AddPoint( bref->b->bounds[0] );
			bounds->AddPoint( bref->b->bounds[1] );
		}
		if( node->planeType == -1 )
		{
			break;
		}
		CM_R_GetNodeBounds( bounds, node->children[1] );
		node = node->children[0];
	}
}

/*
================
CM_GetNodeBounds
================
*/
void CM_GetNodeBounds( idBounds* bounds, cm_node_t* node )
{
	bounds->Clear();
	CM_R_GetNodeBounds( bounds, node );
	if( bounds->IsCleared() )
	{
		bounds->Zero();
	}
}

/*
================
CM_GetNodeContents
================
*/
int CM_GetNodeContents( cm_node_t* node )
{
	int contents;
	cm_polygonRef_t* pref;
	cm_brushRef_t* bref;
	
	contents = 0;
	while( 1 )
	{
		for( pref = node->polygons; pref; pref = pref->next )
		{
			contents |= pref->p->contents;
		}
		for( bref = node->brushes; bref; bref = bref->next )
		{
			contents |= bref->b->contents;
		}
		if( node->planeType == -1 )
		{
			break;
		}
		contents |= CM_GetNodeContents( node->children[1] );
		node = node->children[0];
	}
	return contents;
}

/*
==================
idCollisionModelManagerLocal::RemapEdges
==================
*/
void idCollisionModelManagerLocal::RemapEdges( cm_node_t* node, int* edgeRemap )
{
	cm_polygonRef_t* pref;
	cm_polygon_t* p;
	int i;
	
	while( 1 )
	{
		for( pref = node->polygons; pref; pref = pref->next )
		{
			p = pref->p;
			// if we checked this polygon already
			if( p->checkcount == checkCount )
			{
				continue;
			}
			p->checkcount = checkCount;
			for( i = 0; i < p->numEdges; i++ )
			{
				if( p->edges[i] < 0 )
				{
					p->edges[i] = -edgeRemap[ abs( p->edges[i] ) ];
				}
				else
				{
					p->edges[i] = edgeRemap[ p->edges[i] ];
				}
			}
		}
		if( node->planeType == -1 )
		{
			break;
		}
		
		RemapEdges( node->children[1], edgeRemap );
		node = node->children[0];
	}
}

/*
==================
idCollisionModelManagerLocal::OptimizeArrays

  due to polygon merging and polygon removal the vertex and edge array
  can have a lot of unused entries.
==================
*/
void idCollisionModelManagerLocal::OptimizeArrays( cm_model_t* model )
{
	int i, newNumVertices, newNumEdges, *v;
	int* remap;
	cm_edge_t* oldEdges;
	cm_vertex_t* oldVertices;
	
	remap = ( int* ) Mem_ClearedAlloc( Max( model->numVertices, model->numEdges ) * sizeof( int ), TAG_COLLISION );
	// get all used vertices
	for( i = 0; i < model->numEdges; i++ )
	{
		remap[ model->edges[i].vertexNum[0] ] = true;
		remap[ model->edges[i].vertexNum[1] ] = true;
	}
	// create remap index and move vertices
	newNumVertices = 0;
	for( i = 0; i < model->numVertices; i++ )
	{
		if( remap[ i ] )
		{
			remap[ i ] = newNumVertices;
			model->vertices[ newNumVertices ] = model->vertices[ i ];
			newNumVertices++;
		}
	}
	model->numVertices = newNumVertices;
	// change edge vertex indexes
	for( i = 1; i < model->numEdges; i++ )
	{
		v = model->edges[i].vertexNum;
		v[0] = remap[ v[0] ];
		v[1] = remap[ v[1] ];
	}
	
	// create remap index and move edges
	newNumEdges = 1;
	for( i = 1; i < model->numEdges; i++ )
	{
		// if the edge is used
		if( model->edges[ i ].numUsers )
		{
			remap[ i ] = newNumEdges;
			model->edges[ newNumEdges ] = model->edges[ i ];
			newNumEdges++;
		}
	}
	// change polygon edge indexes
	checkCount++;
	RemapEdges( model->node, remap );
	model->numEdges = newNumEdges;
	
	Mem_Free( remap );
	
	// realloc vertices
	oldVertices = model->vertices;
	model->maxVertices = model->numVertices;
	model->vertices = ( cm_vertex_t* ) Mem_ClearedAlloc( model->numVertices * sizeof( cm_vertex_t ), TAG_COLLISION );
	if( oldVertices )
	{
		memcpy( model->vertices, oldVertices, model->numVertices * sizeof( cm_vertex_t ) );
		Mem_Free( oldVertices );
	}
	
	// realloc edges
	oldEdges = model->edges;
	model->maxEdges = model->numEdges;
	model->edges = ( cm_edge_t* ) Mem_ClearedAlloc( model->numEdges * sizeof( cm_edge_t ), TAG_COLLISION );
	if( oldEdges )
	{
		memcpy( model->edges, oldEdges, model->numEdges * sizeof( cm_edge_t ) );
		Mem_Free( oldEdges );
	}
}

/*
================
idCollisionModelManagerLocal::FinishModel
================
*/
void idCollisionModelManagerLocal::FinishModel( cm_model_t* model )
{
	// try to merge polygons
	checkCount++;
	MergeTreePolygons( model, model->node );
	// find internal edges (no mesh can ever collide with internal edges)
	checkCount++;
	FindInternalEdges( model, model->node );
	// calculate edge normals
	checkCount++;
	CalculateEdgeNormals( model, model->node );
	
	//common->Printf( "%s vertex hash spread is %d\n", model->name.c_str(), cm_vertexHash->GetSpread() );
	//common->Printf( "%s edge hash spread is %d\n", model->name.c_str(), cm_edgeHash->GetSpread() );
	
	// remove all unused vertices and edges
	OptimizeArrays( model );
	// get model bounds from brush and polygon bounds
	CM_GetNodeBounds( &model->bounds, model->node );
	// get model contents
	model->contents = CM_GetNodeContents( model->node );
	// total memory used by this model
	model->usedMemory = model->numVertices * sizeof( cm_vertex_t ) +
						model->numEdges * sizeof( cm_edge_t ) +
						model->polygonMemory +
						model->brushMemory +
						model->numNodes * sizeof( cm_node_t ) +
						model->numPolygonRefs * sizeof( cm_polygonRef_t ) +
						model->numBrushRefs * sizeof( cm_brushRef_t );
}

static const byte BCM_VERSION = 100;
static const unsigned int BCM_MAGIC = ( 'B' << 24 ) | ( 'C' << 16 ) | ( 'M' << 16 ) | BCM_VERSION;

/*
================
idCollisionModelManagerLocal::LoadBinaryModel
================
*/
cm_model_t* idCollisionModelManagerLocal::LoadBinaryModelFromFile( idFile* file, ID_TIME_T sourceTimeStamp )
{

	unsigned int magic = 0;
	file->ReadBig( magic );
	if( magic != BCM_MAGIC )
	{
		return NULL;
	}
	ID_TIME_T storedTimeStamp = FILE_NOT_FOUND_TIMESTAMP;
	file->ReadBig( storedTimeStamp );
	
	// RB: source might be from .resources, so we ignore the time stamp and assume a release build
	if( !fileSystem->InProductionMode() && ( sourceTimeStamp != FILE_NOT_FOUND_TIMESTAMP ) && ( sourceTimeStamp != 0 ) && ( sourceTimeStamp != storedTimeStamp ) )
	{
		return NULL;
	}
	// RB end
	
	cm_model_t* model = AllocModel();
	file->ReadString( model->name );
	file->ReadBig( model->bounds );
	file->ReadBig( model->contents );
	file->ReadBig( model->isConvex );
	file->ReadBig( model->numVertices );
	file->ReadBig( model->numEdges );
	file->ReadBig( model->numPolygons );
	file->ReadBig( model->numBrushes );
	file->ReadBig( model->numNodes );
	file->ReadBig( model->numBrushRefs );
	file->ReadBig( model->numPolygonRefs );
	file->ReadBig( model->numInternalEdges );
	file->ReadBig( model->numSharpEdges );
	file->ReadBig( model->numRemovedPolys );
	file->ReadBig( model->numMergedPolys );
	
	model->maxVertices = model->numVertices;
	model->vertices = ( cm_vertex_t* ) Mem_ClearedAlloc( model->maxVertices * sizeof( cm_vertex_t ), TAG_COLLISION );
	for( int i = 0; i < model->numVertices; i++ )
	{
		file->ReadBig( model->vertices[i].p );
		file->ReadBig( model->vertices[i].checkcount );
		file->ReadBig( model->vertices[i].side );
		file->ReadBig( model->vertices[i].sideSet );
	}
	
	model->maxEdges = model->numEdges;
	model->edges = ( cm_edge_t* ) Mem_ClearedAlloc( model->maxEdges * sizeof( cm_edge_t ), TAG_COLLISION );
	for( int i = 0; i < model->numEdges; i++ )
	{
		file->ReadBig( model->edges[i].checkcount );
		file->ReadBig( model->edges[i].internal );
		file->ReadBig( model->edges[i].numUsers );
		file->ReadBig( model->edges[i].side );
		file->ReadBig( model->edges[i].sideSet );
		file->ReadBig( model->edges[i].vertexNum[0] );
		file->ReadBig( model->edges[i].vertexNum[1] );
		file->ReadBig( model->edges[i].normal );
	}
	
	file->ReadBig( model->polygonMemory );
	model->polygonBlock = ( cm_polygonBlock_t* ) Mem_ClearedAlloc( sizeof( cm_polygonBlock_t ) + model->polygonMemory, TAG_COLLISION );
	model->polygonBlock->bytesRemaining = model->polygonMemory;
	model->polygonBlock->next = ( ( byte* ) model->polygonBlock ) + sizeof( cm_polygonBlock_t );
	
	file->ReadBig( model->brushMemory );
	model->brushBlock = ( cm_brushBlock_t* ) Mem_ClearedAlloc( sizeof( cm_brushBlock_t ) + model->brushMemory, TAG_COLLISION );
	model->brushBlock->bytesRemaining = model->brushMemory;
	model->brushBlock->next = ( ( byte* ) model->brushBlock ) + sizeof( cm_brushBlock_t );
	
	int numMaterials = 0;
	file->ReadBig( numMaterials );
	
	idList< const idMaterial* > materials;
	materials.SetNum( numMaterials );
	idStr materialName;
	for( int i = 0; i < materials.Num(); i++ )
	{
		file->ReadString( materialName );
		if( materialName.IsEmpty() )
		{
			materials[i] = NULL;
		}
		else
		{
			materials[i] = declManager->FindMaterial( materialName );
		}
	}
	idList< cm_polygon_t* > polys;
	idList< cm_brush_t* > brushes;
	polys.SetNum( model->numPolygons );
	brushes.SetNum( model->numBrushes );
	for( int i = 0; i < polys.Num(); i++ )
	{
		int materialIndex = 0;
		file->ReadBig( materialIndex );
		int numEdges = 0;
		file->ReadBig( numEdges );
		polys[i] = AllocPolygon( model, numEdges );
		polys[i]->numEdges = numEdges;
		polys[i]->material = materials[materialIndex];
		file->ReadBig( polys[i]->bounds );
		file->ReadBig( polys[i]->checkcount );
		file->ReadBig( polys[i]->contents );
		file->ReadBig( polys[i]->plane );
		file->ReadBigArray( polys[i]->edges, polys[i]->numEdges );
	}
	for( int i = 0; i < brushes.Num(); i++ )
	{
		int materialIndex = 0;
		file->ReadBig( materialIndex );
		int numPlanes = 0;
		file->ReadBig( numPlanes );
		brushes[i] = AllocBrush( model, numPlanes );
		brushes[i]->numPlanes = numPlanes;
		brushes[i]->material = materials[materialIndex];
		file->ReadBig( brushes[i]->checkcount );
		file->ReadBig( brushes[i]->bounds );
		file->ReadBig( brushes[i]->contents );
		file->ReadBig( brushes[i]->primitiveNum );
		file->ReadBigArray( brushes[i]->planes, brushes[i]->numPlanes );
	}
	struct local
	{
		static void ReadNodeTree( idFile* file, cm_model_t* model, cm_node_t* node, idList< cm_polygon_t* >& polys, idList< cm_brush_t* >& brushes )
		{
			file->ReadBig( node->planeType );
			file->ReadBig( node->planeDist );
			int i = 0;
			while( file->ReadBig( i ) == sizeof( i ) && ( i >= 0 ) )
			{
				cm_polygonRef_t* pref = collisionModelManagerLocal.AllocPolygonReference( model, model->numPolygonRefs );
				pref->p = polys[i];
				pref->next = node->polygons;
				node->polygons = pref;
			}
			while( file->ReadBig( i ) == sizeof( i ) && ( i >= 0 ) )
			{
				cm_brushRef_t* bref = collisionModelManagerLocal.AllocBrushReference( model, model->numBrushRefs );
				bref->b = brushes[i];
				bref->next = node->brushes;
				node->brushes = bref;
			}
			if( node->planeType != -1 )
			{
				node->children[0] = collisionModelManagerLocal.AllocNode( model, model->numNodes );
				node->children[1] = collisionModelManagerLocal.AllocNode( model, model->numNodes );
				node->children[0]->parent = node;
				node->children[1]->parent = node;
				ReadNodeTree( file, model, node->children[0], polys, brushes );
				ReadNodeTree( file, model, node->children[1], polys, brushes );
			}
		}
	};
	model->node = AllocNode( model, model->numNodes + 1 );
	local::ReadNodeTree( file, model, model->node, polys, brushes );
	
	// We should have only allocated a single block, and used every entry in the block
	// assert( model->nodeBlocks != NULL && model->nodeBlocks->next == NULL && model->nodeBlocks->nextNode == NULL );
	assert( model->brushRefBlocks == NULL || ( model->brushRefBlocks->next == NULL && model->brushRefBlocks->nextRef == NULL ) );
	assert( model->polygonRefBlocks == NULL || ( model->polygonRefBlocks->next == NULL && model->polygonRefBlocks->nextRef == NULL ) );
	
	// RB: FIXME
#if !defined(__x86_64__) && !defined(_WIN64)
	assert( model->polygonBlock->bytesRemaining == 0 );
	assert( model->brushBlock->bytesRemaining == 0 );
#endif
	// RB end
	
	model->usedMemory = model->numVertices * sizeof( cm_vertex_t ) +
						model->numEdges * sizeof( cm_edge_t ) +
						model->polygonMemory +
						model->brushMemory +
						model->numNodes * sizeof( cm_node_t ) +
						model->numPolygonRefs * sizeof( cm_polygonRef_t ) +
						model->numBrushRefs * sizeof( cm_brushRef_t );
	return model;
}

/*
================
idCollisionModelManagerLocal::LoadBinaryModel
================
*/
cm_model_t* idCollisionModelManagerLocal::LoadBinaryModel( const char* fileName, ID_TIME_T sourceTimeStamp )
{
	idFileLocal file( fileSystem->OpenFileReadMemory( fileName ) );
	if( file == NULL )
	{
		return NULL;
	}
	return LoadBinaryModelFromFile( file, sourceTimeStamp );
}

/*
================
idCollisionModelManagerLocal::WriteBinaryModel
================
*/
void idCollisionModelManagerLocal::WriteBinaryModelToFile( cm_model_t* model, idFile* file, ID_TIME_T sourceTimeStamp )
{

	file->WriteBig( BCM_MAGIC );
	file->WriteBig( sourceTimeStamp );
	file->WriteString( model->name );
	file->WriteBig( model->bounds );
	file->WriteBig( model->contents );
	file->WriteBig( model->isConvex );
	file->WriteBig( model->numVertices );
	file->WriteBig( model->numEdges );
	file->WriteBig( model->numPolygons );
	file->WriteBig( model->numBrushes );
	file->WriteBig( model->numNodes );
	file->WriteBig( model->numBrushRefs );
	file->WriteBig( model->numPolygonRefs );
	file->WriteBig( model->numInternalEdges );
	file->WriteBig( model->numSharpEdges );
	file->WriteBig( model->numRemovedPolys );
	file->WriteBig( model->numMergedPolys );
	for( int i = 0; i < model->numVertices; i++ )
	{
		file->WriteBig( model->vertices[i].p );
		file->WriteBig( model->vertices[i].checkcount );
		file->WriteBig( model->vertices[i].side );
		file->WriteBig( model->vertices[i].sideSet );
	}
	for( int i = 0; i < model->numEdges; i++ )
	{
		file->WriteBig( model->edges[i].checkcount );
		file->WriteBig( model->edges[i].internal );
		file->WriteBig( model->edges[i].numUsers );
		file->WriteBig( model->edges[i].side );
		file->WriteBig( model->edges[i].sideSet );
		file->WriteBig( model->edges[i].vertexNum[0] );
		file->WriteBig( model->edges[i].vertexNum[1] );
		file->WriteBig( model->edges[i].normal );
	}
	file->WriteBig( model->polygonMemory );
	file->WriteBig( model->brushMemory );
	struct local
	{
		static void BuildUniqueLists( cm_node_t* node, idList< cm_polygon_t* >& polys, idList< cm_brush_t* >& brushes )
		{
			for( cm_polygonRef_t* pr = node->polygons; pr != NULL; pr = pr->next )
			{
				polys.AddUnique( pr->p );
			}
			for( cm_brushRef_t* br = node->brushes; br != NULL; br = br->next )
			{
				brushes.AddUnique( br->b );
			}
			if( node->planeType != -1 )
			{
				BuildUniqueLists( node->children[0], polys, brushes );
				BuildUniqueLists( node->children[1], polys, brushes );
			}
		}
		static void WriteNodeTree( idFile* file, cm_node_t* node, idList< cm_polygon_t* >& polys, idList< cm_brush_t* >& brushes )
		{
			file->WriteBig( node->planeType );
			file->WriteBig( node->planeDist );
			for( cm_polygonRef_t* pr = node->polygons; pr != NULL; pr = pr->next )
			{
				file->WriteBig( polys.FindIndex( pr->p ) );
			}
			file->WriteBig( -1 );
			for( cm_brushRef_t* br = node->brushes; br != NULL; br = br->next )
			{
				file->WriteBig( brushes.FindIndex( br->b ) );
			}
			file->WriteBig( -1 );
			if( node->planeType != -1 )
			{
				WriteNodeTree( file, node->children[0], polys, brushes );
				WriteNodeTree( file, node->children[1], polys, brushes );
			}
		}
	};
	idList< cm_polygon_t* > polys;
	idList< cm_brush_t* > brushes;
	local::BuildUniqueLists( model->node, polys, brushes );
	assert( polys.Num() == model->numPolygons );
	assert( brushes.Num() == model->numBrushes );
	
	idList< const idMaterial* > materials;
	for( int i = 0; i < polys.Num(); i++ )
	{
		materials.AddUnique( polys[i]->material );
	}
	for( int i = 0; i < brushes.Num(); i++ )
	{
		materials.AddUnique( brushes[i]->material );
	}
	file->WriteBig( materials.Num() );
	for( int i = 0; i < materials.Num(); i++ )
	{
		if( materials[i] == NULL )
		{
			file->WriteString( "" );
		}
		else
		{
			file->WriteString( materials[i]->GetName() );
		}
	}
	for( int i = 0; i < polys.Num(); i++ )
	{
		file->WriteBig( ( int )materials.FindIndex( polys[i]->material ) );
		file->WriteBig( polys[i]->numEdges );
		file->WriteBig( polys[i]->bounds );
		file->WriteBig( polys[i]->checkcount );
		file->WriteBig( polys[i]->contents );
		file->WriteBig( polys[i]->plane );
		file->WriteBigArray( polys[i]->edges, polys[i]->numEdges );
	}
	for( int i = 0; i < brushes.Num(); i++ )
	{
		file->WriteBig( ( int )materials.FindIndex( brushes[i]->material ) );
		file->WriteBig( brushes[i]->numPlanes );
		file->WriteBig( brushes[i]->checkcount );
		file->WriteBig( brushes[i]->bounds );
		file->WriteBig( brushes[i]->contents );
		file->WriteBig( brushes[i]->primitiveNum );
		file->WriteBigArray( brushes[i]->planes, brushes[i]->numPlanes );
	}
	local::WriteNodeTree( file, model->node, polys, brushes );
}

/*
================
idCollisionModelManagerLocal::WriteBinaryModel
================
*/
void idCollisionModelManagerLocal::WriteBinaryModel( cm_model_t* model, const char* fileName, ID_TIME_T sourceTimeStamp )
{
	idFileLocal file( fileSystem->OpenFileWrite( fileName, "fs_basepath" ) );
	if( file == NULL )
	{
		common->Printf( "Failed to open %s\n", fileName );
		return;
	}
	WriteBinaryModelToFile( model, file, sourceTimeStamp );
}

/*
================
idCollisionModelManagerLocal::LoadRenderModel
================
*/
cm_model_t* idCollisionModelManagerLocal::LoadRenderModel( const char* fileName )
{
	int i, j;
	idRenderModel* renderModel;
	const modelSurface_t* surf;
	idFixedWinding w;
	cm_node_t* node;
	cm_model_t* model;
	idPlane plane;
	idBounds bounds;
	bool collisionSurface;
	idStr extension;
	
	// only load ASE and LWO models
	idStr( fileName ).ExtractFileExtension( extension );
	if( ( extension.Icmp( "ase" ) != 0 ) && ( extension.Icmp( "lwo" ) != 0 ) && ( extension.Icmp( "ma" ) != 0 ) )
	{
		return NULL;
	}
	
	renderModel = renderModelManager->CheckModel( fileName );
	if( !renderModel )
	{
		return NULL;
	}
	
	idStrStatic< MAX_OSPATH > generatedFileName = "generated/collision/";
	generatedFileName.AppendPath( fileName );
	generatedFileName.SetFileExtension( CMODEL_BINARYFILE_EXT );
	
	ID_TIME_T sourceTimeStamp = renderModel->Timestamp();
	model = LoadBinaryModel( generatedFileName, sourceTimeStamp );
	if( model != NULL )
	{
		return model;
	}
	idLib::Printf( "Writing %s\n", generatedFileName.c_str() );
	
	model = AllocModel();
	model->name = fileName;
	node = AllocNode( model, NODE_BLOCK_SIZE_SMALL );
	node->planeType = -1;
	model->node = node;
	
	model->maxVertices = 0;
	model->numVertices = 0;
	model->maxEdges = 0;
	model->numEdges = 0;
	
	bounds = renderModel->Bounds( NULL );
	
	collisionSurface = false;
	for( i = 0; i < renderModel->NumSurfaces(); i++ )
	{
		surf = renderModel->Surface( i );
		if( surf->shader->GetSurfaceFlags() & SURF_COLLISION )
		{
			collisionSurface = true;
		}
	}
	
	for( i = 0; i < renderModel->NumSurfaces(); i++ )
	{
		surf = renderModel->Surface( i );
		// if this surface has no contents
		if( !( surf->shader->GetContentFlags() & CONTENTS_REMOVE_UTIL ) )
		{
			continue;
		}
		// if the model has a collision surface and this surface is not a collision surface
		if( collisionSurface && !( surf->shader->GetSurfaceFlags() & SURF_COLLISION ) )
		{
			continue;
		}
		// get max verts and edges
		model->maxVertices += surf->geometry->numVerts;
		model->maxEdges += surf->geometry->numIndexes;
	}
	
	model->vertices = ( cm_vertex_t* ) Mem_ClearedAlloc( model->maxVertices * sizeof( cm_vertex_t ), TAG_COLLISION );
	model->edges = ( cm_edge_t* ) Mem_ClearedAlloc( model->maxEdges * sizeof( cm_edge_t ), TAG_COLLISION );
	
	// setup hash to speed up finding shared vertices and edges
	SetupHash();
	
	cm_vertexHash->ResizeIndex( model->maxVertices );
	cm_edgeHash->ResizeIndex( model->maxEdges );
	
	ClearHash( bounds );
	
	for( i = 0; i < renderModel->NumSurfaces(); i++ )
	{
		surf = renderModel->Surface( i );
		// if this surface has no contents
		if( !( surf->shader->GetContentFlags() & CONTENTS_REMOVE_UTIL ) )
		{
			continue;
		}
		// if the model has a collision surface and this surface is not a collision surface
		if( collisionSurface && !( surf->shader->GetSurfaceFlags() & SURF_COLLISION ) )
		{
			continue;
		}
		
		for( j = 0; j < surf->geometry->numIndexes; j += 3 )
		{
			w.Clear();
			w += surf->geometry->verts[ surf->geometry->indexes[ j + 2 ] ].xyz;
			w += surf->geometry->verts[ surf->geometry->indexes[ j + 1 ] ].xyz;
			w += surf->geometry->verts[ surf->geometry->indexes[ j + 0 ] ].xyz;
			w.GetPlane( plane );
			plane = -plane;
			PolygonFromWinding( model, &w, plane, surf->shader, 1 );
		}
	}
	
	// create a BSP tree for the model
	model->node = CreateAxialBSPTree( model, model->node );
	
	model->isConvex = false;
	
	FinishModel( model );
	
	// shutdown the hash
	ShutdownHash();
	
	WriteBinaryModel( model, generatedFileName, sourceTimeStamp );
	
	return model;
}

/*
================
idCollisionModelManagerLocal::CollisionModelForMapEntity
================
*/
cm_model_t* idCollisionModelManagerLocal::CollisionModelForMapEntity( const idMapEntity* mapEnt )
{

	cm_model_t* model;
	idBounds bounds;
	const char* name;
	int i, brushCount;
	
	// if the entity has no primitives
	if( mapEnt->GetNumPrimitives() < 1 )
	{
		return NULL;
	}
	
	// get a name for the collision model
	mapEnt->epairs.GetString( "model", "", &name );
	if( !name[0] )
	{
		mapEnt->epairs.GetString( "name", "", &name );
		if( !name[0] )
		{
			if( !numModels )
			{
				// first model is always the world
				name = "worldMap";
			}
			else
			{
				name = "unnamed inline model";
			}
		}
	}
	
	model = AllocModel();
	model->node = AllocNode( model, NODE_BLOCK_SIZE_SMALL );
	
	CM_EstimateVertsAndEdges( mapEnt, &model->maxVertices, &model->maxEdges );
	model->numVertices = 0;
	model->numEdges = 0;
	model->vertices = ( cm_vertex_t* ) Mem_ClearedAlloc( model->maxVertices * sizeof( cm_vertex_t ), TAG_COLLISION );
	model->edges = ( cm_edge_t* ) Mem_ClearedAlloc( model->maxEdges * sizeof( cm_edge_t ), TAG_COLLISION );
	
	cm_vertexHash->ResizeIndex( model->maxVertices );
	cm_edgeHash->ResizeIndex( model->maxEdges );
	
	model->name = name;
	model->isConvex = false;
	
	// convert brushes
	for( i = 0; i < mapEnt->GetNumPrimitives(); i++ )
	{
		idMapPrimitive*	mapPrim;
		
		mapPrim = mapEnt->GetPrimitive( i );
		if( mapPrim->GetType() == idMapPrimitive::TYPE_BRUSH )
		{
			ConvertBrush( model, static_cast<idMapBrush*>( mapPrim ), i );
			continue;
		}
	}
	
	// create an axial bsp tree for the model if it has more than just a bunch brushes
	brushCount = CM_CountNodeBrushes( model->node );
	if( brushCount > 4 )
	{
		model->node = CreateAxialBSPTree( model, model->node );
	}
	else
	{
		model->node->planeType = -1;
	}
	
	// get bounds for hash
	if( brushCount )
	{
		CM_GetNodeBounds( &bounds, model->node );
	}
	else
	{
		bounds[0].Set( -256, -256, -256 );
		bounds[1].Set( 256, 256, 256 );
	}
	
	// different models do not share edges and vertices with each other, so clear the hash
	ClearHash( bounds );
	
	// create polygons from patches and brushes
	for( i = 0; i < mapEnt->GetNumPrimitives(); i++ )
	{
		idMapPrimitive*	mapPrim;
		
		mapPrim = mapEnt->GetPrimitive( i );
		if( mapPrim->GetType() == idMapPrimitive::TYPE_PATCH )
		{
			ConvertPatch( model, static_cast<idMapPatch*>( mapPrim ), i );
			continue;
		}
		if( mapPrim->GetType() == idMapPrimitive::TYPE_BRUSH )
		{
			ConvertBrushSides( model, static_cast<idMapBrush*>( mapPrim ), i );
			continue;
		}
	}
	
	FinishModel( model );
	
	return model;
}

/*
================
idCollisionModelManagerLocal::FindModel
================
*/
cmHandle_t idCollisionModelManagerLocal::FindModel( const char* name )
{
	int i;
	
	// check if this model is already loaded
	for( i = 0; i < numModels; i++ )
	{
		if( !models[i]->name.Icmp( name ) )
		{
			break;
		}
	}
	// if the model is already loaded
	if( i < numModels )
	{
		return i;
	}
	return -1;
}

/*
==================
idCollisionModelManagerLocal::PrintModelInfo
==================
*/
void idCollisionModelManagerLocal::PrintModelInfo( const cm_model_t* model )
{
	common->Printf( "%6i vertices (%i KB)\n", model->numVertices, ( model->numVertices * sizeof( cm_vertex_t ) ) >> 10 );
	common->Printf( "%6i edges (%i KB)\n", model->numEdges, ( model->numEdges * sizeof( cm_edge_t ) ) >> 10 );
	common->Printf( "%6i polygons (%i KB)\n", model->numPolygons, model->polygonMemory >> 10 );
	common->Printf( "%6i brushes (%i KB)\n", model->numBrushes, model->brushMemory >> 10 );
	common->Printf( "%6i nodes (%i KB)\n", model->numNodes, ( model->numNodes * sizeof( cm_node_t ) ) >> 10 );
	common->Printf( "%6i polygon refs (%i KB)\n", model->numPolygonRefs, ( model->numPolygonRefs * sizeof( cm_polygonRef_t ) ) >> 10 );
	common->Printf( "%6i brush refs (%i KB)\n", model->numBrushRefs, ( model->numBrushRefs * sizeof( cm_brushRef_t ) ) >> 10 );
	common->Printf( "%6i internal edges\n", model->numInternalEdges );
	common->Printf( "%6i sharp edges\n", model->numSharpEdges );
	common->Printf( "%6i contained polygons removed\n", model->numRemovedPolys );
	common->Printf( "%6i polygons merged\n", model->numMergedPolys );
	common->Printf( "%6i KB total memory used\n", model->usedMemory >> 10 );
}

/*
================
idCollisionModelManagerLocal::AccumulateModelInfo
================
*/
void idCollisionModelManagerLocal::AccumulateModelInfo( cm_model_t* model )
{
	int i;
	
	memset( model, 0, sizeof( *model ) );
	// accumulate statistics of all loaded models
	for( i = 0; i < numModels; i++ )
	{
		model->numVertices += models[i]->numVertices;
		model->numEdges += models[i]->numEdges;
		model->numPolygons += models[i]->numPolygons;
		model->polygonMemory += models[i]->polygonMemory;
		model->numBrushes += models[i]->numBrushes;
		model->brushMemory += models[i]->brushMemory;
		model->numNodes += models[i]->numNodes;
		model->numBrushRefs += models[i]->numBrushRefs;
		model->numPolygonRefs += models[i]->numPolygonRefs;
		model->numInternalEdges += models[i]->numInternalEdges;
		model->numSharpEdges += models[i]->numSharpEdges;
		model->numRemovedPolys += models[i]->numRemovedPolys;
		model->numMergedPolys += models[i]->numMergedPolys;
		model->usedMemory += models[i]->usedMemory;
	}
}

/*
================
idCollisionModelManagerLocal::ModelInfo
================
*/
void idCollisionModelManagerLocal::ModelInfo( cmHandle_t model )
{
	cm_model_t modelInfo;
	
	if( model == -1 )
	{
		AccumulateModelInfo( &modelInfo );
		PrintModelInfo( &modelInfo );
		return;
	}
	if( model < 0 || model > MAX_SUBMODELS || model > maxModels )
	{
		common->Printf( "idCollisionModelManagerLocal::ModelInfo: invalid model handle\n" );
		return;
	}
	if( !models[model] )
	{
		common->Printf( "idCollisionModelManagerLocal::ModelInfo: invalid model\n" );
		return;
	}
	
	PrintModelInfo( models[model] );
}

/*
================
idCollisionModelManagerLocal::ListModels
================
*/
void idCollisionModelManagerLocal::ListModels()
{
	int i, totalMemory;
	
	totalMemory = 0;
	for( i = 0; i < numModels; i++ )
	{
		common->Printf( "%4d: %5d KB   %s\n", i, ( models[i]->usedMemory >> 10 ), models[i]->name.c_str() );
		totalMemory += models[i]->usedMemory;
	}
	common->Printf( "%4d KB in %d models\n", ( totalMemory >> 10 ), numModels );
}

/*
================
idCollisionModelManagerLocal::BuildModels
================
*/
void idCollisionModelManagerLocal::BuildModels( const idMapFile* mapFile )
{
	int i;
	const idMapEntity* mapEnt;
	
	idTimer timer;
	timer.Start();
	
	if( !LoadCollisionModelFile( mapFile->GetName(), mapFile->GetGeometryCRC() ) )
	{
	
		if( !mapFile->GetNumEntities() )
		{
			return;
		}
		
		// load the .proc file bsp for data optimisation
		LoadProcBSP( mapFile->GetName() );
		
		// convert brushes and patches to collision data
		for( i = 0; i < mapFile->GetNumEntities(); i++ )
		{
			mapEnt = mapFile->GetEntity( i );
			
			if( numModels >= MAX_SUBMODELS )
			{
				common->Error( "idCollisionModelManagerLocal::BuildModels: more than %d collision models", MAX_SUBMODELS );
				break;
			}
			models[numModels] = CollisionModelForMapEntity( mapEnt );
			if( models[ numModels] )
			{
				numModels++;
			}
		}
		
		// free the proc bsp which is only used for data optimization
		Mem_Free( procNodes );
		procNodes = NULL;
		
		// write the collision models to a file
		WriteCollisionModelsToFile( mapFile->GetName(), 0, numModels, mapFile->GetGeometryCRC() );
	}
	
	timer.Stop();
	
	// print statistics on collision data
	cm_model_t model;
	AccumulateModelInfo( &model );
	common->Printf( "collision data:\n" );
	common->Printf( "%6i models\n", numModels );
	PrintModelInfo( &model );
	common->Printf( "%.0f msec to load collision data.\n", timer.Milliseconds() );
}


/*
================
idCollisionModelManagerLocal::Preload
================
*/
void idCollisionModelManagerLocal::Preload( const char* mapName )
{

	if( !preLoad_Collision.GetBool() )
	{
		return;
	}
	idStrStatic< MAX_OSPATH > manifestName = mapName;
	manifestName.Replace( "game/", "maps/" );
	manifestName.Replace( "maps/maps/", "maps/" );
	manifestName.SetFileExtension( ".preload" );
	idPreloadManifest manifest;
	manifest.LoadManifest( manifestName );
	if( manifest.NumResources() >= 0 )
	{
		common->Printf( "Preloading collision models...\n" );
		int	start = Sys_Milliseconds();
		int numLoaded = 0;
		for( int i = 0; i < manifest.NumResources(); i++ )
		{
			const preloadEntry_s& p = manifest.GetPreloadByIndex( i );
			if( p.resType == PRELOAD_COLLISION )
			{
				LoadModel( p.resourceName );
				numLoaded++;
			}
		}
		int	end = Sys_Milliseconds();
		common->Printf( "%05d collision models preloaded ( or were already loaded ) in %5.1f seconds\n", numLoaded, ( end - start ) * 0.001 );
		common->Printf( "----------------------------------------\n" );
	}
}

/*
================
idCollisionModelManagerLocal::LoadMap
================
*/
void idCollisionModelManagerLocal::LoadMap( const idMapFile* mapFile )
{

	if( mapFile == NULL )
	{
		common->Error( "idCollisionModelManagerLocal::LoadMap: NULL mapFile" );
		return;
	}
	
	// check whether we can keep the current collision map based on the mapName and mapFileTime
	if( loaded )
	{
		if( mapName.Icmp( mapFile->GetName() ) == 0 )
		{
			if( mapFile->GetFileTime() == mapFileTime )
			{
				common->DPrintf( "Using loaded version\n" );
				return;
			}
			common->DPrintf( "Reloading modified map\n" );
		}
		FreeMap();
	}
	
	// clear the collision map
	Clear();
	
	// models
	maxModels = MAX_SUBMODELS;
	numModels = 0;
	models = ( cm_model_t** ) Mem_ClearedAlloc( ( maxModels + 1 ) * sizeof( cm_model_t* ), TAG_COLLISION );
	
	// setup hash to speed up finding shared vertices and edges
	SetupHash();
	
	common->UpdateLevelLoadPacifier();
	
	// setup trace model structure
	SetupTrmModelStructure();
	
	common->UpdateLevelLoadPacifier();
	
	// build collision models
	BuildModels( mapFile );
	
	common->UpdateLevelLoadPacifier();
	
	// save name and time stamp
	mapName = mapFile->GetName();
	mapFileTime = mapFile->GetFileTime();
	loaded = true;
	
	// shutdown the hash
	ShutdownHash();
}

/*
===================
idCollisionModelManagerLocal::GetModelName
===================
*/
const char* idCollisionModelManagerLocal::GetModelName( cmHandle_t model ) const
{
	if( model < 0 || model > MAX_SUBMODELS || model >= numModels || !models[model] )
	{
		common->Printf( "idCollisionModelManagerLocal::GetModelBounds: invalid model handle\n" );
		return "";
	}
	return models[model]->name.c_str();
}

/*
===================
idCollisionModelManagerLocal::GetModelBounds
===================
*/
bool idCollisionModelManagerLocal::GetModelBounds( cmHandle_t model, idBounds& bounds ) const
{

	if( model < 0 || model > MAX_SUBMODELS || model >= numModels || !models[model] )
	{
		common->Printf( "idCollisionModelManagerLocal::GetModelBounds: invalid model handle\n" );
		return false;
	}
	
	bounds = models[model]->bounds;
	return true;
}

/*
===================
idCollisionModelManagerLocal::GetModelContents
===================
*/
bool idCollisionModelManagerLocal::GetModelContents( cmHandle_t model, int& contents ) const
{
	if( model < 0 || model > MAX_SUBMODELS || model >= numModels || !models[model] )
	{
		common->Printf( "idCollisionModelManagerLocal::GetModelContents: invalid model handle\n" );
		return false;
	}
	
	contents = models[model]->contents;
	
	return true;
}

/*
===================
idCollisionModelManagerLocal::GetModelVertex
===================
*/
bool idCollisionModelManagerLocal::GetModelVertex( cmHandle_t model, int vertexNum, idVec3& vertex ) const
{
	if( model < 0 || model > MAX_SUBMODELS || model >= numModels || !models[model] )
	{
		common->Printf( "idCollisionModelManagerLocal::GetModelVertex: invalid model handle\n" );
		return false;
	}
	
	if( vertexNum < 0 || vertexNum >= models[model]->numVertices )
	{
		common->Printf( "idCollisionModelManagerLocal::GetModelVertex: invalid vertex number\n" );
		return false;
	}
	
	vertex = models[model]->vertices[vertexNum].p;
	
	return true;
}

/*
===================
idCollisionModelManagerLocal::GetModelEdge
===================
*/
bool idCollisionModelManagerLocal::GetModelEdge( cmHandle_t model, int edgeNum, idVec3& start, idVec3& end ) const
{
	if( model < 0 || model > MAX_SUBMODELS || model >= numModels || !models[model] )
	{
		common->Printf( "idCollisionModelManagerLocal::GetModelEdge: invalid model handle\n" );
		return false;
	}
	
	edgeNum = abs( edgeNum );
	if( edgeNum >= models[model]->numEdges )
	{
		common->Printf( "idCollisionModelManagerLocal::GetModelEdge: invalid edge number\n" );
		return false;
	}
	
	start = models[model]->vertices[models[model]->edges[edgeNum].vertexNum[0]].p;
	end = models[model]->vertices[models[model]->edges[edgeNum].vertexNum[1]].p;
	
	return true;
}

/*
===================
idCollisionModelManagerLocal::GetModelPolygon
===================
*/
bool idCollisionModelManagerLocal::GetModelPolygon( cmHandle_t model, int polygonNum, idFixedWinding& winding ) const
{
	int i, edgeNum;
	cm_polygon_t* poly;
	
	if( model < 0 || model > MAX_SUBMODELS || model >= numModels || !models[model] )
	{
		common->Printf( "idCollisionModelManagerLocal::GetModelPolygon: invalid model handle\n" );
		return false;
	}
	
	poly = *reinterpret_cast<cm_polygon_t**>( &polygonNum );
	winding.Clear();
	for( i = 0; i < poly->numEdges; i++ )
	{
		edgeNum = poly->edges[i];
		winding += models[model]->vertices[ models[model]->edges[abs( edgeNum )].vertexNum[INT32_SIGNBITSET( edgeNum )] ].p;
	}
	
	return true;
}

/*
==================
idCollisionModelManagerLocal::LoadModel
==================
*/
cmHandle_t idCollisionModelManagerLocal::LoadModel( const char* modelName )
{
	int handle;
	
	handle = FindModel( modelName );
	if( handle >= 0 )
	{
		return handle;
	}
	
	if( numModels >= MAX_SUBMODELS )
	{
		common->Error( "idCollisionModelManagerLocal::LoadModel: no free slots\n" );
		return 0;
	}
	
	idStrStatic< MAX_OSPATH > generatedFileName = "generated/collision/";
	generatedFileName.AppendPath( modelName );
	generatedFileName.SetFileExtension( CMODEL_BINARYFILE_EXT );
	
	ID_TIME_T sourceTimeStamp = fileSystem->GetTimestamp( modelName );
	
	if( models == NULL )
	{
		// raynorpat: best clear this if there are no models ( hit by dmap )
		maxModels = MAX_SUBMODELS;
		numModels = 0;
		models = ( cm_model_t** ) Mem_ClearedAlloc( ( maxModels + 1 ) * sizeof( cm_model_t* ), TAG_COLLISION );
	}
	
	models[ numModels ] = LoadBinaryModel( generatedFileName, sourceTimeStamp );
	if( models[ numModels ] != NULL )
	{
		numModels++;
		if( cvarSystem->GetCVarBool( "fs_buildresources" ) )
		{
			// for resource gathering write this model to the preload file for this map
			fileSystem->AddCollisionPreload( modelName );
		}
		return ( numModels - 1 );
	}
	
	// try to load a .cm file
	if( LoadCollisionModelFile( modelName, 0 ) )
	{
		handle = FindModel( modelName );
		if( handle >= 0  && handle < numModels )
		{
			cm_model_t* cm = models[ handle ];
			WriteBinaryModel( cm, generatedFileName, sourceTimeStamp );
			return handle;
		}
		else
		{
			common->Warning( "idCollisionModelManagerLocal::LoadModel: collision file for '%s' contains different model", modelName );
		}
	}
	
	// try to load a .ASE or .LWO model and convert it to a collision model
	models[ numModels ] = LoadRenderModel( modelName );
	if( models[ numModels ] != NULL )
	{
		numModels++;
		return ( numModels - 1 );
	}
	
	return 0;
}

/*
==================
idCollisionModelManagerLocal::TrmFromModel_r
==================
*/
bool idCollisionModelManagerLocal::TrmFromModel_r( idTraceModel& trm, cm_node_t* node )
{
	cm_polygonRef_t* pref;
	cm_polygon_t* p;
	int i;
	
	while( 1 )
	{
		for( pref = node->polygons; pref; pref = pref->next )
		{
			p = pref->p;
			
			if( p->checkcount == checkCount )
			{
				continue;
			}
			
			p->checkcount = checkCount;
			
			if( trm.numPolys >= MAX_TRACEMODEL_POLYS )
			{
				return false;
			}
			// copy polygon properties
			trm.polys[ trm.numPolys ].bounds = p->bounds;
			trm.polys[ trm.numPolys ].normal = p->plane.Normal();
			trm.polys[ trm.numPolys ].dist = p->plane.Dist();
			trm.polys[ trm.numPolys ].numEdges = p->numEdges;
			// copy edge index
			for( i = 0; i < p->numEdges; i++ )
			{
				trm.polys[ trm.numPolys ].edges[ i ] = p->edges[ i ];
			}
			trm.numPolys++;
		}
		if( node->planeType == -1 )
		{
			break;
		}
		if( !TrmFromModel_r( trm, node->children[1] ) )
		{
			return false;
		}
		node = node->children[0];
	}
	return true;
}

/*
==================
idCollisionModelManagerLocal::TrmFromModel

  NOTE: polygon merging can merge colinear edges and as such might cause dangling edges.
==================
*/
bool idCollisionModelManagerLocal::TrmFromModel( const cm_model_t* model, idTraceModel& trm )
{
	int i, j, numEdgeUsers[MAX_TRACEMODEL_EDGES + 1];
	
	// if the model has too many vertices to fit in a trace model
	if( model->numVertices > MAX_TRACEMODEL_VERTS )
	{
		common->Printf( "idCollisionModelManagerLocal::TrmFromModel: model %s has too many vertices.\n", model->name.c_str() );
		PrintModelInfo( model );
		return false;
	}
	
	// plus one because the collision model accounts for the first unused edge
	if( model->numEdges > MAX_TRACEMODEL_EDGES + 1 )
	{
		common->Printf( "idCollisionModelManagerLocal::TrmFromModel: model %s has too many edges.\n", model->name.c_str() );
		PrintModelInfo( model );
		return false;
	}
	
	trm.type = TRM_CUSTOM;
	trm.numVerts = 0;
	trm.numEdges = 1;
	trm.numPolys = 0;
	trm.bounds.Clear();
	
	// copy polygons
	checkCount++;
	if( !TrmFromModel_r( trm, model->node ) )
	{
		common->Printf( "idCollisionModelManagerLocal::TrmFromModel: model %s has too many polygons.\n", model->name.c_str() );
		PrintModelInfo( model );
		return false;
	}
	
	// copy vertices
	for( i = 0; i < model->numVertices; i++ )
	{
		trm.verts[ i ] = model->vertices[ i ].p;
		trm.bounds.AddPoint( trm.verts[ i ] );
	}
	trm.numVerts = model->numVertices;
	
	// copy edges
	for( i = 0; i < model->numEdges; i++ )
	{
		trm.edges[ i ].v[0] = model->edges[ i ].vertexNum[0];
		trm.edges[ i ].v[1] = model->edges[ i ].vertexNum[1];
	}
	// minus one because the collision model accounts for the first unused edge
	trm.numEdges = model->numEdges - 1;
	
	// each edge should be used exactly twice
	memset( numEdgeUsers, 0, sizeof( numEdgeUsers ) );
	for( i = 0; i < trm.numPolys; i++ )
	{
		for( j = 0; j < trm.polys[i].numEdges; j++ )
		{
			numEdgeUsers[ abs( trm.polys[i].edges[j] ) ]++;
		}
	}
	for( i = 1; i <= trm.numEdges; i++ )
	{
		if( numEdgeUsers[i] != 2 )
		{
			common->Printf( "idCollisionModelManagerLocal::TrmFromModel: model %s has dangling edges, the model has to be an enclosed hull.\n", model->name.c_str() );
			PrintModelInfo( model );
			return false;
		}
	}
	
	// assume convex
	trm.isConvex = true;
	// check if really convex
	for( i = 0; i < trm.numPolys; i++ )
	{
		// to be convex no vertices should be in front of any polygon plane
		for( j = 0; j < trm.numVerts; j++ )
		{
			if( trm.polys[ i ].normal * trm.verts[ j ] - trm.polys[ i ].dist > 0.01f )
			{
				trm.isConvex = false;
				break;
			}
		}
		if( j < trm.numVerts )
		{
			break;
		}
	}
	
	// offset to center of model
	trm.offset = trm.bounds.GetCenter();
	
	trm.GenerateEdgeNormals();
	
	return true;
}

/*
==================
idCollisionModelManagerLocal::TrmFromModel
==================
*/
bool idCollisionModelManagerLocal::TrmFromModel( const char* modelName, idTraceModel& trm )
{
	cmHandle_t handle;
	
	handle = LoadModel( modelName );
	if( !handle )
	{
		common->Printf( "idCollisionModelManagerLocal::TrmFromModel: model %s not found.\n", modelName );
		return false;
	}
	
	return TrmFromModel( models[ handle ], trm );
}
