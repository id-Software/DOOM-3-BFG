/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 2022 Harrie van Ginneken

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

#pragma once
#include "Model_local.h"

class idRenderModelGLTF : public idRenderModelStatic
{
public:
	virtual void				InitFromFile( const char* fileName );
	virtual bool				LoadBinaryModel( idFile* file, const ID_TIME_T sourceTimeStamp );
	virtual void				WriteBinaryModel( idFile* file, ID_TIME_T* _timeStamp = NULL ) const;
	virtual dynamicModel_t		IsDynamicModel() const;
	virtual idBounds			Bounds( const struct renderEntity_s* ent ) const;
	virtual void				Print() const;
	virtual void				List() const;
	virtual void				TouchData();
	virtual void				PurgeModel();
	virtual void				LoadModel();
	virtual int					Memory() const;
	virtual idRenderModel* 		InstantiateDynamicModel( const struct renderEntity_s* ent, const viewDef_t* view, idRenderModel* cachedModel );
	virtual int					NumJoints() const;
	virtual const idMD5Joint* 	GetJoints() const;
	virtual jointHandle_t		GetJointHandle( const char* name ) const;
	virtual const char* 		GetJointName( jointHandle_t handle ) const;
	virtual const idJointQuat* 	GetDefaultPose() const;
	virtual int					NearestJoint( int surfaceNum, int a, int b, int c ) const;
	virtual bool				SupportsBinaryModel() override
	{
		return true;
	}
	static idFile_Memory* GetAnimBin( idStr animName, const ID_TIME_T sourceTimeStamp );
	int rootID;
private:
	void ProcessNode( gltfNode* modelNode, idMat4 trans, gltfData* data );
	void UpdateSurface( const struct renderEntity_s* ent, const idJointMat* entJoints, const idJointMat* entJointsInverted, modelSurface_t* surf, const modelSurface_t& sourceSurf );
	void UpdateMd5Joints();

	const idMD5Joint*			FindMD5Joint( const idStr& name ) const;
	gltfData* data;
	gltfNode* root;
	bool fileExclusive;
	bool hasAnimations;

	float							maxJointVertDist;	// maximum distance a vertex is separated from a joint
	idList<int, TAG_MODEL>			animIds;
	idList<int, TAG_MODEL>			bones;
	idList<int, TAG_MODEL>			MeshNodeIds;
	dynamicModel_t					model_state;
	idStr							meshName;

	idList<idMD5Joint, TAG_MODEL>	md5joints;
	idList<idJointQuat, TAG_MODEL>	defaultPose;
	idList<idJointMat, TAG_MODEL>	invertedDefaultPose;
	gltfSkin* 						currentSkin;
private:
	void DrawJoints( const struct renderEntity_s* ent, const viewDef_t* view );
};