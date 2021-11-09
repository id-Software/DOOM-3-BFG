/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013-2015 Robert Beckebans

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
#include "../renderer/Font.h"
#include "../renderer/Image.h"
//#include "../../libs/rapidjson/include/rapidjson/document.h"

using namespace rapidjson;

#pragma warning(disable: 4355) // 'this' : used in base member initializer list

#define BSWF_VERSION 16		// bumped to 16 for storing atlas image dimensions for unbuffered loads
#define BSWF_MAGIC ( ( 'B' << 24 ) | ( 'S' << 16 ) | ( 'W' << 8 ) | BSWF_VERSION )

// RB begin
#define XSWF_VERSION 16
// RB end

/*
===================
idSWF::LoadSWF
===================
*/
bool idSWF::LoadSWF( const char* fullpath )
{

	idFile* rawfile = fileSystem->OpenFileRead( fullpath );
	if( rawfile == NULL )
	{
		idLib::Printf( "SWF File not found %s\n", fullpath );
		return false;
	}

	swfHeader_t header;
	rawfile->Read( &header, sizeof( header ) );

	if( header.W != 'W' || header.S != 'S' )
	{
		idLib::Warning( "Wrong signature bytes" );
		delete rawfile;
		return false;
	}

	if( header.version > 9 )
	{
		idLib::Warning( "Unsupported version %d", header.version );
		delete rawfile;
		return false;
	}

	bool compressed;
	if( header.compression == 'F' )
	{
		compressed = false;
	}
	else if( header.compression == 'C' )
	{
		compressed = true;
	}
	else
	{
		idLib::Warning( "Unsupported compression type %c", header.compression );
		delete rawfile;
		return false;
	}
	idSwap::Little( header.fileLength );

	// header.fileLength somewhat annoyingly includes the size of the header
	uint32 fileLength2 = header.fileLength - ( uint32 )sizeof( swfHeader_t );

	// slurp the raw file into a giant array, which is somewhat atrocious when loading from the preload since it's already an idFile_Memory
	byte* fileData = ( byte* )Mem_Alloc( fileLength2, TAG_SWF );
	size_t fileSize = rawfile->Read( fileData, fileLength2 );
	delete rawfile;

	if( compressed )
	{
		byte* uncompressed = ( byte* )Mem_Alloc( fileLength2, TAG_SWF );
		if( !Inflate( fileData, ( int )fileSize, uncompressed, fileLength2 ) )
		{
			idLib::Warning( "Inflate error" );
			Mem_Free( uncompressed );
			return false;
		}
		Mem_Free( fileData );
		fileData = uncompressed;
	}
	idSWFBitStream bitstream( fileData, fileLength2, false );

	swfRect_t frameSize;
	bitstream.ReadRect( frameSize );

	if( !frameSize.tl.Compare( vec2_zero ) )
	{
		idLib::Warning( "Invalid frameSize top left" );
		Mem_Free( fileData );
		return false;
	}

	frameWidth = frameSize.br.x;
	frameHeight = frameSize.br.y;
	frameRate = bitstream.ReadU16();

	// parse everything
	mainsprite->Load( bitstream, true );

	// now that all images have been loaded, write out the combined image
	idStr atlasFileName = "generated/";
	atlasFileName += fullpath;
	atlasFileName.SetFileExtension( ".png" );

	WriteSwfImageAtlas( atlasFileName );

	Mem_Free( fileData );

	return true;
}

// RB: write new .swf with only the information we care about
void idSWF::WriteSWF( const char* swfFilename, const byte* atlasImageRGBA, int atlasImageWidth, int atlasImageHeight )
{
	idFile_SWF file( fileSystem->OpenFileWrite( swfFilename, "fs_basepath" ) );
	if( file == NULL )
	{
		return;
	}

	swfHeader_t header;
	header.W = 'W';
	header.S = 'S';
	header.version = 9;
	header.compression = 'F';

	file.Write( &header, sizeof( header ) );

	swfRect_t frameSize;
	frameSize.br.x = frameWidth;
	frameSize.br.y = frameHeight;

	//int fileSize1 = file->Length();

	file.WriteRect( frameSize );

	//int fileSize2 = file->Length();

	file.WriteU16( frameRate );

	file.WriteU16( mainsprite->GetFrameCount() );

	// write FileAttributes tag required for Flash version >= 8
	file.WriteTagHeader( Tag_FileAttributes, 4 );

	file.WriteUBits( 0, 1 );				// Reserved, must be 0
	file.WriteUBits( 0, 1 );				// UseDirectBlit
	file.WriteUBits( 0, 1 );				// UseGPU
	file.WriteUBits( 0, 1 );				// HasMetadata

	file.WriteUBits( 0, 1 );				// ActionScript3
	file.WriteUBits( 0, 1 );				// Reserved, must be 0
	file.WriteUBits( 0, 1 );				// UseNetwork

	file.WriteUBits( 0, 24 );				// Reserved, must be 0

	file.ByteAlign();

	for( int i = 0; i < dictionary.Num(); i++ )
	{
		const idSWFDictionaryEntry& entry = dictionary[i];

		switch( dictionary[i].type )
		{
			case SWF_DICT_IMAGE:
			{
				int width = entry.imageSize[0];
				int height = entry.imageSize[1];

				uint32 colorDataSize = width * height * 4;
				idTempArray<byte> colorData( colorDataSize );

				idTempArray<byte> pngData( colorDataSize );

				for( int h = 0; h < height; h++ )
				{
					for( int w = 0; w < width; w++ )
					{
						int atlasPixelOfs = ( entry.imageAtlasOffset[0] + w + ( ( entry.imageAtlasOffset[1] + h ) * atlasImageWidth ) ) * 4;

						//atlasPixelOfs = idMath::ClampInt( atlasPixelOfs

						const byte* atlasPixel = atlasImageRGBA + atlasPixelOfs;

						byte* pixel = &colorData[( w + ( h * width ) ) * 4];

						pixel[0] = atlasPixel[3];
						pixel[1] = atlasPixel[0];
						pixel[2] = atlasPixel[1];
						pixel[3] = atlasPixel[2];

						pixel = &pngData[( w + ( h * width ) ) * 4];
						pixel[0] = atlasPixel[0];
						pixel[1] = atlasPixel[1];
						pixel[2] = atlasPixel[2];
						pixel[3] = atlasPixel[3];
					}
				}

				idStr imageExportFileName;
				idStr filenameWithoutExt = filename;
				filenameWithoutExt.StripFileExtension();
				sprintf( imageExportFileName, "exported/%s/image_characterid_%i.png", filenameWithoutExt.c_str(), i );

				R_WritePNG( imageExportFileName.c_str(), pngData.Ptr(), 4, width, height, true, "fs_basepath" );

				// RB: add some extra space for zlib
				idTempArray<byte> compressedData( width * height * 4 * 1.02 + 12 );
				int compressedDataSize = compressedData.Size();

				if( !Deflate( colorData.Ptr(), colorDataSize, ( byte* )compressedData.Ptr(), compressedDataSize ) )
				{
					idLib::Warning( "DefineBitsLossless: Failed to deflate bitmap data" );
					//return;
				}

				int tagLength = ( 2 + 1 + 2 + 2 ) + compressedDataSize;

				file.WriteTagHeader( Tag_DefineBitsLossless2, tagLength );

				file.WriteU16( i );						// characterID
				file.WriteU8( 5 );						// format
				file.WriteU16( width );					// width
				file.WriteU16( height );				// height
				file->Write( compressedData.Ptr(), compressedDataSize );
				break;
			}

#if 0
			case SWF_DICT_SHAPE:
			{
				idSWFShape* shape = dictionary[i].shape;

				int numFillDraws = 0;
				for( int d = 0; d < shape->fillDraws.Num(); d++ )
				{
					idSWFShapeDrawFill& fillDraw = shape->fillDraws[d];

					if( fillDraw.style.type == 0 /* || fillDraw.style.type == 4 */ )
					{
						numFillDraws++;
					}
				}

				if( numFillDraws == 0 )
				{
					continue;
				}

				idFile_Memory* tagMem = new idFile_Memory( "shapeTag" );
				idFile_SWF tag( tagMem );

				tag.WriteU16( i );						// characterID
				tag.WriteRect( shape->startBounds );

				if( numFillDraws >= 0xFF )
				{
					tag.WriteU8( 0xFF );
					tag.WriteU16( numFillDraws );
				}
				else
				{
					tag.WriteU8( numFillDraws );
				}

				for( int d = 0; d < shape->fillDraws.Num(); d++ )
				{
					idSWFShapeDrawFill& fillDraw = shape->fillDraws[d];


					if( fillDraw.style.type == 0 )
					{
						// solid
						tag.WriteU8( 0x00 );
						tag.WriteColorRGBA( fillDraw.style.startColor );
					}
					/*
					else
						if( fillDraw.style.type == 4 )
					{
						// bitmap
						tag.WriteU8( 0x40 );

						tag.WriteU16( fillDraw.style.bitmapID );
						tag.WriteMatrix( fillDraw.style.startMatrix );
					}
					*/


					// type: 0 = solid, 1 = gradient, 4 = bitmap
					//if( fillDraw.style.type == 0 )

					//file->WriteBig( fillDraw.style.type );
					//file->WriteBig( fillDraw.style.subType );
					//file->Write( &fillDraw.style.startColor, 4 );
					//file->Write( &fillDraw.style.endColor, 4 );
					/*file->WriteBigArray( ( float* )&fillDraw.style.startMatrix, 6 );
					file->WriteBigArray( ( float* )&fillDraw.style.endMatrix, 6 );
					file->WriteBig( fillDraw.style.gradient.numGradients );
					for( int g = 0; g < fillDraw.style.gradient.numGradients; g++ )
					{
						file->WriteBig( fillDraw.style.gradient.gradientRecords[g].startRatio );
						file->WriteBig( fillDraw.style.gradient.gradientRecords[g].endRatio );
						file->Write( &fillDraw.style.gradient.gradientRecords[g].startColor, 4 );
						file->Write( &fillDraw.style.gradient.gradientRecords[g].endColor, 4 );
					}
					file->WriteBig( fillDraw.style.focalPoint );
					file->WriteBig( fillDraw.style.bitmapID );
					file->WriteBig( fillDraw.startVerts.Num() );
					file->WriteBigArray( fillDraw.startVerts.Ptr(), fillDraw.startVerts.Num() );
					file->WriteBig( fillDraw.endVerts.Num() );
					file->WriteBigArray( fillDraw.endVerts.Ptr(), fillDraw.endVerts.Num() );
					file->WriteBig( fillDraw.indices.Num() );
					file->WriteBigArray( fillDraw.indices.Ptr(), fillDraw.indices.Num() );*/
				}

				// TODO
				tag.WriteU8( 0 ); // no lines
				tag.WriteU8( 1 ); // no shapes

				/*
				file->WriteBig( shape->lineDraws.Num() );
				for( int d = 0; d < shape->lineDraws.Num(); d++ )
				{
					idSWFShapeDrawLine& lineDraw = shape->lineDraws[d];
					file->WriteBig( lineDraw.style.startWidth );
					file->WriteBig( lineDraw.style.endWidth );
					file->Write( &lineDraw.style.startColor, 4 );
					file->Write( &lineDraw.style.endColor, 4 );
					file->WriteBig( lineDraw.startVerts.Num() );
					file->WriteBigArray( lineDraw.startVerts.Ptr(), lineDraw.startVerts.Num() );
					file->WriteBig( lineDraw.endVerts.Num() );
					file->WriteBigArray( lineDraw.endVerts.Ptr(), lineDraw.endVerts.Num() );
					file->WriteBig( lineDraw.indices.Num() );
					file->WriteBigArray( lineDraw.indices.Ptr(), lineDraw.indices.Num() );
				}
				*/

				file.WriteTagHeader( Tag_DefineShape3, tagMem->Length() );
				file.Write( tagMem->GetDataPtr(), tagMem->Length() );
				break;
			}
#endif

			case SWF_DICT_SPRITE:
			{
				dictionary[i].sprite->WriteSWF( file, i );
				break;
			}
		}
	}

	mainsprite->WriteSWF( file, dictionary.Num() );

	// add Tag_End
	file.WriteTagHeader( Tag_End, 0 );

	// go back and write filesize into header
	uint32 fileSize = file->Length();

	file->Seek( offsetof( swfHeader_t, fileLength ), FS_SEEK_SET );
	file.WriteU32( fileSize );
}
// RB end

