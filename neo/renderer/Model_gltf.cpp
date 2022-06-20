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

	// derive mikktspace tangents from normals
	FinishSurfaces( true );
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