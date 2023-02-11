/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2014 Robert Beckebans
Copyright (C) 2022 Stephen Pridham

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

#include "precompiled.h"
#pragma hdrstop

#include "RenderCommon.h"

/*
==========================================================================================

OLD MATRIX MATH

==========================================================================================
*/

/*
======================
R_AxisToModelMatrix
======================
*/
void R_AxisToModelMatrix( const idMat3& axis, const idVec3& origin, float modelMatrix[16] )
{
	modelMatrix[0 * 4 + 0] = axis[0][0];
	modelMatrix[1 * 4 + 0] = axis[1][0];
	modelMatrix[2 * 4 + 0] = axis[2][0];
	modelMatrix[3 * 4 + 0] = origin[0];

	modelMatrix[0 * 4 + 1] = axis[0][1];
	modelMatrix[1 * 4 + 1] = axis[1][1];
	modelMatrix[2 * 4 + 1] = axis[2][1];
	modelMatrix[3 * 4 + 1] = origin[1];

	modelMatrix[0 * 4 + 2] = axis[0][2];
	modelMatrix[1 * 4 + 2] = axis[1][2];
	modelMatrix[2 * 4 + 2] = axis[2][2];
	modelMatrix[3 * 4 + 2] = origin[2];

	modelMatrix[0 * 4 + 3] = 0.0f;
	modelMatrix[1 * 4 + 3] = 0.0f;
	modelMatrix[2 * 4 + 3] = 0.0f;
	modelMatrix[3 * 4 + 3] = 1.0f;
}

