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

#include "i_system.h"
#include "z_zone.h"

#include "m_swap.h"

#include "w_wad.h"

#include "doomdef.h"
#include "r_local.h"
#include "p_local.h"

#include "doomstat.h"
#include "r_sky.h"

#ifdef LINUX
#include  <alloca.h>
#endif


#include "r_data.h"

#include <vector>

//
// Graphics.
// DOOM graphics for walls and ::g->sprites
// is stored in vertical runs of opaque pixels (posts).
// A column is composed of zero or more posts,
// a patch or sprite is composed of zero or more columns.
// 



//
// Texture definition.
// Each texture is composed of one or more patches,
// with patches being lumps stored in the WAD.
// The lumps are referenced by number, and patched
// into the rectangular texture space using origin
// and possibly other attributes.
//


//
// Texture definition.
// A DOOM wall texture is a list of patches
// which are to be combined in a predefined order.
//


// A single patch from a texture definition,
//  basically a rectangular area within
//  the texture rectangle.


// A maptexturedef_t describes a rectangular texture,
//  which is composed of one or more mappatch_t structures
//  that arrange graphic patches.






// for global animation

// needed for pre rendering



//
// MAPTEXTURE_T CACHING
// When a texture is first needed,
//  it counts the number of composite columns
//  required in the texture and allocates space
//  for a column directory and any new columns.
// The directory will simply point inside other patches
//  if there is only one patch in a given column,
//  but any columns with multiple patches
//  will have new postColumn_ts generated.
//



//
// R_DrawColumnInCache
// Clip and draw a column
//  from a patch into a cached post.
//
void
R_DrawColumnInCache
( postColumn_t*	patch,
  byte*			cache,
  int			originy,
  int			cacheheight )
{
    int		count;
    int		position;
    byte*	source;
    byte*	dest;
	
    dest = (byte *)cache + 3;
	
    while (patch->topdelta != 0xff)
    {
	source = (byte *)patch + 3;
	count = patch->length;
	position = originy + patch->topdelta;

	if (position < 0)
	{
	    count += position;
	    position = 0;
	}

	if (position + count > cacheheight)
	    count = cacheheight - position;

	if (count > 0)
	    memcpy (cache + position, source, count);
		
	patch = (postColumn_t *)(  (byte *)patch + patch->length + 4); 
    }
}



//
// R_GenerateComposite
// Using the texture definition,
//  the composite texture is created from the patches,
//  and each column is cached.
//
void R_GenerateComposite (int texnum)
{
    byte*			block;
    texture_t*		texture;
    texpatch_t*		patch;	
    patch_t*		realpatch;
    int				x;
    int				x1;
    int				x2;
    int				i;
    postColumn_t*	patchcol;
    short*			collump;
    unsigned short*	colofs;
	
    texture = ::g->s_textures[texnum];

    block = (byte*)DoomLib::Z_Malloc (::g->s_texturecompositesize[texnum],
		      PU_CACHE_SHARED, 
		      &::g->s_texturecomposite[texnum]);	

    collump = ::g->s_texturecolumnlump[texnum];
    colofs = ::g->s_texturecolumnofs[texnum];
    
    // Composite the columns together.
    patch = texture->patches;
		
    for (i=0 , patch = texture->patches;
	 i<texture->patchcount;
	 i++, patch++)
    {
	realpatch = (patch_t*)W_CacheLumpNum (patch->patch, PU_CACHE_SHARED);
	x1 = patch->originx;
	x2 = x1 + SHORT(realpatch->width);

	if (x1<0)
	    x = 0;
	else
	    x = x1;
	
	if (x2 > texture->width)
	    x2 = texture->width;

	for ( ; x<x2 ; x++)
	{
	    // Column does not have multiple patches?
	    if (collump[x] >= 0)
		continue;
	    
	    patchcol = (postColumn_t *)((byte *)realpatch
				    + LONG(realpatch->columnofs[x-x1]));
	    R_DrawColumnInCache (patchcol,
				 block + colofs[x],
				 patch->originy,
				 texture->height);
	}
						
    }
}



