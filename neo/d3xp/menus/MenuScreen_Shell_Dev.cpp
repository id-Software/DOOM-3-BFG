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

const static int NUM_DEV_OPTIONS = 8;

/*
========================
idMenuScreen_Shell_Dev::Initialize
========================
*/
void idMenuScreen_Shell_Dev::Initialize( idMenuHandler* data )
{
	idMenuScreen::Initialize( data );

	if( data != NULL )
	{
		menuGUI = data->GetGUI();
	}

	SetSpritePath( "menuSettings" );

	options = new( TAG_SWF ) idMenuWidget_DynamicList();
	options->SetNumVisibleOptions( NUM_DEV_OPTIONS );
	options->SetSpritePath( GetSpritePath(), "info", "options" );
	options->SetWrappingAllowed( true );

	while( options->GetChildren().Num() < NUM_DEV_OPTIONS )
	{
		idMenuWidget_Button* const buttonWidget = new( TAG_SWF ) idMenuWidget_Button();
		buttonWidget->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_PRESS_FOCUSED, options->GetChildren().Num() );
		buttonWidget->Initialize( data );
		options->AddChild( buttonWidget );
	}
	options->Initialize( data );

	AddChild( options );

	btnBack = new( TAG_SWF ) idMenuWidget_Button();
	btnBack->Initialize( data );
	btnBack->SetLabel( "MAIN MENU" );
	btnBack->SetSpritePath( GetSpritePath(), "info", "btnBack" );
	btnBack->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_GO_BACK );

	AddChild( btnBack );

	SetupDevOptions();
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
idMenuScreen_Shell_Dev::SetupDevOptions
========================
*/
void idMenuScreen_Shell_Dev::SetupDevOptions()
{

	devOptions.Clear();

	devOptions.Append( devOption_t( "game/mars_city1", "Mars City 1" ) );
	devOptions.Append( devOption_t( "game/mc_underground", "MC Underground" ) );
	devOptions.Append( devOption_t( "game/mars_city2", "Mars City 2" ) );
	devOptions.Append( devOption_t( "game/admin", "Admin" ) );
	devOptions.Append( devOption_t( "game/alphalabs1", "Alpha Labs 1" ) );
	devOptions.Append( devOption_t( "game/alphalabs2", "Alpha Labs 2" ) );
	devOptions.Append( devOption_t( "game/alphalabs3", "Alpha Labs 3" ) );
	devOptions.Append( devOption_t( "game/alphalabs4", "Alpha Labs 4" ) );
	devOptions.Append( devOption_t( "game/enpro", "Enpro" ) );
	devOptions.Append( devOption_t( "game/commoutside", "Comm outside" ) );
	devOptions.Append( devOption_t( "game/comm1", "Comm 1" ) );
	devOptions.Append( devOption_t( "game/recycling1", "Recycling 1" ) );
	devOptions.Append( devOption_t( "game/recycling2", "Recycling 2" ) );
	devOptions.Append( devOption_t( "game/monorail", "Monorail" ) );
	devOptions.Append( devOption_t( "game/delta1", "Delta Labs 1" ) );
	devOptions.Append( devOption_t( "game/delta2a", "Delta Labs 2a" ) );
	devOptions.Append( devOption_t( "game/delta2b", "Delta Labs 2b" ) );
	devOptions.Append( devOption_t( "game/delta3", "Delta Labs 3" ) );
	devOptions.Append( devOption_t( "game/delta4", "Delta Labs 4" ) );
	devOptions.Append( devOption_t( "game/hell1", "Hell 1" ) );
	devOptions.Append( devOption_t( "game/delta5", "Delta Labs 5" ) );
	devOptions.Append( devOption_t( "game/cpu", "CPU" ) );
	devOptions.Append( devOption_t( "game/cpuboss", "CPU Boss" ) );
	devOptions.Append( devOption_t( "game/site3", "Site 3" ) );
	devOptions.Append( devOption_t( "game/caverns1", "Caverns 1" ) );
	devOptions.Append( devOption_t( "game/caverns2", "Caverns 2" ) );
	devOptions.Append( devOption_t( "game/hellhole", "Hell Hole" ) );
	devOptions.Append( devOption_t( NULL, "-DOOM 3 Expansion-" ) );
	devOptions.Append( devOption_t( "game/erebus1", "Erebus 1" ) );
	devOptions.Append( devOption_t( "game/erebus2", "Erebus 2" ) );
	devOptions.Append( devOption_t( "game/erebus3", "Erebus 3" ) );
	devOptions.Append( devOption_t( "game/erebus4", "Erebus 4" ) );
	devOptions.Append( devOption_t( "game/erebus5", "Erebus 5" ) );
	devOptions.Append( devOption_t( "game/erebus6", "Erebus 6" ) );
	devOptions.Append( devOption_t( "game/phobos1", "Phobos 1" ) );
	devOptions.Append( devOption_t( "game/phobos2", "Phobos 2" ) );
	devOptions.Append( devOption_t( "game/phobos3", "Phobos 3" ) );
	devOptions.Append( devOption_t( "game/phobos4", "Phobos 4" ) );
	devOptions.Append( devOption_t( "game/deltax", "Delta X" ) );
	devOptions.Append( devOption_t( "game/hell", "Hell" ) );
	devOptions.Append( devOption_t( NULL, "-Lost Missions-" ) );
	devOptions.Append( devOption_t( "game/le_enpro1", "Enpro 1" ) );
	devOptions.Append( devOption_t( "game/le_enpro2", "Enpro 2" ) );
	devOptions.Append( devOption_t( "game/le_underground", "Undeground" ) );
	devOptions.Append( devOption_t( "game/le_underground2", "Undeground 2" ) );
	devOptions.Append( devOption_t( "game/le_exis1", "Exis 1" ) );
	devOptions.Append( devOption_t( "game/le_exis2", "Exis 2" ) );
	devOptions.Append( devOption_t( "game/le_hell", "Hell" ) );
	devOptions.Append( devOption_t( "game/le_hell_post", "Hell Post" ) );
	devOptions.Append( devOption_t( NULL, "-Test Maps-" ) );
	devOptions.Append( devOption_t( "game/pdas", "PDAs" ) );
	devOptions.Append( devOption_t( "testmaps/test_box", "Box" ) );

	idList< idList< idStr, TAG_IDLIB_LIST_MENU >, TAG_IDLIB_LIST_MENU > menuOptions;

	for( int i = 0; i < devOptions.Num(); ++i )
	{
		idList< idStr > option;
		option.Append( devOptions[ i ].name );
		menuOptions.Append( option );
	}

	options->SetListData( menuOptions );
}

