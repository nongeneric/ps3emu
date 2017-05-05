/* SCE CONFIDENTIAL
 PlayStation(R)3 Programmer Tool Runtime Library 400.001
 * Copyright (C) 2009 Sony Computer Entertainment Inc.
 * All Rights Reserved.
 */

/* standard C++ header */
#include <stdint.h>
#include <assert.h>

#include <spu_intrinsics.h>
#include <spu_printf.h>

#include <cell/dma.h>

/* SPURS */
#include <cell/spurs.h>

#include "../sample_sync_command.h"

using namespace sample_sync_command;

#define	DDPRINTF(...)

void cellSpursJobQueueMain(CellSpursJobContext2* ctx, CellSpursJob256 *job)
{
	uint32_t	eaJobDescriptor = ctx->eaJobDescriptor;
	int	ret;

	DDPRINTF("@@@@@@@@Job(Desc = %08x) is started\n", eaJobDescriptor);

	SampleSyncJobDescriptor*	desc = (SampleSyncJobDescriptor*)job;
	uint32_t	eaSuspendData = desc->eaSuspendData;
	uint32_t	eaQueue = desc->eaRequestQueue;
	uint32_t	eaMyId = (eaSuspendData == 0) ? 
					eaJobDescriptor : eaSuspendData;
	uint32_t	index = desc->myNumber;
	uint32_t	eaWorkArea = desc->eaOutputWorkArea;

	WorkAreaEntry*	data = (WorkAreaEntry*)ctx->ioBuffer;
	/* check the input data consistency */
	uint32_t	myCount = data[index].count;
	for (unsigned i = 0; i < NUM_SYNC_JOBS; i++) {
		//TEST_ASSERT_EQUAL32(myCount, data[i].count);
		if(myCount != data[i].count) {
			spu_printf("[%08x][eaSuspendData = %08x[index = %d]myCount = %d, data[%d] = %d\n", eaJobDescriptor, eaSuspendData, index, myCount, i, data[i].count);
			si_stop(1);
		}
		/* increment count */
		data[i].count++;
	}
	/* for testing, insert delay */
	if(eaSuspendData) {
		DDPRINTF("Job(Desc = %08x)[cout = %d] send signal from PPU(myId = %08x)\n", eaJobDescriptor, myCount, eaMyId);
		PpuRequest	request;
		request.type = REQUEST_TYPE_SIGNAL;
		request.idRequester = eaMyId;
		request.idRequest = myCount;
	
		CellSpursLFQueuePushContainer	container;
		cellSpursLFQueuePushContainerInitialize(&container, &request, ctx->dmaTag);
		ret = cellSpursLFQueueTryPushBegin(eaQueue, &container);
		assert(ret == CELL_OK);
		ret = cellSpursLFQueuePushEnd(eaQueue, &container);
		assert(ret == CELL_OK);

		/* wait for signal received */
		ret = cellSpursJobQueueWaitSignal(eaSuspendData);
		assert(ret == CELL_OK);
		DDPRINTF("Job(Desc = %08x)[count = %d] returned from wait signal\n", eaJobDescriptor, myCount);
	}

	/* scatter the result */
	CellDmaListElement*	element = (CellDmaListElement*)ctx->oBuffer;
	__builtin_memset(element, 0, sizeof(CellDmaListElement) * NUM_SYNC_JOBS);
	for (unsigned i = 0; i < NUM_SYNC_JOBS; i++) {
		element[i].size = sizeof(WorkAreaEntry);
		element[i].eal = eaWorkArea + sizeof(WorkAreaEntry) * (NUM_SYNC_JOBS * i + index); 
	}
	cellDmaListPut(data, 0, element, 8 * NUM_SYNC_JOBS, ctx->dmaTag, 0, 0);

	DDPRINTF("@@@@@@@@Job(Desc = %08x) ######## End of Job count = %d\n", eaJobDescriptor, myCount);
}

/*
 * Local Variables:
 * mode:C
 * c-file-style: "stroustrup"
 * tab-width:4
 * End:
 * vim:ts=4:sw=4:
 */
