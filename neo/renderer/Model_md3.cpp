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

#include "RenderCommon.h"
#include "Model_local.h"
#include "Model_md3.h"

/***********************************************************************

	idMD3Mesh

***********************************************************************/

// DG: added constructor to make sure all members are initialized
idRenderModelMD3::idRenderModelMD3() : index( -1 ), dataSize( 0 ), md3( NULL ), numLods( 0 )
{}

#define	LL(x) x=LittleLong(x)

/*
=================
idRenderModelMD3::InitFromFile
=================
*/
void idRenderModelMD3::InitFromFile( const char* fileName, const idImportOptions* options )
{
	int					i, j;
	md3Header_t*			pinmodel;
	md3Frame_t*			frame;
	md3Surface_t*		surf;
	md3Shader_t*			shader;
	md3Triangle_t*		tri;
	md3St_t*				st;
	md3XyzNormal_t*		xyz;
	md3Tag_t*			tag;
	void*				buffer;
	int					version;
	int					size;


	name = fileName;

	size = fileSystem->ReadFile( fileName, &buffer, NULL );
	if( !buffer || size < 0 )
	{
		return;
	}

	pinmodel = ( md3Header_t* )buffer;

	version = LittleLong( pinmodel->version );
	if( version != MD3_VERSION )
	{
		fileSystem->FreeFile( buffer );
		common->Warning( "InitFromFile: %s has wrong version (%i should be %i)",
						 fileName, version, MD3_VERSION );
		return;
	}

	size = LittleLong( pinmodel->ofsEnd );
	dataSize += size;
	md3 = ( md3Header_t* )Mem_Alloc( size, TAG_MODEL );

	memcpy( md3, buffer, LittleLong( pinmodel->ofsEnd ) );

	LL( md3->ident );
	LL( md3->version );
	LL( md3->numFrames );
	LL( md3->numTags );
	LL( md3->numSurfaces );
	LL( md3->ofsFrames );
	LL( md3->ofsTags );
	LL( md3->ofsSurfaces );
	LL( md3->ofsEnd );

	if( md3->numFrames < 1 )
	{
		common->Warning( "InitFromFile: %s has no frames", fileName );
		fileSystem->FreeFile( buffer );
		return;
	}

	// swap all the frames
	frame = ( md3Frame_t* )( ( byte* )md3 + md3->ofsFrames );
	for( i = 0 ; i < md3->numFrames ; i++, frame++ )
	{
		frame->radius = LittleFloat( frame->radius );
		for( j = 0 ; j < 3 ; j++ )
		{
			frame->bounds[0][j] = LittleFloat( frame->bounds[0][j] );
			frame->bounds[1][j] = LittleFloat( frame->bounds[1][j] );
			frame->localOrigin[j] = LittleFloat( frame->localOrigin[j] );
		}
	}

	// swap all the tags
	tag = ( md3Tag_t* )( ( byte* )md3 + md3->ofsTags );
	for( i = 0 ; i < md3->numTags * md3->numFrames ; i++, tag++ )
	{
		for( j = 0 ; j < 3 ; j++ )
		{
			tag->origin[j] = LittleFloat( tag->origin[j] );
			tag->axis[0][j] = LittleFloat( tag->axis[0][j] );
			tag->axis[1][j] = LittleFloat( tag->axis[1][j] );
			tag->axis[2][j] = LittleFloat( tag->axis[2][j] );
		}
	}

	// swap all the surfaces
	surf = ( md3Surface_t* )( ( byte* )md3 + md3->ofsSurfaces );
	for( i = 0 ; i < md3->numSurfaces ; i++ )
	{

		LL( surf->ident );
		LL( surf->flags );
		LL( surf->numFrames );
		LL( surf->numShaders );
		LL( surf->numTriangles );
		LL( surf->ofsTriangles );
		LL( surf->numVerts );
		LL( surf->ofsShaders );
		LL( surf->ofsSt );
		LL( surf->ofsXyzNormals );
		LL( surf->ofsEnd );

		if( surf->numVerts > SHADER_MAX_VERTEXES )
		{
			common->Error( "InitFromFile: %s has more than %i verts on a surface (%i)",
						   fileName, SHADER_MAX_VERTEXES, surf->numVerts );
		}
		if( surf->numTriangles * 3 > SHADER_MAX_INDEXES )
		{
			common->Error( "InitFromFile: %s has more than %i triangles on a surface (%i)",
						   fileName, SHADER_MAX_INDEXES / 3, surf->numTriangles );
		}

		// change to surface identifier
		surf->ident = 0;	//SF_MD3;

		// lowercase the surface name so skin compares are faster
		int slen = ( int )strlen( surf->name );
		for( j = 0; j < slen; j++ )
		{
			surf->name[j] = tolower( surf->name[j] );
		}

		// strip off a trailing _1 or _2
		// this is a crutch for q3data being a mess
		j = strlen( surf->name );
		if( j > 2 && surf->name[j - 2] == '_' )
		{
			surf->name[j - 2] = 0;
		}

		// register the shaders
		shader = ( md3Shader_t* )( ( byte* )surf + surf->ofsShaders );
		for( j = 0 ; j < surf->numShaders ; j++, shader++ )
		{
			const idMaterial* sh = declManager->FindMaterial( shader->name );
			// DG: md3Shadder_t must use an index to the material instead of a pointer,
			//     otherwise the sizes are wrong on 64bit and we get data corruption
			shader->shaderIndex = ( sh != NULL ) ? shaders.AddUnique( sh ) : -1;
		}

		// swap all the triangles
		tri = ( md3Triangle_t* )( ( byte* )surf + surf->ofsTriangles );
		for( j = 0 ; j < surf->numTriangles ; j++, tri++ )
		{
			LL( tri->indexes[0] );
			LL( tri->indexes[1] );
			LL( tri->indexes[2] );
		}

		// swap all the ST
		st = ( md3St_t* )( ( byte* )surf + surf->ofsSt );
		for( j = 0 ; j < surf->numVerts ; j++, st++ )
		{
			st->st[0] = LittleFloat( st->st[0] );
			st->st[1] = LittleFloat( st->st[1] );
		}

		// swap all the XyzNormals
		xyz = ( md3XyzNormal_t* )( ( byte* )surf + surf->ofsXyzNormals );
		for( j = 0 ; j < surf->numVerts * surf->numFrames ; j++, xyz++ )
		{
			xyz->xyz[0] = LittleShort( xyz->xyz[0] );
			xyz->xyz[1] = LittleShort( xyz->xyz[1] );
			xyz->xyz[2] = LittleShort( xyz->xyz[2] );

			xyz->normal = LittleShort( xyz->normal );
		}


		// find the next surface
		surf = ( md3Surface_t* )( ( byte* )surf + surf->ofsEnd );
	}

	fileSystem->FreeFile( buffer );
}

