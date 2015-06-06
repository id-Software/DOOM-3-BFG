/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013 Robert Beckebans

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
========================
idSWFSprite::idSWFSprite
========================
*/
idSWFSprite::idSWFSprite( idSWF* _swf ) :
	swf( _swf ),
	frameCount( 0 ),
	commandBuffer( NULL )
{
}

/*
========================
idSWFSprite::~idSWFSprite
========================
*/
idSWFSprite::~idSWFSprite()
{
	Mem_Free( commandBuffer );
}

/*
========================
idSWF::DefineSprite
========================
*/
void idSWF::DefineSprite( idSWFBitStream& bitstream )
{
	uint16 characterID = bitstream.ReadU16();
	idSWFDictionaryEntry* entry = AddDictionaryEntry( characterID, SWF_DICT_SPRITE );
	if( entry == NULL )
	{
		return;
	}
	entry->sprite->Load( bitstream, false );
}

/*
========================
idSWFSprite::Load
========================
*/
void idSWFSprite::Load( idSWFBitStream& bitstream, bool parseDictionary )
{

	frameCount = bitstream.ReadU16();
	
	// run through the file once, building the dictionary and accumulating control tags
	frameOffsets.SetNum( frameCount + 1 );
	frameOffsets[0] = 0;
	
	unsigned int currentFrame = 1;
	
	while( true )
	{
		uint16 codeAndLength = bitstream.ReadU16();
		uint32 recordLength = ( codeAndLength & 0x3F );
		if( recordLength == 0x3F )
		{
			recordLength = bitstream.ReadU32();
		}
		
		idSWFBitStream tagStream( bitstream.ReadData( recordLength ), recordLength, false );
		
		swfTag_t tag = ( swfTag_t )( codeAndLength >> 6 );
		
		// ----------------------
		// Definition tags
		// definition tags are only allowed in the main sprite
		// ----------------------
		if( parseDictionary )
		{
			bool handled = true;
			switch( tag )
			{
#define HANDLE_SWF_TAG( x ) case Tag_##x: swf->x( tagStream ); break;
					HANDLE_SWF_TAG( FileAttributes );
					HANDLE_SWF_TAG( Metadata );
					HANDLE_SWF_TAG( SetBackgroundColor );
					HANDLE_SWF_TAG( JPEGTables );
					HANDLE_SWF_TAG( DefineBits );
					HANDLE_SWF_TAG( DefineBitsJPEG2 );
					HANDLE_SWF_TAG( DefineBitsJPEG3 );
					HANDLE_SWF_TAG( DefineBitsLossless );
					HANDLE_SWF_TAG( DefineBitsLossless2 );
					HANDLE_SWF_TAG( DefineShape );
					HANDLE_SWF_TAG( DefineShape2 );
					HANDLE_SWF_TAG( DefineShape3 );
					HANDLE_SWF_TAG( DefineShape4 );
					HANDLE_SWF_TAG( DefineSprite );
					HANDLE_SWF_TAG( DefineSound );
					//HANDLE_SWF_TAG( DefineMorphShape ); // these don't work right
					HANDLE_SWF_TAG( DefineFont2 );
					HANDLE_SWF_TAG( DefineFont3 );
					HANDLE_SWF_TAG( DefineText );
					HANDLE_SWF_TAG( DefineText2 );
					HANDLE_SWF_TAG( DefineEditText );
#undef HANDLE_SWF_TAG
				default:
					handled = false;
			}
			if( handled )
			{
				continue;
			}
		}
		// ----------------------
		// Control tags
		// control tags are stored off in the commands list and processed at run time
		// except for a couple really special control tags like "End" and "FrameLabel"
		// ----------------------
		switch( tag )
		{
			case Tag_End:
				return;
				
			case Tag_ShowFrame:
				frameOffsets[ currentFrame ] = commands.Num();
				currentFrame++;
				break;
				
			case Tag_FrameLabel:
			{
				swfFrameLabel_t& label = frameLabels.Alloc();
				label.frameNum = currentFrame;
				label.frameLabel = tagStream.ReadString();
			}
			break;
			
			case Tag_DoInitAction:
			{
				tagStream.ReadU16();
				
				idSWFBitStream& initaction = doInitActions.Alloc();
				initaction.Load( tagStream.ReadData( recordLength - 2 ), recordLength - 2, true );
			}
			break;
			
			case Tag_DoAction:
			case Tag_PlaceObject2:
			case Tag_PlaceObject3:
			case Tag_RemoveObject2:
			{
				swfSpriteCommand_t& command = commands.Alloc();
				command.tag = tag;
				command.stream.Load( tagStream.ReadData( recordLength ), recordLength, true );
			}
			break;
			
			default:
				// We don't care, about sprite tags we don't support ... RobA
				//idLib::Printf( "Load Sprite: Unhandled tag %s\n", idSWF::GetTagName( tag ) );
				break;
		}
	}
}

