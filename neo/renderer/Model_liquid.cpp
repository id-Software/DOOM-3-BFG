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

#include "precompiled.h"
#pragma hdrstop


#include "RenderCommon.h"
#include "Model_local.h"

#define LIQUID_MAX_SKIP_FRAMES	5
#define LIQUID_MAX_TYPES		3

/*
====================
idRenderModelLiquid::idRenderModelLiquid
====================
*/
idRenderModelLiquid::idRenderModelLiquid()
{
	verts_x		= 32;
	verts_y		= 32;
	scale_x		= 256.0f;
	scale_y		= 256.0f;
	liquid_type = 0;
	density		= 0.97f;
	drop_height = 4;
	drop_radius = 4;
	drop_delay	= 1000;
	shader		= declManager->FindMaterial( NULL );
	update_tics	= 33;  // ~30 hz
	time		= 0;
	seed		= 0;

	random.SetSeed( 0 );
}

/*
====================
idRenderModelLiquid::GenerateSurface
====================
*/
modelSurface_t idRenderModelLiquid::GenerateSurface( float lerp )
{
	srfTriangles_t*	tri;
	int				i, base;
	idDrawVert*		vert;
	modelSurface_t	surf;
	float			inv_lerp;

	inv_lerp = 1.0f - lerp;
	vert = verts.Ptr();
	for( i = 0; i < verts.Num(); i++, vert++ )
	{
		vert->xyz.z = page1[ i ] * lerp + page2[ i ] * inv_lerp;
	}

	tr.pc.c_deformedSurfaces++;
	tr.pc.c_deformedVerts += deformInfo->numOutputVerts;
	tr.pc.c_deformedIndexes += deformInfo->numIndexes;

	tri = R_AllocStaticTriSurf();

	// note that some of the data is references, and should not be freed
	tri->referencedIndexes = true;

	tri->numIndexes = deformInfo->numIndexes;
	tri->indexes = deformInfo->indexes;
	tri->silIndexes = deformInfo->silIndexes;
	tri->numMirroredVerts = deformInfo->numMirroredVerts;
	tri->mirroredVerts = deformInfo->mirroredVerts;
	tri->numDupVerts = deformInfo->numDupVerts;
	tri->dupVerts = deformInfo->dupVerts;

	tri->numVerts = deformInfo->numOutputVerts;
	R_AllocStaticTriSurfVerts( tri, tri->numVerts );
	SIMDProcessor->Memcpy( tri->verts, verts.Ptr(), deformInfo->numSourceVerts * sizeof( tri->verts[0] ) );

	// replicate the mirror seam vertexes
	base = deformInfo->numOutputVerts - deformInfo->numMirroredVerts;
	for( i = 0 ; i < deformInfo->numMirroredVerts ; i++ )
	{
		tri->verts[base + i] = tri->verts[deformInfo->mirroredVerts[i]];
	}

	R_BoundTriSurf( tri );

	surf.geometry	= tri;
	surf.shader		= shader;

	return surf;
}

/*
====================
idRenderModelLiquid::WaterDrop
====================
*/
void idRenderModelLiquid::WaterDrop( int x, int y, float* page )
{
	int		cx, cy;
	int		left, top, right, bottom;
	int		square;
	int		radsquare = drop_radius * drop_radius;
	float	invlength = 1.0f / ( float )radsquare;
	float	dist;

	if( x < 0 )
	{
		x = 1 + drop_radius + random.RandomInt( verts_x - 2 * drop_radius - 1 );
	}
	if( y < 0 )
	{
		y = 1 + drop_radius + random.RandomInt( verts_y - 2 * drop_radius - 1 );
	}

	left = -drop_radius;
	right = drop_radius;
	top = -drop_radius;
	bottom = drop_radius;

	// Perform edge clipping...
	if( x - drop_radius < 1 )
	{
		left -= ( x - drop_radius - 1 );
	}
	if( y - drop_radius < 1 )
	{
		top -= ( y - drop_radius - 1 );
	}
	if( x + drop_radius > verts_x - 1 )
	{
		right -= ( x + drop_radius - verts_x + 1 );
	}
	if( y + drop_radius > verts_y - 1 )
	{
		bottom -= ( y + drop_radius - verts_y + 1 );
	}

	for( cy = top; cy < bottom; cy++ )
	{
		for( cx = left; cx < right; cx++ )
		{
			square = cy * cy + cx * cx;
			if( square < radsquare )
			{
				dist = idMath::Sqrt( ( float )square * invlength );
				page[verts_x * ( cy + y ) + cx + x] += idMath::Cos16( dist * idMath::PI * 0.5f ) * drop_height;
			}
		}
	}
}

