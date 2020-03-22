/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.
Copyright (C) 2015 Robert Beckebans

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "precompiled.h"
#pragma hdrstop

#include "dmap.h"


idList<interAreaPortal_t> interAreaPortals;


int		c_active_portals;
int		c_peak_portals;

/*
===========
AllocPortal
===========
*/
uPortal_t*	AllocPortal()
{
	uPortal_t*	p;

	c_active_portals++;
	if( c_active_portals > c_peak_portals )
	{
		c_peak_portals = c_active_portals;
	}

	p = ( uPortal_t* )Mem_Alloc( sizeof( uPortal_t ), TAG_TOOLS );
	memset( p, 0, sizeof( uPortal_t ) );

	return p;
}


void FreePortal( uPortal_t*  p )
{
	if( p->winding )
	{
		delete p->winding;
	}
	c_active_portals--;
	Mem_Free( p );
}

//==============================================================

/*
=============
Portal_Passable

Returns true if the portal has non-opaque leafs on both sides
=============
*/
static bool Portal_Passable( uPortal_t*  p )
{
	if( !p->onnode )
	{
		return false;	// to global outsideleaf
	}

	if( p->nodes[0]->planenum != PLANENUM_LEAF
			|| p->nodes[1]->planenum != PLANENUM_LEAF )
	{
		common->Error( "Portal_EntityFlood: not a leaf" );
	}

	if( !p->nodes[0]->opaque && !p->nodes[1]->opaque )
	{
		return true;
	}

	return false;
}


//=============================================================================

int		c_tinyportals;

/*
=============
AddPortalToNodes
=============
*/
void AddPortalToNodes( uPortal_t*  p, node_t* front, node_t* back )
{
	if( p->nodes[0] || p->nodes[1] )
	{
		common->Error( "AddPortalToNode: allready included" );
	}

	p->nodes[0] = front;
	p->next[0] = front->portals;
	front->portals = p;

	p->nodes[1] = back;
	p->next[1] = back->portals;
	back->portals = p;
}


/*
=============
RemovePortalFromNode
=============
*/
void RemovePortalFromNode( uPortal_t*  portal, node_t* l )
{
	uPortal_t**	pp, *t;

// remove reference to the current portal
	pp = &l->portals;
	while( 1 )
	{
		t = *pp;
		if( !t )
		{
			common->Error( "RemovePortalFromNode: portal not in leaf" );
		}

		if( t == portal )
		{
			break;
		}

		if( t->nodes[0] == l )
		{
			pp = &t->next[0];
		}
		else if( t->nodes[1] == l )
		{
			pp = &t->next[1];
		}
		else
		{
			common->Error( "RemovePortalFromNode: portal not bounding leaf" );
		}
	}

	if( portal->nodes[0] == l )
	{
		*pp = portal->next[0];
		portal->nodes[0] = NULL;
	}
	else if( portal->nodes[1] == l )
	{
		*pp = portal->next[1];
		portal->nodes[1] = NULL;
	}
	else
	{
		common->Error( "RemovePortalFromNode: mislinked" );
	}
}

//============================================================================

void PrintPortal( uPortal_t* p )
{
	int			i;
	idWinding*	w;

	w = p->winding;
	for( i = 0; i < w->GetNumPoints(); i++ )
	{
		common->Printf( "(%5.0f,%5.0f,%5.0f)\n", ( *w )[i][0], ( *w )[i][1], ( *w )[i][2] );
	}
}

