//**********************************************
//codec.c
//Unipipy @2011
//main entrance of decoder core
//**********************************************
#include "internal.h"
#include "hevc.h"
#include "hevc_frame.h"
#include "hevc_pred.h"
#include "hevc_thread.h"

LENTOIDHE_EXPORT	LentCodecContext *	lentdecoder_alloc_context	(int compatibility)
{
	LentCodecContext *tmp;
	HEVCContext *h;
	tmp=(LentCodecContext *)lent_mallocz(sizeof(LentCodecContext));
	tmp->hevc_ctx=(HEVCContext *)lent_mallocz(sizeof(HEVCContext));
	tmp->compatibility=compatibility;
	h=tmp->hevc_ctx;
	h->ctx=tmp;
	h->get_buffer=lent_default_get_buffer;
	h->release_buffer=lent_default_release_buffer;
	h->free_buffer=lent_default_free_buffer;
	
	return tmp;
}

LENTOIDHE_EXPORT	int					lentdecoder_open			(LentCodecContext *ctx)
{
	if(ctx->thread)
	{
		return frame_thread_init(ctx);
	}
	return 0;
}
LENTOIDHE_EXPORT	void				lentdecoder_flush_context	(LentCodecContext *ctx)
{
	if(ctx->thread)
	{
		lentoid_thread_flush(ctx->hevc_ctx);
		hevc_flush_dpb(ctx->main_thread);
	}
	else
		hevc_flush_dpb(ctx->hevc_ctx);
}

LENTOIDHE_EXPORT	void				lentdecoder_close_context	(LentCodecContext **ctx)
{
	if((*ctx)->thread)
	{
		frame_thread_free((*ctx)->hevc_ctx,(*ctx)->thread);
	}
	hevc_uninitialize((*ctx)->hevc_ctx,1);
	lent_freep(&(*ctx)->hevc_ctx);
	lent_freep(ctx);
}

LENTOIDHE_EXPORT	LentFrame *			lentdecoder_alloc_frame		(void)
{
	LentFrame *tmp;
	tmp=(LentFrame *)lent_malloc(sizeof(LentFrame));
	return tmp;
}

LENTOIDHE_EXPORT	int					lentdecoder_decode			(LentCodecContext *ctx, LentFrame *picture, int *got_picture_ptr, const uint8_t *buf, int buf_size, int64_t pts)
{
	LentPacket pkt;
	pkt.data=(uint8_t*)buf;
	pkt.size=buf_size;
	pkt.pts=pts;
	((HEVCContext*)ctx->hevc_ctx)->pkt=&pkt;
	if(ctx->thread)
		return lentoid_thread_decode_frame(ctx->hevc_ctx,picture,got_picture_ptr);
	else
		return decode_frame(ctx->hevc_ctx,picture,got_picture_ptr);
}


LENTOIDHE_EXPORT	int					lentdecoder_version			(void)
{
	return VERSION;
}

