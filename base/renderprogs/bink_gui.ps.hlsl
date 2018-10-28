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

#include "global.inc.hlsl"

uniform sampler2D samp0 : register(s0); // Y
uniform sampler2D samp1 : register(s1); // Cr
uniform sampler2D samp2 : register(s2); // Cb

struct PS_IN {
	float4 position : VPOS;
	float2 texcoord0 : TEXCOORD0_centroid;
	float4 texcoord1 : TEXCOORD1_centroid;
	float4 color : COLOR0;
};

struct PS_OUT {
	float4 color : COLOR;
};

void main( PS_IN fragment, out PS_OUT result ) {
	const float3 crc = float3( 1.595794678, -0.813476563, 0 );
	const float3 crb = float3( 0, -0.391448975, 2.017822266 );
	const float3 adj = float3( -0.87065506, 0.529705048f, -1.081668854f );
	const float3 YScalar = float3( 1.164123535f, 1.164123535f, 1.164123535f );

	float Y = tex2D( samp0, fragment.texcoord0.xy ).x;
	float Cr = tex2D( samp1, fragment.texcoord0.xy ).x;
	float Cb = tex2D( samp2, fragment.texcoord0.xy ).x;

	float3 p = ( YScalar * Y );
	p += ( crc * Cr ) + ( crb * Cb ) + adj;

	float4 binkImage;
	binkImage.xyz = p;
	binkImage.w = 1.0;

	float4 color = ( binkImage * fragment.color ) + fragment.texcoord1;
	result.color.xyz = color.xyz * color.w;
	result.color.w = color.w;
}