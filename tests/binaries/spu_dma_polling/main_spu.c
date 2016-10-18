/* SCE CONFIDENTIAL
PlayStation(R)3 Programmer Tool Runtime Library 400.001
* Copyright (C) 2007 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#include <sys/spu_thread.h>
#include <cell/dma.h>

#define TAG1	1
#define TAG2	2
#define TAG3	3
#define MASK1	(1<<TAG1)
#define MASK2	(1<<TAG2)
#define MASK3	(1<<TAG3)
#define TID		0
#define RID		0

#define BUF_NUM (128/sizeof(vec_uint4))
vec_uint4 buf1[BUF_NUM] __attribute__((aligned(128)));
vec_uint4 buf2[BUF_NUM] __attribute__((aligned(128)));
vec_uint4 buf3[BUF_NUM] __attribute__((aligned(128)));

int main(uint64_t dst1, uint64_t dst2, uint64_t dst3, uint64_t size)
{
	uint32_t count1 = 0;
	uint32_t count2 = 0;
	uint32_t count3 = 0;

	size >>= 7;							/* divided by 128 */

	uint32_t mask = MASK1|MASK2|MASK3;
	do {
		unsigned int status = cellDmaWaitTagStatusImmediate(mask);

		if ((status & MASK1) && (count1 < size)) {
			for (unsigned int i = 0; i < BUF_NUM; i++) {
				buf1[i] = spu_add(buf1[i], 1);
			}
			cellDmaPut(buf1, dst1, 128, TAG1, TID, RID);
			dst1 += 128;
			count1++;
		}
		if ((status & MASK2) && (count2 < size)) {
			for (unsigned int i = 0; i < BUF_NUM; i++) {
				buf2[i] = spu_add(buf2[i], 1);
			}
			cellDmaPut(buf2, dst2, 128, TAG2, TID, RID);
			dst2 += 128;
			count2++;
		}
		if ((status & MASK3) && (count3 < size)) {
			for (unsigned int i = 0; i < BUF_NUM; i++) {
				buf3[i] = spu_add(buf3[i], 1);
			}
			cellDmaPut(buf3, dst3, 128, TAG3, TID, RID);
			dst3 += 128;
			count3++;
		}

		/*
		 * Do other job here
		 */
	} while (count1 < size || count2 < size || count3 < size);

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
