//**********************************************
//hevc.h
//Unipipy @2011
//header of decoder core
//**********************************************

#ifndef DECODER_HEVC_H
#define DECODER_HEVC_H


#include "internal.h"
typedef struct LentPacket {
    int64_t pts;
    int64_t dts;
    uint8_t *data;
    int   size;
} LentPacket;

/////////////////////////////////////////////////////////////////////////////////////////////////
//Picture,主要的帧数据结构
/////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct Picture{
	LENTOIDHE_COMMON_FRAME
	int16_t poc;
	int8_t	idr;

    struct HEVCContext *owner2; ///< 分配者指针
}Picture;

/////////////////////////////////////////////////////////////////////////////////////////////////
//SPS PPS SliceHeader
//copied from encoder 2011.11.1
/////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct ALFParam ALFParam;
typedef unsigned int UInt;
typedef int Int;
typedef int Bool;
typedef UInt AMVP_MODE;

#define PIC_CROPPING 1
#define H0736_AVC_STYLE_QP_RANGE 1
#define LOSSLESS_CODING 1
#define H0567_DPB_PARAMETERS_PER_TEMPORAL_LAYER 1
#define H0412_REF_PIC_LIST_RESTRICTION 1
#define LCU_SYNTAX_ALF 1
#define RPS_IN_SPS 1
#define TILES_WPP_ENTRY_POINT_SIGNALLING 1
#define TILES_OR_ENTROPY_SYNC_IDC 1
#define REMOVE_TILE_DEPENDENCE 1
#define MAX_TLAYER 8
#define MAX_CU_DEPTH_HM 7

#define MAX_NUM_REF_PICS 16
#define LTRP_MULT 1


#define MAX_CPB_CNT                     32  ///< Upper bound of (cpb_cnt_minus1 + 1)


  #define MAX_VPS_NUM_HRD_PARAMETERS                1
  #define MAX_VPS_NUM_HRD_PARAMETERS_ALLOWED_PLUS1  1024
  #define MAX_VPS_NUH_RESERVED_ZERO_LAYER_ID_PLUS1  1

typedef struct {
	Int  m_numberOfPictures;
	Int  m_numberOfNegativePictures;
	Int  m_numberOfPositivePictures;
	Int  m_numberOfLongtermPictures;
	Int  m_deltaPOC[MAX_NUM_REF_PICS];
	Int  m_POC[MAX_NUM_REF_PICS];
	Bool m_used[MAX_NUM_REF_PICS];
	Bool m_interRPSPrediction;
	Int  m_deltaRIdxMinus1;   
	Int  m_deltaRPS; 
	Int  m_numRefIdc; 
	Int  m_refIdc[MAX_NUM_REF_PICS+1];
	Bool m_bCheckLTMSB[MAX_NUM_REF_PICS];
	Int  m_pocLSBLT[MAX_NUM_REF_PICS];
	Int  m_deltaPOCMSBCycleLT[MAX_NUM_REF_PICS];
	Bool m_deltaPocMSBPresentFlag[MAX_NUM_REF_PICS];
} RPS;

