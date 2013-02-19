//**********************************************
//hevc_mvpred.c
//Unipipy @2011
//mv prediction
//**********************************************
#include "hevc.h"
#include "hevc_mvpred.h"
#include "hevc_thread.h"
const int i_priority_list0[12] = {0 , 1, 0, 2, 1, 2, 0, 3, 1, 3, 2, 3};
const int i_priority_list1[12] = {1 , 0, 2, 0, 2, 1, 3, 0, 3, 1, 3, 2};


static inline void lent_scale_mv( int16_t mv[2], int i_ref_td, int i_add_ref_td )
{
	if( i_ref_td != i_add_ref_td )
	{//scale mv
		int i_tdb = lent_clip_int8( i_ref_td );
		int i_tdd = lent_clip_int8( i_add_ref_td );
		int i_x   = (0x4000 + LENTABS(i_tdd/2)) / i_tdd;
		int scale = (i_tdb * i_x + 32)>> 6;
		scale = lent_clip( scale, -4096, 4095 );

		i_x = scale * mv[0];
		mv[0] = (i_x + 127 + (i_x < 0)) >> 8;
		i_x = scale * mv[1];
		mv[1] = (i_x + 127 + (i_x < 0)) >> 8;
	}
}

//特别注意：调用这个函数必须保证i8_add对应的块已经编码！！！
static int lent_add_mvp( HEVCContext *h, int list, int16_t mvp[][2], int *i_mvp, int i4, int i4_add, int x, int y )
{
	Picture *fref = h->ref_list[list];
	int i_ref = h->cache.ref[list][i4];
	int i_ref_poc = fref[i_ref].poc;
	int i_ref_add = h->cache.ref[list][i4_add];
	//int i_ref_add_poc;
	int16_t mv[2];
	int b_add = 0;

	if( i_ref_add < 0 ) return 0;

	if( i_ref == i_ref_add )
	{
		mv[0] = h->cache.mv[list][i4_add][0];
		mv[1] = h->cache.mv[list][i4_add][1];
		//lent_clip_mv( h, mv, x, y );

		CP32(mvp[*i_mvp], mv);
		(*i_mvp) ++;
		b_add = 1;
	}
	else
	{
		/*int b_ref_add_avail; //这段代码有问题，需要重写，目的是处理GPB问题
		list = 1 - list;
		fref = list ? h->fref1 : h->fref0;
		i_ref_add = h->cu.cache.ref[list][i4_add];
		b_ref_add_avail = i_ref_add >= 0 && (i_ref_add < list ? h->i_ref1 : h->i_ref0);
		i_ref_add_poc = b_ref_add_avail ? fref[i_ref_add]->i_poc : -1;
		if( i_ref_poc == i_ref_add_poc )
		{
			//TODO: 仅有GPB会经过这里，暂时不支持
			assert( 0 );
		}*/
	}

	return b_add;
}

static int lent_add_mvp_order( HEVCContext *h, int list, int16_t mvp[][2], int *i_mvp, int i4, int i4_add, int x, int y )
{
	Picture *fref = h->ref_list[list];
	int i_poc = h->current_picture.poc;
	int i_ref = h->cache.ref[list][i4];
	int i_ref_poc = fref[i_ref].poc;
	int i_ref_add = h->cache.ref[list][i4_add];
	int i_ref_add_neib = h->cache.ref[1-list][i4_add];//TODO: combine list?
	int16_t mv[2];

	if( i_ref_add < 0 )
	{
		i_ref_add = i_ref_add_neib;
		list = 1 - list;
		fref = h->ref_list[list];
	}

	if( i_ref_add >= 0 )
	{
		int i_add_poc = fref[i_ref_add].poc;

		mv[0] = h->cache.mv[list][i4_add][0];
		mv[1] = h->cache.mv[list][i4_add][1];

		lent_scale_mv( mv, i_poc - i_ref_poc, i_poc - i_add_poc );

		//lent_clip_mv( h, mv, x, y );

		CP32(mvp[*i_mvp], mv);
		(*i_mvp) ++;

		return 1;
	}

	return 0;
}

