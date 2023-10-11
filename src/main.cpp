#include <stdarg.h>
#include <stdio.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

static void logging(const char *fmt, ...);

static int decode_packet(AVPacket *pPacket, AVCodecContext *pCodecContext,
                         AVFrame *pFrame);

static void save_gray_frame(unsigned char *buf, int wrap, int xsize, int ysize,
                            char *filename);

int main(int argc, char *argv[]) {
  if (argc < 2) {
    logging("Please provide a movie file");
    return -1;
  }

  char *filename = argv[1];
  logging("Decoding file %s", filename);

  AVFormatContext *pFormatContext = avformat_alloc_context();
  if (!pFormatContext) {
    logging("ERROR could not allocate memory for Format Context");
    return -1;
  }

  logging("Opening file %s", filename);

  if (avformat_open_input(&pFormatContext, filename, NULL, NULL) != 0) {
    logging("ERROR could not open the file");
    return -1;
  }

  logging("Format %s, duration %lld us, bit_rate %lld",
          pFormatContext->iformat->name, pFormatContext->duration,
          pFormatContext->bit_rate);

  logging("Finding stream info from format");

  if (avformat_find_stream_info(pFormatContext, NULL) < 0) {
    logging("ERROR could not get the stream info");
    return -1;
  }

  return 0;
}

static void logging(const char *fmt, ...) {
  va_list args;
  fprintf(stderr, "LOG: ");
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
  fprintf(stderr, "\n");
  return;
}

static int decode_packet(AVPacket *pPacket, AVCodecContext *pCodecContext,
                         AVFrame *pFrame) {
  return 0;
}

static void save_gray_frame(unsigned char *buf, int wrap, int xsize, int ysize,
                            char *filename) {
  return;
}