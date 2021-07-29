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
#include "precompiled.h"
#pragma hdrstop
#include "sys_voicechat.h"

/*
================================================
idVoiceChatMgr::Init
================================================
*/
void idVoiceChatMgr::Init( void* pXAudio2 )
{
}

/*
================================================
idVoiceChatMgr::Shutdown
================================================
*/
void idVoiceChatMgr::Shutdown()
{

	// We shouldn't have voice users if everything shutdown correctly
	assert( talkers.Num() == 0 );
	assert( remoteMachines.Num() == 0 );
}

/*
================================================
idVoiceChatMgr::RegisterTalker
================================================
*/
void idVoiceChatMgr::RegisterTalker( lobbyUser_t* user, int lobbyType, bool isLocal )
{

	int i = FindTalkerIndex( user, lobbyType );

	if( !verify( i == -1 ) )
	{
		assert( talkers[i].lobbyType == lobbyType );
		idLib::Printf( "RegisterTalker: Talker already registered.\n" );
		return;
	}

	// Talker not found, need to create a new one

	talker_t newTalker;

	newTalker.user				= user;
	newTalker.isLocal			= isLocal;
	newTalker.lobbyType			= lobbyType;
	newTalker.registered		= false;
	newTalker.registeredSuccess	= false;
	newTalker.machineIndex		= -1;
	newTalker.groupIndex		= 0;		// 0 is default group

	if( !newTalker.IsLocal() )  		// If this is a remote talker, register his machine address
	{
		newTalker.machineIndex = AddMachine( user->address, lobbyType );
	}

	talkers.Append( newTalker );

	// Since we added a new talker, make sure he is registered. UpdateRegisteredTalkers will catch all users, including this one.
	UpdateRegisteredTalkers();
}

/*
================================================
idVoiceChatMgr::UnregisterTalker
================================================
*/
void idVoiceChatMgr::UnregisterTalker( lobbyUser_t* user, int lobbyType, bool isLocal )
{
	int i = FindTalkerIndex( user, lobbyType );

	if( !verify( i != -1 ) )
	{
		idLib::Printf( "UnregisterTalker: Talker not found.\n" );
		return;
	}

	talker_t& talker = talkers[i];

	assert( talker.IsLocal() == ( talker.machineIndex == -1 ) );
	assert( talker.IsLocal() == isLocal );

	talker.lobbyType = -1;		// Mark for removal
	UpdateRegisteredTalkers();		// Make sure the user gets unregistered before we remove him/her

	if( talker.machineIndex != -1 )
	{
		// Unregister the talkers machine (unique address) handle
		RemoveMachine( talker.machineIndex, lobbyType );
	}

	talkers.RemoveIndex( i );	// Finally, remove the talker
}

/*
================================================
idVoiceChatMgr::GetActiveLocalTalkers
================================================
*/
void idVoiceChatMgr::GetActiveLocalTalkers( idStaticList< int, MAX_PLAYERS >& localTalkers )
{

	localTalkers.Clear();

	for( int i = 0; i < talkers.Num(); i++ )
	{

		if( !talkers[i].IsLocal() )
		{
			continue;
		}

		if( !talkers[i].registeredSuccess )
		{
			continue;
		}

		if( !TalkerHasData( i ) )
		{
			continue;
		}

		localTalkers.Append( i );
	}
}

/*
================================================
idVoiceChatMgr::GetRecipientsForTalker
================================================
*/
void idVoiceChatMgr::GetRecipientsForTalker( int talkerIndex, idStaticList< const lobbyAddress_t*, MAX_PLAYERS >& recipients )
{

	recipients.Clear();

	talker_t& talker = talkers[talkerIndex];

	if( !talker.IsLocal() )
	{
		return;
	}

	sendFrame++;

	for( int i = 0; i < talkers.Num(); i++ )
	{
		if( !talkers[i].registeredSuccess )
		{
			continue;
		}

		if( talkers[i].IsLocal() )
		{
			continue;		// Only want to send to remote talkers
		}

		if( !CanSendVoiceTo( talkerIndex, i ) )
		{
			continue;
		}

		if( !sendGlobal && talkers[i].groupIndex != activeGroupIndex )
		{
			continue;
		}

		assert( talkers[i].machineIndex >= 0 );

		remoteMachine_t& remoteMachine = remoteMachines[ talkers[i].machineIndex ];

		assert( remoteMachine.refCount > 0 );
		assert( remoteMachine.lobbyType == activeLobbyType );

		if( remoteMachine.sendFrame == sendFrame )
		{
			continue;		// Already on the recipient list
		}

		remoteMachine.sendFrame = sendFrame;

		recipients.Append( &remoteMachine.address );
	}
}

