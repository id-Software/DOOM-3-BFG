/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2022 Stephen Pridham
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
#include "RenderCommon.h"	// just for R_FreeWorldInteractions and R_CreateWorldInteractions

#include <sys/DeviceManager.h>

extern DeviceManager* deviceManager;
extern idCVar r_uploadBufferSizeMB;

idCVar binaryLoadRenderModels( "binaryLoadRenderModels", "1", 0, "enable binary load/write of render models" );
idCVar preload_MapModels( "preload_MapModels", "1", CVAR_SYSTEM | CVAR_BOOL, "preload models during begin or end levelload" );

// RB begin
idCVar postLoadExportModels( "postLoadExportModels", "0", CVAR_BOOL | CVAR_RENDERER, "export models after loading to OBJ model format" );
// RB end

class idRenderModelManagerLocal : public idRenderModelManager
{
public:
	idRenderModelManagerLocal();
	virtual					~idRenderModelManagerLocal() {}

	virtual void			Init();
	virtual void			Shutdown();
	virtual idRenderModel* 	AllocModel();
	virtual void			FreeModel( idRenderModel* model );
	virtual idRenderModel* 	FindModel( const char* modelName, const idImportOptions* options = NULL );
	virtual idRenderModel* 	CheckModel( const char* modelName );
	virtual idRenderModel* 	DefaultModel();
	virtual void			AddModel( idRenderModel* model );
	virtual void			RemoveModel( idRenderModel* model );
	virtual void			ReloadModels( bool forceAll = false );
	virtual void			CreateMeshBuffers( nvrhi::ICommandList* commandList );
	virtual void			FreeModelVertexCaches();
	virtual void			WritePrecacheCommands( idFile* file );
	virtual void			BeginLevelLoad();
	virtual void			EndLevelLoad();
	virtual void			Preload( const idPreloadManifest& manifest );

	virtual	void			PrintMemInfo( MemInfo_t* mi );

private:
	idList<idRenderModel*, TAG_MODEL>	models;
	idHashIndex							hash;
	idRenderModel* 						defaultModel;
	idRenderModel* 						beamModel;
	idRenderModel* 						spriteModel;
	bool								insideLevelLoad;		// don't actually load now
	nvrhi::CommandListHandle			commandList;

	idRenderModel* 			GetModel( const char* modelName, bool createIfNotFound, const idImportOptions* options );

	static void				PrintModel_f( const idCmdArgs& args );
	static void				ListModels_f( const idCmdArgs& args );
	static void				ReloadModels_f( const idCmdArgs& args );
	static void				TouchModel_f( const idCmdArgs& args );
};


idRenderModelManagerLocal	localModelManager;
idRenderModelManager* 		renderModelManager = &localModelManager;

/*
==============
idRenderModelManagerLocal::idRenderModelManagerLocal
==============
*/
idRenderModelManagerLocal::idRenderModelManagerLocal()
{
	defaultModel = NULL;
	beamModel = NULL;
	spriteModel = NULL;
	insideLevelLoad = false;
}

/*
==============
idRenderModelManagerLocal::PrintModel_f
==============
*/
void idRenderModelManagerLocal::PrintModel_f( const idCmdArgs& args )
{
	idRenderModel*	model;

	if( args.Argc() != 2 )
	{
		common->Printf( "usage: printModel <modelName>\n" );
		return;
	}

	model = renderModelManager->CheckModel( args.Argv( 1 ) );
	if( !model )
	{
		common->Printf( "model \"%s\" not found\n", args.Argv( 1 ) );
		return;
	}

	model->Print();
}

/*
==============
idRenderModelManagerLocal::ListModels_f
==============
*/
void idRenderModelManagerLocal::ListModels_f( const idCmdArgs& args )
{
	int		totalMem = 0;
	int		inUse = 0;

	common->Printf( " mem   srf verts tris\n" );
	common->Printf( " ---   --- ----- ----\n" );

	for( int i = 0; i < localModelManager.models.Num(); i++ )
	{
		idRenderModel*	model = localModelManager.models[i];

		if( !model->IsLoaded() )
		{
			continue;
		}
		model->List();
		totalMem += model->Memory();
		inUse++;
	}

	common->Printf( " ---   --- ----- ----\n" );
	common->Printf( " mem   srf verts tris\n" );

	common->Printf( "%i loaded models\n", inUse );
	common->Printf( "total memory: %4.1fM\n", ( float )totalMem / ( 1024 * 1024 ) );
}

/*
==============
idRenderModelManagerLocal::ReloadModels_f
==============
*/
void idRenderModelManagerLocal::ReloadModels_f( const idCmdArgs& args )
{
	if( idStr::Icmp( args.Argv( 1 ), "all" ) == 0 )
	{
		localModelManager.ReloadModels( true );
	}
	else
	{
		localModelManager.ReloadModels( false );
	}
}

