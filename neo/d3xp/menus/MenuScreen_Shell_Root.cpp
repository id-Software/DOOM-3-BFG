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
const static int NUM_MAIN_OPTIONS = 6;
/*
========================
idMenuScreen_Shell_Root::Initialize
========================
*/
void idMenuScreen_Shell_Root::Initialize( idMenuHandler* data )
{
	idMenuScreen::Initialize( data );

	if( data != NULL )
	{
		menuGUI = data->GetGUI();
	}

	SetSpritePath( "menuMain" );

	options = new( TAG_SWF ) idMenuWidget_DynamicList();
	options->SetNumVisibleOptions( NUM_MAIN_OPTIONS );
	options->SetSpritePath( GetSpritePath(), "info", "options" );
	options->Initialize( data );
	options->SetWrappingAllowed( true );
	AddChild( options );

	helpWidget = new( TAG_SWF ) idMenuWidget_Help();
	helpWidget->SetSpritePath( GetSpritePath(), "info", "helpTooltip" );
	AddChild( helpWidget );

	while( options->GetChildren().Num() < NUM_MAIN_OPTIONS )
	{
		idMenuWidget_Button* const buttonWidget = new( TAG_SWF ) idMenuWidget_Button();
		buttonWidget->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_PRESS_FOCUSED, options->GetChildren().Num() );
		buttonWidget->Initialize( data );
		buttonWidget->RegisterEventObserver( helpWidget );
		options->AddChild( buttonWidget );
	}

	options->AddEventAction( WIDGET_EVENT_SCROLL_DOWN ).Set( new( TAG_SWF ) idWidgetActionHandler( options, WIDGET_ACTION_EVENT_SCROLL_DOWN_START_REPEATER, WIDGET_EVENT_SCROLL_DOWN ) );
	options->AddEventAction( WIDGET_EVENT_SCROLL_UP ).Set( new( TAG_SWF ) idWidgetActionHandler( options, WIDGET_ACTION_EVENT_SCROLL_UP_START_REPEATER, WIDGET_EVENT_SCROLL_UP ) );
	options->AddEventAction( WIDGET_EVENT_SCROLL_DOWN_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( options, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_DOWN_RELEASE ) );
	options->AddEventAction( WIDGET_EVENT_SCROLL_UP_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( options, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_UP_RELEASE ) );
	options->AddEventAction( WIDGET_EVENT_SCROLL_DOWN_LSTICK ).Set( new( TAG_SWF ) idWidgetActionHandler( options, WIDGET_ACTION_EVENT_SCROLL_DOWN_START_REPEATER, WIDGET_EVENT_SCROLL_DOWN_LSTICK ) );
	options->AddEventAction( WIDGET_EVENT_SCROLL_UP_LSTICK ).Set( new( TAG_SWF ) idWidgetActionHandler( options, WIDGET_ACTION_EVENT_SCROLL_UP_START_REPEATER, WIDGET_EVENT_SCROLL_UP_LSTICK ) );
	options->AddEventAction( WIDGET_EVENT_SCROLL_DOWN_LSTICK_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( options, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_DOWN_LSTICK_RELEASE ) );
	options->AddEventAction( WIDGET_EVENT_SCROLL_UP_LSTICK_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( options, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_UP_LSTICK_RELEASE ) );


	AddEventAction( WIDGET_EVENT_SCROLL_RIGHT ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_SCROLL_RIGHT_START_REPEATER, WIDGET_EVENT_SCROLL_RIGHT ) );
	AddEventAction( WIDGET_EVENT_SCROLL_RIGHT_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_RIGHT_RELEASE ) );
	AddEventAction( WIDGET_EVENT_SCROLL_LEFT ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_SCROLL_LEFT_START_REPEATER, WIDGET_EVENT_SCROLL_LEFT ) );
	AddEventAction( WIDGET_EVENT_SCROLL_LEFT_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_LEFT_RELEASE ) );
	AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_PRESS_FOCUSED, 0 );

}

