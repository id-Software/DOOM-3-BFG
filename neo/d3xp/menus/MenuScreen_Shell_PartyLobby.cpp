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
extern idCVar si_map;
extern idCVar si_mode;

enum partyLobbyCmds_t
{
	PARTY_CMD_QUICK,
	PARTY_CMD_FIND,
	PARTY_CMD_CREATE,
	PARTY_CMD_PWF,
	PARTY_CMD_INVITE,
	PARTY_CMD_LEADERBOARDS,
	PARTY_CMD_TOGGLE_PRIVACY,
	PARTY_CMD_SHOW_PARTY_GAMES,
};

/*
========================
idMenuScreen_Shell_PartyLobby::Initialize
========================
*/
void idMenuScreen_Shell_PartyLobby::Initialize( idMenuHandler* data )
{
	idMenuScreen::Initialize( data );

	if( data != NULL )
	{
		menuGUI = data->GetGUI();
	}

	SetSpritePath( "menuPartyLobby" );

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


	btnBack = new( TAG_SWF ) idMenuWidget_Button();
	btnBack->Initialize( data );
	btnBack->SetLabel( "#str_02305" );
	btnBack->SetSpritePath( GetSpritePath(), "info", "btnBack" );
	btnBack->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_GO_BACK );
	AddChild( btnBack );

	lobby = new( TAG_SWF ) idMenuWidget_LobbyList();
	lobby->SetNumVisibleOptions( 8 );
	lobby->SetSpritePath( GetSpritePath(), "options" );
	lobby->SetWrappingAllowed( true );
	lobby->Initialize( data );
	while( lobby->GetChildren().Num() < 8 )
	{
		idMenuWidget_LobbyButton* const buttonWidget = new idMenuWidget_LobbyButton();
		buttonWidget->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_SELECT_GAMERTAG, lobby->GetChildren().Num() );
		buttonWidget->AddEventAction( WIDGET_EVENT_COMMAND ).Set( WIDGET_ACTION_MUTE_PLAYER, lobby->GetChildren().Num() );
		buttonWidget->Initialize( data );
		lobby->AddChild( buttonWidget );
	}
	AddChild( lobby );

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
idMenuScreen_Shell_PartyLobby::Update
========================
*/
void idMenuScreen_Shell_PartyLobby::Update()
{

	idLobbyBase& activeLobby = session->GetPartyLobbyBase();
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

	UpdateOptions();

	if( menuData != NULL && menuData->NextScreen() == SHELL_AREA_PARTY_LOBBY )
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

			lobbyUserID_t luid;
			if( isHost && CanKickSelectedPlayer( luid ) )
			{
				buttonInfo = cmdBar->GetButton( idMenuWidget_CommandBar::BUTTON_JOY4 );
				buttonInfo->label = "#str_swf_kick";
				buttonInfo->action.Set( WIDGET_ACTION_JOY4_ON_PRESS );
			}

			buttonInfo = cmdBar->GetButton( idMenuWidget_CommandBar::BUTTON_JOY3 );
			if( menuData->GetPlatform() != 2 )
			{
				buttonInfo->label = "#str_swf_view_profile";
			}
			buttonInfo->action.Set( WIDGET_ACTION_SELECT_GAMERTAG );
		}
	}

	if( btnBack != NULL )
	{
		btnBack->BindSprite( root );
	}

	idMenuScreen::Update();
}

