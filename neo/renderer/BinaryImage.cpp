/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2014-2021 Robert Beckebans
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
#include "framework/Common_local.h"

/*
================================================================================================

	idBinaryImage

================================================================================================
*/

#include "RenderCommon.h"
#include "DXT/DXTCodec.h"
#include "Color/ColorSpace.h"

#include "../libs/mesa/format_r11g11b10f.h"

idCVar image_highQualityCompression( "image_highQualityCompression", "0", CVAR_BOOL, "Use high quality (slow) compression" );
idCVar r_useHighQualitySky( "r_useHighQualitySky", "1", CVAR_BOOL | CVAR_ARCHIVE, "Use high quality skyboxes" );

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

	commonLocal.LoadPacifierBinarizeInfo( va( "(%d x %d)", width, height ) );

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

		commonLocal.LoadPacifierBinarizeMiplevel( level + 1, numLevels );

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
				commonLocal.LoadPacifierBinarizeInfo( va( "(%d x %d) - DXT1HQ", width, height ) );

				dxt.CompressImageDXT1HQ( dxtPic, img.data, dxtWidth, dxtHeight );
			}
			else
			{
				commonLocal.LoadPacifierBinarizeInfo( va( "(%d x %d) - DXT1Fast", width, height ) );

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
					commonLocal.LoadPacifierBinarizeInfo( va( "(%d x %d) - NormalMapDXT5HQ", width, height ) );

					dxt.CompressNormalMapDXT5HQ( dxtPic, img.data, dxtWidth, dxtHeight );
				}
				else
				{
					commonLocal.LoadPacifierBinarizeInfo( va( "(%d x %d) - NormalMapDXT5Fast", width, height ) );

					dxt.CompressNormalMapDXT5Fast( dxtPic, img.data, dxtWidth, dxtHeight );
				}
			}
			else if( colorFormat == CFM_YCOCG_DXT5 )
			{
				if( image_highQualityCompression.GetBool() )
				{
					commonLocal.LoadPacifierBinarizeInfo( va( "(%d x %d) - YCoCgDXT5HQ", width, height ) );

					dxt.CompressYCoCgDXT5HQ( dxtPic, img.data, dxtWidth, dxtHeight );
				}
				else
				{
					commonLocal.LoadPacifierBinarizeInfo( va( "(%d x %d) - YCoCgDXT5Fast", width, height ) );

					dxt.CompressYCoCgDXT5Fast( dxtPic, img.data, dxtWidth, dxtHeight );
				}
			}
			else
			{
				fileData.colorFormat = colorFormat = CFM_DEFAULT;
				if( image_highQualityCompression.GetBool() )
				{
					commonLocal.LoadPacifierBinarizeInfo( va( "(%d x %d) - DXT5HQ", width, height ) );

					dxt.CompressImageDXT5HQ( dxtPic, img.data, dxtWidth, dxtHeight );
				}
				else
				{
					commonLocal.LoadPacifierBinarizeInfo( va( "(%d x %d) - DXT5Fast", width, height ) );

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
		else if( textureFormat == FMT_RG16F )
		{
			// RB: copy it as it was a RGBA8 because of the same size
			img.Alloc( scaledWidth * scaledHeight * 4 );
			for( int i = 0; i < img.dataSize; i++ )
			{
				img.data[ i ] = pic[ i ];
			}
		}
		else if( textureFormat == FMT_R11G11B10F )
		{
			// RB: copy it as it was a RGBA8 because of the same size
			img.Alloc( scaledWidth * scaledHeight * 4 );
			for( int i = 0; i < img.dataSize; i++ )
			{
				img.data[ i ] = pic[ i ];
			}
		}
		else if( textureFormat == FMT_RGBA16F )
		{
			img.Alloc( scaledWidth * scaledHeight * 8 );
			for( int i = 0; i < img.dataSize; i++ )
			{
				img.data[ i ] = pic[ i ];
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
RB idBinaryImage::Load2DAtlasMipchainFromMemory
========================
*/
void idBinaryImage::Load2DAtlasMipchainFromMemory( int width, int height, const byte* pic_const, int numLevels, textureFormat_t& textureFormat, textureColor_t& colorFormat )
{
	int sourceWidth = width * ( 2.0f / 3.0f ); // RB

	fileData.textureType = TT_2D;
	fileData.format = textureFormat;
	fileData.colorFormat = CFM_DEFAULT;
	fileData.width = sourceWidth;
	fileData.height = height;
	fileData.numLevels = numLevels;

	commonLocal.LoadPacifierBinarizeInfo( va( "(%d x %d)", width, height ) );

	byte* sourcePic = ( byte* )Mem_Alloc( width * height * 4, TAG_TEMP );
	memcpy( sourcePic, pic_const, width * height * 4 );

	if( colorFormat == CFM_YCOCG_DXT5 )
	{
		// convert the image data to YCoCg and use the YCoCgDXT5 compressor
		idColorSpace::ConvertRGBToCoCg_Y( sourcePic, sourcePic, width, height );
	}
	else if( colorFormat == CFM_NORMAL_DXT5 )
	{
		// Blah, HQ swizzles automatically, Fast doesn't
		if( !image_highQualityCompression.GetBool() )
		{
			for( int i = 0; i < width * height; i++ )
			{
				sourcePic[i * 4 + 3] = sourcePic[i * 4 + 0];
				sourcePic[i * 4 + 0] = 0;
				sourcePic[i * 4 + 2] = 0;
			}
		}
	}
	else if( colorFormat == CFM_GREEN_ALPHA )
	{
		for( int i = 0; i < width * height; i++ )
		{
			sourcePic[i * 4 + 1] = sourcePic[i * 4 + 3];
			sourcePic[i * 4 + 0] = 0;
			sourcePic[i * 4 + 2] = 0;
			sourcePic[i * 4 + 3] = 0;
		}
	}

	images.SetNum( numLevels );

	const int numColors = 5;
	static idVec4 colors[numColors] = { colorBlue, colorCyan, colorGreen, colorYellow, colorRed };

	for( int level = 0; level < images.Num(); level++ )
	{
		idBinaryImageData& img = images[ level ];

		// RB: create shrunk image which is a copy of the sub image in the atlas
		idVec4 rect = R_CalculateMipRect( sourceWidth, level );

		int	scaledWidth = rect.z;
		int scaledHeight = rect.w;

		byte* pic = ( byte* )Mem_Alloc( scaledWidth * scaledHeight * 4, TAG_TEMP );

		for( int x = rect.x; x < ( rect.x + rect.z ); x++ )
			//for( int x = 0; x < rect.z; x++ )
		{
			for( int y = rect.y; y < ( rect.y + rect.w ); y++ )
				//for( int y = 0; y < rect.w; y++ )
			{
				int sx = x - rect.x;
				int sy = y - rect.y;

#if 1
				pic[( sy * scaledWidth + sx ) * 4 + 0] = sourcePic[( y * width + x ) * 4 + 0];
				pic[( sy * scaledWidth + sx ) * 4 + 1] = sourcePic[( y * width + x ) * 4 + 1];
				pic[( sy * scaledWidth + sx ) * 4 + 2] = sourcePic[( y * width + x ) * 4 + 2];
				pic[( sy * scaledWidth + sx ) * 4 + 3] = sourcePic[( y * width + x ) * 4 + 3];
#else
				int colorIdx = level % numColors;
				float color[3];
				color[0] = colors[ colorIdx ].x;
				color[0] = colors[ colorIdx ].y;
				color[0] = colors[ colorIdx ].z;

				uint32_t value = float3_to_r11g11b10f( color );

				union
				{
					uint32	i;
					byte	b[4];
				} tmp;

				tmp.i = value;

				//*( uint32_t* )pic[( sy * scaledWidth + sx ) * 3] = value;

				pic[( sy * scaledWidth + sx ) * 4 + 0] = tmp.b[0];
				pic[( sy * scaledWidth + sx ) * 4 + 1] = tmp.b[1];
				pic[( sy * scaledWidth + sx ) * 4 + 2] = tmp.b[2];
				pic[( sy * scaledWidth + sx ) * 4 + 3] = tmp.b[3];
#endif
			}
		}
		// RB end

		commonLocal.LoadPacifierBinarizeMiplevel( level + 1, numLevels );

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
				commonLocal.LoadPacifierBinarizeInfo( va( "(%d x %d) - DXT1HQ", width, height ) );

				dxt.CompressImageDXT1HQ( dxtPic, img.data, dxtWidth, dxtHeight );
			}
			else
			{
				commonLocal.LoadPacifierBinarizeInfo( va( "(%d x %d) - DXT1Fast", width, height ) );

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
					commonLocal.LoadPacifierBinarizeInfo( va( "(%d x %d) - NormalMapDXT5HQ", width, height ) );

					dxt.CompressNormalMapDXT5HQ( dxtPic, img.data, dxtWidth, dxtHeight );
				}
				else
				{
					commonLocal.LoadPacifierBinarizeInfo( va( "(%d x %d) - NormalMapDXT5Fast", width, height ) );

					dxt.CompressNormalMapDXT5Fast( dxtPic, img.data, dxtWidth, dxtHeight );
				}
			}
			else if( colorFormat == CFM_YCOCG_DXT5 )
			{
				if( image_highQualityCompression.GetBool() )
				{
					commonLocal.LoadPacifierBinarizeInfo( va( "(%d x %d) - YCoCgDXT5HQ", width, height ) );

					dxt.CompressYCoCgDXT5HQ( dxtPic, img.data, dxtWidth, dxtHeight );
				}
				else
				{
					commonLocal.LoadPacifierBinarizeInfo( va( "(%d x %d) - YCoCgDXT5Fast", width, height ) );

					dxt.CompressYCoCgDXT5Fast( dxtPic, img.data, dxtWidth, dxtHeight );
				}
			}
			else
			{
				fileData.colorFormat = colorFormat = CFM_DEFAULT;
				if( image_highQualityCompression.GetBool() )
				{
					commonLocal.LoadPacifierBinarizeInfo( va( "(%d x %d) - DXT5HQ", width, height ) );

					dxt.CompressImageDXT5HQ( dxtPic, img.data, dxtWidth, dxtHeight );
				}
				else
				{
					commonLocal.LoadPacifierBinarizeInfo( va( "(%d x %d) - DXT5Fast", width, height ) );

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
		else if( textureFormat == FMT_RG16F )
		{
			// RB: copy it as it was a RGBA8 because of the same size
			img.Alloc( scaledWidth * scaledHeight * 4 );
			for( int i = 0; i < img.dataSize; i++ )
			{
				img.data[ i ] = pic[ i ];
			}
		}
		else if( textureFormat == FMT_R11G11B10F )
		{
			// RB: copy it as it was a RGBA8 because of the same size
			img.Alloc( scaledWidth * scaledHeight * 4 );
			for( int i = 0; i < img.dataSize; i++ )
			{
				img.data[ i ] = pic[ i ];
			}
		}
		else if( textureFormat == FMT_RGBA16F )
		{
			img.Alloc( scaledWidth * scaledHeight * 8 );
			for( int i = 0; i < img.dataSize; i++ )
			{
				img.data[ i ] = pic[ i ];
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
		/*
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
		*/

		Mem_Free( pic );
	}

	Mem_Free( sourcePic );
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
	commonLocal.LoadPacifierBinarizeInfo( va( "cube (%d)", width ) );

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

			commonLocal.LoadPacifierBinarizeMiplevel( level, fileData.numLevels );

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

				if( image_highQualityCompression.GetBool() )
				{
					commonLocal.LoadPacifierBinarizeInfo( va( "(%d x %d) - DXT1HQ", width, width ) );

					dxt.CompressImageDXT1HQ( padSrc, img.data, padSize, padSize );
				}
				else
				{
					commonLocal.LoadPacifierBinarizeInfo( va( "(%d x %d) - DXT1Fast", width, width ) );

					dxt.CompressImageDXT1Fast( padSrc, img.data, padSize, padSize );
				}
			}
			else if( textureFormat == FMT_DXT5 )
			{
				img.Alloc( padSize * padSize );
				idDxtEncoder dxt;

				if( image_highQualityCompression.GetBool() )
				{
					commonLocal.LoadPacifierBinarizeInfo( va( "(%d x %d) - DXT5HQ", width, width ) );

					dxt.CompressImageDXT5HQ( padSrc, img.data, padSize, padSize );
				}
				else
				{
					commonLocal.LoadPacifierBinarizeInfo( va( "(%d x %d) - DXT5Fast", width, width ) );

					dxt.CompressImageDXT5Fast( padSrc, img.data, padSize, padSize );
				}
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
#if defined(__APPLE__) && defined(USE_VULKAN)
		int imgfile_dataSize = img.dataSize;
		// SRS - Allocate 2x memory to prepare for in-place conversion from FMT_RGB565 to FMT_RGBA8
		if( ( textureFormat_t )fileData.format == FMT_RGB565 )
		{
			img.Alloc( img.dataSize * 2 );
		}
		else
		{
			img.Alloc( img.dataSize );
		}
		if( img.data == NULL )
		{
			return false;
		}

		// SRS - Read image data using actual on-disk data size
		if( bFile->Read( img.data, imgfile_dataSize ) <= 0 )
		{
			return false;
		}

		// SRS - Convert FMT_RGB565 16-bits to FMT_RGBA8 32-bits in place using pre-allocated space
		if( ( textureFormat_t )fileData.format == FMT_RGB565 )
		{
			//SRS - Make sure we have an integer number of RGBA8 storage slots
			assert( img.dataSize % 4 == 0 );
			for( int pixelIndex = img.dataSize / 2 - 2; pixelIndex >= 0; pixelIndex -= 2 )
			{
#if 1
				// SRS - Option 1: Scale and shift algorithm
				uint16 pixelValue_rgb565 = img.data[pixelIndex + 0] << 8 | img.data[pixelIndex + 1];
				img.data[pixelIndex * 2 + 0] = ( ( ( pixelValue_rgb565 ) >> 11 ) * 527 + 23 ) >> 6;
				img.data[pixelIndex * 2 + 1] = ( ( ( pixelValue_rgb565 & 0x07E0 ) >>  5 ) * 259 + 33 ) >> 6;
				img.data[pixelIndex * 2 + 2] = ( ( ( pixelValue_rgb565 & 0x001F ) ) * 527 + 23 ) >> 6;
#else
				// SRS - Option 2: Shift and combine algorithm - is this faster?
				uint8 pixelValue_rgb565_hi = img.data[pixelIndex + 0];
				uint8 pixelValue_rgb565_lo = img.data[pixelIndex + 1];
				img.data[pixelIndex * 2 + 0] = ( pixelValue_rgb565_hi & 0xF8 ) | ( pixelValue_rgb565_hi          >> 5 );
				img.data[pixelIndex * 2 + 1] = ( pixelValue_rgb565_hi        << 5 ) | ( ( pixelValue_rgb565_lo & 0xE0 ) >> 3 ) | ( ( pixelValue_rgb565_hi & 0x07 ) >> 1 );
				img.data[pixelIndex * 2 + 2] = ( pixelValue_rgb565_lo        << 3 ) | ( ( pixelValue_rgb565_lo & 0x1F ) >> 2 );
#endif
				img.data[pixelIndex * 2 + 3] = 0xFF;
			}
		}
#else
		img.Alloc( img.dataSize );
		if( img.data == NULL )
		{
			return false;
		}

		if( bFile->Read( img.data, img.dataSize ) <= 0 )
		{
			return false;
		}
#endif
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


