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
#include "precompiled.h"

/*
=================
idSurface_Patch::SetSize
=================
*/
void idSurface_Patch::SetSize( int patchWidth, int patchHeight )
{
	if( patchWidth < 1 || patchWidth > maxWidth )
	{
		idLib::common->FatalError( "idSurface_Patch::SetSize: invalid patchWidth" );
	}
	if( patchHeight < 1 || patchHeight > maxHeight )
	{
		idLib::common->FatalError( "idSurface_Patch::SetSize: invalid patchHeight" );
	}
	width = patchWidth;
	height = patchHeight;
	verts.SetNum( width * height );
}

/*
=================
idSurface_Patch::PutOnCurve

Expects an expanded patch.
=================
*/
void idSurface_Patch::PutOnCurve()
{
	int i, j;
	idDrawVert prev, next;

	assert( expanded == true );
	// put all the approximating points on the curve
	for( i = 0; i < width; i++ )
	{
		for( j = 1; j < height; j += 2 )
		{
			LerpVert( verts[j * maxWidth + i], verts[( j + 1 )*maxWidth + i], prev );
			LerpVert( verts[j * maxWidth + i], verts[( j - 1 )*maxWidth + i], next );
			LerpVert( prev, next, verts[j * maxWidth + i] );
		}
	}

	for( j = 0; j < height; j++ )
	{
		for( i = 1; i < width; i += 2 )
		{
			LerpVert( verts[j * maxWidth + i], verts[j * maxWidth + i + 1], prev );
			LerpVert( verts[j * maxWidth + i], verts[j * maxWidth + i - 1], next );
			LerpVert( prev, next, verts[j * maxWidth + i] );
		}
	}
}

/*
================
idSurface_Patch::ProjectPointOntoVector
================
*/
void idSurface_Patch::ProjectPointOntoVector( const idVec3& point, const idVec3& vStart, const idVec3& vEnd, idVec3& vProj )
{
	idVec3 pVec, vec;

	pVec = point - vStart;
	vec = vEnd - vStart;
	vec.Normalize();
	// project onto the directional vector for this segment
	vProj = vStart + ( pVec * vec ) * vec;
}

/*
================
idSurface_Patch::RemoveLinearColumnsRows

Expects an expanded patch.
================
*/
void idSurface_Patch::RemoveLinearColumnsRows()
{
	int i, j, k;
	float len, maxLength;
	idVec3 proj, dir;

	assert( expanded == true );
	for( j = 1; j < width - 1; j++ )
	{
		maxLength = 0;
		for( i = 0; i < height; i++ )
		{
			idSurface_Patch::ProjectPointOntoVector( verts[i * maxWidth + j].xyz,
					verts[i * maxWidth + j - 1].xyz, verts[i * maxWidth + j + 1].xyz, proj );
			dir = verts[i * maxWidth + j].xyz - proj;
			len = dir.LengthSqr();
			if( len > maxLength )
			{
				maxLength = len;
			}
		}
		if( maxLength < Square( 0.2f ) )
		{
			width--;
			for( i = 0; i < height; i++ )
			{
				for( k = j; k < width; k++ )
				{
					verts[i * maxWidth + k] = verts[i * maxWidth + k + 1];
				}
			}
			j--;
		}
	}
	for( j = 1; j < height - 1; j++ )
	{
		maxLength = 0;
		for( i = 0; i < width; i++ )
		{
			idSurface_Patch::ProjectPointOntoVector( verts[j * maxWidth + i].xyz,
					verts[( j - 1 )*maxWidth + i].xyz, verts[( j + 1 )*maxWidth + i].xyz, proj );
			dir = verts[j * maxWidth + i].xyz - proj;
			len = dir.LengthSqr();
			if( len > maxLength )
			{
				maxLength = len;
			}
		}
		if( maxLength < Square( 0.2f ) )
		{
			height--;
			for( i = 0; i < width; i++ )
			{
				for( k = j; k < height; k++ )
				{
					verts[k * maxWidth + i] = verts[( k + 1 ) * maxWidth + i];
				}
			}
			j--;
		}
	}
}

/*
================
idSurface_Patch::ResizeExpanded
================
*/
void idSurface_Patch::ResizeExpanded( int newHeight, int newWidth )
{
	int i, j;

	assert( expanded == true );
	if( newHeight <= maxHeight && newWidth <= maxWidth )
	{
		return;
	}
	if( newHeight * newWidth > maxHeight * maxWidth )
	{
		verts.SetNum( newHeight * newWidth );
	}
	// space out verts for new height and width
	for( j = maxHeight - 1; j >= 0; j-- )
	{
		for( i = maxWidth - 1; i >= 0; i-- )
		{
			verts[j * newWidth + i] = verts[j * maxWidth + i];
		}
	}
	maxHeight = newHeight;
	maxWidth = newWidth;
}

