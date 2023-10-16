#ifndef VIDEO_PROCESS_H
#define VIDEO_PROCESS_H

extern "C"
{
#include <libavutil/opt.h>
}

#include "video_debug.h"

typedef struct StreamingParams
{
    char copy_video;
    char copy_audio;
    char *output_extension;
    char *muxer_opt_key;
    char *muxer_opt_value;
    char *video_codec;
    char *audio_codec;
    char *codec_priv_key;
    char *codec_priv_value;
} StreamingParams;

typedef struct StreamingContext
{
    AVFormatContext *avfc;
    AVCodec *video_avc;
    AVCodec *audio_avc;
    AVStream *video_avs;
    AVStream *audio_avs;
    AVCodecContext *video_avcc;
    AVCodecContext *audio_avcc;
    int video_index;
    int audio_index;
    char *filename;
} StreamingContext;

int open_media(const char *in_filename, AVFormatContext **avfc);

int prepare_decoder(StreamingContext *sc);

int fill_stream_info(AVStream *avs, AVCodec **avc, AVCodecContext **avcc);

int prepare_video_encoder(StreamingContext *sc, AVCodecContext *decoder_ctx, AVRational input_framerate,
                          StreamingParams sp);

int prepare_audio_encoder(StreamingContext *sc, int sample_rate, StreamingParams sp);

int prepare_copy(AVFormatContext *avfc, AVStream **avs, AVCodecParameters *decoder_par);

int remux(AVPacket **pkt, AVFormatContext **avfc, AVRational decoder_tb, AVRational encoder_tb);

int encode_video(StreamingContext *decoder, StreamingContext *encoder, AVFrame *input_frame);

int encode_audio(StreamingContext *decoder, StreamingContext *encoder, AVFrame *input_frame);

int transcode_audio(StreamingContext *decoder, StreamingContext *encoder, AVPacket *input_packet, AVFrame *input_frame);

int transcode_video(StreamingContext *decoder, StreamingContext *encoder, AVPacket *input_packet, AVFrame *input_frame);

#endif // VIDEO_PROCESS_H