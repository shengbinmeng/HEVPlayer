//**********************************************
//hevc_cabac.c
//Unipipy @2011
//cabac functions to decode syntaxes
//**********************************************
#include "hevc.h"
#include "hevc_pred.h"
#include "hevc_mvpred.h"
#include "hevc_cabac.h"
#include "hevc_deblock.h"
#include "hevc_compatibility.h"
#if ARCH_X86_32
#include <tmmintrin.h>
#endif

/////////////////////////////////////////////////////////////////////////////////////////////////
//cabac ctx表 acc to HM 6.2
/////////////////////////////////////////////////////////////////////////////////////////////////
/******** 这里定义cabac每个位置对应的context ********/

static const uint8_t LENT_cabac_context_init_I[MAX_CABAC_STATE] = 
{
	//SPLIT_FLAG
	139,  141,  157, 

	//SKIP_FLAG
	CNU,  CNU,  CNU,  

	//MERGE_FLAG_EXT
	CNU, 

	//MERGE_IDX_EXT
	CNU, 

	//PART_SIZE
	184,  CNU,  CNU,  CNU, 

	//CU_AMP_POS
	CNU,

	//PRED_MODE
	CNU, 

	//INTRA_PRED_MODE
	184, 

	//CHROMA_PRED_MODE
	63,  139, 

	//INTER_DIR
	CNU,  CNU,  CNU,  CNU,  CNU, 

	//MVD
	CNU,  CNU, 

	//REF_PIC
	CNU,  CNU, 

	//TRANS_SUBDIV_FLAG
	153,  138,  138,

	//QT_CBF
	111,  141,  CNU,  CNU,  CNU,   94,  138,  182,  CNU,  CNU, 

	//QT_ROOT_CBF
	CNU, 

	//DELTA_QP
	154,  154,  154,  

	//SIG_CG_FLAG
	91,  171,  
	134,  141, 

	//SIG_FLAG_LUMA
	111,  111,  125,  110,  110,   94,  124,  108,  124,
	107,  125,  141,  179,  153,  125,  107,  125,  141,
	179,  153,  125,  107,  125,  141,  179,  153,  125,

	//SIG_FLAG_CHROMA
	140,  139,  182,  182,  152,
	136,  152,  136,  153,  136,
	139,  111,  136,  139,  111,

	//LAST_FLAG_X
	110,  110,  124,  125,  140,  153,  125,  127,  140,  109,  111,  143,  127,  111,   79, 
	108,  123,   63,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU, 

	//LAST_FLAG_Y
	110,  110,  124,  125,  140,  153,  125,  127,  140,  109,  111,  143,  127,  111,   79, 
	108,  123,   63,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU, 

	//ONE_FLAG
	140,   92,  137,  138,  140,  152,  138,  139,
	153,   74,  149,   92,  139,  107,  122,  152,
	140,  179,  166,  182,  140,  227,  122,  197,

	//ABS_FLAG
	138,  153,  136,  167,  152,  152,

	//MVP_IDX
	CNU,  CNU,

	//SAO_MERGE_FLAG_CTX
	153,
	
	//SAO_TYPE_IDX_CTX
	200,
	
	//OFS_TRANSFORMSKIP_FLAG_CTX
	139,  139,

	//CU_TRANSQUANT_BYPASS_FLAG_CTX
	154,

};

static const uint8_t LENT_cabac_context_init_P[MAX_CABAC_STATE] = 
{
	//SPLIT_FLAG
	107,  139,  126, 

	//SKIP_FLAG
	197,  185,  201, 

	//MERGE_FLAG_EXT
	110, 

	//MERGE_IDX_EXT
	122, 

	//PART_SIZE
	154,  139,  CNU,  CNU,

	//CU_AMP_POS
	154,

	//PRED_MODE
	149,

	//INTRA_PRED_MODE
	154,

	//CHROMA_PRED_MODE
	152,  139,

	//INTER_DIR
	95,   79,   63,   31,  31,

	//MVD
	140,  198,

	//REF_PIC
	153,  153, 

	//TRANS_SUBDIV_FLAG
	124,  138,   94,

	//QT_CBF
	153,  111,  CNU,  CNU,  CNU,  149,  107,  167,  CNU,  CNU,

	//QT_ROOT_CBF
	79,

	//DELTA_QP
	154,  154,  154,  

	//SIG_CG_FLAG
	121,  140, 
	61,  154,  

	//SIG_FLAG_LUMA
	155,  154,  139,  153,  139,  123,  123,   63,  153,
	166,  183,  140,  136,  153,  154,  166,  183,  140,
	136,  153,  154,  166,  183,  140,  136,  153,  154,

	//SIG_FLAG_CHROMA
	170,  153,  123,  123,  107,
	121,  107,  121,  167,  151,
	183,  140,  151,  183,  140,

	//LAST_FLAG_X
	125,  110,   94,  110,   95,   79,  125,  111,  110,   78,  110,  111,  111,   95,   94,
	108,  123,  108,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,

	//LAST_FLAG_Y
	125,  110,   94,  110,   95,   79,  125,  111,  110,   78,  110,  111,  111,   95,   94,
	108,  123,  108,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,

	//ONE_FLAG
	154,  196,  196,  167,  154,  152,  167,  182,
	182,  134,  149,  136,  153,  121,  136,  137,
	169,  194,  166,  167,  154,  167,  137,  182,

	//ABS_FLAG
	107,  167,   91,  122,  107,  167,

	//MVP_IDX
	168,  CNU,

	//SAO_MERGE_FLAG_CTX
	153,
	
	//SAO_TYPE_IDX_CTX
	185,

	//OFS_TRANSFORMSKIP_FLAG_CTX
	139,  139,

	//CU_TRANSQUANT_BYPASS_FLAG_CTX
	154,
};

static const uint8_t LENT_cabac_context_init_B[MAX_CABAC_STATE] = 
{
	//SPLIT_FLAG
	107,  139,  126, 

	//SKIP_FLAG
	197,  185,  201, 

	//MERGE_FLAG_EXT
	154,

	//MERGE_IDX_EXT
	137,

	//PART_SIZE
	154,  139,  CNU,  CNU,

	//CU_AMP_POS
	154,

	//PRED_MODE
	134,

	//INTRA_PRED_MODE
	183,

	//CHROMA_PRED_MODE
	152,  139,

	//INTER_DIR
	95,   79,   63,   31,  31,

	//MVD
	169,  198,

	//REF_PIC
	153,  153, 

	//TRANS_SUBDIV_FLAG
	224,  167,  122, 

	//QT_CBF
	153,  111,  CNU,  CNU,  CNU,  149,   92,  167,  CNU,  CNU,

	//QT_ROOT_CBF
	79,

	//DELTA_QP
	154,  154,  154,  

	//SIG_CG_FLAG
	121,  140, 
	61,  154,  

	//SIG_FLAG_LUMA
	170,  154,  139,  153,  139,  123,  123,   63,  124,
	166,  183,  140,  136,  153,  154,  166,  183,  140,
	136,  153,  154,  166,  183,  140,  136,  153,  154,
	
	//SIG_FLAG_CHROMA
	170,  153,  138,  138,  122,
	121,  122,  121,  167,  151,
	183,  140,  151,  183,  140,

	//LAST_FLAG_X
	125,  110,  124,  110,   95,   94,  125,  111,  111,   79,  125,  126,  111,  111,   79,
	108,  123,   93,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU, 

	//LAST_FLAG_Y
	125,  110,  124,  110,   95,   94,  125,  111,  111,   79,  125,  126,  111,  111,   79,
	108,  123,   93,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU, 

	//ONE_FLAG
	154,  196,  167,  167,  154,  152,  167,  182,
	182,  134,  149,  136,  153,  121,  136,  122,
	169,  208,  166,  167,  154,  152,  167,  182,

	//ABS_FLAG
	107,  167,   91,  107,  107,  167,

	//MVP_IDX
	168,  CNU,

	//SAO_MERGE_FLAG_CTX
	153,
	
	//SAO_TYPE_IDX_CTX
	160,
	
	//OFS_TRANSFORMSKIP_FLAG_CTX
	139,  139,

	//CU_TRANSQUANT_BYPASS_FLAG_CTX
	154,
};
const static int last_xy_group_idx[32] = 
{
	0,1,2,3,4,4,5,5,
	6,6,6,6,7,7,7,7,
	8,8,8,8,8,8,8,8,
	9,9,9,9,9,9,9,9
};
const static int min_in_group[11] = 
{
	0,1,2,3,4,6,8,12,16,24,10000
};

const static int scan_type[4][36] = 
{
	{
		0, 0, 0, 0, 0, 0, 2, 2, 2, 
		2, 2, 2, 2, 2, 2, 0, 0, 0, 
		0, 0, 0, 0, 1, 1, 1, 1, 1, 
		1, 1, 1, 1, 0, 0, 0, 0, 0, 
	},
	{
		0, 0, 0, 0, 0, 0, 2, 2, 2, 
		2, 2, 2, 2, 2, 2, 0, 0, 0, 
		0, 0, 0, 0, 1, 1, 1, 1, 1, 
		1, 1, 1, 1, 0, 0, 0, 0, 0, 
	},
	{
		0, 0, 0, 0, 0, 0, 0, 0, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 
	},
	{
		0, 0, 0, 0, 0, 0, 0, 0, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 
	},

};



/////////////////////////////////////////////////////////////////////////////////////////////////
//内部函数
/////////////////////////////////////////////////////////////////////////////////////////////////

