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
idMenuWidget::idMenuWidget
========================
*/
idMenuWidget::idMenuWidget() :
	handlerIsParent( false ),
	menuData( NULL ),
	swfObj( NULL ),
	boundSprite( NULL ),
	parent( NULL ),
	dataSource( NULL ),
	dataSourceFieldIndex( 0 ),
	focusIndex( 0 ),
	widgetState( WIDGET_STATE_NORMAL ),
	refCount( 0 ),
	noAutoFree( false )
{

	eventActionLookup.SetNum( eventActionLookup.Max() );
	for( int i = 0; i < eventActionLookup.Num(); ++i )
	{
		eventActionLookup[ i ] = INVALID_ACTION_INDEX;
	}
}

/*
========================
idMenuWidget::~idMenuWidget
========================
*/
idMenuWidget::~idMenuWidget()
{
	Cleanup();
}

void idMenuWidget::Cleanup()
{
	for( int j = 0; j < observers.Num(); ++j )
	{
		assert( observers[j]->refCount > 0 );
		observers[ j ]->Release();
	}

	observers.Clear();

	// free all children
	for( int i = 0; i < children.Num(); ++i )
	{
		assert( children[i]->refCount > 0 );
		children[ i ]->Release();
	}

	children.Clear();
}

/*
========================
idMenuWidget::AddChild
========================
*/
void idMenuWidget::AddChild( idMenuWidget* widget )
{
	if( !verify( children.Find( widget ) == NULL ) )
	{
		return;	// attempt to add a widget that was already in the list
	}

	if( widget->GetParent() != NULL )
	{
		// take out of previous parent
		widget->GetParent()->RemoveChild( widget );
	}

	widget->AddRef();
	widget->SetParent( this );
	children.Append( widget );
}

/*
========================
idMenuWidget::RemoveAllChildren
========================
*/
void idMenuWidget::RemoveAllChildren()
{

	for( int i = 0; i < children.Num(); ++ i )
	{

		assert( children[ i ]->GetParent() == this );

		children[ i ]->SetParent( NULL );
		children[ i ]->Release();
	}

	children.Clear();
}

/*
========================
idMenuWidget::RemoveChild
========================
*/
void idMenuWidget::RemoveChild( idMenuWidget* widget )
{
	assert( widget->GetParent() == this );

	children.Remove( widget );
	widget->SetParent( NULL );
	widget->Release();
}

/*
========================
idMenuWidget::RemoveChild
========================
*/
bool idMenuWidget::HasChild( idMenuWidget* widget )
{
	for( int i = 0; i < children.Num(); ++ i )
	{
		if( children[ i ] == widget )
		{
			return true;
		}
	}
	return false;
}

/*
========================
idMenuWidget::ReceiveEvent

Events received through this function are passed to the innermost focused widget first, and then
propagates back through each widget within the focus chain.  The first widget that handles the
event will stop propagation.

Each widget along the way will fire off an event to its observers, whether or not it actually
handles the event.

Note: How the focus chain is calculated:
Descend through GetFocus() calls until you reach a NULL focus.  The terminating widget is the
innermost widget, while *this* widget is the outermost widget.
========================
*/
void idMenuWidget::ReceiveEvent( const idWidgetEvent& event )
{
	idStaticList< idMenuWidget*, 16 > focusChain;

	int focusRunawayCounter = focusChain.Max();
	idMenuWidget* focusedWidget = this;
	while( focusedWidget != NULL && --focusRunawayCounter != 0 )
	{
		focusChain.Append( focusedWidget );
		focusedWidget = focusedWidget->GetFocus();
	}

	// If hitting this then more than likely you have a self-referential chain.  If that's not
	// the case, then you may need to increase the size of the focusChain list.
	assert( focusRunawayCounter != 0 );
	for( int focusIndex = focusChain.Num() - 1; focusIndex >= 0; --focusIndex )
	{
		idMenuWidget* const focusedWidget = focusChain[ focusIndex ];

		if( focusedWidget->ExecuteEvent( event ) )
		{
			break;	// this widget has handled the event, so stop propagation
		}
	}
}


/*
========================
idMenuWidget::ExecuteEvent

Handles the event directly, and doesn't pass it through the focus chain.

This should only be used in very specific circumstances!  Most events should go to the focus.
========================
*/
bool idMenuWidget::ExecuteEvent( const idWidgetEvent& event )
{
	idList< idWidgetAction, TAG_IDLIB_LIST_MENU >* const actions = GetEventActions( event.type );

	if( actions != NULL )
	{
		for( int actionIndex = 0; actionIndex < actions->Num(); ++actionIndex )
		{
			HandleAction( ( *actions )[ actionIndex ], event, this );
		}
	}

	SendEventToObservers( event );

	return actions != NULL && actions->Num() > 0;
}

