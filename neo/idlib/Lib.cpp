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
===============================================================================

	idLib

===============================================================================
*/

idSys* 			idLib::sys			= NULL;
idCommon* 		idLib::common		= NULL;
idCVarSystem* 	idLib::cvarSystem	= NULL;
idFileSystem* 	idLib::fileSystem	= NULL;
int				idLib::frameNumber	= 0;
bool			idLib::mainThreadInitialized = 0;
ID_TLS			idLib::isMainThread = 0;

char idException::error[2048];

/*
================
idLib::Init
================
*/
void idLib::Init()
{

	assert( sizeof( bool ) == 1 );

	isMainThread = 1;
	mainThreadInitialized = 1;	// note that the thread-local isMainThread is now valid

	// initialize little/big endian conversion
	Swap_Init();

	// init string memory allocator
	idStr::InitMemory();

	// initialize generic SIMD implementation
	idSIMD::Init();

	// initialize math
	idMath::Init();

	// test idMatX
	//idMatX::Test();

	// test idPolynomial
#ifdef _DEBUG
	idPolynomial::Test();
#endif

	// initialize the dictionary string pools
	idDict::Init();
}

/*
================
idLib::ShutDown
================
*/
void idLib::ShutDown()
{

	// shut down the dictionary string pools
	idDict::Shutdown();

	// shut down the string memory allocator
	idStr::ShutdownMemory();

	// shut down the SIMD engine
	idSIMD::Shutdown();
}


/*
===============================================================================

	Colors

===============================================================================
*/

idVec4	colorBlack	= idVec4( 0.00f, 0.00f, 0.00f, 1.00f );
idVec4	colorWhite	= idVec4( 1.00f, 1.00f, 1.00f, 1.00f );
idVec4	colorRed	= idVec4( 1.00f, 0.00f, 0.00f, 1.00f );
idVec4	colorGreen	= idVec4( 0.00f, 1.00f, 0.00f, 1.00f );
idVec4	colorBlue	= idVec4( 0.00f, 0.00f, 1.00f, 1.00f );
idVec4	colorYellow	= idVec4( 1.00f, 1.00f, 0.00f, 1.00f );
idVec4	colorMagenta = idVec4( 1.00f, 0.00f, 1.00f, 1.00f );
idVec4	colorCyan	= idVec4( 0.00f, 1.00f, 1.00f, 1.00f );
idVec4	colorOrange	= idVec4( 1.00f, 0.50f, 0.00f, 1.00f );
idVec4	colorPurple	= idVec4( 0.60f, 0.00f, 0.60f, 1.00f );
idVec4	colorPink	= idVec4( 0.73f, 0.40f, 0.48f, 1.00f );
idVec4	colorBrown	= idVec4( 0.40f, 0.35f, 0.08f, 1.00f );
idVec4	colorLtGrey	= idVec4( 0.75f, 0.75f, 0.75f, 1.00f );
idVec4	colorMdGrey	= idVec4( 0.50f, 0.50f, 0.50f, 1.00f );
idVec4	colorDkGrey	= idVec4( 0.25f, 0.25f, 0.25f, 1.00f );
// jmarshall
idVec4  colorGold	= idVec4( 0.68f, 0.63f, 0.36f, 1.00f );
// jmarshall end

/*
================
PackColor
================
*/
dword PackColor( const idVec4& color )
{
	byte dx = idMath::Ftob( color.x * 255.0f );
	byte dy = idMath::Ftob( color.y * 255.0f );
	byte dz = idMath::Ftob( color.z * 255.0f );
	byte dw = idMath::Ftob( color.w * 255.0f );
	return ( dx << 0 ) | ( dy << 8 ) | ( dz << 16 ) | ( dw << 24 );
}

/*
================
UnpackColor
================
*/
void UnpackColor( const dword color, idVec4& unpackedColor )
{
	unpackedColor.Set( ( ( color >> 0 ) & 255 ) * ( 1.0f / 255.0f ),
					   ( ( color >> 8 ) & 255 ) * ( 1.0f / 255.0f ),
					   ( ( color >> 16 ) & 255 ) * ( 1.0f / 255.0f ),
					   ( ( color >> 24 ) & 255 ) * ( 1.0f / 255.0f ) );
}