/*
====================
idRenderModelLiquid::IntersectBounds
====================
*/
void idRenderModelLiquid::IntersectBounds( const idBounds& bounds, float displacement )
{
	int		cx, cy;
	int		left, top, right, bottom;
	float	up, down;
	float*	pos;

	left	= ( int )( bounds[ 0 ].x / scale_x );
	right	= ( int )( bounds[ 1 ].x / scale_x );
	top		= ( int )( bounds[ 0 ].y / scale_y );
	bottom	= ( int )( bounds[ 1 ].y / scale_y );
	down	= bounds[ 0 ].z;
	up		= bounds[ 1 ].z;

	if( ( right < 1 ) || ( left >= verts_x ) || ( bottom < 1 ) || ( top >= verts_x ) )
	{
		return;
	}

	// Perform edge clipping...
	if( left < 1 )
	{
		left = 1;
	}
	if( right >= verts_x )
	{
		right = verts_x - 1;
	}
	if( top < 1 )
	{
		top = 1;
	}
	if( bottom >= verts_y )
	{
		bottom = verts_y - 1;
	}

	for( cy = top; cy < bottom; cy++ )
	{
		for( cx = left; cx < right; cx++ )
		{
			pos = &page1[ verts_x * cy + cx ];
			if( *pos > down )   //&& ( *pos < up ) ) {
			{
				*pos = down;
			}
		}
	}
}

/*
====================
idRenderModelLiquid::Update
====================
*/
void idRenderModelLiquid::Update()
{
	int		x, y;
	float*	p2;
	float*	p1;
	float	value;

	time += update_tics;

	SwapValues( page1, page2 );

	if( time > nextDropTime )
	{
		WaterDrop( -1, -1, page2 );
		nextDropTime = time + drop_delay;
	}
	else if( time < nextDropTime - drop_delay )
	{
		nextDropTime = time + drop_delay;
	}

	p1 = page1;
	p2 = page2;

	switch( liquid_type )
	{
		case 0 :
			for( y = 1; y < verts_y - 1; y++ )
			{
				p2 += verts_x;
				p1 += verts_x;
				for( x = 1; x < verts_x - 1; x++ )
				{
					value =
						( p2[ x + verts_x ] +
						  p2[ x - verts_x ] +
						  p2[ x + 1 ] +
						  p2[ x - 1 ] +
						  p2[ x - verts_x - 1 ] +
						  p2[ x - verts_x + 1 ] +
						  p2[ x + verts_x - 1 ] +
						  p2[ x + verts_x + 1 ] +
						  p2[ x ] ) * ( 2.0f / 9.0f ) -
						p1[ x ];

					p1[ x ] = value * density;
				}
			}
			break;

		case 1 :
			for( y = 1; y < verts_y - 1; y++ )
			{
				p2 += verts_x;
				p1 += verts_x;
				for( x = 1; x < verts_x - 1; x++ )
				{
					value =
						( p2[ x + verts_x ] +
						  p2[ x - verts_x ] +
						  p2[ x + 1 ] +
						  p2[ x - 1 ] +
						  p2[ x - verts_x - 1 ] +
						  p2[ x - verts_x + 1 ] +
						  p2[ x + verts_x - 1 ] +
						  p2[ x + verts_x + 1 ] ) * 0.25f -
						p1[ x ];

					p1[ x ] = value * density;
				}
			}
			break;

		case 2 :
			for( y = 1; y < verts_y - 1; y++ )
			{
				p2 += verts_x;
				p1 += verts_x;
				for( x = 1; x < verts_x - 1; x++ )
				{
					value =
						( p2[ x + verts_x ] +
						  p2[ x - verts_x ] +
						  p2[ x + 1 ] +
						  p2[ x - 1 ] +
						  p2[ x - verts_x - 1 ] +
						  p2[ x - verts_x + 1 ] +
						  p2[ x + verts_x - 1 ] +
						  p2[ x + verts_x + 1 ] +
						  p2[ x ] ) * ( 1.0f / 9.0f );

					p1[ x ] = value * density;
				}
			}
			break;
	}
}

