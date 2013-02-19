//****************************************************************
//cm_predict.c
//包含帧内预测相关的sse代码实现
//****************************************************************
#include "config.h"

#if ARCH_X86_32
#include "smmintrin.h"

#include "../../internal.h"
#pragma warning(disable:4133)

#define SRC(x,y) src[(x)+(y)*i_src]
#define SSE_STRIDE (i_src>>4)
#define MMX_STRIDE (i_src>>3)

extern const int invAngTable[9];
extern const int angTable[9];
extern const int intra_filter[5];

void predict_32x32_h_ssse3( pixel *src, int i_src ) 
{
	int y;
	__m128i *dst = (__m128i *)src;
	int i_src_s3 = i_src>>3;
	int i_src_s4 = SSE_STRIDE;

	for( y = 0; y < 32; y += 2 )
	{
		__m128i v;
		
		v = _mm_set1_epi8( SRC(-1,y) );
		*(dst) = v;
		*(dst+1) = v;

		v = _mm_set1_epi8( SRC(-1,y+1) );
		*(dst+i_src_s4) = v;
		*(dst+(i_src_s4+1)) = v;

		dst += i_src_s3;
	}
}

void predict_16x16_h_ssse3( pixel *src, int i_src ) 
{
	int y;
	__m128i *dst = (__m128i *)src;
	int i_src_s2 = i_src >> 2;
	int i_src_s4 = SSE_STRIDE;

	for( y = 0; y < 16; y += 4 )
	{
		*(dst+i_src_s4*0) = _mm_set1_epi8( SRC(-1,y) );
		*(dst+i_src_s4*1) = _mm_set1_epi8( SRC(-1,y+1) );
		*(dst+i_src_s4*2) = _mm_set1_epi8( SRC(-1,y+2) );
		*(dst+i_src_s4*3) = _mm_set1_epi8( SRC(-1,y+3) );

		dst += i_src_s2;
	}
}

void predict_8x8_h_ssse3( pixel *src, int i_src ) 
{
	__m64 *dst = (__m64 *)src;
	int i_src_s3 = i_src >> 3;

	_mm_empty();
	*(dst+i_src_s3*0) = _mm_set1_pi8( SRC(-1,0) );
	*(dst+i_src_s3*1) = _mm_set1_pi8( SRC(-1,1) );
	*(dst+i_src_s3*2) = _mm_set1_pi8( SRC(-1,2) );
	*(dst+i_src_s3*3) = _mm_set1_pi8( SRC(-1,3) );
	*(dst+i_src_s3*4) = _mm_set1_pi8( SRC(-1,4) );
	*(dst+i_src_s3*5) = _mm_set1_pi8( SRC(-1,5) );
	*(dst+i_src_s3*6) = _mm_set1_pi8( SRC(-1,6) );
	*(dst+i_src_s3*7) = _mm_set1_pi8( SRC(-1,7) );
	_mm_empty();
}

void predict_32x32_v_ssse3( pixel *src, int i_src )
{
	int y;
	__m128i *dst = (__m128i *)src;
	__m128i v1, v2;
	int i_src_s3 = i_src >> 3;
	int i_src_s4 = SSE_STRIDE;

	v1 = *(dst-i_src_s4);
	v2 = *(dst-i_src_s4+1);

	for( y = 0; y < 32; y += 2 )
	{
		*dst = v1;
		*(dst+1) = v2;
		*(dst+i_src_s4) = v1;
		*(dst+i_src_s4+1) = v2;

		dst += i_src_s3;
	}
}

void predict_16x16_v_ssse3( pixel *src, int i_src )
{
	__m128i *dst = (__m128i *)src;
	int i_src_s4 = SSE_STRIDE;
	__m128i v1 = *(dst-i_src_s4);

#define COPY(y) \
	*(dst+i_src_s4*(y+0)) = v1; \
	*(dst+i_src_s4*(y+1)) = v1; \
	*(dst+i_src_s4*(y+2)) = v1; \
	*(dst+i_src_s4*(y+3)) = v1

	COPY(0);
	COPY(4);
	COPY(8);
	COPY(12);
#undef COPY
}

void predict_8x8_v_ssse3( pixel *src, int i_src )
{
	__m64 *dst = (__m64 *)src;
	__m64 v1;
	int i_src_s3 = MMX_STRIDE;
	int i_src_s4 = SSE_STRIDE;

#define COPY(y) \
	*(dst+i_src_s3*(y+0)) = v1; \
	*(dst+i_src_s3*(y+1)) = v1; \
	*(dst+i_src_s3*(y+2)) = v1; \
	*(dst+i_src_s3*(y+3)) = v1

	_mm_empty();
	v1 = *(dst-i_src_s3);
	COPY(0);
	COPY(4);
	_mm_empty();
#undef COPY
}

