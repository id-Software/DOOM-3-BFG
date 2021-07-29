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

idBounds bounds_zero( vec3_zero, vec3_zero );
idBounds bounds_zeroOneCube( idVec3( 0.0f ), idVec3( 1.0f ) );
idBounds bounds_unitCube( idVec3( -1.0f ), idVec3( 1.0f ) );

/*
============
idBounds::GetRadius
============
*/
float idBounds::GetRadius() const
{
	int		i;
	float	total, b0, b1;

	total = 0.0f;
	for( i = 0; i < 3; i++ )
	{
		b0 = ( float )idMath::Fabs( b[0][i] );
		b1 = ( float )idMath::Fabs( b[1][i] );
		if( b0 > b1 )
		{
			total += b0 * b0;
		}
		else
		{
			total += b1 * b1;
		}
	}
	return idMath::Sqrt( total );
}

/*
============
idBounds::GetRadius
============
*/
float idBounds::GetRadius( const idVec3& center ) const
{
	int		i;
	float	total, b0, b1;

	total = 0.0f;
	for( i = 0; i < 3; i++ )
	{
		b0 = ( float )idMath::Fabs( center[i] - b[0][i] );
		b1 = ( float )idMath::Fabs( b[1][i] - center[i] );
		if( b0 > b1 )
		{
			total += b0 * b0;
		}
		else
		{
			total += b1 * b1;
		}
	}
	return idMath::Sqrt( total );
}

/*
================
idBounds::PlaneDistance
================
*/
float idBounds::PlaneDistance( const idPlane& plane ) const
{
	idVec3 center;
	float d1, d2;

	center = ( b[0] + b[1] ) * 0.5f;

	d1 = plane.Distance( center );
	d2 = idMath::Fabs( ( b[1][0] - center[0] ) * plane.Normal()[0] ) +
		 idMath::Fabs( ( b[1][1] - center[1] ) * plane.Normal()[1] ) +
		 idMath::Fabs( ( b[1][2] - center[2] ) * plane.Normal()[2] );

	if( d1 - d2 > 0.0f )
	{
		return d1 - d2;
	}
	if( d1 + d2 < 0.0f )
	{
		return d1 + d2;
	}
	return 0.0f;
}

/*
================
idBounds::PlaneSide
================
*/
int idBounds::PlaneSide( const idPlane& plane, const float epsilon ) const
{
	idVec3 center;
	float d1, d2;

	center = ( b[0] + b[1] ) * 0.5f;

	d1 = plane.Distance( center );
	d2 = idMath::Fabs( ( b[1][0] - center[0] ) * plane.Normal()[0] ) +
		 idMath::Fabs( ( b[1][1] - center[1] ) * plane.Normal()[1] ) +
		 idMath::Fabs( ( b[1][2] - center[2] ) * plane.Normal()[2] );

	if( d1 - d2 > epsilon )
	{
		return PLANESIDE_FRONT;
	}
	if( d1 + d2 < -epsilon )
	{
		return PLANESIDE_BACK;
	}
	return PLANESIDE_CROSS;
}

/*
============
idBounds::LineIntersection

  Returns true if the line intersects the bounds between the start and end point.
============
*/
bool idBounds::LineIntersection( const idVec3& start, const idVec3& end ) const
{
	const idVec3 center = ( b[0] + b[1] ) * 0.5f;
	const idVec3 extents = b[1] - center;
	const idVec3 lineDir = 0.5f * ( end - start );
	const idVec3 lineCenter = start + lineDir;
	const idVec3 dir = lineCenter - center;

	const float ld0 = idMath::Fabs( lineDir[0] );
	if( idMath::Fabs( dir[0] ) > extents[0] + ld0 )
	{
		return false;
	}

	const float ld1 = idMath::Fabs( lineDir[1] );
	if( idMath::Fabs( dir[1] ) > extents[1] + ld1 )
	{
		return false;
	}

	const float ld2 = idMath::Fabs( lineDir[2] );
	if( idMath::Fabs( dir[2] ) > extents[2] + ld2 )
	{
		return false;
	}

	const idVec3 cross = lineDir.Cross( dir );

	if( idMath::Fabs( cross[0] ) > extents[1] * ld2 + extents[2] * ld1 )
	{
		return false;
	}

	if( idMath::Fabs( cross[1] ) > extents[0] * ld2 + extents[2] * ld0 )
	{
		return false;
	}

	if( idMath::Fabs( cross[2] ) > extents[0] * ld1 + extents[1] * ld0 )
	{
		return false;
	}

	return true;
}

