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

#include "Model_ma.h"

/*
======================================================================

	Parses Maya ASCII files.

======================================================================
*/


#define MA_VERBOSE( x ) { if ( maGlobal.verbose ) { common->Printf x ; } }

// working variables used during parsing
typedef struct
{
	bool			verbose;
	maModel_t*		model;
	maObject_t*		currentObject;
} ma_t;

static ma_t maGlobal;


void MA_ParseNodeHeader( idParser& parser, maNodeHeader_t* header )
{

	memset( header, 0, sizeof( maNodeHeader_t ) );

	idToken token;
	while( parser.ReadToken( &token ) )
	{
		if( !token.Icmp( "-" ) )
		{
			parser.ReadToken( &token );
			if( !token.Icmp( "n" ) )
			{
				parser.ReadToken( &token );
				strcpy( header->name, token.c_str() );
			}
			else if( !token.Icmp( "p" ) )
			{
				parser.ReadToken( &token );
				strcpy( header->parent, token.c_str() );
			}
		}
		else if( !token.Icmp( ";" ) )
		{
			break;
		}
	}
}

bool MA_ParseHeaderIndex( maAttribHeader_t* header, int& minIndex, int& maxIndex, const char* headerType, const char* skipString )
{

	idParser miniParse;
	idToken token;

	miniParse.LoadMemory( header->name, strlen( header->name ), headerType );
	if( skipString )
	{
		miniParse.SkipUntilString( skipString );
	}

	if( !miniParse.SkipUntilString( "[" ) )
	{
		//This was just a header
		return false;
	}
	minIndex = miniParse.ParseInt();
	miniParse.ReadToken( &token );
	if( !token.Icmp( "]" ) )
	{
		maxIndex = minIndex;
	}
	else
	{
		maxIndex = miniParse.ParseInt();
	}
	return true;
}

bool MA_ParseAttribHeader( idParser& parser, maAttribHeader_t* header )
{

	idToken token;

	memset( header, 0, sizeof( maAttribHeader_t ) );

	parser.ReadToken( &token );
	if( !token.Icmp( "-" ) )
	{
		parser.ReadToken( &token );
		if( !token.Icmp( "s" ) )
		{
			header->size = parser.ParseInt();
			parser.ReadToken( &token );
		}
	}
	strcpy( header->name, token.c_str() );
	return true;
}

bool MA_ReadVec3( idParser& parser, idVec3& vec )
{
	idToken token;
	if( !parser.SkipUntilString( "double3" ) )
	{
		throw idException( va( "Maya Loader '%s': Invalid Vec3", parser.GetFileName() ) );
	}


	//We need to flip y and z because of the maya coordinate system
	vec.x = parser.ParseFloat();
	vec.z = parser.ParseFloat();
	vec.y = parser.ParseFloat();

	return true;
}

bool IsNodeComplete( idToken& token )
{
	if( !token.Icmp( "createNode" ) || !token.Icmp( "connectAttr" ) || !token.Icmp( "select" ) )
	{
		return true;
	}
	return false;
}

bool MA_ParseTransform( idParser& parser )
{

	maNodeHeader_t	header;
	maTransform_t*	transform;
	memset( &header, 0, sizeof( header ) );

	//Allocate room for the transform
	transform = ( maTransform_t* )Mem_Alloc( sizeof( maTransform_t ), TAG_MODEL );
	memset( transform, 0, sizeof( maTransform_t ) );
	transform->scale.x = transform->scale.y = transform->scale.z = 1;

	//Get the header info from the transform
	MA_ParseNodeHeader( parser, &header );

	//Read the transform attributes
	idToken token;
	while( parser.ReadToken( &token ) )
	{
		if( IsNodeComplete( token ) )
		{
			parser.UnreadToken( &token );
			break;
		}
		if( !token.Icmp( "setAttr" ) )
		{
			parser.ReadToken( &token );
			if( !token.Icmp( ".t" ) )
			{
				if( !MA_ReadVec3( parser, transform->translate ) )
				{
					return false;
				}
				transform->translate.y *=  -1;
			}
			else if( !token.Icmp( ".r" ) )
			{
				if( !MA_ReadVec3( parser, transform->rotate ) )
				{
					return false;
				}
			}
			else if( !token.Icmp( ".s" ) )
			{
				if( !MA_ReadVec3( parser, transform->scale ) )
				{
					return false;
				}
			}
			else
			{
				parser.SkipRestOfLine();
			}
		}
	}

	if( header.parent[0] != 0 )
	{
		//Find the parent
		maTransform_t**	parent;
		maGlobal.model->transforms.Get( header.parent, &parent );
		if( parent )
		{
			transform->parent = *parent;
		}
	}

	//Add this transform to the list
	maGlobal.model->transforms.Set( header.name, transform );
	return true;
}

