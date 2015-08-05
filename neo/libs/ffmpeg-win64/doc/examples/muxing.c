/*
 * Copyright (c) 2003 Fabrice Bellard
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/**
 * @file
 * libavformat API example.
 *
 * Output a media file in any supported libavformat format. The default
 * codecs are used.
 * @example muxing.c
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <libavutil/opt.h>
#include <libavutil/mathematics.h>
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>

static int audio_is_eof, video_is_eof;

#define STREAM_DURATION   10.0
#define STREAM_FRAME_RATE 25 /* 25 images/s */
#define STREAM_PIX_FMT    AV_PIX_FMT_YUV420P /* default pix_fmt */

static int sws_flags = SWS_BICUBIC;

static void log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt)
{
    AVRational *time_base = &fmt_ctx->streams[pkt->stream_index]->time_base;

    printf("pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d\n",
           av_ts2str(pkt->pts), av_ts2timestr(pkt->pts, time_base),
           av_ts2str(pkt->dts), av_ts2timestr(pkt->dts, time_base),
           av_ts2str(pkt->duration), av_ts2timestr(pkt->duration, time_base),
           pkt->stream_index);
}

static int write_frame(AVFormatContext *fmt_ctx, const AVRational *time_base, AVStream *st, AVPacket *pkt)
{
    /* rescale output packet timestamp values from codec to stream timebase */
    pkt->pts = av_rescale_q_rnd(pkt->pts, *time_base, st->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
    pkt->dts = av_rescale_q_rnd(pkt->dts, *time_base, st->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
    pkt->duration = av_rescale_q(pkt->duration, *time_base, st->time_base);
    pkt->stream_index = st->index;

    /* Write the compressed frame to the media file. */
    log_packet(fmt_ctx, pkt);
    return av_interleaved_write_frame(fmt_ctx, pkt);
}

/* Add an output stream. */
static AVStream *add_stream(AVFormatContext *oc, AVCodec **codec,
                            enum AVCodecID codec_id)
{
    AVCodecContext *c;
    AVStream *st;

    /* find the encoder */
    *codec = avcodec_find_encoder(codec_id);
    if (!(*codec)) {
        fprintf(stderr, "Could not find encoder for '%s'\n",
                avcodec_get_name(codec_id));
        exit(1);
    }

    st = avformat_new_stream(oc, *codec);
    if (!st) {
        fprintf(stderr, "Could not allocate stream\n");
        exit(1);
    }
    st->id = oc->nb_streams-1;
    c = st->codec;

    switch ((*codec)->type) {
    case AVMEDIA_TYPE_AUDIO:
        c->sample_fmt  = (*codec)->sample_fmts ?
            (*codec)->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
        c->bit_rate    = 64000;
        c->sample_rate = 44100;
        c->channels    = 2;
        break;

    case AVMEDIA_TYPE_VIDEO:
        c->codec_id = codec_id;

        c->bit_rate = 400000;
        /* Resolution must be a multiple of two. */
        c->width    = 352;
        c->height   = 288;
        /* timebase: This is the fundamental unit of time (in seconds) in terms
         * of which frame timestamps are represented. For fixed-fps content,
         * timebase should be 1/framerate and timestamp increments should be
         * identical to 1. */
        c->time_base.den = STREAM_FRAME_RATE;
        c->time_base.num = 1;
        c->gop_size      = 12; /* emit one intra frame every twelve frames at most */
        c->pix_fmt       = STREAM_PIX_FMT;
        if (c->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
            /* just for testing, we also add B frames */
            c->max_b_frames = 2;
        }
        if (c->codec_id == AV_CODEC_ID_MPEG1VIDEO) {
            /* Needed to avoid using macroblocks in which some coeffs overflow.
             * This does not happen with normal video, it just happens here as
             * the motion of the chroma plane does not match the luma plane. */
            c->mb_decision = 2;
        }
    break;

    default:
        break;
    }

    /* Some formats want stream headers to be separate. */
    if (oc->oformat->flags & AVFMT_GLOBALHEADER)
        c->flags |= CODEC_FLAG_GLOBAL_HEADER;

    return st;
}

/**************************************************************/
/* audio output */

static float t, tincr, tincr2;

AVFrame *audio_frame;
static uint8_t **src_samples_data;
static int       src_samples_linesize;
static int       src_nb_samples;

static int max_dst_nb_samples;
uint8_t **dst_samples_data;
int       dst_samples_linesize;
int       dst_samples_size;
int samples_count;

struct SwrContext *swr_ctx = NULL;

static void open_audio(AVFormatContext *oc, AVCodec *codec, AVStream *st)
{
    AVCodecContext *c;
    int ret;

    c = st->codec;

    /* allocate and init a re-usable frame */
    audio_frame = av_frame_alloc();
    if (!audio_frame) {
        fprintf(stderr, "Could not allocate audio frame\n");
        exit(1);
    }

    /* open it */
    ret = avcodec_open2(c, codec, NULL);
    if (ret < 0) {
        fprintf(stderr, "Could not open audio codec: %s\n", av_err2str(ret));
        exit(1);
    }

    /* init signal generator */
    t     = 0;
    tincr = 2 * M_PI * 110.0 / c->sample_rate;
    /* increment frequency by 110 Hz per second */
    tincr2 = 2 * M_PI * 110.0 / c->sample_rate / c->sample_rate;

    src_nb_samples = c->codec->capabilities & CODEC_CAP_VARIABLE_FRAME_SIZE ?
        10000 : c->frame_size;

    ret = av_samples_alloc_array_and_samples(&src_samples_data, &src_samples_linesize, c->channels,
                                             src_nb_samples, AV_SAMPLE_FMT_S16, 0);
    if (ret < 0) {
        fprintf(stderr, "Could not allocate source samples\n");
        exit(1);
    }

    /* compute the number of converted samples: buffering is avoided
     * ensuring that the output buffer will contain at least all the
     * converted input samples */
    max_dst_nb_samples = src_nb_samples;

    /* create resampler context */
    if (c->sample_fmt != AV_SAMPLE_FMT_S16) {
        swr_ctx = swr_alloc();
        if (!swr_ctx) {
            fprintf(stderr, "Could not allocate resampler context\n");
            exit(1);
        }

        /* set options */
        av_opt_set_int       (swr_ctx, "in_channel_count",   c->channels,       0);
        av_opt_set_int       (swr_ctx, "in_sample_rate",     c->sample_rate,    0);
        av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt",      AV_SAMPLE_FMT_S16, 0);
        av_opt_set_int       (swr_ctx, "out_channel_count",  c->channels,       0);
        av_opt_set_int       (swr_ctx, "out_sample_rate",    c->sample_rate,    0);
        av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt",     c->sample_fmt,     0);

        /* initialize the resampling context */
        if ((ret = swr_init(swr_ctx)) < 0) {
            fprintf(stderr, "Failed to initialize the resampling context\n");
            exit(1);
        }

        ret = av_samples_alloc_array_and_samples(&dst_samples_data, &dst_samples_linesize, c->channels,
                                                 max_dst_nb_samples, c->sample_fmt, 0);
        if (ret < 0) {
            fprintf(stderr, "Could not allocate destination samples\n");
            exit(1);
        }
    } else {
        dst_samples_data = src_samples_data;
    }
    dst_samples_size = av_samples_get_buffer_size(NULL, c->channels, max_dst_nb_samples,
                                                  c->sample_fmt, 0);
}

/* Prepare a 16 bit dummy audio frame of 'frame_size' samples and
 * 'nb_channels' channels. */
static void get_audio_frame(int16_t *samples, int frame_size, int nb_channels)
{
    int j, i, v;
    int16_t *q;

    q = samples;
    for (j = 0; j < frame_size; j++) {
        v = (int)(sin(t) * 10000);
        for (i = 0; i < nb_channels; i++)
            *q++ = v;
        t     += tincr;
        tincr += tincr2;
    }
}

static void write_audio_frame(AVFormatContext *oc, AVStream *st, int flush)
{
    AVCodecContext *c;
    AVPacket pkt = { 0 }; // data and size must be 0;
    int got_packet, ret, dst_nb_samples;

    av_init_packet(&pkt);
    c = st->codec;

    if (!flush) {
        get_audio_frame((int16_t *)src_samples_data[0], src_nb_samples, c->channels);

        /* convert samples from native format to destination codec format, using the resampler */
        if (swr_ctx) {
            /* compute destination number of samples */
            dst_nb_samples = av_rescale_rnd(swr_get_delay(swr_ctx, c->sample_rate) + src_nb_samples,
                                            c->sample_rate, c->sample_rate, AV_ROUND_UP);
            if (dst_nb_samples > max_dst_nb_samples) {
                av_free(dst_samples_data[0]);
                ret = av_samples_alloc(dst_samples_data, &dst_samples_linesize, c->channels,
                                       dst_nb_samples, c->sample_fmt, 0);
                if (ret < 0)
                    exit(1);
                max_dst_nb_samples = dst_nb_samples;
                dst_samples_size = av_samples_get_buffer_size(NULL, c->channels, dst_nb_samples,
                                                              c->sample_fmt, 0);
            }

            /* convert to destination format */
            ret = swr_convert(swr_ctx,
                              dst_samples_data, dst_nb_samples,
                              (const uint8_t **)src_samples_data, src_nb_samples);
            if (ret < 0) {
                fprintf(stderr, "Error while converting\n");
                exit(1);
            }
        } else {
            dst_nb_samples = src_nb_samples;
        }

        audio_frame->nb_samples = dst_nb_samples;
        audio_frame->pts = av_rescale_q(samples_count, (AVRational){1, c->sample_rate}, c->time_base);
        avcodec_fill_audio_frame(audio_frame, c->channels, c->sample_fmt,
                                 dst_samples_data[0], dst_samples_size, 0);
        samples_count += dst_nb_samples;
    }

    ret = avcodec_encode_audio2(c, &pkt, flush ? NULL : audio_frame, &got_packet);
    if (ret < 0) {
        fprintf(stderr, "Error encoding audio frame: %s\n", av_err2str(ret));
        exit(1);
    }

    if (!got_packet) {
        if (flush)
            audio_is_eof = 1;
        return;
    }

    ret = write_frame(oc, &c->time_base, st, &pkt);
    if (ret < 0) {
        fprintf(stderr, "Error while writing audio frame: %s\n",
                av_err2str(ret));
        exit(1);
    }
}

static void close_audio(AVFormatContext *oc, AVStream *st)
{
    avcodec_close(st->codec);
    if (dst_samples_data != src_samples_data) {
        av_free(dst_samples_data[0]);
        av_free(dst_samples_data);
    }
    av_free(src_samples_data[0]);
    av_free(src_samples_data);
    av_frame_free(&audio_frame);
}

/**************************************************************/
/* video output */

static AVFrame *frame;
static AVPicture src_picture, dst_picture;
static int frame_count;

static void open_video(AVFormatContext *oc, AVCodec *codec, AVStream *st)
{
    int ret;
    AVCodecContext *c = st->codec;

    /* open the codec */
    ret = avcodec_open2(c, codec, NULL);
    if (ret < 0) {
        fprintf(stderr, "Could not open video codec: %s\n", av_err2str(ret));
        exit(1);
    }

    /* allocate and init a re-usable frame */
    frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "Could not allocate video frame\n");
        exit(1);
    }
    frame->format = c->pix_fmt;
    frame->width = c->width;
    frame->height = c->height;

    /* Allocate the encoded raw picture. */
    ret = avpicture_alloc(&dst_picture, c->pix_fmt, c->width, c->height);
    if (ret < 0) {
        fprintf(stderr, "Could not allocate picture: %s\n", av_err2str(ret));
        exit(1);
    }

    /* If the output format is not YUV420P, then a temporary YUV420P
     * picture is needed too. It is then converted to the required
     * output format. */
    if (c->pix_fmt != AV_PIX_FMT_YUV420P) {
        ret = avpicture_alloc(&src_picture, AV_PIX_FMT_YUV420P, c->width, c->height);
        if (ret < 0) {
            fprintf(stderr, "Could not allocate temporary picture: %s\n",
                    av_err2str(ret));
            exit(1);
        }
    }

    /* copy data and linesize picture pointers to frame */
    *((AVPicture *)frame) = dst_picture;
}

