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

//***************************************************************
// DEFAULT SCOREBOARD
//***************************************************************
static const int MAX_SCOREBOARD_SLOTS = 8;

/*
========================
idMenuScreen_Scoreboard::Initialize
========================
*/
void idMenuScreen_Scoreboard::Initialize( idMenuHandler* data )
{
	idMenuScreen::Initialize( data );

	if( data != NULL )
	{
		menuGUI = data->GetGUI();
	}

	SetSpritePath( "sbDefault" );

	playerList = new( TAG_SWF ) idMenuWidget_ScoreboardList();
	playerList->SetSpritePath( GetSpritePath(), "info", "playerList" );
	playerList->SetNumVisibleOptions( MAX_SCOREBOARD_SLOTS );
	playerList->SetWrappingAllowed( true );
	while( playerList->GetChildren().Num() < MAX_SCOREBOARD_SLOTS )
	{
		idMenuWidget_ScoreboardButton* const buttonWidget = new( TAG_SWF ) idMenuWidget_ScoreboardButton();
		buttonWidget->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_PRESS_FOCUSED, playerList->GetChildren().Num() );
		buttonWidget->AddEventAction( WIDGET_EVENT_COMMAND ).Set( WIDGET_ACTION_MUTE_PLAYER, playerList->GetChildren().Num() );
		buttonWidget->Initialize( data );
		playerList->AddChild( buttonWidget );
	}
	playerList->Initialize( data );
	AddChild( playerList );

	AddEventAction( WIDGET_EVENT_SCROLL_DOWN_LSTICK ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_SCROLL_DOWN_START_REPEATER_VARIABLE, WIDGET_EVENT_SCROLL_DOWN_LSTICK ) );
	AddEventAction( WIDGET_EVENT_SCROLL_UP_LSTICK ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_SCROLL_UP_START_REPEATER_VARIABLE, WIDGET_EVENT_SCROLL_UP_LSTICK ) );
	AddEventAction( WIDGET_EVENT_SCROLL_DOWN_LSTICK_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_DOWN_LSTICK_RELEASE ) );
	AddEventAction( WIDGET_EVENT_SCROLL_UP_LSTICK_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_UP_LSTICK_RELEASE ) );
	AddEventAction( WIDGET_EVENT_SCROLL_DOWN ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_SCROLL_DOWN_START_REPEATER_VARIABLE, WIDGET_EVENT_SCROLL_DOWN ) );
	AddEventAction( WIDGET_EVENT_SCROLL_UP ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_SCROLL_UP_START_REPEATER_VARIABLE, WIDGET_EVENT_SCROLL_UP ) );
	AddEventAction( WIDGET_EVENT_SCROLL_DOWN_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_DOWN_RELEASE ) );
	AddEventAction( WIDGET_EVENT_SCROLL_UP_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_UP_RELEASE ) );
}

/*
========================
idMenuScreen_Scoreboard::Update
========================
*/
void idMenuScreen_Scoreboard::Update()
{

	if( menuData != NULL )
	{
		idMenuWidget_CommandBar* cmdBar = dynamic_cast< idMenuWidget_CommandBar* const >( menuData->GetChildFromIndex( SCOREBOARD_WIDGET_CMD_BAR ) );
		if( cmdBar != NULL )
		{
			cmdBar->ClearAllButtons();
			idMenuWidget_CommandBar::buttonInfo_t* buttonInfo;

			if( gameLocal.mpGame.GetGameState() != idMultiplayerGame::GAMEREVIEW )
			{
				buttonInfo = cmdBar->GetButton( idMenuWidget_CommandBar::BUTTON_JOY2 );
				if( menuData->GetPlatform() != 2 )
				{
					buttonInfo->label = "#str_01345";
				}
				buttonInfo->action.Set( WIDGET_ACTION_GO_BACK );

				buttonInfo = cmdBar->GetButton( idMenuWidget_CommandBar::BUTTON_TAB );
				buttonInfo->label = "";
				buttonInfo->action.Set( WIDGET_ACTION_GO_BACK );
			}

			buttonInfo = cmdBar->GetButton( idMenuWidget_CommandBar::BUTTON_JOY1 );
			if( menuData->GetPlatform() != 2 )
			{
				buttonInfo->label = "#str_swf_view_profile";
			}
			buttonInfo->action.Set( WIDGET_ACTION_PRESS_FOCUSED );
		}
	}

	idMenuScreen::Update();
}

