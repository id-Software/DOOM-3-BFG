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

#include "dmap.h"

int		c_active_brushes;

int		c_nodes;

// if a brush just barely pokes onto the other side,
// let it slide by without chopping
#define	PLANESIDE_EPSILON	0.001
//0.1




/*
================
CountBrushList
================
*/
int	CountBrushList( uBrush_t* brushes )
{
	int	c;

	c = 0;
	for( ; brushes ; brushes = brushes->next )
	{
		c++;
	}
	return c;
}


int BrushSizeForSides( int numsides )
{
	int		c;

	// allocate a structure with a variable number of sides at the end
//	c = (int)&(((uBrush_t *)0)->sides[numsides]);	// bounds checker complains about this
	c = sizeof( uBrush_t ) + sizeof( side_t ) * ( numsides - 6 );

	return c;
}

/*
================
AllocBrush
================
*/
uBrush_t* AllocBrush( int numsides )
{
	uBrush_t*	bb;
	int			c;

	c = BrushSizeForSides( numsides );

	bb = ( uBrush_t* )Mem_Alloc( c, TAG_TOOLS );
	memset( bb, 0, c );
	c_active_brushes++;
	return bb;
}

/*
================
FreeBrush
================
*/
void FreeBrush( uBrush_t* brushes )
{
	int			i;

	for( i = 0 ; i < brushes->numsides ; i++ )
	{
		if( brushes->sides[i].winding )
		{
			delete brushes->sides[i].winding;
		}
		if( brushes->sides[i].visibleHull )
		{
			delete brushes->sides[i].visibleHull;
		}
	}
	Mem_Free( brushes );
	c_active_brushes--;
}


/*
================
FreeBrushList
================
*/
void FreeBrushList( uBrush_t* brushes )
{
	uBrush_t*	next;

	for( ; brushes ; brushes = next )
	{
		next = brushes->next;

		FreeBrush( brushes );
	}
}

/*
==================
CopyBrush

Duplicates the brush, the sides, and the windings
==================
*/
uBrush_t* CopyBrush( uBrush_t* brush )
{
	uBrush_t* newbrush;
	int			size;
	int			i;

	size = BrushSizeForSides( brush->numsides );

	newbrush = AllocBrush( brush->numsides );
	memcpy( newbrush, brush, size );

	for( i = 0 ; i < brush->numsides ; i++ )
	{
		if( brush->sides[i].winding )
		{
			newbrush->sides[i].winding = brush->sides[i].winding->Copy();
		}
	}

	return newbrush;
}


/*
================
DrawBrushList
================
*/
void DrawBrushList( uBrush_t* brush )
{
	int		i;
	side_t*	s;

	GLS_BeginScene();
	for( ; brush ; brush = brush->next )
	{
		for( i = 0 ; i < brush->numsides ; i++ )
		{
			s = &brush->sides[i];
			if( !s->winding )
			{
				continue;
			}
			GLS_Winding( s->winding, 0 );
		}
	}
	GLS_EndScene();
}


/*
=============
PrintBrush
=============
*/
void PrintBrush( uBrush_t* brush )
{
	int		i;

	common->Printf( "brush: %p\n", brush );
	for( i = 0; i < brush->numsides ; i++ )
	{
		brush->sides[i].winding->Print();
		common->Printf( "\n" );
	}
}

/*
==================
BoundBrush

Sets the mins/maxs based on the windings
returns false if the brush doesn't enclose a valid volume
==================
*/
bool BoundBrush( uBrush_t* brush )
{
	int			i, j;
	idWinding*	w;

	brush->bounds.Clear();
	for( i = 0; i < brush->numsides; i++ )
	{
		w = brush->sides[i].winding;
		if( !w )
		{
			continue;
		}
		for( j = 0; j < w->GetNumPoints(); j++ )
		{
			brush->bounds.AddPoint( ( *w )[j].ToVec3() );
		}
	}

	for( i = 0; i < 3; i++ )
	{
		if( brush->bounds[0][i] < MIN_WORLD_COORD || brush->bounds[1][i] > MAX_WORLD_COORD
				|| brush->bounds[0][i] >= brush->bounds[1][i] )
		{
			return false;
		}
	}

	return true;
}

