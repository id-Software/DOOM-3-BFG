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
#include "EditWindow.h"


bool idEditWindow::ParseInternalVar( const char* _name, idTokenParser* src )
{
	if( idStr::Icmp( _name, "maxchars" ) == 0 )
	{
		maxChars = src->ParseInt();
		return true;
	}
	if( idStr::Icmp( _name, "numeric" ) == 0 )
	{
		numeric = src->ParseBool();
		return true;
	}
	if( idStr::Icmp( _name, "wrap" ) == 0 )
	{
		wrap = src->ParseBool();
		return true;
	}
	if( idStr::Icmp( _name, "readonly" ) == 0 )
	{
		readonly = src->ParseBool();
		return true;
	}
	if( idStr::Icmp( _name, "forceScroll" ) == 0 )
	{
		forceScroll = src->ParseBool();
		return true;
	}
	if( idStr::Icmp( _name, "source" ) == 0 )
	{
		ParseString( src, sourceFile );
		return true;
	}
	if( idStr::Icmp( _name, "password" ) == 0 )
	{
		password = src->ParseBool();
		return true;
	}
	if( idStr::Icmp( _name, "cvarMax" ) == 0 )
	{
		cvarMax = src->ParseInt();
		return true;
	}

	return idWindow::ParseInternalVar( _name, src );
}

