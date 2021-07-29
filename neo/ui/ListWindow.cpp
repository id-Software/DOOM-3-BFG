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
#include "ListWindow.h"

// Number of pixels above the text that the rect starts
static const int pixelOffset = 3;

// number of pixels between columns
static const int tabBorder = 4;

// Time in milliseconds between clicks to register as a double-click
static const int doubleClickSpeed = 300;

void idListWindow::CommonInit()
{
	typed = "";
	typedTime = 0;
	clickTime = 0;
	currentSel.Clear();
	top = 0;
	sizeBias = 0;
	horizontal = false;
	scroller = new( TAG_OLD_UI ) idSliderWindow( gui );
	multipleSel = false;
}

idListWindow::idListWindow( idUserInterfaceLocal* g ) : idWindow( g )
{
	gui = g;
	CommonInit();
}

void idListWindow::SetCurrentSel( int sel )
{
	currentSel.Clear();
	currentSel.Append( sel );
}

void idListWindow::ClearSelection( int sel )
{
	int cur = currentSel.FindIndex( sel );
	if( cur >= 0 )
	{
		currentSel.RemoveIndex( cur );
	}
}

void idListWindow::AddCurrentSel( int sel )
{
	currentSel.Append( sel );
}

int idListWindow::GetCurrentSel()
{
	return ( currentSel.Num() ) ? currentSel[0] : 0;
}

bool idListWindow::IsSelected( int index )
{
	return ( currentSel.FindIndex( index ) >= 0 );
}

