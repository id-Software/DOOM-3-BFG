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

#pragma hdrstop
#include "../idlib/precompiled.h"

#include "tr_local.h"

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
void R_AxisToModelMatrix( const idMat3 &axis, const idVec3 &origin, float modelMatrix[16] ) {
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
void R_MatrixMultiply( const float a[16], const float b[16], float out[16] ) {
#ifdef ID_WIN_X86_SSE2_INTRIN

	__m128 a0 = _mm_loadu_ps( a + 0*4 );
	__m128 a1 = _mm_loadu_ps( a + 1*4 );
	__m128 a2 = _mm_loadu_ps( a + 2*4 );
	__m128 a3 = _mm_loadu_ps( a + 3*4 );

	__m128 b0 = _mm_loadu_ps( b + 0*4 );
	__m128 b1 = _mm_loadu_ps( b + 1*4 );
	__m128 b2 = _mm_loadu_ps( b + 2*4 );
	__m128 b3 = _mm_loadu_ps( b + 3*4 );

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

	_mm_storeu_ps( out + 0*4, t0 );
	_mm_storeu_ps( out + 1*4, t1 );
	_mm_storeu_ps( out + 2*4, t2 );
	_mm_storeu_ps( out + 3*4, t3 );

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

	out[0*4+0] = a[0*4+0]*b[0*4+0] + a[0*4+1]*b[1*4+0] + a[0*4+2]*b[2*4+0] + a[0*4+3]*b[3*4+0];
	out[0*4+1] = a[0*4+0]*b[0*4+1] + a[0*4+1]*b[1*4+1] + a[0*4+2]*b[2*4+1] + a[0*4+3]*b[3*4+1];
	out[0*4+2] = a[0*4+0]*b[0*4+2] + a[0*4+1]*b[1*4+2] + a[0*4+2]*b[2*4+2] + a[0*4+3]*b[3*4+2];
	out[0*4+3] = a[0*4+0]*b[0*4+3] + a[0*4+1]*b[1*4+3] + a[0*4+2]*b[2*4+3] + a[0*4+3]*b[3*4+3];

	out[1*4+0] = a[1*4+0]*b[0*4+0] + a[1*4+1]*b[1*4+0] + a[1*4+2]*b[2*4+0] + a[1*4+3]*b[3*4+0];
	out[1*4+1] = a[1*4+0]*b[0*4+1] + a[1*4+1]*b[1*4+1] + a[1*4+2]*b[2*4+1] + a[1*4+3]*b[3*4+1];
	out[1*4+2] = a[1*4+0]*b[0*4+2] + a[1*4+1]*b[1*4+2] + a[1*4+2]*b[2*4+2] + a[1*4+3]*b[3*4+2];
	out[1*4+3] = a[1*4+0]*b[0*4+3] + a[1*4+1]*b[1*4+3] + a[1*4+2]*b[2*4+3] + a[1*4+3]*b[3*4+3];

	out[2*4+0] = a[2*4+0]*b[0*4+0] + a[2*4+1]*b[1*4+0] + a[2*4+2]*b[2*4+0] + a[2*4+3]*b[3*4+0];
	out[2*4+1] = a[2*4+0]*b[0*4+1] + a[2*4+1]*b[1*4+1] + a[2*4+2]*b[2*4+1] + a[2*4+3]*b[3*4+1];
	out[2*4+2] = a[2*4+0]*b[0*4+2] + a[2*4+1]*b[1*4+2] + a[2*4+2]*b[2*4+2] + a[2*4+3]*b[3*4+2];
	out[2*4+3] = a[2*4+0]*b[0*4+3] + a[2*4+1]*b[1*4+3] + a[2*4+2]*b[2*4+3] + a[2*4+3]*b[3*4+3];

	out[3*4+0] = a[3*4+0]*b[0*4+0] + a[3*4+1]*b[1*4+0] + a[3*4+2]*b[2*4+0] + a[3*4+3]*b[3*4+0];
	out[3*4+1] = a[3*4+0]*b[0*4+1] + a[3*4+1]*b[1*4+1] + a[3*4+2]*b[2*4+1] + a[3*4+3]*b[3*4+1];
	out[3*4+2] = a[3*4+0]*b[0*4+2] + a[3*4+1]*b[1*4+2] + a[3*4+2]*b[2*4+2] + a[3*4+3]*b[3*4+2];
	out[3*4+3] = a[3*4+0]*b[0*4+3] + a[3*4+1]*b[1*4+3] + a[3*4+2]*b[2*4+3] + a[3*4+3]*b[3*4+3];

#endif
}

/*
======================
R_MatrixTranspose
======================
*/
void R_MatrixTranspose( const float in[16], float out[16] ) {
	for ( int i = 0; i < 4; i++ ) {
		for ( int j = 0; j < 4; j++ ) {
			out[i*4+j] = in[j*4+i];
		}
	}
}

/*
==========================
R_TransformModelToClip
==========================
*/
void R_TransformModelToClip( const idVec3 &src, const float *modelMatrix, const float *projectionMatrix, idPlane &eye, idPlane &dst ) {
	for ( int i = 0; i < 4; i++ ) {
		eye[i] = 	modelMatrix[i + 0 * 4] * src[0] +
					modelMatrix[i + 1 * 4] * src[1] +
					modelMatrix[i + 2 * 4] * src[2] +
					modelMatrix[i + 3 * 4];
	}

	for ( int i = 0; i < 4; i++ ) {
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
void R_TransformClipToDevice( const idPlane &clip, idVec3 &ndc ) {
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
void R_GlobalToNormalizedDeviceCoordinates( const idVec3 &global, idVec3 &ndc ) {
	idPlane	view;
	idPlane	clip;

	// _D3XP use tr.primaryView when there is no tr.viewDef
	const viewDef_t * viewDef = ( tr.viewDef != NULL ) ? tr.viewDef : tr.primaryView;

	for ( int i = 0; i < 4; i ++ ) {
		view[i] = 	viewDef->worldSpace.modelViewMatrix[i + 0 * 4] * global[0] +
					viewDef->worldSpace.modelViewMatrix[i + 1 * 4] * global[1] +
					viewDef->worldSpace.modelViewMatrix[i + 2 * 4] * global[2] +
					viewDef->worldSpace.modelViewMatrix[i + 3 * 4];
	}

	for ( int i = 0; i < 4; i ++ ) {
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
void R_LocalPointToGlobal( const float modelMatrix[16], const idVec3 &in, idVec3 &out ) {
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
void R_GlobalPointToLocal( const float modelMatrix[16], const idVec3 &in, idVec3 &out ) {
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
void R_LocalVectorToGlobal( const float modelMatrix[16], const idVec3 &in, idVec3 &out ) {
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
void R_GlobalVectorToLocal( const float modelMatrix[16], const idVec3 &in, idVec3 &out ) {
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
void R_GlobalPlaneToLocal( const float modelMatrix[16], const idPlane &in, idPlane &out ) {
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
void R_LocalPlaneToGlobal( const float modelMatrix[16], const idPlane &in, idPlane &out ) {
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
void R_SetupViewMatrix( viewDef_t *viewDef ) {
	static float s_flipMatrix[16] = {
		// convert from our coordinate system (looking down X)
		// to OpenGL's coordinate system (looking down -Z)
		 0, 0, -1, 0,
		-1, 0,  0, 0,
		 0, 1,  0, 0,
		 0, 0,  0, 1
	};

	viewEntity_t *world = &viewDef->worldSpace;
	memset( world, 0, sizeof( *world ) );

	// the model matrix is an identity
	world->modelMatrix[0*4+0] = 1.0f;
	world->modelMatrix[1*4+1] = 1.0f;
	world->modelMatrix[2*4+2] = 1.0f;

	// transform by the camera placement
	const idVec3 & origin = viewDef->renderView.vieworg;
	const idMat3 & axis = viewDef->renderView.viewaxis;

	float viewerMatrix[16];
	viewerMatrix[0*4+0] = axis[0][0];
	viewerMatrix[1*4+0] = axis[0][1];
	viewerMatrix[2*4+0] = axis[0][2];
	viewerMatrix[3*4+0] = - origin[0] * axis[0][0] - origin[1] * axis[0][1] - origin[2] * axis[0][2];

	viewerMatrix[0*4+1] = axis[1][0];
	viewerMatrix[1*4+1] = axis[1][1];
	viewerMatrix[2*4+1] = axis[1][2];
	viewerMatrix[3*4+1] = - origin[0] * axis[1][0] - origin[1] * axis[1][1] - origin[2] * axis[1][2];

	viewerMatrix[0*4+2] = axis[2][0];
	viewerMatrix[1*4+2] = axis[2][1];
	viewerMatrix[2*4+2] = axis[2][2];
	viewerMatrix[3*4+2] = - origin[0] * axis[2][0] - origin[1] * axis[2][1] - origin[2] * axis[2][2];

	viewerMatrix[0*4+3] = 0.0f;
	viewerMatrix[1*4+3] = 0.0f;
	viewerMatrix[2*4+3] = 0.0f;
	viewerMatrix[3*4+3] = 1.0f;

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

void R_SetupProjectionMatrix( viewDef_t *viewDef ) {
	// random jittering is usefull when multiple
	// frames are going to be blended together
	// for motion blurred anti-aliasing
	float jitterx, jittery;
	if ( r_jitter.GetBool() ) {
		static idRandom random;
		jitterx = random.RandomFloat();
		jittery = random.RandomFloat();
	} else {
		jitterx = 0.0f;
		jittery = 0.0f;
	}

	//
	// set up projection matrix
	//
	const float zNear = ( viewDef->renderView.cramZNear ) ? ( r_znear.GetFloat() * 0.25f ) : r_znear.GetFloat();

	float ymax = zNear * tan( viewDef->renderView.fov_y * idMath::PI / 360.0f );
	float ymin = -ymax;

	float xmax = zNear * tan( viewDef->renderView.fov_x * idMath::PI / 360.0f );
	float xmin = -xmax;

	const float width = xmax - xmin;
	const float height = ymax - ymin;

	const int viewWidth = viewDef->viewport.x2 - viewDef->viewport.x1 + 1;
	const int viewHeight = viewDef->viewport.y2 - viewDef->viewport.y1 + 1;

	jitterx = jitterx * width / viewWidth;
	jitterx += r_centerX.GetFloat();
	jitterx += viewDef->renderView.stereoScreenSeparation;
	xmin += jitterx * width;
	xmax += jitterx * width;

	jittery = jittery * height / viewHeight;
	jittery += r_centerY.GetFloat();
	ymin += jittery * height;
	ymax += jittery * height;

	viewDef->projectionMatrix[0*4+0] = 2.0f * zNear / width;
	viewDef->projectionMatrix[1*4+0] = 0.0f;
	viewDef->projectionMatrix[2*4+0] = ( xmax + xmin ) / width;	// normally 0
	viewDef->projectionMatrix[3*4+0] = 0.0f;

	viewDef->projectionMatrix[0*4+1] = 0.0f;
	viewDef->projectionMatrix[1*4+1] = 2.0f * zNear / height;
	viewDef->projectionMatrix[2*4+1] = ( ymax + ymin ) / height;	// normally 0
	viewDef->projectionMatrix[3*4+1] = 0.0f;

	// this is the far-plane-at-infinity formulation, and
	// crunches the Z range slightly so w=0 vertexes do not
	// rasterize right at the wraparound point
	viewDef->projectionMatrix[0*4+2] = 0.0f;
	viewDef->projectionMatrix[1*4+2] = 0.0f;
	viewDef->projectionMatrix[2*4+2] = -0.999f; // adjust value to prevent imprecision issues
	viewDef->projectionMatrix[3*4+2] = -2.0f * zNear;

	viewDef->projectionMatrix[0*4+3] = 0.0f;
	viewDef->projectionMatrix[1*4+3] = 0.0f;
	viewDef->projectionMatrix[2*4+3] = -1.0f;
	viewDef->projectionMatrix[3*4+3] = 0.0f;

	if ( viewDef->renderView.flipProjection ) {
		viewDef->projectionMatrix[1*4+1] = -viewDef->projectionMatrix[1*4+1];
		viewDef->projectionMatrix[1*4+3] = -viewDef->projectionMatrix[1*4+3];
	}
}
