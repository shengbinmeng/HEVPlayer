//**********************************************
//hevctypes.h
//Unipipy @2011
//type definitions of decoder core
//**********************************************

#ifndef DECODER_HEVCTYPES_H
#define DECODER_HEVCTYPES_H

enum nal_unit_type_e
{
	NAL_UNIT_CODED_SLICE_TRAIL_N = 0,   // 0
	NAL_UNIT_CODED_SLICE_TRAIL_R,   // 1

	NAL_UNIT_CODED_SLICE_TSA_N,     // 2
	NAL_UNIT_CODED_SLICE_TLA,       // 3   // Current name in the spec: TSA_R

	NAL_UNIT_CODED_SLICE_STSA_N,    // 4
	NAL_UNIT_CODED_SLICE_STSA_R,    // 5

	NAL_UNIT_CODED_SLICE_RADL_N,    // 6
	NAL_UNIT_CODED_SLICE_DLP,       // 7 // Current name in the spec: RADL_R

	NAL_UNIT_CODED_SLICE_RASL_N,    // 8
	NAL_UNIT_CODED_SLICE_TFD,       // 9 // Current name in the spec: RASL_R

	NAL_UNIT_RESERVED_10,
	NAL_UNIT_RESERVED_11,
	NAL_UNIT_RESERVED_12,
	NAL_UNIT_RESERVED_13,
	NAL_UNIT_RESERVED_14,
	NAL_UNIT_RESERVED_15,

	NAL_UNIT_CODED_SLICE_BLA,       // 16   // Current name in the spec: BLA_W_LP
	NAL_UNIT_CODED_SLICE_BLANT,     // 17   // Current name in the spec: BLA_W_DLP
	NAL_UNIT_CODED_SLICE_BLA_N_LP,  // 18
	NAL_UNIT_CODED_SLICE_IDR,       // 19  // Current name in the spec: IDR_W_DLP
	NAL_UNIT_CODED_SLICE_IDR_N_LP,  // 20
	NAL_UNIT_CODED_SLICE_CRA,       // 21
	NAL_UNIT_RESERVED_22,
	NAL_UNIT_RESERVED_23,

	NAL_UNIT_RESERVED_24,
	NAL_UNIT_RESERVED_25,
	NAL_UNIT_RESERVED_26,
	NAL_UNIT_RESERVED_27,
	NAL_UNIT_RESERVED_28,
	NAL_UNIT_RESERVED_29,
	NAL_UNIT_RESERVED_30,
	NAL_UNIT_RESERVED_31,

	NAL_UNIT_VPS,                   // 32
	NAL_UNIT_SPS,                   // 33
	NAL_UNIT_PPS,                   // 34
	NAL_UNIT_ACCESS_UNIT_DELIMITER, // 35
	NAL_UNIT_EOS,                   // 36
	NAL_UNIT_EOB,                   // 37
	NAL_UNIT_FILLER_DATA,           // 38
	NAL_UNIT_SEI,                   // 39 Prefix SEI
	NAL_UNIT_SEI_SUFFIX,            // 40 Suffix SEI

	NAL_UNIT_RESERVED_41,
	NAL_UNIT_RESERVED_42,
	NAL_UNIT_RESERVED_43,
	NAL_UNIT_RESERVED_44,
	NAL_UNIT_RESERVED_45,
	NAL_UNIT_RESERVED_46,
	NAL_UNIT_RESERVED_47,
	NAL_UNIT_UNSPECIFIED_48,
	NAL_UNIT_UNSPECIFIED_49,
	NAL_UNIT_UNSPECIFIED_50,
	NAL_UNIT_UNSPECIFIED_51,
	NAL_UNIT_UNSPECIFIED_52,
	NAL_UNIT_UNSPECIFIED_53,
	NAL_UNIT_UNSPECIFIED_54,
	NAL_UNIT_UNSPECIFIED_55,
	NAL_UNIT_UNSPECIFIED_56,
	NAL_UNIT_UNSPECIFIED_57,
	NAL_UNIT_UNSPECIFIED_58,
	NAL_UNIT_UNSPECIFIED_59,
	NAL_UNIT_UNSPECIFIED_60,
	NAL_UNIT_UNSPECIFIED_61,
	NAL_UNIT_UNSPECIFIED_62,
	NAL_UNIT_UNSPECIFIED_63,
	NAL_UNIT_INVALID,
};

enum slice_type_e
{
	SLICE_B,
	SLICE_P,
	SLICE_I,
	SLICE_ERROR,
};

enum LENT_pic_type_e
{
	LENT_PIC_PENDING = 0,
	LENT_PIC_IDR,
	LENT_PIC_CDR,//TODO: CDR is a pic type? CDR frame must be I frame?
	LENT_PIC_I,
	LENT_PIC_P,
	LENT_PIC_BREF,
	LENT_PIC_B,
	LENT_PIC_ERROR,
};


enum {
	CU_LOG2_1x1,//dummy
	CU_LOG2_2x2,//dummy
	CU_LOG2_4x4,
	CU_LOG2_8x8,
	CU_LOG2_16x16,
	CU_LOG2_32x32,
	CU_LOG2_64x64,
	CU_LOG2_ERROR,
};

