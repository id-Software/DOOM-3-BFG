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

#ifndef __BV_SPHERE_H__
#define __BV_SPHERE_H__

/*
===============================================================================

	Sphere

===============================================================================
*/

class idSphere
{
public:
	idSphere();
	explicit idSphere( const idVec3& point );
	explicit idSphere( const idVec3& point, const float r );

	float			operator[]( const int index ) const;
	float& 			operator[]( const int index );
	idSphere		operator+( const idVec3& t ) const;				// returns tranlated sphere
	idSphere& 		operator+=( const idVec3& t );					// translate the sphere
	idSphere		operator+( const idSphere& s ) const;
	idSphere& 		operator+=( const idSphere& s );

	bool			Compare( const idSphere& a ) const;							// exact compare, no epsilon
	bool			Compare( const idSphere& a, const float epsilon ) const;	// compare with epsilon
	bool			operator==(	const idSphere& a ) const;						// exact compare, no epsilon
	bool			operator!=(	const idSphere& a ) const;						// exact compare, no epsilon

	void			Clear();									// inside out sphere
	void			Zero();									// single point at origin
	void			SetOrigin( const idVec3& o );					// set origin of sphere
	void			SetRadius( const float r );						// set square radius

	const idVec3& 	GetOrigin() const;						// returns origin of sphere
	float			GetRadius() const;						// returns sphere radius
	bool			IsCleared() const;						// returns true if sphere is inside out

	bool			AddPoint( const idVec3& p );					// add the point, returns true if the sphere expanded
	bool			AddSphere( const idSphere& s );					// add the sphere, returns true if the sphere expanded
	idSphere		Expand( const float d ) const;					// return bounds expanded in all directions with the given value
	idSphere& 		ExpandSelf( const float d );					// expand bounds in all directions with the given value
	idSphere		Translate( const idVec3& translation ) const;
	idSphere& 		TranslateSelf( const idVec3& translation );

	float			PlaneDistance( const idPlane& plane ) const;
	int				PlaneSide( const idPlane& plane, const float epsilon = ON_EPSILON ) const;

	bool			ContainsPoint( const idVec3& p ) const;			// includes touching
	bool			IntersectsSphere( const idSphere& s ) const;	// includes touching
	bool			LineIntersection( const idVec3& start, const idVec3& end ) const;
	// intersection points are (start + dir * scale1) and (start + dir * scale2)
	bool			RayIntersection( const idVec3& start, const idVec3& dir, float& scale1, float& scale2 ) const;

	// Tight sphere for a point set.
	void			FromPoints( const idVec3* points, const int numPoints );
	// Most tight sphere for a translation.
	void			FromPointTranslation( const idVec3& point, const idVec3& translation );
	void			FromSphereTranslation( const idSphere& sphere, const idVec3& start, const idVec3& translation );
	// Most tight sphere for a rotation.
	void			FromPointRotation( const idVec3& point, const idRotation& rotation );
	void			FromSphereRotation( const idSphere& sphere, const idVec3& start, const idRotation& rotation );

	void			AxisProjection( const idVec3& dir, float& min, float& max ) const;

private:
	idVec3			origin;
	float			radius;
};

extern idSphere	sphere_zero;

ID_INLINE idSphere::idSphere()
{
}

ID_INLINE idSphere::idSphere( const idVec3& point )
{
	origin = point;
	radius = 0.0f;
}

ID_INLINE idSphere::idSphere( const idVec3& point, const float r )
{
	origin = point;
	radius = r;
}

ID_INLINE float idSphere::operator[]( const int index ) const
{
	return ( ( float* ) &origin )[index];
}

ID_INLINE float& idSphere::operator[]( const int index )
{
	return ( ( float* ) &origin )[index];
}

ID_INLINE idSphere idSphere::operator+( const idVec3& t ) const
{
	return idSphere( origin + t, radius );
}

ID_INLINE idSphere& idSphere::operator+=( const idVec3& t )
{
	origin += t;
	return *this;
}

ID_INLINE bool idSphere::Compare( const idSphere& a ) const
{
	return ( origin.Compare( a.origin ) && radius == a.radius );
}

ID_INLINE bool idSphere::Compare( const idSphere& a, const float epsilon ) const
{
	return ( origin.Compare( a.origin, epsilon ) && idMath::Fabs( radius - a.radius ) <= epsilon );
}

