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
#ifndef __COLORSPACE_H__
#define __COLORSPACE_H__

/*
================================================================================================
Contains the ColorSpace conversion declarations.
================================================================================================
*/

namespace idColorSpace
{
void	ConvertRGBToYCoCg( byte* dst, const byte* src, int width, int height );
void	ConvertYCoCgToRGB( byte* dst, const byte* src, int width, int height );

void	ConvertRGBToCoCg_Y( byte* dst, const byte* src, int width, int height );
void	ConvertCoCg_YToRGB( byte* dst, const byte* src, int width, int height );
void	ConvertCoCgSYToRGB( byte* dst, const byte* src, int width, int height );

void	ConvertRGBToYCoCg420( byte* dst, const byte* src, int width, int height );
void	ConvertYCoCg420ToRGB( byte* dst, const byte* src, int width, int height );

void	ConvertRGBToYCbCr( byte* dst, const byte* src, int width, int height );
void	ConvertYCbCrToRGB( byte* dst, const byte* src, int width, int height );

void	ConvertRGBToCbCr_Y( byte* dst, const byte* src, int width, int height );
void	ConvertCbCr_YToRGB( byte* dst, const byte* src, int width, int height );

void	ConvertRGBToYCbCr420( byte* dst, const byte* src, int width, int height );
void	ConvertYCbCr420ToRGB( byte* dst, const byte* src, int width, int height );

void	ConvertNormalMapToStereographicHeightMap( byte* heightMap, const byte* normalMap, int width, int height, float& scale );
void	ConvertStereographicHeightMapToNormalMap( byte* normalMap, const byte* heightMap, int width, int height, float scale );

void	ConvertRGBToMonochrome( byte* mono, const byte* rgb, int width, int height );
void	ConvertMonochromeToRGB( byte* rgb, const byte* mono, int width, int height );
};

#endif // !__COLORSPACE_H__
