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

#include "../../../aas/AASFile.h"
#include "../../../aas/AASFile_local.h"
#include "AASReach.h"

#define INSIDEUNITS							2.0f
#define INSIDEUNITS_WALKEND					0.5f
#define INSIDEUNITS_WALKSTART				0.1f
#define INSIDEUNITS_SWIMEND					0.5f
#define INSIDEUNITS_FLYEND					0.5f
#define INSIDEUNITS_WATERJUMP				15.0f


/*
================
idAASReach::ReachabilityExists
================
*/
bool idAASReach::ReachabilityExists( int fromAreaNum, int toAreaNum )
{
	aasArea_t* area;
	idReachability* reach;

	area = &file->areas[fromAreaNum];
	for( reach = area->reach; reach; reach = reach->next )
	{
		if( reach->toAreaNum == toAreaNum )
		{
			return true;
		}
	}
	return false;
}

/*
================
idAASReach::CanSwimInArea
================
*/
ID_INLINE bool idAASReach::CanSwimInArea( int areaNum )
{
	return ( file->areas[areaNum].contents & AREACONTENTS_WATER ) != 0;
}

/*
================
idAASReach::AreaHasFloor
================
*/
ID_INLINE bool idAASReach::AreaHasFloor( int areaNum )
{
	return ( file->areas[areaNum].flags & AREA_FLOOR ) != 0;
}

/*
================
idAASReach::AreaIsClusterPortal
================
*/
ID_INLINE bool idAASReach::AreaIsClusterPortal( int areaNum )
{
	return ( file->areas[areaNum].contents & AREACONTENTS_CLUSTERPORTAL ) != 0;
}

/*
================
idAASReach::AddReachabilityToArea
================
*/
void idAASReach::AddReachabilityToArea( idReachability* reach, int areaNum )
{
	aasArea_t* area;

	area = &file->areas[areaNum];
	reach->next = area->reach;
	area->reach = reach;
	numReachabilities++;
}

/*
================
idAASReach::Reachability_Fly
================
*/
void idAASReach::Reachability_Fly( int areaNum )
{
	int i, faceNum, otherAreaNum;
	aasArea_t* area;
	aasFace_t* face;
	idReachability_Fly* reach;

	area = &file->areas[areaNum];

	for( i = 0; i < area->numFaces; i++ )
	{
		faceNum = file->faceIndex[area->firstFace + i];
		face = &file->faces[abs( faceNum )];

		otherAreaNum = face->areas[INT32_SIGNBITNOTSET( faceNum )];

		if( otherAreaNum == 0 )
		{
			continue;
		}

		if( ReachabilityExists( areaNum, otherAreaNum ) )
		{
			continue;
		}

		// create reachability going through this face
		reach = new idReachability_Fly();
		reach->travelType = TFL_FLY;
		reach->toAreaNum = otherAreaNum;
		reach->fromAreaNum = areaNum;
		reach->edgeNum = 0;
		reach->travelTime = 1;
		reach->start = file->FaceCenter( abs( faceNum ) );
		if( faceNum < 0 )
		{
			reach->end = reach->start + file->planeList[face->planeNum].Normal() * INSIDEUNITS_FLYEND;
		}
		else
		{
			reach->end = reach->start - file->planeList[face->planeNum].Normal() * INSIDEUNITS_FLYEND;
		}
		AddReachabilityToArea( reach, areaNum );
	}
}

