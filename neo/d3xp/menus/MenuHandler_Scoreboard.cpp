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
idMenuHandler_Scoreboard::Update
========================
*/
void idMenuHandler_Scoreboard::Update()
{

	if( gui == NULL || !gui->IsActive() )
	{
		return;
	}

	if( nextScreen != activeScreen )
	{

		if( nextScreen == SCOREBOARD_AREA_INVALID )
		{

			if( activeScreen > SCOREBOARD_AREA_INVALID && activeScreen < SCOREBOARD_NUM_AREAS && menuScreens[ activeScreen ] != NULL )
			{
				menuScreens[ activeScreen ]->HideScreen( static_cast<mainMenuTransition_t>( transition ) );
			}

			idMenuWidget_CommandBar* cmdBar = dynamic_cast< idMenuWidget_CommandBar* >( GetChildFromIndex( SCOREBOARD_WIDGET_CMD_BAR ) );
			if( cmdBar != NULL )
			{
				cmdBar->ClearAllButtons();
				cmdBar->Update();
			}

			idSWFSpriteInstance* bg = gui->GetRootObject().GetNestedSprite( "background" );

			if( bg != NULL )
			{
				bg->PlayFrame( "rollOff" );
			}

		}
		else
		{

			if( activeScreen > SCOREBOARD_AREA_INVALID && activeScreen < SCOREBOARD_NUM_AREAS && menuScreens[ activeScreen ] != NULL )
			{
				menuScreens[ activeScreen ]->HideScreen( static_cast<mainMenuTransition_t>( transition ) );
			}

			if( nextScreen > SCOREBOARD_AREA_INVALID && nextScreen < SCOREBOARD_NUM_AREAS && menuScreens[ nextScreen ] != NULL )
			{
				menuScreens[ nextScreen ]->UpdateCmds();
				menuScreens[ nextScreen ]->ShowScreen( static_cast<mainMenuTransition_t>( transition ) );
			}
		}

		transition = MENU_TRANSITION_INVALID;
		activeScreen = nextScreen;
	}

	idMenuHandler::Update();
}

/*
========================
idMenuHandler_Scoreboard::ActivateMenu
========================
*/
void idMenuHandler_Scoreboard::TriggerMenu()
{
	nextScreen = activationScreen;
}

/*
========================
idMenuHandler_Scoreboard::ActivateMenu
========================
*/
void idMenuHandler_Scoreboard::ActivateMenu( bool show )
{

	idMenuHandler::ActivateMenu( show );

	idPlayer* player = gameLocal.GetLocalPlayer();
	if( player == NULL )
	{
		return;
	}

	if( show )
	{

		idMenuWidget_CommandBar* cmdBar = dynamic_cast< idMenuWidget_CommandBar* >( GetChildFromIndex( SCOREBOARD_WIDGET_CMD_BAR ) );
		if( cmdBar != NULL )
		{
			cmdBar->ClearAllButtons();
			cmdBar->Update();
		}

		nextScreen = SCOREBOARD_AREA_INVALID;
		activeScreen = SCOREBOARD_AREA_INVALID;
	}
	else
	{
		activeScreen = SCOREBOARD_AREA_INVALID;
		nextScreen = SCOREBOARD_AREA_INVALID;
	}


	class idSWFScriptFunction_activateMenu : public idSWFScriptFunction_RefCounted
	{
	public:
		idSWFScriptFunction_activateMenu( idMenuHandler* _handler )
		{
			handler = _handler;
		}
		idSWFScriptVar Call( idSWFScriptObject* thisObject, const idSWFParmList& parms )
		{
			if( handler != NULL )
			{
				handler->TriggerMenu();
			}

			return idSWFScriptVar();
		}
	private:
		idMenuHandler* handler;
	};

	gui->SetGlobal( "activateMenus", new( TAG_SWF ) idSWFScriptFunction_activateMenu( this ) );

}

