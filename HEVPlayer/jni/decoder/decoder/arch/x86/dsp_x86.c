//**********************************************
//dsp_x86.c
//Unipipy @2012
//transform & mc
//**********************************************
#include "config.h"

#if ARCH_X86_32
#include "smmintrin.h"

#include "../../internal.h"

#pragma warning(disable:4133)
#define COMPACT_DCT 0


static const __m128i offset64=
	{64,0,0,0,64,0,0,0,64,0,0,0,64,0,0,0};
static const __m128i zero=
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
static const __m128i max16bit=
	{0xff,0xff,0,0,0xff,0xff,0,0,0xff,0xff,0,0,0xff,0xff,0,0};
static const __m128i max8bit=
	{0xff,0,0,0,0xff,0,0,0,0xff,0,0,0,0xff,0,0,0};


static const __m128i interleave=
	{0,1,8,9,2,3,10,11,4,5,12,13,6,7,14,15};

/////////////////////////////////////////////////////////////////////////////////////////////////
//IDCT
/////////////////////////////////////////////////////////////////////////////////////////////////

extern const dctcoef g_aiT4[4][4];
extern const dctcoef g_aiT8[8][8];
extern const dctcoef g_aiT16[16][16];
extern const dctcoef g_aiT32[32][32];
extern const int LENT_dctdst_mode_ver[36];
extern const int LENT_dctdst_mode_hor[36];


#include "DCT_sse2.c"

void add4x4_idct_sse2( pixel *p_src, int i_src, dctcoef dct[16], int i_mode )
{
	STACK_ALIGN(16,dctcoef,d,16)
	STACK_ALIGN(16,dctcoef,tmp,16)
	dctcoef *p=d;
	int i;
	__m64 s[1],dst[1],zero;
	_mm_empty();
	
	LENT_inv_dct_4_sse2_step1( tmp, dct, 7, LENT_dctdst_mode_ver[i_mode] );
	LENT_inv_dct_4_sse2_step2( d, tmp, 12,LENT_dctdst_mode_hor[i_mode] );

	zero=_mm_setzero_si64();

	for( i = 0; i < 4; i++ )
	{
#if 1//
		s[0]=_mm_set1_pi32(*((int*)(p_src)));

		dst[0]=_mm_unpacklo_pi8(s[0],zero);

		dst[0]=_mm_add_pi16(dst[0],*((__m64*)(p)));

		dst[0]=_mm_packs_pu16(dst[0],dst[0]);
		*((int32_t*)(p_src))=dst[0].m64_i32[0];
#else
		int j;
		for( j = 0; j < 4; j++ )
		{
			p_dst[j] = lent_clip_uint8( p_dst[j] + d[i*4+j] );
		}
#endif
		p+=4;
		p_src += i_src;
	}
	_mm_empty();
}

static void add8x8_idct_sse2( pixel *p_src, int i_src, dctcoef dct[64] )
{
	//dctcoef d[64],*p=d;
	//dctcoef tmp[64];
	STACK_ALIGN(16,dctcoef,d,64)
	STACK_ALIGN(16,dctcoef,tmp,64)
	dctcoef *p=d;
	int i;
	__m128i s[1],dst[1];

	LENT_inv_dct_8_sse2_step1( tmp, dct, 7 );
	LENT_inv_dct_8_sse2_step2( d, tmp, 12 );

	//_mm_empty();

	for( i = 0; i < 8; i++ )
	{
#if 1
			s[0]=_mm_loadl_epi64((__m128i*)(p_src));

			dst[0]=_mm_unpacklo_epi8(s[0],zero);

			dst[0]=_mm_add_epi16(dst[0],_mm_load_si128((__m128i*)(p)));

			_mm_storel_epi64((__m128i*)(p_src),_mm_packus_epi16(dst[0],dst[0]));//sse3
#else
		int j;
		for( j = 0; j < 8; j++ )
		{
			p_dst[j] = lent_clip_uint8( p_dst[j] + d[i*8+j] );
		}
#endif
		p+=8;
		p_src += i_src;
	}
	//_mm_empty();
}

void add16x16_idct_sse2( pixel *p_src, int i_src, dctcoef dct[256] )
{
	//dctcoef buffer_d[256+16],buffer_tmp[256+16];
	//dctcoef *d=(((int32_t)buffer_d)+15)&~0xF;
	//dctcoef *tmp=(((int32_t)buffer_tmp)+15)&~0xF,*p=d;
	STACK_ALIGN(16,dctcoef,d,256)
	STACK_ALIGN(16,dctcoef,tmp,256)
	dctcoef *p=d;
	int i;
	__m128i s[1],dst[2];

	LENT_inv_dct_16_sse2_step1( tmp, dct, 7 );
	LENT_inv_dct_16_sse2_step2( d, tmp, 12 );

	//_mm_empty();

	for( i = 0; i < 16; i++ )
	{
#if 1
			s[0]=_mm_load_si128((__m128i*)(p_src));

			dst[0]=_mm_unpacklo_epi8(s[0],zero);
			dst[1]=_mm_unpackhi_epi8(s[0],zero);

			dst[0]=_mm_add_epi16(dst[0],_mm_load_si128((__m128i*)(p)));
			dst[1]=_mm_add_epi16(dst[1],_mm_load_si128((__m128i*)(p+8)));

			_mm_store_si128((__m128i*)(p_src),_mm_packus_epi16(dst[0],dst[1]));//sse3
#else
		int j;
		for( j = 0; j < 16; j++ )
		{
			p_dst[j] = lent_clip_uint8( p_dst[j] + d[i*16+j] );
		}
#endif
		p+=16;
		p_src += i_src;
	}
	//_mm_empty();
}

void add32x32_idct_sse2( pixel *p_src, int i_src, dctcoef dct[1024] )
{
	//dctcoef buffer_d[1024+16],buffer_tmp[1024+16];
	//dctcoef *d=(((int32_t)buffer_d)+15)&~0xF;
	//dctcoef *tmp=(((int32_t)buffer_tmp)+15)&~0xF,*p=d;
	STACK_ALIGN(16,dctcoef,d,1024)
	STACK_ALIGN(16,dctcoef,tmp,1024)
	dctcoef *p=d;
	int i;
	__m128i s[2],dst[4];

	LENT_inv_dct_32_sse2_step1( tmp, dct, 7 );
	LENT_inv_dct_32_sse2_step2( d, tmp, 12 );

	//_mm_empty();

	for( i = 0; i < 32; i++ )
	{
#if 1
			s[0]=_mm_load_si128((__m128i*)(p_src));
			s[1]=_mm_load_si128((__m128i*)(p_src+16));

			dst[0]=_mm_unpacklo_epi8(s[0],zero);
			dst[1]=_mm_unpackhi_epi8(s[0],zero);
			dst[2]=_mm_unpacklo_epi8(s[1],zero);
			dst[3]=_mm_unpackhi_epi8(s[1],zero);

			dst[0]=_mm_add_epi16(dst[0],_mm_load_si128((__m128i*)(p)));
			dst[1]=_mm_add_epi16(dst[1],_mm_load_si128((__m128i*)(p+8)));
			dst[2]=_mm_add_epi16(dst[2],_mm_load_si128((__m128i*)(p+16)));
			dst[3]=_mm_add_epi16(dst[3],_mm_load_si128((__m128i*)(p+24)));

			_mm_store_si128((__m128i*)(p_src),_mm_packus_epi16(dst[0],dst[1]));
			_mm_store_si128((__m128i*)(p_src+16),_mm_packus_epi16(dst[2],dst[3]));
#else
		int j;
		for( j = 0; j < 32; j++ )
		{
			p_dst[j] = lent_clip_uint8( p_dst[j] + p[j] );
		}
#endif
		p+=32;
		p_src += i_src;
	}
	//_mm_empty();
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//MC相关
/////////////////////////////////////////////////////////////////////////////////////////////////
static int16_t luma_frac_coeff[4][8] =
{
	{  0, 0,   0, 64,  0,   0, 0,  0 },
	{ -1, 4, -10, 58, 17,  -5, 1,  0 },
	{ -1, 4, -11, 40, 40, -11, 4, -1 },
	{  0, 1,  -5, 17, 58, -10, 4, -1 }
};

static int16_t chroma_frac_coeff[8][4] = 
{
	{  0, 64,  0,  0 },
	{ -2, 58, 10, -2 },
	{ -4, 54, 16, -2 },
	{ -6, 46, 28, -4 },
	{ -4, 36, 36, -4 },
	{ -4, 28, 46, -6 },
	{ -2, 16, 54, -4 },
	{ -2, 10, 58, -2 }
};
static const __m128i frac_coeff_8bit[3]=
	{
		{ -1, 4, -10, 58, 17,  -5,  1,  0 , -1, 4, -10, 58, 17,  -5,  1,  0},
		{ -1, 4, -11, 40, 40, -11,  4, -1 , -1, 4, -11, 40, 40, -11,  4, -1},
		{  0, 1,  -5, 17, 58, -10,  4, -1 ,  0, 1,  -5, 17, 58, -10,  4, -1},
	};
/*
static const __m128i frac_coeff_ch[8]=
	{
		{  0, 0,  64, 0,  0, 0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ -3, -1, 60, 0,  8, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ -4, -1, 54, 0, 16, 0, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ -5, -1, 46, 0, 27, 0, -4, -1, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ -4, -1, 36, 0, 36, 0, -4, -1, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ -4, -1, 27, 0, 46, 0, -5, -1, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ -2, -1, 16, 0, 54, 0, -4, -1, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ -1, -1,  8, 0, 60, 0, -3, -1, 0, 0, 0, 0, 0, 0, 0, 0 }
	};
*/
static const __m128i frac_coeff_ch_8bit[8]=
	{
		{  0, 64,  0,  0 , 0, 64,  0,  0 , 0, 64,  0,  0 , 0, 64,  0,  0 },
		{ -2, 58, 10, -2, -2, 58, 10, -2, -2, 58, 10, -2, -2, 58, 10, -2 },
		{ -4, 54, 16, -2, -4, 54, 16, -2, -4, 54, 16, -2, -4, 54, 16, -2 },
		{ -6, 46, 28, -4, -6, 46, 28, -4, -6, 46, 28, -4, -6, 46, 28, -4 },
		{ -4, 36, 36, -4, -4, 36, 36, -4, -4, 36, 36, -4, -4, 36, 36, -4 },
		{ -4, 28, 46, -6, -4, 28, 46, -6, -4, 28, 46, -6, -4, 28, 46, -6 },
		{ -2, 16, 54, -4, -2, 16, 54, -4, -2, 16, 54, -4, -2, 16, 54, -4 },
		{ -2, 10, 58, -2, -2, 10, 58, -2, -2, 10, 58, -2, -2, 10, 58, -2 }
	};

static lent_always_inline void mc_copy( pixel *dst, int i_dst, pixel *src, int i_src, int i_width, int i_height )
{
	int y;

	switch( i_width )
	{
	case 4:
		for( y = 0; y < i_height; y ++, src += i_src, dst += i_dst )
			memcpy( dst, src, 4 * sizeof(pixel) );
		break;
	case 8:
		for( y = 0; y < i_height; y ++, src += i_src, dst += i_dst )
			memcpy( dst, src, 8 * sizeof(pixel) );
		break;
	case 16:
		for( y = 0; y < i_height; y ++, src += i_src, dst += i_dst )
			memcpy( dst, src, 16 * sizeof(pixel) );
		break;
	case 32:
		for( y = 0; y < i_height; y ++, src += i_src, dst += i_dst )
			memcpy( dst, src, 32 * sizeof(pixel) );
		break;
	case 64:
		for( y = 0; y < i_height; y ++, src += i_src, dst += i_dst )
			memcpy( dst, src, 64 * sizeof(pixel) );
		break;
	default:
		for( y = 0; y < i_height; y ++, src += i_src, dst += i_dst )
			memcpy( dst, src, i_width * sizeof(pixel) );
		break;
	}
}
/*
static void hpel_filter_ssse3_core( int16_t *dst, pixel *src,
						int stride, int width, int height,int column )
{
	__m128i s,d[4],tmp,tmp2;

	int x, y;



	src -= stride*3+3;
	//_mm_empty();

	s=frac_coeff[column];

	_mm_prefetch(src, _MM_HINT_T0);

	for( y=-3; y<height+4; y++ )
	{
		_mm_prefetch(src+stride, _MM_HINT_T0);
		_mm_prefetch(src+stride*2, _MM_HINT_T0);
		tmp2=_mm_loadu_si128((__m128i*)(src));

		for( x = 0; x < width; x+=4 )
		{

			tmp=tmp2;

			tmp2=_mm_loadu_si128((__m128i*)(src+x+4));

			d[0]=_mm_madd_epi16(_mm_unpacklo_epi8(tmp,zero),s);//sse2
			tmp=_mm_srli_si128(tmp,1);
			d[1]=_mm_madd_epi16(_mm_unpacklo_epi8(tmp,zero),s);
			tmp=_mm_srli_si128(tmp,1);
			d[2]=_mm_madd_epi16(_mm_unpacklo_epi8(tmp,zero),s);
			tmp=_mm_srli_si128(tmp,1);
			d[3]=_mm_madd_epi16(_mm_unpacklo_epi8(tmp,zero),s);

			d[0]=_mm_hadd_epi32(d[0],d[1]);//sse3
			d[2]=_mm_hadd_epi32(d[2],d[3]);

			d[0]=_mm_hadd_epi32(d[0],d[2]);//sse3

			d[0]=_mm_packs_epi32(d[0],d[0]);

			_mm_storel_epi64((__m128i*)(dst+x),d[0]);//sse3


		}
		dst += HPEL_SIZE;
		src += stride;
	}
	//_mm_empty();
}
*/
static void hpel_filter_ssse3_core_2( int16_t *dst, pixel *src,
						int stride, int width, int height,int column )
{
	__m128i s,d[4],tmp,tmp2,
		shuffle[4]={
			{0,1,2,3,4,5,6,7,1,2,3,4,5,6,7,8},
			{2,3,4,5,6,7,8,9,3,4,5,6,7,8,9,10},
			{4,5,6,7,8,9,10,11,5,6,7,8,9,10,11,12},
			{6,7,8,9,10,11,12,13,7,8,9,10,11,12,13,14},
	};


	int x, y;



	src -= stride*3+3;
	//_mm_empty();

	s=frac_coeff_8bit[column];

	_mm_prefetch(src, _MM_HINT_T0);

	for( y=-3; y<height+4; y++ )
	{
		_mm_prefetch(src+stride, _MM_HINT_T0);
		_mm_prefetch(src+stride*2, _MM_HINT_T0);
		tmp2=_mm_loadu_si128((__m128i*)(src));

		for( x = 0; x < width; x+=8 )
		{
			tmp=tmp2;
			tmp2=_mm_loadu_si128((__m128i*)(src+x+8));

			d[0]=_mm_shuffle_epi8(tmp,shuffle[0]);
			d[1]=_mm_shuffle_epi8(tmp,shuffle[1]);
			d[2]=_mm_shuffle_epi8(tmp,shuffle[2]);
			d[3]=_mm_shuffle_epi8(tmp,shuffle[3]);
			d[0]=_mm_maddubs_epi16(d[0],s);//sse3

			//tmp=_mm_srli_si128(tmp,2);
			d[1]=_mm_maddubs_epi16(d[1],s);//sse3

			//tmp=_mm_srli_si128(tmp,2);
			d[2]=_mm_maddubs_epi16(d[2],s);//sse3
			
			//tmp=_mm_srli_si128(tmp,2);
			d[3]=_mm_maddubs_epi16(d[3],s);//sse3

			d[0]=_mm_hadd_epi16(d[0],d[1]);//sse3
			d[2]=_mm_hadd_epi16(d[2],d[3]);//sse3

			d[0]=_mm_hadd_epi16(d[0],d[2]);//sse3

			_mm_store_si128((__m128i*)(dst+x),d[0]);//sse3
		}
		dst += HPEL_SIZE;
		src += stride;
	}
	//_mm_empty();
}
/*
static void hpel_filter_ssse3( int16_t *dst1, int16_t *dst2, int16_t *dst3, pixel *src,
						int stride, int width, int height,int select_flag )
{
	if(select_flag&2)
		hpel_filter_ssse3_core(dst1,src,stride,width,height,0);
	if(select_flag&4)
		hpel_filter_ssse3_core(dst2,src,stride,width,height,1);
	if(select_flag&8)
		hpel_filter_ssse3_core(dst3,src,stride,width,height,2);
}
*/
static void hpel_filter_ssse3_2( int16_t *dst1, int16_t *dst2, int16_t *dst3, pixel *src,
						int stride, int width, int height,int select_flag )
{
	if(select_flag&2)
		hpel_filter_ssse3_core_2(dst1,src,stride,width,height,0);
	if(select_flag&4)
		hpel_filter_ssse3_core_2(dst2,src,stride,width,height,1);
	if(select_flag&8)
		hpel_filter_ssse3_core_2(dst3,src,stride,width,height,2);
}

void hpel_filter_sse2_core( int16_t *dst, pixel *src,
	int stride, int width, int height,int column )
{
	STACK_ALIGN( 16, int16_t, buf, HPEL_SIZE*HPEL_SIZE ) //for transpose
	STACK_ALIGN( 16, int16_t, buf1,HPEL_SIZE*HPEL_SIZE ) //for interpolation
	int h7 = height + 7, w7 = width + 7;
	int sx, sy;

	src -= stride*3+3;

	//transpose to buf
	{
		__m128i zero = _mm_setzero_si128();
		for( sy = 0; sy < h7; sy += 8 )
		{
			for( sx = 0; sx < w7; sx += 8 )
			{
				pixel *ss = src + sy * stride + sx;
				int16_t *dd = buf + sx * HPEL_SIZE + sy;
				__m128i s[8], d[8];

#define LOAD( i ) \
	s[i] = _mm_loadl_epi64( (__m128i *)(ss + i * stride) ); \
	s[i] = _mm_unpacklo_epi8( s[i], zero )

				LOAD( 0 );
				LOAD( 1 );
				LOAD( 2 );
				LOAD( 3 );
				LOAD( 4 );
				LOAD( 5 );
				LOAD( 6 );
				LOAD( 7 );

#undef LOAD

				TRANSPOSE8_R2R_SSE2( d, s );

				_mm_store_si128( (__m128i *)(dd + 0 * HPEL_SIZE), d[0] );
				_mm_store_si128( (__m128i *)(dd + 1 * HPEL_SIZE), d[4] );
				_mm_store_si128( (__m128i *)(dd + 2 * HPEL_SIZE), d[2] );
				_mm_store_si128( (__m128i *)(dd + 3 * HPEL_SIZE), d[6] );
				_mm_store_si128( (__m128i *)(dd + 4 * HPEL_SIZE), d[1] );
				_mm_store_si128( (__m128i *)(dd + 5 * HPEL_SIZE), d[5] );
				_mm_store_si128( (__m128i *)(dd + 6 * HPEL_SIZE), d[3] );
				_mm_store_si128( (__m128i *)(dd + 7 * HPEL_SIZE), d[7] );
			}
		}
	}

	//interpolation
	{
		__m128i r0, r1, r2, r3;

		if( column == 0 ) //{ -1, 4, -10, 58, 17,  -5, 1,  0 }
		{
			for( sx = 0; sx < width; sx ++ )
			{
				for( sy = 0; sy < h7; sy += 8 )
				{
					int16_t *ss = buf + sx * HPEL_SIZE + sy;

					r0 = _mm_load_si128( (__m128i *)(ss + 6*HPEL_SIZE) );
					r1 = _mm_load_si128( (__m128i *)(ss + 3*HPEL_SIZE) );
					r0 = _mm_add_epi16( r0, _mm_slli_epi16( r1, 5 ) );
					r2 = _mm_load_si128( (__m128i *)(ss + 2*HPEL_SIZE) );
					r3 = _mm_slli_epi16( _mm_sub_epi16( r1, r2 ), 1 );
					r0 = _mm_add_epi16( r0, r3 );
					r2 = _mm_load_si128( (__m128i *)(ss + 1*HPEL_SIZE) );
					r3 = _mm_add_epi16( r3, r2 );
					r2 = _mm_load_si128( (__m128i *)(ss + 5*HPEL_SIZE) );
					r0 = _mm_sub_epi16( r0, r2 );
					r3 = _mm_sub_epi16( r3, r2 );
					r3 = _mm_slli_epi16( r3, 2 );
					r0 = _mm_add_epi16( r0, r3 );
					r2 = _mm_load_si128( (__m128i *)(ss + 4*HPEL_SIZE) );
					r0 = _mm_add_epi16( r0, r2 );
					r1 = _mm_slli_epi16( _mm_add_epi16( r1, r2 ), 4 );
					r0 = _mm_add_epi16( r0, r1 );
					r2 = _mm_load_si128( (__m128i *)(ss + 0*HPEL_SIZE) );
					r0 = _mm_sub_epi16( r0, r2 );

					_mm_store_si128( (__m128i *)(buf1 + sx * HPEL_SIZE + sy), r0 );
				}
			}
		}
		else if( column == 1 ) //{ -1, 4, -11, 40, 40, -11, 4, -1 }
		{
			for( sx = 0; sx < width; sx ++ )
			{
				for( sy = 0; sy < h7; sy += 8 )
				{
					int16_t *ss = buf + sx * HPEL_SIZE + sy;

					r0 = _mm_load_si128( (__m128i *)(ss + 0*HPEL_SIZE) );
					r1 = _mm_load_si128( (__m128i *)(ss + 7*HPEL_SIZE) );
					r0 = _mm_add_epi16( r0, r1 );

					r2 = _mm_load_si128( (__m128i *)(ss + 1*HPEL_SIZE) );
					r3 = _mm_load_si128( (__m128i *)(ss + 6*HPEL_SIZE) );
					r2 = _mm_slli_epi16( _mm_add_epi16( r2, r3 ), 2 );
					r0 = _mm_sub_epi16( r2, r0 );

					r2 = _mm_load_si128( (__m128i *)(ss + 2*HPEL_SIZE) );
					r3 = _mm_load_si128( (__m128i *)(ss + 5*HPEL_SIZE) );
					r1 = _mm_add_epi16( r2, r3 );
					r0 = _mm_sub_epi16( r0, r1 );

					r2 = _mm_load_si128( (__m128i *)(ss + 3*HPEL_SIZE) );
					r3 = _mm_load_si128( (__m128i *)(ss + 4*HPEL_SIZE) );
					r2 = _mm_add_epi16( r2, r3 );
					r3 = _mm_sub_epi16( _mm_slli_epi16( r2, 3 ), _mm_slli_epi16( r1, 1 ) );
					r0 = _mm_add_epi16( r0, r3 );
					r3 = _mm_slli_epi16( r3, 2 );
					r0 = _mm_add_epi16( r0, r3 );

					_mm_store_si128( (__m128i *)(buf1 + sx * HPEL_SIZE + sy), r0 );
				}
			}
		}
		else //{  0, 1,  -5, 17, 58, -10, 4, -1 }
		{
			for( sx = 0; sx < width; sx ++ )
			{
				for( sy = 0; sy < h7; sy += 8 )
				{
					int16_t *ss = buf + sx * HPEL_SIZE + sy;

					r0 = _mm_load_si128( (__m128i *)(ss + 1*HPEL_SIZE) );
					r1 = _mm_load_si128( (__m128i *)(ss + 4*HPEL_SIZE) );
					r0 = _mm_add_epi16( r0, _mm_slli_epi16( r1, 5 ) );
					r2 = _mm_load_si128( (__m128i *)(ss + 5*HPEL_SIZE) );
					r3 = _mm_slli_epi16( _mm_sub_epi16( r1, r2 ), 1 );
					r0 = _mm_add_epi16( r0, r3 );
					r2 = _mm_load_si128( (__m128i *)(ss + 6*HPEL_SIZE) );
					r3 = _mm_add_epi16( r3, r2 );
					r2 = _mm_load_si128( (__m128i *)(ss + 2*HPEL_SIZE) );
					r0 = _mm_sub_epi16( r0, r2 );
					r3 = _mm_sub_epi16( r3, r2 );
					r3 = _mm_slli_epi16( r3, 2 );
					r0 = _mm_add_epi16( r0, r3 );
					r2 = _mm_load_si128( (__m128i *)(ss + 3*HPEL_SIZE) );
					r0 = _mm_add_epi16( r0, r2 );
					r1 = _mm_slli_epi16( _mm_add_epi16( r1, r2 ), 4 );
					r0 = _mm_add_epi16( r0, r1 );
					r2 = _mm_load_si128( (__m128i *)(ss + 7*HPEL_SIZE) );
					r0 = _mm_sub_epi16( r0, r2 );

					_mm_store_si128( (__m128i *)(buf1 + sx * HPEL_SIZE + sy), r0 );
				}
			}
		}
	}

	//transpose
	{
		for( sy = 0; sy < height; sy += 8 )
		{
			for( sx = 0; sx < width; sx += 8 )
			{
				int16_t *ss = buf1 + sx * HPEL_SIZE + sy;
				int16_t *dd = dst  + sy * HPEL_SIZE + sx;
				__m128i s[8], d[8];

				s[0] = _mm_load_si128( (__m128i *)(ss + 0 * HPEL_SIZE) );
				s[1] = _mm_load_si128( (__m128i *)(ss + 1 * HPEL_SIZE) );
				s[2] = _mm_load_si128( (__m128i *)(ss + 2 * HPEL_SIZE) );
				s[3] = _mm_load_si128( (__m128i *)(ss + 3 * HPEL_SIZE) );
				s[4] = _mm_load_si128( (__m128i *)(ss + 4 * HPEL_SIZE) );
				s[5] = _mm_load_si128( (__m128i *)(ss + 5 * HPEL_SIZE) );
				s[6] = _mm_load_si128( (__m128i *)(ss + 6 * HPEL_SIZE) );
				s[7] = _mm_load_si128( (__m128i *)(ss + 7 * HPEL_SIZE) );

				TRANSPOSE8_R2R_SSE2( d, s );

				_mm_store_si128( (__m128i *)(dd + 0 * HPEL_SIZE), d[0] );
				_mm_store_si128( (__m128i *)(dd + 1 * HPEL_SIZE), d[4] );
				_mm_store_si128( (__m128i *)(dd + 2 * HPEL_SIZE), d[2] );
				_mm_store_si128( (__m128i *)(dd + 3 * HPEL_SIZE), d[6] );
				_mm_store_si128( (__m128i *)(dd + 4 * HPEL_SIZE), d[1] );
				_mm_store_si128( (__m128i *)(dd + 5 * HPEL_SIZE), d[5] );
				_mm_store_si128( (__m128i *)(dd + 6 * HPEL_SIZE), d[3] );
				_mm_store_si128( (__m128i *)(dd + 7 * HPEL_SIZE), d[7] );
			}
		}

		for( sx = 0; sx < width; sx += 8 )
		{
			int16_t *ss = buf1 + sx * HPEL_SIZE + sy;
			int16_t *dd = dst  + sy * HPEL_SIZE + sx;
			__m128i s[8], d[8];

			s[0] = _mm_load_si128( (__m128i *)(ss + 0 * HPEL_SIZE) );
			s[1] = _mm_load_si128( (__m128i *)(ss + 1 * HPEL_SIZE) );
			s[2] = _mm_load_si128( (__m128i *)(ss + 2 * HPEL_SIZE) );
			s[3] = _mm_load_si128( (__m128i *)(ss + 3 * HPEL_SIZE) );
			s[4] = _mm_load_si128( (__m128i *)(ss + 4 * HPEL_SIZE) );
			s[5] = _mm_load_si128( (__m128i *)(ss + 5 * HPEL_SIZE) );
			s[6] = _mm_load_si128( (__m128i *)(ss + 6 * HPEL_SIZE) );
			s[7] = _mm_load_si128( (__m128i *)(ss + 7 * HPEL_SIZE) );

			TRANSPOSE8_R2R_SSE2( d, s );

			_mm_store_si128( (__m128i *)(dd + 0 * HPEL_SIZE), d[0] );
			_mm_store_si128( (__m128i *)(dd + 1 * HPEL_SIZE), d[4] );
			_mm_store_si128( (__m128i *)(dd + 2 * HPEL_SIZE), d[2] );
			_mm_store_si128( (__m128i *)(dd + 3 * HPEL_SIZE), d[6] );
			_mm_store_si128( (__m128i *)(dd + 4 * HPEL_SIZE), d[1] );
			_mm_store_si128( (__m128i *)(dd + 5 * HPEL_SIZE), d[5] );
			_mm_store_si128( (__m128i *)(dd + 6 * HPEL_SIZE), d[3] );
			//_mm_store_si128( (__m128i *)(dd + 7 * HPEL_SIZE), d[7] );
		}
	}
}

static void hpel_filter_sse2( int16_t *dst1, int16_t *dst2, int16_t *dst3, pixel *src,
						int stride, int width, int height,int select_flag )
{
	if(select_flag&2)
		hpel_filter_sse2_core(dst1,src,stride,width,height,0);
	if(select_flag&4)
		hpel_filter_sse2_core(dst2,src,stride,width,height,1);
	if(select_flag&8)
		hpel_filter_sse2_core(dst3,src,stride,width,height,2);
}


#define TAPFILTER(src,f) \
	(src[x-3*i_src]*f[0]+src[x-2*i_src]*f[1]+src[x-1*i_src]*f[2]+src[x]*f[3]+ \
	src[x+1*i_src]*f[4]+src[x+2*i_src]*f[5]+src[x+3*i_src]*f[6]+src[x+4*i_src]*f[7])
int16_t* get_ref_sse4_shushuang_03( int16_t *dst, int *i_dst, pixel *src, int16_t hpx[3][HPEL_SIZE*HPEL_SIZE], int i_src,
				  int mvx, int mvy, int i_width, int i_height )
{
	int fx = mvx&3;
	int fy = mvy&3;
	int i_offs = (mvy>>2)*i_src + (mvx>>2);
	int x, y;
	//const int16_t *f = luma_frac_coeff[fy];
	int16_t *rt = NULL;

	
	__m128i d,tmp,temp,tmp1;
	//_mm_empty();

	if( fx == 0 )
	{
		rt = dst;
		src += i_offs;

		if( fy == 0 )
		{
			for( y = 0; y < i_height; y++ )
			{
#if 1
				_mm_prefetch(src+i_src,_MM_HINT_NTA);
				for( x = 0; x < i_width; x+=8 )
				{
					//dst[x] = (src[x]<<6);

					d=_mm_loadl_epi64((__m128i*)(src+x));
					d=_mm_unpacklo_epi8(d,zero);
					d=_mm_slli_epi16(d,6);

					_mm_store_si128((__m128i*)(dst+x),d);

				}
#else
				for( x = 0; x < i_width; x++ )
				{
					dst[x] = (src[x]<<6);
				}
#endif
				dst  += *i_dst;
				src  += i_src;
			}
		}
		else if( fy == 1 )
		{
			for( y = 0; y < i_height; y++ )
			{
#if 1
				_mm_prefetch(src,_MM_HINT_T0);
				_mm_prefetch(src+i_src,_MM_HINT_T0);
				_mm_prefetch(src+i_src*2,_MM_HINT_T0);
				for( x = 0; x < i_width; x+=8 )
				{
					//dst[x] = TAPFILTER( src, f );{ -1, 4, -10, 58, 17,  -5, 1,  0 }

					d=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+3*i_src)),zero);//*1
					tmp=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x)),zero);
					d=_mm_add_epi16(d,_mm_slli_epi16(tmp,5));
					tmp1=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x-1*i_src)),zero);
					temp=_mm_slli_epi16(_mm_sub_epi16(tmp,tmp1),1);
					d=_mm_add_epi16(d,temp);
					tmp1=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x-2*i_src)),zero);
					temp=_mm_add_epi16(temp,tmp1);
					tmp1=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+2*i_src)),zero);
					d=_mm_sub_epi16(d,tmp1);
					temp=_mm_sub_epi16(temp,tmp1);
					temp=_mm_slli_epi16(temp,2);
					d=_mm_add_epi16(d,temp);
					tmp1=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+i_src)),zero);
					d=_mm_add_epi16(d,tmp1);
					tmp=_mm_slli_epi16(_mm_add_epi16(tmp,tmp1),4);
					d=_mm_add_epi16(d,tmp);
					tmp1=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x-3*i_src)),zero);
					d=_mm_sub_epi16(d,tmp1);

					_mm_store_si128((__m128i*)(dst+x),d);

				}
