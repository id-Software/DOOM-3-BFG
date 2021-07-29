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

enum contorlsMenuCmds_t
{
	CONTROLS_CMD_BINDINGS,
	CONTROLS_CMD_GAMEPAD,
	CONTROLS_CMD_GAMEPAD_ENABLED,
	CONTROLS_CMD_INVERT,
	CONTROLS_CMD_MOUSE_SENS
};

/*
========================
idMenuScreen_Shell_Controls::Initialize
========================
*/
void idMenuScreen_Shell_Controls::Initialize( idMenuHandler* data )
{
	idMenuScreen::Initialize( data );

	if( data != NULL )
	{
		menuGUI = data->GetGUI();
	}

	SetSpritePath( "menuControls" );

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
	btnBack->SetLabel( "#str_swf_settings" );
	btnBack->SetSpritePath( GetSpritePath(), "info", "btnBack" );
	btnBack->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_GO_BACK );
	AddChild( btnBack );

	idMenuWidget_ControlButton* control;
	control = new( TAG_SWF ) idMenuWidget_ControlButton();
	control->SetOptionType( OPTION_BUTTON_TEXT );
	control->SetLabel( "#str_swf_keyboard" );	// KEY BINDINGS
	control->SetDescription( "#str_swf_binding_desc" );
	control->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, CONTROLS_CMD_BINDINGS );
	control->RegisterEventObserver( helpWidget );
	options->AddChild( control );

	control = new( TAG_SWF ) idMenuWidget_ControlButton();
	control->SetOptionType( OPTION_BUTTON_TEXT );
	control->SetLabel( "#str_swf_gamepad" );	// Gamepad
	control->SetDescription( "#str_swf_gamepad_desc" );
	control->RegisterEventObserver( helpWidget );
	control->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, CONTROLS_CMD_GAMEPAD );
	options->AddChild( control );

	control = new( TAG_SWF ) idMenuWidget_ControlButton();
	control->SetOptionType( OPTION_SLIDER_TOGGLE );
	control->SetLabel( "#str_swf_gamepad_enabled" );	// Gamepad Enabled
	control->SetDataSource( &controlData, idMenuDataSource_ControlSettings::CONTROLS_FIELD_GAMEPAD_ENABLED );
	control->SetupEvents( DEFAULT_REPEAT_TIME, options->GetChildren().Num() );
	control->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, CONTROLS_CMD_GAMEPAD_ENABLED );
	control->RegisterEventObserver( helpWidget );
	options->AddChild( control );

	control = new( TAG_SWF ) idMenuWidget_ControlButton();
	control->SetOptionType( OPTION_SLIDER_TOGGLE );
	control->SetLabel( "#str_swf_invert_mouse" );	// Invert Mouse
	control->SetDataSource( &controlData, idMenuDataSource_ControlSettings::CONTROLS_FIELD_INVERT_MOUSE );
	control->SetupEvents( DEFAULT_REPEAT_TIME, options->GetChildren().Num() );
	control->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, CONTROLS_CMD_INVERT );
	control->RegisterEventObserver( helpWidget );
	options->AddChild( control );

	control = new( TAG_SWF ) idMenuWidget_ControlButton();
	control->SetOptionType( OPTION_SLIDER_BAR );
	control->SetLabel( "#str_swf_mouse_sens" );	// Mouse Sensitivity
	control->SetDataSource( &controlData, idMenuDataSource_ControlSettings::CONTROLS_FIELD_MOUSE_SENS );
	control->SetupEvents( 2, options->GetChildren().Num() );
	control->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, CONTROLS_CMD_MOUSE_SENS );
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
idMenuScreen_Shell_Controls::Update
========================
*/
void idMenuScreen_Shell_Controls::Update()
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
			idStr controls( idLocalization::GetString( "#str_04158" ) );
			controls.ToUpper();
			heading->SetText( controls );	// CONTROLS
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
idMenuScreen_Shell_Controls::ShowScreen
========================
*/
void idMenuScreen_Shell_Controls::ShowScreen( const mainMenuTransition_t transitionType )
{
	controlData.LoadData();
	idMenuScreen::ShowScreen( transitionType );
}

