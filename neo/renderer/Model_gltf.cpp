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

//HVG_TODO: this has to be moved out before release
#include "d3xp/anim/Anim.h"
#include "d3xp/Game_local.h"

idCVar gltf_ForceBspMeshTexture( "gltf_ForceBspMeshTexture", "0", CVAR_SYSTEM | CVAR_BOOL, "all world geometry has the same forced texture" );
idCVar gltf_ModelSceneName( "gltf_ModelSceneName", "models", CVAR_SYSTEM , "Scene to use when loading specific models" );

idCVar gltf_AnimSampleRate( "gltf_AnimSampleRate", "24", CVAR_SYSTEM | CVAR_INTEGER , "The frame rate of the converted md5anim" );


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

	gltfData::ResolveNodeMatrix( modelNode );

	idMat4 curTrans = trans * modelNode->matrix;

	if( modelNode->mesh >= 0 )
	{
		gltfMesh* targetMesh = meshList[modelNode->mesh];

		idMat4 newTrans;

		if( !animIds.Num() )
		{
			newTrans = curTrans;
		}
		else
		{
			newTrans = modelNode->matrix;
		}

		for( auto prim : targetMesh->primitives )
		{
			//ConvertFromMeshGltf should only be used for the map, ConvertGltfMeshToModelsurfaces should be used.
			auto* mesh = MapPolygonMesh::ConvertFromMeshGltf( prim, data, newTrans );
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
			tri->numIndexes = mesh->GetNumPolygons( ) * 3;
			tri->numVerts = mesh->GetNumVertices( );

			R_AllocStaticTriSurfIndexes( tri, tri->numIndexes );
			R_AllocStaticTriSurfVerts( tri, tri->numVerts );

			int indx = 0;
			for( int i = 0; i < mesh->GetNumPolygons( ); i++ )
			{
				auto& face = mesh->GetFace( i );
				auto& faceIdxs = face.GetIndexes( );
				tri->indexes[indx] = faceIdxs[0];
				tri->indexes[indx + 1] = faceIdxs[1];
				tri->indexes[indx + 2] = faceIdxs[2];
				indx += 3;
			}

			for( int i = 0; i < tri->numVerts; ++i )
			{
				tri->verts[i] = mesh->GetDrawVerts( )[i];
				tri->bounds.AddPoint( tri->verts[i].xyz );
			}

			bounds.AddBounds( tri->bounds );

			surf.geometry = tri;
			AddSurface( surf );
			delete mesh;
		}
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
	hasAnimations = false;
	fileExclusive = false;
	root = nullptr;
	rootID = -1;
	int meshID = -1;
	name = fileName;

	//FIXME FIXME FIXME
	maxJointVertDist = 60;
	idStr meshName;
	idStr gltfFileName = idStr( fileName );
	model_state = DM_STATIC;

	gltfManager::ExtractIdentifier( gltfFileName, meshID, meshName );

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
		//this needs to be fixed to correctly support multiple meshes.
		// atm this will only correctlty with static models.
		// we could do gltfMeshes a la md5
		// or re-use this class
		auto& nodeList = data->NodeList();
		for( auto& nodeID :  data->SceneList()[sceneId]->nodes )
		{
			gltfNode* modelNode = nodeList[nodeID];
			assert( modelNode );
			ProcessNode( modelNode, mat4_identity, data );
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

	//find all animations
	animIds = data->GetAnimationIds( root );

	hasAnimations = animIds.Num() > 0;
	model_state = hasAnimations ? DM_CONTINUOUS : DM_STATIC;

	//check if this model has a skeleton
	if( root->skin >= 0 )
	{

		gltfSkin* skin = data->SkinList( )[root->skin];
		bones = skin->joints;
	}
	else if( animIds.Num( ) )
	{
		bones.Clear( );
		bones.Append( rootID );
	}

	// derive mikktspace tangents from normals
	FinishSurfaces( true );

	LoadModel();
	UpdateMd5Joints();

	// it is now available for use

}

bool idRenderModelGLTF::LoadBinaryModel( idFile* file, const ID_TIME_T sourceTimeStamp )
{
	hasAnimations = false;
	fileExclusive = false; // not written.
	root = nullptr;

	//we should still load the scene information ?
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

	int animCnt;
	file->ReadBig( animCnt );
	if( animCnt > 0 )
	{
		animIds.Resize( animCnt, 1 );
		file->ReadBigArray( animIds.Ptr( ), animCnt );
		animIds.SetNum( animCnt );
	}
	hasAnimations = animCnt > 0;

	int tempNum;
	file->ReadBig( tempNum );
	md5joints.SetNum( tempNum );
	for( int i = 0; i < md5joints.Num( ); i++ )
	{
		file->ReadString( md5joints[i].name );
		int offset;
		file->ReadBig( offset );
		if( offset >= 0 )
		{
			md5joints[i].parent = md5joints.Ptr( ) + offset;
		}
		else
		{
			md5joints[i].parent = NULL;
		}
	}

	int boneCnt;
	file->ReadBig( boneCnt );
	if( boneCnt > 0 )
	{
		bones.Resize( boneCnt, 1 );
		file->ReadBigArray( bones.Ptr( ), boneCnt );
		bones.SetNum( boneCnt );
	}
	else


		if( root->skin == -1 && hasAnimations && !bones.Num() )
		{
			bones.Clear( );
			bones.Append( rootID );
		}

	file->ReadBig( tempNum );
	defaultPose.SetNum( tempNum );
	for( int i = 0; i < defaultPose.Num( ); i++ )
	{
		file->ReadBig( defaultPose[i].q.x );
		file->ReadBig( defaultPose[i].q.y );
		file->ReadBig( defaultPose[i].q.z );
		file->ReadBig( defaultPose[i].q.w );
		file->ReadVec3( defaultPose[i].t );
	}

	file->ReadBig( tempNum );
	invertedDefaultPose.SetNum( tempNum );
	for( int i = 0; i < invertedDefaultPose.Num( ); i++ )
	{
		file->ReadBigArray( invertedDefaultPose[i].ToFloatPtr( ), JOINTMAT_TYPESIZE );
	}
	SIMD_INIT_LAST_JOINT( invertedDefaultPose.Ptr( ), md5joints.Num( ) );

	model_state = hasAnimations ? DM_CONTINUOUS : DM_STATIC;

	//HVG_TODO: replace bonedata with md5jointData
	//UpdateMd5Joints();

	return true;
}

void idRenderModelGLTF::UpdateMd5Joints( )
{
	md5joints.Clear();
	md5joints.Resize( bones.Num() );
	md5joints.SetNum( bones.Num() );
	auto& nodeList = data->NodeList();

	for( int i = 0 ; i < bones.Num(); i++ )
	{
		gltfNode* node = nodeList[bones[i]];

		//check for TRS anim and its artificial root bone
		if( bones.Num() == 1 && node->mesh != -1 )
		{
			md5joints[i].name = "origin";
		}
		else
		{
			md5joints[i].name = node->name;
		}
	}

	auto findMd5Joint = [&]( idStr & name ) -> auto
	{
		for( auto& joint : md5joints )
		{
			if( joint.name == name )
			{
				return &joint;
			}
		}
		assert( 0 );
		static idMD5Joint staticJoint;
		return &staticJoint;
	};

	for( int i = 0 ; i < bones.Num(); i++ )
	{
		gltfNode* node = nodeList[bones[i]];
		if( node->parent )
		{
			md5joints[i].parent = findMd5Joint( node->parent->name );
		}
	}
}

void idRenderModelGLTF::DrawJoints( const struct renderEntity_s* ent, const viewDef_t* view )
{
	int					i;
	int					num;
	idVec3				pos;
	const idJointMat* joint;
	const idMD5Joint* md5Joint;
	int					parentNum;

	num = ent->numJoints;
	joint = ent->joints;
	md5Joint = md5joints.Ptr();
	for( i = 0; i < num; i++, joint++, md5Joint++ )
	{
		pos = ent->origin + joint->ToVec3( ) * ent->axis;
		if( md5Joint->parent )
		{
			parentNum = md5Joint->parent - md5joints.Ptr( );
			common->RW( )->DebugLine( colorWhite, ent->origin + ent->joints[parentNum].ToVec3( ) * ent->axis, pos );
		}

		common->RW( )->DebugLine( colorRed, pos, pos + joint->ToMat3( )[0] * 2.0f * ent->axis );
		common->RW( )->DebugLine( colorGreen, pos, pos + joint->ToMat3( )[1] * 2.0f * ent->axis );
		common->RW( )->DebugLine( colorBlue, pos, pos + joint->ToMat3( )[2] * 2.0f * ent->axis );
	}

	idBounds bounds;

	bounds.FromTransformedBounds( ent->bounds, vec3_zero, ent->axis );
	common->RW( )->DebugBounds( colorMagenta, bounds, ent->origin );

	if( ( r_jointNameScale.GetFloat( ) != 0.0f ) && ( bounds.Expand( 128.0f ).ContainsPoint( view->renderView.vieworg - ent->origin ) ) )
	{
		idVec3	offset( 0, 0, r_jointNameOffset.GetFloat( ) );
		float	scale;

		scale = r_jointNameScale.GetFloat( );
		joint = ent->joints;
		num = ent->numJoints;
		for( i = 0; i < num; i++, joint++ )
		{
			pos = ent->origin + joint->ToVec3( ) * ent->axis;
			common->RW( )->DrawText( md5joints[i].name, pos + offset, scale, colorWhite, view->renderView.viewaxis, 1 );
		}
	}
}

bool gatherBoneInfo( gltfAnimation* gltfAnim, const idList<gltfNode*>& nodes , idList<int, TAG_MODEL>& bones, idList<jointAnimInfo_t, TAG_MD5_ANIM>& jointInfo )
{
	//Gather Bones;
	bool boneLess = false;
	int boneLessTarget = -1;
	for( auto channel : gltfAnim->channels )
	{
		auto* target = nodes[channel->target.node];
		if( target->mesh != 0 )
		{

			if( boneLessTarget == -1 || boneLessTarget != channel->target.node )
			{
				assert( !boneLess );
			}

			boneLess = true;
			boneLessTarget = channel->target.node;
		}
		else
		{
			bones.Append( channel->target.node );
		}
	}

	if( !bones.Num( ) )
	{
		assert( boneLess );
	}
	else
	{
		assert( bones.Num( ) && !boneLess );
	}

	//we cant be sure channels are sorted by bone?
	if( !boneLess )
	{
		assert( 0 );    // implement me
	}
	else
	{
		bones.Append( boneLessTarget );
	}

	//create jointInfo
	jointInfo.SetGranularity( 1 );
	jointInfo.SetNum( bones.Num( ) );
	for( auto& joint : jointInfo )
	{
		joint.animBits = 0;
		joint.firstComponent = -1;
	}

	return boneLess;
}

idFile_Memory* idRenderModelGLTF::GetAnimBin( idStr animName ,  const ID_TIME_T sourceTimeStamp )
{
	///keep in sync with game.
	static const byte B_ANIM_MD5_VERSION = 101;
	static const unsigned int B_ANIM_MD5_MAGIC = ( 'B' << 24 ) | ( 'M' << 16 ) | ( 'D' << 8 ) | B_ANIM_MD5_VERSION;

	idMat4 axisTransform( idAngles( 0.0f, 0.0f, 90.0f ).ToMat3( ), vec3_origin );

	int id;
	idStr gltfFileName = idStr( animName );
	idStr name;
	gltfManager::ExtractIdentifier( gltfFileName, id, name );

	if( gltfParser->currentFile.Length( ) )
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

	gltfData* data = gltfParser->currentAsset;

	auto gltfAnim = data->GetAnimation( name );
	if( !gltfAnim )
	{
		return nullptr;
	}

	auto& accessors = data->AccessorList( );
	auto& nodes = data->NodeList( );

	idList<int, TAG_MODEL>					bones;
	idList<jointAnimInfo_t, TAG_MD5_ANIM>	jointInfo;

	bool boneLess = gatherBoneInfo( gltfAnim, nodes, bones, jointInfo );


	idList<int, TAG_MD5_ANIM>				frameCounts;
	idList<float, TAG_MD5_ANIM>				componentFrames;
	idList<idJointQuat, TAG_MD5_ANIM>		baseFrame;

	idList<idBounds, TAG_MD5_ANIM>			bounds;
	int										numFrames = 0;
	int										frameRate = 0;
	int										numJoints = bones.Num();
	int										numAnimatedComponents = 0;

	gltfNode* root = nullptr;
	int channelCount = 0;
	for( auto channel : gltfAnim->channels )
	{
		auto& sampler = *gltfAnim->samplers[channel->sampler];

		auto* input = accessors[sampler.input];
		auto* output = accessors[sampler.output];
		auto* target = nodes[channel->target.node];
		jointAnimInfo_t* newJoint = &( jointInfo[ bones.FindIndex( channel->target.node ) ] );

		idList<float>& timeStamps = data->GetAccessorView( input );
		root = target;
		int frames = timeStamps.Num();
		frameCounts.Append( frames );

		if( numFrames != 0 && numFrames != frames )
		{
			common->Warning( "Not all channel animations have the same amount of frames" );
		}

		if( frames > numFrames )
		{
			numFrames = frames;
		}

		int parentIndex = data->GetNodeIndex( target->parent );
		newJoint->nameIndex = animationLib.JointIndex( boneLess ? "origin" : target->name );
		newJoint->parentNum = bones.FindIndex( parentIndex );

		if( newJoint->firstComponent == -1 )
		{
			newJoint->firstComponent = numAnimatedComponents;
		}

		switch( channel->target.TRS )
		{
			default:
				break;
			case gltfAnimation_Channel_Target::none:
				break;
			case gltfAnimation_Channel_Target::rotation:
				newJoint->animBits |= ANIM_QX | ANIM_QY | ANIM_QZ;
				numAnimatedComponents += 3;// * frames;
				break;
			case gltfAnimation_Channel_Target::translation:
				newJoint->animBits |= ANIM_TX | ANIM_TY | ANIM_TZ;
				numAnimatedComponents += 3;// * frames;
				break;
			case gltfAnimation_Channel_Target::scale: // this is not supported by engine, but it should be for gltf
				break;
		}

		channelCount++;
	}

	frameRate = gltf_AnimSampleRate.GetInteger();
	INT animLength = ( ( numFrames - 1 ) * 1000 + frameRate - 1 ) / frameRate;

	idFile_Memory* file = new idFile_Memory();
	file->WriteBig( B_ANIM_MD5_MAGIC );
	file->WriteBig( sourceTimeStamp );

	file->WriteBig( numFrames );
	file->WriteBig( frameRate );
	file->WriteBig( animLength );
	file->WriteBig( numJoints );
	file->WriteBig( numAnimatedComponents );

	bounds.SetGranularity( 1 );
	bounds.SetNum( numFrames );
	for( int i = 0 ; i < numFrames; i++ )
	{
		for( int boneId : bones )
		{
			gltfNode* boneNode = nodes[boneId];
			idMat4 trans = mat4_identity;
			gltfData::ResolveNodeMatrix( boneNode, &trans );
			trans *= axisTransform;

			bounds[i].AddPoint( idVec3( trans[0][3], trans[1][3], trans[2][3] ) );
		}
	}

	file->WriteBig( bounds.Num( ) );
	for( int i = 0; i < bounds.Num( ); i++ )
	{
		idBounds& b = bounds[i];
		file->WriteBig( b[0] );
		file->WriteBig( b[1] );
	}

	//namestr list
	file->WriteBig( jointInfo.Num( ) );
	for( int i = 0; i < jointInfo.Num( ); i++ )
	{
		jointAnimInfo_t& j = jointInfo[i];
		idStr jointName = animationLib.JointName( j.nameIndex );
		file->WriteString( jointName );
		file->WriteBig( j.parentNum );
		file->WriteBig( j.animBits );
		file->WriteBig( j.firstComponent );
	}

	baseFrame.SetGranularity( 1 );
	baseFrame.SetNum( bones.Num() );


	for( int i = 0 ; i < bones.Num(); i++ )
	{
		gltfNode* boneNode = nodes[bones[i]];

		baseFrame[i].q = boneNode->rotation * idAngles( 0.0f, 0.0f, 90.0f ).ToMat3( ).Transpose().ToQuat( );
		baseFrame[i].t = boneNode->translation * axisTransform;
		baseFrame[i].w = baseFrame[i].q.CalcW();
	}

	//raw node / skeleton
	file->WriteBig( baseFrame.Num( ) );
	for( int i = 0; i < baseFrame.Num( ); i++ )
	{
		idJointQuat& j = baseFrame[i];
		file->WriteBig( j.q.x );
		file->WriteBig( j.q.y );
		file->WriteBig( j.q.z );
		file->WriteBig( j.q.w );
		file->WriteVec3( j.t );
	}

	componentFrames.SetGranularity( 1 );
	componentFrames.SetNum( numAnimatedComponents * numFrames * numJoints + 1 );
	componentFrames[numAnimatedComponents * numFrames * numJoints  ] = 0.0f;
	int componentFrameIndex = 0;

	for( int i = 0 ; i < numFrames; i++ )
	{

		bool hasTranslation = false;
		bool hasRotation = false;
		bool transDone = false;
		bool rotDone = false;
		//add weight + scale

		while( !transDone && !rotDone )
		{
			idVec3 t = vec3_zero;
			idQuat q;
			q.w = 1.0f;

			for( auto channel : gltfAnim->channels )
			{
				auto& sampler = *gltfAnim->samplers[channel->sampler];

				auto* input = accessors[sampler.input];
				auto* output = accessors[sampler.output];
				auto* target = nodes[channel->target.node];
				idList<float>& timeStamps = data->GetAccessorView( input );
				jointAnimInfo_t* joint = &( jointInfo[ bones.FindIndex( channel->target.node ) ] );
				hasTranslation |= ( joint->animBits & ANIM_TX || joint->animBits & ANIM_TY || joint->animBits & ANIM_TZ );
				hasRotation |= ( joint->animBits & ANIM_QX || joint->animBits & ANIM_QY || joint->animBits & ANIM_QZ );

				int targetID = nodes.FindIndex( target );

				switch( channel->target.TRS )
				{
					default:
						break;
					case gltfAnimation_Channel_Target::none:
						break;
					case gltfAnimation_Channel_Target::rotation:
					{
						idList<idQuat*>& values =  data->GetAccessorView<idQuat>( output );
						if( values.Num() > i )
						{
							q = *values[i];
						}
						rotDone = true;
					}
					break;
					case gltfAnimation_Channel_Target::translation:
					{
						idList<idVec3*>& values = data->GetAccessorView<idVec3>( output );
						if( values.Num() > i )
						{
							t = *values[i];
						}
						transDone = true;
					}
					break;
					case gltfAnimation_Channel_Target::scale: // this is not supported by engine, but it should be for gltf
						break;
				}

				if( !transDone )
				{
					transDone =  !hasTranslation;
				}
				if( !rotDone )
				{
					rotDone =  !hasRotation;
				}
			}

			if( rotDone && transDone )
			{
				if( hasTranslation )
				{
					t *= axisTransform;
					componentFrames[componentFrameIndex++] = t.x;
					componentFrames[componentFrameIndex++] = t.y;
					componentFrames[componentFrameIndex++] = t.z;
				}
				if( hasRotation )
				{
					q *= idAngles( 0.0f, 0.0f, 90.0f ).ToMat3( ).Transpose().ToQuat( );
					idVec3 rot = q.ToRotation().GetVec();
					componentFrames[componentFrameIndex++] = q.x;
					componentFrames[componentFrameIndex++] = q.y;
					componentFrames[componentFrameIndex++] = q.z;
				}
				break;
			}
		}
	}

	assert( componentFrames.Num() == ( componentFrameIndex + 1 ) );
	//per joint timestamp values, T R
	file->WriteBig( componentFrames.Num( ) - 1 );
	for( int i = 0; i < componentFrames.Num( ); i++ )
	{
		file->WriteFloat( componentFrames[i] );
	}

	float* componentPtr = componentFrames.Ptr();
	idVec3 totaldelta;
	// get total move delta
	if( !numAnimatedComponents )
	{
		totaldelta.Zero( );
	}
	else
	{
		componentPtr = &componentFrames[jointInfo[0].firstComponent];
		if( jointInfo[0].animBits & ANIM_TX )
		{
			for( int i = 0; i < numFrames; i++ )
			{
				componentPtr[numAnimatedComponents * i] -= baseFrame[0].t.x;
			}
			totaldelta.x = componentPtr[numAnimatedComponents * ( numFrames - 1 )];
			componentPtr++;
		}
		else
		{
			totaldelta.x = 0.0f;
		}
		if( jointInfo[0].animBits & ANIM_TY )
		{
			for( int i = 0; i < numFrames; i++ )
			{
				componentPtr[numAnimatedComponents * i] -= baseFrame[0].t.y;
			}
			totaldelta.y = componentPtr[numAnimatedComponents * ( numFrames - 1 )];
			componentPtr++;
		}
		else
		{
			totaldelta.y = 0.0f;
		}
		if( jointInfo[0].animBits & ANIM_TZ )
		{
			for( int i = 0; i < numFrames; i++ )
			{
				componentPtr[numAnimatedComponents * i] -= baseFrame[0].t.z;
			}
			totaldelta.z = componentPtr[numAnimatedComponents * ( numFrames - 1 )];
		}
		else
		{
			totaldelta.z = 0.0f;
		}
	}

	file->WriteVec3( totaldelta );
	file->Seek( 0, FS_SEEK_SET );
	file->TakeDataOwnership();
	return file;
}

void idRenderModelGLTF::WriteBinaryModel( idFile* file, ID_TIME_T* _timeStamp /*= NULL */ ) const
{
	idRenderModelStatic::WriteBinaryModel( file );

	if( file == NULL || root == nullptr )
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
		file->WriteBigArray( animIds.Ptr(), animIds.Num() );
	}

	file->WriteBig( md5joints.Num( ) );
	for( int i = 0; i < md5joints.Num( ); i++ )
	{
		file->WriteString( md5joints[i].name );
		int offset = -1;
		if( md5joints[i].parent != NULL )
		{
			offset = md5joints[i].parent - md5joints.Ptr( );
		}
		file->WriteBig( offset );
	}

	file->WriteBig( bones.Num( ) );
	if( bones.Num( ) )
	{
		file->WriteBigArray( bones.Ptr( ), bones.Num( ) );
	}

	file->WriteBig( defaultPose.Num( ) );
	for( int i = 0; i < defaultPose.Num( ); i++ )
	{
		file->WriteBig( defaultPose[i].q.x );
		file->WriteBig( defaultPose[i].q.y );
		file->WriteBig( defaultPose[i].q.z );
		file->WriteBig( defaultPose[i].q.w );
		file->WriteVec3( defaultPose[i].t );
	}

	file->WriteBig( invertedDefaultPose.Num( ) );
	for( int i = 0; i < invertedDefaultPose.Num( ); i++ )
	{
		file->WriteBigArray( invertedDefaultPose[i].ToFloatPtr( ), JOINTMAT_TYPESIZE );
	}
}