/* Prepare a dummy image. */
static void fill_yuv_image(AVPicture *pict, int frame_index,
                           int width, int height)
{
    int x, y, i;

    i = frame_index;

    /* Y */
    for (y = 0; y < height; y++)
        for (x = 0; x < width; x++)
            pict->data[0][y * pict->linesize[0] + x] = x + y + i * 3;

    /* Cb and Cr */
    for (y = 0; y < height / 2; y++) {
        for (x = 0; x < width / 2; x++) {
            pict->data[1][y * pict->linesize[1] + x] = 128 + y + i * 2;
            pict->data[2][y * pict->linesize[2] + x] = 64 + x + i * 5;
        }
    }
}

static void write_video_frame(AVFormatContext *oc, AVStream *st, int flush)
{
    int ret;
    static struct SwsContext *sws_ctx;
    AVCodecContext *c = st->codec;

    if (!flush) {
        if (c->pix_fmt != AV_PIX_FMT_YUV420P) {
            /* as we only generate a YUV420P picture, we must convert it
             * to the codec pixel format if needed */
            if (!sws_ctx) {
                sws_ctx = sws_getContext(c->width, c->height, AV_PIX_FMT_YUV420P,
                                         c->width, c->height, c->pix_fmt,
                                         sws_flags, NULL, NULL, NULL);
                if (!sws_ctx) {
                    fprintf(stderr,
                            "Could not initialize the conversion context\n");
                    exit(1);
                }
            }
            fill_yuv_image(&src_picture, frame_count, c->width, c->height);
            sws_scale(sws_ctx,
                      (const uint8_t * const *)src_picture.data, src_picture.linesize,
                      0, c->height, dst_picture.data, dst_picture.linesize);
        } else {
            fill_yuv_image(&dst_picture, frame_count, c->width, c->height);
        }
    }

    if (oc->oformat->flags & AVFMT_RAWPICTURE && !flush) {
        /* Raw video case - directly store the picture in the packet */
        AVPacket pkt;
        av_init_packet(&pkt);

        pkt.flags        |= AV_PKT_FLAG_KEY;
        pkt.stream_index  = st->index;
        pkt.data          = dst_picture.data[0];
        pkt.size          = sizeof(AVPicture);

        ret = av_interleaved_write_frame(oc, &pkt);
    } else {
        AVPacket pkt = { 0 };
        int got_packet;
        av_init_packet(&pkt);

        /* encode the image */
        frame->pts = frame_count;
        ret = avcodec_encode_video2(c, &pkt, flush ? NULL : frame, &got_packet);
        if (ret < 0) {
            fprintf(stderr, "Error encoding video frame: %s\n", av_err2str(ret));
            exit(1);
        }
        /* If size is zero, it means the image was buffered. */

        if (got_packet) {
            ret = write_frame(oc, &c->time_base, st, &pkt);
        } else {
            if (flush)
                video_is_eof = 1;
            ret = 0;
        }
    }

    if (ret < 0) {
        fprintf(stderr, "Error while writing video frame: %s\n", av_err2str(ret));
        exit(1);
    }
    frame_count++;
}

