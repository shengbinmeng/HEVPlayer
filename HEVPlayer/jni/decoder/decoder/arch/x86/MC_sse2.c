#include <tmmintrin.h>

#define BUTTERFLY_R2R( dst, src, bit, i1, i2 ) \
	dst[i1] = _mm_unpacklo_epi##bit( src[i1], src[i2] ); \
	dst[i2] = _mm_unpackhi_epi##bit( src[i1], src[i2] )
//注意：该处行号不对
#define TRANSPOSE8_R2R_SSE2( dst, src ) \
	BUTTERFLY_R2R( dst, src, 16, 0, 1 ); \
	BUTTERFLY_R2R( dst, src, 16, 2, 3 ); \
	BUTTERFLY_R2R( dst, src, 16, 4, 5 ); \
	BUTTERFLY_R2R( dst, src, 16, 6, 7 ); \
	BUTTERFLY_R2R( src, dst, 32, 0, 2 ); \
	BUTTERFLY_R2R( src, dst, 32, 1, 3 ); \
	BUTTERFLY_R2R( src, dst, 32, 4, 6 ); \
	BUTTERFLY_R2R( src, dst, 32, 5, 7 ); \
	BUTTERFLY_R2R( dst, src, 64, 0, 4 ); \
	BUTTERFLY_R2R( dst, src, 64, 1, 5 ); \
	BUTTERFLY_R2R( dst, src, 64, 2, 6 ); \
	BUTTERFLY_R2R( dst, src, 64, 3, 7 )

