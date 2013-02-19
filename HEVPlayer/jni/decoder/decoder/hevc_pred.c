//**********************************************
//hevc_pred.h
//Unipipy @2011
//prediction tables & functions copied from encoder
//**********************************************
#include "hevc.h"
#include "hevc_pred.h"

static const int chroma_shift[64] = 
{
	32768,16384,10923, 8192, 6554, 5461, 4681, 4096,
	 3641, 3277, 2979, 2731, 2521, 2341, 2185, 2048,
	 1928, 1820, 1725, 1638, 1560, 1489, 1425, 1365,
	 1311, 1260, 1214, 1170, 1130, 1092, 1057, 1024,
	  993,  964,  936,  910,  886,  862,  840,  819,
	  799,  780,  762,  745,  728,  712,  697,  683,
	  669,  655,  643,  630,  618,  607,  596,  585,
	  575,  565,  555,  546,  537,  529,  520,
};

const int intra_filter[5] = 
{
	10, //4x4
	7, //8x8
	1, //16x16
	0, //32x32
	10, //64x64
};

const int angTable[9]    = {
	0,    2,    5,   9,  13,  17,  21,  26,  32
};
const int invAngTable[9] = {
	0, 4096, 1638, 910, 630, 482, 390, 315, 256
};


#define SRC(x,y) src[(x)+(y)*i_src]
#define CLIP_PIXEL(x) lent_clip_uint8(x);
void lent_predict_edge_filter( pixel *src, int i_src, int i_log2_size, int b_ver )
{
	const int size = 1 << i_log2_size;
	int i, ref_pixel = SRC(-1,-1);

	if( b_ver )
	{
		for( i = 0; i < size; i += 4 )
		{
			SRC(0,i  ) = CLIP_PIXEL( SRC(0,i  ) + ((SRC(-1,i  ) - ref_pixel)>>1) );
			SRC(0,i+1) = CLIP_PIXEL( SRC(0,i+1) + ((SRC(-1,i+1) - ref_pixel)>>1) );
			SRC(0,i+2) = CLIP_PIXEL( SRC(0,i+2) + ((SRC(-1,i+2) - ref_pixel)>>1) );
			SRC(0,i+3) = CLIP_PIXEL( SRC(0,i+3) + ((SRC(-1,i+3) - ref_pixel)>>1) );
		}
	}
	else
	{
		for( i = 0; i < size; i += 4 )
		{
			SRC(i  ,0) = CLIP_PIXEL( SRC(i  ,0) + ((SRC(i  ,-1) - ref_pixel)>>1) );
			SRC(i+1,0) = CLIP_PIXEL( SRC(i+1,0) + ((SRC(i+1,-1) - ref_pixel)>>1) );
			SRC(i+2,0) = CLIP_PIXEL( SRC(i+2,0) + ((SRC(i+2,-1) - ref_pixel)>>1) );
			SRC(i+3,0) = CLIP_PIXEL( SRC(i+3,0) + ((SRC(i+3,-1) - ref_pixel)>>1) );
		}
	}
}