/*
========================
idMenuScreen_Shell_Root::Update
========================
*/
void idMenuScreen_Shell_Root::Update()
{

	if( menuData != NULL )
	{
		idMenuWidget_CommandBar* cmdBar = menuData->GetCmdBar();
		if( cmdBar != NULL )
		{
			cmdBar->ClearAllButtons();
			idMenuWidget_CommandBar::buttonInfo_t* buttonInfo;

			if( !g_demoMode.GetBool() )
			{
				buttonInfo = cmdBar->GetButton( idMenuWidget_CommandBar::BUTTON_JOY2 );
				if( menuData->GetPlatform() != 2 )
				{
					buttonInfo->label = "#str_00395";
				}
				buttonInfo->action.Set( WIDGET_ACTION_GO_BACK );
			}

			buttonInfo = cmdBar->GetButton( idMenuWidget_CommandBar::BUTTON_JOY1 );
			if( menuData->GetPlatform() != 2 )
			{
				buttonInfo->label = "#str_SWF_SELECT";
			}
			buttonInfo->action.Set( WIDGET_ACTION_PRESS_FOCUSED );
		}
	}

	idMenuScreen::Update();
}

enum rootMenuCmds_t
{
	ROOT_CMD_START_DEMO,
	ROOT_CMD_START_DEMO2,
	ROOT_CMD_SETTINGS,
	ROOT_CMD_QUIT,
	ROOT_CMD_DEV,
	ROOT_CMD_CAMPAIGN,
	ROOT_CMD_MULTIPLAYER,
	ROOT_CMD_PLAYSTATION,
	ROOT_CMD_CREDITS
};

/*
========================
idMenuScreen_Shell_Root::ShowScreen
========================
*/
void idMenuScreen_Shell_Root::ShowScreen( const mainMenuTransition_t transitionType )
{

	if( menuData != NULL && menuData->GetPlatform() != 2 )
	{
		idList< idList< idStr, TAG_IDLIB_LIST_MENU >, TAG_IDLIB_LIST_MENU > menuOptions;
		idList< idStr > option;

		int index = 0;

		if( g_demoMode.GetBool() )
		{
			idMenuWidget_Button* buttonWidget = NULL;

			option.Append( "START DEMO" );	// START DEMO
			menuOptions.Append( option );
			options->GetChildByIndex( index ).ClearEventActions();
			options->GetChildByIndex( index ).AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, ROOT_CMD_START_DEMO );
			buttonWidget = dynamic_cast< idMenuWidget_Button* >( &options->GetChildByIndex( index ) );
			if( buttonWidget != NULL )
			{
				buttonWidget->SetDescription( "Launch the demo" );
			}
			index++;

			if( g_demoMode.GetInteger() == 2 )
			{
				option.Clear();
				option.Append( "START PRESS DEMO" );	// START DEMO
				menuOptions.Append( option );
				options->GetChildByIndex( index ).ClearEventActions();
				options->GetChildByIndex( index ).AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, ROOT_CMD_START_DEMO2 );
				buttonWidget = dynamic_cast< idMenuWidget_Button* >( &options->GetChildByIndex( index ) );
				if( buttonWidget != NULL )
				{
					buttonWidget->SetDescription( "Launch the press demo" );
				}
				index++;
			}

			option.Clear();
			option.Append( "#str_swf_settings" );	// settings
			menuOptions.Append( option );
			options->GetChildByIndex( index ).ClearEventActions();
			options->GetChildByIndex( index ).AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, ROOT_CMD_SETTINGS );
			buttonWidget = dynamic_cast< idMenuWidget_Button* >( &options->GetChildByIndex( index ) );
			if( buttonWidget != NULL )
			{
				buttonWidget->SetDescription( "#str_02206" );
			}
			index++;

			option.Clear();
			option.Append( "#str_swf_quit" );	// quit
			menuOptions.Append( option );
			options->GetChildByIndex( index ).ClearEventActions();
			options->GetChildByIndex( index ).AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, ROOT_CMD_QUIT );
			buttonWidget = dynamic_cast< idMenuWidget_Button* >( &options->GetChildByIndex( index ) );
			if( buttonWidget != NULL )
			{
				buttonWidget->SetDescription( "#str_01976" );
			}
			index++;

		}
		else
		{

			idMenuWidget_Button* buttonWidget = NULL;

#if !defined ( ID_RETAIL )
			option.Append( "DEV" );	// DEV
			menuOptions.Append( option );
			options->GetChildByIndex( index ).ClearEventActions();
			options->GetChildByIndex( index ).AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, ROOT_CMD_DEV );
			buttonWidget = dynamic_cast< idMenuWidget_Button* >( &options->GetChildByIndex( index ) );
			if( buttonWidget != NULL )
			{
				buttonWidget->SetDescription( "View a list of maps available for play" );
			}
			index++;
