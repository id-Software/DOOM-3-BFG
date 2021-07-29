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

idMenuScreen::idMenuScreen()
{
	menuGUI = NULL;
	transition = MENU_TRANSITION_INVALID;
}

idMenuScreen::~idMenuScreen()
{

}

/*
========================
idMenuScreen::ObserveEvent
========================
*/
void idMenuScreen::ObserveEvent( const idMenuWidget& widget, const idWidgetEvent& event )
{
	if( event.type == WIDGET_EVENT_COMMAND )
	{
		switch( event.arg )
		{
			case idMenuWidget_CommandBar::BUTTON_JOY1:
			{
				ReceiveEvent( idWidgetEvent( WIDGET_EVENT_PRESS, 0, event.thisObject, event.parms ) );
				break;
			}
			case idMenuWidget_CommandBar::BUTTON_JOY2:
			{
				ReceiveEvent( idWidgetEvent( WIDGET_EVENT_BACK, 0, event.thisObject, event.parms ) );
				break;
			}
		}
	}
}

/*
========================
idMenuScreen::Update
========================
*/
void idMenuScreen::Update()
{

	if( menuGUI == NULL )
	{
		return;
	}

	//
	// Display
	//
	for( int childIndex = 0; childIndex < GetChildren().Num(); ++childIndex )
	{
		GetChildByIndex( childIndex ).Update();
	}

	if( menuData != NULL )
	{
		menuData->UpdateChildren();
	}
}

