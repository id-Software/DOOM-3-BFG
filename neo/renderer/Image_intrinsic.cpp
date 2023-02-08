/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013-2021 Robert Beckebans
Copyright (C) 2022 Stephen Pridham

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

#include "imgui.h"

#include "RenderCommon.h"
#include "SMAA/AreaTex.h"
#include "SMAA/SearchTex.h"
#include "Image_brdfLut.h"
//#include "Image_blueNoiseVC_1M.h" // 256^2 R8 data
#include "Image_blueNoiseVC_2.h" // 512^2 RGB8 data

#include "Image_env_UAC_lobby_amb.h"
#include "Image_env_UAC_lobby_spec.h"

#define	DEFAULT_SIZE	16

/*
==================
idImage::MakeDefault

the default image will be grey with a white box outline
to allow you to see the mapping coordinates on a surface
==================
*/
void idImage::MakeDefault( nvrhi::ICommandList* commandList )
{
	int		x, y;
	byte	data[DEFAULT_SIZE][DEFAULT_SIZE][4];

	if( com_developer.GetBool() )
	{
		// grey center
		for( y = 0 ; y < DEFAULT_SIZE ; y++ )
		{
			for( x = 0 ; x < DEFAULT_SIZE ; x++ )
			{
				data[y][x][0] = 32;
				data[y][x][1] = 32;
				data[y][x][2] = 32;
				data[y][x][3] = 255;
			}
		}

		// white border
		for( x = 0 ; x < DEFAULT_SIZE ; x++ )
		{
			data[0][x][0] =
				data[0][x][1] =
					data[0][x][2] =
						data[0][x][3] = 255;

			data[x][0][0] =
				data[x][0][1] =
					data[x][0][2] =
						data[x][0][3] = 255;

			data[DEFAULT_SIZE - 1][x][0] =
				data[DEFAULT_SIZE - 1][x][1] =
					data[DEFAULT_SIZE - 1][x][2] =
						data[DEFAULT_SIZE - 1][x][3] = 255;

			data[x][DEFAULT_SIZE - 1][0] =
				data[x][DEFAULT_SIZE - 1][1] =
					data[x][DEFAULT_SIZE - 1][2] =
						data[x][DEFAULT_SIZE - 1][3] = 255;
		}
	}
	else
	{
		for( y = 0 ; y < DEFAULT_SIZE ; y++ )
		{
			for( x = 0 ; x < DEFAULT_SIZE ; x++ )
			{
				data[y][x][0] = 0;
				data[y][x][1] = 0;
				data[y][x][2] = 0;
				data[y][x][3] = 0;
			}
		}
	}

	GenerateImage( ( byte* )data,
				   DEFAULT_SIZE, DEFAULT_SIZE,
				   TF_DEFAULT, TR_REPEAT, TD_DEFAULT, commandList );

	defaulted = true;
}

static void R_DefaultImage( idImage* image, nvrhi::ICommandList* commandList )
{
	image->MakeDefault( commandList );
}

static void R_WhiteImage( idImage* image, nvrhi::ICommandList* commandList )
{
	byte	data[DEFAULT_SIZE][DEFAULT_SIZE][4];

	// solid white texture
	memset( data, 255, sizeof( data ) );
	image->GenerateImage( ( byte* )data, DEFAULT_SIZE, DEFAULT_SIZE,
						  TF_DEFAULT, TR_REPEAT, TD_DEFAULT, commandList );
}

static void R_BlackImage( idImage* image, nvrhi::ICommandList* commandList )
{
	byte	data[DEFAULT_SIZE][DEFAULT_SIZE][4];

	// solid black texture
	memset( data, 0, sizeof( data ) );
	image->GenerateImage( ( byte* )data, DEFAULT_SIZE, DEFAULT_SIZE,
						  TF_DEFAULT, TR_REPEAT, TD_DEFAULT, commandList );
}

static void R_BlackDiffuseImage( idImage* image, nvrhi::ICommandList* commandList )
{
	byte	data[DEFAULT_SIZE][DEFAULT_SIZE][4];

	// solid black texture
	memset( data, 0, sizeof( data ) );
	image->GenerateImage( ( byte* )data, DEFAULT_SIZE, DEFAULT_SIZE,
						  TF_DEFAULT, TR_REPEAT, TD_DIFFUSE, commandList );
}

static void R_CyanImage( idImage* image, nvrhi::ICommandList* commandList )
{
	byte	data[DEFAULT_SIZE][DEFAULT_SIZE][4];

	for( int x = 0; x < DEFAULT_SIZE; x++ )
	{
		for( int y = 0; y < DEFAULT_SIZE; y++ )
		{
			data[y][x][0] = byte( colorCyan.x * 255 );
			data[y][x][1] = byte( colorCyan.y * 255 );
			data[y][x][2] = byte( colorCyan.z * 255 );
			data[y][x][3] = byte( colorCyan.w * 255 );
		}
	}

	image->GenerateImage( ( byte* )data, DEFAULT_SIZE, DEFAULT_SIZE, TF_DEFAULT, TR_REPEAT, TD_DIFFUSE, commandList );
}

static void R_ChromeSpecImage( idImage* image, nvrhi::ICommandList* commandList )
{
	byte	data[DEFAULT_SIZE][DEFAULT_SIZE][4];

	for( int x = 0; x < DEFAULT_SIZE; x++ )
	{
		for( int y = 0; y < DEFAULT_SIZE; y++ )
		{
			data[y][x][0] = 0;
			data[y][x][1] = 255;
			data[y][x][2] = 255;
			data[y][x][3] = 255;
		}
	}

	image->GenerateImage( ( byte* )data, DEFAULT_SIZE, DEFAULT_SIZE, TF_DEFAULT, TR_REPEAT, TD_SPECULAR_PBR_RMAO, commandList );
}

static void R_PlasticSpecImage( idImage* image, nvrhi::ICommandList* commandList )
{
	byte	data[DEFAULT_SIZE][DEFAULT_SIZE][4];

	for( int x = 0; x < DEFAULT_SIZE; x++ )
	{
		for( int y = 0; y < DEFAULT_SIZE; y++ )
		{
			data[y][x][0] = 0;
			data[y][x][1] = 0;
			data[y][x][2] = 255;
			data[y][x][3] = 255;
		}
	}

	image->GenerateImage( ( byte* )data, DEFAULT_SIZE, DEFAULT_SIZE, TF_DEFAULT, TR_REPEAT, TD_SPECULAR_PBR_RMAO, commandList );
}

