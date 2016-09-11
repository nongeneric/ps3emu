/*  SCE CONFIDENTIAL                                         */
/*  PlayStation(R)3 Programmer Tool Runtime Library 400.001  */
/*  Copyright (C) 2006 Sony Computer Entertainment Inc.      */
/*  All Rights Reserved.                                     */

/**
 *E  Sample: performance/ppu_spu_round_trip/common/
 *
 *   File: spu/status_check.h
 *
 *   Description:
 *     Status check functions for debugging
 */

#ifndef __SAMPLE_ROUND_TRIP_SPU_STATUS_CHECK_H__
#define __SAMPLE_ROUND_TRIP_SPU_STATUS_CHECK_H__

#include <stdint.h>

/*E Error Handling and Debug Settings */
#define UNEXPECTED_VALUE			(0x1000U)
#define UNEXPECTED_ATOMIC_STATUS	(0x2000U)
#define UNEXPECTED_EVENT			(0x3000U)

#define NO_DATA			(0x0U)
#define SEPARATOR		NO_DATA

#define SIZE_OF_STATUS	(12)
extern uint32_t status[SIZE_OF_STATUS];
extern uint32_t gEaDebugBuffer;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void notifyError(uint32_t errorStatus, uint32_t expected, uint32_t observed);
int checkStatus(uint32_t argc, uint32_t *argv);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __SAMPLE_ROUND_TRIP_SPU_STATUS_CHECK_H__ */

/*
 * Local Variables:
 * mode:C
 * tab-width:4
 * End:
 * vim:sw=4:sts=4:ts=4
 */
