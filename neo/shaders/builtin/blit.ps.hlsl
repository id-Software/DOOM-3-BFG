/*
* Copyright (c) 2014-2021, NVIDIA CORPORATION. All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
* DEALINGS IN THE SOFTWARE.
*/

// *INDENT-OFF*
#if TEXTURE_ARRAY
Texture2DArray tex : register( t0 );
#else
Texture2D tex : register( t0 );
#endif
SamplerState samp : register( s0 );

struct PS_IN
{
	float4 posClip	: SV_Position;
	float2 uv		: UV;
};
// *INDENT-ON*

void main(
	PS_IN fragment,
	out float4 o_rgba : SV_Target )
{
#if TEXTURE_ARRAY
	o_rgba = tex.Sample( samp, float3( fragment.uv, 0 ) );
#else
	o_rgba = tex.Sample( samp, fragment.uv );
#endif
}
