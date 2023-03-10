/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013-2014 Robert Beckebans
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

#include "precompiled.h"
#pragma hdrstop
#if defined(_MSC_VER) && !defined(USE_OPENAL)
	#include <sound/XAudio2/XA2_CinematicAudio.h>
#else
	#include <sound/OpenAL/AL_CinematicAudio.h>
#endif


// SRS - Add cvar to control whether cinematic audio is played: default is ON
idCVar s_playCinematicAudio( "s_playCinematicAudio", "1", CVAR_BOOL, "Play audio if available in cinematic video files" );

//extern "C" {
#include <jpeglib.h>
//}

#include "RenderCommon.h"

#define CIN_system	1
#define CIN_loop	2
#define	CIN_hold	4
#define CIN_silent	8
#define CIN_shader	16

#if defined(USE_FFMPEG)
// Carl: ffmpg for bink video files
extern "C"
{

//#ifdef WIN32
#ifndef INT64_C
#define INT64_C(c) (c ## LL)
#define UINT64_C(c) (c ## ULL)
#endif
//#include <inttypes.h>
//#endif

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/imgutils.h>
}
#define NUM_LAG_FRAMES 15	// SRS - Lag audio by 15 frames (~1/2 sec at 30 fps) for ffmpeg bik decoder AV sync
#endif

#ifdef USE_BINKDEC
	// DG: not sure how to use FFMPEG and BINKDEC at the same time.. it might be useful if someone wants to
	//     use binkdec for bink and FFMPEG for other formats in custom code so I didn't just rip FFMPEG out
	//     But right now it's unsupported, if you need this adjust the video loading code etc yourself
	#ifdef USE_FFMPEG
		#error "Currently, only one of FFMPEG and BINKDEC is supported at a time!"
	#endif

	#include <BinkDecoder.h>
#endif // USE_BINKDEC

class idCinematicLocal : public idCinematic
{
public:
	idCinematicLocal();
	virtual					~idCinematicLocal();

	virtual bool			InitFromFile( const char* qpath, bool looping, nvrhi::ICommandList* commandList );
	virtual cinData_t		ImageForTime( int milliseconds, nvrhi::ICommandList* commandList );
	virtual int				AnimationLength();
	// RB begin
	bool                    IsPlaying() const;
	// RB end
	virtual void			Close();
	// SRS begin
	virtual int             GetStartTime();
	// SRS end
	virtual void			ResetTime( int time );

private:

#if defined(USE_FFMPEG)
	int						video_stream_index;
	int						audio_stream_index; //GK: Make extra indexer for audio
	AVFormatContext*		fmt_ctx;
	AVFrame*				frame;
	AVFrame*				frame2;
	AVFrame*				frame3; //GK: make extra frame for audio
#if LIBAVCODEC_VERSION_MAJOR > 58
	const AVCodec*			dec;
	const AVCodec*			dec2;	// SRS - Separate decoder for audio
#else
	AVCodec*				dec;
	AVCodec*				dec2;	// SRS - Separate decoder for audio
#endif
	AVCodecContext*			dec_ctx;
	AVCodecContext*			dec_ctx2;
	SwsContext*				img_convert_ctx;
	bool					hasFrame;
	long					framePos;
	AVSampleFormat			dst_smp;
	bool					hasplanar;
	SwrContext*				swr_ctx;
	cinData_t				ImageForTimeFFMPEG( int milliseconds, nvrhi::ICommandList* commandList );
	bool					InitFromFFMPEGFile( const char* qpath, bool looping, nvrhi::ICommandList* commandList );
	void					FFMPEGReset();
	uint8_t*				lagBuffer[NUM_LAG_FRAMES] = {};
	int						lagBufSize[NUM_LAG_FRAMES] = {};
	int						lagIndex;
	bool					skipLag;
#endif
#ifdef USE_BINKDEC
	BinkHandle				binkHandle;
	cinData_t				ImageForTimeBinkDec( int milliseconds, nvrhi::ICommandList* commandList );
	bool					InitFromBinkDecFile( const char* qpath, bool looping, nvrhi::ICommandList* commandList );
	void					BinkDecReset();

	YUVbuffer				yuvBuffer;
	bool                    hasFrame;
	int						framePos;
	int						numFrames;
	idImage*				imgY;
	idImage*				imgCr;
	idImage*				imgCb;
	uint32_t				audioTracks;
	uint32_t				trackIndex;
	AudioInfo				binkInfo;
#endif
	idImage*				img;
	bool					isRoQ;

	// RB: 64 bit fixes, changed long to int
	size_t					mcomp[256];
	// RB end
	byte** 					qStatus[2];
	idStr					fileName;
	int						CIN_WIDTH, CIN_HEIGHT;
	idFile* 				iFile;
	cinStatus_t				status;
	// RB: 64 bit fixes, changed long to int
	int						tfps;
	int						RoQPlayed;
	int						ROQSize;
	unsigned int			RoQFrameSize;
	int						onQuad;
	int						numQuads;
	int						samplesPerLine;
	unsigned int			roq_id;
	int						screenDelta;
	byte* 					buf;
	int						samplesPerPixel;				// defaults to 2
	unsigned int			xsize, ysize, maxsize, minsize;
	int						normalBuffer0;
	int						roq_flags;
	int						roqF0;
	int						roqF1;
	int						t[2];
	int						roqFPS;
	int						drawX, drawY;
	// RB end

	int						animationLength;
	int						startTime;
	float					frameRate;

	byte* 					image;

	bool					looping;
	bool					dirty;
	bool					half;
	bool					smootheddouble;
	bool					inMemory;

	void					RoQ_init();
	void					blitVQQuad32fs( byte** status, unsigned char* data );
	void					RoQShutdown();
	void					RoQInterrupt();

	void					move8_32( byte* src, byte* dst, int spl );
	void					move4_32( byte* src, byte* dst, int spl );
	void					blit8_32( byte* src, byte* dst, int spl );
	void					blit4_32( byte* src, byte* dst, int spl );
	void					blit2_32( byte* src, byte* dst, int spl );

	// RB: 64 bit fixes, changed long to int
	unsigned short			yuv_to_rgb( int y, int u, int v );
	unsigned int			yuv_to_rgb24( int y, int u, int v );

	void					decodeCodeBook( byte* input, unsigned short roq_flags );
	void					recurseQuad( int startX, int startY, int quadSize, int xOff, int yOff );
	void					setupQuad( int xOff, int yOff );
	void					readQuadInfo( byte* qData );
	void					RoQPrepMcomp( int xoff, int yoff );
	void					RoQReset();
	// RB end

	//GK:Also init variables for XAudio2 or OpenAL (SRS - this must be an instance variable)
	CinematicAudio*			cinematicAudio = NULL;
};

// Carl: ROQ files from original Doom 3
const int DEFAULT_CIN_WIDTH		= 512;
const int DEFAULT_CIN_HEIGHT	= 512;
const int MAXSIZE				=	8;
const int MINSIZE				=	4;

const int ROQ_FILE				= 0x1084;
const int ROQ_QUAD				= 0x1000;
const int ROQ_QUAD_INFO			= 0x1001;
const int ROQ_CODEBOOK			= 0x1002;
const int ROQ_QUAD_VQ			= 0x1011;
const int ROQ_QUAD_JPEG			= 0x1012;
const int ROQ_QUAD_HANG			= 0x1013;
const int ROQ_PACKET			= 0x1030;
const int ZA_SOUND_MONO			= 0x1020;
const int ZA_SOUND_STEREO		= 0x1021;

// temporary buffers used by all cinematics
// RB: 64 bit fixes, changed long to int
static int				ROQ_YY_tab[256];
static int				ROQ_UB_tab[256];
static int				ROQ_UG_tab[256];
static int				ROQ_VG_tab[256];
static int				ROQ_VR_tab[256];
// RB end
static byte* 			file = NULL;
static unsigned short* 	vq2 = NULL;
static unsigned short* 	vq4 = NULL;
static unsigned short* 	vq8 = NULL;



//===========================================

/*
==============
idCinematic::InitCinematic
==============
*/
// RB: 64 bit fixes, changed long to int
void idCinematic::InitCinematic()
{
	// Carl: Doom 3 ROQ:
	float t_ub, t_vr, t_ug, t_vg;
	int i;

	// generate YUV tables
	t_ub = ( 1.77200f / 2.0f ) * ( float )( 1 << 6 ) + 0.5f;
	t_vr = ( 1.40200f / 2.0f ) * ( float )( 1 << 6 ) + 0.5f;
	t_ug = ( 0.34414f / 2.0f ) * ( float )( 1 << 6 ) + 0.5f;
	t_vg = ( 0.71414f / 2.0f ) * ( float )( 1 << 6 ) + 0.5f;
	for( i = 0; i < 256; i++ )
	{
		float x = ( float )( 2 * i - 255 );

		ROQ_UB_tab[i] = ( int )( ( t_ub * x ) + ( 1 << 5 ) );
		ROQ_VR_tab[i] = ( int )( ( t_vr * x ) + ( 1 << 5 ) );
		ROQ_UG_tab[i] = ( int )( ( -t_ug * x ) );
		ROQ_VG_tab[i] = ( int )( ( -t_vg * x ) + ( 1 << 5 ) );
		ROQ_YY_tab[i] = ( int )( ( i << 6 ) | ( i >> 2 ) );
	}

	file = ( byte* )Mem_Alloc( 65536, TAG_CINEMATIC );
	vq2 = ( word* )Mem_Alloc( 256 * 16 * 4 * sizeof( word ), TAG_CINEMATIC );
	vq4 = ( word* )Mem_Alloc( 256 * 64 * 4 * sizeof( word ), TAG_CINEMATIC );
	vq8 = ( word* )Mem_Alloc( 256 * 256 * 4 * sizeof( word ), TAG_CINEMATIC );

}

/*
==============
idCinematic::ShutdownCinematic
==============
*/
void idCinematic::ShutdownCinematic()
{
	// Carl: Original Doom 3 RoQ files:
	Mem_Free( file );
	file = NULL;
	Mem_Free( vq2 );
	vq2 = NULL;
	Mem_Free( vq4 );
	vq4 = NULL;
	Mem_Free( vq8 );
	vq8 = NULL;
}

/*
==============
idCinematic::Alloc
==============
*/
idCinematic* idCinematic::Alloc()
{
	return new idCinematicLocal; //Carl: Use the proper class like in Doom 3, not just the unimplemented abstract one.
}

/*
==============
idCinematic::~idCinematic
==============
*/
idCinematic::~idCinematic()
{
	Close();
}

/*
==============
idCinematic::InitFromFile
==============
*/
bool idCinematic::InitFromFile( const char* qpath, bool looping, nvrhi::ICommandList* commandList )
{
	return false; //Carl: this is just the abstract virtual method
}

/*
==============
idCinematic::AnimationLength
==============
*/
int idCinematic::AnimationLength()
{
	return 0;
}

/*
==============
idCinematic::GetStartTime
==============
*/
int idCinematic::GetStartTime()
{
	return -1;  // SRS - this is just the abstract virtual method
}

/*
==============
idCinematic::ResetTime
==============
*/
void idCinematic::ResetTime( int milliseconds )
{
}

/*
==============
idCinematic::ImageForTime
==============
*/
cinData_t idCinematic::ImageForTime( int milliseconds, nvrhi::ICommandList* commandList )
{
	cinData_t c;
	memset( &c, 0, sizeof( c ) );
	declManager->FindMaterial( "doom64.tga" );
	c.image = globalImages->GetImage( "doom64.tga" );
	return c;
}

/*
==============
idCinematic::ExportToTGA
==============
*/
void idCinematic::ExportToTGA( bool skipExisting )
{
}

/*
==============
idCinematic::GetFrameRate
==============
*/
float idCinematic::GetFrameRate() const
{
	return 30.0f;
}

/*
==============
idCinematic::Close
==============
*/
void idCinematic::Close()
{
}

// RB begin
/*
==============
idCinematic::IsPlaying
==============
*/
bool idCinematic::IsPlaying() const
{
	return false;
}
// RB end

// SRS begin
/*
==============
CinematicAudio::~CinematicAudio
==============
*/
CinematicAudio::~CinematicAudio()
{
}
// SRS end

//===========================================

/*
==============
idCinematicLocal::idCinematicLocal
==============
*/
idCinematicLocal::idCinematicLocal()
{
	qStatus[0] = ( byte** )Mem_Alloc( 32768 * sizeof( byte* ), TAG_CINEMATIC );
	qStatus[1] = ( byte** )Mem_Alloc( 32768 * sizeof( byte* ), TAG_CINEMATIC );

	isRoQ = false;      // SRS - Initialize isRoQ for all cases, not just FFMPEG
#if defined(USE_FFMPEG)
	// Carl: ffmpeg stuff, for bink and normal video files:
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(55,28,1)
	frame = av_frame_alloc();
	frame2 = av_frame_alloc();
	frame3 = av_frame_alloc();
#else
	frame = avcodec_alloc_frame();
	frame2 = avcodec_alloc_frame();
	frame3 = avcodec_alloc_frame();
#endif // LIBAVCODEC_VERSION_INT
	dec_ctx = NULL;
	dec_ctx2 = NULL;
	fmt_ctx = NULL;
	video_stream_index = -1;
	audio_stream_index = -1;
	hasplanar = false;
	swr_ctx = NULL;
	img_convert_ctx = NULL;
	hasFrame = false;
	framePos = -1;
	lagIndex = 0;
	skipLag = false;
#endif

#ifdef USE_BINKDEC
	binkHandle.isValid = false;
	binkHandle.instanceIndex = -1; // whatever this is, it now has a deterministic value
	hasFrame = false;
	framePos = -1;
	numFrames = 0;
	audioTracks = 0;
	trackIndex = -1;
	binkInfo = {};

	imgY = globalImages->AllocStandaloneImage( "_cinematicY" );
	imgCr = globalImages->AllocStandaloneImage( "_cinematicCr" );
	imgCb = globalImages->AllocStandaloneImage( "_cinematicCb" );
	{
		idImageOpts opts;
		opts.format = FMT_LUM8;
		opts.colorFormat = CFM_DEFAULT;
		opts.width = 32;
		opts.height = 32;
		opts.numLevels = 1;
		if( imgY != NULL )
		{
			imgY->AllocImage( opts, TF_LINEAR, TR_REPEAT );
		}
		if( imgCr != NULL )
		{
			imgCr->AllocImage( opts, TF_LINEAR, TR_REPEAT );
		}
		if( imgCb != NULL )
		{
			imgCb->AllocImage( opts, TF_LINEAR, TR_REPEAT );
		}
	}

#endif

	// Carl: Original Doom 3 RoQ files:
	image = NULL;
	status = FMV_EOF;
	buf = NULL;
	iFile = NULL;
	img = globalImages->AllocStandaloneImage( "_cinematic" );
	if( img != NULL )
	{
		idImageOpts opts;
		opts.format = FMT_RGBA8;
		opts.colorFormat = CFM_DEFAULT;
		opts.width = 32;
		opts.height = 32;
		opts.numLevels = 1;
		img->AllocImage( opts, TF_LINEAR, TR_REPEAT );
	}
}

