/*
 * Copyright (c) 2012 Stefano Sabatini
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
 * Demux example.
 *
 * Show how to use the libavformat and libavcodec API to demux and
 * decode audio and video data.
 * @example demuxing_decoding.c
 */

#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>

static AVFormatContext *fmt_ctx = NULL;
static AVCodecContext *video_dec_ctx = NULL, *audio_dec_ctx;
static AVStream *video_in_stream = NULL, *audio_in_stream = NULL;
static const char *src_filename = NULL;

static int video_in_stream_idx = -1, audio_in_stream_idx = -1;
static AVPacket pkt;



static const char *video_enc_filename = NULL;
static FILE       *video_enc_file = NULL;
AVOutputFormat    *video_output_fmt = NULL;
AVFormatContext   *video_output_fmt_ctx = NULL;

static const char *audio_enc_filename = NULL;
static FILE       *audio_enc_file = NULL;
AVOutputFormat    *audio_output_fmt = NULL;
AVFormatContext   *audio_output_fmt_ctx = NULL;


static int open_codec_context(int *stream_idx,
                              AVCodecContext **dec_ctx, AVFormatContext *fmt_ctx, enum AVMediaType type)
{
    int ret, stream_index;
    AVStream *st;
    AVCodec *dec = NULL;
    AVDictionary *opts = NULL;

    ret = av_find_best_stream(fmt_ctx, type, -1, -1, NULL, 0);
    if (ret < 0) {
        fprintf(stderr, "Could not find %s stream in input file '%s'\n",
                av_get_media_type_string(type), src_filename);
        return ret;
    } else {
        stream_index = ret;
        st = fmt_ctx->streams[stream_index];

        /* find decoder for the stream */
        dec = avcodec_find_decoder(st->codecpar->codec_id);
        if (!dec) {
            fprintf(stderr, "Failed to find %s codec\n",
                    av_get_media_type_string(type));
            return AVERROR(EINVAL);
        }

        /* Allocate a codec context for the decoder */
        *dec_ctx = avcodec_alloc_context3(dec);
        if (!*dec_ctx) {
            fprintf(stderr, "Failed to allocate the %s codec context\n",
                    av_get_media_type_string(type));
            return AVERROR(ENOMEM);
        }

        /* Copy codec parameters from input stream to output codec context */
        if ((ret = avcodec_parameters_to_context(*dec_ctx, st->codecpar)) < 0) {
            fprintf(stderr, "Failed to copy %s codec parameters to decoder context\n",
                    av_get_media_type_string(type));
            return ret;
        }

        *stream_idx = stream_index;
    }

    return 0;
}


static void log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt, const char *tag)
{
    AVRational *time_base = &fmt_ctx->streams[pkt->stream_index]->time_base;

    printf("%s: pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d\n",
           tag,
           av_ts2str(pkt->pts), av_ts2timestr(pkt->pts, time_base),
           av_ts2str(pkt->dts), av_ts2timestr(pkt->dts, time_base),
           av_ts2str(pkt->duration), av_ts2timestr(pkt->duration, time_base),
           pkt->stream_index);
}




