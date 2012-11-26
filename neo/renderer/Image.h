/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company. 

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

/*
====================================================================

IMAGE

idImage have a one to one correspondance with GL/DX/GCM textures.

No texture is ever used that does not have a corresponding idImage.

====================================================================
*/

static const int	MAX_TEXTURE_LEVELS = 14;

// How is this texture used?  Determines the storage and color format
typedef enum {
	TD_SPECULAR,			// may be compressed, and always zeros the alpha channel
	TD_DIFFUSE,				// may be compressed
	TD_DEFAULT,				// generic RGBA texture (particles, etc...)
	TD_BUMP,				// may be compressed with 8 bit lookup
	TD_FONT,				// Font image
	TD_LIGHT,				// Light image
	TD_LOOKUP_TABLE_MONO,	// Mono lookup table (including alpha)
	TD_LOOKUP_TABLE_ALPHA,	// Alpha lookup table with a white color channel
	TD_LOOKUP_TABLE_RGB1,	// RGB lookup table with a solid white alpha
	TD_LOOKUP_TABLE_RGBA,	// RGBA lookup table
	TD_COVERAGE,			// coverage map for fill depth pass when YCoCG is used
	TD_DEPTH,				// depth buffer copy for motion blur
} textureUsage_t;

typedef enum {
	CF_2D,			// not a cube map
	CF_NATIVE,		// _px, _nx, _py, etc, directly sent to GL
	CF_CAMERA		// _forward, _back, etc, rotated and flipped as needed before sending to GL
} cubeFiles_t;

#include "ImageOpts.h"
#include "BinaryImage.h"

#define	MAX_IMAGE_NAME	256

class idImage {
public:
				idImage( const char * name );

	const char *	GetName() const { return imgName; }

	// Makes this image active on the current GL texture unit.
	// automatically enables or disables cube mapping
	// May perform file loading if the image was not preloaded.
	void		Bind();

	// Should be called at least once
	void		SetSamplerState( textureFilter_t tf, textureRepeat_t tr );

	// used by callback functions to specify the actual data
	// data goes from the bottom to the top line of the image, as OpenGL expects it
	// These perform an implicit Bind() on the current texture unit
	// FIXME: should we implement cinematics this way, instead of with explicit calls?
	void		GenerateImage( const byte *pic, int width, int height, 
					   textureFilter_t filter, textureRepeat_t repeat, textureUsage_t usage );
	void		GenerateCubeImage( const byte *pic[6], int size, 
						textureFilter_t filter, textureUsage_t usage );

	void		CopyFramebuffer( int x, int y, int width, int height );
	void		CopyDepthbuffer( int x, int y, int width, int height );

	void		UploadScratch( const byte *pic, int width, int height );

	// estimates size of the GL image based on dimensions and storage type
	int			StorageSize() const;

	// print a one line summary of the image
	void		Print() const;

	// check for changed timestamp on disk and reload if necessary
	void		Reload( bool force );

	void		AddReference()				{ refCount++; };

	void		MakeDefault();	// fill with a grid pattern

	const idImageOpts &	GetOpts() const { return opts; }
	int			GetUploadWidth() const { return opts.width; }
	int			GetUploadHeight() const { return opts.height; }

	void		SetReferencedOutsideLevelLoad() { referencedOutsideLevelLoad = true; }
	void		SetReferencedInsideLevelLoad() { levelLoadReferenced = true; }
	void		ActuallyLoadImage( bool fromBackEnd );
	//---------------------------------------------
	// Platform specific implementations
	//---------------------------------------------

	void		AllocImage( const idImageOpts &imgOpts, textureFilter_t filter, textureRepeat_t repeat );

	// Deletes the texture object, but leaves the structure so it can be reloaded
	// or resized.
	void		PurgeImage();

	// z is 0 for 2D textures, 0 - 5 for cube maps, and 0 - uploadDepth for 3D textures. Only 
	// one plane at a time of 3D textures can be uploaded. The data is assumed to be correct for 
	// the format, either bytes, halfFloats, floats, or DXT compressed. The data is assumed to 
	// be in OpenGL RGBA format, the consoles may have to reorganize. pixelPitch is only needed 
	// when updating from a source subrect. Width, height, and dest* are always in pixels, so 
	// they must be a multiple of four for dxt data.
	void		SubImageUpload( int mipLevel, int destX, int destY, int destZ, 
								int width, int height, const void * data, 
								int pixelPitch = 0 ) const;

