/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

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

//#pragma optimize( "", off )

#include "dmap.h"
#ifdef WIN32
	#include <windows.h>
	#include <GL/gl.h>
#endif

/*

  New vertexes will be created where edges cross.

  optimization requires an accurate t junction fixer.



*/

idBounds	optBounds;

#define	MAX_OPT_VERTEXES	0x10000
int			numOptVerts;
optVertex_t optVerts[MAX_OPT_VERTEXES];

#define	MAX_OPT_EDGES		0x40000
static	int		numOptEdges;
static	optEdge_t	optEdges[MAX_OPT_EDGES];

static bool IsTriangleValid( const optVertex_t* v1, const optVertex_t* v2, const optVertex_t* v3 );
static bool IsTriangleDegenerate( const optVertex_t* v1, const optVertex_t* v2, const optVertex_t* v3 );

static idRandom orandom;

/*
==============
ValidateEdgeCounts
==============
*/
static void ValidateEdgeCounts( optIsland_t* island )
{
	optVertex_t*	vert;
	optEdge_t*	e;
	int			c;

	for( vert = island->verts ; vert ; vert = vert->islandLink )
	{
		c = 0;
		for( e = vert->edges ; e ; )
		{
			c++;
			if( e->v1 == vert )
			{
				e = e->v1link;
			}
			else if( e->v2 == vert )
			{
				e = e->v2link;
			}
			else
			{
				common->Error( "ValidateEdgeCounts: mislinked" );
			}
		}
		if( c != 2 && c != 0 )
		{
			// this can still happen at diamond intersections
//			common->Printf( "ValidateEdgeCounts: %i edges\n", c );
		}
	}
}


/*
====================
AllocEdge
====================
*/
static optEdge_t*	AllocEdge()
{
	optEdge_t*	e;

	if( numOptEdges == MAX_OPT_EDGES )
	{
		common->Error( "MAX_OPT_EDGES" );
	}
	e = &optEdges[ numOptEdges ];
	numOptEdges++;
	memset( e, 0, sizeof( *e ) );

	return e;
}

/*
====================
RemoveEdgeFromVert
====================
*/
static	void RemoveEdgeFromVert( optEdge_t* e1, optVertex_t* vert )
{
	optEdge_t**	prev;
	optEdge_t*	e;

	if( !vert )
	{
		return;
	}
	prev = &vert->edges;
	while( *prev )
	{
		e = *prev;
		if( e == e1 )
		{
			if( e1->v1 == vert )
			{
				*prev = e1->v1link;
			}
			else if( e1->v2 == vert )
			{
				*prev = e1->v2link;
			}
			else
			{
				common->Error( "RemoveEdgeFromVert: vert not found" );
			}
			return;
		}

		if( e->v1 == vert )
		{
			prev = &e->v1link;
		}
		else if( e->v2 == vert )
		{
			prev = &e->v2link;
		}
		else
		{
			common->Error( "RemoveEdgeFromVert: vert not found" );
		}
	}
}

/*
====================
UnlinkEdge
====================
*/
static	void UnlinkEdge( optEdge_t* e, optIsland_t* island )
{
	optEdge_t**	prev;

	RemoveEdgeFromVert( e, e->v1 );
	RemoveEdgeFromVert( e, e->v2 );

	for( prev = &island->edges ; *prev ; prev = &( *prev )->islandLink )
	{
		if( *prev == e )
		{
			*prev = e->islandLink;
			return;
		}
	}

	common->Error( "RemoveEdgeFromIsland: couldn't free edge" );
}


/*
====================
LinkEdge
====================
*/
static	void LinkEdge( optEdge_t* e )
{
	e->v1link = e->v1->edges;
	e->v1->edges = e;

	e->v2link = e->v2->edges;
	e->v2->edges = e;
}

/*
================
FindOptVertex
================
*/
static optVertex_t* FindOptVertex( idDrawVert* v, optimizeGroup_t* opt )
{
	int		i;
	float	x, y;
	optVertex_t*	vert;

	// deal with everything strictly as 2D
	x = v->xyz * opt->axis[0];
	y = v->xyz * opt->axis[1];

	// should we match based on the t-junction fixing hash verts?
	for( i = 0 ; i < numOptVerts ; i++ )
	{
		if( optVerts[i].pv[0] == x && optVerts[i].pv[1] == y )
		{
			return &optVerts[i];
		}
	}

	if( numOptVerts >= MAX_OPT_VERTEXES )
	{
		common->Error( "MAX_OPT_VERTEXES" );
		return NULL;
	}

	numOptVerts++;

	vert = &optVerts[i];
	memset( vert, 0, sizeof( *vert ) );
	vert->v = *v;
	vert->pv[0] = x;
	vert->pv[1] = y;
	vert->pv[2] = 0;

	optBounds.AddPoint( vert->pv );

	return vert;
}

/*
================
DrawAllEdges
================
*/
static	void DrawAllEdges()
{
//	int		i;

	if( !dmapGlobals.drawflag )
	{
		return;
	}

#if 0
	Draw_ClearWindow();

	qglBegin( GL_LINES );
	for( i = 0 ; i < numOptEdges ; i++ )
	{
		if( optEdges[i].v1 == NULL )
		{
			continue;
		}
		qglColor3f( 1, 0, 0 );
		qglVertex3fv( optEdges[i].v1->pv.ToFloatPtr() );
		qglColor3f( 0, 0, 0 );
		qglVertex3fv( optEdges[i].v2->pv.ToFloatPtr() );
	}
	qglEnd();
	qglFlush();
#endif
}

/*
================
DrawVerts
================
*/
static void DrawVerts( optIsland_t* island )
{
//	optVertex_t	*vert;

	if( !dmapGlobals.drawflag )
	{
		return;
	}

#if 0
	qglEnable( GL_BLEND );
	qglBlendFunc( GL_ONE, GL_ONE );
	qglColor3f( 0.3f, 0.3f, 0.3f );
	qglPointSize( 3 );
	qglBegin( GL_POINTS );
	for( vert = island->verts ; vert ; vert = vert->islandLink )
	{
		qglVertex3fv( vert->pv.ToFloatPtr() );
	}
	qglEnd();
	qglDisable( GL_BLEND );
	qglFlush();
#endif
}

/*
================
DrawEdges
================
*/
static	void DrawEdges( optIsland_t* island )
{
//	optEdge_t	*edge;

	if( !dmapGlobals.drawflag )
	{
		return;
	}

#if 0
	Draw_ClearWindow();

	qglBegin( GL_LINES );
	for( edge = island->edges ; edge ; edge = edge->islandLink )
	{
		if( edge->v1 == NULL )
		{
			continue;
		}
		qglColor3f( 1, 0, 0 );
		qglVertex3fv( edge->v1->pv.ToFloatPtr() );
		qglColor3f( 0, 0, 0 );
		qglVertex3fv( edge->v2->pv.ToFloatPtr() );
	}
	qglEnd();
	qglFlush();
#endif
}

