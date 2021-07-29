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


extern idCVar g_demoMode;

static const int PEER_UPDATE_INTERVAL = 500;
static const int MAX_MENU_OPTIONS = 6;

void idMenuHandler_Shell::Update()
{

//#if defined ( ID_360 )
//	if ( deviceRequestedSignal.Wait( 0 ) ) {
//		// This clears the delete save dialog to catch the case of a delete confirmation for an old device after we've changed the device.
//		common->Dialog().ClearDialog( GDM_DELETE_SAVE );
//		common->Dialog().ClearDialog( GDM_DELETE_CORRUPT_SAVEGAME );
//		common->Dialog().ClearDialog( GDM_RESTORE_CORRUPT_SAVEGAME );
//		common->Dialog().ClearDialog( GDM_LOAD_DAMAGED_FILE );
//		common->Dialog().ClearDialog( GDM_OVERWRITE_SAVE );
//
//	}
//#endif

	if( gui == NULL || !gui->IsActive() )
	{
		return;
	}

	if( ( IsPacifierVisible() || common->Dialog().IsDialogActive() ) && actionRepeater.isActive )
	{
		ClearWidgetActionRepeater();
	}

	if( nextState != state )
	{

		if( introGui != NULL && introGui->IsActive() )
		{
			gui->StopSound();
			showingIntro = false;
			introGui->Activate( false );
			PlaySound( GUI_SOUND_MUSIC );
		}

		if( nextState == SHELL_STATE_PRESS_START )
		{
			HidePacifier();
			nextScreen = SHELL_AREA_START;
			transition = MENU_TRANSITION_SIMPLE;
			state = nextState;
			if( menuBar != NULL && gui != NULL )
			{
				menuBar->ClearSprite();
			}
		}
		else if( nextState == SHELL_STATE_IDLE )
		{
			HidePacifier();
			if( nextScreen == SHELL_AREA_START || nextScreen == SHELL_AREA_PARTY_LOBBY || nextScreen == SHELL_AREA_GAME_LOBBY || nextScreen == SHELL_AREA_INVALID )
			{
				nextScreen = SHELL_AREA_ROOT;
			}

			if( menuBar != NULL && gui != NULL )
			{
				idSWFScriptObject& root = gui->GetRootObject();
				menuBar->BindSprite( root );
				SetupPCOptions();
			}
			transition = MENU_TRANSITION_SIMPLE;
			state = nextState;
		}
		else if( nextState == SHELL_STATE_PARTY_LOBBY )
		{
			HidePacifier();
			nextScreen = SHELL_AREA_PARTY_LOBBY;
			transition = MENU_TRANSITION_SIMPLE;
			state = nextState;
		}
		else if( nextState == SHELL_STATE_GAME_LOBBY )
		{
			HidePacifier();
			if( state != SHELL_STATE_IN_GAME )
			{
				timeRemaining = WAIT_START_TIME_LONG;
				idMatchParameters matchParameters = session->GetActivePlatformLobbyBase().GetMatchParms();
				/*if ( MatchTypeIsPrivate( matchParameters.matchFlags ) && ActiveScreen() == SHELL_AREA_PARTY_LOBBY ) {
					timeRemaining = 0;
					session->StartMatch();
					state = SHELL_STATE_IN_GAME;
				} else {*/
				nextScreen = SHELL_AREA_GAME_LOBBY;
				transition = MENU_TRANSITION_SIMPLE;
				//}

				state = nextState;
			}
		}
		else if( nextState == SHELL_STATE_PAUSED )
		{
			HidePacifier();
			transition = MENU_TRANSITION_SIMPLE;

			if( gameComplete )
			{
				nextScreen = SHELL_AREA_CREDITS;
			}
			else
			{
				nextScreen = SHELL_AREA_ROOT;
			}

			state = nextState;
		}
		else if( nextState == SHELL_STATE_CONNECTING )
		{
			ShowPacifier( "#str_dlg_connecting" );
			state = nextState;
		}
		else if( nextState == SHELL_STATE_SEARCHING )
		{
			ShowPacifier( "#str_online_mpstatus_searching" );
			state = nextState;
		}
	}

	if( activeScreen != nextScreen )
	{

		ClearWidgetActionRepeater();
		UpdateBGState();

		if( nextScreen == SHELL_AREA_INVALID )
		{

			if( activeScreen > SHELL_AREA_INVALID && activeScreen < SHELL_NUM_AREAS && menuScreens[ activeScreen ] != NULL )
			{
				menuScreens[ activeScreen ]->HideScreen( static_cast<mainMenuTransition_t>( transition ) );
			}

			if( cmdBar != NULL )
			{
				cmdBar->ClearAllButtons();
				cmdBar->Update();
			}

			idSWFSpriteInstance* bg = gui->GetRootObject().GetNestedSprite( "pause_bg" );
			idSWFSpriteInstance* edging = gui->GetRootObject().GetNestedSprite( "_fullscreen" );

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

			if( activeScreen > SHELL_AREA_INVALID && activeScreen < SHELL_NUM_AREAS && menuScreens[ activeScreen ] != NULL )
			{
				menuScreens[ activeScreen ]->HideScreen( static_cast<mainMenuTransition_t>( transition ) );
			}

			if( nextScreen > SHELL_AREA_INVALID && nextScreen < SHELL_NUM_AREAS && menuScreens[ nextScreen ] != NULL )
			{
				menuScreens[ nextScreen ]->UpdateCmds();
				menuScreens[ nextScreen ]->ShowScreen( static_cast<mainMenuTransition_t>( transition ) );
			}
		}

		transition = MENU_TRANSITION_INVALID;
		activeScreen = nextScreen;
	}

	if( cmdBar != NULL && cmdBar->GetSprite() )
	{
		if( common->Dialog().IsDialogActive() )
		{
			cmdBar->GetSprite()->SetVisible( false );
		}
		else
		{
			cmdBar->GetSprite()->SetVisible( true );
		}
	}

	idMenuHandler::Update();

	if( activeScreen == nextScreen && activeScreen == SHELL_AREA_LEADERBOARDS )
	{
		idMenuScreen_Shell_Leaderboards* screen = dynamic_cast< idMenuScreen_Shell_Leaderboards* >( menuScreens[ SHELL_AREA_LEADERBOARDS ] );
		if( screen != NULL )
		{
			screen->PumpLBCache();
			screen->RefreshLeaderboard();
		}
	}
	else if( activeScreen == nextScreen && activeScreen == SHELL_AREA_PARTY_LOBBY )
	{
		idMenuScreen_Shell_PartyLobby* screen = dynamic_cast< idMenuScreen_Shell_PartyLobby* >( menuScreens[ SHELL_AREA_PARTY_LOBBY ] );
		if( screen != NULL )
		{
			screen->UpdateLobby();
		}
	}
	else if( activeScreen == nextScreen && activeScreen == SHELL_AREA_GAME_LOBBY )
	{
		if( session->GetActingGameStateLobbyBase().IsHost() )
		{

			if( timeRemaining <= 0 && state != SHELL_STATE_IN_GAME )
			{
				session->StartMatch();
				state = SHELL_STATE_IN_GAME;
			}

			idMatchParameters matchParameters = session->GetActivePlatformLobbyBase().GetMatchParms();
			if( !MatchTypeIsPrivate( matchParameters.matchFlags ) )
			{
				if( Sys_Milliseconds() >= nextPeerUpdateMs )
				{
					nextPeerUpdateMs = Sys_Milliseconds() + PEER_UPDATE_INTERVAL;
					byte buffer[ 128 ];
					idBitMsg msg;
					msg.InitWrite( buffer, sizeof( buffer ) );
					msg.WriteLong( timeRemaining );
					session->GetActingGameStateLobbyBase().SendReliable( GAME_RELIABLE_MESSAGE_LOBBY_COUNTDOWN, msg, false );
				}
			}
		}

		idMenuScreen_Shell_GameLobby* screen = dynamic_cast< idMenuScreen_Shell_GameLobby* >( menuScreens[ SHELL_AREA_GAME_LOBBY ] );
		if( screen != NULL )
		{
			screen->UpdateLobby();
		}
	}

	if( introGui != NULL && introGui->IsActive() )
	{
		introGui->Render( renderSystem, Sys_Milliseconds() );
	}

	if( continueWaitForEnumerate )
	{
		if( !session->GetSaveGameManager().IsWorking() )
		{
			continueWaitForEnumerate = false;
			common->Dialog().ClearDialog( GDM_REFRESHING );
			idMenuScreen_Shell_Singleplayer* screen = dynamic_cast< idMenuScreen_Shell_Singleplayer* >( menuScreens[ SHELL_AREA_CAMPAIGN ] );
			if( screen != NULL )
			{
				screen->ContinueGame();
			}
		}
	}
}