	// SetPixel is assumed to be a fast memory write on consoles, degenerating to a 
	// SubImageUpload on PCs.  Used to update the page mapping images.
	// We could remove this now, because the consoles don't use the intermediate page mapping
	// textures now that they can pack everything into the virtual page table images.
	void		SetPixel( int mipLevel, int x, int y, const void * data, int dataSize );

	// some scratch images are dynamically resized based on the display window size.  This 
	// simply purges the image and recreates it if the sizes are different, so it should not be 
	// done under any normal circumstances, and probably not at all on consoles.
	void		Resize( int width, int height );

	bool		IsCompressed() const { return ( opts.format == FMT_DXT1 || opts.format == FMT_DXT5 ); }

	void		SetTexParameters();	// update aniso and trilinear

	bool		IsLoaded() const { return texnum != TEXTURE_NOT_LOADED; }

	static void			GetGeneratedName( idStr &_name, const textureUsage_t &_usage, const cubeFiles_t &_cube );

private:
	friend class idImageManager;

	void				AllocImage();
	void				DeriveOpts();

	// parameters that define this image
	idStr				imgName;				// game path, including extension (except for cube maps), may be an image program
	cubeFiles_t			cubeFiles;				// If this is a cube map, and if so, what kind
	void				(*generatorFunction)( idImage *image );	// NULL for files
	textureUsage_t		usage;					// Used to determine the type of compression to use
	idImageOpts			opts;					// Parameters that determine the storage method

	// Sampler settings
	textureFilter_t		filter;
	textureRepeat_t		repeat;

	bool				referencedOutsideLevelLoad;
	bool				levelLoadReferenced;	// for determining if it needs to be purged
	bool				defaulted;				// true if the default image was generated because a file couldn't be loaded
	ID_TIME_T			sourceFileTime;			// the most recent of all images used in creation, for reloadImages command
	ID_TIME_T			binaryFileTime;			// the time stamp of the binary file

	int					refCount;				// overall ref count

	static const GLuint TEXTURE_NOT_LOADED = 0xFFFFFFFF;

	GLuint				texnum;				// gl texture binding

	// we could derive these in subImageUpload each time if necessary
	GLuint				internalFormat;
	GLuint				dataFormat;
	GLuint				dataType;


};

ID_INLINE idImage::idImage( const char * name ) : imgName( name ) {
	texnum = TEXTURE_NOT_LOADED;
	internalFormat = 0;
	dataFormat = 0;
	dataType = 0;
	generatorFunction = NULL;
	filter = TF_DEFAULT;
	repeat = TR_REPEAT;
	usage = TD_DEFAULT;
	cubeFiles = CF_2D;

	referencedOutsideLevelLoad = false;
	levelLoadReferenced = false;
	defaulted = false;
	sourceFileTime = FILE_NOT_FOUND_TIMESTAMP;
	binaryFileTime = FILE_NOT_FOUND_TIMESTAMP;
	refCount = 0;
}


// data is RGBA
void	R_WriteTGA( const char *filename, const byte *data, int width, int height, bool flipVertical = false, const char * basePath = "fs_savepath" );
// data is in top-to-bottom raster order unless flipVertical is set



class idImageManager {
public:

	idImageManager() 
	{
		insideLevelLoad = false;
		preloadingMapImages = false;
	}

	void				Init();
	void				Shutdown();

	// If the exact combination of parameters has been asked for already, an existing
	// image will be returned, otherwise a new image will be created.
	// Be careful not to use the same image file with different filter / repeat / etc parameters
	// if possible, because it will cause a second copy to be loaded.
	// If the load fails for any reason, the image will be filled in with the default
	// grid pattern.
	// Will automatically execute image programs if needed.
	idImage *			ImageFromFile( const char *name,
							 textureFilter_t filter, textureRepeat_t repeat, textureUsage_t usage, cubeFiles_t cubeMap = CF_2D );

	// look for a loaded image, whatever the parameters
	idImage *			GetImage( const char *name ) const;

	// look for a loaded image, whatever the parameters
	idImage *			GetImageWithParameters( const char *name, textureFilter_t filter, textureRepeat_t repeat, textureUsage_t usage, cubeFiles_t cubeMap ) const;

