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

#include "i_system.h"

#include "doomdef.h"
#include "doomstat.h"

#include "r_local.h"
#include "r_sky.h"


// OPTIMIZE: closed two sided ::g->lines as single sided

// True if any of the ::g->segs textures might be visible.

// False if the back side is the same plane.



// angle to line origin

//
// regular wall
//










//
// R_RenderMaskedSegRange
//
void
R_RenderMaskedSegRange
( drawseg_t*	ds,
  int		x1,
  int		x2 )
{
    unsigned		index;
    postColumn_t*	col;
    int				lightnum;
    int				texnum;
    
    // Calculate light table.
    // Use different light tables
    //   for horizontal / vertical / diagonal. Diagonal?
    // OPTIMIZE: get rid of LIGHTSEGSHIFT globally
    ::g->curline = ds->curline;
    ::g->frontsector = ::g->curline->frontsector;
    ::g->backsector = ::g->curline->backsector;
    texnum = ::g->texturetranslation[::g->curline->sidedef->midtexture];
	
    lightnum = (::g->frontsector->lightlevel >> LIGHTSEGSHIFT)+::g->extralight;

    if (::g->curline->v1->y == ::g->curline->v2->y)
	lightnum--;
    else if (::g->curline->v1->x == ::g->curline->v2->x)
	lightnum++;

    if (lightnum < 0)		
	::g->walllights = ::g->scalelight[0];
    else if (lightnum >= LIGHTLEVELS)
	::g->walllights = ::g->scalelight[LIGHTLEVELS-1];
    else
	::g->walllights = ::g->scalelight[lightnum];

    ::g->maskedtexturecol = ds->maskedtexturecol;

    ::g->rw_scalestep = ds->scalestep;		
    ::g->spryscale = ds->scale1 + (x1 - ds->x1)*::g->rw_scalestep;
    ::g->mfloorclip = ds->sprbottomclip;
    ::g->mceilingclip = ds->sprtopclip;
    
    // find positioning
    if (::g->curline->linedef->flags & ML_DONTPEGBOTTOM)
    {
	::g->dc_texturemid = ::g->frontsector->floorheight > ::g->backsector->floorheight
	    ? ::g->frontsector->floorheight : ::g->backsector->floorheight;
	::g->dc_texturemid = ::g->dc_texturemid + ::g->s_textureheight[texnum] - ::g->viewz;
    }
    else
    {
	::g->dc_texturemid =::g->frontsector->ceilingheight < ::g->backsector->ceilingheight
	    ? ::g->frontsector->ceilingheight : ::g->backsector->ceilingheight;
	::g->dc_texturemid = ::g->dc_texturemid - ::g->viewz;
    }
    ::g->dc_texturemid += ::g->curline->sidedef->rowoffset;
			
    if (::g->fixedcolormap)
	::g->dc_colormap = ::g->fixedcolormap;
    
    // draw the columns
    for (::g->dc_x = x1 ; ::g->dc_x <= x2 ; ::g->dc_x++)
    {
	// calculate lighting
	if (::g->maskedtexturecol[::g->dc_x] != SHRT_MAX)
	{
	    if (!::g->fixedcolormap)
	    {
		index = ::g->spryscale>>LIGHTSCALESHIFT;

		if (index >=  MAXLIGHTSCALE )
		    index = MAXLIGHTSCALE-1;

		::g->dc_colormap = ::g->walllights[index];
	    }
			
	    ::g->sprtopscreen = ::g->centeryfrac - FixedMul(::g->dc_texturemid, ::g->spryscale);
	    ::g->dc_iscale = 0xffffffffu / (unsigned)::g->spryscale;
	    
	    // draw the texture
	    col = (postColumn_t *)( 
		(byte *)R_GetColumn(texnum,::g->maskedtexturecol[::g->dc_x]) -3);
			
	    R_DrawMaskedColumn (col);
	    ::g->maskedtexturecol[::g->dc_x] = SHRT_MAX;
	}
	::g->spryscale += ::g->rw_scalestep;
    }
	
}




