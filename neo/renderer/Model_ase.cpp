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


#include "Model_ase.h"

/*
======================================================================

	Parses 3D Studio Max ASCII export files.
	The goal is to parse the information into memory exactly as it is
	represented in the file.  Users of the data will then move it
	into a form that is more convenient for them.

======================================================================
*/


#define VERBOSE( x ) { if ( ase.verbose ) { common->Printf x ; } }

// working variables used during parsing
typedef struct
{
	const char*	buffer;
	const char*	curpos;
	int			len;
	char		token[1024];

	bool	verbose;

	aseModel_t*	model;
	aseObject_t*	currentObject;
	aseMesh_t*	currentMesh;
	aseMaterial_t*	currentMaterial;
	int			currentFace;
	int			currentVertex;
} ase_t;

static ase_t ase;


static aseMesh_t* ASE_GetCurrentMesh()
{
	return ase.currentMesh;
}

static int CharIsTokenDelimiter( int ch )
{
	if( ch <= 32 )
	{
		return 1;
	}
	return 0;
}

static int ASE_GetToken( bool restOfLine )
{
	int i = 0;

	if( ase.buffer == 0 )
	{
		return 0;
	}

	if( ( ase.curpos - ase.buffer ) == ase.len )
	{
		return 0;
	}

	// skip over crap
	while( ( ( ase.curpos - ase.buffer ) < ase.len ) &&
			( *ase.curpos <= 32 ) )
	{
		ase.curpos++;
	}

	while( ( ase.curpos - ase.buffer ) < ase.len )
	{
		ase.token[i] = *ase.curpos;

		ase.curpos++;
		i++;

		if( ( CharIsTokenDelimiter( ase.token[i - 1] ) && !restOfLine ) ||
				( ( ase.token[i - 1] == '\n' ) || ( ase.token[i - 1] == '\r' ) ) )
		{
			ase.token[i - 1] = 0;
			break;
		}
	}

	ase.token[i] = 0;

	return 1;
}

static void ASE_ParseBracedBlock( void ( *parser )( const char* token ) )
{
	int indent = 0;

	while( ASE_GetToken( false ) )
	{
		if( !strcmp( ase.token, "{" ) )
		{
			indent++;
		}
		else if( !strcmp( ase.token, "}" ) )
		{
			--indent;
			if( indent == 0 )
			{
				break;
			}
			else if( indent < 0 )
			{
				common->Error( "Unexpected '}'" );
			}
		}
		else
		{
			if( parser )
			{
				parser( ase.token );
			}
		}
	}
}

static void ASE_SkipEnclosingBraces()
{
	int indent = 0;

	while( ASE_GetToken( false ) )
	{
		if( !strcmp( ase.token, "{" ) )
		{
			indent++;
		}
		else if( !strcmp( ase.token, "}" ) )
		{
			indent--;
			if( indent == 0 )
			{
				break;
			}
			else if( indent < 0 )
			{
				common->Error( "Unexpected '}'" );
			}
		}
	}
}

static void ASE_SkipRestOfLine()
{
	ASE_GetToken( true );
}

static void ASE_KeyMAP_DIFFUSE( const char* token )
{
	aseMaterial_t*	material;

	if( !strcmp( token, "*BITMAP" ) )
	{
		idStr	qpath;
		idStr	matname;

		ASE_GetToken( false );

		// remove the quotes
		char* s = strstr( ase.token + 1, "\"" );
		if( s )
		{
			*s = 0;
		}
		matname = ase.token + 1;

		// convert the 3DSMax material pathname to a qpath
		matname.BackSlashesToSlashes();
		qpath = fileSystem->OSPathToRelativePath( matname );
		idStr::Copynz( ase.currentMaterial->name, qpath, sizeof( ase.currentMaterial->name ) );
	}
	else if( !strcmp( token, "*UVW_U_OFFSET" ) )
	{
		material = ase.model->materials[ase.model->materials.Num() - 1];
		ASE_GetToken( false );
		material->uOffset = atof( ase.token );
	}
	else if( !strcmp( token, "*UVW_V_OFFSET" ) )
	{
		material = ase.model->materials[ase.model->materials.Num() - 1];
		ASE_GetToken( false );
		material->vOffset = atof( ase.token );
	}
	else if( !strcmp( token, "*UVW_U_TILING" ) )
	{
		material = ase.model->materials[ase.model->materials.Num() - 1];
		ASE_GetToken( false );
		material->uTiling = atof( ase.token );
	}
	else if( !strcmp( token, "*UVW_V_TILING" ) )
	{
		material = ase.model->materials[ase.model->materials.Num() - 1];
		ASE_GetToken( false );
		material->vTiling = atof( ase.token );
	}
	else if( !strcmp( token, "*UVW_ANGLE" ) )
	{
		material = ase.model->materials[ase.model->materials.Num() - 1];
		ASE_GetToken( false );
		material->angle = atof( ase.token );
	}
	else
	{
	}
}

