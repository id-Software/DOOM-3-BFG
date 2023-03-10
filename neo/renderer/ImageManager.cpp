/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
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

#include "precompiled.h"
#pragma hdrstop


#include "RenderCommon.h"
#include <sys/DeviceManager.h>

// do this with a pointer, in case we want to make the actual manager
// a private virtual subclass
idImageManager	imageManager;
idImageManager* globalImages = &imageManager;

extern DeviceManager* deviceManager;
extern idCVar r_uploadBufferSizeMB;

idCVar preLoad_Images( "preLoad_Images", "1", CVAR_SYSTEM | CVAR_BOOL, "preload images during beginlevelload" );

/*
===============
R_ReloadImages_f

Regenerate all images that came directly from files that have changed, so
any saved changes will show up in place.

New r_texturesize/r_texturedepth variables will take effect on reload

reloadImages <all>
===============
*/
void R_ReloadImages_f( const idCmdArgs& args )
{
	bool all = false;

	if( args.Argc() == 2 )
	{
		if( !idStr::Icmp( args.Argv( 1 ), "all" ) )
		{
			all = true;
		}
		else
		{
			idLib::Printf( "USAGE: reloadImages <all>\n" );
			return;
		}
	}

	tr.commandList->open();
	globalImages->ReloadImages( all, tr.commandList );
	tr.commandList->close();
	deviceManager->GetDevice()->executeCommandList( tr.commandList );

	// Images (including the framebuffer images) were reloaded, reinitialize the framebuffers.
	Framebuffer::ResizeFramebuffers();
}

typedef struct
{
	idImage*	image;
	int		size;
	int		index;
} sortedImage_t;

/*
=======================
R_QsortImageSizes
=======================
*/
static int R_QsortImageSizes( const void* a, const void* b )
{
	const sortedImage_t*	ea, *eb;

	ea = ( sortedImage_t* )a;
	eb = ( sortedImage_t* )b;

	if( ea->size > eb->size )
	{
		return -1;
	}
	if( ea->size < eb->size )
	{
		return 1;
	}
	return idStr::Icmp( ea->image->GetName(), eb->image->GetName() );
}

/*
=======================
R_QsortImageName
=======================
*/
static int R_QsortImageName( const void* a, const void* b )
{
	const sortedImage_t*	ea, *eb;

	ea = ( sortedImage_t* )a;
	eb = ( sortedImage_t* )b;

	return idStr::Icmp( ea->image->GetName(), eb->image->GetName() );
}