void predict_32x32_dc_ssse3( pixel *src, int i_src )
{
	int i, y;
	int sum = 0;
	__m128i v;
	__m128i *dst = (__m128i *)src;
	int i_src_s3 = i_src >> 3;

	for( i = 0; i < 32; i++ )
	{
		sum += src[-1 + i * i_src];
		sum += src[i - i_src];
	}

	sum = (sum + 32) >> 6;
	v = _mm_set1_epi8( (pixel)sum );

	for( y = 0; y < 32; y += 2 )
	{
		*dst = v;
		*(dst+1) = v;
		*(dst+SSE_STRIDE) = v;
		*(dst+(SSE_STRIDE+1)) = v;

		dst += i_src_s3;
	}
}

void predict_16x16_dc_ssse3( pixel *src, int i_src )
{
	int i;
	int sum = 0;
	__m128i v;
	__m128i *dst = (__m128i *)src;
	int i_src_s4 = SSE_STRIDE;

	for( i = 0; i < 16; i++ )
	{
		sum += src[-1 + i * i_src];
		sum += src[i - i_src];
	}

	sum = (sum + 16) >> 5;
	v = _mm_set1_epi8( (pixel)sum );

#define COPY(y) \
	*(dst+i_src_s4*(y+0)) = v; \
	*(dst+i_src_s4*(y+1)) = v; \
	*(dst+i_src_s4*(y+2)) = v; \
	*(dst+i_src_s4*(y+3)) = v

	COPY(0);
	COPY(4);
	COPY(8);
	COPY(12);
#undef COPY
}

void predict_8x8_dc_ssse3( pixel *src, int i_src )
{
	int i;
	int sum = 0;
	__m64 v;
	__m64 *dst = (__m64 *)src;
	int i_src_s3 = MMX_STRIDE;

	for( i = 0; i < 8; i++ )
	{
		sum += src[-1 + i * i_src];
		sum += src[i - i_src];
	}

	_mm_empty();
	sum = (sum + 8) >> 4;
	v = _mm_set1_pi8( (pixel)sum );

#define COPY(y) \
	*(dst+i_src_s3*(y+0)) = v; \
	*(dst+i_src_s3*(y+1)) = v; \
	*(dst+i_src_s3*(y+2)) = v; \
	*(dst+i_src_s3*(y+3)) = v

	COPY(0);
	COPY(4);
#undef COPY
	_mm_empty();
}

void predict_32x32_edge_filter_hor_ssse3( pixel *src, int i_src )
{
	pixel *src1 = src - i_src;
	__m128i rp = _mm_set1_epi16( SRC(-1,-1) );
	__m128i x0 = _mm_loadl_epi64((__m128i *) src);
	__m128i x1 = _mm_loadl_epi64((__m128i *)(src + 8));
	__m128i x2 = _mm_loadl_epi64((__m128i *)(src + 16));
	__m128i x3 = _mm_loadl_epi64((__m128i *)(src + 24));
	__m128i y0 = _mm_loadl_epi64((__m128i *) src1);
	__m128i y1 = _mm_loadl_epi64((__m128i *)(src1 + 8));
	__m128i y2 = _mm_loadl_epi64((__m128i *)(src1 + 16));
	__m128i y3 = _mm_loadl_epi64((__m128i *)(src1 + 24));
	__m128i zero = _mm_setzero_si128();

	x0 = _mm_unpacklo_epi8( x0, zero );
	x1 = _mm_unpacklo_epi8( x1, zero );
	x2 = _mm_unpacklo_epi8( x2, zero );
	x3 = _mm_unpacklo_epi8( x3, zero );
	y0 = _mm_unpacklo_epi8( y0, zero );
	y1 = _mm_unpacklo_epi8( y1, zero );
	y2 = _mm_unpacklo_epi8( y2, zero );
	y3 = _mm_unpacklo_epi8( y3, zero );

	y0 = _mm_srai_epi16( _mm_sub_epi16( y0, rp ), 1 );
	y1 = _mm_srai_epi16( _mm_sub_epi16( y1, rp ), 1 );
	y2 = _mm_srai_epi16( _mm_sub_epi16( y2, rp ), 1 );
	y3 = _mm_srai_epi16( _mm_sub_epi16( y3, rp ), 1 );

	x0 = _mm_add_epi16( x0, y0 );
	x1 = _mm_add_epi16( x1, y1 );
	x2 = _mm_add_epi16( x2, y2 );
	x3 = _mm_add_epi16( x3, y3 );
	
	*(__m128i *) src     = _mm_packus_epi16( x0, x1 );
	*(__m128i *)(src+16) = _mm_packus_epi16( x2, x3 );
}

