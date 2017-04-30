/* SCE CONFIDENTIAL
PlayStation(R)3 Programmer Tool Runtime Library 360.001
* Copyright (C) 2007 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#include <sys/spu_thread.h>
#include <stdint.h>
#include <cell/sync.h>
#include <cell/dma.h>
#include <cellstatus.h>
#include <string.h>
#include <stdio.h>

#include <spu_internals.h>
#include <spu_intrinsics.h>

uint32_t count_array[4] __attribute__ ((aligned (128)));
char message_buffer[128] __attribute__ ((aligned (128)));
uint32_t tag = 3;
uint64_t u64buffer[8] __attribute__ ((aligned (128)));
vector unsigned int outputsBuffer[16] __attribute__ ((aligned (128)));

char buf[] = {
	1, 2, 3, 4, 5, 6, 7, 8,
	9, 10, 11, 12, 13, 14, 15,
	16, 17, 18, 19, 20, 21, 22,
	23, 24, 25, 26, 27, 28, 29,
	30, 31, 32, 33, 34, 35, 36
};

uint32_t select_f(float f1, float f2, float f3) {
	if (f1 < f2)
		return 1;
	if (f2 == f3)
		return 2;
	if (f2 + f3 > 0)
		return 3;
	return 4;
}

uint32_t select_u(int32_t f1, int32_t f2, int32_t f3) {
	if (f1 < f2)
		return 1;
	if (f2 == f3)
		return 2;
	if (f2 + f3 > 0)
		return 3;
	return 4;
}

int main(uint64_t spu_num, uint64_t u64s, uint64_t outputs, uint64_t message) 
{
	snprintf((char*)message_buffer, 128, "! %d !", 10);

	cellDmaGet(u64buffer, u64s, 64, tag, 0, 0);
	cellDmaWaitTagStatusAll(1<<tag);

	cellDmaGet(outputsBuffer, outputs, sizeof(outputsBuffer), tag, 0, 0);
	cellDmaWaitTagStatusAll(1<<tag);

	qword* v = (qword*)outputsBuffer;

	int i_1[] = { 0, 0, 500, -500 };
	float f_1[] = { 0, 10, -10, 200 };
	float f_2[] = { 0, 1000000.f, -10, 100000000000.f };
	qword i128_1 = *(qword*)i_1;
	qword f128_1 = *(qword*)f_1;
	qword f128_2 = *(qword*)f_2;

	int i = 0;

	v[i++] = si_csflt(i128_1, 0);
	v[i++] = si_csflt(i128_1, 1);
	v[i++] = si_csflt(i128_1, 2);
	v[i++] = si_csflt(i128_1, 4);

	i = 4;

	v[i++] = si_cflts(f128_1, 0);
	v[i++] = si_cflts(f128_1, 1);
	v[i++] = si_cflts(f128_1, 2);
	v[i++] = si_cflts(f128_1, 120);
	
	i = 8;

	v[i++] = si_cuflt(i128_1, 0);
	v[i++] = si_cuflt(i128_1, 1);
	v[i++] = si_cuflt(i128_1, 2);
	v[i++] = si_cuflt(i128_1, 4);

	i = 12;

	v[i++] = si_cfltu(f128_2, 0);
	v[i++] = si_cfltu(f128_2, 1);
	v[i++] = si_cfltu(f128_2, 2);
	v[i++] = si_cfltu(f128_2, 8);

	i = 16;

	cellDmaPut(message_buffer, message, 128, tag, 0, 0);
	cellDmaWaitTagStatusAll(1<<tag);

	cellDmaPut(outputsBuffer, outputs, sizeof(outputsBuffer), tag, 0, 0);
	cellDmaWaitTagStatusAll(1<<tag);

	return 0;
}