/*
===============
R_ListImages_f
===============
*/
void R_ListImages_f( const idCmdArgs& args )
{
	int		i, partialSize;
	idImage*	image;
	int		totalSize;
	int		count = 0;
	bool	uncompressedOnly = false;
	bool	unloaded = false;
	bool	failed = false;
	bool	sorted = false;
	bool	duplicated = false;
	bool	overSized = false;
	bool	sortByName = false;

	if( args.Argc() == 1 )
	{

	}
	else if( args.Argc() == 2 )
	{
		if( idStr::Icmp( args.Argv( 1 ), "uncompressed" ) == 0 )
		{
			uncompressedOnly = true;
		}
		else if( idStr::Icmp( args.Argv( 1 ), "sorted" ) == 0 )
		{
			sorted = true;
		}
		else if( idStr::Icmp( args.Argv( 1 ), "namesort" ) == 0 )
		{
			sortByName = true;
		}
		else if( idStr::Icmp( args.Argv( 1 ), "unloaded" ) == 0 )
		{
			unloaded = true;
		}
		else if( idStr::Icmp( args.Argv( 1 ), "duplicated" ) == 0 )
		{
			duplicated = true;
		}
		else if( idStr::Icmp( args.Argv( 1 ), "oversized" ) == 0 )
		{
			sorted = true;
			overSized = true;
		}
		else
		{
			failed = true;
		}
	}
	else
	{
		failed = true;
	}

	if( failed )
	{
		idLib::Printf( "usage: listImages [ sorted | namesort | unloaded | duplicated | showOverSized ]\n" );
		return;
	}

	const char* header = "       -w-- -h-- filt -fmt-- wrap  size --name-------\n";
	idLib::Printf( "\n%s", header );

	totalSize = 0;

	idList< idImage* >& images = globalImages->images;
	const int numImages = images.Num();

	sortedImage_t* sortedArray = ( sortedImage_t* )alloca( sizeof( sortedImage_t ) * numImages );

	for( i = 0 ; i < numImages; i++ )
	{
		image = images[ i ];

		if( uncompressedOnly )
		{
			if( image->IsCompressed() )
			{
				continue;
			}
		}
		if( unloaded == image->IsLoaded() )
		{
			continue;
		}

		// only print duplicates (from mismatched wrap / clamp, etc)
		if( duplicated )
		{
			int j;
			for( j = i + 1 ; j < numImages ; j++ )
			{
				if( idStr::Icmp( image->GetName(), images[ j ]->GetName() ) == 0 )
				{
					break;
				}
			}
			if( j == numImages )
			{
				continue;
			}
		}

		if( sorted || sortByName )
		{
			sortedArray[count].image = image;
			sortedArray[count].size = image->StorageSize();
			sortedArray[count].index = i;
		}
		else
		{
			idLib::Printf( "%4i:",	i );
			image->Print();
		}
		totalSize += image->StorageSize();
		count++;
	}

	if( sorted || sortByName )
	{
		if( sortByName )
		{
			qsort( sortedArray, count, sizeof( sortedImage_t ), R_QsortImageName );
		}
		else
		{
			qsort( sortedArray, count, sizeof( sortedImage_t ), R_QsortImageSizes );
		}
		partialSize = 0;
		for( i = 0 ; i < count ; i++ )
		{
			idLib::Printf( "%4i:",	sortedArray[i].index );
			sortedArray[i].image->Print();
			partialSize += sortedArray[i].image->StorageSize();
			if( ( ( i + 1 ) % 10 ) == 0 )
			{
				idLib::Printf( "-------- %5.1f of %5.1f megs --------\n",
							   partialSize / ( 1024 * 1024.0 ), totalSize / ( 1024 * 1024.0 ) );
			}
		}
	}

	idLib::Printf( "%s", header );
	idLib::Printf( " %i images (%i total)\n", count, numImages );
	idLib::Printf( " %5.1f total megabytes of images\n\n\n", totalSize / ( 1024 * 1024.0 ) );
}

/*
==============
AllocImage

Allocates an idImage, adds it to the list,
copies the name, and adds it to the hash chain.
==============
*/
idImage* idImageManager::AllocImage( const char* name )
{
	if( strlen( name ) >= MAX_IMAGE_NAME )
	{
		idLib::Error( "idImageManager::AllocImage: \"%s\" is too long\n", name );
	}

	int hash = idStr( name ).FileNameHash();

	idImage* image = new( TAG_IMAGE ) idImage( name );

	imageHash.Add( hash, images.Append( image ) );

	return image;
}

/*
==============
AllocStandaloneImage

Allocates an idImage,does not add it to the list or hash chain

==============
*/
idImage* idImageManager::AllocStandaloneImage( const char* name )
{
	if( strlen( name ) >= MAX_IMAGE_NAME )
	{
		common->Error( "idImageManager::AllocImage: \"%s\" is too long\n", name );
	}

	idImage* image = new( TAG_IMAGE ) idImage( name );

	return image;
}

/*
==============
AllocDeferredImage

Allocates an idDeferredImage to load images from memory, adds it to the hash chain

==============
*/
idDeferredImage* idImageManager::AllocDeferredImage( const char* name )
{
	idDeferredImage* image = new( TAG_IMAGE ) idDeferredImage( name );

	int hash = idStr( name ).FileNameHash();
	deferredImageHash.Add( hash, deferredImages.Append( image ) );

	return image;
}

/*
==================
ImageFromFunction

Images that are procedurally generated are allways specified
with a callback which must work at any time, allowing the OpenGL
system to be completely regenerated if needed.
==================
*/
idImage* idImageManager::ImageFromFunction( const char* _name, ImageGeneratorFunction generatorFunction )
{

	// strip any .tga file extensions from anywhere in the _name
	idStr name = _name;
	name.Replace( ".tga", "" );
	name.BackSlashesToSlashes();

	// see if the image already exists
	int hash = name.FileNameHash();
	for( int i = imageHash.First( hash ); i != -1; i = imageHash.Next( i ) )
	{
		idImage* image = images[i];
		if( name.Icmp( image->GetName() ) == 0 )
		{
			if( image->generatorFunction != generatorFunction )
			{
				common->DPrintf( "WARNING: reused image %s with mixed generators\n", name.c_str() );
			}
			return image;
		}
	}

	// create the image and issue the callback
	idImage*	 image = AllocImage( name );

	image->generatorFunction = generatorFunction;

	// check for precompressed, load is from the front end
	image->referencedOutsideLevelLoad = true;

	return image;
}