bool MA_ParseVertex( idParser& parser, maAttribHeader_t* header )
{

	maMesh_t* pMesh = &maGlobal.currentObject->mesh;
	idToken token;

	//Allocate enough space for all the verts if this is the first attribute for verticies
	if( !pMesh->vertexes )
	{
		pMesh->numVertexes = header->size;
		pMesh->vertexes = ( idVec3* )Mem_Alloc( sizeof( idVec3 ) * pMesh->numVertexes, TAG_MODEL );
	}

	//Get the start and end index for this attribute
	int minIndex, maxIndex;
	if( !MA_ParseHeaderIndex( header, minIndex, maxIndex, "VertexHeader", NULL ) )
	{
		//This was just a header
		return true;
	}

	//Read each vert
	for( int i = minIndex; i <= maxIndex; i++ )
	{
		pMesh->vertexes[i].x = parser.ParseFloat();
		pMesh->vertexes[i].z = parser.ParseFloat();
		pMesh->vertexes[i].y = -parser.ParseFloat();
	}

	return true;
}

bool MA_ParseVertexTransforms( idParser& parser, maAttribHeader_t* header )
{

	maMesh_t* pMesh = &maGlobal.currentObject->mesh;
	idToken token;

	//Allocate enough space for all the verts if this is the first attribute for verticies
	if( !pMesh->vertTransforms )
	{
		if( header->size == 0 )
		{
			header->size = 1;
		}

		pMesh->numVertTransforms = header->size;
		pMesh->vertTransforms = ( idVec4* )Mem_Alloc( sizeof( idVec4 ) * pMesh->numVertTransforms, TAG_MODEL );
		pMesh->nextVertTransformIndex = 0;
	}

	//Get the start and end index for this attribute
	int minIndex, maxIndex;
	if( !MA_ParseHeaderIndex( header, minIndex, maxIndex, "VertexTransformHeader", NULL ) )
	{
		//This was just a header
		return true;
	}

	parser.ReadToken( &token );
	if( !token.Icmp( "-" ) )
	{
		idToken tk2;
		parser.ReadToken( &tk2 );
		if( !tk2.Icmp( "type" ) )
		{
			parser.SkipUntilString( "float3" );
		}
		else
		{
			parser.UnreadToken( &tk2 );
			parser.UnreadToken( &token );
		}
	}
	else
	{
		parser.UnreadToken( &token );
	}

	//Read each vert
	for( int i = minIndex; i <= maxIndex; i++ )
	{
		pMesh->vertTransforms[pMesh->nextVertTransformIndex].x = parser.ParseFloat();
		pMesh->vertTransforms[pMesh->nextVertTransformIndex].z = parser.ParseFloat();
		pMesh->vertTransforms[pMesh->nextVertTransformIndex].y = -parser.ParseFloat();

		//w hold the vert index
		pMesh->vertTransforms[pMesh->nextVertTransformIndex].w = i;

		pMesh->nextVertTransformIndex++;
	}

	return true;
}

bool MA_ParseEdge( idParser& parser, maAttribHeader_t* header )
{

	maMesh_t* pMesh = &maGlobal.currentObject->mesh;
	idToken token;

	//Allocate enough space for all the verts if this is the first attribute for verticies
	if( !pMesh->edges )
	{
		pMesh->numEdges = header->size;
		pMesh->edges = ( idVec3* )Mem_Alloc( sizeof( idVec3 ) * pMesh->numEdges, TAG_MODEL );
	}

	//Get the start and end index for this attribute
	int minIndex, maxIndex;
	if( !MA_ParseHeaderIndex( header, minIndex, maxIndex, "EdgeHeader", NULL ) )
	{
		//This was just a header
		return true;
	}

	//Read each vert
	for( int i = minIndex; i <= maxIndex; i++ )
	{
		pMesh->edges[i].x = parser.ParseFloat();
		pMesh->edges[i].y = parser.ParseFloat();
		pMesh->edges[i].z = parser.ParseFloat();
	}

	return true;
}