static lent_always_inline void decode_predtype(HEVCContext *h,int i_log2_size,int treeidx,int i8)
{
	int ctx = OFS_PART_SIZE_CTX;
	int slice_type = h->sh.m_eSliceType;
	int part_mode;
	int pred_mode;
	//int block_min = h->sps.i_log2_block_min;
	//int size_type = i_log2_size > block_min ? 0 : i_log2_size == 3 ? 1 : 2;

	static const int cabac_bin[3][12][2] = 
	{
		{
			{1, 1}, {2, 1}, {2, 0}, {-1, -1}, {-1, -1}, {-1, -1},
			{0, 1}, {1, 1}, {2, 1}, {-1, -1}, { 2,  0}, {-1, -1},
		},
		{
			{1, 1}, {2, 1}, {2, 0}, {-1, -1}, {-1, -1}, {-1, -1},
			{0, 1}, {1, 1}, {2, 1}, {-1, -1}, { 3,  1}, { 3,  0},
		},
		{
			{1, 1}, {2, 1}, {3, 1}, { 3,  0}, {-1, -1}, {-1, -1},
			{0, 1}, {1, 1}, {2, 1}, { 3,  1}, { 4,  1}, { 4,  0},
		}
	};

	//assert( i_log2_size >= block_min );

	if( slice_type == SLICE_P || slice_type == SLICE_B)//pipy 2012.5.7 HM4->HM6
	{
		if(get_cabac_noinline(&h->cabac,&h->cabac.cabac_state[OFS_PRED_MODE_CTX]))
			pred_mode=MODE_INTRA;
		else
			pred_mode=MODE_INTER;
	}
	else
		pred_mode=MODE_INTRA;

	if(pred_mode == MODE_INTRA)
	{
		if(i_log2_size==h->sps.m_uiCULog2MinSize)
			if(get_cabac_noinline(&h->cabac,&h->cabac.cabac_state[ctx]))
				part_mode=SIZE_2Nx2N;
			else
				part_mode=SIZE_NxN;
		else
			part_mode=SIZE_2Nx2N;
	}
	else
	{
		int i_bits = 2;// i_log2_size == 3 ? 2 : 3;//todo extend to the standard
		int i_depth = MAX_CU_LOG2 - i_log2_size;

		for(part_mode=0;part_mode<i_bits;part_mode++)
			if(get_cabac_noinline(&h->cabac,&h->cabac.cabac_state[ctx++]))
				break;
		/*
		if( slice_type == SLICE_B && part_mode == 3 )
		{
			pred_mode = MODE_INTRA;
			if( i_depth == 4 )
			{
				assert( 0 );//not support
			}
			if( i_depth == 3)
				assert(!get_cabac_noinline(&h->cabac,&h->cabac.cabac_state[ctx++]));

			if(i_depth == 3 && get_cabac_noinline(&h->cabac,&h->cabac.cabac_state[ctx++]))
				part_mode = SIZE_NxN;
			else
				part_mode = SIZE_2Nx2N;
		}
		else if(i_bits==2&& slice_type == SLICE_B && part_mode == 2 )
		{
			int tmp=0;
			pred_mode = MODE_INTRA;

			if(get_cabac_noinline(&h->cabac,&h->cabac.cabac_state[ctx++]))
				assert(0);
			else
			{//转换成intra
				if(i_depth==3 && get_cabac_noinline(&h->cabac,&h->cabac.cabac_state[ctx++]))
				{
					part_mode = SIZE_NxN;
				}
				else
					part_mode = SIZE_2Nx2N;
			}
		}*/
//pipy 2012.5.7 HM4->HM6
		//todo AMP//pipy 2012.5.7 HM4->HM6
	}
	h->cache.part_mode[treeidx]	=part_mode;
	lent_fill_rectangle(&h->cache.pred_mode[i8],LOG22I8WIDTH(i_log2_size),LOG22I8WIDTH(i_log2_size),16,pred_mode);
}
static lent_always_inline void decode_prediction_intra(HEVCContext *h,int i_log2_size,int i_idx)//TODO: optimization
{
	int i4;
	int i_luma_mode=0;
	int i_part_mode = h->cache.part_mode[TREEIDX(i_idx,i_log2_size)];
	int i,j;
	int i_last = (i_part_mode == SIZE_NxN ? 4 : 1);
	int i_idx_bak = i_idx;
	int m[3]={-1,-1,-1},mpm[4];

	
	if(i_part_mode == SIZE_NxN)
		i_log2_size--;
	
	for( i = 0, i_idx = i_idx_bak; i < i_last; i_idx ++, i ++ )
	{
		mpm[i]=get_cabac(&h->cabac,&h->cabac.cabac_state[OFS_INTRA_PRED_MODE_CTX]);
	}
	for( i = 0, i_idx = i_idx_bak; i < i_last; i_idx ++, i ++ )
	{
		int i_top_mode,i_left_mode;
		i4 = LENT_scan4[i_idx];

		i_top_mode = h->cache.intra_pred_mode[i4 - 32];
		i_left_mode = h->cache.intra_pred_mode[i4 - 1];

		if( i_top_mode < 0 ) i_top_mode = DC_IDX;
		if( i_left_mode < 0 ) i_left_mode = DC_IDX;

		if( i_left_mode == i_top_mode )
		{
			if(i_left_mode > DC_IDX)//angular
			{
				m[0]=i_left_mode;
				m[1]=(i_left_mode+29)%32+2;
				m[2]=(i_left_mode-1)%32+2;
			}
			else{
				m[0] = PLANAR_IDX;
				m[1] = DC_IDX;
				m[2] = VER_IDX; 
			}
		}
		else
		{
			m[0]=i_left_mode;
			m[1]=i_top_mode;
			if(i_left_mode&&i_top_mode)
				m[2]=PLANAR_IDX;
			else
			{
				m[2]=(i_left_mode+i_top_mode)<2? VER_IDX : DC_IDX;
			}
		}
		
		//mpm=get_cabac(&h->cabac,&h->cabac.cabac_state[OFS_INTRA_PRED_MODE_CTX]);

		if( mpm[i] )
		{
			for(j=0; j<2; j++)
				if(!get_cabac_bypass(&h->cabac))
					break;
			i_luma_mode=m[j];
		}
		else
		{
			int /*i_diff,*/i_rem=0;
			for(j=4;j>=0;j--)
				i_rem += get_cabac_bypass( &h->cabac) << j;

#define SORT(x,y) if((x)>(y)){int tmp=(x);(x)=(y);(y)=tmp;}
			SORT(m[0],m[1]);
			SORT(m[1],m[2]);
			SORT(m[0],m[1]);
#undef SORT

			for(j=0;j<3;j++)
				if(i_rem>=m[j])
					i_rem++;
			i_luma_mode=i_rem;
		}
		lent_fill_rectangle(&h->cache.intra_pred_mode[i4],LOG22I4WIDTH(i_log2_size),LOG22I4WIDTH(i_log2_size),32,i_luma_mode);
	}
}

static lent_always_inline void decode_prediction_chroma(HEVCContext *h,int i_log2_size,int idx)
{
	int chroma_idx	= idx>>2;
	int i4_chroma	= LENT_scan4[chroma_idx+256];
	int chroma_mode	= 0;
	int i, cmax		= 3;
	if(!get_cabac_noinline(&h->cabac,&h->cabac.cabac_state[OFS_CHROMA_PRED_CTX]))
	{
		lent_fill_rectangle(&h->cache.intra_pred_mode[i4_chroma],LOG22I4WIDTH(i_log2_size)/2,LOG22I4WIDTH(i_log2_size)/2,32,CHROMA_FROM_LUMA);
		return;
	}
	chroma_mode =0;
	for( i = 2 - 1; i >= 0; i -- )//todo 改为一次调用
		chroma_mode|=get_cabac_bypass( &h->cabac )<<i;
	lent_fill_rectangle(&h->cache.intra_pred_mode[i4_chroma],LOG22I4WIDTH(i_log2_size)/2,LOG22I4WIDTH(i_log2_size)/2,32,chroma_mode);

}
static lent_always_inline void lent_decode_mvd(HEVCContext *h, int16_t mvd[2] )//todo 修改
{

	mvd[0]=get_cabac(&h->cabac,&h->cabac.cabac_state[OFS_MV_RES_CTX]);
	mvd[1]=get_cabac(&h->cabac,&h->cabac.cabac_state[OFS_MV_RES_CTX]);


	if( mvd[0])
		mvd[0]+=get_cabac(&h->cabac,&h->cabac.cabac_state[OFS_MV_RES_CTX+1]);

	if( mvd[1])
		mvd[1]+=get_cabac(&h->cabac,&h->cabac.cabac_state[OFS_MV_RES_CTX+1]);

	if( mvd[0] > 0 )
	{
		if( mvd[0] > 1 )
			mvd[0]+=get_cabac_ue_bypass(&h->cabac,1);
		if(get_cabac_bypass(&h->cabac))
			mvd[0]=-mvd[0];
	}
	if( mvd[1] > 0 )
	{
		if( mvd[1] > 1 )
			mvd[1]+=get_cabac_ue_bypass(&h->cabac,1);
		if(get_cabac_bypass(&h->cabac))
			mvd[1]=-mvd[1];
	}
}