#else
				for( x = 0; x < i_width; x++ )
				{
					dst[x] = TAPFILTER( src, f );
				}
#endif
				dst  += *i_dst;
				src  += i_src;
			}
		}
		else if( fy == 2 )
		{
			for( y = 0; y < i_height; y++ )
			{
#if 1
				_mm_prefetch(src,_MM_HINT_T0);
				_mm_prefetch(src+i_src,_MM_HINT_T0);
				_mm_prefetch(src+i_src*2,_MM_HINT_T0);
				for( x = 0; x < i_width; x+=8 )
				{
					//dst[x] = TAPFILTER( src, f );{ -1, 4, -11, 40, 40, -11, 4, -1 }

					d=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x-3*i_src)),zero);
					tmp=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+4*i_src)),zero);
					d=_mm_add_epi16(tmp,d);

					temp=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x-2*i_src)),zero);
					tmp=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+3*i_src)),zero);
					tmp=_mm_slli_epi16(_mm_add_epi16(tmp,temp),2);
					d=_mm_sub_epi16(tmp,d);

					temp=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x-1*i_src)),zero);
					tmp=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+2*i_src)),zero);
					tmp=_mm_add_epi16(tmp,temp);
					d=_mm_sub_epi16(d,tmp);

					temp=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x)),zero);
					tmp1=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+i_src)),zero);
					tmp1=_mm_add_epi16(tmp1,temp);
					temp=_mm_sub_epi16(_mm_slli_epi16(tmp1,3),_mm_slli_epi16(tmp,1));
					d=_mm_add_epi16(temp,d);
					temp=_mm_slli_epi16(temp,2);
					d=_mm_add_epi16(temp,d);

					_mm_store_si128((__m128i*)(dst+x),d);

				}
#else
				for( x = 0; x < i_width; x++ )
				{
					dst[x] = TAPFILTER( src, f );
				}
#endif
				dst  += *i_dst;
				src  += i_src;
			}
		}
		else //if( fy == 3 )
		{
			for( y = 0; y < i_height; y++ )
			{
#if 1
				_mm_prefetch(src,_MM_HINT_T0);
				_mm_prefetch(src+i_src,_MM_HINT_T0);
				_mm_prefetch(src+i_src*2,_MM_HINT_T0);
				for( x = 0; x < i_width; x+=8 )
				{
					//dst[x] = TAPFILTER( src, f );{  0, 1,  -5, 17, 58, -10, 4, -1 }

					d=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x-2*i_src)),zero);//*1
					tmp=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+i_src)),zero);
					d=_mm_add_epi16(d,_mm_slli_epi16(tmp,5));
					tmp1=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+2*i_src)),zero);
					temp=_mm_slli_epi16(_mm_sub_epi16(tmp,tmp1),1);
					d=_mm_add_epi16(d,temp);
					tmp1=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+3*i_src)),zero);
					temp=_mm_add_epi16(temp,tmp1);
					tmp1=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x-i_src)),zero);
					d=_mm_sub_epi16(d,tmp1);
					temp=_mm_sub_epi16(temp,tmp1);
					temp=_mm_slli_epi16(temp,2);
					d=_mm_add_epi16(d,temp);
					tmp1=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x)),zero);
					d=_mm_add_epi16(d,tmp1);
					tmp=_mm_slli_epi16(_mm_add_epi16(tmp,tmp1),4);
					d=_mm_add_epi16(d,tmp);
					tmp1=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+4*i_src)),zero);
					d=_mm_sub_epi16(d,tmp1);

					_mm_store_si128((__m128i*)(dst+x),d);

				}
#else
				for( x = 0; x < i_width; x++ )
				{
					dst[x] = TAPFILTER( src, f );
				}
#endif
				dst  += *i_dst;
				src  += i_src;
			}
		}
	}
	else
	{
		int16_t *hsrc = hpx[fx-1] +3*HPEL_SIZE/*+ i_offs*/;
		i_src = HPEL_SIZE;

		if( fy == 0 )
		{
			*i_dst = i_src;
			rt = hsrc;
		}
		
		else if( fy == 1 )
		{
				rt=dst;
				_mm_prefetch(hpx[fx-1],_MM_HINT_NTA);
			for( y = 0; y < i_height; y++ )
			{
#if 1
				for( x = 0; x < i_width; x+=4 )
				{
					//dst[x] = TAPFILTER( src, f )>>6;{ -1, 4, -10, 58, 17,  -5, 1,  0 }
					d=_mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(hsrc+x+3*i_src)));//*1
					tmp=_mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(hsrc+x)));
					d=_mm_add_epi32(d,_mm_slli_epi32(tmp,5));
					tmp1=_mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(hsrc+x-1*i_src)));
					temp=_mm_slli_epi32(_mm_sub_epi32(tmp,tmp1),1);
					d=_mm_add_epi32(d,temp);
					tmp1=_mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(hsrc+x-2*i_src)));
					temp=_mm_add_epi32(temp,tmp1);
					tmp1=_mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(hsrc+x+2*i_src)));
					d=_mm_sub_epi32(d,tmp1);
					temp=_mm_sub_epi32(temp,tmp1);
					temp=_mm_slli_epi32(temp,2);
					d=_mm_add_epi32(d,temp);
					tmp1=_mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(hsrc+x+i_src)));
					d=_mm_add_epi32(d,tmp1);
					tmp=_mm_slli_epi32(_mm_add_epi32(tmp,tmp1),4);
					d=_mm_add_epi32(d,tmp);
					tmp1=_mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(hsrc+x-3*i_src)));
					d=_mm_sub_epi32(d,tmp1);

					d=_mm_srai_epi32(d,6);
					d=_mm_packs_epi32(d,d);

					_mm_storel_epi64((__m128i*)(dst+x),d);

				}
#else
				for( x = 0; x < i_width; x++ )
				{
					dst[x] = (TAPFILTER( hsrc, f )>>6);
				}
#endif
				dst  += *i_dst;
				hsrc += i_src;
			}
		}
		else if( fy == 2 )
		{
				rt=dst;
				_mm_prefetch(hpx[fx-1],_MM_HINT_NTA);
			for( y = 0; y < i_height; y++ )
			{
#if 1
				for( x = 0; x < i_width; x+=4 )
				{
					//dst[x] = TAPFILTER( hsrc, f )>>6;{ -1, 4, -11, 40, 40, -11, 4, -1 }

					d=_mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(hsrc+x-3*i_src)));
					tmp=_mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(hsrc+x+4*i_src)));
					d=_mm_add_epi32(tmp,d);

					temp=_mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(hsrc+x-2*i_src)));
					tmp=_mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(hsrc+x+3*i_src)));
					tmp=_mm_slli_epi32(_mm_add_epi32(tmp,temp),2);
					d=_mm_sub_epi32(tmp,d);

					temp=_mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(hsrc+x-1*i_src)));
					tmp=_mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(hsrc+x+2*i_src)));
					tmp=_mm_add_epi32(tmp,temp);
					d=_mm_sub_epi32(d,tmp);

					temp=_mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(hsrc+x)));
					tmp1=_mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(hsrc+x+i_src)));
					tmp1=_mm_add_epi32(tmp1,temp);
					temp=_mm_sub_epi32(_mm_slli_epi32(tmp1,3),_mm_slli_epi32(tmp,1));
					d=_mm_add_epi32(temp,d);
					temp=_mm_slli_epi32(temp,2);
					d=_mm_add_epi32(temp,d);

					d=_mm_srai_epi32(d,6);
					d=_mm_packs_epi32(d,d);

					_mm_storel_epi64((__m128i*)(dst+x),d);

				}
#else
				for( x = 0; x < i_width; x++ )
				{
					dst[x] = (TAPFILTER( hsrc, f )>>6);
				}
#endif
				dst  += *i_dst;
				hsrc += i_src;
			}
		}
		else //if( fy == 3 )
		{
				rt=dst;
				_mm_prefetch(hpx[fx-1],_MM_HINT_NTA);
			for( y = 0; y < i_height; y++ )
			{
#if 1
				for( x = 0; x < i_width; x+=4 )
				{
					//dst[x] = TAPFILTER( src, f )>>6;{  0, 1,  -5, 17, 58, -10, 4, -1 }

					d=_mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(hsrc+x-2*i_src)));//*1
					tmp=_mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(hsrc+x+i_src)));
					d=_mm_add_epi32(d,_mm_slli_epi32(tmp,5));
					tmp1=_mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(hsrc+x+2*i_src)));
					temp=_mm_slli_epi32(_mm_sub_epi32(tmp,tmp1),1);
					d=_mm_add_epi32(d,temp);
					tmp1=_mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(hsrc+x+3*i_src)));
					temp=_mm_add_epi32(temp,tmp1);
					tmp1=_mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(hsrc+x-i_src)));
					d=_mm_sub_epi32(d,tmp1);
					temp=_mm_sub_epi32(temp,tmp1);
					temp=_mm_slli_epi32(temp,2);
					d=_mm_add_epi32(d,temp);
					tmp1=_mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(hsrc+x)));
					d=_mm_add_epi32(d,tmp1);
					tmp=_mm_slli_epi32(_mm_add_epi32(tmp,tmp1),4);
					d=_mm_add_epi32(d,tmp);
					tmp1=_mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(hsrc+x+4*i_src)));
					d=_mm_sub_epi32(d,tmp1);

					d=_mm_srai_epi32(d,6);
					d=_mm_packs_epi32(d,d);

					_mm_storel_epi64((__m128i*)(dst+x),d);

				}
#else
				for( x = 0; x < i_width; x++ )
				{
					dst[x] = (TAPFILTER( hsrc, f )>>6);
				}
#endif
				dst  += *i_dst;
				hsrc += i_src;
			}
		}
	}
	//_mm_empty();
	return rt;
}
/*
int16_t* get_ref_ssse3( int16_t *dst, int *i_dst, pixel *src, int16_t hpx[3][HPEL_SIZE*HPEL_SIZE], int i_src,
				  int mvx, int mvy, int i_width, int i_height )
{
	int fx = mvx&3;
	int fy = mvy&3;
	int i_offs = (mvy>>2)*i_src + (mvx>>2);
	int x, y;
	const int16_t *f = luma_frac_coeff[fy];
	int16_t *rt = NULL;

	
	__m128i s[8],d,tmp;
	//_mm_empty();

	if( fx == 0 )
	{
		rt = dst;
		src += i_offs;

		if( fy == 0 )
		{
			for( y = 0; y < i_height; y++ )
			{
#if 1
				for( x = 0; x < i_width; x+=8 )
				{
					//dst[x] = (src[x]<<6);

					d=_mm_loadl_epi64((__m128i*)(src+x));
					d=_mm_unpacklo_epi8(d,zero);
					d=_mm_slli_epi16(d,6);

					_mm_store_si128((__m128i*)(dst+x),d);

				}
#else
				for( x = 0; x < i_width; x++ )
				{
					dst[x] = (src[x]<<6);
				}
#endif
				dst  += *i_dst;
				src  += i_src;
			}
		}
		else
		{
			
			s[0]=_mm_set1_epi16(f[0]);
			s[1]=_mm_set1_epi16(f[1]);
			s[2]=_mm_set1_epi16(f[2]);
			s[3]=_mm_set1_epi16(f[3]);
			s[4]=_mm_set1_epi16(f[4]);
			s[5]=_mm_set1_epi16(f[5]);
			s[6]=_mm_set1_epi16(f[6]);
			s[7]=_mm_set1_epi16(f[7]);
			for( y = 0; y < i_height; y++ )
			{
#if 1
				for( x = 0; x < i_width; x+=8 )
				{

//					dst[x] = TAPFILTER( src, f );

					d=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x-3*i_src)),zero);
					d=_mm_mullo_epi16(d,s[0]);

					tmp=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x-2*i_src)),zero);
					tmp=_mm_mullo_epi16(tmp,s[1]);
					d=_mm_add_epi16(d,tmp);

					tmp=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x-1*i_src)),zero);
					tmp=_mm_mullo_epi16(tmp,s[2]);
					d=_mm_add_epi16(d,tmp);

					tmp=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x-0*i_src)),zero);
					tmp=_mm_mullo_epi16(tmp,s[3]);
					d=_mm_add_epi16(d,tmp);

					tmp=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+1*i_src)),zero);
					tmp=_mm_mullo_epi16(tmp,s[4]);
					d=_mm_add_epi16(d,tmp);

					tmp=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+2*i_src)),zero);
					tmp=_mm_mullo_epi16(tmp,s[5]);
					d=_mm_add_epi16(d,tmp);

					tmp=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+3*i_src)),zero);
					tmp=_mm_mullo_epi16(tmp,s[6]);
					d=_mm_add_epi16(d,tmp);

					tmp=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+4*i_src)),zero);
					tmp=_mm_mullo_epi16(tmp,s[7]);
					d=_mm_add_epi16(d,tmp);

					_mm_store_si128((__m128i*)(dst+x),d);

				}
#else
				for( x = 0; x < i_width; x++ )
				{
					dst[x] = TAPFILTER( src, f );
				}
#endif
				dst  += *i_dst;
				src  += i_src;
			}
		}
	}
	else
	{
		int16_t *hsrc = hpx[fx-1] +3*HPEL_SIZE/ *+ i_offs* /;
		i_src = HPEL_SIZE;

		if( fy == 0 )
		{
			*i_dst = i_src;
			rt = hsrc;
		}
		else
		{
			rt = dst;
			s[0]=_mm_set1_epi16(f[0]);
			s[1]=_mm_set1_epi16(f[1]);
			s[2]=_mm_set1_epi16(f[2]);
			s[3]=_mm_set1_epi16(f[3]);
			s[4]=_mm_set1_epi16(f[4]);
			s[5]=_mm_set1_epi16(f[5]);
			s[6]=_mm_set1_epi16(f[6]);
			s[7]=_mm_set1_epi16(f[7]);

			for( y = 0; y < i_height; y++ )
			{
#if 1
				for( x = 0; x < i_width; x+=4 )
				{

//					dst[x] = (TAPFILTER( hsrc, f )>>6);

					d=_mm_loadl_epi64((__m128i*)(hsrc+x-3*i_src));
					d=_mm_unpacklo_epi16(_mm_mullo_epi16(d,s[0]),_mm_mulhi_epi16(d,s[0]));

					tmp=_mm_loadl_epi64((__m128i*)(hsrc+x-2*i_src));
					tmp=_mm_unpacklo_epi16(_mm_mullo_epi16(tmp,s[1]),_mm_mulhi_epi16(tmp,s[1]));
					d=_mm_add_epi32(d,tmp);

					tmp=_mm_loadl_epi64((__m128i*)(hsrc+x-1*i_src));
					tmp=_mm_unpacklo_epi16(_mm_mullo_epi16(tmp,s[2]),_mm_mulhi_epi16(tmp,s[2]));
					d=_mm_add_epi32(d,tmp);

					tmp=_mm_loadl_epi64((__m128i*)(hsrc+x-0*i_src));
					tmp=_mm_unpacklo_epi16(_mm_mullo_epi16(tmp,s[3]),_mm_mulhi_epi16(tmp,s[3]));
					d=_mm_add_epi32(d,tmp);

					tmp=_mm_loadl_epi64((__m128i*)(hsrc+x+1*i_src));
					tmp=_mm_unpacklo_epi16(_mm_mullo_epi16(tmp,s[4]),_mm_mulhi_epi16(tmp,s[4]));
					d=_mm_add_epi32(d,tmp);

					tmp=_mm_loadl_epi64((__m128i*)(hsrc+x+2*i_src));
					tmp=_mm_unpacklo_epi16(_mm_mullo_epi16(tmp,s[5]),_mm_mulhi_epi16(tmp,s[5]));
					d=_mm_add_epi32(d,tmp);

					tmp=_mm_loadl_epi64((__m128i*)(hsrc+x+3*i_src));
					tmp=_mm_unpacklo_epi16(_mm_mullo_epi16(tmp,s[6]),_mm_mulhi_epi16(tmp,s[6]));
					d=_mm_add_epi32(d,tmp);

					tmp=_mm_loadl_epi64((__m128i*)(hsrc+x+4*i_src));
					tmp=_mm_unpacklo_epi16(_mm_mullo_epi16(tmp,s[7]),_mm_mulhi_epi16(tmp,s[7]));
					d=_mm_add_epi32(d,tmp);

					d=_mm_srai_epi32(d,6);
					d=_mm_packs_epi32(d,d);

					_mm_storel_epi64((__m128i*)(dst+x),d);
					//assert(dst[x]==(TAPFILTER( hsrc, f )>>6));
					//assert(dst[x+1]==(TAPFILTER( (hsrc+1), f )>>6));
					//assert(dst[x+2]==(TAPFILTER( (hsrc+2), f )>>6));
					//assert(dst[x+3]==(TAPFILTER( (hsrc+3), f )>>6));

				}
#else
				for( x = 0; x < i_width; x++ )
					dst[x] = (TAPFILTER( hsrc, f )>>6);
#endif
				dst  += *i_dst;
				hsrc += i_src;
			}
		}
	}
	//_mm_empty();
	return rt;
}
*/
#undef TAPFILTER
int16_t* get_ref_sse2_shushuang( int16_t *dst, int *i_dst, pixel *src, int16_t hpx[3][HPEL_SIZE*HPEL_SIZE], int i_src,
				  int mvx, int mvy, int i_width, int i_height )
{
	int fx = mvx&3;
	int fy = mvy&3;
	int i_offs = (mvy>>2)*i_src + (mvx>>2);
	int x, y;
	//const int16_t *f = luma_frac_coeff[fy];
	int16_t *rt = NULL;

	
	__m128i d,tmp,temp,tmp1;
	//_mm_empty();

	if( fx == 0 )
	{
		rt = dst;
		src += i_offs;

		if( fy == 0 )
		{
			for( y = 0; y < i_height; y++ )
			{
#if 1
				_mm_prefetch(src+i_src,_MM_HINT_NTA);
				for( x = 0; x < i_width; x+=8 )
				{
					//dst[x] = (src[x]<<6);

					d=_mm_loadl_epi64((__m128i*)(src+x));
					d=_mm_unpacklo_epi8(d,zero);
					d=_mm_slli_epi16(d,6);

					_mm_store_si128((__m128i*)(dst+x),d);

				}
#else
				for( x = 0; x < i_width; x++ )
				{
					dst[x] = (src[x]<<6);
				}
#endif
				dst  += *i_dst;
				src  += i_src;
			}
		}
		else if( fy == 1 )
		{
			for( y = 0; y < i_height; y++ )
			{
#if 1
				_mm_prefetch(src,_MM_HINT_T0);
				_mm_prefetch(src+i_src,_MM_HINT_T0);
				_mm_prefetch(src+i_src*2,_MM_HINT_T0);
				for( x = 0; x < i_width; x+=8 )
				{
					//dst[x] = TAPFILTER( src, f );{ -1, 4, -10, 58, 17,  -5, 1,  0 }

					d=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+3*i_src)),zero);//*1
					tmp=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x)),zero);
					d=_mm_add_epi16(d,_mm_slli_epi16(tmp,5));
					tmp1=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x-1*i_src)),zero);
					temp=_mm_slli_epi16(_mm_sub_epi16(tmp,tmp1),1);
					d=_mm_add_epi16(d,temp);
					tmp1=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x-2*i_src)),zero);
					temp=_mm_add_epi16(temp,tmp1);
					tmp1=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+2*i_src)),zero);
					d=_mm_sub_epi16(d,tmp1);
					temp=_mm_sub_epi16(temp,tmp1);
					temp=_mm_slli_epi16(temp,2);
					d=_mm_add_epi16(d,temp);
					tmp1=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+i_src)),zero);
					d=_mm_add_epi16(d,tmp1);
					tmp=_mm_slli_epi16(_mm_add_epi16(tmp,tmp1),4);
					d=_mm_add_epi16(d,tmp);
					tmp1=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x-3*i_src)),zero);
					d=_mm_sub_epi16(d,tmp1);

					_mm_store_si128((__m128i*)(dst+x),d);

				}
#else
				for( x = 0; x < i_width; x++ )
				{
					dst[x] = TAPFILTER( src, f );
				}
#endif
				dst  += *i_dst;
				src  += i_src;
			}
		}
		else if( fy == 2 )
		{
			for( y = 0; y < i_height; y++ )
			{
#if 1
				_mm_prefetch(src,_MM_HINT_T0);
				_mm_prefetch(src+i_src,_MM_HINT_T0);
				_mm_prefetch(src+i_src*2,_MM_HINT_T0);
				for( x = 0; x < i_width; x+=8 )
				{
					//dst[x] = TAPFILTER( src, f );{ -1, 4, -11, 40, 40, -11, 4, -1 }

					d=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x-3*i_src)),zero);
					tmp=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+4*i_src)),zero);
					d=_mm_add_epi16(tmp,d);

					temp=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x-2*i_src)),zero);
					tmp=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+3*i_src)),zero);
					tmp=_mm_slli_epi16(_mm_add_epi16(tmp,temp),2);
					d=_mm_sub_epi16(tmp,d);

					temp=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x-1*i_src)),zero);
					tmp=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+2*i_src)),zero);
					tmp=_mm_add_epi16(tmp,temp);
					d=_mm_sub_epi16(d,tmp);

					temp=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x)),zero);
					tmp1=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+i_src)),zero);
					tmp1=_mm_add_epi16(tmp1,temp);
					temp=_mm_sub_epi16(_mm_slli_epi16(tmp1,3),_mm_slli_epi16(tmp,1));
					d=_mm_add_epi16(temp,d);
					temp=_mm_slli_epi16(temp,2);
					d=_mm_add_epi16(temp,d);

					_mm_store_si128((__m128i*)(dst+x),d);

				}
#else
				for( x = 0; x < i_width; x++ )
				{
					dst[x] = TAPFILTER( src, f );
				}
#endif
				dst  += *i_dst;
				src  += i_src;
			}
		}
		else //if( fy == 3 )
		{
			for( y = 0; y < i_height; y++ )
			{
#if 1
				_mm_prefetch(src,_MM_HINT_T0);
				_mm_prefetch(src+i_src,_MM_HINT_T0);
				_mm_prefetch(src+i_src*2,_MM_HINT_T0);
				for( x = 0; x < i_width; x+=8 )
				{
					//dst[x] = TAPFILTER( src, f );{  0, 1,  -5, 17, 58, -10, 4, -1 }

					d=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x-2*i_src)),zero);//*1
					tmp=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+i_src)),zero);
					d=_mm_add_epi16(d,_mm_slli_epi16(tmp,5));
					tmp1=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+2*i_src)),zero);
					temp=_mm_slli_epi16(_mm_sub_epi16(tmp,tmp1),1);
					d=_mm_add_epi16(d,temp);
					tmp1=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+3*i_src)),zero);
					temp=_mm_add_epi16(temp,tmp1);
					tmp1=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x-i_src)),zero);
					d=_mm_sub_epi16(d,tmp1);
					temp=_mm_sub_epi16(temp,tmp1);
					temp=_mm_slli_epi16(temp,2);
					d=_mm_add_epi16(d,temp);
					tmp1=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x)),zero);
					d=_mm_add_epi16(d,tmp1);
					tmp=_mm_slli_epi16(_mm_add_epi16(tmp,tmp1),4);
					d=_mm_add_epi16(d,tmp);
					tmp1=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+4*i_src)),zero);
					d=_mm_sub_epi16(d,tmp1);

					_mm_store_si128((__m128i*)(dst+x),d);

				}
#else
				for( x = 0; x < i_width; x++ )
				{
					dst[x] = TAPFILTER( src, f );
				}
