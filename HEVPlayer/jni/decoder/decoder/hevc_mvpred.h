//**********************************************
//hevc_mvpred.h
//Unipipy @2011
//mv prediction
//**********************************************

#ifndef DECODER_HEVC_MVPRED_H
#define DECODER_HEVC_MVPRED_H

int lent_predict_mv( HEVCContext *h, int i_idx, int x, int y, int width, int height, int i_depth,
					int list, int16_t mvp[][2] );
int LENT_predict_mv_merge( HEVCContext *h, int i_idx, int x, int y, int width, int height, int i_depth,
						  int16_t mvp0[][2], int16_t mvp1[][2], int8_t ref0[], int8_t ref1[], int i_mvp_idx );
static lent_always_inline void lent_clip_mv( HEVCContext *h, int16_t mv[2], int x, int y )
{
	int i_hor_max = (h->width - (h->cu_x<<6) - x + 8 - 1) << 2;
	int i_hor_min = (-(h->cu_x<<6) - x - 8 - 64 + 1) << 2;
	int i_ver_max = (h->height - (h->cu_y<<6) - y + 8 - 1) << 2;
	int i_ver_min = (-(h->cu_y<<6) - y - 8 - 64 + 1) << 2;

	mv[0] = lent_clip( mv[0], i_hor_min, i_hor_max );
	mv[1] = lent_clip( mv[1], i_ver_min, i_ver_max );
}

#endif