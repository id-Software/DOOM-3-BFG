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
#define FXAA_GREEN_AS_LUMA 1
#define FXAA_EARLY_EXIT 0
#include "Fxaa3_11.h"

uniform sampler2D samp0 : register(s0);
uniform sampler2D samp1 : register(s1); // exponent bias -1
uniform sampler2D samp2 : register(s2); // exponent bias -2

uniform float4 rpUser0 : register( c128 );
uniform float4 rpUser1 : register( c129 );
uniform float4 rpUser2 : register( c130 );
uniform float4 rpUser3 : register( c131 );
uniform float4 rpUser4 : register( c132 );
uniform float4 rpUser5 : register( c133 );
uniform float4 rpUser6 : register( c134 );

struct PS_IN {
	float4 position : VPOS;
	float2 texcoord0 : TEXCOORD0_centroid;
};

struct PS_OUT {	
	float4 color : COLOR;
};

void main( PS_IN fragment, out PS_OUT result ) {

	const float4 FXAAQualityRCPFrame = rpUser0;
	const float4 FXAAConsoleRcpFrameOpt = rpUser1;
	const float4 FXAAConsoleRcpFrameOpt2 = rpUser2;
	const float4 FXAAConsole360RcpFrameOpt2 = rpUser3;
	const float4 FXAAQualityParms = rpUser4;
	const float4 FXAAConsoleEdgeParms = rpUser5;
	const float4 FXAAConsole360ConstDir = rpUser6;

	// Inputs - see more info in fxaa3_11.hfile
	FxaaFloat2	fxaaPos = fragment.texcoord0;
	FxaaFloat4	fxaaConsolePos;

	float2 halfPixelOffset = float2( 0.5, 0.5 ) * FXAAQualityRCPFrame.xy;
	fxaaConsolePos.xy = fxaaPos.xy - ( halfPixelOffset );
	fxaaConsolePos.zw = fxaaPos.xy + ( halfPixelOffset );
	FxaaFloat2	fxaaQualityRcpFrame = FXAAQualityRCPFrame.xy;
	FxaaFloat4	fxaaConsoleRcpFrameOpt = FXAAConsoleRcpFrameOpt;
	FxaaFloat4	fxaaConsoleRcpFrameOpt2 = FXAAConsoleRcpFrameOpt2;
	FxaaFloat4	fxaaConsole360RcpFrameOpt2 = FXAAConsole360RcpFrameOpt2;

	// Quality parms
	FxaaFloat	fxaaQualitySubpix = FXAAQualityParms.x;
	FxaaFloat	fxaaQualityEdgeThreshold = FXAAQualityParms.y;
	FxaaFloat	fxaaQualityEdgeThresholdMin = FXAAQualityParms.z;

	// Console specific Parms
	FxaaFloat	fxaaConsoleEdgeSharpness = FXAAConsoleEdgeParms.x;
	FxaaFloat	fxaaConsoleEdgeThreshold = FXAAConsoleEdgeParms.y;
	FxaaFloat	fxaaConsoleEdgeThresholdMin = FXAAConsoleEdgeParms.z;

	// 360 specific parms these have to come from a constant register so that the compiler
	// does not unoptimize the shader
	FxaaFloat4	fxaaConsole360ConstDir = FXAAConsole360ConstDir;


	float4 colorSample = FxaaPixelShader(	fxaaPos,
											fxaaConsolePos,
											samp0,
											samp1,
											samp2,
											fxaaQualityRcpFrame,
											fxaaConsoleRcpFrameOpt,
											fxaaConsoleRcpFrameOpt2,
											fxaaConsole360RcpFrameOpt2,
											fxaaQualitySubpix,
											fxaaQualityEdgeThreshold,
											fxaaQualityEdgeThresholdMin,
											fxaaConsoleEdgeSharpness,
											fxaaConsoleEdgeThreshold,
											fxaaConsoleEdgeThresholdMin,
											fxaaConsole360ConstDir );
											
	result.color = colorSample;
}