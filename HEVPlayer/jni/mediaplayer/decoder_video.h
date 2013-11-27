#ifndef __DECODER_VIDEO_H__
#define __DECODER_VIDEO_H__

#include "decoder.h"

typedef void (*VideoDecodingHandler) (AVFrame*, double);

class DecoderVideo : public Decoder
{
public:
    DecoderVideo(AVStream* stream);
    ~DecoderVideo();

    VideoDecodingHandler		onDecoded;
	double						mVideoClock;

private:
	AVFrame*					mFrame;

    bool                        prepare();
    double 						synchronize(AVFrame *src_frame, double pts);
    bool                        decode(void* ptr);
    bool                        process(AVPacket *packet);
    static int					getBuffer(struct AVCodecContext *c, AVFrame *pic);
    static void					releaseBuffer(struct AVCodecContext *c, AVFrame *pic);
};

#endif //__DECODER_VIDEO_H__
