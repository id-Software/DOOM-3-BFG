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

extern idCVar g_demoMode;
const static int NUM_PAUSE_OPTIONS = 6;

enum pauseMenuCmds_t
{
	PAUSE_CMD_RESTART,
	PAUSE_CMD_DEAD_RESTART,
	PAUSE_CMD_SETTINGS,
	PAUSE_CMD_EXIT,
	PAUSE_CMD_LEAVE,
	PAUSE_CMD_RETURN,
	PAUSE_CMD_LOAD,
	PAUSE_CMD_SAVE,
	PAUSE_CMD_PS3,
	PAUSE_CMD_INVITE_FRIENDS
};

/*
========================
idMenuScreen_Shell_Pause::Initialize
========================
*/
void idMenuScreen_Shell_Pause::Initialize( idMenuHandler* data )
{
	idMenuScreen::Initialize( data );

	if( data != NULL )
	{
		menuGUI = data->GetGUI();
	}

	SetSpritePath( "menuPause" );

	options = new( TAG_SWF ) idMenuWidget_DynamicList();
	options->SetNumVisibleOptions( NUM_PAUSE_OPTIONS );
	options->SetSpritePath( GetSpritePath(), "info", "options" );
	options->SetWrappingAllowed( true );
	AddChild( options );

	idMenuWidget_Help* const helpWidget = new( TAG_SWF ) idMenuWidget_Help();
	helpWidget->SetSpritePath( GetSpritePath(), "info", "helpTooltip" );
	AddChild( helpWidget );

	while( options->GetChildren().Num() < NUM_PAUSE_OPTIONS )
	{
		idMenuWidget_Button* const buttonWidget = new( TAG_SWF ) idMenuWidget_Button();
		buttonWidget->Initialize( data );
		buttonWidget->RegisterEventObserver( helpWidget );
		options->AddChild( buttonWidget );
	}
	options->Initialize( data );

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
idMenuScreen_Shell_Pause::Update
========================
*/
void idMenuScreen_Shell_Pause::Update()
{

	if( menuData != NULL )
	{
		idMenuWidget_CommandBar* cmdBar = menuData->GetCmdBar();
		if( cmdBar != NULL )
		{
			cmdBar->ClearAllButtons();
			idMenuWidget_CommandBar::buttonInfo_t* buttonInfo;
			buttonInfo = cmdBar->GetButton( idMenuWidget_CommandBar::BUTTON_JOY1 );
			if( menuData->GetPlatform() != 2 )
			{
				buttonInfo->label = "#str_SWF_SELECT";
			}
			buttonInfo->action.Set( WIDGET_ACTION_PRESS_FOCUSED );

			bool isDead = false;
			idPlayer* player = gameLocal.GetLocalPlayer();
			if( player != NULL )
			{
				if( player->health <= 0 )
				{
					isDead = true;
				}
			}

			if( !isDead )
			{
				buttonInfo = cmdBar->GetButton( idMenuWidget_CommandBar::BUTTON_JOY2 );
				if( menuData->GetPlatform() != 2 )
				{
					buttonInfo->label = "#str_00395";
				}
				buttonInfo->action.Set( WIDGET_ACTION_COMMAND, PAUSE_CMD_RETURN );
			}
		}
	}

	idMenuScreen::Update();
}

/*
========================
idMenuScreen_Shell_Pause::ShowScreen
========================
*/
void idMenuScreen_Shell_Pause::ShowScreen( const mainMenuTransition_t transitionType )
{

	idList< idList< idStr, TAG_IDLIB_LIST_MENU >, TAG_IDLIB_LIST_MENU > menuOptions;
	idList< idStr > option;

	bool isDead = false;
	idPlayer* player = gameLocal.GetLocalPlayer();
	if( player != NULL )
	{
		if( player->health <= 0 )
		{
			isDead = true;
		}
	}

	if( g_demoMode.GetBool() )
	{
		isMpPause = false;
		if( isDead )
		{
			option.Append( "#str_swf_restart_map" );	// retart map
			menuOptions.Append( option );
			option.Clear();
			option.Append( "#str_swf_settings" );	// settings
			menuOptions.Append( option );
			option.Clear();
			option.Append( "#str_swf_exit_game" );	// exit game
			menuOptions.Append( option );

			int index = 0;
			options->GetChildByIndex( index ).ClearEventActions();
			options->GetChildByIndex( index ).AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, PAUSE_CMD_DEAD_RESTART );
			index++;
			options->GetChildByIndex( index ).ClearEventActions();
			options->GetChildByIndex( index ).AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, PAUSE_CMD_SETTINGS );
			index++;
			options->GetChildByIndex( index ).ClearEventActions();
			options->GetChildByIndex( index ).AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, PAUSE_CMD_LEAVE );
		}
		else
		{
			option.Append( "#str_04106" );	// return to game
			menuOptions.Append( option );
			option.Clear();
			option.Append( "#str_swf_restart_map" );	// retart map
			menuOptions.Append( option );
			option.Clear();
			option.Append( "#str_swf_settings" );	// settings
			menuOptions.Append( option );
			option.Clear();
			option.Append( "#str_swf_exit_game" );	// exit game
			menuOptions.Append( option );

			int index = 0;
			options->GetChildByIndex( index ).ClearEventActions();
			options->GetChildByIndex( index ).AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, PAUSE_CMD_RETURN );
			index++;
			options->GetChildByIndex( index ).ClearEventActions();
			options->GetChildByIndex( index ).AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, PAUSE_CMD_RESTART );
			index++;
			options->GetChildByIndex( index ).ClearEventActions();
			options->GetChildByIndex( index ).AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, PAUSE_CMD_SETTINGS );
			index++;
			options->GetChildByIndex( index ).ClearEventActions();
			options->GetChildByIndex( index ).AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, PAUSE_CMD_EXIT );
		}
	}
	else
	{
		if( common->IsMultiplayer() )
		{
			isMpPause = true;
			option.Append( "#str_04106" );	// return to game
			menuOptions.Append( option );
			option.Clear();
			option.Append( "#str_swf_settings" );	// settings
			menuOptions.Append( option );
			option.Clear();
			option.Append( "#str_swf_invite_friends_upper" );	// settings
			menuOptions.Append( option );
			option.Clear();
			option.Append( "#str_swf_leave_game" );	// leave game
			menuOptions.Append( option );

			int index = 0;
			idMenuWidget_Button* buttonWidget = NULL;
			options->GetChildByIndex( index ).ClearEventActions();
			options->GetChildByIndex( index ).AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, PAUSE_CMD_RETURN );
			buttonWidget = dynamic_cast< idMenuWidget_Button* >( &options->GetChildByIndex( index ) );
			if( buttonWidget != NULL )
			{
				buttonWidget->SetDescription( "#str_swf_resume_desc" );
			}
			index++;
			options->GetChildByIndex( index ).ClearEventActions();
			options->GetChildByIndex( index ).AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, PAUSE_CMD_SETTINGS );
			buttonWidget = dynamic_cast< idMenuWidget_Button* >( &options->GetChildByIndex( index ) );
			if( buttonWidget != NULL )
			{
				buttonWidget->SetDescription( "#str_02206" );
			}
			index++;
			options->GetChildByIndex( index ).ClearEventActions();
			options->GetChildByIndex( index ).AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, PAUSE_CMD_INVITE_FRIENDS );
			buttonWidget = dynamic_cast< idMenuWidget_Button* >( &options->GetChildByIndex( index ) );
			if( buttonWidget != NULL )
			{
				buttonWidget->SetDescription( "#str_swf_invite_desc" );
			}
			index++;
			options->GetChildByIndex( index ).ClearEventActions();
			options->GetChildByIndex( index ).AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, PAUSE_CMD_LEAVE );
			buttonWidget = dynamic_cast< idMenuWidget_Button* >( &options->GetChildByIndex( index ) );
			if( buttonWidget != NULL )
			{
				buttonWidget->SetDescription( "#str_swf_exit_game_desc" );
			}

		}
		else
		{
			isMpPause = false;
			if( isDead )
			{
				option.Append( "#str_02187" );	// load game
				menuOptions.Append( option );
				option.Clear();
				option.Append( "#str_swf_settings" );	// settings
				menuOptions.Append( option );
				option.Clear();
				option.Append( "#str_swf_exit_game" );	// exit game
				menuOptions.Append( option );

				int index = 0;
				idMenuWidget_Button* buttonWidget = NULL;
				options->GetChildByIndex( index ).ClearEventActions();
				options->GetChildByIndex( index ).AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, PAUSE_CMD_LOAD );
				buttonWidget = dynamic_cast< idMenuWidget_Button* >( &options->GetChildByIndex( index ) );
				if( buttonWidget != NULL )
				{
					buttonWidget->SetDescription( "#str_02213" );
				}
				index++;
				options->GetChildByIndex( index ).ClearEventActions();
				options->GetChildByIndex( index ).AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, PAUSE_CMD_SETTINGS );
				buttonWidget = dynamic_cast< idMenuWidget_Button* >( &options->GetChildByIndex( index ) );
				if( buttonWidget != NULL )
				{
					buttonWidget->SetDescription( "#str_02206" );
				}
				index++;
				options->GetChildByIndex( index ).ClearEventActions();
				options->GetChildByIndex( index ).AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, PAUSE_CMD_EXIT );
				buttonWidget = dynamic_cast< idMenuWidget_Button* >( &options->GetChildByIndex( index ) );
				if( buttonWidget != NULL )
				{
					buttonWidget->SetDescription( "#str_swf_exit_game_desc" );
				}

			}
			else
			{
				option.Append( "#str_04106" );	// return to game
				menuOptions.Append( option );
				option.Clear();
				option.Append( "#str_02179" );	// save game
				menuOptions.Append( option );
				option.Clear();
				option.Append( "#str_02187" );	// load game
				menuOptions.Append( option );
				option.Clear();
				option.Append( "#str_swf_settings" );	// settings
				menuOptions.Append( option );
				option.Clear();
				option.Append( "#str_swf_exit_game" );	// exit game
				menuOptions.Append( option );

				int index = 0;
				idMenuWidget_Button* buttonWidget = NULL;
				options->GetChildByIndex( index ).ClearEventActions();
				options->GetChildByIndex( index ).AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, PAUSE_CMD_RETURN );
				buttonWidget = dynamic_cast< idMenuWidget_Button* >( &options->GetChildByIndex( index ) );
				if( buttonWidget != NULL )
				{
					buttonWidget->SetDescription( "#str_swf_resume_desc" );
				}
				index++;
				options->GetChildByIndex( index ).ClearEventActions();
				options->GetChildByIndex( index ).AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, PAUSE_CMD_SAVE );
				buttonWidget = dynamic_cast< idMenuWidget_Button* >( &options->GetChildByIndex( index ) );
				if( buttonWidget != NULL )
				{
					buttonWidget->SetDescription( "#str_02211" );
				}
				index++;
				options->GetChildByIndex( index ).ClearEventActions();
				options->GetChildByIndex( index ).AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, PAUSE_CMD_LOAD );
				buttonWidget = dynamic_cast< idMenuWidget_Button* >( &options->GetChildByIndex( index ) );
				if( buttonWidget != NULL )
				{
					buttonWidget->SetDescription( "#str_02213" );
				}
				index++;
				options->GetChildByIndex( index ).ClearEventActions();
				options->GetChildByIndex( index ).AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, PAUSE_CMD_SETTINGS );
				buttonWidget = dynamic_cast< idMenuWidget_Button* >( &options->GetChildByIndex( index ) );
				if( buttonWidget != NULL )
				{
					buttonWidget->SetDescription( "#str_02206" );
				}
				index++;
				options->GetChildByIndex( index ).ClearEventActions();
				options->GetChildByIndex( index ).AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, PAUSE_CMD_EXIT );
				buttonWidget = dynamic_cast< idMenuWidget_Button* >( &options->GetChildByIndex( index ) );
				if( buttonWidget != NULL )
				{
					buttonWidget->SetDescription( "#str_swf_exit_game_desc" );
				}
			}
		}
	}

	options->SetListData( menuOptions );
	idMenuScreen::ShowScreen( transitionType );

	if( options->GetFocusIndex() >= menuOptions.Num() )
	{
		options->SetViewIndex( 0 );
		options->SetFocusIndex( 0 );
	}

}

