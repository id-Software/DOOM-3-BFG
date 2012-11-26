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

#ifndef __HULIB__
#define __HULIB__

// We are referring to patches.
#include "r_defs.h"

// font stuff
#define HU_CHARERASE	KEY_BACKSPACE

#define HU_MAXLINES		4
#define HU_MAXLINELENGTH	80

//
// Typedefs of widgets
//

// Text Line widget
//  (parent of Scrolling Text and Input Text widgets)
typedef struct
{
    // left-justified position of scrolling text window
    int		x;
    int		y;
    
    patch_t**	f;			// font
    int		sc;			// start character
    char	l[HU_MAXLINELENGTH+1];	// line of text
    int		len;		      	// current line length

    // whether this line needs to be udpated
    int		needsupdate;	      

} hu_textline_t;



// Scrolling Text window widget
//  (child of Text Line widget)
typedef struct
{
    hu_textline_t	l[HU_MAXLINES];	// text lines to draw
    int			h;		// height in lines
    int			cl;		// current line number

    // pointer to qboolean stating whether to update window
    qboolean*		on;
    qboolean		laston;		// last value of *->on.

} hu_stext_t;



// Input Text Line widget
//  (child of Text Line widget)
typedef struct
{
    hu_textline_t	l;		// text line to input on

     // left margin past which I am not to delete characters
    int			lm;

    // pointer to qboolean stating whether to update window
    qboolean*		on; 
    qboolean		laston; // last value of *->on;

} hu_itext_t;


//
// Widget creation, access, and update routines
//

// initializes heads-up widget library
void HUlib_init(void);

//
// textline code
//

// clear a line of text
void	HUlib_clearTextLine(hu_textline_t *t);

void	HUlib_initTextLine(hu_textline_t *t, int x, int y, patch_t **f, int sc);

// returns success
qboolean HUlib_addCharToTextLine(hu_textline_t *t, char ch);

// returns success
qboolean HUlib_delCharFromTextLine(hu_textline_t *t);

// draws tline
void	HUlib_drawTextLine(hu_textline_t *l, qboolean drawcursor);

// erases text line
void	HUlib_eraseTextLine(hu_textline_t *l); 


//
// Scrolling Text window widget routines
//

// ?
void
HUlib_initSText
( hu_stext_t*	s,
  int		x,
  int		y,
  int		h,
  patch_t**	font,
  int		startchar,
  qboolean*	on );

// add a new line
void HUlib_addLineToSText(hu_stext_t* s);  

// ?
void
HUlib_addMessageToSText
( hu_stext_t*	s,
  const char*		prefix,
  const char*		msg );

// draws stext
void HUlib_drawSText(hu_stext_t* s);

// erases all stext lines
void HUlib_eraseSText(hu_stext_t* s); 

// Input Text Line widget routines
void
HUlib_initIText
( hu_itext_t*	it,
  int		x,
  int		y,
  patch_t**	font,
  int		startchar,
  qboolean*	on );

// enforces left margin
void HUlib_delCharFromIText(hu_itext_t* it);

// enforces left margin
void HUlib_eraseLineFromIText(hu_itext_t* it);

// resets line and left margin
void HUlib_resetIText(hu_itext_t* it);

// left of left-margin
void
HUlib_addPrefixToIText
( hu_itext_t*	it,
  char*		str );

// whether eaten
qboolean
HUlib_keyInIText
( hu_itext_t*	it,
  unsigned char ch );

void HUlib_drawIText(hu_itext_t* it);

// erases all itext lines
void HUlib_eraseIText(hu_itext_t* it); 

#endif

