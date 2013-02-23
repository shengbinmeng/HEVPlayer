#include <android/log.h>
#include <android/bitmap.h>
#include <stdio.h>
#include <time.h>
#include "jniUtils.h"
#include "yuv2rgb565.h"
#include "interface/decoder.h"
extern "C"{
#include "utils/utils.h"
}
#include <pthread.h>

#define TAG "jniplayer"
#define HAVE_NEON 1

struct fields_t {
    jmethodID	drawFrame;
};
struct VideoFrame
{
	int width;
	int height;
	int linesize_y;
	int linesize_uv;
	double pts;
	uint8_t **yuv_data;
};
struct MediaInfo
{
	int width;
	int height;
	char data_src[1024];
};

static fields_t fields;

static JNIEnv *gEnv = NULL;
static JNIEnv *gEnvLocal = NULL;

static jclass gClass = NULL;
static VideoFrame *gVF = NULL;
static MediaInfo media;
static VideoFrame frame;

pthread_t decode_thread;


static const char* const kClassPathName = "pku/shengbin/hevplayer/NativeMediaPlayer";


typedef unsigned char PEL;

double getms()
{
	struct timeval pTime;
	gettimeofday(&pTime, NULL);
	double t2 = ((double)pTime.tv_usec / 1000.0);
	return t2;
}

void *align_malloc( int i_size )
{
    return lent_malloc(i_size);
}

void align_free( void *p )
{
    if(p)
		lent_free(p);
}



#define MAX_AU 10000

enum nal_unit_type_e
{
	NAL_UNIT_CODED_SLICE_TRAIL_N = 0,   // 0
	NAL_UNIT_CODED_SLICE_TRAIL_R,   // 1

	NAL_UNIT_CODED_SLICE_TSA_N,     // 2
	NAL_UNIT_CODED_SLICE_TLA,       // 3   // Current name in the spec: TSA_R

	NAL_UNIT_CODED_SLICE_STSA_N,    // 4
	NAL_UNIT_CODED_SLICE_STSA_R,    // 5

	NAL_UNIT_CODED_SLICE_RADL_N,    // 6
	NAL_UNIT_CODED_SLICE_DLP,       // 7 // Current name in the spec: RADL_R

	NAL_UNIT_CODED_SLICE_RASL_N,    // 8
	NAL_UNIT_CODED_SLICE_TFD,       // 9 // Current name in the spec: RASL_R

	NAL_UNIT_RESERVED_10,
	NAL_UNIT_RESERVED_11,
	NAL_UNIT_RESERVED_12,
	NAL_UNIT_RESERVED_13,
	NAL_UNIT_RESERVED_14,
	NAL_UNIT_RESERVED_15,

	NAL_UNIT_CODED_SLICE_BLA,       // 16   // Current name in the spec: BLA_W_LP
	NAL_UNIT_CODED_SLICE_BLANT,     // 17   // Current name in the spec: BLA_W_DLP
	NAL_UNIT_CODED_SLICE_BLA_N_LP,  // 18
	NAL_UNIT_CODED_SLICE_IDR,       // 19  // Current name in the spec: IDR_W_DLP
	NAL_UNIT_CODED_SLICE_IDR_N_LP,  // 20
	NAL_UNIT_CODED_SLICE_CRA,       // 21
	NAL_UNIT_RESERVED_22,
	NAL_UNIT_RESERVED_23,

	NAL_UNIT_RESERVED_24,
	NAL_UNIT_RESERVED_25,
	NAL_UNIT_RESERVED_26,
	NAL_UNIT_RESERVED_27,
	NAL_UNIT_RESERVED_28,
	NAL_UNIT_RESERVED_29,
	NAL_UNIT_RESERVED_30,
	NAL_UNIT_RESERVED_31,

	NAL_UNIT_VPS,                   // 32
	NAL_UNIT_SPS,                   // 33
	NAL_UNIT_PPS,                   // 34
	NAL_UNIT_ACCESS_UNIT_DELIMITER, // 35
	NAL_UNIT_EOS,                   // 36
	NAL_UNIT_EOB,                   // 37
	NAL_UNIT_FILLER_DATA,           // 38
	NAL_UNIT_SEI,                   // 39 Prefix SEI
	NAL_UNIT_SEI_SUFFIX,            // 40 Suffix SEI

