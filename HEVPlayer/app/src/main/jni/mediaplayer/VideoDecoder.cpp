#include "VideoDecoder.h"
#include "player_utils.h"

#define LOG_TAG "VideoDecoder"

VideoDecoder::VideoDecoder(AVStream* stream) : Decoder(stream) {
	mVideoClock = 0;
}

VideoDecoder::~VideoDecoder() {

}

int VideoDecoder::decode(AVPacket *packet) {
	// Handle customized special packet.
	if (memcmp(packet->data, "END", sizeof("END")) == 0) {
		// send NULL to indicate ending.
		onDecoded(NULL, -1);
		LOGI("end of video decoding\n");
		return 0;
	}
	
	int gotFrame = 0;
	// decode video frame
	int len = avcodec_decode_video2(mStream->codec, mFrame, &gotFrame, packet);
	if (len < 0) {
		LOGE("decode video packet failed\n");
	}

	LOGD("video packet dts: %lld = %lld; packet pts: %lld = %lld; frame pts: %lld; codec time base: %lf, stream time base: %lf\n",
			packet->dts, mFrame->pkt_dts, packet->pts, mFrame->pkt_pts, mFrame->pts, av_q2d(mStream->codec->time_base), av_q2d(mStream->time_base));

	if (gotFrame) {
		mFrame->pts = av_frame_get_best_effort_timestamp(mFrame);
		if (mFrame->pts != AV_NOPTS_VALUE) {
			// Time stamp is meaningful, so we use it.
			mVideoClock = av_q2d(mStream->time_base) * mFrame->pts;
			LOGD("best effort frame pts: %lld\n", mFrame->pts);
		} else {
			// Actually I don't know what to do.
			mVideoClock += av_q2d(mStream->codec->time_base);
			LOGD("time stamp is AV_NOPTS_VALUE: %lld\n", mFrame->pts);
		}

		/* if we are repeating a frame, adjust clock accordingly */
		double frameDelay = mFrame->repeat_pict * (av_q2d(mStream->codec->time_base) * 0.5);
		mVideoClock += frameDelay;

		LOGD("output a video frame, mVideoClock: %lf\n", mVideoClock);

		onDecoded(mFrame, mVideoClock);
	}
	
	return 0;
}
