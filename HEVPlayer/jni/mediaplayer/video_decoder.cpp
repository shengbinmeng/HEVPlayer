#include "video_decoder.h"
#include "player_utils.h"

#define LOG_TAG "VideoDecoder"

VideoDecoder::VideoDecoder(AVStream* stream) : Decoder(stream) {

}

VideoDecoder::~VideoDecoder() {

}

int VideoDecoder::prepare() {
	mVideoClock = 0;
	mFrame = avcodec_alloc_frame();
	if (mFrame == NULL) {
		LOGE("avcodec_alloc_frame failed \n");
		return -1;
	}
	return 0;
}

double VideoDecoder::synchronize(AVFrame *frame, double pts) {
	double frameDelay;
	if (pts != 0) {
		/* if we have pts, set video clock to it */
		mVideoClock = pts;
	} else {
		/* if we aren't given a pts, set it to the clock */
		pts = mVideoClock;
	}
	/* update the video clock */
	frameDelay = av_q2d(mStream->codec->time_base);
	/* if we are repeating a frame, adjust clock accordingly */
	frameDelay += frame->repeat_pict * (frameDelay * 0.5);
	mVideoClock += frameDelay;
	return pts;
}

int VideoDecoder::process(AVPacket *packet) {
	int gotFrame = 0;
	double pts = 0;
	// decode video frame
	if (avcodec_decode_video2(mStream->codec, mFrame, &gotFrame, packet) < 0) {
		LOGE("decode video packet failed");
	}

	if (packet->dts == AV_NOPTS_VALUE && mFrame->opaque
			&& *(uint64_t*) mFrame->opaque != AV_NOPTS_VALUE) {
		pts = *(uint64_t *) mFrame->opaque;
	} else if (packet->dts != AV_NOPTS_VALUE) {
		pts = packet->dts;
	} else {
		pts = 0;
	}

	if (gotFrame) {
		pts *= av_q2d(mStream->time_base);
		pts = synchronize(mFrame, pts);

		onDecoded(mFrame, pts);
	}

	return 0;
}

int VideoDecoder::decode(void* ptr) {
	AVPacket packet;

	while (mRunning) {
		if (mQueue->get(&packet, true) < 0) {
			// aborted
			mRunning = false;
			continue;
		}
		if (memcmp(packet.data, "FLUSH", sizeof("FLUSH")) == 0) {
			avcodec_flush_buffers(mStream->codec);
			LOGD("FLUSH");
			continue;
		}
		LOGD("out queue a video packet; queue size: %d \n", this->queneSize());
		if (process(&packet) != 0) {
			LOGE("process video packet failed \n");
			mRunning = false;
			continue;
		}

		av_free_packet(&packet);
	}

	// free the frame
	av_free(mFrame);

	return 0;
}
