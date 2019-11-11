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

#ifndef __MATH_ANGLES_H__
#define __MATH_ANGLES_H__

/*
===============================================================================

	Euler angles

===============================================================================
*/

// angle indexes
#define	PITCH				0		// up / down
#define	YAW					1		// left / right
#define	ROLL				2		// fall over

class idVec3;
class idQuat;
class idRotation;
class idMat3;
class idMat4;

class idAngles
{
public:
	float			pitch;
	float			yaw;
	float			roll;

	idAngles();
	idAngles( float pitch, float yaw, float roll );
	explicit idAngles( const idVec3& v );

	void 			Set( float pitch, float yaw, float roll );
	idAngles& 		Zero();

	float			operator[]( int index ) const;
	float& 			operator[]( int index );
	idAngles		operator-() const;			// negate angles, in general not the inverse rotation
	idAngles& 		operator=( const idAngles& a );
	idAngles		operator+( const idAngles& a ) const;
	idAngles& 		operator+=( const idAngles& a );
	idAngles		operator-( const idAngles& a ) const;
	idAngles& 		operator-=( const idAngles& a );
	idAngles		operator*( const float a ) const;
	idAngles& 		operator*=( const float a );
	idAngles		operator/( const float a ) const;
	idAngles& 		operator/=( const float a );

	friend idAngles	operator*( const float a, const idAngles& b );

	bool			Compare( const idAngles& a ) const;							// exact compare, no epsilon
	bool			Compare( const idAngles& a, const float epsilon ) const;	// compare with epsilon
	bool			operator==(	const idAngles& a ) const;						// exact compare, no epsilon
	bool			operator!=(	const idAngles& a ) const;						// exact compare, no epsilon

	idAngles& 		Normalize360();	// normalizes 'this'
	idAngles& 		Normalize180();	// normalizes 'this'

	void			Clamp( const idAngles& min, const idAngles& max );

	int				GetDimension() const;

	void			ToVectors( idVec3* forward, idVec3* right = NULL, idVec3* up = NULL ) const;
	idVec3			ToForward() const;
	idQuat			ToQuat() const;
	idRotation		ToRotation() const;
	idMat3			ToMat3() const;
	idMat4			ToMat4() const;
	idVec3			ToAngularVelocity() const;
	const float* 	ToFloatPtr() const;
	float* 			ToFloatPtr();
	const char* 	ToString( int precision = 2 ) const;
};

extern idAngles ang_zero;

ID_INLINE idAngles::idAngles()
{
}

ID_INLINE idAngles::idAngles( float pitch, float yaw, float roll )
{
	this->pitch = pitch;
	this->yaw	= yaw;
	this->roll	= roll;
}

ID_INLINE idAngles::idAngles( const idVec3& v )
{
	this->pitch = v[0];
	this->yaw	= v[1];
	this->roll	= v[2];
}

ID_INLINE void idAngles::Set( float pitch, float yaw, float roll )
{
	this->pitch = pitch;
	this->yaw	= yaw;
	this->roll	= roll;
}

ID_INLINE idAngles& idAngles::Zero()
{
	pitch = yaw = roll = 0.0f;
	return *this;
}

ID_INLINE float idAngles::operator[]( int index ) const
{
	assert( ( index >= 0 ) && ( index < 3 ) );
	return ( &pitch )[ index ];
}

ID_INLINE float& idAngles::operator[]( int index )
{
	assert( ( index >= 0 ) && ( index < 3 ) );
	return ( &pitch )[ index ];
}

ID_INLINE idAngles idAngles::operator-() const
{
	return idAngles( -pitch, -yaw, -roll );
}

ID_INLINE idAngles& idAngles::operator=( const idAngles& a )
{
	pitch	= a.pitch;
	yaw		= a.yaw;
	roll	= a.roll;
	return *this;
}

ID_INLINE idAngles idAngles::operator+( const idAngles& a ) const
{
	return idAngles( pitch + a.pitch, yaw + a.yaw, roll + a.roll );
}

ID_INLINE idAngles& idAngles::operator+=( const idAngles& a )
{
	pitch	+= a.pitch;
	yaw		+= a.yaw;
	roll	+= a.roll;

	return *this;
}

ID_INLINE idAngles idAngles::operator-( const idAngles& a ) const
{
	return idAngles( pitch - a.pitch, yaw - a.yaw, roll - a.roll );
}

ID_INLINE idAngles& idAngles::operator-=( const idAngles& a )
{
	pitch	-= a.pitch;
	yaw		-= a.yaw;
	roll	-= a.roll;

	return *this;
}

ID_INLINE idAngles idAngles::operator*( const float a ) const
{
	return idAngles( pitch * a, yaw * a, roll * a );
}

ID_INLINE idAngles& idAngles::operator*=( float a )
{
	pitch	*= a;
	yaw		*= a;
	roll	*= a;
	return *this;
}

ID_INLINE idAngles idAngles::operator/( const float a ) const
{
	float inva = 1.0f / a;
	return idAngles( pitch * inva, yaw * inva, roll * inva );
}

ID_INLINE idAngles& idAngles::operator/=( float a )
{
	float inva = 1.0f / a;
	pitch	*= inva;
	yaw		*= inva;
	roll	*= inva;
	return *this;
}

ID_INLINE idAngles operator*( const float a, const idAngles& b )
{
	return idAngles( a * b.pitch, a * b.yaw, a * b.roll );
}

ID_INLINE bool idAngles::Compare( const idAngles& a ) const
{
	return ( ( a.pitch == pitch ) && ( a.yaw == yaw ) && ( a.roll == roll ) );
}

ID_INLINE bool idAngles::Compare( const idAngles& a, const float epsilon ) const
{
	if( idMath::Fabs( pitch - a.pitch ) > epsilon )
	{
		return false;
	}

	if( idMath::Fabs( yaw - a.yaw ) > epsilon )
	{
		return false;
	}

	if( idMath::Fabs( roll - a.roll ) > epsilon )
	{
		return false;
	}

	return true;
}

ID_INLINE bool idAngles::operator==( const idAngles& a ) const
{
	return Compare( a );
}

ID_INLINE bool idAngles::operator!=( const idAngles& a ) const
{
	return !Compare( a );
}

ID_INLINE void idAngles::Clamp( const idAngles& min, const idAngles& max )
{
	if( pitch < min.pitch )
	{
		pitch = min.pitch;
	}
	else if( pitch > max.pitch )
	{
		pitch = max.pitch;
	}
	if( yaw < min.yaw )
	{
		yaw = min.yaw;
	}
	else if( yaw > max.yaw )
	{
		yaw = max.yaw;
	}
	if( roll < min.roll )
	{
		roll = min.roll;
	}
	else if( roll > max.roll )
	{
		roll = max.roll;
	}
}

ID_INLINE int idAngles::GetDimension() const
{
	return 3;
}

ID_INLINE const float* idAngles::ToFloatPtr() const
{
	return &pitch;
}

ID_INLINE float* idAngles::ToFloatPtr()
{
	return &pitch;
}

#endif /* !__MATH_ANGLES_H__ */
