/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 2022 Harrie van Ginneken
Copyright (C) 2022 Robert Beckebans

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


#include "Model_gltf.h"
#include "Model_local.h"
#include "RenderCommon.h"

idCVar gltf_ForceBspMeshTexture( "gltf_ForceBspMeshTexture", "0", CVAR_SYSTEM | CVAR_BOOL, "all world geometry has the same forced texture" );
idCVar gltf_ModelSceneName( "gltf_ModelSceneName", "models", CVAR_SYSTEM , "Scene to use when loading specific models" );

bool idRenderModelStatic::ConvertGltfMeshToModelsurfaces( const gltfMesh* mesh )
{
	return false;
}

MapPolygonMesh* MapPolygonMesh::ConvertFromMeshGltf( const gltfMesh_Primitive* prim, gltfData* _data , idMat4 trans )
{
	MapPolygonMesh* mesh = new MapPolygonMesh();
	gltfAccessor* accessor = _data->AccessorList()[prim->indices];
	gltfBufferView* bv = _data->BufferViewList()[accessor->bufferView];
	gltfData* data = bv->parent;

	// files import as y-up. Use this transform to change the model to z-up.
	idMat3 rotation = idAngles( 0.0f, 0.0f, 90.0f ).ToMat3( );
	idMat4 axisTransform( rotation, vec3_origin );

	gltfMaterial* mat = NULL;
	if( prim->material != -1 )
	{
		mat = _data->MaterialList()[prim->material];
	}

	gltfBuffer* buff = data->BufferList()[bv->buffer];
	uint idxDataSize = sizeof( uint ) * accessor->count;
	uint* indices = ( uint* ) Mem_ClearedAlloc( idxDataSize , TAG_IDLIB_GLTF );

	idFile_Memory idxBin = idFile_Memory( "gltfChunkIndices",
										  ( const char* )( ( data->GetData( bv->buffer ) + bv->byteOffset + accessor->byteOffset ) ), bv->byteLength );

	for( int i = 0; i < accessor->count; i++ )
	{
		idxBin.Read( ( void* )( &indices[i] ), accessor->typeSize );
		if( bv->byteStride )
		{
			idxBin.Seek( bv->byteStride - accessor->typeSize, FS_SEEK_CUR );
		}
	}

	for( int i = 0; i < accessor->count; i += 3 )
	{
		MapPolygon& polygon = mesh->polygons.Alloc();

		if( mat != NULL && !gltf_ForceBspMeshTexture.GetBool() )
		{
			polygon.SetMaterial( mat->name );
		}
		else
		{
			polygon.SetMaterial( "textures/base_wall/snpanel2rust" );
		}

		polygon.AddIndex( indices[i + 2] );
		polygon.AddIndex( indices[i + 1] );
		polygon.AddIndex( indices[i + 0] );
	}

	Mem_Free( indices );
	bool sizeSet = false;

	for( auto& attrib : prim->attributes )
	{
		gltfAccessor* attrAcc = data->AccessorList()[attrib->accessorIndex];
		gltfBufferView* attrBv = data->BufferViewList()[attrAcc->bufferView];
		gltfData* attrData = attrBv->parent;
		gltfBuffer* attrbuff = attrData->BufferList()[attrBv->buffer];

		idFile_Memory bin = idFile_Memory( "gltfChunkVertices",
										   ( const char* )( ( attrData->GetData( attrBv->buffer ) + attrBv->byteOffset + attrAcc->byteOffset ) ), attrBv->byteLength );

		if( !sizeSet )
		{
			mesh->verts.AssureSize( attrAcc->count );
			sizeSet = true;
		}

		switch( attrib->type )
		{
			case gltfMesh_Primitive_Attribute::Type::Position:
			{
				for( int i = 0; i < attrAcc->count; i++ )
				{
					idVec3 pos;

					bin.Read( ( void* )( &pos.x ), attrAcc->typeSize );
					bin.Read( ( void* )( &pos.y ), attrAcc->typeSize );
					bin.Read( ( void* )( &pos.z ), attrAcc->typeSize );

					pos *= trans * axisTransform;

					mesh->verts[i].xyz.x = pos.x;
					mesh->verts[i].xyz.y = pos.y;
					mesh->verts[i].xyz.z = pos.z;

					if( attrBv->byteStride )
					{
						bin.Seek( attrBv->byteStride - ( 3 * attrAcc->typeSize ), FS_SEEK_CUR );
					}

					idRandom rnd( i );
					int r = rnd.RandomInt( 255 ), g = rnd.RandomInt( 255 ), b = rnd.RandomInt( 255 );

					//vtxData[i].abgr = 0xff000000 + ( b << 16 ) + ( g << 8 ) + r;
				}

				break;
			}
			case gltfMesh_Primitive_Attribute::Type::Normal:
			{
				idVec3 vec;
				for( int i = 0; i < attrAcc->count; i++ )
				{
					idVec3 vec;
					bin.Read( ( void* )( &vec.x ), attrAcc->typeSize );
					bin.Read( ( void* )( &vec.y ), attrAcc->typeSize );
					bin.Read( ( void* )( &vec.z ), attrAcc->typeSize );
					if( attrBv->byteStride )
					{
						bin.Seek( attrBv->byteStride - ( attrib->elementSize * attrAcc->typeSize ), FS_SEEK_CUR );
					}

					idVec3 normal;

					normal.x = vec.x;
					normal.y = vec.y;
					normal.z = vec.z;

					normal *= axisTransform;
					mesh->verts[i].SetNormal( normal );
				}

				break;
			}
			case gltfMesh_Primitive_Attribute::Type::TexCoord0:
			{
				idVec2 vec;
				for( int i = 0; i < attrAcc->count; i++ )
				{
					bin.Read( ( void* )( &vec.x ), attrAcc->typeSize );
					bin.Read( ( void* )( &vec.y ), attrAcc->typeSize );
					if( attrBv->byteStride )
					{
						bin.Seek( attrBv->byteStride - ( attrib->elementSize * attrAcc->typeSize ), FS_SEEK_CUR );
					}

					//vec.y = 1.0f - vec.y;
					mesh->verts[i].SetTexCoord( vec );
				}

				break;
			}
			case gltfMesh_Primitive_Attribute::Type::Tangent:
			{
				idVec4 vec;
				for( int i = 0; i < attrAcc->count; i++ )
				{
					bin.Read( ( void* )( &vec.x ), attrAcc->typeSize );
					bin.Read( ( void* )( &vec.y ), attrAcc->typeSize );
					bin.Read( ( void* )( &vec.z ), attrAcc->typeSize );
					bin.Read( ( void* )( &vec.w ), attrAcc->typeSize );
					if( attrBv->byteStride )
					{
						bin.Seek( attrBv->byteStride - ( attrib->elementSize * attrAcc->typeSize ), FS_SEEK_CUR );
					}

					idVec3 tangent;

					tangent.x = vec.x;
					tangent.y = vec.y;
					tangent.z = vec.z;

					tangent *= axisTransform;

					mesh->verts[i].SetTangent( tangent );
					mesh->verts[i].SetBiTangentSign( vec.w );
				}
				break;
			}
				//case gltfMesh_Primitive_Attribute::Type::Weight:
				//{
				//	for ( int i = 0; i < attrAcc->count; i++ ) {
				//		bin.Read( ( void * ) ( &vtxData[i].weight.x ), attrAcc->typeSize );
				//		bin.Read( ( void * ) ( &vtxData[i].weight.y ), attrAcc->typeSize );
				//		bin.Read( ( void * ) ( &vtxData[i].weight.z ), attrAcc->typeSize );
				//		bin.Read( ( void * ) ( &vtxData[i].weight.w ), attrAcc->typeSize );
				//		if ( attrBv->byteStride )
				//			bin.Seek( attrBv->byteStride - ( attrib->elementSize * attrAcc->typeSize ), FS_SEEK_CUR );
				//	}
				//	break;
				//}
				//case gltfMesh_Primitive_Attribute::Type::Indices:
				//{
				//	for ( int i = 0; i < attrAcc->count; i++ ) {
				//		bin.Read( ( void * ) ( &vtxData[i].boneIndex.x ), attrAcc->typeSize );
				//		bin.Read( ( void * ) ( &vtxData[i].boneIndex.y ), attrAcc->typeSize );
				//		bin.Read( ( void * ) ( &vtxData[i].boneIndex.z ), attrAcc->typeSize );
				//		bin.Read( ( void * ) ( &vtxData[i].boneIndex.w ), attrAcc->typeSize );
				//		if ( attrBv->byteStride )
				//			bin.Seek( attrBv->byteStride - ( attrib->elementSize * attrAcc->typeSize ), FS_SEEK_CUR );
				//	}
				//	break;
				//}
		}

	}
	mesh->SetContents();
	return mesh;
}