//
// R_GenerateLookup
//
void R_GenerateLookup (int texnum)
{
    texture_t*		texture;
    texpatch_t*		patch;	
    patch_t*		realpatch;
    int			x;
    int			x1;
    int			x2;
    int			i;
    short*		collump;
    unsigned short*	colofs;
	
    texture = ::g->s_textures[texnum];

    // Composited texture not created yet.
    ::g->s_texturecomposite[texnum] = 0;
    
    ::g->s_texturecompositesize[texnum] = 0;
    collump = ::g->s_texturecolumnlump[texnum];
    colofs = ::g->s_texturecolumnofs[texnum];
    
    // Now count the number of columns
    //  that are covered by more than one patch.
    // Fill in the lump / offset, so columns
    //  with only a single patch are all done.
    std::vector<byte> patchcount(texture->width, 0);
    patch = texture->patches;
		
    for (i=0 , patch = texture->patches;
	 i<texture->patchcount;
	 i++, patch++)
    {
	realpatch = (patch_t*)W_CacheLumpNum (patch->patch, PU_CACHE_SHARED);
	x1 = patch->originx;
	x2 = x1 + SHORT(realpatch->width);
	
	if (x1 < 0)
	    x = 0;
	else
	    x = x1;

	if (x2 > texture->width)
	    x2 = texture->width;
	for ( ; x<x2 ; x++)
	{
	    patchcount[x]++;
	    collump[x] = patch->patch;
	    colofs[x] = LONG(realpatch->columnofs[x-x1])+3;
	}
    }
	
    for (x=0 ; x<texture->width ; x++)
    {
	if (!patchcount[x])
	{
	    I_Printf ("R_GenerateLookup: column without a patch (%s)\n",
		    texture->name);
	    return;
	}
	// I_Error ("R_GenerateLookup: column without a patch");
	
	if (patchcount[x] > 1)
	{
	    // Use the cached block.
	    collump[x] = -1;	
	    colofs[x] = ::g->s_texturecompositesize[texnum];
	    
	    if (::g->s_texturecompositesize[texnum] > 0x10000-texture->height)
	    {
		I_Error ("R_GenerateLookup: texture %i is >64k",
			 texnum);
	    }
	    
	    ::g->s_texturecompositesize[texnum] += texture->height;
	}
    }	
}




//
// R_GetColumn
//
byte*
R_GetColumn
( int		tex,
  int		col )
{
    int		lump;
    int		ofs;
	
    col &= ::g->s_texturewidthmask[tex];
    lump = ::g->s_texturecolumnlump[tex][col];
    ofs = ::g->s_texturecolumnofs[tex][col];
    
    if (lump > 0)
	return (byte *)W_CacheLumpNum(lump,PU_CACHE_SHARED)+ofs;

    if (!::g->s_texturecomposite[tex])
	R_GenerateComposite (tex);

    return ::g->s_texturecomposite[tex] + ofs;
}




