/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "precompiled.h"
#pragma hdrstop

#include "Brush.h"

#define BRUSH_EPSILON					0.1f
#define BRUSH_PLANE_NORMAL_EPSILON		0.00001f
#define BRUSH_PLANE_DIST_EPSILON		0.01f

#define OUTPUT_UPDATE_TIME				500		// update every 500 msec

//#define OUTPUT_CHOP_STATS

/*
============
DisplayRealTimeString
============
*/
void DisplayRealTimeString( const char* string, ... )
{
	va_list argPtr;
	char buf[MAX_STRING_CHARS];
	static int lastUpdateTime;
	int time;

	time = Sys_Milliseconds();
	if( time > lastUpdateTime + OUTPUT_UPDATE_TIME )
	{
		va_start( argPtr, string );
		idStr::vsnPrintf( buf, sizeof( buf ), string, argPtr );
		va_end( argPtr );
		common->Printf( buf );
		lastUpdateTime = time;
	}
}


//===============================================================
//
//	idBrushSide
//
//===============================================================

/*
============
idBrushSide::idBrushSide
============
*/
idBrushSide::idBrushSide()
{
	flags = 0;
	planeNum = -1;
	winding = NULL;
}

/*
============
idBrushSide::idBrushSide
============
*/
idBrushSide::idBrushSide( const idPlane& plane, int planeNum )
{
	this->flags = 0;
	this->plane = plane;
	this->planeNum = planeNum;
	this->winding = NULL;
}

/*
============
idBrushSide::~idBrushSide
============
*/
idBrushSide::~idBrushSide()
{
	if( winding )
	{
		delete winding;
	}
}

/*
============
idBrushSide::Copy
============
*/
idBrushSide* idBrushSide::Copy() const
{
	idBrushSide* side;

	side = new idBrushSide( plane, planeNum );
	side->flags = flags;
	if( winding )
	{
		side->winding = winding->Copy();
	}
	else
	{
		side->winding = NULL;
	}
	return side;
}

/*
============
idBrushSide::Split
============
*/
int idBrushSide::Split( const idPlane& splitPlane, idBrushSide** front, idBrushSide** back ) const
{
	idWinding* frontWinding, *backWinding;

	assert( winding );

	*front = *back = NULL;

	winding->Split( splitPlane, 0.0f, &frontWinding, &backWinding );

	if( frontWinding )
	{
		( *front ) = new idBrushSide( plane, planeNum );
		( *front )->winding = frontWinding;
		( *front )->flags = flags;
	}

	if( backWinding )
	{
		( *back ) = new idBrushSide( plane, planeNum );
		( *back )->winding = backWinding;
		( *back )->flags = flags;
	}

	if( frontWinding && backWinding )
	{
		return PLANESIDE_CROSS;
	}
	else if( frontWinding )
	{
		return PLANESIDE_FRONT;
	}
	else
	{
		return PLANESIDE_BACK;
	}
}


//===============================================================
//
//	idBrushSide
//
//===============================================================

/*
============
idBrush::idBrush
============
*/
idBrush::idBrush()
{
	contents = flags = 0;
	bounds.Clear();
	sides.Clear();
	windingsValid = false;
}


/*
============
idBrush::~idBrush
============
*/
idBrush::~idBrush()
{
	for( int i = 0; i < sides.Num(); i++ )
	{
		delete sides[i];
	}
}

/*
============
idBrush::RemoveSidesWithoutWinding
============
*/
bool idBrush::RemoveSidesWithoutWinding()
{
	int i;

	for( i = 0; i < sides.Num(); i++ )
	{

		if( sides[i]->winding )
		{
			continue;
		}

		sides.RemoveIndex( i );
		i--;
	}

	return ( sides.Num() >= 4 );
}

/*
============
idBrush::CreateWindings
============
*/
bool idBrush::CreateWindings()
{
	int i, j;
	idBrushSide* side;

	bounds.Clear();
	for( i = 0; i < sides.Num(); i++ )
	{
		side = sides[i];

		if( side->winding )
		{
			delete side->winding;
		}

		side->winding = new idWinding( side->plane.Normal(), side->plane.Dist() );

		for( j = 0; j < sides.Num() && side->winding; j++ )
		{
			if( i == j )
			{
				continue;
			}
			// keep the winding if on the clip plane
			side->winding = side->winding->Clip( -sides[j]->plane, BRUSH_EPSILON, true );
		}

		if( side->winding )
		{
			for( j = 0; j < side->winding->GetNumPoints(); j++ )
			{
				bounds.AddPoint( ( *side->winding )[j].ToVec3() );
			}
		}
	}

	if( bounds[0][0] > bounds[1][0] )
	{
		return false;
	}
	for( i = 0; i < 3; i++ )
	{
		if( bounds[0][i] < MIN_WORLD_COORD || bounds[1][i] > MAX_WORLD_COORD )
		{
			return false;
		}
	}

	windingsValid = true;

	return true;
}