/*
================================================
idVoiceChatMgr::SetTalkerGroup
================================================
*/
void idVoiceChatMgr::SetTalkerGroup( const lobbyUser_t* user, int lobbyType, int groupIndex )
{
	int i = FindTalkerIndex( user, lobbyType );

	if( !verify( i != -1 ) )
	{
		idLib::Printf( "SetTalkerGroup: Talker not found.\n" );
		return;
	}

	// Assign the new group index to this talker
	talkers[i].groupIndex = groupIndex;

	// Since the group index of this player changed, call UpdateRegisteredTalkers, which will register the
	// appropriate users based on the current active group and session
	UpdateRegisteredTalkers();
}

/*
================================================
idVoiceChatMgr::SetActiveLobby
================================================
*/
void idVoiceChatMgr::SetActiveLobby( int lobbyType )
{
	if( activeLobbyType != lobbyType )
	{
		activeLobbyType = lobbyType;
		// When the active session changes, we need to immediately call UpdateRegisteredTalkers,
		// which will make sure the appropriate talkers are registered depending on the activeSession.
		UpdateRegisteredTalkers();
	}
}

/*
================================================
idVoiceChatMgr::SetActiveChatGroup
================================================
*/
void idVoiceChatMgr::SetActiveChatGroup( int groupIndex )
{
	if( activeGroupIndex != groupIndex )
	{
		activeGroupIndex = groupIndex;
		// When the active group changes, we need to immediately call UpdateRegisteredTalkers,
		// which will make sure the appropriate talkers are registered depending on the activeGroup.
		UpdateRegisteredTalkers();
	}
}

/*
================================================
idVoiceChatMgr::FindTalkerByUserId
================================================
*/
int idVoiceChatMgr::FindTalkerByUserId( lobbyUserID_t userID, int lobbyType )
{
	for( int i = 0; i < talkers.Num(); i++ )
	{
		if( talkers[i].user->lobbyUserID == userID && talkers[i].lobbyType == lobbyType )
		{
			return i;
		}
	}

	return -1;			// Not found
}

/*
================================================
idVoiceChatMgr::GetLocalChatData
================================================
*/
bool idVoiceChatMgr::GetLocalChatData( int talkerIndex, byte* data, int& dataSize )
{
	talker_t& talker = talkers[talkerIndex];

	if( !talker.IsLocal() )
	{
		idLib::Printf( "GetLocalChatData: Talker not local.\n" );
		return false;	// Talker is remote
	}

	if( !talker.registeredSuccess )
	{
		return false;
	}

	idBitMsg voiceMsg;
	voiceMsg.InitWrite( data, dataSize );

	talker.user->lobbyUserID.WriteToMsg( voiceMsg );
	voiceMsg.WriteByteAlign();

	// Remove the size of the userid field from the available buffer size
	int voiceDataSize = dataSize - voiceMsg.GetSize();

	if( !GetLocalChatDataInternal( talkerIndex, voiceMsg.GetWriteData() + voiceMsg.GetSize(), voiceDataSize ) )
	{
		dataSize = 0;
		return false;
	}

	dataSize = voiceDataSize + voiceMsg.GetSize();

	// Mark the user as talking
	talker.talking		= true;
	talker.talkingTime	= Sys_Milliseconds();

	return dataSize > 0 ? true : false;
}

