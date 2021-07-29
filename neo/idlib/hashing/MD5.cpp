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
================================================================================================
Contains the MD5BlockChecksum implementation.
================================================================================================
*/

// POINTER defines a generic pointer type
typedef unsigned char* POINTER;

// UINT2 defines a two byte word
typedef unsigned short int UINT2;

// UINT4 defines a four byte word
typedef unsigned int UINT4;

//------------------------
// The four core functions - F1 is optimized somewhat
// JDC: I wouldn't have condoned the change in something as sensitive as a hash function,
// but it looks ok and a random test function checked it out.
//------------------------
// #define F1(x, y, z) (x & y | ~x & z)
#define F1(x, y, z) (z ^ (x & (y ^ z)))
#define F2(x, y, z) F1(z, x, y)
#define F3(x, y, z) (x ^ y ^ z)
#define F4(x, y, z) (y ^ (x | ~z))

// This is the central step in the MD5 algorithm.
#define MD5STEP(f, w, x, y, z, data, s) ( w += f(x, y, z) + (data),  w = w<<s | w>>(32-s),  w += x )

static unsigned char PADDING[64] =
{
	0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/*
========================
Encode

Encodes input (UINT4) into output (unsigned char). Assumes len is a multiple of 4.
========================
*/
static void Encode( unsigned char* output, UINT4* input, unsigned int len )
{
	unsigned int i, j;

	for( i = 0, j = 0; j < len; i++, j += 4 )
	{
		output[j] = ( unsigned char )( input[i] & 0xff );
		output[j + 1] = ( unsigned char )( ( input[i] >> 8 ) & 0xff );
		output[j + 2] = ( unsigned char )( ( input[i] >> 16 ) & 0xff );
		output[j + 3] = ( unsigned char )( ( input[i] >> 24 ) & 0xff );
	}
}

/*
========================
Decode

Decodes input (unsigned char) into output (UINT4). Assumes len is a multiple of 4.
========================
*/
static void Decode( UINT4* output, const unsigned char* input, unsigned int len )
{
	unsigned int i, j;

	for( i = 0, j = 0; j < len; i++, j += 4 )
	{
		output[i] = ( ( UINT4 )input[j] ) | ( ( ( UINT4 )input[j + 1] ) << 8 ) | ( ( ( UINT4 )input[j + 2] ) << 16 ) | ( ( ( UINT4 )input[j + 3] ) << 24 );
	}
}

/*
========================
MD5_Transform

The core of the MD5 algorithm, this alters an existing MD5 hash to reflect the addition of 16
longwords of new data. MD5Update blocks the data and converts bytes into longwords for this
routine.
========================
*/
void MD5_Transform( unsigned int state[4], const unsigned char block[64] )
{
	unsigned int a, b, c, d, x[16];

	a = state[0];
	b = state[1];
	c = state[2];
	d = state[3];

	Decode( x, block, 64 );

	MD5STEP( F1, a, b, c, d, x[ 0] + 0xd76aa478,  7 );
	MD5STEP( F1, d, a, b, c, x[ 1] + 0xe8c7b756, 12 );
	MD5STEP( F1, c, d, a, b, x[ 2] + 0x242070db, 17 );
	MD5STEP( F1, b, c, d, a, x[ 3] + 0xc1bdceee, 22 );
	MD5STEP( F1, a, b, c, d, x[ 4] + 0xf57c0faf,  7 );
	MD5STEP( F1, d, a, b, c, x[ 5] + 0x4787c62a, 12 );
	MD5STEP( F1, c, d, a, b, x[ 6] + 0xa8304613, 17 );
	MD5STEP( F1, b, c, d, a, x[ 7] + 0xfd469501, 22 );
	MD5STEP( F1, a, b, c, d, x[ 8] + 0x698098d8,  7 );
	MD5STEP( F1, d, a, b, c, x[ 9] + 0x8b44f7af, 12 );
	MD5STEP( F1, c, d, a, b, x[10] + 0xffff5bb1, 17 );
	MD5STEP( F1, b, c, d, a, x[11] + 0x895cd7be, 22 );
	MD5STEP( F1, a, b, c, d, x[12] + 0x6b901122,  7 );
	MD5STEP( F1, d, a, b, c, x[13] + 0xfd987193, 12 );
	MD5STEP( F1, c, d, a, b, x[14] + 0xa679438e, 17 );
	MD5STEP( F1, b, c, d, a, x[15] + 0x49b40821, 22 );

	MD5STEP( F2, a, b, c, d, x[ 1] + 0xf61e2562,  5 );
	MD5STEP( F2, d, a, b, c, x[ 6] + 0xc040b340,  9 );
	MD5STEP( F2, c, d, a, b, x[11] + 0x265e5a51, 14 );
	MD5STEP( F2, b, c, d, a, x[ 0] + 0xe9b6c7aa, 20 );
	MD5STEP( F2, a, b, c, d, x[ 5] + 0xd62f105d,  5 );
	MD5STEP( F2, d, a, b, c, x[10] + 0x02441453,  9 );
	MD5STEP( F2, c, d, a, b, x[15] + 0xd8a1e681, 14 );
	MD5STEP( F2, b, c, d, a, x[ 4] + 0xe7d3fbc8, 20 );
	MD5STEP( F2, a, b, c, d, x[ 9] + 0x21e1cde6,  5 );
	MD5STEP( F2, d, a, b, c, x[14] + 0xc33707d6,  9 );
	MD5STEP( F2, c, d, a, b, x[ 3] + 0xf4d50d87, 14 );
	MD5STEP( F2, b, c, d, a, x[ 8] + 0x455a14ed, 20 );
	MD5STEP( F2, a, b, c, d, x[13] + 0xa9e3e905,  5 );
	MD5STEP( F2, d, a, b, c, x[ 2] + 0xfcefa3f8,  9 );
	MD5STEP( F2, c, d, a, b, x[ 7] + 0x676f02d9, 14 );
	MD5STEP( F2, b, c, d, a, x[12] + 0x8d2a4c8a, 20 );

	MD5STEP( F3, a, b, c, d, x[ 5] + 0xfffa3942,  4 );
	MD5STEP( F3, d, a, b, c, x[ 8] + 0x8771f681, 11 );
	MD5STEP( F3, c, d, a, b, x[11] + 0x6d9d6122, 16 );
	MD5STEP( F3, b, c, d, a, x[14] + 0xfde5380c, 23 );
	MD5STEP( F3, a, b, c, d, x[ 1] + 0xa4beea44,  4 );
	MD5STEP( F3, d, a, b, c, x[ 4] + 0x4bdecfa9, 11 );
	MD5STEP( F3, c, d, a, b, x[ 7] + 0xf6bb4b60, 16 );
	MD5STEP( F3, b, c, d, a, x[10] + 0xbebfbc70, 23 );
	MD5STEP( F3, a, b, c, d, x[13] + 0x289b7ec6,  4 );
	MD5STEP( F3, d, a, b, c, x[ 0] + 0xeaa127fa, 11 );
	MD5STEP( F3, c, d, a, b, x[ 3] + 0xd4ef3085, 16 );
	MD5STEP( F3, b, c, d, a, x[ 6] + 0x04881d05, 23 );
	MD5STEP( F3, a, b, c, d, x[ 9] + 0xd9d4d039,  4 );
	MD5STEP( F3, d, a, b, c, x[12] + 0xe6db99e5, 11 );
	MD5STEP( F3, c, d, a, b, x[15] + 0x1fa27cf8, 16 );
	MD5STEP( F3, b, c, d, a, x[ 2] + 0xc4ac5665, 23 );

	MD5STEP( F4, a, b, c, d, x[ 0] + 0xf4292244,  6 );
	MD5STEP( F4, d, a, b, c, x[ 7] + 0x432aff97, 10 );
	MD5STEP( F4, c, d, a, b, x[14] + 0xab9423a7, 15 );
	MD5STEP( F4, b, c, d, a, x[ 5] + 0xfc93a039, 21 );
	MD5STEP( F4, a, b, c, d, x[12] + 0x655b59c3,  6 );
	MD5STEP( F4, d, a, b, c, x[ 3] + 0x8f0ccc92, 10 );
	MD5STEP( F4, c, d, a, b, x[10] + 0xffeff47d, 15 );
	MD5STEP( F4, b, c, d, a, x[ 1] + 0x85845dd1, 21 );
	MD5STEP( F4, a, b, c, d, x[ 8] + 0x6fa87e4f,  6 );
	MD5STEP( F4, d, a, b, c, x[15] + 0xfe2ce6e0, 10 );
	MD5STEP( F4, c, d, a, b, x[ 6] + 0xa3014314, 15 );
	MD5STEP( F4, b, c, d, a, x[13] + 0x4e0811a1, 21 );
	MD5STEP( F4, a, b, c, d, x[ 4] + 0xf7537e82,  6 );
	MD5STEP( F4, d, a, b, c, x[11] + 0xbd3af235, 10 );
	MD5STEP( F4, c, d, a, b, x[ 2] + 0x2ad7d2bb, 15 );
	MD5STEP( F4, b, c, d, a, x[ 9] + 0xeb86d391, 21 );

	state[0] += a;
	state[1] += b;
	state[2] += c;
	state[3] += d;

	// Zeroize sensitive information.
	memset( ( POINTER )x, 0, sizeof( x ) );
}

/*
========================
MD5_Init

MD5 initialization. Begins an MD5 operation, writing a new context.
========================
*/
void MD5_Init( MD5_CTX* ctx )
{
	ctx->state[0] = 0x67452301;
	ctx->state[1] = 0xefcdab89;
	ctx->state[2] = 0x98badcfe;
	ctx->state[3] = 0x10325476;

	ctx->bits[0] = 0;
	ctx->bits[1] = 0;
}

/*
========================
MD5_Update

MD5 block update operation. Continues an MD5 message-digest operation, processing another
message block, and updating the context.
========================
*/
void MD5_Update( MD5_CTX* context, unsigned char const* input, size_t inputLen )
{
	unsigned int i, index, partLen;

	// Compute number of bytes mod 64
	index = ( unsigned int )( ( context->bits[0] >> 3 ) & 0x3F );

	// Update number of bits
	if( ( context->bits[0] += ( ( UINT4 )inputLen << 3 ) ) < ( ( UINT4 )inputLen << 3 ) )
	{
		context->bits[1]++;
	}

	context->bits[1] += ( ( UINT4 )inputLen >> 29 );

	partLen = 64 - index;

	// Transform as many times as possible.
	if( inputLen >= partLen )
	{
		memcpy( ( POINTER )&context->in[index], ( POINTER )input, partLen );
		MD5_Transform( context->state, context->in );

		for( i = partLen; i + 63 < inputLen; i += 64 )
		{
			MD5_Transform( context->state, &input[i] );
		}

		index = 0;
	}
	else
	{
		i = 0;
	}

	// Buffer remaining input
	memcpy( ( POINTER )&context->in[index], ( POINTER )&input[i], inputLen - i );
}

/*
========================
MD5_Final

MD5 finalization. Ends an MD5 message-digest operation, writing the message digest and
zero-izing the context.
========================
*/
void MD5_Final( MD5_CTX* context, unsigned char digest[16] )
{
	unsigned char bits[8];
	unsigned int index, padLen;

	// Save number of bits
	Encode( bits, context->bits, 8 );

	// Pad out to 56 mod 64.
	index = ( unsigned int )( ( context->bits[0] >> 3 ) & 0x3f );
	padLen = ( index < 56 ) ? ( 56 - index ) : ( 120 - index );
	MD5_Update( context, PADDING, padLen );

	// Append length (before padding)
	MD5_Update( context, bits, 8 );

	// Store state in digest
	Encode( digest, context->state, 16 );

	// Zeroize sensitive information.
	memset( ( POINTER )context, 0, sizeof( *context ) );
}

/*
========================
MD5_BlockChecksum
========================
*/

unsigned int MD5_BlockChecksum( const void* data, size_t length )
{
	unsigned char	digest[16];
	unsigned int	val;
	MD5_CTX			ctx;

	MD5_Init( &ctx );
	MD5_Update( &ctx, ( unsigned char* )data, length );
	MD5_Final( &ctx, ( unsigned char* )digest );

	// Handle it manually to be endian-safe since we don't have access to idSwap.
	val =	( digest[3] << 24 | digest[2] << 16 | digest[1] << 8 | digest[0] ) ^
			( digest[7] << 24 | digest[6] << 16 | digest[5] << 8 | digest[4] ) ^
			( digest[11] << 24 | digest[10] << 16 | digest[9] << 8 | digest[8] ) ^
			( digest[15] << 24 | digest[14] << 16 | digest[13] << 8 | digest[12] );

	return val;
}
