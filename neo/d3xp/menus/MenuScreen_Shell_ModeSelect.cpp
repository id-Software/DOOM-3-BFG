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

const static int NUM_SETTING_OPTIONS = 8;

/*
========================
idMenuScreen_Shell_ModeSelect::Initialize
========================
*/
void idMenuScreen_Shell_ModeSelect::Initialize( idMenuHandler* data )
{
	idMenuScreen::Initialize( data );

	if( data != NULL )
	{
		menuGUI = data->GetGUI();
	}

	SetSpritePath( "menuModeSelect" );

	options = new( TAG_SWF ) idMenuWidget_DynamicList();
	options->SetNumVisibleOptions( NUM_SETTING_OPTIONS );
	options->SetSpritePath( GetSpritePath(), "info", "options" );
	options->SetWrappingAllowed( true );
	AddChild( options );

	idMenuWidget_Help* const helpWidget = new( TAG_SWF ) idMenuWidget_Help();
	helpWidget->SetSpritePath( GetSpritePath(), "info", "helpTooltip" );
	AddChild( helpWidget );

	const idStrList& modes = common->GetModeDisplayList();
	idList< idList< idStr, TAG_IDLIB_LIST_MENU >, TAG_IDLIB_LIST_MENU > menuOptions;
	for( int i = 0; i < modes.Num(); ++i )
	{
		idList< idStr > option;
		option.Append( modes[i] );
		menuOptions.Append( option );
	}
	options->SetListData( menuOptions );

	const char* tips[] = { "#str_swf_deathmatch_desc", "#str_swf_tourney_desc", "#str_swf_team_deathmatch_desc", "#str_swf_lastman_desc", "#str_swf_ctf_desc" };

	while( options->GetChildren().Num() < NUM_SETTING_OPTIONS )
	{
		idMenuWidget_Button* const buttonWidget = new( TAG_SWF ) idMenuWidget_Button();
		buttonWidget->Initialize( data );
		buttonWidget->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_PRESS_FOCUSED, options->GetChildren().Num() );

		if( options->GetChildren().Num() < menuOptions.Num() )
		{
			buttonWidget->SetDescription( tips[options->GetChildren().Num()] );
		}

		buttonWidget->RegisterEventObserver( helpWidget );
		options->AddChild( buttonWidget );
	}
	options->Initialize( data );

	btnBack = new( TAG_SWF ) idMenuWidget_Button();
	btnBack->Initialize( data );
	btnBack->SetLabel( "#str_swf_multiplayer" );
	btnBack->SetSpritePath( GetSpritePath(), "info", "btnBack" );
	btnBack->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_GO_BACK );

	AddChild( btnBack );

	options->AddEventAction( WIDGET_EVENT_SCROLL_DOWN ).Set( new( TAG_SWF ) idWidgetActionHandler( options, WIDGET_ACTION_EVENT_SCROLL_DOWN_START_REPEATER, WIDGET_EVENT_SCROLL_DOWN ) );
	options->AddEventAction( WIDGET_EVENT_SCROLL_UP ).Set( new( TAG_SWF ) idWidgetActionHandler( options, WIDGET_ACTION_EVENT_SCROLL_UP_START_REPEATER, WIDGET_EVENT_SCROLL_UP ) );
	options->AddEventAction( WIDGET_EVENT_SCROLL_DOWN_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( options, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_DOWN_RELEASE ) );
	options->AddEventAction( WIDGET_EVENT_SCROLL_UP_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( options, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_UP_RELEASE ) );
	options->AddEventAction( WIDGET_EVENT_SCROLL_DOWN_LSTICK ).Set( new( TAG_SWF ) idWidgetActionHandler( options, WIDGET_ACTION_EVENT_SCROLL_DOWN_START_REPEATER, WIDGET_EVENT_SCROLL_DOWN_LSTICK ) );
	options->AddEventAction( WIDGET_EVENT_SCROLL_UP_LSTICK ).Set( new( TAG_SWF ) idWidgetActionHandler( options, WIDGET_ACTION_EVENT_SCROLL_UP_START_REPEATER, WIDGET_EVENT_SCROLL_UP_LSTICK ) );
	options->AddEventAction( WIDGET_EVENT_SCROLL_DOWN_LSTICK_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( options, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_DOWN_LSTICK_RELEASE ) );
	options->AddEventAction( WIDGET_EVENT_SCROLL_UP_LSTICK_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( options, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_UP_LSTICK_RELEASE ) );
}

