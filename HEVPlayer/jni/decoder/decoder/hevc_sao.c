#include "hevc.h"
#include "hevc_cabac.h"
#include "hevc_sao.h"
UInt decode_SaoUvlc(HEVCContext *h, UInt maxValue)
{
	UInt res;
	for(res=0;res<maxValue;res++)
	{
		if(!get_cabac_bypass(&h->cabac))
			break;
	}
	return res;
}
void decode_SaoOffset(HEVCContext *h, SaoLcuParam* psSaoLcuParam, UInt compIdx)
{
	UInt uiSymbol;
	static const Int iTypeLength[MAX_NUM_SAO_TYPE] =
	{
		SAO_EO_LEN,
		SAO_EO_LEN,
		SAO_EO_LEN,
		SAO_EO_LEN,
		SAO_BO_LEN
	}; 

	if (compIdx==2)
	{
		uiSymbol = (UInt)( psSaoLcuParam->typeIdx + 1);
	}
	else
	{
		//parseSaoTypeIdx(uiSymbol);
		uiSymbol=get_cabac(&h->cabac, &h->cabac.cabac_state[OFS_SAO_TYPE_IDX_CTX]);
		if(uiSymbol)
		{
			if(!get_cabac_bypass(&h->cabac))
				uiSymbol=5;
		}
	}
	psSaoLcuParam->typeIdx = (Int)uiSymbol - 1;
	assert(psSaoLcuParam->typeIdx>=-1&&psSaoLcuParam->typeIdx<5);
	if (uiSymbol)
	{
		Int bitDepth = compIdx ? h->g_bitDepthC : h->g_bitDepthY;
		Int offsetTh = 1 << LENTMIN(bitDepth - 5,5);
		Int i;

		psSaoLcuParam->length = iTypeLength[psSaoLcuParam->typeIdx];

		if( psSaoLcuParam->typeIdx == SAO_BO )
		{
			for(i=0; i< psSaoLcuParam->length; i++)
			{
				//parseSaoMaxUvlc(uiSymbol, offsetTh -1 );
				uiSymbol=decode_SaoUvlc(h, offsetTh-1);
				psSaoLcuParam->offset[i] = uiSymbol;
			}   
			for(i=0; i< psSaoLcuParam->length; i++)
			{
				if (psSaoLcuParam->offset[i] != 0) 
				{
					uiSymbol=get_cabac_bypass(&h->cabac);
					if (uiSymbol)
					{
						psSaoLcuParam->offset[i] = -psSaoLcuParam->offset[i] ;
					}
				}
			}
			//parseSaoUflc(5, uiSymbol );
			for(i=uiSymbol=0;i<5;i++)//todo bypass-with-length
			{
				uiSymbol<<=1;
				uiSymbol+=get_cabac_bypass(&h->cabac);
			}
			psSaoLcuParam->subTypeIdx = uiSymbol;
		}
		else if( psSaoLcuParam->typeIdx < 4 )
		{
			uiSymbol=decode_SaoUvlc(h, offsetTh-1); psSaoLcuParam->offset[0] = uiSymbol;
			uiSymbol=decode_SaoUvlc(h, offsetTh-1); psSaoLcuParam->offset[1] = uiSymbol;
			uiSymbol=decode_SaoUvlc(h, offsetTh-1); psSaoLcuParam->offset[2] = -(Int)uiSymbol;
			uiSymbol=decode_SaoUvlc(h, offsetTh-1); psSaoLcuParam->offset[3] = -(Int)uiSymbol;
			if (compIdx != 2)
			{
				//parseSaoUflc(2, uiSymbol );
				for(i=uiSymbol=0;i<2;i++)//todo bypass-with-length
				{
					uiSymbol<<=1;
					uiSymbol+=get_cabac_bypass(&h->cabac);
				}
				psSaoLcuParam->subTypeIdx = uiSymbol;
				psSaoLcuParam->typeIdx += psSaoLcuParam->subTypeIdx;
			}
		}
	}
	else
	{
		psSaoLcuParam->length = 0;
	}
}

