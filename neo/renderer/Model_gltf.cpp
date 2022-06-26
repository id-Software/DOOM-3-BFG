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


static const byte GLMB_VERSION = 100;
static const unsigned int GLMB_MAGIC = ( 'M' << 24 ) | ( 'L' << 16 ) | ( 'G' << 8 ) | GLMB_VERSION;
static const char* GLTF_SnapshotName = "_GLTF_Snapshot_";

bool idRenderModelStatic::ConvertGltfMeshToModelsurfaces( const gltfMesh* mesh )
{
	return false;
}

void idRenderModelGLTF::ProcessNode( gltfNode* modelNode, idMat4 trans, gltfData* data )
{
	auto& meshList = data->MeshList( );
	auto& nodeList = data->NodeList( );

	//find all animations
	for( auto anim : data->AnimationList( ) )
	{
		for( auto channel : anim->channels )
		{
			if( channel->target.node >= 0 && nodeList[channel->target.node] == modelNode )
			{
				animIds.Alloc( ) = channel->target.node;
			}
		}
	}

	gltfData::ResolveNodeMatrix( modelNode );

	idMat4 curTrans = trans * modelNode->matrix;

	if( modelNode->mesh >= 0 )
	{
		gltfMesh* targetMesh = meshList[modelNode->mesh];

		idMat4 newTrans  = mat4_identity;

		if( !animIds.Num() )
		{
			newTrans = curTrans;
		}

		for( auto prim : targetMesh->primitives )
		{
			//ConvertFromMeshGltf should only be used for the map, ConvertGltfMeshToModelsurfaces should be used.
			auto* newMesh = MapPolygonMesh::ConvertFromMeshGltf( prim, data, newTrans );
			modelSurface_t	surf;

			gltfMaterial* mat = NULL;
			if( prim->material != -1 )
			{
				mat = data->MaterialList( )[prim->material];
			}
			if( mat != NULL && !gltf_ForceBspMeshTexture.GetBool( ) )
			{
				surf.shader = declManager->FindMaterial( mat->name );
			}
			else
			{
				surf.shader = declManager->FindMaterial( "textures/base_wall/snpanel2rust" );
			}
			surf.id = this->NumSurfaces( );

			srfTriangles_t* tri = R_AllocStaticTriSurf( );
			tri->numIndexes = newMesh->GetNumPolygons( ) * 3;
			tri->numVerts = newMesh->GetNumVertices( );

			R_AllocStaticTriSurfIndexes( tri, tri->numIndexes );
			R_AllocStaticTriSurfVerts( tri, tri->numVerts );

			int indx = 0;
			for( int i = 0; i < newMesh->GetNumPolygons( ); i++ )
			{
				auto& face = newMesh->GetFace( i );
				auto& faceIdxs = face.GetIndexes( );
				tri->indexes[indx] = faceIdxs[0];
				tri->indexes[indx + 1] = faceIdxs[1];
				tri->indexes[indx + 2] = faceIdxs[2];
				indx += 3;
			}

			for( int i = 0; i < tri->numVerts; ++i )
			{
				tri->verts[i] = newMesh->GetDrawVerts( )[i];
				tri->bounds.AddPoint( tri->verts[i].xyz );
			}

			bounds.AddBounds( tri->bounds );

			surf.geometry = tri;
			AddSurface( surf );
		}
	}

	for( auto& child : modelNode->children )
	{
		ProcessNode( nodeList[child], curTrans, data );
	}
}

void idRenderModelGLTF::MakeMD5Mesh( )
{

	meshes.SetGranularity( 1 );
	//meshes.SetNum( num );
}

//constructs a renderModel from a gltfScene node found in the "models" scene of the given gltfFile.
// override with gltf_ModelSceneName
// warning : nodeName cannot have dots!
//[fileName].[nodeName/nodeId].[gltf/glb]
//If no nodeName/nodeId is given, all primitives active in default scene will be added as surfaces.
void idRenderModelGLTF::InitFromFile( const char* fileName )
{
	hasAnimations = false;
	fileExclusive = false;
	root = nullptr;
	int meshID = -1;
	idStr meshName;
	idStr gltfFileName = idStr( fileName );
	model_state = DM_STATIC;

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
	data = gltfParser->currentAsset;

	bounds.Clear();

	int sceneId = data->DefaultScene();

	assert( sceneId >= 0 );

	if( !meshName[0] )
	{
		rootID = -1;

		//this needs to be fixed to correctly support multiple meshes.
		// atm this will only correctlty with static models.
		// we could do gltfMeshes a la md5
		// or re-use this class
		auto& nodeList = data->NodeList();
		for( auto& nodeID :  data->SceneList()[sceneId]->nodes )
		{
			gltfNode* modelNode = nodeList[nodeID];
			if( modelNode )
			{
				ProcessNode( modelNode, mat4_identity, data );

				if( rootID == -1 )
				{
					root = modelNode;
					rootID = nodeID;
				}
			}
		}
		fileExclusive = true;
	}
	else
	{
		gltfNode* modelNode = data->GetNode( gltf_ModelSceneName.GetString(), meshName, &rootID );
		if( modelNode )
		{
			root = modelNode;
			ProcessNode( modelNode, mat4_identity, data );
		}

	}

	if( surfaces.Num( ) <= 0 )
	{
		common->Warning( "Couldn't load model: '%s'", name.c_str( ) );
		MakeDefaultModel( );
		return;
	}
	model_state = animIds.Num() ? DM_CONTINUOUS : DM_STATIC;

	// derive mikktspace tangents from normals
	FinishSurfaces( true );

	// it is now available for use
	purged = false;

	//skin
	//gltfNode * modelNode = data->GetNode(data->SceneList()[data->GetSceneId("models")],targetMesh);
	//__debugbreak();
}

