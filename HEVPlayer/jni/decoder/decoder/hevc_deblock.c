//**********************************************
//hevc_deblock.c
//Unipipy @2011
//loopfilter
//**********************************************
#include "hevc.h"
#include "hevc_deblock.h"
#if ARCH_X86_32
#include <emmintrin.h>
#endif

const uint8_t tc_table[54] =
{
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,5,5,6,6,7,8,9,10,11,13,14,16,18,20,22,24
};

const uint8_t beta_table[52] =
{
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,6,7,8,9,10,11,12,13,14,15,16,17,18,20,22,24,26,28,30,32,34,36,38,40,42,44,46,48,50,52,54,56,58,60,62,64
};

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

static inline void deblock_luma_c( pixel *pix, pixel *judge, int xstride, int ystride, int beta, int tc0 )
{
	int i_side_thresh = (beta+(beta>>1))>>3;
	int i_tc_thresh = tc0 * 10;
	int tcm2 = tc0 << 1;
	int tcd2 = tc0 >> 1;
	int dp, dq, d0, d3, d;
	int dp0, dp3, dq0, dq3;
	int i;//, edge4x4;
	pixel src[8];

	if( tc0 < 0 ) return;

	//for( edge4x4 = 0; edge4x4 < 2; edge4x4 ++ )
	{

		dp0 = cal_dp( judge + 0*ystride, xstride );
		dq0 = cal_dq( judge + 0*ystride, xstride );
		dp3 = cal_dp( judge + 3*ystride, xstride );
		dq3 = cal_dq( judge + 3*ystride, xstride );

		d0 = dp0 + dq0;
		d3 = dp3 + dq3;
		dp = dp0 + dp3;
		dq = dq0 + dq3;

		d = d0 + d3;

		if( d < beta )
		{
			int b_filter_p = dp < i_side_thresh;
			int b_filter_q = dq < i_side_thresh;
			int b_strong = b_strong_filter( judge          , xstride, 2*d0, beta, tc0 )
				        && b_strong_filter( judge+3*ystride, xstride, 2*d3, beta, tc0 );

			for( i = 0; i < 4; i ++ )
			{	
				if( xstride == 1 )//TODO: 如何使用规整代码
					CP64( src, pix-4 );
				else
				{
					int j;
					for( j = 0; j < 8; j ++ )
						src[j] = pix[(j-4)*xstride];
				}

				if( b_strong )
				{//strong filtering
					PIX(-1) = lent_clip( ((int)src[1] + 2*src[2] + 2*src[3] + 2*src[4] + src[5] + 4)>>3, src[3]-tcm2, src[3]+tcm2 );
					PIX( 0) = lent_clip( ((int)src[2] + 2*src[3] + 2*src[4] + 2*src[5] + src[6] + 4)>>3, src[4]-tcm2, src[4]+tcm2 );

					PIX(-2) = lent_clip( ((int)src[1] + src[2] + src[3] + src[4] + 2)>>2, src[2]-tcm2, src[2]+tcm2 );
					PIX( 1) = lent_clip( ((int)src[3] + src[4] + src[5] + src[6] + 2)>>2, src[5]-tcm2, src[5]+tcm2 );

					PIX(-3) = lent_clip( ((int)2*src[0] + 3*src[1] + src[2] + src[3] + src[4] + 4)>>3, src[1]-tcm2, src[1]+tcm2 );
					PIX( 2) = lent_clip( ((int)src[3] + src[4] + src[5] + 3*src[6] + 2*src[7] + 4)>>3, src[6]-tcm2, src[6]+tcm2 );
				}
				else
				{//weak filtering
					int delta, delta1, delta2;

					delta = (9*(src[4]-src[3]) - 3*(src[5]-src[2]) + 8)>>4;
					if( LENTABS(delta) < i_tc_thresh )
					{
						delta = lent_clip( delta, -tc0, tc0);
						PIX(-1) = lent_clip_uint8(src[3]+delta);
						PIX( 0) = lent_clip_uint8(src[4]-delta);

						if( b_filter_p )
						{
							delta1 = ( ((src[1]+src[3]+1)>>1) - src[2] + delta )>>1;
							delta1 = lent_clip( delta1, -tcd2, tcd2 );
							PIX(-2) = lent_clip_uint8(src[2]+delta1);
						}

						if( b_filter_q )
						{
							delta2 = ( ((src[4]+src[6]+1)>>1) - src[5] - delta )>>1;
							delta2 = lent_clip( delta2, -tcd2, tcd2 );
							PIX( 1) = lent_clip_uint8(src[5]+delta2);
						}
					}
				}

				pix += ystride;
				judge += ystride;
			}
		}

		//pix = pix_ori + 4*ystride;
		//judge = judge_ori + 4*ystride;
	}
}
static void deblock_v_luma_c( pixel *pix, pixel *judge, int stride, int beta, int tc0 )
{
	deblock_luma_c( pix, judge, stride, 1, beta, tc0 );
}
static void deblock_h_luma_c( pixel *pix, pixel *judge, int stride, int beta, int tc0 )
{
	deblock_luma_c( pix, judge, 1, stride, beta, tc0 );
}

