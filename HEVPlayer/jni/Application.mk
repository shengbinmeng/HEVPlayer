APP_ABI:=armeabi-v7a x86

DEBUG := $(NDK_DEBUG)

ifndef NDK_DEBUG
	DEBUG := 0
endif
ifeq ($(DEBUG),true)
	DEBUG := 1
endif

ifeq ($(DEBUG),1)
	APP_CFLAGS += -O0 -g
	APP_OPTIM := debug
else
	APP_CFLAGS += -O2
	APP_OPTIM := release
endif


APP_PLATFORM := android-9
NDK_TOOLCHAIN_VERSION := 4.8