bool idRenderModelGLTF::LoadBinaryModel( idFile* file, const ID_TIME_T sourceTimeStamp )
{
	hasAnimations = false;
	fileExclusive = false; // not written.
	root = nullptr;
	prevTrans = mat4_identity;

	// we should still load the scene information ?
	if( !idRenderModelStatic::LoadBinaryModel( file, sourceTimeStamp ) )
	{
		return false;
	}

	unsigned int magic = 0;
	file->ReadBig( magic );

	if( magic != GLMB_MAGIC )
	{
		return false;
	}

	file->ReadBig( model_state );
	file->ReadBig( rootID );

	// TODO get rid of loading the original .glb here
#if 1
	idStr dataFilename;
	file->ReadString( dataFilename );

	if( gltfParser->currentFile.Length( ) )
	{
		if( gltfParser->currentAsset && gltfParser->currentFile != dataFilename )
		{
			common->FatalError( "multiple GLTF file loading not supported" );
		}
	}
	else
	{
		gltfParser->Load( dataFilename );
	}

	data = gltfParser->currentAsset;
	root = data->GetNode( gltf_ModelSceneName.GetString(), rootID );
	assert( root );
#endif

	int animCnt;
	file->ReadBig( animCnt );
	if( animCnt > 0 )
	{
		animIds.Resize( animCnt, 1 );
		file->ReadBigArray( animIds.Ptr( ), animCnt );
		animIds.SetNum( animCnt );
	}
	hasAnimations = animCnt > 0;
	return true;
}

void idRenderModelGLTF::WriteBinaryModel( idFile* file, ID_TIME_T* _timeStamp /*= NULL */ ) const
{
	idRenderModelStatic::WriteBinaryModel( file );

	if( file == NULL )
	{
		return;
	}

	file->WriteBig( GLMB_MAGIC );
	file->WriteBig( model_state );
	file->WriteBig( rootID );
	file->WriteString( data->FileName() );

	file->WriteBig( animIds.Num( ) );
	if( animIds.Num( ) )
	{
		file->WriteBigArray( animIds.Ptr(), animIds.Size() );
	}
	//check if this model has a skeleton
	//if ( root->skin >= 0 )
	//{
	//	gltfSkin *skin = data->SkinList( )[root->skin];
	//	auto &nodeList = data->NodeList( );

	//	file->WriteBig( skin->joints.Num( ) );
	//	for ( int i = 0; i < skin->joints.Num( ); i++ ) {

	//		gltfNode &target = *nodeList[skin->joints[i]];
	//		file->WriteString( target.name );
	//		int offset = -1;
	//		if ( target.parent != NULL ) {
	//			offset = target.parent - skin->joints.Ptr( );
	//		}
	//		file->WriteBig( offset );
	//	}
	//}
	//	file->WriteBig( defaultPose.Num( ) );
	//	for ( int i = 0; i < defaultPose.Num( ); i++ ) {
	//		file->WriteBig( defaultPose[i].q.x );
	//		file->WriteBig( defaultPose[i].q.y );
	//		file->WriteBig( defaultPose[i].q.z );
	//		file->WriteBig( defaultPose[i].q.w );
	//		file->WriteVec3( defaultPose[i].t );
	//	}

	//	file->WriteBig( invertedDefaultPose.Num( ) );
	//	for ( int i = 0; i < invertedDefaultPose.Num( ); i++ ) {
	//		file->WriteBigArray( invertedDefaultPose[i].ToFloatPtr( ), JOINTMAT_TYPESIZE );
	//	}
	//}



	//file->WriteBig( meshes.Num( ) );
	//for ( int i = 0; i < meshes.Num( ); i++ ) {

	//	if ( meshes[i].shader != NULL && meshes[i].shader->GetName( ) != NULL ) {
	//		file->WriteString( meshes[i].shader->GetName( ) );
	//	} else {
	//		file->WriteString( "" );
	//	}

	//	file->WriteBig( meshes[i].numVerts );
	//	file->WriteBig( meshes[i].numTris );

	//	file->WriteBig( meshes[i].numMeshJoints );
	//	file->WriteBigArray( meshes[i].meshJoints, meshes[i].numMeshJoints );
	//	file->WriteBig( meshes[i].maxJointVertDist );

	//	deformInfo_t &deform = *meshes[i].deformInfo;

	//	file->WriteBig( deform.numSourceVerts );
	//	file->WriteBig( deform.numOutputVerts );
	//	file->WriteBig( deform.numIndexes );
	//	file->WriteBig( deform.numMirroredVerts );
	//	file->WriteBig( deform.numDupVerts );
	//	file->WriteBig( deform.numSilEdges );

	//	if ( deform.numOutputVerts > 0 ) {
	//		file->WriteBigArray( deform.verts, deform.numOutputVerts );
	//	}

	//	if ( deform.numIndexes > 0 ) {
	//		file->WriteBigArray( deform.indexes, deform.numIndexes );
	//		file->WriteBigArray( deform.silIndexes, deform.numIndexes );
	//	}

	//	if ( deform.numMirroredVerts > 0 ) {
	//		file->WriteBigArray( deform.mirroredVerts, deform.numMirroredVerts );
	//	}

	//	if ( deform.numDupVerts > 0 ) {
	//		file->WriteBigArray( deform.dupVerts, deform.numDupVerts * 2 );
	//	}

	//	if ( deform.numSilEdges > 0 ) {
	//		for ( int j = 0; j < deform.numSilEdges; j++ ) {
	//			file->WriteBig( deform.silEdges[j].p1 );
	//			file->WriteBig( deform.silEdges[j].p2 );
	//			file->WriteBig( deform.silEdges[j].v1 );
	//			file->WriteBig( deform.silEdges[j].v2 );
	//		}
	//	}

	//	file->WriteBig( meshes[i].surfaceNum );
	//}
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
	return model_state;
}