/*
========================
idMenuHandler_Scoreboard::Initialize
========================
*/
void idMenuHandler_Scoreboard::Initialize( const char* swfFile, idSoundWorld* sw )
{
	idMenuHandler::Initialize( swfFile, sw );

	//---------------------
	// Initialize the menus
	//---------------------
#define BIND_SCOREBOARD_SCREEN( screenId, className, menuHandler )				\
	menuScreens[ (screenId) ] = new className();						\
	menuScreens[ (screenId) ]->Initialize( menuHandler );				\
	menuScreens[ (screenId) ]->AddRef();

	for( int i = 0; i < SCOREBOARD_NUM_AREAS; ++i )
	{
		menuScreens[ i ] = NULL;
	}

	BIND_SCOREBOARD_SCREEN( SCOREBOARD_AREA_DEFAULT, idMenuScreen_Scoreboard, this );
	BIND_SCOREBOARD_SCREEN( SCOREBOARD_AREA_TEAM, idMenuScreen_Scoreboard_Team, this );

	//
	// command bar
	//
	idMenuWidget_CommandBar* const commandBarWidget = new( TAG_SWF ) idMenuWidget_CommandBar();
	commandBarWidget->SetAlignment( idMenuWidget_CommandBar::LEFT );
	commandBarWidget->SetSpritePath( "prompts" );
	commandBarWidget->Initialize( this );

	AddChild( commandBarWidget );

	class idScoreboardGUIClose : public idSWFScriptFunction_RefCounted
	{
	public:
		idSWFScriptVar Call( idSWFScriptObject* thisObject, const idSWFParmList& parms )
		{
			gameLocal.mpGame.SetScoreboardActive( false );
			return idSWFScriptVar();
		}
	};

	if( gui != NULL )
	{
		gui->SetGlobal( "closeScoreboard", new idScoreboardGUIClose() );
	}

	// precache sounds
	// don't load gui music for the pause menu to save some memory
	const idSoundShader* soundShader = NULL;
	soundShader = declManager->FindSound( "gui/list_scroll", true );
	if( soundShader != NULL )
	{
		sounds[ GUI_SOUND_SCROLL ] = soundShader->GetName();
	}
	soundShader = declManager->FindSound( "gui/btn_PDA_advance", true );
	if( soundShader != NULL )
	{
		sounds[ GUI_SOUND_ADVANCE ] = soundShader->GetName();
	}
	soundShader = declManager->FindSound( "gui/btn_PDA_back", true );
	if( soundShader != NULL )
	{
		sounds[ GUI_SOUND_BACK ] = soundShader->GetName();
	}
	soundShader = declManager->FindSound( "gui/pda_next_tab", true );
	if( soundShader != NULL )
	{
		sounds[ GUI_SOUND_BUILD_ON ] = soundShader->GetName();
	}
	soundShader = declManager->FindSound( "gui/pda_prev_tab", true );
	if( soundShader != NULL )
	{
		sounds[ GUI_SOUND_BUILD_OFF ] = soundShader->GetName();
	}
	soundShader = declManager->FindSound( "gui/btn_set_focus", true );
	if( soundShader != NULL )
	{
		sounds[ GUI_SOUND_FOCUS ] = soundShader->GetName();
	}
	soundShader = declManager->FindSound( "gui/btn_roll_over", true );
	if( soundShader != NULL )
	{
		sounds[ GUI_SOUND_ROLL_OVER ] = soundShader->GetName();
	}
	soundShader = declManager->FindSound( "gui/btn_roll_out", true );
	if( soundShader != NULL )
	{
		sounds[ GUI_SOUND_ROLL_OUT ] = soundShader->GetName();
	}
}

/*
========================
idMenuHandler_Scoreboard::GetMenuScreen
========================
*/
idMenuScreen* idMenuHandler_Scoreboard::GetMenuScreen( int index )
{

	if( index < 0 || index >= SCOREBOARD_NUM_AREAS )
	{
		return NULL;
	}

	return menuScreens[ index ];

}

/*
========================
idMenuHandler_Scoreboard::HandleAction
========================
*/
bool idMenuHandler_Scoreboard::HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled )
{

	if( activeScreen == SCOREBOARD_AREA_INVALID )
	{
		return true;
	}

	widgetAction_t actionType = action.GetType();
	//const idSWFParmList & parms = action.GetParms();

	if( event.type == WIDGET_EVENT_COMMAND )
	{
		if( menuScreens[ activeScreen ] != NULL && !forceHandled )
		{
			if( menuScreens[ activeScreen ]->HandleAction( action, event, widget, true ) )
			{
				if( actionType == WIDGET_ACTION_GO_BACK )
				{
					PlaySound( GUI_SOUND_BACK );
				}
				else
				{
					PlaySound( GUI_SOUND_ADVANCE );
				}
				return true;
			}
		}
	}

	return idMenuHandler::HandleAction( action, event, widget, forceHandled );
}

