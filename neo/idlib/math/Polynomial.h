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

#ifndef __MATH_POLYNOMIAL_H__
#define __MATH_POLYNOMIAL_H__

/*
===============================================================================

	Polynomial of arbitrary degree with real coefficients.

===============================================================================
*/


class idPolynomial
{
public:
	idPolynomial();
	explicit idPolynomial( int d );
	explicit idPolynomial( float a, float b );
	explicit idPolynomial( float a, float b, float c );
	explicit idPolynomial( float a, float b, float c, float d );
	explicit idPolynomial( float a, float b, float c, float d, float e );

	float			operator[]( int index ) const;
	float& 			operator[]( int index );

	idPolynomial	operator-() const;
	idPolynomial& 	operator=( const idPolynomial& p );

	idPolynomial	operator+( const idPolynomial& p ) const;
	idPolynomial	operator-( const idPolynomial& p ) const;
	idPolynomial	operator*( const float s ) const;
	idPolynomial	operator/( const float s ) const;

	idPolynomial& 	operator+=( const idPolynomial& p );
	idPolynomial& 	operator-=( const idPolynomial& p );
	idPolynomial& 	operator*=( const float s );
	idPolynomial& 	operator/=( const float s );

	bool			Compare( const idPolynomial& p ) const;						// exact compare, no epsilon
	bool			Compare( const idPolynomial& p, const float epsilon ) const;// compare with epsilon
	bool			operator==(	const idPolynomial& p ) const;					// exact compare, no epsilon
	bool			operator!=(	const idPolynomial& p ) const;					// exact compare, no epsilon

	void			Zero();
	void			Zero( int d );

	int				GetDimension() const;									// get the degree of the polynomial
	int				GetDegree() const;									// get the degree of the polynomial
	float			GetValue( const float x ) const;							// evaluate the polynomial with the given real value
	idComplex		GetValue( const idComplex& x ) const;						// evaluate the polynomial with the given complex value
	idPolynomial	GetDerivative() const;								// get the first derivative of the polynomial
	idPolynomial	GetAntiDerivative() const;							// get the anti derivative of the polynomial

	int				GetRoots( idComplex* roots ) const;							// get all roots
	int				GetRoots( float* roots ) const;								// get the real roots

	static int		GetRoots1( float a, float b, float* roots );
	static int		GetRoots2( float a, float b, float c, float* roots );
	static int		GetRoots3( float a, float b, float c, float d, float* roots );
	static int		GetRoots4( float a, float b, float c, float d, float e, float* roots );

	const float* 	ToFloatPtr() const;
	float* 			ToFloatPtr();
	const char* 	ToString( int precision = 2 ) const;

	static void		Test();

private:
	int				degree;
	int				allocated;
	float* 			coefficient;

	void			Resize( int d, bool keep );
	int				Laguer( const idComplex* coef, const int degree, idComplex& r ) const;
};

ID_INLINE idPolynomial::idPolynomial()
{
	degree = -1;
	allocated = 0;
	coefficient = NULL;
}

ID_INLINE idPolynomial::idPolynomial( int d )
{
	degree = -1;
	allocated = 0;
	coefficient = NULL;
	Resize( d, false );
}

ID_INLINE idPolynomial::idPolynomial( float a, float b )
{
	degree = -1;
	allocated = 0;
	coefficient = NULL;
	Resize( 1, false );
	coefficient[0] = b;
	coefficient[1] = a;
}

ID_INLINE idPolynomial::idPolynomial( float a, float b, float c )
{
	degree = -1;
	allocated = 0;
	coefficient = NULL;
	Resize( 2, false );
	coefficient[0] = c;
	coefficient[1] = b;
	coefficient[2] = a;
}

ID_INLINE idPolynomial::idPolynomial( float a, float b, float c, float d )
{
	degree = -1;
	allocated = 0;
	coefficient = NULL;
	Resize( 3, false );
	coefficient[0] = d;
	coefficient[1] = c;
	coefficient[2] = b;
	coefficient[3] = a;
}

