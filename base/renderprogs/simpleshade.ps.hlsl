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

struct PS_IN {
	float4 position		: VPOS;
	float4 texcoord0	: TEXCOORD0_centroid;
};

struct PS_OUT {
	float4 color : COLOR;
};

uniform sampler2D	samp0 : register(s0);

static float2 screenPosToTexcoord( float2 pos, float4 bias_scale ) { return ( pos * bias_scale.zw + bias_scale.xy ); }

void main( PS_IN fragment, out PS_OUT result ) {
	const float renderWidth = 1280.0f;
	const float renderHeight = 720.0f;
	const float4 positionToViewTexture = float4( 0.5f / renderWidth, 0.5f / renderHeight, 1.0f / renderWidth, 1.0f / renderHeight );
	
	float interpolatedZOverW = ( 1.0 - ( fragment.texcoord0.z / fragment.texcoord0.w ) );

	float3 pos;
	pos.z = 1.0 / interpolatedZOverW;
	pos.xy = pos.z * ( 2.0 * screenPosToTexcoord( fragment.position.xy, positionToViewTexture ) - 1.0 );

	float3 normal = normalize( cross( ddy( pos ), ddx( pos ) ) );

	// light is above and to the right in the eye plane
	float3 L = normalize( float3( 1.0, 1.0, 0.0 ) - pos );

	result.color.xyz = _float3( dot3( normal, L ) * 0.75 );
	result.color.w = 1.0;
}