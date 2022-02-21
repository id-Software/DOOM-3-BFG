/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2014-2016 Robert Beckebans
Copyright (C) 2014-2016 Kot in Action Creative Artel

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

#include "RenderCommon.h"

idCVar r_writeDemoDecals( "r_writeDemoDecals", "1", CVAR_BOOL | CVAR_SYSTEM, "enable Writing of entity decals to demo files." );
idCVar r_writeDemoOverlays( "r_writeDemoOverlays", "1", CVAR_BOOL | CVAR_SYSTEM, "enable Writing of entity Overlays to demo files." );

//idCVar r_writeDemoGUI( "r_writeDemoGUI", "0", CVAR_BOOL | CVAR_SYSTEM, "enable Writing of GUI elements to demo files." );
//#define WRITE_GUIS

extern void WriteDeclCache( idDemoFile* f, int demoCategory, int demoCode, declType_t  declType );

typedef struct
{
	int		version;
	int		sizeofRenderEntity;
	int		sizeofRenderLight;
	char	mapname[256];
} demoHeader_t;


/*
==============
StartWritingDemo
==============
*/
void		idRenderWorldLocal::StartWritingDemo( idDemoFile* demo )
{
	int		i;

	// FIXME: we should track the idDemoFile locally, instead of snooping into session for it

	WriteLoadMap();

	// write the door portal state
	for( i = 0 ; i < numInterAreaPortals ; i++ )
	{
		if( doublePortals[i].blockingBits )
		{
			SetPortalState( i + 1, doublePortals[i].blockingBits );
		}
	}

	// clear the archive counter on all defs
	for( i = 0 ; i < lightDefs.Num() ; i++ )
	{
		if( lightDefs[i] )
		{
			lightDefs[i]->archived = false;
		}
	}
	for( i = 0 ; i < entityDefs.Num() ; i++ )
	{
		if( entityDefs[i] )
		{
			entityDefs[i]->archived = false;
		}
	}
}

void idRenderWorldLocal::StopWritingDemo()
{
//	writeDemo = NULL;
}