#define MIN_UNIT_SIZE 4
void lent_predict_load_neighbor( HEVCContext *h, pixel *src, int i_src, int i_pix_x, int i_pix_y, int i_size, int b_luma )
{
	int i;
	int i_w = i_pix_x / MIN_UNIT_SIZE; //这里正好是当前像素左上角的位置
	int i_h = i_pix_y / MIN_UNIT_SIZE;
	int i_l = (i_size / MIN_UNIT_SIZE) << 1;
	int8_t *unit_map = &h->cache.unit_exist_map[LENT_scan4[0]+i_w+i_h*32], *u;
	pixel *org_src = src;
	const int ver_step = b_luma ? 32 : 64;
	const int hor_step = b_luma ? 1 : 2;
	const int i_step = 1;//b_luma ? 1 : 2;

	if( !unit_map[-32-1] )
	{
		int left, top;
		pixel left_val, top_val;

		u = unit_map - 1;
		for( i = 0; i < i_l; i += i_step, u += ver_step )
			if( *u ) break;
		left = i_l - i;
		left_val = SRC(-1,i*MIN_UNIT_SIZE);

		u = unit_map - 32;
		for( i = 0; i < i_l; i += i_step, u += hor_step )
			if( *u ) break;
		top = i_l - i;
		top_val = SRC(i*MIN_UNIT_SIZE,-1);

		if( !left && !top )
			SRC(-1,-1) = 0x80;
		else if( left && top )
			//SRC(-1,-1) = CLIP_PIXEL( ((int)left_val+top_val)>>1 );
			assert( 0 );
		else if( left )
			SRC(-1,-1) = left_val;
		else
			SRC(-1,-1) = top_val;
	}

	u = unit_map - 1;
	for( i = 0; i < i_l; i += i_step, u += ver_step, src += MIN_UNIT_SIZE*i_src )
	{
		if( *u ) continue;
		SRC(-1,0) = SRC(-1,1) = SRC(-1,2) = SRC(-1,3) = SRC(-1,-1);
	}
	src = org_src;

	u = unit_map - 32;
	for( i = 0; i < i_l; i += i_step, u += hor_step, src += MIN_UNIT_SIZE )
	{
		if( *u ) continue;
		SRC(0,-1) = SRC(1,-1) = SRC(2,-1) = SRC(3,-1) = SRC(-1,-1);
	}
}



#define F1(a,b)   (((a)+(b)+1)>>1)
#define F2(a,b,c) (((a)+2*(b)+(c)+2)>>2)

void lent_predict_dc_filter( pixel *src, int i_src, int i_width )
{
	int i;

	SRC(0,0) = F2( SRC(-1,0), SRC(0,0), SRC(0,-1) );
	for( i = 1; i < i_width; i ++ )
	{
		SRC(0,i) = ((SRC(0,i)*3 + SRC(-1,i) + 2) >> 2);
		SRC(i,0) = ((SRC(i,0)*3 + SRC(i,-1) + 2) >> 2);
	}
}

#define PIXEL_SPLAT_X4(x) ((x)*0x01010101U)

#define PL(y) \
	int l##y = SRC(-1,y);
#define PT(x) \
	int t##x = SRC(x,-1);
#define PREDICT_8x8_LOAD_TOPLEFT \
	int lt = SRC(-1,-1);
#define PREDICT_8x8_LOAD_LEFT \
	PL(0) PL(1) PL(2) PL(3) PL(4) PL(5) PL(6) PL(7)
#define PREDICT_8x8_LOAD_TOP \
	PT(0) PT(1) PT(2) PT(3) PT(4) PT(5) PT(6) PT(7)
#define PREDICT_8x8_LOAD_TOPRIGHT \
	PT(8) PT(9) PT(10) PT(11) PT(12) PT(13) PT(14) PT(15)


//说明:src一般传入的是predict时用来存放65x128临时像素数据的数组元素,而不是传fdec
static void predict_8x8_dc( pixel *src, int i_src )
{
	int i, y;
	uint32_t sum = 0;
	pixel *dst = src;

	for( i = 0; i < 8; i++ )
	{
		sum += src[-1 + i * i_src];
		sum += src[i - i_src];
	}

	sum += 8;
	sum >>= 4;
	sum = PIXEL_SPLAT_X4(sum);
	//sum *= 0x01010101;

	for( y = 0; y < 8; y++ )
	{
		(*((uint32_t *)dst)) = sum;
		(*((uint32_t *)(dst+4))) = sum;
		dst += i_src;
	}
}

static void predict_8x8_h( pixel *src, int i_src )
{
	int y;
	uint32_t v;
	pixel *dst = src;

	for( y = 0; y < 8; y++ )
	{
		v = PIXEL_SPLAT_X4(SRC(-1,y));
		(*((uint32_t *)dst)) = v;
		(*((uint32_t *)(dst+4))) = v;
		dst += i_src;
	}
}

