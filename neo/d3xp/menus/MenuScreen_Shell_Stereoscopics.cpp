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

const static int NUM_SYSTEM_OPTIONS_OPTIONS = 4;

// TRC requires a maximum interoccular distance of 6.5cm even though human adults can easily have an interoccular distance of over 7.5cm
const static float MAX_INTEROCCULAR_DISTANCE = 6.5f;

// This should line up with stereo3DMode_t
static const char* stereoRender_enable_text[] =
{
	"#str_00641",
	"#str_swf_stereo_side_by_side",
	"#str_swf_stereo_top_and_bottom",
	"#str_swf_stereo_side_by_side_full",
	"#str_swf_stereo_interlaced",
	"#str_swf_stereo_quad"
};
static const int NUM_STEREO_ENABLE = sizeof( stereoRender_enable_text ) / sizeof( stereoRender_enable_text[0] );

/*
========================
idMenuScreen_Shell_Stereoscopics::Initialize
========================
*/
void idMenuScreen_Shell_Stereoscopics::Initialize( idMenuHandler* data )
{
	idMenuScreen::Initialize( data );

	if( data != NULL )
	{
		menuGUI = data->GetGUI();
	}

	SetSpritePath( "menuStereoscopics" );

	options = new( TAG_SWF ) idMenuWidget_DynamicList();
	options->SetNumVisibleOptions( NUM_SYSTEM_OPTIONS_OPTIONS );
	options->SetSpritePath( GetSpritePath(), "info", "options" );
	options->SetWrappingAllowed( true );
	options->SetControlList( true );
	options->Initialize( data );
	AddChild( options );

	btnBack = new( TAG_SWF ) idMenuWidget_Button();
	btnBack->Initialize( data );
	btnBack->SetLabel( "#str_swf_settings" );
	btnBack->SetSpritePath( GetSpritePath(), "info", "btnBack" );
	btnBack->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_GO_BACK );
	AddChild( btnBack );

	idMenuWidget_ControlButton* control;

	control = new( TAG_SWF ) idMenuWidget_ControlButton();
	control->SetOptionType( OPTION_SLIDER_TEXT );
	control->SetLabel( "#str_swf_stereoscopic_rendering" );	// Stereoscopics
	control->SetDataSource( &stereoData, idMenuDataSource_StereoSettings::STEREO_FIELD_ENABLE );
	control->SetupEvents( DEFAULT_REPEAT_TIME, options->GetChildren().Num() );
	control->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_PRESS_FOCUSED, options->GetChildren().Num() );
	options->AddChild( control );

	control = new( TAG_SWF ) idMenuWidget_ControlButton();
	control->SetOptionType( OPTION_SLIDER_BAR );
	control->SetLabel( "#str_swf_stereo_seperation" );	// View Offset
	control->SetDataSource( &stereoData, idMenuDataSource_StereoSettings::STEREO_FIELD_SEPERATION );
	control->SetupEvents( 2, options->GetChildren().Num() );
	control->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_PRESS_FOCUSED, options->GetChildren().Num() );
	options->AddChild( control );

	control = new( TAG_SWF ) idMenuWidget_ControlButton();
	control->SetOptionType( OPTION_SLIDER_TOGGLE );
	control->SetLabel( "#str_swf_stereo_eye_swap" );	// Swap Eyes
	control->SetDataSource( &stereoData, idMenuDataSource_StereoSettings::STEREO_FIELD_SWAP_EYES );
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

	leftEyeMat = declManager->FindMaterial( "doomLeftEye" );
	rightEyeMat = declManager->FindMaterial( "doomRightEye" );
}



/*
========================
idMenuScreen_Shell_Stereoscopics::Update
========================
*/
void idMenuScreen_Shell_Stereoscopics::Update()
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
			heading->SetText( "#str_swf_stereoscopics_heading" );	// STEREOSCOPIC RENDERING
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
idMenuScreen_Shell_Stereoscopics::ShowScreen
========================
*/
void idMenuScreen_Shell_Stereoscopics::ShowScreen( const mainMenuTransition_t transitionType )
{
	stereoData.LoadData();
	idMenuScreen::ShowScreen( transitionType );

	if( GetSprite() != NULL )
	{
		idSWFSpriteInstance* leftEye = GetSprite()->GetScriptObject()->GetNestedSprite( "info", "leftEye" );
		idSWFSpriteInstance* rightEye = GetSprite()->GetScriptObject()->GetNestedSprite( "info", "rightEye" );

		if( leftEye != NULL && leftEyeMat != NULL )
		{
			leftEye->SetMaterial( leftEyeMat );
		}

		if( rightEye != NULL && rightEyeMat != NULL )
		{
			rightEye->SetMaterial( rightEyeMat );
		}
	}
}

