#include <jni.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include "mp_listener.h"
#include "jni_utils.h"
#include "gl_renderer.h"

#define LOG_TAG "mp_listener"

VideoFrame *gVF;
pthread_mutex_t gVFMutex;

struct fields_t {
    jfieldID    context;
    jmethodID   postEvent;
    jmethodID	audioTrackWrite;
    jmethodID	drawFrame;
};
static fields_t gFields;

static jclass gClass;
static JNIEnv *gEnvLocal, *gEnvLocal2;
static jbyteArray gByteArray;
static jshortArray gShortArray;
static int gDataArraySize;

MediaPlayerListener::MediaPlayerListener() {
	JNIEnv *env = getJNIEnv();
	jclass clazz = env->FindClass("pku/shengbin/hevplayer/MediaPlayer");
	if (clazz == NULL) {
		jniThrowException(env, "java/lang/RuntimeException", "Can't find MediaPlayer");
		return;
	}

	gFields.postEvent = env->GetStaticMethodID(clazz, "postEventFromNative", "(III)V");
	if (gFields.postEvent == NULL) {
		jniThrowException(env, "java/lang/RuntimeException", "Can't find MediaPlayer.postEventFromNative");
		return;
	}

	gFields.drawFrame = env->GetStaticMethodID(clazz, "drawFrame","(II)I");
	if (gFields.drawFrame == NULL) {
		jniThrowException(env, "java/lang/RuntimeException", "Can't find MediaPlayer.drawFrame");
		return;
	}

	gFields.audioTrackWrite = env->GetStaticMethodID(clazz, "audioTrackWrite","([SII)I");
	if (gFields.audioTrackWrite == NULL) {
		jniThrowException(env, "java/lang/RuntimeException", "Can't find MediaPlayer.audioTrackWrite");
		return;
	}

    // hold onto the MediaPlayer class for use in calling the static method
    gClass = clazz;
    gEnvLocal = gEnvLocal2 = NULL;
    gByteArray = NULL;
    gShortArray = NULL;
    gDataArraySize = 0;
	gVF = NULL;
	pthread_mutex_init(&gVFMutex, NULL);
}

MediaPlayerListener::~MediaPlayerListener() {

}

void MediaPlayerListener::postEvent(int msg, int ext1, int ext2) {
	JNIEnv *env = getJNIEnv();
	env->CallStaticVoidMethod(gClass, gFields.postEvent, msg, ext1, ext2, 0);
}

int MediaPlayerListener::audioTrackWrite(void* data, int offset, int data_size) {
	if (data == NULL) {
		return -1;
	}
	if (gEnvLocal == NULL) {
		gEnvLocal = getJNIEnv();
	}
	int size = data_size / 2;
	if (gShortArray == NULL || gDataArraySize < size) {
		gShortArray = gEnvLocal->NewShortArray(size);
		gDataArraySize = size;
	}
	gEnvLocal->SetShortArrayRegion(gShortArray, 0, size, (jshort*)data);
	LOGD("write to audio track: %d shorts", size);
	int ret = gEnvLocal->CallStaticIntMethod(gClass, gFields.audioTrackWrite, gShortArray, offset, size);
	return ret;

}

int MediaPlayerListener::drawFrame(VideoFrame *vf) {
	pthread_mutex_lock(&gVFMutex);
	gVF = vf;
	pthread_mutex_unlock(&gVFMutex);

	if (gEnvLocal2 == NULL) {
		gEnvLocal2 = getJNIEnv();
	}
    return gEnvLocal2->CallStaticIntMethod(gClass, gFields.drawFrame, vf->width, vf->height);
}