/*
========================
idMenuScreen_Scoreboard::ShowScreen
========================
*/
void idMenuScreen_Scoreboard::ShowScreen( const mainMenuTransition_t transitionType )
{
	idMenuScreen::ShowScreen( transitionType );

	if( GetSWFObject() )
	{
		idSWFScriptObject& root = GetSWFObject()->GetRootObject();
		if( BindSprite( root ) )
		{

			idSWFTextInstance* txtVal = GetSprite()->GetScriptObject()->GetNestedText( "info", "txtScoreboard" );
			if( txtVal != NULL )
			{
				txtVal->SetText( "#str_02618" );
				txtVal->SetStrokeInfo( true, 0.9f, 2.0f );
			}

			txtVal = GetSprite()->GetScriptObject()->GetNestedText( "info", "txtGameType" );
			if( txtVal != NULL )
			{
				idStr mode = idLocalization::GetString( "#str_02376" );
				mode.Append( ": " );
				const idStrList& modes = common->GetModeDisplayList();
				idStr modeName = idLocalization::GetString( modes[ idMath::ClampInt( 0, modes.Num() - 1, gameLocal.gameType ) ] );
				mode.Append( idLocalization::GetString( idLocalization::GetString( modeName ) ) );
				txtVal->SetText( mode );
				txtVal->SetStrokeInfo( true, 0.9f, 1.8f );
			}

			txtVal = GetSprite()->GetScriptObject()->GetNestedText( "info", "txtNameHeading" );
			if( txtVal != NULL )
			{
				txtVal->SetText( "#str_02181" );
				txtVal->SetStrokeInfo( true, 0.75f, 1.75f );
			}
			txtVal = GetSprite()->GetScriptObject()->GetNestedText( "info", "txtScore" );
			if( txtVal != NULL )
			{
				if( gameLocal.gameType == GAME_LASTMAN )
				{
					txtVal->SetText( idLocalization::GetString( "#str_04242" ) );
				}
				else if( gameLocal.gameType == GAME_CTF )
				{
					txtVal->SetText( idLocalization::GetString( "#str_11112" ) );
				}
				else
				{
					txtVal->SetText( idLocalization::GetString( "#str_04243" ) );
				}
				txtVal->SetStrokeInfo( true, 0.75f, 1.75f );
			}
			txtVal = GetSprite()->GetScriptObject()->GetNestedText( "info", "txtWins" );
			if( txtVal != NULL )
			{
				txtVal->SetText( "#str_02619" );
				txtVal->SetStrokeInfo( true, 0.75f, 1.75f );
			}
			txtVal = GetSprite()->GetScriptObject()->GetNestedText( "info", "txtPing" );
			if( txtVal != NULL )
			{
				txtVal->SetText( "#str_02048" );
				txtVal->SetStrokeInfo( true, 0.75f, 1.75f );
			}

			txtVal = GetSprite()->GetScriptObject()->GetNestedText( "info", "txtNameHeading2" );
			if( txtVal != NULL )
			{
				txtVal->SetText( "#str_02181" );
				txtVal->SetStrokeInfo( true, 0.75f, 1.75f );
			}
			txtVal = GetSprite()->GetScriptObject()->GetNestedText( "info", "txtScore2" );
			if( txtVal != NULL )
			{
				if( gameLocal.gameType == GAME_LASTMAN )
				{
					txtVal->SetText( idLocalization::GetString( "#str_04242" ) );
				}
				else if( gameLocal.gameType == GAME_CTF )
				{
					txtVal->SetText( idLocalization::GetString( "#str_11112" ) );
				}
				else
				{
					txtVal->SetText( idLocalization::GetString( "#str_04243" ) );
				}
				txtVal->SetStrokeInfo( true, 0.75f, 1.75f );
			}
			txtVal = GetSprite()->GetScriptObject()->GetNestedText( "info", "txtWins2" );
			if( txtVal != NULL )
			{
				txtVal->SetText( "#str_02619" );
				txtVal->SetStrokeInfo( true, 0.75f, 1.75f );
			}
			txtVal = GetSprite()->GetScriptObject()->GetNestedText( "info", "txtPing2" );
			if( txtVal != NULL )
			{
				txtVal->SetText( "#str_02048" );
				txtVal->SetStrokeInfo( true, 0.75f, 1.75f );
			}

		}
	}

}

