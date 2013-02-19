//**********************************************
//dsp.h
//Unipipy @2011
//transform & mc
//**********************************************

#ifndef DECODER_HEVCDSP_H
#define DECODER_HEVCDSP_H

#define HPEL_SIZE	80

#define CHROMA_DCT 35

enum {
	SIDE_TOP	=1,
	SIDE_BOTTOM	=2,
	SIDE_MAX,
};

#define EDGE_WIDTH					80
#define EDGE_TOP					80
#define EDGE_BOTTOM					80

#define DRAW_EDGE_DELAY				8

typedef struct DSPContext{
	//idct
	void (*add4x4_idct)  ( pixel *p_src, int i_src, dctcoef dct[16], int i_mode );
	void (*add8x8_idct)  ( pixel *p_src, int i_src, dctcoef dct[64] );
	void (*add16x16_idct)( pixel *p_src, int i_src, dctcoef dct[256] );
	void (*add32x32_idct)( pixel *p_src, int i_src, dctcoef dct[1024] );

	//mc
	void (*draw_edge)	 ( pixel *p_dst, int drawheight, int picwidth, int stride, int sides, int shift);
	
	void (*mc_luma)( pixel *dst, int i_dst, pixel *src, int16_t hpx[3][HPEL_SIZE*HPEL_SIZE], int i_src,
		int mvx, int mvy, int i_width, int i_height );
	int16_t* (*get_ref)( int16_t *dst, int *i_dst, pixel *src, int16_t hpx[3][HPEL_SIZE*HPEL_SIZE], int i_src,
		int mvx, int mvy, int i_width, int i_height );
	void (*mc_chroma)( pixel *dst, int i_dst, pixel *src, int i_src,
		int mvx, int mvy, int i_width, int i_height );
	void (*get_ref_chroma)( int16_t *dst, int i_dst, pixel *src, int i_src,
		int mvx, int mvy, int i_width, int i_height );
	void (*hpel_filter)( int16_t *dst1, int16_t *dst2, int16_t *dst3, pixel *src,
		int i_stride, int i_width, int i_height, int selectflag );

	void (*mc_avg)( pixel *dst, int i_dst,
		int16_t *src1, int i_src1, int16_t *src2, int i_src2,
		int i_width, int i_height );
	void (*mc_weight_uni)( pixel *dst, int i_dst,
			int16_t *src, int i_src,
			int i_width, int i_height,
			int _w, int _offset, int _shift, int pixBitDepth);
	void (*mc_weight_bi)( pixel *dst, int i_dst,
			int16_t *src1, int i_src1, int16_t *src2, int i_src2,
			int i_width, int i_height,
			int _w0, int _w1, int _offset, int _shift, int pixBitDepth);
	//dequant
	void (*dequant)( dctcoef *dct, int qp, int i_log2_size);

}DSPContext;




/////////////////////////////////////////////////////////////////////////////////////////////////
//对外接口
/////////////////////////////////////////////////////////////////////////////////////////////////

void dsp_init(DSPContext *dsp,unsigned int machine);

#endif