static inline void deblock_chroma_c( pixel *pix, int xstride, int ystride, int beta, int tc0 )
{
	int i;

	for( i = 0; i < 4; i ++ )
	{
		pixel m4 = PIX( 0);
		pixel m3 = PIX(-1);
		pixel m5 = PIX( 1);
		pixel m2 = PIX(-2);
		int delta = ( (( m4 - m3 ) << 2 ) + m2 - m5 + 4 ) >> 3;

		delta = lent_clip( delta, -tc0, tc0 );
		PIX(-1) = lent_clip_uint8(m3+delta);
		PIX( 0) = lent_clip_uint8(m4-delta);

		pix += ystride;
	}
}
static void deblock_v_chroma_c( pixel *pix, pixel *judge, int stride, int beta, int tc0 )
{
	deblock_chroma_c( pix, stride, 1, beta, tc0 );
}
static void deblock_h_chroma_c( pixel *pix, pixel *judge, int stride, int beta, int tc0 )
{
	deblock_chroma_c( pix, 1, stride, beta, tc0 );
}
#undef PIX
#undef JUD

void lent_deblock_init(HEVCDeblockContext *pf, unsigned int machine)
{
	void lent_deblock_init_x86(HEVCDeblockContext *pf, unsigned int sse);

	pf->deblock_luma[0] = deblock_h_luma_c;
	pf->deblock_luma[1] = deblock_v_luma_c;
	pf->deblock_chroma[0] = deblock_h_chroma_c;
	pf->deblock_chroma[1] = deblock_v_chroma_c;
#if ARCH_X86_32
	lent_deblock_init_x86(pf, machine);
#endif
}

static inline void deblock_edge_luma( HEVCContext *h, pixel *pix1, pixel *pix2, int i_stride, int i_filter_stride,
				int8_t bS_line[16], int8_t *i_qp, int qp_stride, LENT_deblock_func_t pf_deblock )
{
	int i;
	int pix_stride = i_filter_stride*4;

	for( i = 0; i < 16; i ++ )
	{
		int tc;
		int beta;
		int8_t bS;
		int index_tc;
		int index_b;
		int qp;

		if( bS_line[i] )
		{
			pixel *p1 = pix1 + i * pix_stride;
			pixel *p2 = pix2 + i * pix_stride;
			bS = bS_line[i];
			qp = i_qp[(i>>1)*qp_stride];

			index_b = qp;
			index_tc = qp + (bS > 2 ? 2 : 0);
#ifdef USE_DEBLOCK_CONTROL
			if(h->pps.m_deblockingFilterControlPresentFlag)
			{
				int offsetBeta=h->sh.m_deblockingFilterBetaOffsetDiv2<<1;
				int offsetTc=h->sh.m_deblockingFilterTcOffsetDiv2<<1;

				index_b+=offsetBeta;
				index_b=lent_clip(index_b,0,51);

				index_tc+=offsetTc;
				index_tc=lent_clip(index_tc,0,53);
			}
#endif
			beta  = beta_table[index_b];
			tc = tc_table[index_tc];
			if ( tc > 0 && beta > 0 )
			{
				pf_deblock( p1, p2, i_stride, beta, tc );
			}
		}
	}
}

static inline void deblock_edge_chroma( HEVCContext *h, pixel *pix1, pixel *pix2, int i_stride, int i_filter_stride,
				int8_t bS_line[16], int8_t *i_qp, int qp_stride, LENT_deblock_func_t pf_deblock )
{
	int i;
	int pix_stride = i_filter_stride*4;

	for( i = 0; i < 16; i += 2 )
	{
		int tc;
		int beta;
		int8_t bS;
		int index_tc;
		int index_b;
		int qp;

		if( bS_line[i] > 2 )
		{
			pixel *p1 = pix1 + (i>>1) * pix_stride;
			pixel *p2 = pix2 + (i>>1) * pix_stride;
			bS = bS_line[i];
			qp = i_qp[(i>>1)*qp_stride];
			qp = i_chroma_qp_table[qp];

			index_b = qp;
			index_tc = qp + 2/*(bS > 2 ? 2 : 0)*/; //if( bS <= 2 ) do not deblock
#ifdef USE_DEBLOCK_CONTROL
			if(h->pps.m_deblockingFilterControlPresentFlag)
			{
				int offsetBeta=h->sh.m_deblockingFilterBetaOffsetDiv2<<1;
				int offsetTc=h->sh.m_deblockingFilterTcOffsetDiv2<<1;

				index_b+=offsetBeta;
				index_b=lent_clip(index_b,0,51);

				index_tc+=offsetTc;
				index_tc=lent_clip(index_tc,0,53);
			}
#endif
			beta  = beta_table[index_b];
			tc = tc_table[index_tc];
			if ( tc > 0 && beta > 0 )
			{
				pf_deblock( p1, NULL, i_stride, beta, tc );
				pf_deblock( p2, NULL, i_stride, beta, tc );
			}
		}
	}
}

