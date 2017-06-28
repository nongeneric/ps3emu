/* SCE CONFIDENTIAL                                    */
/* PlayStation(R)3 Programmer Tool Runtime Library 400.001                                           */
/* Copyright (C) 2007 Sony Computer Entertainment Inc. */
/* All Rights Reserved.                                */

#include "sort_on_ls.h"

static SortNodeType* SortDivide(SortNodeType* start,SortNodeType* end,SortNodeType* work);
static void VectorOddEvenMergeSort(SortNodeType* s,SortNodeType* t,int num);

#define SORT_TYPE_SELECT 3

SortNodeType* SortOnLS(SortNodeType* start,SortNodeType* end,SortNodeType* work)
{
#if (SORT_TYPE_SELECT==1)
	BakaSort(start,end,work);
	return work;
#elif (SORT_TYPE_SELECT==2)
	SortDivide(start,end,work);
	return start;
#else
	VectorOddEvenMergeSort(start,work,end-start);
	return work;
#endif
}

////////////////////////////////////////////////////////////////////////////////////

#include "sort_body.h"

inline SortNodeType* Divide(SortNodeType* data_s,SortNodeType* data_e,SortNodeType* result_ite);
SortNodeType* Divide(SortNodeType* data_s,SortNodeType* data_e,SortNodeType* result_ite)
{
	SortNodeType p_data;
	KeyType pivot_key;
	SortNodeType* pivot_sel[3]={data_s,data_e-1,data_s+(data_e-data_s)/2};

	int mid=SelectPivotOnLS(*pivot_sel[0],*pivot_sel[1],*pivot_sel[2]);
	p_data=*pivot_sel[mid];
	*pivot_sel[mid]=*data_s;
	pivot_key=Key(p_data);

	SortNodeType* result_s=result_ite;
	SortNodeType* result_e=result_s+(data_e-data_s-1);
	SortNodeType* ite=data_s+1;
	while(ite!=data_e)
	{
		SortNodeType n1,n2;
		n1=*ite++;
		n2=*ite;
		*result_s=n1;
		*result_e=n1;
		result_s=(Key(n1)<pivot_key)?result_s+1:result_s;
		result_e=(Key(n1)<pivot_key)?result_e:result_e-1;
		if(ite==data_e)break;
		ite++;
		*result_s=n2;
		*result_e=n2;
		result_s=(Key(n2)<=pivot_key)?result_s+1:result_s;
		result_e=(Key(n2)<=pivot_key)?result_e:result_e-1;
	}
	AssertIf(result_e!=result_s);
	*result_e=p_data;
	data_s[result_e-result_ite]=p_data;
	return data_s+(result_e-result_ite);
}

SortNodeType* SortDivide(SortNodeType* start,SortNodeType* end,SortNodeType* work)
{
	SortNodeType *pivot;
	while(start<end)
	{
		pivot=Divide(start,end,work);

		AssertIf(!((start<=pivot)&&(pivot<end)));
		if(pivot-start<end-pivot)
		{
			SortDivide(work,work+(pivot-start),start);
			end=work+(end-start);
			start=work+(pivot-start+1);
			work=pivot+1;
		}
		else
		{
			SortDivide(work+(pivot-start+1),work+(end-start),pivot+1);
			end=work+(pivot-start);
			SortNodeType* t= start;
			start=work;
			work=t;
		}
	}
	return start;
}

//////////////////////////////////////////////////////////////////////
#include "oe_mergesort.h"
void VectorOddEvenMergeSort(SortNodeType* s,SortNodeType* t,int num)
{
	const int loop_cnt=(num+7)/8;
	vector unsigned int address[loop_cnt*2+2];
	scalar_vector_table<KeyType>::vector_type key[loop_cnt*2+2];

	vector unsigned int base,diff;
	base=spu_promote((unsigned int)s,0);
	base=spu_insert((unsigned int) (s+1),base,1);
	base=spu_insert((unsigned int) (s+2),base,2);
	base=spu_insert((unsigned int) (s+3),base,3);
	diff=spu_splats((unsigned int) sizeof(SortNodeType)*4);

	for(int i=0;i<num;i++)
	{
		((unsigned int*)address)[i]=(unsigned int)(s+i);
		((KeyType*)key)[i]=Key(s[i]);
	}
	for(int i=num;i<loop_cnt*8+8;i++)
	{
		((unsigned int*)address)[i]=0;
		((KeyType*)key)[i]=std::numeric_limits<KeyType>::max();
	}

	OddEvenMergeSort(key,address,loop_cnt*8);

	for(int i=0;i<num;i++)
	{
		t[i]=*(((scalar_vector_table<KeyType>::vector_type**)(uintptr_t)address)[i]);
	}
}

/*
 * Local Variables:
 * mode: C
 * tab-width: 4
 * End:
 * vim:sw=4:sts=4:ts=4
 */
