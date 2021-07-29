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

enum browserCommand_t
{
	BROWSER_COMMAND_REFRESH_SERVERS,
	BROWSER_COMMAND_SHOW_GAMERTAG,
};

static const int NUM_SERVER_LIST_ITEMS = 10;

/*
================================================
idPair is is a template class Container composed of two objects, which can be of
any type, and provides accessors to these objects as well as Pair equality operators. The main
uses of Pairs in the engine are for the Tools and for callbacks.
================================================
*/
template<class T, class U>
class idPair
{
public:
	idPair() { }
	idPair( const T& f, const U& s ) : first( f ), second( s ) { }

	const bool	operator==( const idPair<T, U>& rhs ) const
	{
		return ( rhs.first == first ) && ( rhs.second == second );
	}

	const bool	operator!=( const idPair<T, U>& rhs ) const
	{
		return !( *this == rhs );
	}

	T first;
	U second;
};

/*
================================================
idSort_PlayerGamesList
================================================
*/
class idSort_PlayerGamesList : public idSort_Quick< idPair< serverInfo_t, int >, idSort_PlayerGamesList >
{
public:
	int Compare( const idPair< serverInfo_t, int >& a, const idPair< serverInfo_t, int >& b ) const
	{
		if( a.first.joinable == b.first.joinable )
		{
			return a.first.gameMode - b.first.gameMode;
		}
		else if( a.first.joinable )
		{
			return 1;
		}
		else
		{
			return -1;
		}
	}
};

/*
========================
idMenuScreen_Shell_GameBrowser::Initialize
========================
*/
void idMenuScreen_Shell_GameBrowser::Initialize( idMenuHandler* data )
{
	idMenuScreen::Initialize( data );

	if( data != NULL )
	{
		menuGUI = data->GetGUI();
	}

	SetSpritePath( "menuPWF" );

	listWidget = new idMenuWidget_GameBrowserList();
	listWidget->SetSpritePath( GetSpritePath(), "info", "options" );
	listWidget->SetNumVisibleOptions( NUM_SERVER_LIST_ITEMS );
	listWidget->SetWrappingAllowed( true );
	listWidget->AddEventAction( WIDGET_EVENT_SCROLL_DOWN ).Set( WIDGET_ACTION_START_REPEATER, WIDGET_ACTION_SCROLL_VERTICAL, 1 );
	listWidget->AddEventAction( WIDGET_EVENT_SCROLL_UP ).Set( WIDGET_ACTION_START_REPEATER, WIDGET_ACTION_SCROLL_VERTICAL, -1 );
	listWidget->AddEventAction( WIDGET_EVENT_SCROLL_DOWN_RELEASE ).Set( WIDGET_ACTION_STOP_REPEATER );
	listWidget->AddEventAction( WIDGET_EVENT_SCROLL_UP_RELEASE ).Set( WIDGET_ACTION_STOP_REPEATER );
	listWidget->AddEventAction( WIDGET_EVENT_SCROLL_DOWN_LSTICK ).Set( WIDGET_ACTION_START_REPEATER, WIDGET_ACTION_SCROLL_VERTICAL, 1 );
	listWidget->AddEventAction( WIDGET_EVENT_SCROLL_UP_LSTICK ).Set( WIDGET_ACTION_START_REPEATER, WIDGET_ACTION_SCROLL_VERTICAL, -1 );
	listWidget->AddEventAction( WIDGET_EVENT_SCROLL_DOWN_LSTICK_RELEASE ).Set( WIDGET_ACTION_STOP_REPEATER );
	listWidget->AddEventAction( WIDGET_EVENT_SCROLL_UP_LSTICK_RELEASE ).Set( WIDGET_ACTION_STOP_REPEATER );
	AddChild( listWidget );

	idMenuWidget_Help* const helpWidget = new( TAG_SWF ) idMenuWidget_Help();
	helpWidget->SetSpritePath( GetSpritePath(), "info", "helpTooltip" );
	AddChild( helpWidget );

	while( listWidget->GetChildren().Num() < NUM_SERVER_LIST_ITEMS )
	{
		idMenuWidget_ServerButton* buttonWidget = new idMenuWidget_ServerButton;
		buttonWidget->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_PRESS_FOCUSED, listWidget->GetChildren().Num() );
		buttonWidget->SetState( WIDGET_STATE_HIDDEN );
		buttonWidget->RegisterEventObserver( helpWidget );
		listWidget->AddChild( buttonWidget );
	}

	btnBack = new( TAG_SWF ) idMenuWidget_Button();
	btnBack->Initialize( data );
	btnBack->SetLabel( "#str_swf_multiplayer" );
	btnBack->SetSpritePath( GetSpritePath(), "info", "btnBack" );
	btnBack->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_GO_BACK );
	AddChild( btnBack );
}

