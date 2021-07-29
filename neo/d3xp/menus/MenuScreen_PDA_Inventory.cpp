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

static const int NUM_INVENTORY_ITEMS_VISIBLE = 9;

/*
========================
idMenuScreen_PDA_Inventory::Initialize
========================
*/
void idMenuScreen_PDA_Inventory::Initialize( idMenuHandler* data )
{

	AddEventAction( WIDGET_EVENT_TAB_NEXT ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_TAB_NEXT, WIDGET_EVENT_TAB_NEXT ) );
	AddEventAction( WIDGET_EVENT_TAB_PREV ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_TAB_PREV, WIDGET_EVENT_TAB_PREV ) );

	if( data != NULL )
	{
		menuGUI = data->GetGUI();
	}
	SetSpritePath( "menuItems" );

	if( menuGUI != NULL )
	{
		idSWFScriptObject& root = menuGUI->GetRootObject();
		BindSprite( root );
	}

	infoBox.SetSpritePath( GetSpritePath(), "info", "details" );
	infoBox.Initialize( data );
	infoBox.SetNoAutoFree( true );

	itemList.SetSpritePath( GetSpritePath(), "info", "options" );
	itemList.SetNumVisibleOptions( NUM_INVENTORY_ITEMS_VISIBLE );
	itemList.SetNoAutoFree( true );
	while( itemList.GetChildren().Num() < NUM_INVENTORY_ITEMS_VISIBLE )
	{
		idMenuWidget_Button* const buttonWidget = new( TAG_SWF ) idMenuWidget_Button();
		buttonWidget->Initialize( data );
		buttonWidget->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_SELECT_PDA_ITEM, itemList.GetChildren().Num() );
		itemList.AddChild( buttonWidget );
	}
	itemList.Initialize( data );

	AddChild( &itemList );
	AddChild( &infoBox );
	//AddChild( assignment );

	AddEventAction( WIDGET_EVENT_SCROLL_LEFT ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_SCROLL_LEFT_START_REPEATER, WIDGET_EVENT_SCROLL_LEFT ) );
	AddEventAction( WIDGET_EVENT_SCROLL_RIGHT ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_SCROLL_RIGHT_START_REPEATER, WIDGET_EVENT_SCROLL_RIGHT ) );
	AddEventAction( WIDGET_EVENT_SCROLL_LEFT_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_LEFT_RELEASE ) );
	AddEventAction( WIDGET_EVENT_SCROLL_RIGHT_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_RIGHT_RELEASE ) );
	AddEventAction( WIDGET_EVENT_SCROLL_LEFT_LSTICK ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_SCROLL_LEFT_START_REPEATER, WIDGET_EVENT_SCROLL_LEFT_LSTICK ) );
	AddEventAction( WIDGET_EVENT_SCROLL_RIGHT_LSTICK ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_SCROLL_RIGHT_START_REPEATER, WIDGET_EVENT_SCROLL_RIGHT_LSTICK ) );
	AddEventAction( WIDGET_EVENT_SCROLL_LEFT_LSTICK_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_LEFT_LSTICK_RELEASE ) );
	AddEventAction( WIDGET_EVENT_SCROLL_RIGHT_LSTICK_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_RIGHT_LSTICK_RELEASE ) );


	idMenuScreen::Initialize( data );
}

/*
========================
idMenuScreen_PDA_Inventory::ShowScreen
========================
*/
void idMenuScreen_PDA_Inventory::ShowScreen( const mainMenuTransition_t transitionType )
{
	idPlayer* player = gameLocal.GetLocalPlayer();
	if( player != NULL )
	{

		int numItems = player->GetInventory().items.Num();
		for( int j = 0; j < numItems; j++ )
		{
			idDict* item = player->GetInventory().items[j];
			if( !item->GetBool( "inv_pda" ) )
			{
				const char* iname = item->GetString( "inv_name" );
				const char* iicon = item->GetString( "inv_icon" );
				const char* itext = item->GetString( "inv_text" );
				iname = iname;
				iicon = iicon;
				itext = itext;
				const idKeyValue* kv = item->MatchPrefix( "inv_id", NULL );
				if( kv )
				{
					//objectiveSystem->SetStateString( va( "inv_id_%i", j ), kv->GetValue() );
				}
			}
		}

		idList<const idMaterial*> weaponIcons;
		for( int j = 0; j < MAX_WEAPONS; j++ )
		{

			const char* weap = GetWeaponName( j );
			if( weap == NULL || *weap == '\0' )
			{
				continue;
			}

			if( !IsVisibleWeapon( j ) )
			{
				continue;
			}

			const idDeclEntityDef* weaponDef = gameLocal.FindEntityDef( weap, false );
			if( weaponDef != NULL )
			{
				weaponIcons.Append( declManager->FindMaterial( weaponDef->dict.GetString( "hudIcon" ), false ) );
			}
		}

		itemList.SetListImages( weaponIcons );
		itemList.SetViewIndex( 0 );
		itemList.SetMoveToIndex( 0 );
		itemList.SetMoveDiff( 0 );

	}

	idMenuScreen::ShowScreen( transitionType );
}

