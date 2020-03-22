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

#ifndef __EDITWINDOW_H__
#define __EDITWINDOW_H__

#include "Window.h"

const int MAX_EDITFIELD = 4096;

class idUserInterfaceLocal;
class idSliderWindow;

class idEditWindow : public idWindow
{
public:
	idEditWindow( idUserInterfaceLocal* gui );
	virtual 			~idEditWindow();

	virtual void		Draw( int time, float x, float y );
	virtual const char* HandleEvent( const sysEvent_t* event, bool* updateVisuals );
	virtual void		PostParse();
	virtual void		GainFocus();
	virtual size_t		Allocated()
	{
		return idWindow::Allocated();
	};

	virtual idWinVar* 	GetWinVarByName( const char* _name, bool winLookup = false, drawWin_t** owner = NULL );

	virtual void 		HandleBuddyUpdate( idWindow* buddy );
	virtual void		Activate( bool activate, idStr& act );

	void				RunNamedEvent( const char* eventName );

private:

	virtual bool		ParseInternalVar( const char* name, idTokenParser* src );

	void				InitCvar();
	// true: read the updated cvar from cvar system
	// false: write to the cvar system
	// force == true overrides liveUpdate 0
	void				UpdateCvar( bool read, bool force = false );

	void				CommonInit();
	void				EnsureCursorVisible();
	void				InitScroller( bool horizontal );

	int					maxChars;
	int					paintOffset;
	int					cursorPos;
	int					cursorLine;
	int					cvarMax;
	bool				wrap;
	bool				readonly;
	bool				numeric;
	idStr				sourceFile;
	idSliderWindow* 	scroller;
	idList<int>			breaks;
	float				sizeBias;
	int					textIndex;
	int					lastTextLength;
	bool				forceScroll;
	idWinBool			password;

	idWinStr			cvarStr;
	idCVar* 			cvar;

	idWinBool			liveUpdate;
	idWinStr			cvarGroup;
};

#endif /* !__EDITWINDOW_H__ */
