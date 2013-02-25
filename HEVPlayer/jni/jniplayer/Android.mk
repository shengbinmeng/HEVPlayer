LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm
LOCAL_ARM_NEON = true

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../decoder
							
LOCAL_SRC_FILES := jniplayer.cpp jniUtils.cpp yuv2rgb565.cpp ../decoder/interface/decoder.cpp

LOCAL_LDLIBS := -llog -lz -ljnigraphics

LOCAL_STATIC_LIBRARIES := lentoid-dec

LOCAL_MODULE := jniplayer

include $(BUILD_SHARED_LIBRARY)