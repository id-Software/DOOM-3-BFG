/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2014 Robert Beckebans

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

#ifndef __GLMATRIX_H__
#define __GLMATRIX_H__

/*
==========================================================================================

This deals with column-major (OpenGL style) matrices where transforms are
applied with right-multiplication.

This is the old DOOM3 matrix code that should really to be replaced with idRenderMatrix.

==========================================================================================
*/

void R_AxisToModelMatrix( const idMat3& axis, const idVec3& origin, float modelMatrix[16] );
void R_MatrixTranspose( const float in[16], float out[16] );
void R_MatrixMultiply( const float* a, const float* b, float* out );

void R_TransformModelToClip( const idVec3& src, const float* modelMatrix, const float* projectionMatrix, idPlane& eye, idPlane& dst );
void R_TransformClipToDevice( const idPlane& clip, idVec3& ndc );
void R_GlobalToNormalizedDeviceCoordinates( const idVec3& global, idVec3& ndc );

// note that these assume a normalized matrix, and will not work with scaled axis
void R_GlobalPointToLocal( const float modelMatrix[16], const idVec3& in, idVec3& out );
void R_LocalPointToGlobal( const float modelMatrix[16], const idVec3& in, idVec3& out );

void R_GlobalVectorToLocal( const float modelMatrix[16], const idVec3& in, idVec3& out );
void R_LocalVectorToGlobal( const float modelMatrix[16], const idVec3& in, idVec3& out );

void R_GlobalPlaneToLocal( const float modelMatrix[16], const idPlane& in, idPlane& out );
void R_LocalPlaneToGlobal( const float modelMatrix[16], const idPlane& in, idPlane& out );

void R_SetupViewMatrix( viewDef_t* viewDef );
void R_SetupProjectionMatrix( viewDef_t* viewDef, bool doJitter );

// RB begin
void R_SetupUnprojection( viewDef_t* viewDef );
void R_SetupProjectionMatrix2( const viewDef_t* viewDef, const float zNear, const float zFar, float out[16] );
void R_MatrixFullInverse( const float in[16], float r[16] );
// RB end

// SP begin
void R_ObliqueProjection( viewDef_t* viewDef );
// SP end

#endif /* !__GLMATRIX_H__ */