/*
========================
idMenuHandler_Shell::SetCanContinue
========================
*/
void idMenuHandler_Shell::SetCanContinue( bool valid )
{

	idMenuScreen_Shell_Singleplayer* screen = dynamic_cast< idMenuScreen_Shell_Singleplayer* >( menuScreens[ SHELL_AREA_CAMPAIGN ] );
	if( screen != NULL )
	{
		screen->SetCanContinue( valid );
	}

}

/*
========================
idMenuHandler_Shell::HandleGuiEvent
========================
*/
bool idMenuHandler_Shell::HandleGuiEvent( const sysEvent_t* sev )
{

	if( IsPacifierVisible() )
	{
		return true;
	}

	if( showingIntro )
	{
		return true;
	}

	if( waitForBinding )
	{

		if( sev->evType == SE_KEY && sev->evValue2 == 1 )
		{

			if( sev->evValue >= K_JOY_STICK1_UP && sev->evValue <= K_JOY_STICK2_RIGHT )
			{
				return true;
			}

			if( sev->evValue == K_ESCAPE )
			{
				waitForBinding = false;

				idMenuScreen_Shell_Bindings* bindScreen = dynamic_cast< idMenuScreen_Shell_Bindings* >( menuScreens[ SHELL_AREA_KEYBOARD ] );
				if( bindScreen != NULL )
				{
					bindScreen->ToggleWait( false );
					bindScreen->Update();
				}

			}
			else
			{

				if( idStr::Icmp( idKeyInput::GetBinding( sev->evValue ), "" ) == 0 )  	// no existing binding found
				{

					idKeyInput::SetBinding( sev->evValue, waitBind );

					idMenuScreen_Shell_Bindings* bindScreen = dynamic_cast< idMenuScreen_Shell_Bindings* >( menuScreens[ SHELL_AREA_KEYBOARD ] );
					if( bindScreen != NULL )
					{
						bindScreen->SetBindingChanged( true );
						bindScreen->UpdateBindingDisplay();
						bindScreen->ToggleWait( false );
						bindScreen->Update();
					}

					waitForBinding = false;

				}
				else  	// binding found prompt to change
				{

					const char* curBind = idKeyInput::GetBinding( sev->evValue );

					if( idStr::Icmp( waitBind, curBind ) == 0 )
					{
						idKeyInput::SetBinding( sev->evValue, "" );
						idMenuScreen_Shell_Bindings* bindScreen = dynamic_cast< idMenuScreen_Shell_Bindings* >( menuScreens[ SHELL_AREA_KEYBOARD ] );
						if( bindScreen != NULL )
						{
							bindScreen->SetBindingChanged( true );
							bindScreen->UpdateBindingDisplay();
							bindScreen->ToggleWait( false );
							bindScreen->Update();
							waitForBinding = false;
						}
					}
					else
					{

						idMenuScreen_Shell_Bindings* bindScreen = dynamic_cast< idMenuScreen_Shell_Bindings* >( menuScreens[ SHELL_AREA_KEYBOARD ] );
						if( bindScreen != NULL )
						{
							class idSWFScriptFunction_RebindKey : public idSWFScriptFunction_RefCounted
							{
							public:
								idSWFScriptFunction_RebindKey( idMenuScreen_Shell_Bindings* _menu, gameDialogMessages_t _msg, bool _accept, idMenuHandler_Shell* _mgr, int _key, const char* _bind )
								{
									menu = _menu;
									msg = _msg;
									accept = _accept;
									mgr = _mgr;
									key = _key;
									bind = _bind;
								}
								idSWFScriptVar Call( idSWFScriptObject* thisObject, const idSWFParmList& parms )
								{
									common->Dialog().ClearDialog( msg );
									mgr->ClearWaitForBinding();
									menu->ToggleWait( false );
									if( accept )
									{
										idKeyInput::SetBinding( key, bind );
										menu->SetBindingChanged( true );
										menu->UpdateBindingDisplay();
										menu->Update();
									}
									return idSWFScriptVar();
								}
							private:
								idMenuScreen_Shell_Bindings* menu;
								gameDialogMessages_t msg;
								bool accept;
								idMenuHandler_Shell* mgr;
								int key;
								const char* bind;
							};

							common->Dialog().AddDialog( GDM_BINDING_ALREDY_SET, DIALOG_ACCEPT_CANCEL, new idSWFScriptFunction_RebindKey( bindScreen, GDM_BINDING_ALREDY_SET, true, this, sev->evValue, waitBind ), new idSWFScriptFunction_RebindKey( bindScreen, GDM_BINDING_ALREDY_SET, false, this, sev->evValue, waitBind ), false );
						}

					}
				}
			}
		}

		return true;
	}

	return idMenuHandler::HandleGuiEvent( sev );

}