typedef struct
{
	Int     m_profileSpace;
	Bool    m_tierFlag;
	Int     m_profileIdc;
	Bool    m_profileCompatibilityFlag[32];
	Int     m_levelIdc;
}ProfileTierLevel;
typedef struct PTL
{
	ProfileTierLevel m_generalPTL;
	ProfileTierLevel m_subLayerPTL[6];      // max. value of max_sub_layers_minus1 is 6
	Bool m_subLayerProfilePresentFlag[6];
	Bool m_subLayerLevelPresentFlag[6];
} PTL;
typedef struct TComBitRatePicRateInfo
{
  Bool        m_bitRateInfoPresentFlag[MAX_TLAYER];
  Bool        m_picRateInfoPresentFlag[MAX_TLAYER];
  Int         m_avgBitRate[MAX_TLAYER];
  Int         m_maxBitRate[MAX_TLAYER];
  Int         m_constantPicRateIdc[MAX_TLAYER];
  Int         m_avgPicRate[MAX_TLAYER];
}TComBitRatePicRateInfo;
typedef struct
{
	Int         m_VPSId;
	UInt        m_uiMaxTLayers;
	UInt        m_uiMaxLayers;
	Bool        m_bTemporalIdNestingFlag;

	UInt        m_numReorderPics[MAX_TLAYER];
	UInt        m_uiMaxDecPicBuffering[MAX_TLAYER]; 
	UInt        m_uiMaxLatencyIncrease[MAX_TLAYER];

	//hrd
	UInt        m_numHrdParameters;
	UInt        m_maxNuhReservedZeroLayerId;
	Bool        m_opLayerIdIncludedFlag[MAX_VPS_NUM_HRD_PARAMETERS_ALLOWED_PLUS1][MAX_VPS_NUH_RESERVED_ZERO_LAYER_ID_PLUS1];

	PTL         m_pcPTL;
	
	//bitrate info
	TComBitRatePicRateInfo    m_bitRatePicRateInfo;
} VPS;

typedef struct
{
	Int      m_scalingListDC               [SCALING_LIST_SIZE_NUM][SCALING_LIST_NUM]; //!< the DC value of the matrix coefficient for 16x16
	Bool     m_useDefaultScalingMatrixFlag [SCALING_LIST_SIZE_NUM][SCALING_LIST_NUM]; //!< UseDefaultScalingMatrixFlag
	UInt     m_refMatrixId                 [SCALING_LIST_SIZE_NUM][SCALING_LIST_NUM]; //!< RefMatrixID
	Bool     m_scalingListPresentFlag;                                                //!< flag for using default matrix
	UInt     m_predMatrixId                [SCALING_LIST_SIZE_NUM][SCALING_LIST_NUM]; //!< reference list index
	Int      *m_scalingListCoef            [SCALING_LIST_SIZE_NUM][SCALING_LIST_NUM]; //!< quantization matrix
	Bool     m_useTransformSkip;                                                      //!< transform skipping flag for setting default scaling matrix for 4x4
}ScalingList;