//=================================================================

/*
=================
VertexBetween
=================
*/
static bool VertexBetween( const optVertex_t* p1, const optVertex_t* v1, const optVertex_t* v2 )
{
	idVec3	d1, d2;
	float	d;

	d1 = p1->pv - v1->pv;
	d2 = p1->pv - v2->pv;
	d = d1 * d2;
	if( d < 0 )
	{
		return true;
	}
	return false;
}


/*
====================
EdgeIntersection

Creates a new optVertex_t where the line segments cross.
This should only be called if PointsStraddleLine returned true

Will return NULL if the lines are colinear
====================
*/
static	optVertex_t* EdgeIntersection( const optVertex_t* p1, const optVertex_t* p2,
									   const optVertex_t* l1, const optVertex_t* l2, optimizeGroup_t* opt )
{
	float	f;
	idDrawVert*	v;
	idVec3	dir1, dir2, cross1, cross2;

	dir1 = p1->pv - l1->pv;
	dir2 = p1->pv - l2->pv;
	cross1 = dir1.Cross( dir2 );

	dir1 = p2->pv - l1->pv;
	dir2 = p2->pv - l2->pv;
	cross2 = dir1.Cross( dir2 );

	if( cross1[2] - cross2[2] == 0 )
	{
		return NULL;
	}

	f = cross1[2] / ( cross1[2] - cross2[2] );

	// FIXME: how are we freeing this, since it doesn't belong to a tri?
	v = ( idDrawVert* )Mem_Alloc( sizeof( *v ), TAG_TOOLS );
	memset( v, 0, sizeof( *v ) );

	v->xyz = p1->v.xyz * ( 1.0 - f ) + p2->v.xyz * f;
	idVec3 normal = p1->v.GetNormal() * ( 1.0 - f ) + p2->v.GetNormal() * f;
	normal.Normalize();
	v->SetNormal( normal );

	idVec2 st;
	st.x = p1->v.GetTexCoordS() * ( 1.0 - f ) + p2->v.GetTexCoordS() * f;
	st.y = p1->v.GetTexCoordT() * ( 1.0 - f ) + p2->v.GetTexCoordT() * f;
	v->SetTexCoord( st );

	return FindOptVertex( v, opt );
}


/*
====================
PointsStraddleLine

Colinear is considdered crossing.
====================
*/
static	bool PointsStraddleLine( optVertex_t* p1, optVertex_t* p2, optVertex_t* l1, optVertex_t* l2 )
{
	bool	t1, t2;

	t1 = IsTriangleDegenerate( l1, l2, p1 );
	t2 = IsTriangleDegenerate( l1, l2, p2 );
	if( t1 && t2 )
	{
		// colinear case
		float	s1, s2, s3, s4;
		bool	positive, negative;

		s1 = ( p1->pv - l1->pv ) * ( l2->pv - l1->pv );
		s2 = ( p2->pv - l1->pv ) * ( l2->pv - l1->pv );
		s3 = ( p1->pv - l2->pv ) * ( l2->pv - l1->pv );
		s4 = ( p2->pv - l2->pv ) * ( l2->pv - l1->pv );

		if( s1 > 0 || s2 > 0 || s3 > 0 || s4 > 0 )
		{
			positive = true;
		}
		else
		{
			positive = false;
		}
		if( s1 < 0 || s2 < 0 || s3 < 0 || s4 < 0 )
		{
			negative = true;
		}
		else
		{
			negative = false;
		}

		if( positive && negative )
		{
			return true;
		}
		return false;
	}
	else if( p1 != l1 && p1 != l2 && p2 != l1 && p2 != l2 )
	{
		// no shared verts
		t1 = IsTriangleValid( l1, l2, p1 );
		t2 = IsTriangleValid( l1, l2, p2 );
		if( t1 && t2 )
		{
			return false;
		}

		t1 = IsTriangleValid( l1, p1, l2 );
		t2 = IsTriangleValid( l1, p2, l2 );
		if( t1 && t2 )
		{
			return false;
		}

		return true;
	}
	else
	{
		// a shared vert, not colinear, so not crossing
		return false;
	}
}


/*
====================
EdgesCross
====================
*/
static	bool EdgesCross( optVertex_t* a1, optVertex_t* a2, optVertex_t* b1, optVertex_t* b2 )
{
	// if both verts match, consider it to be crossed
	if( a1 == b1 && a2 == b2 )
	{
		return true;
	}
	if( a1 == b2 && a2 == b1 )
	{
		return true;
	}
	// if only one vert matches, it might still be colinear, which
	// would be considered crossing

	// if both lines' verts are on opposite sides of the other
	// line, it is crossed
	if( !PointsStraddleLine( a1, a2, b1, b2 ) )
	{
		return false;
	}
	if( !PointsStraddleLine( b1, b2, a1, a2 ) )
	{
		return false;
	}

	return true;
}

/*
====================
TryAddNewEdge

====================
*/
static	bool TryAddNewEdge( optVertex_t* v1, optVertex_t* v2, optIsland_t* island )
{
	optEdge_t*	e;

	// if the new edge crosses any other edges, don't add it
	for( e = island->edges ; e ; e = e->islandLink )
	{
		if( EdgesCross( e->v1, e->v2, v1, v2 ) )
		{
			return false;
		}
	}

	if( dmapGlobals.drawflag )
	{
#if 0
		qglBegin( GL_LINES );
		qglColor3f( 0, ( 128 + orandom.RandomInt( 127 ) ) / 255.0, 0 );
		qglVertex3fv( v1->pv.ToFloatPtr() );
		qglVertex3fv( v2->pv.ToFloatPtr() );
		qglEnd();
		qglFlush();
#endif
	}
	// add it
	e = AllocEdge();

	e->islandLink = island->edges;
	island->edges = e;
	e->v1 = v1;
	e->v2 = v2;

	e->created = true;

	// link the edge to its verts
	LinkEdge( e );

	return true;
}

typedef struct
{
	optVertex_t*	v1, *v2;
	float		length;
} edgeLength_t;


static	int LengthSort( const void* a, const void* b )
{
	const edgeLength_t*	ea, *eb;

	ea = ( const edgeLength_t* )a;
	eb = ( const edgeLength_t* )b;
	if( ea->length < eb->length )
	{
		return -1;
	}
	if( ea->length > eb->length )
	{
		return 1;
	}
	return 0;
}

