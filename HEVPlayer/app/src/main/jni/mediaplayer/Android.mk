LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := mediaplayer
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../lenthevcdec/include/ $(LOCAL_PATH)/../ffmpeg/include/ $(LOCAL_PATH)/../native_player/
LOCAL_SRC_FILES := AudioDecoder.cpp VideoDecoder.cpp Decoder.cpp Queue.cpp FrameQueue.cpp PacketQueue.cpp MediaPlayer.cpp PlayerListener.cpp
include $(BUILD_STATIC_LIBRARY)
