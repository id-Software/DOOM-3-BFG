/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2012-2016 Robert Beckebans
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

#ifndef __MODELDECAL_H__
#define __MODELDECAL_H__

/*
===============================================================================

	Decals are lightweight primitives for bullet / blood marks on static
	geometry. Decals with common materials will be merged together, but
	additional decals will be allocated as needed. The material should not
	be one that receives lighting, because no interactions are generated
	for these lightweight surfaces.

	FIXME:	Decals on models in portalled off areas do not get freed
			until the area becomes visible again.

===============================================================================
*/

static const int NUM_DECAL_BOUNDING_PLANES	= 6;
#ifdef ID_PC
	static const int MAX_DEFERRED_DECALS		= 16;
	static const int DEFFERED_DECAL_TIMEOUT		= 1000;	// don't create a decal if it wasn't visible within the first second
	static const int MAX_DECALS					= 128;
#else
	static const int MAX_DEFERRED_DECALS		= 16;
	static const int DEFFERED_DECAL_TIMEOUT		= 200;	// don't create a decal if it wasn't visible within the first 200 milliseconds
	static const int MAX_DECALS					= 128;
#endif
static const int MAX_DECAL_VERTS			= 3 + NUM_DECAL_BOUNDING_PLANES + 3 + 6;	// 3 triangle verts clipped NUM_DECAL_BOUNDING_PLANES + 3 times (plus 6 for safety)
static const int MAX_DECAL_INDEXES			= ( MAX_DECAL_VERTS - 2 ) * 3;

compile_time_assert( CONST_ISPOWEROFTWO( MAX_DECALS ) );
// the max indices must be a multiple of 2 for copying indices to write-combined memory
compile_time_assert( ( ( MAX_DECAL_INDEXES* sizeof( triIndex_t ) ) & 15 ) == 0 );

struct decalProjectionParms_t
{
	idPlane					boundingPlanes[NUM_DECAL_BOUNDING_PLANES];
	idPlane					fadePlanes[2];
	idPlane					textureAxis[2];
	idVec3					projectionOrigin;
	idBounds				projectionBounds;
	const idMaterial* 		material;
	float					fadeDepth;
	int						startTime;
	bool					parallel;
	bool					force;
};

// RB begin
#if defined(_WIN32)
	ALIGNTYPE16 struct decal_t
#else
	struct decal_t
#endif
{
	ALIGNTYPE16 idDrawVert	verts[MAX_DECAL_VERTS];
	ALIGNTYPE16 triIndex_t	indexes[MAX_DECAL_INDEXES];
	float					vertDepthFade[MAX_DECAL_VERTS];
	int						numVerts;
	int						numIndexes;
	int						startTime;
	const idMaterial* 		material;
}
#if !defined(_WIN32)
	ALIGNTYPE16
#endif
// RB end
;

class idRenderModelDecal
{
public:
	idRenderModelDecal();
	~idRenderModelDecal();

	// Creates decal projection parameters.
	static bool					CreateProjectionParms( decalProjectionParms_t& parms, const idFixedWinding& winding, const idVec3& projectionOrigin, const bool parallel, const float fadeDepth, const idMaterial* material, const int startTime );

	// Transform the projection parameters from global space to local.
	static void					GlobalProjectionParmsToLocal( decalProjectionParms_t& localParms, const decalProjectionParms_t& globalParms, const idVec3& origin, const idMat3& axis );

	// clear the model for reuse
	void						ReUse();

	// Save the parameters for the renderer front-end to actually create the decal.
	void						AddDeferredDecal( const decalProjectionParms_t& localParms );

	// Creates a decal on the given model.
	void						CreateDeferredDecals( const idRenderModel* model );

	// Remove decals that are completely faded away.
	void						RemoveFadedDecals( int time );

	unsigned int				GetNumDecalDrawSurfs();
	struct drawSurf_t* 			CreateDecalDrawSurf( const struct viewEntity_t* space, unsigned int index );

private:
	decal_t						decals[MAX_DECALS];
	unsigned int				firstDecal;
	unsigned int				nextDecal;

	decalProjectionParms_t		deferredDecals[MAX_DEFERRED_DECALS];
	unsigned int				firstDeferredDecal;
	unsigned int				nextDeferredDecal;

	const idMaterial* 			decalMaterials[MAX_DECALS];
	unsigned int				numDecalMaterials;

	void						CreateDecalFromWinding( const idWinding& w, const idMaterial* decalMaterial, const idPlane fadePlanes[2], float fadeDepth, int startTime );
	void						CreateDecal( const idRenderModel* model, const decalProjectionParms_t& localParms );
};

#endif /* !__MODELDECAL_H__ */