/*
==============
ProcessDemoCommand
==============
*/
bool		idRenderWorldLocal::ProcessDemoCommand( idDemoFile* readDemo, renderView_t* renderView, int* demoTimeOffset )
{
	bool	newMap = false;

	if( !readDemo )
	{
		return false;
	}

	demoCommand_t	dc;
	qhandle_t		h;

	if( !readDemo->ReadInt( ( int& )dc ) )
	{
		// a demoShot may not have an endFrame, but it is still valid
		return false;
	}

	switch( dc )
	{
		case DC_LOADMAP:
		{
			// read the initial data
			demoHeader_t	header;

			readDemo->ReadInt( header.version );
			readDemo->ReadInt( header.sizeofRenderEntity );
			readDemo->ReadInt( header.sizeofRenderLight );
			for( int i = 0; i < 256; i++ )
			{
				readDemo->ReadChar( header.mapname[i] );
			}
			// the internal version value got replaced by DS_VERSION at toplevel
			if( header.version != 4 )
			{
				common->Error( "Demo version mismatch.\n" );
			}

			if( r_showDemo.GetBool() )
			{
				common->Printf( "DC_LOADMAP: %s\n", header.mapname );
			}
			// Clean up existing Renderer before loading the new map.
			FreeWorld();
			// Load up the new map.
			InitFromMap( header.mapname );

			newMap = true;		// we will need to set demoTimeOffset

			break;
		}
		case DC_CACHE_SKINS:
		{
			int numSkins = 0;
			readDemo->ReadInt( numSkins );

			for( int i = 0; i < numSkins; ++i )
			{
				const char* declName = readDemo->ReadHashString();
				declManager->FindSkin( declName, true );
			}

			if( r_showDemo.GetBool() )
			{
				common->Printf( "DC_CACHESKINS: %d\n", numSkins );
			}
			break;
		}
		case DC_CACHE_PARTICLES:
		{
			int numDecls = 0;
			readDemo->ReadInt( numDecls );

			for( int i = 0; i < numDecls; ++i )
			{
				const char* declName = readDemo->ReadHashString();
				declManager->FindType( DECL_PARTICLE, declName, true );
			}

			if( r_showDemo.GetBool() )
			{
				common->Printf( "DC_CACHE_PARTICLES: %d\n", numDecls );
			}
			break;
		}
		case DC_CACHE_MATERIALS:
		{
			int numDecls = 0;
			readDemo->ReadInt( numDecls );

			for( int i = 0; i < numDecls; ++i )
			{
				const char* declName = readDemo->ReadHashString();
				declManager->FindMaterial( declName, true );
			}

			if( r_showDemo.GetBool() )
			{
				common->Printf( "DC_CACHE_MATERIALS: %d\n", numDecls );
			}
			break;
		}
		case DC_RENDERVIEW:
		{
			readDemo->ReadInt( renderView->viewID );
			readDemo->ReadFloat( renderView->fov_x );
			readDemo->ReadFloat( renderView->fov_y );
			readDemo->ReadVec3( renderView->vieworg );
			readDemo->ReadMat3( renderView->viewaxis );
			readDemo->ReadBool( renderView->cramZNear );
			readDemo->ReadBool( renderView->forceUpdate );
			// binary compatibility with win32 padded structures
			char tmp;
			readDemo->ReadChar( tmp );
			readDemo->ReadChar( tmp );
			readDemo->ReadInt( renderView->time[0] );
			readDemo->ReadInt( renderView->time[1] );
			for( int i = 0; i < MAX_GLOBAL_SHADER_PARMS; i++ )
			{
				readDemo->ReadFloat( renderView->shaderParms[i] );
			}

			if( !readDemo->ReadInt( ( int& )renderView->globalMaterial ) )
			{
				return false;
			}

			if( r_showDemo.GetBool() )
			{
				// foresthale 2014-05-19: /analyze fix - was time, changed to time[0]
				common->Printf( "DC_RENDERVIEW: %i\n", renderView->time[ 0 ] );
			}

			// possibly change the time offset if this is from a new map
			if( newMap && demoTimeOffset )
			{
				*demoTimeOffset = renderView->time[1] - eventLoop->Milliseconds();
			}
			return false;
		}
		case DC_UPDATE_ENTITYDEF:
		{
			ReadRenderEntity();
			break;
		}
		case DC_DELETE_ENTITYDEF:
		{
			if( !readDemo->ReadInt( h ) )
			{
				return false;
			}
			if( r_showDemo.GetBool() )
			{
				common->Printf( "DC_DELETE_ENTITYDEF: %i\n", h );
			}
			FreeEntityDef( h );
			break;
		}
		case DC_UPDATE_LIGHTDEF:
		{
			ReadRenderLight();
			break;
		}
		case DC_DELETE_LIGHTDEF:
		{
			if( !readDemo->ReadInt( h ) )
			{
				return false;
			}
			if( r_showDemo.GetBool() )
			{
				common->Printf( "DC_DELETE_LIGHTDEF: %i\n", h );
			}
			FreeLightDef( h );
			break;
		}
		case DC_CAPTURE_RENDER:
		{
			if( r_showDemo.GetBool() )
			{
				common->Printf( "DC_CAPTURE_RENDER\n" );
			}
			renderSystem->CaptureRenderToImage( readDemo->ReadHashString() );
			break;
		}
		case DC_CROP_RENDER:
		{
			if( r_showDemo.GetBool() )
			{
				common->Printf( "DC_CROP_RENDER\n" );
			}
			int	width, height;
			readDemo->ReadInt( width );
			readDemo->ReadInt( height );
			renderSystem->CropRenderSize( width, height );
			break;
		}
		case DC_UNCROP_RENDER:
		{
			if( r_showDemo.GetBool() )
			{
				common->Printf( "DC_UNCROP\n" );
			}
			renderSystem->UnCrop();
			break;
		}
		case DC_GUI_MODEL:
		{
			if( r_showDemo.GetBool() )
			{
				common->Printf( "DC_GUI_MODEL\n" );
			}
			break;
		}
		case DC_DEFINE_MODEL:
		{
			idRenderModel*	model = renderModelManager->AllocModel();
			model->ReadFromDemoFile( common->ReadDemo() );
			// add to model manager, so we can find it
			renderModelManager->AddModel( model );

			// save it in the list to free when clearing this map
			localModels.Append( model );

			if( r_showDemo.GetBool() )
			{
				common->Printf( "DC_DEFINE_MODEL\n" );
			}
			break;
		}
		case DC_UPDATE_DECAL:
		{
			if( !readDemo->ReadInt( h ) )
			{
				return false;
			}
			int		data[ 2 ];
			readDemo->ReadInt( data[ 0 ] );
			readDemo->ReadInt( data[ 1 ] );
			decals[ h ].entityHandle = data[ 0 ];
			decals[ h ].lastStartTime = data[ 1 ];
			decals[ h ].decals->ReadFromDemoFile( readDemo );
			break;
		}
		case DC_DELETE_DECAL:
		{
			if( !readDemo->ReadInt( h ) )
			{
				return false;
			}

			int		data[ 2 ];
			readDemo->ReadInt( data[ 0 ] );
			readDemo->ReadInt( data[ 1 ] );
			decals[ h ].entityHandle = data[ 0 ];
			decals[ h ].lastStartTime = data[ 1 ];
			decals[ h ].decals->ReUse();
			break;
		}
		case DC_UPDATE_OVERLAY:
		{
			if( !readDemo->ReadInt( h ) )
			{
				return false;
			}
			int		data[ 2 ];
			readDemo->ReadInt( data[ 0 ] );
			readDemo->ReadInt( data[ 1 ] );
			overlays[ h ].entityHandle = data[ 0 ];
			overlays[ h ].lastStartTime = data[ 1 ];
			overlays[ h ].overlays->ReadFromDemoFile( readDemo );
			break;
		}
		case DC_DELETE_OVERLAY:
		{
			if( !readDemo->ReadInt( h ) )
			{
				return false;
			}
			int		data[ 2 ];
			readDemo->ReadInt( data[ 0 ] );
			readDemo->ReadInt( data[ 1 ] );
			overlays[ h ].entityHandle = data[ 0 ];
			overlays[ h ].lastStartTime = data[ 1 ];
			overlays[ h ].overlays->ReUse();
			break;
		}
		case DC_SET_PORTAL_STATE:
		{
			int		data[2];
			readDemo->ReadInt( data[0] );
			readDemo->ReadInt( data[1] );
			SetPortalState( data[0], data[1] );
			if( r_showDemo.GetBool() )
			{
				common->Printf( "DC_SET_PORTAL_STATE: %i %i\n", data[0], data[1] );
			}
			break;
		}
		case DC_END_FRAME:
		{
			if( r_showDemo.GetBool() )
			{
				common->Printf( "DC_END_FRAME\n" );
			}
			return true;
		}
		default:
			common->Error( "Bad demo render command '%d' in demo stream", dc );
			break;
	}

	return false;
}

