/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2014-2021 Robert Beckebans
Copyright (C) 2014-2016 Kot in Action Creative Artel

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

#ifndef __RENDERWORLDLOCAL_H__
#define __RENDERWORLDLOCAL_H__

#include "BoundsTrack.h"

// assume any lightDef or entityDef index above this is an internal error
const int LUDICROUS_INDEX	= 10000;


typedef struct portal_s
{
	int						intoArea;		// area this portal leads to
	idWinding* 				w;				// winding points have counter clockwise ordering seen this area
	idPlane					plane;			// view must be on the positive side of the plane to cross
	struct portal_s* 		next;			// next portal of the area
	struct doublePortal_s* 	doublePortal;
} portal_t;


typedef struct doublePortal_s
{
	struct portal_s*			portals[2];
	int						blockingBits;	// PS_BLOCK_VIEW, PS_BLOCK_AIR, etc, set by doors that shut them off

	// A portal will be considered closed if it is past the
	// fog-out point in a fog volume.  We only support a single
	// fog volume over each portal.
	idRenderLightLocal* 		fogLight;
	struct doublePortal_s* 	nextFoggedPortal;
} doublePortal_t;

// RB: added Quake 3 style light grid
// however this 2021 version features Spherical Harmonics instead of ambient + directed color
struct lightGridPoint_t
{
	idVec3			origin;				// not saved to .proc
	byte			valid;				// is not in the void

	SphericalHarmonicsT<idVec3, 4>	shRadiance; // L4 Spherical Harmonics
};

class LightGrid
{
public:
	idVec3					lightGridOrigin;
	idVec3					lightGridSize;
	int						lightGridBounds[3];

	idList<lightGridPoint_t> lightGridPoints;

	int						area;
	idImage* 				irradianceImage;
	int						imageSingleProbeSize; // including border
	int						imageBorderSize;

	LightGrid();

	// setup light grid for given world bounds
	void					SetupLightGrid( const idBounds& bounds, const char* baseName, const idRenderWorld* world, const idVec3& gridSize, int _area, int totalAreas, int maxProbes, bool printToConsole );

	void					GetBaseGridCoord( const idVec3& origin, int gridCoord[3] );

	int						GridCoordToProbeIndex( int gridCoord[3] );
	void					ProbeIndexToGridCoord( const int probeIndex, int gridCoord[3] );

	idVec3					GetGridCoordDebugColor( int gridCoord[3] );
	idVec3					GetProbeIndexDebugColor( const int probeIndex );

	int						CountValidGridPoints() const;

	idImage*				GetIrradianceImage() const
	{
		return irradianceImage;
	}

	// fetch grid lighting on a per object basis
	void					SetupEntityGridLighting( idRenderEntityLocal* def );

private:
	void					CalculateLightGridPointPositions( const idRenderWorld* world, int area );
};
// RB end

typedef struct portalArea_s
{
	int				areaNum;
	int				connectedAreaNum[NUM_PORTAL_ATTRIBUTES];	// if two areas have matching connectedAreaNum, they are
	// not separated by a portal with the apropriate PS_BLOCK_* blockingBits

	idBounds		globalBounds;	// RB: AABB of the BSP area used for light grid density

	LightGrid		lightGrid;

	int				viewCount;		// set by R_FindViewLightsAndEntities
	portal_t* 		portals;		// never changes after load
	areaReference_t	entityRefs;		// head/tail of doubly linked list, may change
	areaReference_t	lightRefs;		// head/tail of doubly linked list, may change
	areaReference_t	envprobeRefs;	// head/tail of doubly linked list, may change
} portalArea_t;


static const int	CHILDREN_HAVE_MULTIPLE_AREAS = -2;
static const int	AREANUM_SOLID = -1;
typedef struct
{
	idPlane			plane;
	int				children[2];		// negative numbers are (-1 - areaNumber), 0 = solid
	int				commonChildrenArea;	// if all children are either solid or a single area,
	// this is the area number, else CHILDREN_HAVE_MULTIPLE_AREAS
} areaNode_t;

struct reusableDecal_t
{
	qhandle_t				entityHandle;
	int						lastStartTime;
	idRenderModelDecal* 	decals;
};

struct reusableOverlay_t
{
	qhandle_t				entityHandle;
	int						lastStartTime;
	idRenderModelOverlay* 	overlays;
};

struct portalStack_t;

class idRenderWorldLocal : public idRenderWorld
{
public:
	idRenderWorldLocal();
	virtual					~idRenderWorldLocal();

	virtual	bool			InitFromMap( const char* mapName );
	virtual void			ResetLocalRenderModels();				// Fixes a crash when switching between expansion packs in Doom3:BFG