/*
============
idBrush::BoundBrush
============
*/
void idBrush::BoundBrush( const idBrush* original )
{
	int i, j;
	idBrushSide* side;
	idWinding* w;

	assert( windingsValid );

	bounds.Clear();
	for( i = 0; i < sides.Num(); i++ )
	{
		side = sides[i];

		w = side->winding;

		if( !w )
		{
			continue;
		}

		for( j = 0; j < w->GetNumPoints(); j++ )
		{
			bounds.AddPoint( ( *w )[j].ToVec3() );
		}
	}

	if( bounds[0][0] > bounds[1][0] )
	{
		if( original )
		{
			idBrushMap* bm = new idBrushMap( "error_brush", "_original" );
			bm->WriteBrush( original );
			delete bm;
		}
		common->Error( "idBrush::BoundBrush: brush %d on entity %d without windings", primitiveNum, entityNum );
	}

	for( i = 0; i < 3; i++ )
	{
		if( bounds[0][i] < MIN_WORLD_COORD || bounds[1][i] > MAX_WORLD_COORD )
		{
			if( original )
			{
				idBrushMap* bm = new idBrushMap( "error_brush", "_original" );
				bm->WriteBrush( original );
				delete bm;
			}
			common->Error( "idBrush::BoundBrush: brush %d on entity %d is unbounded", primitiveNum, entityNum );
		}
	}
}

/*
============
idBrush::FromSides
============
*/
bool idBrush::FromSides( idList<idBrushSide*>& sideList )
{
	int i;

	for( i = 0; i < sideList.Num(); i++ )
	{
		sides.Append( sideList[i] );
	}

	sideList.Clear();

	return CreateWindings();
}

/*
============
idBrush::FromWinding
============
*/
bool idBrush::FromWinding( const idWinding& w, const idPlane& windingPlane )
{
	int i, j, bestAxis;
	idPlane plane;
	idVec3 normal, axialNormal;

	sides.Append( new idBrushSide( windingPlane, -1 ) );
	sides.Append( new idBrushSide( -windingPlane, -1 ) );

	bestAxis = 0;
	for( i = 1; i < 3; i++ )
	{
		if( idMath::Fabs( windingPlane.Normal()[i] ) > idMath::Fabs( windingPlane.Normal()[bestAxis] ) )
		{
			bestAxis = i;
		}
	}
	axialNormal = vec3_origin;
	if( windingPlane.Normal()[bestAxis] > 0.0f )
	{
		axialNormal[bestAxis] = 1.0f;
	}
	else
	{
		axialNormal[bestAxis] = -1.0f;
	}

	for( i = 0; i < w.GetNumPoints(); i++ )
	{
		j = ( i + 1 ) % w.GetNumPoints();
		normal = ( w[j].ToVec3() - w[i].ToVec3() ).Cross( axialNormal );
		if( normal.Normalize() < 0.5f )
		{
			continue;
		}
		plane.SetNormal( normal );
		plane.FitThroughPoint( w[j].ToVec3() );
		sides.Append( new idBrushSide( plane, -1 ) );
	}

	if( sides.Num() < 4 )
	{
		for( i = 0; i < sides.Num(); i++ )
		{
			delete sides[i];
		}
		sides.Clear();
		return false;
	}

	sides[0]->winding = w.Copy();
	windingsValid = true;
	BoundBrush();

	return true;
}

/*
============
idBrush::FromBounds
============
*/
bool idBrush::FromBounds( const idBounds& bounds )
{
	int axis, dir;
	idVec3 normal;
	idPlane plane;

	for( axis = 0; axis < 3; axis++ )
	{
		for( dir = -1; dir <= 1; dir += 2 )
		{
			normal = vec3_origin;
			normal[axis] = dir;
			plane.SetNormal( normal );
			plane.SetDist( dir * bounds[( dir == 1 )][axis] );
			sides.Append( new idBrushSide( plane, -1 ) );
		}
	}

	return CreateWindings();
}

/*
============
idBrush::Transform
============
*/
void idBrush::Transform( const idVec3& origin, const idMat3& axis )
{
	int i;
	bool transformed = false;

	if( axis.IsRotated() )
	{
		for( i = 0; i < sides.Num(); i++ )
		{
			sides[i]->plane.RotateSelf( vec3_origin, axis );
		}
		transformed = true;
	}
	if( origin != vec3_origin )
	{
		for( i = 0; i < sides.Num(); i++ )
		{
			sides[i]->plane.TranslateSelf( origin );
		}
		transformed = true;
	}
	if( transformed )
	{
		CreateWindings();
	}
}

/*
============
idBrush::GetVolume
============
*/
float idBrush::GetVolume() const
{
	int i;
	idWinding* w;
	idVec3 corner;
	float d, area, volume;

	// grab the first valid point as a corner
	w = NULL;
	for( i = 0; i < sides.Num(); i++ )
	{
		w = sides[i]->winding;
		if( w )
		{
			break;
		}
	}
	if( !w )
	{
		return 0.0f;
	}
	corner = ( *w )[0].ToVec3();

	// create tetrahedrons to all other sides
	volume = 0.0f;
	for( ; i < sides.Num(); i++ )
	{
		w = sides[i]->winding;
		if( !w )
		{
			continue;
		}
		d = -( corner * sides[i]->plane.Normal() - sides[i]->plane.Dist() );
		area = w->GetArea();
		volume += d * area;
	}

	return ( volume * ( 1.0f / 3.0f ) );
}