/*
================
WriteLoadMap
================
*/
void	idRenderWorldLocal::WriteLoadMap()
{

	// only the main renderWorld writes stuff to demos, not the wipes or
	// menu renders
	if( this != common->RW() )
	{
		return;
	}
	idDemoFile* f = common->WriteDemo();
	f->WriteInt( DS_RENDER );
	f->WriteInt( DC_LOADMAP );

	demoHeader_t	header;
	strncpy( header.mapname, mapName.c_str(), sizeof( header.mapname ) - 1 );
	header.version = 4;
	header.sizeofRenderEntity = sizeof( renderEntity_t );
	header.sizeofRenderLight = sizeof( renderLight_t );
	f->WriteInt( header.version );
	f->WriteInt( header.sizeofRenderEntity );
	f->WriteInt( header.sizeofRenderLight );
	for( int i = 0; i < 256; i++ )
	{
		f->WriteChar( header.mapname[ i ] );
	}

	if( r_showDemo.GetBool() )
	{
		common->Printf( "write DC_LOADMAP: %s\n", mapName.c_str() );
	}

	//////////////////////////////////////////////////////////////////////////

	WriteDeclCache( f, DS_RENDER, DC_CACHE_SKINS, DECL_SKIN );

	if( r_showDemo.GetBool() )
	{
		common->Printf( "write DC_CACHESKINS: %s\n", mapName.c_str() );
	}

	WriteDeclCache( f, DS_RENDER, DC_CACHE_PARTICLES, DECL_PARTICLE );

	if( r_showDemo.GetBool() )
	{
		common->Printf( "write DC_CACHEPARTICLES: %s\n", mapName.c_str() );
	}

	WriteDeclCache( f, DS_RENDER, DC_CACHE_MATERIALS, DECL_MATERIAL );

	if( r_showDemo.GetBool() )
	{
		common->Printf( "write DC_CACHEPARTICLES: %s\n", mapName.c_str() );
	}

	for( int i = 0; i < lightDefs.Num(); i++ )
	{
		idRenderLightLocal* light = lightDefs[ i ];
		if( light )
		{
			WriteRenderLight( f, light->index, &light->parms );
		}
	}

	for( int i = 0; i < entityDefs.Num(); i++ )
	{
		if( entityDefs[ i ] )
		{
			WriteRenderEntity( f, entityDefs[ i ] );
		}
	}

	if( r_showDemo.GetBool() )
	{
		common->Printf( "write DC_CACHESKIN: %s\n", mapName.c_str() );
	}
}

