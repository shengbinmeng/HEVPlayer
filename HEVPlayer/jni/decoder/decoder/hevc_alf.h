//**********************************************
//hevc_alf.h
//Unipipy @2011
//adaptive loopfilter
//**********************************************
#ifndef DECODER_HEVC_ALF_H
#define DECODER_HEVC_ALF_H

#define NO_TEST_FILT		3
#define NO_VAR_BIN				16
#define NUM_BITS				9

#define ALF_MAX_NUM_COEF	42
#define ALF_MAX_NUM_COEF_C	14
#define ALF_NUM_BIT_SHIFT     8                                       ///< bit shift parameter for quantization of ALF param.
#define ALF_ROUND_OFFSET      ( 1 << ( ALF_NUM_BIT_SHIFT - 1 ) )      ///< rounding offset for ALF quantization


// Shape0
static const int depthIntShape0Sym[10] = 
{
  1,    3,    1,
     3, 4, 3, 
  3, 4, 5, 5                 
};
// Shape1
static const int depthIntShape1Sym[9] = 
{
                 9,
                10,
  6, 7, 8, 9,10,11,11                        
};

static const int* const pDepthIntTabShapes[NO_TEST_FILT] =
{ 
  depthIntShape0Sym, depthIntShape1Sym
};


typedef struct ALFParam
{
	int alf_flag;                           ///< indicates use of ALF
	int cu_control_flag;                    ///< coding unit based control flag
	int chroma_idc;                         ///< indicates use of ALF for chroma

	int num_coeff;                          ///< number of filter coefficients
	int coeff[ALF_MAX_NUM_COEF];                             ///< filter coefficient array


	int realfiltNo_chroma;                  ///< index of filter shape (chroma)

	int num_coeff_chroma;                   ///< number of filter coefficients (chroma)
	int coeff_chroma[ALF_MAX_NUM_COEF_C];                      ///< filter coefficient array (chroma)
	//CodeAux related
	int realfiltNo;
	int filtNo;


	int filterPattern[NO_VAR_BIN];


	int startSecondFilter;
	int noFilters;


	int varIndTab[NO_VAR_BIN];

	//Coeff send related
	int filters_per_group_diff; //this can be updated using codedVarBins
	int filters_per_group;

	int codedVarBins[NO_VAR_BIN]; 

	int forceCoeff0;
	int predMethod;
	int coeffmulti[NO_VAR_BIN][ALF_MAX_NUM_COEF];
	int minKStart;
	int maxScanVal;
	int kMinTab[42];
	unsigned int num_alf_cu_flag;
	unsigned int num_cus_in_frame;
	unsigned int alf_max_depth;
	unsigned int *alf_cu_flag;
	unsigned int *p_alf_cu_flag;
	int alf_cu_flag_size;

	int alf_pcr_region_flag; 
}ALFParam;



void lent_decode_filter_qc(HEVCContext *h);

int lent_decode_alfparam_cavlc(HEVCContext *h,ALFParam *alf);
int lent_decode_alfparam_cabac(HEVCContext *h,ALFParam *alf);
void lent_decode_alfctrlparam(HEVCContext *h, ALFParam* pAlfParam);

void lent_copy_alfparam(ALFParam *dst,ALFParam *src);

void lent_free_alfparam(ALFParam **ppalf);
void lent_alloc_alfparam(ALFParam **ppalf,int cu_num);
int lent_alf_Scabac(HEVCContext *h);

void lent_alf_row(HEVCContext *h,int cu_y);

#endif