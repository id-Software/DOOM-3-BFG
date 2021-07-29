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


#include "AAS_local.h"
#include "../Game_local.h"		// for print and error

#define CACHETYPE_AREA				1
#define CACHETYPE_PORTAL			2

#define MAX_ROUTING_CACHE_MEMORY	(2*1024*1024)

#define LEDGE_TRAVELTIME_PANALTY	250

/*
============
idRoutingCache::idRoutingCache
============
*/
idRoutingCache::idRoutingCache( int size )
{
	areaNum = 0;
	cluster = 0;
	next = prev = NULL;
	time_next = time_prev = NULL;
	travelFlags = 0;
	startTravelTime = 0;
	type = 0;
	this->size = size;
	reachabilities = new( TAG_AAS ) byte[size];
	memset( reachabilities, 0, size * sizeof( reachabilities[0] ) );
	travelTimes = new( TAG_AAS ) unsigned short[size];
	memset( travelTimes, 0, size * sizeof( travelTimes[0] ) );
}

/*
============
idRoutingCache::~idRoutingCache
============
*/
idRoutingCache::~idRoutingCache()
{
	delete [] reachabilities;
	delete [] travelTimes;
}

/*
============
idRoutingCache::Size
============
*/
int idRoutingCache::Size() const
{
	return sizeof( idRoutingCache ) + size * sizeof( reachabilities[0] ) + size * sizeof( travelTimes[0] );
}

/*
============
idAASLocal::AreaTravelTime
============
*/
unsigned short idAASLocal::AreaTravelTime( int areaNum, const idVec3& start, const idVec3& end ) const
{
	float dist;

	dist = ( end - start ).Length();

	if( file->GetArea( areaNum ).travelFlags & TFL_CROUCH )
	{
		dist *= 100.0f / 100.0f;
	}
	else if( file->GetArea( areaNum ).travelFlags & TFL_WATER )
	{
		dist *= 100.0f / 150.0f;
	}
	else
	{
		dist *= 100.0f / 300.0f;
	}
	if( dist < 1.0f )
	{
		return 1;
	}
	return ( unsigned short ) idMath::Ftoi( dist );
}

/*
============
idAASLocal::CalculateAreaTravelTimes
============
*/
void idAASLocal::CalculateAreaTravelTimes()
{
	int n, i, j, numReach, numRevReach, t, maxt;
	byte* bytePtr;
	idReachability* reach, *rev_reach;

	// get total memory for all area travel times
	numAreaTravelTimes = 0;
	for( n = 0; n < file->GetNumAreas(); n++ )
	{

		if( !( file->GetArea( n ).flags & ( AREA_REACHABLE_WALK | AREA_REACHABLE_FLY ) ) )
		{
			continue;
		}

		numReach = 0;
		for( reach = file->GetArea( n ).reach; reach; reach = reach->next )
		{
			numReach++;
		}

		numRevReach = 0;
		for( rev_reach = file->GetArea( n ).rev_reach; rev_reach; rev_reach = rev_reach->rev_next )
		{
			numRevReach++;
		}
		numAreaTravelTimes += numReach * numRevReach;
	}

	areaTravelTimes = ( unsigned short* ) Mem_Alloc( numAreaTravelTimes * sizeof( unsigned short ), TAG_AAS );
	bytePtr = ( byte* ) areaTravelTimes;

	for( n = 0; n < file->GetNumAreas(); n++ )
	{

		if( !( file->GetArea( n ).flags & ( AREA_REACHABLE_WALK | AREA_REACHABLE_FLY ) ) )
		{
			continue;
		}

		// for each reachability that starts in this area calculate the travel time
		// towards all the reachabilities that lead towards this area
		for( maxt = i = 0, reach = file->GetArea( n ).reach; reach; reach = reach->next, i++ )
		{
			assert( i < MAX_REACH_PER_AREA );
			if( i >= MAX_REACH_PER_AREA )
			{
				gameLocal.Error( "i >= MAX_REACH_PER_AREA" );
			}
			reach->number = i;
			reach->disableCount = 0;
			reach->areaTravelTimes = ( unsigned short* ) bytePtr;
			for( j = 0, rev_reach = file->GetArea( n ).rev_reach; rev_reach; rev_reach = rev_reach->rev_next, j++ )
			{
				t = AreaTravelTime( n, reach->start, rev_reach->end );
				reach->areaTravelTimes[j] = t;
				if( t > maxt )
				{
					maxt = t;
				}
			}
			bytePtr += j * sizeof( unsigned short );
		}

		// if this area is a portal
		if( file->GetArea( n ).cluster < 0 )
		{
			// set the maximum travel time through this portal
			file->SetPortalMaxTravelTime( -file->GetArea( n ).cluster, maxt );
		}
	}

	// RB: 64 bit fixes, changed unsigned int to ptrdiff_t
	assert( ( ( ptrdiff_t ) bytePtr - ( ptrdiff_t ) areaTravelTimes ) <= numAreaTravelTimes * sizeof( unsigned short ) );
	// RB end

}

/*
============
idAASLocal::DeleteAreaTravelTimes
============
*/
void idAASLocal::DeleteAreaTravelTimes()
{
	Mem_Free( areaTravelTimes );
	areaTravelTimes = NULL;
	numAreaTravelTimes = 0;
}

