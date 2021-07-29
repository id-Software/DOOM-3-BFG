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
idMenuWidget_PDA_UserData::Update
========================
*/
void idMenuWidget_PDA_UserData::Update()
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

	idPlayer* player = gameLocal.GetLocalPlayer();
	if( player == NULL )
	{
		return;
	}

	if( pdaIndex > player->GetInventory().pdas.Num() )
	{
		return;
	}

	const idDeclPDA* pda = player->GetInventory().pdas[ pdaIndex ];

	idSWFScriptObject* dataObj = GetSprite()->GetScriptObject();

	if( dataObj != NULL && pda != NULL )
	{

		idSWFTextInstance* txtName = dataObj->GetNestedText( "txtName" );
		idSWFTextInstance* txtId = dataObj->GetNestedText( "txtId" );
		idSWFTextInstance* txtLocation = dataObj->GetNestedText( "txtLocation" );
		idSWFTextInstance* txtRank = dataObj->GetNestedText( "txtRank" );
		idSWFTextInstance* txtClearance = dataObj->GetNestedText( "txtClearance" );
		idSWFTextInstance* txtLocHeading = dataObj->GetNestedText( "txtLocHeading" );

		if( txtName != NULL )
		{

			if( pdaIndex == 0 )
			{
				txtName->SetIgnoreColor( false );
				txtName->SetText( session->GetLocalUserName( 0 ) );
			}
			else
			{
				txtName->SetIgnoreColor( false );
				txtName->SetText( pda->GetFullName() );
			}
		}

		if( txtLocHeading != NULL )
		{
			if( pdaIndex == 0 )
			{
				txtLocHeading->SetText( idLocalization::GetString( "#str_02516" ) );	// location
			}
			else
			{
				txtLocHeading->SetText( idLocalization::GetString( "#str_02515" ) );	// post
			}
		}

		if( txtId != NULL )
		{
			txtId->SetText( pda->GetID() );
		}

		if( txtLocation != NULL )
		{
			if( pdaIndex == 0 )
			{
				idLocationEntity* locationEntity = gameLocal.LocationForPoint( player->GetEyePosition() );
				if( locationEntity )
				{
					txtLocation->SetText( locationEntity->GetLocation() );
				}
				else
				{
					txtLocation->SetText( idLocalization::GetString( "#str_02911" ) );
				}
			}
			else
			{
				txtLocation->SetText( pda->GetPost() );
			}
		}

		if( txtRank != NULL )
		{
			txtRank->SetText( pda->GetTitle() );
		}

		if( txtClearance != NULL )
		{
			const char* security = pda->GetSecurity();
			if( *security == '\0' )
			{
				txtClearance->SetText( idLocalization::GetString( "#str_00066" ) );
			}
			else
			{
				txtClearance->SetText( security );
			}
		}
	}
}

/*
========================
idMenuWidget_Help::ObserveEvent
========================
*/
void idMenuWidget_PDA_UserData::ObserveEvent( const idMenuWidget& widget, const idWidgetEvent& event )
{
	const idMenuWidget_Button* const button = dynamic_cast< const idMenuWidget_Button* >( &widget );
	if( button == NULL )
	{
		return;
	}

	const idMenuWidget* const listWidget = button->GetParent();

	if( listWidget == NULL )
	{
		return;
	}

	switch( event.type )
	{
		case WIDGET_EVENT_FOCUS_ON:
		{
			const idMenuWidget_DynamicList* const list = dynamic_cast< const idMenuWidget_DynamicList* const >( listWidget );
			pdaIndex = list->GetViewIndex();
			if( GetParent() != NULL && menuData != NULL && menuData->ActiveScreen() == PDA_AREA_USER_DATA )
			{
				GetParent()->Update();
			}
			else
			{
				Update();
			}
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
			const idMenuWidget_DynamicList* const list = dynamic_cast< const idMenuWidget_DynamicList* const >( listWidget );
			pdaIndex = list->GetViewIndex();
			Update();
			break;
		}
		case WIDGET_EVENT_ROLL_OUT:
		{
			const idMenuWidget_DynamicList* const list = dynamic_cast< const idMenuWidget_DynamicList* const >( listWidget );
			pdaIndex = list->GetViewIndex();
			Update();
			break;
		}
	}
}
