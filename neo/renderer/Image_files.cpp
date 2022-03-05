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

#include "precompiled.h"
#pragma hdrstop

#undef strncmp

#define STB_IMAGE_IMPLEMENTATION
#include "../libs/stb/stb_image.h"

//#define STB_IMAGE_WRITE_IMPLEMENTATION
//#include "../libs/stb/stb_image_write.h"

#define TINYEXR_IMPLEMENTATION
#include "../libs/tinyexr/tinyexr.h"

#include "../libs/mesa/format_r11g11b10f.h"

#include "RenderCommon.h"

/*

This file only has a single entry point:

void R_LoadImage( const char *name, byte **pic, int *width, int *height, bool makePowerOf2 );

*/

/*
 * Include file for users of JPEG library.
 * You will need to have included system headers that define at least
 * the typedefs FILE and size_t before you can include jpeglib.h.
 * (stdio.h is sufficient on ANSI-conforming systems.)
 * You may also wish to include "jerror.h".
 */

#include <jpeglib.h>
#include <jerror.h>

// hooks from jpeg lib to our system

void jpg_Error( const char* fmt, ... )
{
	va_list		argptr;
	char		msg[2048];

	va_start( argptr, fmt );
	vsprintf( msg, fmt, argptr );
	va_end( argptr );

	common->FatalError( "%s", msg );
}

void jpg_Printf( const char* fmt, ... )
{
	va_list		argptr;
	char		msg[2048];

	va_start( argptr, fmt );
	vsprintf( msg, fmt, argptr );
	va_end( argptr );

	common->Printf( "%s", msg );
}



/*
================
R_WriteTGA
================
*/
void R_WriteTGA( const char* filename, const byte* data, int width, int height, bool flipVertical, const char* basePath )
{
	byte*	buffer;
	int		i;
	int		bufferSize = width * height * 4 + 18;
	int     imgStart = 18;

	idTempArray<byte> buf( bufferSize );
	buffer = ( byte* )buf.Ptr();
	memset( buffer, 0, 18 );
	buffer[2] = 2;		// uncompressed type
	buffer[12] = width & 255;
	buffer[13] = width >> 8;
	buffer[14] = height & 255;
	buffer[15] = height >> 8;
	buffer[16] = 32;	// pixel size
	if( !flipVertical )
	{
		buffer[17] = ( 1 << 5 );	// flip bit, for normal top to bottom raster order
	}

	// swap rgb to bgr
	for( i = imgStart ; i < bufferSize ; i += 4 )
	{
		buffer[i] = data[i - imgStart + 2];		// blue
		buffer[i + 1] = data[i - imgStart + 1];		// green
		buffer[i + 2] = data[i - imgStart + 0];		// red
		buffer[i + 3] = data[i - imgStart + 3];		// alpha
	}

	fileSystem->WriteFile( filename, buffer, bufferSize, basePath );
}

void LoadTGA( const char* name, byte** pic, int* width, int* height, ID_TIME_T* timestamp );
static void LoadJPG( const char* name, byte** pic, int* width, int* height, ID_TIME_T* timestamp );

/*
========================================================================

TGA files are used for 24/32 bit images

========================================================================
*/

typedef struct _TargaHeader
{
	unsigned char 	id_length, colormap_type, image_type;
	unsigned short	colormap_index, colormap_length;
	unsigned char	colormap_size;
	unsigned short	x_origin, y_origin, width, height;
	unsigned char	pixel_size, attributes;
} TargaHeader;


/*
=========================================================

TARGA LOADING

=========================================================
*/