void lent_deblock_row( HEVCContext *h, int cu_y )
{
	int cu_x, i_cu_xy;
	// int *stride = h->current_picture.linesize;
	int stride_y = h->current_picture.linesize[0], stride_u = h->current_picture.linesize[1], stride_v = h->current_picture.linesize[2];
	pixel *px[3];
	int8_t (*bs)[8][16];
	int8_t (*deblock_qp)[8];

#define FILTER( dir, edge, qp )\
	do\
	{\
		deblock_edge( h, px[0] + 8*edge*(dir?stride_y/*[0]*/:1), px[0] + 8*edge*(dir?stride_y/*[0]*/:1),\
					stride_y/*[0]*/, dir?1:stride_y/*[0]*/, bs[dir][edge], qp, dir?1:8, 0,\
					h->deblock.deblock_luma[dir] );\
		if( !(edge & 1) )\
			deblock_edge( h, px[1] + 4*edge*(dir?stride_u/*[1]*/:1), px[2] + 4*edge*(dir?stride_u/*[1]*/:1),\
								stride_u/*[1]*/, dir?1:stride_u/*[1]*/, bs[dir][edge], qp, dir?1:8, 1,\
								h->deblock.deblock_chroma[dir] );\
	} while(0)
	
	if(h->sh.m_deblockingFilterDisable)
		return;
	for( cu_x = 0; cu_x < h->cu_width; cu_x ++ )
	{
		i_cu_xy = cu_y * h->cu_width + cu_x;
		bs = h->current.deblock_strength[i_cu_xy];
		deblock_qp = h->current.deblock_qp[i_cu_xy];

		px[0] = h->current_picture.data[0] + 64*cu_y*stride_y/*[0]*/ + 64*cu_x;
		//judge = h->current.luma_nonfiltered + 64*cu_y*stride[0] + 64*cu_x;
		px[1] = h->current_picture.data[1] + 32*cu_y*stride_u/*[1]*/ + 32*cu_x;
		px[2] = h->current_picture.data[2] + 32*cu_y*stride_v/*[2]*/ + 32*cu_x;

// 		for( edge = 0; edge < 8; edge ++ )
// 		{
// 			FILTER( 0, edge, &deblock_qp[0][edge] );
// 		}
		{
			pixel *src0 = px[0], *src1 = px[1], *src2 = px[2];
			int8_t *qp = deblock_qp[0];
			int8_t (*pbs)[16] = bs[0];
			LENT_deblock_func_t fl = h->deblock.deblock_luma[0];
			LENT_deblock_func_t fc = h->deblock.deblock_chroma[0];

			deblock_edge_luma( h, src0 + 8*0, src0 + 8*0, stride_y, stride_y, pbs[0], &qp[0], 8, fl );
			deblock_edge_chroma( h, src1 + 4*0, src2 + 4*0, stride_u, stride_u, pbs[0], &qp[0], 8, fc );
			deblock_edge_luma( h, src0 + 8*1, src0 + 8*1, stride_y, stride_y, pbs[1], &qp[1], 8, fl );
			deblock_edge_luma( h, src0 + 8*2, src0 + 8*2, stride_y, stride_y, pbs[2], &qp[2], 8, fl );
			deblock_edge_chroma( h, src1 + 4*2, src2 + 4*2, stride_u, stride_u, pbs[2], &qp[2], 8, fc );
			deblock_edge_luma( h, src0 + 8*3, src0 + 8*3, stride_y, stride_y, pbs[3], &qp[3], 8, fl );
			deblock_edge_luma( h, src0 + 8*4, src0 + 8*4, stride_y, stride_y, pbs[4], &qp[4], 8, fl );
			deblock_edge_chroma( h, src1 + 4*4, src2 + 4*4, stride_u, stride_u, pbs[4], &qp[4], 8, fc );
			deblock_edge_luma( h, src0 + 8*5, src0 + 8*5, stride_y, stride_y, pbs[5], &qp[5], 8, fl );
			deblock_edge_luma( h, src0 + 8*6, src0 + 8*6, stride_y, stride_y, pbs[6], &qp[6], 8, fl );
			deblock_edge_chroma( h, src1 + 4*6, src2 + 4*6, stride_u, stride_u, pbs[6], &qp[6], 8, fc );
			deblock_edge_luma( h, src0 + 8*7, src0 + 8*7, stride_y, stride_y, pbs[7], &qp[7], 8, fl );
		}
/*
		for( edge = 0; edge < 8; edge ++ )
		{
			deblock_edge( h, px[0] + 8*edge*(1), px[0] + 8*edge*(1),
						stride[0], stride[0], bs[0][edge], &deblock_qp[0][edge], 8, 0,
						h->deblock.deblock_luma[0] );
		}
		for( edge = 0; edge < 4; edge ++ )
		{
			deblock_edge( h, px[1] + 8*edge*(1), px[2] + 8*edge*(1),
								stride[1], stride[1], bs[0][edge * 2], &deblock_qp[0][edge * 2], 8, 1,
								h->deblock.deblock_chroma[0] );
		}
*/
	}

	for( cu_x = 0; cu_x < h->cu_width; cu_x ++ )
	{
		i_cu_xy = cu_y * h->cu_width + cu_x;
		bs = h->current.deblock_strength[i_cu_xy];
		deblock_qp = h->current.deblock_qp[i_cu_xy];

		px[0] = h->current_picture.data[0] + 64*cu_y*stride_y/*[0]*/ + 64*cu_x;
		//judge = h->current.luma_nonfiltered + 64*cu_y*stride[0] + 64*cu_x;
		px[1] = h->current_picture.data[1] + 32*cu_y*stride_u/*[1]*/ + 32*cu_x;
		px[2] = h->current_picture.data[2] + 32*cu_y*stride_v/*[2]*/ + 32*cu_x;

// 		for( edge = 0; edge < 8; edge ++ )
// 		{
// 			FILTER( 1, edge, &deblock_qp[edge][0] );
// 		}
		{
			pixel *src0 = px[0], *src1 = px[1], *src2 = px[2];
			int8_t (*pbs)[16] = bs[1];
			LENT_deblock_func_t fl = h->deblock.deblock_luma[1];
			LENT_deblock_func_t fc = h->deblock.deblock_chroma[1];

			deblock_edge_luma( h, src0 + 8*0*stride_y, src0 + 8*0*stride_y, stride_y, 1, pbs[0], deblock_qp[0], 1, fl );
			deblock_edge_chroma( h, src1 + 4*0*stride_u, src2 + 4*0*stride_u, stride_u, 1, pbs[0], deblock_qp[0], 1, fc );
			deblock_edge_luma( h, src0 + 8*1*stride_y, src0 + 8*1*stride_y, stride_y, 1, pbs[1], deblock_qp[1], 1, fl );
			deblock_edge_luma( h, src0 + 8*2*stride_y, src0 + 8*2*stride_y, stride_y, 1, pbs[2], deblock_qp[2], 1, fl );
			deblock_edge_chroma( h, src1 + 4*2*stride_u, src2 + 4*2*stride_u, stride_u, 1, pbs[2], deblock_qp[2], 1, fc );
			deblock_edge_luma( h, src0 + 8*3*stride_y, src0 + 8*3*stride_y, stride_y, 1, pbs[3], deblock_qp[3], 1, fl );
			deblock_edge_luma( h, src0 + 8*4*stride_y, src0 + 8*4*stride_y, stride_y, 1, pbs[4], deblock_qp[4], 1, fl );
			deblock_edge_chroma( h, src1 + 4*4*stride_u, src2 + 4*4*stride_u, stride_u, 1, pbs[4], deblock_qp[4], 1, fc );
			deblock_edge_luma( h, src0 + 8*5*stride_y, src0 + 8*5*stride_y, stride_y, 1, pbs[5], deblock_qp[5], 1, fl );
			deblock_edge_luma( h, src0 + 8*6*stride_y, src0 + 8*6*stride_y, stride_y, 1, pbs[6], deblock_qp[6], 1, fl );
			deblock_edge_chroma( h, src1 + 4*6*stride_u, src2 + 4*6*stride_u, stride_u, 1, pbs[6], deblock_qp[6], 1, fc );
			deblock_edge_luma( h, src0 + 8*7*stride_y, src0 + 8*7*stride_y, stride_y, 1, pbs[7], deblock_qp[7], 1, fl );
		}
/*
		for( edge = 0; edge < 8; edge ++ )
		{
			deblock_edge( h, px[0] + 8*edge*(stride[0]), px[0] + 8*edge*(stride[0]),
						stride[0], 1, bs[1][edge], &deblock_qp[edge][0], 1, 0,
						h->deblock.deblock_luma[1] );
		}
		for( edge = 0; edge < 4; edge ++ )
		{
			deblock_edge( h, px[1] + 8*edge*(stride[1]), px[2] + 8*edge*(stride[1]),
								stride[1], 1, bs[1][edge * 2], &deblock_qp[edge * 2][0], 1, 1,
								h->deblock.deblock_chroma[1] );
		}
*/
	}
}