/*
==============
idCinematicLocal::~idCinematicLocal
==============
*/
idCinematicLocal::~idCinematicLocal()
{
	Close();

	// Carl: Original Doom 3 RoQ files:
	Mem_Free( qStatus[0] );
	qStatus[0] = NULL;
	Mem_Free( qStatus[1] );
	qStatus[1] = NULL;

#if defined(USE_FFMPEG)
	// Carl: ffmpeg for bink and other video files:

	// RB: TODO double check this. It seems we have different versions of ffmpeg on Kubuntu 13.10 and the win32 development files
//#if defined(_WIN32) || defined(_WIN64)
	// SRS - Should use the same version criteria as when the frames are allocated in idCinematicLocal() above
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(55,28,1)
	av_frame_free( &frame );
	av_frame_free( &frame2 );
	av_frame_free( &frame3 );
#else
	av_freep( &frame );
	av_freep( &frame2 );
	av_freep( &frame3 );
#endif
#endif

#ifdef USE_BINKDEC
	delete imgY;
	imgY = NULL;
	delete imgCr;
	imgCr = NULL;
	delete imgCb;
	imgCb = NULL;
#endif

	delete img;
	img = NULL;

	// GK/SRS - Properly close and delete XAudio2 or OpenAL voice if instantiated
	if( cinematicAudio )
	{
		cinematicAudio->ShutdownAudio();
		delete cinematicAudio;
		cinematicAudio = NULL;
	}
}

#if defined(USE_FFMPEG)
/*
==============
GetSampleFormat
==============
*/
const char* GetSampleFormat( AVSampleFormat sample_fmt )
{
	switch( sample_fmt )
	{
		case AV_SAMPLE_FMT_U8:
		case AV_SAMPLE_FMT_U8P:
		{
			return "8-bit";
		}
		case AV_SAMPLE_FMT_S16:
		case AV_SAMPLE_FMT_S16P:
		{
			return "16-bit";
		}
		case AV_SAMPLE_FMT_S32:
		case AV_SAMPLE_FMT_S32P:
		{
			return "32-bit";
		}
		case AV_SAMPLE_FMT_FLT:
		case AV_SAMPLE_FMT_FLTP:
		{
			return "Float";
		}
		case AV_SAMPLE_FMT_DBL:
		case AV_SAMPLE_FMT_DBLP:
		{
			return "Double";
		}
		default:
		{
			return "Unknown";
		}
	}
}

/*
==============
idCinematicLocal::InitFromFFMPEGFile
==============
*/
bool idCinematicLocal::InitFromFFMPEGFile( const char* qpath, bool amilooping, nvrhi::ICommandList* commandList )
{
	int ret;
	int ret2;
	int file_size;
	char error[64];
	looping = amilooping;
	startTime = 0;
	isRoQ = false;
	CIN_HEIGHT = DEFAULT_CIN_HEIGHT;
	CIN_WIDTH  =  DEFAULT_CIN_WIDTH;

	idStr fullpath;
	idFile* testFile = fileSystem->OpenFileRead( qpath );
	if( testFile )
	{
		fullpath = testFile->GetFullPath();
		file_size = testFile->Length();
		fileSystem->CloseFile( testFile );
	}
	// RB: case sensitivity HACK for Linux
	else if( idStr::Cmpn( qpath, "sound/vo", 8 ) == 0 )
	{
		idStr newPath( qpath );
		newPath.Replace( "sound/vo", "sound/VO" );

		testFile = fileSystem->OpenFileRead( newPath );
		if( testFile )
		{
			fullpath = testFile->GetFullPath();
			file_size = testFile->Length();
			fileSystem->CloseFile( testFile );
		}
		else
		{
			common->Warning( "idCinematic: Cannot open FFMPEG video file: '%s', %d\n", qpath, looping );
			return false;
		}
	}

	//idStr fullpath = fileSystem->RelativePathToOSPath( qpath, "fs_basepath" );

	if( ( ret = avformat_open_input( &fmt_ctx, fullpath, NULL, NULL ) ) < 0 )
	{
		// SRS - another case sensitivity HACK for Linux, this time for ffmpeg and RoQ files
		idStr ext;
		fullpath.ExtractFileExtension( ext );
		if( idStr::Cmp( ext.c_str(), "roq" ) == 0 )
		{
			// SRS - If ffmpeg can't open .roq file, then try again with .RoQ extension instead
			fullpath.Replace( ".roq", ".RoQ" );
			ret = avformat_open_input( &fmt_ctx, fullpath, NULL, NULL );
		}
		if( ret < 0 )
		{
			common->Warning( "idCinematic: Cannot open FFMPEG video file: '%s', %d\n", qpath, looping );
			return false;
		}
	}
	if( ( ret = avformat_find_stream_info( fmt_ctx, NULL ) ) < 0 )
	{
		common->Warning( "idCinematic: Cannot find stream info: '%s', %d\n", qpath, looping );
		return false;
	}
	/* select the video stream */
	ret = av_find_best_stream( fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &dec, 0 );
	if( ret < 0 )
	{
		common->Warning( "idCinematic: Cannot find a video stream in: '%s', %d\n", qpath, looping );
		return false;
	}
	video_stream_index = ret;
	dec_ctx = avcodec_alloc_context3( dec );
	if( ( ret = avcodec_parameters_to_context( dec_ctx, fmt_ctx->streams[video_stream_index]->codecpar ) ) < 0 )
	{
		av_strerror( ret, error, sizeof( error ) );
		common->Warning( "idCinematic: Failed to create video codec context from codec parameters with error: %s\n", error );
	}
	//dec_ctx->time_base = fmt_ctx->streams[video_stream_index]->time_base;			// SRS - decoder timebase is set by avcodec_open2()
	dec_ctx->framerate = fmt_ctx->streams[video_stream_index]->avg_frame_rate;
	dec_ctx->pkt_timebase = fmt_ctx->streams[video_stream_index]->time_base;		// SRS - packet timebase for frame->pts timestamps
	/* init the video decoder */
	if( ( ret = avcodec_open2( dec_ctx, dec, NULL ) ) < 0 )
	{
		av_strerror( ret, error, sizeof( error ) );
		common->Warning( "idCinematic: Cannot open video decoder for: '%s', %d, with error: %s\n", qpath, looping, error );
		return false;
	}
	//GK:Begin
	//After the video decoder is open then try to open audio decoder
	ret2 = av_find_best_stream( fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, &dec2, 0 );
	if( ret2 >= 0 && s_playCinematicAudio.GetBool() )  //Make audio optional (only intro video has audio no other)
	{
		audio_stream_index = ret2;
		dec_ctx2 = avcodec_alloc_context3( dec2 );
		if( ( ret2 = avcodec_parameters_to_context( dec_ctx2, fmt_ctx->streams[audio_stream_index]->codecpar ) ) < 0 )
		{
			av_strerror( ret2, error, sizeof( error ) );
			common->Warning( "idCinematic: Failed to create audio codec context from codec parameters with error: %s\n", error );
		}
		//dec_ctx2->time_base = fmt_ctx->streams[audio_stream_index]->time_base;	// SRS - decoder timebase is set by avcodec_open2()
		dec_ctx2->framerate = fmt_ctx->streams[audio_stream_index]->avg_frame_rate;
		dec_ctx2->pkt_timebase = fmt_ctx->streams[audio_stream_index]->time_base;	// SRS - packet timebase for frame3->pts timestamps
		if( ( ret2 = avcodec_open2( dec_ctx2, dec2, NULL ) ) < 0 )
		{
			common->Warning( "idCinematic: Cannot open audio decoder for: '%s', %d\n", qpath, looping );
			//return false;
		}
		if( dec_ctx2->sample_fmt >= AV_SAMPLE_FMT_U8P )											// SRS - Planar formats start at AV_SAMPLE_FMT_U8P
		{
			dst_smp = static_cast<AVSampleFormat>( dec_ctx2->sample_fmt - AV_SAMPLE_FMT_U8P );	// SRS - Setup context to convert from planar to packed
#if	LIBSWRESAMPLE_VERSION_INT >= AV_VERSION_INT(4,7,100)
			if( ( ret2 = swr_alloc_set_opts2( &swr_ctx, &dec_ctx2->ch_layout, dst_smp, dec_ctx2->sample_rate, &dec_ctx2->ch_layout, dec_ctx2->sample_fmt, dec_ctx2->sample_rate, 0, NULL ) ) < 0 )
			{
				av_strerror( ret2, error, sizeof( error ) );
				common->Warning( "idCinematic: Failed to create audio resample context with error: %s\n", error );
			}
#else
			swr_ctx = swr_alloc_set_opts( NULL, dec_ctx2->channel_layout, dst_smp, dec_ctx2->sample_rate, dec_ctx2->channel_layout, dec_ctx2->sample_fmt, dec_ctx2->sample_rate, 0, NULL );
#endif
			ret2 = swr_init( swr_ctx );
			hasplanar = true;
		}
		else
		{
			dst_smp = dec_ctx2->sample_fmt;														// SRS - Must always define the destination format
			hasplanar = false;
		}
		common->Printf( "Cinematic audio stream found: Sample Rate=%d Hz, Channels=%d, Format=%s, Planar=%d\n", dec_ctx2->sample_rate,
#if	LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(59,37,100)
						dec_ctx2->ch_layout.nb_channels,
#else
						dec_ctx2->channels,
#endif
						GetSampleFormat( dec_ctx2->sample_fmt ), hasplanar );

#if defined(_MSC_VER) && !defined(USE_OPENAL)
		cinematicAudio = new( TAG_AUDIO ) CinematicAudio_XAudio2;
#else
		cinematicAudio = new( TAG_AUDIO ) CinematicAudio_OpenAL;
#endif
		cinematicAudio->InitAudio( dec_ctx2 );
	}
	//GK:End
	CIN_WIDTH = dec_ctx->width;
	CIN_HEIGHT = dec_ctx->height;
	/** Calculate Duration in seconds
	  * This is the fundamental unit of time (in seconds) in terms
	  * of which frame timestamps are represented. For fixed-fps content,
	  * timebase should be 1/framerate and timestamp increments should be
	  * identically 1.
	  * - encoding: MUST be set by user.
	  * - decoding: Set by libavcodec.
	  */
	// SRS - Must use consistent duration and timebase parameters in the durationSec calculation (don't mix fmt_ctx duration with dec_ctx timebase)
	float durationSec = static_cast<double>( fmt_ctx->streams[video_stream_index]->duration ) * av_q2d( fmt_ctx->streams[video_stream_index]->time_base );
	//GK: No duration is given. Check if we get at least bitrate to calculate the length, otherwise set it to a fixed 100 seconds (should it be lower ?)
	if( durationSec < 0 )
	{
		// SRS - First check the file context bit rate and estimate duration using file size and overall bit rate
		if( fmt_ctx->bit_rate > 0 )
		{
			durationSec = file_size * 8.0 / fmt_ctx->bit_rate;
		}
		// SRS - Likely an RoQ file, so use the video bit rate tolerance plus audio bit rate to estimate duration, then add 10% to correct for variable bit rate
		else if( dec_ctx->bit_rate_tolerance > 0 )
		{
			durationSec = file_size * 8.0 / ( dec_ctx->bit_rate_tolerance + ( dec_ctx2 ? dec_ctx2->bit_rate : 0 ) ) * 1.1;
		}
		// SRS - Otherwise just set a large max duration
		else
		{
			durationSec = 100.0;
		}
	}
	animationLength = durationSec * 1000;
	frameRate = av_q2d( fmt_ctx->streams[video_stream_index]->avg_frame_rate );
	common->Printf( "Loaded FFMPEG file: '%s', looping=%d, %dx%d, %3.2f FPS, %4.1f sec\n", qpath, looping, CIN_WIDTH, CIN_HEIGHT, frameRate, durationSec );

	// SRS - Get image buffer size (dimensions mod 32 for bik & webm codecs, subsumes mod 16 for mp4 codec), then allocate image and fill with correct parameters
	int bufWidth = ( CIN_WIDTH + 31 ) & ~31;
	int bufHeight = ( CIN_HEIGHT + 31 ) & ~31;
	int img_bytes = av_image_get_buffer_size( AV_PIX_FMT_BGR32, bufWidth, bufHeight, 1 );
	image = ( byte* )Mem_Alloc( img_bytes, TAG_CINEMATIC );
	av_image_fill_arrays( frame2->data, frame2->linesize, image, AV_PIX_FMT_BGR32, CIN_WIDTH, CIN_HEIGHT, 1 ); //GK: Straight out of the FFMPEG source code
	img_convert_ctx = sws_getContext( dec_ctx->width, dec_ctx->height, dec_ctx->pix_fmt, CIN_WIDTH, CIN_HEIGHT, AV_PIX_FMT_BGR32, SWS_BICUBIC, NULL, NULL, NULL );

	status = FMV_PLAY;
	hasFrame = false;
	framePos = -1;
	ImageForTime( 0, commandList );
	status = ( looping ) ? FMV_PLAY : FMV_IDLE;

	return true;
}
#endif

