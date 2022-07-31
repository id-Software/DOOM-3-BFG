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
#ifndef __MATH_VECTORI_H__
#define __MATH_VECTORI_H__

static ID_INLINE int MinInt( int a, int b )
{
	return ( a ) < ( b ) ? ( a ) : ( b );
}
static ID_INLINE int MaxInt( int a, int b )
{
	return ( a ) < ( b ) ? ( b ) : ( a );
}

class idVec2i
{
public:
	int      x, y;

	idVec2i() {}
	idVec2i( int _x, int _y ) : x( _x ), y( _y ) {}

	void		Set( int _x, int _y )
	{
		x = _x;
		y = _y;
	}
	int			Area() const
	{
		return x * y;
	};

	void		Min( idVec2i& v )
	{
		x = MinInt( x, v.x );
		y = MinInt( y, v.y );
	}
	void		Max( idVec2i& v )
	{
		x = MaxInt( x, v.x );
		y = MaxInt( y, v.y );
	}

	int			operator[]( const int index ) const
	{
		assert( index == 0 || index == 1 );
		return ( &x )[index];
	}
	int& 		operator[]( const int index )
	{
		assert( index == 0 || index == 1 );
		return ( &x )[index];
	}

	idVec2i 	operator-() const
	{
		return idVec2i( -x, -y );
	}
	idVec2i 	operator!() const
	{
		return idVec2i( !x, !y );
	}

	idVec2i 	operator>>( const int a ) const
	{
		return idVec2i( x >> a, y >> a );
	}
	idVec2i 	operator<<( const int a ) const
	{
		return idVec2i( x << a, y << a );
	}
	idVec2i 	operator&( const int a ) const
	{
		return idVec2i( x & a, y & a );
	}
	idVec2i 	operator|( const int a ) const
	{
		return idVec2i( x | a, y | a );
	}
	idVec2i 	operator^( const int a ) const
	{
		return idVec2i( x ^ a, y ^ a );
	}
	idVec2i 	operator*( const int a ) const
	{
		return idVec2i( x * a, y * a );
	}
	idVec2i 	operator/( const int a ) const
	{
		return idVec2i( x / a, y / a );
	}
	idVec2i 	operator+( const int a ) const
	{
		return idVec2i( x + a, y + a );
	}
	idVec2i 	operator-( const int a ) const
	{
		return idVec2i( x - a, y - a );
	}

	bool		operator==( const idVec2i& a ) const
	{
		return a.x == x && a.y == y;
	};
	bool		operator!=( const idVec2i& a ) const
	{
		return a.x != x || a.y != y;
	};

	idVec2i		operator>>( const idVec2i& a ) const
	{
		return idVec2i( x >> a.x, y >> a.y );
	}
	idVec2i		operator<<( const idVec2i& a ) const
	{
		return idVec2i( x << a.x, y << a.y );
	}
	idVec2i		operator&( const idVec2i& a ) const
	{
		return idVec2i( x & a.x, y & a.y );
	}
	idVec2i		operator|( const idVec2i& a ) const
	{
		return idVec2i( x | a.x, y | a.y );
	}
	idVec2i		operator^( const idVec2i& a ) const
	{
		return idVec2i( x ^ a.x, y ^ a.y );
	}
	idVec2i		operator*( const idVec2i& a ) const
	{
		return idVec2i( x * a.x, y * a.y );
	}
	idVec2i		operator/( const idVec2i& a ) const
	{
		return idVec2i( x / a.x, y / a.y );
	}
	idVec2i		operator+( const idVec2i& a ) const
	{
		return idVec2i( x + a.x, y + a.y );
	}
	idVec2i		operator-( const idVec2i& a ) const
	{
		return idVec2i( x - a.x, y - a.y );
	}

	idVec2i& 	operator+=( const int a )
	{
		x += a;
		y += a;
		return *this;
	}
	idVec2i& 	operator-=( const int a )
	{
		x -= a;
		y -= a;
		return *this;
	}
	idVec2i& 	operator/=( const int a )
	{
		x /= a;
		y /= a;
		return *this;
	}
	idVec2i& 	operator*=( const int a )
	{
		x *= a;
		y *= a;
		return *this;
	}
	idVec2i& 	operator>>=( const int a )
	{
		x >>= a;
		y >>= a;
		return *this;
	}
	idVec2i& 	operator<<=( const int a )
	{
		x <<= a;
		y <<= a;
		return *this;
	}
	idVec2i& 	operator&=( const int a )
	{
		x &= a;
		y &= a;
		return *this;
	}
	idVec2i& 	operator|=( const int a )
	{
		x |= a;
		y |= a;
		return *this;
	}
	idVec2i& 	operator^=( const int a )
	{
		x ^= a;
		y ^= a;
		return *this;
	}

