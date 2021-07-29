
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

const static int NUM_LOBBY_OPTIONS = 8;

extern idCVar net_inviteOnly;

enum gameLobbyCmd_t
{
	GAME_CMD_START,
	GAME_CMD_INVITE,
	GAME_CMD_SETTINGS,
	GAME_CMD_TOGGLE_PRIVACY,
};

/*
========================
idMenuScreen_Shell_GameLobby::Initialize
========================
*/
void idMenuScreen_Shell_GameLobby::Initialize( idMenuHandler* data )
{
	idMenuScreen::Initialize( data );

	if( data != NULL )
	{
		menuGUI = data->GetGUI();
	}

	SetSpritePath( "menuGameLobby" );

	options = new( TAG_SWF ) idMenuWidget_DynamicList();
	options->SetNumVisibleOptions( NUM_LOBBY_OPTIONS );
	options->SetSpritePath( GetSpritePath(), "info", "options" );
	options->SetWrappingAllowed( true );
	AddChild( options );

	idMenuWidget_Help* const helpWidget = new( TAG_SWF ) idMenuWidget_Help();
	helpWidget->SetSpritePath( GetSpritePath(), "info", "helpTooltip" );
	AddChild( helpWidget );

	while( options->GetChildren().Num() < NUM_LOBBY_OPTIONS )
	{
		idMenuWidget_Button* const buttonWidget = new( TAG_SWF ) idMenuWidget_Button();
		buttonWidget->Initialize( data );
		buttonWidget->RegisterEventObserver( helpWidget );
		options->AddChild( buttonWidget );
	}
	options->Initialize( data );

	lobby = new( TAG_SWF ) idMenuWidget_LobbyList();
	lobby->SetNumVisibleOptions( 8 );
	lobby->SetSpritePath( GetSpritePath(), "options" );
	lobby->SetWrappingAllowed( true );
	lobby->Initialize( data );
	while( lobby->GetChildren().Num() < 8 )
	{
		idMenuWidget_LobbyButton* const buttonWidget = new( TAG_SWF ) idMenuWidget_LobbyButton();
		buttonWidget->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_SELECT_GAMERTAG, lobby->GetChildren().Num() );
		buttonWidget->AddEventAction( WIDGET_EVENT_COMMAND ).Set( WIDGET_ACTION_MUTE_PLAYER, lobby->GetChildren().Num() );
		buttonWidget->Initialize( data );
		lobby->AddChild( buttonWidget );
	}
	AddChild( lobby );

	btnBack = new( TAG_SWF ) idMenuWidget_Button();
	btnBack->Initialize( data );
	btnBack->SetLabel( "#str_swf_multiplayer" );
	btnBack->SetSpritePath( GetSpritePath(), "info", "btnBack" );
	btnBack->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_GO_BACK );
	AddChild( btnBack );

	AddEventAction( WIDGET_EVENT_SCROLL_DOWN ).Set( new( TAG_SWF ) idWidgetActionHandler( options, WIDGET_ACTION_EVENT_SCROLL_DOWN_START_REPEATER, WIDGET_EVENT_SCROLL_DOWN ) );
	AddEventAction( WIDGET_EVENT_SCROLL_UP ).Set( new( TAG_SWF ) idWidgetActionHandler( options, WIDGET_ACTION_EVENT_SCROLL_UP_START_REPEATER, WIDGET_EVENT_SCROLL_UP ) );
	AddEventAction( WIDGET_EVENT_SCROLL_DOWN_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( options, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_DOWN_RELEASE ) );
	AddEventAction( WIDGET_EVENT_SCROLL_UP_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( options, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_UP_RELEASE ) );
	AddEventAction( WIDGET_EVENT_SCROLL_DOWN_LSTICK ).Set( new( TAG_SWF ) idWidgetActionHandler( options, WIDGET_ACTION_EVENT_SCROLL_DOWN_START_REPEATER, WIDGET_EVENT_SCROLL_DOWN_LSTICK ) );
	AddEventAction( WIDGET_EVENT_SCROLL_UP_LSTICK ).Set( new( TAG_SWF ) idWidgetActionHandler( options, WIDGET_ACTION_EVENT_SCROLL_UP_START_REPEATER, WIDGET_EVENT_SCROLL_UP_LSTICK ) );
	AddEventAction( WIDGET_EVENT_SCROLL_DOWN_LSTICK_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( options, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_DOWN_LSTICK_RELEASE ) );
	AddEventAction( WIDGET_EVENT_SCROLL_UP_LSTICK_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( options, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_UP_LSTICK_RELEASE ) );
	AddEventAction( WIDGET_EVENT_SCROLL_DOWN_RSTICK ).Set( new( TAG_SWF ) idWidgetActionHandler( lobby, WIDGET_ACTION_EVENT_SCROLL_DOWN_START_REPEATER, WIDGET_EVENT_SCROLL_DOWN_RSTICK ) );
	AddEventAction( WIDGET_EVENT_SCROLL_UP_RSTICK ).Set( new( TAG_SWF ) idWidgetActionHandler( lobby, WIDGET_ACTION_EVENT_SCROLL_UP_START_REPEATER, WIDGET_EVENT_SCROLL_UP_RSTICK ) );
	AddEventAction( WIDGET_EVENT_SCROLL_DOWN_RSTICK_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( lobby, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_DOWN_RSTICK_RELEASE ) );
	AddEventAction( WIDGET_EVENT_SCROLL_UP_RSTICK_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( lobby, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_UP_RSTICK_RELEASE ) );
}