/*
========================
idSWFSprite::Read
========================
*/
void idSWFSprite::Read( idFile* f )
{
	int num = 0;
	f->ReadBig( frameCount );
	f->ReadBig( num );
	frameOffsets.SetNum( num );
	f->ReadBigArray( frameOffsets.Ptr(), frameOffsets.Num() );
	f->ReadBig( num );
	frameLabels.SetNum( num );
	for( int i = 0; i < frameLabels.Num(); i++ )
	{
		f->ReadBig( frameLabels[i].frameNum );
		f->ReadString( frameLabels[i].frameLabel );
	}
	
	uint32 bufferSize;
	f->ReadBig( bufferSize );
	
	commandBuffer = ( byte* )Mem_Alloc( bufferSize, TAG_SWF );
	f->Read( commandBuffer, bufferSize );
	
	byte* currentBuffer = commandBuffer;
	
	f->ReadBig( num );
	commands.SetNum( num );
	for( int i = 0; i < commands.Num(); i++ )
	{
		uint32 streamLength = 0;
		
		f->ReadBig( commands[i].tag );
		f->ReadBig( streamLength );
		
		commands[i].stream.Load( currentBuffer, streamLength, false );
		currentBuffer += streamLength;
	}
	
	uint32 doInitActionLength = 0;
	f->ReadBig( num );
	doInitActions.SetNum( num );
	for( int i = 0; i < num; i++ )
	{
		f->ReadBig( doInitActionLength );
		idSWFBitStream& initaction = doInitActions[i];
		initaction.Load( currentBuffer, doInitActionLength, true );
		currentBuffer += doInitActionLength;
	}
}

/*
========================
idSWFSprite::Write
========================
*/
void idSWFSprite::Write( idFile* f )
{
	f->WriteBig( frameCount );
	f->WriteBig( frameOffsets.Num() );
	f->WriteBigArray( frameOffsets.Ptr(), frameOffsets.Num() );
	f->WriteBig( frameLabels.Num() );
	for( int i = 0; i < frameLabels.Num(); i++ )
	{
		f->WriteBig( frameLabels[i].frameNum );
		f->WriteString( frameLabels[i].frameLabel );
	}
	uint32 totalLength = 0;
	for( int i = 0; i < commands.Num(); i++ )
	{
		totalLength += commands[i].stream.Length();
	}
	for( int i = 0; i < doInitActions.Num(); i++ )
	{
		totalLength += doInitActions[i].Length();
	}
	f->WriteBig( totalLength );
	for( int i = 0; i < commands.Num(); i++ )
	{
		f->Write( commands[i].stream.Ptr(), commands[i].stream.Length() );
	}
	for( int i = 0; i < doInitActions.Num(); i++ )
	{
		f->Write( doInitActions[i].Ptr(), doInitActions[i].Length() );
	}
	
	f->WriteBig( commands.Num() );
	for( int i = 0; i < commands.Num(); i++ )
	{
		f->WriteBig( commands[i].tag );
		f->WriteBig( commands[i].stream.Length() );
	}
	
	f->WriteBig( doInitActions.Num() );
	for( int i = 0; i < doInitActions.Num(); i++ )
	{
		f->WriteBig( doInitActions[i].Length() );
	}
}