/*
========================
idMenuScreen_Scoreboard::SetPlayerData
========================
*/
void idMenuScreen_Scoreboard::SetPlayerData( idList< scoreboardInfo_t, TAG_IDLIB_LIST_MENU > data )
{
	if( playerList != NULL )
	{
		for( int i = 0; i < data.Num(); ++i )
		{
			if( i < playerList->GetChildren().Num() )
			{
				idMenuWidget_ScoreboardButton* button = dynamic_cast< idMenuWidget_ScoreboardButton* >( &playerList->GetChildByIndex( i ) );
				if( button != NULL )
				{
					button->SetButtonInfo( data[i].index, data[i].values, data[i].voiceState );
				}
			}
			playerList->Update();
		}
	}
}

/*
========================
idMenuScreen_Scoreboard::UpdateGameInfo
========================
*/
void idMenuScreen_Scoreboard::UpdateGameInfo( idStr gameInfo )
{

	if( GetSWFObject() )
	{
		idSWFScriptObject& root = GetSWFObject()->GetRootObject();
		if( BindSprite( root ) )
		{

			idSWFTextInstance* txtVal = GetSprite()->GetScriptObject()->GetNestedText( "info", "txtGameInfo" );
			if( txtVal != NULL )
			{
				txtVal->SetText( gameInfo );
				txtVal->SetStrokeInfo( true, 0.75f, 1.75f );
			}
		}
	}
}

/*
========================
idMenuScreen_Scoreboard::UpdateSpectating
========================
*/
void idMenuScreen_Scoreboard::UpdateSpectating( idStr spectating, idStr follow )
{

	if( GetSWFObject() )
	{
		idSWFScriptObject& root = GetSWFObject()->GetRootObject();
		if( BindSprite( root ) )
		{

			idSWFTextInstance* txtVal = GetSprite()->GetScriptObject()->GetNestedText( "info", "txtSpectating" );
			if( txtVal != NULL )
			{
				txtVal->tooltip = true;
				txtVal->SetText( spectating );
				txtVal->SetStrokeInfo( true, 0.75f, 1.75f );
			}

			txtVal = GetSprite()->GetScriptObject()->GetNestedText( "info", "txtFollow" );
			if( txtVal != NULL )
			{
				txtVal->SetText( follow );
				txtVal->SetStrokeInfo( true, 0.75f, 1.75f );
			}
		}
	}
}

/*
========================
idMenuScreen_Scoreboard::UpdateTeamScores
========================
*/
void idMenuScreen_Scoreboard::UpdateTeamScores( int r, int b )
{

	if( GetSWFObject() )
	{
		idSWFScriptObject& root = GetSWFObject()->GetRootObject();
		if( BindSprite( root ) )
		{

			idSWFTextInstance* txtVal = GetSprite()->GetScriptObject()->GetNestedText( "info", "txtRedScore" );
			if( txtVal != NULL )
			{
				txtVal->SetText( va( "%i", r ) );
				txtVal->SetStrokeInfo( true, 0.75f, 1.75f );
			}

			txtVal = GetSprite()->GetScriptObject()->GetNestedText( "info", "txtBlueScore" );
			if( txtVal != NULL )
			{
				txtVal->SetText( va( "%i", b ) );
				txtVal->SetStrokeInfo( true, 0.75f, 1.75f );
			}
		}
	}
}

