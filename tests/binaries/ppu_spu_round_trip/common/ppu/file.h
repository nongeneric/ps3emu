/*  SCE CONFIDENTIAL                                         */
/*  PlayStation(R)3 Programmer Tool Runtime Library 400.001  */
/*  Copyright (C) 2006 Sony Computer Entertainment Inc.      */
/*  All Rights Reserved.                                     */

/**
 *E  Sample: performance/ppu_spu_round_trip/common
 *
 *   File: ppu/file.h
 *
 *   Description:
 *     Functions to read/write files on the host.
 */

#ifndef __SAMPLE_ROUND_TRIP_PPU_FILE_H__
#define __SAMPLE_ROUND_TRIP_PPU_FILE_H__

#define FILE_PATH_SIZE	256
extern char gSpuSelfFilePath[FILE_PATH_SIZE];
extern char gLogFilePath[FILE_PATH_SIZE];

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

int recordObservedValuesOnFile(void);

int makeFilePath(char *buffer, const char *fileName, const char *mountPoint);

int makePpuSideRoundTripLogFilePath(
				const char *prefix,	uint32_t ppuToSpu, uint32_t spuToPpu);
int makeSpuSideRoundTripLogFilePath(
				const char *prefix,	uint32_t ppuToSpu, uint32_t spuToPpu);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* __SAMPLE_ROUND_TRIP_PPU_FILE_H__ */

/*
 * Local Variables:
 * mode:C
 * tab-width:4
 * End:
 * vim:sw=4:sts=4:ts=4
 */
