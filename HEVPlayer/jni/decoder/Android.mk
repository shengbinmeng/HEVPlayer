LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm
LOCAL_ARM_NEON = true

LENT_HEVC_DEC_SRC += utils/log.c
LENT_HEVC_DEC_SRC += utils/math.c
LENT_HEVC_DEC_SRC += utils/mem.c
LENT_HEVC_DEC_SRC += decoder/codec.c
LENT_HEVC_DEC_SRC += decoder/cabac.c
LENT_HEVC_DEC_SRC += decoder/dsp.c
LENT_HEVC_DEC_SRC += decoder/hevc.c
LENT_HEVC_DEC_SRC += decoder/hevc_compatibility.c
LENT_HEVC_DEC_SRC += decoder/hevc_mvpred.c
LENT_HEVC_DEC_SRC += decoder/hevc_refs.c
#LENT_HEVC_DEC_SRC += decoder/hevc_alf.c
LENT_HEVC_DEC_SRC += decoder/hevc_deblock.c
LENT_HEVC_DEC_SRC += decoder/hevc_pred.c
LENT_HEVC_DEC_SRC += decoder/hevc_sao.c
LENT_HEVC_DEC_SRC += decoder/hevc_cabac.c
LENT_HEVC_DEC_SRC += decoder/hevc_frame.c
LENT_HEVC_DEC_SRC += decoder/hevc_ref.c
LENT_HEVC_DEC_SRC += decoder/hevc_thread.c
#LENT_HEVC_DEC_SRC += decoder/misc/decode_tree.c

LOCAL_MODULE    := lentoid-dec
LOCAL_SRC_FILES := $(LENT_HEVC_DEC_SRC)

include $(BUILD_STATIC_LIBRARY)
