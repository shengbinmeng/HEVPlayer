//**********************************************
//hevc_frame.c
//Unipipy @2011
//frame management
//**********************************************

#include "hevc.h"
#include "hevc_frame.h"
#include "hevc_thread.h"


typedef struct InternalBuffer{
	uint8_t *base[4];
	uint8_t *data[4];
	int linesize[4];
	int width, height;
    int16_t (*motion_val[2])[2];
    //int16_t (*motion_val_base[2])[2];
    int8_t	*ref_index[2];		
}InternalBuffer;


/////////////////////////////////////////////////////////////////////////////////////////////////
//未用图片的管理
/////////////////////////////////////////////////////////////////////////////////////////////////
void lent_release_unused_pictures(HEVCContext *h)
{
	int i;
	for(i=0;i<h->picture_count;i++)
	{
		if(h->picture[i].data[0] && !h->picture[i].reference
			&&h->picture[i].owner2==h
			)
		{
			lent_dlog(NULL,"poc %d released\n",h->picture[i].poc);
			//lent_dlog(NULL,"dbp %d\n",h->dpb_count);
			lent_picture_release_buffer(h,&h->picture[i]);
		}
	}
}
int lent_find_unused_pictures(HEVCContext *h)
{
	int i;
	for(i=0;i<h->picture_count;i++)
	{
            if(h->picture[i].data[0]==NULL && h->picture[i].type!=0) return i; //FIXME
	}
	for(i=0;i<h->picture_count;i++)
	{
            if(h->picture[i].data[0]==NULL) return i;
	}
	return -1;
}


/////////////////////////////////////////////////////////////////////////////////////////////////
//对外接口
/////////////////////////////////////////////////////////////////////////////////////////////////

void lent_picture_start(HEVCContext *h)//开始解码一帧，释放不用的旧帧，开新帧，申请缓冲并置为当前帧
{
	Picture *pic;
	int i;

	lent_release_unused_pictures(h);

	assert(!(h->current_picture_ptr&& h->current_picture_ptr->data[0]==NULL));

	i=lent_find_unused_pictures(h);
	//lent_dlog(NULL,"unused returned %d\n",i);
	
	
	pic=&h->picture[i];

	pic->reference=0;
	if(!h->dropable)
		pic->reference=FRAME_REF;
	pic->coded_picture_number=h->coded_picture_number++;

	lent_alloc_picture(h,pic);
	lent_copy_picture(&h->current_picture,pic);

	h->current_picture_ptr=pic;

	h->next_output_pic=NULL;

	//if(!h->current.luma_nonfiltered)//init nonfiltered pixel buffer
	//	h->current.luma_nonfiltered=lent_malloc((h->cu_height*h->current_picture.linesize[0]+h->width)*64);

}

void lent_picture_end(HEVCContext *h)//当前帧解码结束
{
}


void lent_copy_picture(Picture *dst, Picture *src)//复制picture结构，共享缓冲
{
    *dst = *src;
    dst->type= LENT_BUFFER_TYPE_COPY;
}


/////////////////////////////////////////////////////////////////////////////////////////////////
//内部函数
/////////////////////////////////////////////////////////////////////////////////////////////////


int lent_alloc_picture(HEVCContext *s, Picture *pic)//申请data和表
{
    const int b4_array_size= s->b4_stride*s->cu_height*16;

    int r= -1;

	//申请data buffer
    assert(!pic->data[0]);
	if (r=lent_picture_get_buffer(s, pic) < 0)
		return -1;
	r=0;

	//当前linesize写入context
	s->linesize  = pic->linesize[0];
	s->uvlinesize= pic->linesize[1];

	//初始化表
    //if(pic->motion_val[0]==NULL){
    //    for(i=0; i<2; i++){
    //        LENT_ALLOCZ_OR_GOTO(s->ctx, pic->motion_val_base[i], 2 * (b4_array_size+4)  * sizeof(int16_t), fail)//8x8划分
    //        pic->motion_val[i]= pic->motion_val_base[i]+4;
    //        LENT_ALLOCN_OR_GOTO(s->ctx, pic->ref_index[i], b4_array_size * sizeof(uint8_t), fail)//8x8划分
    //    }
    //}
	memset(pic->ref_index[0],-1,b4_array_size * sizeof(uint8_t));//fixme 在这里合适否？
	memset(pic->ref_index[1],-1,b4_array_size * sizeof(uint8_t));
	pic->ref_count[0]=pic->ref_count[1]=0;


	//allocator
    pic->owner2 = s;
    pic->owner = s;
	pic->width = s->width;
	pic->height = s->height;

    return 0;
//fail: //for the LENT_ALLOCZ_OR_GOTO macro
    if(r>=0)
        lent_picture_release_buffer(s, pic);
    return -1;
}