/*
========================
idMenuScreen::UpdateCmds
========================
*/
void idMenuScreen::UpdateCmds()
{
	idSWF* const gui = menuGUI;

	idSWFScriptObject* const shortcutKeys = gui->GetGlobal( "shortcutKeys" ).GetObject();
	if( !verify( shortcutKeys != NULL ) )
	{
		return;
	}

	idSWFScriptVar clearFunc = shortcutKeys->Get( "clear" );
	if( clearFunc.IsFunction() )
	{
		clearFunc.GetFunction()->Call( NULL, idSWFParmList() );
	}

	// NAVIGATION: UP/DOWN, etc.
	idSWFScriptObject* const buttons = gui->GetRootObject().GetObject( "buttons" );
	if( buttons != NULL )
	{

		idSWFScriptObject* const btnUp = buttons->GetObject( "btnUp" );
		if( btnUp != NULL )
		{
			btnUp->Set( "onPress", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_SCROLL_UP, SCROLL_SINGLE ) );
			btnUp->Set( "onRelease", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_SCROLL_UP_RELEASE, 0 ) );
			shortcutKeys->Set( "UP", btnUp );
			shortcutKeys->Set( "MWHEEL_UP", btnUp );
		}

		idSWFScriptObject* const btnDown = buttons->GetObject( "btnDown" );
		if( btnDown != NULL )
		{
			btnDown->Set( "onPress", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_SCROLL_DOWN, SCROLL_SINGLE ) );
			btnDown->Set( "onRelease", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_SCROLL_DOWN_RELEASE, 0 ) );
			shortcutKeys->Set( "DOWN", btnDown );
			shortcutKeys->Set( "MWHEEL_DOWN", btnDown );
		}

		idSWFScriptObject* const btnUp_LStick = buttons->GetObject( "btnUp_LStick" );
		if( btnUp_LStick != NULL )
		{
			btnUp_LStick->Set( "onPress", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_SCROLL_UP_LSTICK, SCROLL_SINGLE ) );
			btnUp_LStick->Set( "onRelease", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_SCROLL_UP_LSTICK_RELEASE, 0 ) );
			shortcutKeys->Set( "STICK1_UP", btnUp_LStick );
		}

		idSWFScriptObject* const btnDown_LStick = buttons->GetObject( "btnDown_LStick" );
		if( btnDown_LStick != NULL )
		{
			btnDown_LStick->Set( "onPress", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_SCROLL_DOWN_LSTICK, SCROLL_SINGLE ) );
			btnDown_LStick->Set( "onRelease", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_SCROLL_DOWN_LSTICK_RELEASE, 0 ) );
			shortcutKeys->Set( "STICK1_DOWN", btnDown_LStick );
		}

		idSWFScriptObject* const btnUp_RStick = buttons->GetObject( "btnUp_RStick" );
		if( btnUp_RStick != NULL )
		{
			btnUp_RStick->Set( "onPress", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_SCROLL_UP_RSTICK, SCROLL_SINGLE ) );
			btnUp_RStick->Set( "onRelease", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_SCROLL_UP_RSTICK_RELEASE, 0 ) );
			shortcutKeys->Set( "STICK2_UP", btnUp_RStick );
		}

		idSWFScriptObject* const btnDown_RStick = buttons->GetObject( "btnDown_RStick" );
		if( btnDown_RStick != NULL )
		{
			btnDown_RStick->Set( "onPress", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_SCROLL_DOWN_RSTICK, SCROLL_SINGLE ) );
			btnDown_RStick->Set( "onRelease", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_SCROLL_DOWN_RSTICK_RELEASE, 0 ) );
			shortcutKeys->Set( "STICK2_DOWN", btnDown_RStick );
		}

		idSWFScriptObject* const btnPageUp = buttons->GetObject( "btnPageUp" );
		if( btnPageUp != NULL )
		{
			btnPageUp->Set( "onPress", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_SCROLL_PAGEUP, SCROLL_PAGE ) );
			btnPageUp->Set( "onRelease", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_SCROLL_PAGEUP_RELEASE, 0 ) );
			shortcutKeys->Set( "PGUP", btnPageUp );
		}

		idSWFScriptObject* const btnPageDown = buttons->GetObject( "btnPageDown" );
		if( btnPageDown != NULL )
		{
			btnPageDown->Set( "onPress", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_SCROLL_PAGEDWN, SCROLL_PAGE ) );
			btnPageDown->Set( "onRelease", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_SCROLL_PAGEDWN_RELEASE, 0 ) );
			shortcutKeys->Set( "PGDN", btnPageDown );
		}

		idSWFScriptObject* const btnHome = buttons->GetObject( "btnHome" );
		if( btnHome != NULL )
		{
			btnHome->Set( "onPress", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_SCROLL_UP, SCROLL_FULL ) );
			btnHome->Set( "onRelease", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_SCROLL_UP_RELEASE, 0 ) );
			shortcutKeys->Set( "HOME", btnHome );
		}

		idSWFScriptObject* const btnEnd = buttons->GetObject( "btnEnd" );
		if( btnEnd != NULL )
		{
			btnEnd->Set( "onPress", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_SCROLL_DOWN, SCROLL_FULL ) );
			btnEnd->Set( "onRelease", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_SCROLL_DOWN_RELEASE, 0 ) );
			shortcutKeys->Set( "END", btnEnd );
		}

		idSWFScriptObject* const btnLeft = buttons->GetObject( "btnLeft" );
		if( btnLeft != NULL )
		{
			btnLeft->Set( "onPress", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_SCROLL_LEFT, 0 ) );
			btnLeft->Set( "onRelease", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_SCROLL_LEFT_RELEASE, 0 ) );
			shortcutKeys->Set( "LEFT", btnLeft );
		}

		idSWFScriptObject* const btnRight = buttons->GetObject( "btnRight" );
		if( btnRight != NULL )
		{
			btnRight->Set( "onPress", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_SCROLL_RIGHT, 0 ) );
			btnRight->Set( "onRelease", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_SCROLL_RIGHT_RELEASE, 0 ) );
			shortcutKeys->Set( "RIGHT", btnRight );
		}

		idSWFScriptObject* const btnLeft_LStick = buttons->GetObject( "btnLeft_LStick" );
		if( btnLeft_LStick != NULL )
		{
			btnLeft_LStick->Set( "onPress", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_SCROLL_LEFT_LSTICK, 0 ) );
			btnLeft_LStick->Set( "onRelease", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_SCROLL_LEFT_LSTICK_RELEASE, 0 ) );
			shortcutKeys->Set( "STICK1_LEFT", btnLeft_LStick );
		}

		idSWFScriptObject* const btnRight_LStick = buttons->GetObject( "btnRight_LStick" );
		if( btnRight_LStick != NULL )
		{
			btnRight_LStick->Set( "onPress", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_SCROLL_RIGHT_LSTICK, 0 ) );
			btnRight_LStick->Set( "onRelease", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_SCROLL_RIGHT_LSTICK_RELEASE, 0 ) );
			shortcutKeys->Set( "STICK1_RIGHT", btnRight_LStick );
		}

		idSWFScriptObject* const btnLeft_RStick = buttons->GetObject( "btnLeft_RStick" );
		if( btnLeft_RStick != NULL )
		{
			btnLeft_RStick->Set( "onPress", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_SCROLL_LEFT_RSTICK, 0 ) );
			btnLeft_RStick->Set( "onRelease", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_SCROLL_LEFT_RSTICK_RELEASE, 0 ) );
			shortcutKeys->Set( "STICK2_LEFT", btnLeft_RStick );
		}

		idSWFScriptObject* const btnRight_RStick = buttons->GetObject( "btnRight_RStick" );
		if( btnRight_RStick != NULL )
		{
			btnRight_RStick->Set( "onPress", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_SCROLL_RIGHT_RSTICK, 0 ) );
			btnRight_RStick->Set( "onRelease", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_SCROLL_RIGHT_RSTICK_RELEASE, 0 ) );
			shortcutKeys->Set( "STICK2_RIGHT", btnRight_RStick );
		}
	}

	idSWFScriptObject* const navigation = gui->GetRootObject().GetObject( "navBar" );
	if( navigation != NULL )
	{
		// TAB NEXT
		idSWFScriptObject* const btnTabNext = navigation->GetNestedObj( "options", "btnTabNext" );
		if( btnTabNext != NULL )
		{
			btnTabNext->Set( "onPress", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_TAB_NEXT, 0 ) );
			shortcutKeys->Set( "JOY6", btnTabNext );

			if( btnTabNext->GetSprite() != NULL && menuData != NULL )
			{
				btnTabNext->GetSprite()->StopFrame( menuData->GetPlatform() + 1 );
			}

		}

		// TAB PREV
		idSWFScriptObject* const btnTabPrev = navigation->GetNestedObj( "options", "btnTabPrev" );
		if( btnTabPrev != NULL )
		{
			btnTabPrev->Set( "onPress", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_TAB_PREV, 0 ) );
			shortcutKeys->Set( "JOY5", btnTabPrev );

			if( btnTabPrev->GetSprite() != NULL && menuData != NULL )
			{
				btnTabPrev->GetSprite()->StopFrame( menuData->GetPlatform() + 1 );
			}
		}
	}
}