/*
================
MakeHeadnodePortals

The created portals will face the global outside_node
================
*/
#define	SIDESPACE	8
static void MakeHeadnodePortals( tree_t* tree )
{
	idBounds	bounds;
	int			i, j, n;
	uPortal_t*	p, *portals[6];
	idPlane		bplanes[6], *pl;
	node_t* node;

	node = tree->headnode;

	tree->outside_node.planenum = PLANENUM_LEAF;
	tree->outside_node.brushlist = NULL;
	tree->outside_node.portals = NULL;
	tree->outside_node.opaque = false;

	// if no nodes, don't go any farther
	if( node->planenum == PLANENUM_LEAF )
	{
		return;
	}

	// pad with some space so there will never be null volume leafs
	for( i = 0 ; i < 3 ; i++ )
	{
		bounds[0][i] = tree->bounds[0][i] - SIDESPACE;
		bounds[1][i] = tree->bounds[1][i] + SIDESPACE;
		if( bounds[0][i] >= bounds[1][i] )
		{
			common->Error( "Backwards tree volume" );
		}
	}

	for( i = 0 ; i < 3 ; i++ )
	{
		for( j = 0 ; j < 2 ; j++ )
		{
			n = j * 3 + i;

			p = AllocPortal();
			portals[n] = p;

			pl = &bplanes[n];
			memset( pl, 0, sizeof( *pl ) );
			if( j )
			{
				( *pl )[i] = -1;
				( *pl )[3] = bounds[j][i];
			}
			else
			{
				( *pl )[i] = 1;
				( *pl )[3] = -bounds[j][i];
			}
			p->plane = *pl;
			p->winding = new idWinding( *pl );
			AddPortalToNodes( p, node, &tree->outside_node );
		}
	}

	// clip the basewindings by all the other planes
	for( i = 0 ; i < 6 ; i++ )
	{
		for( j = 0 ; j < 6 ; j++ )
		{
			if( j == i )
			{
				continue;
			}
			portals[i]->winding = portals[i]->winding->Clip( bplanes[j], ON_EPSILON );
		}
	}
}

//===================================================


/*
================
BaseWindingForNode
================
*/
#define	BASE_WINDING_EPSILON	0.001f
#define	SPLIT_WINDING_EPSILON	0.001f

idWinding* BaseWindingForNode( node_t* node )
{
	idWinding*	w;
	node_t*		n;

	w = new idWinding( dmapGlobals.mapPlanes[node->planenum] );

	// clip by all the parents
	for( n = node->parent ; n && w ; )
	{
		idPlane& plane = dmapGlobals.mapPlanes[n->planenum];

		if( n->children[0] == node )
		{
			// take front
			w = w->Clip( plane, BASE_WINDING_EPSILON );
		}
		else
		{
			// take back
			idPlane	back = -plane;
			w = w->Clip( back, BASE_WINDING_EPSILON );
		}
		node = n;
		n = n->parent;
	}

	return w;
}

//============================================================

/*
==================
MakeNodePortal

create the new portal by taking the full plane winding for the cutting plane
and clipping it by all of parents of this node
==================
*/
static void MakeNodePortal( node_t* node )
{
	uPortal_t*	new_portal, *p;
	idWinding*	w;
	idVec3		normal;
	int			side;

	w = BaseWindingForNode( node );

	// clip the portal by all the other portals in the node
	for( p = node->portals ; p && w; p = p->next[side] )
	{
		idPlane	plane;

		if( p->nodes[0] == node )
		{
			side = 0;
			plane = p->plane;
		}
		else if( p->nodes[1] == node )
		{
			side = 1;
			plane = -p->plane;
		}
		else
		{
			common->Error( "CutNodePortals_r: mislinked portal" );
			side = 0;	// quiet a compiler warning
		}

		w = w->Clip( plane, CLIP_EPSILON );
	}

	if( !w )
	{
		return;
	}

	if( w->IsTiny() )
	{
		c_tinyportals++;
		delete w;
		return;
	}


	new_portal = AllocPortal();
	new_portal->plane = dmapGlobals.mapPlanes[node->planenum];
	new_portal->onnode = node;
	new_portal->winding = w;
	AddPortalToNodes( new_portal, node->children[0], node->children[1] );
}


