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

#include "doomdef.h"

#include "i_system.h"
#include "z_zone.h"
#include "w_wad.h"

#include "r_local.h"

// Needs access to LFB (guess what).
#include "v_video.h"

// State.
#include "doomstat.h"


// ?

// status bar height at bottom of screen

//
// All drawing to the view buffer is accomplished in this file.
// The other refresh files only know about ccordinates,
//  not the architecture of the frame buffer.
// Conveniently, the frame buffer is a linear one,
//  and we need only the base address,
//  and the total size == width*height*depth/8.,
//



// Color tables for different ::g->players,
//  translate a limited part to another
//  (color ramps used for  suit colors).
//




//
// R_DrawColumn
// Source is the top of the column to scale.
//

// first pixel in a column (possibly virtual) 

// just for profiling 

//
// A column is a vertical slice/span from a wall texture that,
//  given the DOOM style restrictions on the view orientation,
//  will always have constant z depth.
// Thus a special case loop for very fast rendering can
//  be used. It has also been used with Wolfenstein 3D.
// 
void R_DrawColumn ( lighttable_t * dc_colormap,
					byte * dc_source ) 
{ 
	int			count; 
	byte*		dest; 
	fixed_t		frac;
	fixed_t		fracstep;	 

	count = ::g->dc_yh - ::g->dc_yl; 

	// Zero length, column does not exceed a pixel.
	if (count >= 0) {
		//return; 

	#ifdef RANGECHECK 
		if ((unsigned)::g->dc_x >= SCREENWIDTH
			|| ::g->dc_yl < 0
			|| ::g->dc_yh >= SCREENHEIGHT) 
			I_Error ("R_DrawColumn: %i to %i at %i", ::g->dc_yl, ::g->dc_yh, ::g->dc_x); 
	#endif 

		// Framebuffer destination address.
		// Use ::g->ylookup LUT to avoid multiply with ScreenWidth.
		// Use ::g->columnofs LUT for subwindows? 
		dest = ::g->ylookup[::g->dc_yl] + ::g->columnofs[::g->dc_x];  

		// Determine scaling,
		//  which is the only mapping to be done.
		fracstep = ::g->dc_iscale; 
		frac = ::g->dc_texturemid + (::g->dc_yl-::g->centery)*fracstep; 

		// Inner loop that does the actual texture mapping,
		//  e.g. a DDA-lile scaling.
		// This is as fast as it gets.
		do 
		{
			// Re-map color indices from wall texture column
			//  using a lighting/special effects LUT.
			const int truncated1 = frac >> FRACBITS;
			const int wrapped1 = truncated1 & 127;

			*dest = dc_colormap[dc_source[wrapped1]];

			frac += fracstep;
			dest += SCREENWIDTH; 
		} while (count--);
	}
} 



// UNUSED.
// Loop unrolled.
#if 0
void R_DrawColumn (void) 
{ 
	int			count; 
	byte*		source;
	byte*		dest;
	byte*		colormap;

	unsigned		frac;
	unsigned		fracstep;
	unsigned		fracstep2;
	unsigned		fracstep3;
	unsigned		fracstep4;	 

	count = ::g->dc_yh - ::g->dc_yl + 1; 

	source = ::g->dc_source;
	colormap = ::g->dc_colormap;		 
	dest = ::g->ylookup[::g->dc_yl] + ::g->columnofs[::g->dc_x];  

	fracstep = ::g->dc_iscale<<9; 
	frac = (::g->dc_texturemid + (::g->dc_yl-::g->centery)*::g->dc_iscale)<<9; 

	fracstep2 = fracstep+fracstep;
	fracstep3 = fracstep2+fracstep;
	fracstep4 = fracstep3+fracstep;

	while (count >= 8) 
	{ 
		dest[0] = colormap[source[frac>>25]]; 
		dest[SCREENWIDTH] = colormap[source[(frac+fracstep)>>25]]; 
		dest[SCREENWIDTH*2] = colormap[source[(frac+fracstep2)>>25]]; 
		dest[SCREENWIDTH*3] = colormap[source[(frac+fracstep3)>>25]];

		frac += fracstep4; 

		dest[SCREENWIDTH*4] = colormap[source[frac>>25]]; 
		dest[SCREENWIDTH*5] = colormap[source[(frac+fracstep)>>25]]; 
		dest[SCREENWIDTH*6] = colormap[source[(frac+fracstep2)>>25]]; 
		dest[SCREENWIDTH*7] = colormap[source[(frac+fracstep3)>>25]]; 

		frac += fracstep4; 
		dest += SCREENWIDTH*8; 
		count -= 8;
	} 

	while (count > 0)
	{ 
		*dest = colormap[source[frac>>25]]; 
		dest += SCREENWIDTH; 
		frac += fracstep; 
		count--;
	} 
}
#endif


