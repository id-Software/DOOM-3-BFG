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


const static int NUM_SAVE_OPTIONS = 10;

/*
========================
idMenuScreen_Shell_Save::Initialize
========================
*/
void idMenuScreen_Shell_Save::Initialize( idMenuHandler* data )
{
	idMenuScreen::Initialize( data );

	if( data != NULL )
	{
		menuGUI = data->GetGUI();
	}

	SetSpritePath( "menuSave" );

	saveInfo = new( TAG_SWF ) idMenuWidget_Shell_SaveInfo();
	saveInfo->SetSpritePath( GetSpritePath(), "info", "details" );
	saveInfo->Initialize( data );
	saveInfo->SetForSaveScreen( true );

	options = new( TAG_SWF ) idMenuWidget_DynamicList();
	options->SetNumVisibleOptions( NUM_SAVE_OPTIONS );
	options->SetSpritePath( GetSpritePath(), "info", "options" );
	options->SetWrappingAllowed( true );
	while( options->GetChildren().Num() < NUM_SAVE_OPTIONS )
	{
		idMenuWidget_Button* const buttonWidget = new( TAG_SWF ) idMenuWidget_Button();
		buttonWidget->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_PRESS_FOCUSED, options->GetChildren().Num() );
		buttonWidget->RegisterEventObserver( saveInfo );
		buttonWidget->Initialize( data );
		options->AddChild( buttonWidget );
	}
	options->Initialize( data );

	AddChild( options );
	AddChild( saveInfo );

	btnBack = new( TAG_SWF ) idMenuWidget_Button();
	btnBack->Initialize( data );
	btnBack->SetLabel( "#str_swf_pause_menu" );
	btnBack->SetSpritePath( GetSpritePath(), "info", "btnBack" );
	btnBack->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_GO_BACK );

	AddChild( btnBack );

	btnDelete = new idMenuWidget_Button();
	btnDelete->Initialize( data );
	btnDelete->SetLabel( "" );
	btnDelete->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_JOY3_ON_PRESS );
	btnDelete->SetSpritePath( GetSpritePath(), "info", "btnDelete" );

	AddChild( btnDelete );

	options->AddEventAction( WIDGET_EVENT_SCROLL_DOWN ).Set( new( TAG_SWF ) idWidgetActionHandler( options, WIDGET_ACTION_EVENT_SCROLL_DOWN_START_REPEATER, WIDGET_EVENT_SCROLL_DOWN ) );
	options->AddEventAction( WIDGET_EVENT_SCROLL_UP ).Set( new( TAG_SWF ) idWidgetActionHandler( options, WIDGET_ACTION_EVENT_SCROLL_UP_START_REPEATER, WIDGET_EVENT_SCROLL_UP ) );
	options->AddEventAction( WIDGET_EVENT_SCROLL_DOWN_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( options, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_DOWN_RELEASE ) );
	options->AddEventAction( WIDGET_EVENT_SCROLL_UP_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( options, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_UP_RELEASE ) );
	options->AddEventAction( WIDGET_EVENT_SCROLL_DOWN_LSTICK ).Set( new( TAG_SWF ) idWidgetActionHandler( options, WIDGET_ACTION_EVENT_SCROLL_DOWN_START_REPEATER, WIDGET_EVENT_SCROLL_DOWN_LSTICK ) );
	options->AddEventAction( WIDGET_EVENT_SCROLL_UP_LSTICK ).Set( new( TAG_SWF ) idWidgetActionHandler( options, WIDGET_ACTION_EVENT_SCROLL_UP_START_REPEATER, WIDGET_EVENT_SCROLL_UP_LSTICK ) );
	options->AddEventAction( WIDGET_EVENT_SCROLL_DOWN_LSTICK_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( options, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_DOWN_LSTICK_RELEASE ) );
	options->AddEventAction( WIDGET_EVENT_SCROLL_UP_LSTICK_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( options, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_UP_LSTICK_RELEASE ) );
}

/*
========================
idMenuScreen_Shell_Save::Update
========================
*/
void idMenuScreen_Shell_Save::Update()
{

	UpdateSaveEnumerations();

	idSWFScriptObject& root = GetSWFObject()->GetRootObject();
	if( BindSprite( root ) )
	{
		idSWFTextInstance* heading = GetSprite()->GetScriptObject()->GetNestedText( "info", "txtHeading" );
		if( heading != NULL )
		{
			heading->SetText( "#str_02179" );	// SAVE GAME
			heading->SetStrokeInfo( true, 0.75f, 1.75f );
		}

		idSWFSpriteInstance* gradient = GetSprite()->GetScriptObject()->GetNestedSprite( "info", "gradient" );
		if( gradient != NULL && heading != NULL )
		{
			gradient->SetXPos( heading->GetTextLength() );
		}
	}

	if( btnBack != NULL )
	{
		btnBack->BindSprite( root );
	}

	idMenuScreen::Update();
}