/*
================
idAASReach::Reachability_Swim
================
*/
void idAASReach::Reachability_Swim( int areaNum )
{
	int i, faceNum, otherAreaNum;
	aasArea_t* area;
	aasFace_t* face;
	idReachability_Swim* reach;

	if( !CanSwimInArea( areaNum ) )
	{
		return;
	}

	area = &file->areas[areaNum];

	for( i = 0; i < area->numFaces; i++ )
	{
		faceNum = file->faceIndex[area->firstFace + i];
		face = &file->faces[abs( faceNum )];

		otherAreaNum = face->areas[INT32_SIGNBITNOTSET( faceNum )];

		if( otherAreaNum == 0 )
		{
			continue;
		}

		if( !CanSwimInArea( otherAreaNum ) )
		{
			continue;
		}

		if( ReachabilityExists( areaNum, otherAreaNum ) )
		{
			continue;
		}

		// create reachability going through this face
		reach = new idReachability_Swim();
		reach->travelType = TFL_SWIM;
		reach->toAreaNum = otherAreaNum;
		reach->fromAreaNum = areaNum;
		reach->edgeNum = 0;
		reach->travelTime = 1;
		reach->start = file->FaceCenter( abs( faceNum ) );
		if( faceNum < 0 )
		{
			reach->end = reach->start + file->planeList[face->planeNum].Normal() * INSIDEUNITS_SWIMEND;
		}
		else
		{
			reach->end = reach->start - file->planeList[face->planeNum].Normal() * INSIDEUNITS_SWIMEND;
		}
		AddReachabilityToArea( reach, areaNum );
	}
}

/*
================
idAASReach::Reachability_EqualFloorHeight
================
*/
void idAASReach::Reachability_EqualFloorHeight( int areaNum )
{
	int i, k, l, m, n, faceNum, face1Num, face2Num, otherAreaNum, edge1Num = 0, edge2Num;
	aasArea_t* area, *otherArea;
	aasFace_t* face, *face1, *face2;
	idReachability_Walk* reach;

	if( !AreaHasFloor( areaNum ) )
	{
		return;
	}

	area = &file->areas[areaNum];

	for( i = 0; i < area->numFaces; i++ )
	{
		faceNum = file->faceIndex[area->firstFace + i];
		face = &file->faces[abs( faceNum )];

		otherAreaNum = face->areas[INT32_SIGNBITNOTSET( faceNum )];
		if( !AreaHasFloor( otherAreaNum ) )
		{
			continue;
		}

		otherArea = &file->areas[otherAreaNum];

		for( k = 0; k < area->numFaces; k++ )
		{
			face1Num = file->faceIndex[area->firstFace + k];
			face1 = &file->faces[abs( face1Num )];

			if( !( face1->flags & FACE_FLOOR ) )
			{
				continue;
			}
			for( l = 0; l < otherArea->numFaces; l++ )
			{
				face2Num = file->faceIndex[otherArea->firstFace + l];
				face2 = &file->faces[abs( face2Num )];

				if( !( face2->flags & FACE_FLOOR ) )
				{
					continue;
				}

				for( m = 0; m < face1->numEdges; m++ )
				{
					edge1Num = abs( file->edgeIndex[face1->firstEdge + m] );
					for( n = 0; n < face2->numEdges; n++ )
					{
						edge2Num = abs( file->edgeIndex[face2->firstEdge + n] );
						if( edge1Num == edge2Num )
						{
							break;
						}
					}
					if( n < face2->numEdges )
					{
						break;
					}
				}
				if( m < face1->numEdges )
				{
					break;
				}
			}
			if( l < otherArea->numFaces )
			{
				break;
			}
		}
		if( k < area->numFaces )
		{
			// create reachability
			reach = new idReachability_Walk();
			reach->travelType = TFL_WALK;
			reach->toAreaNum = otherAreaNum;
			reach->fromAreaNum = areaNum;
			reach->edgeNum = abs( edge1Num );
			reach->travelTime = 1;
			reach->start = file->EdgeCenter( edge1Num );
			if( faceNum < 0 )
			{
				reach->end = reach->start + file->planeList[face->planeNum].Normal() * INSIDEUNITS_WALKEND;
			}
			else
			{
				reach->end = reach->start - file->planeList[face->planeNum].Normal() * INSIDEUNITS_WALKEND;
			}
			AddReachabilityToArea( reach, areaNum );
		}
	}
}