/*
============
idAASLocal::SetupRoutingCache
============
*/
void idAASLocal::SetupRoutingCache()
{
	int i;
	byte* bytePtr;

	areaCacheIndexSize = 0;
	for( i = 0; i < file->GetNumClusters(); i++ )
	{
		areaCacheIndexSize += file->GetCluster( i ).numReachableAreas;
	}
	areaCacheIndex = ( idRoutingCache*** ) Mem_ClearedAlloc( file->GetNumClusters() * sizeof( idRoutingCache** ) +
					 areaCacheIndexSize * sizeof( idRoutingCache* ), TAG_AAS );
	bytePtr = ( ( byte* )areaCacheIndex ) + file->GetNumClusters() * sizeof( idRoutingCache** );
	for( i = 0; i < file->GetNumClusters(); i++ )
	{
		areaCacheIndex[i] = ( idRoutingCache** ) bytePtr;
		bytePtr += file->GetCluster( i ).numReachableAreas * sizeof( idRoutingCache* );
	}

	portalCacheIndexSize = file->GetNumAreas();
	portalCacheIndex = ( idRoutingCache** ) Mem_ClearedAlloc( portalCacheIndexSize * sizeof( idRoutingCache* ), TAG_AAS );

	areaUpdate = ( idRoutingUpdate* ) Mem_ClearedAlloc( file->GetNumAreas() * sizeof( idRoutingUpdate ), TAG_AAS );
	portalUpdate = ( idRoutingUpdate* ) Mem_ClearedAlloc( ( file->GetNumPortals() + 1 ) * sizeof( idRoutingUpdate ), TAG_AAS );

	goalAreaTravelTimes = ( unsigned short* ) Mem_ClearedAlloc( file->GetNumAreas() * sizeof( unsigned short ), TAG_AAS );

	cacheListStart = cacheListEnd = NULL;
	totalCacheMemory = 0;
}

/*
============
idAASLocal::DeleteClusterCache
============
*/
void idAASLocal::DeleteClusterCache( int clusterNum )
{
	int i;
	idRoutingCache* cache;

	for( i = 0; i < file->GetCluster( clusterNum ).numReachableAreas; i++ )
	{
		for( cache = areaCacheIndex[clusterNum][i]; cache; cache = areaCacheIndex[clusterNum][i] )
		{
			areaCacheIndex[clusterNum][i] = cache->next;
			UnlinkCache( cache );
			delete cache;
		}
	}
}

/*
============
idAASLocal::DeletePortalCache
============
*/
void idAASLocal::DeletePortalCache()
{
	int i;
	idRoutingCache* cache;

	for( i = 0; i < file->GetNumAreas(); i++ )
	{
		for( cache = portalCacheIndex[i]; cache; cache = portalCacheIndex[i] )
		{
			portalCacheIndex[i] = cache->next;
			UnlinkCache( cache );
			delete cache;
		}
	}
}

/*
============
idAASLocal::ShutdownRoutingCache
============
*/
void idAASLocal::ShutdownRoutingCache()
{
	int i;

	for( i = 0; i < file->GetNumClusters(); i++ )
	{
		DeleteClusterCache( i );
	}

	DeletePortalCache();

	Mem_Free( areaCacheIndex );
	areaCacheIndex = NULL;
	areaCacheIndexSize = 0;
	Mem_Free( portalCacheIndex );
	portalCacheIndex = NULL;
	portalCacheIndexSize = 0;
	Mem_Free( areaUpdate );
	areaUpdate = NULL;
	Mem_Free( portalUpdate );
	portalUpdate = NULL;
	Mem_Free( goalAreaTravelTimes );
	goalAreaTravelTimes = NULL;

	cacheListStart = cacheListEnd = NULL;
	totalCacheMemory = 0;
}

/*
============
idAASLocal::SetupRouting
============
*/
bool idAASLocal::SetupRouting()
{
	CalculateAreaTravelTimes();
	SetupRoutingCache();
	return true;
}

/*
============
idAASLocal::ShutdownRouting
============
*/
void idAASLocal::ShutdownRouting()
{
	DeleteAreaTravelTimes();
	ShutdownRoutingCache();
}

/*
============
idAASLocal::RoutingStats
============
*/
void idAASLocal::RoutingStats() const
{
	idRoutingCache* cache;
	int numAreaCache, numPortalCache;
	int totalAreaCacheMemory, totalPortalCacheMemory;

	numAreaCache = numPortalCache = 0;
	totalAreaCacheMemory = totalPortalCacheMemory = 0;
	for( cache = cacheListStart; cache; cache = cache->time_next )
	{
		if( cache->type == CACHETYPE_AREA )
		{
			numAreaCache++;
			totalAreaCacheMemory += sizeof( idRoutingCache ) + cache->size * ( sizeof( unsigned short ) + sizeof( byte ) );
		}
		else
		{
			numPortalCache++;
			totalPortalCacheMemory += sizeof( idRoutingCache ) + cache->size * ( sizeof( unsigned short ) + sizeof( byte ) );
		}
	}

	gameLocal.Printf( "%6d area cache (%d KB)\n", numAreaCache, totalAreaCacheMemory >> 10 );
	gameLocal.Printf( "%6d portal cache (%d KB)\n", numPortalCache, totalPortalCacheMemory >> 10 );
	gameLocal.Printf( "%6d total cache (%d KB)\n", numAreaCache + numPortalCache, totalCacheMemory >> 10 );
	gameLocal.Printf( "%6d area travel times (%d KB)\n", numAreaTravelTimes, ( numAreaTravelTimes * sizeof( unsigned short ) ) >> 10 );
	gameLocal.Printf( "%6d area cache entries (%d KB)\n", areaCacheIndexSize, ( areaCacheIndexSize * sizeof( idRoutingCache* ) ) >> 10 );
	gameLocal.Printf( "%6d portal cache entries (%d KB)\n", portalCacheIndexSize, ( portalCacheIndexSize * sizeof( idRoutingCache* ) ) >> 10 );
}

