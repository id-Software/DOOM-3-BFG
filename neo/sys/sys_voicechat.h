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
#ifndef	__SYS_VOICECHATMGR_H__
#define	__SYS_VOICECHATMGR_H__

#include "sys_lobby_backend.h"

/*
================================================
idVoiceChatMgr
================================================
*/
class idVoiceChatMgr
{
public:
	idVoiceChatMgr() : activeLobbyType( -1 ), activeGroupIndex( 0 ), sendFrame( 0 ), disableVoiceReasons( 0 ), sendGlobal( false )  {}

	virtual void	Init( void* pXAudio2 );
	virtual void	Shutdown();

	void			RegisterTalker( lobbyUser_t* user, int lobbyType, bool isLocal );
	void			UnregisterTalker( lobbyUser_t* user, int lobbyType, bool isLocal );
	void			GetActiveLocalTalkers( idStaticList< int, MAX_PLAYERS >& localTalkers );
	void			GetRecipientsForTalker( int talkerIndex, idStaticList< const lobbyAddress_t*, MAX_PLAYERS >& recipients );

	void			SetTalkerGroup( const lobbyUser_t* user, int lobbyType, int groupIndex );

	void			SetActiveLobby( int lobbyType );
	void			SetActiveChatGroup( int groupIndex );
	int				FindTalkerByUserId( lobbyUserID_t lobbyUserID, int lobbyType );
	bool			GetLocalChatData( int talkerIndex, byte* data, int& dataSize );
	void			SubmitIncomingChatData( const byte* data, int dataSize );
	voiceState_t	GetVoiceState( const lobbyUser_t* user );
	bool			CanSendVoiceTo( int talkerFromIndex, int talkerToIndex );
	bool			IsRestrictedByPrivleges();

	void			SetHeadsetState( int talkerIndex, bool state );
	bool			GetHeadsetState( int talkerIndex ) const
	{
		return talkers[ talkerIndex ].hasHeadset;
	}
	bool			HasHeadsetStateChanged( int talkerIndex );

	enum disableVoiceReason_t
	{
		REASON_GENERIC				= BIT( 0 ),
		REASON_PRIVILEGES			= BIT( 1 ),
	};

	void			SetDisableVoiceReason( disableVoiceReason_t reason );
	void			ClearDisableVoiceReason( disableVoiceReason_t reason );

	virtual bool		GetLocalChatDataInternal( int talkerIndex, byte* data, int& dataSize ) = 0;
	virtual void		SubmitIncomingChatDataInternal( int talkerIndex, const byte* data, int dataSize ) = 0;
	virtual bool		TalkerHasData( int talkerIndex ) = 0;
	virtual void		Pump() {}
	virtual void		FlushBuffers() {}
	virtual void		ToggleMuteLocal( const lobbyUser_t* src, const lobbyUser_t* target );

protected:

	struct remoteMachine_t
	{
		int					lobbyType;
		lobbyAddress_t	address;
		int					refCount;
		int					sendFrame;
	};

	struct talker_t
	{
		talker_t() :
			user( NULL ),
			isLocal( false ),
			lobbyType( -1 ),
			groupIndex( -1 ),
			registered( false ),
			registeredSuccess( false ),
			machineIndex( -1 ),
			isMuted( false ),
			hasHeadset( true ),
			hasHeadsetChanged( false ),
			talking( false ),
			talkingGlobal( false ),
			talkingTime( 0 )
		{}

		lobbyUser_t* 	user;
		bool			isLocal;
		int				lobbyType;
		int				groupIndex;
		bool			registered;			// True if this user is currently registered with the XHV engine
		bool			registeredSuccess;	// True if this user is currently successfully registered with the XHV engine
		int				machineIndex;		// Index into remote machines array (-1 if this is a local talker)
		bool			isMuted;			// This machine is not allowed to hear or talk to this player
		bool			hasHeadset;			// This user has a headset connected
		bool			hasHeadsetChanged;	// This user's headset state has changed
		bool			talking;
		bool			talkingGlobal;
		int				talkingTime;

		bool IsLocal() const
		{
			return isLocal;
		}
	};

	virtual bool	RegisterTalkerInternal( int index ) = 0;
	virtual void	UnregisterTalkerInternal( int index ) = 0;

	int		FindTalkerIndex( const lobbyUser_t* user, int lobbyType );
	int		FindMachine( const lobbyAddress_t& address, int lobbyType );
	int		AddMachine( const lobbyAddress_t& address, int lobbyType );
	void	RemoveMachine( int machineIndex, int lobbyType );
	void	UpdateRegisteredTalkers();

	idStaticList< talker_t, MAX_PLAYERS * 2 >			talkers;			// * 2 to account for handling both session types
	idStaticList< remoteMachine_t, MAX_PLAYERS * 2 >	remoteMachines;		// * 2 to account for handling both session types

	int						activeLobbyType;
	int						activeGroupIndex;
	int						sendFrame;
	uint32					disableVoiceReasons;
	bool					sendGlobal;
};


#endif	// __SYS_VOICECHATMGR_H__ 