void ProcessSceneNode( idMapEntity* newEntity, gltfNode* node, idMat4& trans, gltfData* data , bool staticMesh = false )
{
	auto& nodeList = data->NodeList();

	gltfData::ResolveNodeMatrix( node );
	idMat4 curTrans = trans * node->matrix;

	idDict newPairs = node->extras.strPairs;
	newPairs.SetDefaults( &newEntity->epairs );
	newEntity->epairs = newPairs;

	const char* classname = newEntity->epairs.GetString( "classname" );
	const char* model = newEntity->epairs.GetString( "model" );

	bool isFuncStaticMesh = staticMesh || ( idStr::Icmp( classname, "func_static" ) == 0 ) && ( idStr::Icmp( model, node->name ) == 0 );

	for( auto& child : node->children )
	{
		ProcessSceneNode( newEntity, nodeList[child], curTrans, data, isFuncStaticMesh );
	}

	if( isFuncStaticMesh && node->mesh != -1 )
	{
		for( auto prim : data->MeshList()[node->mesh]->primitives )
		{
			newEntity->AddPrimitive( MapPolygonMesh::ConvertFromMeshGltf( prim, data , curTrans ) );
		}
	}

	if( node->name.Length() )
	{
		newEntity->epairs.Set( "name", node->name );
	}

#if 0
	for( int i = 0; i < newEntity->epairs.GetNumKeyVals(); i++ )
	{
		const idKeyValue* kv = newEntity->epairs.GetKeyVal( i );

		idLib::Printf( "entity[ %s ] key = '%s' value = '%s'\n", node->name.c_str(), kv->GetKey().c_str(), kv->GetValue().c_str() );
	}
#endif
	idVec3 origin;

	origin.x = node->translation.x;
	origin.y = node->translation.y;
	origin.z = node->translation.z;

	// files import as y-up. Use this transform to change the model to z-up.
	idMat3 rotation = idAngles( 0.0f, 0.0f, 90.0f ).ToMat3( );
	idMat4 axisTransform( rotation, vec3_origin );

	origin *= axisTransform;
	newEntity->epairs.Set( "origin", origin.ToString() );

	newEntity->epairs.Set( "rotation", node->rotation.ToMat3().Transpose().ToString() );
}

