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


MapPolygonMesh* MapPolygonMesh::ConvertFromMeshGltf( const gltfMesh_Primitive* prim, gltfData* _data , idMat4 trans )
{
	MapPolygonMesh* mesh = new MapPolygonMesh();
	gltfAccessor* accessor = _data->AccessorList()[prim->indices];
	gltfBufferView* bv = _data->BufferViewList()[accessor->bufferView];
	gltfData* data = bv->parent;

	// files import as y-up. Use this transform to change the model to z-up.
	idMat3 rotation = idAngles( 0.0f, 0.0f, 90.0f ).ToMat3();
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

	int polyCount = accessor->count / 3;

	mesh->polygons.AssureSize( polyCount );
	mesh->polygons.SetNum( polyCount );

	int cnt = 0;
	for( int i = 0; i < accessor->count; i += 3 )
	{
		MapPolygon& polygon = mesh->polygons[cnt++];

		if( mat != NULL )
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

	assert( cnt == polyCount );

	Mem_Free( indices );
	bool sizeSet = false;

	//for( const auto& attrib : prim->attributes )
	for( int a = 0; a < prim->attributes.Num(); a++ )
	{
		gltfMesh_Primitive_Attribute* attrib = prim->attributes[a];

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

					pos *= trans;
					pos *= axisTransform;

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
			case gltfMesh_Primitive_Attribute::Type::Weight:
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

					mesh->verts[i].SetColor2( PackColor( vec ) );

				}
				break;
			}
			case gltfMesh_Primitive_Attribute::Type::Indices:
			{
				if( attrAcc->typeSize == 2 )
				{
					uint16_t vec[4];

					assert( sizeof( vec ) == ( attrAcc->typeSize * 4 ) );

					for( int i = 0; i < attrAcc->count; i++ )
					{
						bin.Read( ( void* )( &vec[0] ), attrAcc->typeSize );
						bin.Read( ( void* )( &vec[1] ), attrAcc->typeSize );
						bin.Read( ( void* )( &vec[2] ), attrAcc->typeSize );
						bin.Read( ( void* )( &vec[3] ), attrAcc->typeSize );
						if( attrBv->byteStride )
						{
							bin.Seek( attrBv->byteStride - ( attrib->elementSize * attrAcc->typeSize ), FS_SEEK_CUR );
						}

						mesh->verts[i].color[0] = vec[0];
						mesh->verts[i].color[1] = vec[1];
						mesh->verts[i].color[2] = vec[2];
						mesh->verts[i].color[3] = vec[3];
					}
				}
				else
				{
					uint8_t vec[4];
					for( int i = 0; i < attrAcc->count; i++ )
					{
						assert( sizeof( vec ) == ( attrAcc->typeSize * 4 ) );

						bin.Read( ( void* )( &vec[0] ), attrAcc->typeSize );
						bin.Read( ( void* )( &vec[1] ), attrAcc->typeSize );
						bin.Read( ( void* )( &vec[2] ), attrAcc->typeSize );
						bin.Read( ( void* )( &vec[3] ), attrAcc->typeSize );
						if( attrBv->byteStride )
						{
							bin.Seek( attrBv->byteStride - ( attrib->elementSize * attrAcc->typeSize ), FS_SEEK_CUR );
						}

						mesh->verts[i].color[0] = vec[0];
						mesh->verts[i].color[1] = vec[1];
						mesh->verts[i].color[2] = vec[2];
						mesh->verts[i].color[3] = vec[3];
					}
				}
				break;

			}
		}

	}
	mesh->SetContents();
	return mesh;
}

void ProcessSceneNode( idMapEntity* newEntity, gltfNode* node, idMat4 trans, gltfData* data , bool staticMesh = false )
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

	if( node->mesh != -1 )
	{
		for( auto* prim : data->MeshList()[node->mesh]->primitives )
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
	idMat3 rotation = idAngles( 0.0f, 0.0f, 90.0f ).ToMat3();
	idMat4 axisTransform( rotation, vec3_origin );

	origin *= axisTransform;

	newEntity->epairs.Set( "origin", origin.ToString() );
}

void Map_AddMeshes( idMapEntity* _Entity, gltfNode* _Node, idMat4& _Trans, gltfData* _Data )
{
	gltfData::ResolveNodeMatrix( _Node );
	idMat4 curTrans = _Trans * _Node->matrix;

	if( _Node->mesh != -1 )
	{
		for( auto prim : _Data->MeshList()[_Node->mesh]->primitives )
		{
			_Entity->AddPrimitive( MapPolygonMesh::ConvertFromMeshGltf( prim, _Data, curTrans ) );
		}
	}

	for( auto& child : _Node->children )
	{
		Map_AddMeshes( _Entity, _Data->NodeList()[child], curTrans, _Data );
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