void idRenderModelGLTF::PurgeModel()
{
	purged = true;
	md5joints.Clear( );
	defaultPose.Clear( );
	meshes.Clear( );
}

idList<idMat4> getSkinningMatrix( gltfData* data, gltfSkin* skin, gltfAccessor* acc )
{
	idMat4 world = mat4_identity;
	idList<idMat4> values = data->GetAccessorViewMat( acc );

	auto& nodeList = data->NodeList( );
	int count = 0;

	if( skin->skeleton == -1 )
	{
		skin->skeleton = skin->joints[0];
	}

	for( int joint : skin->joints )
	{
		world = mat4_identity;
		auto* node = nodeList[joint];
		idMat4* bindMat = &values[count];
		bindMat->TransposeSelf( );
		data->ResolveNodeMatrix( node, &world );
		*bindMat = world * *bindMat;
		bindMat->TransposeSelf( );
		count++;
	}
	return values;
}

void idRenderModelGLTF::LoadModel()
{
	int meshID;
	idStr meshName;
	idStr gltfFileName = idStr( name );

	gltfManager::ExtractIdentifier( gltfFileName, meshID, meshName );

	if( gltfParser->currentFile.Length( ) )
	{
		if( gltfParser->currentAsset && gltfParser->currentFile != gltfFileName )
		{
			common->FatalError( "multiple GLTF file loading not supported" );
		}
	}
	else
	{
		if( !gltfParser->Load( gltfFileName ) )
		{
			MakeDefaultModel( );
			return;
		}
	}

	timeStamp = fileSystem->GetTimestamp( gltfFileName );

	int			num;
	int			parentNum;

	if( !purged )
	{
		PurgeModel( );
	}

	purged = false;

	gltfData* data = gltfParser->currentAsset;
	auto& accessors = data->AccessorList( );
	auto& nodes = data->NodeList( );
	gltfNode* meshRoot = data->GetNode( gltf_ModelSceneName.GetString(), meshName );
	auto animList = data->GetAnimationIds( meshRoot );

	gltfSkin* skin = nullptr;
	gltfAccessor* acc = nullptr;
	if( meshRoot->skin != -1 )
	{
		skin = data->SkinList( )[meshRoot->skin];
		acc = data->AccessorList( )[skin->inverseBindMatrices];
	}
	else
	{
		skin = new gltfSkin;
		skin->joints.AssureSize( 1, rootID );
		idMat4 trans = mat4_identity;
		data->ResolveNodeMatrix( meshRoot, &trans );
		acc = new gltfAccessor();
		acc->matView = new idList<idMat4>( 1 );
		acc->matView->AssureSize( 1, trans.Inverse() );
	}

	auto basepose = getSkinningMatrix( data, skin, acc );

	if( meshRoot->skin == -1 )
	{
		delete skin;
		delete acc;
	}

	num = bones.Num();
	md5joints.SetGranularity( 1 );
	md5joints.SetNum( num );
	defaultPose.SetGranularity( 1 );
	defaultPose.SetNum( num );

	for( int i = 0; i < bones.Num( ); i++ )
	{
		gltfNode* node = nodes[bones[i]];

		//check for TRS anim and its artficial root bone
		if( bones.Num( ) == 1 && node->mesh != -1 )
		{
			md5joints[i].name = "origin";
		}
		else
		{
			md5joints[i].name = node->name;
		}
	}

	auto findMd5Joint = [&]( idStr & name ) -> auto
	{
		for( auto& joint : md5joints )
		{
			if( joint.name == name )
			{
				return &joint;
			}
		}
		assert( 0 );
		static idMD5Joint staticJoint;
		return &staticJoint;
	};

	idAngles axisTransformAngels = idAngles( 0.0f, 0.0f, 90.0f );
	idMat4 axisTransform( axisTransformAngels.ToMat3( ), vec3_origin );

	for( int i = 0; i < bones.Num( ); i++ )
	{
		gltfNode* node = nodes[bones[i]];
		if( node->parent )
		{
			md5joints[i].parent = findMd5Joint( node->parent->name );
		}
		idJointQuat& pose = defaultPose[i];
		pose.q = node->rotation;
		pose.t = node->translation;
	}

	idJointMat* poseMat = ( idJointMat* ) _alloca16( md5joints.Num( ) * sizeof( poseMat[0] ) );
	for( int i = 0; i < md5joints.Num( ); i++ )
	{
		idMD5Joint* joint = &md5joints[i];
		idJointQuat* pose = &defaultPose[i];

		poseMat[i].SetRotation( pose->q.ToMat3() * axisTransformAngels.ToMat3( ).Transpose() );
		poseMat[i].SetTranslation( pose->t * axisTransform );
		if( joint->parent )
		{
			parentNum = joint->parent - md5joints.Ptr( );
			pose->q = ( poseMat[i].ToMat3( ) * poseMat[parentNum].ToMat3( ).Transpose( ) ).ToQuat( );
			pose->t = ( poseMat[i].ToVec3( ) - poseMat[parentNum].ToVec3( ) ) * poseMat[parentNum].ToMat3( ).Transpose( );
		}
	}


	//-----------------------------------------
	// create the inverse of the base pose joints to support tech6 style deformation
	// of base pose vertexes, normals, and tangents.
	//
	// vertex * joints * inverseJoints == vertex when joints is the base pose
	// When the joints are in another pose, it gives the animated vertex position
	//-----------------------------------------
	invertedDefaultPose.SetNum( SIMD_ROUND_JOINTS( md5joints.Num( ) ) );
	for( int i = 0; i < md5joints.Num( ); i++ )
	{
		invertedDefaultPose[i] = poseMat[i];
		invertedDefaultPose[i].Invert( );
	}
	SIMD_INIT_LAST_JOINT( invertedDefaultPose.Ptr( ), md5joints.Num( ) );


	//deformInfo = R_BuildDeformInfo( texCoords.Num( ), basePose, tris.Num( ), tris.Ptr( ),
	//	shader->UseUnsmoothedTangents( ) );

	model_state = hasAnimations ? DM_CONTINUOUS : DM_STATIC;

	// set the timestamp for reloadmodels
	fileSystem->ReadFile( name, NULL, &timeStamp );

	common->UpdateLevelLoadPacifier( );
}