/*
============
idAASLocal::RemoveRoutingCacheUsingArea
============
*/
void idAASLocal::RemoveRoutingCacheUsingArea( int areaNum )
{
	int clusterNum;

	clusterNum = file->GetArea( areaNum ).cluster;
	if( clusterNum > 0 )
	{
		// remove all the cache in the cluster the area is in
		DeleteClusterCache( clusterNum );
	}
	else
	{
		// if this is a portal remove all cache in both the front and back cluster
		DeleteClusterCache( file->GetPortal( -clusterNum ).clusters[0] );
		DeleteClusterCache( file->GetPortal( -clusterNum ).clusters[1] );
	}
	DeletePortalCache();
}

/*
============
idAASLocal::DisableArea
============
*/
void idAASLocal::DisableArea( int areaNum )
{
	assert( areaNum > 0 && areaNum < file->GetNumAreas() );

	if( file->GetArea( areaNum ).travelFlags & TFL_INVALID )
	{
		return;
	}

	file->SetAreaTravelFlag( areaNum, TFL_INVALID );

	RemoveRoutingCacheUsingArea( areaNum );
}

/*
============
idAASLocal::EnableArea
============
*/
void idAASLocal::EnableArea( int areaNum )
{
	assert( areaNum > 0 && areaNum < file->GetNumAreas() );

	if( !( file->GetArea( areaNum ).travelFlags & TFL_INVALID ) )
	{
		return;
	}

	file->RemoveAreaTravelFlag( areaNum, TFL_INVALID );

	RemoveRoutingCacheUsingArea( areaNum );
}

/*
============
idAASLocal::SetAreaState_r
============
*/
bool idAASLocal::SetAreaState_r( int nodeNum, const idBounds& bounds, const int areaContents, bool disabled )
{
	int res;
	const aasNode_t* node;
	bool foundClusterPortal = false;

	while( nodeNum != 0 )
	{
		if( nodeNum < 0 )
		{
			// if this area is a cluster portal
			if( file->GetArea( -nodeNum ).contents & areaContents )
			{
				if( disabled )
				{
					DisableArea( -nodeNum );
				}
				else
				{
					EnableArea( -nodeNum );
				}
				foundClusterPortal |= true;
			}
			break;
		}
		node = &file->GetNode( nodeNum );
		res = bounds.PlaneSide( file->GetPlane( node->planeNum ) );
		if( res == PLANESIDE_BACK )
		{
			nodeNum = node->children[1];
		}
		else if( res == PLANESIDE_FRONT )
		{
			nodeNum = node->children[0];
		}
		else
		{
			foundClusterPortal |= SetAreaState_r( node->children[1], bounds, areaContents, disabled );
			nodeNum = node->children[0];
		}
	}

	return foundClusterPortal;
}

/*
============
idAASLocal::SetAreaState
============
*/
bool idAASLocal::SetAreaState( const idBounds& bounds, const int areaContents, bool disabled )
{
	idBounds expBounds;

	if( !file )
	{
		return false;
	}

	expBounds[0] = bounds[0] - file->GetSettings().boundingBoxes[0][1];
	expBounds[1] = bounds[1] - file->GetSettings().boundingBoxes[0][0];

	// find all areas within or touching the bounds with the given contents and disable/enable them for routing
	return SetAreaState_r( 1, expBounds, areaContents, disabled );
}

/*
============
idAASLocal::GetBoundsAreas_r
============
*/
void idAASLocal::GetBoundsAreas_r( int nodeNum, const idBounds& bounds, idList<int>& areas ) const
{
	int res;
	const aasNode_t* node;

	while( nodeNum != 0 )
	{
		if( nodeNum < 0 )
		{
			areas.Append( -nodeNum );
			break;
		}
		node = &file->GetNode( nodeNum );
		res = bounds.PlaneSide( file->GetPlane( node->planeNum ) );
		if( res == PLANESIDE_BACK )
		{
			nodeNum = node->children[1];
		}
		else if( res == PLANESIDE_FRONT )
		{
			nodeNum = node->children[0];
		}
		else
		{
			GetBoundsAreas_r( node->children[1], bounds, areas );
			nodeNum = node->children[0];
		}
	}
}

/*
============
idAASLocal::SetObstacleState
============
*/
void idAASLocal::SetObstacleState( const idRoutingObstacle* obstacle, bool enable )
{
	int i;
	const aasArea_t* area;
	idReachability* reach, *rev_reach;
	bool inside;

	for( i = 0; i < obstacle->areas.Num(); i++ )
	{

		RemoveRoutingCacheUsingArea( obstacle->areas[i] );

		area = &file->GetArea( obstacle->areas[i] );

		for( rev_reach = area->rev_reach; rev_reach; rev_reach = rev_reach->rev_next )
		{

			if( rev_reach->travelType & TFL_INVALID )
			{
				continue;
			}

			inside = false;

			if( obstacle->bounds.ContainsPoint( rev_reach->end ) )
			{
				inside = true;
			}
			else
			{
				for( reach = area->reach; reach; reach = reach->next )
				{
					if( obstacle->bounds.LineIntersection( rev_reach->end, reach->start ) )
					{
						inside = true;
						break;
					}
				}
			}

			if( inside )
			{
				if( enable )
				{
					rev_reach->disableCount--;
					if( rev_reach->disableCount <= 0 )
					{
						rev_reach->travelType &= ~TFL_INVALID;
						rev_reach->disableCount = 0;
					}
				}
				else
				{
					rev_reach->travelType |= TFL_INVALID;
					rev_reach->disableCount++;
				}
			}
		}
	}
}

