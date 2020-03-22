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

#ifndef __GAME_BRITTLEFRACTURE_H__
#define __GAME_BRITTLEFRACTURE_H__


/*
===============================================================================

B-rep Brittle Fracture - Static entity using the boundary representation
of the render model which can fracture.

===============================================================================
*/

typedef struct shard_s
{
	idClipModel* 				clipModel;
	idFixedWinding				winding;
	idList<idFixedWinding*, TAG_PHYSICS_BRITTLE>	decals;
	idList<bool>				edgeHasNeighbour;
	idList<struct shard_s*, TAG_PHYSICS_BRITTLE>	neighbours;
	idPhysics_RigidBody			physicsObj;
	int							droppedTime;
	bool						atEdge;
	int							islandNum;
} shard_t;


class idBrittleFracture : public idEntity
{

public:
	CLASS_PROTOTYPE( idBrittleFracture );

	idBrittleFracture();
	virtual						~idBrittleFracture();

	void						Save( idSaveGame* savefile ) const;
	void						Restore( idRestoreGame* savefile );

	void						Spawn();

	virtual void				Present();
	virtual void				Think();
	virtual void				ApplyImpulse( idEntity* ent, int id, const idVec3& point, const idVec3& impulse );
	virtual void				AddForce( idEntity* ent, int id, const idVec3& point, const idVec3& force );
	virtual void				AddDamageEffect( const trace_t& collision, const idVec3& velocity, const char* damageDefName );
	virtual void				Killed( idEntity* inflictor, idEntity* attacker, int damage, const idVec3& dir, int location );

	void						ProjectDecal( const idVec3& point, const idVec3& dir, const int time, const char* damageDefName );
	bool						IsBroken() const;

	enum
	{
		EVENT_PROJECT_DECAL = idEntity::EVENT_MAXEVENTS,
		EVENT_SHATTER,
		EVENT_MAXEVENTS
	};

	virtual void				ClientThink( const int curTime, const float fraction, const bool predict );
	virtual void				ClientPredictionThink();
	virtual bool				ClientReceiveEvent( int event, int time, const idBitMsg& msg );

private:
	// setttings
	const idMaterial* 			material;
	const idMaterial* 			decalMaterial;
	float						decalSize;
	float						maxShardArea;
	float						maxShatterRadius;
	float						minShatterRadius;
	float						linearVelocityScale;
	float						angularVelocityScale;
	float						shardMass;
	float						density;
	float						friction;
	float						bouncyness;
	idStr						fxFracture;

	struct fractureEvent_s
	{
		int				eventType;
		idVec3			point;
		idVec3			vector;
	};
	idList<fractureEvent_s>		storedEvents;
	bool						processStoredEvents;
	idRenderModel* 				defaultRenderModel;
	bool						isXraySurface;

	// state
	idPhysics_StaticMulti		physicsObj;
	idList<shard_t*, TAG_PHYSICS_BRITTLE>	shards;
	idBounds					bounds;
	bool						disableFracture;

	// for rendering
	mutable int					lastRenderEntityUpdate;
	mutable bool				changed;

	bool						UpdateRenderEntity( renderEntity_s* renderEntity, const renderView_t* renderView ) const;
	static bool					ModelCallback( renderEntity_s* renderEntity, const renderView_t* renderView );

	void						AddShard( idClipModel* clipModel, idFixedWinding& w );
	void						RemoveShard( int index );
	void						DropShard( shard_t* shard, const idVec3& point, const idVec3& dir, const float impulse, const int time );
	void						Shatter( const idVec3& point, const idVec3& impulse, const int time );
	void						DropFloatingIslands( const idVec3& point, const idVec3& impulse, const int time );
	void						Break();
	void						Fracture_r( idFixedWinding& w, idRandom2& random );
	void						CreateFractures( const idRenderModel* renderModel );
	void						FindNeighbours();

	void						Event_Activate( idEntity* activator );
	void						Event_Touch( idEntity* other, trace_t* trace );
};

#endif /* !__GAME_BRITTLEFRACTURE_H__ */
