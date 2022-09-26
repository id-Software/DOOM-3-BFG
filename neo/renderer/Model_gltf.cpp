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


static const byte GLMB_VERSION = 101;
static const unsigned int GLMB_MAGIC = ( 'M' << 24 ) | ( 'L' << 16 ) | ( 'G' << 8 ) | GLMB_VERSION;
static const char* GLTF_SnapshotName = "_GLTF_Snapshot_";
static const idMat4 blenderToDoomTransform( idAngles( 0.0f, 0.0f, 90 ).ToMat3(), vec3_origin );
//static const idMat4 blenderToDoomTransform = mat4_identity;
static idRenderModelGLTF* lastMeshFromFile = nullptr;

bool idRenderModelStatic::ConvertGltfMeshToModelsurfaces( const gltfMesh* mesh )
{
	return false;
}

void idRenderModelGLTF::ProcessNode_r( gltfNode* modelNode, idMat4 parentTransform, gltfData* data )
{
	auto& meshList = data->MeshList();
	auto& nodeList = data->NodeList();

	gltfData::ResolveNodeMatrix( modelNode );

	idMat4 nodeToWorldTransform = parentTransform * modelNode->matrix;

	if( modelNode->mesh >= 0 )
	{
		gltfMesh* targetMesh = meshList[modelNode->mesh];

		for( auto prim : targetMesh->primitives )
		{
			//ConvertFromMeshGltf should only be used for the map, ConvertGltfMeshToModelsurfaces should be used.
			auto* mesh = MapPolygonMesh::ConvertFromMeshGltf( prim, data,  blenderToDoomTransform * nodeToWorldTransform );
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
			tri->numIndexes = mesh->GetNumPolygons() * 3;
			tri->numVerts = mesh->GetNumVertices();

			R_AllocStaticTriSurfIndexes( tri, tri->numIndexes );
			R_AllocStaticTriSurfVerts( tri, tri->numVerts );

			int indx = 0;
			for( int i = 0; i < mesh->GetNumPolygons(); i++ )
			{
				auto& face = mesh->GetFace( i );
				auto& faceIdxs = face.GetIndexes();
				tri->indexes[indx] = faceIdxs[0];
				tri->indexes[indx + 1] = faceIdxs[1];
				tri->indexes[indx + 2] = faceIdxs[2];
				indx += 3;
			}

			tri->bounds.Clear();
			for( int i = 0; i < tri->numVerts; ++i )
			{
				tri->verts[i] = mesh->GetDrawVerts()[i];
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
		ProcessNode_r( nodeList[child], nodeToWorldTransform, data );
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
	currentSkin = nullptr;

	PurgeModel();

	//FIXME FIXME FIXME
	maxJointVertDist = 10;
	idStr gltfFileName = idStr( fileName );
	model_state = DM_STATIC;

	gltfManager::ExtractIdentifier( gltfFileName, meshID, meshName );
	GLTF_Parser gltf;
	gltf.Load( gltfFileName );

	timeStamp = fileSystem->GetTimestamp( gltfFileName );
	data = gltf.currentAsset;

	bounds.Clear();

	int sceneId = data->DefaultScene();
	assert( sceneId >= 0 );

	auto scene = data->SceneList()[sceneId];
	assert( scene );

	auto nodes = data->NodeList();
	assert( nodes.Num() );

	//determine root node
	if( !meshName[0] )
	{
		root = new gltfNode();
		root->name = scene->name;
		root->children.Append( scene->nodes );
		rootID = -1;
		fileExclusive = true;
	}
	else
	{
		gltfNode* modelNode = data->GetNode( gltf_ModelSceneName.GetString(), meshName, &rootID );
		if( modelNode )
		{
			root = modelNode;
		}
	}

	if( !root )
	{
		common->Warning( "Couldn't find model: '%s'", name.c_str() );
		MakeDefaultModel();
		return;
	}

	//get allmeshes in hierachy, starting at root.
	MeshNodeIds.Clear();
	data->GetAllMeshes( root, MeshNodeIds );

	//find all animations and bones
	bones.Clear();
	int totalAnims = 0;
	for( int meshID : MeshNodeIds )
	{
		gltfNode* tmpNode = nodes[meshID];
		int animCount = data->GetAnimationIds( tmpNode , animIds );

		//check if this model has a skeleton/bones
		//if not but it has an anim, create a bone from the target mesh-node as origin.
		if( tmpNode->skin >= 0 )
		{
			currentSkin = data->SkinList()[tmpNode->skin];
			assert( currentSkin );
			if( currentSkin->joints.Num() )
			{
				bones.Append( currentSkin->joints );
				animCount = data->GetAnimationIds( nodes[bones[0]] , animIds );
			}
		}
		else
		{
			animCount = data->GetAnimationIds( tmpNode , animIds );
			bones.Append( meshID );
		}

		totalAnims += animCount;
	}

	hasAnimations = totalAnims > 0;
	model_state = hasAnimations ? DM_CACHED : DM_STATIC;

	ProcessNode_r( root, mat4_identity, data );

	if( surfaces.Num() <= 0 )
	{
		common->Warning( "Couldn't load model: '%s'", name.c_str() );
		MakeDefaultModel();
		return;
	}


	// derive mikktspace tangents from normals
	FinishSurfaces( true );

	LoadModel();
	UpdateMd5Joints();

	// it is now available for use
	lastMeshFromFile = this;
}

bool idRenderModelGLTF::LoadBinaryModel( idFile* file, const ID_TIME_T sourceTimeStamp )
{
	hasAnimations = false;
	fileExclusive = false; // not written.
	root = nullptr;

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

	int animCnt;
	file->ReadBig( animCnt );
	if( animCnt > 0 )
	{
		animIds.Resize( animCnt, 1 );
		file->ReadBigArray( animIds.Ptr(), animCnt );
		animIds.SetNum( animCnt );
	}
	hasAnimations = animCnt > 0;

	int tempNum;
	file->ReadBig( tempNum );
	md5joints.SetNum( tempNum );
	for( int i = 0; i < md5joints.Num(); i++ )
	{
		file->ReadString( md5joints[i].name );
		int offset;
		file->ReadBig( offset );
		if( offset >= 0 )
		{
			md5joints[i].parent = md5joints.Ptr() + offset;
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
		file->ReadBigArray( bones.Ptr(), boneCnt );
		bones.SetNum( boneCnt );
	}
	else
	{
		if( hasAnimations && !bones.Num() )
		{
			bones.Clear();
			bones.Append( rootID );
		}
	}

	file->ReadBig( tempNum );
	defaultPose.SetNum( tempNum );
	for( int i = 0; i < defaultPose.Num(); i++ )
	{
		file->ReadBig( defaultPose[i].q.x );
		file->ReadBig( defaultPose[i].q.y );
		file->ReadBig( defaultPose[i].q.z );
		file->ReadBig( defaultPose[i].q.w );
		file->ReadVec3( defaultPose[i].t );
	}

	file->ReadBig( tempNum );
	invertedDefaultPose.SetNum( tempNum );
	for( int i = 0; i < invertedDefaultPose.Num(); i++ )
	{
		file->ReadBigArray( invertedDefaultPose[i].ToFloatPtr(), JOINTMAT_TYPESIZE );
	}
	SIMD_INIT_LAST_JOINT( invertedDefaultPose.Ptr(), md5joints.Num() );

	model_state = hasAnimations ? DM_CONTINUOUS : DM_STATIC;

	lastMeshFromFile = this;
	return true;
}

const idMD5Joint* idRenderModelGLTF::FindMD5Joint( const idStr& name ) const
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
}

void idRenderModelGLTF::UpdateMd5Joints()
{
	md5joints.Clear();
	md5joints.Resize( bones.Num() );
	md5joints.SetNum( bones.Num() );
	idStr ovrBoneName;
	auto& nodeList = data->NodeList();

	for( int i = 0 ; i < bones.Num(); i++ )
	{
		gltfNode* node = nodeList[bones[i]];

		//check for TRS anim and its artificial root bone
		if( i == 0 && node->name != "origin" )
		{
			if( node->mesh == -1 )
			{
				common->Warning( "First bone of model is not named \"origin\", name forced!" );
			}

			ovrBoneName = node->name;
			md5joints[i].name = "origin";
		}
		else
		{
			md5joints[i].name = node->name;
		}
	}

	for( int i = 0 ; i < bones.Num(); i++ )
	{
		gltfNode* node = nodeList[bones[i]];
		if( i && node->parent && node->parent != root )
		{
			if( node->parent->name == ovrBoneName )
			{
				md5joints[i].parent = FindMD5Joint( idStr( "origin" ) );
			}
			else
			{
				md5joints[i].parent = FindMD5Joint( node->parent->name );
			}
		}
	}

	if( bones.Num() == 1 )
	{
		//patch bone indices
		for( auto& surf : surfaces )
		{
			for( int i = 0; i < surf.geometry->numVerts; i++ )
			{
				idDrawVert& base = surf.geometry->verts[i];
				base.SetColor( PackColor( ( vec4_zero ) ) );
				base.SetColor2( PackColor( ( vec4_one / 4 ) ) );
			}
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
		pos = ent->origin + joint->ToVec3() * ent->axis;
		if( md5Joint->parent )
		{
			parentNum = md5Joint->parent - md5joints.Ptr();
			common->RW()->DebugLine( colorWhite, ent->origin + ent->joints[parentNum].ToVec3() * ent->axis, pos );
		}

		common->RW()->DebugLine( colorRed, pos, pos + joint->ToMat3()[0] * 2.0f * ent->axis );
		common->RW()->DebugLine( colorGreen, pos, pos + joint->ToMat3()[1] * 2.0f * ent->axis );
		common->RW()->DebugLine( colorBlue, pos, pos + joint->ToMat3()[2] * 2.0f * ent->axis );
	}

	idBounds bounds;

	bounds.FromTransformedBounds( ent->bounds, vec3_zero, ent->axis );
	common->RW()->DebugBounds( colorMagenta, bounds, ent->origin );

	if( ( r_jointNameScale.GetFloat() != 0.0f ) && ( bounds.Expand( 128.0f ).ContainsPoint( view->renderView.vieworg - ent->origin ) ) )
	{
		idVec3	offset( 0, 0, r_jointNameOffset.GetFloat() );
		float	scale;

		scale = r_jointNameScale.GetFloat();
		joint = ent->joints;
		num = ent->numJoints;
		for( i = 0; i < num; i++, joint++ )
		{
			pos = ent->origin + joint->ToVec3() * ent->axis;
			common->RW()->DrawText( md5joints[i].name, pos + offset, scale, colorWhite, view->renderView.viewaxis, 1 );
		}
	}
}

bool gatherBoneInfo( gltfData* data, gltfAnimation* gltfAnim, const idList<gltfNode*>& nodes , idList<int, TAG_MODEL>& bones, idList<jointAnimInfo_t, TAG_MD5_ANIM>& jointInfo )
{
	//Gather Bones;
	bool boneLess = false;
	int targetNode = lastMeshFromFile->rootID;

	auto skin = data->GetSkin( gltfAnim );
	auto targets = data->GetAnimTargets( gltfAnim );

	if( skin == nullptr )
	{
		boneLess = true;
	}

	//we cant be sure channels are sorted by bone?
	if( !boneLess )
	{
		if( skin == nullptr )
		{
			skin = data->GetSkin( targetNode );
		}
		assert( skin );
		bones.Append( skin->joints );
	}
	else
	{
		bones.Append( targetNode );
	}

	//create jointInfo
	jointInfo.SetGranularity( 1 );
	jointInfo.SetNum( bones.Num() );
	for( auto& joint : jointInfo )
	{
		joint.animBits = 0;
		joint.firstComponent = -1;
	}

	return boneLess;
}

idList<idJointQuat> GetPose( idList<gltfNode>& bones, idJointMat* poseMat )
{
	idList<idJointQuat> ret;
	ret.AssureSize( bones.Num() );

	for( int i = 0; i < bones.Num(); i++ )
	{
		auto* node = &bones[i];

		idMat4 trans = mat4_identity;
		gltfData::ResolveNodeMatrix( node, &trans, &bones[0] );

		if( node->parent == nullptr )
		{
			node->matrix *= blenderToDoomTransform;
		}

		idJointQuat& pose = ret[i];
		pose.q = ( trans.ToMat3().ToQuat() );
		pose.t = idVec3( trans[0][3], trans[1][3], trans[2][3] );
		pose.w = pose.q.CalcW();
	}

	for( int i = 0; i < bones.Num(); i++ )
	{
		const gltfNode* joint = &bones[i];
		idJointQuat* pose = &ret[i];
		poseMat[i].SetRotation( pose->q.ToMat3() );
		poseMat[i].SetTranslation( pose->t );
		if( joint->parent )
		{
			int parentNum = bones.FindIndex( *joint->parent );
			pose->q = ( poseMat[i].ToMat3() * poseMat[parentNum].ToMat3().Transpose() ).ToQuat();
			pose->t = ( poseMat[i].ToVec3() - poseMat[parentNum].ToVec3() ) * poseMat[parentNum].ToMat3().Transpose();
		}
	}

	return ret;
}

int copyBones( gltfData* data, const idList<int>& bones, idList<gltfNode>& out )
{
	out.Clear();

	auto nodes = data->NodeList();
	for( auto jointId : bones )
	{
		auto* newNode = &out.Alloc();
		*newNode = *nodes[jointId];
	}

	//patch parents
	for( auto& bone : out )
	{
		bool found = false;
		for( int i = 0; i < out.Num(); i++ )
		{
			if( bone.parent && bone.parent->name == out[i].name )
			{
				bone.parent = &out[i];
				found = true;
				break;
			}
		}

		if( !found )
		{
			bone.parent = nullptr;
		}
	}
	//patch childs!
	// -> skipping because not used.
	return out.Num();
}

idFile_Memory* idRenderModelGLTF::GetAnimBin( idStr animName ,  const ID_TIME_T sourceTimeStamp )
{
	assert( lastMeshFromFile );
	///keep in sync with game!
	static const byte B_ANIM_MD5_VERSION = 101;
	static const unsigned int B_ANIM_MD5_MAGIC = ( 'B' << 24 ) | ( 'M' << 16 ) | ( 'D' << 8 ) | B_ANIM_MD5_VERSION;
	GLTF_Parser gltf;
	int id;
	idStr gltfFileName = idStr( animName );
	idStr name;
	gltfManager::ExtractIdentifier( gltfFileName, id, name );
	gltf.Load( gltfFileName );

	gltfData* data = gltf.currentAsset;
	auto& accessors = data->AccessorList();
	auto& nodes = data->NodeList();

	gltfNode* nodeRoot = nodes[lastMeshFromFile->rootID];
	int boneRootNode = lastMeshFromFile->rootID;
	if( nodeRoot->skin > -1 )
	{
		boneRootNode = nodes[data->SkinList()[nodeRoot->skin]->skeleton]->children[0];
	}

	auto gltfAnim = data->GetAnimation( name, boneRootNode );
	if( !gltfAnim )
	{
		return nullptr;
	}

	idList<int, TAG_MODEL>					bones;
	idList<jointAnimInfo_t, TAG_MD5_ANIM>	jointInfo;


	bool boneLess = gatherBoneInfo( data, gltfAnim, nodes, bones, jointInfo );

	idList<idList<gltfNode>>				animBones;
	idList<float, TAG_MD5_ANIM>				componentFrames;
	idList<idJointQuat, TAG_MD5_ANIM>		baseFrame;

	idList<idBounds, TAG_MD5_ANIM>			bounds;
	int										numFrames = 0;
	int										frameRate = 0;
	int										numJoints = bones.Num();
	int										numAnimatedComponents = 0;

	gameLocal.Printf( "Generating MD5Anim for GLTF anim %s from scene %s\n", name.c_str(), gltf_ModelSceneName.GetString() );

	gltfNode* root = nullptr;
	int channelCount = 0;
	for( auto channel : gltfAnim->channels )
	{
		if( !bones.Find( channel->target.node ) )
		{
			continue;
		}

		auto* sampler = gltfAnim->samplers[channel->sampler];

		auto* input = accessors[sampler->input];
		auto* output = accessors[sampler->output];
		auto* target = nodes[channel->target.node];
		jointAnimInfo_t* newJoint = &( jointInfo[ bones.FindIndex( channel->target.node ) ] );

		idList<float>& timeStamps = data->GetAccessorView( input );
		root = target;
		int frames = timeStamps.Num();

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
				numAnimatedComponents += 3;
				break;
			case gltfAnimation_Channel_Target::translation:
				newJoint->animBits |= ANIM_TX | ANIM_TY | ANIM_TZ;
				numAnimatedComponents += 3;
				break;
			case gltfAnimation_Channel_Target::scale: // this is not supported by engine, but it should be for gltf
				break;
		}

		channelCount++;
	}
	animBones.AssureSize( numFrames );
	animBones.SetNum( numFrames );
	for( int i = 0; i < numFrames; i++ )
	{
		int totalCopied = copyBones( data, bones, animBones[i] );
		assert( totalCopied );
	}

	gameLocal.Printf( "Total bones %i \n", bones.Num() );

	//we can calculate frame rate by:
	// max_timestamp_value / totalFrames
	// but keeping it fixed for now.
	frameRate = gltf_AnimSampleRate.GetInteger();
	int animLength = ( ( numFrames - 1 ) * 1000 + frameRate - 1 ) / frameRate;

	for( int i = 0; i < jointInfo.Num(); i++ )
	{
		jointAnimInfo_t& j = jointInfo[i];
		idStr jointName = animationLib.JointName( j.nameIndex );
		if( i == 0 && ( jointName != "origin" ) )
		{
			gameLocal.Warning( "Renaming bone 0 from %s to %s \n", jointName.c_str(), "origin" );
			jointName = "origin";
		}
	}

	baseFrame.SetGranularity( 1 );
	baseFrame.SetNum( bones.Num() );

	gltfSkin* skin = data->GetSkin( gltfAnim );;
	gltfAccessor* acc = nullptr;
	if( skin != nullptr )
	{
		acc = data->AccessorList()[skin->inverseBindMatrices];
	}
	else
	{
		skin = new gltfSkin;
		skin->joints.AssureSize( 1, data->GetNodeIndex( root ) );
		idMat4 trans = mat4_identity;
		data->ResolveNodeMatrix( root, &trans );
		trans *= blenderToDoomTransform.Inverse();
		acc = new gltfAccessor();
		acc->matView = new idList<idMat4>( 1 );
		acc->matView->AssureSize( 1, trans.Inverse().Transpose() );
	}

	idJointMat* poseMat = ( idJointMat* ) _alloca16( bones.Num() * sizeof( poseMat[0] ) );
	baseFrame = GetPose( animBones[0], poseMat );

	componentFrames.SetGranularity( 1 );
	componentFrames.SetNum( ( ( numAnimatedComponents * numFrames ) ) + 1 );
	int componentFrameIndex = 0;
	for( int i = 0; i < numFrames; i++ )
	{
		for( auto channel : gltfAnim->channels )
		{
			if( !bones.Find( channel->target.node ) )
			{
				continue;
			}

			auto sampler = gltfAnim->samplers[channel->sampler];

			auto* input = accessors[sampler->input];
			auto* output = accessors[sampler->output];
			auto* target = nodes[channel->target.node];
			idList<float>& timeStamps = data->GetAccessorView( input );
			int boneIndex = bones.FindIndex( channel->target.node );

			switch( channel->target.TRS )
			{
				default:
					break;
				case gltfAnimation_Channel_Target::none:
					break;
				case gltfAnimation_Channel_Target::rotation:
				{
					idList<idQuat*>& values = data->GetAccessorView<idQuat>( output );
					if( values.Num() > i )
					{
						animBones[i][boneIndex].rotation = *values[i];
					}
				}
				break;
				case gltfAnimation_Channel_Target::translation:
				{
					idList<idVec3*>& values = data->GetAccessorView<idVec3>( output );
					if( values.Num() > i )
					{
						animBones[i][boneIndex].translation = *values[i];
					}
				}
				break;
				case gltfAnimation_Channel_Target::scale:
					idList<idVec3*>& values = data->GetAccessorView<idVec3>( output );
					if( values.Num() > i )
					{
						animBones[i][boneIndex].scale = *values[i] ;
					}
					break;
			}
		}
		for( int b = 0; b < bones.Num(); b++ )
		{
			auto* node = &animBones[i][b];
			jointAnimInfo_t* joint = &( jointInfo[b] );

			idQuat q = node->rotation;
			idVec3 t = node->translation;

			if( joint->animBits & ( ANIM_TX | ANIM_TY | ANIM_TZ ) )
			{
				if( node->parent == nullptr )
				{
					t = blenderToDoomTransform * t;
				}

				componentFrames[componentFrameIndex++] = t.x;
				componentFrames[componentFrameIndex++] = t.y;
				componentFrames[componentFrameIndex++] = t.z;
			}

			if( joint->animBits & ( ANIM_QX | ANIM_QY | ANIM_QZ ) )
			{
				if( node->parent == nullptr )
				{
					//q = blenderToDoomAngels.ToQuat() * animBones[i][b].rotation;

					q = blenderToDoomTransform.ToMat3().ToQuat() * animBones[i][b].rotation;

					//if( animBones[i].Num() == 1 )
					//{
					// this is not hit
					//	q = -animBones[i][b].rotation;
					//}
				}
				else
				{
					q = -animBones[i][b].rotation;
				}

				componentFrames[componentFrameIndex++] = q.x;
				componentFrames[componentFrameIndex++] = q.y;
				componentFrames[componentFrameIndex++] = q.z;
			}
		}
	}

	assert( componentFrames.Num() == ( componentFrameIndex + 1 ) );

	bounds.SetGranularity( 1 );
	bounds.AssureSize( numFrames );
	bounds.SetNum( numFrames );

	//do software skinning to determine bounds.
	idJointMat* currJoints = ( idJointMat* ) _alloca16( bones.Num() * sizeof( poseMat[0] ) );
	for( int i = 0; i < numFrames; i++ )
	{
		bounds[i].Clear();

		for( int b = 0; b < animBones[i].Num(); b++ )
		{
			if( animBones[i][b].parent == nullptr )
			{
				animBones[i][b].translation = blenderToDoomTransform * animBones[i][b].translation;
			}
		}

		idList<idJointMat> joints;
		GetPose( animBones[i], currJoints );
		for( int b = 0; b < animBones[i].Num(); b++ )
		{
			idJointMat mat = poseMat[b];
			mat.Invert();
			idJointMat::Multiply( joints.Alloc(), currJoints[b], mat );
		}

		// an mesh entry _should_ always be before an anim entry!
		// use those verts as base.
		if( lastMeshFromFile != nullptr )
		{
			for( modelSurface_t& surf : lastMeshFromFile->surfaces )
			{
				idDrawVert* verts = surf.geometry->verts;
				int numVerts = surf.geometry->numVerts;

				for( int v = 0; v < numVerts; v++ )
				{
					const idDrawVert& base = verts[v];

					const idJointMat& j0 = joints[base.color[0]];
					const idJointMat& j1 = joints[base.color[1]];
					const idJointMat& j2 = joints[base.color[2]];
					const idJointMat& j3 = joints[base.color[3]];

					const float w0 = base.color2[0] * ( 1.0f / 255.0f );
					const float w1 = base.color2[1] * ( 1.0f / 255.0f );
					const float w2 = base.color2[2] * ( 1.0f / 255.0f );
					const float w3 = base.color2[3] * ( 1.0f / 255.0f );

					idJointMat accum;
					idJointMat::Mul( accum, j0, w0 );
					idJointMat::Mad( accum, j1, w1 );
					idJointMat::Mad( accum, j2, w2 );
					idJointMat::Mad( accum, j3, w3 );

					idVec3 pos = accum * idVec4( base.xyz.x, base.xyz.y, base.xyz.z, 1.0f );
					bounds[i].AddPoint( pos );
				}
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	/// Start writing ////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////
	idFile_Memory* file = new idFile_Memory();
	file->WriteBig( B_ANIM_MD5_MAGIC );
	file->WriteBig( sourceTimeStamp );

	file->WriteBig( numFrames );
	file->WriteBig( frameRate );
	file->WriteBig( animLength );
	file->WriteBig( numJoints );
	file->WriteBig( numAnimatedComponents );




	file->WriteBig( bounds.Num() );
	for( int i = 0; i < bounds.Num(); i++ )
	{
		idBounds& b = bounds[i];
		file->WriteBig( b[0] );
		file->WriteBig( b[1] );
	}

	//namestr list
	file->WriteBig( jointInfo.Num() );
	for( int i = 0; i < jointInfo.Num(); i++ )
	{
		jointAnimInfo_t& j = jointInfo[i];
		idStr jointName = animationLib.JointName( j.nameIndex );

		file->WriteString( jointName );
		file->WriteBig( j.parentNum );
		file->WriteBig( j.animBits );
		file->WriteBig( j.firstComponent );
	}

	//base frame
	file->WriteBig( baseFrame.Num() );
	for( int i = 0; i < baseFrame.Num(); i++ )
	{
		idJointQuat& j = baseFrame[i];
		file->WriteBig( j.q.x );
		file->WriteBig( j.q.y );
		file->WriteBig( j.q.z );
		file->WriteBig( j.q.w );
		file->WriteVec3( j.t );
	}

	//per joint timestamp values, T R
	file->WriteBig( componentFrames.Num() - 1 );
	for( int i = 0; i < componentFrames.Num(); i++ )
	{
		file->WriteFloat( componentFrames[i] );
	}

	float* componentPtr = componentFrames.Ptr();
	idVec3 totaldelta;
	// get total move delta
	if( !numAnimatedComponents )
	{
		totaldelta.Zero();
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

	if( file == NULL )
	{
		return;
	}

	file->WriteBig( GLMB_MAGIC );
	file->WriteBig( model_state );
	file->WriteBig( rootID );
	file->WriteString( file->GetName() );

	file->WriteBig( animIds.Num() );
	if( animIds.Num() )
	{
		file->WriteBigArray( animIds.Ptr(), animIds.Num() );
	}

	file->WriteBig( md5joints.Num() );
	for( int i = 0; i < md5joints.Num(); i++ )
	{
		file->WriteString( md5joints[i].name );
		int offset = -1;
		if( md5joints[i].parent != NULL )
		{
			offset = md5joints[i].parent - md5joints.Ptr();
		}
		file->WriteBig( offset );
	}

	file->WriteBig( bones.Num() );
	if( bones.Num() )
	{
		file->WriteBigArray( bones.Ptr(), bones.Num() );
	}

	file->WriteBig( defaultPose.Num() );
	for( int i = 0; i < defaultPose.Num(); i++ )
	{
		file->WriteBig( defaultPose[i].q.x );
		file->WriteBig( defaultPose[i].q.y );
		file->WriteBig( defaultPose[i].q.z );
		file->WriteBig( defaultPose[i].q.w );
		file->WriteVec3( defaultPose[i].t );
	}

	file->WriteBig( invertedDefaultPose.Num() );
	for( int i = 0; i < invertedDefaultPose.Num(); i++ )
	{
		file->WriteBigArray( invertedDefaultPose[i].ToFloatPtr(), JOINTMAT_TYPESIZE );
	}
}

void idRenderModelGLTF::PurgeModel()
{
	purged = true;
	md5joints.Clear();
	defaultPose.Clear();
	invertedDefaultPose.Clear();

	animIds.Clear();
	bones.Clear();
	MeshNodeIds.Clear();

	//if no root id was set, it is a generated one.
	if( rootID == -1 && root )
	{
		delete root;
	}
}

void idRenderModelGLTF::LoadModel()
{
	int			num;
	auto& accessors = data->AccessorList();
	auto& nodes = data->NodeList();
	gltfNode* meshRoot = root;
	if( !fileExclusive )
	{
		meshRoot = data->GetNode( gltf_ModelSceneName.GetString(), meshName );
	}

	gltfSkin* skin = nullptr;
	gltfAccessor* acc = nullptr;
	if( currentSkin != nullptr )
	{
		skin = currentSkin;
		acc = data->AccessorList()[skin->inverseBindMatrices];
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

	if( skin && skin->skeleton == -1 )
	{
		skin->skeleton = skin->joints[0];
	}

	num = bones.Num();
	md5joints.SetGranularity( 1 );
	md5joints.SetNum( num );
	defaultPose.SetGranularity( 1 );
	defaultPose.SetNum( num );

	for( int i = 0; i < bones.Num(); i++ )
	{
		gltfNode* node = nodes[bones[i]];

		//check for TRS anim and its artficial root bone
		if( bones.Num() == 0 && node->mesh != -1 )
		{
			md5joints[i].name = "origin";
		}
		else
		{
			md5joints[i].name = node->name;
		}
	}

	for( int i = 0; i < bones.Num(); i++ )
	{
		auto* node = nodes[bones[i]];

		if( i && node->parent && node->parent != root )
		{
			md5joints[i].parent = FindMD5Joint( node->parent->name );
		}
	}

	idJointMat* poseMat = ( idJointMat* ) _alloca16( bones.Num() * sizeof( poseMat[0] ) );
	idList<gltfNode> animBones;
	int totalCopied = copyBones( data, bones, animBones );
	defaultPose = GetPose( animBones, poseMat );

	if( !currentSkin )
	{
		delete skin;
		delete acc;
	}

	//-----------------------------------------
	// create the inverse of the base pose joints to support tech6 style deformation
	// of base pose vertexes, normals, and tangents.
	//
	// vertex * joints * inverseJoints == vertex when joints is the base pose
	// When the joints are in another pose, it gives the animated vertex position
	//-----------------------------------------
	invertedDefaultPose.SetNum( SIMD_ROUND_JOINTS( md5joints.Num() ) );
	for( int i = 0; i < md5joints.Num(); i++ )
	{
		invertedDefaultPose[i] = poseMat[i];
		invertedDefaultPose[i].Invert();
	}
	SIMD_INIT_LAST_JOINT( invertedDefaultPose.Ptr(), md5joints.Num() );

	//auto deformInfo = R_BuildDeformInfo( texCoords.Num(), basePose, tris.Num(), tris.Ptr(),
	//	shader->UseUnsmoothedTangents() );

	model_state = hasAnimations ? DM_CACHED : DM_STATIC;

	// set the timestamp for reloadmodels
	fileSystem->ReadFile( name, NULL, &timeStamp );

	purged = false;

	common->UpdateLevelLoadPacifier();
}

void idRenderModelGLTF::TouchData()
{
	for( int i = 0; i < surfaces.Num(); i++ )
	{
		declManager->FindMaterial( surfaces[i].shader->GetName() );
	}
}

void idRenderModelGLTF::Print() const
{
	idRenderModelStatic::Print();
	// TODO
}

void idRenderModelGLTF::List() const
{
	idRenderModelStatic::List();
	// TODO
}

int idRenderModelGLTF::Memory() const
{
	return idRenderModelStatic::Memory();
	// TODO
}

dynamicModel_t idRenderModelGLTF::IsDynamicModel() const
{
	return model_state;
}

idList<int> TransformVertsAndTangents_GLTF( idDrawVert* targetVerts, const int numVerts, const idDrawVert* baseVerts, const idJointMat* joints )
{
	idList<int> jointIds;
	for( int i = 0; i < numVerts; i++ )
	{
		const idDrawVert& base = baseVerts[i];

		const idJointMat& j0 = joints[base.color[0]];
		const idJointMat& j1 = joints[base.color[1]];
		const idJointMat& j2 = joints[base.color[2]];
		const idJointMat& j3 = joints[base.color[3]];

		for( int j = 0; j < 4; j++ )
		{
			jointIds.AddUnique( base.color[j] );
		}

		const float w0 = base.color2[0] * ( 1.0f / 255.0f );
		const float w1 = base.color2[1] * ( 1.0f / 255.0f );
		const float w2 = base.color2[2] * ( 1.0f / 255.0f );
		const float w3 = base.color2[3] * ( 1.0f / 255.0f );

		idJointMat accum;
		idJointMat::Mul( accum, j0, w0 );
		idJointMat::Mad( accum, j1, w1 );
		idJointMat::Mad( accum, j2, w2 );
		idJointMat::Mad( accum, j3, w3 );

		targetVerts[i].xyz = accum * idVec4( base.xyz.x, base.xyz.y, base.xyz.z, 1.0f );
		targetVerts[i].SetNormal( accum * base.GetNormal() );
		targetVerts[i].SetTangent( accum * base.GetTangent() );
		targetVerts[i].tangent[3] = base.tangent[3];

	}
	return jointIds;
}

void idRenderModelGLTF::UpdateSurface( const struct renderEntity_s* ent, const idJointMat* entJoints, const idJointMat* entJointsInverted, modelSurface_t* surf, const modelSurface_t& sourceSurf )
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
		surf->geometry = R_AllocStaticTriSurf();
	}

	srfTriangles_t* tri = surf->geometry;
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

	idList<int> jointIds;

	if( r_useGPUSkinning.GetBool() && glConfig.gpuSkinningAvailable )
	{
		if( tri->verts != NULL && tri->verts != verts )
		{
			R_FreeStaticTriSurfVerts( tri );
		}
		tri->verts = verts;
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

		jointIds = TransformVertsAndTangents_GLTF( tri->verts, numVerts, verts, entJointsInverted );
	}
	tri->tangentsCalculated = true;

#if defined(USE_INTRINSICS_SSE)
	__m128 minX = vector_float_posInfinity;
	__m128 minY = vector_float_posInfinity;
	__m128 minZ = vector_float_posInfinity;
	__m128 maxX = vector_float_negInfinity;
	__m128 maxY = vector_float_negInfinity;
	__m128 maxZ = vector_float_negInfinity;
	for( int i = 0; i < md5joints.Num(); i++ )
	{
		const idJointMat& joint = entJoints[i];
		__m128 x = _mm_load_ps( joint.ToFloatPtr() + 0 * 4 );
		__m128 y = _mm_load_ps( joint.ToFloatPtr() + 1 * 4 );
		__m128 z = _mm_load_ps( joint.ToFloatPtr() + 2 * 4 );
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
	_mm_store_ss( tri->bounds.ToFloatPtr() + 0, _mm_splat_ps( minX, 3 ) );
	_mm_store_ss( tri->bounds.ToFloatPtr() + 1, _mm_splat_ps( minY, 3 ) );
	_mm_store_ss( tri->bounds.ToFloatPtr() + 2, _mm_splat_ps( minZ, 3 ) );
	_mm_store_ss( tri->bounds.ToFloatPtr() + 3, _mm_splat_ps( maxX, 3 ) );
	_mm_store_ss( tri->bounds.ToFloatPtr() + 4, _mm_splat_ps( maxY, 3 ) );
	_mm_store_ss( tri->bounds.ToFloatPtr() + 5, _mm_splat_ps( maxZ, 3 ) );

#else
	bounds.Clear();
	for( int i = 0; i < jointIds.Num(); i++ )
	{
		const idJointMat& joint = entJoints[i];
		bounds.AddPoint( joint.GetTranslation() );
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

	float* outFloats = outJoints->ToFloatPtr();
	const float* inFloats1 = inJoints1->ToFloatPtr();
	const float* inFloats2 = inJoints2->ToFloatPtr();

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
		GLTF_Parser gltf;
		gltf.Load( name );
		data = gltf.currentAsset;
		LoadModel();
	}

	if( !ent->joints )
	{
		common->Printf( "idRenderModelGLTF::InstantiateDynamicModel: NULL joints on renderEntity for '%s'\n", Name() );
		delete cachedModel;
		return NULL;
	}
	else if( ent->numJoints != md5joints.Num() )
	{
		common->Printf( "idRenderModelGLTF::InstantiateDynamicModel: renderEntity has different number of joints than model for '%s'\n", Name() );
		delete cachedModel;
		return NULL;
	}

	idRenderModelStatic* staticModel;
	if( cachedModel != NULL )
	{
		assert( dynamic_cast< idRenderModelStatic* >( cachedModel ) != NULL );
		assert( idStr::Icmp( cachedModel->Name(), GLTF_SnapshotName ) == 0 );
		staticModel = static_cast< idRenderModelStatic* >( cachedModel );
	}
	else
	{
		staticModel = new( TAG_MODEL ) idRenderModelStatic;
		staticModel->InitEmpty( GLTF_SnapshotName );
		staticModel->jointsInverted = NULL;;
	}

	staticModel->bounds.Clear();

	if( r_showSkel.GetInteger() )
	{
		if( ( view != NULL ) && ( !r_skipSuppress.GetBool() || !ent->suppressSurfaceInViewID || ( ent->suppressSurfaceInViewID != view->renderView.viewID ) ) )
		{
			// only draw the skeleton
			DrawJoints( ent, view );
		}

		if( r_showSkel.GetInteger() > 1 )
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

	if( !staticModel->surfaces.Num() )
	{
		for( int i = 0 ; i < surfaces.Num(); i++ )
		{
			modelSurface_t* newSurf = &staticModel->surfaces.Alloc();
			newSurf->geometry = NULL;
			newSurf->shader = surfaces[i].shader;
		}
	}

	int surfIdx = 0;
	for( modelSurface_t& surf : staticModel->surfaces )
	{

		const idMaterial* shader = surf.shader;
		shader = R_RemapShaderBySkin( shader, ent->customSkin, ent->customShader );

		if( !shader || ( !shader->IsDrawn() && !shader->SurfaceCastsShadow() ) )
		{
			staticModel->DeleteSurfaceWithId( surfIdx++ );
			continue;
		}

		UpdateSurface( ent, ent->joints, staticModel->jointsInverted, &surf, surfaces[surfIdx++] );
		assert( surf.geometry != NULL );
		surf.geometry->staticModelWithJoints = staticModel;
		staticModel->bounds.AddBounds( surf.geometry->bounds );
	}

	return staticModel;
}

int idRenderModelGLTF::NumJoints() const
{

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
	const idMD5Joint* joint = md5joints.Ptr();
	for( int i = 0; i < md5joints.Num(); i++, joint++ )
	{
		if( idStr::Icmp( joint->name.c_str(), name ) == 0 )
		{
			return ( jointHandle_t ) i;
		}
	}

	return INVALID_JOINT;
}

const char* idRenderModelGLTF::GetJointName( jointHandle_t handle ) const
{
	if( ( handle < 0 ) || ( handle >= md5joints.Num() ) )
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
	for( const modelSurface_t& surf : surfaces )
	{
		idDrawVert* verts = surf.geometry->verts;
		int numVerts = surf.geometry->numVerts;

		for( int v = 0; v < numVerts; v++ )
		{
			// duplicated vertices might not have weights
			int vertNum;
			if( a >= 0 && a < numVerts )
			{
				vertNum = a;
			}
			else if( b >= 0 && b < numVerts )
			{
				vertNum = b;
			}
			else if( c >= 0 && c < numVerts )
			{
				vertNum = c;
			}
			else
			{
				// all vertices are duplicates which shouldn't happen
				return 0;
			}

			const idDrawVert& vert = verts[vertNum];

			int bestWeight = 0;
			int bestJoint = 0;
			for( int i = 0; i < 4; i++ )
			{
				if( vert.color2[i] > bestWeight )
				{
					bestWeight = vert.color2[i];
					bestJoint = vert.color[i];
				}
			}

			return bestJoint;
		}
	}

	common->Warning( "Couldn't find NearestJoint for : '%s'", name.c_str() );
	return 0;
}

idBounds idRenderModelGLTF::Bounds( const struct renderEntity_s* ent ) const
{
	if( ent == NULL )
	{
		// this is the bounds for the reference pose
		return bounds;
	}

	return ent->bounds;
}