typedef struct
{
	Bool fixedPicRateFlag;
	UInt picDurationInTcMinus1;
	Bool lowDelayHrdFlag;
	UInt cpbCntMinus1;
	UInt bitRateValueMinus1[MAX_CPB_CNT][2];
	UInt cpbSizeValue      [MAX_CPB_CNT][2];
	UInt duCpbSizeValue    [MAX_CPB_CNT][2];
	UInt cbrFlag           [MAX_CPB_CNT][2];
}HrdSubLayerInfo;
typedef struct
{
  Bool m_aspectRatioInfoPresentFlag;
  Int  m_aspectRatioIdc;
  Int  m_sarWidth;
  Int  m_sarHeight;
  Bool m_overscanInfoPresentFlag;
  Bool m_overscanAppropriateFlag;
  Bool m_videoSignalTypePresentFlag;
  Int  m_videoFormat;
  Bool m_videoFullRangeFlag;
  Bool m_colourDescriptionPresentFlag;
  Int  m_colourPrimaries;
  Int  m_transferCharacteristics;
  Int  m_matrixCoefficients;
  Bool m_chromaLocInfoPresentFlag;
  Int  m_chromaSampleLocTypeTopField;
  Int  m_chromaSampleLocTypeBottomField;
  Bool m_neutralChromaIndicationFlag;
  Bool m_fieldSeqFlag;
  Bool m_picStructPresentFlag;
  Bool m_hrdParametersPresentFlag;
  Bool m_bitstreamRestrictionFlag;
  Bool m_tilesFixedStructureFlag;
  Bool m_motionVectorsOverPicBoundariesFlag;
  Bool m_restrictedRefPicListsFlag;
  Int  m_minSpatialSegmentationIdc;
  Int  m_maxBytesPerPicDenom;
  Int  m_maxBitsPerMinCuDenom;
  Int  m_log2MaxMvLengthHorizontal;
  Int  m_log2MaxMvLengthVertical;
  Bool m_timingInfoPresentFlag;
  UInt m_numUnitsInTick;
  UInt m_timeScale;
  Bool m_nalHrdParametersPresentFlag;
  Bool m_vclHrdParametersPresentFlag;
  Bool m_subPicCpbParamsPresentFlag;
  UInt m_tickDivisorMinus2;
  UInt m_duCpbRemovalDelayLengthMinus1;
  UInt m_bitRateScale;
  UInt m_cpbSizeScale;
  UInt m_duCpbSizeScale;
  UInt m_initialCpbRemovalDelayLengthMinus1;
  UInt m_cpbRemovalDelayLengthMinus1;
  UInt m_dpbOutputDelayLengthMinus1;
  UInt m_numDU;
  HrdSubLayerInfo m_HRD[MAX_TLAYER];
  Bool m_pocProportionalToTimingFlag;
  Int  m_numTicksPocDiffOneMinus1;
}VUI;
typedef struct CroppingWindow
{
  Bool        m_picCroppingFlag;
  Int         m_picCropLeftOffset;
  Int         m_picCropRightOffset;
  Int         m_picCropTopOffset;
  Int         m_picCropBottomOffset;
}CroppingWindow;
typedef struct
{
  Int         m_SPSId;
  Int         m_VPSId;
  Int         m_chromaFormatIdc;

  UInt        m_uiMaxTLayers;           // maximum number of temporal layers

  // Structure
  UInt        m_picWidthInLumaSamples;
  UInt        m_picHeightInLumaSamples;

  CroppingWindow m_picCroppingWindow;

  UInt        m_uiMaxCUWidth;
  UInt        m_uiMaxCUHeight;
  UInt        m_uiMaxCUDepth;
  UInt        m_uiMinTrDepth;
  UInt        m_uiMaxTrDepth;
  UInt        m_RPSNum;//pipy 
  RPS*        m_RPSList;
  Bool        m_bLongTermRefsPresent;
  Bool        m_TMVPFlagsPresent;
  Int         m_numReorderPics[MAX_TLAYER];
  
  // Tool list
  UInt        m_uiCULog2MinSize;//pipy
  UInt        m_uiQuadtreeTULog2MaxSize;
  UInt        m_uiQuadtreeTULog2MinSize;
  UInt        m_uiQuadtreeTUMaxDepthInter;
  UInt        m_uiQuadtreeTUMaxDepthIntra;
  Bool        m_usePCM;
  UInt        m_pcmLog2MaxSize;
  UInt        m_uiPCMLog2MinSize;
  Bool        m_useAMP;

  Bool        m_bUseLComb;
  
  Bool        m_restrictedRefPicListsFlag;
  Bool        m_listsModificationPresentFlag;

  // Parameter
  UInt        m_uiBitDepthY;
  Int         m_qpBDOffsetY;
  UInt        m_uiBitDepthC;
  Int         m_qpBDOffsetC;

  Bool        m_useLossless;

  UInt        m_uiPCMBitDepthLuma;
  UInt        m_uiPCMBitDepthChroma;
  Bool        m_bPCMFilterDisableFlag;

  UInt        m_uiBitsForPOC;
  UInt        m_numLongTermRefPicSPS;
  UInt        m_ltRefPicPocLsbSps[33];
  Bool        m_usedByCurrPicLtSPSFlag[33];
  // Max physical transform size
  UInt        m_uiMaxTrSize;
  
  Int         m_iAMPAcc[MAX_CU_DEPTH];
  Bool        m_bUseSAO; 

  Bool        m_bTemporalIdNestingFlag; // temporal_id_nesting_flag

  Bool        m_scalingListEnabledFlag;
  Bool        m_scalingListPresentFlag;
  ScalingList     m_scalingList;   //!< ScalingList class pointer
  UInt        m_uiMaxDecPicBuffering[MAX_TLAYER]; 
  UInt        m_uiMaxLatencyIncrease[MAX_TLAYER];

  Bool        m_useDF;
//#if STRONG_INTRA_SMOOTHING
  Bool        m_useStrongIntraSmoothing; 
//#endif

  Bool        m_vuiParametersPresentFlag;
  VUI         m_vuiParameters;

  PTL         m_pcPTL;
} SPS;

