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
idMenuScreen_PDA_UserEmails::Initialize
========================
*/
void idMenuScreen_PDA_UserEmails::Initialize( idMenuHandler* data )
{
	idMenuScreen::Initialize( data );

	if( data != NULL )
	{
		menuGUI = data->GetGUI();
	}
	SetSpritePath( "menuEmail" );

	pdaInbox.SetSpritePath( GetSpritePath(), "info", "inbox" );
	pdaInbox.Initialize( data );
	pdaInbox.SetNoAutoFree( true );

	emailScrollbar.SetSpritePath( GetSpritePath(), "info", "email", "info", "scrollbar" );
	emailScrollbar.Initialize( data );
	emailScrollbar.SetNoAutoFree( true );

	emailInfo.SetSpritePath( GetSpritePath(), "info", "email" );
	emailInfo.Initialize( data );
	emailInfo.SetScrollbar( &emailScrollbar );
	emailInfo.AddChild( &emailScrollbar );
	emailInfo.RegisterEventObserver( this );
	emailInfo.AddEventAction( WIDGET_EVENT_ROLL_OVER ).Set( WIDGET_ACTION_EMAIL_HOVER, 1 );
	emailInfo.AddEventAction( WIDGET_EVENT_ROLL_OUT ).Set( WIDGET_ACTION_EMAIL_HOVER, 0 );
	emailInfo.SetNoAutoFree( true );

	AddChild( &pdaInbox );
	AddChild( &emailInfo );

	if( pdaInbox.GetEmailList() != NULL )
	{
		pdaInbox.GetEmailList()->RegisterEventObserver( &emailInfo );
		pdaInbox.GetEmailList()->RegisterEventObserver( &emailScrollbar );

		for( int i = 0; i < pdaInbox.GetEmailList()->GetChildren().Num(); ++i )
		{
			idMenuWidget& child = pdaInbox.GetEmailList()->GetChildByIndex( i );
			idMenuWidget_Button* const button = dynamic_cast< idMenuWidget_Button* >( &child );
			if( button != NULL )
			{
				button->RegisterEventObserver( &emailInfo );
			}
		}

	}

	AddEventAction( WIDGET_EVENT_SCROLL_DOWN_RSTICK ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_SCROLL_DOWN_START_REPEATER, WIDGET_EVENT_SCROLL_DOWN_RSTICK ) );
	AddEventAction( WIDGET_EVENT_SCROLL_UP_RSTICK ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_SCROLL_UP_START_REPEATER, WIDGET_EVENT_SCROLL_UP_RSTICK ) );
	AddEventAction( WIDGET_EVENT_SCROLL_DOWN_RSTICK_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_DOWN_RSTICK_RELEASE ) );
	AddEventAction( WIDGET_EVENT_SCROLL_UP_RSTICK_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_UP_RSTICK_RELEASE ) );

	AddEventAction( WIDGET_EVENT_SCROLL_DOWN ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_SCROLL_DOWN_START_REPEATER, WIDGET_EVENT_SCROLL_DOWN ) );
	AddEventAction( WIDGET_EVENT_SCROLL_UP ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_SCROLL_UP_START_REPEATER, WIDGET_EVENT_SCROLL_UP ) );
	AddEventAction( WIDGET_EVENT_SCROLL_DOWN_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_DOWN_RELEASE ) );
	AddEventAction( WIDGET_EVENT_SCROLL_UP_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_UP_RELEASE ) );

	AddEventAction( WIDGET_EVENT_SCROLL_DOWN_LSTICK ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_SCROLL_DOWN_START_REPEATER, WIDGET_EVENT_SCROLL_DOWN_LSTICK ) );
	AddEventAction( WIDGET_EVENT_SCROLL_UP_LSTICK ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_SCROLL_UP_START_REPEATER, WIDGET_EVENT_SCROLL_UP_LSTICK ) );
	AddEventAction( WIDGET_EVENT_SCROLL_DOWN_LSTICK_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_DOWN_LSTICK_RELEASE ) );
	AddEventAction( WIDGET_EVENT_SCROLL_UP_LSTICK_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_UP_LSTICK_RELEASE ) );

	AddEventAction( WIDGET_EVENT_TAB_NEXT ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_TAB_NEXT, WIDGET_EVENT_TAB_NEXT ) );
	AddEventAction( WIDGET_EVENT_TAB_PREV ).Set( new( TAG_SWF ) idWidgetActionHandler( this, WIDGET_ACTION_EVENT_TAB_PREV, WIDGET_EVENT_TAB_PREV ) );

	class idInfoBoxRefresh : public idSWFScriptFunction_RefCounted
	{
	public:
		idInfoBoxRefresh( idMenuWidget_InfoBox* _widget ) :
			widget( _widget )
		{
		}

		idSWFScriptVar Call( idSWFScriptObject* thisObject, const idSWFParmList& parms )
		{

			if( widget == NULL )
			{
				return idSWFScriptVar();
			}

			widget->Update();
			return idSWFScriptVar();
		}
	private:
		idMenuWidget_InfoBox* widget;
	};

	if( GetSWFObject() != NULL )
	{
		GetSWFObject()->SetGlobal( "refreshInfoBox", new( TAG_SWF ) idInfoBoxRefresh( &emailInfo ) );
	}
}