/*
==================
CreateBrushWindings

makes basewindigs for sides and mins / maxs for the brush
returns false if the brush doesn't enclose a valid volume
==================
*/
bool CreateBrushWindings( uBrush_t* brush )
{
	int			i, j;
	idWinding*	w;
	idPlane*		plane;
	side_t*		side;

	for( i = 0; i < brush->numsides; i++ )
	{
		side = &brush->sides[i];
		plane = &dmapGlobals.mapPlanes[side->planenum];
		w = new idWinding( *plane );
		for( j = 0; j < brush->numsides && w; j++ )
		{
			if( i == j )
			{
				continue;
			}
			if( brush->sides[j].planenum == ( brush->sides[i].planenum ^ 1 ) )
			{
				continue;		// back side clipaway
			}
			plane = &dmapGlobals.mapPlanes[brush->sides[j].planenum ^ 1];
			w = w->Clip( *plane, 0 );//CLIP_EPSILON);
		}
		if( side->winding )
		{
			delete side->winding;
		}
		side->winding = w;
	}

	return BoundBrush( brush );
}

/*
==================
BrushFromBounds

Creates a new axial brush
==================
*/
uBrush_t*	BrushFromBounds( const idBounds& bounds )
{
	uBrush_t*	b;
	int			i;
	idPlane		plane;

	b = AllocBrush( 6 );
	b->numsides = 6;
	for( i = 0 ; i < 3 ; i++ )
	{
		plane[0] = plane[1] = plane[2] = 0;
		plane[i] = 1;
		plane[3] = -bounds[1][i];
		b->sides[i].planenum = FindFloatPlane( plane );

		plane[i] = -1;
		plane[3] = bounds[0][i];
		b->sides[3 + i].planenum = FindFloatPlane( plane );
	}

	CreateBrushWindings( b );

	return b;
}

/*
==================
BrushVolume

==================
*/
float BrushVolume( uBrush_t* brush )
{
	int			i;
	idWinding*	w;
	idVec3		corner;
	float		d, area, volume;
	idPlane*		plane;

	if( !brush )
	{
		return 0;
	}

	// grab the first valid point as the corner

	w = NULL;
	for( i = 0; i < brush->numsides; i++ )
	{
		w = brush->sides[i].winding;
		if( w )
		{
			break;
		}
	}
	if( !w )
	{
		return 0;
	}
	corner = ( *w )[0].ToVec3();

	// make tetrahedrons to all other faces

	volume = 0;
	for( ; i < brush->numsides; i++ )
	{
		w = brush->sides[i].winding;
		if( !w )
		{
			continue;
		}
		plane = &dmapGlobals.mapPlanes[brush->sides[i].planenum];
		d = -plane->Distance( corner );
		area = w->GetArea();
		volume += d * area;
	}

	volume /= 3;
	return volume;
}


/*
==================
WriteBspBrushMap

FIXME: use new brush format
==================
*/
void WriteBspBrushMap( const char* name, uBrush_t* list )
{
	idFile* 	f;
	side_t* 	s;
	int			i;
	idWinding* 	w;

	common->Printf( "writing %s\n", name );
	f = fileSystem->OpenFileWrite( name );

	if( !f )
	{
		common->Error( "Can't write %s\b", name );
	}

	f->Printf( "{\n\"classname\" \"worldspawn\"\n" );

	for( ; list ; list = list->next )
	{
		f->Printf( "{\n" );
		for( i = 0, s = list->sides ; i < list->numsides ; i++, s++ )
		{
			w = new idWinding( dmapGlobals.mapPlanes[s->planenum] );

			f->Printf( "( %i %i %i ) ", ( int )( *w )[0][0], ( int )( *w )[0][1], ( int )( *w )[0][2] );
			f->Printf( "( %i %i %i ) ", ( int )( *w )[1][0], ( int )( *w )[1][1], ( int )( *w )[1][2] );
			f->Printf( "( %i %i %i ) ", ( int )( *w )[2][0], ( int )( *w )[2][1], ( int )( *w )[2][2] );

			f->Printf( "notexture 0 0 0 1 1\n" );
			delete w;
		}
		f->Printf( "}\n" );
	}
	f->Printf( "}\n" );

	fileSystem->CloseFile( f );

}


