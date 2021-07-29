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

const static int NUM_LAYOUT_OPTIONS = 1;

const static int MAX_CONTROLLER_CONFIGS = 2;

typedef struct
{
	const char* textField;
	int keyNum;
} gamepadBindInfo_t;

static gamepadBindInfo_t gamepadBinds[] =
{
	{ "txtJoy1",		K_JOY1			},
	{ "txtJoy2",		K_JOY2			},
	{ "txtJoy3",		K_JOY3			},
	{ "txtJoy4",		K_JOY4			},
	{ "txtDpad",		K_JOY_DPAD_UP	},
	{ "txtStart",		K_JOY9			},
	{ "txtBack",		K_JOY10			},
	{ "txtLClick",		K_JOY7			},
	{ "txtRClick",		K_JOY8			},
	{ "txtLBumper",		K_JOY5			},
	{ "txtRBumper",		K_JOY6			},
	{ "txtLStick",		K_JOY_STICK1_UP	},
	{ "txtRStick",		K_JOY_STICK2_UP	},
	{ "txtLTrigger",	K_JOY_TRIGGER1	},
	{ "txtRTrigger",	K_JOY_TRIGGER2	}
};

static const int numGamepadBinds = sizeof( gamepadBinds ) / sizeof( gamepadBinds[0] );

/*
========================
idMenuScreen_Shell_ControllerLayout::Initialize
========================
*/
void idMenuScreen_Shell_ControllerLayout::Initialize( idMenuHandler* data )
{
	idMenuScreen::Initialize( data );

	if( data != NULL )
	{
		menuGUI = data->GetGUI();
	}

	SetSpritePath( "menuControllerLayout" );

	options = new( TAG_SWF ) idMenuWidget_DynamicList();
	options->SetNumVisibleOptions( NUM_LAYOUT_OPTIONS );
	options->SetSpritePath( GetSpritePath(), "info", "controlInfo", "options" );
	options->SetWrappingAllowed( true );
	options->SetControlList( true );
	options->Initialize( data );
	AddChild( options );

	btnBack = new( TAG_SWF ) idMenuWidget_Button();
	btnBack->Initialize( data );
	btnBack->SetLabel( "#str_swf_gamepad_heading" );	// CONTROLS
	btnBack->SetSpritePath( GetSpritePath(), "info", "btnBack" );
	btnBack->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_GO_BACK );
	AddChild( btnBack );

	idMenuWidget_ControlButton* control = new( TAG_SWF ) idMenuWidget_ControlButton();
	control->SetOptionType( OPTION_BUTTON_FULL_TEXT_SLIDER );
	control->SetLabel( "CONTROL LAYOUT" );	// Auto Weapon Reload
	control->SetDataSource( &layoutData, idMenuDataSource_LayoutSettings::LAYOUT_FIELD_LAYOUT );
	control->SetupEvents( DEFAULT_REPEAT_TIME, options->GetChildren().Num() );
	control->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_PRESS_FOCUSED, options->GetChildren().Num() );
	options->AddChild( control );
}

