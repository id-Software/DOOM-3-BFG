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

#pragma hdrstop
#include "precompiled.h"

#include "DeviceContext.h"
#include "Window.h"
#include "UserInterfaceLocal.h"
#include "BindWindow.h"


void idBindWindow::CommonInit()
{
	bindName = "";
	waitingOnKey = false;
}

idBindWindow::idBindWindow( idUserInterfaceLocal* g ) : idWindow( g )
{
	gui = g;
	CommonInit();
}

idBindWindow::~idBindWindow()
{

}


const char* idBindWindow::HandleEvent( const sysEvent_t* event, bool* updateVisuals )
{
	static char ret[ 256 ];

	if( !( event->evType == SE_KEY && event->evValue2 ) )
	{
		return "";
	}

	int key = event->evValue;

	if( waitingOnKey )
	{
		waitingOnKey = false;
		if( key == K_ESCAPE )
		{
			idStr::snPrintf( ret, sizeof( ret ), "clearbind \"%s\"", bindName.GetName() );
		}
		else
		{
			idStr::snPrintf( ret, sizeof( ret ), "bind %i \"%s\"", key, bindName.GetName() );
		}
		return ret;
	}
	else
	{
		if( key == K_MOUSE1 )
		{
			waitingOnKey = true;
			gui->SetBindHandler( this );
			return "";
		}
	}

	return "";
}

idWinVar* idBindWindow::GetWinVarByName( const char* _name, bool fixup, drawWin_t** owner )
{

	if( idStr::Icmp( _name, "bind" ) == 0 )
	{
		return &bindName;
	}

	return idWindow::GetWinVarByName( _name, fixup, owner );
}

void idBindWindow::PostParse()
{
	idWindow::PostParse();
	bindName.SetGuiInfo( gui->GetStateDict(), bindName );
	bindName.Update();
	//bindName = state.GetString("bind");
	flags |= ( WIN_HOLDCAPTURE | WIN_CANFOCUS );
}

void idBindWindow::Draw( int time, float x, float y )
{
	idVec4 color = foreColor;

	idStr str;
	if( waitingOnKey )
	{
		str = idLocalization::GetString( "#str_07000" );
	}
	else if( bindName.Length() )
	{
		str = bindName.c_str();
	}
	else
	{
		str = idLocalization::GetString( "#str_07001" );
	}

	if( waitingOnKey || ( hover && !noEvents && Contains( gui->CursorX(), gui->CursorY() ) ) )
	{
		color = hoverColor;
	}
	else
	{
		hover = false;
	}

	dc->DrawText( str, textScale, textAlign, color, textRect, false, -1 );
}

void idBindWindow::Activate( bool activate, idStr& act )
{
	idWindow::Activate( activate, act );
	bindName.Update();
}
