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

typedef struct
{
	const char* display;
	const char* bind;
} bindInfo_t;

static bindInfo_t keyboardBinds[] =
{
	{ "#str_02090",			""								},	// HEADING
	{ "#str_02100",	"_forward"								},	// FORWARD
	{ "#str_02101",	"_back"									},	// BACKPEDAL
	{ "#str_02102",	"_moveLeft"								},	// MOVE LEFT
	{ "#str_02103",	"_moveRight"							},	// MOVE RIGHT
	{ "#str_02104",	"_moveUp"								},	// JUMP
	{ "#str_02105",	"_moveDown"								},	// CROUCH
	{ "#str_02106",	"_left"									},	// TURN LEFT
	{ "#str_02107",	"_right"								},	// TURN RIGHT
	{ "#str_02109",	"_speed"								},	// SPRINT

	{ "#str_02095",			""								},	// HEADING
	{ "#str_02112",	"_attack"								},	// ATTACK
	{ "#str_02114",	"_impulse14"							},	// PREV. WEAPON
	{ "#str_02113",	"_impulse15"							},	// NEXT WEAPON
	{ "#str_02115",	"_impulse13"							},	// RELOAD
	{ "#str_swf_action_use",	"_use"						},	// USE
	{ "#str_02116",	"_lookUp"								},	// LOOK UP
	{ "#str_02117",	"_lookDown"								},	// LOOK DOWN
	{ "#str_02121",	"_impulse19"							},	// PDA / SCOREBOARD

	{ "#str_02093",			""								},	// HEADING
	{ "#str_00100177",	"_impulse0"							},	// FISTS / GRABBER
	{ "#str_00100178",	"_impulse2"							},	// PISTOL
	{ "#str_00100179",	"_impulse3"							},	// SHOTGUN / DOUBLE
	{ "#str_00100180",	"_impulse5"							},	// MACHINEGUN
	{ "#str_00100181",	"_impulse6"							},	// CHAINGUN
	{ "#str_00100182",	"_impulse7"							},	// GRENADES
	{ "#str_00100183",	"_impulse8"							},	// PLASMA GUN
	{ "#str_00100184",	"_impulse9"							},	// ROCKETS
	{ "#str_00100185",	"_impulse10"						},	// BFG
	{ "#str_swf_soulcube_artifact",	"_impulse12"			},	// SOULCUBE / ARTIFACT
	{ "#str_00100187",	"_impulse16"						},	// FLASHLIGHT

	{ "#str_04065",			""								},	// HEADING
	{ "#str_04067",	"savegame quick"						},	// QUICK SAVE
	{ "#str_04068",	"loadgame quick"						},	// QUICK LOAD
	{ "#str_04069",	"screenshot"							},	// SCREENSHOT
	{ "#str_02068",	"clientMessageMode"						},	// SCREENSHOT
	{ "#str_02122",	"clientMessageMode 1"					},	// SCREENSHOT
	//{ "#str_04071",	"clientDropWeapon"						}	// DROP WEAPON
};

static const int numBinds = sizeof( keyboardBinds ) / sizeof( keyboardBinds[0] );

