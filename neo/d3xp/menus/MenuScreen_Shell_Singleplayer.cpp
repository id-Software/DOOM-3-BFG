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

const static int NUM_SINGLEPLAYER_OPTIONS = 8;
/*
========================
idMenuScreen_Shell_Singleplayer::Initialize
========================
*/
void idMenuScreen_Shell_Singleplayer::Initialize( idMenuHandler* data )
{
	idMenuScreen::Initialize( data );

	if( data != NULL )
	{
		menuGUI = data->GetGUI();
	}

	SetSpritePath( "menuCampaign" );

	options = new( TAG_SWF ) idMenuWidget_DynamicList();
	options->SetNumVisibleOptions( NUM_SINGLEPLAYER_OPTIONS );
	options->SetSpritePath( GetSpritePath(), "info", "options" );
	options->SetWrappingAllowed( true );
	AddChild( options );

	idMenuWidget_Help* const helpWidget = new( TAG_SWF ) idMenuWidget_Help();
	helpWidget->SetSpritePath( GetSpritePath(), "info", "helpTooltip" );
	AddChild( helpWidget );

	while( options->GetChildren().Num() < NUM_SINGLEPLAYER_OPTIONS )
	{
		idMenuWidget_Button* const buttonWidget = new( TAG_SWF ) idMenuWidget_Button();
		buttonWidget->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_PRESS_FOCUSED, options->GetChildren().Num() );
		buttonWidget->RegisterEventObserver( helpWidget );
		buttonWidget->Initialize( data );
		options->AddChild( buttonWidget );
	}
	options->Initialize( data );

	btnBack = new( TAG_SWF ) idMenuWidget_Button();
	btnBack->Initialize( data );
	btnBack->SetLabel( "#str_02305" );
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
idMenuScreen_Shell_Singleplayer::Update
========================
*/
void idMenuScreen_Shell_Singleplayer::Update()
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
			heading->SetText( "#str_swf_campaign" );
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
idMenuScreen_Shell_Singleplayer::ShowScreen
========================
*/
void idMenuScreen_Shell_Singleplayer::ShowScreen( const mainMenuTransition_t transitionType )
{

	idList< idList< idStr, TAG_IDLIB_LIST_MENU >, TAG_IDLIB_LIST_MENU > menuOptions;
	idList< idStr > option;

	canContinue = false;
	const saveGameDetailsList_t& saveGameInfo = session->GetSaveGameManager().GetEnumeratedSavegames();
	canContinue = ( saveGameInfo.Num() > 0 );
	if( canContinue )
	{
		option.Append( "#str_swf_continue_game" );	// continue game
		menuOptions.Append( option );
		option.Clear();
		option.Append( "#str_01866" );	// new game
		menuOptions.Append( option );
		option.Clear();
		option.Append( "#str_01867" );	// load game
		menuOptions.Append( option );

		int index = 0;
		idMenuWidget_Button* buttonWidget = dynamic_cast< idMenuWidget_Button* >( &options->GetChildByIndex( index ) );
		if( buttonWidget != NULL )
		{
			buttonWidget->SetDescription( "#str_swf_continue_desc" );
		}
		index++;
		buttonWidget = dynamic_cast< idMenuWidget_Button* >( &options->GetChildByIndex( index ) );
		if( buttonWidget != NULL )
		{
			buttonWidget->SetDescription( "#str_02209" );
		}
		index++;
		buttonWidget = dynamic_cast< idMenuWidget_Button* >( &options->GetChildByIndex( index ) );
		if( buttonWidget != NULL )
		{
			buttonWidget->SetDescription( "#str_02213" );
		}
		index++;

	}
	else
	{
		option.Append( "#str_01866" );	// new game
		menuOptions.Append( option );
		option.Clear();
		option.Append( "#str_01867" );	// load game
		menuOptions.Append( option );

		if( options != NULL )
		{
			int index = 0;
			idMenuWidget_Button* buttonWidget = dynamic_cast< idMenuWidget_Button* >( &options->GetChildByIndex( index ) );
			if( buttonWidget != NULL )
			{
				buttonWidget->SetDescription( "#str_02209" );
			}
			index++;
			buttonWidget = dynamic_cast< idMenuWidget_Button* >( &options->GetChildByIndex( index ) );
			if( buttonWidget != NULL )
			{
				buttonWidget->SetDescription( "#str_02213" );
			}
		}
	}

	if( options != NULL )
	{
		options->SetListData( menuOptions );
	}

	idMenuScreen::ShowScreen( transitionType );
}

