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


#include "../Game_local.h"

/*
===============================================================================

	Dynamic Obstacle Avoidance

	- assumes the AI lives inside a bounding box aligned with the gravity direction
	- obstacles in proximity of the AI are gathered
	- if obstacles are found the AAS walls are also considered as obstacles
	- every obstacle is represented by an oriented bounding box (OBB)
	- an OBB is projected onto a 2D plane orthogonal to AI's gravity direction
	- the 2D windings of the projections are expanded for the AI bbox
	- a path tree is build using clockwise and counter clockwise edge walks along the winding edges
	- the path tree is pruned and optimized
	- the shortest path is chosen for navigation

===============================================================================
*/

const float MAX_OBSTACLE_RADIUS			= 256.0f;
const float PUSH_OUTSIDE_OBSTACLES		= 0.5f;
const float CLIP_BOUNDS_EPSILON			= 10.0f;
const int 	MAX_AAS_WALL_EDGES			= 256;
const int 	MAX_OBSTACLES				= 256;
const int	MAX_PATH_NODES				= 256;
const int 	MAX_OBSTACLE_PATH			= 64;

typedef struct obstacle_s
{
	idVec2				bounds[2];
	idWinding2D			winding;
	idEntity* 			entity;
} obstacle_t;

typedef struct pathNode_s
{
	int					dir;
	idVec2				pos;
	idVec2				delta;
	float				dist;
	int					obstacle;
	int					edgeNum;
	int					numNodes;
	struct pathNode_s* 	parent;
	struct pathNode_s* 	children[2];
	struct pathNode_s* 	next;
	void				Init();
} pathNode_t;

void pathNode_s::Init()
{
	dir = 0;
	pos.Zero();
	delta.Zero();
	obstacle = -1;
	edgeNum = -1;
	numNodes = 0;
	parent = children[0] = children[1] = next = NULL;
}

idBlockAlloc<pathNode_t, 128>	pathNodeAllocator;


/*
============
LineIntersectsPath
============
*/
bool LineIntersectsPath( const idVec2& start, const idVec2& end, const pathNode_t* node )
{
	float d0, d1, d2, d3;
	idVec3 plane1, plane2;

	plane1 = idWinding2D::Plane2DFromPoints( start, end );
	d0 = plane1.x * node->pos.x + plane1.y * node->pos.y + plane1.z;
	while( node->parent )
	{
		d1 = plane1.x * node->parent->pos.x + plane1.y * node->parent->pos.y + plane1.z;
		if( IEEE_FLT_SIGNBITSET( d0 ) ^ IEEE_FLT_SIGNBITSET( d1 ) )
		{
			plane2 = idWinding2D::Plane2DFromPoints( node->pos, node->parent->pos );
			d2 = plane2.x * start.x + plane2.y * start.y + plane2.z;
			d3 = plane2.x * end.x + plane2.y * end.y + plane2.z;
			if( IEEE_FLT_SIGNBITSET( d2 ) ^ IEEE_FLT_SIGNBITSET( d3 ) )
			{
				return true;
			}
		}
		d0 = d1;
		node = node->parent;
	}
	return false;
}

/*
============
PointInsideObstacle
============
*/
int PointInsideObstacle( const obstacle_t* obstacles, const int numObstacles, const idVec2& point )
{
	int i;

	for( i = 0; i < numObstacles; i++ )
	{

		const idVec2* bounds = obstacles[i].bounds;
		if( point.x < bounds[0].x || point.y < bounds[0].y || point.x > bounds[1].x || point.y > bounds[1].y )
		{
			continue;
		}

		if( !obstacles[i].winding.PointInside( point, 0.1f ) )
		{
			continue;
		}

		return i;
	}

	return -1;
}

/*
============
GetPointOutsideObstacles
============
*/
void GetPointOutsideObstacles( const obstacle_t* obstacles, const int numObstacles, idVec2& point, int* obstacle, int* edgeNum )
{
	int i, j, k, n, bestObstacle, bestEdgeNum, queueStart, queueEnd, edgeNums[2];
	float d, bestd, scale[2];
	idVec3 plane, bestPlane;
	idVec2 newPoint, dir, bestPoint;
	int* queue;
	bool* obstacleVisited;
	idWinding2D w1, w2;

	if( obstacle )
	{
		*obstacle = -1;
	}
	if( edgeNum )
	{
		*edgeNum = -1;
	}

	bestObstacle = PointInsideObstacle( obstacles, numObstacles, point );
	if( bestObstacle == -1 )
	{
		return;
	}

	const idWinding2D& w = obstacles[bestObstacle].winding;
	bestd = idMath::INFINITUM;
	bestEdgeNum = 0;
	for( i = 0; i < w.GetNumPoints(); i++ )
	{
		plane = idWinding2D::Plane2DFromPoints( w[( i + 1 ) % w.GetNumPoints()], w[i], true );
		d = plane.x * point.x + plane.y * point.y + plane.z;
		if( d < bestd )
		{
			bestd = d;
			bestPlane = plane;
			bestEdgeNum = i;
		}
		// if this is a wall always try to pop out at the first edge
		if( obstacles[bestObstacle].entity == NULL )
		{
			break;
		}
	}

	newPoint = point - ( bestd + PUSH_OUTSIDE_OBSTACLES ) * bestPlane.ToVec2();
	if( PointInsideObstacle( obstacles, numObstacles, newPoint ) == -1 )
	{
		point = newPoint;
		if( obstacle )
		{
			*obstacle = bestObstacle;
		}
		if( edgeNum )
		{
			*edgeNum = bestEdgeNum;
		}
		return;
	}

	queue = ( int* ) _alloca( numObstacles * sizeof( queue[0] ) );
	obstacleVisited = ( bool* ) _alloca( numObstacles * sizeof( obstacleVisited[0] ) );

	queueStart = 0;
	queueEnd = 1;
	queue[0] = bestObstacle;

	memset( obstacleVisited, 0, numObstacles * sizeof( obstacleVisited[0] ) );
	assert( bestObstacle < numObstacles );
	obstacleVisited[bestObstacle] = true;

	bestd = idMath::INFINITUM;
	for( i = queue[0]; queueStart < queueEnd; i = queue[++queueStart] )
	{
		w1 = obstacles[i].winding;
		w1.Expand( PUSH_OUTSIDE_OBSTACLES );

		for( j = 0; j < numObstacles; j++ )
		{
			// if the obstacle has been visited already
			if( obstacleVisited[j] )
			{
				continue;
			}
			// if the bounds do not intersect
			if( obstacles[j].bounds[0].x > obstacles[i].bounds[1].x || obstacles[j].bounds[0].y > obstacles[i].bounds[1].y ||
					obstacles[j].bounds[1].x < obstacles[i].bounds[0].x || obstacles[j].bounds[1].y < obstacles[i].bounds[0].y )
			{
				continue;
			}

			assert( queueEnd < numObstacles );
			queue[queueEnd++] = j;
			obstacleVisited[j] = true;

			w2 = obstacles[j].winding;
			w2.Expand( 0.2f );

			for( k = 0; k < w1.GetNumPoints(); k++ )
			{
				dir = w1[( k + 1 ) % w1.GetNumPoints()] - w1[k];
				if( !w2.RayIntersection( w1[k], dir, scale[0], scale[1], edgeNums ) )
				{
					continue;
				}
				for( n = 0; n < 2; n++ )
				{
					newPoint = w1[k] + scale[n] * dir;
					if( PointInsideObstacle( obstacles, numObstacles, newPoint ) == -1 )
					{
						d = ( newPoint - point ).LengthSqr();
						if( d < bestd )
						{
							bestd = d;
							bestPoint = newPoint;
							bestEdgeNum = edgeNums[n];
							bestObstacle = j;
						}
					}
				}
			}
		}

		if( bestd < idMath::INFINITUM )
		{
			point = bestPoint;
			if( obstacle )
			{
				*obstacle = bestObstacle;
			}
			if( edgeNum )
			{
				*edgeNum = bestEdgeNum;
			}
			return;
		}
	}
	gameLocal.Warning( "GetPointOutsideObstacles: no valid point found" );
}

