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

===============================================================================
*/

#pragma hdrstop
#include "precompiled.h"


#include "CollisionModel_local.h"

#define CM_FILE_EXT			"cm"
#define CM_BINARYFILE_EXT	"bcm"
#define CM_FILEID			"CM"
#define CM_FILEVERSION		"1.00"


/*
===============================================================================

Writing of collision model file

===============================================================================
*/

void CM_GetNodeBounds( idBounds* bounds, cm_node_t* node );
int CM_GetNodeContents( cm_node_t* node );


/*
================
idCollisionModelManagerLocal::WriteNodes
================
*/
void idCollisionModelManagerLocal::WriteNodes( idFile* fp, cm_node_t* node )
{
	fp->WriteFloatString( "\t( %d %f )\n", node->planeType, node->planeDist );
	if( node->planeType != -1 )
	{
		WriteNodes( fp, node->children[0] );
		WriteNodes( fp, node->children[1] );
	}
}

/*
================
idCollisionModelManagerLocal::CountPolygonMemory
================
*/
int idCollisionModelManagerLocal::CountPolygonMemory( cm_node_t* node ) const
{
	cm_polygonRef_t* pref;
	cm_polygon_t* p;
	int memory;
	
	memory = 0;
	for( pref = node->polygons; pref; pref = pref->next )
	{
		p = pref->p;
		if( p->checkcount == checkCount )
		{
			continue;
		}
		p->checkcount = checkCount;
		
		memory += sizeof( cm_polygon_t ) + ( p->numEdges - 1 ) * sizeof( p->edges[0] );
	}
	if( node->planeType != -1 )
	{
		memory += CountPolygonMemory( node->children[0] );
		memory += CountPolygonMemory( node->children[1] );
	}
	return memory;
}

/*
================
idCollisionModelManagerLocal::WritePolygons
================
*/
void idCollisionModelManagerLocal::WritePolygons( idFile* fp, cm_node_t* node )
{
	cm_polygonRef_t* pref;
	cm_polygon_t* p;
	int i;
	
	for( pref = node->polygons; pref; pref = pref->next )
	{
		p = pref->p;
		if( p->checkcount == checkCount )
		{
			continue;
		}
		p->checkcount = checkCount;
		fp->WriteFloatString( "\t%d (", p->numEdges );
		for( i = 0; i < p->numEdges; i++ )
		{
			fp->WriteFloatString( " %d", p->edges[i] );
		}
		fp->WriteFloatString( " ) ( %f %f %f ) %f", p->plane.Normal()[0], p->plane.Normal()[1], p->plane.Normal()[2], p->plane.Dist() );
		fp->WriteFloatString( " ( %f %f %f )", p->bounds[0][0], p->bounds[0][1], p->bounds[0][2] );
		fp->WriteFloatString( " ( %f %f %f )", p->bounds[1][0], p->bounds[1][1], p->bounds[1][2] );
		fp->WriteFloatString( " \"%s\"\n", p->material->GetName() );
	}
	if( node->planeType != -1 )
	{
		WritePolygons( fp, node->children[0] );
		WritePolygons( fp, node->children[1] );
	}
}

/*
================
idCollisionModelManagerLocal::CountBrushMemory
================
*/
int idCollisionModelManagerLocal::CountBrushMemory( cm_node_t* node ) const
{
	cm_brushRef_t* bref;
	cm_brush_t* b;
	int memory;
	
	memory = 0;
	for( bref = node->brushes; bref; bref = bref->next )
	{
		b = bref->b;
		if( b->checkcount == checkCount )
		{
			continue;
		}
		b->checkcount = checkCount;
		
		memory += sizeof( cm_brush_t ) + ( b->numPlanes - 1 ) * sizeof( b->planes[0] );
	}
	if( node->planeType != -1 )
	{
		memory += CountBrushMemory( node->children[0] );
		memory += CountBrushMemory( node->children[1] );
	}
	return memory;
}

