#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include "mediaplayer.h"
#include "jni_utils.h"
#include "gl_renderer.h"

extern "C" {
#include "lenthevcdec.h"
#include "libavutil/imgutils.h"
#include "libavformat/avformat.h"
}

#define LOG_TAG    "mediaplayer"

#define ENABLE_LOGD 0
#if ENABLE_LOGD
#define LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
#else
#define LOGD(...)
#endif
#define LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

#ifndef _countof
#define _countof(a) (sizeof(a) / sizeof((a)[0]))
#endif

#define LOOP_PLAY 1

struct fields_t {
    jmethodID	drawFrame;
    jmethodID   postEvent;
};

struct MediaInfo
{
	int width;
	int height;
	char data_src[1024];
	int raw_bs;
};


VideoFrame *gVF = NULL;

static fields_t fields;

static JNIEnv *gEnv = NULL;
static JNIEnv *gEnvLocal = NULL;

static jclass gClass = NULL;
static MediaInfo media;

static pthread_t decode_thread;

static const char* const kClassPathName = "pku/shengbin/hevplayer/MediaPlayer";

// for lenthevcdec
static const uint32_t AU_COUNT_MAX = 1024 * 1024;
static const uint32_t AU_BUF_SIZE_MAX = 1024 * 1024 * 50;
static uint32_t au_pos[AU_COUNT_MAX];	// too big array, use static to save stack space
static uint32_t au_count, au_buf_size;
static uint8_t *au_buf = NULL;
static lenthevcdec_ctx lent_ctx = NULL;

// for ffmpeg
static AVFormatContext *ff_fmt_ctx = NULL;
static AVCodecContext *ff_vid_dec_ctx = NULL;
static int ff_vid_strm_idx = -1;
static uint8_t *ff_vid_dst_data[4] = {NULL};
static int ff_vid_dst_linesize[4];
static AVFrame *ff_frame = NULL;
static AVPacket ff_pkt;
static volatile int exit_decode_thread = 0;
static volatile int is_playing = 0;
static int ff_threads = 1;

static int frames_sum = 0;
static double tstart = 0;

static int frames = 0;
static double tlast = 0;

static float renderFPS = 0;
static uint64_t renderInterval = 0;
static struct timeval timeStart;

uint32_t getms()
{
	struct timeval t;
	gettimeofday(&t, NULL);
	return (t.tv_sec * 1000) + (t.tv_usec / 1000);
}

void postEvent(int msg, int ext1, int ext2)
{
	JNIEnv *env = getJNIEnv();
    env->CallStaticVoidMethod(gClass, fields.postEvent, msg, ext1, ext2, 0);
}

int drawFrame(VideoFrame* vf)
{
	LOGD("enter drawFrame:%u (%f)", getms(), vf->pts);
	struct timeval timeNow;
	gettimeofday(&timeNow, NULL);
	int64_t timePassed = ((int64_t)(timeNow.tv_sec - timeStart.tv_sec))*1000000 + (timeNow.tv_usec - timeStart.tv_usec);
	int64_t delay = vf->pts - timePassed;
	if (delay > 0) {
		usleep(delay);
	}

	gettimeofday(&timeNow, NULL);
	double tnow = timeNow.tv_sec + (timeNow.tv_usec / 1000000.0);
	if (tlast == 0) tlast = tnow;
	if (tstart == 0) tstart = tnow;
	if (tnow > tlast + 1) {
		double avg_fps;

		LOGI("Video Display FPS:%i", (int)frames);
		frames_sum += frames;
		avg_fps = frames_sum / (tnow - tstart);
		LOGI("Video AVG FPS:%.2lf", avg_fps);
		postEvent(900, int(frames), int(avg_fps * 4096));
		tlast = tlast + 1;
		frames = 0;
	}
	frames++;

    gVF = vf;
	if (gEnvLocal == NULL) gEnvLocal = getJNIEnv();
	LOGD("before request draw:%u (%f)", getms(), vf->pts);
    return gEnvLocal->CallStaticIntMethod(gClass, fields.drawFrame, vf->width, vf->height);
}

