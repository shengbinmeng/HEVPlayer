#include <stdlib.h>
#include <android/log.h>
#include "jniUtils.h"

#define TAG "JniUtils"

static JavaVM *sVm;

extern int register_jniplayer(JNIEnv *env);

/*
 * Throw an exception with the specified class and an optional message.
 */
int jniThrowException(JNIEnv* env, const char* className, const char* msg) {
    jclass exceptionClass = env->FindClass(className);
    if (exceptionClass == NULL) {
        __android_log_print(ANDROID_LOG_ERROR,
			    TAG,
			    "Unable to find exception class %s",
	                    className);
        return -1;
    }

    if (env->ThrowNew(exceptionClass, msg) != JNI_OK) {
        __android_log_print(ANDROID_LOG_ERROR,
			    TAG,
			    "Failed throwing '%s' '%s'",
			    className, msg);
    }
    return 0;
}

JNIEnv* getJNIEnv() {
    JNIEnv* env = NULL;
    int ret = sVm->GetEnv((void**) &env, JNI_VERSION_1_4);
    if (ret == JNI_OK) {
    	return env;
    }else if(ret == JNI_EDETACHED){
    	jint attachSuccess = sVm->AttachCurrentThread(&env,NULL);
    	//__android_log_print(ANDROID_LOG_DEBUG,TAG,"attach return: %d", attachSuccess);
        if(attachSuccess == 0) return  env;
		else
		{
			__android_log_print(ANDROID_LOG_ERROR,
								TAG,
								"AttachCurrentThread(void** penv, void* args) was not successful\
								 This may be due to the thread being attached already to another JVM instance\n");
			return NULL;
		}
    }else{
    	__android_log_print(ANDROID_LOG_ERROR,
    								TAG,
    								"Failed to obtain JNIEnv, return: %d", ret);
    }
    return env;
}

void detachJVM()
{
	int ret;
	ret=sVm->DetachCurrentThread();
	if (ret == JNI_OK)
		__android_log_print(ANDROID_LOG_DEBUG,TAG,"detach return OK: %d", ret);
	else __android_log_print(ANDROID_LOG_DEBUG,TAG,"detach return: %d", ret);
}


/*
 * Register native JNI-callable methods.
 *
 * "className" looks like "java/lang/String".
 */
int jniRegisterNativeMethods(JNIEnv* env,
                             const char* className,
                             const JNINativeMethod* gMethods,
                             int numMethods)
{
    jclass clazz;

    __android_log_print(ANDROID_LOG_INFO, TAG, "Registering %s natives\n", className);
    clazz = env->FindClass(className);
    if (clazz == NULL) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "Native registration unable to find class '%s'\n", className);
        return -1;
    }
    if (env->RegisterNatives(clazz, gMethods, numMethods) < 0) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "RegisterNatives failed for '%s'\n", className);
        return -1;
    }
    return 0;
}



jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    JNIEnv* env = NULL;
    jint result = JNI_ERR;
	sVm = vm;

    if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "GetEnv failed!");
        return result;
    }

    __android_log_print(ANDROID_LOG_INFO, TAG, "loading . . .");


    if(register_jniplayer(env) != JNI_OK) {
    	__android_log_print(ANDROID_LOG_ERROR, TAG, "can't load NativeMoviePlayer");
    }

    __android_log_print(ANDROID_LOG_INFO, TAG, "loaded");

    result = JNI_VERSION_1_4;

end:
    return result;
}