static void R_RGBA8Image( idImage* image, nvrhi::ICommandList* commandList )
{
	byte	data[DEFAULT_SIZE][DEFAULT_SIZE][4];

	memset( data, 0, sizeof( data ) );
	data[0][0][0] = 16;
	data[0][0][1] = 32;
	data[0][0][2] = 48;
	data[0][0][3] = 96;

	image->GenerateImage( ( byte* )data, DEFAULT_SIZE, DEFAULT_SIZE, TF_DEFAULT, TR_REPEAT, TD_LOOKUP_TABLE_RGBA, commandList );
}

static void R_RGBA8LinearImage( idImage* image, nvrhi::ICommandList* commandList )
{
	byte	data[DEFAULT_SIZE][DEFAULT_SIZE][4];

	memset( data, 0, sizeof( data ) );
	data[0][0][0] = 16;
	data[0][0][1] = 32;
	data[0][0][2] = 48;
	data[0][0][3] = 96;

	image->GenerateImage( ( byte* )data, DEFAULT_SIZE, DEFAULT_SIZE, TF_LINEAR, TR_REPEAT, TD_LOOKUP_TABLE_RGBA, commandList );
}

static void R_LdrNativeImage( idImage* image, nvrhi::ICommandList* commandList )
{
	image->GenerateImage( NULL, renderSystem->GetWidth(), renderSystem->GetHeight(), TF_NEAREST, TR_CLAMP, TD_LOOKUP_TABLE_RGBA, nullptr, true, false, 1 );
}

static void R_DepthImage( idImage* image, nvrhi::ICommandList* commandList )
{
	uint sampleCount = R_GetMSAASamples();

	image->GenerateImage( NULL, renderSystem->GetWidth(), renderSystem->GetHeight(), TF_NEAREST, TR_CLAMP, TD_DEPTH_STENCIL, nullptr, true, false, sampleCount );
}

// RB begin
static void R_HDR_RGBA16FImage_ResNative_MSAAOpt( idImage* image, nvrhi::ICommandList* commandList )
{
	uint sampleCount = R_GetMSAASamples();

	image->GenerateImage( NULL, renderSystem->GetWidth(), renderSystem->GetHeight(), TF_NEAREST, TR_CLAMP, TD_RGBA16F, nullptr, true, sampleCount == 1, sampleCount );
}

static void R_HDR_RG16FImage_ResNative( idImage* image, nvrhi::ICommandList* commandList )
{
	image->GenerateImage( NULL, renderSystem->GetWidth(), renderSystem->GetHeight(), TF_NEAREST, TR_CLAMP, TD_RG16F, nullptr, true );
}

static void R_HDR_RGBA16FImage_ResNative( idImage* image, nvrhi::ICommandList* commandList )
{
	image->GenerateImage( NULL, renderSystem->GetWidth(), renderSystem->GetHeight(), TF_NEAREST, TR_CLAMP, TD_RGBA16F, nullptr, true );
}

static void R_HDR_RGBA16FImage_ResNative_UAV( idImage* image, nvrhi::ICommandList* commandList )
{
	image->GenerateImage( NULL, renderSystem->GetWidth(), renderSystem->GetHeight(), TF_NEAREST, TR_CLAMP, TD_RGBA16F, nullptr, true, true );
}

static void R_HDR_RGBA16SImage_ResNative_UAV( idImage* image, nvrhi::ICommandList* commandList )
{
	image->GenerateImage( NULL, renderSystem->GetWidth(), renderSystem->GetHeight(), TF_NEAREST, TR_CLAMP, TD_RGBA16S, nullptr, true, true );
}

static void R_HDR_RGBA16FImage_ResGui( idImage* image, nvrhi::ICommandList* commandList )
{
	image->GenerateImage( NULL, SCREEN_WIDTH, SCREEN_HEIGHT, TF_NEAREST, TR_CLAMP, TD_RGBA16F, nullptr, true );
}

static void R_RGBA8Image_ResGui( idImage* image, nvrhi::ICommandList* commandList )
{
	image->GenerateImage( NULL, SCREEN_WIDTH, SCREEN_HEIGHT, TF_DEFAULT, TR_CLAMP, TD_LOOKUP_TABLE_RGBA, nullptr, true );
}

static void R_HDR_RGBA16FImage_ResNative_Linear( idImage* image, nvrhi::ICommandList* commandList )
{
	image->GenerateImage( NULL, renderSystem->GetWidth(), renderSystem->GetHeight(), TF_NEAREST, TR_CLAMP, TD_RGBA16F, nullptr, true );
}

static void R_HDR_RGBA16FImage_ResNative_NoMSAA( idImage* image, nvrhi::ICommandList* commandList )
{
	image->GenerateImage( NULL, renderSystem->GetWidth(), renderSystem->GetHeight(), TF_NEAREST, TR_CLAMP, TD_RGBA16F, nullptr, true );
}

static void R_HDR_RGBA16FImage_ResQuarter( idImage* image, nvrhi::ICommandList* commandList )
{
	image->GenerateImage( NULL, renderSystem->GetWidth() / 4, renderSystem->GetHeight() / 4, TF_NEAREST, TR_CLAMP, TD_RGBA16F, nullptr, true );
}

static void R_HDR_RGBA16FImage_ResQuarter_Linear( idImage* image, nvrhi::ICommandList* commandList )
{
	image->GenerateImage( NULL, renderSystem->GetWidth() / 4, renderSystem->GetHeight() / 4, TF_LINEAR, TR_CLAMP, TD_LOOKUP_TABLE_RGBA, nullptr, true );
}

static void R_HDR_RGBA16FImage_Res64( idImage* image, nvrhi::ICommandList* commandList )
{
	image->GenerateImage( NULL, 64, 64, TF_NEAREST, TR_CLAMP, TD_RGBA16F, nullptr, true );
}

static void R_EnvprobeImage_HDR( idImage* image, nvrhi::ICommandList* commandList )
{
	image->GenerateImage( NULL, ENVPROBE_CAPTURE_SIZE, ENVPROBE_CAPTURE_SIZE, TF_NEAREST, TR_CLAMP, TD_RGBA16F, nullptr, true );
}

