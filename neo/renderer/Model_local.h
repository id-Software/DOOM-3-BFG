/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2012-2021 Robert Beckebans
Copyright (C) 2022 Stephen Pridham

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

#ifndef __MODEL_LOCAL_H__
#define __MODEL_LOCAL_H__

/*
===============================================================================

	Static model

===============================================================================
*/

class idJointMat;
struct deformInfo_t;
class ColladaParser;	// RB: Collada support
struct objModel_t;		// RB: Wavefront OBJ support

class idRenderModelStatic : public idRenderModel
{
public:
	// the inherited public interface
	static idRenderModel* 		Alloc();

	idRenderModelStatic();
	virtual						~idRenderModelStatic();

	virtual void				InitFromFile( const char* fileName, const idImportOptions* options );
	virtual bool				LoadBinaryModel( idFile* file, const ID_TIME_T sourceTimeStamp );
	virtual void				WriteBinaryModel( idFile* file, ID_TIME_T* _timeStamp = NULL ) const;
	virtual bool				SupportsBinaryModel()
	{
		return true;
	}

	// RB begin
	virtual void				ExportOBJ( idFile* objFile, idFile* mtlFile, ID_TIME_T* _timeStamp = NULL );
	// RB end

	virtual void				PartialInitFromFile( const char* fileName );
	virtual void				PurgeModel();
	virtual void				Reset() {};
	virtual void				LoadModel();
	virtual bool				IsLoaded();
	virtual void				SetLevelLoadReferenced( bool referenced );
	virtual bool				IsLevelLoadReferenced();
	virtual void				TouchData();
	virtual void				CreateBuffers( nvrhi::ICommandList* commandList );
	virtual void				InitEmpty( const char* name );
	virtual void				AddSurface( modelSurface_t surface );
	virtual void				FinishSurfaces( bool useMikktspace );
	virtual void				FreeVertexCache();
	virtual const char* 		Name() const;
	virtual void				Print() const;
	virtual void				List() const;
	virtual int					Memory() const;
	virtual ID_TIME_T			Timestamp() const;
	virtual int					NumSurfaces() const;
	virtual int					NumBaseSurfaces() const;
	virtual const modelSurface_t* Surface( int surfaceNum ) const;
	virtual srfTriangles_t* 	AllocSurfaceTriangles( int numVerts, int numIndexes ) const;
	virtual void				FreeSurfaceTriangles( srfTriangles_t* tris ) const;
	virtual bool				IsStaticWorldModel() const;
	virtual dynamicModel_t		IsDynamicModel() const;
	virtual bool				IsDefaultModel() const;
	virtual bool				IsReloadable() const;
	virtual idRenderModel* 		InstantiateDynamicModel( const struct renderEntity_s* ent, const viewDef_t* view, idRenderModel* cachedModel );
	virtual int					NumJoints() const;
	virtual const idMD5Joint* 	GetJoints() const;
	virtual jointHandle_t		GetJointHandle( const char* name ) const;
	virtual const char* 		GetJointName( jointHandle_t handle ) const;
	virtual const idJointQuat* 	GetDefaultPose() const;
	virtual int					NearestJoint( int surfaceNum, int a, int b, int c ) const;
	virtual idBounds			Bounds( const struct renderEntity_s* ent ) const;
	virtual void				ReadFromDemoFile( class idDemoFile* f );
	virtual void				WriteToDemoFile( class idDemoFile* f );
	virtual float				DepthHack() const;

	virtual bool				ModelHasDrawingSurfaces() const
	{
		return hasDrawingSurfaces;
	};
	virtual bool				ModelHasInteractingSurfaces() const
	{
		return hasInteractingSurfaces;
	};
	virtual bool				ModelHasShadowCastingSurfaces() const
	{
		return hasShadowCastingSurfaces;
	};

	void						MakeDefaultModel();

	bool						LoadASE( const char* fileName, ID_TIME_T* sourceTimeStamp );
	bool						LoadLWO( const char* fileName, ID_TIME_T* sourceTimeStamp );
	bool						LoadMA( const char* filename, ID_TIME_T* sourceTimeStamp );
	bool						LoadDAE( const char* fileName, ID_TIME_T* sourceTimeStamp ); // RB
	bool						LoadOBJ( const char* fileName, ID_TIME_T* sourceTimeStamp ); // RB