/*
==============
SplitNodePortals

Move or split the portals that bound node so that the node's
children have portals instead of node.
==============
*/
static void SplitNodePortals( node_t* node )
{
	uPortal_t*	p, *next_portal, *new_portal;
	node_t*		f, *b, *other_node;
	int			side;
	idPlane*		plane;
	idWinding*	frontwinding, *backwinding;

	plane = &dmapGlobals.mapPlanes[node->planenum];
	f = node->children[0];
	b = node->children[1];

	for( p = node->portals ; p ; p = next_portal )
	{
		if( p->nodes[0] == node )
		{
			side = 0;
		}
		else if( p->nodes[1] == node )
		{
			side = 1;
		}
		else
		{
			common->Error( "SplitNodePortals: mislinked portal" );
			side = 0;	// quiet a compiler warning
		}
		next_portal = p->next[side];

		other_node = p->nodes[!side];
		RemovePortalFromNode( p, p->nodes[0] );
		RemovePortalFromNode( p, p->nodes[1] );

		//
		// cut the portal into two portals, one on each side of the cut plane
		//
		p->winding->Split( *plane, SPLIT_WINDING_EPSILON, &frontwinding, &backwinding );

		if( frontwinding && frontwinding->IsTiny() )
		{
			delete frontwinding;
			frontwinding = NULL;
			c_tinyportals++;
		}

		if( backwinding && backwinding->IsTiny() )
		{
			delete backwinding;
			backwinding = NULL;
			c_tinyportals++;
		}

		if( !frontwinding && !backwinding )
		{
			// tiny windings on both sides
			continue;
		}

		if( !frontwinding )
		{
			delete backwinding;
			if( side == 0 )
			{
				AddPortalToNodes( p, b, other_node );
			}
			else
			{
				AddPortalToNodes( p, other_node, b );
			}
			continue;
		}
		if( !backwinding )
		{
			delete frontwinding;
			if( side == 0 )
			{
				AddPortalToNodes( p, f, other_node );
			}
			else
			{
				AddPortalToNodes( p, other_node, f );
			}
			continue;
		}

		// the winding is split
		new_portal = AllocPortal();
		*new_portal = *p;
		new_portal->winding = backwinding;
		delete p->winding;
		p->winding = frontwinding;

		if( side == 0 )
		{
			AddPortalToNodes( p, f, other_node );
			AddPortalToNodes( new_portal, b, other_node );
		}
		else
		{
			AddPortalToNodes( p, other_node, f );
			AddPortalToNodes( new_portal, other_node, b );
		}
	}

	node->portals = NULL;
}


/*
================
CalcNodeBounds
================
*/
void CalcNodeBounds( node_t* node )
{
	uPortal_t*	p;
	int			s;
	int			i;

	// calc mins/maxs for both leafs and nodes
	node->bounds.Clear();
	for( p = node->portals ; p ; p = p->next[s] )
	{
		s = ( p->nodes[1] == node );
		for( i = 0; i < p->winding->GetNumPoints(); i++ )
		{
			node->bounds.AddPoint( ( *p->winding )[i].ToVec3() );
		}
	}
}


/*
==================
MakeTreePortals_r
==================
*/
void MakeTreePortals_r( node_t* node )
{
	int		i;

	CalcNodeBounds( node );

	if( node->bounds[0][0] >= node->bounds[1][0] )
	{
		common->Warning( "node without a volume" );
	}

	for( i = 0; i < 3; i++ )
	{
		if( node->bounds[0][i] < MIN_WORLD_COORD || node->bounds[1][i] > MAX_WORLD_COORD )
		{
			common->Warning( "node with unbounded volume" );
			break;
		}
	}
	if( node->planenum == PLANENUM_LEAF )
	{
		return;
	}

	MakeNodePortal( node );
	SplitNodePortals( node );

	MakeTreePortals_r( node->children[0] );
	MakeTreePortals_r( node->children[1] );
}

/*
==================
MakeTreePortals
==================
*/
void MakeTreePortals( tree_t* tree )
{
	common->Printf( "----- MakeTreePortals -----\n" );
	MakeHeadnodePortals( tree );
	MakeTreePortals_r( tree->headnode );
}