int lent_hevc_get_sps(uint8_t* buf, int size, uint8_t** sps_ptr)
{
	int i, nal_type, sps_pos;
	sps_pos = -1;
	for ( i = 0; i < (size - 4); i++ ) {
		if ( 0 == buf[i] && 0 == buf[i+1] && 1 == buf[i+2] ) {
			nal_type = (buf[i+3] & 0x7E) >> 1;
			if ( 33 != nal_type && sps_pos >= 0 ) {
				break;
			}
			if ( 33 == nal_type ) { // sps
				sps_pos = i;
			}
			i += 2;
		}
	}
	if ( sps_pos < 0 )
		return 0;
	if ( i == (size - 4) )
		i = size;
	*sps_ptr = buf + sps_pos;
	return i - sps_pos;
}

int lent_hevc_get_frame(uint8_t* buf, int size, int *is_idr)
{
	static int seq_hdr = 0;
	int i, nal_type, idr = 0;
	for ( i = 0; i < (size - 6); i++ ) {
		if ( 0 == buf[i] && 0 == buf[i+1] && 1 == buf[i+2] ) {
			nal_type = (buf[i+3] & 0x7E) >> 1;
			if ( nal_type <= 21 ) {
				if ( buf[i+5] & 0x80 ) { /* first slice in pic */
					if ( !seq_hdr )
						break;
					else
						seq_hdr = 0;
				}
			}
			if ( nal_type >= 32 && nal_type <= 34 ) {
				if ( !seq_hdr ) {
					seq_hdr = 1;
					idr = 1;
					break;
				}
				seq_hdr = 1;
			}
			i += 2;
		}
	}
	if ( i == (size - 6) )
		i = size;
	if ( NULL != is_idr )
		*is_idr = idr;
	return i;
}

void* rawbs_runDecoder(void *p)
{
	int32_t got_frame, width, height, stride[3];
	uint8_t* pixels[3];
	int64_t pts, got_pts;
	int frame_count, ret, i;

	if ( NULL == lent_ctx || NULL == au_buf )
		return NULL;

decode:
	// decode all AUs
	frame_count = 0;
	for ( i = 0; i < au_count && !exit_decode_thread; i++ ) {
		pts = i * 40;
		got_frame = 0;
		uint32_t start_time = getms();
		LOGD("before decode: %u", start_time);
		ret = lenthevcdec_decode_frame(lent_ctx, au_buf + au_pos[i], au_pos[i + 1] - au_pos[i], pts,
					       &got_frame, &width, &height, stride, (void**)pixels, &got_pts);
		if ( ret < 0 ) {
			LOGE("call lenthevcdec_decode_frame failed! ret = %d\n", ret);
			goto exit;
		}
		uint32_t end_time = getms();
		LOGD("after decode: %u", end_time);
		uint32_t dec_time = end_time - start_time;
		if ( got_frame > 0 ) {
			LOGD("decoding time: %u - %u = %u\n", end_time, start_time, dec_time);
			LOGD("decode frame: pts = %" PRId64 ", linesize = {%d,%d,%d}\n", got_pts, stride[0], stride[1], stride[2]);
			if ( media.width != width || media.height != height ) {
				LOGD("Video dimensions change! %dx%d -> %dx%d\n", media.width, media.height, width, height);
				media.width = width;
				media.height = height;
			}
			// draw frame to screen
			VideoFrame vf;
			vf.width = width;
			vf.height = height;
			vf.linesize_y = stride[0];
			vf.linesize_uv = stride[1];
			vf.pts = renderInterval * frame_count;
			vf.yuv_data[0] = pixels[0];
			vf.yuv_data[1] = pixels[1];
			vf.yuv_data[2] = pixels[2];

			if (frame_count == 0) {
				gettimeofday(&timeStart, NULL);
			}
			drawFrame(&vf);
			frame_count++;
		}
	}

#if LOOP_PLAY
	if (!exit_decode_thread) {
		LOGI("automatically play again\n");
		goto decode;
	}
#endif

	// flush decoder
	while ( !exit_decode_thread ) {
		got_frame = 0;
		ret = lenthevcdec_decode_frame(lent_ctx, NULL, 0, pts,
					       &got_frame, &width, &height, stride, (void**)pixels, &got_pts);
		if ( ret < 0 || got_frame <= 0)
			break;
		if ( got_frame > 0 ) {
			if ( media.width != width || media.height != height ) {
				LOGD("Video dimensions change! %dx%d -> %dx%d\n", media.width, media.height, width, height);
				media.width = width;
				media.height = height;
			}
			// draw frame to screen
			VideoFrame vf;
			vf.width = width;
			vf.height = height;
			vf.linesize_y = stride[0];
			vf.linesize_uv = stride[1];
			vf.pts = renderInterval * frame_count;
			vf.yuv_data[0] = pixels[0];
			vf.yuv_data[1] = pixels[1];
			vf.yuv_data[2] = pixels[2];
			drawFrame(&vf);
			frame_count++;
		}
	}

exit:
	if ( NULL != au_buf )
		free(au_buf);
	au_buf = 0;
	if ( NULL != lent_ctx )
		lenthevcdec_destroy(lent_ctx);
	lent_ctx = NULL;
	postEvent(909, int(frame_count), 0); // end of file
	detachJVM();
	is_playing = 0;
	LOGI("decode thread exit\n");
	exit_decode_thread = 0;

	return NULL;
}