/*
========================
idMenuHandler_Scoreboard::AddPlayerInfo
========================
*/
void idMenuHandler_Scoreboard::AddPlayerInfo( int index, voiceStateDisplay_t voiceState, int team, idStr name, int score, int wins, int ping, idStr spectateData )
{

	scoreboardInfo_t info;
	idList< idStr > values;
	values.Append( name );

	if( spectateData.IsEmpty() || gameLocal.mpGame.GetGameState() == idMultiplayerGame::GAMEREVIEW )
	{
		values.Append( va( "%i", score ) );
	}
	else
	{
		values.Append( spectateData );
	}

	values.Append( va( "%i", wins ) );
	values.Append( va( "%i", ping ) );

	info.index = index;
	info.voiceState = voiceState;
	info.values = values;

	if( team == 1 )
	{
		blueInfo.Append( info );
	}
	else
	{
		redInfo.Append( info );
	}
}

/*
========================
idMenuHandler_Scoreboard::UpdateScoreboard
========================
*/
void idMenuHandler_Scoreboard::UpdateSpectating( idStr spectate, idStr follow )
{

	if( nextScreen == SCOREBOARD_AREA_DEFAULT )
	{
		idMenuScreen_Scoreboard* screen = dynamic_cast< idMenuScreen_Scoreboard* >( menuScreens[ SCOREBOARD_AREA_DEFAULT ] );
		if( screen )
		{
			screen->UpdateSpectating( spectate, follow );
		}
	}
	else if( nextScreen == SCOREBOARD_AREA_TEAM )
	{
		idMenuScreen_Scoreboard* screen = dynamic_cast< idMenuScreen_Scoreboard* >( menuScreens[ SCOREBOARD_AREA_TEAM ] );
		if( screen )
		{
			screen->UpdateSpectating( spectate, follow );
		}
	}
}

/*
========================
idMenuHandler_Scoreboard::UpdateScoreboard
========================
*/
void idMenuHandler_Scoreboard::UpdateScoreboard( idList< mpScoreboardInfo >& data, idStr gameInfo )
{

	bool changed = false;
	if( data.Num() != scoreboardInfo.Num() )
	{
		changed = true;
	}
	else
	{
		for( int i = 0; i < data.Num(); ++i )
		{
			if( data[i] != scoreboardInfo[i] )
			{
				changed = true;
				break;
			}
		}
	}

	if( nextScreen == SCOREBOARD_AREA_DEFAULT )
	{
		idMenuScreen_Scoreboard* screen = dynamic_cast< idMenuScreen_Scoreboard* >( menuScreens[ SCOREBOARD_AREA_DEFAULT ] );
		if( screen )
		{
			screen->UpdateGameInfo( gameInfo );
			screen->UpdateTeamScores( redScore, blueScore );
		}
	}
	else if( nextScreen == SCOREBOARD_AREA_TEAM )
	{
		idMenuScreen_Scoreboard* screen = dynamic_cast< idMenuScreen_Scoreboard* >( menuScreens[ SCOREBOARD_AREA_TEAM ] );
		if( screen )
		{
			screen->UpdateGameInfo( gameInfo );
			screen->UpdateTeamScores( redScore, blueScore );
		}
	}

	redInfo.Clear();
	blueInfo.Clear();
	for( int i = 0; i < data.Num(); ++i )
	{
		AddPlayerInfo( data[i].playerNum, data[i].voiceState, data[i].team, data[i].name, data[i].score, data[i].wins, data[i].ping, data[i].spectateData );
	}

	idList< scoreboardInfo_t, TAG_IDLIB_LIST_MENU > listItemInfo;
	for( int i = 0; i < redInfo.Num(); ++i )
	{
		listItemInfo.Append( redInfo[i] );
	}

	// add empty items to list
	if( blueInfo.Num() > 0 )
	{
		while( listItemInfo.Num() < 4 )
		{
			scoreboardInfo_t info;
			listItemInfo.Append( info );
		}
	}

	for( int i = 0; i < blueInfo.Num(); ++i )
	{
		listItemInfo.Append( blueInfo[i] );
	}

	while( listItemInfo.Num() < 8 )
	{
		scoreboardInfo_t info;
		listItemInfo.Append( info );
	}

	if( nextScreen == SCOREBOARD_AREA_DEFAULT || activationScreen == SCOREBOARD_AREA_DEFAULT )
	{
		idMenuScreen_Scoreboard* screen = dynamic_cast< idMenuScreen_Scoreboard* >( menuScreens[ SCOREBOARD_AREA_DEFAULT ] );
		if( screen )
		{
			screen->SetPlayerData( listItemInfo );
		}
	}
	else if( nextScreen == SCOREBOARD_AREA_TEAM || activationScreen == SCOREBOARD_AREA_TEAM )
	{
		idMenuScreen_Scoreboard* screen = dynamic_cast< idMenuScreen_Scoreboard* >( menuScreens[ SCOREBOARD_AREA_TEAM ] );
		if( screen )
		{
			screen->SetPlayerData( listItemInfo );
		}
	}

	scoreboardInfo = data;
}