/*
================
idSurface_Patch::Collapse
================
*/
void idSurface_Patch::Collapse()
{
	int i, j;

	if( !expanded )
	{
		idLib::common->FatalError( "idSurface_Patch::Collapse: patch not expanded" );
	}
	expanded = false;
	if( width != maxWidth )
	{
		for( j = 0; j < height; j++ )
		{
			for( i = 0; i < width; i++ )
			{
				verts[j * width + i] = verts[j * maxWidth + i];
			}
		}
	}
	verts.SetNum( width * height );
}

/*
================
idSurface_Patch::Expand
================
*/
void idSurface_Patch::Expand()
{
	int i, j;

	if( expanded )
	{
		idLib::common->FatalError( "idSurface_Patch::Expand: patch alread expanded" );
	}
	expanded = true;
	verts.SetNum( maxWidth * maxHeight );
	if( width != maxWidth )
	{
		for( j = height - 1; j >= 0; j-- )
		{
			for( i = width - 1; i >= 0; i-- )
			{
				verts[j * maxWidth + i] = verts[j * width + i];
			}
		}
	}
}

/*
============
idSurface_Patch::LerpVert
============
*/
void idSurface_Patch::LerpVert( const idDrawVert& a, const idDrawVert& b, idDrawVert& out ) const
{
	out.xyz[0] = 0.5f * ( a.xyz[0] + b.xyz[0] );
	out.xyz[1] = 0.5f * ( a.xyz[1] + b.xyz[1] );
	out.xyz[2] = 0.5f * ( a.xyz[2] + b.xyz[2] );
	out.SetNormal( ( a.GetNormal() + b.GetNormal() ) * 0.5f );
	out.SetTexCoord( ( a.GetTexCoord() + b.GetTexCoord() ) * 0.5f );
}

/*
=================
idSurface_Patch::GenerateNormals

Handles all the complicated wrapping and degenerate cases
Expects a Not expanded patch.
=================
*/
#define	COPLANAR_EPSILON	0.1f

void idSurface_Patch::GenerateNormals()
{
	int			i, j, k, dist;
	idVec3		norm;
	idVec3		sum;
	int			count;
	idVec3		base;
	idVec3		delta;
	int			x, y;
	idVec3		around[8], temp;
	bool		good[8];
	bool		wrapWidth, wrapHeight;
	static int	neighbors[8][2] =
	{
		{0, 1}, {1, 1}, {1, 0}, {1, -1}, {0, -1}, { -1, -1}, { -1, 0}, { -1, 1}
	};

	assert( expanded == false );

	//
	// if all points are coplanar, set all normals to that plane
	//
	idVec3		extent[3];
	float		offset;

	extent[0] = verts[width - 1].xyz - verts[0].xyz;
	extent[1] = verts[( height - 1 ) * width + width - 1].xyz - verts[0].xyz;
	extent[2] = verts[( height - 1 ) * width].xyz - verts[0].xyz;

	norm = extent[0].Cross( extent[1] );
	if( norm.LengthSqr() == 0.0f )
	{
		norm = extent[0].Cross( extent[2] );
		if( norm.LengthSqr() == 0.0f )
		{
			norm = extent[1].Cross( extent[2] );
		}
	}

	// wrapped patched may not get a valid normal here
	if( norm.Normalize() != 0.0f )
	{

		offset = verts[0].xyz * norm;
		for( i = 1; i < width * height; i++ )
		{
			float d = verts[i].xyz * norm;
			if( idMath::Fabs( d - offset ) > COPLANAR_EPSILON )
			{
				break;
			}
		}

		if( i == width * height )
		{
			// all are coplanar
			for( i = 0; i < width * height; i++ )
			{
				verts[i].SetNormal( norm );
			}
			return;
		}
	}

	// check for wrapped edge cases, which should smooth across themselves
	wrapWidth = false;
	for( i = 0; i < height; i++ )
	{
		delta = verts[i * width].xyz - verts[i * width + width - 1].xyz;
		if( delta.LengthSqr() > Square( 1.0f ) )
		{
			break;
		}
	}
	if( i == height )
	{
		wrapWidth = true;
	}

	wrapHeight = false;
	for( i = 0; i < width; i++ )
	{
		delta = verts[i].xyz - verts[( height - 1 ) * width + i].xyz;
		if( delta.LengthSqr() > Square( 1.0f ) )
		{
			break;
		}
	}
	if( i == width )
	{
		wrapHeight = true;
	}

	for( i = 0; i < width; i++ )
	{
		for( j = 0; j < height; j++ )
		{
			count = 0;
			base = verts[j * width + i].xyz;
			for( k = 0; k < 8; k++ )
			{
				around[k] = vec3_origin;
				good[k] = false;

				for( dist = 1; dist <= 3; dist++ )
				{
					x = i + neighbors[k][0] * dist;
					y = j + neighbors[k][1] * dist;
					if( wrapWidth )
					{
						if( x < 0 )
						{
							x = width - 1 + x;
						}
						else if( x >= width )
						{
							x = 1 + x - width;
						}
					}
					if( wrapHeight )
					{
						if( y < 0 )
						{
							y = height - 1 + y;
						}
						else if( y >= height )
						{
							y = 1 + y - height;
						}
					}

					if( x < 0 || x >= width || y < 0 || y >= height )
					{
						break;					// edge of patch
					}
					temp = verts[y * width + x].xyz - base;
					if( temp.Normalize() == 0.0f )
					{
						continue;				// degenerate edge, get more dist
					}
					else
					{
						good[k] = true;
						around[k] = temp;
						break;					// good edge
					}
				}
			}

			sum = vec3_origin;
			for( k = 0; k < 8; k++ )
			{
				if( !good[k] || !good[( k + 1 ) & 7] )
				{
					continue;	// didn't get two points
				}
				norm = around[( k + 1 ) & 7].Cross( around[k] );
				if( norm.Normalize() == 0.0f )
				{
					continue;
				}
				sum += norm;
				count++;
			}
			if( count == 0 )
			{
				//idLib::common->Printf("bad normal\n");
				count = 1;
			}
			sum.Normalize();
			verts[j * width + i].SetNormal( sum );
		}
	}
}