/*
========================
idMenuScreen_Shell_GameLobby::Update
========================
*/
void idMenuScreen_Shell_GameLobby::Update()
{

	idLobbyBase& activeLobby = session->GetActivePlatformLobbyBase();
	if( lobby != NULL )
	{

		if( activeLobby.GetNumActiveLobbyUsers() != 0 )
		{
			if( lobby->GetFocusIndex() >= activeLobby.GetNumActiveLobbyUsers() )
			{
				lobby->SetFocusIndex( activeLobby.GetNumActiveLobbyUsers() - 1 );
				lobby->SetViewIndex( lobby->GetViewOffset() + lobby->GetFocusIndex() );
			}
		}
	}

	idSWFScriptObject& root = GetSWFObject()->GetRootObject();
	if( BindSprite( root ) )
	{
		idSWFTextInstance* heading = GetSprite()->GetScriptObject()->GetNestedText( "info", "txtHeading" );
		if( heading != NULL )
		{
			heading->SetText( "#str_swf_multiplayer" );	// MULTIPLAYER
			heading->SetStrokeInfo( true, 0.75f, 1.75f );
		}

		idSWFSpriteInstance* gradient = GetSprite()->GetScriptObject()->GetNestedSprite( "info", "gradient" );
		if( gradient != NULL && heading != NULL )
		{
			gradient->SetXPos( heading->GetTextLength() );
		}
	}

	if( privateGameLobby && options != NULL )
	{

		if( session->GetActivePlatformLobbyBase().IsHost() && !isHost )
		{

			menuOptions.Clear();
			idList< idStr > option;

			isHost = true;
			isPeer = false;

			option.Append( "#str_swf_start_match" );	// Start match
			menuOptions.Append( option );
			option.Clear();

			option.Append( "#str_swf_match_settings" );	// Match Settings
			menuOptions.Append( option );
			option.Clear();

			option.Append( "#str_swf_invite_only" );	// Toggle privacy
			menuOptions.Append( option );
			option.Clear();

			option.Append( "#str_swf_invite_friends" );	// Invite Friends
			menuOptions.Append( option );
			option.Clear();

			idMenuWidget_Button* buttonWidget = NULL;
			int index = 0;
			options->GetChildByIndex( index ).ClearEventActions();
			options->GetChildByIndex( index ).AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, GAME_CMD_START, 0 );
			buttonWidget = dynamic_cast< idMenuWidget_Button* >( &options->GetChildByIndex( index ) );
			if( buttonWidget != NULL )
			{
				buttonWidget->SetDescription( "#str_swf_quick_start_desc" );
			}
			index++;
			options->GetChildByIndex( index ).ClearEventActions();
			options->GetChildByIndex( index ).AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, GAME_CMD_SETTINGS, 1 );
			buttonWidget = dynamic_cast< idMenuWidget_Button* >( &options->GetChildByIndex( index ) );
			if( buttonWidget != NULL )
			{
				buttonWidget->SetDescription( "#str_swf_match_setting_desc" );
			}
			index++;
			options->GetChildByIndex( index ).ClearEventActions();
			options->GetChildByIndex( index ).AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, GAME_CMD_TOGGLE_PRIVACY, 2 );
			buttonWidget = dynamic_cast< idMenuWidget_Button* >( &options->GetChildByIndex( index ) );
			if( buttonWidget != NULL )
			{
				buttonWidget->SetDescription( "#str_swf_toggle_privacy_desc" );
			}
			index++;
			options->GetChildByIndex( index ).ClearEventActions();
			options->GetChildByIndex( index ).AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, GAME_CMD_INVITE, 3 );
			buttonWidget = dynamic_cast< idMenuWidget_Button* >( &options->GetChildByIndex( index ) );
			if( buttonWidget != NULL )
			{
				buttonWidget->SetDescription( "#str_swf_invite_desc" );
			}
			index++;

			options->SetListData( menuOptions );

		}
		else if( session->GetActivePlatformLobbyBase().IsPeer() )
		{

			if( !isPeer )
			{

				menuOptions.Clear();
				idList< idStr > option;

				option.Append( "#str_swf_invite_friends" );	// Invite Friends
				menuOptions.Append( option );
				option.Clear();

				idMenuWidget_Button* buttonWidget = NULL;
				int index = 0;
				options->GetChildByIndex( index ).ClearEventActions();
				options->GetChildByIndex( index ).AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, GAME_CMD_INVITE, 0 );
				buttonWidget = dynamic_cast< idMenuWidget_Button* >( &options->GetChildByIndex( index ) );
				if( buttonWidget != NULL )
				{
					buttonWidget->SetDescription( "#str_swf_invite_desc" );
				}

				options->SetListData( menuOptions );
			}

			isPeer = true;
			isHost = false;
		}
	}

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

			buttonInfo = cmdBar->GetButton( idMenuWidget_CommandBar::BUTTON_JOY3 );
			if( menuData->GetPlatform() != 2 )
			{
				buttonInfo->label = "#str_swf_view_profile";
			}
			buttonInfo->action.Set( WIDGET_ACTION_SELECT_GAMERTAG );

			buttonInfo = cmdBar->GetButton( idMenuWidget_CommandBar::BUTTON_JOY1 );
			if( menuData->GetPlatform() != 2 )
			{
				buttonInfo->label = "#str_SWF_SELECT";
			}
			buttonInfo->action.Set( WIDGET_ACTION_PRESS_FOCUSED );

			lobbyUserID_t luid;
			if( isHost && CanKickSelectedPlayer( luid ) )
			{
				buttonInfo = cmdBar->GetButton( idMenuWidget_CommandBar::BUTTON_JOY4 );
				buttonInfo->label = "#str_swf_kick";
				buttonInfo->action.Set( WIDGET_ACTION_JOY4_ON_PRESS );
			}
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
idMenuScreen_Shell_GameLobby::ShowScreen
========================
*/
void idMenuScreen_Shell_GameLobby::ShowScreen( const mainMenuTransition_t transitionType )
{

	if( options != NULL )
	{
		options->SetFocusIndex( 0 );
		options->SetViewIndex( 0 );
	}

	isHost = false;
	isPeer = false;

	idMatchParameters matchParameters = session->GetActivePlatformLobbyBase().GetMatchParms();

	// Make sure map name is up to date.
	if( matchParameters.gameMap >= 0 )
	{
		matchParameters.mapName = common->GetMapList()[ matchParameters.gameMap ].mapFile;
	}

	privateGameLobby = MatchTypeIsPrivate( matchParameters.matchFlags );

	if( !privateGameLobby )  	// Public Game Lobby
	{
		menuOptions.Clear();
		idList< idStr > option;

		if( options != NULL )
		{
			option.Append( "#str_swf_invite_friends" );	// Invite Friends
			menuOptions.Append( option );
			option.Clear();

			int index = 0;
			options->GetChildByIndex( index ).ClearEventActions();
			options->GetChildByIndex( index ).AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, GAME_CMD_INVITE, 0 );
			idMenuWidget_Button* buttonWidget = dynamic_cast< idMenuWidget_Button* >( &options->GetChildByIndex( index ) );
			if( buttonWidget != NULL )
			{
				buttonWidget->SetDescription( "#str_swf_invite_desc" );
			}

			options->SetListData( menuOptions );
		}

		longCountdown = Sys_Milliseconds() + WAIT_START_TIME_LONG;
		longCountRemaining = longCountdown;
		shortCountdown = Sys_Milliseconds() + WAIT_START_TIME_SHORT;
	}

	idSWFScriptObject& root = GetSWFObject()->GetRootObject();
	if( BindSprite( root ) )
	{
		idSWFSpriteInstance* waitTime = GetSprite()->GetScriptObject()->GetNestedSprite( "waitTime" );
		if( waitTime != NULL )
		{
			waitTime->SetVisible( !privateGameLobby );
		}
	}

	idMenuScreen::ShowScreen( transitionType );

	if( lobby != NULL )
	{
		lobby->SetFocusIndex( 0 );
	}

	session->UpdateMatchParms( matchParameters );
}

