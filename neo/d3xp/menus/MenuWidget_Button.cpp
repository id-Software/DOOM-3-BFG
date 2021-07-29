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

/*
================================================================================================
idMenuWidget_Button

SWF object structure
--------------------
BUTTON (Frames: up, over, out, down, release, disabled, sel_up, sel_over, sel_out, sel_down, sel_release, selecting, unselecting)
	txtOption
		txtValue (Text)
	optionType (Frames: One per mainMenuOption_t enum)
		sliderBar
			bar (Frames: 1-100 for percentage filled)
			btnLess
			btnMore
		sliderText
			txtVal (Text)
			btnLess
			btnMore

Future work:
- Perhaps this should be called "MultiButton", since it merges additional controls with a standard button?
================================================================================================
*/

//---------------------------------
// Animation State Transitions
//
// Maps animations that should be called when transitioning states:
//
//		X-axis = state transitioning FROM
//		Y-axis = state transitioning TO
//
// An empty string indicates remain at current animation.
//---------------------------------
static const char* ANIM_STATE_TRANSITIONS[ idMenuWidget_Button::ANIM_STATE_MAX * idMenuWidget_Button::ANIM_STATE_MAX ] =
{
	// UP			DOWN			OVER
	"",				"release",		"out",		// UP
	"down",			"",				"down",		// DOWN
	"over",			"over",			"",			// OVER
};

// script name for the control object for a given type of button
static const char* const CONTROL_SPRITE_NAMES[ MAX_MENU_OPTION_TYPES ] =
{
	NULL,
	"sliderBar",
	"sliderText",
	"sliderText",
	NULL,
	"sliderText",
};
compile_time_assert( sizeof( CONTROL_SPRITE_NAMES ) / sizeof( CONTROL_SPRITE_NAMES[ 0 ] ) == MAX_MENU_OPTION_TYPES );

/*
========================
idMenuWidget_Button::Update
========================
*/
void idMenuWidget_Button::Update()
{

	if( menuData != NULL && menuData->GetGUI() != NULL )
	{
		BindSprite( menuData->GetGUI()->GetRootObject() );
	}

	if( GetSprite() == NULL )
	{
		return;
	}

	idSWFScriptObject* const spriteObject = GetSprite()->GetScriptObject();

	if( btnLabel.IsEmpty() )
	{
		if( values.Num() > 0 )
		{
			for( int val = 0; val < values.Num(); ++val )
			{
				idSWFScriptObject* const textObject = spriteObject->GetNestedObj( va( "label%d", val ), "txtVal" );
				if( textObject != NULL )
				{
					idSWFTextInstance* const text = textObject->GetText();
					text->SetIgnoreColor( ignoreColor );
					text->tooltip = ignoreColor; // ignoreColor does double duty as "allow tooltips"
					text->SetText( values[ val ].c_str() );
					text->SetStrokeInfo( true, 0.75f, 2.0f );
				}
			}
		}
		else if( img != NULL )
		{
			idSWFSpriteInstance* btnImg = spriteObject->GetNestedSprite( "img" );
			if( btnImg != NULL )
			{
				btnImg->SetMaterial( img );
			}

			btnImg = spriteObject->GetNestedSprite( "imgTop" );
			if( btnImg != NULL )
			{
				btnImg->SetMaterial( img );
			}
		}
		else
		{
			ClearSprite();
		}
	}
	else
	{
		idSWFScriptObject* const textObject = spriteObject->GetNestedObj( "label0", "txtVal" );
		if( textObject != NULL )
		{
			idSWFTextInstance* const text = textObject->GetText();
			text->SetIgnoreColor( ignoreColor );
			text->tooltip = ignoreColor; // ignoreColor does double duty as "allow tooltips"
			text->SetText( btnLabel.c_str() );
			text->SetStrokeInfo( true, 0.75f, 2.0f );
		}
	}

	// events
	spriteObject->Set( "onPress", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_PRESS, 0 ) );
	spriteObject->Set( "onRelease", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_RELEASE, 0 ) );

	idSWFScriptObject* hitBox = spriteObject->GetObject( "hitBox" );
	if( hitBox == NULL )
	{
		hitBox = spriteObject;
	}

	hitBox->Set( "onRollOver", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_ROLL_OVER, 0 ) );
	hitBox->Set( "onRollOut", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_ROLL_OUT, 0 ) );
}

