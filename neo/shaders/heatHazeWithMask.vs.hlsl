/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013 Robert Beckebans

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

#include "global_inc.hlsl"


// User Renderparms start at 128 as per renderprogs.h

// *INDENT-OFF*
#if USE_GPU_SKINNING
StructuredBuffer<float4> matrices : register(t11);
#endif

struct VS_IN
{
	float4 position	: POSITION;
	float2 texcoord	: TEXCOORD0;
	float4 normal	: NORMAL;
	float4 tangent	: TANGENT;
	float4 color	: COLOR0;
	float4 color2	: COLOR1;
};

struct VS_OUT
{
	float4 position		: SV_Position;
	float4 texcoord0	: TEXCOORD0_centroid;
	float4 texcoord1	: TEXCOORD1_centroid;
	float4 texcoord2	: TEXCOORD2_centroid;
};
// *INDENT-ON*

void main( VS_IN vertex, out VS_OUT result )
{

#include "skinning.inc.hlsl"


	// texture 0 takes the texture coordinates unmodified
	result.texcoord0 = float4( vertex.texcoord.xy, 0, 0 );

	// texture 1 takes the texture coordinates and adds a scroll
	const float4 textureScroll = rpUser0;
	result.texcoord1 = float4( vertex.texcoord.xy, 0, 0 ) + textureScroll;

	// texture 2 takes the deform magnitude and scales it by the projection distance
	float4 vec = float4( 0, 1, 0, 1 );
	vec.z  = dot4( modelPosition, rpModelViewMatrixZ );

	// magicProjectionAdjust is a magic scalar that scales the projection since we changed from
	// using the X axis to the Y axis to calculate R1.  It is an approximation to closely match
	// what the original game did
	const float magicProjectionAdjust = 0.43f;
	float x = dot4( vec, rpProjectionMatrixY ) * magicProjectionAdjust;
	float w = dot4( vec, rpProjectionMatrixW );

	// don't let the recip get near zero for polygons that cross the view plane
	w = max( w, 1.0 );
	x /= w;
	//x = x * ( 1.0f / ( w + 0.00001f ) );

	// clamp the distance so the the deformations don't get too wacky near the view
	x = min( x, 0.02 );

	const float4 deformMagnitude = rpUser1;
	result.texcoord2 = x * deformMagnitude;
}