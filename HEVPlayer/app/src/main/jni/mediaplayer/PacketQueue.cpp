#include "PacketQueue.h"
#include "player_utils.h"

#define LOG_TAG "PacketQueue"

void PacketQueue::flush() {
	pthread_mutex_lock(&mLock);

	QueueItem *item = mFirst;
	while (item != NULL) {
		QueueItem *next = item->next;
		AVPacket *pkt = (AVPacket *)item->data;
		av_free_packet(pkt);
		free(item->data);
		free(item);
		item = next;
	}

	mLast = NULL;
	mFirst = NULL;
	mSize = 0;

	pthread_mutex_unlock(&mLock);
}

int PacketQueue::put(AVPacket *pkt) {
	QueueItem *item = (QueueItem *)malloc(sizeof(QueueItem));
	if (item == NULL) {
		LOGE("malloc queue item failed\n");
		return -1;
	}
	AVPacket *p = (AVPacket *)malloc(sizeof(AVPacket));
	*p = *pkt; // Copy the fields.
	item->data = p;
	Queue::put(item);
	return 0;
}

int PacketQueue::get(AVPacket *pkt, bool wait) {
	QueueItem *item;
	int ret = Queue::get(&item, wait);
	if (ret == 0) {
		AVPacket *p = (AVPacket *)item->data;
		*pkt = *p;
		free(p);
		free(item);
	}
	return ret;
}
