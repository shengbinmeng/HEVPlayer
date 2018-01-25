#ifndef __VIDEO_DECODER_H__
#define __VIDEO_DECODER_H__

#include "Decoder.h"

typedef void (*VideoDecodingHandler)(AVFrame*, double);

class VideoDecoder: public Decoder {
public:
	VideoDecoder(AVStream* stream);
	~VideoDecoder();

	VideoDecodingHandler onDecoded;
	double mVideoClock;
private:
	int decode(AVPacket *packet);
};

#endif