void idMenuScreen_Shell_PartyLobby::UpdateOptions()
{

	bool forceUpdate = false;


	if( ( session->GetPartyLobbyBase().IsHost() && ( !isHost || forceUpdate ) ) && options != NULL )
	{

		menuOptions.Clear();
		idList< idStr > option;

		isHost = true;
		isPeer = false;

		option.Append( "#str_swf_join_public" );	// Quick Match
		menuOptions.Append( option );
		option.Clear();
		option.Append( "#str_swf_find_match" );	// Find Match
		menuOptions.Append( option );
		option.Clear();
		option.Append( "#str_swf_create_private" );	// Create Match
		menuOptions.Append( option );
		option.Clear();
		option.Append( "#str_swf_pwf" );	// Play With Friends
		menuOptions.Append( option );
		option.Clear();
		option.Append( "#str_swf_leaderboards" );	// Play With Friends
		menuOptions.Append( option );
		option.Clear();
		option.Append( "#str_swf_invite_only" );	// Toggle privacy
		menuOptions.Append( option );
		option.Clear();
		option.Append( "#str_swf_invite_friends" );	// Invite Friends
		menuOptions.Append( option );

		idMenuWidget_Button* buttonWidget = NULL;
		int index = 0;
		options->GetChildByIndex( index ).ClearEventActions();
		options->GetChildByIndex( index ).AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, PARTY_CMD_QUICK, index );
		buttonWidget = dynamic_cast< idMenuWidget_Button* >( &options->GetChildByIndex( index ) );
		if( buttonWidget != NULL )
		{
			buttonWidget->SetDescription( "#str_swf_quick_desc" );
		}
		index++;
		options->GetChildByIndex( index ).ClearEventActions();
		options->GetChildByIndex( index ).AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, PARTY_CMD_FIND, index );
		buttonWidget = dynamic_cast< idMenuWidget_Button* >( &options->GetChildByIndex( index ) );
		if( buttonWidget != NULL )
		{
			buttonWidget->SetDescription( "#str_swf_find_desc" );
		}
		index++;
		options->GetChildByIndex( index ).ClearEventActions();
		options->GetChildByIndex( index ).AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, PARTY_CMD_CREATE, index );
		buttonWidget = dynamic_cast< idMenuWidget_Button* >( &options->GetChildByIndex( index ) );
		if( buttonWidget != NULL )
		{
			buttonWidget->SetDescription( "#str_swf_create_desc" );
		}
		index++;
		options->GetChildByIndex( index ).ClearEventActions();
		options->GetChildByIndex( index ).AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, PARTY_CMD_PWF, index );
		buttonWidget = dynamic_cast< idMenuWidget_Button* >( &options->GetChildByIndex( index ) );
		if( buttonWidget != NULL )
		{
			buttonWidget->SetDescription( "#str_swf_pwf_desc" );
		}
		index++;
		options->GetChildByIndex( index ).ClearEventActions();
		options->GetChildByIndex( index ).AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, PARTY_CMD_LEADERBOARDS, index );
		buttonWidget = dynamic_cast< idMenuWidget_Button* >( &options->GetChildByIndex( index ) );
		if( buttonWidget != NULL )
		{
			buttonWidget->SetDescription( "#str_swf_leaderboards_desc" );
		}
		index++;
		options->GetChildByIndex( index ).ClearEventActions();
		options->GetChildByIndex( index ).AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, PARTY_CMD_TOGGLE_PRIVACY, index );
		buttonWidget = dynamic_cast< idMenuWidget_Button* >( &options->GetChildByIndex( index ) );
		if( buttonWidget != NULL )
		{
			buttonWidget->SetDescription( "#str_swf_toggle_privacy_desc" );
		}
		index++;
		options->GetChildByIndex( index ).ClearEventActions();
		options->GetChildByIndex( index ).AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, PARTY_CMD_INVITE, index );
		buttonWidget = dynamic_cast< idMenuWidget_Button* >( &options->GetChildByIndex( index ) );
		if( buttonWidget != NULL )
		{
			buttonWidget->SetDescription( "#str_swf_invite_desc" );
		}

		options->SetListData( menuOptions );

	}
	else if( session->GetPartyLobbyBase().IsPeer() && options != NULL )
	{
		if( !isPeer || forceUpdate )
		{

			menuOptions.Clear();
			idList< idStr > option;

			idMenuWidget_Button* buttonWidget = NULL;
			option.Append( "#str_swf_leaderboards" );	// Play With Friends
			menuOptions.Append( option );
			option.Clear();
			option.Append( "#str_swf_invite_friends" );	// Play With Friends
			menuOptions.Append( option );

			int index = 0;
			options->GetChildByIndex( index ).ClearEventActions();
			options->GetChildByIndex( index ).AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, PARTY_CMD_LEADERBOARDS, index );
			buttonWidget = dynamic_cast< idMenuWidget_Button* >( &options->GetChildByIndex( index ) );
			if( buttonWidget != NULL )
			{
				buttonWidget->SetDescription( "#str_swf_leaderboards_desc" );
			}
			index++;
			options->GetChildByIndex( index ).ClearEventActions();
			options->GetChildByIndex( index ).AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, PARTY_CMD_INVITE, index );
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

	if( forceUpdate )
	{
		options->Update();
	}

}

