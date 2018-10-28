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

struct VS_IN {
	float4 position : POSITION;
	float2 texcoord : TEXCOORD0;
	float4 normal : NORMAL;
	float4 tangent : TANGENT;
	float4 color : COLOR0;
};

struct VS_OUT {
	float4 position		: POSITION;
	float4 texcoord1	: TEXCOORD1;
	float4 texcoord2	: TEXCOORD2;
	float4 texcoord3	: TEXCOORD3;
	float4 texcoord4	: TEXCOORD4;
	float4 texcoord5	: TEXCOORD5;
	float4 texcoord6	: TEXCOORD6;
	float4 color		: COLOR0;
};

void main( VS_IN vertex, out VS_OUT result ) {

	float4 normal = vertex.normal * 2.0 - 1.0;
	float4 tangent = vertex.tangent * 2.0 - 1.0;
	float3 binormal = cross( normal.xyz, tangent.xyz ) * tangent.w;

	result.position.x = dot4( vertex.position, rpMVPmatrixX );
	result.position.y = dot4( vertex.position, rpMVPmatrixY );
	result.position.z = dot4( vertex.position, rpMVPmatrixZ );
	result.position.w = dot4( vertex.position, rpMVPmatrixW );

	float4 defaultTexCoord = float4( 0.0f, 0.5f, 0.0f, 1.0f );

	//calculate vector to light in R0
	float4 toLight = rpLocalLightOrigin - vertex.position;

	//textures 1 takes the base coordinates by the texture matrix
	result.texcoord1 = defaultTexCoord;
	result.texcoord1.x = dot4( vertex.texcoord.xy, rpBumpMatrixS );
	result.texcoord1.y = dot4( vertex.texcoord.xy, rpBumpMatrixT );

	//# texture 2 has one texgen
	result.texcoord2 = defaultTexCoord;
	result.texcoord2.x = dot4( vertex.position, rpLightFalloffS );

	//# texture 3 has three texgens
	result.texcoord3.x = dot4( vertex.position, rpLightProjectionS );
	result.texcoord3.y = dot4( vertex.position, rpLightProjectionT );
	result.texcoord3.z = 0.0f;
	result.texcoord3.w = dot4( vertex.position, rpLightProjectionQ );

	//# textures 4 takes the base coordinates by the texture matrix
	result.texcoord4 = defaultTexCoord;
	result.texcoord4.x = dot4( vertex.texcoord.xy, rpDiffuseMatrixS );
	result.texcoord4.y = dot4( vertex.texcoord.xy, rpDiffuseMatrixT );

	//# textures 5 takes the base coordinates by the texture matrix
	result.texcoord5 = defaultTexCoord;
	result.texcoord5.x = dot4( vertex.texcoord.xy, rpSpecularMatrixS );
	result.texcoord5.y = dot4( vertex.texcoord.xy, rpSpecularMatrixT );

	//# texture 6's texcoords will be the halfangle in texture space

	//# calculate normalized vector to light in R0
	toLight = normalize( toLight );

	//# calculate normalized vector to viewer in R1
	float4 toView = normalize( rpLocalViewOrigin - vertex.position );
	
	//# add together to become the half angle vector in object space (non-normalized)
	float4 halfAngleVector = toLight + toView;

	//# put into texture space
	result.texcoord6.x = dot3( tangent, halfAngleVector );
	result.texcoord6.y = dot3( binormal, halfAngleVector );
	result.texcoord6.z = dot3( normal, halfAngleVector );
	result.texcoord6.w = 1.0f;

	//# generate the vertex color, which can be 1.0, color, or 1.0 - color
	//# for 1.0 : env[16] = 0, env[17] = 1
	//# for color : env[16] = 1, env[17] = 0
	//# for 1.0-color : env[16] = -1, env[17] = 1	
	result.color = ( swizzleColor( vertex.color ) * rpVertexColorModulate ) + rpVertexColorAdd;
}