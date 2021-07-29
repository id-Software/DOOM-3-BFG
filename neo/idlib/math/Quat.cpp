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

/*
=====================
idQuat::ToAngles
=====================
*/
idAngles idQuat::ToAngles() const
{
	return ToMat3().ToAngles();
}

/*
=====================
idQuat::ToRotation
=====================
*/
idRotation idQuat::ToRotation() const
{
	idVec3 vec;
	float angle;

	vec.x = x;
	vec.y = y;
	vec.z = z;
	angle = idMath::ACos( w );
	if( angle == 0.0f )
	{
		vec.Set( 0.0f, 0.0f, 1.0f );
	}
	else
	{
		//vec *= (1.0f / sin( angle ));
		vec.Normalize();
		vec.FixDegenerateNormal();
		angle *= 2.0f * idMath::M_RAD2DEG;
	}
	return idRotation( vec3_origin, vec, angle );
}

/*
=====================
idQuat::ToMat3
=====================
*/
idMat3 idQuat::ToMat3() const
{
	idMat3	mat;
	float	wx, wy, wz;
	float	xx, yy, yz;
	float	xy, xz, zz;
	float	x2, y2, z2;

	x2 = x + x;
	y2 = y + y;
	z2 = z + z;

	xx = x * x2;
	xy = x * y2;
	xz = x * z2;

	yy = y * y2;
	yz = y * z2;
	zz = z * z2;

	wx = w * x2;
	wy = w * y2;
	wz = w * z2;

	mat[ 0 ][ 0 ] = 1.0f - ( yy + zz );
	mat[ 0 ][ 1 ] = xy - wz;
	mat[ 0 ][ 2 ] = xz + wy;

	mat[ 1 ][ 0 ] = xy + wz;
	mat[ 1 ][ 1 ] = 1.0f - ( xx + zz );
	mat[ 1 ][ 2 ] = yz - wx;

	mat[ 2 ][ 0 ] = xz - wy;
	mat[ 2 ][ 1 ] = yz + wx;
	mat[ 2 ][ 2 ] = 1.0f - ( xx + yy );

	return mat;
}

/*
=====================
idQuat::ToMat4
=====================
*/
idMat4 idQuat::ToMat4() const
{
	return ToMat3().ToMat4();
}

/*
=====================
idQuat::ToCQuat
=====================
*/
idCQuat idQuat::ToCQuat() const
{
	if( w < 0.0f )
	{
		return idCQuat( -x, -y, -z );
	}
	return idCQuat( x, y, z );
}

/*
============
idQuat::ToAngularVelocity
============
*/
idVec3 idQuat::ToAngularVelocity() const
{
	idVec3 vec;

	vec.x = x;
	vec.y = y;
	vec.z = z;
	vec.Normalize();
	return vec * idMath::ACos( w );
}

/*
=============
idQuat::ToString
=============
*/
const char* idQuat::ToString( int precision ) const
{
	return idStr::FloatArrayToString( ToFloatPtr(), GetDimension(), precision );
}

/*
=====================
idQuat::Slerp

Spherical linear interpolation between two quaternions.
=====================
*/
idQuat& idQuat::Slerp( const idQuat& from, const idQuat& to, float t )
{
	idQuat	temp;
	float	omega, cosom, sinom, scale0, scale1;

	if( t <= 0.0f )
	{
		*this = from;
		return *this;
	}

	if( t >= 1.0f )
	{
		*this = to;
		return *this;
	}

	if( from == to )
	{
		*this = to;
		return *this;
	}

	cosom = from.x * to.x + from.y * to.y + from.z * to.z + from.w * to.w;
	if( cosom < 0.0f )
	{
		temp = -to;
		cosom = -cosom;
	}
	else
	{
		temp = to;
	}

	if( ( 1.0f - cosom ) > 1e-6f )
	{
#if 0
		omega = acos( cosom );
		sinom = 1.0f / sin( omega );
		scale0 = sin( ( 1.0f - t ) * omega ) * sinom;
		scale1 = sin( t * omega ) * sinom;
#else
		scale0 = 1.0f - cosom * cosom;
		sinom = idMath::InvSqrt( scale0 );
		omega = idMath::ATan16( scale0 * sinom, cosom );
		scale0 = idMath::Sin16( ( 1.0f - t ) * omega ) * sinom;
		scale1 = idMath::Sin16( t * omega ) * sinom;
#endif
	}
	else
	{
		scale0 = 1.0f - t;
		scale1 = t;
	}

	*this = ( scale0 * from ) + ( scale1 * temp );
	return *this;
}

/*
========================
idQuat::Lerp

Approximation of spherical linear interpolation between two quaternions. The interpolation
traces out the exact same curve as Slerp but does not maintain a constant speed across the arc.
========================
*/
idQuat& idQuat::Lerp( const idQuat& from, const idQuat& to, const float t )
{
	if( t <= 0.0f )
	{
		*this = from;
		return *this;
	}

	if( t >= 1.0f )
	{
		*this = to;
		return *this;
	}

	if( from == to )
	{
		*this = to;
		return *this;
	}

	float cosom = from.x * to.x + from.y * to.y + from.z * to.z + from.w * to.w;

	float scale0 = 1.0f - t;
	float scale1 = ( cosom >= 0.0f ) ? t : -t;

	x = scale0 * from.x + scale1 * to.x;
	y = scale0 * from.y + scale1 * to.y;
	z = scale0 * from.z + scale1 * to.z;
	w = scale0 * from.w + scale1 * to.w;

	float s = idMath::InvSqrt( x * x + y * y + z * z + w * w );

	x *= s;
	y *= s;
	z *= s;
	w *= s;

	return *this;
}

/*
=============
idCQuat::ToAngles
=============
*/
idAngles idCQuat::ToAngles() const
{
	return ToQuat().ToAngles();
}

/*
=============
idCQuat::ToRotation
=============
*/
idRotation idCQuat::ToRotation() const
{
	return ToQuat().ToRotation();
}

/*
=============
idCQuat::ToMat3
=============
*/
idMat3 idCQuat::ToMat3() const
{
	return ToQuat().ToMat3();
}

/*
=============
idCQuat::ToMat4
=============
*/
idMat4 idCQuat::ToMat4() const
{
	return ToQuat().ToMat4();
}

/*
=============
idCQuat::ToString
=============
*/
const char* idCQuat::ToString( int precision ) const
{
	return idStr::FloatArrayToString( ToFloatPtr(), GetDimension(), precision );
}

/*
=====================
Slerp

Spherical linear interpolation between two quaternions.
=====================
*/
idQuat Slerp( const idQuat& from, const idQuat& to, const float t )
{
	return idQuat().Slerp( from, to, t );
}
