#!/bin/bash

#
# change these variables according to your system
#
SYSROOT=$ANDROID_NDK_HOME/platforms/android-9/arch-arm
TOOLCHAIN=$ANDROID_NDK_HOME/toolchains/arm-linux-androideabi-4.8/prebuilt/linux-x86
PREFIX=./android/arm
LIBLENTOID=`pwd`/../liblenthevcdec

echo $LIBLENTOID
./configure \
    --prefix=$PREFIX \
    --target-os=linux \
    --arch=arm \
    --cpu=armv7-a \
    --enable-cross-compile \
    --cross-prefix=$TOOLCHAIN/bin/arm-linux-androideabi- \
    --sysroot=$SYSROOT \
    --extra-cflags="-I$LIBLENTOID/arm -O2 -fPIC -march=armv7-a -mfpu=neon -mfloat-abi=softfp" \
    --extra-ldflags="-L$LIBLENTOID/arm " \
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
