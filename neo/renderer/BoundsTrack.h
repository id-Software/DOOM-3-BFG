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

#ifndef __BOUNDSTRACK_H__
#define __BOUNDSTRACK_H__


struct shortBounds_t;

class idBoundsTrack
{
public:
	idBoundsTrack();
	~idBoundsTrack();

	void ClearAll();

	// more than this will thrash a 32k L1 data cache
	static const int MAX_BOUNDS_TRACK_INDEXES = 2048;

	// the bounds will be clamped and rounded to short integers
	void	SetIndex( const int index, const idBounds& bounds );

	// an index that has been cleared will never be returned by FindIntersections()
	void	ClearIndex( const int index );

	// returns the number of indexes filled in intersectedIndexes[]
	//
	// The intersections may include some bounds that are not truly overlapping
	// due to the rounding from float to short integers.
	int		FindIntersections( const idBounds& testBounds, int intersectedIndexes[ MAX_BOUNDS_TRACK_INDEXES ] ) const;

	// validate implementation
	void	Test();

private:
	// All elements that haven't had SetIndex() called since ClearAll() will be
	// in the cleared state, so they can safely be compared against by an
	// unwound loop.
	shortBounds_t* boundsList;	// [MAX_BOUNDS_TRACK_INDEXES]

	// set to 0 at ClearAll(), maintained greater than the highest index passed
	// to SetIndex() since ClearAll().
	int				maxIndex;
};


#endif // __BOUNDSTRACK_H__