/*
============
idAASLocal::AddObstacle
============
*/
aasHandle_t idAASLocal::AddObstacle( const idBounds& bounds )
{
	idRoutingObstacle* obstacle;

	if( !file )
	{
		return -1;
	}

	obstacle = new( TAG_AAS ) idRoutingObstacle;
	obstacle->bounds[0] = bounds[0] - file->GetSettings().boundingBoxes[0][1];
	obstacle->bounds[1] = bounds[1] - file->GetSettings().boundingBoxes[0][0];
	GetBoundsAreas_r( 1, obstacle->bounds, obstacle->areas );
	SetObstacleState( obstacle, true );

	obstacleList.Append( obstacle );
	return obstacleList.Num() - 1;
}

/*
============
idAASLocal::RemoveObstacle
============
*/
void idAASLocal::RemoveObstacle( const aasHandle_t handle )
{
	if( !file )
	{
		return;
	}
	if( ( handle >= 0 ) && ( handle < obstacleList.Num() ) )
	{
		SetObstacleState( obstacleList[handle], false );

		delete obstacleList[handle];
		obstacleList.RemoveIndex( handle );
	}
}

/*
============
idAASLocal::RemoveAllObstacles
============
*/
void idAASLocal::RemoveAllObstacles()
{
	int i;

	if( !file )
	{
		return;
	}

	for( i = 0; i < obstacleList.Num(); i++ )
	{
		SetObstacleState( obstacleList[i], false );
		delete obstacleList[i];
	}
	obstacleList.Clear();
}

/*
============
idAASLocal::LinkCache

  link the cache in the cache list sorted from oldest to newest cache
============
*/
void idAASLocal::LinkCache( idRoutingCache* cache ) const
{

	// if the cache is already linked
	if( cache->time_next || cache->time_prev || cacheListStart == cache )
	{
		UnlinkCache( cache );
	}

	totalCacheMemory += cache->Size();

	// add cache to the end of the list
	cache->time_next = NULL;
	cache->time_prev = cacheListEnd;
	if( cacheListEnd )
	{
		cacheListEnd->time_next = cache;
	}
	cacheListEnd = cache;
	if( !cacheListStart )
	{
		cacheListStart = cache;
	}
}

/*
============
idAASLocal::UnlinkCache
============
*/
void idAASLocal::UnlinkCache( idRoutingCache* cache ) const
{

	totalCacheMemory -= cache->Size();

	// unlink the cache
	if( cache->time_next )
	{
		cache->time_next->time_prev = cache->time_prev;
	}
	else
	{
		cacheListEnd = cache->time_prev;
	}
	if( cache->time_prev )
	{
		cache->time_prev->time_next = cache->time_next;
	}
	else
	{
		cacheListStart = cache->time_next;
	}
	cache->time_next = cache->time_prev = NULL;
}

/*
============
idAASLocal::DeleteOldestCache
============
*/
void idAASLocal::DeleteOldestCache() const
{
	idRoutingCache* cache;

	assert( cacheListStart );

	// unlink the oldest cache
	cache = cacheListStart;
	UnlinkCache( cache );

	// unlink the oldest cache from the area or portal cache index
	if( cache->next )
	{
		cache->next->prev = cache->prev;
	}
	if( cache->prev )
	{
		cache->prev->next = cache->next;
	}
	else if( cache->type == CACHETYPE_AREA )
	{
		areaCacheIndex[cache->cluster][ClusterAreaNum( cache->cluster, cache->areaNum )] = cache->next;
	}
	else if( cache->type == CACHETYPE_PORTAL )
	{
		portalCacheIndex[cache->areaNum] = cache->next;
	}

	delete cache;
}

/*
============
idAASLocal::GetAreaReachability
============
*/
idReachability* idAASLocal::GetAreaReachability( int areaNum, int reachabilityNum ) const
{
	idReachability* reach;

	for( reach = file->GetArea( areaNum ).reach; reach; reach = reach->next )
	{
		if( --reachabilityNum < 0 )
		{
			return reach;
		}
	}
	return NULL;
}

/*
============
idAASLocal::ClusterAreaNum
============
*/
ID_INLINE int idAASLocal::ClusterAreaNum( int clusterNum, int areaNum ) const
{
	int side, areaCluster;

	areaCluster = file->GetArea( areaNum ).cluster;
	if( areaCluster > 0 )
	{
		return file->GetArea( areaNum ).clusterAreaNum;
	}
	else
	{
		side = file->GetPortal( -areaCluster ).clusters[0] != clusterNum;
		return file->GetPortal( -areaCluster ).clusterAreaNum[side];
	}
}