int main (int argc, char **argv)
{
    int ret = 0, got_frame;

    if (argc != 4 && argc != 5 ) {
        fprintf(stderr, "usage: %s input_file video_dec_file audio_dec_file video_enc_file \n"
                "API example program to show how to read frames from an input file.\n"
                "This program reads frames from a file, and writes decoded\n"
                "video frames to a file named video_output_file, \n"
                "audio frames to a file named audio_output_file.\n\n"
                "\n", argv[0]);
        exit(1);
    }

    src_filename = argv[1];

	video_enc_filename = argv[2];
	audio_enc_filename = argv[3];
	





    /* register all formats and codecs */
    av_register_all();

    /* open input file, and allocate format context */
    if (avformat_open_input(&fmt_ctx, src_filename, NULL, NULL) < 0) {
        fprintf(stderr, "Could not open source file %s\n", src_filename);
        exit(1);
    }

    /* retrieve stream information */
    if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
        fprintf(stderr, "Could not find stream information\n");
        exit(1);
    }

    if (open_codec_context(&video_in_stream_idx, &video_dec_ctx, fmt_ctx, AVMEDIA_TYPE_VIDEO) >= 0) {
        video_in_stream = fmt_ctx->streams[video_in_stream_idx];

    }

    if (open_codec_context(&audio_in_stream_idx, &audio_dec_ctx, fmt_ctx, AVMEDIA_TYPE_AUDIO) >= 0) {
        audio_in_stream = fmt_ctx->streams[audio_in_stream_idx];
    }

    // dump input information to stderr 
    av_dump_format(fmt_ctx, 0, src_filename, 0);

    if (!audio_in_stream && !video_in_stream) {
        fprintf(stderr, "Could not find audio or video stream in the input, aborting\n");
        ret = 1;
        goto end;
    }


	/* Video */
	avformat_alloc_output_context2(&video_output_fmt_ctx, NULL, NULL, video_enc_filename);
	if (!video_output_fmt_ctx) {
        fprintf(stderr, "Could not create video output context\n");
        ret = AVERROR_UNKNOWN;
        goto end;
	}
	video_output_fmt = video_output_fmt_ctx->oformat;

	AVStream *video_out_stream = avformat_new_stream(video_output_fmt_ctx, video_in_stream->codec->codec);
	if (!video_out_stream) {
        fprintf(stderr, "Failed allocating video output stream\n");
        ret = AVERROR_UNKNOWN;
        goto end;	
	}
	ret  = avcodec_copy_context(video_out_stream->codec, video_in_stream->codec);
    if (ret < 0) {
		fprintf(stderr, "Failed to copy context from input to output stream codec context\n");
        goto end;
    }
    video_out_stream->codec->codec_tag = 0;
    if (video_output_fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
        video_out_stream->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
 
    av_dump_format(video_output_fmt_ctx, 0, video_enc_filename, 1);

    if (!(video_output_fmt->flags & AVFMT_NOFILE)) {
        ret = avio_open(&video_output_fmt_ctx->pb, video_enc_filename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            fprintf(stderr, "Could not open video output file '%s'", video_enc_filename);
            goto end;
        }
    }

	/* Audio */
	avformat_alloc_output_context2(&audio_output_fmt_ctx, NULL, NULL, audio_enc_filename);
	if (!audio_output_fmt_ctx) {
        fprintf(stderr, "Could not create audio output context\n");
        ret = AVERROR_UNKNOWN;
        goto end;
	}
	audio_output_fmt = audio_output_fmt_ctx->oformat;

	AVStream *audio_out_stream = avformat_new_stream(audio_output_fmt_ctx, audio_in_stream->codec->codec);
	if (!audio_out_stream) {
        fprintf(stderr, "Failed allocating audio output stream\n");
        ret = AVERROR_UNKNOWN;
        goto end;	
	}
	ret  = avcodec_copy_context(audio_out_stream->codec, audio_in_stream->codec);
    if (ret < 0) {
		fprintf(stderr, "Failed to copy context from input to output stream codec context\n");
        goto end;
    }
    audio_out_stream->codec->codec_tag = 0;
    if (audio_output_fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
        audio_out_stream->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
 
    av_dump_format(audio_output_fmt_ctx, 0, audio_enc_filename, 1);

    if (!(audio_output_fmt->flags & AVFMT_NOFILE)) {
        ret = avio_open(&audio_output_fmt_ctx->pb, audio_enc_filename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            fprintf(stderr, "Could not open audio output file '%s'", audio_enc_filename);
            goto end;
        }
    }

    ret = avformat_write_header(video_output_fmt_ctx, NULL);
    if (ret < 0) {
        fprintf(stderr, "Error occurred when opening video output file\n");
        goto end;
    }

    ret = avformat_write_header(audio_output_fmt_ctx, NULL);
    if (ret < 0) {
        fprintf(stderr, "Error occurred when opening audio output file\n");
        goto end;
    }



    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;

    if (video_in_stream)
        printf("Demuxing video from file '%s' into '%s'\n", src_filename, video_enc_filename);
    if (audio_in_stream)
        printf("Demuxing audio from file '%s' into '%s'\n", src_filename, audio_enc_filename);

    /* read frames from the file */ 
	while(1) {
		ret = av_read_frame(fmt_ctx, &pkt);
		if (ret < 0)
			break; 

		if (pkt.stream_index == video_in_stream_idx) {
			log_packet(fmt_ctx, &pkt, "video_in");
	
			pkt.pts = av_rescale_q_rnd(pkt.pts, video_in_stream->time_base, 
					video_out_stream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);

			pkt.dts = av_rescale_q_rnd(pkt.dts, video_in_stream->time_base, video_out_stream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);

			pkt.duration = av_rescale_q(pkt.duration, video_in_stream->time_base, video_out_stream->time_base);
			pkt.pos = -1;
			pkt.stream_index = 0; // 只有一个流

			log_packet(video_output_fmt_ctx, &pkt, "video_out");
			ret = av_interleaved_write_frame(video_output_fmt_ctx, &pkt);
			if (ret < 0) {
				fprintf(stderr, "Error muxing packet\n");
				break;
			}
			av_packet_unref(&pkt);

		}

		if (pkt.stream_index == audio_in_stream_idx) {
			log_packet(fmt_ctx, &pkt, "audio_in");
			pkt.pts = av_rescale_q_rnd(pkt.pts, audio_in_stream->time_base, 
					audio_out_stream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);

			pkt.dts = av_rescale_q_rnd(pkt.dts, audio_in_stream->time_base, audio_out_stream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);

			pkt.duration = av_rescale_q(pkt.duration, audio_in_stream->time_base, audio_out_stream->time_base);
			pkt.pos = -1;
			pkt.stream_index = 0; // 只有一个流，则其序号必须为0

			log_packet(audio_output_fmt_ctx, &pkt, "audio_out");
			ret = av_interleaved_write_frame(audio_output_fmt_ctx, &pkt);
			if (ret < 0) {
				fprintf(stderr, "Error muxing packet\n");
				break;
			}
			av_packet_unref(&pkt);
		}
	}

	
    av_write_trailer(video_output_fmt_ctx);
    av_write_trailer(audio_output_fmt_ctx);

    printf("Demuxing succeeded.\n");


end:
    avcodec_free_context(&video_dec_ctx);
    avcodec_free_context(&audio_dec_ctx);
    avformat_close_input(&fmt_ctx);

	if (video_enc_file)
		fclose(video_enc_file);
	if (audio_enc_file)
		fclose(audio_enc_file);

    if (video_output_fmt_ctx && !(video_output_fmt->flags & AVFMT_NOFILE))
        avio_closep(&video_output_fmt_ctx->pb);
    if (audio_output_fmt_ctx && !(audio_output_fmt->flags & AVFMT_NOFILE))
        avio_closep(&audio_output_fmt_ctx->pb);

    avformat_free_context(video_output_fmt_ctx);
    avformat_free_context(audio_output_fmt_ctx);


    return ret < 0;
}