	bool						ConvertDAEToModelSurfaces( const ColladaParser* dae ); // RB
	bool						ConvertOBJToModelSurfaces( const objModel_t* obj ); // RB
	bool						ConvertASEToModelSurfaces( const struct aseModel_s* ase );
	bool						ConvertLWOToModelSurfaces( const struct st_lwObject* lwo );
	bool						ConvertMAToModelSurfaces( const struct maModel_s* ma );
	bool						ConvertGltfMeshToModelsurfaces( const gltfMesh* mesh );

	struct aseModel_s* 			ConvertLWOToASE( const struct st_lwObject* obj, const char* fileName );

	bool						DeleteSurfaceWithId( int id );
	void						DeleteSurfacesWithNegativeId();
	bool						FindSurfaceWithId( int id, int& surfaceNum ) const;

public:
	idList<modelSurface_t, TAG_MODEL>	surfaces;
	idBounds					bounds;
	int							overlaysAdded;

	// when an md5 is instantiated, the inverted joints array is stored to allow GPU skinning
	int							numInvertedJoints;
	idJointMat* 				jointsInverted;
	vertCacheHandle_t			jointsInvertedBuffer;

protected:
	int							lastModifiedFrame;
	int							lastArchivedFrame;

	idStr						name;
	bool						isStaticWorldModel;
	bool						defaulted;
	bool						purged;					// eventually we will have dynamic reloading
	bool						fastLoad;				// don't generate tangents and shadow data
	bool						reloadable;				// if not, reloadModels won't check timestamp
	bool						levelLoadReferenced;	// for determining if it needs to be freed
	bool						hasDrawingSurfaces;
	bool						hasInteractingSurfaces;
	bool						hasShadowCastingSurfaces;
	ID_TIME_T					timeStamp;

	static idCVar				r_mergeModelSurfaces;	// combine model surfaces with the same material
	static idCVar				r_slopVertex;			// merge xyz coordinates this far apart
	static idCVar				r_slopTexCoord;			// merge texture coordinates this far apart
	static idCVar				r_slopNormal;			// merge normals that dot less than this
};

/*
===============================================================================

	MD5 animated model

===============================================================================
*/

class idMD5Mesh
{
	friend class				idRenderModelMD5;
	friend class				idRenderModelGLTF;

public:
	idMD5Mesh();
	~idMD5Mesh();

	void						ParseMesh( idLexer& parser, int numJoints, const idJointMat* joints );

	int							NumVerts() const
	{
		return numVerts;
	}
	int							NumTris() const
	{
		return numTris;
	}

	void						UpdateSurface( const struct renderEntity_s* ent, const idJointMat* joints,
			const idJointMat* entJointsInverted, modelSurface_t* surf );
	void						CalculateBounds( const idJointMat* entJoints, idBounds& bounds ) const;
	int							NearestJoint( int a, int b, int c ) const;

private:
	const idMaterial* 			shader;				// material applied to mesh
	int							numVerts;			// number of vertices
	int							numTris;			// number of triangles
	byte* 						meshJoints;			// the joints used by this mesh
	int							numMeshJoints;		// number of mesh joints
	float						maxJointVertDist;	// maximum distance a vertex is separated from a joint
	deformInfo_t* 				deformInfo;			// used to create srfTriangles_t from base frames and new vertexes
	int							surfaceNum;			// number of the static surface created for this mesh
};

class idRenderModelMD5 : public idRenderModelStatic
{
	friend class				idRenderModelGLTF;
public:
	void				InitFromFile( const char* fileName, const idImportOptions* options ) override;
	bool				LoadBinaryModel( idFile* file, const ID_TIME_T sourceTimeStamp ) override;
	void				WriteBinaryModel( idFile* file, ID_TIME_T* _timeStamp = NULL ) const override;
	dynamicModel_t		IsDynamicModel() const override;
	idBounds			Bounds( const struct renderEntity_s* ent ) const override;
	void				Print() const override;
	void				List() const override;
	void				TouchData() override;
	void				PurgeModel() override;
	void				LoadModel() override;
	void				CreateBuffers( nvrhi::ICommandList* commandList ) override;
	int					Memory() const override;
	idRenderModel* 		InstantiateDynamicModel( const struct renderEntity_s* ent, const viewDef_t* view, idRenderModel* cachedModel ) override;
	int					NumJoints() const override;
	const idMD5Joint* 	GetJoints() const override;
	jointHandle_t		GetJointHandle( const char* name ) const override;
	const char* 		GetJointName( jointHandle_t handle ) const override;
	const idJointQuat* 	GetDefaultPose() const override;
	int					NearestJoint( int surfaceNum, int a, int b, int c ) const override;