/*
========================
idMenuWidget_Button::ExecuteEvent
========================
*/
bool idMenuWidget_Button::ExecuteEvent( const idWidgetEvent& event )
{
	bool handled = false;

	// do nothing at all if it's disabled
	if( GetState() != WIDGET_STATE_DISABLED )
	{
		switch( event.type )
		{
			case WIDGET_EVENT_PRESS:
			{
				if( GetMenuData() != NULL )
				{
					GetMenuData()->PlaySound( GUI_SOUND_ADVANCE );
				}
				AnimateToState( ANIM_STATE_DOWN );
				handled = true;
				break;
			}
			case WIDGET_EVENT_RELEASE:
			{
				AnimateToState( ANIM_STATE_UP );
				GetMenuData()->ClearWidgetActionRepeater();
				handled = true;
				break;
			}
			case WIDGET_EVENT_ROLL_OVER:
			{
				if( GetMenuData() != NULL )
				{
					GetMenuData()->PlaySound( GUI_SOUND_ROLL_OVER );
				}
				AnimateToState( ANIM_STATE_OVER );
				handled = true;
				break;
			}
			case WIDGET_EVENT_ROLL_OUT:
			{
				AnimateToState( ANIM_STATE_UP );
				GetMenuData()->ClearWidgetActionRepeater();
				handled = true;
				break;
			}
			case WIDGET_EVENT_FOCUS_OFF:
			{
				SetState( WIDGET_STATE_NORMAL );
				handled = true;
				break;
			}
			case WIDGET_EVENT_FOCUS_ON:
			{
				SetState( WIDGET_STATE_SELECTING );
				handled = true;
				break;
			}
			case WIDGET_EVENT_SCROLL_LEFT_RELEASE:
			{
				GetMenuData()->ClearWidgetActionRepeater();
				break;
			}
			case WIDGET_EVENT_SCROLL_RIGHT_RELEASE:
			{
				GetMenuData()->ClearWidgetActionRepeater();
				break;
			}
		}
	}

	idMenuWidget::ExecuteEvent( event );

	return handled;
}

/*
========================
idMenuWidget_Button::AddValue
========================
*/
void idMenuWidget_Button::SetValues( idList< idStr >& list )
{
	values.Clear();
	for( int i = 0; i < list.Num(); ++ i )
	{
		values.Append( list[ i ] );
	}
}

/*
========================
idMenuWidget_Button::GetValue
========================
*/
const idStr& idMenuWidget_Button::GetValue( int index ) const
{

	return values[ index ];

}

/*
========================
idMenuWidget_Button::SetupTransitionInfo
========================
*/
void idMenuWidget_Button::SetupTransitionInfo( widgetTransition_t& trans, const widgetState_t buttonState, const animState_t sourceAnimState, const animState_t destAnimState ) const
{
	trans.prefixes.Clear();
	if( buttonState == WIDGET_STATE_DISABLED )
	{
		trans.animationName = "disabled";
	}
	else
	{
		const int animIndex = ( int )destAnimState * ANIM_STATE_MAX + ( int )sourceAnimState;
		trans.animationName = ANIM_STATE_TRANSITIONS[ animIndex ];
		if( buttonState == WIDGET_STATE_SELECTING )
		{
			trans.prefixes.Append( "sel_" );
		}
	}
	trans.prefixes.Append( "" );
}

/*
========================
idMenuWidget_Button::AnimateToState

Plays an animation from the current state to the target state.
========================
*/
void idMenuWidget_Button::AnimateToState( const animState_t targetAnimState, const bool force )
{
	if( !force && targetAnimState == GetAnimState() )
	{
		return;
	}

	if( GetSprite() != NULL )
	{
		widgetTransition_t trans;
		SetupTransitionInfo( trans, GetState(), GetAnimState(), targetAnimState );
		if( trans.animationName[0] != '\0' )
		{
			for( int i = 0; i < trans.prefixes.Num(); ++i )
			{
				const char* const frameLabel = va( "%s%s", trans.prefixes[ i ], trans.animationName );
				if( GetSprite()->FrameExists( frameLabel ) )
				{
					GetSprite()->PlayFrame( frameLabel );
					Update();
					break;
				}
			}
		}

		idSWFSpriteInstance* const focusSprite = GetSprite()->GetScriptObject()->GetSprite( "focusIndicator" );
		if( focusSprite != NULL )
		{
			if( targetAnimState == ANIM_STATE_OVER )
			{
				focusSprite->PlayFrame( "show" );
			}
			else
			{
				focusSprite->PlayFrame( "hide" );
			}
		}
	}

	SetAnimState( targetAnimState );
}

