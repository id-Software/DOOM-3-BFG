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

#ifndef __MATH_RANDOM_H__
#define __MATH_RANDOM_H__

/*
===============================================================================

	Random number generator

===============================================================================
*/

class idRandom
{
public:
	idRandom( int seed = 0 );

	void				SetSeed( int seed );
	int					GetSeed() const;

	int					RandomInt();			// random integer in the range [0, MAX_RAND]
	int					RandomInt( int max );		// random integer in the range [0, max[
	float				RandomFloat();		// random number in the range [0.0f, 1.0f]
	float				CRandomFloat();		// random number in the range [-1.0f, 1.0f]

	static const int	MAX_RAND = 0x7fff;

private:
	int					seed;
};

ID_INLINE idRandom::idRandom( int seed )
{
	this->seed = seed;
}

ID_INLINE void idRandom::SetSeed( int seed )
{
	this->seed = seed;
}

ID_INLINE int idRandom::GetSeed() const
{
	return seed;
}

ID_INLINE int idRandom::RandomInt()
{
	seed = 69069 * seed + 1;
	return ( seed & idRandom::MAX_RAND );
}

ID_INLINE int idRandom::RandomInt( int max )
{
	if( max == 0 )
	{
		return 0;			// avoid divide by zero error
	}
	return RandomInt() % max;
}

ID_INLINE float idRandom::RandomFloat()
{
	return ( RandomInt() / ( float )( idRandom::MAX_RAND + 1 ) );
}

ID_INLINE float idRandom::CRandomFloat()
{
	return ( 2.0f * ( RandomFloat() - 0.5f ) );
}


/*
===============================================================================

	Random number generator

===============================================================================
*/

class idRandom2
{
public:
	// DG: use int instead of long for 64bit compatibility in this whole class
	idRandom2( unsigned int seed = 0 );

	void					SetSeed( unsigned int seed );
	unsigned int			GetSeed() const;

	int						RandomInt();			// random integer in the range [0, MAX_RAND]
	int						RandomInt( int max );		// random integer in the range [0, max]
	float					RandomFloat();		// random number in the range [0.0f, 1.0f]
	float					CRandomFloat();		// random number in the range [-1.0f, 1.0f]

	static const int		MAX_RAND = 0x7fff;

private:
	unsigned int			seed;

	static const unsigned int	IEEE_ONE = 0x3f800000;
	static const unsigned int	IEEE_MASK = 0x007fffff;
};

ID_INLINE idRandom2::idRandom2( unsigned int seed )
{
	this->seed = seed;
}

ID_INLINE void idRandom2::SetSeed( unsigned int seed )
{
	this->seed = seed;
}

ID_INLINE unsigned int idRandom2::GetSeed() const
{
	return seed;
}

ID_INLINE int idRandom2::RandomInt()
{
	seed = 1664525L * seed + 1013904223L;
	return ( ( int ) seed & idRandom2::MAX_RAND );
}

ID_INLINE int idRandom2::RandomInt( int max )
{
	if( max == 0 )
	{
		return 0;		// avoid divide by zero error
	}
	return ( RandomInt() >> ( 16 - idMath::BitsForInteger( max ) ) ) % max;
}

ID_INLINE float idRandom2::RandomFloat()
{
	unsigned int i;
	seed = 1664525L * seed + 1013904223L;
	i = idRandom2::IEEE_ONE | ( seed & idRandom2::IEEE_MASK );
	return ( ( *( float* )&i ) - 1.0f );
}

ID_INLINE float idRandom2::CRandomFloat()
{
	unsigned int i;
	seed = 1664525L * seed + 1013904223L;
	i = idRandom2::IEEE_ONE | ( seed & idRandom2::IEEE_MASK );
	return ( 2.0f * ( *( float* )&i ) - 3.0f );
}
// DG end

#endif /* !__MATH_RANDOM_H__ */
