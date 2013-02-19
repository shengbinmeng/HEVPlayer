//**********************************************
//hevc_alf.c
//Unipipy @2011
//adaptive loopfilter
//**********************************************
#include "smmintrin.h"
#include "hevc.h"
#include "hevc_alf.h"


static lent_always_inline int lent_alfGolomb_cavlc(HEVCContext *h,int k)
{
	uint8_t uiSymbol;
	int q = -1;
	int nr = 0;
	int a;

	uiSymbol = 1;
	while (uiSymbol)
	{
		//m_pcEntropyDecoderIf->parseAlfFlag(uiSymbol);
		uiSymbol=get_1bit(&h->bs);
		q++;
	}
	for(a = 0; a < k; ++a)          // read out the sequential log2(M) bits
	{
		//m_pcEntropyDecoderIf->parseAlfFlag(uiSymbol);
		uiSymbol=get_1bit(&h->bs);
		if(uiSymbol)
			nr += 1 << a;
	}
	nr += q << k;
	if(nr != 0)
	{
		//m_pcEntropyDecoderIf->parseAlfFlag(uiSymbol);
		uiSymbol=get_1bit(&h->bs);
		nr = (uiSymbol)? nr: -nr;
	}
	return nr;
}

static lent_always_inline int lent_alfGolomb_cabac(HEVCContext *h,int k)
{
	uint8_t uiSymbol;
	int q = -1;
	int nr = 0;
	int a;

	uiSymbol = 1;
	while (uiSymbol)
	{
		//m_pcEntropyDecoderIf->parseAlfFlag(uiSymbol);
		uiSymbol=get_cabac(&h->cabac,&h->cabac.cabac_state[56]);
		q++;
	}
	for(a = 0; a < k; ++a)          // read out the sequential log2(M) bits
	{
		//m_pcEntropyDecoderIf->parseAlfFlag(uiSymbol);
		uiSymbol=get_cabac(&h->cabac,&h->cabac.cabac_state[56]);
		if(uiSymbol)
			nr += 1 << a;
	}
	nr += q << k;
	if(nr != 0)
	{
		//m_pcEntropyDecoderIf->parseAlfFlag(uiSymbol);
		uiSymbol=get_cabac(&h->cabac,&h->cabac.cabac_state[56]);
		nr = (uiSymbol)? nr: -nr;
	}
	return nr;
}
int lent_decode_alfparam_cavlc(HEVCContext *h,ALFParam *alf)
{
	int i;
	int sqrFiltLengthTab[2] = {10, 9};

	alf->alf_flag=get_1bit(&h->bs);
	if(!alf->alf_flag)
		return 0;
	{//decode aux

		alf->filters_per_group=0;
		memset(alf->filterPattern, 0 , sizeof(alf->filterPattern));
		alf->filtNo=1; //nonZeroCoeffs

		alf->alf_pcr_region_flag=get_1bit(&h->bs);

		alf->realfiltNo=get_ue(&h->bs);

		alf->num_coeff = sqrFiltLengthTab[alf->realfiltNo];


		if (alf->filtNo>=0)
		{
			if(alf->realfiltNo >= 0)
			{
				// filters_per_fr
				alf->noFilters = get_ue(&h->bs)+1;

				alf->filters_per_group = alf->noFilters; 

				if(alf->noFilters == 2)
				{
					alf->startSecondFilter = get_ue(&h->bs);
					alf->filterPattern [alf->startSecondFilter] = 1;
				}
				else if (alf->noFilters > 2)
				{
					alf->filters_per_group = 1;
					for (i=1; i<NO_VAR_BIN; i++) 
					{
						alf->filterPattern[i] = get_1bit(&h->bs);
						alf->filters_per_group += alf->filterPattern[i];
					}
				}
			}
		}
		else
		{
			memset (alf->filterPattern, 0, sizeof(alf->filterPattern));
		}
		// Reconstruct varIndTab[]
		memset(alf->varIndTab, 0, sizeof(alf->varIndTab));
		if(alf->filtNo>=0)
		{
			for(i = 1; i < NO_VAR_BIN; ++i)
			{
				if(alf->filterPattern[i])
					alf->varIndTab[i] = alf->varIndTab[i-1] + 1;
				else
					alf->varIndTab[i] = alf->varIndTab[i-1];
			}
		}
	}
	{//decode Filt

		if (alf->filtNo >= 0)
		{
			alf->filters_per_group_diff = alf->filters_per_group;
			if (alf->filters_per_group > 1)
			{
				alf->forceCoeff0 = 0;
				{
					for (i=0; i<NO_VAR_BIN; i++)
						alf->codedVarBins[i] = 1;

				}
				alf->predMethod = get_1bit(&h->bs);
			}
			else
			{
				alf->forceCoeff0 = 0;
				alf->predMethod = 0;
			}
			{//decodeFilterCoeff (alf);
				{//readFilterCodingParams
					int ind, scanPos;
					int golombIndexBit;
					int kMin;
					int maxScanVal;
					const int *pDepthInt;

					// Determine maxScanVal
					maxScanVal = 0;
					pDepthInt = pDepthIntTabShapes[alf->realfiltNo];

					for(ind = 0; ind < alf->num_coeff; ind++)
						maxScanVal = max(maxScanVal, pDepthInt[ind]);

					// Golomb parameters
					alf->minKStart = 1 + get_ue(&h->bs);

					kMin = alf->minKStart;
					for(scanPos = 0; scanPos < maxScanVal; scanPos++)
					{
						golombIndexBit = get_1bit(&h->bs);
						if(golombIndexBit)
							alf->kMinTab[scanPos] = kMin + 1;
						else
							alf->kMinTab[scanPos] = kMin;
						kMin = alf->kMinTab[scanPos];
					}
				}
				{//readFilterCoeffs
					int ind, scanPos;
					const int *pDepthInt;

					pDepthInt = pDepthIntTabShapes[alf->realfiltNo];

					for(ind = 0; ind < alf->filters_per_group_diff; ++ind)
					{
						for(i = 0; i < alf->num_coeff; i++)
						{
							scanPos = pDepthInt[i] - 1;
							alf->coeffmulti[ind][i] = lent_alfGolomb_cavlc(h,alf->kMinTab[scanPos]);
						}
					}

				}
			}
		}
	}

	alf->chroma_idc=get_ue(&h->bs);
	if(alf->chroma_idc)
	{
		int pos;
		alf->realfiltNo_chroma = get_ue(&h->bs);
		alf->num_coeff_chroma = sqrFiltLengthTab[alf->realfiltNo_chroma];

		// filter coefficients for chroma
		for(pos=0; pos<alf->num_coeff_chroma; pos++)
		{
			alf->coeff_chroma[pos] = get_se(&h->bs);
		}
	}
	return 0;

}

