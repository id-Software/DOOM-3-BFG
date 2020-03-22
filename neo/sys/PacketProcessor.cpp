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

#pragma hdrstop
#include "precompiled.h"
#include "PacketProcessor.h"

// DG: workaround for GCC bug
const int idPacketProcessor::RETURN_TYPE_NONE = 0;
const int idPacketProcessor::RETURN_TYPE_OOB = 1;
const int idPacketProcessor::RETURN_TYPE_INBAND = 2;

const int idPacketProcessor::FRAGMENT_START = 0;
const int idPacketProcessor::FRAGMENT_MIDDLE = 1;
const int idPacketProcessor::FRAGMENT_END = 2;
// DG end

idCVar net_maxRate( "net_maxRate", "50", CVAR_INTEGER, "max send rate in kilobytes per second" );

idCVar net_showReliableCompression( "net_showReliableCompression", "0", CVAR_BOOL, "Show reliable compression ratio." );

// we use an assert(0); return idiom in some places, which lint complains about
//lint -e527	unreachable code at token 'return'

/*
================================================
idPacketProcessor::QueueReliableAck
================================================
*/
void idPacketProcessor::QueueReliableAck( int lastReliable )
{
	// NOTE - Even if it was the last known sequence, go ahead and ack it, in case our last ack for this sequence got dropped
	if( lastReliable >= reliableSequenceRecv )
	{
		queuedReliableAck		= lastReliable;
		reliableSequenceRecv	= lastReliable;
	}
}

/*
================================================
idPacketProcessor::FinalizeRead
================================================
*/
int idPacketProcessor::FinalizeRead( idBitMsg& inMsg, idBitMsg& outMsg, int& userValue )
{
	userValue = 0;

	idInnerPacketHeader header;
	header.ReadFromMsg( inMsg );

	if( !verify( header.Type() != PACKET_TYPE_FRAGMENTED ) )  		// We shouldn't be fragmented at this point
	{
		idLib::Printf( "Received invalid fragmented packet.\n" );
		return RETURN_TYPE_NONE;
	}

	if( header.Type() == PACKET_TYPE_RELIABLE_ACK )
	{
		// Handle reliable ack
		int reliableSequence = inMsg.ReadLong();
		reliable.RemoveOlderThan( reliableSequence + 1 );
		header.ReadFromMsg( inMsg );								// Read the new header, since the reliable ack sits on top the actual header of the message
	}

	if( header.Type() == PACKET_TYPE_OOB )
	{
		// out-of-band packet
		userValue = header.Value();
	}
	else
	{
		// At this point, this MUST be an in-band packet
		if( !verify( header.Type() == PACKET_TYPE_INBAND ) )
		{
			idLib::Printf( "In-band packet expected, received type %i instead.\n", header.Type() );
			return RETURN_TYPE_NONE;
		}

		// Reset number of reliables received (NOTE - This means you MUST unload all reliables as they are received)
		numReliable = 0;

		// Handle reliable portion of in-band packets
		int numReliableRecv = header.Value();
		int bufferPos = 0;

		if( numReliableRecv > 0 )
		{
			// Byte align msg
			inMsg.ReadByteAlign();

			int compressedSize = inMsg.ReadShort();

			lzwCompressionData_t	lzwData;
			idLZWCompressor			lzwCompressor( &lzwData );

			lzwCompressor.Start( ( uint8* )inMsg.GetReadData() + inMsg.GetReadCount(), compressedSize );		// Read from msg

			int reliableSequence = 0;

			lzwCompressor.ReadAgnostic< int >( reliableSequence );

			for( int r = 0; r < numReliableRecv; r++ )
			{
				uint8 uncompMem[ MAX_MSG_SIZE ];

				uint16 reliableDataLength = 0;
				lzwCompressor.ReadAgnostic< uint16 >( reliableDataLength );
				lzwCompressor.Read( uncompMem, reliableDataLength );

				if( reliableSequence + r > reliableSequenceRecv )  		// Only accept newer reliable msg's than we've currently already received
				{
					if( !verify( bufferPos + reliableDataLength <= sizeof( reliableBuffer ) ) )
					{
						idLib::Printf( "Reliable msg size overflow.\n" );
						return RETURN_TYPE_NONE;
					}
					if( !verify( numReliable < MAX_RELIABLE_QUEUE ) )
					{
						idLib::Printf( "Reliable msg count overflow.\n" );
						return RETURN_TYPE_NONE;
					}
					memcpy( reliableBuffer + bufferPos, uncompMem, reliableDataLength );
					reliableMsgSize[ numReliable ] = reliableDataLength;
					reliableMsgPtrs[ numReliable++ ] = &reliableBuffer[ bufferPos ];
					bufferPos += reliableDataLength;
				}
				else
				{
					extern idCVar net_verboseReliable;
					if( net_verboseReliable.GetBool() )
					{
						idLib::Printf( "Ignoring reliable msg %i because %i was already acked\n", ( reliableSequence + r ), reliableSequenceRecv );
					}
				}

				if( !verify( lzwCompressor.IsOverflowed() == false ) )
				{
					idLib::Printf( "lzwCompressor.IsOverflowed() == true.\n" );
					return RETURN_TYPE_NONE;
				}
			}

			inMsg.SetReadCount( inMsg.GetReadCount() + compressedSize );

			QueueReliableAck( reliableSequence + numReliableRecv - 1 );
		}
	}

	// Load actual msg
	outMsg.BeginWriting();
	outMsg.WriteData( inMsg.GetReadData() + inMsg.GetReadCount(), inMsg.GetRemainingData() );
	outMsg.SetSize( inMsg.GetRemainingData() );

	return ( header.Type() == PACKET_TYPE_OOB ) ? RETURN_TYPE_OOB : RETURN_TYPE_INBAND;
}

