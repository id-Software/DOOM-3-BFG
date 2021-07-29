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

#include "precompiled.h"
#pragma hdrstop


#undef min			// windef.h macros
#undef max

#include "BoundsTrack.h"

/*

We want to do one SIMD compare on 8 short components and know that the bounds
overlap if all 8 tests pass

*/

// shortBounds_t is used to track the reference bounds of all entities in a
// cache-friendly and easy to compare way.
//
// To allow all elements to be compared with a single comparison sense, the maxs
// are stored as negated values.
//
// We may need to add a global scale factor to this if there are intersections
// completely outside +/-32k
struct shortBounds_t
{
	shortBounds_t()
	{
		SetToEmpty();
	}

	shortBounds_t( const idBounds& b )
	{
		SetFromReferenceBounds( b );
	}

	short	b[2][4];		// fourth element is just for padding

	idBounds ToFloatBounds() const
	{
		idBounds	f;
		for( int i = 0 ; i < 3 ; i++ )
		{
			f[0][i] = b[0][i];
			f[1][i] = -b[1][i];
		}
		return f;
	}

	bool	IntersectsShortBounds( shortBounds_t& comp ) const
	{
		shortBounds_t test;
		comp.MakeComparisonBounds( test );
		return IntersectsComparisonBounds( test );
	}

	bool	IntersectsComparisonBounds( shortBounds_t& test ) const
	{
		// this can be a single ALTIVEC vcmpgtshR instruction
		return test.b[0][0] > b[0][0]
			   && test.b[0][1] > b[0][1]
			   && test.b[0][2] > b[0][2]
			   && test.b[0][3] > b[0][3]
			   && test.b[1][0] > b[1][0]
			   && test.b[1][1] > b[1][1]
			   && test.b[1][2] > b[1][2]
			   && test.b[1][3] > b[1][3];
	}

	void MakeComparisonBounds( shortBounds_t& comp ) const
	{
		comp.b[0][0] = -b[1][0];
		comp.b[1][0] = -b[0][0];
		comp.b[0][1] = -b[1][1];
		comp.b[1][1] = -b[0][1];
		comp.b[0][2] = -b[1][2];
		comp.b[1][2] = -b[0][2];
		comp.b[0][3] = 0x7fff;
		comp.b[1][3] = 0x7fff;
	}

	void SetFromReferenceBounds( const idBounds& set )
	{
		// the maxs are stored negated
		for( int i = 0 ; i < 3 ; i++ )
		{
			// RB: replaced std::min, max
			int minv = floor( set[0][i] );
			b[0][i] = Max( -32768, minv );
			int maxv = -ceil( set[1][i] );
			b[1][i] = Min( 32767, maxv );
			// RB end
		}
		b[0][3] = b[1][3] = 0;
	}

	void SetToEmpty()
	{
		// this will always fail the comparison
		for( int i = 0 ; i < 2 ; i++ )
		{
			for( int j = 0 ; j < 4 ; j++ )
			{
				b[i][j] = 0x7fff;
			}
		}
	}
};



// pure function
int FindBoundsIntersectionsTEST(
	const shortBounds_t			testBounds,
	const shortBounds_t* const	boundsList,
	const int					numBounds,
	int* const					returnedList )
{

	int hits = 0;
	idBounds	testF = testBounds.ToFloatBounds();
	for( int i = 0 ; i < numBounds ; i++ )
	{
		idBounds	listF = boundsList[i].ToFloatBounds();
		if( testF.IntersectsBounds( listF ) )
		{
			returnedList[hits++] = i;
		}
	}
	return hits;
}

// pure function
int FindBoundsIntersectionsSimSIMD(
	const shortBounds_t			testBounds,
	const shortBounds_t* const	boundsList,
	const int					numBounds,
	int* const					returnedList )
{

	shortBounds_t	compareBounds;
	testBounds.MakeComparisonBounds( compareBounds );

	int hits = 0;
	for( int i = 0 ; i < numBounds ; i++ )
	{
		const shortBounds_t& listBounds = boundsList[i];
		bool	compare[8];
		int		count = 0;
		for( int j = 0 ; j < 8 ; j++ )
		{
			if( ( ( short* )&compareBounds )[j] >= ( ( short* )&listBounds )[j] )
			{
				compare[j] = true;
				count++;
			}
			else
			{
				compare[j] = false;
			}
		}
		if( count == 8 )
		{
			returnedList[hits++] = i;
		}
	}
	return hits;
}