/*
========================
idMenuScreen_Shell_Stereoscopics::HideScreen
========================
*/
void idMenuScreen_Shell_Stereoscopics::HideScreen( const mainMenuTransition_t transitionType )
{
	if( stereoData.IsRestartRequired() )
	{
		class idSWFScriptFunction_Restart : public idSWFScriptFunction_RefCounted
		{
		public:
			idSWFScriptFunction_Restart( gameDialogMessages_t _msg, bool _restart )
			{
				msg = _msg;
				restart = _restart;
			}
			idSWFScriptVar Call( idSWFScriptObject* thisObject, const idSWFParmList& parms )
			{
				common->Dialog().ClearDialog( msg );
				if( restart )
				{
					// DG: Sys_ReLaunch() doesn't need any options anymore
					//     (the old way would have been unnecessarily painful on POSIX systems)
					Sys_ReLaunch();
					// DG end
				}
				return idSWFScriptVar();
			}
		private:
			gameDialogMessages_t msg;
			bool restart;
		};
		idStaticList<idSWFScriptFunction*, 4> callbacks;
		idStaticList<idStrId, 4> optionText;
		callbacks.Append( new idSWFScriptFunction_Restart( GDM_GAME_RESTART_REQUIRED, false ) );
		callbacks.Append( new idSWFScriptFunction_Restart( GDM_GAME_RESTART_REQUIRED, true ) );
		optionText.Append( idStrId( "#str_00100113" ) ); // Continue
		optionText.Append( idStrId( "#str_02487" ) ); // Restart Now
		common->Dialog().AddDynamicDialog( GDM_GAME_RESTART_REQUIRED, callbacks, optionText, true, idStr() );
	}
	if( stereoData.IsDataChanged() )
	{
		stereoData.CommitData();
	}
	idMenuScreen::HideScreen( transitionType );
}

