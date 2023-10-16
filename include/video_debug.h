#ifndef VIDEO_DEBUG_H
#define VIDEO_DEBUG_H

#include <iostream>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

void logging(const char *fmt, ...);
void log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt);
void print_timing(char *name, AVFormatContext *avf, AVCodecContext *avc, AVStream *avs);

#endif // VIDEO_DEBUG_H