/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2014-2015 Robert Beckebans

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
Texture2D t_CurrentRender : register( t0 VK_DESCRIPTOR_SET( 1 ) );
SamplerState samp0 : register( s0 VK_DESCRIPTOR_SET( 2 ) );

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

float linterp( float t )
{
	return saturate( 1.0 - abs( 2.0 * t - 1.0 ) );
}

float remap( float t, float a, float b )
{
	return saturate( ( t - a ) / ( b - a ) );
}

float3 spectrumoffset( float t )
{
	float lo = step( t, 0.5 );
	float hi = 1.0 - lo;
	float w = linterp( remap( t, 1.0 / 6.0, 5.0 / 6.0 ) );
	float3 ret = float3( lo, 1.0, hi ) * float3( 1.0 - w, w, 1.0 - w );

	return ret; // pow( ret, float3( 1.0 / 2.2 ) );
}

void main( PS_IN fragment, out PS_OUT result )
{
	float2 st = fragment.texcoord0;

	// base color with tone mapping applied
	float4 color = t_CurrentRender.Sample( samp0, st );

	const float gaussFact[9] = { 0.13298076, 0.12579441, 0.10648267, 0.08065691, 0.05467002, 0.03315905, 0.01799699, 0.00874063, 0.00379866 };

	const float3 chromaticOffsets[9] =
	{
		float3( 0.5, 0.5, 0.5 ), // w
		float3( 0.8, 0.3, 0.3 ),
		//	float3(1.0, 0.2, 0.2), // r
		float3( 0.5, 0.2, 0.8 ),
		float3( 0.2, 0.2, 1.0 ), // b
		float3( 0.2, 0.3, 0.9 ),
		float3( 0.2, 0.9, 0.2 ), // g
		float3( 0.3, 0.5, 0.3 ),
		float3( 0.3, 0.5, 0.3 ),
		float3( 0.3, 0.5, 0.3 )
		//float3(0.3, 0.5, 0.3)
	};

	float3 sumColor = _float3( 0.0 );
	float3 sumSpectrum = _float3( 0.0 );

	const int tap = 4;
	const int samples = 9;

	float scale = 13.0; // bloom width
	const float weightScale = 2.3; // bloom strength

	for( int i = 0; i < samples; i++ )
	{
		//float t = ( ( float( 4 + ( i ) ) ) / ( float( samples ) - 1.0 ) );
		//float t = log2( float( i ) / ( float( samples ) - 1.0 ) );
		//float t = ( float( i ) / ( float( samples ) - 1.0 ) );

		//float3 so = spectrumoffset( t );
		float3 so = chromaticOffsets[ i ];
		float4 color = t_CurrentRender.Sample( samp0, st + float2( float( i ), 0 ) * rpWindowCoord.xy * scale );

		float weight = gaussFact[ i ];
		sumColor += color.rgb * ( so.rgb * weight * weightScale );
	}

#if 1
	for( int sI = 1; sI < samples; sI++ )
	{
		//float t = ( ( float( 4 + ( i ) ) ) / ( float( samples ) - 1.0 ) );

		//float3 so = spectrumoffset( t );
		float3 so = chromaticOffsets[sI];
		float4 color = t_CurrentRender.Sample( samp0, st + float2( float( -sI ), 0 ) * rpWindowCoord.xy * scale );

		float weight = gaussFact[sI];
		sumColor += color.rgb * ( so.rgb * weight * weightScale );
	}
#endif

	result.color = float4( sumColor, 1.0 );
	//result.color = float4( sumColor / float(samples), 1.0 );
	//result.color = float4( sumColor / sumSpectrum, 1.0 );
}