/*
===================
idSWF::LoadBinary
===================
*/
bool idSWF::LoadBinary( const char* bfilename, ID_TIME_T sourceTimeStamp )
{
	idFile* f = fileSystem->OpenFileReadMemory( bfilename );
	if( f == NULL || f->Length() <= 0 )
	{
		return false;
	}

	uint32 magic = 0;
	ID_TIME_T btimestamp = 0;
	f->ReadBig( magic );
	f->ReadBig( btimestamp );

	// RB: source might be from .resources, so we ignore the time stamp and assume a release build
	if( magic != BSWF_MAGIC || ( !fileSystem->InProductionMode() && ( sourceTimeStamp != FILE_NOT_FOUND_TIMESTAMP ) && ( sourceTimeStamp != 0 ) && ( sourceTimeStamp != btimestamp ) ) )
	{
		delete f;
		return false;
	}
	// RB end

	f->ReadBig( frameWidth );
	f->ReadBig( frameHeight );
	f->ReadBig( frameRate );

	if( mouseX == -1 )
	{
		mouseX = ( frameWidth / 2 );
	}

	if( mouseY == -1 )
	{
		mouseY = ( frameHeight / 2 );
	}

	mainsprite->Read( f );

	int num = 0;
	f->ReadBig( num );
	dictionary.SetNum( num );
	for( int i = 0; i < dictionary.Num(); i++ )
	{
		f->ReadBig( dictionary[i].type );
		switch( dictionary[i].type )
		{
			case SWF_DICT_IMAGE:
			{
				idStr imageName;
				f->ReadString( imageName );
				if( imageName[0] == '.' )
				{
					// internal image in the atlas
					dictionary[i].material = NULL;
				}
				else
				{
					dictionary[i].material = declManager->FindMaterial( imageName );
				}
				for( int j = 0 ; j < 2 ; j++ )
				{
					f->ReadBig( dictionary[i].imageSize[j] );
					f->ReadBig( dictionary[i].imageAtlasOffset[j] );
				}
				for( int j = 0 ; j < 4 ; j++ )
				{
					f->ReadBig( dictionary[i].channelScale[j] );
				}
				break;
			}
			case SWF_DICT_MORPH:
			case SWF_DICT_SHAPE:
			{
				dictionary[i].shape = new( TAG_SWF ) idSWFShape;
				idSWFShape* shape = dictionary[i].shape;
				f->ReadBig( shape->startBounds.tl );
				f->ReadBig( shape->startBounds.br );
				f->ReadBig( shape->endBounds.tl );
				f->ReadBig( shape->endBounds.br );
				f->ReadBig( num );
				shape->fillDraws.SetNum( num );
				for( int d = 0; d < shape->fillDraws.Num(); d++ )
				{
					idSWFShapeDrawFill& fillDraw = shape->fillDraws[d];
					f->ReadBig( fillDraw.style.type );
					f->ReadBig( fillDraw.style.subType );
					f->Read( &fillDraw.style.startColor, 4 );
					f->Read( &fillDraw.style.endColor, 4 );
					f->ReadBigArray( ( float* )&fillDraw.style.startMatrix, 6 );
					f->ReadBigArray( ( float* )&fillDraw.style.endMatrix, 6 );
					f->ReadBig( fillDraw.style.gradient.numGradients );
					for( int g = 0; g < fillDraw.style.gradient.numGradients; g++ )
					{
						f->ReadBig( fillDraw.style.gradient.gradientRecords[g].startRatio );
						f->ReadBig( fillDraw.style.gradient.gradientRecords[g].endRatio );
						f->Read( &fillDraw.style.gradient.gradientRecords[g].startColor, 4 );
						f->Read( &fillDraw.style.gradient.gradientRecords[g].endColor, 4 );
					}
					f->ReadBig( fillDraw.style.focalPoint );
					f->ReadBig( fillDraw.style.bitmapID );
					f->ReadBig( num );
					fillDraw.startVerts.SetNum( num );
					f->ReadBigArray( fillDraw.startVerts.Ptr(), fillDraw.startVerts.Num() );
					f->ReadBig( num );
					fillDraw.endVerts.SetNum( num );
					f->ReadBigArray( fillDraw.endVerts.Ptr(), fillDraw.endVerts.Num() );
					f->ReadBig( num );
					fillDraw.indices.SetNum( num );
					f->ReadBigArray( fillDraw.indices.Ptr(), fillDraw.indices.Num() );
				}
				f->ReadBig( num );
				shape->lineDraws.SetNum( num );
				for( int d = 0; d < shape->lineDraws.Num(); d++ )
				{
					idSWFShapeDrawLine& lineDraw = shape->lineDraws[d];
					f->ReadBig( lineDraw.style.startWidth );
					f->ReadBig( lineDraw.style.endWidth );
					f->Read( &lineDraw.style.startColor, 4 );
					f->Read( &lineDraw.style.endColor, 4 );
					f->ReadBig( num );
					lineDraw.startVerts.SetNum( num );
					f->ReadBigArray( lineDraw.startVerts.Ptr(), lineDraw.startVerts.Num() );
					f->ReadBig( num );
					lineDraw.endVerts.SetNum( num );
					f->ReadBigArray( lineDraw.endVerts.Ptr(), lineDraw.endVerts.Num() );
					f->ReadBig( num );
					lineDraw.indices.SetNum( num );
					f->ReadBigArray( lineDraw.indices.Ptr(), lineDraw.indices.Num() );
				}
				break;
			}
			case SWF_DICT_SPRITE:
			{
				dictionary[i].sprite = new( TAG_SWF ) idSWFSprite( this );
				dictionary[i].sprite->Read( f );
				break;
			}
			case SWF_DICT_FONT:
			{
				dictionary[i].font = new( TAG_SWF ) idSWFFont;
				idSWFFont* font = dictionary[i].font;
				idStr fontName;
				f->ReadString( fontName );
				font->fontID = renderSystem->RegisterFont( fontName );
				f->ReadBig( font->ascent );
				f->ReadBig( font->descent );
				f->ReadBig( font->leading );
				f->ReadBig( num );
				font->glyphs.SetNum( num );
				for( int g = 0; g < font->glyphs.Num(); g++ )
				{
					f->ReadBig( font->glyphs[g].code );
					f->ReadBig( font->glyphs[g].advance );
					f->ReadBig( num );
					font->glyphs[g].verts.SetNum( num );
					f->ReadBigArray( font->glyphs[g].verts.Ptr(), font->glyphs[g].verts.Num() );
					f->ReadBig( num );
					font->glyphs[g].indices.SetNum( num );
					f->ReadBigArray( font->glyphs[g].indices.Ptr(), font->glyphs[g].indices.Num() );
				}
				break;
			}
			case SWF_DICT_TEXT:
			{
				dictionary[i].text = new( TAG_SWF ) idSWFText;
				idSWFText* text = dictionary[i].text;
				f->ReadBig( text->bounds.tl );
				f->ReadBig( text->bounds.br );
				f->ReadBigArray( ( float* )&text->matrix, 6 );
				f->ReadBig( num );
				text->textRecords.SetNum( num );
				for( int t = 0; t < text->textRecords.Num(); t++ )
				{
					idSWFTextRecord& textRecord = text->textRecords[t];
					f->ReadBig( textRecord.fontID );
					f->Read( &textRecord.color, 4 );
					f->ReadBig( textRecord.xOffset );
					f->ReadBig( textRecord.yOffset );
					f->ReadBig( textRecord.textHeight );
					f->ReadBig( textRecord.firstGlyph );
					f->ReadBig( textRecord.numGlyphs );
				}
				f->ReadBig( num );
				text->glyphs.SetNum( num );
				for( int g = 0; g < text->glyphs.Num(); g++ )
				{
					f->ReadBig( text->glyphs[g].index );
					f->ReadBig( text->glyphs[g].advance );
				}
				break;
			}
			case SWF_DICT_EDITTEXT:
			{
				dictionary[i].edittext = new( TAG_SWF ) idSWFEditText;
				idSWFEditText* edittext = dictionary[i].edittext;
				f->ReadBig( edittext->bounds.tl );
				f->ReadBig( edittext->bounds.br );
				f->ReadBig( edittext->flags );
				f->ReadBig( edittext->fontID );
				f->ReadBig( edittext->fontHeight );
				f->Read( &edittext->color, 4 );
				f->ReadBig( edittext->maxLength );
				f->ReadBig( edittext->align );
				f->ReadBig( edittext->leftMargin );
				f->ReadBig( edittext->rightMargin );
				f->ReadBig( edittext->indent );
				f->ReadBig( edittext->leading );
				f->ReadString( edittext->variable );
				f->ReadString( edittext->initialText );
				break;
			}
		}
	}
	delete f;

	return true;
}