#endif
				dst  += *i_dst;
				src  += i_src;
			}
		}
	}
	else
	{
		int16_t *hsrc = hpx[fx-1] +3*HPEL_SIZE/*+ i_offs*/;
		i_src = HPEL_SIZE;

		if( fy == 0 )
		{
			*i_dst = i_src;
			rt = hsrc;
		}
		
		else if( fy == 1 )
		{
				rt=dst;
				_mm_prefetch(hpx[fx-1],_MM_HINT_NTA);
			for( y = 0; y < i_height; y++ )
			{
#if 1
				for( x = 0; x < i_width; x+=4 )
				{
					//dst[x] = TAPFILTER( src, f )>>6;{ -1, 4, -10, 58, 17,  -5, 1,  0 }

					d=_mm_loadl_epi64((__m128i*)(hsrc+x+3*i_src));//*1
					d=_mm_unpacklo_epi16(d,_mm_cmplt_epi16(d,zero));
					tmp=_mm_loadl_epi64((__m128i*)(hsrc+x));
					tmp=_mm_unpacklo_epi16(tmp,_mm_cmplt_epi16(tmp,zero));
					d=_mm_add_epi32(d,_mm_slli_epi32(tmp,5));
					tmp1=_mm_loadl_epi64((__m128i*)(hsrc+x-1*i_src));
					tmp1=_mm_unpacklo_epi16(tmp1,_mm_cmplt_epi16(tmp1,zero));
					temp=_mm_slli_epi32(_mm_sub_epi32(tmp,tmp1),1);
					d=_mm_add_epi32(d,temp);
					tmp1=_mm_loadl_epi64((__m128i*)(hsrc+x-2*i_src));
					tmp1=_mm_unpacklo_epi16(tmp1,_mm_cmplt_epi16(tmp1,zero));
					temp=_mm_add_epi32(temp,tmp1);
					tmp1=_mm_loadl_epi64((__m128i*)(hsrc+x+2*i_src));
					tmp1=_mm_unpacklo_epi16(tmp1,_mm_cmplt_epi16(tmp1,zero));
					d=_mm_sub_epi32(d,tmp1);
					temp=_mm_sub_epi32(temp,tmp1);
					temp=_mm_slli_epi32(temp,2);
					d=_mm_add_epi32(d,temp);
					tmp1=_mm_loadl_epi64((__m128i*)(hsrc+x+i_src));
					tmp1=_mm_unpacklo_epi16(tmp1,_mm_cmplt_epi16(tmp1,zero));
					d=_mm_add_epi32(d,tmp1);
					tmp=_mm_slli_epi32(_mm_add_epi32(tmp,tmp1),4);
					d=_mm_add_epi32(d,tmp);
					tmp1=_mm_loadl_epi64((__m128i*)(hsrc+x-3*i_src));
					tmp1=_mm_unpacklo_epi16(tmp1,_mm_cmplt_epi16(tmp1,zero));
					d=_mm_sub_epi32(d,tmp1);

					d=_mm_srai_epi32(d,6);
					d=_mm_packs_epi32(d,d);

					_mm_storel_epi64((__m128i*)(dst+x),d);

				}
#else
				for( x = 0; x < i_width; x++ )
				{
					dst[x] = (TAPFILTER( hsrc, f )>>6);
				}
#endif
				dst  += *i_dst;
				hsrc += i_src;
			}
		}
		else if( fy == 2 )
		{
				rt=dst;
				_mm_prefetch(hpx[fx-1],_MM_HINT_NTA);
			for( y = 0; y < i_height; y++ )
			{
#if 1
				for( x = 0; x < i_width; x+=4 )
				{
					//dst[x] = TAPFILTER( hsrc, f )>>6;{ -1, 4, -11, 40, 40, -11, 4, -1 }

					d=_mm_loadl_epi64((__m128i*)(hsrc+x-3*i_src));
					d=_mm_unpacklo_epi16(d,_mm_cmplt_epi16(d,zero));
					tmp=_mm_loadl_epi64((__m128i*)(hsrc+x+4*i_src));
					tmp=_mm_unpacklo_epi16(tmp,_mm_cmplt_epi16(tmp,zero));
					d=_mm_add_epi32(tmp,d);

					temp=_mm_loadl_epi64((__m128i*)(hsrc+x-2*i_src));
					temp=_mm_unpacklo_epi16(temp,_mm_cmplt_epi16(temp,zero));
					tmp=_mm_loadl_epi64((__m128i*)(hsrc+x+3*i_src));
					tmp=_mm_unpacklo_epi16(tmp,_mm_cmplt_epi16(tmp,zero));
					tmp=_mm_slli_epi32(_mm_add_epi32(tmp,temp),2);
					d=_mm_sub_epi32(tmp,d);

					temp=_mm_loadl_epi64((__m128i*)(hsrc+x-1*i_src));
					temp=_mm_unpacklo_epi16(temp,_mm_cmplt_epi16(temp,zero));
					tmp=_mm_loadl_epi64((__m128i*)(hsrc+x+2*i_src));
					tmp=_mm_unpacklo_epi16(tmp,_mm_cmplt_epi16(tmp,zero));
					tmp=_mm_add_epi32(tmp,temp);
					d=_mm_sub_epi32(d,tmp);

					temp=_mm_loadl_epi64((__m128i*)(hsrc+x));
					temp=_mm_unpacklo_epi16(temp,_mm_cmplt_epi16(temp,zero));
					tmp1=_mm_loadl_epi64((__m128i*)(hsrc+x+i_src));
					tmp1=_mm_unpacklo_epi16(tmp1,_mm_cmplt_epi16(tmp1,zero));
					tmp1=_mm_add_epi32(tmp1,temp);
					temp=_mm_sub_epi32(_mm_slli_epi32(tmp1,3),_mm_slli_epi32(tmp,1));
					d=_mm_add_epi32(temp,d);
					temp=_mm_slli_epi32(temp,2);
					d=_mm_add_epi32(temp,d);

					d=_mm_srai_epi32(d,6);
					d=_mm_packs_epi32(d,d);

					_mm_storel_epi64((__m128i*)(dst+x),d);

				}
#else
				for( x = 0; x < i_width; x++ )
				{
					dst[x] = (TAPFILTER( hsrc, f )>>6);
				}
#endif
				dst  += *i_dst;
				hsrc += i_src;
			}
		}
		else //if( fy == 3 )
		{
				rt=dst;
				_mm_prefetch(hpx[fx-1],_MM_HINT_NTA);
			for( y = 0; y < i_height; y++ )
			{
#if 1
				for( x = 0; x < i_width; x+=4 )
				{
					//dst[x] = TAPFILTER( src, f )>>6;{  0, 1,  -5, 17, 58, -10, 4, -1 }

					d=_mm_loadl_epi64((__m128i*)(hsrc+x-2*i_src));//*1
					d=_mm_unpacklo_epi16(d,_mm_cmplt_epi16(d,zero));
					tmp=_mm_loadl_epi64((__m128i*)(hsrc+x+i_src));
					tmp=_mm_unpacklo_epi16(tmp,_mm_cmplt_epi16(tmp,zero));
					d=_mm_add_epi32(d,_mm_slli_epi32(tmp,5));
					tmp1=_mm_loadl_epi64((__m128i*)(hsrc+x+2*i_src));
					tmp1=_mm_unpacklo_epi16(tmp1,_mm_cmplt_epi16(tmp1,zero));
					temp=_mm_slli_epi32(_mm_sub_epi32(tmp,tmp1),1);
					d=_mm_add_epi32(d,temp);
					tmp1=_mm_loadl_epi64((__m128i*)(hsrc+x+3*i_src));
					tmp1=_mm_unpacklo_epi16(tmp1,_mm_cmplt_epi16(tmp1,zero));
					temp=_mm_add_epi32(temp,tmp1);
					tmp1=_mm_loadl_epi64((__m128i*)(hsrc+x-i_src));
					tmp1=_mm_unpacklo_epi16(tmp1,_mm_cmplt_epi16(tmp1,zero));
					d=_mm_sub_epi32(d,tmp1);
					temp=_mm_sub_epi32(temp,tmp1);
					temp=_mm_slli_epi32(temp,2);
					d=_mm_add_epi32(d,temp);
					tmp1=_mm_loadl_epi64((__m128i*)(hsrc+x));
					tmp1=_mm_unpacklo_epi16(tmp1,_mm_cmplt_epi16(tmp1,zero));
					d=_mm_add_epi32(d,tmp1);
					tmp=_mm_slli_epi32(_mm_add_epi32(tmp,tmp1),4);
					d=_mm_add_epi32(d,tmp);
					tmp1=_mm_loadl_epi64((__m128i*)(hsrc+x+4*i_src));
					tmp1=_mm_unpacklo_epi16(tmp1,_mm_cmplt_epi16(tmp1,zero));
					d=_mm_sub_epi32(d,tmp1);

					d=_mm_srai_epi32(d,6);
					d=_mm_packs_epi32(d,d);

					_mm_storel_epi64((__m128i*)(dst+x),d);

				}
#else
				for( x = 0; x < i_width; x++ )
				{
					dst[x] = (TAPFILTER( hsrc, f )>>6);
				}
#endif
				dst  += *i_dst;
				hsrc += i_src;
			}
		}
	}
	//_mm_empty();
	return rt;
}


void mc_avg_sse4( pixel *dst, int i_dst,
			int16_t *src1, int i_src1, int16_t *src2, int i_src2,
			int i_width, int i_height )
{
	if( i_width <= 8 )
	{
		int y;
		__m128i s1,s2,d[2];

		for( y = 0; y < i_height; y++ )
		{
			s1=_mm_load_si128((__m128i*)(src1));
			s2=_mm_load_si128((__m128i*)(src2));

			d[0]=_mm_add_epi32(_mm_cvtepi16_epi32(s1),_mm_cvtepi16_epi32(s2));
			d[1]=_mm_add_epi32(_mm_cvtepi16_epi32(_mm_shuffle_epi32(s1,_MM_SHUFFLE(0,0,3,2))),_mm_cvtepi16_epi32(_mm_shuffle_epi32(s2,_MM_SHUFFLE(0,0,3,2))));

			d[0]=_mm_add_epi32(d[0],offset64);
			d[0]=_mm_srai_epi32(d[0],7);
			d[1]=_mm_add_epi32(d[1],offset64);
			d[1]=_mm_srai_epi32(d[1],7);

			d[0]=_mm_packus_epi32(d[0],d[1]);
			d[0]=_mm_packus_epi16(d[0],d[0]);

			_mm_storel_epi64((__m128i*)(dst),d[0]);//sse3

			dst  += i_dst;
			src1 += i_src1;
			src2 += i_src2;
		}
	}
	else if( i_width == 16 )
	{
		int y;
		__m128i s1,s2,s3,d[2];

		for( y = 0; y < i_height; y++ )
		{
			s1=_mm_load_si128((__m128i*)(src1));
			s2=_mm_load_si128((__m128i*)(src2));

			d[0]=_mm_add_epi32(_mm_cvtepi16_epi32(s1),_mm_cvtepi16_epi32(s2));
			d[1]=_mm_add_epi32(_mm_cvtepi16_epi32(_mm_shuffle_epi32(s1,_MM_SHUFFLE(0,0,3,2))),_mm_cvtepi16_epi32(_mm_shuffle_epi32(s2,_MM_SHUFFLE(0,0,3,2))));

			d[0]=_mm_add_epi32(d[0],offset64);
			d[0]=_mm_srai_epi32(d[0],7);
			d[1]=_mm_add_epi32(d[1],offset64);
			d[1]=_mm_srai_epi32(d[1],7);

			s3=_mm_packus_epi32(d[0],d[1]);

			s1=_mm_load_si128((__m128i*)(src1+8));
			s2=_mm_load_si128((__m128i*)(src2+8));

			d[0]=_mm_add_epi32(_mm_cvtepi16_epi32(s1),_mm_cvtepi16_epi32(s2));
			d[1]=_mm_add_epi32(_mm_cvtepi16_epi32(_mm_shuffle_epi32(s1,_MM_SHUFFLE(0,0,3,2))),_mm_cvtepi16_epi32(_mm_shuffle_epi32(s2,_MM_SHUFFLE(0,0,3,2))));

			d[0]=_mm_add_epi32(d[0],offset64);
			d[0]=_mm_srai_epi32(d[0],7);
			d[1]=_mm_add_epi32(d[1],offset64);
			d[1]=_mm_srai_epi32(d[1],7);

			s1=_mm_packus_epi32(d[0],d[1]);

			d[0] = _mm_packus_epi16( s3, s1 );

			_mm_store_si128((__m128i*)(dst),d[0]);

			dst  += i_dst;
			src1 += i_src1;
			src2 += i_src2;
		}
	}
	else
	{
		int x, y;
		__m128i s1,s2,s3,d[2];

		for( y = 0; y < i_height; y++ )
		{
			for( x = 0; x < i_width; x+=16 )
			{
				//dst[x] = lent_clip_uint8((src1[x] + src2[x] + ((1<<6))) >> 7);

				s1=_mm_load_si128((__m128i*)(src1+x));
				s2=_mm_load_si128((__m128i*)(src2+x));

				d[0]=_mm_add_epi32(_mm_cvtepi16_epi32(s1),_mm_cvtepi16_epi32(s2));
				d[1]=_mm_add_epi32(_mm_cvtepi16_epi32(_mm_shuffle_epi32(s1,_MM_SHUFFLE(0,0,3,2))),_mm_cvtepi16_epi32(_mm_shuffle_epi32(s2,_MM_SHUFFLE(0,0,3,2))));

				d[0]=_mm_add_epi32(d[0],offset64);
				d[0]=_mm_srai_epi32(d[0],7);
				d[1]=_mm_add_epi32(d[1],offset64);
				d[1]=_mm_srai_epi32(d[1],7);

				s3=_mm_packus_epi32(d[0],d[1]);
				
				s1=_mm_load_si128((__m128i*)(src1+x+8));
				s2=_mm_load_si128((__m128i*)(src2+x+8));

				d[0]=_mm_add_epi32(_mm_cvtepi16_epi32(s1),_mm_cvtepi16_epi32(s2));
				d[1]=_mm_add_epi32(_mm_cvtepi16_epi32(_mm_shuffle_epi32(s1,_MM_SHUFFLE(0,0,3,2))),_mm_cvtepi16_epi32(_mm_shuffle_epi32(s2,_MM_SHUFFLE(0,0,3,2))));

				d[0]=_mm_add_epi32(d[0],offset64);
				d[0]=_mm_srai_epi32(d[0],7);
				d[1]=_mm_add_epi32(d[1],offset64);
				d[1]=_mm_srai_epi32(d[1],7);

				s1=_mm_packus_epi32(d[0],d[1]);

				d[0] = _mm_packus_epi16( s3, s1 );

				_mm_store_si128((__m128i*)(dst+x),d[0]);

			}
			dst  += i_dst;
			src1 += i_src1;
			src2 += i_src2;
		}
	}

}
void mc_avg_sse2( pixel *dst, int i_dst,
			int16_t *src1, int i_src1, int16_t *src2, int i_src2,
			int i_width, int i_height )
{

	int x, y;
	__m128i s1,s2,d[2],sign[2];

	////_mm_empty();

	for( y = 0; y < i_height; y++ )
	{
#if 1
		for( x = 0; x < i_width; x+=8 )
		{
			//dst[x] = lent_clip_uint8((src1[x] + src2[x] + ((1<<6))) >> 7);

			s1=_mm_load_si128((__m128i*)(src1+x));
			s2=_mm_load_si128((__m128i*)(src2+x));

			sign[0]=_mm_cmplt_epi16(s1,zero);
			sign[1]=_mm_cmplt_epi16(s2,zero);
			d[0]=_mm_add_epi32(_mm_unpacklo_epi16(s1,sign[0]),_mm_unpacklo_epi16(s2,sign[1]));
			d[1]=_mm_add_epi32(_mm_unpackhi_epi16(s1,sign[0]),_mm_unpackhi_epi16(s2,sign[1]));
/*
			d[0] = _mm_add_epi32(_mm_srai_epi32(_mm_unpacklo_epi16(s1, s1), 16), _mm_srai_epi32(_mm_unpacklo_epi16(s2, s2), 16));
			d[1] = _mm_add_epi32(_mm_srai_epi32(_mm_unpackhi_epi16(s1, s1), 16), _mm_srai_epi32(_mm_unpackhi_epi16(s2, s2), 16));
*/

			d[0]=_mm_add_epi32(d[0],offset64);
			d[0]=_mm_srai_epi32(d[0],7);
			d[1]=_mm_add_epi32(d[1],offset64);
			d[1]=_mm_srai_epi32(d[1],7);
/*
					tmp=_mm_cmpgt_epi32(d[0],zero);
					d[0]=_mm_and_si128(d[0],tmp);

					tmp=_mm_cmpgt_epi32(d[0],max8bit);
					d[0]=_mm_or_si128(d[0],tmp);
					d[0]=_mm_and_si128(d[0],max8bit);

					
					tmp=_mm_cmpgt_epi32(d[1],zero);
					d[1]=_mm_and_si128(d[1],tmp);

					tmp=_mm_cmpgt_epi32(d[1],max8bit);
					d[1]=_mm_or_si128(d[1],tmp);
					d[1]=_mm_and_si128(d[1],max8bit);
*/			
			d[0]=_mm_packs_epi32(d[0],d[1]);
			d[0]=_mm_packus_epi16(d[0],d[0]);

			_mm_storel_epi64((__m128i*)(dst+x),d[0]);

		}
#else
		for( x = 0; x < i_width; x+=4 )
		{
			//dst[x] = lent_clip_uint8((src1[x] + src2[x] + ((1<<6))) >> 7);

			s1=_mm_loadl_epi64((__m128i*)(src1+x));
			s2=_mm_loadl_epi64((__m128i*)(src2+x));
			s1=_mm_cvtepi16_epi32(s1);
			s2=_mm_cvtepi16_epi32(s2);
			
			d[0]=_mm_add_epi32(s1,s2);
			d[0]=_mm_add_epi32(d[0],offset64);
			d[0]=_mm_srai_epi32(d[0],7);
			d[0]=_mm_packs_epi32(d[0],d[0]);
			d[0]=_mm_packus_epi16(d[0],d[0]);

			//_mm_storel_epi64((__m128i*)(dst+x),d[0]);//sse3
			*((int*)(dst+x))=_mm_cvtsi128_si32(d[0]);
			//assert(dst[x]==lent_clip_uint8((src1[x] + src2[x] + ((1<<6))) >> 7));
			//assert(dst[x+1]==lent_clip_uint8((src1[x+1] + src2[x+1] + ((1<<6))) >> 7));
			//assert(dst[x+2]==lent_clip_uint8((src1[x+2] + src2[x+2] + ((1<<6))) >> 7));
			//assert(dst[x+3]==lent_clip_uint8((src1[x+3] + src2[x+3] + ((1<<6))) >> 7));

		}
#endif
		dst  += i_dst;
		src1 += i_src1;
		src2 += i_src2;
	}
	////_mm_empty();
}

void mc_chroma_sse4( pixel *dst, int i_dst, pixel *src, int i_src,
			   int mvx, int mvy, int i_width, int i_height )
{
	//pixel *tmp;
	int x, y;
	int fx = mvx&0x7;
	int fy = mvy&0x7;

	__m128i s8,d[4],tmp;
	__m128i shuffle={
		0,1,2,3,
		1,2,3,4,
		2,3,4,5,
		3,4,5,6,
	};
	////_mm_empty();


	//s=frac_coeff_ch[fx];
	s8=frac_coeff_ch_8bit[fx];
	tmp=_mm_set1_epi16(32);


	src += (mvy >> 3) * i_src + (mvx >> 3);
	//tmp = &src[i_src];

	if( fy == 0 )
	{
		//f = chroma_frac_coeff[fx];

#if 1
		if(fx==0)
			mc_copy( dst, i_dst, src, i_src, i_width, i_height );
		else
#endif
		for( y = 0; y < i_height; y++ )
		{
			_mm_prefetch(src+i_src-1,_MM_HINT_T0);
			for( x = 0; x < i_width; x+=8 )
			{
				//dst[x] = lent_clip_uint8(( f[0]*src[x-1]  + f[1]*src[x] + f[2]*src[x+1] + f[3]*src[x+2] + 32 ) >> 6);
				d[0]=_mm_loadu_si128((__m128i*)(src+x-1));//8 pixels
				d[1]=_mm_shuffle_epi8(d[0],shuffle);

				d[2]=_mm_srli_si128(d[0],4);
				d[2]=_mm_shuffle_epi8(d[2],shuffle);

				d[1]=_mm_maddubs_epi16(d[1],s8);
				d[2]=_mm_maddubs_epi16(d[2],s8);

				d[0]=_mm_hadd_epi16(d[1],d[2]);

				d[0]=_mm_add_epi16(d[0],tmp);
				d[0]=_mm_srai_epi16(d[0],6);
				d[0]=_mm_packus_epi16(d[0],d[0]);

				_mm_storel_epi64((__m128i*)(dst+x),d[0]);//sse3
				//*((int*)(dst+x))=_mm_cvtsi128_si32(d[0]);
			}
			dst  += i_dst;
			src += i_src;
		}
	}
	else if( fx == 0 )
	{
		if (fy==1){
			for( y = 0; y < i_height; y++ )
			{
#if 1
				_mm_prefetch(src,_MM_HINT_NTA);
				_mm_prefetch(src+i_src,_MM_HINT_NTA);
				_mm_prefetch(src+i_src*2,_MM_HINT_NTA);
				for( x = 0; x < i_width; x+=8 )
				{
					//dst[x] = lent_clip_uint8(( f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src] + 32 ) >> 6);

					d[1] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x)),zero);
					d[0] = _mm_add_epi16(_mm_slli_epi16(d[1],5),_mm_slli_epi16(d[1],4));
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+i_src)),zero);
					d[1] = _mm_add_epi16(d[1],d[2]);
					d[0] = _mm_add_epi16(d[0],_mm_slli_epi16(d[1],3));
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x-1*i_src)),zero);
					d[1] = _mm_sub_epi16(d[1],d[2]);
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+2*i_src)),zero);
					d[1] = _mm_sub_epi16(d[1],d[2]);
					d[0] = _mm_add_epi16(d[0],_mm_slli_epi16(d[1],1));

					d[0]=_mm_add_epi16(d[0],tmp);
					d[0]=_mm_srai_epi16(d[0],6);
					d[0]=_mm_packus_epi16(d[0],d[0]);

					_mm_storel_epi64((__m128i*)(dst+x),d[0]);
				}
#else
				for( x = 0; x < i_width; x++ )
					dst[x] = lent_clip_uint8(( f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src] + 32 ) >> 6);
#endif
				dst  += i_dst;
				src += i_src;
			}
		}
		else if (fy==7)
		{
			for( y = 0; y < i_height; y++ )
			{
#if 1
				_mm_prefetch(src,_MM_HINT_NTA);
				_mm_prefetch(src+i_src,_MM_HINT_NTA);
				_mm_prefetch(src+i_src*2,_MM_HINT_NTA);
				for( x = 0; x < i_width; x+=8 )
				{
					//dst[x] = lent_clip_uint8(( f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src] + 32 ) >> 6);

					d[1] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+1*i_src)),zero);
					d[0] = _mm_add_epi16(_mm_slli_epi16(d[1],5),_mm_slli_epi16(d[1],4));
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x)),zero);
					d[1] = _mm_add_epi16(d[1],d[2]);
					d[0] = _mm_add_epi16(d[0],_mm_slli_epi16(d[1],3));
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+2*i_src)),zero);
					d[1] = _mm_sub_epi16(d[1],d[2]);
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x-1*i_src)),zero);
					d[1] = _mm_sub_epi16(d[1],d[2]);
					d[0] = _mm_add_epi16(d[0],_mm_slli_epi16(d[1],1));

					d[0]=_mm_add_epi16(d[0],tmp);
					d[0]=_mm_srai_epi16(d[0],6);
					d[0]=_mm_packus_epi16(d[0],d[0]);

					_mm_storel_epi64((__m128i*)(dst+x),d[0]);
				}
#else
				for( x = 0; x < i_width; x++ )
					dst[x] = lent_clip_uint8(( f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src] + 32 ) >> 6);
#endif
				dst  += i_dst;
				src += i_src;
			}
		}
		else if (fy==2)
		{
			for( y = 0; y < i_height; y++ )
			{
#if 1
				_mm_prefetch(src,_MM_HINT_NTA);
				_mm_prefetch(src+i_src,_MM_HINT_NTA);
				_mm_prefetch(src+i_src*2,_MM_HINT_NTA);
				for( x = 0; x < i_width; x+=8 )
				{
					//dst[x] = lent_clip_uint8(( f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src] + 32 ) >> 6);

					d[1] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x)),zero);
					d[0] = _mm_slli_epi16(d[1],5);
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+i_src)),zero);
					d[2] = _mm_add_epi16(d[1],d[2]);
					d[0] = _mm_add_epi16(d[0],_mm_slli_epi16(d[2],4));
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x-i_src)),zero);
					d[2] = _mm_sub_epi16(d[1],d[2]);
					d[0] = _mm_add_epi16(d[0],_mm_slli_epi16(d[2],2));
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+2*i_src)),zero);
					d[2] = _mm_sub_epi16(d[1],d[2]);
					d[0] = _mm_add_epi16(d[0],_mm_slli_epi16(d[2],1));

					d[0]=_mm_add_epi16(d[0],tmp);
					d[0]=_mm_srai_epi16(d[0],6);
					d[0]=_mm_packus_epi16(d[0],d[0]);

					_mm_storel_epi64((__m128i*)(dst+x),d[0]);
				}
#else
				for( x = 0; x < i_width; x++ )
					dst[x] = lent_clip_uint8(( f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src] + 32 ) >> 6);
#endif
				dst  += i_dst;
				src += i_src;
			}
		}
		else if (fy==6)
		{
			for( y = 0; y < i_height; y++ )
			{
#if 1
				_mm_prefetch(src,_MM_HINT_NTA);
				_mm_prefetch(src+i_src,_MM_HINT_NTA);
				_mm_prefetch(src+i_src*2,_MM_HINT_NTA);
				for( x = 0; x < i_width; x+=8 )
				{
					//dst[x] = lent_clip_uint8(( f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src] + 32 ) >> 6);

					d[1] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+i_src)),zero);
					d[0] = _mm_slli_epi16(d[1],5);
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x)),zero);
					d[2] = _mm_add_epi16(d[1],d[2]);
					d[0] = _mm_add_epi16(d[0],_mm_slli_epi16(d[2],4));
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+2*i_src)),zero);
					d[2] = _mm_sub_epi16(d[1],d[2]);
					d[0] = _mm_add_epi16(d[0],_mm_slli_epi16(d[2],2));
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x-i_src)),zero);
					d[2] = _mm_sub_epi16(d[1],d[2]);
					d[0] = _mm_add_epi16(d[0],_mm_slli_epi16(d[2],1));

					d[0]=_mm_add_epi16(d[0],tmp);
					d[0]=_mm_srai_epi16(d[0],6);
					d[0]=_mm_packus_epi16(d[0],d[0]);

					_mm_storel_epi64((__m128i*)(dst+x),d[0]);
				}
#else
				for( x = 0; x < i_width; x++ )
					dst[x] = lent_clip_uint8(( f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src] + 32 ) >> 6);
#endif
				dst  += i_dst;
				src += i_src;
			}
		}
		else if (fy==3)
		{
			for( y = 0; y < i_height; y++ )
			{
#if 1
				_mm_prefetch(src,_MM_HINT_NTA);
				_mm_prefetch(src+i_src,_MM_HINT_NTA);
				_mm_prefetch(src+i_src*2,_MM_HINT_NTA);
				for( x = 0; x < i_width; x+=8 )
				{
					//dst[x] = lent_clip_uint8(( f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src] + 32 ) >> 6);

					d[1] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x)),zero);
					d[0] = _mm_slli_epi16(d[1],3);
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x-i_src)),zero);
					d[3] = _mm_sub_epi16(d[1],d[2]);
					d[0] = _mm_add_epi16(d[0],_mm_slli_epi16(d[3],1));
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+i_src)),zero);
					d[0] = _mm_add_epi16(d[0],_mm_slli_epi16(_mm_add_epi16(d[1],d[2]),5));
					d[3] = _mm_sub_epi16(d[3],d[2]);
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+2*i_src)),zero);
					d[1] = _mm_sub_epi16(d[3],d[2]);
					d[0] = _mm_add_epi16(d[0],_mm_slli_epi16(d[1],2));

					d[0]=_mm_add_epi16(d[0],tmp);
					d[0]=_mm_srai_epi16(d[0],6);
					d[0]=_mm_packus_epi16(d[0],d[0]);

					_mm_storel_epi64((__m128i*)(dst+x),d[0]);
				}
#else
				for( x = 0; x < i_width; x++ )
					dst[x] = lent_clip_uint8(( f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src] + 32 ) >> 6);
#endif
				dst  += i_dst;
				src += i_src;
			}
		}
		else if (fy==5)
		{
			for( y = 0; y < i_height; y++ )
			{
#if 1
				_mm_prefetch(src,_MM_HINT_NTA);
				_mm_prefetch(src+i_src,_MM_HINT_NTA);
				_mm_prefetch(src+i_src*2,_MM_HINT_NTA);
				for( x = 0; x < i_width; x+=8 )
				{
					//dst[x] = lent_clip_uint8(( f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src] + 32 ) >> 6);

					d[1] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+i_src)),zero);
					d[0] = _mm_slli_epi16(d[1],3);
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+2*i_src)),zero);
					d[3] = _mm_sub_epi16(d[1],d[2]);
					d[0] = _mm_add_epi16(d[0],_mm_slli_epi16(d[3],1));
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x)),zero);
					d[0] = _mm_add_epi16(d[0],_mm_slli_epi16(_mm_add_epi16(d[1],d[2]),5));
					d[3] = _mm_sub_epi16(d[3],d[2]);
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x-i_src)),zero);
					d[1] = _mm_sub_epi16(d[3],d[2]);
					d[0] = _mm_add_epi16(d[0],_mm_slli_epi16(d[1],2));


					d[0]=_mm_add_epi16(d[0],tmp);
					d[0]=_mm_srai_epi16(d[0],6);
					d[0]=_mm_packus_epi16(d[0],d[0]);

					_mm_storel_epi64((__m128i*)(dst+x),d[0]);
				}
#else
				for( x = 0; x < i_width; x++ )
					dst[x] = lent_clip_uint8(( f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src] + 32 ) >> 6);
#endif
				dst  += i_dst;
				src += i_src;
			}
		}
		else
		{
			for( y = 0; y < i_height; y++ )
			{
#if 1
				_mm_prefetch(src,_MM_HINT_NTA);
				_mm_prefetch(src+i_src,_MM_HINT_NTA);
				_mm_prefetch(src+i_src*2,_MM_HINT_NTA);
				for( x = 0; x < i_width; x+=8 )
				{
					//dst[x] = lent_clip_uint8(( f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src] + 32 ) >> 6);

					d[1] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x)),zero);
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+i_src)),zero);
					d[1] = _mm_add_epi16(d[1],d[2]);
					d[0] = _mm_slli_epi16(d[1],5);

					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x-i_src)),zero);
					d[1] = _mm_sub_epi16(d[1],d[2]);
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+2*i_src)),zero);
					d[1] = _mm_sub_epi16(d[1],d[2]);
					d[0] = _mm_add_epi16(d[0],_mm_slli_epi16(d[1],2));

					d[0]=_mm_add_epi16(d[0],tmp);
					d[0]=_mm_srai_epi16(d[0],6);
					d[0]=_mm_packus_epi16(d[0],d[0]);

					_mm_storel_epi64((__m128i*)(dst+x),d[0]);
				}
