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

/*

  All triangle list functions should behave reasonably with NULL lists.

*/

/*
===============
AllocTri
===============
*/
mapTri_t*	AllocTri()
{
	mapTri_t*	tri;

	tri = ( mapTri_t* )Mem_Alloc( sizeof( *tri ), TAG_TOOLS );
	memset( tri, 0, sizeof( *tri ) );
	return tri;
}

/*
===============
FreeTri
===============
*/
void		FreeTri( mapTri_t* tri )
{
	Mem_Free( tri );
}


/*
===============
MergeTriLists

This does not copy any tris, it just relinks them
===============
*/
mapTri_t*	MergeTriLists( mapTri_t* a, mapTri_t* b )
{
	mapTri_t**	prev;

	prev = &a;
	while( *prev )
	{
		prev = &( *prev )->next;
	}

	*prev = b;

	return a;
}


/*
===============
FreeTriList
===============
*/
void FreeTriList( mapTri_t* a )
{
	mapTri_t*	next;

	for( ; a ; a = next )
	{
		next = a->next;
		Mem_Free( a );
	}
}

/*
===============
CopyTriList
===============
*/
mapTri_t*	CopyTriList( const mapTri_t* a )
{
	mapTri_t*	testList;
	const mapTri_t*	tri;

	testList = NULL;
	for( tri = a ; tri ; tri = tri->next )
	{
		mapTri_t*	copy;

		copy = CopyMapTri( tri );
		copy ->next = testList;
		testList = copy;
	}

	return testList;
}


/*
=============
CountTriList
=============
*/
int	CountTriList( const mapTri_t* tri )
{
	int		c;

	c = 0;
	while( tri )
	{
		c++;
		tri = tri->next;
	}

	return c;
}


/*
===============
CopyMapTri
===============
*/
mapTri_t*	CopyMapTri( const mapTri_t* tri )
{
	mapTri_t*		t;

	t = ( mapTri_t* )Mem_Alloc( sizeof( *t ), TAG_TOOLS );
	*t = *tri;

	return t;
}

/*
===============
MapTriArea
===============
*/
float MapTriArea( const mapTri_t* tri )
{
	return idWinding::TriangleArea( tri->v[0].xyz, tri->v[1].xyz, tri->v[2].xyz );
}

/*
===============
RemoveBadTris

Return a new list with any zero or negative area triangles removed
===============
*/
mapTri_t*	RemoveBadTris( const mapTri_t* list )
{
	mapTri_t*	newList;
	mapTri_t*	copy;
	const mapTri_t*	tri;

	newList = NULL;

	for( tri = list ; tri ; tri = tri->next )
	{
		if( MapTriArea( tri ) > 0 )
		{
			copy = CopyMapTri( tri );
			copy->next = newList;
			newList = copy;
		}
	}

	return newList;
}

/*
================
BoundTriList
================
*/
void BoundTriList( const mapTri_t* list, idBounds& b )
{
	b.Clear();
	for( ; list ; list = list->next )
	{
		b.AddPoint( list->v[0].xyz );
		b.AddPoint( list->v[1].xyz );
		b.AddPoint( list->v[2].xyz );
	}
}

/*
================
DrawTri
================
*/
void DrawTri( const mapTri_t* tri )
{
	idWinding w;

	w.SetNumPoints( 3 );
	w[0] = tri->v[0].xyz;
	w[1] = tri->v[1].xyz;
	w[2] = tri->v[2].xyz;
	DrawWinding( &w );
}


/*
================
FlipTriList

Swaps the vertex order
================
*/
void	FlipTriList( mapTri_t* tris )
{
	mapTri_t*	tri;

	for( tri = tris ; tri ; tri = tri->next )
	{
		idDrawVert	v;
		const struct hashVert_s* hv;
		struct optVertex_s*	ov;

		v = tri->v[0];
		tri->v[0] = tri->v[2];
		tri->v[2] = v;

		hv = tri->hashVert[0];
		tri->hashVert[0] = tri->hashVert[2];
		tri->hashVert[2] = hv;

		ov = tri->optVert[0];
		tri->optVert[0] = tri->optVert[2];
		tri->optVert[2] = ov;
	}
}