const Int   m_cropUnitX[MAX_CHROMA_FORMAT_IDC+1];
const Int   m_cropUnitY[MAX_CHROMA_FORMAT_IDC+1];

#define H0566_TLA 1
#define H0388 1
#define MULTIBITS_DATA_HIDING 1
#define CABAC_INIT_FLAG 1
#define DBL_CONTROL 1
#define PARALLEL_MERGE 1
#define WPP_SIMPLIFICATION 1
typedef struct _PPS
{
	Int         m_PPSId;                    // pic_parameter_set_id
	Int         m_SPSId;                    // seq_parameter_set_id
	Int         m_picInitQPMinus26;
	Bool        m_useDQP;
	Bool        m_bConstrainedIntraPred;    // constrained_intra_pred_flag
	Bool        m_bSliceChromaQpFlag;       // slicelevel_chroma_qp_flag

	// access channel
	SPS*		m_pcSPS;
	UInt        m_uiMaxCuDQPDepth;
	UInt        m_uiMinCuDQPSize;

	Int         m_chromaCbQpOffset;
	Int         m_chromaCrQpOffset;

	UInt        m_numRefIdxL0DefaultActive;
	UInt        m_numRefIdxL1DefaultActive;

	Bool        m_bUseWeightedPred;           // Use of Weighting Prediction (P_SLICE)
	Bool        m_bUseWeightedBiPred;        // Use of Weighting Bi-Prediction (B_SLICE)
	Bool        m_OutputFlagPresentFlag;   // Indicates the presence of output_flag in slice header

	Bool        m_TransquantBypassEnableFlag; // Indicates presence of cu_transquant_bypass_flag in CUs.
	Bool        m_useTransformSkip;
	Bool        m_dependentSliceEnabledFlag;     //!< Indicates the presence of dependent slices
	Bool        m_tilesEnabledFlag;              //!< Indicates the presence of tiles
	Bool        m_entropyCodingSyncEnabledFlag;  //!< Indicates the presence of wavefronts

	Bool     m_loopFilterAcrossTilesEnabledFlag;
	Int      m_uniformSpacingFlag;
	Int      m_iNumColumnsMinus1;
	UInt*    m_puiColumnWidth;
	Int      m_iNumRowsMinus1;
	UInt*    m_puiRowHeight;

	Int      m_iNumSubstreams;

	Int      m_signHideFlag;

	Bool     m_cabacInitPresentFlag;
	UInt     m_encCABACTableIdx;           // Used to transmit table selection across slices

	Bool     m_sliceHeaderExtensionPresentFlag;
	Bool        m_loopFilterAcrossSlicesEnabledFlag;
	Bool     m_deblockingFilterControlPresentFlag;
	Bool     m_deblockingFilterOverrideEnabledFlag;
	Bool     m_picDisableDeblockingFilterFlag;
	Int      m_deblockingFilterBetaOffsetDiv2;    //< beta offset for deblocking filter
	Int      m_deblockingFilterTcOffsetDiv2;      //< tc offset for deblocking filter
	Bool     m_scalingListPresentFlag;
	ScalingList     m_scalingList;   //!< ScalingList class pointer
	Bool m_listsModificationPresentFlag;
	UInt m_log2ParallelMergeLevelMinus2;
	Int m_numExtraSliceHeaderBits;

} PPS;



typedef int NalUnitType;
typedef int SliceType;
typedef double Double;

enum RefPicList
{
  REF_PIC_LIST_0 = 0,   ///< reference list 0
  REF_PIC_LIST_1 = 1,   ///< reference list 1
  REF_PIC_LIST_C = 2,   ///< combined reference list for uni-prediction in B-Slices
  REF_PIC_LIST_X = 100  ///< special mark
};

