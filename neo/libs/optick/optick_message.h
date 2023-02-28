// The MIT License(MIT)
//
// Copyright(c) 2019 Vadim Slyusarev
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once
#include "optick.config.h"

#if USE_OPTICK

#include "optick_common.h"
#include "optick_serialization.h"

namespace Optick
{
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static const uint32 NETWORK_PROTOCOL_VERSION = 26;
static const uint16 NETWORK_APPLICATION_ID = 0xB50F;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct DataResponse
{
	enum Type : uint16
	{
		FrameDescriptionBoard = 0,		// DescriptionBoard for Instrumental Frames
		EventFrame = 1,					// Instrumental Data
		SamplingFrame = 2,				// Sampling Data
		NullFrame = 3,					// Last Fame Mark
		ReportProgress = 4,				// Report Current Progress
		Handshake = 5,					// Handshake Response
		Reserved_0 = 6,					
		SynchronizationData = 7,		// Synchronization Data for the thread
		TagsPack = 8,					// Pack of tags
		CallstackDescriptionBoard = 9,	// DescriptionBoard with resolved function addresses
		CallstackPack = 10,				// Pack of CallStacks
		Reserved_1 = 11,				
		Reserved_2 = 12,				
		Reserved_3 = 13,				
		Reserved_4 = 14,				
		//...
		Reserved_255 = 255,

		FiberSynchronizationData = 1 << 8, // Synchronization Data for the Fibers
		SyscallPack,
		SummaryPack,
		FramesPack,
	};

	uint32 version;
	uint32 size;
	Type type;
	uint16 application;

	DataResponse(Type t, uint32 s) : version(NETWORK_PROTOCOL_VERSION), size(s), type(t), application(NETWORK_APPLICATION_ID){}
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OutputDataStream& operator << (OutputDataStream& os, const DataResponse& val);
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class IMessage
{
public:
	enum Type : uint16
	{
		Start,
		Stop,
		Cancel,
		TurnSampling,
		COUNT,
	};

	virtual void Apply() = 0;
	virtual ~IMessage() {}

	static IMessage* Create( InputDataStream& str );
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<IMessage::Type MESSAGE_TYPE>
class Message : public IMessage
{
	enum { id = MESSAGE_TYPE };
public:
	static uint32 GetMessageType() { return id; }
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct CaptureSettings
{
	// Capture Mode
	uint32 mode;
	// Category Filter
	uint32 categoryMask;
	// Tracer: Sampling Frequency
	uint32 samplingFrequency;
	// Max Duration for a capture (frames)
	uint32 frameLimit;
	// Max Duration for a capture (us)
	uint32 timeLimitUs;
	// Max Duration for a capture (us)
	uint32 spikeLimitUs;
	// Max Memory for a capture (MB)
	uint64 memoryLimitMb;
	// Tracer: Root Password for the Device
	string password;

	CaptureSettings() : mode(0), categoryMask(0), samplingFrequency(0), frameLimit(0), timeLimitUs(0), spikeLimitUs(0), memoryLimitMb(0) {}
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct StartMessage : public Message<IMessage::Start>
{
	CaptureSettings settings;
	static IMessage* Create(InputDataStream&);
	virtual void Apply() override;
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct StopMessage : public Message<IMessage::Stop>
{
	static IMessage* Create(InputDataStream&);
	virtual void Apply() override;
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct CancelMessage : public Message<IMessage::Cancel>
{
	static IMessage* Create(InputDataStream&);
	virtual void Apply() override;
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct TurnSamplingMessage : public Message<IMessage::TurnSampling>
{
	int32 index;
	byte isSampling;

	static IMessage* Create(InputDataStream& stream);
	virtual void Apply() override;
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}

#endif //USE_OPTICK