//**********************************************
//deblock_x86.c
//Unipipy @2012
//deblock
//**********************************************
#include "config.h"

#if ARCH_X86_32
#include "smmintrin.h"

#include "../../internal.h"
#pragma warning(disable:4133)


extern const uint8_t tc_table[56];

extern const uint8_t beta_table[52];


static inline int cal_dp( pixel *src, int offset )
{
	int d = src[-offset*3] - 2*src[-offset*2] + src[-offset];
	return LENTABS( d );
}

static inline int cal_dq( pixel *src, int offset )
{
	int d = src[0] - 2*src[offset] + src[offset*2];
	return LENTABS( d );
}

/******** 基本函数及封装 ********/
static const __m128i zero=
{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
static const __m128i strong[8]=
{
	{8,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	{2,0,3,0,1,0,1,0,1,0,0,0,0,0,0,0},
	{0,0,2,0,2,0,2,0,2,0,0,0,0,0,0,0},
	{0,0,1,0,2,0,2,0,2,0,1,0,0,0,0,0},
	{0,0,0,0,1,0,2,0,2,0,2,0,1,0,0,0},
	{0,0,0,0,0,0,2,0,2,0,2,0,2,0,0,0},
	{0,0,0,0,0,0,1,0,1,0,1,0,3,0,2,0},
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,8,0},
};

//这是8bit的转置
#define TRANSPOSE8x4_LOAD(src,stride) \
	do{\
	x[0]=_mm_unpacklo_epi8(_mm_loadl_epi64(src+stride*0),zero);\
	x[1]=_mm_unpacklo_epi8(_mm_loadl_epi64(src+stride*1),zero);\
	x[2]=_mm_unpacklo_epi8(_mm_loadl_epi64(src+stride*2),zero);\
	x[3]=_mm_unpacklo_epi8(_mm_loadl_epi64(src+stride*3),zero);\
	x[4]=_mm_setzero_si128();\
	x[5]=_mm_setzero_si128();\
	x[6]=_mm_setzero_si128();\
	x[7]=_mm_setzero_si128();\
	}while(0)

#define BUTTERFLY(bit,A,B,tmp)\
{\
	tmp=A;\
	A=_mm_unpacklo_epi##bit(A,B);\
	B=_mm_unpackhi_epi##bit(tmp,B);\
	}
#define SWAP(A,B,tmp)\
{\
	tmp=A;\
	A=B;\
	B=tmp;\
	}
#define TRANSPOSE8x8_CORE() \
	do{\
	\
	BUTTERFLY(16,x[0],x[1],d[0]);/*x[0]=00 10 01 11 02 12 03 13  x[1]=04 14 05 15 06 16 07 17*/\
	BUTTERFLY(16,x[2],x[3],d[0]);/*x[2]=20 30 21 31 22 32 23 33  x[3]=24 34 25 35 26 36 27 37*/\
	BUTTERFLY(16,x[4],x[5],d[0]);/*x[4]=40 50 41 51 42 52 43 53  x[5]=44 54 45 55 46 56 47 57*/\
	BUTTERFLY(16,x[6],x[7],d[0]);/*x[6]=60 70 61 71 62 72 63 73  x[7]=64 74 65 75 66 76 67 77*/\
	\
	BUTTERFLY(32,x[0],x[2],d[0]);/*x[0]=00 10 20 30 01 11 21 31  x[2]=02 12 22 32 03 13 23 33*/\
	BUTTERFLY(32,x[4],x[6],d[0]);/*x[4]=40 50 60 70 41 51 61 71  x[6]=42 52 62 72 43 53 63 73*/\
	BUTTERFLY(32,x[1],x[3],d[0]);/*x[1]=04 14 24 34 05 15 25 35  x[3]=06 16 26 36 07 17 27 37*/\
	BUTTERFLY(32,x[5],x[7],d[0]);/*x[5]=44 54 64 74 45 55 65 75  x[7]=46 56 66 76 47 57 67 77*/\
	\
	BUTTERFLY(64,x[0],x[4],d[0]);/*x[0]=00 10 20 30 40 50 60 70  x[4]=01 11 21 31 41 51 61 71*/\
	BUTTERFLY(64,x[2],x[6],d[0]);/*x[2]=02 12 22 32 42 52 62 72  x[6]=03 13 23 33 43 53 63 73*/\
	BUTTERFLY(64,x[1],x[5],d[0]);/*x[1]=04 14 24 34 44 54 64 74  x[5]=05 15 25 35 45 55 65 75*/\
	BUTTERFLY(64,x[3],x[7],d[0]);/*x[3]=06 16 26 36 46 56 66 76  x[7]=07 17 27 37 47 57 67 77*/\
	\
	SWAP(x[4],x[1],d[0]);\
	SWAP(x[6],x[3],d[0]);\
	}while(0)
#define TRANSPOSE8x4_STORE(dst,stride) \
	do{\
		_mm_storel_epi64(dst+0*stride,_mm_packus_epi16(x[0],x[0]));\
		_mm_storel_epi64(dst+1*stride,_mm_packus_epi16(x[1],x[1]));\
		_mm_storel_epi64(dst+2*stride,_mm_packus_epi16(x[2],x[2]));\
		_mm_storel_epi64(dst+3*stride,_mm_packus_epi16(x[3],x[3]));\
	}while(0)

#define TRANSPOSE8x8(dst,stride_dst,src,stride_src)\
	do{\
	TRANSPOSE8x8_LOAD(src,stride_src);\
	TRANSPOSE8x8_CORE();\
	TRANSPOSE8x8_STORE(dst,stride_dst);\
	}while(0)

#define PIX(x) pix[(x)*xstride]
#define JUD(x) judge[(x)*xstride]
static inline int b_strong_filter( pixel *judge, int xstride, int d, int beta, int tc )
{
	pixel m4 = JUD( 0);
	pixel m3 = JUD(-1);
	pixel m7 = JUD( 3);
	pixel m0 = JUD(-4);

	int d_strong = LENTABS_INT( m0 - m3 ) + LENTABS_INT( m7 - m4 );

	return (d_strong < (beta>>3)) && (d<(beta>>2)) && 
		(LENTABS_INT(m3-m4) < ((tc*5+1)>>1));
}


//
// SSE2
//

// 绝对值：负数取绝对值为减一取反(或者是取反加一)
#define SSE2_ABS_EPI(bit, v)	_mm_xor_si128(			\
	_mm_add_epi##bit(									\
		(v),											\
		_mm_cmplt_epi##bit((v), _mm_setzero_si128())	\
	),													\
	_mm_cmplt_epi##bit((v), _mm_setzero_si128())		\
)
#define SSE2_ABS_EPI16(v)	SSE2_ABS_EPI(16, v)
#define SSE2_ABS_EPI32(v)	SSE2_ABS_EPI(32, v)
#define SSE2_ABS_EPI64(v)	SSE2_ABS_EPI(64, v)

#define _MM_SHUFFLE4(d,c,b,a) (((d)<<6) | ((c)<<4) | ((b)<<2) | (a))

static void deblock_v_luma_sse2( pixel *pix, pixel *judge, int stride, int beta, int tc0 )
{
	int i_side_thresh = (beta+(beta>>1))>>3;
	int tc = tc0;
	int i_tc_thresh = tc * 10;
	int tcd2=tc >> 1, tcm2=tc << 1;
	int dp, dq, d0, d3, dt;
	int dp0, dp3, dq0, dq3;
	int i;
	__m128i x[8],d[8],mask,delta;

/*
	dp0 = cal_dp( judge + 0*1, stride );
	dq0 = cal_dq( judge + 0*1, stride );
	dp3 = cal_dp( judge + 3*1, stride );
	dq3 = cal_dq( judge + 3*1, stride );

	d0 = dp0 + dq0;
	d3 = dp3 + dq3;
	dp = dp0 + dp3;
	dq = dq0 + dq3;

	dt = d0 + d3;
*/
	{	// vertical filtering for horizontal edges
		__m128i x, xmm_dp, xmm_dq;
		// dp
		xmm_dp = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(judge-3*stride)), _mm_setzero_si128()); // [x70|x60|x50|x40|x30|x20|x10|x00]
		x = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(judge-2*stride)), _mm_setzero_si128()); // [x71|x61|x51|x41|x31|x21|x11|x01]
		xmm_dp = _mm_sub_epi16(xmm_dp, x);
		xmm_dp = _mm_sub_epi16(xmm_dp, x);
		x = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(judge-1*stride)), _mm_setzero_si128());
		xmm_dp = _mm_add_epi16(xmm_dp, x);
		xmm_dp = SSE2_ABS_EPI16(xmm_dp);
		xmm_dp = _mm_unpacklo_epi16(xmm_dp, _mm_setzero_si128()); // to 32bit
		dp0 = _mm_cvtsi128_si32(xmm_dp);
		xmm_dp = _mm_srli_si128(xmm_dp, 12);
		dp3 = _mm_cvtsi128_si32(xmm_dp);
		// dq
		xmm_dq = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(judge+0*stride)), _mm_setzero_si128());
		x = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(judge+1*stride)), _mm_setzero_si128());
		xmm_dq = _mm_sub_epi16(xmm_dq, x);
		xmm_dq = _mm_sub_epi16(xmm_dq, x);
		x = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(judge+2*stride)), _mm_setzero_si128());
		xmm_dq = _mm_add_epi16(xmm_dq, x);
		xmm_dq = SSE2_ABS_EPI16(xmm_dq);
		xmm_dq = _mm_unpacklo_epi16(xmm_dq, _mm_setzero_si128()); // to 32bit
		dq0 = _mm_cvtsi128_si32(xmm_dq);
		xmm_dq = _mm_srli_si128(xmm_dq, 12);
		dq3 = _mm_cvtsi128_si32(xmm_dq);
		//
		d0 = dp0 + dq0;
		d3 = dp3 + dq3;
		dp = dp0 + dp3;
		dq = dq0 + dq3;
		dt = d0 + d3;
	}
	// end

	if( dt < beta )
	{
		int b_filter_p, b_filter_q, b_strong;

		b_filter_p = dp < i_side_thresh;
		b_filter_q = dq < i_side_thresh;
/*
		b_strong = b_strong_filter( judge    , stride, 2*d0, beta, tc )
				&& b_strong_filter( judge+3*1, stride, 2*d3, beta, tc );
*/
/*
#define PIX(x) pix[(x)*xstride]
#define JUD(x) judge[(x)*xstride]
static inline int b_strong_filter( pixel *judge, int xstride, int d, int beta, int tc )
{
	pixel m4 = JUD( 0);
	pixel m3 = JUD(-1);
	pixel m7 = JUD( 3);
	pixel m0 = JUD(-4);

	int d_strong = LENTABS_INT( m0 - m3 ) + LENTABS_INT( m7 - m4 );

	return (d_strong < (beta>>3)) && (d<(beta>>2)) && 
		(LENTABS_INT(m3-m4) < ((tc*5+1)>>1));
}
*/
		{
			__m128i x03lo, x03hi, x03, x0347, x347, x04, ds03;

			x03lo = _mm_unpacklo_epi8(_mm_unpacklo_epi32(_mm_loadl_epi64((__m128i*)(judge-4*stride)), _mm_loadl_epi64((__m128i*)(judge-1*stride))), _mm_setzero_si128()); // [x33|x23|x13|x03|x30|x20|x10|x00]
			x03lo = _mm_shufflelo_epi16(x03lo, _MM_SHUFFLE4(2, 3, 1, 0)); // [x33|x23|x13|x03|x20|x30|x10|x00]
			x03lo = _mm_shufflehi_epi16(x03lo, _MM_SHUFFLE4(2, 3, 1, 0)); // [x23|x33|x13|x03|x20|x30|x10|x00]
			x03lo = _mm_and_si128(x03lo, _mm_set1_epi32(0x0000FFFF)); // [00|x33|00|x03|00|x30|00|x00]
			x03hi = _mm_unpacklo_epi8(_mm_unpacklo_epi32(_mm_loadl_epi64((__m128i*)(judge+0*stride)), _mm_loadl_epi64((__m128i*)(judge+3*stride))), _mm_setzero_si128()); // [x37|x27|x17|x07|x34|x24|x14|x04]
			x03hi = _mm_shufflelo_epi16(x03hi, _MM_SHUFFLE4(2, 3, 1, 0)); // [x37|x27|x17|x07|x24|x34|x14|x04]
			x03hi = _mm_shufflehi_epi16(x03hi, _MM_SHUFFLE4(2, 3, 1, 0)); // [x27|x37|x17|x07|x24|x34|x14|x04]
			x03hi = _mm_and_si128(x03hi, _mm_set1_epi32(0x0000FFFF)); // [00|x37|00|x07|00|x34|00|x04]
			x03 = _mm_packs_epi32(x03lo, x03hi); // [x37|x07|x34|x04|x33|x03|x30|x00]
			x0347 = _mm_shufflelo_epi16(x03, _MM_SHUFFLE4(3,1,2,0)); // [x37|x07|x34|x04|x33|x30|x03|x00]
			x0347 = _mm_shufflehi_epi16(x0347, _MM_SHUFFLE4(3,1,2,0)); // [x37|x34|x07|x04|x33|x30|x03|x00]
			x0347 = _mm_shuffle_epi32(x0347, _MM_SHUFFLE4(3,1,2,0)); // >>> [x37|x34|x33|x30|x07|x04|x03|x00] <<<
			x347 = _mm_srli_si128(x0347, 2); // epi16: [000|x37|x34|x33|x30|x07|x04|x03]
			x0347 = _mm_sub_epi16(x0347, x347);
			x0347 = SSE2_ABS_EPI16(x0347); // [x37-000|x34-x37|x33-x34|x30-x33|x07-x30|x04-x07|x03-x04|x00-x03]
			x04 = _mm_srli_si128(x0347, 4); // [000|000|x37-000|x34-x37|x33-x34|x30-x33|x07-x30|x04-x07]
			ds03 = _mm_add_epi16(x0347, x04); // [x|x|x|ds3|x|x|x|ds0]
			b_strong = (_mm_extract_epi16(ds03, 0) < (beta>>3)) && (_mm_extract_epi16(ds03, 4) < (beta>>3));
			b_strong = b_strong && (_mm_extract_epi16(x0347, 1) < ((tc*5+1)>>1)) && (_mm_extract_epi16(x0347, 5) < ((tc*5+1)>>1));
			b_strong = b_strong && ((2*d0) < (beta>>2)) && ((2*d3) < (beta>>2));
#ifdef _DEBUG
			if ( 1 ) {
				int b_strong_right;
				b_strong_right = b_strong_filter( judge    , stride, 2*d0, beta, tc )
							  && b_strong_filter( judge+3*1, stride, 2*d3, beta, tc );
				if ( b_strong != b_strong_right ) {
					lent_log(NULL, LENT_LOG_ERROR, "deblock_v_luma_sse2 b_strong error, right=%d, opt=%d!\n", b_strong_right, b_strong);
				}
			}
#endif
		}

		if(!b_strong)	// weak filtering
		{
			pixel *p=pix-3*stride;
			// for(i=0;i<8;i++)
			for ( i = 1; i < 7; i++ )
			{
				x[i]=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)p),zero);
				p+=stride;
			}

			delta=_mm_mullo_epi16(_mm_sub_epi16(x[4],x[3]),_mm_set1_epi16(9));
			delta=_mm_sub_epi16(delta,_mm_mullo_epi16(_mm_sub_epi16(x[5],x[2]),_mm_set1_epi16(3)));
			delta=_mm_add_epi16(delta,_mm_set1_epi16(8));
			delta=_mm_srai_epi16(delta,4);
			mask=_mm_cmplt_epi16(SSE2_ABS_EPI16(delta),_mm_set1_epi16(i_tc_thresh));
			mask=_mm_unpacklo_epi64(mask,_mm_setzero_si128());//only consider the low 64 bit(4 coloums)
			if(!_mm_movemask_epi8 (mask))
				return;

			delta=_mm_max_epi16(delta,_mm_set1_epi16(-tc));
			delta=_mm_min_epi16(delta,_mm_set1_epi16(tc));

			d[2]=_mm_add_epi16(x[3],delta);
			d[3]=_mm_sub_epi16(x[4],delta);

			if(b_filter_p)
			{
				__m128i delta1;

				delta1 = _mm_avg_epu16(x[1], x[3]);
				delta1=_mm_sub_epi16(delta1,x[2]);
				delta1=_mm_add_epi16(delta1,delta);
				delta1=_mm_srai_epi16(delta1,1);

				delta1=_mm_min_epi16(delta1,_mm_set1_epi16(tcd2));
				delta1=_mm_max_epi16(delta1,_mm_set1_epi16(-tcd2));

				d[0]=_mm_add_epi16(x[2],delta1);

				d[0]=_mm_and_si128(d[0],mask);
				x[2]=_mm_andnot_si128(mask,x[2]);
				x[2]=_mm_or_si128(d[0],x[2]);
			}
			if(b_filter_q)
			{
				__m128i delta1;

				delta1 = _mm_avg_epu16(x[4], x[6]);
				delta1=_mm_sub_epi16(delta1,x[5]);
				delta1=_mm_sub_epi16(delta1,delta);
				delta1=_mm_srai_epi16(delta1,1);

				delta1=_mm_min_epi16(delta1,_mm_set1_epi16(tcd2));
				delta1=_mm_max_epi16(delta1,_mm_set1_epi16(-tcd2));

				d[0]=_mm_add_epi16(x[5],delta1);

				d[0]=_mm_and_si128(d[0],mask);
				x[5]=_mm_andnot_si128(mask,x[5]);
				x[5]=_mm_or_si128(d[0],x[5]);
			}

			d[2]=_mm_and_si128(d[2],mask);
			d[3]=_mm_and_si128(d[3],mask);

			x[3]=_mm_andnot_si128(mask,x[3]);
			x[4]=_mm_andnot_si128(mask,x[4]);

			x[3]=_mm_or_si128(d[2],x[3]);
			x[4]=_mm_or_si128(d[3],x[4]);

			p=pix-2*stride;
			for(i=2;i<6;i++)
			{
				//_mm_storel_epi64(p,_mm_packus_epi16(x[i],x[i]));
				*((int*)p)=_mm_cvtsi128_si32(_mm_packus_epi16(x[i],x[i]));
				p+=stride;
			}
		}
		else // strong filter
		{
			// test
//			static int strong_v_filter_times = 0;
			// test end
			pixel *p = pix - 4*stride;
			__m128i x07, x16, x25, x52, x34, x43;
			__m128i d16, d25, d34, t;
			__m128i mIn=_mm_set1_epi16(-tcm2), mAx=_mm_set1_epi16(tcm2);
			// for line 0,7
			x[0] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(p + 0*stride)), _mm_setzero_si128());
			x[7] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(p + 7*stride)), _mm_setzero_si128());
			x07 = _mm_unpacklo_epi64(x[0], x[7]);
			d16 = _mm_slli_epi16(x07, 1);	// d16: {*2 3 1 1 1} + 4 / 8, d16 = x07 * 2
			// for line 1,6
			x[1] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(p + 1*stride)), _mm_setzero_si128());
			x[6] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(p + 6*stride)), _mm_setzero_si128());
			x16 = _mm_unpacklo_epi64(x[1], x[6]);
			d16 = _mm_add_epi16(d16, _mm_add_epi16(x16, _mm_slli_epi16(x16, 1)));	// d16: {2 *3 1 1 1} + 4 / 8, d16 += x16 * 3
			d25 = x16;	// d25: 0 *1 1 1 1 / 4, d25 = x16
			d34 = x16;	// d34: 0 *1 2 2 2 1 / 8, d34 = x16
			// for line 2,5
			x[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(p + 2*stride)), _mm_setzero_si128());
			x[5] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(p + 5*stride)), _mm_setzero_si128());
			x25 = _mm_unpacklo_epi64(x[2], x[5]);
			x52 = _mm_unpacklo_epi64(x[5], x[2]);
			d16 = _mm_add_epi16(d16, x25);	// d16: {2 3 *1 1 1} + 4 / 8,	d16 += x25
			d25 = _mm_add_epi16(d25, x25);	// d25: {0 1 *1 1 1} + 2 / 4, d25 += x25
			d34 = _mm_add_epi16(d34, _mm_add_epi16(_mm_slli_epi16(x25, 1), x52));	// d34: {0 1 *2 2 2 *1} + 4 / 8, d34 = x25*2 + x52
			// for line 3,4
			x[3] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(p + 3*stride)), _mm_setzero_si128());
			x[4] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(p + 4*stride)), _mm_setzero_si128());
			x34 = _mm_unpacklo_epi64(x[3], x[4]);
			x43 = _mm_unpacklo_epi64(x[4], x[3]);
			t = _mm_add_epi16(x34, x43);
			d16 = _mm_add_epi16(d16, t);	// d16: {2 3 1 *1 *1} + 4 / 8, d16 += x34 + x43
			d25 = _mm_add_epi16(d25, t);	// // d25: {0 1 1 *1 *1} + 4 / 4, d25 += x34 + x43
			d34 = _mm_add_epi16(d34, _mm_slli_epi16(t, 1));	// d34: {0 1 2 *2 *2 1} + 4 / 8, d34 += x34*2 + x43*2
			// round division
			t = _mm_set1_epi16(2);
			d25 = _mm_srai_epi16(_mm_add_epi16(d25, t), 2);	// // d25: {0 1 1 1 1} + *2 / *4, d25 = (d25 + 2) / 4
			t = _mm_add_epi16(t, t);
			d16 = _mm_srai_epi16(_mm_add_epi16(d16, t), 3);	// d16: {2 3 1 1 1} + *4 / *8, d16 = (d16 + 4) / 8
			d34 = _mm_srai_epi16(_mm_add_epi16(d34, t), 3);	// d34: {0 1 2 2 2 1} + *4 / *8, d34 = (d34 + 4) / 8
			// clip
			t=_mm_sub_epi16(d16, x16);
			t=_mm_min_epi16(t, mAx);
			t=_mm_max_epi16(t, mIn);
			d16 = _mm_add_epi16(x16, t);
			t=_mm_sub_epi16(d25, x25);
			t=_mm_min_epi16(t, mAx);
			t=_mm_max_epi16(t, mIn);
			d25 = _mm_add_epi16(x25, t);
			t=_mm_sub_epi16(d34, x34);
			t=_mm_min_epi16(t, mAx);
			t=_mm_max_epi16(t, mIn);
			d34 = _mm_add_epi16(x34, t);
			// store
			*(uint32_t*)(p + 1*stride) = _mm_cvtsi128_si32(_mm_packus_epi16(d16, d16));
			*(uint32_t*)(p + 6*stride) = _mm_cvtsi128_si32(_mm_packus_epi16(_mm_unpackhi_epi64(d16, d16), _mm_setzero_si128()));
			*(uint32_t*)(p + 2*stride) = _mm_cvtsi128_si32(_mm_packus_epi16(d25, d25));
			*(uint32_t*)(p + 5*stride) = _mm_cvtsi128_si32(_mm_packus_epi16(_mm_unpackhi_epi64(d25, d25), _mm_setzero_si128()));
			*(uint32_t*)(p + 3*stride) = _mm_cvtsi128_si32(_mm_packus_epi16(d34, d34));
			*(uint32_t*)(p + 4*stride) = _mm_cvtsi128_si32(_mm_packus_epi16(_mm_unpackhi_epi64(d34, d34), _mm_setzero_si128()));
		}

	}
}

