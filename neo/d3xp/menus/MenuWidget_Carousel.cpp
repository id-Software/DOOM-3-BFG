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

void idMenuWidget_Carousel::Initialize( idMenuHandler* data )
{
	idMenuWidget::Initialize( data );

	class idCarouselRefresh : public idSWFScriptFunction_RefCounted
	{
	public:
		idCarouselRefresh( idMenuWidget_Carousel* _widget ) :
			widget( _widget )
		{
		}

		idSWFScriptVar Call( idSWFScriptObject* thisObject, const idSWFParmList& parms )
		{

			if( widget == NULL )
			{
				return idSWFScriptVar();
			}

			if( widget->GetMoveDiff() != 0 )
			{
				int diff = widget->GetMoveDiff();
				diff--;

				if( widget->GetScrollLeft() )
				{
					widget->SetViewIndex( widget->GetViewIndex() - 1 );
				}
				else
				{
					widget->SetViewIndex( widget->GetViewIndex() + 1 );
				}

				if( diff > 0 )
				{
					if( widget->GetScrollLeft() )
					{
						widget->MoveToIndex( ( widget->GetNumVisibleOptions() / 2 ) + diff );
					}
					else
					{
						widget->MoveToIndex( diff );
					}
				}
				else
				{
					widget->SetMoveDiff( 0 );
				}
			}

			widget->Update();
			return idSWFScriptVar();
		}
	private:
		idMenuWidget_Carousel* 	widget;
	};

	if( GetSWFObject() != NULL )
	{
		GetSWFObject()->SetGlobal( "refreshCarousel", new idCarouselRefresh( this ) );
	}
}

/*
========================
idMenuWidget_Carousel::Update
========================
*/
void idMenuWidget_Carousel::Update()
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

	int midPoint = GetNumVisibleOptions() / 2 + 1;
	for( int optionIndex = 0; optionIndex < GetNumVisibleOptions(); ++optionIndex )
	{

		int listIndex = viewIndex + optionIndex;
		if( optionIndex >= midPoint )
		{
			listIndex = viewIndex - ( optionIndex - ( midPoint - 1 ) );
		}

		idMenuWidget& child = GetChildByIndex( optionIndex );
		child.SetSpritePath( GetSpritePath(), va( "item%d", optionIndex ) );
		if( child.BindSprite( root ) )
		{
			if( listIndex < 0 || listIndex >= GetTotalNumberOfOptions() )
			{
				child.SetState( WIDGET_STATE_HIDDEN );
			}
			else
			{
				idMenuWidget_Button* const button = dynamic_cast< idMenuWidget_Button* >( &child );
				button->SetImg( imgList[ listIndex ] );
				child.Update();
				if( optionIndex == focusIndex )
				{
					child.SetState( WIDGET_STATE_SELECTING );
				}
				else
				{
					child.SetState( WIDGET_STATE_NORMAL );
				}
			}
		}
	}
}

/*
========================
idMenuWidget_Carousel::SetListImages
========================
*/
void idMenuWidget_Carousel::SetListImages( idList<const idMaterial*>& list )
{
	imgList.Clear();
	for( int i = 0; i < list.Num(); ++i )
	{
		imgList.Append( list[ i ] );
	}
}

/*
========================
idMenuWidget_Carousel::Update
========================
*/
bool idMenuWidget_Carousel::HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled )
{
	return idMenuWidget::HandleAction( action, event, widget, forceHandled );
}

/*
========================
idMenuWidget_Carousel::MoveToFirstItem
========================
*/
void idMenuWidget_Carousel::MoveToFirstItem( bool instant )
{
	if( instant )
	{
		moveDiff = 0;
		viewIndex = 0;
		moveToIndex = 0;

		idSWFScriptObject& root = GetSWFObject()->GetRootObject();
		if( BindSprite( root ) )
		{
			GetSprite()->StopFrame( 1 );
		}

		Update();
	}
}

/*
========================
idMenuWidget_Carousel::MoveToLastItem
========================
*/
void idMenuWidget_Carousel::MoveToLastItem( bool instant )
{
	if( instant )
	{
		moveDiff = 0;
		viewIndex = GetTotalNumberOfOptions() - 1;
		moveToIndex = GetTotalNumberOfOptions() - 1;

		idSWFScriptObject& root = GetSWFObject()->GetRootObject();
		if( BindSprite( root ) )
		{
			GetSprite()->StopFrame( 1 );
		}
		Update();
	}
}

/*
========================
idMenuWidget_Carousel::Update
========================
*/
void idMenuWidget_Carousel::MoveToIndex( int index, bool instant )
{

	idLib::Printf( "moveToIndex %i\n", index );

	if( instant )
	{
		viewIndex = index;
		moveDiff = 0;
		moveToIndex = viewIndex;

		idLib::Printf( "moveDiff = %i\n", moveDiff );

		idSWFScriptObject& root = GetSWFObject()->GetRootObject();
		if( BindSprite( root ) )
		{
			GetSprite()->StopFrame( 1 );
		}

		Update();
		return;
	}

	if( index == 0 )
	{
		fastScroll = false;
		moveDiff = 0;

		idLib::Printf( "moveDiff = %i\n", moveDiff );

		viewIndex = moveToIndex;
		return;
	}

	int midPoint = GetNumVisibleOptions() / 2;
	scrollLeft = false;
	if( index > midPoint )
	{
		moveDiff = index - midPoint;
		scrollLeft = true;
	}
	else
	{
		moveDiff = index;
	}

	idLib::Printf( "moveDiff = %i\n", moveDiff );

	if( scrollLeft )
	{
		moveToIndex = viewIndex - moveDiff;
		if( moveToIndex < 0 )
		{
			moveToIndex = 0;
			int diff = 0 - moveToIndex;
			moveDiff -= diff;
		}
	}
	else
	{
		moveToIndex = viewIndex + moveDiff;
		if( moveToIndex >= GetTotalNumberOfOptions() )
		{
			moveDiff = GetTotalNumberOfOptions() - GetViewIndex() - 1;
			moveToIndex = GetTotalNumberOfOptions() - 1;
		}
	}

	idLib::Printf( "moveDiff = %i\n", moveDiff );

	if( moveDiff != 0 )
	{
		if( moveDiff > 1 )
		{
			if( scrollLeft )
			{
				GetSprite()->PlayFrame( "leftFast" );
			}
			else
			{
				GetSprite()->PlayFrame( "rightFast" );
			}
		}
		else
		{
			if( scrollLeft )
			{
				GetSprite()->PlayFrame( "left" );
			}
			else
			{
				GetSprite()->PlayFrame( "right" );
			}
		}
	}

	idLib::Printf( "moveDiff = %i\n", moveDiff );
}

