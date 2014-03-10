LOCAL_PATH := $(call my-dir)
ARCH_ABI := $(TARGET_ARCH_ABI)

## prebuilt the static libs  
include $(CLEAR_VARS)
LOCAL_MODULE := jpeg
LOCAL_SRC_FILES := lib/$(ARCH_ABI)/libjpeg.a
include $(PREBUILT_STATIC_LIBRARY)


include $(CLEAR_VARS)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../ffmpeg-2.0/include/ $(LOCAL_PATH)/../mediaplayer/
LOCAL_LDLIBS := -llog -lz
LOCAL_SRC_FILES := getframe.c
LOCAL_SHARED_LIBRARIES := ffmpeg
LOCAL_STATIC_LIBRARIES := jpeg
LOCAL_MODULE := getframe
include $(BUILD_SHARED_LIBRARY)