/*
================
PackColor
================
*/
dword PackColor( const idVec3& color )
{
	byte dx = idMath::Ftob( color.x * 255.0f );
	byte dy = idMath::Ftob( color.y * 255.0f );
	byte dz = idMath::Ftob( color.z * 255.0f );
	return ( dx << 0 ) | ( dy << 8 ) | ( dz << 16 );
}

/*
================
UnpackColor
================
*/
void UnpackColor( const dword color, idVec3& unpackedColor )
{
	unpackedColor.Set( ( ( color >> 0 ) & 255 ) * ( 1.0f / 255.0f ),
					   ( ( color >> 8 ) & 255 ) * ( 1.0f / 255.0f ),
					   ( ( color >> 16 ) & 255 ) * ( 1.0f / 255.0f ) );
}

/*
===============
idLib::FatalError
===============
*/
void idLib::FatalError( const char* fmt, ... )
{
	va_list		argptr;
	char		text[MAX_STRING_CHARS];

	va_start( argptr, fmt );
	idStr::vsnPrintf( text, sizeof( text ), fmt, argptr );
	va_end( argptr );

	common->FatalError( "%s", text );

#if !defined(_WIN32)
	// SRS - Added exit to silence build warning since FatalError has attribute noreturn
	exit( EXIT_FAILURE );
#endif
}

/*
===============
idLib::Error
===============
*/
void idLib::Error( const char* fmt, ... )
{
	va_list		argptr;
	char		text[MAX_STRING_CHARS];

	va_start( argptr, fmt );
	idStr::vsnPrintf( text, sizeof( text ), fmt, argptr );
	va_end( argptr );

	common->Error( "%s", text );

#if !defined(_WIN32)
	// SRS - Added exit to silence build warning since FatalError has attribute noreturn
	exit( EXIT_FAILURE );
#endif
}

/*
===============
idLib::Warning
===============
*/
void idLib::Warning( const char* fmt, ... )
{
	va_list		argptr;
	char		text[MAX_STRING_CHARS];

	va_start( argptr, fmt );
	idStr::vsnPrintf( text, sizeof( text ), fmt, argptr );
	va_end( argptr );

	common->Warning( "%s", text );
}

/*
===============
idLib::WarningIf
===============
*/
void idLib::WarningIf( const bool test, const char* fmt, ... )
{
	if( !test )
	{
		return;
	}

	va_list		argptr;
	char		text[MAX_STRING_CHARS];

	va_start( argptr, fmt );
	idStr::vsnPrintf( text, sizeof( text ), fmt, argptr );
	va_end( argptr );

	common->Warning( "%s", text );
}

/*
===============
idLib::Printf
===============
*/
void idLib::Printf( const char* fmt, ... )
{
	va_list		argptr;
	va_start( argptr, fmt );
	if( common )
	{
		common->VPrintf( fmt, argptr );
	}
	va_end( argptr );
}

/*
===============
idLib::PrintfIf
===============
*/
void idLib::PrintfIf( const bool test, const char* fmt, ... )
{
	if( !test )
	{
		return;
	}

	va_list		argptr;
	va_start( argptr, fmt );
	common->VPrintf( fmt, argptr );
	va_end( argptr );
}

/*
===============================================================================

	Byte order functions

===============================================================================
*/

// can't just use function pointers, or dll linkage can mess up
static short( *_BigShort )( short l );
static short( *_LittleShort )( short l );
static int	( *_BigLong )( int l );
static int	( *_LittleLong )( int l );
static float( *_BigFloat )( float l );
static float( *_LittleFloat )( float l );
static void	( *_BigRevBytes )( void* bp, int elsize, int elcount );
static void	( *_LittleRevBytes )( void* bp, int elsize, int elcount );
static void ( *_LittleBitField )( void* bp, int elsize );
static void	( *_SixtetsForInt )( byte* out, int src );
static int	( *_IntForSixtets )( byte* in );

