#ifndef __PACKETQUEUE_H__
#define __PACKETQUEUE_H__

#include "Queue.h"
extern "C" {
#include "libavcodec/avcodec.h"
}

class PacketQueue : public Queue
{
public:
	void flush();
	int put(AVPacket *pkt);
	int get(AVPacket *pkt, bool wait);
};

#endif
