/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2015 Robert Beckebans

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
#include "../renderer/Image.h"
//#include "../../renderer/ImageTools/ImageProcess.h"
#include <jpeglib.h>

idCVar swf_useChannelScale( "swf_useChannelScale", "0", CVAR_BOOL, "compress texture atlas colors" );

/*
========================
idSWF::idDecompressJPEG
These are the static callback functions the jpeg library calls
========================
*/
void swf_jpeg_error_exit( jpeg_common_struct* cinfo )
{
	char buffer[JMSG_LENGTH_MAX] = {0};
	( *cinfo->err->format_message )( cinfo, buffer );
	throw idException( buffer );
}
void swf_jpeg_output_message( jpeg_common_struct* cinfo )
{
	char buffer[JMSG_LENGTH_MAX] = {0};
	( *cinfo->err->format_message )( cinfo, buffer );
	idLib::Printf( "%s\n", buffer );
}
void swf_jpeg_init_source( jpeg_decompress_struct* cinfo )
{
}
boolean swf_jpeg_fill_input_buffer( jpeg_decompress_struct* cinfo )
{
	return TRUE;
}
void swf_jpeg_skip_input_data( jpeg_decompress_struct* cinfo, long num_bytes )
{
	cinfo->src->next_input_byte += num_bytes;
	cinfo->src->bytes_in_buffer -= num_bytes;
}
void swf_jpeg_term_source( jpeg_decompress_struct* cinfo )
{
}

/*
========================
idSWF::idDecompressJPEG::idDecompressJPEG
========================
*/
idSWF::idDecompressJPEG::idDecompressJPEG()
{
	jpeg_decompress_struct* cinfo = new( TAG_SWF ) jpeg_decompress_struct;
	memset( cinfo, 0, sizeof( *cinfo ) );

	cinfo->err = new( TAG_SWF ) jpeg_error_mgr;
	memset( cinfo->err, 0, sizeof( jpeg_error_mgr ) );
	jpeg_std_error( cinfo->err );
	cinfo->err->error_exit = swf_jpeg_error_exit;
	cinfo->err->output_message = swf_jpeg_output_message;

	jpeg_create_decompress( cinfo );

	vinfo = cinfo;
}

/*
========================
idSWF::idDecompressJPEG::~idDecompressJPEG
========================
*/
idSWF::idDecompressJPEG::~idDecompressJPEG()
{
	jpeg_decompress_struct* cinfo = ( jpeg_decompress_struct* )vinfo;

	jpeg_destroy_decompress( cinfo );
	delete cinfo->err;
	delete cinfo;
}

/*
========================
idSWF::idDecompressJPEG::Load
========================
*/
byte* idSWF::idDecompressJPEG::Load( const byte* input, int inputSize, int& width, int& height )
{
	jpeg_decompress_struct* cinfo = ( jpeg_decompress_struct* )vinfo;

	try
	{

		width = 0;
		height = 0;

		jpeg_source_mgr src;
		memset( &src, 0, sizeof( src ) );
		src.next_input_byte = ( JOCTET* )input;
		src.bytes_in_buffer = inputSize;
		src.init_source = swf_jpeg_init_source;
		src.fill_input_buffer = swf_jpeg_fill_input_buffer;
		src.skip_input_data = swf_jpeg_skip_input_data;
		src.resync_to_restart = jpeg_resync_to_restart;
		src.term_source = swf_jpeg_term_source;
		cinfo->src = &src;

		int result = 0;
		do
		{
			result = jpeg_read_header( cinfo, FALSE );
		}
		while( result == JPEG_HEADER_TABLES_ONLY );

		if( result == JPEG_SUSPENDED )
		{
			return NULL;
		}

		jpeg_start_decompress( cinfo );
		if( cinfo->output_components != 4 )
		{
			// This shouldn't really be possible, unless the source image is some kind of strange grayscale format or something
			idLib::Warning( "JPEG output is not 4 components" );
			jpeg_abort_decompress( cinfo );
			cinfo->src = NULL;	// value goes out of scope
			return NULL;
		}
		int outputSize = cinfo->output_width * cinfo->output_height * cinfo->output_components;
		byte* output = ( byte* )Mem_Alloc( outputSize, TAG_SWF );
		memset( output, 255, outputSize );
		while( cinfo->output_scanline < cinfo->output_height )
		{
			JSAMPROW scanlines = output + cinfo->output_scanline * cinfo->output_width * cinfo->output_components;
			jpeg_read_scanlines( cinfo, &scanlines, 1 );
		}
		jpeg_finish_decompress( cinfo );

		width = cinfo->output_width;
		height = cinfo->output_height;

		cinfo->src = NULL;	// value goes out of scope
		return output;

	}
	catch( idException& )
	{
		swf_jpeg_output_message( ( jpeg_common_struct* )cinfo );
		return NULL;
	}
}