void idRenderModelGLTF::TouchData()
{
	for( int i = 0; i < surfaces.Num( ); i++ )
	{
		declManager->FindMaterial( surfaces[i].shader->GetName( ) );
	}
}

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

void TransformVertsAndTangents_GLTF( idDrawVert* targetVerts, const int numVerts, const idDrawVert* baseVerts, const idJointMat* joints )
{
	idJointMat trans = joints[0];
	//add skinning
	for( int i = 0; i < numVerts; i++ )
	{
		const idDrawVert& base = baseVerts[i];

		targetVerts[i].xyz = trans * idVec4( base.xyz.x, base.xyz.y, base.xyz.z, 1.0f );
		targetVerts[i].SetNormal( trans * baseVerts[i].GetNormal( ) );
		float t3 =  base.tangent[3];
		targetVerts[i].SetTangent( trans * base.GetTangent( ) );
		targetVerts[i].tangent[3] = t3;
	}
}

void idRenderModelGLTF::UpdateSurface( const struct renderEntity_s* ent, const idJointMat* entJoints, const idJointMat* entJointsInverted, modelSurface_t* surf )
{
#if defined(USE_INTRINSICS_SSE)
	static const __m128 vector_float_posInfinity = { idMath::INFINITUM, idMath::INFINITUM, idMath::INFINITUM, idMath::INFINITUM };
	static const __m128 vector_float_negInfinity = { -idMath::INFINITUM, -idMath::INFINITUM, -idMath::INFINITUM, -idMath::INFINITUM };
#endif

	//add skinning
	if( surf->geometry != NULL )
	{
		R_FreeStaticTriSurfVertexCaches( surf->geometry );
	}
	else
	{
		surf->geometry = R_AllocStaticTriSurf( );
	}

	srfTriangles_t* tri = surf->geometry;
	modelSurface_t& sourceSurf =  surfaces[0];
	int numVerts = sourceSurf.geometry->numVerts;
	idDrawVert* verts = sourceSurf.geometry->verts;

	tri->referencedIndexes = true;
	tri->numIndexes = sourceSurf.geometry->numIndexes;
	tri->indexes = sourceSurf.geometry->indexes;
	tri->silIndexes = sourceSurf.geometry->silIndexes;
	tri->numMirroredVerts = sourceSurf.geometry->numMirroredVerts;
	tri->mirroredVerts = sourceSurf.geometry->mirroredVerts;
	tri->numDupVerts = sourceSurf.geometry->numDupVerts;
	tri->dupVerts = sourceSurf.geometry->dupVerts;
	tri->numSilEdges = sourceSurf.geometry->numSilEdges;
	tri->silEdges = sourceSurf.geometry->silEdges;

	tri->indexCache = sourceSurf.geometry->indexCache;

	tri->numVerts = numVerts;

	if( r_useGPUSkinning.GetBool( ) && glConfig.gpuSkinningAvailable )
	{
		if( tri->verts != NULL && tri->verts != sourceSurf.geometry->verts )
		{
			R_FreeStaticTriSurfVerts( tri );
		}
		tri->verts = sourceSurf.geometry->verts;
		tri->ambientCache = sourceSurf.geometry->ambientCache;
		tri->shadowCache = sourceSurf.geometry->shadowCache;
		tri->referencedVerts = true;
	}
	else
	{
		if( tri->verts == NULL || tri->verts == verts )
		{
			tri->verts = NULL;
			R_AllocStaticTriSurfVerts( tri, numVerts );
			assert( tri->verts != NULL );	// quiet analyze warning
			memcpy( tri->verts, verts, numVerts * sizeof( verts[0] ) );	// copy over the texture coordinates
			tri->referencedVerts = false;
		}

		TransformVertsAndTangents_GLTF( tri->verts, numVerts, verts, entJointsInverted );
		tri->tangentsCalculated = true;
	}

#if defined(USE_INTRINSICS_SSE)

	__m128 minX = vector_float_posInfinity;
	__m128 minY = vector_float_posInfinity;
	__m128 minZ = vector_float_posInfinity;
	__m128 maxX = vector_float_negInfinity;
	__m128 maxY = vector_float_negInfinity;
	__m128 maxZ = vector_float_negInfinity;
	for( int i = 0; i < numVerts; i++ )
	{
		const idJointMat& joint = entJoints[0];
		__m128 x = _mm_load_ps( joint.ToFloatPtr( ) + 0 * 4 );
		__m128 y = _mm_load_ps( joint.ToFloatPtr( ) + 1 * 4 );
		__m128 z = _mm_load_ps( joint.ToFloatPtr( ) + 2 * 4 );
		minX = _mm_min_ps( minX, x );
		minY = _mm_min_ps( minY, y );
		minZ = _mm_min_ps( minZ, z );
		maxX = _mm_max_ps( maxX, x );
		maxY = _mm_max_ps( maxY, y );
		maxZ = _mm_max_ps( maxZ, z );
	}
	__m128 expand = _mm_splat_ps( _mm_load_ss( &maxJointVertDist ), 0 );
	minX = _mm_sub_ps( minX, expand );
	minY = _mm_sub_ps( minY, expand );
	minZ = _mm_sub_ps( minZ, expand );
	maxX = _mm_add_ps( maxX, expand );
	maxY = _mm_add_ps( maxY, expand );
	maxZ = _mm_add_ps( maxZ, expand );
	_mm_store_ss( tri->bounds.ToFloatPtr( ) + 0, _mm_splat_ps( minX, 3 ) );
	_mm_store_ss( tri->bounds.ToFloatPtr( ) + 1, _mm_splat_ps( minY, 3 ) );
	_mm_store_ss( tri->bounds.ToFloatPtr( ) + 2, _mm_splat_ps( minZ, 3 ) );
	_mm_store_ss( tri->bounds.ToFloatPtr( ) + 3, _mm_splat_ps( maxX, 3 ) );
	_mm_store_ss( tri->bounds.ToFloatPtr( ) + 4, _mm_splat_ps( maxY, 3 ) );
	_mm_store_ss( tri->bounds.ToFloatPtr( ) + 5, _mm_splat_ps( maxZ, 3 ) );

#else

	bounds.Clear( );
	for( int i = 0; i < numMeshJoints; i++ )
	{
		const idJointMat& joint = entJoints[meshJoints[i]];
		bounds.AddPoint( joint.GetTranslation( ) );
	}
	bounds.ExpandSelf( maxJointVertDist );

#endif

}