/*
=================
idSurface_Patch::GenerateIndexes
=================
*/
void idSurface_Patch::GenerateIndexes()
{
	int i, j, v1, v2, v3, v4, index;

	indexes.SetNum( ( width - 1 ) * ( height - 1 ) * 2 * 3 );
	index = 0;
	for( i = 0; i < width - 1; i++ )
	{
		for( j = 0; j < height - 1; j++ )
		{
			v1 = j * width + i;
			v2 = v1 + 1;
			v3 = v1 + width + 1;
			v4 = v1 + width;
			indexes[index++] = v1;
			indexes[index++] = v3;
			indexes[index++] = v2;
			indexes[index++] = v1;
			indexes[index++] = v4;
			indexes[index++] = v3;
		}
	}

	GenerateEdgeIndexes();
}

/*
===============
idSurface_Patch::SampleSinglePatchPoint
===============
*/
void idSurface_Patch::SampleSinglePatchPoint( const idDrawVert ctrl[3][3], float u, float v, idDrawVert* out ) const
{
	float	vCtrl[3][8];
	int		vPoint;
	int		axis;

	// find the control points for the v coordinate
	for( vPoint = 0; vPoint < 3; vPoint++ )
	{
		for( axis = 0; axis < 8; axis++ )
		{
			float a, b, c;
			float qA, qB, qC;
			if( axis < 3 )
			{
				a = ctrl[0][vPoint].xyz[axis];
				b = ctrl[1][vPoint].xyz[axis];
				c = ctrl[2][vPoint].xyz[axis];
			}
			else if( axis < 6 )
			{
				a = ctrl[0][vPoint].GetNormal()[axis - 3];
				b = ctrl[1][vPoint].GetNormal()[axis - 3];
				c = ctrl[2][vPoint].GetNormal()[axis - 3];
			}
			else
			{
				a = ctrl[0][vPoint].GetTexCoord()[axis - 6];
				b = ctrl[1][vPoint].GetTexCoord()[axis - 6];
				c = ctrl[2][vPoint].GetTexCoord()[axis - 6];
			}
			qA = a - 2.0f * b + c;
			qB = 2.0f * b - 2.0f * a;
			qC = a;
			vCtrl[vPoint][axis] = qA * u * u + qB * u + qC;
		}
	}

	// interpolate the v value
	for( axis = 0; axis < 8; axis++ )
	{
		float a, b, c;
		float qA, qB, qC;

		a = vCtrl[0][axis];
		b = vCtrl[1][axis];
		c = vCtrl[2][axis];
		qA = a - 2.0f * b + c;
		qB = 2.0f * b - 2.0f * a;
		qC = a;

		if( axis < 3 )
		{
			out->xyz[axis] = qA * v * v + qB * v + qC;
		}
		else if( axis < 6 )
		{
			idVec3 tempNormal = out->GetNormal();
			tempNormal[axis - 3] = qA * v * v + qB * v + qC;
			out->SetNormal( tempNormal );
			//out->normal[axis-3] = qA * v * v + qB * v + qC;
		}
		else
		{
			idVec2 tempST = out->GetTexCoord();
			tempST[axis - 6] = qA * v * v + qB * v + qC;
			out->SetTexCoord( tempST );
		}
	}
}