/*
========================
idMenuScreen_Shell_GameBrowser::ShowScreen
========================
*/
void idMenuScreen_Shell_GameBrowser::ShowScreen( const mainMenuTransition_t transitionType )
{
	idMenuHandler_Shell* const mgr = dynamic_cast< idMenuHandler_Shell* >( menuData );
	if( mgr == NULL )
	{
		return;
	}

	idSWFScriptObject& root = GetSWFObject()->GetRootObject();
	if( BindSprite( root ) )
	{
		idSWFTextInstance* heading = GetSprite()->GetScriptObject()->GetNestedText( "info", "txtHeading" );
		if( heading != NULL )
		{
			heading->SetText( "#str_swf_pwf_heading" );	// MULTIPLAYER
			heading->SetStrokeInfo( true, 0.75f, 1.75f );
		}

		idSWFSpriteInstance* gradient = GetSprite()->GetScriptObject()->GetNestedSprite( "info", "gradient" );
		if( gradient != NULL && heading != NULL )
		{
			gradient->SetXPos( heading->GetTextLength() );
		}
	}

	listWidget->ClearGames();

	if( mgr->GetCmdBar() != NULL )
	{
		idMenuWidget_CommandBar::buttonInfo_t* buttonInfo;

		mgr->GetCmdBar()->ClearAllButtons();

		buttonInfo = mgr->GetCmdBar()->GetButton( idMenuWidget_CommandBar::BUTTON_JOY2 );
		if( menuData->GetPlatform() != 2 )
		{
			buttonInfo->label = "#str_00395";
		}
		buttonInfo->action.Set( WIDGET_ACTION_GO_BACK );

		buttonInfo = mgr->GetCmdBar()->GetButton( idMenuWidget_CommandBar::BUTTON_JOY3 );
		buttonInfo->label = "#str_02276";
		buttonInfo->action.Set( WIDGET_ACTION_COMMAND, BROWSER_COMMAND_REFRESH_SERVERS );
	}

	mgr->HidePacifier();

	idMenuScreen::ShowScreen( transitionType );
	UpdateServerList();
}

/*
========================
idMenuScreen_Shell_GameBrowser::HideScreen
========================
*/
void idMenuScreen_Shell_GameBrowser::HideScreen( const mainMenuTransition_t transitionType )
{
	idMenuHandler_Shell* const mgr = dynamic_cast< idMenuHandler_Shell* >( menuData );
	if( mgr == NULL )
	{
		return;
	}

	mgr->HidePacifier();

	session->CancelListServers();

	idMenuScreen::HideScreen( transitionType );
}

/*
========================
idMenuScreen_Shell_GameBrowser::UpdateServerList
========================
*/
void idMenuScreen_Shell_GameBrowser::UpdateServerList()
{
	idMenuHandler_Shell* const mgr = dynamic_cast< idMenuHandler_Shell* >( menuData );

	if( mgr == NULL )
	{
		return;
	}

	for( int i = 0; i < listWidget->GetChildren().Num(); ++i )
	{
		idMenuWidget& child = listWidget->GetChildByIndex( i );
		child.SetState( WIDGET_STATE_HIDDEN );
	}

	// need to show the pacifier before actually making the ListServers call, because it can fail
	// immediately and send back an error result to SWF. Things get confused if the showLoadingPacifier
	// then gets called after that.
	mgr->ShowPacifier( "#str_online_mpstatus_searching" );

	session->ListServers( MakeCallback( this, &idMenuScreen_Shell_GameBrowser::OnServerListReady ) );
}