/*
========================
idMenuScreen_Shell_GameLobby::HideScreen
========================
*/
void idMenuScreen_Shell_GameLobby::HideScreen( const mainMenuTransition_t transitionType )
{
	idMenuScreen::HideScreen( transitionType );
}

/*
========================
idMenuScreen_Shell_GameLobby::CanKickSelectedPlayer
========================
*/
bool idMenuScreen_Shell_GameLobby::CanKickSelectedPlayer( lobbyUserID_t& luid )
{

	idMatchParameters matchParameters = session->GetActivePlatformLobbyBase().GetMatchParms();
	const int playerId = lobby->GetFocusIndex();

	// can't kick yourself
	idLobbyBase& activeLobby = session->GetActivePlatformLobbyBase();
	luid = activeLobby.GetLobbyUserIdByOrdinal( playerId );
	if( session->GetSignInManager().GetMasterLocalUser() == activeLobby.GetLocalUserFromLobbyUser( luid ) )
	{
		return false;
	}

	return true;
}

/*
========================
idMenuScreen_Shell_GameLobby::HandleAction h
========================
*/
bool idMenuScreen_Shell_GameLobby::HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled )
{

	if( menuData == NULL )
	{
		return true;
	}

	if( menuData->ActiveScreen() != SHELL_AREA_GAME_LOBBY )
	{
		return false;
	}

	widgetAction_t actionType = action.GetType();
	const idSWFParmList& parms = action.GetParms();

	switch( actionType )
	{
		case WIDGET_ACTION_JOY4_ON_PRESS:
		{
			idLobbyBase& activeLobby = session->GetActivePlatformLobbyBase();
			lobbyUserID_t luid;
			if( CanKickSelectedPlayer( luid ) )
			{
				activeLobby.KickLobbyUser( luid );
			}
			return true;
		}
		case WIDGET_ACTION_GO_BACK:
		{
			class idSWFScriptFunction_Accept : public idSWFScriptFunction_RefCounted
			{
			public:
				idSWFScriptFunction_Accept() { }
				idSWFScriptVar Call( idSWFScriptObject* thisObject, const idSWFParmList& parms )
				{
					common->Dialog().ClearDialog( GDM_LEAVE_LOBBY_RET_NEW_PARTY );
					session->Cancel();

					return idSWFScriptVar();
				}
			};
			class idSWFScriptFunction_Cancel : public idSWFScriptFunction_RefCounted
			{
			public:
				idSWFScriptFunction_Cancel() { }
				idSWFScriptVar Call( idSWFScriptObject* thisObject, const idSWFParmList& parms )
				{
					common->Dialog().ClearDialog( GDM_LEAVE_LOBBY_RET_NEW_PARTY );
					return idSWFScriptVar();
				}
			};

			idLobbyBase& activeLobby = session->GetActivePlatformLobbyBase();

			if( activeLobby.GetNumActiveLobbyUsers() > 1 )
			{
				common->Dialog().AddDialog( GDM_LEAVE_LOBBY_RET_NEW_PARTY, DIALOG_ACCEPT_CANCEL, new( TAG_SWF ) idSWFScriptFunction_Accept(), new( TAG_SWF ) idSWFScriptFunction_Cancel(), false );
			}
			else
			{
				session->Cancel();
			}

			return true;
		}
		case WIDGET_ACTION_MUTE_PLAYER:
		{

			if( parms.Num() != 1 )
			{
				return true;
			}

			int index = parms[0].ToInteger();

			idLobbyBase& activeLobby = session->GetActivePlatformLobbyBase();
			lobbyUserID_t luid = activeLobby.GetLobbyUserIdByOrdinal( index );
			if( luid.IsValid() )
			{
				session->ToggleLobbyUserVoiceMute( luid );
			}

			return true;
		}
		case WIDGET_ACTION_COMMAND:
		{

			if( options == NULL )
			{
				return true;
			}

			int selectionIndex = options->GetFocusIndex();
			if( parms.Num() > 1 )
			{
				selectionIndex = parms[1].ToInteger();
			}

			if( selectionIndex != options->GetFocusIndex() )
			{
				options->SetViewIndex( options->GetViewOffset() + selectionIndex );
				options->SetFocusIndex( selectionIndex );
			}

			switch( parms[0].ToInteger() )
			{
				case GAME_CMD_START:
				{
					idMenuHandler_Shell* handler = dynamic_cast< idMenuHandler_Shell* const >( menuData );
					if( handler != NULL )
					{
						handler->SetTimeRemaining( 0 );
					}
					break;
				}
				case GAME_CMD_SETTINGS:
				{
					menuData->SetNextScreen( SHELL_AREA_MATCH_SETTINGS, MENU_TRANSITION_SIMPLE );
					break;
				}
				case GAME_CMD_TOGGLE_PRIVACY:
				{
					idMatchParameters matchParameters = idMatchParameters( session->GetActivePlatformLobbyBase().GetMatchParms() );
					matchParameters.matchFlags ^= MATCH_INVITE_ONLY;
					session->UpdateMatchParms( matchParameters );
					int bitSet = ( matchParameters.matchFlags & MATCH_INVITE_ONLY );
					net_inviteOnly.SetBool( bitSet != 0 ? true : false );

					// Must update the party parameters too for Xbox JSIP to work in game lobbies.
					idMatchParameters partyParms = session->GetPartyLobbyBase().GetMatchParms();
					if( MatchTypeInviteOnly( matchParameters.matchFlags ) )
					{
						partyParms.matchFlags |= MATCH_INVITE_ONLY;
					}
					else
					{
						partyParms.matchFlags &= ~MATCH_INVITE_ONLY;
					}
					session->UpdatePartyParms( partyParms );

					break;
				}
				case GAME_CMD_INVITE:
				{
					if( session->GetActivePlatformLobbyBase().IsLobbyFull() )
					{
						common->Dialog().AddDialog( GDM_CANNOT_INVITE_LOBBY_FULL, DIALOG_CONTINUE, NULL, NULL, true, __FUNCTION__, __LINE__, false );
						return true;
					}

					InvitePartyOrFriends();
					break;
				}
			}
			return true;
		}
		case WIDGET_ACTION_START_REPEATER:
		{

			if( options == NULL )
			{
				return true;
			}

			if( parms.Num() == 4 )
			{
				int selectionIndex = parms[3].ToInteger();
				if( selectionIndex != options->GetFocusIndex() )
				{
					options->SetViewIndex( options->GetViewOffset() + selectionIndex );
					options->SetFocusIndex( selectionIndex );
				}
			}
			break;
		}
		case WIDGET_ACTION_SELECT_GAMERTAG:
		{
			int selectionIndex = lobby->GetFocusIndex();
			if( parms.Num() > 0 )
			{
				selectionIndex = parms[0].ToInteger();
			}

			if( selectionIndex != lobby->GetFocusIndex() )
			{
				lobby->SetViewIndex( lobby->GetViewOffset() + selectionIndex );
				lobby->SetFocusIndex( selectionIndex );
				return true;
			}

			idLobbyBase& activeLobby = session->GetActivePlatformLobbyBase();
			lobbyUserID_t luid = activeLobby.GetLobbyUserIdByOrdinal( selectionIndex );
			if( luid.IsValid() )
			{
				session->ShowLobbyUserGamerCardUI( luid );
			}

			return true;
		}
	}

	return idMenuWidget::HandleAction( action, event, widget, forceHandled );
}

