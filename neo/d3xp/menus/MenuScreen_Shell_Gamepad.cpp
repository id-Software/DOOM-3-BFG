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

const static int NUM_CONTROLS_OPTIONS = 8;

enum gamepadMenuCmds_t
{
#ifndef ID_PC
	GAMEPAD_CMD_CONFIG,
#endif
	GAMEPAD_CMD_LEFTY,
	GAMEPAD_CMD_INVERT,
	GAMEPAD_CMD_VIBRATE,
	GAMEPAD_CMD_HOR_SENS,
	GAMEPAD_CMD_VERT_SENS,
	GAMEPAD_CMD_ACCELERATION,
	GAMEPAD_CMD_THRESHOLD,
};

/*
========================
idMenuScreen_Shell_Gamepad::Initialize
========================
*/
void idMenuScreen_Shell_Gamepad::Initialize( idMenuHandler* data )
{
	idMenuScreen::Initialize( data );

	if( data != NULL )
	{
		menuGUI = data->GetGUI();
	}

	SetSpritePath( "menuGamepad" );

	options = new( TAG_SWF ) idMenuWidget_DynamicList();
	options->SetNumVisibleOptions( NUM_CONTROLS_OPTIONS );
	options->SetSpritePath( GetSpritePath(), "info", "options" );
	options->SetWrappingAllowed( true );
	options->SetControlList( true );
	options->Initialize( data );
	AddChild( options );

	idMenuWidget_Help* const helpWidget = new( TAG_SWF ) idMenuWidget_Help();
	helpWidget->SetSpritePath( GetSpritePath(), "info", "helpTooltip" );
	AddChild( helpWidget );

	btnBack = new( TAG_SWF ) idMenuWidget_Button();
	btnBack->Initialize( data );
	idStr controls( idLocalization::GetString( "#str_04158" ) );
	controls.ToUpper();
	btnBack->SetLabel( controls );
	btnBack->SetSpritePath( GetSpritePath(), "info", "btnBack" );
	btnBack->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_GO_BACK );
	AddChild( btnBack );

	idMenuWidget_ControlButton* control;
#ifndef ID_PC
	control = new( TAG_SWF ) idMenuWidget_ControlButton();
	control->SetOptionType( OPTION_BUTTON_TEXT );
	control->SetLabel( "#str_swf_gamepad_config" );	// Gamepad Configuration
	control->SetDescription( "#str_swf_config_desc" );
	control->RegisterEventObserver( helpWidget );
	control->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, GAMEPAD_CMD_CONFIG );
	options->AddChild( control );
