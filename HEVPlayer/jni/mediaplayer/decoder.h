#ifndef __DECODER_H__
#define __DECODER_H__

#include "thread.h"
#include "packetqueue.h"
#include "jni_utils.h"

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
}

class Decoder : public Thread
{
public:
	Decoder(AVStream* stream);
	~Decoder();
	
    void						stop();
	void						enqueue(AVPacket* packet);
	int							queneSize();
	void 						flushQueue();

protected:
    PacketQueue*                mQueue;
    AVStream*             		mStream;

    virtual bool                prepare();
    virtual bool                decode(void* ptr);
    virtual bool                process(AVPacket *packet);
	void						run(void* ptr);
};

#endif //DECODER_H