static void ASE_KeyMATERIAL( const char* token )
{
	if( !strcmp( token, "*MAP_DIFFUSE" ) )
	{
		ASE_ParseBracedBlock( ASE_KeyMAP_DIFFUSE );
	}
	else
	{
	}
}

static void ASE_KeyMATERIAL_LIST( const char* token )
{
	if( !strcmp( token, "*MATERIAL_COUNT" ) )
	{
		ASE_GetToken( false );
		VERBOSE( ( "..num materials: %s\n", ase.token ) );
	}
	else if( !strcmp( token, "*MATERIAL" ) )
	{
		VERBOSE( ( "..material %d\n", ase.model->materials.Num() ) );

		ase.currentMaterial = ( aseMaterial_t* )Mem_Alloc( sizeof( aseMaterial_t ), TAG_MODEL );
		memset( ase.currentMaterial, 0, sizeof( aseMaterial_t ) );
		ase.currentMaterial->uTiling = 1;
		ase.currentMaterial->vTiling = 1;
		ase.model->materials.Append( ase.currentMaterial );

		ASE_ParseBracedBlock( ASE_KeyMATERIAL );
	}
}

static void ASE_KeyNODE_TM( const char* token )
{
	int		i;

	if( !strcmp( token, "*TM_ROW0" ) )
	{
		for( i = 0 ; i < 3 ; i++ )
		{
			ASE_GetToken( false );
			ase.currentObject->mesh.transform[0][i] = atof( ase.token );
		}
	}
	else if( !strcmp( token, "*TM_ROW1" ) )
	{
		for( i = 0 ; i < 3 ; i++ )
		{
			ASE_GetToken( false );
			ase.currentObject->mesh.transform[1][i] = atof( ase.token );
		}
	}
	else if( !strcmp( token, "*TM_ROW2" ) )
	{
		for( i = 0 ; i < 3 ; i++ )
		{
			ASE_GetToken( false );
			ase.currentObject->mesh.transform[2][i] = atof( ase.token );
		}
	}
	else if( !strcmp( token, "*TM_ROW3" ) )
	{
		for( i = 0 ; i < 3 ; i++ )
		{
			ASE_GetToken( false );
			ase.currentObject->mesh.transform[3][i] = atof( ase.token );
		}
	}
}

static void ASE_KeyMESH_VERTEX_LIST( const char* token )
{
	aseMesh_t* pMesh = ASE_GetCurrentMesh();

	if( !strcmp( token, "*MESH_VERTEX" ) )
	{
		ASE_GetToken( false );		// skip number

		ASE_GetToken( false );
		pMesh->vertexes[ase.currentVertex].x = atof( ase.token );

		ASE_GetToken( false );
		pMesh->vertexes[ase.currentVertex].y = atof( ase.token );

		ASE_GetToken( false );
		pMesh->vertexes[ase.currentVertex].z = atof( ase.token );

		ase.currentVertex++;

		if( ase.currentVertex > pMesh->numVertexes )
		{
			common->Error( "ase.currentVertex >= pMesh->numVertexes" );
		}
	}
	else
	{
		common->Error( "Unknown token '%s' while parsing MESH_VERTEX_LIST", token );
	}
}

