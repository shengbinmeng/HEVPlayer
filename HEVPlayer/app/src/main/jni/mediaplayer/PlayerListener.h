#ifndef __PLAYERLISTENER_H__
#define __PLAYERLISTENER_H__

#include "FrameQueue.h"

class PlayerListener {
public:
	PlayerListener();
	~PlayerListener();
	void postEvent(int msg, int ext1, int ext2);
	int audioTrackWrite(void* data, int offset, int size);
	int drawFrame(VideoFrame* vf);
};

#endif
