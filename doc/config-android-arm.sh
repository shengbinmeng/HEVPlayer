#!/bin/bash

#
# Change these variables according to your system.
#
SYSROOT=$ANDROID_NDK/platforms/android-9/arch-arm
TOOLCHAIN=$ANDROID_NDK/toolchains/arm-linux-androideabi-4.8/prebuilt/darwin-x86_64
PREFIX=./android/armeabi-v7a
LENTHEVCDEC=./thirdparty/lenthevcdec

#
# Read the configure help carefully before you want to change the following options.
#
./configure \
	--prefix=$PREFIX \
	--target-os=linux \
	--arch=arm \
	--cpu=armv7-a \
	--enable-cross-compile \
	--cross-prefix=$TOOLCHAIN/bin/arm-linux-androideabi- \
	--sysroot=$SYSROOT \
	--extra-cflags="-O2 -I$LENTHEVCDEC/include -fPIC -march=armv7-a -mfpu=neon -mfloat-abi=softfp" \
	--extra-ldflags="-L$LENTHEVCDEC/lib/armeabi-v7a" \
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