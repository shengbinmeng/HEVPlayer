LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm
LOCAL_ARM_NEON = true
							
LOCAL_SRC_FILES := cpufeature.c 
LOCAL_LDLIBS := -llog
LOCAL_WHOLE_STATIC_LIBRARIES := cpufeatures

LOCAL_MODULE := cpufeature

include $(BUILD_SHARED_LIBRARY)

$(call import-module,android/cpufeatures)