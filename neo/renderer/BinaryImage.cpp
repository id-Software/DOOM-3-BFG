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
#pragma hdrstop
#include "precompiled.h"

/*
================================================================================================

	idBinaryImage

================================================================================================
*/

#include "tr_local.h"
#include "DXT/DXTCodec.h"
#include "Color/ColorSpace.h"

idCVar image_highQualityCompression( "image_highQualityCompression", "0", CVAR_BOOL, "Use high quality (slow) compression" );

/*
========================
idBinaryImage::Load2DFromMemory
========================
*/
void idBinaryImage::Load2DFromMemory( int width, int height, const byte* pic_const, int numLevels, textureFormat_t& textureFormat, textureColor_t& colorFormat, bool gammaMips )
{
	fileData.textureType = TT_2D;
	fileData.format = textureFormat;
	fileData.colorFormat = colorFormat;
	fileData.width = width;
	fileData.height = height;
	fileData.numLevels = numLevels;
	
	byte* pic = ( byte* )Mem_Alloc( width * height * 4, TAG_TEMP );
	memcpy( pic, pic_const, width * height * 4 );
	
	if( colorFormat == CFM_YCOCG_DXT5 )
	{
		// convert the image data to YCoCg and use the YCoCgDXT5 compressor
		idColorSpace::ConvertRGBToCoCg_Y( pic, pic, width, height );
	}
	else if( colorFormat == CFM_NORMAL_DXT5 )
	{
		// Blah, HQ swizzles automatically, Fast doesn't
		if( !image_highQualityCompression.GetBool() )
		{
			for( int i = 0; i < width * height; i++ )
			{
				pic[i * 4 + 3] = pic[i * 4 + 0];
				pic[i * 4 + 0] = 0;
				pic[i * 4 + 2] = 0;
			}
		}
	}
	else if( colorFormat == CFM_GREEN_ALPHA )
	{
		for( int i = 0; i < width * height; i++ )
		{
			pic[i * 4 + 1] = pic[i * 4 + 3];
			pic[i * 4 + 0] = 0;
			pic[i * 4 + 2] = 0;
			pic[i * 4 + 3] = 0;
		}
	}
	
	int	scaledWidth = width;
	int scaledHeight = height;
	images.SetNum( numLevels );
	for( int level = 0; level < images.Num(); level++ )
	{
		idBinaryImageData& img = images[ level ];
		
		// Images that are going to be DXT compressed and aren't multiples of 4 need to be
		// padded out before compressing.
		byte* dxtPic = pic;
		int	dxtWidth = 0;
		int	dxtHeight = 0;
		if( textureFormat == FMT_DXT5 || textureFormat == FMT_DXT1 )
		{
			if( ( scaledWidth & 3 ) || ( scaledHeight & 3 ) )
			{
				dxtWidth = ( scaledWidth + 3 ) & ~3;
				dxtHeight = ( scaledHeight + 3 ) & ~3;
				dxtPic = ( byte* )Mem_ClearedAlloc( dxtWidth * 4 * dxtHeight, TAG_IMAGE );
				for( int i = 0; i < scaledHeight; i++ )
				{
					memcpy( dxtPic + i * dxtWidth * 4, pic + i * scaledWidth * 4, scaledWidth * 4 );
				}
			}
			else
			{
				dxtPic = pic;
				dxtWidth = scaledWidth;
				dxtHeight = scaledHeight;
			}
		}
		
		img.level = level;
		img.destZ = 0;
		img.width = scaledWidth;
		img.height = scaledHeight;
		
		// compress data or convert floats as necessary
		if( textureFormat == FMT_DXT1 )
		{
			idDxtEncoder dxt;
			img.Alloc( dxtWidth * dxtHeight / 2 );
			if( image_highQualityCompression.GetBool() )
			{
				dxt.CompressImageDXT1HQ( dxtPic, img.data, dxtWidth, dxtHeight );
			}
			else
			{
				dxt.CompressImageDXT1Fast( dxtPic, img.data, dxtWidth, dxtHeight );
			}
		}
		else if( textureFormat == FMT_DXT5 )
		{
			idDxtEncoder dxt;
			img.Alloc( dxtWidth * dxtHeight );
			if( colorFormat == CFM_NORMAL_DXT5 )
			{
				if( image_highQualityCompression.GetBool() )
				{
					dxt.CompressNormalMapDXT5HQ( dxtPic, img.data, dxtWidth, dxtHeight );
				}
				else
				{
					dxt.CompressNormalMapDXT5Fast( dxtPic, img.data, dxtWidth, dxtHeight );
				}
			}
			else if( colorFormat == CFM_YCOCG_DXT5 )
			{
				if( image_highQualityCompression.GetBool() )
				{
					dxt.CompressYCoCgDXT5HQ( dxtPic, img.data, dxtWidth, dxtHeight );
				}
				else
				{
					dxt.CompressYCoCgDXT5Fast( dxtPic, img.data, dxtWidth, dxtHeight );
				}
			}
			else
			{
				fileData.colorFormat = colorFormat = CFM_DEFAULT;
				if( image_highQualityCompression.GetBool() )
				{
					dxt.CompressImageDXT5HQ( dxtPic, img.data, dxtWidth, dxtHeight );
				}
				else
				{
					dxt.CompressImageDXT5Fast( dxtPic, img.data, dxtWidth, dxtHeight );
				}
			}
		}
		else if( textureFormat == FMT_LUM8 || textureFormat == FMT_INT8 )
		{
			// LUM8 and INT8 just read the red channel
			img.Alloc( scaledWidth * scaledHeight );
			for( int i = 0; i < img.dataSize; i++ )
			{
				img.data[ i ] = pic[ i * 4 ];
			}
		}
		else if( textureFormat == FMT_ALPHA )
		{
			// ALPHA reads the alpha channel
			img.Alloc( scaledWidth * scaledHeight );
			for( int i = 0; i < img.dataSize; i++ )
			{
				img.data[ i ] = pic[ i * 4 + 3 ];
			}
		}
		else if( textureFormat == FMT_L8A8 )
		{
			// L8A8 reads the alpha and red channels
			img.Alloc( scaledWidth * scaledHeight * 2 );
			for( int i = 0; i < img.dataSize / 2; i++ )
			{
				img.data[ i * 2 + 0 ] = pic[ i * 4 + 0 ];
				img.data[ i * 2 + 1 ] = pic[ i * 4 + 3 ];
			}
		}
		else if( textureFormat == FMT_RGB565 )
		{
			img.Alloc( scaledWidth * scaledHeight * 2 );
			for( int i = 0; i < img.dataSize / 2; i++ )
			{
				unsigned short color = ( ( pic[ i * 4 + 0 ] >> 3 ) << 11 ) | ( ( pic[ i * 4 + 1 ] >> 2 ) << 5 ) | ( pic[ i * 4 + 2 ] >> 3 );
				img.data[ i * 2 + 0 ] = ( color >> 8 ) & 0xFF;
				img.data[ i * 2 + 1 ] = color & 0xFF;
			}
		}
		else
		{
			fileData.format = textureFormat = FMT_RGBA8;
			img.Alloc( scaledWidth * scaledHeight * 4 );
			for( int i = 0; i < img.dataSize; i++ )
			{
				img.data[ i ] = pic[ i ];
			}
		}
		
		// if we had to pad to quads, free the padded version
		if( pic != dxtPic )
		{
			Mem_Free( dxtPic );
			dxtPic = NULL;
		}
		
		// downsample for the next level
		byte* shrunk = NULL;
		if( gammaMips )
		{
			shrunk = R_MipMapWithGamma( pic, scaledWidth, scaledHeight );
		}
		else
		{
			shrunk = R_MipMap( pic, scaledWidth, scaledHeight );
		}
		Mem_Free( pic );
		pic = shrunk;
		
		scaledWidth = Max( 1, scaledWidth >> 1 );
		scaledHeight = Max( 1, scaledHeight >> 1 );
	}
	
	Mem_Free( pic );
}

