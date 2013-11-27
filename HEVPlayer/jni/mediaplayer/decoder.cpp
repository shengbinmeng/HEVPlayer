#include "decoder.h"
#include "jni_utils.h"

#define LOG_TAG "Decoder"

Decoder::Decoder(AVStream* stream)
{
	mQueue = new PacketQueue();
	mStream = stream;
}

Decoder::~Decoder()
{
	if (mRunning) {
        stop();
    }
	free(mQueue);
	avcodec_close(mStream->codec);
}

void Decoder::enqueue(AVPacket* packet)
{
	mQueue->put(packet);
}

int Decoder::queneSize()
{
	return mQueue->size();
}

void Decoder::flushQueue()
{
	mQueue->flush();
}

void Decoder::stop()
{
    mQueue->abort();
    int ret = -1;
    if((ret = join()) != 0) {
        LOGE("Couldn't cancel Decoder: %i", ret);
        return;
    }
}

void Decoder::run(void* ptr)
{
	if (!prepare()) {
		LOGI("Couldn't prepare decoder");
        return;
    }
	decode(ptr);
}

bool Decoder::prepare()
{
    return false;
}

bool Decoder::process(AVPacket *packet)
{
	return false;
}

bool Decoder::decode(void* ptr)
{
    return false;
}