/*
============
GetFirstBlockingObstacle
============
*/
bool GetFirstBlockingObstacle( const obstacle_t* obstacles, int numObstacles, int skipObstacle, const idVec2& startPos, const idVec2& delta, float& blockingScale, int& blockingObstacle, int& blockingEdgeNum )
{
	int i, edgeNums[2];
	float dist, scale1, scale2;
	idVec2 bounds[2];

	// get bounds for the current movement delta
	bounds[0] = startPos - idVec2( CM_BOX_EPSILON, CM_BOX_EPSILON );
	bounds[1] = startPos + idVec2( CM_BOX_EPSILON, CM_BOX_EPSILON );
	bounds[IEEE_FLT_SIGNBITNOTSET( delta.x )].x += delta.x;
	bounds[IEEE_FLT_SIGNBITNOTSET( delta.y )].y += delta.y;

	// test for obstacles blocking the path
	blockingScale = idMath::INFINITUM;
	dist = delta.Length();
	for( i = 0; i < numObstacles; i++ )
	{
		if( i == skipObstacle )
		{
			continue;
		}
		if( bounds[0].x > obstacles[i].bounds[1].x || bounds[0].y > obstacles[i].bounds[1].y ||
				bounds[1].x < obstacles[i].bounds[0].x || bounds[1].y < obstacles[i].bounds[0].y )
		{
			continue;
		}
		if( obstacles[i].winding.RayIntersection( startPos, delta, scale1, scale2, edgeNums ) )
		{
			if( scale1 < blockingScale && scale1 * dist > -0.01f && scale2 * dist > 0.01f )
			{
				blockingScale = scale1;
				blockingObstacle = i;
				blockingEdgeNum = edgeNums[0];
			}
		}
	}
	return ( blockingScale < 1.0f );
}

