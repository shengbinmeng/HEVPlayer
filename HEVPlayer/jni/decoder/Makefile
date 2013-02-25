# Makefile

#toolchain
TOOLCHAIN_PATH = $(NDK)/toolchains/arm-linux-androideabi-4.6/prebuilt/darwin-x86/bin
CC = $(TOOLCHAIN_PATH)/arm-linux-androideabi-gcc
LD = $(TOOLCHAIN_PATH)/arm-linux-androideabi-ld
AR = $(TOOLCHAIN_PATH)/arm-linux-androideabi-ar

#config
ARCH = ARM
SYSROOT = $(NDK)/platforms/android-9/arch-arm
CFLAGS = -I. --sysroot=$(SYSROOT)

#source files
SRCS =  decoder/cabac.c decoder/codec.c decoder/dsp.c \
		decoder/hevc_cabac.c decoder/hevc_compatibility.c decoder/hevc_deblock.c \
		decoder/hevc_frame.c decoder/hevc_mvpred.c decoder/hevc_pred.c \
		decoder/hevc_ref.c decoder/hevc_refs.c decoder/hevc_sao.c \
		decoder/hevc_thread.c decoder/hevc.c  utils/mem.c utils/log.c utils/math.c
		 
ifeq ($(ARCH), X86)
SRCS += decoder/arch/x86/dsp_x86.c decoder/arch/x86/deblock_x86.c decoder/arch/x86/pred_x86.c
endif
ifeq ($(ARCH), ARM)
SRCS +=
endif

#object files
OBJS += $(SRCS:%.c=%.o)

#targets
default:lib-static

lib-static:$(OBJS)
	$(AR) crv liblentoid-dec.a $(OBJS)

.PHONY: clean
clean:
	rm decoder/*.o utils/*.o *.a