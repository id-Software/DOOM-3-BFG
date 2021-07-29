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

int c_PlaceObject2;
int c_PlaceObject3;



/*
========================
idSWFSpriteInstance::PlaceObject2
========================
*/
void idSWFSpriteInstance::PlaceObject2( idSWFBitStream& bitstream )
{
	c_PlaceObject2++;

	uint64 flags = bitstream.ReadU8();
	int depth = bitstream.ReadU16();

	int characterID = -1;
	if( ( flags & PlaceFlagHasCharacter ) != 0 )
	{
		characterID = bitstream.ReadU16();
	}

	swfDisplayEntry_t* display = NULL;

	if( ( flags & PlaceFlagMove ) != 0 )
	{
		// modify an existing entry
		display = FindDisplayEntry( depth );
		if( display == NULL )
		{
			idLib::Warning( "PlaceObject2: trying to modify entry %d, which doesn't exist", depth );
			return;
		}
		if( characterID >= 0 )
		{
			// We are very picky about what kind of objects can change characters
			// Shapes can become other shapes, but sprites can never change
			if( display->spriteInstance || display->textInstance )
			{
				idLib::Warning( "PlaceObject2: Trying to change the character of a sprite after it's been created" );
				return;
			}
			idSWFDictionaryEntry* dictEntry = sprite->swf->FindDictionaryEntry( characterID );
			if( dictEntry != NULL )
			{
				if( dictEntry->type == SWF_DICT_SPRITE || dictEntry->type == SWF_DICT_EDITTEXT )
				{
					idLib::Warning( "PlaceObject2: Trying to change the character of a shape to a sprite" );
					return;
				}
			}
			display->characterID = characterID;
		}
	}
	else
	{
		if( characterID < 0 )
		{
			idLib::Warning( "PlaceObject2: Trying to create a new object without a character" );
			return;
		}
		// create a new entry
		display = AddDisplayEntry( depth, characterID );
		if( display == NULL )
		{
			idLib::Warning( "PlaceObject2: trying to create a new entry at %d, but an item already exists there", depth );
			return;
		}
	}
	if( ( flags & PlaceFlagHasMatrix ) != 0 )
	{
		bitstream.ReadMatrix( display->matrix );
	}
	if( ( flags & PlaceFlagHasColorTransform ) != 0 )
	{
		bitstream.ReadColorXFormRGBA( display->cxf );
	}
	if( ( flags & PlaceFlagHasRatio ) != 0 )
	{
		display->ratio = bitstream.ReadU16() * ( 1.0f / 65535.0f );
	}
	if( ( flags & PlaceFlagHasName ) != 0 )
	{
		idStr name = bitstream.ReadString();
		if( display->spriteInstance )
		{
			display->spriteInstance->name = name;
			scriptObject->Set( name, display->spriteInstance->GetScriptObject() );
		}
		else if( display->textInstance )
		{
			scriptObject->Set( name, display->textInstance->GetScriptObject() );
		}
	}
	if( ( flags & PlaceFlagHasClipDepth ) != 0 )
	{
		display->clipDepth = bitstream.ReadU16();
	}
	if( ( flags & PlaceFlagHasClipActions ) != 0 )
	{
		// FIXME: clip actions
	}
}

/*
========================
idSWFSpriteInstance::PlaceObject3
========================
*/
void idSWFSpriteInstance::PlaceObject3( idSWFBitStream& bitstream )
{
	c_PlaceObject3++;

	uint64 flags1 = bitstream.ReadU8();
	uint64 flags2 = bitstream.ReadU8();
	uint16 depth = bitstream.ReadU16();

	if( ( flags2 & PlaceFlagHasClassName ) != 0 || ( ( ( flags2 & PlaceFlagHasImage ) != 0 ) && ( ( flags1 & PlaceFlagHasCharacter ) != 0 ) ) )
	{
		bitstream.ReadString(); // ignored
	}

	int characterID = -1;
	if( ( flags1 & PlaceFlagHasCharacter ) != 0 )
	{
		characterID = bitstream.ReadU16();
	}

	swfDisplayEntry_t* display = NULL;

	if( ( flags1 & PlaceFlagMove ) != 0 )
	{
		// modify an existing entry
		display = FindDisplayEntry( depth );
		if( display == NULL )
		{
			idLib::Warning( "PlaceObject3: trying to modify entry %d, which doesn't exist", depth );
			return;
		}
		if( characterID >= 0 )
		{
			// We are very picky about what kind of objects can change characters
			// Shapes can become other shapes, but sprites can never change
			if( display->spriteInstance || display->textInstance )
			{
				idLib::Warning( "PlaceObject3: Trying to change the character of a sprite after it's been created" );
				return;
			}
			idSWFDictionaryEntry* dictEntry = sprite->swf->FindDictionaryEntry( characterID );
			if( dictEntry != NULL )
			{
				if( dictEntry->type == SWF_DICT_SPRITE || dictEntry->type == SWF_DICT_EDITTEXT )
				{
					idLib::Warning( "PlaceObject3: Trying to change the character of a shape to a sprite" );
					return;
				}
			}
			display->characterID = characterID;
		}
	}
	else
	{
		if( characterID < 0 )
		{
			idLib::Warning( "PlaceObject3: Trying to create a new object without a character" );
			return;
		}
		// create a new entry
		display = AddDisplayEntry( depth, characterID );
		if( display == NULL )
		{
			idLib::Warning( "PlaceObject3: trying to create a new entry at %d, but an item already exists there", depth );
			return;
		}
	}
	if( ( flags1 & PlaceFlagHasMatrix ) != 0 )
	{
		bitstream.ReadMatrix( display->matrix );
	}
	if( ( flags1 & PlaceFlagHasColorTransform ) != 0 )
	{
		bitstream.ReadColorXFormRGBA( display->cxf );
	}
	if( ( flags1 & PlaceFlagHasRatio ) != 0 )
	{
		display->ratio = bitstream.ReadU16() * ( 1.0f / 65535.0f );
	}
	if( ( flags1 & PlaceFlagHasName ) != 0 )
	{
		idStr name = bitstream.ReadString();
		if( display->spriteInstance )
		{
			display->spriteInstance->name = name;
			scriptObject->Set( name, display->spriteInstance->GetScriptObject() );
		}
		else if( display->textInstance )
		{
			scriptObject->Set( name, display->textInstance->GetScriptObject() );
		}
	}
	if( ( flags1 & PlaceFlagHasClipDepth ) != 0 )
	{
		display->clipDepth = bitstream.ReadU16();
	}
	if( ( flags2 & PlaceFlagHasFilterList ) != 0 )
	{
		// we don't support filters and because the filter list is variable length we
		// can't support anything after the filter list either (blend modes and clip actions)
		idLib::Warning( "PlaceObject3: has filters" );
		return;
	}
	if( ( flags2 & PlaceFlagHasBlendMode ) != 0 )
	{
		display->blendMode = bitstream.ReadU8();
	}
	if( ( flags1 & PlaceFlagHasClipActions ) != 0 )
	{
		// FIXME:
	}
}

/*
========================
idSWFSpriteInstance::RemoveObject2
========================
*/
void idSWFSpriteInstance::RemoveObject2( idSWFBitStream& bitstream )
{
	RemoveDisplayEntry( bitstream.ReadU16() );
}