/*
========================
idMenuScreen_Shell_Controls::HideScreen
========================
*/
void idMenuScreen_Shell_Controls::HideScreen( const mainMenuTransition_t transitionType )
{

	if( controlData.IsDataChanged() )
	{
		controlData.CommitData();
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
idMenuScreen_Shell_Controls::HandleAction
========================
*/
bool idMenuScreen_Shell_Controls::HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled )
{

	if( menuData == NULL )
	{
		return true;
	}

	if( menuData->ActiveScreen() != SHELL_AREA_CONTROLS )
	{
		return false;
	}

	widgetAction_t actionType = action.GetType();
	const idSWFParmList& parms = action.GetParms();

	switch( actionType )
	{
		case WIDGET_ACTION_GO_BACK:
		{
			menuData->SetNextScreen( SHELL_AREA_SETTINGS, MENU_TRANSITION_SIMPLE );
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
				case CONTROLS_CMD_BINDINGS:
				{
					menuData->SetNextScreen( SHELL_AREA_KEYBOARD, MENU_TRANSITION_SIMPLE );
					break;
				}
				case CONTROLS_CMD_GAMEPAD:
				{
					menuData->SetNextScreen( SHELL_AREA_GAMEPAD, MENU_TRANSITION_SIMPLE );
					break;
				}
				case CONTROLS_CMD_INVERT:
				{
					controlData.AdjustField( idMenuDataSource_ControlSettings::CONTROLS_FIELD_INVERT_MOUSE, 1 );
					if( options != NULL )
					{
						options->Update();
					}
					break;
				}
				case CONTROLS_CMD_MOUSE_SENS:
				{
					controlData.AdjustField( idMenuDataSource_ControlSettings::CONTROLS_FIELD_MOUSE_SENS, 1 );
					if( options != NULL )
					{
						options->Update();
					}
					break;
				}
				case CONTROLS_CMD_GAMEPAD_ENABLED:
				{
					controlData.AdjustField( idMenuDataSource_ControlSettings::CONTROLS_FIELD_GAMEPAD_ENABLED, 1 );
					if( options != NULL )
					{
						options->Update();
					}
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

extern idCVar in_mouseInvertLook;
extern idCVar in_mouseSpeed;
extern idCVar in_useJoystick;

/*
========================
idMenuScreen_Shell_Controls::idMenuDataSource_AudioSettings::idMenuDataSource_AudioSettings
========================
*/
idMenuScreen_Shell_Controls::idMenuDataSource_ControlSettings::idMenuDataSource_ControlSettings()
{
	fields.SetNum( MAX_CONTROL_FIELDS );
	originalFields.SetNum( MAX_CONTROL_FIELDS );
}

/*
========================
idMenuScreen_Shell_Controls::idMenuDataSource_AudioSettings::LoadData
========================
*/
void idMenuScreen_Shell_Controls::idMenuDataSource_ControlSettings::LoadData()
{
	fields[ CONTROLS_FIELD_INVERT_MOUSE ].SetBool( in_mouseInvertLook.GetBool() );
	float mouseSpeed = ( ( in_mouseSpeed.GetFloat() - 0.25f ) / ( 4.0f - 0.25 ) ) * 100.0f;
	fields[ CONTROLS_FIELD_MOUSE_SENS ].SetFloat( mouseSpeed );
	fields[ CONTROLS_FIELD_GAMEPAD_ENABLED ].SetBool( in_useJoystick.GetBool() );

	originalFields = fields;
}

/*
========================
idMenuScreen_Shell_Controls::idMenuDataSource_AudioSettings::CommitData
========================
*/
void idMenuScreen_Shell_Controls::idMenuDataSource_ControlSettings::CommitData()
{

	in_mouseInvertLook.SetBool( fields[ CONTROLS_FIELD_INVERT_MOUSE ].ToBool() );
	float mouseSpeed = 0.25f + ( ( 4.0f - 0.25 ) * ( fields[ CONTROLS_FIELD_MOUSE_SENS ].ToFloat() / 100.0f ) );
	in_mouseSpeed.SetFloat( mouseSpeed );
	in_useJoystick.SetBool( fields[ CONTROLS_FIELD_GAMEPAD_ENABLED ].ToBool() );

	cvarSystem->SetModifiedFlags( CVAR_ARCHIVE );

	// make the committed fields into the backup fields
	originalFields = fields;
}

/*
========================
idMenuScreen_Shell_Controls::idMenuDataSource_AudioSettings::AdjustField
========================
*/
void idMenuScreen_Shell_Controls::idMenuDataSource_ControlSettings::AdjustField( const int fieldIndex, const int adjustAmount )
{
	if( fieldIndex == CONTROLS_FIELD_INVERT_MOUSE || fieldIndex == CONTROLS_FIELD_GAMEPAD_ENABLED )
	{
		fields[ fieldIndex ].SetBool( !fields[ fieldIndex ].ToBool() );
	}
	else if( fieldIndex == CONTROLS_FIELD_MOUSE_SENS )
	{
		float newValue = idMath::ClampFloat( 0.0f, 100.0f, fields[ fieldIndex ].ToFloat() + adjustAmount );
		fields[ fieldIndex ].SetFloat( newValue );
	}
}

/*
========================
idMenuScreen_Shell_Controls::idMenuDataSource_AudioSettings::IsDataChanged
========================
*/
bool idMenuScreen_Shell_Controls::idMenuDataSource_ControlSettings::IsDataChanged() const
{

	if( fields[ CONTROLS_FIELD_INVERT_MOUSE ].ToBool() != originalFields[ CONTROLS_FIELD_INVERT_MOUSE ].ToBool() )
	{
		return true;
	}

	if( fields[ CONTROLS_FIELD_MOUSE_SENS ].ToFloat() != originalFields[ CONTROLS_FIELD_MOUSE_SENS ].ToFloat() )
	{
		return true;
	}

	if( fields[ CONTROLS_FIELD_GAMEPAD_ENABLED ].ToFloat() != originalFields[ CONTROLS_FIELD_GAMEPAD_ENABLED ].ToFloat() )
	{
		return true;
	}

	return false;
}