/*
================
idCollisionModelManagerLocal::WriteBrushes
================
*/
void idCollisionModelManagerLocal::WriteBrushes( idFile* fp, cm_node_t* node )
{
	cm_brushRef_t* bref;
	cm_brush_t* b;
	int i;
	
	for( bref = node->brushes; bref; bref = bref->next )
	{
		b = bref->b;
		if( b->checkcount == checkCount )
		{
			continue;
		}
		b->checkcount = checkCount;
		fp->WriteFloatString( "\t%d {\n", b->numPlanes );
		for( i = 0; i < b->numPlanes; i++ )
		{
			fp->WriteFloatString( "\t\t( %f %f %f ) %f\n", b->planes[i].Normal()[0], b->planes[i].Normal()[1], b->planes[i].Normal()[2], b->planes[i].Dist() );
		}
		fp->WriteFloatString( "\t} ( %f %f %f )", b->bounds[0][0], b->bounds[0][1], b->bounds[0][2] );
		fp->WriteFloatString( " ( %f %f %f ) \"%s\"\n", b->bounds[1][0], b->bounds[1][1], b->bounds[1][2], StringFromContents( b->contents ) );
	}
	if( node->planeType != -1 )
	{
		WriteBrushes( fp, node->children[0] );
		WriteBrushes( fp, node->children[1] );
	}
}

/*
================
idCollisionModelManagerLocal::WriteCollisionModel
================
*/
void idCollisionModelManagerLocal::WriteCollisionModel( idFile* fp, cm_model_t* model )
{
	int i, polygonMemory, brushMemory;
	
	fp->WriteFloatString( "collisionModel \"%s\" {\n", model->name.c_str() );
	// vertices
	fp->WriteFloatString( "\tvertices { /* numVertices = */ %d\n", model->numVertices );
	for( i = 0; i < model->numVertices; i++ )
	{
		fp->WriteFloatString( "\t/* %d */ ( %f %f %f )\n", i, model->vertices[i].p[0], model->vertices[i].p[1], model->vertices[i].p[2] );
	}
	fp->WriteFloatString( "\t}\n" );
	// edges
	fp->WriteFloatString( "\tedges { /* numEdges = */ %d\n", model->numEdges );
	for( i = 0; i < model->numEdges; i++ )
	{
		fp->WriteFloatString( "\t/* %d */ ( %d %d ) %d %d\n", i, model->edges[i].vertexNum[0], model->edges[i].vertexNum[1], model->edges[i].internal, model->edges[i].numUsers );
	}
	fp->WriteFloatString( "\t}\n" );
	// nodes
	fp->WriteFloatString( "\tnodes {\n" );
	WriteNodes( fp, model->node );
	fp->WriteFloatString( "\t}\n" );
	// polygons
	checkCount++;
	polygonMemory = CountPolygonMemory( model->node );
	fp->WriteFloatString( "\tpolygons /* polygonMemory = */ %d {\n", polygonMemory );
	checkCount++;
	WritePolygons( fp, model->node );
	fp->WriteFloatString( "\t}\n" );
	// brushes
	checkCount++;
	brushMemory = CountBrushMemory( model->node );
	fp->WriteFloatString( "\tbrushes /* brushMemory = */ %d {\n", brushMemory );
	checkCount++;
	WriteBrushes( fp, model->node );
	fp->WriteFloatString( "\t}\n" );
	// closing brace
	fp->WriteFloatString( "}\n" );
}

/*
================
idCollisionModelManagerLocal::WriteCollisionModelsToFile
================
*/
void idCollisionModelManagerLocal::WriteCollisionModelsToFile( const char* filename, int firstModel, int lastModel, unsigned int mapFileCRC )
{
	int i;
	idFile* fp;
	idStr name;
	
	name = filename;
	name.SetFileExtension( CM_FILE_EXT );
	
	common->Printf( "writing %s\n", name.c_str() );
	fp = fileSystem->OpenFileWrite( name, "fs_basepath" );
	if( !fp )
	{
		common->Warning( "idCollisionModelManagerLocal::WriteCollisionModelsToFile: Error opening file %s\n", name.c_str() );
		return;
	}
	
	// write file id and version
	fp->WriteFloatString( "%s \"%s\"\n\n", CM_FILEID, CM_FILEVERSION );
	// write the map file crc
	fp->WriteFloatString( "%u\n\n", mapFileCRC );
	
	// write the collision models
	for( i = firstModel; i < lastModel; i++ )
	{
		WriteCollisionModel( fp, models[ i ] );
	}
	
	fileSystem->CloseFile( fp );
}

