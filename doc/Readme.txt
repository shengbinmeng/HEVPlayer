1. Apply *.patch to ffmpeg(v2.4.4) source ("patch -p1 < *.patch").

2. Create a folder named liblenthevcdec. Put lenthevcdec.h and liblenthevcdec.a(or liblenthevcdec.so) to liblenthevcdec/x86 or liblenthevcdec/arm.

3. Put config_android_*.sh into ffmpeg source folder. 

4. Run config_android_*.sh to configure ffmpeg, then "make".

5. "make install" will copy the header files and generated libraries to the folder configured with "--prefix".

Notice: the folder hierarchy should be like this:
	| ffmpeg source codes
		| config_android_*.sh
	| lenthevcdec
		| include
			| lenthevcdec.h
		| lib
			| armeabi-v7a
				| liblenthevcdec.so
			| x86
				| liblenthevcdec.so
	
(by: Shengbin Meng & Yingming Fan, 2015-06-10)