	virtual	qhandle_t		AddEntityDef( const renderEntity_t* re );
	virtual	void			UpdateEntityDef( qhandle_t entityHandle, const renderEntity_t* re );
	virtual	void			FreeEntityDef( qhandle_t entityHandle );
	virtual const renderEntity_t* GetRenderEntity( qhandle_t entityHandle ) const;

	virtual	qhandle_t		AddLightDef( const renderLight_t* rlight );
	virtual	void			UpdateLightDef( qhandle_t lightHandle, const renderLight_t* rlight );
	virtual	void			FreeLightDef( qhandle_t lightHandle );
	virtual const renderLight_t* GetRenderLight( qhandle_t lightHandle ) const;

	// RB: environment probes for IBL
	virtual	qhandle_t		AddEnvprobeDef( const renderEnvironmentProbe_t* ep );
	virtual	void			UpdateEnvprobeDef( qhandle_t envprobeHandle, const renderEnvironmentProbe_t* ep );
	virtual	void			FreeEnvprobeDef( qhandle_t envprobeHandle );
	virtual const renderEnvironmentProbe_t* GetRenderEnvprobe( qhandle_t envprobeHandle ) const;
	// RB end

	virtual bool			CheckAreaForPortalSky( int areaNum );

	virtual	void			GenerateAllInteractions();
	virtual void			RegenerateWorld();

	virtual void			ProjectDecalOntoWorld( const idFixedWinding& winding, const idVec3& projectionOrigin, const bool parallel, const float fadeDepth, const idMaterial* material, const int startTime );
	virtual void			ProjectDecal( qhandle_t entityHandle, const idFixedWinding& winding, const idVec3& projectionOrigin, const bool parallel, const float fadeDepth, const idMaterial* material, const int startTime );
	virtual void			ProjectOverlay( qhandle_t entityHandle, const idPlane localTextureAxis[2], const idMaterial* material, const int startTime );
	virtual void			RemoveDecals( qhandle_t entityHandle );

	virtual void			SetRenderView( const renderView_t* renderView );
	virtual	void			RenderScene( const renderView_t* renderView );

	virtual	int				NumAreas() const;
	virtual int				PointInArea( const idVec3& point ) const;
	virtual int				BoundsInAreas( const idBounds& bounds, int* areas, int maxAreas ) const;
	virtual	int				NumPortalsInArea( int areaNum );
	virtual exitPortal_t	GetPortal( int areaNum, int portalNum );
	virtual	idBounds		AreaBounds( int areaNum ) const; // RB

	virtual	guiPoint_t		GuiTrace( qhandle_t entityHandle, const idVec3 start, const idVec3 end ) const;
	virtual bool			ModelTrace( modelTrace_t& trace, qhandle_t entityHandle, const idVec3& start, const idVec3& end, const float radius ) const;
	virtual bool			Trace( modelTrace_t& trace, const idVec3& start, const idVec3& end, const float radius, bool skipDynamic = true, bool skipPlayer = false ) const;
	virtual bool			FastWorldTrace( modelTrace_t& trace, const idVec3& start, const idVec3& end ) const;

	virtual void			DebugClearLines( int time );
	virtual void			DebugLine( const idVec4& color, const idVec3& start, const idVec3& end, const int lifetime = 0, const bool depthTest = false );
	virtual void			DebugArrow( const idVec4& color, const idVec3& start, const idVec3& end, int size, const int lifetime = 0 );
	virtual void			DebugWinding( const idVec4& color, const idWinding& w, const idVec3& origin, const idMat3& axis, const int lifetime = 0, const bool depthTest = false );
	virtual void			DebugCircle( const idVec4& color, const idVec3& origin, const idVec3& dir, const float radius, const int numSteps, const int lifetime = 0, const bool depthTest = false );
	virtual void			DebugSphere( const idVec4& color, const idSphere& sphere, const int lifetime = 0, bool depthTest = false );
	virtual void			DebugBounds( const idVec4& color, const idBounds& bounds, const idVec3& org = vec3_origin, const int lifetime = 0 );
	virtual void			DebugBox( const idVec4& color, const idBox& box, const int lifetime = 0 );
	virtual void			DebugCone( const idVec4& color, const idVec3& apex, const idVec3& dir, float radius1, float radius2, const int lifetime = 0 );
	virtual void			DebugScreenRect( const idVec4& color, const idScreenRect& rect, const viewDef_t* viewDef, const int lifetime = 0 );
	virtual void			DebugAxis( const idVec3& origin, const idMat3& axis );

	virtual void			DebugClearPolygons( int time );
	virtual void			DebugPolygon( const idVec4& color, const idWinding& winding, const int lifeTime = 0, const bool depthTest = false );

