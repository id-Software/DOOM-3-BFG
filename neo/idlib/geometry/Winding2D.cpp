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
#include "Winding2D.h"

/*
============
GetAxialBevel
============
*/
bool GetAxialBevel( const idVec3& plane1, const idVec3& plane2, const idVec2& point, idVec3& bevel )
{
	if( IEEE_FLT_SIGNBITSET( plane1.x ) ^ IEEE_FLT_SIGNBITSET( plane2.x ) )
	{
		if( idMath::Fabs( plane1.x ) > 0.1f && idMath::Fabs( plane2.x ) > 0.1f )
		{
			bevel.x = 0.0f;
			if( IEEE_FLT_SIGNBITSET( plane1.y ) )
			{
				bevel.y = -1.0f;
			}
			else
			{
				bevel.y = 1.0f;
			}
			bevel.z = - ( point.x * bevel.x + point.y * bevel.y );
			return true;
		}
	}
	if( IEEE_FLT_SIGNBITSET( plane1.y ) ^ IEEE_FLT_SIGNBITSET( plane2.y ) )
	{
		if( idMath::Fabs( plane1.y ) > 0.1f && idMath::Fabs( plane2.y ) > 0.1f )
		{
			bevel.y = 0.0f;
			if( IEEE_FLT_SIGNBITSET( plane1.x ) )
			{
				bevel.x = -1.0f;
			}
			else
			{
				bevel.x = 1.0f;
			}
			bevel.z = - ( point.x * bevel.x + point.y * bevel.y );
			return true;
		}
	}
	return false;
}

/*
============
idWinding2D::ExpandForAxialBox
============
*/
void idWinding2D::ExpandForAxialBox( const idVec2 bounds[2] )
{
	int i, j, numPlanes;
	idVec2 v;
	idVec3 planes[MAX_POINTS_ON_WINDING_2D], plane, bevel;

	// get planes for the edges and add bevels
	for( numPlanes = i = 0; i < numPoints; i++ )
	{
		j = ( i + 1 ) % numPoints;
		if( ( p[j] - p[i] ).LengthSqr() < 0.01f )
		{
			continue;
		}
		plane = Plane2DFromPoints( p[i], p[j], true );
		if( i )
		{
			if( GetAxialBevel( planes[numPlanes - 1], plane, p[i], bevel ) )
			{
				planes[numPlanes++] = bevel;
			}
		}
		assert( numPlanes < MAX_POINTS_ON_WINDING_2D );
		planes[numPlanes++] = plane;
	}
	assert( numPlanes < MAX_POINTS_ON_WINDING_2D && numPlanes > 0 );
	if( GetAxialBevel( planes[numPlanes - 1], planes[0], p[0], bevel ) )
	{
		planes[numPlanes++] = bevel;
	}

	// expand the planes
	for( i = 0; i < numPlanes; i++ )
	{
		v.x = bounds[ IEEE_FLT_SIGNBITSET( planes[i].x ) ].x;
		v.y = bounds[ IEEE_FLT_SIGNBITSET( planes[i].y ) ].y;
		planes[i].z += v.x * planes[i].x + v.y * planes[i].y;
	}

	// get intersection points of the planes
	for( numPoints = i = 0; i < numPlanes; i++ )
	{
		if( Plane2DIntersection( planes[( i + numPlanes - 1 ) % numPlanes], planes[i], p[numPoints] ) )
		{
			numPoints++;
		}
	}
}

/*
============
idWinding2D::Expand
============
*/
void idWinding2D::Expand( const float d )
{
	int i;
	idVec2 edgeNormals[MAX_POINTS_ON_WINDING_2D];

	for( i = 0; i < numPoints; i++ )
	{
		idVec2& start = p[i];
		idVec2& end = p[( i + 1 ) % numPoints];
		edgeNormals[i].x = start.y - end.y;
		edgeNormals[i].y = end.x - start.x;
		edgeNormals[i].Normalize();
		edgeNormals[i] *= d;
	}

	for( i = 0; i < numPoints; i++ )
	{
		p[i] += edgeNormals[i] + edgeNormals[( i + numPoints - 1 ) % numPoints];
	}
}