/*
========================
idMenuScreen_PDA_UserEmails::ShowScreen
========================
*/
void idMenuScreen_PDA_UserEmails::ShowScreen( const mainMenuTransition_t transitionType )
{

	if( menuGUI != NULL )
	{
		readingEmails = false;

		idSWFScriptObject& root = menuGUI->GetRootObject();
		idSWFSpriteInstance* pdaSprite = root.GetNestedSprite( "pda_persons" );
		if( pdaSprite != NULL && menuData != NULL && menuData->ActiveScreen() != PDA_AREA_USER_DATA )
		{
			pdaSprite->SetVisible( true );
			pdaSprite->PlayFrame( "rollOn" );
		}

		menuGUI->SetGlobal( "emailRollback", false );
		if( pdaInbox.BindSprite( root ) && pdaInbox.GetSprite() )
		{
			pdaInbox.GetSprite()->StopFrame( 1 );
		}
		if( emailInfo.BindSprite( root ) && emailInfo.GetSprite() )
		{
			emailInfo.GetSprite()->StopFrame( 1 );
		}
	}

	scrollEmailInfo = false;

	idMenuScreen::ShowScreen( transitionType );
}

/*
========================
idMenuScreen_PDA_UserEmails::Update
========================
*/
void idMenuScreen_PDA_UserEmails::Update()
{

	if( menuData != NULL )
	{

		if( menuData->NextScreen() != PDA_AREA_USER_EMAIL )
		{
			return;
		}

		idMenuWidget_CommandBar* cmdBar = dynamic_cast< idMenuWidget_CommandBar* const >( menuData->GetChildFromIndex( PDA_WIDGET_CMD_BAR ) );
		if( cmdBar != NULL )
		{
			cmdBar->ClearAllButtons();
			idMenuWidget_CommandBar::buttonInfo_t* buttonInfo;
			if( readingEmails )
			{
				buttonInfo = cmdBar->GetButton( idMenuWidget_CommandBar::BUTTON_JOY2 );
				if( menuData->GetPlatform() != 2 )
				{
					buttonInfo->label = "#str_00395";
				}
				buttonInfo->action.Set( WIDGET_ACTION_GO_BACK );

			}
			else
			{
				buttonInfo = cmdBar->GetButton( idMenuWidget_CommandBar::BUTTON_JOY2 );
				if( menuData->GetPlatform() != 2 )
				{
					buttonInfo->label = "#str_01345";
				}
				buttonInfo->action.Set( WIDGET_ACTION_GO_BACK );

				idMenuWidget_DynamicList* pdaList = dynamic_cast< idMenuWidget_DynamicList* >( menuData->GetChildFromIndex( PDA_WIDGET_PDA_LIST ) );
				if( pdaList != NULL )
				{
					int pdaIndex = pdaList->GetViewIndex();
					idPlayer* player = gameLocal.GetLocalPlayer();
					if( player != NULL )
					{
						if( pdaIndex < player->GetInventory().pdas.Num() )
						{
							const idDeclPDA* pda = player->GetInventory().pdas[ pdaIndex ];
							if( pda != NULL && pdaInbox.GetEmailList() != NULL )
							{
								idStr pdaFullName = pda->GetFullName();
								int emailIndex = pdaInbox.GetEmailList()->GetViewIndex();
								if( emailIndex < pda->GetNumEmails() )
								{
									buttonInfo = cmdBar->GetButton( idMenuWidget_CommandBar::BUTTON_JOY1 );
									if( menuData->GetPlatform() != 2 )
									{
										buttonInfo->label = "#str_01102";
									}
									buttonInfo->action.Set( WIDGET_ACTION_PDA_SELECT_EMAIL );
								}
							}
						}
					}
				}
			}

			buttonInfo = cmdBar->GetButton( idMenuWidget_CommandBar::BUTTON_TAB );
			buttonInfo->label = "";
			buttonInfo->action.Set( WIDGET_ACTION_PDA_CLOSE );
		}
	}

	UpdateEmail();
	idMenuScreen::Update();

}

