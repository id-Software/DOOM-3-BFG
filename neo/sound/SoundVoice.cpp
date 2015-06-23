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

#include "snd_local.h"

idCVar s_subFraction( "s_subFraction", "0.5", CVAR_ARCHIVE | CVAR_FLOAT, "Amount of each sound to send to the LFE channel" );

idVec2 idSoundVoice_Base::speakerPositions[idWaveFile::CHANNEL_INDEX_MAX];
int idSoundVoice_Base::speakerLeft[idWaveFile::CHANNEL_INDEX_MAX] = {0 };
int idSoundVoice_Base::speakerRight[idWaveFile::CHANNEL_INDEX_MAX] = {0 };
int idSoundVoice_Base::dstChannels = 0;
int idSoundVoice_Base::dstMask = 0;
int idSoundVoice_Base::dstCenter = -1;
int idSoundVoice_Base::dstLFE = -1;
int idSoundVoice_Base::dstMap[MAX_CHANNELS_PER_VOICE] = { 0 };
int idSoundVoice_Base::invMap[idWaveFile::CHANNEL_INDEX_MAX] = { 0 };
float idSoundVoice_Base::omniLevel = 1.0f;

/*
========================
idSoundVoice_Base::idSoundVoice_Base
========================
*/
idSoundVoice_Base::idSoundVoice_Base() :
	position( 0.0f ),
	gain( 1.0f ),
	centerChannel( 0.0f ),
	pitch( 1.0f ),
	innerRadius( 32.0f ),
	occlusion( 0.0f ),
	channelMask( 0 ),
	innerSampleRangeSqr( 0.0f ),
	outerSampleRangeSqr( 0.0f )
{
}