	bool				SupportsBinaryModel() override
	{
		return true;
	}

	// RB begin
	void				ExportOBJ( idFile* objFile, idFile* mtlFile, ID_TIME_T* _timeStamp = NULL ) override;
	// RB end

private:
	idList<idMD5Joint, TAG_MODEL>	joints;
	idList<idJointQuat, TAG_MODEL>	defaultPose;
	idList<idJointMat, TAG_MODEL>	invertedDefaultPose;
	idList<idMD5Mesh, TAG_MODEL>	meshes;

	void						DrawJoints( const renderEntity_t* ent, const viewDef_t* view ) const;
	void						ParseJoint( idLexer& parser, idMD5Joint* joint, idJointQuat* defaultPose );
};

/*
===============================================================================

	MD3 animated model

===============================================================================
*/

struct md3Header_s;
struct md3Surface_s;

class idRenderModelMD3 : public idRenderModelStatic
{
public:
	idRenderModelMD3();

	virtual void				InitFromFile( const char* fileName, const idImportOptions* options );
	virtual bool				SupportsBinaryModel()
	{
		return false;
	}
	virtual dynamicModel_t		IsDynamicModel() const;
	virtual idRenderModel* 		InstantiateDynamicModel( const struct renderEntity_s* ent, const viewDef_t* view, idRenderModel* cachedModel );
	virtual idBounds			Bounds( const struct renderEntity_s* ent ) const;

private:
	int							index;			// model = tr.models[model->index]
	int							dataSize;		// just for listing purposes
	struct md3Header_s* 		md3;			// only if type == MOD_MESH
	int							numLods;
	idList<const idMaterial*>	shaders;		// DG: md3Shader_t::shaderIndex indexes into this array

	void						LerpMeshVertexes( srfTriangles_t* tri, const struct md3Surface_s* surf, const float backlerp, const int frame, const int oldframe ) const;
};

/*
===============================================================================

	Liquid model

===============================================================================
*/

class idRenderModelLiquid : public idRenderModelStatic
{
public:
	idRenderModelLiquid();

	virtual void				InitFromFile( const char* fileName, nvrhi::ICommandList* commandList, const idImportOptions* options );
	virtual bool				SupportsBinaryModel()
	{
		return false;
	}
	virtual dynamicModel_t		IsDynamicModel() const;
	virtual idRenderModel* 		InstantiateDynamicModel( const struct renderEntity_s* ent, const viewDef_t* view, idRenderModel* cachedModel );
	virtual idBounds			Bounds( const struct renderEntity_s* ent ) const;
	virtual void				CreateBuffers( nvrhi::ICommandList* commandList );

	virtual void				Reset();
	void						IntersectBounds( const idBounds& bounds, float displacement );

private:
	modelSurface_t				GenerateSurface( float lerp );
	void						WaterDrop( int x, int y, float* page );
	void						Update();

	int							verts_x;
	int							verts_y;
	float						scale_x;
	float						scale_y;
	int							time;
	int							liquid_type;
	int							update_tics;
	int							seed;

	idRandom					random;

	const idMaterial* 			shader;
	deformInfo_t* 				deformInfo;		// used to create srfTriangles_t from base frames
	// and new vertexes

	float						density;
	float						drop_height;
	int							drop_radius;
	float						drop_delay;

	idList<float, TAG_MODEL>	pages;
	float* 						page1;
	float* 						page2;

	idList<idDrawVert, TAG_MODEL>	verts;

	int							nextDropTime;

};

/*
===============================================================================

	PRT model

===============================================================================
*/

class idRenderModelPrt : public idRenderModelStatic
{
public:
	idRenderModelPrt();

	virtual void				InitFromFile( const char* fileName, const idImportOptions* options );
	virtual bool				SupportsBinaryModel()
	{
		return false;
	}
	virtual void				TouchData();
	virtual dynamicModel_t		IsDynamicModel() const;
	virtual idRenderModel* 		InstantiateDynamicModel( const struct renderEntity_s* ent, const viewDef_t* view, idRenderModel* cachedModel );
	virtual idBounds			Bounds( const struct renderEntity_s* ent ) const;
	virtual float				DepthHack() const;
	virtual int					Memory() const;

	// with the addModels2 arrangement we could have light accepting and
	// shadowing dynamic models, but the original game never did
	virtual bool				ModelHasDrawingSurfaces() const
	{
		return true;
	};
	virtual bool				ModelHasInteractingSurfaces() const
	{
		return false;
	};
	virtual bool				ModelHasShadowCastingSurfaces() const
	{
		return false;
	};

private:
	const idDeclParticle* 		particleSystem;
};

