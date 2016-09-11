/*
 *   SCE CONFIDENTIAL                                      
 *   PlayStation(R)3 Programmer Tool Runtime Library 400.001
 *   Copyright (C) 2005 Sony Computer Entertainment Inc.   
 *   All Rights Reserved.                                  
 *
 * File: event_sender.spu.c
 * Description:
 *   This SPU thread sends events to an event queue.
 *   Note that the SPU thread event port number is one that has been associated
 *   with the event queue by sys_event_queue_create().
 */

#include <sys/spu_thread.h>
#include <sys/spu_event.h>
#include <stdint.h>

#define SPU_THREAD_PORT        58

int main(uint32_t data1, uint32_t data2)
{
	sys_spu_thread_send_event(SPU_THREAD_PORT, data1, data2);

	sys_spu_thread_exit(0);
}

