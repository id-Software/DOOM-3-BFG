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
idMenuScreen_PDA_UserData::Initialize
========================
*/
void idMenuScreen_PDA_UserData::Initialize( idMenuHandler* data )
{
	idMenuScreen::Initialize( data );

	if( data != NULL )
	{
		menuGUI = data->GetGUI();
	}
	SetSpritePath( "menuData" );

	pdaUserData.SetSpritePath( GetSpritePath(), "info", "pdaData" );
	pdaUserData.Initialize( data );
	pdaUserData.SetNoAutoFree( true );

	AddChild( &pdaUserData );

	pdaObjectiveSimple.SetSpritePath( GetSpritePath(), "info", "missionInfo" );
	pdaObjectiveSimple.Initialize( data );
	pdaObjectiveSimple.SetNoAutoFree( true );

	AddChild( &pdaObjectiveSimple );

	pdaAudioFiles.SetSpritePath( GetSpritePath(), "info", "audioFiles" );
	pdaAudioFiles.Initialize( data );
	pdaAudioFiles.SetNoAutoFree( true );

	AddChild( &pdaAudioFiles );

	AddEventAction( WIDGET_EVENT_SCROLL_DOWN_RSTICK ).Set( new( TAG_SWF ) idWidgetActionHandler( &pdaAudioFiles.GetChildByIndex( 0 ), WIDGET_ACTION_EVENT_SCROLL_DOWN_START_REPEATER, WIDGET_EVENT_SCROLL_DOWN_RSTICK ) );
	AddEventAction( WIDGET_EVENT_SCROLL_UP_RSTICK ).Set( new( TAG_SWF ) idWidgetActionHandler( &pdaAudioFiles.GetChildByIndex( 0 ), WIDGET_ACTION_EVENT_SCROLL_UP_START_REPEATER, WIDGET_EVENT_SCROLL_UP_RSTICK ) );
	AddEventAction( WIDGET_EVENT_SCROLL_DOWN_RSTICK_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( &pdaAudioFiles.GetChildByIndex( 0 ), WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_DOWN_RSTICK_RELEASE ) );
	AddEventAction( WIDGET_EVENT_SCROLL_UP_RSTICK_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( &pdaAudioFiles.GetChildByIndex( 0 ), WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_UP_RSTICK_RELEASE ) );

	AddEventAction( WIDGET_EVENT_TAB_NEXT ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_TAB_NEXT, WIDGET_EVENT_TAB_NEXT ) );
	AddEventAction( WIDGET_EVENT_TAB_PREV ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_TAB_PREV, WIDGET_EVENT_TAB_PREV ) );

}

/*
========================
idMenuScreen_PDA_UserData::Update
========================
*/
void idMenuScreen_PDA_UserData::Update()
{

	if( menuData != NULL )
	{
		idMenuWidget_CommandBar* cmdBar = dynamic_cast< idMenuWidget_CommandBar* const >( menuData->GetChildFromIndex( PDA_WIDGET_CMD_BAR ) );
		if( cmdBar != NULL )
		{
			cmdBar->ClearAllButtons();
			idMenuWidget_CommandBar::buttonInfo_t* buttonInfo;

			buttonInfo = cmdBar->GetButton( idMenuWidget_CommandBar::BUTTON_JOY2 );
			if( menuData->GetPlatform() != 2 )
			{
				buttonInfo->label = "#str_01345";
			}
			buttonInfo->action.Set( WIDGET_ACTION_GO_BACK );


			buttonInfo = cmdBar->GetButton( idMenuWidget_CommandBar::BUTTON_TAB );
			buttonInfo->label = "";
			buttonInfo->action.Set( WIDGET_ACTION_GO_BACK );

			idPlayer* player = gameLocal.GetLocalPlayer();
			idMenuWidget_DynamicList* pdaList = dynamic_cast< idMenuWidget_DynamicList* >( menuData->GetChildFromIndex( PDA_WIDGET_PDA_LIST ) );
			if( pdaList != NULL && player != NULL )
			{
				int pdaIndex = pdaList->GetViewIndex();
				if( pdaIndex < player->GetInventory().pdas.Num() )
				{
					const idDeclPDA* pda = player->GetInventory().pdas[ pdaIndex ];
					if( pda != NULL && pdaIndex != 0 )
					{
						if( player->IsSoundChannelPlaying( SND_CHANNEL_PDA_AUDIO ) )
						{
							buttonInfo = cmdBar->GetButton( idMenuWidget_CommandBar::BUTTON_JOY1 );
							if( menuData->GetPlatform() != 2 )
							{
								buttonInfo->label = "#str_swf_stop";
							}
							buttonInfo->action.Set( WIDGET_ACTION_PRESS_FOCUSED );
						}
						else if( pda->GetNumAudios() > 0 )
						{
							buttonInfo = cmdBar->GetButton( idMenuWidget_CommandBar::BUTTON_JOY1 );
							if( menuData->GetPlatform() != 2 )
							{
								buttonInfo->label = "#str_swf_play";
							}
							buttonInfo->action.Set( WIDGET_ACTION_PRESS_FOCUSED );
						}
					}
				}
			}

		}
	}

	idMenuScreen::Update();
}