/*
============
idBrush::Subtract
============
*/
bool idBrush::Subtract( const idBrush* b, idBrushList& list ) const
{
	int i;
	idBrush* front, *back;
	const idBrush* in;

	list.Clear();
	in = this;
	for( i = 0; i < b->sides.Num() && in; i++ )
	{

		in->Split( b->sides[i]->plane, b->sides[i]->planeNum, &front, &back );

		if( in != this )
		{
			delete in;
		}
		if( front )
		{
			list.AddToTail( front );
		}
		in = back;
	}
	// if didn't really intersect
	if( !in )
	{
		list.Free();
		return false;
	}

	delete in;
	return true;
}

/*
============
idBrush::TryMerge
============
*/
bool idBrush::TryMerge( const idBrush* brush, const idPlaneSet& planeList )
{
	int i, j, k, l, m, seperatingPlane;
	const idBrush* brushes[2];
	const idWinding* w;
	const idPlane* plane;

	// brush bounds should overlap
	for( i = 0; i < 3; i++ )
	{
		if( bounds[0][i] > brush->bounds[1][i] + 0.1f )
		{
			return false;
		}
		if( bounds[1][i] < brush->bounds[0][i] - 0.1f )
		{
			return false;
		}
	}

	// the brushes should share an opposite plane
	seperatingPlane = -1;
	for( i = 0; i < GetNumSides(); i++ )
	{
		for( j = 0; j < brush->GetNumSides(); j++ )
		{
			if( GetSide( i )->GetPlaneNum() == ( brush->GetSide( j )->GetPlaneNum() ^ 1 ) )
			{
				// may only have one seperating plane
				if( seperatingPlane != -1 )
				{
					return false;
				}
				seperatingPlane = GetSide( i )->GetPlaneNum();
				break;
			}
		}
	}
	if( seperatingPlane == -1 )
	{
		return false;
	}

	brushes[0] = this;
	brushes[1] = brush;

	for( i = 0; i < 2; i++ )
	{

		j = !i;

		for( k = 0; k < brushes[i]->GetNumSides(); k++ )
		{

			// if the brush side plane is the seprating plane
			if( !( ( brushes[i]->GetSide( k )->GetPlaneNum() ^ seperatingPlane ) >> 1 ) )
			{
				continue;
			}

			plane = &brushes[i]->GetSide( k )->GetPlane();

			// all the non seperating brush sides of the other brush should be at the back or on the plane
			for( l = 0; l < brushes[j]->GetNumSides(); l++ )
			{

				w = brushes[j]->GetSide( l )->GetWinding();
				if( !w )
				{
					continue;
				}

				if( !( ( brushes[j]->GetSide( l )->GetPlaneNum() ^ seperatingPlane ) >> 1 ) )
				{
					continue;
				}

				for( m = 0; m < w->GetNumPoints(); m++ )
				{
					if( plane->Distance( ( *w )[m].ToVec3() ) > 0.1f )
					{
						return false;
					}
				}
			}
		}
	}

	// add any sides from the other brush to this brush
	for( i = 0; i < brush->GetNumSides(); i++ )
	{
		for( j = 0; j < GetNumSides(); j++ )
		{
			if( !( ( brush->GetSide( i )->GetPlaneNum() ^ GetSide( j )->GetPlaneNum() ) >> 1 ) )
			{
				break;
			}
		}
		if( j < GetNumSides() )
		{
			sides[j]->flags &= brush->GetSide( i )->GetFlags();
			continue;
		}
		sides.Append( brush->GetSide( i )->Copy() );
	}

	// remove any side from this brush that is the opposite of a side of the other brush
	for( i = 0; i < GetNumSides(); i++ )
	{
		for( j = 0; j < brush->GetNumSides(); j++ )
		{
			if( GetSide( i )->GetPlaneNum() == ( brush->GetSide( j )->GetPlaneNum() ^ 1 ) )
			{
				break;
			}
		}
		if( j < brush->GetNumSides() )
		{
			delete sides[i];
			sides.RemoveIndex( i );
			i--;
			continue;
		}
	}

	contents |= brush->contents;

	CreateWindings();
	BoundBrush();

	return true;
}