#endif

	control = new( TAG_SWF ) idMenuWidget_ControlButton();
	control->SetOptionType( OPTION_SLIDER_TOGGLE );
	control->SetLabel( "#str_swf_lefty_flip" );
	control->SetDataSource( &gamepadData, idMenuDataSource_GamepadSettings::GAMEPAD_FIELD_LEFTY );
	control->SetupEvents( DEFAULT_REPEAT_TIME, options->GetChildren().Num() );
	control->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, GAMEPAD_CMD_LEFTY );
	control->RegisterEventObserver( helpWidget );
	options->AddChild( control );

	control = new( TAG_SWF ) idMenuWidget_ControlButton();
	control->SetOptionType( OPTION_SLIDER_TOGGLE );
	control->SetLabel( "#str_swf_invert_gamepad" );
	control->SetDataSource( &gamepadData, idMenuDataSource_GamepadSettings::GAMEPAD_FIELD_INVERT );
	control->SetupEvents( DEFAULT_REPEAT_TIME, options->GetChildren().Num() );
	control->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, GAMEPAD_CMD_INVERT );
	control->RegisterEventObserver( helpWidget );
	options->AddChild( control );

	control = new( TAG_SWF ) idMenuWidget_ControlButton();
	control->SetOptionType( OPTION_SLIDER_TOGGLE );
	control->SetLabel( "#str_swf_vibration" );
	control->SetDataSource( &gamepadData, idMenuDataSource_GamepadSettings::GAMEPAD_FIELD_VIBRATE );
	control->SetupEvents( DEFAULT_REPEAT_TIME, options->GetChildren().Num() );
	control->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, GAMEPAD_CMD_VIBRATE );
	control->RegisterEventObserver( helpWidget );
	options->AddChild( control );

	control = new( TAG_SWF ) idMenuWidget_ControlButton();
	control->SetOptionType( OPTION_SLIDER_BAR );
	control->SetLabel( "#str_swf_hor_sens" );
	control->SetDataSource( &gamepadData, idMenuDataSource_GamepadSettings::GAMEPAD_FIELD_HOR_SENS );
	control->SetupEvents( 2, options->GetChildren().Num() );
	control->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, GAMEPAD_CMD_HOR_SENS );
	control->RegisterEventObserver( helpWidget );
	options->AddChild( control );

	control = new( TAG_SWF ) idMenuWidget_ControlButton();
	control->SetOptionType( OPTION_SLIDER_BAR );
	control->SetLabel( "#str_swf_vert_sens" );
	control->SetDataSource( &gamepadData, idMenuDataSource_GamepadSettings::GAMEPAD_FIELD_VERT_SENS );
	control->SetupEvents( 2, options->GetChildren().Num() );
	control->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, GAMEPAD_CMD_VERT_SENS );
	control->RegisterEventObserver( helpWidget );
	options->AddChild( control );

	control = new( TAG_SWF ) idMenuWidget_ControlButton();
	control->SetOptionType( OPTION_SLIDER_TOGGLE );
	control->SetLabel( "#str_swf_joy_gammaLook" );
	control->SetDataSource( &gamepadData, idMenuDataSource_GamepadSettings::GAMEPAD_FIELD_ACCELERATION );
	control->SetupEvents( DEFAULT_REPEAT_TIME, options->GetChildren().Num() );
	control->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, GAMEPAD_CMD_ACCELERATION );
	control->RegisterEventObserver( helpWidget );
	options->AddChild( control );

	control = new( TAG_SWF ) idMenuWidget_ControlButton();
	control->SetOptionType( OPTION_SLIDER_TOGGLE );
	control->SetLabel( "#str_swf_joy_mergedThreshold" );
	control->SetDataSource( &gamepadData, idMenuDataSource_GamepadSettings::GAMEPAD_FIELD_THRESHOLD );
	control->SetupEvents( DEFAULT_REPEAT_TIME, options->GetChildren().Num() );
	control->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, GAMEPAD_CMD_THRESHOLD );
	control->RegisterEventObserver( helpWidget );
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
idMenuScreen_Shell_Gamepad::Update
========================
*/
void idMenuScreen_Shell_Gamepad::Update()
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
			heading->SetText( "#str_swf_gamepad_heading" );	// CONTROLS
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
idMenuScreen_Shell_Gamepad::ShowScreen
========================
*/
void idMenuScreen_Shell_Gamepad::ShowScreen( const mainMenuTransition_t transitionType )
{
	gamepadData.LoadData();
	idMenuScreen::ShowScreen( transitionType );
}

/*
========================
idMenuScreen_Shell_Gamepad::HideScreen
========================
*/
void idMenuScreen_Shell_Gamepad::HideScreen( const mainMenuTransition_t transitionType )
{

	if( gamepadData.IsDataChanged() )
	{
		gamepadData.CommitData();
	}

	if( menuData != NULL )
	{
		idMenuHandler_Shell* handler = dynamic_cast< idMenuHandler_Shell* >( menuData );
		if( handler != NULL )
		{
			handler->SetupPCOptions();
		}
	}

	idMenuScreen::HideScreen( transitionType );
}