static void R_EnvprobeImage_Depth( idImage* image, nvrhi::ICommandList* commandList )
{
	image->GenerateImage( NULL, ENVPROBE_CAPTURE_SIZE, ENVPROBE_CAPTURE_SIZE, TF_NEAREST, TR_CLAMP, TD_DEPTH_STENCIL, nullptr, true );
}

static void R_SMAAImage_ResNative( idImage* image, nvrhi::ICommandList* commandList )
{
	image->GenerateImage( NULL, renderSystem->GetWidth(), renderSystem->GetHeight(), TF_LINEAR, TR_CLAMP, TD_LOOKUP_TABLE_RGBA, nullptr, true );
}

static void R_AmbientOcclusionImage_ResNative( idImage* image, nvrhi::ICommandList* commandList )
{
	image->GenerateImage( NULL, renderSystem->GetWidth(), renderSystem->GetHeight(), TF_LINEAR, TR_CLAMP, TD_R8F, nullptr, true, true );
}

static void R_GeometryBufferImage_ResNative( idImage* image, nvrhi::ICommandList* commandList )
{
	uint sampleCount = R_GetMSAASamples();

	image->GenerateImage( NULL, renderSystem->GetWidth(), renderSystem->GetHeight(), TF_LINEAR, TR_CLAMP, TD_RGBA16F, nullptr, true, false, sampleCount );
}

static void R_SSAOImage_ResHalf( idImage* image, nvrhi::ICommandList* commandList )
{
	image->GenerateImage( NULL, renderSystem->GetWidth() / 2, renderSystem->GetHeight() / 2, TF_LINEAR, TR_CLAMP, TD_LOOKUP_TABLE_RGBA, nullptr, true );
}

static void R_HierarchicalZBufferImage_ResNative( idImage* image, nvrhi::ICommandList* commandList )
{
	image->GenerateImage( NULL, renderSystem->GetWidth(), renderSystem->GetHeight(), TF_NEAREST_MIPMAP, TR_CLAMP, TD_R32F, nullptr, true, true );
}

static void R_R8Image_ResNative_Linear( idImage* image, nvrhi::ICommandList* commandList )
{
	image->GenerateImage( NULL, renderSystem->GetWidth(), renderSystem->GetHeight(), TF_LINEAR, TR_CLAMP, TD_LOOKUP_TABLE_MONO, nullptr, true );
}
// RB end

static void R_HDR_RGBA8Image_ResNative( idImage* image, nvrhi::ICommandList* commandList )
{
	image->GenerateImage( NULL, renderSystem->GetWidth(), renderSystem->GetHeight(), TF_NEAREST, TR_CLAMP, TD_LOOKUP_TABLE_RGBA, commandList, true );
}

static void R_AlphaNotchImage( idImage* image, nvrhi::ICommandList* commandList )
{
	byte	data[2][4];

	// this is used for alpha test clip planes

	data[0][0] = data[0][1] = data[0][2] = 255;
	data[0][3] = 0;
	data[1][0] = data[1][1] = data[1][2] = 255;
	data[1][3] = 255;

	image->GenerateImage( ( byte* )data, 2, 1, TF_NEAREST, TR_CLAMP, TD_LOOKUP_TABLE_ALPHA, commandList );
}

static void R_FlatNormalImage( idImage* image, nvrhi::ICommandList* commandList )
{
	byte	data[DEFAULT_SIZE][DEFAULT_SIZE][4];

	// flat normal map for default bump mapping
	for( int i = 0; i < DEFAULT_SIZE; i++ )
	{
		for( int j = 0; j < DEFAULT_SIZE; j++ )
		{
			data[j][i][0] = 128;
			data[j][i][1] = 128;
			data[j][i][2] = 255;
			data[j][i][3] = 255;
		}
	}

	image->GenerateImage( ( byte* )data, 16, 16, TF_DEFAULT, TR_REPEAT, TD_BUMP, commandList );
}

/*
================
R_CreateNoFalloffImage

This is a solid white texture that is zero clamped.
================
*/
static void R_CreateNoFalloffImage( idImage* image, nvrhi::ICommandList* commandList )
{
	int		x, y;
	byte	data[16][FALLOFF_TEXTURE_SIZE][4];

	memset( data, 0, sizeof( data ) );
	for( x = 1 ; x < FALLOFF_TEXTURE_SIZE - 1 ; x++ )
	{
		for( y = 1 ; y < 15 ; y++ )
		{
			data[y][x][0] = 255;
			data[y][x][1] = 255;
			data[y][x][2] = 255;
			data[y][x][3] = 255;
		}
	}
	image->GenerateImage( ( byte* )data, FALLOFF_TEXTURE_SIZE, 16, TF_DEFAULT, TR_CLAMP_TO_ZERO, TD_LOOKUP_TABLE_MONO, commandList );
}

/*
================
R_FogImage

We calculate distance correctly in two planes, but the
third will still be projection based
================
*/
const int	FOG_SIZE = 128;

void R_FogImage( idImage* image, nvrhi::ICommandList* commandList )
{
	int		x, y;
	byte	data[FOG_SIZE][FOG_SIZE][4];
	int		b;

	float	step[256];
	int		i;
	float	remaining = 1.0;
	for( i = 0 ; i < 256 ; i++ )
	{
		step[i] = remaining;
		remaining *= 0.982f;
	}

	for( x = 0 ; x < FOG_SIZE ; x++ )
	{
		for( y = 0 ; y < FOG_SIZE ; y++ )
		{
			float	d;

			d = idMath::Sqrt( ( x - FOG_SIZE / 2 ) * ( x - FOG_SIZE / 2 )
							  + ( y - FOG_SIZE / 2 ) * ( y - FOG_SIZE / 2 ) );
			d /= FOG_SIZE / 2 - 1;

			b = ( byte )( d * 255 );
			if( b <= 0 )
			{
				b = 0;
			}
			else if( b > 255 )
			{
				b = 255;
			}
			b = ( byte )( 255 * ( 1.0 - step[b] ) );
			if( x == 0 || x == FOG_SIZE - 1 || y == 0 || y == FOG_SIZE - 1 )
			{
				b = 255;		// avoid clamping issues
			}
			data[y][x][0] =
				data[y][x][1] =
					data[y][x][2] = 255;
			data[y][x][3] = b;
		}
	}

	image->GenerateImage( ( byte* )data, FOG_SIZE, FOG_SIZE, TF_LINEAR, TR_CLAMP, TD_LOOKUP_TABLE_ALPHA, commandList );
}


