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

#ifndef __GAME_PLAYERVIEW_H__
#define __GAME_PLAYERVIEW_H__

class idMenuHandler_HUD;

/*
===============================================================================

  Player view.

===============================================================================
*/

// screenBlob_t are for the on-screen damage claw marks, etc
typedef struct
{
	const idMaterial* 	material;
	float				x, y, w, h;
	float				s1, t1, s2, t2;
	int					finishTime;
	int					startFadeTime;
	float				driftAmount;
} screenBlob_t;

#define	MAX_SCREEN_BLOBS	8





class WarpPolygon_t
{
public:
	idVec4					outer1;
	idVec4					outer2;
	idVec4					center;
};

class Warp_t
{
public:
	int						id;
	bool					active;

	int						startTime;
	float					initialRadius;

	idVec3					worldOrigin;
	idVec2					screenOrigin;

	int						durationMsec;

	idList<WarpPolygon_t, TAG_IDLIB_LIST_PLAYER>	polys;
};








class idPlayerView;
class FullscreenFXManager;


/*
==================
FxFader
==================
*/
class FxFader
{
	enum
	{
		FX_STATE_OFF,
		FX_STATE_RAMPUP,
		FX_STATE_RAMPDOWN,
		FX_STATE_ON
	};

	int						time;
	int						state;
	float					alpha;
	int						msec;

public:
	FxFader();

	// primary functions
	bool					SetTriggerState( bool active );

	virtual void			Save( idSaveGame* savefile );
	virtual void			Restore( idRestoreGame* savefile );

	// fader functions
	void					SetFadeTime( int t )
	{
		msec = t;
	};
	int						GetFadeTime()
	{
		return msec;
	};

	// misc functions
	float					GetAlpha()
	{
		return alpha;
	};
};


/*
==================
FullscreenFX
==================
*/
class FullscreenFX
{
protected:
	idStr					name;
	FxFader					fader;
	FullscreenFXManager*		fxman;

public:
	FullscreenFX()
	{
		fxman = NULL;
	};
	virtual					~FullscreenFX()							{ };

	virtual void			Initialize()							= 0;
	virtual bool			Active()								= 0;
	virtual void			HighQuality()							= 0;
	virtual void			LowQuality()							{ };
	virtual void			AccumPass( const renderView_t* view )	{ };
	virtual bool			HasAccum()
	{
		return false;
	};

	void					SetName( idStr n )
	{
		name = n;
	};
	idStr					GetName()
	{
		return name;
	};

	void					SetFXManager( FullscreenFXManager* fx )
	{
		fxman = fx;
	};

	bool					SetTriggerState( bool state )
	{
		return fader.SetTriggerState( state );
	};
	void					SetFadeSpeed( int msec )
	{
		fader.SetFadeTime( msec );
	};
	float					GetFadeAlpha()
	{
		return fader.GetAlpha();
	};

	virtual void			Save( idSaveGame* savefile );
	virtual void			Restore( idRestoreGame* savefile );
};

/*
==================
FullscreenFX_Helltime
==================
*/
class FullscreenFX_Helltime : public FullscreenFX
{
	const idMaterial* 		initMaterial;
	const idMaterial* 		captureMaterials[3];
	const idMaterial* 		drawMaterial;
	bool					clearAccumBuffer;

	int						DetermineLevel();

public:
	virtual void			Initialize();
	virtual bool			Active();
	virtual void			HighQuality();
	virtual void			AccumPass( const renderView_t* view );
	virtual bool			HasAccum()
	{
		return true;
	};

	virtual void			Restore( idRestoreGame* savefile );
};

/*
==================
FullscreenFX_Multiplayer
==================
*/
class FullscreenFX_Multiplayer : public FullscreenFX
{
	const idMaterial* 		initMaterial;
	const idMaterial* 		captureMaterial;
	const idMaterial* 		drawMaterial;
	bool					clearAccumBuffer;

	int						DetermineLevel();

public:
	virtual void			Initialize();
	virtual bool			Active();
	virtual void			HighQuality();
	virtual void			AccumPass( const renderView_t* view );
	virtual bool			HasAccum()
	{
		return true;
	};

	virtual void			Restore( idRestoreGame* savefile );
};

/*
==================
FullscreenFX_Warp
==================
*/
class FullscreenFX_Warp : public FullscreenFX
{
	const idMaterial*		material;
	bool					grabberEnabled;
	int						startWarpTime;

	void					DrawWarp( WarpPolygon_t wp, float interp );

public:
	virtual void			Initialize();
	virtual bool			Active();
	virtual void			HighQuality();

	void					EnableGrabber( bool active )
	{
		grabberEnabled = active;
		startWarpTime = gameLocal.slow.time;
	};

	virtual void			Save( idSaveGame* savefile );
	virtual void			Restore( idRestoreGame* savefile );
};