// RB begin
/*
========================
idSWFSprite::WriteXML
========================
*/
void idSWFSprite::WriteXML( idFile* f, int characterID, const char* indentPrefix )
{
	if( characterID >= 0 )
	{
		f->WriteFloatString( "%s<Sprite characterID=\"%i\" frameCount=\"%i\" frameOffsets=\"%i\">\n", indentPrefix, characterID, frameCount, frameOffsets.Num() );
	}
	else
	{
		f->WriteFloatString( "%s<Sprite frameCount=\"%i\" frameOffsets=\"%i\">\n", indentPrefix, frameCount, frameOffsets.Num() );
	}
	
	f->WriteFloatString( "%s\t<frameOffsets>", indentPrefix );
	for( int i = 0; i < frameOffsets.Num(); i++ )
	{
		f->WriteFloatString( "%i ", frameOffsets[i] );
	}
	f->WriteFloatString( "</frameOffsets>\n" );
	
	//f->WriteFloatString( "\t\t<frameLabels num=\"%i\">", frameLabels.Num() );
	for( int i = 0; i < frameLabels.Num(); i++ )
	{
		f->WriteFloatString( "%s\t<FrameLabel frameNum=\"%i\" frameLabel=\"%s\"/>\n", indentPrefix, frameLabels[i].frameNum, frameLabels[i].frameLabel.c_str() );
	}
	
	
#if 1
	idBase64 base64;
	
	for( int i = 0; i < commands.Num(); i++ )
	{
		idSWFSprite::swfSpriteCommand_t& command = commands[i];
		
		//base64.Encode( command.stream.Ptr(), command.stream.Length() );
		//base64.Decode( src );
		
		//f->WriteFloatString( "%s\t<Command tag=\"%s\" streamLength=\"%i\">%s</Command>\n", indentPrefix, idSWF::GetTagName( commands[i].tag ), src.Length(), src.c_str() );
		//f->WriteFloatString( "%s\t<Command tag=\"%s\" streamLength=\"%i\">%s</Command>\n", indentPrefix, idSWF::GetTagName( commands[i].tag ), commands[i].stream.Length(), base64.c_str() );
		
		//f->WriteFloatString( "%s\t<Command tag=\"%s\" streamLength=\"%i\">\n", indentPrefix, idSWF::GetTagName( command.tag ), command.stream.Length(), base64.c_str() );
		//f->WriteFloatString( "%s\t\t<Stream>%s</Stream>\n", indentPrefix, base64.c_str() );
		
		command.stream.Rewind();
		switch( command.tag )
		{
				//case Tag_PlaceObject2:
				//	WriteXML_PlaceObject2( command.stream, indentPrefix );
				//	break;
				
#define HANDLE_SWF_TAG( x ) case Tag_##x: WriteXML_##x( f, command.stream, characterID, i, indentPrefix ); break;
				HANDLE_SWF_TAG( PlaceObject2 );
				HANDLE_SWF_TAG( PlaceObject3 );
				HANDLE_SWF_TAG( RemoveObject2 );
				//HANDLE_SWF_TAG( StartSound );
				HANDLE_SWF_TAG( DoAction );
#undef HANDLE_SWF_TAG
			default:
				break;
				//idLib::Printf( "Export Sprite: Unhandled tag %s\n", idSWF::GetTagName( command.tag ) );
		}
		
		//f->WriteFloatString( "%s\t</Command>\n", indentPrefix );
	}
	
	for( int i = 0; i < doInitActions.Num(); i++ )
	{
		base64.Encode( doInitActions[i].Ptr(), doInitActions[i].Length() );
		
		f->WriteFloatString( "%s\t<DoInitAction streamLength=\"%i\">%s</DoInitAction>\n", indentPrefix, doInitActions[i].Length(), base64.c_str() );
		
		//f->WriteFloatString( "%s\t<DoInitAction streamLength=\"%i\">%s</DoInitAction>\n", indentPrefix, doInitActions[i].Length(), doInitActions[i].Ptr() );
	}
#endif
	
	f->WriteFloatString( "%s</Sprite>\n", indentPrefix );
}
<<<<<<< HEAD
// RB end
=======

