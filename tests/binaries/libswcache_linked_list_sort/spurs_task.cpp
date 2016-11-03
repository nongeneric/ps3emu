/* SCE CONFIDENTIAL
 * PlayStation(R)3 Programmer Tool Runtime Library 400.001
 * Copyright (C) 2008 Sony Computer Entertainment Inc.
 * All Rights Reserved.
 */

//E Spurs task
#include <cell/spurs/task.h>

//E The C++ templates
#include <cell/swcache.h>
using namespace cell::swcache;

#include "linked_list_sort.h"

static const int scHeapSize = 16 * 1024;
static uint8_t sHeap[scHeapSize] __attribute__((aligned(128)));

//E The main() function for the spurs task
int cellSpursTaskMain(qword argTask, uint64_t argTaskSet)
{
	(void)argTaskSet;

	//E initialize swcache
	CacheResource<DefaultCache<DefaultHeap> >::initialize(sHeap, scHeapSize);
	{
		//E parse arguments
		Node *pNode = (Node *)(uintptr_t)spu_extract((vec_uint4)argTask, 0);
		Pointer<NodeP> pNodeP((NodeP *)(uintptr_t)spu_extract((vec_uint4)argTask, 1));

		//E linked list sort by PathcObject
		spu_printf("Linked list by PatchObject\n");
		linked_list_sort(pNode);

		//E linked list sort by Pointer
		spu_printf("Linked list by Pointer\n");
		linked_list_sort(pNodeP);
	}
	//E finish any pending DMA transfers
	CacheResource<DefaultCache<DefaultHeap> >::finalize();

	return CELL_OK;
}