/*
==============
idCinematicLocal::FFMPEGReset
==============
*/
#if defined(USE_FFMPEG)
void idCinematicLocal::FFMPEGReset()
{
	// RB: don't reset startTime here because that breaks video replays in the PDAs
	//startTime = 0;

	framePos = -1;

	// SRS - If we have cinematic audio, reset audio to release any stale buffers and avoid AV drift if looping
	if( cinematicAudio )
	{
		cinematicAudio->ResetAudio();

		lagIndex = 0;
		for( int i = 0; i < NUM_LAG_FRAMES; i++ )
		{
			lagBufSize[ i ] = 0;
			if( lagBuffer[ i ] )
			{
				av_freep( &lagBuffer[ i ] );
			}
		}
	}

	// SRS - For non-RoQ (i.e. bik, mp4, webm, etc) files, use standard frame seek to rewind the stream
	if( dec_ctx->codec_id != AV_CODEC_ID_ROQ && av_seek_frame( fmt_ctx, video_stream_index, 0, 0 ) >= 0 )
	{
		status = FMV_LOOPED;
	}
	// SRS - Special handling for RoQ files: only byte seek works and ffmpeg RoQ decoder needs reset
	else if( dec_ctx->codec_id == AV_CODEC_ID_ROQ && av_seek_frame( fmt_ctx, video_stream_index, 0, AVSEEK_FLAG_BYTE ) >= 0 )
	{
		// Close and reopen the ffmpeg RoQ codec without clearing the context - this seems to reset the decoder properly
		avcodec_close( dec_ctx );
		avcodec_open2( dec_ctx, dec, NULL );

		status = FMV_LOOPED;
	}
	// SRS - Can't rewind the stream so we really are at EOF
	else
	{
		status = FMV_EOF;
	}
}
#endif

/*
==============
idCinematicLocal::InitFromBinkDecFile
==============
*/
#ifdef USE_BINKDEC
bool idCinematicLocal::InitFromBinkDecFile( const char* qpath, bool amilooping, nvrhi::ICommandList* commandList )
{
	looping = amilooping;
	startTime = 0;
	isRoQ = false;
	CIN_HEIGHT = DEFAULT_CIN_HEIGHT;
	CIN_WIDTH  =  DEFAULT_CIN_WIDTH;

	idStr fullpath;
	idFile* testFile = fileSystem->OpenFileRead( qpath );
	if( testFile )
	{
		fullpath = testFile->GetFullPath();
		fileSystem->CloseFile( testFile );
	}
	// RB: case sensitivity HACK for Linux
	else if( idStr::Cmpn( qpath, "sound/vo", 8 ) == 0 )
	{
		idStr newPath( qpath );
		newPath.Replace( "sound/vo", "sound/VO" );

		testFile = fileSystem->OpenFileRead( newPath );
		if( testFile )
		{
			fullpath = testFile->GetFullPath();
			fileSystem->CloseFile( testFile );
		}
		else
		{
			common->Warning( "idCinematic: Cannot open BinkDec video file: '%s', %d\n", qpath, looping );
			return false;
		}
	}

	binkHandle = Bink_Open( fullpath );
	if( !binkHandle.isValid )
	{
		common->Warning( "idCinematic: Cannot open BinkDec video file: '%s', %d\n", qpath, looping );
		return false;
	}

	{
		uint32_t w = 0, h = 0;
		Bink_GetFrameSize( binkHandle, w, h );
		CIN_WIDTH = w;
		CIN_HEIGHT = h;
	}

	// SRS - Support Bink Audio for cinematic playback
	audioTracks = Bink_GetNumAudioTracks( binkHandle );
	if( audioTracks > 0 && s_playCinematicAudio.GetBool() )
	{
		trackIndex = 0;														// SRS - Use the first audio track - is this reasonable?
		binkInfo = Bink_GetAudioTrackDetails( binkHandle, trackIndex );
		common->Printf( "Cinematic audio stream found: Sample Rate=%d Hz, Channels=%d, Format=16-bit\n", binkInfo.sampleRate, binkInfo.nChannels );
#if defined(_MSC_VER) && !defined(USE_OPENAL)
		cinematicAudio = new( TAG_AUDIO ) CinematicAudio_XAudio2;
#else
		cinematicAudio = new( TAG_AUDIO ) CinematicAudio_OpenAL;
#endif
		cinematicAudio->InitAudio( &binkInfo );
	}

	frameRate = Bink_GetFrameRate( binkHandle );
	numFrames = Bink_GetNumFrames( binkHandle );
	float durationSec = numFrames / frameRate;      // SRS - fixed Bink durationSec calculation
	animationLength = durationSec * 1000;           // SRS - animationLength is in milliseconds
	common->Printf( "Loaded BinkDec file: '%s', looping=%d, %dx%d, %3.2f FPS, %4.1f sec\n", qpath, looping, CIN_WIDTH, CIN_HEIGHT, frameRate, durationSec );

	memset( yuvBuffer, 0, sizeof( yuvBuffer ) );

	status = FMV_PLAY;
	hasFrame = false;                               // SRS - Implemented hasFrame for BinkDec behaviour consistency with FFMPEG
	framePos = -1;
	ImageForTime( 0, commandList );                 // SRS - Was missing initial call to ImageForTime() - fixes validation errors when using Vulkan renderer
	status = ( looping ) ? FMV_PLAY : FMV_IDLE;     // SRS - Update status based on looping flag

	return true;
}

/*
==============
idCinematicLocal::BinkDecReset
==============
*/
void idCinematicLocal::BinkDecReset()
{
	framePos = -1;

	// SRS - If we have cinematic audio, reset audio to release any stale buffers (even if looping)
	if( cinematicAudio )
	{
		cinematicAudio->ResetAudio();
	}

	Bink_GotoFrame( binkHandle, 0 );
	status = FMV_LOOPED;
}
#endif // USE_BINKDEC

/*
==============
idCinematicLocal::InitFromFile
==============
*/
bool idCinematicLocal::InitFromFile( const char* qpath, bool amilooping, nvrhi::ICommandList* commandList )
{
	unsigned short RoQID;

	// SRS - Don't need to call Close() here, all initialization is handled by constructor
	//Close();

	inMemory = 0;
	animationLength = 100000;

	// Carl: if no folder is specified, look in the video folder
	if( strstr( qpath, "/" ) == NULL && strstr( qpath, "\\" ) == NULL )
	{
		sprintf( fileName, "video/%s", qpath );
	}
	else
	{
		sprintf( fileName, "%s", qpath );
	}
	// Carl: Look for original Doom 3 RoQ files first:
	idStr temp, ext;
	// SRS - Since loadvideo.bik is hardcoded, first check for legacy idlogo then later for mp4/webm mods
	// If neither idlogo.roq nor loadvideo.mp4/.webm are found, then fall back to stock loadvideo.bik
	if( fileName == "video\\loadvideo.bik" )
	{
		temp = "video\\idlogo.roq";							// Check for legacy idlogo.roq video
	}
	else
	{
		temp = fileName;
	}

	temp.ExtractFileExtension( ext );
	ext.ToLower();
	if( ext == "roq" )
	{
		temp = temp.StripFileExtension() + ".roq";			// Force lower case .roq extension
		iFile = fileSystem->OpenFileRead( temp );
	}
	else
	{
		iFile = NULL;
	}

	// Carl: If the RoQ file doesn't exist, try using bik file extension instead:
	if( !iFile )
	{
		//idLib::Warning( "Original Doom 3 RoQ Cinematic not found: '%s'\n", temp.c_str() );
#if defined(USE_FFMPEG)
		if( fileName == "video\\loadvideo.bik" )
		{
			temp = "video\\loadvideo";						// Check for loadvideo.xxx mod videos
			if( ( iFile = fileSystem->OpenFileRead( temp + ".mp4" ) ) )
			{
				fileSystem->CloseFile( iFile );				// Close the mp4 file and reopen in ffmpeg
				iFile = NULL;
				temp = temp + ".mp4";
				ext = "mp4";
			}
			else if( ( iFile = fileSystem->OpenFileRead( temp + ".webm" ) ) )
			{
				fileSystem->CloseFile( iFile );				// Close the webm file and reopen in ffmpeg
				iFile = NULL;
				temp = temp + ".webm";
				ext = "webm";
			}
			else
			{
				temp = temp + ".bik";						// Fall back to bik if no mod videos found
				ext = "bik";
			}
		}

		if( ext == "roq" || ext == "bik" )					// Check for stock/legacy cinematic types
		{
			temp = fileName.StripFileExtension() + ".bik";
			skipLag = false;			// SRS - Enable lag buffer for ffmpeg bik decoder AV sync
		}
		else												// Enable mp4/webm cinematic types for mods
		{
			skipLag = true;				// SRS - Disable lag buffer for ffmpeg mp4/webm decoder AV sync
		}

		// SRS - Support RoQ cinematic playback via ffmpeg decoder - better quality plus audio support
	}
	else
	{
		fileSystem->CloseFile( iFile );	// SRS - Close the RoQ file and let ffmpeg reopen it
		iFile = NULL;
		skipLag = true;					// SRS - Disable lag buffer for ffmpeg RoQ decoder AV sync
	}
	{
		// SRS End

		animationLength = 0;
		fileName = temp;
		//idLib::Warning( "New filename: '%s'\n", fileName.c_str() );
		return InitFromFFMPEGFile( fileName.c_str(), amilooping, commandList );
#elif defined(USE_BINKDEC)
		temp = fileName.StripFileExtension() + ".bik";
		animationLength = 0;
		fileName = temp;
		//idLib::Warning( "New filename: '%s'\n", fileName.c_str() );
		return InitFromBinkDecFile( fileName.c_str(), amilooping, commandList );
#else
		animationLength = 0;
		return false;
#endif
	}
	// Carl: The rest of this function is for original Doom 3 RoQ files:
	isRoQ = true;
	fileName = temp;
	ROQSize = iFile->Length();

	looping = amilooping;

	CIN_HEIGHT = DEFAULT_CIN_HEIGHT;
	CIN_WIDTH  =  DEFAULT_CIN_WIDTH;
	samplesPerPixel = 4;
	startTime = 0;	//Sys_Milliseconds();
	buf = NULL;

	iFile->Read( file, 16 );

	RoQID = ( unsigned short )( file[0] ) + ( unsigned short )( file[1] ) * 256;

	frameRate = file[6];
	if( frameRate == 32.0f )
	{
		frameRate = 1000.0f / 32.0f;
	}

	if( RoQID == ROQ_FILE )
	{
		RoQ_init();
		status = FMV_PLAY;
		ImageForTime( 0, commandList );
		common->Printf( "Loaded RoQ file: '%s', looping=%d, %dx%d, %3.2f FPS\n", fileName.c_str(), looping, CIN_WIDTH, CIN_HEIGHT, frameRate );
		status = ( looping ) ? FMV_PLAY : FMV_IDLE;
		return true;
	}

	RoQShutdown();
	return false;
}

/*
==============
idCinematicLocal::Close
==============
*/
void idCinematicLocal::Close()
{
	if( image )
	{
		Mem_Free( ( void* )image );
		image = NULL;
		buf = NULL;
		status = FMV_EOF;
	}

	if( isRoQ )
	{
		RoQShutdown();
	}
#if defined(USE_FFMPEG)
	else //if( !isRoQ )
	{
		if( img_convert_ctx )
		{
			sws_freeContext( img_convert_ctx );
			img_convert_ctx = NULL;
		}

		// SRS - If we have cinematic audio, free audio codec context, resample context, and any lagged audio buffers
		if( cinematicAudio )
		{
			if( dec_ctx2 )
			{
				avcodec_free_context( &dec_ctx2 );
			}

			// SRS - Free resample context if we were decoding planar audio
			if( swr_ctx )
			{
				swr_free( &swr_ctx );
			}

			lagIndex = 0;
			for( int i = 0; i < NUM_LAG_FRAMES; i++ )
			{
				lagBufSize[ i ] = 0;
				if( lagBuffer[ i ] )
				{
					av_freep( &lagBuffer[ i ] );
				}
			}
		}

		if( dec_ctx )
		{
			avcodec_free_context( &dec_ctx );
		}

		if( fmt_ctx )
		{
			avformat_close_input( &fmt_ctx );
		}
		status = FMV_EOF;
	}
#elif defined(USE_BINKDEC)
	else //if( !isRoQ )
	{
		if( binkHandle.isValid )
		{
			memset( yuvBuffer, 0 , sizeof( yuvBuffer ) );
			Bink_Close( binkHandle );
		}
		status = FMV_EOF;
	}
#endif
}

/*
==============
idCinematicLocal::AnimationLength
==============
*/
int idCinematicLocal::AnimationLength()
{
	return animationLength;
}

// RB begin
/*
==============
idCinematicLocal::IsPlaying
==============
*/
bool idCinematicLocal::IsPlaying() const
{
	return ( status == FMV_PLAY );
}
// RB end

