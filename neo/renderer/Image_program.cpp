/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2021 Stephen Pridham

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

/*

all uncompressed
uncompressed normal maps

downsample images

16 meg Dynamic cache

Anisotropic texturing

Trilinear on all
Trilinear on normal maps, bilinear on others
Bilinear on all


Manager

->List
->Print
->Reload( bool force )

*/

#include "precompiled.h"
#pragma hdrstop


// tr_imageprogram.c

#include "RenderCommon.h"

/*

Anywhere that an image name is used (diffusemaps, bumpmaps, specularmaps, lights, etc),
an imageProgram can be specified.

This allows load time operations, like heightmap-to-normalmap conversion and image
composition, to be automatically handled in a way that supports timestamped reloads.

*/

/*
=================
R_HeightmapToNormalMap

it is not possible to convert a heightmap into a normal map
properly without knowing the texture coordinate stretching.
We can assume constant and equal ST vectors for walls, but not for characters.
=================
*/
static void R_HeightmapToNormalMap( byte* data, int width, int height, float scale )
{
	int		i, j;
	byte*	depth;

	scale = scale / 256;

	// copy and convert to grey scale
	j = width * height;
	depth = ( byte* )R_StaticAlloc( j, TAG_IMAGE );
	for( i = 0 ; i < j ; i++ )
	{
		depth[i] = ( data[i * 4] + data[i * 4 + 1] + data[i * 4 + 2] ) / 3;
	}

	idVec3	dir, dir2;
	for( i = 0 ; i < height ; i++ )
	{
		for( j = 0 ; j < width ; j++ )
		{
			int		d1, d2, d3, d4;
			int		a1, a2, a3, a4;

			// FIXME: look at five points?

			// look at three points to estimate the gradient
			a1 = d1 = depth[( i * width + j ) ];
			a2 = d2 = depth[( i * width + ( ( j + 1 ) & ( width - 1 ) ) ) ];
			a3 = d3 = depth[( ( ( i + 1 ) & ( height - 1 ) ) * width + j ) ];
			a4 = d4 = depth[( ( ( i + 1 ) & ( height - 1 ) ) * width + ( ( j + 1 ) & ( width - 1 ) ) ) ];

			d2 -= d1;
			d3 -= d1;

			dir[0] = -d2 * scale;
			dir[1] = -d3 * scale;
			dir[2] = 1;
			dir.NormalizeFast();

			a1 -= a3;
			a4 -= a3;

			dir2[0] = -a4 * scale;
			dir2[1] = a1 * scale;
			dir2[2] = 1;
			dir2.NormalizeFast();

			dir += dir2;
			dir.NormalizeFast();

			a1 = ( i * width + j ) * 4;
			data[ a1 + 0 ] = ( byte )( dir[0] * 127 + 128 );
			data[ a1 + 1 ] = ( byte )( dir[1] * 127 + 128 );
			data[ a1 + 2 ] = ( byte )( dir[2] * 127 + 128 );
			data[ a1 + 3 ] = 255;
		}
	}


	R_StaticFree( depth );
}


/*
=================
R_ImageScale
=================
*/
static void R_ImageScale( byte* data, int width, int height, float scale[4] )
{
	int		i, j;
	int		c;

	c = width * height * 4;

	for( i = 0 ; i < c ; i++ )
	{
		j = ( byte )( data[i] * scale[i & 3] );
		if( j < 0 )
		{
			j = 0;
		}
		else if( j > 255 )
		{
			j = 255;
		}
		data[i] = j;
	}
}

/*
=================
R_InvertAlpha
=================
*/
static void R_InvertAlpha( byte* data, int width, int height )
{
	int		i;
	int		c;

	c = width * height * 4;

	for( i = 0 ; i < c ; i += 4 )
	{
		data[i + 3] = 255 - data[i + 3];
	}
}

/*
=================
R_InvertGreen
=================
*/
static void R_InvertGreen( byte* data, int width, int height )
{
	int		i;
	int		c;

	c = width * height * 4;

	for( i = 0 ; i < c ; i += 4 )
	{
		data[i + 1] = 255 - data[i + 1];
	}
}

