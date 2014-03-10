#include <string.h>  
#include <jni.h>  
  
#include <math.h>  
#include <stdio.h>  
#include <stdint.h>  
  
typedef uint8_t BYTE;  
#define true 1  
#define false 0  
  
  
#include "jpeglib.h"  
  
int generateJPEG(BYTE* data,int w, int h, const char* outfilename)  
{  
    int nComponent = 3;  
  
    struct jpeg_compress_struct jcs;  

    struct jpeg_error_mgr jem;  
  
    jcs.err = jpeg_std_error(&jem);   
    jpeg_create_compress(&jcs);  
  
    FILE* f=fopen(outfilename,"wb");  
    if (f==NULL)   
    {  
        free(data);  
        return 0;  
    }  
    jpeg_stdio_dest(&jcs, f);  
    jcs.image_width = w;
    jcs.image_height = h;  
    jcs.input_components = nComponent;
    if (nComponent==1)  
        jcs.in_color_space = JCS_GRAYSCALE; 
    else   
        jcs.in_color_space = JCS_RGB;  
  
    jpeg_set_defaults(&jcs);      
    jpeg_set_quality (&jcs, 60, true);  
  
    jpeg_start_compress(&jcs, TRUE);  
  
    JSAMPROW row_pointer[1];
    int row_stride;
  
    row_stride = jcs.image_width*nComponent;
  
    while (jcs.next_scanline < jcs.image_height) {  
        row_pointer[0] = & data[jcs.next_scanline*row_stride];  
                                                                
        jpeg_write_scanlines(&jcs, row_pointer, 1);  
    }  
  
    jpeg_finish_compress(&jcs);  
    jpeg_destroy_compress(&jcs);  
    fclose(f);  
  
    return 1;  
}  
   
BYTE*  generateRGB24Data()  
{  
    struct {  
        BYTE r;  
        BYTE g;  
        BYTE b;  
    } pRGB[100][199]; 
  
    memset( pRGB, 0, sizeof(pRGB) );
      
    int i=0, j=0;  
    for(  i=50;i<70;i++ ){  
        for( j=70;j<140;j++ ){  
            pRGB[i][j].b = 0xff;  
        }  
    }  
    for(  i=0;i<10;i++ ){  
        for( j=0;j<199;j++ ){  
            pRGB[i][j].r = 0xff;  
        }  
    }  
  
    BYTE* ret = (BYTE*)malloc(sizeof(BYTE)*100*199*3);  
    memcpy(ret, (BYTE*)pRGB, sizeof(pRGB));  
    return ret;  
}  
  
void Java_pku_shengbin_hevplayer_LocalExploreActivity_generateJPG(JNIEnv* env, jobject thiz )  
{  
    BYTE* data = generateRGB24Data();  
    generateJPEG(data,199, 100, "/sdcard/test.jpg");  
  
    free(data);  
} 