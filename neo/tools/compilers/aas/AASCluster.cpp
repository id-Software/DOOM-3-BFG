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
#include "AASCluster.h"


/*
================
idAASCluster::UpdatePortal
================
*/
bool idAASCluster::UpdatePortal( int areaNum, int clusterNum )
{
	int portalNum;
	aasPortal_t* portal;

	// find the portal for this area
	for( portalNum = 1; portalNum < file->portals.Num(); portalNum++ )
	{
		if( file->portals[portalNum].areaNum == areaNum )
		{
			break;
		}
	}

	if( portalNum >= file->portals.Num() )
	{
		common->Error( "no portal for area %d", areaNum );
		return true;
	}

	portal = &file->portals[portalNum];

	// if the portal is already fully updated
	if( portal->clusters[0] == clusterNum )
	{
		return true;
	}
	if( portal->clusters[1] == clusterNum )
	{
		return true;
	}
	// if the portal has no front cluster yet
	if( !portal->clusters[0] )
	{
		portal->clusters[0] = clusterNum;
	}
	// if the portal has no back cluster yet
	else if( !portal->clusters[1] )
	{
		portal->clusters[1] = clusterNum;
	}
	else
	{
		// remove the cluster portal flag contents
		file->areas[areaNum].contents &= ~AREACONTENTS_CLUSTERPORTAL;
		return false;
	}

	// set the area cluster number to the negative portal number
	file->areas[areaNum].cluster = -portalNum;

	// add the portal to the cluster using the portal index
	file->portalIndex.Append( portalNum );
	file->clusters[clusterNum].numPortals++;
	return true;
}

/*
================
idAASCluster::FloodClusterAreas_r
================
*/
bool idAASCluster::FloodClusterAreas_r( int areaNum, int clusterNum )
{
	aasArea_t* area;
	aasFace_t* face;
	int faceNum, i;
	idReachability* reach;

	area = &file->areas[areaNum];

	// if the area is already part of a cluster
	if( area->cluster > 0 )
	{
		if( area->cluster == clusterNum )
		{
			return true;
		}
		// there's a reachability going from one cluster to another only in one direction
		common->Error( "cluster %d touched cluster %d at area %d\r\n", clusterNum, file->areas[areaNum].cluster, areaNum );
		return false;
	}

	// if this area is a cluster portal
	if( area->contents & AREACONTENTS_CLUSTERPORTAL )
	{
		return UpdatePortal( areaNum, clusterNum );
	}

	// set the area cluster number
	area->cluster = clusterNum;

	if( !noFaceFlood )
	{
		// use area faces to flood into adjacent areas
		for( i = 0; i < area->numFaces; i++ )
		{
			faceNum = abs( file->faceIndex[area->firstFace + i] );
			face = &file->faces[faceNum];
			if( face->areas[0] == areaNum )
			{
				if( face->areas[1] )
				{
					if( !FloodClusterAreas_r( face->areas[1], clusterNum ) )
					{
						return false;
					}
				}
			}
			else
			{
				if( face->areas[0] )
				{
					if( !FloodClusterAreas_r( face->areas[0], clusterNum ) )
					{
						return false;
					}
				}
			}
		}
	}

	// use the reachabilities to flood into other areas
	for( reach = file->areas[areaNum].reach; reach; reach = reach->next )
	{
		if( !FloodClusterAreas_r( reach->toAreaNum, clusterNum ) )
		{
			return false;
		}
	}

	// use the reversed reachabilities to flood into other areas
	for( reach = file->areas[areaNum].rev_reach; reach; reach = reach->rev_next )
	{
		if( !FloodClusterAreas_r( reach->fromAreaNum, clusterNum ) )
		{
			return false;
		}
	}

	return true;
}

/*
================
idAASCluster::RemoveAreaClusterNumbers
================
*/
void idAASCluster::RemoveAreaClusterNumbers()
{
	int i;

	for( i = 1; i < file->areas.Num(); i++ )
	{
		file->areas[i].cluster = 0;
	}
}