/*
==========================
R_MatrixMultiply
==========================
*/
void R_MatrixMultiply( const float a[16], const float b[16], float out[16] )
{
#if defined(USE_INTRINSICS_SSE)
	__m128 a0 = _mm_loadu_ps( a + 0 * 4 );
	__m128 a1 = _mm_loadu_ps( a + 1 * 4 );
	__m128 a2 = _mm_loadu_ps( a + 2 * 4 );
	__m128 a3 = _mm_loadu_ps( a + 3 * 4 );

	__m128 b0 = _mm_loadu_ps( b + 0 * 4 );
	__m128 b1 = _mm_loadu_ps( b + 1 * 4 );
	__m128 b2 = _mm_loadu_ps( b + 2 * 4 );
	__m128 b3 = _mm_loadu_ps( b + 3 * 4 );

	__m128 t0 = _mm_mul_ps( _mm_splat_ps( a0, 0 ), b0 );
	__m128 t1 = _mm_mul_ps( _mm_splat_ps( a1, 0 ), b0 );
	__m128 t2 = _mm_mul_ps( _mm_splat_ps( a2, 0 ), b0 );
	__m128 t3 = _mm_mul_ps( _mm_splat_ps( a3, 0 ), b0 );

	t0 = _mm_add_ps( t0, _mm_mul_ps( _mm_splat_ps( a0, 1 ), b1 ) );
	t1 = _mm_add_ps( t1, _mm_mul_ps( _mm_splat_ps( a1, 1 ), b1 ) );
	t2 = _mm_add_ps( t2, _mm_mul_ps( _mm_splat_ps( a2, 1 ), b1 ) );
	t3 = _mm_add_ps( t3, _mm_mul_ps( _mm_splat_ps( a3, 1 ), b1 ) );

	t0 = _mm_add_ps( t0, _mm_mul_ps( _mm_splat_ps( a0, 2 ), b2 ) );
	t1 = _mm_add_ps( t1, _mm_mul_ps( _mm_splat_ps( a1, 2 ), b2 ) );
	t2 = _mm_add_ps( t2, _mm_mul_ps( _mm_splat_ps( a2, 2 ), b2 ) );
	t3 = _mm_add_ps( t3, _mm_mul_ps( _mm_splat_ps( a3, 2 ), b2 ) );

	t0 = _mm_add_ps( t0, _mm_mul_ps( _mm_splat_ps( a0, 3 ), b3 ) );
	t1 = _mm_add_ps( t1, _mm_mul_ps( _mm_splat_ps( a1, 3 ), b3 ) );
	t2 = _mm_add_ps( t2, _mm_mul_ps( _mm_splat_ps( a2, 3 ), b3 ) );
	t3 = _mm_add_ps( t3, _mm_mul_ps( _mm_splat_ps( a3, 3 ), b3 ) );

	_mm_storeu_ps( out + 0 * 4, t0 );
	_mm_storeu_ps( out + 1 * 4, t1 );
	_mm_storeu_ps( out + 2 * 4, t2 );
	_mm_storeu_ps( out + 3 * 4, t3 );

#else

	/*
	for ( int i = 0; i < 4; i++ ) {
		for ( int j = 0; j < 4; j++ ) {
			out[ i * 4 + j ] =
				a[ i * 4 + 0 ] * b[ 0 * 4 + j ] +
				a[ i * 4 + 1 ] * b[ 1 * 4 + j ] +
				a[ i * 4 + 2 ] * b[ 2 * 4 + j ] +
				a[ i * 4 + 3 ] * b[ 3 * 4 + j ];
		}
	}
	*/

	out[0 * 4 + 0] = a[0 * 4 + 0] * b[0 * 4 + 0] + a[0 * 4 + 1] * b[1 * 4 + 0] + a[0 * 4 + 2] * b[2 * 4 + 0] + a[0 * 4 + 3] * b[3 * 4 + 0];
	out[0 * 4 + 1] = a[0 * 4 + 0] * b[0 * 4 + 1] + a[0 * 4 + 1] * b[1 * 4 + 1] + a[0 * 4 + 2] * b[2 * 4 + 1] + a[0 * 4 + 3] * b[3 * 4 + 1];
	out[0 * 4 + 2] = a[0 * 4 + 0] * b[0 * 4 + 2] + a[0 * 4 + 1] * b[1 * 4 + 2] + a[0 * 4 + 2] * b[2 * 4 + 2] + a[0 * 4 + 3] * b[3 * 4 + 2];
	out[0 * 4 + 3] = a[0 * 4 + 0] * b[0 * 4 + 3] + a[0 * 4 + 1] * b[1 * 4 + 3] + a[0 * 4 + 2] * b[2 * 4 + 3] + a[0 * 4 + 3] * b[3 * 4 + 3];

	out[1 * 4 + 0] = a[1 * 4 + 0] * b[0 * 4 + 0] + a[1 * 4 + 1] * b[1 * 4 + 0] + a[1 * 4 + 2] * b[2 * 4 + 0] + a[1 * 4 + 3] * b[3 * 4 + 0];
	out[1 * 4 + 1] = a[1 * 4 + 0] * b[0 * 4 + 1] + a[1 * 4 + 1] * b[1 * 4 + 1] + a[1 * 4 + 2] * b[2 * 4 + 1] + a[1 * 4 + 3] * b[3 * 4 + 1];
	out[1 * 4 + 2] = a[1 * 4 + 0] * b[0 * 4 + 2] + a[1 * 4 + 1] * b[1 * 4 + 2] + a[1 * 4 + 2] * b[2 * 4 + 2] + a[1 * 4 + 3] * b[3 * 4 + 2];
	out[1 * 4 + 3] = a[1 * 4 + 0] * b[0 * 4 + 3] + a[1 * 4 + 1] * b[1 * 4 + 3] + a[1 * 4 + 2] * b[2 * 4 + 3] + a[1 * 4 + 3] * b[3 * 4 + 3];

	out[2 * 4 + 0] = a[2 * 4 + 0] * b[0 * 4 + 0] + a[2 * 4 + 1] * b[1 * 4 + 0] + a[2 * 4 + 2] * b[2 * 4 + 0] + a[2 * 4 + 3] * b[3 * 4 + 0];
	out[2 * 4 + 1] = a[2 * 4 + 0] * b[0 * 4 + 1] + a[2 * 4 + 1] * b[1 * 4 + 1] + a[2 * 4 + 2] * b[2 * 4 + 1] + a[2 * 4 + 3] * b[3 * 4 + 1];
	out[2 * 4 + 2] = a[2 * 4 + 0] * b[0 * 4 + 2] + a[2 * 4 + 1] * b[1 * 4 + 2] + a[2 * 4 + 2] * b[2 * 4 + 2] + a[2 * 4 + 3] * b[3 * 4 + 2];
	out[2 * 4 + 3] = a[2 * 4 + 0] * b[0 * 4 + 3] + a[2 * 4 + 1] * b[1 * 4 + 3] + a[2 * 4 + 2] * b[2 * 4 + 3] + a[2 * 4 + 3] * b[3 * 4 + 3];

	out[3 * 4 + 0] = a[3 * 4 + 0] * b[0 * 4 + 0] + a[3 * 4 + 1] * b[1 * 4 + 0] + a[3 * 4 + 2] * b[2 * 4 + 0] + a[3 * 4 + 3] * b[3 * 4 + 0];
	out[3 * 4 + 1] = a[3 * 4 + 0] * b[0 * 4 + 1] + a[3 * 4 + 1] * b[1 * 4 + 1] + a[3 * 4 + 2] * b[2 * 4 + 1] + a[3 * 4 + 3] * b[3 * 4 + 1];
	out[3 * 4 + 2] = a[3 * 4 + 0] * b[0 * 4 + 2] + a[3 * 4 + 1] * b[1 * 4 + 2] + a[3 * 4 + 2] * b[2 * 4 + 2] + a[3 * 4 + 3] * b[3 * 4 + 2];
	out[3 * 4 + 3] = a[3 * 4 + 0] * b[0 * 4 + 3] + a[3 * 4 + 1] * b[1 * 4 + 3] + a[3 * 4 + 2] * b[2 * 4 + 3] + a[3 * 4 + 3] * b[3 * 4 + 3];

#endif
}

