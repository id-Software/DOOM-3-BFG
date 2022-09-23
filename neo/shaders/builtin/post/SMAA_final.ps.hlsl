/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2015 Robert Beckebans

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


#define SMAA_INCLUDE_VS 0
#define SMAA_INCLUDE_PS 1
#define SMAA_HLSL_4_1 1
#include "SMAA.inc.hlsl"

// *INDENT-OFF*
Texture2D t_CurrentColor	: register( t0 VK_DESCRIPTOR_SET( 1 ) );
Texture2D t_SmaaBlend		: register( t1 VK_DESCRIPTOR_SET( 1 ) );
//Texture2D t_Velocity		: register( t2 VK_DESCRIPTOR_SET( 1 ) );

SamplerState LinearSampler	: register( s0 VK_DESCRIPTOR_SET( 2 ) );
SamplerState PointSampler	: register( s1 VK_DESCRIPTOR_SET( 2 ) );

struct PS_IN
{
	float4 position		: VPOS;
	float2 texcoord0	: TEXCOORD0_centroid;
	float4 texcoord1	: TEXCOORD1_centroid;
};

struct PS_OUT
{
	float4 color : SV_Target0;
};
// *INDENT-ON*


void main( PS_IN fragment, out PS_OUT result )
{
	float2 texcoord = fragment.texcoord0;

	float4 offset = fragment.texcoord1;

	Init( LinearSampler, PointSampler );

	float4 color = SMAANeighborhoodBlendingPS( texcoord,
				   offset,
				   t_CurrentColor,
				   t_SmaaBlend
#if SMAA_REPROJECTION
				   , SMAATexture2D( velocityTex )
#endif
											 );

	//color = tex2D( samp1, texcoord );
	//color = float4( samp1 );
	result.color = color;
}