/*
========================
idMenuScreen_Shell_Save::UpdateSaveEnumerations
========================
*/
void idMenuScreen_Shell_Save::UpdateSaveEnumerations()
{

	const saveGameDetailsList_t& saveGameInfo = session->GetSaveGameManager().GetEnumeratedSavegames();
	sortedSaves = saveGameInfo;
	idList< idList< idStr, TAG_IDLIB_LIST_MENU >, TAG_IDLIB_LIST_MENU > saveList;
	int newSaveOffset = 1;
	bool hasAutosave = false;

	if( session->GetSaveGameManager().IsWorking() )
	{
		idList< idStr > saveName;
		saveName.Append( "#str_dlg_refreshing" );
		saveList.Append( saveName );

		if( options != NULL )
		{
			options->SetListData( saveList );
			options->Update();
		}
	}
	else
	{

		for( int i = 0; i < saveGameInfo.Num(); ++i )
		{
			if( saveGameInfo[i].slotName.Icmp( "autosave" ) == 0 )
			{
				hasAutosave = true;
				break;
			}
		}

		if( saveGameInfo.Num() == MAX_SAVEGAMES || ( !hasAutosave && saveGameInfo.Num() == MAX_SAVEGAMES - 1 ) )
		{
			newSaveOffset = 0;
		}

		if( newSaveOffset != 0 )
		{
			idList< idStr > newSave;
			newSave.Append( "#str_swf_new_save_game" );
			saveList.Append( newSave );
		}

		if( options != NULL )
		{
			sortedSaves.Sort( idSort_SavesByDate() );

			for( int slot = 0; slot < sortedSaves.Num(); ++slot )
			{
				const idSaveGameDetails& details = sortedSaves[slot];
				if( details.slotName.Icmp( "autosave" ) == 0 )
				{
					sortedSaves.RemoveIndex( slot );
					slot--;
				}
			}
			// +1 because the first item is "New Save"
			saveList.SetNum( sortedSaves.Num() + newSaveOffset );
			for( int slot = 0; slot < sortedSaves.Num(); ++slot )
			{
				idStr& slotSaveName = saveList[ slot + newSaveOffset ].Alloc();
				const idSaveGameDetails& details = sortedSaves[slot];
				if( details.damaged )
				{
					slotSaveName =  va( S_COLOR_RED "%s", idLocalization::GetString( "#str_swf_corrupt_file" ) );
				}
				else if( details.GetSaveVersion() > BUILD_NUMBER )
				{
					slotSaveName =  va( S_COLOR_RED "%s", idLocalization::GetString( "#str_swf_wrong_version" ) );
				}
				else
				{
					if( details.slotName.Icmp( "autosave" ) == 0 )
					{
						slotSaveName.Append( S_COLOR_YELLOW );
					}
					else if( details.slotName.Icmp( "quick" ) == 0 )
					{
						slotSaveName.Append( S_COLOR_ORANGE );
					}
					slotSaveName.Append( details.GetMapName() );
				}
			}

			options->SetListData( saveList );
			options->Update();
		}
	}

	if( menuData != NULL )
	{
		idMenuWidget_CommandBar* cmdBar = menuData->GetCmdBar();
		if( cmdBar != NULL )
		{
			cmdBar->ClearAllButtons();
			idMenuWidget_CommandBar::buttonInfo_t* buttonInfo;
			buttonInfo = cmdBar->GetButton( idMenuWidget_CommandBar::BUTTON_JOY2 );
			if( menuData->GetPlatform() != 2 )
			{
				buttonInfo->label = "#str_00395";	// BACK
			}
			buttonInfo->action.Set( WIDGET_ACTION_GO_BACK );


			if( !session->GetSaveGameManager().IsWorking() )
			{
				buttonInfo = cmdBar->GetButton( idMenuWidget_CommandBar::BUTTON_JOY1 );
				if( menuData->GetPlatform() != 2 )
				{
					buttonInfo->label = "#str_02179";	// SAVE GAME
				}
				buttonInfo->action.Set( WIDGET_ACTION_PRESS_FOCUSED );

				if( options != NULL )
				{
					if( options->GetViewIndex() != 0 || ( options->GetViewIndex() == 0 && newSaveOffset == 0 ) )
					{
						buttonInfo = cmdBar->GetButton( idMenuWidget_CommandBar::BUTTON_JOY3 );
						if( menuData->GetPlatform() != 2 )
						{
							buttonInfo->label = "#str_02315";	// DELETE
						}
						buttonInfo->action.Set( WIDGET_ACTION_JOY3_ON_PRESS );

						if( btnDelete != NULL )
						{
							idSWFScriptObject& root = GetSWFObject()->GetRootObject();
							if( btnDelete->BindSprite( root ) )
							{
								if( menuData->GetPlatform() != 2 )
								{
									btnDelete->SetLabel( "" );
								}
								else
								{
									btnDelete->GetSprite()->SetVisible( true );
									btnDelete->SetLabel( "#str_02315" );
								}
							}
							btnDelete->Update();
						}
					}
					else
					{
						if( btnDelete != NULL )
						{
							idSWFScriptObject& root = GetSWFObject()->GetRootObject();
							if( btnDelete->BindSprite( root ) )
							{
								btnDelete->SetLabel( "" );
								btnDelete->Update();
							}
						}
					}
				}
			}
			cmdBar->Update();
		}
	}

	if( saveInfo != NULL )
	{
		saveInfo->Update();
	}

	if( options != NULL && options->GetTotalNumberOfOptions() > 0 && options->GetViewIndex() >= options->GetTotalNumberOfOptions() )
	{
		options->SetViewIndex( options->GetTotalNumberOfOptions() - newSaveOffset );
		if( options->GetViewOffset() > options->GetViewIndex() )
		{
			options->SetViewOffset( options->GetViewIndex() );
		}
		options->SetFocusIndex( options->GetViewIndex() - options->GetViewOffset() );
	}
}