/*
=========================================================

FLOOD ENTITIES

=========================================================
*/

int		c_floodedleafs;

/*
=============
FloodPortals_r
=============
*/
void FloodPortals_r( node_t* node, int dist )
{
	uPortal_t*	p;
	int			s;

	if( node->occupied )
	{
		return;
	}

	if( node->opaque )
	{
		return;
	}

	c_floodedleafs++;
	node->occupied = dist;

	for( p = node->portals ; p ; p = p->next[s] )
	{
		s = ( p->nodes[1] == node );
		FloodPortals_r( p->nodes[!s], dist + 1 );
	}
}

/*
=============
PlaceOccupant
=============
*/
bool PlaceOccupant( node_t* headnode, idVec3 origin, uEntity_t* occupant )
{
	node_t*	node;
	float	d;
	idPlane*	plane;

	// find the leaf to start in
	node = headnode;
	while( node->planenum != PLANENUM_LEAF )
	{
		plane = &dmapGlobals.mapPlanes[node->planenum];
		d = plane->Distance( origin );
		if( d >= 0.0f )
		{
			node = node->children[0];
		}
		else
		{
			node = node->children[1];
		}
	}

	if( node->opaque )
	{
		return false;
	}
	node->occupant = occupant;

	FloodPortals_r( node, 1 );

	return true;
}

/*
=============
FloodEntities

Marks all nodes that can be reached by entites
=============
*/
bool FloodEntities( tree_t* tree )
{
	int		i;
	idVec3	origin;
	const char*	cl;
	bool	inside;
	node_t* headnode;

	headnode = tree->headnode;
	common->Printf( "--- FloodEntities ---\n" );
	inside = false;
	tree->outside_node.occupied = 0;

	c_floodedleafs = 0;
	bool errorShown = false;
	for( i = 1 ; i < dmapGlobals.num_entities ; i++ )
	{
		idMapEntity*	mapEnt;

		mapEnt = dmapGlobals.uEntities[i].mapEntity;
		if( !mapEnt->epairs.GetVector( "origin", "", origin ) )
		{
			continue;
		}

		// any entity can have "noFlood" set to skip it
		if( mapEnt->epairs.GetString( "noFlood", "", &cl ) )
		{
			continue;
		}

		mapEnt->epairs.GetString( "classname", "", &cl );

		if( !strcmp( cl, "light" ) )
		{
			const char*	v;

			// don't place lights that have a light_start field, because they can still
			// be valid if their origin is outside the world
			mapEnt->epairs.GetString( "light_start", "", &v );
			if( v[0] )
			{
				continue;
			}

			// don't place fog lights, because they often
			// have origins outside the light
			mapEnt->epairs.GetString( "texture", "", &v );
			if( v[0] )
			{
				const idMaterial* mat = declManager->FindMaterial( v );
				if( mat->IsFogLight() )
				{
					continue;
				}
			}
		}

		if( PlaceOccupant( headnode, origin, &dmapGlobals.uEntities[i] ) )
		{
			inside = true;
		}

		if( tree->outside_node.occupied && !errorShown )
		{
			errorShown = true;
			common->Printf( "Leak on entity # %d\n", i );
			const char* p;

			mapEnt->epairs.GetString( "classname", "", &p );
			common->Printf( "Entity classname was: %s\n", p );
			mapEnt->epairs.GetString( "name", "", &p );
			common->Printf( "Entity name was: %s\n", p );
			idVec3 origin;
			if( mapEnt->epairs.GetVector( "origin", "", origin ) )
			{
				common->Printf( "Entity origin is: %f %f %f\n\n\n", origin.x, origin.y, origin.z );
			}
		}
	}

	common->Printf( "%5i flooded leafs\n", c_floodedleafs );

	if( !inside )
	{
		common->Printf( "no entities in open -- no filling\n" );
	}
	else if( tree->outside_node.occupied )
	{
		common->Printf( "entity reached from outside -- no filling\n" );
	}

	return ( bool )( inside && !tree->outside_node.occupied );
}

