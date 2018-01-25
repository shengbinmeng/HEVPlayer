#ifndef __FRAMEQUEUE_H__
#define __FRAMEQUEUE_H__

#include <stdint.h>
#include "Queue.h"

struct VideoFrame {
	int width;
	int height;
	int linesize_y;
	int linesize_uv;
	double pts;
	uint8_t* yuv_data[3];
};

class FrameQueue : public Queue {
public:
	void flush();
	int put(VideoFrame *vf);
	int get(VideoFrame **vf, bool block);
};

#endif
