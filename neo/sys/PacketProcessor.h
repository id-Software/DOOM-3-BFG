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
#ifndef __PACKET_PROCESSOR_H__
#define __PACKET_PROCESSOR_H__

/*
================================================
idPacketProcessor
================================================
*/
class idPacketProcessor
{
public:
	// DG: workaround for GCC bug (can't link when compiling with -O0): put definitions in PacketProcessor.cpp
	static const int RETURN_TYPE_NONE; //			= 0;
	static const int RETURN_TYPE_OOB; //			= 1;
	static const int RETURN_TYPE_INBAND; //			= 2;
	// DG end
	typedef uint16				sessionId_t;

	static const int NUM_LOBBY_TYPE_BITS		= 2;
	static const int LOBBY_TYPE_MASK			= ( 1 << NUM_LOBBY_TYPE_BITS ) - 1;

	static const sessionId_t SESSION_ID_INVALID						= 0;
	static const sessionId_t SESSION_ID_CONNECTIONLESS_PARTY		= 1;
	static const sessionId_t SESSION_ID_CONNECTIONLESS_GAME			= 2;
	static const sessionId_t SESSION_ID_CONNECTIONLESS_GAME_STATE	= 3;

	static const int BANDWIDTH_AVERAGE_PERIOD						= 250;

	idPacketProcessor()
	{
		Reset();
	}

	void Reset()
	{
		msgWritePos				= 0;
		fragmentSequence		= 0;
		droppedFrags			= 0;
		fragmentedSend			= false;

		reliable = idDataQueue< MAX_RELIABLE_QUEUE, MAX_MSG_SIZE >();

		reliableSequenceSend	= 1;
		reliableSequenceRecv	= 0;

		numReliable				= 0;

		queuedReliableAck		= -1;

		unsentMsg = idBitMsg();

		lastSendTime			= 0;

		outgoingRateTime		= 0;
		outgoingRateBytes		= 0.0f;
		incomingRateTime		= 0;
		incomingRateBytes		= 0.0f;

		outgoingBytes			= 0;
		incomingBytes			= 0;

		currentOutgoingRate		= 0;
		lastOutgoingRateTime	= 0;
		lastOutgoingBytes		= 0;
		currentIncomingRate		= 0;
		lastIncomingRateTime	= 0;
		lastIncomingBytes		= 0;

		fragmentAccumulator		= 0;


	}

	static const int MAX_MSG_SIZE			= 8000;							// This is the max size you can pass into ProcessOutgoing
	static const int MAX_FINAL_PACKET_SIZE	= 1200;							// Lowest/safe MTU across all our platforms to avoid fragmentation at the transport layer (which is poorly supported by consumer hardware and may cause nasty latency side effects)
	static const int MAX_RELIABLE_QUEUE		= 64;

	// TypeInfo doesn't like sizeof( sessionId_t )?? and then fails to understand the #ifdef/#else/#endif
	//static const int MAX_PACKET_SIZE		= MAX_FINAL_PACKET_SIZE - 6 - sizeof( sessionId_t );	// Largest possible packet before headers and such applied (subtract some for various internal header data, and session id)
	static const int MAX_PACKET_SIZE		= MAX_FINAL_PACKET_SIZE - 6 - 2;			// Largest possible packet before headers and such applied (subtract some for various internal header data, and session id)
	static const int MAX_OOB_MSG_SIZE		= MAX_PACKET_SIZE - 1;			// We don't allow fragmentation for out-of-band msg's, and we need a byte for the header

private:
	void QueueReliableAck( int lastReliable );
	int FinalizeRead( idBitMsg& inMsg, idBitMsg& outMsg, int& userValue );

public:
	bool CanSendMoreData() const;
	void UpdateOutgoingRate( const int time, const int size );
	void UpdateIncomingRate( const int time, const int size );

	void RefreshRates( int time )
	{
		UpdateOutgoingRate( time, 0 );
		UpdateIncomingRate( time, 0 );
	}

	// Used to queue reliable msg's, to be sent on the next ProcessOutgoing
	bool QueueReliableMessage( byte type, const byte* data, int dataLen );
	// Used to process a msg ready to be sent, could get fragmented into multiple fragments
	bool ProcessOutgoing( const int time, const idBitMsg& msg, bool isOOB, int userData );
	// Used to get each fragment for sending through the actual net connection
	bool GetSendFragment( const int time, sessionId_t sessionID, idBitMsg& outMsg );
	// Used to process a fragment received.  Returns true when msg was reconstructed.
	int ProcessIncoming( int time, sessionId_t expectedSessionID, idBitMsg& msg, idBitMsg& out, int& userData, const int peerNum );

	// Returns true if there are more fragments to send
	bool HasMoreFragments() const
	{
		return ( unsentMsg.GetRemainingData() > 0 );
	}

	// Num reliables not ack'd
	int NumQueuedReliables()
	{
		return reliable.Num();
	}
	// True if we need to send a reliable ack
	int NeedToSendReliableAck()
	{
		return queuedReliableAck >= 0 ? true : false;
	}

	// Used for out-of-band non connected peers
	// This doesn't actually support fragmentation, it is just simply here to hide the
	// header structure, so the caller doesn't have to skip over the header data.
	static bool ProcessConnectionlessOutgoing( idBitMsg& msg, idBitMsg& out, int lobbyType, int userData );
	static bool ProcessConnectionlessIncoming( idBitMsg& msg, idBitMsg& out, int& userData );

	// Used to "peek" at a session id of a message fragment
	static sessionId_t GetSessionID( idBitMsg& msg );