/*
=========================================================

FLOOD AREAS

=========================================================
*/

static	int		c_areas;
static	int		c_areaFloods;

/*
=================
FindSideForPortal
=================
*/
static side_t*	FindSideForPortal( uPortal_t* p )
{
	int		i, j, k;
	node_t*	node;
	uBrush_t*	b, *orig;
	side_t*	s, *s2;

	// scan both bordering nodes brush lists for a portal brush
	// that shares the plane
	for( i = 0 ; i < 2 ; i++ )
	{
		node = p->nodes[i];
		for( b = node->brushlist ; b ; b = b->next )
		{
			if( !( b->contents & CONTENTS_AREAPORTAL ) )
			{
				continue;
			}
			orig = b->original;
			for( j = 0 ; j < orig->numsides ; j++ )
			{
				s = orig->sides + j;
				if( !s->visibleHull )
				{
					continue;
				}
				if( !( s->material->GetContentFlags() & CONTENTS_AREAPORTAL ) )
				{
					continue;
				}
				if( ( s->planenum & ~1 ) != ( p->onnode->planenum & ~1 ) )
				{
					continue;
				}
				// remove the visible hull from any other portal sides of this portal brush
				for( k = 0; k < orig->numsides; k++ )
				{
					if( k == j )
					{
						continue;
					}
					s2 = orig->sides + k;
					if( s2->visibleHull == NULL )
					{
						continue;
					}
					if( !( s2->material->GetContentFlags() & CONTENTS_AREAPORTAL ) )
					{
						continue;
					}
					common->Warning( "brush has multiple area portal sides at %s", s2->visibleHull->GetCenter().ToString() );
					delete s2->visibleHull;
					s2->visibleHull = NULL;
				}
				return s;
			}
		}
	}
	return NULL;
}

// RB: extra function to avoid many allocations
static bool	CheckTrianglesForPortal( uPortal_t* p )
{
	int			i;
	node_t*		node;
	mapTri_t*	tri;

	// scan both bordering nodes triangle lists for portal triangles that share the plane
	for( i = 0 ; i < 2 ; i++ )
	{
		node = p->nodes[i];
		for( tri = node->areaPortalTris; tri; tri = tri->next )
		{
			if( !( tri->material->GetContentFlags() & CONTENTS_AREAPORTAL ) )
			{
				continue;
			}

			if( ( tri->planeNum & ~1 ) != ( p->onnode->planenum & ~1 ) )
			{
				continue;
			}

			return true;
		}
	}

	return false;
}

static bool	FindTrianglesForPortal( uPortal_t* p, idList<mapTri_t*>& tris )
{
	int			i;
	node_t*		node;
	mapTri_t*	tri;

	tris.Clear();

	// scan both bordering nodes triangle lists for portal triangles that share the plane
	for( i = 0 ; i < 2 ; i++ )
	{
		node = p->nodes[i];
		for( tri = node->areaPortalTris; tri; tri = tri->next )
		{
			if( !( tri->material->GetContentFlags() & CONTENTS_AREAPORTAL ) )
			{
				continue;
			}

			if( ( tri->planeNum & ~1 ) != ( p->onnode->planenum & ~1 ) )
			{
				continue;
			}

			tris.Append( tri );
		}
	}

	return tris.Num() > 0;
}
// RB end

/*
=============
FloodAreas_r
=============
*/
void FloodAreas_r( node_t* node )
{
	uPortal_t*	p;
	int			s;

	if( node->area != -1 )
	{
		return;		// allready got it
	}
	if( node->opaque )
	{
		return;
	}

	c_areaFloods++;
	node->area = c_areas;

	for( p = node->portals ; p ; p = p->next[s] )
	{
		node_t*	other;

		s = ( p->nodes[1] == node );
		other = p->nodes[!s];

		if( !Portal_Passable( p ) )
		{
			continue;
		}

		// can't flood through an area portal
		if( FindSideForPortal( p ) )
		{
			continue;
		}

		// RB: check area portal triangles as well
		if( CheckTrianglesForPortal( p ) )
		{
			continue;
		}

		FloodAreas_r( other );
	}
}