/*
============
GetObstacles
============
*/
int GetObstacles( const idPhysics* physics, const idAAS* aas, const idEntity* ignore, int areaNum, const idVec3& startPos, const idVec3& seekPos, obstacle_t* obstacles, int maxObstacles, idBounds& clipBounds )
{
	int i, j, numListedClipModels, numObstacles, numVerts, clipMask, blockingObstacle, blockingEdgeNum;
	int wallEdges[MAX_AAS_WALL_EDGES], numWallEdges, verts[2], lastVerts[2], nextVerts[2];
	float stepHeight, headHeight, blockingScale, min, max;
	idVec3 seekDelta, silVerts[32], start, end, nextStart, nextEnd;
	idVec2 expBounds[2], edgeDir, edgeNormal, nextEdgeDir, nextEdgeNormal, lastEdgeNormal;
	idVec2 obDelta;
	idPhysics* obPhys;
	idBox box;
	idEntity* obEnt;
	idClipModel* clipModel;
	idClipModel* clipModelList[ MAX_GENTITIES ];

	numObstacles = 0;

	seekDelta = seekPos - startPos;
	expBounds[0] = physics->GetBounds()[0].ToVec2() - idVec2( CM_BOX_EPSILON, CM_BOX_EPSILON );
	expBounds[1] = physics->GetBounds()[1].ToVec2() + idVec2( CM_BOX_EPSILON, CM_BOX_EPSILON );

	physics->GetAbsBounds().AxisProjection( -physics->GetGravityNormal(), stepHeight, headHeight );
	stepHeight += aas->GetSettings()->maxStepHeight;

	// clip bounds for the obstacle search space
	clipBounds[0] = clipBounds[1] = startPos;
	clipBounds.AddPoint( seekPos );
	clipBounds.ExpandSelf( MAX_OBSTACLE_RADIUS );
	clipMask = physics->GetClipMask();

	// find all obstacles touching the clip bounds
	numListedClipModels = gameLocal.clip.ClipModelsTouchingBounds( clipBounds, clipMask, clipModelList, MAX_GENTITIES );

	for( i = 0; i < numListedClipModels && numObstacles < MAX_OBSTACLES; i++ )
	{
		clipModel = clipModelList[i];
		obEnt = clipModel->GetEntity();

		if( !clipModel->IsTraceModel() )
		{
			continue;
		}

		if( obEnt->IsType( idActor::Type ) )
		{
			obPhys = obEnt->GetPhysics();
			// ignore myself, my enemy, and dead bodies
			if( ( obPhys == physics ) || ( obEnt == ignore ) || ( obEnt->health <= 0 ) )
			{
				continue;
			}
			// if the actor is moving
			idVec3 v1 = obPhys->GetLinearVelocity();
			if( v1.LengthSqr() > Square( 10.0f ) )
			{
				idVec3 v2 = physics->GetLinearVelocity();
				if( v2.LengthSqr() > Square( 10.0f ) )
				{
					// if moving in about the same direction
					if( v1 * v2 > 0.0f )
					{
						continue;
					}
				}
			}
		}
		else if( obEnt->IsType( idMoveable::Type ) )
		{
			// moveables are considered obstacles
		}
		else
		{
			// ignore everything else
			continue;
		}

		// check if we can step over the object
		clipModel->GetAbsBounds().AxisProjection( -physics->GetGravityNormal(), min, max );
		if( max < stepHeight || min > headHeight )
		{
			// can step over this one
			continue;
		}

		// project a box containing the obstacle onto the floor plane
		box = idBox( clipModel->GetBounds(), clipModel->GetOrigin(), clipModel->GetAxis() );
		numVerts = box.GetParallelProjectionSilhouetteVerts( physics->GetGravityNormal(), silVerts );

		// create a 2D winding for the obstacle;
		obstacle_t& obstacle = obstacles[numObstacles++];
		obstacle.winding.Clear();
		for( j = 0; j < numVerts; j++ )
		{
			obstacle.winding.AddPoint( silVerts[j].ToVec2() );
		}

		if( ai_showObstacleAvoidance.GetBool() )
		{
			for( j = 0; j < numVerts; j++ )
			{
				silVerts[j].z = startPos.z;
			}
			for( j = 0; j < numVerts; j++ )
			{
				gameRenderWorld->DebugArrow( colorWhite, silVerts[j], silVerts[( j + 1 ) % numVerts], 4 );
			}
		}

		// expand the 2D winding for collision with a 2D box
		obstacle.winding.ExpandForAxialBox( expBounds );
		obstacle.winding.GetBounds( obstacle.bounds );
		obstacle.entity = obEnt;
	}

	// if there are no dynamic obstacles the path should be through valid AAS space
	if( numObstacles == 0 )
	{
		return 0;
	}

	// if the current path doesn't intersect any dynamic obstacles the path should be through valid AAS space
	if( PointInsideObstacle( obstacles, numObstacles, startPos.ToVec2() ) == -1 )
	{
		if( !GetFirstBlockingObstacle( obstacles, numObstacles, -1, startPos.ToVec2(), seekDelta.ToVec2(), blockingScale, blockingObstacle, blockingEdgeNum ) )
		{
			return 0;
		}
	}

	// create obstacles for AAS walls
	if( aas )
	{
		float halfBoundsSize = ( expBounds[ 1 ].x - expBounds[ 0 ].x ) * 0.5f;

		numWallEdges = aas->GetWallEdges( areaNum, clipBounds, TFL_WALK, wallEdges, MAX_AAS_WALL_EDGES );
		aas->SortWallEdges( wallEdges, numWallEdges );

		lastVerts[0] = lastVerts[1] = 0;
		lastEdgeNormal.Zero();
		nextVerts[0] = nextVerts[1] = 0;
		for( i = 0; i < numWallEdges && numObstacles < MAX_OBSTACLES; i++ )
		{
			aas->GetEdge( wallEdges[i], start, end );
			aas->GetEdgeVertexNumbers( wallEdges[i], verts );
			edgeDir = end.ToVec2() - start.ToVec2();
			edgeDir.Normalize();
			edgeNormal.x = edgeDir.y;
			edgeNormal.y = -edgeDir.x;
			if( i < numWallEdges - 1 )
			{
				aas->GetEdge( wallEdges[i + 1], nextStart, nextEnd );
				aas->GetEdgeVertexNumbers( wallEdges[i + 1], nextVerts );
				nextEdgeDir = nextEnd.ToVec2() - nextStart.ToVec2();
				nextEdgeDir.Normalize();
				nextEdgeNormal.x = nextEdgeDir.y;
				nextEdgeNormal.y = -nextEdgeDir.x;
			}

			obstacle_t& obstacle = obstacles[numObstacles++];
			obstacle.winding.Clear();
			obstacle.winding.AddPoint( end.ToVec2() );
			obstacle.winding.AddPoint( start.ToVec2() );
			obstacle.winding.AddPoint( start.ToVec2() - edgeDir - edgeNormal * halfBoundsSize );
			obstacle.winding.AddPoint( end.ToVec2() + edgeDir - edgeNormal * halfBoundsSize );
			if( lastVerts[1] == verts[0] )
			{
				obstacle.winding[2] -= lastEdgeNormal * halfBoundsSize;
			}
			else
			{
				obstacle.winding[1] -= edgeDir;
			}
			if( verts[1] == nextVerts[0] )
			{
				obstacle.winding[3] -= nextEdgeNormal * halfBoundsSize;
			}
			else
			{
				obstacle.winding[0] += edgeDir;
			}
			obstacle.winding.GetBounds( obstacle.bounds );
			obstacle.entity = NULL;

			memcpy( lastVerts, verts, sizeof( lastVerts ) );
			lastEdgeNormal = edgeNormal;
		}
	}

	// show obstacles
	if( ai_showObstacleAvoidance.GetBool() )
	{
		for( i = 0; i < numObstacles; i++ )
		{
			obstacle_t& obstacle = obstacles[i];
			for( j = 0; j < obstacle.winding.GetNumPoints(); j++ )
			{
				silVerts[j].ToVec2() = obstacle.winding[j];
				silVerts[j].z = startPos.z;
			}
			for( j = 0; j < obstacle.winding.GetNumPoints(); j++ )
			{
				gameRenderWorld->DebugArrow( colorGreen, silVerts[j], silVerts[( j + 1 ) % obstacle.winding.GetNumPoints()], 4 );
			}
		}
	}

	return numObstacles;
}

/*
============
FreePathTree_r
============
*/
void FreePathTree_r( pathNode_t* node )
{
	if( node->children[0] )
	{
		FreePathTree_r( node->children[0] );
	}
	if( node->children[1] )
	{
		FreePathTree_r( node->children[1] );
	}
	pathNodeAllocator.Free( node );
}

/*
============
DrawPathTree
============
*/
void DrawPathTree( const pathNode_t* root, const float height )
{
	int i;
	idVec3 start, end;
	const pathNode_t* node;

	for( node = root; node; node = node->next )
	{
		for( i = 0; i < 2; i++ )
		{
			if( node->children[i] )
			{
				start.ToVec2() = node->pos;
				start.z = height;
				end.ToVec2() = node->children[i]->pos;
				end.z = height;
				gameRenderWorld->DebugArrow( node->edgeNum == -1 ? colorYellow : i ? colorBlue : colorRed, start, end, 1 );
				break;
			}
		}
	}
}

/*
============
GetPathNodeDelta
============
*/
bool GetPathNodeDelta( pathNode_t* node, const obstacle_t* obstacles, const idVec2& seekPos, bool blocked )
{
	int numPoints, edgeNum;
	bool facing;
	idVec2 seekDelta, dir;
	pathNode_t* n;

	numPoints = obstacles[node->obstacle].winding.GetNumPoints();

	// get delta along the current edge
	while( 1 )
	{
		edgeNum = ( node->edgeNum + node->dir ) % numPoints;
		node->delta = obstacles[node->obstacle].winding[edgeNum] - node->pos;
		if( node->delta.LengthSqr() > 0.01f )
		{
			break;
		}
		node->edgeNum = ( node->edgeNum + numPoints + ( 2 * node->dir - 1 ) ) % numPoints;
	}

	// if not blocked
	if( !blocked )
	{

		// test if the current edge faces the goal
		seekDelta = seekPos - node->pos;
		facing = ( ( 2 * node->dir - 1 ) * ( node->delta.x * seekDelta.y - node->delta.y * seekDelta.x ) ) >= 0.0f;

		// if the current edge faces goal and the line from the current
		// position to the goal does not intersect the current path
		if( facing && !LineIntersectsPath( node->pos, seekPos, node->parent ) )
		{
			node->delta = seekPos - node->pos;
			node->edgeNum = -1;
		}
	}

	// if the delta is along the obstacle edge
	if( node->edgeNum != -1 )
	{
		// if the edge is found going from this node to the root node
		for( n = node->parent; n; n = n->parent )
		{

			if( node->obstacle != n->obstacle || node->edgeNum != n->edgeNum )
			{
				continue;
			}

			// test whether or not the edge segments actually overlap
			if( n->pos * node->delta > ( node->pos + node->delta ) * node->delta )
			{
				continue;
			}
			if( node->pos * node->delta > ( n->pos + n->delta ) * node->delta )
			{
				continue;
			}

			break;
		}
		if( n )
		{
			return false;
		}
	}
	return true;
}