/*
========================
idSoundVoice_Base::InitSurround
========================
*/
void idSoundVoice_Base::InitSurround( int outputChannels, int channelMask )
{

	speakerPositions[idWaveFile::CHANNEL_INDEX_FRONT_LEFT			].Set( 0.70710678118654752440084436210485f,  0.70710678118654752440084436210485f );	// 45 degrees
	speakerPositions[idWaveFile::CHANNEL_INDEX_FRONT_RIGHT			].Set( 0.70710678118654752440084436210485f, -0.70710678118654752440084436210485f );	// 315 degrees
	speakerPositions[idWaveFile::CHANNEL_INDEX_FRONT_CENTER			].Set( 0.0f,								  0.0f );									// 0 degrees
	speakerPositions[idWaveFile::CHANNEL_INDEX_LOW_FREQUENCY		].Set( 0.0f,								  0.0f );									// -
	speakerPositions[idWaveFile::CHANNEL_INDEX_BACK_LEFT			].Set( -0.70710678118654752440084436210485f,  0.70710678118654752440084436210485f );	// 135 degrees
	speakerPositions[idWaveFile::CHANNEL_INDEX_BACK_RIGHT			].Set( -0.70710678118654752440084436210485f, -0.70710678118654752440084436210485f );	// 225 degrees
	speakerPositions[idWaveFile::CHANNEL_INDEX_FRONT_LEFT_CENTER	].Set( 0.92387953251128675612818318939679f,  0.3826834323650897717284599840304f );		// 22.5 degrees
	speakerPositions[idWaveFile::CHANNEL_INDEX_FRONT_RIGHT_CENTER	].Set( 0.92387953251128675612818318939679f, -0.3826834323650897717284599840304f );		// 337.5 degrees
	speakerPositions[idWaveFile::CHANNEL_INDEX_BACK_CENTER			].Set( -1.0f,								  0.0f );									// 180 degrees
	speakerPositions[idWaveFile::CHANNEL_INDEX_SIDE_LEFT			].Set( 0.0f,								  1.0f );									// 90 degrees
	speakerPositions[idWaveFile::CHANNEL_INDEX_SIDE_RIGHT			].Set( 0.0f,								 -1.0f );									// 270 degrees
	
	speakerLeft[idWaveFile::CHANNEL_INDEX_FRONT_LEFT_CENTER] = idWaveFile::CHANNEL_INDEX_FRONT_LEFT;
	speakerLeft[idWaveFile::CHANNEL_INDEX_FRONT_LEFT] = idWaveFile::CHANNEL_INDEX_SIDE_LEFT;
	speakerLeft[idWaveFile::CHANNEL_INDEX_SIDE_LEFT] = idWaveFile::CHANNEL_INDEX_BACK_LEFT;
	speakerLeft[idWaveFile::CHANNEL_INDEX_BACK_LEFT] = idWaveFile::CHANNEL_INDEX_BACK_CENTER;
	speakerLeft[idWaveFile::CHANNEL_INDEX_BACK_CENTER] = idWaveFile::CHANNEL_INDEX_BACK_RIGHT;
	speakerLeft[idWaveFile::CHANNEL_INDEX_BACK_RIGHT] = idWaveFile::CHANNEL_INDEX_SIDE_RIGHT;
	speakerLeft[idWaveFile::CHANNEL_INDEX_SIDE_RIGHT] = idWaveFile::CHANNEL_INDEX_FRONT_RIGHT;
	speakerLeft[idWaveFile::CHANNEL_INDEX_FRONT_RIGHT] = idWaveFile::CHANNEL_INDEX_FRONT_RIGHT_CENTER;
	speakerLeft[idWaveFile::CHANNEL_INDEX_FRONT_RIGHT_CENTER] = idWaveFile::CHANNEL_INDEX_FRONT_LEFT_CENTER;
	
	speakerLeft[idWaveFile::CHANNEL_INDEX_FRONT_CENTER] = idWaveFile::CHANNEL_INDEX_FRONT_CENTER;
	speakerLeft[idWaveFile::CHANNEL_INDEX_LOW_FREQUENCY] = idWaveFile::CHANNEL_INDEX_LOW_FREQUENCY;
	
	speakerRight[idWaveFile::CHANNEL_INDEX_FRONT_RIGHT_CENTER] = idWaveFile::CHANNEL_INDEX_FRONT_RIGHT;
	speakerRight[idWaveFile::CHANNEL_INDEX_FRONT_RIGHT] = idWaveFile::CHANNEL_INDEX_SIDE_RIGHT;
	speakerRight[idWaveFile::CHANNEL_INDEX_SIDE_RIGHT] = idWaveFile::CHANNEL_INDEX_BACK_RIGHT;
	speakerRight[idWaveFile::CHANNEL_INDEX_BACK_RIGHT] = idWaveFile::CHANNEL_INDEX_BACK_CENTER;
	speakerRight[idWaveFile::CHANNEL_INDEX_BACK_CENTER] = idWaveFile::CHANNEL_INDEX_BACK_LEFT;
	speakerRight[idWaveFile::CHANNEL_INDEX_BACK_LEFT] = idWaveFile::CHANNEL_INDEX_SIDE_LEFT;
	speakerRight[idWaveFile::CHANNEL_INDEX_SIDE_LEFT] = idWaveFile::CHANNEL_INDEX_FRONT_LEFT;
	speakerRight[idWaveFile::CHANNEL_INDEX_FRONT_LEFT] = idWaveFile::CHANNEL_INDEX_FRONT_LEFT_CENTER;
	speakerRight[idWaveFile::CHANNEL_INDEX_FRONT_LEFT_CENTER] = idWaveFile::CHANNEL_INDEX_FRONT_RIGHT_CENTER;
	
	speakerRight[idWaveFile::CHANNEL_INDEX_FRONT_CENTER] = idWaveFile::CHANNEL_INDEX_FRONT_CENTER;
	speakerRight[idWaveFile::CHANNEL_INDEX_LOW_FREQUENCY] = idWaveFile::CHANNEL_INDEX_LOW_FREQUENCY;
	
	dstChannels = outputChannels;
	dstMask = channelMask;
	
	// dstMap maps a destination channel to a speaker
	// invMap maps a speaker to a destination channel
	dstLFE = -1;
	dstCenter = -1;
	memset( dstMap, 0, sizeof( dstMap ) );
	memset( invMap, 0, sizeof( invMap ) );
	for( int i = 0, c = 0; i < idWaveFile::CHANNEL_INDEX_MAX && c < MAX_CHANNELS_PER_VOICE; i++ )
	{
		if( dstMask & BIT( i ) )
		{
			if( i == idWaveFile::CHANNEL_INDEX_LOW_FREQUENCY )
			{
				dstLFE = c;
			}
			if( i == idWaveFile::CHANNEL_INDEX_FRONT_CENTER )
			{
				dstCenter = c;
			}
			dstMap[c] = i;
			invMap[i] = c++;
		}
		else
		{
			// Remove this speaker from the chain
			int right = speakerRight[i];
			int left = speakerLeft[i];
			speakerRight[left] = right;
			speakerLeft[right] = left;
		}
	}
	assert( ( dstLFE == -1 ) || ( ( dstMask & idWaveFile::CHANNEL_MASK_LOW_FREQUENCY ) != 0 ) );
	assert( ( dstCenter == -1 ) || ( ( dstMask & idWaveFile::CHANNEL_MASK_FRONT_CENTER ) != 0 ) );
	
	float omniChannels = ( float )dstChannels;
	if( dstMask & idWaveFile::CHANNEL_MASK_LOW_FREQUENCY )
	{
		omniChannels -= 1.0f;
	}
	if( dstMask & idWaveFile::CHANNEL_MASK_FRONT_CENTER )
	{
		omniChannels -= 1.0f;
	}
	if( omniChannels > 0.0f )
	{
		omniLevel = 1.0f / omniChannels;
	}
	else
	{
		// This happens in mono mode
		omniLevel = 1.0f;
	}
}