bool MA_ParseNormal( idParser& parser, maAttribHeader_t* header )
{

	maMesh_t* pMesh = &maGlobal.currentObject->mesh;
	idToken token;

	//Allocate enough space for all the verts if this is the first attribute for verticies
	if( !pMesh->normals )
	{
		pMesh->numNormals = header->size;
		pMesh->normals = ( idVec3* )Mem_Alloc( sizeof( idVec3 ) * pMesh->numNormals, TAG_MODEL );
	}

	//Get the start and end index for this attribute
	int minIndex, maxIndex;
	if( !MA_ParseHeaderIndex( header, minIndex, maxIndex, "NormalHeader", NULL ) )
	{
		//This was just a header
		return true;
	}


	parser.ReadToken( &token );
	if( !token.Icmp( "-" ) )
	{
		idToken tk2;
		parser.ReadToken( &tk2 );
		if( !tk2.Icmp( "type" ) )
		{
			parser.SkipUntilString( "float3" );
		}
		else
		{
			parser.UnreadToken( &tk2 );
			parser.UnreadToken( &token );
		}
	}
	else
	{
		parser.UnreadToken( &token );
	}


	//Read each vert
	for( int i = minIndex; i <= maxIndex; i++ )
	{
		pMesh->normals[i].x = parser.ParseFloat();

		//Adjust the normals for the change in coordinate systems
		pMesh->normals[i].z = parser.ParseFloat();
		pMesh->normals[i].y = -parser.ParseFloat();

		pMesh->normals[i].Normalize();

	}

	pMesh->normalsParsed = true;
	pMesh->nextNormal = 0;

	return true;
}



bool MA_ParseFace( idParser& parser, maAttribHeader_t* header )
{

	maMesh_t* pMesh = &maGlobal.currentObject->mesh;
	idToken token;

	//Allocate enough space for all the verts if this is the first attribute for verticies
	if( !pMesh->faces )
	{
		pMesh->numFaces = header->size;
		pMesh->faces = ( maFace_t* )Mem_Alloc( sizeof( maFace_t ) * pMesh->numFaces, TAG_MODEL );
	}

	//Get the start and end index for this attribute
	int minIndex, maxIndex;
	if( !MA_ParseHeaderIndex( header, minIndex, maxIndex, "FaceHeader", NULL ) )
	{
		//This was just a header
		return true;
	}

	//Read the face data
	int currentFace = minIndex - 1;
	while( parser.ReadToken( &token ) )
	{
		if( IsNodeComplete( token ) )
		{
			parser.UnreadToken( &token );
			break;
		}

		if( !token.Icmp( "f" ) )
		{
			int count = parser.ParseInt();
			if( count != 3 )
			{
				throw idException( va( "Maya Loader '%s': Face is not a triangle.", parser.GetFileName() ) );
			}
			//Increment the face number because a new face always starts with an "f" token
			currentFace++;

			//We cannot reorder edges until later because the normal processing
			//assumes the edges are in the original order
			pMesh->faces[currentFace].edge[0] = parser.ParseInt();
			pMesh->faces[currentFace].edge[1] = parser.ParseInt();
			pMesh->faces[currentFace].edge[2] = parser.ParseInt();

			//Some more init stuff
			pMesh->faces[currentFace].vertexColors[0] = pMesh->faces[currentFace].vertexColors[1] = pMesh->faces[currentFace].vertexColors[2] = -1;

		}
		else if( !token.Icmp( "mu" ) )
		{
			int uvstIndex = parser.ParseInt();
			uvstIndex;
			int count = parser.ParseInt();
			if( count != 3 )
			{
				throw idException( va( "Maya Loader '%s': Invalid texture coordinates.", parser.GetFileName() ) );
			}
			pMesh->faces[currentFace].tVertexNum[0] = parser.ParseInt();
			pMesh->faces[currentFace].tVertexNum[1] = parser.ParseInt();
			pMesh->faces[currentFace].tVertexNum[2] = parser.ParseInt();

		}
		else if( !token.Icmp( "mf" ) )
		{
			int count = parser.ParseInt();
			if( count != 3 )
			{
				throw idException( va( "Maya Loader '%s': Invalid texture coordinates.", parser.GetFileName() ) );
			}
			pMesh->faces[currentFace].tVertexNum[0] = parser.ParseInt();
			pMesh->faces[currentFace].tVertexNum[1] = parser.ParseInt();
			pMesh->faces[currentFace].tVertexNum[2] = parser.ParseInt();

		}
		else if( !token.Icmp( "fc" ) )
		{

			int count = parser.ParseInt();
			if( count != 3 )
			{
				throw idException( va( "Maya Loader '%s': Invalid vertex color.", parser.GetFileName() ) );
			}
			pMesh->faces[currentFace].vertexColors[0] = parser.ParseInt();
			pMesh->faces[currentFace].vertexColors[1] = parser.ParseInt();
			pMesh->faces[currentFace].vertexColors[2] = parser.ParseInt();

		}
	}

	return true;
}