/*
==============
idRenderModelManagerLocal::TouchModel_f

Precache a specific model
==============
*/
void idRenderModelManagerLocal::TouchModel_f( const idCmdArgs& args )
{
	const char*	model = args.Argv( 1 );

	if( !model[0] )
	{
		common->Printf( "usage: touchModel <modelName>\n" );
		return;
	}

	common->Printf( "touchModel %s\n", model );
	const bool captureToImage = false;
	common->UpdateScreen( captureToImage );
	idRenderModel* m = renderModelManager->CheckModel( model );
	if( !m )
	{
		common->Printf( "...not found\n" );
	}
}

/*
=================
idRenderModelManagerLocal::WritePrecacheCommands
=================
*/
void idRenderModelManagerLocal::WritePrecacheCommands( idFile* f )
{
	for( int i = 0; i < models.Num(); i++ )
	{
		idRenderModel*	model = models[i];

		if( !model )
		{
			continue;
		}
		if( !model->IsReloadable() )
		{
			continue;
		}

		char	str[1024];
		idStr::snPrintf( str, sizeof( str ), "touchModel %s\n", model->Name() );
		common->Printf( "%s", str );
		f->Printf( "%s", str );
	}
}

/*
=================
idRenderModelManagerLocal::Init
=================
*/
void idRenderModelManagerLocal::Init()
{
	if( !commandList )
	{
		nvrhi::CommandListParameters params = {};
		if( deviceManager->GetGraphicsAPI() == nvrhi::GraphicsAPI::VULKAN )
		{
			// SRS - set upload buffer size to avoid Vulkan staging buffer fragmentation
			size_t maxBufferSize = ( size_t )( r_uploadBufferSizeMB.GetInteger() * 1024 * 1024 );
			params.setUploadChunkSize( maxBufferSize );
		}
		commandList = deviceManager->GetDevice()->createCommandList( params );
	}

	cmdSystem->AddCommand( "listModels", ListModels_f, CMD_FL_RENDERER, "lists all models" );
	cmdSystem->AddCommand( "printModel", PrintModel_f, CMD_FL_RENDERER, "prints model info", idCmdSystem::ArgCompletion_ModelName );
	cmdSystem->AddCommand( "reloadModels", ReloadModels_f, CMD_FL_RENDERER | CMD_FL_CHEAT, "reloads models" );
	cmdSystem->AddCommand( "touchModel", TouchModel_f, CMD_FL_RENDERER, "touches a model", idCmdSystem::ArgCompletion_ModelName );

	insideLevelLoad = false;

	// create a default model
	idRenderModelStatic* model = new( TAG_MODEL ) idRenderModelStatic;
	model->InitEmpty( "_DEFAULT" );
	model->MakeDefaultModel();
	model->SetLevelLoadReferenced( true );
	defaultModel = model;
	AddModel( model );

	// create the beam model
	idRenderModelStatic* beam = new( TAG_MODEL ) idRenderModelBeam;
	beam->InitEmpty( "_BEAM" );
	beam->SetLevelLoadReferenced( true );
	beamModel = beam;
	AddModel( beam );

	idRenderModelStatic* sprite = new( TAG_MODEL ) idRenderModelSprite;
	sprite->InitEmpty( "_SPRITE" );
	sprite->SetLevelLoadReferenced( true );
	spriteModel = sprite;
	AddModel( sprite );
}

/*
=================
idRenderModelManagerLocal::Shutdown
=================
*/
void idRenderModelManagerLocal::Shutdown()
{
	models.DeleteContents( true );
	hash.Free();
	commandList.Reset();
}

