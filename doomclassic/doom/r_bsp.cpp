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

#include "m_bbox.h"

#include "i_system.h"

#include "r_main.h"
#include "r_plane.h"
#include "r_things.h"

// State.
#include "doomstat.h"
#include "r_state.h"

//#include "r_local.h"






void
R_StoreWallRange
( int	start,
  int	stop );




//
// R_ClearDrawSegs
//
void R_ClearDrawSegs (void)
{
    ::g->ds_p = ::g->drawsegs;
}



//
// ClipWallSegment
// Clips the given range of columns
// and includes it in the new clip list.
//



// ::g->newend is one past the last valid seg




//
// R_ClipSolidWallSegment
// Does handle solid walls,
//  e.g. single sided LineDefs (middle texture)
//  that entirely block the view.
// 
void
R_ClipSolidWallSegment
( int			first,
  int			last )
{
    cliprange_t*	next;
    cliprange_t*	start;

    // Find the first range that touches the range
    //  (adjacent pixels are touching).
    start = ::g->solidsegs;
    while (start->last < first-1)
		start++;

    if (first < start->first)
    {
	if (last < start->first-1)
	{
	    // Post is entirely visible (above start),
	    //  so insert a new clippost.
	    R_StoreWallRange (first, last);
	    next = ::g->newend;
	    ::g->newend++;
	    
	    while (next != start)
	    {
		*next = *(next-1);
		next--;
	    }
	    next->first = first;
	    next->last = last;
	    return;
	}
		
	// There is a fragment above *start.
	R_StoreWallRange (first, start->first - 1);
	// Now adjust the clip size.
	start->first = first;	
    }

    // Bottom contained in start?
    if (last <= start->last)
	return;			
		
    next = start;
    while (last >= (next+1)->first-1)
    {
	// There is a fragment between two posts.
	R_StoreWallRange (next->last + 1, (next+1)->first - 1);
	next++;
	
	if (last <= next->last)
	{
	    // Bottom is contained in next.
	    // Adjust the clip size.
	    start->last = next->last;	
	    goto crunch;
	}
    }
	
    // There is a fragment after *next.
    R_StoreWallRange (next->last + 1, last);
    // Adjust the clip size.
    start->last = last;
	
    // Remove start+1 to next from the clip list,
    // because start now covers their area.
  crunch:
    if (next == start)
    {
	// Post just extended past the bottom of one post.
	return;
    }
    

    while (next++ != ::g->newend)
    {
	// Remove a post.
	*++start = *next;
    }

    ::g->newend = start+1;
}



//
// R_ClipPassWallSegment
// Clips the given range of columns,
//  but does not includes it in the clip list.
// Does handle windows,
//  e.g. LineDefs with upper and lower texture.
//
void
R_ClipPassWallSegment
( int	first,
  int	last )
{
    cliprange_t*	start;

    // Find the first range that touches the range
    //  (adjacent pixels are touching).
    start = ::g->solidsegs;
    while (start->last < first-1)
	start++;

    if (first < start->first)
    {
	if (last < start->first-1)
	{
	    // Post is entirely visible (above start).
	    R_StoreWallRange (first, last);
	    return;
	}
		
	// There is a fragment above *start.
	R_StoreWallRange (first, start->first - 1);
    }

    // Bottom contained in start?
    if (last <= start->last)
	return;			
		
    while (last >= (start+1)->first-1)
    {
	// There is a fragment between two posts.
	R_StoreWallRange (start->last + 1, (start+1)->first - 1);
	start++;
	
	if (last <= start->last)
	    return;
    }
	
    // There is a fragment after *next.
    R_StoreWallRange (start->last + 1, last);
}



//
// R_ClearClipSegs
//
void R_ClearClipSegs (void)
{
    ::g->solidsegs[0].first = -0x7fffffff;
    ::g->solidsegs[0].last = -1;
    ::g->solidsegs[1].first = ::g->viewwidth;
    ::g->solidsegs[1].last = 0x7fffffff;
    ::g->newend = ::g->solidsegs+2;
}