/*
============
BuildPathTree
============
*/
pathNode_t* BuildPathTree( const obstacle_t* obstacles, int numObstacles, const idBounds& clipBounds, const idVec2& startPos, const idVec2& seekPos, obstaclePath_t& path )
{
	int blockingEdgeNum, blockingObstacle, obstaclePoints, bestNumNodes = MAX_OBSTACLE_PATH;
	float blockingScale;
	pathNode_t* root, *node, *child;
	// gcc 4.0
	idQueueTemplate<pathNode_t, offsetof( pathNode_t, next ) > pathNodeQueue, treeQueue;

	root = pathNodeAllocator.Alloc();
	root->Init();
	root->pos = startPos;

	root->delta = seekPos - root->pos;
	root->numNodes = 0;
	pathNodeQueue.Add( root );

	for( node = pathNodeQueue.Get(); node != NULL && pathNodeAllocator.GetAllocCount() < MAX_PATH_NODES; node = pathNodeQueue.Get() )
	{

		treeQueue.Add( node );

		// if this path has more than twice the number of nodes than the best path so far
		if( node->numNodes > bestNumNodes * 2 )
		{
			continue;
		}

		// don't move outside of the clip bounds
		idVec2 endPos = node->pos + node->delta;
		if( endPos.x - CLIP_BOUNDS_EPSILON < clipBounds[0].x || endPos.x + CLIP_BOUNDS_EPSILON > clipBounds[1].x ||
				endPos.y - CLIP_BOUNDS_EPSILON < clipBounds[0].y || endPos.y + CLIP_BOUNDS_EPSILON > clipBounds[1].y )
		{
			continue;
		}

		// if an obstacle is blocking the path
		if( GetFirstBlockingObstacle( obstacles, numObstacles, node->obstacle, node->pos, node->delta, blockingScale, blockingObstacle, blockingEdgeNum ) )
		{

			if( path.firstObstacle == NULL )
			{
				path.firstObstacle = obstacles[blockingObstacle].entity;
			}

			node->delta *= blockingScale;

			if( node->edgeNum == -1 )
			{
				node->children[0] = pathNodeAllocator.Alloc();
				node->children[0]->Init();
				node->children[1] = pathNodeAllocator.Alloc();
				node->children[1]->Init();
				node->children[0]->dir = 0;
				node->children[1]->dir = 1;
				node->children[0]->parent = node->children[1]->parent = node;
				node->children[0]->pos = node->children[1]->pos = node->pos + node->delta;
				node->children[0]->obstacle = node->children[1]->obstacle = blockingObstacle;
				node->children[0]->edgeNum = node->children[1]->edgeNum = blockingEdgeNum;
				node->children[0]->numNodes = node->children[1]->numNodes = node->numNodes + 1;
				if( GetPathNodeDelta( node->children[0], obstacles, seekPos, true ) )
				{
					pathNodeQueue.Add( node->children[0] );
				}
				if( GetPathNodeDelta( node->children[1], obstacles, seekPos, true ) )
				{
					pathNodeQueue.Add( node->children[1] );
				}
			}
			else
			{
				node->children[node->dir] = child = pathNodeAllocator.Alloc();
				child->Init();
				child->dir = node->dir;
				child->parent = node;
				child->pos = node->pos + node->delta;
				child->obstacle = blockingObstacle;
				child->edgeNum = blockingEdgeNum;
				child->numNodes = node->numNodes + 1;
				if( GetPathNodeDelta( child, obstacles, seekPos, true ) )
				{
					pathNodeQueue.Add( child );
				}
			}
		}
		else
		{
			node->children[node->dir] = child = pathNodeAllocator.Alloc();
			child->Init();
			child->dir = node->dir;
			child->parent = node;
			child->pos = node->pos + node->delta;
			child->numNodes = node->numNodes + 1;

			// there is a free path towards goal
			if( node->edgeNum == -1 )
			{
				if( node->numNodes < bestNumNodes )
				{
					bestNumNodes = node->numNodes;
				}
				continue;
			}

			child->obstacle = node->obstacle;
			obstaclePoints = obstacles[node->obstacle].winding.GetNumPoints();
			child->edgeNum = ( node->edgeNum + obstaclePoints + ( 2 * node->dir - 1 ) ) % obstaclePoints;

			if( GetPathNodeDelta( child, obstacles, seekPos, false ) )
			{
				pathNodeQueue.Add( child );
			}
		}
	}

	return root;
}

/*
============
PrunePathTree
============
*/
void PrunePathTree( pathNode_t* root, const idVec2& seekPos )
{
	int i;
	float bestDist;
	pathNode_t* node, *lastNode, *n, *bestNode;

	node = root;
	while( node )
	{

		node->dist = ( seekPos - node->pos ).LengthSqr();

		if( node->children[0] )
		{
			node = node->children[0];
		}
		else if( node->children[1] )
		{
			node = node->children[1];
		}
		else
		{

			// find the node closest to the goal along this path
			bestDist = idMath::INFINITUM;
			bestNode = node;
			for( n = node; n; n = n->parent )
			{
				if( n->children[0] && n->children[1] )
				{
					break;
				}
				if( n->dist < bestDist )
				{
					bestDist = n->dist;
					bestNode = n;
				}
			}

			// free tree down from the best node
			for( i = 0; i < 2; i++ )
			{
				if( bestNode->children[i] )
				{
					FreePathTree_r( bestNode->children[i] );
					bestNode->children[i] = NULL;
				}
			}

			for( lastNode = bestNode, node = bestNode->parent; node; lastNode = node, node = node->parent )
			{
				if( node->children[1] && ( node->children[1] != lastNode ) )
				{
					node = node->children[1];
					break;
				}
			}
		}
	}
}