/*
======================
R_MatrixTranspose
======================
*/
void R_MatrixTranspose( const float in[16], float out[16] )
{
	for( int i = 0; i < 4; i++ )
	{
		for( int j = 0; j < 4; j++ )
		{
			out[i * 4 + j] = in[j * 4 + i];
		}
	}
}

/*
==========================
R_TransformModelToClip
==========================
*/
void R_TransformModelToClip( const idVec3& src, const float* modelMatrix, const float* projectionMatrix, idPlane& eye, idPlane& dst )
{
	for( int i = 0; i < 4; i++ )
	{
		eye[i] = 	modelMatrix[i + 0 * 4] * src[0] +
					modelMatrix[i + 1 * 4] * src[1] +
					modelMatrix[i + 2 * 4] * src[2] +
					modelMatrix[i + 3 * 4];
	}

	for( int i = 0; i < 4; i++ )
	{
		dst[i] = 	projectionMatrix[i + 0 * 4] * eye[0] +
					projectionMatrix[i + 1 * 4] * eye[1] +
					projectionMatrix[i + 2 * 4] * eye[2] +
					projectionMatrix[i + 3 * 4] * eye[3];
	}
}

/*
==========================
R_TransformClipToDevice

Clip to normalized device coordinates
==========================
*/
void R_TransformClipToDevice( const idPlane& clip, idVec3& ndc )
{
	const float invW = 1.0f / clip[3];
	ndc[0] = clip[0] * invW;
	ndc[1] = clip[1] * invW;
	ndc[2] = clip[2] * invW;		// NOTE: in D3D this is in the range [0,1]
}

/*
==========================
R_GlobalToNormalizedDeviceCoordinates

-1 to 1 range in x, y, and z
==========================
*/
void R_GlobalToNormalizedDeviceCoordinates( const idVec3& global, idVec3& ndc )
{
	idPlane	view;
	idPlane	clip;

	// _D3XP use tr.primaryView when there is no tr.viewDef
	const viewDef_t* viewDef = ( tr.viewDef != NULL ) ? tr.viewDef : tr.primaryView;

	for( int i = 0; i < 4; i ++ )
	{
		view[i] = 	viewDef->worldSpace.modelViewMatrix[i + 0 * 4] * global[0] +
					viewDef->worldSpace.modelViewMatrix[i + 1 * 4] * global[1] +
					viewDef->worldSpace.modelViewMatrix[i + 2 * 4] * global[2] +
					viewDef->worldSpace.modelViewMatrix[i + 3 * 4];
	}

	for( int i = 0; i < 4; i ++ )
	{
		clip[i] = 	viewDef->projectionMatrix[i + 0 * 4] * view[0] +
					viewDef->projectionMatrix[i + 1 * 4] * view[1] +
					viewDef->projectionMatrix[i + 2 * 4] * view[2] +
					viewDef->projectionMatrix[i + 3 * 4] * view[3];
	}

	const float invW = 1.0f / clip[3];
	ndc[0] = clip[0] * invW;
	ndc[1] = clip[1] * invW;
	ndc[2] = clip[2] * invW;		// NOTE: in D3D this is in the range [0,1]
}

/*
======================
R_LocalPointToGlobal

NOTE: assumes no skewing or scaling transforms
======================
*/
void R_LocalPointToGlobal( const float modelMatrix[16], const idVec3& in, idVec3& out )
{
	out[0] = in[0] * modelMatrix[0 * 4 + 0] + in[1] * modelMatrix[1 * 4 + 0] + in[2] * modelMatrix[2 * 4 + 0] + modelMatrix[3 * 4 + 0];
	out[1] = in[0] * modelMatrix[0 * 4 + 1] + in[1] * modelMatrix[1 * 4 + 1] + in[2] * modelMatrix[2 * 4 + 1] + modelMatrix[3 * 4 + 1];
	out[2] = in[0] * modelMatrix[0 * 4 + 2] + in[1] * modelMatrix[1 * 4 + 2] + in[2] * modelMatrix[2 * 4 + 2] + modelMatrix[3 * 4 + 2];
}

