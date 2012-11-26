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



#include <stdlib.h>
#include <math.h>


#include "doomdef.h"
#include "d_net.h"

#include "m_bbox.h"

#include "r_local.h"
#include "r_sky.h"
#include "i_system.h"




// Fineangles in the SCREENWIDTH wide window.




// increment every time a check is made





// just for profiling purposes






// 0 = high, 1 = low

//
// precalculated math tables
//

// The ::g->viewangletox[::g->viewangle + FINEANGLES/4] lookup
// maps the visible view angles to screen X coordinates,
// flattening the arc to a flat ::g->projection plane.
// There will be many angles mapped to the same X. 

// The xtoviewangleangle[] table maps a screen pixel
// to the lowest ::g->viewangle that maps back to x ranges
// from ::g->clipangle to -::g->clipangle.


// UNUSED.
// The finetangentgent[angle+FINEANGLES/4] table
// holds the fixed_t tangent values for view angles,
// ranging from MININT to 0 to MAXINT.
// fixed_t		finetangent[FINEANGLES/2];

// fixed_t		finesine[5*FINEANGLES/4];
const fixed_t*		finecosine = &finesine[FINEANGLES/4];



// bumped light from gun blasts



void (*colfunc) (lighttable_t * dc_colormap,
				 byte * dc_source);
void (*basecolfunc) (lighttable_t * dc_colormap,
						byte * dc_source);
void (*fuzzcolfunc) (lighttable_t * dc_colormap,
						byte * dc_source);
void (*transcolfunc) (lighttable_t * dc_colormap,
						byte * dc_source);
void (*spanfunc) (fixed_t xfrac,
	fixed_t yfrac,
	fixed_t ds_y,
	int ds_x1,
	int ds_x2,
	fixed_t ds_xstep,
	fixed_t ds_ystep,
	lighttable_t * ds_colormap,
	byte * ds_source);



//
// R_AddPointToBox
// Expand a given bbox
// so that it encloses a given point.
//
void
R_AddPointToBox
( int		x,
 int		y,
 fixed_t*	box )
{
	if (x< box[BOXLEFT])
		box[BOXLEFT] = x;
	if (x> box[BOXRIGHT])
		box[BOXRIGHT] = x;
	if (y< box[BOXBOTTOM])
		box[BOXBOTTOM] = y;
	if (y> box[BOXTOP])
		box[BOXTOP] = y;
}


//
// R_PointOnSide
// Traverse BSP (sub) tree,
//  check point against partition plane.
// Returns side 0 (front) or 1 (back).
//
int
R_PointOnSide
( fixed_t	x,
 fixed_t	y,
 node_t*	node )
{
	fixed_t	dx;
	fixed_t	dy;
	fixed_t	left;
	fixed_t	right;

	if (!node->dx)
	{
		if (x <= node->x)
			return node->dy > 0;

		return node->dy < 0;
	}
	if (!node->dy)
	{
		if (y <= node->y)
			return node->dx < 0;

		return node->dx > 0;
	}

	dx = (x - node->x);
	dy = (y - node->y);

	// Try to quickly decide by looking at sign bits.
	if ( (node->dy ^ node->dx ^ dx ^ dy)&0x80000000 )
	{
		if  ( (node->dy ^ dx) & 0x80000000 )
		{
			// (left is negative)
			return 1;
		}
		return 0;
	}

	left = FixedMul ( node->dy>>FRACBITS , dx );
	right = FixedMul ( dy , node->dx>>FRACBITS );

	if (right < left)
	{
		// front side
		return 0;
	}
	// back side
	return 1;			
}


int
R_PointOnSegSide
( fixed_t	x,
 fixed_t	y,
 seg_t*	line )
{
	fixed_t	lx;
	fixed_t	ly;
	fixed_t	ldx;
	fixed_t	ldy;
	fixed_t	dx;
	fixed_t	dy;
	fixed_t	left;
	fixed_t	right;

	lx = line->v1->x;
	ly = line->v1->y;

	ldx = line->v2->x - lx;
	ldy = line->v2->y - ly;

	if (!ldx)
	{
		if (x <= lx)
			return ldy > 0;

		return ldy < 0;
	}
	if (!ldy)
	{
		if (y <= ly)
			return ldx < 0;

		return ldx > 0;
	}

	dx = (x - lx);
	dy = (y - ly);

	// Try to quickly decide by looking at sign bits.
	if ( (ldy ^ ldx ^ dx ^ dy)&0x80000000 )
	{
		if  ( (ldy ^ dx) & 0x80000000 )
		{
			// (left is negative)
			return 1;
		}
		return 0;
	}

	left = FixedMul ( ldy>>FRACBITS , dx );
	right = FixedMul ( dy , ldx>>FRACBITS );

	if (right < left)
	{
		// front side
		return 0;
	}
	// back side
	return 1;			
}