static const int NUM_BIND_LISTINGS = 14;
/*
========================
idMenuScreen_Shell_Bindings::Initialize
========================
*/
void idMenuScreen_Shell_Bindings::Initialize( idMenuHandler* data )
{
	idMenuScreen::Initialize( data );

	if( data != NULL )
	{
		menuGUI = data->GetGUI();
	}

	SetSpritePath( "menuBindings" );

	restoreDefault = new idMenuWidget_Button();
	restoreDefault->Initialize( data );
	restoreDefault->SetLabel( "" );
	restoreDefault->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_JOY3_ON_PRESS );
	restoreDefault->SetSpritePath( GetSpritePath(), "info", "btnRestore" );

	AddChild( restoreDefault );

	btnBack = new( TAG_SWF ) idMenuWidget_Button();
	btnBack->Initialize( data );
	idStr controls( idLocalization::GetString( "#str_04158" ) );
	controls.ToUpper();
	btnBack->SetLabel( controls );
	btnBack->SetSpritePath( GetSpritePath(), "info", "btnBack" );
	btnBack->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_GO_BACK );

	AddChild( btnBack );

	options = new idMenuWidget_DynamicList();
	options->SetIgnoreColor( true );
	options->SetNumVisibleOptions( NUM_BIND_LISTINGS );
	options->SetSpritePath( GetSpritePath(), "info", "options" );
	options->SetWrappingAllowed( true );

	UpdateBindingDisplay();

	while( options->GetChildren().Num() < NUM_BIND_LISTINGS )
	{
		idMenuWidget_Button* const buttonWidget = new( TAG_SWF ) idMenuWidget_Button();
		buttonWidget->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_PRESS_FOCUSED, options->GetChildren().Num() );
		buttonWidget->Initialize( data );
		options->AddChild( buttonWidget );
	}
	options->Initialize( data );

	AddChild( options );

	AddEventAction( WIDGET_EVENT_SCROLL_DOWN_LSTICK ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_SCROLL_DOWN_START_REPEATER_VARIABLE, WIDGET_EVENT_SCROLL_DOWN_LSTICK ) );
	AddEventAction( WIDGET_EVENT_SCROLL_UP_LSTICK ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_SCROLL_UP_START_REPEATER_VARIABLE, WIDGET_EVENT_SCROLL_UP_LSTICK ) );
	AddEventAction( WIDGET_EVENT_SCROLL_DOWN_LSTICK_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_DOWN_LSTICK_RELEASE ) );
	AddEventAction( WIDGET_EVENT_SCROLL_UP_LSTICK_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_UP_LSTICK_RELEASE ) );
	AddEventAction( WIDGET_EVENT_SCROLL_DOWN ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_SCROLL_DOWN_START_REPEATER_VARIABLE, WIDGET_EVENT_SCROLL_DOWN ) );
	AddEventAction( WIDGET_EVENT_SCROLL_UP ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_SCROLL_UP_START_REPEATER_VARIABLE, WIDGET_EVENT_SCROLL_UP ) );
	AddEventAction( WIDGET_EVENT_SCROLL_DOWN_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_DOWN_RELEASE ) );
	AddEventAction( WIDGET_EVENT_SCROLL_UP_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_UP_RELEASE ) );

}

/*
========================
idMenuScreen_Shell_Bindings::Update
========================
*/
void idMenuScreen_Shell_Bindings::Update()
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
			heading->SetText( "#str_swf_controls_keyboard" );
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
idMenuScreen_Shell_Bindings::ShowScreen
========================
*/
void idMenuScreen_Shell_Bindings::ShowScreen( const mainMenuTransition_t transitionType )
{
	if( options != NULL )
	{
		options->SetViewOffset( 0 );
		options->SetViewIndex( 1 );
		options->SetFocusIndex( 1 );
	}

	if( menuData != NULL )
	{
		menuGUI = menuData->GetGUI();
		if( menuGUI != NULL )
		{
			idSWFScriptObject& root = menuGUI->GetRootObject();
			txtBlinder = root.GetNestedSprite( "menuBindings", "info", "rebind" );
			blinder = root.GetNestedSprite( "menuBindings", "info", "blinder" );
			if( restoreDefault != NULL )
			{
				restoreDefault->BindSprite( root );
			}
		}
	}

	ToggleWait( false );
	UpdateBindingDisplay();

	idMenuScreen::ShowScreen( transitionType );
}

/*
========================
idMenuScreen_Shell_Bindings::HideScreen
========================
*/
void idMenuScreen_Shell_Bindings::HideScreen( const mainMenuTransition_t transitionType )
{

	if( bindingsChanged )
	{
		cvarSystem->SetModifiedFlags( CVAR_ARCHIVE );
		bindingsChanged = false;
	}

	idMenuScreen::HideScreen( transitionType );
}

extern idCVar in_useJoystick;