/*
================
WriteVisibleDefs

================
*/
void	idRenderWorldLocal::WriteVisibleDefs( const viewDef_t* viewDef )
{
	// only the main renderWorld writes stuff to demos, not the wipes or
	// menu renders
	if( this != common->RW() )
	{
		return;
	}

	// make sure all necessary entities and lights are updated
	for( viewEntity_t* viewEnt = viewDef->viewEntitys ; viewEnt ; viewEnt = viewEnt->next )
	{
		idRenderEntityLocal* ent = viewEnt->entityDef;

		if( !ent || ent->archived )
		{
			// still up to date
			continue;
		}
		// write it out
		WriteRenderEntity( common->WriteDemo(), ent );
		ent->archived = true;
	}

	for( viewLight_t* viewLight = viewDef->viewLights ; viewLight ; viewLight = viewLight->next )
	{
		idRenderLightLocal* light = viewLight->lightDef;

		if( light->archived )
		{
			// still up to date
			continue;
		}
		// write it out
		WriteRenderLight( common->WriteDemo(), light->index, &light->parms );
		light->archived = true;
	}
}


/*
================
WriteRenderView
================
*/
void	idRenderWorldLocal::WriteRenderView( const renderView_t* renderView )
{
	int i;

	// only the main renderWorld writes stuff to demos, not the wipes or
	// menu renders
	if( this != common->RW() )
	{
		return;
	}

	// write the actual view command
	common->WriteDemo()->WriteInt( DS_RENDER );
	common->WriteDemo()->WriteInt( DC_RENDERVIEW );
	common->WriteDemo()->WriteInt( renderView->viewID );
	common->WriteDemo()->WriteFloat( renderView->fov_x );
	common->WriteDemo()->WriteFloat( renderView->fov_y );
	common->WriteDemo()->WriteVec3( renderView->vieworg );
	common->WriteDemo()->WriteMat3( renderView->viewaxis );
	common->WriteDemo()->WriteBool( renderView->cramZNear );
	common->WriteDemo()->WriteBool( renderView->forceUpdate );
	// binary compatibility with old win32 version writing padded structures directly to disk
	common->WriteDemo()->WriteUnsignedChar( 0 );
	common->WriteDemo()->WriteUnsignedChar( 0 );
	common->WriteDemo()->WriteInt( renderView->time[0] );
	common->WriteDemo()->WriteInt( renderView->time[1] );
	for( i = 0; i < MAX_GLOBAL_SHADER_PARMS; i++ )
	{
		common->WriteDemo()->WriteFloat( renderView->shaderParms[i] );
	}
	common->WriteDemo()->WriteInt( ( int& )renderView->globalMaterial );

	if( r_showDemo.GetBool() )
	{
		// foresthale 2014-05-19: /analyze fix - was time, changed to time[0]
		common->Printf( "write DC_RENDERVIEW: %i\n", renderView->time[0] );
	}
}

