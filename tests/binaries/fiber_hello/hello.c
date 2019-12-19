/* SCE CONFIDENTIAL
PlayStation(R)3 Programmer Tool Runtime Library 400.001
* Copyright (C) 2008 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#include <cell/sysmodule.h>
#include <cell/fiber/ppu_fiber.h>
#include <sys/process.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define SAMPLE_NAME						"sample_fiber_hello"
#define PRIMARY_PPU_THREAD_PRIORITY		1001
#define PRIMARY_PPU_THREAD_STACK_SIZE	0x10000
SYS_PROCESS_PARAM(PRIMARY_PPU_THREAD_PRIORITY, PRIMARY_PPU_THREAD_STACK_SIZE);

#define	FIBER_HELLO_PRIORITY	1
#define	FIBER_HELLO_STACK_SIZE	2048	// printf() consumes stack memory much.

/* Fiber */

static
int fiber_entry_Hello(uint64_t arg)
{
	(void)arg;

	printf("Hello, fiber!\n");

	return 0;
}

int main(int argc, char* argv[])
{
	(void)argc;
	(void)argv;

	int ret;

	/* Load the libfiber PRX. */

	ret = cellSysmoduleLoadModule(CELL_SYSMODULE_FIBER);
	if (ret != CELL_OK) {
		printf("cellSysmoduleLoadModule failed (0x%x)\n", ret);
		printf("## libfiber : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	/* Initialize the libfiber internal system. */

	ret = cellFiberPpuInitialize();
	if (ret != CELL_OK) {
		printf("cellFiberPpuInitialize failed (0x%x)\n", ret);
		printf("## libfiber : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	/* Create a fiber scheduler. */

	CellFiberPpuScheduler* scheduler =
		(CellFiberPpuScheduler*)memalign(CELL_FIBER_PPU_SCHEDULER_ALIGN, CELL_FIBER_PPU_SCHEDULER_SIZE);
	if (scheduler == NULL) {
		printf("memalign failed\n");
		printf("## libfiber : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	CellFiberPpuSchedulerAttribute schedAttr;
	ret = cellFiberPpuSchedulerAttributeInitialize(&schedAttr);
	assert(ret == CELL_OK);

	schedAttr.debuggerSupport = true;

	ret = cellFiberPpuInitializeScheduler(scheduler, &schedAttr);
	if (ret != CELL_OK) {
		printf("cellFiberPpuInitializeScheduler failed (0x%x)\n", ret);
		printf("## libfiber : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	/* Create a fiber. */

	CellFiberPpu* fiberHello = (CellFiberPpu*)memalign(CELL_FIBER_PPU_ALIGN, CELL_FIBER_PPU_SIZE);
	void* fiberHelloStack = memalign(CELL_FIBER_PPU_STACK_ALIGN, FIBER_HELLO_STACK_SIZE);

	if (fiberHello == NULL || fiberHelloStack == NULL) {
		printf("memalign failed\n");
		printf("## libfiber : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	CellFiberPpuAttribute attr;
	ret = cellFiberPpuAttributeInitialize(&attr);

	strncpy(attr.name, "Sample Hello Fiber", CELL_FIBER_PPU_NAME_MAX_LENGTH);

	ret = cellFiberPpuCreateFiber(
			scheduler, fiberHello,
			fiber_entry_Hello,	// entry function
			0ULL,		// arg
			FIBER_HELLO_PRIORITY,
			fiberHelloStack, FIBER_HELLO_STACK_SIZE,
			&attr
		);
	if (ret != CELL_OK) {
		printf("cellFiberPpuCreate failed (0x%x)\n", ret);
		printf("## libfiber : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	/* Start to run a fiber. */

	ret = cellFiberPpuRunFibers(scheduler);	// Beware the stack size of the caller!
	if (ret != CELL_OK) {
		printf("cellFiberPpuRun failed (0x%x)\n", ret);
		printf("## libfiber : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	/* Return resources of a fiber. */

	int status;
	ret = cellFiberPpuJoinFiber(fiberHello, &status);
	if (ret != CELL_OK) {
		printf("cellFiberPpuJoin failed (0x%x)\n", ret);
		printf("## libfiber : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	assert(status == 0);

	free(fiberHello);
	free(fiberHelloStack);

	/* Destroy the fiber scheduler. */

	ret = cellFiberPpuFinalizeScheduler(scheduler);
	if (ret != CELL_OK) {
		printf("cellFiberPpuFinalizeScheduler failed (0x%x)\n", ret);
		printf("## libfiber : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	free(scheduler);

	/* Unload the libfiber PRX. */

	ret = cellSysmoduleUnloadModule(CELL_SYSMODULE_FIBER);
	if (ret != CELL_OK) {
		printf("cellSysmoduleUnloadModule failed (0x%x)\n", ret);
		printf("## libfiber : " SAMPLE_NAME " FAILED ##\n");
		abort();
	}

	printf("## libfiber : " SAMPLE_NAME " SUCCEEDED ##\n");

	return 0;
}