/*
=============
idWinding2D::Split
=============
*/
int idWinding2D::Split( const idVec3& plane, const float epsilon, idWinding2D** front, idWinding2D** back ) const
{
	float			dists[MAX_POINTS_ON_WINDING_2D];
	byte			sides[MAX_POINTS_ON_WINDING_2D];
	int				counts[3];
	float			dot;
	int				i, j;
	const idVec2* 	p1, *p2;
	idVec2			mid;
	idWinding2D* 	f;
	idWinding2D* 	b;
	int				maxpts;

	counts[0] = counts[1] = counts[2] = 0;

	// determine sides for each point
	for( i = 0; i < numPoints; i++ )
	{
		dists[i] = dot = plane.x * p[i].x + plane.y * p[i].y + plane.z;
		if( dot > epsilon )
		{
			sides[i] = SIDE_FRONT;
		}
		else if( dot < -epsilon )
		{
			sides[i] = SIDE_BACK;
		}
		else
		{
			sides[i] = SIDE_ON;
		}
		counts[sides[i]]++;
	}
	sides[i] = sides[0];
	dists[i] = dists[0];

	*front = *back = NULL;

	// if nothing at the front of the clipping plane
	if( !counts[SIDE_FRONT] )
	{
		*back = Copy();
		return SIDE_BACK;
	}
	// if nothing at the back of the clipping plane
	if( !counts[SIDE_BACK] )
	{
		*front = Copy();
		return SIDE_FRONT;
	}

	maxpts = numPoints + 4;	// cant use counts[0]+2 because of fp grouping errors

	*front = f = new( TAG_IDLIB_WINDING ) idWinding2D;
	*back = b = new( TAG_IDLIB_WINDING ) idWinding2D;

	for( i = 0; i < numPoints; i++ )
	{
		p1 = &p[i];

		if( sides[i] == SIDE_ON )
		{
			f->p[f->numPoints] = *p1;
			f->numPoints++;
			b->p[b->numPoints] = *p1;
			b->numPoints++;
			continue;
		}

		if( sides[i] == SIDE_FRONT )
		{
			f->p[f->numPoints] = *p1;
			f->numPoints++;
		}

		if( sides[i] == SIDE_BACK )
		{
			b->p[b->numPoints] = *p1;
			b->numPoints++;
		}

		if( sides[i + 1] == SIDE_ON || sides[i + 1] == sides[i] )
		{
			continue;
		}

		// generate a split point
		p2 = &p[( i + 1 ) % numPoints];

		// always calculate the split going from the same side
		// or minor epsilon issues can happen
		if( sides[i] == SIDE_FRONT )
		{
			dot = dists[i] / ( dists[i] - dists[i + 1] );
			for( j = 0; j < 2; j++ )
			{
				// avoid round off error when possible
				if( plane[j] == 1.0f )
				{
					mid[j] = plane.z;
				}
				else if( plane[j] == -1.0f )
				{
					mid[j] = -plane.z;
				}
				else
				{
					mid[j] = ( *p1 )[j] + dot * ( ( *p2 )[j] - ( *p1 )[j] );
				}
			}
		}
		else
		{
			dot = dists[i + 1] / ( dists[i + 1] - dists[i] );
			for( j = 0; j < 2; j++ )
			{
				// avoid round off error when possible
				if( plane[j] == 1.0f )
				{
					mid[j] = plane.z;
				}
				else if( plane[j] == -1.0f )
				{
					mid[j] = -plane.z;
				}
				else
				{
					mid[j] = ( *p2 )[j] + dot * ( ( *p1 )[j] - ( *p2 )[j] );
				}
			}
		}

		f->p[f->numPoints] = mid;
		f->numPoints++;
		b->p[b->numPoints] = mid;
		b->numPoints++;
	}

	return SIDE_CROSS;
}