void idSWFSprite::WriteXML_PlaceObject2( idFile* file, idSWFBitStream& bitstream, int sourceCharacterID, int commandID, const char* indentPrefix )
{
	uint64 flags = bitstream.ReadU8();
	int depth = bitstream.ReadU16();
	
	file->WriteFloatString( "%s\t<PlaceObject2 flags=\"%i\" depth=\"%i\"", indentPrefix, flags, depth );
	
	int characterID = -1;
	if( ( flags & PlaceFlagHasCharacter ) != 0 )
	{
		characterID = bitstream.ReadU16();
		file->WriteFloatString( " characterID=\"%i\"", characterID );
	}
	
	file->WriteFloatString( ">\n" );
	
	if( ( flags & PlaceFlagHasMatrix ) != 0 )
	{
		swfMatrix_t m;
		
		bitstream.ReadMatrix( m );
		
		file->WriteFloatString( "%s\t\t<StartMatrix>%f %f %f %f %f %f</StartMatrix>\n", indentPrefix, m.xx, m.yy, m.xy, m.yx, m.tx, m.ty );
	}
	
	if( ( flags & PlaceFlagHasColorTransform ) != 0 )
	{
		swfColorXform_t cxf;
		bitstream.ReadColorXFormRGBA( cxf );
		
		idVec4 color = cxf.mul;
		file->WriteFloatString( "%s\t\t<MulColor r=\"%f\" g=\"%f\" b=\"%f\" a=\"%f\"/>\n", indentPrefix, color.x, color.y, color.z, color.w );
		
		color = cxf.add;
		if( color != vec4_origin )
		{
			file->WriteFloatString( "%s\t\t<AddColor r=\"%f\" g=\"%f\" b=\"%f\" a=\"%f\"/>\n", indentPrefix, color.x, color.y, color.z, color.w );
		}
	}
	
	if( ( flags & PlaceFlagHasRatio ) != 0 )
	{
		float ratio = bitstream.ReadU16() * ( 1.0f / 65535.0f );
		
		file->WriteFloatString( "%s\t\t<Ratio>%f</Ratio>\n", indentPrefix, ratio );
	}
	
	if( ( flags & PlaceFlagHasName ) != 0 )
	{
		idStr name = bitstream.ReadString();
		
		file->WriteFloatString( "%s\t\t<Name>%s</Name>\n", indentPrefix, name.c_str() );
		
		/*if( display->spriteInstance )
		{
			display->spriteInstance->name = name;
			scriptObject->Set( name, display->spriteInstance->GetScriptObject() );
		}
		else if( display->textInstance )
		{
			scriptObject->Set( name, display->textInstance->GetScriptObject() );
		}*/
	}
	
	if( ( flags & PlaceFlagHasClipDepth ) != 0 )
	{
		uint16 clipDepth = bitstream.ReadU16();
		file->WriteFloatString( "%s\t\t<ClipDepth>%i</ClipDepth>\n", indentPrefix, clipDepth );
	}
	
	if( ( flags & PlaceFlagHasClipActions ) != 0 )
	{
		// FIXME: clip actions
	}
	
	file->WriteFloatString( "%s\t</PlaceObject2>\n", indentPrefix );
}


