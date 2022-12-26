/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2014 Carl Kenner

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

#ifndef __CINEMATIC_H__
#define __CINEMATIC_H__

/*
===============================================================================

	cinematic

	Multiple idCinematics can run simultaniously.
	A single idCinematic can be reused for multiple files if desired.

===============================================================================
*/

// cinematic states
typedef enum
{
	FMV_IDLE,
	FMV_PLAY,			// play
	FMV_EOF,			// all other conditions, i.e. stop/EOF/abort
	FMV_ID_BLT,
	FMV_ID_IDLE,
	FMV_LOOPED,
	FMV_ID_WAIT
} cinStatus_t;

class idImage;

// a cinematic stream generates an image buffer, which the caller will upload to a texture
typedef struct
{
	int					imageWidth;
	int					imageHeight;	// will be a power of 2
	idImage*			imageY;
	idImage*			imageCr;
	idImage*			imageCb;
	idImage*			image;
	int					status;
} cinData_t;

class idCinematic
{
public:
	// initialize cinematic play back data
	static void			InitCinematic();

	// shutdown cinematic play back data
	static void			ShutdownCinematic();

	// allocates and returns a private subclass that implements the methods
	// This should be used instead of new
	static idCinematic*	Alloc();

	// frees all allocated memory
	virtual				~idCinematic();

	// returns false if it failed to load
	virtual bool		InitFromFile( const char* qpath, bool looping, nvrhi::ICommandList* commandList );

	// returns the length of the animation in milliseconds
	virtual int			AnimationLength();

	// RB: let us know wether this video went EOF or is still active
	virtual bool        IsPlaying() const;
	// RB end

	// the pointers in cinData_t will remain valid until the next UpdateForTime() call
	virtual cinData_t	ImageForTime( int milliseconds, nvrhi::ICommandList* commandList );

	// closes the file and frees all allocated memory
	virtual void		Close();

	// sets the cinematic to start at that time (can be in the past)
	virtual void		ResetTime( int time );

	// gets the time the cinematic started
	virtual int			GetStartTime();

	virtual void		ExportToTGA( bool skipExisting = true );

	virtual float		GetFrameRate() const;
};

/*
===============================================

	Sound meter.

===============================================
*/

class idSndWindow : public idCinematic
{
public:

	idSndWindow()
	{
		showWaveform = false;
	}
	~idSndWindow() {}

	bool				InitFromFile( const char* qpath, bool looping );
	cinData_t			ImageForTime( int milliseconds );
	int					AnimationLength();

private:
	bool				showWaveform;
};

#endif /* !__CINEMATIC_H__ */