/*
===================
idSWF::WriteBinary
===================
*/
void idSWF::WriteBinary( const char* bfilename )
{
	idFileLocal file( fileSystem->OpenFileWrite( bfilename, "fs_basepath" ) );
	if( file == NULL )
	{
		return;
	}
	file->WriteBig( BSWF_MAGIC );
	file->WriteBig( timestamp );

	file->WriteBig( frameWidth );
	file->WriteBig( frameHeight );
	file->WriteBig( frameRate );

	mainsprite->Write( file );

	file->WriteBig( dictionary.Num() );
	for( int i = 0; i < dictionary.Num(); i++ )
	{
		file->WriteBig( dictionary[i].type );
		switch( dictionary[i].type )
		{
			case SWF_DICT_IMAGE:
			{
				if( dictionary[i].material )
				{
					file->WriteString( dictionary[i].material->GetName() );
				}
				else
				{
					file->WriteString( "." );
				}
				for( int j = 0 ; j < 2 ; j++ )
				{
					file->WriteBig( dictionary[i].imageSize[j] );
					file->WriteBig( dictionary[i].imageAtlasOffset[j] );
				}
				for( int j = 0 ; j < 4 ; j++ )
				{
					file->WriteBig( dictionary[i].channelScale[j] );
				}
				break;
			}
			case SWF_DICT_MORPH:
			case SWF_DICT_SHAPE:
			{
				idSWFShape* shape = dictionary[i].shape;
				file->WriteBig( shape->startBounds.tl );
				file->WriteBig( shape->startBounds.br );
				file->WriteBig( shape->endBounds.tl );
				file->WriteBig( shape->endBounds.br );
				file->WriteBig( shape->fillDraws.Num() );
				for( int d = 0; d < shape->fillDraws.Num(); d++ )
				{
					idSWFShapeDrawFill& fillDraw = shape->fillDraws[d];
					file->WriteBig( fillDraw.style.type );
					file->WriteBig( fillDraw.style.subType );
					file->Write( &fillDraw.style.startColor, 4 );
					file->Write( &fillDraw.style.endColor, 4 );
					file->WriteBigArray( ( float* )&fillDraw.style.startMatrix, 6 );
					file->WriteBigArray( ( float* )&fillDraw.style.endMatrix, 6 );
					file->WriteBig( fillDraw.style.gradient.numGradients );
					for( int g = 0; g < fillDraw.style.gradient.numGradients; g++ )
					{
						file->WriteBig( fillDraw.style.gradient.gradientRecords[g].startRatio );
						file->WriteBig( fillDraw.style.gradient.gradientRecords[g].endRatio );
						file->Write( &fillDraw.style.gradient.gradientRecords[g].startColor, 4 );
						file->Write( &fillDraw.style.gradient.gradientRecords[g].endColor, 4 );
					}
					file->WriteBig( fillDraw.style.focalPoint );
					file->WriteBig( fillDraw.style.bitmapID );
					file->WriteBig( fillDraw.startVerts.Num() );
					file->WriteBigArray( fillDraw.startVerts.Ptr(), fillDraw.startVerts.Num() );
					file->WriteBig( fillDraw.endVerts.Num() );
					file->WriteBigArray( fillDraw.endVerts.Ptr(), fillDraw.endVerts.Num() );
					file->WriteBig( fillDraw.indices.Num() );
					file->WriteBigArray( fillDraw.indices.Ptr(), fillDraw.indices.Num() );
				}
				file->WriteBig( shape->lineDraws.Num() );
				for( int d = 0; d < shape->lineDraws.Num(); d++ )
				{
					idSWFShapeDrawLine& lineDraw = shape->lineDraws[d];
					file->WriteBig( lineDraw.style.startWidth );
					file->WriteBig( lineDraw.style.endWidth );
					file->Write( &lineDraw.style.startColor, 4 );
					file->Write( &lineDraw.style.endColor, 4 );
					file->WriteBig( lineDraw.startVerts.Num() );
					file->WriteBigArray( lineDraw.startVerts.Ptr(), lineDraw.startVerts.Num() );
					file->WriteBig( lineDraw.endVerts.Num() );
					file->WriteBigArray( lineDraw.endVerts.Ptr(), lineDraw.endVerts.Num() );
					file->WriteBig( lineDraw.indices.Num() );
					file->WriteBigArray( lineDraw.indices.Ptr(), lineDraw.indices.Num() );
				}
				break;
			}
			case SWF_DICT_SPRITE:
			{
				dictionary[i].sprite->Write( file );
				break;
			}
			case SWF_DICT_FONT:
			{
				idSWFFont* font = dictionary[i].font;
				file->WriteString( font->fontID->GetName() );
				file->WriteBig( font->ascent );
				file->WriteBig( font->descent );
				file->WriteBig( font->leading );
				file->WriteBig( font->glyphs.Num() );
				for( int g = 0; g < font->glyphs.Num(); g++ )
				{
					file->WriteBig( font->glyphs[g].code );
					file->WriteBig( font->glyphs[g].advance );
					file->WriteBig( font->glyphs[g].verts.Num() );
					file->WriteBigArray( font->glyphs[g].verts.Ptr(), font->glyphs[g].verts.Num() );
					file->WriteBig( font->glyphs[g].indices.Num() );
					file->WriteBigArray( font->glyphs[g].indices.Ptr(), font->glyphs[g].indices.Num() );
				}
				break;
			}
			case SWF_DICT_TEXT:
			{
				idSWFText* text = dictionary[i].text;
				file->WriteBig( text->bounds.tl );
				file->WriteBig( text->bounds.br );
				file->WriteBigArray( ( float* )&text->matrix, 6 );
				file->WriteBig( text->textRecords.Num() );
				for( int t = 0; t < text->textRecords.Num(); t++ )
				{
					idSWFTextRecord& textRecord = text->textRecords[t];
					file->WriteBig( textRecord.fontID );
					file->Write( &textRecord.color, 4 );
					file->WriteBig( textRecord.xOffset );
					file->WriteBig( textRecord.yOffset );
					file->WriteBig( textRecord.textHeight );
					file->WriteBig( textRecord.firstGlyph );
					file->WriteBig( textRecord.numGlyphs );
				}
				file->WriteBig( text->glyphs.Num() );
				for( int g = 0; g < text->glyphs.Num(); g++ )
				{
					file->WriteBig( text->glyphs[g].index );
					file->WriteBig( text->glyphs[g].advance );
				}
				break;
			}
			case SWF_DICT_EDITTEXT:
			{
				idSWFEditText* edittext = dictionary[i].edittext;
				file->WriteBig( edittext->bounds.tl );
				file->WriteBig( edittext->bounds.br );
				file->WriteBig( edittext->flags );
				file->WriteBig( edittext->fontID );
				file->WriteBig( edittext->fontHeight );
				file->Write( &edittext->color, 4 );
				file->WriteBig( edittext->maxLength );
				file->WriteBig( edittext->align );
				file->WriteBig( edittext->leftMargin );
				file->WriteBig( edittext->rightMargin );
				file->WriteBig( edittext->indent );
				file->WriteBig( edittext->leading );
				file->WriteString( edittext->variable );
				file->WriteString( edittext->initialText );
				break;
			}
		}
	}
}


