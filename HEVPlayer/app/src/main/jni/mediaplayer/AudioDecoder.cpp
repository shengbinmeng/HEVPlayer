#include "AudioDecoder.h"
#include "player_utils.h"
extern "C" {
#include "libswresample/swresample.h"
#include "libavutil/opt.h"
}

#define LOG_TAG "AudioDecoder"
#define MAX_AUDIO_FRAME_SIZE 192000

AudioDecoder::AudioDecoder(AVStream* stream) : Decoder(stream) {
	mAudioClock = 0;
	mAudioPts = 0;
	mSamples = NULL;
	mSwrContext = NULL;
}

AudioDecoder::~AudioDecoder() {
	if (mSamples != NULL) {
		free(mSamples);
	}
	if (mSwrContext != NULL) {
		swr_free(&mSwrContext);
	}
}

int AudioDecoder::decode(AVPacket *packet) {
	// Handle customized special packet.
	if (memcmp(packet->data, "END", sizeof("END")) == 0) {
		// send NULL to indicate ending.
		onDecoded(NULL, -1);
		LOGI("end of audio decoding\n");
		return 0;
	}
	
	int gotFrame = 0;
	int len = avcodec_decode_audio4(mStream->codec, mFrame, &gotFrame, packet);
	if (len < 0) {
		LOGE("decode audio packet failed\n");
	}

	LOGD("audio packet dts: %lld = %lld; packet pts: %lld = %lld; frame pts: %lld\n", packet->dts, mFrame->pkt_dts, packet->pts, mFrame->pkt_pts, mFrame->pts);

	if (gotFrame) {
		int size = av_samples_get_buffer_size(NULL, mFrame->channels,
				mFrame->nb_samples, AV_SAMPLE_FMT_S16, 1);
		int bps = av_get_bytes_per_sample((AVSampleFormat)mFrame->format);

		if (packet->pts != AV_NOPTS_VALUE) {
			mAudioClock = av_q2d(mStream->time_base) * packet->pts;
		} else {
			double inc = (double)size / (double)(bps * mFrame->sample_rate * mFrame->channels);
			mAudioClock += inc;
		}

		LOGD("output an audio frame, mAudioClock: %lf\n", mAudioClock);

		// conversion using swresample
		if (mSwrContext == NULL) {
			// Set up SWR context once you've got codec information
			mSwrContext = swr_alloc();
			if (mSwrContext == NULL) {
				LOGE("allocate context failed\n");
			}
			av_opt_set_int(mSwrContext, "in_channel_layout", mFrame->channel_layout, 0);
			av_opt_set_int(mSwrContext, "out_channel_layout", mFrame->channel_layout, 0);
			av_opt_set_int(mSwrContext, "in_sample_rate", mFrame->sample_rate, 0);
			av_opt_set_int(mSwrContext, "out_sample_rate", mFrame->sample_rate, 0);
			av_opt_set_sample_fmt(mSwrContext, "in_sample_fmt", (AVSampleFormat)mFrame->format, 0);
			av_opt_set_sample_fmt(mSwrContext, "out_sample_fmt", AV_SAMPLE_FMT_S16,  0);
			int ret = swr_init(mSwrContext);
			if (ret < 0) {
				LOGE("init context failed\n");
			}
		}
		
		if (mSamples == NULL) {
			mSamples = malloc(MAX_AUDIO_FRAME_SIZE);
			if (mSamples == NULL) {
				LOGE("allocate samples failed\n");
				return -1;
			}
		}
		int ret = swr_convert(mSwrContext, (uint8_t**)&mSamples, mFrame->nb_samples,
		 (const uint8_t **)mFrame->extended_data, mFrame->nb_samples);
		if (ret != mFrame->nb_samples) {
			LOGE("conversion wrong\n");
		}
		
		// call handler for posting buffer to os audio driver
		onDecoded(mSamples, size);
	}

	return 0;
}