/*
================
idCollisionModelManagerLocal::WriteCollisionModelForMapEntity
================
*/
bool idCollisionModelManagerLocal::WriteCollisionModelForMapEntity( const idMapEntity* mapEnt, const char* filename, const bool testTraceModel )
{
	idFile* fp;
	idStr name;
	cm_model_t* model;
	
	SetupHash();
	model = CollisionModelForMapEntity( mapEnt );
	model->name = filename;
	
	name = filename;
	name.SetFileExtension( CM_FILE_EXT );
	
	common->Printf( "writing %s\n", name.c_str() );
	fp = fileSystem->OpenFileWrite( name, "fs_basepath" );
	if( !fp )
	{
		common->Printf( "idCollisionModelManagerLocal::WriteCollisionModelForMapEntity: Error opening file %s\n", name.c_str() );
		FreeModel( model );
		return false;
	}
	
	// write file id and version
	fp->WriteFloatString( "%s \"%s\"\n\n", CM_FILEID, CM_FILEVERSION );
	// write the map file crc
	fp->WriteFloatString( "%u\n\n", 0 );
	
	// write the collision model
	WriteCollisionModel( fp, model );
	
	fileSystem->CloseFile( fp );
	
	if( testTraceModel )
	{
		idTraceModel trm;
		TrmFromModel( model, trm );
	}
	
	FreeModel( model );
	
	return true;
}


/*
===============================================================================

Loading of collision model file

===============================================================================
*/

/*
================
idCollisionModelManagerLocal::ParseVertices
================
*/
void idCollisionModelManagerLocal::ParseVertices( idLexer* src, cm_model_t* model )
{
	int i;
	
	src->ExpectTokenString( "{" );
	model->numVertices = src->ParseInt();
	model->maxVertices = model->numVertices;
	model->vertices = ( cm_vertex_t* ) Mem_ClearedAlloc( model->maxVertices * sizeof( cm_vertex_t ), TAG_COLLISION );
	for( i = 0; i < model->numVertices; i++ )
	{
		src->Parse1DMatrix( 3, model->vertices[i].p.ToFloatPtr() );
		model->vertices[i].side = 0;
		model->vertices[i].sideSet = 0;
		model->vertices[i].checkcount = 0;
	}
	src->ExpectTokenString( "}" );
}

/*
================
idCollisionModelManagerLocal::ParseEdges
================
*/
void idCollisionModelManagerLocal::ParseEdges( idLexer* src, cm_model_t* model )
{
	int i;
	
	src->ExpectTokenString( "{" );
	model->numEdges = src->ParseInt();
	model->maxEdges = model->numEdges;
	model->edges = ( cm_edge_t* ) Mem_ClearedAlloc( model->maxEdges * sizeof( cm_edge_t ), TAG_COLLISION );
	for( i = 0; i < model->numEdges; i++ )
	{
		src->ExpectTokenString( "(" );
		model->edges[i].vertexNum[0] = src->ParseInt();
		model->edges[i].vertexNum[1] = src->ParseInt();
		src->ExpectTokenString( ")" );
		model->edges[i].side = 0;
		model->edges[i].sideSet = 0;
		model->edges[i].internal = src->ParseInt();
		model->edges[i].numUsers = src->ParseInt();
		model->edges[i].normal = vec3_origin;
		model->edges[i].checkcount = 0;
		model->numInternalEdges += model->edges[i].internal;
	}
	src->ExpectTokenString( "}" );
}

/*
================
idCollisionModelManagerLocal::ParseNodes
================
*/
cm_node_t* idCollisionModelManagerLocal::ParseNodes( idLexer* src, cm_model_t* model, cm_node_t* parent )
{
	cm_node_t* node;
	
	model->numNodes++;
	node = AllocNode( model, model->numNodes < NODE_BLOCK_SIZE_SMALL ? NODE_BLOCK_SIZE_SMALL : NODE_BLOCK_SIZE_LARGE );
	node->brushes = NULL;
	node->polygons = NULL;
	node->parent = parent;
	src->ExpectTokenString( "(" );
	node->planeType = src->ParseInt();
	node->planeDist = src->ParseFloat();
	src->ExpectTokenString( ")" );
	if( node->planeType != -1 )
	{
		node->children[0] = ParseNodes( src, model, node );
		node->children[1] = ParseNodes( src, model, node );
	}
	return node;
}

