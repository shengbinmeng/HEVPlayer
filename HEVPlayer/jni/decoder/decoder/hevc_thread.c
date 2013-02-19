//**********************************************
//hevc_thread.c
//Unipipy @2011
//framewise threading
//**********************************************

#include "hevc.h"
#include "hevc_frame.h"
#include "hevc_thread.h"

#if HAVE_PTHREADS
#include <pthread.h>
#endif
#define MAX_BUFFERS 60
/*
#define pthread_mutex_lock() NULL
#define pthread_mutex_unlock() NULL
#define pthread_cond_signal() NULL
#define pthread_cond_wait() NULL
#define pthread_join() NULL
#define pthread_mutex_destroy() NULL
#define pthread_cond_destroy() NULL
#define pthread_cond_broadcast() NULL
#define pthread_mutex_init() NULL
#define pthread_cond_init() NULL
#define pthread_create() NULL
*/
typedef struct PerThreadContext {
	struct FrameThreadContext *parent;

	pthread_t		thread;
	pthread_cond_t	input_cond;      ///< Used to wait for a new packet from the main thread.
	pthread_cond_t	progress_cond;   ///< Used by child threads to wait for progress to change.
	pthread_cond_t	output_cond;     ///< Used by the main thread to wait for frames to finish.

	pthread_mutex_t mutex;          ///< Mutex used to protect the contents of the PerThreadContext.
	pthread_mutex_t progress_mutex; ///< Mutex used to protect frame progress values and progress_cond.
	pthread_mutex_t buffer_mutex;  ///< Mutex used to protect get/release_buffer().

	HEVCContext*	hevc;          ///< Context used to decode packets passed to this thread.

	LentPacket		avpkt;           ///< Input packet (for decoding) or output (for encoding).
	int				allocated_buf_size; ///< Size allocated for avpkt.data

	LentFrame frame;                  ///< Output frame (for decoding) or input (for encoding).
	int     got_frame;              ///< The output of got_picture_ptr from the last avcodec_decode_video() call.
	int     result;                 ///< The result of the last codec decode/encode() call.

	enum {
		STATE_INPUT_READY,          ///< Set when the thread is awaiting a packet.
		STATE_SETTING_UP,           ///< Set before the codec has called lentoid_thread_finish_setup().
		STATE_GET_BUFFER,           /**<
									* Set when the codec calls get_buffer().
									* State is returned to STATE_SETTING_UP afterwards.
									*/
		STATE_SETUP_FINISHED        ///< Set after the codec has called lentoid_thread_finish_setup().
	} state;

	/**
	* Array of frames passed to lentoid_thread_release_buffer().
	* Frames are released after all threads referencing them are finished.
	*/
	LentFrame released_buffers[MAX_BUFFERS];
	int     num_released_buffers;

	/**
	* Array of progress values used by lentoid_thread_get_buffer().
	*/
	int     progress[MAX_BUFFERS][1];
	uint8_t progress_used[MAX_BUFFERS];

	//LentFrame *requested_frame;       ///< AVFrame the codec passed to get_buffer()
} PerThreadContext;

/**
* Context stored in the client AVCodecContext thread_opaque.
*/
typedef struct FrameThreadContext {
	PerThreadContext *threads;     ///< The contexts for each thread.
	PerThreadContext *prev_thread; ///< The last thread submit_packet() was called on.

	int next_decoding;             ///< The next context to submit a packet to.
	int next_finished;             ///< The next context to return output from.

	int delaying;                  /**<
								   * Set for the first N packets, where N is the number of threads.
								   * While it is set, lentoid_thread_en/decode_frame won't return any results.
								   */

	int die;                       ///< Set when threads should exit.
} FrameThreadContext;


static int *allocate_progress(PerThreadContext *p)
{
	int i;

	for (i = 0; i < MAX_BUFFERS; i++)
		if (!p->progress_used[i]) break;

	if (i == MAX_BUFFERS) {
		lent_log(NULL, LENT_LOG_ERROR, "allocate_progress() overflow\n");
		return NULL;
	}

	p->progress_used[i] = 1;

	return p->progress[i];
}


static void free_progress(LentFrame *f)
{
    PerThreadContext *p = f->owner->thread_opaque;
    int *progress = f->thread_opaque;

    p->progress_used[(progress - p->progress[0]) /*/ 2*/] = 0;
}


