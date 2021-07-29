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
#include "../../renderer/RenderCommon.h"

const static int NUM_SETTING_OPTIONS = 7;

enum settingMenuCmds_t
{
	SETTING_CMD_CONTROLS,
	SETTING_CMD_GAMEPLAY,
	SETTING_CMD_SYSTEM,
	SETTING_CMD_3D,
};

/*
========================
idMenuScreen_Shell_Resolution::Initialize
========================
*/
void idMenuScreen_Shell_Resolution::Initialize( idMenuHandler* data )
{
	idMenuScreen::Initialize( data );

	if( data != NULL )
	{
		menuGUI = data->GetGUI();
	}

	SetSpritePath( "menuResolution" );

	options = new( TAG_SWF ) idMenuWidget_DynamicList();
	options->SetNumVisibleOptions( NUM_SETTING_OPTIONS );
	options->SetSpritePath( GetSpritePath(), "info", "options" );
	options->SetWrappingAllowed( true );

	while( options->GetChildren().Num() < NUM_SETTING_OPTIONS )
	{
		idMenuWidget_Button* const buttonWidget = new( TAG_SWF ) idMenuWidget_Button();
		buttonWidget->Initialize( data );
		buttonWidget->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_PRESS_FOCUSED, options->GetChildren().Num() );
		options->AddChild( buttonWidget );
	}
	options->Initialize( data );

	AddChild( options );

	btnBack = new( TAG_SWF ) idMenuWidget_Button();
	btnBack->Initialize( data );
	btnBack->SetLabel( "#str_00183" );
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
idMenuScreen_Shell_Resolution::Update
========================
*/
void idMenuScreen_Shell_Resolution::Update()
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
			heading->SetText( "#str_02154" );
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
idMenuScreen_Shell_Resolution::ShowScreen
========================
*/
void idMenuScreen_Shell_Resolution::ShowScreen( const mainMenuTransition_t transitionType )
{


	originalOption.fullscreen = r_fullscreen.GetInteger();
	originalOption.vidmode = r_vidMode.GetInteger();

	idList< idList< idStr, TAG_IDLIB_LIST_MENU >, TAG_IDLIB_LIST_MENU > menuOptions;
	menuOptions.Alloc().Alloc() = "#str_swf_disabled";
	optionData.Append( optionData_t( 0, 0 ) );

	int viewIndex = 0;
	idList< idList<vidMode_t> > displays;
	for( int displayNum = 0 ; ; displayNum++ )
	{
		idList<vidMode_t>& modeList = displays.Alloc();
		if( !R_GetModeListForDisplay( displayNum, modeList ) )
		{
			displays.RemoveIndex( displays.Num() - 1 );
			break;
		}
	}
	for( int displayNum = 0 ; displayNum < displays.Num(); displayNum++ )
	{
		idList<vidMode_t>& modeList = displays[displayNum];
		for( int i = 0; i < modeList.Num(); i++ )
		{
			const optionData_t thisOption( displayNum + 1, i );
			if( originalOption == thisOption )
			{
				viewIndex = menuOptions.Num();
			}
			idStr str;
			if( displays.Num() > 1 )
			{
				str.Append( va( "%s %i: ", idLocalization::GetString( "#str_swf_monitor" ), displayNum + 1 ) );
			}
			str.Append( va( "%4i x %4i", modeList[i].width, modeList[i].height ) );
			if( modeList[i].displayHz != 60 )
			{
				str.Append( va( " @ %dhz", modeList[i].displayHz ) );
			}
			menuOptions.Alloc().Alloc() = str;
			optionData.Append( thisOption );
		}
	}

	options->SetListData( menuOptions );
	options->SetViewIndex( viewIndex );
	const int topOfLastPage = menuOptions.Num() - NUM_SETTING_OPTIONS;
	if( viewIndex < NUM_SETTING_OPTIONS )
	{
		options->SetViewOffset( 0 );
		options->SetFocusIndex( viewIndex );
	}
	else if( viewIndex >= topOfLastPage )
	{
		options->SetViewOffset( topOfLastPage );
		options->SetFocusIndex( viewIndex - topOfLastPage );
	}
	else
	{
		options->SetViewOffset( viewIndex );
		options->SetFocusIndex( 0 );
	}

	idMenuScreen::ShowScreen( transitionType );
}