/*
====================
TransformJoints
====================
*/
static void TransformJointsFast( idJointMat* __restrict outJoints, const int numJoints, const idJointMat* __restrict inJoints1, const idJointMat* __restrict inJoints2 )
{

	float* outFloats = outJoints->ToFloatPtr( );
	const float* inFloats1 = inJoints1->ToFloatPtr( );
	const float* inFloats2 = inJoints2->ToFloatPtr( );

	assert_16_byte_aligned( outFloats );
	assert_16_byte_aligned( inFloats1 );
	assert_16_byte_aligned( inFloats2 );

#if defined(USE_INTRINSICS_SSE)

	const __m128 mask_keep_last = __m128c( _mm_set_epi32( 0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000 ) );

	for( int i = 0; i < numJoints; i += 2, inFloats1 += 2 * 12, inFloats2 += 2 * 12, outFloats += 2 * 12 )
	{
		__m128 m1a0 = _mm_load_ps( inFloats1 + 0 * 12 + 0 );
		__m128 m1b0 = _mm_load_ps( inFloats1 + 0 * 12 + 4 );
		__m128 m1c0 = _mm_load_ps( inFloats1 + 0 * 12 + 8 );
		__m128 m1a1 = _mm_load_ps( inFloats1 + 1 * 12 + 0 );
		__m128 m1b1 = _mm_load_ps( inFloats1 + 1 * 12 + 4 );
		__m128 m1c1 = _mm_load_ps( inFloats1 + 1 * 12 + 8 );

		__m128 m2a0 = _mm_load_ps( inFloats2 + 0 * 12 + 0 );
		__m128 m2b0 = _mm_load_ps( inFloats2 + 0 * 12 + 4 );
		__m128 m2c0 = _mm_load_ps( inFloats2 + 0 * 12 + 8 );
		__m128 m2a1 = _mm_load_ps( inFloats2 + 1 * 12 + 0 );
		__m128 m2b1 = _mm_load_ps( inFloats2 + 1 * 12 + 4 );
		__m128 m2c1 = _mm_load_ps( inFloats2 + 1 * 12 + 8 );

		__m128 tj0 = _mm_and_ps( m1a0, mask_keep_last );
		__m128 tk0 = _mm_and_ps( m1b0, mask_keep_last );
		__m128 tl0 = _mm_and_ps( m1c0, mask_keep_last );
		__m128 tj1 = _mm_and_ps( m1a1, mask_keep_last );
		__m128 tk1 = _mm_and_ps( m1b1, mask_keep_last );
		__m128 tl1 = _mm_and_ps( m1c1, mask_keep_last );

		__m128 ta0 = _mm_splat_ps( m1a0, 0 );
		__m128 td0 = _mm_splat_ps( m1b0, 0 );
		__m128 tg0 = _mm_splat_ps( m1c0, 0 );
		__m128 ta1 = _mm_splat_ps( m1a1, 0 );
		__m128 td1 = _mm_splat_ps( m1b1, 0 );
		__m128 tg1 = _mm_splat_ps( m1c1, 0 );

		__m128 ra0 = _mm_add_ps( tj0, _mm_mul_ps( ta0, m2a0 ) );
		__m128 rd0 = _mm_add_ps( tk0, _mm_mul_ps( td0, m2a0 ) );
		__m128 rg0 = _mm_add_ps( tl0, _mm_mul_ps( tg0, m2a0 ) );
		__m128 ra1 = _mm_add_ps( tj1, _mm_mul_ps( ta1, m2a1 ) );
		__m128 rd1 = _mm_add_ps( tk1, _mm_mul_ps( td1, m2a1 ) );
		__m128 rg1 = _mm_add_ps( tl1, _mm_mul_ps( tg1, m2a1 ) );

		__m128 tb0 = _mm_splat_ps( m1a0, 1 );
		__m128 te0 = _mm_splat_ps( m1b0, 1 );
		__m128 th0 = _mm_splat_ps( m1c0, 1 );
		__m128 tb1 = _mm_splat_ps( m1a1, 1 );
		__m128 te1 = _mm_splat_ps( m1b1, 1 );
		__m128 th1 = _mm_splat_ps( m1c1, 1 );

		__m128 rb0 = _mm_add_ps( ra0, _mm_mul_ps( tb0, m2b0 ) );
		__m128 re0 = _mm_add_ps( rd0, _mm_mul_ps( te0, m2b0 ) );
		__m128 rh0 = _mm_add_ps( rg0, _mm_mul_ps( th0, m2b0 ) );
		__m128 rb1 = _mm_add_ps( ra1, _mm_mul_ps( tb1, m2b1 ) );
		__m128 re1 = _mm_add_ps( rd1, _mm_mul_ps( te1, m2b1 ) );
		__m128 rh1 = _mm_add_ps( rg1, _mm_mul_ps( th1, m2b1 ) );

		__m128 tc0 = _mm_splat_ps( m1a0, 2 );
		__m128 tf0 = _mm_splat_ps( m1b0, 2 );
		__m128 ti0 = _mm_splat_ps( m1c0, 2 );
		__m128 tf1 = _mm_splat_ps( m1b1, 2 );
		__m128 ti1 = _mm_splat_ps( m1c1, 2 );
		__m128 tc1 = _mm_splat_ps( m1a1, 2 );

		__m128 rc0 = _mm_add_ps( rb0, _mm_mul_ps( tc0, m2c0 ) );
		__m128 rf0 = _mm_add_ps( re0, _mm_mul_ps( tf0, m2c0 ) );
		__m128 ri0 = _mm_add_ps( rh0, _mm_mul_ps( ti0, m2c0 ) );
		__m128 rc1 = _mm_add_ps( rb1, _mm_mul_ps( tc1, m2c1 ) );
		__m128 rf1 = _mm_add_ps( re1, _mm_mul_ps( tf1, m2c1 ) );
		__m128 ri1 = _mm_add_ps( rh1, _mm_mul_ps( ti1, m2c1 ) );

		_mm_store_ps( outFloats + 0 * 12 + 0, rc0 );
		_mm_store_ps( outFloats + 0 * 12 + 4, rf0 );
		_mm_store_ps( outFloats + 0 * 12 + 8, ri0 );
		_mm_store_ps( outFloats + 1 * 12 + 0, rc1 );
		_mm_store_ps( outFloats + 1 * 12 + 4, rf1 );
		_mm_store_ps( outFloats + 1 * 12 + 8, ri1 );
	}

#else

	for( int i = 0; i < numJoints; i++ )
	{
		idJointMat::Multiply( outJoints[i], inJoints1[i], inJoints2[i] );
	}

#endif
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

	if( !ent->joints )
	{
		common->Printf( "idRenderModelGLTF::InstantiateDynamicModel: NULL joints on renderEntity for '%s'\n", Name( ) );
		delete cachedModel;
		return NULL;
	}
	else if( ent->numJoints != md5joints.Num( ) )
	{
		common->Printf( "idRenderModelGLTF::InstantiateDynamicModel: renderEntity has different number of joints than model for '%s'\n", Name( ) );
		delete cachedModel;
		return NULL;
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

		//idStr prevName = name;
		//name = GLTF_SnapshotName;
		////surfaces = surfaces;
		//*staticModel  = *this;
		//name = prevName;
		staticModel->jointsInverted = NULL;;
	}

	staticModel->bounds.Clear();

	if( r_showSkel.GetInteger( ) )
	{
		if( ( view != NULL ) && ( !r_skipSuppress.GetBool( ) || !ent->suppressSurfaceInViewID || ( ent->suppressSurfaceInViewID != view->renderView.viewID ) ) )
		{
			// only draw the skeleton
			DrawJoints( ent, view );
		}

		if( r_showSkel.GetInteger( ) > 1 )
		{
			// turn off the model when showing the skeleton
			staticModel->InitEmpty( GLTF_SnapshotName );
			return staticModel;
		}
	}

	// update the GPU joints array
	const int numInvertedJoints = SIMD_ROUND_JOINTS( md5joints.Num() );
	if( staticModel->jointsInverted == NULL )
	{
		staticModel->numInvertedJoints = numInvertedJoints;
		staticModel->jointsInverted = ( idJointMat* ) Mem_ClearedAlloc( numInvertedJoints * sizeof( idJointMat ), TAG_JOINTMAT );
		staticModel->jointsInvertedBuffer = 0;
	}
	else
	{
		assert( staticModel->numInvertedJoints == numInvertedJoints );
	}

	TransformJointsFast( staticModel->jointsInverted, md5joints.Num(), ent->joints, invertedDefaultPose.Ptr() );

	modelSurface_t* newSurf;
	if( staticModel->surfaces.Num() )
	{
		newSurf = &staticModel->surfaces[0];
	}
	else
	{
		newSurf = &staticModel->surfaces.Alloc( );
		newSurf->geometry = NULL;
		newSurf->shader = surfaces[0].shader;
	}

	int surfIdx = 0;
	for( modelSurface_t& surf : staticModel->surfaces )
	{
		const idMaterial* shader = newSurf->shader;
		shader = R_RemapShaderBySkin( shader, ent->customSkin, ent->customShader );

		if( !shader || ( !shader->IsDrawn( ) && !shader->SurfaceCastsShadow( ) ) )
		{
			staticModel->DeleteSurfaceWithId( surfIdx++ );
			continue;
		}

		UpdateSurface( ent, ent->joints, staticModel->jointsInverted, &surf );
		assert( surf.geometry != NULL );
		surf.geometry->staticModelWithJoints = staticModel;
		staticModel->bounds.AddBounds( surf.geometry->bounds );
	}

	return staticModel;
}