/*
========================
idMenuScreen_Scoreboard::UpdateHighlight
========================
*/
void idMenuScreen_Scoreboard::UpdateHighlight()
{

	if( playerList == NULL || menuData == NULL )
	{
		return;
	}

	idMenuHandler_Scoreboard* data = dynamic_cast< idMenuHandler_Scoreboard* >( menuData );

	int curIndex = playerList->GetViewIndex();
	int newIndex = playerList->GetViewIndex();
	int numRed = data->GetNumPlayers( 0 );
	int numBlue = data->GetNumPlayers( 1 );

	if( numBlue == 0 )
	{
		if( curIndex >= numRed )
		{
			newIndex = numRed - 1;
		}
	}
	else
	{
		if( curIndex > 3 + numBlue )
		{
			newIndex = 3 + numBlue;
		}
		else if( curIndex <= 3 )
		{
			if( numRed == 0 )
			{
				newIndex = 4;
			}
			else
			{
				if( curIndex >= numRed )
				{
					newIndex = numRed - 1;
				}
			}
		}
	}

	// newIndex can be -1 if all players are spectating ( no rankedplayers )
	if( newIndex != curIndex && newIndex != -1 )
	{
		playerList->SetViewIndex( newIndex );
		playerList->SetFocusIndex( newIndex );
	}
}

