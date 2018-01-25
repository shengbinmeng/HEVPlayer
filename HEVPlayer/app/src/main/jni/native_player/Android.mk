LOCAL_PATH := $(call my-dir)

#
# native_player.so
#
include $(CLEAR_VARS)
LOCAL_MODULE := native_player
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../lenthevcdec/include/ $(LOCAL_PATH)/../ffmpeg/include/ $(LOCAL_PATH)/../mediaplayer/
LOCAL_SRC_FILES := jni_utils.cpp gl_renderer.cpp native_player.cpp
LOCAL_LDLIBS := -llog -lz -lGLESv2 -Wl,--no-warn-shared-textrel
LOCAL_SHARED_LIBRARIES := lenthevcdec
LOCAL_STATIC_LIBRARIES :=  ffmpeg mediaplayer
include $(BUILD_SHARED_LIBRARY)