static void ASE_KeyMESH_FACE_LIST( const char* token )
{
	aseMesh_t* pMesh = ASE_GetCurrentMesh();

	if( !strcmp( token, "*MESH_FACE" ) )
	{
		ASE_GetToken( false );	// skip face number

		// we are flipping the order here to change the front/back facing
		// from 3DS to our standard (clockwise facing out)
		ASE_GetToken( false );	// skip label
		ASE_GetToken( false );	// first vertex
		pMesh->faces[ase.currentFace].vertexNum[0] = atoi( ase.token );

		ASE_GetToken( false );	// skip label
		ASE_GetToken( false );	// second vertex
		pMesh->faces[ase.currentFace].vertexNum[2] = atoi( ase.token );

		ASE_GetToken( false );	// skip label
		ASE_GetToken( false );	// third vertex
		pMesh->faces[ase.currentFace].vertexNum[1] = atoi( ase.token );

		ASE_GetToken( true );

		// we could parse material id and smoothing groups here
		/*
				if ( ( p = strstr( ase.token, "*MESH_MTLID" ) ) != 0 )
				{
					p += strlen( "*MESH_MTLID" ) + 1;
					mtlID = atoi( p );
				}
				else
				{
					common->Error( "No *MESH_MTLID found for face!" );
				}
		*/

		ase.currentFace++;
	}
	else
	{
		common->Error( "Unknown token '%s' while parsing MESH_FACE_LIST", token );
	}
}

static void ASE_KeyTFACE_LIST( const char* token )
{
	aseMesh_t* pMesh = ASE_GetCurrentMesh();

	if( !strcmp( token, "*MESH_TFACE" ) )
	{
		int a, b, c;

		ASE_GetToken( false );

		ASE_GetToken( false );
		a = atoi( ase.token );
		ASE_GetToken( false );
		c = atoi( ase.token );
		ASE_GetToken( false );
		b = atoi( ase.token );

		pMesh->faces[ase.currentFace].tVertexNum[0] = a;
		pMesh->faces[ase.currentFace].tVertexNum[1] = b;
		pMesh->faces[ase.currentFace].tVertexNum[2] = c;

		ase.currentFace++;
	}
	else
	{
		common->Error( "Unknown token '%s' in MESH_TFACE", token );
	}
}

static void ASE_KeyCFACE_LIST( const char* token )
{
	aseMesh_t* pMesh = ASE_GetCurrentMesh();

	if( !strcmp( token, "*MESH_CFACE" ) )
	{
		ASE_GetToken( false );

		for( int i = 0 ; i < 3 ; i++ )
		{
			ASE_GetToken( false );
			int a = atoi( ase.token );

			// we flip the vertex order to change the face direction to our style
			static int remap[3] = { 0, 2, 1 };
			pMesh->faces[ase.currentFace].vertexColors[remap[i]][0] = pMesh->cvertexes[a][0] * 255;
			pMesh->faces[ase.currentFace].vertexColors[remap[i]][1] = pMesh->cvertexes[a][1] * 255;
			pMesh->faces[ase.currentFace].vertexColors[remap[i]][2] = pMesh->cvertexes[a][2] * 255;
		}

		ase.currentFace++;
	}
	else
	{
		common->Error( "Unknown token '%s' in MESH_CFACE", token );
	}
}

static void ASE_KeyMESH_TVERTLIST( const char* token )
{
	aseMesh_t* pMesh = ASE_GetCurrentMesh();

	if( !strcmp( token, "*MESH_TVERT" ) )
	{
		const int maxLength = 80;
		char u[maxLength], v[maxLength], w[maxLength];

		ASE_GetToken( false );

		ASE_GetToken( false );
		strncpy( u, ase.token, maxLength );
		u[maxLength - 1] = '\0';

		ASE_GetToken( false );
		strncpy( v, ase.token, maxLength );
		v[maxLength - 1] = '\0';

		ASE_GetToken( false );
		strncpy( w, ase.token, maxLength );
		w[maxLength - 1] = '\0';

		pMesh->tvertexes[ase.currentVertex].x = atof( u );
		// our OpenGL second texture axis is inverted from MAX's sense
		pMesh->tvertexes[ase.currentVertex].y = 1.0f - atof( v );

		ase.currentVertex++;

		if( ase.currentVertex > pMesh->numTVertexes )
		{
			common->Error( "ase.currentVertex > pMesh->numTVertexes" );
		}
	}
	else
	{
		common->Error( "Unknown token '%s' while parsing MESH_TVERTLIST", token );
	}
}