/*
==================
AddInteriorEdges

Add all possible edges between the verts
==================
*/
static	void AddInteriorEdges( optIsland_t* island )
{
	int		c_addedEdges;
	optVertex_t*	vert, *vert2;
	int		c_verts;
	edgeLength_t*	lengths;
	int				numLengths;
	int				i;

	DrawVerts( island );

	// count the verts
	c_verts = 0;
	for( vert = island->verts ; vert ; vert = vert->islandLink )
	{
		if( !vert->edges )
		{
			continue;
		}
		c_verts++;
	}

	// allocate space for all the lengths
	lengths = ( edgeLength_t* )Mem_Alloc( sizeof( *lengths ) * c_verts * c_verts / 2, TAG_TOOLS );
	numLengths = 0;
	for( vert = island->verts ; vert ; vert = vert->islandLink )
	{
		if( !vert->edges )
		{
			continue;
		}
		for( vert2 = vert->islandLink ; vert2 ; vert2 = vert2->islandLink )
		{
			idVec3		dir;

			if( !vert2->edges )
			{
				continue;
			}
			lengths[numLengths].v1 = vert;
			lengths[numLengths].v2 = vert2;
			dir = ( vert->pv - vert2->pv ) ;
			lengths[numLengths].length = dir.Length();
			numLengths++;
		}
	}


	// sort by length, shortest first
	qsort( lengths, numLengths, sizeof( lengths[0] ), LengthSort );

	// try to create them in that order
	c_addedEdges = 0;
	for( i = 0 ; i < numLengths ; i++ )
	{
		if( TryAddNewEdge( lengths[i].v1, lengths[i].v2, island ) )
		{
			c_addedEdges++;
		}
	}

	if( dmapGlobals.verbose )
	{
		common->Printf( "%6i tested segments\n", numLengths );
		common->Printf( "%6i added interior edges\n", c_addedEdges );
	}

	Mem_Free( lengths );
}



//==================================================================

/*
====================
RemoveIfColinear

====================
*/
#define	COLINEAR_EPSILON	0.1
static	void RemoveIfColinear( optVertex_t* ov, optIsland_t* island )
{
	optEdge_t*	e, *e1, *e2;
	optVertex_t* v1 = NULL, *v2 = NULL, *v3 = NULL;
	idVec3		dir1, dir2;
	float		len, dist;
	idVec3		point;
	idVec3		offset;
	float		off;

	v2 = ov;

	// we must find exactly two edges before testing for colinear
	e1 = NULL;
	e2 = NULL;
	for( e = ov->edges ; e ; )
	{
		if( !e1 )
		{
			e1 = e;
		}
		else if( !e2 )
		{
			e2 = e;
		}
		else
		{
			return;		// can't remove a vertex with three edges
		}
		if( e->v1 == v2 )
		{
			e = e->v1link;
		}
		else if( e->v2 == v2 )
		{
			e = e->v2link;
		}
		else
		{
			common->Error( "RemoveIfColinear: mislinked edge" );
		}
	}

	// can't remove if no edges
	if( !e1 )
	{
		return;
	}

	if( !e2 )
	{
		// this may still happen legally when a tiny triangle is
		// the only thing in a group
		common->Printf( "WARNING: vertex with only one edge\n" );
		return;
	}

	if( e1->v1 == v2 )
	{
		v1 = e1->v2;
	}
	else if( e1->v2 == v2 )
	{
		v1 = e1->v1;
	}
	else
	{
		common->Error( "RemoveIfColinear: mislinked edge" );
	}
	if( e2->v1 == v2 )
	{
		v3 = e2->v2;
	}
	else if( e2->v2 == v2 )
	{
		v3 = e2->v1;
	}
	else
	{
		common->Error( "RemoveIfColinear: mislinked edge" );
	}

	if( v1 == v3 )
	{
		common->Error( "RemoveIfColinear: mislinked edge" );
	}

	// they must point in opposite directions
	dist = ( v3->pv - v2->pv ) * ( v1->pv - v2->pv );
	if( dist >= 0 )
	{
		return;
	}

	// see if they are colinear
	dir1 = v3->v.xyz - v1->v.xyz;
	len = dir1.Normalize();
	dir2 = v2->v.xyz - v1->v.xyz;
	dist = dir2 * dir1;
	VectorMA( v1->v.xyz, dist, dir1, point );
	offset = point - v2->v.xyz;
	off = offset.Length();

	if( off > COLINEAR_EPSILON )
	{
		return;
	}

	if( dmapGlobals.drawflag )
	{
#if 0
		qglBegin( GL_LINES );
		qglColor3f( 1, 1, 0 );
		qglVertex3fv( v1->pv.ToFloatPtr() );
		qglVertex3fv( v2->pv.ToFloatPtr() );
		qglEnd();
		qglFlush();
		qglBegin( GL_LINES );
		qglColor3f( 0, 1, 1 );
		qglVertex3fv( v2->pv.ToFloatPtr() );
		qglVertex3fv( v3->pv.ToFloatPtr() );
		qglEnd();
		qglFlush();
#endif
	}

	// replace the two edges with a single edge
	UnlinkEdge( e1, island );
	UnlinkEdge( e2, island );

	// v2 should have no edges now
	if( v2->edges )
	{
		common->Error( "RemoveIfColinear: didn't remove properly" );
	}


	// if there is an existing edge that already
	// has these exact verts, we have just collapsed a
	// sliver triangle out of existance, and all the edges
	// can be removed
	for( e = island->edges ; e ; e = e->islandLink )
	{
		if( ( e->v1 == v1 && e->v2 == v3 )
				|| ( e->v1 == v3 && e->v2 == v1 ) )
		{
			UnlinkEdge( e, island );
			RemoveIfColinear( v1, island );
			RemoveIfColinear( v3, island );
			return;
		}
	}

	// if we can't add the combined edge, link
	// the originals back in
	if( !TryAddNewEdge( v1, v3, island ) )
	{
		e1->islandLink = island->edges;
		island->edges = e1;
		LinkEdge( e1 );

		e2->islandLink = island->edges;
		island->edges = e2;
		LinkEdge( e2 );
		return;
	}

	// recursively try to combine both verts now,
	// because things may have changed since the last combine test
	RemoveIfColinear( v1, island );
	RemoveIfColinear( v3, island );
}

/*
====================
CombineColinearEdges
====================
*/
static	void CombineColinearEdges( optIsland_t* island )
{
	int			c_edges;
	optVertex_t*	ov;
	optEdge_t*	e;

	c_edges = 0;
	for( e = island->edges ; e ; e = e->islandLink )
	{
		c_edges++;
	}
	if( dmapGlobals.verbose )
	{
		common->Printf( "%6i original exterior edges\n", c_edges );
	}

	for( ov = island->verts ; ov ; ov = ov->islandLink )
	{
		RemoveIfColinear( ov, island );
	}

	c_edges = 0;
	for( e = island->edges ; e ; e = e->islandLink )
	{
		c_edges++;
	}
	if( dmapGlobals.verbose )
	{
		common->Printf( "%6i optimized exterior edges\n", c_edges );
	}
}