/*
========================
idMenuScreen_PDA_Inventory::HideScreen
========================
*/
void idMenuScreen_PDA_Inventory::HideScreen( const mainMenuTransition_t transitionType )
{
	idMenuScreen::HideScreen( transitionType );
}

/*
========================
idMenuScreen_PDA_Inventory::GetWeaponName
========================
*/
const char* idMenuScreen_PDA_Inventory::GetWeaponName( int index )
{

	idPlayer* player = gameLocal.GetLocalPlayer();
	if( player == NULL )
	{
		return NULL;
	}

	const char* weaponDefName = va( "def_weapon%d", index );
	if( player->GetInventory().weapons & ( 1 << index ) )
	{
		return player->spawnArgs.GetString( weaponDefName );
	}

	return NULL;
}

/*
========================
idMenuScreen_PDA_Inventory::GetWeaponName
========================
*/
bool idMenuScreen_PDA_Inventory::IsVisibleWeapon( int index )
{

	idPlayer* player = gameLocal.GetLocalPlayer();
	if( player == NULL )
	{
		return false;
	}

	if( player->GetInventory().weapons & ( 1 << index ) )
	{
		return player->spawnArgs.GetBool( va( "weapon%d_visible", index ) );
	}

	return false;

}

/*
========================
idMenuScreen_PDA_Inventory::Update
========================
*/
void idMenuScreen_PDA_Inventory::Update()
{

	idPlayer* player = gameLocal.GetLocalPlayer();
	if( player == NULL )
	{
		idMenuScreen::Update();
		return;
	}

	int validIndex = 0;
	for( int j = 0; j < MAX_WEAPONS; j++ )
	{

		const char* weap = GetWeaponName( j );
		if( weap == NULL || *weap == '\0' )
		{
			continue;
		}

		if( !IsVisibleWeapon( j ) )
		{
			return;
		}

		const idDeclEntityDef* weaponDef = gameLocal.FindEntityDef( weap, false );
		if( weaponDef == NULL )
		{
			continue;
		}

		if( validIndex == itemList.GetMoveToIndex() )
		{
			idStr itemName = weaponDef->dict.GetString( "display_name" );
			idStr itemDesc = weaponDef->dict.GetString( "inv_desc" );
			infoBox.SetHeading( idLocalization::GetString( itemName.c_str() ) );
			infoBox.SetBody( idLocalization::GetString( itemDesc.c_str() ) );
			break;
		}
		validIndex++;
	}

	if( GetSprite() != NULL )
	{
		idSWFSpriteInstance* dpad = GetSprite()->GetScriptObject()->GetNestedSprite( "info", "dpad" );
		if( dpad != NULL )
		{
			dpad->SetVisible( false );
		}
	}

	if( menuData != NULL )
	{
		idMenuWidget_CommandBar* cmdBar = dynamic_cast< idMenuWidget_CommandBar* const >( menuData->GetChildFromIndex( PDA_WIDGET_CMD_BAR ) );
		if( cmdBar != NULL )
		{
			cmdBar->ClearAllButtons();
			idMenuWidget_CommandBar::buttonInfo_t* buttonInfo;
			buttonInfo = cmdBar->GetButton( idMenuWidget_CommandBar::BUTTON_JOY2 );
			if( menuData->GetPlatform() != 2 )
			{
				buttonInfo->label = "#str_01345";
			}
			buttonInfo->action.Set( WIDGET_ACTION_GO_BACK );

			buttonInfo = cmdBar->GetButton( idMenuWidget_CommandBar::BUTTON_JOY3 );
			buttonInfo->label = "#str_SWF_EQUIP";
			buttonInfo->action.Set( WIDGET_ACTION_JOY3_ON_PRESS );

			buttonInfo = cmdBar->GetButton( idMenuWidget_CommandBar::BUTTON_TAB );
			buttonInfo->label = "";
			buttonInfo->action.Set( WIDGET_ACTION_GO_BACK );
		}
	}

	idMenuScreen::Update();
}