/*
=============
LoadTGA
=============
*/
void LoadTGA( const char* name, byte** pic, int* width, int* height, ID_TIME_T* timestamp )
{
	int		columns, rows, numPixels, fileSize, numBytes;
	byte*	pixbuf;
	int		row, column;
	byte*	buf_p;
	byte*	buffer;
	TargaHeader	targa_header;
	byte*		targa_rgba;

	if( !pic )
	{
		fileSystem->ReadFile( name, NULL, timestamp );
		return;	// just getting timestamp
	}

	*pic = NULL;

	//
	// load the file
	//
	fileSize = fileSystem->ReadFile( name, ( void** )&buffer, timestamp );
	if( !buffer )
	{
		return;
	}

	buf_p = buffer;

	targa_header.id_length = *buf_p++;
	targa_header.colormap_type = *buf_p++;
	targa_header.image_type = *buf_p++;

	targa_header.colormap_index = LittleShort( *( short* )buf_p );
	buf_p += 2;
	targa_header.colormap_length = LittleShort( *( short* )buf_p );
	buf_p += 2;
	targa_header.colormap_size = *buf_p++;
	targa_header.x_origin = LittleShort( *( short* )buf_p );
	buf_p += 2;
	targa_header.y_origin = LittleShort( *( short* )buf_p );
	buf_p += 2;
	targa_header.width = LittleShort( *( short* )buf_p );
	buf_p += 2;
	targa_header.height = LittleShort( *( short* )buf_p );
	buf_p += 2;
	targa_header.pixel_size = *buf_p++;
	targa_header.attributes = *buf_p++;

	if( targa_header.image_type != 2 && targa_header.image_type != 10 && targa_header.image_type != 3 )
	{
		common->Error( "LoadTGA( %s ): Only type 2 (RGB), 3 (gray), and 10 (RGB) TGA images supported\n", name );
	}

	if( targa_header.colormap_type != 0 )
	{
		common->Error( "LoadTGA( %s ): colormaps not supported\n", name );
	}

	if( ( targa_header.pixel_size != 32 && targa_header.pixel_size != 24 ) && targa_header.image_type != 3 )
	{
		common->Error( "LoadTGA( %s ): Only 32 or 24 bit images supported (no colormaps)\n", name );
	}

	if( targa_header.image_type == 2 || targa_header.image_type == 3 )
	{
		numBytes = targa_header.width * targa_header.height * ( targa_header.pixel_size >> 3 );
		if( numBytes > fileSize - 18 - targa_header.id_length )
		{
			common->Error( "LoadTGA( %s ): incomplete file\n", name );
		}
	}

	columns = targa_header.width;
	rows = targa_header.height;
	numPixels = columns * rows;

	if( width )
	{
		*width = columns;
	}
	if( height )
	{
		*height = rows;
	}

	targa_rgba = ( byte* )R_StaticAlloc( numPixels * 4, TAG_IMAGE );
	*pic = targa_rgba;

	if( targa_header.id_length != 0 )
	{
		buf_p += targa_header.id_length;  // skip TARGA image comment
	}

	if( targa_header.image_type == 2 || targa_header.image_type == 3 )
	{
		// Uncompressed RGB or gray scale image
		for( row = rows - 1; row >= 0; row-- )
		{
			pixbuf = targa_rgba + row * columns * 4;
			for( column = 0; column < columns; column++ )
			{
				unsigned char red, green, blue, alphabyte;
				switch( targa_header.pixel_size )
				{

					case 8:
						blue = *buf_p++;
						green = blue;
						red = blue;
						*pixbuf++ = red;
						*pixbuf++ = green;
						*pixbuf++ = blue;
						*pixbuf++ = 255;
						break;

					case 24:
						blue = *buf_p++;
						green = *buf_p++;
						red = *buf_p++;
						*pixbuf++ = red;
						*pixbuf++ = green;
						*pixbuf++ = blue;
						*pixbuf++ = 255;
						break;
					case 32:
						blue = *buf_p++;
						green = *buf_p++;
						red = *buf_p++;
						alphabyte = *buf_p++;
						*pixbuf++ = red;
						*pixbuf++ = green;
						*pixbuf++ = blue;
						*pixbuf++ = alphabyte;
						break;
					default:
						common->Error( "LoadTGA( %s ): illegal pixel_size '%d'\n", name, targa_header.pixel_size );
						break;
				}
			}
		}
	}
	else if( targa_header.image_type == 10 )      // Runlength encoded RGB images
	{
		unsigned char red, green, blue, alphabyte, packetHeader, packetSize, j;

		red = 0;
		green = 0;
		blue = 0;
		alphabyte = 0xff;

		for( row = rows - 1; row >= 0; row-- )
		{
			pixbuf = targa_rgba + row * columns * 4;
			for( column = 0; column < columns; )
			{
				packetHeader = *buf_p++;
				packetSize = 1 + ( packetHeader & 0x7f );
				if( packetHeader & 0x80 )           // run-length packet
				{
					switch( targa_header.pixel_size )
					{
						case 24:
							blue = *buf_p++;
							green = *buf_p++;
							red = *buf_p++;
							alphabyte = 255;
							break;
						case 32:
							blue = *buf_p++;
							green = *buf_p++;
							red = *buf_p++;
							alphabyte = *buf_p++;
							break;
						default:
							common->Error( "LoadTGA( %s ): illegal pixel_size '%d'\n", name, targa_header.pixel_size );
							break;
					}

					for( j = 0; j < packetSize; j++ )
					{
						*pixbuf++ = red;
						*pixbuf++ = green;
						*pixbuf++ = blue;
						*pixbuf++ = alphabyte;
						column++;
						if( column == columns )    // run spans across rows
						{
							column = 0;
							if( row > 0 )
							{
								row--;
							}
							else
							{
								goto breakOut;
							}
							pixbuf = targa_rgba + row * columns * 4;
						}
					}
				}
				else                              // non run-length packet
				{
					for( j = 0; j < packetSize; j++ )
					{
						switch( targa_header.pixel_size )
						{
							case 24:
								blue = *buf_p++;
								green = *buf_p++;
								red = *buf_p++;
								*pixbuf++ = red;
								*pixbuf++ = green;
								*pixbuf++ = blue;
								*pixbuf++ = 255;
								break;
							case 32:
								blue = *buf_p++;
								green = *buf_p++;
								red = *buf_p++;
								alphabyte = *buf_p++;
								*pixbuf++ = red;
								*pixbuf++ = green;
								*pixbuf++ = blue;
								*pixbuf++ = alphabyte;
								break;
							default:
								common->Error( "LoadTGA( %s ): illegal pixel_size '%d'\n", name, targa_header.pixel_size );
								break;
						}
						column++;
						if( column == columns )    // pixel packet run spans across rows
						{
							column = 0;
							if( row > 0 )
							{
								row--;
							}
							else
							{
								goto breakOut;
							}
							pixbuf = targa_rgba + row * columns * 4;
						}
					}
				}
			}
breakOut:
			;
		}
	}

	if( ( targa_header.attributes & ( 1 << 5 ) ) )  			// image flp bit
	{
		if( width != NULL && height != NULL )
		{
			R_VerticalFlip( *pic, *width, *height );
		}
	}

	fileSystem->FreeFile( buffer );
}

/*
=========================================================

JPG LOADING

Interfaces with the huge libjpeg
=========================================================
*/