//
// R_AddLine
// Clips the given segment
// and adds any visible pieces to the line list.
//
void R_AddLine (seg_t*	line)
{
    int			x1;
    int			x2;
    angle_t		angle1;
    angle_t		angle2;
    angle_t		span;
    angle_t		tspan;
    
    ::g->curline = line;

    // OPTIMIZE: quickly reject orthogonal back ::g->sides.
    angle1 = R_PointToAngle (line->v1->x, line->v1->y);
    angle2 = R_PointToAngle (line->v2->x, line->v2->y);
    
    // Clip to view edges.
    // OPTIMIZE: make constant out of 2*::g->clipangle (FIELDOFVIEW).
    span = angle1 - angle2;
    
    // Back side? I.e. backface culling?
    if (span >= ANG180)
		return;		

	extern angle_t GetViewAngle();
    // Global angle needed by segcalc.
    ::g->rw_angle1 = angle1;
    angle1 -= GetViewAngle();
    angle2 -= GetViewAngle();
	
    tspan = angle1 + ::g->clipangle;
    if (tspan > 2*::g->clipangle)
    {
		tspan -= 2*::g->clipangle;

		// Totally off the left edge?
		if (tspan >= span)
			return;
		
		angle1 = ::g->clipangle;
    }
    tspan = ::g->clipangle - angle2;
    if (tspan > 2*::g->clipangle)
    {
		tspan -= 2*::g->clipangle;

		// Totally off the left edge?
		if (tspan >= span)
			return;	
		angle2 = -::g->clipangle; // ALANHACK UNSIGNED
    }
    
    // The seg is in the view range,
    // but not necessarily visible.
    angle1 = (angle1+ANG90)>>ANGLETOFINESHIFT;
    angle2 = (angle2+ANG90)>>ANGLETOFINESHIFT;
    x1 = ::g->viewangletox[angle1];
    x2 = ::g->viewangletox[angle2];

    // Does not cross a pixel?
    if (x1 == x2)
		return;				
	
    ::g->backsector = line->backsector;

    // Single sided line?
    if (!::g->backsector)
		goto clipsolid;		

    // Closed door.
    if (::g->backsector->ceilingheight <= ::g->frontsector->floorheight
	|| ::g->backsector->floorheight >= ::g->frontsector->ceilingheight)
		goto clipsolid;		

    // Window.
    if (::g->backsector->ceilingheight != ::g->frontsector->ceilingheight
	|| ::g->backsector->floorheight != ::g->frontsector->floorheight)
		goto clippass;	
		
    // Reject empty ::g->lines used for triggers
    //  and special ::g->events.
    // Identical floor and ceiling on both ::g->sides,
    // identical light levels on both ::g->sides,
    // and no middle texture.
    if (::g->backsector->ceilingpic == ::g->frontsector->ceilingpic
	&& ::g->backsector->floorpic == ::g->frontsector->floorpic
	&& ::g->backsector->lightlevel == ::g->frontsector->lightlevel
	&& ::g->curline->sidedef->midtexture == 0)
    {
		return;
    }
    
				
  clippass:
    R_ClipPassWallSegment (x1, x2-1);	
    return;
		
  clipsolid:
    R_ClipSolidWallSegment (x1, x2-1);
}


//
// R_CheckBBox
// Checks BSP node/subtree bounding box.
// Returns true
//  if some part of the bbox might be visible.
//


