#include <jni.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include "PlayerListener.h"
#include "jni_utils.h"

#define LOG_TAG "PlayerListener"

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
static jshortArray gShortArray;
static int gShortArrayLength;

PlayerListener::PlayerListener() {
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

	// Since Android ICS, class references are not global so we need to peg a
	// global reference to the jclass returned by FindClass(), otherwise we get
	// following error in the log:
	// "JNI ERROR (app bug): attempt to use stale local reference 0xHHHHHHHH".
	gClass = static_cast<jclass>(env->NewGlobalRef(clazz));
	gEnvLocal = gEnvLocal2 = NULL;
	gShortArray = NULL;
	gShortArrayLength = 0;
	gVF = NULL;
	pthread_mutex_init(&gVFMutex, NULL);
}

PlayerListener::~PlayerListener() {
	JNIEnv *env = getJNIEnv();
	env->DeleteGlobalRef(gClass);

	if (gVF != NULL) {
		free(gVF->yuv_data[0]);
		free(gVF);
		gVF = NULL;
	}

	if (gShortArray != NULL) {
		gEnvLocal->DeleteLocalRef(gShortArray);
		gShortArray = NULL;
	}

}

void PlayerListener::postEvent(int msg, int ext1, int ext2) {
	JNIEnv *env = getJNIEnv();
	env->CallStaticVoidMethod(gClass, gFields.postEvent, msg, ext1, ext2, 0);
}

int PlayerListener::audioTrackWrite(void* data, int offset, int data_size) {
	if (data == NULL) {
		return 1;
	}
	if (gEnvLocal == NULL) {
		gEnvLocal = getJNIEnv();
	}
	int size = data_size / 2;
	if (gShortArrayLength < size) {
		if (gShortArray != NULL) {
			gEnvLocal->DeleteLocalRef(gShortArray);
			gShortArray = NULL;
		}
		gShortArray = gEnvLocal->NewShortArray(size);
		gShortArrayLength = size;
	}
	gEnvLocal->SetShortArrayRegion(gShortArray, 0, size, (jshort*)data);
	LOGD("write to audio track: %d shorts \n", size);
	return gEnvLocal->CallStaticIntMethod(gClass, gFields.audioTrackWrite, gShortArray, offset, size);
}

int PlayerListener::drawFrame(VideoFrame *vf) {
	if (vf == NULL) {
		// tell the play activity to finish
		postEvent(909, 0, 0);
		return 1;
	}
	pthread_mutex_lock(&gVFMutex);
	if (gVF != NULL) {
		free(gVF->yuv_data[0]);
		free(gVF);
		gVF = NULL;
	}
	gVF = vf;
	pthread_mutex_unlock(&gVFMutex);

	if (gEnvLocal2 == NULL) {
		gEnvLocal2 = getJNIEnv();
	}
    return gEnvLocal2->CallStaticIntMethod(gClass, gFields.drawFrame, vf->width, vf->height);
}