const char* idListWindow::HandleEvent( const sysEvent_t* event, bool* updateVisuals )
{
	// need to call this to allow proper focus and capturing on embedded children
	const char* ret = idWindow::HandleEvent( event, updateVisuals );

	float vert = GetMaxCharHeight();
	int numVisibleLines = textRect.h / vert;

	int key = event->evValue;

	if( event->evType == SE_KEY )
	{
		if( !event->evValue2 )
		{
			// We only care about key down, not up
			return ret;
		}

		if( key == K_MOUSE1 || key == K_MOUSE2 )
		{
			// If the user clicked in the scroller, then ignore it
			if( scroller->Contains( gui->CursorX(), gui->CursorY() ) )
			{
				return ret;
			}
		}

		if( ( key == K_ENTER || key == K_KP_ENTER ) )
		{
			RunScript( ON_ENTER );
			return cmd;
		}

		if( key == K_MWHEELUP )
		{
			key = K_UPARROW;
		}
		else if( key == K_MWHEELDOWN )
		{
			key = K_DOWNARROW;
		}

		if( key == K_MOUSE1 )
		{
			if( Contains( gui->CursorX(), gui->CursorY() ) )
			{
				int cur = ( int )( ( gui->CursorY() - actualY - pixelOffset ) / vert ) + top;
				if( cur >= 0 && cur < listItems.Num() )
				{
					if( multipleSel && ( idKeyInput::IsDown( K_LCTRL ) || idKeyInput::IsDown( K_RCTRL ) ) )
					{
						if( IsSelected( cur ) )
						{
							ClearSelection( cur );
						}
						else
						{
							AddCurrentSel( cur );
						}
					}
					else
					{
						if( IsSelected( cur ) && ( gui->GetTime() < clickTime + doubleClickSpeed ) )
						{
							// Double-click causes ON_ENTER to get run
							RunScript( ON_ENTER );
							return cmd;
						}
						SetCurrentSel( cur );

						clickTime = gui->GetTime();
					}
				}
				else
				{
					SetCurrentSel( listItems.Num() - 1 );
				}
			}
		}
		else if( key == K_UPARROW || key == K_PGUP || key == K_DOWNARROW || key == K_PGDN )
		{
			int numLines = 1;

			if( key == K_PGUP || key == K_PGDN )
			{
				numLines = numVisibleLines / 2;
			}

			if( key == K_UPARROW || key == K_PGUP )
			{
				numLines = -numLines;
			}

			if( idKeyInput::IsDown( K_LCTRL ) || idKeyInput::IsDown( K_RCTRL ) )
			{
				top += numLines;
			}
			else
			{
				SetCurrentSel( GetCurrentSel() + numLines );
			}
		}
		else
		{
			return ret;
		}
	}
	else if( event->evType == SE_CHAR )
	{
		if( !idStr::CharIsPrintable( key ) )
		{
			return ret;
		}

		if( gui->GetTime() > typedTime + 1000 )
		{
			typed = "";
		}
		typedTime = gui->GetTime();
		typed.Append( key );

		for( int i = 0; i < listItems.Num(); i++ )
		{
			if( idStr::Icmpn( typed, listItems[i], typed.Length() ) == 0 )
			{
				SetCurrentSel( i );
				break;
			}
		}

	}
	else
	{
		return ret;
	}

	if( GetCurrentSel() < 0 )
	{
		SetCurrentSel( 0 );
	}

	if( GetCurrentSel() >= listItems.Num() )
	{
		SetCurrentSel( listItems.Num() - 1 );
	}

	if( scroller->GetHigh() > 0.0f )
	{
		if( !idKeyInput::IsDown( K_LCTRL ) && !idKeyInput::IsDown( K_RCTRL ) )
		{
			if( top > GetCurrentSel() - 1 )
			{
				top = GetCurrentSel() - 1;
			}
			if( top < GetCurrentSel() - numVisibleLines + 2 )
			{
				top = GetCurrentSel() - numVisibleLines + 2;
			}
		}

		if( top > listItems.Num() - 2 )
		{
			top = listItems.Num() - 2;
		}
		if( top < 0 )
		{
			top = 0;
		}
		scroller->SetValue( top );
	}
	else
	{
		top = 0;
		scroller->SetValue( 0.0f );
	}

	if( key != K_MOUSE1 )
	{
		// Send a fake mouse click event so onAction gets run in our parents
		const sysEvent_t ev = sys->GenerateMouseButtonEvent( 1, true );
		idWindow::HandleEvent( &ev, updateVisuals );
	}

	if( currentSel.Num() > 0 )
	{
		for( int i = 0; i < currentSel.Num(); i++ )
		{
			gui->SetStateInt( va( "%s_sel_%i", listName.c_str(), i ), currentSel[i] );
		}
	}
	else
	{
		gui->SetStateInt( va( "%s_sel_0", listName.c_str() ), 0 );
	}
	gui->SetStateInt( va( "%s_numsel", listName.c_str() ), currentSel.Num() );

	return ret;
}


bool idListWindow::ParseInternalVar( const char* _name, idTokenParser* src )
{
	if( idStr::Icmp( _name, "horizontal" ) == 0 )
	{
		horizontal = src->ParseBool();
		return true;
	}
	if( idStr::Icmp( _name, "listname" ) == 0 )
	{
		ParseString( src, listName );
		return true;
	}
	if( idStr::Icmp( _name, "tabstops" ) == 0 )
	{
		ParseString( src, tabStopStr );
		return true;
	}
	if( idStr::Icmp( _name, "tabaligns" ) == 0 )
	{
		ParseString( src, tabAlignStr );
		return true;
	}
	if( idStr::Icmp( _name, "multipleSel" ) == 0 )
	{
		multipleSel = src->ParseBool();
		return true;
	}
	if( idStr::Icmp( _name, "tabvaligns" ) == 0 )
	{
		ParseString( src, tabVAlignStr );
		return true;
	}
	if( idStr::Icmp( _name, "tabTypes" ) == 0 )
	{
		ParseString( src, tabTypeStr );
		return true;
	}
	if( idStr::Icmp( _name, "tabIconSizes" ) == 0 )
	{
		ParseString( src, tabIconSizeStr );
		return true;
	}
	if( idStr::Icmp( _name, "tabIconVOffset" ) == 0 )
	{
		ParseString( src, tabIconVOffsetStr );
		return true;
	}

	idStr strName = _name;
	if( idStr::Icmp( strName.Left( 4 ), "mtr_" ) == 0 )
	{
		idStr matName;
		const idMaterial* mat;

		ParseString( src, matName );
		mat = declManager->FindMaterial( matName );
		if( mat != NULL && !mat->TestMaterialFlag( MF_DEFAULTED ) )
		{
			mat->SetSort( SS_GUI );
		}
		iconMaterials.Set( _name, mat );
		return true;
	}

	return idWindow::ParseInternalVar( _name, src );
}