/*
================================================
idVoiceChatMgr::SubmitIncomingChatData
================================================
*/
void idVoiceChatMgr::SubmitIncomingChatData( const byte* data, int dataSize )
{
	lobbyUserID_t lobbyUserID;

	idBitMsg voiceMsg;
	voiceMsg.InitRead( data, dataSize );

	lobbyUserID.ReadFromMsg( voiceMsg );
	voiceMsg.ReadByteAlign();

	int i = FindTalkerByUserId( lobbyUserID, activeLobbyType );

	if( i == -1 )
	{
		idLib::Printf( "SubmitIncomingChatData: Talker not found in session.\n" );
		return;	// Talker is not in this session
	}

	talker_t& talker = talkers[i];

	if( talker.registeredSuccess && !talker.isMuted )
	{
		// Mark the user as talking
		talker.talking		= true;
		talker.talkingTime	= Sys_Milliseconds();

		SubmitIncomingChatDataInternal( i, voiceMsg.GetReadData() + voiceMsg.GetReadCount(), voiceMsg.GetRemainingData() );
	}
}

/*
========================
idVoiceChatMgr::GetVoiceState
========================
*/
voiceState_t idVoiceChatMgr::GetVoiceState( const lobbyUser_t* user )
{
	int i = FindTalkerByUserId( user->lobbyUserID, activeLobbyType );

	if( i == -1 )
	{
		return VOICECHAT_STATE_NO_MIC;
	}

	talker_t& talker = talkers[i];

	if( !talker.hasHeadset )
	{
		return VOICECHAT_STATE_NO_MIC;
	}

	if( talker.isMuted )
	{
		return VOICECHAT_STATE_MUTED_LOCAL;
	}

	if( talker.talking && Sys_Milliseconds() - talker.talkingTime > 200 )
	{
		talker.talking = false;
	}

	return talker.talking ? ( talker.talkingGlobal ? VOICECHAT_STATE_TALKING_GLOBAL : VOICECHAT_STATE_TALKING ) : VOICECHAT_STATE_NOT_TALKING;
}

/*
========================
idVoiceChatMgr::CanSendVoiceTo
========================
*/
bool idVoiceChatMgr::CanSendVoiceTo( int talkerFromIndex, int talkerToIndex )
{
	talker_t& talkerFrom = talkers[talkerFromIndex];

	if( !talkerFrom.IsLocal() )
	{
		return false;
	}

	talker_t& talkerTo = talkers[talkerToIndex];

	if( talkerTo.isMuted )
	{
		return false;
	}

	return true;
}

/*
========================
idVoiceChatMgr::IsRestrictedByPrivleges
========================
*/
bool idVoiceChatMgr::IsRestrictedByPrivleges()
{
	return ( disableVoiceReasons & REASON_PRIVILEGES ) != 0;
}

/*
========================
idVoiceChatMgr::ToggleMuteLocal
========================
*/
void idVoiceChatMgr::ToggleMuteLocal( const lobbyUser_t* src, const lobbyUser_t* target )
{
	int fromTalkerIndex = FindTalkerByUserId( src->lobbyUserID, activeLobbyType );

	if( fromTalkerIndex == -1 )
	{
		return;
	}

	int toTalkerIndex = FindTalkerByUserId( target->lobbyUserID, activeLobbyType );

	if( toTalkerIndex == -1 )
	{
		return;
	}

	talker_t& targetTalker = talkers[toTalkerIndex];

	targetTalker.isMuted = targetTalker.isMuted ? false : true;
}

//================================================
//			**** INTERNAL **********
//================================================

/*
================================================
idVoiceChatMgr::FindTalkerIndex
================================================
*/
int idVoiceChatMgr::FindTalkerIndex( const lobbyUser_t* user, int lobbyType )
{
	for( int i = 0; i < talkers.Num(); i++ )
	{
		if( talkers[i].user == user && talkers[i].lobbyType == lobbyType )
		{
			return i;
		}
	}

	return -1;			// Not found
}

/*
================================================
idVoiceChatMgr::FindMachine
================================================
*/
int idVoiceChatMgr::FindMachine( const lobbyAddress_t& address, int lobbyType )
{
	for( int i = 0; i < remoteMachines.Num(); i++ )
	{
		if( remoteMachines[i].refCount == 0 )
		{
			continue;
		}
		if( remoteMachines[i].lobbyType == lobbyType && remoteMachines[i].address.Compare( address ) )
		{
			return i;
		}
	}
	return -1;	// Not found
}