/*
============
idBrush::Split
============
*/
int idBrush::Split( const idPlane& plane, int planeNum, idBrush** front, idBrush** back ) const
{
	int res, i, j;
	idBrushSide* side, *frontSide, *backSide;
	float dist, maxBack, maxFront, *maxBackWinding, *maxFrontWinding;
	idWinding* w, *mid;

	assert( windingsValid );

	if( front )
	{
		*front = NULL;
	}
	if( back )
	{
		*back = NULL;
	}

	res = bounds.PlaneSide( plane, -BRUSH_EPSILON );
	if( res == PLANESIDE_FRONT )
	{
		if( front )
		{
			*front = Copy();
		}
		return res;
	}
	if( res == PLANESIDE_BACK )
	{
		if( back )
		{
			*back = Copy();
		}
		return res;
	}

	maxBackWinding = ( float* ) _alloca16( sides.Num() * sizeof( float ) );
	maxFrontWinding = ( float* ) _alloca16( sides.Num() * sizeof( float ) );

	maxFront = maxBack = 0.0f;
	for( i = 0; i < sides.Num(); i++ )
	{
		side = sides[i];

		w = side->winding;

		if( !w )
		{
			continue;
		}

		maxBackWinding[i] = 10.0f;
		maxFrontWinding[i] = -10.0f;

		for( j = 0; j < w->GetNumPoints(); j++ )
		{

			dist = plane.Distance( ( *w )[j].ToVec3() );
			if( dist > maxFrontWinding[i] )
			{
				maxFrontWinding[i] = dist;
			}
			if( dist < maxBackWinding[i] )
			{
				maxBackWinding[i] = dist;
			}
		}

		if( maxFrontWinding[i] > maxFront )
		{
			maxFront = maxFrontWinding[i];
		}
		if( maxBackWinding[i] < maxBack )
		{
			maxBack = maxBackWinding[i];
		}
	}

	if( maxFront < BRUSH_EPSILON )
	{
		if( back )
		{
			*back = Copy();
		}
		return PLANESIDE_BACK;
	}

	if( maxBack > -BRUSH_EPSILON )
	{
		if( front )
		{
			*front = Copy();
		}
		return PLANESIDE_FRONT;
	}

	mid = new idWinding( plane.Normal(), plane.Dist() );

	for( i = 0; i < sides.Num() && mid; i++ )
	{
		mid = mid->Clip( -sides[i]->plane, BRUSH_EPSILON, false );
	}

	if( mid )
	{
		if( mid->IsTiny() )
		{
			delete mid;
			mid = NULL;
		}
		else if( mid->IsHuge() )
		{
			// if the winding is huge then the brush is unbounded
			common->Warning( "brush %d on entity %d is unbounded"
							 "( %1.2f %1.2f %1.2f )-( %1.2f %1.2f %1.2f )-( %1.2f %1.2f %1.2f )", primitiveNum, entityNum,
							 bounds[0][0], bounds[0][1], bounds[0][2], bounds[1][0], bounds[1][1], bounds[1][2],
							 bounds[1][0] - bounds[0][0], bounds[1][1] - bounds[0][1], bounds[1][2] - bounds[0][2] );
			delete mid;
			mid = NULL;
		}
	}

	if( !mid )
	{
		if( maxFront > - maxBack )
		{
			if( front )
			{
				*front = Copy();
			}
			return PLANESIDE_FRONT;
		}
		else
		{
			if( back )
			{
				*back = Copy();
			}
			return PLANESIDE_BACK;
		}
	}

	if( !front && !back )
	{
		delete mid;
		return PLANESIDE_CROSS;
	}

	*front = new idBrush();
	( *front )->SetContents( contents );
	( *front )->SetEntityNum( entityNum );
	( *front )->SetPrimitiveNum( primitiveNum );
	*back = new idBrush();
	( *back )->SetContents( contents );
	( *back )->SetEntityNum( entityNum );
	( *back )->SetPrimitiveNum( primitiveNum );

	for( i = 0; i < sides.Num(); i++ )
	{
		side = sides[i];

		if( !side->winding )
		{
			continue;
		}

		// if completely at the front
		if( maxBackWinding[i] >= BRUSH_EPSILON )
		{
			( *front )->sides.Append( side->Copy() );
		}
		// if completely at the back
		else if( maxFrontWinding[i] <= -BRUSH_EPSILON )
		{
			( *back )->sides.Append( side->Copy() );
		}
		else
		{
			// split the side
			side->Split( plane, &frontSide, &backSide );
			if( frontSide )
			{
				( *front )->sides.Append( frontSide );
			}
			else if( maxFrontWinding[i] > -BRUSH_EPSILON )
			{
				// favor an overconstrained brush
				side = side->Copy();
				side->winding = side->winding->Clip( idPlane( plane.Normal(), ( plane.Dist() - ( BRUSH_EPSILON + 0.02f ) ) ), 0.01f, true );
				assert( side->winding );
				( *front )->sides.Append( side );
			}
			if( backSide )
			{
				( *back )->sides.Append( backSide );
			}
			else if( maxBackWinding[i] < BRUSH_EPSILON )
			{
				// favor an overconstrained brush
				side = side->Copy();
				side->winding = side->winding->Clip( idPlane( -plane.Normal(), -( plane.Dist() + ( BRUSH_EPSILON + 0.02f ) ) ), 0.01f, true );
				assert( side->winding );
				( *back )->sides.Append( side );
			}
		}
	}

	side = new idBrushSide( -plane, planeNum ^ 1 );
	side->winding = mid->Reverse();
	side->flags |= SFL_SPLIT;
	( *front )->sides.Append( side );
	( *front )->windingsValid = true;
	( *front )->BoundBrush( this );

	side = new idBrushSide( plane, planeNum );
	side->winding = mid;
	side->flags |= SFL_SPLIT;
	( *back )->sides.Append( side );
	( *back )->windingsValid = true;
	( *back )->BoundBrush( this );

	return PLANESIDE_CROSS;
}