short	BigShort( short l )
{
	return _BigShort( l );
}
short	LittleShort( short l )
{
	return _LittleShort( l );
}
int		BigLong( int l )
{
	return _BigLong( l );
}
int		LittleLong( int l )
{
	return _LittleLong( l );
}
float	BigFloat( float l )
{
	return _BigFloat( l );
}
float	LittleFloat( float l )
{
	return _LittleFloat( l );
}
void	BigRevBytes( void* bp, int elsize, int elcount )
{
	_BigRevBytes( bp, elsize, elcount );
}
void	LittleRevBytes( void* bp, int elsize, int elcount )
{
	_LittleRevBytes( bp, elsize, elcount );
}
void	LittleBitField( void* bp, int elsize )
{
	_LittleBitField( bp, elsize );
}

void	SixtetsForInt( byte* out, int src )
{
	_SixtetsForInt( out, src );
}
int		IntForSixtets( byte* in )
{
	return _IntForSixtets( in );
}

/*
================
ShortSwap
================
*/
short ShortSwap( short l )
{
	byte    b1, b2;

	b1 = l & 255;
	b2 = ( l >> 8 ) & 255;

	return ( b1 << 8 ) + b2;
}

/*
================
ShortNoSwap
================
*/
short ShortNoSwap( short l )
{
	return l;
}

/*
================
LongSwap
================
*/
int LongSwap( int l )
{
	byte    b1, b2, b3, b4;

	b1 = l & 255;
	b2 = ( l >> 8 ) & 255;
	b3 = ( l >> 16 ) & 255;
	b4 = ( l >> 24 ) & 255;

	return ( ( int )b1 << 24 ) + ( ( int )b2 << 16 ) + ( ( int )b3 << 8 ) + b4;
}

/*
================
LongNoSwap
================
*/
int	LongNoSwap( int l )
{
	return l;
}

/*
================
FloatSwap
================
*/
float FloatSwap( float f )
{
	union
	{
		float	f;
		byte	b[4];
	} dat1, dat2;


	dat1.f = f;
	dat2.b[0] = dat1.b[3];
	dat2.b[1] = dat1.b[2];
	dat2.b[2] = dat1.b[1];
	dat2.b[3] = dat1.b[0];
	return dat2.f;
}

/*
================
FloatNoSwap
================
*/
float FloatNoSwap( float f )
{
	return f;
}

/*
=====================================================================
RevBytesSwap

Reverses byte order in place.

INPUTS
   bp       bytes to reverse
   elsize   size of the underlying data type
   elcount  number of elements to swap

RESULTS
   Reverses the byte order in each of elcount elements.
===================================================================== */
void RevBytesSwap( void* bp, int elsize, int elcount )
{
	unsigned char* p, *q;

	p = ( unsigned char* ) bp;

	if( elsize == 2 )
	{
		q = p + 1;
		while( elcount-- )
		{
			*p ^= *q;
			*q ^= *p;
			*p ^= *q;
			p += 2;
			q += 2;
		}
		return;
	}

	while( elcount-- )
	{
		q = p + elsize - 1;
		while( p < q )
		{
			*p ^= *q;
			*q ^= *p;
			*p ^= *q;
			++p;
			--q;
		}
		p += elsize >> 1;
	}
}

/*
 =====================================================================
 RevBytesSwap

 Reverses byte order in place, then reverses bits in those bytes

 INPUTS
 bp       bitfield structure to reverse
 elsize   size of the underlying data type

 RESULTS
 Reverses the bitfield of size elsize.
 ===================================================================== */
void RevBitFieldSwap( void* bp, int elsize )
{
	int i;
	unsigned char* p, t, v;

	LittleRevBytes( bp, elsize, 1 );

	p = ( unsigned char* ) bp;
	while( elsize-- )
	{
		v = *p;
		t = 0;
		for( i = 7; i >= 0; i-- )
		{
			t <<= 1;
			v >>= 1;
			t |= v & 1;
		}
		*p++ = t;
	}
}

