#include <iostream>

#include "config.h"
#include "video_debug.h"
#include "video_process.h"

int main(int argc, char *argv[])
{
    /*
     * H264 -> H265
     * Audio -> remuxed (untouched)
     * MP4 - MP4
     */
    StreamingParams sp = {0};
    sp.copy_audio = 1;
    sp.copy_video = 0;
    sp.video_codec = "libx265";
    sp.codec_priv_key = "x265-params";
    sp.codec_priv_value = "keyint=60:min-keyint=60:scenecut=0";

    /*
     * H264 -> H264 (fixed gop)
     * Audio -> remuxed (untouched)
     * MP4 - MP4
     */
    // StreamingParams sp = {0};
    // sp.copy_audio = 1;
    // sp.copy_video = 0;
    // sp.video_codec = "libx264";
    // sp.codec_priv_key = "x264-params";
    // sp.codec_priv_value = "keyint=60:min-keyint=60:scenecut=0:force-cfr=1";

    /*
     * H264 -> H264 (fixed gop)
     * Audio -> remuxed (untouched)
     * MP4 - fragmented MP4
     */
    // StreamingParams sp = {0};
    // sp.copy_audio = 1;
    // sp.copy_video = 0;
    // sp.video_codec = "libx264";
    // sp.codec_priv_key = "x264-params";
    // sp.codec_priv_value = "keyint=60:min-keyint=60:scenecut=0:force-cfr=1";
    // sp.muxer_opt_key = "movflags";
    // sp.muxer_opt_value = "frag_keyframe+empty_moov+delay_moov+default_base_moof";

    /*
     * H264 -> H264 (fixed gop)
     * Audio -> AAC
     * MP4 - MPEG-TS
     */
    // StreamingParams sp = {0};
    // sp.copy_audio = 0;
    // sp.copy_video = 0;
    // sp.video_codec = "libx264";
    // sp.codec_priv_key = "x264-params";
    // sp.codec_priv_value = "keyint=60:min-keyint=60:scenecut=0:force-cfr=1";
    // sp.audio_codec = "aac";
    // sp.output_extension = ".ts";

    /*
     * H264 -> VP9
     * Audio -> Vorbis
     * MP4 - WebM
     */
    // StreamingParams sp = {0};
    // sp.copy_audio = 0;
    // sp.copy_video = 0;
    // sp.video_codec = "libvpx-vp9";
    // sp.audio_codec = "libvorbis";
    // sp.output_extension = ".webm";

    StreamingContext *decoder = (StreamingContext *)calloc(1, sizeof(StreamingContext));
    decoder->filename = argv[1];

    StreamingContext *encoder = (StreamingContext *)calloc(1, sizeof(StreamingContext));
    encoder->filename = argv[2];

    if (sp.output_extension)
        strcat(encoder->filename, sp.output_extension);

    if (open_media(decoder->filename, &decoder->avfc))
        return -1;
    if (prepare_decoder(decoder))
        return -1;

    avformat_alloc_output_context2(&encoder->avfc, NULL, NULL, encoder->filename);
    if (!encoder->avfc)
    {
        logging("could not allocate memory for output format");
        return -1;
    }

    if (!sp.copy_video)
    {
        AVRational input_framerate = av_guess_frame_rate(decoder->avfc, decoder->video_avs, NULL);
        prepare_video_encoder(encoder, decoder->video_avcc, input_framerate, sp);
    }
    else
    {
        if (prepare_copy(encoder->avfc, &encoder->video_avs, decoder->video_avs->codecpar))
        {
            return -1;
        }
    }

    if (!sp.copy_audio)
    {
        if (prepare_audio_encoder(encoder, decoder->audio_avcc->sample_rate, sp))
        {
            return -1;
        }
    }
    else
    {
        if (prepare_copy(encoder->avfc, &encoder->audio_avs, decoder->audio_avs->codecpar))
        {
            return -1;
        }
    }

    if (encoder->avfc->oformat->flags & AVFMT_GLOBALHEADER)
        encoder->avfc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    if (!(encoder->avfc->oformat->flags & AVFMT_NOFILE))
    {
        if (avio_open(&encoder->avfc->pb, encoder->filename, AVIO_FLAG_WRITE) < 0)
        {
            logging("could not open the output file");
            return -1;
        }
    }

    AVDictionary *muxer_opts = NULL;

    if (sp.muxer_opt_key && sp.muxer_opt_value)
    {
        av_dict_set(&muxer_opts, sp.muxer_opt_key, sp.muxer_opt_value, 0);
    }

    if (avformat_write_header(encoder->avfc, &muxer_opts) < 0)
    {
        logging("an error occurred when opening output file");
        return -1;
    }

    AVFrame *input_frame = av_frame_alloc();
    if (!input_frame)
    {
        logging("failed to allocated memory for AVFrame");
        return -1;
    }

    AVPacket *input_packet = av_packet_alloc();
    if (!input_packet)
    {
        logging("failed to allocated memory for AVPacket");
        return -1;
    }

    while (av_read_frame(decoder->avfc, input_packet) >= 0)
    {
        if (decoder->avfc->streams[input_packet->stream_index]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            if (!sp.copy_video)
            {
                // TODO: refactor to be generic for audio and video (receiving a function pointer to the differences)
                if (transcode_video(decoder, encoder, input_packet, input_frame))
                    return -1;
                av_packet_unref(input_packet);
            }
            else
            {
                if (remux(&input_packet, &encoder->avfc, decoder->video_avs->time_base, encoder->video_avs->time_base))
                    return -1;
            }
        }
        else if (decoder->avfc->streams[input_packet->stream_index]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            if (!sp.copy_audio)
            {
                if (transcode_audio(decoder, encoder, input_packet, input_frame))
                    return -1;
                av_packet_unref(input_packet);
            }
            else
            {
                if (remux(&input_packet, &encoder->avfc, decoder->audio_avs->time_base, encoder->audio_avs->time_base))
                    return -1;
            }
        }
        else
        {
            logging("ignoring all non video or audio packets");
        }
    }
    // TODO: should I also flush the audio encoder?
    if (encode_video(decoder, encoder, NULL))
        return -1;

    av_write_trailer(encoder->avfc);

    if (muxer_opts != NULL)
    {
        av_dict_free(&muxer_opts);
        muxer_opts = NULL;
    }

    if (input_frame != NULL)
    {
        av_frame_free(&input_frame);
        input_frame = NULL;
    }

    if (input_packet != NULL)
    {
        av_packet_free(&input_packet);
        input_packet = NULL;
    }

    avformat_close_input(&decoder->avfc);

    avformat_free_context(decoder->avfc);
    decoder->avfc = NULL;
    avformat_free_context(encoder->avfc);
    encoder->avfc = NULL;

    avcodec_free_context(&decoder->video_avcc);
    decoder->video_avcc = NULL;
    avcodec_free_context(&decoder->audio_avcc);
    decoder->audio_avcc = NULL;

    free(decoder);
    decoder = NULL;
    free(encoder);
    encoder = NULL;
    return 0;
}