void predict_16x16_edge_filter_hor_ssse3( pixel *src, int i_src )
{
	pixel *src1 = src - i_src;
	__m128i rp = _mm_set1_epi16( SRC(-1,-1) );
	__m128i x0 = _mm_loadl_epi64((__m128i *) src);
	__m128i x1 = _mm_loadl_epi64((__m128i *)(src + 8));
	__m128i y0 = _mm_loadl_epi64((__m128i *) src1);
	__m128i y1 = _mm_loadl_epi64((__m128i *)(src1 + 8));
	__m128i zero = _mm_setzero_si128();

	x0 = _mm_unpacklo_epi8( x0, zero );
	x1 = _mm_unpacklo_epi8( x1, zero );
	y0 = _mm_unpacklo_epi8( y0, zero );
	y1 = _mm_unpacklo_epi8( y1, zero );

	y0 = _mm_srai_epi16( _mm_sub_epi16( y0, rp ), 1 );
	y1 = _mm_srai_epi16( _mm_sub_epi16( y1, rp ), 1 );

	x0 = _mm_add_epi16( x0, y0 );
	x1 = _mm_add_epi16( x1, y1 );

	*(__m128i *)src = _mm_packus_epi16( x0, x1 );
}

void predict_8x8_edge_filter_hor_ssse3( pixel *src, int i_src )
{
	pixel *src1 = src - i_src;
	__m128i rp = _mm_set1_epi16( SRC(-1,-1) );
	__m128i x0 = _mm_loadl_epi64((__m128i *) src);
	__m128i y0 = _mm_loadl_epi64((__m128i *) src1);
	__m128i zero = _mm_setzero_si128();

	y0 = _mm_unpacklo_epi8( y0, zero );
	x0 = _mm_unpacklo_epi8( x0, zero );

	y0 = _mm_srai_epi16( _mm_sub_epi16( y0, rp ), 1 );

	x0 = _mm_add_epi16( x0, y0 );

	_mm_storel_epi64( (__m128i *)src, _mm_packus_epi16( x0, x0 ) );
}

#define F3(a,b,c) (((a)+((b)<<1)+(c)+2)>>2)
static void predict_ang_filter_strong( pixel *dst, pixel *src, int i_src, int i_left, int i_top,
																					 int mode_angle_abs, int intra_pred_order )
{
	const int i_size = 32, i_size2 = 64, shift = 6;
	pixel tl = SRC(-1,-1);
	pixel tr = intra_pred_order >= 18 ? SRC(i_size2-1,-1) : SRC(-1,i_size2-1);
	pixel bl = intra_pred_order >= 18 ? SRC(-1,i_size2-1) : SRC(i_size2-1,-1);
	int i_inv_angle = invAngTable[mode_angle_abs];
	int i_inv_angle_sum = 128; 
	int i;

	dst[-1] = tl;
	for( i = 0; i < i_top; i ++ )
	{
		dst[i] = ((i_size2-i-1)*tl + (i+1)*tr + i_size) >> shift;
	}
	dst --;
	for( i = -1; i >= i_left; i -- )
	{
		int idx;

		i_inv_angle_sum += i_inv_angle;
		idx = (i_inv_angle_sum>>8)-1;
		dst[i] = ((i_size2+idx+1)*tl - (idx+1)*bl + i_size) >> shift;
	}
}
static inline void predict_ang_filter_sse2( pixel *dst, pixel *src, int i_src, int i_left, int i_top,
																					 int mode_angle_abs, int intra_pred_order )
{
	int i_inv_angle = invAngTable[mode_angle_abs];
	int i_inv_angle_sum = 128; 
	int i;

	dst[-1] = F3( SRC(0,-1), SRC(-1,-1), SRC(-1,0) );

	if( intra_pred_order >= 18 )
	{
		for( i = 0; i < i_top; i ++ )
		{
			dst[i] = F3( SRC(i-1,-1), SRC(i,-1), SRC(i+1,-1) );
		}
		dst --;
		for( i = -1; i >= i_left; i -- )
		{
			int idx;

			i_inv_angle_sum += i_inv_angle;
			idx = (i_inv_angle_sum>>8)-1;
			dst[i] = F3( SRC(-1,idx-1), SRC(-1,idx), SRC(-1,idx+1) );
		}
	}
	else
	{
		for( i = 0; i < i_top; i ++ )
		{
			dst[i] = F3( SRC(-1,i-1), SRC(-1,i), SRC(-1,i+1) );
		}
		dst --;
		for( i = -1; i >= i_left; i -- )
		{
			int idx;

			i_inv_angle_sum += i_inv_angle;
			idx = (i_inv_angle_sum>>8)-1;
			dst[i] = F3( SRC(idx-1,-1), SRC(idx,-1), SRC(idx+1,-1) );
		}
	}
}
#undef F3