/*
================
WriteFreeEntity
================
*/
void	idRenderWorldLocal::WriteFreeEntity( qhandle_t handle )
{

	// only the main renderWorld writes stuff to demos, not the wipes or
	// menu renders
	if( this != common->RW() )
	{
		return;
	}

	common->WriteDemo()->WriteInt( DS_RENDER );
	common->WriteDemo()->WriteInt( DC_DELETE_ENTITYDEF );
	common->WriteDemo()->WriteInt( handle );

	if( r_showDemo.GetBool() )
	{
		common->Printf( "write DC_DELETE_ENTITYDEF: %i\n", handle );
	}
}

/*
================
WriteFreeLightEntity
================
*/
void	idRenderWorldLocal::WriteFreeLight( qhandle_t handle )
{

	// only the main renderWorld writes stuff to demos, not the wipes or
	// menu renders
	if( this != common->RW() )
	{
		return;
	}

	common->WriteDemo()->WriteInt( DS_RENDER );
	common->WriteDemo()->WriteInt( DC_DELETE_LIGHTDEF );
	common->WriteDemo()->WriteInt( handle );

	if( r_showDemo.GetBool() )
	{
		common->Printf( "write DC_DELETE_LIGHTDEF: %i\n", handle );
	}
}

/*
================
WriteRenderLight
================
*/
void	idRenderWorldLocal::WriteRenderLight( idDemoFile* f, qhandle_t handle, const renderLight_t* light )
{

	// only the main renderWorld writes stuff to demos, not the wipes or
	// menu renders
	if( this != common->RW() )
	{
		return;
	}

	f->WriteInt( DS_RENDER );
	f->WriteInt( DC_UPDATE_LIGHTDEF );
	f->WriteInt( handle );

	f->WriteMat3( light->axis );
	f->WriteVec3( light->origin );
	f->WriteInt( light->suppressLightInViewID );
	f->WriteInt( light->allowLightInViewID );
	f->WriteBool( light->noShadows );
	f->WriteBool( light->noSpecular );
	f->WriteBool( light->pointLight );
	f->WriteBool( light->parallel );
	f->WriteVec3( light->lightRadius );
	f->WriteVec3( light->lightCenter );
	f->WriteVec3( light->target );
	f->WriteVec3( light->right );
	f->WriteVec3( light->up );
	f->WriteVec3( light->start );
	f->WriteVec3( light->end );
	f->WriteInt( light->lightId );
	f->WriteInt( ( int& )light->prelightModel );
	f->WriteInt( ( int& )light->shader );
	for( int i = 0; i < MAX_ENTITY_SHADER_PARMS; i++ )
	{
		f->WriteFloat( light->shaderParms[ i ] );
	}
	f->WriteInt( ( int& )light->referenceSound );

	if( light->prelightModel )
	{
		f->WriteInt( 1 );
		f->WriteHashString( light->prelightModel->Name() );
	}
	else
	{
		f->WriteInt( 0 );
	}
	if( light->shader )
	{
		f->WriteInt( 1 );
		f->WriteHashString( light->shader->GetName() );
	}
	else
	{
		f->WriteInt( 0 );
	}
	if( light->referenceSound )
	{
		int	index = light->referenceSound->Index();
		f->WriteInt( 1 );
		f->WriteInt( index );
	}
	else
	{
		f->WriteInt( 0 );
	}

	if( r_showDemo.GetBool() )
	{
		common->Printf( "write DC_UPDATE_LIGHTDEF: %i\n", handle );
	}
}

