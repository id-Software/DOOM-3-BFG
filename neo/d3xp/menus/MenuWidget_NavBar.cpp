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
idMenuWidget_NavBar::Initialize
========================
*/
void idMenuWidget_NavBar::Initialize( idMenuHandler* data )
{
	idMenuWidget::Initialize( data );
}

/*
========================
idMenuWidget_NavBar::PrepareListElement
========================
*/
void idMenuWidget_NavBar::Update()
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

	int rightIndex = 0;

	buttonPos = initialPos;

	for( int index = 0; index < GetNumVisibleOptions() - 1; ++index )
	{
		idSWFSpriteInstance* const rightOption = GetSprite()->GetScriptObject()->GetSprite( va( "optionRight%d", index ) );
		rightOption->SetVisible( false );
		idSWFSpriteInstance* const leftOption = GetSprite()->GetScriptObject()->GetSprite( va( "optionLeft%d", index ) );
		leftOption->SetVisible( false );
	}

	for( int index = 0; index < GetTotalNumberOfOptions(); ++index )
	{
		idMenuWidget& child = GetChildByIndex( index );
		idMenuWidget_NavButton* const button = dynamic_cast< idMenuWidget_NavButton* >( &child );
		button->SetLabel( "" );
	}

	for( int index = 0; index < GetNumVisibleOptions(); ++index )
	{
		if( index < GetFocusIndex() )
		{
			idMenuWidget& child = GetChildByIndex( index );
			child.SetSpritePath( GetSpritePath(), va( "optionLeft%d", index ) );

			if( child.BindSprite( root ) )
			{
				PrepareListElement( child, index );
				child.Update();
			}

		}
		else if( index > GetFocusIndex() )
		{
			int rightChildIndex = ( GetNumVisibleOptions() - 1 ) + ( index - 1 );
			idMenuWidget& child = GetChildByIndex( rightChildIndex );
			child.SetSpritePath( GetSpritePath(), va( "optionRight%d", rightIndex ) );
			rightIndex++;

			if( child.BindSprite( root ) )
			{
				PrepareListElement( child, index );
				child.Update();
			}

		}
		else
		{
			int mainIndex = GetTotalNumberOfOptions() - 1;
			idMenuWidget& child = GetChildByIndex( mainIndex );
			child.SetSpritePath( GetSpritePath(), va( "option" ) );

			if( child.BindSprite( root ) )
			{
				PrepareListElement( child, index );
				child.Update();
			}
		}
	}
}

/*
========================
idMenuWidget_NavBar::SetListHeadings
========================
*/
void idMenuWidget_NavBar::SetListHeadings( idList< idStr >& list )
{
	headings.Clear();
	for( int index = 0; index < list.Num(); ++index )
	{
		headings.Append( list[ index ] );
	}
}

/*
========================
idMenuWidget_NavBar::GetTotalNumberOfOptions
========================
*/
int idMenuWidget_NavBar::GetTotalNumberOfOptions() const
{
	return GetChildren().Num();
}

/*
========================
idMenuWidget_NavBar::PrepareListElement
========================
*/
bool idMenuWidget_NavBar::PrepareListElement( idMenuWidget& widget, const int navIndex )
{

	if( navIndex >= GetNumVisibleOptions() || navIndex >= headings.Num() )
	{
		return false;
	}

	idMenuWidget_NavButton* const button = dynamic_cast< idMenuWidget_NavButton* >( &widget );
	if( button == NULL || button->GetSprite() == NULL )
	{
		return false;
	}

	button->SetLabel( headings[navIndex] );
	idSWFTextInstance* ti = button->GetSprite()->GetScriptObject()->GetNestedText( "txtVal" );
	if( ti != NULL )
	{
		ti->SetStrokeInfo( true, 0.7f, 1.25f );
		if( navIndex < GetFocusIndex() )
		{
			ti->SetText( headings[ navIndex ] );
			buttonPos = buttonPos + ti->GetTextLength();
			button->SetPosition( buttonPos );
			button->SetNavIndex( navIndex, idMenuWidget_NavButton::NAV_WIDGET_LEFT );
			buttonPos += leftSpacer;
		}
		else if( navIndex > GetFocusIndex() )
		{
			ti->SetText( headings[ navIndex ] );
			ti->SetStrokeInfo( true, 0.7f, 1.25f );
			button->GetSprite()->SetXPos( buttonPos );
			button->SetPosition( buttonPos );
			button->SetNavIndex( navIndex, idMenuWidget_NavButton::NAV_WIDGET_RIGHT );
			buttonPos = buttonPos + ti->GetTextLength() + rightSpacer;
		}
		else
		{
			ti->SetText( headings[ navIndex ] );
			ti->SetStrokeInfo( true, 0.7f, 1.25f );
			button->GetSprite()->SetXPos( buttonPos );
			button->SetPosition( buttonPos );
			button->SetNavIndex( navIndex, idMenuWidget_NavButton::NAV_WIDGET_SELECTED );
			buttonPos = buttonPos + ti->GetTextLength() + selectedSpacer;
		}
	}

	return true;

}
