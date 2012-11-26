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

#include "Precompiled.h"
#include "globaldata.h"

#include <ctype.h>

#include "doomdef.h"

#include "v_video.h"
#include "m_swap.h"

#include "hu_lib.h"
#include "r_local.h"
#include "r_draw.h"

// qboolean : whether the screen is always erased


void HUlib_init(void)
{
}

void HUlib_clearTextLine(hu_textline_t* t)
{
    t->len = 0;
    t->l[0] = 0;
    t->needsupdate = true;
}

void
HUlib_initTextLine
( hu_textline_t*	t,
  int			x,
  int			y,
  patch_t**		f,
  int			sc )
{
    t->x = x;
    t->y = y;
    t->f = f;
    t->sc = sc;
    HUlib_clearTextLine(t);
}

qboolean
HUlib_addCharToTextLine
( hu_textline_t*	t,
  char			ch )
{

    if (t->len == HU_MAXLINELENGTH)
	return false;
    else
    {
	t->l[t->len++] = ch;
	t->l[t->len] = 0;
	t->needsupdate = 4;
	return true;
    }

}

qboolean HUlib_delCharFromTextLine(hu_textline_t* t)
{

    if (!t->len) return false;
    else
    {
	t->l[--t->len] = 0;
	t->needsupdate = 4;
	return true;
    }

}

void
HUlib_drawTextLine
( hu_textline_t*	l,
  qboolean		drawcursor )
{

    int			i;
    int			w;
    int			x;
    unsigned char	c;

    // draw the new stuff
    x = l->x;
    for (i=0;i<l->len;i++)
    {
	c = toupper(l->l[i]);
	if (c != ' '
	    && c >= l->sc
	    && c <= '_')
	{
	    w = SHORT(l->f[c - l->sc]->width);
	    if (x+w > SCREENWIDTH)
		break;
	    V_DrawPatchDirect(x, l->y, FG, l->f[c - l->sc]);
	    x += w;
	}
	else
	{
	    x += 4;
	    if (x >= SCREENWIDTH)
		break;
	}
    }

    // draw the cursor if requested
    if (drawcursor
	&& x + SHORT(l->f['_' - l->sc]->width) <= SCREENWIDTH)
    {
	V_DrawPatchDirect(x, l->y, FG, l->f['_' - l->sc]);
    }
}


// sorta called by HU_Erase and just better darn get things straight
void HUlib_eraseTextLine(hu_textline_t* l)
{
    int			lh;
    int			y;
    int			yoffset;

    // Only erases when NOT in automap and the screen is reduced,
    // and the text must either need updating or refreshing
    // (because of a recent change back from the automap)

    if (!::g->automapactive &&
	::g->viewwindowx && l->needsupdate)
    {
	lh = SHORT(l->f[0]->height) + 1;
	for (y=l->y,yoffset=y*SCREENWIDTH ; y<l->y+lh ; y++,yoffset+=SCREENWIDTH)
	{
	    if (y < ::g->viewwindowy || y >= ::g->viewwindowy + ::g->viewheight)
		R_VideoErase(yoffset, SCREENWIDTH); // erase entire line
	    else
	    {
		R_VideoErase(yoffset, ::g->viewwindowx); // erase left border
		R_VideoErase(yoffset + ::g->viewwindowx + ::g->viewwidth, ::g->viewwindowx);
		// erase right border
	    }
	}
    }

    ::g->lastautomapactive = ::g->automapactive;
    if (l->needsupdate) l->needsupdate--;

}

void
HUlib_initSText
( hu_stext_t*	s,
  int		x,
  int		y,
  int		h,
  patch_t**	font,
  int		startchar,
  qboolean*	on )
{

    int i;

    s->h = h;
    s->on = on;
    s->laston = true;
    s->cl = 0;
    for (i=0;i<h;i++)
	HUlib_initTextLine(&s->l[i],
			   x, y - i*(SHORT(font[0]->height)+1),
			   font, startchar);

}

void HUlib_addLineToSText(hu_stext_t* s)
{

    int i;

    // add a clear line
    if (++s->cl == s->h)
	s->cl = 0;
    HUlib_clearTextLine(&s->l[s->cl]);

    // everything needs updating
    for (i=0 ; i<s->h ; i++)
	s->l[i].needsupdate = 4;

}

void
HUlib_addMessageToSText
( hu_stext_t*	s,
  const char*		prefix,
  const char*		msg )
{
    HUlib_addLineToSText(s);
    if (prefix)
	while (*prefix)
	    HUlib_addCharToTextLine(&s->l[s->cl], *(prefix++));

    while (*msg)
	HUlib_addCharToTextLine(&s->l[s->cl], *(msg++));
}

void HUlib_drawSText(hu_stext_t* s)
{
    int i, idx;
    hu_textline_t *l;

    if (!*s->on)
	return; // if not on, don't draw

    // draw everything
    for (i=0 ; i<s->h ; i++)
    {
	idx = s->cl - i;
	if (idx < 0)
	    idx += s->h; // handle queue of ::g->lines
	
	l = &s->l[idx];

	// need a decision made here on whether to skip the draw
	HUlib_drawTextLine(l, false); // no cursor, please
    }

}

void HUlib_eraseSText(hu_stext_t* s)
{

    int i;

    for (i=0 ; i<s->h ; i++)
    {
	if (s->laston && !*s->on)
	    s->l[i].needsupdate = 4;
	HUlib_eraseTextLine(&s->l[i]);
    }
    s->laston = *s->on;

}

void
HUlib_initIText
( hu_itext_t*	it,
  int		x,
  int		y,
  patch_t**	font,
  int		startchar,
  qboolean*	on )
{
    it->lm = 0; // default left margin is start of text
    it->on = on;
    it->laston = true;
    HUlib_initTextLine(&it->l, x, y, font, startchar);
}


// The following deletion routines adhere to the left margin restriction
void HUlib_delCharFromIText(hu_itext_t* it)
{
    if (it->l.len != it->lm)
	HUlib_delCharFromTextLine(&it->l);
}

void HUlib_eraseLineFromIText(hu_itext_t* it)
{
    while (it->lm != it->l.len)
	HUlib_delCharFromTextLine(&it->l);
}

// Resets left margin as well
void HUlib_resetIText(hu_itext_t* it)
{
    it->lm = 0;
    HUlib_clearTextLine(&it->l);
}

void
HUlib_addPrefixToIText
( hu_itext_t*	it,
  char*		str )
{
    while (*str)
	HUlib_addCharToTextLine(&it->l, *(str++));
    it->lm = it->l.len;
}

// wrapper function for handling general keyed input.
// returns true if it ate the key
qboolean
HUlib_keyInIText
( hu_itext_t*	it,
  unsigned char ch )
{

    if (ch >= ' ' && ch <= '_') 
  	HUlib_addCharToTextLine(&it->l, (char) ch);
    else 
	if (ch == KEY_BACKSPACE) 
	    HUlib_delCharFromIText(it);
	else 
	    if (ch != KEY_ENTER) 
		return false; // did not eat key

    return true; // ate the key

}

void HUlib_drawIText(hu_itext_t* it)
{

    hu_textline_t *l = &it->l;

    if (!*it->on)
	return;
    HUlib_drawTextLine(l, true); // draw the line w/ cursor

}

void HUlib_eraseIText(hu_itext_t* it)
{
    if (it->laston && !*it->on)
	it->l.needsupdate = 4;
    HUlib_eraseTextLine(&it->l);
    it->laston = *it->on;
}