/*
============
idBrush::AddBevelsForAxialBox
============
*/
#define BRUSH_BEVEL_EPSILON		0.1f

void idBrush::AddBevelsForAxialBox()
{
	int axis, dir, i, j, k, l, order;
	idBrushSide* side, *newSide;
	idPlane plane;
	idVec3 normal, vec;
	idWinding* w, *w2;
	float d, minBack;

	assert( windingsValid );

	// add the axial planes
	order = 0;
	for( axis = 0; axis < 3; axis++ )
	{

		for( dir = -1; dir <= 1; dir += 2, order++ )
		{

			// see if the plane is already present
			for( i = 0; i < sides.Num(); i++ )
			{
				if( dir > 0 )
				{
					if( sides[i]->plane.Normal()[axis] >= 0.9999f )
					{
						break;
					}
				}
				else
				{
					if( sides[i]->plane.Normal()[axis] <= -0.9999f )
					{
						break;
					}
				}
			}

			if( i >= sides.Num() )
			{
				normal = vec3_origin;
				normal[axis] = dir;
				plane.SetNormal( normal );
				plane.SetDist( dir * bounds[( dir == 1 )][axis] );
				newSide = new idBrushSide( plane, -1 );
				newSide->SetFlag( SFL_BEVEL );
				sides.Append( newSide );
			}
		}
	}

	// if the brush is pure axial we're done
	if( sides.Num() == 6 )
	{
		return;
	}

	// test the non-axial plane edges
	for( i = 0; i < sides.Num(); i++ )
	{
		side = sides[i];
		w = side->winding;
		if( !w )
		{
			continue;
		}

		for( j = 0; j < w->GetNumPoints(); j++ )
		{
			k = ( j + 1 ) % w->GetNumPoints();
			vec = ( *w )[j].ToVec3() - ( *w )[k].ToVec3();
			if( vec.Normalize() < 0.5f )
			{
				continue;
			}
			for( k = 0; k < 3; k++ )
			{
				if( vec[k] == 1.0f || vec[k] == -1.0f || ( vec[k] == 0.0f && vec[( k + 1 ) % 3] == 0.0f ) )
				{
					break;	// axial
				}
			}
			if( k < 3 )
			{
				continue;	// only test non-axial edges
			}

			// try the six possible slanted axials from this edge
			for( axis = 0; axis < 3; axis++ )
			{

				for( dir = -1; dir <= 1; dir += 2 )
				{

					// construct a plane
					normal = vec3_origin;
					normal[axis] = dir;
					normal = vec.Cross( normal );
					if( normal.Normalize() < 0.5f )
					{
						continue;
					}
					plane.SetNormal( normal );
					plane.FitThroughPoint( ( *w )[j].ToVec3() );

					// if all the points on all the sides are
					// behind this plane, it is a proper edge bevel
					for( k = 0; k < sides.Num(); k++ )
					{

						// if this plane has allready been used, skip it
						if( plane.Compare( sides[k]->plane, 0.001f, 0.1f ) )
						{
							break;
						}

						w2 = sides[k]->winding;
						if( !w2 )
						{
							continue;
						}
						minBack = 0.0f;
						for( l = 0; l < w2->GetNumPoints(); l++ )
						{
							d = plane.Distance( ( *w2 )[l].ToVec3() );
							if( d > BRUSH_BEVEL_EPSILON )
							{
								break;	// point at the front
							}
							if( d < minBack )
							{
								minBack = d;
							}
						}
						// if some point was at the front
						if( l < w2->GetNumPoints() )
						{
							break;
						}
						// if no points at the back then the winding is on the bevel plane
						if( minBack > -BRUSH_BEVEL_EPSILON )
						{
							break;
						}
					}

					if( k < sides.Num() )
					{
						continue;	// wasn't part of the outer hull
					}

					// add this plane
					newSide = new idBrushSide( plane, -1 );
					newSide->SetFlag( SFL_BEVEL );
					sides.Append( newSide );
				}
			}
		}
	}
}