/*
========================
idMenuHandler_Shell::Initialize
========================
*/
void idMenuHandler_Shell::Initialize( const char* swfFile, idSoundWorld* sw )
{
	idMenuHandler::Initialize( swfFile, sw );

	//---------------------
	// Initialize the menus
	//---------------------
#define BIND_SHELL_SCREEN( screenId, className, menuHandler )				\
	menuScreens[ (screenId) ] = new (TAG_SWF) className();	\
	menuScreens[ (screenId) ]->Initialize( menuHandler );	\
	menuScreens[ (screenId) ]->AddRef();

	for( int i = 0; i < SHELL_NUM_AREAS; ++i )
	{
		menuScreens[ i ] = NULL;
	}

	// done for build game purposes so these get touched
	delete new idSWF( "doomIntro", NULL );
	delete new idSWF( "roeIntro", NULL );
	delete new idSWF( "leIntro", NULL );

	if( inGame )
	{
		BIND_SHELL_SCREEN( SHELL_AREA_ROOT, idMenuScreen_Shell_Pause, this );
		BIND_SHELL_SCREEN( SHELL_AREA_SETTINGS, idMenuScreen_Shell_Settings, this );
		BIND_SHELL_SCREEN( SHELL_AREA_LOAD, idMenuScreen_Shell_Load, this );
		BIND_SHELL_SCREEN( SHELL_AREA_SYSTEM_OPTIONS, idMenuScreen_Shell_SystemOptions, this );
		BIND_SHELL_SCREEN( SHELL_AREA_GAME_OPTIONS, idMenuScreen_Shell_GameOptions, this );
		BIND_SHELL_SCREEN( SHELL_AREA_SAVE, idMenuScreen_Shell_Save, this );
		BIND_SHELL_SCREEN( SHELL_AREA_STEREOSCOPICS, idMenuScreen_Shell_Stereoscopics, this );
		BIND_SHELL_SCREEN( SHELL_AREA_CONTROLS, idMenuScreen_Shell_Controls, this );
		BIND_SHELL_SCREEN( SHELL_AREA_KEYBOARD, idMenuScreen_Shell_Bindings, this );
		BIND_SHELL_SCREEN( SHELL_AREA_RESOLUTION, idMenuScreen_Shell_Resolution, this );
		BIND_SHELL_SCREEN( SHELL_AREA_CONTROLLER_LAYOUT, idMenuScreen_Shell_ControllerLayout, this );

		BIND_SHELL_SCREEN( SHELL_AREA_GAMEPAD, idMenuScreen_Shell_Gamepad, this );
		BIND_SHELL_SCREEN( SHELL_AREA_CREDITS, idMenuScreen_Shell_Credits, this );

	}
	else
	{
		BIND_SHELL_SCREEN( SHELL_AREA_START, idMenuScreen_Shell_PressStart, this );
		BIND_SHELL_SCREEN( SHELL_AREA_ROOT, idMenuScreen_Shell_Root, this );
		BIND_SHELL_SCREEN( SHELL_AREA_CAMPAIGN, idMenuScreen_Shell_Singleplayer, this );
		BIND_SHELL_SCREEN( SHELL_AREA_SETTINGS, idMenuScreen_Shell_Settings, this );
		BIND_SHELL_SCREEN( SHELL_AREA_LOAD, idMenuScreen_Shell_Load, this );
		BIND_SHELL_SCREEN( SHELL_AREA_NEW_GAME, idMenuScreen_Shell_NewGame, this );
		BIND_SHELL_SCREEN( SHELL_AREA_SYSTEM_OPTIONS, idMenuScreen_Shell_SystemOptions, this );
		BIND_SHELL_SCREEN( SHELL_AREA_GAME_OPTIONS, idMenuScreen_Shell_GameOptions, this );
		BIND_SHELL_SCREEN( SHELL_AREA_PARTY_LOBBY, idMenuScreen_Shell_PartyLobby, this );
		BIND_SHELL_SCREEN( SHELL_AREA_GAME_LOBBY, idMenuScreen_Shell_GameLobby, this );
		BIND_SHELL_SCREEN( SHELL_AREA_STEREOSCOPICS, idMenuScreen_Shell_Stereoscopics, this );
		BIND_SHELL_SCREEN( SHELL_AREA_DIFFICULTY, idMenuScreen_Shell_Difficulty, this );
		BIND_SHELL_SCREEN( SHELL_AREA_CONTROLS, idMenuScreen_Shell_Controls, this );
		BIND_SHELL_SCREEN( SHELL_AREA_KEYBOARD, idMenuScreen_Shell_Bindings, this );
		BIND_SHELL_SCREEN( SHELL_AREA_RESOLUTION, idMenuScreen_Shell_Resolution, this );
		BIND_SHELL_SCREEN( SHELL_AREA_CONTROLLER_LAYOUT, idMenuScreen_Shell_ControllerLayout, this );
		BIND_SHELL_SCREEN( SHELL_AREA_DEV, idMenuScreen_Shell_Dev, this );
		BIND_SHELL_SCREEN( SHELL_AREA_LEADERBOARDS, idMenuScreen_Shell_Leaderboards, this );
		BIND_SHELL_SCREEN( SHELL_AREA_GAMEPAD, idMenuScreen_Shell_Gamepad, this );
		BIND_SHELL_SCREEN( SHELL_AREA_MATCH_SETTINGS, idMenuScreen_Shell_MatchSettings, this );
		BIND_SHELL_SCREEN( SHELL_AREA_MODE_SELECT, idMenuScreen_Shell_ModeSelect, this );
		BIND_SHELL_SCREEN( SHELL_AREA_BROWSER, idMenuScreen_Shell_GameBrowser, this );
		BIND_SHELL_SCREEN( SHELL_AREA_CREDITS, idMenuScreen_Shell_Credits, this );

		doom3Intro = declManager->FindMaterial( "gui/intro/introloop" );
		roeIntro = declManager->FindMaterial( "gui/intro/marsflyby" );

		//typeSoundShader = declManager->FindSound( "gui/teletype/print_text", true );
		typeSoundShader = declManager->FindSound( "gui/teletype/print_text", true );
		declManager->FindSound( "gui/doomintro", true );

		marsRotation = declManager->FindMaterial( "gui/shell/mars_rotation" );
	}

	menuBar = new( TAG_SWF ) idMenuWidget_MenuBar();
	menuBar->SetSpritePath( "pcBar" );
	menuBar->Initialize( this );
	menuBar->SetNumVisibleOptions( MAX_MENU_OPTIONS );
	menuBar->SetWrappingAllowed( true );
	menuBar->SetButtonSpacing( 45.0f );
	while( menuBar->GetChildren().Num() < MAX_MENU_OPTIONS )
	{
		idMenuWidget_MenuButton* const navButton = new( TAG_SWF ) idMenuWidget_MenuButton();
		idMenuScreen_Shell_Root* rootScreen = dynamic_cast< idMenuScreen_Shell_Root* >( menuScreens[ SHELL_AREA_ROOT ] );
		if( rootScreen != NULL )
		{
			navButton->RegisterEventObserver( rootScreen->GetHelpWidget() );
		}
		menuBar->AddChild( navButton );
	}
	AddChild( menuBar );

	//
	// command bar
	//
	cmdBar = new( TAG_SWF ) idMenuWidget_CommandBar();
	cmdBar->SetAlignment( idMenuWidget_CommandBar::LEFT );
	cmdBar->SetSpritePath( "prompts" );
	cmdBar->Initialize( this );
	AddChild( cmdBar );

	pacifier = new( TAG_SWF ) idMenuWidget();
	pacifier->SetSpritePath( "pacifier" );
	AddChild( pacifier );

	// precache sounds
	// don't load gui music for the pause menu to save some memory
	const idSoundShader* soundShader = NULL;
	if( !inGame )
	{
		soundShader = declManager->FindSound( "gui/menu_music", true );
		if( soundShader != NULL )
		{
			sounds[ GUI_SOUND_MUSIC ] = soundShader->GetName();
		}
	}
	else
	{
		idStrStatic< MAX_OSPATH > shortMapName = gameLocal.GetMapFileName();
		shortMapName.StripFileExtension();
		shortMapName.StripLeading( "maps/" );
		shortMapName.StripLeading( "game/" );
		if( ( shortMapName.Icmp( "le_hell_post" ) == 0 ) || ( shortMapName.Icmp( "hellhole" ) == 0 ) || ( shortMapName.Icmp( "hell" ) == 0 ) )
		{
			soundShader = declManager->FindSound( "hell_music_credits", true );
			if( soundShader != NULL )
			{
				sounds[ GUI_SOUND_MUSIC ] = soundShader->GetName();
			}
		}
	}

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
	soundShader = declManager->FindSound( "gui/menu_build_on", true );
	if( soundShader != NULL )
	{
		sounds[ GUI_SOUND_BUILD_ON ] = soundShader->GetName();
	}
	soundShader = declManager->FindSound( "gui/pda_next_tab", true );
	if( soundShader != NULL )
	{
		sounds[ GUI_SOUND_BUILD_ON ] = soundShader->GetName();
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

	class idPauseGUIClose : public idSWFScriptFunction_RefCounted
	{
	public:
		idSWFScriptVar Call( idSWFScriptObject* thisObject, const idSWFParmList& parms )
		{
			gameLocal.Shell_Show( false );
			return idSWFScriptVar();
		}
	};

	if( gui != NULL )
	{
		gui->SetGlobal( "closeMenu", new idPauseGUIClose() );
	}
}

/*
========================
idMenuHandler_Shell::Cleanup
========================
*/
void idMenuHandler_Shell::Cleanup()
{
	idMenuHandler::Cleanup();

	delete introGui;
	introGui = NULL;
}

/*
========================
idMenuHandler_Shell::ActivateMenu
========================
*/
void idMenuHandler_Shell::ActivateMenu( bool show )
{

	if( show && gui != NULL && gui->IsActive() )
	{
		return;
	}
	else if( !show && gui != NULL && !gui->IsActive() )
	{
		return;
	}


	if( inGame )
	{
		idPlayer* player = gameLocal.GetLocalPlayer();
		if( player != NULL )
		{
			if( !show )
			{
				bool isDead = false;
				if( player->health <= 0 )
				{
					isDead = true;
				}

				if( isDead && !common->IsMultiplayer() )
				{
					return;
				}
			}
		}
	}

	idMenuHandler::ActivateMenu( show );
	if( show )
	{

		if( !inGame )
		{
			PlaySound( GUI_SOUND_MUSIC );

			if( gui != NULL )
			{

				idSWFSpriteInstance* mars = gui->GetRootObject().GetNestedSprite( "mars" );
				if( mars )
				{
					mars->stereoDepth = STEREO_DEPTH_TYPE_FAR;

					idSWFSpriteInstance* planet = mars->GetScriptObject()->GetNestedSprite( "planet" );

					if( marsRotation != NULL && planet != NULL )
					{
						const idMaterial* mat = marsRotation;
						if( mat != NULL )
						{
							int c = mat->GetNumStages();
							for( int i = 0; i < c; i++ )
							{
								const shaderStage_t* stage = mat->GetStage( i );
								if( stage != NULL && stage->texture.cinematic )
								{
									stage->texture.cinematic->ResetTime( Sys_Milliseconds() );
								}
							}
						}

						planet->SetMaterial( mat );
					}
				}
			}
		}

		SetupPCOptions();

		if( cmdBar != NULL )
		{
			cmdBar->ClearAllButtons();
			cmdBar->Update();
		}

	}
	else
	{
		ClearWidgetActionRepeater();
		nextScreen = SHELL_AREA_INVALID;
		activeScreen = SHELL_AREA_INVALID;
		nextState = SHELL_STATE_INVALID;
		state = SHELL_STATE_INVALID;
		smallFrameShowing = false;
		largeFrameShowing = false;
		bgShowing = true;

		common->Dialog().ClearDialog( GDM_LEAVE_LOBBY_RET_NEW_PARTY );
	}
}

enum shellCommandsPC_t
{
	SHELL_CMD_DEMO0,
	SHELL_CMD_DEMO1,
	SHELL_CMD_DEV,
	SHELL_CMD_CAMPAIGN,
	SHELL_CMD_MULTIPLAYER,
	SHELL_CMD_SETTINGS,
	SHELL_CMD_CREDITS,
	SHELL_CMD_QUIT
};

/*
========================
idMenuHandler_Shell::SetPCOptionsVisible
========================
*/
void idMenuHandler_Shell::SetupPCOptions()
{

	if( inGame )
	{
		return;
	}

	navOptions.Clear();

	if( GetPlatform() == 2 && menuBar != NULL )
	{
		if( g_demoMode.GetBool() )
		{
			navOptions.Append( "START DEMO" );	// START DEMO
			if( g_demoMode.GetInteger() == 2 )
			{
				navOptions.Append( "START PRESS DEMO" );	// START DEMO
			}
			navOptions.Append( "#str_swf_settings" );	// settings
			navOptions.Append( "#str_swf_quit" );	// quit

			idMenuWidget_MenuButton* buttonWidget = NULL;
			int index = 0;
			buttonWidget = dynamic_cast< idMenuWidget_MenuButton* >( &menuBar->GetChildByIndex( index ) );
			if( buttonWidget != NULL )
			{
				buttonWidget->ClearEventActions();
				buttonWidget->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, SHELL_CMD_DEMO0, index );
				buttonWidget->SetDescription( "Launch the demo" );
			}
			if( g_demoMode.GetInteger() == 2 )
			{
				index++;
				buttonWidget = dynamic_cast< idMenuWidget_MenuButton* >( &menuBar->GetChildByIndex( index ) );
				if( buttonWidget != NULL )
				{
					buttonWidget->ClearEventActions();
					buttonWidget->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, SHELL_CMD_DEMO1, index );
					buttonWidget->SetDescription( "Launch the press Demo" );
				}
			}
			index++;
			buttonWidget = dynamic_cast< idMenuWidget_MenuButton* >( &menuBar->GetChildByIndex( index ) );
			if( buttonWidget != NULL )
			{
				buttonWidget->ClearEventActions();
				buttonWidget->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, SHELL_CMD_SETTINGS, index );
				buttonWidget->SetDescription( "#str_02206" );
			}
			index++;
			buttonWidget = dynamic_cast< idMenuWidget_MenuButton* >( &menuBar->GetChildByIndex( index ) );
			if( buttonWidget != NULL )
			{
				buttonWidget->ClearEventActions();
				buttonWidget->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, SHELL_CMD_QUIT, index );
				buttonWidget->SetDescription( "#str_01976" );
			}
		}
		else
		{
#if !defined ( ID_RETAIL )
			navOptions.Append( "DEV" );	// DEV
#endif
			navOptions.Append( "#str_swf_campaign" );	// singleplayer
			navOptions.Append( "#str_swf_multiplayer" );	// multiplayer
			navOptions.Append( "#str_swf_settings" );	// settings
			navOptions.Append( "#str_swf_credits" );	// credits
			navOptions.Append( "#str_swf_quit" );	// quit


			idMenuWidget_MenuButton* buttonWidget = NULL;
			int index = 0;
#if !defined ( ID_RETAIL )
			buttonWidget = dynamic_cast< idMenuWidget_MenuButton* >( &menuBar->GetChildByIndex( index ) );
			if( buttonWidget != NULL )
			{
				buttonWidget->ClearEventActions();
				buttonWidget->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, SHELL_CMD_DEV, index );
				buttonWidget->SetDescription( "View a list of maps available for play" );
			}
			index++;