/*
========================
idMenuScreen_Shell_GameLobby::UpdateLobby
========================
*/
void idMenuScreen_Shell_GameLobby::UpdateLobby()
{

	if( menuData != NULL && menuData->ActiveScreen() != SHELL_AREA_GAME_LOBBY )
	{
		return;
	}

	// Keep this menu in sync with the session host/peer status.
	if( session->GetActivePlatformLobbyBase().IsHost() && !isHost )
	{
		Update();
	}

	if( session->GetActivePlatformLobbyBase().IsPeer() && !isPeer )
	{
		Update();
	}

	if( !privateGameLobby )
	{
		int ms = 0;
		if( session->GetActivePlatformLobbyBase().IsHost() )
		{
			idMenuHandler_Shell* handler = dynamic_cast< idMenuHandler_Shell* const >( menuData );
			if( handler != NULL )
			{
				if( session->GetActivePlatformLobbyBase().IsLobbyFull() )
				{
					longCountdown = Sys_Milliseconds() + longCountRemaining;
					int timeRemaining = shortCountdown - Sys_Milliseconds();
					if( timeRemaining < 0 )
					{
						timeRemaining = 0;
					}
					ms = ( int ) ceilf( timeRemaining / 1000.0f );
					handler->SetTimeRemaining( timeRemaining );
				}
				else if( session->GetActivePlatformLobbyBase().GetNumLobbyUsers() > 1 )
				{
					int timeRemaining = longCountdown - Sys_Milliseconds();
					if( timeRemaining > WAIT_START_TIME_SHORT )
					{
						shortCountdown = Sys_Milliseconds() + WAIT_START_TIME_SHORT;
					}
					else
					{
						shortCountdown = timeRemaining;
					}
					longCountRemaining = timeRemaining;
					if( timeRemaining < 0 )
					{
						timeRemaining = 0;
					}
					ms = ( int ) ceilf( timeRemaining / 1000.0f );
					handler->SetTimeRemaining( timeRemaining );
				}
				else
				{
					ms = 0;
					longCountdown = Sys_Milliseconds() + WAIT_START_TIME_LONG;
					longCountRemaining = longCountdown;
					shortCountdown = Sys_Milliseconds() + WAIT_START_TIME_SHORT;
					handler->SetTimeRemaining( longCountRemaining );
				}
			}
		}
		else
		{
			if( menuData != NULL )
			{
				idMenuHandler_Shell* handler = dynamic_cast< idMenuHandler_Shell* const >( menuData );
				if( handler != NULL )
				{
					ms = ( int ) ceilf( handler->GetTimeRemaining() / 1000.0f );
				}
			}
		}

		idSWFScriptObject& root = GetSWFObject()->GetRootObject();
		if( BindSprite( root ) )
		{
			idSWFTextInstance* waitTime = GetSprite()->GetScriptObject()->GetNestedText( "waitTime", "txtVal" );
			if( waitTime != NULL )
			{
				idStr status;
				if( ms == 1 )
				{
					status = idLocalization::GetString( "#str_online_game_starts_in_second" );
					status.Replace( "<DNT_VAL>", idStr( ms ) );
					waitTime->SetText( status );
				}
				else if( ms > 0 && ms < 30 )
				{
					status = idLocalization::GetString( "#str_online_game_starts_in_seconds" );
					status.Replace( "<DNT_VAL>", idStr( ms ) );
					waitTime->SetText( status );
				}
				else
				{
					waitTime->SetText( "" );
				}
				waitTime->SetStrokeInfo( true, 0.75f, 2.0f );
			}
		}
		Update();

	}
	else
	{

		if( isPeer )
		{
			Update();
		}

	}

	if( session->GetState() == idSession::GAME_LOBBY )
	{

		if( options != NULL )
		{
			if( options->GetFocusIndex() >= options->GetTotalNumberOfOptions() && options->GetTotalNumberOfOptions() > 0 )
			{
				options->SetViewIndex( options->GetTotalNumberOfOptions() - 1 );
				options->SetFocusIndex( options->GetTotalNumberOfOptions() - 1 );
			}
		}

		idMatchParameters matchParameters = session->GetActivePlatformLobbyBase().GetMatchParms();

		idSWFTextInstance* mapName = GetSprite()->GetScriptObject()->GetNestedText( "matchInfo", "txtMapName" );
		idSWFTextInstance* modeName = GetSprite()->GetScriptObject()->GetNestedText( "matchInfo", "txtModeName" );

		if( mapName != NULL )
		{
			const idList< mpMap_t > maps = common->GetMapList();
			idStr name = idLocalization::GetString( maps[ idMath::ClampInt( 0, maps.Num() - 1, matchParameters.gameMap ) ].mapName );
			mapName->SetText( name );
			mapName->SetStrokeInfo( true );
		}

		if( modeName != NULL )
		{
			const idStrList& modes = common->GetModeDisplayList();
			idStr mode = idLocalization::GetString( modes[ idMath::ClampInt( 0, modes.Num() - 1, matchParameters.gameMode ) ] );
			modeName->SetText( mode );
			modeName->SetStrokeInfo( true );
		}

		idSWFTextInstance* privacy = GetSprite()->GetScriptObject()->GetNestedText( "matchInfo", "txtPrivacy" );
		if( privacy != NULL )
		{
			if( isPeer || !privateGameLobby )
			{
				privacy->SetText( "" );
			}
			else
			{
				int bitSet = ( matchParameters.matchFlags & MATCH_INVITE_ONLY );
				bool privacySet = ( bitSet != 0 ? true : false );
				if( privacySet )
				{
					privacy->SetText( "#str_swf_privacy_closed" );
					privacy->SetStrokeInfo( true );
				}
				else if( !privacySet )
				{
					privacy->SetText( "#str_swf_privacy_open" );
					privacy->SetStrokeInfo( true );
				}
			}
		}

		idLocalUser* user = session->GetSignInManager().GetMasterLocalUser();
		if( user != NULL && options != NULL )
		{
			if( user->IsInParty() && user->GetPartyCount() > 1 && !session->IsPlatformPartyInLobby() && menuOptions.Num() > 0 )
			{
				if( menuOptions[ menuOptions.Num() - 1 ][0] != "#str_swf_invite_xbox_live_party" )
				{
					menuOptions[ menuOptions.Num() - 1 ][0] = "#str_swf_invite_xbox_live_party";	// invite Xbox LIVE party
					options->SetListData( menuOptions );
					options->Update();
				}
			}
			else if( menuOptions.Num() > 0 )
			{
				if( menuOptions[ menuOptions.Num() - 1 ][0] != "#str_swf_invite_friends" )
				{
					menuOptions[ menuOptions.Num() - 1 ][0] = "#str_swf_invite_friends";	// invite Xbox LIVE party
					options->SetListData( menuOptions );
					options->Update();
				}
			}
		}
	}

	// setup names for lobby;
	if( lobby != NULL )
	{
		idMenuHandler_Shell* mgr = dynamic_cast< idMenuHandler_Shell* >( menuData );
		if( mgr != NULL )
		{
			mgr->UpdateLobby( lobby );
			lobby->Update();
		}

		if( lobby->GetNumEntries() > 0 && lobby->GetFocusIndex() >= lobby->GetNumEntries() )
		{
			lobby->SetFocusIndex( lobby->GetNumEntries() - 1 );
			lobby->SetViewIndex( lobby->GetNumEntries() - 1 );
		}
	}
}