/*
=================
R_InvertColor
=================
*/
static void R_InvertColor( byte* data, int width, int height )
{
	int		i;
	int		c;

	c = width * height * 4;

	for( i = 0 ; i < c ; i += 4 )
	{
		data[i + 0] = 255 - data[i + 0];
		data[i + 1] = 255 - data[i + 1];
		data[i + 2] = 255 - data[i + 2];
	}
}


/*
===================
R_AddNormalMaps

===================
*/
static void R_AddNormalMaps( byte* data1, int width1, int height1, byte* data2, int width2, int height2 )
{
	int		i, j;
	byte*	newMap;

	// resample pic2 to the same size as pic1
	if( width2 != width1 || height2 != height1 )
	{
		newMap = R_Dropsample( data2, width2, height2, width1, height1 );
		data2 = newMap;
	}
	else
	{
		newMap = NULL;
	}

	// add the normal change from the second and renormalize
	for( i = 0 ; i < height1 ; i++ )
	{
		for( j = 0 ; j < width1 ; j++ )
		{
			byte*	d1, *d2;
			idVec3	n;
			float   len;

			d1 = data1 + ( i * width1 + j ) * 4;
			d2 = data2 + ( i * width1 + j ) * 4;

			n[0] = ( d1[0] - 128 ) / 127.0;
			n[1] = ( d1[1] - 128 ) / 127.0;
			n[2] = ( d1[2] - 128 ) / 127.0;

			// There are some normal maps that blend to 0,0,0 at the edges
			// this screws up compression, so we try to correct that here by instead fading it to 0,0,1
			len = n.LengthFast();
			if( len < 1.0f )
			{
				n[2] = idMath::Sqrt( 1.0 - ( n[0] * n[0] ) - ( n[1] * n[1] ) );
			}

			n[0] += ( d2[0] - 128 ) / 127.0;
			n[1] += ( d2[1] - 128 ) / 127.0;
			n.Normalize();

			d1[0] = ( byte )( n[0] * 127 + 128 );
			d1[1] = ( byte )( n[1] * 127 + 128 );
			d1[2] = ( byte )( n[2] * 127 + 128 );
			d1[3] = 255;
		}
	}

	if( newMap )
	{
		R_StaticFree( newMap );
	}
}

/*
================
R_SmoothNormalMap
================
*/
static void R_SmoothNormalMap( byte* data, int width, int height )
{
	byte*	orig;
	int		i, j, k, l;
	idVec3	normal;
	byte*	out;
	static float	factors[3][3] =
	{
		{ 1, 1, 1 },
		{ 1, 1, 1 },
		{ 1, 1, 1 }
	};

	orig = ( byte* )R_StaticAlloc( width * height * 4, TAG_IMAGE );
	memcpy( orig, data, width * height * 4 );

	for( i = 0 ; i < width ; i++ )
	{
		for( j = 0 ; j < height ; j++ )
		{
			normal = vec3_origin;
			for( k = -1 ; k < 2 ; k++ )
			{
				for( l = -1 ; l < 2 ; l++ )
				{
					byte*	in;

					in = orig + ( ( ( j + l ) & ( height - 1 ) ) * width + ( ( i + k ) & ( width - 1 ) ) ) * 4;

					// ignore 000 and -1 -1 -1
					if( in[0] == 0 && in[1] == 0 && in[2] == 0 )
					{
						continue;
					}
					if( in[0] == 128 && in[1] == 128 && in[2] == 128 )
					{
						continue;
					}

					normal[0] += factors[k + 1][l + 1] * ( in[0] - 128 );
					normal[1] += factors[k + 1][l + 1] * ( in[1] - 128 );
					normal[2] += factors[k + 1][l + 1] * ( in[2] - 128 );
				}
			}
			normal.Normalize();
			out = data + ( j * width + i ) * 4;
			out[0] = ( byte )( 128 + 127 * normal[0] );
			out[1] = ( byte )( 128 + 127 * normal[1] );
			out[2] = ( byte )( 128 + 127 * normal[2] );
		}
	}

	R_StaticFree( orig );
}