//
// R_InitTextures
// Initializes the texture list
//  with the s_textures from the world map.
//
void R_InitTextures (void)
{
    maptexture_t*	mtexture;
    texture_t*		texture;
    mappatch_t*		mpatch;
    texpatch_t*		patch;

    int			i;
    int			j;

    int*		maptex;
    int*		maptex2;
    int*		maptex1;
    
    char		name[9];
    char*		names;
    char*		name_p;
    
    int			totalwidth;
    int			nummappatches;
    int			offset;
    int			maxoff;
    int			maxoff2;
    int			numtextures1;
    int			numtextures2;

    int*		directory;
    
    int			temp1;
    int			temp2;
    int			temp3;

    
    // Load the patch names from pnames.lmp.
    name[8] = 0;	
    names = (char*)W_CacheLumpName ("PNAMES", PU_CACHE_SHARED);
    nummappatches = LONG ( *((int *)names) );
    name_p = names+4;
    
	std::vector<int> patchlookup(nummappatches);

    for (i=0 ; i<nummappatches ; i++)
    {
		strncpy (name,name_p+i*8, 8);
		patchlookup[i] = W_CheckNumForName (name);
    }
    Z_Free(names);
    
	if (::g->s_numtextures == 0)
	{

		// Load the map texture definitions from textures.lmp.
		// The data is contained in one or two lumps,
		//  TEXTURE1 for shareware, plus TEXTURE2 for commercial.
		maptex = maptex1 = (int*)W_CacheLumpName ("TEXTURE1", PU_CACHE_SHARED); // ALAN:  LOADTIME
		numtextures1 = LONG(*maptex);
		maxoff = W_LumpLength (W_GetNumForName ("TEXTURE1"));
		directory = maptex+1;

		if (W_CheckNumForName ("TEXTURE2") != -1)
		{
			maptex2 = (int*)W_CacheLumpName ("TEXTURE2", PU_CACHE_SHARED); // ALAN:  LOADTIME
			numtextures2 = LONG(*maptex2);
			maxoff2 = W_LumpLength (W_GetNumForName ("TEXTURE2"));
		}
		else
		{
			maptex2 = NULL;
			numtextures2 = 0;
			maxoff2 = 0;
		}


		::g->s_numtextures = numtextures1 + numtextures2;

		::g->s_textures = (texture_t**)DoomLib::Z_Malloc (::g->s_numtextures*sizeof(texture_t*), PU_STATIC_SHARED, 0);
		::g->s_texturecolumnlump = (short**)DoomLib::Z_Malloc (::g->s_numtextures*sizeof(short*), PU_STATIC_SHARED, 0);
		::g->s_texturecolumnofs = (unsigned short**)DoomLib::Z_Malloc (::g->s_numtextures*sizeof(unsigned short*), PU_STATIC_SHARED, 0);
		::g->s_texturewidthmask = (int*)DoomLib::Z_Malloc (::g->s_numtextures*4, PU_STATIC_SHARED, 0);
		::g->s_textureheight = (fixed_t*)DoomLib::Z_Malloc (::g->s_numtextures*4, PU_STATIC_SHARED, 0);
		::g->s_texturecomposite = (byte**)DoomLib::Z_Malloc (::g->s_numtextures*sizeof(byte*), PU_STATIC_SHARED, 0);
		::g->s_texturecompositesize = (int*)DoomLib::Z_Malloc (::g->s_numtextures*4, PU_STATIC_SHARED, 0);

		totalwidth = 0;

		//	Really complex printing shit...
		temp1 = W_GetNumForName ("S_START");  // P_???????
		temp2 = W_GetNumForName ("S_END") - 1;
		temp3 = ((temp2-temp1+63)/64) + ((::g->s_numtextures+63)/64);
		I_Printf("[");
		for (i = 0; i < temp3; i++)
			I_Printf(" ");
		I_Printf("         ]");
		for (i = 0; i < temp3; i++)
			I_Printf("\x8");
		I_Printf("\x8\x8\x8\x8\x8\x8\x8\x8\x8\x8");	

		for (i=0 ; i < ::g->s_numtextures ; i++, directory++)
		{
			if (!(i&63))
				I_Printf (".");

			if (i == numtextures1)
			{
				// Start looking in second texture file.
				maptex = maptex2;
				maxoff = maxoff2;
				directory = maptex+1;
			}
		
			offset = LONG(*directory);

			if (offset > maxoff)
				I_Error ("R_InitTextures: bad texture directory");
		
			mtexture = (maptexture_t *) ( (byte *)maptex + offset);

			texture = ::g->s_textures[i] = (texture_t*)DoomLib::Z_Malloc (sizeof(texture_t)
				+ sizeof(texpatch_t)*(SHORT(mtexture->patchcount)-1), PU_STATIC_SHARED, 0);

			texture->width = SHORT(mtexture->width);
			texture->height = SHORT(mtexture->height);
			texture->patchcount = SHORT(mtexture->patchcount);

			memcpy (texture->name, mtexture->name, sizeof(texture->name));
			mpatch = &mtexture->patches[0];
			patch = &texture->patches[0];

			for (j=0 ; j<texture->patchcount ; j++, mpatch++, patch++)
			{
				patch->originx = SHORT(mpatch->originx);
				patch->originy = SHORT(mpatch->originy);
				patch->patch = patchlookup[SHORT(mpatch->patch)];
				if (patch->patch == -1)
				{
					I_Error ("R_InitTextures: Missing patch in texture %s",
					texture->name);
				}
			}		
			::g->s_texturecolumnlump[i] = (short*)DoomLib::Z_Malloc (texture->width*2, PU_STATIC_SHARED,0);
			::g->s_texturecolumnofs[i] = (unsigned short*)DoomLib::Z_Malloc (texture->width*2, PU_STATIC_SHARED,0);

			j = 1;
			while (j*2 <= texture->width)
				j<<=1;

			::g->s_texturewidthmask[i] = j-1;
			::g->s_textureheight[i] = texture->height<<FRACBITS;

			totalwidth += texture->width;
		}

		Z_Free(maptex1);
		if (maptex2)
			Z_Free(maptex2);


		// Precalculate whatever possible.	
		for (i=0 ; i < ::g->s_numtextures ; i++)
			R_GenerateLookup (i);
	}

	// ALAN:  These animations are done globally -- can it be shared?
	// Create translation table for global animation.
	::g->texturetranslation = (int*)DoomLib::Z_Malloc ((::g->s_numtextures+1)*4, PU_STATIC, 0);

	for (i=0 ; i < ::g->s_numtextures ; i++)
		::g->texturetranslation[i] = i;	
}



