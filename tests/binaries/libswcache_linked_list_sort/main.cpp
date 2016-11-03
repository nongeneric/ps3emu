/* SCE CONFIDENTIAL
 * PlayStation(R)3 Programmer Tool Runtime Library 400.001
 * Copyright (C) 2008 Sony Computer Entertainment Inc.
 * All Rights Reserved.
 */

#include <sys/process.h>
SYS_PROCESS_PARAM(1001, 0x10000)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>

using namespace std;

#include <hl_spurs.h>
using namespace sce::hl::spurs;
#include <spu_printf.h>

#include "linked_list_sort.h"

static const char *SAMPLE_NAME = "sample_swcache_linked_list_sort";

static const unsigned int scNum =128;

template<class NodePointer> static void createRandomLinkedList(NodePointer pNode, unsigned int n)
{
	//E create bi-directional link
	for(unsigned int i = 0; i < n; i++)
	{
		if (i == 0)
		{
			pNode->mpPrev = NULL;
		} else {
			pNode->mpPrev = pNode-1;
		}
		if (i == n - 1)
		{
			pNode->mpNext = NULL;
		} else {
			pNode->mpNext = pNode+1;
		}
		pNode++->mVal = rand();
	}
}

template<class NodePointer> static int verify(NodePointer pNode)
{
	int ret = 0;
	while(pNode->mpPrev) pNode = pNode->mpPrev;
	while(pNode->mpNext)
	{
		if (pNode->mVal > pNode->mpNext->mVal)
		{
			printf("### Sort error %#x@%p > %#x@%p\n",
				pNode->mVal, (void *)pNode, pNode->mpNext->mVal, (void *)pNode->mpNext);
			ret = -1;
		}
		pNode = pNode->mpNext;
	}
	return ret;
}

//E SPU Elf symbols
extern char _binary_task_spurs_task_elf_start[];

static Spurs sSpurs;
//E exec sample on SPURS task
static int sort_by_spu()
{
	int ret;
	//E create SPURS taskset
	Taskset* taskset = static_cast<Taskset*>(std::memalign(Taskset::ALIGN, sizeof(Taskset)));
	ret = Taskset::create(taskset, "linked_list_sort", &sSpurs, 0);
	assert(ret == CELL_OK);

	//E create SPURS task
	Task* task = static_cast<Task*>(std::memalign(Task::ALIGN, sizeof(Task)));

	//E create linked list by PatchPointer
	static Node sNodes[scNum];
	createRandomLinkedList<Node *>(sNodes, scNum);

	//E create linked list by Pointer
	static NodeP sNodePs[scNum];
	Pointer<NodeP> ptr(sNodePs);
	createRandomLinkedList<Pointer<NodeP> >(ptr, scNum);

	CellSpursTaskArgument taskArg;
	taskArg.u32[0] = (uintptr_t)sNodes;
	taskArg.u32[1] = (uintptr_t)sNodePs;
	ret = Task::create(task, taskset, _binary_task_spurs_task_elf_start, taskArg);
	assert(ret == CELL_OK);

	ret = task->join();
	assert(ret == CELL_OK);

	printf("verify linked list sort by PatchObject\n");
	ret = verify<Node *>(sNodes);
	printf("verify linked list sort by Pointer\n");
	ret |= verify<Pointer<NodeP> >(ptr);

	std::free(task);

	taskset->shutdown();
	taskset->join();
	std::free(taskset);
	return ret;
}

static int sort_by_ppu()
{
	int ret;
	//E sort linked list by PatchObject
	printf("Linked list by PatchedObject\n");
	static Node sNodes[scNum];
	createRandomLinkedList<Node *>(sNodes, scNum);
	linked_list_sort(sNodes);
	printf("verify linked list sort by PatchObject\n");
	ret = verify<Node *>(sNodes);

	//E sort linked list by Pointer
	printf("Linked list by Pointer\n");
	static NodeP sNodePs[scNum];
	Pointer<NodeP> ptr(sNodePs);
	createRandomLinkedList<Pointer<NodeP> >(ptr, scNum);
	linked_list_sort(ptr);
	printf("verify linked list sort by Pointer\n");
	ret |= verify<Pointer<NodeP> >(ptr);
	return ret;
}

int main()
{
	int ret;

	ret = spu_printf_initialize(Spurs::DEFAULT_PPU_THREAD_PRIORITY + 1, 0);
	assert(ret == CELL_OK);

	//E initialize SPURS
	ret = Spurs::initialize(&sSpurs, "sample",1);
	assert(ret == CELL_OK);

	printf("(1) ### Sort linked list by PPU ###\n");
	ret = sort_by_ppu();

	printf("(2) ### Sort linked list by SPU ###\n");
	ret |= sort_by_spu();

	sSpurs.finalize();

	spu_printf_finalize();

	cout << "## libswcache : " << SAMPLE_NAME << (ret?" FAILED ##":" SUCCEEDED ##") << endl;

	return 0;
}