idWinVar* idListWindow::GetWinVarByName( const char* _name, bool fixup, drawWin_t** owner )
{
	return idWindow::GetWinVarByName( _name, fixup, owner );
}

void idListWindow::PostParse()
{
	idWindow::PostParse();

	InitScroller( horizontal );

	idList<int> tabStops;
	idList<int> tabAligns;
	if( tabStopStr.Length() )
	{
		idParser src( tabStopStr, tabStopStr.Length(), "tabstops", LEXFL_NOFATALERRORS | LEXFL_NOSTRINGCONCAT | LEXFL_NOSTRINGESCAPECHARS );
		idToken tok;
		while( src.ReadToken( &tok ) )
		{
			if( tok == "," )
			{
				continue;
			}
			tabStops.Append( atoi( tok ) );
		}
	}
	if( tabAlignStr.Length() )
	{
		idParser src( tabAlignStr, tabAlignStr.Length(), "tabaligns", LEXFL_NOFATALERRORS | LEXFL_NOSTRINGCONCAT | LEXFL_NOSTRINGESCAPECHARS );
		idToken tok;
		while( src.ReadToken( &tok ) )
		{
			if( tok == "," )
			{
				continue;
			}
			tabAligns.Append( atoi( tok ) );
		}
	}
	idList<int> tabVAligns;
	if( tabVAlignStr.Length() )
	{
		idParser src( tabVAlignStr, tabVAlignStr.Length(), "tabvaligns", LEXFL_NOFATALERRORS | LEXFL_NOSTRINGCONCAT | LEXFL_NOSTRINGESCAPECHARS );
		idToken tok;
		while( src.ReadToken( &tok ) )
		{
			if( tok == "," )
			{
				continue;
			}
			tabVAligns.Append( atoi( tok ) );
		}
	}

	idList<int> tabTypes;
	if( tabTypeStr.Length() )
	{
		idParser src( tabTypeStr, tabTypeStr.Length(), "tabtypes", LEXFL_NOFATALERRORS | LEXFL_NOSTRINGCONCAT | LEXFL_NOSTRINGESCAPECHARS );
		idToken tok;
		while( src.ReadToken( &tok ) )
		{
			if( tok == "," )
			{
				continue;
			}
			tabTypes.Append( atoi( tok ) );
		}
	}
	idList<idVec2> tabSizes;
	if( tabIconSizeStr.Length() )
	{
		idParser src( tabIconSizeStr, tabIconSizeStr.Length(), "tabiconsizes", LEXFL_NOFATALERRORS | LEXFL_NOSTRINGCONCAT | LEXFL_NOSTRINGESCAPECHARS );
		idToken tok;
		while( src.ReadToken( &tok ) )
		{
			if( tok == "," )
			{
				continue;
			}
			idVec2 size;
			size.x = atoi( tok );

			src.ReadToken( &tok );	//","
			src.ReadToken( &tok );

			size.y = atoi( tok );
			tabSizes.Append( size );
		}
	}

	idList<float> tabIconVOffsets;
	if( tabIconVOffsetStr.Length() )
	{
		idParser src( tabIconVOffsetStr, tabIconVOffsetStr.Length(), "tabiconvoffsets", LEXFL_NOFATALERRORS | LEXFL_NOSTRINGCONCAT | LEXFL_NOSTRINGESCAPECHARS );
		idToken tok;
		while( src.ReadToken( &tok ) )
		{
			if( tok == "," )
			{
				continue;
			}
			tabIconVOffsets.Append( atof( tok ) );
		}
	}

	int c = tabStops.Num();
	bool doAligns = ( tabAligns.Num() == tabStops.Num() );
	for( int i = 0; i < c; i++ )
	{
		idTabRect r;
		r.x = tabStops[i];
		r.w = ( i < c - 1 ) ? tabStops[i + 1] - r.x - tabBorder : -1;
		r.align = ( doAligns ) ? tabAligns[i] : 0;
		if( tabVAligns.Num() > 0 )
		{
			r.valign = tabVAligns[i];
		}
		else
		{
			r.valign = 0;
		}
		if( tabTypes.Num() > 0 )
		{
			r.type = tabTypes[i];
		}
		else
		{
			r.type = TAB_TYPE_TEXT;
		}
		if( tabSizes.Num() > 0 )
		{
			r.iconSize = tabSizes[i];
		}
		else
		{
			r.iconSize.Zero();
		}
		if( tabIconVOffsets.Num() > 0 )
		{
			r.iconVOffset = tabIconVOffsets[i];
		}
		else
		{
			r.iconVOffset = 0;
		}
		tabInfo.Append( r );
	}
	flags |= WIN_CANFOCUS;
}

