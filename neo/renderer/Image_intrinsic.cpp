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
#include "../idlib/precompiled.h"


#include "tr_local.h"

#define	DEFAULT_SIZE	16

/*
==================
idImage::MakeDefault

the default image will be grey with a white box outline
to allow you to see the mapping coordinates on a surface
==================
*/
void idImage::MakeDefault() {
	int		x, y;
	byte	data[DEFAULT_SIZE][DEFAULT_SIZE][4];

	if ( com_developer.GetBool() ) {
		// grey center
		for ( y = 0 ; y < DEFAULT_SIZE ; y++ ) {
			for ( x = 0 ; x < DEFAULT_SIZE ; x++ ) {
				data[y][x][0] = 32;
				data[y][x][1] = 32;
				data[y][x][2] = 32;
				data[y][x][3] = 255;
			}
		}

		// white border
		for ( x = 0 ; x < DEFAULT_SIZE ; x++ ) {
			data[0][x][0] =
				data[0][x][1] =
				data[0][x][2] =
				data[0][x][3] = 255;

			data[x][0][0] =
				data[x][0][1] =
				data[x][0][2] =
				data[x][0][3] = 255;

			data[DEFAULT_SIZE-1][x][0] =
				data[DEFAULT_SIZE-1][x][1] =
				data[DEFAULT_SIZE-1][x][2] =
				data[DEFAULT_SIZE-1][x][3] = 255;

			data[x][DEFAULT_SIZE-1][0] =
				data[x][DEFAULT_SIZE-1][1] =
				data[x][DEFAULT_SIZE-1][2] =
				data[x][DEFAULT_SIZE-1][3] = 255;
		}
	} else {
		for ( y = 0 ; y < DEFAULT_SIZE ; y++ ) {
			for ( x = 0 ; x < DEFAULT_SIZE ; x++ ) {
				data[y][x][0] = 0;
				data[y][x][1] = 0;
				data[y][x][2] = 0;
				data[y][x][3] = 0;
			}
		}
	}

	GenerateImage( (byte *)data, 
		DEFAULT_SIZE, DEFAULT_SIZE, 
		TF_DEFAULT, TR_REPEAT, TD_DEFAULT );

	defaulted = true;
}

static void R_DefaultImage( idImage *image ) {
	image->MakeDefault();
}

static void R_WhiteImage( idImage *image ) {
	byte	data[DEFAULT_SIZE][DEFAULT_SIZE][4];

	// solid white texture
	memset( data, 255, sizeof( data ) );
	image->GenerateImage( (byte *)data, DEFAULT_SIZE, DEFAULT_SIZE, 
		TF_DEFAULT, TR_REPEAT, TD_DEFAULT );
}

static void R_BlackImage( idImage *image ) {
	byte	data[DEFAULT_SIZE][DEFAULT_SIZE][4];

	// solid black texture
	memset( data, 0, sizeof( data ) );
	image->GenerateImage( (byte *)data, DEFAULT_SIZE, DEFAULT_SIZE, 
		TF_DEFAULT, TR_REPEAT, TD_DEFAULT );
}

static void R_RGBA8Image( idImage *image ) {
	byte	data[DEFAULT_SIZE][DEFAULT_SIZE][4];

	memset( data, 0, sizeof( data ) );
	data[0][0][0] = 16;
	data[0][0][1] = 32;
	data[0][0][2] = 48;
	data[0][0][3] = 96;

	image->GenerateImage( (byte *)data, DEFAULT_SIZE, DEFAULT_SIZE, TF_DEFAULT, TR_REPEAT, TD_LOOKUP_TABLE_RGBA );
}

static void R_DepthImage( idImage *image ) {
	byte	data[DEFAULT_SIZE][DEFAULT_SIZE][4];

	memset( data, 0, sizeof( data ) );
	data[0][0][0] = 16;
	data[0][0][1] = 32;
	data[0][0][2] = 48;
	data[0][0][3] = 96;

	image->GenerateImage( (byte *)data, DEFAULT_SIZE, DEFAULT_SIZE, TF_NEAREST, TR_CLAMP, TD_DEPTH );
}

static void R_AlphaNotchImage( idImage *image ) {
	byte	data[2][4];

	// this is used for alpha test clip planes

	data[0][0] = data[0][1] = data[0][2] = 255;
	data[0][3] = 0;
	data[1][0] = data[1][1] = data[1][2] = 255;
	data[1][3] = 255;

	image->GenerateImage( (byte *)data, 2, 1, TF_NEAREST, TR_CLAMP, TD_LOOKUP_TABLE_ALPHA );
}