//
// R_PointToAngle
// To get a global angle from cartesian coordinates,
//  the coordinates are flipped until they are in
//  the first octant of the coordinate system, then
//  the y (<=x) is scaled and divided by x to get a
//  tangent (slope) value which is looked up in the
//  tantoangle[] table.

//




angle_t
R_PointToAngle
( fixed_t	x,
 fixed_t	y )
{	
	extern fixed_t GetViewX(); extern fixed_t GetViewY();
	x -= GetViewX();
	y -= GetViewY();

	if ( (!x) && (!y) )
		return 0;

	if (x>= 0)
	{
		// x >=0
		if (y>= 0)
		{
			// y>= 0

			if (x>y)
			{
				// octant 0
				return tantoangle[ SlopeDiv(y,x)];
			}
			else
			{
				// octant 1
				return ANG90-1-tantoangle[ SlopeDiv(x,y)];
			}
		}
		else
		{
			// y<0
			y = -y;

			if (x>y)
			{
				// octant 8
				return -tantoangle[SlopeDiv(y,x)]; // // ALANHACK UNSIGNED
			}
			else
			{
				// octant 7
				return ANG270+tantoangle[ SlopeDiv(x,y)];
			}
		}
	}
	else
	{
		// x<0
		x = -x;

		if (y>= 0)
		{
			// y>= 0
			if (x>y)
			{
				// octant 3
				return ANG180-1-tantoangle[ SlopeDiv(y,x)];
			}
			else
			{
				// octant 2
				return ANG90+ tantoangle[ SlopeDiv(x,y)];
			}
		}
		else
		{
			// y<0
			y = -y;

			if (x>y)
			{
				// octant 4
				return ANG180+tantoangle[ SlopeDiv(y,x)];
			}
			else
			{
				// octant 5
				return ANG270-1-tantoangle[ SlopeDiv(x,y)];
			}
		}
	}
	return 0;
}


angle_t
R_PointToAngle2
( fixed_t	x1,
 fixed_t	y1,
 fixed_t	x2,
 fixed_t	y2 )
{	
	extern void SetViewX( fixed_t ); extern void SetViewY( fixed_t );
	SetViewX( x1 );
	SetViewY( y1 );

	return R_PointToAngle (x2, y2);
}


fixed_t
R_PointToDist
( fixed_t	x,
 fixed_t	y )
{
	int		angle;
	fixed_t	dx;
	fixed_t	dy;
	fixed_t	temp;
	fixed_t	dist;

	extern fixed_t GetViewX(); extern fixed_t GetViewY();
	dx = abs(x - GetViewX());
	dy = abs(y - GetViewY());

	if (dy>dx)
	{
		temp = dx;
		dx = dy;
		dy = temp;
	}

	angle = (tantoangle[ FixedDiv(dy,dx)>>DBITS ]+ANG90) >> ANGLETOFINESHIFT;

	// use as cosine
	dist = FixedDiv (dx, finesine[angle] );	

	return dist;
}




//
// R_InitPointToAngle
//
void R_InitPointToAngle (void)
{
	// UNUSED - now getting from tables.c
#if 0
	int	i;
	long	t;
	float	f;
	//
	// slope (tangent) to angle lookup
	//
	for (i=0 ; i<=SLOPERANGE ; i++)
	{
		f = atan( (float)i/SLOPERANGE )/(3.141592657*2);
		t = 0xffffffff*f;
		tantoangle[i] = t;
	}
#endif
}