/*
============
idAASLocal::UpdateAreaRoutingCache
============
*/
void idAASLocal::UpdateAreaRoutingCache( idRoutingCache* areaCache ) const
{
	int i, nextAreaNum, cluster, badTravelFlags, clusterAreaNum, numReachableAreas;
	unsigned short t, startAreaTravelTimes[MAX_REACH_PER_AREA];
	idRoutingUpdate* updateListStart, *updateListEnd, *curUpdate, *nextUpdate;
	idReachability* reach;
	const aasArea_t* nextArea;

	// number of reachability areas within this cluster
	numReachableAreas = file->GetCluster( areaCache->cluster ).numReachableAreas;

	// number of the start area within the cluster
	clusterAreaNum = ClusterAreaNum( areaCache->cluster, areaCache->areaNum );
	if( clusterAreaNum >= numReachableAreas )
	{
		return;
	}

	areaCache->travelTimes[clusterAreaNum] = areaCache->startTravelTime;
	badTravelFlags = ~areaCache->travelFlags;
	memset( startAreaTravelTimes, 0, sizeof( startAreaTravelTimes ) );

	// initialize first update
	curUpdate = &areaUpdate[clusterAreaNum];
	curUpdate->areaNum = areaCache->areaNum;
	curUpdate->areaTravelTimes = startAreaTravelTimes;
	curUpdate->tmpTravelTime = areaCache->startTravelTime;
	curUpdate->next = NULL;
	curUpdate->prev = NULL;
	updateListStart = curUpdate;
	updateListEnd = curUpdate;

	// while there are updates in the list
	while( updateListStart )
	{

		curUpdate = updateListStart;
		if( curUpdate->next )
		{
			curUpdate->next->prev = NULL;
		}
		else
		{
			updateListEnd = NULL;
		}
		updateListStart = curUpdate->next;

		curUpdate->isInList = false;

		for( i = 0, reach = file->GetArea( curUpdate->areaNum ).rev_reach; reach; reach = reach->rev_next, i++ )
		{

			// if the reachability uses an undesired travel type
			if( reach->travelType & badTravelFlags )
			{
				continue;
			}

			// next area the reversed reachability leads to
			nextAreaNum = reach->fromAreaNum;
			nextArea = &file->GetArea( nextAreaNum );

			// if traveling through the next area requires an undesired travel flag
			if( nextArea->travelFlags & badTravelFlags )
			{
				continue;
			}

			// get the cluster number of the area
			cluster = nextArea->cluster;
			// don't leave the cluster, however do flood into cluster portals
			if( cluster > 0 && cluster != areaCache->cluster )
			{
				continue;
			}

			// get the number of the area in the cluster
			clusterAreaNum = ClusterAreaNum( areaCache->cluster, nextAreaNum );
			if( clusterAreaNum >= numReachableAreas )
			{
				continue;	// should never happen
			}

			assert( clusterAreaNum < areaCache->size );

			// time already travelled plus the traveltime through the current area
			// plus the travel time of the reachability towards the next area
			t = curUpdate->tmpTravelTime + curUpdate->areaTravelTimes[i] + reach->travelTime;

			if( !areaCache->travelTimes[clusterAreaNum] || t < areaCache->travelTimes[clusterAreaNum] )
			{

				areaCache->travelTimes[clusterAreaNum] = t;
				areaCache->reachabilities[clusterAreaNum] = reach->number; // reversed reachability used to get into this area
				nextUpdate = &areaUpdate[clusterAreaNum];
				nextUpdate->areaNum = nextAreaNum;
				nextUpdate->tmpTravelTime = t;
				nextUpdate->areaTravelTimes = reach->areaTravelTimes;

				// if we are not allowed to fly
				if( badTravelFlags & TFL_FLY )
				{
					// avoid areas near ledges
					if( file->GetArea( nextAreaNum ).flags & AREA_LEDGE )
					{
						nextUpdate->tmpTravelTime += LEDGE_TRAVELTIME_PANALTY;
					}
				}

				if( !nextUpdate->isInList )
				{
					nextUpdate->next = NULL;
					nextUpdate->prev = updateListEnd;
					if( updateListEnd )
					{
						updateListEnd->next = nextUpdate;
					}
					else
					{
						updateListStart = nextUpdate;
					}
					updateListEnd = nextUpdate;
					nextUpdate->isInList = true;
				}
			}
		}
	}
}

/*
============
idAASLocal::GetAreaRoutingCache
============
*/
idRoutingCache* idAASLocal::GetAreaRoutingCache( int clusterNum, int areaNum, int travelFlags ) const
{
	int clusterAreaNum;
	idRoutingCache* cache, *clusterCache;

	// number of the area in the cluster
	clusterAreaNum = ClusterAreaNum( clusterNum, areaNum );
	// pointer to the cache for the area in the cluster
	clusterCache = areaCacheIndex[clusterNum][clusterAreaNum];
	// check if cache without undesired travel flags already exists
	for( cache = clusterCache; cache; cache = cache->next )
	{
		if( cache->travelFlags == travelFlags )
		{
			break;
		}
	}
	// if no cache found
	if( !cache )
	{
		cache = new( TAG_AAS ) idRoutingCache( file->GetCluster( clusterNum ).numReachableAreas );
		cache->type = CACHETYPE_AREA;
		cache->cluster = clusterNum;
		cache->areaNum = areaNum;
		cache->startTravelTime = 1;
		cache->travelFlags = travelFlags;
		cache->prev = NULL;
		cache->next = clusterCache;
		if( clusterCache )
		{
			clusterCache->prev = cache;
		}
		areaCacheIndex[clusterNum][clusterAreaNum] = cache;
		UpdateAreaRoutingCache( cache );
	}
	LinkCache( cache );
	return cache;
}