/*
================================================
idPacketProcessor::QueueReliableMessage
================================================
*/
bool idPacketProcessor::QueueReliableMessage( byte type, const byte* data, int dataLen )
{
	return reliable.Append( reliableSequenceSend++, &type, 1, data, dataLen );
}

/*
========================
idPacketProcessor::CanSendMoreData
========================
*/
bool idPacketProcessor::CanSendMoreData() const
{
	if( net_maxRate.GetInteger() == 0 )
	{
		return true;
	}

	return ( outgoingRateBytes <= net_maxRate.GetInteger() * 1024 );
}

/*
========================
idPacketProcessor::UpdateOutgoingRate
========================
*/
void idPacketProcessor::UpdateOutgoingRate( const int time, const int size )
{
	outgoingBytes += size;

	// update outgoing rate variables
	if( time > outgoingRateTime )
	{
		outgoingRateBytes -= outgoingRateBytes * ( float )( time - outgoingRateTime ) / 1000.0f;
		if( outgoingRateBytes < 0.0f )
		{
			outgoingRateBytes = 0.0f;
		}
	}

	outgoingRateTime = time;
	outgoingRateBytes += size;

	// compute an average bandwidth at intervals
	if( time - lastOutgoingRateTime > BANDWIDTH_AVERAGE_PERIOD )
	{
		currentOutgoingRate = 1000 * ( outgoingBytes - lastOutgoingBytes ) / ( time - lastOutgoingRateTime );
		lastOutgoingBytes = outgoingBytes;
		lastOutgoingRateTime = time;
	}
}

/*
=================
idPacketProcessor::UpdateIncomingRate
=================
*/
void idPacketProcessor::UpdateIncomingRate( const int time, const int size )
{
	incomingBytes += size;

	// update incoming rate variables
	if( time > incomingRateTime )
	{
		incomingRateBytes -= incomingRateBytes * ( float )( time - incomingRateTime ) / 1000.0f;
		if( incomingRateBytes < 0.0f )
		{
			incomingRateBytes = 0.0f;
		}
	}
	incomingRateTime = time;
	incomingRateBytes += size;

	// compute an average bandwidth at intervals
	if( time - lastIncomingRateTime > BANDWIDTH_AVERAGE_PERIOD )
	{
		currentIncomingRate = 1000 * ( incomingBytes - lastIncomingBytes ) / ( time - lastIncomingRateTime );
		lastIncomingBytes = incomingBytes;
		lastIncomingRateTime = time;
	}
}