/*
========================
idMenuScreen_Shell_Pause::HideScreen
========================
*/
void idMenuScreen_Shell_Pause::HideScreen( const mainMenuTransition_t transitionType )
{
	idMenuScreen::HideScreen( transitionType );
}

/*
========================
idMenuScreen_Shell_Pause::HandleExitGameBtn
========================
*/
void idMenuScreen_Shell_Pause::HandleExitGameBtn()
{
	class idSWFScriptFunction_QuitDialog : public idSWFScriptFunction_RefCounted
	{
	public:
		idSWFScriptFunction_QuitDialog( idMenuScreen_Shell_Pause* _menu, gameDialogMessages_t _msg, bool _accept )
		{
			menu = _menu;
			msg = _msg;
			accept = _accept;
		}
		idSWFScriptVar Call( idSWFScriptObject* thisObject, const idSWFParmList& parms )
		{
			common->Dialog().ClearDialog( msg );
			if( accept )
			{
				cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "disconnect\n" );
			}
			return idSWFScriptVar();
		}
	private:
		idMenuScreen_Shell_Pause* menu;
		gameDialogMessages_t msg;
		bool accept;
	};

	gameDialogMessages_t msg = GDM_SP_QUIT_SAVE;

	if( common->IsMultiplayer() )
	{
		if( ( session->GetGameLobbyBase().GetNumLobbyUsers() > 1 ) && MatchTypeHasStats( session->GetGameLobbyBase().GetMatchParms().matchFlags ) )
		{
			msg = GDM_MULTI_VDM_QUIT_LOSE_LEADERBOARDS;
		}
		else
		{
			msg = GDM_MULTI_VDM_QUIT;
		}
	}

	common->Dialog().AddDialog( msg, DIALOG_ACCEPT_CANCEL, new idSWFScriptFunction_QuitDialog( this, msg, true ), new idSWFScriptFunction_QuitDialog( this, msg, false ), false );
}