/*
============
OptimizePath
============
*/
int OptimizePath( const pathNode_t* root, const pathNode_t* leafNode, const obstacle_t* obstacles, int numObstacles, idVec2 optimizedPath[MAX_OBSTACLE_PATH] )
{
	int i, numPathPoints, edgeNums[2];
	const pathNode_t* curNode, *nextNode;
	idVec2 curPos, curDelta, bounds[2];
	float scale1, scale2, curLength;

	optimizedPath[0] = root->pos;
	numPathPoints = 1;

	for( nextNode = curNode = root; curNode != leafNode; curNode = nextNode )
	{

		for( nextNode = leafNode; nextNode->parent != curNode; nextNode = nextNode->parent )
		{

			// can only take shortcuts when going from one object to another
			if( nextNode->obstacle == curNode->obstacle )
			{
				continue;
			}

			curPos = curNode->pos;
			curDelta = nextNode->pos - curPos;
			curLength = curDelta.Length();

			// get bounds for the current movement delta
			bounds[0] = curPos - idVec2( CM_BOX_EPSILON, CM_BOX_EPSILON );
			bounds[1] = curPos + idVec2( CM_BOX_EPSILON, CM_BOX_EPSILON );
			bounds[IEEE_FLT_SIGNBITNOTSET( curDelta.x )].x += curDelta.x;
			bounds[IEEE_FLT_SIGNBITNOTSET( curDelta.y )].y += curDelta.y;

			// test if the shortcut intersects with any obstacles
			for( i = 0; i < numObstacles; i++ )
			{
				if( bounds[0].x > obstacles[i].bounds[1].x || bounds[0].y > obstacles[i].bounds[1].y ||
						bounds[1].x < obstacles[i].bounds[0].x || bounds[1].y < obstacles[i].bounds[0].y )
				{
					continue;
				}
				if( obstacles[i].winding.RayIntersection( curPos, curDelta, scale1, scale2, edgeNums ) )
				{
					if( scale1 >= 0.0f && scale1 <= 1.0f && ( i != nextNode->obstacle || scale1 * curLength < curLength - 0.5f ) )
					{
						break;
					}
					if( scale2 >= 0.0f && scale2 <= 1.0f && ( i != nextNode->obstacle || scale2 * curLength < curLength - 0.5f ) )
					{
						break;
					}
				}
			}
			if( i >= numObstacles )
			{
				break;
			}
		}

		// store the next position along the optimized path
		optimizedPath[numPathPoints++] = nextNode->pos;
	}

	return numPathPoints;
}

/*
============
PathLength
============
*/
float PathLength( idVec2 optimizedPath[MAX_OBSTACLE_PATH], int numPathPoints, const idVec2& curDir )
{
	int i;
	float pathLength;

	// calculate the path length
	pathLength = 0.0f;
	for( i = 0; i < numPathPoints - 1; i++ )
	{
		pathLength += ( optimizedPath[i + 1] - optimizedPath[i] ).LengthFast();
	}

	// add penalty if this path does not go in the current direction
	if( curDir * ( optimizedPath[1] - optimizedPath[0] ) < 0.0f )
	{
		pathLength += 100.0f;
	}
	return pathLength;
}

/*
============
FindOptimalPath

  Returns true if there is a path all the way to the goal.
============
*/
bool FindOptimalPath( const pathNode_t* root, const obstacle_t* obstacles, int numObstacles, const float height, const idVec3& curDir, idVec3& seekPos )
{
	int i, numPathPoints, bestNumPathPoints;
	const pathNode_t* node, *lastNode, *bestNode;
	idVec2 optimizedPath[MAX_OBSTACLE_PATH];
	float pathLength, bestPathLength;
	bool pathToGoalExists, optimizedPathCalculated;

	seekPos.Zero();
	seekPos.z = height;

	pathToGoalExists = false;
	optimizedPathCalculated = false;

	bestNode = root;
	bestNumPathPoints = 0;
	bestPathLength = idMath::INFINITUM;

	node = root;
	while( node )
	{

		pathToGoalExists |= ( node->dist < 0.1f );

		if( node->dist <= bestNode->dist )
		{

			if( idMath::Fabs( node->dist - bestNode->dist ) < 0.1f )
			{

				if( !optimizedPathCalculated )
				{
					bestNumPathPoints = OptimizePath( root, bestNode, obstacles, numObstacles, optimizedPath );
					bestPathLength = PathLength( optimizedPath, bestNumPathPoints, curDir.ToVec2() );
					seekPos.ToVec2() = optimizedPath[1];
				}

				numPathPoints = OptimizePath( root, node, obstacles, numObstacles, optimizedPath );
				pathLength = PathLength( optimizedPath, numPathPoints, curDir.ToVec2() );

				if( pathLength < bestPathLength )
				{
					bestNode = node;
					bestNumPathPoints = numPathPoints;
					bestPathLength = pathLength;
					seekPos.ToVec2() = optimizedPath[1];
				}
				optimizedPathCalculated = true;

			}
			else
			{

				bestNode = node;
				optimizedPathCalculated = false;
			}
		}

		if( node->children[0] )
		{
			node = node->children[0];
		}
		else if( node->children[1] )
		{
			node = node->children[1];
		}
		else
		{
			for( lastNode = node, node = node->parent; node; lastNode = node, node = node->parent )
			{
				if( node->children[1] && node->children[1] != lastNode )
				{
					node = node->children[1];
					break;
				}
			}
		}
	}

	if( root != NULL )
	{
		if( !pathToGoalExists )
		{
			if( root->children[0] != NULL )
			{
				seekPos.ToVec2() = root->children[0]->pos;
			}
			else
			{
				seekPos.ToVec2() = root->pos;
			}
		}
		else if( !optimizedPathCalculated )
		{
			OptimizePath( root, bestNode, obstacles, numObstacles, optimizedPath );
			seekPos.ToVec2() = optimizedPath[1];
		}

		if( ai_showObstacleAvoidance.GetBool() )
		{
			idVec3 start, end;
			start.z = end.z = height + 4.0f;
			numPathPoints = OptimizePath( root, bestNode, obstacles, numObstacles, optimizedPath );
			for( i = 0; i < numPathPoints - 1; i++ )
			{
				start.ToVec2() = optimizedPath[i];
				end.ToVec2() = optimizedPath[i + 1];
				gameRenderWorld->DebugArrow( colorCyan, start, end, 1 );
			}
		}
	}

	return pathToGoalExists;
}