static lent_always_inline int lent_decode_mvp_idx( HEVCContext *h, int b_merge)
{
	int ctx= b_merge ? OFS_MERGE_IDX_EXT_CTX : OFS_MVP_IDX_CTX;
	int mvp_idx=0;
	if(b_merge)
	{
		if(mvp_idx=get_cabac(&h->cabac,&h->cabac.cabac_state[ctx]))
			for(;mvp_idx<h->sh.m_maxNumMergeCand-1;mvp_idx++)
				if(!get_cabac_bypass(&h->cabac))
			 		break;
		return mvp_idx;
	}
	else
		return get_cabac(&h->cabac,&h->cabac.cabac_state[ctx]);
}
int decode_ref_idx(HEVCContext *h, int i_max)
{
	int i_ref;
	{
		if(i_max>0)
		{
			if( get_cabac_noinline(&h->cabac,&h->cabac.cabac_state[OFS_REF_NO_CTX]) )//i_ref[i]>0
			{
				if( i_max > 1 )
				{
					if(get_cabac_noinline(&h->cabac,&h->cabac.cabac_state[OFS_REF_NO_CTX+1]))//i_ref[i]>1
					{
						for(i_ref=2;i_ref<i_max;i_ref++)
						{
							if(!get_cabac_bypass(&h->cabac))
								break;
						}
					}
					else
						i_ref=1;
				}
				else
					i_ref=1;
			}
			else
				i_ref=0;
		}
		else
			i_ref=0;

	}
	return i_ref;
}
static lent_always_inline void lent_decode_motion( HEVCContext *h, int i_idx,
					 int i_pix_x0, int i_pix_y0, int i_width, int i_height, int i_depth )
{
	//TODO:
	int i8 = LENT_scan8[i_idx >> 2];
	int i4 = LENT_scan4[i_idx];
	int b_merge;
	int i;

	//if( h->sps.b_MRG )
	{
		b_merge = h->cache.b_merge = get_cabac(&h->cabac,&h->cabac.cabac_state[OFS_MERGE_FLAG_EXT_CTX]);
	}

	if( b_merge )
	{
		int16_t mvp[2][5][2], mv[2];
		int8_t ref[2][5];
		int i_mvp,i_mvp_idx;
		int list_count = h->sh.m_eSliceType == SLICE_B ? 2 : 1;

		i_mvp_idx=lent_decode_mvp_idx( h, b_merge ); //add by ckj
		i_mvp = LENT_predict_mv_merge( h, i_idx, i_pix_x0, i_pix_y0, i_width, i_height, i_depth,
			mvp[0], mvp[1], ref[0], ref[1], i_mvp_idx+1 );
		//i_mvp_idx=lent_decode_mvp_idx( h, b_merge );

		for( i = 0; i < list_count; i ++ )
		{
			if(ref[i][i_mvp_idx]>=0)
			{
				mv[0]=mvp[i][i_mvp_idx][0];
				mv[1]=mvp[i][i_mvp_idx][1];


				lent_fill_rectangle(&h->cache.ref[i][i4],i_width/4,i_height/4,32,ref[i][i_mvp_idx]);
				{//todo optimize 不能使用fill_rectangle因为它是8位的
					int x,y;
					int32_t *p=(int32_t *)h->cache.mv[i][i4];
					for(y=0;y<i_height/4;y++)
						for(x=0;x<i_width/4;x++)
							p[y*32+x]=M32(mv);
				}
			}

		}

	}
	else
	{
		int i_ref[2]={-1,-1};// = h->cu.cache.ref[0][i8];
		int16_t mv[2], mvp[3][2], mvd[2];
		int mvp_idx;
		if( h->sh.m_eSliceType == SLICE_B )
		{
			if(get_cabac_noinline(&h->cabac,&h->cabac.cabac_state[OFS_INTER_DIR_CTX+i_depth]))
			{
				i_ref[0]=i_ref[1]=0;
			}
			else
			{
				i_ref[0]=0;
			}
		}
		else
			i_ref[0]=0;

		if(i_ref[1]>=0)
		{
			i_ref[0]=decode_ref_idx(h,h->i_ref_count[0]-1);
			lent_fill_rectangle(&h->cache.ref[0][i4],i_width/4,i_height/4,32,i_ref[0]);
			lent_predict_mv( h, i_idx, i_pix_x0, i_pix_y0, i_width, i_height, i_depth, 0, mvp );
			lent_decode_mvd( h, mvd );
			mvp_idx=lent_decode_mvp_idx( h, b_merge );

			mv[0]=mvp[mvp_idx][0]+mvd[0];
			mv[1]=mvp[mvp_idx][1]+mvd[1];
			{//todo optimize 不能使用fill_rectangle因为它是8位的
				int x,y;
				int32_t *p=(int32_t *)h->cache.mv[0][i4];
				for(y=0;y<i_height/4;y++)
					for(x=0;x<i_width/4;x++)
						p[y*32+x]=M32(mv);
			}

			i_ref[1]=decode_ref_idx(h,h->i_ref_count[1]-1);
			lent_fill_rectangle(&h->cache.ref[1][i4],i_width/4,i_height/4,32,i_ref[1]);
			lent_predict_mv( h, i_idx, i_pix_x0, i_pix_y0, i_width, i_height, i_depth, 1, mvp );
			lent_decode_mvd( h, mvd );
			mvp_idx=lent_decode_mvp_idx( h, b_merge );

			mv[0]=mvp[mvp_idx][0]+mvd[0];
			mv[1]=mvp[mvp_idx][1]+mvd[1];
			{//todo optimize 不能使用fill_rectangle因为它是8位的
				int x,y;
				int32_t *p=(int32_t *)h->cache.mv[1][i4];
				for(y=0;y<i_height/4;y++)
					for(x=0;x<i_width/4;x++)
						p[y*32+x]=M32(mv);
			}
		}
		else
		{
			int list=0,tmp;
			if(h->sh.m_eSliceType == SLICE_B)
			{
				list=get_cabac(&h->cabac,&h->cabac.cabac_state[OFS_INTER_DIR_CTX+4]);
			}
			tmp=decode_ref_idx(h,h->i_ref_count[list]-1);

			i_ref[list]=tmp;

			lent_fill_rectangle(&h->cache.ref[list][i4],i_width/4,i_height/4,32,i_ref[list]);

			lent_predict_mv( h, i_idx, i_pix_x0, i_pix_y0, i_width, i_height, i_depth, list, mvp );

			lent_decode_mvd( h, mvd );
			mvp_idx=lent_decode_mvp_idx( h, b_merge );

			mv[0]=mvp[mvp_idx][0]+mvd[0];
			mv[1]=mvp[mvp_idx][1]+mvd[1];
			{//todo optimize 不能使用fill_rectangle因为它是8位的
				int x,y;
				int32_t *p=(int32_t *)h->cache.mv[list][i4];
				for(y=0;y<i_height/4;y++)
					for(x=0;x<i_width/4;x++)
						p[y*32+x]=M32(mv);
			}
		}
	}
	lent_fill_rectangle(&h->cache.unit_exist_map[LENT_scan4[i_idx]],i_width/4,i_height/4,32,1);
}
static lent_always_inline void decode_prediction(HEVCContext *h,int i_log2_size,int idx,int treeidx,int pix_x0,int pix_y0)
{
	int width		= 1 << i_log2_size;
	int height		= 1 << i_log2_size;
	int depth		= MAX_CU_LOG2 - i_log2_size;

	int part_mode	= h->cache.part_mode[treeidx];
	int i8			= LENT_scan8[idx >> 2];
	int idx_step	= DEPTH2IDXSTEPC(depth);//1 << ((i_log2_size - MIN_BLOCK_LOG2) << 1);
	int pred_mode	= h->cache.pred_mode[i8];

	if( pred_mode == MODE_INTRA )
	{
		if(h->sps.m_usePCM)
		{
			assert(0);
		}

		decode_prediction_intra( h, i_log2_size, idx );
		decode_prediction_chroma( h, i_log2_size, idx );

		lent_fill_rectangle(&h->cache.unit_exist_map[LENT_scan4[idx]],width/4,height/4,32,1);
	}
	else /*MODE_INTER*/
	{
		
		int i_w = width >> 1;
		int i_h = height >> 1;

		switch( part_mode )
		{
		case SIZE_2Nx2N:
			lent_decode_motion( h, idx, pix_x0,		pix_y0,		width,	height,	depth );
			break;
		case SIZE_2NxN:
			lent_decode_motion( h, idx, pix_x0,		pix_y0,		width,	i_h,	depth );
			idx += idx_step*2;
			lent_decode_motion( h, idx, pix_x0,		pix_y0+i_h,	width,	i_h,	depth );
			break;
		case SIZE_Nx2N:
			lent_decode_motion( h, idx, pix_x0,		pix_y0,		i_w,	height, depth );
			idx += idx_step;
			lent_decode_motion( h, idx, pix_x0+i_w,	pix_y0,		i_w,	height, depth );
			break;
		case SIZE_NxN:
			lent_decode_motion( h, idx, pix_x0,		pix_y0,		i_w,	i_h,	depth );
			idx += idx_step;
			lent_decode_motion( h, idx, pix_x0+i_w,	pix_y0,		i_w,	i_h,	depth );
			idx += idx_step;
			lent_decode_motion( h, idx, pix_x0,		pix_y0+i_h,	i_w,	i_h,	depth );
			idx += idx_step;
			lent_decode_motion( h, idx, pix_x0+i_w,	pix_y0+i_h,	i_w,	i_h,	depth );
			break;
		}
		
	}
}

static lent_always_inline void decode_last_coeff_xy(HEVCContext *h, int i_log2_size, int i_plane, int *px, int *py)//暂时不支持NSQT
{
	int i,i_rem;
	int i_ctx_x = OFS_CTX_LAST_FLAG_X+(i_plane?15:0);
	int i_ctx_y = OFS_CTX_LAST_FLAG_Y+(i_plane?15:0);
	int pre_x, pre_y;
	int i_log2_m2 = i_log2_size - 2;
	int i_width = 1 << i_log2_size;
	int group_idx = last_xy_group_idx[i_width-1];

	int i_blk_offset = i_plane ? 0 : (i_log2_m2 * 3 + ((i_log2_m2+1)>>2));
	int i_shift = i_plane ? i_log2_m2 : ((i_log2_m2 + 3) >> 2); 

	{
		for(pre_x=0;pre_x<group_idx;pre_x++)
			if(!get_cabac(&h->cabac,&h->cabac.cabac_state[i_ctx_x + i_blk_offset + (pre_x>>i_shift)]))
				break;
		
		for(pre_y=0;pre_y<group_idx;pre_y++)
			if(!get_cabac(&h->cabac,&h->cabac.cabac_state[i_ctx_y + i_blk_offset + (pre_y>>i_shift)]))
				break;
	}

	i_rem=0;
	if( pre_x > 3 )
	{
		int i_bits = ( pre_x - 2 ) >> 1;

		for( i = i_bits - 1; i >= 0; i -- )
			i_rem=(i_rem<<1)|get_cabac_bypass(&h->cabac);
	}
	*px= min_in_group[pre_x]+i_rem;

	i_rem=0;
	if( pre_y > 3 )
	{
		int i_bits = ( pre_y - 2 ) >> 1;

		for( i = i_bits - 1; i >= 0; i -- )
			i_rem=(i_rem<<1)|get_cabac_bypass(&h->cabac);
	}
	*py= min_in_group[pre_y]+i_rem;

	return;

}

static lent_always_inline void decode_coeff_level(HEVCContext *h, int i_rice_param, int16_t *level)
{
	int length=0;
	int i,lev=0;
	while(get_cabac_bypass(&h->cabac))length++;

	if(length<3)
	{
		for( i = i_rice_param - 1; i >= 0; i -- )
			lev+=get_cabac_bypass(&h->cabac)<<i;
		*level=lev+(length<<i_rice_param);
	}
	else
	{
		length=length-3;
		lev+=((1<<length)-1)<<i_rice_param;
		length+=i_rice_param;

		for( i = length - 1; i >= 0; i -- )
			lev+=get_cabac_bypass(&h->cabac)<<i;
		*level=lev+(3<<i_rice_param);
	}
}

const static int8_t i_idx_map[5][16] =
{
	{
		2, 1, 1, 0,
		1, 1, 0, 0,
		1, 0, 0, 0,
		0, 0, 0, 0,
	},
	{
		2, 2, 2, 2,
		1, 1, 1, 1,
		0, 0, 0, 0,
		0, 0, 0, 0,
	},
	{
		2, 1, 0, 0,
		2, 1, 0, 0,
		2, 1, 0, 0,
		2, 1, 0, 0,
	},
	{
		2, 2, 2, 2,
		2, 2, 2, 2,
		2, 2, 2, 2,
		2, 2, 2, 2,
	},
	{
		0, 1, 4, 5,
		2, 3, 4, 5,
		6, 6, 8, 8,
		7, 7, 8, 8,
	}
};