/*
======================
R_GlobalPointToLocal

NOTE: assumes no skewing or scaling transforms
======================
*/
void R_GlobalPointToLocal( const float modelMatrix[16], const idVec3& in, idVec3& out )
{
	idVec3 temp;

	temp[0] = in[0] - modelMatrix[3 * 4 + 0];
	temp[1] = in[1] - modelMatrix[3 * 4 + 1];
	temp[2] = in[2] - modelMatrix[3 * 4 + 2];

	out[0] = temp[0] * modelMatrix[0 * 4 + 0] + temp[1] * modelMatrix[0 * 4 + 1] + temp[2] * modelMatrix[0 * 4 + 2];
	out[1] = temp[0] * modelMatrix[1 * 4 + 0] + temp[1] * modelMatrix[1 * 4 + 1] + temp[2] * modelMatrix[1 * 4 + 2];
	out[2] = temp[0] * modelMatrix[2 * 4 + 0] + temp[1] * modelMatrix[2 * 4 + 1] + temp[2] * modelMatrix[2 * 4 + 2];
}

/*
======================
R_LocalVectorToGlobal

NOTE: assumes no skewing or scaling transforms
======================
*/
void R_LocalVectorToGlobal( const float modelMatrix[16], const idVec3& in, idVec3& out )
{
	out[0] = in[0] * modelMatrix[0 * 4 + 0] + in[1] * modelMatrix[1 * 4 + 0] + in[2] * modelMatrix[2 * 4 + 0];
	out[1] = in[0] * modelMatrix[0 * 4 + 1] + in[1] * modelMatrix[1 * 4 + 1] + in[2] * modelMatrix[2 * 4 + 1];
	out[2] = in[0] * modelMatrix[0 * 4 + 2] + in[1] * modelMatrix[1 * 4 + 2] + in[2] * modelMatrix[2 * 4 + 2];
}

/*
======================
R_GlobalVectorToLocal

NOTE: assumes no skewing or scaling transforms
======================
*/
void R_GlobalVectorToLocal( const float modelMatrix[16], const idVec3& in, idVec3& out )
{
	out[0] = in[0] * modelMatrix[0 * 4 + 0] + in[1] * modelMatrix[0 * 4 + 1] + in[2] * modelMatrix[0 * 4 + 2];
	out[1] = in[0] * modelMatrix[1 * 4 + 0] + in[1] * modelMatrix[1 * 4 + 1] + in[2] * modelMatrix[1 * 4 + 2];
	out[2] = in[0] * modelMatrix[2 * 4 + 0] + in[1] * modelMatrix[2 * 4 + 1] + in[2] * modelMatrix[2 * 4 + 2];
}

/*
======================
R_GlobalPlaneToLocal

NOTE: assumes no skewing or scaling transforms
======================
*/
void R_GlobalPlaneToLocal( const float modelMatrix[16], const idPlane& in, idPlane& out )
{
	out[0] = in[0] * modelMatrix[0 * 4 + 0] + in[1] * modelMatrix[0 * 4 + 1] + in[2] * modelMatrix[0 * 4 + 2];
	out[1] = in[0] * modelMatrix[1 * 4 + 0] + in[1] * modelMatrix[1 * 4 + 1] + in[2] * modelMatrix[1 * 4 + 2];
	out[2] = in[0] * modelMatrix[2 * 4 + 0] + in[1] * modelMatrix[2 * 4 + 1] + in[2] * modelMatrix[2 * 4 + 2];
	out[3] = in[0] * modelMatrix[3 * 4 + 0] + in[1] * modelMatrix[3 * 4 + 1] + in[2] * modelMatrix[3 * 4 + 2] + in[3];
}

/*
======================
R_LocalPlaneToGlobal

NOTE: assumes no skewing or scaling transforms
======================
*/
void R_LocalPlaneToGlobal( const float modelMatrix[16], const idPlane& in, idPlane& out )
{
	out[0] = in[0] * modelMatrix[0 * 4 + 0] + in[1] * modelMatrix[1 * 4 + 0] + in[2] * modelMatrix[2 * 4 + 0];
	out[1] = in[0] * modelMatrix[0 * 4 + 1] + in[1] * modelMatrix[1 * 4 + 1] + in[2] * modelMatrix[2 * 4 + 1];
	out[2] = in[0] * modelMatrix[0 * 4 + 2] + in[1] * modelMatrix[1 * 4 + 2] + in[2] * modelMatrix[2 * 4 + 2];
	out[3] = in[3] - modelMatrix[3 * 4 + 0] * out[0] - modelMatrix[3 * 4 + 1] * out[1] - modelMatrix[3 * 4 + 2] * out[2];
}

