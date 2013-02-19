
static int DECODE_TREE(HEVCContext *h,int i_log2_size,int idx,int pix_x,int pix_y)//解码CU四叉树
{
	int width=1<<i_log2_size;
	int height=1<<i_log2_size;
	int depth=MAX_CU_LOG2-i_log2_size;
	int split;
	int more_data=0;
	int i8=LENT_scan8[idx>>2];


	if(IN_PIC(h,pix_x+width-1,pix_y+height-1)&&
		i_log2_size>h->sps.m_uiCULog2MinSize
		)//当前block确定全部处于图像范围内，且不为最小块大小，则可能分割可能不分割
	{
		int left=h->cache.cu_depth[i8-1];
		int top=h->cache.cu_depth[i8-16];
		int ctx=OFS_SPLIT_FLAG_CTX + (left > depth) + (top > depth);
		split=get_cabac_noinline(&h->cabac,&h->cabac.cabac_state[ctx]);
	}
	else if(IN_PIC(h,pix_x+width-1,pix_y+height-1))//在图像内部，且为最小块大小，则必定不分割
	{
		split=0;
	}
	else
	{
		split=1;
	}
	if(split)
	{
		int pix_x_next=pix_x+(width>>1);
		int pix_y_next=pix_y+(height>>1);
		int idx_step=DEPTH2IDXSTEPC(depth);

		more_data=DECODE_TREE(h,i_log2_size-1,idx,pix_x,pix_y);

		idx+=idx_step;
		if(IN_PIC(h,pix_x_next,pix_y)&&more_data)
			more_data=DECODE_TREE(h,i_log2_size-1,idx,pix_x_next,pix_y);

		idx+=idx_step;
		if(IN_PIC(h,pix_x,pix_y_next)&&more_data)
			more_data=DECODE_TREE(h,i_log2_size-1,idx,pix_x,pix_y_next);

		idx+=idx_step;
		if(IN_PIC(h,pix_x_next,pix_y_next)&&more_data)
			more_data=DECODE_TREE(h,i_log2_size-1,idx,pix_x_next,pix_y_next);
	}
	else
	{
		lent_fill_rectangle(&h->cache.cu_depth[i8],LOG22I8WIDTH(i_log2_size),LOG22I8WIDTH(i_log2_size),16,depth);
		decode_block(h,i_log2_size,idx,pix_x,pix_y);

		if(0)//granularity_block_boundary: not defined yet
		{
		}
		else
			more_data=1;

	}
	return more_data;
}