	idVec2i& 	operator>>=( const idVec2i& a )
	{
		x >>= a.x;
		y >>= a.y;
		return *this;
	}
	idVec2i& 	operator<<=( const idVec2i& a )
	{
		x <<= a.x;
		y <<= a.y;
		return *this;
	}
	idVec2i& 	operator&=( const idVec2i& a )
	{
		x &= a.x;
		y &= a.y;
		return *this;
	}
	idVec2i& 	operator|=( const idVec2i& a )
	{
		x |= a.x;
		y |= a.y;
		return *this;
	}
	idVec2i& 	operator^=( const idVec2i& a )
	{
		x ^= a.x;
		y ^= a.y;
		return *this;
	}
	idVec2i& 	operator+=( const idVec2i& a )
	{
		x += a.x;
		y += a.y;
		return *this;
	}
	idVec2i& 	operator-=( const idVec2i& a )
	{
		x -= a.x;
		y -= a.y;
		return *this;
	}
	idVec2i& 	operator/=( const idVec2i& a )
	{
		x /= a.x;
		y /= a.y;
		return *this;
	}
	idVec2i& 	operator*=( const idVec2i& a )
	{
		x *= a.x;
		y *= a.y;
		return *this;
	}
};


//===============================================================
//
//	idVec4i - 4D vector
//
//===============================================================

class idVec4i
{
public:
	uint8			x;
	uint8			y;
	uint8			z;
	uint8			w;

	idVec4i( void );
	explicit idVec4i( const uint8 x )
	{
		Set( x, x, x, x );
	}
	explicit idVec4i( const uint8 x, const uint8 y, const uint8 z, const uint8 w );

	void			Set( const uint8 x, const uint8 y, const uint8 z, const uint8 w );
	void			Zero( void );

	int				operator[]( const int index ) const;
	uint8& operator[]( const int index );
	idVec4i			operator-( ) const;
	uint8			operator*( const idVec4i& a ) const;
	idVec4i			operator*( const uint8 a ) const;
	idVec4i			operator/( const uint8 a ) const;
	idVec4i			operator+( const idVec4i& a ) const;
	idVec4i			operator-( const idVec4i& a ) const;
	idVec4i& operator+=( const idVec4i& a );
	idVec4i& operator-=( const idVec4i& a );
	idVec4i& operator/=( const idVec4i& a );
	idVec4i& operator/=( const uint8 a );
	idVec4i& operator*=( const uint8 a );

	friend idVec4i	operator*( const uint8 a, const idVec4i b );

	idVec4i			Multiply( const idVec4i& a ) const;
	bool			Compare( const idVec4i& a ) const;							// exact compare, no epsilon

	bool			operator==( const idVec4i& a ) const;						// exact compare, no epsilon
	bool			operator!=( const idVec4i& a ) const;						// exact compare, no epsilon

	float			Length( void ) const;
	float			LengthSqr( void ) const;
	float			Normalize( void );			// returns length
	float			NormalizeFast( void );		// returns length

	int				GetDimension( void ) const;

	const uint8* ToIntPtr( void ) const;
	uint8* ToIntPtr( void );
	const char* ToString( int precision = 2 ) const;

	void			Lerp( const idVec4i& v1, const idVec4i& v2, const float l );
};


ID_INLINE idVec4i::idVec4i( void ) { }

ID_INLINE idVec4i::idVec4i( const uint8 x, const uint8 y, const uint8 z, const uint8 w )
{
	this->x = x;
	this->y = y;
	this->z = z;
	this->w = w;
}

ID_INLINE void idVec4i::Set( const uint8 x, const uint8 y, const uint8 z, const uint8 w )
{
	this->x = x;
	this->y = y;
	this->z = z;
	this->w = w;
}