#endif

			option.Clear();
			option.Append( "#str_swf_campaign" );	// singleplayer
			menuOptions.Append( option );
			options->GetChildByIndex( index ).ClearEventActions();
			options->GetChildByIndex( index ).AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, ROOT_CMD_CAMPAIGN );
			buttonWidget = dynamic_cast< idMenuWidget_Button* >( &options->GetChildByIndex( index ) );
			if( buttonWidget != NULL )
			{
				buttonWidget->SetDescription( "#str_swf_campaign_desc" );
			}
			index++;

			option.Clear();
			option.Append( "#str_swf_multiplayer" );	// multiplayer
			menuOptions.Append( option );
			options->GetChildByIndex( index ).ClearEventActions();
			options->GetChildByIndex( index ).AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, ROOT_CMD_MULTIPLAYER );
			buttonWidget = dynamic_cast< idMenuWidget_Button* >( &options->GetChildByIndex( index ) );
			if( buttonWidget != NULL )
			{
				buttonWidget->SetDescription( "#str_02215" );
			}
			index++;

			option.Clear();
			option.Append( "#str_swf_settings" );	// settings
			menuOptions.Append( option );
			options->GetChildByIndex( index ).ClearEventActions();
			options->GetChildByIndex( index ).AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, ROOT_CMD_SETTINGS );
			buttonWidget = dynamic_cast< idMenuWidget_Button* >( &options->GetChildByIndex( index ) );
			if( buttonWidget != NULL )
			{
				buttonWidget->SetDescription( "#str_02206" );
			}
			index++;


			option.Clear();
			option.Append( "#str_swf_credits" );	// credits
			menuOptions.Append( option );
			options->GetChildByIndex( index ).ClearEventActions();
			options->GetChildByIndex( index ).AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, ROOT_CMD_CREDITS );
			buttonWidget = dynamic_cast< idMenuWidget_Button* >( &options->GetChildByIndex( index ) );
			if( buttonWidget != NULL )
			{
				buttonWidget->SetDescription( "#str_02219" );
			}
			index++;

			// only add quit option for PC
			option.Clear();
			option.Append( "#str_swf_quit" );	// quit
			menuOptions.Append( option );
			options->GetChildByIndex( index ).ClearEventActions();
			options->GetChildByIndex( index ).AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, ROOT_CMD_QUIT );
			buttonWidget = dynamic_cast< idMenuWidget_Button* >( &options->GetChildByIndex( index ) );
			if( buttonWidget != NULL )
			{
				buttonWidget->SetDescription( "#str_01976" );
			}
			index++;
		}
		options->SetListData( menuOptions );
	}
	else
	{
		idList< idList< idStr, TAG_IDLIB_LIST_MENU >, TAG_IDLIB_LIST_MENU > menuOptions;
		options->SetListData( menuOptions );
	}

	idMenuScreen::ShowScreen( transitionType );

	if( menuData != NULL && menuData->GetPlatform() == 2 )
	{
		idMenuHandler_Shell* shell = dynamic_cast< idMenuHandler_Shell* >( menuData );
		if( shell != NULL )
		{
			idMenuWidget_MenuBar* menuBar = shell->GetMenuBar();
			if( menuBar != NULL )
			{
				menuBar->SetFocusIndex( GetRootIndex() );
			}
		}
	}
}

/*
========================
idMenuScreen_Shell_Root::HideScreen
========================
*/
void idMenuScreen_Shell_Root::HideScreen( const mainMenuTransition_t transitionType )
{
	idMenuScreen::HideScreen( transitionType );
}