/*
========================
idSoundVoice_Base::CalculateSurround
========================
*/
void idSoundVoice_Base::CalculateSurround( int srcChannels, float pLevelMatrix[ MAX_CHANNELS_PER_VOICE * MAX_CHANNELS_PER_VOICE ], float scale )
{
	// Hack for mono
	if( dstChannels == 1 )
	{
		if( srcChannels == 1 )
		{
			pLevelMatrix[ 0 ] = scale;
		}
		else if( srcChannels == 2 )
		{
			pLevelMatrix[ 0 ] = scale * 0.7071f;
			pLevelMatrix[ 1 ] = scale * 0.7071f;
		}
		return;
	}
	
#define MATINDEX( src, dst ) ( srcChannels * dst + src )
	
	float subFraction = s_subFraction.GetFloat();
	
	if( srcChannels == 1 )
	{
		idVec2 p2 = position.ToVec2();
		
		float centerFraction = centerChannel;
		
		float sqrLength = p2.LengthSqr();
		if( sqrLength <= 0.01f )
		{
			// If we are on top of the listener, simply route all channels to each speaker equally
			for( int i = 0; i < dstChannels; i++ )
			{
				pLevelMatrix[MATINDEX( 0, i )] = omniLevel;
			}
		}
		else
		{
			float invLength = idMath::InvSqrt( sqrLength );
			float distance = ( invLength * sqrLength );
			p2 *= invLength;
			
			float spatialize = 1.0f;
			if( distance < innerRadius )
			{
				spatialize = distance / innerRadius;
			}
			float omni = omniLevel * ( 1.0f - spatialize );
			
			if( dstCenter != -1 )
			{
				centerFraction *= Max( 0.0f, p2.x );
				spatialize *= ( 1.0f - centerFraction );
				omni *= ( 1.0f - centerFraction );
			}
			
			float channelDots[MAX_CHANNELS_PER_VOICE] = { 0 };
			for( int i = 0; i < dstChannels; i++ )
			{
				// Calculate the contribution to each destination channel
				channelDots[i] = speakerPositions[dstMap[i]] * p2;
			}
			// Find the speaker nearest to the sound
			int channelA = 0;
			for( int i = 1; i < dstChannels; i++ )
			{
				if( channelDots[i] > channelDots[channelA] )
				{
					channelA = i;
				}
			}
			int speakerA = dstMap[channelA];
			
			// Find the 2nd nearest speaker
			int speakerB;
			float speakerACross = ( speakerPositions[speakerA].x * p2.y ) - ( speakerPositions[speakerA].y * p2.x );
			if( speakerACross > 0.0f )
			{
				speakerB = speakerLeft[speakerA];
			}
			else
			{
				speakerB = speakerRight[speakerA];
			}
			int channelB = invMap[speakerB];
			
			// Divide the amplitude between the 2 closest speakers
			float distA = ( speakerPositions[speakerA] - p2 ).Length();
			float distB = ( speakerPositions[speakerB] - p2 ).Length();
			float distCinv = 1.0f / ( distA + distB );
			float volumes[MAX_CHANNELS_PER_VOICE] = { 0 };
			volumes[channelA] = ( distB * distCinv );
			volumes[channelB] = ( distA * distCinv );
			for( int i = 0; i < dstChannels; i++ )
			{
				pLevelMatrix[MATINDEX( 0, i )] = ( volumes[i] * spatialize ) + omni;
			}
		}
		if( dstLFE != -1 )
		{
			pLevelMatrix[MATINDEX( 0, dstLFE )] = subFraction;
		}
		if( dstCenter != -1 )
		{
			pLevelMatrix[MATINDEX( 0, dstCenter )] = centerFraction;
		}
	}
	else if( srcChannels == 2 )
	{
		pLevelMatrix[ MATINDEX( 0, 0 ) ] = 1.0f;
		pLevelMatrix[ MATINDEX( 1, 1 ) ] = 1.0f;
		if( dstLFE != -1 )
		{
			pLevelMatrix[ MATINDEX( 0, dstLFE ) ] = subFraction * 0.5f;
			pLevelMatrix[ MATINDEX( 1, dstLFE ) ] = subFraction * 0.5f;
		}
	}
	else
	{
		idLib::Warning( "We don't support %d channel sound files", srcChannels );
	}
	for( int i = 0; i < srcChannels * dstChannels; i++ )
	{
		pLevelMatrix[ i ] *= scale;
	}
}