/*
========================
idMenuScreen_Shell_Gamepad::HandleAction
========================
*/
bool idMenuScreen_Shell_Gamepad::HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled )
{

	if( menuData == NULL )
	{
		return true;
	}

	if( menuData->ActiveScreen() != SHELL_AREA_GAMEPAD )
	{
		return false;
	}

	widgetAction_t actionType = action.GetType();
	const idSWFParmList& parms = action.GetParms();

	switch( actionType )
	{
		case WIDGET_ACTION_GO_BACK:
		{
			menuData->SetNextScreen( SHELL_AREA_CONTROLS, MENU_TRANSITION_SIMPLE );
			return true;
		}
		case WIDGET_ACTION_COMMAND:
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

			switch( parms[0].ToInteger() )
			{
#ifndef ID_PC
				case GAMEPAD_CMD_CONFIG:
				{
					menuData->SetNextScreen( SHELL_AREA_CONTROLLER_LAYOUT, MENU_TRANSITION_SIMPLE );
					break;
				}
#endif
				case GAMEPAD_CMD_INVERT:
				{
					gamepadData.AdjustField( idMenuDataSource_GamepadSettings::GAMEPAD_FIELD_INVERT, 1 );
					options->Update();
					break;
				}
				case GAMEPAD_CMD_LEFTY:
				{
					gamepadData.AdjustField( idMenuDataSource_GamepadSettings::GAMEPAD_FIELD_LEFTY, 1 );
					options->Update();
					break;
				}
				case GAMEPAD_CMD_VIBRATE:
				{
					gamepadData.AdjustField( idMenuDataSource_GamepadSettings::GAMEPAD_FIELD_VIBRATE, 1 );
					options->Update();
					break;
				}
				case GAMEPAD_CMD_HOR_SENS:
				{
					gamepadData.AdjustField( idMenuDataSource_GamepadSettings::GAMEPAD_FIELD_HOR_SENS, 1 );
					options->Update();
					break;
				}
				case GAMEPAD_CMD_VERT_SENS:
				{
					gamepadData.AdjustField( idMenuDataSource_GamepadSettings::GAMEPAD_FIELD_VERT_SENS, 1 );
					options->Update();
					break;
				}
				case GAMEPAD_CMD_ACCELERATION:
				{
					gamepadData.AdjustField( idMenuDataSource_GamepadSettings::GAMEPAD_FIELD_ACCELERATION, 1 );
					options->Update();
					break;
				}
				case GAMEPAD_CMD_THRESHOLD:
				{
					gamepadData.AdjustField( idMenuDataSource_GamepadSettings::GAMEPAD_FIELD_THRESHOLD, 1 );
					options->Update();
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
	}

	return idMenuWidget::HandleAction( action, event, widget, forceHandled );
}

/////////////////////////////////
// SCREEN SETTINGS
/////////////////////////////////

extern idCVar in_invertLook;
extern idCVar in_joystickRumble;
extern idCVar joy_pitchSpeed;
extern idCVar joy_yawSpeed;
extern idCVar joy_gammaLook;
extern idCVar joy_mergedThreshold;

/*
========================
idMenuScreen_Shell_Gamepad::idMenuDataSource_AudioSettings::idMenuDataSource_AudioSettings
========================
*/
idMenuScreen_Shell_Gamepad::idMenuDataSource_GamepadSettings::idMenuDataSource_GamepadSettings()
{
	fields.SetNum( MAX_GAMEPAD_FIELDS );
	originalFields.SetNum( MAX_GAMEPAD_FIELDS );
}

/*
========================
idMenuScreen_Shell_Gamepad::idMenuDataSource_AudioSettings::LoadData
========================
*/
void idMenuScreen_Shell_Gamepad::idMenuDataSource_GamepadSettings::LoadData()
{
	idPlayerProfile* profile = session->GetProfileFromMasterLocalUser();

	fields[ GAMEPAD_FIELD_INVERT ].SetBool( in_invertLook.GetBool() );
	fields[ GAMEPAD_FIELD_LEFTY ].SetBool( profile ? profile->GetLeftyFlip() : false );
	fields[ GAMEPAD_FIELD_VIBRATE ].SetBool( in_joystickRumble.GetBool() );
	fields[ GAMEPAD_FIELD_HOR_SENS ].SetFloat( 100.0f * ( ( joy_yawSpeed.GetFloat() - 100.0f ) / 300.0f ) );
	fields[ GAMEPAD_FIELD_VERT_SENS ].SetFloat( 100.0f * ( ( joy_pitchSpeed.GetFloat() - 60.0f ) / 200.0f ) );
	fields[ GAMEPAD_FIELD_ACCELERATION ].SetBool( joy_gammaLook.GetBool() );
	fields[ GAMEPAD_FIELD_THRESHOLD ].SetBool( joy_mergedThreshold.GetBool() );

	originalFields = fields;
}

/*
========================
idMenuScreen_Shell_Gamepad::idMenuDataSource_AudioSettings::CommitData
========================
*/
void idMenuScreen_Shell_Gamepad::idMenuDataSource_GamepadSettings::CommitData()
{

	in_invertLook.SetBool( fields[ GAMEPAD_FIELD_INVERT ].ToBool() );
	in_joystickRumble.SetBool( fields[ GAMEPAD_FIELD_VIBRATE ].ToBool() );
	joy_yawSpeed.SetFloat( ( ( fields[ GAMEPAD_FIELD_HOR_SENS ].ToFloat() / 100.0f ) * 300.0f ) + 100.0f );
	joy_pitchSpeed.SetFloat( ( ( fields[ GAMEPAD_FIELD_VERT_SENS ].ToFloat() / 100.0f ) * 200.0f ) + 60.0f );
	joy_gammaLook.SetBool( fields[ GAMEPAD_FIELD_ACCELERATION ].ToBool() );
	joy_mergedThreshold.SetBool( fields[ GAMEPAD_FIELD_THRESHOLD ].ToBool() );

	idPlayerProfile* profile = session->GetProfileFromMasterLocalUser();
	if( profile != NULL )
	{
		profile->SetLeftyFlip( fields[ GAMEPAD_FIELD_LEFTY ].ToBool() );
	}
	cvarSystem->SetModifiedFlags( CVAR_ARCHIVE );

	// make the committed fields into the backup fields
	originalFields = fields;
}

/*
========================
idMenuScreen_Shell_Gamepad::idMenuDataSource_AudioSettings::AdjustField
========================
*/
void idMenuScreen_Shell_Gamepad::idMenuDataSource_GamepadSettings::AdjustField( const int fieldIndex, const int adjustAmount )
{

	if( fieldIndex == GAMEPAD_FIELD_INVERT || fieldIndex == GAMEPAD_FIELD_LEFTY || fieldIndex == GAMEPAD_FIELD_VIBRATE || fieldIndex == GAMEPAD_FIELD_ACCELERATION || fieldIndex == GAMEPAD_FIELD_THRESHOLD )
	{
		fields[ fieldIndex ].SetBool( !fields[ fieldIndex ].ToBool() );
	}
	else
	{
		float newValue = idMath::ClampFloat( 0.0f, 100.0f, fields[ fieldIndex ].ToFloat() + adjustAmount );
		fields[ fieldIndex ].SetFloat( newValue );
	}
}

/*
========================
idMenuScreen_Shell_Gamepad::idMenuDataSource_AudioSettings::IsDataChanged
========================
*/
bool idMenuScreen_Shell_Gamepad::idMenuDataSource_GamepadSettings::IsDataChanged() const
{

	if( fields[ GAMEPAD_FIELD_INVERT ].ToBool() != originalFields[ GAMEPAD_FIELD_INVERT ].ToBool() )
	{
		return true;
	}

	if( fields[ GAMEPAD_FIELD_LEFTY ].ToBool() != originalFields[ GAMEPAD_FIELD_LEFTY ].ToBool() )
	{
		return true;
	}

	if( fields[ GAMEPAD_FIELD_VIBRATE ].ToBool() != originalFields[ GAMEPAD_FIELD_VIBRATE ].ToBool() )
	{
		return true;
	}

	if( fields[ GAMEPAD_FIELD_HOR_SENS ].ToFloat() != originalFields[ GAMEPAD_FIELD_HOR_SENS ].ToFloat() )
	{
		return true;
	}

	if( fields[ GAMEPAD_FIELD_VERT_SENS ].ToFloat() != originalFields[ GAMEPAD_FIELD_VERT_SENS ].ToFloat() )
	{
		return true;
	}

	if( fields[ GAMEPAD_FIELD_ACCELERATION ].ToBool() != originalFields[ GAMEPAD_FIELD_ACCELERATION ].ToBool() )
	{
		return true;
	}

	if( fields[ GAMEPAD_FIELD_THRESHOLD ].ToBool() != originalFields[ GAMEPAD_FIELD_THRESHOLD ].ToBool() )
	{
		return true;
	}

	return false;
}