static void ASE_KeyMESH_CVERTLIST( const char* token )
{
	aseMesh_t* pMesh = ASE_GetCurrentMesh();

	pMesh->colorsParsed = true;

	if( !strcmp( token, "*MESH_VERTCOL" ) )
	{
		ASE_GetToken( false );

		ASE_GetToken( false );
		pMesh->cvertexes[ase.currentVertex][0] = atof( token );

		ASE_GetToken( false );
		pMesh->cvertexes[ase.currentVertex][1] = atof( token );

		ASE_GetToken( false );
		pMesh->cvertexes[ase.currentVertex][2] = atof( token );

		ase.currentVertex++;

		if( ase.currentVertex > pMesh->numCVertexes )
		{
			common->Error( "ase.currentVertex > pMesh->numCVertexes" );
		}
	}
	else
	{
		common->Error( "Unknown token '%s' while parsing MESH_CVERTLIST", token );
	}
}

static void ASE_KeyMESH_NORMALS( const char* token )
{
	aseMesh_t* pMesh = ASE_GetCurrentMesh();
	aseFace_t*	f;
	idVec3		n;

	pMesh->normalsParsed = true;
	f = &pMesh->faces[ase.currentFace];

	if( !strcmp( token, "*MESH_FACENORMAL" ) )
	{
		int	num;

		ASE_GetToken( false );
		num = atoi( ase.token );

		if( num >= pMesh->numFaces || num < 0 )
		{
			common->Error( "MESH_NORMALS face index out of range: %i", num );
		}

		if( num != ase.currentFace )
		{
			common->Error( "MESH_NORMALS face index != currentFace" );
		}

		ASE_GetToken( false );
		n[0] = atof( ase.token );
		ASE_GetToken( false );
		n[1] = atof( ase.token );
		ASE_GetToken( false );
		n[2] = atof( ase.token );

		f->faceNormal[0] = n[0] * pMesh->transform[0][0] + n[1] * pMesh->transform[1][0] + n[2] * pMesh->transform[2][0];
		f->faceNormal[1] = n[0] * pMesh->transform[0][1] + n[1] * pMesh->transform[1][1] + n[2] * pMesh->transform[2][1];
		f->faceNormal[2] = n[0] * pMesh->transform[0][2] + n[1] * pMesh->transform[1][2] + n[2] * pMesh->transform[2][2];

		f->faceNormal.Normalize();

		ase.currentFace++;
	}
	else if( !strcmp( token, "*MESH_VERTEXNORMAL" ) )
	{
		int	num;
		int	v;

		ASE_GetToken( false );
		num = atoi( ase.token );

		if( num >= pMesh->numVertexes || num < 0 )
		{
			common->Error( "MESH_NORMALS vertex index out of range: %i", num );
		}

		f = &pMesh->faces[ ase.currentFace - 1 ];

		for( v = 0 ; v < 3 ; v++ )
		{
			if( num == f->vertexNum[ v ] )
			{
				break;
			}
		}

		if( v >= 3 )
		{
			common->Error( "MESH_NORMALS vertex index doesn't match face" );
			return;
		}

		ASE_GetToken( false );
		n[0] = atof( ase.token );
		ASE_GetToken( false );
		n[1] = atof( ase.token );
		ASE_GetToken( false );
		n[2] = atof( ase.token );

		f->vertexNormals[ v ][0] = n[0] * pMesh->transform[0][0] + n[1] * pMesh->transform[1][0] + n[2] * pMesh->transform[2][0];
		f->vertexNormals[ v ][1] = n[0] * pMesh->transform[0][1] + n[1] * pMesh->transform[1][1] + n[2] * pMesh->transform[2][1];
		f->vertexNormals[ v ][2] = n[0] * pMesh->transform[0][2] + n[1] * pMesh->transform[1][2] + n[2] * pMesh->transform[2][2];

		f->vertexNormals[v].Normalize();
	}
}

