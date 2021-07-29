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


const static int NUM_LOAD_OPTIONS = 10;

/*
========================
idMenuScreen_Shell_Load::Initialize
========================
*/
void idMenuScreen_Shell_Load::Initialize( idMenuHandler* data )
{
	idMenuScreen::Initialize( data );

	if( data != NULL )
	{
		menuGUI = data->GetGUI();
	}

	SetSpritePath( "menuLoad" );

	saveInfo = new( TAG_SWF ) idMenuWidget_Shell_SaveInfo();
	saveInfo->SetSpritePath( GetSpritePath(), "info", "details" );
	saveInfo->Initialize( data );

	options = new( TAG_SWF ) idMenuWidget_DynamicList();
	options->SetNumVisibleOptions( NUM_LOAD_OPTIONS );
	options->SetSpritePath( GetSpritePath(), "info", "options" );
	options->SetWrappingAllowed( true );
	while( options->GetChildren().Num() < NUM_LOAD_OPTIONS )
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
	idMenuHandler_Shell* handler = dynamic_cast< idMenuHandler_Shell* >( data );
	if( handler != NULL && handler->GetInGame() )
	{
		btnBack->SetLabel( "#str_swf_pause_menu" );
	}
	else
	{
		btnBack->SetLabel( "#str_swf_campaign" );
	}
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
idMenuScreen_Shell_Load::Update
========================
*/
void idMenuScreen_Shell_Load::Update()
{

	UpdateSaveEnumerations();

	idSWFScriptObject& root = GetSWFObject()->GetRootObject();
	if( BindSprite( root ) )
	{
		idSWFTextInstance* heading = GetSprite()->GetScriptObject()->GetNestedText( "info", "txtHeading" );
		if( heading != NULL )
		{
			heading->SetText( "#str_02187" );	// LOAD GAME
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
idMenuScreen_Shell_Load::UpdateSaveEnumerations
========================
*/
void idMenuScreen_Shell_Load::UpdateSaveEnumerations()
{

	const saveGameDetailsList_t& saveGameInfo = session->GetSaveGameManager().GetEnumeratedSavegames();
	sortedSaves = saveGameInfo;
	sortedSaves.Sort( idSort_SavesByDate() );

	if( options != NULL )
	{
		idList< idList< idStr, TAG_IDLIB_LIST_MENU >, TAG_IDLIB_LIST_MENU > saveList;
		if( session->GetSaveGameManager().IsWorking() )
		{
			idList< idStr > saveName;
			saveName.Append( "#str_dlg_refreshing" );
			saveList.Append( saveName );
		}
		else if( sortedSaves.Num() == 0 )
		{
			idList< idStr > saveName;
			saveName.Append( "#str_no_saves_found" );
			saveList.Append( saveName );
		}
		else
		{

			saveList.SetNum( sortedSaves.Num() );
			for( int slot = 0; slot < sortedSaves.Num(); ++slot )
			{
				idStr& slotSaveName = saveList[slot].Alloc();
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
		}
		options->SetListData( saveList );
		options->Update();
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



			if( sortedSaves.Num() > 0 && !session->GetSaveGameManager().IsWorking() )
			{
				buttonInfo = cmdBar->GetButton( idMenuWidget_CommandBar::BUTTON_JOY1 );
				if( menuData->GetPlatform() != 2 )
				{
					buttonInfo->label = "#str_02187";	// LOAD GAME
				}
				buttonInfo->action.Set( WIDGET_ACTION_PRESS_FOCUSED );

				buttonInfo = cmdBar->GetButton( idMenuWidget_CommandBar::BUTTON_JOY3 );
				if( menuData->GetPlatform() != 2 )
				{
					buttonInfo->label = "#str_02315";	// DELETE GAME
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
			cmdBar->Update();
		}
	}

	if( saveInfo != NULL )
	{
		saveInfo->Update();
	}

	if( options != NULL && options->GetTotalNumberOfOptions() > 0 && options->GetViewIndex() >= options->GetTotalNumberOfOptions() )
	{
		options->SetViewIndex( options->GetTotalNumberOfOptions() - 1 );
		if( options->GetViewOffset() > options->GetViewIndex() )
		{
			options->SetViewOffset( options->GetViewIndex() );
		}
		options->SetFocusIndex( options->GetViewIndex() - options->GetViewOffset() );
	}
}

/*
========================
idMenuScreen_Shell_Load::ShowScreen
========================
*/
void idMenuScreen_Shell_Load::ShowScreen( const mainMenuTransition_t transitionType )
{
	idMenuScreen::ShowScreen( transitionType );
}

/*
========================
idMenuScreen_Shell_Load::HideScreen
========================
*/
void idMenuScreen_Shell_Load::HideScreen( const mainMenuTransition_t transitionType )
{
	idMenuScreen::HideScreen( transitionType );
}


/*
========================
idMenuScreen_Shell_Load::LoadDamagedGame
========================
*/
void idMenuScreen_Shell_Load::LoadDamagedGame( int index )
{

	if( index >= sortedSaves.Num() )
	{
		return;
	}

	class idSWFScriptFunction_LoadDamaged : public idSWFScriptFunction_RefCounted
	{
	public:
		idSWFScriptFunction_LoadDamaged( gameDialogMessages_t _msg, bool _accept, int _index, idMenuScreen_Shell_Load* _screen )
		{
			msg = _msg;
			accept = _accept;
			index = _index;
			screen = _screen;
		}
		idSWFScriptVar Call( idSWFScriptObject* thisObject, const idSWFParmList& parms )
		{
			common->Dialog().ClearDialog( msg );
			if( accept )
			{
				screen->DeleteGame( index );
			}
			return idSWFScriptVar();
		}
	private:
		gameDialogMessages_t msg;
		int index;
		bool accept;
		idMenuScreen_Shell_Load* screen;
	};

	idStaticList< idSWFScriptFunction*, 4 > callbacks;
	callbacks.Append( new( TAG_SWF ) idSWFScriptFunction_LoadDamaged( GDM_LOAD_DAMAGED_FILE, true, index, this ) );
	callbacks.Append( new( TAG_SWF ) idSWFScriptFunction_LoadDamaged( GDM_LOAD_DAMAGED_FILE, false, index, this ) );
	idStaticList< idStrId, 4 > optionText;
	optionText.Append( idStrId( "#str_02315" ) );	// DELETE
	optionText.Append( idStrId( "#STR_SWF_CANCEL" ) );

	common->Dialog().AddDynamicDialog( GDM_LOAD_DAMAGED_FILE, callbacks, optionText, false, "" );
}

/*
========================
idMenuScreen_Shell_Load::LoadGame
========================
*/
void idMenuScreen_Shell_Load::LoadGame( int index )
{

	if( menuData == NULL )
	{
		return;
	}

	if( index < GetSortedSaves().Num() && GetSortedSaves()[index].damaged )
	{
		LoadDamagedGame( index );
		return;
	}

	bool isDead = false;
	idPlayer* player = gameLocal.GetLocalPlayer();
	if( player != NULL && player->health <= 0 )
	{
		isDead = true;
	}

	idMenuHandler_Shell* mgr = dynamic_cast< idMenuHandler_Shell* >( menuData );
	if( mgr != NULL && mgr->GetInGame() && !isDead )
	{

		class idSWFScriptFunction_LoadDialog : public idSWFScriptFunction_RefCounted
		{
		public:
			idSWFScriptFunction_LoadDialog( gameDialogMessages_t _msg, bool _accept, const char* _name )
			{
				msg = _msg;
				accept = _accept;
				name = _name;
			}
			idSWFScriptVar Call( idSWFScriptObject* thisObject, const idSWFParmList& parms )
			{
				common->Dialog().ClearDialog( msg );
				if( accept && name != NULL )
				{

					cmdSystem->AppendCommandText( va( "loadgame %s\n", name ) );
				}
				return idSWFScriptVar();
			}
		private:
			gameDialogMessages_t msg;
			bool accept;
			const char* name;
		};

		if( index < sortedSaves.Num() )
		{
			const idStr& name = sortedSaves[ index ].slotName;
			common->Dialog().AddDialog( GDM_SP_LOAD_SAVE, DIALOG_ACCEPT_CANCEL, new idSWFScriptFunction_LoadDialog( GDM_SP_LOAD_SAVE, true, name.c_str() ), new idSWFScriptFunction_LoadDialog( GDM_SP_LOAD_SAVE, false, name.c_str() ), false );
		}

	}
	else
	{
		if( index < sortedSaves.Num() )
		{
			const idStr& name = sortedSaves[ index ].slotName;

			cmdSystem->AppendCommandText( va( "loadgame %s\n", name.c_str() ) );
		}
	}
}

/*
========================
idMenuScreen_Shell_Save::DeleteGame
========================
*/
void idMenuScreen_Shell_Load::DeleteGame( int index )
{

	class idSWFScriptFunction_DeleteGame : public idSWFScriptFunction_RefCounted
	{
	public:
		idSWFScriptFunction_DeleteGame( gameDialogMessages_t _msg, bool _accept, int _index, idMenuScreen_Shell_Load* _screen )
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
		idMenuScreen_Shell_Load* screen;
	};

	common->Dialog().AddDialog( GDM_DELETE_SAVE, DIALOG_ACCEPT_CANCEL, new idSWFScriptFunction_DeleteGame( GDM_DELETE_SAVE, true, index, this ), new idSWFScriptFunction_DeleteGame( GDM_DELETE_SAVE, false, index, this ), false );

}

/*
========================
idMenuScreen_Shell_Load::HandleAction h
========================
*/
bool idMenuScreen_Shell_Load::HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled )
{

	if( menuData != NULL )
	{
		if( menuData->ActiveScreen() != SHELL_AREA_LOAD )
		{
			return false;
		}
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

			int selectionIndex = options->GetViewIndex();
			DeleteGame( selectionIndex );
			return true;
		}
		case WIDGET_ACTION_GO_BACK:
		{
			if( menuData != NULL )
			{
				if( game->IsInGame() )
				{
					menuData->SetNextScreen( SHELL_AREA_ROOT, MENU_TRANSITION_SIMPLE );
				}
				else
				{
					menuData->SetNextScreen( SHELL_AREA_CAMPAIGN, MENU_TRANSITION_SIMPLE );
				}
			}
			return true;
		}
		case WIDGET_ACTION_PRESS_FOCUSED:
		{
			if( options == NULL )
			{
				return true;
			}

			if( sortedSaves.Num() == 0 )
			{
				return true;
			}

			int selectionIndex = options->GetViewIndex();
			if( parms.Num() == 1 )
			{
				selectionIndex = parms[0].ToInteger();

				if( selectionIndex != options->GetFocusIndex() )
				{
					options->SetViewIndex( options->GetViewOffset() + selectionIndex );
					options->SetFocusIndex( selectionIndex );
				}
				else
				{
					LoadGame( options->GetViewOffset() + selectionIndex );
				}
			}
			else
			{
				LoadGame( options->GetViewIndex() );
			}

			return true;
		}
		case WIDGET_ACTION_SCROLL_VERTICAL:
		{
			return true;
		}
	}

	return idMenuWidget::HandleAction( action, event, widget, forceHandled );
}