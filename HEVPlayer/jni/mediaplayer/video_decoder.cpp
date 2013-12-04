#include "video_decoder.h"
#include "player_utils.h"

#define LOG_TAG "VideoDecoder"

static uint64_t global_video_pkt_pts = AV_NOPTS_VALUE;

VideoDecoder::VideoDecoder(AVStream* stream) : Decoder(stream) {
	mStream->codec->get_buffer = getBuffer;
	mStream->codec->release_buffer = releaseBuffer;
}

VideoDecoder::~VideoDecoder() {

}

int VideoDecoder::prepare() {
	mVideoClock = 0;
	mFrame = avcodec_alloc_frame();
	if (mFrame == NULL) {
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
		//return false;
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
			LOGE("get video packet failed \n");
			mRunning = false;
			return -1;
		}
		if (memcmp(packet.data, "FLUSH", sizeof("FLUSH")) == 0) {
			avcodec_flush_buffers(mStream->codec);
			LOGD("FLUSH");
			continue;
		} LOGD("out queue a video packet; queue size: %d \n", this->queneSize());
		if (!process(&packet)) {
			mRunning = false;
			return -1;
		}

		av_free_packet(&packet);
	}

	// free the frame
	av_free(mFrame);

	return 0;
}

/* These are called whenever we allocate a frame
 * buffer. We use this to store the global_pts in
 * a frame at the time it is allocated.
 */
int VideoDecoder::getBuffer(struct AVCodecContext *c, AVFrame *pic) {
	int ret = avcodec_default_get_buffer(c, pic);
	uint64_t *pts = (uint64_t *) av_malloc(sizeof(uint64_t));
	*pts = global_video_pkt_pts;
	pic->opaque = pts;
	return ret;
}
void VideoDecoder::releaseBuffer(struct AVCodecContext *c, AVFrame *pic) {
	if (pic) {
		av_freep(&pic->opaque);
	}
	avcodec_default_release_buffer(c, pic);
}