/*
==========================================================================================

WORLD/VIEW/PROJECTION MATRIX SETUP

==========================================================================================
*/

/*
======================
R_SetupViewMatrix

Sets up the world to view matrix for a given viewParm
======================
*/
void R_SetupViewMatrix( viewDef_t* viewDef )
{
	static float s_flipMatrix[16] =
	{
		// convert from our coordinate system (looking down X)
		// to OpenGL's coordinate system (looking down -Z)
		0, 0, -1, 0,
		-1, 0,  0, 0,
		0, 1,  0, 0,
		0, 0,  0, 1
	};

	viewEntity_t* world = &viewDef->worldSpace;
	memset( world, 0, sizeof( *world ) );

	// the model matrix is an identity
	world->modelMatrix[0 * 4 + 0] = 1.0f;
	world->modelMatrix[1 * 4 + 1] = 1.0f;
	world->modelMatrix[2 * 4 + 2] = 1.0f;

	// transform by the camera placement
	const idVec3& origin = viewDef->renderView.vieworg;
	const idMat3& axis = viewDef->renderView.viewaxis;

	float viewerMatrix[16];
	viewerMatrix[0 * 4 + 0] = axis[0][0];
	viewerMatrix[1 * 4 + 0] = axis[0][1];
	viewerMatrix[2 * 4 + 0] = axis[0][2];
	viewerMatrix[3 * 4 + 0] = - origin[0] * axis[0][0] - origin[1] * axis[0][1] - origin[2] * axis[0][2];

	viewerMatrix[0 * 4 + 1] = axis[1][0];
	viewerMatrix[1 * 4 + 1] = axis[1][1];
	viewerMatrix[2 * 4 + 1] = axis[1][2];
	viewerMatrix[3 * 4 + 1] = - origin[0] * axis[1][0] - origin[1] * axis[1][1] - origin[2] * axis[1][2];

	viewerMatrix[0 * 4 + 2] = axis[2][0];
	viewerMatrix[1 * 4 + 2] = axis[2][1];
	viewerMatrix[2 * 4 + 2] = axis[2][2];
	viewerMatrix[3 * 4 + 2] = - origin[0] * axis[2][0] - origin[1] * axis[2][1] - origin[2] * axis[2][2];

	viewerMatrix[0 * 4 + 3] = 0.0f;
	viewerMatrix[1 * 4 + 3] = 0.0f;
	viewerMatrix[2 * 4 + 3] = 0.0f;
	viewerMatrix[3 * 4 + 3] = 1.0f;

	// convert from our coordinate system (looking down X)
	// to OpenGL's coordinate system (looking down -Z)
	R_MatrixMultiply( viewerMatrix, s_flipMatrix, world->modelViewMatrix );
}

/*
======================
R_SetupProjectionMatrix

This uses the "infinite far z" trick
======================
*/
idCVar r_centerX( "r_centerX", "0", CVAR_FLOAT, "projection matrix center adjust" );
idCVar r_centerY( "r_centerY", "0", CVAR_FLOAT, "projection matrix center adjust" );
idCVar r_centerScale( "r_centerScale", "1", CVAR_FLOAT, "projection matrix center adjust" );

inline float sgn( float a )
{
	if( a > 0.0f )
	{
		return ( 1.0f );
	}
	if( a < 0.0f )
	{
		return ( -1.0f );
	}
	return ( 0.0f );
}

