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

static const int MAX_PDA_ITEMS = 15;
static const int MAX_NAV_OPTIONS = 4;

/*
========================
idMenuHandler_PDA::Update
========================
*/
void idMenuHandler_PDA::Update()
{

	if( gui == NULL || !gui->IsActive() )
	{
		return;
	}

	if( activeScreen != nextScreen )
	{

		if( nextScreen == PDA_AREA_INVALID )
		{
			menuScreens[ activeScreen ]->HideScreen( static_cast<mainMenuTransition_t>( transition ) );

			idMenuWidget_CommandBar* cmdBar = dynamic_cast< idMenuWidget_CommandBar* >( GetChildFromIndex( PDA_WIDGET_CMD_BAR ) );
			if( cmdBar != NULL )
			{
				cmdBar->ClearAllButtons();
				cmdBar->Update();
			}

			idSWFSpriteInstance* menu = gui->GetRootObject().GetNestedSprite( "navBar" );
			idSWFSpriteInstance* bg = gui->GetRootObject().GetNestedSprite( "background" );
			idSWFSpriteInstance* edging = gui->GetRootObject().GetNestedSprite( "_fullScreen" );

			if( menu != NULL )
			{
				menu->PlayFrame( "rollOff" );
			}

			if( bg != NULL )
			{
				bg->PlayFrame( "rollOff" );
			}

			if( edging != NULL )
			{
				edging->PlayFrame( "rollOff" );
			}

		}
		else
		{
			if( activeScreen > PDA_AREA_INVALID && activeScreen < PDA_NUM_AREAS && menuScreens[ activeScreen ] != NULL )
			{
				menuScreens[ activeScreen ]->HideScreen( static_cast<mainMenuTransition_t>( transition ) );
			}

			if( nextScreen > PDA_AREA_INVALID && nextScreen < PDA_NUM_AREAS && menuScreens[ nextScreen ] != NULL )
			{
				menuScreens[ nextScreen ]->UpdateCmds();
				menuScreens[ nextScreen ]->ShowScreen( static_cast<mainMenuTransition_t>( transition ) );
			}
		}

		transition = MENU_TRANSITION_INVALID;
		activeScreen = nextScreen;
	}

	idPlayer* player = gameLocal.GetLocalPlayer();
	if( player != NULL )
	{
		if( activeScreen == PDA_AREA_USER_DATA )
		{
			bool isPlaying = player->IsSoundChannelPlaying( SND_CHANNEL_PDA_AUDIO );
			UpdateAudioLogPlaying( isPlaying );
		}

		if( activeScreen == PDA_AREA_VIDEO_DISKS )
		{
			bool isPlaying = player->IsSoundChannelPlaying( SND_CHANNEL_PDA_VIDEO );
			UdpateVideoPlaying( isPlaying );
		}
	}

	idMenuHandler::Update();
}

/*
================================================
idMenuHandler::TriggerMenu
================================================
*/
void idMenuHandler_PDA::TriggerMenu()
{
	nextScreen = PDA_AREA_USER_DATA;
	transition = MENU_TRANSITION_FORCE;
}

