/* SCE CONFIDENTIAL
PlayStation(R)3 Programmer Tool Runtime Library 400.001
* Copyright (C) 2007 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#include <sys/spu_thread.h>				/* sys_spu_thread_exit */
#include <cell/dma.h>
#include <cell/atomic.h>

#define TAG		0
#define TID		0
#define RID		0

uint32_t buf[128/sizeof(uint32_t)] __attribute__((aligned(128)));

static inline
void semaphore_p(uint64_t ea)
{
	do {} while (cellAtomicTestAndDecr32(buf, ea) == 0);
}

static inline
void semaphore_v(uint64_t ea)
{
	cellAtomicIncr32(buf, ea);
}

int main(uint64_t semaphore, uint64_t result, uint64_t c_, uint64_t arg4)
{
	int challenge = c_;

	(void)arg4;

	for (int i = 0; i < challenge; i++) {
		semaphore_p(semaphore);

		/* do some mutually excluded job */
		uint32_t r = cellDmaGetUint32(result, TAG, TID, RID);
		cellDmaPutUint32(r + 1, result, TAG, TID, RID);

		semaphore_v(semaphore);
	}

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