	virtual void			DrawText( const char* text, const idVec3& origin, float scale, const idVec4& color, const idMat3& viewAxis, const int align = 1, const int lifetime = 0, bool depthTest = false );

	//-----------------------

	idStr					mapName;				// ie: maps/tim_dm2.proc, written to demoFile
	ID_TIME_T				mapTimeStamp;			// for fast reloads of the same level

	areaNode_t* 			areaNodes;
	int						numAreaNodes;

	portalArea_t* 			portalAreas;
	int						numPortalAreas;
	int						connectedAreaNum;		// incremented every time a door portal state changes

	idScreenRect* 			areaScreenRect;

	doublePortal_t* 		doublePortals;
	int						numInterAreaPortals;

	idList<idRenderModel*, TAG_MODEL>	localModels;

	idList<idRenderEntityLocal*, TAG_ENTITY>		entityDefs;
	idList<idRenderLightLocal*, TAG_LIGHT>			lightDefs;
	idList<RenderEnvprobeLocal*, TAG_ENVPROBE>		envprobeDefs; // RB

	idBlockAlloc<areaReference_t, 1024> areaReferenceAllocator;
	idBlockAlloc<idInteraction, 256>	interactionAllocator;

#ifdef ID_PC
	static const int MAX_DECAL_SURFACES = 32;
#else
	static const int MAX_DECAL_SURFACES = 16;
#endif
	idArray<reusableDecal_t, MAX_DECAL_SURFACES>	decals;
	idArray<reusableOverlay_t, MAX_DECAL_SURFACES>	overlays;

	// all light / entity interactions are referenced here for fast lookup without
	// having to crawl the doubly linked lists.  EnntityDefs are sequential for better
	// cache access, because the table is accessed by light in idRenderWorldLocal::CreateLightDefInteractions()
	// Growing this table is time consuming, so we add a pad value to the number
	// of entityDefs and lightDefs
	idInteraction** 		interactionTable;
	int						interactionTableWidth;		// entityDefs
	int						interactionTableHeight;		// lightDefs

	bool					generateAllInteractionsCalled;

	//-----------------------
	// RenderWorld_load.cpp

	idRenderModel* 			ParseModel( idLexer* src, const char* mapName, ID_TIME_T mapTimeStamp, idFile* fileOut );
	idRenderModel* 			ParseShadowModel( idLexer* src, idFile* fileOut );
	void					SetupAreaRefs();
	void					ParseInterAreaPortals( idLexer* src, idFile* fileOut );
	void					ParseNodes( idLexer* src, idFile* fileOut );
	int						CommonChildrenArea_r( areaNode_t* node );
	void					FreeWorld();
	void					ClearWorld();
	void					FreeDefs();
	void					TouchWorldModels();
	void					AddWorldModelEntities();
	void					ClearPortalStates();
	void					ReadBinaryAreaPortals( idFile* file );
	void					ReadBinaryNodes( idFile* file );
	idRenderModel* 			ReadBinaryModel( idFile* file );
	idRenderModel* 			ReadBinaryShadowModel( idFile* file );

	//--------------------------
	// RenderWorld_portals.cpp

	// if we hit this many planes, we will just stop cropping the
	// view down, which is still correct, just conservative
	static const int MAX_PORTAL_PLANES	= 20;

	struct portalStack_t
	{
		const portal_t* 		p;
		const portalStack_t* 	next;
		// positive side is outside the visible frustum
		int						numPortalPlanes;
		idPlane					portalPlanes[MAX_PORTAL_PLANES + 1];
		idScreenRect			rect;
	};

	bool					CullEntityByPortals( const idRenderEntityLocal* entity, const portalStack_t* ps );
	void					AddAreaViewEntities( int areaNum, const portalStack_t* ps );

	bool					CullLightByPortals( const idRenderLightLocal* light, const portalStack_t* ps );
	void					AddAreaViewLights( int areaNum, const portalStack_t* ps );

	// RB begin
	bool					CullEnvprobeByPortals( const RenderEnvprobeLocal* probe, const portalStack_t* ps );
	void					AddAreaViewEnvprobes( int areaNum, const portalStack_t* ps );
	// RB end

	void					AddAreaToView( int areaNum, const portalStack_t* ps );
	idScreenRect			ScreenRectFromWinding( const idWinding* w, const viewEntity_t* space );
	bool					PortalIsFoggedOut( const portal_t* p );
	void					FloodViewThroughArea_r( const idVec3& origin, int areaNum, const portalStack_t* ps );
	void					FlowViewThroughPortals( const idVec3& origin, int numPlanes, const idPlane* planes );
	void					BuildConnectedAreas_r( int areaNum );
	void					BuildConnectedAreas();
	void					FindViewLightsAndEntities();

