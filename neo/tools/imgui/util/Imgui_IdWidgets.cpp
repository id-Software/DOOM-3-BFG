/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.
Copyright (C) 2022 Stephen Pridham

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "precompiled.h"
#pragma hdrstop

#include "imgui.h"

#include "Imgui_IdWidgets.h"

static const char* bodyContentsNames[5] =
{
	"solid",
	"body",
	"corpse",
	"playerclip",
	"monsterclip"
};

static int contentMappingFlags[5] =
{
	CONTENTS_SOLID,
	CONTENTS_BODY,
	CONTENTS_CORPSE,
	CONTENTS_PLAYERCLIP,
	CONTENTS_MONSTERCLIP
};

MultiSelectWidget::MultiSelectWidget( const char** aNames, int* contentMapping, int aNumEntries )
	: names( aNames )
	, contentMapping( contentMapping )
	, numEntries( aNumEntries )
	, selectables( nullptr )
{
	selectables = ( bool* )Mem_Alloc( numEntries * sizeof( bool ), TAG_AF );
	std::fill( selectables, selectables + numEntries, false );
}

MultiSelectWidget::~MultiSelectWidget()
{
	Mem_Free( selectables );
}

void MultiSelectWidget::Update( int index, bool value )
{
	assert( index < numEntries );
	selectables[index] = value;
}

void MultiSelectWidget::UpdateWithBitFlags( int bitFlags )
{
	for( int i = 0; i < numEntries; i++ )
	{
		Update( i, bitFlags & contentMapping[i] );
	}
}

bool DoMultiSelect( MultiSelectWidget* widget, int* contents )
{
	bool pressed = false;
	for( int i = 0; i < 5; i++ )
	{
		if( ImGui::Selectable( widget->names[i], &widget->selectables[i] ) )
		{
			pressed = true;
			if( widget->selectables[i] )
			{
				*contents |= widget->contentMapping[i];
			}
			else
			{
				*contents &= ~widget->contentMapping[i];
			}
		}
	}

	return pressed;
}

void HelpMarker( const char* desc )
{
	ImGui::TextDisabled( "(?)" );
	if( ImGui::IsItemHovered() )
	{
		ImGui::BeginTooltip();
		ImGui::PushTextWrapPos( ImGui::GetFontSize() * 35.0f );
		ImGui::TextUnformatted( desc );
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}

bool StringListItemGetter( void* data, int index, const char** outText )
{
	idStrList* list = reinterpret_cast<idStrList*>( data );
	assert( index < list->Num() );

	*outText = ( *list )[index];
	return true;
}

MultiSelectWidget MakePhysicsContentsSelector()
{
	return MultiSelectWidget( bodyContentsNames, contentMappingFlags, 5 );
}