//=====================================================================================

/*
====================
FilterBrushIntoTree_r
====================
*/
int FilterBrushIntoTree_r( uBrush_t* b, node_t* node )
{
	uBrush_t*		front, *back;
	int				c;

	if( !b )
	{
		return 0;
	}

	// add it to the leaf list
	if( node->planenum == PLANENUM_LEAF )
	{
		b->next = node->brushlist;
		node->brushlist = b;

		// classify the leaf by the structural brush
		if( b->opaque )
		{
			if( !node->opaque )
			{
				node->opaque = true;
				return 1;
			}
		}

		return 0;
	}

	// split it by the node plane
	SplitBrush( b, node->planenum, &front, &back );
	FreeBrush( b );

	c = 0;
	c += FilterBrushIntoTree_r( front, node->children[0] );
	c += FilterBrushIntoTree_r( back, node->children[1] );

	return c;
}


/*
=====================
FilterBrushesIntoTree

Mark the leafs as opaque and areaportals and put brush
fragments in each leaf so portal surfaces can be matched
to materials
=====================
*/
void FilterBrushesIntoTree( uEntity_t* e )
{
	primitive_t*			prim;
	uBrush_t*			b, *newb;
	int					r;
	int					c_unique, c_clusters;

	common->Printf( "----- FilterBrushesIntoTree -----\n" );

	c_unique = 0;
	c_clusters = 0;
	for( prim = e->primitives ; prim ; prim = prim->next )
	{
		b = prim->brush;
		if( !b )
		{
			continue;
		}
		c_unique++;
		newb = CopyBrush( b );
		r = FilterBrushIntoTree_r( newb, e->tree->headnode );
		c_clusters += r;
	}

	common->Printf( "%5i total brushes\n", c_unique );
	common->Printf( "%5i cluster references\n", c_clusters );
}



/*
================
AllocTree
================
*/
tree_t* AllocTree()
{
	tree_t*	tree;

	tree = ( tree_t* )Mem_Alloc( sizeof( *tree ), TAG_TOOLS );
	memset( tree, 0, sizeof( *tree ) );
	tree->bounds.Clear();

	return tree;
}

/*
================
AllocNode
================
*/
node_t* AllocNode()
{
	node_t*	node;

	node = ( node_t* )Mem_Alloc( sizeof( *node ), TAG_TOOLS );
	memset( node, 0, sizeof( *node ) );

	return node;
}

//============================================================

/*
==================
BrushMostlyOnSide

==================
*/
int BrushMostlyOnSide( uBrush_t* brush, idPlane& plane )
{
	int			i, j;
	idWinding*	w;
	float		d, max;
	int			side;

	max = 0;
	side = PSIDE_FRONT;
	for( i = 0; i < brush->numsides; i++ )
	{
		w = brush->sides[i].winding;
		if( !w )
		{
			continue;
		}
		for( j = 0; j < w->GetNumPoints(); j++ )
		{
			d = plane.Distance( ( *w )[j].ToVec3() );
			if( d > max )
			{
				max = d;
				side = PSIDE_FRONT;
			}
			if( -d > max )
			{
				max = -d;
				side = PSIDE_BACK;
			}
		}
	}
	return side;
}