/*
========================
idMenuScreen_Shell_Root::HandleExitGameBtn
========================
*/
void idMenuScreen_Shell_Root::HandleExitGameBtn()
{
	class idSWFScriptFunction_QuitDialog : public idSWFScriptFunction_RefCounted
	{
	public:
		idSWFScriptFunction_QuitDialog( gameDialogMessages_t _msg, int _accept )
		{
			msg = _msg;
			accept = _accept;
		}
		idSWFScriptVar Call( idSWFScriptObject* thisObject, const idSWFParmList& parms )
		{
			common->Dialog().ClearDialog( msg );
			if( accept == 1 )
			{
				common->Quit();
			}
			else if( accept == -1 )
			{
				session->MoveToPressStart();
			}
			return idSWFScriptVar();
		}
	private:
		gameDialogMessages_t msg;
		int accept;
	};

	idStaticList< idSWFScriptFunction*, 4 > callbacks;
	idStaticList< idStrId, 4 > optionText;
	callbacks.Append( new( TAG_SWF ) idSWFScriptFunction_QuitDialog( GDM_QUIT_GAME, 1 ) );
	callbacks.Append( new( TAG_SWF ) idSWFScriptFunction_QuitDialog( GDM_QUIT_GAME, 0 ) );
	callbacks.Append( new( TAG_SWF ) idSWFScriptFunction_QuitDialog( GDM_QUIT_GAME, -1 ) );
	optionText.Append( idStrId( "#STR_SWF_ACCEPT" ) );
	optionText.Append( idStrId( "#STR_SWF_CANCEL" ) );
	optionText.Append( idStrId( "#str_swf_change_game" ) );

	common->Dialog().AddDynamicDialog( GDM_QUIT_GAME, callbacks, optionText, true, "" );
}

/*
========================
idMenuScreen_Shell_Root::GetRootIndex
========================
*/
int idMenuScreen_Shell_Root::GetRootIndex()
{
	if( options != NULL )
	{
		return options->GetFocusIndex();
	}

	return 0;
}

/*
========================
idMenuScreen_Shell_Root::SetRootIndex
========================
*/
void idMenuScreen_Shell_Root::SetRootIndex( int index )
{
	if( options != NULL )
	{
		options->SetFocusIndex( index );
	}
}