/*
============
idBrush::ExpandForAxialBox
============
*/
void idBrush::ExpandForAxialBox( const idBounds& bounds )
{
	int i, j;
	idBrushSide* side;
	idVec3 v;

	AddBevelsForAxialBox();

	for( i = 0; i < sides.Num(); i++ )
	{
		side = sides[i];

		for( j = 0; j < 3; j++ )
		{
			if( side->plane.Normal()[j] > 0.0f )
			{
				v[j] = bounds[0][j];
			}
			else
			{
				v[j] = bounds[1][j];
			}
		}

		side->plane.SetDist( side->plane.Dist() + v * -side->plane.Normal() );
	}

	if( !CreateWindings() )
	{
		common->Error( "idBrush::ExpandForAxialBox: brush %d on entity %d imploded", primitiveNum, entityNum );
	}

	/*
	// after expansion at least all non bevel sides should have a winding
	for ( i = 0; i < sides.Num(); i++ ) {
		side = sides[i];
		if ( !side->winding ) {
			if ( !( side->flags & SFL_BEVEL ) ) {
				int shit = 1;
			}
		}
	}
	*/
}

/*
============
idBrush::Copy
============
*/
idBrush* idBrush::Copy() const
{
	int i;
	idBrush* b;

	b = new idBrush();
	b->entityNum = entityNum;
	b->primitiveNum = primitiveNum;
	b->contents = contents;
	b->windingsValid = windingsValid;
	b->bounds = bounds;
	for( i = 0; i < sides.Num(); i++ )
	{
		b->sides.Append( sides[i]->Copy() );
	}
	return b;
}


//===============================================================
//
//	idBrushList
//
//===============================================================

/*
============
idBrushList::idBrushList
============
*/
idBrushList::idBrushList()
{
	numBrushes = numBrushSides = 0;
	head = tail = NULL;
}

/*
============
idBrushList::~idBrushList
============
*/
idBrushList::~idBrushList()
{
}

/*
============
idBrushList::GetBounds
============
*/
idBounds idBrushList::GetBounds() const
{
	idBounds bounds;
	idBrush* b;

	bounds.Clear();
	for( b = Head(); b; b = b->Next() )
	{
		bounds += b->GetBounds();
	}
	return bounds;
}

/*
============
idBrushList::AddToTail
============
*/
void idBrushList::AddToTail( idBrush* brush )
{
	brush->next = NULL;
	if( tail )
	{
		tail->next = brush;
	}
	tail = brush;
	if( !head )
	{
		head = brush;
	}
	numBrushes++;
	numBrushSides += brush->sides.Num();
}

/*
============
idBrushList::AddToTail
============
*/
void idBrushList::AddToTail( idBrushList& list )
{
	idBrush* brush, *next;

	for( brush = list.head; brush; brush = next )
	{
		next = brush->next;
		brush->next = NULL;
		if( tail )
		{
			tail->next = brush;
		}
		tail = brush;
		if( !head )
		{
			head = brush;
		}
		numBrushes++;
		numBrushSides += brush->sides.Num();
	}
	list.head = list.tail = NULL;
	list.numBrushes = 0;
}

/*
============
idBrushList::AddToFront
============
*/
void idBrushList::AddToFront( idBrush* brush )
{
	brush->next = head;
	head = brush;
	if( !tail )
	{
		tail = brush;
	}
	numBrushes++;
	numBrushSides += brush->sides.Num();
}

/*
============
idBrushList::AddToFront
============
*/
void idBrushList::AddToFront( idBrushList& list )
{
	idBrush* brush, *next;

	for( brush = list.head; brush; brush = next )
	{
		next = brush->next;
		brush->next = head;
		head = brush;
		if( !tail )
		{
			tail = brush;
		}
		numBrushes++;
		numBrushSides += brush->sides.Num();
	}
	list.head = list.tail = NULL;
	list.numBrushes = 0;
}

/*
============
idBrushList::Remove
============
*/
void idBrushList::Remove( idBrush* brush )
{
	idBrush*	b, *last;

	last = NULL;
	for( b = head; b; b = b->next )
	{
		if( b == brush )
		{
			if( last )
			{
				last->next = b->next;
			}
			else
			{
				head = b->next;
			}
			if( b == tail )
			{
				tail = last;
			}
			numBrushes--;
			numBrushSides -= brush->sides.Num();
			return;
		}
		last = b;
	}
}

/*
============
idBrushList::Delete
============
*/
void idBrushList::Delete( idBrush* brush )
{
	idBrush*	b, *last;

	last = NULL;
	for( b = head; b; b = b->next )
	{
		if( b == brush )
		{
			if( last )
			{
				last->next = b->next;
			}
			else
			{
				head = b->next;
			}
			if( b == tail )
			{
				tail = last;
			}
			numBrushes--;
			numBrushSides -= b->sides.Num();
			delete b;
			return;
		}
		last = b;
	}
}

/*
============
idBrushList::Copy
============
*/
idBrushList* idBrushList::Copy() const
{
	idBrush* brush;
	idBrushList* list;

	list = new idBrushList;

	for( brush = head; brush; brush = brush->next )
	{
		list->AddToTail( brush->Copy() );
	}
	return list;
}

/*
============
idBrushList::Free
============
*/
void idBrushList::Free()
{
	idBrush* brush, *next;

	for( brush = head; brush; brush = next )
	{
		next = brush->next;
		delete brush;
	}
	head = tail = NULL;
	numBrushes = numBrushSides = 0;
}