/*
========================
idSWF::WriteSwfImageAtlas

Now that all images have been found, allocate them in an atlas
and write it out.
========================
*/
void RectAllocator( const idList<idVec2i>& inputSizes, idList<idVec2i>& outputPositions, idVec2i& totalSize, const int START_MAX = 16384, const int imageMax = 1024 );
float RectPackingFraction( const idList<idVec2i>& inputSizes, const idVec2i totalSize );

void idSWF::WriteSwfImageAtlas( const char* filename )
{
	idList<idVec2i>	inputSizes;
	inputSizes.SetNum( packImages.Num() );
	for( int i = 0 ; i < packImages.Num() ; i++ )
	{
		// these are in DXT blocks, not pixels
		inputSizes[i] = packImages[i].allocSize;
	}

	idList<idVec2i>	outputPositions;
	idVec2i	totalSize;
	// smart allocator
	RectAllocator( inputSizes, outputPositions, totalSize );

	float frac = RectPackingFraction( inputSizes, totalSize );
	idLib::Printf( "%5.2f packing fraction in %ix%i image\n", frac, totalSize.x * 4, totalSize.y * 4 );

	int atlasWidth =  Max( 4, totalSize.x * 4 ) ;
	int atlasHeight =  Max( 4, totalSize.y * 4 ) ;

	// we require multiple-of-128 widths to use the image data directly
	// without re-packing on the 360 and PS3.  The growth checks in RectAllocator()
	// will always align, but a single image won't necessarily be.
	atlasWidth = ( atlasWidth + 127 ) & ~127;

	idTempArray<byte> swfAtlas( atlasWidth * atlasHeight * 4 );

	// fill everything with solid red
	for( int i = 0; i < atlasWidth * atlasHeight; i++ )
	{
		swfAtlas[i * 4 + 0] = 255;
		swfAtlas[i * 4 + 1] = 0;
		swfAtlas[i * 4 + 2] = 0;
		swfAtlas[i * 4 + 3] = 255;
	}

	// allocate the blocks and copy the texels
	for( int i = 0 ; i < packImages.Num() ; i++ )
	{
		imageToPack_t& pack = packImages[i];
		assert( pack.imageData != NULL );

		int	blockWidth = pack.allocSize.x;
		int	blockHeight = pack.allocSize.y;

		int x = outputPositions[i].x;
		int y = outputPositions[i].y;

		// get the range for each channel so we can maximize it
		// for better compression
		int	minV[4] = { 255, 255, 255, 255 };
		int	maxV[4] = { 0, 0, 0, 0 };
		for( int j = 0 ; j < pack.trueSize.x * pack.trueSize.y * 4 ; j++ )
		{
			int	v = pack.imageData[ j ];
			int	x = j & 3;
			if( v < minV[x] )
			{
				minV[x] = v;
			}
			if( v > maxV[x] )
			{
				maxV[x] = v;
			}
		}
//		idLib::Printf( "Color normalize: %3i:%3i  %3i:%3i  %3i:%3i  %3i:%3i\n",
//			minV[0], maxV[0], minV[1], maxV[1], minV[2], maxV[2], minV[3], maxV[3] );

		// don't divide by zero
		for( int x = 0 ; x < 4 ; x++ )
		{
			if( maxV[x] == 0 )
			{
				maxV[x] = 1;
			}
		}
		// rescale the image
		//
		// Note that this must be done in RGBA space, before YCoCg conversion,
		// or the scale factors couldn't be combined with the normal swf coloring.
		//
		// If we create an idMaterial for each atlas element, we could add
		// a bias as well as a scale to enable us to take advantage of the
		// min values as well as the max, but very few gui images don't go to black,
		// and just doing a scale avoids changing more code.

		if( swf_useChannelScale.GetBool() )
		{
			for( int j = 0; j < pack.trueSize.x * pack.trueSize.y * 4; j++ )
			{
				int	v = pack.imageData[ j ];
				int	x = j & 3;
				v = v * 255 / maxV[x];
				pack.imageData[ j ] = v;
			}
		}

		assert( ( x + blockWidth ) * 4 <= atlasWidth );
		assert( ( y + blockHeight ) * 4 <= atlasHeight );
		// Extend the pixels with clamp-to-edge to the edge of the allocation block.
		// The GPU hardware addressing should completely ignore texels outside the true block
		// size, but the compressor works on complete blocks, regardless of the true rect size.
		x <<= 2;
		y <<= 2;
		for( int dstY = 0; dstY < blockHeight << 2; dstY++ )
		{
			int	srcY = dstY - 1;
			if( srcY < 0 )
			{
				srcY = 0;
			}
			if( srcY >= pack.trueSize.y )
			{
				srcY = pack.trueSize.y - 1;
			}
			for( int dstX = 0 ; dstX < blockWidth << 2 ; dstX++ )
			{
				int srcX = dstX - 1;
				if( srcX < 0 )
				{
					srcX = 0;
				}
				if( srcX >= pack.trueSize.x )
				{
					srcX = pack.trueSize.x - 1;
				}
				( ( int* )swfAtlas.Ptr() )[( y + dstY ) * atlasWidth + ( x + dstX ) ] =
					( ( int* )pack.imageData )[ srcY * pack.trueSize.x + srcX ];
			}
		}

		// save the information in the SWF dictionary
		idSWFDictionaryEntry* entry = FindDictionaryEntry( pack.characterID );
		assert( entry->material == NULL );
		entry->imageSize.x = pack.trueSize.x;
		entry->imageSize.y = pack.trueSize.y;
		entry->imageAtlasOffset.x = x + 1;
		entry->imageAtlasOffset.y = y + 1;

		if( swf_useChannelScale.GetBool() )
		{
			for( int i = 0; i < 4; i++ )
			{
				entry->channelScale[i] = maxV[i] / 255.0f;
			}
		}

		Mem_Free( pack.imageData );
		pack.imageData = NULL;
	}

	// the TGA is only for examination during development
	//R_WriteTGA( filename, swfAtlas.Ptr(), atlasWidth, atlasHeight, false, "fs_basepath" );
	R_WritePNG( filename, swfAtlas.Ptr(), 4, atlasWidth, atlasHeight, true, "fs_basepath" );
}

