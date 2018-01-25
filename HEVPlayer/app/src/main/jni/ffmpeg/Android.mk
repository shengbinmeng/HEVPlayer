LOCAL_PATH := $(call my-dir)

ARCH_ABI := $(TARGET_ARCH_ABI)

#
# libav* prebuilt static libraries
#
include $(CLEAR_VARS)
LOCAL_MODULE	:= avutil
LOCAL_SRC_FILES	:= lib/$(ARCH_ABI)/libavutil.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE	:= avcodec
LOCAL_SRC_FILES	:= lib/$(ARCH_ABI)/libavcodec.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE	:= avformat
LOCAL_SRC_FILES	:= lib/$(ARCH_ABI)/libavformat.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE	:= swscale
LOCAL_SRC_FILES	:= lib/$(ARCH_ABI)/libswscale.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE	:= swresample
LOCAL_SRC_FILES	:= lib/$(ARCH_ABI)/libswresample.a
include $(PREBUILT_STATIC_LIBRARY)


#
# FFmpeg static library
#
include $(CLEAR_VARS)
LOCAL_MODULE := ffmpeg
LOCAL_WHOLE_STATIC_LIBRARIES := avutil avcodec avformat swscale swresample
include $(BUILD_STATIC_LIBRARY)