/*
=============
LoadJPG
=============
*/
static void LoadJPG( const char* filename, unsigned char** pic, int* width, int* height, ID_TIME_T* timestamp )
{
	/* This struct contains the JPEG decompression parameters and pointers to
	 * working space (which is allocated as needed by the JPEG library).
	 */
	struct jpeg_decompress_struct cinfo;
	/* We use our private extension JPEG error handler.
	 * Note that this struct must live as long as the main JPEG parameter
	 * struct, to avoid dangling-pointer problems.
	 */
	/* This struct represents a JPEG error handler.  It is declared separately
	 * because applications often want to supply a specialized error handler
	 * (see the second half of this file for an example).  But here we just
	 * take the easy way out and use the standard error handler, which will
	 * print a message on stderr and call exit() if compression fails.
	 * Note that this struct must live as long as the main JPEG parameter
	 * struct, to avoid dangling-pointer problems.
	 */
	struct jpeg_error_mgr jerr;
	/* More stuff */
	JSAMPARRAY buffer;		/* Output row buffer */
	int row_stride;		/* physical row width in output buffer */
	unsigned char* out;
	byte*	fbuffer;
	byte*  bbuf;
	int     len;

	/* In this example we want to open the input file before doing anything else,
	 * so that the setjmp() error recovery below can assume the file is open.
	 * VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
	 * requires it in order to read binary files.
	 */

	// JDC: because fill_input_buffer() blindly copies INPUT_BUF_SIZE bytes,
	// we need to make sure the file buffer is padded or it may crash
	if( pic )
	{
		*pic = NULL;		// until proven otherwise
	}
	{
		idFile* f;

		f = fileSystem->OpenFileRead( filename );
		if( !f )
		{
			return;
		}
		len = f->Length();
		if( timestamp )
		{
			*timestamp = f->Timestamp();
		}
		if( !pic )
		{
			fileSystem->CloseFile( f );
			return;	// just getting timestamp
		}
		fbuffer = ( byte* )Mem_ClearedAlloc( len + 4096, TAG_JPG );
		f->Read( fbuffer, len );
		fileSystem->CloseFile( f );
	}


	/* Step 1: allocate and initialize JPEG decompression object */

	/* We have to set up the error handler first, in case the initialization
	 * step fails.  (Unlikely, but it could happen if you are out of memory.)
	 * This routine fills in the contents of struct jerr, and returns jerr's
	 * address which we place into the link field in cinfo.
	 */
	cinfo.err = jpeg_std_error( &jerr );

	/* Now we can initialize the JPEG decompression object. */
	jpeg_create_decompress( &cinfo );

	/* Step 2: specify data source (eg, a file) */

#ifdef USE_NEWER_JPEG
	jpeg_mem_src( &cinfo, fbuffer, len );
#else
	jpeg_stdio_src( &cinfo, fbuffer );
#endif
	/* Step 3: read file parameters with jpeg_read_header() */

	jpeg_read_header( &cinfo, true );
	/* We can ignore the return value from jpeg_read_header since
	 *   (a) suspension is not possible with the stdio data source, and
	 *   (b) we passed TRUE to reject a tables-only JPEG file as an error.
	 * See libjpeg.doc for more info.
	 */

	/* Step 4: set parameters for decompression */

	/* In this example, we don't need to change any of the defaults set by
	 * jpeg_read_header(), so we do nothing here.
	 */

	/* Step 5: Start decompressor */

	jpeg_start_decompress( &cinfo );
	/* We can ignore the return value since suspension is not possible
	 * with the stdio data source.
	 */

	/* We may need to do some setup of our own at this point before reading
	 * the data.  After jpeg_start_decompress() we have the correct scaled
	 * output image dimensions available, as well as the output colormap
	 * if we asked for color quantization.
	 * In this example, we need to make an output work buffer of the right size.
	 */
	/* JSAMPLEs per row in output buffer */
	row_stride = cinfo.output_width * cinfo.output_components;

	if( cinfo.output_components != 4 )
	{
		common->DWarning( "JPG %s is unsupported color depth (%d)",
						  filename, cinfo.output_components );
	}
	out = ( byte* )R_StaticAlloc( cinfo.output_width * cinfo.output_height * 4, TAG_IMAGE );

	*pic = out;
	*width = cinfo.output_width;
	*height = cinfo.output_height;

	/* Step 6: while (scan lines remain to be read) */
	/*           jpeg_read_scanlines(...); */

	/* Here we use the library's state variable cinfo.output_scanline as the
	 * loop counter, so that we don't have to keep track ourselves.
	 */
	while( cinfo.output_scanline < cinfo.output_height )
	{
		/* jpeg_read_scanlines expects an array of pointers to scanlines.
		 * Here the array is only one element long, but you could ask for
		 * more than one scanline at a time if that's more convenient.
		 */
		bbuf = ( ( out + ( row_stride * cinfo.output_scanline ) ) );
		buffer = &bbuf;
		jpeg_read_scanlines( &cinfo, buffer, 1 );
	}

	// clear all the alphas to 255
	{
		int	i, j;
		byte*	buf;

		buf = *pic;

		j = cinfo.output_width * cinfo.output_height * 4;
		for( i = 3 ; i < j ; i += 4 )
		{
			buf[i] = 255;
		}
	}

	/* Step 7: Finish decompression */

	jpeg_finish_decompress( &cinfo );
	/* We can ignore the return value since suspension is not possible
	 * with the stdio data source.
	 */

	/* Step 8: Release JPEG decompression object */

	/* This is an important step since it will release a good deal of memory. */
	jpeg_destroy_decompress( &cinfo );

	/* After finish_decompress, we can close the input file.
	 * Here we postpone it until after no more JPEG errors are possible,
	 * so as to simplify the setjmp error logic above.  (Actually, I don't
	 * think that jpeg_destroy can do an error exit, but why assume anything...)
	 */
	Mem_Free( fbuffer );

	/* At this point you may want to check to see whether any corrupt-data
	 * warnings occurred (test whether jerr.pub.num_warnings is nonzero).
	 */

	/* And we're done! */
}

// RB begin
/*
=========================================================

PNG LOADING

=========================================================
*/

extern "C"
{
#include <string.h>
#include <png.h>


	static void png_Error( png_structp pngPtr, png_const_charp msg )
	{
		common->FatalError( "%s", msg );
	}

	static void png_Warning( png_structp pngPtr, png_const_charp msg )
	{
		common->Warning( "%s", msg );
	}

	static void	png_ReadData( png_structp pngPtr, png_bytep data, png_size_t length )
	{
#if PNG_LIBPNG_VER_MAJOR == 1 && PNG_LIBPNG_VER_MINOR <= 4
		memcpy( data, ( byte* )pngPtr->io_ptr, length );

		pngPtr->io_ptr = ( ( byte* ) pngPtr->io_ptr ) + length;
#else
		// There is a get_io_ptr but not a set_io_ptr.. Therefore we need some tmp storage here.
		byte** ioptr = ( byte** )png_get_io_ptr( pngPtr );
		memcpy( data, *ioptr, length );
		*ioptr += length;
#endif
	}

}