/*
================================================
idPacketProcessor::ProcessOutgoing
NOTE - We only compress reliables because we assume everything else has already been compressed.
================================================
*/
bool idPacketProcessor::ProcessOutgoing( const int time, const idBitMsg& msg, bool isOOB, int userData )
{
	// We can only do ONE ProcessOutgoing call, then we need to do GetSendFragment to
	// COMPLETELY empty unsentMsg before calling ProcessOutgoing again.
	if( !verify( fragmentedSend == false ) )
	{
		idLib::Warning( "ProcessOutgoing: fragmentedSend == true!" );
		return false;
	}

	if( !verify( unsentMsg.GetRemainingData() == 0 ) )
	{
		idLib::Warning( "ProcessOutgoing: unsentMsg.GetRemainingData() > 0!" );
		return false;
	}

	// Build the full msg to send, which could include reliable data
	unsentMsg.InitWrite( unsentBuffer, sizeof( unsentBuffer ) );
	unsentMsg.BeginWriting();

	// Ack reliables if we need to (NOTE - We will send this ack on both the in-band and out-of-band channels)
	if( queuedReliableAck >= 0 )
	{
		idInnerPacketHeader header( PACKET_TYPE_RELIABLE_ACK, 0 );
		header.WriteToMsg( unsentMsg );
		unsentMsg.WriteLong( queuedReliableAck );
		queuedReliableAck = -1;
	}

	if( isOOB )
	{
		if( msg.GetSize() + unsentMsg.GetSize() > MAX_OOB_MSG_SIZE )  		// Fragmentation not allowed for out-of-band msg's
		{
			idLib::Printf( "Out-of-band packet too large %i\n", unsentMsg.GetSize() );
			assert( 0 );
			return false;
		}
		// We don't need to worry about reliable for out of band packets
		idInnerPacketHeader header( PACKET_TYPE_OOB, userData );
		header.WriteToMsg( unsentMsg );
	}
	else
	{
		// Add reliable msg's here if this is an in-band packet
		idInnerPacketHeader header( PACKET_TYPE_INBAND, reliable.Num() );
		header.WriteToMsg( unsentMsg );
		if( reliable.Num() > 0 )
		{
			// Byte align unsentMsg
			unsentMsg.WriteByteAlign();

			lzwCompressionData_t	lzwData;
			idLZWCompressor			lzwCompressor( &lzwData );

			lzwCompressor.Start( unsentMsg.GetWriteData() + unsentMsg.GetSize() + 2, unsentMsg.GetRemainingSpace() - 2 );		// Write to compressed mem, not exceeding MAX_MSG_SIZE (+2 to reserve space for compressed size)

			int uncompressedSize = 4;
			lzwCompressor.WriteAgnostic< int >( reliable.ItemSequence( 0 ) );
			for( int i = 0; i < reliable.Num(); i++ )
			{
				lzwCompressor.WriteAgnostic< uint16 >( reliable.ItemLength( i ) );
				lzwCompressor.Write( reliable.ItemData( i ), reliable.ItemLength( i ) );
				uncompressedSize += 2 + reliable.ItemLength( i );
			}

			lzwCompressor.End();

			if( lzwCompressor.IsOverflowed() )
			{
				idLib::Error( "reliable msg compressor overflow." );
			}

			unsentMsg.WriteShort( lzwCompressor.Length() );
			unsentMsg.SetSize( unsentMsg.GetSize() + lzwCompressor.Length() );

			if( net_showReliableCompression.GetBool() )
			{
				static int totalUncompressed = 0;
				static int totalCompressed = 0;

				totalUncompressed += uncompressedSize;
				totalCompressed += lzwCompressor.Length();

				float ratio1 = ( float )lzwCompressor.Length() / ( float )uncompressedSize;
				float ratio2 = ( float )totalCompressed / ( float )totalUncompressed;

				idLib::Printf( "Uncompressed: %i, Compressed: %i, TotalUncompressed: %i, TotalCompressed: %i, (%2.2f / %2.2f )\n", uncompressedSize, lzwCompressor.Length(), totalUncompressed, totalCompressed, ratio1, ratio2 );
			}
		}
	}

	// Fill up with actual msg
	unsentMsg.WriteData( msg.GetReadData(), msg.GetSize() );

	if( unsentMsg.GetSize() > MAX_PACKET_SIZE )
	{
		if( isOOB )
		{
			idLib::Error( "oob msg's cannot fragment" );
		}
		fragmentedSend = true;
	}

	return true;
}