/*
=================
idRenderModelManagerLocal::GetModel
=================
*/
idRenderModel* idRenderModelManagerLocal::GetModel( const char* _modelName, bool createIfNotFound, const idImportOptions* options )
{
	if( !_modelName || !_modelName[0] )
	{
		return NULL;
	}

	idStrStatic< MAX_OSPATH > canonical = _modelName;
	canonical.ToLower();

	idStrStatic< MAX_OSPATH > extension;
	canonical.ExtractFileExtension( extension );

	bool isGLTF = false;
	// HvG: GLTF 2 support
	if( ( extension.Icmp( GLTF_GLB_EXT ) == 0 ) || ( extension.Icmp( GLTF_EXT ) == 0 ) )
	{
		isGLTF = true;
	}

	// see if it is already present
	int key = hash.GenerateKey( canonical, false );
	for( int i = hash.First( key ); i != -1; i = hash.Next( i ) )
	{
		idRenderModel* model = models[i];

		if( canonical.Icmp( model->Name() ) == 0 )
		{
			if( !model->IsLoaded() )
			{
				// reload it if it was purged
				idStr generatedFileName = "generated/rendermodels/";
				generatedFileName.AppendPath( canonical );
				generatedFileName.SetFileExtension( va( "b%s", extension.c_str() ) );

				// Get the timestamp on the original file, if it's newer than what is stored in binary model, regenerate it

				ID_TIME_T sourceTimeStamp;

				if( isGLTF )
				{
					idStr gltfFileName = idStr( canonical );
					int gltfMeshId = -1;
					idStr gltfMeshName;
					gltfManager::ExtractIdentifier( gltfFileName, gltfMeshId, gltfMeshName );

					sourceTimeStamp = fileSystem->GetTimestamp( gltfFileName );
				}
				else
				{
					sourceTimeStamp = fileSystem->GetTimestamp( canonical );
				}

				if( model->SupportsBinaryModel() && binaryLoadRenderModels.GetBool() )
				{
					idFileLocal file( fileSystem->OpenFileReadMemory( generatedFileName ) );
					model->PurgeModel();
					if( !model->LoadBinaryModel( file, sourceTimeStamp ) )
					{
						if( isGLTF )
						{
							model->InitFromFile( canonical, options );
						}
						else
						{
							model->LoadModel();
						}
					}
				}
				else
				{
					model->LoadModel();
				}
			}
			else if( insideLevelLoad && !model->IsLevelLoadReferenced() )
			{
				// we are reusing a model already in memory, but
				// touch all the materials to make sure they stay
				// in memory as well
				model->TouchData();
			}

			model->SetLevelLoadReferenced( true );
			return model;
		}
	}

	// see if we can load it

	// determine which subclass of idRenderModel to initialize

	idRenderModel* model = NULL;

	// HvG: GLTF 2 support
	if( isGLTF )
	{
		model = new( TAG_MODEL ) idRenderModelGLTF;
		isGLTF = true;
	}
	// RB: Collada DAE and Wavefront OBJ
	else if( ( extension.Icmp( "dae" ) == 0 ) || ( extension.Icmp( "obj" ) == 0 )
			 || ( extension.Icmp( "ase" ) == 0 ) || ( extension.Icmp( "lwo" ) == 0 )
			 || ( extension.Icmp( "flt" ) == 0 ) || ( extension.Icmp( "ma" ) == 0 ) )
	{
		model = new( TAG_MODEL ) idRenderModelStatic;
	}
	else if( extension.Icmp( MD5_MESH_EXT ) == 0 )
	{
		model = new( TAG_MODEL ) idRenderModelMD5;
	}
	else if( extension.Icmp( "md3" ) == 0 )
	{
		model = new( TAG_MODEL ) idRenderModelMD3;
	}
	else if( extension.Icmp( "prt" ) == 0 )
	{
		model = new( TAG_MODEL ) idRenderModelPrt;
	}
	else if( extension.Icmp( "liquid" ) == 0 )
	{
		model = new( TAG_MODEL ) idRenderModelLiquid;
	}

	idStrStatic< MAX_OSPATH > generatedFileName;

	if( model != NULL )
	{
		generatedFileName = "generated/rendermodels/";
		generatedFileName.AppendPath( canonical );
		generatedFileName.SetFileExtension( va( "b%s", extension.c_str() ) );

		// Get the timestamp on the original file, if it's newer than what is stored in binary model, regenerate it
		ID_TIME_T sourceTimeStamp;

		if( isGLTF )
		{
			idStr gltfFileName = idStr( canonical );
			int gltfMeshId = -1;
			idStr gltfMeshName;
			gltfManager::ExtractIdentifier( gltfFileName, gltfMeshId, gltfMeshName );

			sourceTimeStamp = fileSystem->GetTimestamp( gltfFileName );
		}
		else
		{
			sourceTimeStamp = fileSystem->GetTimestamp( canonical );
		}

		idFileLocal file( fileSystem->OpenFileReadMemory( generatedFileName ) );

		if( !model->SupportsBinaryModel() || !binaryLoadRenderModels.GetBool() )
		{
			model->InitFromFile( canonical, options );
		}
		else
		{
			if( !model->LoadBinaryModel( file, sourceTimeStamp ) )
			{
				model->InitFromFile( canonical, options );

				// RB: default models shouldn't be cached as binary models
				if( !model->IsDefaultModel() )
				{
					idFileLocal outputFile( fileSystem->OpenFileWrite( generatedFileName, "fs_basepath" ) );
					idLib::Printf( "Writing %s\n", generatedFileName.c_str() );
					model->WriteBinaryModel( outputFile, &sourceTimeStamp );
				}
				// RB end
			} /* else {
				idLib::Printf( "loaded binary model %s from file %s\n", model->Name(), generatedFileName.c_str() );
			} */
		}
	}

	// Not one of the known formats
	if( model == NULL )
	{

		if( extension.Length() )
		{
			common->Warning( "unknown model type '%s'", canonical.c_str() );
		}

		if( !createIfNotFound )
		{
			return NULL;
		}

		idRenderModelStatic*	smodel = new( TAG_MODEL ) idRenderModelStatic;
		smodel->InitEmpty( canonical );
		smodel->MakeDefaultModel();

		model = smodel;
	}

	if( cvarSystem->GetCVarBool( "fs_buildresources" ) )
	{
		fileSystem->AddModelPreload( canonical );
	}
	model->SetLevelLoadReferenced( true );

	if( !createIfNotFound && model->IsDefaultModel() )
	{
		delete model;
		model = NULL;

		return NULL;
	}

	if( cvarSystem->GetCVarBool( "fs_buildgame" ) )
	{
		fileSystem->AddModelPreload( model->Name() );
	}

	// RB begin
	if( postLoadExportModels.GetBool() && ( model != defaultModel && model != beamModel && model != spriteModel ) )
	{
		idStrStatic< MAX_OSPATH > exportedFileName;

		exportedFileName = "exported/rendermodels/";

		if( com_editors & EDITOR_EXPORTDEFS )
		{
			exportedFileName = "_tb/";
		}

		exportedFileName.AppendPath( canonical );
		exportedFileName.SetFileExtension( ".obj" );

		ID_TIME_T sourceTimeStamp = fileSystem->GetTimestamp( canonical );
		ID_TIME_T timeStamp = fileSystem->GetTimestamp( exportedFileName );

		// TODO only update if generated has changed

		//if( timeStamp == FILE_NOT_FOUND_TIMESTAMP )
		{
			idFileLocal objFile( fileSystem->OpenFileWrite( exportedFileName, "fs_basepath" ) );
			idLib::Printf( "Writing %s\n", exportedFileName.c_str() );

			exportedFileName.SetFileExtension( ".mtl" );
			idFileLocal mtlFile( fileSystem->OpenFileWrite( exportedFileName, "fs_basepath" ) );

			model->ExportOBJ( objFile, mtlFile );
		}
	}
	// RB end

	AddModel( model );

	return model;
}