#define WAIT_LINE(pic,y) lentoid_thread_await_progress((LentFrame*)(pic),LENTMIN((pic)->height-1,LENTMAX(0,y)))
int lent_predict_mv( HEVCContext *h, int i_idx, int x, int y, int width, int height, int i_depth,
					int list, int16_t mvp[][2] )
{
	const int w_step = width >> 2;
	const int h_step = height >> 2;
	int i4 = LENT_scan4[i_idx];
	int i8 = LENT_scan8[i_idx>>2];
	int i_mvp = 0, b_add, b_add_smvp = 0;
	int unit_idx = (y/MIN_UNIT_SIZE)*32+(x/MIN_UNIT_SIZE);
	int8_t *unit_map = &h->cache.unit_exist_map[LENT_scan4[0]];
	int b_top = unit_map[unit_idx-32];
	int b_topleft = unit_map[unit_idx-32-1];
	int b_topright = unit_map[unit_idx-32+width/MIN_UNIT_SIZE];
	int b_left = unit_map[unit_idx-1];
	int b_downleft = unit_map[unit_idx+height/MIN_UNIT_SIZE*32-1];
	int i4_left     = i4 + (h_step-1)*32 - 1;
	int i8_left     = i8 + (h_step/2-1)*16 - 1;
	int i4_downleft = i4 + h_step*32 - 1;
	int i8_downleft = i8 + h_step/2*16 - 1;
	int i4_topleft  = i4 - 32 - 1;
	int i4_top      = i4 - 32 + w_step-1;
	int i4_topright = i4 - 32 + w_step;

	int x_cu = x;
	int y_cu = y;
	int cu_size = 1 << (MAX_CU_LOG2 - i_depth);
	
	if( width < cu_size && x % cu_size )
	{
		x_cu -= width;
		assert( x_cu % cu_size == 0 );
	}
	if( height < cu_size && y % cu_size )
	{
		y_cu -= height;
		assert( y_cu % cu_size == 0 );
	}

	{
		b_add_smvp = (b_left     && h->cache.pred_mode[i8_left]     != MODE_INTRA)
		    ||       (b_downleft && h->cache.pred_mode[i8_downleft] != MODE_INTRA);
	}


	b_add = 0;
	if( b_downleft )
		b_add = lent_add_mvp( h, list, mvp, &i_mvp, i4, i4_downleft, x_cu, y_cu );

	if( b_left && !b_add )
		b_add = lent_add_mvp( h, list, mvp, &i_mvp, i4, i4_left, x_cu, y_cu );

	if( b_downleft && !b_add )
		b_add = lent_add_mvp_order( h, list, mvp, &i_mvp, i4, i4_downleft, x_cu, y_cu );

	if( b_left && !b_add )
		b_add = lent_add_mvp_order( h, list, mvp, &i_mvp, i4, i4_left, x_cu, y_cu );


	b_add = 0;
	if( b_topright )
		b_add = lent_add_mvp( h, list, mvp, &i_mvp, i4, i4_topright, x_cu, y_cu );

	if( b_top && !b_add )
		b_add = lent_add_mvp( h, list, mvp, &i_mvp, i4, i4_top, x_cu, y_cu );

	if( b_topleft && !b_add )
		b_add = lent_add_mvp( h, list, mvp, &i_mvp, i4, i4_topleft, x_cu, y_cu );

	b_add = b_add_smvp;
	if( i_mvp == 2 ) b_add = 1;

	if( b_topright && !b_add ) 
		b_add = lent_add_mvp_order( h, list, mvp, &i_mvp, i4, i4_topright, x_cu, y_cu );

	if( b_top && !b_add )
		b_add = lent_add_mvp_order( h, list, mvp, &i_mvp, i4, i4_top, x_cu, y_cu );

	if( b_topleft && !b_add )
		b_add = lent_add_mvp_order( h, list, mvp, &i_mvp, i4, i4_topleft, x_cu, y_cu );


  // unique mvp
  if( i_mvp > 1 )
  {
    int i, j;
    int n = 1;

    for( i = 1; i < i_mvp; i ++ )
    {
      for( j = n - 1; j >= 0; j -- )
        if( M32(mvp[i]) == M32(mvp[j]) )
          break;
      if( j < 0 )
        M32(mvp[n++]) = M32(mvp[i]);
    }
    i_mvp = n;
  }

	if( h->sh.m_enableTMVPFlag )
	{
		int i_ref = h->cache.ref[list][i4];
		Picture *fref = list ? h->ref_list[1] : h->ref_list[0];
		int ref_x, ref_y, i_ref_offset, i_ref_ref;
		int i_poc = h->sh.m_iPOC;
		int i_ref_poc = fref[i_ref].poc;
		int i_add_poc;
		int i_add_ref_poc;
		int b_check_center = 0;

		ref_x = h->cu_x*16 + x/4 + w_step;
		ref_y = h->cu_y*16 + y/4 + h_step;
		if( (ref_x<<2) < h->width && (ref_y<<2) < h->height
      && (y+height) < 64 )
		{
		}
		else 
		{
			ref_x = h->cu_x*16 + x/4 + w_step/2;
			ref_y = h->cu_y*16 + y/4 + h_step/2;
			b_check_center = 1;
		}
		ref_x &= ~3;
		ref_y &= ~3;

#if 1
		if( h->i_ref_count[1] && !h->sh.m_colFromL0Flag  )
		{
			fref = h->ref_list[1];
			list = 0;
		}
		else
		{
			fref = h->ref_list[0];
			list = 1;
		}
#else
		fref = list ? h->fref1 : h->fref0;
#endif

		i_ref_offset = ref_y*h->b4_stride + ref_x;
		
		WAIT_LINE(fref+0,h->cu_y*64+64-DRAW_EDGE_DELAY);
		i_ref_ref = i_ref < 0 ? -2 :
			fref[0].ref_index[list] ? fref[0].ref_index[list][i_ref_offset] : -2;
		if( i_ref_ref < 0 && fref[0].ref_count[1-list] > 0 )
		{
			list = 1 - list;
			i_ref_ref = i_ref < 0 ? -2 : fref[0].ref_index[list][i_ref_offset];
		}

		if( i_ref_ref < 0 && !b_check_center )
		{
			ref_x = h->cu_x*16 + x/4 + w_step/2;// - ((w_step+1)/2);
			ref_y = h->cu_y*16 + y/4 + h_step/2;// - ((h_step+1)/2);
			ref_x &= ~3;
			ref_y &= ~3;
			i_ref_offset = ref_y*(h->b4_stride) + ref_x;

			if( h->i_ref_count[1] && !h->sh.m_colFromL0Flag )
			{
				fref = h->ref_list[1];
				list = 0;
			}
			else
			{
				fref = h->ref_list[0];
				list = 1;
			}
			
			WAIT_LINE(fref+0,h->cu_y*64+64-DRAW_EDGE_DELAY);
			i_ref_ref = i_ref < 0 ? -2 :
				fref[0].ref_index[list] ? fref[0].ref_index[list][i_ref_offset] : -2;
			if( i_ref_ref < 0 && fref[0].ref_count[1-list] > 0 )
			{
				list = 1 - list;
				i_ref_ref = i_ref < 0 ? -2 : fref[0].ref_index[list][i_ref_offset];
			}
		}

		if( i_ref >= 0 && i_ref_ref >= 0 )
		{
			int16_t mv[2];

			i_add_poc = fref[0].poc;
			i_add_ref_poc = fref[0].ref_poc[list][i_ref_ref];

			CP32(mv, fref[0].motion_val[list][i_ref_offset]);
			lent_scale_mv( mv, i_poc - i_ref_poc, i_add_poc - i_add_ref_poc );
			//LENT_clip_mv( h, mv, x_cu, y_cu );
			M32(mvp[i_mvp ++]) = M32(mv);
		}
	}

	if( (i_mvp == 1 && M32(mvp[0]) != 0) || i_mvp == 0 )
		M32(mvp[i_mvp ++]) = 0;

	return LENTMIN(i_mvp,2);
}