void* ffmpeg_runDecoder(void *p)
{
	int end_of_file, got_frame, ret, total_frames;
ffmpeg_decode:
	total_frames = 0;
	end_of_file = 0;
	is_playing = 1;
	while ( !exit_decode_thread && (!end_of_file || (end_of_file && !got_frame)) ) {
		if ( !end_of_file ) {
			ret = av_read_frame(ff_fmt_ctx, &ff_pkt);
			if ( ret < 0 ) {
				// end of file
#if LOOP_PLAY
				// do not flush decoder
				break;
#endif
				ff_pkt.data = NULL; // flush decoder
				ff_pkt.size = 0;
				end_of_file = 1;
			} else {
				if ( ff_pkt.stream_index != ff_vid_strm_idx )
					continue;
				//__android_log_print(ANDROID_LOG_VERBOSE, TAG, "read frame: size = %d\n", ff_pkt.size);
			}
		}
		// ff_vid_dec_ctx->skip_frame = AVDISCARD_NONREF;
		ret = avcodec_decode_video2(ff_vid_dec_ctx, ff_frame, &got_frame, &ff_pkt);
		if ( ret < 0 ) {
			LOGE("call avcodec_decode_video2() failed !\n");
			break;
		}
		if ( got_frame ) {
			LOGD("decode frame: pts = %" PRId64 "\n", ff_frame->pts);
			// draw frame to screen
			VideoFrame vf;
			vf.width = ff_vid_dec_ctx->width;
			vf.height = ff_vid_dec_ctx->height;
			vf.linesize_y = ff_frame->linesize[0];
			vf.linesize_uv = ff_frame->linesize[1];
			vf.pts = renderInterval * total_frames;
			vf.yuv_data[0] = ff_frame->data[0];
			vf.yuv_data[1] = ff_frame->data[1];
			vf.yuv_data[2] = ff_frame->data[2];

			if (total_frames == 0) {
				gettimeofday(&timeStart, NULL);
			}
			drawFrame(&vf);
			total_frames++;
		}
		av_free_packet(&ff_pkt);
	}

#if LOOP_PLAY
	if (!exit_decode_thread) {
		if (avformat_seek_file(ff_fmt_ctx, ff_vid_strm_idx, LONG_LONG_MIN, 1, LONG_LONG_MAX, AVSEEK_FLAG_FRAME) < 0) {
			LOGD("avformat_seek_file error, will try av_seek_frame\n");
			if (av_seek_frame(ff_fmt_ctx, ff_vid_strm_idx, 0, AVSEEK_FLAG_BACKWARD) < 0) {
				LOGE("av_seek_frame error, can not auto-replay\n");
			} else {
				LOGI("automatically play again\n");
				goto ffmpeg_decode;
			}
		} else {
			LOGI("automatically play again\n");
			goto ffmpeg_decode;
		}
	}
#endif

	if ( NULL != ff_vid_dec_ctx )
		avcodec_close(ff_vid_dec_ctx);
	avformat_close_input(&ff_fmt_ctx);
	av_free(ff_frame);
	ff_fmt_ctx = NULL;
	ff_vid_dec_ctx = NULL;
	ff_vid_strm_idx = -1;
	ff_vid_dst_data[0] = NULL;
	ff_frame = NULL;
	ff_pkt.data = NULL;
	postEvent(909, int(total_frames), 0); // end of file
	detachJVM();
	is_playing = 0;
	LOGI("decode thread exit\n");
	exit_decode_thread = 0;

	return NULL;
}


