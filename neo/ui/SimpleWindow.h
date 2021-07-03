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

#ifndef __SIMPLEWIN_H__
#define __SIMPLEWIN_H__

class idUserInterfaceLocal;
class idDeviceContext;
class idSimpleWindow;

typedef struct
{
	idWindow* win;
	idSimpleWindow* simp;
} drawWin_t;

class idSimpleWindow
{
	friend class idWindow;
public:
	idSimpleWindow( idWindow* win );
	virtual			~idSimpleWindow();
	void			Redraw( float x, float y );
	void			StateChanged( bool redraw );

	idStr			name;

	idWinVar* 		GetWinVarByName( const char* _name );
	int				GetWinVarOffset( idWinVar* wv, drawWin_t* owner );
	size_t			Size();

	idWindow*		GetParent()
	{
		return mParent;
	}

	virtual void	WriteToSaveGame( idFile* savefile );
	virtual void	ReadFromSaveGame( idFile* savefile );

protected:
	void 			CalcClientRect( float xofs, float yofs );
	void 			SetupTransforms( float x, float y );
	void 			DrawBackground( const idRectangle& drawRect );
	void 			DrawBorderAndCaption( const idRectangle& drawRect );

	idUserInterfaceLocal* gui;
	int 			flags;
	idRectangle 	drawRect;			// overall rect
	idRectangle 	clientRect;			// client area
	idRectangle 	textRect;
	idVec2			origin;
	class idFont* 	font;
	float 			matScalex;
	float 			matScaley;
	float 			borderSize;
	int 			textAlign;
	float 			textAlignx;
	float 			textAligny;
	int				textShadow;

	idWinStr		text;
	idWinBool		visible;
	idWinRectangle	rect;				// overall rect
	idWinVec4		backColor;
	idWinVec4		matColor;
	idWinVec4		foreColor;
	idWinVec4		borderColor;
	idWinFloat		textScale;
	idWinFloat		rotate;
	idWinVec2		shear;
	idWinBackground	backGroundName;

	const idMaterial* background;

	idWindow* 		mParent;

	idWinBool	hideCursor;
};

#endif /* !__SIMPLEWIN_H__ */