/*
=============
LoadPNG
=============
*/
void LoadPNG( const char* filename, unsigned char** pic, int* width, int* height, ID_TIME_T* timestamp )
{
	byte*	fbuffer;
#if PNG_LIBPNG_VER_MAJOR > 1 || PNG_LIBPNG_VER_MINOR > 4
	byte*   readptr;
#endif

	if( !pic )
	{
		fileSystem->ReadFile( filename, NULL, timestamp );
		return;	// just getting timestamp
	}

	*pic = NULL;

	//
	// load the file
	//
	int fileSize = fileSystem->ReadFile( filename, ( void** )&fbuffer, timestamp );
	if( !fbuffer )
	{
		return;
	}

	// create png_struct with the custom error handlers
	png_structp pngPtr = png_create_read_struct( PNG_LIBPNG_VER_STRING, ( png_voidp ) NULL, png_Error, png_Warning );
	if( !pngPtr )
	{
		common->Error( "LoadPNG( %s ): png_create_read_struct failed", filename );
	}

	// allocate the memory for image information
	png_infop infoPtr = png_create_info_struct( pngPtr );
	if( !infoPtr )
	{
		common->Error( "LoadPNG( %s ): png_create_info_struct failed", filename );
	}

#if PNG_LIBPNG_VER_MAJOR == 1 && PNG_LIBPNG_VER_MINOR <= 4
	png_set_read_fn( pngPtr, fbuffer, png_ReadData );
#else
	readptr = fbuffer;
	png_set_read_fn( pngPtr, &readptr, png_ReadData );
#endif

	png_set_sig_bytes( pngPtr, 0 );

	png_read_info( pngPtr, infoPtr );

	png_uint_32 pngWidth, pngHeight;
	int bitDepth, colorType, interlaceType;
	png_get_IHDR( pngPtr, infoPtr, &pngWidth, &pngHeight, &bitDepth, &colorType, &interlaceType, NULL, NULL );

	// 16 bit -> 8 bit
	png_set_strip_16( pngPtr );

	// 1, 2, 4 bit -> 8 bit
	if( bitDepth < 8 )
	{
		png_set_packing( pngPtr );
	}

#if 1
	if( colorType & PNG_COLOR_MASK_PALETTE )
	{
		png_set_expand( pngPtr );
	}

	if( !( colorType & PNG_COLOR_MASK_COLOR ) )
	{
		png_set_gray_to_rgb( pngPtr );
	}

#else
	if( colorType == PNG_COLOR_TYPE_PALETTE )
	{
		png_set_palette_to_rgb( pngPtr );
	}

	if( colorType == PNG_COLOR_TYPE_GRAY && bitDepth < 8 )
	{
		png_set_expand_gray_1_2_4_to_8( pngPtr );
	}
#endif

	// set paletted or RGB images with transparency to full alpha so we get RGBA
	if( png_get_valid( pngPtr, infoPtr, PNG_INFO_tRNS ) )
	{
		png_set_tRNS_to_alpha( pngPtr );
	}

	// make sure every pixel has an alpha value
	if( !( colorType & PNG_COLOR_MASK_ALPHA ) )
	{
		png_set_filler( pngPtr, 255, PNG_FILLER_AFTER );
	}

	png_read_update_info( pngPtr, infoPtr );

	byte* out = ( byte* )R_StaticAlloc( pngWidth * pngHeight * 4, TAG_IMAGE );

	*pic = out;
	*width = pngWidth;
	*height = pngHeight;

	png_uint_32 rowBytes = png_get_rowbytes( pngPtr, infoPtr );

	png_bytep* rowPointers = ( png_bytep* ) R_StaticAlloc( sizeof( png_bytep ) * pngHeight );
	for( png_uint_32 row = 0; row < pngHeight; row++ )
	{
		rowPointers[row] = ( png_bytep )( out + ( row * pngWidth * 4 ) );
	}

	png_read_image( pngPtr, rowPointers );

	png_read_end( pngPtr, infoPtr );

	png_destroy_read_struct( &pngPtr, &infoPtr, NULL );

	R_StaticFree( rowPointers );
	Mem_Free( fbuffer );

	// RB: PNG needs to be flipped to match the .tga behavior
	// edit: this is wrong and flips images UV mapped in Blender 2.83
	//R_VerticalFlip( *pic, *width, *height );
}


extern "C"
{

	static int png_compressedSize = 0;
	static void	png_WriteData( png_structp pngPtr, png_bytep data, png_size_t length )
	{
#if PNG_LIBPNG_VER_MAJOR == 1 && PNG_LIBPNG_VER_MINOR <= 4
		memcpy( ( byte* )pngPtr->io_ptr, data, length );
		pngPtr->io_ptr = ( ( byte* ) pngPtr->io_ptr ) + length;
#else
		byte** ioptr = ( byte** )png_get_io_ptr( pngPtr );
		memcpy( *ioptr, data, length );
		*ioptr += length;
#endif
		png_compressedSize += length;
	}

	static void	png_FlushData( png_structp pngPtr ) { }

}

/*
================
R_WritePNG
================
*/
void R_WritePNG( const char* filename, const byte* data, int bytesPerPixel, int width, int height, bool flipVertical, const char* basePath )
{
	png_structp pngPtr = png_create_write_struct( PNG_LIBPNG_VER_STRING, NULL, png_Error, png_Warning );
	if( !pngPtr )
	{
		common->Error( "R_WritePNG( %s ): png_create_write_struct failed", filename );
	}

	png_infop infoPtr = png_create_info_struct( pngPtr );
	if( !infoPtr )
	{
		common->Error( "R_WritePNG( %s ): png_create_info_struct failed", filename );
	}

	png_compressedSize = 0;
	byte* buffer = ( byte* ) Mem_Alloc( width * height * bytesPerPixel, TAG_TEMP );
#if PNG_LIBPNG_VER_MAJOR == 1 && PNG_LIBPNG_VER_MINOR <= 4
	png_set_write_fn( pngPtr, buffer, png_WriteData, png_FlushData );
#else
	byte* ioptr  = buffer;
	png_set_write_fn( pngPtr, &ioptr, png_WriteData, png_FlushData );
#endif

	if( bytesPerPixel == 4 )
	{
		png_set_IHDR( pngPtr, infoPtr, width, height, 8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT );
	}
	else if( bytesPerPixel == 3 )
	{
		png_set_IHDR( pngPtr, infoPtr, width, height, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT );
	}
	else
	{
		common->Error( "R_WritePNG( %s ): bytesPerPixel = %i not supported", filename, bytesPerPixel );
	}

	// write header
	png_write_info( pngPtr, infoPtr );

	png_bytep* rowPointers = ( png_bytep* ) Mem_Alloc( sizeof( png_bytep ) * height, TAG_TEMP );

	if( !flipVertical )
	{
		for( int row = 0, flippedRow = height - 1; row < height; row++, flippedRow-- )
		{
			rowPointers[flippedRow] = ( png_bytep )( data + ( row * width * bytesPerPixel ) );
		}
	}
	else
	{
		for( int row = 0; row < height; row++ )
		{
			rowPointers[row] = ( png_bytep )( data + ( row * width * bytesPerPixel ) );
		}
	}

	png_write_image( pngPtr, rowPointers );
	png_write_end( pngPtr, infoPtr );

	png_destroy_write_struct( &pngPtr, &infoPtr );

	Mem_Free( rowPointers );

	fileSystem->WriteFile( filename, buffer, png_compressedSize, basePath );

	Mem_Free( buffer );
}

