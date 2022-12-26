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
#include "SliderWindow.h"

/*
============
idSliderWindow::CommonInit
============
*/
void idSliderWindow::CommonInit()
{
	value = 0.0;
	low = 0.0;
	high = 100.0;
	stepSize = 1.0;
	thumbMat = declManager->FindMaterial( "_default" );
	buddyWin = NULL;

	cvar = NULL;
	cvar_init = false;
	liveUpdate = true;

	vertical = false;
	scrollbar = false;

	verticalFlip = false;
}

idSliderWindow::idSliderWindow( idUserInterfaceLocal* g ) : idWindow( g )
{
	gui = g;
	CommonInit();
}

idSliderWindow::~idSliderWindow()
{

}

bool idSliderWindow::ParseInternalVar( const char* _name, idTokenParser* src )
{
	if( idStr::Icmp( _name, "stepsize" ) == 0 || idStr::Icmp( _name, "step" ) == 0 )
	{
		stepSize = src->ParseFloat();
		return true;
	}
	if( idStr::Icmp( _name, "low" ) == 0 )
	{
		low = src->ParseFloat();
		return true;
	}
	if( idStr::Icmp( _name, "high" ) == 0 )
	{
		high = src->ParseFloat();
		return true;
	}
	if( idStr::Icmp( _name, "vertical" ) == 0 )
	{
		vertical = src->ParseBool();
		return true;
	}
	if( idStr::Icmp( _name, "verticalflip" ) == 0 )
	{
		verticalFlip = src->ParseBool();
		return true;
	}
	if( idStr::Icmp( _name, "scrollbar" ) == 0 )
	{
		scrollbar = src->ParseBool();
		return true;
	}
	if( idStr::Icmp( _name, "thumbshader" ) == 0 )
	{
		ParseString( src, thumbShader );
		declManager->FindMaterial( thumbShader );
		return true;
	}
	return idWindow::ParseInternalVar( _name, src );
}

idWinVar* idSliderWindow::GetWinVarByName( const char* _name, bool fixup, drawWin_t** owner )
{

	if( idStr::Icmp( _name, "value" ) == 0 )
	{
		return &value;
	}
	if( idStr::Icmp( _name, "cvar" ) == 0 )
	{
		return &cvarStr;
	}
	if( idStr::Icmp( _name, "liveUpdate" ) == 0 )
	{
		return &liveUpdate;
	}
	if( idStr::Icmp( _name, "cvarGroup" ) == 0 )
	{
		return &cvarGroup;
	}

	return idWindow::GetWinVarByName( _name, fixup, owner );
}

const char* idSliderWindow::HandleEvent( const sysEvent_t* event, bool* updateVisuals )
{

	if( !( event->evType == SE_KEY && event->evValue2 ) )
	{
		return "";
	}

	int key = event->evValue;

	if( event->evValue2 && key == K_MOUSE1 )
	{
		SetCapture( this );
		RouteMouseCoords( 0.0f, 0.0f );
		return "";
	}

	if( key == K_RIGHTARROW || key == K_KP_6 || ( key == K_MOUSE2 && gui->CursorY() > thumbRect.y ) )
	{
		value = value + stepSize;
	}

	if( key == K_LEFTARROW || key == K_KP_4 || ( key == K_MOUSE2 && gui->CursorY() < thumbRect.y ) )
	{
		value = value - stepSize;
	}

	if( buddyWin )
	{
		buddyWin->HandleBuddyUpdate( this );
	}
	else
	{
		gui->SetStateFloat( cvarStr, value );
		UpdateCvar( false );
	}

	return "";
}


void idSliderWindow::SetBuddy( idWindow* buddy )
{
	buddyWin = buddy;
}

void idSliderWindow::PostParse()
{
	idWindow::PostParse();
	value = 0.0;
	thumbMat = declManager->FindMaterial( thumbShader );
	thumbMat->SetSort( SS_GUI );
	thumbWidth = thumbMat->GetImageWidth();
	thumbHeight = thumbMat->GetImageHeight();
	//vertical = state.GetBool("vertical");
	//scrollbar = state.GetBool("scrollbar");
	flags |= ( WIN_HOLDCAPTURE | WIN_CANFOCUS );
	InitCvar();
}

void idSliderWindow::InitWithDefaults( const char* _name, const idRectangle& _rect, const idVec4& _foreColor, const idVec4& _matColor, const char* _background, const char* thumbShader, bool _vertical, bool _scrollbar )
{
	SetInitialState( _name );
	rect = _rect;
	foreColor = _foreColor;
	matColor = _matColor;
	thumbMat = declManager->FindMaterial( thumbShader );
	thumbMat->SetSort( SS_GUI );
	thumbWidth = thumbMat->GetImageWidth();
	thumbHeight = thumbMat->GetImageHeight();
	background = declManager->FindMaterial( _background );
	background->SetSort( SS_GUI );
	vertical = _vertical;
	scrollbar = _scrollbar;
	flags |= WIN_HOLDCAPTURE;
}

void idSliderWindow::SetRange( float _low, float _high, float _step )
{
	low = _low;
	high = _high;
	stepSize = _step;
}

void idSliderWindow::SetValue( float _value )
{
	value = _value;
}

