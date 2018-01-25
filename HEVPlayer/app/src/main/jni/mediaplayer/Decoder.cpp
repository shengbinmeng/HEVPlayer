#include "Decoder.h"
#include "player_utils.h"

#define LOG_TAG "Decoder"

Decoder::Decoder(AVStream* stream) {
	mStream = stream;
	mBlock = true;
}

Decoder::~Decoder() {

}

int Decoder::start() {
	mQueue = new PacketQueue();
	mFrame = av_frame_alloc();
	if (mFrame == NULL) {
		LOGE("avcodec_alloc_frame failed\n");
		return -1;
	}
	pthread_mutex_init(&mLock, NULL);
	pthread_cond_init(&mCondition, NULL);
	
	pthread_create(&mThread, NULL, startThread, this);
	mRunning = true;
    return 0;
}

int Decoder::enqueue(AVPacket* packet) {
	return mQueue->put(packet);
}

int Decoder::dequeue(AVPacket* packet) {
	return mQueue->get(packet, mBlock);
}

int Decoder::queueSize() {
	return mQueue->size();
}

void Decoder::flushQueue() {
	mQueue->flush();
}

int Decoder::stop() {
	mRunning = false;

	mQueue->abort();
	
	int ret = pthread_join(mThread, NULL);
	if (ret != 0) {
		LOGE("decoder join failed: %d\n", ret);
	}
	
	pthread_cond_destroy(&mCondition);
	pthread_mutex_destroy(&mLock);
	av_frame_free(&mFrame);
	delete mQueue;
	return ret;
}

void Decoder::waitOnNotify() {
	pthread_mutex_lock(&mLock);
	pthread_cond_wait(&mCondition, &mLock);
	pthread_mutex_unlock(&mLock);
}

void Decoder::notify() {
	pthread_mutex_lock(&mLock);
	pthread_cond_signal(&mCondition);
	pthread_mutex_unlock(&mLock);
}

void* Decoder::startThread(void* ptr) {
	Decoder* decoder = (Decoder *) ptr;
	decoder->run(ptr);
	return NULL;
}

void Decoder::run(void* ptr) {
	AVPacket packet;
	
	while (mRunning) {
		if (dequeue(&packet) != 0) {
			break;
		}
		
		LOGD("dequeue a packet; queue size: %d\n", this->queueSize());

		// Handle customized special packet.
		if (memcmp(packet.data, "FLUSH", sizeof("FLUSH")) == 0) {
			avcodec_flush_buffers(mStream->codec);
			LOGD("FLUSH");
			continue;
		}
		
		int ret = decode(&packet);
		
		av_free_packet(&packet);
		
		if (ret != 0) {
			break;
		}
	}
}
