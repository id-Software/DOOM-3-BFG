#pragma once
#include "Model_local.h"

class gltfManager
{
public:
	static bool ExtractMeshIdentifier( idStr& filename , int& meshId, idStr& meshName );
};

class idGltfMesh
{
public:
	idGltfMesh( gltfMesh* _mesh, gltfData* _data ) : mesh( _mesh ), data( _data ) {};

private:
	gltfMesh* mesh;
	gltfData* data;
};

class idRenderModelGLTF : public idRenderModelStatic
{
public:
	virtual void				InitFromFile( const char* fileName );
	virtual bool				LoadBinaryModel( idFile* file, const ID_TIME_T sourceTimeStamp );
	virtual void				WriteBinaryModel( idFile* file, ID_TIME_T* _timeStamp = NULL ) const;
	virtual dynamicModel_t		IsDynamicModel( ) const;
	virtual idBounds			Bounds( const struct renderEntity_s* ent ) const;
	virtual void				Print( ) const;
	virtual void				List( ) const;
	virtual void				TouchData( );
	virtual void				PurgeModel( );
	virtual void				LoadModel( );
	virtual int					Memory( ) const;
	virtual idRenderModel* 		InstantiateDynamicModel( const struct renderEntity_s* ent, const viewDef_t* view, idRenderModel* cachedModel );
	virtual int					NumJoints( ) const;
	virtual const idMD5Joint* 	GetJoints( ) const;
	virtual jointHandle_t		GetJointHandle( const char* name ) const;
	virtual const char* 		GetJointName( jointHandle_t handle ) const;
	virtual const idJointQuat* 	GetDefaultPose( ) const;
	virtual int					NearestJoint( int surfaceNum, int a, int b, int c ) const;
	virtual bool				SupportsBinaryModel( ) override
	{
		return false;
	}
private:
	void ProcessNode( gltfNode* modelNode, idMat4 trans, gltfData* data );
};