enum cu_neighbor_e
{
	CU_LEFT		= 0x01,
	CU_TOP		= 0x02,
	CU_TOPRIGHT	= 0x04,
	//CU_TOPLEFT	= 0x08,
	//CU_DOWNLEFT = 0x10,

	CU_ALL		= 0x1f
};

enum cu_part_size_e
{
	SIZE_2Nx2N = 0,           ///< symmetric motion partition,  2Nx2N
	SIZE_2NxN  = 1,            ///< symmetric motion partition,  2Nx N
	SIZE_Nx2N  = 2,            ///< symmetric motion partition,   Nx2N
	SIZE_NxN   = 3,             ///< symmetric motion partition,   Nx N  
	SIZE_NONE = 15
};

enum cu_pred_mode_e
{
	MODE_SKIP,            ///< SKIP mode
	MODE_INTER,           ///< inter-prediction mode
	MODE_INTRA,           ///< intra-prediction mode
	MODE_NONE = 15
};

enum scan_type_e
{
	SCAN_ZIGZAG = 0,
	SCAN_HOR,
	SCAN_VER,
	SCAN_DIAG
};

enum ScalingListSize_e
{
  SCALING_LIST_4x4 = 0,
  SCALING_LIST_8x8,
  SCALING_LIST_16x16,
  SCALING_LIST_32x32,
  SCALING_LIST_SIZE_NUM
};

#define NTAPS_LUMA        8 ///< Number of taps for luma
#define NTAPS_CHROMA      4 ///< Number of taps for chroma
#define IF_INTERNAL_PREC 14 ///< Number of bits for internal precision
#define IF_FILTER_PREC    6 ///< Log2 of sum of filter taps
#define IF_INTERNAL_OFFS (1<<(IF_INTERNAL_PREC-1)) ///< Offset used internally

#define SCALING_LIST_NUM 6         ///< list number for quantization matrix
#define SCALING_LIST_NUM_32x32 2   ///< list number for quantization matrix 32x32
#define SCALING_LIST_REM_NUM 6     ///< remainder of QP/6
#define SCALING_LIST_START_VALUE 8 ///< start value for dpcm mode
#define MAX_MATRIX_COEF_NUM 64     ///< max coefficient number for quantization matrix
#define MAX_MATRIX_SIZE_NUM 8      ///< max size number for quantization matrix
#define SCALING_LIST_DC 16         ///< default DC value


#define MAX_GOP                     64          ///< max. value of hierarchical GOP size

#define MAX_NUM_REF_PICS            16          ///< max. number of pictures used for reference
#define MAX_NUM_REF                 16          ///< max. number of entries in picture reference list
#define MAX_NUM_REF_LC              MAX_NUM_REF_PICS  // TODO: remove this macro definition (leftover from combined list concept)

#define MAX_CHROMA_FORMAT_IDC   3


#define MAX_VPS_COUNT			(4)
#define MAX_SPS_COUNT			(4)
#define MAX_PPS_COUNT			(4)
#define MAX_TEMPORAL_LAYER		(5)
#define MAX_DELAYED_PIC_COUNT	(9)
#define MAX_DPB_SIZE			(9)
#define MAX_REF					(16)


#define MAX_CU_DEPTH			(3)
#define MIN_CU_LOG2				(CU_LOG2_64x64-MAX_CU_DEPTH)
#define MAX_CU_LOG2				(CU_LOG2_64x64)
#define MIN_BLOCK_LOG2			(3)

#define IDXSIZE_LOG2			(2)


#define CU_TOTAL_PART 341

#define LENT_SCAN_SIZE			(16*10)
#define LENT_SCAN_LUMA_SIZE		(16*9)


#define CBF_LUMA_FLAG			0x01
#define CBF_CB_FLAG				0x02
#define CBF_CR_FLAG				0x04

#define CBF_LUMA(x)				((x)&0x01)
#define CBF_CHROMA(x)			(((x))&0x06)
#define CBF_CB(x)				(((x))&0x02)
#define CBF_CR(x)				(((x))&0x04)



#define IDX2x(idx)				((((idx)&1)+(((idx)&4)>>2)*2+(((idx)&16)>>4)*4+(((idx)&64)>>6)*8)*(1<<IDXSIZE_LOG2))
#define IDX2y(idx)				((((idx)&2)+(((idx)&8)>>2)*2+(((idx)&32)>>4)*4+(((idx)&128)>>6)*8)*(1<<IDXSIZE_LOG2)/2)

#define DEPTH2IDXSTEPP(depth)	((1<<((MAX_CU_LOG2-IDXSIZE_LOG2)<<1))>>((depth)<<1))		//todo need optimize?//nope
#define DEPTH2IDXSTEPC(depth)	(DEPTH2IDXSTEPP((depth))>>2)
#define DEPTH2LOG2(depth)		(MAX_CU_LOG2-(depth))
#define LOG22I4WIDTH(log2)		(1<<((log2)-CU_LOG2_4x4))
#define LOG22I8WIDTH(log2)		(1<<((log2)-CU_LOG2_8x8))
#define LOG22DEPTH(log2)		(MAX_CU_LOG2-(log2))
#define TREEIDX(idx,log2)		(LENT_cu_depth_treeidx[LOG22DEPTH(log2)] + (((idx)>>((log2)-IDXSIZE_LOG2))>>((log2)-IDXSIZE_LOG2)))


#define dctcoef					int16_t
#define pixel					uint8_t

#endif