/*
========================
idMenuScreen_Shell_Stereoscopics::HandleAction h
========================
*/
bool idMenuScreen_Shell_Stereoscopics::HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled )
{

	if( menuData != NULL )
	{
		if( menuData->ActiveScreen() != SHELL_AREA_STEREOSCOPICS )
		{
			return false;
		}
	}

	widgetAction_t actionType = action.GetType();
	const idSWFParmList& parms = action.GetParms();

	switch( actionType )
	{
		case WIDGET_ACTION_GO_BACK:
		{
			if( menuData != NULL )
			{
				menuData->SetNextScreen( SHELL_AREA_SETTINGS, MENU_TRANSITION_SIMPLE );
			}
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
			stereoData.AdjustField( selectionIndex, 1 );
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

extern idCVar stereoRender_interOccularCentimeters;
extern idCVar stereoRender_swapEyes;

/*
========================
idMenuScreen_Shell_Stereoscopics::idMenuDataSource_StereoSettings::idMenuDataSource_StereoSettings
========================
*/
idMenuScreen_Shell_Stereoscopics::idMenuDataSource_StereoSettings::idMenuDataSource_StereoSettings()
{
	fields.SetNum( MAX_STEREO_FIELDS );
	originalFields.SetNum( MAX_STEREO_FIELDS );
}

/*
========================
idMenuScreen_Shell_Stereoscopics::idMenuDataSource_StereoSettings::LoadData
========================
*/
void idMenuScreen_Shell_Stereoscopics::idMenuDataSource_StereoSettings::LoadData()
{

	fields[ STEREO_FIELD_ENABLE ].SetInteger( renderSystem->GetStereoScopicRenderingMode() );

	fields[ STEREO_FIELD_SEPERATION ].SetFloat( 100.0f * ( stereoRender_interOccularCentimeters.GetFloat() / MAX_INTEROCCULAR_DISTANCE ) );

	fields[ STEREO_FIELD_SWAP_EYES ].SetBool( stereoRender_swapEyes.GetBool() );
	originalFields = fields;
}

/*
========================
idMenuScreen_Shell_Stereoscopics::idMenuDataSource_StereoSettings::CommitData
========================
*/
void idMenuScreen_Shell_Stereoscopics::idMenuDataSource_StereoSettings::CommitData()
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
idMenuScreen_Shell_Stereoscopics::idMenuDataSource_StereoSettings::AdjustField
========================
*/
void idMenuScreen_Shell_Stereoscopics::idMenuDataSource_StereoSettings::AdjustField( const int fieldIndex, const int adjustAmount )
{

	if( fieldIndex == STEREO_FIELD_ENABLE )
	{
		int numOptions = NUM_STEREO_ENABLE;
		if( !renderSystem->HasQuadBufferSupport() )
		{
			numOptions--;
		}

		int adjusted = fields[ fieldIndex ].ToInteger() + adjustAmount;
		adjusted += numOptions;
		adjusted %= numOptions;
		fields[fieldIndex].SetInteger( adjusted );
		renderSystem->EnableStereoScopicRendering( ( stereo3DMode_t )adjusted );

		gameLocal.Shell_ClearRepeater();

	}
	else if( fieldIndex == STEREO_FIELD_SWAP_EYES )
	{

		fields[ fieldIndex ].SetBool( !fields[ fieldIndex ].ToBool() );
		stereoRender_swapEyes.SetBool( fields[ fieldIndex ].ToBool() );

	}
	else if( fieldIndex == STEREO_FIELD_SEPERATION )
	{

		float newValue = idMath::ClampFloat( 0.0f, 100.0f, fields[ fieldIndex ].ToFloat() + adjustAmount );
		fields[ fieldIndex ].SetFloat( newValue );

		stereoRender_interOccularCentimeters.SetFloat( ( fields[ STEREO_FIELD_SEPERATION ].ToFloat() / 100.0f ) * MAX_INTEROCCULAR_DISTANCE );

	}

	// do this so we don't save every time we modify a setting.  Only save once when we leave the screen
	cvarSystem->ClearModifiedFlags( CVAR_ARCHIVE );

}

/*
========================
idMenuScreen_Shell_Stereoscopics::idMenuDataSource_StereoSettings::IsDataChanged
========================
*/
idSWFScriptVar idMenuScreen_Shell_Stereoscopics::idMenuDataSource_StereoSettings::GetField( const int fieldIndex ) const
{
	if( fieldIndex == STEREO_FIELD_ENABLE )
	{
		return idSWFScriptVar( stereoRender_enable_text[fields[fieldIndex].ToInteger()] );
	}
	return fields[ fieldIndex ];
}

/*
========================
idMenuScreen_Shell_Stereoscopics::idMenuDataSource_StereoSettings::IsDataChanged
========================
*/
bool idMenuScreen_Shell_Stereoscopics::idMenuDataSource_StereoSettings::IsDataChanged() const
{
	if( fields[ STEREO_FIELD_SWAP_EYES ].ToBool() != originalFields[ STEREO_FIELD_SWAP_EYES ].ToBool() )
	{
		return true;
	}

	if( fields[ STEREO_FIELD_ENABLE ].ToInteger() != originalFields[ STEREO_FIELD_ENABLE ].ToInteger() )
	{
		return true;
	}

	if( fields[ STEREO_FIELD_SEPERATION ].ToFloat() != originalFields[ STEREO_FIELD_SEPERATION ].ToFloat() )
	{
		return true;
	}

	return false;
}

/*
========================
idMenuScreen_Shell_Stereoscopics::idMenuDataSource_StereoSettings::IsRestartRequired
========================
*/
bool idMenuScreen_Shell_Stereoscopics::idMenuDataSource_StereoSettings::IsRestartRequired() const
{
	if( fields[ STEREO_FIELD_ENABLE ].ToInteger() != originalFields[ STEREO_FIELD_ENABLE ].ToInteger() )
	{
		if( fields[ STEREO_FIELD_ENABLE ].ToInteger() == STEREO3D_QUAD_BUFFER || originalFields[ STEREO_FIELD_ENABLE ].ToInteger() == STEREO3D_QUAD_BUFFER )
		{
			return true;
		}
	}
	return false;
}