/*
========================
idMenuScreen_Shell_PartyLobby::ShowScreen
========================
*/
void idMenuScreen_Shell_PartyLobby::ShowScreen( const mainMenuTransition_t transitionType )
{

	isPeer = false;
	isHost = false;

	idSWFScriptObject& root = GetSWFObject()->GetRootObject();
	if( BindSprite( root ) )
	{
		idSWFSpriteInstance* waitTime = GetSprite()->GetScriptObject()->GetNestedSprite( "waitTime" );
		if( waitTime != NULL )
		{
			waitTime->SetVisible( false );
		}
	}

	if( session->GetPartyLobbyBase().IsHost() )
	{
		idMatchParameters matchParameters = session->GetPartyLobbyBase().GetMatchParms();
		if( net_inviteOnly.GetBool() )
		{
			matchParameters.matchFlags |= MATCH_INVITE_ONLY;
		}
		else
		{
			matchParameters.matchFlags &= ~MATCH_INVITE_ONLY;
		}

		matchParameters.numSlots = session->GetTitleStorageInt( "MAX_PLAYERS_ALLOWED", 4 );

		session->UpdatePartyParms( matchParameters );
	}

	idMenuScreen::ShowScreen( transitionType );
	if( lobby != NULL )
	{
		lobby->SetFocusIndex( 0 );
	}
}

/*
========================
idMenuScreen_Shell_PartyLobby::HideScreen
========================
*/
void idMenuScreen_Shell_PartyLobby::HideScreen( const mainMenuTransition_t transitionType )
{
	idMenuScreen::HideScreen( transitionType );
}

/*
========================
idMenuScreen_Shell_PartyLobby::CanKickSelectedPlayer
========================
*/
bool idMenuScreen_Shell_PartyLobby::CanKickSelectedPlayer( lobbyUserID_t& luid )
{

	idMatchParameters matchParameters = session->GetPartyLobbyBase().GetMatchParms();
	const int playerId = lobby->GetFocusIndex();

	// can't kick yourself
	idLobbyBase& activeLobby = session->GetPartyLobbyBase();
	luid = activeLobby.GetLobbyUserIdByOrdinal( playerId );
	if( session->GetSignInManager().GetMasterLocalUser() == activeLobby.GetLocalUserFromLobbyUser( luid ) )
	{
		return false;
	}

	return true;
}

/*
========================
idMenuScreen_Shell_PartyLobby::ShowLeaderboards
========================
*/
void idMenuScreen_Shell_PartyLobby::ShowLeaderboards()
{

	const bool canPlayOnline = session->GetSignInManager().GetMasterLocalUser() != NULL && session->GetSignInManager().GetMasterLocalUser()->CanPlayOnline();

	if( !canPlayOnline )
	{
		common->Dialog().AddDialog( GDM_LEADERBOARD_ONLINE_NO_PROFILE, DIALOG_CONTINUE, NULL, NULL, true, __FUNCTION__, __LINE__, false );
	}
	else if( menuData != NULL )
	{
		menuData->SetNextScreen( SHELL_AREA_LEADERBOARDS, MENU_TRANSITION_SIMPLE );
	}
}