//
// R_RenderSegLoop
// Draws zero, one, or two textures (and possibly a masked
//  texture) for walls.
// Can draw or mark the starting pixel of floor and ceiling
//  textures.
// CALLED: CORE LOOPING ROUTINE.
//

void R_RenderSegLoop (void)
{
    angle_t		angle;
    unsigned		index;
    int			yl;
    int			yh;
    int			mid;
    fixed_t		texturecolumn;
    int			top;
    int			bottom;

    texturecolumn = 0;				// shut up compiler warning
	
    for ( ; ::g->rw_x < ::g->rw_stopx ; ::g->rw_x++)
    {
		// mark floor / ceiling areas
		yl = (::g->topfrac+HEIGHTUNIT-1)>>HEIGHTBITS;

		// no space above wall?
		if (yl < ::g->ceilingclip[::g->rw_x]+1)
			yl = ::g->ceilingclip[::g->rw_x]+1;
		
		if (::g->markceiling)
		{
			top = ::g->ceilingclip[::g->rw_x]+1;
			bottom = yl-1;

			if (bottom >= ::g->floorclip[::g->rw_x])
				bottom = ::g->floorclip[::g->rw_x]-1;

			if (top <= bottom)
			{
				::g->ceilingplane->top[::g->rw_x] = top;
				::g->ceilingplane->bottom[::g->rw_x] = bottom;
			}
		}
		
		yh = ::g->bottomfrac>>HEIGHTBITS;

		if (yh >= ::g->floorclip[::g->rw_x])
			yh = ::g->floorclip[::g->rw_x]-1;

		if (::g->markfloor)
		{
			top = yh+1;
			bottom = ::g->floorclip[::g->rw_x]-1;
			if (top <= ::g->ceilingclip[::g->rw_x])
				top = ::g->ceilingclip[::g->rw_x]+1;
			if (top <= bottom)
			{
				::g->floorplane->top[::g->rw_x] = top;
				::g->floorplane->bottom[::g->rw_x] = bottom;
			}
		}
		
	// texturecolumn and lighting are independent of wall tiers
	if (::g->segtextured)
	{
	    // calculate texture offset
	    angle = (::g->rw_centerangle + ::g->xtoviewangle[::g->rw_x])>>ANGLETOFINESHIFT;
	    texturecolumn = ::g->rw_offset-FixedMul(finetangent[angle],::g->rw_distance);
	    texturecolumn >>= FRACBITS;
	    // calculate lighting
	    index = ::g->rw_scale>>LIGHTSCALESHIFT;

	    if (index >=  MAXLIGHTSCALE )
			index = MAXLIGHTSCALE-1;

	    ::g->dc_colormap = ::g->walllights[index];
	    ::g->dc_x = ::g->rw_x;
	    ::g->dc_iscale = 0xffffffffu / (unsigned)::g->rw_scale;
	}
	
	// draw the wall tiers
	if (::g->midtexture)
	{
	    // single sided line
	    ::g->dc_yl = yl;
	    ::g->dc_yh = yh;
	    ::g->dc_texturemid = ::g->rw_midtexturemid;
	    ::g->dc_source = R_GetColumn(::g->midtexture,texturecolumn);
	    colfunc ( ::g->dc_colormap, ::g->dc_source );
	    ::g->ceilingclip[::g->rw_x] = ::g->viewheight;
	    ::g->floorclip[::g->rw_x] = -1;
	}
	else
	{
	    // two sided line
	    if (::g->toptexture)
	    {
			// top wall
			mid = ::g->pixhigh>>HEIGHTBITS;
			::g->pixhigh += ::g->pixhighstep;

			if (mid >= ::g->floorclip[::g->rw_x])
				mid = ::g->floorclip[::g->rw_x]-1;

			if (mid >= yl)
			{
				::g->dc_yl = yl;
				::g->dc_yh = mid;
				::g->dc_texturemid = ::g->rw_toptexturemid;
				::g->dc_source = R_GetColumn(::g->toptexture,texturecolumn);
				colfunc ( ::g->dc_colormap, ::g->dc_source );
				::g->ceilingclip[::g->rw_x] = mid;
			}
			else
				::g->ceilingclip[::g->rw_x] = yl-1;
		}
	    else
	    {
			// no top wall
			if (::g->markceiling)
				::g->ceilingclip[::g->rw_x] = yl-1;
		}
			
	    if (::g->bottomtexture)
	    {
			// bottom wall
			mid = (::g->pixlow+HEIGHTUNIT-1)>>HEIGHTBITS;
			::g->pixlow += ::g->pixlowstep;

			// no space above wall?
			if (mid <= ::g->ceilingclip[::g->rw_x])
				mid = ::g->ceilingclip[::g->rw_x]+1;
			
			if (mid <= yh)
			{
				::g->dc_yl = mid;
				::g->dc_yh = yh;
				::g->dc_texturemid = ::g->rw_bottomtexturemid;
				::g->dc_source = R_GetColumn(::g->bottomtexture,
							texturecolumn);
				colfunc ( ::g->dc_colormap, ::g->dc_source );
				::g->floorclip[::g->rw_x] = mid;
			}
			else
				::g->floorclip[::g->rw_x] = yh+1;
	    }
	    else
	    {
			// no bottom wall
			if (::g->markfloor)
				::g->floorclip[::g->rw_x] = yh+1;
			}
			
			if (::g->maskedtexture)
			{
				// save texturecol
				//  for backdrawing of masked mid texture
				::g->maskedtexturecol[::g->rw_x] = texturecolumn;
			}
		}
		
		::g->rw_scale += ::g->rw_scalestep;
		::g->topfrac += ::g->topstep;
		::g->bottomfrac += ::g->bottomstep;
    }
}