/*
=================
idRenderModelMD3::IsDynamicModel
=================
*/
dynamicModel_t idRenderModelMD3::IsDynamicModel() const
{
	return DM_CACHED;
}

/*
=================
idRenderModelMD3::LerpMeshVertexes
=================
*/
void idRenderModelMD3::LerpMeshVertexes( srfTriangles_t* tri, const struct md3Surface_s* surf, const float backlerp, const int frame, const int oldframe ) const
{
	short*	oldXyz, *newXyz;
	float	oldXyzScale, newXyzScale;
	int		vertNum;
	int		numVerts;

	newXyz = ( short* )( ( byte* )surf + surf->ofsXyzNormals ) + ( frame * surf->numVerts * 4 );

	newXyzScale = MD3_XYZ_SCALE * ( 1.0 - backlerp );

	numVerts = surf->numVerts;

	if( backlerp == 0 )
	{
		//
		// just copy the vertexes
		//
		for( vertNum = 0 ; vertNum < numVerts ; vertNum++, newXyz += 4 )
		{

			idDrawVert* outvert = &tri->verts[tri->numVerts];

			outvert->xyz.x = newXyz[0] * newXyzScale;
			outvert->xyz.y = newXyz[1] * newXyzScale;
			outvert->xyz.z = newXyz[2] * newXyzScale;

			tri->numVerts++;
		}
	}
	else
	{
		//
		// interpolate and copy the vertexes
		//
		oldXyz = ( short* )( ( byte* )surf + surf->ofsXyzNormals ) + ( oldframe * surf->numVerts * 4 );

		oldXyzScale = MD3_XYZ_SCALE * backlerp;

		for( vertNum = 0 ; vertNum < numVerts ; vertNum++, oldXyz += 4, newXyz += 4 )
		{

			idDrawVert* outvert = &tri->verts[tri->numVerts];

			// interpolate the xyz
			outvert->xyz.x = oldXyz[0] * oldXyzScale + newXyz[0] * newXyzScale;
			outvert->xyz.y = oldXyz[1] * oldXyzScale + newXyz[1] * newXyzScale;
			outvert->xyz.z = oldXyz[2] * oldXyzScale + newXyz[2] * newXyzScale;

			tri->numVerts++;
		}
	}
}