/*
========================
idMenuScreen_Shell_PartyLobby::HandleAction h
========================
*/
bool idMenuScreen_Shell_PartyLobby::HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled )
{

	if( menuData == NULL )
	{
		return true;
	}

	if( menuData->ActiveScreen() != SHELL_AREA_PARTY_LOBBY )
	{
		return false;
	}

	widgetAction_t actionType = action.GetType();
	const idSWFParmList& parms = action.GetParms();

	switch( actionType )
	{
		case WIDGET_ACTION_JOY4_ON_PRESS:
		{

			idLobbyBase& activeLobby = session->GetPartyLobbyBase();
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
					common->Dialog().ClearDialog( GDM_LEAVE_LOBBY_RET_MAIN );
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
					common->Dialog().ClearDialog( GDM_LEAVE_LOBBY_RET_MAIN );
					return idSWFScriptVar();
				}
			};

			idLobbyBase& activeLobby = session->GetActivePlatformLobbyBase();

			if( activeLobby.GetNumActiveLobbyUsers() > 1 )
			{
				common->Dialog().AddDialog( GDM_LEAVE_LOBBY_RET_MAIN, DIALOG_ACCEPT_CANCEL, new( TAG_SWF ) idSWFScriptFunction_Accept(), new( TAG_SWF ) idSWFScriptFunction_Cancel(), false );
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

			idLobbyBase& activeLobby = session->GetPartyLobbyBase();
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
				case PARTY_CMD_QUICK:
				{
					idMatchParameters matchParameters = idMatchParameters( session->GetPartyLobbyBase().GetMatchParms() );

					// Reset these to random for quick match.
					matchParameters.gameMap =  GAME_MAP_RANDOM;
					matchParameters.gameMode = GAME_MODE_RANDOM;

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
					break;
				}
				case PARTY_CMD_FIND:
				{
					menuData->SetNextScreen( SHELL_AREA_MODE_SELECT, MENU_TRANSITION_SIMPLE );
					break;
				}
				case PARTY_CMD_CREATE:
				{
					idMatchParameters matchParameters = idMatchParameters( session->GetPartyLobbyBase().GetMatchParms() );

					const bool isInviteOnly = MatchTypeInviteOnly( matchParameters.matchFlags );

					matchParameters.matchFlags = DefaultPartyFlags | DefaultPrivateGameFlags;

					if( isInviteOnly )
					{
						matchParameters.matchFlags |= MATCH_INVITE_ONLY;
					}

					int mode = idMath::ClampInt( -1, GAME_COUNT - 1, si_mode.GetInteger() );
					const idList< mpMap_t > maps = common->GetMapList();
					int map = idMath::ClampInt( -1, maps.Num() - 1, si_map.GetInteger() );

					matchParameters.gameMap = map;
					matchParameters.gameMode = mode;
					cvarSystem->MoveCVarsToDict( CVAR_SERVERINFO, matchParameters.serverInfo );
					session->CreateMatch( matchParameters );
					break;
				}
				case PARTY_CMD_PWF:
				{
					menuData->SetNextScreen( SHELL_AREA_BROWSER, MENU_TRANSITION_SIMPLE );
					break;
				}
				case PARTY_CMD_LEADERBOARDS:
				{
					ShowLeaderboards();
					break;
				}
				case PARTY_CMD_TOGGLE_PRIVACY:
				{
					idMatchParameters matchParameters = idMatchParameters( session->GetPartyLobbyBase().GetMatchParms() );
					matchParameters.matchFlags ^= MATCH_INVITE_ONLY;
					session->UpdatePartyParms( matchParameters );
					int bitSet = ( matchParameters.matchFlags & MATCH_INVITE_ONLY );
					net_inviteOnly.SetBool( bitSet != 0 ? true : false );
					break;
				}
				case PARTY_CMD_SHOW_PARTY_GAMES:
				{
					session->ShowPartySessions();
					break;
				}
				case PARTY_CMD_INVITE:
				{
					if( session->GetPartyLobbyBase().IsLobbyFull() )
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

			idLobbyBase& activeLobby = session->GetPartyLobbyBase();
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
idMenuScreen_Shell_PartyLobby::UpdateLobby
========================
*/
void idMenuScreen_Shell_PartyLobby::UpdateLobby()
{

	if( menuData != NULL && menuData->ActiveScreen() != SHELL_AREA_PARTY_LOBBY )
	{
		return;
	}

	// Keep this menu in sync with the session host/peer status.
	if( session->GetPartyLobbyBase().IsHost() && !isHost )
	{
		Update();
	}

	if( session->GetPartyLobbyBase().IsPeer() && !isPeer )
	{
		Update();
	}

	if( isPeer )
	{
		Update();
	}

	UpdateOptions();

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

	if( session->GetState() == idSession::PARTY_LOBBY )
	{

		if( options != NULL )
		{
			if( options->GetFocusIndex() >= options->GetTotalNumberOfOptions() && options->GetTotalNumberOfOptions() > 0 )
			{
				options->SetViewIndex( options->GetTotalNumberOfOptions() - 1 );
				options->SetFocusIndex( options->GetTotalNumberOfOptions() - 1 );
			}
		}

		idSWFTextInstance* privacy = GetSprite()->GetScriptObject()->GetNestedText( "matchInfo", "txtPrivacy" );
		if( privacy != NULL )
		{
			if( isPeer )
			{
				privacy->SetText( "" );
			}
			else
			{

				idMatchParameters matchParameters = session->GetPartyLobbyBase().GetMatchParms();
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
}