//==================================================================

/*
===================
FreeOptTriangles

===================
*/
static void FreeOptTriangles( optIsland_t* island )
{
	optTri_t*	opt, *next;

	for( opt = island->tris ; opt ; opt = next )
	{
		next = opt->next;
		Mem_Free( opt );
	}

	island->tris = NULL;
}


/*
=================
IsTriangleValid

empty area will be considered invalid.
Due to some truly aweful epsilon issues, a triangle can switch between
valid and invalid depending on which order you look at the verts, so
consider it invalid if any one of the possibilities is invalid.
=================
*/
static bool IsTriangleValid( const optVertex_t* v1, const optVertex_t* v2, const optVertex_t* v3 )
{
	idVec3	d1, d2, normal;

	d1 = v2->pv - v1->pv;
	d2 = v3->pv - v1->pv;
	normal = d1.Cross( d2 );
	if( normal[2] <= 0 )
	{
		return false;
	}

	d1 = v3->pv - v2->pv;
	d2 = v1->pv - v2->pv;
	normal = d1.Cross( d2 );
	if( normal[2] <= 0 )
	{
		return false;
	}

	d1 = v1->pv - v3->pv;
	d2 = v2->pv - v3->pv;
	normal = d1.Cross( d2 );
	if( normal[2] <= 0 )
	{
		return false;
	}

	return true;
}


/*
=================
IsTriangleDegenerate

Returns false if it is either front or back facing
=================
*/
static bool IsTriangleDegenerate( const optVertex_t* v1, const optVertex_t* v2, const optVertex_t* v3 )
{
#if 1
	idVec3	d1, d2, normal;

	d1 = v2->pv - v1->pv;
	d2 = v3->pv - v1->pv;
	normal = d1.Cross( d2 );
	if( normal[2] == 0 )
	{
		return true;
	}
	return false;
#else
	return ( bool )!IsTriangleValid( v1, v2, v3 );
#endif
}


/*
==================
PointInTri

Tests if a 2D point is inside an original triangle
==================
*/
static bool PointInTri( const idVec3& p, const mapTri_t* tri, optIsland_t* island )
{
	idVec3	d1, d2, normal;

	// the normal[2] == 0 case is not uncommon when a square is triangulated in
	// the opposite manner to the original

	d1 = tri->optVert[0]->pv - p;
	d2 = tri->optVert[1]->pv - p;
	normal = d1.Cross( d2 );
	if( normal[2] < 0 )
	{
		return false;
	}

	d1 = tri->optVert[1]->pv - p;
	d2 = tri->optVert[2]->pv - p;
	normal = d1.Cross( d2 );
	if( normal[2] < 0 )
	{
		return false;
	}

	d1 = tri->optVert[2]->pv - p;
	d2 = tri->optVert[0]->pv - p;
	normal = d1.Cross( d2 );
	if( normal[2] < 0 )
	{
		return false;
	}

	return true;
}


/*
====================
LinkTriToEdge

====================
*/
static void LinkTriToEdge( optTri_t* optTri, optEdge_t* edge )
{
	if( ( edge->v1 == optTri->v[0] && edge->v2 == optTri->v[1] )
			|| ( edge->v1 == optTri->v[1] && edge->v2 == optTri->v[2] )
			|| ( edge->v1 == optTri->v[2] && edge->v2 == optTri->v[0] ) )
	{
		if( edge->backTri )
		{
			common->Printf( "Warning: LinkTriToEdge: already in use\n" );
			return;
		}
		edge->backTri = optTri;
		return;
	}
	if( ( edge->v1 == optTri->v[1] && edge->v2 == optTri->v[0] )
			|| ( edge->v1 == optTri->v[2] && edge->v2 == optTri->v[1] )
			|| ( edge->v1 == optTri->v[0] && edge->v2 == optTri->v[2] ) )
	{
		if( edge->frontTri )
		{
			common->Printf( "Warning: LinkTriToEdge: already in use\n" );
			return;
		}
		edge->frontTri = optTri;
		return;
	}
	common->Error( "LinkTriToEdge: edge not found on tri" );
}

/*
===============
CreateOptTri
===============
*/
static void CreateOptTri( optVertex_t* first, optEdge_t* e1, optEdge_t* e2, optIsland_t* island )
{
	optEdge_t*		opposite = NULL;
	optVertex_t*		second = NULL, *third = NULL;
	optTri_t*		optTri = NULL;
	mapTri_t*		tri = NULL;

	if( e1->v1 == first )
	{
		second = e1->v2;
	}
	else if( e1->v2 == first )
	{
		second = e1->v1;
	}
	else
	{
		common->Error( "CreateOptTri: mislinked edge" );
	}

	if( e2->v1 == first )
	{
		third = e2->v2;
	}
	else if( e2->v2 == first )
	{
		third = e2->v1;
	}
	else
	{
		common->Error( "CreateOptTri: mislinked edge" );
	}

	if( !IsTriangleValid( first, second, third ) )
	{
		common->Error( "CreateOptTri: invalid" );
	}

//DrawEdges( island );

	// identify the third edge
	if( dmapGlobals.drawflag )
	{
#if 0
		qglColor3f( 1, 1, 0 );
		qglBegin( GL_LINES );
		qglVertex3fv( e1->v1->pv.ToFloatPtr() );
		qglVertex3fv( e1->v2->pv.ToFloatPtr() );
		qglEnd();
		qglFlush();
		qglColor3f( 0, 1, 1 );
		qglBegin( GL_LINES );
		qglVertex3fv( e2->v1->pv.ToFloatPtr() );
		qglVertex3fv( e2->v2->pv.ToFloatPtr() );
		qglEnd();
		qglFlush();
#endif
	}

	for( opposite = second->edges ; opposite ; )
	{
		if( opposite != e1 && ( opposite->v1 == third || opposite->v2 == third ) )
		{
			break;
		}
		if( opposite->v1 == second )
		{
			opposite = opposite->v1link;
		}
		else if( opposite->v2 == second )
		{
			opposite = opposite->v2link;
		}
		else
		{
			common->Error( "BuildOptTriangles: mislinked edge" );
		}
	}

	if( !opposite )
	{
		common->Printf( "Warning: BuildOptTriangles: couldn't locate opposite\n" );
		return;
	}

	if( dmapGlobals.drawflag )
	{
#if 0
		qglColor3f( 1, 0, 1 );
		qglBegin( GL_LINES );
		qglVertex3fv( opposite->v1->pv.ToFloatPtr() );
		qglVertex3fv( opposite->v2->pv.ToFloatPtr() );
		qglEnd();
		qglFlush();
#endif
	}

	// create new triangle
	optTri = ( optTri_t* )Mem_Alloc( sizeof( *optTri ), TAG_TOOLS );
	optTri->v[0] = first;
	optTri->v[1] = second;
	optTri->v[2] = third;
	optTri->midpoint = ( optTri->v[0]->pv + optTri->v[1]->pv + optTri->v[2]->pv ) * ( 1.0f / 3.0f );
	optTri->next = island->tris;
	island->tris = optTri;

	if( dmapGlobals.drawflag )
	{
#if 0
		qglColor3f( 1, 1, 1 );
		qglPointSize( 4 );
		qglBegin( GL_POINTS );
		qglVertex3fv( optTri->midpoint.ToFloatPtr() );
		qglEnd();
		qglFlush();
#endif
	}

	// find the midpoint, and scan through all the original triangles to
	// see if it is inside any of them
	for( tri = island->group->triList ; tri ; tri = tri->next )
	{
		if( PointInTri( optTri->midpoint, tri, island ) )
		{
			break;
		}
	}
	if( tri )
	{
		optTri->filled = true;
	}
	else
	{
		optTri->filled = false;
	}
	if( dmapGlobals.drawflag )
	{
#if 0
		if( optTri->filled )
		{
			qglColor3f( ( 128 + orandom.RandomInt( 127 ) ) / 255.0, 0, 0 );
		}
		else
		{
			qglColor3f( 0, ( 128 + orandom.RandomInt( 127 ) ) / 255.0, 0 );
		}
		qglBegin( GL_TRIANGLES );
		qglVertex3fv( optTri->v[0]->pv.ToFloatPtr() );
		qglVertex3fv( optTri->v[1]->pv.ToFloatPtr() );
		qglVertex3fv( optTri->v[2]->pv.ToFloatPtr() );
		qglEnd();
		qglColor3f( 1, 1, 1 );
		qglBegin( GL_LINE_LOOP );
		qglVertex3fv( optTri->v[0]->pv.ToFloatPtr() );
		qglVertex3fv( optTri->v[1]->pv.ToFloatPtr() );
		qglVertex3fv( optTri->v[2]->pv.ToFloatPtr() );
		qglEnd();
		qglFlush();
#endif
	}

	// link the triangle to it's edges
	LinkTriToEdge( optTri, e1 );
	LinkTriToEdge( optTri, e2 );
	LinkTriToEdge( optTri, opposite );
}