static void deblock_h_luma_sse2( pixel *pix, pixel *judge, int stride, int beta, int tc0 )
{
	int i_side_thresh;
	int tc, i_tc_thresh, tcd2, tcm2;
	int dp, dq, d0, d3, dt;

	// test
#ifdef _DEBUG
	static int strong_h_filter_times = 0;
	++strong_h_filter_times;
	if ( 1889 == strong_h_filter_times ) {
		lent_log(NULL, LENT_LOG_VERBOSE, "deblock_h_luma_sse2 hit %d times\n", strong_h_filter_times);
	}
#endif
	// test end

	i_side_thresh = (beta+(beta>>1))>>3;
	tc = tc0;
	i_tc_thresh = tc * 10;
	tcd2=tc >> 1;
	tcm2=tc << 1;

	// modify by James.DF, 2012.11.29
	{	// horizontal filtering for vertical edges

		//int dp0 = cal_dp( judge + 0*stride, 1 );
		//int dq0 = cal_dq( judge + 0*stride, 1 );
		//int dp3 = cal_dp( judge + 3*stride, 1 );
		//int dq3 = cal_dq( judge + 3*stride, 1 );
		//d0 = dp0 + dq0;
		//d3 = dp3 + dq3;
		//dp = dp0 + dp3;
		//dq = dq0 + dq3;
		//dt = d0 + d3;

		// dp = | x1 - 2*x2 + x3 |, dq = | x4 - 2*x5 + x6 |
		const __m128i coeff = {0, 0,  1, 0,   -2, -1,   1, 0,   1, 0,   -2, -1,   1, 0,   0, 0};
		__m128i x0, x3, xmm_d0, xmm_d3, xmm_dpq, xmm_d;
		
		x0 = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(judge+stride*0-4)), _mm_setzero_si128());	// [x7|x6|x5|x4|x3|x2|x1|x0]
		xmm_d0 = _mm_madd_epi16(x0, coeff); // [0*x7+1*x6|-2*x5+1*x4|1*x3+-2*x2|1*x1+0*x0]
		xmm_d0 = _mm_add_epi32(xmm_d0, _mm_shuffle_epi32(xmm_d0, _MM_SHUFFLE4(2,3,0,1))); // [0*x7+1*x6 + -2*x5+1*x4|0*x7+1*x6 + -2*x5+1*x4|1*x3+-2*x2 + 1*x1+0*x0|1*x3+-2*x2 + 1*x1+0*x0]
		xmm_d0 = SSE2_ABS_EPI32(xmm_d0); // [dq0|dq0|dp0|dp0]

		x3 = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(judge+stride*3-4)), _mm_setzero_si128());	// [x7|x6|x5|x4|x3|x2|x1|x0]
		xmm_d3 = _mm_madd_epi16(x3, coeff); // [0*x7+1*x6|-2*x5+1*x4|1*x3+-2*x2|1*x1+0*x0]
		xmm_d3 = _mm_add_epi32(xmm_d3, _mm_shuffle_epi32(xmm_d3, _MM_SHUFFLE4(2,3,0,1))); // [0*x7+1*x6 + -2*x5+1*x4|0*x7+1*x6 + -2*x5+1*x4|1*x3+-2*x2 + 1*x1+0*x0|1*x3+-2*x2 + 1*x1+0*x0]
		xmm_d3 = SSE2_ABS_EPI32(xmm_d3); // [dq3|dq3|dp3|dp3]

		xmm_dpq = _mm_add_epi32(xmm_d0, xmm_d3); // [dq0+dq3|dq0+dq3|dp0+dp3|dp0+dp3] = [dq|dq|dp|dp]
		dp = _mm_cvtsi128_si32(xmm_dpq);
		xmm_dpq = _mm_unpackhi_epi64(xmm_dpq, xmm_dpq); // [dq|dq|dq|dq]
		dq = _mm_cvtsi128_si32(xmm_dpq);

		xmm_d0 = _mm_add_epi32(xmm_d0, _mm_srli_si128(xmm_d0, 8)); // [dq0+0|dq0+0|dp0+dq0|dp0+dq0]
		d0 = _mm_cvtsi128_si32(xmm_d0);
		xmm_d3 = _mm_add_epi32(xmm_d3, _mm_srli_si128(xmm_d3, 8)); // [dq3+0|dq3+0|dp3+dq3|dp3+dq3]
		d3 = _mm_cvtsi128_si32(xmm_d3);

		xmm_d = _mm_add_epi32(xmm_d0, xmm_d3); // [dq0+dq3|dq0+dq3|dp0+dq0+dp3+dq3|dp0+dq0+dp3+dq3] = [dq|dq|d|d]
		dt = _mm_cvtsi128_si32(xmm_d);

#ifdef _DEBUG
		if ( 1 ) {
			int dp0 = cal_dp( judge + 0*stride, 1 );
			int dq0 = cal_dq( judge + 0*stride, 1 );
			int dp3 = cal_dp( judge + 3*stride, 1 );
			int dq3 = cal_dq( judge + 3*stride, 1 );
			int d0r = dp0 + dq0;
			int d3r = dp3 + dq3;
			int dpr = dp0 + dp3;
			int dqr = dq0 + dq3;
			int dtr = d0r + d3r;
			if ( d0 != d0r || d3 != d3r || dp != dpr || dq != dqr || dt != dtr ) {
				lent_log(NULL, LENT_LOG_ERROR, "deblock_h_luma_sse2 dt error at %d!\n", strong_h_filter_times);
			}
		}
#endif
	}
	// end

	if( dt < beta )
	{
		int b_filter_p, b_filter_q, b_strong;
		b_filter_p = dp < i_side_thresh;
		b_filter_q = dq < i_side_thresh;
/*
		b_strong =     b_strong_filter( judge         , 1, 2*d0, beta, tc )
					&& b_strong_filter( judge+3*stride, 1, 2*d3, beta, tc );
*/
		{
			__m128i x0, x3, x0347, x347, x04, ds03;
			x0 = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(judge+stride*0-4)), _mm_setzero_si128());	// [x7|x6|x5|x4|x3|x2|x1|x0]
			x0 = _mm_shufflelo_epi16(x0, _MM_SHUFFLE4(2,1,3,0)); // [x7|x6|x5|x4|x2|x1|x3|x0]
			x0 = _mm_shufflehi_epi16(x0, _MM_SHUFFLE4(2,1,3,0)); // [x6|x5|x7|x4|x2|x1|x3|x0]
			x0 = _mm_shuffle_epi32(x0, _MM_SHUFFLE4(3,1,2,0)); // [x6|x5|x2|x1|x7|x4|x3|x0]
			x3 = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(judge+stride*3-4)), _mm_setzero_si128());	// [x7|x6|x5|x4|x3|x2|x1|x0]
			x3 = _mm_shufflelo_epi16(x3, _MM_SHUFFLE4(2,1,3,0)); // [x7|x6|x5|x4|x2|x1|x3|x0]
			x3 = _mm_shufflehi_epi16(x3, _MM_SHUFFLE4(2,1,3,0)); // [x6|x5|x7|x4|x2|x1|x3|x0]
			x3 = _mm_shuffle_epi32(x3, _MM_SHUFFLE4(3,1,2,0)); // [x6|x5|x2|x1|x7|x4|x3|x0]
			x0347 = _mm_unpacklo_epi64(x0, x3); // [x37|x34|x33|x30|x07|x04|x03|x00]
			x347 = _mm_srli_si128(x0347, 2); // epi16: [000|x37|x34|x33|x30|x07|x04|x03]
			x0347 = _mm_sub_epi16(x0347, x347);
			x0347 = SSE2_ABS_EPI16(x0347); // [x37-000|x34-x37|x33-x34|x30-x33|x07-x30|x04-x07|x03-x04|x00-x03]
			x04 = _mm_srli_si128(x0347, 4); // [000|000|x37-000|x34-x37|x33-x34|x30-x33|x07-x30|x04-x07]
			ds03 = _mm_add_epi16(x0347, x04); // [x|x|x|ds3|x|x|x|ds0]
			b_strong = (_mm_extract_epi16(ds03, 0) < (beta>>3)) && (_mm_extract_epi16(ds03, 4) < (beta>>3));
			b_strong = b_strong && (_mm_extract_epi16(x0347, 1) < ((tc*5+1)>>1)) && (_mm_extract_epi16(x0347, 5) < ((tc*5+1)>>1));
			b_strong = b_strong && ((2*d0) < (beta>>2)) && ((2*d3) < (beta>>2));