static inline int LENT_add_mvp_merge( HEVCContext *h, int i4_add, int *i_mvp,
							   int16_t mvp0[][2], int16_t mvp1[][2], int8_t ref0[], int8_t ref1[] )
{
	int8_t i_ref_add0 = h->cache.ref[0][i4_add];
	int8_t i_ref_add1 = h->cache.ref[1][i4_add];
	
	if( i_ref_add0 < 0 && i_ref_add1 < 0 ) return 0;

	CP32( mvp0[*i_mvp], h->cache.mv[0][i4_add] );
	CP32( mvp1[*i_mvp], h->cache.mv[1][i4_add] );
	ref0[*i_mvp] = i_ref_add0;
	ref1[*i_mvp] = i_ref_add1;
	(*i_mvp) ++;
	return 1;
}

static inline void LENT_del_equal_mvp( int *i_mvp, int m0, int m1, int16_t mvp0[][2], int16_t mvp1[][2], int8_t ref0[], int8_t ref1[] )
{
	if( *i_mvp < 2 ) return;

	if( ref0[m0] == ref0[m1] && ref1[m0] == ref1[m1] )
	{
		if( (ref0[m0] >= 0 && M32(mvp0[m0]) == M32(mvp0[m1]) || ref0[m0] < 0 ) &&
			(ref1[m0] >= 0 && M32(mvp1[m0]) == M32(mvp1[m1]) || ref1[m0] < 0 ) )
			(*i_mvp) --;
	}
}

