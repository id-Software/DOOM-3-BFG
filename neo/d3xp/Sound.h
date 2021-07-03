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

#ifndef __GAME_SOUND_H__
#define __GAME_SOUND_H__

/*
===============================================================================

  Generic sound emitter.

===============================================================================
*/

class idSound : public idEntity
{
public:
	CLASS_PROTOTYPE( idSound );

	idSound();

	void			Save( idSaveGame* savefile ) const;
	void			Restore( idRestoreGame* savefile );

	virtual void	UpdateChangeableSpawnArgs( const idDict* source );

	void			Spawn();

	void			ToggleOnOff( idEntity* other, idEntity* activator );
	void			Think();
	void			SetSound( const char* sound, int channel = SND_CHANNEL_ANY );

	virtual void	ShowEditingDialog();

private:
	float			lastSoundVol;
	float			soundVol;
	float			random;
	float			wait;
	bool			timerOn;
	idVec3			shakeTranslate;
	idAngles		shakeRotate;
	int				playingUntilTime;

	void			Event_Trigger( idEntity* activator );
	void			Event_Timer();
	void			Event_On();
	void			Event_Off();
	void			DoSound( bool play );
};

#endif /* !__GAME_SOUND_H__ */