// clipPlane is a plane in camera space.
void ModifyProjectionMatrix( viewDef_t* viewDef, const idPlane& clipPlane )
{
	static float s_flipMatrix[16] =
	{
		// convert from our coordinate system (looking down X)
		// to OpenGL's coordinate system (looking down -Z)
		0, 0, -1, 0,
		-1, 0,  0, 0,
		0, 1,  0, 0,
		0, 0,  0, 1
	};

	idMat4 flipMatrix;
	memcpy( &flipMatrix, &( s_flipMatrix[0] ), sizeof( float ) * 16 );

	idVec4 vec = clipPlane.ToVec4();// * flipMatrix;
	idPlane newPlane( vec[0], vec[1], vec[2], vec[3] );

	// Calculate the clip-space corner point opposite the clipping plane
	// as (sgn(clipPlane.x), sgn(clipPlane.y), 1, 1) and
	// transform it into camera space by multiplying it
	// by the inverse of the projection matrix

	//idVec4 q;
	//q.x = (sgn(newPlane[0]) + viewDef->projectionMatrix[8]) / viewDef->projectionMatrix[0];
	//q.y = (sgn(newPlane[1]) + viewDef->projectionMatrix[9]) / viewDef->projectionMatrix[5];
	//q.z = -1.0F;
	//q.w = (1.0F + viewDef->projectionMatrix[10]) / viewDef->projectionMatrix[14];

	idMat4 unprojection;
	R_MatrixFullInverse( viewDef->projectionMatrix, ( float* )&unprojection );
	idVec4 q = unprojection * idVec4( sgn( newPlane[0] ), sgn( newPlane[1] ), 1.0f, 1.0f );

	// Calculate the scaled plane vector
	idVec4 c = newPlane.ToVec4() * ( 2.0f / ( q * newPlane.ToVec4() ) );

	float matrix[16];
	std::memcpy( matrix, viewDef->projectionMatrix, sizeof( float ) * 16 );

	// Replace the third row of the projection matrix
	matrix[2] = c[0];
	matrix[6] = c[1];
	matrix[10] = c[2] + 1.0f;
	matrix[14] = c[3];

	memcpy( viewDef->projectionMatrix, matrix, sizeof( float ) * 16 );
}

void R_SetupProjectionMatrix( viewDef_t* viewDef, bool doJitter )
{
	// random jittering is usefull when multiple
	// frames are going to be blended together
	// for motion blurred anti-aliasing
	float jitterx, jittery;

	if( R_UseTemporalAA() && doJitter && !( viewDef->renderView.rdflags & RDF_IRRADIANCE ) )
	{
		idVec2 jitter = tr.backend.GetCurrentPixelOffset();
		jitterx = jitter.x;
		jittery = jitter.y;
	}
	else
	{
		jitterx = 0.0f;
		jittery = 0.0f;
	}

	//
	// set up projection matrix
	//
	const float zNear = ( viewDef->renderView.cramZNear ) ? ( r_znear.GetFloat() * 0.25f ) : r_znear.GetFloat();

	const int viewWidth = viewDef->viewport.x2 - viewDef->viewport.x1 + 1;
	const int viewHeight = viewDef->viewport.y2 - viewDef->viewport.y1 + 1;

	// TODO integrate jitterx += viewDef->renderView.stereoScreenSeparation;

	// this mimics the logic in the Donut Feature Demo
	const float xoffset = -2.0f * jitterx / ( 1.0f * viewWidth );
	const float yoffset = -2.0f * jittery / ( 1.0f * viewHeight );

	float* projectionMatrix = doJitter ? viewDef->projectionMatrix : viewDef->unjitteredProjectionMatrix;

#if 1

	float ymax = zNear * tan( viewDef->renderView.fov_y * idMath::PI / 360.0f );
	float ymin = -ymax;

	float xmax = zNear * tan( viewDef->renderView.fov_x * idMath::PI / 360.0f );
	float xmin = -xmax;

	const float width = xmax - xmin;
	const float height = ymax - ymin;

	projectionMatrix[0 * 4 + 0] = 2.0f * zNear / width;
	projectionMatrix[1 * 4 + 0] = 0.0f;
	projectionMatrix[2 * 4 + 0] = xoffset;
	projectionMatrix[3 * 4 + 0] = 0.0f;

	projectionMatrix[0 * 4 + 1] = 0.0f;
	projectionMatrix[1 * 4 + 1] = 2.0f * zNear / height;
	projectionMatrix[2 * 4 + 1] = yoffset;
	projectionMatrix[3 * 4 + 1] = 0.0f;

	// this is the far-plane-at-infinity formulation, and
	// crunches the Z range slightly so w=0 vertexes do not
	// rasterize right at the wraparound point
	projectionMatrix[0 * 4 + 2] = 0.0f;
	projectionMatrix[1 * 4 + 2] = 0.0f;
	projectionMatrix[2 * 4 + 2] = -0.999f;			// adjust value to prevent imprecision issues

	// RB: was -2.0f * zNear
	// the transformation into window space has changed from [-1 .. 1] to [0 .. 1]
	projectionMatrix[3 * 4 + 2] = -1.0f * zNear;

	projectionMatrix[0 * 4 + 3] = 0.0f;
	projectionMatrix[1 * 4 + 3] = 0.0f;
	projectionMatrix[2 * 4 + 3] = -1.0f;
	projectionMatrix[3 * 4 + 3] = 0.0f;

#else

	// alternative far plane at infinity Z for better precision in the distance but still no reversed depth buffer
	// see Foundations of Game Engine Development 2, chapter 6.3

	float aspect = viewDef->renderView.fov_x / viewDef->renderView.fov_y;

	float yScale = 1.0f / ( tanf( 0.5f * DEG2RAD( viewDef->renderView.fov_y ) ) );
	float xScale = yScale / aspect;

	const float epsilon = 1.9073486328125e-6F;	// 2^-19;
	const float zFar = 160000;

	//float k = zFar / ( zFar - zNear );
	float k = 1.0f - epsilon;

	projectionMatrix[0 * 4 + 0] = xScale;
	projectionMatrix[1 * 4 + 0] = 0.0f;
	projectionMatrix[2 * 4 + 0] = xoffset;
	projectionMatrix[3 * 4 + 0] = 0.0f;

	projectionMatrix[0 * 4 + 1] = 0.0f;
	projectionMatrix[1 * 4 + 1] = yScale;
	projectionMatrix[2 * 4 + 1] = yoffset;
	projectionMatrix[3 * 4 + 1] = 0.0f;

	projectionMatrix[0 * 4 + 2] = 0.0f;
	projectionMatrix[1 * 4 + 2] = 0.0f;

	// adjust value to prevent imprecision issues
	projectionMatrix[2 * 4 + 2] = -k;

	// the clip space Z range has changed from [-1 .. 1] to [0 .. 1] for DX12 & Vulkan
	projectionMatrix[3 * 4 + 2] = -k * zNear;

	projectionMatrix[0 * 4 + 3] = 0.0f;
	projectionMatrix[1 * 4 + 3] = 0.0f;
	projectionMatrix[2 * 4 + 3] = -1.0f;
	projectionMatrix[3 * 4 + 3] = 0.0f;

#endif

	if( viewDef->renderView.flipProjection )
	{
		projectionMatrix[1 * 4 + 1] = -projectionMatrix[1 * 4 + 1];
		projectionMatrix[1 * 4 + 3] = -projectionMatrix[1 * 4 + 3];
	}

	// SP Begin
	if( viewDef->isObliqueProjection && doJitter )
	{
		R_ObliqueProjection( viewDef );
	}
	// SP End
}