#endif
			buttonWidget = dynamic_cast< idMenuWidget_MenuButton* >( &menuBar->GetChildByIndex( index ) );
			if( buttonWidget != NULL )
			{
				buttonWidget->ClearEventActions();
				buttonWidget->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, SHELL_CMD_CAMPAIGN, index );
				buttonWidget->SetDescription( "#str_swf_campaign_desc" );
			}
			index++;
			buttonWidget = dynamic_cast< idMenuWidget_MenuButton* >( &menuBar->GetChildByIndex( index ) );
			if( buttonWidget != NULL )
			{
				buttonWidget->ClearEventActions();
				buttonWidget->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, SHELL_CMD_MULTIPLAYER, index );
				buttonWidget->SetDescription( "#str_02215" );
			}
			index++;
			buttonWidget = dynamic_cast< idMenuWidget_MenuButton* >( &menuBar->GetChildByIndex( index ) );
			if( buttonWidget != NULL )
			{
				buttonWidget->ClearEventActions();
				buttonWidget->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, SHELL_CMD_SETTINGS, index );
				buttonWidget->SetDescription( "#str_02206" );
			}
			index++;
			buttonWidget = dynamic_cast< idMenuWidget_MenuButton* >( &menuBar->GetChildByIndex( index ) );
			if( buttonWidget != NULL )
			{
				buttonWidget->ClearEventActions();
				buttonWidget->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, SHELL_CMD_CREDITS, index );
				buttonWidget->SetDescription( "#str_02219" );
			}
			index++;
			buttonWidget = dynamic_cast< idMenuWidget_MenuButton* >( &menuBar->GetChildByIndex( index ) );
			if( buttonWidget != NULL )
			{
				buttonWidget->ClearEventActions();
				buttonWidget->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, SHELL_CMD_QUIT, index );
				buttonWidget->SetDescription( "#str_01976" );
			}
		}
	}

	if( menuBar != NULL && gui != NULL )
	{
		idSWFScriptObject& root = gui->GetRootObject();
		if( menuBar->BindSprite( root ) )
		{
			menuBar->GetSprite()->SetVisible( true );
			menuBar->SetListHeadings( navOptions );
			menuBar->Update();

			idMenuScreen_Shell_Root* menu = dynamic_cast< idMenuScreen_Shell_Root* >( menuScreens[ SHELL_AREA_ROOT ] );
			if( menu != NULL )
			{
				const int activeIndex = menu->GetRootIndex();
				menuBar->SetViewIndex( activeIndex );
				menuBar->SetFocusIndex( activeIndex );
			}

		}
	}
}