idBoundsTrack::idBoundsTrack()
{
	boundsList = ( shortBounds_t* )Mem_Alloc( MAX_BOUNDS_TRACK_INDEXES * sizeof( *boundsList ), TAG_RENDER );
	ClearAll();
}

idBoundsTrack::~idBoundsTrack()
{
	Mem_Free( boundsList );
}

void idBoundsTrack::ClearAll()
{
	maxIndex = 0;
	for( int i = 0 ; i < MAX_BOUNDS_TRACK_INDEXES ; i++ )
	{
		ClearIndex( i );
	}
}

void	idBoundsTrack::SetIndex( const int index, const idBounds& bounds )
{
	assert( ( unsigned )index < MAX_BOUNDS_TRACK_INDEXES );
	// RB: replaced std::max
	maxIndex = Max( maxIndex, index + 1 );
	// RB end
	boundsList[index].SetFromReferenceBounds( bounds );
}

void	idBoundsTrack::ClearIndex( const int index )
{
	assert( ( unsigned )index < MAX_BOUNDS_TRACK_INDEXES );
	boundsList[index].SetToEmpty();
}

int		idBoundsTrack::FindIntersections( const idBounds& testBounds, int intersectedIndexes[ MAX_BOUNDS_TRACK_INDEXES ] ) const
{
	const shortBounds_t	shortTestBounds( testBounds );
	return FindBoundsIntersectionsTEST( shortTestBounds, boundsList, maxIndex, intersectedIndexes );
}

void	idBoundsTrack::Test()
{
	ClearAll();
	idRandom	r;

	for( int i = 0 ; i < 1800 ; i++ )
	{
		idBounds b;
		for( int j = 0 ; j < 3 ; j++ )
		{
			b[0][j] = r.RandomInt( 20000 ) - 10000;
			b[1][j] = b[0][j] + r.RandomInt( 1000 );
		}
		SetIndex( i, b );
	}

	const idBounds testBounds( idVec3( -1000, 2000, -3000 ), idVec3( 1500, 4500, -500 ) );
	SetIndex( 1800, testBounds );
	SetIndex( 0, testBounds );

	const shortBounds_t	shortTestBounds( testBounds );

	int intersectedIndexes1[ MAX_BOUNDS_TRACK_INDEXES ];
	const int numHits1 = FindBoundsIntersectionsTEST( shortTestBounds, boundsList, maxIndex, intersectedIndexes1 );

	int intersectedIndexes2[ MAX_BOUNDS_TRACK_INDEXES ];
	const int numHits2 = FindBoundsIntersectionsSimSIMD( shortTestBounds, boundsList, maxIndex, intersectedIndexes2 );
	idLib::Printf( "%i intersections\n", numHits1 );
	if( numHits1 != numHits2 )
	{
		idLib::Printf( "different results\n" );
	}
	else
	{
		for( int i = 0 ; i < numHits1 ; i++ )
		{
			if( intersectedIndexes1[i] != intersectedIndexes2[i] )
			{
				idLib::Printf( "different results\n" );
				break;
			}
		}
	}

	// run again for debugging failure
	FindBoundsIntersectionsTEST( shortTestBounds, boundsList, maxIndex, intersectedIndexes1 );
	FindBoundsIntersectionsSimSIMD( shortTestBounds, boundsList, maxIndex, intersectedIndexes2 );

	// timing
	const int64 start = Sys_Microseconds();
	for( int i = 0 ; i < 40 ; i++ )
	{
		FindBoundsIntersectionsSimSIMD( shortTestBounds, boundsList, maxIndex, intersectedIndexes2 );
	}
	const int64 stop = Sys_Microseconds();
	idLib::Printf( "%lli microseconds for 40 itterations\n", stop - start );
}



class interactionPair_t
{
	int		entityIndex;
	int		lightIndex;
};

/*

keep a sorted list of static interactions and interactions already generated this frame?

determine if the light needs more exact culling because it is rotated or a spot light
for each entity on the bounds intersection list
	if entity is not directly visible, determine if it can cast a shadow into the view
		if the light center is in-frustum
			and the entity bounds is out-of-frustum, it can't contribue
		else the light center is off-frustum
			if any of the view frustum planes can be moved out to the light center and the entity bounds is still outside it, it can't contribute
	if a static interaction exists
		continue
	possibly perform more exact refernce bounds to rotated or spot light

	create an interaction pair and add it to the list


all models will have an interaction with light -1 for ambient surface
sort the interaction list by model
do
	if the model is dynamic, create it
	add the ambient surface and skip interaction -1
	for all interactions
		check for static interaction
		check for current-frame interaction
		else create shadow for this light

*/
