//**********************************************
//hevc_pred.h
//Unipipy @2011
//prediction tables & functions
//**********************************************
#ifndef DECODER_HEVC_PRED_H
#define DECODER_HEVC_PRED_H

#define CHROMA_FROM_LUMA 4

#define PLANAR_IDX             0
#define VER_IDX                26                    // index for intra VERTICAL   mode
#define HOR_IDX                10                    // index for intra HORIZONTAL mode
#define DC_IDX                 1                     // index for intra DC mode



/////////////////////////////////////////////////////////////////////////////////////////////////
//对外接口
/////////////////////////////////////////////////////////////////////////////////////////////////

void lent_predict_edge_filter( pixel *src, int i_src, int i_log2_size, int b_ver );

void lent_predict_load_neighbor( HEVCContext *h, pixel *src, int i_src, int i_pix_x, int i_pix_y, int i_size, int b_luma );
void lent_predict_dc_filter( pixel *src, int i_src, int i_width );

static const int chroma_mode_trans_table[4] = { 0, 26, 10, 1 };
static lent_always_inline int lent_get_chroma_mode( int i_luma_mode, int i_chroma_type )
{
	if( i_chroma_type == CHROMA_FROM_LUMA )
		return i_luma_mode;
	else if( chroma_mode_trans_table[i_chroma_type] == i_luma_mode )
		return 34;
	else
		return chroma_mode_trans_table[i_chroma_type];
}

int lent_pred_init(HEVCPredContext *ctx, unsigned int machine);

#endif