/*
============
idAASLocal::UpdatePortalRoutingCache
============
*/
void idAASLocal::UpdatePortalRoutingCache( idRoutingCache* portalCache ) const
{
	int i, portalNum, clusterAreaNum;
	unsigned short t;
	const aasPortal_t* portal;
	const aasCluster_t* cluster;
	idRoutingCache* cache;
	idRoutingUpdate* updateListStart, *updateListEnd, *curUpdate, *nextUpdate;

	curUpdate = &portalUpdate[ file->GetNumPortals() ];
	curUpdate->cluster = portalCache->cluster;
	curUpdate->areaNum = portalCache->areaNum;
	curUpdate->tmpTravelTime = portalCache->startTravelTime;

	//put the area to start with in the current read list
	curUpdate->next = NULL;
	curUpdate->prev = NULL;
	updateListStart = curUpdate;
	updateListEnd = curUpdate;

	// while there are updates in the current list
	while( updateListStart )
	{

		curUpdate = updateListStart;
		// remove the current update from the list
		if( curUpdate->next )
		{
			curUpdate->next->prev = NULL;
		}
		else
		{
			updateListEnd = NULL;
		}
		updateListStart = curUpdate->next;
		// current update is removed from the list
		curUpdate->isInList = false;

		cluster = &file->GetCluster( curUpdate->cluster );
		cache = GetAreaRoutingCache( curUpdate->cluster, curUpdate->areaNum, portalCache->travelFlags );

		// take all portals of the cluster
		for( i = 0; i < cluster->numPortals; i++ )
		{
			portalNum = file->GetPortalIndex( cluster->firstPortal + i );
			assert( portalNum < portalCache->size );
			portal = &file->GetPortal( portalNum );

			clusterAreaNum = ClusterAreaNum( curUpdate->cluster, portal->areaNum );
			if( clusterAreaNum >= cluster->numReachableAreas )
			{
				continue;
			}

			t = cache->travelTimes[clusterAreaNum];
			if( t == 0 )
			{
				continue;
			}
			t += curUpdate->tmpTravelTime;

			if( !portalCache->travelTimes[portalNum] || t < portalCache->travelTimes[portalNum] )
			{

				portalCache->travelTimes[portalNum] = t;
				portalCache->reachabilities[portalNum] = cache->reachabilities[clusterAreaNum];
				nextUpdate = &portalUpdate[portalNum];
				if( portal->clusters[0] == curUpdate->cluster )
				{
					nextUpdate->cluster = portal->clusters[1];
				}
				else
				{
					nextUpdate->cluster = portal->clusters[0];
				}
				nextUpdate->areaNum = portal->areaNum;
				// add travel time through the actual portal area for the next update
				nextUpdate->tmpTravelTime = t + portal->maxAreaTravelTime;

				if( !nextUpdate->isInList )
				{

					nextUpdate->next = NULL;
					nextUpdate->prev = updateListEnd;
					if( updateListEnd )
					{
						updateListEnd->next = nextUpdate;
					}
					else
					{
						updateListStart = nextUpdate;
					}
					updateListEnd = nextUpdate;
					nextUpdate->isInList = true;
				}
			}
		}
	}
}

/*
============
idAASLocal::GetPortalRoutingCache
============
*/
idRoutingCache* idAASLocal::GetPortalRoutingCache( int clusterNum, int areaNum, int travelFlags ) const
{
	idRoutingCache* cache;

	// check if cache without undesired travel flags already exists
	for( cache = portalCacheIndex[areaNum]; cache; cache = cache->next )
	{
		if( cache->travelFlags == travelFlags )
		{
			break;
		}
	}
	// if no cache found
	if( !cache )
	{
		cache = new( TAG_AAS ) idRoutingCache( file->GetNumPortals() );
		cache->type = CACHETYPE_PORTAL;
		cache->cluster = clusterNum;
		cache->areaNum = areaNum;
		cache->startTravelTime = 1;
		cache->travelFlags = travelFlags;
		cache->prev = NULL;
		cache->next = portalCacheIndex[areaNum];
		if( portalCacheIndex[areaNum] )
		{
			portalCacheIndex[areaNum]->prev = cache;
		}
		portalCacheIndex[areaNum] = cache;
		UpdatePortalRoutingCache( cache );
	}
	LinkCache( cache );
	return cache;
}

