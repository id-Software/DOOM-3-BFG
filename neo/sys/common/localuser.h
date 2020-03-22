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
#ifndef __WIN_LOCALUSER_H__
#define __WIN_LOCALUSER_H__

// This is to quickly get/set the data needed for disc-swapping
typedef struct
{
	int					inputDevice;
} winUserState_t;

/*
================================================
idLocalUserWin
================================================
*/
class idLocalUserWin : public idLocalUser
{
public:
	static const int MAX_GAMERTAG = 64;			// max number of bytes for a gamertag
	static const int MAX_GAMERTAG_CHARS = 16;	// max number of UTF-8 characters to show

	idLocalUserWin() : inputDevice( 0 ) {}

	idLocalUserWin& operator=( idLocalUserWin&& other )
	{
		gamertag = std::move( other.gamertag );
		inputDevice = other.inputDevice;
		return *this;
	}

	//==========================================================================================
	// idLocalUser interface
	//==========================================================================================
	virtual bool				IsProfileReady() const;
	virtual bool				IsOnline() const;
	virtual bool				IsInParty() const;
	virtual int					GetPartyCount() const;
	virtual uint32				GetOnlineCaps() const
	{
		return ( IsPersistent() && IsOnline() ) ? ( CAP_IS_ONLINE | CAP_CAN_PLAY_ONLINE ) : 0;
	}
	virtual int					GetInputDevice() const
	{
		return inputDevice;
	}
	virtual const char* 		GetGamerTag() const
	{
		return gamertag.c_str();
	}
	virtual void				PumpPlatform() {}

	//==========================================================================================
	// idLocalUserWin interface
	//==========================================================================================
	void						SetInputDevice( int inputDevice_ )
	{
		inputDevice = inputDevice_;
	}
	void						SetGamerTag( const char* gamerTag_ )
	{
		gamertag = gamerTag_;
	}
	winUserState_t				GetUserState()
	{
		winUserState_t a = { inputDevice };
		return a;
	}
	bool						VerifyUserState( winUserState_t& state );

	void						Init( int inputDevice_, const char* gamertag_, int numLocalUsers );

private:
	idStrStatic< MAX_GAMERTAG >	gamertag;
	int							inputDevice;
};

#endif // __WIN_LOCALUSER_H__