/*
========================
idMenuHandler_PDA::ActivateMenu
========================
*/
void idMenuHandler_PDA::ActivateMenu( bool show )
{
	idMenuHandler::ActivateMenu( show );

	if( show )
	{
		// Add names to pda
		idPlayer* player = gameLocal.GetLocalPlayer();
		if( player == NULL )
		{
			return;
		}

		pdaNames.Clear();
		for( int j = 0; j < player->GetInventory().pdas.Num(); j++ )
		{
			const idDeclPDA* pda = player->GetInventory().pdas[ j ];
			idList< idStr > names;
			names.Append( pda->GetPdaName() );
			pdaNames.Append( names );
		}
		idMenuWidget_DynamicList* pdaList = dynamic_cast< idMenuWidget_DynamicList* >( GetChildFromIndex( PDA_WIDGET_PDA_LIST ) );
		if( pdaList != NULL )
		{
			pdaList->SetListData( pdaNames );
		}

		navOptions.Clear();
		navOptions.Append( idLocalization::GetString( "#str_04190" ) );
		navOptions.Append( idLocalization::GetString( "#str_01442" ) );
		navOptions.Append( idLocalization::GetString( "#str_01440" ) );
		navOptions.Append( idLocalization::GetString( "#str_01414" ) );
		idMenuWidget_NavBar* navBar = dynamic_cast< idMenuWidget_NavBar* >( GetChildFromIndex( PDA_WIDGET_NAV_BAR ) );
		if( navBar != NULL )
		{
			navBar->SetListHeadings( navOptions );
			navBar->SetFocusIndex( 0 );
			navBar->Update();
		}

		idMenuWidget_CommandBar* cmdBar = dynamic_cast< idMenuWidget_CommandBar* >( GetChildFromIndex( PDA_WIDGET_CMD_BAR ) );
		if( cmdBar != NULL )
		{
			cmdBar->ClearAllButtons();
			cmdBar->Update();
		}

	}
	else
	{
		nextScreen = PDA_AREA_INVALID;
		activeScreen = PDA_AREA_INVALID;
	}

}