/*
=============
idRenderModelMD3::InstantiateDynamicModel
=============
*/
idRenderModel* idRenderModelMD3::InstantiateDynamicModel( const struct renderEntity_s* ent, const viewDef_t* view, idRenderModel* cachedModel )
{
	int				i, j;
	float			backlerp;
	int* 			triangles;
	int				indexes;
	int				numVerts;
	md3Surface_t* 	surface;
	int				frame, oldframe;
	idRenderModelStatic*	staticModel;

	if( md3 == NULL )
	{
		return NULL;
	}

	if( cachedModel )
	{
		delete cachedModel;
		cachedModel = NULL;
	}

	staticModel = new( TAG_MODEL ) idRenderModelStatic;
	staticModel->bounds.Clear();

	surface = ( md3Surface_t* )( ( byte* )md3 + md3->ofsSurfaces );

	// TODO: these need set by an entity
	frame = ent->shaderParms[SHADERPARM_MD3_FRAME];			// probably want to keep frames < 1000 or so
	oldframe = ent->shaderParms[SHADERPARM_MD3_LASTFRAME];
	backlerp = ent->shaderParms[SHADERPARM_MD3_BACKLERP];

	for( i = 0; i < md3->numSurfaces; i++ )
	{

		srfTriangles_t* tri = R_AllocStaticTriSurf();
		R_AllocStaticTriSurfVerts( tri, surface->numVerts );
		R_AllocStaticTriSurfIndexes( tri, surface->numTriangles * 3 );
		tri->bounds.Clear();

		modelSurface_t	surf;

		surf.geometry = tri;

		md3Shader_t* shaders = ( md3Shader_t* )( ( byte* )surface + surface->ofsShaders );
		// DG: turned md3Shader_t::shader (pointer) into an int (index)
		int shaderIdx = shaders->shaderIndex;
		surf.shader = ( shaderIdx >= 0 ) ? this->shaders[shaderIdx] : NULL;

		LerpMeshVertexes( tri, surface, backlerp, frame, oldframe );

		triangles = ( int* )( ( byte* )surface + surface->ofsTriangles );
		indexes = surface->numTriangles * 3;
		for( j = 0 ; j < indexes ; j++ )
		{
			tri->indexes[j] = triangles[j];
		}
		tri->numIndexes += indexes;

		const idVec2* texCoords = ( idVec2* )( ( byte* )surface + surface->ofsSt );

		numVerts = surface->numVerts;
		for( j = 0; j < numVerts; j++ )
		{
			tri->verts[j].SetTexCoord( texCoords[j] );
		}

		R_BoundTriSurf( tri );

		surf.id = staticModel->NumSurfaces(); // DG: make sure to initialize id; FIXME: or just set id to 0?
		staticModel->AddSurface( surf );
		staticModel->bounds.AddPoint( surf.geometry->bounds[0] );
		staticModel->bounds.AddPoint( surf.geometry->bounds[1] );

		// find the next surface
		surface = ( md3Surface_t* )( ( byte* )surface + surface->ofsEnd );
	}

	return staticModel;
}

/*
=====================
idRenderModelMD3::Bounds
=====================
*/

idBounds idRenderModelMD3::Bounds( const struct renderEntity_s* ent ) const
{
	idBounds		ret;

	ret.Clear();

	if( !ent || !md3 )
	{
		// just give it the editor bounds
		ret.AddPoint( idVec3( -10, -10, -10 ) );
		ret.AddPoint( idVec3( 10, 10, 10 ) );
		return ret;
	}

	md3Frame_t*	frame = ( md3Frame_t* )( ( byte* )md3 + md3->ofsFrames );
	frame += ( int )ent->shaderParms[SHADERPARM_MD3_FRAME]; // DG: use bounds of current frame

	ret.AddPoint( frame->bounds[0] );
	ret.AddPoint( frame->bounds[1] );

	return ret;
}