/*
===================
idSWF::FileAttributes
Extra data that won't fit in a SWF header
===================
*/
void idSWF::FileAttributes( idSWFBitStream& bitstream )
{
	bitstream.Seek( 5 ); // 5 booleans
}

/*
===================
idSWF::Metadata
===================
*/
void idSWF::Metadata( idSWFBitStream& bitstream )
{
	bitstream.ReadString(); // XML string
}

/*
===================
idSWF::SetBackgroundColor
===================
*/
void idSWF::SetBackgroundColor( idSWFBitStream& bitstream )
{
	bitstream.Seek( 4 ); // int
}

// RB begin

/*
===================
idSWF::LoadJSON
===================
*/
bool idSWF::LoadJSON( const char* filename )
{
	idFile* f = fileSystem->OpenFileReadMemory( filename );
	if( f == NULL || f->Length() <= 0 )
	{
		return false;
	}

	int fileLength = f->Length();
	const char* fileData = ( const char* )Mem_Alloc( fileLength, TAG_SWF );
	size_t fileSize = f->Read( ( byte* ) fileData, fileLength );
	delete f;

	rapidjson::Document d;
	d.Parse( fileData );

	assert( d.IsObject() );


	if( d.HasMember( "version" ) )
	{
		Value& s = d["version"];
		int version = s.GetInt();

		//idLib::Printf( "version = %i", version );
	}

	//ID_TIME_T btimestamp = s.GetInt64();
	//f->ReadBig( btimestamp );

	// these values can be trash
	frameWidth = d["frameWidth"].GetDouble();
	frameHeight = d["frameHeight"].GetDouble();
	frameRate = d["frameRate"].GetUint();

	if( mouseX == -1 )
	{
		mouseX = ( frameWidth / 2 );
	}

	if( mouseY == -1 )
	{
		mouseY = ( frameHeight / 2 );
	}

	Value& a = d["dict"];
	assert( a.IsArray() );

	// do not include the last item which is the main sprite
	dictionary.SetNum( a.Size() - 1 );

	for( SizeType i = 0; i < a.Size(); i++ )
	{
		Value& entry = a[i];
		idStr type = entry["type"].GetString();

		if( type == "IMAGE" )
		{
			//dictionary[i].type = SWF_DICT_IMAGE;

			idStrStatic< MAX_OSPATH > imageName = entry["imageFile"].GetString();
			if( imageName[0] == '.' )
			{
				// internal image in the atlas
				dictionary[i].material = NULL;
			}
			else
			{
				dictionary[i].material = declManager->FindMaterial( imageName );
			}

			//dictionary[i].imageSize[0] = entry["width"].GetDouble();
			//dictionary[i].imageSize[1] = entry["height"].GetDouble();

			//idVec4& channelScale = dictionary[i].channelScale;
			//channelScale.x = entry["channelScale"]["x"].GetDouble();
			//channelScale.y = entry["channelScale"]["y"].GetDouble();
			//channelScale.z = entry["channelScale"]["z"].GetDouble();
			//channelScale.w = entry["channelScale"]["w"].GetDouble();

			byte* imageData = NULL;
			int width, height;
			ID_TIME_T timestamp;
			R_LoadImage( imageName.c_str(), &imageData, &width, &height, &timestamp, false, NULL );
			if( imageData != NULL )
			{
				LoadImage( i, imageData, width, height );

				Mem_Free( imageData );
			}
		}
		else if( type == "SHAPE" || type == "MORPH" )
		{
			if( type == "MORPH" )
			{
				dictionary[i].type = SWF_DICT_MORPH;
			}
			else
			{
				dictionary[i].type = SWF_DICT_SHAPE;
			}

			dictionary[i].shape = new( TAG_SWF ) idSWFShape;

			idSWFShape* shape = dictionary[i].shape;

			shape->startBounds.tl.x = entry["startBounds"][0].GetDouble();
			shape->startBounds.tl.y = entry["startBounds"][1].GetDouble();
			shape->startBounds.br.x = entry["startBounds"][2].GetDouble();
			shape->startBounds.br.y = entry["startBounds"][3].GetDouble();

			shape->endBounds.tl.x = entry["endBounds"][0].GetDouble();
			shape->endBounds.tl.y = entry["endBounds"][1].GetDouble();
			shape->endBounds.br.x = entry["endBounds"][2].GetDouble();
			shape->endBounds.br.y = entry["endBounds"][3].GetDouble();

			if( entry.HasMember( "fillDraws" ) )
			{
				shape->fillDraws.SetNum( entry["fillDraws"].Size() );
				for( int d = 0; d < shape->fillDraws.Num(); d++ )
				{
					idSWFShapeDrawFill& fillDraw = shape->fillDraws[d];
					Value& jsonDraw = entry["fillDraws"][d];

					Value& style = jsonDraw["style"];
					idStr type = style["type"].GetString();

					// 0 = solid, 1 = gradient, 4 = bitmap
					if( type == "gradient" )
					{
						fillDraw.style.type = 1;
					}
					else if( type == "bitmap" )
					{
						fillDraw.style.type = 4;
					}
					else if( type == "solid" )
					{
						fillDraw.style.type = 0;
					}
					else
					{
						// unknown
						fillDraw.style.type = 0;
					}

					// 0 = linear, 2 = radial, 3 = focal; 0 = repeat, 1 = clamp, 2 = near repeat, 3 = near clamp
					idStr subType = style["subType"].GetString();

					if( subType == "linear" ||  subType == "repeat" )
					{
						fillDraw.style.subType = 0;
					}
					else if( subType == "radial" || subType == "near repeat" )
					{
						fillDraw.style.subType = 2;
					}
					else if( subType == "focal" || subType == "near clamp" )
					{
						fillDraw.style.subType = 3;
					}
					else
					{
						// unknown
						fillDraw.style.subType = 0;
					}

					if( fillDraw.style.type == 0 ) //style.HasMember["startColor"] )// )
					{
						Value& startColor = style["startColor"];
						fillDraw.style.startColor.r = ( uint8 )( startColor[0].GetDouble() * 255 ) & 0xFF;
						fillDraw.style.startColor.g = ( uint8 )( startColor[1].GetDouble() * 255 ) & 0xFF;
						fillDraw.style.startColor.b = ( uint8 )( startColor[2].GetDouble() * 255 ) & 0xFF;
						fillDraw.style.startColor.a = ( uint8 )( startColor[3].GetDouble() * 255 ) & 0xFF;

						if( style.HasMember( "endColor" ) )
						{
							Value& endColor = style["endColor"];
							fillDraw.style.endColor.r = ( uint8 )( startColor[0].GetDouble() * 255 ) & 0xFF;
							fillDraw.style.endColor.g = ( uint8 )( startColor[1].GetDouble() * 255 ) & 0xFF;
							fillDraw.style.endColor.b = ( uint8 )( startColor[2].GetDouble() * 255 ) & 0xFF;
							fillDraw.style.endColor.a = ( uint8 )( startColor[3].GetDouble() * 255 ) & 0xFF;
						}
						else
						{
							fillDraw.style.endColor = fillDraw.style.startColor;
						}
					}

					if( fillDraw.style.type > 0 )
					{
						Value& startMatrix = style["startMatrix"];
						fillDraw.style.startMatrix.xx =  startMatrix[0].GetDouble();
						fillDraw.style.startMatrix.yy =  startMatrix[1].GetDouble();
						fillDraw.style.startMatrix.xy =  startMatrix[2].GetDouble();
						fillDraw.style.startMatrix.yx =  startMatrix[3].GetDouble();
						fillDraw.style.startMatrix.tx =  startMatrix[4].GetDouble();
						fillDraw.style.startMatrix.ty =  startMatrix[5].GetDouble();

						if( style.HasMember( "endMatrix" ) )
						{
							Value& endMatrix = style["endMatrix"];
							fillDraw.style.endMatrix.xx =  endMatrix[0].GetDouble();
							fillDraw.style.endMatrix.yy =  endMatrix[1].GetDouble();
							fillDraw.style.endMatrix.xy =  endMatrix[2].GetDouble();
							fillDraw.style.endMatrix.yx =  endMatrix[3].GetDouble();
							fillDraw.style.endMatrix.tx =  endMatrix[4].GetDouble();
							fillDraw.style.endMatrix.ty =  endMatrix[5].GetDouble();
						}
						else
						{
							fillDraw.style.endMatrix = fillDraw.style.startMatrix;
						}
					}

					// gradient
					if( fillDraw.style.type == 1 )
					{
						Value& gradients = style["gradients"];
						fillDraw.style.gradient.numGradients = gradients.Size();

						for( int g = 0; g < fillDraw.style.gradient.numGradients; g++ )
						{
							swfGradientRecord_t gr = fillDraw.style.gradient.gradientRecords[g];

							Value& gradientRecord = gradients[g];

							gr.startRatio = gradientRecord["startRatio"].GetUint() & 0xFF;
							gr.endRatio = gradientRecord["endRatio"].GetUint() & 0xFF;

							Value& startColor = gradientRecord["startColor"];
							gr.startColor.r = ( uint8 )( startColor[0].GetDouble() * 255 ) & 0xFF;
							gr.startColor.g = ( uint8 )( startColor[1].GetDouble() * 255 ) & 0xFF;
							gr.startColor.b = ( uint8 )( startColor[2].GetDouble() * 255 ) & 0xFF;
							gr.startColor.a = ( uint8 )( startColor[3].GetDouble() * 255 ) & 0xFF;

							Value& endColor = gradientRecord["endColor"];
							gr.endColor.r = ( uint8 )( startColor[0].GetDouble() * 255 ) & 0xFF;
							gr.endColor.g = ( uint8 )( startColor[1].GetDouble() * 255 ) & 0xFF;
							gr.endColor.b = ( uint8 )( startColor[2].GetDouble() * 255 ) & 0xFF;
							gr.endColor.a = ( uint8 )( startColor[3].GetDouble() * 255 ) & 0xFF;
						}

						if( style.HasMember( "focalPoint" ) )
						{
							fillDraw.style.focalPoint = style["focalPoint"].GetDouble();
						}
						else
						{
							fillDraw.style.focalPoint = 0;
						}
					}

					// bitmap
					if( fillDraw.style.type == 4 )
					{
						fillDraw.style.bitmapID = style["bitmapID"].GetUint();
					}
					else
					{
						fillDraw.style.bitmapID = 65535;
					}

					if( jsonDraw.HasMember( "startVerts" ) )
					{
						Value& startVerts = jsonDraw["startVerts"];

						fillDraw.startVerts.SetNum( startVerts.Size() );
						for( int v = 0; v < fillDraw.startVerts.Num(); v++ )
						{
							idVec2& vert = fillDraw.startVerts[v];

							vert.x = startVerts[v]["v"][0].GetDouble();
							vert.y = startVerts[v]["v"][1].GetDouble();
						}
					}

					if( jsonDraw.HasMember( "endVerts" ) )
					{
						// this is a morph shape

						Value& endVerts = jsonDraw["endVerts"];

						fillDraw.endVerts.SetNum( endVerts.Size() );
						for( int v = 0; v < fillDraw.endVerts.Num(); v++ )
						{
							idVec2& vert = fillDraw.endVerts[v];

							vert.x = endVerts[v]["v"][0].GetDouble();
							vert.y = endVerts[v]["v"][1].GetDouble();
						}
					}

					if( jsonDraw.HasMember( "indices" ) )
					{
						Value& indices = jsonDraw["indices"];
						fillDraw.indices.SetNum( indices.Size() );

#if 1
						for( int v = 0; v < fillDraw.indices.Num(); v++ )
						{
							uint16& vert = fillDraw.indices[v];

							vert = indices[v].GetUint();
						}
#else
						for( int v = fillDraw.indices.Num() - 1; v >= 0; v-- )
						{
							uint16& vert = fillDraw.indices[v];

							vert = indices[v].GetUint();
						}
#endif
					}
				}
			}

			if( entry.HasMember( "lineDraws" ) )
			{
				shape->lineDraws.SetNum( entry["lineDraws"].Size() );
				for( int d = 0; d < shape->lineDraws.Num(); d++ )
				{
					idSWFShapeDrawLine& lineDraw = shape->lineDraws[d];
					Value& jsonDraw = entry["lineDraw"][d];

					Value& style = jsonDraw["style"];
					lineDraw.style.startWidth = style["startWidth"].GetUint();
					lineDraw.style.endWidth = style["endWidth"].GetUint();

					Value& startColor = style["startColor"];
					lineDraw.style.startColor.r = ( uint8 )( startColor[0].GetDouble() * 255 ) & 0xFF;
					lineDraw.style.startColor.g = ( uint8 )( startColor[1].GetDouble() * 255 ) & 0xFF;
					lineDraw.style.startColor.b = ( uint8 )( startColor[2].GetDouble() * 255 ) & 0xFF;
					lineDraw.style.startColor.a = ( uint8 )( startColor[3].GetDouble() * 255 ) & 0xFF;

					if( style.HasMember( "endColor" ) )
					{
						Value& endColor = style["endColor"];
						lineDraw.style.endColor.r = ( uint8 )( startColor[0].GetDouble() * 255 ) & 0xFF;
						lineDraw.style.endColor.g = ( uint8 )( startColor[1].GetDouble() * 255 ) & 0xFF;
						lineDraw.style.endColor.b = ( uint8 )( startColor[2].GetDouble() * 255 ) & 0xFF;
						lineDraw.style.endColor.a = ( uint8 )( startColor[3].GetDouble() * 255 ) & 0xFF;
					}
					else
					{
						lineDraw.style.endColor = lineDraw.style.startColor;
					}

					Value& startVerts = jsonDraw["startVerts"];

					lineDraw.startVerts.SetNum( startVerts.Size() );
					for( int v = 0; v < lineDraw.startVerts.Num(); v++ )
					{
						idVec2& vert = lineDraw.startVerts[v];

						vert.x = startVerts[v]["v"][0].GetDouble();
						vert.y = startVerts[v]["v"][1].GetDouble();
					}

					if( jsonDraw.HasMember( "endVerts" ) )
					{
						// this is a morph shape

						Value& endVerts = jsonDraw["endVerts"];

						lineDraw.endVerts.SetNum( endVerts.Size() );
						for( int v = 0; v < lineDraw.endVerts.Num(); v++ )
						{
							idVec2& vert = lineDraw.endVerts[v];

							vert.x = endVerts[v]["v"][0].GetDouble();
							vert.y = endVerts[v]["v"][1].GetDouble();
						}
					}

					Value& indices = jsonDraw["indices"];
					lineDraw.indices.SetNum( indices.Size() );

#if 1
					for( int v = 0; v < lineDraw.indices.Num(); v++ )
					{
						uint16& vert = lineDraw.indices[v];

						vert = indices[v].GetUint();
					}
#else
					for( int v = fillDraw.indices.Num() - 1; v >= 0; v-- )
					{
						uint16& vert = fillDraw.indices[v];

						vert = indices[v].GetUint();
					}
#endif
				}
			}
		}
		else if( type == "SPRITE" )
		{
			if( entry.HasMember( "mainsprite" ) || i == ( a.Size() - 1 ) )
			{
				mainsprite->ReadJSON( entry );
			}
			else
			{
				dictionary[i].type = SWF_DICT_SPRITE;
				dictionary[i].sprite = new( TAG_SWF ) idSWFSprite( this );
				dictionary[i].sprite->ReadJSON( entry );
			}
		}
		else if( type == "FONT" )
		{
			dictionary[i].type = SWF_DICT_FONT;
			dictionary[i].font = new( TAG_SWF ) idSWFFont;

			idSWFFont* font = dictionary[i].font;
			idStr fontName = entry["name"].GetString();
			font->fontID = renderSystem->RegisterFont( fontName );


			font->ascent = entry["ascent"].GetUint();
			font->descent = entry["descent"].GetUint();
			font->leading = entry["leading"].GetUint();

			// RB: ignore glyphs because they are not used
		}
		else if( type == "EDITTEXT" )
		{
			dictionary[i].type = SWF_DICT_EDITTEXT;
			dictionary[i].edittext = new( TAG_SWF ) idSWFEditText;
			idSWFEditText* edittext = dictionary[i].edittext;

			edittext->bounds.tl.x = entry["bounds"][0].GetDouble();
			edittext->bounds.tl.y = entry["bounds"][1].GetDouble();
			edittext->bounds.br.x = entry["bounds"][2].GetDouble();
			edittext->bounds.br.y = entry["bounds"][3].GetDouble();


			edittext->flags = entry["flags"].GetUint();
			edittext->fontID = entry["fontID"].GetUint();
			edittext->fontHeight = entry["fontHeight"].GetUint();

			Value& color = entry["color"];
			edittext->color.r = ( uint8 )( color[0].GetDouble() * 255 ) & 0xFF;
			edittext->color.g = ( uint8 )( color[1].GetDouble() * 255 ) & 0xFF;
			edittext->color.b = ( uint8 )( color[2].GetDouble() * 255 ) & 0xFF;
			edittext->color.a = ( uint8 )( color[3].GetDouble() * 255 ) & 0xFF;

			edittext->maxLength = entry["maxLength"].GetUint();

			idStr align = entry["align"].GetString();
			if( align == "RIGHT" )
			{
				edittext->align = SWF_ET_ALIGN_RIGHT;
			}
			else if( align == "CENTER" )
			{
				edittext->align = SWF_ET_ALIGN_CENTER;
			}
			else if( align == "JUSTITY" )
			{
				edittext->align = SWF_ET_ALIGN_JUSTIFY;
			}
			else
			{
				edittext->align = SWF_ET_ALIGN_LEFT;
			}

			edittext->leftMargin = entry["leftMargin"].GetUint();
			edittext->rightMargin = entry["rightMargin"].GetUint();
			edittext->indent = entry["indent"].GetUint();
			edittext->leading = entry["leading"].GetUint();

			edittext->variable = entry["variable"].GetString();
			edittext->initialText = entry["initialText"].GetString();
		}
		else if( type == "TEXT" )
		{
			// RB: FIXME? not used on BFG files
		}
	}

	// now that all images have been loaded, write out the combined image
	idStr atlasFileName = "generated/";
	atlasFileName += filename;
	atlasFileName.SetFileExtension( ".png" );

	WriteSwfImageAtlas( atlasFileName );

	return true;
}