/*
========================
idMenuHandler_PDA::Initialize
========================
*/
void idMenuHandler_PDA::Initialize( const char* swfFile, idSoundWorld* sw )
{
	idMenuHandler::Initialize( swfFile, sw );

	//---------------------
	// Initialize the menus
	//---------------------
#define BIND_PDA_SCREEN( screenId, className, menuHandler )				\
	menuScreens[ (screenId) ] = new (TAG_SWF) className();	\
	menuScreens[ (screenId) ]->Initialize( menuHandler ); \
	menuScreens[ (screenId) ]->AddRef(); \
	menuScreens[ (screenId) ]->SetNoAutoFree( true );

	for( int i = 0; i < PDA_NUM_AREAS; ++i )
	{
		menuScreens[ i ] = NULL;
	}

	BIND_PDA_SCREEN( PDA_AREA_USER_DATA, idMenuScreen_PDA_UserData, this );
	BIND_PDA_SCREEN( PDA_AREA_USER_EMAIL, idMenuScreen_PDA_UserEmails, this );
	BIND_PDA_SCREEN( PDA_AREA_VIDEO_DISKS, idMenuScreen_PDA_VideoDisks, this );
	BIND_PDA_SCREEN( PDA_AREA_INVENTORY, idMenuScreen_PDA_Inventory, this );


	pdaScrollBar.SetSpritePath( "pda_persons", "info", "scrollbar" );
	pdaScrollBar.Initialize( this );
	pdaScrollBar.SetNoAutoFree( true );

	pdaList.SetSpritePath( "pda_persons", "info", "list" );
	pdaList.SetNumVisibleOptions( MAX_PDA_ITEMS );
	pdaList.SetWrappingAllowed( true );
	pdaList.SetNoAutoFree( true );

	while( pdaList.GetChildren().Num() < MAX_PDA_ITEMS )
	{
		idMenuWidget_Button* const buttonWidget = new( TAG_SWF ) idMenuWidget_Button();
		buttonWidget->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_PDA_SELECT_USER, pdaList.GetChildren().Num() );
		buttonWidget->Initialize( this );
		if( menuScreens[ PDA_AREA_USER_DATA ] != NULL )
		{
			idMenuScreen_PDA_UserData* userDataScreen = dynamic_cast< idMenuScreen_PDA_UserData* >( menuScreens[ PDA_AREA_USER_DATA ] );
			if( userDataScreen != NULL )
			{
				buttonWidget->RegisterEventObserver( userDataScreen->GetUserData() );
				buttonWidget->RegisterEventObserver( userDataScreen->GetObjective() );
				buttonWidget->RegisterEventObserver( userDataScreen->GetAudioFiles() );
			}
		}
		if( menuScreens[ PDA_AREA_USER_EMAIL ] != NULL )
		{
			idMenuScreen_PDA_UserEmails* userEmailScreen = dynamic_cast< idMenuScreen_PDA_UserEmails* >( menuScreens[ PDA_AREA_USER_EMAIL ] );
			if( userEmailScreen != NULL )
			{
				buttonWidget->RegisterEventObserver( &userEmailScreen->GetInbox() );
				buttonWidget->RegisterEventObserver( userEmailScreen );
			}
		}
		buttonWidget->RegisterEventObserver( &pdaScrollBar );
		pdaList.AddChild( buttonWidget );
	}
	pdaList.AddChild( &pdaScrollBar );
	pdaList.Initialize( this );

	navBar.SetSpritePath( "navBar", "options" );
	navBar.Initialize( this );
	navBar.SetNumVisibleOptions( MAX_NAV_OPTIONS );
	navBar.SetWrappingAllowed( true );
	navBar.SetButtonSpacing( 20.0f, 25.0f, 75.0f );
	navBar.SetInitialXPos( 40.0f );
	navBar.SetNoAutoFree( true );
	for( int count = 0; count < ( MAX_NAV_OPTIONS * 2 - 1 ); ++count )
	{
		idMenuWidget_NavButton* const navButton = new( TAG_SWF ) idMenuWidget_NavButton();

		if( count < MAX_NAV_OPTIONS - 1 )
		{
			navButton->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_PDA_SELECT_NAV, count );
		}
		else if( count < ( ( MAX_NAV_OPTIONS - 1 ) * 2 ) )
		{
			int index = ( count - ( MAX_NAV_OPTIONS - 1 ) ) + 1;
			navButton->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_PDA_SELECT_NAV, index );
		}
		else
		{
			navButton->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_PDA_SELECT_NAV, -1 );
		}

		navBar.AddChild( navButton );
	}

	//
	// command bar
	//
	commandBarWidget.SetAlignment( idMenuWidget_CommandBar::LEFT );
	commandBarWidget.SetSpritePath( "prompts" );
	commandBarWidget.Initialize( this );
	commandBarWidget.SetNoAutoFree( true );

	AddChild( &navBar );
	AddChild( &pdaList );
	AddChild( &pdaScrollBar );
	AddChild( &commandBarWidget );

	pdaList.AddEventAction( WIDGET_EVENT_SCROLL_DOWN_LSTICK ).Set( new( TAG_SWF ) idWidgetActionHandler( &pdaList, WIDGET_ACTION_EVENT_SCROLL_DOWN_START_REPEATER, WIDGET_EVENT_SCROLL_DOWN_LSTICK ) );
	pdaList.AddEventAction( WIDGET_EVENT_SCROLL_UP_LSTICK ).Set( new( TAG_SWF ) idWidgetActionHandler( &pdaList, WIDGET_ACTION_EVENT_SCROLL_UP_START_REPEATER, WIDGET_EVENT_SCROLL_UP_LSTICK ) );
	pdaList.AddEventAction( WIDGET_EVENT_SCROLL_DOWN_LSTICK_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( &pdaList, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_DOWN_LSTICK_RELEASE ) );
	pdaList.AddEventAction( WIDGET_EVENT_SCROLL_UP_LSTICK_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( &pdaList, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_UP_LSTICK_RELEASE ) );
	pdaList.AddEventAction( WIDGET_EVENT_SCROLL_DOWN ).Set( new( TAG_SWF ) idWidgetActionHandler( &pdaList, WIDGET_ACTION_EVENT_SCROLL_DOWN_START_REPEATER, WIDGET_EVENT_SCROLL_DOWN ) );
	pdaList.AddEventAction( WIDGET_EVENT_SCROLL_UP ).Set( new( TAG_SWF ) idWidgetActionHandler( &pdaList, WIDGET_ACTION_EVENT_SCROLL_UP_START_REPEATER, WIDGET_EVENT_SCROLL_UP ) );
	pdaList.AddEventAction( WIDGET_EVENT_SCROLL_DOWN_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( &pdaList, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_DOWN_RELEASE ) );
	pdaList.AddEventAction( WIDGET_EVENT_SCROLL_UP_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( &pdaList, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_UP_RELEASE ) );

	menuScreens[ PDA_AREA_USER_DATA ]->RegisterEventObserver( &pdaList );
	menuScreens[ PDA_AREA_USER_EMAIL ]->RegisterEventObserver( &pdaList );

	idPlayer* player = gameLocal.GetLocalPlayer();
	if( player != NULL )
	{

		for( int j = 0; j < MAX_WEAPONS; j++ )
		{
			const char* weaponDefName = va( "def_weapon%d", j );
			const char* weap = player->spawnArgs.GetString( weaponDefName );
			if( weap != NULL && *weap != '\0' )
			{
				const idDeclEntityDef* weaponDef = gameLocal.FindEntityDef( weap, false );
				if( weaponDef != NULL )
				{
					declManager->FindMaterial( weaponDef->dict.GetString( "pdaIcon" ) );
					declManager->FindMaterial( weaponDef->dict.GetString( "hudIcon" ) );
				}
			}
		}

	}

	class idPDAGGUIClose : public idSWFScriptFunction_RefCounted
	{
	public:
		idSWFScriptVar Call( idSWFScriptObject* thisObject, const idSWFParmList& parms )
		{
			idPlayer* player = gameLocal.GetLocalPlayer();
			if( player != NULL )
			{
				player->TogglePDA();
			}
			return idSWFScriptVar();
		}
	};

	if( gui != NULL )
	{
		gui->SetGlobal( "closePDA", new idPDAGGUIClose() );
	}

	// precache sounds
	// don't load gui music for the pause menu to save some memory
	const idSoundShader* soundShader = NULL;
	soundShader = declManager->FindSound( "gui/list_scroll", true );
	if( soundShader != NULL )
	{
		sounds[ GUI_SOUND_SCROLL ] = soundShader->GetName();
	}
	soundShader = declManager->FindSound( "gui/btn_PDA_advance", true );
	if( soundShader != NULL )
	{
		sounds[ GUI_SOUND_ADVANCE ] = soundShader->GetName();
	}
	soundShader = declManager->FindSound( "gui/btn_PDA_back", true );
	if( soundShader != NULL )
	{
		sounds[ GUI_SOUND_BACK ] = soundShader->GetName();
	}
	soundShader = declManager->FindSound( "gui/pda_next_tab", true );
	if( soundShader != NULL )
	{
		sounds[ GUI_SOUND_BUILD_ON ] = soundShader->GetName();
	}
	soundShader = declManager->FindSound( "gui/pda_prev_tab", true );
	if( soundShader != NULL )
	{
		sounds[ GUI_SOUND_BUILD_OFF ] = soundShader->GetName();
	}
	soundShader = declManager->FindSound( "gui/btn_set_focus", true );
	if( soundShader != NULL )
	{
		sounds[ GUI_SOUND_FOCUS ] = soundShader->GetName();
	}
	soundShader = declManager->FindSound( "gui/btn_roll_over", true );
	if( soundShader != NULL )
	{
		sounds[ GUI_SOUND_ROLL_OVER ] = soundShader->GetName();
	}
	soundShader = declManager->FindSound( "gui/btn_roll_out", true );
	if( soundShader != NULL )
	{
		sounds[ GUI_SOUND_ROLL_OUT ] = soundShader->GetName();
	}
}

