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

#ifndef __Z_ZONE__
#define __Z_ZONE__

#include <stdio.h>

//
// ZONE MEMORY
// PU - purge tags.
// Tags < 100 are not overwritten until freed.
#define PU_STATIC		1	// static entire execution time
#define PU_SOUND		2	// static while playing
#define PU_MUSIC		3	// static while playing
#define PU_LEVEL		50	// static until level exited
#define PU_LEVSPEC		51      // a special thinker in a level
// Tags >= 100 are purgable whenever needed.
#define PU_PURGELEVEL	100
#define PU_CACHE 101

/*
#define PU_STATIC_SHARED		4	// static entire execution time
#define PU_SOUND_SHARED		5	// static while playing
#define PU_MUSIC_SHARED		6	// static while playing
#define PU_LEVEL_SHARED		52	// static until level exited
#define PU_LEVSPEC_SHARED		53      // a special thinker in a level
#define PU_CACHE_SHARED		102
*/
#define PU_STATIC_SHARED		PU_STATIC	// static entire execution time
#define PU_SOUND_SHARED		PU_SOUND	// static while playing
#define PU_MUSIC_SHARED		PU_MUSIC	// static while playing
#define PU_LEVEL_SHARED		PU_LEVEL	// static until level exited
#define PU_LEVSPEC_SHARED		PU_LEVSPEC      // a special thinker in a level
#define PU_CACHE_SHARED		PU_CACHE


bool Z_IsStatic( int tag );

void	Z_Init (void);
void*	Z_Malloc (int size, int tag, void *ptr);
void    Z_Free (void *ptr);
void    Z_FreeTag(int lowtag );
void    Z_FreeTags(int lowtag, int hightag );
void    Z_DumpHeap (int lowtag, int hightag);
void    Z_FileDumpHeap (FILE *f);
void    Z_CheckHeap (void);
void Z_ChangeTag2 (void **ptr, int tag);
int     Z_FreeMemory (void);


//bool MallocForLump( int lump, size_t size, void **ptr, int tag );


template< class _type_ >
bool MallocForLump( int lump, size_t size, _type_ * & ptr, int tag ) {
	ptr = static_cast< _type_ * >( Z_Malloc( size, tag, 0 ) );

	return true;
}


typedef struct memblock_s
{
    int			size;	// including the header and possibly tiny fragments
    void**		user;	// NULL if a free block
    int			tag;	// purgelevel
    int			id;	// should be ZONEID
    struct memblock_s*	next;
    struct memblock_s*	prev;
} memblock_t;


//
// This is used to get the local FILE:LINE info from CPP
// prior to really call the function in question.
//
#define Z_ChangeTag(p,t) \
{ \
      if (( (memblock_t *)( (byte *)(p) - sizeof(memblock_t)))->id!=0x1d4a11) \
	  I_Error("Z_CT at "__FILE__":%i",__LINE__); \
	  Z_ChangeTag2((void**)&p,t); \
};



#endif

