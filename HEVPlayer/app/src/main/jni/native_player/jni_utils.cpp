#include <stdlib.h>
#include <android/log.h>
#include "jni_utils.h"
#include <pthread.h>

#define LOG_TAG "jni_utils"

static JavaVM *gVM;
static pthread_key_t gKey;

extern int register_player(JNIEnv *env);
extern int register_renderer(JNIEnv *env);

/*
 * Throw an exception with the specified class and an optional message.
 */
int jniThrowException(JNIEnv* env, const char* className, const char* msg) {
	jclass exceptionClass = env->FindClass(className);
	if (exceptionClass == NULL) {
		LOGE("Unable to find exception class %s \n", className);
		return -1;
	}
	if (env->ThrowNew(exceptionClass, msg) != JNI_OK) {
		LOGE("Failed throwing '%s' '%s' \n", className, msg);
	}
	return 0;
}

JNIEnv* getJNIEnv() {
    JNIEnv *env = (JNIEnv *) pthread_getspecific(gKey);
	if (env != NULL) {
        return  env;
    }
	int ret = gVM->GetEnv((void**) &env, JNI_VERSION_1_4);
	if (ret == JNI_OK) {
        pthread_setspecific(gKey, env);
        return env;
	} else if (ret == JNI_EDETACHED) {
		jint attachSuccess = gVM->AttachCurrentThread(&env, NULL);
		if (attachSuccess != 0) {
			LOGE("attach current thread failed \n");
            return NULL;
		}
	} else {
		LOGE("obtain JNIEnv failed, return: %d \n", ret);
	}
    pthread_setspecific(gKey, env);
    return env;
}

void detachJVM() {
	int ret;
	ret = gVM->DetachCurrentThread();
	if (ret == JNI_OK) {
		LOGI("detach return OK: %d \n", ret);
	} else {
		LOGE("detach return NOT OK: %d \n", ret);
	}
}

void detachCurrentThread(void* a) {
    pthread_t this_thread = pthread_self();
    LOGI("detachCurrentThread, pthread_self() = %ld", this_thread);
    detachJVM();
}

/*
 * Register native JNI-callable methods.
 *
 * "className" looks like "java/lang/String".
 */
int jniRegisterNativeMethods(JNIEnv* env, const char* className,
		const JNINativeMethod* gMethods, int numMethods) {
	jclass clazz;

	LOGI("Registering %s natives \n", className);
	clazz = env->FindClass(className);
	if (clazz == NULL) {
		LOGE("Native registration unable to find class '%s' \n", className);
		return -1;
	}
	if (env->RegisterNatives(clazz, gMethods, numMethods) < 0) {
		LOGE("RegisterNatives failed for '%s' \n", className);
		return -1;
	}
	return 0;
}

jint JNI_OnLoad(JavaVM* vm, void* reserved) {
	JNIEnv* env = NULL;
	jint result = JNI_ERR;
	gVM = vm;
    pthread_key_create(&gKey, detachCurrentThread);

	if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
		LOGE("GetEnv failed! \n");
		return JNI_ERR;
	}

	LOGI("loading . . . \n");
	if (register_player(env) != JNI_OK) {
		LOGE("can't register player \n");
		return JNI_ERR;
	}
	if (register_renderer(env) != JNI_OK) {
		LOGE("can't register renderer \n");
		return JNI_ERR;
	}
	LOGI("loaded \n");
	return JNI_VERSION_1_4;
}