//*****************************************************************************************************************
// CONTROL BUTTON

/*
========================
idMenuWidget_ControlButton::Update
========================
*/
void idMenuWidget_ControlButton::Update()
{

	if( GetSprite() == NULL )
	{
		return;
	}

	idSWFScriptObject* const spriteObject = GetSprite()->GetScriptObject()->GetNestedObj( "type" );
	if( spriteObject == NULL )
	{
		return;
	}
	idSWFSpriteInstance* type = spriteObject->GetSprite();

	if( type == NULL )
	{
		return;
	}

	if( GetOptionType() != OPTION_BUTTON_FULL_TEXT_SLIDER )
	{
		type->StopFrame( GetOptionType() + 1 );
	}

	idSWFTextInstance* text = spriteObject->GetNestedText( "label0", "txtVal" );
	if( text != NULL )
	{
		text->SetText( btnLabel );
		text->SetStrokeInfo( true, 0.75f, 2.0f );
	}

	if( CONTROL_SPRITE_NAMES[ GetOptionType() ] != NULL )
	{
		idSWFSpriteInstance* controlSprite = NULL;
		if( CONTROL_SPRITE_NAMES[ GetOptionType() ] != NULL )
		{
			controlSprite = type->GetScriptObject()->GetSprite( CONTROL_SPRITE_NAMES[ GetOptionType() ] );
			if( verify( controlSprite != NULL ) )
			{
				if( verify( GetDataSource() != NULL ) )
				{
					idSWFScriptVar fieldValue = GetDataSource()->GetField( GetDataSourceFieldIndex() );
					if( GetOptionType() == OPTION_SLIDER_BAR )
					{
						controlSprite->StopFrame( 1 + fieldValue.ToInteger() );
					}
					else if( GetOptionType() == OPTION_SLIDER_TOGGLE )
					{
						idSWFTextInstance* const txtInfo = controlSprite->GetScriptObject()->GetNestedText( "txtVal" );
						if( verify( txtInfo != NULL ) )
						{
							txtInfo->SetText( fieldValue.ToBool() ? "#str_swf_enabled" : "#str_swf_disabled" );
							txtInfo->SetStrokeInfo( true, 0.75f, 2.0f );
						}
					}
					else if( GetOptionType() == OPTION_SLIDER_TEXT || GetOptionType() == OPTION_BUTTON_FULL_TEXT_SLIDER )
					{
						idSWFTextInstance* const txtInfo = controlSprite->GetScriptObject()->GetNestedText( "txtVal" );
						if( verify( txtInfo != NULL ) )
						{
							txtInfo->SetText( fieldValue.ToString() );
							txtInfo->SetStrokeInfo( true, 0.75f, 2.0f );
						}
					}
				}

				idSWFScriptObject* const btnLess = GetSprite()->GetScriptObject()->GetObject( "btnLess" );
				idSWFScriptObject* const btnMore = GetSprite()->GetScriptObject()->GetObject( "btnMore" );

				if( btnLess != NULL && btnMore != NULL )
				{
					if( disabled )
					{
						btnLess->GetSprite()->SetVisible( false );
						btnMore->GetSprite()->SetVisible( false );
					}
					else
					{
						btnLess->GetSprite()->SetVisible( true );
						btnMore->GetSprite()->SetVisible( true );

						btnLess->Set( "onPress", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_SCROLL_LEFT, 0 ) );
						btnLess->Set( "onRelease", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_SCROLL_LEFT_RELEASE, 0 ) );

						btnMore->Set( "onPress", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_SCROLL_RIGHT, 0 ) );
						btnMore->Set( "onRelease", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_SCROLL_RIGHT_RELEASE, 0 ) );

						btnLess->Set( "onRollOver", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_ROLL_OVER, 0 ) );
						btnLess->Set( "onRollOut", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_ROLL_OUT, 0 ) );

						btnMore->Set( "onRollOver", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_ROLL_OVER, 0 ) );
						btnMore->Set( "onRollOut", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_ROLL_OUT, 0 ) );
					}
				}
			}
		}
	}
	else
	{
		idSWFScriptObject* const btnLess = GetSprite()->GetScriptObject()->GetObject( "btnLess" );
		idSWFScriptObject* const btnMore = GetSprite()->GetScriptObject()->GetObject( "btnMore" );

		if( btnLess != NULL && btnMore != NULL )
		{
			btnLess->GetSprite()->SetVisible( false );
			btnMore->GetSprite()->SetVisible( false );
		}
	}

	// events
	GetSprite()->GetScriptObject()->Set( "onPress", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_PRESS, 0 ) );
	GetSprite()->GetScriptObject()->Set( "onRelease", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_RELEASE, 0 ) );

	idSWFScriptObject* hitBox = GetSprite()->GetScriptObject()->GetObject( "hitBox" );
	if( hitBox == NULL )
	{
		hitBox = GetSprite()->GetScriptObject();
	}

	hitBox->Set( "onRollOver", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_ROLL_OVER, 0 ) );
	hitBox->Set( "onRollOut", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_ROLL_OUT, 0 ) );

}