/*
==================
FullscreenFX_EnviroSuit
==================
*/
class FullscreenFX_EnviroSuit : public FullscreenFX
{
	const idMaterial*		material;

public:
	virtual void			Initialize();
	virtual bool			Active();
	virtual void			HighQuality();
};

/*
==================
FullscreenFX_DoubleVision
==================
*/
class FullscreenFX_DoubleVision : public FullscreenFX
{
	const idMaterial*		material;

public:
	virtual void			Initialize();
	virtual bool			Active();
	virtual void			HighQuality();
};

/*
==================
FullscreenFX_InfluenceVision
==================
*/
class FullscreenFX_InfluenceVision : public FullscreenFX
{

public:
	virtual void			Initialize();
	virtual bool			Active();
	virtual void			HighQuality();
};

/*
==================
FullscreenFX_Bloom
==================
*/
class FullscreenFX_Bloom : public FullscreenFX
{
	const idMaterial*		drawMaterial;
	const idMaterial*		initMaterial;

	float					currentIntensity;
	float					targetIntensity;

public:
	virtual void			Initialize();
	virtual bool			Active();
	virtual void			HighQuality();

	virtual void			Save( idSaveGame* savefile );
	virtual void			Restore( idRestoreGame* savefile );
};



/*
==================
FullscreenFXManager
==================
*/
class FullscreenFXManager
{
	idList<FullscreenFX*, TAG_FX>	fx;

	idPlayerView* 			playerView;
	const idMaterial*		blendBackMaterial;

	void					CreateFX( idStr name, idStr fxtype, int fade );

public:
	FullscreenFXManager();
	virtual					~FullscreenFXManager();

	void					Initialize( idPlayerView* pv );

	void					Process( const renderView_t* view );
	void					Blendback( float alpha );

	idPlayerView*			GetPlayerView()
	{
		return playerView;
	};
	idPlayer*				GetPlayer()
	{
		return gameLocal.GetLocalPlayer();
	};

	int						GetNum()
	{
		return fx.Num();
	};
	FullscreenFX*			GetFX( int index )
	{
		return fx[index];
	};
	FullscreenFX*			FindFX( idStr name );

	void					Save( idSaveGame* savefile );
	void					Restore( idRestoreGame* savefile );
};










class idPlayerView
{
public:
	idPlayerView();
	~idPlayerView();

	void				Save( idSaveGame* savefile ) const;
	void				Restore( idRestoreGame* savefile );

	void				SetPlayerEntity( class idPlayer* playerEnt );

	void				ClearEffects();

	void				DamageImpulse( idVec3 localKickDir, const idDict* damageDef );

	void				WeaponFireFeedback( const idDict* weaponDef );

	idAngles			AngleOffset() const;			// returns the current kick angle

	idMat3				ShakeAxis() const;			// returns the current shake angle

	void				CalculateShake();

	// this may involve rendering to a texture and displaying
	// that with a warp model or in double vision mode
	void				RenderPlayerView( idMenuHandler_HUD* hudManager );
	void				EmitStereoEyeView( const int eye, idMenuHandler_HUD* hudManager );

	void				Fade( idVec4 color, int time );

	void				Flash( idVec4 color, int time );

	// temp for view testing
	void				EnableBFGVision( bool b )
	{
		bfgVision = b;
	};

private:
	void				SingleView( const renderView_t* view, idMenuHandler_HUD* hudManager );
	void				ScreenFade();

	screenBlob_t* 		GetScreenBlob();

	screenBlob_t		screenBlobs[MAX_SCREEN_BLOBS];

public:
	int					dvFinishTime;		// double vision will be stopped at this time

	int					kickFinishTime;		// view kick will be stopped at this time
	idAngles			kickAngles;

	bool				bfgVision;			//

	const idMaterial* 	tunnelMaterial;		// health tunnel vision
	const idMaterial* 	armorMaterial;		// armor damage view effect
	const idMaterial* 	berserkMaterial;	// berserk effect
	const idMaterial* 	irGogglesMaterial;	// ir effect
	const idMaterial* 	bloodSprayMaterial; // blood spray
	const idMaterial* 	bfgMaterial;		// when targeted with BFG
	float				lastDamageTime;		// accentuate the tunnel effect for a while

	idVec4				fadeColor;			// fade color
	idVec4				fadeToColor;		// color to fade to
	idVec4				fadeFromColor;		// color to fade from
	float				fadeRate;			// fade rate
	int					fadeTime;			// fade time

	idAngles			shakeAng;			// from the sound sources

	idPlayer* 			player;
	renderView_t		view;

	FullscreenFXManager*	fxManager;

public:
	int					AddWarp( idVec3 worldOrigin, float centerx, float centery, float initialRadius, float durationMsec );
	void				FreeWarp( int id );
};

// the crosshair is swapped for a laser sight in stereo rendering
bool	IsGameStereoRendered();

#endif /* !__GAME_PLAYERVIEW_H__ */
