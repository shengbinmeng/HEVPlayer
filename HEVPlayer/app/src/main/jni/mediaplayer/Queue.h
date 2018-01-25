#ifndef __QUEUE_H__
#define __QUEUE_H__

#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>

struct QueueItem {
	void *data;
	QueueItem *next;
};

class Queue {
public:
	Queue();
	~Queue();

	void flush();
	int put(QueueItem *item);

	/* return < 0 if aborted, 0 if no packet and > 0 if packet.  */
	int get(QueueItem **item, bool wait);

	int size();

	void abort();

protected:
	QueueItem* mFirst;
	QueueItem* mLast;
	int mSize;
	bool mAbort;
	pthread_mutex_t mLock;
	pthread_cond_t mCondition;
};

#endif
