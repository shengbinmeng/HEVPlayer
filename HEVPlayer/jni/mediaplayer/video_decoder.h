#ifndef __VIDEO_DECODER_H__
#define __VIDEO_DECODER_H__

#include "decoder.h"

typedef void (*VideoDecodingHandler)(AVFrame*, double);

class VideoDecoder: public Decoder {
public:
	VideoDecoder(AVStream* stream);
	~VideoDecoder();

	VideoDecodingHandler onDecoded;
	double mVideoClock;

private:
	AVFrame* mFrame;

	int prepare();
	double synchronize(AVFrame *src_frame, double pts);
	int decode(void* ptr);
	int process(AVPacket *packet);
	static int getBuffer(struct AVCodecContext *c, AVFrame *pic);
	static void releaseBuffer(struct AVCodecContext *c, AVFrame *pic);
};

#endif