typedef struct
{
  UInt      m_bRefPicListModificationFlagL0;  
  UInt      m_bRefPicListModificationFlagL1;  

  UInt      m_RefPicSetIdxL0[32];

  UInt      m_RefPicSetIdxL1[32];
} TComRefPicListModification;

typedef struct _wpScalingParam{
  // Explicit weighted prediction parameters parsed in slice header,
  // or Implicit weighted prediction parameters (8 bits depth values).
  Bool        bPresentFlag;
  UInt        uiLog2WeightDenom;
  Int         iWeight;
  Int         iOffset;

  // Weighted prediction scaling values built from above parameters (bitdepth scaled):
  Int         w, o, offset, shift/*, round*/;
} wpScalingParam;

typedef struct _SliceHeader
{
	Bool        m_saoEnabledFlag;
	Bool        m_saoEnabledFlagChroma;      ///< SAO Cb&Cr enabled flag
	Int         m_iPPSId;               ///< picture parameter set ID
	Bool        m_PicOutputFlag;        ///< pic_output_flag 
	Int         m_iPOC;
	Int         m_iLastIDR;
	Int         m_prevPOC;
	RPS        *m_pcRPS;
	RPS         m_LocalRPS;
	Int         m_iBDidx; 
	Int         m_iCombinationBDidx;
	Bool        m_bCombineWithReferenceFlag;
	TComRefPicListModification m_RefPicListModification;
	NalUnitType m_eNalUnitType;         ///< Nal unit type for the slice
	SliceType   m_eSliceType;
	Int         m_iSliceQp;
	Bool        m_dependentSliceFlag;
	//#if ADAPTIVE_QP_SELECTION
	Int         m_iSliceQpBase;
	//#endif
	Bool        m_deblockingFilterDisable;
	Bool        m_deblockingFilterOverrideFlag;      //< offsets for deblocking filter inherit from PPS
	Int         m_deblockingFilterBetaOffsetDiv2;    //< beta offset for deblocking filter
	Int         m_deblockingFilterTcOffsetDiv2;      //< tc offset for deblocking filter

	Int         m_aiNumRefIdx   [3];    //  for multiple reference of current slice

	Int         m_iRefIdxOfLC[2][MAX_NUM_REF_LC];
	Int         m_eListIdFromIdxOfLC[MAX_NUM_REF_LC];
	Int         m_iRefIdxFromIdxOfLC[MAX_NUM_REF_LC];
	Int         m_iRefIdxOfL1FromRefIdxOfL0[MAX_NUM_REF_LC];
	Int         m_iRefIdxOfL0FromRefIdxOfL1[MAX_NUM_REF_LC];
	Bool        m_bRefPicListModificationFlagLC;
	Bool        m_bRefPicListCombinationFlag;

	Bool        m_bCheckLDC;

	//  Data
	Int         m_iSliceQpDelta;
	Int         m_iSliceQpDeltaCb;
	Int         m_iSliceQpDeltaCr;
	//TComPic*    m_apcRefPicList [2][MAX_NUM_REF+1];
	Int         m_aiRefPOCList  [2][MAX_NUM_REF+1];
	Int         m_iDepth;

	// referenced slice?
	Bool        m_bRefenced;

	// access channel
	VPS*        m_pcVPS;
	SPS*        m_pcSPS;
	PPS*        m_pcPPS;
	Picture*    m_pcPic;
	//#if ADAPTIVE_QP_SELECTION
	//TComTrQuant* m_pcTrQuant;
	//#endif  
	UInt        m_colFromL0Flag;  // collocated picture from List0 flag

	UInt        m_colRefIdx;
	UInt        m_maxNumMergeCand;


	//#if SAO_CHROMA_LAMBDA
	Double      m_dLambdaLuma;
	Double      m_dLambdaChroma;
	//#else
	//  Double      m_dLambda;
	//#endif

	Bool        m_abEqualRef  [2][MAX_NUM_REF][MAX_NUM_REF];

	Bool        m_bNoBackPredFlag;
	UInt        m_uiTLayer;
	Bool        m_bTLayerSwitchingFlag;

	UInt        m_uiSliceMode;
	UInt        m_uiSliceArgument;
	UInt        m_uiSliceCurStartCUAddr;
	UInt        m_uiSliceCurEndCUAddr;
	UInt        m_uiSliceIdx;
	UInt        m_uiDependentSliceMode;
	UInt        m_uiDependentSliceArgument;
	UInt        m_uiDependentSliceCurStartCUAddr;
	UInt        m_uiDependentSliceCurEndCUAddr;
	Bool        m_bNextSlice;
	Bool        m_bNextDependentSlice;
	UInt        m_uiSliceBits;
	UInt        m_uiDependentSliceCounter;
	Bool        m_bFinalized;

	wpScalingParam  m_weightPredTable[2][MAX_NUM_REF][3]; // [REF_PIC_LIST_0 or REF_PIC_LIST_1][refIdx][0:Y, 1:U, 2:V]
	//wpACDCParam    m_weightACDCParam[3];                 // [0:Y, 1:U, 2:V]

	//std::vector<UInt> m_tileByteLocation;
	UInt        m_uiTileOffstForMultES;

	//UInt*       m_puiSubstreamSizes;
	//TComScalingList*     m_scalingList;                 //!< pointer of quantization matrix
	Bool        m_cabacInitFlag; 

	Bool       m_bLMvdL1Zero;
	Int         m_numEntryPointOffsets;
	Bool       m_temporalLayerNonReferenceFlag;
	Bool       m_LFCrossSliceBoundaryFlag;

	Bool       m_enableTMVPFlag;

}SliceHeader;