/*
========================
idMenuScreen_Shell_GameBrowser::OnServerListReady
========================
*/
void idMenuScreen_Shell_GameBrowser::OnServerListReady()
{
	idMenuHandler_Shell* const mgr = dynamic_cast< idMenuHandler_Shell* >( menuData );

	if( mgr == NULL )
	{
		return;
	}

	mgr->HidePacifier();

	idList< idPair< serverInfo_t, int > > servers;
	for( int i = 0; i < session->NumServers(); ++i )
	{
		const serverInfo_t* const server = session->ServerInfo( i );
		if( server != NULL && server->joinable )
		{
			idPair< serverInfo_t, int >& serverPair = servers.Alloc();
			serverPair.first = *server;
			serverPair.second = i;
		}
	}

	servers.SortWithTemplate( idSort_PlayerGamesList() );

	listWidget->ClearGames();
	for( int i = 0; i < servers.Num(); ++i )
	{
		idPair< serverInfo_t, int >& serverPair = servers[ i ];
		DescribeServer( serverPair.first, serverPair.second );
	}

	if( servers.Num() > 0 )
	{
		listWidget->Update();
		listWidget->SetViewOffset( 0 );
		listWidget->SetViewIndex( 0 );
		listWidget->SetFocusIndex( 0 );
	}
	else
	{
		listWidget->AddGame( "#str_swf_no_servers_found", idStrId(), idStr(), -1, 0, 0, false, false );
		listWidget->Update();
		listWidget->SetViewOffset( 0 );
		listWidget->SetViewIndex( 0 );
		listWidget->SetFocusIndex( 0 );
	}

	if( mgr->GetCmdBar() != NULL )
	{
		idMenuWidget_CommandBar::buttonInfo_t* buttonInfo;

		mgr->GetCmdBar()->ClearAllButtons();

		if( servers.Num() > 0 )
		{
			buttonInfo = mgr->GetCmdBar()->GetButton( idMenuWidget_CommandBar::BUTTON_JOY1 );
			if( menuData->GetPlatform() != 2 )
			{
				buttonInfo->label = "#STR_SWF_SELECT";
			}
			buttonInfo->action.Set( WIDGET_ACTION_PRESS_FOCUSED );
		}

		buttonInfo = mgr->GetCmdBar()->GetButton( idMenuWidget_CommandBar::BUTTON_JOY2 );
		if( menuData->GetPlatform() != 2 )
		{
			buttonInfo->label = "#str_00395";
		}
		buttonInfo->action.Set( WIDGET_ACTION_GO_BACK );

		buttonInfo = mgr->GetCmdBar()->GetButton( idMenuWidget_CommandBar::BUTTON_JOY3 );
		buttonInfo->label = "#str_02276";
		buttonInfo->action.Set( WIDGET_ACTION_COMMAND, BROWSER_COMMAND_REFRESH_SERVERS );

		if( servers.Num() > 0 )
		{
			buttonInfo = mgr->GetCmdBar()->GetButton( idMenuWidget_CommandBar::BUTTON_JOY4 );
			buttonInfo->label = "#str_swf_view_profile";
			buttonInfo->action.Set( WIDGET_ACTION_COMMAND, BROWSER_COMMAND_SHOW_GAMERTAG );
		}

		mgr->GetCmdBar()->Update();
	}
}

/*
========================
idMenuScreen_Shell_GameBrowser::DescribeServers
========================
*/
void idMenuScreen_Shell_GameBrowser::DescribeServer( const serverInfo_t& server, const int index )
{

	idStr serverName;
	int serverIndex = index;
	bool joinable = false;
	bool validMap = false;
	int players = 0;
	int maxPlayers = 0;
	idStrId mapName;
	idStr modeName;

	const idList< mpMap_t > maps = common->GetMapList();
	const bool isMapValid = ( server.gameMap >= 0 ) && ( server.gameMap < maps.Num() );
	if( !isMapValid )
	{
		validMap = false;
		serverName = server.serverName;
		mapName = "#str_online_in_lobby";
		modeName = "";
		players = server.numPlayers;
		maxPlayers = server.maxPlayers;
		joinable = server.joinable;
	}
	else
	{
		mapName = common->GetMapList()[ server.gameMap ].mapName;

		const idStrList& modes = common->GetModeDisplayList();
		idStr mode = idLocalization::GetString( modes[ idMath::ClampInt( 0, modes.Num() - 1, server.gameMode ) ] );
		validMap = true;
		serverName = server.serverName;
		modeName = mode;
		players = server.numPlayers;
		maxPlayers = server.maxPlayers;
		joinable = server.joinable;
	}

	listWidget->AddGame( serverName, mapName, modeName, serverIndex, players, maxPlayers, joinable, validMap );
}

/*
========================
idMenuScreen_Shell_GameBrowser::HandleAction h
========================
*/
bool idMenuScreen_Shell_GameBrowser::HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandle )
{
	idMenuHandler_Shell* const mgr = dynamic_cast< idMenuHandler_Shell* >( menuData );

	if( mgr == NULL )
	{
		return false;
	}

	if( mgr->ActiveScreen() != SHELL_AREA_BROWSER )
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
		case WIDGET_ACTION_COMMAND:
		{
			switch( parms[ 0 ].ToInteger() )
			{
				case BROWSER_COMMAND_REFRESH_SERVERS:
				{
					UpdateServerList();
					break;
				}
				case BROWSER_COMMAND_SHOW_GAMERTAG:
				{
					int index = listWidget->GetServerIndex();
					if( index != -1 )
					{
						session->ShowServerGamerCardUI( index );
					}
					break;
				}
			}
			return true;
		}
		case WIDGET_ACTION_PRESS_FOCUSED:
		{
			int selectionIndex = listWidget->GetFocusIndex();
			if( parms.Num() > 0 )
			{
				selectionIndex = parms[0].ToInteger();
			}

			if( selectionIndex != listWidget->GetFocusIndex() )
			{
				listWidget->SetViewIndex( listWidget->GetViewOffset() + selectionIndex );
				listWidget->SetFocusIndex( selectionIndex );
				return true;
			}

			int index = listWidget->GetServerIndex();
			if( index != -1 )
			{
				session->ConnectToServer( index );
			}
			return true;
		}
	}

	return idMenuScreen::HandleAction( action, event, widget, forceHandle );
}
