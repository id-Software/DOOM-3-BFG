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
idMenuWidget_CommandBar

Provides a paged view of this widgets children.  Each child is expected to take on the following
naming scheme.  Children outside of the given window size (NumVisibleOptions) are not rendered,
and will affect which type of arrow indicators are shown.

This transparently supports the "UseCircleForAccept" behavior that we need for Japanese PS3 SKU.

SWF object structure
--------------------
COMMANDBAR
	joy#
		img (Frames: platform)
		txt_info (Text)
================================================================================================
*/

static const char* const BUTTON_NAMES[] =
{
	"joy1",
	"joy2",
	"joy3",
	"joy4",
	"joy10",
	"tab"
};
compile_time_assert( sizeof( BUTTON_NAMES ) / sizeof( BUTTON_NAMES[ 0 ] ) == idMenuWidget_CommandBar::MAX_BUTTONS );

/*
========================
idMenuWidget_CommandBar::ClearAllButtons
========================
*/
void idMenuWidget_CommandBar::ClearAllButtons()
{
	for( int index = 0; index < MAX_BUTTONS; ++index )
	{
		buttons[index].label.Clear();
		buttons[index].action.Set( WIDGET_ACTION_NONE );
	}
}

/*
========================
idMenuWidget_CommandBar::Update
========================
*/
void idMenuWidget_CommandBar::Update()
{

	if( GetSWFObject() == NULL )
	{
		return;
	}

	idSWFScriptObject& root = GetSWFObject()->GetRootObject();

	if( !BindSprite( root ) )
	{
		return;
	}

	const int BASE_PADDING			= 35;
	const int PER_BUTTON_PADDING	= 65;
	const int ALIGNMENT_SCALE		= ( GetAlignment() == LEFT ) ? 1 : -1;

	int xPos = ALIGNMENT_SCALE * BASE_PADDING;

	// Setup the button order.
	idStaticList< button_t, MAX_BUTTONS > buttonOrder;
	for( int i = 0; i < buttonOrder.Max(); ++i )
	{
		buttonOrder.Append( static_cast< button_t >( i ) );
	}

	// NOTE: Special consideration is done for JPN PS3 where the standard accept button is
	// swapped with the standard back button.  i.e. In US: X = Accept, O = Back, but in JPN
	// X = Back, O = Accept.
	if( GetSWFObject()->UseCircleForAccept() )
	{
		buttonOrder[ BUTTON_JOY2 ] = BUTTON_JOY1;
		buttonOrder[ BUTTON_JOY1 ] = BUTTON_JOY2;
	}

	// FIXME: handle animating in of the button bar?
	GetSprite()->SetVisible( true );

	idStr shortcutName;
	for( int i = 0; i < buttonOrder.Num(); ++i )
	{
		const char* const buttonName = BUTTON_NAMES[ buttonOrder[ i ] ];

		idSWFSpriteInstance* const buttonSprite = GetSprite()->GetScriptObject()->GetSprite( buttonName );
		if( buttonSprite == NULL )
		{
			continue;
		}
		idSWFTextInstance* const buttonText = buttonSprite->GetScriptObject()->GetText( "txt_info" );
		if( buttonText == NULL )
		{
			continue;
		}
		idSWFSpriteInstance* const imageSprite = buttonSprite->GetScriptObject()->GetSprite( "img" );
		if( imageSprite == NULL )
		{
			continue;
		}

		if( buttons[ i ].action.GetType() != WIDGET_ACTION_NONE )
		{
			idSWFScriptObject* const shortcutKeys = GetSWFObject()->GetGlobal( "shortcutKeys" ).GetObject();
			if( verify( shortcutKeys != NULL ) )
			{
				buttonSprite->GetScriptObject()->Set( "onPress", new WrapWidgetSWFEvent( this, WIDGET_EVENT_COMMAND, i ) );

				// bind the main action - need to use all caps here because shortcuts are stored that way
				shortcutName = buttonName;
				shortcutName.ToUpper();
				shortcutKeys->Set( shortcutName, buttonSprite->GetScriptObject() );

				// Some other keys have additional bindings. Remember that the button here is
				// actually the virtual button, and the physical button could be swapped based
				// on the UseCircleForAccept business on JPN PS3.
				switch( i )
				{
					case BUTTON_JOY1:
					{
						shortcutKeys->Set( "ENTER", buttonSprite->GetScriptObject() );
						break;
					}
					case BUTTON_JOY2:
					{
						shortcutKeys->Set( "ESCAPE", buttonSprite->GetScriptObject() );
						shortcutKeys->Set( "BACKSPACE", buttonSprite->GetScriptObject() );
						break;
					}
					case BUTTON_TAB:
					{
						shortcutKeys->Set( "K_TAB", buttonSprite->GetScriptObject() );
						break;
					}
				}
			}

			if( buttons[ i ].label.IsEmpty() )
			{
				buttonSprite->SetVisible( false );
			}
			else
			{
				imageSprite->SetVisible( true );
				imageSprite->StopFrame( menuData->GetPlatform() + 1 );
				buttonSprite->SetVisible( true );
				buttonSprite->SetXPos( xPos );
				buttonText->SetText( buttons[ i ].label );
				xPos += ALIGNMENT_SCALE * ( buttonText->GetTextLength() + PER_BUTTON_PADDING );
			}
		}
		else
		{
			buttonSprite->SetVisible( false );
			idSWFScriptObject* const shortcutKeys = GetSWFObject()->GetGlobal( "shortcutKeys" ).GetObject();
			if( verify( shortcutKeys != NULL ) )
			{
				// RB: 64 bit fixes, changed NULL to 0
				buttonSprite->GetScriptObject()->Set( "onPress", 0 );
				// RB end

				// bind the main action - need to use all caps here because shortcuts are stored that way
				shortcutName = buttonName;
				shortcutName.ToUpper();
				shortcutKeys->Set( shortcutName, buttonSprite->GetScriptObject() );
			}
		}
	}
}

/*
========================
idMenuWidget_CommandBar::ReceiveEvent
========================
*/
bool idMenuWidget_CommandBar::ExecuteEvent( const idWidgetEvent& event )
{
	if( event.type == WIDGET_EVENT_COMMAND )
	{
		if( verify( event.arg >= 0 && event.arg < buttons.Num() ) )
		{
			HandleAction( buttons[ event.arg ].action, event, this );
		}
		return true;
	}
	else
	{
		return idMenuWidget::ExecuteEvent( event );
	}
}
