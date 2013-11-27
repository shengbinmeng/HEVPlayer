#include "decoder_audio.h"
#include "jni_utils.h"

#define LOG_TAG "AudioDecoder"
#define MAX_AUDIO_FRAME_SIZE 192000

bool DecoderAudio::prepare()
{
    mAudioClock = 0;
    mAudioClock_pts = 0;
    mSamplesSize = MAX_AUDIO_FRAME_SIZE;
    mSamples = (int16_t *) av_malloc(mSamplesSize);
    if (mSamples == NULL) {
    	return false;
    }
    return true;
}

bool DecoderAudio::process(AVPacket *packet)
{

    if (packet->pts != AV_NOPTS_VALUE) {
    	mAudioClock_pts = av_q2d(mStream->time_base) * packet->pts;
    	//LOGE("pts: %lld; av_q2d: %f; mAudioClock_pts: %f", packet->pts, av_q2d(mStream->time_base), mAudioClock_pts);
    } else {
    	mAudioClock_pts = -99.99;
    }

	//LOGI("process decoding an audio packet");
    int size = mSamplesSize;
    int len = avcodec_decode_audio3(mStream->codec, mSamples, &size, packet);
    if (len < 0) {
    	LOGE("decode audio packet failed");
    	//return false;
    }
    //LOGI("an audio packet decoded");

	// save the audio clock for sync
	int n = 2 * mStream->codec->channels;
	double inc = (double)size / (double)(n * mStream->codec->sample_rate);
	mAudioClock += inc;


    //call handler for posting buffer to os audio driver
    onDecode(mSamples, size);
    //LOGI("an audio frame output");
    return true;
}

bool DecoderAudio::decode(void* ptr)
{
    AVPacket        packet;

    LOGI("decoding audio");

    while (mRunning) {
        if (mQueue->get(&packet, true) < 0) {
        	LOGE("get audio packet failed");
            mRunning = false;
            return false;
        }
        if (memcmp(packet.data, "FLUSH", sizeof("FLUSH")) == 0) {
          avcodec_flush_buffers(mStream->codec);
          LOGD("FLUSH");
          continue;
        }
        //LOGI("out queue an audio packet. Number:%d", packets());
        if (!process(&packet)) {
            mRunning = false;
            return false;
        }
        // Free the packet that was allocated by av_read_frame
        av_free_packet(&packet);
    }

    LOGI("decoding audio ended");

    // Free audio samples buffer
    av_free(mSamples);
    return true;
}
