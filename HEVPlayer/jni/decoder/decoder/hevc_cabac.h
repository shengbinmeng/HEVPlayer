//**********************************************
//hevc_cabac.h
//Unipipy @2011
//cabac functions to decode syntaxes
//**********************************************
#ifndef DECODER_HEVC_CABAC_H
#define DECODER_HEVC_CABAC_H


#define OFS_CTX_START				(0)

#define OFS_SPLIT_FLAG_CTX			(0		+OFS_CTX_START)
//#define NUM_SPLIT_FLAG_CTX		(3)

#define OFS_SKIP_FLAG_CTX			(3		+OFS_SPLIT_FLAG_CTX)
//#define NUM_SKIP_FLAG_CTX			(3)

#define OFS_MERGE_FLAG_EXT_CTX		(3		+OFS_SKIP_FLAG_CTX)
//#define NUM_MERGE_FLAG_EXT_CTX	(1)

#define OFS_MERGE_IDX_EXT_CTX		(1		+OFS_MERGE_FLAG_EXT_CTX)
//#define NUM_MERGE_IDX_EXT_CTX		(1)


#define OFS_PART_SIZE_CTX			(1		+OFS_MERGE_IDX_EXT_CTX)
//#define NUM_PART_SIZE_CTX			(4)

#define OFS_CU_AMP_CTX				(4		+OFS_PART_SIZE_CTX)
//#define NUM_CU_AMP_CTX			(1)

#define OFS_PRED_MODE_CTX			(1		+OFS_CU_AMP_CTX)
//#define NUM_PRED_MODE_CTX			(1)

#define OFS_INTRA_PRED_MODE_CTX					(1		+OFS_PRED_MODE_CTX)
//#define NUM_ADI_CTX				(1)

#define OFS_CHROMA_PRED_CTX			(1		+OFS_INTRA_PRED_MODE_CTX)
//#define NUM_CHROMA_PRED_CTX		(2)
#define OFS_INTER_DIR_CTX			(2		+OFS_CHROMA_PRED_CTX)
//#define NUM_INTER_DIR_CTX			(5)
#define OFS_MV_RES_CTX				(5		+OFS_INTER_DIR_CTX)
//#define NUM_MV_RES_CTX			(2)

#define OFS_REF_NO_CTX				(2		+OFS_MV_RES_CTX)
//#define NUM_REF_NO_CTX			(2)
#define OFS_TRANS_SUBDIV_FLAG_CTX	(2		+OFS_REF_NO_CTX)
//#define NUM_TRANS_SUBDIV_FLAG_CTX	(3)
#define OFS_QT_CBF_CTX				(3		+OFS_TRANS_SUBDIV_FLAG_CTX)
//#define NUM_QT_CBF_CTX			(5)
#define OFS_QT_ROOT_CBF_CTX			(5*2		+OFS_QT_CBF_CTX)
//#define NUM_QT_ROOT_CBF_CTX		(1)
#define OFS_DELTA_QP_CTX			(1		+OFS_QT_ROOT_CBF_CTX)
//#define NUM_DELTA_QP_CTX			(3)

#define OFS_SIG_CG_FLAG_CTX			(3		+OFS_DELTA_QP_CTX)
//#define NUM_SIG_CG_FLAG_CTX		(2)

#define OFS_SIG_FLAG_CTX_LUMA		(2*2		+OFS_SIG_CG_FLAG_CTX)
//#define NUM_SIG_FLAG_CTX_LUMA		(27)
#define OFS_SIG_FLAG_CTX_CHROMA		(27		+OFS_SIG_FLAG_CTX_LUMA)
//#define NUM_SIG_FLAG_CTX_CHROMA	(15)

#define OFS_CTX_LAST_FLAG_X			(15		+OFS_SIG_FLAG_CTX_CHROMA)
//#define NUM_CTX_LAST_FLAG_XY		(15)

#define OFS_CTX_LAST_FLAG_Y			(15*2		+OFS_CTX_LAST_FLAG_X)
//#define NUM_CTX_LAST_FLAG_XY		(15)

#define OFS_ONE_FLAG_CTX_LUMA		(15*2		+OFS_CTX_LAST_FLAG_Y)
//#define NUM_ONE_FLAG_CTX_LUMA		(16)
#define OFS_ONE_FLAG_CTX_CHROMA		(16		+OFS_ONE_FLAG_CTX_LUMA)
//#define NUM_ONE_FLAG_CTX_CHROMA	(8)

#define OFS_ABS_FLAG_CTX_LUMA		(8		+OFS_ONE_FLAG_CTX_CHROMA)
//#define NUM_ABS_FLAG_CTX_LUMA		(4)
#define OFS_ABS_FLAG_CTX_CHROMA		(4		+OFS_ABS_FLAG_CTX_LUMA)
//#define NUM_ABS_FLAG_CTX_CHROMA	(2)


#define OFS_MVP_IDX_CTX				(2		+OFS_ABS_FLAG_CTX_CHROMA)
//#define NUM_MVP_IDX_CTX			(2)

#define OFS_SAO_MERGE_FLAG_CTX				(2		+OFS_MVP_IDX_CTX)
//#define NUM_SAO_MERGE_FLAG_CTX			(1)

#define OFS_SAO_TYPE_IDX_CTX				(1		+OFS_SAO_MERGE_FLAG_CTX)
//#define NUM_SAO_TYPE_IDX_CTX				(1)

#define OFS_TRANSFORMSKIP_FLAG_CTX			(1		+OFS_SAO_TYPE_IDX_CTX)
//#define NUM_TRANSFORMSKIP_FLAG_CTX		(2)

#define OFS_CU_TRANSQUANT_BYPASS_FLAG_CTX	(2	+OFS_TRANSFORMSKIP_FLAG_CTX)
//#define NUM_CU_TRANSQUANT_BYPASS_FLAG_CTX	(1)

#define CNU							(154)

#define MAX_CABAC_STATE (1+OFS_CU_TRANSQUANT_BYPASS_FLAG_CTX)




/////////////////////////////////////////////////////////////////////////////////////////////////
//cabac
/////////////////////////////////////////////////////////////////////////////////////////////////
int		lent_hevc_decode_cu_cabac		(HEVCContext *h);
void	lent_hevc_init_cabac_states		(HEVCContext *h);

/////////////////////////////////////////////////////////////////////////////////////////////////
//Zigzag
/////////////////////////////////////////////////////////////////////////////////////////////////
int		lent_zigzag_init				(HEVCContext *h);
void	lent_zigzag_uninit				(HEVCContext *h);




#endif