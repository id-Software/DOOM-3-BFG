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
#if USE_GPU_SKINNING
StructuredBuffer<float4> matrices : register( t11 );
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
	float2 texcoord0	: TEXCOORD0_centroid;
	float3 texcoord1	: TEXCOORD1_centroid;
	float3 texcoord2	: TEXCOORD2_centroid;
	float3 texcoord3	: TEXCOORD3_centroid;
	float3 texcoord4	: TEXCOORD4_centroid;
	float4 color		: COLOR0;
};
// *INDENT-ON*

void main( VS_IN vertex, out VS_OUT result )
{
	float4 vNormal = vertex.normal * 2.0 - 1.0;
	float4 vTangent = vertex.tangent * 2.0 - 1.0;
	float3 vBitangent = cross( vNormal.xyz, vTangent.xyz ) * vTangent.w;

#if USE_GPU_SKINNING
	//--------------------------------------------------------------
	// GPU transformation of the normal / tangent / bitangent
	//
	// multiplying with 255.1 give us the same result and is faster than floor( w * 255 + 0.5 )
	//--------------------------------------------------------------
	const float w0 = vertex.color2.x;
	const float w1 = vertex.color2.y;
	const float w2 = vertex.color2.z;
	const float w3 = vertex.color2.w;

	float4 matX, matY, matZ;	// must be float4 for vec4
	int joint = int( vertex.color.x * 255.1 * 3.0 );
	matX = matrices[int( joint + 0 )] * w0;
	matY = matrices[int( joint + 1 )] * w0;
	matZ = matrices[int( joint + 2 )] * w0;

	joint = int( vertex.color.y * 255.1 * 3.0 );
	matX += matrices[int( joint + 0 )] * w1;
	matY += matrices[int( joint + 1 )] * w1;
	matZ += matrices[int( joint + 2 )] * w1;

	joint = int( vertex.color.z * 255.1 * 3.0 );
	matX += matrices[int( joint + 0 )] * w2;
	matY += matrices[int( joint + 1 )] * w2;
	matZ += matrices[int( joint + 2 )] * w2;

	joint = int( vertex.color.w * 255.1 * 3.0 );
	matX += matrices[int( joint + 0 )] * w3;
	matY += matrices[int( joint + 1 )] * w3;
	matZ += matrices[int( joint + 2 )] * w3;

	float3 normal;
	normal.x = dot3( matX, vNormal );
	normal.y = dot3( matY, vNormal );
	normal.z = dot3( matZ, vNormal );
	normal = normalize( normal );

	float3 tangent;
	tangent.x = dot3( matX, vTangent );
	tangent.y = dot3( matY, vTangent );
	tangent.z = dot3( matZ, vTangent );
	tangent = normalize( tangent );

	float3 bitangent;
	bitangent.x = dot3( matX, vBitangent );
	bitangent.y = dot3( matY, vBitangent );
	bitangent.z = dot3( matZ, vBitangent );
	bitangent = normalize( bitangent );

	float4 modelPosition;
	modelPosition.x = dot4( matX, vertex.position );
	modelPosition.y = dot4( matY, vertex.position );
	modelPosition.z = dot4( matZ, vertex.position );
	modelPosition.w = 1.0;

#else
	float4 modelPosition = vertex.position;
	float3 normal = vNormal.xyz;
	float3 tangent = vTangent.xyz;
	float3 bitangent = vBitangent.xyz;
#endif

	result.position.x = dot4( modelPosition, rpMVPmatrixX );
	result.position.y = dot4( modelPosition, rpMVPmatrixY );
	result.position.z = dot4( modelPosition, rpMVPmatrixZ );
	result.position.w = dot4( modelPosition, rpMVPmatrixW );

	// textures 0 takes the base coordinates by the texture matrix
	result.texcoord0.x = dot4( vertex.texcoord.xy, rpBumpMatrixS );
	result.texcoord0.y = dot4( vertex.texcoord.xy, rpBumpMatrixT );

	//float4 toEye = rpLocalViewOrigin - modelPosition;
	//result.texcoord1.x = dot3( toEye, rpModelMatrixX );
	//result.texcoord1.y = dot3( toEye, rpModelMatrixY );
	//result.texcoord1.z = dot3( toEye, rpModelMatrixZ );

#if 0
	// rotate into world space
	result.texcoord2.x = dot3( tangent, rpModelMatrixX );
	result.texcoord3.x = dot3( tangent, rpModelMatrixY );
	result.texcoord4.x = dot3( tangent, rpModelMatrixZ );

	result.texcoord2.y = dot3( bitangent, rpModelMatrixX );
	result.texcoord3.y = dot3( bitangent, rpModelMatrixY );
	result.texcoord4.y = dot3( bitangent, rpModelMatrixZ );

	result.texcoord2.z = dot3( normal, rpModelMatrixX );
	result.texcoord3.z = dot3( normal, rpModelMatrixY );
	result.texcoord4.z = dot3( normal, rpModelMatrixZ );

#else
	// rotate into view space
	result.texcoord2.x = dot3( tangent, rpModelViewMatrixX );
	result.texcoord3.x = dot3( tangent, rpModelViewMatrixY );
	result.texcoord4.x = dot3( tangent, rpModelViewMatrixZ );

	result.texcoord2.y = dot3( bitangent, rpModelViewMatrixX );
	result.texcoord3.y = dot3( bitangent, rpModelViewMatrixY );
	result.texcoord4.y = dot3( bitangent, rpModelViewMatrixZ );

	result.texcoord2.z = dot3( normal, rpModelViewMatrixX );
	result.texcoord3.z = dot3( normal, rpModelViewMatrixY );
	result.texcoord4.z = dot3( normal, rpModelViewMatrixZ );
#endif

#if USE_GPU_SKINNING
	// for joint transformation of the tangent space, we use color and
	// color2 for weighting information, so hopefully there aren't any
	// effects that need vertex color...
	result.color = float4( 1.0f, 1.0f, 1.0f, 1.0f );
#else
	//# generate the vertex color, which can be 1.0, color, or 1.0 - color
	//# for 1.0 : env[16] = 0, env[17] = 1
	//# for color : env[16] = 1, env[17] = 0
	//# for 1.0-color : env[16] = -1, env[17] = 1
	result.color = ( swizzleColor( vertex.color ) * rpVertexColorModulate ) + rpVertexColorAdd;
#endif
}