// SRS - Implement virtual method to override abstract virtual method
/*
==============
 idCinematicLocal::GetStartTime
==============
*/
int idCinematicLocal::GetStartTime()
{
	return startTime;
}
// SRS end

/*
==============
idCinematicLocal::ResetTime
==============
*/
void idCinematicLocal::ResetTime( int time )
{
	startTime = time; //originally this was: ( backEnd.viewDef ) ? 1000 * backEnd.viewDef->floatTime : -1;
	status = FMV_PLAY;
}

/*
==============
idCinematicLocal::ImageForTime
==============
*/
cinData_t idCinematicLocal::ImageForTime( int thisTime, nvrhi::ICommandList* commandList )
{
	cinData_t	cinData;

	if( thisTime <= 0 )
	{
		thisTime = Sys_Milliseconds();
	}

	memset( &cinData, 0, sizeof( cinData ) );
	// SRS - also return if commandList is null, typically when called from within InitFromFile()
	if( r_skipDynamicTextures.GetBool() || status == FMV_EOF || status == FMV_IDLE || !commandList )
	{
		return cinData;
	}

#if defined(USE_FFMPEG)
	// Carl: Handle BFG format BINK videos separately
	if( !isRoQ )
	{
		return ImageForTimeFFMPEG( thisTime, commandList );
	}
#elif defined(USE_BINKDEC) // DG: libbinkdec support
	if( !isRoQ )
	{
		return ImageForTimeBinkDec( thisTime, commandList );
	}
#endif

	if( !iFile )
	{
		// RB: neither .bik or .roq found
		return cinData;
	}

	if( buf == NULL || startTime == -1 )
	{
		if( startTime == -1 )
		{
			RoQReset();
		}
		startTime = thisTime;
	}

	tfps = ( ( thisTime - startTime ) * frameRate ) / 1000;

	if( tfps < 0 )
	{
		tfps = 0;
	}

	// SRS - Need to use numQuads - 1 for frame position (otherwise get into reset loop at start)
	if( tfps < numQuads - 1 )
	{
		RoQReset();
		buf = NULL;
		status = FMV_PLAY;
	}

	if( buf == NULL )
	{
		// SRS - This frame init loop is not really necessary, but leaving in to avoid breakage
		while( buf == NULL )
		{
			RoQInterrupt();
		}
	}
	else
	{
		// SRS - This frame loop is really all we need and could handle the above case as well
		while( ( numQuads - 1 < tfps && status == FMV_PLAY ) )
		{
			RoQInterrupt();
		}
	}

	if( status == FMV_LOOPED || status == FMV_EOF )
	{
		if( looping )
		{
			//RoQReset();		// SRS - RoQReset() already called by RoQInterrupt() when looping
			buf = NULL;
			status = FMV_PLAY;
			while( buf == NULL && status == FMV_PLAY )
			{
				RoQInterrupt();
			}
			startTime = thisTime;
		}
		else
		{
			buf = NULL;
			status = FMV_IDLE;
			//RoQShutdown();	//SRS - RoQShutdown() not needed on EOF, return null data instead
			return cinData;
		}
	}

	cinData.imageWidth = CIN_WIDTH;
	cinData.imageHeight = CIN_HEIGHT;
	cinData.status = status;
	img->UploadScratch( image, CIN_WIDTH, CIN_HEIGHT, commandList );
	cinData.image = img;

	return cinData;
}

/*
==============
idCinematicLocal::ImageForTimeFFMPEG
==============
*/
#if defined(USE_FFMPEG)
cinData_t idCinematicLocal::ImageForTimeFFMPEG( int thisTime, nvrhi::ICommandList* commandList )
{
	cinData_t	cinData;
	char		error[64];
	uint8_t*	audioBuffer = NULL;
	int			num_bytes = 0;

	memset( &cinData, 0, sizeof( cinData ) );
	if( !fmt_ctx )
	{
		// RB: .bik requested but not found
		return cinData;
	}

	if( ( !hasFrame ) || startTime == -1 )
	{
		if( startTime == -1 )
		{
			FFMPEGReset();
		}
		startTime = thisTime;
	}

	long desiredFrame = ( ( thisTime - startTime ) * frameRate ) / 1000;
	if( desiredFrame < 0 )
	{
		desiredFrame = 0;
	}

	if( desiredFrame < framePos )
	{
		FFMPEGReset();
		hasFrame = false;
		status = FMV_PLAY;
	}

	if( hasFrame && desiredFrame == framePos )
	{
		cinData.imageWidth = CIN_WIDTH;
		cinData.imageHeight = CIN_HEIGHT;
		cinData.status = status;
		cinData.image = img;
		return cinData;
	}

	AVPacket packet;
	while( framePos < desiredFrame )
	{
		int frameFinished = -1;
		int res = 0;

		// Do a single frame by getting packets until we have a full frame
		while( frameFinished != 0 )
		{
			// if we got to the end or failed
			if( av_read_frame( fmt_ctx, &packet ) < 0 )
			{
				// can't read any more, set to EOF
				status = FMV_EOF;
				if( looping )
				{
					desiredFrame = 0;
					FFMPEGReset();
					hasFrame = false;
					startTime = thisTime;
					if( av_read_frame( fmt_ctx, &packet ) < 0 )
					{
						status = FMV_IDLE;
						return cinData;
					}
					status = FMV_PLAY;
				}
				else
				{
					hasFrame = false;
					status = FMV_IDLE;
					return cinData;
				}
			}
			// Is this a packet from the video stream?
			if( packet.stream_index == video_stream_index )
			{
				// Decode video frame
				if( ( res = avcodec_send_packet( dec_ctx, &packet ) ) != 0 )
				{
					av_strerror( res, error, sizeof( error ) );
					common->Warning( "idCinematic: Failed to send video packet for decoding with error: %s\n", error );
				}
				else
				{
					frameFinished = avcodec_receive_frame( dec_ctx, frame );
					if( frameFinished != 0 && frameFinished != AVERROR( EAGAIN ) )
					{
						av_strerror( frameFinished, error, sizeof( error ) );
						common->Warning( "idCinematic: Failed to receive video frame from decoding with error: %s\n", error );
					}
				}
			}
			//GK:Begin
			else if( cinematicAudio && packet.stream_index == audio_stream_index ) //Check if it found any audio data
			{
				res = avcodec_send_packet( dec_ctx2, &packet );
				if( res != 0 && res != AVERROR( EAGAIN ) )
				{
					av_strerror( res, error, sizeof( error ) );
					common->Warning( "idCinematic: Failed to send audio packet for decoding with error: %s\n", error );
				}
				//SRS - Separate frame finisher for audio since there can be multiple audio frames per video frame (e.g. at bik startup)
				int frameFinished1 = 0;
				while( frameFinished1 == 0 )
				{
					if( ( frameFinished1 = avcodec_receive_frame( dec_ctx2, frame3 ) ) != 0 )
					{
						if( frameFinished1 != AVERROR( EAGAIN ) )
						{
							av_strerror( frameFinished1, error, sizeof( error ) );
							common->Warning( "idCinematic: Failed to receive audio frame from decoding with error: %s\n", error );
						}
					}
					// SRS - For the final (desired) frame only: allocate audio buffers, convert to packed format, and play the synced audio frames
					else if( framePos + 1 == desiredFrame )
					{
						// SRS - Since destination sample format is packed (non-planar), returned bufflinesize equals num_bytes
#if	LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(59,37,100)
						res = av_samples_alloc( &audioBuffer, &num_bytes, frame3->ch_layout.nb_channels, frame3->nb_samples, dst_smp, 0 );
#else
						res = av_samples_alloc( &audioBuffer, &num_bytes, frame3->channels, frame3->nb_samples, dst_smp, 0 );
#endif
						if( res < 0 || res != num_bytes )
						{
							common->Warning( "idCinematic: Failed to allocate audio buffer with result: %d\n", res );
						}
						if( hasplanar )
						{
							// SRS - Convert from planar to packed format keeping sample count the same
							res = swr_convert( swr_ctx, &audioBuffer, frame3->nb_samples, ( const uint8_t** )frame3->extended_data, frame3->nb_samples );
							if( res < 0 || res != frame3->nb_samples )
							{
								common->Warning( "idCinematic: Failed to convert planar audio data to packed format with result: %d\n", res );
							}
						}
						else
						{
							// SRS - Since audio is already in packed format, just copy into audio buffer
							if( num_bytes > 0 )
							{
								memcpy( audioBuffer, frame3->extended_data[0], num_bytes );
							}
						}
						// SRS - If we have cinematic audio data, play a lagged frame (for bik video sync) and save the current frame
						if( num_bytes > 0 )
						{
							// SRS - If we have a lagged cinematic audio frame, then play it now
							if( lagBufSize[ lagIndex ] > 0 )
							{
								// SRS - Note that PlayAudio() is responsible for releasing any audio buffers sent to it
								cinematicAudio->PlayAudio( lagBuffer[ lagIndex ], lagBufSize[ lagIndex ] );
							}

							// SRS - Save the current (new) audio buffer and its size to play in the future
							lagBuffer[ lagIndex ] = audioBuffer;
							lagBufSize[ lagIndex ] = num_bytes;

							// SRS - If skipLag is true (e.g. RoQ, mp4, webm), reduce lag to 1 frame since AV playback is already synced
							lagIndex = ( lagIndex + 1 ) % ( skipLag ? 1 : NUM_LAG_FRAMES );
						}
						// SRS - Not sure if an audioBuffer can ever be allocated on failure, but check and free just in case
						else if( audioBuffer )
						{
							av_freep( &audioBuffer );
						}
						//common->Printf( "idCinematic: video pts = %7.3f, audio pts = %7.3f, samples = %4d, num_bytes = %5d\n", static_cast<double>( frame->pts ) * av_q2d( dec_ctx->pkt_timebase ), static_cast<double>( frame3->pts ) * av_q2d( dec_ctx2->pkt_timebase ), frame3->nb_samples, num_bytes );
					}
				}
			}
			//GK:End
			// Free the packet that was allocated by av_read_frame
			av_packet_unref( &packet );
		}

		framePos++;
	}

	// We have reached the desired frame
	// Convert the image from its native format to RGB
	sws_scale( img_convert_ctx, frame->data, frame->linesize, 0, dec_ctx->height, frame2->data, frame2->linesize );
	cinData.imageWidth = CIN_WIDTH;
	cinData.imageHeight = CIN_HEIGHT;
	cinData.status = status;
	img->UploadScratch( image, CIN_WIDTH, CIN_HEIGHT, commandList );
	hasFrame = true;
	cinData.image = img;

	return cinData;
}
#endif


/*
==============
idCinematicLocal::ImageForTimeBinkDec
==============
*/
#ifdef USE_BINKDEC
cinData_t idCinematicLocal::ImageForTimeBinkDec( int thisTime, nvrhi::ICommandList* commandList )
{
	cinData_t	cinData;
	int16_t*	audioBuffer = NULL;
	uint32_t	num_bytes = 0;

	memset( &cinData, 0, sizeof( cinData ) );
	if( !binkHandle.isValid )
	{
		// RB: .bik requested but not found
		return cinData;
	}

	// SRS - Implement hasFrame so BinkDec startTime is handled the same as with FFMPEG
	if( ( !hasFrame ) || startTime == -1 )
	{
		if( startTime == -1 )
		{
			BinkDecReset();
		}
		startTime = thisTime;
	}

	int desiredFrame = ( ( thisTime - startTime ) * frameRate ) / 1000.0f;
	if( desiredFrame < 0 )
	{
		desiredFrame = 0;
	}

	// SRS - Enable video replay within PDAs
	if( desiredFrame < framePos )
	{
		BinkDecReset();
		hasFrame = false;
		status = FMV_PLAY;
	}
	// SRS end

	if( hasFrame && desiredFrame == framePos )
	{
		cinData.imageWidth = CIN_WIDTH;
		cinData.imageHeight = CIN_HEIGHT;
		cinData.status = status;

		cinData.imageY = imgY;
		cinData.imageCr = imgCr;
		cinData.imageCb = imgCb;
		return cinData;
	}

	if( desiredFrame >= numFrames )
	{
		status = FMV_EOF;
		if( looping )
		{
			desiredFrame = 0;
			BinkDecReset();
			hasFrame = false;
			startTime = thisTime;
			status = FMV_PLAY;
		}
		else
		{
			hasFrame = false;
			status = FMV_IDLE;
			return cinData;
		}
	}

	// Bink_GotoFrame(binkHandle, desiredFrame);
	// apparently Bink_GotoFrame() doesn't work super well, so skip frames
	// (if necessary) by calling Bink_GetNextFrame()
	while( framePos < desiredFrame )
	{
		framePos = Bink_GetNextFrame( binkHandle, yuvBuffer );
	}

	cinData.imageWidth = CIN_WIDTH;
	cinData.imageHeight = CIN_HEIGHT;
	cinData.status = status;

	double invAspRat = double( CIN_HEIGHT ) / double( CIN_WIDTH );

	idImage* imgs[ 3 ] = { imgY, imgCb, imgCr }; // that's the order of the channels in yuvBuffer[]
	for( int i = 0; i < 3; ++i )
	{
		// Note: img->UploadScratch() seems to assume 32bit per pixel data, but this is 8bit/pixel
		//       so uploading is a bit more manual here (compared to ffmpeg or RoQ)
		idImage* img = imgs[i];
		int w = yuvBuffer[i].width;
		int h = yuvBuffer[i].height;
		// some videos, including the logo video and the main menu background,
		// seem to have superfluous rows in at least some of the channels,
		// leading to a black or glitchy bar at the bottom of the video.
		// cut that off by reducing the height to the expected height
		if( h > CIN_HEIGHT )
		{
			h = CIN_HEIGHT;
		}
		else if( h < CIN_HEIGHT )
		{
			// the U and V channels have a lower resolution than the Y channel
			// (or the logical video resolution), so use the aspect ratio to
			// calculate the real height
			int hExp = invAspRat * w + 0.5;
			if( h > hExp )
			{
				h = hExp;
			}
		}

#if defined( USE_NVRHI )
		img->UploadScratch( yuvBuffer[i].data, w, h, commandList );
#else
		if( img->GetUploadWidth() != w || img->GetUploadHeight() != h )
		{
			idImageOpts opts = img->GetOpts();
			opts.width = w;
			opts.height = h;
			img->AllocImage( opts, TF_LINEAR, TR_REPEAT );
		}
		img->SubImageUpload( 0, 0, 0, 0, w, h, yuvBuffer[i].data, commandList );
#endif
	}

	hasFrame = true;
	cinData.imageY = imgY;
	cinData.imageCr = imgCr;
	cinData.imageCb = imgCb;

	if( cinematicAudio )
	{
		audioBuffer = ( int16_t* )Mem_Alloc( binkInfo.idealBufferSize, TAG_AUDIO );
		num_bytes = Bink_GetAudioData( binkHandle, trackIndex, audioBuffer );

		// SRS - If we have cinematic audio data, start playing it now
		if( num_bytes > 0 )
		{
			// SRS - Note that PlayAudio() is responsible for releasing any audio buffers sent to it
			cinematicAudio->PlayAudio( ( uint8_t* )audioBuffer, num_bytes );
		}
		else
		{
			// SRS - Even though we have no audio data to play, still need to free the audio buffer
			Mem_Free( audioBuffer );
			audioBuffer = NULL;
		}
	}

	return cinData;
}
#endif