ID_INLINE bool idSphere::operator==( const idSphere& a ) const
{
	return Compare( a );
}

ID_INLINE bool idSphere::operator!=( const idSphere& a ) const
{
	return !Compare( a );
}

ID_INLINE void idSphere::Clear()
{
	origin.Zero();
	radius = -1.0f;
}

ID_INLINE void idSphere::Zero()
{
	origin.Zero();
	radius = 0.0f;
}

ID_INLINE void idSphere::SetOrigin( const idVec3& o )
{
	origin = o;
}

ID_INLINE void idSphere::SetRadius( const float r )
{
	radius = r;
}

ID_INLINE const idVec3& idSphere::GetOrigin() const
{
	return origin;
}

ID_INLINE float idSphere::GetRadius() const
{
	return radius;
}

ID_INLINE bool idSphere::IsCleared() const
{
	return ( radius < 0.0f );
}

ID_INLINE bool idSphere::AddPoint( const idVec3& p )
{
	if( radius < 0.0f )
	{
		origin = p;
		radius = 0.0f;
		return true;
	}
	else
	{
		float r = ( p - origin ).LengthSqr();
		if( r > radius * radius )
		{
			r = idMath::Sqrt( r );
			origin += ( p - origin ) * 0.5f * ( 1.0f - radius / r );
			radius += 0.5f * ( r - radius );
			return true;
		}
		return false;
	}
}

ID_INLINE bool idSphere::AddSphere( const idSphere& s )
{
	if( radius < 0.0f )
	{
		origin = s.origin;
		radius = s.radius;
		return true;
	}
	else
	{
		float r = ( s.origin - origin ).LengthSqr();
		if( r > ( radius + s.radius ) * ( radius + s.radius ) )
		{
			r = idMath::Sqrt( r );
			origin += ( s.origin - origin ) * 0.5f * ( 1.0f - radius / ( r + s.radius ) );
			radius += 0.5f * ( ( r + s.radius ) - radius );
			return true;
		}
		return false;
	}
}

ID_INLINE idSphere idSphere::Expand( const float d ) const
{
	return idSphere( origin, radius + d );
}

ID_INLINE idSphere& idSphere::ExpandSelf( const float d )
{
	radius += d;
	return *this;
}

ID_INLINE idSphere idSphere::Translate( const idVec3& translation ) const
{
	return idSphere( origin + translation, radius );
}

ID_INLINE idSphere& idSphere::TranslateSelf( const idVec3& translation )
{
	origin += translation;
	return *this;
}

ID_INLINE bool idSphere::ContainsPoint( const idVec3& p ) const
{
	if( ( p - origin ).LengthSqr() > radius * radius )
	{
		return false;
	}
	return true;
}

ID_INLINE bool idSphere::IntersectsSphere( const idSphere& s ) const
{
	float r = s.radius + radius;
	if( ( s.origin - origin ).LengthSqr() > r * r )
	{
		return false;
	}
	return true;
}

ID_INLINE void idSphere::FromPointTranslation( const idVec3& point, const idVec3& translation )
{
	origin = point + 0.5f * translation;
	radius = idMath::Sqrt( 0.5f * translation.LengthSqr() );
}

ID_INLINE void idSphere::FromSphereTranslation( const idSphere& sphere, const idVec3& start, const idVec3& translation )
{
	origin = start + sphere.origin + 0.5f * translation;
	radius = idMath::Sqrt( 0.5f * translation.LengthSqr() ) + sphere.radius;
}

ID_INLINE void idSphere::FromPointRotation( const idVec3& point, const idRotation& rotation )
{
	idVec3 end = rotation * point;
	origin = ( point + end ) * 0.5f;
	radius = idMath::Sqrt( 0.5f * ( end - point ).LengthSqr() );
}

ID_INLINE void idSphere::FromSphereRotation( const idSphere& sphere, const idVec3& start, const idRotation& rotation )
{
	idVec3 end = rotation * sphere.origin;
	origin = start + ( sphere.origin + end ) * 0.5f;
	radius = idMath::Sqrt( 0.5f * ( end - sphere.origin ).LengthSqr() ) + sphere.radius;
}

ID_INLINE void idSphere::AxisProjection( const idVec3& dir, float& min, float& max ) const
{
	float d;
	d = dir * origin;
	min = d - radius;
	max = d + radius;
}

#endif /* !__BV_SPHERE_H__ */