/*
========================
idMenuScreen_Shell_Save::ShowScreen
========================
*/
void idMenuScreen_Shell_Save::ShowScreen( const mainMenuTransition_t transitionType )
{
	idMenuScreen::ShowScreen( transitionType );
}

/*
========================
idMenuScreen_Shell_Save::HideScreen
========================
*/
void idMenuScreen_Shell_Save::HideScreen( const mainMenuTransition_t transitionType )
{
	idMenuScreen::HideScreen( transitionType );
}

/*
========================
idMenuScreen_Shell_Save::SaveGame
========================
*/
void idMenuScreen_Shell_Save::SaveGame( int index )
{
	const saveGameDetailsList_t& saveGameInfo = session->GetSaveGameManager().GetEnumeratedSavegames();
	int newSaveOffset = 1;

	bool hasAutosave = false;
	for( int i = 0; i < saveGameInfo.Num(); ++i )
	{
		if( saveGameInfo[i].slotName.Icmp( "autosave" ) == 0 )
		{
			hasAutosave = true;
			break;
		}
	}

	if( saveGameInfo.Num() == MAX_SAVEGAMES || ( ( saveGameInfo.Num() == MAX_SAVEGAMES - 1 ) && !hasAutosave ) )
	{
		newSaveOffset = 0;
	}

	if( index == 0 && newSaveOffset == 1 )
	{
		// New save...

		// Scan all the savegames for the first doom3_xxx slot.
		const idStr savePrefix = "doom3_";
		uint64 slotMask = 0;
		for( int slot = 0; slot < saveGameInfo.Num(); ++slot )
		{
			const idSaveGameDetails& details = saveGameInfo[slot];

			if( details.slotName.Icmp( "autosave" ) == 0 )
			{
				continue;
			}

			idStr name = details.slotName;

			name.ToLower();		// PS3 saves are uppercase ... we need to lower case-ify them for comparison here
			name.StripLeading( savePrefix.c_str() );
			if( name.IsNumeric() )
			{
				int slotNumber = atoi( name.c_str() );
				slotMask |= ( 1ULL << slotNumber );
			}
		}

		int slotNumber = 0;
		for( slotNumber = 0; slotNumber < ( sizeof( slotMask ) * 8 ); slotNumber++ )
		{
			// If the slot isn't used, grab it
			if( !( slotMask & ( 1ULL << slotNumber ) ) )
			{
				break;
			}
		}

		assert( slotNumber < ( sizeof( slotMask ) * 8 ) );
		idStr name = va( "%s%d", savePrefix.c_str(), slotNumber );
		cmdSystem->AppendCommandText( va( "savegame %s\n", name.c_str() ) );

		// Throw up the saving message...
		common->Dialog().ShowSaveIndicator( true );
	}
	else
	{

		class idSWFScriptFunction_OverwriteSave : public idSWFScriptFunction_RefCounted
		{
		public:
			idSWFScriptFunction_OverwriteSave( gameDialogMessages_t _msg, bool _accept, int _index, idMenuScreen_Shell_Save* _screen )
			{
				msg = _msg;
				accept = _accept;
				index = _index;
				screen = _screen;
			}
			idSWFScriptVar Call( idSWFScriptObject* thisObject, const idSWFParmList& parms )
			{
				common->Dialog().ClearDialog( msg );
				if( accept && screen != NULL )
				{
					// Replace the save
					if( index < screen->GetSortedSaves().Num() )
					{
						idStr name = screen->GetSortedSaves()[ index ].slotName;
						cmdSystem->AppendCommandText( va( "savegame %s\n", name.c_str() ) );

						// Throw up the saving message...
						common->Dialog().ShowSaveIndicator( true );
					}
				}
				return idSWFScriptVar();
			}
		private:
			gameDialogMessages_t msg;
			int index;
			bool accept;
			idMenuScreen_Shell_Save* screen;
		};

		if( newSaveOffset == 1 )
		{
			index--;
		}

		common->Dialog().AddDialog( GDM_OVERWRITE_SAVE, DIALOG_ACCEPT_CANCEL, new idSWFScriptFunction_OverwriteSave( GDM_OVERWRITE_SAVE, true, index, this ), new idSWFScriptFunction_OverwriteSave( GDM_OVERWRITE_SAVE, false, index, this ), false );
	}

}

