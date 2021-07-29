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
========================
idMenuWidget_NavButton::Update
========================
*/
void idMenuWidget_NavButton::Update()
{

	if( GetSprite() == NULL )
	{
		return;
	}

	if( btnLabel.IsEmpty() )
	{
		GetSprite()->SetVisible( false );
		return;
	}

	GetSprite()->SetVisible( true );

	idSWFScriptObject* const spriteObject = GetSprite()->GetScriptObject();
	idSWFTextInstance* const text = spriteObject->GetNestedText( "txtVal" );
	if( text != NULL )
	{
		text->SetText( btnLabel.c_str() );
		text->SetStrokeInfo( true, 0.7f, 1.25f );
	}

	GetSprite()->SetXPos( xPos );

	if( navState == NAV_WIDGET_SELECTED )
	{
		idSWFSpriteInstance* backing = GetSprite()->GetScriptObject()->GetNestedSprite( "backing" );
		if( backing != NULL && text != NULL )
		{
			backing->SetXPos( text->GetTextLength() + 53.0f );
		}
	}

	//
	// events
	//

	idSWFScriptObject* textObj = spriteObject->GetNestedObj( "txtVal" );

	if( textObj != NULL )
	{

		textObj->Set( "onPress", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_PRESS, 0 ) );
		textObj->Set( "onRelease", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_RELEASE, 0 ) );

		idSWFScriptObject* hitBox = spriteObject->GetObject( "hitBox" );
		if( hitBox == NULL )
		{
			hitBox = textObj;
		}

		hitBox->Set( "onRollOver", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_ROLL_OVER, 0 ) );
		hitBox->Set( "onRollOut", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_ROLL_OUT, 0 ) );
	}
}

/*
========================
idMenuWidget_NavButton::ExecuteEvent
========================
*/
bool idMenuWidget_NavButton::ExecuteEvent( const idWidgetEvent& event )
{

	bool handled = false;

	//// do nothing at all if it's disabled
	if( GetState() != WIDGET_STATE_DISABLED )
	{
		switch( event.type )
		{
			case WIDGET_EVENT_PRESS:
			{
				//AnimateToState( ANIM_STATE_DOWN );
				handled = true;
				break;
			}
			case WIDGET_EVENT_ROLL_OVER:
			{
				if( GetMenuData() )
				{
					GetMenuData()->PlaySound( GUI_SOUND_ROLL_OVER );
				}
				handled = true;
				break;
			}
		}
	}

	idMenuWidget::ExecuteEvent( event );

	return handled;
}

//*********************************************************************************************************
// idMenuWidget_MenuButton


/*
========================
idMenuWidget_NavButton::Update
========================
*/
void idMenuWidget_MenuButton::Update()
{

	if( GetSprite() == NULL )
	{
		return;
	}

	if( btnLabel.IsEmpty() )
	{
		GetSprite()->SetVisible( false );
		return;
	}

	GetSprite()->SetVisible( true );

	idSWFScriptObject* const spriteObject = GetSprite()->GetScriptObject();
	idSWFTextInstance* const text = spriteObject->GetNestedText( "txtVal" );
	if( text != NULL )
	{
		text->SetText( btnLabel.c_str() );
		text->SetStrokeInfo( true, 0.7f, 1.25f );

		idSWFSpriteInstance* selBar = spriteObject->GetNestedSprite( "sel", "bar" );
		idSWFSpriteInstance* hoverBar = spriteObject->GetNestedSprite( "hover", "bar" );

		if( selBar != NULL )
		{
			selBar->SetXPos( text->GetTextLength() / 2.0f );
			selBar->SetScale( 100.0f * ( text->GetTextLength() / 300.0f ), 100.0f );
		}

		if( hoverBar != NULL )
		{
			hoverBar->SetXPos( text->GetTextLength() / 2.0f );
			hoverBar->SetScale( 100.0f * ( text->GetTextLength() / 352.0f ), 100.0f );
		}

		idSWFSpriteInstance* hitBox = spriteObject->GetNestedSprite( "hitBox" );
		if( hitBox != NULL )
		{
			hitBox->SetScale( 100.0f * ( text->GetTextLength() / 235 ), 100.0f );
		}
	}

	GetSprite()->SetXPos( xPos );

	idSWFScriptObject* textObj = spriteObject->GetNestedObj( "txtVal" );

	if( textObj != NULL )
	{

		textObj->Set( "onPress", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_PRESS, 0 ) );
		textObj->Set( "onRelease", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_RELEASE, 0 ) );

		idSWFScriptObject* hitBox = spriteObject->GetObject( "hitBox" );
		if( hitBox == NULL )
		{
			hitBox = textObj;
		}

		hitBox->Set( "onRollOver", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_ROLL_OVER, 0 ) );
		hitBox->Set( "onRollOut", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_ROLL_OUT, 0 ) );
	}
}