#ifdef _DEBUG
			if ( 1 ) {
				int b_strong_right;
				b_strong_right = b_strong_filter( judge         , 1, 2*d0, beta, tc )
							  && b_strong_filter( judge+3*stride, 1, 2*d3, beta, tc );
				if ( b_strong != b_strong_right ) {
					lent_log(NULL, LENT_LOG_ERROR, "deblock_h_luma_sse2 b_strong error at %d, right=%d, opt=%d!\n", strong_h_filter_times, b_strong_right, b_strong);
				}
			}
#endif
		}

		if ( !b_strong ) { // weak filter
			__m128i x01, x23, x0123, a0123, a4567, a01, a23, a45, a67, a1, a2, a3, a4, a5, a6, d2, d3, d4, d5, d23, d45;
			__m128i t, delta, mask;
			int i_mask;

			// load & transpose
			x01 = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(pix-4 + 0*stride)), _mm_loadl_epi64((__m128i*)(pix-4 + 1*stride))); // [17|07|16|06|15|05|14|04|13|03|12|02|11|01|10|00]
			x23 = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(pix-4 + 2*stride)), _mm_loadl_epi64((__m128i*)(pix-4 + 3*stride))); // [37|27|36|26|35|25|34|24|33|23|32|22|31|21|30|20]
			a0123 = _mm_unpacklo_epi16(x01, x23); // [33|23|13|03|32|22|12|02|31|21|11|01|30|20|10|00]
			a4567 = _mm_unpackhi_epi16(x01, x23); // [37|27|17|07|36|26|16|06|35|25|15|05|34|24|14|04]
			a01 = _mm_unpacklo_epi8(a0123, _mm_setzero_si128()); // [31|21|11|01|30|20|10|00]
			a23 = _mm_unpackhi_epi8(a0123, _mm_setzero_si128()); // [33|23|13|03|32|22|12|02]
			a45 = _mm_unpacklo_epi8(a4567, _mm_setzero_si128()); // [35|25|15|05|34|24|14|04]
			a67 = _mm_unpackhi_epi8(a4567, _mm_setzero_si128()); // [37|27|17|07|36|26|16|06]
			a1 = _mm_unpackhi_epi64(a01, _mm_setzero_si128()); // [xxxx|31|21|11|01]
			a2 = _mm_unpacklo_epi64(a23, _mm_setzero_si128()); // [xxxx|32|22|12|02]
			a3 = _mm_unpackhi_epi64(a23, _mm_setzero_si128()); // [xxxx|33|23|13|03]
			a4 = _mm_unpacklo_epi64(a45, _mm_setzero_si128()); // [xxxx|34|24|14|04]
			a5 = _mm_unpackhi_epi64(a45, _mm_setzero_si128()); // [xxxx|35|25|15|05]
			a6 = _mm_unpacklo_epi64(a67, _mm_setzero_si128()); // [xxxx|36|26|16|06]
			d2 = a2;
			d3 = a3;
			d4 = a4;
			d5 = a5;

			// delta = (9*(src[4]-src[3]) - 3*(src[5]-src[2]) + 8)>>4;
			//       = 0*src[0] + 0*src[1] + 3*src[2] - 9*src[3] + 9*src[4] - 3*src[5] + 0*src[6] + 0*src[7]
			t = _mm_sub_epi16(a4, a3);
			delta = _mm_add_epi16(t, _mm_slli_epi16(t, 3));	// (a4 - a3) * (1 + 8)
			t = _mm_sub_epi16(a5, a2);
			t = _mm_add_epi16(t, _mm_slli_epi16(t, 1)); // (a5 - a2) * (1 + 2)
			delta = _mm_sub_epi16(delta, t); // 9*(src[4]-src[3]) - 3*(src[5]-src[2])
			delta = _mm_add_epi16(delta, _mm_set1_epi16(8));
			delta = _mm_srai_epi16(delta, 4);

			// if( LENTABS(delta) < i_tc_thresh )
			mask=_mm_cmplt_epi16(SSE2_ABS_EPI16(delta), _mm_set1_epi16(i_tc_thresh));
			i_mask = _mm_movemask_epi8(mask);
			if ( !i_mask )
				return;

			// delta = lent_clip( delta, -tc0, tc0);
			delta = _mm_max_epi16(delta, _mm_set1_epi16(-tc));
			delta = _mm_min_epi16(delta, _mm_set1_epi16(tc));
			// dst(3) = lent_clip_uint8(src[3]+delta);
			t = _mm_add_epi16(a3, delta);
			t = _mm_and_si128(mask, t);
			d3 = _mm_andnot_si128(mask, d3);
			d3 = _mm_or_si128(d3, t);
			// dst(4) = lent_clip_uint8(src[4]-delta);
			t = _mm_sub_epi16(a4, delta);
			t = _mm_and_si128(mask, t);
			d4 = _mm_andnot_si128(mask, d4);
			d4 = _mm_or_si128(d4, t);

			if ( b_filter_p ) {
				//delta1 = ( ((src[1]+src[3]+1)>>1) - src[2] + delta )>>1;
				//delta1 = lent_clip( delta1, -tcd2, tcd2 );
				//PIX(-2) = lent_clip_uint8(src[2]+delta1);
				__m128i delta1;
				delta1 = _mm_add_epi16(a1, a3);
				delta1 = _mm_add_epi16(delta1, _mm_set1_epi16(1));
				delta1 = _mm_srai_epi16(delta1, 1);
				delta1 = _mm_sub_epi16(delta1, a2);
				delta1 = _mm_add_epi16(delta1, delta);
				delta1 = _mm_srai_epi16(delta1, 1);
				delta1 = _mm_min_epi16(delta1, _mm_set1_epi16(tcd2));
				delta1 = _mm_max_epi16(delta1, _mm_set1_epi16(-tcd2));
				t = _mm_add_epi16(a2, delta1);
				t = _mm_and_si128(mask, t);
				d2 = _mm_andnot_si128(mask, d2);
				d2 = _mm_or_si128(d2, t);
			}

			if ( b_filter_q ) {
				//delta2 = ( ((src[4]+src[6]+1)>>1) - src[5] - delta )>>1;
				//delta2 = lent_clip( delta2, -tcd2, tcd2 );
				//PIX( 1) = lent_clip_uint8(src[5]+delta2);
				__m128i delta2;
				delta2 = _mm_add_epi16(a4, a6);
				delta2 = _mm_add_epi16(delta2, _mm_set1_epi16(1));
				delta2 = _mm_srai_epi16(delta2, 1);
				delta2 = _mm_sub_epi16(delta2, a5);
				delta2 = _mm_sub_epi16(delta2, delta);
				delta2 = _mm_srai_epi16(delta2, 1);
				delta2 = _mm_min_epi16(delta2, _mm_set1_epi16(tcd2));
				delta2 = _mm_max_epi16(delta2, _mm_set1_epi16(-tcd2));
				t = _mm_add_epi16(a5, delta2);
				t = _mm_and_si128(mask, t);
				d5 = _mm_andnot_si128(mask, d5);
				d5 = _mm_or_si128(d5, t);
			}

			// transpose & store
			d23 = _mm_unpacklo_epi16(d2, d3); // [33|32|23|22|13|12|03|02]
			d45 = _mm_unpacklo_epi16(d4, d5); // [35|34|25|24|15|14|05|04]
			x01 = _mm_unpacklo_epi32(d23, d45); // [15|14|13|12|05|04|03|02]
			x23 = _mm_unpackhi_epi32(d23, d45); // [35|34|33|32|25|24|23|22]
			x0123 = _mm_packus_epi16(x01, x23); // [35|34|33|32|25|24|23|22|15|14|13|12|05|04|03|02]

			// check
#ifdef _DEBUG
			if ( 1 ) {
				pixel* pix_src = pix;
				__m128i t = x0123;
				int i;
				for ( i = 0; i < 4; i++ )
				{
					int delta, delta1, delta2;
					pixel src[8], d[4];
					uint32_t dright, dopt;
					int j;
					for( j = 0; j < 8; j ++ )
						src[j] = pix_src[j-4];
					d[0] = src[2];
					d[1] = src[3];
					d[2] = src[4];
					d[3] = src[5];
					delta = (9*(src[4]-src[3]) - 3*(src[5]-src[2]) + 8)>>4;
					if( LENTABS(delta) < i_tc_thresh )
					{
						delta = lent_clip( delta, -tc0, tc0);
						d[1] = lent_clip_uint8(src[3]+delta);
						d[2] = lent_clip_uint8(src[4]-delta);

						if( b_filter_p )
						{
							delta1 = ( ((src[1]+src[3]+1)>>1) - src[2] + delta )>>1;
							delta1 = lent_clip( delta1, -tcd2, tcd2 );
							d[0] = lent_clip_uint8(src[2]+delta1);
						}

						if( b_filter_q )
						{
							delta2 = ( ((src[4]+src[6]+1)>>1) - src[5] - delta )>>1;
							delta2 = lent_clip( delta2, -tcd2, tcd2 );
							d[3] = lent_clip_uint8(src[5]+delta2);
						}
					}
					dright =  (((uint32_t)d[0]) <<  0)
							| (((uint32_t)d[1]) <<  8)
							| (((uint32_t)d[2]) << 16)
							| (((uint32_t)d[3]) << 24);
					dopt = _mm_cvtsi128_si32(t);
					t = _mm_srli_si128(t, 4);
					if ( dopt != dright ) {
						lent_log(NULL, LENT_LOG_ERROR, "deblock_h_luma_sse2 weak filter optimize error at %d, i=%d, right=0x%08x, opt=0x%08x!\n", strong_h_filter_times, i, dright, dopt);
					}
					pix_src += stride;
				}
			}
#endif
//			if ( i_mask & 0x0003 )
				*(uint32_t*)(pix-2 + 0*stride) = _mm_cvtsi128_si32(x0123);
//			if ( i_mask & 0x000C )
				*(uint32_t*)(pix-2 + 1*stride) = _mm_cvtsi128_si32(_mm_srli_si128(x0123, 4));
//			if ( i_mask & 0x0030 )
				*(uint32_t*)(pix-2 + 2*stride) = _mm_cvtsi128_si32(_mm_srli_si128(x0123, 8));
