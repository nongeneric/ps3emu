/* SCE CONFIDENTIAL                                    */
/* PlayStation(R)3 Programmer Tool Runtime Library 400.001                                           */
/* Copyright (C) 2007 Sony Computer Entertainment Inc. */
/* All Rights Reserved.                                */

#include <spu_intrinsics.h>
#include <spu_mfcio.h>

#include "sort_body.h"


#define DMA_TAG	31


static SortNodeType& LoadNode(unsigned int ea,SortNodeType& s)
{
	mfc_get(&s,ea,sizeof(SortNodeType),DMA_TAG,0,0);
	mfc_write_tag_mask(1<<DMA_TAG);
	mfc_read_tag_status_all();
	return s;
}

static void StoreNode(unsigned int ea,SortNodeType& s)
{
	mfc_put(&s,ea,sizeof(SortNodeType),DMA_TAG,0,0);
	mfc_write_tag_mask(1<<DMA_TAG);
	mfc_read_tag_status_all();
}

KeyType SelectPivot(unsigned int start_ea,unsigned int end_ea,SortNodeType& p_data)
{
	KeyType pivot_key;
	if(end_ea-start_ea>10*sizeof(SortNodeType)){
		SortNodeType data[3];
		unsigned int ea[3];

		ea[0]=start_ea;
		ea[1]=end_ea-sizeof(SortNodeType);
		ea[2]=((start_ea+end_ea)/2)&~(sizeof(SortNodeType)-1);
		LoadNode(ea[0],data[0]);
		LoadNode(ea[1],data[1]);
		LoadNode(ea[2],data[2]);
		int mid=SelectPivotOnLS(data[0],data[1],data[2]);
		StoreNode(ea[mid],data[0]);
		p_data=data[mid];
		pivot_key=Key(p_data);
	}
	else
		pivot_key=Key(LoadNode(start_ea,p_data));
	return pivot_key;
}

unsigned int QSortSub(unsigned int start_ea,unsigned int end_ea)  // E not recursive
{
	SortNodeType s_data,e_data,p_data;
	KeyType pivot_key;

	pivot_key=SelectPivot(start_ea,end_ea,p_data);
	while(1)
	{
		// E pivot is at start_ea!
		do
		{
			end_ea-=sizeof(SortNodeType);
			if(start_ea==end_ea)
			{
				StoreNode(start_ea,p_data);
				return start_ea;
			}
		}while(Key(LoadNode(end_ea,e_data))>pivot_key);
		StoreNode(start_ea,e_data); // E pivot move to end_ea!
		// E pivot is at end_ea!
		do
		{
			start_ea+=sizeof(SortNodeType);
			if(start_ea==end_ea)
			{
				StoreNode(start_ea,p_data);
				return start_ea;
			}
		}while(Key(LoadNode(start_ea,s_data))<pivot_key);
		StoreNode(end_ea,s_data); // E pivot move to start_ea!
	}
}

/*
 * Local Variables:
 * mode: C
 * tab-width: 4
 * End:
 * vim:sw=4:sts=4:ts=4
 */
