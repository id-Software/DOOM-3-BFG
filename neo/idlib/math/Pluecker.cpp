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

#pragma hdrstop
#include "precompiled.h"

idPluecker pluecker_origin( 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f );

/*
================
idPluecker::FromPlanes

  pluecker coordinate for the intersection of two planes
================
*/
bool idPluecker::FromPlanes( const idPlane& p1, const idPlane& p2 )
{

	p[0] = -( p1[2] * -p2[3] - p2[2] * -p1[3] );
	p[1] = -( p2[1] * -p1[3] - p1[1] * -p2[3] );
	p[2] = p1[1] * p2[2] - p2[1] * p1[2];

	p[3] = -( p1[0] * -p2[3] - p2[0] * -p1[3] );
	p[4] = p1[0] * p2[1] - p2[0] * p1[1];
	p[5] = p1[0] * p2[2] - p2[0] * p1[2];

	return ( p[2] != 0.0f || p[5] != 0.0f || p[4] != 0.0f );
}

/*
================
idPluecker::Distance3DSqr

  calculates square of shortest distance between the two
  3D lines represented by their pluecker coordinates
================
*/
float idPluecker::Distance3DSqr( const idPluecker& a ) const
{
	float d, s;
	idVec3 dir;

	dir[0] = -a.p[5] *  p[4] -  a.p[4] * -p[5];
	dir[1] =  a.p[4] *  p[2] -  a.p[2] *  p[4];
	dir[2] =  a.p[2] * -p[5] - -a.p[5] *  p[2];
	if( dir[0] == 0.0f && dir[1] == 0.0f && dir[2] == 0.0f )
	{
		return -1.0f;	// FIXME: implement for parallel lines
	}
	d = a.p[4] * ( p[2] * dir[1] - -p[5] * dir[0] ) +
		a.p[5] * ( p[2] * dir[2] -  p[4] * dir[0] ) +
		a.p[2] * ( -p[5] * dir[2] -  p[4] * dir[1] );
	s = PermutedInnerProduct( a ) / d;
	return ( dir * dir ) * ( s * s );
}

/*
=============
idPluecker::ToString
=============
*/
const char* idPluecker::ToString( int precision ) const
{
	return idStr::FloatArrayToString( ToFloatPtr(), GetDimension(), precision );
}
