//**********************************************
//decoder.c
//Unipipy @2011
//Main C++ entrance of this decoder
//**********************************************
#include "decoder.h"
extern "C"{
#include "decoder/internal.h"
}

DecodeCore::DecodeCore()
{
	ctx=NULL;
	frame=NULL;
	i_thread = 1;
}

DecodeCore::~DecodeCore()
{
	Clean();
}
void DecodeCore::Clean()
{
	if (ctx) {
		lentdecoder_flush_context(ctx);
		lentdecoder_close_context(&ctx);
		ctx=NULL;
	}
	if (frame) {
		lent_freep((void**)&frame);
	}
}
void DecodeCore::FlushDecoder()
{
	if(IsReleased())
		return;
	lentdecoder_flush_context(ctx);
}


int DecodeCore::StartDecoder(int compatibility)
{
	lent_log_set_level(LENT_LOG_DEBUG);


	do{
		if(ctx||frame)
			break;

		if(!(ctx=lentdecoder_alloc_context(compatibility)))
			break;

		ctx->thread=i_thread>1?i_thread:0;
		if(lentdecoder_open(ctx))
			break;

		if(!(frame=lentdecoder_alloc_frame()))
			break;

		return 0;

	}while(0);
	Clean();
	return -1;
}
void DecodeCore::DecodeFrame(uint8_t *InputNalBuffer, uint8_t **OutputYUVBuffer, long *pDataLength, int64_t* pts, int *width, int stride[3])
{
	LentCodecContext *local_ctx=ctx;
	LentFrame *local_frame=frame;

	int remainLen=*pDataLength,bytesUsed,framesGot=0;
	*pDataLength = 0;
	if((!InputNalBuffer||remainLen>0)){

		int64_t pts_value=pts?*pts:LENT_NOPTS_VALUE;
		bytesUsed=lentdecoder_decode(local_ctx,local_frame,&framesGot,InputNalBuffer,remainLen,pts_value);

		if(bytesUsed<0)
		{
			*pDataLength=-1;
			return;
		}

		if(framesGot)
		{
			if(pts)
				*pts = local_frame->pkt_pts;

			*width = local_ctx->width;

			stride[0] = local_frame->linesize[0];
			stride[1] = local_frame->linesize[1];
			stride[2] = local_frame->linesize[2];

			*OutputYUVBuffer     = local_frame->data[0];
			*(OutputYUVBuffer+1) = local_frame->data[1];
			*(OutputYUVBuffer+2) = local_frame->data[2];
			*pDataLength=*width * local_ctx->height*3/2;
		}
	}
}


void DecodeCore::UninitDecoder()
{
	Clean();
}

bool DecodeCore::IsReleased()
{
	return !ctx;
}

void DecodeCore::Set_Thread(int n)
{
	i_thread = n;
}