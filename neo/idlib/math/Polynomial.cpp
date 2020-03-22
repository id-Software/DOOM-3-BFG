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

const float EPSILON		= 1e-6f;

/*
=============
idPolynomial::Laguer
=============
*/
int idPolynomial::Laguer( const idComplex* coef, const int degree, idComplex& x ) const
{
	const int MT = 10, MAX_ITERATIONS = MT * 8;
	static const float frac[] = { 0.0f, 0.5f, 0.25f, 0.75f, 0.13f, 0.38f, 0.62f, 0.88f, 1.0f };
	int i, j;
	float abx, abp, abm, err;
	idComplex dx, cx, b, d, f, g, s, gps, gms, g2;

	for( i = 1; i <= MAX_ITERATIONS; i++ )
	{
		b = coef[degree];
		err = b.Abs();
		d.Zero();
		f.Zero();
		abx = x.Abs();
		for( j = degree - 1; j >= 0; j-- )
		{
			f = x * f + d;
			d = x * d + b;
			b = x * b + coef[j];
			err = b.Abs() + abx * err;
		}
		if( b.Abs() < err * EPSILON )
		{
			return i;
		}
		g = d / b;
		g2 = g * g;
		s = ( ( degree - 1 ) * ( degree * ( g2 - 2.0f * f / b ) - g2 ) ).Sqrt();
		gps = g + s;
		gms = g - s;
		abp = gps.Abs();
		abm = gms.Abs();
		if( abp < abm )
		{
			gps = gms;
		}
		if( Max( abp, abm ) > 0.0f )
		{
			dx = degree / gps;
		}
		else
		{
			dx = idMath::Exp( idMath::Log( 1.0f + abx ) ) * idComplex( idMath::Cos( i ), idMath::Sin( i ) );
		}
		cx = x - dx;
		if( x == cx )
		{
			return i;
		}
		if( i % MT == 0 )
		{
			x = cx;
		}
		else
		{
			x -= frac[i / MT] * dx;
		}
	}
	return i;
}

/*
=============
idPolynomial::GetRoots
=============
*/
int idPolynomial::GetRoots( idComplex* roots ) const
{
	int i, j;
	idComplex x, b, c, *coef;

	coef = ( idComplex* ) _alloca16( ( degree + 1 ) * sizeof( idComplex ) );
	for( i = 0; i <= degree; i++ )
	{
		coef[i].Set( coefficient[i], 0.0f );
	}

	for( i = degree - 1; i >= 0; i-- )
	{
		x.Zero();
		Laguer( coef, i + 1, x );
		if( idMath::Fabs( x.i ) < 2.0f * EPSILON * idMath::Fabs( x.r ) )
		{
			x.i = 0.0f;
		}
		roots[i] = x;
		b = coef[i + 1];
		for( j = i; j >= 0; j-- )
		{
			c = coef[j];
			coef[j] = b;
			b = x * b + c;
		}
	}

	for( i = 0; i <= degree; i++ )
	{
		coef[i].Set( coefficient[i], 0.0f );
	}
	for( i = 0; i < degree; i++ )
	{
		Laguer( coef, degree, roots[i] );
	}

	for( i = 1; i < degree; i++ )
	{
		x = roots[i];
		for( j = i - 1; j >= 0; j-- )
		{
			if( roots[j].r <= x.r )
			{
				break;
			}
			roots[j + 1] = roots[j];
		}
		roots[j + 1] = x;
	}

	return degree;
}

/*
=============
idPolynomial::GetRoots
=============
*/
int idPolynomial::GetRoots( float* roots ) const
{
	int i, num;
	idComplex* complexRoots;

	switch( degree )
	{
		case 0:
			return 0;
		case 1:
			return GetRoots1( coefficient[1], coefficient[0], roots );
		case 2:
			return GetRoots2( coefficient[2], coefficient[1], coefficient[0], roots );
		case 3:
			return GetRoots3( coefficient[3], coefficient[2], coefficient[1], coefficient[0], roots );
		case 4:
			return GetRoots4( coefficient[4], coefficient[3], coefficient[2], coefficient[1], coefficient[0], roots );
	}

	// The Abel-Ruffini theorem states that there is no general solution
	// in radicals to polynomial equations of degree five or higher.
	// A polynomial equation can be solved by radicals if and only if
	// its Galois group is a solvable group.

	complexRoots = ( idComplex* ) _alloca16( degree * sizeof( idComplex ) );

	GetRoots( complexRoots );

	for( num = i = 0; i < degree; i++ )
	{
		if( complexRoots[i].i == 0.0f )
		{
			roots[i] = complexRoots[i].r;
			num++;
		}
	}
	return num;
}

/*
=============
idPolynomial::ToString
=============
*/
const char* idPolynomial::ToString( int precision ) const
{
	return idStr::FloatArrayToString( ToFloatPtr(), GetDimension(), precision );
}

/*
=============
idPolynomial::Test
=============
*/
void idPolynomial::Test()
{
	int i, num;
	float roots[4], value;
	idComplex complexRoots[4], complexValue;
	idPolynomial p;

	p = idPolynomial( -5.0f, 4.0f );
	num = p.GetRoots( roots );
	for( i = 0; i < num; i++ )
	{
		value = p.GetValue( roots[i] );
		assert( idMath::Fabs( value ) < 1e-4f );
	}

	p = idPolynomial( -5.0f, 4.0f, 3.0f );
	num = p.GetRoots( roots );
	for( i = 0; i < num; i++ )
	{
		value = p.GetValue( roots[i] );
		assert( idMath::Fabs( value ) < 1e-4f );
	}

	p = idPolynomial( 1.0f, 4.0f, 3.0f, -2.0f );
	num = p.GetRoots( roots );
	for( i = 0; i < num; i++ )
	{
		value = p.GetValue( roots[i] );
		assert( idMath::Fabs( value ) < 1e-4f );
	}

	p = idPolynomial( 5.0f, 4.0f, 3.0f, -2.0f );
	num = p.GetRoots( roots );
	for( i = 0; i < num; i++ )
	{
		value = p.GetValue( roots[i] );
		assert( idMath::Fabs( value ) < 1e-4f );
	}

	p = idPolynomial( -5.0f, 4.0f, 3.0f, 2.0f, 1.0f );
	num = p.GetRoots( roots );
	for( i = 0; i < num; i++ )
	{
		value = p.GetValue( roots[i] );
		assert( idMath::Fabs( value ) < 1e-4f );
	}

	p = idPolynomial( 1.0f, 4.0f, 3.0f, -2.0f );
	num = p.GetRoots( complexRoots );
	for( i = 0; i < num; i++ )
	{
		complexValue = p.GetValue( complexRoots[i] );
		assert( idMath::Fabs( complexValue.r ) < 1e-4f && idMath::Fabs( complexValue.i ) < 1e-4f );
	}

	p = idPolynomial( 5.0f, 4.0f, 3.0f, -2.0f );
	num = p.GetRoots( complexRoots );
	for( i = 0; i < num; i++ )
	{
		complexValue = p.GetValue( complexRoots[i] );
		assert( idMath::Fabs( complexValue.r ) < 1e-4f && idMath::Fabs( complexValue.i ) < 1e-4f );
	}
}