/*
=========================================================

EXR LOADING

Interfaces with tinyexr
=========================================================
*/

/*
=======================
LoadEXR
=======================
*/
static void LoadEXR( const char* filename, unsigned char** pic, int* width, int* height, ID_TIME_T* timestamp )
{
	if( !pic )
	{
		fileSystem->ReadFile( filename, NULL, timestamp );
		return;	// just getting timestamp
	}

	*pic = NULL;

	// load the file
	const byte* fbuffer = NULL;
	int fileSize = fileSystem->ReadFile( filename, ( void** )&fbuffer, timestamp );
	if( !fbuffer )
	{
		return;
	}

	float* rgba;
	const char* err;

	{
		int ret = LoadEXRFromMemory( &rgba, width, height, fbuffer, fileSize, &err );
		if( ret != 0 )
		{
			common->Error( "LoadEXR( %s ): %s\n", filename, err );
			return;
		}
	}

#if 0
	// dump file as .hdr for testing - this works
	{
		idStrStatic< MAX_OSPATH > hdrFileName = "test";
		//hdrFileName.AppendPath( filename );
		hdrFileName.SetFileExtension( ".hdr" );

		int ret = stbi_write_hdr( hdrFileName.c_str(), *width, *height, 4, rgba );

		if( ret == 0 )
		{
			return; // fail
		}
	}
#endif

	if( rgba )
	{
		int32 pixelCount = *width * *height;
		byte* out = ( byte* )R_StaticAlloc( pixelCount * 4, TAG_IMAGE );

		*pic = out;

		// convert to packed R11G11B10F as uint32 for each pixel

		const float* src = rgba;
		byte* dst = out;
		for( int i = 0; i < pixelCount; i++ )
		{
			// read 3 floats and ignore the alpha channel
			float p[3];

			p[0] = src[0];
			p[1] = src[1];
			p[2] = src[2];

			// convert
			uint32_t value = float3_to_r11g11b10f( p );
			*( uint32_t* )dst = value;

			src += 4;
			dst += 4;
		}

		free( rgba );
	}

	// RB: EXR needs to be flipped to match the .tga behavior
	//R_VerticalFlip( *pic, *width, *height );

	Mem_Free( ( void* )fbuffer );
}