bool MA_ParseColor( idParser& parser, maAttribHeader_t* header )
{

	maMesh_t* pMesh = &maGlobal.currentObject->mesh;
	idToken token;

	//Allocate enough space for all the verts if this is the first attribute for verticies
	if( !pMesh->colors )
	{
		pMesh->numColors = header->size;
		pMesh->colors = ( byte* )Mem_Alloc( sizeof( byte ) * pMesh->numColors * 4, TAG_MODEL );
	}

	//Get the start and end index for this attribute
	int minIndex, maxIndex;
	if( !MA_ParseHeaderIndex( header, minIndex, maxIndex, "ColorHeader", NULL ) )
	{
		//This was just a header
		return true;
	}

	//Read each vert
	for( int i = minIndex; i <= maxIndex; i++ )
	{
		pMesh->colors[i * 4] = parser.ParseFloat() * 255;
		pMesh->colors[i * 4 + 1] = parser.ParseFloat() * 255;
		pMesh->colors[i * 4 + 2] = parser.ParseFloat() * 255;
		pMesh->colors[i * 4 + 3] = parser.ParseFloat() * 255;
	}

	return true;
}

bool MA_ParseTVert( idParser& parser, maAttribHeader_t* header )
{

	maMesh_t* pMesh = &maGlobal.currentObject->mesh;
	idToken token;

	//This is not the texture coordinates. It is just the name so ignore it
	if( strstr( header->name, "uvsn" ) )
	{
		return true;
	}

	//Allocate enough space for all the data
	if( !pMesh->tvertexes )
	{
		pMesh->numTVertexes = header->size;
		pMesh->tvertexes = ( idVec2* )Mem_Alloc( sizeof( idVec2 ) * pMesh->numTVertexes, TAG_MODEL );
	}

	//Get the start and end index for this attribute
	int minIndex, maxIndex;
	if( !MA_ParseHeaderIndex( header, minIndex, maxIndex, "TextureCoordHeader", "uvsp" ) )
	{
		//This was just a header
		return true;
	}

	parser.ReadToken( &token );
	if( !token.Icmp( "-" ) )
	{
		idToken tk2;
		parser.ReadToken( &tk2 );
		if( !tk2.Icmp( "type" ) )
		{
			parser.SkipUntilString( "float2" );
		}
		else
		{
			parser.UnreadToken( &tk2 );
			parser.UnreadToken( &token );
		}
	}
	else
	{
		parser.UnreadToken( &token );
	}

	//Read each tvert
	for( int i = minIndex; i <= maxIndex; i++ )
	{
		pMesh->tvertexes[i].x = parser.ParseFloat();
		pMesh->tvertexes[i].y = 1.0f - parser.ParseFloat();
	}

	return true;
}



/*
*	Quick check to see if the vert participates in a shared normal
*/
bool MA_QuickIsVertShared( int faceIndex, int vertIndex )
{

	maMesh_t* pMesh = &maGlobal.currentObject->mesh;
	int vertNum = pMesh->faces[faceIndex].vertexNum[vertIndex];

	for( int i = 0; i < 3; i++ )
	{
		int edge = pMesh->faces[faceIndex].edge[i];
		if( edge < 0 )
		{
			edge = idMath::Fabs( edge ) - 1;
		}
		if( pMesh->edges[edge].z == 1 && ( pMesh->edges[edge].x == vertNum || pMesh->edges[edge].y == vertNum ) )
		{
			return true;
		}
	}
	return false;
}

void MA_GetSharedFace( int faceIndex, int vertIndex, int& sharedFace, int& sharedVert )
{

	maMesh_t* pMesh = &maGlobal.currentObject->mesh;
	int vertNum = pMesh->faces[faceIndex].vertexNum[vertIndex];

	sharedFace = -1;
	sharedVert = -1;

	//Find a shared edge on this face that contains the specified vert
	for( int edgeIndex = 0; edgeIndex < 3; edgeIndex++ )
	{

		int edge = pMesh->faces[faceIndex].edge[edgeIndex];
		if( edge < 0 )
		{
			edge = idMath::Fabs( edge ) - 1;
		}

		if( pMesh->edges[edge].z == 1 && ( pMesh->edges[edge].x == vertNum || pMesh->edges[edge].y == vertNum ) )
		{

			for( int i = 0; i < faceIndex; i++ )
			{

				for( int j = 0; j < 3; j++ )
				{
					if( pMesh->faces[i].vertexNum[j] == vertNum )
					{
						sharedFace = i;
						sharedVert = j;
						break;
					}
				}
			}
		}
		if( sharedFace != -1 )
		{
			break;
		}

	}
}