typedef struct _SaoLcuParam
{
	Bool       mergeUpFlag;
	Bool       mergeLeftFlag;
	Int        typeIdx;
	Int        subTypeIdx;                  ///< indicates EO class or BO band position
	Int        offset[4];
	Int        partIdx;
	Int        partIdxTmp;
	Int        length;
} SaoLcuParam;

typedef struct _SAOParam
{
	Bool       bSaoFlag[2];
	//SAOQTPart* psSaoPart[3];
	Int        iMaxSplitLevel;
	//Bool         oneUnitFlag[3];
	SaoLcuParam* saoLcuParam[3];
}SAOParam;


/////////////////////////////////////////////////////////////////////////////////////////////////
//空间层解码CTX
/////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct HEVCContext{

	struct LentCodecContext *ctx;	//主解码CTX
	int initialized;				//当前CTX的初始化情况
	LentPacket *pkt;

	void *thread_opaque;			//thread信息

    int width, height;				//本层帧输出大小

	int cu_width, cu_height;		//以64x64计算的帧大小
    int cu_stride;					//b64x64
    int b32_stride;					//b32x32
    int b16_stride;					//b16x16
    int b8_stride;					//b8x8
    int b4_stride;					//b4x4
    int cu_num;						//以64x64计算的CU数量,==cu_width*cu_height
    int linesize;					//padding并对齐后的帧Y宽度
    int uvlinesize;					//padding并对齐后的帧UV宽度


	//frame management
    int (*get_buffer)(struct HEVCContext *c, LentFrame *pic);
    void (*release_buffer)(struct HEVCContext *c, LentFrame *pic);
	void (*free_buffer)(struct HEVCContext *c);
    int internal_buffer_count;
    void *internal_buffer;

    Picture *picture;				//帧结构缓冲
    int picture_count;				//帧结构缓冲数量
    Picture current_picture;		//当前帧cache
    Picture *current_picture_ptr;	//缓冲的当前帧结构指针
	int coded_picture_number;		//当前帧编码序号

	//各种CTX
    DSPContext dsp;				//DSP相关CTX
	CABACContext cabac;				//CABAC相关CTX
	BitstreamContext bs;			//码流读取相关CTX
    HEVCDeblockContext deblock;				//Deblock相关CTX
	HEVCPredContext pred;			//intra-pred CTX

	char *rbsp_buffer;				//码流缓冲
	int rbsp_buffer_size;			//码流缓冲大小

	//Parameter sets
	VPS vps;
	SPS sps;						//当前sps
	PPS pps;						//当前pps
	VPS *vps_buffers[MAX_VPS_COUNT];
    SPS *sps_buffers[MAX_SPS_COUNT];//SPS缓冲
    PPS *pps_buffers[MAX_PPS_COUNT];//PPS缓冲

	//refs
    Picture *default_ref_buffer[MAX_DPB_SIZE];
	int dpb_count;
	Picture ref_list[3][MAX_REF];
	char ref_list2_to_list01[MAX_REF],ref_list2_to_list01_idx[MAX_REF];


	//outputting
    Picture *delayed_pic[MAX_DELAYED_PIC_COUNT+2];	//输出队列
    Picture *next_output_pic;						//输出帧
    int next_outputed_poc;							//
	int delay_length;



    /* current value */
	int nal_unit_type;				//NAL_TYPE
	int nal_ref_idc;				//
	int tid;						//temporal id
	int did,qid;					//
    int cu_x, cu_y, cu_xy, cu_index;//当前CU
	int dropable;					//nonref_B
	//int luma_qscale;				//Luma QP
    //int chroma_qscale;				//Chroma QP
	int i_neighbor;
	int i_ref_count[3];
	SliceHeader sh;