/*
============
idAI::FindPathAroundObstacles

  Finds a path around dynamic obstacles using a path tree with clockwise and counter clockwise edge walks.
============
*/
bool idAI::FindPathAroundObstacles( const idPhysics* physics, const idAAS* aas, const idEntity* ignore, const idVec3& startPos, const idVec3& seekPos, obstaclePath_t& path )
{
	int numObstacles, areaNum, insideObstacle;
	obstacle_t obstacles[MAX_OBSTACLES];
	idBounds clipBounds;
	idBounds bounds;
	pathNode_t* root;
	bool pathToGoalExists;

	path.seekPos = seekPos;
	path.firstObstacle = NULL;
	path.startPosOutsideObstacles = startPos;
	path.startPosObstacle = NULL;
	path.seekPosOutsideObstacles = seekPos;
	path.seekPosObstacle = NULL;

	if( !aas )
	{
		return true;
	}

	bounds[1] = aas->GetSettings()->boundingBoxes[0][1];
	bounds[0] = -bounds[1];
	bounds[1].z = 32.0f;

	// get the AAS area number and a valid point inside that area
	areaNum = aas->PointReachableAreaNum( path.startPosOutsideObstacles, bounds, ( AREA_REACHABLE_WALK | AREA_REACHABLE_FLY ) );
	aas->PushPointIntoAreaNum( areaNum, path.startPosOutsideObstacles );

	// get all the nearby obstacles
	numObstacles = GetObstacles( physics, aas, ignore, areaNum, path.startPosOutsideObstacles, path.seekPosOutsideObstacles, obstacles, MAX_OBSTACLES, clipBounds );

	// get a source position outside the obstacles
	GetPointOutsideObstacles( obstacles, numObstacles, path.startPosOutsideObstacles.ToVec2(), &insideObstacle, NULL );
	if( insideObstacle != -1 )
	{
		path.startPosObstacle = obstacles[insideObstacle].entity;
	}

	// get a goal position outside the obstacles
	GetPointOutsideObstacles( obstacles, numObstacles, path.seekPosOutsideObstacles.ToVec2(), &insideObstacle, NULL );
	if( insideObstacle != -1 )
	{
		path.seekPosObstacle = obstacles[insideObstacle].entity;
	}

	// if start and destination are pushed to the same point, we don't have a path around the obstacle
	if( ( path.seekPosOutsideObstacles.ToVec2() - path.startPosOutsideObstacles.ToVec2() ).LengthSqr() < Square( 1.0f ) )
	{
		if( ( seekPos.ToVec2() - startPos.ToVec2() ).LengthSqr() > Square( 2.0f ) )
		{
			return false;
		}
	}

	// build a path tree
	root = BuildPathTree( obstacles, numObstacles, clipBounds, path.startPosOutsideObstacles.ToVec2(), path.seekPosOutsideObstacles.ToVec2(), path );

	// draw the path tree
	if( ai_showObstacleAvoidance.GetBool() )
	{
		DrawPathTree( root, physics->GetOrigin().z );
	}

	// prune the tree
	PrunePathTree( root, path.seekPosOutsideObstacles.ToVec2() );

	// find the optimal path
	pathToGoalExists = FindOptimalPath( root, obstacles, numObstacles, physics->GetOrigin().z, physics->GetLinearVelocity(), path.seekPos );

	// free the tree
	FreePathTree_r( root );

	return pathToGoalExists;
}

/*
============
idAI::FreeObstacleAvoidanceNodes
============
*/
void idAI::FreeObstacleAvoidanceNodes()
{
	pathNodeAllocator.Shutdown();
}


/*
===============================================================================

	Path Prediction

	Uses the AAS to quickly and accurately predict a path for a certain
	period of time based on an initial position and velocity.

===============================================================================
*/

const float OVERCLIP			= 1.001f;
const int MAX_FRAME_SLIDE		= 5;

typedef struct pathTrace_s
{
	float					fraction;
	idVec3					endPos;
	idVec3					normal;
	const idEntity* 		blockingEntity;
} pathTrace_t;

/*
============
PathTrace

  Returns true if a stop event was triggered.
============
*/
bool PathTrace( const idEntity* ent, const idAAS* aas, const idVec3& start, const idVec3& end, int stopEvent, struct pathTrace_s& trace, predictedPath_t& path )
{
	trace_t clipTrace;
	aasTrace_t aasTrace;

	memset( &trace, 0, sizeof( trace ) );

	if( !aas || !aas->GetSettings() )
	{

		gameLocal.clip.Translation( clipTrace, start, end, ent->GetPhysics()->GetClipModel(),
									ent->GetPhysics()->GetClipModel()->GetAxis(), MASK_MONSTERSOLID, ent );

		// NOTE: could do (expensive) ledge detection here for when there is no AAS file

		trace.fraction = clipTrace.fraction;
		trace.endPos = clipTrace.endpos;
		trace.normal = clipTrace.c.normal;
		trace.blockingEntity = gameLocal.entities[ clipTrace.c.entityNum ];
	}
	else
	{
		aasTrace.getOutOfSolid = true;
		if( stopEvent & SE_ENTER_LEDGE_AREA )
		{
			aasTrace.flags |= AREA_LEDGE;
		}
		if( stopEvent & SE_ENTER_OBSTACLE )
		{
			aasTrace.travelFlags |= TFL_INVALID;
		}

		aas->Trace( aasTrace, start, end );

		gameLocal.clip.TranslationEntities( clipTrace, start, aasTrace.endpos, ent->GetPhysics()->GetClipModel(),
											ent->GetPhysics()->GetClipModel()->GetAxis(), MASK_MONSTERSOLID, ent );

		if( clipTrace.fraction >= 1.0f )
		{

			trace.fraction = aasTrace.fraction;
			trace.endPos = aasTrace.endpos;
			trace.normal = aas->GetPlane( aasTrace.planeNum ).Normal();
			trace.blockingEntity = gameLocal.world;

			if( aasTrace.fraction < 1.0f )
			{
				if( stopEvent & SE_ENTER_LEDGE_AREA )
				{
					if( aas->AreaFlags( aasTrace.blockingAreaNum ) & AREA_LEDGE )
					{
						path.endPos = trace.endPos;
						path.endNormal = trace.normal;
						path.endEvent = SE_ENTER_LEDGE_AREA;
						path.blockingEntity = trace.blockingEntity;

						if( ai_debugMove.GetBool() )
						{
							gameRenderWorld->DebugLine( colorRed, start, aasTrace.endpos );
						}
						return true;
					}
				}
				if( stopEvent & SE_ENTER_OBSTACLE )
				{
					if( aas->AreaTravelFlags( aasTrace.blockingAreaNum ) & TFL_INVALID )
					{
						path.endPos = trace.endPos;
						path.endNormal = trace.normal;
						path.endEvent = SE_ENTER_OBSTACLE;
						path.blockingEntity = trace.blockingEntity;

						if( ai_debugMove.GetBool() )
						{
							gameRenderWorld->DebugLine( colorRed, start, aasTrace.endpos );
						}
						return true;
					}
				}
			}
		}
		else
		{
			trace.fraction = clipTrace.fraction;
			trace.endPos = clipTrace.endpos;
			trace.normal = clipTrace.c.normal;
			trace.blockingEntity = gameLocal.entities[ clipTrace.c.entityNum ];
		}
	}

	if( trace.fraction >= 1.0f )
	{
		trace.blockingEntity = NULL;
	}

	return false;
}

