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

void idMenuWidget_ScrollBar::Initialize( idMenuHandler* data )
{
	idMenuWidget::Initialize( data );

	AddEventAction( WIDGET_EVENT_DRAG_START ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_DRAG_START, WIDGET_EVENT_DRAG_START ) );
	AddEventAction( WIDGET_EVENT_DRAG_STOP ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_DRAG_STOP, WIDGET_EVENT_DRAG_STOP ) );
}

/*
========================
idMenuWidget_ScrollBar::Update
========================
*/
void idMenuWidget_ScrollBar::Update()
{

	if( GetSWFObject() == NULL )
	{
		return;
	}

	idSWFScriptObject& root = GetSWFObject()->GetRootObject();
	if( !BindSprite( root ) || GetSprite() == NULL )
	{
		return;
	}

	if( GetParent() == NULL )
	{
		return;
	}

	CalcTopAndBottom();

	idSWFScriptObject* node = GetSprite()->GetScriptObject()->GetNestedObj( "node" );
	idSWFSpriteInstance* nodeSprite = GetSprite()->GetScriptObject()->GetNestedSprite( "node" );
	if( node != NULL && nodeSprite != NULL )
	{
		node->Set( "onDrag", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_DRAG_START, 0 ) );
		node->Set( "onRelease", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_DRAG_STOP, 0 ) );

		const idMenuWidget_DynamicList* const list = dynamic_cast< const idMenuWidget_DynamicList* const >( GetParent() );
		if( list != NULL )
		{
			float percent = 0.0f;
			if( ( list->GetTotalNumberOfOptions() - list->GetNumVisibleOptions() ) > 0 )
			{
				percent = ( float )list->GetViewOffset() / ( ( float )list->GetTotalNumberOfOptions() - list->GetNumVisibleOptions() );
				float range = yBot - yTop;
				nodeSprite->SetVisible( true );
				nodeSprite->SetYPos( percent * range );
				//GetSprite()->StopFrame( int( ( percent * 100.0f ) + 2.0f ) );
			}
			else
			{
				nodeSprite->SetVisible( 0 );
			}
		}

		idMenuWidget_InfoBox* const infoBox = dynamic_cast< idMenuWidget_InfoBox* const >( GetParent() );
		if( infoBox != NULL )
		{
			float percent = 0.0f;
			if( infoBox->GetMaxScroll() == 0 )
			{
				nodeSprite->SetVisible( 0 );
			}
			else
			{
				percent = ( float )infoBox->GetScroll() / ( float )infoBox->GetMaxScroll();
				float range = yBot - yTop;
				nodeSprite->SetVisible( true );
				nodeSprite->SetYPos( percent * range );
				//GetSprite()->StopFrame( int( ( percent * 100.0f ) + 2.0f ) );
			}
		}
	}
}

/*
========================
idMenuWidget_ScrollBar::CalcTopAndBottom
========================
*/
void idMenuWidget_ScrollBar::CalcTopAndBottom()
{

	if( GetSWFObject() == NULL )
	{
		return;
	}

	idSWFScriptObject& root = GetSWFObject()->GetRootObject();
	if( !BindSprite( root ) || GetSprite() == NULL )
	{
		return;
	}

	int tempPos = 0.0f;
	idSWFSpriteInstance* curMC = GetSprite()->GetScriptObject()->GetNestedSprite( "top" );
	if( curMC != NULL )
	{
		tempPos = curMC->GetYPos();
		while( curMC->parent != NULL )
		{
			curMC = curMC->parent;
			tempPos += curMC->GetYPos();
		}
	}
	yTop = tempPos;

	tempPos = 0.0f;
	curMC = GetSprite()->GetScriptObject()->GetNestedSprite( "bottom" );
	if( curMC != NULL )
	{
		tempPos = curMC->GetYPos();
		while( curMC->parent != NULL )
		{
			curMC = curMC->parent;
			tempPos += curMC->GetYPos();
		}
	}
	yBot = tempPos;
}