/*
=================
idRenderModelManagerLocal::AllocModel
=================
*/
idRenderModel* idRenderModelManagerLocal::AllocModel()
{
	return new( TAG_MODEL ) idRenderModelStatic();
}

/*
=================
idRenderModelManagerLocal::FreeModel
=================
*/
void idRenderModelManagerLocal::FreeModel( idRenderModel* model )
{
	if( !model )
	{
		return;
	}
	if( !dynamic_cast<idRenderModelStatic*>( model ) )
	{
		common->Error( "idRenderModelManager::FreeModel: model '%s' is not a static model", model->Name() );
		return;
	}
	if( model == defaultModel )
	{
		common->Error( "idRenderModelManager::FreeModel: can't free the default model" );
		return;
	}
	if( model == beamModel )
	{
		common->Error( "idRenderModelManager::FreeModel: can't free the beam model" );
		return;
	}
	if( model == spriteModel )
	{
		common->Error( "idRenderModelManager::FreeModel: can't free the sprite model" );
		return;
	}

	R_CheckForEntityDefsUsingModel( model );

	delete model;
}

/*
=================
idRenderModelManagerLocal::FindModel
=================
*/
idRenderModel* idRenderModelManagerLocal::FindModel( const char* modelName, const idImportOptions* options )
{
	return GetModel( modelName, true, options );
}

/*
=================
idRenderModelManagerLocal::CheckModel
=================
*/
idRenderModel* idRenderModelManagerLocal::CheckModel( const char* modelName )
{
	return GetModel( modelName, false, nullptr );
}

/*
=================
idRenderModelManagerLocal::DefaultModel
=================
*/
idRenderModel* idRenderModelManagerLocal::DefaultModel()
{
	return defaultModel;
}

/*
=================
idRenderModelManagerLocal::AddModel
=================
*/
void idRenderModelManagerLocal::AddModel( idRenderModel* model )
{
	hash.Add( hash.GenerateKey( model->Name(), false ), models.Append( model ) );
}

/*
=================
idRenderModelManagerLocal::RemoveModel
=================
*/
void idRenderModelManagerLocal::RemoveModel( idRenderModel* model )
{
	int index = models.FindIndex( model );
	if( index != -1 )
	{
		hash.RemoveIndex( hash.GenerateKey( model->Name(), false ), index );
		models.RemoveIndex( index );
	}
}

/*
=================
idRenderModelManagerLocal::ReloadModels
=================
*/
void idRenderModelManagerLocal::ReloadModels( bool forceAll )
{
	if( forceAll )
	{
		common->Printf( "Reloading all model files...\n" );
	}
	else
	{
		common->Printf( "Checking for changed model files...\n" );
	}

	R_FreeDerivedData();

	// skip the default model at index 0
	for( int i = 1; i < models.Num(); i++ )
	{
		idRenderModel*	model = models[i];

		// we may want to allow world model reloading in the future, but we don't now
		if( !model->IsReloadable() )
		{
			continue;
		}

		bool isGLTF = false;
		idStr filename = model->Name();
		idStr extension;
		idStr assetName = filename;
		assetName.ExtractFileExtension( extension );
		isGLTF = extension.Icmp( "glb" ) == 0 || extension.Icmp( "gltf" ) == 0;
		if( !forceAll )
		{
			// check timestamp
			ID_TIME_T current;

			if( isGLTF )
			{
				idStr meshName;
				int meshID = -1;
				gltfManager::ExtractIdentifier( filename, meshID, meshName );
			}

			fileSystem->ReadFile( filename, NULL, &current );
			if( current <= model->Timestamp() )
			{
				continue;
			}
		}

		common->DPrintf( "^1Reloading %s.\n", model->Name() );

		if( isGLTF )
		{
			// RB: we don't have the options here so make sure this only applies to static models
			model->InitFromFile( model->Name(), NULL );
		}
		else
		{
			model->LoadModel();
		}

	}

	// we must force the world to regenerate, because models may
	// have changed size, making their references invalid
	R_ReCreateWorldReferences();
}