/*
========================
idMenuWidget::SendEventToObservers

Sends an event to all the observers
========================
*/
void idMenuWidget::SendEventToObservers( const idWidgetEvent& event )
{
	for( int i = 0; i < observers.Num(); ++i )
	{
		observers[ i ]->ObserveEvent( *this, event );
	}
}

/*
========================
idMenuWidget::RegisterEventObserver

Adds an observer to our observers list
========================
*/
void idMenuWidget::RegisterEventObserver( idMenuWidget* observer )
{
	if( !verify( observers.Find( observer ) == NULL ) )
	{
		return;
	}

	observer->AddRef();
	observers.Append( observer );
}

/*
========================
idMenuWidget::SetSpritePath
========================
*/
void idMenuWidget::SetSpritePath( const char* arg1, const char* arg2, const char* arg3, const char* arg4, const char* arg5 )
{
	const char* args[] = { arg1, arg2, arg3, arg4, arg5 };
	const int numArgs = sizeof( args ) / sizeof( args[ 0 ] );
	spritePath.Clear();
	for( int i = 0; i < numArgs; ++i )
	{
		if( args[ i ] == NULL )
		{
			break;
		}
		spritePath.Append( args[ i ] );
	}
}

/*
========================
idMenuWidget::SetSpritePath
========================
*/
void idMenuWidget::SetSpritePath( const idList< idStr >& spritePath_, const char* arg1, const char* arg2, const char* arg3, const char* arg4, const char* arg5 )
{
	const char* args[] = { arg1, arg2, arg3, arg4, arg5 };
	const int numArgs = sizeof( args ) / sizeof( args[ 0 ] );
	spritePath = spritePath_;
	for( int i = 0; i < numArgs; ++i )
	{
		if( args[ i ] == NULL )
		{
			break;
		}
		spritePath.Append( args[ i ] );
	}
}

/*
========================
idMenuWidget::ClearSprite
========================
*/
void idMenuWidget::ClearSprite()
{
	if( GetSprite() == NULL )
	{
		return;
	}
	GetSprite()->SetVisible( false );
	boundSprite = NULL;
}

/*
========================
idMenuWidget::GetSWFObject
========================
*/
idSWF* idMenuWidget::GetSWFObject()
{

	if( swfObj != NULL )
	{
		return swfObj;
	}

	if( parent != NULL )
	{
		return parent->GetSWFObject();
	}

	if( menuData != NULL )
	{
		return menuData->GetGUI();
	}

	return NULL;
}

/*
========================
idMenuWidget::GetMenuData
========================
*/
idMenuHandler* idMenuWidget::GetMenuData()
{
	if( parent != NULL )
	{
		return parent->GetMenuData();
	}

	return menuData;
}

/*
========================
idMenuWidget::BindSprite

Takes the sprite path strings and resolves it to an actual sprite relative to a given root.

This is setup in this manner, because we can't resolve from path -> sprite immediately since
SWFs aren't necessarily loaded at the time widgets are instantiated.
========================
*/
bool idMenuWidget::BindSprite( idSWFScriptObject& root )
{

	const char* args[ 6 ] = { NULL };
	assert( GetSpritePath().Num() > 0 );
	for( int i = 0; i < GetSpritePath().Num(); ++i )
	{
		args[ i ] = GetSpritePath()[ i ].c_str();
	}
	boundSprite = root.GetNestedSprite( args[ 0 ], args[ 1 ], args[ 2 ], args[ 3 ], args[ 4 ], args[ 5 ] );
	return boundSprite != NULL;
}

/*
========================
idMenuWidget::Show
========================
*/
void idMenuWidget::Show()
{
	if( GetSWFObject() == NULL )
	{
		return;
	}

	if( !BindSprite( GetSWFObject()->GetRootObject() ) )
	{
		return;
	}

	GetSprite()->SetVisible( true );
	int currentFrame = GetSprite()->GetCurrentFrame();
	int findFrame = GetSprite()->FindFrame( "rollOn" );
	int idleFrame = GetSprite()->FindFrame( "idle" );
	if( currentFrame == findFrame || ( currentFrame > 1 && currentFrame <= idleFrame ) )
	{
		return;
	}

	GetSprite()->PlayFrame( findFrame );
}

/*
========================
idMenuWidget::Hide
========================
*/
void idMenuWidget::Hide()
{
	if( GetSWFObject() == NULL )
	{
		return;
	}

	if( !BindSprite( GetSWFObject()->GetRootObject() ) )
	{
		return;
	}

	int currentFrame = GetSprite()->GetCurrentFrame();
	int findFrame = GetSprite()->FindFrame( "rollOff" );
	if( currentFrame >= findFrame || currentFrame == 1 )
	{
		return;
	}

	GetSprite()->PlayFrame( findFrame );
}

/*
========================
idMenuWidget::SetDataSource
========================
*/
void idMenuWidget::SetDataSource( idMenuDataSource* dataSource_, const int fieldIndex )
{
	dataSource = dataSource_;
	dataSourceFieldIndex = fieldIndex;
}

