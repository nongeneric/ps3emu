/*  SCE CONFIDENTIAL                                         */
/*  PlayStation(R)3 Programmer Tool Runtime Library 400.001  */
/*  Copyright (C) 2006 Sony Computer Entertainment Inc.      */
/*  All Rights Reserved.                                     */

/**
 *E  Sample: performance/ppu_spu_round_trip/common
 *
 *   File: ppu/spu_thread.h
 *
 *   Description:
 *     Settings of a SPU thread to measure the performance
 */

#ifndef __SAMPLE_ROUND_TRIP_PPU_SPU_THREAD_H__
#define __SAMPLE_ROUND_TRIP_PPU_SPU_THREAD_H__

#include <sys/spu_thread_group.h>
#include <sys/spu_thread.h>
#include <sys/event.h>

/*E SPU thread Settings */
extern sys_spu_thread_group_t gSpuThreadGroupId;
extern sys_spu_thread_t gSpuThreadId;
extern sys_event_queue_t gEventQueueId;

/*E The exit cause and status of the SPU thread group */
extern int gSpuThreadGroupExitCause;
extern int gSpuThreadGroupExitStatus;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*E SPU thread Settings */
int prepareMeasurementSpuThread(void);
int cleanupMeasurementSpuThread(void);

/*E The exit cause and status of the SPU thread group */
void showSpuThreadGroupExitCauseAndStatus(void);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __SAMPLE_ROUND_TRIP_PPU_SPU_THREAD_H__ */

/*
 * Local Variables:
 * mode:C
 * tab-width:4
 * End:
 * vim:sw=4:sts=4:ts=4
 */