	NAL_UNIT_RESERVED_41,
	NAL_UNIT_RESERVED_42,
	NAL_UNIT_RESERVED_43,
	NAL_UNIT_RESERVED_44,
	NAL_UNIT_RESERVED_45,
	NAL_UNIT_RESERVED_46,
	NAL_UNIT_RESERVED_47,
	NAL_UNIT_UNSPECIFIED_48,
	NAL_UNIT_UNSPECIFIED_49,
	NAL_UNIT_UNSPECIFIED_50,
	NAL_UNIT_UNSPECIFIED_51,
	NAL_UNIT_UNSPECIFIED_52,
	NAL_UNIT_UNSPECIFIED_53,
	NAL_UNIT_UNSPECIFIED_54,
	NAL_UNIT_UNSPECIFIED_55,
	NAL_UNIT_UNSPECIFIED_56,
	NAL_UNIT_UNSPECIFIED_57,
	NAL_UNIT_UNSPECIFIED_58,
	NAL_UNIT_UNSPECIFIED_59,
	NAL_UNIT_UNSPECIFIED_60,
	NAL_UNIT_UNSPECIFIED_61,
	NAL_UNIT_UNSPECIFIED_62,
	NAL_UNIT_UNSPECIFIED_63,
	NAL_UNIT_INVALID,
};

unsigned int AUStart[MAX_AU];

int findAU(PEL *buffer,int start, int maxlen)
{
	int k=start;
	static int tag=0;
	while(1){
		if((buffer[k]==0&&buffer[k+1]==0&&buffer[k+2]==1&&(((buffer[k+3]&0x7F)>>1)<=21)))
		{
				break;
		}
		k++;
		if(k+1>maxlen)
			return maxlen;
	}
	return k;
}
void findAUs(PEL *buffer,int maxlen)
{
	int i=0,j=0;
	while(j<maxlen)
	{
		AUStart[i++]=j=findAU(buffer,j,maxlen);
		j++;
	}
	AUStart[0]=0;
	//AUStart[1]=maxlen;
}

void outputFrame(PEL *buffer[3], int frame_size, int width, int stride[3], FILE *file)
{
	int i, height = frame_size * 2 / 3 / width;
	PEL *out = buffer[0];
	for(i = 0; i < height; i ++)
	{
		fwrite(out,1,width,file);
		out += stride[0];
	}
	width >>= 1;
	out = buffer[1];
	for(i = 0; i < height; i += 2)
	{
		fwrite(out,1,width,file);
		out += stride[1];
	}
	out = buffer[2];
	for(i = 0; i < height; i += 2)
	{
		fwrite(out,1,width,file);
		out += stride[2];
	}
}


int drawFrame(VideoFrame* vf)
{
    gVF = vf;
	if (gEnvLocal == NULL) gEnvLocal = getJNIEnv();
    return gEnvLocal->CallStaticIntMethod(gClass, fields.drawFrame, media.width, media.height);
}

static int
MediaPlayer_setDataSource(JNIEnv *env, jobject thiz, jstring path)
{
    const char *pathStr = env->GetStringUTFChars(path, NULL);
    strcpy(media.data_src, pathStr);
    // Make sure that local ref is released before a potential exception
    env->ReleaseStringUTFChars(path, pathStr);

    return 0;
}

static int
MediaPlayer_prepare(JNIEnv *env, jobject thiz, jint threadNumber)
{
	media.height = 720;
	media.width = 1280;
	frame.height = media.height;
	frame.width = media.width;

	return 0;
}