void idRenderModelManagerLocal::CreateMeshBuffers( nvrhi::ICommandList* commandList )
{
	for( int i = 0; i < models.Num(); i++ )
	{
		idRenderModel* model = models[i];

		// Upload vertices and indices and shadow vertices into the vertex cache.
		assert( false && "Stephen should implement me!" );
	}
}

/*
=================
idRenderModelManagerLocal::FreeModelVertexCaches
=================
*/
void idRenderModelManagerLocal::FreeModelVertexCaches()
{
	for( int i = 0; i < models.Num(); i++ )
	{
		idRenderModel* model = models[i];
		model->FreeVertexCache();
	}
}

/*
=================
idRenderModelManagerLocal::BeginLevelLoad
=================
*/
void idRenderModelManagerLocal::BeginLevelLoad()
{
	insideLevelLoad = true;

	for( int i = 0; i < models.Num(); i++ )
	{
		idRenderModel* model = models[i];

		// always reload all models
		if( model->IsReloadable() )
		{
			R_CheckForEntityDefsUsingModel( model );
			model->PurgeModel();
		}

		model->SetLevelLoadReferenced( false );
	}

	vertexCache.FreeStaticData();
}

/*
=================
idRenderModelManagerLocal::Preload
=================
*/
void idRenderModelManagerLocal::Preload( const idPreloadManifest& manifest )
{
	if( preload_MapModels.GetBool() )
	{
		// preload this levels images
		int	start = Sys_Milliseconds();
		int numLoaded = 0;
		idList< preloadSort_t > preloadSort;
		preloadSort.Resize( manifest.NumResources() );
		for( int i = 0; i < manifest.NumResources(); i++ )
		{
			const preloadEntry_s& p = manifest.GetPreloadByIndex( i );
			idResourceCacheEntry rc;
			idStrStatic< MAX_OSPATH > filename;
			if( p.resType == PRELOAD_MODEL )
			{
				filename = "generated/rendermodels/";
				filename += p.resourceName;
				idStrStatic< 16 > ext;
				filename.ExtractFileExtension( ext );
				filename.SetFileExtension( va( "b%s", ext.c_str() ) );
			}
			if( p.resType == PRELOAD_PARTICLE )
			{
				filename = "generated/particles/";
				filename += p.resourceName;
				filename += ".bprt";
			}
			if( !filename.IsEmpty() )
			{
				if( fileSystem->GetResourceCacheEntry( filename, rc ) )
				{
					preloadSort_t ps = {};
					ps.idx = i;
					ps.ofs = rc.offset;
					preloadSort.Append( ps );
				}
			}
		}

		preloadSort.SortWithTemplate( idSort_Preload() );

		for( int i = 0; i < preloadSort.Num(); i++ )
		{
			const preloadSort_t& ps = preloadSort[ i ];
			const preloadEntry_s& p = manifest.GetPreloadByIndex( ps.idx );
			if( p.resType == PRELOAD_MODEL )
			{
				idRenderModel* model = FindModel( p.resourceName );
				if( model != NULL )
				{
					model->SetLevelLoadReferenced( true );
				}
			}
			else if( p.resType == PRELOAD_PARTICLE )
			{
				declManager->FindType( DECL_PARTICLE, p.resourceName );
			}
			numLoaded++;
		}

		int	end = Sys_Milliseconds();
		common->Printf( "%05d models preloaded ( or were already loaded ) in %5.1f seconds\n", numLoaded, ( end - start ) * 0.001 );
		common->Printf( "----------------------------------------\n" );
	}
}

/*
=================
idRenderModelManagerLocal::EndLevelLoad
=================
*/
void idRenderModelManagerLocal::EndLevelLoad()
{
	common->Printf( "----- idRenderModelManagerLocal::EndLevelLoad -----\n" );

	int start = Sys_Milliseconds();

	insideLevelLoad = false;
	int	purgeCount = 0;
	int	keepCount = 0;
	int	loadCount = 0;

	// purge any models not touched
	for( int i = 0; i < models.Num(); i++ )
	{
		idRenderModel* model = models[i];

		if( !model->IsLevelLoadReferenced() && model->IsLoaded() && model->IsReloadable() )
		{
//			common->Printf( "purging %s\n", model->Name() );

			purgeCount++;

			R_CheckForEntityDefsUsingModel( model );

			model->PurgeModel();

		}
		else
		{

//			common->Printf( "keeping %s\n", model->Name() );

			keepCount++;
		}

		common->UpdateLevelLoadPacifier();
	}

	// load any new ones
	for( int i = 0; i < models.Num(); i++ )
	{
		common->UpdateLevelLoadPacifier();


		idRenderModel* model = models[i];

		if( model->IsLevelLoadReferenced() && !model->IsLoaded() && model->IsReloadable() )
		{
			loadCount++;
			model->LoadModel();
		}
	}

	commandList->open();

	for( int i = 0; i < models.Num(); i++ )
	{
		idRenderModel* model = models[i];
		model->CreateBuffers( commandList );
	}

	// create static vertex/index buffers for all models
	for( int i = 0; i < models.Num(); i++ )
	{
		idRenderModel* model = models[i];
		if( model->IsLoaded() )
		{
			for( int j = 0; j < model->NumSurfaces(); j++ )
			{
				R_CreateStaticBuffersForTri( *( model->Surface( j )->geometry ), commandList );
			}
		}
	}

	commandList->close();
	deviceManager->GetDevice()->executeCommandList( commandList );

	// _D3XP added this
	int	end = Sys_Milliseconds();
	common->Printf( "%5i models purged from previous level, ", purgeCount );
	common->Printf( "%5i models kept.\n", keepCount );
	if( loadCount )
	{
		common->Printf( "%5i new models loaded in %5.1f seconds\n", loadCount, ( end - start ) * 0.001 );
	}
	common->Printf( "---------------------------------------------------\n" );
}