/*
=============
FindAreas_r

Just decend the tree, and for each node that hasn't had an
area set, flood fill out from there
=============
*/
void FindAreas_r( node_t* node )
{
	if( node->planenum != PLANENUM_LEAF )
	{
		FindAreas_r( node->children[0] );
		FindAreas_r( node->children[1] );
		return;
	}

	if( node->opaque )
	{
		return;
	}

	if( node->area != -1 )
	{
		return;		// allready got it
	}

	c_areaFloods = 0;
	FloodAreas_r( node );
	common->Printf( "area %i has %i leafs\n", c_areas, c_areaFloods );
	c_areas++;
}

/*
============
CheckAreas_r
============
*/
void CheckAreas_r( node_t* node )
{
	if( node->planenum != PLANENUM_LEAF )
	{
		CheckAreas_r( node->children[0] );
		CheckAreas_r( node->children[1] );
		return;
	}
	if( !node->opaque && node->area < 0 )
	{
		common->Error( "CheckAreas_r: area = %i", node->area );
	}
}

/*
============
ClearAreas_r

Set all the areas to -1 before filling
============
*/
void ClearAreas_r( node_t* node )
{
	if( node->planenum != PLANENUM_LEAF )
	{
		ClearAreas_r( node->children[0] );
		ClearAreas_r( node->children[1] );
		return;
	}
	node->area = -1;
}

//=============================================================


/*
=================
FindInterAreaPortals_r
=================
*/
static void FindInterAreaPortals_r( node_t* node )
{
	uPortal_t*	p;
	int			s;
	int			i;
	idWinding*	w;
	interAreaPortal_t*	iap;
	side_t*		side;

	if( node->planenum != PLANENUM_LEAF )
	{
		FindInterAreaPortals_r( node->children[0] );
		FindInterAreaPortals_r( node->children[1] );
		return;
	}

	if( node->opaque )
	{
		return;
	}

	for( p = node->portals ; p ; p = p->next[s] )
	{
		node_t*	other;

		s = ( p->nodes[1] == node );
		other = p->nodes[!s];

		if( other->opaque )
		{
			continue;
		}

		// only report areas going from lower number to higher number
		// so we don't report the portal twice
		if( other->area <= node->area )
		{
			continue;
		}

		side = FindSideForPortal( p );
//		w = p->winding;
		if( !side )
		{
			common->Warning( "FindSideForPortal failed at %s", p->winding->GetCenter().ToString() );
			continue;
		}
		w = side->visibleHull;
		if( !w )
		{
			continue;
		}

		// see if we have created this portal before
		for( i = 0; i < interAreaPortals.Num(); i++ )
		{
			iap = &interAreaPortals[i];

			if( side == iap->side &&
					( ( p->nodes[0]->area == iap->area0 && p->nodes[1]->area == iap->area1 )
					  || ( p->nodes[1]->area == iap->area0 && p->nodes[0]->area == iap->area1 ) ) )
			{
				break;
			}
		}

		if( i != interAreaPortals.Num() )
		{
			continue;	// already emited
		}

		iap = &interAreaPortals.Alloc();

		if( side->planenum == p->onnode->planenum )
		{
			iap->area0 = p->nodes[0]->area;
			iap->area1 = p->nodes[1]->area;
		}
		else
		{
			iap->area0 = p->nodes[1]->area;
			iap->area1 = p->nodes[0]->area;
		}
		iap->side = side;
	}

	// RB: check area portal triangles
	idList<mapTri_t*> apTriangles;
	for( p = node->portals ; p ; p = p->next[s] )
	{
		node_t*	other;

		s = ( p->nodes[1] == node );
		other = p->nodes[!s];

		if( other->opaque )
		{
			continue;
		}

		// only report areas going from lower number to higher number
		// so we don't report the portal twice
		if( other->area <= node->area )
		{
			continue;
		}

		FindTrianglesForPortal( p, apTriangles );
		if( apTriangles.Num() < 2 )
		{
			//common->Warning( "FindTrianglesForPortal failed at %s", p->winding->GetCenter().ToString() );
			continue;
		}

		// see if we have created this portal before
		for( i = 0; i < interAreaPortals.Num(); i++ )
		{
			iap = &interAreaPortals[i];

			if( apTriangles[0]->polygonId == iap->polygonId &&
					( ( p->nodes[0]->area == iap->area0 && p->nodes[1]->area == iap->area1 )
					  || ( p->nodes[1]->area == iap->area0 && p->nodes[0]->area == iap->area1 ) ) )
			{
				break;
			}
		}

		if( i != interAreaPortals.Num() )
		{
			continue;	// already emited
		}

		iap = &interAreaPortals.Alloc();

		if( apTriangles[0]->planeNum == p->onnode->planenum )
		{
			iap->area0 = p->nodes[0]->area;
			iap->area1 = p->nodes[1]->area;
		}
		else
		{
			iap->area0 = p->nodes[1]->area;
			iap->area1 = p->nodes[0]->area;
		}

		iap->polygonId = apTriangles[0]->polygonId;

		// merge triangles to a new winding
		for( int j = 0; j < apTriangles.Num(); j++ )
		{
			mapTri_t* tri = apTriangles[j];
			idVec3 planeNormal = dmapGlobals.mapPlanes[ tri->planeNum].Normal();

			for( int k = 0; k < 3; k++ )
			{
				iap->w.AddToConvexHull( tri->v[k].xyz, planeNormal );
			}
		}


	}
	// RB end
}