/*
========================
idMenuScreen_Shell_ModeSelect::Update
========================
*/
void idMenuScreen_Shell_ModeSelect::Update()
{

	if( menuData != NULL )
	{
		idMenuWidget_CommandBar* cmdBar = menuData->GetCmdBar();
		if( cmdBar != NULL )
		{
			cmdBar->ClearAllButtons();
			idMenuWidget_CommandBar::buttonInfo_t* buttonInfo;
			buttonInfo = cmdBar->GetButton( idMenuWidget_CommandBar::BUTTON_JOY2 );
			if( menuData->GetPlatform() != 2 )
			{
				buttonInfo->label = "#str_00395";
			}
			buttonInfo->action.Set( WIDGET_ACTION_GO_BACK );

			buttonInfo = cmdBar->GetButton( idMenuWidget_CommandBar::BUTTON_JOY1 );
			if( menuData->GetPlatform() != 2 )
			{
				buttonInfo->label = "#str_SWF_SELECT";
			}
			buttonInfo->action.Set( WIDGET_ACTION_PRESS_FOCUSED );
		}
	}

	idSWFScriptObject& root = GetSWFObject()->GetRootObject();
	if( BindSprite( root ) )
	{
		idSWFTextInstance* heading = GetSprite()->GetScriptObject()->GetNestedText( "info", "txtHeading" );
		if( heading != NULL )
		{
			heading->SetText( "#str_swf_find_match_heading" );
			heading->SetStrokeInfo( true, 0.75f, 1.75f );
		}

		idSWFSpriteInstance* gradient = GetSprite()->GetScriptObject()->GetNestedSprite( "info", "gradient" );
		if( gradient != NULL && heading != NULL )
		{
			gradient->SetXPos( heading->GetTextLength() );
		}
	}

	if( btnBack != NULL )
	{
		btnBack->BindSprite( root );
	}

	idMenuScreen::Update();
}

/*
========================
idMenuScreen_Shell_ModeSelect::ShowScreen
========================
*/
void idMenuScreen_Shell_ModeSelect::ShowScreen( const mainMenuTransition_t transitionType )
{
	idMenuScreen::ShowScreen( transitionType );
}

/*
========================
idMenuScreen_Shell_ModeSelect::HideScreen
========================
*/
void idMenuScreen_Shell_ModeSelect::HideScreen( const mainMenuTransition_t transitionType )
{
	idMenuScreen::HideScreen( transitionType );
}

/*
========================
idMenuScreen_Shell_ModeSelect::HandleAction h
========================
*/
bool idMenuScreen_Shell_ModeSelect::HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled )
{

	if( menuData == NULL )
	{
		return true;
	}

	if( menuData->ActiveScreen() != SHELL_AREA_MODE_SELECT )
	{
		return false;
	}

	widgetAction_t actionType = action.GetType();
	const idSWFParmList& parms = action.GetParms();

	switch( actionType )
	{
		case WIDGET_ACTION_GO_BACK:
		{
			menuData->SetNextScreen( SHELL_AREA_PARTY_LOBBY, MENU_TRANSITION_SIMPLE );
			return true;
		}
		case WIDGET_ACTION_PRESS_FOCUSED:
		{
			if( options == NULL )
			{
				return true;
			}
			int selectionIndex = options->GetViewIndex();
			if( parms.Num() == 1 )
			{
				selectionIndex = parms[0].ToInteger();
			}

			if( options->GetFocusIndex() != selectionIndex )
			{
				options->SetFocusIndex( selectionIndex );
				options->SetViewIndex( options->GetViewOffset() + selectionIndex );
			}

			idMatchParameters matchParameters = idMatchParameters( session->GetPartyLobbyBase().GetMatchParms() );
			matchParameters.gameMap = GAME_MAP_RANDOM;
			matchParameters.gameMode = selectionIndex;

			// Always a public match.
			matchParameters.matchFlags &= ~MATCH_INVITE_ONLY;

			session->UpdatePartyParms( matchParameters );

			// Update flags for game lobby.
			matchParameters.matchFlags = DefaultPartyFlags | DefaultPublicGameFlags;

			cvarSystem->MoveCVarsToDict( CVAR_SERVERINFO, matchParameters.serverInfo );

			// Force a default value for the si_timelimit and si_fraglimit for quickmatch
			matchParameters.serverInfo.SetInt( "si_timelimit", 15 );
			matchParameters.serverInfo.SetInt( "si_fraglimit", 10 );

			session->FindOrCreateMatch( matchParameters );

			return true;
		}
	}

	return idMenuWidget::HandleAction( action, event, widget, forceHandled );
}