/*
================
SplitBrush

Generates two new brushes, leaving the original
unchanged
================
*/
void SplitBrush( uBrush_t* brush, int planenum, uBrush_t** front, uBrush_t** back )
{
	uBrush_t*	b[2];
	int			i, j;
	idWinding*	w, *cw[2], *midwinding;
	side_t*		s, *cs;
	float		d, d_front, d_back;

	*front = *back = NULL;
	idPlane& plane = dmapGlobals.mapPlanes[planenum];

	// check all points
	d_front = d_back = 0;
	for( i = 0; i < brush->numsides; i++ )
	{
		w = brush->sides[i].winding;
		if( !w )
		{
			continue;
		}
		for( j = 0; j < w->GetNumPoints(); j++ )
		{
			d = plane.Distance( ( *w )[j].ToVec3() );
			if( d > 0 && d > d_front )
			{
				d_front = d;
			}
			if( d < 0 && d < d_back )
			{
				d_back = d;
			}
		}
	}
	if( d_front < 0.1 ) // PLANESIDE_EPSILON)
	{
		// only on back
		*back = CopyBrush( brush );
		return;
	}
	if( d_back > -0.1 ) // PLANESIDE_EPSILON)
	{
		// only on front
		*front = CopyBrush( brush );
		return;
	}

	// create a new winding from the split plane

	w = new idWinding( plane );
	for( i = 0; i < brush->numsides && w; i++ )
	{
		idPlane& plane2 = dmapGlobals.mapPlanes[brush->sides[i].planenum ^ 1];
		w = w->Clip( plane2, 0 ); // PLANESIDE_EPSILON);
	}

	if( !w || w->IsTiny() )
	{
		// the brush isn't really split
		int		side;

		side = BrushMostlyOnSide( brush, plane );
		if( side == PSIDE_FRONT )
		{
			*front = CopyBrush( brush );
		}
		if( side == PSIDE_BACK )
		{
			*back = CopyBrush( brush );
		}
		return;
	}

	if( w->IsHuge() )
	{
		common->Printf( "WARNING: huge winding\n" );
	}

	midwinding = w;

	// split it for real

	for( i = 0; i < 2; i++ )
	{
		b[i] = AllocBrush( brush->numsides + 1 );
		memcpy( b[i], brush, sizeof( uBrush_t ) - sizeof( brush->sides ) );
		b[i]->numsides = 0;
		b[i]->next = NULL;
		b[i]->original = brush->original;
	}

	// split all the current windings

	for( i = 0; i < brush->numsides; i++ )
	{
		s = &brush->sides[i];
		w = s->winding;
		if( !w )
		{
			continue;
		}
		w->Split( plane, 0 /*PLANESIDE_EPSILON*/, &cw[0], &cw[1] );
		for( j = 0; j < 2; j++ )
		{
			if( !cw[j] )
			{
				continue;
			}
			/*
						if ( cw[j]->IsTiny() )
						{
							delete cw[j];
							continue;
						}
			*/
			cs = &b[j]->sides[b[j]->numsides];
			b[j]->numsides++;
			*cs = *s;
			cs->winding = cw[j];
		}
	}


	// see if we have valid polygons on both sides

	for( i = 0 ; i < 2 ; i++ )
	{
		if( !BoundBrush( b[i] ) )
		{
			break;
		}

		if( b[i]->numsides < 3 )
		{
			FreeBrush( b[i] );
			b[i] = NULL;
		}
	}

	if( !( b[0] && b[1] ) )
	{
		if( !b[0] && !b[1] )
		{
			common->Printf( "split removed brush\n" );
		}
		else
		{
			common->Printf( "split not on both sides\n" );
		}
		if( b[0] )
		{
			FreeBrush( b[0] );
			*front = CopyBrush( brush );
		}
		if( b[1] )
		{
			FreeBrush( b[1] );
			*back = CopyBrush( brush );
		}
		return;
	}

	// add the midwinding to both sides
	for( i = 0 ; i < 2 ; i++ )
	{
		cs = &b[i]->sides[b[i]->numsides];
		b[i]->numsides++;

		cs->planenum = planenum ^ i ^ 1;
		cs->material = NULL;
		if( i == 0 )
		{
			cs->winding = midwinding->Copy();
		}
		else
		{
			cs->winding = midwinding;
		}
	}

	{
		float	v1;
		int		i;

		for( i = 0 ; i < 2 ; i++ )
		{
			v1 = BrushVolume( b[i] );
			if( v1 < 1.0 )
			{
				FreeBrush( b[i] );
				b[i] = NULL;
//			common->Printf ("tiny volume after clip\n");
			}
		}
	}

	*front = b[0];
	*back = b[1];
}
