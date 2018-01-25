#include "FrameQueue.h"
#include "player_utils.h"

#define LOG_TAG "FrameQueue"

void FrameQueue::flush() {
	pthread_mutex_lock(&mLock);
	
	QueueItem *item = mFirst;
	while (item != NULL) {
		QueueItem *next = item->next;
		VideoFrame *vf = (VideoFrame *)item->data;
		free(vf->yuv_data[0]);
		free(item->data);
		free(item);
		item = next;
	}
	
	mLast = NULL;
	mFirst = NULL;
	mSize = 0;

	pthread_mutex_unlock(&mLock);
}

int FrameQueue::put(VideoFrame *vf) {
	QueueItem *item = (QueueItem *)malloc(sizeof(QueueItem));
	if (item == NULL) {
		LOGE("malloc queue item failed\n");
		return -1;
	}
	item->data = vf;
	Queue::put(item);
	return 0;
}

int FrameQueue::get(VideoFrame **vf, bool wait) {
	QueueItem *item;
	int ret = Queue::get(&item, wait);
	if (ret == 0) {
		*vf = (VideoFrame *)item->data;
	}
	return ret;

}
