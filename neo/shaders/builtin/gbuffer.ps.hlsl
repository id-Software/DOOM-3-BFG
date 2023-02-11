/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2016 Robert Beckebans

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
Texture2D	 t_NormalMap	: register( t0 VK_DESCRIPTOR_SET( 1 ) );
SamplerState s_Sampler		: register( s0 VK_DESCRIPTOR_SET( 2 ) );

struct PS_IN
{
	float4 position		: SV_Position;
	float2 texcoord0	: TEXCOORD0_centroid;
	float3 texcoord1	: TEXCOORD1_centroid;
	float3 texcoord2	: TEXCOORD2_centroid;
	float3 texcoord3	: TEXCOORD3_centroid;
	float3 texcoord4	: TEXCOORD4_centroid;
	float4 color		: COLOR0;
};

struct PS_OUT
{
	float4 color : SV_Target0;
};
// *INDENT-ON*

void main( PS_IN fragment, out PS_OUT result )
{
	float4 bump = t_NormalMap.Sample( s_Sampler, fragment.texcoord0 ) * 2.0f - 1.0f;

	// RB begin
	float3 localNormal;
#if USE_NORMAL_FMT_RGB8
	localNormal = float3( bump.rg, 0.0f );
#else
	localNormal = float3( bump.wy, 0.0f );
#endif
	// RB end
	localNormal.z = sqrt( 1.0f - dot3( localNormal, localNormal ) );

	float3 globalNormal;

#if 1
	// rotate normal into view space
	globalNormal.x = dot3( localNormal, fragment.texcoord2 );
	globalNormal.y = dot3( localNormal, fragment.texcoord3 );
	globalNormal.z = dot3( localNormal, fragment.texcoord4 );
#else
	// only the normal in view space
	globalNormal.x = fragment.texcoord2.z;
	globalNormal.y = fragment.texcoord3.z;
	globalNormal.z = fragment.texcoord4.z;
	//globalNormal.z = fragment.texcoord4.z * -0.001; //sqrt( abs( dot( globalNormal.xy, globalNormal.xy ) - 0.25 ) );
	globalNormal = normalize( globalNormal );
#endif

	// RB: rpColor is white and only used to generate the _fa_ uniform array
	result.color.rgb = ( globalNormal.xyz * 0.5 + 0.5 ) * fragment.color.rgb;// * rpColor;
	result.color.a = 1.0;
}