/*
============
idWinding2D::ClipInPlace
============
*/
bool idWinding2D::ClipInPlace( const idVec3& plane, const float epsilon, const bool keepOn )
{
	int i, j, maxpts, newNumPoints;
	int sides[MAX_POINTS_ON_WINDING_2D + 1], counts[3];
	float dot, dists[MAX_POINTS_ON_WINDING_2D + 1];
	idVec2* p1, *p2, mid, newPoints[MAX_POINTS_ON_WINDING_2D + 4];

	counts[SIDE_FRONT] = counts[SIDE_BACK] = counts[SIDE_ON] = 0;

	for( i = 0; i < numPoints; i++ )
	{
		dists[i] = dot = plane.x * p[i].x + plane.y * p[i].y + plane.z;
		if( dot > epsilon )
		{
			sides[i] = SIDE_FRONT;
		}
		else if( dot < -epsilon )
		{
			sides[i] = SIDE_BACK;
		}
		else
		{
			sides[i] = SIDE_ON;
		}
		counts[sides[i]]++;
	}
	sides[i] = sides[0];
	dists[i] = dists[0];

	// if the winding is on the plane and we should keep it
	if( keepOn && !counts[SIDE_FRONT] && !counts[SIDE_BACK] )
	{
		return true;
	}
	if( !counts[SIDE_FRONT] )
	{
		numPoints = 0;
		return false;
	}
	if( !counts[SIDE_BACK] )
	{
		return true;
	}

	maxpts = numPoints + 4;		// cant use counts[0]+2 because of fp grouping errors
	newNumPoints = 0;

	for( i = 0; i < numPoints; i++ )
	{
		p1 = &p[i];

		if( newNumPoints + 1 > maxpts )
		{
			return true;		// can't split -- fall back to original
		}

		if( sides[i] == SIDE_ON )
		{
			newPoints[newNumPoints] = *p1;
			newNumPoints++;
			continue;
		}

		if( sides[i] == SIDE_FRONT )
		{
			newPoints[newNumPoints] = *p1;
			newNumPoints++;
		}

		if( sides[i + 1] == SIDE_ON || sides[i + 1] == sides[i] )
		{
			continue;
		}

		if( newNumPoints + 1 > maxpts )
		{
			return true;		// can't split -- fall back to original
		}

		// generate a split point
		p2 = &p[( i + 1 ) % numPoints];

		dot = dists[i] / ( dists[i] - dists[i + 1] );
		for( j = 0; j < 2; j++ )
		{
			// avoid round off error when possible
			if( plane[j] == 1.0f )
			{
				mid[j] = plane.z;
			}
			else if( plane[j] == -1.0f )
			{
				mid[j] = -plane.z;
			}
			else
			{
				mid[j] = ( *p1 )[j] + dot * ( ( *p2 )[j] - ( *p1 )[j] );
			}
		}

		newPoints[newNumPoints] = mid;
		newNumPoints++;
	}

	if( newNumPoints >= MAX_POINTS_ON_WINDING_2D )
	{
		return true;
	}

	numPoints = newNumPoints;
	memcpy( p, newPoints, newNumPoints * sizeof( idVec2 ) );

	return true;
}

/*
=============
idWinding2D::Copy
=============
*/
idWinding2D* idWinding2D::Copy() const
{
	idWinding2D* w;

	w = new( TAG_IDLIB_WINDING ) idWinding2D;
	w->numPoints = numPoints;
	memcpy( w->p, p, numPoints * sizeof( p[0] ) );
	return w;
}

/*
=============
idWinding2D::Reverse
=============
*/
idWinding2D* idWinding2D::Reverse() const
{
	idWinding2D* w;
	int i;

	w = new( TAG_IDLIB_WINDING ) idWinding2D;
	w->numPoints = numPoints;
	for( i = 0; i < numPoints; i++ )
	{
		w->p[ numPoints - i - 1 ] = p[i];
	}
	return w;
}

/*
============
idWinding2D::GetArea
============
*/
float idWinding2D::GetArea() const
{
	int i;
	idVec2 d1, d2;
	float total;

	total = 0.0f;
	for( i = 2; i < numPoints; i++ )
	{
		d1 = p[i - 1] - p[0];
		d2 = p[i] - p[0];
		total += d1.x * d2.y - d1.y * d2.x;
	}
	return total * 0.5f;
}

/*
============
idWinding2D::GetCenter
============
*/
idVec2 idWinding2D::GetCenter() const
{
	int i;
	idVec2 center;

	center.Zero();
	for( i = 0; i < numPoints; i++ )
	{
		center += p[i];
	}
	center *= ( 1.0f / numPoints );
	return center;
}

/*
============
idWinding2D::GetRadius
============
*/
float idWinding2D::GetRadius( const idVec2& center ) const
{
	int i;
	float radius, r;
	idVec2 dir;

	radius = 0.0f;
	for( i = 0; i < numPoints; i++ )
	{
		dir = p[i] - center;
		r = dir * dir;
		if( r > radius )
		{
			radius = r;
		}
	}
	return idMath::Sqrt( radius );
}