/*
==============
idCinematicLocal::move8_32
==============
*/
void idCinematicLocal::move8_32( byte* src, byte* dst, int spl )
{
#if 1
	int* dsrc, *ddst;
	int dspl;

	dsrc = ( int* )src;
	ddst = ( int* )dst;
	dspl = spl >> 2;

	ddst[0 * dspl + 0] = dsrc[0 * dspl + 0];
	ddst[0 * dspl + 1] = dsrc[0 * dspl + 1];
	ddst[0 * dspl + 2] = dsrc[0 * dspl + 2];
	ddst[0 * dspl + 3] = dsrc[0 * dspl + 3];
	ddst[0 * dspl + 4] = dsrc[0 * dspl + 4];
	ddst[0 * dspl + 5] = dsrc[0 * dspl + 5];
	ddst[0 * dspl + 6] = dsrc[0 * dspl + 6];
	ddst[0 * dspl + 7] = dsrc[0 * dspl + 7];

	ddst[1 * dspl + 0] = dsrc[1 * dspl + 0];
	ddst[1 * dspl + 1] = dsrc[1 * dspl + 1];
	ddst[1 * dspl + 2] = dsrc[1 * dspl + 2];
	ddst[1 * dspl + 3] = dsrc[1 * dspl + 3];
	ddst[1 * dspl + 4] = dsrc[1 * dspl + 4];
	ddst[1 * dspl + 5] = dsrc[1 * dspl + 5];
	ddst[1 * dspl + 6] = dsrc[1 * dspl + 6];
	ddst[1 * dspl + 7] = dsrc[1 * dspl + 7];

	ddst[2 * dspl + 0] = dsrc[2 * dspl + 0];
	ddst[2 * dspl + 1] = dsrc[2 * dspl + 1];
	ddst[2 * dspl + 2] = dsrc[2 * dspl + 2];
	ddst[2 * dspl + 3] = dsrc[2 * dspl + 3];
	ddst[2 * dspl + 4] = dsrc[2 * dspl + 4];
	ddst[2 * dspl + 5] = dsrc[2 * dspl + 5];
	ddst[2 * dspl + 6] = dsrc[2 * dspl + 6];
	ddst[2 * dspl + 7] = dsrc[2 * dspl + 7];

	ddst[3 * dspl + 0] = dsrc[3 * dspl + 0];
	ddst[3 * dspl + 1] = dsrc[3 * dspl + 1];
	ddst[3 * dspl + 2] = dsrc[3 * dspl + 2];
	ddst[3 * dspl + 3] = dsrc[3 * dspl + 3];
	ddst[3 * dspl + 4] = dsrc[3 * dspl + 4];
	ddst[3 * dspl + 5] = dsrc[3 * dspl + 5];
	ddst[3 * dspl + 6] = dsrc[3 * dspl + 6];
	ddst[3 * dspl + 7] = dsrc[3 * dspl + 7];

	ddst[4 * dspl + 0] = dsrc[4 * dspl + 0];
	ddst[4 * dspl + 1] = dsrc[4 * dspl + 1];
	ddst[4 * dspl + 2] = dsrc[4 * dspl + 2];
	ddst[4 * dspl + 3] = dsrc[4 * dspl + 3];
	ddst[4 * dspl + 4] = dsrc[4 * dspl + 4];
	ddst[4 * dspl + 5] = dsrc[4 * dspl + 5];
	ddst[4 * dspl + 6] = dsrc[4 * dspl + 6];
	ddst[4 * dspl + 7] = dsrc[4 * dspl + 7];

	ddst[5 * dspl + 0] = dsrc[5 * dspl + 0];
	ddst[5 * dspl + 1] = dsrc[5 * dspl + 1];
	ddst[5 * dspl + 2] = dsrc[5 * dspl + 2];
	ddst[5 * dspl + 3] = dsrc[5 * dspl + 3];
	ddst[5 * dspl + 4] = dsrc[5 * dspl + 4];
	ddst[5 * dspl + 5] = dsrc[5 * dspl + 5];
	ddst[5 * dspl + 6] = dsrc[5 * dspl + 6];
	ddst[5 * dspl + 7] = dsrc[5 * dspl + 7];

	ddst[6 * dspl + 0] = dsrc[6 * dspl + 0];
	ddst[6 * dspl + 1] = dsrc[6 * dspl + 1];
	ddst[6 * dspl + 2] = dsrc[6 * dspl + 2];
	ddst[6 * dspl + 3] = dsrc[6 * dspl + 3];
	ddst[6 * dspl + 4] = dsrc[6 * dspl + 4];
	ddst[6 * dspl + 5] = dsrc[6 * dspl + 5];
	ddst[6 * dspl + 6] = dsrc[6 * dspl + 6];
	ddst[6 * dspl + 7] = dsrc[6 * dspl + 7];

	ddst[7 * dspl + 0] = dsrc[7 * dspl + 0];
	ddst[7 * dspl + 1] = dsrc[7 * dspl + 1];
	ddst[7 * dspl + 2] = dsrc[7 * dspl + 2];
	ddst[7 * dspl + 3] = dsrc[7 * dspl + 3];
	ddst[7 * dspl + 4] = dsrc[7 * dspl + 4];
	ddst[7 * dspl + 5] = dsrc[7 * dspl + 5];
	ddst[7 * dspl + 6] = dsrc[7 * dspl + 6];
	ddst[7 * dspl + 7] = dsrc[7 * dspl + 7];
#else
	double* dsrc, *ddst;
	int dspl;

	dsrc = ( double* )src;
	ddst = ( double* )dst;
	dspl = spl >> 3;

	ddst[0] = dsrc[0];
	ddst[1] = dsrc[1];
	ddst[2] = dsrc[2];
	ddst[3] = dsrc[3];
	dsrc += dspl;
	ddst += dspl;
	ddst[0] = dsrc[0];
	ddst[1] = dsrc[1];
	ddst[2] = dsrc[2];
	ddst[3] = dsrc[3];
	dsrc += dspl;
	ddst += dspl;
	ddst[0] = dsrc[0];
	ddst[1] = dsrc[1];
	ddst[2] = dsrc[2];
	ddst[3] = dsrc[3];
	dsrc += dspl;
	ddst += dspl;
	ddst[0] = dsrc[0];
	ddst[1] = dsrc[1];
	ddst[2] = dsrc[2];
	ddst[3] = dsrc[3];
	dsrc += dspl;
	ddst += dspl;
	ddst[0] = dsrc[0];
	ddst[1] = dsrc[1];
	ddst[2] = dsrc[2];
	ddst[3] = dsrc[3];
	dsrc += dspl;
	ddst += dspl;
	ddst[0] = dsrc[0];
	ddst[1] = dsrc[1];
	ddst[2] = dsrc[2];
	ddst[3] = dsrc[3];
	dsrc += dspl;
	ddst += dspl;
	ddst[0] = dsrc[0];
	ddst[1] = dsrc[1];
	ddst[2] = dsrc[2];
	ddst[3] = dsrc[3];
	dsrc += dspl;
	ddst += dspl;
	ddst[0] = dsrc[0];
	ddst[1] = dsrc[1];
	ddst[2] = dsrc[2];
	ddst[3] = dsrc[3];
#endif
}

/*
==============
idCinematicLocal::move4_32
==============
*/
void idCinematicLocal::move4_32( byte* src, byte* dst, int spl )
{
#if 1
	int* dsrc, *ddst;
	int dspl;

	dsrc = ( int* )src;
	ddst = ( int* )dst;
	dspl = spl >> 2;

	ddst[0 * dspl + 0] = dsrc[0 * dspl + 0];
	ddst[0 * dspl + 1] = dsrc[0 * dspl + 1];
	ddst[0 * dspl + 2] = dsrc[0 * dspl + 2];
	ddst[0 * dspl + 3] = dsrc[0 * dspl + 3];

	ddst[1 * dspl + 0] = dsrc[1 * dspl + 0];
	ddst[1 * dspl + 1] = dsrc[1 * dspl + 1];
	ddst[1 * dspl + 2] = dsrc[1 * dspl + 2];
	ddst[1 * dspl + 3] = dsrc[1 * dspl + 3];

	ddst[2 * dspl + 0] = dsrc[2 * dspl + 0];
	ddst[2 * dspl + 1] = dsrc[2 * dspl + 1];
	ddst[2 * dspl + 2] = dsrc[2 * dspl + 2];
	ddst[2 * dspl + 3] = dsrc[2 * dspl + 3];

	ddst[3 * dspl + 0] = dsrc[3 * dspl + 0];
	ddst[3 * dspl + 1] = dsrc[3 * dspl + 1];
	ddst[3 * dspl + 2] = dsrc[3 * dspl + 2];
	ddst[3 * dspl + 3] = dsrc[3 * dspl + 3];
#else
	double* dsrc, *ddst;
	int dspl;

	dsrc = ( double* )src;
	ddst = ( double* )dst;
	dspl = spl >> 3;

	ddst[0] = dsrc[0];
	ddst[1] = dsrc[1];
	dsrc += dspl;
	ddst += dspl;
	ddst[0] = dsrc[0];
	ddst[1] = dsrc[1];
	dsrc += dspl;
	ddst += dspl;
	ddst[0] = dsrc[0];
	ddst[1] = dsrc[1];
	dsrc += dspl;
	ddst += dspl;
	ddst[0] = dsrc[0];
	ddst[1] = dsrc[1];
#endif
}

