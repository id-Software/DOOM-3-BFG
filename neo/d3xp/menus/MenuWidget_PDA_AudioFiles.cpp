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

static const int MAX_AUDIO_ITEMS = 3;


/*
========================
idMenuWidget_PDA_AudioFiles::~idMenuWidget_PDA_AudioFiles
========================
*/
idMenuWidget_PDA_AudioFiles::~idMenuWidget_PDA_AudioFiles()
{
}

/*
========================
idMenuWidget_PDA_AudioFiles::Initialize
========================
*/
void idMenuWidget_PDA_AudioFiles::Initialize( idMenuHandler* data )
{

	idMenuWidget_DynamicList* pdaAudioList = new( TAG_SWF ) idMenuWidget_DynamicList();

	pdaAudioList->SetSpritePath( GetSpritePath(), "info", "options" );
	pdaAudioList->SetNumVisibleOptions( MAX_AUDIO_ITEMS );
	pdaAudioList->SetWrappingAllowed( true );

	while( pdaAudioList->GetChildren().Num() < MAX_AUDIO_ITEMS )
	{
		idMenuWidget_Button* const buttonWidget = new( TAG_SWF ) idMenuWidget_Button();
		buttonWidget->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_SELECT_PDA_AUDIO, pdaAudioList->GetChildren().Num() );
		buttonWidget->Initialize( data );
		pdaAudioList->AddChild( buttonWidget );
	}
	pdaAudioList->Initialize( data );

	AddChild( pdaAudioList );
}

/*
========================
idMenuWidget_PDA_AudioFiles::Update
========================
*/
void idMenuWidget_PDA_AudioFiles::Update()
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

	idSWFScriptObject* dataObj = GetSprite()->GetScriptObject()->GetNestedObj( "info" );
	if( dataObj != NULL && pda != NULL )
	{

		idSWFTextInstance* txtOwner = dataObj->GetNestedText( "txtOwner" );
		if( txtOwner != NULL )
		{
			idStr ownerText = idLocalization::GetString( "#str_04203" );
			ownerText.Append( ": " );
			ownerText.Append( pda->GetFullName() );
			txtOwner->SetText( ownerText.c_str() );
		}

		idMenuWidget_DynamicList* const audioList = dynamic_cast< idMenuWidget_DynamicList* >( &GetChildByIndex( 0 ) );
		if( audioList != NULL )
		{
			audioFileNames.Clear();
			if( pda->GetNumAudios() == 0 )
			{
				idList< idStr > audioName;
				audioName.Append( idLocalization::GetString( "#str_04168" ) );
				audioList->GetChildByIndex( 0 ).SetState( WIDGET_STATE_DISABLED );
				audioFileNames.Append( audioName );
			}
			else
			{
				audioList->GetChildByIndex( 0 ).SetState( WIDGET_STATE_NORMAL );
				const idDeclAudio* aud = NULL;
				for( int index = 0; index < pda->GetNumAudios(); ++index )
				{
					idList< idStr > audioName;
					aud = pda->GetAudioByIndex( index );
					if( aud != NULL )
					{
						audioName.Append( aud->GetAudioName() );
					}
					else
					{
						audioName.Append( "" );
					}
					audioFileNames.Append( audioName );
				}
			}

			audioList->SetListData( audioFileNames );
			if( audioList->BindSprite( root ) )
			{
				audioList->Update();
			}
		}
	}

	//idSWFSpriteInstance * dataSprite = dataObj->GetSprite();
}

/*
========================
idMenuWidget_PDA_AudioFiles::ObserveEvent
========================
*/
void idMenuWidget_PDA_AudioFiles::ObserveEvent( const idMenuWidget& widget, const idWidgetEvent& event )
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
					GetSprite()->PlayFrame( "rollOff" );
				}
				else if( ( pdaIndex == 0 || GetSprite()->GetCurrentFrame() <= 2 || GetSprite()->GetCurrentFrame() > GetSprite()->FindFrame( "idle" ) ) && list->GetViewIndex() != 0 )
				{
					GetSprite()->PlayFrame( "rollOn" );
				}
			}
			pdaIndex = list->GetViewIndex();
			if( GetParent() != NULL && menuData != NULL && menuData->ActiveScreen() == PDA_AREA_USER_DATA )
			{
				GetParent()->Update();
			}
			else
			{
				Update();
			}
			idMenuWidget_DynamicList* const audioList = dynamic_cast< idMenuWidget_DynamicList* >( &GetChildByIndex( 0 ) );
			if( audioList != NULL )
			{
				audioList->SetFocusIndex( 0 );
				audioList->SetViewIndex( 0 );
			}
			break;
		}
		case WIDGET_EVENT_FOCUS_OFF:
		{
			idMenuWidget_DynamicList* const audioList = dynamic_cast< idMenuWidget_DynamicList* >( &GetChildByIndex( 0 ) );
			if( audioList != NULL )
			{
				audioList->SetFocusIndex( 0 );
				audioList->SetViewIndex( 0 );
			}
			Update();
			break;
		}
	}
}