static void ASE_KeyMESH( const char* token )
{
	aseMesh_t* pMesh = ASE_GetCurrentMesh();

	if( !strcmp( token, "*TIMEVALUE" ) )
	{
		ASE_GetToken( false );

		pMesh->timeValue = atoi( ase.token );
		VERBOSE( ( ".....timevalue: %d\n", pMesh->timeValue ) );
	}
	else if( !strcmp( token, "*MESH_NUMVERTEX" ) )
	{
		ASE_GetToken( false );

		pMesh->numVertexes = atoi( ase.token );
		VERBOSE( ( ".....num vertexes: %d\n", pMesh->numVertexes ) );
	}
	else if( !strcmp( token, "*MESH_NUMTVERTEX" ) )
	{
		ASE_GetToken( false );

		pMesh->numTVertexes = atoi( ase.token );
		VERBOSE( ( ".....num tvertexes: %d\n", pMesh->numTVertexes ) );
	}
	else if( !strcmp( token, "*MESH_NUMCVERTEX" ) )
	{
		ASE_GetToken( false );

		pMesh->numCVertexes = atoi( ase.token );
		VERBOSE( ( ".....num cvertexes: %d\n", pMesh->numCVertexes ) );
	}
	else if( !strcmp( token, "*MESH_NUMFACES" ) )
	{
		ASE_GetToken( false );

		pMesh->numFaces = atoi( ase.token );
		VERBOSE( ( ".....num faces: %d\n", pMesh->numFaces ) );
	}
	else if( !strcmp( token, "*MESH_NUMTVFACES" ) )
	{
		ASE_GetToken( false );

		pMesh->numTVFaces = atoi( ase.token );
		VERBOSE( ( ".....num tvfaces: %d\n", pMesh->numTVFaces ) );

		if( pMesh->numTVFaces != pMesh->numFaces )
		{
			common->Error( "MESH_NUMTVFACES != MESH_NUMFACES" );
		}
	}
	else if( !strcmp( token, "*MESH_NUMCVFACES" ) )
	{
		ASE_GetToken( false );

		pMesh->numCVFaces = atoi( ase.token );
		VERBOSE( ( ".....num cvfaces: %d\n", pMesh->numCVFaces ) );

		if( pMesh->numTVFaces != pMesh->numFaces )
		{
			common->Error( "MESH_NUMCVFACES != MESH_NUMFACES" );
		}
	}
	else if( !strcmp( token, "*MESH_VERTEX_LIST" ) )
	{
		pMesh->vertexes = ( idVec3* )Mem_Alloc( sizeof( idVec3 ) * pMesh->numVertexes, TAG_MODEL );
		ase.currentVertex = 0;
		VERBOSE( ( ".....parsing MESH_VERTEX_LIST\n" ) );
		ASE_ParseBracedBlock( ASE_KeyMESH_VERTEX_LIST );
	}
	else if( !strcmp( token, "*MESH_TVERTLIST" ) )
	{
		ase.currentVertex = 0;
		pMesh->tvertexes = ( idVec2* )Mem_Alloc( sizeof( idVec2 ) * pMesh->numTVertexes, TAG_MODEL );
		VERBOSE( ( ".....parsing MESH_TVERTLIST\n" ) );
		ASE_ParseBracedBlock( ASE_KeyMESH_TVERTLIST );
	}
	else if( !strcmp( token, "*MESH_CVERTLIST" ) )
	{
		ase.currentVertex = 0;
		pMesh->cvertexes = ( idVec3* )Mem_Alloc( sizeof( idVec3 ) * pMesh->numCVertexes, TAG_MODEL );
		VERBOSE( ( ".....parsing MESH_CVERTLIST\n" ) );
		ASE_ParseBracedBlock( ASE_KeyMESH_CVERTLIST );
	}
	else if( !strcmp( token, "*MESH_FACE_LIST" ) )
	{
		pMesh->faces = ( aseFace_t* )Mem_Alloc( sizeof( aseFace_t ) * pMesh->numFaces, TAG_MODEL );
		ase.currentFace = 0;
		VERBOSE( ( ".....parsing MESH_FACE_LIST\n" ) );
		ASE_ParseBracedBlock( ASE_KeyMESH_FACE_LIST );
	}
	else if( !strcmp( token, "*MESH_TFACELIST" ) )
	{
		if( !pMesh->faces )
		{
			common->Error( "*MESH_TFACELIST before *MESH_FACE_LIST" );
		}
		ase.currentFace = 0;
		VERBOSE( ( ".....parsing MESH_TFACE_LIST\n" ) );
		ASE_ParseBracedBlock( ASE_KeyTFACE_LIST );
	}
	else if( !strcmp( token, "*MESH_CFACELIST" ) )
	{
		if( !pMesh->faces )
		{
			common->Error( "*MESH_CFACELIST before *MESH_FACE_LIST" );
		}
		ase.currentFace = 0;
		VERBOSE( ( ".....parsing MESH_CFACE_LIST\n" ) );
		ASE_ParseBracedBlock( ASE_KeyCFACE_LIST );
	}
	else if( !strcmp( token, "*MESH_NORMALS" ) )
	{
		if( !pMesh->faces )
		{
			common->Warning( "*MESH_NORMALS before *MESH_FACE_LIST" );
		}
		ase.currentFace = 0;
		VERBOSE( ( ".....parsing MESH_NORMALS\n" ) );
		ASE_ParseBracedBlock( ASE_KeyMESH_NORMALS );
	}
}