/*
================
FogFraction

Height values below zero are inside the fog volume
================
*/
static const float	RAMP_RANGE =	8;
static const float	DEEP_RANGE =	-30;
static float	FogFraction( float viewHeight, float targetHeight )
{
	float	total = idMath::Fabs( targetHeight - viewHeight );

//	return targetHeight >= 0 ? 0 : 1.0;

	// only ranges that cross the ramp range are special
	if( targetHeight > 0 && viewHeight > 0 )
	{
		return 0.0;
	}
	if( targetHeight < -RAMP_RANGE && viewHeight < -RAMP_RANGE )
	{
		return 1.0;
	}

	float	above;
	if( targetHeight > 0 )
	{
		above = targetHeight;
	}
	else if( viewHeight > 0 )
	{
		above = viewHeight;
	}
	else
	{
		above = 0;
	}

	float	rampTop, rampBottom;

	if( viewHeight > targetHeight )
	{
		rampTop = viewHeight;
		rampBottom = targetHeight;
	}
	else
	{
		rampTop = targetHeight;
		rampBottom = viewHeight;
	}
	if( rampTop > 0 )
	{
		rampTop = 0;
	}
	if( rampBottom < -RAMP_RANGE )
	{
		rampBottom = -RAMP_RANGE;
	}

	float	rampSlope = 1.0 / RAMP_RANGE;

	if( !total )
	{
		return -viewHeight * rampSlope;
	}

	float ramp = ( 1.0 - ( rampTop * rampSlope + rampBottom * rampSlope ) * -0.5 ) * ( rampTop - rampBottom );

	float	frac = ( total - above - ramp ) / total;

	// after it gets moderately deep, always use full value
	float deepest = viewHeight < targetHeight ? viewHeight : targetHeight;

	float	deepFrac = deepest / DEEP_RANGE;
	if( deepFrac >= 1.0 )
	{
		return 1.0;
	}

	frac = frac * ( 1.0 - deepFrac ) + deepFrac;

	return frac;
}

/*
================
R_FogEnterImage

Modulate the fog alpha density based on the distance of the
start and end points to the terminator plane
================
*/
void R_FogEnterImage( idImage* image, nvrhi::ICommandList* commandList )
{
	int		x, y;
	byte	data[FOG_ENTER_SIZE][FOG_ENTER_SIZE][4];
	int		b;

	for( x = 0 ; x < FOG_ENTER_SIZE ; x++ )
	{
		for( y = 0 ; y < FOG_ENTER_SIZE ; y++ )
		{
			float	d;

			d = FogFraction( x - ( FOG_ENTER_SIZE / 2 ), y - ( FOG_ENTER_SIZE / 2 ) );

			b = ( byte )( d * 255 );
			if( b <= 0 )
			{
				b = 0;
			}
			else if( b > 255 )
			{
				b = 255;
			}
			data[y][x][0] =
				data[y][x][1] =
					data[y][x][2] = 255;
			data[y][x][3] = b;
		}
	}

	// if mipmapped, acutely viewed surfaces fade wrong
	image->GenerateImage( ( byte* )data, FOG_ENTER_SIZE, FOG_ENTER_SIZE, TF_LINEAR, TR_CLAMP, TD_LOOKUP_TABLE_ALPHA, commandList );
}


/*
================
R_QuadraticImage

================
*/
static const int	QUADRATIC_WIDTH = 32;
static const int	QUADRATIC_HEIGHT = 4;

void R_QuadraticImage( idImage* image, nvrhi::ICommandList* commandList )
{
	int		x, y;
	byte	data[QUADRATIC_HEIGHT][QUADRATIC_WIDTH][4];
	int		b;


	for( x = 0 ; x < QUADRATIC_WIDTH ; x++ )
	{
		for( y = 0 ; y < QUADRATIC_HEIGHT ; y++ )
		{
			float	d;

			d = x - ( QUADRATIC_WIDTH / 2 - 0.5 );
			d = idMath::Fabs( d );
			d -= 0.5;
			d /= QUADRATIC_WIDTH / 2;

			d = 1.0 - d;
			d = d * d;

			b = ( byte )( d * 255 );
			if( b <= 0 )
			{
				b = 0;
			}
			else if( b > 255 )
			{
				b = 255;
			}
			data[y][x][0] =
				data[y][x][1] =
					data[y][x][2] = b;
			data[y][x][3] = 255;
		}
	}

	image->GenerateImage( ( byte* )data, QUADRATIC_WIDTH, QUADRATIC_HEIGHT, TF_DEFAULT, TR_CLAMP, TD_LOOKUP_TABLE_RGB1, commandList );
}

// RB begin
static void R_CreateShadowMapImage_Atlas( idImage* image, nvrhi::ICommandList* commandList )
{
	image->GenerateImage( NULL, r_shadowMapAtlasSize.GetInteger(), r_shadowMapAtlasSize.GetInteger(), TF_LINEAR, TR_CLAMP_TO_ZERO_ALPHA, TD_DEPTH, commandList, true );
}

static void R_CreateShadowMapImage_Res0( idImage* image, nvrhi::ICommandList* commandList )
{
	int size = shadowMapResolutions[0];
	image->GenerateShadowArray( size, size, TF_LINEAR, TR_CLAMP_TO_ZERO_ALPHA, TD_SHADOW_ARRAY, commandList );
}

static void R_CreateShadowMapImage_Res1( idImage* image, nvrhi::ICommandList* commandList )
{
	int size = shadowMapResolutions[1];
	image->GenerateShadowArray( size, size, TF_LINEAR, TR_CLAMP_TO_ZERO_ALPHA, TD_SHADOW_ARRAY, commandList );
}

static void R_CreateShadowMapImage_Res2( idImage* image, nvrhi::ICommandList* commandList )
{
	int size = shadowMapResolutions[2];
	image->GenerateShadowArray( size, size, TF_LINEAR, TR_CLAMP_TO_ZERO_ALPHA, TD_SHADOW_ARRAY, commandList );
}