//
// R_ScaleFromGlobalAngle
// Returns the texture mapping scale
//  for the current line (horizontal span)
//  at the given angle.
// ::g->rw_distance must be calculated first.
//
fixed_t R_ScaleFromGlobalAngle (angle_t visangle)
{
	fixed_t		scale;
	//int			anglea;
	//int			angleb;
	angle_t		anglea;
	angle_t		angleb;
	int			sinea;
	int			sineb;
	fixed_t		num;
	int			den;

	// UNUSED
#if 0
	{
		fixed_t		dist;
		fixed_t		z;
		fixed_t		sinv;
		fixed_t		cosv;

		sinv = finesine[(visangle-::g->rw_normalangle)>>ANGLETOFINESHIFT];	
		dist = FixedDiv (::g->rw_distance, sinv);
		cosv = finecosine[(::g->viewangle-visangle)>>ANGLETOFINESHIFT];
		z = abs(FixedMul (dist, cosv));
		scale = FixedDiv(::g->projection, z);
		return scale;
	}
#endif

	extern angle_t GetViewAngle();
	anglea = ANG90 + (visangle-GetViewAngle());
	angleb = ANG90 + (visangle-::g->rw_normalangle);

	// both sines are allways positive
	sinea = finesine[anglea>>ANGLETOFINESHIFT];	
	sineb = finesine[angleb>>ANGLETOFINESHIFT];
	num = FixedMul(::g->projection,sineb) << ::g->detailshift;
	den = FixedMul(::g->rw_distance,sinea);

	// DHM - Nerve :: If the den is pretty much 0, don't try the divide
	if (den>>8 > 0 && den > num>>16)
	{
		scale = FixedDiv (num, den);

		if (scale > 64*FRACUNIT)
			scale = 64*FRACUNIT;
		else if (scale < 256)
			scale = 256;
	}
	else
		scale = 64*FRACUNIT;

	return scale;
}



//
// R_InitTables
//
void R_InitTables (void)
{
	// UNUSED: now getting from tables.c
#if 0
	int		i;
	float	a;
	float	fv;
	int		t;

	// ::g->viewangle tangent table
	for (i=0 ; i<FINEANGLES/2 ; i++)
	{
		a = (i-FINEANGLES/4+0.5)*PI*2/FINEANGLES;
		fv = FRACUNIT*tan (a);
		t = fv;
		finetangent[i] = t;
	}

	// finesine table
	for (i=0 ; i<5*FINEANGLES/4 ; i++)
	{
		// OPTIMIZE: mirror...
		a = (i+0.5)*PI*2/FINEANGLES;
		t = FRACUNIT*sin (a);
		finesine[i] = t;
	}
#endif

}



//
// R_InitTextureMapping
//
void R_InitTextureMapping (void)
{
	int			i;
	int			x;
	int			t;
	fixed_t		focallength;

	// Use tangent table to generate viewangletox:
	//  ::g->viewangletox will give the next greatest x
	//  after the view angle.
	//
	// Calc focallength
	//  so FIELDOFVIEW angles covers SCREENWIDTH.
	focallength = FixedDiv (::g->centerxfrac,
		finetangent[FINEANGLES/4+FIELDOFVIEW/2] );

	for (i=0 ; i<FINEANGLES/2 ; i++)
	{
		if (finetangent[i] > FRACUNIT*2)
			t = -1;
		else if (finetangent[i] < -FRACUNIT*2)
			t = ::g->viewwidth+1;
		else
		{
			t = FixedMul (finetangent[i], focallength);
			t = (::g->centerxfrac - t+FRACUNIT-1)>>FRACBITS;

			if (t < -1)
				t = -1;
			else if (t>::g->viewwidth+1)
				t = ::g->viewwidth+1;
		}
		::g->viewangletox[i] = t;
	}

	// Scan ::g->viewangletox[] to generate ::g->xtoviewangle[]:
	//  ::g->xtoviewangle will give the smallest view angle
	//  that maps to x.	
	for (x=0;x<=::g->viewwidth;x++)
	{
		i = 0;
		while (::g->viewangletox[i]>x)
			i++;
		::g->xtoviewangle[x] = (i<<ANGLETOFINESHIFT)-ANG90;
	}

	// Take out the fencepost cases from ::g->viewangletox.
	for (i=0 ; i<FINEANGLES/2 ; i++)
	{
		t = FixedMul (finetangent[i], focallength);
		t = ::g->centerx - t;

		if (::g->viewangletox[i] == -1)
			::g->viewangletox[i] = 0;
		else if (::g->viewangletox[i] == ::g->viewwidth+1)
			::g->viewangletox[i]  = ::g->viewwidth;
	}

	::g->clipangle = ::g->xtoviewangle[0];
}



