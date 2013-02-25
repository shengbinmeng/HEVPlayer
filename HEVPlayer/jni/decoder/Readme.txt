two build methods: 
1. GNU make (using the Makefile)
2. Android ndk-build (using the Android.mk)


######################
Change Log:
utils/attributes.h:16
_inline -> __inline

decoder/codec.h:85
__int64 -> int64_t

decoder/codec.c:65
__int64 -> int64_t

decoder/hevc_thread.c:12
"pthread\Pre-built.2\include\pthread.h" -> "pthread/Pre-built.2/include/pthread.h" -> <pthread.h>

interface/decoder.h:24
__int64 -> int64_t

interface/decoder.h:8
(add) -> #include <inttypes.h>

interface/decoder.cpp:66
__int64 -> int64_t