/*
================
WindingForTri
================
*/
idWinding* WindingForTri( const mapTri_t* tri )
{
	idWinding*	w;

	w = new idWinding( 3 );
	w->SetNumPoints( 3 );
	( *w )[0] = tri->v[0].xyz;
	( *w )[1] = tri->v[1].xyz;
	( *w )[2] = tri->v[2].xyz;
	return w;
}

/*
================
TriVertsFromOriginal

Regenerate the texcoords and colors on a fragmented tri from the plane equations
================
*/
void		TriVertsFromOriginal( mapTri_t* tri, const mapTri_t* original )
{
	int		i, j;
	float	denom;

	denom = idWinding::TriangleArea( original->v[0].xyz, original->v[1].xyz, original->v[2].xyz );
	if( denom == 0 )
	{
		return;		// original was degenerate, so it doesn't matter
	}

	for( i = 0 ; i < 3 ; i++ )
	{
		float	a, b, c;

		// find the barycentric coordinates
		a = idWinding::TriangleArea( tri->v[i].xyz, original->v[1].xyz, original->v[2].xyz ) / denom;
		b = idWinding::TriangleArea( tri->v[i].xyz, original->v[2].xyz, original->v[0].xyz ) / denom;
		c = idWinding::TriangleArea( tri->v[i].xyz, original->v[0].xyz, original->v[1].xyz ) / denom;

		// regenerate the interpolated values

		// RB begin
		const idVec2 aST = original->v[0].GetTexCoord();
		const idVec2 bST = original->v[1].GetTexCoord();
		const idVec2 cST = original->v[2].GetTexCoord();

		tri->v[i].SetTexCoord(	a * aST.x + b * bST.x + c * cST.x,
								a * aST.y + b * bST.y + c * cST.y );

		idVec3 tempNormal;
		for( j = 0 ; j < 3 ; j++ )
		{
			tempNormal[j] = a * original->v[0].GetNormal()[j] + b * original->v[1].GetNormal()[j] + c * original->v[2].GetNormal()[j];
		}
		tempNormal.Normalize();
		tri->v[i].SetNormal( tempNormal );
		// RB end
	}
}

/*
================
WindingToTriList

Generates a new list of triangles with proper texcoords from a winding
created by clipping the originalTri

OriginalTri can be NULL if you don't care about texCoords
================
*/
mapTri_t* WindingToTriList( const idWinding* w, const mapTri_t* originalTri )
{
	mapTri_t*	tri;
	mapTri_t*	triList;
	int			i, j;
	const idVec3*	vec;

	if( !w )
	{
		return NULL;
	}

	triList = NULL;
	for( i = 2 ; i < w->GetNumPoints() ; i++ )
	{
		tri = AllocTri();
		if( !originalTri )
		{
			memset( tri, 0, sizeof( *tri ) );
		}
		else
		{
			*tri = *originalTri;
		}
		tri->next = triList;
		triList = tri;

		for( j = 0 ; j < 3 ; j++ )
		{
			if( j == 0 )
			{
				vec = &( ( *w )[0] ).ToVec3();
			}
			else if( j == 1 )
			{
				vec = &( ( *w )[i - 1] ).ToVec3();
			}
			else
			{
				vec = &( ( *w )[i] ).ToVec3();
			}
			tri->v[j].xyz = *vec;
		}
		if( originalTri )
		{
			TriVertsFromOriginal( tri, originalTri );
		}
	}

	return triList;
}


/*
==================
ClipTriList
==================
*/
void	ClipTriList( const mapTri_t* list, const idPlane& plane, float epsilon,
					 mapTri_t** front, mapTri_t** back )
{
	const mapTri_t* tri;
	mapTri_t*		newList;
	idWinding*		w, *frontW, *backW;

	*front = NULL;
	*back = NULL;

	for( tri = list ; tri ; tri = tri->next )
	{
		w = WindingForTri( tri );
		w->Split( plane, epsilon, &frontW, &backW );

		newList = WindingToTriList( frontW, tri );
		*front = MergeTriLists( *front, newList );

		newList = WindingToTriList( backW, tri );
		*back = MergeTriLists( *back, newList );

		delete w;
	}

}

/*
==================
PlaneForTri
==================
*/
void	PlaneForTri( const mapTri_t* tri, idPlane& plane )
{
	plane.FromPoints( tri->v[0].xyz, tri->v[1].xyz, tri->v[2].xyz );
}