static int ff_open_codec_context(int *stream_idx,
				 AVFormatContext *fmt_ctx, enum AVMediaType type)
{
    int ret;
    AVStream *st;
    AVCodecContext *dec_ctx = NULL;
    AVCodec *dec = NULL;

    ret = av_find_best_stream(fmt_ctx, type, -1, -1, NULL, 0);
    if (ret < 0) {
        return ret;
    } else {
        *stream_idx = ret;
        st = fmt_ctx->streams[*stream_idx];

        /* find decoder for the stream */
        dec_ctx = st->codec;
        dec = avcodec_find_decoder(dec_ctx->codec_id);
        if (!dec) {
            fprintf(stderr, "Failed to find %s codec\n",
                    av_get_media_type_string(type));
            return ret;
        }

        dec_ctx->thread_count = ff_threads;
        if ((ret = avcodec_open2(dec_ctx, dec, NULL)) < 0) {
            return ret;
        }
    }

    return 0;
}


static int
MediaPlayer_setDataSource(JNIEnv *env, jobject thiz, jstring path)
{
	const char *pathStr = env->GetStringUTFChars(path, NULL);
	memset(&media, 0, sizeof(media));
	strcpy(media.data_src, pathStr);
	// Make sure that local ref is released before a potential exception
	env->ReleaseStringUTFChars(path, pathStr);
	// is raw HEVC bitstream file ?
	static const char * hevc_raw_bs_ext[] = {".hm91", ".hm10", ".hevc", ".bit"};
	char * ext = strrchr(media.data_src, '.');
	if ( NULL != ext ) {
		int i;
		for ( i = 0; i < _countof(hevc_raw_bs_ext); i++ ) {
			if ( strcasecmp(hevc_raw_bs_ext[i], ext) == 0 )
				break;
		}
		if ( i < _countof(hevc_raw_bs_ext) )
			media.raw_bs = 1;
	}
	return 0;
}

