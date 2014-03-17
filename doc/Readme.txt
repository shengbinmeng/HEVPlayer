1. Apply *.patch to ffmpeg(v2.0) source ("patch -p1 < *.patch")

2. Put config_android_*.sh to ffmpeg source folder. 
	You may need to modify the toolchain path and variables in the shell script according to your system)

3. Put lenthevcdec.h and liblenthevcdec.a(or liblenthevcdec.so) to the thirdparty folder (or the toolchain's sysroot include and lib directories).
	The .a or .so library should be pre-built using the same toolchain as configured in config-android_*.sh

4. Run config_android_*.sh to configure ffmpeg, then "make".

5. "make install" will copy the header files and generated libraries to the folder configured with "--prefix".

(by: Shengbin Meng)
