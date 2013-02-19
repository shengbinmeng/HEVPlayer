
#include "hevc.h"
#include "hevc_cabac.h"
const uint8_t LENT_cabac_context_init_I_091[MAX_CABAC_STATE] = 
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
	224,  167,  122, 

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
	160,
	
	//OFS_TRANSFORMSKIP_FLAG_CTX
	139,  139,

	//CU_TRANSQUANT_BYPASS_FLAG_CTX
	154,

};

const uint8_t LENT_cabac_context_init_P_091[MAX_CABAC_STATE] = 
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

const uint8_t LENT_cabac_context_init_B_091[MAX_CABAC_STATE] = 
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
	153,  138,  138,

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
	200,
	
	//OFS_TRANSFORMSKIP_FLAG_CTX
	139,  139,

	//CU_TRANSQUANT_BYPASS_FLAG_CTX
	154,
};

int decode_sps_ptl(HEVCContext *h, PTL *rpcPTL, Bool profilePresentFlag, Int maxNumSubLayersMinus1 );
int decode_vps_091(HEVCContext *h)
{
	UInt  uiCode;
	UInt i,opIdx;
	VPS *pcVPS;
	UInt subLayerOrderingInfoPresentFlag;
														  READ_CODE(  4, uiCode, "video_parameter_set_id" );
	if(h->vps_buffers[uiCode]==NULL)
	{
		h->vps_buffers[uiCode]=(VPS*)lent_mallocz(sizeof(VPS));
	}
	pcVPS=h->vps_buffers[uiCode];
	
	pcVPS->m_bTemporalIdNestingFlag						= READ_FLAG(     uiCode, "vps_temporal_id_nesting_flag" );
														  READ_CODE(  2, uiCode, "vps_reserved_three_2bits" );         assert(uiCode == 3);
														  READ_CODE(  6, uiCode, "vps_reserved_zero_6bits" );          assert(uiCode == 0);

	pcVPS->m_uiMaxTLayers								= READ_CODE(  3, uiCode, "vps_max_sub_layers_minus1" ) + 1;
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
	pcVPS->m_numHrdParameters							= READ_UVLC(     uiCode, "vps_num_hrd_parameters" );
	pcVPS->m_maxNuhReservedZeroLayerId					= READ_CODE(  6, uiCode, "vps_max_nuh_reserved_zero_layer_id" );
	assert( pcVPS->m_numHrdParameters < MAX_VPS_NUM_HRD_PARAMETERS_ALLOWED_PLUS1 );
	assert( pcVPS->m_maxNuhReservedZeroLayerId < MAX_VPS_NUH_RESERVED_ZERO_LAYER_ID_PLUS1 );
	for( opIdx = 0; opIdx < pcVPS->m_numHrdParameters; opIdx++ )
	{
		if( opIdx > 0 )
		{
			// operation_point_layer_id_flag( opIdx )
			for( i = 0; i <= pcVPS->m_maxNuhReservedZeroLayerId; i++ )
			{
				pcVPS->m_opLayerIdIncludedFlag[opIdx][i]= READ_FLAG(     uiCode, "op_layer_id_included_flag[opIdx][i]" );
			}
		}
		// TODO: add hrd_parameters()
	}
	// hrd_parameters
	READ_FLAG(     uiCode,  "vps_extension_flag" );              assert(!uiCode);
	return 0;
}