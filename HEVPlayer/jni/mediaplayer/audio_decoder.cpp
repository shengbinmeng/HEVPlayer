#include "audio_decoder.h"
#include "player_utils.h"

#define LOG_TAG "AudioDecoder"
#define MAX_AUDIO_FRAME_SIZE 192000

AudioDecoder::AudioDecoder(AVStream* stream) : Decoder(stream)
{
}

int AudioDecoder::prepare()
{
    mAudioClock = 0;
    mAudioPts = 0;
    mSamplesSize = MAX_AUDIO_FRAME_SIZE;
    mSamples = (int16_t *) av_malloc(mSamplesSize);
    if (mSamples == NULL) {
    	return -1;
    }
    return 0;
}

int AudioDecoder::process(AVPacket *packet)
{

    if (packet->pts != AV_NOPTS_VALUE) {
    	mAudioPts = av_q2d(mStream->time_base) * packet->pts;
    	LOGD("pts: %lld; av_q2d: %f; mAudioPts: %f \n", packet->pts, av_q2d(mStream->time_base), mAudioPts);
    } else {
    	mAudioPts = -99.99;
    }

    int size = mSamplesSize;
    int len = avcodec_decode_audio3(mStream->codec, mSamples, &size, packet);
    if (len < 0) {
    	LOGE("decode audio packet failed \n");
    	//return false;
    }

	// save the audio clock for sync
	int n = 2 * mStream->codec->channels;
	double inc = (double)size / (double)(n * mStream->codec->sample_rate);
	mAudioClock += inc;


    // call handler for posting buffer to os audio driver
    onDecoded(mSamples, size);
    return 0;
}

int AudioDecoder::decode(void* ptr)
{
    AVPacket        packet;

    while (mRunning) {
        if (mQueue->get(&packet, true) < 0) {
        	LOGE("get audio packet failed");
            mRunning = false;
            return -1;
        }
        if (memcmp(packet.data, "FLUSH", sizeof("FLUSH")) == 0) {
          avcodec_flush_buffers(mStream->codec);
          LOGD("FLUSH");
          continue;
        }
        //LOGI("out queue an audio packet. Number:%d", packets());
        if (!process(&packet)) {
            mRunning = false;
            return -1;
        }
        // free the packet that was allocated by av_read_frame
        av_free_packet(&packet);
    }

    // free audio samples buffer
    av_free(mSamples);
    return 0;
}