/*
========================
idMenuScreen_Scoreboard::HandleAction
========================
*/
bool idMenuScreen_Scoreboard::HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled )
{

	if( menuData == NULL )
	{
		return true;
	}

	widgetAction_t actionType = action.GetType();
	const idSWFParmList& parms = action.GetParms();

	switch( actionType )
	{
		case WIDGET_ACTION_GO_BACK:
		{
			menuData->SetNextScreen( SCOREBOARD_AREA_INVALID, MENU_TRANSITION_SIMPLE );
			return true;
		}
		case WIDGET_ACTION_MUTE_PLAYER:
		{

			if( parms.Num() != 1 )
			{
				return true;
			}

			idMenuHandler_Scoreboard* data = dynamic_cast< idMenuHandler_Scoreboard* >( menuData );

			if( !data )
			{
				return true;
			}
			int index = parms[0].ToInteger();
			data->MutePlayer( index );

			return true;
		}
		case WIDGET_ACTION_PRESS_FOCUSED:
		{

			if( playerList == NULL )
			{
				return true;
			}

			int selectionIndex = playerList->GetViewIndex();
			if( parms.Num() == 1 )
			{
				selectionIndex = parms[0].ToInteger();
			}

			if( selectionIndex != playerList->GetFocusIndex() )
			{
				playerList->SetViewIndex( playerList->GetViewOffset() + selectionIndex );
				playerList->SetFocusIndex( selectionIndex );
			}

			idMenuHandler_Scoreboard* data = dynamic_cast< idMenuHandler_Scoreboard* >( menuData );

			if( !data )
			{
				return true;
			}

			int numRed = data->GetNumPlayers( 0 );
			int numBlue = data->GetNumPlayers( 1 );

			if( selectionIndex >= 4 && numBlue != 0 )
			{
				int index = numRed + ( selectionIndex - 4 );
				data->ViewPlayerProfile( index );
			}
			else
			{
				data->ViewPlayerProfile( selectionIndex );
			}


			return true;
		}
		case WIDGET_ACTION_SCROLL_VERTICAL_VARIABLE:
		{

			if( parms.Num() == 0 )
			{
				return true;
			}

			if( playerList )
			{

				int dir = parms[ 0 ].ToInteger();
				int scroll = 0;
				int curIndex = playerList->GetFocusIndex();

				idMenuHandler_Scoreboard* data = dynamic_cast< idMenuHandler_Scoreboard* >( menuData );

				if( !data )
				{
					return true;
				}

				int numRed = data->GetNumPlayers( 0 );
				int numBlue = data->GetNumPlayers( 1 );

				if( numRed + numBlue <= 1 )
				{
					return true;
				}

				if( dir > 0 )
				{
					if( numBlue == 0 )
					{
						if( curIndex + 1 >= numRed )
						{
							scroll = MAX_SCOREBOARD_SLOTS - curIndex;
						}
						else
						{
							scroll = dir;
						}
					}
					else
					{
						if( curIndex < 4 )
						{
							if( curIndex + 1 >= numRed )
							{
								scroll = ( MAX_SCOREBOARD_SLOTS * 0.5f ) - curIndex;
							}
							else
							{
								scroll = dir;
							}
						}
						else
						{
							if( curIndex - 3 >= numBlue )
							{
								scroll = MAX_SCOREBOARD_SLOTS - curIndex;
							}
							else
							{
								scroll = dir;
							}
						}
					}
				}
				else if( dir < 0 )
				{
					if( numBlue == 0 )
					{
						if( curIndex - 1 < 0 )
						{
							scroll = numRed - 1;
						}
						else
						{
							scroll = dir;
						}
					}
					else
					{
						if( curIndex < 4 )
						{
							if( curIndex - 1 < 0 )
							{
								scroll = ( ( MAX_SCOREBOARD_SLOTS * 0.5f ) + numBlue ) - 1;
							}
							else
							{
								scroll = dir;
							}
						}
						else
						{
							if( curIndex - 1 < 4 )
							{
								scroll = -( ( MAX_SCOREBOARD_SLOTS * 0.5f ) - ( numRed - 1 ) );
							}
							else
							{
								scroll = dir;
							}
						}
					}
				}

				if( scroll != 0 )
				{
					playerList->Scroll( scroll );
				}
			}
			return true;
		}
	}

	return idMenuWidget::HandleAction( action, event, widget, forceHandled );
}

//***************************************************************
// CTF SCOREBOARD
//***************************************************************

/*
========================
idMenuScreen_Scoreboard_CTF::Initialize
========================
*/
void idMenuScreen_Scoreboard_CTF::Initialize( idMenuHandler* data )
{
	idMenuScreen::Initialize( data );

	if( data != NULL )
	{
		menuGUI = data->GetGUI();
	}

	SetSpritePath( "sbCTF" );

	playerList = new( TAG_SWF ) idMenuWidget_ScoreboardList();
	playerList->SetSpritePath( GetSpritePath(), "info", "playerList" );
	playerList->SetNumVisibleOptions( MAX_SCOREBOARD_SLOTS );
	playerList->SetWrappingAllowed( true );
	while( playerList->GetChildren().Num() < MAX_SCOREBOARD_SLOTS )
	{
		idMenuWidget_ScoreboardButton* const buttonWidget = new( TAG_SWF ) idMenuWidget_ScoreboardButton();
		buttonWidget->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_PRESS_FOCUSED, playerList->GetChildren().Num() );
		buttonWidget->AddEventAction( WIDGET_EVENT_COMMAND ).Set( WIDGET_ACTION_MUTE_PLAYER, playerList->GetChildren().Num() );
		buttonWidget->Initialize( data );
		playerList->AddChild( buttonWidget );
	}
	playerList->Initialize( data );
	AddChild( playerList );

	AddEventAction( WIDGET_EVENT_SCROLL_DOWN_LSTICK ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_SCROLL_DOWN_START_REPEATER_VARIABLE, WIDGET_EVENT_SCROLL_DOWN_LSTICK ) );
	AddEventAction( WIDGET_EVENT_SCROLL_UP_LSTICK ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_SCROLL_UP_START_REPEATER_VARIABLE, WIDGET_EVENT_SCROLL_UP_LSTICK ) );
	AddEventAction( WIDGET_EVENT_SCROLL_DOWN_LSTICK_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_DOWN_LSTICK_RELEASE ) );
	AddEventAction( WIDGET_EVENT_SCROLL_UP_LSTICK_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_UP_LSTICK_RELEASE ) );
	AddEventAction( WIDGET_EVENT_SCROLL_DOWN ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_SCROLL_DOWN_START_REPEATER_VARIABLE, WIDGET_EVENT_SCROLL_DOWN ) );
	AddEventAction( WIDGET_EVENT_SCROLL_UP ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_SCROLL_UP_START_REPEATER_VARIABLE, WIDGET_EVENT_SCROLL_UP ) );
	AddEventAction( WIDGET_EVENT_SCROLL_DOWN_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_DOWN_RELEASE ) );
	AddEventAction( WIDGET_EVENT_SCROLL_UP_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_UP_RELEASE ) );

}