/*
=================
idRenderModelManagerLocal::PrintMemInfo
=================
*/
void idRenderModelManagerLocal::PrintMemInfo( MemInfo_t* mi )
{
	int i, j, totalMem = 0;
	int* sortIndex;
	idFile* f;

	f = fileSystem->OpenFileWrite( mi->filebase + "_models.txt" );
	if( !f )
	{
		return;
	}

	// sort first
	sortIndex = new( TAG_MODEL ) int[ localModelManager.models.Num()];

	for( i = 0; i <  localModelManager.models.Num(); i++ )
	{
		sortIndex[i] = i;
	}

	for( i = 0; i <  localModelManager.models.Num() - 1; i++ )
	{
		for( j = i + 1; j <  localModelManager.models.Num(); j++ )
		{
			if( localModelManager.models[sortIndex[i]]->Memory() <  localModelManager.models[sortIndex[j]]->Memory() )
			{
				int temp = sortIndex[i];
				sortIndex[i] = sortIndex[j];
				sortIndex[j] = temp;
			}
		}
	}

	// print next
	for( int i = 0; i < localModelManager.models.Num(); i++ )
	{
		idRenderModel*	model = localModelManager.models[sortIndex[i]];
		int mem;

		if( !model->IsLoaded() )
		{
			continue;
		}

		mem = model->Memory();
		totalMem += mem;
		f->Printf( "%s %s\n", idStr::FormatNumber( mem ).c_str(), model->Name() );
	}

	delete [] sortIndex;
	mi->modelAssetsTotal = totalMem;

	f->Printf( "\nTotal model bytes allocated: %s\n", idStr::FormatNumber( totalMem ).c_str() );
	fileSystem->CloseFile( f );
}



// RB: added Maya exporter options
/*
==============================================================================================

	idTokenizer

==============================================================================================
*/

/*
=================
MayaError
=================
*/
void MayaError( const char* fmt, ... )
{
	va_list	argptr;
	char	text[ 8192 ];

	va_start( argptr, fmt );
	idStr::vsnPrintf( text, sizeof( text ), fmt, argptr );
	va_end( argptr );

	throw idException( text );
}


class idTokenizer
{
private:
	int					currentToken;
	idStrList			tokens;

public:
	idTokenizer()
	{
		Clear();
	};
	void				Clear()
	{
		currentToken = 0;
		tokens.Clear();
	};

	int					SetTokens( const char* buffer );
	const char*			NextToken( const char* errorstring = NULL );

	bool				TokenAvailable()
	{
		return currentToken < tokens.Num();
	};
	int					Num()
	{
		return tokens.Num();
	};
	void				UnGetToken()
	{
		if( currentToken > 0 )
		{
			currentToken--;
		}
	};
	const char*			GetToken( int index )
	{
		if( ( index >= 0 ) && ( index < tokens.Num() ) )
		{
			return tokens[ index ];
		}
		else
		{
			return NULL;
		}
	};
	const char*			CurrentToken()
	{
		return GetToken( currentToken );
	};
};

/*
====================
idTokenizer::SetTokens
====================
*/
int idTokenizer::SetTokens( const char* buffer )
{
	const char* cmd;

	Clear();

	// tokenize commandline
	cmd = buffer;
	while( *cmd )
	{
		// skip whitespace
		while( *cmd && isspace( *cmd ) )
		{
			cmd++;
		}

		if( !*cmd )
		{
			break;
		}

		idStr& current = tokens.Alloc();
		while( *cmd && !isspace( *cmd ) )
		{
			current += *cmd;
			cmd++;
		}
	}

	return tokens.Num();
}

/*
====================
idTokenizer::NextToken
====================
*/
const char* idTokenizer::NextToken( const char* errorstring )
{
	if( currentToken < tokens.Num() )
	{
		return tokens[ currentToken++ ];
	}

	if( errorstring )
	{
		MayaError( "Error: %s", errorstring );
	}

	return NULL;
}




/*
==============================================================================================

	idImportOptions

==============================================================================================
*/