/*
========================
idMenuScreen_Shell_Dev::Update
========================
*/
void idMenuScreen_Shell_Dev::Update()
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
			heading->SetText( "DEV" );
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
idMenuScreen_Shell_Dev::ShowScreen
========================
*/
void idMenuScreen_Shell_Dev::ShowScreen( const mainMenuTransition_t transitionType )
{
	idMenuScreen::ShowScreen( transitionType );
}

/*
========================
idMenuScreen_Shell_Dev::HideScreen
========================
*/
void idMenuScreen_Shell_Dev::HideScreen( const mainMenuTransition_t transitionType )
{
	idMenuScreen::HideScreen( transitionType );
}

/*
========================
idMenuScreen_Shell_Dev::HandleAction h
========================
*/
bool idMenuScreen_Shell_Dev::HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled )
{

	if( menuData == NULL )
	{
		return true;
	}

	if( menuData->ActiveScreen() != SHELL_AREA_DEV )
	{
		return false;
	}

	widgetAction_t actionType = action.GetType();
	const idSWFParmList& parms = action.GetParms();

	switch( actionType )
	{
		case WIDGET_ACTION_GO_BACK:
		{
			menuData->SetNextScreen( SHELL_AREA_ROOT, MENU_TRANSITION_SIMPLE );
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

			if( options->GetFocusIndex() != selectionIndex - options->GetViewOffset() )
			{
				options->SetFocusIndex( selectionIndex );
				options->SetViewIndex( options->GetViewOffset() + selectionIndex );
			}

			int mapIndex = options->GetViewIndex();
			if( ( mapIndex < devOptions.Num() ) && ( devOptions[ mapIndex ].map != NULL ) )
			{
				cmdSystem->AppendCommandText( va( "devmap %s\n", devOptions[ mapIndex ].map ) );
			}

			return true;
		}
	}

	return idMenuWidget::HandleAction( action, event, widget, forceHandled );
}