void lent_thread_picture_release_buffer(HEVCContext *h, LentFrame *pic)//释放data缓冲
{
	LentCodecContext *ctx=h->ctx;


	if (!(ctx->thread))
	{
		h->release_buffer(h,(LentFrame*)pic);
		return;
	}
	else
	{
		PerThreadContext *p = h->thread_opaque;

		if (p->num_released_buffers >= MAX_BUFFERS) {
			lent_log(NULL, LENT_LOG_ERROR, "too many thread_release_buffer calls!\n");
			return;
		}

		//if(avctx->debug & LENTOID_DEBUG_BUFFERS)
		//    av_log(avctx, AV_LOG_DEBUG, "thread_release_buffer called on pic %p, %d buffers used\n",
		//                                f, f->owner->internal_buffer_count);

		p->released_buffers[p->num_released_buffers++] = *((LentFrame*)pic);

		memset(pic->data, 0, sizeof(pic->data));//清空picture结构体中的data缓冲指针（并非释放缓冲）
		pic->motion_val[0]=NULL;
		pic->motion_val[1]=NULL;
		pic->ref_index[0]=NULL;
		pic->ref_index[1]=NULL;
	}

}

int lent_thread_picture_get_buffer(HEVCContext *h, LentFrame *pic)//申请data缓冲
{
	LentCodecContext *ctx=h->ctx;
	int err;

	if (!(ctx->thread))
	{
		err=h->get_buffer(h,(LentFrame*)pic);
		assert(err>=0);
		return err;
	}
	else
	{
		PerThreadContext *p = h->thread_opaque;
		int *progress;

		if (p->state != STATE_SETTING_UP)
		{
			lent_log(NULL, LENT_LOG_ERROR, "get_buffer() cannot be called after setting up\n");
			return -1;
		}

		pthread_mutex_lock(&p->buffer_mutex);

		pic->thread_opaque = progress = allocate_progress(p);

		progress[0] =
		/*progress[1] = */-1;

		if (h->get_buffer == lent_default_get_buffer)
		{
			err = h->get_buffer(h, pic);
		}else {
			assert(0);
		}
		pthread_mutex_unlock(&p->buffer_mutex);

	}
	return err;
}

#define REBASE_PICTURE(pic, new_ctx, old_ctx) (pic ? \
    (pic >= old_ctx->picture && pic < old_ctx->picture+old_ctx->picture_count ?\
        &new_ctx->picture[pic - old_ctx->picture] : pic - (Picture*)old_ctx + (Picture*)new_ctx)\
    : NULL)

static void copy_picture_range(Picture **to, Picture **from, int count, HEVCContext *new_base, HEVCContext *old_base)
{
    int i;

    for (i=0; i<count; i++){
        to[i] = REBASE_PICTURE(from[i], new_base, old_base);
    }
}
static void copy_parameter_set(void **to, void **from, int count, int size)
{
    int i;

    for (i=0; i<count; i++){
        if (to[i] && !from[i]) lent_freep(&to[i]);
        else if (from[i] && !to[i]) to[i] = lent_malloc(size);

        if (from[i]) memcpy(to[i], from[i], size);
    }
}
#define copy_fields(to, from, start_field, end_field) memcpy(&to->start_field, &from->start_field, (char*)&to->end_field - (char*)&to->start_field)
static int lent_update_thread_context(HEVCContext *dst, const HEVCContext *src){

	HEVCContext *h=dst,*h1=(HEVCContext*)src;
	int inited = h->initialized, err;

	if(dst == src)
		return 0;
	
	//VPS/SPS/PPS
	copy_parameter_set((void**)h->vps_buffers, (void**)h1->vps_buffers, MAX_VPS_COUNT, sizeof(VPS));
	h->vps             = h1->vps;
	copy_parameter_set((void**)h->sps_buffers, (void**)h1->sps_buffers, MAX_SPS_COUNT, sizeof(SPS));
	h->sps             = h1->sps;
	copy_parameter_set((void**)h->pps_buffers, (void**)h1->pps_buffers, MAX_PPS_COUNT, sizeof(PPS));
	h->pps             = h1->pps;

	if( !h1->initialized)
		return 0;

	if(!inited){
		if((err=hevc_initialize(h,h1->width,h1->height))<0)
			return err;
		assert(h->machine==h1->machine);
	}

	//SH
	dst->sh.m_prevPOC=src->sh.m_prevPOC;


	//reference lists
	memcpy(h->picture, h1->picture, h1->picture_count * sizeof(Picture));

	h->dpb_count=h1->dpb_count;
	copy_picture_range(h->default_ref_buffer, h1->default_ref_buffer, MAX_DPB_SIZE,h,h1);

	copy_picture_range(h->delayed_pic, h1->delayed_pic, MAX_DELAYED_PIC_COUNT+2, h, h1);
	h->coded_picture_number=h1->coded_picture_number;

    return 0;
}