/*
========================
idMenuWidget_ScrollBar::CalculatePosition
========================
*/
void idMenuWidget_ScrollBar::CalculatePosition( float x, float y )
{
	if( GetSprite() == NULL )
	{
		return;
	}

	if( y >= yTop && y <= yBot )
	{
		float range = yBot - yTop;
		float val = y - yTop;

		float percent = val / range;
		idSWFSpriteInstance* node = GetSprite()->GetScriptObject()->GetNestedSprite( "node" );
		if( node != NULL )
		{
			node->SetYPos( percent * range );
		}

		idMenuWidget_DynamicList* const list = dynamic_cast< idMenuWidget_DynamicList* const >( GetParent() );
		if( list != NULL )
		{
			float maxScroll = list->GetTotalNumberOfOptions() - list->GetNumVisibleOptions();
			int offset = list->GetViewOffset();

			float segment = ( maxScroll + 0.5f ) / 100.0f;
			int newOffset = ( int )( ( ( percent * segment ) * 100.0f ) );

			if( newOffset >= maxScroll )
			{
				int i = 1;
				i = i;
			}

			if( newOffset != offset )
			{

				int viewIndex = list->GetViewIndex();
				list->SetViewOffset( newOffset );
				idLib::Printf( "newOffset = %d\n", newOffset );
				if( viewIndex < newOffset )
				{
					viewIndex = newOffset;
					list->SetViewIndex( viewIndex );
				}
				else if( viewIndex > ( newOffset + list->GetNumVisibleOptions() - 1 ) )
				{
					viewIndex = ( newOffset + list->GetNumVisibleOptions() - 1 );
					list->SetViewIndex( viewIndex );
				}
				idLib::Printf( "newView = %d\n", list->GetViewIndex() );

				int newFocus = viewIndex - newOffset;
				if( newFocus >= 0 )
				{
					list->SetFocusIndex( newFocus );
				}
				list->Update();
			}
		}

		idMenuWidget_InfoBox* const infoBox = dynamic_cast< idMenuWidget_InfoBox* const >( GetParent() );
		if( infoBox != NULL )
		{
			float maxScroll = infoBox->GetMaxScroll();
			int scroll = infoBox->GetScroll();
			float segment = ( maxScroll + 1.0f ) / 100.0f;
			int newScroll = ( int )( ( ( percent * segment ) * 100.0f ) );
			if( newScroll != scroll )
			{
				infoBox->SetScroll( newScroll );
			}
		}
	}
}

/*
========================
idMenuWidget_ScrollBar::HandleAction
========================
*/
bool idMenuWidget_ScrollBar::HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled )
{

	widgetAction_t actionType = action.GetType();

	switch( actionType )
	{
		case WIDGET_ACTION_SCROLL_DRAG:
		{

			if( event.parms.Num() != 3 )
			{
				return true;
			}

			dragging = true;

			float x = event.parms[0].ToFloat();
			float y = event.parms[1].ToFloat();
			bool initial = event.parms[2].ToBool();
			if( initial )
			{
				CalcTopAndBottom();
			}

			CalculatePosition( x, y );
			return true;
		}
		case( widgetAction_t ) WIDGET_ACTION_EVENT_DRAG_STOP:               // SRS - Cast actionHandler_t to widgetAction_t
		{
			dragging = false;
			return true;
		}
	}

	return idMenuWidget::HandleAction( action, event, widget, forceHandled );
}

/*
========================
idMenuWidget_Help::ObserveEvent
========================
*/
void idMenuWidget_ScrollBar::ObserveEvent( const idMenuWidget& widget, const idWidgetEvent& event )
{
	switch( event.type )
	{
		case WIDGET_EVENT_SCROLL_UP:
		case WIDGET_EVENT_SCROLL_DOWN:
		case WIDGET_EVENT_SCROLL_UP_LSTICK:
		case WIDGET_EVENT_SCROLL_UP_RSTICK:
		case WIDGET_EVENT_SCROLL_DOWN_LSTICK:
		case WIDGET_EVENT_SCROLL_DOWN_RSTICK:
		case WIDGET_EVENT_FOCUS_ON:
		{
			if( !dragging )
			{
				Update();
			}
			break;
		}
	}
}
