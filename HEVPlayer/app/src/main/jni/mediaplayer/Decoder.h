#ifndef __DECODER_H__
#define __DECODER_H__

#include "PacketQueue.h"
#include "player_utils.h"
extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
}

class Decoder {
public:
	Decoder(AVStream* stream);
	~Decoder();

	int start();
	int stop();
	int join();
	void waitOnNotify();
	void notify();
	
	int enqueue(AVPacket* packet);
	int queueSize();
	void flushQueue();
	void endQueue();

protected:
	PacketQueue* mQueue;
	AVStream* mStream;
	AVFrame* mFrame;
	int dequeue(AVPacket* packet);
	
private:
	static void* startThread(void* ptr);
	pthread_t mThread;
	pthread_mutex_t mLock;
	pthread_cond_t mCondition;
	bool mBlock;
	bool mRunning;
	void run(void* ptr);
	virtual int decode(AVPacket *packet) = 0;
};

#endif //DECODER_H