/*
========================
idMenuHandler_Scoreboard::SetTeamScore
========================
*/
void idMenuHandler_Scoreboard::SetTeamScores( int r, int b )
{
	redScore = r;
	blueScore = b;
}

/*
========================
idMenuHandler_Scoreboard::GetNumPlayers
========================
*/
int idMenuHandler_Scoreboard::GetNumPlayers( int team )
{

	if( team == 1 )
	{
		return blueInfo.Num();
	}
	else
	{
		return redInfo.Num();
	}

}

/*
========================
idMenuHandler_Scoreboard::SetActivationScreen
========================
*/
void idMenuHandler_Scoreboard::SetActivationScreen( int screen, int trans )
{
	activationScreen = screen;
	transition = trans;
}

/*
========================
idMenuHandler_Scoreboard::GetUserID
========================
*/
void idMenuHandler_Scoreboard::GetUserID( int slot, lobbyUserID_t& luid )
{
	idList< int > redList;
	idList< int > blueList;

	for( int i = 0; i < scoreboardInfo.Num(); ++i )
	{
		if( scoreboardInfo[i].team == 1 )
		{
			blueList.Append( scoreboardInfo[i].playerNum );
		}
		else
		{
			redList.Append( scoreboardInfo[i].playerNum );
		}
	}

	idList< int > displayList;

	for( int i = 0; i < redList.Num(); ++i )
	{
		displayList.Append( redList[ i ] );
	}

	for( int i = 0; i < blueList.Num(); ++i )
	{
		displayList.Append( blueList[ i ] );
	}

	if( slot >= displayList.Num() )
	{
		return;
	}

	luid = gameLocal.lobbyUserIDs[ displayList[ slot ] ];
}

/*
========================
idMenuHandler_Scoreboard::ViewPlayerProfile
========================
*/
void idMenuHandler_Scoreboard::ViewPlayerProfile( int slot )
{

	lobbyUserID_t luid;
	GetUserID( slot, luid );
	if( luid.IsValid() )
	{
		session->ShowLobbyUserGamerCardUI( luid );
	}
}

/*
========================
idMenuHandler_Scoreboard::MutePlayer
========================
*/
void idMenuHandler_Scoreboard::MutePlayer( int slot )
{

	lobbyUserID_t luid;
	GetUserID( slot, luid );
	if( luid.IsValid() )
	{
		session->ToggleLobbyUserVoiceMute( luid );
	}
}

/*
========================
idMenuHandler_Scoreboard::UpdateScoreboardSelection
========================
*/
void idMenuHandler_Scoreboard::UpdateScoreboardSelection()
{

	if( nextScreen == SCOREBOARD_AREA_DEFAULT || activationScreen == SCOREBOARD_AREA_DEFAULT )
	{
		idMenuScreen_Scoreboard* screen = dynamic_cast< idMenuScreen_Scoreboard* >( menuScreens[ SCOREBOARD_AREA_DEFAULT ] );
		if( screen )
		{
			screen->UpdateHighlight();
		}
	}
	else if( nextScreen == SCOREBOARD_AREA_TEAM || activationScreen == SCOREBOARD_AREA_TEAM )
	{
		idMenuScreen_Scoreboard* screen = dynamic_cast< idMenuScreen_Scoreboard* >( menuScreens[ SCOREBOARD_AREA_TEAM ] );
		if( screen )
		{
			screen->UpdateHighlight();
		}
	}
}