idWinVar* idEditWindow::GetWinVarByName( const char* _name, bool fixup, drawWin_t** owner )
{
	if( idStr::Icmp( _name, "cvar" ) == 0 )
	{
		return &cvarStr;
	}
	if( idStr::Icmp( _name, "password" ) == 0 )
	{
		return &password;
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

void idEditWindow::CommonInit()
{
	maxChars = 128;
	numeric = false;
	paintOffset = 0;
	cursorPos = 0;
	cursorLine = 0;
	cvarMax = 0;
	wrap = false;
	sourceFile = "";
	scroller = NULL;
	sizeBias = 0;
	lastTextLength = 0;
	forceScroll = false;
	password = false; // DG: this is a bool, so use false, not NULL
	cvar = NULL;
	liveUpdate = true;
	readonly = false;

	scroller = new( TAG_OLD_UI ) idSliderWindow( gui );
}


idEditWindow::idEditWindow( idUserInterfaceLocal* g ) : idWindow( g )
{
	gui = g;
	CommonInit();
}

idEditWindow::~idEditWindow()
{

}

void idEditWindow::GainFocus()
{
	cursorPos = text.Length();
	EnsureCursorVisible();
}

void idEditWindow::Draw( int time, float x, float y )
{
	idVec4 color = foreColor;

	UpdateCvar( true );

	int len = text.Length();
	if( len != lastTextLength )
	{
		scroller->SetValue( 0.0f );
		EnsureCursorVisible();
		lastTextLength = len;
	}
	float scale = textScale;

	idStr		pass;
	const char* buffer;
	if( password )
	{
		const char* temp = text;
		for( ; *temp; temp++ )
		{
			pass += "*";
		}
		buffer = pass;
	}
	else
	{
		buffer = text;
	}

	if( cursorPos > len )
	{
		cursorPos = len;
	}

	idRectangle rect = textRect;

	rect.x -= paintOffset;
	rect.w += paintOffset;

	if( wrap && scroller->GetHigh() > 0.0f )
	{
		float lineHeight = GetMaxCharHeight() + 5;
		rect.y -= scroller->GetValue() * lineHeight;
		rect.w -= sizeBias;
		rect.h = ( breaks.Num() + 1 ) * lineHeight;
	}

	if( hover && !noEvents && Contains( gui->CursorX(), gui->CursorY() ) )
	{
		color = hoverColor;
	}
	else
	{
		hover = false;
	}
	if( flags & WIN_FOCUS )
	{
		color = hoverColor;
	}

	dc->DrawText( buffer, scale, 0, color, rect, wrap, ( flags & WIN_FOCUS ) ? cursorPos : -1 );
}

/*
=============
idEditWindow::HandleEvent
=============
*/
const char* idEditWindow::HandleEvent( const sysEvent_t* event, bool* updateVisuals )
{
	static char buffer[ MAX_EDITFIELD ];

	if( wrap )
	{
		// need to call this to allow proper focus and capturing on embedded children
		const char* ret = idWindow::HandleEvent( event, updateVisuals );
		if( ret != NULL && *ret != '\0' )
		{
			return ret;
		}
	}

	if( ( event->evType != SE_CHAR && event->evType != SE_KEY ) )
	{
		return "";
	}

	idStr::Copynz( buffer, text.c_str(), sizeof( buffer ) );
	int key = event->evValue;
	int len = text.Length();

	if( event->evType == SE_CHAR )
	{
		if( key == '`' )
		{
			return "";
		}

		if( updateVisuals )
		{
			*updateVisuals = true;
		}

		if( maxChars && len > maxChars )
		{
			len = maxChars;
		}

		if( readonly )
		{
			return "";
		}

		//
		// ignore any non printable chars (except enter when wrap is enabled)
		//
		if( wrap && ( key == K_ENTER || key == K_KP_ENTER ) )
		{
		}
		else if( !idStr::CharIsPrintable( key ) )
		{
			return "";
		}

		if( numeric )
		{
			if( ( key < '0' || key > '9' ) && key != '.' )
			{
				return "";
			}
		}

		if( dc->GetOverStrike() )
		{
			if( maxChars && cursorPos >= maxChars )
			{
				return "";
			}
		}
		else
		{
			if( ( len == MAX_EDITFIELD - 1 ) || ( maxChars && len >= maxChars ) )
			{
				return "";
			}
			memmove( &buffer[ cursorPos + 1 ], &buffer[ cursorPos ], len + 1 - cursorPos );
		}

		buffer[ cursorPos ] = key;

		text = buffer;
		UpdateCvar( false );
		RunScript( ON_ACTION );

		if( cursorPos < len + 1 )
		{
			cursorPos++;
		}
		EnsureCursorVisible();

	}
	else if( event->evType == SE_KEY && event->evValue2 )
	{

		if( updateVisuals )
		{
			*updateVisuals = true;
		}

		if( key == K_DEL )
		{
			if( readonly )
			{
				return "";
			}
			if( cursorPos < len )
			{
				memmove( &buffer[cursorPos], &buffer[cursorPos + 1], len - cursorPos );
				text = buffer;
				UpdateCvar( false );
				RunScript( ON_ACTION );
			}
			return "";
		}

		if( key == K_BACKSPACE )  	// ctrl-h is backspace
		{
			if( readonly )
			{
				return "";
			}
			if( cursorPos > 0 )
			{
				if( cursorPos >= len )
				{
					buffer[len - 1] = 0;
					cursorPos = len - 1;
				}
				else
				{
					memmove( &buffer[ cursorPos - 1 ], &buffer[ cursorPos ], len + 1 - cursorPos );
					cursorPos--;
				}

				text = buffer;
				UpdateCvar( false );
				RunScript( ON_ACTION );
			}

			return "";
		}
		if( key == K_RIGHTARROW )
		{
			if( cursorPos < len )
			{
				if( ( idKeyInput::IsDown( K_LCTRL ) || idKeyInput::IsDown( K_RCTRL ) ) )
				{
					// skip to next word
					while( ( cursorPos < len ) && ( buffer[ cursorPos ] != ' ' ) )
					{
						cursorPos++;
					}

					while( ( cursorPos < len ) && ( buffer[ cursorPos ] == ' ' ) )
					{
						cursorPos++;
					}
				}
				else
				{
					if( cursorPos < len )
					{
						cursorPos++;
					}
				}
			}

			EnsureCursorVisible();

			return "";
		}

		if( key == K_LEFTARROW )
		{
			if( ( idKeyInput::IsDown( K_LCTRL ) || idKeyInput::IsDown( K_RCTRL ) ) )
			{
				// skip to previous word
				while( ( cursorPos > 0 ) && ( buffer[ cursorPos - 1 ] == ' ' ) )
				{
					cursorPos--;
				}

				while( ( cursorPos > 0 ) && ( buffer[ cursorPos - 1 ] != ' ' ) )
				{
					cursorPos--;
				}
			}
			else
			{
				if( cursorPos > 0 )
				{
					cursorPos--;
				}
			}

			EnsureCursorVisible();

			return "";
		}

		if( key == K_HOME )
		{
			if( ( idKeyInput::IsDown( K_LCTRL ) || idKeyInput::IsDown( K_RCTRL ) ) || cursorLine <= 0 || ( cursorLine >= breaks.Num() ) )
			{
				cursorPos = 0;
			}
			else
			{
				cursorPos = breaks[cursorLine];
			}
			EnsureCursorVisible();
			return "";
		}

		if( key == K_END )
		{
			if( ( idKeyInput::IsDown( K_LCTRL ) || idKeyInput::IsDown( K_RCTRL ) ) || ( cursorLine < -1 ) || ( cursorLine >= breaks.Num() - 1 ) )
			{
				cursorPos = len;
			}
			else
			{
				cursorPos = breaks[cursorLine + 1] - 1;
			}
			EnsureCursorVisible();
			return "";
		}

		if( key == K_INS )
		{
			if( !readonly )
			{
				dc->SetOverStrike( !dc->GetOverStrike() );
			}
			return "";
		}

		if( key == K_DOWNARROW )
		{
			if( ( idKeyInput::IsDown( K_LCTRL ) || idKeyInput::IsDown( K_RCTRL ) ) )
			{
				scroller->SetValue( scroller->GetValue() + 1.0f );
			}
			else
			{
				if( cursorLine < breaks.Num() - 1 )
				{
					int offset = cursorPos - breaks[cursorLine];
					cursorPos = breaks[cursorLine + 1] + offset;
					EnsureCursorVisible();
				}
			}
		}

		if( key == K_UPARROW )
		{
			if( ( idKeyInput::IsDown( K_LCTRL ) || idKeyInput::IsDown( K_RCTRL ) ) )
			{
				scroller->SetValue( scroller->GetValue() - 1.0f );
			}
			else
			{
				if( cursorLine > 0 )
				{
					int offset = cursorPos - breaks[cursorLine];
					cursorPos = breaks[cursorLine - 1] + offset;
					EnsureCursorVisible();
				}
			}
		}

		if( key == K_ENTER || key == K_KP_ENTER )
		{
			RunScript( ON_ACTION );
			RunScript( ON_ENTER );
			return cmd;
		}

		if( key == K_ESCAPE )
		{
			RunScript( ON_ESC );
			return cmd;
		}

	}
	else if( event->evType == SE_KEY && !event->evValue2 )
	{
		if( key == K_ENTER || key == K_KP_ENTER )
		{
			RunScript( ON_ENTERRELEASE );
			return cmd;
		}
		else
		{
			RunScript( ON_ACTIONRELEASE );
		}
	}

	return "";
}

void idEditWindow::PostParse()
{
	idWindow::PostParse();

	if( maxChars == 0 )
	{
		maxChars = 10;
	}
	if( sourceFile.Length() )
	{
		void* buffer;
		fileSystem->ReadFile( sourceFile, &buffer );
		text = ( char* ) buffer;
		fileSystem->FreeFile( buffer );
	}

	InitCvar();
	InitScroller( false );

	EnsureCursorVisible();

	flags |= WIN_CANFOCUS;
}

/*
================
idEditWindow::InitScroller

This is the same as in idListWindow
================
*/
void idEditWindow::InitScroller( bool horizontal )
{
	const char* thumbImage = "guis/assets/scrollbar_thumb.tga";
	const char* barImage = "guis/assets/scrollbarv.tga";
	const char* scrollerName = "_scrollerWinV";

	if( horizontal )
	{
		barImage = "guis/assets/scrollbarh.tga";
		scrollerName = "_scrollerWinH";
	}

	const idMaterial* mat = declManager->FindMaterial( barImage );
	mat->SetSort( SS_GUI );
	sizeBias = mat->GetImageWidth();

	idRectangle scrollRect;
	if( horizontal )
	{
		sizeBias = mat->GetImageHeight();
		scrollRect.x = 0;
		scrollRect.y = ( clientRect.h - sizeBias );
		scrollRect.w = clientRect.w;
		scrollRect.h = sizeBias;
	}
	else
	{
		scrollRect.x = ( clientRect.w - sizeBias );
		scrollRect.y = 0;
		scrollRect.w = sizeBias;
		scrollRect.h = clientRect.h;
	}

	scroller->InitWithDefaults( scrollerName, scrollRect, foreColor, matColor, mat->GetName(), thumbImage, !horizontal, true );
	InsertChild( scroller, NULL );
	scroller->SetBuddy( this );
}

void idEditWindow::HandleBuddyUpdate( idWindow* buddy )
{
}

void idEditWindow::EnsureCursorVisible()
{
	if( readonly )
	{
		cursorPos = -1;
	}
	else if( maxChars == 1 )
	{
		cursorPos = 0;
	}

	if( !dc )
	{
		return;
	}

	SetFont();
	if( !wrap )
	{
		int cursorX = 0;
		if( password )
		{
			cursorX = cursorPos * dc->CharWidth( '*', textScale );
		}
		else
		{
			int i = 0;
			while( i < text.Length() && i < cursorPos )
			{
				if( idStr::IsColor( &text[i] ) )
				{
					i += 2;
				}
				else
				{
					cursorX += dc->CharWidth( text[i], textScale );
					i++;
				}
			}
		}
		int maxWidth = GetMaxCharWidth();
		int left = cursorX - maxWidth;
		int right = ( cursorX - textRect.w ) + maxWidth;

		if( paintOffset > left )
		{
			// When we go past the left side, we want the text to jump 6 characters
			paintOffset = left - maxWidth * 6;
		}
		if( paintOffset <  right )
		{
			paintOffset = right;
		}
		if( paintOffset < 0 )
		{
			paintOffset = 0;
		}
		scroller->SetRange( 0.0f, 0.0f, 1.0f );

	}
	else
	{
		// Word wrap

		breaks.Clear();
		idRectangle rect = textRect;
		rect.w -= sizeBias;
		dc->DrawText( text, textScale, textAlign, colorWhite, rect, true, ( flags & WIN_FOCUS ) ? cursorPos : -1, true, &breaks );

		int fit = textRect.h / ( GetMaxCharHeight() + 5 );
		if( fit < breaks.Num() + 1 )
		{
			scroller->SetRange( 0, breaks.Num() + 1 - fit, 1 );
		}
		else
		{
			// The text fits completely in the box
			scroller->SetRange( 0.0f, 0.0f, 1.0f );
		}

		if( forceScroll )
		{
			scroller->SetValue( breaks.Num() - fit );
		}
		else if( readonly )
		{
		}
		else
		{
			cursorLine = 0;
			for( int i = 1; i < breaks.Num(); i++ )
			{
				if( cursorPos >= breaks[i] )
				{
					cursorLine = i;
				}
				else
				{
					break;
				}
			}
			int topLine = idMath::Ftoi( scroller->GetValue() );
			if( cursorLine < topLine )
			{
				scroller->SetValue( cursorLine );
			}
			else if( cursorLine >= topLine + fit )
			{
				scroller->SetValue( ( cursorLine - fit ) + 1 );
			}
		}
	}
}

void idEditWindow::Activate( bool activate, idStr& act )
{
	idWindow::Activate( activate, act );
	if( activate )
	{
		UpdateCvar( true, true );
		EnsureCursorVisible();
	}
}

/*
============
idEditWindow::InitCvar
============
*/
void idEditWindow::InitCvar()
{
	if( cvarStr[0] == '\0' )
	{
		if( text.GetName() == NULL )
		{
			common->Warning( "idEditWindow::InitCvar: gui '%s' window '%s' has an empty cvar string", gui->GetSourceFile(), name.c_str() );
		}
		cvar = NULL;
		return;
	}

	cvar = cvarSystem->Find( cvarStr );
	if( !cvar )
	{
		common->Warning( "idEditWindow::InitCvar: gui '%s' window '%s' references undefined cvar '%s'", gui->GetSourceFile(), name.c_str(), cvarStr.c_str() );
		return;
	}
}

/*
============
idEditWindow::UpdateCvar
============
*/
void idEditWindow::UpdateCvar( bool read, bool force )
{
	if( force || liveUpdate )
	{
		if( cvar )
		{
			if( read )
			{
				text = cvar->GetString();
			}
			else
			{
				cvar->SetString( text );
				if( cvarMax && ( cvar->GetInteger() > cvarMax ) )
				{
					cvar->SetInteger( cvarMax );
				}
			}
		}
	}
}

/*
============
idEditWindow::RunNamedEvent
============
*/
void idEditWindow::RunNamedEvent( const char* eventName )
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
