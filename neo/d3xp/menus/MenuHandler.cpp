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

extern idCVar in_useJoystick;

/*
================================================
idMenuHandler::~idMenuHandler
================================================
*/
idMenuHandler::idMenuHandler()
{
	scrollingMenu = false;
	scrollCounter = 0;
	activeScreen = -1;
	nextScreen = -1;
	transition = -1;
	platform = 0;
	gui = NULL;
	cmdBar = NULL;

	for( int index = 0; index < MAX_SCREEN_AREAS; ++index )
	{
		menuScreens[ index ] = NULL;
	}

	sounds.SetNum( NUM_GUI_SOUNDS );

}

/*
================================================
idMenuHandler::~idMenuHandler
================================================
*/
idMenuHandler::~idMenuHandler()
{
	Cleanup();
}

/*
================================================
idMenuHandler::Initialize
================================================
*/
void idMenuHandler::Initialize( const char* swfFile, idSoundWorld* sw )
{
	Cleanup();
	gui = new idSWF( swfFile, sw );

	platform = 2;

}

/*
================================================
idMenuHandler::AddChild
================================================
*/
void idMenuHandler::AddChild( idMenuWidget* widget )
{
	widget->SetSWFObj( gui );
	widget->SetHandlerIsParent( true );
	children.Append( widget );
	widget->AddRef();
}

/*
================================================
idMenuHandler::GetChildFromIndex
================================================
*/
idMenuWidget* idMenuHandler::GetChildFromIndex( int index )
{

	if( children.Num() == 0 )
	{
		return NULL;
	}

	if( index > children.Num() )
	{
		return NULL;
	}

	return children[ index ];
}

/*
================================================
idMenuHandler::GetPlatform
================================================
*/
int idMenuHandler::GetPlatform( bool realPlatform )
{

	if( platform == 2 && in_useJoystick.GetBool() && !realPlatform )
	{
		return 0;
	}

	return platform;
}

/*
================================================
idMenuHandler::GetPlatform
================================================
*/
void idMenuHandler::PlaySound( menuSounds_t type, int channel )
{

	if( gui == NULL )
	{
		return;
	}

	if( type >= sounds.Num() )
	{
		return;
	}

	if( sounds[ type ].IsEmpty() )
	{
		return;
	}

	int c = SCHANNEL_ANY;
	if( channel != -1 )
	{
		c = channel;
	}

	gui->PlaySound( sounds[ type ], c );

}

/*
================================================
idMenuHandler::StopSound
================================================
*/
void idMenuHandler::StopSound( int channel )
{
	gui->StopSound();
}

/*
================================================
idMenuHandler::Cleanup
================================================
*/
void idMenuHandler::Cleanup()
{
	for( int index = 0; index < children.Num(); ++index )
	{
		assert( children[ index ]->GetRefCount() > 0 );
		children[ index ]->Release();
	}
	children.Clear();

	for( int index = 0; index < MAX_SCREEN_AREAS; ++index )
	{
		if( menuScreens[ index ] != NULL )
		{
			menuScreens[ index ]->Release();
		}
	}

	delete gui;
	gui = NULL;
}

/*
================================================
idMenuHandler::TriggerMenu
================================================
*/
void idMenuHandler::TriggerMenu()
{

}

/*
================================================
idMenuHandler::IsActive
================================================
*/
bool idMenuHandler::IsActive()
{
	if( gui == NULL )
	{
		return false;
	}

	return gui->IsActive();
}

/*
================================================
idMenuHandler::ActivateMenu
================================================
*/
void idMenuHandler::ActivateMenu( bool show )
{

	if( gui == NULL )
	{
		return;
	}

	if( !show )
	{
		gui->Activate( show );
		return;
	}


	class idSWFScriptFunction_updateMenuDisplay : public idSWFScriptFunction_RefCounted
	{
	public:
		idSWFScriptFunction_updateMenuDisplay( idSWF* _gui, idMenuHandler* _handler )
		{
			gui = _gui;
			handler = _handler;
		}
		idSWFScriptVar Call( idSWFScriptObject* thisObject, const idSWFParmList& parms )
		{
			if( handler != NULL )
			{
				int screen = parms[0].ToInteger();
				handler->UpdateMenuDisplay( screen );
			}

			return idSWFScriptVar();
		}
	private:
		idSWF* gui;
		idMenuHandler* handler;
	};

	class idSWFScriptFunction_activateMenu : public idSWFScriptFunction_RefCounted
	{
	public:
		idSWFScriptFunction_activateMenu( idMenuHandler* _handler )
		{
			handler = _handler;
		}
		idSWFScriptVar Call( idSWFScriptObject* thisObject, const idSWFParmList& parms )
		{
			if( handler != NULL )
			{
				handler->TriggerMenu();
			}

			return idSWFScriptVar();
		}
	private:
		idMenuHandler* handler;
	};

	gui->SetGlobal( "updateMenuDisplay", new( TAG_SWF ) idSWFScriptFunction_updateMenuDisplay( gui, this ) );
	gui->SetGlobal( "activateMenus", new( TAG_SWF ) idSWFScriptFunction_activateMenu( this ) );

	gui->Activate( show );
}

/*
================================================
idMenuHandler::Update
================================================
*/
void idMenuHandler::Update()
{

	PumpWidgetActionRepeater();

	if( gui != NULL && gui->IsActive() )
	{
		gui->Render( renderSystem, Sys_Milliseconds() );
	}
}