// RB: standard OpenGL projection matrix
void R_SetupProjectionMatrix2( const viewDef_t* viewDef, const float zNear, const float zFar, float projectionMatrix[16] )
{
	float ymax = zNear * tan( viewDef->renderView.fov_y * idMath::PI / 360.0f );
	float ymin = -ymax;

	float xmax = zNear * tan( viewDef->renderView.fov_x * idMath::PI / 360.0f );
	float xmin = -xmax;

	const float width = xmax - xmin;
	const float height = ymax - ymin;

	const int viewWidth = viewDef->viewport.x2 - viewDef->viewport.x1 + 1;
	const int viewHeight = viewDef->viewport.y2 - viewDef->viewport.y1 + 1;

	float jitterx, jittery;
	jitterx = 0.0f;
	jittery = 0.0f;
	jitterx = jitterx * width / viewWidth;
	jitterx += r_centerX.GetFloat();
	jitterx += viewDef->renderView.stereoScreenSeparation;
	xmin += jitterx * width;
	xmax += jitterx * width;

	jittery = jittery * height / viewHeight;
	jittery += r_centerY.GetFloat();
	ymin += jittery * height;
	ymax += jittery * height;

	float depth = zFar - zNear;

	projectionMatrix[0 * 4 + 0] = 2.0f * zNear / width;
	projectionMatrix[1 * 4 + 0] = 0.0f;
	projectionMatrix[2 * 4 + 0] = ( xmax + xmin ) / width;	// normally 0
	projectionMatrix[3 * 4 + 0] = 0.0f;

	projectionMatrix[0 * 4 + 1] = 0.0f;
	projectionMatrix[1 * 4 + 1] = 2.0f * zNear / height;
	projectionMatrix[2 * 4 + 1] = ( ymax + ymin ) / height;	// normally 0
	projectionMatrix[3 * 4 + 1] = 0.0f;

	projectionMatrix[0 * 4 + 2] = 0.0f;
	projectionMatrix[1 * 4 + 2] = 0.0f;
	projectionMatrix[2 * 4 + 2] =  -( zFar + zNear ) / depth;		// -0.999f; // adjust value to prevent imprecision issues
	projectionMatrix[3 * 4 + 2] = -2 * zFar * zNear / depth;	// -2.0f * zNear;

	projectionMatrix[0 * 4 + 3] = 0.0f;
	projectionMatrix[1 * 4 + 3] = 0.0f;
	projectionMatrix[2 * 4 + 3] = -1.0f;
	projectionMatrix[3 * 4 + 3] = 0.0f;

	if( viewDef->renderView.flipProjection )
	{
		projectionMatrix[1 * 4 + 1] = -viewDef->projectionMatrix[1 * 4 + 1];
		projectionMatrix[1 * 4 + 3] = -viewDef->projectionMatrix[1 * 4 + 3];
	}

#if defined(USE_VULKAN)
	projectionMatrix[1 * 4 + 1] *= -1.0F;
#endif
}