void R_DrawColumnLow ( lighttable_t * dc_colormap,
					   byte * dc_source ) 
{ 
	int			count; 
	byte*		dest; 
	byte*		dest2;
	fixed_t		frac;
	fixed_t		fracstep;	 

	count = ::g->dc_yh - ::g->dc_yl; 

	// Zero length.
	if (count < 0) 
		return; 

#ifdef RANGECHECK 
	if ((unsigned)::g->dc_x >= SCREENWIDTH
		|| ::g->dc_yl < 0
		|| ::g->dc_yh >= SCREENHEIGHT)
	{

		I_Error ("R_DrawColumn: %i to %i at %i", ::g->dc_yl, ::g->dc_yh, ::g->dc_x);
	}
	//	::g->dccount++; 
#endif 
	// Blocky mode, need to multiply by 2.
	::g->dc_x <<= 1;

	dest = ::g->ylookup[::g->dc_yl] + ::g->columnofs[::g->dc_x];
	dest2 = ::g->ylookup[::g->dc_yl] + ::g->columnofs[::g->dc_x+1];

	fracstep = ::g->dc_iscale; 
	frac = ::g->dc_texturemid + (::g->dc_yl-::g->centery)*fracstep;

	do 
	{
		// Hack. Does not work corretly.
		*dest2 = *dest = ::g->dc_colormap[::g->dc_source[(frac>>FRACBITS)&127]];
		dest += SCREENWIDTH;
		dest2 += SCREENWIDTH;
		frac += fracstep; 

	} while (count--);
}


//
// Spectre/Invisibility.
//





//
// Framebuffer postprocessing.
// Creates a fuzzy image by copying pixels
//  from adjacent ones to left and right.
// Used with an all black colormap, this
//  could create the SHADOW effect,
//  i.e. spectres and invisible ::g->players.
//
void R_DrawFuzzColumn ( lighttable_t * dc_colormap,
						  byte * dc_source ) 
{ 
	int			count; 
	byte*		dest; 
	fixed_t		frac;
	fixed_t		fracstep;	 

	// Adjust borders. Low... 
	if (!::g->dc_yl) 
		::g->dc_yl = 1;

	// .. and high.
	if (::g->dc_yh == ::g->viewheight-1) 
		::g->dc_yh = ::g->viewheight - 2; 

	count = ::g->dc_yh - ::g->dc_yl; 

	// Zero length.
	if (count < 0) 
		return; 


#ifdef RANGECHECK 
	if ((unsigned)::g->dc_x >= SCREENWIDTH
		|| ::g->dc_yl < 0 || ::g->dc_yh >= SCREENHEIGHT)
	{
		I_Error ("R_DrawFuzzColumn: %i to %i at %i",
			::g->dc_yl, ::g->dc_yh, ::g->dc_x);
	}
#endif


	// Keep till ::g->detailshift bug in blocky mode fixed,
	//  or blocky mode removed.
	/* WATCOM code 
	if (::g->detailshift)
	{
	if (::g->dc_x & 1)
	{
	outpw (GC_INDEX,GC_READMAP+(2<<8) ); 
	outp (SC_INDEX+1,12); 
	}
	else
	{
	outpw (GC_INDEX,GC_READMAP); 
	outp (SC_INDEX+1,3); 
	}
	dest = destview + ::g->dc_yl*80 + (::g->dc_x>>1); 
	}
	else
	{
	outpw (GC_INDEX,GC_READMAP+((::g->dc_x&3)<<8) ); 
	outp (SC_INDEX+1,1<<(::g->dc_x&3)); 
	dest = destview + ::g->dc_yl*80 + (::g->dc_x>>2); 
	}*/


	// Does not work with blocky mode.
	dest = ::g->ylookup[::g->dc_yl] + ::g->columnofs[::g->dc_x];

	// Looks familiar.
	fracstep = ::g->dc_iscale; 
	frac = ::g->dc_texturemid + (::g->dc_yl-::g->centery)*fracstep; 

	// Looks like an attempt at dithering,
	//  using the colormap #6 (of 0-31, a bit
	//  brighter than average).
	do 
	{
		// Lookup framebuffer, and retrieve
		//  a pixel that is either one column
		//  left or right of the current one.
		// Add index from colormap to index.
		*dest = ::g->colormaps[6*256+dest[::g->fuzzoffset[::g->fuzzpos]]]; 

		// Clamp table lookup index.
		if (++::g->fuzzpos == FUZZTABLE) 
			::g->fuzzpos = 0;

		dest += SCREENWIDTH;

		frac += fracstep; 
	} while (count--); 
} 