ID_INLINE idPolynomial::idPolynomial( float a, float b, float c, float d, float e )
{
	degree = -1;
	allocated = 0;
	coefficient = NULL;
	Resize( 4, false );
	coefficient[0] = e;
	coefficient[1] = d;
	coefficient[2] = c;
	coefficient[3] = b;
	coefficient[4] = a;
}

ID_INLINE float idPolynomial::operator[]( int index ) const
{
	assert( index >= 0 && index <= degree );
	return coefficient[ index ];
}

ID_INLINE float& idPolynomial::operator[]( int index )
{
	assert( index >= 0 && index <= degree );
	return coefficient[ index ];
}

ID_INLINE idPolynomial idPolynomial::operator-() const
{
	int i;
	idPolynomial n;

	n = *this;
	for( i = 0; i <= degree; i++ )
	{
		n[i] = -n[i];
	}
	return n;
}

ID_INLINE idPolynomial& idPolynomial::operator=( const idPolynomial& p )
{
	Resize( p.degree, false );
	for( int i = 0; i <= degree; i++ )
	{
		coefficient[i] = p.coefficient[i];
	}
	return *this;
}

ID_INLINE idPolynomial idPolynomial::operator+( const idPolynomial& p ) const
{
	int i;
	idPolynomial n;

	if( degree > p.degree )
	{
		n.Resize( degree, false );
		for( i = 0; i <= p.degree; i++ )
		{
			n.coefficient[i] = coefficient[i] + p.coefficient[i];
		}
		for( ; i <= degree; i++ )
		{
			n.coefficient[i] = coefficient[i];
		}
		n.degree = degree;
	}
	else if( p.degree > degree )
	{
		n.Resize( p.degree, false );
		for( i = 0; i <= degree; i++ )
		{
			n.coefficient[i] = coefficient[i] + p.coefficient[i];
		}
		for( ; i <= p.degree; i++ )
		{
			n.coefficient[i] = p.coefficient[i];
		}
		n.degree = p.degree;
	}
	else
	{
		n.Resize( degree, false );
		n.degree = 0;
		for( i = 0; i <= degree; i++ )
		{
			n.coefficient[i] = coefficient[i] + p.coefficient[i];
			if( n.coefficient[i] != 0.0f )
			{
				n.degree = i;
			}
		}
	}
	return n;
}

ID_INLINE idPolynomial idPolynomial::operator-( const idPolynomial& p ) const
{
	int i;
	idPolynomial n;

	if( degree > p.degree )
	{
		n.Resize( degree, false );
		for( i = 0; i <= p.degree; i++ )
		{
			n.coefficient[i] = coefficient[i] - p.coefficient[i];
		}
		for( ; i <= degree; i++ )
		{
			n.coefficient[i] = coefficient[i];
		}
		n.degree = degree;
	}
	else if( p.degree >= degree )
	{
		n.Resize( p.degree, false );
		for( i = 0; i <= degree; i++ )
		{
			n.coefficient[i] = coefficient[i] - p.coefficient[i];
		}
		for( ; i <= p.degree; i++ )
		{
			n.coefficient[i] = - p.coefficient[i];
		}
		n.degree = p.degree;
	}
	else
	{
		n.Resize( degree, false );
		n.degree = 0;
		for( i = 0; i <= degree; i++ )
		{
			n.coefficient[i] = coefficient[i] - p.coefficient[i];
			if( n.coefficient[i] != 0.0f )
			{
				n.degree = i;
			}
		}
	}
	return n;
}

ID_INLINE idPolynomial idPolynomial::operator*( const float s ) const
{
	idPolynomial n;

	if( s == 0.0f )
	{
		n.degree = 0;
	}
	else
	{
		n.Resize( degree, false );
		for( int i = 0; i <= degree; i++ )
		{
			n.coefficient[i] = coefficient[i] * s;
		}
	}
	return n;
}