/******** bs计算过程 ********/
static inline void LENT_block_strength_block( HEVCContext *h, int i_idx, int x0, int y0, int i_width, int i_height, int trans_border )
{
	const int b_border_0 = trans_border & 1;
	const int b_border_1 = (trans_border>>1) & 1;
	int i8 = LENT_scan8[i_idx >> 2];
	int i4 = LENT_scan4[i_idx];
	int i_pred_mode = h->cache.pred_mode[i8];
	int8_t (*bs)[8][16] = h->current.deblock_strength[h->cu_index];
	int8_t (*ref)[LENT_SCAN_SIZE*4] = h->cache.ref;
	int16_t (*mv)[LENT_SCAN_SIZE*4][2] = h->cache.mv;
	int i_bframe = h->sh.m_eSliceType == SLICE_B;

	int xs = x0 >> 3;
	int ys = y0 >> 3;
	int l_w = i_width >> 2;
	int l_h = i_height >> 2;
	int x, y;
	
	ys <<= 1;
	//if( bs[0][xs][ys] < 0 )
	{
		if( i_pred_mode == MODE_INTRA )
		{
			for( y = 0; y < l_h; y ++ )
			{
				if( bs[0][xs][y+ys] < 0 )
					bs[0][xs][y+ys] = 3;
			}
		}
		else
		{
			for( y = 0; y < l_h; y ++ )
			{
				if( bs[0][xs][y+ys] < 0 )
				{
					int loc = i4 + (y)*32;
					int locn = loc - 1;
					int loc_flag  = DEBLOCK_CBF_TL<<((y&1)<<1);
					int locn_flag = DEBLOCK_CBF_TR<<((y&1)<<1);
					int loc_flag_act  = h->cache.cu_flag[i8 + (y>>1)*16];
					int locn_flag_act = h->cache.cu_flag[i8 + (y>>1)*16-1];

					if( b_border_0 && ((loc_flag_act & loc_flag) || 
						(locn_flag_act & locn_flag)) )
						bs[0][xs][y+ys] = 2;
					else if( ref[0][loc] != ref[0][locn] || (ref[0][loc] >= 0 && (
						LENTABS_INT( mv[0][loc][0] - mv[0][locn][0] ) >= 4 ||
						LENTABS_INT( mv[0][loc][1] - mv[0][locn][1] ) >= 4)))
						bs[0][xs][y+ys] = 1;
					else if( i_bframe && (ref[1][loc] != ref[1][locn] || (ref[1][loc] >= 0 && (
						LENTABS_INT( mv[1][loc][0] - mv[1][locn][0] ) >= 4 ||
						LENTABS_INT( mv[1][loc][1] - mv[1][locn][1] ) >= 4))))
						bs[0][xs][y+ys] = 1;
					else
						bs[0][xs][y+ys] = 0;
				}
			}
		}
	}
	ys >>= 1;
	xs <<= 1;

	//if( bs[1][ys][xs] < 0 )
	{
		if( i_pred_mode == MODE_INTRA )
		{
			for( x = 0; x < l_w; x ++ )
			{
				if( bs[1][ys][x+xs] < 0 )
					bs[1][ys][x+xs] = 3;
			}
		}
		else
		{
			for( x = 0; x < l_w; x ++ )
			{
				if( bs[1][ys][x+xs] < 0 )
				{
					int loc = i4 + (x);
					int locn = loc - 32;
					int loc_flag  = DEBLOCK_CBF_TL<<(x&1);
					int locn_flag = DEBLOCK_CBF_DL<<(x&1);
					int loc_flag_act  = h->cache.cu_flag[i8 + (x>>1)];
					int locn_flag_act = h->cache.cu_flag[i8 + (x>>1)-16];

					if( b_border_1 && ((loc_flag_act & loc_flag) || 
						(locn_flag_act & locn_flag)) )
						bs[1][ys][x+xs] = 2;
					else if( ref[0][loc] != ref[0][locn] || (ref[0][loc] >= 0 && (
						LENTABS_INT( mv[0][loc][0] - mv[0][locn][0] ) >= 4 ||
						LENTABS_INT( mv[0][loc][1] - mv[0][locn][1] ) >= 4)))
						bs[1][ys][x+xs] = 1;
					else if( i_bframe && (ref[1][loc] != ref[1][locn] || (ref[1][loc] >= 0 && (
						LENTABS_INT( mv[1][loc][0] - mv[1][locn][0] ) >= 4 ||
						LENTABS_INT( mv[1][loc][1] - mv[1][locn][1] ) >= 4))))
						bs[1][ys][x+xs] = 1;
					else
						bs[1][ys][x+xs] = 0;
				}
			}
		}
	}
	xs >>= 1;
}

