/* 
 *  SCE CONFIDENTIAL
 *   PlayStation(R)3 Programmer Tool Runtime Library 400.001 
 *   Copyright (C) 2004 Sony Computer Entertainment Inc.    
 *   All Rights Reserved.
 */                                   

/*E
 * File: dma_sync.spu.c
 * Decription:
 *  This program is loaded to an SPU thread which is created by 
 *  "spu_thr_dma_sync".
 */

#include <spu_intrinsics.h>
#include <sys/spu_thread.h>
#include <sys/spu_event.h>
#include <stdint.h>
#include "spu_thr_dma_sync.h"

#define TRANSFER_SIZE 1024
#define ITERATION     15
#define SPU_THREAD_PORT        58

unsigned volatile int
 
	dma_buffer[TRANSFER_SIZE / sizeof(unsigned int)]
	__attribute__ ((aligned(128)));
unsigned int signal_buffer[16 / sizeof(unsigned int)]
	__attribute__ ((aligned(128)));

#define SNR_1         0x1400c

#ifndef MFC_GET_CMD
#define MFC_GET_CMD 0x40
#endif /* MFC_GET_CMD */
#ifndef MFC_PUT_CMD
#define MFC_PUT_CMD 0x20
#endif /* MFC_PUT_CMD */
#ifndef MFC_PUTR_CMD
#define MFC_PUTR_CMD 0x30
#endif /* MFC_PUTR_CMD */

void wait_dma_completion(unsigned int tag);
void write_to_ls(int spu_num, unsigned int offset);
void read_from_pas(uint64_t ea);
void write_to_pas(uint64_t ea);
void send_event_to_pu(void);
void send_signal(int spu_num, unsigned int value);
unsigned int wait_signal(void);
void increment_mark(unsigned int marker);

/*E
 * Wait for completion of DMA commands with the specified tag number.
 * The sequence of commands executed in this function is described in 
 * BPA Book I-III 8.9 "SPU Tag Group Status Channles".
 *
 * tag: DMA tag 
 */
void wait_dma_completion(unsigned int tag)
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

/*E
 * MMIO-write to LS address "offset" of another SPU thread.
 * The SPU thread is specified by "spu_num".  The target SPU thread and 
 * caller SPU thread must belong to the same SPU thread group. 
 * 
 * The size of transfer is statically defined by the TRANSFER_SIZE macro.
 * 
 * spu_num: SPU number in the SPU thread group
 * offset:  Offset from the local-storage base address
 */
void write_to_ls(int spu_num, unsigned int offset)
{
	unsigned int ls_base_low =
		spu_num * SYS_SPU_THREAD_OFFSET + SYS_SPU_THREAD_BASE_LOW +
		SYS_SPU_THREAD_LS_BASE;
	unsigned int ls_base_high = 0x0;
	unsigned int tag = 0x0;

	spu_mfcdma64(dma_buffer, ls_base_high, ls_base_low + offset,
				 TRANSFER_SIZE, tag, MFC_PUT_CMD);
	wait_dma_completion(tag);
}

/*E
 * Send a singal to another SPU thread via MMIO.
 * 
 * spu_num: SPU number in the SPU thread group
 * value: a value in the signal
 */
void send_signal(int spu_num, unsigned int value)
{
	unsigned int signal_base_low =
		spu_num * SYS_SPU_THREAD_OFFSET + SYS_SPU_THREAD_BASE_LOW +
		SYS_SPU_THREAD_SNR1;
	unsigned int signal_base_high = 0x0;
	unsigned int tag = 0x0;

	signal_buffer[3] = value;
	spu_mfcdma64(&(signal_buffer[3]), signal_base_high, signal_base_low,
				 4, tag, MFC_PUT_CMD);
	wait_dma_completion(tag);
}

/*E
 * Wait until receiving a signal.
 * Return the received singal value.
 */
unsigned int wait_signal(void)
{
	return spu_readch(SPU_RdSigNotify1);
}

/*E
 * 
 */
void read_from_pas(uint64_t ea)
{
	unsigned int pas_base_low = (unsigned int)(ea & 0x00000000FFFFFFFFUL);
	unsigned int pas_base_high = (unsigned int)(ea >> 32);
	unsigned int tag = 0;

	spu_mfcdma64(dma_buffer, pas_base_high, pas_base_low,
				 TRANSFER_SIZE, tag, MFC_GET_CMD);
	wait_dma_completion(tag);
}

/*E
 * Write to an effective address in the main memory by DMA.
 * The size of transfer is statically defined by the TRANSFER_SIZE macro.
 * 
 * eal: target effective address
 */
void write_to_pas(uint64_t ea)
{
	unsigned int pas_base_low = (unsigned int)(ea & 0x00000000FFFFFFFFUL);
	unsigned int pas_base_high = (unsigned int)(ea >> 32);
	unsigned int tag = 0;

	spu_mfcdma64(dma_buffer, pas_base_high, pas_base_low,
				 TRANSFER_SIZE, tag, MFC_PUTR_CMD);
	wait_dma_completion(tag);
}

/*E
 * Send an event
 */
void send_event_to_pu(void)
{
	sys_spu_thread_send_event(SPU_THREAD_PORT, 0, 0);
}

/*E
 * Incremnt all values in dma_buffer by "marker".
 */
void increment_mark(unsigned int marker)
{
	int i;
	int buf_size = TRANSFER_SIZE / sizeof(unsigned int);
	for (i = 0; i < buf_size; i++) {
		dma_buffer[i] += marker;
	}
}

/*E
 * main()
 *  ea: Effective address of the DMA buffer in main memory
 */
int main(int my_spu_num, int friend_spu_num, uint64_t ea)
{
	int i;

	/*E
	 * If this SPU thread's SPU number is 0,
	 *  1. Wait for a signal. (PU is supposed to send it.)
	 *  2. Read from the buffer in main memory to the local storage.
	 *  3. Increment the fetched values 
	 *  4. Write the values to the friend's local storage
	 *  5. Send a signal to friend that data has been sent.
	 *
	 * If this SPU thread's SPU number is 1,
	 *  1. Wait until a signal. (The friend SPU thread is supposed to send it.
	 *     It notifies that the friend has sent data to my buffer.
	 *  2. Increment the data in the buffer.
	 *  3. Write the data to PPU's main memory.
	 *  4. Send an event to PPU which notifies data has been sent.
	 */
	if (my_spu_num == 0) {
		for (i = 0; i < ITERATION; i++) {
			wait_signal();
			read_from_pas(ea);
			increment_mark(SPU_MARKER_0);
			write_to_ls(friend_spu_num, (unsigned int)dma_buffer);
			send_signal(friend_spu_num, SPU_MARKER_0);
		}
	} else if (my_spu_num == 1) {
		for (i = 0; i < ITERATION; i++) {
			wait_signal();
			increment_mark(SPU_MARKER_1);
			write_to_pas(ea);
			send_event_to_pu();
		}
	} else {
		sys_spu_thread_group_exit(-1);
	}

	sys_spu_thread_exit(0);
}