void MA_ParseMesh( idParser& parser )
{

	maObject_t*	object;
	object = ( maObject_t* )Mem_Alloc( sizeof( maObject_t ), TAG_MODEL );
	memset( object, 0, sizeof( maObject_t ) );
	maGlobal.model->objects.Append( object );
	maGlobal.currentObject = object;
	object->materialRef = -1;


	//Get the header info from the mesh
	maNodeHeader_t	header;
	MA_ParseNodeHeader( parser, &header );

	//Find my parent
	if( header.parent[0] != 0 )
	{
		//Find the parent
		maTransform_t**	parent;
		maGlobal.model->transforms.Get( header.parent, &parent );
		if( parent )
		{
			maGlobal.currentObject->mesh.transform = *parent;
		}
	}

	strcpy( object->name, header.name );

	//Read the transform attributes
	idToken token;
	while( parser.ReadToken( &token ) )
	{
		if( IsNodeComplete( token ) )
		{
			parser.UnreadToken( &token );
			break;
		}
		if( !token.Icmp( "setAttr" ) )
		{
			maAttribHeader_t header;
			MA_ParseAttribHeader( parser, &header );

			if( strstr( header.name, ".vt" ) )
			{
				MA_ParseVertex( parser, &header );
			}
			else if( strstr( header.name, ".ed" ) )
			{
				MA_ParseEdge( parser, &header );
			}
			else if( strstr( header.name, ".pt" ) )
			{
				MA_ParseVertexTransforms( parser, &header );
			}
			else if( strstr( header.name, ".n" ) )
			{
				MA_ParseNormal( parser, &header );
			}
			else if( strstr( header.name, ".fc" ) )
			{
				MA_ParseFace( parser, &header );
			}
			else if( strstr( header.name, ".clr" ) )
			{
				MA_ParseColor( parser, &header );
			}
			else if( strstr( header.name, ".uvst" ) )
			{
				MA_ParseTVert( parser, &header );
			}
			else
			{
				parser.SkipRestOfLine();
			}
		}
	}


	maMesh_t* pMesh = &maGlobal.currentObject->mesh;

	//Get the verts from the edge
	for( int i = 0; i < pMesh->numFaces; i++ )
	{
		for( int j = 0; j < 3; j++ )
		{
			int edge = pMesh->faces[i].edge[j];
			if( edge < 0 )
			{
				edge = idMath::Fabs( edge ) - 1;
				pMesh->faces[i].vertexNum[j] = pMesh->edges[edge].y;
			}
			else
			{
				pMesh->faces[i].vertexNum[j] = pMesh->edges[edge].x;
			}
		}
	}

	//Get the normals
	if( pMesh->normalsParsed )
	{
		for( int i = 0; i < pMesh->numFaces; i++ )
		{
			for( int j = 0; j < 3; j++ )
			{

				//Is this vertex shared
				int sharedFace = -1;
				int sharedVert = -1;

				if( MA_QuickIsVertShared( i, j ) )
				{
					MA_GetSharedFace( i, j, sharedFace, sharedVert );
				}

				if( sharedFace != -1 )
				{
					//Get the normal from the share
					pMesh->faces[i].vertexNormals[j] = pMesh->faces[sharedFace].vertexNormals[sharedVert];

				}
				else
				{
					//The vertex is not shared so get the next normal
					if( pMesh->nextNormal >= pMesh->numNormals )
					{
						//We are using more normals than exist
						throw idException( va( "Maya Loader '%s': Invalid Normals Index.", parser.GetFileName() ) );
					}
					pMesh->faces[i].vertexNormals[j] = pMesh->normals[pMesh->nextNormal];
					pMesh->nextNormal++;
				}
			}
		}
	}

	//Now that the normals are good...lets reorder the verts to make the tris face the right way
	for( int i = 0; i < pMesh->numFaces; i++ )
	{
		int tmp = pMesh->faces[i].vertexNum[1];
		pMesh->faces[i].vertexNum[1] = pMesh->faces[i].vertexNum[2];
		pMesh->faces[i].vertexNum[2] = tmp;

		idVec3 tmpVec = pMesh->faces[i].vertexNormals[1];
		pMesh->faces[i].vertexNormals[1] = pMesh->faces[i].vertexNormals[2];
		pMesh->faces[i].vertexNormals[2] = tmpVec;

		tmp = pMesh->faces[i].tVertexNum[1];
		pMesh->faces[i].tVertexNum[1] = pMesh->faces[i].tVertexNum[2];
		pMesh->faces[i].tVertexNum[2] = tmp;

		tmp = pMesh->faces[i].vertexColors[1];
		pMesh->faces[i].vertexColors[1] = pMesh->faces[i].vertexColors[2];
		pMesh->faces[i].vertexColors[2] = tmp;
	}

	//Now apply the pt transformations
	for( int i = 0; i < pMesh->numVertTransforms; i++ )
	{
		pMesh->vertexes[( int )pMesh->vertTransforms[i].w] +=  pMesh->vertTransforms[i].ToVec3();
	}

	MA_VERBOSE( ( va( "MESH %s - parent %s\n", header.name, header.parent ) ) );
	MA_VERBOSE( ( va( "\tverts:%d\n", maGlobal.currentObject->mesh.numVertexes ) ) );
	MA_VERBOSE( ( va( "\tfaces:%d\n", maGlobal.currentObject->mesh.numFaces ) ) );
}