/*
========================
PadImageTo4x4

DXT Compression requres a complete 4x4 block, even if the GPU will only be sampling
a subset of it, so pad to 4x4 with replicated texels to maximize compression.
========================
*/
static void PadImageTo4x4( const byte* src, int width, int height, byte dest[64] )
{
	// we probably will need to support this for non-square images, but I'll address
	// that when needed
	assert( width <= 4 && height <= 4 );
	assert( width > 0 && height > 0 );
	
	for( int y = 0 ; y < 4 ; y++ )
	{
		int	sy = y % height;
		for( int x = 0 ; x < 4 ; x++ )
		{
			int	sx = x % width;
			for( int c = 0 ; c < 4 ; c++ )
			{
				dest[( y * 4 + x ) * 4 + c] = src[( sy * width + sx ) * 4 + c];
			}
		}
	}
}

/*
========================
idBinaryImage::LoadCubeFromMemory
========================
*/
void idBinaryImage::LoadCubeFromMemory( int width, const byte* pics[6], int numLevels, textureFormat_t& textureFormat, bool gammaMips )
{
	fileData.textureType = TT_CUBIC;
	fileData.format = textureFormat;
	fileData.colorFormat = CFM_DEFAULT;
	fileData.height = fileData.width = width;
	fileData.numLevels = numLevels;
	
	images.SetNum( fileData.numLevels * 6 );
	
	for( int side = 0; side < 6; side++ )
	{
		const byte* orig = pics[side];
		const byte* pic = orig;
		int	scaledWidth = fileData.width;
		for( int level = 0; level < fileData.numLevels; level++ )
		{
			// compress data or convert floats as necessary
			idBinaryImageData& img = images[ level * 6 + side ];
			
			// handle padding blocks less than 4x4 for the DXT compressors
			ALIGN16( byte padBlock[64] );
			int		padSize;
			const byte* padSrc;
			if( scaledWidth < 4 && ( textureFormat == FMT_DXT1 || textureFormat == FMT_DXT5 ) )
			{
				PadImageTo4x4( pic, scaledWidth, scaledWidth, padBlock );
				padSize = 4;
				padSrc = padBlock;
			}
			else
			{
				padSize = scaledWidth;
				padSrc = pic;
			}
			
			img.level = level;
			img.destZ = side;
			img.width = padSize;
			img.height = padSize;
			if( textureFormat == FMT_DXT1 )
			{
				img.Alloc( padSize * padSize / 2 );
				idDxtEncoder dxt;
				dxt.CompressImageDXT1Fast( padSrc, img.data, padSize, padSize );
			}
			else if( textureFormat == FMT_DXT5 )
			{
				img.Alloc( padSize * padSize );
				idDxtEncoder dxt;
				dxt.CompressImageDXT5Fast( padSrc, img.data, padSize, padSize );
			}
			else
			{
				fileData.format = textureFormat = FMT_RGBA8;
				img.Alloc( padSize * padSize * 4 );
				memcpy( img.data, pic, img.dataSize );
			}
			
			// downsample for the next level
			byte* shrunk = NULL;
			if( gammaMips )
			{
				shrunk = R_MipMapWithGamma( pic, scaledWidth, scaledWidth );
			}
			else
			{
				shrunk = R_MipMap( pic, scaledWidth, scaledWidth );
			}
			if( pic != orig )
			{
				Mem_Free( ( void* )pic );
				pic = NULL;
			}
			pic = shrunk;
			
			scaledWidth = Max( 1, scaledWidth >> 1 );
		}
		if( pic != orig )
		{
			// free the down sampled version
			Mem_Free( ( void* )pic );
			pic = NULL;
		}
	}
}