//			if ( i_mask & 0x00C0 )
				*(uint32_t*)(pix-2 + 3*stride) = _mm_cvtsi128_si32(_mm_srli_si128(x0123, 12));

		} else { // strong filter

			__m128i x01, x23, a0123, a4567, a01, a23, a45, a67, a14, a36;
			__m128i d0, d1, d2, d3, d4, d5, d6, d7, d01, d23, d45, d67;
			__m128i t, t02, t13, t46, t57, t01, t23, t45, t67, x0, x1, x2, x3;
			//__m128i delta, mask;
			//int i_mask;

			// load & transpose
			x01 = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(pix-4 + 0*stride)), _mm_loadl_epi64((__m128i*)(pix-4 + 1*stride))); // [17|07|16|06|15|05|14|04|13|03|12|02|11|01|10|00]
			x23 = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(pix-4 + 2*stride)), _mm_loadl_epi64((__m128i*)(pix-4 + 3*stride))); // [37|27|36|26|35|25|34|24|33|23|32|22|31|21|30|20]
			a0123 = _mm_unpacklo_epi16(x01, x23); // [33|23|13|03|32|22|12|02|31|21|11|01|30|20|10|00]
			a4567 = _mm_unpackhi_epi16(x01, x23); // [37|27|17|07|36|26|16|06|35|25|15|05|34|24|14|04]
			a01 = _mm_unpacklo_epi8(a0123, _mm_setzero_si128()); // [31|21|11|01|30|20|10|00]
			a23 = _mm_unpackhi_epi8(a0123, _mm_setzero_si128()); // [33|23|13|03|32|22|12|02]
			a45 = _mm_unpacklo_epi8(a4567, _mm_setzero_si128()); // [35|25|15|05|34|24|14|04]
			a67 = _mm_unpackhi_epi8(a4567, _mm_setzero_si128()); // [37|27|17|07|36|26|16|06]
			a14 = _mm_unpacklo_epi64(_mm_unpackhi_epi64(a01, _mm_setzero_si128()), a45); // [34|24|14|04|31|21|11|01]
			a36 = _mm_unpacklo_epi64(_mm_unpackhi_epi64(a23, _mm_setzero_si128()), a67); // [36|26|16|06|33|23|13|03]

			//dst(0) = src[0]
			d0 = _mm_unpacklo_epi8(a0123, _mm_setzero_si128()); // [xxxx|30|20|10|00]
			d0 = _mm_slli_epi16(d0, 3);
			//dst(7) = src[7]
			d7 = _mm_unpackhi_epi8(_mm_srli_epi64(a4567, 32), _mm_setzero_si128()); // [xxxx|37|27|17|07]
			d7 = _mm_slli_epi16(d7, 3);

			//dst(1) = lent_clip( ((int)2*src[0] + 3*src[1] + src[2] + src[3] + src[4] + 4)>>3, src[1]-tcm2, src[1]+tcm2 );
			d1 = _mm_add_epi16(a01, a01); // [a1*2|a0*2]
			d1 = _mm_add_epi16(d1, a23); // [a1*2+a3|a0*2+a2]
			d1 = _mm_add_epi16(d1, a14); // [a1*2+a3+a4|a0*2+a2+a1]
			t = _mm_unpackhi_epi64(d1, d1); // [xxxx|a1*2+a3+a4]
			d1 = _mm_add_epi16(d1, t); // [xxxx|d31|d21|d11|d01]
			//dst(6) = lent_clip( ((int)src[3] + src[4] + src[5] + 3*src[6] + 2*src[7] + 4)>>3, src[6]-tcm2, src[6]+tcm2 );
			d6 = _mm_add_epi16(a67, a67); // [a7*2|a6*2]
			d6 = _mm_add_epi16(d6, a45); // [a7*2+a5|a6*2+a4]
			d6 = _mm_add_epi16(d6, a36); // [a7*2+a5+a6|a6*2+a4+a3]
			t = _mm_unpackhi_epi64(d6, d6); // [xxxx|a7*2+a5+a6]
			d6 = _mm_add_epi16(d6, t); // [xxxx|d36|d26|d16|d06]

			//dst(2) = lent_clip( ((int)src[1] + src[2] + src[3] + src[4] + 2)>>2, src[2]-tcm2, src[2]+tcm2 );
			d2 = _mm_add_epi16(a23, a14);
			d2 = _mm_add_epi16(d2, d2);
			t = _mm_unpackhi_epi64(d2, d2);
			d2 = _mm_add_epi16(d2, t); // [xxxx|d32|d22|d12|d02]
			//dst(5) = lent_clip( ((int)src[3] + src[4] + src[5] + src[6] + 2)>>2, src[5]-tcm2, src[5]+tcm2 );
			d5 = _mm_add_epi16(a45, a36);
			d5 = _mm_add_epi16(d5, d5);
			t = _mm_unpackhi_epi64(d5, d5);
			d5 = _mm_add_epi16(d5, t); // [xxxx|d35|d25|d15|d05]

			//dst(3) = lent_clip( ((int)src[1] + 2*src[2] + 2*src[3] + 2*src[4] + src[5] + 4)>>3, src[3]-tcm2, src[3]+tcm2 );
			d3 = _mm_add_epi16(a23, a23);
			d3 = _mm_add_epi16(d3, a45);
			d3 = _mm_add_epi16(d3, a14);
			t = _mm_unpackhi_epi64(d3, d3);
			d3 = _mm_add_epi16(d3, t); // [xxxx|d33|d23|d13|d03]
			//dst(4) = lent_clip( ((int)src[2] + 2*src[3] + 2*src[4] + 2*src[5] + src[6] + 4)>>3, src[4]-tcm2, src[4]+tcm2 );
			d4 = _mm_add_epi16(a45, a45);
			d4 = _mm_add_epi16(d4, a23);
			d4 = _mm_add_epi16(d4, a36);
			t = _mm_unpackhi_epi64(d4, d4);
			d4 = _mm_add_epi16(d4, t); // [xxxx|d34|d24|d14|d04]

			// clip
			d01 = _mm_unpacklo_epi64(d0, d1);
			d01 = _mm_add_epi16(d01, _mm_set1_epi16(4));
			d01 = _mm_srai_epi16(d01, 3);
			t = _mm_sub_epi16(d01, a01);
			t = _mm_min_epi16(t, _mm_set1_epi16(tcm2));
			t = _mm_max_epi16(t, _mm_set1_epi16(-tcm2));
			d01 = _mm_add_epi16(a01, t); // [31|21|11|01|30|20|10|00]
			d23 = _mm_unpacklo_epi64(d2, d3);
			d23 = _mm_add_epi16(d23, _mm_set1_epi16(4));
			d23 = _mm_srai_epi16(d23, 3);
			t = _mm_sub_epi16(d23, a23);
			t = _mm_min_epi16(t, _mm_set1_epi16(tcm2));
			t = _mm_max_epi16(t, _mm_set1_epi16(-tcm2));
			d23 = _mm_add_epi16(a23, t); // [33|23|13|03|32|22|12|02]
			d45 = _mm_unpacklo_epi64(d4, d5);
			d45 = _mm_add_epi16(d45, _mm_set1_epi16(4));
			d45 = _mm_srai_epi16(d45, 3);
			t = _mm_sub_epi16(d45, a45);
			t = _mm_min_epi16(t, _mm_set1_epi16(tcm2));
			t = _mm_max_epi16(t, _mm_set1_epi16(-tcm2));
			d45 = _mm_add_epi16(a45, t); // [35|25|15|05|34|24|14|04]
			d67 = _mm_unpacklo_epi64(d6, d7);
			d67 = _mm_add_epi16(d67, _mm_set1_epi16(4));
			d67 = _mm_srai_epi16(d67, 3);
			t = _mm_sub_epi16(d67, a67);
			t = _mm_min_epi16(t, _mm_set1_epi16(tcm2));
			t = _mm_max_epi16(t, _mm_set1_epi16(-tcm2));
			d67 = _mm_add_epi16(a67, t); // [37|27|17|07|36|26|16|06]

			// transpose & store
			t02 = _mm_unpacklo_epi16(d01, d23); // [32|30|22|20|12|10|02|00]
			t13 = _mm_unpackhi_epi16(d01, d23); // [33|31|23|21|13|11|03|01]
			t46 = _mm_unpacklo_epi16(d45, d67); // [36|34|26|24|16|14|06|04]
			t57 = _mm_unpackhi_epi16(d45, d67); // [37|35|27|25|17|15|07|05]
			t01 = _mm_unpacklo_epi16(t02, t13); // [13|12|11|10|03|02|01|00]
			t23 = _mm_unpackhi_epi16(t02, t13); // [33|32|31|30|23|22|21|20]
			t45 = _mm_unpacklo_epi16(t46, t57); // [17|16|15|14|07|06|05|04]
			t67 = _mm_unpackhi_epi16(t46, t57); // [37|36|35|34|27|26|25|24]
			x0 = _mm_unpacklo_epi64(t01, t45); // [07|06|05|04|03|02|01|00]
			x1 = _mm_unpackhi_epi64(t01, t45); // [17|16|15|14|13|12|11|10]
			x2 = _mm_unpacklo_epi64(t23, t67); // [27|26|25|24|23|22|21|20]
			x3 = _mm_unpackhi_epi64(t23, t67); // [37|36|35|34|33|32|31|30]
			x0 = _mm_packus_epi16(x0, x0); // [xxxxxxxx|07|06|05|04|03|02|01|00]
			x1 = _mm_packus_epi16(x1, x1); // [xxxxxxxx|17|16|15|14|13|12|11|10]
			x2 = _mm_packus_epi16(x2, x2); // [xxxxxxxx|27|26|25|24|23|22|21|20]
			x3 = _mm_packus_epi16(x3, x3); // [xxxxxxxx|37|36|35|34|33|32|31|30]
			// check
#ifdef _DEBUG
			if ( 1 ) {
				pixel src[8], t[8];
				uint64_t dright, dopt;
				int i, j;
				for ( i = 0; i < 4; i++ ) {
					for( j = 0; j < 8; j ++ )
						src[j] = pix[-4 + i*stride + j];

					t[3] = lent_clip( ((int)src[1] + 2*src[2] + 2*src[3] + 2*src[4] + src[5] + 4)>>3, src[3]-tcm2, src[3]+tcm2 );
					t[4] = lent_clip( ((int)src[2] + 2*src[3] + 2*src[4] + 2*src[5] + src[6] + 4)>>3, src[4]-tcm2, src[4]+tcm2 );

					t[2] = lent_clip( ((int)src[1] + src[2] + src[3] + src[4] + 2)>>2, src[2]-tcm2, src[2]+tcm2 );
					t[5] = lent_clip( ((int)src[3] + src[4] + src[5] + src[6] + 2)>>2, src[5]-tcm2, src[5]+tcm2 );

					t[1] = lent_clip( ((int)2*src[0] + 3*src[1] + src[2] + src[3] + src[4] + 4)>>3, src[1]-tcm2, src[1]+tcm2 );
					t[6] = lent_clip( ((int)src[3] + src[4] + src[5] + 3*src[6] + 2*src[7] + 4)>>3, src[6]-tcm2, src[6]+tcm2 );

					t[0] = src[0];
					t[7] = src[7];

					dright =  (((uint64_t)t[0]) <<  0)
							| (((uint64_t)t[1]) <<  8)
							| (((uint64_t)t[2]) << 16)
							| (((uint64_t)t[3]) << 24)
							| (((uint64_t)t[4]) << 32)
							| (((uint64_t)t[5]) << 40)
							| (((uint64_t)t[6]) << 48)
							| (((uint64_t)t[7]) << 56);
					if ( 0 == i )
						_mm_storel_epi64((__m128i*)&dopt, x0);
					else if ( 1 == i )
						_mm_storel_epi64((__m128i*)&dopt, x1);
					else if ( 2 == i )
						_mm_storel_epi64((__m128i*)&dopt, x2);
					else if ( 3 == i )
						_mm_storel_epi64((__m128i*)&dopt, x3);
					if ( dopt != dright ) {
						lent_log(NULL, LENT_LOG_ERROR, "deblock_h_luma_sse2 strong filter optimize error at %d, i=%d, right=0x%I64x, opt=0x%I64x!\n", strong_h_filter_times, i, dright, dopt);
					}
				}
			}
#endif
			_mm_storel_epi64((__m128i*)(pix-4 + 0*stride), x0);
			_mm_storel_epi64((__m128i*)(pix-4 + 1*stride), x1);
			_mm_storel_epi64((__m128i*)(pix-4 + 2*stride), x2);
			_mm_storel_epi64((__m128i*)(pix-4 + 3*stride), x3);
		}
	}
}

static void deblock_v_chroma_sse2( pixel *pix, pixel *judge, int stride, int beta, int tc0 )
{
	__m128i x2, x3, x4, x5, d;
#ifdef _DEBUG
	static int chroma_v_filter_times = 0;
	pixel right3[4], right4[4];
	++chroma_v_filter_times;
	{	// self test
		int xstride = stride, ystride = 1;
		int i;
		pixel * p = pix;
		for( i = 0; i < 4; i ++ )
		{
			pixel m4 = p[( 0)*xstride];
			pixel m3 = p[(-1)*xstride];
			pixel m5 = p[( 1)*xstride];
			pixel m2 = p[(-2)*xstride];
			int delta = ( (( m4 - m3 ) << 2 ) + m2 - m5 + 4 ) >> 3;

			delta = lent_clip( delta, -tc0, tc0 );
			right3[i] = lent_clip_uint8(m3+delta);
			right4[i] = lent_clip_uint8(m4-delta);

			p += ystride;
		}
	}
#endif
	x4 = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(pix + 0*stride)), _mm_setzero_si128());
	x3 = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(pix - 1*stride)), _mm_setzero_si128());
	d = _mm_slli_epi16(_mm_sub_epi16(x4, x3), 2);
	x2 = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(pix - 2*stride)), _mm_setzero_si128());
	d = _mm_add_epi16(d, x2);
	x5 = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(pix + 1*stride)), _mm_setzero_si128());
	d = _mm_sub_epi16(d, x5);
	d = _mm_add_epi16(d, _mm_set1_epi16(4));
	d = _mm_srai_epi16(d, 3);
	d = _mm_max_epi16(d, _mm_set1_epi16(-tc0));
	d = _mm_min_epi16(d, _mm_set1_epi16(tc0));
	x3 = _mm_add_epi16(x3, d);
	x3 = _mm_packus_epi16(x3, x3);
	*(uint32_t*)(pix - 1*stride) = _mm_cvtsi128_si32(x3);
	x4 = _mm_sub_epi16(x4, d);
	x4 = _mm_packus_epi16(x4, x4);
	*(uint32_t*)(pix + 0*stride) = _mm_cvtsi128_si32(x4);
#ifdef _DEBUG
	{ // self test
		uint32_t myx3, myx4, r3, r4;
		myx3 = _mm_cvtsi128_si32(x3);
		myx4 = _mm_cvtsi128_si32(x4);
		r3 = ((int)right3[0] << 0) | ((int)right3[1] << 8) | ((int)right3[2] << 16) | ((int)right3[3] << 24);
		r4 = ((int)right4[0] << 0) | ((int)right4[1] << 8) | ((int)right4[2] << 16) | ((int)right4[3] << 24);
		if ( myx3 != r3 || myx4 != r4 ) {
			lent_log(NULL, LENT_LOG_ERROR, "deblock_v_chroma_sse2 filter optimize error at %d, r3=0x%08x, o3=0x%08x, r4=0x%08x, o4=0x%08x!\n", chroma_v_filter_times, r3, myx3, r4, myx4);
		}
	}
#endif
}

static void deblock_h_chroma_sse2( pixel *pix, pixel *judge, int stride, int beta, int tc0 )
{
	// int delta = ( (( m4 - m3 ) << 2 ) + m2 - m5 + 4 ) >> 3
	//           = ((m4 - m3) * 4 + m2 - m5 + 4) / 8
	//           = (1*m2 - 4*m3 + 4*m4 - 1*m5 + 4) / 8
	__m128i x, xlo, xhi, a32, a45, a34, d, t43, t52;
#ifdef _DEBUG
	static int chroma_h_filter_times = 0;
	pixel right3[4], right4[4];
	++chroma_h_filter_times;
	{	// self test
		int xstride = 1, ystride = stride;
		int i;
		pixel * p = pix;
		for( i = 0; i < 4; i ++ )
		{
			pixel m4 = p[( 0)*xstride];
			pixel m3 = p[(-1)*xstride];
			pixel m5 = p[( 1)*xstride];
			pixel m2 = p[(-2)*xstride];
			int delta = ( (( m4 - m3 ) << 2 ) + m2 - m5 + 4 ) >> 3;

			delta = lent_clip( delta, -tc0, tc0 );
			right3[i] = lent_clip_uint8(m3+delta);
			right4[i] = lent_clip_uint8(m4-delta);

			p += ystride;
		}
	}
#endif
	xlo = _mm_unpacklo_epi8(_mm_srli_si128(_mm_loadl_epi64((__m128i*)(pix + 0*stride - 4)), 2), _mm_srli_si128(_mm_loadl_epi64((__m128i*)(pix + 1*stride - 4)), 2)); // [xxxxxxxx|x15|x05|x14|x04|x13|x03|x12|x02]
	xhi = _mm_unpacklo_epi8(_mm_srli_si128(_mm_loadl_epi64((__m128i*)(pix + 2*stride - 4)), 2), _mm_srli_si128(_mm_loadl_epi64((__m128i*)(pix + 3*stride - 4)), 2)); // [xxxxxxxx|x35|x25|x34|x24|x33|x23|x32|x22]
	x = _mm_unpacklo_epi16(xlo, xhi); // [x35|x25|x15|x05|x34|x24|x14|x04|x33|x23|x13|x03|x32|x22|x12|x02]
	a32 = _mm_shuffle_epi32(_mm_unpacklo_epi8(x, _mm_setzero_si128()), _MM_SHUFFLE4(1,0,3,2)); // [x32|x22|x12|x02|x33|x23|x13|x03]
	a45 = _mm_unpackhi_epi8(x, _mm_setzero_si128());                                           // [x35|x25|x15|x05|x34|x24|x14|x04]
	a34 = _mm_unpacklo_epi64(a32, a45); // [x34|x24|x14|x04|x33|x23|x13|x03]
	d = _mm_sub_epi16(a45, a32); // [xi5-xi2|xi4-xi3] i=0,1,2,3
	t43 = _mm_unpacklo_epi64(d, _mm_setzero_si128()); // [xxxx|xi4-xi3] i=0,1,2,3
	t52 = _mm_unpackhi_epi64(d, _mm_setzero_si128()); // [xxxx|xi5-xi2] i=0,1,2,3
	d = _mm_sub_epi16( _mm_slli_epi16(t43, 2), t52 ); // [xxxx|(xi4-xi3)*4 - (xi5-xi2)] i=0,1,2,3
	d = _mm_add_epi16(d, _mm_set1_epi16(4));
	d = _mm_srai_epi16(d, 3);
	d = _mm_max_epi16(d, _mm_set1_epi16(-tc0));
	d = _mm_min_epi16(d, _mm_set1_epi16(tc0)); // [xxxx|d3|d2|d1|d0]
	a34 = _mm_add_epi16(a34, _mm_unpacklo_epi64(d, _mm_sub_epi16(_mm_setzero_si128(), d))); // [x34-d|x24-d|x14-d|x04-d|x33+d|x23+d|x13+d|x03+d]
	a34 = _mm_unpacklo_epi16(a34, _mm_unpackhi_epi64(a34, a34)); // [x34-d|x33+d|x24-d|x23+d|x14-d|x13+d|x04-d|x03+d]
	a34 = _mm_packus_epi16(a34, a34); // [xxxxxxxx|x34-d|x33+d|x24-d|x23+d|x14-d|x13+d|x04-d|x03+d]
	*(uint16_t*)(pix + 0*stride - 1) = _mm_extract_epi16(a34, 0);
	*(uint16_t*)(pix + 1*stride - 1) = _mm_extract_epi16(a34, 1);
	*(uint16_t*)(pix + 2*stride - 1) = _mm_extract_epi16(a34, 2);
	*(uint16_t*)(pix + 3*stride - 1) = _mm_extract_epi16(a34, 3);
#ifdef _DEBUG
	{ // self test
		uint32_t rlo, rhi, olo, ohi;
		rlo = ((uint32_t)right3[0] <<  0) | ((uint32_t)right4[0] <<  8)
			| ((uint32_t)right3[1] << 16) | ((uint32_t)right4[1] << 24);
		rhi = ((uint32_t)right3[2] <<  0) | ((uint32_t)right4[2] <<  8)
			| ((uint32_t)right3[3] << 16) | ((uint32_t)right4[3] << 24);
		olo = (uint32_t)_mm_cvtsi128_si32(a34);
		ohi = (uint32_t)_mm_cvtsi128_si32(_mm_srli_si128(a34, 4));
		if ( olo != rlo || ohi != rhi ) {
			lent_log(NULL, LENT_LOG_ERROR, "deblock_h_chroma_sse2 filter optimize error at %d, r=0x%08x%08x, o=0x%08x%08x!\n", chroma_h_filter_times, rhi, rlo, ohi, olo);
		}
	}
#endif
}