/*
================
idListWindow::InitScroller

This is the same as in idEditWindow
================
*/
void idListWindow::InitScroller( bool horizontal )
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

void idListWindow::Draw( int time, float x, float y )
{
	idVec4 color;
	idStr work;
	int count = listItems.Num();
	idRectangle rect = textRect;
	float scale = textScale;
	float lineHeight = GetMaxCharHeight();

	float bottom = textRect.Bottom();
	float width = textRect.w;

	if( scroller->GetHigh() > 0.0f )
	{
		if( horizontal )
		{
			bottom -= sizeBias;
		}
		else
		{
			width -= sizeBias;
			rect.w = width;
		}
	}

	if( noEvents || !Contains( gui->CursorX(), gui->CursorY() ) )
	{
		hover = false;
	}

	for( int i = top; i < count; i++ )
	{
		if( IsSelected( i ) )
		{
			rect.h = lineHeight;
			dc->DrawFilledRect( rect.x, rect.y + pixelOffset, rect.w, rect.h, borderColor );
			if( flags & WIN_FOCUS )
			{
				idVec4 color = borderColor;
				color.w = 1.0f;
				dc->DrawRect( rect.x, rect.y + pixelOffset, rect.w, rect.h, 1.0f, color );
			}
		}
		rect.y ++;
		rect.h = lineHeight - 1;
		if( hover && !noEvents && Contains( rect, gui->CursorX(), gui->CursorY() ) )
		{
			color = hoverColor;
		}
		else
		{
			color = foreColor;
		}
		rect.h = lineHeight + pixelOffset;
		rect.y --;

		if( tabInfo.Num() > 0 )
		{
			int start = 0;
			int tab = 0;
			int stop = listItems[i].Find( '\t', 0 );
			while( start < listItems[i].Length() )
			{
				if( tab >= tabInfo.Num() )
				{
					common->Warning( "idListWindow::Draw: gui '%s' window '%s' tabInfo.Num() exceeded", gui->GetSourceFile(), name.c_str() );
					break;
				}
				listItems[i].Mid( start, stop - start, work );

				rect.x = textRect.x + tabInfo[tab].x;
				rect.w = ( tabInfo[tab].w == -1 ) ? width - tabInfo[tab].x : tabInfo[tab].w;
				dc->PushClipRect( rect );

				if( tabInfo[tab].type == TAB_TYPE_TEXT )
				{
					dc->DrawText( work, scale, tabInfo[tab].align, color, rect, false, -1 );
				}
				else if( tabInfo[tab].type == TAB_TYPE_ICON )
				{

					const idMaterial**	hashMat;
					const idMaterial*	iconMat;

					// leaving the icon name empty doesn't draw anything
					if( work[0] != '\0' )
					{

						if( iconMaterials.Get( work, &hashMat ) == false )
						{
							iconMat = declManager->FindMaterial( "_default" );
						}
						else
						{
							iconMat = *hashMat;
						}

						idRectangle iconRect;
						iconRect.w = tabInfo[tab].iconSize.x;
						iconRect.h = tabInfo[tab].iconSize.y;

						if( tabInfo[tab].align == idDeviceContext::ALIGN_LEFT )
						{
							iconRect.x = rect.x;
						}
						else if( tabInfo[tab].align == idDeviceContext::ALIGN_CENTER )
						{
							iconRect.x = rect.x + rect.w / 2.0f - iconRect.w / 2.0f;
						}
						else if( tabInfo[tab].align == idDeviceContext::ALIGN_RIGHT )
						{
							iconRect.x  = rect.x + rect.w - iconRect.w;
						}

						if( tabInfo[tab].valign == 0 ) //Top
						{
							iconRect.y = rect.y + tabInfo[tab].iconVOffset;
						}
						else if( tabInfo[tab].valign == 1 )   //Center
						{
							iconRect.y = rect.y + rect.h / 2.0f - iconRect.h / 2.0f + tabInfo[tab].iconVOffset;
						}
						else if( tabInfo[tab].valign == 2 )   //Bottom
						{
							iconRect.y = rect.y + rect.h - iconRect.h + tabInfo[tab].iconVOffset;
						}

						dc->DrawMaterial( iconRect.x, iconRect.y, iconRect.w, iconRect.h, iconMat, idVec4( 1.0f, 1.0f, 1.0f, 1.0f ), 1.0f, 1.0f );

					}
				}

				dc->PopClipRect();

				start = stop + 1;
				stop = listItems[i].Find( '\t', start );
				if( stop < 0 )
				{
					stop = listItems[i].Length();
				}
				tab++;
			}
			rect.x = textRect.x;
			rect.w = width;
		}
		else
		{
			dc->DrawText( listItems[i], scale, 0, color, rect, false, -1 );
		}
		rect.y += lineHeight;
		if( rect.y > bottom )
		{
			break;
		}
	}
}

