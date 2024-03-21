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

#ifndef __USERINTERFACE_H__
#define __USERINTERFACE_H__

/*
===============================================================================

	Draws an interactive 2D surface.
	Used for all user interaction with the game.

===============================================================================
*/

class idFile;


class idUserInterface
{
public:
	virtual						~idUserInterface() {};

	// Returns the name of the gui.
	virtual const char* 		Name() const = 0;

	// Returns a comment on the gui.
	virtual const char* 		Comment() const = 0;

	// Returns true if the gui is interactive.
	virtual bool				IsInteractive() const = 0;

	virtual bool				IsUniqued() const = 0;

	virtual void				SetUniqued( bool b ) = 0;
	// returns false if it failed to load
	virtual bool				InitFromFile( const char* qpath, bool rebuild = true, bool cache = true ) = 0;

	// handles an event, can return an action string, the caller interprets
	// any return and acts accordingly
	virtual const char* 		HandleEvent( const sysEvent_t* event, int time, bool* updateVisuals = NULL ) = 0;

	// handles a named event
	virtual void				HandleNamedEvent( const char* eventName ) = 0;

	// repaints the ui
	virtual void				Redraw( int time, bool hud = false ) = 0;

	// repaints the cursor
	virtual void				DrawCursor() = 0;

	// Provides read access to the idDict that holds this gui's state.
	virtual const idDict& 		State() const = 0;

	// Removes a gui state variable
	virtual void				DeleteStateVar( const char* varName ) = 0;

	// Sets a gui state variable.
	virtual void				SetStateString( const char* varName, const char* value ) = 0;
	virtual void				SetStateBool( const char* varName, const bool value ) = 0;
	virtual void				SetStateInt( const char* varName, const int value ) = 0;
	virtual void				SetStateFloat( const char* varName, const float value ) = 0;

	// Gets a gui state variable
	virtual const char*			GetStateString( const char* varName, const char* defaultString = "" ) const = 0;
	virtual bool				GetStateBool( const char* varName, const char* defaultString = "0" ) const  = 0;
	virtual int					GetStateInt( const char* varName, const char* defaultString = "0" ) const = 0;
	virtual float				GetStateFloat( const char* varName, const char* defaultString = "0" ) const = 0;

	// The state has changed and the gui needs to update from the state idDict.
	virtual void				StateChanged( int time, bool redraw = false ) = 0;

	// Activated the gui.
	virtual const char* 		Activate( bool activate, int time ) = 0;

	// Triggers the gui and runs the onTrigger scripts.
	virtual void				Trigger( int time ) = 0;

	virtual bool				WriteToSaveGame( idFile* savefile ) const = 0;
	virtual bool				ReadFromSaveGame( idFile* savefile ) = 0;
	virtual void				SetKeyBindingNames() = 0;

	virtual void				SetCursor( float x, float y ) = 0;
	virtual float				CursorX() = 0;
	virtual float				CursorY() = 0;
};


class idUserInterfaceManager
{
public:
	virtual						~idUserInterfaceManager() {};

	virtual void				Init() = 0;
	virtual void				Shutdown() = 0;
	virtual void				Touch( const char* name ) = 0;
	virtual void				WritePrecacheCommands( idFile* f ) = 0;

	// use either the optimized or legacy implementation for
	// testing, based on the g_useNewGuiCode cvar
	virtual void				SetDrawingDC() = 0;

	// Sets the size for 640x480 adjustment.
	virtual void				SetSize( float width, float height ) = 0;

	virtual void				BeginLevelLoad() = 0;
	virtual void				EndLevelLoad( const char* mapName ) = 0;
	virtual void				Preload( const char* mapName ) = 0;

	// Reloads changed guis, or all guis.
	virtual void				Reload( bool all ) = 0;

	// lists all guis
	virtual void				ListGuis() const = 0;

	// Returns true if gui exists.
	virtual bool				CheckGui( const char* qpath ) const = 0;

	// Allocates a new gui.
	virtual idUserInterface* 	Alloc() const = 0;

	// De-allocates a gui.. ONLY USE FOR PRECACHING
	virtual void				DeAlloc( idUserInterface* gui ) = 0;

	// Returns NULL if gui by that name does not exist.
	virtual idUserInterface* 	FindGui( const char* qpath, bool autoLoad = false, bool needUnique = false, bool forceUnique = false ) = 0;

	// Returns NULL if gui by that name does not exist.
	virtual idUserInterface* 	FindDemoGui( const char* qpath ) = 0;

	// Allocates a new GUI list handler
	virtual	idListGUI* 			AllocListGUI() const = 0;

	// De-allocates a list gui
	virtual void				FreeListGUI( idListGUI* listgui ) = 0;
};

extern idUserInterfaceManager* 	uiManager;

#endif /* !__USERINTERFACE_H__ */