#define DEFAULT_ANIM_EPSILON	0.125f
#define DEFAULT_QUAT_EPSILON	( 1.0f / 8192.0f )

void idImportOptions::Init( const char* commandline, const char* ospath )
{
	idStr		token;
	idNamePair	joints;
	int			i;
	idAnimGroup*	group;
	idStr		sourceDir;
	idStr		destDir;

	//Reset( commandline );
	scale				= 1.0f;
	//type				= WRITE_MESH;
	startframe			= -1;
	endframe			= -1;
	ignoreMeshes		= false;
	clearOrigin			= false;
	clearOriginAxis		= false;
	addOrigin			= false;
	transferRootMotion	= "";
	framerate			= 24;
	align				= "";
	rotate				= 0.0f;
	commandLine			= commandline;
	prefix				= "";
	jointThreshold		= 0.05f;
	ignoreScale			= false;
	xyzPrecision		= DEFAULT_ANIM_EPSILON;
	quatPrecision		= DEFAULT_QUAT_EPSILON;
	cycleStart			= -1;
	reOrient			= ang_zero;
	armature			= "";
	noMikktspace		= false;

	src.Clear();
	dest.Clear();

	idTokenizer tokens;
	tokens.SetTokens( commandline );

	keepjoints.Clear();
	renamejoints.Clear();
	remapjoints.Clear();
	exportgroups.Clear();
	skipmeshes.Clear();
	keepmeshes.Clear();
	groups.Clear();

	/*
	token = tokens.NextToken( "Missing export command" );
	if( token == "mesh" )
	{
		type = WRITE_MESH;
	}
	else if( token == "anim" )
	{
		type = WRITE_ANIM;
	}
	else if( token == "camera" )
	{
		type = WRITE_CAMERA;
	}
	else
	{
		MayaError( "Unknown export command '%s'", token.c_str() );
	}
	*/

	//src = tokens.NextToken( "Missing source filename" );
	//dest = src;

	for( token = tokens.NextToken(); token != ""; token = tokens.NextToken() )
	{
		if( token == "-" )
		{
			token = tokens.NextToken( "Missing import parameter" );

			if( token == "force" )
			{
				// skip
			}
			else if( token == "game" )
			{
				// parse game name
				game = tokens.NextToken( "Expecting game name after -game" );

			}
			else if( token == "rename" )
			{
				// parse joint to rename
				joints.from = tokens.NextToken( "Missing joint name for -rename.  Usage: -rename [joint name] [new name]" );
				joints.to	= tokens.NextToken( "Missing new name for -rename.  Usage: -rename [joint name] [new name]" );
				renamejoints.Append( joints );

			}
			else if( token == "prefix" )
			{
				prefix = tokens.NextToken( "Missing name for -prefix.  Usage: -prefix [joint prefix]" );

			}
			else if( token == "parent" )
			{
				// parse joint to reparent
				joints.from = tokens.NextToken( "Missing joint name for -parent.  Usage: -parent [joint name] [new parent]" );
				joints.to	= tokens.NextToken( "Missing new parent for -parent.  Usage: -parent [joint name] [new parent]" );
				remapjoints.Append( joints );

			}
			else if( !token.Icmp( "sourcedir" ) )
			{
				// parse source directory
				sourceDir = tokens.NextToken( "Missing filename for -sourcedir.  Usage: -sourcedir [directory]" );

			}
			else if( !token.Icmp( "destdir" ) )
			{
				// parse destination directory
				destDir = tokens.NextToken( "Missing filename for -destdir.  Usage: -destdir [directory]" );

			}
			else if( token == "dest" )
			{
				// parse destination filename
				dest = tokens.NextToken( "Missing filename for -dest.  Usage: -dest [filename]" );

			}
			else if( token == "range" )
			{
				// parse frame range to export
				token		= tokens.NextToken( "Missing start frame for -range.  Usage: -range [start frame] [end frame]" );
				startframe	= atoi( token );
				token		= tokens.NextToken( "Missing end frame for -range.  Usage: -range [start frame] [end frame]" );
				endframe	= atoi( token );

				if( startframe > endframe )
				{
					MayaError( "Start frame is greater than end frame." );
				}

			}
			else if( !token.Icmp( "cycleStart" ) )
			{
				// parse start frame of cycle
				token		= tokens.NextToken( "Missing cycle start frame for -cycleStart.  Usage: -cycleStart [first frame of cycle]" );
				cycleStart	= atoi( token );

			}
			else if( token == "scale" )
			{
				// parse scale
				token	= tokens.NextToken( "Missing scale amount for -scale.  Usage: -scale [scale amount]" );
				scale	= atof( token );

			}
			else if( token == "align" )
			{
				// parse align joint
				align = tokens.NextToken( "Missing joint name for -align.  Usage: -align [joint name]" );

			}
			else if( token == "rotate" )
			{
				// parse angle rotation
				token	= tokens.NextToken( "Missing value for -rotate.  Usage: -rotate [yaw]" );
				rotate	= atof( token );

			}
			else if( token == "nomesh" )
			{
				ignoreMeshes = true;

			}
			else if( token == "nomikktspace" )
			{
				noMikktspace = true;

			}
			else if( token == "clearorigin" )
			{
				clearOrigin = true;
				clearOriginAxis = true;

			}
			else if( token == "clearoriginaxis" )
			{
				clearOriginAxis = true;

			}
			else if( token == "addorigin" )
			{
				addOrigin = true;
			}
			else if( token == "transfermotion" )
			{
				token = tokens.NextToken( "Missing value for -transfermotion.  Usage: -transfermotion [bonename]" );
				transferRootMotion = token;
			}
			else if( token == "ignorescale" )
			{
				ignoreScale = true;

			}
			else if( token == "xyzprecision" )
			{
				// parse quaternion precision
				token = tokens.NextToken( "Missing value for -xyzprecision.  Usage: -xyzprecision [precision]" );
				xyzPrecision = atof( token );
				if( xyzPrecision < 0.0f )
				{
					MayaError( "Invalid value for -xyzprecision.  Must be >= 0" );
				}

			}
			else if( token == "quatprecision" )
			{
				// parse quaternion precision
				token = tokens.NextToken( "Missing value for -quatprecision.  Usage: -quatprecision [precision]" );
				quatPrecision = atof( token );
				if( quatPrecision < 0.0f )
				{
					MayaError( "Invalid value for -quatprecision.  Must be >= 0" );
				}

			}
			else if( token == "jointthreshold" )
			{
				// parse joint threshold
				token			= tokens.NextToken( "Missing weight for -jointthreshold.  Usage: -jointthreshold [minimum joint weight]" );
				jointThreshold	= atof( token );

			}
			else if( token == "skipmesh" )
			{
				token = tokens.NextToken( "Missing name for -skipmesh.  Usage: -skipmesh [name of mesh to skip]" );
				skipmeshes.AddUnique( token );

			}
			else if( token == "keepmesh" )
			{
				token = tokens.NextToken( "Missing name for -keepmesh.  Usage: -keepmesh [name of mesh to keep]" );
				keepmeshes.AddUnique( token );

			}
			else if( token == "jointgroup" )
			{
				token	= tokens.NextToken( "Missing name for -jointgroup.  Usage: -jointgroup [group name] [joint1] [joint2]...[joint n]" );
				group = groups.Ptr();
				for( i = 0; i < groups.Num(); i++, group++ )
				{
					if( group->name == token )
					{
						break;
					}
				}

				if( i >= groups.Num() )
				{
					// create a new group
					group = &groups.Alloc();
					group->name = token;
				}

				while( tokens.TokenAvailable() )
				{
					token = tokens.NextToken();
					if( token[ 0 ] == '-' )
					{
						tokens.UnGetToken();
						break;
					}

					group->joints.AddUnique( token );
				}
			}
			else if( token == "group" )
			{
				// add the list of groups to export (these don't affect the hierarchy)
				while( tokens.TokenAvailable() )
				{
					token = tokens.NextToken();
					if( token[ 0 ] == '-' )
					{
						tokens.UnGetToken();
						break;
					}

					group = groups.Ptr();
					for( i = 0; i < groups.Num(); i++, group++ )
					{
						if( group->name == token )
						{
							break;
						}
					}

					if( i >= groups.Num() )
					{
						MayaError( "Unknown group '%s'", token.c_str() );
					}

					exportgroups.AddUnique( group );
				}
			}
			else if( token == "keep" )
			{
				// add joints that are kept whether they're used by a mesh or not
				while( tokens.TokenAvailable() )
				{
					token = tokens.NextToken();
					if( token[ 0 ] == '-' )
					{
						tokens.UnGetToken();
						break;
					}
					keepjoints.AddUnique( token );
				}
			}
			else if( token == "reorient" )
			{
				while( tokens.TokenAvailable() )
				{
					idAngles angle;
					float x = atof( tokens.NextToken() );
					float y = atof( tokens.NextToken() );
					float z = atof( tokens.NextToken() );
					reOrient = idAngles( x, y, z );
					token = tokens.NextToken();
					if( token[0] == '-' )
					{
						tokens.UnGetToken();
						break;
					}
				}
			}
			else if( token == "armature" )
			{
				armature = tokens.NextToken( "Missing skin name for -armature.  Usage: -armature [gltfSkin name]" );
			}
			else
			{
				MayaError( "Unknown option '%s'", token.c_str() );
			}
		}
	}

	token = src;
	src = ospath;
	src.BackSlashesToSlashes();
	src.AppendPath( sourceDir );
	src.AppendPath( token );

	token = dest;
	dest = ospath;
	dest.BackSlashesToSlashes();
	dest.AppendPath( destDir );
	dest.AppendPath( token );

	// Maya only accepts unix style path separators
	src.BackSlashesToSlashes();
	dest.BackSlashesToSlashes();

	if( skipmeshes.Num() && keepmeshes.Num() )
	{
		MayaError( "Can't use -keepmesh and -skipmesh together." );
	}
}

// RB end