/*
============
idWinding2D::GetBounds
============
*/
void idWinding2D::GetBounds( idVec2 bounds[2] ) const
{
	int i;

	if( !numPoints )
	{
		bounds[0].x = bounds[0].y = idMath::INFINITUM;
		bounds[1].x = bounds[1].y = -idMath::INFINITUM;
		return;
	}
	bounds[0] = bounds[1] = p[0];
	for( i = 1; i < numPoints; i++ )
	{
		if( p[i].x < bounds[0].x )
		{
			bounds[0].x = p[i].x;
		}
		else if( p[i].x > bounds[1].x )
		{
			bounds[1].x = p[i].x;
		}
		if( p[i].y < bounds[0].y )
		{
			bounds[0].y = p[i].y;
		}
		else if( p[i].y > bounds[1].y )
		{
			bounds[1].y = p[i].y;
		}
	}
}

/*
=============
idWinding2D::IsTiny
=============
*/
#define	EDGE_LENGTH		0.2f

bool idWinding2D::IsTiny() const
{
	int		i;
	float	len;
	idVec2	delta;
	int		edges;

	edges = 0;
	for( i = 0; i < numPoints; i++ )
	{
		delta = p[( i + 1 ) % numPoints] - p[i];
		len = delta.Length();
		if( len > EDGE_LENGTH )
		{
			if( ++edges == 3 )
			{
				return false;
			}
		}
	}
	return true;
}

/*
=============
idWinding2D::IsHuge
=============
*/
bool idWinding2D::IsHuge() const
{
	int i, j;

	for( i = 0; i < numPoints; i++ )
	{
		for( j = 0; j < 2; j++ )
		{
			if( p[i][j] <= MIN_WORLD_COORD || p[i][j] >= MAX_WORLD_COORD )
			{
				return true;
			}
		}
	}
	return false;
}

/*
=============
idWinding2D::Print
=============
*/
void idWinding2D::Print() const
{
	int i;

	for( i = 0; i < numPoints; i++ )
	{
		idLib::common->Printf( "(%5.1f, %5.1f)\n", p[i][0], p[i][1] );
	}
}

/*
=============
idWinding2D::PlaneDistance
=============
*/
float idWinding2D::PlaneDistance( const idVec3& plane ) const
{
	int		i;
	float	d, min, max;

	min = idMath::INFINITUM;
	max = -min;
	for( i = 0; i < numPoints; i++ )
	{
		d = plane.x * p[i].x + plane.y * p[i].y + plane.z;
		if( d < min )
		{
			min = d;
			if( IEEE_FLT_SIGNBITSET( min ) & IEEE_FLT_SIGNBITNOTSET( max ) )
			{
				return 0.0f;
			}
		}
		if( d > max )
		{
			max = d;
			if( IEEE_FLT_SIGNBITSET( min ) & IEEE_FLT_SIGNBITNOTSET( max ) )
			{
				return 0.0f;
			}
		}
	}
	if( IEEE_FLT_SIGNBITNOTSET( min ) )
	{
		return min;
	}
	if( IEEE_FLT_SIGNBITSET( max ) )
	{
		return max;
	}
	return 0.0f;
}

/*
=============
idWinding2D::PlaneSide
=============
*/
int idWinding2D::PlaneSide( const idVec3& plane, const float epsilon ) const
{
	bool	front, back;
	int		i;
	float	d;

	front = false;
	back = false;
	for( i = 0; i < numPoints; i++ )
	{
		d = plane.x * p[i].x + plane.y * p[i].y + plane.z;
		if( d < -epsilon )
		{
			if( front )
			{
				return SIDE_CROSS;
			}
			back = true;
			continue;
		}
		else if( d > epsilon )
		{
			if( back )
			{
				return SIDE_CROSS;
			}
			front = true;
			continue;
		}
	}

	if( back )
	{
		return SIDE_BACK;
	}
	if( front )
	{
		return SIDE_FRONT;
	}
	return SIDE_ON;
}

/*
============
idWinding2D::PointInside
============
*/
bool idWinding2D::PointInside( const idVec2& point, const float epsilon ) const
{
	int i;
	float d;
	idVec3 plane;

	for( i = 0; i < numPoints; i++ )
	{
		plane = Plane2DFromPoints( p[i], p[( i + 1 ) % numPoints] );
		d = plane.x * point.x + plane.y * point.y + plane.z;
		if( d > epsilon )
		{
			return false;
		}
	}
	return true;
}