/*
================
R_WriteEXR
================
*/
void R_WriteEXR( const char* filename, const void* rgba16f, int channelsPerPixel, int width, int height, const char* basePath )
{
#if 0
	// miniexr.cpp - v0.2 - public domain - 2013 Aras Pranckevicius / Unity Technologies
	//
	// Writes OpenEXR RGB files out of half-precision RGBA or RGB data.
	//
	// Only tested on Windows (VS2008) and Mac (clang 3.3), little endian.
	// Testing status: "works for me".
	//
	// History:
	// 0.2 Source data can be RGB or RGBA now.
	// 0.1 Initial release.

	const unsigned ww = width - 1;
	const unsigned hh = height - 1;
	const unsigned char kHeader[] =
	{
		0x76, 0x2f, 0x31, 0x01, // magic
		2, 0, 0, 0, // version, scanline
		// channels
		'c', 'h', 'a', 'n', 'n', 'e', 'l', 's', 0,
		'c', 'h', 'l', 'i', 's', 't', 0,
		55, 0, 0, 0,
		'B', 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, // B, half
		'G', 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, // G, half
		'R', 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, // R, half
		0,
		// compression
		'c', 'o', 'm', 'p', 'r', 'e', 's', 's', 'i', 'o', 'n', 0,
		'c', 'o', 'm', 'p', 'r', 'e', 's', 's', 'i', 'o', 'n', 0,
		1, 0, 0, 0,
		0, // no compression
		// dataWindow
		'd', 'a', 't', 'a', 'W', 'i', 'n', 'd', 'o', 'w', 0,
		'b', 'o', 'x', '2', 'i', 0,
		16, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		uint8( ww & 0xFF ), uint8( ( ww >> 8 ) & 0xFF ), uint8( ( ww >> 16 ) & 0xFF ), uint8( ( ww >> 24 ) & 0xFF ),
		uint8( hh & 0xFF ), uint8( ( hh >> 8 ) & 0xFF ), uint8( ( hh >> 16 ) & 0xFF ), uint8( ( hh >> 24 ) & 0xFF ),
		// displayWindow
		'd', 'i', 's', 'p', 'l', 'a', 'y', 'W', 'i', 'n', 'd', 'o', 'w', 0,
		'b', 'o', 'x', '2', 'i', 0,
		16, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		uint8( ww & 0xFF ), uint8( ( ww >> 8 ) & 0xFF ), uint8( ( ww >> 16 ) & 0xFF ), uint8( ( ww >> 24 ) & 0xFF ),
		uint8( hh & 0xFF ), uint8( ( hh >> 8 ) & 0xFF ), uint8( ( hh >> 16 ) & 0xFF ), uint8( ( hh >> 24 ) & 0xFF ),
		// lineOrder
		'l', 'i', 'n', 'e', 'O', 'r', 'd', 'e', 'r', 0,
		'l', 'i', 'n', 'e', 'O', 'r', 'd', 'e', 'r', 0,
		1, 0, 0, 0,
		0, // increasing Y
		// pixelAspectRatio
		'p', 'i', 'x', 'e', 'l', 'A', 's', 'p', 'e', 'c', 't', 'R', 'a', 't', 'i', 'o', 0,
		'f', 'l', 'o', 'a', 't', 0,
		4, 0, 0, 0,
		0, 0, 0x80, 0x3f, // 1.0f
		// screenWindowCenter
		's', 'c', 'r', 'e', 'e', 'n', 'W', 'i', 'n', 'd', 'o', 'w', 'C', 'e', 'n', 't', 'e', 'r', 0,
		'v', '2', 'f', 0,
		8, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		// screenWindowWidth
		's', 'c', 'r', 'e', 'e', 'n', 'W', 'i', 'n', 'd', 'o', 'w', 'W', 'i', 'd', 't', 'h', 0,
		'f', 'l', 'o', 'a', 't', 0,
		4, 0, 0, 0,
		0, 0, 0x80, 0x3f, // 1.0f
		// end of header
		0,
	};
	const int kHeaderSize = sizeof( kHeader );

	const int kScanlineTableSize = 8 * height;
	const unsigned pixelRowSize = width * 3 * 2;
	const unsigned fullRowSize = pixelRowSize + 8;

	unsigned bufSize = kHeaderSize + kScanlineTableSize + height * fullRowSize;
	unsigned char* buf = ( unsigned char* )Mem_Alloc( bufSize, TAG_TEMP );
	if( !buf )
	{
		return;
	}

	// copy in header
	memcpy( buf, kHeader, kHeaderSize );

	// line offset table
	unsigned ofs = kHeaderSize + kScanlineTableSize;
	unsigned char* ptr = buf + kHeaderSize;
	for( int y = 0; y < height; ++y )
	{
		*ptr++ = ofs & 0xFF;
		*ptr++ = ( ofs >> 8 ) & 0xFF;
		*ptr++ = ( ofs >> 16 ) & 0xFF;
		*ptr++ = ( ofs >> 24 ) & 0xFF;
		*ptr++ = 0;
		*ptr++ = 0;
		*ptr++ = 0;
		*ptr++ = 0;
		ofs += fullRowSize;
	}

	// scanline data
	const unsigned char* src = ( const unsigned char* )rgba16f;
	const int stride = channelsPerPixel * 2;
	for( int y = 0; y < height; ++y )
	{
		// coordinate
		*ptr++ = y & 0xFF;
		*ptr++ = ( y >> 8 ) & 0xFF;
		*ptr++ = ( y >> 16 ) & 0xFF;
		*ptr++ = ( y >> 24 ) & 0xFF;
		// data size
		*ptr++ = pixelRowSize & 0xFF;
		*ptr++ = ( pixelRowSize >> 8 ) & 0xFF;
		*ptr++ = ( pixelRowSize >> 16 ) & 0xFF;
		*ptr++ = ( pixelRowSize >> 24 ) & 0xFF;
		// B, G, R
		const unsigned char* chsrc;
		chsrc = src + 4;
		for( int x = 0; x < width; ++x )
		{
			*ptr++ = chsrc[0];
			*ptr++ = chsrc[1];
			chsrc += stride;
		}
		chsrc = src + 2;
		for( int x = 0; x < width; ++x )
		{
			*ptr++ = chsrc[0];
			*ptr++ = chsrc[1];
			chsrc += stride;
		}
		chsrc = src + 0;
		for( int x = 0; x < width; ++x )
		{
			*ptr++ = chsrc[0];
			*ptr++ = chsrc[1];
			chsrc += stride;
		}

		src += width * stride;
	}

	assert( ptr - buf == bufSize );

	fileSystem->WriteFile( filename, buf, bufSize, basePath );

	Mem_Free( buf );

#else

	// TinyEXR version with compression to save disc size

	if( channelsPerPixel != 3 )
	{
		common->Error( "R_WriteEXR( %s ): channelsPerPixel = %i not supported", filename, channelsPerPixel );
	}

	EXRHeader header;
	InitEXRHeader( &header );

	EXRImage image;
	InitEXRImage( &image );

	image.num_channels = 3;

	std::vector<halfFloat_t> images[3];
	images[0].resize( width * height );
	images[1].resize( width * height );
	images[2].resize( width * height );

	halfFloat_t* rgb = ( halfFloat_t* ) rgba16f;

	for( int i = 0; i < width * height; i++ )
	{
		images[0][i] = ( rgb[3 * i + 0] );
		images[1][i] = ( rgb[3 * i + 1] );
		images[2][i] = ( rgb[3 * i + 2] );
	}

	halfFloat_t* image_ptr[3];
	image_ptr[0] = &( images[2].at( 0 ) ); // B
	image_ptr[1] = &( images[1].at( 0 ) ); // G
	image_ptr[2] = &( images[0].at( 0 ) ); // R

	image.images = ( unsigned char** )image_ptr;
	image.width = width;
	image.height = height;

	header.num_channels = 3;
	header.channels = ( EXRChannelInfo* )malloc( sizeof( EXRChannelInfo ) * header.num_channels );

	// Must be BGR(A) order, since most of EXR viewers expect this channel order.
	strncpy( header.channels[0].name, "B", 255 );
	header.channels[0].name[strlen( "B" )] = '\0';
	strncpy( header.channels[1].name, "G", 255 );
	header.channels[1].name[strlen( "G" )] = '\0';
	strncpy( header.channels[2].name, "R", 255 );
	header.channels[2].name[strlen( "R" )] = '\0';

	header.pixel_types = ( int* )malloc( sizeof( int ) * header.num_channels );
	header.requested_pixel_types = ( int* )malloc( sizeof( int ) * header.num_channels );
	for( int i = 0; i < header.num_channels; i++ )
	{
		header.pixel_types[i] = TINYEXR_PIXELTYPE_FLOAT; // pixel type of input image
		header.requested_pixel_types[i] = TINYEXR_PIXELTYPE_HALF; // pixel type of output image to be stored in .EXR
	}

	header.compression_type = TINYEXR_COMPRESSIONTYPE_ZIP;

	byte* buffer = NULL;
	const char* err;
	size_t size = SaveEXRImageToMemory( &image, &header, &buffer, &err );
	if( size == 0 )
	{
		common->Error( "R_WriteEXR( %s ): Save EXR err: %s\n", filename, err );

		goto cleanup;
	}

	fileSystem->WriteFile( filename, buffer, size, basePath );

cleanup:
	free( header.channels );
	free( header.pixel_types );
	free( header.requested_pixel_types );

#endif
}
// RB end