static void LENT_deblock_strength_trans_tree( HEVCContext *h, int i_idx, int i_pix_x0, int i_pix_y0, int i_log2_size )
{
	int i_width		= 1 << i_log2_size;
	int i_height	= 1 << i_log2_size;
	int i_depth		= LOG22DEPTH(i_log2_size);
	int i_flag_idx	= TREEIDX(i_idx,i_log2_size);
	int b_split_trans = h->cache.b_split_trans[i_flag_idx];

	if( b_split_trans && i_log2_size > 3 )//滤波最小8x8
	{
		int i_pix_x1 = i_pix_x0 + (i_width>>1);
		int i_pix_y1 = i_pix_y0 + (i_height>>1);
		int i_idx_step = DEPTH2IDXSTEPC(i_depth);

		LENT_deblock_strength_trans_tree( h, i_idx, i_pix_x0, i_pix_y0, i_log2_size-1 );
		i_idx += i_idx_step;
		LENT_deblock_strength_trans_tree( h, i_idx, i_pix_x1, i_pix_y0, i_log2_size-1);
		i_idx += i_idx_step;
		LENT_deblock_strength_trans_tree( h, i_idx, i_pix_x0, i_pix_y1, i_log2_size-1);
		i_idx += i_idx_step;
		LENT_deblock_strength_trans_tree( h, i_idx, i_pix_x1, i_pix_y1, i_log2_size-1);
	}
	else
	{
		int i8 = LENT_scan8[i_idx >> 2];

		if( b_split_trans )
		{
			int tmp = h->cache.cu_flag[i8];
			int i_sub_flag = TREEIDX(i_idx,i_log2_size-1);//LENT_cu_depth_idx[i_depth+1] + ((i_idx>>(i_log2_size-3))>>(i_log2_size-3));

			if( CBF_LUMA(h->cache.i_cbp[i_sub_flag]) )
				tmp |= DEBLOCK_CBF_TL;
			if( CBF_LUMA(h->cache.i_cbp[i_sub_flag+1]) )
				tmp |= DEBLOCK_CBF_TR;
			if( CBF_LUMA(h->cache.i_cbp[i_sub_flag+2]) )
				tmp |= DEBLOCK_CBF_DL;
			if( CBF_LUMA(h->cache.i_cbp[i_sub_flag+3]) )
				tmp |= DEBLOCK_CBF_DR;

			lent_fill_rectangle(&h->cache.cu_flag[i8], i_width>>3, i_height>>3, 16, tmp );
		}
		else if( CBF_LUMA(h->cache.i_cbp[i_flag_idx]) )
		{
			int tmp = h->cache.cu_flag[i8];

			tmp |= DEBLOCK_CBF_ALL;
			lent_fill_rectangle(&h->cache.cu_flag[i8], i_width>>3, i_height>>3, 16, tmp );
		}
		LENT_block_strength_block( h, i_idx, i_pix_x0, i_pix_y0, i_width, i_height, 3 );
	}
}

