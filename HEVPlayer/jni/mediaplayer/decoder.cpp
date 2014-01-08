#include "decoder.h"
#include "player_utils.h"

#define LOG_TAG "Decoder"

Decoder::Decoder(AVStream* stream) {
	mQueue = new PacketQueue();
	mStream = stream;
}

Decoder::~Decoder() {
	free(mQueue);
}

void Decoder::enqueue(AVPacket* packet) {
	mQueue->put(packet);
}

int Decoder::queneSize() {
	return mQueue->size();
}

void Decoder::flushQueue() {
	mQueue->flush();
}

void Decoder::stop() {
	mQueue->abort();
	int ret = -1;
	if ((ret = join()) != 0) {
		LOGE("decoder join failed: %i \n", ret);
		return;
	}
}

void Decoder::run(void* ptr) {
	if (prepare() != 0) {
		LOGE("decoder prepare failed \n");
		return;
	}
	decode(ptr);
}
