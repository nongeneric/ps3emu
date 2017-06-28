/* SCE CONFIDENTIAL                                    */
/* PlayStation(R)3 Programmer Tool Runtime Library 400.001                                           */
/* Copyright (C) 2008 Sony Computer Entertainment Inc. */
/* All Rights Reserved.                                */

#include <cstdio>
#include <cstdlib>
#include <cassert>
#include "common.h"
#include <cell/sync/barrier.h>
#include <hl_spurs.h>
#include <spu_printf.h>

using namespace sce::hl::spurs;
Spurs spurs;
Taskset taskset;
Task task[POISSON_SPU_NUM];

static float x[NX][NY] __attribute__((aligned(128)));
static float r[NX][NY] __attribute__((aligned(128)));
static float p0[NX][NY] __attribute__((aligned(128)));
static float p1[NX][NY] __attribute__((aligned(128)));
static float ap[NX][NY] __attribute__((aligned(128)));
static ScalarData scalar[POISSON_SPU_NUM] __attribute__((aligned(16)));
CellSyncBarrier barrier;

DataAddress trans;

/* embedded SPU ELF symbols */
extern char _binary_spu_elf_start[];

// for test & debug
void printMatrix(float m[NX][NY])
{
	for (int i = 0; i < NX; i++) {
		for (int j = 0; j < NY; j++) {
			std::printf("%01.2f ", m[i][j]);
		}

		std::printf("\n");
	}

	std::printf("\n");
}

float *poissonSolver(void)
{
	// initialize
	trans.nx = NX;
	trans.ny = NY;
	trans.sync_ea = (uint64_t) & barrier;
	trans.scalar_ea = (uint64_t) scalar;
	trans.x_ea = (uint64_t) x;
	trans.r_ea = (uint64_t) r;
	trans.p_ea[0] = (uint64_t) p0;
	trans.p_ea[1] = (uint64_t) p1;
	trans.ap_ea = (uint64_t) ap;
	int ret;

	cellSyncBarrierInitialize(&barrier, POISSON_SPU_NUM);

	// initialize & configuration of variables
	for (uint32_t i = 0; i < NX; i++) {
		for (uint32_t j = 0; j < NY; j++) {
			r[i][j] = 0.0f;
			x[i][j] = 0.0f;
			p0[i][j] = x[i][j];
		}
	}

	// scene setting
	const int div = 5;
	for (int i = 1; i < div; i++) {
		r[NX * 1 / 4][NY * i / div] = +(float)(i + 1);
		r[NX * 3 / 4][NY * i / div] = -(float)(i + 1);
	}

	ret = spu_printf_initialize(Spurs::DEFAULT_PPU_THREAD_PRIORITY + 1, 0);
	assert(ret == CELL_OK);

	// Step.1 initialize SPURS
	ret = Spurs::initialize(&spurs, "poisson", POISSON_SPU_NUM, Spurs::DEFAULT_SPU_THREAD_GROUP_PRIORITY, Spurs::DEFAULT_PPU_THREAD_PRIORITY, Spurs::TYPE_SHARED_WITH_HIGHER);
	assert(ret == CELL_OK);
	// Step.2 create SPURS taskset
	ret = Taskset::create(&taskset, "poisson", &spurs, 0);
	assert(ret == CELL_OK);
	// Step.3 create SPURS task
	for (int i = 0; i < POISSON_SPU_NUM; i++) {
		CellSpursTaskArgument arg;
		arg.u64[0] = (uint64_t) &trans;
		arg.u32[2] = i;
		ret = Task::create(&task[i], &taskset, _binary_spu_elf_start, &arg);
		assert(ret == CELL_OK);
	}

	for (int i = 0; i < POISSON_SPU_NUM; i++) {
		ret = task[i].join();
		assert(ret == CELL_OK);
	}
	ret = taskset.shutdown();
	assert(ret == CELL_OK);
	ret = taskset.join();
	assert(ret == CELL_OK);

	ret = spurs.finalize();
	assert(ret == CELL_OK);

	//printMatrix(p0);
	//printMatrix(p1);
	//printMatrix(r);
	//printMatrix(x);
	//printMatrix(ap);

	ret = spu_printf_finalize();
	assert(ret == CELL_OK);
	std::printf("SPURS task finished\n");
	return &x[0][0];
}

/*
 * Local Variables:
 * mode: C
 * c-file-style: "stroustrup"
 * tab-width: 4
 * End:
 * vim:sw=4:sts=4:ts=4
 */