/*
================
ReadRenderLight
================
*/
void	idRenderWorldLocal::ReadRenderLight()
{
	renderLight_t	light;
	int				index, i;

	common->ReadDemo()->ReadInt( index );
	if( index < 0 )
	{
		common->Error( "ReadRenderLight: index < 0 " );
	}

	if( r_showDemo.GetBool() )
	{
		common->Printf( "DC_UPDATE_LIGHTDEF: init %i\n", index );
	}
	/* Initialize Pointers */
	light.prelightModel = NULL;
	light.shader = NULL;
	light.referenceSound = NULL;

	common->ReadDemo()->ReadMat3( light.axis );
	common->ReadDemo()->ReadVec3( light.origin );
	common->ReadDemo()->ReadInt( light.suppressLightInViewID );
	common->ReadDemo()->ReadInt( light.allowLightInViewID );
	common->ReadDemo()->ReadBool( light.noShadows );
	common->ReadDemo()->ReadBool( light.noSpecular );
	common->ReadDemo()->ReadBool( light.pointLight );
	common->ReadDemo()->ReadBool( light.parallel );
	common->ReadDemo()->ReadVec3( light.lightRadius );
	common->ReadDemo()->ReadVec3( light.lightCenter );
	common->ReadDemo()->ReadVec3( light.target );
	common->ReadDemo()->ReadVec3( light.right );
	common->ReadDemo()->ReadVec3( light.up );
	common->ReadDemo()->ReadVec3( light.start );
	common->ReadDemo()->ReadVec3( light.end );
	common->ReadDemo()->ReadInt( light.lightId );
	common->ReadDemo()->ReadInt( ( int& )light.prelightModel );
	common->ReadDemo()->ReadInt( ( int& )light.shader );
	for( int i = 0; i < MAX_ENTITY_SHADER_PARMS; i++ )
	{
		common->ReadDemo()->ReadFloat( light.shaderParms[ i ] );
	}
	common->ReadDemo()->ReadInt( ( int& )light.referenceSound );

	common->ReadDemo()->ReadInt( i );
	if( i )
	{
		light.prelightModel = renderModelManager->FindModel( common->ReadDemo()->ReadHashString() );
	}
	common->ReadDemo()->ReadInt( i );
	if( i )
	{
		light.shader = declManager->FindMaterial( common->ReadDemo()->ReadHashString() );
	}

	common->ReadDemo()->ReadInt( i );
	if( i )
	{
		int	index;
		common->ReadDemo()->ReadInt( index );
		light.referenceSound = common->SW()->EmitterForIndex( index );
	}

	UpdateLightDef( index, &light );

	if( r_showDemo.GetBool() )
	{
		common->Printf( "DC_UPDATE_LIGHTDEF: %i\n", index );
	}
}

/*
================
WriteRenderEntity
================
*/
void	idRenderWorldLocal::WriteRenderEntity( idDemoFile* f, idRenderEntityLocal* entity )
{
	// only the main renderWorld writes stuff to demos, not the wipes or
	// menu renders
	if( this != common->RW() )
	{
		return;
	}

	if( entity->decals && entity->decals->demoSerialCurrent != entity->decals->demoSerialWrite )
	{
		entity->decals->demoSerialWrite = entity->decals->demoSerialCurrent;
		WriteRenderDecal( f, entity->decals->index );
	}

	if( entity->overlays && entity->overlays->demoSerialCurrent != entity->overlays->demoSerialWrite )
	{
		entity->overlays->demoSerialWrite = entity->overlays->demoSerialCurrent;
		WriteRenderOverlay( f, entity->overlays->index );
	}

	f->WriteInt( DS_RENDER );
	f->WriteInt( DC_UPDATE_ENTITYDEF );
	f->WriteInt( entity->index );
	entity->WriteToDemoFile( f );

	// write decal ref
	if( entity->decals )
	{
		f->WriteBool( true );
		f->WriteInt( entity->decals->index );
	}
	else
	{
		f->WriteBool( false );
	}

	// write overlay ref
	if( entity->overlays )
	{
		f->WriteBool( true );
		f->WriteInt( entity->overlays->index );
	}
	else
	{
		f->WriteBool( false );
	}
}


/*
================
ReadRenderEntity
================
*/
void idRenderWorldLocal::ReadRenderEntity()
{
	renderEntity_t ent;
	int index;

	common->ReadDemo()->ReadInt( index );
	//tr.pc.c_entityUpdates++;
	while( index >= entityDefs.Num() )
	{
		entityDefs.Append( NULL );
	}

	idRenderEntityLocal* def = entityDefs[ index ];
	if( def == NULL )
	{
		def = new( TAG_RENDER_ENTITY )idRenderEntityLocal;
		def->world = this;
		def->index = index;
		entityDefs[ index ] = def;
	}
	def->ReadFromDemoFile( common->ReadDemo() );

	// decals
	bool hasDecal = false, hasOverlay = false;

	common->ReadDemo()->ReadBool( hasDecal );
	if( hasDecal )
	{
		int index = 0;
		common->ReadDemo()->ReadInt( index );

		if( r_writeDemoDecals.GetBool() )
		{
			def->decals = decals[ index ].decals;
		}
	}

	common->ReadDemo()->ReadBool( hasOverlay );
	if( hasOverlay )
	{
		int index = 0;
		common->ReadDemo()->ReadInt( index );

		if( r_writeDemoOverlays.GetBool() )
		{
			def->overlays = overlays[ index ].overlays;
		}
	}
}