static void predict_8x8_v( pixel *src, int i_src )
{
	int y;
	uint32_t v1,v2;
	pixel *dst = src;

	v1 = (*(uint32_t *)(src - i_src));
	v2 = (*(uint32_t *)(src - i_src + 4));

	for( y = 0; y < 8; y++ )
	{
		(*((uint32_t *)dst)) = v1;
		(*((uint32_t *)(dst+4))) = v2;
		dst += i_src;
	}
}

static void predict_4x4_dc( pixel *src, int i_src )
{
	uint32_t dc = PIXEL_SPLAT_X4( (SRC(-1,0) + SRC(-1,1) + SRC(-1,2) + SRC(-1,3) +
		SRC(0,-1) + SRC(1,-1) + SRC(2,-1) + SRC(3,-1) + 4) >> 3 );
	*(uint32_t *)(src + 0 * i_src) = dc;
	*(uint32_t *)(src + 1 * i_src) = dc;
	*(uint32_t *)(src + 2 * i_src) = dc;
	*(uint32_t *)(src + 3 * i_src) = dc;
}

static void predict_4x4_h( pixel *src, int i_src )
{
	int y;
	uint32_t v;
	pixel *dst = src;

	for( y = 0; y < 4; y++ )
	{
		v = PIXEL_SPLAT_X4(SRC(-1,y));
		(*((uint32_t *)dst)) = v;
		dst += i_src;
	}
}

static void predict_4x4_v( pixel *src, int i_src )
{
	int y;
	uint32_t v1;
	pixel *dst = src;

	v1 = (*(uint32_t *)(src - i_src));
	//v1 = SRC(0,-1)<<24|SRC(1,-1)<<16|SRC(2,-1)<<8|SRC(3,-1);

	for( y = 0; y < 4; y++ )
	{
		(*((uint32_t *)dst)) = v1;
		dst += i_src;
	}
}

