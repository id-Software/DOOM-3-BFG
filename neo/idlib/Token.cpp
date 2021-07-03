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
================
idToken::NumberValue
================
*/
void idToken::NumberValue()
{
	int i, pow, div, c;
	const char* p;
	double m;

	assert( type == TT_NUMBER );
	p = c_str();
	floatvalue = 0;
	intvalue = 0;
	// floating point number
	if( subtype & TT_FLOAT )
	{
		if( subtype & ( TT_INFINITE | TT_INDEFINITE | TT_NAN ) )
		{
			if( subtype & TT_INFINITE )  			// 1.#INF
			{
				unsigned int inf = 0x7f800000;
				floatvalue = ( double ) * ( float* )&inf;
			}
			else if( subtype & TT_INDEFINITE )  	// 1.#IND
			{
				unsigned int ind = 0xffc00000;
				floatvalue = ( double ) * ( float* )&ind;
			}
			else if( subtype & TT_NAN )  			// 1.#QNAN
			{
				unsigned int nan = 0x7fc00000;
				floatvalue = ( double ) * ( float* )&nan;
			}
		}
		else
		{
			while( *p && *p != '.' && *p != 'e' )
			{
				floatvalue = floatvalue * 10.0 + ( double )( *p - '0' );
				p++;
			}
			if( *p == '.' )
			{
				p++;
				for( m = 0.1; *p && *p != 'e'; p++ )
				{
					floatvalue = floatvalue + ( double )( *p - '0' ) * m;
					m *= 0.1;
				}
			}
			if( *p == 'e' )
			{
				p++;
				if( *p == '-' )
				{
					div = true;
					p++;
				}
				else if( *p == '+' )
				{
					div = false;
					p++;
				}
				else
				{
					div = false;
				}
				pow = 0;
				for( pow = 0; *p; p++ )
				{
					pow = pow * 10 + ( int )( *p - '0' );
				}
				for( m = 1.0, i = 0; i < pow; i++ )
				{
					m *= 10.0;
				}
				if( div )
				{
					floatvalue /= m;
				}
				else
				{
					floatvalue *= m;
				}
			}
		}
		intvalue = idMath::Ftoi( floatvalue );
	}
	else if( subtype & TT_DECIMAL )
	{
		while( *p )
		{
			intvalue = intvalue * 10 + ( *p - '0' );
			p++;
		}
		floatvalue = intvalue;
	}
	else if( subtype & TT_IPADDRESS )
	{
		c = 0;
		while( *p && *p != ':' )
		{
			if( *p == '.' )
			{
				while( c != 3 )
				{
					intvalue = intvalue * 10;
					c++;
				}
				c = 0;
			}
			else
			{
				intvalue = intvalue * 10 + ( *p - '0' );
				c++;
			}
			p++;
		}
		while( c != 3 )
		{
			intvalue = intvalue * 10;
			c++;
		}
		floatvalue = intvalue;
	}
	else if( subtype & TT_OCTAL )
	{
		// step over the first zero
		p += 1;
		while( *p )
		{
			intvalue = ( intvalue << 3 ) + ( *p - '0' );
			p++;
		}
		floatvalue = intvalue;
	}
	else if( subtype & TT_HEX )
	{
		// step over the leading 0x or 0X
		p += 2;
		while( *p )
		{
			intvalue <<= 4;
			if( *p >= 'a' && *p <= 'f' )
			{
				intvalue += *p - 'a' + 10;
			}
			else if( *p >= 'A' && *p <= 'F' )
			{
				intvalue += *p - 'A' + 10;
			}
			else
			{
				intvalue += *p - '0';
			}
			p++;
		}
		floatvalue = intvalue;
	}
	else if( subtype & TT_BINARY )
	{
		// step over the leading 0b or 0B
		p += 2;
		while( *p )
		{
			intvalue = ( intvalue << 1 ) + ( *p - '0' );
			p++;
		}
		floatvalue = intvalue;
	}
	subtype |= TT_VALUESVALID;
}

/*
================
idToken::ClearTokenWhiteSpace
================
*/
void idToken::ClearTokenWhiteSpace()
{
	whiteSpaceStart_p = NULL;
	whiteSpaceEnd_p = NULL;
	linesCrossed = 0;
}
