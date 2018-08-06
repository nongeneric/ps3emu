// SCE CONFIDENTIAL
// PlayStation(R)3 Programmer Tool Runtime Library 475.001
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

	// initialize spu_printf server
	ret = spu_printf_initialize(SPU_PRINTF_HANDLER_PRIORITY, 0);
	if (ret) {
		std::printf("Error: spu_printf_initialize(): %#x\n", ret);
		std::printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		std::abort();
	}

	// initialize SPURS attribute
	cell::Spurs::SpursAttribute attribute;
	ret = cell::Spurs::SpursAttribute::initialize(&attribute, SPURS_MAX_SPU - 1, SPURS_SPU_THREAD_PRIORITY, SPURS_PPU_THREAD_PRIORITY, false);
	if (ret) {
		std::printf("Error: cell::Spurs::SpursAttribute::initialize(): %#x\n", ret);
		std::printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		std::abort();
	}

	ret = attribute.setNamePrefix(SPURS_PREFIX, std::strlen(SPURS_PREFIX));
	if (ret) {
		std::printf("Error: attribute.setNamePrefix(): %#x\n", ret);
		std::printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		std::abort();
	}

	ret = attribute.setSpuThreadGroupType(SYS_SPU_THREAD_GROUP_TYPE_EXCLUSIVE_NON_CONTEXT);
	if (ret) {
		std::printf("Error: attribute.setSpuThreadGroupType(): %#x\n", ret);
		std::printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		std::abort();
	}

	ret = attribute.enableSpuPrintfIfAvailable();
	if (ret) {
		std::printf("Error: attribute.enableSpuPrintfIfAvailable(): %#x\n", ret);
		std::printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		std::abort();
	}

	// allocate memory
	cell::Spurs::Spurs2 *spurs = new cell::Spurs::Spurs2;
	if (spurs == 0) {
		std::printf("Error: new cell::Spurs::Spurs2\n");
		std::printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		std::abort();
	}

	// create SPURS instance
	ret = cell::Spurs::Spurs2::initialize(spurs, &attribute);
	if (ret) {
		std::printf("Error: cell::Spurs::Spurs2::initialize(): %#x\n", ret);
		std::printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		std::abort();
	}

	// run this sample
	ret = sample_main(spurs);
	if (ret) {
		std::printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		std::abort();
	}

	// destroy SPURS instance
	ret = spurs->finalize();
	if (ret) {
		std::printf("Error: spurs->finalize(): %#x\n", ret);
		std::printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		std::abort();
	}

	// free memory
	delete spurs;

	// finalize spu_printf server
	ret = spu_printf_finalize();
	if (ret) {
		std::printf("Error: spu_printf_finalize(): %#x\n", ret);
		std::printf("## libspurs : " SAMPLE_NAME " FAILED ##\n");
		std::abort();
	}

	//std::printf("## libspurs : " SAMPLE_NAME " SUCCEEDED ##\n");

	return 0;
}

// Local Variables:
// mode: C++
// c-file-style: "stroustrup"
// tab-width: 4
// End:
// vim:sw=4:sts=4:ts=4