static void close_video(AVFormatContext *oc, AVStream *st)
{
    avcodec_close(st->codec);
    av_free(src_picture.data[0]);
    av_free(dst_picture.data[0]);
    av_frame_free(&frame);
}

/**************************************************************/
/* media file output */

int main(int argc, char **argv)
{
    const char *filename;
    AVOutputFormat *fmt;
    AVFormatContext *oc;
    AVStream *audio_st, *video_st;
    AVCodec *audio_codec, *video_codec;
    double audio_time, video_time;
    int flush, ret;

    /* Initialize libavcodec, and register all codecs and formats. */
    av_register_all();

    if (argc != 2) {
        printf("usage: %s output_file\n"
               "API example program to output a media file with libavformat.\n"
               "This program generates a synthetic audio and video stream, encodes and\n"
               "muxes them into a file named output_file.\n"
               "The output format is automatically guessed according to the file extension.\n"
               "Raw images can also be output by using '%%d' in the filename.\n"
               "\n", argv[0]);
        return 1;
    }

    filename = argv[1];

    /* allocate the output media context */
    avformat_alloc_output_context2(&oc, NULL, NULL, filename);
    if (!oc) {
        printf("Could not deduce output format from file extension: using MPEG.\n");
        avformat_alloc_output_context2(&oc, NULL, "mpeg", filename);
    }
    if (!oc)
        return 1;

    fmt = oc->oformat;

    /* Add the audio and video streams using the default format codecs
     * and initialize the codecs. */
    video_st = NULL;
    audio_st = NULL;

    if (fmt->video_codec != AV_CODEC_ID_NONE)
        video_st = add_stream(oc, &video_codec, fmt->video_codec);
    if (fmt->audio_codec != AV_CODEC_ID_NONE)
        audio_st = add_stream(oc, &audio_codec, fmt->audio_codec);

    /* Now that all the parameters are set, we can open the audio and
     * video codecs and allocate the necessary encode buffers. */
    if (video_st)
        open_video(oc, video_codec, video_st);
    if (audio_st)
        open_audio(oc, audio_codec, audio_st);

    av_dump_format(oc, 0, filename, 1);

    /* open the output file, if needed */
    if (!(fmt->flags & AVFMT_NOFILE)) {
        ret = avio_open(&oc->pb, filename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            fprintf(stderr, "Could not open '%s': %s\n", filename,
                    av_err2str(ret));
            return 1;
        }
    }

    /* Write the stream header, if any. */
    ret = avformat_write_header(oc, NULL);
    if (ret < 0) {
        fprintf(stderr, "Error occurred when opening output file: %s\n",
                av_err2str(ret));
        return 1;
    }

    flush = 0;
    while ((video_st && !video_is_eof) || (audio_st && !audio_is_eof)) {
        /* Compute current audio and video time. */
        audio_time = (audio_st && !audio_is_eof) ? audio_st->pts.val * av_q2d(audio_st->time_base) : INFINITY;
        video_time = (video_st && !video_is_eof) ? video_st->pts.val * av_q2d(video_st->time_base) : INFINITY;

        if (!flush &&
            (!audio_st || audio_time >= STREAM_DURATION) &&
            (!video_st || video_time >= STREAM_DURATION)) {
            flush = 1;
        }

        /* write interleaved audio and video frames */
        if (audio_st && !audio_is_eof && audio_time <= video_time) {
            write_audio_frame(oc, audio_st, flush);
        } else if (video_st && !video_is_eof && video_time < audio_time) {
            write_video_frame(oc, video_st, flush);
        }
    }

    /* Write the trailer, if any. The trailer must be written before you
     * close the CodecContexts open when you wrote the header; otherwise
     * av_write_trailer() may try to use memory that was freed on
     * av_codec_close(). */
    av_write_trailer(oc);

    /* Close each codec. */
    if (video_st)
        close_video(oc, video_st);
    if (audio_st)
        close_audio(oc, audio_st);

    if (!(fmt->flags & AVFMT_NOFILE))
        /* Close the output file. */
        avio_close(oc->pb);

    /* free the stream */
    avformat_free_context(oc);

    return 0;
}