/*
============
idAI::PredictPath

  Can also be used when there is no AAS file available however ledges are not detected.
============
*/
bool idAI::PredictPath( const idEntity* ent, const idAAS* aas, const idVec3& start, const idVec3& velocity, int totalTime, int frameTime, int stopEvent, predictedPath_t& path )
{
	int i, j, step, numFrames, curFrameTime;
	idVec3 delta, curStart, curEnd, curVelocity, lastEnd, stepUp, tmpStart;
	idVec3 gravity, gravityDir, invGravityDir;
	float maxStepHeight, minFloorCos;
	pathTrace_t trace;

	if( aas && aas->GetSettings() )
	{
		gravity = aas->GetSettings()->gravity;
		gravityDir = aas->GetSettings()->gravityDir;
		invGravityDir = aas->GetSettings()->invGravityDir;
		maxStepHeight = aas->GetSettings()->maxStepHeight;
		minFloorCos = aas->GetSettings()->minFloorCos;
	}
	else
	{
		gravity = DEFAULT_GRAVITY_VEC3;
		gravityDir = idVec3( 0, 0, -1 );
		invGravityDir = idVec3( 0, 0, 1 );
		maxStepHeight = 14.0f;
		minFloorCos = 0.7f;
	}

	path.endPos = start;
	path.endVelocity = velocity;
	path.endNormal.Zero();
	path.endEvent = 0;
	path.endTime = 0;
	path.blockingEntity = NULL;

	curStart = start;
	curVelocity = velocity;

	// RB: fixed integer division by 0
	if( frameTime != 0 )
	{
		numFrames = ( totalTime + frameTime - 1 ) / frameTime;
	}
	else
	{
		numFrames = ( totalTime + frameTime - 1 );
	}
	// RB end
	curFrameTime = frameTime;
	for( i = 0; i < numFrames; i++ )
	{

		if( i == numFrames - 1 )
		{
			curFrameTime = totalTime - i * curFrameTime;
		}

		delta = curVelocity * curFrameTime * 0.001f;

		path.endVelocity = curVelocity;
		path.endTime = i * frameTime;

		// allow sliding along a few surfaces per frame
		for( j = 0; j < MAX_FRAME_SLIDE; j++ )
		{

			idVec3 lineStart = curStart;

			// allow stepping up three times per frame
			for( step = 0; step < 3; step++ )
			{

				curEnd = curStart + delta;
				if( PathTrace( ent, aas, curStart, curEnd, stopEvent, trace, path ) )
				{
					return true;
				}

				if( step )
				{

					// step down at end point
					tmpStart = trace.endPos;
					curEnd = tmpStart - stepUp;
					if( PathTrace( ent, aas, tmpStart, curEnd, stopEvent, trace, path ) )
					{
						return true;
					}

					// if not moved any further than without stepping up, or if not on a floor surface
					if( ( lastEnd - start ).LengthSqr() > ( trace.endPos - start ).LengthSqr() - 0.1f ||
							( trace.normal * invGravityDir ) < minFloorCos )
					{
						if( stopEvent & SE_BLOCKED )
						{
							path.endPos = lastEnd;
							path.endEvent = SE_BLOCKED;

							if( ai_debugMove.GetBool() )
							{
								gameRenderWorld->DebugLine( colorRed, lineStart, lastEnd );
							}

							return true;
						}

						curStart = lastEnd;
						break;
					}
				}

				path.endNormal = trace.normal;
				path.blockingEntity = trace.blockingEntity;

				// if the trace is not blocked or blocked by a floor surface
				if( trace.fraction >= 1.0f || ( trace.normal * invGravityDir ) > minFloorCos )
				{
					curStart = trace.endPos;
					break;
				}

				// save last result
				lastEnd = trace.endPos;

				// step up
				stepUp = invGravityDir * maxStepHeight;
				if( PathTrace( ent, aas, curStart, curStart + stepUp, stopEvent, trace, path ) )
				{
					return true;
				}
				stepUp *= trace.fraction;
				curStart = trace.endPos;
			}

			if( ai_debugMove.GetBool() )
			{
				gameRenderWorld->DebugLine( colorRed, lineStart, curStart );
			}

			if( trace.fraction >= 1.0f )
			{
				break;
			}

			delta.ProjectOntoPlane( trace.normal, OVERCLIP );
			curVelocity.ProjectOntoPlane( trace.normal, OVERCLIP );

			if( stopEvent & SE_BLOCKED )
			{
				// if going backwards
				if( ( curVelocity - gravityDir * curVelocity * gravityDir ) *
						( velocity - gravityDir * velocity * gravityDir ) < 0.0f )
				{
					path.endPos = curStart;
					path.endEvent = SE_BLOCKED;

					return true;
				}
			}
		}

		if( j >= MAX_FRAME_SLIDE )
		{
			if( stopEvent & SE_BLOCKED )
			{
				path.endPos = curStart;
				path.endEvent = SE_BLOCKED;
				return true;
			}
		}

		// add gravity
		curVelocity += gravity * frameTime * 0.001f;
	}

	path.endTime = totalTime;
	path.endVelocity = curVelocity;
	path.endPos = curStart;
	path.endEvent = 0;

	return false;
}


/*
===============================================================================

	Trajectory Prediction

	Finds the best collision free trajectory for a clip model based on an
	initial position, target position and speed.

===============================================================================
*/

/*
=====================
Ballistics

  get the ideal aim pitch angle in order to hit the target
  also get the time it takes for the projectile to arrive at the target
=====================
*/

int Ballistics( const idVec3& start, const idVec3& end, float speed, float gravity, ballistics_t bal[2] )
{
	int n, i;
	float x, y, a, b, c, d, sqrtd, inva, p[2];

	x = ( end.ToVec2() - start.ToVec2() ).Length();
	y = end[2] - start[2];

	a = 4.0f * y * y + 4.0f * x * x;
	b = -4.0f * speed * speed - 4.0f * y * gravity;
	c = gravity * gravity;

	d = b * b - 4.0f * a * c;
	if( d <= 0.0f || a == 0.0f )
	{
		return 0;
	}
	sqrtd = idMath::Sqrt( d );
	inva = 0.5f / a;
	p[0] = ( - b + sqrtd ) * inva;
	p[1] = ( - b - sqrtd ) * inva;
	n = 0;
	for( i = 0; i < 2; i++ )
	{
		if( p[i] <= 0.0f )
		{
			continue;
		}
		d = idMath::Sqrt( p[i] );
		bal[n].angle = atan2( 0.5f * ( 2.0f * y * p[i] - gravity ) / d, d * x );
		bal[n].time = x / ( cos( bal[n].angle ) * speed );
		bal[n].angle = idMath::AngleNormalize180( RAD2DEG( bal[n].angle ) );
		n++;
	}

	return n;
}

#if 0
// not used
/*
=====================
HeightForTrajectory

Returns the maximum hieght of a given trajectory
=====================
*/
static float HeightForTrajectory( const idVec3& start, float zVel, float gravity )
{
	float maxHeight, t;

	t = zVel / gravity;
	// maximum height of projectile
	maxHeight = start.z - 0.5f * gravity * ( t * t );

	return maxHeight;
}
#endif

