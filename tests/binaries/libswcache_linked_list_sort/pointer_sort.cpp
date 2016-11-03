/* SCE CONFIDENTIAL
 * PlayStation(R)3 Programmer Tool Runtime Library 400.001
 * Copyright (C) 2008 Sony Computer Entertainment Inc.
 * All Rights Reserved.
 */

#include <stdio.h>

#include <cell/swcache/pointer.h>
#include "linked_list_sort.h"

static void replace(Pointer<NodeP> pOrg, Pointer<NodeP> pAlt)
{
	if (pOrg->mpPrev)
	{
		Pointer<NodeP>::Grab grab(pOrg->mpPrev, 1);
		pOrg->mpPrev->mpNext = pAlt;
	}
	if (pOrg->mpNext)
	{
		Pointer<NodeP>::Grab grab(pOrg->mpNext, 1);
		pOrg->mpNext->mpPrev = pAlt;
	}
	pAlt->mpPrev = pOrg->mpPrev;
	pAlt->mpNext = pOrg->mpNext;
	pAlt.setWritable();
}

static void swap(Pointer<NodeP> pNode1, Pointer<NodeP> pNode2)
{
	NodeP dummy;
	Pointer<NodeP> pDummy = &dummy;
	replace(pNode1, pDummy);
	replace(pNode2, pNode1);
	replace(pDummy, pNode2);
}

void linked_list_sort(Pointer<NodeP> pData)
{
	for(Pointer<NodeP> pNode1 = pData; pNode1; pNode1 = pNode1->mpNext)
	{
		for(Pointer<NodeP> pNode2 = pNode1->mpNext; pNode2; pNode2 = pNode2->mpNext)
		{
			if (pNode1->mVal > pNode2->mVal)
			{
				swap(pNode1, pNode2);

				Pointer<NodeP> pDummy;
				pDummy = pNode1;
				pNode1 = pNode2;
				pNode2 = pDummy;
			}
		}
	}
}