	// The callback will be issued immediately, and later if images are reloaded or vid_restart
	// The callback function should call one of the idImage::Generate* functions to fill in the data
	idImage *			ImageFromFunction( const char *name, void (*generatorFunction)( idImage *image ));

	// scratch images are for internal renderer use.  ScratchImage names should always begin with an underscore
	idImage *			ScratchImage( const char *name, idImageOpts *imgOpts, textureFilter_t filter, textureRepeat_t repeat, textureUsage_t usage );

	// purges all the images before a vid_restart
	void				PurgeAllImages();

	// reloads all apropriate images after a vid_restart
	void				ReloadImages( bool all );

	// unbind all textures from all texture units
	void				UnbindAll();

	// disable the active texture unit
	void				BindNull();

	// Called only by renderSystem::BeginLevelLoad
	void				BeginLevelLoad();

	// Called only by renderSystem::EndLevelLoad
	void				EndLevelLoad();

	void				Preload( const idPreloadManifest &manifest, const bool & mapPreload );

	// Loads unloaded level images
	int					LoadLevelImages( bool pacifier );

	// used to clear and then write the dds conversion batch file
	void				StartBuild();
	void				FinishBuild( bool removeDups = false );

	void				PrintMemInfo( MemInfo_t *mi );

	// built-in images
	void CreateIntrinsicImages();
	idImage *			defaultImage;
	idImage *			flatNormalMap;				// 128 128 255 in all pixels
	idImage *			alphaNotchImage;			// 2x1 texture with just 1110 and 1111 with point sampling
	idImage *			whiteImage;					// full of 0xff
	idImage *			blackImage;					// full of 0x00
	idImage *			noFalloffImage;				// all 255, but zero clamped
	idImage *			fogImage;					// increasing alpha is denser fog
	idImage *			fogEnterImage;				// adjust fogImage alpha based on terminator plane
	idImage *			scratchImage;
	idImage *			scratchImage2;
	idImage *			accumImage;
	idImage *			currentRenderImage;				// for SS_POST_PROCESS shaders
	idImage *			currentDepthImage;				// for motion blur
	idImage *			originalCurrentRenderImage;		// currentRenderImage before any changes for stereo rendering
	idImage *			loadingIconImage;				// loading icon must exist always
	idImage *			hellLoadingIconImage;				// loading icon must exist always

	//--------------------------------------------------------
	
	idImage *			AllocImage( const char *name );
	idImage *			AllocStandaloneImage( const char *name );

	bool				ExcludePreloadImage( const char *name );

	idList<idImage*, TAG_IDLIB_LIST_IMAGE>	images;
	idHashIndex			imageHash;

	bool				insideLevelLoad;			// don't actually load images now
	bool				preloadingMapImages;		// unless this is set
};

extern idImageManager	*globalImages;		// pointer to global list for the rest of the system

int MakePowerOfTwo( int num );

/*
====================================================================

IMAGEPROCESS

FIXME: make an "imageBlock" type to hold byte*,width,height?
====================================================================
*/

byte *R_Dropsample( const byte *in, int inwidth, int inheight, int outwidth, int outheight );
byte *R_ResampleTexture( const byte *in, int inwidth, int inheight, int outwidth, int outheight );
byte *R_MipMapWithAlphaSpecularity( const byte *in, int width, int height );
byte *R_MipMapWithGamma( const byte *in, int width, int height );
byte *R_MipMap( const byte *in, int width, int height );

// these operate in-place on the provided pixels
void R_BlendOverTexture( byte *data, int pixelCount, const byte blend[4] );
void R_HorizontalFlip( byte *data, int width, int height );
void R_VerticalFlip( byte *data, int width, int height );
void R_RotatePic( byte *data, int width );

/*
====================================================================

IMAGEFILES

====================================================================
*/

void R_LoadImage( const char *name, byte **pic, int *width, int *height, ID_TIME_T *timestamp, bool makePowerOf2 );
// pic is in top to bottom raster format
bool R_LoadCubeImages( const char *cname, cubeFiles_t extensions, byte *pic[6], int *size, ID_TIME_T *timestamp );

/*
====================================================================

IMAGEPROGRAM

====================================================================
*/

void R_LoadImageProgram( const char *name, byte **pic, int *width, int *height, ID_TIME_T *timestamp, textureUsage_t * usage = NULL );
const char *R_ParsePastImageProgram( idLexer &src );