static void R_CreateShadowMapImage_Res3( idImage* image, nvrhi::ICommandList* commandList )
{
	int size = shadowMapResolutions[3];
	image->GenerateShadowArray( size, size, TF_LINEAR, TR_CLAMP_TO_ZERO_ALPHA, TD_SHADOW_ARRAY, commandList );
}

static void R_CreateShadowMapImage_Res4( idImage* image, nvrhi::ICommandList* commandList )
{
	int size = shadowMapResolutions[4];
	image->GenerateShadowArray( size, size, TF_LINEAR, TR_CLAMP_TO_ZERO_ALPHA, TD_SHADOW_ARRAY, commandList );
}

const static int JITTER_SIZE = 128;
static void R_CreateJitterImage16( idImage* image, nvrhi::ICommandList* commandList )
{
	static byte	data[JITTER_SIZE][JITTER_SIZE * 16][4];

	for( int i = 0 ; i < JITTER_SIZE ; i++ )
	{
		for( int s = 0 ; s < 16 ; s++ )
		{
			int sOfs = 64 * ( s & 3 );
			int tOfs = 64 * ( ( s >> 2 ) & 3 );

			for( int j = 0 ; j < JITTER_SIZE ; j++ )
			{
				data[i][s * JITTER_SIZE + j][0] = ( rand() & 63 ) | sOfs;
				data[i][s * JITTER_SIZE + j][1] = ( rand() & 63 ) | tOfs;
				data[i][s * JITTER_SIZE + j][2] = rand();
				data[i][s * JITTER_SIZE + j][3] = 0;
			}
		}
	}

	image->GenerateImage( ( byte* )data, JITTER_SIZE * 16, JITTER_SIZE, TF_NEAREST, TR_REPEAT, TD_LOOKUP_TABLE_RGBA, commandList );
}

static void R_CreateJitterImage4( idImage* image, nvrhi::ICommandList* commandList )
{
	byte	data[JITTER_SIZE][JITTER_SIZE * 4][4];

	for( int i = 0 ; i < JITTER_SIZE ; i++ )
	{
		for( int s = 0 ; s < 4 ; s++ )
		{
			int sOfs = 128 * ( s & 1 );
			int tOfs = 128 * ( ( s >> 1 ) & 1 );

			for( int j = 0 ; j < JITTER_SIZE ; j++ )
			{
				data[i][s * JITTER_SIZE + j][0] = ( rand() & 127 ) | sOfs;
				data[i][s * JITTER_SIZE + j][1] = ( rand() & 127 ) | tOfs;
				data[i][s * JITTER_SIZE + j][2] = rand();
				data[i][s * JITTER_SIZE + j][3] = 0;
			}
		}
	}

	image->GenerateImage( ( byte* )data, JITTER_SIZE * 4, JITTER_SIZE, TF_NEAREST, TR_REPEAT, TD_LOOKUP_TABLE_RGBA, commandList );
}

static void R_CreateJitterImage1( idImage* image, nvrhi::ICommandList* commandList )
{
	byte	data[JITTER_SIZE][JITTER_SIZE][4];

	for( int i = 0 ; i < JITTER_SIZE ; i++ )
	{
		for( int j = 0 ; j < JITTER_SIZE ; j++ )
		{
			data[i][j][0] = rand();
			data[i][j][1] = rand();
			data[i][j][2] = rand();
			data[i][j][3] = 0;
		}
	}

	image->GenerateImage( ( byte* )data, JITTER_SIZE, JITTER_SIZE, TF_NEAREST, TR_REPEAT, TD_LOOKUP_TABLE_RGBA, commandList );
}

static void R_CreateRandom256Image( idImage* image, nvrhi::ICommandList* commandList )
{
	byte	data[256][256][4];

	for( int i = 0 ; i < 256 ; i++ )
	{
		for( int j = 0 ; j < 256 ; j++ )
		{
			data[i][j][0] = rand();
			data[i][j][1] = rand();
			data[i][j][2] = rand();
			data[i][j][3] = rand();
		}
	}

	image->GenerateImage( ( byte* )data, 256, 256, TF_NEAREST, TR_REPEAT, TD_LOOKUP_TABLE_RGBA, commandList );
}

// RB
static void R_CreateBlueNoise256Image( idImage* image, nvrhi::ICommandList* commandList )
{
	static byte	data[BLUENOISE_TEX_HEIGHT][BLUENOISE_TEX_WIDTH][4];

	for( int x = 0; x < BLUENOISE_TEX_WIDTH; x++ )
	{
		for( int y = 0; y < BLUENOISE_TEX_HEIGHT; y++ )
		{
#if 1
			data[x][y][0] = blueNoiseTexBytes[ y * BLUENOISE_TEX_PITCH + x * 3 + 0 ];
			data[x][y][1] = blueNoiseTexBytes[ y * BLUENOISE_TEX_PITCH + x * 3 + 1 ];
			data[x][y][2] = blueNoiseTexBytes[ y * BLUENOISE_TEX_PITCH + x * 3 + 2 ];
#else
			data[x][y][0] = blueNoiseTexBytes[ y * BLUENOISE_TEX_PITCH + x ];
			data[x][y][1] = blueNoiseTexBytes[ y * BLUENOISE_TEX_PITCH + x ];
			data[x][y][2] = blueNoiseTexBytes[ y * BLUENOISE_TEX_PITCH + x ];
#endif
			data[x][y][3] = 1;
		}
	}

	image->GenerateImage( ( byte* )data, BLUENOISE_TEX_WIDTH, BLUENOISE_TEX_HEIGHT, TF_NEAREST, TR_REPEAT, TD_LOOKUP_TABLE_RGBA, commandList );
}


