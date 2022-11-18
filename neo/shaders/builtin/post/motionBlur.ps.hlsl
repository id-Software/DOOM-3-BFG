/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2014-2022 Robert Beckebans

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

Texture2D t_ViewColor : register( t0 VK_DESCRIPTOR_SET( 0 ) );
Texture2D t_ViewDepth : register( t1 VK_DESCRIPTOR_SET( 0 ) );

SamplerState LinearSampler : register( s0 VK_DESCRIPTOR_SET( 1 ) );


struct PS_IN 
{
	float4 position : SV_Position;
	float2 texcoord0 : TEXCOORD0_centroid;
};

struct PS_OUT
{
	float4 color : SV_Target0;
};
// *INDENT-ON*

void main( PS_IN fragment, out PS_OUT result )
{
#if 0
	if( fragment.texcoord0.x < 0.5 )
	{
		// only draw on half the screen for comparison
		discard;
		return;
	}
#endif

	// don't motion blur the hands, which were drawn with alpha = 0
	if( t_ViewColor.Sample( LinearSampler, fragment.texcoord0 ).a == 0.0 )
	{
		discard;
		return;
	}

	// derive clip space from the depth buffer and screen position
	float windowZ = t_ViewDepth.Sample( LinearSampler, fragment.texcoord0 ).x;

	//float3 ndc = float3( fragment.texcoord0 * 2.0 - 1.0, windowZ * 2.0 - 1.0 );
	//float clipW = -rpProjectionMatrixZ.w / ( -rpProjectionMatrixZ.z - ndc.z );

	float3 ndc = float3( fragment.texcoord0.x * 2.0 - 1.0, 1.0 - fragment.texcoord0.y * 2.0, windowZ );
	float clipW = 1;

	float4 clip = float4( ndc * clipW, clipW );

	// convert from clip space this frame to clip space previous frame
	float4 prevClipPos;
	prevClipPos.x = dot( rpMVPmatrixX, clip );
	prevClipPos.y = dot( rpMVPmatrixY, clip );
	prevClipPos.z = dot( rpMVPmatrixZ, clip );
	prevClipPos.w = dot( rpMVPmatrixW, clip );

	if( prevClipPos.w <= 0 )
	{
		return;
	}

	// convert to NDC values
	float2 prevTexCoord;
	prevTexCoord.x = 0.5 + ( prevClipPos.x / prevClipPos.w ) * 0.5;
	prevTexCoord.y = 0.5 - ( prevClipPos.y / prevClipPos.w ) * 0.5;

	// sample along the line from prevTexCoord to fragment.texcoord0

	float2 delta = ( fragment.texcoord0 - prevTexCoord );

#if VECTORS_ONLY
	float2 prevWindowPos = prevTexCoord * rpWindowCoord.zw;

	float2 deltaPos = prevWindowPos - fragment.position.xy;
	float2 deltaUV = prevTexCoord - fragment.texcoord0;

	result.color = float4( deltaPos, 0.0, 1.0 );
#else
	float3 sum = _float3( 0.0 );
	float goodSamples = 0.0;
	float samples = rpOverbright.x;

	for( float i = 0.0 ; i < samples ; i = i + 1.0 )
	{
		float2 pos = fragment.texcoord0 + delta * ( ( i / ( samples - 1.0 ) ) - 0.5 );
		float4 color = t_ViewColor.Sample( LinearSampler, pos );
		// only take the values that are not part of the weapon
		sum += color.xyz * color.w;
		goodSamples += color.w;
	}
	float invScale = 1.0 / goodSamples;

	result.color = float4( sum * invScale, 1.0 );
#endif
}
