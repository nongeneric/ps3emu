/*
 *   SCE CONFIDENTIAL                                      
 *   PlayStation(R)3 Programmer Tool Runtime Library 400.001
 *   Copyright (C) 2005 Sony Computer Entertainment Inc.   
 *   All Rights Reserved.                                  
 *
 * File: event_receiver.spu.c
 * Description:
 *   This SPU thread receives events from the PPU thread until receiving
 *   EVENT_TERMINATE value which indicates the end of a series of events.
 *   Eventually, the SPU thread which receives EVENT_TERMINATE terminates the
 *   SPU thread group.
 */

#include <sys/spu_thread.h>
#include <sys/spu_event.h>

#define SPU_QUEUE_NUMBER    16850944
#define EVENT_TERMINATE 0xfee1dead

/* 
 * main()
 * The first parameter is the SPU queue number.
 * This SPU thread receives events until receiving an event which stores 
 * EVENT_TERMINATE.  If the EVENT_TERMINATE event is received, this SPU thread
 * exits.  The exit status equals to the sum of all received event data.
 */
int main(int spuq) 
{
	int sum = 0;
	uint32_t data1, data2, data3;
	while (1) {
		sys_spu_thread_receive_event(spuq, &data1, &data2, &data3);
		if (data1 == EVENT_TERMINATE) {
			break;
		}
		sum = sum + data1 + data2 + data3;
	}  

	sys_spu_thread_exit(sum);
}