/*
============
idBrushList::Split
============
*/
void idBrushList::Split( const idPlane& plane, int planeNum, idBrushList& frontList, idBrushList& backList, bool useBrushSavedPlaneSide )
{
	idBrush* b, *front, *back;

	frontList.Clear();
	backList.Clear();

	if( !useBrushSavedPlaneSide )
	{
		for( b = head; b; b = b->next )
		{
			b->Split( plane, planeNum, &front, &back );
			if( front )
			{
				frontList.AddToTail( front );
			}
			if( back )
			{
				backList.AddToTail( back );
			}
		}
		return;
	}

	for( b = head; b; b = b->next )
	{
		if( b->savedPlaneSide & BRUSH_PLANESIDE_BOTH )
		{
			b->Split( plane, planeNum, &front, &back );
			if( front )
			{
				frontList.AddToTail( front );
			}
			if( back )
			{
				backList.AddToTail( back );
			}
		}
		else if( b->savedPlaneSide & BRUSH_PLANESIDE_FRONT )
		{
			frontList.AddToTail( b->Copy() );
		}
		else
		{
			backList.AddToTail( b->Copy() );
		}
	}
}

/*
============
idBrushList::Chop
============
*/
void idBrushList::Chop( bool ( *ChopAllowed )( idBrush* b1, idBrush* b2 ) )
{
	idBrush*	b1, *b2, *next;
	idBrushList sub1, sub2, keep;
	int i, j, c1, c2;
	idPlaneSet planeList;

#ifdef OUTPUT_CHOP_STATS
	common->Printf( "[Brush CSG]\n" );
	common->Printf( "%6d original brushes\n", this->Num() );
#endif

	CreatePlaneList( planeList );

	for( b1 = this->Head(); b1; b1 = this->Head() )
	{

		for( b2 = b1->next; b2; b2 = next )
		{

			next = b2->next;

			for( i = 0; i < 3; i++ )
			{
				if( b1->bounds[0][i] >= b2->bounds[1][i] )
				{
					break;
				}
				if( b1->bounds[1][i] <= b2->bounds[0][i] )
				{
					break;
				}
			}
			if( i < 3 )
			{
				continue;
			}

			for( i = 0; i < b1->GetNumSides(); i++ )
			{
				for( j = 0; j < b2->GetNumSides(); j++ )
				{
					if( b1->GetSide( i )->GetPlaneNum() == ( b2->GetSide( j )->GetPlaneNum() ^ 1 ) )
					{
						// opposite planes, so not touching
						break;
					}
				}
				if( j < b2->GetNumSides() )
				{
					break;
				}
			}
			if( i < b1->GetNumSides() )
			{
				continue;
			}

			sub1.Clear();
			sub2.Clear();

			c1 = 999999;
			c2 = 999999;

			// if b2 may chop up b1
			if( !ChopAllowed || ChopAllowed( b2,  b1 ) )
			{
				if( !b1->Subtract( b2, sub1 ) )
				{
					// didn't really intersect
					continue;
				}
				if( sub1.IsEmpty() )
				{
					// b1 is swallowed by b2
					this->Delete( b1 );
					break;
				}
				c1 = sub1.Num();
			}

			// if b1 may chop up b2
			if( !ChopAllowed || ChopAllowed( b1,  b2 ) )
			{
				if( !b2->Subtract( b1, sub2 ) )
				{
					// didn't really intersect
					continue;
				}
				if( sub2.IsEmpty() )
				{
					// b2 is swallowed by b1
					sub1.Free();
					this->Delete( b2 );
					continue;
				}
				c2 = sub2.Num();
			}

			if( sub1.IsEmpty() && sub2.IsEmpty() )
			{
				continue;
			}

			// don't allow too much fragmentation
			if( c1 > 2 && c2 > 2 )
			{
				sub1.Free();
				sub2.Free();
				continue;
			}

			if( c1 < c2 )
			{
				sub2.Free();
				this->AddToTail( sub1 );
				this->Delete( b1 );
				break;
			}
			else
			{
				sub1.Free();
				this->AddToTail( sub2 );
				this->Delete( b2 );
				continue;
			}
		}

		if( !b2 )
		{
			// b1 is no longer intersecting anything, so keep it
			this->Remove( b1 );
			keep.AddToTail( b1 );
#ifdef OUTPUT_CHOP_STATS
			DisplayRealTimeString( "\r%6d", keep.numBrushes );
#endif
		}
	}

	*this = keep;

#ifdef OUTPUT_CHOP_STATS
	common->Printf( "\r%6d output brushes\n", Num() );
#endif
}