void lent_hevc_decode_saoparam(HEVCContext *h)
{
	SAOParam *saoParam = &h->current.sao;
	Int numCuInWidth  = h->cu_width;
	Int iCUAddr = h->cu_index;
	Int iCUAddrInSlice = iCUAddr;//iCUAddr - rpcPic->getPicSym()->getCUOrderMap(pcSlice->getSliceCurStartCUAddr()/rpcPic->getNumPartInCU());//todo slices support
	Int iCUAddrUpInSlice  = iCUAddrInSlice - numCuInWidth;
	Int rx = h->cu_x;
	Int ry = h->cu_y;
	Int allowMergeLeft = 1;
	Int allowMergeUp   = 1;
	//if (rx!=0)//todo tiles support
	//{
	//	if (rpcPic->getPicSym()->getTileIdxMap(iCUAddr-1) != rpcPic->getPicSym()->getTileIdxMap(iCUAddr))
	//	{
	//		allowMergeLeft = 0;
	//	}
	//}
	//if (ry!=0)
	//{
	//	if (rpcPic->getPicSym()->getTileIdxMap(iCUAddr-numCuInWidth) != rpcPic->getPicSym()->getTileIdxMap(iCUAddr))
	//	{
	//		allowMergeUp = 0;
	//	}
	//}
	
	{
		Int iAddr = iCUAddr;
		UInt uiSymbol;
		Int iCompIdx;
		SAOParam *pSaoParam=saoParam;
		//for (iCompIdx=0; iCompIdx<3; iCompIdx++)//not necessary
		//{
		//	pSaoParam->saoLcuParam[iCompIdx][iAddr].mergeUpFlag    = 0;
		//	pSaoParam->saoLcuParam[iCompIdx][iAddr].mergeLeftFlag  = 0;
		//	pSaoParam->saoLcuParam[iCompIdx][iAddr].subTypeIdx     = 0;
		//	pSaoParam->saoLcuParam[iCompIdx][iAddr].typeIdx        = -1;
		//	pSaoParam->saoLcuParam[iCompIdx][iAddr].offset[0]     = 0;
		//	pSaoParam->saoLcuParam[iCompIdx][iAddr].offset[1]     = 0;
		//	pSaoParam->saoLcuParam[iCompIdx][iAddr].offset[2]     = 0;
		//	pSaoParam->saoLcuParam[iCompIdx][iAddr].offset[3]     = 0;

		//}
		if (pSaoParam->bSaoFlag[0] || pSaoParam->bSaoFlag[1] )
		{
			if (rx>0 && iCUAddrInSlice!=0 && allowMergeLeft)
			{
				uiSymbol=get_cabac(&h->cabac,&h->cabac.cabac_state[OFS_SAO_MERGE_FLAG_CTX]);
				pSaoParam->saoLcuParam[0][iAddr].mergeLeftFlag = (Bool)uiSymbol;  
			}
			if (pSaoParam->saoLcuParam[0][iAddr].mergeLeftFlag==0)
			{
				if ((ry > 0) && (iCUAddrUpInSlice>=0) && allowMergeUp)
				{
					uiSymbol=get_cabac(&h->cabac,&h->cabac.cabac_state[OFS_SAO_MERGE_FLAG_CTX]);
					pSaoParam->saoLcuParam[0][iAddr].mergeUpFlag = (Bool)uiSymbol;
				}
			}
		}

		for (iCompIdx=0; iCompIdx<3; iCompIdx++)
		{
			if ((iCompIdx == 0  && pSaoParam->bSaoFlag[0]) || (iCompIdx > 0  && pSaoParam->bSaoFlag[1]) )
			{
				if (rx>0 && iCUAddrInSlice!=0 && allowMergeLeft)
				{
					pSaoParam->saoLcuParam[iCompIdx][iAddr].mergeLeftFlag = pSaoParam->saoLcuParam[0][iAddr].mergeLeftFlag;
				}
				else
				{
					pSaoParam->saoLcuParam[iCompIdx][iAddr].mergeLeftFlag = 0;
				}

				if (pSaoParam->saoLcuParam[iCompIdx][iAddr].mergeLeftFlag==0)
				{
					if ((ry > 0) && (iCUAddrUpInSlice>=0) && allowMergeUp)
					{
						pSaoParam->saoLcuParam[iCompIdx][iAddr].mergeUpFlag = pSaoParam->saoLcuParam[0][iAddr].mergeUpFlag;
					}
					else
					{
						pSaoParam->saoLcuParam[iCompIdx][iAddr].mergeUpFlag = 0;
					}
					if (!pSaoParam->saoLcuParam[iCompIdx][iAddr].mergeUpFlag)
					{
						pSaoParam->saoLcuParam[2][iAddr].typeIdx = pSaoParam->saoLcuParam[1][iAddr].typeIdx;
						decode_SaoOffset(h, &(pSaoParam->saoLcuParam[iCompIdx][iAddr]), iCompIdx);
					}
					else
					{
						//copySaoOneLcuParam(&pSaoParam->saoLcuParam[iCompIdx][iAddr], &pSaoParam->saoLcuParam[iCompIdx][iAddr-pSaoParam->numCuInWidth]);
						memcpy(&pSaoParam->saoLcuParam[iCompIdx][iAddr],  &pSaoParam->saoLcuParam[iCompIdx][iAddr-numCuInWidth], sizeof(SaoLcuParam));
					}
				}
				else
				{
					//copySaoOneLcuParam(&pSaoParam->saoLcuParam[iCompIdx][iAddr],  &pSaoParam->saoLcuParam[iCompIdx][iAddr-1]);
					memcpy(&pSaoParam->saoLcuParam[iCompIdx][iAddr],  &pSaoParam->saoLcuParam[iCompIdx][iAddr-1], sizeof(SaoLcuParam));
				}
			}
			else
			{
				pSaoParam->saoLcuParam[iCompIdx][iAddr].typeIdx = -1;
				pSaoParam->saoLcuParam[iCompIdx][iAddr].subTypeIdx = 0;
			}
		}
	}
	
}

