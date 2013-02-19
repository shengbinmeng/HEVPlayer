//**********************************************
//hevc_deblock.h
//Unipipy @2011
//loopfilter
//**********************************************

#ifndef DECODER_HEVC_DEBLOCK_H
#define DECODER_HEVC_DEBLOCK_H


/////////////////////////////////////////////////////////////////////////////////////////////////
//对外接口
/////////////////////////////////////////////////////////////////////////////////////////////////

void lent_deblock_init(HEVCDeblockContext *ctx, unsigned int machine);
void lent_deblock_row( HEVCContext *h, int cu_y );
void lent_deblock_strength( HEVCContext *h );

#endif