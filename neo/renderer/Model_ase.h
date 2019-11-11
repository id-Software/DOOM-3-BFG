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

#ifndef __MODEL_ASE_H__
#define __MODEL_ASE_H__

/*
===============================================================================

	ASE loader. (3D Studio Max ASCII Export)

===============================================================================
*/

typedef struct
{
	int						vertexNum[3];
	int						tVertexNum[3];
	idVec3					faceNormal;
	idVec3					vertexNormals[3];
	byte					vertexColors[3][4];
} aseFace_t;

typedef struct
{
	int						timeValue;

	int						numVertexes;
	int						numTVertexes;
	int						numCVertexes;
	int						numFaces;
	int						numTVFaces;
	int						numCVFaces;

	idVec3					transform[4];			// applied to normals

	bool					colorsParsed;
	bool					normalsParsed;
	idVec3* 				vertexes;
	idVec2* 				tvertexes;
	idVec3* 				cvertexes;
	aseFace_t* 				faces;
} aseMesh_t;

typedef struct
{
	char					name[128];
	float					uOffset, vOffset;		// max lets you offset by material without changing texCoords
	float					uTiling, vTiling;		// multiply tex coords by this
	float					angle;					// in clockwise radians
} aseMaterial_t;

typedef struct
{
	char					name[128];
	int						materialRef;

	aseMesh_t				mesh;

	// frames are only present with animations
	idList<aseMesh_t*, TAG_MODEL>		frames;			// aseMesh_t
} aseObject_t;

typedef struct aseModel_s
{
	ID_TIME_T					timeStamp;
	idList<aseMaterial_t*, TAG_MODEL>	materials;
	idList<aseObject_t*, TAG_MODEL>	objects;
} aseModel_t;


aseModel_t* ASE_Load( const char* fileName );
void		ASE_Free( aseModel_t* ase );

#endif /* !__MODEL_ASE_H__ */