void idSWFSprite::WriteXML_PlaceObject3( idFile* file, idSWFBitStream& bitstream, int sourceCharacterID, int commandID, const char* indentPrefix )
{
	uint64 flags1 = bitstream.ReadU8();
	uint64 flags2 = bitstream.ReadU8();
	int depth = bitstream.ReadU16();
	
	file->WriteFloatString( "%s\t<PlaceObject3 flags1=\"%i\" flags2=\"%i\" depth=\"%i\"", indentPrefix, flags1, flags2, depth );
	
	int characterID = -1;
	if( ( flags1 & PlaceFlagHasCharacter ) != 0 )
	{
		characterID = bitstream.ReadU16();
		file->WriteFloatString( " characterID=\"%i\"", characterID );
	}
	
	file->WriteFloatString( ">\n" );
	
	if( ( flags1 & PlaceFlagHasMatrix ) != 0 )
	{
		swfMatrix_t m;
		
		bitstream.ReadMatrix( m );
		
		file->WriteFloatString( "%s\t\t<StartMatrix>%f %f %f %f %f %f</StartMatrix>\n", indentPrefix, m.xx, m.yy, m.xy, m.yx, m.tx, m.ty );
	}
	
	if( ( flags1 & PlaceFlagHasColorTransform ) != 0 )
	{
		swfColorXform_t cxf;
		bitstream.ReadColorXFormRGBA( cxf );
		
		idVec4 color = cxf.mul;
		file->WriteFloatString( "%s\t\t<MulColor r=\"%f\" g=\"%f\" b=\"%f\" a=\"%f\"/>\n", indentPrefix, color.x, color.y, color.z, color.w );
		
		color = cxf.add;
		if( color != vec4_origin )
		{
			file->WriteFloatString( "%s\t\t<AddColor r=\"%f\" g=\"%f\" b=\"%f\" a=\"%f\"/>\n", indentPrefix, color.x, color.y, color.z, color.w );
		}
	}
	
	if( ( flags1 & PlaceFlagHasRatio ) != 0 )
	{
		float ratio = bitstream.ReadU16() * ( 1.0f / 65535.0f );
		
		file->WriteFloatString( "%s\t\t<Ratio>%f</Ratio>\n", indentPrefix, ratio );
	}
	
	if( ( flags1 & PlaceFlagHasName ) != 0 )
	{
		idStr name = bitstream.ReadString();
		
		file->WriteFloatString( "%s\t\t<Name>%s</Name>\n", indentPrefix, name.c_str() );
		
		/*if( display->spriteInstance )
		{
			display->spriteInstance->name = name;
			scriptObject->Set( name, display->spriteInstance->GetScriptObject() );
		}
		else if( display->textInstance )
		{
			scriptObject->Set( name, display->textInstance->GetScriptObject() );
		}*/
	}
	
	if( ( flags1 & PlaceFlagHasClipDepth ) != 0 )
	{
		uint16 clipDepth = bitstream.ReadU16();
		file->WriteFloatString( "%s\t\t<ClipDepth>%i</ClipDepth>\n", indentPrefix, clipDepth );
	}
	
	if( ( flags2 & PlaceFlagHasFilterList ) != 0 )
	{
		// we don't support filters and because the filter list is variable length we
		// can't support anything after the filter list either (blend modes and clip actions)
		//idLib::Warning( "PlaceObject3: has filters" );
		file->WriteFloatString( "%s\t\t<!-- WARNING: PlaceObject 3 has filters -->\n", indentPrefix );
		return;
	}
	
	if( ( flags2 & PlaceFlagHasBlendMode ) != 0 )
	{
		uint8 blendMode = bitstream.ReadU8();
		file->WriteFloatString( "%s\t\t<BlendMode>%i</BlendMode>\n", indentPrefix, blendMode );
	}
	
	if( ( flags1 & PlaceFlagHasClipActions ) != 0 )
	{
		// FIXME: clip actions
	}
	
	file->WriteFloatString( "%s\t</PlaceObject3>\n", indentPrefix );
}

void idSWFSprite::WriteXML_RemoveObject2( idFile* file, idSWFBitStream& bitstream, int sourceCharacterID, int commandID, const char* indentPrefix )
{
	int depth = bitstream.ReadU16();
	
	file->WriteFloatString( "%s\t<RemoveObject2 depth=\"%i\"/>\n", indentPrefix, depth );
}

