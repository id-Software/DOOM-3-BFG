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

#include "DeviceContext.h"
#include "Window.h"
#include "UserInterfaceLocal.h"
#include "FieldWindow.h"


void idFieldWindow::CommonInit()
{
	cursorPos = 0;
	lastTextLength = 0;
	lastCursorPos = 0;
	paintOffset = 0;
	showCursor = false;
}

idFieldWindow::idFieldWindow( idUserInterfaceLocal* g ) : idWindow( g )
{
	gui = g;
	CommonInit();
}

idFieldWindow::~idFieldWindow()
{

}

bool idFieldWindow::ParseInternalVar( const char* _name, idTokenParser* src )
{
	if( idStr::Icmp( _name, "cursorvar" ) == 0 )
	{
		ParseString( src, cursorVar );
		return true;
	}
	if( idStr::Icmp( _name, "showcursor" ) == 0 )
	{
		showCursor = src->ParseBool();
		return true;
	}
	return idWindow::ParseInternalVar( _name, src );
}


void idFieldWindow::CalcPaintOffset( int len )
{
	lastCursorPos = cursorPos;
	lastTextLength = len;
	paintOffset = 0;
	int tw = dc->TextWidth( text, textScale, -1 );
	if( tw < textRect.w )
	{
		return;
	}
	while( tw > textRect.w && len > 0 )
	{
		tw = dc->TextWidth( text, textScale, --len );
		paintOffset++;
	}
}


void idFieldWindow::Draw( int time, float x, float y )
{
	float scale = textScale;
	int len = text.Length();
	cursorPos = gui->State().GetInt( cursorVar );
	if( len != lastTextLength || cursorPos != lastCursorPos )
	{
		CalcPaintOffset( len );
	}
	idRectangle rect = textRect;
	if( paintOffset >= len )
	{
		paintOffset = 0;
	}
	if( cursorPos > len )
	{
		cursorPos = len;
	}
	dc->DrawText( &text[paintOffset], scale, 0, foreColor, rect, false, ( ( flags & WIN_FOCUS ) || showCursor ) ? cursorPos - paintOffset : -1 );
}