static void R_CreateHeatmap5ColorsImage( idImage* image, nvrhi::ICommandList* commandList )
{
	int		x, y;
	byte	data[16][FALLOFF_TEXTURE_SIZE][4];

	const int numColors = 5;
	static idVec4 colors[numColors] = { colorBlue, colorCyan, colorGreen, colorYellow, colorRed };

	memset( data, 0, sizeof( data ) );
	for( x = 0 ; x < FALLOFF_TEXTURE_SIZE; x++ )
	{
		int index1, index2;

		float value = x / ( float )FALLOFF_TEXTURE_SIZE;

		float lerp = 0.0f;

		if( value <= 0.0 )
		{
			index1 = index2 = 0;
		}
		else if( value >= 1.0f )
		{
			index1 = index2 = numColors - 1;
		}
		else
		{
			value = value * ( numColors - 1 );
			index1 = idMath::Floor( value );
			index2 = index1 + 1;
			lerp = value - float( index1 );
		}

		idVec4 color( 0, 0, 0, 1 );

		color.x = ( colors[index2].x - colors[index1].x ) * lerp + colors[index1].x;
		color.y = ( colors[index2].y - colors[index1].y ) * lerp + colors[index1].y;
		color.z = ( colors[index2].z - colors[index1].z ) * lerp + colors[index1].z;

		for( y = 0 ; y < 16 ; y++ )
		{
			data[y][x][0] = color.x * 255;
			data[y][x][1] = color.y * 255;
			data[y][x][2] = color.z * 255;
			data[y][x][3] = 255;
		}
	}

	image->GenerateImage( ( byte* )data, FALLOFF_TEXTURE_SIZE, 16, TF_LINEAR, TR_CLAMP, TD_LOOKUP_TABLE_RGBA, commandList );
}

static void R_CreateHeatmap7ColorsImage( idImage* image, nvrhi::ICommandList* commandList )
{
	int		x, y;
	byte	data[16][FALLOFF_TEXTURE_SIZE][4];

	const int numColors = 7;
	static idVec4 colors[numColors] = { colorBlack, colorBlue, colorCyan, colorGreen, colorYellow, colorRed, colorWhite };

	memset( data, 0, sizeof( data ) );
	for( x = 0 ; x < FALLOFF_TEXTURE_SIZE; x++ )
	{
		int index1, index2;

		float value = x / ( float )FALLOFF_TEXTURE_SIZE;

		float lerp = 0.0f;

		if( value <= 0.0 )
		{
			index1 = index2 = 0;
		}
		else if( value >= 1.0f )
		{
			index1 = index2 = numColors - 1;
		}
		else
		{
			value = value * ( numColors - 1 );
			index1 = idMath::Floor( value );
			index2 = index1 + 1;
			lerp = value - float( index1 );
		}

		idVec4 color( 0, 0, 0, 1 );

		color.x = ( colors[index2].x - colors[index1].x ) * lerp + colors[index1].x;
		color.y = ( colors[index2].y - colors[index1].y ) * lerp + colors[index1].y;
		color.z = ( colors[index2].z - colors[index1].z ) * lerp + colors[index1].z;

		for( y = 0 ; y < 16 ; y++ )
		{
			data[y][x][0] = color.x * 255;
			data[y][x][1] = color.y * 255;
			data[y][x][2] = color.z * 255;
			data[y][x][3] = 255;
		}
	}

	image->GenerateImage( ( byte* )data, FALLOFF_TEXTURE_SIZE, 16, TF_LINEAR, TR_CLAMP, TD_LOOKUP_TABLE_RGBA, commandList );
}

static void R_CreateGrainImage1( idImage* image, nvrhi::ICommandList* commandList )
{
	const static int GRAIN_SIZE = 128;

	static byte	data[GRAIN_SIZE][GRAIN_SIZE][4];

	idRandom2 random( Sys_Milliseconds() );

	for( int i = 0 ; i < GRAIN_SIZE ; i++ )
	{
		for( int j = 0 ; j < GRAIN_SIZE ; j++ )
		{
#if 0
			//int value = 127 - 8 + ( rand() & 15 ); //random.RandomInt( 127 );
			int value = 127 - 8 + random.RandomInt( 15 );

			data[i][j][0] = value;
			data[i][j][1] = value;
			data[i][j][2] = value;
			data[i][j][3] = 0;
#else
			data[i][j][0] = 127 - 8 + random.RandomInt( 15 );
			data[i][j][1] = 127 - 8 + random.RandomInt( 15 );
			data[i][j][2] = 127 - 8 + random.RandomInt( 15 );
			data[i][j][3] = 0;
#endif
		}
	}

	image->GenerateImage( ( byte* )data, GRAIN_SIZE, GRAIN_SIZE, TF_NEAREST, TR_REPEAT, TD_LOOKUP_TABLE_RGBA, commandList );
}

static void R_CreateSMAAAreaImage( idImage* image, nvrhi::ICommandList* commandList )
{
	static byte	data[AREATEX_HEIGHT][AREATEX_WIDTH][4];

	for( int x = 0; x < AREATEX_WIDTH; x++ )
	{
		for( int y = 0; y < AREATEX_HEIGHT; y++ )
		{
#if 0
			data[AREATEX_HEIGHT - y][x][0] = areaTexBytes[ y * AREATEX_PITCH + x * 2 + 0 ];
			data[AREATEX_HEIGHT - y][x][1] = areaTexBytes[ y * AREATEX_PITCH + x * 2 + 1 ];
			data[AREATEX_HEIGHT - y][x][2] = 0;
			data[AREATEX_HEIGHT - y][x][3] = 1;
#else
			data[y][x][0] = areaTexBytes[ y * AREATEX_PITCH + x * 2 + 0 ];
			data[y][x][1] = areaTexBytes[ y * AREATEX_PITCH + x * 2 + 1 ];
			data[y][x][2] = 0;
			data[y][x][3] = 1;
#endif
		}
	}

	image->GenerateImage( ( byte* )data, AREATEX_WIDTH, AREATEX_HEIGHT, TF_LINEAR, TR_CLAMP, TD_LOOKUP_TABLE_RGBA, commandList );
}

static void R_CreateSMAASearchImage( idImage* image, nvrhi::ICommandList* commandList )
{
	static byte	data[SEARCHTEX_HEIGHT][SEARCHTEX_WIDTH][4];

	for( int x = 0; x < SEARCHTEX_WIDTH; x++ )
	{
		for( int y = 0; y < SEARCHTEX_HEIGHT; y++ )
		{
#if 0
			data[SEARCHTEX_HEIGHT - y][x][0] = searchTexBytes[ y * SEARCHTEX_PITCH + x ];
			data[SEARCHTEX_HEIGHT - y][x][1] = 0;
			data[SEARCHTEX_HEIGHT - y][x][2] = 0;
			data[SEARCHTEX_HEIGHT - y][x][3] = 1;
#else
			data[y][x][0] = searchTexBytes[ y * SEARCHTEX_PITCH + x ];
			data[y][x][1] = 0;
			data[y][x][2] = 0;
			data[y][x][3] = 1;
#endif
		}
	}

	image->GenerateImage( ( byte* )data, SEARCHTEX_WIDTH, SEARCHTEX_HEIGHT, TF_LINEAR, TR_CLAMP, TD_LOOKUP_TABLE_MONO, commandList );
}