ID_INLINE void idVec4i::Zero( void )
{
	x = y = z = w = 0.0f;
}

ID_INLINE int idVec4i::operator[]( int index ) const
{
	return ( &x )[index];
}

ID_INLINE uint8& idVec4i::operator[]( int index )
{
	return ( &x )[index];
}

ID_INLINE idVec4i idVec4i::operator-( ) const
{
	return idVec4i( -x, -y, -z, -w );
}

ID_INLINE idVec4i idVec4i::operator-( const idVec4i& a ) const
{
	return idVec4i( x - a.x, y - a.y, z - a.z, w - a.w );
}

ID_INLINE uint8 idVec4i::operator*( const idVec4i& a ) const
{
	return x * a.x + y * a.y + z * a.z + w * a.w;
}

ID_INLINE idVec4i idVec4i::operator*( const uint8 a ) const
{
	return idVec4i( x * a, y * a, z * a, w * a );
}

ID_INLINE idVec4i idVec4i::operator/( const uint8 a ) const
{
	float inva = 1.0f / a;
	return idVec4i( x * inva, y * inva, z * inva, w * inva );
}

ID_INLINE idVec4i operator*( const int a, const idVec4i b )
{
	return idVec4i( b.x * a, b.y * a, b.z * a, b.w * a );
}

ID_INLINE idVec4i idVec4i::operator+( const idVec4i& a ) const
{
	return idVec4i( x + a.x, y + a.y, z + a.z, w + a.w );
}

ID_INLINE idVec4i& idVec4i::operator+=( const idVec4i& a )
{
	x += a.x;
	y += a.y;
	z += a.z;
	w += a.w;

	return *this;
}

ID_INLINE idVec4i& idVec4i::operator/=( const idVec4i& a )
{
	x /= a.x;
	y /= a.y;
	z /= a.z;
	w /= a.w;

	return *this;
}

ID_INLINE idVec4i& idVec4i::operator/=( const uint8 a )
{
	float inva = 1.0f / a;
	x *= inva;
	y *= inva;
	z *= inva;
	w *= inva;

	return *this;
}

ID_INLINE idVec4i& idVec4i::operator-=( const idVec4i& a )
{
	x -= a.x;
	y -= a.y;
	z -= a.z;
	w -= a.w;

	return *this;
}

ID_INLINE idVec4i& idVec4i::operator*=( const uint8 a )
{
	x *= a;
	y *= a;
	z *= a;
	w *= a;

	return *this;
}

ID_INLINE idVec4i idVec4i::Multiply( const idVec4i& a ) const
{
	return idVec4i( x * a.x, y * a.y, z * a.z, w * a.w );
}

ID_INLINE bool idVec4i::Compare( const idVec4i& a ) const
{
	return ( ( x == a.x ) && ( y == a.y ) && ( z == a.z ) && w == a.w );
}

ID_INLINE bool idVec4i::operator==( const idVec4i& a ) const
{
	return Compare( a );
}

ID_INLINE bool idVec4i::operator!=( const idVec4i& a ) const
{
	return !Compare( a );
}

ID_INLINE float idVec4i::Length( void ) const
{
	return ( float ) idMath::Sqrt( x * x + y * y + z * z + w * w );
}

ID_INLINE float idVec4i::LengthSqr( void ) const
{
	return ( x * x + y * y + z * z + w * w );
}

ID_INLINE float idVec4i::Normalize( void )
{
	float sqrLength, invLength;

	sqrLength = x * x + y * y + z * z + w * w;
	invLength = idMath::InvSqrt( sqrLength );
	x *= invLength;
	y *= invLength;
	z *= invLength;
	w *= invLength;
	return invLength * sqrLength;
}

ID_INLINE float idVec4i::NormalizeFast( void )
{
	float sqrLength, invLength;

	sqrLength = x * x + y * y + z * z + w * w;
	invLength = idMath::RSqrt( sqrLength );
	x *= invLength;
	y *= invLength;
	z *= invLength;
	w *= invLength;
	return invLength * sqrLength;
}

ID_INLINE int idVec4i::GetDimension( void ) const
{
	return 4;
}

ID_INLINE const uint8* idVec4i::ToIntPtr( void ) const
{
	return &x;
}

ID_INLINE uint8* idVec4i::ToIntPtr( void )
{
	return &x;
}


#endif