/*
================================================
idVoiceChatMgr::AddMachine
================================================
*/
int idVoiceChatMgr::AddMachine( const lobbyAddress_t& address, int lobbyType )
{

	int machineIndex = FindMachine( address, lobbyType );

	if( machineIndex != -1 )
	{
		// If we find an existing machine, just increase the ref
		remoteMachines[machineIndex].refCount++;
		return machineIndex;
	}

	//
	// We didn't find a machine, we'll need to add one
	//

	// First, see if there is a free machine slot to take
	int index = -1;

	for( int i = 0; i < remoteMachines.Num(); i++ )
	{
		if( remoteMachines[i].refCount == 0 )
		{
			index = i;
			break;
		}
	}

	remoteMachine_t newMachine;

	newMachine.lobbyType	= lobbyType;
	newMachine.address		= address;
	newMachine.refCount		= 1;
	newMachine.sendFrame	= -1;

	if( index == -1 )
	{
		// If we didn't find a machine slot, then add one
		index = remoteMachines.Append( newMachine );
	}
	else
	{
		// Re-use the machine slot we found
		remoteMachines[index] = newMachine;
	}

	return index;
}

/*
================================================
idVoiceChatMgr::RemoveMachine
================================================
*/
void idVoiceChatMgr::RemoveMachine( int machineIndex, int lobbyType )
{

	assert( remoteMachines[machineIndex].refCount > 0 );
	assert( remoteMachines[machineIndex].lobbyType == lobbyType );

	// Don't remove the machine.  refCount will eventually reach 0, which will free up the slot to reclaim.
	// We don't want to remove it, because that would invalidate users machineIndex handles into the array
	remoteMachines[machineIndex].refCount--;
}

/*
================================================
idVoiceChatMgr::UpdateRegisteredTalkers
================================================
*/
void idVoiceChatMgr::UpdateRegisteredTalkers()
{
	for( int pass = 0; pass < 2; pass++ )
	{
		for( int i = 0; i < talkers.Num(); i++ )
		{
			talker_t& talker = talkers[i];

			bool shouldBeRegistered = ( talker.lobbyType != -1 && disableVoiceReasons == 0 && talker.lobbyType == activeLobbyType );

			if( shouldBeRegistered && pass == 0 )
			{
				continue;		// Only unregister on first pass to make room for when the second pass will possibly register new talkers
			}

			if( talker.registered != shouldBeRegistered )
			{
				if( !talker.registered )
				{
					talker.registeredSuccess = RegisterTalkerInternal( i );
				}
				else if( talker.registeredSuccess )
				{
					UnregisterTalkerInternal( i );
					talker.registeredSuccess = false;
				}

				talker.registered = shouldBeRegistered;
			}
		}
	}
}

/*
================================================
idVoiceChatMgr::SetDisableVoiceReason
================================================
*/
void idVoiceChatMgr::SetDisableVoiceReason( disableVoiceReason_t reason )
{
	if( ( disableVoiceReasons & reason ) == 0 )
	{
		disableVoiceReasons |= reason;
		UpdateRegisteredTalkers();
	}
}

/*
================================================
idVoiceChatMgr::ClearDisableVoiceReason
================================================
*/
void idVoiceChatMgr::ClearDisableVoiceReason( disableVoiceReason_t reason )
{
	if( ( disableVoiceReasons & reason ) != 0 )
	{
		disableVoiceReasons &= ~reason;
		UpdateRegisteredTalkers();
	}
}

/*
================================================
idVoiceChatMgr::SetHeadsetState
================================================
*/
void idVoiceChatMgr::SetHeadsetState( int talkerIndex, bool state )
{
	talker_t& talker = talkers[ talkerIndex ];

	talker.hasHeadset = state;
}

/*
================================================
idVoiceChatMgr::HasHeadsetStateChanged
================================================
*/
bool idVoiceChatMgr::HasHeadsetStateChanged( int talkerIndex )
{
	talker_t& talker = talkers[ talkerIndex ];

	// We should only be checking this on the local user
	assert( talker.IsLocal() );

	bool ret = talker.hasHeadsetChanged;
	talker.hasHeadsetChanged = false;

	return ret;
}