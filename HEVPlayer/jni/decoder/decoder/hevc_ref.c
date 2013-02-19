//**********************************************
//hevc_ref.c
//Unipipy @2011
//reference list related code
//**********************************************

#include "hevc.h"
#include "hevc_ref.h"
#include "hevc_frame.h"

static void unreference_picture(Picture *pic)
{
	pic->reference&=~FRAME_REF;
}

void lent_remove_all_refs(HEVCContext *h)
{
	int j;
	for(j=0;j<h->dpb_count;j++)
	{
		unreference_picture(h->default_ref_buffer[j]);
		h->default_ref_buffer[j]=NULL;
	}
	h->dpb_count=0;
}
void lent_build_def_list(HEVCContext *h)
{
	int i,j,l0,l1,l0_act=0,l1_act=0,curPOC=h->sh.m_iPOC;

	RPS *rps=h->sh.m_pcRPS;

	for(j=0;j<h->dpb_count;j++)
		if(h->default_ref_buffer[j]->poc>curPOC)
			break;
	memset(h->ref_list,0,sizeof(h->ref_list));
	memset(h->ref_list2_to_list01,0,sizeof(h->ref_list2_to_list01));
	memset(h->ref_list2_to_list01,0,sizeof(h->ref_list2_to_list01_idx));
	h->i_ref_count[2]=0;

	assert(j>=rps->m_numberOfNegativePictures);
	assert(h->dpb_count-j>=rps->m_numberOfPositivePictures);
	l0=LENTMIN(rps->m_numberOfNegativePictures,j);
	l1=LENTMIN(rps->m_numberOfPositivePictures,h->dpb_count-j);
	for(i=0;i<l0;i++)
	{
		int tag=0;
		for(j=0;j<h->dpb_count;j++)
		{
			if(rps->m_deltaPOC[i]+curPOC==h->default_ref_buffer[j]->poc)
			{
				if(rps->m_used[i])
				{
					h->ref_list[0][l0_act++]=*h->default_ref_buffer[j];
				}
				tag=1;
				break;
			}
		}
		if(!tag)
			lent_dlog(NULL,"Missing reference picture! POC %d\n",rps->m_deltaPOC[i]+h->sh.m_iPOC);
	}
	for(i=0;i<l1;i++)
	{
		int tag=0;
		for(j=0;j<h->dpb_count;j++)
			if(rps->m_deltaPOC[i+rps->m_numberOfNegativePictures]+curPOC==h->default_ref_buffer[j]->poc)
			{
				if(rps->m_used[i+rps->m_numberOfNegativePictures])
				{
					h->ref_list[1][l1_act++]=*h->default_ref_buffer[j];
				}
				tag=1;
				break;
			}
		if(!tag)
			lent_dlog(NULL,"Missing reference picture! POC %d\n",rps->m_deltaPOC[i+rps->m_numberOfNegativePictures]+h->sh.m_iPOC);
	}
	if(h->sh.m_eSliceType==SLICE_B)
	{
		int j=0,k=0;
		i=0;
		do{
			if(j<l0_act)
			{
				h->ref_list2_to_list01_idx[i]=j;
				h->ref_list[2][i++]=h->ref_list[0][j++];
			}
			if(i>=l0_act+l1_act)
				break;
			if(k<l1_act)
			{
				h->ref_list2_to_list01[i]=1;
				h->ref_list2_to_list01_idx[i]=k;
				h->ref_list[2][i++]=h->ref_list[1][k++];
			}
			if(i>=l0_act+l1_act)
				break;
		}while(1);

		for(j=0;j<l1_act;j++)
			h->ref_list[0][j+l0_act]=h->ref_list[1][j];
		
		for(j=0;j<l0_act;j++)
			h->ref_list[1][j+l1_act]=h->ref_list[0][j];

		h->i_ref_count[2]  =
					l0_act =
					l1_act = l0_act+l1_act;
	}
	else
	{
		memcpy(h->ref_list[2],h->ref_list[0],sizeof(h->ref_list[0]));
		h->i_ref_count[2] = l0_act;
		for(j=0;j<l0_act;j++)
			h->ref_list2_to_list01_idx[j]=j;
	}
	h->i_ref_count[0]=l0_act<h->sh.m_aiNumRefIdx[0]?l0_act:h->sh.m_aiNumRefIdx[0];
	h->i_ref_count[1]=l1_act<h->sh.m_aiNumRefIdx[1]?l1_act:h->sh.m_aiNumRefIdx[1];
	//h->i_ref_count[2]=l1_act<h->sh.m_aiNumRefIdx[2]?l1_act:h->sh.m_aiNumRefIdx[2];
}
void lent_ref_insert(HEVCContext *h,Picture *pic)
{
	int j,count=h->dpb_count;
	Picture *temp_list[MAX_DPB_SIZE],**p=temp_list;

	for(j=0;j<count;j++)
		if(h->default_ref_buffer[j]->poc>pic->poc)
			break;

	if(count==MAX_DPB_SIZE)
	{
		memcpy(p,h->default_ref_buffer+1,(j-1)*sizeof(Picture*));
		p+=j-1;
		unreference_picture(h->default_ref_buffer[0]);
	}
	else
	{
		memcpy(p,h->default_ref_buffer,j*sizeof(Picture*));
		p+=j;
		h->dpb_count++;
	}
	*p=pic;
	p++;
	memcpy(p,h->default_ref_buffer+j,(count-j)*sizeof(Picture*));
	memcpy(h->default_ref_buffer,temp_list,h->dpb_count*sizeof(Picture*));
}