//
// R_InitLightTables
// Only inits the ::g->zlight table,
//  because the ::g->scalelight table changes with view size.
//

void R_InitLightTables (void)
{
	int		i;
	int		j;
	int		level;
	int		nocollide_startmap; 	
	int		scale;

	// Calculate the light levels to use
	//  for each level / distance combination.
	for (i=0 ; i< LIGHTLEVELS ; i++)
	{
		nocollide_startmap = ((LIGHTLEVELS-1-i)*2)*NUMCOLORMAPS/LIGHTLEVELS;
		for (j=0 ; j<MAXLIGHTZ ; j++)
		{
			scale = FixedDiv ((SCREENWIDTH/2*FRACUNIT), (j+1)<<LIGHTZSHIFT);
			scale >>= LIGHTSCALESHIFT;
			level = nocollide_startmap - scale/DISTMAP;

			if (level < 0)
				level = 0;

			if (level >= NUMCOLORMAPS)
				level = NUMCOLORMAPS-1;

			::g->zlight[i][j] = ::g->colormaps + level*256;
		}
	}
}



//
// R_SetViewSize
// Do not really change anything here,
//  because it might be in the middle of a refresh.
// The change will take effect next refresh.
//


void
R_SetViewSize
( int		blocks,
 int		detail )
{
	::g->setsizeneeded = true;
	::g->setblocks = blocks;
	::g->setdetail = detail;
}


//
// R_ExecuteSetViewSize
//
void R_ExecuteSetViewSize (void)
{
	fixed_t	cosadj;
	fixed_t	dy;
	int		i;
	int		j;
	int		level;
	int		nocollide_startmap; 	

	::g->setsizeneeded = false;

	if (::g->setblocks == 11)
	{
		::g->scaledviewwidth = ORIGINAL_WIDTH;
		::g->viewheight = ORIGINAL_HEIGHT;
	}
	else
	{
		::g->scaledviewwidth = ::g->setblocks*32;
		::g->viewheight = (::g->setblocks*168/10)&~7;
	}

	// SMF - temp
	::g->scaledviewwidth *= GLOBAL_IMAGE_SCALER;
	::g->viewheight *= GLOBAL_IMAGE_SCALER;

	::g->detailshift = ::g->setdetail;
	::g->viewwidth = ::g->scaledviewwidth>>::g->detailshift;

	::g->centery = ::g->viewheight/2;
	::g->centerx = ::g->viewwidth/2;
	::g->centerxfrac = ::g->centerx<<FRACBITS;
	::g->centeryfrac = ::g->centery<<FRACBITS;
	::g->projection = ::g->centerxfrac;

	if (!::g->detailshift)
	{
		colfunc = basecolfunc = R_DrawColumn;
		fuzzcolfunc = R_DrawFuzzColumn;
		transcolfunc = R_DrawTranslatedColumn;
		spanfunc = R_DrawSpan;
	}
	else
	{
		colfunc = basecolfunc = R_DrawColumnLow;
		fuzzcolfunc = R_DrawFuzzColumn;
		transcolfunc = R_DrawTranslatedColumn;
		spanfunc = R_DrawSpanLow;
	}

	R_InitBuffer (::g->scaledviewwidth, ::g->viewheight);

	R_InitTextureMapping ();

	// psprite scales
	::g->pspritescale = FRACUNIT*::g->viewwidth/ORIGINAL_WIDTH;
	::g->pspriteiscale = FRACUNIT*ORIGINAL_WIDTH/::g->viewwidth;

	// thing clipping
	for (i=0 ; i < ::g->viewwidth ; i++)
		::g->screenheightarray[i] = ::g->viewheight;

	// planes
	for (i=0 ; i < ::g->viewheight ; i++)
	{
		dy = ((i-::g->viewheight/2)<<FRACBITS)+FRACUNIT/2;
		dy = abs(dy);
		::g->yslope[i] = FixedDiv ( (::g->viewwidth << ::g->detailshift)/2*FRACUNIT, dy);
	}

	for (i=0 ; i < ::g->viewwidth ; i++)
	{
		cosadj = abs(finecosine[::g->xtoviewangle[i]>>ANGLETOFINESHIFT]);
		::g->distscale[i] = FixedDiv (FRACUNIT,cosadj);
	}

	// Calculate the light levels to use
	//  for each level / scale combination.
	for (i=0 ; i< LIGHTLEVELS ; i++)
	{
		nocollide_startmap = ((LIGHTLEVELS-1-i)*2)*NUMCOLORMAPS/LIGHTLEVELS;
		for (j=0 ; j<MAXLIGHTSCALE ; j++)
		{
			level = nocollide_startmap - j*SCREENWIDTH/(::g->viewwidth << ::g->detailshift)/DISTMAP;

			if (level < 0)
				level = 0;

			if (level >= NUMCOLORMAPS)
				level = NUMCOLORMAPS-1;

			::g->scalelight[i][j] = ::g->colormaps + level*256;
		}
	}
}