/*
============
idAASLocal::RouteToGoalArea
============
*/
bool idAASLocal::RouteToGoalArea( int areaNum, const idVec3 origin, int goalAreaNum, int travelFlags, int& travelTime, idReachability** reach ) const
{
	int clusterNum, goalClusterNum, portalNum, i, clusterAreaNum;
	unsigned short int t, bestTime;
	const aasPortal_t* portal;
	const aasCluster_t* cluster;
	idRoutingCache* areaCache, *portalCache, *clusterCache;
	idReachability* bestReach, *r, *nextr;

	travelTime = 0;
	*reach = NULL;

	if( !file )
	{
		return false;
	}

	if( areaNum == goalAreaNum )
	{
		return true;
	}

	if( areaNum <= 0 || areaNum >= file->GetNumAreas() )
	{
		gameLocal.Printf( "RouteToGoalArea: areaNum %d out of range\n", areaNum );
		return false;
	}
	if( goalAreaNum <= 0 || goalAreaNum >= file->GetNumAreas() )
	{
		gameLocal.Printf( "RouteToGoalArea: goalAreaNum %d out of range\n", goalAreaNum );
		return false;
	}

	while( totalCacheMemory > MAX_ROUTING_CACHE_MEMORY )
	{
		DeleteOldestCache();
	}

	clusterNum = file->GetArea( areaNum ).cluster;
	goalClusterNum = file->GetArea( goalAreaNum ).cluster;

	// if the source area is a cluster portal, read directly from the portal cache
	if( clusterNum < 0 )
	{
		// if the goal area is a portal
		if( goalClusterNum < 0 )
		{
			// just assume the goal area is part of the front cluster
			portal = &file->GetPortal( -goalClusterNum );
			goalClusterNum = portal->clusters[0];
		}
		// get the portal routing cache
		portalCache = GetPortalRoutingCache( goalClusterNum, goalAreaNum, travelFlags );
		*reach = GetAreaReachability( areaNum, portalCache->reachabilities[-clusterNum] );
		travelTime = portalCache->travelTimes[-clusterNum] + AreaTravelTime( areaNum, origin, ( *reach )->start );
		return true;
	}

	bestTime = 0;
	bestReach = NULL;

	// check if the goal area is a portal of the source area cluster
	if( goalClusterNum < 0 )
	{
		portal = &file->GetPortal( -goalClusterNum );
		if( portal->clusters[0] == clusterNum || portal->clusters[1] == clusterNum )
		{
			goalClusterNum = clusterNum;
		}
	}

	// if both areas are in the same cluster
	if( clusterNum > 0 && goalClusterNum > 0 && clusterNum == goalClusterNum )
	{
		clusterCache = GetAreaRoutingCache( clusterNum, goalAreaNum, travelFlags );
		clusterAreaNum = ClusterAreaNum( clusterNum, areaNum );
		if( clusterCache->travelTimes[clusterAreaNum] )
		{
			bestReach = GetAreaReachability( areaNum, clusterCache->reachabilities[clusterAreaNum] );
			bestTime = clusterCache->travelTimes[clusterAreaNum] + AreaTravelTime( areaNum, origin, bestReach->start );
		}
		else
		{
			clusterCache = NULL;
		}
	}
	else
	{
		clusterCache = NULL;
	}

	clusterNum = file->GetArea( areaNum ).cluster;
	goalClusterNum = file->GetArea( goalAreaNum ).cluster;

	// if the goal area is a portal
	if( goalClusterNum < 0 )
	{
		// just assume the goal area is part of the front cluster
		portal = &file->GetPortal( -goalClusterNum );
		goalClusterNum = portal->clusters[0];
	}
	// get the portal routing cache
	portalCache = GetPortalRoutingCache( goalClusterNum, goalAreaNum, travelFlags );

	// the cluster the area is in
	cluster = &file->GetCluster( clusterNum );
	// current area inside the current cluster
	clusterAreaNum = ClusterAreaNum( clusterNum, areaNum );
	// if the area is not a reachable area
	if( clusterAreaNum >= cluster->numReachableAreas )
	{
		return false;
	}

	// find the portal of the source area cluster leading towards the goal area
	for( i = 0; i < cluster->numPortals; i++ )
	{
		portalNum = file->GetPortalIndex( cluster->firstPortal + i );

		// if the goal area isn't reachable from the portal
		if( !portalCache->travelTimes[portalNum] )
		{
			continue;
		}

		portal = &file->GetPortal( portalNum );
		// get the cache of the portal area
		areaCache = GetAreaRoutingCache( clusterNum, portal->areaNum, travelFlags );
		// if the portal is not reachable from this area
		if( !areaCache->travelTimes[clusterAreaNum] )
		{
			continue;
		}

		r = GetAreaReachability( areaNum, areaCache->reachabilities[clusterAreaNum] );

		if( clusterCache )
		{
			// if the next reachability from the portal leads back into the cluster
			nextr = GetAreaReachability( portal->areaNum, portalCache->reachabilities[portalNum] );
			if( file->GetArea( nextr->toAreaNum ).cluster < 0 || file->GetArea( nextr->toAreaNum ).cluster == clusterNum )
			{
				continue;
			}
		}

		// the total travel time is the travel time from the portal area to the goal area
		// plus the travel time from the source area towards the portal area
		t = portalCache->travelTimes[portalNum] + areaCache->travelTimes[clusterAreaNum];
		// NOTE:	Should add the exact travel time through the portal area.
		//			However we add the largest travel time through the portal area.
		//			We cannot directly calculate the exact travel time through the portal area
		//			because the reachability used to travel into the portal area is not known.
		t += portal->maxAreaTravelTime;

		// if the time is better than the one already found
		if( !bestTime || t < bestTime )
		{
			bestReach = r;
			bestTime = t;
		}
	}

	if( !bestReach )
	{
		return false;
	}

	*reach = bestReach;
	travelTime = bestTime;

	return true;
}

/*
============
idAASLocal::TravelTimeToGoalArea
============
*/
int idAASLocal::TravelTimeToGoalArea( int areaNum, const idVec3& origin, int goalAreaNum, int travelFlags ) const
{
	int travelTime;
	idReachability* reach;

	if( !file )
	{
		return 0;
	}

	if( !RouteToGoalArea( areaNum, origin, goalAreaNum, travelFlags, travelTime, &reach ) )
	{
		return 0;
	}
	return travelTime;
}

