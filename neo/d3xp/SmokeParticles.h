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

#ifndef __SMOKEPARTICLES_H__
#define __SMOKEPARTICLES_H__

/*
===============================================================================

	Smoke systems are for particles that are emitted off of things that are
	constantly changing position and orientation, like muzzle smoke coming
	from a bone on a weapon, blood spurting from a wound, or particles
	trailing from a monster limb.

	The smoke particles are always evaluated and rendered each tic, so there
	is a performance cost with using them for continuous effects. The general
	particle systems are completely parametric, and have no performance
	overhead when not in view.

	All smoke systems share the same shaderparms, so any coloration must be
	done in the particle definition.

	Each particle model has its own shaderparms, which can be used by the
	particle materials.

===============================================================================
*/

typedef struct singleSmoke_s
{
	struct singleSmoke_s* 		next;
	int							privateStartTime;	// start time for this particular particle
	int							index;				// particle index in system, 0 <= index < stage->totalParticles
	idRandom					random;
	idVec3						origin;
	idMat3						axis;
	int							timeGroup;
} singleSmoke_t;

typedef struct
{
	const idParticleStage* 		stage;
	singleSmoke_t* 				smokes;
} activeSmokeStage_t;


class idSmokeParticles
{
public:
	idSmokeParticles();

	// creats an entity covering the entire world that will call back each rendering
	void						Init();
	void						Shutdown();

	// spits out a particle, returning false if the system will not emit any more particles in the future
	bool						EmitSmoke( const idDeclParticle* smoke, const int startTime, const float diversity,
										   const idVec3& origin, const idMat3& axis, int timeGroup /*_D3XP*/ );

	// free old smokes
	void						FreeSmokes();

private:
	bool						initialized;

	renderEntity_t				renderEntity;			// used to present a model to the renderer
	int							renderEntityHandle;		// handle to static renderer model

	static const int			MAX_SMOKE_PARTICLES = 10000;
	singleSmoke_t				smokes[MAX_SMOKE_PARTICLES];

	idList<activeSmokeStage_t, TAG_PARTICLE>	activeStages;
	singleSmoke_t* 				freeSmokes;
	int							numActiveSmokes;
	int							currentParticleTime;	// don't need to recalculate if == view time

	bool						UpdateRenderEntity( renderEntity_s* renderEntity, const renderView_t* renderView );
	static bool					ModelCallback( renderEntity_s* renderEntity, const renderView_t* renderView );
};

#endif /* !__SMOKEPARTICLES_H__ */