/*
====================
BuildOptTriangles

Generate a new list of triangles from the optEdeges
====================
*/
static void BuildOptTriangles( optIsland_t* island )
{
	optVertex_t*		ov = NULL, *second = NULL, *third = NULL, *middle = NULL;
	optEdge_t*		e1 = NULL, *e1Next = NULL, *e2 = NULL, *e2Next = NULL, *check = NULL, *checkNext = NULL;

	// free them
	FreeOptTriangles( island );

	// clear the vertex emitted flags
	for( ov = island->verts ; ov ; ov = ov->islandLink )
	{
		ov->emited = false;
	}

	// clear the edge triangle links
	for( check = island->edges ; check ; check = check->islandLink )
	{
		check->frontTri = check->backTri = NULL;
	}

	// check all possible triangle made up out of the
	// edges coming off the vertex
	for( ov = island->verts ; ov ; ov = ov->islandLink )
	{
		if( !ov->edges )
		{
			continue;
		}

#if 0
		if( dmapGlobals.drawflag && ov == ( optVertex_t* )0x1845a60 )
		{
			for( e1 = ov->edges ; e1 ; e1 = e1Next )
			{
				qglBegin( GL_LINES );
				qglColor3f( 0, 1, 0 );
				qglVertex3fv( e1->v1->pv.ToFloatPtr() );
				qglVertex3fv( e1->v2->pv.ToFloatPtr() );
				qglEnd();
				qglFlush();
				if( e1->v1 == ov )
				{
					e1Next = e1->v1link;
				}
				else if( e1->v2 == ov )
				{
					e1Next = e1->v2link;
				}
			}
		}
#endif
		for( e1 = ov->edges ; e1 ; e1 = e1Next )
		{
			if( e1->v1 == ov )
			{
				second = e1->v2;
				e1Next = e1->v1link;
			}
			else if( e1->v2 == ov )
			{
				second = e1->v1;
				e1Next = e1->v2link;
			}
			else
			{
				common->Error( "BuildOptTriangles: mislinked edge" );
			}

			// if the vertex has already been used, it can't be used again
			if( second->emited )
			{
				continue;
			}

			for( e2 = ov->edges ; e2 ; e2 = e2Next )
			{
				if( e2->v1 == ov )
				{
					third = e2->v2;
					e2Next = e2->v1link;
				}
				else if( e2->v2 == ov )
				{
					third = e2->v1;
					e2Next = e2->v2link;
				}
				else
				{
					common->Error( "BuildOptTriangles: mislinked edge" );
				}
				if( e2 == e1 )
				{
					continue;
				}

				// if the vertex has already been used, it can't be used again
				if( third->emited )
				{
					continue;
				}

				// if the triangle is backwards or degenerate, don't use it
				if( !IsTriangleValid( ov, second, third ) )
				{
					continue;
				}

				// see if any other edge bisects these two, which means
				// this triangle shouldn't be used
				for( check = ov->edges ; check ; check = checkNext )
				{
					if( check->v1 == ov )
					{
						middle = check->v2;
						checkNext = check->v1link;
					}
					else if( check->v2 == ov )
					{
						middle = check->v1;
						checkNext = check->v2link;
					}
					else
					{
						common->Error( "BuildOptTriangles: mislinked edge" );
					}

					if( check == e1 || check == e2 )
					{
						continue;
					}

					if( IsTriangleValid( ov, second, middle )
							&& IsTriangleValid( ov, middle, third ) )
					{
						break;	// should use the subdivided ones
					}
				}

				if( check )
				{
					continue;	// don't use it
				}

				// the triangle is valid
				CreateOptTri( ov, e1, e2, island );
			}
		}

		// later vertexes will not emit triangles that use an
		// edge that this vert has already used
		ov->emited = true;
	}
}



/*
====================
RegenerateTriangles

Add new triangles to the group's regeneratedTris
====================
*/
static	void	RegenerateTriangles( optIsland_t* island )
{
	optTri_t*		optTri;
	mapTri_t*		tri;
	int				c_out;

	c_out = 0;

	for( optTri = island->tris ; optTri ; optTri = optTri->next )
	{
		if( !optTri->filled )
		{
			continue;
		}

		// create a new mapTri_t
		tri = AllocTri();

		tri->material = island->group->material;
		tri->mergeGroup = island->group->mergeGroup;

		tri->v[0] = optTri->v[0]->v;
		tri->v[1] = optTri->v[1]->v;
		tri->v[2] = optTri->v[2]->v;

		idPlane plane;
		PlaneForTri( tri, plane );
		if( plane.Normal() * dmapGlobals.mapPlanes[ island->group->planeNum ].Normal() <= 0 )
		{
			// this can happen reasonably when a triangle is nearly degenerate in
			// optimization planar space, and winds up being degenerate in 3D space
			common->Printf( "WARNING: backwards triangle generated!\n" );
			// discard it
			FreeTri( tri );
			continue;
		}

		c_out++;
		tri->next = island->group->regeneratedTris;
		island->group->regeneratedTris = tri;
	}

	FreeOptTriangles( island );

	if( dmapGlobals.verbose )
	{
		common->Printf( "%6i tris out\n", c_out );
	}
}