/*
============
idAASLocal::FindNearestGoal
============
*/
bool idAASLocal::FindNearestGoal( aasGoal_t& goal, int areaNum, const idVec3 origin, const idVec3& target, int travelFlags, aasObstacle_t* obstacles, int numObstacles, idAASCallback& callback ) const
{
	int i, j, k, badTravelFlags, nextAreaNum, bestAreaNum;
	unsigned short t, bestTravelTime;
	idRoutingUpdate* updateListStart, *updateListEnd, *curUpdate, *nextUpdate;
	idReachability* reach;
	const aasArea_t* nextArea;
	idVec3 v1, v2, p;
	float targetDist, dist;

	if( file == NULL || areaNum <= 0 )
	{
		goal.areaNum = areaNum;
		goal.origin = origin;
		return false;
	}

	// if the first area is valid goal, just return the origin
	if( callback.TestArea( this, areaNum ) )
	{
		goal.areaNum = areaNum;
		goal.origin = origin;
		return true;
	}

	// setup obstacles
	for( k = 0; k < numObstacles; k++ )
	{
		obstacles[k].expAbsBounds[0] = obstacles[k].absBounds[0] - file->GetSettings().boundingBoxes[0][1];
		obstacles[k].expAbsBounds[1] = obstacles[k].absBounds[1] - file->GetSettings().boundingBoxes[0][0];
	}

	badTravelFlags = ~travelFlags;
	SIMDProcessor->Memset( goalAreaTravelTimes, 0, file->GetNumAreas() * sizeof( unsigned short ) );

	targetDist = ( target - origin ).Length();

	// initialize first update
	curUpdate = &areaUpdate[areaNum];
	curUpdate->areaNum = areaNum;
	curUpdate->tmpTravelTime = 0;
	curUpdate->start = origin;
	curUpdate->next = NULL;
	curUpdate->prev = NULL;
	updateListStart = curUpdate;
	updateListEnd = curUpdate;

	bestTravelTime = 0;
	bestAreaNum = 0;

	// while there are updates in the list
	while( updateListStart )
	{

		curUpdate = updateListStart;
		if( curUpdate->next )
		{
			curUpdate->next->prev = NULL;
		}
		else
		{
			updateListEnd = NULL;
		}
		updateListStart = curUpdate->next;

		curUpdate->isInList = false;

		// if we already found a closer location
		if( bestTravelTime && curUpdate->tmpTravelTime >= bestTravelTime )
		{
			continue;
		}

		for( i = 0, reach = file->GetArea( curUpdate->areaNum ).reach; reach; reach = reach->next, i++ )
		{

			// if the reachability uses an undesired travel type
			if( reach->travelType & badTravelFlags )
			{
				continue;
			}

			// next area the reversed reachability leads to
			nextAreaNum = reach->toAreaNum;
			nextArea = &file->GetArea( nextAreaNum );

			// if traveling through the next area requires an undesired travel flag
			if( nextArea->travelFlags & badTravelFlags )
			{
				continue;
			}

			t = curUpdate->tmpTravelTime +
				AreaTravelTime( curUpdate->areaNum, curUpdate->start, reach->start ) +
				reach->travelTime;

			// project target origin onto movement vector through the area
			v1 = reach->end - curUpdate->start;
			v1.Normalize();
			v2 = target - curUpdate->start;
			p = curUpdate->start + ( v2 * v1 ) * v1;

			// get the point on the path closest to the target
			for( j = 0; j < 3; j++ )
			{
				if( ( p[j] > curUpdate->start[j] + 0.1f && p[j] > reach->end[j] + 0.1f ) ||
						( p[j] < curUpdate->start[j] - 0.1f && p[j] < reach->end[j] - 0.1f ) )
				{
					break;
				}
			}
			if( j >= 3 )
			{
				dist = ( target - p ).Length();
			}
			else
			{
				dist = ( target - reach->end ).Length();
			}

			// avoid moving closer to the target
			if( dist < targetDist )
			{
				t += ( targetDist - dist ) * 10;
			}

			// if we already found a closer location
			if( bestTravelTime && t >= bestTravelTime )
			{
				continue;
			}

			// if this is not the best path towards the next area
			if( goalAreaTravelTimes[nextAreaNum] && t >= goalAreaTravelTimes[nextAreaNum] )
			{
				continue;
			}

			// path may not go through any obstacles
			for( k = 0; k < numObstacles; k++ )
			{
				// if the movement vector intersects the expanded obstacle bounds
				if( obstacles[k].expAbsBounds.LineIntersection( curUpdate->start, reach->end ) )
				{
					break;
				}
			}
			if( k < numObstacles )
			{
				continue;
			}

			goalAreaTravelTimes[nextAreaNum] = t;
			nextUpdate = &areaUpdate[nextAreaNum];
			nextUpdate->areaNum = nextAreaNum;
			nextUpdate->tmpTravelTime = t;
			nextUpdate->start = reach->end;

			// if we are not allowed to fly
			if( badTravelFlags & TFL_FLY )
			{
				// avoid areas near ledges
				if( file->GetArea( nextAreaNum ).flags & AREA_LEDGE )
				{
					nextUpdate->tmpTravelTime += LEDGE_TRAVELTIME_PANALTY;
				}
			}

			if( !nextUpdate->isInList )
			{
				nextUpdate->next = NULL;
				nextUpdate->prev = updateListEnd;
				if( updateListEnd )
				{
					updateListEnd->next = nextUpdate;
				}
				else
				{
					updateListStart = nextUpdate;
				}
				updateListEnd = nextUpdate;
				nextUpdate->isInList = true;
			}

			// don't put goal near a ledge
			if( !( nextArea->flags & AREA_LEDGE ) )
			{

				// add travel time through the area
				t += AreaTravelTime( reach->toAreaNum, reach->end, nextArea->center );

				if( !bestTravelTime || t < bestTravelTime )
				{
					// if the area is not visible to the target
					if( callback.TestArea( this, reach->toAreaNum ) )
					{
						bestTravelTime = t;
						bestAreaNum = reach->toAreaNum;
					}
				}
			}
		}
	}

	if( bestAreaNum )
	{
		goal.areaNum = bestAreaNum;
		goal.origin = AreaCenter( bestAreaNum );
		return true;
	}

	return false;
}