void TransformVertsAndTangents( idDrawVert* targetVerts, const int numVerts, idMat4 trans )
{
	for( int i = 0; i < numVerts; i++ )
	{

		targetVerts[i].xyz *= trans;// * idVec3( base.xyz.x, base.xyz.y, base.xyz.z);
		targetVerts[i].SetNormal( trans.ToMat3() * targetVerts[i].GetNormal( ) );
		targetVerts[i].SetTangent( trans.ToMat3() * targetVerts[i].GetTangent( ) );
	}
}

void idRenderModelGLTF::UpdateSurface( const struct renderEntity_s* ent, idMat4 trans, modelSurface_t* surf )
{

	if( surf->geometry != NULL )
	{
		R_FreeStaticTriSurfVertexCaches( surf->geometry );
	}
	else
	{
		surf->geometry = R_AllocStaticTriSurf( );
	}

	srfTriangles_t* tri = surf->geometry;
	TransformVertsAndTangents( tri->verts, tri->numVerts, trans );

}

idRenderModel* idRenderModelGLTF::InstantiateDynamicModel( const struct renderEntity_s* ent, const viewDef_t* view, idRenderModel* cachedModel )
{
	if( cachedModel != NULL && !r_useCachedDynamicModels.GetBool() )
	{
		delete cachedModel;
		cachedModel = NULL;
	}

	if( purged )
	{
		common->DWarning( "model %s instantiated while purged", Name() );
		LoadModel();
	}

	idRenderModelStatic* staticModel;
	if( cachedModel != NULL )
	{
		assert( dynamic_cast< idRenderModelStatic* >( cachedModel ) != NULL );
		assert( idStr::Icmp( cachedModel->Name( ), GLTF_SnapshotName ) == 0 );
		staticModel = static_cast< idRenderModelStatic* >( cachedModel );
	}
	else
	{
		staticModel = new( TAG_MODEL ) idRenderModelStatic;
		staticModel->InitEmpty( GLTF_SnapshotName );
	}

	idStr prevName = name;
	name = GLTF_SnapshotName;
	*staticModel = *this;
	name = prevName;

	idMat3 rotation = idAngles( 0.0f, 0.0f, 90.0f ).ToMat3( );
	idMat4 axisTransform( rotation, vec3_origin );
	idMat4 trans = prevTrans;
	bool wasDirty = root->dirty;

	gltfData::ResolveNodeMatrix( root, &trans );

	if( wasDirty )
	{
		trans *= axisTransform;
	}

	for( modelSurface_t& surf : staticModel->surfaces )
	{
		UpdateSurface( ent, trans, &surf );
	}

	prevTrans = root->matrix.Inverse();

	//staticModel->bounds *= trans.ToMat3( );
	//bounds.Translate( idVec3( trans[0][3], trans[1][3], trans[2][3] ) );

	return staticModel;
}

int idRenderModelGLTF::NumJoints() const
{
	if( !root )
	{
		common->FatalError( "Trying to determine Joint count without a model node loaded" );
	}

	if( root->skin >= 0 )
	{
		gltfSkin* skin = data->SkinList( )[root->skin];
		return skin->joints.Num();
	}

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

idGltfMesh::idGltfMesh( gltfMesh* _mesh, gltfData* _data ) : mesh( _mesh ), data( _data )
{

}