/*
============
idBounds::RayIntersection

  Returns true if the ray intersects the bounds.
  The ray can intersect the bounds in both directions from the start point.
  If start is inside the bounds it is considered an intersection with scale = 0
============
*/
bool idBounds::RayIntersection( const idVec3& start, const idVec3& dir, float& scale ) const
{
	int i, ax0, ax1, ax2, side, inside;
	float f;
	idVec3 hit;

	ax0 = -1;
	inside = 0;
	for( i = 0; i < 3; i++ )
	{
		if( start[i] < b[0][i] )
		{
			side = 0;
		}
		else if( start[i] > b[1][i] )
		{
			side = 1;
		}
		else
		{
			inside++;
			continue;
		}
		if( dir[i] == 0.0f )
		{
			continue;
		}
		f = ( start[i] - b[side][i] );
		if( ax0 < 0 || idMath::Fabs( f ) > idMath::Fabs( scale * dir[i] ) )
		{
			scale = - ( f / dir[i] );
			ax0 = i;
		}
	}

	if( ax0 < 0 )
	{
		scale = 0.0f;
		// return true if the start point is inside the bounds
		return ( inside == 3 );
	}

	ax1 = ( ax0 + 1 ) % 3;
	ax2 = ( ax0 + 2 ) % 3;
	hit[ax1] = start[ax1] + scale * dir[ax1];
	hit[ax2] = start[ax2] + scale * dir[ax2];

	return ( hit[ax1] >= b[0][ax1] && hit[ax1] <= b[1][ax1] &&
			 hit[ax2] >= b[0][ax2] && hit[ax2] <= b[1][ax2] );
}

/*
============
idBounds::FromTransformedBounds
============
*/
void idBounds::FromTransformedBounds( const idBounds& bounds, const idVec3& origin, const idMat3& axis )
{
	int i;
	idVec3 center, extents, rotatedExtents;

	center = ( bounds[0] + bounds[1] ) * 0.5f;
	extents = bounds[1] - center;

	for( i = 0; i < 3; i++ )
	{
		rotatedExtents[i] = idMath::Fabs( extents[0] * axis[0][i] ) +
							idMath::Fabs( extents[1] * axis[1][i] ) +
							idMath::Fabs( extents[2] * axis[2][i] );
	}

	center = origin + center * axis;
	b[0] = center - rotatedExtents;
	b[1] = center + rotatedExtents;
}

/*
============
idBounds::FromPoints

  Most tight bounds for a point set.
============
*/
void idBounds::FromPoints( const idVec3* points, const int numPoints )
{
	SIMDProcessor->MinMax( b[0], b[1], points, numPoints );
}

/*
============
idBounds::FromPointTranslation

  Most tight bounds for the translational movement of the given point.
============
*/
void idBounds::FromPointTranslation( const idVec3& point, const idVec3& translation )
{
	int i;

	for( i = 0; i < 3; i++ )
	{
		if( translation[i] < 0.0f )
		{
			b[0][i] = point[i] + translation[i];
			b[1][i] = point[i];
		}
		else
		{
			b[0][i] = point[i];
			b[1][i] = point[i] + translation[i];
		}
	}
}

/*
============
idBounds::FromBoundsTranslation

  Most tight bounds for the translational movement of the given bounds.
============
*/
void idBounds::FromBoundsTranslation( const idBounds& bounds, const idVec3& origin, const idMat3& axis, const idVec3& translation )
{
	int i;

	if( axis.IsRotated() )
	{
		FromTransformedBounds( bounds, origin, axis );
	}
	else
	{
		b[0] = bounds[0] + origin;
		b[1] = bounds[1] + origin;
	}
	for( i = 0; i < 3; i++ )
	{
		if( translation[i] < 0.0f )
		{
			b[0][i] += translation[i];
		}
		else
		{
			b[1][i] += translation[i];
		}
	}
}