/*
===================
idSurface_Patch::SampleSinglePatch
===================
*/
void idSurface_Patch::SampleSinglePatch( const idDrawVert ctrl[3][3], int baseCol, int baseRow, int width, int horzSub, int vertSub, idDrawVert* outVerts ) const
{
	int		i, j;
	float	u, v;

	horzSub++;
	vertSub++;
	for( i = 0; i < horzSub; i++ )
	{
		for( j = 0; j < vertSub; j++ )
		{
			u = ( float ) i / ( horzSub - 1 );
			v = ( float ) j / ( vertSub - 1 );
			SampleSinglePatchPoint( ctrl, u, v, &outVerts[( ( baseRow + j ) * width ) + i + baseCol] );
		}
	}
}

/*
=================
idSurface_Patch::SubdivideExplicit
=================
*/
void idSurface_Patch::SubdivideExplicit( int horzSubdivisions, int vertSubdivisions, bool genNormals, bool removeLinear )
{
	int i, j, k, l;
	idDrawVert sample[3][3];
	int outWidth = ( ( width - 1 ) / 2 * horzSubdivisions ) + 1;
	int outHeight = ( ( height - 1 ) / 2 * vertSubdivisions ) + 1;
	idDrawVert* dv = new( TAG_IDLIB_SURFACE ) idDrawVert[ outWidth * outHeight ];

	// generate normals for the control mesh
	if( genNormals )
	{
		GenerateNormals();
	}

	int baseCol = 0;
	for( i = 0; i + 2 < width; i += 2 )
	{
		int baseRow = 0;
		for( j = 0; j + 2 < height; j += 2 )
		{
			for( k = 0; k < 3; k++ )
			{
				for( l = 0; l < 3; l++ )
				{
					sample[k][l] = verts[( ( j + l ) * width ) + i + k ];
				}
			}
			SampleSinglePatch( sample, baseCol, baseRow, outWidth, horzSubdivisions, vertSubdivisions, dv );
			baseRow += vertSubdivisions;
		}
		baseCol += horzSubdivisions;
	}
	verts.SetNum( outWidth * outHeight );
	for( i = 0; i < outWidth * outHeight; i++ )
	{
		verts[i] = dv[i];
	}

	delete[] dv;

	width = maxWidth = outWidth;
	height = maxHeight = outHeight;
	expanded = false;

	if( removeLinear )
	{
		Expand();
		RemoveLinearColumnsRows();
		Collapse();
	}

	// normalize all the lerped normals
	if( genNormals )
	{
		idVec3 tempNormal;
		for( i = 0; i < width * height; i++ )
		{
			tempNormal = verts[i].GetNormal();
			tempNormal.Normalize();
			verts[i].SetNormal( tempNormal );
		}
	}

	GenerateIndexes();
}

