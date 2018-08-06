/*   SCE CONFIDENTIAL                                       */
/*   PlayStation(R)3 Programmer Tool Runtime Library 475.001 */
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
	unsigned* a = (unsigned*)0x3fe68;
	*a += 1;
	sys_spu_thread_send_event(SPU_THREAD_PORT, *a, 100);
    sys_spu_thread_exit(3);
}