//===========================================================================

/*
====================
RemoveInteriorEdges

Edges that have triangles of the same type (filled / empty)
on both sides will be removed
====================
*/
static	void RemoveInteriorEdges( optIsland_t* island )
{
	int		c_interiorEdges;
	int		c_exteriorEdges;
	optEdge_t*	e, *next;
	bool	front, back;

	c_exteriorEdges = 0;
	c_interiorEdges = 0;
	for( e = island->edges ; e ; e = next )
	{
		// we might remove the edge, so get the next link now
		next = e->islandLink;

		if( !e->frontTri )
		{
			front = false;
		}
		else
		{
			front = e->frontTri->filled;
		}
		if( !e->backTri )
		{
			back = false;
		}
		else
		{
			back = e->backTri->filled;
		}

		if( front == back )
		{
			// free the edge
			UnlinkEdge( e, island );
			c_interiorEdges++;
			continue;
		}

		c_exteriorEdges++;
	}

	if( dmapGlobals.verbose )
	{
		common->Printf( "%6i original interior edges\n", c_interiorEdges );
		common->Printf( "%6i original exterior edges\n", c_exteriorEdges );
	}
}

//==================================================================================

typedef struct
{
	optVertex_t*	v1, *v2;
} originalEdges_t;

/*
=================
AddEdgeIfNotAlready
=================
*/
void AddEdgeIfNotAlready( optVertex_t* v1, optVertex_t* v2 )
{
	optEdge_t*	e;

	// make sure that there isn't an identical edge already added
	for( e = v1->edges ; e ; )
	{
		if( ( e->v1 == v1 && e->v2 == v2 ) || ( e->v1 == v2 && e->v2 == v1 ) )
		{
			return;		// already added
		}
		if( e->v1 == v1 )
		{
			e = e->v1link;
		}
		else if( e->v2 == v1 )
		{
			e = e->v2link;
		}
		else
		{
			common->Error( "SplitEdgeByList: bad edge link" );
		}
	}

	// this edge is a keeper
	e = AllocEdge();
	e->v1 = v1;
	e->v2 = v2;

	e->islandLink = NULL;

	// link the edge to its verts
	LinkEdge( e );
}



/*
=================
DrawOriginalEdges
=================
*/
static void DrawOriginalEdges( int numOriginalEdges, originalEdges_t* originalEdges )
{
//	int		i;

	if( !dmapGlobals.drawflag )
	{
		return;
	}

#if 0
	Draw_ClearWindow();

	qglBegin( GL_LINES );
	for( i = 0 ; i < numOriginalEdges ; i++ )
	{
		qglColor3f( 1, 0, 0 );
		qglVertex3fv( originalEdges[i].v1->pv.ToFloatPtr() );
		qglColor3f( 0, 0, 0 );
		qglVertex3fv( originalEdges[i].v2->pv.ToFloatPtr() );
	}
	qglEnd();
	qglFlush();
#endif
}


typedef struct edgeCrossing_s
{
	struct edgeCrossing_s*	next;
	optVertex_t*		ov;
} edgeCrossing_t;

static	originalEdges_t*	originalEdges;
static	int				numOriginalEdges;

/*
=================
AddOriginalTriangle
=================
*/
static void AddOriginalTriangle( optVertex_t* v[3] )
{
	optVertex_t*		v1, *v2;

	// if this triangle is backwards (possible with epsilon issues)
	// ignore it completely
	if( !IsTriangleValid( v[0], v[1], v[2] ) )
	{
		common->Printf( "WARNING: backwards triangle in input!\n" );
		return;
	}

	for( int i = 0 ; i < 3 ; i++ )
	{
		v1 = v[i];
		v2 = v[( i + 1 ) % 3];

		if( v1 == v2 )
		{
			// this probably shouldn't happen, because the
			// tri would be degenerate
			continue;
		}
		int j;
		// see if there is an existing one
		for( j = 0 ; j < numOriginalEdges ; j++ )
		{
			if( originalEdges[j].v1 == v1 && originalEdges[j].v2 == v2 )
			{
				break;
			}
			if( originalEdges[j].v2 == v1 && originalEdges[j].v1 == v2 )
			{
				break;
			}
		}

		if( j == numOriginalEdges )
		{
			// add it
			originalEdges[j].v1 = v1;
			originalEdges[j].v2 = v2;
			numOriginalEdges++;
		}
	}
}

/*
=================
AddOriginalEdges
=================
*/
static	void AddOriginalEdges( optimizeGroup_t* opt )
{
	mapTri_t*		tri;
	optVertex_t*		v[3];
	int				numTris;

	if( dmapGlobals.verbose )
	{
		common->Printf( "----\n" );
		common->Printf( "%6i original tris\n", CountTriList( opt->triList ) );
	}

	optBounds.Clear();

	// allocate space for max possible edges
	numTris = CountTriList( opt->triList );
	originalEdges = ( originalEdges_t* )Mem_Alloc( numTris * 3 * sizeof( *originalEdges ), TAG_TOOLS );
	numOriginalEdges = 0;

	// add all unique triangle edges
	numOptVerts = 0;
	numOptEdges = 0;
	for( tri = opt->triList ; tri ; tri = tri->next )
	{
		v[0] = tri->optVert[0] = FindOptVertex( &tri->v[0], opt );
		v[1] = tri->optVert[1] = FindOptVertex( &tri->v[1], opt );
		v[2] = tri->optVert[2] = FindOptVertex( &tri->v[2], opt );

		AddOriginalTriangle( v );
	}
}

