LOCAL_PATH := $(call my-dir)


#
# jniplayer.so
#
include $(CLEAR_VARS)

ifeq ($(TARGET_ARCH_ABI), armeabi-v7a)
LENT_CFLAGS := -DARCH_ARM=1 -DHAVE_NEON=1
endif
ifeq ($(TARGET_ARCH_ABI), x86)
LENT_CFLAGS := -DARCH_X86_32=1
endif

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../lenthevcdec/include/ $(LOCAL_PATH)/../../../../../../ $(LOCAL_PATH)/../ffmpeg-2.0/include/

LOCAL_SRC_FILES := jniplayer.cpp jniUtils.cpp yuv2rgb565.cpp gl_renderer.cpp

LOCAL_LDLIBS := -llog -lz -ljnigraphics -lGLESv2

LOCAL_CFLAGS += $(LENT_CFLAGS)

LOCAL_SHARED_LIBRARIES := ffmpeg lenthevcdec

LOCAL_MODULE := jniplayer

include $(BUILD_SHARED_LIBRARY)