static int rawbs_prepare(int threads)
{
	FILE *in_file;
	int32_t got_frame, width, height, stride[3];
	uint8_t* pixels[3];
	int64_t pts, got_pts;
	uint8_t *sps;
	lenthevcdec_ctx one_thread_ctx;
	int compatibility, frame_count, sps_len, ret, i;

	in_file = NULL;
	au_buf = NULL;
	lent_ctx = NULL;
	one_thread_ctx = NULL;

	// get compatibility version
	compatibility = 0x7fffffff;
	if ( strncasecmp(".hm91", media.data_src + (strlen(media.data_src) - 5), 5) == 0 )
		compatibility = 91;
	else if ( strncasecmp(".hm10", media.data_src + (strlen(media.data_src) - 5), 5) == 0 )
		compatibility = 100;

	// read file
	in_file = fopen(media.data_src, "rb");
	if ( NULL == in_file ) {
		LOGE("Can not open input file '%s'\n", media.data_src);
		goto error_exit;
	}
	fseek(in_file, 0, SEEK_END);
	au_buf_size = ftell(in_file);
	fseek(in_file, 0, SEEK_SET);
	LOGD("file size is %d bytes\n", au_buf_size);
	if ( au_buf_size > AU_BUF_SIZE_MAX )
		au_buf_size = AU_BUF_SIZE_MAX;
	au_buf = (uint8_t*)malloc(au_buf_size);
	if ( NULL == au_buf ) {
		LOGE("call malloc failed! size is %d\n", au_buf_size);
		goto error_exit;
	}
	if ( fread(au_buf, 1, au_buf_size, in_file) != au_buf_size ) {
		LOGE("call fread failed!\n");
		goto error_exit;
	}
	fclose(in_file);
	in_file = NULL;
	LOGD("%d bytes read to address %p\n", au_buf_size, au_buf);

	// find all AU
	au_count = 0;
	for ( i = 0; i < au_buf_size && au_count < (AU_COUNT_MAX - 1); i+=3 ) {
		i += lent_hevc_get_frame(au_buf + i, au_buf_size - i, NULL);
		if (i < au_buf_size) {
			au_pos[au_count++] = i;
		}
		LOGD("AU[%d] = %d\n", au_count - 1, au_pos[au_count - 1]);
	}
	au_pos[au_count] = au_buf_size; // include last AU
	LOGD("found %d AUs\n", au_count);

	// open lentoid HEVC decoder
	LOGI("create lentoid decoder: compatibility = %d, threads = %d\n", compatibility, threads);
	lent_ctx = lenthevcdec_create(threads, compatibility, NULL);
	if ( NULL == lent_ctx ) {
		LOGE("call lenthevcdec_create failed!\n");
		goto error_exit;
	}
	LOGD("get decoder %p\n", lent_ctx);

	// find sps, decode it and get video resolution
	sps_len = lent_hevc_get_sps(au_buf, au_buf_size, &sps);
	if ( sps_len > 0 ) {
		// get a one-thread decoder to decode SPS
		one_thread_ctx = lenthevcdec_create(1, compatibility, NULL);
		if ( NULL == lent_ctx )
			goto error_exit;
		width = 0;
		height = 0;
		ret = lenthevcdec_decode_frame(one_thread_ctx, sps, sps_len, 0, &got_frame, &width, &height, stride, (void**)pixels, &pts);
		if ( 0 != width && 0 != height ) {
			media.width = width;
			media.height = height;
			LOGD("Video dimensions is %dx%d\n", width, height);
		}
		lenthevcdec_destroy(one_thread_ctx);
		one_thread_ctx = NULL;
	}
	return 0;

error_exit:
	if ( NULL != in_file )
		fclose(in_file);
	in_file = NULL;
	if ( NULL != au_buf )
		free(au_buf);
	au_buf = NULL;
	if ( NULL != lent_ctx )
		lenthevcdec_destroy(lent_ctx);
	lent_ctx = NULL;
	if ( NULL != one_thread_ctx )
		lenthevcdec_destroy(one_thread_ctx);
	one_thread_ctx = NULL;

	return -1;
}

