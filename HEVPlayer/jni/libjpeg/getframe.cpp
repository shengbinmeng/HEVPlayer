#include <jni.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>//注意要包含此头文件与sprintf函数相关

extern "C"
{
	//ffmpeg相关的头文件
	#include "libavutil/avstring.h"
	#include "libavformat/avformat.h"
	#include "libswscale/swscale.h"
	#include "libavutil/opt.h"
	#include "jpeglib.h"
}
int framenum=0 ;
//static int  sws_flags = SWS_BICUBIC ;

//实现视频帧的jpeg压缩
void draw_jpeg(AVPicture *pic,int width,int height)
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
    sprintf(fname, "/sdcard/%sDLPShot-%d.jpg", "frame", framenum++);
#else
    sprintf(fname, "/sdcard/%sDLPShot-%d.jpg", "frame", framenum++);
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


    jpeg_set_quality(&cinfo, 80,true);


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

int captrue_frame( char * ifilename )//main()
{
 	AVFormatContext *pFormatCtx;
 	//static char * ifilename="1.asf" ;
 	AVStream *st;
 	AVCodecContext *pCodecCtx;
 	AVCodec *pCodec;
 	AVFrame *pFrame,*pFrameRGB;
 	uint8_t *buffer;
 	AVPacket packet;
 	struct SwsContext *img_convert_ctx=NULL;
 	int numBytes;

 	int i,videoStream=-1,frameFinished;


 	av_register_all();
 	if(av_open_input_file2(&pFormatCtx, ifilename, NULL, 0, NULL)!=0)
 	{
  	  	printf("open video failed!/n") ;
  		return 0;
 	}

 	//read information about input file ;
 	if(av_find_stream_info(pFormatCtx)<0)
 	{
 	   	printf("get information failed!/n") ;
  	  	return 0;
 	}
	//print information about file
 	dump_format(pFormatCtx, 0, ifilename, 0);

 	for(i=0; i<pFormatCtx->nb_streams; i++)
 	{
  	  	if(pFormatCtx->streams[i]->codec->codec_type==CODEC_TYPE_VIDEO)
  	  	{
  		  	videoStream=i;
  		  	break;
  	  	}
 	}
 	if(videoStream==-1)
  	  	return 0; // Didn't find a video stream

  	// Get a pointer to the codec context for the video stream
 	st=pFormatCtx->streams[videoStream] ;
 	pCodecCtx=st->codec;

 	// Find the decoder for the video stream
 	pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
 	if(pCodec==NULL) {
  		fprintf(stderr, "Unsupported codec!/n");
  		return 0; // Codec not found
 	}
	// Open codec
 	if(avcodec_open(pCodecCtx, pCodec)<0)
 	{
  		printf("open encoder failed!") ;
  		return 0;
 	}

 	// Allocate video frame
 	pFrame=avcodec_alloc_frame();

 	   // Allocate an AVFrame structure
 	pFrameRGB=avcodec_alloc_frame();
 	if(pFrameRGB==NULL)
 	{
  	  	printf("allocate AVframe failed!/n") ;
  		return 0;
 	}


 // Determine required buffer size and allocate buffer
 	numBytes=avpicture_get_size(PIX_FMT_RGB24, pCodecCtx->width,
                            pCodecCtx->height);
 	buffer=(uint8_t *)av_malloc(numBytes*sizeof(uint8_t));

 // Assign appropriate parts of buffer to image planes in pFrameRGB
// Note that pFrameRGB is an AVFrame, but AVFrame is a superset
// of AVPicture
 	avpicture_fill((AVPicture *)pFrameRGB, buffer,PIX_FMT_RGB24,
 //  avpicture_fill((AVPicture *)pFrameRGB, buffer, PIX_FMT_RGB24,
    pCodecCtx->width, pCodecCtx->height);
 //pFrameRGB->alloc_picture(PIX_FMT_RGB24,pCodecCtx->width,pCodecCtx->height) ;

 	i=0;
 	while(av_read_frame(pFormatCtx, &packet)>=0)
 	{
  // Is this a packet from the video stream?
  		if(packet.stream_index==videoStream)
  		{
 // Decode video frame
   			 avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished,
                         packet.data, packet.size);

    // Did we get a video frame?
   			if(frameFinished)
   			{
  			// Convert the image from its native format to RGB

    			if(img_convert_ctx==NULL)
    			{
      			  	img_convert_ctx=sws_getCachedContext(img_convert_ctx,pCodecCtx->width,pCodecCtx->height,
      			  	//PIX_FMT_YUV420P,pCodecCtx->width,pCodecCtx->height,pCodecCtx->pix_fmt,
       			 	pCodecCtx->pix_fmt,pCodecCtx->width,pCodecCtx->height,PIX_FMT_RGB24 ,
     			   	SWS_X ,NULL,NULL,NULL) ;
      			  	if (img_convert_ctx == NULL)
      			  	{
       				 	printf("can't init convert context!/n") ;
       				 	return 0;
      			  	}

    			}
    			sws_scale(img_convert_ctx, pFrame->data, pFrame->linesize,
         	   		0, pCodecCtx->width, pFrameRGB->data, pFrameRGB->linesize);
   					 //av_picture_copy((AVPicture*)pFrameRGB,(AVPicture*)pFrame,PIX_FMT_RGB24,pCodecCtx->width,pCodecCtx->height) ;

   					 // Save the frame to disk
   				int a=++i ;
    			if((a>50)&&(a<100))
    				 // SaveFrame(pFrameRGB, pCodecCtx->width,
    				 //  pCodecCtx->height, i);
    				 draw_jpeg((AVPicture*)pFrameRGB,pCodecCtx->width,pCodecCtx->height) ;
				break;
   			}
  		}

  			  	// Free the packet that was allocated by av_read_frame
 		av_free_packet(&packet);
 	}
 		  	// Free the RGB image
 	av_free(buffer);
 	av_free(pFrameRGB);

			// Free the YUV frame
 	av_free(pFrame);

			// Close the codec
 	avcodec_close(pCodecCtx);

			// Close the video file
 	av_close_input_file(pFormatCtx);
	return 1;
}
void Java_pku_shengbin_hevplayer_LocalExploreActivity_getframe(JNIEnv * env, jobject this, jstring path)
{
	const char *pathStr = env->GetStringUTFChars(path, NULL);
	return captrue_frame((char *)pathStr);
}