/*
====================
idRenderModelLiquid::Reset
====================
*/
void idRenderModelLiquid::Reset()
{
	int	i, x, y;

	if( pages.Num() < 2 * verts_x * verts_y )
	{
		return;
	}

	nextDropTime = 0;
	time = 0;
	random.SetSeed( seed );

	page1 = pages.Ptr();
	page2 = page1 + verts_x * verts_y;

	for( i = 0, y = 0; y < verts_y; y++ )
	{
		for( x = 0; x < verts_x; x++, i++ )
		{
			page1[ i ] = 0.0f;
			page2[ i ] = 0.0f;
			verts[ i ].xyz.z = 0.0f;
		}
	}
}

/*
====================
idRenderModelLiquid::InitFromFile
====================
*/
void idRenderModelLiquid::InitFromFile( const char* fileName, nvrhi::ICommandList* commandList, const idImportOptions* options )
{
	int				i, x, y;
	idToken			token;
	idParser		parser( LEXFL_ALLOWPATHNAMES | LEXFL_NOSTRINGESCAPECHARS );
	idList<int>		tris;
	float			size_x, size_y;
	float			rate;

	name = fileName;

	if( !parser.LoadFile( fileName ) )
	{
		MakeDefaultModel();
		return;
	}

	size_x = scale_x * verts_x;
	size_y = scale_y * verts_y;

	while( parser.ReadToken( &token ) )
	{
		if( !token.Icmp( "seed" ) )
		{
			seed = parser.ParseInt();
		}
		else if( !token.Icmp( "size_x" ) )
		{
			size_x = parser.ParseFloat();
		}
		else if( !token.Icmp( "size_y" ) )
		{
			size_y = parser.ParseFloat();
		}
		else if( !token.Icmp( "verts_x" ) )
		{
			verts_x = parser.ParseFloat();
			if( verts_x < 2 )
			{
				parser.Warning( "Invalid # of verts.  Using default model." );
				MakeDefaultModel();
				return;
			}
		}
		else if( !token.Icmp( "verts_y" ) )
		{
			verts_y = parser.ParseFloat();
			if( verts_y < 2 )
			{
				parser.Warning( "Invalid # of verts.  Using default model." );
				MakeDefaultModel();
				return;
			}
		}
		else if( !token.Icmp( "liquid_type" ) )
		{
			liquid_type = parser.ParseInt() - 1;
			if( ( liquid_type < 0 ) || ( liquid_type >= LIQUID_MAX_TYPES ) )
			{
				parser.Warning( "Invalid liquid_type.  Using default model." );
				MakeDefaultModel();
				return;
			}
		}
		else if( !token.Icmp( "density" ) )
		{
			density = parser.ParseFloat();
		}
		else if( !token.Icmp( "drop_height" ) )
		{
			drop_height = parser.ParseFloat();
		}
		else if( !token.Icmp( "drop_radius" ) )
		{
			drop_radius = parser.ParseInt();
		}
		else if( !token.Icmp( "drop_delay" ) )
		{
			drop_delay = SEC2MS( parser.ParseFloat() );
		}
		else if( !token.Icmp( "shader" ) )
		{
			parser.ReadToken( &token );
			shader = declManager->FindMaterial( token );
		}
		else if( !token.Icmp( "update_rate" ) )
		{
			rate = parser.ParseFloat();
			if( ( rate <= 0.0f ) || ( rate > 60.0f ) )
			{
				parser.Warning( "Invalid update_rate.  Must be between 0 and 60.  Using default model." );
				MakeDefaultModel();
				return;
			}
			update_tics = 1000 / rate;
		}
		else
		{
			parser.Warning( "Unknown parameter '%s'.  Using default model.", token.c_str() );
			MakeDefaultModel();
			return;
		}
	}

	scale_x = size_x / ( verts_x - 1 );
	scale_y = size_y / ( verts_y - 1 );

	pages.SetNum( 2 * verts_x * verts_y );
	page1 = pages.Ptr();
	page2 = page1 + verts_x * verts_y;

	verts.SetNum( verts_x * verts_y );
	for( i = 0, y = 0; y < verts_y; y++ )
	{
		for( x = 0; x < verts_x; x++, i++ )
		{
			page1[ i ] = 0.0f;
			page2[ i ] = 0.0f;
			verts[ i ].Clear();
			verts[ i ].xyz.Set( x * scale_x, y * scale_y, 0.0f );
			verts[ i ].SetTexCoord( ( float ) x / ( float )( verts_x - 1 ), ( float ) - y / ( float )( verts_y - 1 ) );
		}
	}

	tris.SetNum( ( verts_x - 1 ) * ( verts_y - 1 ) * 6 );
	for( i = 0, y = 0; y < verts_y - 1; y++ )
	{
		for( x = 1; x < verts_x; x++, i += 6 )
		{
			tris[ i + 0 ] = y * verts_x + x;
			tris[ i + 1 ] = y * verts_x + x - 1;
			tris[ i + 2 ] = ( y + 1 ) * verts_x + x - 1;

			tris[ i + 3 ] = ( y + 1 ) * verts_x + x - 1;
			tris[ i + 4 ] = ( y + 1 ) * verts_x + x;
			tris[ i + 5 ] = y * verts_x + x;
		}
	}

	// build the information that will be common to all animations of this mesh:
	// sil edge connectivity and normal / tangent generation information
	deformInfo = R_BuildDeformInfo( verts.Num(), verts.Ptr(), tris.Num(), tris.Ptr(), true );

	bounds.Clear();
	bounds.AddPoint( idVec3( 0.0f, 0.0f, drop_height * -10.0f ) );
	bounds.AddPoint( idVec3( ( verts_x - 1 ) * scale_x, ( verts_y - 1 ) * scale_y, drop_height * 10.0f ) );

	// set the timestamp for reloadmodels
	fileSystem->ReadFile( name, NULL, &timeStamp );

	Reset();
}

