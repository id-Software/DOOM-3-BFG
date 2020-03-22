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
#ifndef	__SYS_LOBBY_BACKEND_DIRECT_H__
#define	__SYS_LOBBY_BACKEND_DIRECT_H__

/*
========================
idLobbyBackendDirect
========================
*/
class idLobbyBackendDirect : public idLobbyBackend
{
public:
	idLobbyBackendDirect();

	// idLobbyBackend interface
	virtual void				StartHosting( const idMatchParameters& p, float skillLevel, lobbyBackendType_t type );
	virtual void				StartFinding( const idMatchParameters& p, int numPartyUsers, float skillLevel );
	virtual void				JoinFromConnectInfo( const lobbyConnectInfo_t& connectInfo );
	virtual void				GetSearchResults( idList< lobbyConnectInfo_t >& searchResults );
	virtual void				FillMsgWithPostConnectInfo( idBitMsg& msg ) {}
	virtual void				PostConnectFromMsg( idBitMsg& msg ) {}
	virtual void				Shutdown();
	virtual void				GetOwnerAddress( lobbyAddress_t& outAddr );
	virtual void				SetIsJoinable( bool joinable );
	virtual lobbyConnectInfo_t	GetConnectInfo();
	virtual bool				IsOwnerOfConnectInfo( const lobbyConnectInfo_t& connectInfo ) const;
	virtual void				Pump();
	virtual void				UpdateMatchParms( const idMatchParameters& p );
	virtual void				UpdateLobbySkill( float lobbySkill );
	virtual void				SetInGame( bool value );
	virtual lobbyBackendState_t	GetState()
	{
		return state;
	}

	virtual void				BecomeHost( int numInvites );
	virtual void				FinishBecomeHost();

	virtual void			RegisterUser( lobbyUser_t* user, bool isLocal );
	virtual void			UnregisterUser( lobbyUser_t* user, bool isLocal );

private:

	lobbyBackendState_t		state;
	netadr_t				address;
};

#endif	// __SYS_LOBBY_BACKEND_DIRECT_H__ 