void idSWFSprite::WriteXML_DoAction( idFile* file, idSWFBitStream& bitstream, int characterID, int commandID, const char* indentPrefix )
{
	idBase64 base64;
	
	base64.Encode( bitstream.Ptr(), bitstream.Length() );
	
	idSWFScriptObject* scriptObject = idSWFScriptObject::Alloc();
	scriptObject->SetPrototype( &spriteInstanceScriptObjectPrototype );
//	scriptObject->SetSprite( this );

	idSWFScriptFunction_Script* actionScript = idSWFScriptFunction_Script::Alloc();
	
	idList<idSWFScriptObject*, TAG_SWF> scope;
	//scope.Append( swf->globals );
	scope.Append( scriptObject );
	actionScript->SetScope( scope );
//	actionScript->SetDefaultSprite( this );

	actionScript->SetData( bitstream.Ptr(), bitstream.Length() );
	idStr scriptText = actionScript->CallToScript( scriptObject, idSWFParmList(), file->GetName(), characterID, commandID );
	
	//file->WriteFloatString( "%s\t<DoAction streamLength=\"%i\">%s</DoAction>\n", indentPrefix, bitstream.Length(), base64.c_str() );
	
	file->WriteFloatString( "%s\t<DoAction streamLength=\"%i\">\n%s%s\t</DoAction>\n", indentPrefix, bitstream.Length(), scriptText.c_str(), indentPrefix );
	
	delete actionScript;
	delete scriptObject;
}


void idSWFSprite::WriteSWF( idFile_SWF& f, int characterID )
{
	// TODO frameLabels
	
	// TODO Tag_ShowFrames
	
	int tagLength = 4;
	
	for( int i = 0; i < doInitActions.Num(); i++ )
	{
		tagLength += idFile_SWF::GetTagHeaderSize( Tag_DoInitAction, doInitActions[i].Length() );
		tagLength += doInitActions[i].Length();
	}
	
	for( int i = 0; i < commands.Num(); i++ )
	{
		idSWFSprite::swfSpriteCommand_t& command = commands[i];
		
		switch( command.tag )
		{
			case Tag_PlaceObject2:
			case Tag_PlaceObject3:
			case Tag_RemoveObject2:
			case Tag_DoAction:
				tagLength += idFile_SWF::GetTagHeaderSize( command.tag, command.stream.Length() );
				tagLength += command.stream.Length();
				break;
				
			default:
				//idLib::Printf( "Export Sprite: Unhandled tag %s\n", idSWF::GetTagName( command.tag ) );
				break;
		}
	}
	
	f.WriteTagHeader( Tag_DefineSprite, tagLength );
	
	f.WriteU16( characterID );
	f.WriteU16( frameCount );
	
	
	
	/*
	for( int i = 0; i < frameLabels.Num(); i++ )
	{
		f->WriteFloatString( "%s\t<FrameLabel frameNum=\"%i\" frameLabel=\"%s\"/>\n", indentPrefix, frameLabels[i].frameNum, frameLabels[i].frameLabel.c_str() );
	}
	*/
	
	for( int i = 0; i < doInitActions.Num(); i++ )
	{
		f.WriteTagHeader( Tag_DoInitAction, doInitActions[i].Length() );
		f.Write( doInitActions[i].Ptr(), doInitActions[i].Length() );
	}
	
	for( int i = 0; i < commands.Num(); i++ )
	{
		idSWFSprite::swfSpriteCommand_t& command = commands[i];
		
		command.stream.Rewind();
		switch( command.tag )
		{
			case Tag_PlaceObject2:
			case Tag_PlaceObject3:
			case Tag_RemoveObject2:
			case Tag_DoAction:
				f.WriteTagHeader( command.tag, command.stream.Length() );
				f.Write( command.stream.Ptr(), command.stream.Length() );
				break;
				
			default:
				//idLib::Printf( "Export Sprite: Unhandled tag %s\n", idSWF::GetTagName( command.tag ) );
				break;
		}
		
	}
	
	//f.WriteTagHeader( Tag_End, 0 );
}

// RB end
>>>>>>> 2f86bde... Extended Shape-Command exports