/*
========================
idMenuHandler_Shell::HandleExitGameBtn
========================
*/
void idMenuHandler_Shell::HandleExitGameBtn()
{
	class idSWFScriptFunction_QuitDialog : public idSWFScriptFunction_RefCounted
	{
	public:
		idSWFScriptFunction_QuitDialog( gameDialogMessages_t _msg, int _accept )
		{
			msg = _msg;
			accept = _accept;
		}
		idSWFScriptVar Call( idSWFScriptObject* thisObject, const idSWFParmList& parms )
		{
			common->Dialog().ClearDialog( msg );
			if( accept == 1 )
			{
				common->Quit();
			}
			else if( accept == -1 )
			{
				session->MoveToPressStart();
			}
			return idSWFScriptVar();
		}
	private:
		gameDialogMessages_t msg;
		int accept;
	};

	idStaticList< idSWFScriptFunction*, 4 > callbacks;
	idStaticList< idStrId, 4 > optionText;
	callbacks.Append( new( TAG_SWF ) idSWFScriptFunction_QuitDialog( GDM_QUIT_GAME, 1 ) );
	callbacks.Append( new( TAG_SWF ) idSWFScriptFunction_QuitDialog( GDM_QUIT_GAME, 0 ) );
	callbacks.Append( new( TAG_SWF ) idSWFScriptFunction_QuitDialog( GDM_QUIT_GAME, -1 ) );
	optionText.Append( idStrId( "#STR_SWF_ACCEPT" ) );
	optionText.Append( idStrId( "#STR_SWF_CANCEL" ) );
	optionText.Append( idStrId( "#str_swf_change_game" ) );

	common->Dialog().AddDynamicDialog( GDM_QUIT_GAME, callbacks, optionText, true, "" );
}

/*
========================
idMenuHandler_Shell::HandleAction
========================
*/
bool idMenuHandler_Shell::HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled )
{

	if( activeScreen == SHELL_AREA_INVALID )
	{
		return true;
	}

	widgetAction_t actionType = action.GetType();
	const idSWFParmList& parms = action.GetParms();

	if( event.type == WIDGET_EVENT_COMMAND )
	{

		/*if ( activeScreen == SHELL_AREA_ROOT && navOptions.Num() > 0 ) {
			return true;
		}*/

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
		case WIDGET_ACTION_COMMAND:
		{

			if( parms.Num() < 2 )
			{
				return true;
			}

			int cmd = parms[0].ToInteger();

			if( ( activeScreen == SHELL_AREA_GAME_LOBBY || activeScreen == SHELL_AREA_MATCH_SETTINGS ) && cmd != SHELL_CMD_QUIT && cmd != SHELL_CMD_MULTIPLAYER )
			{
				session->Cancel();
				session->Cancel();
			}
			else if( ( activeScreen == SHELL_AREA_PARTY_LOBBY || activeScreen == SHELL_AREA_LEADERBOARDS || activeScreen == SHELL_AREA_BROWSER || activeScreen == SHELL_AREA_MODE_SELECT ) && cmd != SHELL_CMD_QUIT && cmd != SHELL_CMD_MULTIPLAYER )
			{
				session->Cancel();
			}

			if( cmd != SHELL_CMD_QUIT && ( nextScreen == SHELL_AREA_STEREOSCOPICS || nextScreen == SHELL_AREA_SYSTEM_OPTIONS || nextScreen == SHELL_AREA_GAME_OPTIONS ||
										   nextScreen == SHELL_AREA_GAMEPAD || nextScreen == SHELL_AREA_MATCH_SETTINGS ) )
			{

				cvarSystem->SetModifiedFlags( CVAR_ARCHIVE );
			}

			const int index = parms[1].ToInteger();
			menuBar->SetFocusIndex( index );
			menuBar->SetViewIndex( index );

			idMenuScreen_Shell_Root* menu = dynamic_cast< idMenuScreen_Shell_Root* >( menuScreens[ SHELL_AREA_ROOT ] );
			if( menu != NULL )
			{
				menu->SetRootIndex( index );
			}

			switch( cmd )
			{
				case SHELL_CMD_DEMO0:
				{
					cmdSystem->AppendCommandText( va( "devmap %s %d\n", "demo/enpro_e3_2012", 1 ) );
					break;
				}
				case SHELL_CMD_DEMO1:
				{
					cmdSystem->AppendCommandText( va( "devmap %s %d\n", "game/le_hell", 2 ) );
					break;
				}
				case SHELL_CMD_DEV:
				{
					nextScreen = SHELL_AREA_DEV;
					transition = MENU_TRANSITION_SIMPLE;
					break;
				}
				case SHELL_CMD_CAMPAIGN:
				{
					nextScreen = SHELL_AREA_CAMPAIGN;
					transition = MENU_TRANSITION_SIMPLE;
					break;
				}
				case SHELL_CMD_MULTIPLAYER:
				{
					idMatchParameters matchParameters;
					matchParameters.matchFlags = DefaultPartyFlags;
					session->CreatePartyLobby( matchParameters );
					break;
				}
				case SHELL_CMD_SETTINGS:
				{
					nextScreen = SHELL_AREA_SETTINGS;
					transition = MENU_TRANSITION_SIMPLE;
					break;
				}
				case SHELL_CMD_CREDITS:
				{
					nextScreen = SHELL_AREA_CREDITS;
					transition = MENU_TRANSITION_SIMPLE;
					break;
				}
				case SHELL_CMD_QUIT:
				{
					HandleExitGameBtn();
					break;
				}
			}

			return true;
		}
	}

	return idMenuHandler::HandleAction( action, event, widget, forceHandled );
}

/*
========================
idMenuHandler_Shell::GetMenuScreen
========================
*/
idMenuScreen* idMenuHandler_Shell::GetMenuScreen( int index )
{

	if( index < 0 || index >= SHELL_NUM_AREAS )
	{
		return NULL;
	}

	return menuScreens[ index ];
}

/*
========================
idMenuHandler_Shell::ShowSmallFrame
========================
*/
void idMenuHandler_Shell::ShowSmallFrame( bool show )
{

	if( gui == NULL )
	{
		return;
	}

	idSWFSpriteInstance* smallFrame = gui->GetRootObject().GetNestedSprite( "smallFrame" );
	if( smallFrame == NULL )
	{
		return;
	}

	smallFrame->stereoDepth = STEREO_DEPTH_TYPE_MID;

	if( show && !smallFrameShowing )
	{
		smallFrame->PlayFrame( "rollOn" );
	}
	else if( !show && smallFrameShowing )
	{
		smallFrame->PlayFrame( "rollOff" );
	}

	smallFrameShowing = show;

}

/*
========================
idMenuHandler_Shell::ShowMPFrame
========================
*/
void idMenuHandler_Shell::ShowMPFrame( bool show )
{

	if( gui == NULL )
	{
		return;
	}

	idSWFSpriteInstance* smallFrame = gui->GetRootObject().GetNestedSprite( "smallFrameMP" );
	if( smallFrame == NULL )
	{
		return;
	}

	smallFrame->stereoDepth = STEREO_DEPTH_TYPE_MID;

	if( show && !largeFrameShowing )
	{
		smallFrame->PlayFrame( "rollOn" );
	}
	else if( !show && largeFrameShowing )
	{
		smallFrame->PlayFrame( "rollOff" );
	}

	largeFrameShowing = show;

}