/*
================
idAASReach::Reachability_Step_Barrier_WaterJump_WalkOffLedge
================
*/
bool idAASReach::Reachability_Step_Barrier_WaterJump_WalkOffLedge( int area1num, int area2num )
{
	int i, j, k, l, edge1Num, edge2Num, areas[10];
	int floor_bestArea1FloorEdgeNum = 0, floor_bestArea2FloorEdgeNum, floor_foundReach;
	int water_bestArea1FloorEdgeNum, water_bestArea2FloorEdgeNum, water_foundReach;
	int side1, faceSide1, floorFace1Num;
	float dist, dist1, dist2, diff, invGravityDot, orthogonalDot;
	float x1, x2, x3, x4, y1, y2, y3, y4, tmp, y;
	float length, floor_bestLength, water_bestLength, floor_bestDist, water_bestDist;
	idVec3 v1, v2, v3, v4, tmpv, p1area1, p1area2, p2area1, p2area2;
	idVec3 normal, orthogonal, edgeVec, start, end;
	idVec3 floor_bestStart, floor_bestEnd, floor_bestNormal;
	idVec3 water_bestStart, water_bestEnd, water_bestNormal;
	idVec3 testPoint;
	idPlane* plane;
	aasArea_t* area1, *area2;
	aasFace_t* floorFace1, *floorFace2, *floor_bestFace1, *water_bestFace1;
	aasEdge_t* edge1, *edge2;
	idReachability_Walk* walkReach;
	idReachability_BarrierJump* barrierJumpReach;
	idReachability_WaterJump* waterJumpReach;
	idReachability_WalkOffLedge* walkOffLedgeReach;
	aasTrace_t trace;

	// must be able to walk or swim in the first area
	if( !AreaHasFloor( area1num ) && !CanSwimInArea( area1num ) )
	{
		return false;
	}

	if( !AreaHasFloor( area2num ) && !CanSwimInArea( area2num ) )
	{
		return false;
	}

	area1 = &file->areas[area1num];
	area2 = &file->areas[area2num];

	// if the areas are not near anough in the x-y direction
	for( i = 0; i < 2; i++ )
	{
		if( area1->bounds[0][i] > area2->bounds[1][i] + 2.0f )
		{
			return false;
		}
		if( area1->bounds[1][i] < area2->bounds[0][i] - 2.0f )
		{
			return false;
		}
	}

	floor_foundReach = false;
	floor_bestDist = 99999;
	floor_bestLength = 0;
	floor_bestArea2FloorEdgeNum = 0;

	water_foundReach = false;
	water_bestDist = 99999;
	water_bestLength = 0;
	water_bestArea2FloorEdgeNum = 0;

	for( i = 0; i < area1->numFaces; i++ )
	{
		floorFace1Num = file->faceIndex[area1->firstFace + i];
		faceSide1 = floorFace1Num < 0;
		floorFace1 = &file->faces[abs( floorFace1Num )];

		// if this isn't a floor face
		if( !( floorFace1->flags & FACE_FLOOR ) )
		{

			// if we can swim in the first area
			if( CanSwimInArea( area1num ) )
			{

				// face plane must be more or less horizontal
				plane = &file->planeList[ floorFace1->planeNum ^ ( !faceSide1 ) ];
				if( plane->Normal() * file->settings.invGravityDir < file->settings.minFloorCos )
				{
					continue;
				}
			}
			else
			{
				// if we can't swim in the area it must be a ground face
				continue;
			}
		}

		for( k = 0; k < floorFace1->numEdges; k++ )
		{
			edge1Num = file->edgeIndex[floorFace1->firstEdge + k];
			side1 = ( edge1Num < 0 );
			// NOTE: for water faces we must take the side area 1 is on into
			// account because the face is shared and doesn't have to be oriented correctly
			if( !( floorFace1->flags & FACE_FLOOR ) )
			{
				side1 = ( side1 == faceSide1 );
			}
			edge1Num = abs( edge1Num );
			edge1 = &file->edges[edge1Num];
			// vertices of the edge
			v1 = file->vertices[edge1->vertexNum[!side1]];
			v2 = file->vertices[edge1->vertexNum[side1]];
			// get a vertical plane through the edge
			// NOTE: normal is pointing into area 2 because the face edges are stored counter clockwise
			edgeVec = v2 - v1;
			normal = edgeVec.Cross( file->settings.invGravityDir );
			normal.Normalize();
			dist = normal * v1;

			// check the faces from the second area
			for( j = 0; j < area2->numFaces; j++ )
			{
				floorFace2 = &file->faces[abs( file->faceIndex[area2->firstFace + j] )];
				// must be a ground face
				if( !( floorFace2->flags & FACE_FLOOR ) )
				{
					continue;
				}
				// check the edges of this ground face
				for( l = 0; l < floorFace2->numEdges; l++ )
				{
					edge2Num = abs( file->edgeIndex[floorFace2->firstEdge + l] );
					edge2 = &file->edges[edge2Num];
					// vertices of the edge
					v3 = file->vertices[edge2->vertexNum[0]];
					v4 = file->vertices[edge2->vertexNum[1]];
					// check the distance between the two points and the vertical plane through the edge of area1
					diff = normal * v3 - dist;
					if( diff < -0.2f || diff > 0.2f )
					{
						continue;
					}
					diff = normal * v4 - dist;
					if( diff < -0.2f || diff > 0.2f )
					{
						continue;
					}

					// project the two ground edges into the step side plane
					// and calculate the shortest distance between the two
					// edges if they overlap in the direction orthogonal to
					// the gravity direction
					orthogonal = file->settings.invGravityDir.Cross( normal );
					invGravityDot = file->settings.invGravityDir * file->settings.invGravityDir;
					orthogonalDot = orthogonal * orthogonal;
					// projection into the step plane
					// NOTE: since gravity is vertical this is just the z coordinate
					y1 = v1[2];//(v1 * file->settings.invGravity) / invGravityDot;
					y2 = v2[2];//(v2 * file->settings.invGravity) / invGravityDot;
					y3 = v3[2];//(v3 * file->settings.invGravity) / invGravityDot;
					y4 = v4[2];//(v4 * file->settings.invGravity) / invGravityDot;

					x1 = ( v1 * orthogonal ) / orthogonalDot;
					x2 = ( v2 * orthogonal ) / orthogonalDot;
					x3 = ( v3 * orthogonal ) / orthogonalDot;
					x4 = ( v4 * orthogonal ) / orthogonalDot;

					if( x1 > x2 )
					{
						tmp = x1;
						x1 = x2;
						x2 = tmp;
						tmp = y1;
						y1 = y2;
						y2 = tmp;
						tmpv = v1;
						v1 = v2;
						v2 = tmpv;
					}
					if( x3 > x4 )
					{
						tmp = x3;
						x3 = x4;
						x4 = tmp;
						tmp = y3;
						y3 = y4;
						y4 = tmp;
						tmpv = v3;
						v3 = v4;
						v4 = tmpv;
					}
					// if the two projected edge lines have no overlap
					if( x2 <= x3 || x4 <= x1 )
					{
						continue;
					}
					// if the two lines fully overlap
					if( ( x1 - 0.5f < x3 && x4 < x2 + 0.5f ) && ( x3 - 0.5f < x1 && x2 < x4 + 0.5f ) )
					{
						dist1 = y3 - y1;
						dist2 = y4 - y2;
						p1area1 = v1;
						p2area1 = v2;
						p1area2 = v3;
						p2area2 = v4;
					}
					else
					{
						// if the points are equal
						if( x1 > x3 - 0.1f && x1 < x3 + 0.1f )
						{
							dist1 = y3 - y1;
							p1area1 = v1;
							p1area2 = v3;
						}
						else if( x1 < x3 )
						{
							y = y1 + ( x3 - x1 ) * ( y2 - y1 ) / ( x2 - x1 );
							dist1 = y3 - y;
							p1area1 = v3;
							p1area1[2] = y;
							p1area2 = v3;
						}
						else
						{
							y = y3 + ( x1 - x3 ) * ( y4 - y3 ) / ( x4 - x3 );
							dist1 = y - y1;
							p1area1 = v1;
							p1area2 = v1;
							p1area2[2] = y;
						}
						// if the points are equal
						if( x2 > x4 - 0.1f && x2 < x4 + 0.1f )
						{
							dist2 = y4 - y2;
							p2area1 = v2;
							p2area2 = v4;
						}
						else if( x2 < x4 )
						{
							y = y3 + ( x2 - x3 ) * ( y4 - y3 ) / ( x4 - x3 );
							dist2 = y - y2;
							p2area1 = v2;
							p2area2 = v2;
							p2area2[2] = y;
						}
						else
						{
							y = y1 + ( x4 - x1 ) * ( y2 - y1 ) / ( x2 - x1 );
							dist2 = y4 - y;
							p2area1 = v4;
							p2area1[2] = y;
							p2area2 = v4;
						}
					}

					// if both distances are pretty much equal then we take the middle of the points
					if( dist1 > dist2 - 1.0f && dist1 < dist2 + 1.0f )
					{
						dist = dist1;
						start = ( p1area1 + p2area1 ) * 0.5f;
						end = ( p1area2 + p2area2 ) * 0.5f;
					}
					else if( dist1 < dist2 )
					{
						dist = dist1;
						start = p1area1;
						end = p1area2;
					}
					else
					{
						dist = dist2;
						start = p2area1;
						end = p2area2;
					}

					// get the length of the overlapping part of the edges of the two areas
					length = ( p2area2 - p1area2 ).Length();

					if( floorFace1->flags & FACE_FLOOR )
					{
						// if the vertical distance is smaller
						if( dist < floor_bestDist ||
								// or the vertical distance is pretty much the same
								// but the overlapping part of the edges is longer
								( dist < floor_bestDist + 1.0f && length > floor_bestLength ) )
						{
							floor_bestDist = dist;
							floor_bestLength = length;
							floor_foundReach = true;
							floor_bestArea1FloorEdgeNum = edge1Num;
							floor_bestArea2FloorEdgeNum = edge2Num;
							floor_bestFace1 = floorFace1;
							floor_bestStart = start;
							floor_bestNormal = normal;
							floor_bestEnd = end;
						}
					}
					else
					{
						// if the vertical distance is smaller
						if( dist < water_bestDist ||
								//or the vertical distance is pretty much the same
								//but the overlapping part of the edges is longer
								( dist < water_bestDist + 1.0f && length > water_bestLength ) )
						{
							water_bestDist = dist;
							water_bestLength = length;
							water_foundReach = true;
							water_bestArea1FloorEdgeNum = edge1Num;
							water_bestArea2FloorEdgeNum = edge2Num;
							water_bestFace1 = floorFace1;
							water_bestStart = start;	// best start point in area1
							water_bestNormal = normal;	// normal is pointing into area2
							water_bestEnd = end;		// best point towards area2
						}
					}
				}
			}
		}
	}
	//
	// NOTE: swim reachabilities should already be filtered out
	//
	// Steps
	//
	//         ---------
	//         |          step height -> TFL_WALK
	// --------|
	//
	//         ---------
	// ~~~~~~~~|          step height and low water -> TFL_WALK
	// --------|
	//
	// ~~~~~~~~~~~~~~~~~~
	//         ---------
	//         |          step height and low water up to the step -> TFL_WALK
	// --------|
	//
	// check for a step reachability
	if( floor_foundReach )
	{
		// if area2 is higher but lower than the maximum step height
		// NOTE: floor_bestDist >= 0 also catches equal floor reachabilities
		if( floor_bestDist >= 0 && floor_bestDist < file->settings.maxStepHeight )
		{
			// create walk reachability from area1 to area2
			walkReach = new idReachability_Walk();
			walkReach->travelType = TFL_WALK;
			walkReach->toAreaNum = area2num;
			walkReach->fromAreaNum = area1num;
			walkReach->start = floor_bestStart + INSIDEUNITS_WALKSTART * floor_bestNormal;
			walkReach->end = floor_bestEnd + INSIDEUNITS_WALKEND * floor_bestNormal;
			walkReach->edgeNum = abs( floor_bestArea1FloorEdgeNum );
			walkReach->travelTime = 0;
			if( area2->flags & AREA_CROUCH )
			{
				walkReach->travelTime += file->settings.tt_startCrouching;
			}
			AddReachabilityToArea( walkReach, area1num );
			return true;
		}
	}
	//
	// Water Jumps
	//
	//         ---------
	//         |
	// ~~~~~~~~|
	//         |
	//         |          higher than step height and water up to waterjump height -> TFL_WATERJUMP
	// --------|
	//
	// ~~~~~~~~~~~~~~~~~~
	//         ---------
	//         |
	//         |
	//         |
	//         |          higher than step height and low water up to the step -> TFL_WATERJUMP
	// --------|
	//
	// check for a waterjump reachability
	if( water_foundReach )
	{
		// get a test point a little bit towards area1
		testPoint = water_bestEnd - INSIDEUNITS * water_bestNormal;
		// go down the maximum waterjump height
		testPoint[2] -= file->settings.maxWaterJumpHeight;
		// if there IS water the sv_maxwaterjump height below the bestend point
		if( area1->flags & AREA_LIQUID )
		{
			// don't create rediculous water jump reachabilities from areas very far below the water surface
			if( water_bestDist < file->settings.maxWaterJumpHeight + 24 )
			{
				// water jumping from or towards a crouch only areas is not possible
				if( !( area1->flags & AREA_CROUCH ) && !( area2->flags & AREA_CROUCH ) )
				{
					// create water jump reachability from area1 to area2
					waterJumpReach = new idReachability_WaterJump();
					waterJumpReach->travelType = TFL_WATERJUMP;
					waterJumpReach->toAreaNum = area2num;
					waterJumpReach->fromAreaNum = area1num;
					waterJumpReach->start = water_bestStart;
					waterJumpReach->end = water_bestEnd + INSIDEUNITS_WATERJUMP * water_bestNormal;
					waterJumpReach->edgeNum = abs( floor_bestArea1FloorEdgeNum );
					waterJumpReach->travelTime = file->settings.tt_waterJump;
					AddReachabilityToArea( waterJumpReach, area1num );
					return true;
				}
			}
		}
	}
	//
	// Barrier Jumps
	//
	//         ---------
	//         |
	//         |
	//         |
	//         |         higher than max step height lower than max barrier height -> TFL_BARRIERJUMP
	// --------|
	//
	//         ---------
	//         |
	//         |
	//         |
	// ~~~~~~~~|         higher than max step height lower than max barrier height
	// --------|         and a thin layer of water in the area to jump from -> TFL_BARRIERJUMP
	//
	// check for a barrier jump reachability
	if( floor_foundReach )
	{
		//if area2 is higher but lower than the maximum barrier jump height
		if( floor_bestDist > 0 && floor_bestDist < file->settings.maxBarrierHeight )
		{
			//if no water in area1 or a very thin layer of water on the ground
			if( !water_foundReach || ( floor_bestDist - water_bestDist < 16 ) )
			{
				// cannot perform a barrier jump towards or from a crouch area
				if( !( area1->flags & AREA_CROUCH ) && !( area2->flags & AREA_CROUCH ) )
				{
					// create barrier jump reachability from area1 to area2
					barrierJumpReach = new idReachability_BarrierJump();
					barrierJumpReach->travelType = TFL_BARRIERJUMP;
					barrierJumpReach->toAreaNum = area2num;
					barrierJumpReach->fromAreaNum = area1num;
					barrierJumpReach->start = floor_bestStart + INSIDEUNITS_WALKSTART * floor_bestNormal;
					barrierJumpReach->end = floor_bestEnd + INSIDEUNITS_WALKEND * floor_bestNormal;
					barrierJumpReach->edgeNum = abs( floor_bestArea1FloorEdgeNum );
					barrierJumpReach->travelTime = file->settings.tt_barrierJump;
					AddReachabilityToArea( barrierJumpReach, area1num );
					return true;
				}
			}
		}
	}
	//
	// Walk and Walk Off Ledge
	//
	// --------|
	//         |          can walk or step back -> TFL_WALK
	//         ---------
	//
	// --------|
	//         |
	//         |
	//         |
	//         |          cannot walk/step back -> TFL_WALKOFFLEDGE
	//         ---------
	//
	// --------|
	//         |
	//         |~~~~~~~~
	//         |
	//         |          cannot step back but can waterjump back -> TFL_WALKOFFLEDGE
	//         ---------  FIXME: create TFL_WALK reach??
	//
	// check for a walk or walk off ledge reachability
	if( floor_foundReach )
	{
		if( floor_bestDist < 0 )
		{
			if( floor_bestDist > -file->settings.maxStepHeight )
			{
				// create walk reachability from area1 to area2
				walkReach = new idReachability_Walk();
				walkReach->travelType = TFL_WALK;
				walkReach->toAreaNum = area2num;
				walkReach->fromAreaNum = area1num;
				walkReach->start = floor_bestStart + INSIDEUNITS_WALKSTART * floor_bestNormal;
				walkReach->end = floor_bestEnd + INSIDEUNITS_WALKEND * floor_bestNormal;
				walkReach->edgeNum = abs( floor_bestArea1FloorEdgeNum );
				walkReach->travelTime = 1;
				AddReachabilityToArea( walkReach, area1num );
				return true;
			}
			// if no maximum fall height set or less than the max
			if( !file->settings.maxFallHeight || idMath::Fabs( floor_bestDist ) < file->settings.maxFallHeight )
			{
				// trace a bounding box vertically to check for solids
				floor_bestEnd += INSIDEUNITS * floor_bestNormal;
				start = floor_bestEnd;
				start[2] = floor_bestStart[2];
				end = floor_bestEnd;
				end[2] += 4;
				trace.areas = areas;
				trace.maxAreas = sizeof( areas ) / sizeof( int );
				file->Trace( trace, start, end );
				// if the trace didn't start in solid and nothing was hit
				if( trace.lastAreaNum && trace.fraction >= 1.0f )
				{
					// the trace end point must be in the goal area
					if( trace.lastAreaNum == area2num )
					{
						// don't create reachability if going through a cluster portal
						for( i = 0; i < trace.numAreas; i++ )
						{
							if( AreaIsClusterPortal( trace.areas[i] ) )
							{
								break;
							}
						}
						if( i >= trace.numAreas )
						{
							// create a walk off ledge reachability from area1 to area2
							walkOffLedgeReach = new idReachability_WalkOffLedge();
							walkOffLedgeReach->travelType = TFL_WALKOFFLEDGE;
							walkOffLedgeReach->toAreaNum = area2num;
							walkOffLedgeReach->fromAreaNum = area1num;
							walkOffLedgeReach->start = floor_bestStart;
							walkOffLedgeReach->end = floor_bestEnd;
							walkOffLedgeReach->edgeNum = abs( floor_bestArea1FloorEdgeNum );
							walkOffLedgeReach->travelTime = file->settings.tt_startWalkOffLedge + idMath::Fabs( floor_bestDist ) * 50 / file->settings.gravityValue;
							AddReachabilityToArea( walkOffLedgeReach, area1num );
							return true;
						}
					}
				}
			}
		}
	}
	return false;
}

