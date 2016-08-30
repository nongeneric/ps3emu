/* SCE CONFIDENTIAL
PlayStation(R)3 Programmer Tool Runtime Library 400.001
* Copyright (C) 2007 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#include <sys/spu_thread.h>
#include <cell/sync.h>

#include "../common.h"

#define PARA_NUM 8

int main(uint64_t spu_num, uint64_t queue1, uint64_t queue2) 
{
	(void)spu_num;

	static vec_uint4 buffer[PARA_NUM];
	CellSyncLFQueuePopContainer  pop [PARA_NUM];
	CellSyncLFQueuePushContainer push[PARA_NUM];
	for(int i = 0; i < PARA_NUM; i++) {
		cellSyncLFQueuePopContainerInitialize(&pop[i],
											  (void *)&buffer[i],
											  i); /* tag */
		cellSyncLFQueuePushContainerInitialize(&push[i],
											   (const void *)&buffer[i],
											   i+PARA_NUM); /* tag */
	}

	int ret;
	int in1=0,in2=0,out1=0,out2=0;
	do {
		while((in1 < ITERATION) && (in1 - out2 < PARA_NUM)) {
			ret = cellSyncLFQueueTryPopBegin(queue1, &pop[in1%PARA_NUM]);
			if (ret != CELL_OK) break;
			in1++;
		}
		if (in1 > in2) {
			cellSyncLFQueuePopEnd(queue1, &pop[in2%PARA_NUM]);
			in2++;
		}
		while(in2 > out1) {
			ret = cellSyncLFQueueTryPushBegin(queue2, &push[out1%PARA_NUM]);
			if (ret != CELL_OK) break;
			out1++;
		}
		if (out1 > out2) {
			cellSyncLFQueuePushEnd(queue2, &push[out2%PARA_NUM]);
			out2++;
		}
	} while(out2 != ITERATION);

	sys_spu_thread_exit(0);

}

/*
 * Local Variables:
 * mode:C
 * c-file-style: "stroustrup"
 * tab-width:4
 * End:
 * vim:ts=4:sw=4:
 */
