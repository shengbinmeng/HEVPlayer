//**********************************************
//hevc.c
//Unipipy @2011
//main file of decoder core
//**********************************************

#include "hevc.h"
#include "hevc_frame.h"
#include "hevc_pred.h"
#include "hevc_cabac.h"
#include "hevc_ref.h"
#include "hevc_deblock.h"
#include "hevc_thread.h"
#include "hevc_sao.h"
#include "hevc_mvpred.h"
#include "hevc_compatibility.h"

#if ARCH_X86_32
#include <tmmintrin.h>
#endif


static int get_consumed_bytes(HEVCContext *h, int pos, int buf_size){
	if(pos==0) pos=1; //avoid infinite loops (i doubt that is needed but ...)
	if(pos+10>buf_size) pos=buf_size; // oops ;)

	return pos;
}

static const uint8_t *extract_nal(HEVCContext *h, const uint8_t *src, int *dst_length, int *consumed, int length){
	int i, si, di;
	uint8_t *dst;
	int bufidx;
	assert((src[0]&0x80) == 0);   //forbidden bit, must be zero
	h->nal_unit_type = (src[0]&0x7F)>>1;
	h->nal_ref_idc = h->nal_unit_type!=NAL_UNIT_CODED_SLICE_TRAIL_N;
	src++; length--;
	
	h->tid=src[0]-1;
	src++; length--;

#if 0
	for(i=0; i<length; i++)
		printf("%2X ", src[i]);
#endif

#if HAVE_FAST_UNALIGNED
# if HAVE_FAST_64BIT
#   define RS 7
	for(i=0; i+1<length; i+=9){
		if(!((~AV_RN64A(src+i) & (AV_RN64A(src+i) - 0x0100010001000101ULL)) & 0x8000800080008080ULL))
# else
#   define RS 3
	for(i=0; i+1<length; i+=5){
		if(!((~AV_RN32A(src+i) & (AV_RN32A(src+i) - 0x01000101U)) & 0x80008080U))
# endif
			continue;
		if(i>0 && !src[i]) i--;
		while(src[i]) i++;
#else
#define RS 0
	for(i=0; i+1<length; i+=2){
		if(src[i]) continue;
		if(i>0 && src[i-1]==0) i--;
#endif
		if(i+2<length && src[i+1]==0 && src[i+2]<=3){
			if(src[i+2]!=3){
				/* startcode, so we must be past the end */
				length=i;
			}
			break;
		}
		i-= RS;
	}

	if(i>=length-1){ //no escaped 0
		*dst_length= length;
		*consumed= length+1; //+1 for the header
		return src;
	}

	bufidx = 0;

	lent_fast_malloc(&h->rbsp_buffer, &h->rbsp_buffer_size, length+lent_INPUT_BUFFER_PADDING_SIZE);
	dst= h->rbsp_buffer;

	if (dst == NULL){
		return NULL;
	}

	//printf("decoding esc\n");
	memcpy(dst, src, i);
	si=di=i;
	while(si+2<length){
		//remove escapes (very rare 1:2^22)
		if(src[si+2]>3){
			dst[di++]= src[si++];
			dst[di++]= src[si++];
		}else if(src[si]==0 && src[si+1]==0){
			if(src[si+2]==3){ //escape
				dst[di++]= 0;
				dst[di++]= 0;
				si+=3;
				continue;
			}else //next start code
				goto nsc;
		}

		dst[di++]= src[si++];
	}
	while(si<length)
		dst[di++]= src[si++];
nsc:

	memset(dst+di, 0, lent_INPUT_BUFFER_PADDING_SIZE);

	*dst_length= di;
	*consumed= si + 1;//+1 for the header

	return dst;
}
static lent_always_inline int decode_rbsp_trailing(HEVCContext *h,uint8_t *src)
{
	int v= *src;
	int r;

	for(r=1; r<9; r++){
		if(v&1) return r;
		v>>=1;
	}
	return 0;
}


static lent_always_inline void sortDeltaPOC(RPS* rps)
{
	Int j, deltaPOC, used, k, temp, numNegPics;
  for(j=1; j < rps->m_numberOfPictures; j++)
  { 
	deltaPOC = rps->m_deltaPOC[j];
	used = rps->m_used[j];
	for (k=j-1; k >= 0; k--)
	{
	  temp = rps->m_deltaPOC[k];
	  if (deltaPOC < temp)
	  {
		rps->m_deltaPOC[k+1] = temp;
		rps->m_used[k+1] = rps->m_used[k];
		rps->m_deltaPOC[k] = deltaPOC;
		rps->m_used[k] = used;
	  }
	}
  }
  // flip the negative values to largest first
  numNegPics = rps->m_numberOfNegativePictures;
  for(j=0, k=numNegPics-1; j < numNegPics>>1; j++, k--)
  { 
	deltaPOC = rps->m_deltaPOC[j];
	used = rps->m_used[j];
	rps->m_deltaPOC[j] = rps->m_deltaPOC[k];
	rps->m_used[j] = rps->m_used[k];
	rps->m_deltaPOC[k] = deltaPOC;
	rps->m_used[k] = used;
  }
}
static lent_always_inline void parseShortTermRefPicSet (HEVCContext *h, SPS *sps, RPS *rps, int idx)
{
	UInt code;
	UInt interRPSPred;
	if (idx > 0)
	{
		rps->m_interRPSPrediction						= READ_FLAG(     interRPSPred, "inter_ref_pic_set_prediction_flag");
	}
	else
	{
		interRPSPred									= 0;
		rps->m_interRPSPrediction						= 0;
	}

	if (interRPSPred) 
	{
		UInt bit;
		Int rIdx;
		RPS* rpsRef;
		Int j, k = 0, k0 = 0, k1 = 0;
		Int deltaRPS; // delta_RPS
		Int numNegPics;
		if(idx == sps->m_RPSNum)
		{
														  READ_UVLC(     code, "delta_idx_minus1" ); // delta index of the Reference Picture Set used for prediction minus 1
		}
		else
		{
			code = 0;
		}
		assert(code <= idx-1); // delta_idx_minus1 shall not be larger than idx-1, otherwise we will predict from a negative row position that does not exist. When idx equals 0 there is no legal value and interRPSPred must be zero. See J0185-r2
		rIdx =  idx - 1 - code;
		assert (rIdx <= idx-1 && rIdx >= 0); // Made assert tighter; if rIdx = idx then prediction is done from itself. rIdx must belong to range 0, idx-1, inclusive, see J0185-r2
		rpsRef = &sps->m_RPSList[rIdx];
														  READ_CODE(  1, bit, "delta_rps_sign"); // delta_RPS_sign
														  READ_UVLC(     code, "abs_delta_rps_minus1");  // absolute delta RPS minus 1
		deltaRPS = (1 - (bit<<1)) * (code + 1); // delta_RPS
		for(j=0 ; j <= rpsRef->m_numberOfPictures; j++)
		{
			Int refIdc									= READ_CODE(  1, bit, "used_by_curr_pic_flag" ); //first bit is "1" if Idc is 1 
			if (refIdc == 0) 
			{
														  READ_CODE(  1, bit, "use_delta_flag" ); //second bit is "1" if Idc is 2, "0" otherwise.
				refIdc = bit<<1; //second bit is "1" if refIdc is 2, "0" if refIdc = 0.
			}
			if (refIdc == 1 || refIdc == 2)
			{
				Int deltaPOC = deltaRPS + ((j < rpsRef->m_numberOfPictures)? rpsRef->m_deltaPOC[j] : 0);
				rps->m_deltaPOC[k]=deltaPOC;
				rps->m_used[k]= (refIdc == 1);

				if (deltaPOC < 0) {
					k0++;
				}
				else 
				{
					k1++;
				}
				k++;
			}  
			rps->m_refIdc[j]=refIdc;
		}
		rps->m_numRefIdc=rpsRef->m_numberOfPictures+1;
		rps->m_numberOfPictures=(k);
		rps->m_numberOfNegativePictures=(k0);
		rps->m_numberOfPositivePictures=(k1);
		

		//rps->sortDeltaPOC();
		for(j=1; j < rps->m_numberOfPictures; j++)
		{ 
			Int deltaPOC = rps->m_deltaPOC[j];
			Bool used = rps->m_used[j];
			for (k=j-1; k >= 0; k--)
			{
				Int temp = rps->m_deltaPOC[k];
				if (deltaPOC < temp)
				{
					rps->m_deltaPOC[k+1]	=temp;
					rps->m_used[k+1]		=rps->m_used[k];
					rps->m_deltaPOC[k]		=deltaPOC;
					rps->m_used[k]			=used;
				}
			}
		}
		// flip the negative values to largest first
		numNegPics = rps->m_numberOfNegativePictures;
		for(j=0, k=numNegPics-1; j < numNegPics>>1; j++, k--)
		{ 
			Int deltaPOC = rps->m_deltaPOC[j];
			Bool used = rps->m_used[j];
			rps->m_deltaPOC[j]				=rps->m_deltaPOC[k];
			rps->m_used[j]					=rps->m_used[k];
			rps->m_deltaPOC[k]				=deltaPOC;
			rps->m_used[k]					=used;
		}
	}
	else
	{
		Int prev = 0;
		Int poc;
		Int j;
		rps->m_numberOfNegativePictures					= READ_UVLC(     code, "num_negative_pics");
		rps->m_numberOfPositivePictures					= READ_UVLC(     code, "num_positive_pics");
		for(j=0 ; j < rps->m_numberOfNegativePictures; j++)
		{
														  READ_UVLC(     code, "delta_poc_s0_minus1");
			poc = prev-code-1;
			prev = poc;
			rps->m_deltaPOC[j]=poc;
			rps->m_used[j]								= READ_FLAG(     code, "used_by_curr_pic_s0_flag");
		}
		prev = 0;
		for(j=rps->m_numberOfNegativePictures; j < rps->m_numberOfNegativePictures+rps->m_numberOfPositivePictures; j++)
		{
														  READ_UVLC(code, "delta_poc_s1_minus1");
			poc = prev+code+1;
			prev = poc;
			rps->m_deltaPOC[j]=poc;
			rps->m_used[j]								= READ_FLAG(     code, "used_by_curr_pic_s1_flag");
		}
		rps->m_numberOfPictures=(rps->m_numberOfNegativePictures+rps->m_numberOfPositivePictures);
	}

}
static lent_always_inline void decode_sps_ptl_profileTier(HEVCContext *h, ProfileTierLevel *ptl)
{
	UInt uiCode;
	Int j;
	ptl->m_profileSpace									= READ_CODE( 2 , uiCode, "XXX_profile_space[]");
	ptl->m_tierFlag										= READ_FLAG( uiCode, "XXX_tier_flag[]"    );
	ptl->m_profileIdc									= READ_CODE( 5 , uiCode, "XXX_profile_idc[]"  );

	for(j = 0; j < 32; j++)
	{
		ptl->m_profileCompatibilityFlag[j]				= READ_FLAG(  uiCode, "XXX_profile_compatibility_flag[][j]");
	}
														  READ_CODE( 16, uiCode, "XXX_reserved_zero_16bits[]");assert( uiCode == 0 );  
}
int decode_sps_ptl(HEVCContext *h, PTL *rpcPTL, Bool profilePresentFlag, Int maxNumSubLayersMinus1 )
{
	UInt uiCode;
	Int i;
	if(profilePresentFlag)
	{
		decode_sps_ptl_profileTier(h, &rpcPTL->m_generalPTL);
	}
	rpcPTL->m_generalPTL.m_levelIdc						= READ_CODE( 8, uiCode, "general_level_idc" );

	for(i = 0; i < maxNumSubLayersMinus1; i++)
	{
		rpcPTL->m_subLayerProfilePresentFlag[i]			= READ_FLAG( uiCode, "sub_layer_profile_present_flag[i]" );
		rpcPTL->m_subLayerLevelPresentFlag[i]			= READ_FLAG( uiCode, "sub_layer_level_present_flag[i]"   ); 

		if( profilePresentFlag && rpcPTL->m_subLayerProfilePresentFlag[i] )
		{
			decode_sps_ptl_profileTier(h, &rpcPTL->m_subLayerPTL[i]);
		}
		if(rpcPTL->m_subLayerLevelPresentFlag[i])
		{
			rpcPTL->m_subLayerPTL[i].m_levelIdc			= READ_CODE( 8, uiCode, "sub_layer_level_idc[i]" );
		}
	}
	return 0;
}
static lent_always_inline void *decode_sps_scalingList_getDefault(HEVCContext *h, UInt sizeId, UInt listId)
{
	const Int *src = 0;
	const static Int g_quantTSDefault4x4[16] =
	{
		16,16,16,16,
		16,16,16,16,
		16,16,16,16,
		16,16,16,16
	};

	const static Int g_quantIntraDefault8x8[64] =
	{
		16,16,16,16,17,18,21,24,
		16,16,16,16,17,19,22,25,
		16,16,17,18,20,22,25,29,
		16,16,18,21,24,27,31,36,
		17,17,20,24,30,35,41,47,
		18,19,22,27,35,44,54,65,
		21,22,25,31,41,54,70,88,
		24,25,29,36,47,65,88,115
	};

	const static Int g_quantInterDefault8x8[64] =
	{
		16,16,16,16,17,18,20,24,
		16,16,16,17,18,20,24,25,
		16,16,17,18,20,24,25,28,
		16,17,18,20,24,25,28,33,
		17,18,20,24,25,28,33,41,
		18,20,24,25,28,33,41,54,
		20,24,25,28,33,41,54,71,
		24,25,28,33,41,54,71,91
	};
	switch(sizeId)
	{
	case SCALING_LIST_4x4:
		src = g_quantTSDefault4x4;
		break;
	case SCALING_LIST_8x8:
		src = (listId<3) ? g_quantIntraDefault8x8 : g_quantInterDefault8x8;
		break;
	case SCALING_LIST_16x16:
		src = (listId<3) ? g_quantIntraDefault8x8 : g_quantInterDefault8x8;
		break;
	case SCALING_LIST_32x32:
		src = (listId<1) ? g_quantIntraDefault8x8 : g_quantInterDefault8x8;
		break;
	default:
		assert(0);
		src = NULL;
		break;
	}
	return (void*)src;
}
static lent_always_inline void decode_sps_scalingList(HEVCContext *h, ScalingList *scalingList)
{
	UInt  code, sizeId, listId, refListId;
	Bool scalingListPredModeFlag;

	UInt g_scalingListNum[SCALING_LIST_SIZE_NUM]={6,6,6,2};
	UInt g_scalingListSize   [4] = {16,64,256,1024}; 
	//for each size
	for(sizeId = 0; sizeId < SCALING_LIST_SIZE_NUM; sizeId++)
	{
		for(listId = 0; listId <  g_scalingListNum[sizeId]; listId++)
		{
			scalingListPredModeFlag						= READ_FLAG( code, "scaling_list_pred_mode_flag");

			if(!scalingListPredModeFlag) //Copy Mode
			{
														  READ_UVLC( code, "scaling_list_pred_matrix_id_delta");
				scalingList->m_refMatrixId[sizeId][listId]=(Int)listId-code;
				if( sizeId > SCALING_LIST_8x8 )
				{
					scalingList->m_scalingListDC[sizeId][listId]=(listId == scalingList->m_refMatrixId[sizeId][listId]? 16 :scalingList->m_scalingListDC[sizeId][scalingList->m_refMatrixId[sizeId][listId]]);
				}
				refListId=scalingList->m_refMatrixId[sizeId][listId];
				memcpy(scalingList->m_scalingListCoef[sizeId][listId],((listId == refListId)? decode_sps_scalingList_getDefault(h, sizeId, refListId): scalingList->m_scalingListCoef[sizeId][refListId]),sizeof(Int)*LENTMIN(MAX_MATRIX_COEF_NUM,(Int)g_scalingListSize[sizeId]));

			}
			else //DPCM Mode
			{
				assert(0);//FIXME pipy
				/*
				Int i,coefNum = LENTMIN(MAX_MATRIX_COEF_NUM,(Int)g_scalingListSize[sizeId]);
				Int data;
				Int scalingListDcCoefMinus8 = 0;
				Int nextCoef = SCALING_LIST_START_VALUE;
				UInt* scan  = (sizeId == 0) ? h->scan_order[ DEPTH2LOG2(1)-2 ] [ SCAN_DIAG ]  :  g_sigLastScanCG32x32L;
				Int *dst = scalingList->m_scalingListCoef[sizeId][listId];


				if( sizeId > SCALING_LIST_8x8 )
				{
					READ_SVLC( scalingListDcCoefMinus8, "scaling_list_dc_coef_minus8");
					scalingList->m_scalingListDC[sizeId][listId]=scalingListDcCoefMinus8 + 8;
					nextCoef = scalingList->m_scalingListDC[sizeId][listId];
				}

				for(i = 0; i < coefNum; i++)
				{
					READ_SVLC( data, "scaling_list_delta_coef");
					nextCoef = (nextCoef + data + 256 ) % 256;
					dst[scan[i]] = nextCoef;
				}
			*/
			}
		}
	}

}
static lent_always_inline int decode_sps(HEVCContext *h)
{
	int i = 0;
	UInt sps_id,vps_id,max_tlayers;
	UInt  uiCode;
	UInt subLayerOrderingInfoPresentFlag;
	UInt bTemporalIdNestingFlag;

	SPS *pcSPS;
	PTL ptl;
	RPS* rpsList;
	
	const Int m_cropUnitX[]={1,2,2,1};
	const Int m_cropUnitY[]={1,2,1,1};

	//--------------------------------------------------header--------------------------------------------------
	vps_id												= READ_CODE( 4,  uiCode, "video_parameter_set_id");
	max_tlayers											= READ_CODE( 3,  uiCode, "sps_max_sub_layers_minus1" ) + 1;
	
	bTemporalIdNestingFlag								= READ_FLAG( uiCode, "sps_temporal_id_nesting_flag" );
	//													  READ_FLAG(     uiCode, "sps_reserved_zero_bit");assert(0==uiCode);
	decode_sps_ptl(h, &ptl, 1, max_tlayers - 1);

	sps_id												= READ_UVLC(     uiCode, "seq_parameter_set_id" );
	if(h->sps_buffers[sps_id]==NULL)
	{
		h->sps_buffers[sps_id]=(SPS*)lent_mallocz(sizeof(SPS));
	}
	pcSPS=h->sps_buffers[sps_id];
	pcSPS->m_SPSId = sps_id;
	pcSPS->m_VPSId = vps_id;
	pcSPS->m_uiMaxTLayers = max_tlayers;
	pcSPS->m_pcPTL = ptl;
	pcSPS->m_bTemporalIdNestingFlag	= bTemporalIdNestingFlag;
	
	//--------------------------------------------------content--------------------------------------------------
	pcSPS->m_chromaFormatIdc							= READ_UVLC(     uiCode, "chroma_format_idc" );

	assert(uiCode == 1);
	if( uiCode == 3 )
	{
														  READ_FLAG(     uiCode, "separate_colour_plane_flag");
		assert(uiCode == 0);
	}

	pcSPS->m_picWidthInLumaSamples						= READ_UVLC(     uiCode, "pic_width_in_luma_samples" );
	pcSPS->m_picHeightInLumaSamples						= READ_UVLC(     uiCode, "pic_height_in_luma_samples" );
														  READ_FLAG(     uiCode, "pic_cropping_flag");
	if (uiCode != 0)
	{
		CroppingWindow *crop = &pcSPS->m_picCroppingWindow;
		crop->m_picCroppingFlag							= 1;
		crop->m_picCropLeftOffset						= READ_UVLC(     uiCode, "pic_crop_left_offset" ) * m_cropUnitX[pcSPS->m_chromaFormatIdc];
		crop->m_picCropRightOffset						= READ_UVLC(     uiCode, "pic_crop_right_offset" ) * m_cropUnitX[pcSPS->m_chromaFormatIdc];
		crop->m_picCropTopOffset						= READ_UVLC(     uiCode, "pic_crop_top_offset" ) * m_cropUnitY[pcSPS->m_chromaFormatIdc];
		crop->m_picCropBottomOffset						= READ_UVLC(     uiCode, "pic_crop_bottom_offset" ) * m_cropUnitY[pcSPS->m_chromaFormatIdc];
	}

	pcSPS->m_uiBitDepthY								= READ_UVLC(     uiCode, "bit_depth_luma_minus8" ) + 8;
	pcSPS->m_qpBDOffsetY								= 6 * uiCode;
	h->g_bitDepthY = 8 + uiCode;
	//h->g_maxLumaVal = (1<<h->g_bitDepth) - 1;
	
	pcSPS->m_uiBitDepthC								= READ_UVLC(     uiCode, "bit_depth_chroma_minus8" ) + 8;
	pcSPS->m_qpBDOffsetC								= 6 * uiCode;
	h->g_bitDepthC = 8 + uiCode;

	pcSPS->m_uiBitsForPOC								= READ_UVLC(     uiCode, "log2_max_pic_order_cnt_lsb_minus4" ) + 4;

	subLayerOrderingInfoPresentFlag						= READ_FLAG(     uiCode, "sps_sub_layer_ordering_info_present_flag");

	for(i=0; i < pcSPS->m_uiMaxTLayers; i++)
	{
		pcSPS->m_uiMaxDecPicBuffering[i]				= READ_UVLC(     uiCode, "max_dec_pic_buffering");
		pcSPS->m_numReorderPics[i]						= READ_UVLC(     uiCode, "num_reorder_pics" );
		pcSPS->m_uiMaxLatencyIncrease[i]				= READ_UVLC(     uiCode, "max_latency_increase");
		if(!subLayerOrderingInfoPresentFlag)
		{
			for (i++; i < pcSPS->m_uiMaxTLayers; i++)
			{
				pcSPS->m_uiMaxDecPicBuffering[i]		= pcSPS->m_uiMaxDecPicBuffering[i];
				pcSPS->m_numReorderPics[i]				= pcSPS->m_numReorderPics[i];
				pcSPS->m_uiMaxLatencyIncrease[i]		= pcSPS->m_uiMaxLatencyIncrease[i];
			}
			break;
		}
	}

	//pcSPS->m_restrictedRefPicListsFlag					= READ_FLAG(     uiCode, "restricted_ref_pic_lists_flag" );
	//if( pcSPS->m_restrictedRefPicListsFlag )
	//{
	//	pcSPS->m_listsModificationPresentFlag			= READ_FLAG(     uiCode, "lists_modification_present_flag" );
	//}
	//else 
	//{
	//	pcSPS->m_listsModificationPresentFlag			= 1;
	//}

	{
		UInt log2MinCUSize								= READ_UVLC(     uiCode, "log2_min_coding_block_size_minus3" ) + 3;
		UInt uiMaxCUDepthCorrect						= READ_UVLC(     uiCode, "log2_diff_max_min_coding_block_size" );

		pcSPS->m_uiMaxCUWidth  = ( 1<<(log2MinCUSize + uiMaxCUDepthCorrect) ); h->g_uiMaxCUWidth  = 1<<(log2MinCUSize + uiMaxCUDepthCorrect);
		pcSPS->m_uiMaxCUHeight = ( 1<<(log2MinCUSize + uiMaxCUDepthCorrect) ); h->g_uiMaxCUHeight = 1<<(log2MinCUSize + uiMaxCUDepthCorrect);

		pcSPS->m_uiQuadtreeTULog2MinSize				= READ_UVLC(     uiCode, "log2_min_transform_block_size_minus2" ) + 2;
		pcSPS->m_uiQuadtreeTULog2MaxSize				= READ_UVLC(     uiCode, "log2_diff_max_min_transform_block_size" ) + pcSPS->m_uiQuadtreeTULog2MinSize;
		pcSPS->m_uiMaxTrSize							= 1<<pcSPS->m_uiQuadtreeTULog2MaxSize ;

		pcSPS->m_uiQuadtreeTUMaxDepthInter				= READ_UVLC(     uiCode, "max_transform_hierarchy_depth_inter" ) + 1;
		pcSPS->m_uiQuadtreeTUMaxDepthIntra				= READ_UVLC(     uiCode, "max_transform_hierarchy_depth_intra" ) + 1;

		pcSPS->m_uiCULog2MinSize						= log2MinCUSize;
		h->g_uiAddCUDepth = 0;
		while( ( pcSPS->m_uiMaxCUWidth >> uiMaxCUDepthCorrect ) > ( 1 << ( pcSPS->m_uiQuadtreeTULog2MinSize + h->g_uiAddCUDepth )  ) )
		{
			h->g_uiAddCUDepth++;
		}

		pcSPS->m_uiMaxCUDepth = uiMaxCUDepthCorrect+h->g_uiAddCUDepth; 

		h->g_uiMaxCUDepth  = uiMaxCUDepthCorrect+h->g_uiAddCUDepth;
	}
	// BB: these parameters may be removed completly and replaced by the fixed values
	pcSPS->m_uiMinTrDepth = 0;
	pcSPS->m_uiMaxTrDepth = 1;
	pcSPS->m_scalingListEnabledFlag						= READ_FLAG(     uiCode, "scaling_list_enabled_flag" );
	if(pcSPS->m_scalingListEnabledFlag)
	{
		pcSPS->m_scalingListPresentFlag					= READ_FLAG(     uiCode, "sps_scaling_list_data_present_flag" );
		if(pcSPS->m_scalingListPresentFlag)
		{
			decode_sps_scalingList(h, &pcSPS->m_scalingList );
		}
	}
	pcSPS->m_useAMP										= READ_FLAG(     uiCode, "asymmetric_motion_partitions_enabled_flag" );
	pcSPS->m_bUseSAO									= READ_FLAG(     uiCode, "sample_adaptive_offset_enabled_flag" );



	pcSPS->m_usePCM										= READ_FLAG(     uiCode, "pcm_enabled_flag" );
	if( pcSPS->m_usePCM )
	{
		pcSPS->m_uiPCMBitDepthLuma						= READ_CODE(  4, uiCode, "pcm_bit_depth_luma_minus1" ) + 1;
		pcSPS->m_uiPCMBitDepthChroma					= READ_CODE(  4, uiCode, "pcm_bit_depth_chroma_minus1" ) + 1;
		pcSPS->m_uiPCMLog2MinSize						= READ_UVLC(     uiCode, "log2_min_pcm_coding_block_size_minus3" ) + 3;
		pcSPS->m_pcmLog2MaxSize							= READ_UVLC(     uiCode, "log2_diff_max_min_pcm_coding_block_size" ) + pcSPS->m_uiPCMLog2MinSize;
		pcSPS->m_bPCMFilterDisableFlag					= READ_FLAG(     uiCode, "pcm_loop_filter_disable_flag" );
	}

	pcSPS->m_RPSNum										= READ_UVLC(     uiCode, "num_short_term_ref_pic_sets" );
	//pcSPS->createRPSList(uiCode);
	lent_freep(&pcSPS->m_RPSList);
	pcSPS->m_RPSList=(RPS*)lent_malloc(sizeof(RPS)*pcSPS->m_RPSNum);//todo free the memories

	rpsList = pcSPS->m_RPSList;

	for(i=0; i< pcSPS->m_RPSNum; i++)
		parseShortTermRefPicSet(h,pcSPS,&rpsList[i],i);

	pcSPS->m_bLongTermRefsPresent						= READ_FLAG(     uiCode, "long_term_ref_pics_present_flag" );
	if (pcSPS->m_bLongTermRefsPresent) 
	{
		UInt k;
		pcSPS->m_numLongTermRefPicSPS					= READ_UVLC(     uiCode, "num_long_term_ref_pic_sps" );
		for (k = 0; k < pcSPS->m_numLongTermRefPicSPS; k++)
		{
			pcSPS->m_ltRefPicPocLsbSps[k]				= READ_CODE( pcSPS->m_uiBitsForPOC, uiCode, "lt_ref_pic_poc_lsb_sps" );
			pcSPS->m_usedByCurrPicLtSPSFlag[k]			= READ_FLAG(     uiCode,  "used_by_curr_pic_lt_sps_flag[i]");
		}
	}
	pcSPS->m_TMVPFlagsPresent							= READ_FLAG(     uiCode, "sps_temporal_mvp_enable_flag" );

//#if STRONG_INTRA_SMOOTHING
	pcSPS->m_useStrongIntraSmoothing					= READ_FLAG(     uiCode, "sps_strong_intra_smoothing_enable_flag" );
//#endif

	pcSPS->m_vuiParametersPresentFlag					= READ_FLAG(     uiCode, "vui_parameters_present_flag" );

	if (pcSPS->m_vuiParametersPresentFlag)
	{
		VUI *pcVUI = &pcSPS->m_vuiParameters;

		pcVUI->m_aspectRatioInfoPresentFlag				= READ_FLAG(     uiCode, "aspect_ratio_info_present_flag");
		if (uiCode)
		{
			pcVUI->m_aspectRatioIdc						= READ_CODE(  8, uiCode, "aspect_ratio_idc");
			if (uiCode == 255)
			{
				pcVUI->m_sarWidth						= READ_CODE( 16, uiCode, "sar_width");
				pcVUI->m_sarHeight						= READ_CODE( 16, uiCode, "sar_height");
			}
		}

		pcVUI->m_overscanInfoPresentFlag				= READ_FLAG(     uiCode, "overscan_info_present_flag");
		if (uiCode)
		{
			pcVUI->m_overscanAppropriateFlag			= READ_FLAG(     uiCode, "overscan_appropriate_flag");                ;
		}

		pcVUI->m_videoSignalTypePresentFlag				= READ_FLAG(     uiCode, "video_signal_type_present_flag");
		if (uiCode)
		{
			pcVUI->m_videoFormat						= READ_CODE(  3, uiCode, "video_format");
			pcVUI->m_videoFullRangeFlag					= READ_FLAG(     uiCode, "video_full_range_flag");
			pcVUI->m_colourDescriptionPresentFlag		= READ_FLAG(     uiCode, "colour_description_present_flag");
			if (uiCode)
			{
				pcVUI->m_colourPrimaries				= READ_CODE(  8, uiCode, "colour_primaries");
				pcVUI->m_transferCharacteristics		= READ_CODE(  8, uiCode, "transfer_characteristics");
				pcVUI->m_matrixCoefficients				= READ_CODE(  8, uiCode, "matrix_coefficients");
			}
		}

		pcVUI->m_chromaLocInfoPresentFlag				= READ_FLAG(     uiCode, "chroma_loc_info_present_flag");
		if (uiCode)
		{
			pcVUI->m_chromaSampleLocTypeTopField		= READ_UVLC(     uiCode, "chroma_sample_loc_type_top_field" );
			pcVUI->m_chromaSampleLocTypeBottomField		= READ_UVLC(     uiCode, "chroma_sample_loc_type_bottom_field" );
		}

		pcVUI->m_neutralChromaIndicationFlag			= READ_FLAG(     uiCode, "neutral_chroma_indication_flag" );

		pcVUI->m_fieldSeqFlag							= READ_FLAG(     uiCode, "field_seq_flag" );

		assert(uiCode == 0);        // not supported yet

														  READ_FLAG(uiCode, "default_display_window_flag"); assert(uiCode == 0);
		pcVUI->m_picStructPresentFlag					= READ_FLAG(uiCode, "pic_struct_present_flag");

		pcVUI->m_hrdParametersPresentFlag				= READ_FLAG(     uiCode, "hrd_parameters_present_flag");
		if(uiCode)
		{
			Int i, j, nalOrVcl;

			pcVUI->m_timingInfoPresentFlag				= READ_FLAG(     uiCode, "timing_info_present_flag" );
			if(uiCode)
			{
				pcVUI->m_numUnitsInTick					= READ_CODE( 32, uiCode, "num_units_in_tick" );
				pcVUI->m_timeScale						= READ_CODE( 32, uiCode, "time_scale" );
			}
			pcVUI->m_nalHrdParametersPresentFlag		= READ_FLAG(     uiCode, "nal_hrd_parameters_present_flag" );
			pcVUI->m_vclHrdParametersPresentFlag		= READ_FLAG(     uiCode, "vcl_hrd_parameters_present_flag" );

			if( pcVUI->m_nalHrdParametersPresentFlag || pcVUI->m_vclHrdParametersPresentFlag )
			{
				pcVUI->m_subPicCpbParamsPresentFlag		= READ_FLAG(     uiCode, "sub_pic_Cpb_params_present_flag" );
				if(uiCode)
				{
					pcVUI->m_tickDivisorMinus2			= READ_CODE(  8, uiCode, "tick_divisor_minus2" );
					pcVUI->m_duCpbRemovalDelayLengthMinus1	= READ_CODE( 5, uiCode, "du_cpb_removal_delay_length_minus1" );
				}
				pcVUI->m_bitRateScale					= READ_CODE(  4, uiCode, "bit_rate_scale" );
				pcVUI->m_cpbSizeScale					= READ_CODE(  4, uiCode, "cpb_size_scale" );
				if( pcVUI->m_subPicCpbParamsPresentFlag )
				{
					pcVUI->m_duCpbSizeScale				= READ_CODE( 4, uiCode, "du_cpb_size_scale" );
				}
				pcVUI->m_initialCpbRemovalDelayLengthMinus1=READ_CODE( 5, uiCode, "initial_cpb_removal_delay_length_minus1" );
				pcVUI->m_cpbRemovalDelayLengthMinus1	= READ_CODE(  5, uiCode, "cpb_removal_delay_length_minus1" );
				pcVUI->m_dpbOutputDelayLengthMinus1		= READ_CODE(  5, uiCode, "dpb_output_delay_length_minus1" );
			}

			for( i = 0; i < pcSPS->m_uiMaxTLayers; i ++ )
			{
				pcVUI->m_HRD[i].fixedPicRateFlag		= READ_FLAG(     uiCode, "fixed_pic_rate_flag" );
				if( uiCode )
				{
					pcVUI->m_HRD[i].picDurationInTcMinus1=READ_UVLC(     uiCode, "pic_duration_in_tc_minus1" );
				}
				pcVUI->m_HRD[i].lowDelayHrdFlag			= READ_FLAG(     uiCode, "low_delay_hrd_flag" );
				pcVUI->m_HRD[i].cpbCntMinus1			= READ_UVLC(     uiCode, "cpb_cnt_minus1" );
				for( nalOrVcl = 0; nalOrVcl < 2; nalOrVcl ++ )
				{
					if( ( ( nalOrVcl == 0 ) && ( pcVUI->m_nalHrdParametersPresentFlag ) ) ||
						( ( nalOrVcl == 1 ) && ( pcVUI->m_vclHrdParametersPresentFlag ) ) )
					{
						for( j = 0; j < ( pcVUI->m_HRD[i].cpbCntMinus1 + 1 ); j ++ )
						{
							pcVUI->m_HRD[i].bitRateValueMinus1[j][nalOrVcl]	= READ_UVLC( uiCode, "bit_size_value_minus1" );
							pcVUI->m_HRD[i].cpbSizeValue[j][nalOrVcl]		= READ_UVLC( uiCode, "cpb_size_value_minus1" );
							if( pcVUI->m_subPicCpbParamsPresentFlag )
							{
								pcVUI->m_HRD[i].duCpbSizeValue[j][nalOrVcl]= READ_UVLC( uiCode, "du_cpb_size_value_minus1" );
							}
							pcVUI->m_HRD[i].cbrFlag[j][nalOrVcl]			= READ_FLAG( uiCode, "cbr_flag" );
						}
					}
				}
			}
		}
		pcVUI->m_pocProportionalToTimingFlag								= READ_FLAG( uiCode, "poc_proportional_to_timing_flag" );
		if( pcVUI->m_pocProportionalToTimingFlag && pcVUI->m_timingInfoPresentFlag )
		{
			pcVUI->m_numTicksPocDiffOneMinus1								= READ_UVLC( uiCode, "num_ticks_poc_diff_one_minus1" );
		}
		pcVUI->m_bitstreamRestrictionFlag				= READ_FLAG(     uiCode, "bitstream_restriction_flag");
		if (uiCode)
		{
			pcVUI->m_tilesFixedStructureFlag			= READ_FLAG(     uiCode, "tiles_fixed_structure_flag");
			pcVUI->m_motionVectorsOverPicBoundariesFlag	= READ_FLAG(     uiCode, "motion_vectors_over_pic_boundaries_flag");
			pcVUI->m_restrictedRefPicListsFlag			= READ_FLAG(   uiCode, "restricted_ref_pic_lists_flag");
			pcVUI->m_minSpatialSegmentationIdc			= READ_CODE( 8, uiCode, "min_spatial_segmentation_idc");
			pcVUI->m_maxBytesPerPicDenom				= READ_UVLC(     uiCode, "max_bytes_per_pic_denom" );
			pcVUI->m_maxBitsPerMinCuDenom				= READ_UVLC(     uiCode, "max_bits_per_mincu_denom" );
			pcVUI->m_log2MaxMvLengthHorizontal			= READ_UVLC(     uiCode, "log2_max_mv_length_horizontal" );
			pcVUI->m_log2MaxMvLengthVertical			= READ_UVLC(     uiCode, "log2_max_mv_length_vertical" );
		}
	}

	if (READ_FLAG( uiCode, "sps_extension_flag"))
	{
		assert(0);
		//while ( xMoreRbspData() )
		//{
		//	READ_FLAG( uiCode, "sps_extension_data_flag");
		//}
	}

	if(!h->initialized)
	{
		hevc_initialize(h,pcSPS->m_picWidthInLumaSamples,pcSPS->m_picHeightInLumaSamples);
	}
	return 0;
}

static lent_always_inline int decode_pps(HEVCContext *h)
{
	int tmp,i;
	PPS *pps, *pcPPS;
	UInt uiCode;
	Int iCode;

	tmp = get_ue(&h->bs); //pps_id
	if(!h->pps_buffers[tmp])
	{
		lent_mallocp(&h->pps_buffers[tmp],sizeof(PPS));
	}
	pps = h->pps_buffers[tmp];
	memset(pps,0,sizeof(*pps));
	pcPPS = pps;

	pcPPS->m_PPSId = tmp;

	pcPPS->m_PPSId										= READ_UVLC(     uiCode, "seq_parameter_set_id");
	pcPPS->m_dependentSliceEnabledFlag					= READ_FLAG(     uiCode, "dependent_slices_enabled_flag");

	pcPPS->m_signHideFlag								= READ_FLAG(     uiCode, "sign_data_hiding_flag" );

	pcPPS->m_cabacInitPresentFlag						= READ_FLAG(     uiCode,   "cabac_init_present_flag" );

	pcPPS->m_numRefIdxL0DefaultActive					= READ_UVLC(     uiCode, "num_ref_idx_l0_default_active_minus1")+1;
	pcPPS->m_numRefIdxL1DefaultActive					= READ_UVLC(     uiCode, "num_ref_idx_l1_default_active_minus1")+1;

	pcPPS->m_picInitQPMinus26							= READ_SVLC(     iCode, "pic_init_qp_minus26" );
	pcPPS->m_bConstrainedIntraPred						= READ_FLAG(     uiCode, "constrained_intra_pred_flag" );
	pcPPS->m_useTransformSkip							= READ_FLAG(     uiCode, "transform_skip_enabled_flag" );     

	pcPPS->m_useDQP										= READ_FLAG(     uiCode, "cu_qp_delta_enabled_flag" );
	if( pcPPS->m_useDQP )
	{
		pcPPS->m_uiMaxCuDQPDepth						= READ_UVLC(     uiCode, "diff_cu_qp_delta_depth" );
	}
	else
	{
		pcPPS->m_uiMaxCuDQPDepth						= 0;
	}
	pcPPS->m_chromaCbQpOffset							= READ_SVLC(     iCode, "cb_qp_offset");
	assert( pcPPS->m_chromaCbQpOffset >= -12 );
	assert( pcPPS->m_chromaCbQpOffset <=  12 );
	
	pcPPS->m_chromaCrQpOffset							= READ_SVLC(     iCode, "cr_qp_offset");
	assert( pcPPS->m_chromaCrQpOffset >= -12 );
	assert( pcPPS->m_chromaCrQpOffset <=  12 );

	pcPPS->m_bSliceChromaQpFlag							= READ_FLAG(     uiCode, "slicelevel_chroma_qp_flag" );

	pcPPS->m_bUseWeightedPred								= READ_FLAG(     uiCode, "weighted_pred_flag" );          // Use of Weighting Prediction (P_SLICE)
	
	pcPPS->m_bUseWeightedBiPred							= READ_FLAG(     uiCode, "weighted_bipred_flag" );         // Use of Bi-Directional Weighting Prediction (B_SLICE)

	pcPPS->m_OutputFlagPresentFlag						= READ_FLAG(     uiCode, "output_flag_present_flag" );

	pcPPS->m_TransquantBypassEnableFlag					= READ_FLAG(     uiCode, "transquant_bypass_enable_flag");

	pcPPS->m_tilesEnabledFlag							= READ_FLAG( uiCode, "tiles_enabled_flag"               );
	pcPPS->m_entropyCodingSyncEnabledFlag				= READ_FLAG( uiCode, "entropy_coding_sync_enabled_flag" );

	if( pcPPS->m_tilesEnabledFlag )
	{
		pcPPS->m_iNumColumnsMinus1						= READ_UVLC ( uiCode, "num_tile_columns_minus1" );
		pcPPS->m_iNumRowsMinus1							= READ_UVLC ( uiCode, "num_tile_rows_minus1" );
		pcPPS->m_uniformSpacingFlag						= READ_FLAG ( uiCode, "uniform_spacing_flag" );

		if( !pcPPS->m_uniformSpacingFlag)
		{
			if( pcPPS->m_iNumColumnsMinus1 > 0 )
			{
				lent_freep(&pcPPS->m_puiColumnWidth);
				pcPPS->m_puiColumnWidth = (UInt*)lent_malloc(pcPPS->m_iNumColumnsMinus1*sizeof(UInt));
				for(i=0; i<pcPPS->m_iNumColumnsMinus1; i++)
				{ 
					pcPPS->m_puiColumnWidth[i]			= READ_UVLC( uiCode, "column_width_minus1" ) + 1;  
				}
			}
			if( pcPPS->m_iNumRowsMinus1 > 0 )
			{
				lent_freep(&pcPPS->m_puiRowHeight);
				pcPPS->m_puiRowHeight = (UInt*)lent_malloc(pcPPS->m_iNumRowsMinus1*sizeof(UInt));
				for(i=0; i<pcPPS->m_iNumRowsMinus1; i++)
				{ 
					pcPPS->m_puiRowHeight[i]			= READ_UVLC( uiCode, "row_height_minus1" ) + 1;  
				}
			}
		}

		if(pcPPS->m_iNumColumnsMinus1 !=0 || pcPPS->m_iNumRowsMinus1 !=0)
		{
			pcPPS->m_loopFilterAcrossTilesEnabledFlag	= READ_FLAG( uiCode, "loop_filter_across_tiles_enabled_flag" );
		}
	}
	pcPPS->m_loopFilterAcrossSlicesEnabledFlag			= READ_FLAG( uiCode, "loop_filter_across_slices_enabled_flag" );
	pcPPS->m_deblockingFilterControlPresentFlag			= READ_FLAG( uiCode, "deblocking_filter_control_present_flag" );
	if(pcPPS->m_deblockingFilterControlPresentFlag)
	{
		pcPPS->m_deblockingFilterOverrideEnabledFlag	= READ_FLAG( uiCode, "deblocking_filter_override_enabled_flag" );
		pcPPS->m_picDisableDeblockingFilterFlag			= READ_FLAG( uiCode, "pic_disable_deblocking_filter_flag" );
		if(!pcPPS->m_picDisableDeblockingFilterFlag)
		{
			pcPPS->m_deblockingFilterBetaOffsetDiv2		= READ_SVLC ( iCode, "pps_beta_offset_div2" );
			pcPPS->m_deblockingFilterTcOffsetDiv2		= READ_SVLC ( iCode, "pps_tc_offset_div2" );
		}
	}
	pcPPS->m_scalingListPresentFlag						= READ_FLAG( uiCode, "pps_scaling_list_data_present_flag" );
	if(pcPPS->m_scalingListPresentFlag)
	{
		decode_sps_scalingList(h, &pcPPS->m_scalingList );
	}
	
	pcPPS->m_listsModificationPresentFlag				= READ_FLAG( uiCode, "lists_modification_present_flag");

	pcPPS->m_log2ParallelMergeLevelMinus2				= READ_UVLC( uiCode, "log2_parallel_merge_level_minus2");
	
	pcPPS->m_numExtraSliceHeaderBits					= READ_CODE(3, uiCode, "num_extra_slice_header_bits");


	pcPPS->m_sliceHeaderExtensionPresentFlag			= READ_FLAG( uiCode, "slice_header_extension_present_flag");

	READ_FLAG( uiCode, "pps_extension_flag");
	if (uiCode)
	{
		assert(0);
		//while ( xMoreRbspData() )
		//{
		//	READ_FLAG( uiCode, "pps_extension_data_flag");
		//}
	}

	return 0;
}

static int decode_vps(HEVCContext *h)
{
	UInt  uiCode;
	UInt i,opsIdx;
	VPS *pcVPS;
	UInt subLayerOrderingInfoPresentFlag;
	READ_CODE(  4, uiCode, "video_parameter_set_id" );
	//if(h->vps_buffers[uiCode]==NULL)
	{
		lent_freep((void**)&h->vps_buffers[uiCode]);
		h->vps_buffers[uiCode]=(VPS*)lent_mallocz(sizeof(VPS));
	}
	pcVPS=h->vps_buffers[uiCode];

	READ_CODE(  2, uiCode, "vps_reserved_three_2bits" );         assert(uiCode == 3);
	READ_CODE(  6, uiCode, "vps_reserved_zero_6bits" );          assert(uiCode == 0);

	pcVPS->m_uiMaxTLayers								= READ_CODE(  3, uiCode, "vps_max_sub_layers_minus1" ) + 1;
	pcVPS->m_bTemporalIdNestingFlag						= READ_FLAG(     uiCode, "vps_temporal_id_nesting_flag" );
	assert (pcVPS->m_uiMaxTLayers>1||pcVPS->m_bTemporalIdNestingFlag);
	READ_CODE( 16, uiCode, "vps_reserved_ffff_16bits" );         assert(uiCode == 0xffff);
	decode_sps_ptl(h, &pcVPS->m_pcPTL, 1, pcVPS->m_uiMaxTLayers-1);

	//parseBitratePicRateInfo( pcVPS->getBitratePicrateInfo(), 0, pcVPS->getMaxTLayers() - 1);
	{
		TComBitRatePicRateInfo *info=&pcVPS->m_bitRatePicRateInfo;
		for(i = 0; i < pcVPS->m_uiMaxTLayers; i++)
		{
			info->m_bitRateInfoPresentFlag[i]			= READ_FLAG(     uiCode, "bit_rate_info_present_flag[i]" );
			info->m_picRateInfoPresentFlag[i]			= READ_FLAG(     uiCode, "pic_rate_info_present_flag[i]" );
			if(info->m_bitRateInfoPresentFlag[i])
			{
				info->m_avgBitRate[i]					= READ_CODE( 16, uiCode, "avg_bit_rate[i]" );
				info->m_maxBitRate[i]					= READ_CODE( 16, uiCode, "max_bit_rate[i]" );
			}
			if(info->m_picRateInfoPresentFlag[i])
			{
				info->m_constantPicRateIdc[i]			= READ_CODE(  2, uiCode,  "constant_pic_rate_idc[i]" );
				info->m_avgPicRate[i]					= READ_CODE( 16, uiCode,  "avg_pic_rate[i]"          );
			}
		}
	}
	subLayerOrderingInfoPresentFlag						= READ_FLAG(     uiCode, "vps_sub_layer_ordering_info_present_flag");

	for(i = 0; i < pcVPS->m_uiMaxTLayers; i++)
	{
		pcVPS->m_uiMaxDecPicBuffering[i]				= READ_UVLC(     uiCode, "vps_max_dec_pic_buffering[i]" );
		pcVPS->m_numReorderPics[i]						= READ_UVLC(     uiCode, "vps_num_reorder_pics[i]" );
		pcVPS->m_uiMaxLatencyIncrease[i]				= READ_UVLC(     uiCode, "vps_max_latency_increase[i]" );
		if (!subLayerOrderingInfoPresentFlag)
		{
			for (i++; i < pcVPS->m_uiMaxTLayers; i++)
			{
				pcVPS->m_uiMaxDecPicBuffering[i]			= pcVPS->m_uiMaxDecPicBuffering[0];
				pcVPS->m_numReorderPics[i]					= pcVPS->m_numReorderPics[0];
				pcVPS->m_uiMaxLatencyIncrease[i]			= pcVPS->m_uiMaxLatencyIncrease[0];
			}
			break;
		}
	}
	READ_CODE( 6, uiCode, "vps_max_nuh_reserved_zero_layer_id" );   pcVPS->m_maxNuhReservedZeroLayerId	=( uiCode );
	READ_UVLC(    uiCode, "vps_max_op_sets_minus1" );               assert(!uiCode);//pcVPS->m_numOpSets					=( uiCode + 1 );
	//for( opsIdx = 1; opsIdx <= ( pcVPS->m_numOpSets - 1 ); opsIdx ++ )
	//{
	//	// Operation point set
	//	for( i = 0; i <= pcVPS->m_maxNuhReservedZeroLayerId; i ++ )
	//	{
	//		READ_FLAG( uiCode, "layer_id_included_flag[opsIdx][i]" );   //pcVPS->m_layerIdIncludedFlag[opsIdx][i]		=( uiCode );
	//	}
	//}

	READ_UVLC( uiCode, "vps_num_hrd_parameters" );                  pcVPS->m_numHrdParameters				=( uiCode );

	if( uiCode )
	{
		//pcVPS->createHrdParamBuffer();
	}
	/*for( i = 0; i < pcVPS->m_numHrdParameters; i ++ )
	{
		READ_UVLC( uiCode, "hrd_op_set_idx" );                       pcVPS->m_hrdOpSetIdx[ i ]				=( uiCode );
		if( i > 0 )
		{
			READ_FLAG( uiCode, "cprms_present_flag[i]" );               pcVPS->m_cprmsPresentFlag[ i ]		=( uiCode );
		}
		decode_HRDParameters(h, pcVPS->m_hrdParameters+i, pcVPS->m_cprmsPresentFlag[ i ], pcVPS->m_uiMaxTLayers - 1);
	}*/

	READ_FLAG( uiCode,  "vps_extension_flag" );
	if (uiCode)
	{
		//while ( xMoreRbspData() )
		{
			//  READ_FLAG( uiCode, "vps_extension_data_flag");
		}
	}
	return 0;
}

void hevc_uninitialize(HEVCContext *s,int free_pics)
{
	int i;

	//帧结构
	if(free_pics)
	{
		if(s->picture){
			for(i=0; i<s->picture_count; i++){
				lent_free_picture(s, &s->picture[i]);
			}
		}
	}
	lent_free_picture(s,&s->cache.alf_pic);
	lent_freep(&s->picture);

	//当前帧信息缓冲
	s->current.cu_depth_edge=NULL;
	s->current.cu_flag_edge=NULL;
	s->current.intra_pred_mode=NULL;
	s->current.predmode_edge=NULL;
	lent_freep((void**)&s->current.cu_depth_edge_base);
	lent_freep((void**)&s->current.cu_flag_edge_base);
	lent_freep((void**)&s->current.predmode_edge_base);
	lent_freep(&s->current.intra_pred_mode_base);
	lent_freep(&s->current.sao.saoLcuParam[0]);
	lent_freep(&s->current.sao.saoLcuParam[1]);
	lent_freep(&s->current.sao.saoLcuParam[2]);
	//lent_freep((void**)&s->current.luma);
	//lent_freep((void**)&s->current.chroma[0]);
	//lent_freep((void**)&s->current.chroma[1]);
	lent_freep((void**)&s->current.cu_depth);
	lent_freep((void**)&s->current.qp);
	//lent_freep((void**)&s->current.split_trans);
	//lent_freep((void**)&s->current.luma_nonfiltered);
	lent_freep((void**)&s->current.deblock_qp);
	lent_freep((void**)&s->current.deblock_strength);
	//lent_freep((void**)&s->current.i_cbp);
	lent_freep((void**)&s->intra_border[0]);
	lent_freep((void**)&s->temp_buffer);

	//SET
	
	for(i=0;i<MAX_VPS_COUNT;i++)
	{
		lent_freep(&s->vps_buffers[i]);
	}

	for(i=0;i<MAX_SPS_COUNT;i++)
	{
		if(free_pics&&s->sps_buffers[i]&&s->sps_buffers[i]->m_RPSList)
			lent_freep(&s->sps_buffers[i]->m_RPSList);
		lent_freep(&s->sps_buffers[i]);
	}
		
	for(i=0;i<MAX_PPS_COUNT;i++)
	{
		if(s->pps_buffers[i])
			lent_freep(&s->pps_buffers[i]->m_puiRowHeight);
		lent_freep(&s->pps_buffers[i]);
	}
	

	//lent_free_alfparam(&s->sh.alf);

	//内部缓冲
	s->free_buffer(s);

	//zigzag表
	lent_zigzag_uninit(s);

	//misc
	s->initialized = 0;
	s->current_picture_ptr= NULL;
	s->linesize= s->uvlinesize= 0;
	s->rbsp_buffer_size=0;
	lent_freep(&s->rbsp_buffer);
}
int hevc_initialize(HEVCContext *s,int width,int height)
{
	int i;

#if ARCH_X86_32
	int cpuInfo[4];
	int index=1;
	const int sse4_2 = 1 << (20);
	const int sse4_1 = 1 << (19);
	const int sse3 = 1 << (0);
	const int ssse3 = 1 << (9);
	const int sse2 = 1 << (26);
	const int sse1 = 1 << (25);
	__cpuid(cpuInfo, index);

	s->machine = 0;

	if ( 0 )
	{
	}
	else if(cpuInfo[2]&sse4_2) 
	{
		s->machine=42;
	}
	else if(cpuInfo[2]&sse4_1)
	{
		s->machine=41;
	}
	else if(cpuInfo[2]&ssse3)
	{
		s->machine=31;
	}
	else if ( cpuInfo[2] & sse3 )
	{
		s->machine = 30;
	}
	else if(cpuInfo[3]&sse2)
	{
		s->machine=20;
	}
	else if(cpuInfo[3]&sse1)
	{
		s->machine=10;
	}

#ifdef _DEBUG
	lent_log(NULL, LENT_LOG_DEBUG, "machine=%d\n", s->machine);
#endif
#endif

	s->width=width;
	s->height=height;
	s->ctx->width=width;
	s->ctx->height=height;

	//dsp_init
	dsp_init(&s->dsp, s->machine);

	//cu counting
	s->cu_width		= (s->width			+ 63) / 64;
	s->cu_height	= (s->height		+ 63) / 64;
	s->cu_stride	= s->cu_width		+ 1;
	s->b32_stride	= s->cu_width*2		+ 1;
	s->b16_stride	= s->cu_width*4		+ 1;
	s->b8_stride	= s->cu_width*8		+ 1;
	s->b4_stride	= s->cu_width*16	+ 1;
	s->cu_num = s->cu_width * s->cu_height;

	//picture structure buffer
	s->picture_count = MAX_PICTURE_COUNT;
	LENT_ALLOCZ_OR_GOTO(s, s->picture, s->picture_count * sizeof(Picture), fail);

	//initialize pictures
	for(i = 0; i < s->picture_count; i++) {
		lent_frame_get_defaults((LentFrame *)&s->picture[i]);
	}

	//当前帧信息缓冲
	LENT_ALLOCN_OR_GOTO(s, s->current.sao.saoLcuParam[0],	s->cu_width*s->cu_height								*sizeof(*s->current.sao.saoLcuParam[0]),fail);
	LENT_ALLOCN_OR_GOTO(s, s->current.sao.saoLcuParam[1],	s->cu_width*s->cu_height								*sizeof(*s->current.sao.saoLcuParam[1]),fail);
	LENT_ALLOCN_OR_GOTO(s, s->current.sao.saoLcuParam[2],	s->cu_width*s->cu_height								*sizeof(*s->current.sao.saoLcuParam[2]),fail);

	LENT_ALLOCN_OR_GOTO(s, s->current.cu_depth_edge_base,	s->cu_stride*(s->cu_height+1)							*sizeof(*s->current.cu_depth_edge_base),fail);
	LENT_ALLOCZ_OR_GOTO(s, s->current.cu_flag_edge_base,	s->cu_stride*(s->cu_height+1)							*sizeof(*s->current.cu_flag_edge_base),	fail);
	LENT_ALLOCN_OR_GOTO(s, s->current.predmode_edge_base,	s->cu_stride*(s->cu_height+1)							*sizeof(*s->current.predmode_edge_base),fail);
	LENT_ALLOCN_OR_GOTO(s, s->current.intra_pred_mode_base,	s->cu_stride*(s->cu_height+1)*(16/*64/4*64/4+64/8*64/8*2*/)	*sizeof(int8_t),						fail);
	//LENT_ALLOCZ_OR_GOTO(s, s->current.luma,					s->cu_width	*s->cu_height								*sizeof(*s->current.luma),				fail);
	//LENT_ALLOCZ_OR_GOTO(s, s->current.chroma[0],			s->cu_width	*s->cu_height								*sizeof(*s->current.chroma[0]),			fail);
	//LENT_ALLOCZ_OR_GOTO(s, s->current.chroma[1],			s->cu_width	*s->cu_height								*sizeof(*s->current.chroma[1]),			fail);
	LENT_ALLOCZ_OR_GOTO(s, s->current.cu_depth,				s->cu_width	*s->cu_height								*sizeof(*s->current.cu_depth),			fail);
	LENT_ALLOCZ_OR_GOTO(s, s->current.qp,					s->cu_width	*s->cu_height								*sizeof(*s->current.qp),				fail);
	//LENT_ALLOCZ_OR_GOTO(s, s->current.split_trans,			s->cu_width	*s->cu_height								*sizeof(*s->current.split_trans),		fail);
	LENT_ALLOCZ_OR_GOTO(s, s->current.deblock_qp,			s->cu_width	*s->cu_height								*sizeof(*s->current.deblock_qp),		fail);
	LENT_ALLOCZ_OR_GOTO(s, s->current.deblock_strength,		s->cu_width	*s->cu_height								*sizeof(*s->current.deblock_strength),	fail);
	//LENT_ALLOCZ_OR_GOTO(s, s->current.i_cbp,				s->cu_width	*s->cu_height								*sizeof(*s->current.i_cbp),			fail);
	LENT_ALLOCZ_OR_GOTO(s, s->intra_border[0],				s->cu_width	*64*2										*sizeof(*s->intra_border),				fail);
	LENT_ALLOCZ_OR_GOTO(s, s->temp_buffer,					64*64*4													*sizeof(*s->temp_buffer),				fail);
	s->intra_border[1]=s->intra_border[0]+s->cu_width*64;
	s->intra_border[2]=s->intra_border[1]+s->cu_width*32;

	s->current.cu_depth_edge	=s->current.cu_depth_edge_base		+(s->cu_stride+1);
	s->current.cu_flag_edge		=s->current.cu_flag_edge_base		+(s->cu_stride+1);
	s->current.predmode_edge	=s->current.predmode_edge_base		+(s->cu_stride+1);
	s->current.intra_pred_mode	=s->current.intra_pred_mode_base	+(s->cu_stride+1)*(16/*64/4*64/4+64/8*64/8*2*/);

	//initialize zigzag
	lent_zigzag_init(s);


	//initialize loopfilter
	lent_deblock_init(&s->deblock,s->machine);
	lent_pred_init(&s->pred,s->machine);

	s->delay_length=MAX_DELAYED_PIC_COUNT;//todo. 可能可以优化，减小输出延迟

	//initialized tag
	s->initialized = 1;
	return 0;

fail:
	hevc_uninitialize(s,1);
	return -1;
}
static lent_always_inline Int getNumRpsCurrTempList(int m_eSliceType, RPS *m_pcRPS)
{
  Int numRpsCurrTempList = 0;
  UInt i;

  if (m_eSliceType == SLICE_I) 
  {
    return 0;
  }
  for(i=0; i < m_pcRPS->m_numberOfNegativePictures+ m_pcRPS->m_numberOfPositivePictures + m_pcRPS->m_numberOfLongtermPictures; i++)
  {
    if(m_pcRPS->m_used[i])
    {
      numRpsCurrTempList++;
    }
  }
  return numRpsCurrTempList;
}
static void decode_sh_PredWeightTable( HEVCContext *h )
{
	SliceHeader		*pcSlice=&h->sh;
	wpScalingParam  *wp;
	Bool            bChroma     = 1; // color always present in HEVC ?
	PPS*			pps         = pcSlice->m_pcPPS;
	SliceType       eSliceType  = pcSlice->m_eSliceType;
	Int             iNbRef       = (eSliceType == SLICE_B ) ? (2) : (1);
	UInt            uiLog2WeightDenomLuma, uiLog2WeightDenomChroma;
	UInt            uiMode      = 0;
	UInt            uiTotalSignalledWeightFlags = 0;
	Int iNumRef;
	Int iDeltaDenom;
	Int iRefIdx;
	UInt  uiCode;

	// decode delta_luma_log2_weight_denom :
																		  READ_UVLC( uiLog2WeightDenomLuma, "luma_log2_weight_denom" );     // ue(v): luma_log2_weight_denom
	if( bChroma ) 
	{
																		  READ_SVLC( iDeltaDenom, "delta_chroma_log2_weight_denom" );     // se(v): delta_chroma_log2_weight_denom
		assert((iDeltaDenom + (Int)uiLog2WeightDenomLuma)>=0);
		uiLog2WeightDenomChroma = (UInt)(iDeltaDenom + uiLog2WeightDenomLuma);
	}

	for ( iNumRef=0 ; iNumRef<iNbRef ; iNumRef++ ) 
	{
		Int  eRefPicList = ( iNumRef ? REF_PIC_LIST_1 : REF_PIC_LIST_0 );
		for ( iRefIdx=0 ; iRefIdx<pcSlice->m_aiNumRefIdx[eRefPicList] ; iRefIdx++ ) 
		{
			wp=pcSlice->m_weightPredTable[eRefPicList][iRefIdx];

			wp[0].uiLog2WeightDenom = uiLog2WeightDenomLuma;
			wp[1].uiLog2WeightDenom = uiLog2WeightDenomChroma;
			wp[2].uiLog2WeightDenom = uiLog2WeightDenomChroma;

			wp[0].bPresentFlag											= READ_FLAG( uiCode, "luma_weight_lX_flag" );           // u(1): luma_weight_l0_flag
			uiTotalSignalledWeightFlags									+=uiCode;
		}
		if ( bChroma ) 
		{
			for ( iRefIdx=0 ; iRefIdx<pcSlice->m_aiNumRefIdx[eRefPicList] ; iRefIdx++ ) 
			{
				wp=pcSlice->m_weightPredTable[eRefPicList][iRefIdx];
				READ_FLAG( uiCode, "chroma_weight_lX_flag" );      // u(1): chroma_weight_l0_flag
				wp[1].bPresentFlag = uiCode;
				wp[2].bPresentFlag = uiCode;
				uiTotalSignalledWeightFlags += 2*wp[1].bPresentFlag;
			}
		}
		for ( iRefIdx=0 ; iRefIdx<pcSlice->m_aiNumRefIdx[eRefPicList] ; iRefIdx++ ) 
		{
			wp=pcSlice->m_weightPredTable[eRefPicList][iRefIdx];
			if ( wp[0].bPresentFlag ) 
			{
				Int iDeltaWeight;
				READ_SVLC( iDeltaWeight, "delta_luma_weight_lX" );  // se(v): delta_luma_weight_l0[i]
				wp[0].iWeight = (iDeltaWeight + (1<<wp[0].uiLog2WeightDenom));
				READ_SVLC( wp[0].iOffset, "luma_offset_lX" );       // se(v): luma_offset_l0[i]
			}
			else 
			{
				wp[0].iWeight = (1 << wp[0].uiLog2WeightDenom);
				wp[0].iOffset = 0;
			}
			if ( bChroma ) 
			{
				Int j;
				if ( wp[1].bPresentFlag ) 
				{
					for ( j=1 ; j<3 ; j++ ) 
					{
						Int iDeltaWeight;
						Int iDeltaChroma;
						Int shift = 1<<(h->g_bitDepthC-1);
						Int pred;
						READ_SVLC( iDeltaWeight, "delta_chroma_weight_lX" );  // se(v): chroma_weight_l0[i][j]
						wp[j].iWeight = (iDeltaWeight + (1<<wp[1].uiLog2WeightDenom));

						READ_SVLC( iDeltaChroma, "delta_chroma_offset_lX" );  // se(v): delta_chroma_offset_l0[i][j]
						pred = ( shift - ( ( shift*wp[j].iWeight)>>(wp[j].uiLog2WeightDenom) ) );
						wp[j].iOffset = lent_clip((iDeltaChroma + pred), -128, 127  );
					}
				}
				else 
				{
					for ( j=1 ; j<3 ; j++ ) 
					{
						wp[j].iWeight = (1 << wp[j].uiLog2WeightDenom);
						wp[j].iOffset = 0;
					}
				}
			}
		}

		for ( iRefIdx=pcSlice->m_aiNumRefIdx[eRefPicList] ; iRefIdx<MAX_NUM_REF ; iRefIdx++ ) 
		{
			wp=pcSlice->m_weightPredTable[eRefPicList][iRefIdx];

			wp[0].bPresentFlag = 0;
			wp[1].bPresentFlag = 0;
			wp[2].bPresentFlag = 0;
		}
	}
	assert(uiTotalSignalledWeightFlags<=24);
}

static lent_always_inline int decode_slice_header(HEVCContext *h)
{
	SliceHeader *sh=&h->sh;
	SPS *sps;
	PPS *pps;

	SliceHeader *rpcSlice = sh;

	UInt firstSliceInPic;
	UInt  uiCode;
	Int   iCode;

	Int numCUs;
	Int maxParts;
	Int numParts = 0;
	UInt lCUAddress = 0;
	Int reqBitsOuter = 0;
	Int reqBitsInner = 0;
	UInt innerAddress = 0;
	Int  sliceAddress = 0;

	firstSliceInPic														= READ_FLAG(     uiCode, "first_slice_in_pic_flag" );
	if(  h->nal_unit_type == NAL_UNIT_CODED_SLICE_IDR
      || h->nal_unit_type == NAL_UNIT_CODED_SLICE_IDR_N_LP
      || h->nal_unit_type == NAL_UNIT_CODED_SLICE_BLA_N_LP
      || h->nal_unit_type == NAL_UNIT_CODED_SLICE_BLANT
      || h->nal_unit_type == NAL_UNIT_CODED_SLICE_BLA
      || h->nal_unit_type == NAL_UNIT_CODED_SLICE_CRA
	  )
	{ 
																		  READ_FLAG(     uiCode, "no_output_of_prior_pics_flag" );  //ignored
	}


	rpcSlice->m_iPPSId													= READ_UVLC(     uiCode, "pic_parameter_set_id" );
	h->pps=*h->pps_buffers[uiCode];
	h->sps=*h->sps_buffers[h->pps.m_SPSId];
	h->vps=*h->vps_buffers[h->sps.m_VPSId];
	pps=&h->pps;
	sps=&h->sps;
	rpcSlice->m_pcPPS=pps;
	rpcSlice->m_pcSPS=sps;

	rpcSlice->m_eNalUnitType=h->nal_unit_type;

//#if DEPENDENT_SLICE_SEGMENT_FLAGS
	if( pps->m_dependentSliceEnabledFlag && ( !firstSliceInPic ))
	{
		rpcSlice->m_dependentSliceFlag									= READ_FLAG(     uiCode, "dependent_slice_flag" );
	}
	else
	{
		rpcSlice->m_dependentSliceFlag									= 0;
	}
//#endif
	numCUs = ((sps->m_picWidthInLumaSamples+sps->m_uiMaxCUWidth-1)/sps->m_uiMaxCUWidth)*((sps->m_picHeightInLumaSamples+sps->m_uiMaxCUHeight-1)/sps->m_uiMaxCUHeight);
	maxParts = (1<<(sps->m_uiMaxCUDepth<<1));
	while(numCUs>(1<<reqBitsOuter))
	{
		reqBitsOuter++;
	}
	while((numParts)>(1<<reqBitsInner)) 
	{
		reqBitsInner++;
	}

	if(!firstSliceInPic)
	{
		UInt address;
		address=														  READ_CODE( reqBitsOuter+reqBitsInner, uiCode, "slice_address" );
		lCUAddress = address >> reqBitsInner;
		innerAddress = address - (lCUAddress<<reqBitsInner);
	}
	//set uiCode to equal slice start address (or dependent slice start address)
	sliceAddress=(maxParts*lCUAddress)+(innerAddress);
	rpcSlice->m_uiDependentSliceCurStartCUAddr=sliceAddress;
	rpcSlice->m_uiDependentSliceCurEndCUAddr=numCUs*maxParts;


	if (rpcSlice->m_dependentSliceFlag)
	{
		rpcSlice->m_bNextSlice			=0;
		rpcSlice->m_bNextDependentSlice	=1;
	}
	else
	{
		rpcSlice->m_bNextSlice			=1;
		rpcSlice->m_bNextDependentSlice	=0;

		rpcSlice->m_uiSliceCurStartCUAddr=sliceAddress;
		rpcSlice->m_uiSliceCurEndCUAddr=numCUs*maxParts;
	}

	if(!rpcSlice->m_dependentSliceFlag)
	{
		TComRefPicListModification* refPicListModification;
		rpcSlice->m_eSliceType											= READ_UVLC(     uiCode, "slice_type" );
		if( pps->m_OutputFlagPresentFlag )
		{
			rpcSlice->m_PicOutputFlag									= READ_FLAG(     uiCode, "pic_output_flag" );
		}
		else
		{
			rpcSlice->m_PicOutputFlag									= 1;
		}
		// in the first version chroma_format_idc is equal to one, thus colour_plane_id will not be present
		assert (sps->m_chromaFormatIdc == 1 );
		// if( separate_colour_plane_flag  ==  1 )
		//   colour_plane_id                                      u(2)

		if( rpcSlice->m_eNalUnitType == NAL_UNIT_CODED_SLICE_IDR || rpcSlice->m_eNalUnitType == NAL_UNIT_CODED_SLICE_IDR_N_LP )
		{
			RPS* rps = &rpcSlice->m_LocalRPS;
			rpcSlice->m_iPOC=0;
			rps->m_numberOfNegativePictures=(0);
			rps->m_numberOfPositivePictures=(0);
			rps->m_numberOfLongtermPictures=(0);
			rps->m_numberOfPictures=(0);
			rpcSlice->m_pcRPS=rps;
		}
		else
		{
			
			RPS* rps;
			//decode POC	 
			Int iPOClsb													= READ_CODE(sps->m_uiBitsForPOC, uiCode, "pic_order_cnt_lsb"); 
			Int iPrevPOC = rpcSlice->m_prevPOC;
			Int iMaxPOClsb = 1<< sps->m_uiBitsForPOC;
			Int iPrevPOClsb = iPrevPOC%iMaxPOClsb;
			Int iPrevPOCmsb = iPrevPOC-iPrevPOClsb;
			Int iPOCmsb;
			if( ( iPOClsb  <  iPrevPOClsb ) && ( ( iPrevPOClsb - iPOClsb )  >=  ( iMaxPOClsb / 2 ) ) )
			{
				iPOCmsb = iPrevPOCmsb + iMaxPOClsb;
			}
			else if( (iPOClsb  >  iPrevPOClsb )  && ( (iPOClsb - iPrevPOClsb )  >  ( iMaxPOClsb / 2 ) ) ) 
			{
				iPOCmsb = iPrevPOCmsb - iMaxPOClsb;
			}
			else
			{
				iPOCmsb = iPrevPOCmsb;
			}
			if ( rpcSlice->m_eNalUnitType == NAL_UNIT_CODED_SLICE_BLA
				|| rpcSlice->m_eNalUnitType == NAL_UNIT_CODED_SLICE_BLANT
				|| rpcSlice->m_eNalUnitType == NAL_UNIT_CODED_SLICE_BLA_N_LP )
			{
				// For BLA picture types, POCmsb is set to 0.
				iPOCmsb = 0;
			}
			rpcSlice->m_iPOC= (iPOCmsb+iPOClsb);



			//decode reference
																		  READ_FLAG(      uiCode, "short_term_ref_pic_set_sps_flag" );
			if(uiCode == 0) // use short-term reference picture set explicitly signalled in slice header
			{
				rps = &rpcSlice->m_LocalRPS;
				parseShortTermRefPicSet(h, sps,rps, sps->m_RPSNum);
				rpcSlice->m_pcRPS=rps;
			}
			else // use reference to short-term reference picture set in PPS
			{
																		  READ_UVLC(     uiCode, "short_term_ref_pic_set_idx");
				rpcSlice->m_pcRPS=&sps->m_RPSList[uiCode];
				rps = rpcSlice->m_pcRPS;
			}
			if(sps->m_bLongTermRefsPresent)
			{
				Int offset = rps->m_numberOfNegativePictures+rps->m_numberOfPositivePictures;
				Int j,k;
				UInt numOfLtrp = 0;
				UInt numLtrpInSPS = 0;
				Int bitsForLtrpInSPS = 1;
				Int maxPicOrderCntLSB = 1 << rpcSlice->m_pcSPS->m_uiBitsForPOC;
				Int prevLSB = 0, prevDeltaMSB = 0, deltaPocMSBCycleLT = 0;

				if (rpcSlice->m_pcSPS->m_numLongTermRefPicSPS > 0)
				{
					numLtrpInSPS										= READ_UVLC(     uiCode, "num_long_term_sps");
					numOfLtrp += numLtrpInSPS;
					rps->m_numberOfLongtermPictures=(numOfLtrp);
				}
				while (rpcSlice->m_pcSPS->m_numLongTermRefPicSPS > (1 << bitsForLtrpInSPS))
				{
					bitsForLtrpInSPS++;
				}
				rps->m_numberOfLongtermPictures							= READ_UVLC(     uiCode, "num_long_term_pics");
				numOfLtrp += uiCode;
				rps->m_numberOfLongtermPictures=(numOfLtrp);
				for(j=offset+rps->m_numberOfLongtermPictures-1, k = 0; k < numOfLtrp; j--, k++)
				{
					Int pocLsbLt;
					Bool mSBPresentFlag;
					if (k < numLtrpInSPS)
					{
						Int usedByCurrFromSPS;
																		  READ_CODE(bitsForLtrpInSPS, uiCode, "lt_idx_sps[i]");
						usedByCurrFromSPS=rpcSlice->m_pcSPS->m_usedByCurrPicLtSPSFlag[uiCode];
						pocLsbLt = rpcSlice->m_pcSPS->m_ltRefPicPocLsbSps[uiCode];

						rps->m_used[j]=usedByCurrFromSPS;
					}
					else
					{
						pocLsbLt										= READ_CODE(rpcSlice->m_pcSPS->m_uiBitsForPOC, uiCode, "poc_lsb_lt");
						rps->m_used[j]									= READ_FLAG(     uiCode, "used_by_curr_pic_lt_flag");
					}
					
					mSBPresentFlag										= READ_FLAG(     uiCode,"delta_poc_msb_present_flag");
					if(mSBPresentFlag)                  
					{
						Bool deltaFlag = 0;
						Int pocLTCurr;
																		  READ_UVLC(     uiCode, "delta_poc_msb_cycle_lt[i]" );
						//            First LTRP                               || First LTRP from SH           || curr LSB    != prev LSB
						if( (j == offset+rps->m_numberOfLongtermPictures-1) || (j == offset+(numOfLtrp-numLtrpInSPS)-1) || (pocLsbLt != prevLSB) )
						{
							deltaFlag = 1;
						}
						if(deltaFlag)
						{
							deltaPocMSBCycleLT = uiCode;
						}
						else
						{
							deltaPocMSBCycleLT = uiCode + prevDeltaMSB;              
						}

						pocLTCurr				= rpcSlice->m_iPOC - deltaPocMSBCycleLT * maxPicOrderCntLSB - iPOClsb + pocLsbLt;
						rps->m_POC[j]			= pocLTCurr; 
						rps->m_deltaPOC[j]		= - rpcSlice->m_iPOC + pocLTCurr;
						rps->m_bCheckLTMSB[j]	=1;
					}
					else
					{
						rps->m_POC[j]			= pocLsbLt; 
						rps->m_deltaPOC[j]		= - rpcSlice->m_iPOC + pocLsbLt;
						rps->m_bCheckLTMSB[j]	= 0;
					}
					prevLSB = pocLsbLt;
					prevDeltaMSB = deltaPocMSBCycleLT;
				}
				offset += rps->m_numberOfLongtermPictures;
				rps->m_numberOfPictures=(offset);        
			}  
			if ( rpcSlice->m_eNalUnitType == NAL_UNIT_CODED_SLICE_BLA
				|| rpcSlice->m_eNalUnitType == NAL_UNIT_CODED_SLICE_BLANT
				|| rpcSlice->m_eNalUnitType == NAL_UNIT_CODED_SLICE_BLA_N_LP )
			{
				// In the case of BLA picture types, rps data is read from slice header but ignored
				rps								= &rpcSlice->m_LocalRPS;
				rps->m_numberOfNegativePictures	= 0;
				rps->m_numberOfPositivePictures	= 0;
				rps->m_numberOfLongtermPictures	= 0;
				rps->m_numberOfPictures			= 0;
				rpcSlice->m_pcRPS				= rps;
			}
			
			if(h->ctx->compatibility>=92)
			{
				if (rpcSlice->m_pcSPS->m_TMVPFlagsPresent)
				{
					rpcSlice->m_enableTMVPFlag								= READ_FLAG(     uiCode, "enable_temporal_mvp_flag" );
				}
				else
				{
					rpcSlice->m_enableTMVPFlag								= 0;
				}
			}
		}

		if(sps->m_bUseSAO)
		{
			//if (sps->getUseSAO())
			{
				rpcSlice->m_saoEnabledFlag								= READ_FLAG(     uiCode, "slice_sao_luma_flag");
				{
					rpcSlice->m_saoEnabledFlagChroma					= READ_FLAG(     uiCode, "slice_sao_chroma_flag");
				}
			}
		}
		if ((rpcSlice->m_eNalUnitType==NAL_UNIT_CODED_SLICE_IDR||rpcSlice->m_eNalUnitType==NAL_UNIT_CODED_SLICE_IDR_N_LP))
		{
				rpcSlice->m_enableTMVPFlag								= 0;
		}
		else
			if(h->ctx->compatibility<92)
				if (rpcSlice->m_pcSPS->m_TMVPFlagsPresent)
				{
					rpcSlice->m_enableTMVPFlag								= READ_FLAG(     uiCode, "enable_temporal_mvp_flag" );
				}
				else
				{
					rpcSlice->m_enableTMVPFlag								= 0;
				}
		if (rpcSlice->m_eSliceType != SLICE_I)
		{
																		  READ_FLAG( uiCode, "num_ref_idx_active_override_flag");
			if (uiCode)
			{
				rpcSlice->m_aiNumRefIdx[REF_PIC_LIST_0]					= READ_UVLC (uiCode, "num_ref_idx_l0_active_minus1") + 1;
				if (rpcSlice->m_eSliceType == SLICE_B)
				{
					rpcSlice->m_aiNumRefIdx[REF_PIC_LIST_1]				= READ_UVLC (uiCode, "num_ref_idx_l1_active_minus1") + 1;
				}
				else
				{
					rpcSlice->m_aiNumRefIdx[REF_PIC_LIST_1]				= 0;
				}
			}
			else
			{
				rpcSlice->m_aiNumRefIdx[REF_PIC_LIST_0]					= rpcSlice->m_pcPPS->m_numRefIdxL0DefaultActive;
				if (rpcSlice->m_eSliceType == SLICE_B)
				{
					rpcSlice->m_aiNumRefIdx[REF_PIC_LIST_1]				= rpcSlice->m_pcPPS->m_numRefIdxL1DefaultActive;
				}
				else
				{
					rpcSlice->m_aiNumRefIdx[REF_PIC_LIST_1]				= 0;
				}
			}
		}


		refPicListModification = &rpcSlice->m_RefPicListModification;
		if(rpcSlice->m_eSliceType != SLICE_I)
		{
			if( !rpcSlice->m_pcSPS->m_listsModificationPresentFlag || getNumRpsCurrTempList(rpcSlice->m_eSliceType, rpcSlice->m_pcRPS) <= 1 )
			{
				refPicListModification->m_bRefPicListModificationFlagL0 = 0;
			}
			else
			{
				refPicListModification->m_bRefPicListModificationFlagL0	= READ_FLAG(     uiCode, "ref_pic_list_modification_flag_l0" );
			}

			if(refPicListModification->m_bRefPicListModificationFlagL0)
			{
				Int i = 0;
				Int numRpsCurrTempList0 = getNumRpsCurrTempList(rpcSlice->m_eSliceType, rpcSlice->m_pcRPS);
				uiCode = 0;
				if ( numRpsCurrTempList0 > 1 )
				{
					Int length = 1;
					numRpsCurrTempList0 --;
					while ( numRpsCurrTempList0 >>= 1) 
					{
						length ++;
					}
					for (i = 0; i < rpcSlice->m_aiNumRefIdx[REF_PIC_LIST_0]; i ++)
					{
						refPicListModification->m_RefPicSetIdxL0[i]		= READ_CODE( length, uiCode, "list_entry_l0" );
					}
				}
				else
				{
					for (i = 0; i < rpcSlice->m_aiNumRefIdx[REF_PIC_LIST_0]; i ++)
					{
						refPicListModification->m_RefPicSetIdxL0[i]		= 0;
					}
				}
			}
		}
		else
		{
			refPicListModification->m_bRefPicListModificationFlagL0		= 0;
		}
		if(rpcSlice->m_eSliceType == SLICE_B)
		{
			if( !rpcSlice->m_pcSPS->m_listsModificationPresentFlag || getNumRpsCurrTempList(rpcSlice->m_eSliceType, rpcSlice->m_pcRPS) <= 1 )
			{
				refPicListModification->m_bRefPicListModificationFlagL1 = 0;
			}
			else
			{
				refPicListModification->m_bRefPicListModificationFlagL1 = READ_FLAG(     uiCode, "ref_pic_list_modification_flag_l1" ); 
			}

			if(refPicListModification->m_bRefPicListModificationFlagL1)
			{
				Int i = 0;
				Int numRpsCurrTempList1 = getNumRpsCurrTempList(rpcSlice->m_eSliceType, rpcSlice->m_pcRPS);
				uiCode = 0;
				if ( numRpsCurrTempList1 > 1 )
				{
					Int length = 1;
					numRpsCurrTempList1 --;
					while ( numRpsCurrTempList1 >>= 1) 
					{
						length ++;
					}
					for (i = 0; i < rpcSlice->m_aiNumRefIdx[REF_PIC_LIST_1]; i ++)
					{
						refPicListModification->m_RefPicSetIdxL1[i]		= READ_CODE( length, uiCode, "list_entry_l1" );
					}
				}
				else
				{
					for (i = 0; i < rpcSlice->m_aiNumRefIdx[REF_PIC_LIST_1]; i ++)
					{
						refPicListModification->m_RefPicSetIdxL1[i]		= 0;
					}
				}
			}
		}  
		else
		{
			refPicListModification->m_bRefPicListModificationFlagL1		= 0;
		}
		if (rpcSlice->m_eSliceType == SLICE_B)
		{
			rpcSlice->m_bLMvdL1Zero										= READ_FLAG(     uiCode, "mvd_l1_zero_flag" );
		}

		rpcSlice->m_cabacInitFlag										= 0;
		if(pps->m_cabacInitPresentFlag && !rpcSlice->m_eSliceType == SLICE_I)
		{
			rpcSlice->m_cabacInitFlag									= READ_FLAG(     uiCode, "cabac_init_flag");
		}

		if ( rpcSlice->m_enableTMVPFlag )
		{
			if ( rpcSlice->m_eSliceType == SLICE_B )
			{
				rpcSlice->m_colFromL0Flag								= READ_FLAG( uiCode, "collocated_from_l0_flag" );
			}
			else
			{
				rpcSlice->m_colFromL0Flag								= 1;
			}

			if ( rpcSlice->m_eSliceType != SLICE_I &&
				((rpcSlice->m_colFromL0Flag==1 && rpcSlice->m_aiNumRefIdx[REF_PIC_LIST_0]>1)||
				(rpcSlice->m_colFromL0Flag ==0 && rpcSlice->m_aiNumRefIdx[REF_PIC_LIST_1]>1)))
			{
				rpcSlice->m_colRefIdx									= READ_UVLC( uiCode, "collocated_ref_idx" );
			}
			else
			{
				rpcSlice->m_colRefIdx									= 0;
			}
		}
		if ( (pps->m_bUseWeightedPred && rpcSlice->m_eSliceType==SLICE_P) || (pps->m_bUseWeightedBiPred && rpcSlice->m_eSliceType==SLICE_B) )
		{
			//Int e,i,yuv,bitDepth;
			wpScalingParam (*wp)[MAX_NUM_REF][3]=h->sh.m_weightPredTable;
			decode_sh_PredWeightTable(h);
			  //for ( e=0 ; e<2 ; e++ )//seems useless
			  //{
				 // for ( i=0 ; i<MAX_NUM_REF ; i++ )
				 // {
					//  for ( yuv=0 ; yuv<3 ; yuv++ )
					//  {
					//	  wpScalingParam  *pwp = &(wp[e][i][yuv]);
					//	  if ( !pwp->bPresentFlag ) {
					//		  // Inferring values not present :
					//		  pwp->iWeight = (1 << pwp->uiLog2WeightDenom);
					//		  pwp->iOffset = 0;
					//	  }

					//	  pwp->w      = pwp->iWeight;
					//	  bitDepth = yuv ? h->g_bitDepthC : h->g_bitDepthY;
					//	  pwp->o      = pwp->iOffset << (bitDepth-8);
					//	  pwp->shift  = pwp->uiLog2WeightDenom;
					//	  pwp->round  = (pwp->uiLog2WeightDenom>=1) ? (1 << (pwp->uiLog2WeightDenom-1)) : (0);
					//  }
				 // }
			  //}
		}
		if (rpcSlice->m_eSliceType!=SLICE_I)
		{
#define MRG_MAX_NUM_CANDS 5
																		  READ_UVLC( uiCode, "five_minus_max_num_merge_cand");
			rpcSlice->m_maxNumMergeCand									= MRG_MAX_NUM_CANDS - uiCode;
		}

		rpcSlice->m_iSliceQp											= READ_SVLC( iCode, "slice_qp_delta" )+ 26 + pps->m_picInitQPMinus26;

		assert( rpcSlice->m_iSliceQp >= -sps->m_qpBDOffsetY );
		assert( rpcSlice->m_iSliceQp <=  51 );

		if (rpcSlice->m_pcPPS->m_bSliceChromaQpFlag)
		{
			rpcSlice->m_iSliceQpDeltaCb									= READ_SVLC( iCode, "slice_qp_delta_cb" );
			assert( rpcSlice->m_iSliceQpDeltaCb >= -12 );
			assert( rpcSlice->m_iSliceQpDeltaCb <=  12 );
			assert( (rpcSlice->m_pcPPS->m_chromaCbQpOffset + rpcSlice->m_iSliceQpDeltaCb) >= -12 );
			assert( (rpcSlice->m_pcPPS->m_chromaCbQpOffset + rpcSlice->m_iSliceQpDeltaCb) <=  12 );
			
			rpcSlice->m_iSliceQpDeltaCr									= READ_SVLC( iCode, "slice_qp_delta_cr" );
			assert( rpcSlice->m_iSliceQpDeltaCr >= -12 );
			assert( rpcSlice->m_iSliceQpDeltaCr <=  12 );
			assert( (rpcSlice->m_pcPPS->m_chromaCrQpOffset + rpcSlice->m_iSliceQpDeltaCr) >= -12 );
			assert( (rpcSlice->m_pcPPS->m_chromaCrQpOffset + rpcSlice->m_iSliceQpDeltaCr) <=  12 );
		}

		if (rpcSlice->m_pcPPS->m_deblockingFilterControlPresentFlag)
		{
			if(rpcSlice->m_pcPPS->m_deblockingFilterOverrideEnabledFlag)
			{
				rpcSlice->m_deblockingFilterOverrideFlag				= READ_FLAG ( uiCode, "deblocking_filter_override_flag" );
			}
			else
			{  
				rpcSlice->m_deblockingFilterOverrideFlag				= 0;
			}
			if(rpcSlice->m_deblockingFilterOverrideFlag)
			{
				rpcSlice->m_deblockingFilterDisable						= READ_FLAG ( uiCode, "slice_disable_deblocking_filter_flag" );
				if(!rpcSlice->m_deblockingFilterDisable)
				{
					rpcSlice->m_deblockingFilterBetaOffsetDiv2			= READ_SVLC( iCode, "beta_offset_div2" );
					rpcSlice->m_deblockingFilterTcOffsetDiv2			= READ_SVLC( iCode, "tc_offset_div2" );
				}
			}
			else
			{
				rpcSlice->m_deblockingFilterDisable						= ( rpcSlice->m_pcPPS->m_picDisableDeblockingFilterFlag );
				rpcSlice->m_deblockingFilterBetaOffsetDiv2				= ( rpcSlice->m_pcPPS->m_deblockingFilterBetaOffsetDiv2 );
				rpcSlice->m_deblockingFilterTcOffsetDiv2				= ( rpcSlice->m_pcPPS->m_deblockingFilterTcOffsetDiv2 );
			}
		}
		{
			Bool isSAOEnabled = (!rpcSlice->m_pcSPS->m_bUseSAO)?(0):(rpcSlice->m_saoEnabledFlag||rpcSlice->m_saoEnabledFlagChroma);
			Bool isDBFEnabled = (!rpcSlice->m_deblockingFilterDisable);

			if(rpcSlice->m_pcPPS->m_loopFilterAcrossSlicesEnabledFlag && ( isSAOEnabled || isDBFEnabled ))
			{
				uiCode													= READ_FLAG( uiCode, "slice_loop_filter_across_slices_enabled_flag");
			}
			else
			{
				uiCode													= rpcSlice->m_pcPPS->m_loopFilterAcrossSlicesEnabledFlag;
			}
			rpcSlice->m_LFCrossSliceBoundaryFlag						= uiCode;
		}

	}
	if( pps->m_tilesEnabledFlag || pps->m_entropyCodingSyncEnabledFlag )
	{
		UInt *entryPointOffset          = NULL;
		UInt numEntryPointOffsets, offsetLenMinus1;
		UInt idx;

		rpcSlice->m_numEntryPointOffsets								= READ_UVLC(numEntryPointOffsets, "num_entry_point_offsets");
		if (numEntryPointOffsets>0)
		{
																		  READ_UVLC(offsetLenMinus1, "offset_len_minus1");
		}

		entryPointOffset = (UInt*)lent_malloc(numEntryPointOffsets*sizeof(UInt));
		for (idx=0; idx<numEntryPointOffsets; idx++)
		{
			entryPointOffset[ idx ]										= READ_CODE(offsetLenMinus1+1, uiCode, "entry_point_offset");
		}

		if ( pps->m_tilesEnabledFlag )
		{
			assert(0);//todo tiles support
			//rpcSlice->setTileLocationCount( numEntryPointOffsets );
			//rpcSlice->m_tileByteLocation

			//UInt prevPos = 0;
			//for (Int idx=0; idx<rpcSlice->getTileLocationCount(); idx++)
			//{
			//	rpcSlice->setTileLocation( idx, prevPos + entryPointOffset [ idx ] );
			//	prevPos += entryPointOffset[ idx ];
			//}
		}
		else if ( pps->m_entropyCodingSyncEnabledFlag )
		{
			assert(0);
			//Int numSubstreams = rpcSlice->getNumEntryPointOffsets()+1;
			//rpcSlice->allocSubstreamSizes(numSubstreams);
			//UInt *pSubstreamSizes       = rpcSlice->getSubstreamSizes();
			//for (Int idx=0; idx<numSubstreams-1; idx++)
			//{
			//	if ( idx < numEntryPointOffsets )
			//	{
			//		pSubstreamSizes[ idx ] = ( entryPointOffset[ idx ] << 3 ) ;
			//	}
			//	else
			//	{
			//		pSubstreamSizes[ idx ] = 0;
			//	}
			//}
		}

		if (entryPointOffset)
		{
			lent_free(entryPointOffset);
		}
	}
	else
	{
		rpcSlice->m_numEntryPointOffsets								= 0;
	}

	if(pps->m_sliceHeaderExtensionPresentFlag)
	{
		Int i;
		READ_UVLC(uiCode,"slice_header_extension_length");
		for(i=0; i<uiCode; i++)
		{
			UInt ignore;
			READ_CODE(8,ignore,"slice_header_extension_data_byte");
		}
	}
	
	uiCode																= READ_FLAG( uiCode, "align");assert(uiCode);
	

	h->dropable=!(h->nal_ref_idc>0);

	lent_picture_start(h);

	rpcSlice->m_prevPOC=rpcSlice->m_iPOC;

	////////////////////////////////////////
	// old version


	h->current_picture_ptr->idr=h->nal_unit_type==NAL_UNIT_CODED_SLICE_IDR||h->nal_unit_type==NAL_UNIT_CODED_SLICE_IDR_N_LP;
	h->current_picture_ptr->poc=
		h->current_picture.poc=sh->m_iPOC;

	//lent_dlog(NULL,"poc %d start dpbcount %d\n",sh->i_poc,h->dpb_count);
	
	if(sh->m_eSliceType!=SLICE_I)
		lent_build_def_list(h);


	//save list info
	{
		int i,list;
		for(list=0;list<2;list++)
		{
			h->current_picture_ptr->ref_count[list]=h->i_ref_count[list];
			for(i=0;i<h->i_ref_count[list];i++)
			{
				h->current_picture_ptr->ref_poc[list][i]=h->ref_list[list][i].poc;
			}
		}
	}

	h->cache.i_last_qp=sh->m_iSliceQp;

	return 0;
}

#define WAIT_LINE(pic,y) lentoid_thread_await_progress((LentFrame*)(pic),LENTMIN((pic)->height-1,LENTMAX(0,y)))

static lent_always_inline void mc_internal(HEVCContext *h, pixel *p_dst0, pixel *p_dst1, pixel *p_dst2, int idx,int width,int height,int cu_x,int cu_y)
{
	pixel *src[2][3];
	int x,y;
	unsigned int i4=LENT_scan4[idx];
	int offsetsrc,linesize,linesizeuv;
	int weighted;

	int16_t *buffer[2];
	int stridesrc[2]={64,64};
	buffer[0]=(int16_t*)h->temp_buffer;
	buffer[1]=buffer[0]+(1<<12);

	weighted=(h->sh.m_eSliceType==SLICE_P&&h->sh.m_pcPPS->m_bUseWeightedPred)||(h->sh.m_eSliceType==SLICE_B&&h->sh.m_pcPPS->m_bUseWeightedBiPred);

	x=(i4&(32-1)-16)<<2;
	assert(x==IDX2x(idx));
	y=((i4-32)>>5)<<2;
	assert(y==IDX2y(idx));

	linesize=h->current_picture.linesize[0];
	linesizeuv=h->current_picture.linesize[1];
	assert(h->current_picture.linesize[1]==h->current_picture.linesize[2]);

	offsetsrc   =(h->cu_y*64)*linesize+h->cu_x*64;

	if(h->cache.ref[0][i4]>=0&&h->cache.ref[1][i4]>=0)//bipred
	{
		int mvx[2],mvy[2],list;
		
		//LUMA
		for(list=0;list<2;list++)
		{
			int16_t mv[2];
			int ref=h->cache.ref[list][i4];
			CP32(mv,h->cache.mv[list][LENT_scan4[idx]]);
			lent_clip_mv(h,mv,cu_x,cu_y);

			mvx[list]=(x)*4+mv[0];
			mvy[list]=(y)*4+mv[1];

			src[list][0]=h->ref_list[list][ref].data[0];
			src[list][1]=h->ref_list[list][ref].data[1];
			src[list][2]=h->ref_list[list][ref].data[2];

			WAIT_LINE(&h->ref_list[list][ref],(mvy[list]>>2)+height+h->cu_y*64+4);//todo is this correct?

			if(mvx[list]&3)
				h->dsp.hpel_filter(h->cache.hpel[list][0],h->cache.hpel[list][1],h->cache.hpel[list][2],src[list][0]+offsetsrc+(mvx[list]>>2)+(mvy[list]>>2)*linesize,linesize,width,height,1<<(mvx[list]&3));

			src[list][0]=(pixel*)h->dsp.get_ref(buffer[list],&stridesrc[list],src[list][0]+offsetsrc,h->cache.hpel[list],linesize,mvx[list],mvy[list],width,height);
		}
		if(!weighted)
		{
			h->dsp.mc_avg(p_dst0,linesize,(int16_t*)src[0][0],stridesrc[0],(int16_t*)src[1][0],stridesrc[1],width,height);

			//CHROMA
			width>>=1;
			height>>=1;

			//U
			h->dsp.get_ref_chroma( buffer[0], 64, src[0][1]+(h->cu_y*32)*linesizeuv+h->cu_x*32, linesizeuv,
				mvx[0], mvy[0], width, height );
			h->dsp.get_ref_chroma( buffer[1], 64, src[1][1]+(h->cu_y*32)*linesizeuv+h->cu_x*32, linesizeuv,
				mvx[1], mvy[1], width, height );
			h->dsp.mc_avg( p_dst1, linesizeuv,
				buffer[0], 64, buffer[1], 64, width, height );
			//V
			h->dsp.get_ref_chroma( buffer[0], 64, src[0][2]+(h->cu_y*32)*linesizeuv+h->cu_x*32, linesizeuv,
				mvx[0], mvy[0], width, height );
			h->dsp.get_ref_chroma( buffer[1], 64, src[1][2]+(h->cu_y*32)*linesizeuv+h->cu_x*32, linesizeuv,
				mvx[1], mvy[1], width, height );
			h->dsp.mc_avg( p_dst2, linesizeuv,
				buffer[0], 64, buffer[1], 64, width, height );
		}
		else
		{
			int yuv;
			wpScalingParam *wp0 = h->sh.m_weightPredTable[0][h->cache.ref[0][i4]];
			wpScalingParam *wp1 = h->sh.m_weightPredTable[1][h->cache.ref[1][i4]];
			for ( yuv=0 ; yuv<3 ; yuv++ )
			{
				Int bitDepth = yuv ? h->g_bitDepthC : h->g_bitDepthY;
				wp0[yuv].w      = wp0[yuv].iWeight;
				wp0[yuv].o      = wp0[yuv].iOffset * (1 << (bitDepth-8));
				wp1[yuv].w      = wp1[yuv].iWeight;
				wp1[yuv].o      = wp1[yuv].iOffset * (1 << (bitDepth-8));
				wp0[yuv].offset = wp0[yuv].o + wp1[yuv].o;
				wp0[yuv].shift  = wp0[yuv].uiLog2WeightDenom + 1;
				//wp0[yuv].round  = (1 << wp0[yuv].uiLog2WeightDenom);
				wp1[yuv].offset = wp0[yuv].offset;
				wp1[yuv].shift  = wp0[yuv].shift;
				//wp1[yuv].round  = wp0[yuv].round;
			}
			h->dsp.mc_weight_bi(p_dst0,linesize,(int16_t*)src[0][0],stridesrc[0],(int16_t*)src[1][0],stridesrc[1],width,height,wp0[0].w,wp1[0].w,wp0[0].offset,wp0[0].shift,h->g_bitDepthY);

			//CHROMA
			width>>=1;
			height>>=1;

			//U
			h->dsp.get_ref_chroma( buffer[0], 64, src[0][1]+(h->cu_y*32)*linesizeuv+h->cu_x*32, linesizeuv,
				mvx[0], mvy[0], width, height );
			h->dsp.get_ref_chroma( buffer[1], 64, src[1][1]+(h->cu_y*32)*linesizeuv+h->cu_x*32, linesizeuv,
				mvx[1], mvy[1], width, height );
			h->dsp.mc_weight_bi( p_dst1, linesizeuv,
				buffer[0], 64, buffer[1], 64, width, height,
				wp0[1].w,wp1[1].w,wp0[1].offset,wp0[1].shift,h->g_bitDepthC );
			//V
			h->dsp.get_ref_chroma( buffer[0], 64, src[0][2]+(h->cu_y*32)*linesizeuv+h->cu_x*32, linesizeuv,
				mvx[0], mvy[0], width, height );
			h->dsp.get_ref_chroma( buffer[1], 64, src[1][2]+(h->cu_y*32)*linesizeuv+h->cu_x*32, linesizeuv,
				mvx[1], mvy[1], width, height );
			h->dsp.mc_weight_bi( p_dst2, linesizeuv,
				buffer[0], 64, buffer[1], 64, width, height,
				wp0[2].w,wp1[2].w,wp0[2].offset,wp0[2].shift,h->g_bitDepthC );

		}
		
	}
	else//list 0 or list 1
	{
		int list=1-(h->cache.ref[0][i4]>=0);
		int ref=h->cache.ref[list][i4];
		int mvx,mvy;
		int16_t mv[2];

		assert(ref>=0);


		CP32(mv,h->cache.mv[list][i4]);
		lent_clip_mv(h,mv,cu_x,cu_y);

		mvx=(x)*4+mv[0];
		mvy=(y)*4+mv[1];

		src[0][0]=h->ref_list[list][ref].data[0];
		src[0][1]=h->ref_list[list][ref].data[1];
		src[0][2]=h->ref_list[list][ref].data[2];

		WAIT_LINE(&h->ref_list[list][ref],(mvy>>2)+height+h->cu_y*64+4);//todo is this correct?

		if(mvx&3)
			h->dsp.hpel_filter(h->cache.hpel[0][0],h->cache.hpel[0][1],h->cache.hpel[0][2],src[0][0]+offsetsrc+(mvx>>2)+(mvy>>2)*linesize,linesize,width,height,1<<(mvx&3));

		if(!weighted)
		{//simple
			h->dsp.mc_luma	(p_dst0, linesize, src[0][0]+(h->cu_y*64)*linesize+h->cu_x*64,h->cache.hpel[0],linesize,mvx,mvy,width,height);
			h->dsp.mc_chroma(p_dst1, linesizeuv, src[0][1]+(h->cu_y*32)*linesizeuv+h->cu_x*32,linesizeuv,mvx,mvy,width>>1,height>>1);
			h->dsp.mc_chroma(p_dst2, linesizeuv, src[0][2]+(h->cu_y*32)*linesizeuv+h->cu_x*32,linesizeuv,mvx,mvy,width>>1,height>>1);
		}
		else
		{
			int yuv;
			wpScalingParam *pwp = h->sh.m_weightPredTable[list][ref];
			for ( yuv=0 ; yuv<3 ; yuv++ )
			{
			  Int bitDepth = yuv ? h->g_bitDepthC : h->g_bitDepthY;
			  pwp[yuv].w      = pwp[yuv].iWeight;
			  pwp[yuv].offset = pwp[yuv].iOffset * (1 << (bitDepth-8));
			  pwp[yuv].shift  = pwp[yuv].uiLog2WeightDenom;
			  //pwp[yuv].round  = (pwp[yuv].uiLog2WeightDenom>=1) ? (1 << (pwp[yuv].uiLog2WeightDenom-1)) : (0);
			}

			src[0][0]=(pixel*)h->dsp.get_ref	(buffer[0],	&stridesrc[0],src[0][0]+(h->cu_y*64)*linesize+h->cu_x*64,h->cache.hpel[0],linesize,mvx,mvy,width,height);
			h->dsp.mc_weight_uni(p_dst0, linesize, (int16_t*)src[0][0], stridesrc[0], width, height, pwp[0].w, pwp[0].offset, pwp[0].shift, h->g_bitDepthY);
			
			width>>=1;
			height>>=1;
			h->dsp.get_ref_chroma	(buffer[0],	64,src[0][1]+(h->cu_y*32)*linesize+h->cu_x*32,linesizeuv,mvx,mvy,width,height);
			h->dsp.mc_weight_uni(p_dst1, linesizeuv, buffer[0], 64, width, height, pwp[1].w, pwp[1].offset, pwp[1].shift, h->g_bitDepthC);
			h->dsp.get_ref_chroma	(buffer[0],	64,src[0][2]+(h->cu_y*32)*linesize+h->cu_x*32,linesizeuv,mvx,mvy,width,height);
			h->dsp.mc_weight_uni(p_dst2, linesizeuv, buffer[0], 64, width, height, pwp[2].w, pwp[2].offset, pwp[2].shift, h->g_bitDepthC);
		}
	}
}

static lent_always_inline void load_recon_cache(HEVCContext *h)
{
	int linesize	=h->current_picture_ptr->linesize[0];
	int linesizeuv	=h->current_picture_ptr->linesize[1];
	int i_neighbor	=h->i_neighbor;
	assert(h->current_picture_ptr->linesize[1]==h->current_picture_ptr->linesize[2]);

	//load neighbor pixels
	//load_neighbor( h->cache.p_fdec[0], h->current_picture_ptr->data[0]+(h->cu_y*linesize+h->cu_x)*64,   h->intra_border[0]+(h->cu_x<<6), linesize,   1, i_neighbor );
	//load_neighbor( h->cache.p_fdec[1], h->current_picture_ptr->data[1]+(h->cu_y*linesizeuv+h->cu_x)*32,	h->intra_border[1]+(h->cu_x<<5), linesizeuv, 0, i_neighbor );
	//load_neighbor( h->cache.p_fdec[2], h->current_picture_ptr->data[2]+(h->cu_y*linesizeuv+h->cu_x)*32,	h->intra_border[2]+(h->cu_x<<5), linesizeuv, 0, i_neighbor );

	//generate exist_map
	memset(h->cache.unit_exist_map,0,sizeof(h->cache.unit_exist_map));
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

static lent_always_inline void xchg_cu_border_before( HEVCContext *h, pixel *p_src[3], int i_width, int i_pix_x0 )
{ //TODO: 优化 可以根据mode仅拷贝intra预测需要的数据
	int ofx = h->cu_x * 32 + (i_pix_x0 >> 1);
	int w2 = i_width == 4 ? 10 : i_width + 2;
	int ww = (i_width << 1) + 2;
	int *linesize = h->current_picture_ptr->linesize;
	pixel *border;
	pixel *buf = h->temp_buffer;
	pixel *src;
	
	border = &h->intra_border[0][ofx<<1] - 1;
	src = p_src[0] - linesize[0] - 1;
	memcpy( buf, src, ww );
	memcpy( src, border, ww );

	border = &h->intra_border[1][ofx] - 1;
	src = p_src[1] - linesize[1] - 1;
	memcpy( buf+144, src, w2 );
	memcpy( src, border, w2 );

	border = &h->intra_border[2][ofx] - 1;
	src = p_src[2] - linesize[1] - 1;
	memcpy( buf+224, src, w2 );
	memcpy( src, border, w2 );
}

static lent_always_inline void xchg_cu_border_after( HEVCContext *h, pixel *p_src[3], int i_width )
{ //TODO: 优化 可以根据mode仅拷贝intra预测需要的数据
	int w2 = w2 = i_width == 4 ? 10 : i_width + 2;
	int ww = (i_width << 1) + 2;
	int *linesize = h->current_picture_ptr->linesize;
	pixel *buf = h->temp_buffer;
	pixel *src;
	
	src = p_src[0] - linesize[0] - 1;
	memcpy( src, buf, ww );

	src = p_src[1] - linesize[1] - 1;
	memcpy( src, buf+144, w2 );

	src = p_src[2] - linesize[1] - 1;
	memcpy( src, buf+224, w2 );
}

static void recon_tree(HEVCContext *h,int idx,int depth)
{
	int i_width=1<<DEPTH2LOG2(depth),i_height=i_width;
	int i_log2_size=DEPTH2LOG2(depth);
	//int treeidx=LENT_cu_depth_treeidx[depth] + ((idx>>(CU_LOG2_64x64-CU_LOG2_4x4-depth))>>(CU_LOG2_64x64-CU_LOG2_4x4-depth));
	int treeidx=TREEIDX(idx,DEPTH2LOG2(depth));
	int idxstep=DEPTH2IDXSTEPC(depth);
	int i_mode=h->cache.intra_pred_mode[LENT_scan4[idx]];
	int i_chroma_mode=lent_get_chroma_mode(i_mode,h->cache.intra_pred_mode[LENT_scan4[256+(idx>>2)]]);
	int i_chroma_size=LENTMAX((i_width>>1),4);
	int i_pix_x0=IDX2x(idx),i_pix_y0=IDX2y(idx);
	//int b_split_trans=h->current.split_trans[h->cu_index][treeidx];
	int b_split_trans=h->cache.b_split_trans[treeidx];
	int x,y;

	pixel *p_src[3];
	int linesize	=h->current_picture_ptr->linesize[0];
	int linesizeuv	=h->current_picture_ptr->linesize[1];
	int hx = i_pix_x0 >> 1, hy = i_pix_y0 >> 1;

	p_src[0] = h->current_picture_ptr->data[0]+(h->cu_y*64+i_pix_y0)*linesize+h->cu_x*64+i_pix_x0;
	p_src[1] = h->current_picture_ptr->data[1]+(h->cu_y*32+hy)*linesizeuv+h->cu_x*32+hx;
	p_src[2] = h->current_picture_ptr->data[2]+(h->cu_y*32+hy)*linesizeuv+h->cu_x*32+hx;

	x=(LENT_scan4[idx]&(32-1)-16)<<2;
	y=((LENT_scan4[idx]-32)>>5)<<2;
	if(i_mode<0)
	{
		if(!h->cache.unit_exist_map[LENT_scan4[idx]])
		{
			int partmode=h->cache.part_mode[treeidx];
			int thisstep=DEPTH2IDXSTEPP(depth);
			int step4 = thisstep>>2;
			int hw = i_width >> 1;
			int hh = i_height >> 1;
			int ofy, ofuv;
			int hhh = hh >> 1;
			int hhw = hw >> 1;

			switch(partmode)
			{
			case SIZE_2Nx2N:
				mc_internal(h,p_src[0],p_src[1],p_src[2],idx,i_width,i_height,x,y);
				break;
			case SIZE_2NxN:
				ofy = hh*linesize;
				ofuv = hhh*linesizeuv;
				mc_internal(h,p_src[0],p_src[1],p_src[2],idx,            i_width,hh,x,y);
				mc_internal(h,p_src[0]+ofy,p_src[1]+ofuv,p_src[2]+ofuv,idx+(step4<<1), i_width,hh,x,y);
				break;
			case SIZE_Nx2N:
				mc_internal(h,p_src[0],p_src[1],p_src[2],idx,       hw,i_height,x,y);
				mc_internal(h,p_src[0]+hw,p_src[1]+hhw,p_src[2]+hhw,idx+step4, hw,i_height,x,y);
				break;
			case SIZE_NxN:
				ofy = hh*linesize;
				ofuv = hh*linesizeuv;
				mc_internal(h,p_src[0],p_src[1],p_src[2],idx,         hw,hh,x,y);
				mc_internal(h,p_src[0]+hw,p_src[1]+hhw,p_src[2]+hhw,idx+step4,   hw,hh,x,y);
				mc_internal(h,p_src[0]+ofy,p_src[1]+ofuv,p_src[2]+ofuv,idx+step4*2, hw,hh,x,y);
				mc_internal(h,p_src[0]+ofy+hw,p_src[1]+ofuv+hhw,p_src[2]+ofuv+hhw,idx+step4*3, hw,hh,x,y);
				break;
			default:
				assert(0);
			}
			lent_fill_rectangle(&h->cache.unit_exist_map[LENT_scan4[idx]],i_width/4,i_height/4,32,1);
			
		}
	}
	if(b_split_trans)
	{
		recon_tree(h,idx,			depth+1);
		recon_tree(h,idx+idxstep*1,	depth+1);
		recon_tree(h,idx+idxstep*2,	depth+1);
		recon_tree(h,idx+idxstep*3,	depth+1);
	}
	else
	{
		if(i_mode>=0)
		{
			if( i_pix_y0 == 0 )
				xchg_cu_border_before( h, p_src, i_width, i_pix_x0 );

			{
				lent_predict_load_neighbor( h, p_src[0], linesize, i_pix_x0, i_pix_y0, i_width, 1 );
				switch( i_mode )
				{
				case PLANAR_IDX:
					h->pred.predict_planar[i_log2_size-2]( p_src[0], linesize, i_log2_size,  1 + h->sps.m_useStrongIntraSmoothing );
					break;
				case DC_IDX: 
					h->pred.predict_dc[i_log2_size-2]( p_src[0], linesize );
					if(depth>1)
						lent_predict_dc_filter( p_src[0], linesize, i_width );
					break;
				case HOR_IDX:
					h->pred.predict_h[i_log2_size-2]( p_src[0], linesize );
					if(depth>1)
						lent_predict_edge_filter( p_src[0], linesize, DEPTH2LOG2(depth), 0 );
					break;
				case VER_IDX:
					h->pred.predict_v[i_log2_size-2]( p_src[0], linesize );
					if(depth>1)
						lent_predict_edge_filter( p_src[0], linesize, DEPTH2LOG2(depth), 1 );
					break;
				default:
					h->pred.predict_ang[i_log2_size-2]( p_src[0], linesize, i_mode, i_log2_size,  1 + h->sps.m_useStrongIntraSmoothing );
				}
			}
			if((idx&3)==0){
				if(i_log2_size>2)
					i_log2_size--;
				lent_predict_load_neighbor( h, p_src[1], linesizeuv, i_pix_x0, i_pix_y0, i_chroma_size, 0 );
				lent_predict_load_neighbor( h, p_src[2], linesizeuv, i_pix_x0, i_pix_y0, i_chroma_size, 0 );
				switch( i_chroma_mode )
				{
				case PLANAR_IDX:
					h->pred.predict_planar[i_log2_size-2]( p_src[1], linesizeuv, i_log2_size, 0 );
					h->pred.predict_planar[i_log2_size-2]( p_src[2], linesizeuv, i_log2_size, 0 );
					break;
				case DC_IDX: 
					h->pred.predict_dc[i_log2_size-2]( p_src[1], linesizeuv );
					h->pred.predict_dc[i_log2_size-2]( p_src[2], linesizeuv );
					break;
				case HOR_IDX:
					h->pred.predict_h[i_log2_size-2]( p_src[1], linesizeuv );
					h->pred.predict_h[i_log2_size-2]( p_src[2], linesizeuv );
					break;
				case VER_IDX:
					h->pred.predict_v[i_log2_size-2]( p_src[1], linesizeuv );
					h->pred.predict_v[i_log2_size-2]( p_src[2], linesizeuv );
					break;
// 				case 35:
// 					if(i_width>4)
// 						lent_predict_lmc( h, i_pix_x0, i_pix_y0, DEPTH2LOG2(depth)-1 );
// 					break;
				default:
					h->pred.predict_ang[i_log2_size-2]( p_src[1], linesizeuv, i_chroma_mode, i_log2_size, 0 );
					h->pred.predict_ang[i_log2_size-2]( p_src[2], linesizeuv, i_chroma_mode, i_log2_size, 0 );
				}
			}

			if( i_pix_y0 == 0 )
				xchg_cu_border_after( h, p_src, i_width );
		}
		if(CBF_LUMA(h->cache.i_cbp[treeidx]))
		{
			switch(i_width)
			{
			case 4:
				h->dsp.add4x4_idct(p_src[0],linesize,&h->cache.luma_out[idx<<4],i_mode<0?35:i_mode);
				break;
			case 8:
				h->dsp.add8x8_idct(p_src[0],linesize,&h->cache.luma_out[idx<<4]);
				break;
			case 16:
				h->dsp.add16x16_idct(p_src[0],linesize,&h->cache.luma_out[idx<<4]);
				break;
			case 32:
				h->dsp.add32x32_idct(p_src[0],linesize,&h->cache.luma_out[idx<<4]);
				break;
			default:
				assert(0);
				;
			}
		}
		if(CBF_CB(h->cache.i_cbp[TREEIDX(idx&~3,DEPTH2LOG2(depth))]))
		switch(i_width)
		{
		case 4:
			if((idx&3)==3)
			{
				h->dsp.add4x4_idct(p_src[1]-2*linesizeuv-2,linesizeuv,&h->cache.chroma_out[0][(idx-3)<<2],CHROMA_DCT);
			}
			break;
		case 8:
			h->dsp.add4x4_idct(p_src[1],linesizeuv,&h->cache.chroma_out[0][idx<<2],CHROMA_DCT);
			break;
		case 16:
			h->dsp.add8x8_idct(p_src[1],linesizeuv,&h->cache.chroma_out[0][idx<<2]);
			break;
		case 32:
			h->dsp.add16x16_idct(p_src[1],linesizeuv,&h->cache.chroma_out[0][idx<<2]);
			break;
		default:
			assert(0);
			;
		}
		if(CBF_CR(h->cache.i_cbp[TREEIDX(idx&~3,DEPTH2LOG2(depth))]))
		//if(dct_nonzero(&h->current.chroma[0][h->cu_index][(idx&~3)<<2],i_chroma_size*i_chroma_size*sizeof(dctcoef))||dct_nonzero(&h->current.chroma[1][h->cu_index][(idx&~3)<<2],i_chroma_size*i_chroma_size*sizeof(dctcoef)))
		switch(i_width)
		{
		case 4:
			if((idx&3)==3)
			{
				h->dsp.add4x4_idct(p_src[2]-2*linesizeuv-2,linesizeuv,&h->cache.chroma_out[1][(idx-3)<<2],CHROMA_DCT);
			}
			break;
		case 8:
			h->dsp.add4x4_idct(p_src[2],linesizeuv,&h->cache.chroma_out[1][idx<<2],CHROMA_DCT);
			break;
		case 16:
			h->dsp.add8x8_idct(p_src[2],linesizeuv,&h->cache.chroma_out[1][idx<<2]);
			break;
		case 32:
			h->dsp.add16x16_idct(p_src[2],linesizeuv,&h->cache.chroma_out[1][idx<<2]);
			break;
		default:
			assert(0);
			;
		}
		lent_fill_rectangle(&h->cache.unit_exist_map[LENT_scan4[idx]],i_width/4,i_height/4,32,1);

	}
}
static lent_always_inline void finish_row(HEVCContext *h, int y)//第y行CU解码完毕
{
	int sides,drawheight;
	int offset=y*64*h->current_picture.linesize[0];

	int cu_y=y+1;

	if( cu_y < h->cu_height )
	{
		memcpy( h->intra_border[0], h->current_picture.data[0]+((cu_y<<6)-1)*h->current_picture.linesize[0], h->cu_width<<6 );
		memcpy( h->intra_border[1], h->current_picture.data[1]+((cu_y<<5)-1)*h->current_picture.linesize[1], h->cu_width<<5 );
		memcpy( h->intra_border[2], h->current_picture.data[2]+((cu_y<<5)-1)*h->current_picture.linesize[2], h->cu_width<<5 );
	}

	//deblock

	if(cu_y<=h->cu_height)
	{
		//memcpy(h->current.luma_nonfiltered+offset,h->current_picture.data[0]+offset,h->current_picture.linesize[0]*64);//backup nonfiltered value
		lent_deblock_row( h, y );
	}


	//B-nonref
	if(h->dropable)
		return;

	//draw edges
	if(cu_y<=h->cu_height)
	{
		sides=0;

		if(!y)
			sides|=SIDE_TOP;
		if(y==h->cu_height-1)
			sides|=SIDE_BOTTOM;
		drawheight=LENTMIN((h->height-y*64),64);

		h->dsp.draw_edge(h->current_picture.data[0]+y*64*h->current_picture.linesize[0],		drawheight,h->width, h->current_picture.linesize[0],sides,0);
		h->dsp.draw_edge(h->current_picture.data[1]+(y*64>>1)*h->current_picture.linesize[1],	drawheight/2,h->width/2,h->current_picture.linesize[1],sides,1);
		h->dsp.draw_edge(h->current_picture.data[2]+(y*64>>1)*h->current_picture.linesize[2],	drawheight/2,h->width/2,h->current_picture.linesize[2],sides,1);
	}

	//report for multithread
	lentoid_thread_report_progress((LentFrame*)h->current_picture_ptr,LENTMAX(y*64-DRAW_EDGE_DELAY,-1));
}
static lent_always_inline int decode_slice(HEVCContext *h)
{
	int static test=0;
	/* realign */
	align_get_bits( &h->bs );

	/* init cabac */
	lent_init_cabac_states( &h->cabac);
	lent_init_cabac_decoder( &h->cabac,
		h->bs.buffer + get_bits_count(&h->bs)/8,
		(get_bits_left(&h->bs) + 7)/8);

	lent_hevc_init_cabac_states(h);

	h->cu_x=h->cu_y=0;
	h->i_neighbor=0;
	test++;

	
	h->current.sao.bSaoFlag[0] = h->sh.m_saoEnabledFlag;
	h->current.sao.bSaoFlag[1] = h->sh.m_saoEnabledFlagChroma;
	//main decoding loop
	//lent_dlog(NULL,"Thread %d Begin poc %d\n",lent_thread_getnumber(h),h->sh.i_poc);
	for(;;)
	{
		int idx,idxstep;
		h->cu_index=h->cu_x+h->cu_y*h->cu_width;
		h->cu_xy=h->cu_x+h->cu_y*h->cu_stride;
		if(h->cu_x==h->cu_width-1)
			h->i_neighbor&=~CU_TOPRIGHT;
		//语法解析

		if(h->sps.m_bUseSAO&&(h->sh.m_saoEnabledFlag||h->sh.m_saoEnabledFlagChroma))
		{
			lent_hevc_decode_saoparam(h);
		}
	//lent_log(NULL, LENT_LOG_ERROR, "Enter cabac\n");
		lent_hevc_decode_cu_cabac(h);
		if(h->sh.m_iPOC==80)
		lent_dlog(NULL,"CU %d end, cabac %d\n",h->cu_index,h->cabac.range);
	//lent_log(NULL, LENT_LOG_ERROR, "leave cabac\n");
	
		load_recon_cache(h);
		//recon
		for(idx=0;idx<256;idx+=idxstep)
		{
			//int depth=h->current.cu_depth[h->cu_index][idx];
			int depth=h->cache.cu_depth[LENT_scan8[idx>>2]];
			if(depth<0){idxstep=4;continue;}

			idxstep=DEPTH2IDXSTEPP(depth);

			recon_tree(h,idx,depth);
		}
		//save_recon_cache(h);

		if( ++h->cu_x == h->cu_width )
		{
			h->cu_x=0;
			h->cu_y++;
			h->i_neighbor=CU_TOP|CU_TOPRIGHT;
			finish_row(h,h->cu_y-1);
		}
		else
			h->i_neighbor|=CU_LEFT;

		if(h->cu_y>=h->cu_height)
		{
			finish_row(h,h->cu_height);// finish alf
			lentoid_thread_report_progress((LentFrame*)h->current_picture_ptr,h->height-1);
			break;
		}
		if(get_cabac_terminate(&h->cabac))
		{
			break;
		}
	}

	return 0;
}
static lent_always_inline void idr(HEVCContext *h)
{
	lent_remove_all_refs(h);
}
static lent_always_inline void decode_postinit(HEVCContext *h,int report_setup)
{
	int i,pics,idx;
	int out_of_order=0;
	Picture *cur,*out;

	cur=h->current_picture_ptr;
	cur->reference|=DELAYED_PIC_REF;

	if(!h->dropable)
	{
		lent_ref_insert(h,cur);
	}

	for(pics=0;h->delayed_pic[pics];pics++);
	assert(pics<=MAX_DELAYED_PIC_COUNT);
	h->delayed_pic[pics++]=cur;

	out=h->delayed_pic[idx=0];
	for(i=1;i<pics;i++)//todo:找到poc最小的图像，poc机制调整后这段需要修改
	{
		if(h->delayed_pic[i]->idr)
			break;
		if(h->delayed_pic[i]->poc<out->poc)
		{
			out=h->delayed_pic[i];
			idx=i;
		}
	}

	if(out->poc<h->next_outputed_poc&&!out->idr)
		out_of_order=1;

	if(out_of_order)//todo
	{
	}

	if(pics>h->delay_length)
	{
		out->reference&=~DELAYED_PIC_REF;
		h->next_output_pic=out;
		h->next_outputed_poc=out->poc;//todo：如果poc回绕，需要在此处处理，以免out_of_order误判
		for(i=idx; h->delayed_pic[i]; i++)
			h->delayed_pic[i] = h->delayed_pic[i+1];
	}
	else
	{
		//lent_log(NULL,LENT_LOG_DEBUG,"no picture to output\n");
	}

	if(report_setup)//todo
	{
		lentoid_thread_finish_setup(h);
	}

}


/*use h->current.i_cbp instead
static lent_always_inline int dct_nonzero(void *start,int length)//因为这个函数目前只用在dct探测，至少是4x4=16byte
{
	__int64 *p=(__int64*)start;
	int i;
	for(i=0;i<length;i+=sizeof(*p))
		if(*(p++))
			return 1;
	return 0;
}
*/


static int decode_nal_units(HEVCContext *h, const uint8_t *buf, int buf_size){
	LentCodecContext * const ctx= h->ctx;
	int buf_index=0;
	int context_count = 0;

	h->current_picture_ptr= NULL;

	for(;;){
		int consumed;
		int dst_length;
		int bit_length;
		const uint8_t *ptr;
		int i, nalsize = 0;
		int err;

		{
			// start code prefix search
			for(; buf_index + 3 < buf_size; buf_index++){
				// This should always succeed in the first iteration.
				if(buf[buf_index] == 0 && buf[buf_index+1] == 0 && buf[buf_index+2] == 1)
					break;
			}

			if(buf_index+3 >= buf_size) break;

			buf_index+=3;
			//if(buf_index >= next_avc) continue;
		}

		ptr = extract_nal(h, buf + buf_index, &dst_length, &consumed, buf_size - buf_index);
		if (ptr==NULL || dst_length < 0){
			return -1;
		}
		i= buf_index + consumed;

		if(1){
			while(ptr[dst_length - 1] == 0 && dst_length > 0)
				dst_length--;
		}

		bit_length= !dst_length ? 0 : (8*dst_length - decode_rbsp_trailing(h, (uint8_t*)ptr + dst_length - 1));

		buf_index += consumed;

		err = 0;
		//lent_dlog(NULL,"Nal %d\n",h->nal_unit_type);

		switch(h->nal_unit_type){

		case NAL_UNIT_CODED_SLICE_IDR:
		case NAL_UNIT_CODED_SLICE_IDR_N_LP:
			idr(h);
		case NAL_UNIT_CODED_SLICE_TRAIL_R:
		case NAL_UNIT_CODED_SLICE_TRAIL_N:
		case NAL_UNIT_CODED_SLICE_TLA:
		case NAL_UNIT_CODED_SLICE_TSA_N:
		case NAL_UNIT_CODED_SLICE_STSA_R:
		case NAL_UNIT_CODED_SLICE_STSA_N:
		case NAL_UNIT_CODED_SLICE_BLA:
		case NAL_UNIT_CODED_SLICE_BLANT:
		case NAL_UNIT_CODED_SLICE_BLA_N_LP:
		case NAL_UNIT_CODED_SLICE_CRA:
		case NAL_UNIT_CODED_SLICE_DLP:
		case NAL_UNIT_CODED_SLICE_TFD:
			init_get_bits(&h->bs, ptr, bit_length);

			if((err = decode_slice_header(h)))
				break;

			decode_postinit(h,1);

			context_count++;
			decode_slice(h);//解码
			break;

		case NAL_UNIT_SPS:
			init_get_bits(&h->bs, ptr, bit_length);
			if(decode_sps(h)<0)
				return -1;
			
			break;

		case NAL_UNIT_PPS:
			init_get_bits(&h->bs, ptr, bit_length);
			if(decode_pps(h))
				return -1;

			break;
		case NAL_UNIT_VPS:
			init_get_bits(&h->bs, ptr, bit_length);
			if(h->ctx->compatibility>=92)
			{
				if(decode_vps(h))
					return -1;
			}
			else
			{
				if(decode_vps_091(h))
					return -1;
			}

			break;
		case NAL_UNIT_ACCESS_UNIT_DELIMITER:
		case NAL_UNIT_EOS:
		case NAL_UNIT_EOB:
		case NAL_UNIT_FILLER_DATA:
			break;
		default:
			lent_dlog(ctx, "Unknown NAL code: %d (%d bits)\n", h->nal_unit_type, bit_length);
		}

		if (err < 0)
			lent_log(h->ctx, LENT_LOG_ERROR, "decode_slice_header error\n");
	}

	return buf_index;
}

int decode_frame(HEVCContext *h,
				 void *data, int *data_size)
{

	LentPacket *pkt=h->pkt;
	const uint8_t *buf = pkt->data;
	int buf_size = pkt->size;
	//HEVCContext *h = ctx->hevc_ctx;
	LentFrame *pict = (LentFrame*)data;
	int buf_index=0;

out:
	//endofstream, flush pics in buffer
	if(!buf_size)
	{
		Picture *out;
		int i, out_idx;

		h->current_picture_ptr = NULL;

		out = h->delayed_pic[0];
		out_idx = 0;
		for(i=1; h->delayed_pic[i]&&!h->delayed_pic[i]->idr; i++)
		{
			if(h->delayed_pic[i]->poc < out->poc){
				out = h->delayed_pic[i];
				out_idx = i;
			}
		}

		for(i=out_idx; h->delayed_pic[i]; i++)
			h->delayed_pic[i] = h->delayed_pic[i+1];

		if(out){
			*data_size = sizeof(LentFrame);
			*pict= *(LentFrame*)out;
		}

		return 0;
	}

	//do decoding
	//lent_log(NULL, LENT_LOG_ERROR, "Enter decode_nal_units\n");
	buf_index=decode_nal_units(h, buf, buf_size);
	//lent_log(NULL, LENT_LOG_ERROR, "Leave decode_nal_units\n");

	//遇到ENDOFSEQUENCE包，则转到flush状态
	if(!h->current_picture_ptr && h->nal_unit_type == NAL_UNIT_EOS)
	{
		buf_size = 0;
		goto out;
	}

	//无法解码一帧，报错
	if(!h->current_picture_ptr)
	{
		lent_log(NULL, LENT_LOG_ERROR, "No frame! buf_size %d\n", buf_size);
		return -1;
	}

	//成功解码一帧，准备输出
	if(h->cu_y >= h->cu_height && h->cu_height)
	{

		h->current_picture_ptr->height=h->height;
		h->current_picture_ptr->width=h->width;

		lent_picture_end(h);
		if (!h->next_output_pic)
		{
			*data_size = 0;
		}
		else
		{
			*data_size = sizeof(LentFrame);
			*pict= *(LentFrame*)h->next_output_pic;
		}
	}

	assert(pict->data[0] || !*data_size);

	return get_consumed_bytes(h, buf_index, buf_size);

}
void hevc_flush_dpb(HEVCContext *h)
{
	int i;

	for(i=0; i<MAX_DELAYED_PIC_COUNT; i++) {
		if(h->delayed_pic[i])
			h->delayed_pic[i]->reference= 0;
		h->delayed_pic[i]= NULL;
	}
	h->next_outputed_poc=INT_MIN;
	idr(h);
	if(h->current_picture_ptr)
		h->current_picture_ptr->reference=0;
	h->current_picture_ptr=NULL;
	for(i=0;i<h->picture_count;i++)
		if(h->picture[i].data[0])
			lent_picture_release_buffer(h,&h->picture[i]);
}