static int ffmpeg_prepare(int threadNumber)
{
	// save decode thread number
	ff_threads = threadNumber;

	// init ffmepg
	av_register_all();
	// open input file
	if ( avformat_open_input(&ff_fmt_ctx, media.data_src, NULL, NULL) < 0 ) {
		LOGE("call avformat_open_intput() failed!\n");
		return -1;
	}
	// retrieve stream information
	if ( avformat_find_stream_info(ff_fmt_ctx, NULL) < 0 ) {
		LOGE("call avformat_find_stream_info() failed!\n");
		return -2;
	}
	if ( ff_open_codec_context(&ff_vid_strm_idx, ff_fmt_ctx, AVMEDIA_TYPE_VIDEO) < 0 ) {
		LOGE("Can not find video stream in the input file!\n");
		return -3;
	}
	AVStream *ff_vid_stream = ff_fmt_ctx->streams[ff_vid_strm_idx];
	ff_vid_dec_ctx = ff_vid_stream->codec;
	if ( av_image_alloc(ff_vid_dst_data, ff_vid_dst_linesize, ff_vid_dec_ctx->width, ff_vid_dec_ctx->height, ff_vid_dec_ctx->pix_fmt, 1) < 0 ) {
		LOGE("call av_image_alloc(,,%d,%d,%d) failed!\n",
				ff_vid_dec_ctx->width, ff_vid_dec_ctx->height, ff_vid_dec_ctx->pix_fmt);
		return -4;
	}
	ff_frame = avcodec_alloc_frame();
	if ( NULL == ff_frame ) {
		LOGE("call avcodec_alloc_frame() failed!\n");
		return -5;
	}
	av_init_packet(&ff_pkt);
	ff_pkt.data = NULL;
	ff_pkt.size = 0;
	
	{
		char codec_name[256];
		avcodec_string(codec_name, sizeof(codec_name), ff_vid_dec_ctx, 0);
		LOGD("vid_strm(%d): id = %d, codec = %d(%s)\n"
				"\t width = %d, height = %d\n",
				ff_vid_strm_idx, ff_vid_stream->id, ff_vid_dec_ctx->codec_id, codec_name,
				ff_vid_dec_ctx->width, ff_vid_dec_ctx->height);
	}

	// save picture size
	media.height = ff_vid_dec_ctx->height;
	media.width = ff_vid_dec_ctx->width;
	return 0;
}

static int
MediaPlayer_prepare(JNIEnv *env, jobject thiz, jint threadNumber, jfloat fps)
{
	renderFPS = fps;
	if (fps == 0) renderInterval = 1;
	else {
		renderInterval = 1.0 / fps * 1000000; // us
	}

	if ( media.raw_bs ) {
		return rawbs_prepare(threadNumber);
	} else {
		return ffmpeg_prepare(threadNumber);
	}
}

static int
MediaPlayer_start(JNIEnv *env, jobject thiz)
{
	LOGI("start decoding thread");
	if ( media.raw_bs ) {
		pthread_create(&decode_thread, NULL, rawbs_runDecoder, NULL);
	} else {
		pthread_create(&decode_thread, NULL, ffmpeg_runDecoder, NULL);
	}

	return 0;
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
	void* result;
	exit_decode_thread = 1;
	pthread_join(decode_thread, &result);
	exit_decode_thread = 0;
	LOGI("media player stopped\n");
	return 0;
}

static bool
MediaPlayer_isPlaying(JNIEnv *env, jobject thiz)
{
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
    clazz = env->FindClass(kClassPathName);
    if (clazz == NULL) {
        jniThrowException(env, "java/lang/RuntimeException", "Can't find MediaPlayer");
        return;
    }

    fields.postEvent = env->GetStaticMethodID(clazz, "postEventFromNative", "(III)V");
	if (fields.postEvent == NULL) {
		jniThrowException(env, "java/lang/RuntimeException", "Can't find MediaPlayer.postEventFromNative");
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
	gEnvLocal = NULL;

	frames_sum = 0;
	tstart = 0;

	frames = 0;
	tlast = 0;

	renderFPS = 0;
	renderInterval = 0;
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


// ----------------------------------------------------------------------------

static JNINativeMethod gMethods[] = {
    {"setDataSource",       "(Ljava/lang/String;)I",            (void *)MediaPlayer_setDataSource},
    {"native_prepare",            "(IF)I",                             (void *)MediaPlayer_prepare},
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
};

int register_player(JNIEnv *env) {
	return jniRegisterNativeMethods(env, kClassPathName, gMethods, sizeof(gMethods) / sizeof(gMethods[0]));
}
