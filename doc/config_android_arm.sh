#!/bin/bash

#
# change these variables according to your system
#
SYSROOT=$ANDROID_NDK/platforms/android-9/arch-arm
TOOLCHAIN=$ANDROID_NDK/toolchains/arm-linux-androideabi-4.8/prebuilt/darwin-x86_64
PREFIX=./android/arm

#
# read the configure help carefully before you want to change the following options
#
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