/*
=====================
SplitOriginalEdgesAtCrossings
=====================
*/
void SplitOriginalEdgesAtCrossings( optimizeGroup_t* opt )
{
	int				i, j, k, l;
	int				numOriginalVerts;
	edgeCrossing_t**	crossings;

	numOriginalVerts = numOptVerts;
	// now split any crossing edges and create optEdges
	// linked to the vertexes

	// debug drawing bounds
	dmapGlobals.drawBounds = optBounds;

	dmapGlobals.drawBounds[0][0] -= 2;
	dmapGlobals.drawBounds[0][1] -= 2;
	dmapGlobals.drawBounds[1][0] += 2;
	dmapGlobals.drawBounds[1][1] += 2;

	// generate crossing points between all the original edges
	crossings = ( edgeCrossing_t** )Mem_ClearedAlloc( numOriginalEdges * sizeof( *crossings ), TAG_TOOLS );

	for( i = 0 ; i < numOriginalEdges ; i++ )
	{
		if( dmapGlobals.drawflag )
		{
#if 0
			DrawOriginalEdges( numOriginalEdges, originalEdges );
			qglBegin( GL_LINES );
			qglColor3f( 0, 1, 0 );
			qglVertex3fv( originalEdges[i].v1->pv.ToFloatPtr() );
			qglColor3f( 0, 0, 1 );
			qglVertex3fv( originalEdges[i].v2->pv.ToFloatPtr() );
			qglEnd();
			qglFlush();
#endif
		}
		for( j = i + 1 ; j < numOriginalEdges ; j++ )
		{
			optVertex_t*	v1, *v2, *v3, *v4;
			optVertex_t*	newVert;
			edgeCrossing_t*	cross;

			v1 = originalEdges[i].v1;
			v2 = originalEdges[i].v2;
			v3 = originalEdges[j].v1;
			v4 = originalEdges[j].v2;

			if( !EdgesCross( v1, v2, v3, v4 ) )
			{
				continue;
			}

			// this is the only point in optimization where
			// completely new points are created, and it only
			// happens if there is overlapping coplanar
			// geometry in the source triangles
			newVert = EdgeIntersection( v1, v2, v3, v4, opt );

			if( !newVert )
			{
//common->Printf( "lines %i (%i to %i) and %i (%i to %i) are colinear\n", i, v1 - optVerts, v2 - optVerts,
//		   j, v3 - optVerts, v4 - optVerts );	// !@#
				// colinear, so add both verts of each edge to opposite
				if( VertexBetween( v3, v1, v2 ) )
				{
					cross = ( edgeCrossing_t* )Mem_ClearedAlloc( sizeof( *cross ), TAG_TOOLS );
					cross->ov = v3;
					cross->next = crossings[i];
					crossings[i] = cross;
				}

				if( VertexBetween( v4, v1, v2 ) )
				{
					cross = ( edgeCrossing_t* )Mem_ClearedAlloc( sizeof( *cross ), TAG_TOOLS );
					cross->ov = v4;
					cross->next = crossings[i];
					crossings[i] = cross;
				}

				if( VertexBetween( v1, v3, v4 ) )
				{
					cross = ( edgeCrossing_t* )Mem_ClearedAlloc( sizeof( *cross ), TAG_TOOLS );
					cross->ov = v1;
					cross->next = crossings[j];
					crossings[j] = cross;
				}

				if( VertexBetween( v2, v3, v4 ) )
				{
					cross = ( edgeCrossing_t* )Mem_ClearedAlloc( sizeof( *cross ), TAG_TOOLS );
					cross->ov = v2;
					cross->next = crossings[j];
					crossings[j] = cross;
				}

				continue;
			}
#if 0
			if( newVert && newVert != v1 && newVert != v2 && newVert != v3 && newVert != v4 )
			{
				common->Printf( "lines %i (%i to %i) and %i (%i to %i) cross at new point %i\n", i, v1 - optVerts, v2 - optVerts,
								j, v3 - optVerts, v4 - optVerts, newVert - optVerts );
			}
			else if( newVert )
			{
				common->Printf( "lines %i (%i to %i) and %i (%i to %i) intersect at old point %i\n", i, v1 - optVerts, v2 - optVerts,
								j, v3 - optVerts, v4 - optVerts, newVert - optVerts );
			}
#endif
			if( newVert != v1 && newVert != v2 )
			{
				cross = ( edgeCrossing_t* )Mem_ClearedAlloc( sizeof( *cross ), TAG_TOOLS );
				cross->ov = newVert;
				cross->next = crossings[i];
				crossings[i] = cross;
			}

			if( newVert != v3 && newVert != v4 )
			{
				cross = ( edgeCrossing_t* )Mem_ClearedAlloc( sizeof( *cross ), TAG_TOOLS );
				cross->ov = newVert;
				cross->next = crossings[j];
				crossings[j] = cross;
			}

		}
	}


	// now split each edge by its crossing points
	// colinear edges will have duplicated edges added, but it won't hurt anything
	for( i = 0 ; i < numOriginalEdges ; i++ )
	{
		edgeCrossing_t*	cross, *nextCross;
		int				numCross;
		optVertex_t**		sorted;

		numCross = 0;
		for( cross = crossings[i] ; cross ; cross = cross->next )
		{
			numCross++;
		}
		numCross += 2;	// account for originals
		sorted = ( optVertex_t** )Mem_Alloc( numCross * sizeof( *sorted ), TAG_TOOLS );
		sorted[0] = originalEdges[i].v1;
		sorted[1] = originalEdges[i].v2;
		j = 2;
		for( cross = crossings[i] ; cross ; cross = nextCross )
		{
			nextCross = cross->next;
			sorted[j] = cross->ov;
			Mem_Free( cross );
			j++;
		}

		// add all possible fragment combinations that aren't divided
		// by another point
		for( j = 0 ; j < numCross ; j++ )
		{
			for( k = j + 1 ; k < numCross ; k++ )
			{
				for( l = 0 ; l < numCross ; l++ )
				{
					if( sorted[l] == sorted[j] || sorted[l] == sorted[k] )
					{
						continue;
					}
					if( sorted[j] == sorted[k] )
					{
						continue;
					}
					if( VertexBetween( sorted[l], sorted[j], sorted[k] ) )
					{
						break;
					}
				}
				if( l == numCross )
				{
//common->Printf( "line %i fragment from point %i to %i\n", i, sorted[j] - optVerts, sorted[k] - optVerts );
					AddEdgeIfNotAlready( sorted[j], sorted[k] );
				}
			}
		}

		Mem_Free( sorted );
	}


	Mem_Free( crossings );
	Mem_Free( originalEdges );

	// check for duplicated edges
	for( i = 0 ; i < numOptEdges ; i++ )
	{
		for( j = i + 1 ; j < numOptEdges ; j++ )
		{
			if( ( optEdges[i].v1 == optEdges[j].v1 && optEdges[i].v2 == optEdges[j].v2 )
					|| ( optEdges[i].v1 == optEdges[j].v2 && optEdges[i].v2 == optEdges[j].v1 ) )
			{
				common->Printf( "duplicated optEdge\n" );
			}
		}
	}

	if( dmapGlobals.verbose )
	{
		common->Printf( "%6i original edges\n", numOriginalEdges );
		common->Printf( "%6i edges after splits\n", numOptEdges );
		common->Printf( "%6i original vertexes\n", numOriginalVerts );
		common->Printf( "%6i vertexes after splits\n", numOptVerts );
	}
}

//=================================================================