/*
========================
idMenuScreen_Shell_Bindings::UpdateBindingDisplay
========================
*/
void idMenuScreen_Shell_Bindings::UpdateBindingDisplay()
{

	idList< idList< idStr, TAG_IDLIB_LIST_MENU >, TAG_IDLIB_LIST_MENU > bindList;

	for( int i = 0; i < numBinds; ++i )
	{
		idList< idStr > option;

		option.Append( keyboardBinds[i].display );

		if( ( idStr::Icmp( keyboardBinds[i].bind, "" ) != 0 ) )
		{
			keyBindings_t bind = idKeyInput::KeyBindingsFromBinding( keyboardBinds[i].bind, false, true );

			idStr bindings;

			if( !bind.gamepad.IsEmpty() && in_useJoystick.GetBool() )
			{
				idStrList joyBinds;
				int start = 0;
				while( start < bind.gamepad.Length() )
				{
					int end = bind.gamepad.Find( ", ", true, start );
					if( end < 0 )
					{
						end = bind.gamepad.Length();
					}
					joyBinds.Alloc().CopyRange( bind.gamepad, start, end );
					start = end + 2;
				}
				const char* buttonsWithImages[] =
				{
					"JOY1", "JOY2", "JOY3", "JOY4", "JOY5", "JOY6",
					"JOY_TRIGGER1", "JOY_TRIGGER2", 0
				};
				for( int i = 0; i < joyBinds.Num(); i++ )
				{
					if( joyBinds[i].Icmpn( "JOY_STICK", 9 ) == 0 )
					{
						continue; // Can't rebind the sticks, so don't even show them
					}
					bool hasImage = false;
					for( const char** b = buttonsWithImages; *b != 0; b++ )
					{
						if( joyBinds[i].Icmp( *b ) == 0 )
						{
							hasImage = true;
							break;
						}
					}
					if( !bindings.IsEmpty() )
					{
						bindings.Append( ", " );
					}
					if( hasImage )
					{
						bindings.Append( '<' );
						bindings.Append( joyBinds[i] );
						bindings.Append( '>' );
					}
					else
					{
						bindings.Append( joyBinds[i] );
					}
				}
				bindings.Replace( "JOY_DPAD", "DPAD" );
			}

			if( !bind.keyboard.IsEmpty() )
			{
				if( !bindings.IsEmpty() )
				{
					bindings.Append( ", " );
				}
				bindings.Append( bind.keyboard );
			}

			if( !bind.mouse.IsEmpty() )
			{
				if( !bindings.IsEmpty() )
				{
					bindings.Append( ", " );
				}
				bindings.Append( bind.mouse );
			}

			bindings.ToUpper();
			option.Append( bindings );

		}
		else
		{
			option.Append( "" );
		}

		bindList.Append( option );
	}

	options->SetListData( bindList );

}

/*
========================
idMenuScreen_Shell_Bindings::ToggleWait
========================
*/
void idMenuScreen_Shell_Bindings::ToggleWait( bool wait )
{

	if( wait )
	{

		if( blinder != NULL )
		{
			blinder->SetVisible( true );
			if( options != NULL )
			{
				blinder->StopFrame( options->GetFocusIndex() + 1 );
			}
		}

		if( txtBlinder != NULL )
		{
			txtBlinder->SetVisible( true );
		}

		if( restoreDefault != NULL )
		{
			restoreDefault->SetLabel( "" );
		}

	}
	else
	{

		if( blinder != NULL )
		{
			blinder->SetVisible( false );
		}

		if( txtBlinder != NULL )
		{
			txtBlinder->SetVisible( false );
		}

		if( restoreDefault != NULL )
		{
			if( menuData != NULL )
			{
				menuGUI = menuData->GetGUI();
				if( menuGUI != NULL )
				{
					idSWFScriptObject& root = menuGUI->GetRootObject();
					restoreDefault->SetSpritePath( GetSpritePath(), "info", "btnRestore" );
					restoreDefault->BindSprite( root );
				}
			}
			if( restoreDefault->GetSprite() )
			{
				restoreDefault->GetSprite()->SetVisible( true );
			}
			restoreDefault->SetLabel( "#str_swf_restore_defaults" );
		}

	}
}