static void predict_4x4_ang( pixel *src, int i_src, int intra_pred_order, int i_log2_size/*dummy*/, int filter/*dummy*/ )
{
	const int i_size = 4; 
	int mode_angle = intra_pred_order >= 18 ? 
		intra_pred_order - 26 :
	10 - intra_pred_order;
	int mode_angle_abs = LENTABS( mode_angle );
	int i_angle = mode_angle < 0 ? -angTable[mode_angle_abs] : angTable[mode_angle_abs];
	int diff = LENTMIN( LENTABS( intra_pred_order-10 ), LENTABS( intra_pred_order-26 ) );
	int k;
	const int top_offset = 16/*for_align*/+32/*left*/;
	//ALIGNED_16( pixel ref_pix[16/*for_align*/+32/*left*/+64/*top*/] );
	STACK_ALIGN( 16, pixel, ref_pix, 16/*for_align*/+32/*left*/+64/*top*/ )
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
	for( k = 0; k < i_size; k ++ )
	{
		int i_pix, i_frac;

		i_pos += i_angle;
		i_pix  = i_pos >> 5;
		i_frac = i_pos & 31;

		if( i_frac )
		{
			int t = i_pix+top_offset;
			dst[0] = CLIP_PIXEL( ((32-i_frac) * ref_pix[t  ] + 
				i_frac * ref_pix[t+1] + 16) >> 5 );
			dst[1] = CLIP_PIXEL( ((32-i_frac) * ref_pix[t+1] + 
				i_frac * ref_pix[t+2] + 16) >> 5 );
			dst[2] = CLIP_PIXEL( ((32-i_frac) * ref_pix[t+2] + 
				i_frac * ref_pix[t+3] + 16) >> 5 );
			dst[3] = CLIP_PIXEL( ((32-i_frac) * ref_pix[t+3] + 
				i_frac * ref_pix[t+4] + 16) >> 5 );
		}
		else
		{
			CP32( dst, &ref_pix[i_pix+top_offset] );
		}

		dst += i_src;
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

static void predict_4x4_planar( pixel *src, int i_src, int size/*dummy*/, int filter/*dummy*/ )
{
	int16_t top[4];
	int16_t bot[4];
	int16_t lef[4];
	int16_t rig[4];
	int16_t tr = SRC(4,-1);
	int16_t bl = SRC(-1,4);
	int i, j;

	for( i = 0; i < 4; i ++ )
	{
		top[i] = SRC(i,-1);
		lef[i] = SRC(-1,i);
		bot[i] = bl - top[i];
		rig[i] = tr - lef[i];
		top[i] <<= 2;
		lef[i] <<= 2;
	}

	for( i = 0; i < 4; i ++ )
	{
		int hor = lef[i] + 4;

		for( j = 0; j < 4; j ++ )
		{
			hor += rig[i];
			top[j] += bot[j];
			SRC(j,i) = ((hor + top[j]) >> 3 );
		}
	}
}


static void predict_16x16_dc( pixel *src, int i_src )
{
	int i, y;
	uint32_t sum = 0;

	for( i = 0; i < 16; i++ )
	{
		sum += src[-1 + i * i_src];
		sum += src[i - i_src];
	}

	sum = (sum + 16) >> 5;
	sum = PIXEL_SPLAT_X4(sum);
	//sum *= 0x01010101;

	for( y = 0; y < 16; y++ )
	{
		(*((uint32_t *)src)) = sum;
		(*((uint32_t *)(src+4))) = sum;
		(*((uint32_t *)(src+8))) = sum;
		(*((uint32_t *)(src+12))) = sum;
		src += i_src;
	}
}

static void predict_16x16_h( pixel *src, int i_src )
{
	int y;
	uint32_t v;
	pixel *dst = src;

	for( y = 0; y < 16; y++ )
	{
		v = PIXEL_SPLAT_X4(SRC(-1,y));
		(*((uint32_t *)dst)) = v;
		(*((uint32_t *)(dst+4))) = v;
		(*((uint32_t *)(dst+8))) = v;
		(*((uint32_t *)(dst+12))) = v;
		dst += i_src;
	}
}

static void predict_16x16_v( pixel *src, int i_src )
{
	int y;
	uint32_t v1,v2,v3,v4;
	pixel *dst = src;

	v1 = (*(uint32_t *)(src - i_src));
	v2 = (*(uint32_t *)(src - i_src + 4));
	v3 = (*(uint32_t *)(src - i_src + 8));
	v4 = (*(uint32_t *)(src - i_src + 12));

	for( y = 0; y < 16; y++ )
	{
		(*((uint32_t *)dst)) = v1;
		(*((uint32_t *)(dst+4))) = v2;
		(*((uint32_t *)(dst+8))) = v3;
		(*((uint32_t *)(dst+12))) = v4;
		dst += i_src;
	}
}

static void predict_16x16_planar( pixel *src, int i_src )
{
	/************************************************************************/
	/*	edge:
	0 1-32
	33
	|
	64
	*/
	/************************************************************************/
	int  bottomLeft,topRight;
	int x,y;
	int edge[65];//filter

	//do filter
	edge[0] = ( SRC(0,-1) + SRC(-1,-1)*2 + SRC(-1,0) + 2 ) >>2 ;
	for( x = 0; x < 31; x++)
		edge[x+1] = ( SRC(x-1,-1) + SRC(x,-1)*2 + SRC(x+1,-1) + 2 ) >> 2;
	x++;
	for( y = 0; x < 63; x++,y++)
		edge[x+1] = ( SRC(-1,y-1) + SRC(-1,y)*2 + SRC(-1,y+1) + 2 ) >> 2;

	edge[32] = SRC(-1,31);
	edge[64] = SRC(31,-1);

	bottomLeft = edge[49];
	topRight = edge[17];

	for( x = 0; x < 16; x++)
	{
		for( y = 0; y < 16; y++)
		{
			SRC(x,y) = ( (15-x)*edge[33+y] + (x+1)*topRight + (15-y)*edge[1+x] +  (y+1)*bottomLeft + 16 ) >> 5;
		}
	}
}

static void predict_32x32_dc( pixel *src, int i_src )
{
	int i, y;
	uint32_t sum = 0;

	for( i = 0; i < 32; i++ )
	{
		sum += src[-1 + i * i_src];
		sum += src[i - i_src];
	}

	sum = (sum + 32) >> 6;
	sum = PIXEL_SPLAT_X4(sum);

	for( y = 0; y < 32; y++ )
	{
		(*((uint32_t *)src)) = sum;
		(*((uint32_t *)(src+4))) = sum;
		(*((uint32_t *)(src+8))) = sum;
		(*((uint32_t *)(src+12))) = sum;
		(*((uint32_t *)(src+16))) = sum;
		(*((uint32_t *)(src+20))) = sum;
		(*((uint32_t *)(src+24))) = sum;
		(*((uint32_t *)(src+28))) = sum;
		src += i_src;
	}
}

static void predict_32x32_h( pixel *src, int i_src )
{
	int y;
	uint32_t v;
	pixel *dst = src;

	for( y = 0; y < 32; y++ )
	{
		v = PIXEL_SPLAT_X4(SRC(-1,y));
		(*((uint32_t *)dst)) = v;
		(*((uint32_t *)(dst+4))) = v;
		(*((uint32_t *)(dst+8))) = v;
		(*((uint32_t *)(dst+12))) = v;
		(*((uint32_t *)(dst+16))) = v;
		(*((uint32_t *)(dst+20))) = v;
		(*((uint32_t *)(dst+24))) = v;
		(*((uint32_t *)(dst+28))) = v;
		dst += i_src;
	}
}

static void predict_32x32_v( pixel *src, int i_src )
{
	int y;
	uint32_t v1,v2,v3,v4,v5,v6,v7,v8;
	pixel *dst = src;

	v1 = (*(uint32_t *)(src - i_src));
	v2 = (*(uint32_t *)(src - i_src + 4));
	v3 = (*(uint32_t *)(src - i_src + 8));
	v4 = (*(uint32_t *)(src - i_src + 12));
	v5 = (*(uint32_t *)(src - i_src + 16));
	v6 = (*(uint32_t *)(src - i_src + 20));
	v7 = (*(uint32_t *)(src - i_src + 24));
	v8 = (*(uint32_t *)(src - i_src + 28));

	for( y = 0; y < 32; y++ )
	{
		(*((uint32_t *)dst)) = v1;
		(*((uint32_t *)(dst+4))) = v2;
		(*((uint32_t *)(dst+8))) = v3;
		(*((uint32_t *)(dst+12))) = v4;
		(*((uint32_t *)(dst+16))) = v5;
		(*((uint32_t *)(dst+20))) = v6;
		(*((uint32_t *)(dst+24))) = v7;
		(*((uint32_t *)(dst+28))) = v8;
		dst += i_src;
	}
}

#define F3(a,b,c) (((int)(a)+((b)<<1)+(c)+2)>>2)
static void predict_planar_internal( pixel *src, int i_src, int i_log2_size, int filter )
{
	DECLARE_ALIGNED(16, int16_t, top)[32];
	DECLARE_ALIGNED(16, int16_t, bot)[32];
	DECLARE_ALIGNED(16, int16_t, lef)[32];
	DECLARE_ALIGNED(16, int16_t, rig)[32];
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
			assert( size == 32 );
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

// 	for( i = 0; i < size; i ++ ) //slow
// 	{
// 		for( j = 0; j < size; j ++ )
// 		{
// 			SRC(j,i) = ( (size_m1-j)*lef[i] + (j+1)*tr + (size_m1-i)*top[j] + (i+1)*bl + size ) >> shift2D;
// 		}
// 	}

	for( i = 0; i < size; i ++ )
	{
		bot[i] = bl - top[i];
		rig[i] = tr - lef[i];
		top[i] <<= i_log2_size;
		lef[i] <<= i_log2_size;
	}

	for( i = 0; i < size; i ++ )
	{
		int hor = lef[i] + size;

		for( j = 0; j < size; j ++ )
		{
			hor += rig[i];
			top[j] += bot[j];
			SRC(j,i) = ((hor + top[j]) >> shift2D );
		}
	}
}
#undef F3

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
#define F3(a,b,c) (((a)+((b)<<1)+(c)+2)>>2)
static inline void predict_ang_filter( pixel *dst, pixel *src, int i_src, int i_left, int i_top,
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

static void predict_ang_internal( pixel *src, int i_src, int intra_pred_order, int i_log2_size, int filter )
{
	int i_size = 1 << i_log2_size;
	int mode_angle = intra_pred_order >= 18 ? 
		intra_pred_order - 26 :
	10 - intra_pred_order;
	int mode_angle_abs = LENTABS( mode_angle );
	int i_angle = mode_angle < 0 ? -angTable[mode_angle_abs] : angTable[mode_angle_abs];
	int shift_m2 = i_log2_size - 2;
	int diff = LENTMIN( LENTABS( intra_pred_order-10 ), LENTABS( intra_pred_order-26 ) );
	int k, l;
	const int top_offset = 16/*for_align*/+32/*left*/;
	DECLARE_ALIGNED(16, pixel,		ref_pix		)[16/*for_align*/+32/*left*/+64/*top*/];
	int ref_pix_l, ref_pix_r;
	pixel *dst;

	int i_pos = 0;

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
				predict_ang_filter( ref_pix + top_offset, src, i_src, ref_pix_l, ref_pix_r,
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
				predict_ang_filter( ref_pix + top_offset, src, i_src, ref_pix_l, ref_pix_r,
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
			for( l = 0; l < i_size; l += 4 )
			{
				int t = l+i_pix+top_offset;
				dst[l  ] = CLIP_PIXEL( ((32-i_frac) * ref_pix[t  ] + 
					i_frac * ref_pix[t+1] + 16) >> 5 );
				dst[l+1] = CLIP_PIXEL( ((32-i_frac) * ref_pix[t+1] + 
					i_frac * ref_pix[t+2] + 16) >> 5 );
				dst[l+2] = CLIP_PIXEL( ((32-i_frac) * ref_pix[t+2] + 
					i_frac * ref_pix[t+3] + 16) >> 5 );
				dst[l+3] = CLIP_PIXEL( ((32-i_frac) * ref_pix[t+3] + 
					i_frac * ref_pix[t+4] + 16) >> 5 );
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
		for ( k = 0; k < i_size-1; k ++ )
		{
			for ( l = k + 1; l < i_size; l ++ )
			{
				pixel *s1 = &src[k*i_src+l];
				pixel *s2 = &src[l*i_src+k];
				pixel tmp = *s1;
				*s1 = *s2;
				*s2 = tmp;
			}
		}
	}
}


//////////////////////////////////////////////////////////////////////////////

#define LINK_PRED_FUNCTIONS(a,b,type) \
	a[0]=b##_##4x4##_##type; \
	a[1]=b##_##8x8##_##type; \
	a[2]=b##_##16x16##_##type; \
	a[3]=b##_##32x32##_##type;
	
int lent_pred_init(HEVCPredContext *pc,unsigned int machine)
{
	int lent_pred_init_x86(HEVCPredContext *pc,unsigned int sse);

	LINK_PRED_FUNCTIONS(pc->predict_dc,predict,dc)
	LINK_PRED_FUNCTIONS(pc->predict_h,predict,h)
	LINK_PRED_FUNCTIONS(pc->predict_v,predict,v)

	pc->predict_planar[0]=predict_4x4_planar;
	pc->predict_planar[1]=predict_planar_internal;
	pc->predict_planar[2]=predict_planar_internal;
	pc->predict_planar[3]=predict_planar_internal;

	pc->predict_ang[0]=predict_4x4_ang;
	pc->predict_ang[1]=predict_ang_internal;
	pc->predict_ang[2]=predict_ang_internal;
	pc->predict_ang[3]=predict_ang_internal;

	
#if ARCH_X86_32
	lent_pred_init_x86(pc,machine);
#endif

	return 0;
}

#undef LINK_PRED_FUNCTIONS