/*
========================
idMenuScreen_Shell_ControllerLayout::Update
========================
*/
void idMenuScreen_Shell_ControllerLayout::Update()
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
			heading->SetText( "#str_swf_controller_layout" );	// CONTROLLER LAYOUT
			heading->SetStrokeInfo( true, 0.75f, 1.75f );
		}

		idSWFSpriteInstance* gradient = GetSprite()->GetScriptObject()->GetNestedSprite( "info", "gradient" );
		if( gradient != NULL && heading != NULL )
		{
			gradient->SetXPos( heading->GetTextLength() );
		}

		if( menuData != NULL )
		{
			idSWFSpriteInstance* layout = NULL;

			layout = GetSprite()->GetScriptObject()->GetNestedSprite( "info", "controlInfo", "layout360" );

			if( layout != NULL )
			{
				if( menuData->GetPlatform( true ) == 2 )
				{
					layout->StopFrame( 1 );
				}
				else
				{
					layout->StopFrame( menuData->GetPlatform( true ) + 1 );
				}
			}
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
idMenuScreen_Shell_ControllerLayout::ShowScreen
========================
*/
void idMenuScreen_Shell_ControllerLayout::ShowScreen( const mainMenuTransition_t transitionType )
{
	layoutData.LoadData();
	idMenuScreen::ShowScreen( transitionType );

	if( GetSprite() != NULL )
	{

		idSWFSpriteInstance* layout360 = NULL;
		idSWFSpriteInstance* layoutPS3 = NULL;

		layout360 = GetSprite()->GetScriptObject()->GetNestedSprite( "info", "controlInfo", "layout360" );
		layoutPS3 = GetSprite()->GetScriptObject()->GetNestedSprite( "info", "controlInfo", "layoutPS3" );

		if( layout360 != NULL && layoutPS3 != NULL )
		{
			layout360->SetVisible( true );
			layoutPS3->SetVisible( false );
		}
	}

	UpdateBindingInfo();
}

/*
========================
idMenuScreen_Shell_ControllerLayout::HideScreen
========================
*/
void idMenuScreen_Shell_ControllerLayout::HideScreen( const mainMenuTransition_t transitionType )
{
	layoutData.CommitData();
	idMenuScreen::HideScreen( transitionType );
}

/*
========================
idMenuScreen_Shell_ControllerLayout::UpdateBindingInfo
========================
*/
void idMenuScreen_Shell_ControllerLayout::UpdateBindingInfo()
{

	if( !GetSprite() )
	{
		return;
	}

	for( int i = 0; i < numGamepadBinds; ++i )
	{

		const char* txtField = gamepadBinds[i].textField;
		int keyNum = gamepadBinds[i].keyNum;

		idSWFTextInstance* txtVal = NULL;

		txtVal = GetSprite()->GetScriptObject()->GetNestedText( "info", "controlInfo", "layout360", txtField );

		if( txtVal != NULL )
		{
			const char* binding = idKeyInput::GetBinding( keyNum );
			if( binding == NULL || binding[0] == 0 )
			{
				txtVal->SetText( "" );
			}
			else if( keyNum == K_JOY7 )
			{
				idStr action = idLocalization::GetString( va( "#str_swf_action%s", binding ) );
				txtVal->SetText( action.c_str() );
			}
			else if( keyNum == K_JOY8 )
			{
				idStr action = idLocalization::GetString( va( "#str_swf_action%s", binding ) );
				txtVal->SetText( action.c_str() );
			}
			else
			{
				txtVal->SetText( va( "#str_swf_action%s", binding ) );
			}
		}
	}
}

/*
========================
idMenuScreen_Shell_ControllerLayout::HandleAction h
========================
*/
bool idMenuScreen_Shell_ControllerLayout::HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled )
{

	if( menuData == NULL )
	{
		return true;
	}

	if( menuData->ActiveScreen() != SHELL_AREA_CONTROLLER_LAYOUT )
	{
		return false;
	}

	widgetAction_t actionType = action.GetType();
	const idSWFParmList& parms = action.GetParms();

	switch( actionType )
	{
		case WIDGET_ACTION_GO_BACK:
		{
			menuData->SetNextScreen( SHELL_AREA_GAMEPAD, MENU_TRANSITION_SIMPLE );
			return true;
		}
		case WIDGET_ACTION_PRESS_FOCUSED:
		{
			if( parms.Num() != 1 )
			{
				return true;
			}

			if( options == NULL )
			{
				return true;
			}

			int selectionIndex = parms[0].ToInteger();
			if( selectionIndex != options->GetFocusIndex() )
			{
				options->SetViewIndex( options->GetViewOffset() + selectionIndex );
				options->SetFocusIndex( selectionIndex );
			}

			layoutData.AdjustField( selectionIndex, 1 );
			options->Update();
			UpdateBindingInfo();
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
		case WIDGET_ACTION_ADJUST_FIELD:
		{
			if( widget != NULL && widget->GetDataSource() != NULL )
			{
				widget->GetDataSource()->AdjustField( widget->GetDataSourceFieldIndex(), parms[ 0 ].ToInteger() );
				widget->Update();
			}
			UpdateBindingInfo();
			return true;
		}
	}

	return idMenuWidget::HandleAction( action, event, widget, forceHandled );
}

/*
========================
idMenuScreen_Shell_ControllerLayout::idMenuDataSource_AudioSettings::idMenuDataSource_AudioSettings
========================
*/
idMenuScreen_Shell_ControllerLayout::idMenuDataSource_LayoutSettings::idMenuDataSource_LayoutSettings()
{
	fields.SetNum( MAX_LAYOUT_FIELDS );
	originalFields.SetNum( MAX_LAYOUT_FIELDS );
}

/*
========================
idMenuScreen_Shell_ControllerLayout::idMenuDataSource_AudioSettings::LoadData
========================
*/
void idMenuScreen_Shell_ControllerLayout::idMenuDataSource_LayoutSettings::LoadData()
{

	idPlayerProfile* profile = session->GetProfileFromMasterLocalUser();
	if( profile == NULL )
	{
		return;
	}

	int configSet = profile->GetConfig();

	fields[ LAYOUT_FIELD_LAYOUT ].SetString( idLocalization::GetString( va( "#str_swf_config_360_%i", configSet ) ) );

	originalFields = fields;
}

/*
========================
idMenuScreen_Shell_ControllerLayout::idMenuDataSource_AudioSettings::CommitData
========================
*/
void idMenuScreen_Shell_ControllerLayout::idMenuDataSource_LayoutSettings::CommitData()
{

	if( IsDataChanged() )
	{
		cvarSystem->SetModifiedFlags( CVAR_ARCHIVE );
	}

	// make the committed fields into the backup fields
	originalFields = fields;
}

/*
========================
idMenuScreen_Shell_ControllerLayout::idMenuDataSource_AudioSettings::AdjustField
========================
*/
void idMenuScreen_Shell_ControllerLayout::idMenuDataSource_LayoutSettings::AdjustField( const int fieldIndex, const int adjustAmount )
{

	idPlayerProfile* profile = session->GetProfileFromMasterLocalUser();
	if( profile == NULL )
	{
		return;
	}

	int configSet = profile->GetConfig();

	if( fieldIndex == LAYOUT_FIELD_LAYOUT )
	{
		configSet += adjustAmount;
		if( configSet < 0 )
		{
			configSet = MAX_CONTROLLER_CONFIGS - 1;
		}
		else if( configSet >= MAX_CONTROLLER_CONFIGS )
		{
			configSet = 0;
		}
	}

	fields[ LAYOUT_FIELD_LAYOUT ].SetString( idLocalization::GetString( va( "#str_swf_config_360_%i", configSet ) ) );

	profile->SetConfig( configSet, false );

}

/*
========================
idMenuScreen_Shell_ControllerLayout::idMenuDataSource_AudioSettings::IsDataChanged
========================
*/
bool idMenuScreen_Shell_ControllerLayout::idMenuDataSource_LayoutSettings::IsDataChanged() const
{
	bool hasLocalChanges = false;

	if( fields[ LAYOUT_FIELD_LAYOUT ].ToString() != originalFields[ LAYOUT_FIELD_LAYOUT ].ToString() )
	{
		return true;
	}

	return hasLocalChanges;
}