//
// R_DrawTranslatedColumn
// Used to draw player ::g->sprites
//  with the green colorramp mapped to others.
// Could be used with different translation
//  tables, e.g. the lighter colored version
//  of the BaronOfHell, the HellKnight, uses
//  identical ::g->sprites, kinda brightened up.
//

void R_DrawTranslatedColumn ( lighttable_t * dc_colormap,
						  byte * dc_source ) 
{ 
	int			count; 
	byte*		dest; 
	fixed_t		frac;
	fixed_t		fracstep;	 

	count = ::g->dc_yh - ::g->dc_yl; 
	if (count < 0) 
		return; 

#ifdef RANGECHECK 
	if ((unsigned)::g->dc_x >= SCREENWIDTH
		|| ::g->dc_yl < 0
		|| ::g->dc_yh >= SCREENHEIGHT)
	{
		I_Error ( "R_DrawColumn: %i to %i at %i",
			::g->dc_yl, ::g->dc_yh, ::g->dc_x);
	}

#endif 


	// WATCOM VGA specific.
	/* Keep for fixing.
	if (::g->detailshift)
	{
	if (::g->dc_x & 1)
	outp (SC_INDEX+1,12); 
	else
	outp (SC_INDEX+1,3);

	dest = destview + ::g->dc_yl*80 + (::g->dc_x>>1); 
	}
	else
	{
	outp (SC_INDEX+1,1<<(::g->dc_x&3)); 

	dest = destview + ::g->dc_yl*80 + (::g->dc_x>>2); 
	}*/


	// FIXME. As above.
	dest = ::g->ylookup[::g->dc_yl] + ::g->columnofs[::g->dc_x]; 

	// Looks familiar.
	fracstep = ::g->dc_iscale; 
	frac = ::g->dc_texturemid + (::g->dc_yl-::g->centery)*fracstep; 

	// Here we do an additional index re-mapping.
	do 
	{
		// Translation tables are used
		//  to map certain colorramps to other ones,
		//  used with PLAY ::g->sprites.
		// Thus the "green" ramp of the player 0 sprite
		//  is mapped to gray, red, black/indigo. 
		*dest = dc_colormap[::g->dc_translation[dc_source[frac>>FRACBITS]]];
		dest += SCREENWIDTH;

		frac += fracstep; 
	} while (count--); 
} 




//
// R_InitTranslationTables
// Creates the translation tables to map
//  the green color ramp to gray, brown, red.
// Assumes a given structure of the PLAYPAL.
// Could be read from a lump instead.
//
void R_InitTranslationTables (void)
{
	int		i;

	::g->translationtables = (byte*)DoomLib::Z_Malloc (256*3+255, PU_STATIC, 0);
	::g->translationtables = (byte *)(( (intptr_t)::g->translationtables + 255 )& ~255);

	// translate just the 16 green colors
	for (i=0 ; i<256 ; i++)
	{
		if (i >= 0x70 && i<= 0x7f)
		{
			// map green ramp to gray, brown, red
			::g->translationtables[i] = 0x60 + (i&0xf);
			::g->translationtables [i+256] = 0x40 + (i&0xf);
			::g->translationtables [i+512] = 0x20 + (i&0xf);
		}
		else
		{
			// Keep all other colors as is.
			::g->translationtables[i] = ::g->translationtables[i+256] 
			= ::g->translationtables[i+512] = i;
		}
	}
}




