//**********************************************
//hevc_ref.h
//Unipipy @2011
//reference list related code
//**********************************************
#ifndef DECODER_HEVC_REF_H
#define DECODER_HEVC_REF_H


/////////////////////////////////////////////////////////////////////////////////////////////////
//����ӿ�
/////////////////////////////////////////////////////////////////////////////////////////////////
void lent_build_def_list	(HEVCContext *h);
void lent_ref_insert		(HEVCContext *h,Picture *pic);
void lent_remove_all_refs	(HEVCContext *h);







#endif