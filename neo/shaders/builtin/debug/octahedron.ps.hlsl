/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2020 Robert Beckebans

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


// *INDENT-OFF*
Texture2D t_CubeMap : register( t0 VK_DESCRIPTOR_SET( 1 ) );
SamplerState samp0 : register( s0 VK_DESCRIPTOR_SET( 2 ) ); // texture 0 is octahedron cube map


struct PS_IN {
	float4 position		: SV_Position;
	float3 texcoord0	: TEXCOORD0_centroid;
	float3 texcoord1	: TEXCOORD1_centroid;
	float4 color		: COLOR0;
};

struct PS_OUT {
	float4 color : SV_Target0;
};
// *INDENT-ON*

void main( PS_IN fragment, out PS_OUT result )
{

	float3 globalNormal = normalize( fragment.texcoord1 );
	float3 globalEye = normalize( fragment.texcoord0 );

	float3 reflectionVector = _float3( dot3( globalEye, globalNormal ) );
	reflectionVector *= globalNormal;
	reflectionVector = ( reflectionVector * 2.0f ) - globalEye;

	float2 normalizedOctCoord = octEncode( reflectionVector );
	float2 normalizedOctCoordZeroOne = ( normalizedOctCoord + _float2( 1.0 ) ) * 0.5;

	// offset by one pixel border bleed size for linear filtering
#if 0
	float2 octCoordNormalizedToTextureDimensions = ( normalizedOctCoordZeroOne * ( rpCascadeDistances.x - float( 2.0 ) ) ) / rpCascadeDistances.xy;

	float2 probeTopLeftPosition = float2( 1.0, 1.0 );
	float2 normalizedProbeTopLeftPosition = probeTopLeftPosition * rpCascadeDistances.zw;

	normalizedOctCoordZeroOne.xy = normalizedProbeTopLeftPosition + octCoordNormalizedToTextureDimensions;
#endif

	//normalizedOctCoordZeroOne = TextureCoordFromDirection( reflectionVector, 0, int( rpCascadeDistances.x ), int( rpCascadeDistances.y ), int( rpCascadeDistances.x ) - 2 );

	float4 envMap = t_CubeMap.SampleLevel( samp0, normalizedOctCoordZeroOne, 0 );

	result.color = float4( envMap.xyz, 1.0f ) * fragment.color * 1.0;
}