/*
========================
idMenuHandler_Shell::ShowSmallFrame
========================
*/
void idMenuHandler_Shell::ShowLogo( bool show )
{

	if( gui == NULL )
	{
		return;
	}

	if( show == bgShowing )
	{
		return;
	}

	idSWFSpriteInstance* logo = gui->GetRootObject().GetNestedSprite( "logoInfo" );
	idSWFSpriteInstance* bg = gui->GetRootObject().GetNestedSprite( "background" );
	if( logo != NULL && bg != NULL )
	{

		bg->stereoDepth = STEREO_DEPTH_TYPE_MID;

		if( show && !bgShowing )
		{
			logo->PlayFrame( "rollOn" );
			bg->PlayFrame( "rollOff" );
		}
		else if( !show && bgShowing )
		{
			logo->PlayFrame( "rollOff" );
			bg->PlayFrame( "rollOn" );
		}
	}

	bgShowing = show;

}

/*
========================
idMenuHandler_Shell::UpdateSavedGames
========================
*/
void idMenuHandler_Shell::UpdateSavedGames()
{
	if( activeScreen == SHELL_AREA_LOAD )
	{
		idMenuScreen_Shell_Load* screen = dynamic_cast< idMenuScreen_Shell_Load* >( menuScreens[ SHELL_AREA_LOAD ] );
		if( screen != NULL )
		{
			screen->UpdateSaveEnumerations();
		}
	}
	else if( activeScreen == SHELL_AREA_SAVE )
	{
		idMenuScreen_Shell_Save* screen = dynamic_cast< idMenuScreen_Shell_Save* >( menuScreens[ SHELL_AREA_SAVE ] );
		if( screen != NULL )
		{
			screen->UpdateSaveEnumerations();
		}
	}
}

/*
========================
idMenuHandler_Shell::UpdateBGState
========================
*/
void idMenuHandler_Shell::UpdateBGState()
{

	if( smallFrameShowing )
	{
		if( nextScreen != SHELL_AREA_PLAYSTATION && nextScreen != SHELL_AREA_SETTINGS && nextScreen != SHELL_AREA_CAMPAIGN && nextScreen != SHELL_AREA_DEV )
		{
			if( nextScreen != SHELL_AREA_RESOLUTION && nextScreen != SHELL_AREA_GAMEPAD && nextScreen != SHELL_AREA_DIFFICULTY && nextScreen != SHELL_AREA_SYSTEM_OPTIONS && nextScreen != SHELL_AREA_GAME_OPTIONS && nextScreen != SHELL_AREA_NEW_GAME && nextScreen != SHELL_AREA_STEREOSCOPICS &&
					nextScreen != SHELL_AREA_CONTROLS )
			{
				ShowSmallFrame( false );
			}
		}
	}
	else
	{
		if( nextScreen == SHELL_AREA_RESOLUTION || nextScreen == SHELL_AREA_GAMEPAD || nextScreen == SHELL_AREA_PLAYSTATION || nextScreen == SHELL_AREA_SETTINGS || nextScreen == SHELL_AREA_CAMPAIGN || nextScreen == SHELL_AREA_CONTROLS || nextScreen == SHELL_AREA_DEV || nextScreen == SHELL_AREA_DIFFICULTY )
		{
			ShowSmallFrame( true );
		}
	}

	if( largeFrameShowing )
	{
		if( nextScreen != SHELL_AREA_PARTY_LOBBY && nextScreen != SHELL_AREA_GAME_LOBBY && nextScreen != SHELL_AREA_CONTROLLER_LAYOUT && nextScreen != SHELL_AREA_KEYBOARD && nextScreen != SHELL_AREA_LEADERBOARDS && nextScreen != SHELL_AREA_MATCH_SETTINGS && nextScreen != SHELL_AREA_MODE_SELECT &&
				nextScreen != SHELL_AREA_BROWSER && nextScreen != SHELL_AREA_LOAD && nextScreen != SHELL_AREA_SAVE && nextScreen != SHELL_AREA_CREDITS )
		{
			ShowMPFrame( false );
		}
	}
	else
	{
		if( nextScreen == SHELL_AREA_PARTY_LOBBY || nextScreen == SHELL_AREA_CONTROLLER_LAYOUT || nextScreen == SHELL_AREA_GAME_LOBBY || nextScreen == SHELL_AREA_KEYBOARD || nextScreen == SHELL_AREA_LEADERBOARDS || nextScreen == SHELL_AREA_MATCH_SETTINGS || nextScreen == SHELL_AREA_MODE_SELECT ||
				nextScreen == SHELL_AREA_BROWSER || nextScreen == SHELL_AREA_LOAD || nextScreen == SHELL_AREA_SAVE || nextScreen == SHELL_AREA_CREDITS )
		{
			ShowMPFrame( true );
		}
	}

	if( smallFrameShowing || largeFrameShowing || nextScreen == SHELL_AREA_START )
	{
		ShowLogo( false );
	}
	else
	{
		ShowLogo( true );
	}

}

/*
========================
idMenuHandler_Shell::UpdateLeaderboard
========================
*/
void idMenuHandler_Shell::UpdateLeaderboard( const idLeaderboardCallback* callback )
{
	idMenuScreen_Shell_Leaderboards* screen = dynamic_cast< idMenuScreen_Shell_Leaderboards* >( menuScreens[ SHELL_AREA_LEADERBOARDS ] );
	if( screen != NULL )
	{
		screen->UpdateLeaderboard( callback );
	}
}

/*
========================
idMenuManager_Shell::ShowPacifier
========================
*/
void idMenuHandler_Shell::ShowPacifier( const idStr& msg )
{
	if( GetPacifier() != NULL && gui != NULL )
	{
		gui->SetGlobal( "paciferMessage", msg );
		GetPacifier()->Show();
	}
}

/*
========================
idMenuManager_Shell::HidePacifier
========================
*/
void idMenuHandler_Shell::HidePacifier()
{
	if( GetPacifier() != NULL )
	{
		GetPacifier()->Hide();
	}
}

/*
========================
idMenuHandler_Shell::CopySettingsFromSession
========================
*/
void idMenuHandler_Shell::UpdateLobby( idMenuWidget_LobbyList* lobbyList )
{

	if( lobbyList == NULL )
	{
		return;
	}

	idLobbyBase& lobby = session->GetActivePlatformLobbyBase();
	const int numLobbyPlayers = lobby.GetNumLobbyUsers();
	int maxPlayers = session->GetTitleStorageInt( "MAX_PLAYERS_ALLOWED", 4 );

	idStaticList< lobbyPlayerInfo_t, MAX_PLAYERS > lobbyPlayers;
	for( int i = 0; i < numLobbyPlayers; ++i )
	{
		lobbyPlayerInfo_t* lobbyPlayer = lobbyPlayers.Alloc();

		lobbyUserID_t lobbyUserID = lobby.GetLobbyUserIdByOrdinal( i );

		if( !lobbyUserID.IsValid() )
		{
			continue;
		}

		lobbyPlayer->name = lobby.GetLobbyUserName( lobbyUserID );
		// Voice
		lobbyPlayer->voiceState = session->GetDisplayStateFromVoiceState( session->GetLobbyUserVoiceState( lobbyUserID ) );
	}


	for( int i = 0; i < maxPlayers; ++i )
	{
		if( i >= lobbyPlayers.Num() )
		{
			lobbyList->SetEntryData( i, "", VOICECHAT_DISPLAY_NONE );
		}
		else
		{
			lobbyPlayerInfo_t& lobbyPlayer = lobbyPlayers[ i ];
			lobbyList->SetEntryData( i, lobbyPlayer.name, lobbyPlayer.voiceState );
		}
	}

	lobbyList->SetNumEntries( lobbyPlayers.Num() );

}

