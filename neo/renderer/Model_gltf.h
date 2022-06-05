#pragma once
#include "Model_local.h"

class gltfManager {
public:
	static bool ExtractMeshIdentifier(idStr& filename , int & meshId, idStr & meshName );
};

class idGltfMesh 
{
public:
	idGltfMesh(gltfMesh * _mesh, gltfData * _data) : mesh(_mesh),data(_data){};

private:
	gltfMesh * mesh;
	gltfData * data;
};

class idRenderModelGLTF : public idRenderModelStatic
{
public:
	void InitFromFile( const char *fileName ) override;
	bool LoadBinaryModel( idFile *file, const ID_TIME_T sourceTimeStamp ) override;
	void WriteBinaryModel( idFile *file, ID_TIME_T *_timeStamp = NULL ) const override;
	bool SupportsBinaryModel( ) override;
	void ExportOBJ( idFile *objFile, idFile *mtlFile, ID_TIME_T *_timeStamp = NULL ) override;
	void PartialInitFromFile( const char *fileName ) override;
	void PurgeModel( ) override;
	void Reset( ) override;
	void LoadModel( ) override;
	bool IsLoaded( ) override;
	void SetLevelLoadReferenced( bool referenced ) override;
	bool IsLevelLoadReferenced( ) override;
	void TouchData( ) override;
	void CreateBuffers( nvrhi::ICommandList *commandList ) override;
	void InitEmpty( const char *name ) override;
	void AddSurface( modelSurface_t surface ) override;
	void FinishSurfaces( bool useMikktspace ) override;
	void FreeVertexCache( ) override;
	const char *Name( ) const override;
	void Print( ) const override;
	void List( ) const override;
	int Memory( ) const override;
	ID_TIME_T Timestamp( ) const override;
	int NumSurfaces( ) const override;
	int NumBaseSurfaces( ) const override;
	const modelSurface_t *Surface( int surfaceNum ) const override;
	srfTriangles_t *AllocSurfaceTriangles( int numVerts, int numIndexes ) const override;
	void FreeSurfaceTriangles( srfTriangles_t *tris ) const override;
	bool IsStaticWorldModel( ) const override;
	dynamicModel_t IsDynamicModel( ) const override;
	bool IsDefaultModel( ) const override;
	bool IsReloadable( ) const override;
	idRenderModel *InstantiateDynamicModel( const struct renderEntity_s *ent, const viewDef_t *view, idRenderModel *cachedModel ) override;
	int NumJoints( ) const override;
	const idMD5Joint *GetJoints( ) const override;
	jointHandle_t GetJointHandle( const char *name ) const override;
	const char *GetJointName( jointHandle_t handle ) const override;
	const idJointQuat *GetDefaultPose( ) const override;
	int NearestJoint( int surfaceNum, int a, int b, int c ) const override;
	idBounds Bounds( const struct renderEntity_s *ent ) const override;
	void ReadFromDemoFile( class idDemoFile *f ) override;
	void WriteToDemoFile( class idDemoFile *f ) override;
	float DepthHack( ) const override;
	bool ModelHasDrawingSurfaces( ) const override;
	bool ModelHasInteractingSurfaces( ) const override;
	bool ModelHasShadowCastingSurfaces( ) const override;
};