//
// R_DrawSpan 
// With DOOM style restrictions on view orientation,
//  the floors and ceilings consist of horizontal slices
//  or spans with constant z depth.
// However, rotation around the world z axis is possible,
//  thus this mapping, while simpler and faster than
//  perspective correct texture mapping, has to traverse
//  the texture at an angle in all but a few cases.
// In consequence, flats are not stored by column (like walls),
//  and the inner loop has to step in texture space u and v.
//



// start of a 64*64 tile image 

// just for profiling


//
// Draws the actual span.
void R_DrawSpan ( fixed_t xfrac,
				  fixed_t yfrac,
				  fixed_t ds_y,
				  int ds_x1,
				  int ds_x2,
				  fixed_t ds_xstep,
				  fixed_t ds_ystep,
				  lighttable_t * ds_colormap,
				  byte * ds_source ) 
{ 
	byte*		dest; 
	int			count;
	int			spot; 

#ifdef RANGECHECK
	if (::g->ds_x2 < ::g->ds_x1
		|| ::g->ds_x1<0
		|| ::g->ds_x2>=SCREENWIDTH  
		|| (unsigned)::g->ds_y>SCREENHEIGHT)
	{
		I_Error( "R_DrawSpan: %i to %i at %i",
			::g->ds_x1,::g->ds_x2,::g->ds_y);
	}
	//	::g->dscount++; 
#endif 

	dest = ::g->ylookup[::g->ds_y] + ::g->columnofs[::g->ds_x1];

	// We do not check for zero spans here?
	count = ds_x2 - g->ds_x1; 

	if ( ds_x2 < ds_x1 ) {
		return;						// SMF - think this is the sky
	}

	do 
	{
		// Current texture index in u,v.
		spot = ((yfrac>>(16-6))&(63*64)) + ((xfrac>>16)&63);

		// Lookup pixel from flat texture tile,
		//  re-index using light/colormap.
		*dest++ = ds_colormap[ds_source[spot]];

		// Next step in u,v.
		xfrac += ds_xstep; 
		yfrac += ds_ystep;

	} while (count--); 
} 



// UNUSED.
// Loop unrolled by 4.
#if 0
void R_DrawSpan (void) 
{ 
	unsigned	position, step;

	byte*	source;
	byte*	colormap;
	byte*	dest;

	unsigned	count;
	usingned	spot; 
	unsigned	value;
	unsigned	temp;
	unsigned	xtemp;
	unsigned	ytemp;

	position = ((::g->ds_xfrac<<10)&0xffff0000) | ((::g->ds_yfrac>>6)&0xffff);
	step = ((::g->ds_xstep<<10)&0xffff0000) | ((::g->ds_ystep>>6)&0xffff);

	source = ::g->ds_source;
	colormap = ::g->ds_colormap;
	dest = ::g->ylookup[::g->ds_y] + ::g->columnofs[::g->ds_x1];	 
	count = ::g->ds_x2 - ::g->ds_x1 + 1; 

	while (count >= 4) 
	{ 
		ytemp = position>>4;
		ytemp = ytemp & 4032;
		xtemp = position>>26;
		spot = xtemp | ytemp;
		position += step;
		dest[0] = colormap[source[spot]]; 

		ytemp = position>>4;
		ytemp = ytemp & 4032;
		xtemp = position>>26;
		spot = xtemp | ytemp;
		position += step;
		dest[1] = colormap[source[spot]];

		ytemp = position>>4;
		ytemp = ytemp & 4032;
		xtemp = position>>26;
		spot = xtemp | ytemp;
		position += step;
		dest[2] = colormap[source[spot]];

		ytemp = position>>4;
		ytemp = ytemp & 4032;
		xtemp = position>>26;
		spot = xtemp | ytemp;
		position += step;
		dest[3] = colormap[source[spot]]; 

		count -= 4;
		dest += 4;
	} 
	while (count > 0) 
	{ 
		ytemp = position>>4;
		ytemp = ytemp & 4032;
		xtemp = position>>26;
		spot = xtemp | ytemp;
		position += step;
		*dest++ = colormap[source[spot]]; 
		count--;
	} 
} 
#endif