//
// R_InitFlats
//
void R_InitFlats (void)
{
    int		i;
	
    ::g->firstflat = W_GetNumForName ("F_START") + 1;
    ::g->lastflat = W_GetNumForName ("F_END") - 1;
    ::g->numflats = ::g->lastflat - ::g->firstflat + 1;
	
    // Create translation table for global animation.
    ::g->flattranslation = (int*)DoomLib::Z_Malloc ((::g->numflats+1)*4, PU_STATIC, 0);
    
    for (i=0 ; i < ::g->numflats ; i++)
	::g->flattranslation[i] = i;
}


//
// R_InitSpriteLumps
// Finds the width and hoffset of all ::g->sprites in the wad,
//  so the sprite does not need to be cached completely
//  just for having the header info ready during rendering.
//
void R_InitSpriteLumps (void)
{
    int		i;
    patch_t	*patch;
	
    ::g->firstspritelump = W_GetNumForName ("S_START") + 1;
    ::g->lastspritelump = W_GetNumForName ("S_END") - 1;
    
    ::g->numspritelumps = ::g->lastspritelump - ::g->firstspritelump + 1;
    ::g->spritewidth = (fixed_t*)DoomLib::Z_Malloc (::g->numspritelumps*4, PU_STATIC, 0);
    ::g->spriteoffset = (fixed_t*)DoomLib::Z_Malloc (::g->numspritelumps*4, PU_STATIC, 0);
    ::g->spritetopoffset = (fixed_t*)DoomLib::Z_Malloc (::g->numspritelumps*4, PU_STATIC, 0);
	
    for (i=0 ; i< ::g->numspritelumps ; i++)
    {
	if (!(i&63))
	    I_Printf (".");

	patch = (patch_t*)W_CacheLumpNum (::g->firstspritelump+i, PU_CACHE_SHARED);
	::g->spritewidth[i] = SHORT(patch->width)<<FRACBITS;
	::g->spriteoffset[i] = SHORT(patch->leftoffset)<<FRACBITS;
	::g->spritetopoffset[i] = SHORT(patch->topoffset)<<FRACBITS;
    }
}



//
// R_InitColormaps
//
void R_InitColormaps (void)
{
    int	lump, length;
    
    // Load in the light tables, 
    //  256 byte align tables.
    lump = W_GetNumForName("COLORMAP"); 
    length = W_LumpLength (lump) + 255; 
    ::g->colormaps = (lighttable_t*)DoomLib::Z_Malloc (length, PU_STATIC, 0); 
    ::g->colormaps = (byte *)( ((intptr_t)::g->colormaps + 255)&~0xff);
    W_ReadLump (lump,::g->colormaps); 
}



//
// R_InitData
// Locates all the lumps
//  that will be used by all views
// Must be called after W_Init.
//
void R_InitData (void)
{
    R_InitTextures ();
    I_Printf ("\nInitTextures");
    R_InitFlats ();
    I_Printf ("\nInitFlats");
    R_InitSpriteLumps ();
    I_Printf ("\nInitSprites");
    R_InitColormaps ();
    I_Printf ("\nInitColormaps");
}