static int lent_alf_Scabac(HEVCContext *h)
{
	uint32_t uiCode;
	int32_t  iSign;
	int32_t  i,riVal;

	//m_pcTDecBinIf->decodeBin( uiCode, m_cALFSvlcSCModel.get( 0, 0, 0 ) );
	uiCode=get_cabac(&h->cabac,&h->cabac.cabac_state[59]);

	if ( uiCode == 0 )
	{
		riVal = 0;
		return riVal;
	}

	// read sign
	//m_pcTDecBinIf->decodeBin( uiCode, m_cALFSvlcSCModel.get( 0, 0, 1 ) );
	uiCode=get_cabac(&h->cabac,&h->cabac.cabac_state[60]);

	if ( uiCode == 0 ) iSign =  1;
	else               iSign = -1;

	// read magnitude
	i=1;
	while (1)
	{
		//m_pcTDecBinIf->decodeBin( uiCode, m_cALFSvlcSCModel.get( 0, 0, 2 ) );
		uiCode=get_cabac(&h->cabac,&h->cabac.cabac_state[61]);
		if ( uiCode == 0 ) break;
		i++;
	}

	riVal = i*iSign;
	return riVal;
}
int lent_decode_alfparam_cabac(HEVCContext *h,ALFParam *alf)
{
	int i;
	int sqrFiltLengthTab[2] = {10, 9};

	alf->alf_flag=get_cabac(&h->cabac,&h->cabac.cabac_state[56]);
	if(!alf->alf_flag)
		return 0;
	{//decode aux

		alf->filters_per_group=0;
		memset(alf->filterPattern, 0 , sizeof(alf->filterPattern));
		alf->filtNo=1; //nonZeroCoeffs

		alf->alf_pcr_region_flag=get_cabac(&h->cabac,&h->cabac.cabac_state[56]);

		if(get_cabac(&h->cabac,&h->cabac.cabac_state[57]))
		{
			alf->realfiltNo=1;
			while(get_cabac(&h->cabac,&h->cabac.cabac_state[58]))
				alf->realfiltNo++;
		}
		else
		{
			alf->realfiltNo=0;
		}

		alf->num_coeff = sqrFiltLengthTab[alf->realfiltNo];


		if (alf->filtNo>=0)
		{
			if(alf->realfiltNo >= 0)
			{
				// filters_per_fr
				if(get_cabac(&h->cabac,&h->cabac.cabac_state[57]))
				{
					alf->noFilters=1;
					while(get_cabac(&h->cabac,&h->cabac.cabac_state[58]))
						alf->noFilters++;
				}
				else
				{
					alf->noFilters=0;
				}
				alf->noFilters++;

				alf->filters_per_group = alf->noFilters; 

				if(alf->noFilters == 2)
				{
					if(get_cabac(&h->cabac,&h->cabac.cabac_state[57]))
					{
						alf->startSecondFilter=1;
						while(get_cabac(&h->cabac,&h->cabac.cabac_state[58]))
							alf->startSecondFilter++;
					}
					else
					{
						alf->startSecondFilter=0;
					}
					alf->filterPattern [alf->startSecondFilter] = 1;
				}
				else if (alf->noFilters > 2)
				{
					alf->filters_per_group = 1;
					for (i=1; i<NO_VAR_BIN; i++) 
					{
						alf->filterPattern[i] = get_cabac(&h->cabac,&h->cabac.cabac_state[56]);
						alf->filters_per_group += alf->filterPattern[i];
					}
				}
			}
		}
		else
		{
			memset (alf->filterPattern, 0, sizeof(alf->filterPattern));
		}
		// Reconstruct varIndTab[]
		memset(alf->varIndTab, 0, sizeof(alf->varIndTab));
		if(alf->filtNo>=0)
		{
			for(i = 1; i < NO_VAR_BIN; ++i)
			{
				if(alf->filterPattern[i])
					alf->varIndTab[i] = alf->varIndTab[i-1] + 1;
				else
					alf->varIndTab[i] = alf->varIndTab[i-1];
			}
		}
	}
	{//decode Filt

		if (alf->filtNo >= 0)
		{
			alf->filters_per_group_diff = alf->filters_per_group;
			if (alf->filters_per_group > 1)
			{
				alf->forceCoeff0 = 0;
				{
					for (i=0; i<NO_VAR_BIN; i++)
						alf->codedVarBins[i] = 1;

				}
				alf->predMethod = get_cabac(&h->cabac,&h->cabac.cabac_state[56]);
			}
			else
			{
				alf->forceCoeff0 = 0;
				alf->predMethod = 0;
			}
			{//decodeFilterCoeff (alf);
				{//readFilterCodingParams
					int ind, scanPos;
					int golombIndexBit;
					int kMin;
					int maxScanVal;
					const int *pDepthInt;

					// Determine maxScanVal
					maxScanVal = 0;
					pDepthInt = pDepthIntTabShapes[alf->realfiltNo];

					for(ind = 0; ind < alf->num_coeff; ind++)
						maxScanVal = max(maxScanVal, pDepthInt[ind]);

					// Golomb parameters
					if(get_cabac(&h->cabac,&h->cabac.cabac_state[57]))
					{
						alf->minKStart=1;
						while(get_cabac(&h->cabac,&h->cabac.cabac_state[58]))
							alf->minKStart++;
					}
					else
					{
						alf->minKStart=0;
					}
					alf->minKStart ++;

					kMin = alf->minKStart;
					for(scanPos = 0; scanPos < maxScanVal; scanPos++)
					{
						golombIndexBit = get_cabac(&h->cabac,&h->cabac.cabac_state[56]);
						if(golombIndexBit)
							alf->kMinTab[scanPos] = kMin + 1;
						else
							alf->kMinTab[scanPos] = kMin;
						kMin = alf->kMinTab[scanPos];
					}
				}
				{//readFilterCoeffs
					int ind, scanPos;
					const int *pDepthInt;

					pDepthInt = pDepthIntTabShapes[alf->realfiltNo];

					for(ind = 0; ind < alf->filters_per_group_diff; ++ind)
					{
						for(i = 0; i < alf->num_coeff; i++)
						{
							scanPos = pDepthInt[i] - 1;
							alf->coeffmulti[ind][i] = lent_alfGolomb_cabac(h,alf->kMinTab[scanPos]);
						}
					}

				}
			}
		}
	}
	if(get_cabac(&h->cabac,&h->cabac.cabac_state[57]))
	{
		alf->chroma_idc=1;
		while(get_cabac(&h->cabac,&h->cabac.cabac_state[58]))
			alf->chroma_idc++;
	}
	else
	{
		alf->chroma_idc=0;
	}
	if(alf->chroma_idc)
	{
		int pos;
		if(get_cabac(&h->cabac,&h->cabac.cabac_state[57]))
		{
			alf->realfiltNo_chroma=1;
			while(get_cabac(&h->cabac,&h->cabac.cabac_state[58]))
				alf->realfiltNo_chroma++;
		}
		else
		{
			alf->realfiltNo_chroma=0;
		}
		alf->num_coeff_chroma = sqrFiltLengthTab[alf->realfiltNo_chroma];

		// filter coefficients for chroma
		for(pos=0; pos<alf->num_coeff_chroma; pos++)
		{
			alf->coeff_chroma[pos] = lent_alf_Scabac(h);
		}
	}
	return 0;

}