/*
================
idAASReach::Reachability_WalkOffLedge
================
*/
void idAASReach::Reachability_WalkOffLedge( int areaNum )
{
	int i, j, faceNum, edgeNum, side, reachAreaNum, p, areas[10];
	aasArea_t* area;
	aasFace_t* face;
	aasEdge_t* edge;
	idPlane* plane;
	idVec3 v1, v2, mid, dir, testEnd;
	idReachability_WalkOffLedge* reach;
	aasTrace_t trace;

	if( !AreaHasFloor( areaNum ) || CanSwimInArea( areaNum ) )
	{
		return;
	}

	area = &file->areas[areaNum];

	for( i = 0; i < area->numFaces; i++ )
	{
		faceNum = file->faceIndex[area->firstFace + i];
		face = &file->faces[abs( faceNum )];

		// face must be a floor face
		if( !( face->flags & FACE_FLOOR ) )
		{
			continue;
		}

		for( j = 0; j < face->numEdges; j++ )
		{

			edgeNum = file->edgeIndex[face->firstEdge + j];
			edge = &file->edges[abs( edgeNum )];

			//if ( !(edge->flags & EDGE_LEDGE) ) {
			//	continue;
			//}

			side = edgeNum < 0;

			v1 = file->vertices[edge->vertexNum[side]];
			v2 = file->vertices[edge->vertexNum[!side]];

			plane = &file->planeList[face->planeNum ^ INT32_SIGNBITSET( faceNum ) ];

			// get the direction into the other area
			dir = plane->Normal().Cross( v2 - v1 );
			dir.Normalize();

			mid = ( v1 + v2 ) * 0.5f;
			testEnd = mid + INSIDEUNITS_WALKEND * dir;
			testEnd[2] -= file->settings.maxFallHeight + 1.0f;
			trace.areas = areas;
			trace.maxAreas = sizeof( areas ) / sizeof( int );
			file->Trace( trace, mid, testEnd );

			reachAreaNum = trace.lastAreaNum;
			if( !reachAreaNum || reachAreaNum == areaNum )
			{
				continue;
			}
			if( idMath::Fabs( mid[2] - trace.endpos[2] ) > file->settings.maxFallHeight )
			{
				continue;
			}
			if( !AreaHasFloor( reachAreaNum ) && !CanSwimInArea( reachAreaNum ) )
			{
				continue;
			}
			if( ReachabilityExists( areaNum, reachAreaNum ) )
			{
				continue;
			}
			// if not going through a cluster portal
			for( p = 0; p < trace.numAreas; p++ )
			{
				if( AreaIsClusterPortal( trace.areas[p] ) )
				{
					break;
				}
			}
			if( p < trace.numAreas )
			{
				continue;
			}

			reach = new idReachability_WalkOffLedge();
			reach->travelType = TFL_WALKOFFLEDGE;
			reach->toAreaNum = reachAreaNum;
			reach->fromAreaNum = areaNum;
			reach->start = mid;
			reach->end = trace.endpos;
			reach->edgeNum = abs( edgeNum );
			reach->travelTime = file->settings.tt_startWalkOffLedge + idMath::Fabs( mid[2] - trace.endpos[2] ) * 50 / file->settings.gravityValue;
			AddReachabilityToArea( reach, areaNum );
		}
	}
}

