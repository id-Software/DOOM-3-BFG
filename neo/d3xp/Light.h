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

#ifndef __GAME_LIGHT_H__
#define __GAME_LIGHT_H__

/*
===============================================================================

  Generic light.

===============================================================================
*/

extern const idEventDef EV_Light_GetLightParm;
extern const idEventDef EV_Light_SetLightParm;
extern const idEventDef EV_Light_SetLightParms;

// jmarshall
struct rvmLightStyleState_t
{
	rvmLightStyleState_t();

	int				dl_frame;
	float			dl_framef;
	int				dl_oldframe;
	int				dl_time;
	float			dl_backlerp;

	void			Reset();
};

ID_INLINE rvmLightStyleState_t::rvmLightStyleState_t()
{
	Reset();
}

ID_INLINE void rvmLightStyleState_t::Reset()
{
	dl_frame = 0;
	dl_framef = 0;
	dl_oldframe = 0;
	dl_time = 0;
	dl_backlerp = 0;
}
// jmarshall end



class idLight : public idEntity
{
public:
	CLASS_PROTOTYPE( idLight );

	idLight();
	~idLight();

	void			Spawn();

	void			Save( idSaveGame* savefile ) const;					// archives object for save game file
	void			Restore( idRestoreGame* savefile );					// unarchives object from save game file

	virtual void	UpdateChangeableSpawnArgs( const idDict* source );
	virtual void	Think();
	virtual void	ClientThink( const int curTime, const float fraction, const bool predict );
	virtual void	FreeLightDef();
	virtual bool	GetPhysicsToSoundTransform( idVec3& origin, idMat3& axis );
	void			Present();
// jmarshall
	virtual void	SharedThink();
// jmarshall end
	void			SaveState( idDict* args );
	virtual void	SetColor( float red, float green, float blue );
	virtual void	SetColor( const idVec4& color );
	void			SetColor( const idVec3& color );
	virtual void	GetColor( idVec3& out ) const;
	virtual void	GetColor( idVec4& out ) const;
	const idVec3& 	GetBaseColor() const
	{
		return baseColor;
	}
	void			SetShader( const char* shadername );
	void			SetLightParm( int parmnum, float value );
	void			SetLightParms( float parm0, float parm1, float parm2, float parm3 );
	void			SetRadiusXYZ( float x, float y, float z );
	void			SetRadius( float radius );
	void			On();
	void			Off();
	void			Fade( const idVec4& to, float fadeTime );
	void			FadeOut( float time );
	void			FadeIn( float time );
	void			Killed( idEntity* inflictor, idEntity* attacker, int damage, const idVec3& dir, int location );
	void			BecomeBroken( idEntity* activator );
	qhandle_t		GetLightDefHandle() const
	{
		return lightDefHandle;
	}
	void			SetLightParent( idEntity* lparent )
	{
		lightParent = lparent;
	}
	void			SetLightLevel();

	virtual void	ShowEditingDialog();

	const renderLight_t& GetRenderLight() const
	{
		return renderLight;
	}

	enum
	{
		EVENT_BECOMEBROKEN = idEntity::EVENT_MAXEVENTS,
		EVENT_MAXEVENTS
	};

	virtual void	ClientPredictionThink();
	virtual void	WriteToSnapshot( idBitMsg& msg ) const;
	virtual void	ReadFromSnapshot( const idBitMsg& msg );
	virtual bool	ClientReceiveEvent( int event, int time, const idBitMsg& msg );

private:
	renderLight_t	renderLight;				// light presented to the renderer
	idVec3			localLightOrigin;			// light origin relative to the physics origin
	idMat3			localLightAxis;				// light axis relative to physics axis
	qhandle_t		lightDefHandle;				// handle to renderer light def
	idStr			brokenModel;
	int				levels;
	int				currentLevel;
	idVec3			baseColor;

	// Colors used for client-side interpolation.
	idVec3			previousBaseColor;
	idVec3			nextBaseColor;

	bool			breakOnTrigger;
	int				count;
// jmarshall
	int				lightStyle;
	int				lightStyleFrameTime;
	idVec3			lightStyleBase;
// jmarshall end
	int				triggercount;
	idEntity* 		lightParent;
	idVec4			fadeFrom;
	idVec4			fadeTo;
	int				fadeStart;
	int				fadeEnd;
	bool			soundWasPlaying;

private:
	void			PresentLightDefChange();
	void			PresentModelDefChange();

	void			Event_SetShader( const char* shadername );
	void			Event_GetLightParm( int parmnum );
	void			Event_SetLightParm( int parmnum, float value );
	void			Event_SetLightParms( float parm0, float parm1, float parm2, float parm3 );
	void			Event_SetRadiusXYZ( float x, float y, float z );
	void			Event_SetRadius( float radius );
	void			Event_Hide();
	void			Event_Show();
	void			Event_On();
	void			Event_Off();
	void			Event_ToggleOnOff( idEntity* activator );
	void			Event_SetSoundHandles();
	void			Event_FadeOut( float time );
	void			Event_FadeIn( float time );

// jmarshall
	idList<idStr>	light_styles;
	rvmLightStyleState_t lightStyleState;
// jmarshall end
};

#endif /* !__GAME_LIGHT_H__ */