//***************************************************************
// TEAM SCOREBOARD
//***************************************************************

/*
========================
idMenuScreen_Scoreboard_Team::Initialize
========================
*/
void idMenuScreen_Scoreboard_Team::Initialize( idMenuHandler* data )
{
	idMenuScreen::Initialize( data );

	if( data != NULL )
	{
		menuGUI = data->GetGUI();
	}

	SetSpritePath( "sbTeam" );

	playerList = new( TAG_SWF ) idMenuWidget_ScoreboardList();
	playerList->SetSpritePath( GetSpritePath(), "info", "playerList" );
	playerList->SetNumVisibleOptions( MAX_SCOREBOARD_SLOTS );
	playerList->SetWrappingAllowed( true );
	while( playerList->GetChildren().Num() < MAX_SCOREBOARD_SLOTS )
	{
		idMenuWidget_ScoreboardButton* const buttonWidget = new( TAG_SWF ) idMenuWidget_ScoreboardButton();
		buttonWidget->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_PRESS_FOCUSED, playerList->GetChildren().Num() );
		buttonWidget->AddEventAction( WIDGET_EVENT_COMMAND ).Set( WIDGET_ACTION_MUTE_PLAYER, playerList->GetChildren().Num() );
		buttonWidget->Initialize( data );
		playerList->AddChild( buttonWidget );
	}
	playerList->Initialize( data );
	AddChild( playerList );

	AddEventAction( WIDGET_EVENT_SCROLL_DOWN_LSTICK ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_SCROLL_DOWN_START_REPEATER_VARIABLE, WIDGET_EVENT_SCROLL_DOWN_LSTICK ) );
	AddEventAction( WIDGET_EVENT_SCROLL_UP_LSTICK ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_SCROLL_UP_START_REPEATER_VARIABLE, WIDGET_EVENT_SCROLL_UP_LSTICK ) );
	AddEventAction( WIDGET_EVENT_SCROLL_DOWN_LSTICK_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_DOWN_LSTICK_RELEASE ) );
	AddEventAction( WIDGET_EVENT_SCROLL_UP_LSTICK_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_UP_LSTICK_RELEASE ) );
	AddEventAction( WIDGET_EVENT_SCROLL_DOWN ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_SCROLL_DOWN_START_REPEATER_VARIABLE, WIDGET_EVENT_SCROLL_DOWN ) );
	AddEventAction( WIDGET_EVENT_SCROLL_UP ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_SCROLL_UP_START_REPEATER_VARIABLE, WIDGET_EVENT_SCROLL_UP ) );
	AddEventAction( WIDGET_EVENT_SCROLL_DOWN_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_DOWN_RELEASE ) );
	AddEventAction( WIDGET_EVENT_SCROLL_UP_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_UP_RELEASE ) );

}