static void R_CreateImGuiFontImage( idImage* image, nvrhi::ICommandList* commandList )
{
	ImGuiIO& io = ImGui::GetIO();

	byte* pixels = NULL;
	int width, height;
	io.Fonts->GetTexDataAsRGBA32( &pixels, &width, &height ); // Load as RGBA 32-bits for OpenGL3 demo because it is more likely to be compatible with user's existing shader.

	image->GenerateImage( ( byte* )pixels, width, height, TF_LINEAR, TR_CLAMP, TD_LOOKUP_TABLE_RGBA, commandList );

	// Store our identifier
	//io.Fonts->TexID = ( void* )( intptr_t )image->GetImGuiTextureID();
	io.Fonts->TexID = ( void* )( intptr_t )declManager->FindMaterial( "_imguiFont" );

	// Cleanup (don't clear the input data if you want to append new fonts later)
	//io.Fonts->ClearInputData();
	//io.Fonts->ClearTexData();
}

static void R_CreateBrdfLutImage( idImage* image, nvrhi::ICommandList* commandList )
{
	image->GenerateImage( ( byte* )brfLutTexBytes, BRDFLUT_TEX_WIDTH, BRDFLUT_TEX_HEIGHT, TF_LINEAR, TR_CLAMP, TD_RG16F, commandList );
}

static void R_CreateEnvprobeImage_UAC_lobby_irradiance( idImage* image, nvrhi::ICommandList* commandList )
{
	image->GenerateImage( ( byte* )IMAGE_ENV_UAC_LOBBY_AMB_H_Bytes, IMAGE_ENV_UAC_LOBBY_AMB_H_TEX_WIDTH, IMAGE_ENV_UAC_LOBBY_AMB_H_TEX_HEIGHT, TF_DEFAULT, TR_CLAMP, TD_R11G11B10F, commandList, false, false, 1, CF_2D_PACKED_MIPCHAIN );
}

static void R_CreateEnvprobeImage_UAC_lobby_radiance( idImage* image, nvrhi::ICommandList* commandList )
{
	image->GenerateImage( ( byte* )IMAGE_ENV_UAC_LOBBY_SPEC_H_Bytes, IMAGE_ENV_UAC_LOBBY_SPEC_H_TEX_WIDTH, IMAGE_ENV_UAC_LOBBY_SPEC_H_TEX_HEIGHT, TF_DEFAULT, TR_CLAMP, TD_R11G11B10F, commandList, false, false, 1, CF_2D_PACKED_MIPCHAIN );
}

// RB end

static void R_GuiEditFunction( idImage* image, nvrhi::ICommandList* commandList )
{
	image->GenerateImage( nullptr, 640, 480, TF_NEAREST, TR_CLAMP, TD_LOOKUP_TABLE_RGBA, nullptr, true, false, 1 );
}

static void R_GuiEditDepthStencilFunction( idImage* image, nvrhi::ICommandList* commandList )
{
	image->GenerateImage( nullptr, 640, 480, TF_NEAREST, TR_CLAMP, TD_DEPTH_STENCIL, nullptr, true, false, 1 );
}

