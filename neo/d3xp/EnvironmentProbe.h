/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2015 Robert Beckebans

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

#ifndef __GAME_ENVIRONMENTPROBE_H__
#define __GAME_ENVIRONMENTPROBE_H__

/*
===============================================================================

  Environment probe for Image Based Lighting (part of PBR).

===============================================================================
*/

class EnvironmentProbe : public idEntity
{
public:
	CLASS_PROTOTYPE( EnvironmentProbe );

	EnvironmentProbe();
	~EnvironmentProbe();

	void			Spawn();

	void			Save( idSaveGame* savefile ) const;					// archives object for save game file
	void			Restore( idRestoreGame* savefile );					// unarchives object from save game file

	virtual void	UpdateChangeableSpawnArgs( const idDict* source );
	virtual void	Think();
	virtual void	ClientThink( const int curTime, const float fraction, const bool predict );
	virtual void	FreeEnvprobeDef();
	void			Present();

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
	void			SetEnvprobeParm( int parmnum, float value );
	void			SetEnvprobeParms( float parm0, float parm1, float parm2, float parm3 );
	void			On();
	void			Off();
	void			Fade( const idVec4& to, float fadeTime );
	void			FadeOut( float time );
	void			FadeIn( float time );

	qhandle_t		GetEnvprobeDefHandle() const
	{
		return envprobeDefHandle;
	}

	void			SetEnvprobeParent( idEntity* lparent )
	{
		lightParent = lparent;
	}

	void			SetLightLevel();

	virtual void	ShowEditingDialog();

	enum
	{
		EVENT_BECOMEBROKEN = idEntity::EVENT_MAXEVENTS,
		EVENT_MAXEVENTS
	};

	virtual void	ClientPredictionThink();
	virtual void	WriteToSnapshot( idBitMsg& msg ) const;
	virtual void	ReadFromSnapshot( const idBitMsg& msg );
//	virtual bool	ClientReceiveEvent( int event, int time, const idBitMsg& msg );

private:
	renderEnvironmentProbe_t	renderEnvprobe;		// envprobe presented to the renderer
	idVec3			localEnvprobeOrigin;			// light origin relative to the physics origin
	idMat3			localEnvprobeAxis;				// light axis relative to physics axis
	qhandle_t		envprobeDefHandle;				// handle to renderer light def
	int				levels;
	int				currentLevel;
	idVec3			baseColor;

	// Colors used for client-side interpolation.
	idVec3			previousBaseColor;
	idVec3			nextBaseColor;

	int				count;
	int				triggercount;
	idEntity* 		lightParent;
	idVec4			fadeFrom;
	idVec4			fadeTo;
	int				fadeStart;
	int				fadeEnd;

private:
	void			PresentEnvprobeDefChange();

	void			Event_GetEnvprobeParm( int parmnum );
	void			Event_SetEnvprobeParm( int parmnum, float value );
	void			Event_SetEnvprobeParms( float parm0, float parm1, float parm2, float parm3 );
	void			Event_SetRadiusXYZ( float x, float y, float z );
	void			Event_SetRadius( float radius );
	void			Event_Hide();
	void			Event_Show();
	void			Event_On();
	void			Event_Off();
	void			Event_ToggleOnOff( idEntity* activator );
	void			Event_FadeOut( float time );
	void			Event_FadeIn( float time );
};

#endif /* !__GAME_ENVIRONMENTPROBE_H__ */