/*
===================
idSWF::WriteJSON
===================
*/
void idSWF::WriteJSON( const char* jsonFilename )
{
	const bool exportBitmapShapesOnly = false;

	idFileLocal file( fileSystem->OpenFileWrite( jsonFilename, "fs_basepath" ) );
	if( file == NULL )
	{
		return;
	}

	file->WriteFloatString( "{\n \t\"version\": %i,\n \t\"frameWidth\": %f,\n \t\"frameHeight\": %f,\n \t\"frameRate\": %i,\n", XSWF_VERSION, ( float )frameWidth, ( float )frameHeight, ( int )frameRate );

	file->WriteFloatString( "\t\"dict\":\n\t[\n" );
	for( int i = 0; i < dictionary.Num(); i++ )
	{
		const idSWFDictionaryEntry& entry = dictionary[i];

		//if( dictionary[i].type != SWF_DICT_NULL )
		{
			file->WriteFloatString( "\t\t{\n" );

			file->WriteFloatString( "\t\t\t\"type\": \"%s\",\n", idSWF::GetDictTypeName( dictionary[i].type ) );
			file->WriteFloatString( "\t\t\t\"characterID\": %i%s\n", i, dictionary[i].type != SWF_DICT_NULL ? "," : "" );
		}

		switch( dictionary[i].type )
		{
			case SWF_DICT_IMAGE:
			{
				if( dictionary[i].material )
				{
					file->WriteFloatString( "\t\t\t\"imageFile\": \"%s\",\n", dictionary[i].material->GetName() );
				}
				else
				{
					idStr filenameWithoutExt = filename;
					filenameWithoutExt.StripFileExtension();

					file->WriteFloatString( "\t\t\t\"imageFile\": \"%s/image_characterid_%i.png\",\n", filenameWithoutExt.c_str(), i );
				}

				file->WriteFloatString( "\t\t\t\"width\": %i, \"height\": %i, \"atlasOffsetX\": %i, \"atlasOffsetY\": %i,\n",
										entry.imageSize[0], entry.imageSize[1], entry.imageAtlasOffset[0], entry.imageAtlasOffset[1] );

				//file->WriteFloatString( "\t\t\t\"channelScale\": { \"x\": %f, \"y\": %f, \"z\": %f, \"w\": %f }\n", entry.channelScale.x, entry.channelScale.y, entry.channelScale.z, entry.channelScale.w );
				file->WriteFloatString( "\t\t\t\"channelScale\": { \"x\": 1, \"y\": 1, \"z\": 1, \"w\": 1 }\n" );
				break;
			}

			case SWF_DICT_MORPH:
			case SWF_DICT_SHAPE:
			{
				idSWFShape* shape = dictionary[i].shape;

				idVec2 tl = shape->startBounds.tl;
				idVec2 br = shape->startBounds.br;

				//file->WriteFloatString( "\t\t\t\"imageSize\": { \"width\": %f, \"height\": %f },\n", dictionary[i].imageSize[0], dictionary[i].imageSize[1] );
				//file->WriteFloatString( "\t\t\t\"startBounds\": { \"x\": %f, \"y\": %f, \"width\": %f, \"height\": %f },\n", x, y, width, height );

				file->WriteFloatString( "\t\t\t\"startBounds\": [ %f, %f, %f, %f ],\n", tl.x, tl.y, br.x, br.y );

				tl = shape->endBounds.tl;
				br = shape->endBounds.br;

				file->WriteFloatString( "\t\t\t\"endBounds\": [ %f, %f, %f, %f ]", tl.x, tl.y, br.x, br.y );

				// export fill draws
				if( shape->fillDraws.Num() > 0 )
				{
					file->WriteFloatString( ",\n\t\t\t\"fillDraws\":\n\t\t\t[\n" );

					if( shape->fillDraws.Num() > 1 )
					{
						idLib::Printf( S_COLOR_YELLOW "WARNING: " S_COLOR_RED "%s.Shape%i has %i fill draws\n", filename.c_str(), i, shape->fillDraws.Num() );
					}

					for( int d = 0; d < shape->fillDraws.Num(); d++ )
					{
						idSWFShapeDrawFill& fillDraw = shape->fillDraws[d];

						if( exportBitmapShapesOnly && fillDraw.style.type != 4 )
						{
							continue;
						}

						file->WriteFloatString( "\t\t\t\t{\n" );

						file->WriteFloatString( "\t\t\t\t\t\"style\":\n\t\t\t\t\t{\n\t\t\t\t\t\t\"type\": " );

						// 0 = solid, 1 = gradient, 4 = bitmap
						if( fillDraw.style.type == 0 )
						{
							file->WriteFloatString( "\"solid\"" );
						}
						else if( fillDraw.style.type == 1 )
						{
							file->WriteFloatString( "\"gradient\"" );
						}
						else if( fillDraw.style.type == 4 )
						{
							file->WriteFloatString( "\"bitmap\"" );
						}
						else
						{
							file->WriteFloatString( "\"%i\"", fillDraw.style.type );
						}

						// 0 = linear, 2 = radial, 3 = focal; 0 = repeat, 1 = clamp, 2 = near repeat, 3 = near clamp
						file->WriteFloatString( ",\n\t\t\t\t\t\t\"subType\": " );
						if( fillDraw.style.subType == 0 )
						{
							file->WriteFloatString( "\"linear\"" );
						}
						else if( fillDraw.style.subType == 1 )
						{
							file->WriteFloatString( "\"radial\"" );
						}
						else if( fillDraw.style.subType == 2 )
						{
							file->WriteFloatString( "\"focal\"" );
						}
						else if( fillDraw.style.subType == 3 )
						{
							file->WriteFloatString( "\"near clamp\"" );
						}
						else
						{
							file->WriteFloatString( "\"%i\"", fillDraw.style.subType );
						}

						if( fillDraw.style.type == 1 && fillDraw.style.subType == 3 )
						{
							file->WriteFloatString( ",\n\t\t\t\t\t\t\"focalPoint\": %f", fillDraw.style.focalPoint );
						}

						if( fillDraw.style.type == 4 )
						{
							file->WriteFloatString( ",\n\t\t\t\t\t\t\"bitmapID\": %i", fillDraw.style.bitmapID );
						}

						if( fillDraw.style.type == 0 )
						{
							idVec4 color = fillDraw.style.startColor.ToVec4();
							file->WriteFloatString( ",\n\t\t\t\t\t\t\"startColor\": [ %f, %f, %f, %f ]", color.x, color.y, color.z, color.w );

							color = fillDraw.style.endColor.ToVec4();
							file->WriteFloatString( ",\n\t\t\t\t\t\t\"endColor\": [ %f, %f, %f, %f ]\n", color.x, color.y, color.z, color.w );
						}

						if( fillDraw.style.type > 0 )
						{
							swfMatrix_t m = fillDraw.style.startMatrix;
							file->WriteFloatString( ",\n\t\t\t\t\t\t\"startMatrix\": [ %f, %f, %f, %f, %f, %f ]", m.xx, m.yy, m.xy, m.yx, m.tx, m.ty );

							if( fillDraw.style.startMatrix != fillDraw.style.endMatrix )
							{
								m = fillDraw.style.endMatrix;
								file->WriteFloatString( ",\n\t\t\t\t\t\t\"endMatrix\": [ %f, %f, %f, %f, %f, %f ],\n", m.xx, m.yy, m.xy, m.yx, m.tx, m.ty );
							}
						}

						// not used in BFG menus
						if( fillDraw.style.gradient.numGradients )
						{
							file->WriteFloatString( "\t\t\t\t\t\"gradients\":\n\t\t\t\t\t[\n" );

							for( int g = 0; g < fillDraw.style.gradient.numGradients; g++ )
							{
								swfGradientRecord_t gr = fillDraw.style.gradient.gradientRecords[g];

								file->WriteFloatString( "\t\t\t\t\t{ \"startRatio\": %i, \"endRatio\": %i,\n", gr.startRatio, gr.endRatio );

								idVec4 color = gr.startColor.ToVec4();
								file->WriteFloatString( ",\n\t\t\t\t\t\t\"startColor\": [ %f, %f, %f, %f ]", color.x, color.y, color.z, color.w );

								idVec4 endColor = gr.endColor.ToVec4();
								if( color != endColor )
								{
									file->WriteFloatString( ",\n\t\t\t\t\t\t\"endColor\": [ %f, %f, %f, %f ]", endColor.x, endColor.y, endColor.z, endColor.w );
								}

								file->WriteFloatString( "\t\t\t\t\t\t}%s\n", ( g == ( fillDraw.style.gradient.numGradients - 1 ) ) ? "" : "," );
							}

							file->WriteFloatString( "\t\t\t\t\t\t],\n" );
						}

						file->WriteFloatString( "\n\t\t\t\t\t}" );

						if( fillDraw.startVerts.Num() )
						{
							file->WriteFloatString( ",\n\t\t\t\t\t\"startVerts\":\n\t\t\t\t\t[\n" );
							for( int v = 0; v < fillDraw.startVerts.Num(); v++ )
							{
								const idVec2& vert = fillDraw.startVerts[v];

								file->WriteFloatString( "\t\t\t\t\t\t{ \"v\": [ %f, %f ] }%s\n", vert.x, vert.y, ( v == ( fillDraw.startVerts.Num() - 1 ) ) ? "" : "," );
							}
							file->WriteFloatString( "\t\t\t\t\t]" );
						}
						else
						{
							idLib::Printf( "fillDraw %i of characterID %i has no startVerts\n", d, i );
						}

						if( fillDraw.endVerts.Num() )
						{
							file->WriteFloatString( ",\n\t\t\t\t\t\"endVerts\":\n\t\t\t\t\t[\n" );
							for( int v = 0; v < fillDraw.endVerts.Num(); v++ )
							{
								const idVec2& vert = fillDraw.endVerts[v];

								file->WriteFloatString( "\t\t\t\t\t\t{ \"v\": [ %f, %f ] }%s\n", vert.x, vert.y, ( v == ( fillDraw.endVerts.Num() - 1 ) ) ? "" : "," );
							}
							file->WriteFloatString( "\t\t\t\t\t]" );
						}

						if( fillDraw.indices.Num() )
						{
							file->WriteFloatString( ",\n\t\t\t\t\t\"indices\": [ " );
#if 1
							for( int v = 0; v < fillDraw.indices.Num(); v++ )
							{
								const uint16& vert = fillDraw.indices[v];

								file->WriteFloatString( "%i%s", vert, ( v == fillDraw.indices.Num() - 1 ) ? "" : ", " );
							}
#else
							for( int v = fillDraw.indices.Num() - 1; v >= 0; v-- )
							{
								const uint16& vert = fillDraw.indices[v];

								file->WriteFloatString( "%i%s", vert, ( v == 0 ) ? "" : ", " );
							}
#endif
							file->WriteFloatString( "]\n" );
						}

						if( !fillDraw.startVerts.Num() && !fillDraw.endVerts.Num() && !fillDraw.indices.Num() )
						{
							file->WriteFloatString( "\n" );
						}
						//else
						{
							// skip \t\t\t\t}
							//file->WriteFloatString( "%s\n", ( d == ( shape->fillDraws.Num() - 1 ) ) ? "" : "," );
						}

						file->WriteFloatString( "\t\t\t\t}%s\n", ( d == ( shape->fillDraws.Num() - 1 ) ) ? "" : "," );
					}

					file->WriteFloatString( "\t\t\t]" );
				}

				// export line draws
				if( shape->lineDraws.Num() > 0 )
				{
					file->WriteFloatString( ",\n\t\t\t\"lineDraws\":\n\t\t\t[\n" );

					for( int d = 0; d < shape->lineDraws.Num(); d++ )
					{
						const idSWFShapeDrawLine& lineDraw = shape->lineDraws[d];

						file->WriteFloatString( "\t\t\t\t{\n" );

						file->WriteFloatString( "\t\t\t\t\t\"style\":\n\t\t\t\t\t{\n\t\t\t\t\t\t\"startWidth\": %i", lineDraw.style.startWidth );

						file->WriteFloatString( ",\n\t\t\t\t\t\t\"endWidth\": %i", lineDraw.style.endWidth );

						idVec4 color = lineDraw.style.startColor.ToVec4();
						file->WriteFloatString( ",\n\t\t\t\t\t\t\"startColor\": [ %f, %f, %f, %f ]", color.x, color.y, color.z, color.w );

						idVec4 endColor = lineDraw.style.endColor.ToVec4();
						if( color != endColor )
						{
							file->WriteFloatString( ",\n\t\t\t\t\t\t\"endColor\": [ %f, %f, %f, %f ]\n", endColor.x, endColor.y, endColor.z, endColor.w );
						}

						file->WriteFloatString( "\n\t\t\t\t\t}" );

						if( lineDraw.startVerts.Num() )
						{
							file->WriteFloatString( ",\n\t\t\t\t\t\"startVerts\":\n\t\t\t\t\t[\n" );
							for( int v = 0; v < lineDraw.startVerts.Num(); v++ )
							{
								const idVec2& vert = lineDraw.startVerts[v];

								file->WriteFloatString( "\t\t\t\t\t\t{ \"v\": [ %f, %f ] }%s\n", vert.x, vert.y, ( v == ( lineDraw.startVerts.Num() - 1 ) ) ? "" : "," );
							}
							file->WriteFloatString( "\t\t\t\t\t]" );
						}

						if( lineDraw.endVerts.Num() )
						{
							file->WriteFloatString( ",\n\t\t\t\t\t\"endVerts\":\n\t\t\t\t\t[\n" );
							for( int v = 0; v < lineDraw.endVerts.Num(); v++ )
							{
								const idVec2& vert = lineDraw.endVerts[v];

								file->WriteFloatString( "\t\t\t\t\t\t{ \"v\": [ %f, %f ] }%s\n", vert.x, vert.y, ( v == ( lineDraw.endVerts.Num() - 1 ) ) ? "" : "," );
							}
							file->WriteFloatString( "\t\t\t\t\t]" );
						}

						if( lineDraw.indices.Num() )
						{
							file->WriteFloatString( ",\n\t\t\t\t\t\"indices\": [ " );
#if 1
							for( int v = 0; v < lineDraw.indices.Num(); v++ )
							{
								const uint16& vert = lineDraw.indices[v];

								file->WriteFloatString( "%i%s", vert, ( v == lineDraw.indices.Num() - 1 ) ? "" : ", " );
							}
#else
							for( int v = fillDraw.indices.Num() - 1; v >= 0; v-- )
							{
								const uint16& vert = fillDraw.indices[v];

								file->WriteFloatString( "%i%s", vert, ( v == 0 ) ? "" : ", " );
							}
#endif
							file->WriteFloatString( "]\n" );
						}

						file->WriteFloatString( "\t\t\t\t}%s\n", ( d == ( shape->lineDraws.Num() - 1 ) ) ? "" : "," );
					}

					file->WriteFloatString( "\t\t\t]" );
				}

				file->WriteFloatString( "\n" );
				break;
			}

			case SWF_DICT_SPRITE:
			{
				dictionary[i].sprite->WriteJSON( file, i );
				break;
			}

			case SWF_DICT_FONT:
			{
				const idSWFFont* font = dictionary[i].font;

				file->WriteFloatString( "\t\t\t\"name\": \"%s\", \"ascent\": %i, \"descent\": %i, \"leading\": %i\n", //, \"glyphsNum\": %i\n",
										font->fontID->GetName(), font->ascent, font->descent, font->leading ); //, font->glyphs.Num() );

#if 0
				for( int g = 0; g < font->glyphs.Num(); g++ )
				{
					file->WriteFloatString( "\t\t\t<Glyph code=\"%i\" advance=\"%i\"/>\n", font->glyphs[g].code, font->glyphs[g].advance );

#if 0
					for( int v = 0; v < font->glyphs[g].verts.Num(); v++ )
					{
						const idVec2& vert = font->glyphs[g].verts[v];

						file->WriteFloatString( "\t\t\t\t<Vertex x=\"%f\" y=\"%f\"/>\n", vert.x, vert.y );
					}

					file->WriteFloatString( "\t\t\t\t<Indices num=\"%i\">", font->glyphs[g].indices.Num() );
					for( int v = 0; v < font->glyphs[g].indices.Num(); v++ )
					{
						const uint16& vert = font->glyphs[g].indices[v];

						file->WriteFloatString( "%i ", vert );
					}
					file->WriteFloatString( "</Indices>\n" );

					file->WriteFloatString( "\t\t\t</Glyph>\n" );
#endif
				}
#endif
				break;
			}

			case SWF_DICT_TEXT:
			{
				// RB: not used in BFG files

				const idSWFText* text = dictionary[i].text;

				file->WriteFloatString( "\t\t<Text characterID=\"%i\">\n", i );

				float x = text->bounds.tl.y;
				float y = text->bounds.tl.x;
				float width = fabs( text->bounds.br.y - text->bounds.tl.y );
				float height = fabs( text->bounds.br.x - text->bounds.tl.x );

				file->WriteFloatString( "\t\t\t<Bounds x=\"%f\" y=\"%f\" width=\"%f\" height=\"%f\" />\n", x, y, width, height );

				//file->WriteBig( text->bounds.tl );
				//file->WriteBig( text->bounds.br );

				//file->WriteBigArray( ( float* )&text->matrix, 6 );

				swfMatrix_t m = text->matrix;
				file->WriteFloatString( "\t\t\t<Matrix>%f %f %f %f %f %f</Matrix>\n",
										m.xx, m.yy, m.xy, m.yx, m.tx, m.ty );

				//file->WriteBig( text->textRecords.Num() );
				for( int t = 0; t < text->textRecords.Num(); t++ )
				{
					const idSWFTextRecord& textRecord = text->textRecords[t];

					file->WriteFloatString( "\t\t\t\t<Record fontID=\"%i\" xOffet=\"%i\" yOffset=\"%i\" textHeight=\"%f\" firstGlyph=\"%i\" numGlyphs=\"%i\">\n",
											textRecord.fontID, textRecord.xOffset, textRecord.yOffset, textRecord.textHeight, textRecord.firstGlyph, textRecord.numGlyphs );

					idVec4 color = textRecord.color.ToVec4();
					file->WriteFloatString( "\t\t\t\t\t<Color r=\"%f\" g=\"%f\" b=\"%f\" a=\"%f\"/>\n",
											color.x, color.y, color.z, color.w );

					file->WriteFloatString( "\t\t\t\t</Record>\n" );

					/*file->WriteBig( textRecord.fontID );
					file->Write( &textRecord.color, 4 );
					file->WriteBig( textRecord.xOffset );
					file->WriteBig( textRecord.yOffset );
					file->WriteBig( textRecord.textHeight );
					file->WriteBig( textRecord.firstGlyph );
					file->WriteBig( textRecord.numGlyphs );*/
				}

				for( int g = 0; g < text->glyphs.Num(); g++ )
				{
					file->WriteFloatString( "\t\t\t\t<Glyph index=\"%i\" advance=\"%i\">\n", text->glyphs[g].index, text->glyphs[g].advance );
				}

				/*
				file->WriteBig( text->glyphs.Num() );
				for( int g = 0; g < text->glyphs.Num(); g++ )
				{
					file->WriteBig( text->glyphs[g].index );
					file->WriteBig( text->glyphs[g].advance );
				}
				*/

				file->WriteFloatString( "\t\t</Text>\n" );
				break;
			}

			case SWF_DICT_EDITTEXT:
			{
				const idSWFEditText* et = dictionary[i].edittext;

				idStr initialText = idStr::CStyleQuote( et->initialText.c_str() );

				file->WriteFloatString( "\t\t\t\"flags\": %i, \"fontID\": %i, \"fontHeight\": %i, \"maxLength\": %i, \"align\": \"%s\", \"leftMargin\": %i, \"rightMargin\": %i, \"indent\": %i, \"leading\": %i, \"variable\": \"%s\", \"initialText\": %s,\n",
										et->flags, et->fontID, et->fontHeight, et->maxLength, idSWF::GetEditTextAlignName( et->align ),
										et->leftMargin, et->rightMargin, et->indent, et->leading,
										et->variable.c_str(), initialText.c_str() );

				idVec2 tl = et->bounds.tl;
				idVec2 br = et->bounds.br;

				file->WriteFloatString( "\t\t\t\"bounds\": [ %f, %f, %f, %f ],\n", tl.x, tl.y, br.x, br.y );

				idVec4 color = et->color.ToVec4();
				file->WriteFloatString( "\t\t\t\"color\": [ %f, %f, %f, %f ]\n", color.x, color.y, color.z, color.w );
				break;
			}
		}

		//if( dictionary[i].type != SWF_DICT_NULL )
		{
			//file->WriteFloatString( "\t\t}%s\n", ( i == ( dictionary.Num() - 1 ) ) ? "" : "," );
			file->WriteFloatString( "\t\t},\n" );
		}
	}

	file->WriteFloatString( "\t\t{\n" );
	file->WriteFloatString( "\t\t\t\"type\": \"SPRITE\",\n" );
	file->WriteFloatString( "\t\t\t\"characterID\": %i,\n", dictionary.Num() );
	file->WriteFloatString( "\t\t\t\"mainsprite\": true,\n" );
	mainsprite->WriteJSON( file, dictionary.Num() );
	file->WriteFloatString( "\t\t}\n" );
	file->WriteFloatString( "\t]\n" );
	file->WriteFloatString( "}\n" );
}
// RB end