/*
============
idWinding2D::LineIntersection
============
*/
bool idWinding2D::LineIntersection( const idVec2& start, const idVec2& end ) const
{
	int i, numEdges;
	int sides[MAX_POINTS_ON_WINDING_2D + 1], counts[3];
	float d1, d2, epsilon = 0.1f;
	idVec3 plane, edges[2];

	counts[SIDE_FRONT] = counts[SIDE_BACK] = counts[SIDE_ON] = 0;

	plane = Plane2DFromPoints( start, end );
	for( i = 0; i < numPoints; i++ )
	{
		d1 = plane.x * p[i].x + plane.y * p[i].y + plane.z;
		if( d1 > epsilon )
		{
			sides[i] = SIDE_FRONT;
		}
		else if( d1 < -epsilon )
		{
			sides[i] = SIDE_BACK;
		}
		else
		{
			sides[i] = SIDE_ON;
		}
		counts[sides[i]]++;
	}
	sides[i] = sides[0];

	if( !counts[SIDE_FRONT] )
	{
		return false;
	}
	if( !counts[SIDE_BACK] )
	{
		return false;
	}

	numEdges = 0;
	for( i = 0; i < numPoints; i++ )
	{
		if( sides[i] != sides[i + 1] && sides[i + 1] != SIDE_ON )
		{
			edges[numEdges++] = Plane2DFromPoints( p[i], p[( i + 1 ) % numPoints] );
			if( numEdges >= 2 )
			{
				break;
			}
		}
	}
	if( numEdges < 2 )
	{
		return false;
	}

	d1 = edges[0].x * start.x + edges[0].y * start.y + edges[0].z;
	d2 = edges[0].x * end.x + edges[0].y * end.y + edges[0].z;
	if( IEEE_FLT_SIGNBITNOTSET( d1 ) & IEEE_FLT_SIGNBITNOTSET( d2 ) )
	{
		return false;
	}
	d1 = edges[1].x * start.x + edges[1].y * start.y + edges[1].z;
	d2 = edges[1].x * end.x + edges[1].y * end.y + edges[1].z;
	if( IEEE_FLT_SIGNBITNOTSET( d1 ) & IEEE_FLT_SIGNBITNOTSET( d2 ) )
	{
		return false;
	}
	return true;
}

/*
============
idWinding2D::RayIntersection
============
*/
bool idWinding2D::RayIntersection( const idVec2& start, const idVec2& dir, float& scale1, float& scale2, int* edgeNums ) const
{
	int i, numEdges, localEdgeNums[2];
	int sides[MAX_POINTS_ON_WINDING_2D + 1], counts[3];
	float d1, d2, epsilon = 0.1f;
	idVec3 plane, edges[2];

	scale1 = scale2 = 0.0f;
	counts[SIDE_FRONT] = counts[SIDE_BACK] = counts[SIDE_ON] = 0;

	plane = Plane2DFromVecs( start, dir );
	for( i = 0; i < numPoints; i++ )
	{
		d1 = plane.x * p[i].x + plane.y * p[i].y + plane.z;
		if( d1 > epsilon )
		{
			sides[i] = SIDE_FRONT;
		}
		else if( d1 < -epsilon )
		{
			sides[i] = SIDE_BACK;
		}
		else
		{
			sides[i] = SIDE_ON;
		}
		counts[sides[i]]++;
	}
	sides[i] = sides[0];

	if( !counts[SIDE_FRONT] )
	{
		return false;
	}
	if( !counts[SIDE_BACK] )
	{
		return false;
	}

	numEdges = 0;
	for( i = 0; i < numPoints; i++ )
	{
		if( sides[i] != sides[i + 1] && sides[i + 1] != SIDE_ON )
		{
			localEdgeNums[numEdges] = i;
			edges[numEdges++] = Plane2DFromPoints( p[i], p[( i + 1 ) % numPoints] );
			if( numEdges >= 2 )
			{
				break;
			}
		}
	}
	if( numEdges < 2 )
	{
		return false;
	}

	d1 = edges[0].x * start.x + edges[0].y * start.y + edges[0].z;
	d2 = - ( edges[0].x * dir.x + edges[0].y * dir.y );
	if( d2 == 0.0f )
	{
		return false;
	}
	scale1 = d1 / d2;
	d1 = edges[1].x * start.x + edges[1].y * start.y + edges[1].z;
	d2 = - ( edges[1].x * dir.x + edges[1].y * dir.y );
	if( d2 == 0.0f )
	{
		return false;
	}
	scale2 = d1 / d2;

	if( idMath::Fabs( scale1 ) > idMath::Fabs( scale2 ) )
	{
		SwapValues( scale1, scale2 );
		SwapValues( localEdgeNums[0], localEdgeNums[1] );
	}

	if( edgeNums )
	{
		edgeNums[0] = localEdgeNums[0];
		edgeNums[1] = localEdgeNums[1];
	}
	return true;
}
