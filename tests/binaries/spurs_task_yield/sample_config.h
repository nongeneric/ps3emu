// SCE CONFIDENTIAL
// PlayStation(R)3 Programmer Tool Runtime Library 400.001
// Copyright (C) 2010 Sony Computer Entertainment Inc.
// All Rights Reserved.

#ifndef __SAMPLE_CONFIG__
#define __SAMPLE_CONFIG__ 1

#define SAMPLE_NAME						"sample_spurs_yield"
#define NUM_SPU							1
#define NUM_TASK						5

#define SPURS_MAX_SPU					6
#define SPURS_PPU_THREAD_PRIORITY		2
#define SPURS_SPU_THREAD_PRIORITY		100
#define SPURS_PREFIX					"Sample"

#define PRIMARY_PPU_THREAD_PRIORITY		1001
#define PRIMARY_PPU_THREAD_STACK_SIZE	65536

#define SPU_PRINTF_HANDLER_PRIORITY		999

#include <cell/spurs.h>
int sample_main(cell::Spurs::Spurs *spurs);

#endif // __SAMPLE_CONFIG__

// Local Variables:
// mode: C++
// c-file-style: "stroustrup"
// tab-width: 4
// End:
// vim:sw=4:sts=4:ts=4