//
// Again..
//
void R_DrawSpanLow ( fixed_t xfrac,
				  fixed_t yfrac,
				  fixed_t ds_y,
				  int ds_x1,
				  int ds_x2,
				  fixed_t ds_xstep,
				  fixed_t ds_ystep,
				  lighttable_t * ds_colormap,
				  byte * ds_source ) 
{
	byte*		dest; 
	int			count;
	int			spot; 

#ifdef RANGECHECK 
	if (::g->ds_x2 < ::g->ds_x1
		|| ::g->ds_x1<0
		|| ::g->ds_x2>=SCREENWIDTH  
		|| (unsigned)::g->ds_y>SCREENHEIGHT)
	{
		I_Error( "R_DrawSpan: %i to %i at %i",
			::g->ds_x1,::g->ds_x2,::g->ds_y);
	}
	//	::g->dscount++; 
#endif 

	// Blocky mode, need to multiply by 2.
	::g->ds_x1 <<= 1;
	::g->ds_x2 <<= 1;

	dest = ::g->ylookup[::g->ds_y] + ::g->columnofs[::g->ds_x1];


	count = ::g->ds_x2 - ::g->ds_x1; 
	do 
	{ 
		spot = ((yfrac>>(16-6))&(63*64)) + ((xfrac>>16)&63);
		// Lowres/blocky mode does it twice,
		//  while scale is adjusted appropriately.
		*dest++ = ::g->ds_colormap[::g->ds_source[spot]]; 
		*dest++ = ::g->ds_colormap[::g->ds_source[spot]];

		xfrac += ::g->ds_xstep; 
		yfrac += ::g->ds_ystep; 

	} while (count--); 
}

//
// R_InitBuffer 
// Creats lookup tables that avoid
//  multiplies and other hazzles
//  for getting the framebuffer address
//  of a pixel to draw.
//
void
R_InitBuffer
( int		width,
 int		height ) 
{ 
	int		i; 

	// Handle resize,
	//  e.g. smaller view windows
	//  with border and/or status bar.
	::g->viewwindowx = (SCREENWIDTH-width) >> 1; 

	// Column offset. For windows.
	for (i=0 ; i<width ; i++) 
		::g->columnofs[i] = ::g->viewwindowx + i;

	// Samw with base row offset.
	if (width == SCREENWIDTH) 
		::g->viewwindowy = 0; 
	else 
		::g->viewwindowy = (SCREENHEIGHT-SBARHEIGHT-height) >> 1; 

	// Preclaculate all row offsets.
	for (i=0 ; i<height ; i++) 
		::g->ylookup[i] = ::g->screens[0] + (i+::g->viewwindowy)*SCREENWIDTH; 
} 




