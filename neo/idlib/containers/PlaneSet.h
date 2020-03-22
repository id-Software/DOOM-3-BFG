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

#ifndef __PLANESET_H__
#define __PLANESET_H__

/*
===============================================================================

	Plane Set

===============================================================================
*/

class idPlaneSet : public idList<idPlane>
{
public:

	void					Clear()
	{
		idList<idPlane>::Clear();
		hash.Free();
	}

	int						FindPlane( const idPlane& plane, const float normalEps, const float distEps );

private:
	idHashIndex				hash;
};

ID_INLINE int idPlaneSet::FindPlane( const idPlane& plane, const float normalEps, const float distEps )
{
	int i, border, hashKey;

	assert( distEps <= 0.125f );

	hashKey = ( int )( idMath::Fabs( plane.Dist() ) * 0.125f );
	for( border = -1; border <= 1; border++ )
	{
		for( i = hash.First( hashKey + border ); i >= 0; i = hash.Next( i ) )
		{
			if( ( *this )[i].Compare( plane, normalEps, distEps ) )
			{
				return i;
			}
		}
	}

	if( plane.Type() >= PLANETYPE_NEGX && plane.Type() < PLANETYPE_TRUEAXIAL )
	{
		Append( -plane );
		hash.Add( hashKey, Num() - 1 );
		Append( plane );
		hash.Add( hashKey, Num() - 1 );
		return ( Num() - 1 );
	}
	else
	{
		Append( plane );
		hash.Add( hashKey, Num() - 1 );
		Append( -plane );
		hash.Add( hashKey, Num() - 1 );
		return ( Num() - 2 );
	}
}

#endif /* !__PLANESET_H__ */