static void ASE_KeyMESH_ANIMATION( const char* token )
{
	aseMesh_t* mesh;

	// loads a single animation frame
	if( !strcmp( token, "*MESH" ) )
	{
		VERBOSE( ( "...found MESH\n" ) );

		mesh = ( aseMesh_t* )Mem_Alloc( sizeof( aseMesh_t ), TAG_MODEL );
		memset( mesh, 0, sizeof( aseMesh_t ) );
		ase.currentMesh = mesh;

		ase.currentObject->frames.Append( mesh );

		ASE_ParseBracedBlock( ASE_KeyMESH );
	}
	else
	{
		common->Error( "Unknown token '%s' while parsing MESH_ANIMATION", token );
	}
}

static void ASE_KeyGEOMOBJECT( const char* token )
{
	aseObject_t*	object;

	object = ase.currentObject;

	if( !strcmp( token, "*NODE_NAME" ) )
	{
		ASE_GetToken( true );
		VERBOSE( ( " %s\n", ase.token ) );
		idStr::Copynz( object->name, ase.token, sizeof( object->name ) );
	}
	else if( !strcmp( token, "*NODE_PARENT" ) )
	{
		ASE_SkipRestOfLine();
	}
	// ignore unused data blocks
	else if( !strcmp( token, "*NODE_TM" ) ||
			 !strcmp( token, "*TM_ANIMATION" ) )
	{
		ASE_ParseBracedBlock( ASE_KeyNODE_TM );
	}
	// ignore regular meshes that aren't part of animation
	else if( !strcmp( token, "*MESH" ) )
	{
		ase.currentMesh = &ase.currentObject->mesh;
		idVec3	transforms[ 4 ];
		for( int i = 0; i < 4; ++i )
		{
			transforms[ i ] = ase.currentMesh->transform[ i ];
		}

		memset( ase.currentMesh, 0, sizeof( *ase.currentMesh ) );
		for( int i = 0; i < 4; ++i )
		{
			ase.currentMesh->transform[ i ] = transforms[ i ];
		}

		ASE_ParseBracedBlock( ASE_KeyMESH );
	}
	// according to spec these are obsolete
	else if( !strcmp( token, "*MATERIAL_REF" ) )
	{
		ASE_GetToken( false );

		object->materialRef = atoi( ase.token );
	}
	// loads a sequence of animation frames
	else if( !strcmp( token, "*MESH_ANIMATION" ) )
	{
		VERBOSE( ( "..found MESH_ANIMATION\n" ) );

		ASE_ParseBracedBlock( ASE_KeyMESH_ANIMATION );
	}
	// skip unused info
	else if( !strcmp( token, "*PROP_MOTIONBLUR" ) ||
			 !strcmp( token, "*PROP_CASTSHADOW" ) ||
			 !strcmp( token, "*PROP_RECVSHADOW" ) )
	{
		ASE_SkipRestOfLine();
	}

}

void ASE_ParseGeomObject()
{
	aseObject_t*	object;

	VERBOSE( ( "GEOMOBJECT" ) );

	object = ( aseObject_t* )Mem_Alloc( sizeof( aseObject_t ), TAG_MODEL );
	memset( object, 0, sizeof( aseObject_t ) );
	ase.model->objects.Append( object );
	ase.currentObject = object;

	object->frames.Resize( 32, 32 );

	ASE_ParseBracedBlock( ASE_KeyGEOMOBJECT );
}

