/*   SCE CONFIDENTIAL                                       */
/*   PlayStation(R)3 Programmer Tool Runtime Library 400.001 */
/*   Copyright (C) 2004 Sony Computer Entertainment Inc.    */
/*   All Rights Reserved.                                   */
/* File: SPU_mmio.c
 * Description:
 *   This sample shows how to pass a signal to another SPU thread in the same
 *   group. 
 * 
 *   Loaded on an SPU thread, this program receives a signal from the previous
 *   SPU thread and sends a signal to the next SPU thread in the group.  
 *   ("Next" SPU thread means that one whose spu_num equals spu_num of this SPU
 *   thread incremented by one.)  Everytime a signal is passed to the next SPU 
 *   thread, the signal is incremented by one.
 *
 *   Except on an SPU thread with spu_num = 0, this program reads the signal
 *   notification 1 channel.  This is read-blocking enabled channel, so it
 *   blocks until it receives a signal.  If a signal is received, the receiving
 *   SPU increments it by one, and passes it to the next SPU thread via MMIO. 
 *   If it is the last SPU thread, pass the signal to the first SPU thread.
 *
 *   If this program is loaded on the SPU thread with spu_num = 0, its behavior
 *   is slightly different.  This program initialize the signal to 1, and 
 *   sends it over to the 2nd SPU thread, and wait for a signal from the last 
 *   SPU thread.  Because the signal is incremented by one, the resultant 
 *   signal should equal the total number of SPU threads.  After reciving 
 *   the signal, the first SPU thread sends the resultant value to the PPU 
 *   as an event.
 */

#include <spu_intrinsics.h>
#include <sys/spu_thread.h>   
#include <sys/spu_event.h>
#include <stdio.h>

#define SIGNAL_1_OFFSET  0x1400C
#define DMA_TAG_0              0
#define SPU_THREAD_PORT        58

#ifndef MFC_GET_CMD
#define MFC_GET_CMD 0x40
#endif /* MFC_GET_CMD */
#ifndef MFC_PUT_CMD
#define MFC_PUT_CMD 0x20
#endif /* MFC_PUT_CMD */
#ifndef MFC_PUTR_CMD
#define MFC_PUTR_CMD 0x30
#endif /* MFC_PUTR_CMD */

/*
 * The DMA buffer is aligned to 16 bytes as sepcified by BPA. 
 */
unsigned int value[4]__attribute__((aligned(16)));

int main(unsigned int total, unsigned int spu_num)
{
    unsigned int sig_val;
        
    /*
     * If this SPU thread's number is 0, initialize the buffer to 0.
     * Otherwise, wait for a signal.
     */
    if (spu_num != 0) {
        sig_val = spu_readch(SPU_RdSigNotify1);
        value[3] = sig_val + 1;
    } else {
        value[3] = 1;
    }

    /*
     * Write to a Signal Notification Reg 1 of the next SPU thread.
     * For example, if this SPU thread's number is 1 and there are 3 SPU
     * threads in total, sends a signal to an SPU thread with spu_num = 2. 
     */
    unsigned int target_spu_num = (spu_num + 1) % total;
    unsigned int snr1_addr = SYS_SPU_THREAD_OFFSET * target_spu_num 
        + SYS_SPU_THREAD_BASE_LOW + SYS_SPU_THREAD_SNR1;

    /*
     * The lowest 4 bits of the local-storage address and the effective 
     * address must be the same.   
     * value[0] is aligned to 16 bytes, and the lowest 5 bits are 0x0.
     * Thus, value[3]'s lowest 5 bits are 0xC.
     * This is necessary because the offset of singal notification register 1 
     * is 0x1400C.
     */
    spu_mfcdma32(&value[3], snr1_addr, sizeof(unsigned int), DMA_TAG_0, 
                 MFC_PUT_CMD);

    /*
     * If this SPU thread's number is 0, sends the result to the PPU as an event
     */
    if (spu_num == 0) {
        sig_val = spu_readch(SPU_RdSigNotify1);
		sys_spu_thread_send_event(SPU_THREAD_PORT, sig_val, 0);
    }

    sys_spu_thread_exit(0);
}