/*
=================
idSurface_Patch::Subdivide
=================
*/
void idSurface_Patch::Subdivide( float maxHorizontalError, float maxVerticalError, float maxLength, bool genNormals )
{
	int			i, j, k, l;
	idDrawVert	prev, next, mid;
	idVec3		prevxyz, nextxyz, midxyz;
	idVec3		delta;
	float		maxHorizontalErrorSqr, maxVerticalErrorSqr, maxLengthSqr;

	// generate normals for the control mesh
	if( genNormals )
	{
		GenerateNormals();
	}

	maxHorizontalErrorSqr = Square( maxHorizontalError );
	maxVerticalErrorSqr = Square( maxVerticalError );
	maxLengthSqr = Square( maxLength );

	Expand();

	// horizontal subdivisions
	for( j = 0; j + 2 < width; j += 2 )
	{
		// check subdivided midpoints against control points
		for( i = 0; i < height; i++ )
		{
			for( l = 0; l < 3; l++ )
			{
				prevxyz[l] = verts[i * maxWidth + j + 1].xyz[l] - verts[i * maxWidth + j  ].xyz[l];
				nextxyz[l] = verts[i * maxWidth + j + 2].xyz[l] - verts[i * maxWidth + j + 1].xyz[l];
				midxyz[l] = ( verts[i * maxWidth + j  ].xyz[l] + verts[i * maxWidth + j + 1].xyz[l] * 2.0f +
							  verts[i * maxWidth + j + 2].xyz[l] ) * 0.25f;
			}

			if( maxLength > 0.0f )
			{
				// if the span length is too long, force a subdivision
				if( prevxyz.LengthSqr() > maxLengthSqr || nextxyz.LengthSqr() > maxLengthSqr )
				{
					break;
				}
			}
			// see if this midpoint is off far enough to subdivide
			delta = verts[i * maxWidth + j + 1].xyz - midxyz;
			if( delta.LengthSqr() > maxHorizontalErrorSqr )
			{
				break;
			}
		}

		if( i == height )
		{
			continue;	// didn't need subdivision
		}

		if( width + 2 >= maxWidth )
		{
			ResizeExpanded( maxHeight, maxWidth + 4 );
		}

		// insert two columns and replace the peak
		width += 2;

		for( i = 0; i < height; i++ )
		{
			idSurface_Patch::LerpVert( verts[i * maxWidth + j  ], verts[i * maxWidth + j + 1], prev );
			idSurface_Patch::LerpVert( verts[i * maxWidth + j + 1], verts[i * maxWidth + j + 2], next );
			idSurface_Patch::LerpVert( prev, next, mid );

			for( k = width - 1; k > j + 3; k-- )
			{
				verts[i * maxWidth + k] = verts[i * maxWidth + k - 2];
			}
			verts[i * maxWidth + j + 1] = prev;
			verts[i * maxWidth + j + 2] = mid;
			verts[i * maxWidth + j + 3] = next;
		}

		// back up and recheck this set again, it may need more subdivision
		j -= 2;
	}

	// vertical subdivisions
	for( j = 0; j + 2 < height; j += 2 )
	{
		// check subdivided midpoints against control points
		for( i = 0; i < width; i++ )
		{
			for( l = 0; l < 3; l++ )
			{
				prevxyz[l] = verts[( j + 1 ) * maxWidth + i].xyz[l] - verts[j * maxWidth + i].xyz[l];
				nextxyz[l] = verts[( j + 2 ) * maxWidth + i].xyz[l] - verts[( j + 1 ) * maxWidth + i].xyz[l];
				midxyz[l] = ( verts[j * maxWidth + i].xyz[l] + verts[( j + 1 ) * maxWidth + i].xyz[l] * 2.0f +
							  verts[( j + 2 ) * maxWidth + i].xyz[l] ) * 0.25f;
			}

			if( maxLength > 0.0f )
			{
				// if the span length is too long, force a subdivision
				if( prevxyz.LengthSqr() > maxLengthSqr || nextxyz.LengthSqr() > maxLengthSqr )
				{
					break;
				}
			}
			// see if this midpoint is off far enough to subdivide
			delta = verts[( j + 1 ) * maxWidth + i].xyz - midxyz;
			if( delta.LengthSqr() > maxVerticalErrorSqr )
			{
				break;
			}
		}

		if( i == width )
		{
			continue;	// didn't need subdivision
		}

		if( height + 2 >= maxHeight )
		{
			ResizeExpanded( maxHeight + 4, maxWidth );
		}

		// insert two columns and replace the peak
		height += 2;

		for( i = 0; i < width; i++ )
		{
			LerpVert( verts[j * maxWidth + i], verts[( j + 1 )*maxWidth + i], prev );
			LerpVert( verts[( j + 1 )*maxWidth + i], verts[( j + 2 )*maxWidth + i], next );
			LerpVert( prev, next, mid );

			for( k = height - 1; k > j + 3; k-- )
			{
				verts[k * maxWidth + i] = verts[( k - 2 ) * maxWidth + i];
			}
			verts[( j + 1 )*maxWidth + i] = prev;
			verts[( j + 2 )*maxWidth + i] = mid;
			verts[( j + 3 )*maxWidth + i] = next;
		}

		// back up and recheck this set again, it may need more subdivision
		j -= 2;
	}

	PutOnCurve();

	RemoveLinearColumnsRows();

	Collapse();

	// normalize all the lerped normals
	if( genNormals )
	{
		idVec3 tempNormal;
		for( i = 0; i < width * height; i++ )
		{
			tempNormal = verts[i].GetNormal();
			tempNormal.Normalize();
			verts[i].SetNormal( tempNormal );
		}
	}

	GenerateIndexes();
}
