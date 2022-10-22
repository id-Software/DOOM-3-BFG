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
#include "precompiled.h"
#pragma hdrstop

/*
========================
idSWF::idSWFDictionaryEntry::idSWFDictionaryEntry
========================
*/
idSWFDictionaryEntry::idSWFDictionaryEntry() :
	type( SWF_DICT_NULL ),
	material( NULL ),
	shape( NULL ),
	sprite( NULL ),
	font( NULL ),
	text( NULL ),
	edittext( NULL ),
	imageSize( 0, 0 ),
	imageAtlasOffset( 0, 0 ),
	channelScale( 1.0f, 1.0f, 1.0f, 1.0f )
{
}

/*
========================
idSWF::idSWFDictionaryEntry::idSWFDictionaryEntry
========================
*/
idSWFDictionaryEntry::~idSWFDictionaryEntry()
{
	delete shape;
	delete sprite;
	delete font;
	delete text;
	delete edittext;
}

/*
========================
idSWF::idSWFDictionaryEntry::operator=
This exists mostly so idList works right
========================
*/
idSWFDictionaryEntry& idSWFDictionaryEntry::operator=( idSWFDictionaryEntry& other )
{
	type = other.type;
	material = other.material;
	shape = other.shape;
	sprite = other.sprite;
	font = other.font;
	text = other.text;
	edittext = other.edittext;
	imageSize = other.imageSize;
	imageAtlasOffset = other.imageAtlasOffset;
	other.type = SWF_DICT_NULL;
	other.material = NULL;
	other.shape = NULL;
	other.sprite = NULL;
	other.font = NULL;
	other.text = NULL;
	other.edittext = NULL;
	return *this;
}

/*
========================
idSWF::idSWFDictionaryEntry::operator= (move)
========================
*/
idSWFDictionaryEntry& idSWFDictionaryEntry::operator=( idSWFDictionaryEntry&& other )
{
	type = other.type;
	material = other.material;
	shape = other.shape;
	sprite = other.sprite;
	font = other.font;
	text = other.text;
	edittext = other.edittext;
	imageSize = other.imageSize;
	imageAtlasOffset = other.imageAtlasOffset;
	other.type = SWF_DICT_NULL;
	other.material = NULL;
	other.shape = NULL;
	other.sprite = NULL;
	other.font = NULL;
	other.text = NULL;
	other.edittext = NULL;
	return *this;
}

/*
========================
idSWF::AddDictionaryEntry
========================
*/
idSWFDictionaryEntry* idSWF::AddDictionaryEntry( int characterID, swfDictType_t type )
{

	if( dictionary.Num() < characterID + 1 )
	{
		dictionary.SetNum( characterID + 1 );
	}

	if( dictionary[ characterID ].type != SWF_DICT_NULL )
	{
		idLib::Warning( "%s: Duplicate character %d", filename.c_str(), characterID );
		return NULL;
	}

	dictionary[ characterID ].type = type;

	if( ( type == SWF_DICT_SHAPE ) || ( type == SWF_DICT_MORPH ) )
	{
		dictionary[ characterID ].shape = new( TAG_SWF ) idSWFShape;
	}
	else if( type == SWF_DICT_SPRITE )
	{
		dictionary[ characterID ].sprite = new( TAG_SWF ) idSWFSprite( this );
	}
	else if( type == SWF_DICT_FONT )
	{
		dictionary[ characterID ].font = new( TAG_SWF ) idSWFFont;
	}
	else if( type == SWF_DICT_TEXT )
	{
		dictionary[ characterID ].text = new( TAG_SWF ) idSWFText;
	}
	else if( type == SWF_DICT_EDITTEXT )
	{
		dictionary[ characterID ].edittext = new( TAG_SWF ) idSWFEditText;
	}

	return &dictionary[ characterID ];
}

/*
========================
FindDictionaryEntry
========================
*/
idSWFDictionaryEntry* idSWF::FindDictionaryEntry( int characterID, swfDictType_t type )
{

	if( dictionary.Num() < characterID + 1 )
	{
		idLib::Warning( "%s: Could not find character %d", filename.c_str(), characterID );
		return NULL;
	}

	if( dictionary[ characterID ].type != type )
	{
		idLib::Warning( "%s: Character %d is the wrong type", filename.c_str(), characterID );
		return NULL;
	}

	return &dictionary[ characterID ];
}


/*
========================
FindDictionaryEntry
========================
*/
idSWFDictionaryEntry* idSWF::FindDictionaryEntry( int characterID )
{

	if( dictionary.Num() < characterID + 1 )
	{
		idLib::Warning( "%s: Could not find character %d", filename.c_str(), characterID );
		return NULL;
	}

	return &dictionary[ characterID ];
}