/*
========================
idMenuScreen_PDA_UserEmails::HideScreen
========================
*/
void idMenuScreen_PDA_UserEmails::HideScreen( const mainMenuTransition_t transitionType )
{

	if( menuGUI != NULL )
	{
		idSWFScriptObject& root = menuGUI->GetRootObject();
		idSWFSpriteInstance* pdaSprite = root.GetNestedSprite( "pda_persons" );
		if( pdaSprite != NULL && menuData != NULL )
		{
			if( menuData->NextScreen() != PDA_AREA_USER_DATA )
			{
				pdaSprite->SetVisible( true );
				pdaSprite->PlayFrame( "rollOff" );
				readingEmails = false;
			}
			else
			{
				if( readingEmails )
				{
					readingEmails = false;
					pdaSprite->SetVisible( true );
					pdaSprite->PlayFrame( "rollOn" );
				}
			}
		}
	}

	idMenuScreen::HideScreen( transitionType );
}

/*
========================
idMenuScreen_PDA_UserEmails::UpdateEmail
========================
*/
void idMenuScreen_PDA_UserEmails::UpdateEmail()
{
	idMenuWidget_DynamicList* pdaList = dynamic_cast< idMenuWidget_DynamicList* >( menuData->GetChildFromIndex( PDA_WIDGET_PDA_LIST ) );
	if( pdaList != NULL )
	{

		int pdaIndex = pdaList->GetViewIndex();

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
		if( pda != NULL && pdaInbox.GetEmailList() != NULL )
		{
			idStr pdaFullName = pda->GetFullName();
			int emailIndex = pdaInbox.GetEmailList()->GetViewIndex();
			if( emailIndex < pda->GetNumEmails() )
			{
				const idDeclEmail* email = pda->GetEmailByIndex( emailIndex );
				if( email != NULL )
				{
					emailInfo.SetHeading( email->GetSubject() );
					emailInfo.SetBody( email->GetBody() );
					emailInfo.Update();
				}
			}
		}
	}
}

/*
========================
idMenuScreen_PDA_UserEmails::HandleAction
========================
*/
bool idMenuScreen_PDA_UserEmails::ScrollCorrectList( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget )
{

	bool handled = false;
	bool leftScroll = false;
	if( event.type == WIDGET_EVENT_SCROLL_UP_LSTICK || event.type == WIDGET_EVENT_SCROLL_DOWN_LSTICK ||
			event.type == WIDGET_EVENT_SCROLL_UP || event.type == WIDGET_EVENT_SCROLL_DOWN )
	{
		leftScroll = true;
	}

	if( readingEmails )
	{
		if( leftScroll && !scrollEmailInfo )
		{
			idMenuWidget_DynamicList* inbox = pdaInbox.GetEmailList();
			if( inbox != NULL )
			{
				inbox->HandleAction( action, event, inbox );
				UpdateEmail();
				handled = true;
			}
		}
		else
		{
			emailInfo.HandleAction( action, event, &emailInfo );
			handled = true;
		}
	}
	else if( !leftScroll )
	{
		idMenuWidget_DynamicList* inbox = pdaInbox.GetEmailList();
		if( inbox != NULL )
		{
			inbox->HandleAction( action, event, inbox );
			UpdateEmail();
			handled = true;
		}
	}
	else if( menuData != NULL )
	{
		idMenuWidget_DynamicList* pdaList = dynamic_cast< idMenuWidget_DynamicList* const >( menuData->GetChildFromIndex( PDA_WIDGET_PDA_LIST ) );
		if( pdaList != NULL )
		{
			pdaList->HandleAction( action, event, pdaList );
			handled = true;
		}
	}

	return handled;
}

