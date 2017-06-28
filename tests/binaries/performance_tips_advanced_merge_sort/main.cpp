/* SCE CONFIDENTIAL                                    */
/* PlayStation(R)3 Programmer Tool Runtime Library 400.001                                           */
/* Copyright (C) 2008 Sony Computer Entertainment Inc. */
/* All Rights Reserved.                                */

/* standard headers */
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <sys/sys_time.h>
#include <hl_spurs.h>
#include <spu_printf.h>

#include "measure.h"

/* embedded SPU ELF symbols */
extern char _binary_spu_merge_sort_spu_elf_start[];

using namespace sce::hl::spurs;

/*E Configurations for SORT ****************************************************/

struct SpuCommunication // virtual to each communication
{
	virtual void Initialize(int)=0;
	virtual void DebugPrint(){};
	virtual uint64_t CommunicationEA()=0;
	virtual ~SpuCommunication(){};
};

void InitializeSortData(void);


#define SIZE_OF_SORT_NODE 10000

#define USER_ERROR_CODE		100000
#define SORT_NOT_SUCCESS		(USER_ERROR_CODE+1)

#define SORT_SPU_NUM	5
#define	SPURS_SPU_NUM SORT_SPU_NUM

#define TASK_STACK_SIZE 20

/*E Sort *************************************************************************/

typedef struct _DataNodeType
{
	unsigned int key;
	int data1; // E keep initial position cause of prepare to re-sort;
	int data2; // E not used
	int data3; // E not used
}
 __attribute__((aligned(16))) DataNodeType;

DataNodeType sort_set[SIZE_OF_SORT_NODE];
DataNodeType sort_work[SIZE_OF_SORT_NODE];

void InitializeSortData(void)
{
	int i;
	std::printf("Initilaize Sort Data ...");
	srand(13);
	/*E random data creation */
	for(i=0;i<SIZE_OF_SORT_NODE;i++)
	{
		sort_set[i].key=i;
	}
	for(i=0;i<SIZE_OF_SORT_NODE;i++)
	{
		DataNodeType tmp;
		int target=rand()%SIZE_OF_SORT_NODE;
		tmp=sort_set[target];
		sort_set[target]=sort_set[i];
		sort_set[i]=tmp;
	}
	/*E reserve position */
	for(i=0;i<SIZE_OF_SORT_NODE;i++)
	{
		sort_set[i].data1=i;
	}
	std::printf(" Done\n");
}

static int checkSortData(void)
{
	int i;
	unsigned int prev=0;
	std::printf("Check Sorted Data ...");
	for(i=0;i<SIZE_OF_SORT_NODE;i++)
	{
		if(prev>sort_set[i].key)
		{
			std::printf("\nERROR!\n[%8d] %8d: %8d %8d %8d\n[%8d] %8d: %8d %8d %8d\n",
						 i-1,
						 sort_set[i-1].key,
						 sort_set[i-1].data1,
						 sort_set[i-1].data2,
						 sort_set[i-1].data3,
						 i,
						 sort_set[i].key,
						 sort_set[i].data1,
						 sort_set[i].data2,
						 sort_set[i].data3);
			i=SIZE_OF_SORT_NODE-1;
			std::printf("\nERROR!\n[%8d] %8d: %8d %8d %8d\n[%8d] %8d: %8d %8d %8d\n",
						 i-1,
						 sort_set[i-1].key,
						 sort_set[i-1].data1,
						 sort_set[i-1].data2,
						 sort_set[i-1].data3,
						 i,
						 sort_set[i].key,
						 sort_set[i].data1,
						 sort_set[i].data2,
						 sort_set[i].data3);
			return SORT_NOT_SUCCESS;
		}
		prev=sort_set[i].key;
	}
	std::printf(" done\n");
	return CELL_OK;
}

/*E Common area for MergeSort ********************************************************/