#else
				for( x = 0; x < i_width; x++ )
					dst[x] = lent_clip_uint8(( f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src] + 32 ) >> 6);
#endif
				dst  += i_dst;
				src += i_src;
			}
		}
	}
	else
	{
		STACK_ALIGN(16,dctcoef,buf_array,32*35)
		dctcoef *buf = buf_array;

		//tmp = src;
		src -= i_src;
		_mm_prefetch(buf_array,_MM_HINT_NTA);
		for( y = -1; y < i_height+2; y++ )
		{
#if 1
			_mm_prefetch(src+i_src-1,_MM_HINT_NTA);
			for( x = 0; x < i_width; x+=8 )
			{
				

				d[0]=_mm_loadu_si128((__m128i*)(src+x-1));//8 pixels
				d[1]=_mm_shuffle_epi8(d[0],shuffle);

				d[2]=_mm_srli_si128(d[0],4);
				d[2]=_mm_shuffle_epi8(d[2],shuffle);

				d[1]=_mm_maddubs_epi16(d[1],s8);
				d[2]=_mm_maddubs_epi16(d[2],s8);

				d[1]=_mm_hadd_epi16(d[1],d[2]);

				_mm_store_si128((__m128i*)(buf+x),d[1]);//sse3
			}
#else
			{
				int16_t *f=chroma_frac_coeff[fx];
				for( x = 0; x < i_width; x++ )
				{
					buf[x] = f[0]*src[x-1]  + f[1]*src[x] + f[2]*src[x+1] + f[3]*src[x+2];
				}
			}
#endif
			buf  += 32;
			src  += i_src;
		}
		
		buf = buf_array + 32;
		tmp=_mm_set1_epi32(2048);
		_mm_prefetch(buf_array-32,_MM_HINT_NTA);
		if (fy==1){
			for( y = 0; y < i_height; y++ )
			{
#if 1
				for( x = 0; x < i_width; x+=4 )
				{
					d[1] = _mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(buf+x)));
					d[0] = _mm_add_epi32(_mm_slli_epi32(d[1],5),_mm_slli_epi32(d[1],4));
					d[2] = _mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(buf+x+32)));
					d[1] = _mm_add_epi32(d[1],d[2]);
					d[0] = _mm_add_epi32(d[0],_mm_slli_epi32(d[1],3));
					d[2] = _mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(buf+x-32)));
					d[1] = _mm_sub_epi32(d[1],d[2]);
					d[2] = _mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(buf+x+64)));
					d[1] = _mm_sub_epi32(d[1],d[2]);
					d[0] = _mm_add_epi32(d[0],_mm_slli_epi32(d[1],1));


					d[0]=_mm_add_epi32(d[0],tmp);
					d[0]=_mm_srai_epi32(d[0],12);

					d[0]=_mm_packus_epi32(d[0],d[0]);
					d[0]=_mm_packus_epi16(d[0],d[0]);

					*((int*)(dst+x))=_mm_cvtsi128_si32(d[0]);
				}
#else
				for( x = 0; x < i_width; x++ )
					dst[x] = lent_clip_uint8(( f[0]*buf[x-32]  + f[1]*buf[x] + f[2]*buf[x+32] + f[3]*buf[x+2*32] + ((1<<11)) ) >> 12);
#endif
				dst  += i_dst;
				buf  += 32;
			}
		}
		else if (fy==7){
			for( y = 0; y < i_height; y++ )
			{
#if 1
				for( x = 0; x < i_width; x+=4 )
				{
					//dst[x] = (f[0]*buf[x-32]  + f[1]*buf[x] + f[2]*buf[x+32] + f[3]*buf[x+2*32]) >> 6;

					d[1] = _mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(buf+x+32)));
					d[0] = _mm_add_epi32(_mm_slli_epi32(d[1],5),_mm_slli_epi32(d[1],4));
					d[2] = _mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(buf+x)));
					d[1] = _mm_add_epi32(d[1],d[2]);
					d[0] = _mm_add_epi32(d[0],_mm_slli_epi32(d[1],3));
					d[2] = _mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(buf+x+64)));
					d[1] = _mm_sub_epi32(d[1],d[2]);
					d[2] = _mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(buf+x-32)));
					d[1] = _mm_sub_epi32(d[1],d[2]);
					d[0] = _mm_add_epi32(d[0],_mm_slli_epi32(d[1],1));

					d[0]=_mm_add_epi32(d[0],tmp);
					d[0]=_mm_srai_epi32(d[0],12);

					d[0]=_mm_packus_epi32(d[0],d[0]);
					d[0]=_mm_packus_epi16(d[0],d[0]);

					*((int*)(dst+x))=_mm_cvtsi128_si32(d[0]);
				}
#else
				for( x = 0; x < i_width; x++ )
					dst[x] = lent_clip_uint8(( f[0]*buf[x-32]  + f[1]*buf[x] + f[2]*buf[x+32] + f[3]*buf[x+2*32] + ((1<<11)) ) >> 12);
#endif
				dst  += i_dst;
				buf  += 32;
			}
		}
		else if (fy==2){
			for( y = 0; y < i_height; y++ )
			{
#if 1
				for( x = 0; x < i_width; x+=4 )
				{
					//dst[x] = (f[0]*buf[x-32]  + f[1]*buf[x] + f[2]*buf[x+32] + f[3]*buf[x+2*32]) >> 6;

					d[1] = _mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(buf+x)));
					d[0] = _mm_slli_epi32(d[1],5);
					d[2] = _mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(buf+x+32)));
					d[2] = _mm_add_epi32(d[1],d[2]);
					d[0] = _mm_add_epi32(d[0],_mm_slli_epi32(d[2],4));
					d[2] = _mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(buf+x-32)));
					d[2] = _mm_sub_epi32(d[1],d[2]);
					d[0] = _mm_add_epi32(d[0],_mm_slli_epi32(d[2],2));
					d[2] = _mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(buf+x+64)));
					d[2] = _mm_sub_epi32(d[1],d[2]);
					d[0] = _mm_add_epi32(d[0],_mm_slli_epi32(d[2],1));

					d[0]=_mm_add_epi32(d[0],tmp);
					d[0]=_mm_srai_epi32(d[0],12);

					d[0]=_mm_packus_epi32(d[0],d[0]);
					d[0]=_mm_packus_epi16(d[0],d[0]);

					*((int*)(dst+x))=_mm_cvtsi128_si32(d[0]);
				}
#else
				for( x = 0; x < i_width; x++ )
					dst[x] = lent_clip_uint8(( f[0]*buf[x-32]  + f[1]*buf[x] + f[2]*buf[x+32] + f[3]*buf[x+2*32] + ((1<<11)) ) >> 12);
#endif
				dst  += i_dst;
				buf  += 32;
			}
		}
		else if (fy==6){
			for( y = 0; y < i_height; y++ )
			{
#if 1
				for( x = 0; x < i_width; x+=4 )
				{
					//dst[x] = (f[0]*buf[x-32]  + f[1]*buf[x] + f[2]*buf[x+32] + f[3]*buf[x+2*32]) >> 6;

					d[1] = _mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(buf+x+32)));
					d[0] = _mm_slli_epi32(d[1],5);
					d[2] = _mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(buf+x)));
					d[2] = _mm_add_epi32(d[1],d[2]);
					d[0] = _mm_add_epi32(d[0],_mm_slli_epi32(d[2],4));
					d[2] = _mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(buf+x+64)));
					d[2] = _mm_sub_epi32(d[1],d[2]);
					d[0] = _mm_add_epi32(d[0],_mm_slli_epi32(d[2],2));
					d[2] = _mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(buf+x-32)));
					d[2] = _mm_sub_epi32(d[1],d[2]);
					d[0] = _mm_add_epi32(d[0],_mm_slli_epi32(d[2],1));

					d[0]=_mm_add_epi32(d[0],tmp);
					d[0]=_mm_srai_epi32(d[0],12);

					d[0]=_mm_packus_epi32(d[0],d[0]);
					d[0]=_mm_packus_epi16(d[0],d[0]);

					*((int*)(dst+x))=_mm_cvtsi128_si32(d[0]);
				}
#else
				for( x = 0; x < i_width; x++ )
					dst[x] = lent_clip_uint8(( f[0]*buf[x-32]  + f[1]*buf[x] + f[2]*buf[x+32] + f[3]*buf[x+2*32] + ((1<<11)) ) >> 12);
#endif
				dst  += i_dst;
				buf  += 32;
			}
		}
		else if (fy==3){
			for( y = 0; y < i_height; y++ )
			{
#if 1
				for( x = 0; x < i_width; x+=4 )
				{
					d[1] = _mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(buf+x)));
					d[0] = _mm_slli_epi32(d[1],3);
					d[2] = _mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(buf+x-32)));
					d[3] = _mm_sub_epi32(d[1],d[2]);
					d[0] = _mm_add_epi32(d[0],_mm_slli_epi32(d[3],1));
					d[2] = _mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(buf+x+32)));
					d[0] = _mm_add_epi32(d[0],_mm_slli_epi32(_mm_add_epi32(d[1],d[2]),5));
					d[3] = _mm_sub_epi32(d[3],d[2]);
					d[2] = _mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(buf+x+64)));
					d[1] = _mm_sub_epi32(d[3],d[2]);
					d[0] = _mm_add_epi32(d[0],_mm_slli_epi32(d[1],2));

					d[0]=_mm_add_epi32(d[0],tmp);
					d[0]=_mm_srai_epi32(d[0],12);

					d[0]=_mm_packus_epi32(d[0],d[0]);
					d[0]=_mm_packus_epi16(d[0],d[0]);

					*((int*)(dst+x))=_mm_cvtsi128_si32(d[0]);
				}
#else
				for( x = 0; x < i_width; x++ )
					dst[x] = lent_clip_uint8(( f[0]*buf[x-32]  + f[1]*buf[x] + f[2]*buf[x+32] + f[3]*buf[x+2*32] + ((1<<11)) ) >> 12);
#endif
				dst  += i_dst;
				buf  += 32;
			}
		}
		else if (fy==5){
			for( y = 0; y < i_height; y++ )
			{
#if 1
				for( x = 0; x < i_width; x+=4 )
				{
					d[1] = _mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(buf+x+32)));
					d[0] = _mm_slli_epi32(d[1],3);
					d[2] = _mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(buf+x+64)));
					d[3] = _mm_sub_epi32(d[1],d[2]);
					d[0] = _mm_add_epi32(d[0],_mm_slli_epi32(d[3],1));
					d[2] = _mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(buf+x)));
					d[0] = _mm_add_epi32(d[0],_mm_slli_epi32(_mm_add_epi32(d[1],d[2]),5));
					d[3] = _mm_sub_epi32(d[3],d[2]);
					d[2] = _mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(buf+x-32)));
					d[1] = _mm_sub_epi32(d[3],d[2]);
					d[0] = _mm_add_epi32(d[0],_mm_slli_epi32(d[1],2));

					d[0]=_mm_add_epi32(d[0],tmp);
					d[0]=_mm_srai_epi32(d[0],12);

					d[0]=_mm_packus_epi32(d[0],d[0]);
					d[0]=_mm_packus_epi16(d[0],d[0]);

					*((int*)(dst+x))=_mm_cvtsi128_si32(d[0]);
				}
#else
				for( x = 0; x < i_width; x++ )
					dst[x] = lent_clip_uint8(( f[0]*buf[x-32]  + f[1]*buf[x] + f[2]*buf[x+32] + f[3]*buf[x+2*32] + ((1<<11)) ) >> 12);
#endif
				dst  += i_dst;
				buf  += 32;
			}
		}
		else{
			for( y = 0; y < i_height; y++ )
			{
#if 1
				for( x = 0; x < i_width; x+=4 )
				{
					//dst[x] = (f[0]*buf[x-32]  + f[1]*buf[x] + f[2]*buf[x+32] + f[3]*buf[x+2*32]) >> 6;

					d[1] = _mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(buf+x)));
					d[2] = _mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(buf+x+32)));
					d[1] = _mm_add_epi32(d[1],d[2]);
					d[0] = _mm_slli_epi32(d[1],5);

					d[2] = _mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(buf+x-32)));
					d[1] = _mm_sub_epi32(d[1],d[2]);
					d[2] = _mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(buf+x+64)));
					d[1] = _mm_sub_epi32(d[1],d[2]);
					d[0] = _mm_add_epi32(d[0],_mm_slli_epi32(d[1],2));

					d[0]=_mm_add_epi32(d[0],tmp);
					d[0]=_mm_srai_epi32(d[0],12);

					d[0]=_mm_packus_epi32(d[0],d[0]);
					d[0]=_mm_packus_epi16(d[0],d[0]);

					*((int*)(dst+x))=_mm_cvtsi128_si32(d[0]);
				}
#else
				for( x = 0; x < i_width; x++ )
					dst[x] = lent_clip_uint8(( f[0]*buf[x-32]  + f[1]*buf[x] + f[2]*buf[x+32] + f[3]*buf[x+2*32] + ((1<<11)) ) >> 12);
#endif
				dst  += i_dst;
				buf  += 32;
			}
		}
	}
	////_mm_empty();
}
void mc_chroma_ssse3( pixel *dst, int i_dst, pixel *src, int i_src,
			   int mvx, int mvy, int i_width, int i_height )
{
	//pixel *tmp;
	int x, y;
	int fx = mvx&0x7;
	int fy = mvy&0x7;

	__m128i s8,d[4],tmp;
	__m128i shuffle={
		0,1,2,3,
		1,2,3,4,
		2,3,4,5,
		3,4,5,6,
	};
	////_mm_empty();


	//s=frac_coeff_ch[fx];
	s8=frac_coeff_ch_8bit[fx];
	tmp=_mm_set1_epi16(32);


	src += (mvy >> 3) * i_src + (mvx >> 3);
	//tmp = &src[i_src];

	if( fy == 0 )
	{
		//f = chroma_frac_coeff[fx];

#if 1
		if(fx==0)
			mc_copy( dst, i_dst, src, i_src, i_width, i_height );
		else
#endif
		for( y = 0; y < i_height; y++ )
		{
#if 1
			for( x = 0; x < i_width; x+=8 )
			{
				//dst[x] = lent_clip_uint8(( f[0]*src[x-1]  + f[1]*src[x] + f[2]*src[x+1] + f[3]*src[x+2] + 32 ) >> 6);
				d[0]=_mm_loadu_si128((__m128i*)(src+x-1));//8 pixels
				d[1]=_mm_shuffle_epi8(d[0],shuffle);

				d[2]=_mm_srli_si128(d[0],4);
				d[2]=_mm_shuffle_epi8(d[2],shuffle);

				d[1]=_mm_maddubs_epi16(d[1],s8);
				d[2]=_mm_maddubs_epi16(d[2],s8);

				d[0]=_mm_hadd_epi16(d[1],d[2]);

				d[0]=_mm_add_epi16(d[0],tmp);
				d[0]=_mm_srai_epi16(d[0],6);
				d[0]=_mm_packus_epi16(d[0],d[0]);

				_mm_storel_epi64((__m128i*)(dst+x),d[0]);//sse3
				//*((int*)(dst+x))=_mm_cvtsi128_si32(d[0]);
				
			}
#else
			{
				int16_t *f = chroma_frac_coeff[fx];
				for( x = 0; x < i_width; x++ )
					dst[x] = lent_clip_uint8(( f[0]*src[x-1]  + f[1]*src[x] + f[2]*src[x+1] + f[3]*src[x+2] + 32 ) >> 6);
			}
#endif
			dst  += i_dst;
			src += i_src;
		}
	}
	else if( fx == 0 )
	{
		if (fy==1){
			for( y = 0; y < i_height; y++ )
			{
#if 1
				_mm_prefetch(src,_MM_HINT_NTA);
				_mm_prefetch(src+i_src,_MM_HINT_NTA);
				_mm_prefetch(src+i_src*2,_MM_HINT_NTA);
				for( x = 0; x < i_width; x+=8 )
				{
					//dst[x] = lent_clip_uint8(( f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src] + 32 ) >> 6);

					d[1] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x)),zero);
					d[0] = _mm_add_epi16(_mm_slli_epi16(d[1],5),_mm_slli_epi16(d[1],4));
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+i_src)),zero);
					d[1] = _mm_add_epi16(d[1],d[2]);
					d[0] = _mm_add_epi16(d[0],_mm_slli_epi16(d[1],3));
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x-1*i_src)),zero);
					d[1] = _mm_sub_epi16(d[1],d[2]);
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+2*i_src)),zero);
					d[1] = _mm_sub_epi16(d[1],d[2]);
					d[0] = _mm_add_epi16(d[0],_mm_slli_epi16(d[1],1));

					d[0]=_mm_add_epi16(d[0],tmp);
					d[0]=_mm_srai_epi16(d[0],6);
					d[0]=_mm_packus_epi16(d[0],d[0]);

					_mm_storel_epi64((__m128i*)(dst+x),d[0]);
				}
#else
				for( x = 0; x < i_width; x++ )
					dst[x] = lent_clip_uint8(( f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src] + 32 ) >> 6);
#endif
				dst  += i_dst;
				src += i_src;
			}
		}
		else if (fy==7)
		{
			for( y = 0; y < i_height; y++ )
			{
#if 1
				_mm_prefetch(src,_MM_HINT_NTA);
				_mm_prefetch(src+i_src,_MM_HINT_NTA);
				_mm_prefetch(src+i_src*2,_MM_HINT_NTA);
				for( x = 0; x < i_width; x+=8 )
				{
					//dst[x] = lent_clip_uint8(( f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src] + 32 ) >> 6);

					d[1] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+1*i_src)),zero);
					d[0] = _mm_add_epi16(_mm_slli_epi16(d[1],5),_mm_slli_epi16(d[1],4));
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x)),zero);
					d[1] = _mm_add_epi16(d[1],d[2]);
					d[0] = _mm_add_epi16(d[0],_mm_slli_epi16(d[1],3));
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+2*i_src)),zero);
					d[1] = _mm_sub_epi16(d[1],d[2]);
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x-1*i_src)),zero);
					d[1] = _mm_sub_epi16(d[1],d[2]);
					d[0] = _mm_add_epi16(d[0],_mm_slli_epi16(d[1],1));

					d[0]=_mm_add_epi16(d[0],tmp);
					d[0]=_mm_srai_epi16(d[0],6);
					d[0]=_mm_packus_epi16(d[0],d[0]);

					_mm_storel_epi64((__m128i*)(dst+x),d[0]);
				}
#else
				for( x = 0; x < i_width; x++ )
					dst[x] = lent_clip_uint8(( f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src] + 32 ) >> 6);
#endif
				dst  += i_dst;
				src += i_src;
			}
		}
		else if (fy==2)
		{
			for( y = 0; y < i_height; y++ )
			{
#if 1
				_mm_prefetch(src,_MM_HINT_NTA);
				_mm_prefetch(src+i_src,_MM_HINT_NTA);
				_mm_prefetch(src+i_src*2,_MM_HINT_NTA);
				for( x = 0; x < i_width; x+=8 )
				{
					//dst[x] = lent_clip_uint8(( f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src] + 32 ) >> 6);

					d[1] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x)),zero);
					d[0] = _mm_slli_epi16(d[1],5);
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+i_src)),zero);
					d[2] = _mm_add_epi16(d[1],d[2]);
					d[0] = _mm_add_epi16(d[0],_mm_slli_epi16(d[2],4));
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x-i_src)),zero);
					d[2] = _mm_sub_epi16(d[1],d[2]);
					d[0] = _mm_add_epi16(d[0],_mm_slli_epi16(d[2],2));
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+2*i_src)),zero);
					d[2] = _mm_sub_epi16(d[1],d[2]);
					d[0] = _mm_add_epi16(d[0],_mm_slli_epi16(d[2],1));

					d[0]=_mm_add_epi16(d[0],tmp);
					d[0]=_mm_srai_epi16(d[0],6);
					d[0]=_mm_packus_epi16(d[0],d[0]);

					_mm_storel_epi64((__m128i*)(dst+x),d[0]);
				}
#else
				for( x = 0; x < i_width; x++ )
					dst[x] = lent_clip_uint8(( f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src] + 32 ) >> 6);
#endif
				dst  += i_dst;
				src += i_src;
			}
		}
		else if (fy==6)
		{
			for( y = 0; y < i_height; y++ )
			{
#if 1
				_mm_prefetch(src,_MM_HINT_NTA);
				_mm_prefetch(src+i_src,_MM_HINT_NTA);
				_mm_prefetch(src+i_src*2,_MM_HINT_NTA);
				for( x = 0; x < i_width; x+=8 )
				{
					//dst[x] = lent_clip_uint8(( f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src] + 32 ) >> 6);

					d[1] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+i_src)),zero);
					d[0] = _mm_slli_epi16(d[1],5);
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x)),zero);
					d[2] = _mm_add_epi16(d[1],d[2]);
					d[0] = _mm_add_epi16(d[0],_mm_slli_epi16(d[2],4));
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+2*i_src)),zero);
					d[2] = _mm_sub_epi16(d[1],d[2]);
					d[0] = _mm_add_epi16(d[0],_mm_slli_epi16(d[2],2));
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x-i_src)),zero);
					d[2] = _mm_sub_epi16(d[1],d[2]);
					d[0] = _mm_add_epi16(d[0],_mm_slli_epi16(d[2],1));

					d[0]=_mm_add_epi16(d[0],tmp);
					d[0]=_mm_srai_epi16(d[0],6);
					d[0]=_mm_packus_epi16(d[0],d[0]);

					_mm_storel_epi64((__m128i*)(dst+x),d[0]);
				}
#else
				for( x = 0; x < i_width; x++ )
					dst[x] = lent_clip_uint8(( f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src] + 32 ) >> 6);
#endif
				dst  += i_dst;
				src += i_src;
			}
		}
		else if (fy==3)
		{
			for( y = 0; y < i_height; y++ )
			{
#if 1
				_mm_prefetch(src,_MM_HINT_NTA);
				_mm_prefetch(src+i_src,_MM_HINT_NTA);
				_mm_prefetch(src+i_src*2,_MM_HINT_NTA);
				for( x = 0; x < i_width; x+=8 )
				{
					//dst[x] = lent_clip_uint8(( f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src] + 32 ) >> 6);

					d[1] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x)),zero);
					d[0] = _mm_slli_epi16(d[1],3);
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x-i_src)),zero);
					d[3] = _mm_sub_epi16(d[1],d[2]);
					d[0] = _mm_add_epi16(d[0],_mm_slli_epi16(d[3],1));
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+i_src)),zero);
					d[0] = _mm_add_epi16(d[0],_mm_slli_epi16(_mm_add_epi16(d[1],d[2]),5));
					d[3] = _mm_sub_epi16(d[3],d[2]);
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+2*i_src)),zero);
					d[1] = _mm_sub_epi16(d[3],d[2]);
					d[0] = _mm_add_epi16(d[0],_mm_slli_epi16(d[1],2));

					d[0]=_mm_add_epi16(d[0],tmp);
					d[0]=_mm_srai_epi16(d[0],6);
					d[0]=_mm_packus_epi16(d[0],d[0]);

					_mm_storel_epi64((__m128i*)(dst+x),d[0]);
				}
#else
				for( x = 0; x < i_width; x++ )
					dst[x] = lent_clip_uint8(( f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src] + 32 ) >> 6);
#endif
				dst  += i_dst;
				src += i_src;
			}
		}
		else if (fy==5)
		{
			for( y = 0; y < i_height; y++ )
			{
#if 1
				_mm_prefetch(src,_MM_HINT_NTA);
				_mm_prefetch(src+i_src,_MM_HINT_NTA);
				_mm_prefetch(src+i_src*2,_MM_HINT_NTA);
				for( x = 0; x < i_width; x+=8 )
				{
					//dst[x] = lent_clip_uint8(( f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src] + 32 ) >> 6);

					d[1] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+i_src)),zero);
					d[0] = _mm_slli_epi16(d[1],3);
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+2*i_src)),zero);
					d[3] = _mm_sub_epi16(d[1],d[2]);
					d[0] = _mm_add_epi16(d[0],_mm_slli_epi16(d[3],1));
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x)),zero);
					d[0] = _mm_add_epi16(d[0],_mm_slli_epi16(_mm_add_epi16(d[1],d[2]),5));
					d[3] = _mm_sub_epi16(d[3],d[2]);
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x-i_src)),zero);
					d[1] = _mm_sub_epi16(d[3],d[2]);
					d[0] = _mm_add_epi16(d[0],_mm_slli_epi16(d[1],2));


					d[0]=_mm_add_epi16(d[0],tmp);
					d[0]=_mm_srai_epi16(d[0],6);
					d[0]=_mm_packus_epi16(d[0],d[0]);

					_mm_storel_epi64((__m128i*)(dst+x),d[0]);
				}
#else
				for( x = 0; x < i_width; x++ )
					dst[x] = lent_clip_uint8(( f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src] + 32 ) >> 6);
#endif
				dst  += i_dst;
				src += i_src;
			}
		}
		else
		{
			for( y = 0; y < i_height; y++ )
			{
#if 1
				_mm_prefetch(src,_MM_HINT_NTA);
				_mm_prefetch(src+i_src,_MM_HINT_NTA);
				_mm_prefetch(src+i_src*2,_MM_HINT_NTA);
				for( x = 0; x < i_width; x+=8 )
				{
					//dst[x] = lent_clip_uint8(( f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src] + 32 ) >> 6);

					d[1] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x)),zero);
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+i_src)),zero);
					d[1] = _mm_add_epi16(d[1],d[2]);
					d[0] = _mm_slli_epi16(d[1],5);

					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x-i_src)),zero);
					d[1] = _mm_sub_epi16(d[1],d[2]);
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+2*i_src)),zero);
					d[1] = _mm_sub_epi16(d[1],d[2]);
					d[0] = _mm_add_epi16(d[0],_mm_slli_epi16(d[1],2));

					d[0]=_mm_add_epi16(d[0],tmp);
					d[0]=_mm_srai_epi16(d[0],6);
					d[0]=_mm_packus_epi16(d[0],d[0]);

					_mm_storel_epi64((__m128i*)(dst+x),d[0]);
				}
#else
				for( x = 0; x < i_width; x++ )
					dst[x] = lent_clip_uint8(( f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src] + 32 ) >> 6);
