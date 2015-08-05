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


#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>

#include <ctype.h>


#include "doomdef.h"
#include "g_game.h"
#include "z_zone.h"

#include "m_swap.h"
#include "m_argv.h"

#include "w_wad.h"

#include "i_system.h"
#include "i_video.h"
#include "v_video.h"

#include "hu_stuff.h"

// State.
#include "doomstat.h"

// Data.
#include "dstrings.h"

#include "m_misc.h"
#include "d3xp/Game_local.h"

//
// M_DrawText
// Returns the final X coordinate
// HU_Init must have been called to init the font
//

int
M_DrawText
( int		x,
  int		y,
  qboolean	direct,
  char*		string )
{
    int 	c;
    int		w;

    while (*string)
    {
	c = toupper(*string) - HU_FONTSTART;
	string++;
	if (c < 0 || c> HU_FONTSIZE)
	{
	    x += 4;
	    continue;
	}
		
	w = SHORT (::g->hu_font[c]->width);
	if (x+w > SCREENWIDTH)
	    break;
	if (direct)
	    V_DrawPatchDirect(x, y, 0, ::g->hu_font[c]);
	else
	    V_DrawPatch(x, y, 0, ::g->hu_font[c]);
	x+=w;
    }

    return x;
}


//
// M_WriteFile
//
bool M_WriteFile ( char const*	name, void*		source, int		length ) {
	
	idFile *		handle = NULL;
	int		count;

	handle = fileSystem->OpenFileWrite( name, "fs_savepath" );

	if (handle == NULL )
		return false;

	count = handle->Write( source, length );
	fileSystem->CloseFile( handle );

	if (count < length)
		return false;

	return true;
}


//
// M_ReadFile
//
int M_ReadFile ( char const*	name, byte**	buffer ) {
	int count, length;
	idFile * handle = NULL;
	byte		*buf;

	handle = fileSystem->OpenFileRead( name, false );

	if (handle == NULL ) {
		I_Error ("Couldn't read file %s", name);
	}

	length = handle->Length();

	buf = ( byte* )Z_Malloc ( handle->Length(), PU_STATIC, NULL);
	count = handle->Read( buf, length );

	if (count < length ) {
		I_Error ("Couldn't read file %s", name);
	}

	fileSystem->CloseFile( handle );

	*buffer = buf;
	return length;
}

//
// Write a save game to the specified device using the specified game name.
//
static qboolean SaveGame( void* source, int length )
{
	return false;
}


qboolean M_WriteSaveGame( void* source, int length )
{
	return SaveGame( source, length );
}

int M_ReadSaveGame( byte** buffer )
{
	return 0;
}


//
// DEFAULTS
//











// machine-independent sound params


// UNIX hack, to be removed.
#ifdef SNDSERV
#endif

#ifdef LINUX
#endif

extern const char* const temp_chat_macros[];







//
// M_SaveDefaults
//
void M_SaveDefaults (void)
{
/*
    int		i;
    int		v;
    FILE*	f;
	
    f = f o pen (::g->defaultfile, "w");
    if (!f)
	return; // can't write the file, but don't complain
		
    for (i=0 ; i<::g->numdefaults ; i++)
    {
	if (::g->defaults[i].defaultvalue > -0xfff
	    && ::g->defaults[i].defaultvalue < 0xfff)
	{
	    v = *::g->defaults[i].location;
	    fprintf (f,"%s\t\t%i\n",::g->defaults[i].name,v);
	} else {
	    fprintf (f,"%s\t\t\"%s\"\n",::g->defaults[i].name,
		     * (char **) (::g->defaults[i].location));
	}
    }
	
    fclose (f);
*/
}


//
// M_LoadDefaults
//

void M_LoadDefaults (void)
{
    int		i;
    //int		len;
    //FILE*	f;
    //char	def[80];
    //char	strparm[100];
    //char*	newstring;
    //int		parm;
    //qboolean	isstring;
    
    // set everything to base values
    ::g->numdefaults = sizeof(::g->defaults)/sizeof(::g->defaults[0]);
    for (i=0 ; i < ::g->numdefaults ; i++)
		*::g->defaults[i].location = ::g->defaults[i].defaultvalue;
    
    // check for a custom default file
    i = M_CheckParm ("-config");
    if (i && i < ::g->myargc-1)
    {
		::g->defaultfile = ::g->myargv[i+1];
		I_Printf ("	default file: %s\n",::g->defaultfile);
    }
    else
		::g->defaultfile = ::g->basedefault;

/*
    // read the file in, overriding any set ::g->defaults
    f = f o pen (::g->defaultfile, "r");
    if (f)
    {
		while (!feof(f))
		{
			isstring = false;
			if (fscanf (f, "%79s %[^\n]\n", def, strparm) == 2)
			{
				if (strparm[0] == '"')
				{
					// get a string default
					isstring = true;
					len = strlen(strparm);
					newstring = (char *)DoomLib::Z_Malloc(len, PU_STATIC, 0);
					strparm[len-1] = 0;
					strcpy(newstring, strparm+1);
				}
				else if (strparm[0] == '0' && strparm[1] == 'x')
					sscanf(strparm+2, "%x", &parm);
				else
					sscanf(strparm, "%i", &parm);
				
				for (i=0 ; i<::g->numdefaults ; i++)
					if (!strcmp(def, ::g->defaults[i].name))
					{
						if (!isstring)
							*::g->defaults[i].location = parm;
						else
							*::g->defaults[i].location = (int) newstring;
						break;
					}
			}
		}
			
		fclose (f);
    }
*/
}


//
// SCREEN SHOTS
//




//
// WritePCXfile
//
void
WritePCXfile
( char*		filename,
  byte*		data,
  int		width,
  int		height,
  byte*		palette )
{
	I_Error( "depreciated" );
}


//
// M_ScreenShot
//
void M_ScreenShot (void)
{
/*
    int		i;
    byte*	linear;
    char	lbmname[12];
    
    // munge planar buffer to linear
    linear = ::g->screens[2];
    I_ReadScreen (linear);
    
    // find a file name to save it to
    strcpy(lbmname,"DOOM00.pcx");
		
    for (i=0 ; i<=99 ; i++)
    {
		lbmname[4] = i/10 + '0';
		lbmname[5] = i%10 + '0';
		if (_access(lbmname,0) == -1)
			break;	// file doesn't exist
    }
    if (i==100)
		I_Error ("M_ScreenShot: Couldn't create a PCX");
    
    // save the pcx file
    WritePCXfile (lbmname, linear,
		  SCREENWIDTH, SCREENHEIGHT,
		  (byte*)W_CacheLumpName ("PLAYPAL",PU_CACHE_SHARED));
	
    ::g->players[::g->consoleplayer].message = "screen shot";
*/
}