/*
================
idAASCluster::NumberClusterAreas
================
*/
void idAASCluster::NumberClusterAreas( int clusterNum )
{
	int i, portalNum;
	aasCluster_t* cluster;
	aasPortal_t* portal;

	cluster = &file->clusters[clusterNum];
	cluster->numAreas = 0;
	cluster->numReachableAreas = 0;

	// number all areas in this cluster WITH reachabilities
	for( i = 1; i < file->areas.Num(); i++ )
	{

		if( file->areas[i].cluster != clusterNum )
		{
			continue;
		}

		if( !( file->areas[i].flags & ( AREA_REACHABLE_WALK | AREA_REACHABLE_FLY ) ) )
		{
			continue;
		}

		file->areas[i].clusterAreaNum = cluster->numAreas++;
		cluster->numReachableAreas++;
	}

	// number all portals in this cluster WITH reachabilities
	for( i = 0; i < cluster->numPortals; i++ )
	{
		portalNum = file->portalIndex[cluster->firstPortal + i];
		portal = &file->portals[portalNum];

		if( !( file->areas[portal->areaNum].flags & ( AREA_REACHABLE_WALK | AREA_REACHABLE_FLY ) ) )
		{
			continue;
		}

		if( portal->clusters[0] == clusterNum )
		{
			portal->clusterAreaNum[0] = cluster->numAreas++;
		}
		else
		{
			portal->clusterAreaNum[1] = cluster->numAreas++;
		}
		cluster->numReachableAreas++;
	}

	// number all areas in this cluster WITHOUT reachabilities
	for( i = 1; i < file->areas.Num(); i++ )
	{

		if( file->areas[i].cluster != clusterNum )
		{
			continue;
		}

		if( file->areas[i].flags & ( AREA_REACHABLE_WALK | AREA_REACHABLE_FLY ) )
		{
			continue;
		}

		file->areas[i].clusterAreaNum = cluster->numAreas++;
	}

	// number all portals in this cluster WITHOUT reachabilities
	for( i = 0; i < cluster->numPortals; i++ )
	{
		portalNum = file->portalIndex[cluster->firstPortal + i];
		portal = &file->portals[portalNum];

		if( file->areas[portal->areaNum].flags & ( AREA_REACHABLE_WALK | AREA_REACHABLE_FLY ) )
		{
			continue;
		}

		if( portal->clusters[0] == clusterNum )
		{
			portal->clusterAreaNum[0] = cluster->numAreas++;
		}
		else
		{
			portal->clusterAreaNum[1] = cluster->numAreas++;
		}
	}
}

/*
================
idAASCluster::FindClusters
================
*/
bool idAASCluster::FindClusters()
{
	int i, clusterNum;
	aasCluster_t cluster;

	RemoveAreaClusterNumbers();

	for( i = 1; i < file->areas.Num(); i++ )
	{
		// if the area is already part of a cluster
		if( file->areas[i].cluster )
		{
			continue;
		}

		// if not flooding through faces only use areas that have reachabilities
		if( noFaceFlood )
		{
			if( !( file->areas[i].flags & ( AREA_REACHABLE_WALK | AREA_REACHABLE_FLY ) ) )
			{
				continue;
			}
		}

		// if the area is a cluster portal
		if( file->areas[i].contents & AREACONTENTS_CLUSTERPORTAL )
		{
			continue;
		}

		cluster.numAreas = 0;
		cluster.numReachableAreas = 0;
		cluster.firstPortal = file->portalIndex.Num();
		cluster.numPortals = 0;
		clusterNum = file->clusters.Num();
		file->clusters.Append( cluster );

		// flood the areas in this cluster
		if( !FloodClusterAreas_r( i, clusterNum ) )
		{
			return false;
		}

		// number the cluster areas
		NumberClusterAreas( clusterNum );
	}
	return true;
}

/*
================
idAASCluster::CreatePortals
================
*/
void idAASCluster::CreatePortals()
{
	int i;
	aasPortal_t portal;

	for( i = 1; i < file->areas.Num(); i++ )
	{
		// if the area is a cluster portal
		if( file->areas[i].contents & AREACONTENTS_CLUSTERPORTAL )
		{
			portal.areaNum = i;
			portal.clusters[0] = portal.clusters[1] = 0;
			portal.maxAreaTravelTime = 0;
			file->portals.Append( portal );
		}
	}
}

/*
================
idAASCluster::TestPortals
================
*/
bool idAASCluster::TestPortals()
{
	int i;
	aasPortal_t* portal, *portal2;
	aasArea_t* area, *area2;
	idReachability* reach;
	bool ok;

	ok = true;
	for( i = 1; i < file->portals.Num(); i++ )
	{
		portal = &file->portals[i];
		area = &file->areas[portal->areaNum];

		// if this portal was already removed
		if( !( area->contents & AREACONTENTS_CLUSTERPORTAL ) )
		{
			continue;
		}

		// may not removed this portal if it has a reachability to a removed portal
		for( reach = area->reach; reach; reach = reach->next )
		{
			area2 = &file->areas[ reach->toAreaNum ];
			if( area2->contents & AREACONTENTS_CLUSTERPORTAL )
			{
				continue;
			}
			if( area2->cluster < 0 )
			{
				break;
			}
		}
		if( reach )
		{
			continue;
		}

		// may not removed this portal if it has a reversed reachability to a removed portal
		for( reach = area->rev_reach; reach; reach = reach->rev_next )
		{
			area2 = &file->areas[ reach->toAreaNum ];
			if( area2->contents & AREACONTENTS_CLUSTERPORTAL )
			{
				continue;
			}
			if( area2->cluster < 0 )
			{
				break;
			}
		}
		if( reach )
		{
			continue;
		}

		// portal should have two clusters set
		if( !portal->clusters[0] )
		{
			area->contents &= ~AREACONTENTS_CLUSTERPORTAL;
			ok = false;
			continue;
		}
		if( !portal->clusters[1] )
		{
			area->contents &= ~AREACONTENTS_CLUSTERPORTAL;
			ok = false;
			continue;
		}

		// this portal may not have reachabilities to a portal that doesn't seperate the same clusters
		for( reach = area->reach; reach; reach = reach->next )
		{
			area2 = &file->areas[ reach->toAreaNum ];

			if( !( area2->contents & AREACONTENTS_CLUSTERPORTAL ) )
			{
				continue;
			}

			if( area2->cluster > 0 )
			{
				area2->contents &= ~AREACONTENTS_CLUSTERPORTAL;
				ok = false;
				continue;
			}

			portal2 = &file->portals[ -file->areas[ reach->toAreaNum ].cluster ];

			if( ( portal2->clusters[0] != portal->clusters[0] && portal2->clusters[0] != portal->clusters[1] ) ||
					( portal2->clusters[1] != portal->clusters[0] && portal2->clusters[1] != portal->clusters[1] ) )
			{
				area2->contents &= ~AREACONTENTS_CLUSTERPORTAL;
				ok = false;
				continue;
			}
		}
	}

	return ok;
}