static void mc_chroma_sse2_core16( int16_t *dst, int i_dst_stride, int16_t *buf, int fy,
																int i_width, int i_height )
{
	__m128i r0, r1, r2, r3;
	int x, y;

	int16_t *stmp = dst;
	int16_t *btmp = buf;

	if( fy == 1 ) //{ -2, 58, 10, -2 },
	{
		for( y = 0; y < i_height; y ++ )
		{
			for( x = 0; x < i_width; x += 8 )
			{
				int16_t *ss = btmp + x;

				r1 = _mm_load_si128( (__m128i *)(ss + 1*48) );
				r0 = _mm_add_epi16( _mm_slli_epi16( r1, 5 ), _mm_slli_epi16( r1, 4 ) );
				r2 = _mm_load_si128( (__m128i *)(ss + 2*48) );
				r1 = _mm_add_epi16( r1, r2 );
				r0 = _mm_add_epi16( r0, _mm_slli_epi16( r1, 3 ) );
				r2 = _mm_load_si128( (__m128i *)(ss + 0*48) );
				r1 = _mm_sub_epi16( r1, r2 );
				r2 = _mm_load_si128( (__m128i *)(ss + 3*48) );
				r1 = _mm_sub_epi16( r1, r2 );
				r0 = _mm_add_epi16( r0, _mm_slli_epi16( r1, 1 ) );

				_mm_store_si128( (__m128i *)(stmp + x), r0 );
			}
			stmp += i_dst_stride;
			btmp += 48;
		}
	}
	else if( fy == 7 ) //{ -2, 10, 58, -2 }
	{
		for( y = 0; y < i_height; y ++ )
		{
			for( x = 0; x < i_width; x += 8 )
			{
				int16_t *ss = btmp + x;

				r1 = _mm_load_si128( (__m128i *)(ss + 2*48) );
				r0 = _mm_add_epi16( _mm_slli_epi16( r1, 5 ), _mm_slli_epi16( r1, 4 ) );
				r2 = _mm_load_si128( (__m128i *)(ss + 1*48) );
				r1 = _mm_add_epi16( r1, r2 );
				r0 = _mm_add_epi16( r0, _mm_slli_epi16( r1, 3 ) );
				r2 = _mm_load_si128( (__m128i *)(ss + 3*48) );
				r1 = _mm_sub_epi16( r1, r2 );
				r2 = _mm_load_si128( (__m128i *)(ss + 0*48) );
				r1 = _mm_sub_epi16( r1, r2 );
				r0 = _mm_add_epi16( r0, _mm_slli_epi16( r1, 1 ) );

				_mm_store_si128( (__m128i *)(stmp + x), r0 );
			}
			stmp += i_dst_stride;
			btmp += 48;
		}
	}
	else if( fy == 2 ) //{ -4, 54, 16, -2 },
	{
		for( y = 0; y < i_height; y ++ )
		{
			for( x = 0; x < i_width; x += 8 )
			{
				int16_t *ss = btmp + x;

				r1 = _mm_load_si128( (__m128i *)(ss + 1*48) );
				r0 = _mm_slli_epi16( r1, 5 );
				r2 = _mm_load_si128( (__m128i *)(ss + 2*48) );
				r2 = _mm_add_epi16( r1, r2 );
				r0 = _mm_add_epi16( r0, _mm_slli_epi16( r2, 4 ) );
				r2 = _mm_load_si128( (__m128i *)(ss + 0*48) );
				r2 = _mm_sub_epi16( r1, r2 );
				r0 = _mm_add_epi16( r0, _mm_slli_epi16( r2, 2 ) );
				r2 = _mm_load_si128( (__m128i *)(ss + 3*48) );
				r2 = _mm_sub_epi16( r1, r2 );
				r0 = _mm_add_epi16( r0, _mm_slli_epi16( r2, 1 ) );

				_mm_store_si128( (__m128i *)(stmp + x), r0 );
			}
			stmp += i_dst_stride;
			btmp += 48;
		}
	}
	else if( fy == 6 ) //{ -2, 16, 54, -4 },
	{
		for( y = 0; y < i_height; y ++ )
		{
			for( x = 0; x < i_width; x += 8 )
			{
				int16_t *ss = btmp + x;

				r1 = _mm_load_si128( (__m128i *)(ss + 2*48) );
				r0 = _mm_slli_epi16( r1, 5 );
				r2 = _mm_load_si128( (__m128i *)(ss + 1*48) );
				r2 = _mm_add_epi16( r1, r2 );
				r0 = _mm_add_epi16( r0, _mm_slli_epi16( r2, 4 ) );
				r2 = _mm_load_si128( (__m128i *)(ss + 3*48) );
				r2 = _mm_sub_epi16( r1, r2 );
				r0 = _mm_add_epi16( r0, _mm_slli_epi16( r2, 2 ) );
				r2 = _mm_load_si128( (__m128i *)(ss + 0*48) );
				r2 = _mm_sub_epi16( r1, r2 );
				r0 = _mm_add_epi16( r0, _mm_slli_epi16( r2, 1 ) );

				_mm_store_si128( (__m128i *)(stmp + x), r0 );
			}
			stmp += i_dst_stride;
			btmp += 48;
		}
	}
	else if( fy == 3 ) //{ -6, 46, 28, -4 },
	{
		for( y = 0; y < i_height; y ++ )
		{
			for( x = 0; x < i_width; x += 8 )
			{
				int16_t *ss = btmp + x;

				r1 = _mm_load_si128( (__m128i *)(ss + 1*48) );
				r0 = _mm_slli_epi16( r1, 3 );
				r2 = _mm_load_si128( (__m128i *)(ss + 0*48) );
				r3 = _mm_sub_epi16( r1, r2 );
				r0 = _mm_add_epi16( r0, _mm_slli_epi16( r3, 1 ) );
				r2 = _mm_load_si128( (__m128i *)(ss + 2*48) );
				r0 = _mm_add_epi16( r0, _mm_slli_epi16( _mm_add_epi16( r1, r2 ), 5 ) );
				r3 = _mm_sub_epi16( r3, r2 );
				r2 = _mm_load_si128( (__m128i *)(ss + 3*48) );
				r1 = _mm_sub_epi16( r3, r2 );
				r0 = _mm_add_epi16( r0, _mm_slli_epi16( r1, 2 ) );

				_mm_store_si128( (__m128i *)(stmp + x), r0 );
			}
			stmp += i_dst_stride;
			btmp += 48;
		}
	}
	else if( fy == 5 ) //{ -4, 28, 46, -6 },
	{
		for( y = 0; y < i_height; y ++ )
		{
			for( x = 0; x < i_width; x += 8 )
			{
				int16_t *ss = btmp + x;

				r1 = _mm_load_si128( (__m128i *)(ss + 2*48) );
				r0 = _mm_slli_epi16( r1, 3 );
				r2 = _mm_load_si128( (__m128i *)(ss + 3*48) );
				r3 = _mm_sub_epi16( r1, r2 );
				r0 = _mm_add_epi16( r0, _mm_slli_epi16( r3, 1 ) );
				r2 = _mm_load_si128( (__m128i *)(ss + 1*48) );
				r0 = _mm_add_epi16( r0, _mm_slli_epi16( _mm_add_epi16( r1, r2 ), 5 ) );
				r3 = _mm_sub_epi16( r3, r2 );
				r2 = _mm_load_si128( (__m128i *)(ss + 0*48) );
				r1 = _mm_sub_epi16( r3, r2 );
				r0 = _mm_add_epi16( r0, _mm_slli_epi16( r1, 2 ) );

				_mm_store_si128( (__m128i *)(stmp + x), r0 );
			}
			stmp += i_dst_stride;
			btmp += 48;
		}
	}
	else //if( fy == 4 ) //{ -4, 36, 36, -4 },
	{
		for( y = 0; y < i_height; y ++ )
		{
			for( x = 0; x < i_width; x += 8 )
			{
				int16_t *ss = btmp + x;

				r1 = _mm_load_si128( (__m128i *)(ss + 1*48) );
				r2 = _mm_load_si128( (__m128i *)(ss + 2*48) );
				r1 = _mm_add_epi16( r1, r2 );
				r0 = _mm_slli_epi16( r1, 5 );

				r2 = _mm_load_si128( (__m128i *)(ss + 0*48) );
				r1 = _mm_sub_epi16( r1, r2 );
				r2 = _mm_load_si128( (__m128i *)(ss + 3*48) );
				r1 = _mm_sub_epi16( r1, r2 );
				r0 = _mm_add_epi16( r0, _mm_slli_epi16( r1, 2 ) );

				_mm_store_si128( (__m128i *)(stmp + x), r0 );
			}
			stmp += i_dst_stride;
			btmp += 48;
		}
	}
}

