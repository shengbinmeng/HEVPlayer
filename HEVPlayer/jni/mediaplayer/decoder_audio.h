#ifndef __DECODER_AUDIO_H__
#define __DECODER_AUDIO_H__

#include "decoder.h"

typedef void (*AudioDecodingHandler) (int16_t*,int);

class DecoderAudio : public Decoder
{
public:
    DecoderAudio(AVStream* stream);

    ~DecoderAudio();

    AudioDecodingHandler		onDecode;
	double						mAudioClock;
	double						mAudioClock_pts;

private:
    int16_t*                    mSamples;
    int                         mSamplesSize;

    bool                        prepare();
    bool                        decode(void* ptr);
    bool                        process(AVPacket *packet);
};

#endif //__DECODER_AUDIO_H__