/*
================
idAASReach::FlagReachableAreas
================
*/
void idAASReach::FlagReachableAreas( idAASFileLocal* file )
{
	int i, numReachableAreas;

	numReachableAreas = 0;
	for( i = 1; i < file->areas.Num(); i++ )
	{

		if( ( file->areas[i].flags & ( AREA_FLOOR | AREA_LADDER ) ) ||
				( file->areas[i].contents & AREACONTENTS_WATER ) )
		{
			file->areas[i].flags |= AREA_REACHABLE_WALK;
		}
		if( file->GetSettings().allowFlyReachabilities )
		{
			file->areas[i].flags |= AREA_REACHABLE_FLY;
		}
		numReachableAreas++;
	}

	common->Printf( "%6d reachable areas\n", numReachableAreas );
}

/*
================
idAASReach::Build
================
*/
bool idAASReach::Build( const idMapFile* mapFile, idAASFileLocal* file )
{
	int i, j, lastPercent, percent;

	this->mapFile = mapFile;
	this->file = file;
	numReachabilities = 0;

	common->Printf( "[Reachability]\n" );

	// delete all existing reachabilities
	file->DeleteReachabilities();

	FlagReachableAreas( file );

	for( i = 1; i < file->areas.Num(); i++ )
	{
		if( !( file->areas[i].flags & AREA_REACHABLE_WALK ) )
		{
			continue;
		}
		if( file->GetSettings().allowSwimReachabilities )
		{
			Reachability_Swim( i );
		}
		Reachability_EqualFloorHeight( i );
	}

	lastPercent = -1;
	for( i = 1; i < file->areas.Num(); i++ )
	{

		if( !( file->areas[i].flags & AREA_REACHABLE_WALK ) )
		{
			continue;
		}

		for( j = 0; j < file->areas.Num(); j++ )
		{
			if( i == j )
			{
				continue;
			}

			if( !( file->areas[j].flags & AREA_REACHABLE_WALK ) )
			{
				continue;
			}

			if( ReachabilityExists( i, j ) )
			{
				continue;
			}
			if( Reachability_Step_Barrier_WaterJump_WalkOffLedge( i, j ) )
			{
				continue;
			}
		}

		//Reachability_WalkOffLedge( i );

		percent = 100 * i / file->areas.Num();
		if( percent > lastPercent )
		{
			common->Printf( "\r%6d%%", percent );
			lastPercent = percent;
		}
	}

	if( file->GetSettings().allowFlyReachabilities )
	{
		for( i = 1; i < file->areas.Num(); i++ )
		{
			Reachability_Fly( i );
		}
	}

	file->LinkReversedReachability();

	common->Printf( "\r%6d reachabilities\n", numReachabilities );

	return true;
}