/*
================
idAASCluster::RemoveInvalidPortals
================
*/
void idAASCluster::RemoveInvalidPortals()
{
	int i, j, k, face1Num, face2Num, otherAreaNum, numOpenAreas, numInvalidPortals;
	aasFace_t* face1, *face2;

	numInvalidPortals = 0;
	for( i = 0; i < file->areas.Num(); i++ )
	{
		if( !( file->areas[i].contents & AREACONTENTS_CLUSTERPORTAL ) )
		{
			continue;
		}

		numOpenAreas = 0;
		for( j = 0; j < file->areas[i].numFaces; j++ )
		{
			face1Num = file->faceIndex[ file->areas[i].firstFace + j ];
			face1 = &file->faces[ abs( face1Num ) ];
			otherAreaNum = face1->areas[ face1Num < 0 ];

			if( !otherAreaNum )
			{
				continue;
			}

			for( k = 0; k < j; k++ )
			{
				face2Num = file->faceIndex[ file->areas[i].firstFace + k ];
				face2 = &file->faces[ abs( face2Num ) ];
				if( otherAreaNum == face2->areas[ face2Num < 0 ] )
				{
					break;
				}
			}
			if( k < j )
			{
				continue;
			}

			if( !( file->areas[otherAreaNum].contents & AREACONTENTS_CLUSTERPORTAL ) )
			{
				numOpenAreas++;
			}
		}

		if( numOpenAreas <= 1 )
		{
			file->areas[i].contents &= AREACONTENTS_CLUSTERPORTAL;
			numInvalidPortals++;
		}
	}

	common->Printf( "\r%6d invalid portals removed\n", numInvalidPortals );
}

/*
================
idAASCluster::Build
================
*/
bool idAASCluster::Build( idAASFileLocal* file )
{

	common->Printf( "[Clustering]\n" );

	this->file = file;
	this->noFaceFlood = true;

	RemoveInvalidPortals();

	while( 1 )
	{

		// delete all existing clusters
		file->DeleteClusters();

		// create the portals from the portal areas
		CreatePortals();

		common->Printf( "\r%6d", file->portals.Num() );

		// find the clusters
		if( !FindClusters() )
		{
			continue;
		}

		// test the portals
		if( !TestPortals() )
		{
			continue;
		}

		break;
	}

	common->Printf( "\r%6d portals\n", file->portals.Num() );
	common->Printf( "%6d clusters\n", file->clusters.Num() );

	for( int i = 0; i < file->clusters.Num(); i++ )
	{
		common->Printf( "%6d reachable areas in cluster %d\n", file->clusters[i].numReachableAreas, i );
	}

	file->ReportRoutingEfficiency();

	return true;
}

/*
================
idAASCluster::BuildSingleCluster
================
*/
bool idAASCluster::BuildSingleCluster( idAASFileLocal* file )
{
	int i, numAreas;
	aasCluster_t cluster;

	common->Printf( "[Clustering]\n" );

	this->file = file;

	// delete all existing clusters
	file->DeleteClusters();

	cluster.firstPortal = 0;
	cluster.numPortals = 0;
	cluster.numAreas = file->areas.Num();
	cluster.numReachableAreas = 0;
	// give all reachable areas in the cluster a number
	for( i = 0; i < file->areas.Num(); i++ )
	{
		file->areas[i].cluster = file->clusters.Num();
		if( file->areas[i].flags & ( AREA_REACHABLE_WALK | AREA_REACHABLE_FLY ) )
		{
			file->areas[i].clusterAreaNum = cluster.numReachableAreas++;
		}
	}
	// give the remaining areas a number within the cluster
	numAreas = cluster.numReachableAreas;
	for( i = 0; i < file->areas.Num(); i++ )
	{
		if( file->areas[i].flags & ( AREA_REACHABLE_WALK | AREA_REACHABLE_FLY ) )
		{
			continue;
		}
		file->areas[i].clusterAreaNum = numAreas++;
	}
	file->clusters.Append( cluster );

	common->Printf( "%6d portals\n", file->portals.Num() );
	common->Printf( "%6d clusters\n", file->clusters.Num() );

	for( i = 0; i < file->clusters.Num(); i++ )
	{
		common->Printf( "%6d reachable areas in cluster %d\n", file->clusters[i].numReachableAreas, i );
	}

	file->ReportRoutingEfficiency();

	return true;
}
