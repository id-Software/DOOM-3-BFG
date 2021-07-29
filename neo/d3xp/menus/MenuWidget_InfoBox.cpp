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
idMenuWidget_InfoBox::Update
========================
*/
void idMenuWidget_InfoBox::Initialize( idMenuHandler* data )
{
	idMenuWidget::Initialize( data );
}

/*
========================
idMenuWidget_InfoBox::Update
========================
*/
void idMenuWidget_InfoBox::Update()
{

	if( GetSWFObject() == NULL )
	{
		return;
	}

	idSWFScriptObject& root = GetSWFObject()->GetRootObject();
	if( !BindSprite( root ) || GetSprite() == NULL )
	{
		return;
	}

	idSWFTextInstance* txtHeading = GetSprite()->GetScriptObject()->GetNestedText( "info", "heading", "txtVal" );
	idSWFTextInstance* txtBody = GetSprite()->GetScriptObject()->GetNestedText( "info", "txtBody" );

	if( txtHeading != NULL )
	{
		txtHeading->SetText( heading );
	}

	if( txtBody != NULL )
	{
		txtBody->SetText( info );
	}

	if( scrollbar != NULL && txtBody != NULL )
	{
		txtBody->CalcMaxScroll();
		scrollbar->Update();
	}

	idSWFScriptObject* info = GetSprite()->GetScriptObject()->GetNestedObj( "info" );
	if( info != NULL )
	{
		info->Set( "onRollOver", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_ROLL_OVER, 0 ) );
		info->Set( "onRollOut", new( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_ROLL_OUT, 0 ) );
	}

}

/*
========================
idMenuWidget_InfoBox::ObserveEvent
========================
*/
void idMenuWidget_InfoBox::ResetInfoScroll()
{

	idSWFScriptObject& root = GetSWFObject()->GetRootObject();
	if( !BindSprite( root ) || GetSprite() == NULL )
	{
		return;
	}

	idSWFTextInstance* txtBody = GetSprite()->GetScriptObject()->GetNestedText( "info", "txtBody" );
	if( txtBody != NULL )
	{
		txtBody->scroll = 0;
	}

	if( scrollbar != NULL )
	{
		scrollbar->Update();
	}
}

/*
========================
idMenuWidget_InfoBox::Scroll
========================
*/
void idMenuWidget_InfoBox::Scroll( int d )
{

	idSWFTextInstance* txtBody = GetSprite()->GetScriptObject()->GetNestedText( "info", "txtBody" );

	if( txtBody != NULL && txtBody->scroll + d >= 0 && txtBody->scroll + d <= txtBody->maxscroll )
	{
		txtBody->scroll += d;
	}

	if( scrollbar != NULL )
	{
		scrollbar->Update();
	}

}

/*
========================
idMenuWidget_InfoBox::GetScroll
========================
*/
int	idMenuWidget_InfoBox::GetScroll()
{

	idSWFTextInstance* txtBody = GetSprite()->GetScriptObject()->GetNestedText( "info", "txtBody" );
	if( txtBody != NULL )
	{
		return txtBody->scroll;
	}

	return 0;
}

/*
========================
idMenuWidget_InfoBox::GetMaxScroll
========================
*/
int idMenuWidget_InfoBox::GetMaxScroll()
{

	idSWFTextInstance* txtBody = GetSprite()->GetScriptObject()->GetNestedText( "info", "txtBody" );
	if( txtBody != NULL )
	{
		return txtBody->maxscroll;
	}

	return 0;
}

/*
========================
idMenuWidget_InfoBox::SetScroll
========================
*/
void idMenuWidget_InfoBox::SetScroll( int scroll )
{

	idSWFTextInstance* txtBody = GetSprite()->GetScriptObject()->GetNestedText( "info", "txtBody" );

	if( txtBody != NULL && scroll <= txtBody->maxscroll )
	{
		txtBody->scroll = scroll;
	}

}

/*
========================
idMenuWidget_InfoBox::SetScrollbar
========================
*/
void idMenuWidget_InfoBox::SetScrollbar( idMenuWidget_ScrollBar* bar )
{
	scrollbar = bar;
}

/*
========================
idMenuWidget_InfoBox::ObserveEvent
========================
*/
bool idMenuWidget_InfoBox::HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled )
{

	const idSWFParmList& parms = action.GetParms();

	if( action.GetType() == WIDGET_ACTION_SCROLL_VERTICAL )
	{
		const scrollType_t scrollType = static_cast< scrollType_t >( event.arg );
		if( scrollType == SCROLL_SINGLE )
		{
			Scroll( parms[ 0 ].ToInteger() );
		}
		return true;
	}

	return idMenuWidget::HandleAction( action, event, widget, forceHandled );
}

/*
========================
idMenuWidget_InfoBox::ObserveEvent
========================
*/
void idMenuWidget_InfoBox::ObserveEvent( const idMenuWidget& widget, const idWidgetEvent& event )
{
	switch( event.type )
	{
		case WIDGET_EVENT_FOCUS_ON:
		{
			ResetInfoScroll();
			break;
		}
	}
}