//
// SSSE3
//


static void deblock_v_luma_ssse3( pixel *pix, pixel *judge, int stride, int beta, int tc0 )
{
//	deblock_luma_ssse3( pix, judge, stride, 1, beta, tc0 );
	int i_side_thresh = (beta+(beta>>1))>>3;
	int tc = tc0;
	int i_tc_thresh = tc * 10;
	int tcd2=tc >> 1,
		tcm2=tc << 1;
	int dp, dq, d0, d3, dt;
	int dp0, dp3, dq0, dq3;
	int i;
	//pixel px[8][8];
	__m128i x[8],d[8],mask,delta;


	// modify by James.DF, 2012.12.03, move up to 'deblock_edge' function, not test!!!
//	if( tc0 < 0 ) return;
	//_mm_empty();

	// modify by James.DF, 2012.11.29
/*
	dp0 = cal_dp( judge + 0*1, stride );
	dq0 = cal_dq( judge + 0*1, stride );
	dp3 = cal_dp( judge + 3*1, stride );
	dq3 = cal_dq( judge + 3*1, stride );

	d0 = dp0 + dq0;
	d3 = dp3 + dq3;
	dp = dp0 + dp3;
	dq = dq0 + dq3;

	dt = d0 + d3;
*/
	{	// vertical filtering for horizontal edges
		__m128i x, xmm_dp, xmm_dq;
		// dp
		xmm_dp = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(judge-3*stride)), _mm_setzero_si128()); // [x70|x60|x50|x40|x30|x20|x10|x00]
		x = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(judge-2*stride)), _mm_setzero_si128()); // [x71|x61|x51|x41|x31|x21|x11|x010]
		xmm_dp = _mm_sub_epi16(xmm_dp, x);
		xmm_dp = _mm_sub_epi16(xmm_dp, x);
		x = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(judge-1*stride)), _mm_setzero_si128());
		xmm_dp = _mm_add_epi16(xmm_dp, x);
		xmm_dp = _mm_abs_epi16(xmm_dp);
		xmm_dp = _mm_unpacklo_epi16(xmm_dp, _mm_setzero_si128()); // to 32bit
		dp0 = _mm_cvtsi128_si32(xmm_dp);
		xmm_dp = _mm_srli_si128(xmm_dp, 12);
		dp3 = _mm_cvtsi128_si32(xmm_dp);
		// dq
		xmm_dq = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(judge+0*stride)), _mm_setzero_si128());
		x = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(judge+1*stride)), _mm_setzero_si128());
		xmm_dq = _mm_sub_epi16(xmm_dq, x);
		xmm_dq = _mm_sub_epi16(xmm_dq, x);
		x = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(judge+2*stride)), _mm_setzero_si128());
		xmm_dq = _mm_add_epi16(xmm_dq, x);
		xmm_dq = _mm_abs_epi16(xmm_dq);
		xmm_dq = _mm_unpacklo_epi16(xmm_dq, _mm_setzero_si128()); // to 32bit
		dq0 = _mm_cvtsi128_si32(xmm_dq);
		xmm_dq = _mm_srli_si128(xmm_dq, 12);
		dq3 = _mm_cvtsi128_si32(xmm_dq);
		//
		d0 = dp0 + dq0;
		d3 = dp3 + dq3;
		dp = dp0 + dp3;
		dq = dq0 + dq3;
		dt = d0 + d3;
	}
	// end

	if( dt < beta )
	{
		int b_filter_p, b_filter_q, b_strong;

		b_filter_p = dp < i_side_thresh;
		b_filter_q = dq < i_side_thresh;
/*
		b_strong = b_strong_filter( judge    , stride, 2*d0, beta, tc )
				&& b_strong_filter( judge+3*1, stride, 2*d3, beta, tc );
*/
/*
#define PIX(x) pix[(x)*xstride]
#define JUD(x) judge[(x)*xstride]
static inline int b_strong_filter( pixel *judge, int xstride, int d, int beta, int tc )
{
	pixel m4 = JUD( 0);
	pixel m3 = JUD(-1);
	pixel m7 = JUD( 3);
	pixel m0 = JUD(-4);

	int d_strong = LENTABS_INT( m0 - m3 ) + LENTABS_INT( m7 - m4 );

	return (d_strong < (beta>>3)) && (d<(beta>>2)) && 
		(LENTABS_INT(m3-m4) < ((tc*5+1)>>1));
}
*/
		{
			static const __m128i shuffle_0347 = {
				0, 0x80, 4, 0x80,  8, 0x80, 12, 0x80,	// for x0
				3, 0x80, 7, 0x80, 11, 0x80, 15, 0x80,	// for x3
			};
			__m128i x03lo, x03hi, x03, x0347, x347, x04, ds03;
			x03lo = _mm_unpacklo_epi32(_mm_loadl_epi64((__m128i*)(judge-4*stride)), _mm_loadl_epi64((__m128i*)(judge-1*stride))); // [xxxxxxxx|x33|x23|x13|x03|x30|x20|x10|x00]
			x03hi = _mm_unpacklo_epi32(_mm_loadl_epi64((__m128i*)(judge+0*stride)), _mm_loadl_epi64((__m128i*)(judge+3*stride))); // [xxxxxxxx|x37|x27|x17|x07|x34|x24|x14|x04]
			x03 = _mm_unpacklo_epi64(x03lo, x03hi); // [x37|x27|x17|x07|x34|x24|x14|x04|x33|x23|x13|x03|x30|x20|x10|x00]
			x0347 = _mm_shuffle_epi8(x03, shuffle_0347); // epi16: [x37|x34|x33|x30|x07|x04|x03|x00]
			x347 = _mm_srli_si128(x0347, 2); // epi16: [000|x37|x34|x33|x30|x07|x04|x03]
			x0347 = _mm_sub_epi16(x0347, x347);
			x0347 = _mm_abs_epi16(x0347); // [x37-000|x34-x37|x33-x34|x30-x33|x07-x30|x04-x07|x03-x04|x00-x03]
			x04 = _mm_srli_si128(x0347, 4); // [000|000|x37-000|x34-x37|x33-x34|x30-x33|x07-x30|x04-x07]
			ds03 = _mm_add_epi16(x0347, x04); // [x|x|x|ds3|x|x|x|ds0]
			b_strong = (_mm_extract_epi16(ds03, 0) < (beta>>3)) && (_mm_extract_epi16(ds03, 4) < (beta>>3));
			b_strong = b_strong && (_mm_extract_epi16(x0347, 1) < ((tc*5+1)>>1)) && (_mm_extract_epi16(x0347, 5) < ((tc*5+1)>>1));
			b_strong = b_strong && ((2*d0) < (beta>>2)) && ((2*d3) < (beta>>2));
#ifdef _DEBUG
			if ( 1 ) {
				int b_strong_right;
				b_strong_right = b_strong_filter( judge    , stride, 2*d0, beta, tc )
							  && b_strong_filter( judge+3*1, stride, 2*d3, beta, tc );
				if ( b_strong != b_strong_right ) {
					lent_log(NULL, LENT_LOG_ERROR, "deblock_v_luma_ssse3 b_strong error, right=%d, opt=%d!\n", b_strong_right, b_strong);
				}
			}
#endif
		}

		if(!b_strong)
		{
			{
				pixel *p=pix-2*stride;
				// for(i=0;i<8;i++)
// 				for ( i = 1; i < 7; i++ )
// 				{
// 					x[i]=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)p),zero);
// 					p+=stride;
// 				}
				x[1] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(p-stride)),zero);
				x[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(p)),zero);
				x[3] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(p+stride)),zero);
				p = pix + stride;
				x[4] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(pix)),zero);
				x[5] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(p)),zero);
				x[6] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(p+stride)),zero);
			}

			delta=_mm_mullo_epi16(_mm_sub_epi16(x[4],x[3]),_mm_set1_epi16(9));
			delta=_mm_sub_epi16(delta,_mm_mullo_epi16(_mm_sub_epi16(x[5],x[2]),_mm_set1_epi16(3)));
			delta=_mm_add_epi16(delta,_mm_set1_epi16(8));
			delta=_mm_srai_epi16(delta,4);
			mask=_mm_cmplt_epi16(_mm_abs_epi16(delta),_mm_set1_epi16(i_tc_thresh));
			mask=_mm_unpacklo_epi64(mask,_mm_setzero_si128());//only consider the low 64 bit(4 coloums)
/*
			// delta = (9*(src[4]-src[3]) + 3*(src[2]-src[5]) + 8)>>4;
			{
				const __m128i coeff93 = {9, 0, 9, 0, 9, 0, 9, 0, 3, 0, 3, 0, 3, 0, 3, 0};
				__m128i x42, x35;
				x42 = _mm_unpacklo_epi64(x[4], x[2]);
				x35 = _mm_unpacklo_epi64(x[3], x[5]);
				delta = _mm_sub_epi16(x42, x35);
				delta = _mm_mullo_epi16(delta, coeff93);
				delta = _mm_add_epi16(delta, _mm_unpackhi_epi64(delta, delta));

				delta=_mm_add_epi16(delta,_mm_set1_epi16(8));
				delta=_mm_srai_epi16(delta,4);
				mask=_mm_cmplt_epi16(_mm_abs_epi16(delta),_mm_set1_epi16(i_tc_thresh));
				mask=_mm_unpacklo_epi64(mask,_mm_setzero_si128());//only consider the low 64 bit(4 coloums)
			}
*/
			if(!_mm_movemask_epi8 (mask))
				return;

			delta=_mm_max_epi16(delta,_mm_set1_epi16(-tc));
			delta=_mm_min_epi16(delta,_mm_set1_epi16(tc));

			d[2]=_mm_add_epi16(x[3],delta);
			d[3]=_mm_sub_epi16(x[4],delta);

			if(b_filter_p)
			{
				__m128i delta1;

				//x[1]=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(pix-3*stride)),zero);

/*
				delta1=_mm_add_epi16(x[1],x[3]);
				delta1=_mm_add_epi16(delta1,_mm_set1_epi16(1));
				delta1=_mm_srai_epi16(delta1,1);
*/				delta1 = _mm_avg_epu16(x[1], x[3]);

				delta1=_mm_sub_epi16(delta1,x[2]);
				delta1=_mm_add_epi16(delta1,delta);
				delta1=_mm_srai_epi16(delta1,1);

				delta1=_mm_min_epi16(delta1,_mm_set1_epi16(tcd2));
				delta1=_mm_max_epi16(delta1,_mm_set1_epi16(-tcd2));

				d[0]=_mm_add_epi16(x[2],delta1);

				d[0]=_mm_and_si128(d[0],mask);
				x[2]=_mm_andnot_si128(mask,x[2]);
				x[2]=_mm_or_si128(d[0],x[2]);
			}
			if(b_filter_q)
			{
				__m128i delta1;

				//x[6]=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(pix+2*stride)),zero);

/*
				delta1=_mm_add_epi16(x[4],x[6]);
				delta1=_mm_add_epi16(delta1,_mm_set1_epi16(1));
				delta1=_mm_srai_epi16(delta1,1);
*/				delta1 = _mm_avg_epu16(x[4], x[6]);

				delta1=_mm_sub_epi16(delta1,x[5]);
				delta1=_mm_sub_epi16(delta1,delta);
				delta1=_mm_srai_epi16(delta1,1);

				delta1=_mm_min_epi16(delta1,_mm_set1_epi16(tcd2));
				delta1=_mm_max_epi16(delta1,_mm_set1_epi16(-tcd2));

				d[0]=_mm_add_epi16(x[5],delta1);

				d[0]=_mm_and_si128(d[0],mask);
				x[5]=_mm_andnot_si128(mask,x[5]);
				x[5]=_mm_or_si128(d[0],x[5]);
			}

			d[2]=_mm_and_si128(d[2],mask);
			d[3]=_mm_and_si128(d[3],mask);

			x[3]=_mm_andnot_si128(mask,x[3]);
			x[4]=_mm_andnot_si128(mask,x[4]);

			x[3]=_mm_or_si128(d[2],x[3]);
			x[4]=_mm_or_si128(d[3],x[4]);
		}
		else
		{
			// test
//			static int strong_v_filter_times = 0;
//			lent_log(NULL, LENT_LOG_VERBOSE, "deblock_v_luma_ssse3 strong filter %d times %d\n", ++strong_v_filter_times);
			// test end
			if(0)
			{
				__m128i mIn=_mm_set1_epi16(-tcm2),mAx=_mm_set1_epi16(tcm2);

				{
					pixel *p=pix-4*stride;
					for(i=0;i<8;i++)
					{
						x[i]=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)p),zero);
						p+=stride;
					}
				}