static void R_FlatNormalImage( idImage *image ) {
	byte	data[DEFAULT_SIZE][DEFAULT_SIZE][4];

	// flat normal map for default bunp mapping
	for ( int i = 0 ; i < 4 ; i++ ) {
		data[0][i][0] = 128;
		data[0][i][1] = 128;
		data[0][i][2] = 255;
		data[0][i][3] = 255;
	}
	image->GenerateImage( (byte *)data, 2, 2, TF_DEFAULT, TR_REPEAT, TD_BUMP );
}

/*
================
R_CreateNoFalloffImage

This is a solid white texture that is zero clamped.
================
*/
static void R_CreateNoFalloffImage( idImage *image ) {
	int		x,y;
	byte	data[16][FALLOFF_TEXTURE_SIZE][4];

	memset( data, 0, sizeof( data ) );
	for (x=1 ; x<FALLOFF_TEXTURE_SIZE-1 ; x++) {
		for (y=1 ; y<15 ; y++) {
			data[y][x][0] = 255;
			data[y][x][1] = 255;
			data[y][x][2] = 255;
			data[y][x][3] = 255;
		}
	}
	image->GenerateImage( (byte *)data, FALLOFF_TEXTURE_SIZE, 16, TF_DEFAULT, TR_CLAMP_TO_ZERO, TD_LOOKUP_TABLE_MONO );
}

/*
================
R_FogImage

We calculate distance correctly in two planes, but the
third will still be projection based
================
*/
const int	FOG_SIZE = 128;

void R_FogImage( idImage *image ) {
	int		x,y;
	byte	data[FOG_SIZE][FOG_SIZE][4];
	int		b;

	float	step[256];
	int		i;
	float	remaining = 1.0;
	for ( i = 0 ; i < 256 ; i++ ) {
		step[i] = remaining;
		remaining *= 0.982f;
	}

	for (x=0 ; x<FOG_SIZE ; x++) {
		for (y=0 ; y<FOG_SIZE ; y++) {
			float	d;

			d = idMath::Sqrt( (x - FOG_SIZE/2) * (x - FOG_SIZE/2) 
				+ (y - FOG_SIZE/2) * (y - FOG_SIZE / 2) );
			d /= FOG_SIZE/2-1;

			b = (byte)(d * 255);
			if ( b <= 0 ) {
				b = 0;
			} else if ( b > 255 ) {
				b = 255;
			}
			b = (byte)(255 * ( 1.0 - step[b] ));
			if ( x == 0 || x == FOG_SIZE-1 || y == 0 || y == FOG_SIZE-1 ) {
				b = 255;		// avoid clamping issues
			}
			data[y][x][0] =
			data[y][x][1] =
			data[y][x][2] = 255;
			data[y][x][3] = b;
		}
	}

	image->GenerateImage( (byte *)data, FOG_SIZE, FOG_SIZE, TF_LINEAR, TR_CLAMP, TD_LOOKUP_TABLE_ALPHA );
}