/*
========================
idMenuScreen_PDA_UserEmails::HandleAction
========================
*/
void idMenuScreen_PDA_UserEmails::ShowEmail( bool show )
{

	idSWFSpriteInstance* pdaSprite = NULL;

	if( menuGUI != NULL )
	{
		idSWFScriptObject& root = menuGUI->GetRootObject();
		pdaSprite = root.GetNestedSprite( "pda_persons" );

		if( show && !readingEmails )
		{

			scrollEmailInfo = false;

			if( pdaSprite != NULL )
			{
				pdaSprite->SetVisible( true );
				pdaSprite->PlayFrame( "rollOff" );
			}

			if( emailInfo.BindSprite( root ) && emailInfo.GetSprite() != NULL )
			{
				emailInfo.GetSprite()->PlayFrame( "rollOn" );
				emailInfo.Update();
			}

			if( pdaInbox.BindSprite( root ) && pdaInbox.GetSprite() != NULL )
			{
				pdaInbox.GetSprite()->PlayFrame( "rollOff" );
			}
		}
		else if( !show && readingEmails )
		{

			if( emailInfo.BindSprite( root ) && emailInfo.GetSprite() != NULL )
			{
				emailInfo.GetSprite()->PlayFrame( "rollOff" );
			}

			if( pdaInbox.BindSprite( root ) && pdaInbox.GetSprite() != NULL )
			{
				pdaInbox.GetSprite()->PlayFrame( "rollOn" );
			}

			if( pdaSprite != NULL )
			{
				pdaSprite->SetVisible( true );
				pdaSprite->PlayFrame( "rollOn" );

				if( menuData != NULL )
				{
					idMenuWidget_DynamicList* pdaList = dynamic_cast< idMenuWidget_DynamicList* const >( menuData->GetChildFromIndex( PDA_WIDGET_PDA_LIST ) );
					if( pdaList != NULL )
					{
						pdaList->SetFocusIndex( pdaList->GetFocusIndex() );
					}
				}
			}
		}
	}


	readingEmails = show;
	Update();
}

/*
========================
idMenuScreen_PDA_UserEmails::HandleAction
========================
*/
bool idMenuScreen_PDA_UserEmails::HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled )
{

	if( menuData == NULL )
	{
		return true;
	}

	if( menuData->ActiveScreen() != PDA_AREA_USER_EMAIL )
	{
		return false;
	}

	widgetAction_t actionType = action.GetType();
	const idSWFParmList& parms = action.GetParms();

	switch( actionType )
	{
		case WIDGET_ACTION_PDA_CLOSE:
		{
			menuData->SetNextScreen( PDA_AREA_INVALID, MENU_TRANSITION_ADVANCE );
			return true;
		}
		case WIDGET_ACTION_GO_BACK:
		{
			if( readingEmails )
			{
				ShowEmail( false );
			}
			else
			{
				menuData->SetNextScreen( PDA_AREA_INVALID, MENU_TRANSITION_ADVANCE );
			}
			return true;
		}
		case WIDGET_ACTION_REFRESH:
		{
			UpdateEmail();
			return true;
		}
		case WIDGET_ACTION_PDA_SELECT_EMAIL:
		{

			if( widget->GetParent() != NULL )
			{
				idMenuWidget_DynamicList* emailList = dynamic_cast< idMenuWidget_DynamicList* >( widget->GetParent() );
				int index = parms[0].ToInteger();
				if( emailList != NULL )
				{
					emailList->SetViewIndex( emailList->GetViewOffset() + index );
					emailList->SetFocusIndex( index );
				}
			}

			ShowEmail( true );

			return true;
		}
		case WIDGET_ACTION_EMAIL_HOVER:
		{
			scrollEmailInfo = parms[0].ToBool();
			return true;
		}
		case WIDGET_ACTION_SCROLL_VERTICAL:
		{
			if( ScrollCorrectList( action, event, widget ) )
			{
				return true;
			}
			UpdateEmail();
			break;
		}
	}

	return idMenuWidget::HandleAction( action, event, widget, forceHandled );
}

/*
========================
idMenuScreen_PDA_UserEmails::ObserveEvent
========================
*/
void idMenuScreen_PDA_UserEmails::ObserveEvent( const idMenuWidget& widget, const idWidgetEvent& event )
{

	if( menuData != NULL && menuData->ActiveScreen() != PDA_AREA_USER_EMAIL )
	{
		return;
	}

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
			Update();
			break;
		}
	}
}