/*
========================
idBinaryImage::WriteGeneratedFile
========================
*/
ID_TIME_T idBinaryImage::WriteGeneratedFile( ID_TIME_T sourceFileTime )
{
	idStr binaryFileName;
	MakeGeneratedFileName( binaryFileName );
	idFileLocal file( fileSystem->OpenFileWrite( binaryFileName, "fs_basepath" ) );
	if( file == NULL )
	{
		idLib::Warning( "idBinaryImage: Could not open file '%s'", binaryFileName.c_str() );
		return FILE_NOT_FOUND_TIMESTAMP;
	}
	idLib::Printf( "Writing %s: %ix%i\n", binaryFileName.c_str(), fileData.width, fileData.height );
	
	fileData.headerMagic = BIMAGE_MAGIC;
	fileData.sourceFileTime = sourceFileTime;
	
	file->WriteBig( fileData.sourceFileTime );
	file->WriteBig( fileData.headerMagic );
	file->WriteBig( fileData.textureType );
	file->WriteBig( fileData.format );
	file->WriteBig( fileData.colorFormat );
	file->WriteBig( fileData.width );
	file->WriteBig( fileData.height );
	file->WriteBig( fileData.numLevels );
	
	for( int i = 0; i < images.Num(); i++ )
	{
		idBinaryImageData& img = images[ i ];
		file->WriteBig( img.level );
		file->WriteBig( img.destZ );
		file->WriteBig( img.width );
		file->WriteBig( img.height );
		file->WriteBig( img.dataSize );
		file->Write( img.data, img.dataSize );
	}
	return file->Timestamp();
}