/*
========================
idSWF::LoadImage
Loads RGBA data into an image at the specificied character id in the dictionary
========================
*/
void idSWF::LoadImage( int characterID, const byte* imageData, int width, int height )
{
	idSWFDictionaryEntry* entry = AddDictionaryEntry( characterID, SWF_DICT_IMAGE );
	if( entry == NULL )
	{
		return;
	}

	// save the data off so we can do the image atlas allocation after we have collected
	// all the images that are used by the entire swf
	imageToPack_t	pack;
	pack.characterID = characterID;
	pack.imageData = ( byte* )Mem_Alloc( width * height * 4, TAG_SWF );
	memcpy( pack.imageData, imageData, width * height * 4 );
	pack.trueSize.x = width;
	pack.trueSize.y = height;
	for( int i = 0 ; i < 2 ; i++ )
	{
		int	v = pack.trueSize[i];
		// Swf images are usually completely random in size, but perform all allocations in
		// DXT blocks of 4.  If we choose to DCT / HDP encode the image block, we should probably
		// increae the block size to 8 or 16 to prevent neighbor effects.
		v = ( v + 3 ) >> 2;

		// Allways allocate a single pixel border around the images so there won't be any edge
		// bleeds.  This can often be hidden in in the round-up to DXT size.
		if( ( v << 2 ) - pack.trueSize[i] < 2 )
		{
			v++;
		}
		pack.allocSize[i] = v;
	}
	packImages.Append( pack );

	entry->material = NULL;
}

/*
========================
idSWF::JPEGTables
Reads jpeg table data, there can only be one of these in the file, and it has to come before any DefineBits tags
We don't have to worry about clearing the jpeg object because jpeglib will automagically overwrite any tables that are already set (I think?)
========================
*/
void idSWF::JPEGTables( idSWFBitStream& bitstream )
{
	if( bitstream.Length() == 0 )
	{
		// no clue why this happens
		return;
	}
	int width, height;
	jpeg.Load( bitstream.ReadData( bitstream.Length() ), bitstream.Length(), width, height );
}

/*
========================
idSWF::DefineBits
Reads a partial jpeg image, using the tables set by the JPEGTables tag
========================
*/
void idSWF::DefineBits( idSWFBitStream& bitstream )
{
	uint16 characterID = bitstream.ReadU16();

	int jpegSize = bitstream.Length() - sizeof( uint16 );

	int width, height;
	byte* imageData = jpeg.Load( bitstream.ReadData( jpegSize ), jpegSize, width, height );
	if( imageData == NULL )
	{
		return;
	}

	LoadImage( characterID, imageData, width, height );

	Mem_Free( imageData );
}