static void mc_chroma_sse2_core32( int32_t *dst, int i_dst_stride, int16_t *buf, int fy,
																	int i_width, int i_height )
{
	__m128i r0, r1, r2, r3, zero = _mm_setzero_si128(), sign, ri;
	int x, y;

	int32_t *stmp = dst;
	int16_t *btmp = buf;

#define LOAD( rdst, src ) \
	ri = _mm_loadl_epi64( (__m128i *)(src) ); \
	sign = _mm_cmplt_epi16( ri, zero ); \
	rdst = _mm_unpacklo_epi16( ri, sign )

	if( fy == 1 ) //{ -2, 58, 10, -2 },
	{
		for( y = 0; y < i_height; y ++ )
		{
			for( x = 0; x < i_width; x += 4 )
			{
				int16_t *ss = btmp + x;

				LOAD( r1, ss + 1*48 );
				r0 = _mm_add_epi32( _mm_slli_epi32( r1, 5 ), _mm_slli_epi32( r1, 4 ) );
				LOAD( r2, ss + 2*48 );
				r1 = _mm_add_epi32( r1, r2 );
				r0 = _mm_add_epi32( r0, _mm_slli_epi32( r1, 3 ) );
				LOAD( r2, ss + 0*48 );
				r1 = _mm_sub_epi32( r1, r2 );
				LOAD( r2, ss + 3*48 );
				r1 = _mm_sub_epi32( r1, r2 );
				r0 = _mm_add_epi32( r0, _mm_slli_epi32( r1, 1 ) );

				_mm_store_si128( (__m128i *)(stmp + x), r0 );
			}
			stmp += i_dst_stride;
			btmp += 48;
		}
	}
	else if( fy == 7 ) //{ -2, 10, 58, -2 }
	{
		for( y = 0; y < i_height; y ++ )
		{
			for( x = 0; x < i_width; x += 4 )
			{
				int16_t *ss = btmp + x;

				LOAD( r1, ss + 2*48 );
				r0 = _mm_add_epi32( _mm_slli_epi32( r1, 5 ), _mm_slli_epi32( r1, 4 ) );
				LOAD( r2, ss + 1*48 );
				r1 = _mm_add_epi32( r1, r2 );
				r0 = _mm_add_epi32( r0, _mm_slli_epi32( r1, 3 ) );
				LOAD( r2, ss + 3*48 );
				r1 = _mm_sub_epi32( r1, r2 );
				LOAD( r2, ss + 0*48 );
				r1 = _mm_sub_epi32( r1, r2 );
				r0 = _mm_add_epi32( r0, _mm_slli_epi32( r1, 1 ) );

				_mm_store_si128( (__m128i *)(stmp + x), r0 );
			}
			stmp += i_dst_stride;
			btmp += 48;
		}
	}
	else if( fy == 2 ) //{ -4, 54, 16, -2 },
	{
		for( y = 0; y < i_height; y ++ )
		{
			for( x = 0; x < i_width; x += 4 )
			{
				int16_t *ss = btmp + x;

				LOAD( r1, ss + 1*48 );
				r0 = _mm_slli_epi32( r1, 5 );
				LOAD( r2, ss + 2*48 );
				r2 = _mm_add_epi32( r1, r2 );
				r0 = _mm_add_epi32( r0, _mm_slli_epi32( r2, 4 ) );
				LOAD( r2, ss + 0*48 );
				r2 = _mm_sub_epi32( r1, r2 );
				r0 = _mm_add_epi32( r0, _mm_slli_epi32( r2, 2 ) );
				LOAD( r2, ss + 3*48 );
				r2 = _mm_sub_epi32( r1, r2 );
				r0 = _mm_add_epi32( r0, _mm_slli_epi32( r2, 1 ) );

				_mm_store_si128( (__m128i *)(stmp + x), r0 );
			}
			stmp += i_dst_stride;
			btmp += 48;
		}
	}
	else if( fy == 6 ) //{ -2, 16, 54, -4 },
	{
		for( y = 0; y < i_height; y ++ )
		{
			for( x = 0; x < i_width; x += 4 )
			{
				int16_t *ss = btmp + x;

				LOAD( r1, ss + 2*48 );
				r0 = _mm_slli_epi32( r1, 5 );
				LOAD( r2, ss + 1*48 );
				r2 = _mm_add_epi32( r1, r2 );
				r0 = _mm_add_epi32( r0, _mm_slli_epi32( r2, 4 ) );
				LOAD( r2, ss + 3*48 );
				r2 = _mm_sub_epi32( r1, r2 );
				r0 = _mm_add_epi32( r0, _mm_slli_epi32( r2, 2 ) );
				LOAD( r2, ss + 0*48 );
				r2 = _mm_sub_epi32( r1, r2 );
				r0 = _mm_add_epi32( r0, _mm_slli_epi32( r2, 1 ) );

				_mm_store_si128( (__m128i *)(stmp + x), r0 );
			}
			stmp += i_dst_stride;
			btmp += 48;
		}
	}
	else if( fy == 3 ) //{ -6, 46, 28, -4 },
	{
		for( y = 0; y < i_height; y ++ )
		{
			for( x = 0; x < i_width; x += 4 )
			{
				int16_t *ss = btmp + x;

				LOAD( r1, ss + 1*48 );
				r0 = _mm_slli_epi32( r1, 3 );
				LOAD( r2, ss + 0*48 );
				r3 = _mm_sub_epi32( r1, r2 );
				r0 = _mm_add_epi32( r0, _mm_slli_epi32( r3, 1 ) );
				LOAD( r2, ss + 2*48 );
				r0 = _mm_add_epi32( r0, _mm_slli_epi32( _mm_add_epi32( r1, r2 ), 5 ) );
				r3 = _mm_sub_epi32( r3, r2 );
				LOAD( r2, ss + 3*48 );
				r1 = _mm_sub_epi32( r3, r2 );
				r0 = _mm_add_epi32( r0, _mm_slli_epi32( r1, 2 ) );

				_mm_store_si128( (__m128i *)(stmp + x), r0 );
			}
			stmp += i_dst_stride;
			btmp += 48;
		}
	}
	else if( fy == 5 ) //{ -4, 28, 46, -6 },
	{
		for( y = 0; y < i_height; y ++ )
		{
			for( x = 0; x < i_width; x += 4 )
			{
				int16_t *ss = btmp + x;

				LOAD( r1, ss + 2*48 );
				r0 = _mm_slli_epi32( r1, 3 );
				LOAD( r2, ss + 3*48 );
				r3 = _mm_sub_epi32( r1, r2 );
				r0 = _mm_add_epi32( r0, _mm_slli_epi32( r3, 1 ) );
				LOAD( r2, ss + 1*48 );
				r0 = _mm_add_epi32( r0, _mm_slli_epi32( _mm_add_epi32( r1, r2 ), 5 ) );
				r3 = _mm_sub_epi32( r3, r2 );
				LOAD( r2, ss + 0*48 );
				r1 = _mm_sub_epi32( r3, r2 );
				r0 = _mm_add_epi32( r0, _mm_slli_epi32( r1, 2 ) );

				_mm_store_si128( (__m128i *)(stmp + x), r0 );
			}
			stmp += i_dst_stride;
			btmp += 48;
		}
	}
	else //if( fy == 4 ) //{ -4, 36, 36, -4 },
	{
		for( y = 0; y < i_height; y ++ )
		{
			for( x = 0; x < i_width; x += 4 )
			{
				int16_t *ss = btmp + x;

				LOAD( r1, ss + 1*48 );
				LOAD( r2, ss + 2*48 );
				r1 = _mm_add_epi32( r1, r2 );
				r0 = _mm_slli_epi32( r1, 5 );

				LOAD( r2, ss + 0*48 );
				r1 = _mm_sub_epi32( r1, r2 );
				LOAD( r2, ss + 3*48 );
				r1 = _mm_sub_epi32( r1, r2 );
				r0 = _mm_add_epi32( r0, _mm_slli_epi32( r1, 2 ) );

				_mm_store_si128( (__m128i *)(stmp + x), r0 );
			}
			stmp += i_dst_stride;
			btmp += 48;
		}
	}

#undef LOAD
}

