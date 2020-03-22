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

#ifndef __GAME_PVS_H__
#define __GAME_PVS_H__

/*
===================================================================================

	PVS

	Note: mirrors and other special view portals are not taken into account

===================================================================================
*/


typedef struct pvsHandle_s
{
	int					i;			// index to current pvs
	unsigned int		h;			// handle for current pvs
} pvsHandle_t;


typedef struct pvsCurrent_s
{
	pvsHandle_t			handle;		// current pvs handle
	byte* 				pvs;		// current pvs bit string
} pvsCurrent_t;

#define MAX_CURRENT_PVS		64		// must be a power of 2

typedef enum
{
	PVS_NORMAL				= 0,	// PVS through portals taking portal states into account
	PVS_ALL_PORTALS_OPEN	= 1,	// PVS through portals assuming all portals are open
	PVS_CONNECTED_AREAS		= 2		// PVS considering all topologically connected areas visible
} pvsType_t;


class idPVS
{
public:
	idPVS();
	~idPVS();
	// setup for the current map
	void				Init();
	void				Shutdown();
	// get the area(s) the source is in
	int					GetPVSArea( const idVec3& point ) const;		// returns the area number
	int					GetPVSAreas( const idBounds& bounds, int* areas, int maxAreas ) const;	// returns number of areas
	// setup current PVS for the source
	pvsHandle_t			SetupCurrentPVS( const idVec3& source, const pvsType_t type = PVS_NORMAL ) const;
	pvsHandle_t			SetupCurrentPVS( const idBounds& source, const pvsType_t type = PVS_NORMAL ) const;
	pvsHandle_t			SetupCurrentPVS( const int sourceArea, const pvsType_t type = PVS_NORMAL ) const;
	pvsHandle_t			SetupCurrentPVS( const int* sourceAreas, const int numSourceAreas, const pvsType_t type = PVS_NORMAL ) const;
	pvsHandle_t			MergeCurrentPVS( pvsHandle_t pvs1, pvsHandle_t pvs2 ) const;
	void				FreeCurrentPVS( pvsHandle_t handle ) const;
	// returns true if the target is within the current PVS
	bool				InCurrentPVS( const pvsHandle_t handle, const idVec3& target ) const;
	bool				InCurrentPVS( const pvsHandle_t handle, const idBounds& target ) const;
	bool				InCurrentPVS( const pvsHandle_t handle, const int targetArea ) const;
	bool				InCurrentPVS( const pvsHandle_t handle, const int* targetAreas, int numTargetAreas ) const;
	// draw all portals that are within the PVS of the source
	void				DrawPVS( const idVec3& source, const pvsType_t type = PVS_NORMAL ) const;
	void				DrawPVS( const idBounds& source, const pvsType_t type = PVS_NORMAL ) const;
	// visualize the PVS the handle points to
	void				DrawCurrentPVS( const pvsHandle_t handle, const idVec3& source ) const;

	bool				CheckAreasForPortalSky( const pvsHandle_t handle, const idVec3& origin );

private:
	int					numAreas;
	int					numPortals;
	bool* 				connectedAreas;
	int* 				areaQueue;
	byte* 				areaPVS;
	// current PVS for a specific source possibly taking portal states (open/closed) into account
	mutable pvsCurrent_t currentPVS[MAX_CURRENT_PVS];
	// used to create PVS
	int					portalVisBytes;
	int					portalVisLongs; // DG: Note: these are really ints now..
	int					areaVisBytes;
	int					areaVisLongs;   // DG: Note: these are really ints now..
	struct pvsPortal_s* pvsPortals;
	struct pvsArea_s* 	pvsAreas;

private:
	int					GetPortalCount() const;
	void				CreatePVSData();
	void				DestroyPVSData();
	void				CopyPortalPVSToMightSee() const;
	void				FloodFrontPortalPVS_r( struct pvsPortal_s* portal, int areaNum ) const;
	void				FrontPortalPVS() const;
	struct pvsStack_s* 	FloodPassagePVS_r( struct pvsPortal_s* source, const struct pvsPortal_s* portal, struct pvsStack_s* prevStack ) const;
	void				PassagePVS() const;
	void				AddPassageBoundaries( const idWinding& source, const idWinding& pass, bool flipClip, idPlane* bounds, int& numBounds, int maxBounds ) const;
	void				CreatePassages() const;
	void				DestroyPassages() const;
	int					AreaPVSFromPortalPVS() const;
	void				GetConnectedAreas( int srcArea, bool* connectedAreas ) const;
	pvsHandle_t			AllocCurrentPVS( unsigned int h ) const;
};

#endif /* !__GAME_PVS_H__ */