	int				GetNumReliables() const
	{
		return numReliable;
	}
	const byte* 	GetReliable( int i ) const
	{
		return reliableMsgPtrs[ i ];
	}
	int				GetReliableSize( int i ) const
	{
		return reliableMsgSize[ i ];
	}

	void			SetLastSendTime( int i )
	{
		lastSendTime = i;
	}
	int				GetLastSendTime() const
	{
		return lastSendTime;
	}
	float			GetOutgoingRateBytes() const
	{
		return outgoingRateBytes;
	}
	int				GetOutgoingBytes() const
	{
		return outgoingBytes;
	}
	float			GetIncomingRateBytes() const
	{
		return incomingRateBytes;
	}
	int				GetIncomingBytes() const
	{
		return incomingBytes;
	}

	// more reliable computation, based on a suitably small interval
	int				GetOutgoingRate2() const
	{
		return currentOutgoingRate;
	}
	int				GetIncomingRate2() const
	{
		return currentIncomingRate;
	}
	// decrease a fragmentation counter, so we reflect how much we're maxing the MTU
	bool			TickFragmentAccumulator()
	{
		if( fragmentAccumulator > 0 )
		{
			fragmentAccumulator--;
			return true;
		}
		return false;
	}

	int				GetReliableDataSize() const
	{
		return reliable.GetDataLength();
	}

	void			VerifyEmptyReliableQueue( byte keepMsgBelowThis, byte replaceWithThisMsg );

private:

	// Packet header types
	static const int PACKET_TYPE_INBAND			= 0;	// In-band. Number of reliable msg's stored in userData portion of header
	static const int PACKET_TYPE_OOB			= 1;	// Out-of-band. userData free to use by the caller. Cannot fragment.
	static const int PACKET_TYPE_RELIABLE_ACK   = 2;	// Header type used to piggy-back on top of msgs to ack reliable msg's
	static const int PACKET_TYPE_FRAGMENTED		= 3;	// The msg is fragmented, fragment type stored in the userData portion of header


	// PACKET_TYPE_FRAGMENTED userData values
	// DG: workaround for GCC bug (can't link when compiling with -O0): put definitions in PacketProcessor.cpp
	static const int FRAGMENT_START; //				= 0;
	static const int FRAGMENT_MIDDLE; //			= 1;
	static const int FRAGMENT_END;	//			= 2;
	// DG end

	class idOuterPacketHeader
	{
	public:
		idOuterPacketHeader() : sessionID( SESSION_ID_INVALID ) {}
		idOuterPacketHeader( sessionId_t sessionID_ ) : sessionID( sessionID_ ) {}

		void WriteToMsg( idBitMsg& msg )
		{
			msg.WriteUShort( sessionID );
		}

		void ReadFromMsg( idBitMsg& msg )
		{
			sessionID = msg.ReadUShort();
		}

		sessionId_t GetSessionID()
		{
			return sessionID;
		}
	private:
		sessionId_t	sessionID;
	};

	class idInnerPacketHeader
	{
	public:
		idInnerPacketHeader() : type( 0 ), userData( 0 ) {}
		idInnerPacketHeader( int inType, int inData ) : type( inType ), userData( inData ) {}

		void WriteToMsg( idBitMsg& msg )
		{
			msg.WriteBits( type, 2 );
			msg.WriteBits( userData, 6 );
		}

		void ReadFromMsg( idBitMsg& msg )
		{
			type = msg.ReadBits( 2 );
			userData = msg.ReadBits( 6 );
		}

		int Type()
		{
			return type;
		}
		int Value()
		{
			return userData;
		}

	private:
		int			type;
		int			userData;
	};

	byte			msgBuffer[ MAX_MSG_SIZE ];					// Buffer used to reconstruct the msg
	int				msgWritePos;								// Write position into the msg reconstruction buffer
	int				fragmentSequence;							// Fragment sequence number
	int				droppedFrags;								// Number of dropped fragments
	bool			fragmentedSend;								// Used to determine if the current send requires fragmenting

	idDataQueue< MAX_RELIABLE_QUEUE, MAX_MSG_SIZE >	reliable;	// list of unacknowledged reliable messages

	int				reliableSequenceSend;						// sequence number of the next reliable packet we're going to send to this peer
	int				reliableSequenceRecv;						// sequence number of the last reliable packet we received from this peer

	// These are for receiving reliables, you need to get these before the next process call or they will get cleared
	int				numReliable;
	byte			reliableBuffer[ MAX_MSG_SIZE ];				// We shouldn't have to hold more than this
	const byte* 	reliableMsgPtrs[ MAX_RELIABLE_QUEUE ];
	int				reliableMsgSize[ MAX_RELIABLE_QUEUE ];

	int				queuedReliableAck;							// Used to piggy back on the next send to ack reliables

	idBitMsg		unsentMsg;
	byte			unsentBuffer[ MAX_MSG_SIZE ];				// Buffer used hold the current msg until it's all sent

	int				lastSendTime;

	// variables to keep track of the rate
	int				outgoingRateTime;
	float			outgoingRateBytes;		// B/S
	int				incomingRateTime;
	float			incomingRateBytes;		// B/S

	int				outgoingBytes;
	int				incomingBytes;

	int				currentOutgoingRate;
	int				lastOutgoingRateTime;
	int				lastOutgoingBytes;
	int				currentIncomingRate;
	int				lastIncomingRateTime;
	int				lastIncomingBytes;


	int				fragmentAccumulator;	// counts max size packets we are sending for the net debug hud
};

#endif /* !__PACKET_PROCESSOR_H__ */