int idRenderModelGLTF::NumJoints() const
{
	if( !root )
	{
		common->FatalError( "Trying to determine Joint count without a model node loaded" );
	}

	return bones.Num();
}

const idMD5Joint* idRenderModelGLTF::GetJoints() const
{
	idMD5Joint* result  = nullptr;
	if( md5joints.Num() )
	{
		return &md5joints[0];
	}
	else
	{
		common->Warning( "GltfModel has no Joints" );
		return nullptr;
	}
}

jointHandle_t idRenderModelGLTF::GetJointHandle( const char* name ) const
{
	const idMD5Joint* joint = md5joints.Ptr( );
	for( int i = 0; i < md5joints.Num( ); i++, joint++ )
	{
		if( idStr::Icmp( joint->name.c_str( ), name ) == 0 )
		{
			return ( jointHandle_t ) i;
		}
	}

	return INVALID_JOINT;
}

const char* idRenderModelGLTF::GetJointName( jointHandle_t handle ) const
{
	if( ( handle < 0 ) || ( handle >= md5joints.Num( ) ) )
	{
		return "<invalid joint>";
	}

	return md5joints[handle].name;
}

const idJointQuat* idRenderModelGLTF::GetDefaultPose() const
{
	return defaultPose.Ptr();
}

int idRenderModelGLTF::NearestJoint( int surfaceNum, int a, int b, int c ) const
{

	///this needs to be implemented for correct traces
	common->Warning( "The method or operation is not implemented." );
	return -1;
}

idBounds idRenderModelGLTF::Bounds( const struct renderEntity_s* ent ) const
{
	return bounds;
}
