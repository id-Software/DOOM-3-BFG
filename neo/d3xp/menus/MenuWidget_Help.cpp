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
idMenuWidget_Help

Shows a help tooltip message based on observed events.  It's expected that the widgets being
observed are all buttons, and therefore have a GetDescription() call to get the help message.

SWF object structure
--------------------
HELPTOOLTIP (Frames: shown, shown, hide, hidden)
	txtOption
		txtValue (Text)
Note: Frame 1 should, effectively, be a "hidden" state.

Future work:
- Make this act more like a help tooltip when on PC?
================================================================================================
*/

/*
========================
idMenuWidget_Help::Update
========================
*/
void idMenuWidget_Help::Update()
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

	const idStr& msg = ( lastHoveredMessage.Length() > 0 ) ? lastHoveredMessage : lastFocusedMessage;
	if( msg.Length() > 0 && !hideMessage )
	{
		// try to show it if...
		//		- we are on the first frame
		//		- we aren't still animating while being between the "show" and "shown" frames
		//
		if( GetSprite()->GetCurrentFrame() != GetSprite()->FindFrame( "shown" )
				&& ( GetSprite()->GetCurrentFrame() == 1 || !( GetSprite()->IsPlaying() && GetSprite()->IsBetweenFrames( "show", "shown" ) ) ) )
		{
			GetSprite()->PlayFrame( "show" );
		}

		idSWFScriptObject* const textObject = GetSprite()->GetScriptObject()->GetNestedObj( "txtOption", "txtValue" );
		if( textObject != NULL )
		{
			idSWFTextInstance* const text = textObject->GetText();
			text->SetText( msg );
			text->SetStrokeInfo( true, 0.75f, 2.0f );
		}
	}
	else
	{
		// try to hide it
		if( GetSprite()->GetCurrentFrame() != 1
				&& GetSprite()->GetCurrentFrame() != GetSprite()->FindFrame( "hidden" )
				&& !GetSprite()->IsBetweenFrames( "hide", "hidden" ) )
		{
			GetSprite()->PlayFrame( "hide" );
		}
	}
}

/*
========================
idMenuWidget_Help::ObserveEvent
========================
*/
void idMenuWidget_Help::ObserveEvent( const idMenuWidget& widget, const idWidgetEvent& event )
{
	const idMenuWidget_Button* const button = dynamic_cast< const idMenuWidget_Button* >( &widget );
	if( button == NULL )
	{
		return;
	}

	switch( event.type )
	{
		case WIDGET_EVENT_FOCUS_ON:
		{
			hideMessage = false;
			lastFocusedMessage = button->GetDescription();
			lastHoveredMessage.Clear();
			Update();
			break;
		}
		case WIDGET_EVENT_FOCUS_OFF:
		{
			// Don't do anything when losing focus. Focus updates come in pairs, so we can
			// skip doing anything on the "lost focus" event, and instead do updates on the
			// "got focus" event.
			break;
		}
		case WIDGET_EVENT_ROLL_OVER:
		{
			idStr desc = button->GetDescription();
			if( desc.IsEmpty() )
			{
				hideMessage = true;
			}
			else
			{
				hideMessage = false;
				lastHoveredMessage = button->GetDescription();
			}
			Update();
			break;
		}
		case WIDGET_EVENT_ROLL_OUT:
		{
			hideMessage = false;
			lastHoveredMessage.Clear();
			Update();
			break;
		}
	}
}