/*
========================
idMenuScreen_Shell_Root::HandleAction
========================
*/
bool idMenuScreen_Shell_Root::HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled )
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
		case WIDGET_ACTION_GO_BACK:
		{
			session->MoveToPressStart();
			return true;
		}
		case WIDGET_ACTION_PRESS_FOCUSED:
		{
			if( menuData->GetPlatform() == 2 )
			{

				idMenuHandler_Shell* shell = dynamic_cast< idMenuHandler_Shell* >( menuData );
				if( !shell )
				{
					return true;
				}

				idMenuWidget_MenuBar* menuBar = shell->GetMenuBar();

				if( !menuBar )
				{
					return true;
				}

				const idMenuWidget_MenuButton* buttonWidget = dynamic_cast< idMenuWidget_MenuButton* >( &menuBar->GetChildByIndex( menuBar->GetFocusIndex() ) );
				if( !buttonWidget )
				{
					return true;
				}

				idWidgetEvent pressEvent( WIDGET_EVENT_PRESS, 0, NULL, idSWFParmList() );
				menuBar->ReceiveEvent( pressEvent );
				return true;
			}
			break;
		}
		case WIDGET_ACTION_SCROLL_HORIZONTAL:
		{

			if( menuData->GetPlatform() != 2 )
			{
				return true;
			}

			idMenuHandler_Shell* shell = dynamic_cast< idMenuHandler_Shell* >( menuData );
			if( !shell )
			{
				return true;
			}

			idMenuWidget_MenuBar* menuBar = shell->GetMenuBar();

			if( !menuBar )
			{
				return true;
			}

			int index = menuBar->GetViewIndex();
			const int dir = parms[0].ToInteger();
#ifdef ID_RETAIL
			const int totalCount = menuBar->GetTotalNumberOfOptions() - 1;
#else
			const int totalCount = menuBar->GetTotalNumberOfOptions();
#endif
			index += dir;
			if( index < 0 )
			{
				index = totalCount - 1;
			}
			else if( index >= totalCount )
			{
				index = 0;
			}

			SetRootIndex( index );
			menuBar->SetViewIndex( index );
			menuBar->SetFocusIndex( index );

			return true;
		}
		case WIDGET_ACTION_COMMAND:
		{
			switch( parms[0].ToInteger() )
			{
				case ROOT_CMD_START_DEMO:
				{
					cmdSystem->AppendCommandText( va( "devmap %s %d\n", "demo/enpro_e3_2012", 1 ) );
					break;
				}
				case ROOT_CMD_START_DEMO2:
				{
					cmdSystem->AppendCommandText( va( "devmap %s %d\n", "game/le_hell", 2 ) );
					break;
				}
				case ROOT_CMD_SETTINGS:
				{
					menuData->SetNextScreen( SHELL_AREA_SETTINGS, MENU_TRANSITION_SIMPLE );
					break;
				}
				case ROOT_CMD_QUIT:
				{
					HandleExitGameBtn();
					break;
				}
				case ROOT_CMD_DEV:
				{
					menuData->SetNextScreen( SHELL_AREA_DEV, MENU_TRANSITION_SIMPLE );
					break;
				}
				case ROOT_CMD_CAMPAIGN:
				{
					menuData->SetNextScreen( SHELL_AREA_CAMPAIGN, MENU_TRANSITION_SIMPLE );
					break;
				}
				case ROOT_CMD_MULTIPLAYER:
				{
					const idLocalUser* masterUser = session->GetSignInManager().GetMasterLocalUser();

					if( masterUser == NULL )
					{
						break;
					}

					if( masterUser->GetOnlineCaps() & CAP_BLOCKED_PERMISSION )
					{
						common->Dialog().AddDialog( GDM_ONLINE_INCORRECT_PERMISSIONS, DIALOG_CONTINUE, NULL, NULL, true, __FUNCTION__, __LINE__, false );
					}
					else if( !masterUser->CanPlayOnline() )
					{
						class idSWFScriptFunction_Accept : public idSWFScriptFunction_RefCounted
						{
						public:
							idSWFScriptFunction_Accept() { }
							idSWFScriptVar Call( idSWFScriptObject* thisObject, const idSWFParmList& parms )
							{
								common->Dialog().ClearDialog( GDM_PLAY_ONLINE_NO_PROFILE );
								session->ShowOnlineSignin();
								return idSWFScriptVar();
							}
						};
						class idSWFScriptFunction_Cancel : public idSWFScriptFunction_RefCounted
						{
						public:
							idSWFScriptFunction_Cancel() { }
							idSWFScriptVar Call( idSWFScriptObject* thisObject, const idSWFParmList& parms )
							{
								common->Dialog().ClearDialog( GDM_PLAY_ONLINE_NO_PROFILE );
								return idSWFScriptVar();
							}
						};

						common->Dialog().AddDialog( GDM_PLAY_ONLINE_NO_PROFILE, DIALOG_ACCEPT_CANCEL, new( TAG_SWF ) idSWFScriptFunction_Accept(), new( TAG_SWF ) idSWFScriptFunction_Cancel(), false );
					}
					else
					{
						idMatchParameters matchParameters;
						matchParameters.matchFlags = DefaultPartyFlags;
						session->CreatePartyLobby( matchParameters );
					}
					break;
				}
				case ROOT_CMD_PLAYSTATION:
				{
					menuData->SetNextScreen( SHELL_AREA_PLAYSTATION, MENU_TRANSITION_SIMPLE );
					break;
				}
				case ROOT_CMD_CREDITS:
				{
					menuData->SetNextScreen( SHELL_AREA_CREDITS, MENU_TRANSITION_SIMPLE );
					break;
				}
			}
			return true;
		}
	}

	return idMenuWidget::HandleAction( action, event, widget, forceHandled );
}