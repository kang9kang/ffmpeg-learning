#include "video_process.h"

int fill_stream_info(AVStream *avs, AVCodec **avc, AVCodecContext **avcc)
{
    *avc = const_cast<AVCodec *>(avcodec_find_decoder(avs->codecpar->codec_id));
    if (!*avc)
    {
        logging("failed to find the codec");
        return -1;
    }

    *avcc = avcodec_alloc_context3(*avc);
    if (!*avcc)
    {
        logging("failed to alloc memory for codec context");
        return -1;
    }

    if (avcodec_parameters_to_context(*avcc, avs->codecpar) < 0)
    {
        logging("failed to fill codec context");
        return -1;
    }

    if (avcodec_open2(*avcc, *avc, NULL) < 0)
    {
        logging("failed to open codec");
        return -1;
    }
    return 0;
}

int open_media(const char *in_filename, AVFormatContext **avfc)
{
    *avfc = avformat_alloc_context();
    if (!*avfc)
    {
        logging("failed to alloc memory for format");
        return -1;
    }

    if (avformat_open_input(avfc, in_filename, NULL, NULL) != 0)
    {
        logging("failed to open input file %s", in_filename);
        return -1;
    }

    if (avformat_find_stream_info(*avfc, NULL) < 0)
    {
        logging("failed to get stream info");
        return -1;
    }
    return 0;
}

int prepare_decoder(StreamingContext *sc)
{
    for (int i = 0; i < sc->avfc->nb_streams; i++)
    {
        if (sc->avfc->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            sc->video_avs = sc->avfc->streams[i];
            sc->video_index = i;

            if (fill_stream_info(sc->video_avs, &sc->video_avc, &sc->video_avcc))
            {
                return -1;
            }
        }
        else if (sc->avfc->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            sc->audio_avs = sc->avfc->streams[i];
            sc->audio_index = i;

            if (fill_stream_info(sc->audio_avs, &sc->audio_avc, &sc->audio_avcc))
            {
                return -1;
            }
        }
        else
        {
            logging("skipping streams other than audio and video");
        }
    }

    return 0;
}

int prepare_video_encoder(StreamingContext *sc, AVCodecContext *decoder_ctx, AVRational input_framerate,
                          StreamingParams sp)
{
    sc->video_avs = avformat_new_stream(sc->avfc, NULL);

    sc->video_avc = const_cast<AVCodec *>(avcodec_find_encoder_by_name(sp.video_codec));
    if (!sc->video_avc)
    {
        logging("could not find the proper codec");
        return -1;
    }

    sc->video_avcc = avcodec_alloc_context3(sc->video_avc);
    if (!sc->video_avcc)
    {
        logging("could not allocated memory for codec context");
        return -1;
    }

    av_opt_set(sc->video_avcc->priv_data, "preset", "fast", 0);
    if (sp.codec_priv_key && sp.codec_priv_value)
        av_opt_set(sc->video_avcc->priv_data, sp.codec_priv_key, sp.codec_priv_value, 0);

    sc->video_avcc->height = decoder_ctx->height;
    sc->video_avcc->width = decoder_ctx->width;
    sc->video_avcc->sample_aspect_ratio = decoder_ctx->sample_aspect_ratio;
    if (sc->video_avc->pix_fmts)
        sc->video_avcc->pix_fmt = sc->video_avc->pix_fmts[0];
    else
        sc->video_avcc->pix_fmt = decoder_ctx->pix_fmt;

    sc->video_avcc->bit_rate = 2 * 1000 * 1000;
    sc->video_avcc->rc_buffer_size = 4 * 1000 * 1000;
    sc->video_avcc->rc_max_rate = 2 * 1000 * 1000;
    sc->video_avcc->rc_min_rate = 2.5 * 1000 * 1000;

    sc->video_avcc->time_base = av_inv_q(input_framerate);
    sc->video_avs->time_base = sc->video_avcc->time_base;

    if (avcodec_open2(sc->video_avcc, sc->video_avc, NULL) < 0)
    {
        logging("could not open the codec");
        return -1;
    }
    avcodec_parameters_from_context(sc->video_avs->codecpar, sc->video_avcc);
    return 0;
}

int prepare_audio_encoder(StreamingContext *sc, int sample_rate, StreamingParams sp)
{
    sc->audio_avs = avformat_new_stream(sc->avfc, NULL);

    sc->audio_avc = const_cast<AVCodec *>(avcodec_find_encoder_by_name(sp.audio_codec));
    if (!sc->audio_avc)
    {
        logging("could not find the proper codec");
        return -1;
    }

    sc->audio_avcc = avcodec_alloc_context3(sc->audio_avc);
    if (!sc->audio_avcc)
    {
        logging("could not allocated memory for codec context");
        return -1;
    }

    int OUTPUT_CHANNELS = 2;
    int OUTPUT_BIT_RATE = 196000;
    sc->audio_avcc->channels = OUTPUT_CHANNELS;
    sc->audio_avcc->channel_layout = av_get_default_channel_layout(OUTPUT_CHANNELS);
    sc->audio_avcc->sample_rate = sample_rate;
    sc->audio_avcc->sample_fmt = sc->audio_avc->sample_fmts[0];
    sc->audio_avcc->bit_rate = OUTPUT_BIT_RATE;
    sc->audio_avcc->time_base = (AVRational){1, sample_rate};

    sc->audio_avcc->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;

    sc->audio_avs->time_base = sc->audio_avcc->time_base;

    if (avcodec_open2(sc->audio_avcc, sc->audio_avc, NULL) < 0)
    {
        logging("could not open the codec");
        return -1;
    }
    avcodec_parameters_from_context(sc->audio_avs->codecpar, sc->audio_avcc);
    return 0;
}