#define ADD(x,y)	_mm_add_epi16(x,y)
#define ADDS(x,y)	x=ADD(x,y)

				for(i=0;i<7;i++)
					d[i]=ADD(x[i],x[i+1]);

				delta=_mm_slli_epi16(d[0],1);//2 3 1 1 1
				ADDS(delta,d[1]);
				ADDS(delta,d[3]);
				delta=_mm_add_epi16(delta,_mm_set1_epi16(4));
				delta=_mm_srai_epi16(delta,3);

				delta=_mm_sub_epi16(delta,x[1]);
				delta=_mm_min_epi16(delta,mAx);
				delta=_mm_max_epi16(delta,mIn);

				ADDS(x[1],delta);

				delta=ADD(d[1],d[3]);//0 2 2 2 2
				delta=_mm_slli_epi16(delta,1);
				delta=_mm_add_epi16(delta,_mm_set1_epi16(4));
				delta=_mm_srai_epi16(delta,3);
				
				delta=_mm_sub_epi16(delta,x[2]);
				delta=_mm_min_epi16(delta,mAx);
				delta=_mm_max_epi16(delta,mIn);
				ADDS(x[2],delta);

				delta=ADD(d[1],d[2]);//0 1 2 2 2 1
				ADDS(delta,d[3]);
				ADDS(delta,d[4]);
				delta=_mm_add_epi16(delta,_mm_set1_epi16(4));
				delta=_mm_srai_epi16(delta,3);
				
				delta=_mm_sub_epi16(delta,x[3]);
				delta=_mm_min_epi16(delta,mAx);
				delta=_mm_max_epi16(delta,mIn);
				ADDS(x[3],delta);


				delta=ADD(d[2],d[3]);//0 0 1 2 2 2 1
				ADDS(delta,d[4]);
				ADDS(delta,d[5]);
				delta=_mm_add_epi16(delta,_mm_set1_epi16(4));
				delta=_mm_srai_epi16(delta,3);
				
				delta=_mm_sub_epi16(delta,x[4]);
				delta=_mm_min_epi16(delta,mAx);
				delta=_mm_max_epi16(delta,mIn);
				ADDS(x[4],delta);

				delta=ADD(d[3],d[5]);//0 0 0 2 2 2 2
				delta=_mm_slli_epi16(delta,1);
				delta=_mm_add_epi16(delta,_mm_set1_epi16(4));
				delta=_mm_srai_epi16(delta,3);
				
				delta=_mm_sub_epi16(delta,x[5]);
				delta=_mm_min_epi16(delta,mAx);
				delta=_mm_max_epi16(delta,mIn);
				ADDS(x[5],delta);

				delta=_mm_slli_epi16(d[6],1);//0 0 0 1 1 1 3 2
				ADDS(delta,d[3]);
				ADDS(delta,d[5]);
				delta=_mm_add_epi16(delta,_mm_set1_epi16(4));
				delta=_mm_srai_epi16(delta,3);
				
				delta=_mm_sub_epi16(delta,x[6]);
				delta=_mm_min_epi16(delta,mAx);
				delta=_mm_max_epi16(delta,mIn);
				ADDS(x[6],delta);

			}
			else
			{
				pixel *p = pix - 4*stride;
				__m128i x07, x16, x25, x52, x34, x43;
				__m128i d16, d25, d34, t;
				__m128i mIn=_mm_set1_epi16(-tcm2), mAx=_mm_set1_epi16(tcm2);
				// test
//				__m128i xbak[8];
//				xbak[1] = x[1];
//				xbak[2] = x[2];
//				xbak[3] = x[3];
//				xbak[4] = x[4];
//				xbak[5] = x[5];
//				xbak[6] = x[6];
				// test end
				// for line 0,7
				x[0] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(p + 0*stride)), _mm_setzero_si128());
				x[7] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(p + 7*stride)), _mm_setzero_si128());
				x07 = _mm_unpacklo_epi64(x[0], x[7]);
				d16 = _mm_slli_epi16(x07, 1);	// d16: {*2 3 1 1 1} + 4 / 8, d16 = x07 * 2
				// for line 1,6
				x[1] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(p + 1*stride)), _mm_setzero_si128());
				x[6] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(p + 6*stride)), _mm_setzero_si128());
				x16 = _mm_unpacklo_epi64(x[1], x[6]);
				d16 = _mm_add_epi16(d16, _mm_add_epi16(x16, _mm_slli_epi16(x16, 1)));	// d16: {2 *3 1 1 1} + 4 / 8, d16 += x16 * 3
				d25 = x16;	// d25: 0 *1 1 1 1 / 4, d25 = x16
				d34 = x16;	// d34: 0 *1 2 2 2 1 / 8, d34 = x16
				// for line 2,5
				x[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(p + 2*stride)), _mm_setzero_si128());
				x[5] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(p + 5*stride)), _mm_setzero_si128());
				x25 = _mm_unpacklo_epi64(x[2], x[5]);
				x52 = _mm_unpacklo_epi64(x[5], x[2]);
				d16 = _mm_add_epi16(d16, x25);	// d16: {2 3 *1 1 1} + 4 / 8,	d16 += x25
				d25 = _mm_add_epi16(d25, x25);	// d25: {0 1 *1 1 1} + 2 / 4, d25 += x25
				d34 = _mm_add_epi16(d34, _mm_add_epi16(_mm_slli_epi16(x25, 1), x52));	// d34: {0 1 *2 2 2 *1} + 4 / 8, d34 = x25*2 + x52
				// for line 3,4
				x[3] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(p + 3*stride)), _mm_setzero_si128());
				x[4] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(p + 4*stride)), _mm_setzero_si128());
				x34 = _mm_unpacklo_epi64(x[3], x[4]);
				x43 = _mm_unpacklo_epi64(x[4], x[3]);
				t = _mm_add_epi16(x34, x43);
				d16 = _mm_add_epi16(d16, t);	// d16: {2 3 1 *1 *1} + 4 / 8, d16 += x34 + x43
				d25 = _mm_add_epi16(d25, t);	// // d25: {0 1 1 *1 *1} + 4 / 4, d25 += x34 + x43
				d34 = _mm_add_epi16(d34, _mm_slli_epi16(t, 1));	// d34: {0 1 2 *2 *2 1} + 4 / 8, d34 += x34*2 + x43*2
				// round division
				t = _mm_set1_epi16(2);
				d25 = _mm_srai_epi16(_mm_add_epi16(d25, t), 2);	// // d25: {0 1 1 1 1} + *2 / *4, d25 = (d25 + 2) / 4
				t = _mm_add_epi16(t, t);
				d16 = _mm_srai_epi16(_mm_add_epi16(d16, t), 3);	// d16: {2 3 1 1 1} + *4 / *8, d16 = (d16 + 4) / 8
				d34 = _mm_srai_epi16(_mm_add_epi16(d34, t), 3);	// d34: {0 1 2 2 2 1} + *4 / *8, d34 = (d34 + 4) / 8
				// clip
				t=_mm_sub_epi16(d16, x16);
				t=_mm_min_epi16(t, mAx);
				t=_mm_max_epi16(t, mIn);
				d16 = _mm_add_epi16(x16, t);
				t=_mm_sub_epi16(d25, x25);
				t=_mm_min_epi16(t, mAx);
				t=_mm_max_epi16(t, mIn);
				d25 = _mm_add_epi16(x25, t);
				t=_mm_sub_epi16(d34, x34);
				t=_mm_min_epi16(t, mAx);
				t=_mm_max_epi16(t, mIn);
				d34 = _mm_add_epi16(x34, t);
				// test
#if 0
				{
					__m128i xr16, xr25, xr34;
					xr16 = _mm_unpacklo_epi64(xbak[1], xbak[6]);
					xr25 = _mm_unpacklo_epi64(xbak[2], xbak[5]);
					xr34 = _mm_unpacklo_epi64(xbak[3], xbak[4]);
					if ( _mm_movemask_epi8(_mm_cmpgt_epi16(xr16, d16)) || _mm_movemask_epi8(_mm_cmplt_epi16(xr16, d16)) ) {
						lent_log(NULL, LENT_LOG_ERROR, "deblock_v_luma_ssse3 strong filter optimize error at %d in x16!\n", strong_v_filter_times);
					}
					if ( _mm_movemask_epi8(_mm_cmpgt_epi16(xr25, d25)) || _mm_movemask_epi8(_mm_cmplt_epi16(xr25, d25)) ) {
						lent_log(NULL, LENT_LOG_ERROR, "deblock_v_luma_ssse3 strong filter optimize error at %d in x25!\n", strong_v_filter_times);
					}
					if ( _mm_movemask_epi8(_mm_cmpgt_epi16(xr34, d34)) || _mm_movemask_epi8(_mm_cmplt_epi16(xr34, d34)) ) {
						lent_log(NULL, LENT_LOG_ERROR, "deblock_v_luma_ssse3 strong filter optimize error at %d in x34!\n", strong_v_filter_times);
					}
				}