//
// R_FlatNumForName
// Retrieval, get a flat number for a flat name.
//
int R_FlatNumForName (const char* name)
{
    int		i;
    char	namet[9];

    i = W_CheckNumForName (name);

    if (i == -1)
    {
		namet[8] = 0;
		memcpy (namet, name,8);
		I_Error ("R_FlatNumForName: %s not found",namet);
    }
    return i - ::g->firstflat;
}




//
// R_CheckTextureNumForName
// Check whether texture is available.
// Filter out NoTexture indicator.
//
int	R_CheckTextureNumForName (const char *name)
{
    int		i;

    // "NoTexture" marker.
    if (name[0] == '-')		
	return 0;
		
    for (i=0 ; i < ::g->s_numtextures ; i++)
	if ( !idStr::Icmpn( ::g->s_textures[i]->name, name, 8 ) )
	    return i;
		
    return -1;
}



//
// R_TextureNumForName
// Calls R_CheckTextureNumForName,
//  aborts with error message.
//
int	R_TextureNumForName (const char* name)
{
    int		i;
	
    i = R_CheckTextureNumForName (name);

    if (i==-1)
    {
	I_Error ("R_TextureNumForName: %s not found",
		 name);
    }
    return i;
}




//
// R_PrecacheLevel
// Preloads all relevant graphics for the level.
//

void R_PrecacheLevel (void)
{
    int			i;
    int			j;
    int			k;
    int			lump;
    
    texture_t*		texture;
    thinker_t*		th;
    spriteframe_t*	sf;

    if (::g->demoplayback)
	return;
    
    // Precache flats.
	std::vector<char> flatpresent(::g->numflats, 0);
    
    for (i=0 ; i < ::g->numsectors ; i++)
    {
	flatpresent[::g->sectors[i].floorpic] = 1;
	flatpresent[::g->sectors[i].ceilingpic] = 1;
    }
	
    ::g->flatmemory = 0;

    for (i=0 ; i < ::g->numflats ; i++)
    {
	if (flatpresent[i])
	{
	    lump = ::g->firstflat + i;
	    ::g->flatmemory += lumpinfo[lump].size;
	    W_CacheLumpNum(lump, PU_CACHE_SHARED);
	}
    }
    
    // Precache textures.
    std::vector<char> texturepresent(::g->s_numtextures, 0);
	
    for (i=0 ; i < ::g->numsides ; i++)
    {
	texturepresent[::g->sides[i].toptexture] = 1;
	texturepresent[::g->sides[i].midtexture] = 1;
	texturepresent[::g->sides[i].bottomtexture] = 1;
    }

    // Sky texture is always present.
    // Note that F_SKY1 is the name used to
    //  indicate a sky floor/ceiling as a flat,
    //  while the sky texture is stored like
    //  a wall texture, with an episode dependend
    //  name.
    texturepresent[::g->skytexture] = 1;
	
    ::g->texturememory = 0;
    for (i=0 ; i < ::g->s_numtextures ; i++)
    {
	if (!texturepresent[i])
	    continue;

	texture = ::g->s_textures[i];
	
	for (j=0 ; j<texture->patchcount ; j++)
	{
	    lump = texture->patches[j].patch;
	    ::g->texturememory += lumpinfo[lump].size;
	    W_CacheLumpNum(lump , PU_CACHE_SHARED);
	}
    }
    
    // Precache ::g->sprites.
    std::vector<char> spritepresent(::g->numsprites, 0);
	
    for (th = ::g->thinkercap.next ; th != &::g->thinkercap ; th=th->next)
    {
	if (th->function.acp1 == (actionf_p1)P_MobjThinker)
	    spritepresent[((mobj_t *)th)->sprite] = 1;
    }
	
    ::g->spritememory = 0;
    for (i=0 ; i < ::g->numsprites ; i++)
    {
	if (!spritepresent[i])
	    continue;

	for (j=0 ; j < ::g->sprites[i].numframes ; j++)
	{
	    sf = &::g->sprites[i].spriteframes[j];
	    for (k=0 ; k<8 ; k++)
	    {
		lump = ::g->firstspritelump + sf->lump[k];
		::g->spritememory += lumpinfo[lump].size;
		W_CacheLumpNum(lump , PU_CACHE_SHARED);
	    }
	}
    }
}