//
// R_FillBackScreen
// Fills the back screen with a pattern
//  for variable screen sizes
// Also draws a beveled edge.
//
void R_FillBackScreen (void) 
{ 
	byte*		src;
	byte*		dest; 
	int			x;
	int			y; 
	int			width, height, windowx, windowy;
	patch_t*	patch;

	// DOOM border patch.
	char	name1[] = "FLOOR7_2";
	// DOOM II border patch.
	char	name2[] = "GRNROCK";	

	char*	name;

	if (::g->scaledviewwidth == SCREENWIDTH)
		return;

	if ( ::g->gamemode == commercial)
		name = name2;
	else
		name = name1;

	src = (byte*)W_CacheLumpName (name, PU_CACHE_SHARED); 
	dest = ::g->screens[1]; 

	for (y=0 ; y<SCREENHEIGHT-SBARHEIGHT ; y++) { 
		for (x=0 ; x<SCREENWIDTH/64 ; x++) 	{ 
			memcpy(dest, src+((y&63)<<6), 64); 
			dest += 64; 
		} 
		if (SCREENWIDTH&63) 
		{ 
			memcpy(dest, src+((y&63)<<6), SCREENWIDTH&63); 
			dest += (SCREENWIDTH&63); 
		} 
	} 

	width = ::g->scaledviewwidth / GLOBAL_IMAGE_SCALER;
	height = ::g->viewheight / GLOBAL_IMAGE_SCALER;
	windowx = ::g->viewwindowx / GLOBAL_IMAGE_SCALER;
	windowy = ::g->viewwindowy / GLOBAL_IMAGE_SCALER;

	patch = (patch_t*)W_CacheLumpName ("brdr_t",PU_CACHE_SHARED);
	for (x=0 ; x<width ; x+=8) {
		V_DrawPatch (windowx+x,windowy-8,1,patch);
	}

	patch = (patch_t*)W_CacheLumpName ("brdr_b",PU_CACHE_SHARED);
	for (x=0 ; x<width ; x+=8) {
		V_DrawPatch (windowx+x,windowy+height,1,patch);
	}

	patch = (patch_t*)W_CacheLumpName ("brdr_l",PU_CACHE_SHARED);
	for (y=0 ; y<height ; y+=8) {
		V_DrawPatch (windowx-8,windowy+y,1,patch);
	}

	patch = (patch_t*)W_CacheLumpName ("brdr_r",PU_CACHE_SHARED);
	for (y=0 ; y<height ; y+=8) {
		V_DrawPatch (windowx+width,windowy+y,1,patch);
	}

	// Draw beveled edge. 
	V_DrawPatch(windowx-8, windowy-8, 1, (patch_t*)W_CacheLumpName ("brdr_tl",PU_CACHE_SHARED));
	V_DrawPatch(windowx+width, windowy-8, 1, (patch_t*)W_CacheLumpName ("brdr_tr",PU_CACHE_SHARED));
	V_DrawPatch(windowx-8, windowy+height, 1, (patch_t*)W_CacheLumpName ("brdr_bl",PU_CACHE_SHARED));
	V_DrawPatch (windowx+width, windowy+height, 1, (patch_t*)W_CacheLumpName ("brdr_br",PU_CACHE_SHARED));
} 


//
// Copy a screen buffer.
//
void
R_VideoErase
( unsigned	ofs,
 int		count ) 
{ 
	// LFB copy.
	// This might not be a good idea if memcpy
	//  is not optiomal, e.g. byte by byte on
	//  a 32bit CPU, as GNU GCC/Linux libc did
	//  at one point.
	memcpy(::g->screens[0]+ofs, ::g->screens[1]+ofs, count); 
} 


//
// R_DrawViewBorder
// Draws the border around the view
//  for different size windows?
//
void
V_MarkRect
( int		x,
 int		y,
 int		width,
 int		height ); 

void R_DrawViewBorder (void) 
{ 
	int		top;
	int		side;
	int		ofs;
	int		i; 

	if (::g->scaledviewwidth == SCREENWIDTH) 
		return; 

	top = ((SCREENHEIGHT-SBARHEIGHT)-::g->viewheight)/2; 
	side = (SCREENWIDTH-::g->scaledviewwidth)/2; 

	// copy top and one line of left side 
	R_VideoErase (0, top*SCREENWIDTH+side); 

	// copy one line of right side and bottom 
	ofs = (::g->viewheight+top)*SCREENWIDTH-side; 
	R_VideoErase (ofs, top*SCREENWIDTH+side); 

	// copy ::g->sides using wraparound 
	ofs = top*SCREENWIDTH + SCREENWIDTH-side; 
	side <<= 1;

	for (i=1 ; i < ::g->viewheight ; i++) 
	{ 
		R_VideoErase (ofs, side); 
		ofs += SCREENWIDTH; 
	} 

	// ? 
	V_MarkRect (0,0,SCREENWIDTH, SCREENHEIGHT-SBARHEIGHT); 
} 