/*
===============
ImageFromFile

Finds or loads the given image, always returning a valid image pointer.
Loading of the image may be deferred for dynamic loading.
==============
*/
idImage*	idImageManager::ImageFromFile( const char* _name, textureFilter_t filter,
		textureRepeat_t repeat, textureUsage_t usage, cubeFiles_t cubeMap, int cubeMapSize )
{

	if( !_name || !_name[0] || idStr::Icmp( _name, "default" ) == 0 || idStr::Icmp( _name, "_default" ) == 0 )
	{
		declManager->MediaPrint( "DEFAULTED\n" );
		return globalImages->defaultImage;
	}
	if( idStr::Icmpn( _name, "fonts", 5 ) == 0 || idStr::Icmpn( _name, "newfonts", 8 ) == 0 )
	{
		usage = TD_FONT;
	}
	if( idStr::Icmpn( _name, "lights", 6 ) == 0 )
	{
		usage = TD_LIGHT;
	}

	// strip any .tga file extensions from anywhere in the _name, including image program parameters
	idStrStatic< MAX_OSPATH > name = _name;
	name.Replace( ".tga", "" );
	name.BackSlashesToSlashes();

	//
	// see if the image is already loaded, unless we
	// are in a reloadImages call
	//
	int hash = name.FileNameHash();
	for( int i = imageHash.First( hash ); i != -1; i = imageHash.Next( i ) )
	{
		idImage*	 image = images[i];
		if( name.Icmp( image->GetName() ) == 0 )
		{
			// the built in's, like _white and _flat always match the other options
			if( name[0] == '_' )
			{
				return image;
			}
			if( image->cubeFiles != cubeMap )
			{
				idLib::Error( "Image '%s' has been referenced with conflicting cube map states", _name );
			}

			if( image->filter != filter || image->repeat != repeat )
			{
				// we might want to have the system reset these parameters on every bind and
				// share the image data
				continue;
			}
			if( image->usage != usage )
			{
				// If an image is used differently then we need 2 copies of it because usage affects the way it's compressed and swizzled
				continue;
			}

			image->usage = usage;
			image->levelLoadReferenced = true;

			if( ( !insideLevelLoad  || preloadingMapImages ) && !image->IsLoaded() )
			{
				image->referencedOutsideLevelLoad = ( !insideLevelLoad && !preloadingMapImages );

				image->FinalizeImage( false, nullptr );

				declManager->MediaPrint( "%ix%i %s (reload for mixed referneces)\n", image->GetUploadWidth(), image->GetUploadHeight(), image->GetName() );
			}
			return image;
		}
	}

	//
	// create a new image
	//
	idImage* image = AllocImage( name );
	image->cubeFiles = cubeMap;
	image->cubeMapSize = cubeMapSize;
	image->usage = usage;
	image->filter = filter;
	image->repeat = repeat;

	image->levelLoadReferenced = true;

	// load it if we aren't in a level preload
	if( !insideLevelLoad || preloadingMapImages )
	{
		image->referencedOutsideLevelLoad = ( !insideLevelLoad && !preloadingMapImages );
		image->FinalizeImage( false, nullptr );

		declManager->MediaPrint( "%ix%i %s\n", image->GetUploadWidth(), image->GetUploadHeight(), image->GetName() );
	}
	else
	{
		declManager->MediaPrint( "%s\n", image->GetName() );
	}

	return image;
}