#endif
				// test end
				// store
				*(uint32_t*)(p + 1*stride) = _mm_cvtsi128_si32(_mm_packus_epi16(d16, d16));
				*(uint32_t*)(p + 6*stride) = _mm_cvtsi128_si32(_mm_packus_epi16(_mm_unpackhi_epi64(d16, d16), _mm_setzero_si128()));
				*(uint32_t*)(p + 2*stride) = _mm_cvtsi128_si32(_mm_packus_epi16(d25, d25));
				*(uint32_t*)(p + 5*stride) = _mm_cvtsi128_si32(_mm_packus_epi16(_mm_unpackhi_epi64(d25, d25), _mm_setzero_si128()));
				*(uint32_t*)(p + 3*stride) = _mm_cvtsi128_si32(_mm_packus_epi16(d34, d34));
				*(uint32_t*)(p + 4*stride) = _mm_cvtsi128_si32(_mm_packus_epi16(_mm_unpackhi_epi64(d34, d34), _mm_setzero_si128()));
			}
		}

		{
			if(b_strong)
			{
/*				pixel *p=pix-3*stride;
				for(i=1;i<7;i++)
				{
					//_mm_storel_epi64(p,_mm_packus_epi16(x[i],x[i]));
					*((int*)p)=_mm_cvtsi128_si32(_mm_packus_epi16(x[i],x[i]));
					p+=stride;
				}
*/			}
			else
			{
//				pixel *p=pix-2*stride;
// 				for(i=2;i<6;i++)
// 				{
// 					//_mm_storel_epi64(p,_mm_packus_epi16(x[i],x[i]));
// 					*((int*)p)=_mm_cvtsi128_si32(_mm_packus_epi16(x[i],x[i]));
// 					p+=stride;
// 				}
				*((int*)(pix-stride*2)) =_mm_cvtsi128_si32(_mm_packus_epi16(x[2],x[2]));
				*((int*)(pix-stride)) =_mm_cvtsi128_si32(_mm_packus_epi16(x[3],x[3]));
				*((int*)(pix))        =_mm_cvtsi128_si32(_mm_packus_epi16(x[4],x[4]));
				*((int*)(pix+stride)) =_mm_cvtsi128_si32(_mm_packus_epi16(x[5],x[5]));
			}
		}
	}
}
static void deblock_h_luma_ssse3( pixel *pix, pixel *judge, int stride, int beta, int tc0 )
{
	// deblock_luma_ssse3( pix, judge, 1, stride, beta, tc0 );
	int i_side_thresh;
	int tc, i_tc_thresh, tcd2, tcm2;
	int dp, dq, d0, d3, dt;
	__m128i x03;

	// test
#ifdef _DEBUG
	static int part_times[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	static int strong_h_filter_times = 0;
	++strong_h_filter_times;
	part_times[0]++;
#endif
	// test end

	i_side_thresh = (beta+(beta>>1))>>3;
	tc = tc0;
	i_tc_thresh = tc * 10;
	tcd2=tc >> 1;
	tcm2=tc << 1;

	// modify by James.DF, 2012.12.03, move up to 'deblock_edge' function, not test!!!
//	if( tc0 < 0 ) return;
	//_mm_empty();

	// modify by James.DF, 2012.11.29
/*
	dp0 = cal_dp( judge + 0*stride, 1 );
	dq0 = cal_dq( judge + 0*stride, 1 );
	dp3 = cal_dp( judge + 3*stride, 1 );
	dq3 = cal_dq( judge + 3*stride, 1 );

	d0 = dp0 + dq0;
	d3 = dp3 + dq3;
	dp = dp0 + dp3;
	dq = dq0 + dq3;

	dt = d0 + d3;
*/
	{	// horizontal filtering for vertical edges
		const __m128i coeff = {0, 1, -2, 1, 1, -2, 1, 0, 0, 1, -2, 1, 1, -2, 1, 0};
		__m128i x0, x3, xmm_d, xmm_d01, xmm_dpq, xmm_dt;
		x0 = _mm_loadl_epi64((__m128i*)(judge+stride*0-4));
		x3 = _mm_loadl_epi64((__m128i*)(judge+stride*3-4));
		x03 = _mm_unpacklo_epi64(x0, x3);
		// now, we got x03 = |x[0,-4]|x[0,-3]|x[0,-2]|x[0,-1]|x[0,0]|x[0,1]|x[0,2]|x[0,3]|x[3,-4]|x[3,-3]|x[3,-2]|x[3,-1]|x[3,0]|x[3,1]|x[3,2]|x[3,3]|
		xmm_d = _mm_maddubs_epi16(x03, coeff);
		xmm_d = _mm_hadd_epi16(xmm_d, xmm_d);
		xmm_d = _mm_abs_epi16(xmm_d);
		xmm_d = _mm_unpacklo_epi16(xmm_d, _mm_setzero_si128());
		// now, xmm_d = |p0|q0|p3|q3|
		xmm_d01 = _mm_hadd_epi32(xmm_d, xmm_d);	// now, xmm_d01 = |p0+q0|p3+q3|p0+q0|p3+q3|
		xmm_dt = _mm_hadd_epi32(xmm_d01, xmm_d01); // now, xmm_dt = |p0+q0+p3+q3|p0+q0+p3+q3|p0+q0+p3+q3|p0+q0+p3+q3|
		d0 = _mm_cvtsi128_si32(xmm_d01);
		dt = _mm_cvtsi128_si32(xmm_dt);
		xmm_d01 = _mm_srli_epi64(xmm_d01, 32);
		d3 = _mm_cvtsi128_si32(xmm_d01);
		// get dp, dq
		xmm_d = _mm_shuffle_epi32(xmm_d, 0xD8);// 11 01 10 00b
		xmm_dpq = _mm_hadd_epi32(xmm_d, xmm_d);
		dp = _mm_cvtsi128_si32(xmm_dpq);
		xmm_dpq = _mm_srli_epi64(xmm_dpq, 32);
		dq = _mm_cvtsi128_si32(xmm_dpq);
	}
	// end

	if( dt < beta )
	{
		int b_filter_p, b_filter_q, b_strong;
		int i;
		__m128i x[4], mask, delta;

		b_filter_p = dp < i_side_thresh;
		b_filter_q = dq < i_side_thresh;
/*
		b_strong =     b_strong_filter( judge         , 1, 2*d0, beta, tc )
					&& b_strong_filter( judge+3*stride, 1, 2*d3, beta, tc );
*/
/*
#define PIX(x) pix[(x)*xstride]
#define JUD(x) judge[(x)*xstride]
static inline int b_strong_filter( pixel *judge, int xstride, int d, int beta, int tc )
{
	pixel m4 = JUD( 0);
	pixel m3 = JUD(-1);
	pixel m7 = JUD( 3);
	pixel m0 = JUD(-4);

	int d_strong = LENTABS_INT( m0 - m3 ) + LENTABS_INT( m7 - m4 );

	return (d_strong < (beta>>3)) && (d<(beta>>2)) && 
		(LENTABS_INT(m3-m4) < ((tc*5+1)>>1));
}
*/
		{
			static const __m128i shuffle_0347 = {
				0, 0x80,  3, 0x80,  4, 0x80,  7, 0x80,	// for x0
				8, 0x80, 11, 0x80, 12, 0x80, 15, 0x80,	// for x3
			};
			__m128i x0347, x347, x04, ds03;
			x0347 = _mm_shuffle_epi8(x03, shuffle_0347); // epi16: [x37|x34|x33|x30|x07|x04|x03|x00]
			x347 = _mm_srli_si128(x0347, 2); // epi16: [000|x37|x34|x33|x30|x07|x04|x03]
			x0347 = _mm_sub_epi16(x0347, x347);
			x0347 = _mm_abs_epi16(x0347); // [x37-000|x34-x37|x33-x34|x30-x33|x07-x30|x04-x07|x03-x04|x00-x03]
			x04 = _mm_srli_si128(x0347, 4); // [000|000|x37-000|x34-x37|x33-x34|x30-x33|x07-x30|x04-x07]
			ds03 = _mm_add_epi16(x0347, x04); // [x|x|x|ds3|x|x|x|ds0]
			b_strong = (_mm_extract_epi16(ds03, 0) < (beta>>3)) && (_mm_extract_epi16(ds03, 4) < (beta>>3));
			b_strong = b_strong && (_mm_extract_epi16(x0347, 1) < ((tc*5+1)>>1)) && (_mm_extract_epi16(x0347, 5) < ((tc*5+1)>>1));
			b_strong = b_strong && ((2*d0) < (beta>>2)) && ((2*d3) < (beta>>2));
#ifdef _DEBUG
			if ( 1 ) {
				int b_strong_right;
				b_strong_right = b_strong_filter( judge         , 1, 2*d0, beta, tc )
							  && b_strong_filter( judge+3*stride, 1, 2*d3, beta, tc );
				if ( b_strong != b_strong_right ) {
					lent_log(NULL, LENT_LOG_ERROR, "deblock_h_luma_ssse3 b_strong error at %d, right=%d, opt=%d!\n", strong_h_filter_times, b_strong_right, b_strong);
				}
			}
#endif
		}


#ifdef _DEBUG
		part_times[1]++;
		if ( 3257224 == strong_h_filter_times ) {
			lent_log(NULL, LENT_LOG_VERBOSE, "deblock_h_luma_ssse3 hit %d times\n", strong_h_filter_times);
		}
#endif

		if ( !b_strong ) { // weak filter

			// delta = (9*(src[4]-src[3]) - 3*(src[5]-src[2]) + 8)>>4;
			//       = 0*src[0] + 0*src[1] + 3*src[2] - 9*src[3] + 9*src[4] - 3*src[5] + 0*src[6] + 0*src[7]
			// static const __m128i coeff_delta = {0, 0, 0, 0, 0, 3, -1, -9, 0, 9, -1, -3, 0, 0, 0, 0};
			static const __m128i coeff_delta = {
				/*for x0*/ 3, -9, 9, -3,
				/*for x1*/ 3, -9, 9, -3,
				/*for x2*/ 3, -9, 9, -3,
				/*for x3*/ 3, -9, 9, -3,
			};
			static const __m128i mask_x25_epi16 = {
				0xff, 0xff, 0, 0, 0, 0, 0xff, 0xff, 
				0xff, 0xff, 0, 0, 0, 0, 0xff, 0xff,
			};
			__m128i xdeltalo, xdeltahi, xdelta, dvlo, dvhi, dv;
			__m128i dv34, dv34lo, dv34hi, dv34delta;
			__m128i dv2, dv2lo, dv2hi;
			__m128i dv5, dv5lo, dv5hi;
			int i_mask;
#ifdef _DEBUG
			part_times[2]++;
#endif
			x[0] = _mm_loadl_epi64((__m128i*)(pix-4 + 0*stride));
			x[1] = _mm_loadl_epi64((__m128i*)(pix-4 + 1*stride));
			x[2] = _mm_loadl_epi64((__m128i*)(pix-4 + 2*stride));
			x[3] = _mm_loadl_epi64((__m128i*)(pix-4 + 3*stride));
			xdeltalo = _mm_unpacklo_epi32(_mm_srli_si128(x[0], 2), _mm_srli_si128(x[1], 2));	// [xxxxxxxx|15|14|13|12|05|04|03|02]
			xdeltahi = _mm_unpacklo_epi32(_mm_srli_si128(x[2], 2), _mm_srli_si128(x[3], 2));	// [xxxxxxxx|35|34|33|32|25|24|23|22]
			xdelta = _mm_unpacklo_epi64(xdeltalo, xdeltahi);	// [35|34|33|32|25|24|23|22|15|14|13|12|05|04|03|02]
			delta = _mm_maddubs_epi16(xdelta, coeff_delta);
			delta = _mm_hadd_epi16(delta, delta);
			delta=_mm_add_epi16(delta,_mm_set1_epi16(8));
			delta=_mm_srai_epi16(delta,4);
			mask=_mm_cmplt_epi16(_mm_abs_epi16(delta),_mm_set1_epi16(i_tc_thresh));
			mask=_mm_unpacklo_epi64(mask,_mm_setzero_si128());//only consider the low 64 bit(4 coloums)
			i_mask = _mm_movemask_epi8 (mask);
			if ( !i_mask )
				return;

#ifdef _DEBUG
			part_times[3]++;
#endif
			delta=_mm_max_epi16(delta,_mm_set1_epi16(-tc));
			delta=_mm_min_epi16(delta,_mm_set1_epi16(tc));
			dv34lo = _mm_unpacklo_epi16(_mm_srli_si128(x[0], 3), _mm_srli_si128(x[1], 3)); // [xxxxxxxx|16|15|06|05|14|13|04|03]
			dv34hi = _mm_unpacklo_epi16(_mm_srli_si128(x[2], 3), _mm_srli_si128(x[3], 3)); // [xxxxxxxx|36|35|26|25|34|33|24|23]
			dv34 = _mm_unpacklo_epi32(dv34lo, dv34hi); // [xxxxxxxx|34|33|24|23|14|13|04|03]
			dv34 = _mm_unpacklo_epi8(dv34, _mm_setzero_si128()); // [34|33|24|23|14|13|04|03]
			dv34delta = _mm_unpacklo_epi16(delta, _mm_sub_epi16(_mm_setzero_si128(), delta));
			dv34 = _mm_add_epi16(dv34, dv34delta);	// [d34|d33|d24|d24|d14|d13|d04|d03]

			if(b_filter_p)
			{
				// delta1 = ( ((src[1]+src[3]+1)>>1) - src[2] + delta )>>1;
				//        = (0*src[0] + 1*src[1] - 2*src[2] + 1*src[3] + 2*delta + 1) / 4
				static const __m128i coeff_delta1 = {
					0, 1, -2, 1,
					0, 1, -2, 1,
					0, 1, -2, 1,
					0, 1, -2, 1,
				};
				__m128i delta1, xdelta1;
#ifdef _DEBUG
				part_times[4]++;
#endif
				xdelta1 = _mm_unpacklo_epi64(_mm_unpacklo_epi32(x[0], x[1]), _mm_unpacklo_epi32(x[2], x[3]));
				delta1 = _mm_maddubs_epi16(xdelta1, coeff_delta1);
				delta1 = _mm_hadd_epi16(delta1, delta1);
				delta1 = _mm_add_epi16(delta1, _mm_add_epi16(delta, delta));
				delta1 = _mm_add_epi16(delta1, _mm_set1_epi16(1));
				delta1 = _mm_srai_epi16(delta1, 2);
				delta1=_mm_min_epi16(delta1,_mm_set1_epi16(tcd2));
				delta1=_mm_max_epi16(delta1,_mm_set1_epi16(-tcd2));	// [d3|d2|d1|d0|d3|d2|d1|d0]
				dv2lo = _mm_unpacklo_epi8(_mm_srli_si128(x[0], 2), _mm_srli_si128(x[1], 2)); // [xxxxxxxx|15|05|14|04|13|03|12|02]
				dv2hi = _mm_unpacklo_epi8(_mm_srli_si128(x[2], 2), _mm_srli_si128(x[3], 2)); // [xxxxxxxx|35|25|34|24|33|23|32|22]
				dv2 = _mm_unpacklo_epi16(dv2lo, dv2hi); // [xxxxxxxx|33|23|13|03|32|22|12|02]
				dv2 = _mm_unpacklo_epi8(dv2, _mm_setzero_si128()); // [xxxx|32|22|12|02]
				dv2 = _mm_add_epi16(dv2, delta1); // [xxxx|d32|d22|d12|d02]
			}

			if ( b_filter_q ) {
				// delta2 = ( ((src[4]+src[6]+1)>>1) - src[5] - delta )>>1;
				//        = (1*src[4] - 2*src[5] + 1*src[6] + 0*src[7] - 2*delta + 1) / 4
				static const __m128i coeff_delta2 = {
					1, -2, 1, 0,
					1, -2, 1, 0,
					1, -2, 1, 0,
					1, -2, 1, 0,
				};
				__m128i delta2, xdelta2;
#ifdef _DEBUG
				part_times[5]++;
#endif
				xdelta2 = _mm_unpacklo_epi64(_mm_srli_si128(_mm_unpacklo_epi32(x[0], x[1]), 8), _mm_srli_si128(_mm_unpacklo_epi32(x[2], x[3]), 8));
				delta2 = _mm_maddubs_epi16(xdelta2, coeff_delta2);
				delta2 = _mm_hadd_epi16(delta2, delta2);
				delta2 = _mm_sub_epi16(delta2, _mm_add_epi16(delta, delta));
				delta2 = _mm_add_epi16(delta2, _mm_set1_epi16(1));
				delta2 = _mm_srai_epi16(delta2, 2);
				delta2=_mm_min_epi16(delta2,_mm_set1_epi16(tcd2));
				delta2=_mm_max_epi16(delta2,_mm_set1_epi16(-tcd2));
				dv5lo = _mm_unpacklo_epi8(_mm_srli_si128(x[0], 5), _mm_srli_si128(x[1], 5)); // [xxxxxxxxxxxx|16|06|15|05]
				dv5hi = _mm_unpacklo_epi8(_mm_srli_si128(x[2], 5), _mm_srli_si128(x[3], 5)); // [xxxxxxxxxxxx|36|26|35|25]
				dv5 = _mm_unpacklo_epi16(dv5lo, dv5hi); // [xxxxxxxx|36|26|16|06|35|25|15|05]
				dv5 = _mm_unpacklo_epi8(dv5, _mm_setzero_si128()); // [xxxx|35|25|15|05]
				dv5 = _mm_add_epi16(dv5, delta2); // [xxxx|d35|d25|d15|d05]
			}

			// line 0,1
			dv34lo = _mm_unpacklo_epi32(dv34, _mm_setzero_si128());	// [000|000|d14|d13|000|000|d04|d03]
			dv34lo = _mm_slli_si128(dv34lo, 2);	// [000|d14|d13|000|000|d04|d03|000]
			xdeltalo = _mm_unpacklo_epi8(xdeltalo, _mm_setzero_si128()); // [x15|x14|x13|x12|x05|x04|x03|x02]
			xdeltalo = _mm_and_si128(xdeltalo, mask_x25_epi16);// [x15|000|000|x12|x05|000|000|x02]
			dvlo = _mm_or_si128(xdeltalo, dv34lo); // [x15|d14|d13|x12|x05|d04|d03|x02]
			if ( b_filter_p ) {
				static const __m128i mask_x2_epi16 = {
					0xff, 0xff, 0, 0, 0, 0, 0, 0, 
					0xff, 0xff, 0, 0, 0, 0, 0, 0,
				};
				dv2lo = _mm_unpacklo_epi16(dv2, _mm_setzero_si128()); // [000|d32|000|d22|000|d12|000|d02]
				dv2lo = _mm_unpacklo_epi32(dv2lo, _mm_setzero_si128()); // [000|000|000|d12|000|000|000|d02]
				dvlo = _mm_andnot_si128(mask_x2_epi16, dvlo);
				dvlo = _mm_or_si128(dvlo, dv2lo); // [x15|d14|d13|d12|x05|d04|d03|d02]
			}
			if ( b_filter_q ) {
				static const __m128i mask_x5_epi16 = {
					0, 0, 0, 0, 0, 0, 0xff, 0xff, 
					0, 0, 0, 0, 0, 0, 0xff, 0xff,
				};
				dv5lo = _mm_unpacklo_epi16(dv5, _mm_setzero_si128()); // [000|d35|000|d25|000|d15|000|d05]
				dv5lo = _mm_unpacklo_epi32(dv5lo, _mm_setzero_si128()); // [000|000|000|d15|000|000|000|d05]
				dv5lo = _mm_slli_si128(dv5lo, 6); // [d15|000|000|000|d05|000|000|000]
				dvlo = _mm_andnot_si128(mask_x5_epi16, dvlo);
				dvlo = _mm_or_si128(dvlo, dv5lo); // [d15|d14|d13|x12|d05|d04|d03|x02]
			}
			// line 2,3
			dv34hi = _mm_unpackhi_epi32(dv34, _mm_setzero_si128());	// [000|000|d24|d23|000|000|d24|d23]
			dv34hi = _mm_slli_si128(dv34hi, 2);	// [000|d24|d23|000|000|d24|d23|000]
			xdeltahi = _mm_unpacklo_epi8(xdeltahi, _mm_setzero_si128()); // [x35|x34|x33|x32|x25|x24|x23|x22]
			xdeltahi = _mm_and_si128(xdeltahi, mask_x25_epi16);// [x35|000|000|x32|x25|000|000|x22]
			dvhi = _mm_or_si128(xdeltahi, dv34hi); // [x35|d34|d33|x32|x25|d24|d23|x22]
			if ( b_filter_p ) {
				static const __m128i mask_x2_epi16 = {
					0xff, 0xff, 0, 0, 0, 0, 0, 0, 
					0xff, 0xff, 0, 0, 0, 0, 0, 0,
				};
				dv2hi = _mm_unpacklo_epi16(dv2, _mm_setzero_si128()); // [000|d32|000|d22|000|d12|000|d02]
				dv2hi = _mm_unpackhi_epi32(dv2hi, _mm_setzero_si128()); // [000|000|000|d32|000|000|000|d22]
				dvhi = _mm_andnot_si128(mask_x2_epi16, dvhi);
				dvhi = _mm_or_si128(dvhi, dv2hi); // [x35|d34|d33|d32|x25|d24|d23|d22]
			}
			if ( b_filter_q ) {
				static const __m128i mask_x5_epi16 = {
					0, 0, 0, 0, 0, 0, 0xff, 0xff, 
					0, 0, 0, 0, 0, 0, 0xff, 0xff,
				};
				dv5hi = _mm_unpacklo_epi16(dv5, _mm_setzero_si128()); // [000|d35|000|d25|000|d15|000|d05]
				dv5hi = _mm_unpackhi_epi32(dv5hi, _mm_setzero_si128()); // [000|000|000|d35|000|000|000|d25]
				dv5hi = _mm_slli_si128(dv5hi, 6); // [d35|000|000|000|d25|000|000|000]
				dvhi = _mm_andnot_si128(mask_x5_epi16, dvhi);
				dvhi = _mm_or_si128(dvhi, dv5hi); // [d35|d34|d33|d32|d25|d24|d23|d22]
			}
			// commbo
			dv = _mm_packus_epi16(dvlo, dvhi);	// [d35|d34|d33|d32|d25|d24|d23|d22|d15|d14|d13|d12|d05|d04|d03|d02]
			// check
#ifdef _DEBUG
			if ( 1 ) {
				pixel* pix_src = pix;
				__m128i t = dv;
				for ( i = 0; i < 4; i++ )
				{
					int delta, delta1, delta2;
					pixel src[8], d[4];
					uint32_t dright, dopt;
					int j;
					for( j = 0; j < 8; j ++ )
						src[j] = pix_src[j-4];
					d[0] = src[2];
					d[1] = src[3];
					d[2] = src[4];
					d[3] = src[5];
					delta = (9*(src[4]-src[3]) - 3*(src[5]-src[2]) + 8)>>4;
					if( LENTABS(delta) < i_tc_thresh )
					{
						delta = lent_clip( delta, -tc0, tc0);
						d[1] = lent_clip_uint8(src[3]+delta);
						d[2] = lent_clip_uint8(src[4]-delta);

						if( b_filter_p )
						{
							delta1 = ( ((src[1]+src[3]+1)>>1) - src[2] + delta )>>1;
							delta1 = lent_clip( delta1, -tcd2, tcd2 );
							d[0] = lent_clip_uint8(src[2]+delta1);
						}

						if( b_filter_q )
						{
							delta2 = ( ((src[4]+src[6]+1)>>1) - src[5] - delta )>>1;
							delta2 = lent_clip( delta2, -tcd2, tcd2 );
							d[3] = lent_clip_uint8(src[5]+delta2);
						}
					}
					dright =  (((uint32_t)d[0]) <<  0)
							| (((uint32_t)d[1]) <<  8)
							| (((uint32_t)d[2]) << 16)
							| (((uint32_t)d[3]) << 24);
					dopt = _mm_cvtsi128_si32(t);
					if ( (i_mask & (0x03 << (i*2))) == 0 ) {	// mask is 0
						dopt = (((uint32_t)src[2]) <<  0)
							| (((uint32_t)src[3]) <<  8)
							| (((uint32_t)src[4]) << 16)
							| (((uint32_t)src[5]) << 24);
					}
					t = _mm_srli_si128(t, 4);
					if ( dopt != dright ) {
						lent_log(NULL, LENT_LOG_ERROR, "deblock_h_luma_ssse3 weak filter optimize error at %d, i=%d, right=0x%08x, opt=0x%08x!\n", strong_h_filter_times, i, dright, dopt);
					}
					pix_src += stride;
				}
			}
#endif
			// store
			if ( i_mask & 0x03 ) {
#ifdef _DEBUG
				part_times[6]++;
#endif
				*(uint32_t*)(pix-2 + 0*stride) = _mm_cvtsi128_si32(dv);
			}
			if ( i_mask & 0x0c ) {
#ifdef _DEBUG
				part_times[7]++;
#endif
				*(uint32_t*)(pix-2 + 1*stride) = _mm_cvtsi128_si32(_mm_srli_si128(dv, 4));
			}
			if ( i_mask & 0x30 ) {
#ifdef _DEBUG
				part_times[8]++;
#endif
				*(uint32_t*)(pix-2 + 2*stride) = _mm_cvtsi128_si32(_mm_srli_si128(dv, 8));
			}
			if ( i_mask & 0xc0 ) {
#ifdef _DEBUG
				part_times[9]++;
#endif
				*(uint32_t*)(pix-2 + 3*stride) = _mm_cvtsi128_si32(_mm_srli_si128(dv, 12));
			}

		} else { // strong filter

			//PIX(-1) = lent_clip( ((int)src[1] + 2*src[2] + 2*src[3] + 2*src[4] + src[5] + 4)>>3, src[3]-tcm2, src[3]+tcm2 );
			//PIX( 0) = lent_clip( ((int)src[2] + 2*src[3] + 2*src[4] + 2*src[5] + src[6] + 4)>>3, src[4]-tcm2, src[4]+tcm2 );

			//PIX(-2) = lent_clip( ((int)src[1] + src[2] + src[3] + src[4] + 2)>>2, src[2]-tcm2, src[2]+tcm2 );
			//PIX( 1) = lent_clip( ((int)src[3] + src[4] + src[5] + src[6] + 2)>>2, src[5]-tcm2, src[5]+tcm2 );

			//PIX(-3) = lent_clip( ((int)2*src[0] + 3*src[1] + src[2] + src[3] + src[4] + 4)>>3, src[1]-tcm2, src[1]+tcm2 );
			//PIX( 2) = lent_clip( ((int)src[3] + src[4] + src[5] + 3*src[6] + 2*src[7] + 4)>>3, src[6]-tcm2, src[6]+tcm2 );
			
			static const __m128i coeff[] = {
				{2, 3, 1, 1, 1, 0, 0, 0,	0, 0, 0, 1, 1, 1, 3, 2},
				{0, 2, 2, 2, 2, 0, 0, 0,	0, 0, 0, 2, 2, 2, 2, 0},
				{0, 1, 2, 2, 2, 1, 0, 0,	0, 0, 1, 2, 2, 2, 1, 0},
			};
			__m128i mIn = _mm_set1_epi16(-tcm2), mAx = _mm_set1_epi16(tcm2);
			uint8_t * p = pix - 4;
#ifdef _DEBUG
			part_times[10]++;
#endif
			for ( i = 0; i < 4; i++ ) {
				static const __m128i shuffle = {0x80, 0x80, 4, 5, 8, 9, 12, 13, 14, 15, 10, 11, 6, 7, 0x80, 0x80};
				static const __m128i mask_x07_epi16 = {0xff, 0xff, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff};
				__m128i x, d16, d0716, d25, d34, d2534, d;
				x = _mm_loadl_epi64((__m128i*)p);	// [00|00|00|00|00|00|00|00|x7|x6|x5|x4|x3|x2|x1|x0]
				x = _mm_unpacklo_epi64(x, x);			// [x7|x6|x5|x4|x3|x2|x1|x0|x7|x6|x5|x4|x3|x2|x1|x0]
				d16 = _mm_maddubs_epi16(x, coeff[0]);	// [d6hh|d6hl|d6lh|d6ll|d1hh|d1hl|d1lh|d1ll]
				d16 = _mm_hadd_epi16(d16, d16);			// [d6h|d6l|d1h|d1l|d6h|d6l|d1h|d1l]
				d0716 = _mm_unpacklo_epi64(_mm_setzero_si128(), d16);	// [d6h|d6l|d1h|d1l|000|000|000|000]
				d0716 = _mm_hadd_epi16(d0716, d0716);	// [d6|d1|00|00|d6|d1|00|00]
				d25 = _mm_maddubs_epi16(x, coeff[1]);	// [d5hh|d5hl|d5lh|d5ll|d2hh|d2hl|d2lh|d2ll]
				d25 = _mm_hadd_epi16(d25, d25);			// [d5h|d5l|d2h|d2l|d5h|d5l|d2h|d2l]
				d34 = _mm_maddubs_epi16(x, coeff[2]);	// [d4hh|d4hl|d4lh|d4ll|d3hh|d3hl|d3lh|d3ll]
				d34 = _mm_hadd_epi16(d34, d34);			// [d4h|d4l|d3h|d3l|d4h|d4l|d3h|d3l]
				d2534 = _mm_unpacklo_epi64(d25, d34);	// [d4h|d4l|d3h|d3l|d5h|d5l|d2h|d2l]
				d2534 = _mm_hadd_epi16(d2534, d2534);	// [d4|d3|d5|d2|d4|d3|d5|d2]
				d = _mm_unpacklo_epi64(d0716, d2534);	// [d4|d3|d5|d2|d6|d1|00|00]
				d = _mm_add_epi16(d, _mm_set1_epi16(4));
				d = _mm_srai_epi16(d, 3);
				d = _mm_shuffle_epi8(d, shuffle);		// [00|d6|d5|d4|d3|d2|d1|00]
				x = _mm_unpacklo_epi8(x, _mm_setzero_si128());	// [x7|x6|x5|x4|x3|x2|x1|x0]
				d = _mm_sub_epi16(d, x);
				d = _mm_min_epi16(d, mAx);
				d = _mm_max_epi16(d, mIn);
				d = _mm_add_epi16(x, d);
				d = _mm_andnot_si128(mask_x07_epi16, d);
				x = _mm_and_si128(x, mask_x07_epi16);
				d = _mm_or_si128(d, x);					// [x7|d6|d5|d4|d3|d2|d1|x0]
				d = _mm_packus_epi16(d, d);				// [x7|d6|d5|d4|d3|d2|d1|x0|x7|d6|d5|d4|d3|d2|d1|x0]
			// check
#ifdef _DEBUG
				if ( 1 ) {

					pixel src[8], t[8];
					uint64_t dright, dopt;
					int j;
					for( j = 0; j < 8; j ++ )
						src[j] = pix[-4 + i*stride + j];

					t[3] = lent_clip( ((int)src[1] + 2*src[2] + 2*src[3] + 2*src[4] + src[5] + 4)>>3, src[3]-tcm2, src[3]+tcm2 );
					t[4] = lent_clip( ((int)src[2] + 2*src[3] + 2*src[4] + 2*src[5] + src[6] + 4)>>3, src[4]-tcm2, src[4]+tcm2 );

					t[2] = lent_clip( ((int)src[1] + src[2] + src[3] + src[4] + 2)>>2, src[2]-tcm2, src[2]+tcm2 );
					t[5] = lent_clip( ((int)src[3] + src[4] + src[5] + src[6] + 2)>>2, src[5]-tcm2, src[5]+tcm2 );

					t[1] = lent_clip( ((int)2*src[0] + 3*src[1] + src[2] + src[3] + src[4] + 4)>>3, src[1]-tcm2, src[1]+tcm2 );
					t[6] = lent_clip( ((int)src[3] + src[4] + src[5] + 3*src[6] + 2*src[7] + 4)>>3, src[6]-tcm2, src[6]+tcm2 );

					t[0] = src[0];
					t[7] = src[7];

					dright =  (((uint64_t)t[0]) <<  0)
							| (((uint64_t)t[1]) <<  8)
							| (((uint64_t)t[2]) << 16)
							| (((uint64_t)t[3]) << 24)
							| (((uint64_t)t[4]) << 32)
							| (((uint64_t)t[5]) << 40)
							| (((uint64_t)t[6]) << 48)
							| (((uint64_t)t[7]) << 56);
					_mm_storel_epi64((__m128i*)&dopt, d);
					if ( dopt != dright ) {
						lent_log(NULL, LENT_LOG_ERROR, "deblock_h_luma_ssse3 strong filter optimize error at %d, i=%d, right=0x%I64x, opt=0x%I64x!\n", strong_h_filter_times, i, dright, dopt);
					}

				}
#endif
				_mm_storel_epi64((__m128i*)p, d);
				p += stride;
			}
		}
	}
}
#undef JUD
#undef PIX

static void deblock_h_chroma_ssse3( pixel *pix, pixel *judge, int stride, int beta, int tc0 )
{
#if 0
	deblock_chroma_ssse3( pix, 1, stride, beta, tc0 );
#else
	// int delta = ( (( m4 - m3 ) << 2 ) + m2 - m5 + 4 ) >> 3
	//           = ((m4 - m3) * 4 + m2 - m5 + 4) / 8
	//           = (1*m2 - 4*m3 + 4*m4 - 1*m5 + 4) / 8
	static const __m128i coeff = {
		1, -4, 4, -1,
		1, -4, 4, -1,
		1, -4, 4, -1,
		1, -4, 4, -1,
	};
	static const __m128i shuffle_x34 = {
		 1, 0x80,  2, 0x80,
		 5, 0x80,  6, 0x80,
		 9, 0x80, 10, 0x80,
		13, 0x80, 14, 0x80,
	};
	__m128i /*coeff,*/ xlo, xhi, x, d, dn;
#ifdef _DEBUG
	static int chroma_h_filter_times = 0;
	pixel right3[4], right4[4];
	++chroma_h_filter_times;
	{	// self test
		int xstride = 1, ystride = stride;
		int i;
		pixel * p = pix;
		for( i = 0; i < 4; i ++ )
		{
			pixel m4 = p[( 0)*xstride];
			pixel m3 = p[(-1)*xstride];
			pixel m5 = p[( 1)*xstride];
			pixel m2 = p[(-2)*xstride];
			int delta = ( (( m4 - m3 ) << 2 ) + m2 - m5 + 4 ) >> 3;

			delta = lent_clip( delta, -tc0, tc0 );
			right3[i] = lent_clip_uint8(m3+delta);
			right4[i] = lent_clip_uint8(m4-delta);

			p += ystride;
		}
	}
#endif
	xlo = _mm_unpacklo_epi32(_mm_srli_si128(_mm_loadl_epi64((__m128i*)(pix + 0*stride - 4)), 2), _mm_srli_si128(_mm_loadl_epi64((__m128i*)(pix + 1*stride - 4)), 2));
	xhi = _mm_unpacklo_epi32(_mm_srli_si128(_mm_loadl_epi64((__m128i*)(pix + 2*stride - 4)), 2), _mm_srli_si128(_mm_loadl_epi64((__m128i*)(pix + 3*stride - 4)), 2));
	x = _mm_unpacklo_epi64(xlo, xhi); // [x35|x34|x33|x32|x25|x24|x23|x22|x15|x14|x13|x12|x05|x04|x03|x02]
//	coeff = _mm_shuffle_epi32(_mm_cvtsi32_si128(0xff04fc01), 0); // 1, -4, 4, -1,
	d = _mm_maddubs_epi16(x, coeff);
	d = _mm_hadd_epi16(d, d); // [d3|d2|d1|d0|d3|d2|d1|d0]
	d = _mm_add_epi16(d, _mm_set1_epi16(4));
	d = _mm_srai_epi16(d, 3);
	d = _mm_max_epi16(d, _mm_set1_epi16(-tc0));
	d = _mm_min_epi16(d, _mm_set1_epi16(tc0));
	dn = _mm_sub_epi16(_mm_setzero_si128(), d); // [-d3|-d2|-d1|-d0|-d3|-d2|-d1|-d0]
	d = _mm_unpacklo_epi16(d, dn); // [-d3|d3|-d2|d2|-d1|d1|-d0|d0]
	x = _mm_shuffle_epi8(x, shuffle_x34); // [x34|x33|x24|x23|x14|x13|x04|x03]
	x = _mm_add_epi16(x, d);
	x = _mm_packus_epi16(x, x); // epi8: [x34|x33|x24|x23|x14|x13|x04|x03|x34|x33|x24|x23|x14|x13|x04|x03]
	*(uint16_t*)(pix + 0*stride - 1) = _mm_extract_epi16(x, 0);
	*(uint16_t*)(pix + 1*stride - 1) = _mm_extract_epi16(x, 1);
	*(uint16_t*)(pix + 2*stride - 1) = _mm_extract_epi16(x, 2);
	*(uint16_t*)(pix + 3*stride - 1) = _mm_extract_epi16(x, 3);
#ifdef _DEBUG
	{ // self test
		uint32_t rlo, rhi, olo, ohi;
		rlo = ((uint32_t)right3[0] <<  0) | ((uint32_t)right4[0] <<  8)
			| ((uint32_t)right3[1] << 16) | ((uint32_t)right4[1] << 24);
		rhi = ((uint32_t)right3[2] <<  0) | ((uint32_t)right4[2] <<  8)
			| ((uint32_t)right3[3] << 16) | ((uint32_t)right4[3] << 24);
		olo = (uint32_t)_mm_cvtsi128_si32(x);
		ohi = (uint32_t)_mm_cvtsi128_si32(_mm_srli_si128(x, 4));
		if ( olo != rlo || ohi != rhi ) {
			lent_log(NULL, LENT_LOG_ERROR, "deblock_h_chroma_ssse3 filter optimize error at %d, r=0x%08x%08x, o=0x%08x%08x!\n", chroma_h_filter_times, rhi, rlo, ohi, olo);
		}
	}
#endif
#endif
}

void lent_deblock_init_x86(HEVCDeblockContext *pf, unsigned int sse)
{
	if ( sse >= 20 )
	{
		pf->deblock_luma[0] = deblock_h_luma_sse2;
		pf->deblock_luma[1] = deblock_v_luma_sse2;
		pf->deblock_chroma[0] = deblock_h_chroma_sse2;
		pf->deblock_chroma[1] = deblock_v_chroma_sse2;
	}
	if(sse>=31)
	{
		pf->deblock_luma[0] = deblock_h_luma_ssse3;
		pf->deblock_luma[1] = deblock_v_luma_ssse3;
		pf->deblock_chroma[0] = deblock_h_chroma_ssse3;
		pf->deblock_chroma[1] = deblock_v_chroma_sse2;
	}
}




#endif