/*
================
BoundsForPointRotation

  only for rotations < 180 degrees
================
*/
idBounds BoundsForPointRotation( const idVec3& start, const idRotation& rotation )
{
	int i;
	float radiusSqr;
	idVec3 v1, v2;
	idVec3 origin, axis, end;
	idBounds bounds;

	end = start * rotation;
	axis = rotation.GetVec();
	origin = rotation.GetOrigin() + axis * ( axis * ( start - rotation.GetOrigin() ) );
	radiusSqr = ( start - origin ).LengthSqr();
	v1 = ( start - origin ).Cross( axis );
	v2 = ( end - origin ).Cross( axis );

	for( i = 0; i < 3; i++ )
	{
		// if the derivative changes sign along this axis during the rotation from start to end
		if( ( v1[i] > 0.0f && v2[i] < 0.0f ) || ( v1[i] < 0.0f && v2[i] > 0.0f ) )
		{
			if( ( 0.5f * ( start[i] + end[i] ) - origin[i] ) > 0.0f )
			{
				bounds[0][i] = Min( start[i], end[i] );
				bounds[1][i] = origin[i] + idMath::Sqrt( radiusSqr * ( 1.0f - axis[i] * axis[i] ) );
			}
			else
			{
				bounds[0][i] = origin[i] - idMath::Sqrt( radiusSqr * ( 1.0f - axis[i] * axis[i] ) );
				bounds[1][i] = Max( start[i], end[i] );
			}
		}
		else if( start[i] > end[i] )
		{
			bounds[0][i] = end[i];
			bounds[1][i] = start[i];
		}
		else
		{
			bounds[0][i] = start[i];
			bounds[1][i] = end[i];
		}
	}

	return bounds;
}

/*
============
idBounds::FromPointRotation

  Most tight bounds for the rotational movement of the given point.
============
*/
void idBounds::FromPointRotation( const idVec3& point, const idRotation& rotation )
{
	float radius;

	if( idMath::Fabs( rotation.GetAngle() ) < 180.0f )
	{
		( *this ) = BoundsForPointRotation( point, rotation );
	}
	else
	{

		radius = ( point - rotation.GetOrigin() ).Length();

		// FIXME: these bounds are usually way larger
		b[0].Set( -radius, -radius, -radius );
		b[1].Set( radius, radius, radius );
	}
}

/*
============
idBounds::FromBoundsRotation

  Most tight bounds for the rotational movement of the given bounds.
============
*/
void idBounds::FromBoundsRotation( const idBounds& bounds, const idVec3& origin, const idMat3& axis, const idRotation& rotation )
{
	int i;
	float radius;
	idVec3 point;
	idBounds rBounds;

	if( idMath::Fabs( rotation.GetAngle() ) < 180.0f )
	{

		( *this ) = BoundsForPointRotation( bounds[0] * axis + origin, rotation );
		for( i = 1; i < 8; i++ )
		{
			point[0] = bounds[( i ^ ( i >> 1 ) ) & 1][0];
			point[1] = bounds[( i >> 1 ) & 1][1];
			point[2] = bounds[( i >> 2 ) & 1][2];
			( *this ) += BoundsForPointRotation( point * axis + origin, rotation );
		}
	}
	else
	{

		point = ( bounds[1] - bounds[0] ) * 0.5f;
		radius = ( bounds[1] - point ).Length() + ( point - rotation.GetOrigin() ).Length();

		// FIXME: these bounds are usually way larger
		b[0].Set( -radius, -radius, -radius );
		b[1].Set( radius, radius, radius );
	}
}

/*
============
idBounds::ToPoints
============
*/
void idBounds::ToPoints( idVec3 points[8] ) const
{
	for( int i = 0; i < 8; i++ )
	{
		points[i][0] = b[( i ^ ( i >> 1 ) ) & 1][0];
		points[i][1] = b[( i >> 1 ) & 1][1];
		points[i][2] = b[( i >> 2 ) & 1][2];
	}
}