/*
================
idImageManager::CreateIntrinsicImages
================
*/
void idImageManager::CreateIntrinsicImages()
{
	// create built in images
	defaultImage = ImageFromFunction( "_default", R_DefaultImage );
	whiteImage = ImageFromFunction( "_white", R_WhiteImage );
	blackImage = ImageFromFunction( "_black", R_BlackImage );
	blackDiffuseImage = ImageFromFunction( "_blackDiffuse", R_BlackDiffuseImage );
	cyanImage = ImageFromFunction( "_cyan", R_CyanImage );
	flatNormalMap = ImageFromFunction( "_flat", R_FlatNormalImage );
	alphaNotchImage = ImageFromFunction( "_alphaNotch", R_AlphaNotchImage );
	fogImage = ImageFromFunction( "_fog", R_FogImage );
	fogEnterImage = ImageFromFunction( "_fogEnter", R_FogEnterImage );
	noFalloffImage = ImageFromFunction( "_noFalloff", R_CreateNoFalloffImage );
	ImageFromFunction( "_quadratic", R_QuadraticImage );

	// RB begin
	shadowAtlasImage = ImageFromFunction( "_shadowMapAtlas", R_CreateShadowMapImage_Atlas );
	shadowImage[0] = ImageFromFunction( va( "_shadowMapArray0_%i", shadowMapResolutions[0] ), R_CreateShadowMapImage_Res0 );
	shadowImage[1] = ImageFromFunction( va( "_shadowMapArray1_%i", shadowMapResolutions[1] ), R_CreateShadowMapImage_Res1 );
	shadowImage[2] = ImageFromFunction( va( "_shadowMapArray2_%i", shadowMapResolutions[2] ), R_CreateShadowMapImage_Res2 );
	shadowImage[3] = ImageFromFunction( va( "_shadowMapArray3_%i", shadowMapResolutions[3] ), R_CreateShadowMapImage_Res3 );
	shadowImage[4] = ImageFromFunction( va( "_shadowMapArray4_%i", shadowMapResolutions[4] ), R_CreateShadowMapImage_Res4 );

	jitterImage1 = globalImages->ImageFromFunction( "_jitter1", R_CreateJitterImage1 );
	jitterImage4 = globalImages->ImageFromFunction( "_jitter4", R_CreateJitterImage4 );
	jitterImage16 = globalImages->ImageFromFunction( "_jitter16", R_CreateJitterImage16 );

	randomImage256 = globalImages->ImageFromFunction( "_random256", R_CreateRandom256Image );
	blueNoiseImage256 = globalImages->ImageFromFunction( "_blueNoise256", R_CreateBlueNoise256Image );

	currentRenderHDRImage = globalImages->ImageFromFunction( "_currentRenderHDR", R_HDR_RGBA16FImage_ResNative_MSAAOpt );
	currentRenderHDRImage64 = globalImages->ImageFromFunction( "_currentRenderHDR64", R_HDR_RGBA16FImage_Res64 );
	ldrImage = globalImages->ImageFromFunction( "_currentRenderLDR", R_LdrNativeImage );

	taaMotionVectorsImage = ImageFromFunction( "_taaMotionVectors", R_HDR_RG16FImage_ResNative ); // RB: could be shared with _currentNormals.zw
	taaResolvedImage = ImageFromFunction( "_taaResolved", R_HDR_RGBA16FImage_ResNative_UAV );
	taaFeedback1Image = ImageFromFunction( "_taaFeedback1", R_HDR_RGBA16SImage_ResNative_UAV );
	taaFeedback2Image = ImageFromFunction( "_taaFeedback2", R_HDR_RGBA16SImage_ResNative_UAV );

	envprobeHDRImage = globalImages->ImageFromFunction( "_envprobeHDR", R_EnvprobeImage_HDR );
	envprobeDepthImage = ImageFromFunction( "_envprobeDepth", R_EnvprobeImage_Depth );

	bloomRenderImage[0] = globalImages->ImageFromFunction( "_bloomRender0", R_HDR_RGBA16FImage_ResQuarter_Linear );
	bloomRenderImage[1] = globalImages->ImageFromFunction( "_bloomRender1", R_HDR_RGBA16FImage_ResQuarter_Linear );

	glowImage[0] = globalImages->ImageFromFunction( "_glowImage0", R_RGBA8Image_ResGui );
	glowImage[1] = globalImages->ImageFromFunction( "_glowImage1", R_RGBA8Image_ResGui );
	glowDepthImage[0] = globalImages->ImageFromFunction( "_glowDepthImage0", R_DepthImage );
	glowDepthImage[1] = globalImages->ImageFromFunction( "_glowDepthImage1", R_DepthImage );

	accumTransparencyImage = globalImages->ImageFromFunction( "_accumTransparencyImage", R_HDR_RGBA16FImage_ResNative_Linear );
	revealTransparencyImage = globalImages->ImageFromFunction( "_revealTransparencyImage", R_R8Image_ResNative_Linear );

	heatmap5Image = ImageFromFunction( "_heatmap5", R_CreateHeatmap5ColorsImage );
	heatmap7Image = ImageFromFunction( "_heatmap7", R_CreateHeatmap7ColorsImage );

	grainImage1 = globalImages->ImageFromFunction( "_grain1", R_CreateGrainImage1 );

	smaaInputImage = ImageFromFunction( "_smaaInput", R_RGBA8LinearImage );

	smaaAreaImage = globalImages->ImageFromFunction( "_smaaArea", R_CreateSMAAAreaImage );
	smaaSearchImage = globalImages->ImageFromFunction( "_smaaSearch", R_CreateSMAASearchImage );

	smaaEdgesImage = globalImages->ImageFromFunction( "_smaaEdges", R_SMAAImage_ResNative );
	smaaBlendImage = globalImages->ImageFromFunction( "_smaaBlend", R_SMAAImage_ResNative );

	gbufferNormalsRoughnessImage = ImageFromFunction( "_currentNormals", R_GeometryBufferImage_ResNative );

	ambientOcclusionImage[0] = ImageFromFunction( "_ao0", R_AmbientOcclusionImage_ResNative );
	ambientOcclusionImage[1] = ImageFromFunction( "_ao1", R_AmbientOcclusionImage_ResNative );

	hierarchicalZbufferImage = ImageFromFunction( "_cszBuffer", R_HierarchicalZBufferImage_ResNative );

	imguiFontImage = ImageFromFunction( "_imguiFont", R_CreateImGuiFontImage );

	chromeSpecImage = ImageFromFunction( "_chromeSpec", R_ChromeSpecImage );
	plasticSpecImage = ImageFromFunction( "_plasticSpec", R_PlasticSpecImage );
	brdfLutImage = ImageFromFunction( "_brdfLut", R_CreateBrdfLutImage );
	// RB end

	// scratchImage is used for screen wipes/doublevision etc..
	scratchImage = ImageFromFunction( "_scratch", R_RGBA8Image );
	scratchImage2 = ImageFromFunction( "_scratch2", R_RGBA8Image );
	accumImage = ImageFromFunction( "_accum", R_RGBA8Image );
	currentRenderImage = ImageFromFunction( "_currentRender", R_HDR_RGBA16FImage_ResNative );
	currentDepthImage = ImageFromFunction( "_currentDepth", R_DepthImage );

	// save a copy of this for material comparison, because currentRenderImage may get
	// reassigned during stereo rendering
	originalCurrentRenderImage = currentRenderImage;

	loadingIconImage = ImageFromFile( "textures/loadingicon2", TF_DEFAULT, TR_CLAMP, TD_DEFAULT, CF_2D );
	hellLoadingIconImage = ImageFromFile( "textures/loadingicon3", TF_DEFAULT, TR_CLAMP, TD_DEFAULT, CF_2D );

	// RB begin
#if 0
	defaultUACIrradianceCube = ImageFromFile( "env/UAC5_amb", TF_DEFAULT, TR_CLAMP, TD_R11G11B10F, CF_2D_PACKED_MIPCHAIN );
	defaultUACRadianceCube = ImageFromFile( "env/UAC5_spec", TF_DEFAULT, TR_CLAMP, TD_R11G11B10F, CF_2D_PACKED_MIPCHAIN );
#else
	defaultUACIrradianceCube = ImageFromFunction( "_defaultUACIrradiance", R_CreateEnvprobeImage_UAC_lobby_irradiance );
	defaultUACRadianceCube = ImageFromFunction( "_defaultUACRadiance", R_CreateEnvprobeImage_UAC_lobby_radiance );
#endif
	// RB end

	guiEdit = ImageFromFunction( "_guiEdit", R_GuiEditFunction );
	guiEditDepthStencilImage = ImageFromFunction( "_guiEditDepthStencil", R_GuiEditDepthStencilFunction );

	release_assert( loadingIconImage->referencedOutsideLevelLoad );
	release_assert( hellLoadingIconImage->referencedOutsideLevelLoad );
}
