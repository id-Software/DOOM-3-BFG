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

static const int MAX_VIDEO_ITEMS = 5;

/*
========================
idMenuScreen_PDA_VideoDisks::Initialize
========================
*/
void idMenuScreen_PDA_VideoDisks::Initialize( idMenuHandler* data )
{

	if( data != NULL )
	{
		menuGUI = data->GetGUI();
	}
	SetSpritePath( "menuVideo" );


	videoDetails.SetSpritePath( GetSpritePath(), "info", "details" );
	videoDetails.Initialize( data );
	videoDetails.SetNoAutoFree( true );

	scrollbar.SetSpritePath( GetSpritePath(), "info", "videoList", "scrollbar" );
	scrollbar.Initialize( data );
	scrollbar.SetNoAutoFree( true );

	pdaVideoList.SetSpritePath( GetSpritePath(), "info", "videoList", "options" );
	pdaVideoList.SetNumVisibleOptions( MAX_VIDEO_ITEMS );
	pdaVideoList.SetWrappingAllowed( true );
	pdaVideoList.SetNoAutoFree( true );
	while( pdaVideoList.GetChildren().Num() < MAX_VIDEO_ITEMS )
	{
		idMenuWidget_Button* const buttonWidget = new( TAG_SWF ) idMenuWidget_Button();
		buttonWidget->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_SELECT_PDA_VIDEO, pdaVideoList.GetChildren().Num() );
		buttonWidget->RegisterEventObserver( &videoDetails );
		buttonWidget->RegisterEventObserver( &scrollbar );
		buttonWidget->Initialize( data );
		pdaVideoList.AddChild( buttonWidget );
	}
	pdaVideoList.Initialize( data );
	pdaVideoList.AddChild( &scrollbar );

	AddChild( &pdaVideoList );
	AddChild( &videoDetails );

	AddEventAction( WIDGET_EVENT_SCROLL_DOWN_LSTICK ).Set( new( TAG_SWF ) idWidgetActionHandler( &pdaVideoList, WIDGET_ACTION_EVENT_SCROLL_DOWN_START_REPEATER, WIDGET_EVENT_SCROLL_DOWN_LSTICK ) );
	AddEventAction( WIDGET_EVENT_SCROLL_UP_LSTICK ).Set( new( TAG_SWF ) idWidgetActionHandler( &pdaVideoList, WIDGET_ACTION_EVENT_SCROLL_UP_START_REPEATER, WIDGET_EVENT_SCROLL_UP_LSTICK ) );
	AddEventAction( WIDGET_EVENT_SCROLL_DOWN_LSTICK_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( &pdaVideoList, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_DOWN_LSTICK_RELEASE ) );
	AddEventAction( WIDGET_EVENT_SCROLL_UP_LSTICK_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( &pdaVideoList, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_UP_LSTICK_RELEASE ) );
	AddEventAction( WIDGET_EVENT_SCROLL_DOWN ).Set( new( TAG_SWF ) idWidgetActionHandler( &pdaVideoList, WIDGET_ACTION_EVENT_SCROLL_DOWN_START_REPEATER, WIDGET_EVENT_SCROLL_DOWN ) );
	AddEventAction( WIDGET_EVENT_SCROLL_UP ).Set( new( TAG_SWF ) idWidgetActionHandler( &pdaVideoList, WIDGET_ACTION_EVENT_SCROLL_UP_START_REPEATER, WIDGET_EVENT_SCROLL_UP ) );
	AddEventAction( WIDGET_EVENT_SCROLL_DOWN_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( &pdaVideoList, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_DOWN_RELEASE ) );
	AddEventAction( WIDGET_EVENT_SCROLL_UP_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( &pdaVideoList, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_UP_RELEASE ) );
	AddEventAction( WIDGET_EVENT_TAB_NEXT ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_TAB_NEXT, WIDGET_EVENT_TAB_NEXT ) );
	AddEventAction( WIDGET_EVENT_TAB_PREV ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_TAB_PREV, WIDGET_EVENT_TAB_PREV ) );

	idMenuScreen::Initialize( data );
}

