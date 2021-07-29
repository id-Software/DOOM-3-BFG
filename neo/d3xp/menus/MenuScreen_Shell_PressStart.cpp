/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2012 Robert Beckebans

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
#include "../../framework/Common_local.h"

static const int NUM_GAME_SELECTIONS_VISIBLE = 5;
extern idCVar g_demoMode;

namespace
{
/*
================================================
UICmd_RegisterUser
================================================
*/
class UICmd_RegisterUser : public idSWFScriptFunction_RefCounted
{
public:
	idSWFScriptVar Call( idSWFScriptObject* thisObject, const idSWFParmList& parms )
	{
		if( parms.Num() != 1 )
		{
			idLib::Warning( "No device specified when registering mouse user" );
			return idSWFScriptVar();
		}

		const int device = parms[ 0 ].ToInteger();
		session->GetSignInManager().RegisterLocalUser( device );
		return idSWFScriptVar();
	}
};
}

/*
========================
idMenuScreen_Shell_PressStart::Initialize
========================
*/
void idMenuScreen_Shell_PressStart::Initialize( idMenuHandler* data )
{
	idMenuScreen::Initialize( data );

	if( data != NULL )
	{
		menuGUI = data->GetGUI();
	}

	SetSpritePath( "menuStart" );

	itemList = new( TAG_SWF ) idMenuWidget_Carousel();
	itemList->SetSpritePath( GetSpritePath(), "info", "options" );
	itemList->SetNumVisibleOptions( NUM_GAME_SELECTIONS_VISIBLE );
	while( itemList->GetChildren().Num() < NUM_GAME_SELECTIONS_VISIBLE )
	{
		idMenuWidget_Button* const buttonWidget = new( TAG_SWF ) idMenuWidget_Button();
		buttonWidget->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_PRESS_FOCUSED, itemList->GetChildren().Num() );
		buttonWidget->Initialize( data );
		itemList->AddChild( buttonWidget );
	}
	itemList->Initialize( data );

	AddChild( itemList );

	AddEventAction( WIDGET_EVENT_SCROLL_LEFT ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_SCROLL_LEFT_START_REPEATER, WIDGET_EVENT_SCROLL_LEFT ) );
	AddEventAction( WIDGET_EVENT_SCROLL_RIGHT ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_SCROLL_RIGHT_START_REPEATER, WIDGET_EVENT_SCROLL_RIGHT ) );
	AddEventAction( WIDGET_EVENT_SCROLL_LEFT_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_LEFT_RELEASE ) );
	AddEventAction( WIDGET_EVENT_SCROLL_RIGHT_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_RIGHT_RELEASE ) );

	AddEventAction( WIDGET_EVENT_SCROLL_UP ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_SCROLL_UP_START_REPEATER, WIDGET_EVENT_SCROLL_UP ) );
	AddEventAction( WIDGET_EVENT_SCROLL_DOWN ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_SCROLL_DOWN_START_REPEATER, WIDGET_EVENT_SCROLL_DOWN ) );
	AddEventAction( WIDGET_EVENT_SCROLL_UP_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_UP_RELEASE ) );
	AddEventAction( WIDGET_EVENT_SCROLL_DOWN_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_DOWN_RELEASE ) );

	AddEventAction( WIDGET_EVENT_SCROLL_LEFT_LSTICK ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_SCROLL_LEFT_START_REPEATER, WIDGET_EVENT_SCROLL_LEFT_LSTICK ) );
	AddEventAction( WIDGET_EVENT_SCROLL_RIGHT_LSTICK ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_SCROLL_RIGHT_START_REPEATER, WIDGET_EVENT_SCROLL_RIGHT_LSTICK ) );
	AddEventAction( WIDGET_EVENT_SCROLL_LEFT_LSTICK_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_LEFT_LSTICK_RELEASE ) );
	AddEventAction( WIDGET_EVENT_SCROLL_RIGHT_LSTICK_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_RIGHT_LSTICK_RELEASE ) );

	doomCover = declManager->FindMaterial( "guis/assets/mainmenu/doom_cover.tga" );
	doom2Cover = declManager->FindMaterial( "guis/assets/mainmenu/doom2_cover.tga" );
	doom3Cover = declManager->FindMaterial( "guis/assets/mainmenu/doom3_cover.tga" );

	startButton = new idMenuWidget_Button();
	startButton->SetSpritePath( GetSpritePath(), "info", "btnStart" );
	AddChild( startButton );
}

/*
========================
idMenuScreen_Shell_Root::Update
========================
*/
void idMenuScreen_Shell_PressStart::Update()
{

	if( !g_demoMode.GetBool() )
	{
		if( menuData != NULL )
		{
			idMenuWidget_CommandBar* cmdBar = menuData->GetCmdBar();
			if( cmdBar != NULL )
			{
				cmdBar->ClearAllButtons();
				idMenuWidget_CommandBar::buttonInfo_t* buttonInfo;
				buttonInfo = cmdBar->GetButton( idMenuWidget_CommandBar::BUTTON_JOY1 );
				if( menuData->GetPlatform() != 2 )
				{
					buttonInfo->label = "#str_SWF_SELECT";
				}
				buttonInfo->action.Set( WIDGET_ACTION_PRESS_FOCUSED );
			}
		}
	}

	idMenuScreen::Update();
}