/*
===================
CullUnusedVerts

Unlink any verts with no edges, so they
won't be used in the retriangulation
===================
*/
static void CullUnusedVerts( optIsland_t* island )
{
	optVertex_t**	prev, *vert;
	int			c_keep, c_free;
	optEdge_t*	edge;

	c_keep = 0;
	c_free = 0;

	for( prev = &island->verts ; *prev ; )
	{
		vert = *prev;

		if( !vert->edges )
		{
			// free it
			*prev = vert->islandLink;
			c_free++;
		}
		else
		{
			edge = vert->edges;
			if( ( edge->v1 == vert && !edge->v1link )
					|| ( edge->v2 == vert && !edge->v2link ) )
			{
				// is is occasionally possible to get a vert
				// with only a single edge when colinear optimizations
				// crunch down a complex sliver
				UnlinkEdge( edge, island );
				// free it
				*prev = vert->islandLink;
				c_free++;
			}
			else
			{
				prev = &vert->islandLink;
				c_keep++;
			}
		}
	}

	if( dmapGlobals.verbose )
	{
		common->Printf( "%6i verts kept\n", c_keep );
		common->Printf( "%6i verts freed\n", c_free );
	}
}



/*
====================
OptimizeIsland

At this point, all needed vertexes are already in the
list, including any that were added at crossing points.

Interior and colinear vertexes will be removed, and
a new triangulation will be created.
====================
*/
static void OptimizeIsland( optIsland_t* island )
{
	// add space-filling fake edges so we have a complete
	// triangulation of a convex hull before optimization
	AddInteriorEdges( island );
	DrawEdges( island );

	// determine all the possible triangles, and decide if
	// the are filled or empty
	BuildOptTriangles( island );

	// remove interior vertexes that have filled triangles
	// between all their edges
	RemoveInteriorEdges( island );
	DrawEdges( island );

	ValidateEdgeCounts( island );

	// remove vertexes that only have two colinear edges
	CombineColinearEdges( island );
	CullUnusedVerts( island );
	DrawEdges( island );

	// add new internal edges between the remaining exterior edges
	// to give us a full triangulation again
	AddInteriorEdges( island );
	DrawEdges( island );

	// determine all the possible triangles, and decide if
	// the are filled or empty
	BuildOptTriangles( island );

	// make mapTri_t out of the filled optTri_t
	RegenerateTriangles( island );
}

/*
================
AddVertexToIsland_r
================
*/
static void AddVertexToIsland_r( optVertex_t* vert, optIsland_t* island )
{
	optEdge_t*	e;

	// we can't just check islandLink, because the
	// last vert will have a NULL
	if( vert->addedToIsland )
	{
		return;
	}
	vert->addedToIsland = true;
	vert->islandLink = island->verts;
	island->verts = vert;

	for( e = vert->edges ; e ; )
	{
		if( !e->addedToIsland )
		{
			e->addedToIsland = true;

			e->islandLink = island->edges;
			island->edges = e;
		}

		if( e->v1 == vert )
		{
			AddVertexToIsland_r( e->v2, island );
			e = e->v1link;
			continue;
		}
		if( e->v2 == vert )
		{
			AddVertexToIsland_r( e->v1, island );
			e = e->v2link;
			continue;
		}
		common->Error( "AddVertexToIsland_r: mislinked vert" );
	}

}


static void DontSeparateIslands( optimizeGroup_t* opt )
{
	int		i;
	optIsland_t	island;

	DrawAllEdges();

	memset( &island, 0, sizeof( island ) );
	island.group = opt;

	// link everything together
	for( i = 0 ; i < numOptVerts ; i++ )
	{
		optVerts[i].islandLink = island.verts;
		island.verts = &optVerts[i];
	}

	for( i = 0 ; i < numOptEdges ; i++ )
	{
		optEdges[i].islandLink = island.edges;
		island.edges = &optEdges[i];
	}

	OptimizeIsland( &island );
}

/*
====================
OptimizeOptList
====================
*/
static	void OptimizeOptList( optimizeGroup_t* opt )
{
	optimizeGroup_t*	oldNext;

	// fix the t junctions among this single list
	// so we can match edges
	// can we avoid doing this if colinear vertexes break edges?
	oldNext = opt->nextGroup;
	opt->nextGroup = NULL;
	FixAreaGroupsTjunctions( opt );
	opt->nextGroup = oldNext;

	// create the 2D vectors
	dmapGlobals.mapPlanes[opt->planeNum].Normal().NormalVectors( opt->axis[0], opt->axis[1] );

	AddOriginalEdges( opt );
	SplitOriginalEdgesAtCrossings( opt );

#if 0
	// seperate any discontinuous areas for individual optimization
	// to reduce the scope of the problem
	SeparateIslands( opt );
#else
	DontSeparateIslands( opt );
#endif

	// now free the hash verts
	FreeTJunctionHash();

	// free the original list and use the new one
	FreeTriList( opt->triList );
	opt->triList = opt->regeneratedTris;
	opt->regeneratedTris = NULL;
}


/*
==================
SetGroupTriPlaneNums

Copies the group planeNum to every triangle in each group
==================
*/
void SetGroupTriPlaneNums( optimizeGroup_t* groups )
{
	mapTri_t*	tri;
	optimizeGroup_t*	group;

	for( group = groups ; group ; group = group->nextGroup )
	{
		for( tri = group->triList ; tri ; tri = tri->next )
		{
			tri->planeNum = group->planeNum;
		}
	}
}


/*
===================
OptimizeGroupList

This will also fix tjunctions

===================
*/
void	OptimizeGroupList( optimizeGroup_t* groupList )
{
	int			c_in, c_edge, c_tjunc2;
	optimizeGroup_t*	group;

	if( !groupList )
	{
		return;
	}

	c_in = CountGroupListTris( groupList );

	// optimize and remove colinear edges, which will
	// re-introduce some t junctions
	for( group = groupList ; group ; group = group->nextGroup )
	{
		OptimizeOptList( group );
	}
	c_edge = CountGroupListTris( groupList );

	// fix t junctions again
	FixAreaGroupsTjunctions( groupList );
	FreeTJunctionHash();
	c_tjunc2 = CountGroupListTris( groupList );

	SetGroupTriPlaneNums( groupList );

	common->Printf( "----- OptimizeAreaGroups Results -----\n" );
	common->Printf( "%6i tris in\n", c_in );
	common->Printf( "%6i tris after edge removal optimization\n", c_edge );
	common->Printf( "%6i tris after final t junction fixing\n", c_tjunc2 );
}


/*
==================
OptimizeEntity
==================
*/
void	OptimizeEntity( uEntity_t* e )
{
	int		i;

	common->Printf( "----- OptimizeEntity -----\n" );
	for( i = 0 ; i < e->numAreas ; i++ )
	{
		OptimizeGroupList( e->areas[i].groups );
	}
}