/*
==============
idCinematicLocal::blit8_32
==============
*/
void idCinematicLocal::blit8_32( byte* src, byte* dst, int spl )
{
#if 1
	int* dsrc, *ddst;
	int dspl;

	dsrc = ( int* )src;
	ddst = ( int* )dst;
	dspl = spl >> 2;

	ddst[0 * dspl + 0] = dsrc[ 0];
	ddst[0 * dspl + 1] = dsrc[ 1];
	ddst[0 * dspl + 2] = dsrc[ 2];
	ddst[0 * dspl + 3] = dsrc[ 3];
	ddst[0 * dspl + 4] = dsrc[ 4];
	ddst[0 * dspl + 5] = dsrc[ 5];
	ddst[0 * dspl + 6] = dsrc[ 6];
	ddst[0 * dspl + 7] = dsrc[ 7];

	ddst[1 * dspl + 0] = dsrc[ 8];
	ddst[1 * dspl + 1] = dsrc[ 9];
	ddst[1 * dspl + 2] = dsrc[10];
	ddst[1 * dspl + 3] = dsrc[11];
	ddst[1 * dspl + 4] = dsrc[12];
	ddst[1 * dspl + 5] = dsrc[13];
	ddst[1 * dspl + 6] = dsrc[14];
	ddst[1 * dspl + 7] = dsrc[15];

	ddst[2 * dspl + 0] = dsrc[16];
	ddst[2 * dspl + 1] = dsrc[17];
	ddst[2 * dspl + 2] = dsrc[18];
	ddst[2 * dspl + 3] = dsrc[19];
	ddst[2 * dspl + 4] = dsrc[20];
	ddst[2 * dspl + 5] = dsrc[21];
	ddst[2 * dspl + 6] = dsrc[22];
	ddst[2 * dspl + 7] = dsrc[23];

	ddst[3 * dspl + 0] = dsrc[24];
	ddst[3 * dspl + 1] = dsrc[25];
	ddst[3 * dspl + 2] = dsrc[26];
	ddst[3 * dspl + 3] = dsrc[27];
	ddst[3 * dspl + 4] = dsrc[28];
	ddst[3 * dspl + 5] = dsrc[29];
	ddst[3 * dspl + 6] = dsrc[30];
	ddst[3 * dspl + 7] = dsrc[31];

	ddst[4 * dspl + 0] = dsrc[32];
	ddst[4 * dspl + 1] = dsrc[33];
	ddst[4 * dspl + 2] = dsrc[34];
	ddst[4 * dspl + 3] = dsrc[35];
	ddst[4 * dspl + 4] = dsrc[36];
	ddst[4 * dspl + 5] = dsrc[37];
	ddst[4 * dspl + 6] = dsrc[38];
	ddst[4 * dspl + 7] = dsrc[39];

	ddst[5 * dspl + 0] = dsrc[40];
	ddst[5 * dspl + 1] = dsrc[41];
	ddst[5 * dspl + 2] = dsrc[42];
	ddst[5 * dspl + 3] = dsrc[43];
	ddst[5 * dspl + 4] = dsrc[44];
	ddst[5 * dspl + 5] = dsrc[45];
	ddst[5 * dspl + 6] = dsrc[46];
	ddst[5 * dspl + 7] = dsrc[47];

	ddst[6 * dspl + 0] = dsrc[48];
	ddst[6 * dspl + 1] = dsrc[49];
	ddst[6 * dspl + 2] = dsrc[50];
	ddst[6 * dspl + 3] = dsrc[51];
	ddst[6 * dspl + 4] = dsrc[52];
	ddst[6 * dspl + 5] = dsrc[53];
	ddst[6 * dspl + 6] = dsrc[54];
	ddst[6 * dspl + 7] = dsrc[55];

	ddst[7 * dspl + 0] = dsrc[56];
	ddst[7 * dspl + 1] = dsrc[57];
	ddst[7 * dspl + 2] = dsrc[58];
	ddst[7 * dspl + 3] = dsrc[59];
	ddst[7 * dspl + 4] = dsrc[60];
	ddst[7 * dspl + 5] = dsrc[61];
	ddst[7 * dspl + 6] = dsrc[62];
	ddst[7 * dspl + 7] = dsrc[63];
#else
	double* dsrc, *ddst;
	int dspl;

	dsrc = ( double* )src;
	ddst = ( double* )dst;
	dspl = spl >> 3;

	ddst[0] = dsrc[0];
	ddst[1] = dsrc[1];
	ddst[2] = dsrc[2];
	ddst[3] = dsrc[3];
	dsrc += 4;
	ddst += dspl;
	ddst[0] = dsrc[0];
	ddst[1] = dsrc[1];
	ddst[2] = dsrc[2];
	ddst[3] = dsrc[3];
	dsrc += 4;
	ddst += dspl;
	ddst[0] = dsrc[0];
	ddst[1] = dsrc[1];
	ddst[2] = dsrc[2];
	ddst[3] = dsrc[3];
	dsrc += 4;
	ddst += dspl;
	ddst[0] = dsrc[0];
	ddst[1] = dsrc[1];
	ddst[2] = dsrc[2];
	ddst[3] = dsrc[3];
	dsrc += 4;
	ddst += dspl;
	ddst[0] = dsrc[0];
	ddst[1] = dsrc[1];
	ddst[2] = dsrc[2];
	ddst[3] = dsrc[3];
	dsrc += 4;
	ddst += dspl;
	ddst[0] = dsrc[0];
	ddst[1] = dsrc[1];
	ddst[2] = dsrc[2];
	ddst[3] = dsrc[3];
	dsrc += 4;
	ddst += dspl;
	ddst[0] = dsrc[0];
	ddst[1] = dsrc[1];
	ddst[2] = dsrc[2];
	ddst[3] = dsrc[3];
	dsrc += 4;
	ddst += dspl;
	ddst[0] = dsrc[0];
	ddst[1] = dsrc[1];
	ddst[2] = dsrc[2];
	ddst[3] = dsrc[3];
#endif
}

/*
==============
idCinematicLocal::blit4_32
==============
*/
void idCinematicLocal::blit4_32( byte* src, byte* dst, int spl )
{
#if 1
	int* dsrc, *ddst;
	int dspl;

	dsrc = ( int* )src;
	ddst = ( int* )dst;
	dspl = spl >> 2;

	ddst[0 * dspl + 0] = dsrc[ 0];
	ddst[0 * dspl + 1] = dsrc[ 1];
	ddst[0 * dspl + 2] = dsrc[ 2];
	ddst[0 * dspl + 3] = dsrc[ 3];
	ddst[1 * dspl + 0] = dsrc[ 4];
	ddst[1 * dspl + 1] = dsrc[ 5];
	ddst[1 * dspl + 2] = dsrc[ 6];
	ddst[1 * dspl + 3] = dsrc[ 7];
	ddst[2 * dspl + 0] = dsrc[ 8];
	ddst[2 * dspl + 1] = dsrc[ 9];
	ddst[2 * dspl + 2] = dsrc[10];
	ddst[2 * dspl + 3] = dsrc[11];
	ddst[3 * dspl + 0] = dsrc[12];
	ddst[3 * dspl + 1] = dsrc[13];
	ddst[3 * dspl + 2] = dsrc[14];
	ddst[3 * dspl + 3] = dsrc[15];
#else
	double* dsrc, *ddst;
	int dspl;

	dsrc = ( double* )src;
	ddst = ( double* )dst;
	dspl = spl >> 3;

	ddst[0] = dsrc[0];
	ddst[1] = dsrc[1];
	dsrc += 2;
	ddst += dspl;
	ddst[0] = dsrc[0];
	ddst[1] = dsrc[1];
	dsrc += 2;
	ddst += dspl;
	ddst[0] = dsrc[0];
	ddst[1] = dsrc[1];
	dsrc += 2;
	ddst += dspl;
	ddst[0] = dsrc[0];
	ddst[1] = dsrc[1];
#endif
}

/*
==============
idCinematicLocal::blit2_32
==============
*/
void idCinematicLocal::blit2_32( byte* src, byte* dst, int spl )
{
#if 1
	int* dsrc, *ddst;
	int dspl;

	dsrc = ( int* )src;
	ddst = ( int* )dst;
	dspl = spl >> 2;

	ddst[0 * dspl + 0] = dsrc[0];
	ddst[0 * dspl + 1] = dsrc[1];
	ddst[1 * dspl + 0] = dsrc[2];
	ddst[1 * dspl + 1] = dsrc[3];
#else
	double* dsrc, *ddst;
	int dspl;

	dsrc = ( double* )src;
	ddst = ( double* )dst;
	dspl = spl >> 3;

	ddst[0] = dsrc[0];
	ddst[dspl] = dsrc[1];
#endif
}

/*
==============
idCinematicLocal::blitVQQuad32fs
==============
*/
void idCinematicLocal::blitVQQuad32fs( byte** status, unsigned char* data )
{
	unsigned short	newd, celdata, code;
	unsigned int	index, i;

	newd	= 0;
	celdata = 0;
	index	= 0;

	do
	{
		if( !newd )
		{
			newd = 7;
			celdata = data[0] + data[1] * 256;
			data += 2;
		}
		else
		{
			newd--;
		}

		code = ( unsigned short )( celdata & 0xc000 );
		celdata <<= 2;

		switch( code )
		{
			case	0x8000:													// vq code
				blit8_32( ( byte* )&vq8[( *data ) * 128], status[index], samplesPerLine );
				data++;
				index += 5;
				break;
			case	0xc000:													// drop
				index++;													// skip 8x8
				for( i = 0; i < 4; i++ )
				{
					if( !newd )
					{
						newd = 7;
						celdata = data[0] + data[1] * 256;
						data += 2;
					}
					else
					{
						newd--;
					}

					code = ( unsigned short )( celdata & 0xc000 );
					celdata <<= 2;

					switch( code )  											// code in top two bits of code
					{
						case	0x8000:										// 4x4 vq code
							blit4_32( ( byte* )&vq4[( *data ) * 32], status[index], samplesPerLine );
							data++;
							break;
						case	0xc000:										// 2x2 vq code
							blit2_32( ( byte* )&vq2[( *data ) * 8], status[index], samplesPerLine );
							data++;
							blit2_32( ( byte* )&vq2[( *data ) * 8], status[index] + 8, samplesPerLine );
							data++;
							blit2_32( ( byte* )&vq2[( *data ) * 8], status[index] + samplesPerLine * 2, samplesPerLine );
							data++;
							blit2_32( ( byte* )&vq2[( *data ) * 8], status[index] + samplesPerLine * 2 + 8, samplesPerLine );
							data++;
							break;
						case	0x4000:										// motion compensation
							move4_32( status[index] + mcomp[( *data )], status[index], samplesPerLine );
							data++;
							break;
					}
					index++;
				}
				break;
			case	0x4000:													// motion compensation
				move8_32( status[index] + mcomp[( *data )], status[index], samplesPerLine );
				data++;
				index += 5;
				break;
			case	0x0000:
				index += 5;
				break;
		}
	}
	while( status[index] != NULL );
}

#define VQ2TO4(a,b,c,d) { \
    	*c++ = a[0];	\
	*d++ = a[0];	\
	*d++ = a[0];	\
	*c++ = a[1];	\
	*d++ = a[1];	\
	*d++ = a[1];	\
	*c++ = b[0];	\
	*d++ = b[0];	\
	*d++ = b[0];	\
	*c++ = b[1];	\
	*d++ = b[1];	\
	*d++ = b[1];	\
	*d++ = a[0];	\
	*d++ = a[0];	\
	*d++ = a[1];	\
	*d++ = a[1];	\
	*d++ = b[0];	\
	*d++ = b[0];	\
	*d++ = b[1];	\
	*d++ = b[1];	\
	a += 2; b += 2; }

#define VQ2TO2(a,b,c,d) { \
	*c++ = *a;	\
	*d++ = *a;	\
	*d++ = *a;	\
	*c++ = *b;	\
	*d++ = *b;	\
	*d++ = *b;	\
	*d++ = *a;	\
	*d++ = *a;	\
	*d++ = *b;	\
	*d++ = *b;	\
	a++; b++; }

/*
==============
idCinematicLocal::yuv_to_rgb
==============
*/
// RB: 64 bit fixes, changed long to int
unsigned short idCinematicLocal::yuv_to_rgb( int y, int u, int v )
{
	int r, g, b, YY = ( int )( ROQ_YY_tab[( y )] );

	r = ( YY + ROQ_VR_tab[v] ) >> 9;
	g = ( YY + ROQ_UG_tab[u] + ROQ_VG_tab[v] ) >> 8;
	b = ( YY + ROQ_UB_tab[u] ) >> 9;

	if( r < 0 )
	{
		r = 0;
	}
	if( g < 0 )
	{
		g = 0;
	}
	if( b < 0 )
	{
		b = 0;
	}
	if( r > 31 )
	{
		r = 31;
	}
	if( g > 63 )
	{
		g = 63;
	}
	if( b > 31 )
	{
		b = 31;
	}

	return ( unsigned short )( ( r << 11 ) + ( g << 5 ) + ( b ) );
}
// RB end


/*
==============
idCinematicLocal::yuv_to_rgb24
==============
*/
// RB: 64 bit fixes, changed long to int
unsigned int idCinematicLocal::yuv_to_rgb24( int y, int u, int v )
{
	int r, g, b, YY = ( int )( ROQ_YY_tab[( y )] );

	r = ( YY + ROQ_VR_tab[v] ) >> 6;
	g = ( YY + ROQ_UG_tab[u] + ROQ_VG_tab[v] ) >> 6;
	b = ( YY + ROQ_UB_tab[u] ) >> 6;

	if( r < 0 )
	{
		r = 0;
	}
	if( g < 0 )
	{
		g = 0;
	}
	if( b < 0 )
	{
		b = 0;
	}
	if( r > 255 )
	{
		r = 255;
	}
	if( g > 255 )
	{
		g = 255;
	}
	if( b > 255 )
	{
		b = 255;
	}

	return LittleLong( ( r ) + ( g << 8 ) + ( b << 16 ) );
}
// RB end