/*
========================
idMenuHandler_PDA::HandleAction
========================
*/
bool idMenuHandler_PDA::HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled )
{

	if( activeScreen == PDA_AREA_INVALID )
	{
		return true;
	}

	widgetAction_t actionType = action.GetType();
	const idSWFParmList& parms = action.GetParms();

	if( event.type == WIDGET_EVENT_COMMAND )
	{
		if( menuScreens[ activeScreen ] != NULL && !forceHandled )
		{
			if( menuScreens[ activeScreen ]->HandleAction( action, event, widget, true ) )
			{
				if( actionType == WIDGET_ACTION_GO_BACK )
				{
					PlaySound( GUI_SOUND_BACK );
				}
				else
				{
					PlaySound( GUI_SOUND_ADVANCE );
				}
				return true;
			}
		}
	}

	switch( actionType )
	{
		case WIDGET_ACTION_PDA_SELECT_USER:
		{
			int index = parms[0].ToInteger();
			idMenuWidget_DynamicList* pdaList = dynamic_cast< idMenuWidget_DynamicList* >( GetChildFromIndex( PDA_WIDGET_PDA_LIST ) );
			if( pdaList != NULL )
			{
				pdaList->SetViewIndex( pdaList->GetViewOffset() + index );
				pdaList->SetFocusIndex( index );
			}
			return true;
		}
		case WIDGET_ACTION_SCROLL_TAB:
		{

			if( transition != MENU_TRANSITION_INVALID )
			{
				return true;
			}
			int delta = parms[0].ToInteger();
			idMenuWidget_NavBar* navBar = dynamic_cast< idMenuWidget_NavBar* >( GetChildFromIndex( PDA_WIDGET_NAV_BAR ) );
			if( navBar != NULL )
			{
				int focused = navBar->GetFocusIndex();
				focused += delta;
				if( focused < 0 )
				{
					focused = navBar->GetNumVisibleOptions() - 1;
				}
				else if( focused >= navBar->GetNumVisibleOptions() )
				{
					focused = 0;
				}

				navBar->SetViewIndex( focused );
				navBar->SetFocusIndex( focused, true );
				navBar->Update();

				nextScreen = activeScreen + delta;
				if( nextScreen < 0 )
				{
					nextScreen = PDA_NUM_AREAS - 1;
				}
				else if( nextScreen == PDA_NUM_AREAS )
				{
					nextScreen = 0;
				}

				if( delta < 0 )
				{
					transition = MENU_TRANSITION_BACK;
				}
				else
				{
					transition = MENU_TRANSITION_ADVANCE;
				}

			}
			return true;
		}
		case WIDGET_ACTION_PDA_SELECT_NAV:
		{
			int index = parms[0].ToInteger();

			if( index == -1 && activeScreen == PDA_AREA_USER_EMAIL )
			{
				idMenuScreen_PDA_UserEmails* screen = dynamic_cast< idMenuScreen_PDA_UserEmails* const >( menuScreens[ PDA_AREA_USER_EMAIL ] );
				if( screen )
				{
					screen->ShowEmail( false );
				}
				return true;
			}

			// click on the current nav tab
			if( index == -1 )
			{
				return true;
			}

			idMenuWidget_NavBar* navBar = dynamic_cast< idMenuWidget_NavBar* >( GetChildFromIndex( PDA_WIDGET_NAV_BAR ) );
			if( navBar != NULL )
			{
				navBar->SetViewIndex( navBar->GetViewOffset() + index );
				navBar->SetFocusIndex( index, true );
				navBar->Update();

				if( index < activeScreen )
				{
					nextScreen = index;
					transition = MENU_TRANSITION_BACK;
				}
				else if( index > activeScreen )
				{
					nextScreen = index;
					transition = MENU_TRANSITION_ADVANCE;
				}
			}
			return true;
		}
		case WIDGET_ACTION_SELECT_PDA_AUDIO:
		{
			if( activeScreen == PDA_AREA_USER_DATA )
			{
				int index = parms[0].ToInteger();
				idMenuWidget_DynamicList* pdaList = dynamic_cast< idMenuWidget_DynamicList* >( GetChildFromIndex( PDA_WIDGET_PDA_LIST ) );

				bool change = false;
				if( pdaList != NULL )
				{
					int pdaIndex = pdaList->GetViewIndex();
					change = PlayPDAAudioLog( pdaIndex, index );
				}

				if( change )
				{
					if( widget->GetParent() != NULL )
					{
						idMenuWidget_DynamicList* audioList = dynamic_cast< idMenuWidget_DynamicList* >( widget->GetParent() );
						int index = parms[0].ToInteger();
						if( audioList != NULL )
						{
							audioList->SetFocusIndex( index );
						}
					}
				}
			}

			return true;
		}
		case WIDGET_ACTION_SELECT_PDA_VIDEO:
		{
			if( activeScreen == PDA_AREA_VIDEO_DISKS )
			{
				int index = parms[0].ToInteger();
				if( menuScreens[ PDA_AREA_VIDEO_DISKS ] != NULL )
				{
					idMenuScreen_PDA_VideoDisks* screen = dynamic_cast< idMenuScreen_PDA_VideoDisks* const >( menuScreens[ PDA_AREA_VIDEO_DISKS ] );
					if( screen != NULL )
					{
						screen->SelectedVideoToPlay( index );
					}
				}
			}
			return true;
		}
	}

	return idMenuHandler::HandleAction( action, event, widget, forceHandled );
}