void idListWindow::Activate( bool activate, idStr& act )
{
	idWindow::Activate( activate, act );

	if( activate )
	{
		UpdateList();
	}
}

void idListWindow::HandleBuddyUpdate( idWindow* buddy )
{
	top = scroller->GetValue();
}

void idListWindow::UpdateList()
{
	idStr str, strName;
	listItems.Clear();
	for( int i = 0; i < MAX_LIST_ITEMS; i++ )
	{
		if( gui->State().GetString( va( "%s_item_%i", listName.c_str(), i ), "", str ) )
		{
			if( str.Length() )
			{
				listItems.Append( str );
			}
		}
		else
		{
			break;
		}
	}
	float vert = GetMaxCharHeight();
	int fit = textRect.h / vert;
	if( listItems.Num() < fit )
	{
		scroller->SetRange( 0.0f, 0.0f, 1.0f );
	}
	else
	{
		scroller->SetRange( 0.0f, ( listItems.Num() - fit ) + 1.0f, 1.0f );
	}

	SetCurrentSel( gui->State().GetInt( va( "%s_sel_0", listName.c_str() ) ) );

	float value = scroller->GetValue();
	if( value > listItems.Num() - 1 )
	{
		value = listItems.Num() - 1;
	}
	if( value < 0.0f )
	{
		value = 0.0f;
	}
	scroller->SetValue( value );
	top = value;

	typedTime = 0;
	clickTime = 0;
	typed = "";
}

void idListWindow::StateChanged( bool redraw )
{
	UpdateList();
}

