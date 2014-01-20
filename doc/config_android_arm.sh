#!/bin/bash

#
# change these variables according to your system
#
SYSROOT=$ANDROID_NDK/platforms/android-9/arch-arm
TOOLCHAIN=$ANDROID_NDK/toolchains/arm-linux-androideabi-4.8/prebuilt/darwin-x86_64
PREFIX=./android/arm

./configure \
    --prefix=$PREFIX \
    --target-os=linux \
    --arch=arm \
    --cpu=armv7-a \
    --enable-cross-compile \
    --cross-prefix=$TOOLCHAIN/bin/arm-linux-androideabi- \
    --sysroot=$SYSROOT \
    --extra-cflags="-O2 -Ithirdparty/arm -fPIC -march=armv7-a -mfpu=neon -mfloat-abi=softfp" \
    --extra-ldflags="-Lthirdparty/arm " \
    --enable-static \
    --enable-gpl \
    --enable-version3 \
    --enable-nonfree \
    --disable-doc \
    --disable-htmlpages \
    --disable-manpages \
    --disable-podpages \
    --disable-txtpages \
    --enable-ffmpeg \
    --disable-ffplay \
    --disable-ffserver \
    --disable-ffprobe \
    --disable-zlib \
    --disable-bzlib \
    --disable-avdevice \
    --disable-postproc \
    --disable-avresample \
    --disable-encoders \
    --disable-muxers \
    --disable-devices \
    --disable-filters \
    --disable-bsfs \
    --enable-liblenthevcdec \
    --enable-decoder=liblenthevchm91 \
    --enable-decoder=liblenthevchm10 \
    --enable-decoder=liblenthevc \