#endif
				dst  += i_dst;
				src += i_src;
			}
		}
	}
	else
	{
		STACK_ALIGN(16,dctcoef,buf_array,32*35)
		dctcoef *buf = buf_array;

		//tmp = src;
		src -= i_src;
		for( y = -1; y < i_height+2; y++ )
		{
#if 1
			for( x = 0; x < i_width; x+=8 )
			{
				

				d[0]=_mm_loadu_si128((__m128i*)(src+x-1));//8 pixels
				d[1]=_mm_shuffle_epi8(d[0],shuffle);

				d[2]=_mm_srli_si128(d[0],4);
				d[2]=_mm_shuffle_epi8(d[2],shuffle);

				d[1]=_mm_maddubs_epi16(d[1],s8);
				d[2]=_mm_maddubs_epi16(d[2],s8);

				d[1]=_mm_hadd_epi16(d[1],d[2]);

				_mm_store_si128((__m128i*)(buf+x),d[1]);//sse3
			}
#else
			{
				int16_t *f=chroma_frac_coeff[fx];
				for( x = 0; x < i_width; x++ )
				{
					buf[x] = f[0]*src[x-1]  + f[1]*src[x] + f[2]*src[x+1] + f[3]*src[x+2];
				}
			}
#endif
			buf  += 32;
			src  += i_src;
		}
		
		buf = buf_array + 32;
		tmp=_mm_set1_epi32(2048);
		if (fy==1){
			for( y = 0; y < i_height; y++ )
			{
#if 1
				for( x = 0; x < i_width; x+=4 )
				{
					//dst[x] = lent_clip_uint8(( f[0]*buf[x-32]  + f[1]*buf[x] + f[2]*buf[x+32] + f[3]*buf[x+2*32] + ((1<<11)) ) >> 12);

					d[1] = _mm_loadl_epi64((__m128i*)(buf+x));
					d[1] = _mm_unpacklo_epi16(d[1],_mm_cmplt_epi16(d[1],zero));
					d[0] = _mm_add_epi32(_mm_slli_epi32(d[1],5),_mm_slli_epi32(d[1],4));
					d[2] = _mm_loadl_epi64((__m128i*)(buf+x+32));
					d[2] = _mm_unpacklo_epi16(d[2],_mm_cmplt_epi16(d[2],zero));
					d[1] = _mm_add_epi32(d[1],d[2]);
					d[0] = _mm_add_epi32(d[0],_mm_slli_epi32(d[1],3));
					d[2] = _mm_loadl_epi64((__m128i*)(buf+x-32));
					d[2] = _mm_unpacklo_epi16(d[2],_mm_cmplt_epi16(d[2],zero));
					d[1] = _mm_sub_epi32(d[1],d[2]);
					d[2] = _mm_loadl_epi64((__m128i*)(buf+x+64));
					d[2] = _mm_unpacklo_epi16(d[2],_mm_cmplt_epi16(d[2],zero));
					d[1] = _mm_sub_epi32(d[1],d[2]);
					d[0] = _mm_add_epi32(d[0],_mm_slli_epi32(d[1],1));

					d[0]=_mm_add_epi32(d[0],tmp);
					d[0]=_mm_srai_epi32(d[0],12);

					d[1]=_mm_cmpgt_epi32(d[0],zero);
					d[0]=_mm_and_si128(d[0],d[1]);

					d[1]=_mm_cmpgt_epi32(d[0],max8bit);
					d[0]=_mm_or_si128(d[0],d[1]);
					d[0]=_mm_and_si128(d[0],max8bit);

					d[0]=_mm_packs_epi32(d[0],d[0]);
					d[0]=_mm_packus_epi16(d[0],d[0]);

					*((int*)(dst+x))=_mm_cvtsi128_si32(d[0]);
				}
#else
				for( x = 0; x < i_width; x++ )
					dst[x] = lent_clip_uint8(( f[0]*buf[x-32]  + f[1]*buf[x] + f[2]*buf[x+32] + f[3]*buf[x+2*32] + ((1<<11)) ) >> 12);
#endif
				dst  += i_dst;
				buf  += 32;
			}
		}
		else if (fy==7){
			for( y = 0; y < i_height; y++ )
			{
#if 1
				for( x = 0; x < i_width; x+=4 )
				{
					//dst[x] = lent_clip_uint8(( f[0]*buf[x-32]  + f[1]*buf[x] + f[2]*buf[x+32] + f[3]*buf[x+2*32] + ((1<<11)) ) >> 12);

					d[1] = _mm_loadl_epi64((__m128i*)(buf+x+32));
					d[1] = _mm_unpacklo_epi16(d[1],_mm_cmplt_epi16(d[1],zero));
					d[0] = _mm_add_epi32(_mm_slli_epi32(d[1],5),_mm_slli_epi32(d[1],4));
					d[2] = _mm_loadl_epi64((__m128i*)(buf+x));
					d[2] = _mm_unpacklo_epi16(d[2],_mm_cmplt_epi16(d[2],zero));
					d[1] = _mm_add_epi32(d[1],d[2]);
					d[0] = _mm_add_epi32(d[0],_mm_slli_epi32(d[1],3));
					d[2] = _mm_loadl_epi64((__m128i*)(buf+x+64));
					d[2] = _mm_unpacklo_epi16(d[2],_mm_cmplt_epi16(d[2],zero));
					d[1] = _mm_sub_epi32(d[1],d[2]);
					d[2] = _mm_loadl_epi64((__m128i*)(buf+x-32));
					d[2] = _mm_unpacklo_epi16(d[2],_mm_cmplt_epi16(d[2],zero));
					d[1] = _mm_sub_epi32(d[1],d[2]);
					d[0] = _mm_add_epi32(d[0],_mm_slli_epi32(d[1],1));

					d[0]=_mm_add_epi32(d[0],tmp);
					d[0]=_mm_srai_epi32(d[0],12);

					d[1]=_mm_cmpgt_epi32(d[0],zero);
					d[0]=_mm_and_si128(d[0],d[1]);

					d[1]=_mm_cmpgt_epi32(d[0],max8bit);
					d[0]=_mm_or_si128(d[0],d[1]);
					d[0]=_mm_and_si128(d[0],max8bit);

					d[0]=_mm_packs_epi32(d[0],d[0]);
					d[0]=_mm_packus_epi16(d[0],d[0]);

					*((int*)(dst+x))=_mm_cvtsi128_si32(d[0]);
				}
#else
				for( x = 0; x < i_width; x++ )
					dst[x] = lent_clip_uint8(( f[0]*buf[x-32]  + f[1]*buf[x] + f[2]*buf[x+32] + f[3]*buf[x+2*32] + ((1<<11)) ) >> 12);
#endif
				dst  += i_dst;
				buf  += 32;
			}
		}
		else if (fy==2){
			for( y = 0; y < i_height; y++ )
			{
#if 1
				for( x = 0; x < i_width; x+=4 )
				{
					//dst[x] = lent_clip_uint8(( f[0]*buf[x-32]  + f[1]*buf[x] + f[2]*buf[x+32] + f[3]*buf[x+2*32] + ((1<<11)) ) >> 12);

					d[1] = _mm_loadl_epi64((__m128i*)(buf+x));
					d[1] = _mm_unpacklo_epi16(d[1],_mm_cmplt_epi16(d[1],zero));
					d[0] = _mm_slli_epi32(d[1],5);
					d[2] = _mm_loadl_epi64((__m128i*)(buf+x+32));
					d[2] = _mm_unpacklo_epi16(d[2],_mm_cmplt_epi16(d[2],zero));
					d[2] = _mm_add_epi32(d[1],d[2]);
					d[0] = _mm_add_epi32(d[0],_mm_slli_epi32(d[2],4));
					d[2] = _mm_loadl_epi64((__m128i*)(buf+x-32));
					d[2] = _mm_unpacklo_epi16(d[2],_mm_cmplt_epi16(d[2],zero));
					d[2] = _mm_sub_epi32(d[1],d[2]);
					d[0] = _mm_add_epi32(d[0],_mm_slli_epi32(d[2],2));
					d[2] = _mm_loadl_epi64((__m128i*)(buf+x+64));
					d[2] = _mm_unpacklo_epi16(d[2],_mm_cmplt_epi16(d[2],zero));
					d[2] = _mm_sub_epi32(d[1],d[2]);
					d[0] = _mm_add_epi32(d[0],_mm_slli_epi32(d[2],1));

					d[0]=_mm_add_epi32(d[0],tmp);
					d[0]=_mm_srai_epi32(d[0],12);

					d[1]=_mm_cmpgt_epi32(d[0],zero);
					d[0]=_mm_and_si128(d[0],d[1]);

					d[1]=_mm_cmpgt_epi32(d[0],max8bit);
					d[0]=_mm_or_si128(d[0],d[1]);
					d[0]=_mm_and_si128(d[0],max8bit);

					d[0]=_mm_packs_epi32(d[0],d[0]);
					d[0]=_mm_packus_epi16(d[0],d[0]);

					*((int*)(dst+x))=_mm_cvtsi128_si32(d[0]);
				}
#else
				for( x = 0; x < i_width; x++ )
					dst[x] = lent_clip_uint8(( f[0]*buf[x-32]  + f[1]*buf[x] + f[2]*buf[x+32] + f[3]*buf[x+2*32] + ((1<<11)) ) >> 12);
#endif
				dst  += i_dst;
				buf  += 32;
			}
		}
		else if (fy==6){
			for( y = 0; y < i_height; y++ )
			{
#if 1
				for( x = 0; x < i_width; x+=4 )
				{
					//dst[x] = lent_clip_uint8(( f[0]*buf[x-32]  + f[1]*buf[x] + f[2]*buf[x+32] + f[3]*buf[x+2*32] + ((1<<11)) ) >> 12);

					d[1] = _mm_loadl_epi64((__m128i*)(buf+x+32));
					d[1] = _mm_unpacklo_epi16(d[1],_mm_cmplt_epi16(d[1],zero));
					d[0] = _mm_slli_epi32(d[1],5);
					d[2] = _mm_loadl_epi64((__m128i*)(buf+x));
					d[2] = _mm_unpacklo_epi16(d[2],_mm_cmplt_epi16(d[2],zero));
					d[2] = _mm_add_epi32(d[1],d[2]);
					d[0] = _mm_add_epi32(d[0],_mm_slli_epi32(d[2],4));
					d[2] = _mm_loadl_epi64((__m128i*)(buf+x+64));
					d[2] = _mm_unpacklo_epi16(d[2],_mm_cmplt_epi16(d[2],zero));
					d[2] = _mm_sub_epi32(d[1],d[2]);
					d[0] = _mm_add_epi32(d[0],_mm_slli_epi32(d[2],2));
					d[2] = _mm_loadl_epi64((__m128i*)(buf+x-32));
					d[2] = _mm_unpacklo_epi16(d[2],_mm_cmplt_epi16(d[2],zero));
					d[2] = _mm_sub_epi32(d[1],d[2]);
					d[0] = _mm_add_epi32(d[0],_mm_slli_epi32(d[2],1));

					d[0]=_mm_add_epi32(d[0],tmp);
					d[0]=_mm_srai_epi32(d[0],12);

					d[1]=_mm_cmpgt_epi32(d[0],zero);
					d[0]=_mm_and_si128(d[0],d[1]);

					d[1]=_mm_cmpgt_epi32(d[0],max8bit);
					d[0]=_mm_or_si128(d[0],d[1]);
					d[0]=_mm_and_si128(d[0],max8bit);

					d[0]=_mm_packs_epi32(d[0],d[0]);
					d[0]=_mm_packus_epi16(d[0],d[0]);

					*((int*)(dst+x))=_mm_cvtsi128_si32(d[0]);
				}
#else
				for( x = 0; x < i_width; x++ )
					dst[x] = lent_clip_uint8(( f[0]*buf[x-32]  + f[1]*buf[x] + f[2]*buf[x+32] + f[3]*buf[x+2*32] + ((1<<11)) ) >> 12);
#endif
				dst  += i_dst;
				buf  += 32;
			}
		}
		else if (fy==3){
			for( y = 0; y < i_height; y++ )
			{
#if 1
				for( x = 0; x < i_width; x+=4 )
				{
					//dst[x] = lent_clip_uint8(( f[0]*buf[x-32]  + f[1]*buf[x] + f[2]*buf[x+32] + f[3]*buf[x+2*32] + ((1<<11)) ) >> 12);

					d[1] = _mm_loadl_epi64((__m128i*)(buf+x));
					d[1] = _mm_unpacklo_epi16(d[1],_mm_cmplt_epi16(d[1],zero));
					d[0] = _mm_slli_epi32(d[1],3);
					d[2] = _mm_loadl_epi64((__m128i*)(buf+x-32));
					d[2] = _mm_unpacklo_epi16(d[2],_mm_cmplt_epi16(d[2],zero));
					d[3] = _mm_sub_epi32(d[1],d[2]);
					d[0] = _mm_add_epi32(d[0],_mm_slli_epi32(d[3],1));
					d[2] = _mm_loadl_epi64((__m128i*)(buf+x+32));
					d[2] = _mm_unpacklo_epi16(d[2],_mm_cmplt_epi16(d[2],zero));
					d[0] = _mm_add_epi32(d[0],_mm_slli_epi32(_mm_add_epi32(d[1],d[2]),5));
					d[3] = _mm_sub_epi32(d[3],d[2]);
					d[2] = _mm_loadl_epi64((__m128i*)(buf+x+64));
					d[2] = _mm_unpacklo_epi16(d[2],_mm_cmplt_epi16(d[2],zero));
					d[1] = _mm_sub_epi32(d[3],d[2]);
					d[0] = _mm_add_epi32(d[0],_mm_slli_epi32(d[1],2));

					d[0]=_mm_add_epi32(d[0],tmp);
					d[0]=_mm_srai_epi32(d[0],12);

					d[1]=_mm_cmpgt_epi32(d[0],zero);
					d[0]=_mm_and_si128(d[0],d[1]);

					d[1]=_mm_cmpgt_epi32(d[0],max8bit);
					d[0]=_mm_or_si128(d[0],d[1]);
					d[0]=_mm_and_si128(d[0],max8bit);

					d[0]=_mm_packs_epi32(d[0],d[0]);
					d[0]=_mm_packus_epi16(d[0],d[0]);

					*((int*)(dst+x))=_mm_cvtsi128_si32(d[0]);
				}
#else
				for( x = 0; x < i_width; x++ )
					dst[x] = lent_clip_uint8(( f[0]*buf[x-32]  + f[1]*buf[x] + f[2]*buf[x+32] + f[3]*buf[x+2*32] + ((1<<11)) ) >> 12);
#endif
				dst  += i_dst;
				buf  += 32;
			}
		}
		else if (fy==5){
			for( y = 0; y < i_height; y++ )
			{
#if 1
				for( x = 0; x < i_width; x+=4 )
				{
					//dst[x] = lent_clip_uint8(( f[0]*buf[x-32]  + f[1]*buf[x] + f[2]*buf[x+32] + f[3]*buf[x+2*32] + ((1<<11)) ) >> 12);

					d[1] = _mm_loadl_epi64((__m128i*)(buf+x+32));
					d[1] = _mm_unpacklo_epi16(d[1],_mm_cmplt_epi16(d[1],zero));
					d[0] = _mm_slli_epi32(d[1],3);
					d[2] = _mm_loadl_epi64((__m128i*)(buf+x+64));
					d[2] = _mm_unpacklo_epi16(d[2],_mm_cmplt_epi16(d[2],zero));
					d[3] = _mm_sub_epi32(d[1],d[2]);
					d[0] = _mm_add_epi32(d[0],_mm_slli_epi32(d[3],1));
					d[2] = _mm_loadl_epi64((__m128i*)(buf+x));
					d[2] = _mm_unpacklo_epi16(d[2],_mm_cmplt_epi16(d[2],zero));
					d[0] = _mm_add_epi32(d[0],_mm_slli_epi32(_mm_add_epi32(d[1],d[2]),5));
					d[3] = _mm_sub_epi32(d[3],d[2]);
					d[2] = _mm_loadl_epi64((__m128i*)(buf+x-32));
					d[2] = _mm_unpacklo_epi16(d[2],_mm_cmplt_epi16(d[2],zero));
					d[1] = _mm_sub_epi32(d[3],d[2]);
					d[0] = _mm_add_epi32(d[0],_mm_slli_epi32(d[1],2));

					d[0]=_mm_add_epi32(d[0],tmp);
					d[0]=_mm_srai_epi32(d[0],12);

					d[1]=_mm_cmpgt_epi32(d[0],zero);
					d[0]=_mm_and_si128(d[0],d[1]);

					d[1]=_mm_cmpgt_epi32(d[0],max8bit);
					d[0]=_mm_or_si128(d[0],d[1]);
					d[0]=_mm_and_si128(d[0],max8bit);

					d[0]=_mm_packs_epi32(d[0],d[0]);
					d[0]=_mm_packus_epi16(d[0],d[0]);

					*((int*)(dst+x))=_mm_cvtsi128_si32(d[0]);
				}
#else
				for( x = 0; x < i_width; x++ )
					dst[x] = lent_clip_uint8(( f[0]*buf[x-32]  + f[1]*buf[x] + f[2]*buf[x+32] + f[3]*buf[x+2*32] + ((1<<11)) ) >> 12);
#endif
				dst  += i_dst;
				buf  += 32;
			}
		}
		else{
			for( y = 0; y < i_height; y++ )
			{
#if 1
				for( x = 0; x < i_width; x+=4 )
				{
					//dst[x] = lent_clip_uint8(( f[0]*buf[x-32]  + f[1]*buf[x] + f[2]*buf[x+32] + f[3]*buf[x+2*32] + ((1<<11)) ) >> 12);

					d[1] = _mm_loadl_epi64((__m128i*)(buf+x));
					d[1] = _mm_unpacklo_epi16(d[1],_mm_cmplt_epi16(d[1],zero));
					d[2] = _mm_loadl_epi64((__m128i*)(buf+x+32));
					d[2] = _mm_unpacklo_epi16(d[2],_mm_cmplt_epi16(d[2],zero));
					d[1] = _mm_add_epi32(d[1],d[2]);
					d[0] = _mm_slli_epi32(d[1],5);

					d[2] = _mm_loadl_epi64((__m128i*)(buf+x-32));
					d[2] = _mm_unpacklo_epi16(d[2],_mm_cmplt_epi16(d[2],zero));
					d[1] = _mm_sub_epi32(d[1],d[2]);
					d[2] = _mm_loadl_epi64((__m128i*)(buf+x+64));
					d[2] = _mm_unpacklo_epi16(d[2],_mm_cmplt_epi16(d[2],zero));
					d[1] = _mm_sub_epi32(d[1],d[2]);
					d[0] = _mm_add_epi32(d[0],_mm_slli_epi32(d[1],2));

					d[0]=_mm_add_epi32(d[0],tmp);
					d[0]=_mm_srai_epi32(d[0],12);

					d[1]=_mm_cmpgt_epi32(d[0],zero);
					d[0]=_mm_and_si128(d[0],d[1]);

					d[1]=_mm_cmpgt_epi32(d[0],max8bit);
					d[0]=_mm_or_si128(d[0],d[1]);
					d[0]=_mm_and_si128(d[0],max8bit);

					d[0]=_mm_packs_epi32(d[0],d[0]);
					d[0]=_mm_packus_epi16(d[0],d[0]);

					*((int*)(dst+x))=_mm_cvtsi128_si32(d[0]);
				}
#else
				for( x = 0; x < i_width; x++ )
					dst[x] = lent_clip_uint8(( f[0]*buf[x-32]  + f[1]*buf[x] + f[2]*buf[x+32] + f[3]*buf[x+2*32] + ((1<<11)) ) >> 12);
#endif
				dst  += i_dst;
				buf  += 32;
			}
		}
	}
	////_mm_empty();
}

/*
void get_ref_chroma_sse4( int16_t *dst, int i_dst, pixel *src, int i_src,
						   int mvx, int mvy, int i_width, int i_height )
{
	int x, y;
	int fx = mvx&0x7;
	int fy = mvy&0x7;

	__m128i s8,sy[4],d[4];
	__m128i shuffle={
		0,1,2,3,
		1,2,3,4,
		2,3,4,5,
		3,4,5,6
	};
	////_mm_empty();
	//s=frac_coeff_ch[fx];
	s8=frac_coeff_ch_8bit[fx];

	src += (mvy >> 3) * i_src + (mvx >> 3);

	if( fy == 0 )
	{

#if 1
		if( fx == 0 )
		{
			for( y = 0; y < i_height; y++ )
			{
				for( x = 0; x < i_width; x+=8 )
				{
					//dst[x] = (src[x]<<6);

					d[0]=_mm_loadl_epi64((__m128i*)(src+x));
					d[0]=_mm_unpacklo_epi8(d[0],zero);
					d[0]=_mm_slli_epi16(d[0],6);

					_mm_store_si128((__m128i*)(dst+x),d[0]);

				}
				dst  += i_dst;
				src  += i_src;
			}
		}
		else
#endif
		for( y = 0; y < i_height; y++ )
		{
			for( x = 0; x < i_width; x+=8 )
			{
				//dst[x] = f[0]*src[x-1]  + f[1]*src[x] + f[2]*src[x+1] + f[3]*src[x+2];
				
				//d[1]=_mm_loadl_epi64((__m128i*)(src+x-1));//8 pixels

				d[0]=_mm_loadu_si128((__m128i*)(src+x-1));//8 pixels
				d[1]=_mm_shuffle_epi8(d[0],shuffle);

				d[2]=_mm_srli_si128(d[0],4);
				d[2]=_mm_shuffle_epi8(d[2],shuffle);

				d[1]=_mm_maddubs_epi16(d[1],s8);
				d[2]=_mm_maddubs_epi16(d[2],s8);

				d[1]=_mm_hadd_epi16(d[1],d[2]);

				//_mm_storel_epi64((__m128i*)(dst+x),d[0]);//sse3
				_mm_store_si128((__m128i*)(dst+x),d[1]);//sse3
			}
			dst  += i_dst;
			src += i_src;
		}
	}
	else if( fx == 0 )
	{
		sy[0]=_mm_set1_epi16(chroma_frac_coeff[fy][0]);
		sy[1]=_mm_set1_epi16(chroma_frac_coeff[fy][1]);
		sy[2]=_mm_set1_epi16(chroma_frac_coeff[fy][2]);
		sy[3]=_mm_set1_epi16(chroma_frac_coeff[fy][3]);
		for( y = 0; y < i_height; y++ )
		{
#if 1
			for( x = 0; x < i_width; x+=8 )
			{
				//dst[x] = f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src];

				d[0]=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x-1*i_src)),zero);
				d[0]=_mm_mullo_epi16(d[0],sy[0]);

				d[1]=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x-0*i_src)),zero);
				d[1]=_mm_mullo_epi16(d[1],sy[1]);
				d[0]=_mm_add_epi16(d[0],d[1]);

				d[1]=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+1*i_src)),zero);
				d[1]=_mm_mullo_epi16(d[1],sy[2]);
				d[0]=_mm_add_epi16(d[0],d[1]);

				d[1]=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+2*i_src)),zero);
				d[1]=_mm_mullo_epi16(d[1],sy[3]);
				d[0]=_mm_add_epi16(d[0],d[1]);

				_mm_store_si128((__m128i*)(dst+x),d[0]);
			}
#else
			{
				int16_t *f=chroma_frac_coeff[fy];
				for( x = 0; x < i_width; x++ )
				{
					dst[x] = f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src];
				}
			}
#endif
			dst  += i_dst;
			src  += i_src;
		}
	}
	else
	{
		STACK_ALIGN(16,dctcoef,buf_array,32*35)
		dctcoef *buf = buf_array;

		//tmp = src;
		src -= i_src;
		for( y = -1; y < i_height+2; y++ )
		{
			_mm_prefetch(src+i_src-1,_MM_HINT_NTA);
			for( x = 0; x < i_width; x+=8 )
			{
				//buf[x] = f[0]*src[x-1]  + f[1]*src[x] + f[2]*src[x+1] + f[3]*src[x+2];
				
				d[0]=_mm_loadu_si128((__m128i*)(src+x-1));//8 pixels
				d[1]=_mm_shuffle_epi8(d[0],shuffle);

				d[2]=_mm_srli_si128(d[0],4);
				d[2]=_mm_shuffle_epi8(d[2],shuffle);

				d[1]=_mm_maddubs_epi16(d[1],s8);
				d[2]=_mm_maddubs_epi16(d[2],s8);

				d[1]=_mm_hadd_epi16(d[1],d[2]);

				_mm_store_si128((__m128i*)(buf+x),d[1]);//sse3
			}

			buf  += 32;
			src  += i_src;
		}

		buf = buf_array + 32;
		sy[0]=_mm_set1_epi32(chroma_frac_coeff[fy][0]);
		sy[1]=_mm_set1_epi32(chroma_frac_coeff[fy][1]);
		sy[2]=_mm_set1_epi32(chroma_frac_coeff[fy][2]);
		sy[3]=_mm_set1_epi32(chroma_frac_coeff[fy][3]);
		_mm_prefetch(buf_array-32,_MM_HINT_NTA);
		for( y = 0; y < i_height; y++ )
		{
#if 1
			for( x = 0; x < i_width; x+=4 )
			{
				//dst[x] = (f[0]*buf[x-32]  + f[1]*buf[x] + f[2]*buf[x+32] + f[3]*buf[x+2*32]) >> 6;

				d[0]=_mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(buf+x-32)));
				d[1]=_mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(buf+x)));
				d[2]=_mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(buf+x+32)));
				d[3]=_mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(buf+x+64)));

				d[0]=_mm_mullo_epi32(d[0],sy[0]);

				d[1]=_mm_mullo_epi32(d[1],sy[1]);
				d[0]=_mm_add_epi32(d[0],d[1]);

				d[2]=_mm_mullo_epi32(d[2],sy[2]);
				d[0]=_mm_add_epi32(d[0],d[2]);

				d[3]=_mm_mullo_epi32(d[3],sy[3]);
				d[0]=_mm_add_epi32(d[0],d[3]);
				
				d[0]=_mm_srai_epi32(d[0],6);
				d[0]=_mm_packs_epi32(d[0],d[0]);

				_mm_storel_epi64((__m128i*)(dst+x),d[0]);//sse3
			}
#else
			{
				int16_t *f=chroma_frac_coeff[fy];
				for( x = 0; x < i_width; x++ )
				{
					dst[x] = (f[0]*buf[x-32]  + f[1]*buf[x] + f[2]*buf[x+32] + f[3]*buf[x+2*32]) >> 6;
				}
			}
#endif
			dst  += i_dst;
			buf  += 32;
		}
	}
	////_mm_empty();
}
*/
void get_ref_chroma_sse4_shushuang( int16_t *dst, int i_dst, pixel *src, int i_src,
						   int mvx, int mvy, int i_width, int i_height )
{
	int x, y;
	int fx = mvx&0x7;
	int fy = mvy&0x7;

	__m128i s8,d[4];
	__m128i shuffle={
		0,1,2,3,
		1,2,3,4,
		2,3,4,5,
		3,4,5,6
	};
	////_mm_empty();
	//s=frac_coeff_ch[fx];
	s8=frac_coeff_ch_8bit[fx];

	src += (mvy >> 3) * i_src + (mvx >> 3);

	if( fy == 0 )
	{

#if 1
		if( fx == 0 )
		{
			for( y = 0; y < i_height; y++ )
			{
				for( x = 0; x < i_width; x+=8 )
				{
					//dst[x] = (src[x]<<6);

					d[0]=_mm_loadl_epi64((__m128i*)(src+x));
					d[0]=_mm_unpacklo_epi8(d[0],zero);
					d[0]=_mm_slli_epi16(d[0],6);

					_mm_store_si128((__m128i*)(dst+x),d[0]);

				}
				dst  += i_dst;
				src  += i_src;
			}
		}
		else
#endif
		for( y = 0; y < i_height; y++ )
		{
			for( x = 0; x < i_width; x+=8 )
			{
				//dst[x] = f[0]*src[x-1]  + f[1]*src[x] + f[2]*src[x+1] + f[3]*src[x+2];
				
				//d[1]=_mm_loadl_epi64((__m128i*)(src+x-1));//8 pixels

				d[0]=_mm_loadu_si128((__m128i*)(src+x-1));//8 pixels
				d[1]=_mm_shuffle_epi8(d[0],shuffle);

				d[2]=_mm_srli_si128(d[0],4);
				d[2]=_mm_shuffle_epi8(d[2],shuffle);

				d[1]=_mm_maddubs_epi16(d[1],s8);
				d[2]=_mm_maddubs_epi16(d[2],s8);

				d[1]=_mm_hadd_epi16(d[1],d[2]);

				//_mm_storel_epi64((__m128i*)(dst+x),d[0]);//sse3
				_mm_store_si128((__m128i*)(dst+x),d[1]);//sse3
			}
			dst  += i_dst;
			src += i_src;
		}
	}
	else if( fx == 0 )
	{
		_mm_prefetch(src,_MM_HINT_NTA);
		_mm_prefetch(src+i_src,_MM_HINT_NTA);
		if (fy == 1){
			for( y = 0; y < i_height; y++ )
			{
#if 1
				for( x = 0; x < i_width; x+=8 )
				{
					//dst[x] = f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src];
					d[1] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x)),zero);
					d[0] = _mm_add_epi16(_mm_slli_epi16(d[1],5),_mm_slli_epi16(d[1],4));
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+i_src)),zero);
					d[1] = _mm_add_epi16(d[1],d[2]);
					d[0] = _mm_add_epi16(d[0],_mm_slli_epi16(d[1],3));
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x-1*i_src)),zero);
					d[1] = _mm_sub_epi16(d[1],d[2]);
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+2*i_src)),zero);
					d[1] = _mm_sub_epi16(d[1],d[2]);
					d[0] = _mm_add_epi16(d[0],_mm_slli_epi16(d[1],1));

					_mm_store_si128((__m128i*)(dst+x),d[0]);
				}
#else
				{
					int16_t *f=chroma_frac_coeff[fy];
					for( x = 0; x < i_width; x++ )
					{
						dst[x] = f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src];
					}
				}
#endif
				dst  += i_dst;
				src  += i_src;
			}
		}
		else if (fy == 7){
			for( y = 0; y < i_height; y++ )
			{
#if 1
				for( x = 0; x < i_width; x+=8 )
				{
					//dst[x] = f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src];
					d[1] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+1*i_src)),zero);
					d[0] = _mm_add_epi16(_mm_slli_epi16(d[1],5),_mm_slli_epi16(d[1],4));
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x)),zero);
					d[1] = _mm_add_epi16(d[1],d[2]);
					d[0] = _mm_add_epi16(d[0],_mm_slli_epi16(d[1],3));
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+2*i_src)),zero);
					d[1] = _mm_sub_epi16(d[1],d[2]);
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x-1*i_src)),zero);
					d[1] = _mm_sub_epi16(d[1],d[2]);
					d[0] = _mm_add_epi16(d[0],_mm_slli_epi16(d[1],1));

					_mm_store_si128((__m128i*)(dst+x),d[0]);
				}
#else
				{
					int16_t *f=chroma_frac_coeff[fy];
					for( x = 0; x < i_width; x++ )
					{
						dst[x] = f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src];
					}
				}
#endif
				dst  += i_dst;
				src  += i_src;
			}
		}
		else if (fy == 2){
			for( y = 0; y < i_height; y++ )
			{
#if 1
				for( x = 0; x < i_width; x+=8 )
				{
					//dst[x] = f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src];
					d[1] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x)),zero);
					d[0] = _mm_slli_epi16(d[1],5);
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+i_src)),zero);
					d[2] = _mm_add_epi16(d[1],d[2]);
					d[0] = _mm_add_epi16(d[0],_mm_slli_epi16(d[2],4));
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x-i_src)),zero);
					d[2] = _mm_sub_epi16(d[1],d[2]);
					d[0] = _mm_add_epi16(d[0],_mm_slli_epi16(d[2],2));
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+2*i_src)),zero);
					d[2] = _mm_sub_epi16(d[1],d[2]);
					d[0] = _mm_add_epi16(d[0],_mm_slli_epi16(d[2],1));

					_mm_store_si128((__m128i*)(dst+x),d[0]);
				}
