//**********************************************
//codec.h
//Unipipy @2011
//Main entrance of this decoder
//**********************************************

#ifndef DECODER_DECODER_H
#define DECODER_DECODER_H

//pipy svc///////////////////////////
#ifdef __cplusplus
	#define LENTOIDHE_EXPORT extern "C" 
#else
	#define LENTOIDHE_EXPORT 
#endif
//end///////////////////////////

#define LENT_NOPTS_VALUE          INT64_C(0x8000000000000000)

#define LENTOID_INPUT_BUFFER_PADDING_SIZE 8



#define LENTOIDHE_COMMON_FRAME	\
    uint8_t *data[4];			\
    uint8_t *base[4];			\
    int linesize[4];			\
    int width,height;			\
								\
    int pict_type;				\
								\
    int64_t pts;				\
								\
    int coded_picture_number;	\
								\
    int reference;				\
								\
	/*buffer type*/				\
    int type;					\
								\
    int16_t (*motion_val[2])[2];\
    int8_t	*ref_index[2];		\
	int8_t	ref_count[2];		\
	/*todo:is this necessary?*/	\
	int		ref_poc[2][16];		\
								\
	void*	thread_opaque;		\
								\
								\
    int64_t pkt_pts;			\
    int64_t pkt_dts;			\
								\
    struct HEVCContext *owner;	\
//LENTOIDHE_FRAME end


typedef struct LentFrame{
	LENTOIDHE_COMMON_FRAME
}LentFrame;


typedef struct LentCodecContext {

    const LentClass *lent_class;

    int width, height;
	int thread;
	int compatibility;

    void *hevc_ctx;//codec ctx，在多线程解码时主要是作为FrameThreadContext的入口
	void *main_thread;//多线程解码时，第一个线程的HEVCctx指针，用于flush,release等。

} LentCodecContext;


LENTOIDHE_EXPORT	LentCodecContext *	lentdecoder_alloc_context	(int compatibility);
LENTOIDHE_EXPORT	int					lentdecoder_open			(LentCodecContext *);
LENTOIDHE_EXPORT	void				lentdecoder_flush_context	(LentCodecContext *);
LENTOIDHE_EXPORT	void				lentdecoder_close_context	(LentCodecContext **);


LENTOIDHE_EXPORT	LentFrame *			lentdecoder_alloc_frame		(void);


LENTOIDHE_EXPORT	int					lentdecoder_decode			(LentCodecContext *ctx, LentFrame *picture, int *got_picture_ptr, const uint8_t *buf, int buf_size, int64_t pts);



LENTOIDHE_EXPORT	int					lentdecoder_version			(void);

#endif