/*
==============
idCinematicLocal::decodeCodeBook
==============
*/
// RB: 64 bit fixes, changed long to int
void idCinematicLocal::decodeCodeBook( byte* input, unsigned short roq_flags )
{
	int	i, j, two, four;
	unsigned short*	aptr, *bptr, *cptr, *dptr;
	int	y0, y1, y2, y3, cr, cb;
	unsigned int* iaptr, *ibptr, *icptr, *idptr;

	if( !roq_flags )
	{
		two = four = 256;
	}
	else
	{
		two  = roq_flags >> 8;
		if( !two )
		{
			two = 256;
		}
		four = roq_flags & 0xff;
	}

	four *= 2;

	bptr = ( unsigned short* )vq2;

	if( !half )
	{
		if( !smootheddouble )
		{
//
// normal height
//
			if( samplesPerPixel == 2 )
			{
				for( i = 0; i < two; i++ )
				{
					y0 = ( int ) * input++;
					y1 = ( int ) * input++;
					y2 = ( int ) * input++;
					y3 = ( int ) * input++;
					cr = ( int ) * input++;
					cb = ( int ) * input++;
					*bptr++ = yuv_to_rgb( y0, cr, cb );
					*bptr++ = yuv_to_rgb( y1, cr, cb );
					*bptr++ = yuv_to_rgb( y2, cr, cb );
					*bptr++ = yuv_to_rgb( y3, cr, cb );
				}

				cptr = ( unsigned short* )vq4;
				dptr = ( unsigned short* )vq8;

				for( i = 0; i < four; i++ )
				{
					aptr = ( unsigned short* )vq2 + ( *input++ ) * 4;
					bptr = ( unsigned short* )vq2 + ( *input++ ) * 4;
					for( j = 0; j < 2; j++ )
					{
						VQ2TO4( aptr, bptr, cptr, dptr );
					}
				}
			}
			else if( samplesPerPixel == 4 )
			{
				ibptr = ( unsigned int* )bptr;
				for( i = 0; i < two; i++ )
				{
					y0 = ( int ) * input++;
					y1 = ( int ) * input++;
					y2 = ( int ) * input++;
					y3 = ( int ) * input++;
					cr = ( int ) * input++;
					cb = ( int ) * input++;
					*ibptr++ = yuv_to_rgb24( y0, cr, cb );
					*ibptr++ = yuv_to_rgb24( y1, cr, cb );
					*ibptr++ = yuv_to_rgb24( y2, cr, cb );
					*ibptr++ = yuv_to_rgb24( y3, cr, cb );
				}

				icptr = ( unsigned int* )vq4;
				idptr = ( unsigned int* )vq8;

				for( i = 0; i < four; i++ )
				{
					iaptr = ( unsigned int* )vq2 + ( *input++ ) * 4;
					ibptr = ( unsigned int* )vq2 + ( *input++ ) * 4;
					for( j = 0; j < 2; j++ )
					{
						VQ2TO4( iaptr, ibptr, icptr, idptr );
					}
				}
			}
		}
		else
		{
//
// double height, smoothed
//
			if( samplesPerPixel == 2 )
			{
				for( i = 0; i < two; i++ )
				{
					y0 = ( int ) * input++;
					y1 = ( int ) * input++;
					y2 = ( int ) * input++;
					y3 = ( int ) * input++;
					cr = ( int ) * input++;
					cb = ( int ) * input++;
					*bptr++ = yuv_to_rgb( y0, cr, cb );
					*bptr++ = yuv_to_rgb( y1, cr, cb );
					*bptr++ = yuv_to_rgb( ( ( y0 * 3 ) + y2 ) / 4, cr, cb );
					*bptr++ = yuv_to_rgb( ( ( y1 * 3 ) + y3 ) / 4, cr, cb );
					*bptr++ = yuv_to_rgb( ( y0 + ( y2 * 3 ) ) / 4, cr, cb );
					*bptr++ = yuv_to_rgb( ( y1 + ( y3 * 3 ) ) / 4, cr, cb );
					*bptr++ = yuv_to_rgb( y2, cr, cb );
					*bptr++ = yuv_to_rgb( y3, cr, cb );
				}

				cptr = ( unsigned short* )vq4;
				dptr = ( unsigned short* )vq8;

				for( i = 0; i < four; i++ )
				{
					aptr = ( unsigned short* )vq2 + ( *input++ ) * 8;
					bptr = ( unsigned short* )vq2 + ( *input++ ) * 8;
					for( j = 0; j < 2; j++ )
					{
						VQ2TO4( aptr, bptr, cptr, dptr );
						VQ2TO4( aptr, bptr, cptr, dptr );
					}
				}
			}
			else if( samplesPerPixel == 4 )
			{
				ibptr = ( unsigned int* )bptr;
				for( i = 0; i < two; i++ )
				{
					y0 = ( int ) * input++;
					y1 = ( int ) * input++;
					y2 = ( int ) * input++;
					y3 = ( int ) * input++;
					cr = ( int ) * input++;
					cb = ( int ) * input++;
					*ibptr++ = yuv_to_rgb24( y0, cr, cb );
					*ibptr++ = yuv_to_rgb24( y1, cr, cb );
					*ibptr++ = yuv_to_rgb24( ( ( y0 * 3 ) + y2 ) / 4, cr, cb );
					*ibptr++ = yuv_to_rgb24( ( ( y1 * 3 ) + y3 ) / 4, cr, cb );
					*ibptr++ = yuv_to_rgb24( ( y0 + ( y2 * 3 ) ) / 4, cr, cb );
					*ibptr++ = yuv_to_rgb24( ( y1 + ( y3 * 3 ) ) / 4, cr, cb );
					*ibptr++ = yuv_to_rgb24( y2, cr, cb );
					*ibptr++ = yuv_to_rgb24( y3, cr, cb );
				}

				icptr = ( unsigned int* )vq4;
				idptr = ( unsigned int* )vq8;

				for( i = 0; i < four; i++ )
				{
					iaptr = ( unsigned int* )vq2 + ( *input++ ) * 8;
					ibptr = ( unsigned int* )vq2 + ( *input++ ) * 8;
					for( j = 0; j < 2; j++ )
					{
						VQ2TO4( iaptr, ibptr, icptr, idptr );
						VQ2TO4( iaptr, ibptr, icptr, idptr );
					}
				}
			}
		}
	}
	else
	{
//
// 1/4 screen
//
		if( samplesPerPixel == 2 )
		{
			for( i = 0; i < two; i++ )
			{
				y0 = ( int ) * input;
				input += 2;
				y2 = ( int ) * input;
				input += 2;
				cr = ( int ) * input++;
				cb = ( int ) * input++;
				*bptr++ = yuv_to_rgb( y0, cr, cb );
				*bptr++ = yuv_to_rgb( y2, cr, cb );
			}

			cptr = ( unsigned short* )vq4;
			dptr = ( unsigned short* )vq8;

			for( i = 0; i < four; i++ )
			{
				aptr = ( unsigned short* )vq2 + ( *input++ ) * 2;
				bptr = ( unsigned short* )vq2 + ( *input++ ) * 2;
				for( j = 0; j < 2; j++ )
				{
					VQ2TO2( aptr, bptr, cptr, dptr );
				}
			}
		}
		else if( samplesPerPixel == 4 )
		{
			ibptr = ( unsigned int* ) bptr;
			for( i = 0; i < two; i++ )
			{
				y0 = ( int ) * input;
				input += 2;
				y2 = ( int ) * input;
				input += 2;
				cr = ( int ) * input++;
				cb = ( int ) * input++;
				*ibptr++ = yuv_to_rgb24( y0, cr, cb );
				*ibptr++ = yuv_to_rgb24( y2, cr, cb );
			}

			icptr = ( unsigned int* )vq4;
			idptr = ( unsigned int* )vq8;

			for( i = 0; i < four; i++ )
			{
				iaptr = ( unsigned int* )vq2 + ( *input++ ) * 2;
				ibptr = ( unsigned int* )vq2 + ( *input++ ) * 2;
				for( j = 0; j < 2; j++ )
				{
					VQ2TO2( iaptr, ibptr, icptr, idptr );
				}
			}
		}
	}
}
// RB end

/*
==============
idCinematicLocal::recurseQuad
==============
*/
// RB: 64 bit fixes, changed long to int
void idCinematicLocal::recurseQuad( int startX, int startY, int quadSize, int xOff, int yOff )
{
	byte* scroff;
	int bigx, bigy, lowx, lowy, useY;
	int offset;

	offset = screenDelta;

	lowx = lowy = 0;
	bigx = xsize;
	bigy = ysize;

	if( bigx > CIN_WIDTH )
	{
		bigx = CIN_WIDTH;
	}
	if( bigy > CIN_HEIGHT )
	{
		bigy = CIN_HEIGHT;
	}

	if( ( startX >= lowx ) && ( startX + quadSize ) <= ( bigx ) && ( startY + quadSize ) <= ( bigy ) && ( startY >= lowy ) && quadSize <= MAXSIZE )
	{
		useY = startY;
		scroff = image + ( useY + ( ( CIN_HEIGHT - bigy ) >> 1 ) + yOff ) * ( samplesPerLine ) + ( ( ( startX + xOff ) ) * samplesPerPixel );

		qStatus[0][onQuad  ] = scroff;
		qStatus[1][onQuad++] = scroff + offset;
	}

	if( quadSize != MINSIZE )
	{
		quadSize >>= 1;
		recurseQuad( startX,		  startY		  , quadSize, xOff, yOff );
		recurseQuad( startX + quadSize, startY		  , quadSize, xOff, yOff );
		recurseQuad( startX,		  startY + quadSize , quadSize, xOff, yOff );
		recurseQuad( startX + quadSize, startY + quadSize , quadSize, xOff, yOff );
	}
}
// RB end

/*
==============
idCinematicLocal::setupQuad
==============
*/
// RB: 64 bit fixes, changed long to int
void idCinematicLocal::setupQuad( int xOff, int yOff )
{
	int numQuadCels, i, x, y;
	byte* temp;

	numQuadCels  = ( CIN_WIDTH * CIN_HEIGHT ) / ( 16 );
	numQuadCels += numQuadCels / 4 + numQuadCels / 16;
	numQuadCels += 64;							  // for overflow

	numQuadCels  = ( xsize * ysize ) / ( 16 );
	numQuadCels += numQuadCels / 4;
	numQuadCels += 64;							  // for overflow

	onQuad = 0;

	for( y = 0; y < ( int )ysize; y += 16 )
		for( x = 0; x < ( int )xsize; x += 16 )
		{
			recurseQuad( x, y, 16, xOff, yOff );
		}

	temp = NULL;

	for( i = ( numQuadCels - 64 ); i < numQuadCels; i++ )
	{
		qStatus[0][i] = temp;			  // eoq
		qStatus[1][i] = temp;			  // eoq
	}
}
// RB end

/*
==============
idCinematicLocal::readQuadInfo
==============
*/
void idCinematicLocal::readQuadInfo( byte* qData )
{
	xsize    = qData[0] + qData[1] * 256;
	ysize    = qData[2] + qData[3] * 256;
	maxsize  = qData[4] + qData[5] * 256;
	minsize  = qData[6] + qData[7] * 256;

	CIN_HEIGHT = ysize;
	CIN_WIDTH  = xsize;

	samplesPerLine = CIN_WIDTH * samplesPerPixel;
	screenDelta = CIN_HEIGHT * samplesPerLine;

	if( !image )
	{
		image = ( byte* )Mem_Alloc( CIN_WIDTH * CIN_HEIGHT * samplesPerPixel * 2, TAG_CINEMATIC );
	}

	half = false;
	smootheddouble = false;

	// RB: 64 bit fixes, changed unsigned int to ptrdiff_t
	t[0] = ( 0 - ( ptrdiff_t )image ) + ( ptrdiff_t )image + screenDelta;
	t[1] = ( 0 - ( ( ptrdiff_t )image + screenDelta ) ) + ( ptrdiff_t )image;
	// RB end

	drawX = CIN_WIDTH;
	drawY = CIN_HEIGHT;
}

/*
==============
idCinematicLocal::RoQPrepMcomp
==============
*/
// RB: 64 bit fixes, changed long to int
void idCinematicLocal::RoQPrepMcomp( int xoff, int yoff )
{
	int i, j, x, y, temp, temp2;

	i = samplesPerLine;
	j = samplesPerPixel;
	if( xsize == ( ysize * 4 ) && !half )
	{
		j = j + j;
		i = i + i;
	}

	for( y = 0; y < 16; y++ )
	{
		temp2 = ( y + yoff - 8 ) * i;
		for( x = 0; x < 16; x++ )
		{
			temp = ( x + xoff - 8 ) * j;
			mcomp[( x * 16 ) + y] = normalBuffer0 - ( temp2 + temp );
		}
	}
}
// RB end

/*
==============
idCinematicLocal::RoQReset
==============
*/
void idCinematicLocal::RoQReset()
{

	iFile->Seek( 0, FS_SEEK_SET );
	iFile->Read( file, 16 );
	RoQ_init();
	status = FMV_LOOPED;
}


typedef struct
{
	struct jpeg_source_mgr pub;	/* public fields */

	byte*   infile;		/* source stream */
	JOCTET* buffer;		/* start of buffer */
	boolean start_of_file;	/* have we gotten any data yet? */
	int	memsize;
} my_source_mgr;

typedef my_source_mgr* my_src_ptr;

#define INPUT_BUF_SIZE  32768	/* choose an efficiently fread'able size */

/* jpeg error handling */
struct jpeg_error_mgr jerr;

/*
 * Fill the input buffer --- called whenever buffer is emptied.
 *
 * In typical applications, this should read fresh data into the buffer
 * (ignoring the current state of next_input_byte & bytes_in_buffer),
 * reset the pointer & count to the start of the buffer, and return TRUE
 * indicating that the buffer has been reloaded.  It is not necessary to
 * fill the buffer entirely, only to obtain at least one more byte.
 *
 * There is no such thing as an EOF return.  If the end of the file has been
 * reached, the routine has a choice of ERREXIT() or inserting fake data into
 * the buffer.  In most cases, generating a warning message and inserting a
 * fake EOI marker is the best course of action --- this will allow the
 * decompressor to output however much of the image is there.  However,
 * the resulting error message is misleading if the real problem is an empty
 * input file, so we handle that case specially.
 *
 * In applications that need to be able to suspend compression due to input
 * not being available yet, a FALSE return indicates that no more data can be
 * obtained right now, but more may be forthcoming later.  In this situation,
 * the decompressor will return to its caller (with an indication of the
 * number of scanlines it has read, if any).  The application should resume
 * decompression after it has loaded more data into the input buffer.  Note
 * that there are substantial restrictions on the use of suspension --- see
 * the documentation.
 *
 * When suspending, the decompressor will back up to a convenient restart point
 * (typically the start of the current MCU). next_input_byte & bytes_in_buffer
 * indicate where the restart point will be if the current call returns FALSE.
 * Data beyond this point must be rescanned after resumption, so move it to
 * the front of the buffer rather than discarding it.
 */

#ifdef USE_NEWER_JPEG
	METHODDEF( boolean )
#else
	METHODDEF boolean
#endif
fill_input_buffer( j_decompress_ptr cinfo )
{
	my_src_ptr src = ( my_src_ptr ) cinfo->src;
	int nbytes;

	nbytes = INPUT_BUF_SIZE;
	if( nbytes > src->memsize )
	{
		nbytes = src->memsize;
	}
	if( nbytes == 0 )
	{
		/* Insert a fake EOI marker */
		src->buffer[0] = ( JOCTET ) 0xFF;
		src->buffer[1] = ( JOCTET ) JPEG_EOI;
		nbytes = 2;
	}
	else
	{
		memcpy( src->buffer, src->infile, INPUT_BUF_SIZE );
		src->infile = src->infile + nbytes;
		src->memsize = src->memsize - INPUT_BUF_SIZE;
	}
	src->pub.next_input_byte = src->buffer;
	src->pub.bytes_in_buffer = nbytes;
	src->start_of_file = FALSE;

	return TRUE;
}
/*
 * Initialize source --- called by jpeg_read_header
 * before any data is actually read.
 */