void Map_AddMeshes( idMapEntity* _Entity, gltfNode* _Node, idMat4& _Trans, gltfData* _Data )
{
	gltfData::ResolveNodeMatrix( _Node );
	idMat4 curTrans = _Trans * _Node->matrix;

	if( _Node->mesh != -1 )
	{
		for( auto prim : _Data->MeshList( )[_Node->mesh]->primitives )
		{
			_Entity->AddPrimitive( MapPolygonMesh::ConvertFromMeshGltf( prim, _Data, curTrans ) );
		}
	}

	for( auto& child : _Node->children )
	{
		Map_AddMeshes( _Entity, _Data->NodeList( )[child], curTrans, _Data );
	}
};

int idMapEntity::GetEntities( gltfData* data, EntityListRef entities, int sceneID )
{
	idMapEntity* worldspawn = new( TAG_IDLIB_GLTF ) idMapEntity();
	entities.Append( worldspawn );

	bool wpSet = false;

	int entityCount = 0;
	for( auto& nodeID :  data->SceneList()[sceneID]->nodes )
	{
		auto* node = data->NodeList()[nodeID];
		const char* classname = node->extras.strPairs.GetString( "classname" );

		bool isWorldSpawn = idStr::Icmp( classname, "worldspawn" ) == 0;
		if( isWorldSpawn )
		{
			assert( !wpSet );
			worldspawn->epairs.Copy( node->extras.strPairs );
			wpSet = true;
		}
		else
		{
			// account all meshes starting with "worldspawn." or "BSP" in the name
			if( idStr::Icmpn( node->name, "BSP", 3 ) == 0 || idStr::Icmpn( node->name, "worldspawn.", 11 ) == 0 )
			{
				Map_AddMeshes( worldspawn, node, mat4_identity, data );
			}
			else
			{
				idMapEntity* newEntity = new( TAG_IDLIB_GLTF ) idMapEntity();
				entities.Append( newEntity );

				ProcessSceneNode( newEntity, node, mat4_identity, data );

				entityCount++;
			}
		}
	}

	return entityCount;
}