/*
========================
idMenuHandler_PDA::PlayPDAAudioLog
========================
*/
bool idMenuHandler_PDA::PlayPDAAudioLog( int pdaIndex, int audioIndex )
{
	idPlayer* player = gameLocal.GetLocalPlayer();
	if( player != NULL )
	{
		const idDeclPDA* pda = player->GetInventory().pdas[ pdaIndex ];
		if( pda != NULL && pda->GetNumAudios() > audioIndex )
		{
			const idDeclAudio* aud = pda->GetAudioByIndex( audioIndex );

			if( audioFile == aud )
			{
				player->EndAudioLog();
				return true;
			}
			else if( aud != NULL )
			{
				audioFile = aud;
				player->EndAudioLog();
				player->PlayAudioLog( aud->GetWave() );
				return true;
			}
		}
	}
	return false;
}

/*
========================
idMenuHandler_PDA::GetMenuScreen
========================
*/
idMenuScreen* idMenuHandler_PDA::GetMenuScreen( int index )
{

	if( index < 0 || index >= PDA_NUM_AREAS )
	{
		return NULL;
	}

	return menuScreens[ index ];

}

/*
========================
idMenuHandler_PDA::GetMenuScreen
========================
*/
void idMenuHandler_PDA::UpdateAudioLogPlaying( bool playing )
{

	if( playing != audioLogPlaying && activeScreen == PDA_AREA_USER_DATA && menuScreens[ activeScreen ] != NULL )
	{
		menuScreens[ activeScreen ]->Update();
	}

	audioLogPlaying = playing;
	if( !playing )
	{
		audioFile = NULL;
	}
}