/*
========================
idMenuWidget_ControlButton::Update
========================
*/
void idMenuWidget_ControlButton::SetupEvents( int delay, int index )
{
	AddEventAction( WIDGET_EVENT_SCROLL_LEFT ).Set( WIDGET_ACTION_START_REPEATER, WIDGET_ACTION_ADJUST_FIELD, -1, delay, index );
	AddEventAction( WIDGET_EVENT_SCROLL_RIGHT ).Set( WIDGET_ACTION_START_REPEATER, WIDGET_ACTION_ADJUST_FIELD, 1, delay, index );
	AddEventAction( WIDGET_EVENT_SCROLL_LEFT_RELEASE ).Set( WIDGET_ACTION_STOP_REPEATER );
	AddEventAction( WIDGET_EVENT_SCROLL_RIGHT_RELEASE ).Set( WIDGET_ACTION_STOP_REPEATER );
	AddEventAction( WIDGET_EVENT_SCROLL_LEFT_LSTICK ).Set( WIDGET_ACTION_START_REPEATER, WIDGET_ACTION_ADJUST_FIELD, -1, delay, index );
	AddEventAction( WIDGET_EVENT_SCROLL_RIGHT_LSTICK ).Set( WIDGET_ACTION_START_REPEATER, WIDGET_ACTION_ADJUST_FIELD, 1, delay, index );
	AddEventAction( WIDGET_EVENT_SCROLL_LEFT_LSTICK_RELEASE ).Set( WIDGET_ACTION_STOP_REPEATER );
	AddEventAction( WIDGET_EVENT_SCROLL_RIGHT_LSTICK_RELEASE ).Set( WIDGET_ACTION_STOP_REPEATER );
}

//****************************************************************
// SERVER BUTTON
//****************************************************************

/*
========================
idMenuWidget_ServerButton::Update
========================
*/
void idMenuWidget_ServerButton::Update()
{

	if( GetSprite() == NULL )
	{
		return;
	}

	idSWFScriptObject* const spriteObject = GetSprite()->GetScriptObject();
	idSWFTextInstance* const txtName = spriteObject->GetNestedText( "label0", "txtVal" );

	if( txtName != NULL )
	{
		txtName->SetText( serverName );
		txtName->SetStrokeInfo( true, 0.75f, 1.75f );
	}

	// events
	spriteObject->Set( "onPress", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_PRESS, 0 ) );
	spriteObject->Set( "onRelease", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_RELEASE, 0 ) );

	idSWFScriptObject* hitBox = spriteObject->GetObject( "hitBox" );
	if( hitBox == NULL )
	{
		hitBox = spriteObject;
	}

	hitBox->Set( "onRollOver", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_ROLL_OVER, 0 ) );
	hitBox->Set( "onRollOut", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_ROLL_OUT, 0 ) );

}

/*
========================
idMenuWidget_ServerButton::SetButtonInfo
========================
*/
void idMenuWidget_ServerButton::SetButtonInfo( idStr name_, idStrId mapName_, idStr modeName_, int index_, int players_, int maxPlayers_, bool joinable_, bool validMap_ )
{
	serverName = name_;
	index = index_;
	players = players_;
	maxPlayers = maxPlayers_;
	joinable = joinable_;
	validMap = validMap_;
	mapName = mapName_;
	modeName = modeName_;

	idStr desc;
	if( index >= 0 )
	{
		idStr playerVal = va( "%s %d/%d", idLocalization::GetString( "#str_02396" ), players, maxPlayers );
		idStr lobbyMapName = va( "%s %s", idLocalization::GetString( "#str_02045" ), mapName.GetLocalizedString() );
		idStr lobbyMode;
		if( !modeName.IsEmpty() )
		{
			lobbyMode = va( "%s %s", idLocalization::GetString( "#str_02044" ), modeName.c_str() );
		}

		desc = va( "%s   %s   %s", playerVal.c_str(), lobbyMapName.c_str(), lobbyMode.c_str() );
	}
	SetDescription( desc );

}