qboolean R_CheckBBox (fixed_t*	bspcoord)
{
    int			boxx;
    int			boxy;
    int			boxpos;

    fixed_t		x1;
    fixed_t		y1;
    fixed_t		x2;
    fixed_t		y2;
    
    angle_t		angle1;
    angle_t		angle2;
    angle_t		span;
    angle_t		tspan;
    
    cliprange_t*	start;

    int			sx1;
    int			sx2;

	extern fixed_t GetViewX(); extern fixed_t GetViewY();
    // Find the corners of the box
    // that define the edges from current viewpoint.
    if (GetViewX() <= bspcoord[BOXLEFT])
	boxx = 0;
    else if (GetViewX() < bspcoord[BOXRIGHT])
	boxx = 1;
    else
	boxx = 2;
		
    if (GetViewY() >= bspcoord[BOXTOP])
	boxy = 0;
    else if (GetViewY() > bspcoord[BOXBOTTOM])
	boxy = 1;
    else
	boxy = 2;
		
    boxpos = (boxy<<2)+boxx;
    if (boxpos == 5)
	return true;
	
    x1 = bspcoord[::g->checkcoord[boxpos][0]];
    y1 = bspcoord[::g->checkcoord[boxpos][1]];
    x2 = bspcoord[::g->checkcoord[boxpos][2]];
    y2 = bspcoord[::g->checkcoord[boxpos][3]];
    
    // check clip list for an open space
	extern angle_t GetViewAngle();
    angle1 = R_PointToAngle (x1, y1) - GetViewAngle();
    angle2 = R_PointToAngle (x2, y2) - GetViewAngle();
	
    span = angle1 - angle2;

    // Sitting on a line?
    if (span >= ANG180)
	return true;
    
    tspan = angle1 + ::g->clipangle;

    if (tspan > 2*::g->clipangle)
    {
	tspan -= 2*::g->clipangle;

	// Totally off the left edge?
	if (tspan >= span)
	    return false;	

	angle1 = ::g->clipangle;
    }
    tspan = ::g->clipangle - angle2;
    if (tspan > 2*::g->clipangle)
    {
	tspan -= 2*::g->clipangle;

	// Totally off the left edge?
	if (tspan >= span)
	    return false;
	
	angle2 = -::g->clipangle;// ALANHACK UNSIGNED
    }


    // Find the first clippost
    //  that touches the source post
    //  (adjacent pixels are touching).
    angle1 = (angle1+ANG90)>>ANGLETOFINESHIFT;
    angle2 = (angle2+ANG90)>>ANGLETOFINESHIFT;
    sx1 = ::g->viewangletox[angle1];
    sx2 = ::g->viewangletox[angle2];

    // Does not cross a pixel.
    if (sx1 == sx2)
	return false;			
    sx2--;
	
    start = ::g->solidsegs;
    while (start->last < sx2)
	start++;
    
    if (sx1 >= start->first
	&& sx2 <= start->last)
    {
	// The clippost contains the new span.
	return false;
    }

    return true;
}



//
// R_Subsector
// Determine floor/ceiling planes.
// Add ::g->sprites of things in sector.
// Draw one or more line segments.
//
void R_Subsector (int num)
{
    int			count;
    seg_t*		line;
    subsector_t*	sub;
	
#ifdef RANGECHECK
    if (num>=::g->numsubsectors)
	I_Error ("R_Subsector: ss %i with numss = %i",
		 num,
		 ::g->numsubsectors);
#endif

    ::g->sscount++;
    sub = &::g->subsectors[num];
    ::g->frontsector = sub->sector;
    count = sub->numlines;
    line = &::g->segs[sub->firstline];

    if (::g->frontsector->floorheight < ::g->viewz)
    {
		::g->floorplane = R_FindPlane (::g->frontsector->floorheight,
					::g->frontsector->floorpic,
					::g->frontsector->lightlevel);
    }
    else
		::g->floorplane = NULL;
    
    if (::g->frontsector->ceilingheight > ::g->viewz 
	|| ::g->frontsector->ceilingpic == ::g->skyflatnum)
    {
		 ::g->ceilingplane = R_FindPlane (::g->frontsector->ceilingheight,
						::g->frontsector->ceilingpic,
						::g->frontsector->lightlevel);
    }
    else
		::g->ceilingplane = NULL;
		
    R_AddSprites (::g->frontsector);	

    while (count--)
    {
		R_AddLine (line);
		line++;
    }
}




//
// RenderBSPNode
// Renders all ::g->subsectors below a given node,
//  traversing subtree recursively.
// Just call with BSP root.
void R_RenderBSPNode (int bspnum)
{
    node_t*	bsp;
    int		side;

    // Found a subsector?
    if (bspnum & NF_SUBSECTOR)
    {
	if (bspnum == -1)			
	    R_Subsector (0);
	else
	    R_Subsector (bspnum&(~NF_SUBSECTOR));
	return;
    }
		
    bsp = &::g->nodes[bspnum];
    
	extern fixed_t GetViewX(); extern fixed_t GetViewY();
    // Decide which side the view point is on.
    side = R_PointOnSide (GetViewX(), GetViewY(), bsp);

    // Recursively divide front space.
    R_RenderBSPNode (bsp->children[side]); 

    // Possibly divide back space.
    if (R_CheckBBox (bsp->bbox[side^1]))	
	R_RenderBSPNode (bsp->children[side^1]);
}



