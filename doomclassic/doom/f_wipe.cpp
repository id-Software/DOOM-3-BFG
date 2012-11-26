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

#include "z_zone.h"
#include "i_video.h"
#include "i_system.h"
#include "v_video.h"
#include "m_random.h"
#include "doomdef.h"
#include "f_wipe.h"

//
//                       SCREEN WIPE PACKAGE
//

// when zero, stop the wipe


void
wipe_shittyColMajorXform
( short*	array,
  int		width,
  int		height )
{
    int		x;
    int		y;
    short*	dest;

    //dest = (short*) DoomLib::Z_Malloc(width*height*2, PU_STATIC, 0 );
	dest = new short[ width * height ];

    for(y=0;y<height;y++)
		for(x=0;x<width;x++)
			dest[x*height+y] = array[y*width+x];

    memcpy(array, dest, width*height*2);

    //Z_Free(dest);
	delete[] dest;
}


int
wipe_initMelt
( int	width,
  int	height,
  int	ticks )
{
    int i, r;
    
    // copy start screen to main screen
    memcpy(::g->wipe_scr, ::g->wipe_scr_start, width*height);
    
    // makes this wipe faster (in theory)
    // to have stuff in column-major format
    wipe_shittyColMajorXform((short*)::g->wipe_scr_start, width/2, height);
    wipe_shittyColMajorXform((short*)::g->wipe_scr_end, width/2, height);
    
    // setup initial column positions
    // (::g->wipe_y<0 => not ready to scroll yet)
    ::g->wipe_y = (int *) DoomLib::Z_Malloc(width*sizeof(int), PU_STATIC, 0);

    ::g->wipe_y[0] = -(M_Random()%16);

	for (i=1;i<width;i++)
	{
		r = (M_Random()%3) - 1;

		::g->wipe_y[i] = ::g->wipe_y[i-1] + r;

		if (::g->wipe_y[i] > 0)
			::g->wipe_y[i] = 0;
		else if (::g->wipe_y[i] == -16)
			::g->wipe_y[i] = -15;
	}

    return 0;
}

int wipe_doMelt( int width, int height, int ticks ) {
	int		i;
	int		j;
	int		dy;
	int		idx;

	short*	s;
	short*	d;
	qboolean	done = true;

	width/=2;

	while (ticks--)
	{
		for (i=0;i<width;i++)
		{
			if (::g->wipe_y[i]<0) {

				::g->wipe_y[i]++;
				done = false;
			}
			else if (::g->wipe_y[i] < height) {

				dy = (::g->wipe_y[i] < 16 * GLOBAL_IMAGE_SCALER) ? ::g->wipe_y[i]+1 : 8 * GLOBAL_IMAGE_SCALER;

				if (::g->wipe_y[i]+dy >= height)
					dy = height - ::g->wipe_y[i];

				s = &((short *)::g->wipe_scr_end)[i*height+::g->wipe_y[i]];
				d = &((short *)::g->wipe_scr)[::g->wipe_y[i]*width+i];

				idx = 0;
				for (j=dy;j;j--)
				{
					d[idx] = *(s++);
					idx += width;
				}

				::g->wipe_y[i] += dy;

				s = &((short *)::g->wipe_scr_start)[i*height];
				d = &((short *)::g->wipe_scr)[::g->wipe_y[i]*width+i];

				idx = 0;
				for (j=height-::g->wipe_y[i];j;j--)
				{
					d[idx] = *(s++);
					idx += width;
				}

				done = false;
			}
		}
	}

	return done;
}

int
wipe_exitMelt
( int	width,
  int	height,
  int	ticks )
{
    Z_Free(::g->wipe_y);
	::g->wipe_y = NULL;
    return 0;
}

int
wipe_StartScreen
( int	x,
  int	y,
  int	width,
  int	height )
{
    ::g->wipe_scr_start = ::g->screens[2];
    I_ReadScreen(::g->wipe_scr_start);
    return 0;
}

int
wipe_EndScreen
( int	x,
  int	y,
  int	width,
  int	height )
{
    ::g->wipe_scr_end = ::g->screens[3];
    I_ReadScreen(::g->wipe_scr_end);
    V_DrawBlock(x, y, 0, width, height, ::g->wipe_scr_start); // restore start scr.
    return 0;
}

int
wipe_ScreenWipe
( int	x,
  int	y,
  int	width,
  int	height,
  int	ticks )
{
	int rc;

	// initial stuff
	if (!::g->go)
	{
		::g->go = 1;
		::g->wipe_scr = ::g->screens[0];

		wipe_initMelt(width, height, ticks);
	}

	// do a piece of wipe-in
	V_MarkRect(0, 0, width, height);

	rc = wipe_doMelt(width, height, ticks);

	// final stuff
	if (rc)
	{
		::g->go = 0;
		wipe_exitMelt(width, height, ticks);
	}

	return !::g->go;
}