ID_INLINE idPolynomial idPolynomial::operator/( const float s ) const
{
	float invs;
	idPolynomial n;

	assert( s != 0.0f );
	n.Resize( degree, false );
	invs = 1.0f / s;
	for( int i = 0; i <= degree; i++ )
	{
		n.coefficient[i] = coefficient[i] * invs;
	}
	return n;
}

ID_INLINE idPolynomial& idPolynomial::operator+=( const idPolynomial& p )
{
	int i;

	if( degree > p.degree )
	{
		for( i = 0; i <= p.degree; i++ )
		{
			coefficient[i] += p.coefficient[i];
		}
	}
	else if( p.degree > degree )
	{
		Resize( p.degree, true );
		for( i = 0; i <= degree; i++ )
		{
			coefficient[i] += p.coefficient[i];
		}
		for( ; i <= p.degree; i++ )
		{
			coefficient[i] = p.coefficient[i];
		}
	}
	else
	{
		for( i = 0; i <= degree; i++ )
		{
			coefficient[i] += p.coefficient[i];
			if( coefficient[i] != 0.0f )
			{
				degree = i;
			}
		}
	}
	return *this;
}

ID_INLINE idPolynomial& idPolynomial::operator-=( const idPolynomial& p )
{
	int i;

	if( degree > p.degree )
	{
		for( i = 0; i <= p.degree; i++ )
		{
			coefficient[i] -= p.coefficient[i];
		}
	}
	else if( p.degree > degree )
	{
		Resize( p.degree, true );
		for( i = 0; i <= degree; i++ )
		{
			coefficient[i] -= p.coefficient[i];
		}
		for( ; i <= p.degree; i++ )
		{
			coefficient[i] = - p.coefficient[i];
		}
	}
	else
	{
		for( i = 0; i <= degree; i++ )
		{
			coefficient[i] -= p.coefficient[i];
			if( coefficient[i] != 0.0f )
			{
				degree = i;
			}
		}
	}
	return *this;
}

ID_INLINE idPolynomial& idPolynomial::operator*=( const float s )
{
	if( s == 0.0f )
	{
		degree = 0;
	}
	else
	{
		for( int i = 0; i <= degree; i++ )
		{
			coefficient[i] *= s;
		}
	}
	return *this;
}

ID_INLINE idPolynomial& idPolynomial::operator/=( const float s )
{
	float invs;

	assert( s != 0.0f );
	invs = 1.0f / s;
	for( int i = 0; i <= degree; i++ )
	{
		coefficient[i] = invs;
	}
	return *this;;
}

ID_INLINE bool idPolynomial::Compare( const idPolynomial& p ) const
{
	if( degree != p.degree )
	{
		return false;
	}
	for( int i = 0; i <= degree; i++ )
	{
		if( coefficient[i] != p.coefficient[i] )
		{
			return false;
		}
	}
	return true;
}

ID_INLINE bool idPolynomial::Compare( const idPolynomial& p, const float epsilon ) const
{
	if( degree != p.degree )
	{
		return false;
	}
	for( int i = 0; i <= degree; i++ )
	{
		if( idMath::Fabs( coefficient[i] - p.coefficient[i] ) > epsilon )
		{
			return false;
		}
	}
	return true;
}

ID_INLINE bool idPolynomial::operator==( const idPolynomial& p ) const
{
	return Compare( p );
}

ID_INLINE bool idPolynomial::operator!=( const idPolynomial& p ) const
{
	return !Compare( p );
}

ID_INLINE void idPolynomial::Zero()
{
	degree = 0;
}

ID_INLINE void idPolynomial::Zero( int d )
{
	Resize( d, false );
	for( int i = 0; i <= degree; i++ )
	{
		coefficient[i] = 0.0f;
	}
}

ID_INLINE int idPolynomial::GetDimension() const
{
	return degree;
}

ID_INLINE int idPolynomial::GetDegree() const
{
	return degree;
}