static void decode_residual(HEVCContext *h,int i_log2_size,int i_idx,int i_scan_idx,int i_plane,int qp)
{
	int x, y, n, i, i_sub, i_last_sub, nnz;
	dctcoef *dct = (i_plane ? h->cache.chroma_out[i_plane-1] : h->cache.luma_out) + (i_plane ? (i_idx<<2) : (i_idx<<4));
	int16_t *scan = h->scan_order[i_log2_size-2][i_log2_size-2][i_scan_idx];
	int16_t *scan2 = h->scan_order[0][0][i_scan_idx];

	int16_t *scan_inv = h->scan_order_inv[i_log2_size-2][i_log2_size-2][i_scan_idx];
	int16_t scan_mask = (1<<i_log2_size) - 1;

	int i_sig = 0;
	dctcoef buf[16]={0},sig[16]={0};
	int8_t order[16];

	int8_t group_nnz_table[9][9]={0};
	int i_ctx, c1 = 1, i_rice_param;
	int i_ctx_offset_global = i_log2_size == 3 ? (i_scan_idx == SCAN_DIAG ? 9 : 15) : (i_plane ? 12 : 21);

	char b_code_sig=0, b_group_nnz;


	assert( i_log2_size >= h->sps.m_uiQuadtreeTULog2MinSize );
	//h->sps.m_uiQuadtreeTULog2MinSize

	memset(dct,0,sizeof(*dct)*(1<<(i_log2_size<<1)) );


	if( i_scan_idx == SCAN_VER )
	{
		decode_last_coeff_xy( h, i_log2_size, i_plane, &y, &x );
	}
	else
	{
		decode_last_coeff_xy( h, i_log2_size, i_plane, &x, &y );
	}

	//optimize n

	n=scan_inv[(y<<i_log2_size)+x];

	i_last_sub=i_sub=n>>4;//从最后一个系数所在的组开始搜，而不是从整个矩阵的最后开始搜//pipy 2012.2.8

	buf[n&0xF]=1;sig[n&0xF] = 1;
	b_code_sig=1;

	n--;
	for(; i_sub>=0; i_sub -- )//如果i_sig==0，说明剩下已经没有系数了//pipy 2012.2.8
	{
		int i_zigzag;
		int blockx, blocky, i_pattern_sig, i_ctx_offset;
		i_zigzag = i_sub << 4;

		blockx = (scan[i_zigzag]&scan_mask)>>2;
		blocky = (scan[i_zigzag]>>i_log2_size)>>2;

		//检查本组是否有系数
		if( i_sub != i_last_sub && i_sub )
		{
/*			int i;
			b_group_nnz = 0;
			for( i = 0; i < 16; i ++ ) //TODO: 优化 不需要scan
			{
				x = scan[i_zigzag+i] & 0xFFFF;
				y = scan[i_zigzag+i] >> 16;

				b_group_nnz |= !!dct[x|(y<<i_log2_size)];
			}
*/
			i_ctx = OFS_SIG_CG_FLAG_CTX + (i_plane ? 2 : 0);
			i_ctx += group_nnz_table[blocky+1][blockx] || group_nnz_table[blocky][blockx+1];
			b_group_nnz=get_cabac( &h->cabac, &h->cabac.cabac_state[i_ctx] );
		}
		else
			b_group_nnz = 1;
		group_nnz_table[blocky][blockx] = b_group_nnz;

		if( !b_group_nnz )
		{
			n-=16;
			continue;
		}

		//解码本组

		i_rice_param = 0;
		
		i_pattern_sig = i_log2_size == 2 ? 4 :
			group_nnz_table[blocky][blockx+1] | (group_nnz_table[blocky+1][blockx] << 1);
		i_ctx_offset = i_log2_size == 2 ? 0 :
			i_ctx_offset_global + ((i_plane == 0 && blockx+blocky > 0) ? 3 : 0);
		i_ctx = (i_plane ? OFS_SIG_FLAG_CTX_CHROMA : OFS_SIG_FLAG_CTX_LUMA);
		while(n>i_zigzag){
			int idx = n & 0xF;
			int ctx_inc;
			
			ctx_inc = i_ctx_offset + i_idx_map[i_pattern_sig][scan2[idx]];

			sig[idx]=get_cabac( &h->cabac, &h->cabac.cabac_state[i_ctx + ctx_inc]);

			b_code_sig |= sig[idx];
			n--;
		}
		if( n == i_zigzag )
		{
			if( i_sub == 0 || b_code_sig )
			{
				int ctx_inc = n == 0 ? 0 : i_ctx_offset + i_idx_map[i_pattern_sig][0];
				sig[0]=get_cabac( &h->cabac, &h->cabac.cabac_state[i_ctx + ctx_inc]);
			}
			else
				sig[0] = 1;
			b_code_sig |= sig[0];
			n--;
		}
		nnz = 0;
		for( i = 15; i >= 0; i -- )
		{
			buf[nnz] = sig[i];
			order[nnz] = i;
			nnz += sig[i];
		}
		{
			int i_ctx_set = i_sub > 0 && !i_plane ? 2 : 0;
			int i_c1_total = LENTMIN( nnz, 8 ), i_c2_idx = -1;

			i_ctx_set += (c1 == 0);

			i_ctx = i_plane ? OFS_ONE_FLAG_CTX_CHROMA : OFS_ONE_FLAG_CTX_LUMA;
			i_ctx += (i_ctx_set << 2);
			c1 = 1;

			for( i = 0; i < i_c1_total; i ++ )
			{
				int gt1 =get_cabac( &h->cabac, &h->cabac.cabac_state[i_ctx + c1]);
				if( gt1 )
				{
					c1 = 0;
					i_c2_idx = i_c2_idx == -1 ? i : i_c2_idx;
					buf[i]=2;
				}
				else if( c1 < 3 && c1 > 0 )
					c1 ++;
			}

			if( c1 == 0 && i_c2_idx != -1 )
			{
				i_ctx = i_plane ? OFS_ABS_FLAG_CTX_CHROMA : OFS_ABS_FLAG_CTX_LUMA;
				buf[i_c2_idx]=get_cabac( &h->cabac, &h->cabac.cabac_state[i_ctx + i_ctx_set])?3:2;
			}

			if( 0 )//TODO: sign hidden
			{
			}
			else
			{
				for( i = 0; i < nnz; i ++ )
				{
					buf[i]=get_cabac_bypass( &h->cabac)?-buf[i]:buf[i];
				}
			}

			i_c2_idx = 1;
			if( c1 == 0 || nnz > 8 )
			{
				for( i = 0; i < nnz; i ++ )
				{
					int i_base_level = i < 8 ? 2 + i_c2_idx : 1;
					int16_t tmp;
					int abs_buf = LENTABS(buf[i]);

					if( abs_buf >= i_base_level )
					{
						decode_coeff_level( h, i_rice_param, &tmp );
						buf[i] += (buf[i] > 0 ? tmp : -tmp );
						abs_buf += tmp;
						
						i_rice_param = abs_buf <= 3*(1<<i_rice_param) ? i_rice_param :
							LENTMIN(i_rice_param+1,4);
					}

					if( abs_buf >= 2 )
						i_c2_idx = 0;
				}
			}

			for(i=0;i<nnz;i++)
			{
				dct[scan[i_zigzag+order[i]]]=buf[i];
			}
		}
		b_code_sig = 0;
	}
	h->dsp.dequant(dct,qp,i_log2_size);

 }

static inline int decode_cabac_dqp( HEVCContext *h, int i_idx )
{
	int dqp;

	if(!get_cabac(&h->cabac,&h->cabac.cabac_state[OFS_DELTA_QP_CTX]))
		dqp=0;
	else
	{
		int neg=get_cabac_bypass(&h->cabac);

		if(get_cabac(&h->cabac,&h->cabac.cabac_state[OFS_DELTA_QP_CTX+1]))
		{
			for(dqp=1;;dqp++)
				if(!get_cabac(&h->cabac,&h->cabac.cabac_state[OFS_DELTA_QP_CTX+2]))
					break;
		}
		else
			dqp=0;
		dqp++;

		if(neg)
			dqp=-dqp;
	}
	return dqp;
}