/*
=====================
idAI::TestTrajectory
=====================
*/
bool idAI::TestTrajectory( const idVec3& start, const idVec3& end, float zVel, float gravity, float time, float max_height, const idClipModel* clip, int clipmask, const idEntity* ignore, const idEntity* targetEntity, int drawtime )
{
	int i, numSegments;
	float maxHeight, t, t2;
	idVec3 points[5];
	trace_t trace;
	bool result;

	t = zVel / gravity;
	// maximum height of projectile
	maxHeight = start.z - 0.5f * gravity * ( t * t );
	// time it takes to fall from the top to the end height
	t = idMath::Sqrt( ( maxHeight - end.z ) / ( 0.5f * -gravity ) );

	// start of parabolic
	points[0] = start;

	if( t < time )
	{
		numSegments = 4;
		// point in the middle between top and start
		t2 = ( time - t ) * 0.5f;
		points[1].ToVec2() = start.ToVec2() + ( end.ToVec2() - start.ToVec2() ) * ( t2 / time );
		points[1].z = start.z + t2 * zVel + 0.5f * gravity * t2 * t2;
		// top of parabolic
		t2 = time - t;
		points[2].ToVec2() = start.ToVec2() + ( end.ToVec2() - start.ToVec2() ) * ( t2 / time );
		points[2].z = start.z + t2 * zVel + 0.5f * gravity * t2 * t2;
		// point in the middel between top and end
		t2 = time - t * 0.5f;
		points[3].ToVec2() = start.ToVec2() + ( end.ToVec2() - start.ToVec2() ) * ( t2 / time );
		points[3].z = start.z + t2 * zVel + 0.5f * gravity * t2 * t2;
	}
	else
	{
		numSegments = 2;
		// point halfway through
		t2 = time * 0.5f;
		points[1].ToVec2() = start.ToVec2() + ( end.ToVec2() - start.ToVec2() ) * 0.5f;
		points[1].z = start.z + t2 * zVel + 0.5f * gravity * t2 * t2;
	}

	// end of parabolic
	points[numSegments] = end;

	if( drawtime )
	{
		for( i = 0; i < numSegments; i++ )
		{
			gameRenderWorld->DebugLine( colorRed, points[i], points[i + 1], drawtime );
		}
	}

	// make sure projectile doesn't go higher than we want it to go
	for( i = 0; i < numSegments; i++ )
	{
		if( points[i].z > max_height )
		{
			// goes higher than we want to allow
			return false;
		}
	}

	result = true;
	for( i = 0; i < numSegments; i++ )
	{
		gameLocal.clip.Translation( trace, points[i], points[i + 1], clip, mat3_identity, clipmask, ignore );
		if( trace.fraction < 1.0f )
		{
			if( gameLocal.GetTraceEntity( trace ) == targetEntity )
			{
				result = true;
			}
			else
			{
				result = false;
			}
			break;
		}
	}

	if( drawtime )
	{
		if( clip )
		{
			gameRenderWorld->DebugBounds( result ? colorGreen : colorYellow, clip->GetBounds().Expand( 1.0f ), trace.endpos, drawtime );
		}
		else
		{
			idBounds bnds( trace.endpos );
			bnds.ExpandSelf( 1.0f );
			gameRenderWorld->DebugBounds( result ? colorGreen : colorYellow, bnds, vec3_zero, drawtime );
		}
	}

	return result;
}

/*
=====================
idAI::PredictTrajectory

  returns true if there is a collision free trajectory for the clip model
  aimDir is set to the ideal aim direction in order to hit the target
=====================
*/
bool idAI::PredictTrajectory( const idVec3& firePos, const idVec3& target, float projectileSpeed, const idVec3& projGravity, const idClipModel* clip, int clipmask, float max_height, const idEntity* ignore, const idEntity* targetEntity, int drawtime, idVec3& aimDir )
{
	int n, i, j;
	float zVel, a, t, pitch, s, c;
	trace_t trace;
	ballistics_t ballistics[2];
	idVec3 dir[2];
	idVec3 velocity;
	idVec3 lastPos, pos;

	if( targetEntity == NULL )
	{
		return false;
	}

	// check if the projectile starts inside the target
	if( targetEntity->GetPhysics()->GetAbsBounds().IntersectsBounds( clip->GetBounds().Translate( firePos ) ) )
	{
		aimDir = target - firePos;
		aimDir.Normalize();
		return true;
	}

	// if no velocity or the projectile is not affected by gravity
	if( projectileSpeed <= 0.0f || projGravity == vec3_origin )
	{

		aimDir = target - firePos;
		aimDir.Normalize();

		gameLocal.clip.Translation( trace, firePos, target, clip, mat3_identity, clipmask, ignore );

		if( drawtime )
		{
			gameRenderWorld->DebugLine( colorRed, firePos, target, drawtime );
			idBounds bnds( trace.endpos );
			bnds.ExpandSelf( 1.0f );
			gameRenderWorld->DebugBounds( ( trace.fraction >= 1.0f || ( gameLocal.GetTraceEntity( trace ) == targetEntity ) ) ? colorGreen : colorYellow, bnds, vec3_zero, drawtime );
		}

		return ( trace.fraction >= 1.0f || ( gameLocal.GetTraceEntity( trace ) == targetEntity ) );
	}

	n = Ballistics( firePos, target, projectileSpeed, projGravity[2], ballistics );
	if( n == 0 )
	{
		// there is no valid trajectory
		aimDir = target - firePos;
		aimDir.Normalize();
		return false;
	}

	// make sure the first angle is the smallest
	if( n == 2 )
	{
		if( ballistics[1].angle < ballistics[0].angle )
		{
			a = ballistics[0].angle;
			ballistics[0].angle = ballistics[1].angle;
			ballistics[1].angle = a;
			t = ballistics[0].time;
			ballistics[0].time = ballistics[1].time;
			ballistics[1].time = t;
		}
	}

	// test if there is a collision free trajectory
	for( i = 0; i < n; i++ )
	{
		pitch = DEG2RAD( ballistics[i].angle );
		idMath::SinCos( pitch, s, c );
		dir[i] = target - firePos;
		dir[i].z = 0.0f;
		dir[i] *= c * idMath::InvSqrt( dir[i].LengthSqr() );
		dir[i].z = s;

		zVel = projectileSpeed * dir[i].z;

		if( ai_debugTrajectory.GetBool() )
		{
			t = ballistics[i].time / 100.0f;
			velocity = dir[i] * projectileSpeed;
			lastPos = firePos;
			pos = firePos;
			for( j = 1; j < 100; j++ )
			{
				pos += velocity * t;
				velocity += projGravity * t;
				gameRenderWorld->DebugLine( colorCyan, lastPos, pos );
				lastPos = pos;
			}
		}

		if( TestTrajectory( firePos, target, zVel, projGravity[2], ballistics[i].time, firePos.z + max_height, clip, clipmask, ignore, targetEntity, drawtime ) )
		{
			aimDir = dir[i];
			return true;
		}
	}

	aimDir = dir[0];

	// there is no collision free trajectory
	return false;
}