ID_INLINE float idPolynomial::GetValue( const float x ) const
{
	float y, z;
	y = coefficient[0];
	z = x;
	for( int i = 1; i <= degree; i++ )
	{
		y += coefficient[i] * z;
		z *= x;
	}
	return y;
}

ID_INLINE idComplex idPolynomial::GetValue( const idComplex& x ) const
{
	idComplex y, z;
	y.Set( coefficient[0], 0.0f );
	z = x;
	for( int i = 1; i <= degree; i++ )
	{
		y += coefficient[i] * z;
		z *= x;
	}
	return y;
}

ID_INLINE idPolynomial idPolynomial::GetDerivative() const
{
	idPolynomial n;

	if( degree == 0 )
	{
		return n;
	}
	n.Resize( degree - 1, false );
	for( int i = 1; i <= degree; i++ )
	{
		n.coefficient[i - 1] = i * coefficient[i];
	}
	return n;
}

ID_INLINE idPolynomial idPolynomial::GetAntiDerivative() const
{
	idPolynomial n;

	if( degree == 0 )
	{
		return n;
	}
	n.Resize( degree + 1, false );
	n.coefficient[0] = 0.0f;
	for( int i = 0; i <= degree; i++ )
	{
		n.coefficient[i + 1] = coefficient[i] / ( i + 1 );
	}
	return n;
}

ID_INLINE int idPolynomial::GetRoots1( float a, float b, float* roots )
{
	assert( a != 0.0f );
	roots[0] = - b / a;
	return 1;
}

ID_INLINE int idPolynomial::GetRoots2( float a, float b, float c, float* roots )
{
	float inva, ds;

	if( a != 1.0f )
	{
		assert( a != 0.0f );
		inva = 1.0f / a;
		c *= inva;
		b *= inva;
	}
	ds = b * b - 4.0f * c;
	if( ds < 0.0f )
	{
		return 0;
	}
	else if( ds > 0.0f )
	{
		ds = idMath::Sqrt( ds );
		roots[0] = 0.5f * ( -b - ds );
		roots[1] = 0.5f * ( -b + ds );
		return 2;
	}
	else
	{
		roots[0] = 0.5f * -b;
		return 1;
	}
}

ID_INLINE int idPolynomial::GetRoots3( float a, float b, float c, float d, float* roots )
{
	float inva, f, g, halfg, ofs, ds, dist, angle, cs, ss, t;

	if( a != 1.0f )
	{
		assert( a != 0.0f );
		inva = 1.0f / a;
		d *= inva;
		c *= inva;
		b *= inva;
	}

	f = ( 1.0f / 3.0f ) * ( 3.0f * c - b * b );
	g = ( 1.0f / 27.0f ) * ( 2.0f * b * b * b - 9.0f * c * b + 27.0f * d );
	halfg = 0.5f * g;
	ofs = ( 1.0f / 3.0f ) * b;
	ds = 0.25f * g * g + ( 1.0f / 27.0f ) * f * f * f;

	if( ds < 0.0f )
	{
		dist = idMath::Sqrt( ( -1.0f / 3.0f ) * f );
		angle = ( 1.0f / 3.0f ) * idMath::ATan( idMath::Sqrt( -ds ), -halfg );
		cs = idMath::Cos( angle );
		ss = idMath::Sin( angle );
		roots[0] = 2.0f * dist * cs - ofs;
		roots[1] = -dist * ( cs + idMath::SQRT_THREE * ss ) - ofs;
		roots[2] = -dist * ( cs - idMath::SQRT_THREE * ss ) - ofs;
		return 3;
	}
	else if( ds > 0.0f )
	{
		ds = idMath::Sqrt( ds );
		t = -halfg + ds;
		if( t >= 0.0f )
		{
			roots[0] = idMath::Pow( t, ( 1.0f / 3.0f ) );
		}
		else
		{
			roots[0] = -idMath::Pow( -t, ( 1.0f / 3.0f ) );
		}
		t = -halfg - ds;
		if( t >= 0.0f )
		{
			roots[0] += idMath::Pow( t, ( 1.0f / 3.0f ) );
		}
		else
		{
			roots[0] -= idMath::Pow( -t, ( 1.0f / 3.0f ) );
		}
		roots[0] -= ofs;
		return 1;
	}
	else
	{
		if( halfg >= 0.0f )
		{
			t = -idMath::Pow( halfg, ( 1.0f / 3.0f ) );
		}
		else
		{
			t = idMath::Pow( -halfg, ( 1.0f / 3.0f ) );
		}
		roots[0] = 2.0f * t - ofs;
		roots[1] = -t - ofs;
		roots[2] = roots[1];
		return 3;
	}
}

