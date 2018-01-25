#ifndef __MEDIAPLAYER_H__
#define __MEDIAPLAYER_H__

#include <pthread.h>
#include "AudioDecoder.h"
#include "VideoDecoder.h"
#include "FrameQueue.h"
#include "PlayerListener.h"

class MediaPlayer {
public:
	MediaPlayer();
	~MediaPlayer();
	int setListener(PlayerListener *listener);
	int setThreadNumber(int num);
	int setLoopPlay(int loop);
	int open(char* file);
	int start();
	int pause();
	int go();
	int stop();
	int close();
	bool isPlaying();
	int getVideoWidth(int *w);
	int getVideoHeight(int *h);
	int seekTo(int msec);
	int getCurrentPosition(int *msec);
	int getDuration(int *msec);
	int getAudioParams(int *params);
	int wait();

private:
	int mThreadNumber;
	int mLoopPlay;
	int mNeedSeek;
	int mDemuxed;
	int mPause;
	int mStop;
	int mPrepared;

	int mFrameCount;
	double mTimeStart;
	int mFrames;
	double mTimeLast;
	AVPacket mFlushPacket;
	AVPacket mEndPacket;

	pthread_mutex_t mLock;

	pthread_t mDemuxingThread;
	pthread_t mRenderingThread;

	PlayerListener* mListener;
	AVFormatContext* mFormatContext;
	int mAudioStreamIndex;
	int mVideoStreamIndex;
	AudioDecoder* mAudioDecoder;
	VideoDecoder* mVideoDecoder;

	int mVideoWidth;
	int mVideoHeight;
	FrameQueue* mFrameQueue;

	// in ms
	int64_t mDuration;
	int64_t mCurrentPosition;
	int64_t mSeekPosition;

	// in seconds
	double mTime;
	double mAudioClock;
	double mFrameLastDelay, mFrameLastPTS;

	int prepareAudio();
	int prepareVideo();

	void demuxMedia(void* ptr);
	void renderVideo(void* ptr);

	static void* startDemuxing(void* ptr);
	static void* startRendering(void* ptr);

	static void videoOutput(AVFrame* frame, double pts);
	static void audioOutput(void* buffer, int buffer_size);

};

#endif
