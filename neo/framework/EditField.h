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

#ifndef __EDITFIELD_H__
#define __EDITFIELD_H__

/*
===============================================================================

	Edit field

===============================================================================
*/

const int MAX_EDIT_LINE = 256;

typedef struct autoComplete_s
{
	bool			valid;
	int				length;
	char			completionString[MAX_EDIT_LINE];
	char			currentMatch[MAX_EDIT_LINE];
	int				matchCount;
	int				matchIndex;
	int				findMatchIndex;
} autoComplete_t;

class idEditField
{
public:
	idEditField();
	~idEditField();
	
	void			Clear();
	void			SetWidthInChars( int w );
	void			SetCursor( int c );
	int				GetCursor() const;
	void			ClearAutoComplete();
	int				GetAutoCompleteLength() const;
	void			AutoComplete();
	void			CharEvent( int c );
	void			KeyDownEvent( int key );
	void			Paste();
	char* 			GetBuffer();
	void			Draw( int x, int y, int width, bool showCursor );
	void			SetBuffer( const char* buffer );
	
private:
	int				cursor;
	int				scroll;
	int				widthInChars;
	char			buffer[MAX_EDIT_LINE];
	autoComplete_t	autoComplete;
};

#endif /* !__EDITFIELD_H__ */