/*
========================
idMenuScreen_PDA_Inventory::EquipWeapon
========================
*/
void idMenuScreen_PDA_Inventory::EquipWeapon()
{

	if( itemList.GetViewIndex() != itemList.GetMoveToIndex() )
	{
		return;
	}

	idPlayer* player = gameLocal.GetLocalPlayer();
	if( player == NULL )
	{
		return;
	}

	int validIndex = 0;
	for( int j = 0; j < MAX_WEAPONS; j++ )
	{

		const char* weap = GetWeaponName( j );
		if( weap == NULL || *weap == '\0' )
		{
			continue;
		}

		if( !IsVisibleWeapon( j ) )
		{
			continue;
		}

		if( validIndex == itemList.GetMoveToIndex() )
		{
			int slot = player->SlotForWeapon( weap );
			player->SetPreviousWeapon( slot );
			break;
		}
		validIndex++;
	}

	player->TogglePDA();

}

/*
========================
idMenuScreen_PDA_Inventory::HandleAction
========================
*/
bool idMenuScreen_PDA_Inventory::HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled )
{

	if( menuData == NULL )
	{
		return true;
	}

	if( menuData->ActiveScreen() != PDA_AREA_INVENTORY )
	{
		return false;
	}

	widgetAction_t actionType = action.GetType();
	const idSWFParmList& parms = action.GetParms();

	switch( actionType )
	{
		case WIDGET_ACTION_JOY3_ON_PRESS:
		{
			EquipWeapon();
			return true;
		}
		case WIDGET_ACTION_GO_BACK:
		{
			menuData->SetNextScreen( PDA_AREA_INVALID, MENU_TRANSITION_ADVANCE );
			return true;
		}
		case WIDGET_ACTION_START_REPEATER:
		{
			idWidgetAction repeatAction;
			widgetAction_t repeatActionType = static_cast< widgetAction_t >( parms[ 0 ].ToInteger() );
			assert( parms.Num() == 2 );
			repeatAction.Set( repeatActionType, parms[ 1 ] );
			menuData->StartWidgetActionRepeater( widget, repeatAction, event );
			return true;
		}
		case WIDGET_ACTION_SELECT_PDA_ITEM:
		{

			if( itemList.GetMoveDiff() > 0 )
			{
				itemList.MoveToIndex( itemList.GetMoveToIndex(), true );
			}

			int index = parms[0].ToInteger();
			if( index != 0 )
			{
				itemList.MoveToIndex( index );
				Update();
			}

			return true;
		}
		case WIDGET_ACTION_STOP_REPEATER:
		{
			menuData->ClearWidgetActionRepeater();
			return true;
		}
		case WIDGET_ACTION_SCROLL_HORIZONTAL:
		{

			if( itemList.GetTotalNumberOfOptions() <= 1 )
			{
				return true;
			}

			if( itemList.GetMoveDiff() > 0 )
			{
				itemList.MoveToIndex( itemList.GetMoveToIndex(), true );
			}

			int direction = parms[0].ToInteger();
			if( direction == 1 )
			{
				if( itemList.GetViewIndex() == itemList.GetTotalNumberOfOptions() - 1 )
				{
					return true;
				}
				else
				{
					itemList.MoveToIndex( 1 );
				}
			}
			else
			{
				if( itemList.GetViewIndex() == 0 )
				{
					return true;
				}
				else
				{
					itemList.MoveToIndex( ( itemList.GetNumVisibleOptions() / 2 ) + 1 );
				}
			}
			Update();

			return true;
		}
	}

	return idMenuWidget::HandleAction( action, event, widget, forceHandled );
}