/*
========================
idSWF::DefineBitsJPEG2
Identical to DefineBits, except it uses a local JPEG table (not the one defined by JPEGTables)
========================
*/
void idSWF::DefineBitsJPEG2( idSWFBitStream& bitstream )
{
	uint16 characterID = bitstream.ReadU16();

	idDecompressJPEG jpeg;

	int jpegSize = bitstream.Length() - sizeof( uint16 );

	int width, height;
	byte* imageData = jpeg.Load( bitstream.ReadData( jpegSize ), jpegSize, width, height );
	if( imageData == NULL )
	{
		return;
	}

	LoadImage( characterID, imageData, width, height );

	Mem_Free( imageData );
}

/*
========================
idSWF::DefineBitsJPEG3
Mostly identical to DefineBitsJPEG2, except it has an additional zlib compressed alpha map
========================
*/
void idSWF::DefineBitsJPEG3( idSWFBitStream& bitstream )
{
	uint16 characterID = bitstream.ReadU16();
	uint32 jpegSize = bitstream.ReadU32();

	idDecompressJPEG jpeg;

	int width, height;
	byte* imageData = jpeg.Load( bitstream.ReadData( jpegSize ), jpegSize, width, height );
	if( imageData == NULL )
	{
		return;
	}

	{
		idTempArray<byte> alphaMap( width * height );

		int alphaSize = bitstream.Length() - jpegSize - sizeof( characterID ) - sizeof( jpegSize );
		if( !Inflate( bitstream.ReadData( alphaSize ), alphaSize, alphaMap.Ptr(), ( int )alphaMap.Size() ) )
		{
			idLib::Warning( "DefineBitsJPEG3: Failed to inflate alpha data" );
			Mem_Free( imageData );
			return;
		}
		for( int i = 0; i < width * height; i++ )
		{
			imageData[i * 4 + 3] = alphaMap[i];
		}
	}

	LoadImage( characterID, imageData, width, height );

	Mem_Free( imageData );
}

/*
========================
idSWF::DefineBitsLossless
========================
*/
void idSWF::DefineBitsLossless( idSWFBitStream& bitstream )
{
	uint16 characterID = bitstream.ReadU16();
	uint8 format = bitstream.ReadU8();
	uint16 width = bitstream.ReadU16();
	uint16 height = bitstream.ReadU16();

	idTempArray< byte > buf( width * height * 4 );
	byte* imageData = buf.Ptr();

	if( format == 3 )
	{
		uint32 paddedWidth = ( width + 3 ) & ~3;
		uint32 colorTableSize = ( bitstream.ReadU8() + 1 ) * 3;
		idTempArray<byte> colorMapData( colorTableSize + ( paddedWidth * height ) );
		uint32 colorDataSize = bitstream.Length() - bitstream.Tell();
		if( !Inflate( bitstream.ReadData( colorDataSize ), colorDataSize, colorMapData.Ptr(), ( int )colorMapData.Size() ) )
		{
			idLib::Warning( "DefineBitsLossless: Failed to inflate color map data" );
			return;
		}
		byte* indices = colorMapData.Ptr() + colorTableSize;
		for( int h = 0; h < height; h++ )
		{
			for( int w = 0; w < width; w++ )
			{
				byte index = indices[w + ( h * paddedWidth )];
				byte* pixel = &imageData[( w + ( h * width ) ) * 4];
				pixel[0] = colorMapData[index * 3 + 0];
				pixel[1] = colorMapData[index * 3 + 1];
				pixel[2] = colorMapData[index * 3 + 2];
				pixel[3] = 0xFF;
			}
		}
	}
	else if( format == 4 )
	{
		uint32 paddedWidth = ( width + 1 ) & 1;
		idTempArray<uint16> bitmapData( paddedWidth * height * 2 );
		uint32 colorDataSize = bitstream.Length() - bitstream.Tell();
		if( !Inflate( bitstream.ReadData( colorDataSize ), colorDataSize, ( byte* )bitmapData.Ptr(), ( int )bitmapData.Size() ) )
		{
			idLib::Warning( "DefineBitsLossless: Failed to inflate bitmap data" );
			return;
		}
		for( int h = 0; h < height; h++ )
		{
			for( int w = 0; w < width; w++ )
			{
				uint16 pix15 = bitmapData[w + ( h * paddedWidth )];
				idSwap::Big( pix15 );
				byte* pixel = &imageData[( w + ( h * width ) ) * 4];
				pixel[0] = ( pix15 >> 10 ) & 0x1F;
				pixel[1] = ( pix15 >> 5 ) & 0x1F;
				pixel[2] = ( pix15 >> 0 ) & 0x1F;
				pixel[3] = 0xFF;
			}
		}
	}
	else if( format == 5 )
	{
		idTempArray<uint32> bitmapData( width * height );
		uint32 colorDataSize = bitstream.Length() - bitstream.Tell();
		if( !Inflate( bitstream.ReadData( colorDataSize ), colorDataSize, ( byte* )bitmapData.Ptr(), ( int )bitmapData.Size() ) )
		{
			idLib::Warning( "DefineBitsLossless: Failed to inflate bitmap data" );
			return;
		}
		for( int h = 0; h < height; h++ )
		{
			for( int w = 0; w < width; w++ )
			{
				uint32 pix24 = bitmapData[w + ( h * width )];
				idSwap::Big( pix24 );
				byte* pixel = &imageData[( w + ( h * width ) ) * 4];
				pixel[0] = ( pix24 >> 16 ) & 0xFF;
				pixel[1] = ( pix24 >> 8 ) & 0xFF;
				pixel[2] = ( pix24 >> 0 ) & 0xFF;
				pixel[3] = 0xFF;
			}
		}
	}
	else
	{
		idLib::Warning( "DefineBitsLossless: Unknown image format %d", format );
		memset( imageData, 0xFF, width * height * 4 );
	}

	LoadImage( characterID, imageData, width, height );
}

