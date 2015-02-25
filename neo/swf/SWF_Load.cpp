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
#include "../renderer/Font.h"

#pragma warning(disable: 4355) // 'this' : used in base member initializer list

#define BSWF_VERSION 16		// bumped to 16 for storing atlas image dimensions for unbuffered loads
#define BSWF_MAGIC ( ( 'B' << 24 ) | ( 'S' << 16 ) | ( 'W' << 8 ) | BSWF_VERSION )

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
	atlasFileName.SetFileExtension( ".tga" );
	
	WriteSwfImageAtlas( atlasFileName );
	
	Mem_Free( fileData );
	
	return true;
}

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
