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

#ifndef __MATH_ROTATION_H__
#define __MATH_ROTATION_H__

/*
===============================================================================

	Describes a complete rotation in degrees about an abritray axis.
	A local rotation matrix is stored for fast rotation of multiple points.

===============================================================================
*/


class idAngles;
class idQuat;
class idMat3;

class idRotation
{

	friend class idAngles;
	friend class idQuat;
	friend class idMat3;

public:
	idRotation();
	idRotation( const idVec3& rotationOrigin, const idVec3& rotationVec, const float rotationAngle );

	void				Set( const idVec3& rotationOrigin, const idVec3& rotationVec, const float rotationAngle );
	void				SetOrigin( const idVec3& rotationOrigin );
	void				SetVec( const idVec3& rotationVec );					// has to be normalized
	void				SetVec( const float x, const float y, const float z );	// has to be normalized
	void				SetAngle( const float rotationAngle );
	void				Scale( const float s );
	void				ReCalculateMatrix();
	const idVec3& 		GetOrigin() const;
	const idVec3& 		GetVec() const;
	float				GetAngle() const;

	idRotation			operator-() const;										// flips rotation
	idRotation			operator*( const float s ) const;						// scale rotation
	idRotation			operator/( const float s ) const;						// scale rotation
	idRotation& 		operator*=( const float s );							// scale rotation
	idRotation& 		operator/=( const float s );							// scale rotation
	idVec3				operator*( const idVec3& v ) const;						// rotate vector

	friend idRotation	operator*( const float s, const idRotation& r );		// scale rotation
	friend idVec3		operator*( const idVec3& v, const idRotation& r );		// rotate vector
	friend idVec3& 		operator*=( idVec3& v, const idRotation& r );			// rotate vector

	idAngles			ToAngles() const;
	idQuat				ToQuat() const;
	const idMat3& 		ToMat3() const;
	idMat4				ToMat4() const;
	idVec3				ToAngularVelocity() const;

	void				RotatePoint( idVec3& point ) const;

	void				Normalize180();
	void				Normalize360();

private:
	idVec3				origin;			// origin of rotation
	idVec3				vec;			// normalized vector to rotate around
	float				angle;			// angle of rotation in degrees
	mutable idMat3		axis;			// rotation axis
	mutable bool		axisValid;		// true if rotation axis is valid
};


ID_INLINE idRotation::idRotation()
{
}

ID_INLINE idRotation::idRotation( const idVec3& rotationOrigin, const idVec3& rotationVec, const float rotationAngle )
{
	origin = rotationOrigin;
	vec = rotationVec;
	angle = rotationAngle;
	axisValid = false;
}

ID_INLINE void idRotation::Set( const idVec3& rotationOrigin, const idVec3& rotationVec, const float rotationAngle )
{
	origin = rotationOrigin;
	vec = rotationVec;
	angle = rotationAngle;
	axisValid = false;
}

ID_INLINE void idRotation::SetOrigin( const idVec3& rotationOrigin )
{
	origin = rotationOrigin;
}

ID_INLINE void idRotation::SetVec( const idVec3& rotationVec )
{
	vec = rotationVec;
	axisValid = false;
}

ID_INLINE void idRotation::SetVec( float x, float y, float z )
{
	vec[0] = x;
	vec[1] = y;
	vec[2] = z;
	axisValid = false;
}

ID_INLINE void idRotation::SetAngle( const float rotationAngle )
{
	angle = rotationAngle;
	axisValid = false;
}

ID_INLINE void idRotation::Scale( const float s )
{
	angle *= s;
	axisValid = false;
}

ID_INLINE void idRotation::ReCalculateMatrix()
{
	axisValid = false;
	ToMat3();
}

ID_INLINE const idVec3& idRotation::GetOrigin() const
{
	return origin;
}

ID_INLINE const idVec3& idRotation::GetVec() const
{
	return vec;
}

ID_INLINE float idRotation::GetAngle() const
{
	return angle;
}

ID_INLINE idRotation idRotation::operator-() const
{
	return idRotation( origin, vec, -angle );
}

ID_INLINE idRotation idRotation::operator*( const float s ) const
{
	return idRotation( origin, vec, angle * s );
}

ID_INLINE idRotation idRotation::operator/( const float s ) const
{
	assert( s != 0.0f );
	return idRotation( origin, vec, angle / s );
}

ID_INLINE idRotation& idRotation::operator*=( const float s )
{
	angle *= s;
	axisValid = false;
	return *this;
}

ID_INLINE idRotation& idRotation::operator/=( const float s )
{
	assert( s != 0.0f );
	angle /= s;
	axisValid = false;
	return *this;
}

ID_INLINE idVec3 idRotation::operator*( const idVec3& v ) const
{
	if( !axisValid )
	{
		ToMat3();
	}
	return ( ( v - origin ) * axis + origin );
}

ID_INLINE idRotation operator*( const float s, const idRotation& r )
{
	return r * s;
}

ID_INLINE idVec3 operator*( const idVec3& v, const idRotation& r )
{
	return r * v;
}

ID_INLINE idVec3& operator*=( idVec3& v, const idRotation& r )
{
	v = r * v;
	return v;
}

ID_INLINE void idRotation::RotatePoint( idVec3& point ) const
{
	if( !axisValid )
	{
		ToMat3();
	}
	point = ( ( point - origin ) * axis + origin );
}

#endif /* !__MATH_ROTATION_H__ */