//  not dots allowed in [%s]!
// [filename].[%i|%s].[gltf/glb]
bool gltfManager::ExtractMeshIdentifier( idStr& filename, int& meshId, idStr& meshName )
{
	idStr extension;
	idStr meshStr;
	filename.ExtractFileExtension( extension );
	idStr file = filename;
	file = file.StripFileExtension();
	file.ExtractFileExtension( meshStr );

	if( !extension.Length() )
	{
		idLib::Warning( "no gltf mesh identifier" );
		return false;
	}

	if( meshStr.Length() )
	{
		filename = file.Left( file.Length() - meshStr.Length() - 1 ) + "." + extension;

		idParser	parser( LEXFL_ALLOWPATHNAMES | LEXFL_NOSTRINGESCAPECHARS );
		parser.LoadMemory( meshStr.c_str(), meshStr.Size(), "model:GltfmeshID" );

		idToken token;
		if( parser.ExpectAnyToken( &token ) )
		{
			if( ( token.type == TT_NUMBER ) && ( token.subtype & TT_INTEGER ) )
			{
				meshId = token.GetIntValue();
			}
			else if( token.type == TT_NAME )
			{
				meshName = token;
			}
			else
			{
				parser.Warning( "malformed gltf mesh identifier" );
				return false;
			}
			return true;
		}
		else
		{
			parser.Warning( "malformed gltf mesh identifier" );
		}
	}

	return false;
}

void idRenderModelGLTF::ProcessNode( gltfNode* modelNode, idMat4 trans, gltfData* data )
{
	auto& meshList = data->MeshList();
	auto& nodeList = data->NodeList();

	gltfData::ResolveNodeMatrix( modelNode );
	idMat4 curTrans = trans * modelNode->matrix;

	gltfMesh* targetMesh = meshList[modelNode->mesh];

	for( auto prim : targetMesh->primitives )
	{
		auto* newMesh = MapPolygonMesh::ConvertFromMeshGltf( prim, data, curTrans );
		modelSurface_t	surf;

		gltfMaterial* mat = NULL;
		if( prim->material != -1 )
		{
			mat = data->MaterialList()[prim->material];
		}
		if( mat != NULL && !gltf_ForceBspMeshTexture.GetBool() )
		{
			surf.shader = declManager->FindMaterial( mat->name );
		}
		else
		{
			surf.shader = declManager->FindMaterial( "textures/base_wall/snpanel2rust" );
		}
		surf.id = this->NumSurfaces();

		srfTriangles_t* tri = R_AllocStaticTriSurf();
		tri->numIndexes = newMesh->GetNumPolygons() * 3;
		tri->numVerts = newMesh->GetNumVertices();

		R_AllocStaticTriSurfIndexes( tri, tri->numIndexes );
		R_AllocStaticTriSurfVerts( tri, tri->numVerts );

		int indx = 0;
		for( int i = 0; i < newMesh->GetNumPolygons(); i++ )
		{
			auto& face = newMesh->GetFace( i );
			auto& faceIdxs = face.GetIndexes();
			tri->indexes[indx] = faceIdxs[0];
			tri->indexes[indx + 1] = faceIdxs[1];
			tri->indexes[indx + 2] = faceIdxs[2];
			indx += 3;
		}

		for( int i = 0; i < tri->numVerts; ++i )
		{
			tri->verts[i] = newMesh->GetDrawVerts()[i];
			tri->bounds.AddPoint( tri->verts[i].xyz );
		}

		bounds.AddBounds( tri->bounds );

		surf.geometry = tri;
		AddSurface( surf );
	}

	for( auto& child : modelNode->children )
	{
		ProcessNode( nodeList[child], curTrans, data );
	}
}