void lent_free_picture(HEVCContext *s, Picture *pic)//清空data和表，仅用于解码结束
{

	//清空data
    if(pic->data[0] && pic->type!=LENT_BUFFER_TYPE_SHARED){
        lent_picture_release_buffer(s, pic);
    }
	////清空表
 //   for(i=0; i<2; i++){
	//	pic->motion_val[0]=NULL;
	//	pic->motion_val[1]=NULL;
 //       lent_freep((void**)&pic->motion_val_base[i]);
 //       lent_freep(&pic->ref_index[i]);
 //   }
}




/////////////////////////////////////////////////////////////////////////////////////////////////
//data缓冲区管理
/////////////////////////////////////////////////////////////////////////////////////////////////


void lent_picture_release_buffer(HEVCContext *h, Picture *pic)//释放data缓冲，对于帧并行解码的情况，调用thread版本以进行延迟释放
{
	lent_thread_picture_release_buffer(h,(LentFrame*)pic);
}

int lent_picture_get_buffer(HEVCContext *h, Picture *pic)//申请data缓冲
{
	return lent_thread_picture_get_buffer(h,(LentFrame*)pic);
}

int lent_default_get_buffer(HEVCContext *s, LentFrame *pic)//默认的data申请函数
{
	int i;
	int w= s->width;
	int h= s->height;
	InternalBuffer *buf;
    const int b4_array_size= s->b4_stride*s->cu_height*16;

	if(pic->data[0]!=NULL) {
		lent_log(s, LENT_LOG_ERROR, "pic->data[0]!=NULL in avcodec_default_get_buffer\n");
		return -1;
	}
	if(s->internal_buffer_count >= INTERNAL_BUFFER_SIZE) {
		lent_log(s, LENT_LOG_ERROR, "internal_buffer_count overflow (missing release_buffer?)\n");
		return -2;
	}

	if(s->internal_buffer==NULL){
		s->internal_buffer= lent_mallocz((INTERNAL_BUFFER_SIZE+1)*sizeof(InternalBuffer));
	}
	if(!s->internal_buffer){
		lent_log(s, LENT_LOG_ERROR, "cannot alloc internal buffer (missing release_buffer?)\n");
		return -3;
	}

	buf= &((InternalBuffer*)s->internal_buffer)[s->internal_buffer_count];

	if(buf->base[0] && (buf->width != w || buf->height != h/* || buf->pix_fmt != s->pix_fmt*/)){//检查已经存在的buffer长宽是否变化，若变化则需要重新申请（尽量避免）
		for(i=0; i<4; i++){
			lent_freep(&buf->base[i]);
			buf->data[i]= NULL;
		}
	}

	if(buf->base[0]){

	}else{
		int size[4] = {0};
		int tmpsize;
		Picture picture;
		int stride_align[4];

		w=LENTALIGN(w,64);//将长宽按64对齐（大CU）
		h=LENTALIGN(h,64);
		stride_align[0]=
			stride_align[1]=
			stride_align[2]=
			stride_align[3]=ALIGN_SIZE;

		w+= EDGE_WIDTH*2;//padding 80
		h+= EDGE_TOP+EDGE_BOTTOM;

		picture.linesize[0]=LENTALIGN(w,ALIGN_SIZE);//padding后的大小再按ALIGN_SIZE对齐
		picture.linesize[1]=LENTALIGN(w/2,ALIGN_SIZE);
		picture.linesize[2]=LENTALIGN(w/2,ALIGN_SIZE);
		picture.linesize[3]=0;

		picture.data[0]=NULL;
		picture.data[1]=picture.data[0]+h*picture.linesize[0];
		picture.data[2]=picture.data[1]+h/2*picture.linesize[1];
		picture.data[3]=0;
		tmpsize=h*(picture.linesize[0]+picture.linesize[1]);

		size[0]=h*picture.linesize[0];
		size[1]=h*picture.linesize[1]/2;
		size[2]=h*picture.linesize[2]/2;

		memset(buf->base, 0, sizeof(buf->base));
		memset(buf->data, 0, sizeof(buf->data));

		for(i=0; i<4 && size[i]; i++){
			const int h_shift= i==0 ? 0 : 1;
			const int v_shift= i==0 ? 0 : 1;

			buf->linesize[i]= picture.linesize[i];

			buf->base[i]= lent_malloc(size[i]+ALIGN_SIZE); //FIXME 16
			if(buf->base[i]==NULL) return -5;
			memset(buf->base[i], 128, size[i]);

			buf->data[i] = buf->base[i] + LENTALIGN(
				(buf->linesize[i]*EDGE_TOP>>v_shift)//上方的EDGE_TOP行
				+ (EDGE_WIDTH>>h_shift), //左方的EDGE_TOP列
				stride_align[i]);//按ALIGN_SIZE对齐
		}
		buf->width  = s->width;
		buf->height = s->height;

        for(i=0; i<2; i++){
            //LENT_ALLOCZ_OR_GOTO(s->ctx, buf->motion_val_base[i], 2 * (b4_array_size+4)  * sizeof(int16_t), fail)//4x4划分
            //buf->motion_val[i]= buf->motion_val_base[i]+4;
            LENT_ALLOCN_OR_GOTO(s->ctx, buf->motion_val[i], 2 * b4_array_size * sizeof(uint16_t), fail)//4x4划分
            LENT_ALLOCN_OR_GOTO(s->ctx, buf->ref_index[i], b4_array_size * sizeof(uint8_t), fail)//4x4划分
        }
	}
	pic->type=LENT_BUFFER_TYPE_INTERNAL;

	for(i=0; i<4; i++){
		pic->base[i]= buf->base[i];
		pic->data[i]= buf->data[i];
		pic->linesize[i]= buf->linesize[i];
	}
    for(i=0; i<2; i++){
		pic->motion_val[i]=buf->motion_val[i];
		pic->ref_index[i]=buf->ref_index[i];
	}
	s->internal_buffer_count++;

	if(s->pkt)		pic->pkt_pts= s->pkt->pts;
	else			pic->pkt_pts= LENT_NOPTS_VALUE;

	return 0;
fail:
	return -5;
}


