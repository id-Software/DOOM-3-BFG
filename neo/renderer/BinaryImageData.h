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
#ifndef __BINARYIMAGEDATA_H__
#define __BINARYIMAGEDATA_H__

/*
================================================================================================

This is where the Binary image headers go that are also included by external tools such as the cloud.

================================================================================================
*/

// These structures are used for memory mapping bimage files, but
// not for the normal loading, so be careful making changes.
// Values are big endien to reduce effort on consoles.
#define BIMAGE_VERSION 10
#define BIMAGE_MAGIC (unsigned int)( ('B'<<0)|('I'<<8)|('M'<<16)|(BIMAGE_VERSION<<24) )

struct bimageImage_t
{
	int		level;
	int		destZ;
	int		width;
	int		height;
	int		dataSize;
	// dataSize bytes follow
};

#pragma pack( push, 1 )
struct bimageFile_t
{
	ID_TIME_T	sourceFileTime;
	int		headerMagic;
	int		textureType;
	int		format;
	int		colorFormat;
	int		width;
	int		height;
	int		numLevels;
	// one or more bimageImage_t structures follow
};
#pragma pack( pop )

#endif // __BINARYIMAGEDATA_H__