/*
========================
idMenuHandler_Shell::StartGame
========================
*/
void idMenuHandler_Shell::StartGame( int index )
{
	if( index == 0 )
	{
		cmdSystem->AppendCommandText( va( "map %s %d\n", "game/mars_city1", 0 ) );
	}
	else if( index == 1 )
	{
		cmdSystem->AppendCommandText( va( "map %s %d\n", "game/erebus1", 1 ) );
	}
	else if( index == 2 )
	{
		cmdSystem->AppendCommandText( va( "map %s %d\n", "game/le_enpro1", 2 ) );
	}
}

static const int NUM_DOOM_INTRO_LINES = 7;
/*
========================
idMenuHandler_Shell::ShowIntroVideo
========================
*/
void idMenuHandler_Shell::ShowDoomIntro()
{

	StopSound();

	showingIntro = true;

	delete introGui;
	introGui = new idSWF( "doomIntro", common->MenuSW() );

	if( introGui != NULL )
	{
		const idMaterial* mat = doom3Intro;
		if( mat != NULL )
		{
			int c = mat->GetNumStages();
			for( int i = 0; i < c; i++ )
			{
				const shaderStage_t* stage = mat->GetStage( i );
				if( stage != NULL && stage->texture.cinematic )
				{
					stage->texture.cinematic->ResetTime( Sys_Milliseconds() );
				}
			}
		}

		introGui->Activate( true );

		int numTextFields = NUM_DOOM_INTRO_LINES;
		idStr textEntries[NUM_DOOM_INTRO_LINES] = { va( "%s %s", idLocalization::GetString( "#str_04052" ), idLocalization::GetString( "#str_04053" ) ),
													va( "%s %s", idLocalization::GetString( "#str_04054" ), idLocalization::GetString( "#str_04055" ) ),
													idLocalization::GetString( "#str_03012" ),
													idLocalization::GetString( "#str_04056" ),
													idLocalization::GetString( "#str_04057" ),
													va( "%s %s", idLocalization::GetString( "#str_04058" ), idLocalization::GetString( "#str_04059" ) ),
													va( "%s %s", idLocalization::GetString( "#str_04060" ), idLocalization::GetString( "#str_04061" ) )
												  };

		for( int i = 0; i < numTextFields; ++i )
		{

			idSWFTextInstance* txtVal = introGui->GetRootObject().GetNestedText( va( "info%d", i ), "txtInfo", "txtVal" );
			if( txtVal != NULL )
			{
				txtVal->SetText( textEntries[i] );
				txtVal->SetStrokeInfo( true );
				txtVal->renderMode = SWF_TEXT_RENDER_PARAGRAPH;
				txtVal->rndSpotsVisible = -1;
				txtVal->renderDelay = 50;
				txtVal->generatingText = false;
				if( typeSoundShader != NULL )
				{
					txtVal->soundClip = typeSoundShader->GetName();
				}
			}

			idSWFSpriteInstance* infoSprite = introGui->GetRootObject().GetNestedSprite( va( "info%d", i ) );
			if( infoSprite != NULL && txtVal != NULL )
			{
				class idIntroTextUpdate : public idSWFScriptFunction_RefCounted
				{
				public:
					idIntroTextUpdate( idSWFTextInstance* _txtVal, int _numLines, int _nextIndex, idMenuHandler_Shell* _shell, idSWF* _gui )
					{
						txtVal = _txtVal;
						generating = false;
						numLines = _numLines;
						nextIndex = _nextIndex;
						shell = _shell;
						gui = _gui;
					}
					idSWFScriptVar Call( idSWFScriptObject* thisObject, const idSWFParmList& parms )
					{
						if( thisObject->GetSprite() == NULL )
						{
							return idSWFScriptVar();
						}

						if( thisObject->GetSprite()->currentFrame == 1 )
						{
							return idSWFScriptVar();
						}

						if( txtVal == NULL )
						{
							return idSWFScriptVar();
						}

						if( !generating )
						{
							generating = true;
							txtVal->triggerGenerate = true;
						}
						else if( generating )
						{
							if( !txtVal->generatingText )
							{
								float newYPos = thisObject->GetSprite()->GetYPos() - 1.5f;
								if( newYPos <= 350.0f - ( numLines * 36.0f ) )
								{
									if( thisObject->GetSprite()->IsVisible() )
									{
										thisObject->GetSprite()->SetVisible( false );
										if( nextIndex >= NUM_DOOM_INTRO_LINES )
										{
											shell->StartGame( 0 );
										}
									}
								}
								else if( newYPos <= 665.0f - ( numLines * 36.0f ) )
								{
									if( nextIndex < NUM_DOOM_INTRO_LINES )
									{
										idSWFSpriteInstance* nextInfo = gui->GetRootObject().GetNestedSprite( va( "info%d", nextIndex ) );
										if( nextInfo != NULL && nextInfo->GetCurrentFrame() != nextInfo->FindFrame( "active" ) )
										{
											nextInfo->StopFrame( "active" );
										}
									}

									float alpha = 1.0f;
									if( newYPos <= 450 )
									{
										alpha = ( newYPos - 350.0f ) / 100.0f;
									}
									thisObject->GetSprite()->SetAlpha( alpha );
									thisObject->GetSprite()->SetYPos( newYPos );
								}
								else
								{
									thisObject->GetSprite()->SetYPos( newYPos );
									thisObject->GetSprite()->SetAlpha( 1.0f );
								}
							}
						}

						return idSWFScriptVar();
					}
				private:
					idSWFTextInstance* txtVal;
					idMenuHandler_Shell* shell;
					int	numLines;
					int nextIndex;
					bool generating;
					idSWF* gui;
				};

				infoSprite->GetScriptObject()->Set( "onEnterFrame", new idIntroTextUpdate( txtVal, txtVal->CalcNumLines(), i + 1, this, introGui ) );
			}
		}

		class idIntroVOStart : public idSWFScriptFunction_RefCounted
		{
		public:
			idIntroVOStart( idSWF* gui )
			{
				introGui = gui;
			}
			idSWFScriptVar Call( idSWFScriptObject* thisObject, const idSWFParmList& parms )
			{
				if( introGui != NULL )
				{
					introGui->PlaySound( "gui/doomintro" );
				}
				return idSWFScriptVar();
			}
		private:
			idSWF* introGui;
		};

		if( introGui != NULL )
		{
			introGui->SetGlobal( "playVo", new idIntroVOStart( introGui ) );
		}

		idSWFSpriteInstance* img = introGui->GetRootObject().GetNestedSprite( "intro", "img" );
		if( img != NULL )
		{
			if( mat != NULL )
			{
				img->SetMaterial( mat );
			}
		}
	}
}