static int decode_trans_tree(HEVCContext *h,int i_log2_size,int i_log2_size_block, int i_idx,int i_pix_x0,int i_pix_y0)
{
	int i_width			=1<<i_log2_size;
	int i_height		=1<<i_log2_size;
	int i_depth			=LOG22DEPTH(i_log2_size);
	int i8				=LENT_scan8[i_idx>>2];
	int i8_idx			= i_idx >> 2;
	int i_flag_idx		=TREEIDX(i_idx,i_log2_size);
	int i_trans_depth	=i_depth-h->cache.cu_depth[i8];
	int i_pred_mode		=h->cache.pred_mode[i8];
	int i_part_mode		=h->cache.part_mode[i_flag_idx];
	int b_intra_split	=i_pred_mode == MODE_INTRA && h->cache.part_mode[i_flag_idx] == SIZE_NxN;

	int i_trans_max = h->sps.m_uiQuadtreeTULog2MaxSize;
	int i_trans_min = h->sps.m_uiQuadtreeTULog2MinSize;

	int b_residual;
	int i_cbp=0;
	int b_split_trans=0;
	
	if( i_trans_depth == 0 && !b_intra_split )
	{
		if(i_pred_mode!=MODE_INTRA && h->cache.b_merge == 0 )
		{
			int ctx = OFS_QT_ROOT_CBF_CTX;

			b_residual = get_cabac_noinline(&h->cabac,&h->cabac.cabac_state[ctx]);
		}
		else
		{
			b_residual=1;
		}
	}
	else
		b_residual=1;
	
	if( b_residual )
	{
		int b_split = (b_intra_split && i_trans_depth == 0) ? 1 : 0;
		int i_max_depth = (i_pred_mode == MODE_INTRA) ? 
			h->sps.m_uiQuadtreeTUMaxDepthIntra + b_intra_split :
			h->sps.m_uiQuadtreeTUMaxDepthInter;
		int i_idx_step = DEPTH2IDXSTEPC(i_depth);//1 << (i_log2_size - MIN_BLOCK_LOG2);
		int i_top_flag = TREEIDX(i_idx,i_log2_size+1);


		if( i_pred_mode == MODE_INTRA && i_part_mode == SIZE_NxN )
		{
			assert( b_intra_split );
			b_intra_split=1;
			b_split_trans=1;
		}
		else if( i_log2_size <= i_trans_max && i_log2_size > i_trans_min && i_trans_depth < i_max_depth && !b_split )
			b_split_trans=get_cabac_noinline(&h->cabac,&h->cabac.cabac_state[OFS_TRANS_SUBDIV_FLAG_CTX+i_depth-1]);
		else if( i_log2_size > i_trans_max)
			b_split_trans=1;
		else if(i_log2_size <= i_trans_min)
			b_split_trans=0;
		
		//if( i_log2_size <= i_trans_max )
		{
			int b_first_cbf = i_trans_depth == 0;
			int i_up_cbp = h->cache.i_cbp[i_top_flag];

			if( b_first_cbf || i_log2_size > i_trans_min )
			{
				int i_idx_cb = OFS_QT_CBF_CTX+5 + i_trans_depth;
				int i_idx_cr = OFS_QT_CBF_CTX+5 + i_trans_depth;
				if( b_first_cbf || CBF_CB(i_up_cbp) )
				{
					if(get_cabac_noinline(&h->cabac,&h->cabac.cabac_state[i_idx_cb]))
						i_cbp|=CBF_CB_FLAG;
				}
				if( b_first_cbf || CBF_CR(i_up_cbp) )
				{
					if(get_cabac_noinline(&h->cabac,&h->cabac.cabac_state[i_idx_cr]))
						i_cbp|=CBF_CR_FLAG;
				}
			}
			else if(i_log2_size == CU_LOG2_4x4 && ((i_idx&3)==0))//fix 4x4->2x2 chroma
				i_cbp=i_up_cbp&(CBF_CB_FLAG|CBF_CR_FLAG);
		}
		if(i_log2_size == CU_LOG2_4x4 && ((i_idx&3)==3))//fix 4x4->2x2 chroma
				i_cbp=h->cache.i_cbp[i_top_flag]&(CBF_CB_FLAG|CBF_CR_FLAG);
 		h->cache.i_cbp[i_flag_idx]=i_cbp;//提前将CbCr的cbf存入表中，以进行子块解码
		if( b_split_trans )
		{
			int i_pix_x1 = i_pix_x0 + (i_width>>1);
			int i_pix_y1 = i_pix_y0 + (i_width>>1);

			//h->cache.i_split_trans[i_flag_dx]=1;//记录本块trans划分

			i_cbp|=decode_trans_tree( h, i_log2_size-1, i_log2_size_block, i_idx, i_pix_x0, i_pix_y0  );
			i_idx += i_idx_step;
			i_cbp|=decode_trans_tree( h, i_log2_size-1, i_log2_size_block, i_idx, i_pix_x1, i_pix_y0  );
			i_idx += i_idx_step;
			i_cbp|=decode_trans_tree( h, i_log2_size-1, i_log2_size_block, i_idx, i_pix_x0, i_pix_y1  );
			i_idx += i_idx_step;
			i_cbp|=decode_trans_tree( h, i_log2_size-1, i_log2_size_block, i_idx, i_pix_x1, i_pix_y1  );
		}
		else
		{
			int i_idx_luma = OFS_QT_CBF_CTX + (i_trans_depth==0 ? 1 : 0);
			int b_dqp_cbp = 0;

				{
#if 0
				#undef fprintf
						FILE *fp=fopen("logdec.txt","a");
						fprintf(fp,"\t\ttrans %d\n",i_idx);
						fclose(fp);
#endif
				}

			//h->cache.i_split_trans[i_flag_dx]=0;//记录本块trans划分

			if( i_pred_mode != MODE_INTRA && i_trans_depth == 0 &&
				!CBF_CHROMA(i_cbp) )
			{
				i_cbp|=CBF_LUMA_FLAG;
				//assert(CBF_LUMA(i_cbp));
			}
			else
			{
				if(get_cabac_noinline(&h->cabac,&h->cabac.cabac_state[i_idx_luma]))
					i_cbp|=CBF_LUMA_FLAG;
			}

			//h->cache.i_cbp[i_flag_idx]|=i_cbp;//将cbf存入表中
			b_dqp_cbp = i_log2_size == 2 && (i_idx&3) == 0 ? 
							(i_cbp | CBF_CHROMA(h->cache.i_cbp[i_flag_idx+3])) : i_cbp;

			//解码本块
			if( h->pps.m_useDQP && !h->cache.i_dqp_decoded && b_dqp_cbp)
			{
				int i_qp, i_ref_qp;
				int dqp=decode_cabac_dqp( h, i_idx );
				int cover=(1<<((4-h->pps.m_uiMaxCuDQPDepth)<<1))-1;
				int start,i8_start;
				h->cache.i_dqp_decoded =1;
				start=i_idx&~cover;
				i8_start=LENT_scan8[start>>2];

				i_ref_qp = ((h->cache.qp[i8_start-1]>0?h->cache.qp[i8_start-1]:h->cache.i_last_qp)+(h->cache.qp[i8_start-16]>0?h->cache.qp[i8_start-16]:h->cache.i_last_qp)+1)>>1;

				i_qp=dqp+i_ref_qp;

				h->cache.i_last_qp=i_qp;

				lent_fill_rectangle(&h->cache.qp[i8_start], 1 << (3 - h->pps.m_uiMaxCuDQPDepth), 1 << (3 - h->pps.m_uiMaxCuDQPDepth), 16, i_qp );
				if(i_log2_size_block>=3)
				{
					cover=(1<<((i_log2_size_block-2)<<1))-1;
					start=i_idx&~cover;
					i8_start=LENT_scan8[start>>2];
					lent_fill_rectangle(&h->cache.qp[i8_start], 1<<(i_log2_size_block-3), 1<<(i_log2_size_block-3), 16, i_qp );
				}
			}

			if(CBF_LUMA(i_cbp)){
				int i_scan_idx;
				int luma_qp;

				luma_qp=h->cache.qp[i8];

				assert(luma_qp>0);

				if( h->cache.pred_mode[i8] == MODE_INTRA )
				{
					int i_luma_mode = h->cache.intra_pred_mode[LENT_scan4[i_idx]];
					i_scan_idx = scan_type[i_log2_size-2][i_luma_mode];
				}
				else
					i_scan_idx = SCAN_ZIGZAG;

				i_scan_idx = i_scan_idx == SCAN_ZIGZAG ? SCAN_DIAG : i_scan_idx;

	
				decode_residual( h, i_log2_size, i_idx,
						 i_scan_idx, 0, luma_qp );
			}

			if(CBF_CHROMA(i_cbp)&&(i_log2_size>2 || (i_idx&3)==3)){
				int i_scan_idx;
					int luma_qp,chroma_qp;

					luma_qp=h->cache.qp[i8];
					assert(luma_qp>0);
					chroma_qp=i_chroma_qp_table[luma_qp];


					if( h->cache.pred_mode[i8] == MODE_INTRA )
					{
						int i_luma_mode = h->cache.intra_pred_mode[LENT_scan4[i_idx&~3]];
						int i_chroma_mode = h->cache.intra_pred_mode[LENT_scan4[256+i8_idx]];

						i_chroma_mode = lent_get_chroma_mode( i_luma_mode, i_chroma_mode );
						i_scan_idx = scan_type[i_log2_size-2][i_chroma_mode];
					}
					else
						i_scan_idx = SCAN_ZIGZAG;

					i_scan_idx = i_scan_idx == SCAN_ZIGZAG ? SCAN_DIAG : i_scan_idx;

					if(CBF_CB(i_cbp))
						decode_residual( h, i_log2_size-(i_log2_size>2), i_idx&~3,
							 i_scan_idx, 1, chroma_qp );
					if(CBF_CR(i_cbp))
						decode_residual( h, i_log2_size-(i_log2_size>2), i_idx&~3,
							 i_scan_idx, 2, chroma_qp );
			}
		}
	}
	else
	{
	}
	h->cache.b_split_trans[i_flag_idx]=b_split_trans;//记录本块trans划分
	h->cache.i_cbp[i_flag_idx]=i_cbp;//将cbf存入表中

	if( i_trans_depth == 0 )//当前块大小和cu一致
	{
		{
			int tmp = h->cache.i_cbp[i_flag_idx] ? h->cache.cu_flag[i8] |  RESIDUAL_FLAG :
				h->cache.cu_flag[i8] & ~RESIDUAL_FLAG;
				lent_fill_rectangle( &h->cache.cu_flag[i8], i_width/8, i_width/8, 16,  tmp);
		}
		if(!h->pps.m_useDQP)
		{
			lent_fill_rectangle(&h->cache.qp[i8], i_width/8, i_height/8, 16, h->cache.i_last_qp );
		}
		if(h->pps.m_useDQP&&(i_cbp==0&&!h->cache.i_dqp_decoded))//cbp为0的话
		{
			int i_ref_qp = ((h->cache.qp[i8-1]>0?h->cache.qp[i8-1]:h->cache.i_last_qp)+(h->cache.qp[i8-16]>0?h->cache.qp[i8-16]:h->cache.i_last_qp)+1)>>1;

			lent_fill_rectangle(&h->cache.qp[i8], i_width/8, i_height/8, 16, i_ref_qp );
			h->cache.i_last_qp = i_ref_qp;
		}
	}



	return i_cbp;

}
static void decode_block(HEVCContext *h,int i_log2_size,int i_idx,int i_pix_x0,int i_pix_y0)
{
	int i_width	=1<<i_log2_size;
	int i_height=1<<i_log2_size;
	int i_depth	=LOG22DEPTH(i_log2_size);
	int i8		=LENT_scan8[i_idx>>2];
	int i4		=LENT_scan4[i_idx];
	int treeidx =TREEIDX(i_idx,i_log2_size);

	int skip=0;
	int more_data=0;

	if(i_idx%(1<<((4-h->pps.m_uiMaxCuDQPDepth)<<1))==0)
		h->cache.i_dqp_decoded=0;

	if(!h->pps.m_useDQP)
		lent_fill_rectangle(&h->cache.qp[i8], i_width>>3, i_height>>3, 16, h->cache.i_last_qp);

		{
#if 0
#undef fprintf
		FILE *fp=fopen("logdec.txt","a");
		fprintf(fp,"\tblock %d\n",i_idx);
		fclose(fp);
#endif
	}

	
	//read skip flag for B/P Slice
	if(h->sh.m_eSliceType!=SLICE_I)
	{
		int ctx = OFS_SKIP_FLAG_CTX + !!(h->cache.cu_flag[i8 - 16] & SKIP_FLAG)
			+ !!(h->cache.cu_flag[i8 - 1] & SKIP_FLAG);
		skip=get_cabac_noinline(&h->cabac,&h->cabac.cabac_state[ctx]);
	}
	if(skip)
	{
		int16_t mvp[2][5][2], mv[2];
		int8_t ref[2][5];
		int i_mvp_idx, i;
		int list_count = h->sh.m_eSliceType == SLICE_B ? 2 : 1;
		int tmp = (h->cache.cu_flag[i8] & ~RESIDUAL_FLAG)|SKIP_FLAG;
		
		i_mvp_idx=lent_decode_mvp_idx( h, 1 ); //add by ckj 
		lent_fill_rectangle( &h->cache.cu_flag[i8], i_width>>3, i_height>>3, 16, tmp );
		LENT_predict_mv_merge( h, i_idx, i_pix_x0, i_pix_y0, i_width, i_height, i_depth, mvp[0], mvp[1], ref[0], ref[1], i_mvp_idx+1 );
		//i_mvp_idx=lent_decode_mvp_idx( h, 1 );



		for( i = 0; i < list_count; i ++ )
		{

			if(ref[i][i_mvp_idx]>=0)
			{
				mv[0]=mvp[i][i_mvp_idx][0];
				mv[1]=mvp[i][i_mvp_idx][1];

				lent_fill_rectangle(&h->cache.ref[i][i4],i_width/4,i_height/4,32,ref[i][i_mvp_idx]);
				{//todo optimize 不能使用fill_rectangle因为它是8位的
					int x,y;
					int32_t *p=(int32_t *)h->cache.mv[i][i4];
					int w4 = i_width/4, h4 = i_height/4;
					for(y=0;y<h4;y++)
						for(x=0;x<w4;x++)
							p[y*32+x]=M32(mv);
				}
			}

		}
		lent_fill_rectangle(&h->cache.unit_exist_map[LENT_scan4[i_idx]],i_width/4,i_height/4,32,1);
		lent_fill_rectangle(&h->cache.pred_mode[i8],i_width/8,i_height/8,16,MODE_INTER);
		h->cache.part_mode[treeidx]	=SIZE_2Nx2N;
		if(h->pps.m_useDQP)
		if(h->cache.i_dqp_decoded==0)
		{
			int i_ref_qp = ((h->cache.qp[i8-1]>0?h->cache.qp[i8-1]:h->cache.i_last_qp)+(h->cache.qp[i8-16]>0?h->cache.qp[i8-16]:h->cache.i_last_qp)+1)>>1;
			lent_fill_rectangle(&h->cache.qp[i8], i_width>>3, i_height>>3, 16, i_ref_qp );
			h->cache.i_last_qp=h->cache.qp[i8];
		}
		h->cache.b_merge=1;
		return;
	}
	else
	{
		if( h->sh.m_eSliceType != SLICE_I || i_log2_size == h->sps.m_uiCULog2MinSize )
			decode_predtype( h, i_log2_size, treeidx, i8 );
		else if(h->sh.m_eSliceType == SLICE_I)
		{
			h->cache.part_mode[treeidx]	=SIZE_2Nx2N;
			lent_fill_rectangle(&h->cache.pred_mode[i8],LOG22I8WIDTH(i_log2_size),LOG22I8WIDTH(i_log2_size),16,MODE_INTRA);
		}


		decode_prediction( h, i_log2_size , i_idx, treeidx, i_pix_x0, i_pix_y0);
	}

	decode_trans_tree(h,i_log2_size, i_log2_size,i_idx,i_pix_x0,i_pix_y0);
	return;
}