void lent_copy_alfparam(ALFParam *dst,ALFParam *src)
{
	if(!src->alf_flag)
		return;
	{
		ALFParam tmp=*src;

		tmp.alf_cu_flag=dst->alf_cu_flag;
		memcpy(dst->alf_cu_flag,src->alf_cu_flag,sizeof(*src->alf_cu_flag)*src->alf_cu_flag_size);

		*dst=tmp;
	}
}

void lent_alloc_alfparam(ALFParam **ppalf,int cu_num)
{
	if(!*ppalf)
	{
		*ppalf=lent_mallocz(sizeof(ALFParam));
		(*ppalf)->alf_cu_flag_size=cu_num*(4096)*sizeof(*(*ppalf)->alf_cu_flag);//test
		(*ppalf)->alf_cu_flag=lent_malloc((*ppalf)->alf_cu_flag_size);
	}
	(*ppalf)->num_cus_in_frame=cu_num;

}
void lent_free_alfparam(ALFParam **ppalf)
{
	if(*ppalf)
	{
		lent_freep(&(*ppalf)->alf_cu_flag);
		lent_freep(ppalf);
	}
}

void lent_decode_alfctrlparam(HEVCContext *h, ALFParam* pAlfParam)
{
	uint32_t uiSymbol;
	int32_t iSymbol;
	uint32_t i;

	if(pAlfParam->alf_flag ==0)
	{
		return;
	} 
	pAlfParam->cu_control_flag = get_cabac(&h->cabac,&h->cabac.cabac_state[56]);
	if (pAlfParam->cu_control_flag)
	{
		//m_pcEntropyDecoderIf->setAlfCtrl(true);
		//m_pcEntropyDecoderIf->parseAlfCtrlDepth(uiSymbol);
		//m_pcEntropyDecoderIf->setMaxAlfCtrlDepth(uiSymbol);
		if(get_cabac(&h->cabac,&h->cabac.cabac_state[57]))
		{
			for(i=1;i<3;i++)
				if(!get_cabac(&h->cabac,&h->cabac.cabac_state[58]))
				{
					break;
				}
		}
		else
			i=0;
		pAlfParam->alf_max_depth = i;
	}
	else
	{
		//m_pcEntropyDecoderIf->setAlfCtrl(false);
		return;
	}

	//m_pcEntropyDecoderIf->parseAlfSvlc(iSymbol);
	iSymbol=lent_alf_Scabac(h);

	uiSymbol = iSymbol + pAlfParam->num_cus_in_frame;


	//if(bFirstSliceInPic)
	//{
	pAlfParam->num_alf_cu_flag = 0;
	//}
	//UInt uiSliceAlfFlagsPos = pAlfParam->num_alf_cu_flag;

	pAlfParam->num_alf_cu_flag += uiSymbol;


	for(i=0/*always from cu 0*/; i<pAlfParam->num_alf_cu_flag; i++)
	{
		//m_pcEntropyDecoderIf->parseAlfCtrlFlag( pAlfParam->alf_cu_flag[i] );
		pAlfParam->alf_cu_flag[i]=get_cabac(&h->cabac,&h->cabac.cabac_state[62]);
	}
	pAlfParam->p_alf_cu_flag=pAlfParam->alf_cu_flag;
}


static void reconstructFilterCoeffs(ALFParam* pcAlfParam,int pfilterCoeffSym[][MAX_SQR_FILT_LENGTH], int bit_depth)
{
	int i, ind;

	int m_filterCoeffSymTmp[NO_VAR_BIN][MAX_SQR_FILT_LENGTH];

	// Copy non zero filters in filterCoeffTmp
	for(ind = 0; ind < pcAlfParam->filters_per_group_diff; ++ind)
	{
		for(i = 0; i < pcAlfParam->num_coeff; i++)
			pfilterCoeffSym[ind][i] = pcAlfParam->coeffmulti[ind][i];
	}
	// Undo prediction
	for(ind = 0; ind < pcAlfParam->filters_per_group_diff; ++ind)
	{
		if((!pcAlfParam->predMethod) || (ind == 0)) 
		{
			memcpy(m_filterCoeffSymTmp[ind],pfilterCoeffSym[ind],sizeof(int)*pcAlfParam->num_coeff);
		}
		else
		{
			// Prediction
			for(i = 0; i < pcAlfParam->num_coeff; ++i)
				m_filterCoeffSymTmp[ind][i] = (int)(pfilterCoeffSym[ind][i] + m_filterCoeffSymTmp[ind - 1][i]);
		}
	}

	// Inverse quantization
	// Add filters forced to zero
	if(pcAlfParam->forceCoeff0)
	{
		assert(0);
	}
	else
	{
		assert(pcAlfParam->filters_per_group_diff == pcAlfParam->filters_per_group);
		for(ind = 0; ind < pcAlfParam->filters_per_group; ++ind)
			memcpy(pfilterCoeffSym[ind],m_filterCoeffSymTmp[ind],sizeof(int)*pcAlfParam->num_coeff);
	}
}
int patternShape0Sym_Quart[29] = 
{
	0,  0,  0,  1,  0,  2,  0,  3,  0,  0,  0,
	0,  0,  0,  0,  4,  5,  6,  0,  0,  0,  0,
	0,  0,  0,  7,  8,  9, 10
};
int patternShape1Sym_Quart[29] = 
{
	0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  2,  0,  0,  0,  0,  0,
	3,  4,  5,  6,  7,  8,  9
};

