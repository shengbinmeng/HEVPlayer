#ifndef __MEDIAPLAYER_H__
#define __MEDIAPLAYER_H__

#include <pthread.h>
#include "audio_decoder.h"
#include "video_decoder.h"
#include "framequeue.h"

enum media_player_state {
	MEDIA_PLAYER_STATE_ERROR = 1 << 0,
	MEDIA_PLAYER_IDLE = 1 << 1,
	MEDIA_PLAYER_INITIALIZED = 1 << 2,
	MEDIA_PLAYER_PREPARED = 1 << 3,
	MEDIA_PLAYER_DECODED = 1 << 4,
	MEDIA_PLAYER_STARTED = 1 << 5,
	MEDIA_PLAYER_PAUSED = 1 << 6,
	MEDIA_PLAYER_STOPPED = 1 << 7,
	MEDIA_PLAYER_PLAYBACK_COMPLETE = 1 << 8
};

class MediaPlayerListener {
public:
	MediaPlayerListener();
	~MediaPlayerListener();
	void postEvent(int msg, int ext1, int ext2);
	int audioTrackWrite(void* data, int offset, int size);
	int audioTrackFlush();
	int drawFrame(VideoFrame* vf);
};

class MediaPlayer {
public:
	MediaPlayer();
	~MediaPlayer();
	int setListener(MediaPlayerListener *listener);
	int setDataSource(const char *url);
	int setThreadNumber(int num);
	int prepare();
	int start();
	int stop();
	int pause();
	int go();
	bool isPlaying();
	int getVideoWidth(int *w);
	int getVideoHeight(int *h);
	int seekTo(int msec);
	int getCurrentPosition(int *msec);
	int getDuration(int *msec);
	int getAudioParams(int *params);

	pthread_mutex_t mLock;
	pthread_cond_t mCondition;

private:
	char mDataSource[1024];
	int mThreadNumber;

	media_player_state mCurrentState;

	pthread_t mDecodingThread;
	pthread_t mRenderingThread;

	MediaPlayerListener* mListener;
	AVFormatContext* mFormatContext;
	int mAudioStreamIndex;
	int mVideoStreamIndex;
	AudioDecoder* mAudioDecoder;
	VideoDecoder* mVideoDecoder;

	int mVideoWidth;
	int mVideoHeight;
	FrameQueue mFrameQueue;

	int64_t mDuration;
	int64_t mCurrentPosition;
	int64_t mSeekPosition;

	double mTime;
	double mAudioClock;
	double mFrameLastDelay, mFrameLastPTS;

	int prepareAudio();
	int prepareVideo();

	void decodeMedia(void* ptr);
	void renderVideo(void* ptr);

	static void* startDecoding(void* ptr);
	static void* startRendering(void* ptr);

	static void videoOutput(AVFrame* frame, double pts);
	static void audioOutput(int16_t* buffer, int buffer_size);
};

#endif
