/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2021 Robert Beckebans

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

#include "renderprogs/global.inc.hlsl"


// *INDENT-OFF*
uniform sampler2D	samp0 : register(s0); // texture 0 is octahedron cube map atlas

struct PS_IN {
	float4 position		: VPOS;
	float3 texcoord0	: TEXCOORD0_centroid;
	float3 texcoord1	: TEXCOORD1_centroid;
	float4 color		: COLOR0;
};

struct PS_OUT {
	float4 color : COLOR;
};
// *INDENT-ON*


int3 GetBaseGridCoord( float3 origin )
{
	float3 lightGridOrigin = rpGlobalLightOrigin.xyz;
	float3 lightGridSize = rpJitterTexScale.xyz;
	int3 lightGridBounds = int3( rpJitterTexOffset.x, rpJitterTexOffset.y, rpJitterTexOffset.z );

	int3 pos;

	float3 lightOrigin = origin - lightGridOrigin;
	for( int i = 0; i < 3; i++ )
	{
		float           v;

		v = lightOrigin[i] * ( 1.0f / lightGridSize[i] );
		pos[i] = int( floor( v ) );

		if( pos[i] < 0 )
		{
			pos[i] = 0;
		}
		else if( pos[i] >= lightGridBounds[i] - 1 )
		{
			pos[i] = lightGridBounds[i] - 1;
		}
	}

	return pos;
}

void main( PS_IN fragment, out PS_OUT result )
{
	const int LIGHTGRID_IRRADIANCE_SIZE	= 32;

	float3 globalPosition = fragment.texcoord0.xyz;
	float3 globalNormal = normalize( fragment.texcoord1 );

	float2 normalizedOctCoord = octEncode( globalNormal );
	float2 normalizedOctCoordZeroOne = ( normalizedOctCoord + float2( 1.0 ) ) * 0.5;

	float3 lightGridOrigin = rpGlobalLightOrigin.xyz;
	float3 lightGridSize = rpJitterTexScale.xyz;
	int3 lightGridBounds = int3( rpJitterTexOffset.x, rpJitterTexOffset.y, rpJitterTexOffset.z );

	float invXZ = ( 1.0 / ( lightGridBounds[0] * lightGridBounds[2] ) );
	float invY = ( 1.0 / lightGridBounds[1] );

	normalizedOctCoordZeroOne.x *= invXZ;
	normalizedOctCoordZeroOne.y *= invY;

	int3 gridStep;

	gridStep[0] = 1;
	gridStep[1] = lightGridBounds[0];
	gridStep[2] = lightGridBounds[0] * lightGridBounds[1];

	int3 gridCoord = GetBaseGridCoord( globalPosition );

	normalizedOctCoordZeroOne.x += ( gridCoord[0] * gridStep[0] + gridCoord[2] * gridStep[1] ) * invXZ;
	normalizedOctCoordZeroOne.y += ( gridCoord[1] * invY );

	// offset by one pixel border bleed size for linear filtering
#if 1
	// rpScreenCorrectionFactor.x = probeSize - borderSize, e.g. ( 18 - 2 ) = 16
	// rpScreenCorrectionFactor.y = probeSize including border, e.g = 18
	// rpScreenCorrectionFactor.z = borderSize e.g = 2
	// rpScreenCorrectionFactor.w = probeSize factor accounting account offset border, e.g = ( 16 / 18 ) = 0.8888
	float2 octCoordNormalizedToTextureDimensions = normalizedOctCoordZeroOne * rpScreenCorrectionFactor.w;

	float2 probeTopLeftPosition;
	probeTopLeftPosition.x = ( gridCoord[0] * gridStep[0] + gridCoord[2] * gridStep[1] ) * rpScreenCorrectionFactor.z + rpScreenCorrectionFactor.z * 0.5;
	probeTopLeftPosition.y = ( gridCoord[1] ) * rpScreenCorrectionFactor.z + rpScreenCorrectionFactor.z * 0.5;

	float2 normalizedProbeTopLeftPosition = probeTopLeftPosition * rpCascadeDistances.zw;

	normalizedOctCoordZeroOne.xy = normalizedProbeTopLeftPosition + octCoordNormalizedToTextureDimensions;
#endif

	float4 envMap = texture( samp0, normalizedOctCoordZeroOne, 0 );

	result.color = float4( envMap.xyz, 1.0f ) * 1.0 * fragment.color;
}
