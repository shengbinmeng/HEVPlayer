LOCAL_PATH := $(call my-dir)

ARCH_ABI := $(TARGET_ARCH_ABI)

#
# FFmpeg prebuilt static libraries
#
include $(CLEAR_VARS)
LOCAL_MODULE	:= avutil_prebuilt
LOCAL_SRC_FILES	:= lib/$(ARCH_ABI)/libavutil.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE	:= avcodec_prebuilt
LOCAL_SRC_FILES	:= lib/$(ARCH_ABI)/libavcodec.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE	:= avformat_prebuilt
LOCAL_SRC_FILES	:= lib/$(ARCH_ABI)/libavformat.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE	:= swscale_prebuilt
LOCAL_SRC_FILES	:= lib/$(ARCH_ABI)/libswscale.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE	:= swresample_prebuilt
LOCAL_SRC_FILES	:= lib/$(ARCH_ABI)/libswresample.a
include $(PREBUILT_STATIC_LIBRARY)


#
# FFmpeg shared library
#
include $(CLEAR_VARS)

LOCAL_MODULE := ffmpeg
LOCAL_WHOLE_STATIC_LIBRARIES := avutil_prebuilt avcodec_prebuilt avformat_prebuilt \
								avfilter_prebuilt swscale_prebuilt swresample_prebuilt
LOCAL_SHARED_LIBRARIES := lenthevcdec

include $(BUILD_SHARED_LIBRARY)