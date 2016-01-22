/* SCE CONFIDENTIAL
PlayStation(R)3 Programmer Tool Runtime Library 400.001
* Copyright (C) 2007 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#include <sys/spu_thread.h>
#include <cell/dma.h>

#include "shared_types.h"

#define TAG		0
#define TID		0
#define RID		0

struct argument Config;

int main(uint64_t argp, uint64_t arg2, uint64_t arg3, uint64_t arg4)
{
	(void)arg2;							/* unused */
	(void)arg3;							/* unused */
	(void)arg4;							/* unused */

	cellDmaLargeGet(&Config, argp, sizeof(struct argument), TAG, TID, RID);
	cellDmaWaitTagStatusAll(1<<TAG);

	/* You have Config now */

	/* Pass back to PPU instead of "printf" here */
	cellDmaLargePut(&Config, Config.returnAddress, sizeof(struct argument), TAG, TID, RID);
	cellDmaWaitTagStatusAll(1<<TAG);

	sys_spu_thread_exit(0);
	return 0;
}

/*
 * Local Variables:
 * mode: C
 * tab-width: 4
 * End:
 * vim:sw=4:sts=4:ts=4
 */