ID_INLINE int idPolynomial::GetRoots4( float a, float b, float c, float d, float e, float* roots )
{
	int count;
	float inva, y, ds, r, s1, s2, t1, t2, tp, tm;
	float roots3[3];

	if( a != 1.0f )
	{
		assert( a != 0.0f );
		inva = 1.0f / a;
		e *= inva;
		d *= inva;
		c *= inva;
		b *= inva;
	}

	count = 0;

	GetRoots3( 1.0f, -c, b * d - 4.0f * e, -b * b * e + 4.0f * c * e - d * d, roots3 );
	y = roots3[0];
	ds = 0.25f * b * b - c + y;

	if( ds < 0.0f )
	{
		return 0;
	}
	else if( ds > 0.0f )
	{
		r = idMath::Sqrt( ds );
		t1 = 0.75f * b * b - r * r - 2.0f * c;
		t2 = ( 4.0f * b * c - 8.0f * d - b * b * b ) / ( 4.0f * r );
		tp = t1 + t2;
		tm = t1 - t2;

		if( tp >= 0.0f )
		{
			s1 = idMath::Sqrt( tp );
			roots[count++] = -0.25f * b + 0.5f * ( r + s1 );
			roots[count++] = -0.25f * b + 0.5f * ( r - s1 );
		}
		if( tm >= 0.0f )
		{
			s2 = idMath::Sqrt( tm );
			roots[count++] = -0.25f * b + 0.5f * ( s2 - r );
			roots[count++] = -0.25f * b - 0.5f * ( s2 + r );
		}
		return count;
	}
	else
	{
		t2 = y * y - 4.0f * e;
		if( t2 >= 0.0f )
		{
			t2 = 2.0f * idMath::Sqrt( t2 );
			t1 = 0.75f * b * b - 2.0f * c;
			if( t1 + t2 >= 0.0f )
			{
				s1 = idMath::Sqrt( t1 + t2 );
				roots[count++] = -0.25f * b + 0.5f * s1;
				roots[count++] = -0.25f * b - 0.5f * s1;
			}
			if( t1 - t2 >= 0.0f )
			{
				s2 = idMath::Sqrt( t1 - t2 );
				roots[count++] = -0.25f * b + 0.5f * s2;
				roots[count++] = -0.25f * b - 0.5f * s2;
			}
		}
		return count;
	}
}

ID_INLINE const float* idPolynomial::ToFloatPtr() const
{
	return coefficient;
}

ID_INLINE float* idPolynomial::ToFloatPtr()
{
	return coefficient;
}

ID_INLINE void idPolynomial::Resize( int d, bool keep )
{
	int alloc = ( d + 1 + 3 ) & ~3;
	if( alloc > allocated )
	{
		float* ptr = ( float* ) Mem_Alloc16( alloc * sizeof( float ), TAG_MATH );
		if( coefficient != NULL )
		{
			if( keep )
			{
				for( int i = 0; i <= degree; i++ )
				{
					ptr[i] = coefficient[i];
				}
			}
			Mem_Free16( coefficient );
		}
		allocated = alloc;
		coefficient = ptr;
	}
	degree = d;
}

#endif /* !__MATH_POLYNOMIAL_H__ */