void lent_sao_row(HEVCContext *h, int y)
{
	SAOParam *pcSaoParam=&h->current.sao;
	if (pcSaoParam->bSaoFlag[0] || pcSaoParam->bSaoFlag[1])
	{
		int m_uiSaoBitIncreaseY = LENTMAX(h->g_bitDepthY - 10, 0);
		int m_uiSaoBitIncreaseC = LENTMAX(h->g_bitDepthC - 10, 0);

		//if(m_bUseNIF)//todo support tiles&slices
		//{
		//	m_pcPic->getPicYuvRec()->copyToPic(m_pcYuvTmp);
		//}
		//if (m_saoLcuBasedOptimization)
		//{
		//	pcSaoParam->oneUnitFlag[0] = 0;  
		//	pcSaoParam->oneUnitFlag[1] = 0;  
		//	pcSaoParam->oneUnitFlag[2] = 0;  
		//}
		//if (pcSaoParam->bSaoFlag[0])
		//{
		//	processSaoUnitAll( pcSaoParam->saoLcuParam[0], /*pcSaoParam->oneUnitFlag[0],*/ 0);
		//}
		//if(pcSaoParam->bSaoFlag[1])
		//{
		//	processSaoUnitAll( pcSaoParam->saoLcuParam[1], /*pcSaoParam->oneUnitFlag[1],*/ 1);//Cb
		//	processSaoUnitAll( pcSaoParam->saoLcuParam[2], /*pcSaoParam->oneUnitFlag[2],*/ 2);//Cr
		//}
		//m_pcPic = NULL;
	}
}