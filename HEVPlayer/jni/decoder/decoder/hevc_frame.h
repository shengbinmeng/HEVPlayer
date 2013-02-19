//**********************************************
//hevc_frame.h
//Unipipy @2011
//frame management
//**********************************************

#ifndef DECODER_HEVC_FRAME_H
#define DECODER_HEVC_FRAME_H

#define INTERNAL_BUFFER_SIZE		30
#define MAX_PICTURE_COUNT			(MAX_DPB_SIZE+MAX_DELAYED_PIC_COUNT+1)//must be greater than DPB_SIZE//test
#define ALIGN_SIZE					16

#define	FRAME_REF					3
#define DELAYED_PIC_REF				4

/////////////////////////////////////////////////////////////////////////////////////////////////
//BUFFER类型FIXME not necessary?
/////////////////////////////////////////////////////////////////////////////////////////////////
#define LENT_BUFFER_TYPE_INTERNAL	1
#define LENT_BUFFER_TYPE_USER		2//暂不支持
#define LENT_BUFFER_TYPE_SHARED		4//暂不支持
#define LENT_BUFFER_TYPE_COPY		8//暂不支持


/////////////////////////////////////////////////////////////////////////////////////////////////
//对外接口
/////////////////////////////////////////////////////////////////////////////////////////////////
void	lent_copy_picture			(Picture *dst, Picture *src);
void	lent_picture_end			(HEVCContext *h);
void	lent_picture_start			(HEVCContext *h);


/////////////////////////////////////////////////////////////////////////////////////////////////
//内部函数
/////////////////////////////////////////////////////////////////////////////////////////////////
int		lent_alloc_picture			(HEVCContext *h, Picture *pic);
void	lent_free_picture			(HEVCContext *h, Picture *pic);


/////////////////////////////////////////////////////////////////////////////////////////////////
//data缓冲区管理
/////////////////////////////////////////////////////////////////////////////////////////////////
void	lent_picture_release_buffer	(HEVCContext *h, Picture *pic);
int		lent_picture_get_buffer		(HEVCContext *h, Picture *pic);

int		lent_default_get_buffer		(HEVCContext *h, LentFrame *pic);
void	lent_default_release_buffer	(HEVCContext *h, LentFrame *pic);
void	lent_default_free_buffer	(HEVCContext *h);


/////////////////////////////////////////////////////////////////////////////////////////////////
//杂项
/////////////////////////////////////////////////////////////////////////////////////////////////
static lent_always_inline void
lent_frame_get_defaults				(LentFrame *pic)
{
    memset(pic, 0, sizeof(LentFrame));
    pic->pts= LENT_NOPTS_VALUE;
}


#endif