/* SCE CONFIDENTIAL
PlayStation(R)3 Programmer Tool Runtime Library 400.001
* Copyright (C) 2007 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#include <sys/spu_thread.h>				/* sys_spu_thread_exit */
#include <cell/atomic.h>

uint64_t buf[128/sizeof(uint64_t)] __attribute__((aligned(128)));

static inline
uint64_t myAtomicXor64(uint64_t ea, uint64_t value)
{
	uint64_t old;

	do {
		old = cellAtomicLockLine64(buf, ea);
	} while (__builtin_expect(cellAtomicStoreConditional64(buf, ea, old ^ value), 0));
	return old;
}

int main(uint64_t value, uint64_t mask, uint64_t arg3, uint64_t arg4)
{
	(void)arg3;
	(void)arg4;

	for (int i = 0; i < 120; ++i) {
		myAtomicXor64(value, (mask * 2 + 10 * i) << mask);
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