#define IN_PIC(h,x,y) (x+(h->cu_x<<6)<h->width&&y+(h->cu_y<<6)<h->height)
#define DECODE_TREE decode_tree_edge
#include"misc/decode_tree.c"
#undef	DECODE_TREE
#undef	IN_PIC


#define IN_PIC(h,x,y)	1//hack to get higher speed
#define DECODE_TREE decode_tree_fast
#include"misc/decode_tree.c"
#undef	DECODE_TREE
#undef	IN_PIC


/////////////////////////////////////////////////////////////////////////////////////////////////
//语法读取api
/////////////////////////////////////////////////////////////////////////////////////////////////
void lent_hevc_init_cabac_states(HEVCContext *h)
{
    int i;
    const uint8_t (*tab);
	if(h->ctx->compatibility>=92)
		switch(h->sh.m_eSliceType)
		{
		case SLICE_I:
			tab=LENT_cabac_context_init_I;
			break;
		case SLICE_P:
			tab=LENT_cabac_context_init_P;
			break;
		case SLICE_B:
			tab=LENT_cabac_context_init_B;
			break;
		default:
			errorinfo(h,"Unknown slice type in cabac init");
		}
	else
		switch(h->sh.m_eSliceType)
		{
		case SLICE_I:
			tab=LENT_cabac_context_init_I_091;
			break;
		case SLICE_P:
			tab=LENT_cabac_context_init_P_091;
			break;
		case SLICE_B:
			tab=LENT_cabac_context_init_B_091;
			break;
		default:
			errorinfo(h,"Unknown slice type in cabac init");
		}
	

	//for( i = 0; i < MAX_CABAC_STATE; i++ ) {
 //       int pre = 2*(((tab[i][0] * h->sh.i_qp) >>4 ) + tab[i][1]) - 127;

 //       pre^= pre>>31;
 //       if(pre > 124)
 //           pre= 124 + (pre&1);

 //       h->cabac.cabac_state[i] =  pre;
 //   }


	for( i = 0; i < MAX_CABAC_STATE; i++ ) {
		int initValue	= tab[i];
		int slope		= (initValue>>4)*5 - 45;
		int offset		= ((initValue&15)<<3)-16;
		int initState	=  LENTMIN( LENTMAX( 1, ( ( ( slope * h->sh.m_iSliceQp ) >> 4 ) + offset ) ), 126 );

		h->cabac.cabac_state[i] = ( (initState >= 64? (initState - 64):(63 - initState)) <<1) + (initState >= 64 );
	}//HM6 new cabac initialization

	return;
}
#define MEMSET(address,value,length) memset(address,value,length)
static lent_always_inline void decode_cache_load(HEVCContext *h)//todo optimize
{
	int i;
	int i_neighbor=h->i_neighbor;
	int top_xy,left_xy;

	do{
// #if ARCH_X86_32
// 		if(h->machine>=20){
// 			__m128i t, *start1_ptr, *start0_ptr, *end_ptr, *p, t1, t0;
// 			start1_ptr = (__m128i*)h->cache.cu_depth;
// 			start0_ptr = (__m128i*)h->cache.cu_flag;
// 			end_ptr = (__m128i*)h->cache.fdec_buf;
// 			t1 = _mm_set1_epi32(-1);
// 			t0 = _mm_setzero_si128();
// 			for ( p = start1_ptr; p < start0_ptr; p++ )
// 				_mm_store_si128(p, t1);
// 			for ( ; p < end_ptr; p++ )
// 				_mm_store_si128(p, t0);
// 			break;
// 		}
// #endif
	MEMSET(h->cache.cu_depth,-1,(char*)h->cache.cu_flag-(char*)h->cache.cu_depth);
	MEMSET(h->cache.cu_flag,0,(char*)h->cache.qp-(char*)h->cache.cu_flag);
	h->cache.i_dqp_decoded = 0;
	}while(0);
	//h->cache.luma_out=h->current.luma[h->cu_index];
	//h->cache.chroma_out[0]=h->current.chroma[0][h->cu_index];
	//h->cache.chroma_out[1]=h->current.chroma[1][h->cu_index];

	top_xy=h->cu_xy-h->cu_stride;
	left_xy=h->cu_xy-1;

/*
	//cu_depth
	memcpy(&h->cache.cu_depth[LENT_scan8[0]-16],&h->current.cu_depth_edge[top_xy][8],8*sizeof(int8_t));
	for(i=0;i<8;i++)
	{
		h->cache.cu_depth[LENT_scan8[0]-1+i*16]	=h->current.cu_depth_edge[left_xy][i];
	}
	//cu flag
	memcpy(&h->cache.cu_flag[LENT_scan8[0]-16],&h->current.cu_flag_edge[top_xy][8],8*sizeof(int8_t));
	for(i=0;i<8;i++)
	{
		h->cache.cu_flag[LENT_scan8[0]-1+i*16]	=h->current.cu_flag_edge[left_xy][i];
	}
	//predmode
	memcpy(&h->cache.pred_mode[LENT_scan8[0]-16],&h->current.predmode_edge[top_xy][8],8*sizeof(int8_t));
	for(i=0;i<8;i++)
	{
		h->cache.pred_mode[LENT_scan8[0]-1+i*16]	=h->current.predmode_edge[left_xy][i];
	}
*/
	memcpy(&h->cache.cu_depth[LENT_scan8[0]-16],&h->current.cu_depth_edge[top_xy][8],8*sizeof(int8_t));	//cu_depth
	memcpy(&h->cache.cu_flag[LENT_scan8[0]-16],&h->current.cu_flag_edge[top_xy][8],8*sizeof(int8_t));	//cu flag
	memcpy(&h->cache.pred_mode[LENT_scan8[0]-16],&h->current.predmode_edge[top_xy][8],8*sizeof(int8_t));//predmode
	for(i=0;i<8;i++)
	{
		h->cache.cu_depth[LENT_scan8[0]-1+i*16]	=h->current.cu_depth_edge[left_xy][i];	//cu_depth
		h->cache.cu_flag[LENT_scan8[0]-1+i*16]	=h->current.cu_flag_edge[left_xy][i];	//cu flag
		h->cache.pred_mode[LENT_scan8[0]-1+i*16]	=h->current.predmode_edge[left_xy][i];	//predmode
	}


	//luma part of intra_pred_type
	{
		int8_t *dst = &h->cache.intra_pred_mode[LENT_scan4[0]-32];
		int8_t *src;

#define COPY( idx ) \
	dst[idx] = -1
#define COPY4( idx ) \
	COPY( idx + 0 ); \
	COPY( idx + 1 ); \
	COPY( idx + 2 ); \
	COPY( idx + 3 )

		COPY4(  0 );
		COPY4(  4 );
		COPY4(  8 );
		COPY4( 12 );

#undef  COPY4
#undef  COPY

		dst = &h->cache.intra_pred_mode[LENT_scan4[0]-1];
		src = &h->current.intra_pred_mode[left_xy*16];

#define COPY( idx ) \
	dst[(idx)*32] = src[idx]
#define COPY4( idx ) \
	COPY( idx + 0 ); \
	COPY( idx + 1 ); \
	COPY( idx + 2 ); \
	COPY( idx + 3 )

		COPY4(  0 );
		COPY4(  4 );
		COPY4(  8 );
		COPY4( 12 );

#undef  COPY4
#undef  COPY
	}
	//left qp
	if(i_neighbor&CU_LEFT)
	{
		int8_t *src = h->current.qp[h->cu_index-1];

		h->cache.qp[LENT_scan8[ 0]-1] = src[0];
		h->cache.qp[LENT_scan8[ 2]-1] = src[1];
		h->cache.qp[LENT_scan8[ 8]-1] = src[2];
		h->cache.qp[LENT_scan8[10]-1] = src[3];
		h->cache.qp[LENT_scan8[32]-1] = src[4];
		h->cache.qp[LENT_scan8[34]-1] = src[5];
		h->cache.qp[LENT_scan8[40]-1] = src[6];
		h->cache.qp[LENT_scan8[42]-1] = src[7];
	}
	
	if(h->sh.m_eSliceType!=SLICE_I)
	{
		//ref & mv
		int list,b4xy=(h->cu_x+h->cu_y*h->b4_stride)*16,b4_stride=h->b4_stride;
		//save mv & refidx todo optimize
		for(list=0;list<2&&h->i_ref_count[list];list++)
		{
			if((i_neighbor&CU_TOP)&&(i_neighbor&CU_LEFT))
			{
				h->cache.ref[list][LENT_scan4[0]-1- 1*32]=h->current_picture.ref_index[list][b4xy-1- 1*b4_stride];
				CP32(h->cache.mv[list][LENT_scan4[0]-1- 1*32],h->current_picture.motion_val[list][b4xy-1- 1*b4_stride]);
			}
			if(i_neighbor&CU_TOPRIGHT)
			{
				h->cache.ref[list][LENT_scan4[0]+16- 1*32]=h->current_picture.ref_index[list][b4xy+16- 1*b4_stride];
				CP32(h->cache.mv[list][LENT_scan4[0]+16- 1*32],h->current_picture.motion_val[list][b4xy+16- 1*b4_stride]);
			}
			if(i_neighbor&CU_TOP)
			{
				memcpy(&h->cache.ref[list][LENT_scan4[0]-32],&h->current_picture.ref_index[list][b4xy-b4_stride],16*sizeof(h->cache.ref[0][0]));
				memcpy(h->cache.mv[list][LENT_scan4[0]-32],&h->current_picture.motion_val[list][b4xy-b4_stride],16*sizeof(h->cache.mv[0][0]));
			}
			if(i_neighbor&CU_LEFT)
			{
				{
					int8_t *dst = &h->cache.ref[list][LENT_scan4[0]-1];
					int8_t *src = &h->current_picture.ref_index[list][b4xy-1];

#define COPY( idx ) \
	dst[(idx)*32] = src[(idx)*b4_stride]
#define COPY4( idx ) \
	COPY( idx + 0 ); \
	COPY( idx + 1 ); \
	COPY( idx + 2 ); \
	COPY( idx + 3 )

					COPY4(  0 );
					COPY4(  4 );
					COPY4(  8 );
					COPY4( 12 );

#undef  COPY4
#undef  COPY
				}
				
				{
					int16_t *dst = h->cache.mv[list][LENT_scan4[0]-1];
					int16_t *src = h->current_picture.motion_val[list][b4xy-1];

#define COPY( idx ) \
	CP32( &dst[((idx)<<1)*32], &src[((idx)<<1)*b4_stride] )
#define COPY4( idx ) \
	COPY( idx + 0 ); \
	COPY( idx + 1 ); \
	COPY( idx + 2 ); \
	COPY( idx + 3 )

					COPY4(  0 );
					COPY4(  4 );
					COPY4(  8 );
					COPY4( 12 );

#undef  COPY4
#undef  COPY
				}
			}
		}



		//generate exist_map for mv pred.
		if((i_neighbor&CU_TOP)&&(i_neighbor&CU_LEFT))
			h->cache.unit_exist_map[LENT_scan4[0]-32-1]=1;

		if(i_neighbor&CU_TOPRIGHT)//todo:optimize
		{
			memset(&h->cache.unit_exist_map[LENT_scan4[0]-32],1,16*sizeof(h->cache.unit_exist_map[0]));//TOP is complete
			h->cache.unit_exist_map[LENT_scan4[0]-32+16+0]=
			h->cache.unit_exist_map[LENT_scan4[0]-32+16+1]=h->current.cu_depth_edge[h->cu_xy-h->cu_stride+1][8+0]!=-1;
			h->cache.unit_exist_map[LENT_scan4[0]-32+16+2]=
			h->cache.unit_exist_map[LENT_scan4[0]-32+16+3]=h->current.cu_depth_edge[h->cu_xy-h->cu_stride+1][8+1]!=-1;
			h->cache.unit_exist_map[LENT_scan4[0]-32+16+4]=
			h->cache.unit_exist_map[LENT_scan4[0]-32+16+5]=h->current.cu_depth_edge[h->cu_xy-h->cu_stride+1][8+2]!=-1;
			h->cache.unit_exist_map[LENT_scan4[0]-32+16+6]=
			h->cache.unit_exist_map[LENT_scan4[0]-32+16+7]=h->current.cu_depth_edge[h->cu_xy-h->cu_stride+1][8+3]!=-1;
		}
		else if(i_neighbor&CU_TOP)//not necessary but may be faster
		{
			h->cache.unit_exist_map[LENT_scan4[0]-32+ 0]=
			h->cache.unit_exist_map[LENT_scan4[0]-32+ 1]=h->current.cu_depth_edge[h->cu_xy-h->cu_stride][8+0]!=-1;
			h->cache.unit_exist_map[LENT_scan4[0]-32+ 2]=
			h->cache.unit_exist_map[LENT_scan4[0]-32+ 3]=h->current.cu_depth_edge[h->cu_xy-h->cu_stride][8+1]!=-1;
			h->cache.unit_exist_map[LENT_scan4[0]-32+ 4]=
			h->cache.unit_exist_map[LENT_scan4[0]-32+ 5]=h->current.cu_depth_edge[h->cu_xy-h->cu_stride][8+2]!=-1;
			h->cache.unit_exist_map[LENT_scan4[0]-32+ 6]=
			h->cache.unit_exist_map[LENT_scan4[0]-32+ 7]=h->current.cu_depth_edge[h->cu_xy-h->cu_stride][8+3]!=-1;

			h->cache.unit_exist_map[LENT_scan4[0]-32+ 8]=
			h->cache.unit_exist_map[LENT_scan4[0]-32+ 9]=h->current.cu_depth_edge[h->cu_xy-h->cu_stride][8+4]!=-1;
			h->cache.unit_exist_map[LENT_scan4[0]-32+10]=
			h->cache.unit_exist_map[LENT_scan4[0]-32+11]=h->current.cu_depth_edge[h->cu_xy-h->cu_stride][8+5]!=-1;
			h->cache.unit_exist_map[LENT_scan4[0]-32+12]=
			h->cache.unit_exist_map[LENT_scan4[0]-32+13]=h->current.cu_depth_edge[h->cu_xy-h->cu_stride][8+6]!=-1;
			h->cache.unit_exist_map[LENT_scan4[0]-32+14]=
			h->cache.unit_exist_map[LENT_scan4[0]-32+15]=h->current.cu_depth_edge[h->cu_xy-h->cu_stride][8+7]!=-1;
		}
		if(i_neighbor&CU_LEFT)//not necessary but may be faster
		{
			h->cache.unit_exist_map[LENT_scan4[0]-1+ 0*32]=
			h->cache.unit_exist_map[LENT_scan4[0]-1+ 1*32]=h->current.cu_depth_edge[h->cu_xy-1][0]!=-1;
			h->cache.unit_exist_map[LENT_scan4[0]-1+ 2*32]=
			h->cache.unit_exist_map[LENT_scan4[0]-1+ 3*32]=h->current.cu_depth_edge[h->cu_xy-1][1]!=-1;
			h->cache.unit_exist_map[LENT_scan4[0]-1+ 4*32]=
			h->cache.unit_exist_map[LENT_scan4[0]-1+ 5*32]=h->current.cu_depth_edge[h->cu_xy-1][2]!=-1;
			h->cache.unit_exist_map[LENT_scan4[0]-1+ 6*32]=
			h->cache.unit_exist_map[LENT_scan4[0]-1+ 7*32]=h->current.cu_depth_edge[h->cu_xy-1][3]!=-1;

			h->cache.unit_exist_map[LENT_scan4[0]-1+ 8*32]=
			h->cache.unit_exist_map[LENT_scan4[0]-1+ 9*32]=h->current.cu_depth_edge[h->cu_xy-1][4]!=-1;
			h->cache.unit_exist_map[LENT_scan4[0]-1+10*32]=
			h->cache.unit_exist_map[LENT_scan4[0]-1+11*32]=h->current.cu_depth_edge[h->cu_xy-1][5]!=-1;
			h->cache.unit_exist_map[LENT_scan4[0]-1+12*32]=
			h->cache.unit_exist_map[LENT_scan4[0]-1+13*32]=h->current.cu_depth_edge[h->cu_xy-1][6]!=-1;
			h->cache.unit_exist_map[LENT_scan4[0]-1+14*32]=
			h->cache.unit_exist_map[LENT_scan4[0]-1+15*32]=h->current.cu_depth_edge[h->cu_xy-1][7]!=-1;
		}
	}

}
static lent_always_inline void decode_cache_save(HEVCContext *h)//todo optimize//todo 由于存在motion compress（见mvpred系列函数），在一帧解码结束后，可以不必在图像缓存中保存所有的模式和mv
{
	int i;

	//save coeffs
	//memcpy(h->current.luma[h->cu_index],		h->cache.luma_out,		sizeof(h->cache.luma_out));
	//memcpy(h->current.chroma[0][h->cu_index],	h->cache.chroma_out[0],	sizeof(h->cache.chroma_out[0]));
	//memcpy(h->current.chroma[1][h->cu_index],	h->cache.chroma_out[1],	sizeof(h->cache.chroma_out[1]));

	//save edge flag
	for(i=0;i<8;i++)
	{
		h->current.cu_flag_edge[h->cu_xy][i]		=h->cache.cu_flag[LENT_scan8[0]+7+i*16];
	}
	memcpy(&h->current.cu_flag_edge[h->cu_xy][8],	&h->cache.cu_flag[LENT_scan8[0]+7*16],8*sizeof(int8_t));

	//save edge depth for cabac & unit_map generation
	for(i=0;i<8;i++)
	{
		h->current.cu_depth_edge[h->cu_xy][i]		=h->cache.cu_depth[LENT_scan8[0]+7+i*16];
	}
	memcpy(&h->current.cu_depth_edge[h->cu_xy][8],	&h->cache.cu_depth[LENT_scan8[0]+7*16],8*sizeof(int8_t));

	//save predmode for deblocking
	for(i=0;i<8;i++)
	{
		h->current.predmode_edge[h->cu_xy][i]		=h->cache.pred_mode[LENT_scan8[0]+7+i*16];
	}
	memcpy(&h->current.predmode_edge[h->cu_xy][8],	&h->cache.pred_mode[LENT_scan8[0]+7*16],8*sizeof(int8_t));

	////save all depth for recon
//	for(i=0;i<256;i++)//是否应该基于8x8
//	{
//		h->current.cu_depth[h->cu_index][i]			=h->cache.cu_depth[LENT_scan8[i>>2]];
//	}

// 	//save qp for 8x8 blocks
// 	for(i=0;i<64;i++)
// 	{
// 		h->current.qp[h->cu_index][i]				=h->cache.qp[LENT_scan8[i]];
// 	}
	{
		int8_t *dst = h->current.qp[h->cu_index];

		dst[0] = h->cache.qp[LENT_scan8[ 0]];
		dst[1] = h->cache.qp[LENT_scan8[ 2]];
		dst[2] = h->cache.qp[LENT_scan8[ 8]];
		dst[3] = h->cache.qp[LENT_scan8[10]];
		dst[4] = h->cache.qp[LENT_scan8[32]];
		dst[5] = h->cache.qp[LENT_scan8[34]];
		dst[6] = h->cache.qp[LENT_scan8[40]];
		dst[7] = h->cache.qp[LENT_scan8[42]];
	}

//	{
//#undef fprintf
//		FILE *fp=fopen("qp.txt","a");
//		fprintf(fp,"POC %d CU %d last %d\n",h->sh.i_poc,h->cu_index,h->cache.i_last_qp);
//		for(i=0;i<LENTMIN(h->height/8-h->cu_y*8,8);i++)
//		{
//			int j;
//			for(j=0;j<LENTMIN(h->width/8-h->cu_x*8,8);j++)
//			{
//				fprintf(fp,"%d ",h->cache.qp[LENT_scan8[0]+j+i*16]);
//			}
//			fprintf(fp,"\n");
//		}
//		fclose(fp);
//	}
	//save split_trans flag
	//memcpy(h->current.split_trans[h->cu_index],		 h->cache.b_split_trans,sizeof(h->cache.b_split_trans));


	{
		//save intra_pred_type
		int8_t *dst = &h->current.intra_pred_mode[h->cu_xy*16];

#define COPY( idx ) \
	dst[idx] = h->cache.intra_pred_mode[LENT_scan4[0] + 15 + (idx)*32]
#define COPY4( idx ) \
	COPY( idx + 0 ); \
	COPY( idx + 1 ); \
	COPY( idx + 2 ); \
	COPY( idx + 3 )

		COPY4(  0 );
		COPY4(  4 );
		COPY4(  8 );
		COPY4( 12 );

#undef  COPY4
#undef  COPY
	}

	if(h->sh.m_eSliceType!=SLICE_I)
	{//todo 为了加快效率，是否应该在current中放一份mv和ref_index?
		{
			int b4xy=(h->cu_x+h->cu_y*h->b4_stride)*16, b4_stride = h->b4_stride;
			int8_t *rd = &h->current_picture.ref_index[0][b4xy];
			int16_t *md = h->current_picture.motion_val[0][b4xy];

#define COPY( idx ) \
	memcpy( rd + (idx)*b4_stride, &h->cache.ref[0][48/*LENT_scan4[0]*/+(idx)*32], 16*sizeof(h->cache.ref[0][0]) ); \
	memcpy( md + ((idx)<<1)*b4_stride, h->cache.mv[0][48/*LENT_scan4[0]*/+(idx)*32],16*sizeof(h->cache.mv[0][0]))
#define COPY4( idx ) \
	COPY( idx + 0 ); \
	COPY( idx + 1 ); \
	COPY( idx + 2 ); \
	COPY( idx + 3 )

			COPY4(  0 );
			COPY4(  4 );
			COPY4(  8 );
			COPY4( 12 );

#undef  COPY4
#undef  COPY

			if( h->i_ref_count[1] )
			{
				rd = &h->current_picture.ref_index[1][b4xy];
				md = h->current_picture.motion_val[1][b4xy];

#define COPY( idx ) \
	memcpy( rd + (idx)*b4_stride, &h->cache.ref[1][48/*LENT_scan4[0]*/+(idx)*32], 16*sizeof(h->cache.ref[0][0]) ); \
	memcpy( md + ((idx)<<1)*b4_stride, h->cache.mv[1][48/*LENT_scan4[0]*/+(idx)*32],16*sizeof(h->cache.mv[0][0]))
#define COPY4( idx ) \
	COPY( idx + 0 ); \
	COPY( idx + 1 ); \
	COPY( idx + 2 ); \
	COPY( idx + 3 )

				COPY4(  0 );
				COPY4(  4 );
				COPY4(  8 );
				COPY4( 12 );

#undef  COPY4
#undef  COPY
			}
		}
	}
	//cbp

	//memcpy(&h->current.i_cbp[h->cu_index],h->cache.i_cbp,sizeof(h->cache.i_cbp));
	
}
int lent_hevc_decode_cu_cabac(HEVCContext *h)
{
	decode_cache_load(h);
		{
#if 0
#undef fprintf
		FILE *fp=fopen("logdec.txt","a");
		fprintf(fp,"poc %d CU %d\n",h->sh.m_iPOC,h->cu_index);
		fclose(fp);
#endif
	}
	if(h->cu_x<h->cu_width-1&&h->cu_y<h->cu_height-1)
		decode_tree_fast(h,CU_LOG2_64x64,0,0,0);//在图像内部的时候避免进行边界判断
	else
		decode_tree_edge(h,CU_LOG2_64x64,0,0,0);

	//todo 如果要支持质量层，则此函数要写到recon中，并且loadcache
	lent_deblock_strength( h );
	decode_cache_save(h);
	return 0;
}