/*
========================
idMenuScreen_Shell_Save::DeleteGame
========================
*/
void idMenuScreen_Shell_Save::DeleteGame( int index )
{

	class idSWFScriptFunction_DeleteGame : public idSWFScriptFunction_RefCounted
	{
	public:
		idSWFScriptFunction_DeleteGame( gameDialogMessages_t _msg, bool _accept, int _index, idMenuScreen_Shell_Save* _screen )
		{
			msg = _msg;
			accept = _accept;
			index = _index;
			screen = _screen;
		}
		idSWFScriptVar Call( idSWFScriptObject* thisObject, const idSWFParmList& parms )
		{
			common->Dialog().ClearDialog( msg );
			if( accept && screen != NULL )
			{
				if( index < screen->GetSortedSaves().Num() )
				{
					session->DeleteSaveGameSync( screen->GetSortedSaves()[ index ].slotName );
				}
			}
			return idSWFScriptVar();
		}
	private:
		gameDialogMessages_t msg;
		int index;
		bool accept;
		idMenuScreen_Shell_Save* screen;
	};

	bool hasAutosave = false;
	const saveGameDetailsList_t& saveInfo = session->GetSaveGameManager().GetEnumeratedSavegames();
	for( int i = 0; i < saveInfo.Num(); ++i )
	{
		if( saveInfo[i].slotName.Icmp( "autosave" ) == 0 )
		{
			hasAutosave = true;
			break;
		}
	}

	if( ( saveInfo.Num() < MAX_SAVEGAMES - 1 ) || ( ( saveInfo.Num() == MAX_SAVEGAMES - 1 ) && hasAutosave ) )
	{
		index--;	// Subtract 1 from the index for 'New Game'
	}

	common->Dialog().AddDialog( GDM_DELETE_SAVE, DIALOG_ACCEPT_CANCEL, new idSWFScriptFunction_DeleteGame( GDM_DELETE_SAVE, true, index, this ), new idSWFScriptFunction_DeleteGame( GDM_DELETE_SAVE, false, index, this ), false );

}

/*
========================
idMenuScreen_Shell_Save::HandleAction
========================
*/
bool idMenuScreen_Shell_Save::HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled )
{

	if( menuData == NULL )
	{
		return true;
	}

	if( menuData->ActiveScreen() != SHELL_AREA_SAVE )
	{
		return false;
	}

	widgetAction_t actionType = action.GetType();
	const idSWFParmList& parms = action.GetParms();

	switch( actionType )
	{
		case WIDGET_ACTION_JOY4_ON_PRESS:
		{
			return true;
		}
		case WIDGET_ACTION_JOY3_ON_PRESS:
		{

			if( options == NULL )
			{
				return true;
			}

			DeleteGame( options->GetViewIndex() );
			return true;
		}
		case WIDGET_ACTION_GO_BACK:
		{
			menuData->SetNextScreen( SHELL_AREA_ROOT, MENU_TRANSITION_SIMPLE );
			return true;
		}
		case WIDGET_ACTION_PRESS_FOCUSED:
		{
			if( options == NULL )
			{
				return true;
			}

			if( session->GetSaveGameManager().IsWorking() )
			{
				return true;
			}

			if( parms.Num() == 1 )
			{
				int selectionIndex = parms[0].ToInteger();
				if( selectionIndex != options->GetFocusIndex() )
				{
					options->SetViewIndex( options->GetViewOffset() + selectionIndex );
					options->SetFocusIndex( selectionIndex );
					return true;
				}
			}

			SaveGame( options->GetViewIndex() );
			return true;
		}
		case WIDGET_ACTION_SCROLL_VERTICAL:
		{
			return true;
		}
	}

	return idMenuWidget::HandleAction( action, event, widget, forceHandled );
}