/*
=============
FloodAreas

Mark each leaf with an area, bounded by CONTENTS_AREAPORTAL
Sets e->areas.numAreas
=============
*/
void FloodAreas( uEntity_t* e )
{
	common->Printf( "--- FloodAreas ---\n" );

	// set all areas to -1
	ClearAreas_r( e->tree->headnode );

	// flood fill from non-opaque areas
	c_areas = 0;
	FindAreas_r( e->tree->headnode );

	common->Printf( "%5i areas\n", c_areas );
	e->numAreas = c_areas;

	// make sure we got all of them
	CheckAreas_r( e->tree->headnode );

	// identify all portals between areas if this is the world
	if( e == &dmapGlobals.uEntities[0] )
	{
		interAreaPortals.Clear();
		FindInterAreaPortals_r( e->tree->headnode );
	}
}

/*
======================================================

FILL OUTSIDE

======================================================
*/

static	int		c_outside;
static	int		c_inside;
static	int		c_solid;

void FillOutside_r( node_t* node )
{
	if( node->planenum != PLANENUM_LEAF )
	{
		FillOutside_r( node->children[0] );
		FillOutside_r( node->children[1] );
		return;
	}

	// anything not reachable by an entity
	// can be filled away
	if( !node->occupied )
	{
		if( !node->opaque )
		{
			c_outside++;
			node->opaque = true;
		}
		else
		{
			c_solid++;
		}
	}
	else
	{
		c_inside++;
	}

}

/*
=============
FillOutside

Fill (set node->opaque = true) all nodes that can't be reached by entities
=============
*/
void FillOutside( uEntity_t* e )
{
	c_outside = 0;
	c_inside = 0;
	c_solid = 0;
	common->Printf( "--- FillOutside ---\n" );
	FillOutside_r( e->tree->headnode );
	common->Printf( "%5i solid leafs\n", c_solid );
	common->Printf( "%5i leafs filled\n", c_outside );
	common->Printf( "%5i inside leafs\n", c_inside );
}