/////////////////////////////////////////////////////////////////////////////////////////////////
//Zigzag
/////////////////////////////////////////////////////////////////////////////////////////////////

#define PACK(x,y) ((x)+((y)*i_width))

static lent_always_inline void lent_init_scan_horizontal( int16_t *scan, int16_t *scan_inv, int i_width, int i_height )
{
	int x, y, bx, by, i;

	by = 0; i = 0;
	while( by < i_height )
	{
		bx = 0;
		while( bx < i_width )
		{
			for( y = 0; y < 4; y ++ )
			{
				for( x = 0; x < 4; x ++ )
				{
					scan_inv[(by+y)*(i_width)+(bx+x)]=i;
					scan[i ++] = PACK(bx+x,by+y);
				}
			}

			bx += 4;
		}
		by += 4;
	}
}

static lent_always_inline void lent_init_scan_vertical( int16_t *scan, int16_t *scan_inv, int i_width, int i_height )
{
	int x, y, bx, by, i;

	bx = 0; i = 0;
	while( bx < i_width )
	{
		by = 0;
		while( by < i_height )
		{
			for( x = 0; x < 4; x ++ )
			{
				for( y = 0; y < 4; y ++ )
				{
					scan_inv[(by+y)*(i_width)+(bx+x)]=i;
					scan[i ++] = PACK(bx+x,by+y);
				}
			}

			by += 4;
		}
		bx += 4;
	}
}

