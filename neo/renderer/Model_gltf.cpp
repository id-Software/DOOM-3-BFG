
#include "precompiled.h"
#pragma hdrstop


#include "Model_gltf.h"
#include "Model_local.h"



bool idRenderModelStatic::ConvertGltfMeshToModelsurfaces( const gltfMesh* mesh )
{

	//for ( auto mesh : currentAsset->MeshList( ) ) {
	//	for ( auto prim : mesh->primitives ) {

	return false;
}

void MapPolygonMesh::ConvertFromMeshGltf( const gltfMesh* _mesh, gltfData* data )
{
	for( auto* gltfMesh : data->MeshList( ) )
	{
		for( auto prim : gltfMesh->primitives )
		{
			common->Printf( "primitive for %s\n", gltfMesh->name.c_str() );

			gltfAccessor* accessor = data->AccessorList( )[prim->indices];
			gltfBufferView* bv = data->BufferViewList( )[accessor->bufferView];
			gltfData* data = bv->parent;

			gltfBuffer* buff = data->BufferList( )[bv->buffer];
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
				MapPolygon& polygon = polygons.Alloc( );
				polygon.SetMaterial( "textures/base_wall/lfwall27d" );
				polygon.AddIndex( indices[i + 1] );
				polygon.AddIndex( indices[i + 2] );
				polygon.AddIndex( indices[i + 0] );
			}

			Mem_Free( indices );
			bool sizeSet = false;

			for( auto& attrib : prim->attributes )
			{
				gltfAccessor* attrAcc = data->AccessorList( )[attrib->accessorIndex];
				gltfBufferView* attrBv = data->BufferViewList( )[attrAcc->bufferView];
				gltfData* attrData = attrBv->parent;
				gltfBuffer* attrbuff = attrData->BufferList( )[attrBv->buffer];

				idFile_Memory bin = idFile_Memory( "gltfChunkVertices",
												   ( const char* )( ( attrData->GetData( attrBv->buffer ) + attrBv->byteOffset + attrAcc->byteOffset ) ), attrBv->byteLength );

				if( !sizeSet )
				{
					verts.AssureSize( attrAcc->count );
					sizeSet = true;
				}

				switch( attrib->type )
				{
					case gltfMesh_Primitive_Attribute::Type::Position:
					{
						for( int i =  attrAcc->count - 1; i >= 0; i-- )
						{
							idVec3 pos;

							bin.Read( ( void* )( &pos.x ), attrAcc->typeSize );
							bin.Read( ( void* )( &pos.y ), attrAcc->typeSize );
							bin.Read( ( void* )( &pos.z ), attrAcc->typeSize );

							verts[i].xyz = pos;

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
							verts[i].SetNormal( vec );
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
							verts[i].SetTexCoord( vec );
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
							verts[i].SetTangent( vec.ToVec3() );
							verts[i].SetBiTangentSign( vec.w );
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
		}
	}
	SetContents();
}

int idMapEntity::GetEntities( gltfData* data, EntityListRef entities, int sceneID )
{
	idMapEntity* worldspawn = new idMapEntity();
	entities.Append( worldspawn );

	int entityCount = 0;
	for( auto& nodeID :  data->SceneList()[sceneID]->nodes )
	{
		auto* node = data->NodeList()[nodeID];

		idMapEntity* newEntity = NULL;

		bool isWorldSpawn = idStr::Icmp( node->extras.strPairs.GetString( "classname" ), "worldspawn" ) == 0;
		if( isWorldSpawn )
		{
			worldspawn->primitives.Resize( 1024, 256 );
			worldspawn->epairs.Copy( node->extras.strPairs );
		}
		else
		{
			// account all meshes starting with worldspawn. or BSP in the name
			if( idStr::Icmpn( node->name, "BSP", 3 ) == 0 || idStr::Icmpn( node->name, "worldspawn.", 11 ) == 0 )
			{
				MapPolygonMesh* meshPrim = new MapPolygonMesh();
				//meshPrim->epairs.Copy( newEntity->epairs );

				meshPrim->ConvertFromMeshGltf( data->MeshList()[node->mesh], data );
				worldspawn->AddPrimitive( meshPrim );
			}
			else
			{
				newEntity = new idMapEntity();

				// set name and retrieve epairs from node extras
				if( node->name.Length() )
				{
					newEntity->epairs.Set( "name", node->name );
				}
				newEntity->epairs.Copy( node->extras.strPairs );

				for( int i = 0; i < newEntity->epairs.GetNumKeyVals(); i++ )
				{
					const idKeyValue* kv = newEntity->epairs.GetKeyVal( i );

					idLib::Printf( "entity[ %s ] key = '%s' value = '%s'\n", node->name.c_str(), kv->GetKey().c_str(), kv->GetValue().c_str() );
				}

				data->ResolveNodeMatrix( node );
				newEntity->epairs.Set( "origin", node->translation.ToString() );

				common->Printf( " %s \n ", node->name.c_str( ) );
				entities.Append( newEntity );

				entityCount++;
			}
		}
	}

	/*
	if( entities.Num( ) > 0 && ( idStr::Icmp( entities[0]->epairs.GetString( "name" ), "worldspawn" ) != 0 ) )
	{
		// move world spawn to first place
		for( int i = 1; i < entities.Num( ); i++ )
		{
			if( idStr::Icmp( entities[i]->epairs.GetString( "name" ), "worldspawn" ) == 0 )
			{
				idMapEntity* tmp = entities[0];
				entities[0] = entities[i];
				entities[i] = tmp;
				break;
			}
		}
	}
	*/

	return entityCount;
}


// [filename].[%i|%s].[gltf/glb]
bool gltfManager::ExtractMeshIdentifier( idStr& filename, int& meshId, idStr& meshName )
{

	idStr extension;
	filename.ExtractFileExtension( extension );

	idStr idPart = 	filename.Left( filename.Length() - extension.Length() - 1 );
	idStr id;
	idPart.ExtractFileExtension( id );

	if( !id.Length() )
	{
		idLib::Warning( "no gltf mesh identifier" );
		return false;
	}

	filename = idPart.Left( idPart.Length() - id.Length() ) + extension;

	idLexer lexer( LEXFL_ALLOWMULTICHARLITERALS | LEXFL_NOSTRINGESCAPECHARS );
	lexer.LoadMemory( id.c_str( ), id.Size( ), "GltfmeshID", 0 );

	idToken token;
	if( lexer.ExpectAnyToken( &token ) )
	{
		if( lexer.EndOfFile() && ( token.type == TT_NUMBER ) && ( token.subtype & TT_INTEGER ) )
		{
			meshId = token.GetIntValue();
		}
		else if( token.type == TT_NUMBER || token.type == TT_STRING )
		{
			meshName = id;
		}
		else
		{
			lexer.Warning( "malformed gltf mesh identifier" );
			return false;
		}
		return true;
	}
	else
	{
		lexer.Warning( "malformed gltf mesh identifier" );
	}

	return false;
}

void idRenderModelGLTF::InitFromFile( const char* fileName )
{
	common->Warning( "The method or operation is not implemented." );

}

bool idRenderModelGLTF::LoadBinaryModel( idFile* file, const ID_TIME_T sourceTimeStamp )
{
	common->Warning( "The method or operation is not implemented." );
	return false;
}

void idRenderModelGLTF::WriteBinaryModel( idFile* file, ID_TIME_T* _timeStamp /*= NULL */ ) const
{
	common->Warning( "The method or operation is not implemented." );
}

bool idRenderModelGLTF::SupportsBinaryModel( )
{
	common->Warning( "The method or operation is not implemented." );
	return false;
}

void idRenderModelGLTF::ExportOBJ( idFile* objFile, idFile* mtlFile, ID_TIME_T* _timeStamp /*= NULL */ )
{
	common->Warning( "The method or operation is not implemented." );
}

void idRenderModelGLTF::PartialInitFromFile( const char* fileName )
{
	common->Warning( "The method or operation is not implemented." );
}

void idRenderModelGLTF::PurgeModel( )
{
	common->Warning( "The method or operation is not implemented." );
}

void idRenderModelGLTF::Reset( )
{
	common->Warning( "The method or operation is not implemented." );
}

void idRenderModelGLTF::LoadModel( )
{
	common->Warning( "The method or operation is not implemented." );
}

bool idRenderModelGLTF::IsLoaded( )
{
	common->Warning( "The method or operation is not implemented." );
	return false;
}

void idRenderModelGLTF::SetLevelLoadReferenced( bool referenced )
{
	common->Warning( "The method or operation is not implemented." );
}

bool idRenderModelGLTF::IsLevelLoadReferenced( )
{
	common->Warning( "The method or operation is not implemented." );
	return false;
}

void idRenderModelGLTF::TouchData( )
{
	common->Warning( "The method or operation is not implemented." );
}

void idRenderModelGLTF::CreateBuffers( nvrhi::ICommandList* commandList )
{
	common->Warning( "The method or operation is not implemented." );
}

void idRenderModelGLTF::InitEmpty( const char* name )
{
	common->Warning( "The method or operation is not implemented." );
}

void idRenderModelGLTF::AddSurface( modelSurface_t surface )
{
	common->Warning( "The method or operation is not implemented." );
}

void idRenderModelGLTF::FinishSurfaces( bool useMikktspace )
{
	common->Warning( "The method or operation is not implemented." );
}

void idRenderModelGLTF::FreeVertexCache( )
{
	common->Warning( "The method or operation is not implemented." );
}

const char* idRenderModelGLTF::Name( ) const
{
	common->Warning( "The method or operation is not implemented." );
	return "";
}

void idRenderModelGLTF::Print( ) const
{
	common->Warning( "The method or operation is not implemented." );
}

void idRenderModelGLTF::List( ) const
{
	common->Warning( "The method or operation is not implemented." );
}

int idRenderModelGLTF::Memory( ) const
{
	common->Warning( "The method or operation is not implemented." );
	return -1;
}

ID_TIME_T idRenderModelGLTF::Timestamp( ) const
{
	common->Warning( "The method or operation is not implemented." );
	return FILE_NOT_FOUND_TIMESTAMP;
}

int idRenderModelGLTF::NumSurfaces( ) const
{
	common->Warning( "The method or operation is not implemented." );
	return -1;
}

int idRenderModelGLTF::NumBaseSurfaces( ) const
{
	common->Warning( "The method or operation is not implemented." );
	return -1;
}

const modelSurface_t* idRenderModelGLTF::Surface( int surfaceNum ) const
{
	common->Warning( "The method or operation is not implemented." );
	return nullptr;
}

srfTriangles_t* idRenderModelGLTF::AllocSurfaceTriangles( int numVerts, int numIndexes ) const
{
	common->Warning( "The method or operation is not implemented." );
	return nullptr;
}

void idRenderModelGLTF::FreeSurfaceTriangles( srfTriangles_t* tris ) const
{
	common->Warning( "The method or operation is not implemented." );
}

bool idRenderModelGLTF::IsStaticWorldModel( ) const
{
	common->Warning( "The method or operation is not implemented." );
	return false;
}

dynamicModel_t idRenderModelGLTF::IsDynamicModel( ) const
{
	common->Warning( "The method or operation is not implemented." );
	return dynamicModel_t();
}

bool idRenderModelGLTF::IsDefaultModel( ) const
{
	common->Warning( "The method or operation is not implemented." );
	return false;
}

bool idRenderModelGLTF::IsReloadable( ) const
{
	common->Warning( "The method or operation is not implemented." );
	return false;
}

idRenderModel* idRenderModelGLTF::InstantiateDynamicModel( const struct renderEntity_s* ent, const viewDef_t* view, idRenderModel* cachedModel )
{
	common->Warning( "The method or operation is not implemented." );
	return nullptr;
}

int idRenderModelGLTF::NumJoints( ) const
{
	common->Warning( "The method or operation is not implemented." );
	return 0;
}

const idMD5Joint* idRenderModelGLTF::GetJoints( ) const
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

const idJointQuat* idRenderModelGLTF::GetDefaultPose( ) const
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
	common->Warning( "The method or operation is not implemented." );
	return idBounds();
}

void idRenderModelGLTF::ReadFromDemoFile( class idDemoFile* f )
{
	common->Warning( "The method or operation is not implemented." );
}

void idRenderModelGLTF::WriteToDemoFile( class idDemoFile* f )
{
	common->Warning( "The method or operation is not implemented." );
}

float idRenderModelGLTF::DepthHack( ) const
{
	common->Warning( "The method or operation is not implemented." );
	return -1.0f;
}

bool idRenderModelGLTF::ModelHasDrawingSurfaces( ) const
{
	common->Warning( "The method or operation is not implemented." );
	return false;
}

bool idRenderModelGLTF::ModelHasInteractingSurfaces( ) const
{
	common->Warning( "The method or operation is not implemented." );
	return false;
}

bool idRenderModelGLTF::ModelHasShadowCastingSurfaces( ) const
{
	common->Warning( "The method or operation is not implemented." );
	return false;
}
