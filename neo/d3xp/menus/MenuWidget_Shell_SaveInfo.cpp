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
idMenuWidget_Shell_SaveInfo::Update
========================
*/
void idMenuWidget_Shell_SaveInfo::Update()
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

	const saveGameDetailsList_t& saveGameInfo = session->GetSaveGameManager().GetEnumeratedSavegames();

	saveGameDetailsList_t sortedSaves = saveGameInfo;
	sortedSaves.Sort( idSort_SavesByDate() );

	for( int slot = 0; slot < sortedSaves.Num(); ++slot )
	{
		const idSaveGameDetails& details = sortedSaves[slot];
		if( forSaveScreen && details.slotName.Icmp( "autosave" ) == 0 )
		{
			sortedSaves.RemoveIndex( slot );
			slot--;
		}
	}

	idStr info;
	if( loadIndex >= 0 && sortedSaves.Num() != 0 && loadIndex < sortedSaves.Num() )
	{
		const idSaveGameDetails& details = sortedSaves[ loadIndex ];

		info.Append( Sys_TimeStampToStr( details.date ) );
		info.Append( "\n" );

		// PS3 only strings that use the dict just set
		const char* expansionStr = "";
		switch( details.GetExpansion() )
		{
			case GAME_D3XP:
				expansionStr = idLocalization::GetString( "#str_swf_resurrection" );
				break;
			case GAME_D3LE:
				expansionStr = idLocalization::GetString( "#str_swf_lost_episodes" );
				break;
			case GAME_BASE:
				expansionStr = idLocalization::GetString( "#str_swf_doom3" );
				break;
			default:
				expansionStr = idLocalization::GetString( "#str_savegame_title" );
				break;
		}

		const char* difficultyStr = "";
		switch( details.GetDifficulty() )
		{
			case 0:
				difficultyStr = idLocalization::GetString( "#str_04089" );
				break;
			case 1:
				difficultyStr = idLocalization::GetString( "#str_04091" );
				break;
			case 2:
				difficultyStr = idLocalization::GetString( "#str_04093" );
				break;
			case 3:
				difficultyStr = idLocalization::GetString( "#str_02357" );
				break;
		}

		idStr summary;
		summary.Format( idLocalization::GetString( "#str_swf_save_info_format" ), difficultyStr, Sys_SecToStr( details.GetPlaytime() ), expansionStr );

		info.Append( summary );

		if( details.damaged )
		{
			info.Append( "\n" );
			info.Append( va( "^1%s^0", idLocalization::GetString( "#str_swf_damaged" ) ) );
		}
		else if( details.GetSaveVersion() > BUILD_NUMBER )
		{
			info.Append( "\n" );
			info.Append( va( "^1%s^0", idLocalization::GetString( "#str_swf_wrong_version" ) ) );
		}
	}

	idSWFTextInstance* infoSprite = GetSprite()->GetScriptObject()->GetNestedText( "txtDesc" );
	if( infoSprite != NULL )
	{
		infoSprite->SetText( info );
	}

	idSWFSpriteInstance* img = GetSprite()->GetScriptObject()->GetNestedSprite( "img" );
	if( img != NULL )
	{
		// TODO_SPARTY: until we have a thumbnail hide the image
		img->SetVisible( false );
	}
}

/*
========================
idMenuWidget_Help::ObserveEvent
========================
*/
void idMenuWidget_Shell_SaveInfo::ObserveEvent( const idMenuWidget& widget, const idWidgetEvent& event )
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
			loadIndex = list->GetViewIndex();

			const saveGameDetailsList_t& detailList = session->GetSaveGameManager().GetEnumeratedSavegames();
			bool hasAutoSave = false;
			for( int i = 0; i < detailList.Num(); ++i )
			{
				if( detailList[i].slotName.Icmp( "autosave" ) == 0 )
				{
					hasAutoSave = true;
				}
			}


			if( forSaveScreen && ( ( detailList.Num() < MAX_SAVEGAMES - 1 ) || ( ( detailList.Num() == MAX_SAVEGAMES - 1 ) && hasAutoSave ) ) )
			{
				loadIndex -= 1;
			}

			Update();

			idMenuScreen_Shell_Load* loadScreen = dynamic_cast< idMenuScreen_Shell_Load* >( GetParent() );
			if( loadScreen )
			{
				loadScreen->UpdateSaveEnumerations();
			}

			idMenuScreen_Shell_Save* saveScreen = dynamic_cast< idMenuScreen_Shell_Save* >( GetParent() );
			if( saveScreen )
			{
				saveScreen->UpdateSaveEnumerations();
			}

			break;
		}
	}
}