static inline void transpose8_ssse3( pixel *dst, int i_dst, pixel *src, int i_src )
{
	__m128i x[4], d[8];

	d[0]=_mm_loadl_epi64(src+i_src*0);
	d[1]=_mm_loadl_epi64(src+i_src*1);
	d[2]=_mm_loadl_epi64(src+i_src*2);
	d[3]=_mm_loadl_epi64(src+i_src*3);
	d[4]=_mm_loadl_epi64(src+i_src*4);
	d[5]=_mm_loadl_epi64(src+i_src*5);
	d[6]=_mm_loadl_epi64(src+i_src*6);
	d[7]=_mm_loadl_epi64(src+i_src*7);

	x[0] = _mm_unpacklo_epi8( d[0], d[1] );
	x[1] = _mm_unpacklo_epi8( d[2], d[3] );
	x[2] = _mm_unpacklo_epi8( d[4], d[5] );
	x[3] = _mm_unpacklo_epi8( d[6], d[7] );

	d[0] = _mm_unpacklo_epi16( x[0], x[1] );
	d[1] = _mm_unpackhi_epi16( x[0], x[1] );
	d[2] = _mm_unpacklo_epi16( x[2], x[3] );
	d[3] = _mm_unpackhi_epi16( x[2], x[3] );

	x[0] = _mm_unpacklo_epi32( d[0], d[2] );
	x[2] = _mm_unpackhi_epi32( d[0], d[2] );
	x[1] = _mm_unpacklo_epi32( d[1], d[3] );
	x[3] = _mm_unpackhi_epi32( d[1], d[3] );

	M64(dst+i_dst*0) = x[0].m128i_i64[0];
	M64(dst+i_dst*1) = x[0].m128i_i64[1];
	M64(dst+i_dst*2) = x[2].m128i_i64[0];
	M64(dst+i_dst*3) = x[2].m128i_i64[1];
	M64(dst+i_dst*4) = x[1].m128i_i64[0];
	M64(dst+i_dst*5) = x[1].m128i_i64[1];
	M64(dst+i_dst*6) = x[3].m128i_i64[0];
	M64(dst+i_dst*7) = x[3].m128i_i64[1];
}

static inline void transpose16_ssse3( pixel *dst, int i_dst, pixel *src, int i_src )
{
	//ALIGNED_16( pixel buf[16*8] );
	STACK_ALIGN( 16, pixel, buf, 16*8 )
	int i;

	transpose8_ssse3( dst, i_dst, src, i_src );
	transpose8_ssse3( dst + i_dst*8 + 8, i_dst, src + i_src*8 + 8, i_src );
	transpose8_ssse3( buf, 16, src + i_src*8, i_src );
	transpose8_ssse3( dst + i_dst*8, i_dst, src + 8, i_src );

	for( i = 0; i < 8; i ++ )
		CP64( dst + 8 + i*i_dst, buf + i*16 );
}