void MA_ParseFileNode( idParser& parser )
{

	//Get the header info from the node
	maNodeHeader_t	header;
	MA_ParseNodeHeader( parser, &header );

	//Read the transform attributes
	idToken token;
	while( parser.ReadToken( &token ) )
	{
		if( IsNodeComplete( token ) )
		{
			parser.UnreadToken( &token );
			break;
		}
		if( !token.Icmp( "setAttr" ) )
		{
			maAttribHeader_t attribHeader;
			MA_ParseAttribHeader( parser, &attribHeader );

			if( strstr( attribHeader.name, ".ftn" ) )
			{
				parser.SkipUntilString( "string" );
				parser.ReadToken( &token );
				if( !token.Icmp( "(" ) )
				{
					parser.ReadToken( &token );
				}

				maFileNode_t* fileNode;
				fileNode = ( maFileNode_t* )Mem_Alloc( sizeof( maFileNode_t ), TAG_MODEL );
				strcpy( fileNode->name, header.name );
				strcpy( fileNode->path, token.c_str() );

				maGlobal.model->fileNodes.Set( fileNode->name, fileNode );
			}
			else
			{
				parser.SkipRestOfLine();
			}
		}
	}
}

void MA_ParseMaterialNode( idParser& parser )
{

	//Get the header info from the node
	maNodeHeader_t	header;
	MA_ParseNodeHeader( parser, &header );

	maMaterialNode_t* matNode;
	matNode = ( maMaterialNode_t* )Mem_Alloc( sizeof( maMaterialNode_t ), TAG_MODEL );
	memset( matNode, 0, sizeof( maMaterialNode_t ) );

	strcpy( matNode->name, header.name );

	maGlobal.model->materialNodes.Set( matNode->name, matNode );
}

void MA_ParseCreateNode( idParser& parser )
{

	idToken token;
	parser.ReadToken( &token );

	if( !token.Icmp( "transform" ) )
	{
		MA_ParseTransform( parser );
	}
	else if( !token.Icmp( "mesh" ) )
	{
		MA_ParseMesh( parser );
	}
	else if( !token.Icmp( "file" ) )
	{
		MA_ParseFileNode( parser );
	}
	else if( !token.Icmp( "shadingEngine" ) || !token.Icmp( "lambert" ) || !token.Icmp( "phong" ) || !token.Icmp( "blinn" ) )
	{
		MA_ParseMaterialNode( parser );
	}
}


int MA_AddMaterial( const char* materialName )
{


	maMaterialNode_t**	destNode;
	maGlobal.model->materialNodes.Get( materialName, &destNode );
	if( destNode )
	{
		maMaterialNode_t* matNode = *destNode;

		//Iterate down the tree until we get a file
		while( matNode && !matNode->file )
		{
			matNode = matNode->child;
		}
		if( matNode && matNode->file )
		{

			//Got the file
			maMaterial_t*	material;
			material = ( maMaterial_t* )Mem_Alloc( sizeof( maMaterial_t ), TAG_MODEL );
			memset( material, 0, sizeof( maMaterial_t ) );

			//Remove the OS stuff
			idStr qPath;
			qPath = fileSystem->OSPathToRelativePath( matNode->file->path );

			strcpy( material->name, qPath.c_str() );

			maGlobal.model->materials.Append( material );
			return maGlobal.model->materials.Num() - 1;
		}
	}
	return -1;
}