int* patternMapTabShapes[NO_TEST_FILT] =
{
	patternShape0Sym_Quart, patternShape1Sym_Quart
};

int weightsShape0Sym[10] = 
{
	2,    2,    2,    
	2, 2, 2,        
	2, 2, 1, 1
};
int weightsShape1Sym[9] = 
{                      
	2,
	2,
	2, 2, 2, 2, 2, 1, 1
};

int* weightsTabShapes[NO_TEST_FILT] =
{
	weightsShape0Sym, weightsShape1Sym
};
void getCurrentFilter(int alfcoeff_final[][MAX_SQR_FILT_LENGTH],int filterCoeffSym[][MAX_SQR_FILT_LENGTH],ALFParam* pcAlfParam)
{ 
	int i,  k, varInd;
	int *patternMap;
	{
		for(varInd=0; varInd<NO_VAR_BINS; ++varInd)
		{
			memset(alfcoeff_final[varInd],0,sizeof(int)*MAX_SQR_FILT_LENGTH);
		}
		patternMap=patternMapTabShapes[pcAlfParam->realfiltNo];

		for(varInd=0; varInd<NO_VAR_BINS; ++varInd)
		{
			k=0;
			for(i = 0; i < MAX_SQR_FILT_LENGTH; i++)
			{
				if (patternMap[i]>0)
				{
					alfcoeff_final[varInd][i]=filterCoeffSym[pcAlfParam->varIndTab[varInd]][k];
					k++;
				}
				else
				{
					alfcoeff_final[varInd][i]=0;
				}
			}
		}
	}
}
void lent_decode_filter_qc(HEVCContext *h)
{
	int i;
	int numBits = NUM_BITS; 
	ALFParam *pAlfParam=h->sh.alf;

	int (*pfilterCoeffSym)[MAX_SQR_FILT_LENGTH];
	pfilterCoeffSym= h->cache.alfcoeff;

	if(h->sh.alf->filtNo>=0)
	{
		//// Reconstruct filter coefficients
		reconstructFilterCoeffs( h->sh.alf, pfilterCoeffSym, numBits);
	}
	else
	{
		for(i = 0; i < NO_VAR_BIN; i++)
		{
			h->sh.alf->varIndTab[i]=0;
			memset(pfilterCoeffSym[i],0,sizeof(pfilterCoeffSym[i][0])*MAX_SQR_FILT_LENGTH);
		}
	}
	getCurrentFilter(h->cache.alfcoeff_final,h->cache.alfcoeff,h->sh.alf);

	{//predict chroma coeff
		int i, sum, pred, N;
		const int* pFiltMag = NULL;

		pFiltMag = weightsTabShapes[pAlfParam->realfiltNo_chroma];
		N = pAlfParam->num_coeff_chroma - 1;

		sum=0;
		for(i=0; i<N;i++)
		{
			sum+=pFiltMag[i]*pAlfParam->coeff_chroma[i];
		}
		pred=(1<<ALF_NUM_BIT_SHIFT)-(sum-pAlfParam->coeff_chroma[N-1]);
		pAlfParam->coeff_chroma[N-1]=pred-pAlfParam->coeff_chroma[N-1];
	}

}
#define ALF_SSE 1
void lent_alf_row(HEVCContext *h,int cu_y)
{
	ALFParam *alf=h->sh.alf;

	char var[16][16];
	int16_t tmp_hor[66][66],tmp_ver[66][66];
	int i,j,k,stride=h->current_picture.linesize[0],stridech=h->current_picture.linesize[1];
	int cu_x,direction,avg_var;
	int step1 = NO_VAR_BINS/3 - 1;
	int th[NO_VAR_BINS] = {0, 1, 2, 2, 2, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4}; 
	int var_max= NO_VAR_BINS-1;
	const int shift_h=2,shift_w=2;
	int numBitsMinus1= NUM_BITS-1;
	int offset = (1<<(NUM_BITS-2));
	int sqrFiltLengthTab[2] = {10, 9};
	int regionTable[NO_VAR_BINS] = {0, 1, 4, 5, 15, 2, 3, 6, 14, 11, 10, 7, 13, 12,  9,  8}; 

	int *coeff,tmppixel,idx,depth=alf->alf_max_depth,idxstep;

	pixel *p,*p_dst;

	__m128i d[8]={0},tmp,zero=_mm_setzero_si128(),shuffle={2,3,4,5,6,7,8,9,6,7,8,9,10,11,12,13};

	for(cu_x=0;cu_x<h->cu_width;cu_x++)
	{
		int width=LENTMIN_INT(h->width-cu_x*64,64),height=LENTMIN_INT(h->height-cu_y*64,64);

		if(!alf->alf_pcr_region_flag)
		{//calculate var
			p=h->cache.alf_pic.data[0]+((cu_y)*stride+cu_x)*64;
			for(i=0;i<height+1;i+=2)
			{
				for(j=0;j<width+1;j+=2)
				{
					tmp_hor[i+1][j+1]=LENTABS_INT(p[j]*2-p[j-1]-p[j+1]);
					tmp_ver[i+1][j+1]=LENTABS_INT(p[j]*2-p[j-stride]-p[j+stride]);
				}
				p+=2*stride;
			}

			for(i=1;i<height+1;i+=4)
			{
				for(j=1;j<width+1;j+=4)
				{
					tmp_hor[i-1][j-1]=tmp_hor[i][j]+tmp_hor[i][j+2]+tmp_hor[i+2][j]+tmp_hor[i+2][j+2];
					tmp_ver[i-1][j-1]=tmp_ver[i][j]+tmp_ver[i][j+2]+tmp_ver[i+2][j]+tmp_ver[i+2][j+2];
					direction = 0;
					if (tmp_ver[i-1][j-1] > 2*tmp_hor[i-1][j-1]) direction = 1; //vertical
					if (tmp_hor[i-1][j-1] > 2*tmp_ver[i-1][j-1]) direction = 2; //horizontal

					avg_var = (tmp_ver[i-1][j-1]+tmp_hor[i-1][j-1])>>2; // BA_SUB: average for 4 pixels
					avg_var = lent_clip(avg_var>>1,0,var_max);
					avg_var = th[avg_var];
					avg_var = lent_clip(avg_var,0,step1) + (step1+1)*direction;
					var[(i - 1)>>shift_h][(j - 1)>>shift_w] = avg_var;
				}
			}
		}
		else
		{
			//region mode;
			int xInterval = ((( (h->width+63)/64) + 1) / 4 * 64)>>2;  
			int yInterval = ((((h->height+63)/64) + 1) / 4 * 64)>>2;

			int index_x=h->cu_x*64/4/xInterval;
			int index_y=h->cu_y*64/4/xInterval;

			memset(var,regionTable[index_y*4+index_x],sizeof(var));
		}

		for(idx=0;idx<256;idx+=idxstep)
		{
			int x=(LENT_scan4[idx]&(32-1)-16)<<2;
			int y=((LENT_scan4[idx]-32)>>5)<<2;

			if(x+cu_x*64<h->width&&y+cu_y*64<h->height)//如果当前块在图像中
			{
			int loc_depth=LENTMIN(h->current.cu_depth[h->cu_width*cu_y+cu_x][idx],depth);

			int loc_width=width>>loc_depth;
			int loc_height=height>>loc_depth;

			idxstep=256/(1<<(loc_depth<<1));

			if((!alf->cu_control_flag)||(alf->cu_control_flag&&*alf->p_alf_cu_flag++))
			{

				{//luma
					pixel *p1,*p2,*p3,*p4;
					p=h->cache.alf_pic.data[0]+((cu_y)*stride+cu_x)*64;
					p_dst=h->current_picture.data[0]+((cu_y)*stride+cu_x)*64;

					p+=x+y*stride;
					p_dst+=x+y*stride;

					p1=p+stride;
					p2=p-stride;
					p3=p+stride*2;
					p4=p-stride*2;
					if(alf->realfiltNo==0)
					{

						for(i=0;i<loc_height;i++)
						{
							for(j=0;j<loc_width;j+=4)
							{
								//if((j&0x3)==0)
								{
									coeff=h->cache.alfcoeff_final[var[i>>2][j>>2]];
									
									d[0]=_mm_set1_epi32(coeff[MAX_SQR_FILT_LENGTH-1]+offset);
									d[1]=_mm_unpacklo_epi8(_mm_loadl_epi64(&p4[j-2]),zero);
									d[2]=_mm_unpacklo_epi8(_mm_loadl_epi64(&p2[j-2]),zero);
									d[3]=_mm_unpacklo_epi8(_mm_loadl_epi64(&p[j-2]),zero);
									d[4]=_mm_unpacklo_epi8(_mm_loadl_epi64(&p1[j-2]),zero);
									d[5]=_mm_unpacklo_epi8(_mm_loadl_epi64(&p3[j-2]),zero);
									
									d[6]=_mm_srli_si128(_mm_add_epi16(d[1],d[5]),4);
									d[7]=_mm_srli_si128(_mm_add_epi16(d[2],d[4]),4);
									tmp=_mm_shuffle_epi32(d[5],_MM_SHUFFLE(1,0,3,2));
									d[1]=_mm_add_epi16(d[1],tmp);
									tmp=_mm_shuffle_epi8(d[4],shuffle);
									tmp=_mm_shuffle_epi32(tmp,_MM_SHUFFLE(1,0,3,2));
									d[2]=_mm_shuffle_epi8(d[2],shuffle);
									d[2]=_mm_add_epi16(d[2],tmp);

								}
								//中轴
								tmp=_mm_set1_epi32(coeff[5]);
								tmp=_mm_mullo_epi32(tmp,_mm_unpacklo_epi16(d[6],zero));
								d[0]=_mm_add_epi32(d[0],tmp);
								
								tmp=_mm_set1_epi32(coeff[16]);
								tmp=_mm_mullo_epi32(tmp,_mm_unpacklo_epi16(d[7],zero));
								d[0]=_mm_add_epi32(d[0],tmp);



								//1st
								tmp=_mm_set1_epi32(coeff[3]);
								tmp=_mm_mullo_epi32(tmp,_mm_unpacklo_epi16(d[1],zero));
								d[0]=_mm_add_epi32(d[0],tmp);

								d[1]=_mm_srli_si128(d[1],8);
								tmp=_mm_set1_epi32(coeff[7]);
								tmp=_mm_mullo_epi32(tmp,_mm_unpacklo_epi16(d[1],zero));
								d[0]=_mm_add_epi32(d[0],tmp);

								//2nd
								tmp=_mm_set1_epi32(coeff[15]);
								tmp=_mm_mullo_epi32(tmp,_mm_unpacklo_epi16(d[2],zero));
								d[0]=_mm_add_epi32(d[0],tmp);

								d[2]=_mm_srli_si128(d[2],8);
								tmp=_mm_set1_epi32(coeff[17]);
								tmp=_mm_mullo_epi32(tmp,_mm_unpacklo_epi16(d[2],zero));
								d[0]=_mm_add_epi32(d[0],tmp);


								//3rd
								tmp=_mm_set1_epi32(coeff[25]);
								tmp=_mm_mullo_epi32(tmp,_mm_unpacklo_epi16(d[3],zero));
								d[0]=_mm_add_epi32(d[0],tmp);
								d[3]=_mm_srli_si128(d[3],2);
								tmp=_mm_set1_epi32(coeff[26]);
								tmp=_mm_mullo_epi32(tmp,_mm_unpacklo_epi16(d[3],zero));
								d[0]=_mm_add_epi32(d[0],tmp);
								d[3]=_mm_srli_si128(d[3],2);
								tmp=_mm_set1_epi32(coeff[27]);
								tmp=_mm_mullo_epi32(tmp,_mm_unpacklo_epi16(d[3],zero));
								d[0]=_mm_add_epi32(d[0],tmp);
								d[3]=_mm_srli_si128(d[3],2);
								tmp=_mm_set1_epi32(coeff[26]);
								tmp=_mm_mullo_epi32(tmp,_mm_unpacklo_epi16(d[3],zero));
								d[0]=_mm_add_epi32(d[0],tmp);
								d[3]=_mm_srli_si128(d[3],2);
								tmp=_mm_set1_epi32(coeff[25]);
								tmp=_mm_mullo_epi32(tmp,_mm_unpacklo_epi16(d[3],zero));
								d[0]=_mm_add_epi32(d[0],tmp);

								d[0]=_mm_srai_epi32(d[0],numBitsMinus1);
								d[0]=_mm_packus_epi32(d[0],d[0]);
								d[0]=_mm_packus_epi16(d[0],d[0]);
								//_mm_storel_epi64(&p_dst[i*stride + j],d[0]);
								((int*)&p_dst[i*stride + j])[0]=d[0].m128i_i32[0];


							}
							p+=stride;
							p1+=stride;
							p2+=stride;
							p3+=stride;
							p4+=stride;
						}
					}
					else if(alf->realfiltNo==1)
					{
						for(i=0;i<loc_height;i++)
						{
							for(j=0;j<loc_width;j+=4)
							{
								//if((j&0x3)==0)
								{
									coeff=h->cache.alfcoeff_final[var[i>>2][j>>2]];

									d[0]=_mm_set1_epi32(coeff[MAX_SQR_FILT_LENGTH-1]+offset);
									d[1]=_mm_unpacklo_epi8(_mm_loadl_epi64(&p3[j]),zero);
									d[2]=_mm_unpacklo_epi8(_mm_loadl_epi64(&p1[j]),zero);
									d[3]=_mm_unpacklo_epi8(_mm_loadl_epi64(&p[j-5]),zero);
									d[6]=_mm_unpacklo_epi8(_mm_loadl_epi64(&p[j]),zero);
									d[7]=_mm_unpacklo_epi8(_mm_loadl_epi64(&p[j+1]),zero);
									d[4]=_mm_unpacklo_epi8(_mm_loadl_epi64(&p2[j]),zero);
									d[5]=_mm_unpacklo_epi8(_mm_loadl_epi64(&p4[j]),zero);


									d[1]=_mm_add_epi16(d[1],d[5]);
									d[2]=_mm_add_epi16(d[2],d[4]);
								}

								//1st
								tmp=_mm_set1_epi32(coeff[5]);
								tmp=_mm_mullo_epi32(tmp,_mm_unpacklo_epi16(d[1],zero));
								d[0]=_mm_add_epi32(d[0],tmp);

								//2nd
								tmp=_mm_set1_epi32(coeff[16]);
								tmp=_mm_mullo_epi32(tmp,_mm_unpacklo_epi16(d[2],zero));
								d[0]=_mm_add_epi32(d[0],tmp);

								//3rd left 5
								tmp=_mm_set1_epi32(coeff[22]);
								tmp=_mm_mullo_epi32(tmp,_mm_unpacklo_epi16(d[3],zero));
								d[0]=_mm_add_epi32(d[0],tmp);
								d[3]=_mm_srli_si128(d[3],2);
								tmp=_mm_set1_epi32(coeff[23]);
								tmp=_mm_mullo_epi32(tmp,_mm_unpacklo_epi16(d[3],zero));
								d[0]=_mm_add_epi32(d[0],tmp);
								d[3]=_mm_srli_si128(d[3],2);
								tmp=_mm_set1_epi32(coeff[24]);
								tmp=_mm_mullo_epi32(tmp,_mm_unpacklo_epi16(d[3],zero));
								d[0]=_mm_add_epi32(d[0],tmp);
								d[3]=_mm_srli_si128(d[3],2);
								tmp=_mm_set1_epi32(coeff[25]);
								tmp=_mm_mullo_epi32(tmp,_mm_unpacklo_epi16(d[3],zero));
								d[0]=_mm_add_epi32(d[0],tmp);
								d[3]=_mm_srli_si128(d[3],2);
								tmp=_mm_set1_epi32(coeff[26]);
								tmp=_mm_mullo_epi32(tmp,_mm_unpacklo_epi16(d[3],zero));
								d[0]=_mm_add_epi32(d[0],tmp);

								//3rd middle
								tmp=_mm_set1_epi32(coeff[27]);
								tmp=_mm_mullo_epi32(tmp,_mm_unpacklo_epi16(d[6],zero));
								d[0]=_mm_add_epi32(d[0],tmp);

								//3rd right

								tmp=_mm_set1_epi32(coeff[26]);
								tmp=_mm_mullo_epi32(tmp,_mm_unpacklo_epi16(d[7],zero));
								d[0]=_mm_add_epi32(d[0],tmp);
								d[7]=_mm_srli_si128(d[7],2);
								tmp=_mm_set1_epi32(coeff[25]);
								tmp=_mm_mullo_epi32(tmp,_mm_unpacklo_epi16(d[7],zero));
								d[0]=_mm_add_epi32(d[0],tmp);
								d[7]=_mm_srli_si128(d[7],2);
								tmp=_mm_set1_epi32(coeff[24]);
								tmp=_mm_mullo_epi32(tmp,_mm_unpacklo_epi16(d[7],zero));
								d[0]=_mm_add_epi32(d[0],tmp);
								d[7]=_mm_srli_si128(d[7],2);
								tmp=_mm_set1_epi32(coeff[23]);
								tmp=_mm_mullo_epi32(tmp,_mm_unpacklo_epi16(d[7],zero));
								d[0]=_mm_add_epi32(d[0],tmp);
								d[7]=_mm_srli_si128(d[7],2);
								tmp=_mm_set1_epi32(coeff[22]);
								tmp=_mm_mullo_epi32(tmp,_mm_unpacklo_epi16(d[7],zero));
								d[0]=_mm_add_epi32(d[0],tmp);

								d[0]=_mm_srai_epi32(d[0],numBitsMinus1);
								d[0]=_mm_packus_epi32(d[0],d[0]);
								d[0]=_mm_packus_epi16(d[0],d[0]);
								//_mm_storel_epi64(&p_dst[i*stride + j],d[0]);
								((int*)&p_dst[i*stride + j])[0]=d[0].m128i_i32[0];

							}
							p+=stride;
							p1+=stride;
							p2+=stride;
							p3+=stride;
							p4+=stride;

						}
					}
					else
					{
						assert(0);//other filtNo is not supported
					}
				}
				loc_width>>=1;
				loc_height>>=1;
				for(k=0;k<2;k++)
				{
					if(alf->chroma_idc&(1<<(1-k)))
					{
						int *qh=alf->coeff_chroma;
						int N=sqrFiltLengthTab[alf->realfiltNo_chroma] - 1;
						pixel *p1,*p2,*p3,*p4;
						p=h->cache.alf_pic.data[k+1]+((cu_y)*stridech+cu_x)*32;
						p_dst=h->current_picture.data[k+1]+((cu_y)*stridech+cu_x)*32;

						p+=(x+y*stridech)/2;
						p_dst+=(x+y*stridech)/2;


						p1=p+stridech;
						p2=p-stridech;
						p3=p+stridech*2;
						p4=p-stridech*2;
						if(alf->realfiltNo_chroma==0)
						{
							for(i=0;i<loc_height;i++)
							{
								for(j=0;j<loc_width;j+=4)
								{
									/*
									tmppixel  = 0;
									tmppixel += qh[0] * (p3[j+2] + p4[j-2]);
									tmppixel += qh[1] * (p3[j]   + p4[j]);
									tmppixel += qh[2] * (p3[j-2] + p4[j+2]);

									tmppixel += qh[3] * (p1[j+1] + p2[j-1]);
									tmppixel += qh[4] * (p1[j]   + p2[j]);
									tmppixel += qh[5] * (p1[j-1] + p2[j+1]);

									tmppixel += qh[6] * (p[j+2]     + p[j-2]);
									tmppixel += qh[7] * (p[j+1]     + p[j-1]);
									tmppixel += qh[8] * (p[j]);

									// DC offset
									tmppixel += qh[N] << 0;
									tmppixel = (tmppixel + ALF_ROUND_OFFSET)>>ALF_NUM_BIT_SHIFT;
									p_dst[i*stridech + j]=lent_clip_uint8(tmppixel);
									*/


								{
									coeff=qh;
									
									d[0]=_mm_set1_epi32(qh[N]+ALF_ROUND_OFFSET);
									d[1]=_mm_unpacklo_epi8(_mm_loadl_epi64(&p4[j-2]),zero);
									d[2]=_mm_unpacklo_epi8(_mm_loadl_epi64(&p2[j-2]),zero);
									d[3]=_mm_unpacklo_epi8(_mm_loadl_epi64(&p[j-2]),zero);
									d[4]=_mm_unpacklo_epi8(_mm_loadl_epi64(&p1[j-2]),zero);
									d[5]=_mm_unpacklo_epi8(_mm_loadl_epi64(&p3[j-2]),zero);


									
									d[6]=_mm_srli_si128(_mm_add_epi16(d[1],d[5]),4);
									d[7]=_mm_srli_si128(_mm_add_epi16(d[2],d[4]),4);
									tmp=_mm_shuffle_epi32(d[5],_MM_SHUFFLE(1,0,3,2));
									d[1]=_mm_add_epi16(d[1],tmp);
									tmp=_mm_shuffle_epi8(d[4],shuffle);
									tmp=_mm_shuffle_epi32(tmp,_MM_SHUFFLE(1,0,3,2));
									d[2]=_mm_shuffle_epi8(d[2],shuffle);
									d[2]=_mm_add_epi16(d[2],tmp);

								}
								//1st
								tmp=_mm_set1_epi32(coeff[0]);
								tmp=_mm_mullo_epi32(tmp,_mm_unpacklo_epi16(d[1],zero));
								d[0]=_mm_add_epi32(d[0],tmp);

								//d[1]=_mm_srli_si128(d[1],4);
								tmp=_mm_set1_epi32(coeff[1]);
								tmp=_mm_mullo_epi32(tmp,_mm_unpacklo_epi16(d[6],zero));
								d[0]=_mm_add_epi32(d[0],tmp);
								
								d[1]=_mm_srli_si128(d[1],8);
								tmp=_mm_set1_epi32(coeff[2]);
								tmp=_mm_mullo_epi32(tmp,_mm_unpacklo_epi16(d[1],zero));
								d[0]=_mm_add_epi32(d[0],tmp);

								//2nd
								tmp=_mm_set1_epi32(coeff[3]);
								tmp=_mm_mullo_epi32(tmp,_mm_unpacklo_epi16(d[2],zero));
								d[0]=_mm_add_epi32(d[0],tmp);

								//d[2]=_mm_srli_si128(d[2],2);
								tmp=_mm_set1_epi32(coeff[4]);
								tmp=_mm_mullo_epi32(tmp,_mm_unpacklo_epi16(d[7],zero));
								d[0]=_mm_add_epi32(d[0],tmp);

								d[2]=_mm_srli_si128(d[2],8);
								tmp=_mm_set1_epi32(coeff[5]);
								tmp=_mm_mullo_epi32(tmp,_mm_unpacklo_epi16(d[2],zero));
								d[0]=_mm_add_epi32(d[0],tmp);


								//3rd
								tmp=_mm_set1_epi32(coeff[6]);
								tmp=_mm_mullo_epi32(tmp,_mm_unpacklo_epi16(d[3],zero));
								d[0]=_mm_add_epi32(d[0],tmp);
								d[3]=_mm_srli_si128(d[3],2);
								tmp=_mm_set1_epi32(coeff[7]);
								tmp=_mm_mullo_epi32(tmp,_mm_unpacklo_epi16(d[3],zero));
								d[0]=_mm_add_epi32(d[0],tmp);
								d[3]=_mm_srli_si128(d[3],2);
								tmp=_mm_set1_epi32(coeff[8]);
								tmp=_mm_mullo_epi32(tmp,_mm_unpacklo_epi16(d[3],zero));
								d[0]=_mm_add_epi32(d[0],tmp);
								d[3]=_mm_srli_si128(d[3],2);
								tmp=_mm_set1_epi32(coeff[7]);
								tmp=_mm_mullo_epi32(tmp,_mm_unpacklo_epi16(d[3],zero));
								d[0]=_mm_add_epi32(d[0],tmp);
								d[3]=_mm_srli_si128(d[3],2);
								tmp=_mm_set1_epi32(coeff[6]);
								tmp=_mm_mullo_epi32(tmp,_mm_unpacklo_epi16(d[3],zero));
								d[0]=_mm_add_epi32(d[0],tmp);


								d[0]=_mm_srai_epi32(d[0],ALF_NUM_BIT_SHIFT);
								d[0]=_mm_packus_epi32(d[0],d[0]);
								d[0]=_mm_packus_epi16(d[0],d[0]);
								//_mm_storel_epi64(&p_dst[i*stride + j],d[0]);
								((int*)&p_dst[i*stridech + j])[0]=d[0].m128i_i32[0];

								}
								p+=stridech;
								p1+=stridech;
								p2+=stridech;
								p3+=stridech;
								p4+=stridech;

							}
						}
						else if(alf->realfiltNo_chroma==1)
						{
							for(i=0;i<loc_height;i++)
							{
								for(j=0;j<loc_width;j+=4)
								{
									/*
									tmppixel  = 0;
									tmppixel += qh[0] * (p3[j] + p4[j]);
									tmppixel += qh[1] * (p1[j] + p2[j]);

									tmppixel += qh[2] * (p[j+5]   + p[j-5]);
									tmppixel += qh[3] * (p[j+4]   + p[j-4]);
									tmppixel += qh[4] * (p[j+3]   + p[j-3]);
									tmppixel += qh[5] * (p[j+2]   + p[j-2]);
									tmppixel += qh[6] * (p[j+1]   + p[j-1]);
									tmppixel += qh[7] * (p[j]);

									// DC offset
									tmppixel += qh[N] << 0;
									tmppixel = (tmppixel + ALF_ROUND_OFFSET)>>ALF_NUM_BIT_SHIFT;
									p_dst[i*stridech + j]=lent_clip_uint8(tmppixel);
									*/
							{
									coeff=qh;

									d[0]=_mm_set1_epi32(qh[N]+ALF_ROUND_OFFSET);
									d[1]=_mm_unpacklo_epi8(_mm_loadl_epi64(&p3[j]),zero);
									d[2]=_mm_unpacklo_epi8(_mm_loadl_epi64(&p1[j]),zero);
									d[3]=_mm_unpacklo_epi8(_mm_loadl_epi64(&p[j-5]),zero);
									d[6]=_mm_unpacklo_epi8(_mm_loadl_epi64(&p[j]),zero);
									d[7]=_mm_unpacklo_epi8(_mm_loadl_epi64(&p[j+1]),zero);
									d[4]=_mm_unpacklo_epi8(_mm_loadl_epi64(&p2[j]),zero);
									d[5]=_mm_unpacklo_epi8(_mm_loadl_epi64(&p4[j]),zero);


									d[1]=_mm_add_epi16(d[1],d[5]);
									d[2]=_mm_add_epi16(d[2],d[4]);
								}

								//1st
								tmp=_mm_set1_epi32(coeff[0]);
								tmp=_mm_mullo_epi32(tmp,_mm_unpacklo_epi16(d[1],zero));
								d[0]=_mm_add_epi32(d[0],tmp);

								//2nd
								tmp=_mm_set1_epi32(coeff[1]);
								tmp=_mm_mullo_epi32(tmp,_mm_unpacklo_epi16(d[2],zero));
								d[0]=_mm_add_epi32(d[0],tmp);

								//3rd left 5
								tmp=_mm_set1_epi32(coeff[2]);
								tmp=_mm_mullo_epi32(tmp,_mm_unpacklo_epi16(d[3],zero));
								d[0]=_mm_add_epi32(d[0],tmp);
								d[3]=_mm_srli_si128(d[3],2);
								tmp=_mm_set1_epi32(coeff[3]);
								tmp=_mm_mullo_epi32(tmp,_mm_unpacklo_epi16(d[3],zero));
								d[0]=_mm_add_epi32(d[0],tmp);
								d[3]=_mm_srli_si128(d[3],2);
								tmp=_mm_set1_epi32(coeff[4]);
								tmp=_mm_mullo_epi32(tmp,_mm_unpacklo_epi16(d[3],zero));
								d[0]=_mm_add_epi32(d[0],tmp);
								d[3]=_mm_srli_si128(d[3],2);
								tmp=_mm_set1_epi32(coeff[5]);
								tmp=_mm_mullo_epi32(tmp,_mm_unpacklo_epi16(d[3],zero));
								d[0]=_mm_add_epi32(d[0],tmp);
								d[3]=_mm_srli_si128(d[3],2);
								tmp=_mm_set1_epi32(coeff[6]);
								tmp=_mm_mullo_epi32(tmp,_mm_unpacklo_epi16(d[3],zero));
								d[0]=_mm_add_epi32(d[0],tmp);

								//3rd middle
								tmp=_mm_set1_epi32(coeff[7]);
								tmp=_mm_mullo_epi32(tmp,_mm_unpacklo_epi16(d[6],zero));
								d[0]=_mm_add_epi32(d[0],tmp);

								//3rd right

								tmp=_mm_set1_epi32(coeff[6]);
								tmp=_mm_mullo_epi32(tmp,_mm_unpacklo_epi16(d[7],zero));
								d[0]=_mm_add_epi32(d[0],tmp);
								d[7]=_mm_srli_si128(d[7],2);
								tmp=_mm_set1_epi32(coeff[5]);
								tmp=_mm_mullo_epi32(tmp,_mm_unpacklo_epi16(d[7],zero));
								d[0]=_mm_add_epi32(d[0],tmp);
								d[7]=_mm_srli_si128(d[7],2);
								tmp=_mm_set1_epi32(coeff[4]);
								tmp=_mm_mullo_epi32(tmp,_mm_unpacklo_epi16(d[7],zero));
								d[0]=_mm_add_epi32(d[0],tmp);
								d[7]=_mm_srli_si128(d[7],2);
								tmp=_mm_set1_epi32(coeff[3]);
								tmp=_mm_mullo_epi32(tmp,_mm_unpacklo_epi16(d[7],zero));
								d[0]=_mm_add_epi32(d[0],tmp);
								d[7]=_mm_srli_si128(d[7],2);
								tmp=_mm_set1_epi32(coeff[2]);
								tmp=_mm_mullo_epi32(tmp,_mm_unpacklo_epi16(d[7],zero));
								d[0]=_mm_add_epi32(d[0],tmp);

								d[0]=_mm_srai_epi32(d[0],ALF_NUM_BIT_SHIFT);
								d[0]=_mm_packus_epi32(d[0],d[0]);
								d[0]=_mm_packus_epi16(d[0],d[0]);
								//_mm_storel_epi64(&p_dst[i*stride + j],d[0]);
								((int*)&p_dst[i*stridech + j])[0]=d[0].m128i_i32[0];
								}
								p+=stridech;
								p1+=stridech;
								p2+=stridech;
								p3+=stridech;
								p4+=stridech;

							}
						}
						else
						{
							assert(0);//other filtNo is not supported
						}
					}

				}

			}
			}
			else
				idxstep=1;
		}



	}
}