/*
==========================
idBinaryImage::LoadFromGeneratedFile

Load the preprocessed image from the generated folder.
==========================
*/
ID_TIME_T idBinaryImage::LoadFromGeneratedFile( ID_TIME_T sourceFileTime )
{
	idStr binaryFileName;
	MakeGeneratedFileName( binaryFileName );
	idFileLocal bFile = fileSystem->OpenFileRead( binaryFileName );
	if( bFile == NULL )
	{
		return FILE_NOT_FOUND_TIMESTAMP;
	}
	if( LoadFromGeneratedFile( bFile, sourceFileTime ) )
	{
		return bFile->Timestamp();
	}
	return FILE_NOT_FOUND_TIMESTAMP;
}

/*
==========================
idBinaryImage::LoadFromGeneratedFile

Load the preprocessed image from the generated folder.
==========================
*/
bool idBinaryImage::LoadFromGeneratedFile( idFile* bFile, ID_TIME_T sourceTimeStamp )
{
	if( bFile->Read( &fileData, sizeof( fileData ) ) <= 0 )
	{
		return false;
	}
	idSwapClass<bimageFile_t> swap;
	swap.Big( fileData.sourceFileTime );
	swap.Big( fileData.headerMagic );
	swap.Big( fileData.textureType );
	swap.Big( fileData.format );
	swap.Big( fileData.colorFormat );
	swap.Big( fileData.width );
	swap.Big( fileData.height );
	swap.Big( fileData.numLevels );
	
	if( BIMAGE_MAGIC != fileData.headerMagic )
	{
		return false;
	}
	
	// RB: source might be from .resources, so we ignore the time stamp and assume a release build
	if( !fileSystem->InProductionMode() && ( sourceTimeStamp != FILE_NOT_FOUND_TIMESTAMP ) && ( sourceTimeStamp != 0 ) && ( sourceTimeStamp != fileData.sourceFileTime ) )
	{
		return false;
	}
	// RB end
	
	int numImages = fileData.numLevels;
	if( fileData.textureType == TT_CUBIC )
	{
		numImages *= 6;
	}
	
	images.SetNum( numImages );
	
	for( int i = 0; i < numImages; i++ )
	{
		idBinaryImageData& img = images[ i ];
		if( bFile->Read( &img, sizeof( bimageImage_t ) ) <= 0 )
		{
			return false;
		}
		idSwapClass<bimageImage_t> swap;
		swap.Big( img.level );
		swap.Big( img.destZ );
		swap.Big( img.width );
		swap.Big( img.height );
		swap.Big( img.dataSize );
		assert( img.level >= 0 && img.level < fileData.numLevels );
		assert( img.destZ == 0 || fileData.textureType == TT_CUBIC );
		assert( img.dataSize > 0 );
		// DXT images need to be padded to 4x4 block sizes, but the original image
		// sizes are still retained, so the stored data size may be larger than
		// just the multiplication of dimensions
		assert( img.dataSize >= img.width * img.height * BitsForFormat( ( textureFormat_t )fileData.format ) / 8 );
		img.Alloc( img.dataSize );
		if( img.data == NULL )
		{
			return false;
		}
		
		if( bFile->Read( img.data, img.dataSize ) <= 0 )
		{
			return false;
		}
	}
	
	return true;
}

/*
==========================
idBinaryImage::MakeGeneratedFileName
==========================
*/
void idBinaryImage::MakeGeneratedFileName( idStr& gfn )
{
	GetGeneratedFileName( gfn, GetName() );
}
/*
==========================
idBinaryImage::GetGeneratedFileName
==========================
*/
void idBinaryImage::GetGeneratedFileName( idStr& gfn, const char* name )
{
	gfn.Format( "generated/images/%s.bimage", name );
	gfn.Replace( "(", "/" );
	gfn.Replace( ",", "/" );
	gfn.Replace( ")", "" );
	gfn.Replace( " ", "" );
}


