//**********************************************
//hevc_thread.h
//Unipipy @2011
//framewise threading
//**********************************************
#ifndef DECODER_HEVC_THREAD_H
#define DECODER_HEVC_THREAD_H

int lent_thread_picture_get_buffer(HEVCContext *h, LentFrame *pic);//…Í«Îdataª∫≥Â
void lent_thread_picture_release_buffer(HEVCContext *h, LentFrame *pic);// Õ∑≈dataª∫≥Â

void lentoid_thread_finish_setup(HEVCContext *h);
void lentoid_thread_await_progress(LentFrame *f, int n);
void lentoid_thread_report_progress(LentFrame *f, int n);


void lentoid_thread_flush(HEVCContext *h);
void frame_thread_free(HEVCContext *h, int thread_count);
int frame_thread_init(LentCodecContext *ctx);


int lentoid_thread_decode_frame(HEVCContext *h,
                           LentFrame *picture, int *got_picture_ptr);

#ifdef _DEBUG
int lent_thread_getnumber(HEVCContext *h);
#endif

#endif