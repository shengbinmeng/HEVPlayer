#!/bin/bash

#
# change these variables according to your system
#
SYSROOT=$ANDROID_NDK/platforms/android-9/arch-x86
TOOLCHAIN=$ANDROID_NDK/toolchains/x86-4.8/prebuilt/darwin-x86_64
PREFIX=./android/x86

#
# read the configure help carefully before you want to change the following options
#
./configure \
    --prefix=$PREFIX \
    --target-os=linux \
    --arch=x86 \
    --cpu=atom \
    --enable-cross-compile \
    --cross-prefix=$TOOLCHAIN/bin/i686-linux-android- \
    --sysroot=$SYSROOT \
    --extra-cflags="-O2 -Ithirdparty/x86 -msse3 -ffast-math -mfpmath=sse" \
    --extra-ldflags="-Lthirdparty/x86" \
    --disable-amd3dnow \
    --disable-amd3dnowext \
    --disable-mmx \
	--enable-gpl \
    --enable-version3 \
    --enable-nonfree \
    --disable-doc \
    --disable-programs \
    --enable-ffmpeg \
    --disable-avdevice \
    --disable-postproc \
    --disable-devices \
    --disable-filters \
    --disable-bsfs \
    --disable-zlib \
    --disable-bzlib \
    --disable-encoders \
    --disable-muxers \
    --enable-liblenthevcdec \