void* decode(void *p)
{
	DecodeCore decoder;
	long framesGot=0,bytesUsed;
	decoder.Set_Thread(4);

	decoder.StartDecoder(91);


	PEL *bitstream;
	int remainLen;//
	bitstream=(PEL*)align_malloc(1024*1024*100);
	memset(bitstream,0,1024*1024*100);
	FILE *in;
	in=fopen(media.data_src,"rb");
	if(!in) {
		lent_log(NULL, LENT_LOG_ERROR, "can not open input file '%s'!\n", media.data_src);
		return NULL;
	}
	remainLen=fread(bitstream,1,1024*1024*100,in);
	fclose(in);
	findAUs(bitstream,remainLen);

	lent_log(NULL, LENT_LOG_DEBUG, "input file opened\n");
    __android_log_print(ANDROID_LOG_INFO, TAG, "input file opened: %s\n", media.data_src);


	#ifdef OUTPUTYUV
		FILE *fout = NULL;
		char out_file[1024];
		strcpy(out_file, media.data_src);
		strcat(out_file, ".yuv");
		fout = fopen(out_file, "wb");
		if ( NULL == fout ) {
			lent_log(NULL, LENT_LOG_ERROR, "can not create output file '%s'!\n", out_file);
		    __android_log_print(ANDROID_LOG_ERROR, TAG, "can not create output file '%s'!\n", out_file);
			return NULL;
		}
	#endif

	int count=0,i=0;
	int tStart=clock();
	PEL *OutputYUV[3];
	int stride[3], width = 0;


	while(AUStart[i+1])
	{
		__android_log_print(ANDROID_LOG_DEBUG, TAG, "before decode a frame: %.3f", getms());

		bytesUsed=AUStart[i+1]-AUStart[i];
		decoder.DecodeFrame(bitstream+AUStart[i],OutputYUV,&bytesUsed,NULL,&width,stride);
	    __android_log_print(ANDROID_LOG_INFO, TAG, "DecodeFrame returned\n");

		if(bytesUsed)
		{
			lent_dlog(NULL,"decoded a picture: %d\n",count);
		    __android_log_print(ANDROID_LOG_INFO, TAG, "decoded a picture: %d\n",count);

			count++;

			// draw frame to screen
			frame.yuv_data = OutputYUV;
			frame.linesize_y = stride[0];
			frame.linesize_uv = stride[1];
			drawFrame(&frame);

#ifdef OUTPUTYUV
			//if(count>100)
			if ( NULL != fout )
			{
				//fwrite(buffer,bytesUsed,1,fout);
				outputFrame(OutputYUV,bytesUsed,width,stride,fout);
			}
#endif
			__android_log_print(ANDROID_LOG_DEBUG, TAG, "after render this frame: %.3f", getms());

		}
		else
		{
			//printf("decoded no pic!\n");
		    __android_log_print(ANDROID_LOG_INFO, TAG, "decoded no picture\n");
		}

		if(bytesUsed<0)
		{
			//printf("decode error\n");
			break;
		}
		i++;
		//if(i>=49)
		//	break;
	}

	do{
		bytesUsed=0;
		decoder.DecodeFrame(0,OutputYUV,&bytesUsed,0,&width,stride);
		if(bytesUsed)
		{
			lent_dlog(NULL,"decoded a picture: %d\n",count);
			count++;
#ifdef OUTPUTYUV
			//if(count>100)
			if ( NULL != fout )
			{
				//fwrite(buffer,bytesUsed,1,fout);
				outputFrame(OutputYUV,bytesUsed,width,stride,fout);
			}
#endif
		}
		if(bytesUsed<0)
		{
			//printf("decode error\n");
			break;
		}
	}while(bytesUsed);

	lent_log(NULL,LENT_LOG_DEBUG,"Decoding time: %d ms\nSpeed: %d FPS.\n",clock()-tStart,count*1000/(clock()-tStart));
	#ifdef OUTPUTYUV
		if ( NULL != fout )
			fclose(fout);
	#endif
	align_free(bitstream);
	//align_free(buffer);
	//getchar();
	decoder.UninitDecoder();

	detachJVM();
}

static int
MediaPlayer_start(JNIEnv *env, jobject thiz)
{

	__android_log_print(ANDROID_LOG_INFO, TAG, "start decoding thread");
	pthread_create(&decode_thread, NULL, decode, NULL);
}

static int
MediaPlayer_pause(JNIEnv *env, jobject thiz)
{
	return 0;
}

static int
MediaPlayer_go(JNIEnv *env, jobject thiz)
{
	return 0;
}


static int
MediaPlayer_stop(JNIEnv *env, jobject thiz)
{
	return 0;
}

static bool
MediaPlayer_isPlaying(JNIEnv *env, jobject thiz)
{
    bool is_playing = true;
    return is_playing;
}

static int
MediaPlayer_seekTo(JNIEnv *env, jobject thiz, jint msec)
{
	return 0;
}

static int
MediaPlayer_getVideoWidth(JNIEnv *env, jobject thiz)
{
    int w = media.width;
    return w;
}

static int
MediaPlayer_getVideoHeight(JNIEnv *env, jobject thiz)
{
    int h = media.height;
    return h;
}


static int
MediaPlayer_getCurrentPosition(JNIEnv *env, jobject thiz)
{
    int msec = 0;
    return msec;
}

static int
MediaPlayer_getDuration(JNIEnv *env, jobject thiz)
{
    int msec = 0;
    return msec;
}



