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

#ifndef __SLIDERWINDOW_H__
#define __SLIDERWINDOW_H__

class idUserInterfaceLocal;

class idSliderWindow : public idWindow
{
public:
	idSliderWindow( idUserInterfaceLocal* gui );
	virtual				~idSliderWindow();

	void				InitWithDefaults( const char* _name, const idRectangle& rect, const idVec4& foreColor, const idVec4& matColor, const char* _background, const char* thumbShader, bool _vertical, bool _scrollbar );

	void				SetRange( float _low, float _high, float _step );
	float				GetLow()
	{
		return low;
	}
	float				GetHigh()
	{
		return high;
	}

	void				SetValue( float _value );
	float				GetValue()
	{
		return value;
	};

	virtual size_t		Allocated()
	{
		return idWindow::Allocated();
	};
	virtual idWinVar* 	GetWinVarByName( const char* _name, bool winLookup = false, drawWin_t** owner = NULL );
	virtual const char* HandleEvent( const sysEvent_t* event, bool* updateVisuals );
	virtual void		PostParse();
	virtual void		Draw( int time, float x, float y );
	virtual void		DrawBackground( const idRectangle& drawRect );
	virtual const char* RouteMouseCoords( float xd, float yd );
	virtual void		Activate( bool activate, idStr& act );
	virtual void		SetBuddy( idWindow* buddy );

	void				RunNamedEvent( const char* eventName );

private:
	virtual bool		ParseInternalVar( const char* name, idTokenParser* src );
	void				CommonInit();
	void				InitCvar();
	// true: read the updated cvar from cvar system
	// false: write to the cvar system
	// force == true overrides liveUpdate 0
	void				UpdateCvar( bool read, bool force = false );

	idWinFloat			value;
	float				low;
	float				high;
	float				thumbWidth;
	float				thumbHeight;
	float				stepSize;
	float				lastValue;
	idRectangle			thumbRect;
	const idMaterial* 	thumbMat;
	bool				vertical;
	bool				verticalFlip;
	bool				scrollbar;
	idWindow* 			buddyWin;
	idStr				thumbShader;

	idWinStr			cvarStr;
	idCVar* 			cvar;
	bool				cvar_init;
	idWinBool			liveUpdate;
	idWinStr			cvarGroup;
};

#endif /* !__SLIDERWINDOW_H__ */

