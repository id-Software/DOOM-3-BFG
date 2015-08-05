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

#ifdef __GNUG__
#pragma implementation "w_wad.h"
#endif
#include "w_wad.h"

#include "globaldata.h"


#include <ctype.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <vector>

#include "doomtype.h"
#include "m_swap.h"
#include "i_system.h"
#include "z_zone.h"

#include "idlib/precompiled.h"


//
// GLOBALS
//

lumpinfo_t*	lumpinfo = NULL;
int			numlumps;
void**		lumpcache;



int filelength (FILE* handle) 
{ 
	// DHM - not used :: development tool (loading single lump not in a WAD file)
	return 0;
}


void
ExtractFileBase
( const char*		path,
  char*		dest )
{
	const char*	src;
	int		length;

	src = path + strlen(path) - 1;

	// back up until a \ or the start
	while (src != path
		&& *(src-1) != '\\'
		&& *(src-1) != '/')
	{
		src--;
	}
    
	// copy up to eight characters
	memset (dest,0,8);
	length = 0;

	while (*src && *src != '.')
	{
		if (++length == 9)
			I_Error ("Filename base of %s >8 chars",path);

		*dest++ = toupper((int)*src++);
	}
}





//
// LUMP BASED ROUTINES.
//

//
// W_AddFile
// All files are optional, but at least one file must be
//  found (PWAD, if all required lumps are present).
// Files with a .wad extension are wadlink files
//  with multiple lumps.
// Other files are single lumps with the base filename
//  for the lump name.
//
// If filename starts with a tilde, the file is handled
//  specially to allow map reloads.
// But: the reload feature is a fragile hack...

const char*			reloadname;


void W_AddFile ( const char *filename)
{
    wadinfo_t		header;
    lumpinfo_t*		lump_p;
    int		i;
    idFile *		handle;
    int			length;
    int			startlump;
    std::vector<filelump_t>	fileinfo( 1 );
    
    // open the file and add to directory
    if ( (handle = fileSystem->OpenFileRead(filename)) == 0)
    {
		I_Printf (" couldn't open %s\n",filename);
		return;
    }

    I_Printf (" adding %s\n",filename);
    startlump = numlumps;
	
    if ( idStr::Icmp( filename+strlen(filename)-3 , "wad" ) )
    {
		// single lump file
		fileinfo[0].filepos = 0;
		fileinfo[0].size = 0;
		ExtractFileBase (filename, fileinfo[0].name);
		numlumps++;
    }
    else 
    {
		// WAD file
		handle->Read( &header, sizeof( header ) );
		if ( idStr::Cmpn( header.identification,"IWAD",4 ) )
		{
			// Homebrew levels?
			if ( idStr::Cmpn( header.identification, "PWAD", 4 ) )
			{
				I_Error ("Wad file %s doesn't have IWAD "
					"or PWAD id\n", filename);
			}
		    
			// ???modifiedgame = true;		
		}
		header.numlumps = LONG(header.numlumps);
		header.infotableofs = LONG(header.infotableofs);
		length = header.numlumps*sizeof(filelump_t);
		fileinfo.resize(header.numlumps);
		handle->Seek(  header.infotableofs, FS_SEEK_SET );
		handle->Read( &fileinfo[0], length );
		numlumps += header.numlumps;
    }

    
	// Fill in lumpinfo
	if (lumpinfo == NULL) {
		lumpinfo = (lumpinfo_t*)malloc( numlumps*sizeof(lumpinfo_t) );
	} else {
		lumpinfo = (lumpinfo_t*)realloc( lumpinfo, numlumps*sizeof(lumpinfo_t) );
	}

	if (!lumpinfo)
		I_Error ("Couldn't realloc lumpinfo");

	lump_p = &lumpinfo[startlump];

	::g->wadFileHandles[ ::g->numWadFiles++ ] = handle;

	filelump_t * filelumpPointer = &fileinfo[0];
	for (i=startlump ; i<numlumps ; i++,lump_p++, filelumpPointer++)
	{
		lump_p->handle = handle;
		lump_p->position = LONG(filelumpPointer->filepos);
		lump_p->size = LONG(filelumpPointer->size);
		strncpy (lump_p->name, filelumpPointer->name, 8);
	}
}




//
// W_Reload
// Flushes any of the reloadable lumps in memory
//  and reloads the directory.
//
void W_Reload (void)
{
	// DHM - unused development tool
}

//
// W_FreeLumps
// Frees all lump data
//
void W_FreeLumps() {
	if ( lumpcache != NULL ) {
		for ( int i = 0; i < numlumps; i++ ) {
			if ( lumpcache[i] ) {
				Z_Free( lumpcache[i] );
			}
		}

		Z_Free( lumpcache );
		lumpcache = NULL;
	}

	if ( lumpinfo != NULL ) {
		free( lumpinfo );
		lumpinfo = NULL;
		numlumps = 0;
	}
}

//
// W_FreeWadFiles
// Free this list of wad files so that a new list can be created
//
void W_FreeWadFiles() {
	for (int i = 0 ; i < MAXWADFILES ; i++) {
		wadfiles[i] = NULL;
		if ( ::g->wadFileHandles[i] ) {
			delete ::g->wadFileHandles[i];
		}
		::g->wadFileHandles[i] = NULL;
	}
	::g->numWadFiles = 0;
	extraWad = 0;
}



