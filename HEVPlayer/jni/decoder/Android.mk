LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm
LOCAL_ARM_NEON = true

LOCAL_MODULE := decoder-prebuilt
LOCAL_SRC_FILES :=libdecoder.a
include $(PREBUILT_STATIC_LIBRARY)

