/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013 Robert Beckebans

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
#ifndef __SOUNDVOICE_H__
#define __SOUNDVOICE_H__

/*
================================================
idSoundVoice_Base
================================================
*/

class idSoundVoice_Base
{
public:
	idSoundVoice_Base();
	
	static void InitSurround( int outputChannels, int channelMask );
	
	void		CalculateSurround( int srcChannels, float pLevelMatrix[ MAX_CHANNELS_PER_VOICE * MAX_CHANNELS_PER_VOICE ], float scale );
	
	// RB begin
	virtual void	SetPosition( const idVec3& p )
	{
		position = p;
	}
	
	virtual void	SetGain( float g )
	{
		gain = g;
	}
	
	virtual void	SetPitch( float p )
	{
		pitch = p;
	}
	// RB end
	
	void		SetCenterChannel( float c )
	{
		centerChannel = c;
	}
	
	void		SetInnerRadius( float r )
	{
		innerRadius = r;
	}
	void		SetChannelMask( uint32 mask )
	{
		channelMask = mask;
	}
	
	const idSoundSample* GetCurrentSample();
	
	// Controls the low pass filter, where 0.0f = no filtering, 1.0f = full filter
	void		SetOcclusion( float f )
	{
		occlusion = f;
	}
	
	float		GetGain()
	{
		return gain;
	}
	float		GetPitch()
	{
		return pitch;
	}
	
protected:
	idVec3		position;			// Position of the sound relative to listener
	float		gain;				// Volume (0-1)
	float		centerChannel;		// Value (0-1) which indicates how much of this voice goes to the center channel
	float		pitch;				// Pitch multiplier
	float		innerRadius;		// Anything closer than this is omni
	float		occlusion;			// How much of this sound is occluded (0-1)
	uint32		channelMask;		// Set to override the default channel mask
	
	// These are some setting used to do SSF_DISTANCE_BASED_STERO blending
	float		innerSampleRangeSqr;
	float		outerSampleRangeSqr;
	
	idList< idSoundSample*, TAG_AUDIO> samples;
	
	// These are constants which are initialized with InitSurround
	//-------------------------------------------------------------
	
	static idVec2 speakerPositions[idWaveFile::CHANNEL_INDEX_MAX];
	
	// This is to figure out which speakers are "next to" this one
	static int speakerLeft[idWaveFile::CHANNEL_INDEX_MAX];
	static int speakerRight[idWaveFile::CHANNEL_INDEX_MAX];
	
	// Number of channels in the output hardware
	static int dstChannels;
	
	// Mask indicating which speakers exist in the hardware configuration
	static int dstMask;
	
	// dstMap maps a destination channel to a speaker
	// invMap maps a speaker to a destination channel
	static int dstCenter;
	static int dstLFE;
	static int dstMap[MAX_CHANNELS_PER_VOICE];
	static int invMap[idWaveFile::CHANNEL_INDEX_MAX];
	
	// specifies what volume to specify for each channel when a speaker is omni
	static float omniLevel;
};

#endif