/*
========================
idMenuScreen_Shell_PressStart::ShowScreen
========================
*/
void idMenuScreen_Shell_PressStart::ShowScreen( const mainMenuTransition_t transitionType )
{

	idSWFScriptObject& root = GetSWFObject()->GetRootObject();
	if( BindSprite( root ) )
	{
		if( g_demoMode.GetBool() )
		{

			idList<const idMaterial*> coverIcons;
			if( itemList != NULL )
			{
				itemList->SetListImages( coverIcons );
			}

			if( startButton != NULL )
			{
				startButton->BindSprite( root );
				startButton->SetLabel( idLocalization::GetString( "#str_swf_press_start" ) );
			}

			idSWFSpriteInstance* backing = GetSprite()->GetScriptObject()->GetNestedSprite( "backing" );
			if( backing != NULL )
			{
				backing->SetVisible( false );
			}

		}
		else
		{

			idList<const idMaterial*> coverIcons;

			coverIcons.Append( doomCover );
			coverIcons.Append( doom3Cover );
			coverIcons.Append( doom2Cover );

			if( itemList != NULL )
			{
				itemList->SetListImages( coverIcons );
				itemList->SetFocusIndex( 1, true );
				itemList->SetViewIndex( 1 );
				itemList->SetMoveToIndex( 1 );
			}

			if( startButton != NULL )
			{
				startButton->BindSprite( root );
				startButton->SetLabel( "" );
			}

			idSWFSpriteInstance* backing = GetSprite()->GetScriptObject()->GetNestedSprite( "backing" );
			if( backing != NULL )
			{
				backing->SetVisible( true );
			}

		}
	}

	idMenuScreen::ShowScreen( transitionType );
}

/*
========================
idMenuScreen_Shell_PressStart::HideScreen
========================
*/
void idMenuScreen_Shell_PressStart::HideScreen( const mainMenuTransition_t transitionType )
{
	idMenuScreen::HideScreen( transitionType );
}

/*
========================
idMenuScreen_Shell_PressStart::HandleAction
========================
*/
bool idMenuScreen_Shell_PressStart::HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled )
{

	if( menuData == NULL )
	{
		return true;
	}

	if( menuData->ActiveScreen() != SHELL_AREA_START )
	{
		return false;
	}

	widgetAction_t actionType = action.GetType();
	const idSWFParmList& parms = action.GetParms();

	switch( actionType )
	{
		case WIDGET_ACTION_PRESS_FOCUSED:
		{
			if( itemList == NULL )
			{
				return true;
			}

			if( event.parms.Num() != 1 )
			{
				return true;
			}

			if( itemList->GetMoveToIndex() != itemList->GetViewIndex() )
			{
				return true;
			}

			if( parms.Num() > 0 )
			{
				const int index = parms[0].ToInteger();
				if( index != 0 )
				{
					itemList->MoveToIndex( index );
					Update();
				}
			}

			// RB begin
#if defined(USE_DOOMCLASSIC)
			if( itemList->GetMoveToIndex() == 0 )
			{
				common->SwitchToGame( DOOM_CLASSIC );
			}
			else
#endif
				if( itemList->GetMoveToIndex() == 1 )
				{
					if( session->GetSignInManager().GetMasterLocalUser() == NULL )
					{
						const int device = event.parms[ 0 ].ToInteger();
						session->GetSignInManager().RegisterLocalUser( device );
					}
					else
					{
						menuData->SetNextScreen( SHELL_AREA_ROOT, MENU_TRANSITION_SIMPLE );
					}
				}
#if defined(USE_DOOMCLASSIC)
				else if( itemList->GetMoveToIndex() == 2 )
				{
					common->SwitchToGame( DOOM2_CLASSIC );
				}
#endif
			// RB end

			return true;
		}
		case WIDGET_ACTION_START_REPEATER:
		{
			idWidgetAction repeatAction;
			widgetAction_t repeatActionType = static_cast< widgetAction_t >( parms[ 0 ].ToInteger() );
			assert( parms.Num() == 2 );
			repeatAction.Set( repeatActionType, parms[ 1 ] );
			menuData->StartWidgetActionRepeater( widget, repeatAction, event );
			return true;
		}
		case WIDGET_ACTION_STOP_REPEATER:
		{
			menuData->ClearWidgetActionRepeater();
			return true;
		}
		case WIDGET_ACTION_SCROLL_HORIZONTAL:
		{

			if( itemList == NULL )
			{
				return true;
			}

			if( itemList->GetTotalNumberOfOptions() <= 1 )
			{
				return true;
			}

			idLib::Printf( "scroll \n" );

			if( itemList->GetMoveDiff() > 0 )
			{
				itemList->MoveToIndex( itemList->GetMoveToIndex(), true );
			}

			int direction = parms[0].ToInteger();
			if( direction == 1 )
			{
				if( itemList->GetViewIndex() == itemList->GetTotalNumberOfOptions() - 1 )
				{
					return true;
				}
				else
				{
					itemList->MoveToIndex( 1 );
				}
			}
			else
			{
				if( itemList->GetViewIndex() == 0 )
				{
					return true;
				}
				else
				{
					itemList->MoveToIndex( ( itemList->GetNumVisibleOptions() / 2 ) + 1 );
				}
			}

			return true;
		}
	}

	return idMenuWidget::HandleAction( action, event, widget, forceHandled );
}