/*
========================
idMenuScreen_Shell_Pause::HandleRestartBtn
========================
*/
void idMenuScreen_Shell_Pause::HandleRestartBtn()
{
	class idSWFScriptFunction_RestartDialog : public idSWFScriptFunction_RefCounted
	{
	public:
		idSWFScriptFunction_RestartDialog( idMenuScreen_Shell_Pause* _menu, gameDialogMessages_t _msg, bool _accept )
		{
			menu = _menu;
			msg = _msg;
			accept = _accept;
		}
		idSWFScriptVar Call( idSWFScriptObject* thisObject, const idSWFParmList& parms )
		{
			common->Dialog().ClearDialog( msg );
			if( accept )
			{
				cmdSystem->AppendCommandText( "restartMap\n" );
			}
			return idSWFScriptVar();
		}
	private:
		idMenuScreen_Shell_Pause* menu;
		gameDialogMessages_t msg;
		bool accept;
	};

	common->Dialog().AddDialog( GDM_SP_RESTART_SAVE, DIALOG_ACCEPT_CANCEL, new idSWFScriptFunction_RestartDialog( this, GDM_SP_RESTART_SAVE, true ), new idSWFScriptFunction_RestartDialog( this, GDM_SP_RESTART_SAVE, false ), false );
}

/*
========================
idMenuScreen_Shell_Pause::HandleAction
========================
*/
bool idMenuScreen_Shell_Pause::HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled )
{

	if( menuData == NULL )
	{
		return true;
	}

	if( menuData->ActiveScreen() != SHELL_AREA_ROOT )
	{
		return false;
	}

	widgetAction_t actionType = action.GetType();
	const idSWFParmList& parms = action.GetParms();

	switch( actionType )
	{
		case WIDGET_ACTION_COMMAND:
		{
			switch( parms[0].ToInteger() )
			{
				case PAUSE_CMD_RESTART:
				{
					HandleRestartBtn();
					break;
				}
				case PAUSE_CMD_DEAD_RESTART:
				{
					cmdSystem->AppendCommandText( "restartMap\n" );
					break;
				}
				case PAUSE_CMD_SETTINGS:
				{
					menuData->SetNextScreen( SHELL_AREA_SETTINGS, MENU_TRANSITION_SIMPLE );
					break;
				}
				case PAUSE_CMD_LEAVE:
				case PAUSE_CMD_EXIT:
				{
					HandleExitGameBtn();
					break;
				}
				case PAUSE_CMD_RETURN:
				{
					menuData->SetNextScreen( SHELL_AREA_INVALID, MENU_TRANSITION_SIMPLE );
					break;
				}
				case PAUSE_CMD_LOAD:
				{
					menuData->SetNextScreen( SHELL_AREA_LOAD, MENU_TRANSITION_SIMPLE );
					break;
				}
				case PAUSE_CMD_SAVE:
				{
					menuData->SetNextScreen( SHELL_AREA_SAVE, MENU_TRANSITION_SIMPLE );
					break;
				}
				case PAUSE_CMD_PS3:
				{
					menuData->SetNextScreen( SHELL_AREA_PLAYSTATION, MENU_TRANSITION_SIMPLE );
					break;
				}
				case PAUSE_CMD_INVITE_FRIENDS:
				{
					session->InviteFriends();
					break;
				}
			}
			return true;
		}
	}

	return idMenuWidget::HandleAction( action, event, widget, forceHandled );
}