/*
=========================================================

HDR LOADING

Interfaces with stb_image
=========================================================
*/


/*
=======================
LoadHDR

RB: load floating point data from memory and convert it into packed R11G11B10F data
=======================
*/
static void LoadHDR( const char* filename, unsigned char** pic, int* width, int* height, ID_TIME_T* timestamp )
{
	if( !pic )
	{
		fileSystem->ReadFile( filename, NULL, timestamp );
		return;	// just getting timestamp
	}

	*pic = NULL;

	// load the file
	const byte* fbuffer = NULL;
	int fileSize = fileSystem->ReadFile( filename, ( void** )&fbuffer, timestamp );
	if( !fbuffer )
	{
		return;
	}

	int32 numChannels;

	float* rgba = stbi_loadf_from_memory( ( stbi_uc const* ) fbuffer, fileSize, width, height, &numChannels, 0 );

	if( numChannels != 3 )
	{
		common->Error( "LoadHDR( %s ): HDR has not 3 channels\n", filename );
	}

	if( rgba )
	{
		int32 pixelCount = *width * *height;
		byte* out = ( byte* )R_StaticAlloc( pixelCount * 4, TAG_IMAGE );

		*pic = out;

		// convert to packed R11G11B10F as uint32 for each pixel

		const float* src = rgba;
		byte* dst = out;
		for( int i = 0; i < pixelCount; i++ )
		{
			// read 3 floats and ignore the alpha channel
			float p[3];

			p[0] = src[0];
			p[1] = src[1];
			p[2] = src[2];

			// convert
			uint32_t value = float3_to_r11g11b10f( p );
			*( uint32_t* )dst = value;

			src += 4;
			dst += 4;
		}

		free( rgba );
	}

	Mem_Free( ( void* )fbuffer );
}

//===================================================================


typedef struct
{
	const char*	ext;
	void	( *ImageLoader )( const char* filename, unsigned char** pic, int* width, int* height, ID_TIME_T* timestamp );
} imageExtToLoader_t;

static imageExtToLoader_t imageLoaders[] =
{
	{"png", LoadPNG},
	{"tga", LoadTGA},
	{"jpg", LoadJPG},
	{"exr", LoadEXR},
	{"hdr", LoadHDR},
};

static const int numImageLoaders = sizeof( imageLoaders ) / sizeof( imageLoaders[0] );

/*
=================
R_LoadImage

Loads any of the supported image types into a cannonical
32 bit format.

Automatically attempts to load .jpg files if .tga files fail to load.

*pic will be NULL if the load failed.

Anything that is going to make this into a texture would use
makePowerOf2 = true, but something loading an image as a lookup
table of some sort would leave it in identity form.

It is important to do this at image load time instead of texture load
time for bump maps.

Timestamp may be NULL if the value is going to be ignored

If pic is NULL, the image won't actually be loaded, it will just find the
timestamp.
=================
*/
void R_LoadImage( const char* cname, byte** pic, int* width, int* height, ID_TIME_T* timestamp, bool makePowerOf2, textureUsage_t* usage )
{
	idStr name = cname;

	if( pic )
	{
		*pic = NULL;
	}
	if( timestamp )
	{
		*timestamp = FILE_NOT_FOUND_TIMESTAMP;
	}
	if( width )
	{
		*width = 0;
	}
	if( height )
	{
		*height = 0;
	}

	name.DefaultFileExtension( ".tga" );

	if( name.Length() < 5 )
	{
		return;
	}

	name.ToLower();
	idStr ext;
	name.ExtractFileExtension( ext );
	idStr origName = name;

	// RB begin

	// PBR HACK - look for the same file name that provides a _rmao[d] suffix and prefer it
	// if it is available, otherwise
	bool pbrImageLookup = false;
	if( usage && *usage == TD_SPECULAR )
	{
		name.StripFileExtension();

		if( name.StripTrailingOnce( "_s" ) )
		{
			name += "_rmao";
		}

		ext = "png";
		name.DefaultFileExtension( ".png" );

		pbrImageLookup = true;
	}
#if 0
	else if( usage && *usage == TD_R11G11B10F )
	{
		name.StripFileExtension();

		ext = "exr";
		name.DefaultFileExtension( ".exr" );
	}
#endif

retry:

	// try
	if( !ext.IsEmpty() )
	{
		// try only the image with the specified extension: default .tga
		int i;
		for( i = 0; i < numImageLoaders; i++ )
		{
			if( !ext.Icmp( imageLoaders[i].ext ) )
			{
				imageLoaders[i].ImageLoader( name.c_str(), pic, width, height, timestamp );
				break;
			}
		}

		if( i < numImageLoaders )
		{
			if( ( pic && *pic == NULL ) || ( timestamp && *timestamp == FILE_NOT_FOUND_TIMESTAMP ) )
			{
				// image with the specified extension was not found so try all extensions
				for( i = 0; i < numImageLoaders; i++ )
				{
					name.SetFileExtension( imageLoaders[i].ext );
					imageLoaders[i].ImageLoader( name.c_str(), pic, width, height, timestamp );

					if( pic && *pic != NULL )
					{
						//idLib::Warning( "image %s failed to load, using %s instead", origName.c_str(), name.c_str());
						break;
					}

					if( !pic && timestamp && *timestamp != FILE_NOT_FOUND_TIMESTAMP )
					{
						// we are only interested in the timestamp and we got one
						break;
					}
				}
			}
		}

		if( pbrImageLookup )
		{
			if( ( pic && *pic == NULL ) || ( !pic && timestamp && *timestamp == FILE_NOT_FOUND_TIMESTAMP ) )
			{
				name = origName;
				name.ExtractFileExtension( ext );

				pbrImageLookup = false;
				goto retry;
			}

			if( ( pic && *pic != NULL ) || ( !pic && timestamp && *timestamp != FILE_NOT_FOUND_TIMESTAMP ) )
			{
				idLib::Printf( "PBR hack: using '%s' instead of '%s'\n", name.c_str(), origName.c_str() );
				*usage = TD_SPECULAR_PBR_RMAO;
			}
		}
	}
	// RB end

	if( ( width && *width < 1 ) || ( height && *height < 1 ) )
	{
		if( pic && *pic )
		{
			R_StaticFree( *pic );
			*pic = 0;
		}
	}

	//
	// convert to exact power of 2 sizes
	//
	/*
	if ( pic && *pic && makePowerOf2 ) {
		int		w, h;
		int		scaled_width, scaled_height;
		byte	*resampledBuffer;

		w = *width;
		h = *height;

		for (scaled_width = 1 ; scaled_width < w ; scaled_width<<=1)
			;
		for (scaled_height = 1 ; scaled_height < h ; scaled_height<<=1)
			;

		if ( scaled_width != w || scaled_height != h ) {
			resampledBuffer = R_ResampleTexture( *pic, w, h, scaled_width, scaled_height );
			R_StaticFree( *pic );
			*pic = resampledBuffer;
			*width = scaled_width;
			*height = scaled_height;
		}
	}
	*/
}

