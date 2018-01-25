#include "Queue.h"

#define LOG_TAG "Queue"

Queue::Queue() {
	pthread_mutex_init(&mLock, NULL);
	pthread_cond_init(&mCondition, NULL);
	mFirst = NULL;
	mLast = NULL;
	mSize = 0;
	mAbort = false;
}

Queue::~Queue() {
	flush();
	pthread_mutex_destroy(&mLock);
	pthread_cond_destroy(&mCondition);
}

int Queue::size() {
	pthread_mutex_lock(&mLock);
	int size = mSize;
	pthread_mutex_unlock(&mLock);
	return size;
}

void Queue::flush() {
	pthread_mutex_lock(&mLock);

	QueueItem *item = mFirst;
	while (item != NULL) {
		QueueItem *next = item->next;
		free(item->data);
		free(item);
		item = next;
	}

	mLast = NULL;
	mFirst = NULL;
	mSize = 0;

	pthread_mutex_unlock(&mLock);
}

int Queue::put(QueueItem *item) {
	pthread_mutex_lock(&mLock);

	if (mLast == NULL) {
		// The queue has no item. We are putting the first one.
		mFirst = item;
	} else {
		mLast->next = item;
	}

	item->next = NULL;
	mLast = item;
	mSize++;

	pthread_cond_signal(&mCondition);
	pthread_mutex_unlock(&mLock);
	return 0;
}

int Queue::get(QueueItem **item, bool wait) {
	int ret = 0;

	pthread_mutex_lock(&mLock);

	while (1) {
		if (mAbort) {
			ret = -1;
			break;
		}

		if (mFirst) {
			// The queue has items.
			*item = mFirst;
			mFirst = mFirst->next;
			if (mFirst == NULL) {
				// The queue becomes empty.
				mLast = NULL;
			}
			mSize--;
			break;
		} else {
			// The queue has no item.
			if (!wait) {
				ret = -1;
				break;
			} else {
				pthread_cond_wait(&mCondition, &mLock);
			}
		}
	}

	pthread_mutex_unlock(&mLock);
	return ret;
}

void Queue::abort() {
	pthread_mutex_lock(&mLock);
	mAbort = true;
	pthread_cond_signal(&mCondition);
	pthread_mutex_unlock(&mLock);
}