//
// R_Init
//



void R_Init (void)
{
	R_InitData ();
	I_Printf ("\nR_InitData");
	R_InitPointToAngle ();
	I_Printf ("\nR_InitPointToAngle");
	R_InitTables ();
	// ::g->viewwidth / ::g->viewheight / ::g->detailLevel are set by the defaults
	I_Printf ("\nR_InitTables");

	R_SetViewSize (::g->screenblocks, ::g->detailLevel);
	R_InitPlanes ();
	I_Printf ("\nR_InitPlanes");
	R_InitLightTables ();
	I_Printf ("\nR_InitLightTables");
	R_InitSkyMap ();
	I_Printf ("\nR_InitSkyMap");
	R_InitTranslationTables ();
	I_Printf ("\nR_InitTranslationsTables");

	::g->framecount = 0;
}


//
// R_PointInSubsector
//
subsector_t*
R_PointInSubsector
( fixed_t	x,
 fixed_t	y )
{
	node_t*	node;
	int		side;
	int		nodenum;

	// single subsector is a special case
	if (!::g->numnodes)				
		return ::g->subsectors;

	nodenum = ::g->numnodes-1;

	while (! (nodenum & NF_SUBSECTOR) )
	{
		node = &::g->nodes[nodenum];
		side = R_PointOnSide (x, y, node);
		nodenum = node->children[side];
	}

	return &::g->subsectors[nodenum & ~NF_SUBSECTOR];
}



//
// R_SetupFrame
//
void R_SetupFrame (player_t* player)
{		
	int		i;

	::g->viewplayer = player;
	extern void SetViewX( fixed_t ); extern void SetViewY( fixed_t ); extern void SetViewAngle( angle_t );
	SetViewX( player->mo->x );
	SetViewY( player->mo->y );
	SetViewAngle( player->mo->angle + ::g->viewangleoffset );
	::g->extralight = player->extralight;

	::g->viewz = player->viewz;

	extern angle_t GetViewAngle();

	::g->viewsin = finesine[GetViewAngle()>>ANGLETOFINESHIFT];
	::g->viewcos = finecosine[GetViewAngle()>>ANGLETOFINESHIFT];

	::g->sscount = 0;

	if (player->fixedcolormap)
	{
		::g->fixedcolormap =
			::g->colormaps
			+ player->fixedcolormap*256*sizeof(lighttable_t);

		::g->walllights = ::g->scalelightfixed;

		for (i=0 ; i<MAXLIGHTSCALE ; i++)
			::g->scalelightfixed[i] = ::g->fixedcolormap;
	}
	else
		::g->fixedcolormap = 0;

	::g->framecount++;
	::g->validcount++;
}



//
// R_RenderView
//
void R_RenderPlayerView (player_t* player)
{
	if ( player->mo == NULL ) {
		return;
	}

	R_SetupFrame (player);

	// Clear buffers.
	R_ClearClipSegs ();
	R_ClearDrawSegs ();
	R_ClearPlanes ();
	R_ClearSprites ();

	// check for new console commands.
	NetUpdate ( NULL );

	// The head node is the last node output.
	R_RenderBSPNode (::g->numnodes-1);

	// Check for new console commands.
	NetUpdate ( NULL );

	R_DrawPlanes ();

	// Check for new console commands.
	NetUpdate ( NULL );

	R_DrawMasked ();

	// Check for new console commands.
	NetUpdate ( NULL );				
}

