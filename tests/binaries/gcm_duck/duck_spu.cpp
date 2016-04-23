/*   SCE CONFIDENTIAL
 *   PlayStation(R)3 Programmer Tool Runtime Library 400.001
 *   Copyright (C) 2006 Sony Computer Entertainment Inc.
 *   All Rights Reserved. 
 */

#include <spu_intrinsics.h>

#define BUF_SIZE  128
#define ALIGN16(x)         ((((x)+15)/16)*16)

typedef struct
{
	unsigned int  fragment_constant_addr;
	unsigned int  spu_read_label_addr;
	unsigned int  spu_write_label_addr;
} SpuParam_t;

SpuParam_t spuparam __attribute__ ((aligned(128)));


/*E
 * 128-byte aligned DMA buffer
 */
float dma_buffer[BUF_SIZE / sizeof(float)] __attribute__ ((aligned(128)));
unsigned int read_label_buffer[16] __attribute__ ((aligned(128)));
unsigned int write_label_buffer[16] __attribute__ ((aligned(128)));

static void wait_dma_completion(unsigned int tag)
{
	/*Clear MFC tag update */
	spu_writech(MFC_WrTagUpdate, 0x0);
	for (; spu_readchcnt(MFC_WrTagUpdate) != 1;);
	spu_readch(MFC_RdTagStat);
	/* Set tag update and wait for completion */
	spu_writech(MFC_WrTagMask, (1 << tag));
	spu_writech(MFC_WrTagUpdate, 0x2);
	spu_readch(MFC_RdTagStat);
}

static inline void mfcDma(void* ls, unsigned int ea, unsigned int size, unsigned int cmd)
{
	while (size) {
		unsigned int chunk = size > 16*1024 ? 16*1024 : size;
		spu_mfcdma32(ls, ea, chunk, 0, cmd);
		wait_dma_completion(0);
		size -= chunk;
		ls = (void*)((unsigned int)ls + chunk);
		ea += chunk;
	}
}

static inline void mfcGetData(void* ls, unsigned int ea, unsigned int size)
{
	mfcDma(ls, ea, size, 0x40);
}

static inline void mfcPutData(void* ls, unsigned int ea, unsigned int size)
{
	mfcDma(ls, ea, size, 0x20);
}

static inline unsigned int swapU32_16(const unsigned int v)
{
    return (v>>16) | (v<<16);
}

static inline float swapF32_16(const float f)
{
    union SwapF32_16
    {
        unsigned int ui;
        float f;
    } v;
    v.f = f;
    v.ui = swapU32_16(v.ui);
    return v.f;
}


float Light[4] = {0.3f, 0.8f, 1.0f, 0.0f};
int main(void)
{
	unsigned int buff = (unsigned int)spu_readch(SPU_RdInMbox);
	mfcGetData((void*)&spuparam, buff, ALIGN16(sizeof(SpuParam_t)));
	while (1) {
		// get destination addr
		unsigned int buff_ea = (unsigned int)spuparam.fragment_constant_addr;
	
		// create command on ls
		float* ptr = dma_buffer;
		*ptr++ = swapF32_16(Light[0]);
		*ptr++ = swapF32_16(Light[1]);
		*ptr++ = swapF32_16(Light[2]);
		*ptr++ = swapF32_16(Light[3]);

		// send created commands to ea
		unsigned int size = (((unsigned int)ptr - (unsigned int)dma_buffer + 15)/16)*16;

		// wait notify from RSX
		while (1) {
			mfcGetData(read_label_buffer, spuparam.spu_read_label_addr, 16);
			if (read_label_buffer[0] == 0xbeefbeef) {
				for ( int i = 0; i < 4; i++)
					read_label_buffer[i] = 0xcafecafe;
				// clear the buffer
				mfcPutData(read_label_buffer, spuparam.spu_read_label_addr, 16);
				break;
			}
		}
		mfcPutData(dma_buffer, buff_ea, size);

		// notify to RSX
		write_label_buffer[0] = 0xabcdabcd;
		write_label_buffer[1] = 0xabcdabcd;
		write_label_buffer[2] = 0xabcdabcd;
		write_label_buffer[3] = 0xabcdabcd;
		mfcPutData(write_label_buffer, spuparam.spu_write_label_addr, 16);
	}
	return 0;
}