static int update_context_from_thread(HEVCContext *dst, HEVCContext *src, int for_user)
{
    int err = 0;

    if (dst != src) {
        dst->width     = src->width;
        dst->height    = src->height;
    }

    if (for_user) {
        //dst->coded_frame   = src->coded_frame;
    } else {
        err = lent_update_thread_context(dst, src);
    }

    return err;
}

static void update_context_from_user(HEVCContext *dst, HEVCContext *src)
{
	//todo 需要在多线程解码途中改变的东西，例如flag等，可以在此处更新。目前暂无必要。
}


static void release_delayed_buffers(PerThreadContext *p)
{

    while (p->num_released_buffers > 0) {
        LentFrame *f = &p->released_buffers[--p->num_released_buffers];

        pthread_mutex_lock(&p->buffer_mutex);
        free_progress(f);
        f->thread_opaque = NULL;

        f->owner->release_buffer(f->owner, f);
        pthread_mutex_unlock(&p->buffer_mutex);
    }
}

static int submit_packet(PerThreadContext *p, LentPacket *avpkt)
{
    FrameThreadContext *fctx = p->parent;
    PerThreadContext *prev_thread = fctx->prev_thread;

    uint8_t *buf = p->avpkt.data;

    pthread_mutex_lock(&p->mutex);

    release_delayed_buffers(p);

    if (prev_thread) {
        int err;
        if (prev_thread->state == STATE_SETTING_UP) {
            pthread_mutex_lock(&prev_thread->progress_mutex);
            while (prev_thread->state == STATE_SETTING_UP)
                pthread_cond_wait(&prev_thread->progress_cond, &prev_thread->progress_mutex);
            pthread_mutex_unlock(&prev_thread->progress_mutex);
        }

        err = update_context_from_thread(p->hevc, prev_thread->hevc, 0);
        if (err) {
            pthread_mutex_unlock(&p->mutex);
            return err;
        }
    }

    lent_fast_malloc(&buf, &p->allocated_buf_size, avpkt->size + LENTOID_INPUT_BUFFER_PADDING_SIZE);
    p->avpkt = *avpkt;
    p->avpkt.data = buf;
    memcpy(buf, avpkt->data, avpkt->size);
    memset(buf + avpkt->size, 0, LENTOID_INPUT_BUFFER_PADDING_SIZE);

    p->state = STATE_SETTING_UP;
    pthread_cond_signal(&p->input_cond);
    pthread_mutex_unlock(&p->mutex);

    fctx->prev_thread = p;

    return 0;
}

int lentoid_thread_decode_frame(HEVCContext *h,
                           LentFrame *picture, int *got_picture_ptr)
{
    FrameThreadContext *fctx = h->thread_opaque;
    int finished = fctx->next_finished;
    PerThreadContext *p;
    int err;

	LentPacket *pkt=h->pkt;
    /*
     * Submit a packet to the next decoding thread.
     */

    p = &fctx->threads[fctx->next_decoding];
    update_context_from_user(p->hevc, h);

    err = submit_packet(p, pkt);
    if (err) return err;

    fctx->next_decoding++;

    /*
     * If we're still receiving the initial packets, don't return a frame.
     */

    if (fctx->delaying && pkt->size) {
        if (fctx->next_decoding >= (h->ctx->thread-1)) fctx->delaying = 0;

        *got_picture_ptr=0;
        return 0;
    }

    /*
     * Return the next available frame from the oldest thread.
     * If we're at the end of the stream, then we have to skip threads that
     * didn't output a frame, because we don't want to accidentally signal
     * EOF (avpkt->size == 0 && *got_picture_ptr == 0).
     */

    do {
        p = &fctx->threads[finished++];

        if (p->state != STATE_INPUT_READY) {
            pthread_mutex_lock(&p->progress_mutex);
            while (p->state != STATE_INPUT_READY)
                pthread_cond_wait(&p->output_cond, &p->progress_mutex);
            pthread_mutex_unlock(&p->progress_mutex);
        }

        *picture = p->frame;
        *got_picture_ptr = p->got_frame;
        picture->pkt_dts = p->avpkt.dts;

        /*
         * A later call with avkpt->size == 0 may loop over all threads,
         * including this one, searching for a frame to return before being
         * stopped by the "finished != fctx->next_finished" condition.
         * Make sure we don't mistakenly return the same frame again.
         */
        p->got_frame = 0;

        if (finished >= h->ctx->thread) finished = 0;
    } while (!pkt->size && !*got_picture_ptr && finished != fctx->next_finished);

    update_context_from_thread(h, p->hevc, 1);

    if (fctx->next_decoding >= h->ctx->thread) fctx->next_decoding = 0;

    fctx->next_finished = finished;

    return p->result;
}

