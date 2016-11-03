/* SCE CONFIDENTIAL
 * PlayStation(R)3 Programmer Tool Runtime Library 400.001
 * Copyright (C) 2008 Sony Computer Entertainment Inc.
 * All Rights Reserved.
 */

#include <stdio.h>
#include <cell/swcache/patch_object.h>

#include "linked_list_sort.h"

static void replace(Node *pOrg, Node *pAlt)
{
	CELL_SWCACHE_PATCH_CONST_OBJECT(Node, pOrg);
	CELL_SWCACHE_PATCH_OBJECT_AND_BLOCK(Node, pAlt);
	Node* &pOrg_mpPrev = pOrg->mpPrev;
	Node* &pOrg_mpNext = pOrg->mpNext;
	CELL_SWCACHE_PATCH_OBJECT(Node, pOrg_mpPrev);
	CELL_SWCACHE_PATCH_OBJECT_AND_BLOCK(Node, pOrg_mpNext);
	if (pOrg->mpPrev)
	{
		pOrg->mpPrev->mpNext = pAlt;
		Node* &pOrg_mpPrev_mpNext = pOrg->mpPrev->mpNext;
		CELL_SWCACHE_PATCH_CONST_OBJECT(Node, pOrg_mpPrev_mpNext);
	}
	if (pOrg->mpNext)
	{
		pOrg->mpNext->mpPrev = pAlt;
		Node* &pOrg_mpNext_mpPrev = pOrg->mpNext->mpPrev;
		CELL_SWCACHE_PATCH_CONST_OBJECT(Node, pOrg_mpNext_mpPrev);
	}
	pAlt->mpPrev = pOrg->mpPrev;
	pAlt->mpNext = pOrg->mpNext;
	Node* &pAlt_mpPrev = pAlt->mpPrev;
	Node* &pAlt_mpNext = pAlt->mpNext;
	CELL_SWCACHE_PATCH_CONST_OBJECT(Node, pAlt_mpPrev);
	CELL_SWCACHE_PATCH_CONST_OBJECT(Node, pAlt_mpNext);
}

static void swap(Node* pNode1, Node* pNode2)
{
	CELL_SWCACHE_PATCH_CONST_OBJECT(Node, pNode1);
	CELL_SWCACHE_PATCH_CONST_OBJECT(Node, pNode2);
	Node dummy;
	replace(pNode1, &dummy);
	replace(pNode2, pNode1);
	replace(&dummy, pNode2);
}

void linked_list_sort(Node *pData)
{
	CELL_SWCACHE_PATCH_CONST_OBJECT(Node, pData);
	Node *pNode1 = NULL;
	CELL_SWCACHE_PATCH_CONST_OBJECT(Node, pNode1);
	for(pNode1 = pData; pNode1; pNode1 = pNode1->mpNext)
	{
		CELL_SWCACHE_REPATCH_AND_BLOCK(pNode1);
		Node *pNode2 = NULL;
		CELL_SWCACHE_PATCH_CONST_OBJECT(Node, pNode2);
		for(pNode2 = pNode1->mpNext; pNode2; pNode2 = pNode2->mpNext)
		{
			CELL_SWCACHE_REPATCH_AND_BLOCK(pNode2);
			if (pNode1->mVal > pNode2->mVal)
			{
				swap(pNode1, pNode2);

				Node *pDummy;
				pDummy = pNode1;
				CELL_SWCACHE_PATCH_CONST_OBJECT(Node, pDummy);
				pNode1 = pNode2;
				CELL_SWCACHE_REPATCH(pNode1);
				pNode2 = pDummy;
				CELL_SWCACHE_REPATCH_AND_BLOCK(pNode2);
			}
		}
	}
}