#else
				{
					int16_t *f=chroma_frac_coeff[fy];
					for( x = 0; x < i_width; x++ )
					{
						dst[x] = f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src];
					}
				}
#endif
				dst  += i_dst;
				src  += i_src;
			}
		}
		else if (fy == 6){
			for( y = 0; y < i_height; y++ )
			{
#if 1
				for( x = 0; x < i_width; x+=8 )
				{
					//dst[x] = f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src];
					
					d[1] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+i_src)),zero);
					d[0] = _mm_slli_epi16(d[1],5);
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x)),zero);
					d[2] = _mm_add_epi16(d[1],d[2]);
					d[0] = _mm_add_epi16(d[0],_mm_slli_epi16(d[2],4));
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+2*i_src)),zero);
					d[2] = _mm_sub_epi16(d[1],d[2]);
					d[0] = _mm_add_epi16(d[0],_mm_slli_epi16(d[2],2));
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x-i_src)),zero);
					d[2] = _mm_sub_epi16(d[1],d[2]);
					d[0] = _mm_add_epi16(d[0],_mm_slli_epi16(d[2],1));

					_mm_store_si128((__m128i*)(dst+x),d[0]);
				}
#else
				{
					int16_t *f=chroma_frac_coeff[fy];
					for( x = 0; x < i_width; x++ )
					{
						dst[x] = f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src];
					}
				}
#endif
				dst  += i_dst;
				src  += i_src;
			}
		}
		else if (fy == 3){
			for( y = 0; y < i_height; y++ )
			{
#if 1
				for( x = 0; x < i_width; x+=8 )
				{
					//dst[x] = f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src];
					d[1] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x)),zero);
					d[0] = _mm_slli_epi16(d[1],3);
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x-i_src)),zero);
					d[3] = _mm_sub_epi16(d[1],d[2]);
					d[0] = _mm_add_epi16(d[0],_mm_slli_epi16(d[3],1));
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+i_src)),zero);
					d[0] = _mm_add_epi16(d[0],_mm_slli_epi16(_mm_add_epi16(d[1],d[2]),5));
					d[3] = _mm_sub_epi16(d[3],d[2]);
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+2*i_src)),zero);
					d[1] = _mm_sub_epi16(d[3],d[2]);
					d[0] = _mm_add_epi16(d[0],_mm_slli_epi16(d[1],2));

					_mm_store_si128((__m128i*)(dst+x),d[0]);
				}
#else
				{
					int16_t *f=chroma_frac_coeff[fy];
					for( x = 0; x < i_width; x++ )
					{
						dst[x] = f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src];
					}
				}
#endif
				dst  += i_dst;
				src  += i_src;
			}
		}
		else if (fy == 5){
			for( y = 0; y < i_height; y++ )
			{
#if 1
				for( x = 0; x < i_width; x+=8 )
				{
					//dst[x] = f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src];
					
					d[1] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+i_src)),zero);
					d[0] = _mm_slli_epi16(d[1],3);
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+2*i_src)),zero);
					d[3] = _mm_sub_epi16(d[1],d[2]);
					d[0] = _mm_add_epi16(d[0],_mm_slli_epi16(d[3],1));
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x)),zero);
					d[0] = _mm_add_epi16(d[0],_mm_slli_epi16(_mm_add_epi16(d[1],d[2]),5));
					d[3] = _mm_sub_epi16(d[3],d[2]);
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x-i_src)),zero);
					d[1] = _mm_sub_epi16(d[3],d[2]);
					d[0] = _mm_add_epi16(d[0],_mm_slli_epi16(d[1],2));

					_mm_store_si128((__m128i*)(dst+x),d[0]);
				}
#else
				{
					int16_t *f=chroma_frac_coeff[fy];
					for( x = 0; x < i_width; x++ )
					{
						dst[x] = f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src];
					}
				}
#endif
				dst  += i_dst;
				src  += i_src;
			}
		}
		else
		{
			for( y = 0; y < i_height; y++ )
			{
#if 1
				for( x = 0; x < i_width; x+=8 )
				{
					//dst[x] = f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src];
					d[1] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x)),zero);
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+i_src)),zero);
					d[1] = _mm_add_epi16(d[1],d[2]);
					d[0] = _mm_slli_epi16(d[1],5);

					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x-i_src)),zero);
					d[1] = _mm_sub_epi16(d[1],d[2]);
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+2*i_src)),zero);
					d[1] = _mm_sub_epi16(d[1],d[2]);
					d[0] = _mm_add_epi16(d[0],_mm_slli_epi16(d[1],2));

					_mm_store_si128((__m128i*)(dst+x),d[0]);
				}
#else
				{
					int16_t *f=chroma_frac_coeff[fy];
					for( x = 0; x < i_width; x++ )
					{
						dst[x] = f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src];
					}
				}
#endif
				dst  += i_dst;
				src  += i_src;
			}
		}
	}
	else
	{
		STACK_ALIGN(16,dctcoef,buf_array,32*35)
		dctcoef *buf = buf_array;

		//tmp = src;
		src -= i_src;
		for( y = -1; y < i_height+2; y++ )
		{
			_mm_prefetch(src+i_src-1,_MM_HINT_NTA);
			for( x = 0; x < i_width; x+=8 )
			{
				//buf[x] = f[0]*src[x-1]  + f[1]*src[x] + f[2]*src[x+1] + f[3]*src[x+2];
				
				d[0]=_mm_loadu_si128((__m128i*)(src+x-1));//8 pixels
				d[1]=_mm_shuffle_epi8(d[0],shuffle);

				d[2]=_mm_srli_si128(d[0],4);
				d[2]=_mm_shuffle_epi8(d[2],shuffle);

				d[1]=_mm_maddubs_epi16(d[1],s8);
				d[2]=_mm_maddubs_epi16(d[2],s8);

				d[1]=_mm_hadd_epi16(d[1],d[2]);

				_mm_store_si128((__m128i*)(buf+x),d[1]);//sse3
			}

			buf  += 32;
			src  += i_src;
		}

		buf = buf_array + 32;
		
		_mm_prefetch(buf_array-32,_MM_HINT_NTA);
		
		if (fy == 1){
			for( y = 0; y < i_height; y++ )
			{
#if 1
				for( x = 0; x < i_width; x+=4 )
				{
					//dst[x] = f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src];
					d[1] = _mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(buf+x)));
					d[0] = _mm_add_epi32(_mm_slli_epi32(d[1],5),_mm_slli_epi32(d[1],4));
					d[2] = _mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(buf+x+32)));
					d[1] = _mm_add_epi32(d[1],d[2]);
					d[0] = _mm_add_epi32(d[0],_mm_slli_epi32(d[1],3));
					d[2] = _mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(buf+x-32)));
					d[1] = _mm_sub_epi32(d[1],d[2]);
					d[2] = _mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(buf+x+64)));
					d[1] = _mm_sub_epi32(d[1],d[2]);
					d[0] = _mm_add_epi32(d[0],_mm_slli_epi32(d[1],1));

					d[0]=_mm_srai_epi32(d[0],6);
					d[0]=_mm_packs_epi32(d[0],d[0]);

					_mm_storel_epi64((__m128i*)(dst+x),d[0]);//sse3
				}
#else
				{
					int16_t *f=chroma_frac_coeff[fy];
					for( x = 0; x < i_width; x++ )
					{
						dst[x] = (f[0]*buf[x-32]  + f[1]*buf[x] + f[2]*buf[x+32] + f[3]*buf[x+2*32]) >> 6;
					}
				}
#endif
				dst  += i_dst;
				buf  += 32;
			}
		}
		else if (fy == 7){
			for( y = 0; y < i_height; y++ )
			{
#if 1
				for( x = 0; x < i_width; x+=4 )
				{
					//dst[x] = f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src];
					d[1] = _mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(buf+x+32)));
					d[0] = _mm_add_epi32(_mm_slli_epi32(d[1],5),_mm_slli_epi32(d[1],4));
					d[2] = _mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(buf+x)));
					d[1] = _mm_add_epi32(d[1],d[2]);
					d[0] = _mm_add_epi32(d[0],_mm_slli_epi32(d[1],3));
					d[2] = _mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(buf+x+64)));
					d[1] = _mm_sub_epi32(d[1],d[2]);
					d[2] = _mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(buf+x-32)));
					d[1] = _mm_sub_epi32(d[1],d[2]);
					d[0] = _mm_add_epi32(d[0],_mm_slli_epi32(d[1],1));

					d[0]=_mm_srai_epi32(d[0],6);
					d[0]=_mm_packs_epi32(d[0],d[0]);

					_mm_storel_epi64((__m128i*)(dst+x),d[0]);//sse3
				}
#else
				{
					int16_t *f=chroma_frac_coeff[fy];
					for( x = 0; x < i_width; x++ )
					{
						dst[x] = (f[0]*buf[x-32]  + f[1]*buf[x] + f[2]*buf[x+32] + f[3]*buf[x+2*32]) >> 6;
					}
				}
#endif
				dst  += i_dst;
				buf  += 32;
			}
		}
		else if (fy == 2){
			for( y = 0; y < i_height; y++ )
			{
#if 1
				for( x = 0; x < i_width; x+=4 )
				{
					//dst[x] = f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src];
					d[1] = _mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(buf+x)));
					d[0] = _mm_slli_epi32(d[1],5);
					d[2] = _mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(buf+x+32)));
					d[2] = _mm_add_epi32(d[1],d[2]);
					d[0] = _mm_add_epi32(d[0],_mm_slli_epi32(d[2],4));
					d[2] = _mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(buf+x-32)));
					d[2] = _mm_sub_epi32(d[1],d[2]);
					d[0] = _mm_add_epi32(d[0],_mm_slli_epi32(d[2],2));
					d[2] = _mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(buf+x+64)));
					d[2] = _mm_sub_epi32(d[1],d[2]);
					d[0] = _mm_add_epi32(d[0],_mm_slli_epi32(d[2],1));

					d[0]=_mm_srai_epi32(d[0],6);
					d[0]=_mm_packs_epi32(d[0],d[0]);

					_mm_storel_epi64((__m128i*)(dst+x),d[0]);//sse3
				}
#else
				{
					int16_t *f=chroma_frac_coeff[fy];
					for( x = 0; x < i_width; x++ )
					{
						dst[x] = (f[0]*buf[x-32]  + f[1]*buf[x] + f[2]*buf[x+32] + f[3]*buf[x+2*32]) >> 6;
					}
				}
#endif
				dst  += i_dst;
				buf  += 32;
			}
		}
		else if (fy == 6){
			for( y = 0; y < i_height; y++ )
			{
#if 1
				for( x = 0; x < i_width; x+=4 )
				{
					//dst[x] = f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src];
					
					d[1] = _mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(buf+x+32)));
					d[0] = _mm_slli_epi32(d[1],5);
					d[2] = _mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(buf+x)));
					d[2] = _mm_add_epi32(d[1],d[2]);
					d[0] = _mm_add_epi32(d[0],_mm_slli_epi32(d[2],4));
					d[2] = _mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(buf+x+64)));
					d[2] = _mm_sub_epi32(d[1],d[2]);
					d[0] = _mm_add_epi32(d[0],_mm_slli_epi32(d[2],2));
					d[2] = _mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(buf+x-32)));
					d[2] = _mm_sub_epi32(d[1],d[2]);
					d[0] = _mm_add_epi32(d[0],_mm_slli_epi32(d[2],1));

					d[0]=_mm_srai_epi32(d[0],6);
					d[0]=_mm_packs_epi32(d[0],d[0]);

					_mm_storel_epi64((__m128i*)(dst+x),d[0]);//sse3
				}
#else
				{
					int16_t *f=chroma_frac_coeff[fy];
					for( x = 0; x < i_width; x++ )
					{
						dst[x] = (f[0]*buf[x-32]  + f[1]*buf[x] + f[2]*buf[x+32] + f[3]*buf[x+2*32]) >> 6;
					}
				}
#endif
				dst  += i_dst;
				buf  += 32;
			}
		}
		else if (fy == 3){
			for( y = 0; y < i_height; y++ )
			{
#if 1
				for( x = 0; x < i_width; x+=4 )
				{
					//dst[x] = f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src];
					d[1] = _mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(buf+x)));
					d[0] = _mm_slli_epi32(d[1],3);
					d[2] = _mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(buf+x-32)));
					d[3] = _mm_sub_epi32(d[1],d[2]);
					d[0] = _mm_add_epi32(d[0],_mm_slli_epi32(d[3],1));
					d[2] = _mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(buf+x+32)));
					d[0] = _mm_add_epi32(d[0],_mm_slli_epi32(_mm_add_epi32(d[1],d[2]),5));
					d[3] = _mm_sub_epi32(d[3],d[2]);
					d[2] = _mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(buf+x+64)));
					d[1] = _mm_sub_epi32(d[3],d[2]);
					d[0] = _mm_add_epi32(d[0],_mm_slli_epi32(d[1],2));

					d[0]=_mm_srai_epi32(d[0],6);
					d[0]=_mm_packs_epi32(d[0],d[0]);

					_mm_storel_epi64((__m128i*)(dst+x),d[0]);//sse3
				}
#else
				{
					int16_t *f=chroma_frac_coeff[fy];
					for( x = 0; x < i_width; x++ )
					{
						dst[x] = (f[0]*buf[x-32]  + f[1]*buf[x] + f[2]*buf[x+32] + f[3]*buf[x+2*32]) >> 6;
					}
				}
#endif
				dst  += i_dst;
				buf  += 32;
			}
		}
		else if (fy == 5){
			for( y = 0; y < i_height; y++ )
			{
#if 1
				for( x = 0; x < i_width; x+=4 )
				{
					//dst[x] = f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src];
					
					
					d[1] = _mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(buf+x+32)));
					d[0] = _mm_slli_epi32(d[1],3);
					d[2] = _mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(buf+x+64)));
					d[3] = _mm_sub_epi32(d[1],d[2]);
					d[0] = _mm_add_epi32(d[0],_mm_slli_epi32(d[3],1));
					d[2] = _mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(buf+x)));
					d[0] = _mm_add_epi32(d[0],_mm_slli_epi32(_mm_add_epi32(d[1],d[2]),5));
					d[3] = _mm_sub_epi32(d[3],d[2]);
					d[2] = _mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(buf+x-32)));
					d[1] = _mm_sub_epi32(d[3],d[2]);
					d[0] = _mm_add_epi32(d[0],_mm_slli_epi32(d[1],2));

					d[0]=_mm_srai_epi32(d[0],6);
					d[0]=_mm_packs_epi32(d[0],d[0]);

					_mm_storel_epi64((__m128i*)(dst+x),d[0]);//sse3
				}
#else
				{
					int16_t *f=chroma_frac_coeff[fy];
					for( x = 0; x < i_width; x++ )
					{
						dst[x] = (f[0]*buf[x-32]  + f[1]*buf[x] + f[2]*buf[x+32] + f[3]*buf[x+2*32]) >> 6;
					}
				}
#endif
				dst  += i_dst;
				buf  += 32;
			}
		}
		else{
			for( y = 0; y < i_height; y++ )
			{
#if 1
				for( x = 0; x < i_width; x+=4 )
				{
					//dst[x] = f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src];
					d[1] = _mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(buf+x)));
					d[2] = _mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(buf+x+32)));
					d[1] = _mm_add_epi32(d[1],d[2]);
					d[0] = _mm_slli_epi32(d[1],5);

					d[2] = _mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(buf+x-32)));
					d[1] = _mm_sub_epi32(d[1],d[2]);
					d[2] = _mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(buf+x+64)));
					d[1] = _mm_sub_epi32(d[1],d[2]);
					d[0] = _mm_add_epi32(d[0],_mm_slli_epi32(d[1],2));

					d[0]=_mm_srai_epi32(d[0],6);
					d[0]=_mm_packs_epi32(d[0],d[0]);

					_mm_storel_epi64((__m128i*)(dst+x),d[0]);//sse3
				}
#else
				{
					int16_t *f=chroma_frac_coeff[fy];
					for( x = 0; x < i_width; x++ )
					{
						dst[x] = (f[0]*buf[x-32]  + f[1]*buf[x] + f[2]*buf[x+32] + f[3]*buf[x+2*32]) >> 6;
					}
				}
#endif
				dst  += i_dst;
				buf  += 32;
			}
		}
	}
	////_mm_empty();
}

/*
void get_ref_chroma_ssse3( int16_t *dst, int i_dst, pixel *src, int i_src,
						   int mvx, int mvy, int i_width, int i_height )
{
	int x, y;
	int fx = mvx&0x7;
	int fy = mvy&0x7;

	__m128i s8,sy[4],d[4];
	__m128i shuffle={
		0,1,2,3,
		1,2,3,4,
		2,3,4,5,
		3,4,5,6
	};
	////_mm_empty();
	//s=frac_coeff_ch[fx];
	s8=frac_coeff_ch_8bit[fx];

	src += (mvy >> 3) * i_src + (mvx >> 3);

	if( fy == 0 )
	{

#if 1
		if( fx == 0 )
		{
			for( y = 0; y < i_height; y++ )
			{
				for( x = 0; x < i_width; x+=8 )
				{
					//dst[x] = (src[x]<<6);

					d[0]=_mm_loadl_epi64((__m128i*)(src+x));
					d[0]=_mm_unpacklo_epi8(d[0],zero);
					d[0]=_mm_slli_epi16(d[0],6);

					_mm_store_si128((__m128i*)(dst+x),d[0]);

				}
				dst  += i_dst;
				src  += i_src;
			}
		}
		else
#endif
		for( y = 0; y < i_height; y++ )
		{
#if 1
			for( x = 0; x < i_width; x+=8 )
			{
				//dst[x] = f[0]*src[x-1]  + f[1]*src[x] + f[2]*src[x+1] + f[3]*src[x+2];
				
				//d[1]=_mm_loadl_epi64((__m128i*)(src+x-1));//8 pixels

				d[0]=_mm_loadu_si128((__m128i*)(src+x-1));//8 pixels
				d[1]=_mm_shuffle_epi8(d[0],shuffle);

				d[2]=_mm_srli_si128(d[0],4);
				d[2]=_mm_shuffle_epi8(d[2],shuffle);

				d[1]=_mm_maddubs_epi16(d[1],s8);
				d[2]=_mm_maddubs_epi16(d[2],s8);

				d[1]=_mm_hadd_epi16(d[1],d[2]);

				//_mm_storel_epi64((__m128i*)(dst+x),d[0]);//sse3
				_mm_store_si128((__m128i*)(dst+x),d[1]);//sse3
				
			}
#else
			{
				int16_t *f=chroma_frac_coeff[fx];
				for( x = 0; x < i_width; x++ )
				{
					dst[x] = f[0]*src[x-1]  + f[1]*src[x] + f[2]*src[x+1] + f[3]*src[x+2];
				}
			}
#endif
			dst  += i_dst;
			src += i_src;
		}
	}
	else if( fx == 0 )
	{
		sy[0]=_mm_set1_epi16(chroma_frac_coeff[fy][0]);
		sy[1]=_mm_set1_epi16(chroma_frac_coeff[fy][1]);
		sy[2]=_mm_set1_epi16(chroma_frac_coeff[fy][2]);
		sy[3]=_mm_set1_epi16(chroma_frac_coeff[fy][3]);
		for( y = 0; y < i_height; y++ )
		{
#if 1
			for( x = 0; x < i_width; x+=8 )
			{
				//dst[x] = f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src];

				d[0]=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x-1*i_src)),zero);
				d[0]=_mm_mullo_epi16(d[0],sy[0]);

				d[1]=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x-0*i_src)),zero);
				d[1]=_mm_mullo_epi16(d[1],sy[1]);
				d[0]=_mm_add_epi16(d[0],d[1]);

				d[1]=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+1*i_src)),zero);
				d[1]=_mm_mullo_epi16(d[1],sy[2]);
				d[0]=_mm_add_epi16(d[0],d[1]);

				d[1]=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+2*i_src)),zero);
				d[1]=_mm_mullo_epi16(d[1],sy[3]);
				d[0]=_mm_add_epi16(d[0],d[1]);

				_mm_store_si128((__m128i*)(dst+x),d[0]);
			}
#else
			{
				int16_t *f=chroma_frac_coeff[fy];
				for( x = 0; x < i_width; x++ )
				{
					dst[x] = f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src];
				}
			}
#endif
			dst  += i_dst;
			src  += i_src;
		}
	}
	else
	{
		STACK_ALIGN(16,dctcoef,buf_array,32*35)
		dctcoef *buf = buf_array;

		//tmp = src;
		src -= i_src;
		for( y = -1; y < i_height+2; y++ )
		{
#if 1
			_mm_prefetch(src+i_src-1,_MM_HINT_NTA);
			for( x = 0; x < i_width; x+=8 )
			{
				//buf[x] = f[0]*src[x-1]  + f[1]*src[x] + f[2]*src[x+1] + f[3]*src[x+2];
				
				d[0]=_mm_loadu_si128((__m128i*)(src+x-1));//8 pixels
				d[1]=_mm_shuffle_epi8(d[0],shuffle);

				d[2]=_mm_srli_si128(d[0],4);
				d[2]=_mm_shuffle_epi8(d[2],shuffle);

				d[1]=_mm_maddubs_epi16(d[1],s8);
				d[2]=_mm_maddubs_epi16(d[2],s8);

				d[1]=_mm_hadd_epi16(d[1],d[2]);

				_mm_store_si128((__m128i*)(buf+x),d[1]);//sse3
			}
#else
			{
				int16_t *f=chroma_frac_coeff[fx];
				for( x = 0; x < i_width; x++ )
				{
					buf[x] = f[0]*src[x-1]  + f[1]*src[x] + f[2]*src[x+1] + f[3]*src[x+2];
				}
			}
#endif
			buf  += 32;
			src  += i_src;
		}

		buf = buf_array + 32;
		sy[0]=_mm_set1_epi16(chroma_frac_coeff[fy][0]);
		sy[1]=_mm_set1_epi16(chroma_frac_coeff[fy][1]);
		sy[2]=_mm_set1_epi16(chroma_frac_coeff[fy][2]);
		sy[3]=_mm_set1_epi16(chroma_frac_coeff[fy][3]);
		for( y = 0; y < i_height; y++ )
		{
#if 1
			for( x = 0; x < i_width; x+=4 )
			{
				//dst[x] = (f[0]*buf[x-32]  + f[1]*buf[x] + f[2]*buf[x+32] + f[3]*buf[x+2*32]) >> 6;

				d[0]=_mm_loadl_epi64((__m128i*)(buf+x-32));
				d[1]=_mm_loadl_epi64((__m128i*)(buf+x));
				d[2]=_mm_loadl_epi64((__m128i*)(buf+x+32));
				d[3]=_mm_loadl_epi64((__m128i*)(buf+x+64));

				d[0]=_mm_unpacklo_epi16(_mm_mullo_epi16(d[0],sy[0]),_mm_mulhi_epi16(d[0],sy[0]));

				d[1]=_mm_unpacklo_epi16(_mm_mullo_epi16(d[1],sy[1]),_mm_mulhi_epi16(d[1],sy[1]));
				d[0]=_mm_add_epi32(d[0],d[1]);

				d[2]=_mm_unpacklo_epi16(_mm_mullo_epi16(d[2],sy[2]),_mm_mulhi_epi16(d[2],sy[2]));
				d[0]=_mm_add_epi32(d[0],d[2]);

				d[3]=_mm_unpacklo_epi16(_mm_mullo_epi16(d[3],sy[3]),_mm_mulhi_epi16(d[3],sy[3]));
				d[0]=_mm_add_epi32(d[0],d[3]);
				
				d[0]=_mm_srai_epi32(d[0],6);
				d[0]=_mm_packs_epi32(d[0],d[0]);

				_mm_storel_epi64((__m128i*)(dst+x),d[0]);//sse3
			}
#else
			{
				int16_t *f=chroma_frac_coeff[fy];
				for( x = 0; x < i_width; x++ )
				{
					dst[x] = (f[0]*buf[x-32]  + f[1]*buf[x] + f[2]*buf[x+32] + f[3]*buf[x+2*32]) >> 6;
				}
			}
#endif
			dst  += i_dst;
			buf  += 32;
		}
	}
	////_mm_empty();
}
*/
void get_ref_chroma_ssse3_shushuang( int16_t *dst, int i_dst, pixel *src, int i_src,
						   int mvx, int mvy, int i_width, int i_height )
{
	int x, y;
	int fx = mvx&0x7;
	int fy = mvy&0x7;

	__m128i s8,d[4];
	__m128i shuffle={
		0,1,2,3,
		1,2,3,4,
		2,3,4,5,
		3,4,5,6
	};
	////_mm_empty();
	//s=frac_coeff_ch[fx];
	s8=frac_coeff_ch_8bit[fx];

	src += (mvy >> 3) * i_src + (mvx >> 3);

	if( fy == 0 )
	{

#if 1
		if( fx == 0 )
		{
			for( y = 0; y < i_height; y++ )
			{
				for( x = 0; x < i_width; x+=8 )
				{
					//dst[x] = (src[x]<<6);

					d[0]=_mm_loadl_epi64((__m128i*)(src+x));
					d[0]=_mm_unpacklo_epi8(d[0],zero);
					d[0]=_mm_slli_epi16(d[0],6);

					_mm_store_si128((__m128i*)(dst+x),d[0]);

				}
				dst  += i_dst;
				src  += i_src;
			}
		}
		else
#endif
		for( y = 0; y < i_height; y++ )
		{
#if 1
			for( x = 0; x < i_width; x+=8 )
			{
				//dst[x] = f[0]*src[x-1]  + f[1]*src[x] + f[2]*src[x+1] + f[3]*src[x+2];
				
				//d[1]=_mm_loadl_epi64((__m128i*)(src+x-1));//8 pixels

				d[0]=_mm_loadu_si128((__m128i*)(src+x-1));//8 pixels
				d[1]=_mm_shuffle_epi8(d[0],shuffle);

				d[2]=_mm_srli_si128(d[0],4);
				d[2]=_mm_shuffle_epi8(d[2],shuffle);

				d[1]=_mm_maddubs_epi16(d[1],s8);
				d[2]=_mm_maddubs_epi16(d[2],s8);

				d[1]=_mm_hadd_epi16(d[1],d[2]);

				//_mm_storel_epi64((__m128i*)(dst+x),d[0]);//sse3
				_mm_store_si128((__m128i*)(dst+x),d[1]);//sse3
				
			}
#else
			{
				int16_t *f=chroma_frac_coeff[fx];
				for( x = 0; x < i_width; x++ )
				{
					dst[x] = f[0]*src[x-1]  + f[1]*src[x] + f[2]*src[x+1] + f[3]*src[x+2];
				}
			}
#endif
			dst  += i_dst;
			src += i_src;
		}
	}
	else if( fx == 0 )
	{
		if (fy == 1){
			for( y = 0; y < i_height; y++ )
			{
#if 1
				for( x = 0; x < i_width; x+=8 )
				{
					//dst[x] = f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src];
					d[1] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x)),zero);
					d[0] = _mm_add_epi16(_mm_slli_epi16(d[1],5),_mm_slli_epi16(d[1],4));
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+i_src)),zero);
					d[1] = _mm_add_epi16(d[1],d[2]);
					d[0] = _mm_add_epi16(d[0],_mm_slli_epi16(d[1],3));
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x-1*i_src)),zero);
					d[1] = _mm_sub_epi16(d[1],d[2]);
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+2*i_src)),zero);
					d[1] = _mm_sub_epi16(d[1],d[2]);
					d[0] = _mm_add_epi16(d[0],_mm_slli_epi16(d[1],1));

					_mm_store_si128((__m128i*)(dst+x),d[0]);
				}
#else
				{
					int16_t *f=chroma_frac_coeff[fy];
					for( x = 0; x < i_width; x++ )
					{
						dst[x] = f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src];
					}
				}
#endif
				dst  += i_dst;
				src  += i_src;
			}
		}
		else if (fy == 7){
			for( y = 0; y < i_height; y++ )
			{
#if 1
				for( x = 0; x < i_width; x+=8 )
				{
					//dst[x] = f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src];
					d[1] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+1*i_src)),zero);
					d[0] = _mm_add_epi16(_mm_slli_epi16(d[1],5),_mm_slli_epi16(d[1],4));
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x)),zero);
					d[1] = _mm_add_epi16(d[1],d[2]);
					d[0] = _mm_add_epi16(d[0],_mm_slli_epi16(d[1],3));
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+2*i_src)),zero);
					d[1] = _mm_sub_epi16(d[1],d[2]);
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x-1*i_src)),zero);
					d[1] = _mm_sub_epi16(d[1],d[2]);
					d[0] = _mm_add_epi16(d[0],_mm_slli_epi16(d[1],1));

					_mm_store_si128((__m128i*)(dst+x),d[0]);
				}
#else
				{
					int16_t *f=chroma_frac_coeff[fy];
					for( x = 0; x < i_width; x++ )
					{
						dst[x] = f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src];
					}
				}