//****************************************************************
// LOBBY BUTTON
//****************************************************************

/*
========================
idMenuWidget_LobbyButton::Update
========================
*/
void idMenuWidget_LobbyButton::Update()
{

	if( GetSprite() == NULL )
	{
		return;
	}

	idSWFScriptObject* const spriteObject = GetSprite()->GetScriptObject();
	idSWFTextInstance* const txtName = spriteObject->GetNestedText( "itemName", "txtVal" );
	idSWFSpriteInstance* talkIcon = spriteObject->GetNestedSprite( "chaticon" );

	if( txtName != NULL )
	{
		txtName->SetText( name );
	}

	if( talkIcon != NULL )
	{
		talkIcon->StopFrame( voiceState + 1 );
		talkIcon->GetScriptObject()->Set( "onPress", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_COMMAND, WIDGET_ACTION_MUTE_PLAYER ) );
	}

	// events
	spriteObject->Set( "onPress", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_PRESS, 0 ) );
	spriteObject->Set( "onRelease", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_RELEASE, 0 ) );

	idSWFScriptObject* hitBox = spriteObject->GetObject( "hitBox" );
	if( hitBox == NULL )
	{
		hitBox = spriteObject;
	}

	hitBox->Set( "onRollOver", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_ROLL_OVER, 0 ) );
	hitBox->Set( "onRollOut", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_ROLL_OUT, 0 ) );
}

/*
========================
idMenuWidget_LobbyButton::SetButtonInfo
========================
*/
void idMenuWidget_LobbyButton::SetButtonInfo( idStr name_, voiceStateDisplay_t voiceState_ )
{
	name = name_;
	voiceState = voiceState_;
}

//****************************************************************
// SCOREBOARD BUTTON
//****************************************************************

/*
========================
idMenuWidget_ScoreboardButton::Update
========================
*/
void idMenuWidget_ScoreboardButton::Update()
{

	if( GetSprite() == NULL )
	{
		return;
	}

	if( index == -1 )
	{
		GetSprite()->SetVisible( false );
		return;
	}

	GetSprite()->SetVisible( true );

	idSWFScriptObject* const spriteObject = GetSprite()->GetScriptObject();
	for( int val = 0; val < values.Num(); ++val )
	{
		idSWFScriptObject* const textObject = spriteObject->GetNestedObj( va( "label%d", val ), "txtVal" );
		if( textObject != NULL )
		{
			idSWFTextInstance* const text = textObject->GetText();
			text->SetIgnoreColor( ignoreColor );
			text->tooltip = ignoreColor; // ignoreColor does double duty as "allow tooltips"
			text->SetText( values[ val ].c_str() );
			text->SetStrokeInfo( true, 0.75f, 2.0f );
		}
	}

	idSWFSpriteInstance* talkIcon = spriteObject->GetNestedSprite( "chaticon" );
	if( talkIcon != NULL )
	{
		talkIcon->StopFrame( voiceState + 1 );
		talkIcon->GetScriptObject()->Set( "onPress", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_COMMAND, WIDGET_ACTION_MUTE_PLAYER ) );
	}

	// events
	spriteObject->Set( "onPress", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_PRESS, 0 ) );
	spriteObject->Set( "onRelease", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_RELEASE, 0 ) );

	idSWFScriptObject* hitBox = spriteObject->GetObject( "hitBox" );
	if( hitBox == NULL )
	{
		hitBox = spriteObject;
	}

	hitBox->Set( "onRollOver", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_ROLL_OVER, 0 ) );
	hitBox->Set( "onRollOut", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_ROLL_OUT, 0 ) );
}

/*
========================
idMenuWidget_ScoreboardButton::SetButtonInfo
========================
*/
void idMenuWidget_ScoreboardButton::SetButtonInfo( int index_, idList< idStr >& list, voiceStateDisplay_t voiceState_ )
{
	index = index_;
	voiceState = voiceState_;
	SetValues( list );
}