//
// W_InitMultipleFiles
// Pass a null terminated list of files to use.
// All files are optional, but at least one file
//  must be found.
// Files with a .wad extension are idlink files
//  with multiple lumps.
// Other files are single lumps with the base filename
//  for the lump name.
// Lump names can appear multiple times.
// The name searcher looks backwards, so a later file
//  does override all earlier ones.
//
void W_InitMultipleFiles (const char** filenames)
{
	int		size;

	if (lumpinfo == NULL)
	{
		// open all the files, load headers, and count lumps
		numlumps = 0;

		// will be realloced as lumps are added
		lumpinfo = NULL;

		for ( ; *filenames ; filenames++)
		{
			W_AddFile (*filenames);
		}
		
		if (!numlumps)
			I_Error ("W_InitMultipleFiles: no files found");

		// set up caching
		size = numlumps * sizeof(*lumpcache);
		lumpcache = (void**)DoomLib::Z_Malloc(size, PU_STATIC_SHARED, 0 );

		if (!lumpcache)
			I_Error ("Couldn't allocate lumpcache");

		memset (lumpcache,0, size);
	} else {
		// set up caching
		size = numlumps * sizeof(*lumpcache);
		lumpcache = (void**)DoomLib::Z_Malloc(size, PU_STATIC_SHARED, 0 );

		if (!lumpcache)
			I_Error ("Couldn't allocate lumpcache");

		memset (lumpcache,0, size);
	}
}


void W_Shutdown() {
/*
	for (int i = 0 ; i < MAXWADFILES ; i++) {
		if ( ::g->wadFileHandles[i] ) {
			doomFiles->FClose( ::g->wadFileHandles[i] );
		}
	}

	if ( lumpinfo != NULL ) {
		free( lumpinfo );
		lumpinfo = NULL;
	}
*/
	W_FreeLumps();
	W_FreeWadFiles();
}

//
// W_NumLumps
//
int W_NumLumps (void)
{
    return numlumps;
}



//
// W_CheckNumForName
// Returns -1 if name not found.
//

int W_CheckNumForName (const char* name)
{
	const int NameLength = 9;

    union {
		char	s[NameLength];
		int	x[2];
	
    } name8;
    
    int		v1;
    int		v2;
    lumpinfo_t*	lump_p;

    // make the name into two integers for easy compares
    strncpy (name8.s,name, NameLength - 1);

    // in case the name was a fill 8 chars
    name8.s[NameLength - 1] = 0;

    // case insensitive
	for ( int i = 0; i < NameLength; ++i ) {
		name8.s[i] = toupper( name8.s[i] );		
	}

    v1 = name8.x[0];
    v2 = name8.x[1];


    // scan backwards so patch lump files take precedence
    lump_p = lumpinfo + numlumps;

    while (lump_p-- != lumpinfo)
    {
		if ( *(int *)lump_p->name == v1
			&& *(int *)&lump_p->name[4] == v2)
		{
			return lump_p - lumpinfo;
		}
    }

    // TFB. Not found.
    return -1;
}




//
// W_GetNumForName
// Calls W_CheckNumForName, but bombs out if not found.
//
int W_GetNumForName ( const char* name)
{
    int	i;

    i = W_CheckNumForName ( name);
    
    if (i == -1)
      I_Error ("W_GetNumForName: %s not found!", name);
      
    return i;
}


//
// W_LumpLength
// Returns the buffer size needed to load the given lump.
//
int W_LumpLength (int lump)
{
    if (lump >= numlumps)
		I_Error ("W_LumpLength: %i >= numlumps",lump);

    return lumpinfo[lump].size;
}



//
// W_ReadLump
// Loads the lump into the given buffer,
//  which must be >= W_LumpLength().
//
void
W_ReadLump
( int		lump,
  void*		dest )
{
    int			c;
    lumpinfo_t*	l;
    idFile *		handle;
	
    if (lump >= numlumps)
		I_Error ("W_ReadLump: %i >= numlumps",lump);

    l = lumpinfo+lump;
	
	handle = l->handle;
	
	handle->Seek( l->position, FS_SEEK_SET );
	c = handle->Read( dest, l->size );

    if (c < l->size)
		I_Error ("W_ReadLump: only read %i of %i on lump %i",  c,l->size,lump);	
}




//
// W_CacheLumpNum
//
void*
W_CacheLumpNum
( int		lump,
  int		tag )
{
#ifdef RANGECHECK
	if (lump >= numlumps)
	I_Error ("W_CacheLumpNum: %i >= numlumps",lump);
#endif

	if (!lumpcache[lump])
	{
		byte*	ptr;
		// read the lump in
		//I_Printf ("cache miss on lump %i\n",lump);
		ptr = (byte*)DoomLib::Z_Malloc(W_LumpLength (lump), tag, &lumpcache[lump]);
		W_ReadLump (lump, lumpcache[lump]);
	}

	return lumpcache[lump];
}



//
// W_CacheLumpName
//
void*
W_CacheLumpName
( const char*		name,
  int		tag )
{
    return W_CacheLumpNum (W_GetNumForName(name), tag);
}


void W_Profile (void)
{
}