#endif
				dst  += i_dst;
				src  += i_src;
			}
		}
		else if (fy == 2){
			for( y = 0; y < i_height; y++ )
			{
#if 1
				for( x = 0; x < i_width; x+=8 )
				{
					//dst[x] = f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src];
					d[1] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x)),zero);
					d[0] = _mm_slli_epi16(d[1],5);
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+i_src)),zero);
					d[2] = _mm_add_epi16(d[1],d[2]);
					d[0] = _mm_add_epi16(d[0],_mm_slli_epi16(d[2],4));
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x-i_src)),zero);
					d[2] = _mm_sub_epi16(d[1],d[2]);
					d[0] = _mm_add_epi16(d[0],_mm_slli_epi16(d[2],2));
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+2*i_src)),zero);
					d[2] = _mm_sub_epi16(d[1],d[2]);
					d[0] = _mm_add_epi16(d[0],_mm_slli_epi16(d[2],1));

					_mm_store_si128((__m128i*)(dst+x),d[0]);
				}
#else
				{
					int16_t *f=chroma_frac_coeff[fy];
					for( x = 0; x < i_width; x++ )
					{
						dst[x] = f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src];
					}
				}
#endif
				dst  += i_dst;
				src  += i_src;
			}
		}
		else if (fy == 6){
			for( y = 0; y < i_height; y++ )
			{
#if 1
				for( x = 0; x < i_width; x+=8 )
				{
					//dst[x] = f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src];
					
					d[1] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+i_src)),zero);
					d[0] = _mm_slli_epi16(d[1],5);
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x)),zero);
					d[2] = _mm_add_epi16(d[1],d[2]);
					d[0] = _mm_add_epi16(d[0],_mm_slli_epi16(d[2],4));
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+2*i_src)),zero);
					d[2] = _mm_sub_epi16(d[1],d[2]);
					d[0] = _mm_add_epi16(d[0],_mm_slli_epi16(d[2],2));
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x-i_src)),zero);
					d[2] = _mm_sub_epi16(d[1],d[2]);
					d[0] = _mm_add_epi16(d[0],_mm_slli_epi16(d[2],1));

					_mm_store_si128((__m128i*)(dst+x),d[0]);
				}
#else
				{
					int16_t *f=chroma_frac_coeff[fy];
					for( x = 0; x < i_width; x++ )
					{
						dst[x] = f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src];
					}
				}
#endif
				dst  += i_dst;
				src  += i_src;
			}
		}
		else if (fy == 3){
			for( y = 0; y < i_height; y++ )
			{
#if 1
				for( x = 0; x < i_width; x+=8 )
				{
					//dst[x] = f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src];
					d[1] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x)),zero);
					d[0] = _mm_slli_epi16(d[1],3);
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x-i_src)),zero);
					d[3] = _mm_sub_epi16(d[1],d[2]);
					d[0] = _mm_add_epi16(d[0],_mm_slli_epi16(d[3],1));
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+i_src)),zero);
					d[0] = _mm_add_epi16(d[0],_mm_slli_epi16(_mm_add_epi16(d[1],d[2]),5));
					d[3] = _mm_sub_epi16(d[3],d[2]);
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+2*i_src)),zero);
					d[1] = _mm_sub_epi16(d[3],d[2]);
					d[0] = _mm_add_epi16(d[0],_mm_slli_epi16(d[1],2));

					_mm_store_si128((__m128i*)(dst+x),d[0]);
				}
#else
				{
					int16_t *f=chroma_frac_coeff[fy];
					for( x = 0; x < i_width; x++ )
					{
						dst[x] = f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src];
					}
				}
#endif
				dst  += i_dst;
				src  += i_src;
			}
		}
		else if (fy == 5){
			for( y = 0; y < i_height; y++ )
			{
#if 1
				for( x = 0; x < i_width; x+=8 )
				{
					//dst[x] = f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src];
					
					d[1] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+i_src)),zero);
					d[0] = _mm_slli_epi16(d[1],3);
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+2*i_src)),zero);
					d[3] = _mm_sub_epi16(d[1],d[2]);
					d[0] = _mm_add_epi16(d[0],_mm_slli_epi16(d[3],1));
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x)),zero);
					d[0] = _mm_add_epi16(d[0],_mm_slli_epi16(_mm_add_epi16(d[1],d[2]),5));
					d[3] = _mm_sub_epi16(d[3],d[2]);
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x-i_src)),zero);
					d[1] = _mm_sub_epi16(d[3],d[2]);
					d[0] = _mm_add_epi16(d[0],_mm_slli_epi16(d[1],2));

					_mm_store_si128((__m128i*)(dst+x),d[0]);
				}
#else
				{
					int16_t *f=chroma_frac_coeff[fy];
					for( x = 0; x < i_width; x++ )
					{
						dst[x] = f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src];
					}
				}
#endif
				dst  += i_dst;
				src  += i_src;
			}
		}
		else
		{
			for( y = 0; y < i_height; y++ )
			{
#if 1
				for( x = 0; x < i_width; x+=8 )
				{
					//dst[x] = f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src];
					d[1] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x)),zero);
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+i_src)),zero);
					d[1] = _mm_add_epi16(d[1],d[2]);
					d[0] = _mm_slli_epi16(d[1],5);

					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x-i_src)),zero);
					d[1] = _mm_sub_epi16(d[1],d[2]);
					d[2] = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+2*i_src)),zero);
					d[1] = _mm_sub_epi16(d[1],d[2]);
					d[0] = _mm_add_epi16(d[0],_mm_slli_epi16(d[1],2));

					_mm_store_si128((__m128i*)(dst+x),d[0]);
				}
#else
				{
					int16_t *f=chroma_frac_coeff[fy];
					for( x = 0; x < i_width; x++ )
					{
						dst[x] = f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src];
					}
				}
#endif
				dst  += i_dst;
				src  += i_src;
			}
		}
	}
	else
	{
		STACK_ALIGN(16,dctcoef,buf_array,32*35)
		dctcoef *buf = buf_array;

		//tmp = src;
		src -= i_src;
		for( y = -1; y < i_height+2; y++ )
		{
#if 1
			_mm_prefetch(src+i_src-1,_MM_HINT_NTA);
			for( x = 0; x < i_width; x+=8 )
			{
				//buf[x] = f[0]*src[x-1]  + f[1]*src[x] + f[2]*src[x+1] + f[3]*src[x+2];
				
				d[0]=_mm_loadu_si128((__m128i*)(src+x-1));//8 pixels
				d[1]=_mm_shuffle_epi8(d[0],shuffle);

				d[2]=_mm_srli_si128(d[0],4);
				d[2]=_mm_shuffle_epi8(d[2],shuffle);

				d[1]=_mm_maddubs_epi16(d[1],s8);
				d[2]=_mm_maddubs_epi16(d[2],s8);

				d[1]=_mm_hadd_epi16(d[1],d[2]);

				_mm_store_si128((__m128i*)(buf+x),d[1]);//sse3
			}
#else
			{
				int16_t *f=chroma_frac_coeff[fx];
				for( x = 0; x < i_width; x++ )
				{
					buf[x] = f[0]*src[x-1]  + f[1]*src[x] + f[2]*src[x+1] + f[3]*src[x+2];
				}
			}
#endif
			buf  += 32;
			src  += i_src;
		}

		buf = buf_array + 32;
		_mm_prefetch(buf_array-32,_MM_HINT_NTA);
		
		if (fy == 1){
			for( y = 0; y < i_height; y++ )
			{
#if 1
				for( x = 0; x < i_width; x+=4 )
				{
					//dst[x] = f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src];
					d[1] = _mm_loadl_epi64((__m128i*)(buf+x));
					d[1] = _mm_unpacklo_epi16(d[1],_mm_cmplt_epi16(d[1],zero));
					d[0] = _mm_add_epi32(_mm_slli_epi32(d[1],5),_mm_slli_epi32(d[1],4));
					d[2] = _mm_loadl_epi64((__m128i*)(buf+x+32));
					d[2] = _mm_unpacklo_epi16(d[2],_mm_cmplt_epi16(d[2],zero));
					d[1] = _mm_add_epi32(d[1],d[2]);
					d[0] = _mm_add_epi32(d[0],_mm_slli_epi32(d[1],3));
					d[2] = _mm_loadl_epi64((__m128i*)(buf+x-32));
					d[2] = _mm_unpacklo_epi16(d[2],_mm_cmplt_epi16(d[2],zero));
					d[1] = _mm_sub_epi32(d[1],d[2]);
					d[2] = _mm_loadl_epi64((__m128i*)(buf+x+64));
					d[2] = _mm_unpacklo_epi16(d[2],_mm_cmplt_epi16(d[2],zero));
					d[1] = _mm_sub_epi32(d[1],d[2]);
					d[0] = _mm_add_epi32(d[0],_mm_slli_epi32(d[1],1));

					d[0]=_mm_srai_epi32(d[0],6);
					d[0]=_mm_packs_epi32(d[0],d[0]);

					_mm_storel_epi64((__m128i*)(dst+x),d[0]);//sse3
				}
#else
				{
					int16_t *f=chroma_frac_coeff[fy];
					for( x = 0; x < i_width; x++ )
					{
						dst[x] = (f[0]*buf[x-32]  + f[1]*buf[x] + f[2]*buf[x+32] + f[3]*buf[x+2*32]) >> 6;
					}
				}
#endif
				dst  += i_dst;
				buf  += 32;
			}
		}
		else if (fy == 7){
			for( y = 0; y < i_height; y++ )
			{
#if 1
				for( x = 0; x < i_width; x+=4 )
				{
					//dst[x] = f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src];
					d[1] = _mm_loadl_epi64((__m128i*)(buf+x+32));
					d[1] = _mm_unpacklo_epi16(d[1],_mm_cmplt_epi16(d[1],zero));
					d[0] = _mm_add_epi32(_mm_slli_epi32(d[1],5),_mm_slli_epi32(d[1],4));
					d[2] = _mm_loadl_epi64((__m128i*)(buf+x));
					d[2] = _mm_unpacklo_epi16(d[2],_mm_cmplt_epi16(d[2],zero));
					d[1] = _mm_add_epi32(d[1],d[2]);
					d[0] = _mm_add_epi32(d[0],_mm_slli_epi32(d[1],3));
					d[2] = _mm_loadl_epi64((__m128i*)(buf+x+64));
					d[2] = _mm_unpacklo_epi16(d[2],_mm_cmplt_epi16(d[2],zero));
					d[1] = _mm_sub_epi32(d[1],d[2]);
					d[2] = _mm_loadl_epi64((__m128i*)(buf+x-32));
					d[2] = _mm_unpacklo_epi16(d[2],_mm_cmplt_epi16(d[2],zero));
					d[1] = _mm_sub_epi32(d[1],d[2]);
					d[0] = _mm_add_epi32(d[0],_mm_slli_epi32(d[1],1));

					d[0]=_mm_srai_epi32(d[0],6);
					d[0]=_mm_packs_epi32(d[0],d[0]);

					_mm_storel_epi64((__m128i*)(dst+x),d[0]);//sse3
				}
#else
				{
					int16_t *f=chroma_frac_coeff[fy];
					for( x = 0; x < i_width; x++ )
					{
						dst[x] = (f[0]*buf[x-32]  + f[1]*buf[x] + f[2]*buf[x+32] + f[3]*buf[x+2*32]) >> 6;
					}
				}
#endif
				dst  += i_dst;
				buf  += 32;
			}
		}
		else if (fy == 2){
			for( y = 0; y < i_height; y++ )
			{
#if 1
				for( x = 0; x < i_width; x+=4 )
				{
					//dst[x] = f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src];
					d[1] = _mm_loadl_epi64((__m128i*)(buf+x));
					d[1] = _mm_unpacklo_epi16(d[1],_mm_cmplt_epi16(d[1],zero));
					d[0] = _mm_slli_epi32(d[1],5);
					d[2] = _mm_loadl_epi64((__m128i*)(buf+x+32));
					d[2] = _mm_unpacklo_epi16(d[2],_mm_cmplt_epi16(d[2],zero));
					d[2] = _mm_add_epi32(d[1],d[2]);
					d[0] = _mm_add_epi32(d[0],_mm_slli_epi32(d[2],4));
					d[2] = _mm_loadl_epi64((__m128i*)(buf+x-32));
					d[2] = _mm_unpacklo_epi16(d[2],_mm_cmplt_epi16(d[2],zero));
					d[2] = _mm_sub_epi32(d[1],d[2]);
					d[0] = _mm_add_epi32(d[0],_mm_slli_epi32(d[2],2));
					d[2] = _mm_loadl_epi64((__m128i*)(buf+x+64));
					d[2] = _mm_unpacklo_epi16(d[2],_mm_cmplt_epi16(d[2],zero));
					d[2] = _mm_sub_epi32(d[1],d[2]);
					d[0] = _mm_add_epi32(d[0],_mm_slli_epi32(d[2],1));

					d[0]=_mm_srai_epi32(d[0],6);
					d[0]=_mm_packs_epi32(d[0],d[0]);

					_mm_storel_epi64((__m128i*)(dst+x),d[0]);//sse3
				}
#else
				{
					int16_t *f=chroma_frac_coeff[fy];
					for( x = 0; x < i_width; x++ )
					{
						dst[x] = (f[0]*buf[x-32]  + f[1]*buf[x] + f[2]*buf[x+32] + f[3]*buf[x+2*32]) >> 6;
					}
				}
#endif
				dst  += i_dst;
				buf  += 32;
			}
		}
		else if (fy == 6){
			for( y = 0; y < i_height; y++ )
			{
#if 1
				for( x = 0; x < i_width; x+=4 )
				{
					//dst[x] = f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src];
					
					d[1] = _mm_loadl_epi64((__m128i*)(buf+x+32));
					d[1] = _mm_unpacklo_epi16(d[1],_mm_cmplt_epi16(d[1],zero));
					d[0] = _mm_slli_epi32(d[1],5);
					d[2] = _mm_loadl_epi64((__m128i*)(buf+x));
					d[2] = _mm_unpacklo_epi16(d[2],_mm_cmplt_epi16(d[2],zero));
					d[2] = _mm_add_epi32(d[1],d[2]);
					d[0] = _mm_add_epi32(d[0],_mm_slli_epi32(d[2],4));
					d[2] = _mm_loadl_epi64((__m128i*)(buf+x+64));
					d[2] = _mm_unpacklo_epi16(d[2],_mm_cmplt_epi16(d[2],zero));
					d[2] = _mm_sub_epi32(d[1],d[2]);
					d[0] = _mm_add_epi32(d[0],_mm_slli_epi32(d[2],2));
					d[2] = _mm_loadl_epi64((__m128i*)(buf+x-32));
					d[2] = _mm_unpacklo_epi16(d[2],_mm_cmplt_epi16(d[2],zero));
					d[2] = _mm_sub_epi32(d[1],d[2]);
					d[0] = _mm_add_epi32(d[0],_mm_slli_epi32(d[2],1));

					d[0]=_mm_srai_epi32(d[0],6);
					d[0]=_mm_packs_epi32(d[0],d[0]);

					_mm_storel_epi64((__m128i*)(dst+x),d[0]);//sse3
				}
#else
				{
					int16_t *f=chroma_frac_coeff[fy];
					for( x = 0; x < i_width; x++ )
					{
						dst[x] = (f[0]*buf[x-32]  + f[1]*buf[x] + f[2]*buf[x+32] + f[3]*buf[x+2*32]) >> 6;
					}
				}
#endif
				dst  += i_dst;
				buf  += 32;
			}
		}
		else if (fy == 3){
			for( y = 0; y < i_height; y++ )
			{
#if 1
				for( x = 0; x < i_width; x+=4 )
				{
					//dst[x] = f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src];
					d[1] = _mm_loadl_epi64((__m128i*)(buf+x));
					d[1] = _mm_unpacklo_epi16(d[1],_mm_cmplt_epi16(d[1],zero));
					d[0] = _mm_slli_epi32(d[1],3);
					d[2] = _mm_loadl_epi64((__m128i*)(buf+x-32));
					d[2] = _mm_unpacklo_epi16(d[2],_mm_cmplt_epi16(d[2],zero));
					d[3] = _mm_sub_epi32(d[1],d[2]);
					d[0] = _mm_add_epi32(d[0],_mm_slli_epi32(d[3],1));
					d[2] = _mm_loadl_epi64((__m128i*)(buf+x+32));
					d[2] = _mm_unpacklo_epi16(d[2],_mm_cmplt_epi16(d[2],zero));
					d[0] = _mm_add_epi32(d[0],_mm_slli_epi32(_mm_add_epi32(d[1],d[2]),5));
					d[3] = _mm_sub_epi32(d[3],d[2]);
					d[2] = _mm_loadl_epi64((__m128i*)(buf+x+64));
					d[2] = _mm_unpacklo_epi16(d[2],_mm_cmplt_epi16(d[2],zero));
					d[1] = _mm_sub_epi32(d[3],d[2]);
					d[0] = _mm_add_epi32(d[0],_mm_slli_epi32(d[1],2));

					d[0]=_mm_srai_epi32(d[0],6);
					d[0]=_mm_packs_epi32(d[0],d[0]);

					_mm_storel_epi64((__m128i*)(dst+x),d[0]);//sse3
				}
#else
				{
					int16_t *f=chroma_frac_coeff[fy];
					for( x = 0; x < i_width; x++ )
					{
						dst[x] = (f[0]*buf[x-32]  + f[1]*buf[x] + f[2]*buf[x+32] + f[3]*buf[x+2*32]) >> 6;
					}
				}
#endif
				dst  += i_dst;
				buf  += 32;
			}
		}
		else if (fy == 5){
			for( y = 0; y < i_height; y++ )
			{
#if 1
				for( x = 0; x < i_width; x+=4 )
				{
					//dst[x] = f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src];
					
					
					d[1] = _mm_loadl_epi64((__m128i*)(buf+x+32));
					d[1] = _mm_unpacklo_epi16(d[1],_mm_cmplt_epi16(d[1],zero));
					d[0] = _mm_slli_epi32(d[1],3);
					d[2] = _mm_loadl_epi64((__m128i*)(buf+x+64));
					d[2] = _mm_unpacklo_epi16(d[2],_mm_cmplt_epi16(d[2],zero));
					d[3] = _mm_sub_epi32(d[1],d[2]);
					d[0] = _mm_add_epi32(d[0],_mm_slli_epi32(d[3],1));
					d[2] = _mm_loadl_epi64((__m128i*)(buf+x));
					d[2] = _mm_unpacklo_epi16(d[2],_mm_cmplt_epi16(d[2],zero));
					d[0] = _mm_add_epi32(d[0],_mm_slli_epi32(_mm_add_epi32(d[1],d[2]),5));
					d[3] = _mm_sub_epi32(d[3],d[2]);
					d[2] = _mm_loadl_epi64((__m128i*)(buf+x-32));
					d[2] = _mm_unpacklo_epi16(d[2],_mm_cmplt_epi16(d[2],zero));
					d[1] = _mm_sub_epi32(d[3],d[2]);
					d[0] = _mm_add_epi32(d[0],_mm_slli_epi32(d[1],2));

					d[0]=_mm_srai_epi32(d[0],6);
					d[0]=_mm_packs_epi32(d[0],d[0]);

					_mm_storel_epi64((__m128i*)(dst+x),d[0]);//sse3
				}
#else
				{
					int16_t *f=chroma_frac_coeff[fy];
					for( x = 0; x < i_width; x++ )
					{
						dst[x] = (f[0]*buf[x-32]  + f[1]*buf[x] + f[2]*buf[x+32] + f[3]*buf[x+2*32]) >> 6;
					}
				}
#endif
				dst  += i_dst;
				buf  += 32;
			}
		}
		else{
			for( y = 0; y < i_height; y++ )
			{
#if 1
				for( x = 0; x < i_width; x+=4 )
				{
					//dst[x] = f[0]*src[x-i_src]  + f[1]*src[x] + f[2]*src[x+i_src] + f[3]*src[x+2*i_src];
					d[1] = _mm_loadl_epi64((__m128i*)(buf+x));
					d[1] = _mm_unpacklo_epi16(d[1],_mm_cmplt_epi16(d[1],zero));
					d[2] = _mm_loadl_epi64((__m128i*)(buf+x+32));
					d[2] = _mm_unpacklo_epi16(d[2],_mm_cmplt_epi16(d[2],zero));
					d[1] = _mm_add_epi32(d[1],d[2]);
					d[0] = _mm_slli_epi32(d[1],5);

					d[2] = _mm_loadl_epi64((__m128i*)(buf+x-32));
					d[2] = _mm_unpacklo_epi16(d[2],_mm_cmplt_epi16(d[2],zero));
					d[1] = _mm_sub_epi32(d[1],d[2]);
					d[2] = _mm_loadl_epi64((__m128i*)(buf+x+64));
					d[2] = _mm_unpacklo_epi16(d[2],_mm_cmplt_epi16(d[2],zero));
					d[1] = _mm_sub_epi32(d[1],d[2]);
					d[0] = _mm_add_epi32(d[0],_mm_slli_epi32(d[1],2));

					d[0]=_mm_srai_epi32(d[0],6);
					d[0]=_mm_packs_epi32(d[0],d[0]);

					_mm_storel_epi64((__m128i*)(dst+x),d[0]);//sse3
				}
#else
				{
					int16_t *f=chroma_frac_coeff[fy];
					for( x = 0; x < i_width; x++ )
					{
						dst[x] = (f[0]*buf[x-32]  + f[1]*buf[x] + f[2]*buf[x+32] + f[3]*buf[x+2*32]) >> 6;
					}
				}
#endif
				dst  += i_dst;
				buf  += 32;
			}
		}
	}
	////_mm_empty();
}

#define TAPFILTER(src,f) \
	((int32_t)src[x-3*i_src]*f[0]+src[x-2*i_src]*f[1]+src[x-1*i_src]*f[2]+src[x]*f[3]+ \
	src[x+1*i_src]*f[4]+src[x+2*i_src]*f[5]+src[x+3*i_src]*f[6]+src[x+4*i_src]*f[7]) 
void mc_luma_sse4( pixel *dst, int i_dst, pixel *src, int16_t hpx[3][HPEL_SIZE*HPEL_SIZE], int i_src,
			 int mvx, int mvy, int i_width, int i_height )
{
	int fx = mvx&3;
	int fy = mvy&3;
	int i_offs = (mvy>>2)*i_src + (mvx>>2);
	int x, y;
	const int16_t *f = luma_frac_coeff[fy];

	__m128i d,tmp,tmp1,temp,tmp2; 
	////_mm_empty();
	tmp2=_mm_set1_epi16(32);

	if( fx == 0 )
	{//use src
		src += i_offs;
		if( fy == 0 )
			mc_copy( dst, i_dst, src, i_src, i_width, i_height );
		else if( fy == 1 )
		{
			for( y = 0; y < i_height; y++ )
			{
#if 1
				_mm_prefetch(src,_MM_HINT_T0);
				_mm_prefetch(src+i_src,_MM_HINT_T0);
				_mm_prefetch(src+i_src*2,_MM_HINT_T0);
				for( x = 0; x < i_width; x+=8 )
				{
					//dst[x] = TAPFILTER( src, f );{ -1, 4, -10, 58, 17,  -5, 1,  0 }

					d=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+3*i_src)),zero);//*1
					tmp=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x)),zero);
					d=_mm_add_epi16(d,_mm_slli_epi16(tmp,5));
					tmp1=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x-1*i_src)),zero);
					temp=_mm_slli_epi16(_mm_sub_epi16(tmp,tmp1),1);
					d=_mm_add_epi16(d,temp);
					tmp1=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x-2*i_src)),zero);
					temp=_mm_add_epi16(temp,tmp1);
					tmp1=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+2*i_src)),zero);
					d=_mm_sub_epi16(d,tmp1);
					temp=_mm_sub_epi16(temp,tmp1);
					temp=_mm_slli_epi16(temp,2);
					d=_mm_add_epi16(d,temp);
					tmp1=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+i_src)),zero);
					d=_mm_add_epi16(d,tmp1);
					tmp=_mm_slli_epi16(_mm_add_epi16(tmp,tmp1),4);
					d=_mm_add_epi16(d,tmp);
					tmp1=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x-3*i_src)),zero);
					d=_mm_sub_epi16(d,tmp1);

					d=_mm_add_epi16(d,tmp2);//add 32
					d=_mm_srai_epi16(d,6);
					d=_mm_packus_epi16(d,d);

					_mm_storel_epi64((__m128i*)(dst+x),d);

				}
#else
				for( x = 0; x < i_width; x++ )
					dst[x] = lent_clip_uint8((TAPFILTER( src, f )+32)>>6);
#endif
				dst  += i_dst;
				src  += i_src;
			}
		}
		else if( fy == 2 )
		{
			for( y = 0; y < i_height; y++ )
			{
#if 1
				_mm_prefetch(src,_MM_HINT_T0);
				_mm_prefetch(src+i_src,_MM_HINT_T0);
				_mm_prefetch(src+i_src*2,_MM_HINT_T0);
				for( x = 0; x < i_width; x+=8 )
				{
					//dst[x] = TAPFILTER( src, f );{ -1, 4, -11, 40, 40, -11, 4, -1 }

					d=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x-3*i_src)),zero);
					tmp=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+4*i_src)),zero);
					d=_mm_add_epi16(tmp,d);

					temp=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x-2*i_src)),zero);
					tmp=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+3*i_src)),zero);
					tmp=_mm_slli_epi16(_mm_add_epi16(tmp,temp),2);
					d=_mm_sub_epi16(tmp,d);

					temp=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x-1*i_src)),zero);
					tmp=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+2*i_src)),zero);
					tmp=_mm_add_epi16(tmp,temp);
					d=_mm_sub_epi16(d,tmp);

					temp=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x)),zero);
					tmp1=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+i_src)),zero);
					tmp1=_mm_add_epi16(tmp1,temp);
					temp=_mm_sub_epi16(_mm_slli_epi16(tmp1,3),_mm_slli_epi16(tmp,1));
					d=_mm_add_epi16(temp,d);
					temp=_mm_slli_epi16(temp,2);
					d=_mm_add_epi16(temp,d);

					d=_mm_add_epi16(d,tmp2);//add 32
					d=_mm_srai_epi16(d,6);
					d=_mm_packus_epi16(d,d);

					_mm_storel_epi64((__m128i*)(dst+x),d);

				}
#else
				for( x = 0; x < i_width; x++ )
					dst[x] = lent_clip_uint8((TAPFILTER( src, f )+32)>>6);
#endif
				dst  += i_dst;
				src  += i_src;
			}
		}
		else //if( fy == 3 )
		{
			for( y = 0; y < i_height; y++ )
			{
#if 1
				_mm_prefetch(src,_MM_HINT_T0);
				_mm_prefetch(src+i_src,_MM_HINT_T0);
				_mm_prefetch(src+i_src*2,_MM_HINT_T0);
				for( x = 0; x < i_width; x+=8 )
				{
					//dst[x] = TAPFILTER( src, f );{  0, 1,  -5, 17, 58, -10, 4, -1 }

					d=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x-2*i_src)),zero);//*1
					tmp=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+i_src)),zero);
					d=_mm_add_epi16(d,_mm_slli_epi16(tmp,5));
					tmp1=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+2*i_src)),zero);
					temp=_mm_slli_epi16(_mm_sub_epi16(tmp,tmp1),1);
					d=_mm_add_epi16(d,temp);
					tmp1=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+3*i_src)),zero);
					temp=_mm_add_epi16(temp,tmp1);
					tmp1=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x-i_src)),zero);
					d=_mm_sub_epi16(d,tmp1);
					temp=_mm_sub_epi16(temp,tmp1);
					temp=_mm_slli_epi16(temp,2);
					d=_mm_add_epi16(d,temp);
					tmp1=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x)),zero);
					d=_mm_add_epi16(d,tmp1);
					tmp=_mm_slli_epi16(_mm_add_epi16(tmp,tmp1),4);
					d=_mm_add_epi16(d,tmp);
					tmp1=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+4*i_src)),zero);
					d=_mm_sub_epi16(d,tmp1);

					d=_mm_add_epi16(d,tmp2);//add 32
					d=_mm_srai_epi16(d,6);
					d=_mm_packus_epi16(d,d);

					_mm_storel_epi64((__m128i*)(dst+x),d);

				}
#else
				for( x = 0; x < i_width; x++ )
					dst[x] = lent_clip_uint8((TAPFILTER( src, f )+32)>>6);
