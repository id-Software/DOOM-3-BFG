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

static const int TIP_DISPLAY_TIME = 5000;

/*
========================
idMenuHandler_HUD::Update
========================
*/
void idMenuHandler_HUD::Update()
{

	if( gui == NULL || !gui->IsActive() )
	{
		return;
	}

	if( nextScreen != activeScreen )
	{

		if( activeScreen > HUD_AREA_INVALID && activeScreen < HUD_NUM_AREAS && menuScreens[ activeScreen ] != NULL )
		{
			menuScreens[ activeScreen ]->HideScreen( static_cast<mainMenuTransition_t>( transition ) );
		}

		if( nextScreen > HUD_AREA_INVALID && nextScreen < HUD_NUM_AREAS && menuScreens[ nextScreen ] != NULL )
		{
			menuScreens[ nextScreen ]->ShowScreen( static_cast<mainMenuTransition_t>( transition ) );
		}

		transition = MENU_TRANSITION_INVALID;
		activeScreen = nextScreen;
	}

	idPlayer* player = gameLocal.GetLocalPlayer();
	if( player != NULL )
	{
		if( player->IsTipVisible() && autoHideTip && !hiding )
		{
			if( gameLocal.time >= tipStartTime + TIP_DISPLAY_TIME )
			{
				player->HideTip();
			}
		}

		if( player->IsSoundChannelPlaying( SND_CHANNEL_PDA_AUDIO ) && GetHud() != NULL )
		{
			GetHud()->UpdateAudioLog( true );
		}
		else
		{
			GetHud()->UpdateAudioLog( false );
		}

		if( radioMessage )
		{
			GetHud()->UpdateCommunication( true, player );
		}
		else
		{
			GetHud()->UpdateCommunication( false, player );
		}

	}

	idMenuHandler::Update();
}

/*
========================
idMenuHandler_HUD::ActivateMenu
========================
*/
void idMenuHandler_HUD::ActivateMenu( bool show )
{

	idMenuHandler::ActivateMenu( show );

	idPlayer* player = gameLocal.GetLocalPlayer();
	if( player == NULL )
	{
		return;
	}

	if( show )
	{
		activeScreen = HUD_AREA_INVALID;
		nextScreen = HUD_AREA_PLAYING;
	}
	else
	{
		activeScreen = HUD_AREA_INVALID;
		nextScreen = HUD_AREA_INVALID;
	}

}

/*
========================
idMenuHandler_HUD::Initialize
========================
*/
void idMenuHandler_HUD::Initialize( const char* swfFile, idSoundWorld* sw )
{
	idMenuHandler::Initialize( swfFile, sw );

	//---------------------
	// Initialize the menus
	//---------------------
#define BIND_HUD_SCREEN( screenId, className, menuHandler )				\
	menuScreens[ (screenId) ] = new className();						\
	menuScreens[ (screenId) ]->Initialize( menuHandler );				\
	menuScreens[ (screenId) ]->AddRef();

	for( int i = 0; i < HUD_NUM_AREAS; ++i )
	{
		menuScreens[ i ] = NULL;
	}

	BIND_HUD_SCREEN( HUD_AREA_PLAYING, idMenuScreen_HUD, this );
}

/*
========================
idMenuHandler_HUD::GetMenuScreen
========================
*/
idMenuScreen* idMenuHandler_HUD::GetMenuScreen( int index )
{

	if( index < 0 || index >= HUD_NUM_AREAS )
	{
		return NULL;
	}

	return menuScreens[ index ];

}

/*
========================
idMenuHandler_HUD::GetHud
========================
*/
idMenuScreen_HUD* idMenuHandler_HUD::GetHud()
{
	idMenuScreen_HUD* screen = dynamic_cast< idMenuScreen_HUD* >( menuScreens[ HUD_AREA_PLAYING ] );
	return screen;
}

/*
========================
idMenuHandler_HUD::ShowTip
========================
*/
void idMenuHandler_HUD::ShowTip( const char* title, const char* tip, bool autoHide )
{
	// SRS - Changed to assign autoHide to autoHideTip vs. assign autoHideTip to itself
	autoHideTip = autoHide;
	tipStartTime = gameLocal.time;
	hiding = false;
	idMenuScreen_HUD* screen = GetHud();
	if( screen != NULL )
	{
		screen->ShowTip( title, tip );
	}
}

/*
========================
idMenuHandler_HUD::HideTip
========================
*/
void idMenuHandler_HUD::HideTip()
{
	idMenuScreen_HUD* screen = GetHud();
	if( screen != NULL && !hiding )
	{
		screen->HideTip();
	}
	hiding = true;
}