static inline void LENT_deblock_strength_cu( HEVCContext *h, int i_idx, int x0, int y0, int i_log2_size )
{
	int i_width		= 1 << i_log2_size;
	int i_height	= 1 << i_log2_size;
	int i_depth		= LOG22DEPTH(i_log2_size);
	int i_flag_idx	= TREEIDX(i_idx,i_log2_size);
	int i8			= LENT_scan8[i_idx >> 2];
	int i_part_mode = h->cache.part_mode[i_flag_idx];
	int i_pred_mode = h->cache.pred_mode[i8];
	int b_split_trans = h->cache.b_split_trans[i_flag_idx];
	int8_t (*bs)[8][16] = h->current.deblock_strength[h->cu_index];

	{//TODO: 计算CU边界的bS
		int xs = x0 >> 3;
		int ys = y0 >> 3;
		int l_w = i_width >> 3;
		int l_h = i_height >> 3;
		int x, y;

		for( y = 0; y < l_h; y ++ )
		{
			int i_neib_mode = h->cache.pred_mode[i8 + y*16 - 1];

			if( i_neib_mode < 0 )
				bs[0][xs][ (y+ys)<<1   ] = 
				bs[0][xs][((y+ys)<<1)+1] = 0;
			else if( i_pred_mode == MODE_INTRA || i_neib_mode == MODE_INTRA )
				bs[0][xs][ (y+ys)<<1   ] = 
				bs[0][xs][((y+ys)<<1)+1] = 4;
			else
			{
				//INTER的CU边界没有区别，在变换处统一处理
			}
		}

		for( x = 0; x < l_w; x ++ )
		{
			int i_neib_mode = h->cache.pred_mode[i8 + x - 16];

			if( i_neib_mode < 0 )
				bs[1][ys][ (x+xs)<<1   ] = 
				bs[1][ys][((x+xs)<<1)+1] = 0;
			else if( i_pred_mode == MODE_INTRA || i_neib_mode == MODE_INTRA )
				bs[1][ys][ (x+xs)<<1   ] = 
				bs[1][ys][((x+xs)<<1)+1] = 4;
			else
			{
				//INTER的CU边界没有区别，在变换处统一处理
			}
		}
	}

	if( i_pred_mode == MODE_INTER && i_part_mode != SIZE_2Nx2N && !b_split_trans )
	{//特殊情况：以PU为边界
		int hw = i_width >> 1;
		int hh = i_height >> 1;
		int x1 = x0 + hw;
		int y1 = y0 + hh;
		int i_idx_step = DEPTH2IDXSTEPC(i_depth);

		if( CBF_LUMA(h->cache.i_cbp[i_flag_idx]) )
		{
			int tmp = h->cache.cu_flag[i8];

			tmp |= DEBLOCK_CBF_ALL;
			lent_fill_rectangle(&h->cache.cu_flag[i8], i_width>>3, i_height>>3, 16, tmp );
		}

		switch( i_part_mode )
		{
		case SIZE_2NxN:
			LENT_block_strength_block( h, i_idx, x0, y0, i_width, hh, 3 );
			i_idx += i_idx_step*2;
			LENT_block_strength_block( h, i_idx, x0, y1, i_width, hh, 1 );
			break;
		case SIZE_Nx2N:
			LENT_block_strength_block( h, i_idx, x0, y0, hw, i_height, 3 );
			i_idx += i_idx_step;
			LENT_block_strength_block( h, i_idx, x1, y0, hw, i_height, 2 );
			break;
		case SIZE_NxN:
			LENT_block_strength_block( h, i_idx, x0, y0, hw, hh, 3 );
			i_idx += i_idx_step;
			LENT_block_strength_block( h, i_idx, x1, y0, hw, hh, 2 );
			i_idx += i_idx_step;
			LENT_block_strength_block( h, i_idx, x0, y1, hw, hh, 1 );
			i_idx += i_idx_step;
			LENT_block_strength_block( h, i_idx, x1, y1, hw, hh, 0 );
		}
	}
	else
		LENT_deblock_strength_trans_tree( h, i_idx, x0, y0, i_log2_size );
}