//flags:
#define SKIP_FLAG 1
#define RESIDUAL_FLAG 2
#define DEBLOCK_CBF_TL 16
#define DEBLOCK_CBF_TR 32
#define DEBLOCK_CBF_DL 64
#define DEBLOCK_CBF_DR 128
#define DEBLOCK_CBF_ALL (DEBLOCK_CBF_TL|DEBLOCK_CBF_TR|DEBLOCK_CBF_DL|DEBLOCK_CBF_DR)
//end flag

#define MIN_UNIT_SIZE 4
#define UNIT_MAP_STRIDE 25
#define MAX_SQR_FILT_LENGTH 29
#define NO_VAR_BINS 16

	struct
	{

		//应该初始化为-1的
		DECLARE_ALIGNED(16, int8_t,		cu_depth		)[LENT_SCAN_LUMA_SIZE];
// 		DECLARE_ALIGNED(16, int8_t,		qp				)[LENT_SCAN_LUMA_SIZE];
// 		DECLARE_ALIGNED(16, int8_t,		pred_mode		)[LENT_SCAN_LUMA_SIZE];
		DECLARE_ALIGNED(16, int8_t,		intra_pred_mode	)[LENT_SCAN_SIZE*4];
		DECLARE_ALIGNED(16, int8_t,		ref				)[2][LENT_SCAN_SIZE*4];
		DECLARE_ALIGNED(16,	int32_t,	part_mode		)[CU_TOTAL_PART];


		//应该初始化为0的
		DECLARE_ALIGNED(16, int8_t,		cu_flag			)[LENT_SCAN_SIZE];//cu级flag统一使用该数组
// 		DECLARE_ALIGNED(16, int16_t,	mv				)[2][LENT_SCAN_SIZE*4][2];
// 		DECLARE_ALIGNED(16,	int8_t,		b_split_trans	)[CU_TOTAL_PART];
		DECLARE_ALIGNED(16, int8_t,		i_cbp			)[CU_TOTAL_PART]; //0: luma  1-2: chroma
		DECLARE_ALIGNED(16,	int8_t,		unit_exist_map	)[32*25];

		//不需要初始化的
		DECLARE_ALIGNED(16, int8_t,		qp				)[LENT_SCAN_LUMA_SIZE];
		DECLARE_ALIGNED(16, int8_t,		pred_mode		)[LENT_SCAN_LUMA_SIZE];
		DECLARE_ALIGNED(16, int16_t,	mv				)[2][LENT_SCAN_SIZE*4][2];
		DECLARE_ALIGNED(16,	int8_t,		b_split_trans	)[CU_TOTAL_PART];

		int i_dqp_decoded; //应该初始化为0的

		DECLARE_ALIGNED(16, int16_t,	hpel			)[2][3][HPEL_SIZE*HPEL_SIZE];
		DECLARE_ALIGNED(16, dctcoef,	luma_out		)[4096];//当前CU系数表
		DECLARE_ALIGNED(16, dctcoef,	chroma_out		)[2][1024];//当前CU系数表
		int alfcoeff[NO_VAR_BINS][MAX_SQR_FILT_LENGTH];
		int alfcoeff_final[NO_VAR_BINS][MAX_SQR_FILT_LENGTH];
		Picture alf_pic;
		//dctcoef *luma_out;
		//dctcoef *chroma_out[2];
		pixel *p_fdec[3];
		int i_last_qp;
		int b_merge;
	} cache;

	struct//当前帧的主要信息缓冲
	{

/////////*边界缓冲																	*/

		//CU_DEPTH & CU_FLAG
		//*******0
		//*******1
		//*******2
		//*******3
		//*******4
		//*******5
		//*******6
		//89012345(7)
		int8_t	(*cu_depth_edge)[16],(*cu_depth_edge_base)[16];	//使用[cu_xy][0-15]访问
		int8_t	(*cu_flag_edge)[16],(*cu_flag_edge_base)[16];	//使用[cu_xy][0-15]访问
		//predmode
		int8_t	(*predmode_edge)[16],(*predmode_edge_base)[16];		//使用[cu_xy][0-15]访问

/////////*边界缓冲end																*/

/////////*全图缓冲																	*/

		//全图cu_depth
		int8_t	(*cu_depth)[256];						//使用[cu_index][idx]访问
		//全图qp
		int8_t	(*qp)[8];								//使用[cu_index][idx]访问
		//全图split_trans flag

		int8_t	*intra_pred_mode,*intra_pred_mode_base;	//使用[cu_xy*384+idx]访问
		//系数
		//dctcoef (*luma)[4096],(*chroma[2])[1024];						//使用cu_index访问//todo:是否应该把系数放在一起方便缓存？

		//全图未滤波像素值
		//pixel *luma_nonfiltered;
		int8_t (*deblock_strength)[2][8][16];			//使用[cu_index]访问
		int8_t (*deblock_qp)[8][8];						//使用[cu_index]访问
/////////*全图缓冲end																*/

		SAOParam sao;
	} current;

	//cabac
	int16_t *scan_order[4][4][4];//[log2_width-2][log2_height-2][scan_idx]
	int16_t *scan_order_inv[4][4][4];//optimize calculation of n from x&y in cabac


	//recon
	uint8_t *intra_border[3];

	//temp data

	uint8_t *temp_buffer;

	//for compatibility
	int g_bitDepthY;
	int g_bitDepthC;
	//int g_maxLumaVal;
	int g_uiMaxCUWidth;
	int g_uiMaxCUHeight;
	int g_uiAddCUDepth;
	int g_uiMaxCUDepth;


	//machine related
	unsigned int machine;
}HEVCContext;


/////////////////////////////////////////////////////////////////////////////////////////////////
//报错退出
/////////////////////////////////////////////////////////////////////////////////////////////////
#define errorinfo(h,...) do{lent_log(h,LENT_LOG_ERROR,__VA_ARGS__);abort();}while(0)




/////////////////////////////////////////////////////////////////////////////////////////////////
//函数接口
/////////////////////////////////////////////////////////////////////////////////////////////////
int		decode_frame					(HEVCContext *h, void *data, int *data_size);
void	hevc_flush_dpb					(HEVCContext *h);
void	hevc_uninitialize				(HEVCContext *s, int free_pics);
int		hevc_initialize					(HEVCContext *s,int width,int height);





#endif