static const int NUM_ROE_INTRO_LINES = 6;
/*
========================
idMenuHandler_Shell::ShowROEIntro
========================
*/
void idMenuHandler_Shell::ShowROEIntro()
{

	StopSound();

	showingIntro = true;

	delete introGui;
	introGui = new idSWF( "roeIntro", common->MenuSW() );

	if( introGui != NULL )
	{
		const idMaterial* mat = roeIntro;
		if( mat != NULL )
		{
			int c = mat->GetNumStages();
			for( int i = 0; i < c; i++ )
			{
				const shaderStage_t* stage = mat->GetStage( i );
				if( stage != NULL && stage->texture.cinematic )
				{
					stage->texture.cinematic->ResetTime( Sys_Milliseconds() );
				}
			}
		}

		introGui->Activate( true );

		int numTextFields = NUM_ROE_INTRO_LINES;
		idStr textEntries[NUM_ROE_INTRO_LINES] =
		{
			idLocalization::GetString( "#str_00100870" ),
			idLocalization::GetString( "#str_00100854" ),
			idLocalization::GetString( "#str_00100879" ),
			idLocalization::GetString( "#str_00100855" ),
			idLocalization::GetString( "#str_00100890" ),
			idLocalization::GetString( "#str_00100856" ),
		};

		for( int i = 0; i < numTextFields; ++i )
		{

			idSWFTextInstance* txtVal = introGui->GetRootObject().GetNestedText( va( "info%d", i ), "txtInfo", "txtVal" );
			if( txtVal != NULL )
			{
				txtVal->SetText( textEntries[i] );
				txtVal->SetStrokeInfo( true );
				txtVal->renderMode = SWF_TEXT_RENDER_PARAGRAPH;
				txtVal->rndSpotsVisible = -1;
				txtVal->renderDelay = 40;
				txtVal->generatingText = false;
				if( typeSoundShader != NULL )
				{
					txtVal->soundClip = typeSoundShader->GetName();
				}
			}

			idSWFSpriteInstance* infoSprite = introGui->GetRootObject().GetNestedSprite( va( "info%d", i ) );
			if( infoSprite != NULL && txtVal != NULL )
			{
				class idIntroTextUpdate : public idSWFScriptFunction_RefCounted
				{
				public:
					idIntroTextUpdate( idSWFTextInstance* _txtVal, int _numLines, int _nextIndex, idMenuHandler_Shell* _shell, idSWF* _gui )
					{
						txtVal = _txtVal;
						generating = false;
						numLines = _numLines;
						nextIndex = _nextIndex;
						shell = _shell;
						gui = _gui;
						startFade = 0;
					}
					idSWFScriptVar Call( idSWFScriptObject* thisObject, const idSWFParmList& parms )
					{
						if( thisObject->GetSprite() == NULL )
						{
							return idSWFScriptVar();
						}

						if( thisObject->GetSprite()->currentFrame == 1 )
						{
							return idSWFScriptVar();
						}

						if( txtVal == NULL )
						{
							return idSWFScriptVar();
						}

						if( !generating )
						{
							generating = true;
							txtVal->triggerGenerate = true;
						}
						else if( generating )
						{
							if( !txtVal->generatingText && thisObject->GetSprite()->IsVisible() )
							{
								if( nextIndex % 2 != 0 )
								{
									if( nextIndex < NUM_ROE_INTRO_LINES )
									{
										idSWFSpriteInstance* nextInfo = gui->GetRootObject().GetNestedSprite( va( "info%d", nextIndex ) );
										if( nextInfo != NULL && nextInfo->GetCurrentFrame() != nextInfo->FindFrame( "active" ) )
										{
											nextInfo->StopFrame( "active" );
										}
										else if( nextInfo != NULL && nextInfo->IsVisible() )
										{
											idSWFTextInstance* txtData = nextInfo->GetScriptObject()->GetNestedText( "txtInfo", "txtVal" );
											if( txtData != NULL && !txtData->generatingText )
											{
												if( startFade == 0 )
												{
													startFade = Sys_Milliseconds();
												}
												else
												{
													if( Sys_Milliseconds() - startFade >= 3000 )
													{
														nextInfo->SetVisible( false );
														thisObject->GetSprite()->SetVisible( false );

														int nextDateIndex = ( nextIndex + 1 );
														if( nextDateIndex < NUM_ROE_INTRO_LINES )
														{
															idSWFSpriteInstance* nextInfo = gui->GetRootObject().GetNestedSprite( va( "info%d", nextDateIndex ) );
															if( nextInfo != NULL && nextInfo->GetCurrentFrame() != nextInfo->FindFrame( "active" ) )
															{
																nextInfo->StopFrame( "active" );
																return idSWFScriptVar();
															}
														}
														else
														{
															shell->StartGame( 1 );
															return idSWFScriptVar();
														}
													}
													else
													{
														float alpha = 1.0f - ( ( float )( Sys_Milliseconds() - startFade ) / 3000.0f );
														nextInfo->SetAlpha( alpha );
														thisObject->GetSprite()->SetAlpha( alpha );
													}
												}
											}
										}
									}
								}
							}
						}

						return idSWFScriptVar();
					}
				private:
					idSWFTextInstance* txtVal;
					idMenuHandler_Shell* shell;
					int	numLines;
					int nextIndex;
					bool generating;
					idSWF* gui;
					int startFade;
				};

				infoSprite->GetScriptObject()->Set( "onEnterFrame", new idIntroTextUpdate( txtVal, txtVal->CalcNumLines(), i + 1, this, introGui ) );
			}
		}

		idSWFSpriteInstance* img = introGui->GetRootObject().GetNestedSprite( "intro", "img" );
		if( img != NULL )
		{
			if( mat != NULL )
			{
				img->SetMaterial( mat );
			}
		}
	}
}

static const int NUM_LE_INTRO_LINES = 1;
/*
========================
idMenuHandler_Shell::ShowLEIntro
========================
*/
void idMenuHandler_Shell::ShowLEIntro()
{

	StopSound();

	showingIntro = true;

	delete introGui;
	introGui = new idSWF( "leIntro", common->MenuSW() );

	if( introGui != NULL )
	{
		introGui->Activate( true );

		idStr textEntry = va( "%s\n%s\n%s", idLocalization::GetString( "#str_00200071" ), idLocalization::GetString( "#str_00200072" ), idLocalization::GetString( "#str_00200073" ) );
		idSWFTextInstance* txtVal = introGui->GetRootObject().GetNestedText( "info0", "txtInfo", "txtVal" );
		if( txtVal != NULL )
		{
			txtVal->SetText( textEntry );
			txtVal->SetStrokeInfo( true );
			txtVal->renderMode = SWF_TEXT_RENDER_PARAGRAPH;
			txtVal->rndSpotsVisible = -1;
			txtVal->renderDelay = 60;
			txtVal->generatingText = false;
			if( typeSoundShader != NULL )
			{
				txtVal->soundClip = typeSoundShader->GetName();
			}
		}

		idSWFSpriteInstance* infoSprite = introGui->GetRootObject().GetNestedSprite( "info0" );
		if( infoSprite != NULL )
		{
			class idIntroTextUpdate : public idSWFScriptFunction_RefCounted
			{
			public:
				idIntroTextUpdate( idSWFTextInstance* _txtVal, idMenuHandler_Shell* _shell )
				{
					txtVal = _txtVal;
					generating = false;
					shell = _shell;
					startFade = 0;
				}
				idSWFScriptVar Call( idSWFScriptObject* thisObject, const idSWFParmList& parms )
				{
					if( thisObject->GetSprite() == NULL )
					{
						return idSWFScriptVar();
					}

					if( thisObject->GetSprite()->currentFrame == 1 )
					{
						return idSWFScriptVar();
					}

					if( txtVal == NULL )
					{
						return idSWFScriptVar();
					}

					if( !generating )
					{
						generating = true;
						txtVal->triggerGenerate = true;
					}
					else if( generating )
					{
						if( !txtVal->generatingText )
						{
							if( startFade == 0 )
							{
								startFade = Sys_Milliseconds();
							}
							else
							{
								float alpha = 1.0f - ( ( float )( Sys_Milliseconds() - startFade ) / 3000.0f );
								if( alpha <= 0.0f )
								{
									thisObject->GetSprite()->SetVisible( false );
									shell->StartGame( 2 );
									return idSWFScriptVar();
								}
								thisObject->GetSprite()->SetAlpha( alpha );
							}
						}
					}

					return idSWFScriptVar();
				}
			private:
				idSWFTextInstance* txtVal;
				idMenuHandler_Shell* shell;
				bool generating;
				int startFade;
			};

			infoSprite->GetScriptObject()->Set( "onEnterFrame", new idIntroTextUpdate( txtVal, this ) );
		}
	}
}