	void					FloodLightThroughArea_r( idRenderLightLocal* light, int areaNum, const portalStack_t* ps );
	void					FlowLightThroughPortals( idRenderLightLocal* light );

	int						NumPortals() const;
	qhandle_t				FindPortal( const idBounds& b ) const;
	void					SetPortalState( qhandle_t portal, int blockingBits );
	int						GetPortalState( qhandle_t portal );
	bool					AreasAreConnected( int areaNum1, int areaNum2, portalConnection_t connection ) const;
	void					FloodConnectedAreas( portalArea_t* area, int portalAttributeIndex );
	idScreenRect& 			GetAreaScreenRect( int areaNum ) const
	{
		return areaScreenRect[areaNum];
	}

	//--------------------------
	// RenderWorld_demo.cpp

	void					StartWritingDemo( idDemoFile* demo );
	void					StopWritingDemo();
	bool					ProcessDemoCommand( idDemoFile* readDemo, renderView_t* demoRenderView, int* demoTimeOffset );

	void					WriteLoadMap();
	void					WriteRenderView( const renderView_t* renderView );
	void					WriteVisibleDefs( const viewDef_t* viewDef );
	void					WriteFreeDecal( idDemoFile* f, qhandle_t handle );
	void					WriteFreeOverlay( idDemoFile* f, qhandle_t handle );
	void					WriteFreeLight( qhandle_t handle );
	void					WriteFreeEntity( qhandle_t handle );
	void					WriteFreeEnvprobe( qhandle_t handle ); // RB
	void					WriteRenderDecal( idDemoFile* f, qhandle_t handle );
	void					WriteRenderOverlay( idDemoFile* f, qhandle_t handle );
	void					WriteRenderLight( idDemoFile* f, qhandle_t handle, const renderLight_t* light );
	void					WriteRenderEntity( idDemoFile* f, idRenderEntityLocal* entity );
	void					WriteRenderEnvprobe( qhandle_t handle, const renderEnvironmentProbe_t* probe ); // RB
	void					ReadRenderEntity();
	void					ReadRenderLight();
	void					ReadRenderEnvprobe(); // RB


	//--------------------------
	// RenderWorld.cpp

	void					ResizeInteractionTable();

	void					AddEntityRefToArea( idRenderEntityLocal* def, portalArea_t* area );
	void					AddLightRefToArea( idRenderLightLocal* light, portalArea_t* area );
	void					AddEnvprobeRefToArea( RenderEnvprobeLocal* probe, portalArea_t* area ); // RB

	void					RecurseProcBSP_r( modelTrace_t* results, int parentNodeNum, int nodeNum, float p1f, float p2f, const idVec3& p1, const idVec3& p2 ) const;
	void					BoundsInAreas_r( int nodeNum, const idBounds& bounds, int* areas, int* numAreas, int maxAreas ) const;

	float					DrawTextLength( const char* text, float scale, int len = 0 );

	void					FreeInteractions();

	void					PushFrustumIntoTree_r( idRenderEntityLocal* def, idRenderLightLocal* light, const frustumCorners_t& corners, int nodeNum );
	void					PushFrustumIntoTree( idRenderEntityLocal* def, idRenderLightLocal* light, const idRenderMatrix& frustumTransform, const idBounds& frustumBounds );
	void					PushEnvprobeIntoTree_r( RenderEnvprobeLocal* probe, int nodeNum ); // RB

	idRenderModelDecal* 	AllocDecal( qhandle_t newEntityHandle, int startTime );
	idRenderModelOverlay* 	AllocOverlay( qhandle_t newEntityHandle, int startTime );

	//-------------------------------
	// tr_light.c
	void					CreateLightDefInteractions( idRenderLightLocal* const ldef, const int renderViewID );

// RB begin

	//--------------------------
	// RenderWorld_lightgrid.cpp

//private:
	void					SetupLightGrid();

	void					WriteLightGridsToFile( const char* filename );
	void					WriteLightGrid( idFile* fp, const LightGrid& lightGrid );

	bool					LoadLightGridFile( const char* name );
	void					LoadLightGridImages();

	void					ParseLightGridPoints( idLexer* src, idFile* fileOut );
	void					ReadBinaryLightGridPoints( idFile* file );
// RB end
};

// if an entity / light combination has been evaluated and found to not genrate any surfaces or shadows,
// the constant INTERACTION_EMPTY will be stored in the interaction table, int contrasts to NULL, which
// means that the combination has not yet been tested for having surfaces.
static idInteraction* const INTERACTION_EMPTY = ( idInteraction* )1;

void R_ListRenderLightDefs_f( const idCmdArgs& args );
void R_ListRenderEntityDefs_f( const idCmdArgs& args );

#endif /* !__RENDERWORLDLOCAL_H__ */