void idRenderWorldLocal::WriteRenderDecal( idDemoFile* f, qhandle_t handle )
{
	// only the main renderWorld writes stuff to demos, not the wipes or
	// menu renders
	if( this != common->RW() )
	{
		return;
	}

	if( handle < 0 || !f )
	{
		return;
	}
	if( !r_writeDemoDecals.GetBool() )
	{
		return;
	}

	// actually update the decal.
	f->WriteInt( DS_RENDER );
	f->WriteInt( DC_UPDATE_DECAL );
	f->WriteInt( handle );
	f->WriteInt( decals[ handle ].entityHandle );
	f->WriteInt( decals[ handle ].lastStartTime );
	decals[ handle ].decals->WriteToDemoFile( f );
}

void idRenderWorldLocal::WriteFreeDecal( idDemoFile* f, qhandle_t handle )
{
	// only the main renderWorld writes stuff to demos, not the wipes or
	// menu renders
	if( this != common->RW() )
	{
		return;
	}

	if( !r_writeDemoDecals.GetBool() )
	{
		return;
	}

	// When Decals are Freed, all that really happens is they get reallocated.
	f->WriteInt( DS_RENDER );
	f->WriteInt( DC_DELETE_DECAL );
	f->WriteInt( handle );
	f->WriteInt( decals[ handle ].entityHandle );
	f->WriteInt( decals[ handle ].lastStartTime );

	if( r_showDemo.GetBool() )
	{
		common->Printf( "write DC_DELETE_DECAL: %i\n", handle );
	}
}

void idRenderWorldLocal::WriteRenderOverlay( idDemoFile* f, qhandle_t handle )
{
	// only the main renderWorld writes stuff to demos, not the wipes or
	// menu renders
	if( this != common->RW() )
	{
		return;
	}

	if( handle < 0 || !f || !r_writeDemoOverlays.GetBool() )
	{
		return;
	}

	// actually update the decal.
	f->WriteInt( DS_RENDER );
	f->WriteInt( DC_UPDATE_OVERLAY );
	f->WriteInt( handle );
	f->WriteInt( overlays[ handle ].entityHandle );
	f->WriteInt( overlays[ handle ].lastStartTime );
	overlays[ handle ].overlays->WriteToDemoFile( f );

	if( r_showDemo.GetBool() )
	{
		common->Printf( "write DC_UPDATE_OVERLAY: %i\n", handle );
	}
}

void idRenderWorldLocal::WriteFreeOverlay( idDemoFile* f, qhandle_t handle )
{
	// only the main renderWorld writes stuff to demos, not the wipes or
	// menu renders
	if( this != common->RW() )
	{
		return;
	}

	if( !r_writeDemoOverlays.GetBool() )
	{
		return;
	}

	// When Decals are Freed, all that really happens is they get reallocated.
	f->WriteInt( DS_RENDER );
	f->WriteInt( DC_DELETE_OVERLAY );
	f->WriteInt( handle );
	f->WriteInt( overlays[ handle ].entityHandle );
	f->WriteInt( overlays[ handle ].lastStartTime );

	if( r_showDemo.GetBool() )
	{
		common->Printf( "write DC_DELETE_OVERLAY: %i\n", handle );
	}
}


// RB begin
void	idRenderWorldLocal::WriteFreeEnvprobe( qhandle_t handle )
{

	// only the main renderWorld writes stuff to demos, not the wipes or
	// menu renders
	if( this != common->RW() )
	{
		return;
	}

	common->WriteDemo()->WriteInt( DS_RENDER );
	common->WriteDemo()->WriteInt( DC_DELETE_ENVPROBEDEF );
	common->WriteDemo()->WriteInt( handle );

	if( r_showDemo.GetBool() )
	{
		common->Printf( "write DC_DELETE_ENVPROBEDEF: %i\n", handle );
	}
}
// RB end