// ----------------------------------------------------------------------------

static void MediaPlayer_native_init(JNIEnv *env)
{
    jclass clazz;
    clazz = env->FindClass("pku/shengbin/hevplayer/NativeMediaPlayer");
    if (clazz == NULL) {
        jniThrowException(env, "java/lang/RuntimeException", "Can't find MediaPlayer");
        return;
    }

	fields.drawFrame = env->GetStaticMethodID(clazz, "drawFrame","(II)I");
	if (fields.drawFrame == NULL) {
		jniThrowException(env, "java/lang/RuntimeException", "Can't find MediaPlayer.drawFrame");
		return;
	}

	gClass = NULL;
	gEnv = NULL;
	gVF = NULL;
}

static void
MediaPlayer_native_setup(JNIEnv *env, jobject thiz, jobject weak_this)
{
	// Hold onto the MediaPlayer class for use in calling the static method
	// that posts events to the application thread.
	jclass clazz = env->GetObjectClass(thiz);
	if (clazz == NULL) {
		jniThrowException(env, "java/lang/Exception", kClassPathName);
		return;
	}
	gClass = (jclass)env->NewGlobalRef(clazz);
	gEnv = env;
}

static void
MediaPlayer_renderBitmap(JNIEnv *env, jobject  obj, jobject bitmap)
{
	void*              pixels;
	int                ret;

	if ((ret = AndroidBitmap_lockPixels(env, bitmap, &pixels)) < 0) {
		__android_log_print(ANDROID_LOG_DEBUG, TAG, "AndroidBitmap_lockPixels() failed ! error=%d", ret);
	}

	// Convert the image from its native format to RGB565
	__android_log_print(ANDROID_LOG_DEBUG, TAG, "before scale: %.3f", getms());
#if HAVE_NEON
	//__android_log_print(ANDROID_LOG_DEBUG, TAG, "scale with neon");
	ConvertYCbCrToRGB565_neon(	gVF->yuv_data[0],
								gVF->yuv_data[1],
								gVF->yuv_data[2],
								(uint8_t*)pixels,
								gVF->width,
								gVF->height,
								gVF->linesize_y,
								gVF->linesize_uv,
								gVF->width * 2,
								420  );
#else
	ConvertYCbCrToRGB565_c(		gVF->yuv_data[0],
								gVF->yuv_data[1],
								gVF->yuv_data[2],
								(uint8_t*)pixels,
								gVF->width,
								gVF->height,
								gVF->linesize_y,
								gVF->linesize_uv,
								gVF->width * 2,
								420  );
#endif
	__android_log_print(ANDROID_LOG_DEBUG, TAG, "after scale: %.3f", getms());

	AndroidBitmap_unlockPixels(env, bitmap);
}

// ----------------------------------------------------------------------------

static JNINativeMethod gMethods[] = {
    {"setDataSource",       "(Ljava/lang/String;)I",            (void *)MediaPlayer_setDataSource},
    {"native_prepare",            "(I)I",                             (void *)MediaPlayer_prepare},
    {"native_start",              "()I",                              (void *)MediaPlayer_start},
    {"native_stop",               "()I",                              (void *)MediaPlayer_stop},
    {"getVideoWidth",       "()I",                              (void *)MediaPlayer_getVideoWidth},
    {"getVideoHeight",      "()I",                              (void *)MediaPlayer_getVideoHeight},
    {"native_seekTo",             "(I)I",                             (void *)MediaPlayer_seekTo},
    {"native_pause",              "()I",                              (void *)MediaPlayer_pause},
    {"native_go",                 "()I",                              (void *)MediaPlayer_go},
    {"isPlaying",           "()Z",                              (void *)MediaPlayer_isPlaying},
    {"getCurrentPosition",  "()I",                              (void *)MediaPlayer_getCurrentPosition},
    {"getDuration",         "()I",                              (void *)MediaPlayer_getDuration},
    {"native_init",         "()V",                              (void *)MediaPlayer_native_init},
    {"native_setup",        "(Ljava/lang/Object;)V",            (void *)MediaPlayer_native_setup},
    {"renderBitmap",        "(Landroid/graphics/Bitmap;)V",     (void *)MediaPlayer_renderBitmap},
};

int register_jniplayer(JNIEnv *env) {
	return jniRegisterNativeMethods(env, kClassPathName, gMethods, sizeof(gMethods) / sizeof(gMethods[0]));
}