/*
================================================
idPacketProcessor::GetSendFragment
================================================
*/
bool idPacketProcessor::GetSendFragment( const int time, sessionId_t sessionID, idBitMsg& outMsg )
{
	lastSendTime = time;

	if( unsentMsg.GetRemainingData() <= 0 )
	{
		return false;	// Nothing to send
	}

	outMsg.BeginWriting();


	idOuterPacketHeader	outerHeader( sessionID );

	// Write outer packet header to the msg
	outerHeader.WriteToMsg( outMsg );

	if( !fragmentedSend )
	{
		// Simple case, no fragments to sent
		outMsg.WriteData( unsentMsg.GetReadData(), unsentMsg.GetSize() );
		unsentMsg.SetSize( 0 );
	}
	else
	{
		int currentSize = idMath::ClampInt( 0, MAX_PACKET_SIZE, unsentMsg.GetRemainingData() );
		assert( currentSize > 0 );
		assert( unsentMsg.GetRemainingData() - currentSize >= 0 );

		// See if we'll have more fragments once we subtract off how much we're about to write
		bool moreFragments = ( unsentMsg.GetRemainingData() - currentSize > 0 ) ? true : false;

		if( !unsentMsg.GetReadCount() )  		// If this is the first read, then we know it's the first fragment
		{
			assert( moreFragments );			// If we have a first, we must have more or something went wrong
			idInnerPacketHeader header( PACKET_TYPE_FRAGMENTED, FRAGMENT_START );
			header.WriteToMsg( outMsg );
		}
		else
		{
			idInnerPacketHeader header( PACKET_TYPE_FRAGMENTED, moreFragments ? FRAGMENT_MIDDLE : FRAGMENT_END );
			header.WriteToMsg( outMsg );
		}

		outMsg.WriteLong( fragmentSequence );
		outMsg.WriteData( unsentMsg.GetReadData() + unsentMsg.GetReadCount(), currentSize );
		unsentMsg.ReadData( NULL, currentSize );

		assert( moreFragments == unsentMsg.GetRemainingData() > 0 );
		fragmentedSend = moreFragments;

		fragmentSequence++;				// Advance sequence

		fragmentAccumulator++;			// update the counter for the net debug hud
	}


	// The caller needs to send this packet, so assume he did, and update rates
	UpdateOutgoingRate( time, outMsg.GetSize() );

	return true;
}