/*
================
idCollisionModelManagerLocal::ParsePolygons
================
*/
void idCollisionModelManagerLocal::ParsePolygons( idLexer* src, cm_model_t* model )
{
	cm_polygon_t* p;
	int i, numEdges;
	idVec3 normal;
	idToken token;
	
	if( src->CheckTokenType( TT_NUMBER, 0, &token ) )
	{
		model->polygonBlock = ( cm_polygonBlock_t* ) Mem_ClearedAlloc( sizeof( cm_polygonBlock_t ) + token.GetIntValue(), TAG_COLLISION );
		model->polygonBlock->bytesRemaining = token.GetIntValue();
		model->polygonBlock->next = ( ( byte* ) model->polygonBlock ) + sizeof( cm_polygonBlock_t );
	}
	
	src->ExpectTokenString( "{" );
	while( !src->CheckTokenString( "}" ) )
	{
		// parse polygon
		numEdges = src->ParseInt();
		p = AllocPolygon( model, numEdges );
		p->numEdges = numEdges;
		src->ExpectTokenString( "(" );
		for( i = 0; i < p->numEdges; i++ )
		{
			p->edges[i] = src->ParseInt();
		}
		src->ExpectTokenString( ")" );
		src->Parse1DMatrix( 3, normal.ToFloatPtr() );
		p->plane.SetNormal( normal );
		p->plane.SetDist( src->ParseFloat() );
		src->Parse1DMatrix( 3, p->bounds[0].ToFloatPtr() );
		src->Parse1DMatrix( 3, p->bounds[1].ToFloatPtr() );
		src->ExpectTokenType( TT_STRING, 0, &token );
		// get material
		p->material = declManager->FindMaterial( token );
		p->contents = p->material->GetContentFlags();
		p->checkcount = 0;
		// filter polygon into tree
		R_FilterPolygonIntoTree( model, model->node, NULL, p );
	}
}

/*
================
idCollisionModelManagerLocal::ParseBrushes
================
*/
void idCollisionModelManagerLocal::ParseBrushes( idLexer* src, cm_model_t* model )
{
	cm_brush_t* b;
	int i, numPlanes;
	idVec3 normal;
	idToken token;
	
	if( src->CheckTokenType( TT_NUMBER, 0, &token ) )
	{
		model->brushBlock = ( cm_brushBlock_t* ) Mem_ClearedAlloc( sizeof( cm_brushBlock_t ) + token.GetIntValue(), TAG_COLLISION );
		model->brushBlock->bytesRemaining = token.GetIntValue();
		model->brushBlock->next = ( ( byte* ) model->brushBlock ) + sizeof( cm_brushBlock_t );
	}
	
	src->ExpectTokenString( "{" );
	while( !src->CheckTokenString( "}" ) )
	{
		// parse brush
		numPlanes = src->ParseInt();
		b = AllocBrush( model, numPlanes );
		b->numPlanes = numPlanes;
		src->ExpectTokenString( "{" );
		for( i = 0; i < b->numPlanes; i++ )
		{
			src->Parse1DMatrix( 3, normal.ToFloatPtr() );
			b->planes[i].SetNormal( normal );
			b->planes[i].SetDist( src->ParseFloat() );
		}
		src->ExpectTokenString( "}" );
		src->Parse1DMatrix( 3, b->bounds[0].ToFloatPtr() );
		src->Parse1DMatrix( 3, b->bounds[1].ToFloatPtr() );
		src->ReadToken( &token );
		if( token.type == TT_NUMBER )
		{
			b->contents = token.GetIntValue();		// old .cm files use a single integer
		}
		else
		{
			b->contents = ContentsFromString( token );
		}
		b->checkcount = 0;
		b->primitiveNum = 0;
		b->material = NULL;
		// filter brush into tree
		R_FilterBrushIntoTree( model, model->node, NULL, b );
	}
}

/*
================
idCollisionModelManagerLocal::ParseCollisionModel
================
*/
cm_model_t* idCollisionModelManagerLocal::ParseCollisionModel( idLexer* src )
{

	cm_model_t* model;
	idToken token;
	
	if( numModels >= MAX_SUBMODELS )
	{
		common->Error( "LoadModel: no free slots" );
		return NULL;
	}
	model = AllocModel();
	models[numModels ] = model;
	numModels++;
	// parse the file
	src->ExpectTokenType( TT_STRING, 0, &token );
	model->name = token;
	src->ExpectTokenString( "{" );
	while( !src->CheckTokenString( "}" ) )
	{
	
		src->ReadToken( &token );
		
		if( token == "vertices" )
		{
			ParseVertices( src, model );
			continue;
		}
		
		if( token == "edges" )
		{
			ParseEdges( src, model );
			continue;
		}
		
		if( token == "nodes" )
		{
			src->ExpectTokenString( "{" );
			model->node = ParseNodes( src, model, NULL );
			src->ExpectTokenString( "}" );
			continue;
		}
		
		if( token == "polygons" )
		{
			ParsePolygons( src, model );
			continue;
		}
		
		if( token == "brushes" )
		{
			ParseBrushes( src, model );
			continue;
		}
		
		src->Error( "ParseCollisionModel: bad token \"%s\"", token.c_str() );
	}
	// calculate edge normals
	checkCount++;
	CalculateEdgeNormals( model, model->node );
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
						
	return model;
}