/*
================================================
idMenuHandler::UpdateChildren
================================================
*/
void idMenuHandler::UpdateChildren()
{
	for( int index = 0; index < children.Num(); ++index )
	{
		if( children[ index ] != NULL )
		{
			children[index]->Update();
		}
	}
}

/*
================================================
idMenuHandler::UpdateMenuDisplay
================================================
*/
void idMenuHandler::UpdateMenuDisplay( int menu )
{

	if( menuScreens[ menu ] != NULL )
	{
		menuScreens[ menu ]->Update();
	}

	UpdateChildren();

}

/*
================================================
idMenuHandler::Update
================================================
*/
bool idMenuHandler::HandleGuiEvent( const sysEvent_t* sev )
{

	if( gui != NULL && activeScreen != -1 )
	{
		return gui->HandleEvent( sev );
	}

	return false;
}

/*
================================================
idMenuHandler::Update
================================================
*/
bool idMenuHandler::HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled )
{

	widgetAction_t actionType = action.GetType();
	const idSWFParmList& parms = action.GetParms();

	switch( actionType )
	{
		case WIDGET_ACTION_ADJUST_FIELD:
		{
			if( widget != NULL && widget->GetDataSource() != NULL )
			{
				widget->GetDataSource()->AdjustField( widget->GetDataSourceFieldIndex(), parms[ 0 ].ToInteger() );
				widget->Update();
			}
			return true;
		}
		case WIDGET_ACTION_FUNCTION:
		{
			if( verify( action.GetScriptFunction() != NULL ) )
			{
				action.GetScriptFunction()->Call( event.thisObject, event.parms );
			}
			return true;
		}
		case WIDGET_ACTION_PRESS_FOCUSED:
		{
			idMenuScreen* const screen = menuScreens[ activeScreen ];
			if( screen != NULL )
			{
				idWidgetEvent pressEvent( WIDGET_EVENT_PRESS, 0, event.thisObject, idSWFParmList() );
				screen->ReceiveEvent( pressEvent );
			}
			return true;
		}
		case WIDGET_ACTION_START_REPEATER:
		{
			idWidgetAction repeatAction;
			widgetAction_t repeatActionType = static_cast< widgetAction_t >( parms[ 0 ].ToInteger() );
			assert( parms.Num() >= 2 );
			int repeatDelay = DEFAULT_REPEAT_TIME;
			if( parms.Num() >= 3 )
			{
				repeatDelay = parms[2].ToInteger();
			}
			repeatAction.Set( repeatActionType, parms[ 1 ], repeatDelay );
			StartWidgetActionRepeater( widget, repeatAction, event );
			return true;
		}
		case WIDGET_ACTION_STOP_REPEATER:
		{
			ClearWidgetActionRepeater();
			return true;
		}
	}

	if( !widget->GetHandlerIsParent() )
	{
		for( int index = 0; index < children.Num(); ++index )
		{
			if( children[index] != NULL )
			{
				if( children[index]->HandleAction( action, event, widget, forceHandled ) )
				{
					return true;
				}
			}
		}
	}

	return false;
}

/*
========================
idMenuHandler::StartWidgetActionRepeater
========================
*/
void idMenuHandler::StartWidgetActionRepeater( idMenuWidget* widget, const idWidgetAction& action, const idWidgetEvent& event )
{
	if( actionRepeater.isActive && actionRepeater.action == action )
	{
		return;	// don't attempt to reactivate an already active repeater
	}

	actionRepeater.isActive = true;
	actionRepeater.action = action;
	actionRepeater.widget = widget;
	actionRepeater.event = event;
	actionRepeater.numRepetitions = 0;
	actionRepeater.nextRepeatTime = 0;
	actionRepeater.screenIndex = activeScreen;	// repeaters are cleared between screens

	if( action.GetParms().Num() == 2 )
	{
		actionRepeater.repeatDelay = action.GetParms()[ 1 ].ToInteger();
	}
	else
	{
		actionRepeater.repeatDelay = DEFAULT_REPEAT_TIME;
	}

	// do the first event immediately
	PumpWidgetActionRepeater();
}

/*
========================
idMenuHandler::PumpWidgetActionRepeater
========================
*/
void idMenuHandler::PumpWidgetActionRepeater()
{
	if( !actionRepeater.isActive )
	{
		return;
	}

	if( activeScreen != actionRepeater.screenIndex || nextScreen != activeScreen )    // || common->IsDialogActive() ) {
	{
		actionRepeater.isActive = false;
		return;
	}

	if( actionRepeater.nextRepeatTime > Sys_Milliseconds() )
	{
		return;
	}

	// need to hold down longer on the first iteration before we continue to scroll
	if( actionRepeater.numRepetitions == 0 )
	{
		actionRepeater.nextRepeatTime = Sys_Milliseconds() + 400;
	}
	else
	{
		actionRepeater.nextRepeatTime = Sys_Milliseconds() + actionRepeater.repeatDelay;
	}

	if( verify( actionRepeater.widget != NULL ) )
	{
		actionRepeater.widget->HandleAction( actionRepeater.action, actionRepeater.event, actionRepeater.widget );
		actionRepeater.numRepetitions++;
	}
}

/*
========================
idMenuHandler::ClearWidgetActionRepeater
========================
*/
void idMenuHandler::ClearWidgetActionRepeater()
{
	actionRepeater.isActive = false;
}