/*
================================================
idPacketProcessor::ProcessIncoming
================================================
*/
int idPacketProcessor::ProcessIncoming( int time, sessionId_t expectedSessionID, idBitMsg& msg, idBitMsg& out, int& userData, const int peerNum )
{
	assert( msg.GetSize() <= MAX_FINAL_PACKET_SIZE );

	UpdateIncomingRate( time, msg.GetSize() );


	idOuterPacketHeader outerHeader;
	outerHeader.ReadFromMsg( msg );

	sessionId_t sessionID = outerHeader.GetSessionID();
	assert( sessionID == expectedSessionID );

	if( !verify( sessionID != SESSION_ID_CONNECTIONLESS_PARTY && sessionID != SESSION_ID_CONNECTIONLESS_GAME && sessionID != SESSION_ID_CONNECTIONLESS_GAME_STATE ) )
	{
		idLib::Printf( "Expected non connectionless ID, but got a connectionless one\n" );
		return RETURN_TYPE_NONE;
	}

	if( sessionID != expectedSessionID )
	{
		idLib::Printf( "Expected session id: %8x but got %8x instead\n", expectedSessionID, sessionID );
		return RETURN_TYPE_NONE;
	}

	int c, b;
	msg.SaveReadState( c, b );

	idInnerPacketHeader header;
	header.ReadFromMsg( msg );

	if( header.Type() != PACKET_TYPE_FRAGMENTED )
	{
		// Non fragmented
		msg.RestoreReadState( c, b );		// Reset since we took a byte to check the type
		return FinalizeRead( msg, out, userData );
	}

	// Decode fragmented packet
	int readSequence = msg.ReadLong();	// Read sequence of fragment

	if( header.Value() == FRAGMENT_START )
	{
		msgWritePos = 0;				// Reset msg reconstruction write pos
	}
	else if( fragmentSequence == -1 || readSequence != fragmentSequence + 1 )
	{
		droppedFrags++;
		idLib::Printf( "Dropped Fragments - PeerNum: %i FragmentSeq: %i, ReadSeq: %i, Total: %i\n", peerNum, fragmentSequence, readSequence, droppedFrags );

		// If this is the middle or end, make sure we are reading in fragmentSequence
		fragmentSequence = -1;
		return RETURN_TYPE_NONE;		// Out of sequence
	}
	fragmentSequence = readSequence;
	assert( msg.GetRemainingData() > 0 );

	if( !verify( msgWritePos + msg.GetRemainingData() < sizeof( msgBuffer ) ) )
	{
		idLib::Error( "ProcessIncoming: Fragmented msg buffer overflow." );
	}

	memcpy( msgBuffer + msgWritePos, msg.GetReadData() + msg.GetReadCount(), msg.GetRemainingData() );
	msgWritePos += msg.GetRemainingData();

	if( header.Value() == FRAGMENT_END )
	{
		// Done reconstructing the msg
		idBitMsg msg( msgBuffer, sizeof( msgBuffer ) );
		msg.SetSize( msgWritePos );
		return FinalizeRead( msg, out, userData );
	}

	if( !verify( header.Value() == FRAGMENT_START || header.Value() == FRAGMENT_MIDDLE ) )
	{
		idLib::Printf( "ProcessIncoming: Invalid packet.\n" );
	}

	// If we get here, this is part (either beginning or end) of a fragmented packet.
	// We return RETURN_TYPE_NONE to let the caller know they don't need to do anything yet.
	return RETURN_TYPE_NONE;
}

/*
================================================
idPacketProcessor::ProcessConnectionlessOutgoing
================================================
*/
bool idPacketProcessor::ProcessConnectionlessOutgoing( idBitMsg& msg, idBitMsg& out, int lobbyType, int userData )
{
	sessionId_t sessionID = lobbyType + 1;


	// Write outer header
	idOuterPacketHeader outerHeader( sessionID );
	outerHeader.WriteToMsg( out );

	// Write inner header
	idInnerPacketHeader header( PACKET_TYPE_OOB, userData );
	header.WriteToMsg( out );

	// Write msg
	out.WriteData( msg.GetReadData(), msg.GetSize() );


	return true;
}