/*
========================
idMenuHandler_PDA::GetMenuScreen
========================
*/
void idMenuHandler_PDA::UdpateVideoPlaying( bool playing )
{

	if( playing != videoPlaying )
	{
		if( activeScreen == PDA_AREA_VIDEO_DISKS && menuScreens[ activeScreen ] != NULL )
		{
			idPlayer* player = gameLocal.GetLocalPlayer();
			if( !playing )
			{
				player->EndVideoDisk();
			}

			idMenuScreen_PDA_VideoDisks* screen = dynamic_cast< idMenuScreen_PDA_VideoDisks* const >( menuScreens[ PDA_AREA_VIDEO_DISKS ] );
			if( screen != NULL )
			{
				if( !playing )
				{
					screen->ClearActiveVideo();
				}
				screen->Update();
			}
		}

		videoPlaying = playing;
	}
}

/*
================================================
idMenuHandler_PDA::Cleanup
================================================
*/
void idMenuHandler_PDA::Cleanup()
{
	idMenuHandler::Cleanup();
	for( int index = 0; index < MAX_SCREEN_AREAS; ++index )
	{
		delete menuScreens[ index ];
		menuScreens[ index ] = NULL;
	}
}


/*
================================================
idMenuHandler_PDA::~idMenuHandler_PDA
================================================
*/
idMenuHandler_PDA::~idMenuHandler_PDA()
{
	pdaScrollBar.Cleanup();
	pdaList.Cleanup();
	navBar.Cleanup();
	commandBarWidget.Cleanup();
	Cleanup();
}