/*
========================
idMenuScreen_Shell_Bindings::SetBinding
========================
*/
void idMenuScreen_Shell_Bindings::SetBinding( int keyNum )
{

	int listIndex = options->GetViewIndex();
	idKeyInput::SetBinding( keyNum, keyboardBinds[ listIndex ].bind );
	UpdateBindingDisplay();
	ToggleWait( false );
	Update();

}

/*
========================
idMenuScreen_Shell_Bindings::HandleRestoreDefaults
========================
*/
void idMenuScreen_Shell_Bindings::HandleRestoreDefaults()
{


	class idSWFScriptFunction_Restore : public idSWFScriptFunction_RefCounted
	{
	public:
		idSWFScriptFunction_Restore( gameDialogMessages_t _msg, bool _accept, idMenuScreen_Shell_Bindings* _menu )
		{
			msg = _msg;
			accept = _accept;
			menu = _menu;
		}
		idSWFScriptVar Call( idSWFScriptObject* thisObject, const idSWFParmList& parms )
		{
			common->Dialog().ClearDialog( msg );
			if( accept )
			{
				idLocalUser* user = session->GetSignInManager().GetMasterLocalUser();
				if( user != NULL )
				{
					idPlayerProfile* profile = user->GetProfile();
					if( profile != NULL )
					{
						profile->RestoreDefault();
						if( menu != NULL )
						{
							menu->UpdateBindingDisplay();
							menu->Update();
						}
					}
				}
			}
			return idSWFScriptVar();
		}
	private:
		gameDialogMessages_t msg;
		bool accept;
		idMenuScreen_Shell_Bindings* menu;
	};

	common->Dialog().AddDialog( GDM_BINDINGS_RESTORE, DIALOG_ACCEPT_CANCEL, new idSWFScriptFunction_Restore( GDM_BINDINGS_RESTORE, true, this ), new idSWFScriptFunction_Restore( GDM_BINDINGS_RESTORE, false, this ), false );

}

/*
========================
idMenuScreen_Shell_Bindings::HandleAction
========================
*/
bool idMenuScreen_Shell_Bindings::HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled )
{

	if( menuData == NULL )
	{
		return true;
	}

	if( menuData->ActiveScreen() != SHELL_AREA_KEYBOARD )
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
		case WIDGET_ACTION_JOY3_ON_PRESS:
		{
			HandleRestoreDefaults();
			return true;
		}
		case WIDGET_ACTION_PRESS_FOCUSED:
		{

			int listIndex = 0;
			if( parms.Num() > 0 )
			{
				listIndex = options->GetViewOffset() + parms[ 0 ].ToInteger();
			}
			else
			{
				listIndex = options->GetViewIndex();
			}

			if( listIndex < 0 || listIndex >= numBinds )
			{
				return true;
			}

			if( options->GetViewIndex() != listIndex )
			{

				if( idStr::Icmp( keyboardBinds[ listIndex ].bind, "" ) == 0 )
				{
					return true;
				}

				options->SetViewIndex( listIndex );
				options->SetFocusIndex( listIndex - options->GetViewOffset() );
			}
			else
			{

				idMenuHandler_Shell* data = dynamic_cast< idMenuHandler_Shell* >( menuData );
				if( data != NULL )
				{
					ToggleWait( true );
					Update();
					data->SetWaitForBinding( keyboardBinds[ listIndex ].bind );
				}

			}

			return true;
		}
		case WIDGET_ACTION_SCROLL_VERTICAL_VARIABLE:
		{

			if( parms.Num() == 0 )
			{
				return true;
			}

			if( options != NULL )
			{

				int dir = parms[ 0 ].ToInteger();
				int scroll = 0;
				int curIndex = options->GetViewIndex();

				if( dir != 0 )
				{
					if( curIndex + dir >= numBinds )
					{
						scroll = dir * 2;
					}
					else if( curIndex + dir < 1 )
					{
						scroll = dir * 2;
					}
					else
					{
						if( idStr::Icmp( keyboardBinds[curIndex + dir].bind, "" ) == 0 )
						{
							scroll = dir * 2;
						}
						else
						{
							scroll = dir;
						}
					}
				}

				options->Scroll( scroll, true );
			}

			return true;
		}
	}

	return idMenuWidget::HandleAction( action, event, widget, forceHandled );
}