/*
================================================
idPacketProcessor::ProcessConnectionlessIncoming
================================================
*/
bool idPacketProcessor::ProcessConnectionlessIncoming( idBitMsg& msg, idBitMsg& out, int& userData )
{

	idOuterPacketHeader outerHeader;
	outerHeader.ReadFromMsg( msg );

	sessionId_t sessionID = outerHeader.GetSessionID();

	if( sessionID != SESSION_ID_CONNECTIONLESS_PARTY && sessionID != SESSION_ID_CONNECTIONLESS_GAME && sessionID != SESSION_ID_CONNECTIONLESS_GAME_STATE )
	{
		// Not a connectionless msg (this can happen if a previously connected peer keeps sending data for whatever reason)
		idLib::Printf( "ProcessConnectionlessIncoming: Invalid session ID - %d\n", sessionID );
		return false;
	}

	idInnerPacketHeader header;
	header.ReadFromMsg( msg );

	if( header.Type() != PACKET_TYPE_OOB )
	{
		idLib::Printf( "ProcessConnectionlessIncoming: header.Type() != PACKET_TYPE_OOB\n" );
		return false;		// Only out-of-band packets supported for connectionless
	}

	userData = header.Value();

	out.BeginWriting();
	out.WriteData( msg.GetReadData() + msg.GetReadCount(), msg.GetRemainingData() );
	out.SetSize( msg.GetRemainingData() );

	return true;
}

/*
================================================
idPacketProcessor::GetSessionID
================================================
*/
idPacketProcessor::sessionId_t idPacketProcessor::GetSessionID( idBitMsg& msg )
{
	sessionId_t sessionID;
	int c, b;
	msg.SaveReadState( c, b );
	// Read outer header
	idOuterPacketHeader outerHeader;
	outerHeader.ReadFromMsg( msg );

	// Get session ID
	sessionID = outerHeader.GetSessionID();

	msg.RestoreReadState( c, b );
	return sessionID;
}

/*
================================================
idPacketProcessor::VerifyEmptyReliableQueue
================================================
*/
idCVar net_verifyReliableQueue( "net_verifyReliableQueue", "2", CVAR_INTEGER, "0: warn only, 1: error, 2: fixup, 3: fixup and verbose, 4: force test" );
#define RELIABLE_VERBOSE if ( net_verifyReliableQueue.GetInteger() >= 3 ) idLib::Printf
void idPacketProcessor::VerifyEmptyReliableQueue( byte keepMsgBelowThis, byte replaceWithThisMsg )
{
	if( net_verifyReliableQueue.GetInteger() == 4 )
	{
		RELIABLE_VERBOSE( "pushing a fake game reliable\n" );
		const char* garbage = "garbage";
		QueueReliableMessage( keepMsgBelowThis + 4, ( const byte* )garbage, 8 );
		QueueReliableMessage( replaceWithThisMsg, NULL, 0 );
	}
	if( reliable.Num() == 0 )
	{
		return;
	}
	if( net_verifyReliableQueue.GetInteger() == 1 )
	{
		idLib::Error( "reliable queue is not empty: %d messages", reliable.Num() );
		return;
	}
	idLib::Warning( "reliable queue is not empty: %d messages", reliable.Num() );
	if( net_verifyReliableQueue.GetInteger() == 0 )
	{
		return;
	}
	// drop some stuff that is potentially dangerous and should not transmit
	idDataQueue< MAX_RELIABLE_QUEUE, MAX_MSG_SIZE > clean;
	RELIABLE_VERBOSE( "rollback send sequence from %d to %d\n", reliableSequenceSend, reliable.ItemSequence( 0 ) );
	for( int i = 0; i < reliable.Num(); i++ )
	{
		byte peek = reliable.ItemData( i )[0];
		if( peek < keepMsgBelowThis )
		{
			RELIABLE_VERBOSE( "keeping %d\n", peek );
			clean.Append( reliable.ItemSequence( i ), reliable.ItemData( i ), reliable.ItemLength( i ) );
		}
		else
		{
			// Replace with fake msg, so we retain itemsequence ordering.
			// If we don't do this, it's possible we remove the last msg, then append a single msg before the next send,
			// and the client may think he already received the msg, since his last reliableSequenceRecv could be greater than our
			// reliableSequenceSend if he already received the group of reliables we are mucking with
			clean.Append( reliable.ItemSequence( i ), &replaceWithThisMsg, 1 );
			RELIABLE_VERBOSE( "dropping %d\n", peek );
		}
	}

	assert( reliable.Num() == clean.Num() );

	reliable = clean;
}
