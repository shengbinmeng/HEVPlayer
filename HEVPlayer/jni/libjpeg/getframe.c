#include <jni.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>

#include "libavcodec/avcodec.h"
#include "libavutil/avstring.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/opt.h"
#include "jpeglib.h"
#include "player_utils.h"
#define LOG_TAG "getframe"
int framenum=0 ;
//static int  sws_flags = SWS_BICUBIC ;
void draw_jpeg(AVPicture *pic, char * savename, char * dir, int width,int height) 
{
 	char fname[128];
// AVPicture my_pic ;
 	struct jpeg_compress_struct cinfo ;
 	struct jpeg_error_mgr jerr ;
 	JSAMPROW  row_pointer[1] ;
 	int row_stride ;
 	uint8_t *buffer ;
 	FILE *fp ;
 
 	//vfmt2rgb(my_pic,pic) ;
    buffer = pic->data[0];

#ifdef __MINGW32__
    sprintf(fname, "%s/%s.jpg" , dir, savename);
#else
    sprintf(fname, "%s/%s.jpg" , dir, savename);
#endif
    fp = fopen (fname, "wb");
    if (fp == NULL)
    {
        av_log(NULL, AV_LOG_ERROR, "fopen %s error/n", fname);
        return;
    }
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    jpeg_stdio_dest(&cinfo, fp);

    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;

    jpeg_set_defaults(&cinfo);
   

    jpeg_set_quality(&cinfo, 80,TRUE);
    
    
    jpeg_start_compress(&cinfo, TRUE);

    row_stride = width * 3;
    while (cinfo.next_scanline < height)
    {
        row_pointer[0] = &buffer[cinfo.next_scanline * row_stride];
        (void)jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }

    jpeg_finish_compress(&cinfo);
    fclose(fp);
    jpeg_destroy_compress(&cinfo);
 	printf("compress %d frame finished!/n",framenum) ;
    return ;
}

int captrue_frame( char * ifilename, char * savename, char * dir )//main()
{
 	AVFormatContext *pFormatCtx;
 	
 	AVStream *st;
 	AVCodecContext *pCodecCtx;
 	AVCodec *pCodec;
 	AVFrame *pFrame,*pFrameRGB;
 	uint8_t *buffer;
 	AVPacket packet;
 	struct SwsContext *img_convert_ctx=NULL;
 	int numBytes;
	int ret;
 	int i,videoStream=-1,frameFinished;
 	
	
 	av_register_all();
	
	LOGI("%s \n", ifilename);
	pFormatCtx = NULL;
	ret = avformat_open_input(&pFormatCtx, ifilename, NULL, NULL);
 	if(ret != 0)
 	{
		//LOGI("%s \n", ifilename);
  	  	LOGI("open video failed!/n") ;
  		return 0;
 	}
	
	LOGI("%s \n", "step0");

 	if(avformat_find_stream_info(pFormatCtx, NULL)<0)
 	{
 	   	printf("get information failed!/n") ;
  	  	return 0;
 	}
	LOGI("%s \n", "step1");

 
 	for(i=0; i<pFormatCtx->nb_streams; i++)
 	{
  	  	if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO)
  	  	{
  		  	videoStream=i;
  		  	break;
  	  	}
 	}
 	if(videoStream==-1)
  	  	return 0; 
	LOGI("%s \n", "step2");
  	
 	st=pFormatCtx->streams[videoStream] ;
 	pCodecCtx=st->codec;

 	
 	pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
 	if(pCodec==NULL) {
  		fprintf(stderr, "Unsupported codec!/n");
  		return 0; 
 	}
	
 	if(avcodec_open2(pCodecCtx, pCodec, NULL)<0)
 	{
  		printf("open encoder failed!") ;
  		return 0;
 	}

 	
 	pFrame=avcodec_alloc_frame();
 
 	   
 	pFrameRGB=avcodec_alloc_frame();
 	if(pFrameRGB==NULL)
 	{
  	  	printf("allocate AVframe failed!/n") ;
  		return 0;
 	}
	


 	numBytes=avpicture_get_size(PIX_FMT_RGB24, pCodecCtx->width,
                            pCodecCtx->height);
 	buffer=(uint8_t *)av_malloc(numBytes*sizeof(uint8_t));
 

 	avpicture_fill((AVPicture *)pFrameRGB, buffer,PIX_FMT_RGB24,

    pCodecCtx->width, pCodecCtx->height);
	LOGI("%s \n", "step3");
 
	
 	i=0;
 	while(av_read_frame(pFormatCtx, &packet)>=0)
 	{
		LOGI("%s \n", "step4");
		
  		if(packet.stream_index==videoStream)
  		{
   			avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished,
                         &packet);
			LOGI("%s \n", "step5");
						 
   			if(frameFinished)
   			{
				LOGI("%s \n", "step6");
    			if(img_convert_ctx==NULL)
    			{
      			  	img_convert_ctx = sws_getCachedContext(img_convert_ctx,pCodecCtx->width,pCodecCtx->height,
       			 	pCodecCtx->pix_fmt,pCodecCtx->width,pCodecCtx->height,PIX_FMT_RGB24 ,
     			   	SWS_X ,NULL,NULL,NULL) ;
      			  	if (img_convert_ctx == NULL) 
      			  	{   
						LOGI("%s \n", "step6");
       				 	printf("can't init convert context!/n") ;
       				 	return 0;
      			  	}

    			}
				LOGI("%s \n", "step7");
				//break;
				
    			sws_scale(img_convert_ctx, pFrame->data, pFrame->linesize,
         	   		0, pCodecCtx->height, pFrameRGB->data, pFrameRGB->linesize);
   				//int a=++i ;
    			//if((a>50)&&(a<100))
    			//{
					 draw_jpeg((AVPicture*)pFrameRGB, savename, dir, pCodecCtx->width, pCodecCtx->height) ;
					 LOGI("%s \n", "step8");
    				 break;
    			//}
				
   			}
			
  		}
		
 		av_free_packet(&packet);
		
 	}

 	av_free(buffer);
 	av_free(pFrameRGB);


 	avcodec_free_frame(&pFrame);

 	avcodec_close(pCodecCtx);

 	avformat_close_input(&pFormatCtx);
	
	return 1;
}
jint Java_pku_shengbin_hevplayer_LocalExplorerAdapter_getframe(JNIEnv * env, jobject this, jstring path, jstring savename, jstring dir)
{
	const char *pathStr = (*env)->GetStringUTFChars(env,path, NULL);
	const char *savenameStr = (*env)->GetStringUTFChars(env,savename, NULL);
	const char *dirStr = (*env)->GetStringUTFChars(env,dir, NULL);
	return captrue_frame((char *)pathStr, (char *)savenameStr, (char *)dirStr);
}