/*
========================
idImageManager::ScratchImage
========================
*/
idImage* idImageManager::ScratchImage( const char* _name, idImageOpts* imgOpts, textureFilter_t filter, textureRepeat_t repeat, textureUsage_t usage )
{
	if( !_name || !_name[0] )
	{
		idLib::FatalError( "idImageManager::ScratchImage called with empty name" );
	}

	if( imgOpts == NULL )
	{
		idLib::FatalError( "idImageManager::ScratchImage called with NULL imgOpts" );
	}

	idStr name = _name;

	//
	// see if the image is already loaded, unless we
	// are in a reloadImages call
	//
	int hash = name.FileNameHash();
	for( int i = imageHash.First( hash ); i != -1; i = imageHash.Next( i ) )
	{
		idImage*	 image = images[i];
		if( name.Icmp( image->GetName() ) == 0 )
		{
			// the built in's, like _white and _flat always match the other options
			if( name[0] == '_' )
			{
				return image;
			}

			if( image->filter != filter || image->repeat != repeat )
			{
				// we might want to have the system reset these parameters on every bind and
				// share the image data
				continue;
			}
			if( image->usage != usage )
			{
				// If an image is used differently then we need 2 copies of it because usage affects the way it's compressed and swizzled
				continue;
			}

			image->usage = usage;
			image->levelLoadReferenced = true;
			image->referencedOutsideLevelLoad = true;
			return image;
		}
	}

	// clamp is the only repeat mode that makes sense for cube maps, but
	// some platforms let them stay in repeat mode and get border seam issues
	if( imgOpts->textureType == TT_CUBIC && repeat != TR_CLAMP )
	{
		repeat = TR_CLAMP;
	}

	//
	// create a new image
	//
	idImage* newImage = AllocImage( name );
	if( newImage != NULL )
	{
		newImage->AllocImage( *imgOpts, filter, repeat );
	}
	return newImage;
}

/*
===============
idImageManager::ScratchImage
===============
*/
idImage* idImageManager::ScratchImage( const char* name, const idImageOpts& opts )
{
	if( !name || !name[0] )
	{
		idLib::FatalError( "idImageManager::ScratchImage" );
	}

	idImage* image = GetImage( name );
	if( image == NULL )
	{
		image = AllocImage( name );
	}
	else
	{
		image->PurgeImage();
	}

	image->opts = opts;
	image->AllocImage();
	image->referencedOutsideLevelLoad = true;

	return image;
}

/*
===============
idImageManager::GetImage
===============
*/
idImage* idImageManager::GetImage( const char* _name ) const
{

	if( !_name || !_name[0] || idStr::Icmp( _name, "default" ) == 0 || idStr::Icmp( _name, "_default" ) == 0 )
	{
		declManager->MediaPrint( "DEFAULTED\n" );
		return globalImages->defaultImage;
	}

	// strip any .tga file extensions from anywhere in the _name, including image program parameters
	idStr name = _name;
	name.Replace( ".tga", "" );
	name.BackSlashesToSlashes();

	//
	// look in loaded images
	//
	int hash = name.FileNameHash();
	for( int i = imageHash.First( hash ); i != -1; i = imageHash.Next( i ) )
	{
		idImage* image = images[i];
		if( name.Icmp( image->GetName() ) == 0 )
		{
			return image;
		}
	}

	return NULL;
}

/*
===============
PurgeAllImages
===============
*/
void idImageManager::PurgeAllImages()
{
	for( int i = 0; i < images.Num() ; i++ )
	{
		images[ i ]->PurgeImage();
	}
}

/*
===============
ReloadImages
===============
*/
void idImageManager::ReloadImages( bool all, nvrhi::ICommandList* commandList )
{
	for( int i = 0 ; i < images.Num() ; i++ )
	{
		images[ i ]->Reload( all, commandList );
	}

	LoadDeferredImages( commandList );
}