bool MA_ParseConnectAttr( idParser& parser )
{

	idStr temp;
	idStr srcName;
	idStr srcType;
	idStr destName;
	idStr destType;

	idToken token;
	parser.ReadToken( &token );
	temp = token;
	int dot = temp.Find( "." );
	if( dot == -1 )
	{
		throw idException( va( "Maya Loader '%s': Invalid Connect Attribute.", parser.GetFileName() ) );
	}
	srcName = temp.Left( dot );
	srcType = temp.Right( temp.Length() - dot - 1 );

	parser.ReadToken( &token );
	temp = token;
	dot = temp.Find( "." );
	if( dot == -1 )
	{
		throw idException( va( "Maya Loader '%s': Invalid Connect Attribute.", parser.GetFileName() ) );
	}
	destName = temp.Left( dot );
	destType = temp.Right( temp.Length() - dot - 1 );

	if( srcType.Find( "oc" ) != -1 )
	{

		//Is this attribute a material node attribute
		maMaterialNode_t**	matNode;
		maGlobal.model->materialNodes.Get( srcName, &matNode );
		if( matNode )
		{
			maMaterialNode_t**	destNode;
			maGlobal.model->materialNodes.Get( destName, &destNode );
			if( destNode )
			{
				( *destNode )->child = *matNode;
			}
		}

		//Is this attribute a file node
		maFileNode_t** fileNode;
		maGlobal.model->fileNodes.Get( srcName, &fileNode );
		if( fileNode )
		{
			maMaterialNode_t**	destNode;
			maGlobal.model->materialNodes.Get( destName, &destNode );
			if( destNode )
			{
				( *destNode )->file = *fileNode;
			}
		}
	}

	if( srcType.Find( "iog" ) != -1 )
	{
		//Is this an attribute for one of our meshes
		for( int i = 0; i < maGlobal.model->objects.Num(); i++ )
		{
			if( !strcmp( maGlobal.model->objects[i]->name, srcName ) )
			{
				//maGlobal.model->objects[i]->materialRef = MA_AddMaterial(destName);
				strcpy( maGlobal.model->objects[i]->materialName, destName );
				break;
			}
		}
	}

	return true;
}


void MA_BuildScale( idMat4& mat, float x, float y, float z )
{
	mat.Identity();
	mat[0][0] = x;
	mat[1][1] = y;
	mat[2][2] = z;
}

void MA_BuildAxisRotation( idMat4& mat, float ang, int axis )
{

	float sinAng = idMath::Sin( ang );
	float cosAng = idMath::Cos( ang );

	mat.Identity();
	switch( axis )
	{
		case 0: //x
			mat[1][1] = cosAng;
			mat[1][2] = sinAng;
			mat[2][1] = -sinAng;
			mat[2][2] = cosAng;
			break;
		case 1:	//y
			mat[0][0] = cosAng;
			mat[0][2] = -sinAng;
			mat[2][0] = sinAng;
			mat[2][2] = cosAng;
			break;
		case 2://z
			mat[0][0] = cosAng;
			mat[0][1] = sinAng;
			mat[1][0] = -sinAng;
			mat[1][1] = cosAng;
			break;
	}
}

void MA_ApplyTransformation( maModel_t* model )
{

	for( int i = 0; i < model->objects.Num(); i++ )
	{
		maMesh_t* mesh = &model->objects[i]->mesh;
		maTransform_t* transform = mesh->transform;



		while( transform )
		{

			idMat4 rotx, roty, rotz;
			idMat4 scale;

			rotx.Identity();
			roty.Identity();
			rotz.Identity();

			if( fabs( transform->rotate.x ) > 0.0f )
			{
				MA_BuildAxisRotation( rotx, DEG2RAD( -transform->rotate.x ), 0 );
			}
			if( fabs( transform->rotate.y ) > 0.0f )
			{
				MA_BuildAxisRotation( roty, DEG2RAD( transform->rotate.y ), 1 );
			}
			if( fabs( transform->rotate.z ) > 0.0f )
			{
				MA_BuildAxisRotation( rotz, DEG2RAD( -transform->rotate.z ), 2 );
			}

			MA_BuildScale( scale, transform->scale.x, transform->scale.y, transform->scale.z );

			//Apply the transformation to each vert
			for( int j = 0; j < mesh->numVertexes; j++ )
			{
				mesh->vertexes[j] = scale * mesh->vertexes[j];

				mesh->vertexes[j] = rotx * mesh->vertexes[j];
				mesh->vertexes[j] = rotz * mesh->vertexes[j];
				mesh->vertexes[j] = roty * mesh->vertexes[j];

				mesh->vertexes[j] = mesh->vertexes[j] + transform->translate;
			}

			transform = transform->parent;
		}
	}
}