/*
================
FogFraction

Height values below zero are inside the fog volume
================
*/
static const float	RAMP_RANGE =	8;
static const float	DEEP_RANGE =	-30;
static float	FogFraction( float viewHeight, float targetHeight ) {
	float	total = idMath::Fabs( targetHeight - viewHeight );

//	return targetHeight >= 0 ? 0 : 1.0;

	// only ranges that cross the ramp range are special
	if ( targetHeight > 0 && viewHeight > 0 ) {
		return 0.0;
	}
	if ( targetHeight < -RAMP_RANGE && viewHeight < -RAMP_RANGE ) {
		return 1.0;
	}

	float	above;
	if ( targetHeight > 0 ) {
		above = targetHeight;
	} else if ( viewHeight > 0 ) {
		above = viewHeight;
	} else {
		above = 0;
	}

	float	rampTop, rampBottom;

	if ( viewHeight > targetHeight ) {
		rampTop = viewHeight;
		rampBottom = targetHeight;
	} else {
		rampTop = targetHeight;
		rampBottom = viewHeight;
	}
	if ( rampTop > 0 ) {
		rampTop = 0;
	}
	if ( rampBottom < -RAMP_RANGE ) {
		rampBottom = -RAMP_RANGE;
	}

	float	rampSlope = 1.0 / RAMP_RANGE;

	if ( !total ) {
		return -viewHeight * rampSlope;
	}

	float ramp = ( 1.0 - ( rampTop * rampSlope + rampBottom * rampSlope ) * -0.5 ) * ( rampTop - rampBottom );

	float	frac = ( total - above - ramp ) / total;

	// after it gets moderately deep, always use full value
	float deepest = viewHeight < targetHeight ? viewHeight : targetHeight;

	float	deepFrac = deepest / DEEP_RANGE;
	if ( deepFrac >= 1.0 ) {
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
void R_FogEnterImage( idImage *image ) {
	int		x,y;
	byte	data[FOG_ENTER_SIZE][FOG_ENTER_SIZE][4];
	int		b;

	for (x=0 ; x<FOG_ENTER_SIZE ; x++) {
		for (y=0 ; y<FOG_ENTER_SIZE ; y++) {
			float	d;

			d = FogFraction( x - (FOG_ENTER_SIZE / 2), y - (FOG_ENTER_SIZE / 2) );

			b = (byte)(d * 255);
			if ( b <= 0 ) {
				b = 0;
			} else if ( b > 255 ) {
				b = 255;
			}
			data[y][x][0] =
			data[y][x][1] =
			data[y][x][2] = 255;
			data[y][x][3] = b;
		}
	}

	// if mipmapped, acutely viewed surfaces fade wrong
	image->GenerateImage( (byte *)data, FOG_ENTER_SIZE, FOG_ENTER_SIZE, TF_LINEAR, TR_CLAMP, TD_LOOKUP_TABLE_ALPHA );
}


/*
================
R_QuadraticImage

================
*/
static const int	QUADRATIC_WIDTH = 32;
static const int	QUADRATIC_HEIGHT = 4;

void R_QuadraticImage( idImage *image ) {
	int		x,y;
	byte	data[QUADRATIC_HEIGHT][QUADRATIC_WIDTH][4];
	int		b;


	for (x=0 ; x<QUADRATIC_WIDTH ; x++) {
		for (y=0 ; y<QUADRATIC_HEIGHT ; y++) {
			float	d;

			d = x - (QUADRATIC_WIDTH/2 - 0.5);
			d = idMath::Fabs( d );
			d -= 0.5;
			d /= QUADRATIC_WIDTH/2;
		
			d = 1.0 - d;
			d = d * d;

			b = (byte)(d * 255);
			if ( b <= 0 ) {
				b = 0;
			} else if ( b > 255 ) {
				b = 255;
			}
			data[y][x][0] =
			data[y][x][1] =
			data[y][x][2] = b;
			data[y][x][3] = 255;
		}
	}

	image->GenerateImage( (byte *)data, QUADRATIC_WIDTH, QUADRATIC_HEIGHT, TF_DEFAULT, TR_CLAMP, TD_LOOKUP_TABLE_RGB1 );
}

/*
================
idImageManager::CreateIntrinsicImages
================
*/
void idImageManager::CreateIntrinsicImages() {
	// create built in images
	defaultImage = ImageFromFunction( "_default", R_DefaultImage );
	whiteImage = ImageFromFunction( "_white", R_WhiteImage );
	blackImage = ImageFromFunction( "_black", R_BlackImage );
	flatNormalMap = ImageFromFunction( "_flat", R_FlatNormalImage );
	alphaNotchImage = ImageFromFunction( "_alphaNotch", R_AlphaNotchImage );
	fogImage = ImageFromFunction( "_fog", R_FogImage );
	fogEnterImage = ImageFromFunction( "_fogEnter", R_FogEnterImage );
	noFalloffImage = ImageFromFunction( "_noFalloff", R_CreateNoFalloffImage );
	ImageFromFunction( "_quadratic", R_QuadraticImage );

	// scratchImage is used for screen wipes/doublevision etc..
	scratchImage = ImageFromFunction("_scratch", R_RGBA8Image );
	scratchImage2 = ImageFromFunction("_scratch2", R_RGBA8Image );
	accumImage = ImageFromFunction("_accum", R_RGBA8Image );
	currentRenderImage = ImageFromFunction("_currentRender", R_RGBA8Image );
	currentDepthImage = ImageFromFunction("_currentDepth", R_DepthImage );

	// save a copy of this for material comparison, because currentRenderImage may get
	// reassigned during stereo rendering
	originalCurrentRenderImage = currentRenderImage;

	loadingIconImage = ImageFromFile("textures/loadingicon2", TF_DEFAULT, TR_CLAMP, TD_DEFAULT, CF_2D );
	hellLoadingIconImage = ImageFromFile("textures/loadingicon3", TF_DEFAULT, TR_CLAMP, TD_DEFAULT, CF_2D );

	release_assert( loadingIconImage->referencedOutsideLevelLoad );
	release_assert( hellLoadingIconImage->referencedOutsideLevelLoad );
}