static void LENT_deblock_strength_tree( HEVCContext *h, int i_idx, int i_pix_x0, int i_pix_y0, int i_log2_size )
{
	int i_width = 1 << i_log2_size;
	int i_height = 1 << i_log2_size;
	int i_pic_width = h->width - (h->cu_x<<6);
	int i_pic_height = h->height - (h->cu_y<<6);
	int i_depth = LOG22DEPTH(i_log2_size);
//	int i_flag_idx = TREEIDX(i_idx,i_log2_size);
	int b_split = h->cache.cu_depth[LENT_scan8[i_idx>>2]]>i_depth;//h->cu.b_split_cu[i_flag_idx];

	if( b_split )
	{
		int i_pix_x1 = i_pix_x0 + (i_width>>1);
		int i_pix_y1 = i_pix_y0 + (i_height>>1);
		int i_idx_step = DEPTH2IDXSTEPC(i_depth);
		
		LENT_deblock_strength_tree( h, i_idx, i_pix_x0, i_pix_y0, i_log2_size-1 );
		i_idx += i_idx_step;
		if( i_pix_x1 < i_pic_width )
			LENT_deblock_strength_tree( h, i_idx, i_pix_x1, i_pix_y0, i_log2_size-1);
		i_idx += i_idx_step;
		if( i_pix_y1 < i_pic_height )
			LENT_deblock_strength_tree( h, i_idx, i_pix_x0, i_pix_y1, i_log2_size-1);
		i_idx += i_idx_step;
		if( i_pix_x1 < i_pic_width && i_pix_y1 < i_pic_height )
			LENT_deblock_strength_tree( h, i_idx, i_pix_x1, i_pix_y1, i_log2_size-1);
	}
	else
		LENT_deblock_strength_cu( h, i_idx, i_pix_x0, i_pix_y0, i_log2_size );
}
void lent_deblock_strength( HEVCContext *h )
{
	int i,j, dir;
	int8_t (*bs)[8][16] = h->current.deblock_strength[h->cu_index];
	int8_t (*deblock_qp)[8] = h->current.deblock_qp[h->cu_index];

	for( i = 0; i < 8; i ++ )
		CP64(deblock_qp[i], &h->cache.qp[LENT_scan8[0] + i*16]);

#if ARCH_X86_32
	if(h->machine>=20)
	{
		_mm_store_si128((__m128i*)bs[0][0] +  0, _mm_set1_epi8(-1));
		_mm_store_si128((__m128i*)bs[0][0] +  1, _mm_set1_epi8(-1));
		_mm_store_si128((__m128i*)bs[0][0] +  2, _mm_set1_epi8(-1));
		_mm_store_si128((__m128i*)bs[0][0] +  3, _mm_set1_epi8(-1));
		_mm_store_si128((__m128i*)bs[0][0] +  4, _mm_set1_epi8(-1));
		_mm_store_si128((__m128i*)bs[0][0] +  5, _mm_set1_epi8(-1));
		_mm_store_si128((__m128i*)bs[0][0] +  6, _mm_set1_epi8(-1));
		_mm_store_si128((__m128i*)bs[0][0] +  7, _mm_set1_epi8(-1));
		_mm_store_si128((__m128i*)bs[0][0] +  8, _mm_set1_epi8(-1));
		_mm_store_si128((__m128i*)bs[0][0] +  9, _mm_set1_epi8(-1));
		_mm_store_si128((__m128i*)bs[0][0] + 10, _mm_set1_epi8(-1));
		_mm_store_si128((__m128i*)bs[0][0] + 11, _mm_set1_epi8(-1));
		_mm_store_si128((__m128i*)bs[0][0] + 12, _mm_set1_epi8(-1));
		_mm_store_si128((__m128i*)bs[0][0] + 13, _mm_set1_epi8(-1));
		_mm_store_si128((__m128i*)bs[0][0] + 14, _mm_set1_epi8(-1));
		_mm_store_si128((__m128i*)bs[0][0] + 15, _mm_set1_epi8(-1));

		LENT_deblock_strength_tree( h, 0, 0, 0, 6 );

		for( dir = 0; dir < 2; dir ++ )
		{
			__m128i x[8], *p = (__m128i*)bs[dir];
			x[0] = _mm_load_si128(p + 0); _mm_store_si128(p + 0, _mm_and_si128(x[0], _mm_cmpgt_epi8(x[0], _mm_setzero_si128())));
			x[1] = _mm_load_si128(p + 1); _mm_store_si128(p + 1, _mm_and_si128(x[1], _mm_cmpgt_epi8(x[1], _mm_setzero_si128())));
			x[2] = _mm_load_si128(p + 2); _mm_store_si128(p + 2, _mm_and_si128(x[2], _mm_cmpgt_epi8(x[2], _mm_setzero_si128())));
			x[3] = _mm_load_si128(p + 3); _mm_store_si128(p + 3, _mm_and_si128(x[3], _mm_cmpgt_epi8(x[3], _mm_setzero_si128())));
			x[4] = _mm_load_si128(p + 4); _mm_store_si128(p + 4, _mm_and_si128(x[4], _mm_cmpgt_epi8(x[4], _mm_setzero_si128())));
			x[5] = _mm_load_si128(p + 5); _mm_store_si128(p + 5, _mm_and_si128(x[5], _mm_cmpgt_epi8(x[5], _mm_setzero_si128())));
			x[6] = _mm_load_si128(p + 6); _mm_store_si128(p + 6, _mm_and_si128(x[6], _mm_cmpgt_epi8(x[6], _mm_setzero_si128())));
			x[7] = _mm_load_si128(p + 7); _mm_store_si128(p + 7, _mm_and_si128(x[7], _mm_cmpgt_epi8(x[7], _mm_setzero_si128())));
		}
		return;
	}
#endif

	memset( bs, -1, sizeof(*bs)*2 );

	LENT_deblock_strength_tree( h, 0, 0, 0, 6 );

	for( dir = 0; dir < 2; dir ++ ) //TODO: 优化？
		for( i = 0; i < 8; i ++ )
			for( j = 0; j < 16; j ++ )
				if( bs[dir][i][j] < 0 )
					bs[dir][i][j] = 0;
}

