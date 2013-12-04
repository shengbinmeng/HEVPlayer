#ifndef __AUDIO_DECODER_H__
#define __AUDIO_DECODER_H__

#include "decoder.h"

typedef void (*AudioDecodingHandler)(int16_t*, int);

class AudioDecoder: public Decoder {
public:
	AudioDecoder(AVStream* stream);

	~AudioDecoder();

	AudioDecodingHandler onDecoded;
	double mAudioClock;
	double mAudioPts;

private:
	int16_t* mSamples;
	int mSamplesSize;

	int prepare();
	int decode(void* ptr);
	int process(AVPacket *packet);
};

#endif