/*
===============
R_CombineCubeImages_f

Used to combine animations of six separate tga files into
a serials of 6x taller tga files, for preparation to roq compress
===============
*/
void R_CombineCubeImages_f( const idCmdArgs& args )
{
	if( args.Argc() != 2 )
	{
		idLib::Printf( "usage: combineCubeImages <baseName>\n" );
		idLib::Printf( " combines basename[1-6][0001-9999].tga to basenameCM[0001-9999].tga\n" );
		idLib::Printf( " 1: forward 2:right 3:back 4:left 5:up 6:down\n" );
		return;
	}

	idStr	baseName = args.Argv( 1 );
	common->SetRefreshOnPrint( true );

	for( int frameNum = 1 ; frameNum < 10000 ; frameNum++ )
	{
		char	filename[MAX_IMAGE_NAME];
		byte*	pics[6];
		int		width = 0, height = 0;
		int		side;
		int		orderRemap[6] = { 1, 3, 4, 2, 5, 6 };
		for( side = 0 ; side < 6 ; side++ )
		{
			sprintf( filename, "%s%i%04i.tga", baseName.c_str(), orderRemap[side], frameNum );

			idLib::Printf( "reading %s\n", filename );
			R_LoadImage( filename, &pics[side], &width, &height, NULL, true, NULL );

			if( !pics[side] )
			{
				idLib::Printf( "not found.\n" );
				break;
			}

			// convert from "camera" images to native cube map images
			switch( side )
			{
				case 0:	// forward
					R_RotatePic( pics[side], width );
					break;
				case 1:	// back
					R_RotatePic( pics[side], width );
					R_HorizontalFlip( pics[side], width, height );
					R_VerticalFlip( pics[side], width, height );
					break;
				case 2:	// left
					R_VerticalFlip( pics[side], width, height );
					break;
				case 3:	// right
					R_HorizontalFlip( pics[side], width, height );
					break;
				case 4:	// up
					R_RotatePic( pics[side], width );
					break;
				case 5: // down
					R_RotatePic( pics[side], width );
					break;
			}
		}

		if( side != 6 )
		{
			for( int i = 0 ; i < side ; side++ )
			{
				Mem_Free( pics[side] );
			}
			break;
		}

		idTempArray<byte> buf( width * height * 6 * 4 );
		byte*	combined = ( byte* )buf.Ptr();
		for( side = 0 ; side < 6 ; side++ )
		{
			memcpy( combined + width * height * 4 * side, pics[side], width * height * 4 );
			Mem_Free( pics[side] );
		}
		sprintf( filename, "%sCM%04i.tga", baseName.c_str(), frameNum );

		idLib::Printf( "writing %s\n", filename );
		R_WriteTGA( filename, combined, width, height * 6 );
	}
	common->SetRefreshOnPrint( false );
}

/*
===============
Init
===============
*/
void idImageManager::Init()
{
	images.Resize( 1024, 1024 );
	imageHash.ResizeIndex( 1024 );

	CreateIntrinsicImages();

	cmdSystem->AddCommand( "reloadImages", R_ReloadImages_f, CMD_FL_RENDERER, "reloads images" );
	cmdSystem->AddCommand( "listImages", R_ListImages_f, CMD_FL_RENDERER, "lists images" );
	cmdSystem->AddCommand( "combineCubeImages", R_CombineCubeImages_f, CMD_FL_RENDERER, "combines six images for roq compression" );

	// should forceLoadImages be here?
	LoadDeferredImages();
}

/*
===============
Shutdown
===============
*/
void idImageManager::Shutdown()
{
	images.DeleteContents( true );
	imageHash.Clear();
	deferredImages.DeleteContents( true );
	deferredImageHash.Clear();
	commandList.Reset();
}

/*
====================
idImageManager::BeginLevelLoad
Frees all images used by the previous level
====================
*/
void idImageManager::BeginLevelLoad()
{
	insideLevelLoad = true;

	for( int i = 0 ; i < images.Num() ; i++ )
	{
		idImage*	image = images[ i ];

		// generator function images are always kept around
		if( image->generatorFunction )
		{
			continue;
		}

		if( !image->referencedOutsideLevelLoad && image->IsLoaded() )
		{
			image->PurgeImage();
			//idLib::Printf( "purging %s\n", image->GetName() );
		}
		else
		{
			//idLib::Printf( "not purging %s\n", image->GetName() );
		}

		image->levelLoadReferenced = false;
	}
}


/*
====================
idImageManager::ExcludePreloadImage
====================
*/
bool idImageManager::ExcludePreloadImage( const char* name )
{
	idStrStatic< MAX_OSPATH > imgName = name;
	imgName.ToLower();
	if( imgName.Find( "newfonts/", false ) >= 0 )
	{
		return true;
	}
	if( imgName.Find( "generated/swf/", false ) >= 0 )
	{
		return true;
	}
	if( imgName.Find( "/loadscreens/", false ) >= 0 )
	{
		return true;
	}
	return false;
}

