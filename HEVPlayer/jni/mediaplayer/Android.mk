LOCAL_PATH := $(call my-dir)


#
# mediaplayer.so
#
include $(CLEAR_VARS)

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../lenthevcdec/include/ $(LOCAL_PATH)/../ffmpeg-2.0/include/

LOCAL_SRC_FILES := decoder_audio.cpp decoder_video.cpp decoder.cpp framequeue.cpp \
					packetqueue.cpp thread.cpp mediaplayer.cpp jni_utils.cpp gl_renderer.cpp

LOCAL_LDLIBS := -llog -lz -lGLESv2

LOCAL_SHARED_LIBRARIES := ffmpeg lenthevcdec

LOCAL_MODULE := mediaplayer

include $(BUILD_SHARED_LIBRARY)