static inline void transpose32_ssse3( pixel *dst, int i_dst, pixel *src, int i_src )
{
	//ALIGNED_16( pixel buf[16*16] );
	STACK_ALIGN( 16, pixel, buf, 16*16 )
	int i;

	transpose16_ssse3( dst, i_dst, src, i_src );
	transpose16_ssse3( dst + i_dst*16 + 16, i_dst, src + i_src*16 + 16, i_src );
	transpose16_ssse3( buf, 16, src + i_src*16, i_src );
	transpose16_ssse3( dst + i_dst*16, i_dst, src + 16, i_src );

	for( i = 0; i < 16; i ++ )
		CP128( dst + 16 + i*i_dst, buf + i*16 );
}
void predict_ang_internal_sse2( pixel *src, int i_src, int intra_pred_order, int i_log2_size, int filter )
{
	int i_size = 1 << i_log2_size; //for test
	int mode_angle = intra_pred_order >= 18 ? 
		intra_pred_order - 26 :
	10 - intra_pred_order;
	int mode_angle_abs = LENTABS( mode_angle );
	int i_angle = mode_angle < 0 ? -angTable[mode_angle_abs] : angTable[mode_angle_abs];
	int shift_m2 = i_log2_size - 2;
	int diff = LENTMIN( LENTABS( intra_pred_order-10 ), LENTABS( intra_pred_order-26 ) );
	int k, l;
	const int top_offset = 16/*for_align*/+32/*left*/;
	//ALIGNED_16( pixel ref_pix[16/*for_align*/+32/*left*/+64/*top*/] );
	STACK_ALIGN( 16, pixel, ref_pix, 16/*for_align*/+32/*left*/+64/*top*/ )
	int ref_pix_l, ref_pix_r;
	pixel *dst;

	int i_pos = 0;

	assert( i_size >= 8 );

	//get refPix
	dst = ref_pix+top_offset;
	if( i_angle < 0 )
	{ 
		int pix_max = (i_size*(-i_angle))>>5;

		ref_pix_l =  - pix_max;
		ref_pix_r = i_size;

		if( filter && diff > intra_filter[shift_m2] )
		{
			int b_strong = 0;

			if( filter > 1 )
			{
				int i_idx = (i_size<<1)-1;
				int i_left = SRC(-1,i_idx)+SRC(-1,-1)-2*SRC(-1,i_size-1);
				int i_top  = SRC(i_idx,-1)+SRC(-1,-1)-2*SRC(i_size-1,-1);
				int b_left = i_left < 8 && i_left > -8;
				int b_top  = i_top  < 8 && i_top  > -8;

				b_strong = b_top && b_left;
			}

			if( !b_strong )
				predict_ang_filter_sse2( ref_pix + top_offset, src, i_src, ref_pix_l, ref_pix_r,
					mode_angle_abs, intra_pred_order );
			else
				predict_ang_filter_strong( ref_pix + top_offset, src, i_src, ref_pix_l, ref_pix_r,
					mode_angle_abs, intra_pred_order );
		}
		else
		{
			int i_inv_angle = invAngTable[mode_angle_abs];
			int i_inv_angle_sum = 128; 

			if( intra_pred_order >= 18 )
			{
				memcpy( dst-1, src-i_src-1, i_size+1 );
				dst --;
				for( k = 1; k <= pix_max; k ++ )
				{
					i_inv_angle_sum += i_inv_angle;
					dst[-k] = SRC(-1,(i_inv_angle_sum>>8)-1);
				}
			}
			else
			{
				for( k = -1; k < i_size; k ++ )
					dst[k] = SRC(-1,k);
				dst --;
				for( k = 1; k <= pix_max; k ++ )
				{
					i_inv_angle_sum += i_inv_angle;
					dst[-k] = SRC((i_inv_angle_sum>>8)-1,-1);
				}
			}
		}
	}
	else
	{
		ref_pix_l = 0;
		ref_pix_r = 2*i_size;

		if( filter && diff > intra_filter[shift_m2] )
		{
			int b_strong = 0;

			if( filter > 1 )
			{
				int i_idx = (i_size<<1)-1;
				int i_left = SRC(-1,i_idx)+SRC(-1,-1)-2*SRC(-1,i_size-1);
				int i_top  = SRC(i_idx,-1)+SRC(-1,-1)-2*SRC(i_size-1,-1);
				int b_left = i_left < 8 && i_left > -8;
				int b_top  = i_top  < 8 && i_top  > -8;

				b_strong = b_top && b_left;
			}

			if( !b_strong )
				predict_ang_filter_sse2( ref_pix + top_offset, src, i_src, ref_pix_l, ref_pix_r,
					mode_angle_abs, intra_pred_order );
			else
				predict_ang_filter_strong( ref_pix + top_offset, src, i_src, ref_pix_l, ref_pix_r,
					mode_angle_abs, intra_pred_order );
			ref_pix[top_offset+ref_pix_r-1] = intra_pred_order >= 18 ? 
				SRC(ref_pix_r-1,-1) : SRC(-1,ref_pix_r-1);
		}
		else
		{
			if( intra_pred_order >= 18 )
			{
				memcpy( dst, src-i_src, 2*i_size );
			}
			else
			{
				int pix_max = 2*i_size;
				for( k = 0; k < pix_max; k += 4 )
				{
					dst[k  ] = SRC(-1,k  );
					dst[k+1] = SRC(-1,k+1);
					dst[k+2] = SRC(-1,k+2);
					dst[k+3] = SRC(-1,k+3);
				}
			}
		}
	}

	//interception
	dst = src;
	for( k = 0; k < i_size; k ++ )
	{
		int i_pix, i_frac;

		i_pos += i_angle;
		i_pix  = i_pos >> 5;
		i_frac = i_pos & 31;

		if( i_frac )
		{
// 			for( l = 0; l < i_size; l += 4 )
// 			{
// 				int t = l+i_pix+top_offset;
// 				dst[l  ] = CLIP_PIXEL( ((32-i_frac) * ref_pix[t  ] + 
// 					i_frac * ref_pix[t+1] + 16) >> 5 );
// 				dst[l+1] = CLIP_PIXEL( ((32-i_frac) * ref_pix[t+1] + 
// 					i_frac * ref_pix[t+2] + 16) >> 5 );
// 				dst[l+2] = CLIP_PIXEL( ((32-i_frac) * ref_pix[t+2] + 
// 					i_frac * ref_pix[t+3] + 16) >> 5 );
// 				dst[l+3] = CLIP_PIXEL( ((32-i_frac) * ref_pix[t+3] + 
// 					i_frac * ref_pix[t+4] + 16) >> 5 );
// 			}
			__m128i zero = _mm_setzero_si128();
			__m128i f0 = _mm_set1_epi16( 32-i_frac );
			__m128i f1 = _mm_set1_epi16( i_frac );
			__m128i bias = _mm_set1_epi16( 16 );

			if( i_size == 8 )
			{
				l = 0;
				//for( l = 0; l < i_size; l += 8 )
				{
					int t = l+i_pix+top_offset;
					__m128i s0 = _mm_loadl_epi64( ref_pix + t );
					__m128i s1 = _mm_loadl_epi64( ref_pix + t+1 );
					__m128i t0, t1;

					t0 = _mm_unpacklo_epi8( s0, zero );
					t1 = _mm_unpacklo_epi8( s1, zero );

					s0 = _mm_mullo_epi16( t0, f0 );
					s1 = _mm_mullo_epi16( t1, f1 );

					t0 = _mm_add_epi16( _mm_add_epi16( s0, bias ), s1 );
					t1 = _mm_srai_epi16( t0, 5 );
					s0 = _mm_packus_epi16( t1, zero );

					_mm_storel_epi64( dst+l, s0 );
				}
			}
			else
			{
				for( l = 0; l < i_size; l += 16 )
				{
					int t = l+i_pix+top_offset;
					__m128i s0 = _mm_loadu_si128( ref_pix + t );
					__m128i s1 = _mm_loadu_si128( ref_pix + t+1 );
					__m128i t0, t1, t2, t3;

					t0 = _mm_unpacklo_epi8( s0, zero );
					t1 = _mm_unpackhi_epi8( s0, zero );
					t2 = _mm_unpacklo_epi8( s1, zero );
					t3 = _mm_unpackhi_epi8( s1, zero );

					t0 = _mm_mullo_epi16( t0, f0 );
					t1 = _mm_mullo_epi16( t1, f0 );
					t2 = _mm_mullo_epi16( t2, f1 );
					t3 = _mm_mullo_epi16( t3, f1 );

					s0 = _mm_add_epi16( _mm_add_epi16( t0, bias ), t2 );
					s1 = _mm_add_epi16( _mm_add_epi16( t1, bias ), t3 );
					t0 = _mm_srai_epi16( s0, 5 );
					t1 = _mm_srai_epi16( s1, 5 );
					s0 = _mm_packus_epi16( t0, t1 );

					_mm_store_si128( dst+l, s0 );
				}
			}
		}
		else
		{
			memcpy( dst, &ref_pix[i_pix+top_offset], i_size );
		}

		dst += i_src;
	}

	//transpose
	if( intra_pred_order < 18 )
	{ 
		if( i_size == 8 )
			transpose8_ssse3( src, i_src, src, i_src );
		else if( i_size == 16 )
			transpose16_ssse3( src, i_src, src, i_src );
		else
			transpose32_ssse3( src, i_src, src, i_src );
	}
}