/*
================
RevBytesNoSwap
================
*/
void RevBytesNoSwap( void* bp, int elsize, int elcount )
{
	return;
}

/*
 ================
 RevBytesNoSwap
 ================
 */
void RevBitFieldNoSwap( void* bp, int elsize )
{
	return;
}

/*
================
SixtetsForIntLittle
================
*/
void SixtetsForIntLittle( byte* out, int src )
{
	byte* b = ( byte* )&src;
	out[0] = ( b[0] & 0xfc ) >> 2;
	out[1] = ( ( b[0] & 0x3 ) << 4 ) + ( ( b[1] & 0xf0 ) >> 4 );
	out[2] = ( ( b[1] & 0xf ) << 2 ) + ( ( b[2] & 0xc0 ) >> 6 );
	out[3] = b[2] & 0x3f;
}

/*
================
SixtetsForIntBig
TTimo: untested - that's the version from initial base64 encode
================
*/
void SixtetsForIntBig( byte* out, int src )
{
	for( int i = 0 ; i < 4 ; i++ )
	{
		out[i] = src & 0x3f;
		src >>= 6;
	}
}

/*
================
IntForSixtetsLittle
================
*/
int IntForSixtetsLittle( byte* in )
{
	int ret = 0;
	byte* b = ( byte* )&ret;
	b[0] |= in[0] << 2;
	b[0] |= ( in[1] & 0x30 ) >> 4;
	b[1] |= ( in[1] & 0xf ) << 4;
	b[1] |= ( in[2] & 0x3c ) >> 2;
	b[2] |= ( in[2] & 0x3 ) << 6;
	b[2] |= in[3];
	return ret;
}

/*
================
IntForSixtetsBig
TTimo: untested - that's the version from initial base64 decode
================
*/
int IntForSixtetsBig( byte* in )
{
	int ret = 0;
	ret |= in[0];
	ret |= in[1] << 6;
	ret |= in[2] << 2 * 6;
	ret |= in[3] << 3 * 6;
	return ret;
}

/*
================
Swap_Init
================
*/
void Swap_Init()
{
	byte	swaptest[2] = {1, 0};

	// set the byte swapping variables in a portable manner
	if( *( short* )swaptest == 1 )
	{
		// little endian ex: x86
		_BigShort = ShortSwap;
		_LittleShort = ShortNoSwap;
		_BigLong = LongSwap;
		_LittleLong = LongNoSwap;
		_BigFloat = FloatSwap;
		_LittleFloat = FloatNoSwap;
		_BigRevBytes = RevBytesSwap;
		_LittleRevBytes = RevBytesNoSwap;
		_LittleBitField = RevBitFieldNoSwap;
		_SixtetsForInt = SixtetsForIntLittle;
		_IntForSixtets = IntForSixtetsLittle;
	}
	else
	{
		// big endian ex: ppc
		_BigShort = ShortNoSwap;
		_LittleShort = ShortSwap;
		_BigLong = LongNoSwap;
		_LittleLong = LongSwap;
		_BigFloat = FloatNoSwap;
		_LittleFloat = FloatSwap;
		_BigRevBytes = RevBytesNoSwap;
		_LittleRevBytes = RevBytesSwap;
		_LittleBitField = RevBitFieldSwap;
		_SixtetsForInt = SixtetsForIntBig;
		_IntForSixtets = IntForSixtetsBig;
	}
}

/*
==========
Swap_IsBigEndian
==========
*/
bool Swap_IsBigEndian()
{
	byte	swaptest[2] = {1, 0};
	return *( short* )swaptest != 1;
}


/*
========================
BreakOnListGrowth

debug tool to find uses of idlist that are dynamically growing
========================
*/
void BreakOnListGrowth()
{
}

/*
========================
BreakOnListDefault
========================
*/
void BreakOnListDefault()
{
}
