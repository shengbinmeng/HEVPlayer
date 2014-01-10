#include "audio_decoder.h"
#include "player_utils.h"
extern "C" {
#include "libswresample/swresample.h"
#include "libavutil/opt.h"
}

#define LOG_TAG "AudioDecoder"
#define MAX_AUDIO_FRAME_SIZE 192000
FILE *file_out, *file_out_f;
AudioDecoder::AudioDecoder(AVStream* stream) : Decoder(stream) {

}

AudioDecoder::~AudioDecoder() {

}


int AudioDecoder::prepare() {
    mAudioClock = 0;
    mAudioPts = 0;
    mFrame = avcodec_alloc_frame();
	if (mFrame == NULL) {
		LOGE("avcodec_alloc_frame failed \n");
		return -1;
	}
    mSamples = malloc(MAX_AUDIO_FRAME_SIZE);
    if (mSamples == NULL) {
		LOGE("allocate samples failed \n");
		return -1;
    }
    mSwrContext = NULL;
    return 0;
}

int AudioDecoder::process(AVPacket *packet) {
	int gotFrame = 0;
	int len = avcodec_decode_audio4(mStream->codec, mFrame, &gotFrame, packet);
	if (len < 0) {
		LOGE("decode audio packet failed \n");
	}

	LOGD("packet dts: %lld = %lld; packet pts: %lld = %lld; frame pts: %lld", packet->dts, mFrame->pkt_dts, packet->pts, mFrame->pkt_pts, mFrame->pts);

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

		LOGD("output an audio frame: %d * %d * (%d->2) = %d, mAudioClock: %lf", mFrame->channels, mFrame->nb_samples, bps, size, mAudioClock);

		LOGD("before conversion \n");
#if 1
		// conversion using swresample
		if (mSwrContext == NULL) {
			// Set up SWR context once you've got codec information
			mSwrContext = swr_alloc();
			if (mSwrContext == NULL) {
				LOGE("allocate context failed \n");
			}
			av_opt_set_int(mSwrContext, "in_channel_layout",  mFrame->channel_layout, 0);
			av_opt_set_int(mSwrContext, "out_channel_layout", mFrame->channel_layout, 0);
			av_opt_set_int(mSwrContext, "in_sample_rate",     mFrame->sample_rate, 0);
			av_opt_set_int(mSwrContext, "out_sample_rate",    mFrame->sample_rate, 0);
			av_opt_set_sample_fmt(mSwrContext, "in_sample_fmt", (AVSampleFormat)mFrame->format, 0);
			av_opt_set_sample_fmt(mSwrContext, "out_sample_fmt", AV_SAMPLE_FMT_S16,  0);
			int ret = swr_init(mSwrContext);
			if (ret < 0) {
				LOGE("init context failed \n");
			}
		}
		int ret = swr_convert(mSwrContext, (uint8_t**)&mSamples, mFrame->nb_samples,
		 (const uint8_t **)mFrame->extended_data, mFrame->nb_samples);
		if (ret != mFrame->nb_samples) {
			LOGE("conversion wrong \n");
		}
#else
		// my conversion
		if (mFrame->format == AV_SAMPLE_FMT_FLTP) {
			int nb_samples = mFrame->nb_samples;
			int channels = mFrame->channels;
			short *samples = (short*) mSamples;
			for (int i = 0; i < nb_samples; i++) {
				 for (int c = 0; c < channels; c++) {
					 float* extended_data = (float*)mFrame->extended_data[c];
					 float sample = extended_data[i];
					 if (sample < -1.0f) {
						 sample = -1.0f;
					 } else if (sample > 1.0f) {
						 sample = 1.0f;
					 }
					 samples[i * channels + c] = (short)round(sample * 32767.0f);
				 }
			}
		} else if (mFrame->format == AV_SAMPLE_FMT_S16P) {
			int nb_samples = mFrame->nb_samples;
			int channels = mFrame->channels;
			short *samples = (short*) mSamples;
			for (int i = 0; i < nb_samples; i++) {
				 for (int c = 0; c < channels; c++) {
					 short* extended_data = (short*)mFrame->extended_data[c];
					 short sample = extended_data[i];
					 samples[i * channels + c] = sample;
				 }
			}
		}
#endif
		LOGD("conversion finished \n");

		// call handler for posting buffer to os audio driver
	    onDecoded(mSamples, size);
	    //fwrite(mSamples, 1, size, file_out);
	    //fwrite(mFrame->extended_data[0], 1, size, file_out_f);
	    //fwrite(mFrame->extended_data[1], 1, size, file_out_f);
	}

    return 0;
}

int AudioDecoder::decode(void* ptr) {
    AVPacket packet;
    file_out = fopen("/sdcard/audio.pcm", "wb");
    file_out_f = fopen("/sdcard/audio_f.pcm", "wb");

    while (mRunning) {
        if (outqueue(&packet) != 0) {
        	// aborted
            mRunning = false;
            continue;
        }
        if (memcmp(packet.data, "FLUSH", sizeof("FLUSH")) == 0) {
			avcodec_flush_buffers(mStream->codec);
			LOGD("FLUSH");
			continue;
        }

		LOGD("out queue an audio packet; queue size: %d \n", this->queneSize());
        if (process(&packet) != 0) {
			LOGE("process audio packet failed \n");
			mRunning = false;
			continue;
        }

        // free the packet that was allocated by av_read_frame
        av_free_packet(&packet);
    }

	// free the frame
	avcodec_free_frame(&mFrame);
	free(mSamples);
	if (mSwrContext != NULL) {
		swr_free(&mSwrContext);
	}
	fflush(file_out);
	fclose(file_out);
	fflush(file_out_f);
	fclose(file_out_f);

	detachJVM();

	return 0;
}