void predict_4x4_ang_sse2( pixel *src, int i_src, int intra_pred_order, int i_log2_size/*dummy*/, int filter/*dummy*/ )
{
	const int i_size = 4; 
	int mode_angle = intra_pred_order >= 18 ? 
		intra_pred_order - 26 :
	10 - intra_pred_order;
	int mode_angle_abs = LENTABS( mode_angle );
	int i_angle = mode_angle < 0 ? -angTable[mode_angle_abs] : angTable[mode_angle_abs];
	int diff = LENTMIN( LENTABS( intra_pred_order-10 ), LENTABS( intra_pred_order-26 ) );
	int k;
	const int top_offset = 16/*for_align*//*left*/;
	//ALIGNED_16( pixel ref_pix[16/*for_align*//*left*/+16/*top*/] );
	__m128i ref_pix_buf[2];
	pixel *ref_pix=(pixel*)ref_pix_buf;
	pixel *dst;

	int i_pos = 0;

	//get refPix
	dst = ref_pix+top_offset;
	if( i_angle < 0 )
	{ 
		int pix_max = (i_size*(-i_angle))>>5;
		int i_inv_angle = invAngTable[mode_angle_abs];
		int i_inv_angle_sum = 128; 

		if( intra_pred_order >= 18 )
		{
			CP64( dst-1, src-i_src-1 );
			dst --;
			for( k = 1; k <= pix_max; k ++ )
			{
				i_inv_angle_sum += i_inv_angle;
				dst[-k] = SRC(-1,(i_inv_angle_sum>>8)-1);
			}
		}
		else
		{
			for( k = -1; k < i_size; k ++ )
				dst[k] = SRC(-1,k);
			dst --;
			for( k = 1; k <= pix_max; k ++ )
			{
				i_inv_angle_sum += i_inv_angle;
				dst[-k] = SRC((i_inv_angle_sum>>8)-1,-1);
			}
		}
	}
	else
	{
		if( intra_pred_order >= 18 )
		{
			memcpy( dst, src-i_src, 2*i_size );
		}
		else
		{
			int pix_max = 2*i_size;
			for( k = 0; k < pix_max; k += 4 )
			{
				dst[k  ] = SRC(-1,k  );
				dst[k+1] = SRC(-1,k+1);
				dst[k+2] = SRC(-1,k+2);
				dst[k+3] = SRC(-1,k+3);
			}
		}
	}

	//interception
	dst = src;
	{
		__m128i zero = _mm_setzero_si128();
		__m128i bias = _mm_set1_epi16( 16 );

		for( k = 0; k < i_size; k ++ )
		{
			int i_pix, i_frac;

			i_pos += i_angle;
			i_pix  = i_pos >> 5;
			i_frac = i_pos & 31;

			if( i_frac )
			{
				int t = i_pix+top_offset;
				__m128i s0 = _mm_loadl_epi64( ref_pix + t );
				__m128i s1 = _mm_loadl_epi64( ref_pix + t+1 );
				__m128i f0 = _mm_set1_epi16( 32-i_frac );
				__m128i f1 = _mm_set1_epi16( i_frac );
				__m128i t0, t1;

				t0 = _mm_unpacklo_epi8( s0, zero );
				t1 = _mm_unpacklo_epi8( s1, zero );

				s0 = _mm_mullo_epi16( t0, f0 );
				s1 = _mm_mullo_epi16( t1, f1 );

				t0 = _mm_add_epi16( _mm_add_epi16( s0, bias ), s1 );
				t1 = _mm_srai_epi16( t0, 5 );
				s0 = _mm_packus_epi16( t1, zero );

				M32(dst) = _mm_cvtsi128_si32( s0 );
			}
			else
			{
				CP32( dst, &ref_pix[i_pix+top_offset] );
			}

			dst += i_src;
		}
	}

	//transpose
	if( intra_pred_order < 18 )
	{ 
#define SWAP( a, b ) \
do \
	{ \
	pixel tmp = SRC(a,b); \
	SRC(a,b) = SRC(b,a); \
	SRC(b,a) = tmp; \
} while( 0 )
		SWAP( 0, 1 );
		SWAP( 0, 2 );
		SWAP( 0, 3 );
		SWAP( 1, 2 );
		SWAP( 1, 3 );
		SWAP( 2, 3 );
#undef SWAP
	}
}

