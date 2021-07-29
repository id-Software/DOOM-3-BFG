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
idMenuWidget_MenuBar::Initialize
========================
*/
void idMenuWidget_MenuBar::Initialize( idMenuHandler* data )
{
	idMenuWidget::Initialize( data );
}

/*
========================
idMenuWidget_MenuBar::Update
========================
*/
void idMenuWidget_MenuBar::Update()
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

	totalWidth = 0.0f;
	buttonPos = 0.0f;

	for( int index = 0; index < GetNumVisibleOptions(); ++index )
	{

		if( index >= children.Num() )
		{
			break;
		}

		if( index != 0 )
		{
			totalWidth += rightSpacer;
		}

		idMenuWidget& child = GetChildByIndex( index );
		child.SetSpritePath( GetSpritePath(), va( "btn%d", index ) );
		if( child.BindSprite( root ) )
		{
			PrepareListElement( child, index );
			child.Update();
		}
	}

	// 640 is half the size of our flash files width
	float xPos = 640.0f - ( totalWidth / 2.0f );
	GetSprite()->SetXPos( xPos );

	idSWFSpriteInstance* backing = GetSprite()->GetScriptObject()->GetNestedSprite( "backing" );
	if( backing != NULL )
	{
		if( menuData != NULL && menuData->GetPlatform() != 2 )
		{
			backing->SetVisible( false );
		}
		else
		{
			backing->SetVisible( true );
			backing->SetXPos( totalWidth / 2.0f );
		}
	}

}

/*
========================
idMenuWidget_MenuBar::SetListHeadings
========================
*/
void idMenuWidget_MenuBar::SetListHeadings( idList< idStr >& list )
{
	headings.Clear();
	for( int index = 0; index < list.Num(); ++index )
	{
		headings.Append( list[ index ] );
	}
}

/*
========================
idMenuWidget_MenuBar::GetTotalNumberOfOptions
========================
*/
int idMenuWidget_MenuBar::GetTotalNumberOfOptions() const
{
	return GetChildren().Num();
}

/*
========================
idMenuWidget_MenuBar::PrepareListElement
========================
*/
bool idMenuWidget_MenuBar::PrepareListElement( idMenuWidget& widget, const int navIndex )
{

	if( navIndex >= GetNumVisibleOptions() )
	{
		return false;
	}

	idMenuWidget_MenuButton* const button = dynamic_cast< idMenuWidget_MenuButton* >( &widget );
	if( button == NULL || button->GetSprite() == NULL )
	{
		return false;
	}

	if( navIndex >= headings.Num() )
	{
		button->SetLabel( "" );
	}
	else
	{
		button->SetLabel( headings[navIndex] );
		idSWFTextInstance* ti = button->GetSprite()->GetScriptObject()->GetNestedText( "txtVal" );
		if( ti != NULL )
		{
			ti->SetStrokeInfo( true, 0.7f, 1.25f );
			ti->SetText( headings[ navIndex ] );
			button->SetPosition( buttonPos );
			totalWidth += ti->GetTextLength();
			buttonPos += rightSpacer + ti->GetTextLength();
		}
	}

	return true;
}