int prepare_copy(AVFormatContext *avfc, AVStream **avs, AVCodecParameters *decoder_par)
{
    *avs = avformat_new_stream(avfc, NULL);
    avcodec_parameters_copy((*avs)->codecpar, decoder_par);
    return 0;
}

int remux(AVPacket **pkt, AVFormatContext **avfc, AVRational decoder_tb, AVRational encoder_tb)
{
    av_packet_rescale_ts(*pkt, decoder_tb, encoder_tb);
    if (av_interleaved_write_frame(*avfc, *pkt) < 0)
    {
        logging("error while copying stream packet");
        return -1;
    }
    return 0;
}

int encode_video(StreamingContext *decoder, StreamingContext *encoder, AVFrame *input_frame)
{
    if (input_frame)
        input_frame->pict_type = AV_PICTURE_TYPE_NONE;

    AVPacket *output_packet = av_packet_alloc();
    if (!output_packet)
    {
        logging("could not allocate memory for output packet");
        return -1;
    }

    int response = avcodec_send_frame(encoder->video_avcc, input_frame);

    while (response >= 0)
    {
        response = avcodec_receive_packet(encoder->video_avcc, output_packet);
        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF)
        {
            break;
        }
        else if (response < 0)
        {
            logging("Error while receiving packet from encoder");
            return -1;
        }

        output_packet->stream_index = decoder->video_index;
        output_packet->duration = encoder->video_avs->time_base.den / encoder->video_avs->time_base.num /
                                  decoder->video_avs->avg_frame_rate.num * decoder->video_avs->avg_frame_rate.den;

        av_packet_rescale_ts(output_packet, decoder->video_avs->time_base, encoder->video_avs->time_base);
        response = av_interleaved_write_frame(encoder->avfc, output_packet);
        if (response != 0)
        {
            logging("Error %d while receiving packet from decoder", response);
            return -1;
        }
    }
    av_packet_unref(output_packet);
    av_packet_free(&output_packet);
    return 0;
}

int encode_audio(StreamingContext *decoder, StreamingContext *encoder, AVFrame *input_frame)
{
    AVPacket *output_packet = av_packet_alloc();
    if (!output_packet)
    {
        logging("could not allocate memory for output packet");
        return -1;
    }

    int response = avcodec_send_frame(encoder->audio_avcc, input_frame);

    while (response >= 0)
    {
        response = avcodec_receive_packet(encoder->audio_avcc, output_packet);
        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF)
        {
            break;
        }
        else if (response < 0)
        {
            logging("Error while receiving packet from encoder");
            return -1;
        }

        output_packet->stream_index = decoder->audio_index;

        av_packet_rescale_ts(output_packet, decoder->audio_avs->time_base, encoder->audio_avs->time_base);
        response = av_interleaved_write_frame(encoder->avfc, output_packet);
        if (response != 0)
        {
            logging("Error %d while receiving packet from decoder", response);
            return -1;
        }
    }
    av_packet_unref(output_packet);
    av_packet_free(&output_packet);
    return 0;
}

int transcode_audio(StreamingContext *decoder, StreamingContext *encoder, AVPacket *input_packet, AVFrame *input_frame)
{
    int response = avcodec_send_packet(decoder->audio_avcc, input_packet);
    if (response < 0)
    {
        logging("Error while sending packet to decoder");
        return response;
    }

    while (response >= 0)
    {
        response = avcodec_receive_frame(decoder->audio_avcc, input_frame);
        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF)
        {
            break;
        }
        else if (response < 0)
        {
            logging("Error while receiving frame from decoder");
            return response;
        }

        if (response >= 0)
        {
            if (encode_audio(decoder, encoder, input_frame))
                return -1;
        }
        av_frame_unref(input_frame);
    }
    return 0;
}

int transcode_video(StreamingContext *decoder, StreamingContext *encoder, AVPacket *input_packet, AVFrame *input_frame)
{
    int response = avcodec_send_packet(decoder->video_avcc, input_packet);
    if (response < 0)
    {
        logging("Error while sending packet to decoder");
        return response;
    }

    while (response >= 0)
    {
        response = avcodec_receive_frame(decoder->video_avcc, input_frame);
        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF)
        {
            break;
        }
        else if (response < 0)
        {
            logging("Error while receiving frame from decoder");
            return response;
        }

        if (response >= 0)
        {
            if (encode_video(decoder, encoder, input_frame))
                return -1;
        }
        av_frame_unref(input_frame);
    }
    return 0;
}