/*
========================
idMenuScreen_PDA_VideoDisks::Update
========================
*/
void idMenuScreen_PDA_VideoDisks::Update()
{

	idPlayer* player = gameLocal.GetLocalPlayer();

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

			if( player != NULL && player->GetInventory().videos.Num() > 0 )
			{
				if( player->GetVideoMaterial() == NULL )
				{
					buttonInfo = cmdBar->GetButton( idMenuWidget_CommandBar::BUTTON_JOY1 );
					if( menuData->GetPlatform() != 2 )
					{
						buttonInfo->label = "#str_swf_play";
					}
					buttonInfo->action.Set( WIDGET_ACTION_PRESS_FOCUSED );
				}
				else
				{
					buttonInfo = cmdBar->GetButton( idMenuWidget_CommandBar::BUTTON_JOY1 );
					if( menuData->GetPlatform() != 2 )
					{
						buttonInfo->label = "#str_swf_stop";
					}
					buttonInfo->action.Set( WIDGET_ACTION_PRESS_FOCUSED );
				}
			}
		}
	}

	if( player != NULL )
	{
		//if ( pdaVideoList == NULL ) {
		//	idMenuScreen::Update();
		//	return;
		//}

		idSWFScriptObject& root = GetSWFObject()->GetRootObject();
		int index = pdaVideoList.GetViewIndex();
		const idDeclVideo* video = player->GetVideo( index );

		if( video == NULL )
		{
			idMenuScreen::Update();
			return;
		}

		if( player->GetVideoMaterial() != NULL )
		{
			// update video material
			if( BindSprite( root ) && GetSprite() != NULL )
			{
				idSWFSpriteInstance* videoSprite = GetSprite()->GetScriptObject()->GetNestedSprite( "info", "details", "video", "img" );
				const idMaterial* mat = player->GetVideoMaterial();

				if( videoSprite != NULL && mat != NULL )
				{
					videoSprite->SetMaterial( mat );
				}
			}
		}
		else
		{
			idSWFSpriteInstance* videoSprite = GetSprite()->GetScriptObject()->GetNestedSprite( "info", "details", "video", "img" );
			if( videoSprite != NULL )
			{
				videoSprite->SetMaterial( video->GetPreview() );
			}
		}
	}

	idMenuScreen::Update();
}

/*
========================
idMenuScreen_PDA_VideoDisks::ShowScreen
========================
*/
void idMenuScreen_PDA_VideoDisks::ShowScreen( const mainMenuTransition_t transitionType )
{

	videoItems.Clear();
	idPlayer* player = gameLocal.GetLocalPlayer();
	if( player != NULL )
	{
		int numVideos = player->GetInventory().videos.Num();
		for( int i = 0; i < numVideos; ++i )
		{
			const idDeclVideo* video = player->GetVideo( i );
			if( video != NULL )
			{
				idList< idStr > item;
				item.Append( video->GetVideoName() );
				videoItems.Append( item );
			}
		}
	}

	pdaVideoList.SetListData( videoItems );

	idMenuScreen::ShowScreen( transitionType );
}

/*
========================
idMenuScreen_PDA_VideoDisks::ToggleVideoDiskPlay
========================
*/
void idMenuScreen_PDA_VideoDisks::ToggleVideoDiskPlay()
{

	idPlayer* player = gameLocal.GetLocalPlayer();
	if( player == NULL )
	{
		return;
	}


	int index = pdaVideoList.GetViewIndex();
	const idDeclVideo* video = player->GetVideo( index );

	if( video == NULL )
	{
		return;
	}

	if( video == activeVideo )
	{
		player->EndVideoDisk();
		activeVideo = NULL;
		return;
	}

	activeVideo = video;

	if( player->GetVideoMaterial() == NULL )
	{
		player->PlayVideoDisk( video );
	}
	else
	{
		player->EndVideoDisk();
	}
}

/*
========================
idMenuScreen_PDA_VideoDisks::SelectedVideoToPlay
========================
*/
void idMenuScreen_PDA_VideoDisks::SelectedVideoToPlay( int index )
{

	idPlayer* player = gameLocal.GetLocalPlayer();
	if( player == NULL )
	{
		return;
	}

	player->EndVideoDisk();
	if( menuData != NULL )
	{
		idMenuHandler_PDA* pdaHandler = dynamic_cast< idMenuHandler_PDA* const >( menuData );
		pdaHandler->ClearVideoPlaying();
	}


	pdaVideoList.SetViewIndex( pdaVideoList.GetViewOffset() + index );
	pdaVideoList.SetFocusIndex( index );
	const idDeclVideo* video = player->GetVideo( pdaVideoList.GetViewOffset() + index );

	if( video == NULL )
	{
		return;
	}

	if( video == activeVideo )
	{
		activeVideo = NULL;
		return;
	}

	activeVideo = video;

	player->PlayVideoDisk( video );


}

/*
========================
idMenuScreen_PDA_VideoDisks::HideScreen
========================
*/
void idMenuScreen_PDA_VideoDisks::HideScreen( const mainMenuTransition_t transitionType )
{
	idMenuScreen::HideScreen( transitionType );
}

/*
========================
idMenuScreen_PDA_VideoDisks::HandleAction
========================
*/
bool idMenuScreen_PDA_VideoDisks::HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled )
{

	if( menuData == NULL )
	{
		return true;
	}

	if( menuData->ActiveScreen() != PDA_AREA_VIDEO_DISKS )
	{
		return false;
	}

	widgetAction_t actionType = action.GetType();
	const idSWFParmList& parms = action.GetParms();

	switch( actionType )
	{
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
		case WIDGET_ACTION_PRESS_FOCUSED:
		{
			ToggleVideoDiskPlay();
			Update();
			return true;
		}
	}

	return idMenuWidget::HandleAction( action, event, widget, forceHandled );
}