#endif
				dst  += i_dst;
				src  += i_src;
			}
		}
	}
	else
	{//use hpx
		int16_t *hsrc = hpx[fx-1]+3*HPEL_SIZE /*+ i_offs*/;
		i_src=HPEL_SIZE;

		if( fy == 0 )
		{//加速
			for( y = 0; y < i_height; y++ )
			{

#if 1
				for( x = 0; x < i_width; x+=8 )
				{
					//dst[x] = lent_clip_uint8((hsrc[x]  + 32) >> 6);

					d=_mm_loadu_si128((__m128i*)(hsrc+x));
					d=_mm_add_epi16(d,tmp2);//add 32
					d=_mm_srai_epi16(d,6);
					d=_mm_packus_epi16(d,d);

					_mm_storel_epi64((__m128i*)(dst+x),d);//sse3

				}
#else
				for( x = 0; x < i_width; x++ )
					dst[x] = lent_clip_uint8((hsrc[x]  + 32) >> 6);
#endif
				dst  += i_dst;
				hsrc  += i_src;
			}
		}
		else if( fy == 1 )
		{
			tmp2=_mm_set1_epi32(2048);
			for( y = 0; y < i_height; y++ )
			{
#if 1
				for( x = 0; x < i_width; x+=4 )
				{
					//dst[x] = TAPFILTER( src, f )>>6;{ -1, 4, -10, 58, 17,  -5, 1,  0 }

					d=_mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(hsrc+x+3*i_src)));
					tmp=_mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(hsrc+x)));
					d=_mm_add_epi32(d,_mm_slli_epi32(tmp,5));
					tmp1=_mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(hsrc+x-1*i_src)));
					temp=_mm_slli_epi32(_mm_sub_epi32(tmp,tmp1),1);
					d=_mm_add_epi32(d,temp);
					tmp1=_mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(hsrc+x-2*i_src)));
					temp=_mm_add_epi32(temp,tmp1);
					tmp1=_mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(hsrc+x+2*i_src)));
					d=_mm_sub_epi32(d,tmp1);
					temp=_mm_sub_epi32(temp,tmp1);
					temp=_mm_slli_epi32(temp,2);
					d=_mm_add_epi32(d,temp);
					tmp1=_mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(hsrc+x+i_src)));
					d=_mm_add_epi32(d,tmp1);
					tmp=_mm_slli_epi32(_mm_add_epi32(tmp,tmp1),4);
					d=_mm_add_epi32(d,tmp);
					tmp1=_mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(hsrc+x-3*i_src)));
					d=_mm_sub_epi32(d,tmp1);

					d=_mm_add_epi32(d,tmp2);
					d=_mm_srai_epi32(d,12);


					tmp=_mm_cmpgt_epi32(d,zero);
					d=_mm_and_si128(d,tmp);

					tmp=_mm_cmpgt_epi32(d,max8bit);
					d=_mm_or_si128(d,tmp);
					d=_mm_and_si128(d,max8bit);

					d=_mm_packs_epi32(d,d);
					d=_mm_packus_epi16(d,d);

					*((int*)(dst+x))=_mm_cvtsi128_si32(d);

				}
#else
				for( x = 0; x < i_width; x++ )
					dst[x] = lent_clip_uint8((TAPFILTER( hsrc, f ) + ((1<<11))) >> 12);
#endif
				dst  += i_dst;
				hsrc  += i_src;
			}
		}
		else if( fy == 2 )
		{
			tmp2=_mm_set1_epi32(2048);
			for( y = 0; y < i_height; y++ )
			{
#if 1
				for( x = 0; x < i_width; x+=4 )
				{
					//dst[x] = TAPFILTER( hsrc, f )>>6;{ -1, 4, -11, 40, 40, -11, 4, -1 }

					d=_mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(hsrc+x-3*i_src)));
					tmp=_mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(hsrc+x+4*i_src)));
					d=_mm_add_epi32(tmp,d);

					temp=_mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(hsrc+x-2*i_src)));
					tmp=_mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(hsrc+x+3*i_src)));
					tmp=_mm_slli_epi32(_mm_add_epi32(tmp,temp),2);
					d=_mm_sub_epi32(tmp,d);

					temp=_mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(hsrc+x-1*i_src)));
					tmp=_mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(hsrc+x+2*i_src)));
					tmp=_mm_add_epi32(tmp,temp);
					d=_mm_sub_epi32(d,tmp);

					temp=_mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(hsrc+x)));
					tmp1=_mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(hsrc+x+i_src)));
					tmp1=_mm_add_epi32(tmp1,temp);
					temp=_mm_sub_epi32(_mm_slli_epi32(tmp1,3),_mm_slli_epi32(tmp,1));
					d=_mm_add_epi32(temp,d);
					temp=_mm_slli_epi32(temp,2);
					d=_mm_add_epi32(temp,d);

					d=_mm_add_epi32(d,tmp2);
					d=_mm_srai_epi32(d,12);


					tmp=_mm_cmpgt_epi32(d,zero);
					d=_mm_and_si128(d,tmp);

					tmp=_mm_cmpgt_epi32(d,max8bit);
					d=_mm_or_si128(d,tmp);
					d=_mm_and_si128(d,max8bit);

					d=_mm_packs_epi32(d,d);
					d=_mm_packus_epi16(d,d);

					*((int*)(dst+x))=_mm_cvtsi128_si32(d);

				}
#else
				for( x = 0; x < i_width; x++ )
					dst[x] = lent_clip_uint8((TAPFILTER( hsrc, f ) + ((1<<11))) >> 12);
#endif
				dst  += i_dst;
				hsrc  += i_src;
			}
		}
		else //if( fy == 3 )
		{
			tmp2=_mm_set1_epi32(2048);
			for( y = 0; y < i_height; y++ )
			{
#if 1
				for( x = 0; x < i_width; x+=4 )
				{
					//dst[x] = TAPFILTER( src, f )>>6;{  0, 1,  -5, 17, 58, -10, 4, -1 }

					d=_mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(hsrc+x-2*i_src)));//*1
					tmp=_mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(hsrc+x+i_src)));
					d=_mm_add_epi32(d,_mm_slli_epi32(tmp,5));
					tmp1=_mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(hsrc+x+2*i_src)));
					temp=_mm_slli_epi32(_mm_sub_epi32(tmp,tmp1),1);
					d=_mm_add_epi32(d,temp);
					tmp1=_mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(hsrc+x+3*i_src)));
					temp=_mm_add_epi32(temp,tmp1);
					tmp1=_mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(hsrc+x-i_src)));
					d=_mm_sub_epi32(d,tmp1);
					temp=_mm_sub_epi32(temp,tmp1);
					temp=_mm_slli_epi32(temp,2);
					d=_mm_add_epi32(d,temp);
					tmp1=_mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(hsrc+x)));
					d=_mm_add_epi32(d,tmp1);
					tmp=_mm_slli_epi32(_mm_add_epi32(tmp,tmp1),4);
					d=_mm_add_epi32(d,tmp);
					tmp1=_mm_cvtepi16_epi32(_mm_loadl_epi64((__m128i*)(hsrc+x+4*i_src)));
					d=_mm_sub_epi32(d,tmp1);

					d=_mm_add_epi32(d,tmp2);
					d=_mm_srai_epi32(d,12);


					tmp=_mm_cmpgt_epi32(d,zero);
					d=_mm_and_si128(d,tmp);

					tmp=_mm_cmpgt_epi32(d,max8bit);
					d=_mm_or_si128(d,tmp);
					d=_mm_and_si128(d,max8bit);

					d=_mm_packs_epi32(d,d);
					d=_mm_packus_epi16(d,d);

					*((int*)(dst+x))=_mm_cvtsi128_si32(d);

				}
#else
				for( x = 0; x < i_width; x++ )
					dst[x] = lent_clip_uint8((TAPFILTER( hsrc, f ) + ((1<<11))) >> 12);
#endif
				dst  += i_dst;
				hsrc  += i_src;
			}
		}
	}
	////_mm_empty();
}
void mc_luma_sse2( pixel *dst, int i_dst, pixel *src, int16_t hpx[3][HPEL_SIZE*HPEL_SIZE], int i_src,
			 int mvx, int mvy, int i_width, int i_height )
{
	int fx = mvx&3;
	int fy = mvy&3;
	int i_offs = (mvy>>2)*i_src + (mvx>>2);
	int x, y;
	const int16_t *f = luma_frac_coeff[fy];

	__m128i d,tmp,tmp1,temp,tmp2; 
	////_mm_empty();
	tmp2=_mm_set1_epi16(32);

	if( fx == 0 )
	{//use src
		src += i_offs;
		if( fy == 0 )
			mc_copy( dst, i_dst, src, i_src, i_width, i_height );
		else if( fy == 1 )
		{
			for( y = 0; y < i_height; y++ )
			{
#if 1
				_mm_prefetch(src,_MM_HINT_T0);
				_mm_prefetch(src+i_src,_MM_HINT_T0);
				_mm_prefetch(src+i_src*2,_MM_HINT_T0);
				for( x = 0; x < i_width; x+=8 )
				{
					//dst[x] = TAPFILTER( src, f );{ -1, 4, -10, 58, 17,  -5, 1,  0 }

					d=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+3*i_src)),zero);//*1
					tmp=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x)),zero);
					d=_mm_add_epi16(d,_mm_slli_epi16(tmp,5));
					tmp1=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x-1*i_src)),zero);
					temp=_mm_slli_epi16(_mm_sub_epi16(tmp,tmp1),1);
					d=_mm_add_epi16(d,temp);
					tmp1=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x-2*i_src)),zero);
					temp=_mm_add_epi16(temp,tmp1);
					tmp1=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+2*i_src)),zero);
					d=_mm_sub_epi16(d,tmp1);
					temp=_mm_sub_epi16(temp,tmp1);
					temp=_mm_slli_epi16(temp,2);
					d=_mm_add_epi16(d,temp);
					tmp1=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+i_src)),zero);
					d=_mm_add_epi16(d,tmp1);
					tmp=_mm_slli_epi16(_mm_add_epi16(tmp,tmp1),4);
					d=_mm_add_epi16(d,tmp);
					tmp1=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x-3*i_src)),zero);
					d=_mm_sub_epi16(d,tmp1);

					d=_mm_add_epi16(d,tmp2);//add 32
					d=_mm_srai_epi16(d,6);
					d=_mm_packus_epi16(d,d);

					_mm_storel_epi64((__m128i*)(dst+x),d);

				}
#else
				for( x = 0; x < i_width; x++ )
					dst[x] = lent_clip_uint8((TAPFILTER( src, f )+32)>>6);
#endif
				dst  += i_dst;
				src  += i_src;
			}
		}
		else if( fy == 2 )
		{
			for( y = 0; y < i_height; y++ )
			{
#if 1
				_mm_prefetch(src,_MM_HINT_T0);
				_mm_prefetch(src+i_src,_MM_HINT_T0);
				_mm_prefetch(src+i_src*2,_MM_HINT_T0);
				for( x = 0; x < i_width; x+=8 )
				{
					//dst[x] = TAPFILTER( src, f );{ -1, 4, -11, 40, 40, -11, 4, -1 }

					d=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x-3*i_src)),zero);
					tmp=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+4*i_src)),zero);
					d=_mm_add_epi16(tmp,d);

					temp=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x-2*i_src)),zero);
					tmp=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+3*i_src)),zero);
					tmp=_mm_slli_epi16(_mm_add_epi16(tmp,temp),2);
					d=_mm_sub_epi16(tmp,d);

					temp=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x-1*i_src)),zero);
					tmp=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+2*i_src)),zero);
					tmp=_mm_add_epi16(tmp,temp);
					d=_mm_sub_epi16(d,tmp);

					temp=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x)),zero);
					tmp1=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+i_src)),zero);
					tmp1=_mm_add_epi16(tmp1,temp);
					temp=_mm_sub_epi16(_mm_slli_epi16(tmp1,3),_mm_slli_epi16(tmp,1));
					d=_mm_add_epi16(temp,d);
					temp=_mm_slli_epi16(temp,2);
					d=_mm_add_epi16(temp,d);

					d=_mm_add_epi16(d,tmp2);//add 32
					d=_mm_srai_epi16(d,6);
					d=_mm_packus_epi16(d,d);

					_mm_storel_epi64((__m128i*)(dst+x),d);

				}
#else
				for( x = 0; x < i_width; x++ )
					dst[x] = lent_clip_uint8((TAPFILTER( src, f )+32)>>6);
#endif
				dst  += i_dst;
				src  += i_src;
			}
		}
		else //if( fy == 3 )
		{
			for( y = 0; y < i_height; y++ )
			{
#if 1
				_mm_prefetch(src,_MM_HINT_T0);
				_mm_prefetch(src+i_src,_MM_HINT_T0);
				_mm_prefetch(src+i_src*2,_MM_HINT_T0);
				for( x = 0; x < i_width; x+=8 )
				{
					//dst[x] = TAPFILTER( src, f );{  0, 1,  -5, 17, 58, -10, 4, -1 }

					d=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x-2*i_src)),zero);//*1
					tmp=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+i_src)),zero);
					d=_mm_add_epi16(d,_mm_slli_epi16(tmp,5));
					tmp1=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+2*i_src)),zero);
					temp=_mm_slli_epi16(_mm_sub_epi16(tmp,tmp1),1);
					d=_mm_add_epi16(d,temp);
					tmp1=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+3*i_src)),zero);
					temp=_mm_add_epi16(temp,tmp1);
					tmp1=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x-i_src)),zero);
					d=_mm_sub_epi16(d,tmp1);
					temp=_mm_sub_epi16(temp,tmp1);
					temp=_mm_slli_epi16(temp,2);
					d=_mm_add_epi16(d,temp);
					tmp1=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x)),zero);
					d=_mm_add_epi16(d,tmp1);
					tmp=_mm_slli_epi16(_mm_add_epi16(tmp,tmp1),4);
					d=_mm_add_epi16(d,tmp);
					tmp1=_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)(src+x+4*i_src)),zero);
					d=_mm_sub_epi16(d,tmp1);

					d=_mm_add_epi16(d,tmp2);//add 32
					d=_mm_srai_epi16(d,6);
					d=_mm_packus_epi16(d,d);

					_mm_storel_epi64((__m128i*)(dst+x),d);

				}
#else
				for( x = 0; x < i_width; x++ )
					dst[x] = lent_clip_uint8((TAPFILTER( src, f )+32)>>6);
#endif
				dst  += i_dst;
				src  += i_src;
			}
		}
	}
	else
	{//use hpx
		int16_t *hsrc = hpx[fx-1]+3*HPEL_SIZE /*+ i_offs*/;
		i_src=HPEL_SIZE;

		if( fy == 0 )
		{//加速
			for( y = 0; y < i_height; y++ )
			{

#if 1
				for( x = 0; x < i_width; x+=8 )
				{
					//dst[x] = lent_clip_uint8((hsrc[x]  + 32) >> 6);

					d=_mm_loadu_si128((__m128i*)(hsrc+x));
					d=_mm_add_epi16(d,tmp2);//add 32
					d=_mm_srai_epi16(d,6);
					d=_mm_packus_epi16(d,d);

					_mm_storel_epi64((__m128i*)(dst+x),d);//sse3

				}
#else
				for( x = 0; x < i_width; x++ )
					dst[x] = lent_clip_uint8((hsrc[x]  + 32) >> 6);
#endif
				dst  += i_dst;
				hsrc  += i_src;
			}
		}
		else if( fy == 1 )
		{
			tmp2=_mm_set1_epi32(2048);
			for( y = 0; y < i_height; y++ )
			{
#if 1
				for( x = 0; x < i_width; x+=4 )
				{
					//dst[x] = TAPFILTER( src, f )>>6;{ -1, 4, -10, 58, 17,  -5, 1,  0 }

					d=_mm_loadl_epi64((__m128i*)(hsrc+x+3*i_src));//*1
					d=_mm_unpacklo_epi16(d,_mm_cmplt_epi16(d,zero));
					tmp=_mm_loadl_epi64((__m128i*)(hsrc+x));
					tmp=_mm_unpacklo_epi16(tmp,_mm_cmplt_epi16(tmp,zero));
					d=_mm_add_epi32(d,_mm_slli_epi32(tmp,5));
					tmp1=_mm_loadl_epi64((__m128i*)(hsrc+x-1*i_src));
					tmp1=_mm_unpacklo_epi16(tmp1,_mm_cmplt_epi16(tmp1,zero));
					temp=_mm_slli_epi32(_mm_sub_epi32(tmp,tmp1),1);
					d=_mm_add_epi32(d,temp);
					tmp1=_mm_loadl_epi64((__m128i*)(hsrc+x-2*i_src));
					tmp1=_mm_unpacklo_epi16(tmp1,_mm_cmplt_epi16(tmp1,zero));
					temp=_mm_add_epi32(temp,tmp1);
					tmp1=_mm_loadl_epi64((__m128i*)(hsrc+x+2*i_src));
					tmp1=_mm_unpacklo_epi16(tmp1,_mm_cmplt_epi16(tmp1,zero));
					d=_mm_sub_epi32(d,tmp1);
					temp=_mm_sub_epi32(temp,tmp1);
					temp=_mm_slli_epi32(temp,2);
					d=_mm_add_epi32(d,temp);
					tmp1=_mm_loadl_epi64((__m128i*)(hsrc+x+i_src));
					tmp1=_mm_unpacklo_epi16(tmp1,_mm_cmplt_epi16(tmp1,zero));
					d=_mm_add_epi32(d,tmp1);
					tmp=_mm_slli_epi32(_mm_add_epi32(tmp,tmp1),4);
					d=_mm_add_epi32(d,tmp);
					tmp1=_mm_loadl_epi64((__m128i*)(hsrc+x-3*i_src));
					tmp1=_mm_unpacklo_epi16(tmp1,_mm_cmplt_epi16(tmp1,zero));
					d=_mm_sub_epi32(d,tmp1);

					d=_mm_add_epi32(d,tmp2);
					d=_mm_srai_epi32(d,12);


					tmp=_mm_cmpgt_epi32(d,zero);
					d=_mm_and_si128(d,tmp);

					tmp=_mm_cmpgt_epi32(d,max8bit);
					d=_mm_or_si128(d,tmp);
					d=_mm_and_si128(d,max8bit);

					d=_mm_packs_epi32(d,d);
					d=_mm_packus_epi16(d,d);

					*((int*)(dst+x))=_mm_cvtsi128_si32(d);

				}
#else
				for( x = 0; x < i_width; x++ )
					dst[x] = lent_clip_uint8((TAPFILTER( hsrc, f ) + ((1<<11))) >> 12);
#endif
				dst  += i_dst;
				hsrc  += i_src;
			}
		}
		else if( fy == 2 )
		{
			tmp2=_mm_set1_epi32(2048);
			for( y = 0; y < i_height; y++ )
			{
#if 1
				for( x = 0; x < i_width; x+=4 )
				{
					//dst[x] = TAPFILTER( hsrc, f )>>6;{ -1, 4, -11, 40, 40, -11, 4, -1 }

					d=_mm_loadl_epi64((__m128i*)(hsrc+x-3*i_src));
					d=_mm_unpacklo_epi16(d,_mm_cmplt_epi16(d,zero));
					tmp=_mm_loadl_epi64((__m128i*)(hsrc+x+4*i_src));
					tmp=_mm_unpacklo_epi16(tmp,_mm_cmplt_epi16(tmp,zero));
					d=_mm_add_epi32(tmp,d);

					temp=_mm_loadl_epi64((__m128i*)(hsrc+x-2*i_src));
					temp=_mm_unpacklo_epi16(temp,_mm_cmplt_epi16(temp,zero));
					tmp=_mm_loadl_epi64((__m128i*)(hsrc+x+3*i_src));
					tmp=_mm_unpacklo_epi16(tmp,_mm_cmplt_epi16(tmp,zero));
					tmp=_mm_slli_epi32(_mm_add_epi32(tmp,temp),2);
					d=_mm_sub_epi32(tmp,d);

					temp=_mm_loadl_epi64((__m128i*)(hsrc+x-1*i_src));
					temp=_mm_unpacklo_epi16(temp,_mm_cmplt_epi16(temp,zero));
					tmp=_mm_loadl_epi64((__m128i*)(hsrc+x+2*i_src));
					tmp=_mm_unpacklo_epi16(tmp,_mm_cmplt_epi16(tmp,zero));
					tmp=_mm_add_epi32(tmp,temp);
					d=_mm_sub_epi32(d,tmp);

					temp=_mm_loadl_epi64((__m128i*)(hsrc+x));
					temp=_mm_unpacklo_epi16(temp,_mm_cmplt_epi16(temp,zero));
					tmp1=_mm_loadl_epi64((__m128i*)(hsrc+x+i_src));
					tmp1=_mm_unpacklo_epi16(tmp1,_mm_cmplt_epi16(tmp1,zero));
					tmp1=_mm_add_epi32(tmp1,temp);
					temp=_mm_sub_epi32(_mm_slli_epi32(tmp1,3),_mm_slli_epi32(tmp,1));
					d=_mm_add_epi32(temp,d);
					temp=_mm_slli_epi32(temp,2);
					d=_mm_add_epi32(temp,d);

					d=_mm_add_epi32(d,tmp2);
					d=_mm_srai_epi32(d,12);


					tmp=_mm_cmpgt_epi32(d,zero);
					d=_mm_and_si128(d,tmp);

					tmp=_mm_cmpgt_epi32(d,max8bit);
					d=_mm_or_si128(d,tmp);
					d=_mm_and_si128(d,max8bit);

					d=_mm_packs_epi32(d,d);
					d=_mm_packus_epi16(d,d);

					*((int*)(dst+x))=_mm_cvtsi128_si32(d);

				}
#else
				for( x = 0; x < i_width; x++ )
					dst[x] = lent_clip_uint8((TAPFILTER( hsrc, f ) + ((1<<11))) >> 12);
#endif
				dst  += i_dst;
				hsrc  += i_src;
			}
		}
		else //if( fy == 3 )
		{
			tmp2=_mm_set1_epi32(2048);
			for( y = 0; y < i_height; y++ )
			{
#if 1
				for( x = 0; x < i_width; x+=4 )
				{
					//dst[x] = TAPFILTER( src, f )>>6;{  0, 1,  -5, 17, 58, -10, 4, -1 }

					d=_mm_loadl_epi64((__m128i*)(hsrc+x-2*i_src));//*1
					d=_mm_unpacklo_epi16(d,_mm_cmplt_epi16(d,zero));
					tmp=_mm_loadl_epi64((__m128i*)(hsrc+x+i_src));
					tmp=_mm_unpacklo_epi16(tmp,_mm_cmplt_epi16(tmp,zero));
					d=_mm_add_epi32(d,_mm_slli_epi32(tmp,5));
					tmp1=_mm_loadl_epi64((__m128i*)(hsrc+x+2*i_src));
					tmp1=_mm_unpacklo_epi16(tmp1,_mm_cmplt_epi16(tmp1,zero));
					temp=_mm_slli_epi32(_mm_sub_epi32(tmp,tmp1),1);
					d=_mm_add_epi32(d,temp);
					tmp1=_mm_loadl_epi64((__m128i*)(hsrc+x+3*i_src));
					tmp1=_mm_unpacklo_epi16(tmp1,_mm_cmplt_epi16(tmp1,zero));
					temp=_mm_add_epi32(temp,tmp1);
					tmp1=_mm_loadl_epi64((__m128i*)(hsrc+x-i_src));
					tmp1=_mm_unpacklo_epi16(tmp1,_mm_cmplt_epi16(tmp1,zero));
					d=_mm_sub_epi32(d,tmp1);
					temp=_mm_sub_epi32(temp,tmp1);
					temp=_mm_slli_epi32(temp,2);
					d=_mm_add_epi32(d,temp);
					tmp1=_mm_loadl_epi64((__m128i*)(hsrc+x));
					tmp1=_mm_unpacklo_epi16(tmp1,_mm_cmplt_epi16(tmp1,zero));
					d=_mm_add_epi32(d,tmp1);
					tmp=_mm_slli_epi32(_mm_add_epi32(tmp,tmp1),4);
					d=_mm_add_epi32(d,tmp);
					tmp1=_mm_loadl_epi64((__m128i*)(hsrc+x+4*i_src));
					tmp1=_mm_unpacklo_epi16(tmp1,_mm_cmplt_epi16(tmp1,zero));
					d=_mm_sub_epi32(d,tmp1);

					d=_mm_add_epi32(d,tmp2);
					d=_mm_srai_epi32(d,12);


					tmp=_mm_cmpgt_epi32(d,zero);
					d=_mm_and_si128(d,tmp);

					tmp=_mm_cmpgt_epi32(d,max8bit);
					d=_mm_or_si128(d,tmp);
					d=_mm_and_si128(d,max8bit);

					d=_mm_packs_epi32(d,d);
					d=_mm_packus_epi16(d,d);

					*((int*)(dst+x))=_mm_cvtsi128_si32(d);

				}
#else
				for( x = 0; x < i_width; x++ )
					dst[x] = lent_clip_uint8((TAPFILTER( hsrc, f ) + ((1<<11))) >> 12);
#endif
				dst  += i_dst;
				hsrc  += i_src;
			}
		}
	}
	////_mm_empty();
}
#undef TAPFILTER

#include "MC_sse2.c"

/////////////////////////////////////////////////////////////////////////////////////////////////
//反量化
/////////////////////////////////////////////////////////////////////////////////////////////////
//dequant tables
static const int i_dequant_scales[6] =
{
	40,45,51,57,64,72
};

static void dequant_sse2( dctcoef *dct, int qp, int i_log2_size)
{
	int i;
	int i_shift = i_log2_size - 1;
	int i_size = (1<<(i_log2_size<<1));
	int mf=i_dequant_scales[qp%6];
	__m128i d[2],f,b,tmp;
	////_mm_empty();
	f=_mm_set1_epi16(i_dequant_scales[qp%6]<<(qp/6));
	b=_mm_set1_epi32(1 << (i_shift - 1));
	tmp=_mm_set1_epi16(1);
	for( i = 0; i < i_size; i +=8 )
	{
		d[0]=_mm_load_si128(dct+i);

		d[1]=_mm_mulhi_epi16(d[0],f);
		d[0]=_mm_mullo_epi16(d[0],f);

		tmp=d[0];
		d[0]=_mm_unpacklo_epi16(d[0],d[1]);
		d[1]=_mm_unpackhi_epi16(tmp,d[1]);


		d[0]=_mm_add_epi32(d[0],b);
		d[0]=_mm_srai_epi32(d[0],i_shift);

		d[1]=_mm_add_epi32(d[1],b);
		d[1]=_mm_srai_epi32(d[1],i_shift);

		d[0]=_mm_packs_epi32(d[0],d[1]);

		_mm_store_si128(dct+i,d[0]);
	}
	////_mm_empty();
}
static void dequant_sse4( dctcoef *dct, int qp, int i_log2_size)
{
	int i;
	int i_shift = i_log2_size - 1;
	int i_size = (1<<(i_log2_size<<1));
	int mf=i_dequant_scales[qp%6];
	__m128i d[2],f,b,tmp;
	////_mm_empty();
	f=_mm_set1_epi32(i_dequant_scales[qp%6]<<(qp/6));
	b=_mm_set1_epi32(1 << (i_shift - 1));
	tmp=_mm_set1_epi16(1);
	for( i = 0; i < i_size; i +=8 )
	{

		d[0]=_mm_load_si128(dct+i);

		d[1]=_mm_cvtepi16_epi32(_mm_shuffle_epi32(d[0],_MM_SHUFFLE(0,0,3,2)));
		d[0]=_mm_cvtepi16_epi32(d[0]);

		d[0]=_mm_mullo_epi32(d[0],f);
		d[1]=_mm_mullo_epi32(d[1],f);

		d[0]=_mm_add_epi32(d[0],b);
		d[1]=_mm_add_epi32(d[1],b);

		d[0]=_mm_srai_epi32(d[0],i_shift);
		d[1]=_mm_srai_epi32(d[1],i_shift);

		d[0]=_mm_packs_epi32(d[0],d[1]);

		_mm_store_si128(dct+i,d[0]);

	}
	////_mm_empty();
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//初始化对外接口
/////////////////////////////////////////////////////////////////////////////////////////////////
void dsp_init_x86(DSPContext *dsp,unsigned int sse)
{
	if ( sse >= 20 )
	{
		dsp->add32x32_idct = add32x32_idct_sse2;
		dsp->add16x16_idct = add16x16_idct_sse2;
		dsp->add8x8_idct = add8x8_idct_sse2;
		dsp->add4x4_idct = add4x4_idct_sse2;
		dsp->hpel_filter = hpel_filter_sse2;
		dsp->get_ref_chroma = get_ref_chroma_sse2;
		dsp->mc_chroma = mc_chroma_sse2;
		dsp->get_ref = get_ref_sse2_shushuang;
		dsp->mc_avg = mc_avg_sse2;
		dsp->mc_luma = mc_luma_sse2;
		dsp->dequant = dequant_sse2;
	}
	if(sse>=31)
	{
		// 目前科吉的 SSE2 版本比之前的 SSSE3 版本快
		dsp->add32x32_idct = add32x32_idct_sse2;
		dsp->add16x16_idct = add16x16_idct_sse2;
		dsp->add8x8_idct = add8x8_idct_sse2;
		dsp->add4x4_idct = add4x4_idct_sse2;
		dsp->hpel_filter = hpel_filter_ssse3_2;
		dsp->get_ref_chroma = get_ref_chroma_ssse3_shushuang;
		dsp->mc_chroma = mc_chroma_ssse3;
		dsp->get_ref = get_ref_sse2_shushuang;
		dsp->mc_avg = mc_avg_sse2;
		dsp->mc_luma = mc_luma_sse2;
		dsp->dequant = dequant_sse2;
	}
	if(sse>=41)
	{
		dsp->get_ref_chroma = get_ref_chroma_sse4_shushuang;
		dsp->mc_chroma = mc_chroma_sse4;
		dsp->get_ref = get_ref_sse4_shushuang_03;
		dsp->mc_avg = mc_avg_sse4;
		dsp->mc_luma = mc_luma_sse4;
		dsp->dequant = dequant_sse4;
	}
}

#endif