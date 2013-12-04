#ifndef __DECODER_H__
#define __DECODER_H__

#include "thread.h"
#include "packetqueue.h"
#include "player_utils.h"
extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
}

class Decoder: public Thread {
public:
	Decoder(AVStream* stream);
	~Decoder();

	void enqueue(AVPacket* packet);
	int queneSize();
	void flushQueue();
	void stop();

protected:
	PacketQueue* mQueue;
	AVStream* mStream;

	virtual int prepare() = 0;
	virtual int decode(void* ptr) = 0;
	virtual int process(AVPacket *packet) = 0;

	void run(void* ptr);
};

#endif //DECODER_H