/*
=================
MA_Parse
=================
*/
maModel_t* MA_Parse( const char* buffer, const char* filename, bool verbose )
{
	memset( &maGlobal, 0, sizeof( maGlobal ) );

	maGlobal.verbose = verbose;




	maGlobal.currentObject = NULL;

	// NOTE: using new operator because aseModel_t contains idList class objects
	maGlobal.model = new( TAG_MODEL ) maModel_t;
	maGlobal.model->objects.Resize( 32, 32 );
	maGlobal.model->materials.Resize( 32, 32 );


	idParser parser;
	parser.SetFlags( LEXFL_NOSTRINGCONCAT );
	parser.LoadMemory( buffer, strlen( buffer ), filename );

	idToken token;
	while( parser.ReadToken( &token ) )
	{

		if( !token.Icmp( "createNode" ) )
		{
			MA_ParseCreateNode( parser );
		}
		else if( !token.Icmp( "connectAttr" ) )
		{
			MA_ParseConnectAttr( parser );
		}
	}

	//Resolve The Materials
	for( int i = 0; i < maGlobal.model->objects.Num(); i++ )
	{
		maGlobal.model->objects[i]->materialRef = MA_AddMaterial( maGlobal.model->objects[i]->materialName );
	}



	//Apply Transformation
	MA_ApplyTransformation( maGlobal.model );

	return maGlobal.model;
}

/*
=================
MA_Load
=================
*/
maModel_t* MA_Load( const char* fileName )
{
	char* buf;
	ID_TIME_T timeStamp;
	maModel_t* ma;

	fileSystem->ReadFile( fileName, ( void** )&buf, &timeStamp );
	if( !buf )
	{
		return NULL;
	}

	try
	{
		ma = MA_Parse( buf, fileName, false );
		ma->timeStamp = timeStamp;
	}
	catch( idException& e )
	{
		common->Warning( "%s", e.GetError() );
		if( maGlobal.model )
		{
			MA_Free( maGlobal.model );
		}
		ma = NULL;
	}

	fileSystem->FreeFile( buf );

	return ma;
}

/*
=================
MA_Free
=================
*/
void MA_Free( maModel_t* ma )
{
	int					i;
	maObject_t*			obj;
	maMesh_t*			mesh;
	maMaterial_t*		material;

	if( !ma )
	{
		return;
	}
	for( i = 0; i < ma->objects.Num(); i++ )
	{
		obj = ma->objects[i];

		// free the base nesh
		mesh = &obj->mesh;

		if( mesh->vertexes )
		{
			Mem_Free( mesh->vertexes );
		}
		if( mesh->vertTransforms )
		{
			Mem_Free( mesh->vertTransforms );
		}
		if( mesh->normals )
		{
			Mem_Free( mesh->normals );
		}
		if( mesh->tvertexes )
		{
			Mem_Free( mesh->tvertexes );
		}
		if( mesh->edges )
		{
			Mem_Free( mesh->edges );
		}
		if( mesh->colors )
		{
			Mem_Free( mesh->colors );
		}
		if( mesh->faces )
		{
			Mem_Free( mesh->faces );
		}
		Mem_Free( obj );
	}
	ma->objects.Clear();

	for( i = 0; i < ma->materials.Num(); i++ )
	{
		material = ma->materials[i];
		Mem_Free( material );
	}
	ma->materials.Clear();

	maTransform_t** trans;
	for( i = 0; i < ma->transforms.Num(); i++ )
	{
		trans = ma->transforms.GetIndex( i );
		Mem_Free( *trans );
	}
	ma->transforms.Clear();


	maFileNode_t** fileNode;
	for( i = 0; i < ma->fileNodes.Num(); i++ )
	{
		fileNode = ma->fileNodes.GetIndex( i );
		Mem_Free( *fileNode );
	}
	ma->fileNodes.Clear();

	maMaterialNode_t** matNode;
	for( i = 0; i < ma->materialNodes.Num(); i++ )
	{
		matNode = ma->materialNodes.GetIndex( i );
		Mem_Free( *matNode );
	}
	ma->materialNodes.Clear();
	delete ma;
}
