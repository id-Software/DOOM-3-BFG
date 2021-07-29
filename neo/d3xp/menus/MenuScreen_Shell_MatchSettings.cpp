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

const static int NUM_GAME_OPTIONS_OPTIONS = 8;
/*
========================
idMenuScreen_Shell_MatchSettings::Initialize
========================
*/
void idMenuScreen_Shell_MatchSettings::Initialize( idMenuHandler* data )
{
	idMenuScreen::Initialize( data );

	if( data != NULL )
	{
		menuGUI = data->GetGUI();
	}

	SetSpritePath( "menuMatchSettings" );

	options = new( TAG_SWF ) idMenuWidget_DynamicList();
	options->SetNumVisibleOptions( NUM_GAME_OPTIONS_OPTIONS );
	options->SetSpritePath( GetSpritePath(), "info", "options" );
	options->SetWrappingAllowed( true );
	options->SetControlList( true );
	options->Initialize( data );
	AddChild( options );

	btnBack = new( TAG_SWF ) idMenuWidget_Button();
	btnBack->Initialize( data );
	btnBack->SetLabel( "#str_swf_multiplayer" );
	btnBack->SetSpritePath( GetSpritePath(), "info", "btnBack" );
	btnBack->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_GO_BACK );
	AddChild( btnBack );

	idMenuWidget_ControlButton* control;
	control = new( TAG_SWF ) idMenuWidget_ControlButton();
	control->SetOptionType( OPTION_SLIDER_TEXT );
	control->SetLabel( "#str_swf_mode" );	// Mode
	control->SetDataSource( &matchData, idMenuDataSource_MatchSettings::MATCH_FIELD_MODE );
	control->SetupEvents( DEFAULT_REPEAT_TIME, options->GetChildren().Num() );
	control->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_PRESS_FOCUSED, options->GetChildren().Num() );
	options->AddChild( control );

	control = new( TAG_SWF ) idMenuWidget_ControlButton();
	control->SetOptionType( OPTION_SLIDER_TEXT );
	control->SetLabel( "#str_02049" );	// Map
	control->SetDataSource( &matchData, idMenuDataSource_MatchSettings::MATCH_FIELD_MAP );
	control->SetupEvents( DEFAULT_REPEAT_TIME, options->GetChildren().Num() );
	control->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_PRESS_FOCUSED, options->GetChildren().Num() );
	options->AddChild( control );

	control = new( TAG_SWF ) idMenuWidget_ControlButton();
	control->SetOptionType( OPTION_SLIDER_TEXT );
	control->SetLabel( "#str_02183" );	// Time
	control->SetDataSource( &matchData, idMenuDataSource_MatchSettings::MATCH_FIELD_TIME );
	control->SetupEvents( DEFAULT_REPEAT_TIME, options->GetChildren().Num() );
	control->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_PRESS_FOCUSED, options->GetChildren().Num() );
	options->AddChild( control );

	control = new( TAG_SWF ) idMenuWidget_ControlButton();
	control->SetOptionType( OPTION_SLIDER_TEXT );
	control->SetLabel( "#str_00100917" );	// Score
	control->SetDataSource( &matchData, idMenuDataSource_MatchSettings::MATCH_FIELD_SCORE );
	control->SetupEvents( DEFAULT_REPEAT_TIME, options->GetChildren().Num() );
	control->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_PRESS_FOCUSED, options->GetChildren().Num() );
	options->AddChild( control );

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
idMenuScreen_Shell_MatchSettings::Update
========================
*/
void idMenuScreen_Shell_MatchSettings::Update()
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
			buttonInfo->action.Set( WIDGET_ACTION_PRESS_FOCUSED );
		}
	}

	idSWFScriptObject& root = GetSWFObject()->GetRootObject();
	if( BindSprite( root ) )
	{
		idSWFTextInstance* heading = GetSprite()->GetScriptObject()->GetNestedText( "info", "txtHeading" );
		if( heading != NULL )
		{
			heading->SetText( "#str_swf_match_settings_heading" );	// SYSTEM SETTINGS
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
idMenuScreen_Shell_MatchSettings::ShowScreen
========================
*/
void idMenuScreen_Shell_MatchSettings::ShowScreen( const mainMenuTransition_t transitionType )
{
	matchData.LoadData();
	idMenuScreen::ShowScreen( transitionType );
}

/*
========================
idMenuScreen_Shell_MatchSettings::HideScreen
========================
*/
void idMenuScreen_Shell_MatchSettings::HideScreen( const mainMenuTransition_t transitionType )
{
	if( matchData.IsDataChanged() )
	{
		matchData.CommitData();
	}
	idMenuScreen::HideScreen( transitionType );
}

/*
========================
idMenuScreen_Shell_MatchSettings::HandleAction h
========================
*/
bool idMenuScreen_Shell_MatchSettings::HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled )
{

	if( menuData == NULL )
	{
		return true;
	}

	if( menuData->ActiveScreen() != SHELL_AREA_MATCH_SETTINGS )
	{
		return false;
	}

	widgetAction_t actionType = action.GetType();
	const idSWFParmList& parms = action.GetParms();

	switch( actionType )
	{
		case WIDGET_ACTION_ADJUST_FIELD:
		{
			if( widget != NULL && widget->GetDataSource() != NULL && options != NULL )
			{
				widget->GetDataSource()->AdjustField( widget->GetDataSourceFieldIndex(), parms[ 0 ].ToInteger() );
				widget->Update();

				if( matchData.MapChanged() )
				{
					idMenuWidget_ControlButton* button = dynamic_cast< idMenuWidget_ControlButton* >( &options->GetChildByIndex( 1 ) );
					if( button != NULL )
					{
						button->Update();
					}
					matchData.ClearMapChanged();
				}
			}
			return true;
		}
		case WIDGET_ACTION_GO_BACK:
		{
			menuData->SetNextScreen( SHELL_AREA_GAME_LOBBY, MENU_TRANSITION_SIMPLE );
			return true;
		}
		case WIDGET_ACTION_PRESS_FOCUSED:
		{

			if( options == NULL )
			{
				return true;
			}

			int selectionIndex = options->GetFocusIndex();
			if( parms.Num() > 0 )
			{
				selectionIndex = parms[0].ToInteger();
			}

			if( selectionIndex != options->GetFocusIndex() )
			{
				options->SetViewIndex( options->GetViewOffset() + selectionIndex );
				options->SetFocusIndex( selectionIndex );
			}

			matchData.AdjustField( selectionIndex, 1 );
			options->Update();
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
	}

	return idMenuWidget::HandleAction( action, event, widget, forceHandled );
}

/////////////////////////////////
// SCREEN SETTINGS
/////////////////////////////////

extern idCVar si_timeLimit;
extern idCVar si_fragLimit;
extern idCVar si_map;
extern idCVar si_mode;

/*
========================
idMenuScreen_Shell_MatchSettings::idMenuDataSource_MatchSettings::idMenuDataSource_MatchSettings
========================
*/
idMenuScreen_Shell_MatchSettings::idMenuDataSource_MatchSettings::idMenuDataSource_MatchSettings()
{
	fields.SetNum( MAX_MATCH_FIELDS );
	originalFields.SetNum( MAX_MATCH_FIELDS );
	updateMap = false;
}

/*
========================
idMenuScreen_Shell_MatchSettings::idMenuDataSource_MatchSettings::LoadData
========================
*/
void idMenuScreen_Shell_MatchSettings::idMenuDataSource_MatchSettings::LoadData()
{
	updateMap = false;
	idMatchParameters matchParameters = session->GetActivePlatformLobbyBase().GetMatchParms();
	idStr val;
	GetMapName( matchParameters.gameMap, val );
	fields[ MATCH_FIELD_MAP ].SetString( val );
	GetModeName( matchParameters.gameMode, val );
	fields[ MATCH_FIELD_MODE  ].SetString( val );

	int time = matchParameters.serverInfo.GetInt( "si_timeLimit" );
	if( time == 0 )
	{
		fields[ MATCH_FIELD_TIME ].SetString( "#str_02844" );	// none
	}
	else
	{
		fields[ MATCH_FIELD_TIME ].SetString( va( "%i", time ) );
	}

	int fragLimit = matchParameters.serverInfo.GetInt( "si_fragLimit" );
	fields[ MATCH_FIELD_SCORE ].SetInteger( fragLimit );

	originalFields = fields;
}

/*
========================
idMenuScreen_Shell_MatchSettings::idMenuDataSource_MatchSettings::CommitData
========================
*/
void idMenuScreen_Shell_MatchSettings::idMenuDataSource_MatchSettings::CommitData()
{

	cvarSystem->SetModifiedFlags( CVAR_ARCHIVE );

	// make the committed fields into the backup fields
	originalFields = fields;
}

/*
========================
idMenuScreen_Shell_MatchSettings::idMenuDataSource_MatchSettings::GetMapName
========================
*/
void idMenuScreen_Shell_MatchSettings::idMenuDataSource_MatchSettings::GetMapName( int index, idStr& name )
{
	idLobbyBase& lobby = session->GetActivePlatformLobbyBase();
	const idMatchParameters& matchParameters = lobby.GetMatchParms();
	name = "#str_swf_filter_random";
	if( matchParameters.gameMap >= 0 )
	{
		const idList< mpMap_t > maps = common->GetMapList();
		name = idLocalization::GetString( maps[ idMath::ClampInt( 0, maps.Num() - 1, matchParameters.gameMap ) ].mapName );
	}
}

/*
========================
idMenuScreen_Shell_MatchSettings::idMenuDataSource_MatchSettings::GetModeName
========================
*/
void idMenuScreen_Shell_MatchSettings::idMenuDataSource_MatchSettings::GetModeName( int index, idStr& name )
{
	idLobbyBase& lobby = session->GetActivePlatformLobbyBase();
	const idMatchParameters& matchParameters = lobby.GetMatchParms();
	name = "#str_swf_filter_random";
	if( matchParameters.gameMode >= 0 )
	{
		const idStrList& modes = common->GetModeDisplayList();
		name = idLocalization::GetString( modes[ idMath::ClampInt( 0, modes.Num() - 1, matchParameters.gameMode ) ] );
	}
}

/*
========================
idMenuScreen_Shell_MatchSettings::idMenuDataSource_MatchSettings::AdjustField
========================
*/
void idMenuScreen_Shell_MatchSettings::idMenuDataSource_MatchSettings::AdjustField( const int fieldIndex, const int adjustAmount )
{

	const idStrList& modes = common->GetModeList();
	const idList< mpMap_t > maps = common->GetMapList();

	idMatchParameters matchParameters = session->GetActivePlatformLobbyBase().GetMatchParms();
	if( fieldIndex == MATCH_FIELD_MAP )
	{
		for( int i = 0; i < maps.Num(); i++ )
		{
			// Don't allow random maps in the game lobby
			matchParameters.gameMap += adjustAmount;
			if( matchParameters.gameMap < 0 )
			{
				matchParameters.gameMap = maps.Num() - 1;
			}
			matchParameters.gameMap %= maps.Num();
			matchParameters.mapName = maps[ matchParameters.gameMap ].mapFile;
			if( ( maps[matchParameters.gameMap].supportedModes & BIT( matchParameters.gameMode ) ) != 0 )
			{
				// This map supports this mode
				break;
			}
		}
		session->UpdateMatchParms( matchParameters );

		idStr val;
		GetMapName( matchParameters.gameMap, val );
		si_map.SetInteger( matchParameters.gameMap );
		fields[ MATCH_FIELD_MAP ].SetString( val );

	}
	else if( fieldIndex == MATCH_FIELD_MODE )
	{
		// Don't allow random modes in the game lobby
		matchParameters.gameMode += adjustAmount;

		if( matchParameters.gameMode < 0 )
		{
			matchParameters.gameMode = modes.Num() - 1;
		}
		matchParameters.gameMode %= modes.Num();
		updateMap = false;
		if( ( maps[matchParameters.gameMap].supportedModes & BIT( matchParameters.gameMode ) ) == 0 )
		{
			for( int i = 0; i < maps.Num(); ++i )
			{
				if( ( maps[i].supportedModes & BIT( matchParameters.gameMode ) ) != 0 )
				{
					matchParameters.gameMap = i;
					updateMap = true;
					break;
				}
			}
		}

		session->UpdateMatchParms( matchParameters );
		idStr val;

		GetModeName( matchParameters.gameMode, val );
		si_mode.SetInteger( matchParameters.gameMode );
		fields[ MATCH_FIELD_MODE ].SetString( val );

		if( updateMap )
		{
			GetMapName( matchParameters.gameMap, val );
			si_map.SetInteger( matchParameters.gameMap );
			fields[ MATCH_FIELD_MAP ].SetString( val );
		}

	}
	else if( fieldIndex == MATCH_FIELD_TIME )
	{
		int time = si_timeLimit.GetInteger() + ( adjustAmount * 5 );
		if( time < 0 )
		{
			time = 60;
		}
		else if( time > 60 )
		{
			time = 0;
		}

		if( time == 0 )
		{
			fields[ MATCH_FIELD_TIME ].SetString( "#str_02844" );	// none
		}
		else
		{
			fields[ MATCH_FIELD_TIME ].SetString( va( "%i", time ) );
		}

		si_timeLimit.SetInteger( time );

		matchParameters.serverInfo.SetInt( "si_timeLimit", si_timeLimit.GetInteger() );
		session->UpdateMatchParms( matchParameters );

	}
	else if( fieldIndex == MATCH_FIELD_SCORE )
	{

		int val = fields[ fieldIndex ].ToInteger() + ( adjustAmount * 5 );
		if( val < 5 )
		{
			val = MP_PLAYER_MAXFRAGS;
		}
		else if( val > MP_PLAYER_MAXFRAGS )
		{
			val = 5;
		}

		fields[ fieldIndex ].SetInteger( val );
		si_fragLimit.SetInteger( val );

		matchParameters.serverInfo.SetInt( "si_fragLimit", si_fragLimit.GetInteger() );
		session->UpdateMatchParms( matchParameters );
	}

	cvarSystem->ClearModifiedFlags( CVAR_ARCHIVE );
}

/*
========================
idMenuScreen_Shell_MatchSettings::idMenuDataSource_MatchSettings::IsDataChanged
========================
*/
bool idMenuScreen_Shell_MatchSettings::idMenuDataSource_MatchSettings::IsDataChanged() const
{

	if( fields[ MATCH_FIELD_TIME ].ToString() != originalFields[ MATCH_FIELD_TIME ].ToString() )
	{
		return true;
	}

	if( fields[ MATCH_FIELD_MAP ].ToString() != originalFields[ MATCH_FIELD_MAP ].ToString() )
	{
		return true;
	}

	if( fields[ MATCH_FIELD_MODE ].ToString() != originalFields[ MATCH_FIELD_MODE ].ToString() )
	{
		return true;
	}

	if( fields[ MATCH_FIELD_SCORE ].ToInteger() != originalFields[ MATCH_FIELD_SCORE ].ToInteger() )
	{
		return true;
	}

	return false;
}