/*
========================
idMenuScreen_Shell_Resolution::HideScreen
========================
*/
void idMenuScreen_Shell_Resolution::HideScreen( const mainMenuTransition_t transitionType )
{
	idMenuScreen::HideScreen( transitionType );
}

/*
========================
idMenuScreen_Shell_Resolution::HandleAction h
========================
*/
bool idMenuScreen_Shell_Resolution::HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled )
{

	if( menuData == NULL )
	{
		return true;
	}

	if( menuData->ActiveScreen() != SHELL_AREA_RESOLUTION )
	{
		return false;
	}

	widgetAction_t actionType = action.GetType();
	const idSWFParmList& parms = action.GetParms();

	switch( actionType )
	{
		case WIDGET_ACTION_GO_BACK:
		{
			menuData->SetNextScreen( SHELL_AREA_SYSTEM_OPTIONS, MENU_TRANSITION_SIMPLE );
			return true;
		}
		case WIDGET_ACTION_PRESS_FOCUSED:
		{
			if( options != NULL )
			{
				int selectionIndex = options->GetFocusIndex();
				if( parms.Num() == 1 )
				{
					selectionIndex = parms[0].ToInteger();
				}

				if( options->GetFocusIndex() != selectionIndex )
				{
					options->SetFocusIndex( selectionIndex );
					options->SetViewIndex( options->GetViewOffset() + selectionIndex );
				}
				const optionData_t& currentOption = optionData[options->GetViewIndex()];

				if( currentOption == originalOption )
				{
					// No change
					menuData->SetNextScreen( SHELL_AREA_SYSTEM_OPTIONS, MENU_TRANSITION_SIMPLE );
				}
				else if( currentOption.fullscreen == 0 )
				{
					// Changing to windowed mode
					r_fullscreen.SetInteger( 0 );
					cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "vid_restart\n" );
					menuData->SetNextScreen( SHELL_AREA_SYSTEM_OPTIONS, MENU_TRANSITION_SIMPLE );
				}
				else
				{
					// Changing to fullscreen mode
					r_fullscreen.SetInteger( currentOption.fullscreen );
					r_vidMode.SetInteger( currentOption.vidmode );
					cvarSystem->ClearModifiedFlags( CVAR_ARCHIVE );
					cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "vid_restart\n" );

					class idSWFFuncAcceptVideoChanges : public idSWFScriptFunction_RefCounted
					{
					public:
						idSWFFuncAcceptVideoChanges( idMenuHandler* _menu, gameDialogMessages_t _msg, const optionData_t& _optionData, bool _accept )
						{
							menuHandler = _menu;
							msg = _msg;
							optionData = _optionData;
							accept = _accept;
						}
						idSWFScriptVar Call( idSWFScriptObject* thisObject, const idSWFParmList& parms )
						{
							common->Dialog().ClearDialog( msg );
							if( accept )
							{
								cvarSystem->SetModifiedFlags( CVAR_ARCHIVE );
								if( menuHandler != NULL )
								{
									menuHandler->SetNextScreen( SHELL_AREA_SYSTEM_OPTIONS, MENU_TRANSITION_SIMPLE );
								}
							}
							else
							{
								r_fullscreen.SetInteger( optionData.fullscreen );
								r_vidMode.SetInteger( optionData.vidmode );
								cvarSystem->ClearModifiedFlags( CVAR_ARCHIVE );
								cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "vid_restart\n" );
							}
							return idSWFScriptVar();
						}
					private:
						idMenuHandler* menuHandler;
						gameDialogMessages_t msg;
						optionData_t optionData;
						bool accept;
					};
					common->Dialog().AddDialog( GDM_CONFIRM_VIDEO_CHANGES, DIALOG_TIMER_ACCEPT_REVERT, new( TAG_SWF ) idSWFFuncAcceptVideoChanges( menuData, GDM_CONFIRM_VIDEO_CHANGES, currentOption, true ), new( TAG_SWF ) idSWFFuncAcceptVideoChanges( menuData, GDM_CONFIRM_VIDEO_CHANGES, originalOption, false ), false );
				}
				return true;
			}
			return true;
		}
	}

	return idMenuWidget::HandleAction( action, event, widget, forceHandled );
}