void lentoid_thread_report_progress(LentFrame *f, int n)
{
    PerThreadContext *p;
    int *progress = f->thread_opaque;

    if (!progress || progress[0] >= n) return;

    p = f->owner->thread_opaque;

    //if (f->owner->debug&LENTOID_DEBUG_THREADS)
    //    av_log(f->owner, AV_LOG_DEBUG, "%p finished %d field %d\n", progress, n, field);

    pthread_mutex_lock(&p->progress_mutex);
    progress[0] = n;
    pthread_cond_broadcast(&p->progress_cond);
    pthread_mutex_unlock(&p->progress_mutex);
}

void lentoid_thread_await_progress(LentFrame *f, int n)
{
    PerThreadContext *p;
    int *progress = f->thread_opaque;

    if (!progress || progress[0] >= n) return;

    p = f->owner->thread_opaque;

    //if (f->owner->debug&LENTOID_DEBUG_THREADS)
    //    av_log(f->owner, AV_LOG_DEBUG, "thread awaiting %d field %d from %p\n", n, field, progress);

    pthread_mutex_lock(&p->progress_mutex);
    while (progress[0] < n)
        pthread_cond_wait(&p->progress_cond, &p->progress_mutex);
    pthread_mutex_unlock(&p->progress_mutex);
}

void lentoid_thread_finish_setup(HEVCContext *h)
{
    PerThreadContext *p = h->thread_opaque;

    if (!(h->ctx->thread)) return;

    pthread_mutex_lock(&p->progress_mutex);
    p->state = STATE_SETUP_FINISHED;
    pthread_cond_broadcast(&p->progress_cond);
    pthread_mutex_unlock(&p->progress_mutex);
}


/////////////////////////////////////////////////////////////////////////////////////////////////
//工作线程
/////////////////////////////////////////////////////////////////////////////////////////////////
static void *frame_worker_thread(void *arg)
{
    PerThreadContext *p = arg;
    FrameThreadContext *fctx = p->parent;
    HEVCContext *h = p->hevc;

    while (1) {
        if (p->state == STATE_INPUT_READY && !fctx->die) {
            pthread_mutex_lock(&p->mutex);
            while (p->state == STATE_INPUT_READY && !fctx->die)
                pthread_cond_wait(&p->input_cond, &p->mutex);
            pthread_mutex_unlock(&p->mutex);
        }

        if (fctx->die) break;

        pthread_mutex_lock(&p->mutex);
        lent_frame_get_defaults(&p->frame);
        p->got_frame = 0;
        p->result = decode_frame(h, &p->frame, &p->got_frame);

        if (p->state == STATE_SETTING_UP) lentoid_thread_finish_setup(h);

        p->state = STATE_INPUT_READY;

        pthread_mutex_lock(&p->progress_mutex);
        pthread_cond_signal(&p->output_cond);
        pthread_mutex_unlock(&p->progress_mutex);

        pthread_mutex_unlock(&p->mutex);
    }

    return NULL;
}





/////////////////////////////////////////////////////////////////////////////////////////////////
//初始化以及扫尾工作等
/////////////////////////////////////////////////////////////////////////////////////////////////


static void park_frame_worker_threads(FrameThreadContext *fctx, int thread_count)
{
    int i;

    for (i = 0; i < thread_count; i++) {
        PerThreadContext *p = &fctx->threads[i];

        if (p->state != STATE_INPUT_READY) {
            pthread_mutex_lock(&p->progress_mutex);
            while (p->state != STATE_INPUT_READY)
                pthread_cond_wait(&p->output_cond, &p->progress_mutex);
            pthread_mutex_unlock(&p->progress_mutex);
        }
    }
}

