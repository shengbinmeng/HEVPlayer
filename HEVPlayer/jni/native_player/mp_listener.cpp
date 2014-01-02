#include <jni.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include "mp_listener.h"
#include "jni_utils.h"
#include "gl_renderer.h"

VideoFrame *gVF;

struct fields_t {
    jfieldID    context;
    jmethodID   postEvent;
    jmethodID	audioTrackWrite;
    jmethodID	drawFrame;
};
static fields_t gFields;

static jclass gClass;
static JNIEnv *gEnvLocal, *gEnvLocal2;
static jshortArray gDataArray;
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

    // hold onto the MediaPlayer class for use in calling the static method
    gClass = clazz;
    gEnvLocal = gEnvLocal2 = NULL;
    gDataArray = NULL;
    gDataArraySize = 0;
	gVF = NULL;
}

MediaPlayerListener::~MediaPlayerListener() {

}

void MediaPlayerListener::postEvent(int msg, int ext1, int ext2) {
	JNIEnv *env = getJNIEnv();
	env->CallStaticVoidMethod(gClass, gFields.postEvent, msg, ext1, ext2, 0);
}

int MediaPlayerListener::audioTrackWrite(void* data, int offset, int size) {
	size = size / 2;  // from byte to short
	if (gEnvLocal == NULL) {
		gEnvLocal = getJNIEnv();
	}
    if (gDataArray == NULL || gDataArraySize < size) {
    	gDataArray = gEnvLocal->NewShortArray(size);
    	gDataArraySize = size;
    }
    gEnvLocal->SetShortArrayRegion(gDataArray, 0, size, (jshort*)data);
    int ret = gEnvLocal->CallStaticIntMethod(gClass, gFields.audioTrackWrite, gDataArray, offset, size);
    return ret;
}

int MediaPlayerListener::drawFrame(VideoFrame *vf) {
	gVF = vf;
	if (gEnvLocal2 == NULL) {
		gEnvLocal2 = getJNIEnv();
	}
    return gEnvLocal2->CallStaticIntMethod(gClass, gFields.drawFrame, vf->width, vf->height);
}
