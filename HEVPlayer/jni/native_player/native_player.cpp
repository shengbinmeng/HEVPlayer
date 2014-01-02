
#include <jni.h>
#include "jni_utils.h"
#include "mediaplayer.h"
#include "mp_listener.h"

#define LOG_TAG    "native_player"

static MediaPlayer *gMP;

static int MediaPlayer_setDataSource(JNIEnv *env, jobject thiz, jstring path) {
	const char *pathStr = env->GetStringUTFChars(path, NULL);

	env->ReleaseStringUTFChars(path, pathStr);

	return 0;
}

static int MediaPlayer_prepare(JNIEnv *env, jobject thiz, jint threadNumber, jfloat fps) {
	int ret = gMP->prepare();
	return ret;
}

static int MediaPlayer_start(JNIEnv *env, jobject thiz) {
	int ret = gMP->start();
	return ret;
}

static int MediaPlayer_pause(JNIEnv *env, jobject thiz) {
	int ret = gMP->pause();
	return ret;
}

static int MediaPlayer_go(JNIEnv *env, jobject thiz) {
	int ret = gMP->go();
	return ret;
}


static int MediaPlayer_stop(JNIEnv *env, jobject thiz) {
	int ret = gMP->stop();
	return ret;
}

static bool MediaPlayer_isPlaying(JNIEnv *env, jobject thiz) {
	bool ret = gMP->isPlaying();
	return ret;
}

static int MediaPlayer_seekTo(JNIEnv *env, jobject thiz, jint msec) {
	int ret = gMP->seekTo(msec);
	return ret;
}

static int MediaPlayer_getVideoWidth(JNIEnv *env, jobject thiz) {
    int w = 0;
    gMP->getVideoWidth(&w);
    return w;
}

static int MediaPlayer_getVideoHeight(JNIEnv *env, jobject thiz) {
    int h = 0;
    gMP->getVideoHeight(&h);
    return h;
}


static int MediaPlayer_getCurrentPosition(JNIEnv *env, jobject thiz) {
    int msec = 0;
    gMP->getCurrentPosition(&msec);
    return msec;
}

static int MediaPlayer_getDuration(JNIEnv *env, jobject thiz) {
    int msec = 0;
    gMP->getDuration(&msec);
    return msec;
}


static void native_init(JNIEnv *env, jobject thiz) {
	MediaPlayer* mp = new MediaPlayer();
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
    {"native_init",         "(Ljava/lang/Object;)V",            (void *)native_init},
};

int register_player(JNIEnv *env) {
	return jniRegisterNativeMethods(env, "pku/shengbin/hevplayer/MediaPlayer", gMethods, sizeof(gMethods) / sizeof(gMethods[0]));
}