/*
========================
idMenuScreen_Shell_Singleplayer::HideScreen
========================
*/
void idMenuScreen_Shell_Singleplayer::HideScreen( const mainMenuTransition_t transitionType )
{
	idMenuScreen::HideScreen( transitionType );
}

/*
========================
idMenuScreen_Shell_Singleplayer::ContinueGame
========================
*/
void idMenuScreen_Shell_Singleplayer::ContinueGame()
{
	const saveGameDetailsList_t& saveGameInfo = session->GetSaveGameManager().GetEnumeratedSavegames();
	saveGameDetailsList_t sortedSaves = saveGameInfo;
	sortedSaves.Sort( idSort_SavesByDate() );
	if( sortedSaves.Num() > 0 )
	{
		if( sortedSaves[0].damaged )
		{
			class idSWFScriptFunction_ContinueDamaged : public idSWFScriptFunction_RefCounted
			{
			public:
				idSWFScriptVar Call( idSWFScriptObject* thisObject, const idSWFParmList& parms )
				{
					common->Dialog().ClearDialog( GDM_CORRUPT_CONTINUE );
					return idSWFScriptVar();
				}
			};

			idStaticList< idSWFScriptFunction*, 4 > callbacks;
			callbacks.Append( new( TAG_SWF ) idSWFScriptFunction_ContinueDamaged() );
			idStaticList< idStrId, 4 > optionText;
			optionText.Append( idStrId( "#str_04339" ) );	// OK
			common->Dialog().AddDynamicDialog( GDM_CORRUPT_CONTINUE, callbacks, optionText, false, "" );
		}
		else
		{
			const idStr& name = sortedSaves[ 0 ].slotName;
			cmdSystem->AppendCommandText( va( "loadgame %s\n", name.c_str() ) );
		}
	}
}

/*
========================
idMenuScreen_Shell_Singleplayer::HandleAction
========================
*/
bool idMenuScreen_Shell_Singleplayer::HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled )
{

	if( menuData == NULL )
	{
		return true;
	}

	if( menuData->ActiveScreen() != SHELL_AREA_CAMPAIGN )
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

			canContinue = false;
			const saveGameDetailsList_t& saveGameInfo = session->GetSaveGameManager().GetEnumeratedSavegames();
			canContinue = ( saveGameInfo.Num() > 0 );
			if( canContinue )
			{
				if( selectionIndex == 0 )
				{
					ContinueGame();

				}
				else if( selectionIndex == 1 )
				{
					class idSWFScriptFunction_NewGame : public idSWFScriptFunction_RefCounted
					{
					public:
						idSWFScriptFunction_NewGame( idMenuHandler* _menuData, bool _accept )
						{
							menuData = _menuData;
							accept = _accept;
						}
						idSWFScriptVar Call( idSWFScriptObject* thisObject, const idSWFParmList& parms )
						{
							common->Dialog().ClearDialog( GDM_DELETE_AUTOSAVE );
							if( accept )
							{
								menuData->SetNextScreen( SHELL_AREA_NEW_GAME, MENU_TRANSITION_SIMPLE );
							}
							return idSWFScriptVar();
						}
					private:
						idMenuHandler* menuData;
						bool accept;
					};
					common->Dialog().AddDialog( GDM_DELETE_AUTOSAVE, DIALOG_ACCEPT_CANCEL, new idSWFScriptFunction_NewGame( menuData, true ), new idSWFScriptFunction_NewGame( menuData, false ), true );
				}
				else if( selectionIndex == 2 )
				{
					menuData->SetNextScreen( SHELL_AREA_LOAD, MENU_TRANSITION_SIMPLE );
				}
			}
			else
			{
				if( selectionIndex == 0 )
				{
					menuData->SetNextScreen( SHELL_AREA_NEW_GAME, MENU_TRANSITION_SIMPLE );
				}
				else if( selectionIndex == 1 )
				{
					menuData->SetNextScreen( SHELL_AREA_LOAD, MENU_TRANSITION_SIMPLE );
				}
			}

			return true;
		}
	}

	return idMenuWidget::HandleAction( action, event, widget, forceHandled );
}