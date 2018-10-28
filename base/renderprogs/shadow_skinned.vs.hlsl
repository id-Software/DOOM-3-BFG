/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company. 
Coypright (C) 2014 Robert Beckebans

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

uniform matrices_ubo { float4 matrices[408]; };

struct VS_IN {
	float4 position : POSITION;
	float4 color : COLOR0;
	float4 color2 : COLOR1;
};

struct VS_OUT {
	float4 position : POSITION;
};

void main( VS_IN vertex, out VS_OUT result ) {
	//--------------------------------------------------------------
	// GPU transformation of the normal / binormal / bitangent
	//
	// multiplying with 255.1 give us the same result and is faster than floor( w * 255 + 0.5 )
	//--------------------------------------------------------------
	const float w0 = vertex.color2.x;
	const float w1 = vertex.color2.y;
	const float w2 = vertex.color2.z;
	const float w3 = vertex.color2.w;

	float4 matX, matY, matZ;	// must be float4 for vec4
	int joint = int(vertex.color.x * 255.1 * 3.0);
	matX = matrices[int(joint+0)] * w0;
	matY = matrices[int(joint+1)] * w0;
	matZ = matrices[int(joint+2)] * w0;

	joint = int(vertex.color.y * 255.1 * 3.0);
	matX += matrices[int(joint+0)] * w1;
	matY += matrices[int(joint+1)] * w1;
	matZ += matrices[int(joint+2)] * w1;

	joint = int(vertex.color.z * 255.1 * 3.0);
	matX += matrices[int(joint+0)] * w2;
	matY += matrices[int(joint+1)] * w2;
	matZ += matrices[int(joint+2)] * w2;

	joint = int(vertex.color.w * 255.1 * 3.0);
	matX += matrices[int(joint+0)] * w3;
	matY += matrices[int(joint+1)] * w3;
	matZ += matrices[int(joint+2)] * w3;

	float4 vertexPosition = vertex.position;
	vertexPosition.w = 1.0;

	float4 modelPosition;
	modelPosition.x = dot4( matX, vertexPosition );
	modelPosition.y = dot4( matY, vertexPosition );
	modelPosition.z = dot4( matZ, vertexPosition );
	modelPosition.w = vertex.position.w;

	float4 vPos = modelPosition - rpLocalLightOrigin;
	vPos = ( vPos.wwww * rpLocalLightOrigin ) + vPos;

	result.position.x = dot4( vPos, rpMVPmatrixX );
	result.position.y = dot4( vPos, rpMVPmatrixY );
	result.position.z = dot4( vPos, rpMVPmatrixZ );
	result.position.w = dot4( vPos, rpMVPmatrixW );
}