/*
===================
R_ImageAdd

===================
*/
static void R_ImageAdd( byte* data1, int width1, int height1, byte* data2, int width2, int height2 )
{
	int		i, j;
	int		c;
	byte*	newMap;

	// resample pic2 to the same size as pic1
	if( width2 != width1 || height2 != height1 )
	{
		newMap = R_Dropsample( data2, width2, height2, width1, height1 );
		data2 = newMap;
	}
	else
	{
		newMap = NULL;
	}


	c = width1 * height1 * 4;

	for( i = 0 ; i < c ; i++ )
	{
		j = data1[i] + data2[i];
		if( j > 255 )
		{
			j = 255;
		}
		data1[i] = j;
	}

	if( newMap )
	{
		R_StaticFree( newMap );
	}
}

// SP begin
static void R_CombineRgba( byte* data1, int width1, int height1, byte* data2, int width2, int height2, byte* data3, int width3, int height3 )
{
	assert( width1 == width2 );
	//assert(width2 == width3);
	assert( height1 == height2 );

	for( int j = 0; j < 4 * height1; j += 4 )
	{
		for( int i = 0; i < 4 * width1; i += 4 )
		{
			// Assume that these textures are all grayscale images. just take the r channel of each and set them to
			// the respective rgb.
			byte r = data1[i + j * width1];

			byte g = data2[i + j * width1];

			byte b = 255;

			if( data3 && width1 == width3 )
			{
				b = data3[i + j * width1];
			}

			byte a = 255;

			data1[0 + i + j * width1] = r;
			data1[1 + i + j * width1] = g;
			data1[2 + i + j * width1] = b;
			data1[3 + i + j * width1] = a;
		}
	}

}
// SP end


// we build a canonical token form of the image program here
static char parseBuffer[MAX_IMAGE_NAME];

/*
===================
AppendToken
===================
*/
static void AppendToken( idToken& token )
{
	// add a leading space if not at the beginning
	if( parseBuffer[0] )
	{
		idStr::Append( parseBuffer, MAX_IMAGE_NAME, " " );
	}
	idStr::Append( parseBuffer, MAX_IMAGE_NAME, token.c_str() );
}

/*
===================
MatchAndAppendToken
===================
*/
static void MatchAndAppendToken( idLexer& src, const char* match )
{
	if( !src.ExpectTokenString( match ) )
	{
		return;
	}
	// a matched token won't need a leading space
	idStr::Append( parseBuffer, MAX_IMAGE_NAME, match );
}