void lent_init_scan_diag( int16_t *scan, int16_t *scan_inv, int i_width, int i_height )
{
	int x, y, bx, by, i, line;
	int i_total;
	int i_bwidth = i_width >> 2;
	int i_bheight = i_height >> 2;

	i_total = (i_width * i_height) >> 4;

	i = 0;
	for( line = 0; (i>>4) < i_total; line ++ )
	{
		by = line;
		bx = 0;
		while( by >= i_bheight )
		{
			by --;
			bx ++;
		}
		while( by >= 0 && bx < i_bwidth )
		{
			int l1, j = 0;
			for( l1 = 0; j < 16; l1 ++ )
			{
				y = l1;
				x = 0;
				while( y >= 4 )
				{
					y --;
					x ++;
				}
				while( y >= 0 && x < 4 )
				{
					scan_inv[PACK((bx<<2)+x,(by<<2)+y)]=i;
					scan[i ++] = PACK((bx<<2)+x,(by<<2)+y);
					y --; x ++; j ++;
				}
			}

			by --;
			bx ++;
		}
	}
}

#undef PACK


void lent_zigzag_uninit( HEVCContext *h )
{
	int i_lg2_w, i_lg2_h, i_scan;

	for( i_lg2_h = 2; i_lg2_h <= 5; i_lg2_h ++ )
		for( i_lg2_w = 2; i_lg2_w <= 5; i_lg2_w ++ )
			for( i_scan = 0; i_scan <= 3; i_scan ++ )
			{
				lent_freep( &h->scan_order[i_lg2_w-2][i_lg2_h-2][i_scan] );
				lent_freep( &h->scan_order_inv[i_lg2_w-2][i_lg2_h-2][i_scan] );
			}
}

int lent_zigzag_init( HEVCContext *h )
{
	int i_lg2_w, i_lg2_h, i_scan;

	for( i_lg2_h = 2; i_lg2_h <= 5; i_lg2_h ++ )
	{
		for( i_lg2_w = 2; i_lg2_w <= 5; i_lg2_w ++ )
		{
			int i_width =  1<<i_lg2_w;
			int i_height = 1<<i_lg2_h;

			if( i_lg2_h != i_lg2_w ) continue;//暂时只支持NxN变换以及相应的zigzag

			for( i_scan = 0; i_scan <= 3; i_scan ++ )
			{
				LENT_ALLOCZ_OR_GOTO(h, h->scan_order[i_lg2_w-2][i_lg2_h-2][i_scan], sizeof(int16_t)*i_width*i_height,fail );
				LENT_ALLOCZ_OR_GOTO(h, h->scan_order_inv[i_lg2_w-2][i_lg2_h-2][i_scan], sizeof(int16_t)*i_width*i_height,fail );
			}

			lent_init_scan_horizontal ( h->scan_order[i_lg2_w-2][i_lg2_h-2][1],h->scan_order_inv[i_lg2_w-2][i_lg2_h-2][1], i_width, i_height );
			lent_init_scan_vertical   ( h->scan_order[i_lg2_w-2][i_lg2_h-2][2],h->scan_order_inv[i_lg2_w-2][i_lg2_h-2][2], i_width, i_height );
			lent_init_scan_diag       ( h->scan_order[i_lg2_w-2][i_lg2_h-2][3],h->scan_order_inv[i_lg2_w-2][i_lg2_h-2][3], i_width, i_height );
		}
	}

	return 0;

fail:
	lent_zigzag_uninit( h );
	return -1;
}