static void ASE_KeyGROUP( const char* token )
{
	if( !strcmp( token, "*GEOMOBJECT" ) )
	{
		ASE_ParseGeomObject();
	}
}

/*
=================
ASE_Parse
=================
*/
aseModel_t* ASE_Parse( const char* buffer, bool verbose )
{
	memset( &ase, 0, sizeof( ase ) );

	ase.verbose = verbose;

	ase.buffer = buffer;
	ase.len = strlen( buffer );
	ase.curpos = ase.buffer;
	ase.currentObject = NULL;

	// NOTE: using new operator because aseModel_t contains idList class objects
	ase.model = new( TAG_MODEL ) aseModel_t;
	memset( ase.model, 0, sizeof( aseModel_t ) );
	ase.model->objects.Resize( 32, 32 );
	ase.model->materials.Resize( 32, 32 );

	while( ASE_GetToken( false ) )
	{
		if( !strcmp( ase.token, "*3DSMAX_ASCIIEXPORT" ) ||
				!strcmp( ase.token, "*COMMENT" ) )
		{
			ASE_SkipRestOfLine();
		}
		else if( !strcmp( ase.token, "*SCENE" ) )
		{
			ASE_SkipEnclosingBraces();
		}
		else if( !strcmp( ase.token, "*GROUP" ) )
		{
			ASE_GetToken( false );		// group name
			ASE_ParseBracedBlock( ASE_KeyGROUP );
		}
		else if( !strcmp( ase.token, "*SHAPEOBJECT" ) )
		{
			ASE_SkipEnclosingBraces();
		}
		else if( !strcmp( ase.token, "*CAMERAOBJECT" ) )
		{
			ASE_SkipEnclosingBraces();
		}
		else if( !strcmp( ase.token, "*MATERIAL_LIST" ) )
		{
			VERBOSE( ( "MATERIAL_LIST\n" ) );

			ASE_ParseBracedBlock( ASE_KeyMATERIAL_LIST );
		}
		else if( !strcmp( ase.token, "*GEOMOBJECT" ) )
		{
			ASE_ParseGeomObject();
		}
		else if( ase.token[0] )
		{
			common->Printf( "Unknown token '%s'\n", ase.token );
		}
	}

	return ase.model;
}

/*
=================
ASE_Load
=================
*/
aseModel_t* ASE_Load( const char* fileName )
{
	char* buf;
	ID_TIME_T timeStamp;
	aseModel_t* ase;

	fileSystem->ReadFile( fileName, ( void** )&buf, &timeStamp );
	if( !buf )
	{
		return NULL;
	}

	ase = ASE_Parse( buf, false );
	ase->timeStamp = timeStamp;

	fileSystem->FreeFile( buf );

	return ase;
}

/*
=================
ASE_Free
=================
*/
void ASE_Free( aseModel_t* ase )
{
	int					i, j;
	aseObject_t*			obj;
	aseMesh_t*			mesh;
	aseMaterial_t*		material;

	if( !ase )
	{
		return;
	}
	for( i = 0; i < ase->objects.Num(); i++ )
	{
		obj = ase->objects[i];
		for( j = 0; j < obj->frames.Num(); j++ )
		{
			mesh = obj->frames[j];
			if( mesh->vertexes )
			{
				Mem_Free( mesh->vertexes );
			}
			if( mesh->tvertexes )
			{
				Mem_Free( mesh->tvertexes );
			}
			if( mesh->cvertexes )
			{
				Mem_Free( mesh->cvertexes );
			}
			if( mesh->faces )
			{
				Mem_Free( mesh->faces );
			}
			Mem_Free( mesh );
		}

		obj->frames.Clear();

		// free the base nesh
		mesh = &obj->mesh;
		if( mesh->vertexes )
		{
			Mem_Free( mesh->vertexes );
		}
		if( mesh->tvertexes )
		{
			Mem_Free( mesh->tvertexes );
		}
		if( mesh->cvertexes )
		{
			Mem_Free( mesh->cvertexes );
		}
		if( mesh->faces )
		{
			Mem_Free( mesh->faces );
		}
		Mem_Free( obj );
	}
	ase->objects.Clear();

	for( i = 0; i < ase->materials.Num(); i++ )
	{
		material = ase->materials[i];
		Mem_Free( material );
	}
	ase->materials.Clear();

	delete ase;
}