/*
=================
R_SetupUnprojection
create a matrix with similar functionality like gluUnproject, project from window space to world space
=================
*/
void R_SetupUnprojection( viewDef_t* viewDef )
{
	// RB: I don't like that this doesn't work
	//idRenderMatrix::Inverse( *( idRenderMatrix* ) viewDef->projectionMatrix, viewDef->unprojectionToCameraRenderMatrix );

	R_MatrixFullInverse( viewDef->projectionMatrix, viewDef->unprojectionToCameraMatrix );
	idRenderMatrix::Transpose( *( idRenderMatrix* )viewDef->unprojectionToCameraMatrix, viewDef->unprojectionToCameraRenderMatrix );

	R_MatrixMultiply( viewDef->worldSpace.modelViewMatrix, viewDef->projectionMatrix, viewDef->unprojectionToWorldMatrix );
	R_MatrixFullInverse( viewDef->unprojectionToWorldMatrix, viewDef->unprojectionToWorldMatrix );

	idRenderMatrix::Transpose( *( idRenderMatrix* )viewDef->unprojectionToWorldMatrix, viewDef->unprojectionToWorldRenderMatrix );
}

void R_MatrixFullInverse( const float a[16], float r[16] )
{
	idMat4	am;

	for( int i = 0 ; i < 4 ; i++ )
	{
		for( int j = 0 ; j < 4 ; j++ )
		{
			am[i][j] = a[j * 4 + i];
		}
	}

//	idVec4 test( 100, 100, 100, 1 );
//	idVec4	transformed, inverted;
//	transformed = test * am;

	if( !am.InverseSelf() )
	{
		common->Error( "Invert failed" );
	}
//	inverted = transformed * am;

	for( int i = 0 ; i < 4 ; i++ )
	{
		for( int j = 0 ; j < 4 ; j++ )
		{
			r[j * 4 + i] = am[i][j];
		}
	}
}
// RB end

// SP begin
/*
=====================
R_ObliqueProjection - adjust near plane of previously set projection matrix to perform an oblique projection
credits to motorsep: https://github.com/motorsep/StormEngine2/blob/743a0f9581a10837a91cb296ff5a1114535e8d4e/neo/renderer/tr_frontend_subview.cpp#L225
=====================
*/
void R_ObliqueProjection( viewDef_t* parms )
{
	float mvt[16]; // model view transpose
	idPlane pB = parms->clipPlanes[0];
	idPlane cp; // camera space plane
	R_MatrixTranspose( parms->worldSpace.modelViewMatrix, mvt );

	// transform plane (which is set to the surface we're mirroring about's plane) to camera space
	R_GlobalPlaneToLocal( mvt, pB, cp );

	// oblique projection adjustment code
	idVec4 clipPlane( cp[0], cp[1], cp[2], cp[3] );
	idVec4 q;
	q[0] = ( ( clipPlane[0] < 0.0f ? -1.0f : clipPlane[0] > 0.0f ? 1.0f : 0.0f ) + parms->projectionMatrix[8] ) / parms->projectionMatrix[0];
	q[1] = ( ( clipPlane[1] < 0.0f ? -1.0f : clipPlane[1] > 0.0f ? 1.0f : 0.0f ) + parms->projectionMatrix[9] ) / parms->projectionMatrix[5];
	q[2] = -1.0f;
	q[3] = ( 1.0f + parms->projectionMatrix[10] ) / parms->projectionMatrix[14];

	// scaled plane vector
	float d = 2.0f / ( clipPlane * q );

	// Replace the third row of the projection matrix
	parms->projectionMatrix[2] = clipPlane[0] * d;
	parms->projectionMatrix[6] = clipPlane[1] * d;
	parms->projectionMatrix[10] = clipPlane[2] * d + 1.0f;
	parms->projectionMatrix[14] = clipPlane[3] * d;
}
// SP end