#define F3(a,b,c) (((int)(a)+((b)<<1)+(c)+2)>>2)
void predict_planar_internal_sse2( pixel *src, int i_src, int i_log2_size, int filter )
{
	DECLARE_ALIGNED(16, int16_t, top)[32];
	DECLARE_ALIGNED(16, int16_t, lef)[32];
	int size = 1 << i_log2_size, i, j;
	int size_m1 = size - 1;
	int shift2D = i_log2_size + 1;
	pixel tr = SRC(size,-1);
	pixel bl = SRC(-1,size);

	if( filter )
	{
		const int i_size2 = 64;
		pixel tl = SRC(-1,-1);
		pixel str = SRC(i_size2-1,-1);
		pixel sbl = SRC(-1,i_size2-1);
		int b_strong = 0;

		if( filter > 1 )
		{
			int i_left = sbl+tl-2*SRC(-1,size-1);
			int i_top  = str+tl-2*SRC(size-1,-1);
			int b_left = i_left < 8 && i_left > -8;
			int b_top  = i_top  < 8 && i_top  > -8;

			b_strong = b_top && b_left;
		}

		if( !b_strong || size<32 )
		{
			for( i = 0; i < size; i ++ )
			{
				top[i] = F3( SRC(i-1,-1), SRC(i,-1), SRC(i+1,-1) );
				lef[i] = F3( SRC(-1,i-1), SRC(-1,i), SRC(-1,i+1) );
			}
			tr = F3( SRC(i-1,-1), SRC(i,-1), SRC(i+1,-1) );
			bl = F3( SRC(-1,i-1), SRC(-1,i), SRC(-1,i+1) );
		}
		else
		{
			const int shift = 6;
			for( i = 0; i < size; i ++ )
			{
				top[i] = ((i_size2-i-1)*tl + (i+1)*str + size) >> shift;
				lef[i] = ((i_size2-i-1)*tl + (i+1)*sbl + size) >> shift;
			}
			tr = ((i_size2-i-1)*tl + (i+1)*str + size) >> shift; 
			bl = ((i_size2-i-1)*tl + (i+1)*sbl + size) >> shift;
		}
	}
	else
	{
		for( i = 0; i <= size; i ++ )
		{
			top[i] = SRC(i,-1);
			lef[i] = SRC(-1,i);
		}
	}

/*	
	for( i = 0; i < size; i ++ )
	{
		int a,b;
		a = (lef[i]<<i_log2_size) + size + bl*(i+1);
		b = tr - lef[i];
		for( j = 0; j < size; j ++ )
		{
			SRC(j,i) = ((a + top[j]*(size-i-1) + b*(j+1)) >> shift2D);
		}
	}*/
	for ( i = 0; i < size; i ++ )
	{
		int a,b;
		__m128i r[5];
		static const __m128i x0 = {1,0,2,0,3,0,4,0,5,0,6,0,7,0,8,0};
		a = (lef[i]<<i_log2_size) + size + bl*(i+1);
		b = tr - lef[i];
		r[3] = _mm_set1_epi16(b);
		r[4] = _mm_mullo_epi16(r[3],x0);
		r[0] = _mm_set1_epi16(a);
		r[4] = _mm_add_epi16(r[4],r[0]);//a+b*1
		r[3] = _mm_slli_epi16(r[3],3);//b*8
		r[2] = _mm_set1_epi16(size - i - 1);//size-i-1
		for( j = 0; j < size; j += 8 )
		{
			//SRC(j,i) = (( (lef[i]<<i_log2_size) + (tr-lef[i])*(j+1) + size + (top[j]<<i_log2_size) + (bl-top[j])*(i+1)) >> shift2D );
			r[0] = _mm_load_si128((__m128i*)(top+j));
			r[0] = _mm_mullo_epi16(r[0],r[2]);
			r[1] = _mm_add_epi16(r[4],r[0]);
			r[1] = _mm_srai_epi16(r[1],shift2D);

			r[1] = _mm_packus_epi16(r[1],r[1]);
			_mm_storel_epi64((__m128i*)(src+j+i*i_src),r[1]);

			r[4] = _mm_add_epi16(r[4],r[3]);
		}
	}
}
#undef F3

