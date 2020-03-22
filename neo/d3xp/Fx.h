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

#ifndef __GAME_FX_H__
#define __GAME_FX_H__

/*
===============================================================================

  Special effects.

===============================================================================
*/

typedef struct
{
	renderLight_t			renderLight;			// light presented to the renderer
	qhandle_t				lightDefHandle;			// handle to renderer light def
	renderEntity_t			renderEntity;			// used to present a model to the renderer
	int						modelDefHandle;			// handle to static renderer model
	float					delay;
	int						particleSystem;
	int						start;
	bool					soundStarted;
	bool					shakeStarted;
	bool					decalDropped;
	bool					launched;
} idFXLocalAction;

class idEntityFx : public idEntity
{
public:
	CLASS_PROTOTYPE( idEntityFx );

	idEntityFx();
	virtual					~idEntityFx();

	void					Spawn();

	void					Save( idSaveGame* savefile ) const;
	void					Restore( idRestoreGame* savefile );

	virtual void			Think();
	void					Setup( const char* fx );
	void					Run( int time );
	void					Start( int time );
	void					Stop();
	const int				Duration();
	const char* 			EffectName();
	const char* 			Joint();
	const bool				Done();

	virtual void			WriteToSnapshot( idBitMsg& msg ) const;
	virtual void			ReadFromSnapshot( const idBitMsg& msg );
	virtual void			ClientThink( const int curTime, const float fraction, const bool predict );
	virtual void			ClientPredictionThink();

	static idEntityFx* 		StartFx( const char* fx, const idVec3* useOrigin, const idMat3* useAxis, idEntity* ent, bool bind );

protected:
	void					Event_Trigger( idEntity* activator );
	void					Event_ClearFx();

	void					CleanUp();
	void					CleanUpSingleAction( const idFXSingleAction& fxaction, idFXLocalAction& laction );
	void					ApplyFade( const idFXSingleAction& fxaction, idFXLocalAction& laction, const int time, const int actualStart );

	int						started;
	int						nextTriggerTime;
	const idDeclFX* 		fxEffect;				// GetFX() should be called before using fxEffect as a pointer
	idList<idFXLocalAction, TAG_FX>	actions;
	idStr					systemName;
};

class idTeleporter : public idEntityFx
{
public:
	CLASS_PROTOTYPE( idTeleporter );

private:
	// teleporters to this location
	void					Event_DoAction( idEntity* activator );
};

#endif /* !__GAME_FX_H__ */
