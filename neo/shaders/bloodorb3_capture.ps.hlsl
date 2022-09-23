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

#include "global_inc.hlsl"


// *INDENT-OFF*
Texture2D t_Accum			: register( t0 VK_DESCRIPTOR_SET( 0 ) );
Texture2D t_CurrentRender	: register( t1 VK_DESCRIPTOR_SET( 0 ) );
Texture2D t_Mask			: register( t2 VK_DESCRIPTOR_SET( 0 ) );

SamplerState LinearSampler : register( s0 VK_DESCRIPTOR_SET( 1 ) );

struct PS_IN 
{
	float4 position : SV_Position;
	float2 texcoord0 : TEXCOORD0_centroid;
	float2 texcoord1 : TEXCOORD1_centroid;
	float2 texcoord2 : TEXCOORD2_centroid;
	float2 texcoord3 : TEXCOORD3_centroid;
	float2 texcoord4 : TEXCOORD4;
};

struct PS_OUT {
	float4 color : SV_Target;
};
// *INDENT-ON*

void main( PS_IN fragment, out PS_OUT result )
{
	float colorFactor = fragment.texcoord4.x;

	float4 color0 = float4( 1.0f - colorFactor,	1.0f - colorFactor,		1.0f,	1.0f );
	float4 color1 = float4( 1.0f,	0.95f - colorFactor,		0.95f,	0.5f );
	float4 color2 = float4( 0.015f,	0.015f, 0.015f, 0.01f );

	float4 accumSample0 = t_Accum.Sample( LinearSampler, fragment.texcoord0 ) * color0;
	float4 accumSample1 = t_Accum.Sample( LinearSampler, fragment.texcoord1 ) * color1;
	float4 accumSample2 = t_Accum.Sample( LinearSampler, fragment.texcoord2 ) * color2;
	float4 maskSample = t_Mask.Sample( LinearSampler, fragment.texcoord3 );

	float4 tint = float4( 0.8, 0.5, 0.5, 1 );
	float4 currentRenderSample = t_CurrentRender.Sample( LinearSampler, fragment.texcoord3 ) * tint;

	// blend of the first 2 accumulation samples
	float4 accumColor = lerp( accumSample0, accumSample1, 0.5f );
	// add thrid sample
	accumColor += accumSample2;

	accumColor = lerp( accumColor, currentRenderSample, maskSample.a );
	result.color = accumColor;
}