void mc_chroma_sse2( pixel *dst, int i_dst, pixel *src, int i_src,
										int mvx, int mvy, int i_width, int i_height )
{
	__declspec(align(16)) int16_t buf[48*48];
	__declspec(align(16)) int16_t dst_buf[48*48];
	int fx = mvx&0x7;
	int fy = mvy&0x7;
	const int w4 = i_width + 4, h4 = i_height + 4; 

	src += (mvy >> 3) * i_src + (mvx >> 3);
	if( fy == 0 && fx == 0 ) 
	{
		mc_copy( dst, i_dst, src, i_src, i_width, i_height );
	}
	else
	{
		int x, y, colume;
		pixel *stmp;
		int16_t *btmp;
		int ww, hh; //size of interpolation

		if( fx == 0 ) //unpack
		{
			__m128i r0, r1, r2 = _mm_setzero_si128();

			stmp = src - i_src;
			btmp = buf;
			for( y = 0; y < h4; y ++ )
			{
				for( x = 0; x < i_width; x += 8 )
				{
					r0 = _mm_loadl_epi64( (__m128i *)(stmp + x) );
					r1 = _mm_unpacklo_epi8( r0, r2 );
					_mm_store_si128( (__m128i *)(btmp + x), r1 );
				}
				stmp += i_src;
				btmp += 48;
			}

			colume = fy;
			ww = i_width;
			hh = i_height;
		}
		else //unpack and transpose
		{
			__m128i zero = _mm_setzero_si128();

			ww = fy ? h4 : i_height;
			hh = i_width;

			stmp = src - (fy ? i_src : 0) - 1;
			btmp = buf;
			for( y = 0; y < ww; y += 8 )
			{
				for( x = 0; x < w4; x += 8 )
				{
					__m128i s[8], d[8];
					pixel *ss = stmp + x;
					int16_t *dd = btmp + x*48;

#define LOAD( i ) \
	s[i] = _mm_loadl_epi64( (__m128i *)(ss + i*i_src) ); \
	s[i] = _mm_unpacklo_epi8( s[i], zero )

					LOAD( 0 );
					LOAD( 1 );
					LOAD( 2 );
					LOAD( 3 );
					LOAD( 4 );
					LOAD( 5 );
					LOAD( 6 );
					LOAD( 7 );

#undef  LOAD

					TRANSPOSE8_R2R_SSE2( d, s );

					_mm_store_si128( (__m128i *)(dd + 0*48), d[0] );
					_mm_store_si128( (__m128i *)(dd + 1*48), d[4] );
					_mm_store_si128( (__m128i *)(dd + 2*48), d[2] );
					_mm_store_si128( (__m128i *)(dd + 3*48), d[6] );
					_mm_store_si128( (__m128i *)(dd + 4*48), d[1] );
					_mm_store_si128( (__m128i *)(dd + 5*48), d[5] );
					_mm_store_si128( (__m128i *)(dd + 6*48), d[3] );
					_mm_store_si128( (__m128i *)(dd + 7*48), d[7] );
				}
				stmp += (i_src<<3);
				btmp += 8;
			}

			colume = fx;
		}

		//interpolation
		mc_chroma_sse2_core16( dst_buf, 48, buf, colume, ww, hh );

		if( fx == 0 )
		{
			__m128i bias;

			btmp = dst_buf;
			stmp = dst;
			bias = _mm_set1_epi16( 32 );
			for( y = 0; y < i_height; y ++ )
			{
				for( x = 0; x < i_width; x += 8 )
				{
					__m128i r0, r1;

					r0 = _mm_load_si128( (__m128i *)(btmp + x) );
					r1 = _mm_add_epi16( r0, bias );
					r0 = _mm_srai_epi16( r1, 6 );
					r1 = _mm_packus_epi16( r0, r0 );
					_mm_storel_epi64( (__m128i *)(stmp + x), r1 );
				}
				btmp += 48;
				stmp += i_dst;
			}
		}
		else if( fy == 0 )//transpose and pack
		{
			__m128i bias;

			bias = _mm_set1_epi16( 32 );
			for( y = 0; y < i_height; y += 8 )
			{
				for( x = 0; x < i_width; x += 8 )
				{
					__m128i s[8], d[8];
					int16_t *ss = dst_buf + x * 48 + y;
					pixel *dd = dst + x + y * i_dst;

#define LOAD( i ) \
	s[i] = _mm_load_si128( (__m128i *)(ss + i*48) )

					LOAD( 0 );
					LOAD( 1 );
					LOAD( 2 );
					LOAD( 3 );
					LOAD( 4 );
					LOAD( 5 );
					LOAD( 6 );
					LOAD( 7 );

#undef  LOAD

					TRANSPOSE8_R2R_SSE2( d, s );

#define STORE( idx, i ) \
	d[i] = _mm_add_epi16( d[i], bias ); \
	s[i] = _mm_srai_epi16( d[i], 6 ); \
	d[i] = _mm_packus_epi16( s[i], s[i] );\
	_mm_storel_epi64( (__m128i *)(dd + idx*i_dst), d[i] )

					STORE( 0, 0 );
					STORE( 1, 4 );
					STORE( 2, 2 );
					STORE( 3, 6 );
					STORE( 4, 1 );
					STORE( 5, 5 );
					STORE( 6, 3 );
					STORE( 7, 7 );

#undef  STORE
				}
			}
		}
		else
		{
			//transpose to buf
			for( y = 0; y < h4; y += 8 )
			{
				for( x = 0; x < i_width; x += 8 )
				{
					__m128i s[8], d[8];
					int16_t *ss = dst_buf + x * 48 + y;
					int16_t *dd = buf + x + y * 48;

#define LOAD( i ) \
	s[i] = _mm_load_si128( (__m128i *)(ss + i*48) )

					LOAD( 0 );
					LOAD( 1 );
					LOAD( 2 );
					LOAD( 3 );
					LOAD( 4 );
					LOAD( 5 );
					LOAD( 6 );
					LOAD( 7 );

#undef  LOAD

					TRANSPOSE8_R2R_SSE2( d, s );

#define STORE( idx, i ) \
	_mm_store_si128( (__m128i *)(dd + idx*48), d[i] )

					STORE( 0, 0 );
					STORE( 1, 4 );
					STORE( 2, 2 );
					STORE( 3, 6 );
					STORE( 4, 1 );
					STORE( 5, 5 );
					STORE( 6, 3 );
					STORE( 7, 7 );

#undef  STORE
				}
			}

			//32 * 32 * 2 < 48 * 48，用dst_buf
			mc_chroma_sse2_core32( (int32_t *)dst_buf, 32, buf, fy, i_width, i_height );

			{
				__m128i bias;

				btmp = dst_buf;
				stmp = dst;
				bias = _mm_set1_epi32( 1<<11 );
				for( y = 0; y < i_height; y ++ )
				{
					for( x = 0; x < i_width; x += 8 )
					{
						__m128i r0, r1, r2;
						int16_t *ss = btmp + (x<<1);

						r0 = _mm_load_si128( (__m128i *)(ss + 0) );
						r1 = _mm_load_si128( (__m128i *)(ss + 8) );
						r0 = _mm_add_epi32( r0, bias );
						r1 = _mm_add_epi32( r1, bias );
						r0 = _mm_srai_epi32( r0, 12 );
						r1 = _mm_srai_epi32( r1, 12 );
						r2 = _mm_packs_epi32( r0, r1 );
						r1 = _mm_packus_epi16( r2, r2 );
						_mm_storel_epi64( (__m128i *)(stmp + x), r1 );
					}
					btmp += 32*2;
					stmp += i_dst;
				}
			}
		}
	}
}