//constructs a renderModel from a gltfScene node found in the "models" scene of the given gltfFile.
// override with gltf_ModelSceneName
// warning : nodeName cannot have dots!
//[fileName].[nodeName/nodeId].[gltf/glb]
//If no nodeName/nodeId is given, all primitives active in default scene will be added as surfaces.
void idRenderModelGLTF::InitFromFile( const char* fileName )
{
	int meshID = -1;
	idStr meshName;
	idStr gltfFileName = idStr( fileName );

	gltfManager::ExtractMeshIdentifier( gltfFileName, meshID, meshName );

	if( gltfParser->currentFile.Length() )
	{
		if( gltfParser->currentAsset && gltfParser->currentFile != gltfFileName )
		{
			common->FatalError( "multiple GLTF file loading not supported" );
		}
	}
	else
	{
		gltfParser->Load( gltfFileName );
	}

	timeStamp = fileSystem->GetTimestamp( gltfFileName );
	gltfData* data = gltfParser->currentAsset;

	bounds.Clear();

	int sceneId = data->DefaultScene();

	assert( sceneId >= 0 );

	if( !meshName[0] )
	{
		auto& nodeList = data->NodeList();
		for( auto& nodeID :  data->SceneList()[sceneId]->nodes )
		{
			gltfNode* modelNode = nodeList[nodeID];
			assert( modelNode );
			ProcessNode( modelNode, mat4_identity, data );
		}
	}
	else
	{
		gltfNode* modelNode = data->GetNode( gltf_ModelSceneName.GetString(), meshName );
		if( modelNode )
		{
			ProcessNode( modelNode, mat4_identity, data );
		}
	}

	if( surfaces.Num( ) <= 0 )
	{
		common->Warning( "Couldn't load model: '%s'", name.c_str( ) );
		MakeDefaultModel( );
		return;
	}

	// it is now available for use
	purged = false;

	//skin
	//gltfNode * modelNode = data->GetNode(data->SceneList()[data->GetSceneId("models")],targetMesh);
	//__debugbreak();
}

bool idRenderModelGLTF::LoadBinaryModel( idFile* file, const ID_TIME_T sourceTimeStamp )
{
	common->Warning( "The method or operation is not implemented." );
	return false;
}

void idRenderModelGLTF::WriteBinaryModel( idFile* file, ID_TIME_T* _timeStamp /*= NULL */ ) const
{
	common->Warning( "idRenderModelGLTF::WriteBinaryModel is not implemented." );
}

void idRenderModelGLTF::PurgeModel()
{
	common->Warning( "idRenderModelGLTF::PurgeModel is not implemented." );
}

void idRenderModelGLTF::LoadModel()
{
	common->Warning( "The method or operation is not implemented." );
}

void idRenderModelGLTF::TouchData()
{
	common->Warning( "The method or operation is not implemented." );
}

/*
void idRenderModelGLTF::CreateBuffers()
{
	common->Warning( "The method or operation is not implemented." );
}
*/

void idRenderModelGLTF::Print() const
{
	common->Warning( "The method or operation is not implemented." );
}

void idRenderModelGLTF::List() const
{
	common->Warning( "The method or operation is not implemented." );
}

int idRenderModelGLTF::Memory() const
{
	common->Warning( "The method or operation is not implemented." );
	return -1;
}

dynamicModel_t idRenderModelGLTF::IsDynamicModel() const
{
	return DM_STATIC;
}

idRenderModel* idRenderModelGLTF::InstantiateDynamicModel( const struct renderEntity_s* ent, const viewDef_t* view, idRenderModel* cachedModel )
{
	common->Warning( "The method or operation is not implemented." );
	return nullptr;
}

int idRenderModelGLTF::NumJoints() const
{
	common->Warning( "The method or operation is not implemented." );
	return 0;
}

const idMD5Joint* idRenderModelGLTF::GetJoints() const
{
	common->Warning( "The method or operation is not implemented." );
	return nullptr;
}

jointHandle_t idRenderModelGLTF::GetJointHandle( const char* name ) const
{
	common->Warning( "The method or operation is not implemented." );
	return jointHandle_t();
}

const char* idRenderModelGLTF::GetJointName( jointHandle_t handle ) const
{
	common->Warning( "The method or operation is not implemented." );
	return "";
}

const idJointQuat* idRenderModelGLTF::GetDefaultPose() const
{
	common->Warning( "The method or operation is not implemented." );
	return nullptr;
}

int idRenderModelGLTF::NearestJoint( int surfaceNum, int a, int b, int c ) const
{
	common->Warning( "The method or operation is not implemented." );
	return -1;
}

idBounds idRenderModelGLTF::Bounds( const struct renderEntity_s* ent ) const
{
	return bounds;
}