/*
========================
idSWF::DefineBitsLossless2
========================
*/
void idSWF::DefineBitsLossless2( idSWFBitStream& bitstream )
{
	uint16 characterID = bitstream.ReadU16();
	uint8 format = bitstream.ReadU8();
	uint16 width = bitstream.ReadU16();
	uint16 height = bitstream.ReadU16();

	idTempArray< byte > buf( width * height * 4 );
	byte* imageData = buf.Ptr();

	if( format == 3 )
	{
		uint32 paddedWidth = ( width + 3 ) & ~3;
		uint32 colorTableSize = ( bitstream.ReadU8() + 1 ) * 4;
		idTempArray<byte> colorMapData( colorTableSize + ( paddedWidth * height ) );
		uint32 colorDataSize = bitstream.Length() - bitstream.Tell();
		if( !Inflate( bitstream.ReadData( colorDataSize ), colorDataSize, colorMapData.Ptr(), ( int )colorMapData.Size() ) )
		{
			idLib::Warning( "DefineBitsLossless2: Failed to inflate color map data" );
			return;
		}
		byte* indices = colorMapData.Ptr() + colorTableSize;
		for( int h = 0; h < height; h++ )
		{
			for( int w = 0; w < width; w++ )
			{
				byte index = indices[w + ( h * paddedWidth )];
				byte* pixel = &imageData[( w + ( h * width ) ) * 4];
				pixel[0] = colorMapData[index * 4 + 0];
				pixel[1] = colorMapData[index * 4 + 1];
				pixel[2] = colorMapData[index * 4 + 2];
				pixel[3] = colorMapData[index * 4 + 3];
			}
		}
	}
	else if( format == 5 )
	{
		idTempArray<uint32> bitmapData( width * height );
		uint32 colorDataSize = bitstream.Length() - bitstream.Tell();
		if( !Inflate( bitstream.ReadData( colorDataSize ), colorDataSize, ( byte* )bitmapData.Ptr(), ( int )bitmapData.Size() ) )
		{
			idLib::Warning( "DefineBitsLossless2: Failed to inflate bitmap data" );
			return;
		}
		for( int h = 0; h < height; h++ )
		{
			for( int w = 0; w < width; w++ )
			{
				uint32 pix32 = bitmapData[w + ( h * width )];
				idSwap::Big( pix32 );
				byte* pixel = &imageData[( w + ( h * width ) ) * 4];
				pixel[0] = ( pix32 >> 16 ) & 0xFF;
				pixel[1] = ( pix32 >> 8 ) & 0xFF;
				pixel[2] = ( pix32 >> 0 ) & 0xFF;
				pixel[3] = ( pix32 >> 24 ) & 0xFF;
			}
		}
	}
	else
	{
		idLib::Warning( "DefineBitsLossless2: Unknown image format %d", format );
		memset( imageData, 0xFF, width * height * 4 );
	}

	LoadImage( characterID, imageData, width, height );
}