/*
===============================================================================

	Beam model

===============================================================================
*/

class idRenderModelBeam : public idRenderModelStatic
{
public:
	virtual dynamicModel_t		IsDynamicModel() const;
	virtual bool				SupportsBinaryModel()
	{
		return false;
	}
	virtual bool				IsLoaded() const;
	virtual idRenderModel* 		InstantiateDynamicModel( const struct renderEntity_s* ent, const viewDef_t* view, idRenderModel* cachedModel );
	virtual idBounds			Bounds( const struct renderEntity_s* ent ) const;

	// with the addModels2 arrangement we could have light accepting and
	// shadowing dynamic models, but the original game never did
	virtual bool				ModelHasDrawingSurfaces() const
	{
		return true;
	};
	virtual bool				ModelHasInteractingSurfaces() const
	{
		return false;
	};
	virtual bool				ModelHasShadowCastingSurfaces() const
	{
		return false;
	};
};

/*
===============================================================================

	Beam model

===============================================================================
*/
#define MAX_TRAIL_PTS	20

struct Trail_t
{
	int							lastUpdateTime;
	int							duration;

	idVec3						pts[MAX_TRAIL_PTS];
	int							numPoints;
};

class idRenderModelTrail : public idRenderModelStatic
{
	idList<Trail_t, TAG_MODEL>	trails;
	int							numActive;
	idBounds					trailBounds;

public:
	idRenderModelTrail();

	virtual dynamicModel_t		IsDynamicModel() const;
	virtual bool				SupportsBinaryModel()
	{
		return false;
	}
	virtual bool				IsLoaded() const;
	virtual idRenderModel* 		InstantiateDynamicModel( const struct renderEntity_s* ent, const viewDef_t* view, idRenderModel* cachedModel );
	virtual idBounds			Bounds( const struct renderEntity_s* ent ) const;

	// with the addModels2 arrangement we could have light accepting and
	// shadowing dynamic models, but the original game never did
	virtual bool				ModelHasDrawingSurfaces() const
	{
		return true;
	};
	virtual bool				ModelHasInteractingSurfaces() const
	{
		return false;
	};
	virtual bool				ModelHasShadowCastingSurfaces() const
	{
		return false;
	};

	int							NewTrail( idVec3 pt, int duration );
	void						UpdateTrail( int index, idVec3 pt );
	void						DrawTrail( int index, const struct renderEntity_s* ent, srfTriangles_t* tri, float globalAlpha );
};

/*
===============================================================================

	Lightning model

===============================================================================
*/

class idRenderModelLightning : public idRenderModelStatic
{
public:
	virtual dynamicModel_t		IsDynamicModel() const;
	virtual bool				SupportsBinaryModel()
	{
		return false;
	}
	virtual bool				IsLoaded() const;
	virtual idRenderModel* 		InstantiateDynamicModel( const struct renderEntity_s* ent, const viewDef_t* view, idRenderModel* cachedModel );
	virtual idBounds			Bounds( const struct renderEntity_s* ent ) const;

	// with the addModels2 arrangement we could have light accepting and
	// shadowing dynamic models, but the original game never did
	virtual bool				ModelHasDrawingSurfaces() const
	{
		return true;
	};
	virtual bool				ModelHasInteractingSurfaces() const
	{
		return false;
	};
	virtual bool				ModelHasShadowCastingSurfaces() const
	{
		return false;
	};
};

/*
================================================================================

	idRenderModelSprite

================================================================================
*/
class idRenderModelSprite : public idRenderModelStatic
{
public:
	virtual	dynamicModel_t		IsDynamicModel() const;
	virtual bool				SupportsBinaryModel()
	{
		return false;
	}
	virtual	bool				IsLoaded() const;
	virtual	idRenderModel* 		InstantiateDynamicModel( const struct renderEntity_s* ent, const viewDef_t* view, idRenderModel* cachedModel );
	virtual	idBounds			Bounds( const struct renderEntity_s* ent ) const;

	// with the addModels2 arrangement we could have light accepting and
	// shadowing dynamic models, but the original game never did
	virtual bool				ModelHasDrawingSurfaces() const
	{
		return true;
	};
	virtual bool				ModelHasInteractingSurfaces() const
	{
		return false;
	};
	virtual bool				ModelHasShadowCastingSurfaces() const
	{
		return false;
	};
};

#endif /* !__MODEL_LOCAL_H__ */
