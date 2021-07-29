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
idMenuWidget_PDA_Objective::Update
========================
*/
void idMenuWidget_PDA_Objective::Update()
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

	idSWFScriptObject* dataObj = GetSprite()->GetScriptObject()->GetNestedObj( "info" );
	idSWFSpriteInstance* dataSprite = dataObj->GetSprite();

	if( dataObj != NULL && dataSprite != NULL )
	{

		idSWFSpriteInstance* img = dataObj->GetNestedSprite( "objImg", "img" );

		if( player->GetInventory().objectiveNames.Num() == 0 )
		{
			dataSprite->StopFrame( 1 );
		}
		else
		{
			int numObjectives = player->GetInventory().objectiveNames.Num();

			int objStartIndex = 0;
			if( numObjectives == 1 )
			{
				dataSprite->StopFrame( 2 );
				objStartIndex = 0;
			}
			else
			{
				dataSprite->StopFrame( 3 );
				objStartIndex = 1;
			}

			idSWFTextInstance* txtDesc = dataObj->GetNestedText( "txtDesc" );

			int displayCount = 0;
			for( int index = numObjectives - 1; displayCount < 2 && index >= 0; --index )
			{

				if( img != NULL )
				{
					if( player->GetInventory().objectiveNames[index].screenshot == NULL )
					{
						img->SetVisible( false );
					}
					else
					{
						img->SetVisible( true );
						img->SetMaterial( player->GetInventory().objectiveNames[index].screenshot );
					}
				}

				idSWFSpriteInstance* objSel = dataObj->GetNestedSprite( va( "obj%d", objStartIndex - displayCount ), "sel" );
				idSWFTextInstance* txtNote = dataObj->GetNestedText( va( "obj%d", objStartIndex - displayCount ), "txtVal" );

				if( objSel != NULL )
				{
					if( displayCount == 0 )
					{
						objSel->SetVisible( true );
					}
					else
					{
						objSel->SetVisible( false );
					}
				}

				if( txtNote != NULL )
				{
					txtNote->SetText( player->GetInventory().objectiveNames[index].title.c_str() );
				}

				if( displayCount == 0 )
				{
					txtDesc->SetText( player->GetInventory().objectiveNames[index].text.c_str() );
				}

				displayCount++;
			}
		}

		// Set the main objective text
		idTarget_SetPrimaryObjective* mainObj = player->GetPrimaryObjective();
		idSWFTextInstance* txtMainObj = dataObj->GetNestedText( "txtObj" );
		if( txtMainObj != NULL )
		{
			if( mainObj != NULL )
			{
				txtMainObj->SetText( mainObj->spawnArgs.GetString( "text", idLocalization::GetString( "#str_04253" ) ) );
			}
			else
			{
				txtMainObj->SetText( idLocalization::GetString( "#str_02526" ) );
			}
		}
	}
}

/*
========================
idMenuWidget_Help::ObserveEvent
========================
*/
void idMenuWidget_PDA_Objective::ObserveEvent( const idMenuWidget& widget, const idWidgetEvent& event )
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
			if( GetSprite() != NULL )
			{
				if( list->GetViewIndex() == 0 )
				{
					GetSprite()->PlayFrame( "rollOn" );
				}
				else if( pdaIndex == 0 && list->GetViewIndex() != 0 )
				{
					GetSprite()->PlayFrame( "rollOff" );
				}
			}
			pdaIndex = list->GetViewIndex();
			Update();
			break;
		}
	}
}