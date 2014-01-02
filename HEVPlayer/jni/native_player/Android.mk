LOCAL_PATH := $(call my-dir)


#
# mediaplayer.so
#
include $(CLEAR_VARS)

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../lenthevcdec/include/ $(LOCAL_PATH)/../ffmpeg-2.0/include/ $(LOCAL_PATH)/../mediaplayer/

LOCAL_SRC_FILES := jni_utils.cpp gl_renderer.cpp native_player.cpp mp_listener.cpp

LOCAL_LDLIBS := -llog -lz -lGLESv2

LOCAL_SHARED_LIBRARIES := ffmpeg lenthevcdec
LOCAL_STATIC_LIBRARIES := mediaplayer

LOCAL_MODULE := native_player

include $(BUILD_SHARED_LIBRARY)