/*
============
idBrushList::Merge
============
*/
void idBrushList::Merge( bool ( *MergeAllowed )( idBrush* b1, idBrush* b2 ) )
{
	idPlaneSet planeList;
	idBrush* b1, *b2, *nextb2;
	int numMerges;

	common->Printf( "[Brush Merge]\n" );
	common->Printf( "%6d original brushes\n", Num() );

	CreatePlaneList( planeList );

	numMerges = 0;
	for( b1 = Head(); b1; b1 = b1->next )
	{

		for( b2 = Head(); b2; b2 = nextb2 )
		{
			nextb2 = b2->Next();

			if( b2 == b1 )
			{
				continue;
			}

			if( MergeAllowed && !MergeAllowed( b1, b2 ) )
			{
				continue;
			}

			if( b1->TryMerge( b2, planeList ) )
			{
				Delete( b2 );
				DisplayRealTimeString( "\r%6d", ++numMerges );
				nextb2 = Head();
			}
		}
	}

	common->Printf( "\r%6d brushes merged\n", numMerges );
}

/*
============
idBrushList::SetFlagOnFacingBrushSides
============
*/
void idBrushList::SetFlagOnFacingBrushSides( const idPlane& plane, int flag )
{
	int i;
	idBrush* b;
	const idWinding* w;

	for( b = head; b; b = b->next )
	{
		if( idMath::Fabs( b->GetBounds().PlaneDistance( plane ) ) > 0.1f )
		{
			continue;
		}
		for( i = 0; i < b->GetNumSides(); i++ )
		{
			w = b->GetSide( i )->GetWinding();
			if( !w )
			{
				if( b->GetSide( i )->GetPlane().Compare( plane, BRUSH_PLANE_NORMAL_EPSILON, BRUSH_PLANE_DIST_EPSILON ) )
				{
					b->GetSide( i )->SetFlag( flag );
				}
				continue;
			}
			if( w->PlaneSide( plane ) == SIDE_ON )
			{
				b->GetSide( i )->SetFlag( flag );
			}
		}
	}
}

/*
============
idBrushList::CreatePlaneList
============
*/
void idBrushList::CreatePlaneList( idPlaneSet& planeList ) const
{
	int i;
	idBrush* b;
	idBrushSide* side;

	planeList.Resize( 512, 128 );
	for( b = Head(); b; b = b->Next() )
	{
		for( i = 0; i < b->GetNumSides(); i++ )
		{
			side = b->GetSide( i );
			side->SetPlaneNum( planeList.FindPlane( side->GetPlane(), BRUSH_PLANE_NORMAL_EPSILON, BRUSH_PLANE_DIST_EPSILON ) );
		}
	}
}

/*
============
idBrushList::CreatePlaneList
============
*/
void idBrushList::WriteBrushMap( const idStr& fileName, const idStr& ext ) const
{
	idBrushMap* map;

	map = new idBrushMap( fileName, ext );
	map->WriteBrushList( *this );
	delete map;
}


//===============================================================
//
//	idBrushMap
//
//===============================================================

/*
============
idBrushMap::idBrushMap
============
*/
idBrushMap::idBrushMap( const idStr& fileName, const idStr& ext )
{
	idStr qpath;

	qpath = fileName;
	qpath.StripFileExtension();
	qpath += ext;
	qpath.SetFileExtension( "map" );

	common->Printf( "writing %s...\n", qpath.c_str() );

	fp = fileSystem->OpenFileWrite( qpath, "fs_devpath" );
	if( !fp )
	{
		common->Error( "Couldn't open %s\n", qpath.c_str() );
		return;
	}

	texture = "textures/washroom/btile01";

	fp->WriteFloatString( "Version %1.2f\n", ( float ) CURRENT_MAP_VERSION );
	fp->WriteFloatString( "{\n" );
	fp->WriteFloatString( "\"classname\" \"worldspawn\"\n" );

	brushCount = 0;
}

/*
============
idBrushMap::~idBrushMap
============
*/
idBrushMap::~idBrushMap()
{
	if( !fp )
	{
		return;
	}
	fp->WriteFloatString( "}\n" );
	fileSystem->CloseFile( fp );
}

/*
============
idBrushMap::WriteBrush
============
*/
void idBrushMap::WriteBrush( const idBrush* brush )
{
	int i;
	idBrushSide* side;

	if( !fp )
	{
		return;
	}

	fp->WriteFloatString( "// primitive %d\n{\nbrushDef3\n{\n", brushCount++ );

	for( i = 0; i < brush->GetNumSides(); i++ )
	{
		side = brush->GetSide( i );
		fp->WriteFloatString( " ( %f %f %f %f ) ", side->GetPlane()[0], side->GetPlane()[1], side->GetPlane()[2], -side->GetPlane().Dist() );
		fp->WriteFloatString( "( ( 0.031250 0 0 ) ( 0 0.031250 0 ) ) %s 0 0 0\n", texture.c_str() );

	}
	fp->WriteFloatString( "}\n}\n" );
}

/*
============
idBrushMap::WriteBrushList
============
*/
void idBrushMap::WriteBrushList( const idBrushList& brushList )
{
	idBrush* b;

	if( !fp )
	{
		return;
	}

	for( b = brushList.Head(); b; b = b->Next() )
	{
		WriteBrush( b );
	}
}
