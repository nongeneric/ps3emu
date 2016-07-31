// SCE CONFIDENTIAL
// PlayStation(R)3 Programmer Tool Runtime Library 360.001
// Copyright (C) 2010 Sony Computer Entertainment Inc.
// All Rights Reserved.

#include "sample_config.h"

// standard C libraries
#include <cstdio>						// printf
#include <cstdlib>						// abort
#include <cstring>						// strlen

#include <sys/spu_thread_group.h>		// SYS_SPU_THREAD_GROUP_TYPE_EXCLUSIVE_NON_CONTEXT
#include <sys/process.h>				// SYS_PROCESS_PARAM
#include <spu_printf.h>

SYS_PROCESS_PARAM(PRIMARY_PPU_THREAD_PRIORITY, PRIMARY_PPU_THREAD_STACK_SIZE);

int main(void)
{
	int ret;

	// run this sample
	ret = sample_main();
	if (ret) {
		std::printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		std::abort();
	}


	return 0;
}

// Local Variables:
// mode: C++
// c-file-style: "stroustrup"
// tab-width: 4
// End:
// vim:sw=4:sts=4:ts=4