#pragma warning(default:4133)

#define LINK_PRED_FUNCTIONS(a,b,type) \
	/*a[0]=b##_##4x4##_##type; */\
	a[1]=b##_##8x8##_##type; \
	a[2]=b##_##16x16##_##type; \
	a[3]=b##_##32x32##_##type;

int lent_pred_init_x86(HEVCPredContext *pc,unsigned int sse)
{
	if ( sse >= 20 )
	{
		// to do ...
		pc->predict_ang[0]=predict_4x4_ang_sse2;
		pc->predict_ang[1]=predict_ang_internal_sse2;
		pc->predict_ang[2]=predict_ang_internal_sse2;
		pc->predict_ang[3]=predict_ang_internal_sse2;
		pc->predict_planar[1] = predict_planar_internal_sse2;
		pc->predict_planar[2]=predict_planar_internal_sse2;
		pc->predict_planar[3]=predict_planar_internal_sse2;
//		LINK_PRED_FUNCTIONS(pc->predict_dc,predict,dc_sse3)
//		LINK_PRED_FUNCTIONS(pc->predict_h,predict,h_sse3)
//		LINK_PRED_FUNCTIONS(pc->predict_v,predict,v_sse3)
//		pc->predict_ang[0]=predict_4x4_ang_sse3;
//		pc->predict_ang[1]=predict_ang_internal_sse3;
//		pc->predict_ang[2]=predict_ang_internal_sse3;
//		pc->predict_ang[3]=predict_ang_internal_sse3;
	}
	if(sse>=31)
	{
		LINK_PRED_FUNCTIONS(pc->predict_dc,predict,dc_ssse3)
		LINK_PRED_FUNCTIONS(pc->predict_h,predict,h_ssse3)
		LINK_PRED_FUNCTIONS(pc->predict_v,predict,v_ssse3)
	}
	return 0;
}
#undef LINK_PRED_FUNCTIONS
#endif