#define COMP_RET if(i_mvp == i_mvp_idx) return i_mvp_idx
int LENT_predict_mv_merge( HEVCContext *h, int i_idx, int x, int y, int width, int height, int i_depth,
						  int16_t mvp0[][2], int16_t mvp1[][2], int8_t ref0[], int8_t ref1[], int i_mvp_idx )
{
	const int w_step = width >> 2;
	const int h_step = height >> 2;
	const int b_sqr = width == height;
	const int b_2NxN = width > height;
	const int b_Nx2N = width < height;
	int i4 = LENT_scan4[i_idx];
	int i_mvp = 0, b_add_top, b_add_left, b_top_equal_left, b_add, i, max_ref;
	int unit_idx = (y/MIN_UNIT_SIZE)*32+(x/MIN_UNIT_SIZE);
	int8_t *unit_map = &h->cache.unit_exist_map[LENT_scan4[0]];
	int b_top = unit_map[unit_idx-32];
	int b_topleft = unit_map[unit_idx-32-1];
	int b_topright = unit_map[unit_idx-32+width/MIN_UNIT_SIZE];
	int b_left = unit_map[unit_idx-1];
	int b_downleft = unit_map[unit_idx+height/MIN_UNIT_SIZE*32-1];
	int b_bi_dir = h->sh.m_eSliceType == SLICE_B;
	int i4_left     = i4 + (h_step-1)*32 - 1;
	int i4_downleft = i4 + h_step*32 - 1;
	int i4_topleft  = i4 - 32 - 1;
	int i4_top      = i4 - 32 + w_step-1;
	int i4_topright = i4 - 32 + w_step;

	if( !(b_Nx2N && x % height != 0 ) && b_left )
	{
		b_add_left = LENT_add_mvp_merge( h, i4_left, &i_mvp, mvp0, mvp1, ref0, ref1 );
		COMP_RET;
	}

	if( !(b_2NxN && y % width  != 0 ) && b_top  )
	{
		int i_mvp_bak = i_mvp;

		b_add_top = LENT_add_mvp_merge( h, i4_top, &i_mvp, mvp0, mvp1, ref0, ref1 );
		LENT_del_equal_mvp( &i_mvp, 0, 1, mvp0, mvp1, ref0, ref1 );
		b_top_equal_left = b_add_top && i_mvp == i_mvp_bak;
		COMP_RET;
	}

	if( b_topright )
	{
		b_add = LENT_add_mvp_merge( h, i4_topright, &i_mvp, mvp0, mvp1, ref0, ref1 );
		if( b_add && b_add_top )
			LENT_del_equal_mvp( &i_mvp, i_mvp - 2, i_mvp - 1, mvp0, mvp1, ref0, ref1 );
		COMP_RET;
	}

	if( b_downleft )
	{
		b_add = LENT_add_mvp_merge( h, i4_downleft, &i_mvp, mvp0, mvp1, ref0, ref1 );
		if( b_add && b_add_left )
			LENT_del_equal_mvp( &i_mvp, 0, i_mvp - 1, mvp0, mvp1, ref0, ref1 );
		COMP_RET;
	}

	if( i_mvp < 4 && b_topleft )
	{
		int i_mvp_bak = i_mvp;

		b_add = LENT_add_mvp_merge( h, i4_topleft, &i_mvp, mvp0, mvp1, ref0, ref1 );
		if( b_add && b_add_left )
			LENT_del_equal_mvp( &i_mvp, 0, i_mvp - 1, mvp0, mvp1, ref0, ref1 );
		if( b_add && b_add_top && i_mvp != i_mvp_bak )//if add_left top_idx = 1
			LENT_del_equal_mvp( &i_mvp, b_add_left && !b_top_equal_left, i_mvp - 1, mvp0, mvp1, ref0, ref1 );
		COMP_RET;
	}

	if( h->sh.m_enableTMVPFlag )
	{
    int dir;


    b_add = 0;
    for( dir = 0; dir <= b_bi_dir; dir ++ )
    {
      int list = dir;
      int i_ref = 0;
	  Picture *fref = list ? h->ref_list[1] : h->ref_list[0];
      int ref_x, ref_y, i_ref_offset, i_ref_ref;
      int i_poc = h->sh.m_iPOC;
      int i_ref_poc = fref[i_ref].poc;
      int i_add_poc;
      int i_add_ref_poc;
      int b_check_center = 0;
      int16_t *mvp = list ? mvp1[i_mvp] : mvp0[i_mvp];
      int8_t *ref = list ? &ref1[i_mvp] : &ref0[i_mvp];

      ref_x = h->cu_x*16 + x/4 + w_step;
      ref_y = h->cu_y*16 + y/4 + h_step;
      if( (ref_x<<2) < h->width && (ref_y<<2) < h->height
        && (y + height) < 64 )
      {
      }
      else 
      {
		ref_x = h->cu_x*16 + x/4 + w_step/2;
		ref_y = h->cu_y*16 + y/4 + h_step/2;
        b_check_center = 1;
      }
      ref_x &= ~3;
      ref_y &= ~3;

      if( h->i_ref_count[1] && !h->sh.m_colFromL0Flag )
      {
        fref = h->ref_list[1];
        list = 0;
      }
      else
      {
        fref = h->ref_list[0];
        list = 1;
      }
	  
	  WAIT_LINE(fref+0,h->cu_y*64+64-DRAW_EDGE_DELAY);
      i_ref_offset = ref_y*(h->b4_stride) + ref_x;
      i_ref_ref = i_ref < 0 ? -2 :
        fref[0].ref_index[list] ? fref[0].ref_index[list][i_ref_offset] : -2;
      if( i_ref_ref < 0 && fref[0].ref_count[1-list] > 0 )
      {
        list = 1 - list;
        i_ref_ref = i_ref < 0 ? -2 : fref[0].ref_index[list][i_ref_offset];
      }

      if( i_ref_ref < 0 && !b_check_center )
      {
        ref_x = h->cu_x*16 + x/4 + w_step/2;
        ref_y = h->cu_y*16 + y/4 + h_step/2;
        ref_x &= ~3;
        ref_y &= ~3;
        i_ref_offset = ref_y*(h->b4_stride) + ref_x;

        if( h->i_ref_count[1] && !h->sh.m_colFromL0Flag )
        {
          fref = h->ref_list[1];
          list = 0;
        }
        else
        {
          fref = h->ref_list[0];
          list = 1;
        }
		
		WAIT_LINE(fref+0,h->cu_y*64+64-DRAW_EDGE_DELAY);
        i_ref_ref = i_ref < 0 ? -2 :
			fref[0].ref_index[list] ? fref[0].ref_index[list][i_ref_offset] : -2;
        if( i_ref_ref < 0 && fref[0].ref_count[1-list] > 0 )
        {
          list = 1 - list;
          i_ref_ref = i_ref < 0 ? -2 : fref[0].ref_index[list][i_ref_offset];
        }
      }

      if( i_ref >= 0 && i_ref_ref >= 0 )
      {
        int16_t mv[2];

        i_add_poc = fref[0].poc;
        i_add_ref_poc = fref[0].ref_poc[list][i_ref_ref];

        CP32(mv, fref[0].motion_val[list][i_ref_offset]);
        lent_scale_mv( mv, i_poc - i_ref_poc, i_add_poc - i_add_ref_poc );
        //LENT_clip_mv( h, mv, x_cu, y_cu );
        M32(mvp) = M32(mv);
        *ref = i_ref;
        b_add = 1;
      }
      else
        *ref = -2;
    }

    if( !b_bi_dir && b_add )
      ref1[i_mvp] = -2;

    i_mvp += b_add;
	COMP_RET;
	}

	if( b_bi_dir )
	{
		int total = i_mvp * (i_mvp - 1);

		for( i = 0; i < total && i_mvp < 5; i ++ )
		{
			int l0 = i_priority_list0[i];
			int l1 = i_priority_list1[i];

			if( l0 < i_mvp && l1 < i_mvp && ref0[l0] >= 0 && ref1[l1] >= 0 )
			{
				CP32( mvp0[i_mvp], mvp0[l0] );
				CP32( mvp1[i_mvp], mvp1[l1] );
				ref0[i_mvp] = ref0[l0];
				ref1[i_mvp] = ref1[l1];
				i_mvp ++;
				COMP_RET;
			}
		}
	}

	max_ref = b_bi_dir ? LENTMIN( h->i_ref_count[0], h->i_ref_count[1] ) : h->i_ref_count[0];
	for( i = 0; i < max_ref && i_mvp < 5; i ++ )
	{
		M32( mvp0[i_mvp] ) = 0;
		M32( mvp1[i_mvp] ) = 0 ;
		ref0[i_mvp] = i;
		ref1[i_mvp] = b_bi_dir ? i : -2;
		i_mvp ++;	
		COMP_RET;
	}

	return i_mvp;
}