//
// R_StoreWallRange
// A wall segment will be drawn
//  between start and stop pixels (inclusive).
//
void
R_StoreWallRange
( int	start,
  int	stop )
{
    fixed_t		hyp;
    fixed_t		sineval;
    angle_t		distangle, offsetangle;
    fixed_t		vtop;
    int			lightnum;

    // don't overflow and crash
    if (::g->ds_p == &::g->drawsegs[MAXDRAWSEGS])
	return;		
		
#ifdef RANGECHECK
    if (start >=::g->viewwidth || start > stop)
	I_Error ("Bad R_RenderWallRange: %i to %i", start , stop);
#endif
    
    ::g->sidedef = ::g->curline->sidedef;
    ::g->linedef = ::g->curline->linedef;

    // mark the segment as visible for auto map
    ::g->linedef->flags |= ML_MAPPED;
    
    // calculate ::g->rw_distance for scale calculation
    ::g->rw_normalangle = ::g->curline->angle + ANG90;
	offsetangle = abs((long)(::g->rw_normalangle-::g->rw_angle1));
    
    if (offsetangle > ANG90)
	offsetangle = ANG90;

    distangle = ANG90 - offsetangle;
    hyp = R_PointToDist (::g->curline->v1->x, ::g->curline->v1->y);
    sineval = finesine[distangle>>ANGLETOFINESHIFT];
    ::g->rw_distance = FixedMul (hyp, sineval);
		
	
    ::g->ds_p->x1 = ::g->rw_x = start;
    ::g->ds_p->x2 = stop;
    ::g->ds_p->curline = ::g->curline;
    ::g->rw_stopx = stop+1;
    
    // calculate scale at both ends and step
	extern angle_t GetViewAngle();
    ::g->ds_p->scale1 = ::g->rw_scale = 
	R_ScaleFromGlobalAngle (GetViewAngle() + ::g->xtoviewangle[start]);
    
    if (stop > start )
    {
	::g->ds_p->scale2 = R_ScaleFromGlobalAngle (GetViewAngle() + ::g->xtoviewangle[stop]);
	::g->ds_p->scalestep = ::g->rw_scalestep = 
	    (::g->ds_p->scale2 - ::g->rw_scale) / (stop-start);
    }
    else
    {
	// UNUSED: try to fix the stretched line bug
#if 0
	if (::g->rw_distance < FRACUNIT/2)
	{
	    fixed_t		trx,try;
	    fixed_t		gxt,gyt;

	extern fixed_t GetViewX(); extern fixed_t GetViewY(); 
	    trx = ::g->curline->v1->x - GetViewX();
	    try = ::g->curline->v1->y - GetVewY();
			
	    gxt = FixedMul(trx,::g->viewcos); 
	    gyt = -FixedMul(try,::g->viewsin); 
	    ::g->ds_p->scale1 = FixedDiv(::g->projection, gxt-gyt) << ::g->detailshift;
	}
#endif
	::g->ds_p->scale2 = ::g->ds_p->scale1;
    }
    
    // calculate texture boundaries
    //  and decide if floor / ceiling marks are needed
    ::g->worldtop = ::g->frontsector->ceilingheight - ::g->viewz;
    ::g->worldbottom = ::g->frontsector->floorheight - ::g->viewz;
	
    ::g->midtexture = ::g->toptexture = ::g->bottomtexture = ::g->maskedtexture = 0;
    ::g->ds_p->maskedtexturecol = NULL;
	
    if (!::g->backsector)
    {
	// single sided line
	::g->midtexture = ::g->texturetranslation[::g->sidedef->midtexture];
	// a single sided line is terminal, so it must mark ends
	::g->markfloor = ::g->markceiling = true;
	if (::g->linedef->flags & ML_DONTPEGBOTTOM)
	{
	    vtop = ::g->frontsector->floorheight +
		::g->s_textureheight[::g->sidedef->midtexture];
	    // bottom of texture at bottom
	    ::g->rw_midtexturemid = vtop - ::g->viewz;	
	}
	else
	{
	    // top of texture at top
	    ::g->rw_midtexturemid = ::g->worldtop;
	}
	::g->rw_midtexturemid += ::g->sidedef->rowoffset;

	::g->ds_p->silhouette = SIL_BOTH;
	::g->ds_p->sprtopclip = ::g->screenheightarray;
	::g->ds_p->sprbottomclip = ::g->negonearray;
	::g->ds_p->bsilheight = MAXINT;
	::g->ds_p->tsilheight = MININT;
    }
    else
    {
	// two sided line
	::g->ds_p->sprtopclip = ::g->ds_p->sprbottomclip = NULL;
	::g->ds_p->silhouette = 0;
	
	if (::g->frontsector->floorheight > ::g->backsector->floorheight)
	{
	    ::g->ds_p->silhouette = SIL_BOTTOM;
	    ::g->ds_p->bsilheight = ::g->frontsector->floorheight;
	}
	else if (::g->backsector->floorheight > ::g->viewz)
	{
	    ::g->ds_p->silhouette = SIL_BOTTOM;
	    ::g->ds_p->bsilheight = MAXINT;
	    // ::g->ds_p->sprbottomclip = ::g->negonearray;
	}
	
	if (::g->frontsector->ceilingheight < ::g->backsector->ceilingheight)
	{
	    ::g->ds_p->silhouette |= SIL_TOP;
	    ::g->ds_p->tsilheight = ::g->frontsector->ceilingheight;
	}
	else if (::g->backsector->ceilingheight < ::g->viewz)
	{
	    ::g->ds_p->silhouette |= SIL_TOP;
	    ::g->ds_p->tsilheight = MININT;
	    // ::g->ds_p->sprtopclip = ::g->screenheightarray;
	}
		
	if (::g->backsector->ceilingheight <= ::g->frontsector->floorheight)
	{
	    ::g->ds_p->sprbottomclip = ::g->negonearray;
	    ::g->ds_p->bsilheight = MAXINT;
	    ::g->ds_p->silhouette |= SIL_BOTTOM;
	}
	
	if (::g->backsector->floorheight >= ::g->frontsector->ceilingheight)
	{
	    ::g->ds_p->sprtopclip = ::g->screenheightarray;
	    ::g->ds_p->tsilheight = MININT;
	    ::g->ds_p->silhouette |= SIL_TOP;
	}
	
	::g->worldhigh = ::g->backsector->ceilingheight - ::g->viewz;
	::g->worldlow = ::g->backsector->floorheight - ::g->viewz;
		
	// hack to allow height changes in outdoor areas
	if (::g->frontsector->ceilingpic == ::g->skyflatnum 
	    && ::g->backsector->ceilingpic == ::g->skyflatnum)
	{
	    ::g->worldtop = ::g->worldhigh;
	}
	
			
	if (::g->worldlow != ::g->worldbottom 
	    || ::g->backsector->floorpic != ::g->frontsector->floorpic
	    || ::g->backsector->lightlevel != ::g->frontsector->lightlevel)
	{
	    ::g->markfloor = true;
	}
	else
	{
	    // same plane on both ::g->sides
	    ::g->markfloor = false;
	}
	
			
	if (::g->worldhigh != ::g->worldtop 
	    || ::g->backsector->ceilingpic != ::g->frontsector->ceilingpic
	    || ::g->backsector->lightlevel != ::g->frontsector->lightlevel)
	{
	    ::g->markceiling = true;
	}
	else
	{
	    // same plane on both ::g->sides
	    ::g->markceiling = false;
	}
	
	if (::g->backsector->ceilingheight <= ::g->frontsector->floorheight
	    || ::g->backsector->floorheight >= ::g->frontsector->ceilingheight)
	{
	    // closed door
	    ::g->markceiling = ::g->markfloor = true;
	}
	

	if (::g->worldhigh < ::g->worldtop)
	{
	    // top texture
	    ::g->toptexture = ::g->texturetranslation[::g->sidedef->toptexture];
	    if (::g->linedef->flags & ML_DONTPEGTOP)
	    {
		// top of texture at top
		::g->rw_toptexturemid = ::g->worldtop;
	    }
	    else
	    {
		vtop =
		    ::g->backsector->ceilingheight
		    + ::g->s_textureheight[::g->sidedef->toptexture];
		
		// bottom of texture
		::g->rw_toptexturemid = vtop - ::g->viewz;	
	    }
	}
	if (::g->worldlow > ::g->worldbottom)
	{
	    // bottom texture
	    ::g->bottomtexture = ::g->texturetranslation[::g->sidedef->bottomtexture];

	    if (::g->linedef->flags & ML_DONTPEGBOTTOM )
	    {
		// bottom of texture at bottom
		// top of texture at top
		::g->rw_bottomtexturemid = ::g->worldtop;
	    }
	    else	// top of texture at top
		::g->rw_bottomtexturemid = ::g->worldlow;
	}
	::g->rw_toptexturemid += ::g->sidedef->rowoffset;
	::g->rw_bottomtexturemid += ::g->sidedef->rowoffset;
	
	// allocate space for masked texture tables
	if (::g->sidedef->midtexture)
	{
	    // masked ::g->midtexture
	    ::g->maskedtexture = true;
	    ::g->ds_p->maskedtexturecol = ::g->maskedtexturecol = ::g->lastopening - ::g->rw_x;
	    ::g->lastopening += ::g->rw_stopx - ::g->rw_x;
	}
    }
    
    // calculate ::g->rw_offset (only needed for textured ::g->lines)
    ::g->segtextured = ::g->midtexture | ::g->toptexture | ::g->bottomtexture | ::g->maskedtexture;

    if (::g->segtextured)
    {
		offsetangle = ::g->rw_normalangle-::g->rw_angle1;
		
		if (offsetangle > ANG180)
			offsetangle = -offsetangle; // ALANHACK UNSIGNED

		if (offsetangle > ANG90)
			offsetangle = ANG90;

		sineval = finesine[offsetangle >>ANGLETOFINESHIFT];
		::g->rw_offset = FixedMul (hyp, sineval);

		if (::g->rw_normalangle-::g->rw_angle1 < ANG180)
			::g->rw_offset = -::g->rw_offset;

		::g->rw_offset += ::g->sidedef->textureoffset + ::g->curline->offset;
		::g->rw_centerangle = ANG90 + GetViewAngle() - ::g->rw_normalangle;
		
		// calculate light table
		//  use different light tables
		//  for horizontal / vertical / diagonal
		// OPTIMIZE: get rid of LIGHTSEGSHIFT globally
		if (!::g->fixedcolormap)
		{
			lightnum = (::g->frontsector->lightlevel >> LIGHTSEGSHIFT)+::g->extralight;

			if (::g->curline->v1->y == ::g->curline->v2->y)
			lightnum--;
			else if (::g->curline->v1->x == ::g->curline->v2->x)
			lightnum++;

			if (lightnum < 0)		
			::g->walllights = ::g->scalelight[0];
			else if (lightnum >= LIGHTLEVELS)
			::g->walllights = ::g->scalelight[LIGHTLEVELS-1];
			else
			::g->walllights = ::g->scalelight[lightnum];
		}
    }
    
    // if a floor / ceiling plane is on the wrong side
    //  of the view plane, it is definitely invisible
    //  and doesn't need to be marked.
    
  
    if (::g->frontsector->floorheight >= ::g->viewz)
    {
	// above view plane
	::g->markfloor = false;
    }
    
    if (::g->frontsector->ceilingheight <= ::g->viewz 
	&& ::g->frontsector->ceilingpic != ::g->skyflatnum)
    {
	// below view plane
	::g->markceiling = false;
    }

    
    // calculate incremental stepping values for texture edges
    ::g->worldtop >>= 4;
    ::g->worldbottom >>= 4;
	
    ::g->topstep = -FixedMul (::g->rw_scalestep, ::g->worldtop);
    ::g->topfrac = (::g->centeryfrac>>4) - FixedMul (::g->worldtop, ::g->rw_scale);

    ::g->bottomstep = -FixedMul (::g->rw_scalestep,::g->worldbottom);
    ::g->bottomfrac = (::g->centeryfrac>>4) - FixedMul (::g->worldbottom, ::g->rw_scale);
	
    if (::g->backsector)
    {	
	::g->worldhigh >>= 4;
	::g->worldlow >>= 4;

	if (::g->worldhigh < ::g->worldtop)
	{
	    ::g->pixhigh = (::g->centeryfrac>>4) - FixedMul (::g->worldhigh, ::g->rw_scale);
	    ::g->pixhighstep = -FixedMul (::g->rw_scalestep,::g->worldhigh);
	}
	
	if (::g->worldlow > ::g->worldbottom)
	{
	    ::g->pixlow = (::g->centeryfrac>>4) - FixedMul (::g->worldlow, ::g->rw_scale);
	    ::g->pixlowstep = -FixedMul (::g->rw_scalestep,::g->worldlow);
	}
    }
    
    // render it
	 if (::g->markceiling)
		 ::g->ceilingplane = R_CheckPlane (::g->ceilingplane, ::g->rw_x, ::g->rw_stopx-1);

	 if (::g->markfloor)
		 ::g->floorplane = R_CheckPlane (::g->floorplane, ::g->rw_x, ::g->rw_stopx-1);

    R_RenderSegLoop ();

    
    // save sprite clipping info
    if ( ((::g->ds_p->silhouette & SIL_TOP) || ::g->maskedtexture)
	 && !::g->ds_p->sprtopclip)
    {
	memcpy (::g->lastopening, ::g->ceilingclip+start, 2*(::g->rw_stopx-start));
	::g->ds_p->sprtopclip = ::g->lastopening - start;
	::g->lastopening += ::g->rw_stopx - start;
    }
    
    if ( ((::g->ds_p->silhouette & SIL_BOTTOM) || ::g->maskedtexture)
	 && !::g->ds_p->sprbottomclip)
    {
	memcpy (::g->lastopening, ::g->floorclip+start, 2*(::g->rw_stopx-start));
	::g->ds_p->sprbottomclip = ::g->lastopening - start;
	::g->lastopening += ::g->rw_stopx - start;	
    }

    if (::g->maskedtexture && !(::g->ds_p->silhouette&SIL_TOP))
    {
	::g->ds_p->silhouette |= SIL_TOP;
	::g->ds_p->tsilheight = MININT;
    }
    if (::g->maskedtexture && !(::g->ds_p->silhouette&SIL_BOTTOM))
    {
	::g->ds_p->silhouette |= SIL_BOTTOM;
	::g->ds_p->bsilheight = MAXINT;
    }
    ::g->ds_p++;
}