/*
===================
R_ParseImageProgram_r

If pic is NULL, the timestamps will be filled in, but no image will be generated
If both pic and timestamps are NULL, it will just advance past it, which can be
used to parse an image program from a text stream.
===================
*/
static bool R_ParseImageProgram_r( idLexer& src, byte** pic, int* width, int* height,
								   ID_TIME_T* timestamps, textureUsage_t* usage )
{
	idToken		token;
	ID_TIME_T	timestamp;

	src.ReadToken( &token );

	// Since all interaction shaders now assume YCoCG diffuse textures.  We replace all entries for the intrinsic
	// _black texture to the black texture on disk.  Doing this will cause a YCoCG compliant texture to be generated.
	// Without a YCoCG compliant black texture we will get color artifacts for any interaction
	// material that specifies the _black texture.
	if( token == "_black" )
	{
		token = "textures\\black";
	}

	// also check for _white
	if( token == "_white" )
	{
		token = "guis\\assets\\white";
	}

	AppendToken( token );

	if( !token.Icmp( "heightmap" ) )
	{
		MatchAndAppendToken( src, "(" );

		if( !R_ParseImageProgram_r( src, pic, width, height, timestamps, usage ) )
		{
			return false;
		}

		MatchAndAppendToken( src, "," );

		src.ReadToken( &token );
		AppendToken( token );
		float scale = token.GetFloatValue();

		// process it
		if( pic )
		{
			R_HeightmapToNormalMap( *pic, *width, *height, scale );
			if( usage )
			{
				*usage = TD_BUMP;
			}
		}

		MatchAndAppendToken( src, ")" );
		return true;
	}

	if( !token.Icmp( "addnormals" ) )
	{
		byte*	pic2 = NULL;
		int		width2, height2;

		MatchAndAppendToken( src, "(" );

		if( !R_ParseImageProgram_r( src, pic, width, height, timestamps, usage ) )
		{
			return false;
		}

		MatchAndAppendToken( src, "," );

		if( !R_ParseImageProgram_r( src, pic ? &pic2 : NULL, &width2, &height2, timestamps, usage ) )
		{
			if( pic )
			{
				R_StaticFree( *pic );
				*pic = NULL;
			}
			return false;
		}

		// process it
		if( pic )
		{
			R_AddNormalMaps( *pic, *width, *height, pic2, width2, height2 );
			R_StaticFree( pic2 );
			if( usage )
			{
				*usage = TD_BUMP;
			}
		}

		MatchAndAppendToken( src, ")" );
		return true;
	}

	if( !token.Icmp( "smoothnormals" ) )
	{
		MatchAndAppendToken( src, "(" );

		if( !R_ParseImageProgram_r( src, pic, width, height, timestamps, usage ) )
		{
			return false;
		}

		if( pic )
		{
			R_SmoothNormalMap( *pic, *width, *height );
			if( usage )
			{
				*usage = TD_BUMP;
			}
		}

		MatchAndAppendToken( src, ")" );
		return true;
	}

	if( !token.Icmp( "add" ) )
	{
		byte*	pic2 = NULL;
		int		width2, height2;

		MatchAndAppendToken( src, "(" );

		if( !R_ParseImageProgram_r( src, pic, width, height, timestamps, usage ) )
		{
			return false;
		}

		MatchAndAppendToken( src, "," );

		if( !R_ParseImageProgram_r( src, pic ? &pic2 : NULL, &width2, &height2, timestamps, usage ) )
		{
			if( pic )
			{
				R_StaticFree( *pic );
				*pic = NULL;
			}
			return false;
		}

		// process it
		if( pic )
		{
			R_ImageAdd( *pic, *width, *height, pic2, width2, height2 );
			R_StaticFree( pic2 );
		}

		MatchAndAppendToken( src, ")" );
		return true;
	}

	if( !token.Icmp( "scale" ) )
	{
		float	scale[4];
		int		i;

		MatchAndAppendToken( src, "(" );

		R_ParseImageProgram_r( src, pic, width, height, timestamps, usage );

		for( i = 0 ; i < 4 ; i++ )
		{
			MatchAndAppendToken( src, "," );
			src.ReadToken( &token );
			AppendToken( token );
			scale[i] = token.GetFloatValue();
		}

		// process it
		if( pic )
		{
			R_ImageScale( *pic, *width, *height, scale );
		}

		MatchAndAppendToken( src, ")" );
		return true;
	}

	if( !token.Icmp( "invertAlpha" ) )
	{
		MatchAndAppendToken( src, "(" );

		R_ParseImageProgram_r( src, pic, width, height, timestamps, usage );

		// process it
		if( pic )
		{
			R_InvertAlpha( *pic, *width, *height );
		}

		MatchAndAppendToken( src, ")" );
		return true;
	}

	// RB: invertGreen to allow flipping the Y-Axis of normal maps
	if( !token.Icmp( "invertGreen" ) )
	{
		MatchAndAppendToken( src, "(" );

		R_ParseImageProgram_r( src, pic, width, height, timestamps, usage );

		// process it
		if( pic )
		{
			R_InvertGreen( *pic, *width, *height );
		}

		MatchAndAppendToken( src, ")" );
		return true;
	}

	if( !token.Icmp( "invertColor" ) )
	{
		MatchAndAppendToken( src, "(" );

		R_ParseImageProgram_r( src, pic, width, height, timestamps, usage );

		// process it
		if( pic )
		{
			R_InvertColor( *pic, *width, *height );
		}

		MatchAndAppendToken( src, ")" );
		return true;
	}

	if( !token.Icmp( "makeIntensity" ) )
	{
		int		i;

		MatchAndAppendToken( src, "(" );

		R_ParseImageProgram_r( src, pic, width, height, timestamps, usage );

		// copy red to green, blue, and alpha
		if( pic )
		{
			int		c;
			c = *width * *height * 4;
			for( i = 0 ; i < c ; i += 4 )
			{
				( *pic )[i + 1] =
					( *pic )[i + 2] =
						( *pic )[i + 3] = ( *pic )[i];
			}
		}

		MatchAndAppendToken( src, ")" );
		return true;
	}

	if( !token.Icmp( "makeAlpha" ) )
	{
		int		i;

		MatchAndAppendToken( src, "(" );

		R_ParseImageProgram_r( src, pic, width, height, timestamps, usage );

		// average RGB into alpha, then set RGB to white
		if( pic )
		{
			int		c;
			c = *width * *height * 4;
			for( i = 0 ; i < c ; i += 4 )
			{
				( *pic )[i + 3] = ( ( *pic )[i + 0] + ( *pic )[i + 1] + ( *pic )[i + 2] ) / 3;
				( *pic )[i + 0] =
					( *pic )[i + 1] =
						( *pic )[i + 2] = 255;
			}
		}

		MatchAndAppendToken( src, ")" );
		return true;
	}

	if( !token.Icmp( "combineRgba" ) )
	{
		byte* pic2 = nullptr;
		byte* pic3 = nullptr;
		int	width2, height2;
		int width3, height3;

		MatchAndAppendToken( src, "(" );

		if( !R_ParseImageProgram_r( src, pic, width, height, timestamps, usage ) )
		{
			return false;
		}

		MatchAndAppendToken( src, "," );

		if( !R_ParseImageProgram_r( src, pic ? &pic2 : NULL, &width2, &height2, timestamps, usage ) )
		{
			if( pic )
			{
				R_StaticFree( *pic );
				*pic = NULL;
			}
			return false;
		}

		MatchAndAppendToken( src, "," );

		if( !R_ParseImageProgram_r( src, pic2 ? &pic3 : NULL, &width3, &height3, timestamps, usage ) )
		{
			if( pic )
			{
				R_StaticFree( *pic );
				*pic = NULL;
			}
			return false;
		}

		// process it
		if( pic )
		{
			R_CombineRgba( *pic, *width, *height, pic2, width2, height2, pic3, width3, height3 );
			R_StaticFree( pic2 );
			R_StaticFree( pic3 );
		}

		MatchAndAppendToken( src, ")" );
		return true;
	}

	// if we are just parsing instead of loading or checking,
	// don't do the R_LoadImage
	if( !timestamps && !pic )
	{
		return true;
	}

	// load it as an image
	R_LoadImage( token.c_str(), pic, width, height, &timestamp, true, usage );

	if( timestamp == -1 )
	{
		return false;
	}

	// add this to the timestamp
	if( timestamps )
	{
		if( timestamp > *timestamps )
		{
			*timestamps = timestamp;
		}
	}

	return true;
}


/*
===================
R_LoadImageProgram
===================
*/
void R_LoadImageProgram( const char* name, byte** pic, int* width, int* height, ID_TIME_T* timestamps, textureUsage_t* usage )
{
	idLexer src;

	src.LoadMemory( name, strlen( name ), name );
	src.SetFlags( LEXFL_NOFATALERRORS | LEXFL_NOSTRINGCONCAT | LEXFL_NOSTRINGESCAPECHARS | LEXFL_ALLOWPATHNAMES );

	parseBuffer[0] = 0;
	if( timestamps )
	{
		*timestamps = 0;
	}

	R_ParseImageProgram_r( src, pic, width, height, timestamps, usage );

	src.FreeSource();
}

/*
===================
R_ParsePastImageProgram
===================
*/
const char* R_ParsePastImageProgram( idLexer& src )
{
	parseBuffer[0] = 0;
	R_ParseImageProgram_r( src, NULL, NULL, NULL, NULL, NULL );
	return parseBuffer;
}