void lent_default_release_buffer(HEVCContext *s, LentFrame *pic)//默认的data释放函数
{
	int i;
	InternalBuffer *buf, *last;

	assert(pic->type==LENT_BUFFER_TYPE_INTERNAL);
	assert(s->internal_buffer_count);

	if(s->internal_buffer){
		buf = NULL; /* avoids warning */
		for(i=0; i<s->internal_buffer_count; i++){ //just 3-5 checks so is not worth to optimize
			buf= &((InternalBuffer*)s->internal_buffer)[i];
			if(buf->data[0] == pic->data[0])
				break;
		}
		assert(i < s->internal_buffer_count);
		s->internal_buffer_count--;
		last = &((InternalBuffer*)s->internal_buffer)[s->internal_buffer_count];

		LENTSWAP(InternalBuffer, *buf, *last);
	}

	for(i=0; i<4; i++){
		pic->data[i]=NULL;
	}
	pic->motion_val[0]=NULL;
	pic->ref_index[1]=NULL;
	pic->motion_val[0]=NULL;
	pic->ref_index[1]=NULL;
}

void lent_default_free_buffer(HEVCContext *s)//默认的data清空函数，用于解码结束
{
    int i, j;

    if(s->internal_buffer==NULL) return;

    if (s->internal_buffer_count)
        lent_log(NULL, LENT_LOG_WARNING, "Found %i unreleased buffers!\n", s->internal_buffer_count);
    for(i=0; i<INTERNAL_BUFFER_SIZE; i++){
        InternalBuffer *buf= &((InternalBuffer*)s->internal_buffer)[i];
        for(j=0; j<4; j++){
            lent_freep(&buf->base[j]);
            buf->data[j]= NULL;
        }
		for(j=0; j<2; j++){
            lent_freep((void**)&buf->motion_val[j]);
            buf->motion_val[j]= NULL;
			lent_freep((void**)&buf->ref_index[j]);
        }
    }
    lent_freep(&s->internal_buffer);

    s->internal_buffer_count=0;
}