/*
====================
idRenderModelLiquid::InstantiateDynamicModel
====================
*/
idRenderModel* idRenderModelLiquid::InstantiateDynamicModel( const struct renderEntity_s* ent, const viewDef_t* view, idRenderModel* cachedModel )
{
	idRenderModelStatic*	staticModel;
	int		frames;
	int		t;
	float	lerp;

	if( cachedModel )
	{
		delete cachedModel;
		cachedModel = NULL;
	}

	if( !deformInfo )
	{
		return NULL;
	}

	if( !view )
	{
		t = 0;
	}
	else
	{
		t = view->renderView.time[0];
	}

	// update the liquid model
	frames = ( t - time ) / update_tics;
	if( frames > LIQUID_MAX_SKIP_FRAMES )
	{
		// don't let time accumalate when skipping frames
		time += update_tics * ( frames - LIQUID_MAX_SKIP_FRAMES );

		frames = LIQUID_MAX_SKIP_FRAMES;
	}

	while( frames > 0 )
	{
		Update();
		frames--;
	}

	// create the surface
	lerp = ( float )( t - time ) / ( float )update_tics;
	modelSurface_t surf = GenerateSurface( lerp );

	staticModel = new( TAG_MODEL ) idRenderModelStatic;
	staticModel->AddSurface( surf );
	staticModel->bounds = surf.geometry->bounds;

	return staticModel;
}

/*
====================
idRenderModelLiquid::IsDynamicModel
====================
*/
dynamicModel_t idRenderModelLiquid::IsDynamicModel() const
{
	return DM_CONTINUOUS;
}

/*
====================
idRenderModelLiquid::Bounds
====================
*/
idBounds idRenderModelLiquid::Bounds( const struct renderEntity_s* ent ) const
{
	// FIXME: need to do this better
	return bounds;
}

/*
====================
idRenderModelLiquid::CreateBuffers
====================
*/
void idRenderModelLiquid::CreateBuffers( nvrhi::ICommandList* commandList )
{
	R_CreateDeformStaticVertices( deformInfo, commandList );
}