/*
================
idCollisionModelManagerLocal::LoadCollisionModelFile
================
*/
bool idCollisionModelManagerLocal::LoadCollisionModelFile( const char* name, unsigned int mapFileCRC )
{
	idToken token;
	idLexer* src;
	unsigned int crc;
	
	// load it
	idStrStatic< MAX_OSPATH > fileName = name;
	
	// check for generated file
	idStrStatic< MAX_OSPATH > generatedFileName = fileName;
	generatedFileName.Insert( "generated/", 0 );
	generatedFileName.SetFileExtension( CM_BINARYFILE_EXT );
	
	// if we are reloading the same map, check the timestamp
	// and try to skip all the work
	ID_TIME_T currentTimeStamp = fileSystem->GetTimestamp( fileName );
	
	// see if we have a generated version of this
	bool loaded = false;
	idFileLocal file( fileSystem->OpenFileReadMemory( generatedFileName ) );
	if( file != NULL )
	{
		int numEntries = 0;
		file->ReadBig( numEntries );
		file->ReadString( mapName );
		file->ReadBig( crc );
		idStrStatic< 32 > fileID;
		idStrStatic< 32 > fileVersion;
		file->ReadString( fileID );
		file->ReadString( fileVersion );
		if( fileID == CM_FILEID && fileVersion == CM_FILEVERSION && crc == mapFileCRC && numEntries > 0 )
		{
			loaded = true; // DG: moved this up here to prevent segfaults, see below
			for( int i = 0; i < numEntries; i++ )
			{
				cm_model_t* model = LoadBinaryModelFromFile( file, currentTimeStamp );
				// DG: handle the case that loading the binary model fails gracefully
				//     (otherwise we'll get a segfault when someone wants to use models[numModels])
				if( model == NULL )
				{
					loaded = false;
					break;
				}
				// DG end
				models[ numModels ] = model;
				numModels++;
			}
		}
	}
	
	if( !loaded )
	{
	
		fileName.SetFileExtension( CM_FILE_EXT );
		src = new( TAG_COLLISION ) idLexer( fileName );
		src->SetFlags( LEXFL_NOSTRINGCONCAT | LEXFL_NODOLLARPRECOMPILE );
		if( !src->IsLoaded() )
		{
			delete src;
			return false;
		}
		
		int numEntries = 0;
		idFileLocal outputFile( fileSystem->OpenFileWrite( generatedFileName, "fs_basepath" ) );
		if( outputFile != NULL )
		{
			outputFile->WriteBig( numEntries );
			outputFile->WriteString( mapName );
			outputFile->WriteBig( mapFileCRC );
			outputFile->WriteString( CM_FILEID );
			outputFile->WriteString( CM_FILEVERSION );
		}
		
		if( !src->ExpectTokenString( CM_FILEID ) )
		{
			common->Warning( "%s is not an CM file.", fileName.c_str() );
			delete src;
			return false;
		}
		
		if( !src->ReadToken( &token ) || token != CM_FILEVERSION )
		{
			common->Warning( "%s has version %s instead of %s", fileName.c_str(), token.c_str(), CM_FILEVERSION );
			delete src;
			return false;
		}
		
		if( !src->ExpectTokenType( TT_NUMBER, TT_INTEGER, &token ) )
		{
			common->Warning( "%s has no map file CRC", fileName.c_str() );
			delete src;
			return false;
		}
		
		crc = token.GetUnsignedLongValue();
		if( mapFileCRC && crc != mapFileCRC )
		{
			common->Printf( "%s is out of date\n", fileName.c_str() );
			delete src;
			return false;
		}
		
		// parse the file
		while( 1 )
		{
			if( !src->ReadToken( &token ) )
			{
				break;
			}
			
			if( token == "collisionModel" )
			{
				cm_model_t* model = ParseCollisionModel( src );
				if( model == NULL )
				{
					delete src;
					return false;
				}
				if( outputFile != NULL )
				{
					WriteBinaryModelToFile( model, outputFile, currentTimeStamp );
					numEntries++;
				}
				continue;
			}
			
			src->Error( "idCollisionModelManagerLocal::LoadCollisionModelFile: bad token \"%s\"", token.c_str() );
		}
		delete src;
		if( outputFile != NULL )
		{
			outputFile->Seek( 0, FS_SEEK_SET );
			outputFile->WriteBig( numEntries );
		}
	}
	
	return true;
}