/*
========================
idMenuWidget::SetFocusIndex
========================
*/
void idMenuWidget::SetFocusIndex( const int index, bool skipSound )
{

	if( GetChildren().Num() == 0 )
	{
		return;
	}

	const int oldIndex = focusIndex;

	assert( index >= 0 && index < GetChildren().Num() ); //&& oldIndex >= 0 && oldIndex < GetChildren().Num() );

	focusIndex = index;

	if( oldIndex != focusIndex && !skipSound )
	{
		if( menuData != NULL )
		{
			menuData->PlaySound( GUI_SOUND_FOCUS );
		}
	}

	idSWFParmList parms;
	parms.Append( oldIndex );
	parms.Append( index );

	// need to mark the widget as having lost focus
	if( oldIndex != index && oldIndex >= 0 && oldIndex < GetChildren().Num() && GetChildByIndex( oldIndex ).GetState() != WIDGET_STATE_HIDDEN )
	{
		GetChildByIndex( oldIndex ).ReceiveEvent( idWidgetEvent( WIDGET_EVENT_FOCUS_OFF, 0, NULL, parms ) );
	}

	//assert( GetChildByIndex( index ).GetState() != WIDGET_STATE_HIDDEN );
	GetChildByIndex( index ).ReceiveEvent( idWidgetEvent( WIDGET_EVENT_FOCUS_ON, 0, NULL, parms ) );
}

/*
========================
idMenuWidget_Button::SetState

Transitioning from the current button state to the new button state
========================
*/
void idMenuWidget::SetState( const widgetState_t state )
{
	if( GetSprite() != NULL )
	{
		// FIXME: will need some more intelligence in the transitions to go from, say,
		// selected_up -> up ... but this should work fine for now.
		if( state == WIDGET_STATE_HIDDEN )
		{
			GetSprite()->SetVisible( false );
		}
		else
		{
			GetSprite()->SetVisible( true );
			if( state == WIDGET_STATE_DISABLED )
			{
				GetSprite()->PlayFrame( "disabled" );
			}
			else if( state == WIDGET_STATE_SELECTING )
			{
				if( widgetState == WIDGET_STATE_NORMAL )
				{
					GetSprite()->PlayFrame( "selecting" );	// transition from unselected to selected
				}
				else
				{
					GetSprite()->PlayFrame( "sel_up" );
				}
			}
			else if( state == WIDGET_STATE_SELECTED )
			{
				GetSprite()->PlayFrame( "sel_up" );
			}
			else if( state == WIDGET_STATE_NORMAL )
			{
				if( widgetState == WIDGET_STATE_SELECTING )
				{
					GetSprite()->PlayFrame( "unselecting" );	// transition from selected to unselected
				}
				else if( widgetState != WIDGET_STATE_HIDDEN && widgetState != WIDGET_STATE_NORMAL )
				{
					GetSprite()->PlayFrame( "out" );
				}
				else
				{
					GetSprite()->PlayFrame( "up" );
				}
			}
		}

		Update();
	}

	widgetState = state;
}

/*
========================
idMenuWidget::HandleAction
========================
*/
bool idMenuWidget::HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled )
{

	bool handled = false;
	if( GetParent() != NULL )
	{
		handled = GetParent()->HandleAction( action, event, widget );
	}
	else
	{

		if( forceHandled )
		{
			return false;
		}

		idMenuHandler* data = GetMenuData();
		if( data != NULL )
		{
			return data->HandleAction( action, event, widget, false );
		}
	}

	return handled;
}

/*
========================
idMenuWidget::GetEventActions
========================
*/
idList< idWidgetAction, TAG_IDLIB_LIST_MENU >* idMenuWidget::GetEventActions( const widgetEvent_t eventType )
{
	if( eventActionLookup[ eventType ] == INVALID_ACTION_INDEX )
	{
		return NULL;
	}
	return &eventActions[ eventActionLookup[ eventType ] ];
}

/*
========================
idMenuWidget::AddEventAction
========================
*/
idWidgetAction& idMenuWidget::AddEventAction( const widgetEvent_t eventType )
{
	if( eventActionLookup[ eventType ] == INVALID_ACTION_INDEX )
	{
		eventActionLookup[ eventType ] = eventActions.Num();
		eventActions.Alloc();
	}
	return eventActions[ eventActionLookup[ eventType ] ].Alloc();
}

/*
========================
idMenuWidget::ClearEventActions
========================
*/
void idMenuWidget::ClearEventActions()
{
	eventActions.Clear();
	eventActionLookup.Clear();
	eventActionLookup.SetNum( eventActionLookup.Max() );
	for( int i = 0; i < eventActionLookup.Num(); ++i )
	{
		eventActionLookup[ i ] = INVALID_ACTION_INDEX;
	}
}





