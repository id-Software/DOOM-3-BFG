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
#include "../Game_local.h"

/*
========================
idMenuWidget_LobbyList::Update
========================
*/
void idMenuWidget_LobbyList::Update()
{

	if( GetSWFObject() == NULL )
	{
		return;
	}

	idSWFScriptObject& root = GetSWFObject()->GetRootObject();

	if( !BindSprite( root ) )
	{
		return;
	}

	for( int i = 0; i < headings.Num(); ++i )
	{
		idSWFTextInstance* txtHeading = GetSprite()->GetScriptObject()->GetNestedText( va( "heading%d", i ) );
		if( txtHeading != NULL )
		{
			txtHeading->SetText( headings[i] );
			txtHeading->SetStrokeInfo( true, 0.75f, 1.75f );
		}
	}

	for( int optionIndex = 0; optionIndex < GetNumVisibleOptions(); ++optionIndex )
	{
		bool shown = false;
		if( optionIndex < GetChildren().Num() )
		{
			idMenuWidget& child = GetChildByIndex( optionIndex );
			child.SetSpritePath( GetSpritePath(), va( "item%d", optionIndex ) );
			if( child.BindSprite( root ) )
			{
				shown = PrepareListElement( child, optionIndex );
				if( shown )
				{
					child.GetSprite()->SetVisible( true );
					child.Update();
				}
				else
				{
					child.GetSprite()->SetVisible( false );
				}
			}
		}
	}
}

/*
========================
idMenuWidget_LobbyList::PrepareListElement
========================
*/
bool idMenuWidget_LobbyList::PrepareListElement( idMenuWidget& widget, const int childIndex )
{

	idMenuWidget_LobbyButton* const button = dynamic_cast< idMenuWidget_LobbyButton* >( &widget );
	if( button == NULL )
	{
		return false;
	}

	if( !button->IsValid() )
	{
		return false;
	}

	return true;

}

/*
========================
idMenuWidget_LobbyList::SetHeadingInfo
========================
*/
void idMenuWidget_LobbyList::SetHeadingInfo( idList< idStr >& list )
{
	headings.Clear();
	for( int index = 0; index < list.Num(); ++index )
	{
		headings.Append( list[ index ] );
	}
}

/*
========================
idMenuWidget_LobbyList::SetEntryData
========================
*/
void idMenuWidget_LobbyList::SetEntryData( int index, idStr name, voiceStateDisplay_t voiceState )
{

	if( GetChildren().Num() == 0 || index >= GetChildren().Num() )
	{
		return;
	}

	idMenuWidget& child = GetChildByIndex( index );
	idMenuWidget_LobbyButton* const button = dynamic_cast< idMenuWidget_LobbyButton* >( &child );

	if( button == NULL )
	{
		return;
	}

	button->SetButtonInfo( name, voiceState );
}