/*
====================
idImageManager::Preload
====================
*/
void idImageManager::Preload( const idPreloadManifest& manifest, const bool& mapPreload )
{
	if( preLoad_Images.GetBool() && manifest.NumResources() > 0 )
	{
		// preload this levels images
		idLib::Printf( "Preloading images...\n" );
		preloadingMapImages = mapPreload;
		int	start = Sys_Milliseconds();
		int numLoaded = 0;

		//fileSystem->StartPreload( preloadImageFiles );
		for( int i = 0; i < manifest.NumResources(); i++ )
		{
			const preloadEntry_s& p = manifest.GetPreloadByIndex( i );
			if( p.resType == PRELOAD_IMAGE && !ExcludePreloadImage( p.resourceName ) )
			{
				globalImages->ImageFromFile( p.resourceName, ( textureFilter_t )p.imgData.filter, ( textureRepeat_t )p.imgData.repeat, ( textureUsage_t )p.imgData.usage, ( cubeFiles_t )p.imgData.cubeMap );
				numLoaded++;
			}
		}
		//fileSystem->StopPreload();
		int	end = Sys_Milliseconds();
		idLib::Printf( "%05d images preloaded ( or were already loaded ) in %5.1f seconds\n", numLoaded, ( end - start ) * 0.001 );
		idLib::Printf( "----------------------------------------\n" );
		preloadingMapImages = false;
	}
}

/*
===============
idImageManager::LoadLevelImages
===============
*/
int idImageManager::LoadLevelImages( bool pacifier )
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

	common->UpdateLevelLoadPacifier();

	commandList->open();

	int	loadCount = 0;
	for( int i = 0 ; i < images.Num() ; i++ )
	{
		idImage* image = images[ i ];

		if( image->generatorFunction )
		{
			continue;
		}

		if( image->levelLoadReferenced && !image->IsLoaded() )
		{
			loadCount++;
			image->FinalizeImage( false, commandList );
		}
	}

	globalImages->LoadDeferredImages( commandList );

	commandList->close();

	deviceManager->GetDevice()->executeCommandList( commandList );

	common->UpdateLevelLoadPacifier();

	return loadCount;
}

/*
===============
idImageManager::EndLevelLoad
===============
*/
void idImageManager::EndLevelLoad()
{
	insideLevelLoad = false;

	idLib::Printf( "----- idImageManager::EndLevelLoad -----\n" );
	int start = Sys_Milliseconds();
	int	loadCount = LoadLevelImages( true );

	int	end = Sys_Milliseconds();
	idLib::Printf( "%5i images loaded in %5.1f seconds\n", loadCount, ( end - start ) * 0.001 );
	idLib::Printf( "----------------------------------------\n" );
	//R_ListImages_f( idCmdArgs( "sorted sorted", false ) );
}

/*
===============
idImageManager::PrintMemInfo
===============
*/
void idImageManager::PrintMemInfo( MemInfo_t* mi )
{
	int i, j, total = 0;
	int* sortIndex;
	idFile* f;

	f = fileSystem->OpenFileWrite( mi->filebase + "_images.txt" );
	if( !f )
	{
		return;
	}

	// sort first
	sortIndex = new( TAG_IMAGE ) int[images.Num()];

	for( i = 0; i < images.Num(); i++ )
	{
		sortIndex[i] = i;
	}

	for( i = 0; i < images.Num() - 1; i++ )
	{
		for( j = i + 1; j < images.Num(); j++ )
		{
			if( images[sortIndex[i]]->StorageSize() < images[sortIndex[j]]->StorageSize() )
			{
				int temp = sortIndex[i];
				sortIndex[i] = sortIndex[j];
				sortIndex[j] = temp;
			}
		}
	}

	// print next
	for( i = 0; i < images.Num(); i++ )
	{
		idImage* im = images[sortIndex[i]];
		int size;

		size = im->StorageSize();
		total += size;

		f->Printf( "%s %3i %s\n", idStr::FormatNumber( size ).c_str(), im->refCount, im->GetName() );
	}

	delete [] sortIndex;
	mi->imageAssetsTotal = total;

	f->Printf( "\nTotal image bytes allocated: %s\n", idStr::FormatNumber( total ).c_str() );
	fileSystem->CloseFile( f );
}

void idImageManager::LoadDeferredImages( nvrhi::ICommandList* _commandList )
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

	nvrhi::ICommandList* thisCmdList = _commandList;
	if( !_commandList )
	{
		thisCmdList = commandList;
		thisCmdList->open();
	}

	for( int i = 0; i < globalImages->imagesToLoad.Num(); i++ )
	{
		// This is a "deferred" load of textures to the gpu.
		globalImages->imagesToLoad[i]->FinalizeImage( false, thisCmdList );
	}

	if( !_commandList )
	{
		thisCmdList->close();
		deviceManager->GetDevice()->executeCommandList( thisCmdList );
	}

	globalImages->imagesToLoad.Clear();
}