/*
========================
idMenuScreen_PDA_UserData::ShowScreen
========================
*/
void idMenuScreen_PDA_UserData::ShowScreen( const mainMenuTransition_t transitionType )
{

	if( menuGUI != NULL && menuData != NULL )
	{
		idSWFScriptObject& root = menuGUI->GetRootObject();
		idSWFSpriteInstance* pdaSprite = root.GetNestedSprite( "pda_persons" );
		if( pdaSprite != NULL && menuData != NULL && menuData->ActiveScreen() != PDA_AREA_USER_EMAIL )
		{
			pdaSprite->SetVisible( true );
			pdaSprite->PlayFrame( "rollOn" );
		}

		idSWFSpriteInstance* navBar = root.GetNestedSprite( "navBar" );
		if( navBar != NULL && menuData != NULL && menuData->ActiveScreen() == PDA_AREA_INVALID )
		{
			navBar->PlayFrame( "rollOn" );
		}
	}

	idMenuScreen::ShowScreen( transitionType );

	if( menuData != NULL )
	{
		idMenuWidget_DynamicList* pdaList = dynamic_cast< idMenuWidget_DynamicList* >( menuData->GetChildFromIndex( PDA_WIDGET_PDA_LIST ) );
		if( pdaList != NULL )
		{
			pdaList->SetFocusIndex( pdaList->GetFocusIndex() );
		}
	}
}

/*
========================
idMenuScreen_PDA_UserData::HideScreen
========================
*/
void idMenuScreen_PDA_UserData::HideScreen( const mainMenuTransition_t transitionType )
{

	if( menuGUI != NULL )
	{
		idSWFScriptObject& root = menuGUI->GetRootObject();
		idSWFSpriteInstance* pdaSprite = root.GetNestedSprite( "pda_persons" );
		if( pdaSprite != NULL && menuData != NULL && menuData->NextScreen() != PDA_AREA_USER_EMAIL )
		{
			pdaSprite->SetVisible( true );
			pdaSprite->PlayFrame( "rollOff" );
		}
	}

	idMenuScreen::HideScreen( transitionType );
}

/*
========================
idMenuScreen_PDA_UserData::HandleAction
========================
*/
bool idMenuScreen_PDA_UserData::HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled )
{

	if( menuData == NULL )
	{
		return true;
	}

	if( menuData->ActiveScreen() != PDA_AREA_USER_DATA )
	{
		return false;
	}

	widgetAction_t actionType = action.GetType();
	const idSWFParmList& parms = action.GetParms();

	switch( actionType )
	{
		case WIDGET_ACTION_PRESS_FOCUSED:
		{

			idMenuWidget_DynamicList* pdaList = dynamic_cast< idMenuWidget_DynamicList* >( menuData->GetChildFromIndex( PDA_WIDGET_PDA_LIST ) );
			if( pdaList == NULL )
			{
				return true;
			}

			int pdaIndex = pdaList->GetViewIndex();
			if( pdaIndex == 0 )
			{
				return true;
			}

			idPlayer* player = gameLocal.GetLocalPlayer();
			if( player != NULL && player->IsSoundChannelPlaying( SND_CHANNEL_PDA_AUDIO ) )
			{
				player->EndAudioLog();
			}
			else
			{
				if( menuData != NULL && pdaAudioFiles.GetChildren().Num() > 0 )
				{
					int index = pdaAudioFiles.GetChildByIndex( 0 ).GetFocusIndex();
					idMenuHandler_PDA* pdaHandler = dynamic_cast< idMenuHandler_PDA* const >( menuData );
					if( pdaHandler != NULL )
					{
						pdaHandler->PlayPDAAudioLog( pdaIndex, index );
					}
				}
			}
			return true;
		}
		case WIDGET_ACTION_GO_BACK:
		{
			menuData->SetNextScreen( PDA_AREA_INVALID, MENU_TRANSITION_ADVANCE );
			return true;
		}
		case WIDGET_ACTION_START_REPEATER:
		{
			idWidgetAction repeatAction;
			widgetAction_t repeatActionType = static_cast< widgetAction_t >( parms[ 0 ].ToInteger() );
			assert( parms.Num() == 2 );
			repeatAction.Set( repeatActionType, parms[ 1 ] );
			if( menuData != NULL )
			{
				menuData->StartWidgetActionRepeater( widget, repeatAction, event );
			}
			return true;
		}
		case WIDGET_ACTION_STOP_REPEATER:
		{
			if( menuData != NULL )
			{
				menuData->ClearWidgetActionRepeater();
			}
			return true;
		}
	}

	return idMenuWidget::HandleAction( action, event, widget, forceHandled );
}