void get_ref_chroma_sse2( int16_t *dst, int i_dst, pixel *src, int i_src,
												 int mvx, int mvy, int i_width, int i_height )
{
	__declspec(align(16)) int16_t buf[48*48];
	__declspec(align(16)) int16_t dst_buf[48*48];
	int fx = mvx&0x7;
	int fy = mvy&0x7;
	const int w4 = i_width + 4, h4 = i_height + 4; 

	src += (mvy >> 3) * i_src + (mvx >> 3);
	if( fy == 0 && fx == 0 ) //unpack to dst
	{
		int x, y;
		pixel *ss;
		int16_t *dd;
		__m128i zero = _mm_setzero_si128();

		ss = src;
		dd = dst;
		for( y = 0; y < i_height; y ++ )
		{
			for( x = 0; x < i_width; x += 8 )
			{
				__m128i r0, r1;

				r0 = _mm_loadl_epi64( (__m128i *)(ss + x) );
				r1 = _mm_unpacklo_epi8( r0, zero );
				r0 = _mm_slli_epi16( r1, 6 );
				_mm_store_si128( (__m128i *)(dd + x), r0 );
			}
			ss += i_src;
			dd += i_dst;
		}
	}
	else
	{
		int x, y, colume, i_dst_stride;
		pixel *stmp;
		int16_t *btmp, *dtmp;
		int ww, hh; //size of interpolation

		if( fx == 0 ) //unpack
		{
			__m128i r0, r1, r2 = _mm_setzero_si128();

			stmp = src - i_src;
			btmp = buf;
			for( y = 0; y < h4; y ++ )
			{
				for( x = 0; x < i_width; x += 8 )
				{
					r0 = _mm_loadl_epi64( (__m128i *)(stmp + x) );
					r1 = _mm_unpacklo_epi8( r0, r2 );
					_mm_store_si128( (__m128i *)(btmp + x), r1 );
				}
				stmp += i_src;
				btmp += 48;
			}

			colume = fy;
			ww = i_width;
			hh = i_height;
			dtmp = dst;
			i_dst_stride = i_dst;
		}
		else //unpack and transpose
		{
			__m128i zero = _mm_setzero_si128();

			ww = fy ? h4 : i_height;
			hh = i_width;

			stmp = src - (fy ? i_src : 0) - 1;
			btmp = buf;
			for( y = 0; y < ww; y += 8 )
			{
				for( x = 0; x < w4; x += 8 )
				{
					__m128i s[8], d[8];
					pixel *ss = stmp + x;
					int16_t *dd = btmp + x*48;

#define LOAD( i ) \
	s[i] = _mm_loadl_epi64( (__m128i *)(ss + i*i_src) ); \
	s[i] = _mm_unpacklo_epi8( s[i], zero )

					LOAD( 0 );
					LOAD( 1 );
					LOAD( 2 );
					LOAD( 3 );
					LOAD( 4 );
					LOAD( 5 );
					LOAD( 6 );
					LOAD( 7 );

#undef  LOAD

					TRANSPOSE8_R2R_SSE2( d, s );

					_mm_store_si128( (__m128i *)(dd + 0*48), d[0] );
					_mm_store_si128( (__m128i *)(dd + 1*48), d[4] );
					_mm_store_si128( (__m128i *)(dd + 2*48), d[2] );
					_mm_store_si128( (__m128i *)(dd + 3*48), d[6] );
					_mm_store_si128( (__m128i *)(dd + 4*48), d[1] );
					_mm_store_si128( (__m128i *)(dd + 5*48), d[5] );
					_mm_store_si128( (__m128i *)(dd + 6*48), d[3] );
					_mm_store_si128( (__m128i *)(dd + 7*48), d[7] );
				}
				stmp += (i_src<<3);
				btmp += 8;
			}

			colume = fx;
			dtmp = dst_buf;
			i_dst_stride = 48;
		}

		//interpolation
		mc_chroma_sse2_core16( dtmp, i_dst_stride, buf, colume, ww, hh );

		if( fx == 0 )
		{
			//do nothing
		}
		else if( fy == 0 )//transpose and pack
		{
			for( y = 0; y < i_height; y += 8 )
			{
				for( x = 0; x < i_width; x += 8 )
				{
					__m128i s[8], d[8];
					int16_t *ss = dst_buf + x * 48 + y;
					int16_t *dd = dst + x + y * i_dst;

#define LOAD( i ) \
	s[i] = _mm_load_si128( (__m128i *)(ss + i*48) )

					LOAD( 0 );
					LOAD( 1 );
					LOAD( 2 );
					LOAD( 3 );
					LOAD( 4 );
					LOAD( 5 );
					LOAD( 6 );
					LOAD( 7 );

#undef  LOAD

					TRANSPOSE8_R2R_SSE2( d, s );

#define STORE( idx, i ) \
	_mm_store_si128( (__m128i *)(dd + idx*i_dst), d[i] )

					STORE( 0, 0 );
					STORE( 1, 4 );
					STORE( 2, 2 );
					STORE( 3, 6 );
					STORE( 4, 1 );
					STORE( 5, 5 );
					STORE( 6, 3 );
					STORE( 7, 7 );

#undef  STORE
				}
			}
		}
		else
		{
			//transpose to buf
			for( y = 0; y < h4; y += 8 )
			{
				for( x = 0; x < i_width; x += 8 )
				{
					__m128i s[8], d[8];
					int16_t *ss = dst_buf + x * 48 + y;
					int16_t *dd = buf + x + y * 48;

#define LOAD( i ) \
	s[i] = _mm_load_si128( (__m128i *)(ss + i*48) )

					LOAD( 0 );
					LOAD( 1 );
					LOAD( 2 );
					LOAD( 3 );
					LOAD( 4 );
					LOAD( 5 );
					LOAD( 6 );
					LOAD( 7 );

#undef  LOAD

					TRANSPOSE8_R2R_SSE2( d, s );

#define STORE( idx, i ) \
	_mm_store_si128( (__m128i *)(dd + idx*48), d[i] )

					STORE( 0, 0 );
					STORE( 1, 4 );
					STORE( 2, 2 );
					STORE( 3, 6 );
					STORE( 4, 1 );
					STORE( 5, 5 );
					STORE( 6, 3 );
					STORE( 7, 7 );

#undef  STORE
				}
			}

			//32 * 32 * 2 < 48 * 48，用dst_buf
			mc_chroma_sse2_core32( (int32_t *)dst_buf, 32, buf, fy, i_width, i_height );

			{
				btmp = dst_buf;
				dtmp = dst;
				for( y = 0; y < i_height; y ++ )
				{
					for( x = 0; x < i_width; x += 8 )
					{
						__m128i r0, r1, r2;
						int16_t *ss = btmp + (x<<1);

						r0 = _mm_load_si128( (__m128i *)(ss + 0) );
						r1 = _mm_load_si128( (__m128i *)(ss + 8) );
						r0 = _mm_srai_epi32( r0, 6 );
						r1 = _mm_srai_epi32( r1, 6 );
						r2 = _mm_packs_epi32( r0, r1 );
						_mm_store_si128( (__m128i *)(dtmp + x), r2 );
					}
					btmp += 32*2;
					dtmp += i_dst;
				}
			}
		}
	}
}