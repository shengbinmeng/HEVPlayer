#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include "native_player.h"
#include "jni_utils.h"
#include "gl_renderer.h"
#include "mediaplayer.h"
extern "C" {
#include "lenthevcdec.h"
#include "libavutil/imgutils.h"
#include "libavformat/avformat.h"
}

#define LOG_TAG    "native_player"

struct fields_t {
    jfieldID    context;
    jmethodID   postEvent;
    jmethodID	audioTrackWrite;
    jmethodID	drawFrame;
};
static fields_t fields;

static jclass gClass;
static JNIEnv *gEnvLocal, *gEnvLocal2;
static jshortArray gDataArray;
static int gDataArraySize;

VideoFrame *gVF;
static MediaPlayer *gMP;

MediaPlayerListener::MediaPlayerListener()
{
    // hold onto the MediaPlayer class for use in calling the static method
	JNIEnv *env = getJNIEnv();
    gClass = env->FindClass("pku/shengbin/hevplayer/MediaPlayer");

    gEnvLocal = gEnvLocal2 = NULL;
    gDataArray = NULL;
    gDataArraySize = 0;

    gVF = NULL;
    gMP = NULL;

}

MediaPlayerListener::~MediaPlayerListener()
{

}

void MediaPlayerListener::postEvent(int msg, int ext1, int ext2)
{
	JNIEnv *env = getJNIEnv();
	env->CallStaticVoidMethod(gClass, fields.postEvent, msg, ext1, ext2, 0);
}

int MediaPlayerListener::audioTrackWrite(void* data, int offset, int size)
{
	size = size/2;  // from byte to short
	if (gEnvLocal == NULL) {
		gEnvLocal = getJNIEnv();
	}
    if (gDataArray == NULL || gDataArraySize < size) {
    	gDataArray = gEnvLocal->NewShortArray(size);
    	gDataArraySize = size;
    }
    gEnvLocal->SetShortArrayRegion(gDataArray, 0, size, (jshort*)data);
    int ret = gEnvLocal->CallStaticIntMethod(gClass, fields.audioTrackWrite, gDataArray, offset, size);
    return ret;
}

int MediaPlayerListener::drawFrame(VideoFrame *vf)
{
	gVF = vf;
	if (gEnvLocal2 == NULL) {
		gEnvLocal2 = getJNIEnv();
	}
    return gEnvLocal2->CallStaticIntMethod(gClass, fields.drawFrame, vf->width, vf->height);
}



static int MediaPlayer_setDataSource(JNIEnv *env, jobject thiz, jstring path)
{
	const char *pathStr = env->GetStringUTFChars(path, NULL);

	env->ReleaseStringUTFChars(path, pathStr);

	return 0;
}

static int MediaPlayer_prepare(JNIEnv *env, jobject thiz, jint threadNumber, jfloat fps)
{

}

static int MediaPlayer_start(JNIEnv *env, jobject thiz)
{
	return 0;
}

static int MediaPlayer_pause(JNIEnv *env, jobject thiz)
{
	return 0;
}

static int MediaPlayer_go(JNIEnv *env, jobject thiz)
{
	return 0;
}


static int MediaPlayer_stop(JNIEnv *env, jobject thiz)
{

}

static bool MediaPlayer_isPlaying(JNIEnv *env, jobject thiz)
{

}

static int MediaPlayer_seekTo(JNIEnv *env, jobject thiz, jint msec)
{
	return 0;
}

static int MediaPlayer_getVideoWidth(JNIEnv *env, jobject thiz)
{
    int w = 0;
    return w;
}

static int MediaPlayer_getVideoHeight(JNIEnv *env, jobject thiz)
{
    int h = 0;
    return h;
}


static int MediaPlayer_getCurrentPosition(JNIEnv *env, jobject thiz)
{
    int msec = 0;
    return msec;
}

static int MediaPlayer_getDuration(JNIEnv *env, jobject thiz)
{
    int msec = 0;
    return msec;
}


static void MediaPlayer_native_init(JNIEnv *env, jobject thiz)
{
    jclass clazz;
    clazz = env->FindClass("pku/shengbin/hevplayer/MediaPlayer");
    if (clazz == NULL) {
        jniThrowException(env, "java/lang/RuntimeException", "Can't find MediaPlayer");
        return;
    }

    fields.postEvent = env->GetStaticMethodID(clazz, "postEventFromNative", "(III)V");
	if (fields.postEvent == NULL) {
		jniThrowException(env, "java/lang/RuntimeException", "Can't find MediaPlayer.postEventFromNative");
		return;
	}

	fields.drawFrame = env->GetStaticMethodID(clazz, "drawFrame","(II)I");
	if (fields.drawFrame == NULL) {
		jniThrowException(env, "java/lang/RuntimeException", "Can't find MediaPlayer.drawFrame");
		return;
	}

	MediaPlayer* mp = new MediaPlayer();
	if (mp == NULL) {
		LOGE("native init: mp == null");
		jniThrowException(env, "java/lang/RuntimeException", "Out of memory");
		return;
	}

	// create new listener and give it to MediaPlayer
	MediaPlayerListener* listener = new MediaPlayerListener();
	mp->setListener(listener);

	gMP = mp;
}


// ----------------------------------------------------------------------------

static JNINativeMethod gMethods[] = {
    {"setDataSource",       "(Ljava/lang/String;)I",            (void *)MediaPlayer_setDataSource},
    {"native_prepare",            "(IF)I",                             (void *)MediaPlayer_prepare},
    {"native_start",              "()I",                              (void *)MediaPlayer_start},
    {"native_stop",               "()I",                              (void *)MediaPlayer_stop},
    {"getVideoWidth",       "()I",                              (void *)MediaPlayer_getVideoWidth},
    {"getVideoHeight",      "()I",                              (void *)MediaPlayer_getVideoHeight},
    {"native_seekTo",             "(I)I",                             (void *)MediaPlayer_seekTo},
    {"native_pause",              "()I",                              (void *)MediaPlayer_pause},
    {"native_go",                 "()I",                              (void *)MediaPlayer_go},
    {"isPlaying",           "()Z",                              (void *)MediaPlayer_isPlaying},
    {"getCurrentPosition",  "()I",                              (void *)MediaPlayer_getCurrentPosition},
    {"getDuration",         "()I",                              (void *)MediaPlayer_getDuration},
    {"native_init",         "(Ljava/lang/Object;)V",            (void *)MediaPlayer_native_init},
};

int register_player(JNIEnv *env) {
	return jniRegisterNativeMethods(env, "pku/shengbin/hevplayer/NativeMediaPlayer", gMethods, sizeof(gMethods) / sizeof(gMethods[0]));
}