void frame_thread_free(HEVCContext *h, int thread_count)
{
    FrameThreadContext *fctx = h->thread_opaque;
    int i;

    park_frame_worker_threads(fctx, thread_count);

    if (fctx->prev_thread)
        update_context_from_thread(fctx->threads->hevc, fctx->prev_thread->hevc, 0);

    fctx->die = 1;

    for (i = 0; i < thread_count; i++) {
        PerThreadContext *p = &fctx->threads[i];

        pthread_mutex_lock(&p->mutex);
        pthread_cond_signal(&p->input_cond);
        pthread_mutex_unlock(&p->mutex);

        pthread_join(p->thread, NULL);

        release_delayed_buffers(p);
    }

    for (i = 0; i < thread_count; i++) {
        PerThreadContext *p = &fctx->threads[i];

		if(p->hevc->cache.alf_pic.data[0])
			p->hevc->release_buffer(p->hevc,(LentFrame*)&p->hevc->cache.alf_pic);
			
        lent_default_free_buffer(p->hevc);


        pthread_mutex_destroy(&p->mutex);
        pthread_mutex_destroy(&p->progress_mutex);
		pthread_mutex_destroy(&p->buffer_mutex);
        pthread_cond_destroy(&p->input_cond);
        pthread_cond_destroy(&p->progress_cond);
        pthread_cond_destroy(&p->output_cond);
        lent_freep(&p->avpkt.data);

		hevc_uninitialize(p->hevc,!i);
        lent_freep(&p->hevc);
    }

    lent_freep(&fctx->threads);
	lent_freep(&h->thread_opaque);
}

int frame_thread_init(LentCodecContext *ctx)
{
    int thread_count = ctx->thread;
	HEVCContext *src = ctx->hevc_ctx;
    FrameThreadContext *fctx;
    int i, err = 0;

    if (thread_count <= 1) {
        ctx->thread = 0;
        return 0;
    }

    src->thread_opaque = fctx = lent_mallocz(sizeof(FrameThreadContext));

    fctx->threads = lent_mallocz(sizeof(PerThreadContext) * thread_count);
    fctx->delaying = 1;

    for (i = 0; i < thread_count; i++) {
        HEVCContext *copy = lent_mallocz(sizeof(HEVCContext));

        PerThreadContext *p  = &fctx->threads[i];

        pthread_mutex_init(&p->mutex, NULL);
        pthread_mutex_init(&p->progress_mutex, NULL);
		pthread_mutex_init(&p->buffer_mutex, NULL);
        pthread_cond_init(&p->input_cond, NULL);
        pthread_cond_init(&p->progress_cond, NULL);
        pthread_cond_init(&p->output_cond, NULL);

        p->parent	= fctx;
        p->hevc		= copy;

        *copy = *src;
        copy->thread_opaque = p;
        copy->pkt = &p->avpkt;
		copy->ctx = ctx;

        if (!i) {
            src = copy;
			ctx->main_thread=copy;

			//update_context_from_thread(ctx->hevc_ctx, copy, 1);
        } else {
            //copy->is_copy   = 1;
        }

        if (err) goto error;

        pthread_create(&p->thread, NULL, frame_worker_thread, p);
    }

    return 0;

error:
	frame_thread_free(ctx->hevc_ctx, i+1);

    return err;
}

void lentoid_thread_flush(HEVCContext *h)
{
    FrameThreadContext *fctx = h->thread_opaque;

    if (!h->thread_opaque) return;

	park_frame_worker_threads(fctx, h->ctx->thread);

    if (fctx->prev_thread)
        update_context_from_thread(fctx->threads->hevc, fctx->prev_thread->hevc, 0);

    fctx->next_decoding = fctx->next_finished = 0;
    fctx->delaying = 1;
    fctx->prev_thread = NULL;
}


/////////////////////////////////////////////////////////////////////////////////////////////////
//Debugging
/////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
int lent_thread_getnumber(HEVCContext *h)
{
	if(h->thread_opaque)
	{
		return ((PerThreadContext*)h->thread_opaque)-((PerThreadContext*)h->thread_opaque)->parent->threads;
	}
	else
		return -1;
}
#endif