class MergeSortCommunication:public SpuCommunication
{
	typedef struct _CommonInfo
	{
		unsigned int spu_count;
		unsigned int dummy1[3];

		unsigned int data_start;
		unsigned int data_end;
		unsigned int work_start;
		unsigned int work_end;

		unsigned char status[8];
	}CommonInfo __attribute__((aligned(128)));

	CommonInfo common_info;

public:
	virtual void Initialize(int);
	virtual void DebugPrint();
	virtual uint64_t CommunicationEA(){return reinterpret_cast<uint64_t>(&common_info);}
};

void MergeSortCommunication::Initialize(int count)
{
	common_info.spu_count=count;
	common_info.data_start = (unsigned int)&sort_set[0];
	common_info.data_end   = (unsigned int)&sort_set[SIZE_OF_SORT_NODE];
	common_info.work_start = (unsigned int)&sort_work[0];
	common_info.work_end   = (unsigned int)&sort_work[SIZE_OF_SORT_NODE];
	for(int i=0;i<8;i++)
	{
		common_info.status[i]=0;
	}
}
void MergeSortCommunication::DebugPrint()
{
	std::printf("status: 0[%3d] 1[%3d] 2[%3d] 3[%3d] 4[%3d] 5[%3d] 6[%3d] 7[%3d]\n"
				 ,common_info.status[0],common_info.status[1]
				 ,common_info.status[2],common_info.status[3]
				 ,common_info.status[4],common_info.status[5]
				 ,common_info.status[6],common_info.status[7]);
}

int main (int argc, char *argv[])
{
	static MergeSortCommunication msca;
	system_time_t beginTime;
	float msec;
	int ret;
	(void)argc;
	(void)argv;

	InitializeSortData();

	printf("initializing communication\n");

	msca.Initialize(4);

	Spurs* spurs = (Spurs*)std::memalign(Spurs::ALIGN, sizeof(Spurs));
    Taskset* taskset = (Taskset*)std::memalign(Taskset::ALIGN, sizeof(Taskset));
	Task* task[4];
	for (size_t i = 0; i < 4; i++) {
		task[i] = (Task*)std::memalign(Task::ALIGN, sizeof(Task));
	}

	ret = spu_printf_initialize(Spurs::DEFAULT_PPU_THREAD_PRIORITY + 1, 0);
	assert(ret == CELL_OK);

	// Step.1 initialize SPURS
	ret = Spurs::initialize(spurs, "sample");
	assert(ret == CELL_OK);
    // Step.2 create SPURS taskset
	ret = Taskset::create(taskset, "sample", spurs, 0);
	assert(ret == CELL_OK);
    // Step.3 create SPURS task
	beginTime = _statBegin(0x10);
	CellSpursTaskArgument arg;
	arg.u64[1] = msca.CommunicationEA();
	for (size_t i = 0; i < 4; i++) {
		arg.u64[0] = (uint64_t)i;
		ret = Task::create(task[i], taskset, _binary_spu_merge_sort_spu_elf_start, arg);
		assert(ret == CELL_OK);
	}

	// finalize SPURS
	for (size_t i = 0; i < 4; i++) {
		ret = task[i]->join();
		assert(ret == CELL_OK);
		std::free(task[i]);
	}

	ret = taskset->shutdown();
	assert(ret == CELL_OK);
    ret = taskset->join();
	assert(ret == CELL_OK);
	std::free(taskset);

    ret = spurs->finalize();
	assert(ret == CELL_OK);
	std::free(spurs);

	msec = _statEnd(0x11, beginTime);

	checkSortData();

	//std::printf("Merge Sort(sample number %d) %12f msec\n", SIZE_OF_SORT_NODE, msec);

	spu_printf_finalize();
	std::printf("SPURS task finished\n");
    return 0;
}

/*
 * Local Variables:
 * mode: C
 * c-file-style: "stroustrup"
 * tab-width: 4
 * End:
 * vim:sw=4:sts=4:ts=4
 */