#ifdef USE_NEWER_JPEG
	METHODDEF( void )
#else
	METHODDEF void
#endif
init_source( j_decompress_ptr cinfo )
{
	my_src_ptr src = ( my_src_ptr ) cinfo->src;

	/* We reset the empty-input-file flag for each image,
	 * but we don't clear the input buffer.
	 * This is correct behavior for reading a series of images from one source.
	 */
	src->start_of_file = TRUE;
}

/*
 * Skip data --- used to skip over a potentially large amount of
 * uninteresting data (such as an APPn marker).
 *
 * Writers of suspendable-input applications must note that skip_input_data
 * is not granted the right to give a suspension return.  If the skip extends
 * beyond the data currently in the buffer, the buffer can be marked empty so
 * that the next read will cause a fill_input_buffer call that can suspend.
 * Arranging for additional bytes to be discarded before reloading the input
 * buffer is the application writer's problem.
 */

#ifdef USE_NEWER_JPEG
	METHODDEF( void )
#else
	METHODDEF void
#endif
skip_input_data( j_decompress_ptr cinfo, long num_bytes )
{
	my_src_ptr src = ( my_src_ptr ) cinfo->src;

	/* Just a dumb implementation for now.  Could use fseek() except
	 * it doesn't work on pipes.  Not clear that being smart is worth
	 * any trouble anyway --- large skips are infrequent.
	 */
	if( num_bytes > 0 )
	{
		src->infile = src->infile + num_bytes;
		src->pub.next_input_byte += ( size_t ) num_bytes;
		src->pub.bytes_in_buffer -= ( size_t ) num_bytes;
	}
}


/*
 * An additional method that can be provided by data source modules is the
 * resync_to_restart method for error recovery in the presence of RST markers.
 * For the moment, this source module just uses the default resync method
 * provided by the JPEG library.  That method assumes that no backtracking
 * is possible.
 */


/*
 * Terminate source --- called by jpeg_finish_decompress
 * after all data has been read.  Often a no-op.
 *
 * NB: *not* called by jpeg_abort or jpeg_destroy; surrounding
 * application must deal with any cleanup that should happen even
 * for error exit.
 */

#ifdef USE_NEWER_JPEG
	METHODDEF( void )
#else
	METHODDEF void
#endif
term_source( j_decompress_ptr cinfo )
{
	cinfo = cinfo;
	/* no work necessary here */
}

#ifdef USE_NEWER_JPEG
	GLOBAL( void )
#else
	GLOBAL void
#endif
jpeg_memory_src( j_decompress_ptr cinfo, byte* infile, int size )
{
	my_src_ptr src;

	/* The source object and input buffer are made permanent so that a series
	 * of JPEG images can be read from the same file by calling jpeg_stdio_src
	 * only before the first one.  (If we discarded the buffer at the end of
	 * one image, we'd likely lose the start of the next one.)
	 * This makes it unsafe to use this manager and a different source
	 * manager serially with the same JPEG object.  Caveat programmer.
	 */
	if( cinfo->src == NULL )  	/* first time for this JPEG object? */
	{
		cinfo->src = ( struct jpeg_source_mgr* )
					 ( *cinfo->mem->alloc_small )( ( j_common_ptr ) cinfo, JPOOL_PERMANENT,
							 sizeof( my_source_mgr ) );
		src = ( my_src_ptr ) cinfo->src;
		src->buffer = ( JOCTET* )
					  ( *cinfo->mem->alloc_small )( ( j_common_ptr ) cinfo, JPOOL_PERMANENT,
							  INPUT_BUF_SIZE * sizeof( JOCTET ) );
	}

	src = ( my_src_ptr ) cinfo->src;
	src->pub.init_source = init_source;
	src->pub.fill_input_buffer = fill_input_buffer;
	src->pub.skip_input_data = skip_input_data;
	src->pub.resync_to_restart = jpeg_resync_to_restart; /* use default method */
	src->pub.term_source = term_source;
	src->infile = infile;
	src->memsize = size;
	src->pub.bytes_in_buffer = 0; /* forces fill_input_buffer on first read */
	src->pub.next_input_byte = NULL; /* until buffer loaded */
}

int JPEGBlit( byte* wStatus, byte* data, int datasize )
{
	/* This struct contains the JPEG decompression parameters and pointers to
	 * working space (which is allocated as needed by the JPEG library).
	 */
	struct jpeg_decompress_struct cinfo;
	/* We use our private extension JPEG error handler.
	 * Note that this struct must live as long as the main JPEG parameter
	 * struct, to avoid dangling-pointer problems.
	 */
	/* More stuff */
	JSAMPARRAY buffer;		/* Output row buffer */
	int row_stride;		/* physical row width in output buffer */

	/* Step 1: allocate and initialize JPEG decompression object */

	/* We set up the normal JPEG error routines, then override error_exit. */
	cinfo.err = jpeg_std_error( &jerr );

	/* Now we can initialize the JPEG decompression object. */
	jpeg_create_decompress( &cinfo );

	/* Step 2: specify data source (eg, a file) */

	jpeg_memory_src( &cinfo, data, datasize );

	/* Step 3: read file parameters with jpeg_read_header() */

	jpeg_read_header( &cinfo, TRUE );
	/* We can ignore the return value from jpeg_read_header since
	 *   (a) suspension is not possible with the stdio data source, and
	 *   (b) we passed TRUE to reject a tables-only JPEG file as an error.
	 * See libjpeg.doc for more info.
	 */

	/* Step 4: set parameters for decompression */

	/* In this example, we don't need to change any of the defaults set by
	 * jpeg_read_header(), so we do nothing here.
	 */

	/* Step 5: Start decompressor */

	cinfo.dct_method = JDCT_IFAST;
	cinfo.dct_method = JDCT_FASTEST;
	cinfo.dither_mode = JDITHER_NONE;
	cinfo.do_fancy_upsampling = FALSE;
//	cinfo.out_color_space = JCS_GRAYSCALE;

	jpeg_start_decompress( &cinfo );
	/* We can ignore the return value since suspension is not possible
	 * with the stdio data source.
	 */

	/* We may need to do some setup of our own at this point before reading
	 * the data.  After jpeg_start_decompress() we have the correct scaled
	 * output image dimensions available, as well as the output colormap
	 * if we asked for color quantization.
	 * In this example, we need to make an output work buffer of the right size.
	 */
	/* JSAMPLEs per row in output buffer */
	row_stride = cinfo.output_width * cinfo.output_components;

	/* Make a one-row-high sample array that will go away when done with image */
	buffer = ( *cinfo.mem->alloc_sarray )
			 ( ( j_common_ptr ) &cinfo, JPOOL_IMAGE, row_stride, 1 );

	/* Step 6: while (scan lines remain to be read) */
	/*           jpeg_read_scanlines(...); */

	/* Here we use the library's state variable cinfo.output_scanline as the
	 * loop counter, so that we don't have to keep track ourselves.
	 */

	wStatus += ( cinfo.output_height - 1 ) * row_stride;
	while( cinfo.output_scanline < cinfo.output_height )
	{
		/* jpeg_read_scanlines expects an array of pointers to scanlines.
		 * Here the array is only one element long, but you could ask for
		 * more than one scanline at a time if that's more convenient.
		 */
		jpeg_read_scanlines( &cinfo, &buffer[0], 1 );

		/* Assume put_scanline_someplace wants a pointer and sample count. */
		memcpy( wStatus, &buffer[0][0], row_stride );
		/*
		int x;
		unsigned int *buf = (unsigned int *)&buffer[0][0];
		unsigned int *out = (unsigned int *)wStatus;
		for(x=0;x<cinfo.output_width;x++) {
			unsigned int pixel = buf[x];
			byte *roof = (byte *)&pixel;
			byte temp = roof[0];
			roof[0] = roof[2];
			roof[2] = temp;
			out[x] = pixel;
		}
		*/
		wStatus -= row_stride;
	}

	/* Step 7: Finish decompression */

	jpeg_finish_decompress( &cinfo );
	/* We can ignore the return value since suspension is not possible
	 * with the stdio data source.
	 */

	/* Step 8: Release JPEG decompression object */

	/* This is an important step since it will release a good deal of memory. */
	jpeg_destroy_decompress( &cinfo );

	/* At this point you may want to check to see whether any corrupt-data
	 * warnings occurred (test whether jerr.pub.num_warnings is nonzero).
	 */

	/* And we're done! */
	return 1;
}

/*
==============
idCinematicLocal::RoQInterrupt
==============
*/
void idCinematicLocal::RoQInterrupt()
{
	byte*				framedata;

	iFile->Read( file, RoQFrameSize + 8 );
	if( RoQPlayed >= ROQSize )
	{
		if( looping )
		{
			RoQReset();
		}
		else
		{
			status = FMV_EOF;
		}
		return;
	}

	framedata = file;
//
// new frame is ready
//
redump:
	switch( roq_id )
	{
		case	ROQ_QUAD_VQ:
			if( ( numQuads & 1 ) )
			{
				normalBuffer0 = t[1];
				RoQPrepMcomp( roqF0, roqF1 );
				blitVQQuad32fs( qStatus[1], framedata );
				buf = 	image + screenDelta;
			}
			else
			{
				normalBuffer0 = t[0];
				RoQPrepMcomp( roqF0, roqF1 );
				blitVQQuad32fs( qStatus[0], framedata );
				buf = 	image;
			}
			if( numQuads == 0 )  		// first frame
			{
				memcpy( image + screenDelta, image, samplesPerLine * ysize );
			}
			numQuads++;
			dirty = true;
			break;
		case	ROQ_CODEBOOK:
			decodeCodeBook( framedata, ( unsigned short )roq_flags );
			break;
		case	ZA_SOUND_MONO:
			break;
		case	ZA_SOUND_STEREO:
			break;
		case	ROQ_QUAD_INFO:
			if( numQuads == -1 )
			{
				readQuadInfo( framedata );
				setupQuad( 0, 0 );
			}
			if( numQuads != 1 )
			{
				numQuads = 0;
			}
			break;
		case	ROQ_PACKET:
			inMemory = ( roq_flags != 0 );
			RoQFrameSize = 0;           // for header
			break;
		case	ROQ_QUAD_HANG:
			RoQFrameSize = 0;
			break;
		case	ROQ_QUAD_JPEG:
			if( !numQuads )
			{
				normalBuffer0 = t[0];
				JPEGBlit( image, framedata, RoQFrameSize );
				memcpy( image + screenDelta, image, samplesPerLine * ysize );
				numQuads++;
			}
			break;
		default:
			status = FMV_EOF;
			break;
	}
//
// read in next frame data
//
	if( RoQPlayed >= ROQSize || status == FMV_EOF )		// SRS - handle FMV_EOF case
	{
		if( looping )
		{
			RoQReset();
		}
		else
		{
			status = FMV_EOF;
		}
		return;
	}

	framedata		 += RoQFrameSize;
	roq_id		 = framedata[0] + framedata[1] * 256;
	RoQFrameSize = framedata[2] + framedata[3] * 256 + framedata[4] * 65536;
	roq_flags	 = framedata[6] + framedata[7] * 256;
	roqF0		 = ( char )framedata[7];
	roqF1		 = ( char )framedata[6];

	if( RoQFrameSize > 65536 || roq_id == 0x1084 )
	{
		common->DPrintf( "roq_size>65536||roq_id==0x1084\n" );
		status = FMV_EOF;
		if( looping )
		{
			RoQReset();
		}
		return;
	}
	if( inMemory && ( status != FMV_EOF ) )
	{
		inMemory = false;
		framedata += 8;
		goto redump;
	}
//
// one more frame hits the dust
//
//	assert(RoQFrameSize <= 65536);
//	r = Sys_StreamedRead( file, RoQFrameSize+8, 1, iFile );
	RoQPlayed	+= RoQFrameSize + 8;
}

/*
==============
idCinematicLocal::RoQ_init
==============
*/
void idCinematicLocal::RoQ_init()
{

	RoQPlayed = 24;

	/*	get frame rate */
	roqFPS	 = file[ 6] + file[ 7] * 256;

	if( !roqFPS )
	{
		roqFPS = 30;
	}

	numQuads = -1;

	roq_id		= file[ 8] + file[ 9] * 256;
	RoQFrameSize = file[10] + file[11] * 256 + file[12] * 65536;
	roq_flags	= file[14] + file[15] * 256;
}

/*
==============
idCinematicLocal::RoQShutdown
==============
*/
void idCinematicLocal::RoQShutdown()
{
	// SRS - Depending on status, this could prevent closing of iFile on shutdown, disable it
	/*
	if( status == FMV_IDLE )
	{
		return;
	}
	*/
	status = FMV_EOF;	// SRS - Changed from FMV_IDLE to FMV_EOF for shutdown consistency

	if( iFile )
	{
		fileSystem->CloseFile( iFile );
		iFile = NULL;
	}

	fileName = "";
}

//===========================================

/*
==============
idSndWindow::InitFromFile
==============
*/
bool idSndWindow::InitFromFile( const char* qpath, bool looping )
{
	idStr fname = qpath;

	fname.ToLower();
	if( !fname.Icmp( "waveform" ) )
	{
		showWaveform = true;
	}
	else
	{
		showWaveform = false;
	}
	return true;
}

/*
==============
idSndWindow::ImageForTime
==============
*/
cinData_t idSndWindow::ImageForTime( int milliseconds )
{
	return soundSystem->ImageForTime( milliseconds, showWaveform );
}

/*
==============
idSndWindow::AnimationLength
==============
*/
int idSndWindow::AnimationLength()
{
	return -1;
}