void idSliderWindow::Draw( int time, float x, float y )
{
	idVec4 color = foreColor;

	if( !cvar && !buddyWin )
	{
		return;
	}

	if( !thumbWidth || !thumbHeight )
	{
		thumbWidth = thumbMat->GetImageWidth();
		thumbHeight = thumbMat->GetImageHeight();
	}

	UpdateCvar( true );
	if( value > high )
	{
		value = high;
	}
	else if( value < low )
	{
		value = low;
	}

	float range = high - low;

	if( range <= 0.0f )
	{
		return;
	}

	float thumbPos = ( range ) ? ( value - low ) / range : 0.0;
	if( vertical )
	{
		if( verticalFlip )
		{
			thumbPos = 1.f - thumbPos;
		}
		thumbPos *= drawRect.h - thumbHeight;
		thumbPos += drawRect.y;
		thumbRect.y = thumbPos;
		thumbRect.x = drawRect.x;
	}
	else
	{
		thumbPos *= drawRect.w - thumbWidth;
		thumbPos += drawRect.x;
		thumbRect.x = thumbPos;
		thumbRect.y = drawRect.y;
	}
	thumbRect.w = thumbWidth;
	thumbRect.h = thumbHeight;

	if( hover && !noEvents && Contains( gui->CursorX(), gui->CursorY() ) )
	{
		color = hoverColor;
	}
	else
	{
		hover = false;
	}
	if( flags & WIN_CAPTURE )
	{
		color = hoverColor;
		hover = true;
	}

	dc->DrawMaterial( thumbRect.x, thumbRect.y, thumbRect.w, thumbRect.h, thumbMat, color );
	if( flags & WIN_FOCUS )
	{
		dc->DrawRect( thumbRect.x + 1.0f, thumbRect.y + 1.0f, thumbRect.w - 2.0f, thumbRect.h - 2.0f, 1.0f, color );
	}
}


void idSliderWindow::DrawBackground( const idRectangle& _drawRect )
{
	if( !cvar && !buddyWin )
	{
		return;
	}

	if( high - low <= 0.0f )
	{
		return;
	}

	idRectangle r = _drawRect;
	if( !scrollbar )
	{
		if( vertical )
		{
			r.y += thumbHeight / 2.f;
			r.h -= thumbHeight;
		}
		else
		{
			r.x += thumbWidth / 2.0;
			r.w -= thumbWidth;
		}
	}
	idWindow::DrawBackground( r );
}

const char* idSliderWindow::RouteMouseCoords( float xd, float yd )
{
	float pct;

	if( !( flags & WIN_CAPTURE ) )
	{
		return "";
	}

	idRectangle r = drawRect;
	r.x = actualX;
	r.y = actualY;
	r.x += thumbWidth / 2.0;
	r.w -= thumbWidth;
	if( vertical )
	{
		r.y += thumbHeight / 2;
		r.h -= thumbHeight;
		if( gui->CursorY() >= r.y && gui->CursorY() <= r.Bottom() )
		{
			pct = ( gui->CursorY() - r.y ) / r.h;
			if( verticalFlip )
			{
				pct = 1.f - pct;
			}
			value = low + ( high - low ) * pct;
		}
		else if( gui->CursorY() < r.y )
		{
			if( verticalFlip )
			{
				value = high;
			}
			else
			{
				value = low;
			}
		}
		else
		{
			if( verticalFlip )
			{
				value = low;
			}
			else
			{
				value = high;
			}
		}
	}
	else
	{
		r.x += thumbWidth / 2;
		r.w -= thumbWidth;
		if( gui->CursorX() >= r.x && gui->CursorX() <= r.Right() )
		{
			pct = ( gui->CursorX() - r.x ) / r.w;
			value = low + ( high - low ) * pct;
		}
		else if( gui->CursorX() < r.x )
		{
			value = low;
		}
		else
		{
			value = high;
		}
	}

	if( buddyWin )
	{
		buddyWin->HandleBuddyUpdate( this );
	}
	else
	{
		gui->SetStateFloat( cvarStr, value );
	}
	UpdateCvar( false );

	return "";
}


void idSliderWindow::Activate( bool activate, idStr& act )
{
	idWindow::Activate( activate, act );
	if( activate )
	{
		UpdateCvar( true, true );
	}
}

/*
============
idSliderWindow::InitCvar
============
*/
void idSliderWindow::InitCvar()
{
	if( cvarStr[0] == '\0' )
	{
		if( !buddyWin )
		{
			common->Warning( "idSliderWindow::InitCvar: gui '%s' window '%s' has an empty cvar string", gui->GetSourceFile(), name.c_str() );
		}
		cvar_init = true;
		cvar = NULL;
		return;
	}

	cvar = cvarSystem->Find( cvarStr );
	if( !cvar )
	{
		common->Warning( "idSliderWindow::InitCvar: gui '%s' window '%s' references undefined cvar '%s'", gui->GetSourceFile(), name.c_str(), cvarStr.c_str() );
		cvar_init = true;
		return;
	}
}

/*
============
idSliderWindow::UpdateCvar
============
*/
void idSliderWindow::UpdateCvar( bool read, bool force )
{
	if( buddyWin || !cvar )
	{
		return;
	}
	if( force || liveUpdate )
	{
		value = cvar->GetFloat();
		if( value != gui->State().GetFloat( cvarStr ) )
		{
			if( read )
			{
				gui->SetStateFloat( cvarStr, value );
			}
			else
			{
				value = gui->State().GetFloat( cvarStr );
				cvar->SetFloat( value );
			}
		}
	}
}

/*
============
idSliderWindow::RunNamedEvent
============
*/
void idSliderWindow::RunNamedEvent( const char* eventName )
{
	idStr event, group;

	if( !idStr::Cmpn( eventName, "cvar read ", 10 ) )
	{
		event = eventName;
		group = event.Mid( 10, event.Length() - 10 );
		if( !group.Cmp( cvarGroup ) )
		{
			UpdateCvar( true, true );
		}
	}
	else if( !idStr::Cmpn( eventName, "cvar write ", 11 ) )
	{
		event = eventName;
		group = event.Mid( 11, event.Length() - 11 );
		if( !group.Cmp( cvarGroup ) )
		{
			UpdateCvar( false, true );
		}
	}
}

