#include <jni.h>
#include <machine/cpu-features.h>
#include <android/log.h>

jint Java_pku_shengbin_hevplayer_NativeMediaPlayer_hasNeon
  (JNIEnv * env, jobject obj)
{
	//__android_log_print(ANDROID_LOG_DEBUG,"cpu","%d == %d",android_getCpuFamily(), ANDROID_CPU_FAMILY_ARM);
	//__android_log_print(ANDROID_LOG_DEBUG,"cpu"," %llu & %d",android_getCpuFeatures(), ANDROID_CPU_ARM_FEATURE_NEON);

	if (android_getCpuFamily() == 1 &&
		   (android_getCpuFeatures() & (1 << 2)) != 0) return 1;
	else return 0;
}