/*
=======================
R_LoadCubeImages

Loads six files with proper extensions
=======================
*/
bool R_LoadCubeImages( const char* imgName, cubeFiles_t extensions, byte* pics[6], int* outSize, ID_TIME_T* timestamp, int cubeMapSize )
{
	int		i, j;
	const char*	cameraSides[6] =  { "_forward.tga", "_back.tga", "_left.tga", "_right.tga",
									"_up.tga", "_down.tga"
								  };
	const char*	axisSides[6] =  { "_px.tga", "_nx.tga", "_py.tga", "_ny.tga",
								  "_pz.tga", "_nz.tga"
								};
	const char**	sides;
	char	fullName[MAX_IMAGE_NAME];
	int		width, height, size = 0;

	if( extensions == CF_CAMERA )
	{
		sides = cameraSides;
	}
	else
	{
		sides = axisSides;
	}

	// FIXME: precompressed cube map files
	if( pics )
	{
		memset( pics, 0, 6 * sizeof( pics[0] ) );
	}
	if( timestamp )
	{
		*timestamp = 0;
	}

	if( extensions == CF_SINGLE && cubeMapSize != 0 )
	{
		ID_TIME_T thisTime;
		byte* thisPic[1];
		thisPic[0] = nullptr;

		if( pics )
		{
			R_LoadImageProgram( imgName, thisPic, &width, &height, &thisTime );
		}
		else
		{
			// load just the timestamps
			R_LoadImageProgram( imgName, nullptr, &width, &height, &thisTime );
		}


		if( thisTime == FILE_NOT_FOUND_TIMESTAMP )
		{
			return false;
		}

		if( timestamp )
		{
			if( thisTime > *timestamp )
			{
				*timestamp = thisTime;
			}
		}

		if( pics )
		{
			*outSize = cubeMapSize;

			for( int i = 0; i < 6; i++ )
			{
				pics[i] = R_GenerateCubeMapSideFromSingleImage( thisPic[0], width, height, cubeMapSize, i );
				switch( i )
				{
					case 0:	// forward
						R_RotatePic( pics[i], cubeMapSize );
						break;
					case 1:	// back
						R_RotatePic( pics[i], cubeMapSize );
						R_HorizontalFlip( pics[i], cubeMapSize, cubeMapSize );
						R_VerticalFlip( pics[i], cubeMapSize, cubeMapSize );
						break;
					case 2:	// left
						R_VerticalFlip( pics[i], cubeMapSize, cubeMapSize );
						break;
					case 3:	// right
						R_HorizontalFlip( pics[i], cubeMapSize, cubeMapSize );
						break;
					case 4:	// up
						R_RotatePic( pics[i], cubeMapSize );
						break;
					case 5: // down
						R_RotatePic( pics[i], cubeMapSize );
						break;
				}
			}

			R_StaticFree( thisPic[0] );
		}

		return true;
	}

	for( i = 0 ; i < 6 ; i++ )
	{
		idStr::snPrintf( fullName, sizeof( fullName ), "%s%s", imgName, sides[i] );

		ID_TIME_T thisTime;
		if( !pics )
		{
			// just checking timestamps
			R_LoadImageProgram( fullName, NULL, &width, &height, &thisTime );
		}
		else
		{
			R_LoadImageProgram( fullName, &pics[i], &width, &height, &thisTime );
		}

		if( thisTime == FILE_NOT_FOUND_TIMESTAMP )
		{
			break;
		}
		if( i == 0 )
		{
			size = width;
		}
		if( width != size || height != size )
		{
			common->Warning( "Mismatched sizes on cube map '%s'", imgName );
			break;
		}
		if( timestamp )
		{
			if( thisTime > *timestamp )
			{
				*timestamp = thisTime;
			}
		}
		if( pics && extensions == CF_CAMERA )
		{
			// convert from "camera" images to native cube map images
			switch( i )
			{
				case 0:	// forward
					R_RotatePic( pics[i], width );
					break;
				case 1:	// back
					R_RotatePic( pics[i], width );
					R_HorizontalFlip( pics[i], width, height );
					R_VerticalFlip( pics[i], width, height );
					break;
				case 2:	// left
					R_VerticalFlip( pics[i], width, height );
					break;
				case 3:	// right
					R_HorizontalFlip( pics[i], width, height );
					break;
				case 4:	// up
					R_RotatePic( pics[i], width );
					break;
				case 5: // down
					R_RotatePic( pics[i], width );
					break;
			}
		}
	}

	if( i != 6 )
	{
		// we had an error, so free everything
		if( pics )
		{
			for( j = 0 ; j < i ; j++ )
			{
				R_StaticFree( pics[j] );
			}
		}

		if( timestamp )
		{
			*timestamp = 0;
		}
		return false;
	}

	if( outSize )
	{
		*outSize = size;
	}
	return true;
}
