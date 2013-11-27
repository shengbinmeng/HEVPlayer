LOCAL_PATH := $(call my-dir)

ARCH_ABI := $(TARGET_ARCH_ABI)

#
# FFMPEG prebuilt static library
#
include $(CLEAR_VARS)
LOCAL_MODULE	:= avutil
LOCAL_SRC_FILES	:= ../ffmpeg-2.0/lib/$(ARCH_ABI)/libavutil.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE	:= avcodec
LOCAL_SRC_FILES	:= ../ffmpeg-2.0/lib/$(ARCH_ABI)/libavcodec.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE	:= avformat
LOCAL_SRC_FILES	:= ../ffmpeg-2.0/lib/$(ARCH_ABI)/libavformat.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE	:= swscale
LOCAL_SRC_FILES	:= ../ffmpeg-2.0/lib/$(ARCH_ABI)/libswscale.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE	:= swresample
LOCAL_SRC_FILES	:= ../ffmpeg-2.0/lib/$(ARCH_ABI)/libswresample.a
include $(PREBUILT_STATIC_LIBRARY)


#
# ffmpeg.so
#
include $(CLEAR_VARS)

LOCAL_WHOLE_STATIC_LIBRARIES := avutil avcodec avformat swscale swresample
LOCAL_SHARED_LIBRARIES := lenthevcdec

LOCAL_MODULE := ffmpeg

include $(BUILD_SHARED_LIBRARY)