/*
========================
idMenuScreen::HideScreen
========================
*/
void idMenuScreen::HideScreen( const mainMenuTransition_t transitionType )
{

	if( menuGUI == NULL )
	{
		return;
	}

	if( !BindSprite( menuGUI->GetRootObject() ) )
	{
		return;
	}

	if( transitionType == MENU_TRANSITION_SIMPLE )
	{
		GetSprite()->PlayFrame( "rollOff" );
	}
	else if( transitionType == MENU_TRANSITION_ADVANCE )
	{
		GetSprite()->PlayFrame( "rollOffBack" );
	}
	else
	{
		GetSprite()->PlayFrame( "rollOffFront" );
	}

	Update();

}

/*
========================
idMenuScreen::ShowScreen
========================
*/
void idMenuScreen::ShowScreen( const mainMenuTransition_t transitionType )
{
	if( menuGUI == NULL )
	{
		return;
	}

	if( !BindSprite( menuGUI->GetRootObject() ) )
	{
		return;
	}

	GetSprite()->SetVisible( true );
	if( transitionType == MENU_TRANSITION_SIMPLE )
	{
		if( menuData != NULL && menuData->ActiveScreen() != -1 )
		{
			menuData->PlaySound( GUI_SOUND_BUILD_ON );
		}
		GetSprite()->PlayFrame( "rollOn" );
	}
	else if( transitionType == MENU_TRANSITION_ADVANCE )
	{
		if( menuData != NULL && menuData->ActiveScreen() != -1 )
		{
			menuData->PlaySound( GUI_SOUND_BUILD_ON );
		}
		GetSprite()->PlayFrame( "rollOnFront" );
	}
	else
	{
		if( menuData != NULL )
		{
			menuData->PlaySound( GUI_SOUND_BUILD_OFF );
		}
		GetSprite()->PlayFrame( "rollOnBack" );
	}

	Update();

	SetFocusIndex( GetFocusIndex(), true );
}

/*
========================
idMenuScreen::HandleMenu

NOTE: This is holdover from the way the menu system was setup before.  It should be able to
be removed when the old way is fully replaced, and instead events will just be sent directly
to the screen.
========================
*/
void idMenuScreen::HandleMenu( const mainMenuTransition_t type )
{
	if( type == MENU_TRANSITION_ADVANCE )
	{
		ReceiveEvent( idWidgetEvent( WIDGET_EVENT_PRESS, 0, NULL, idSWFParmList() ) );
	}
	else if( type == MENU_TRANSITION_BACK )
	{
		ReceiveEvent( idWidgetEvent( WIDGET_EVENT_BACK, 0, NULL, idSWFParmList() ) );
	}

	transition = type;
}

